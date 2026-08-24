#!/usr/bin/env python3
# Copyright 2026 LLVM-MOS Project
# Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
# See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
# information.
"""Check that the bank layout says the same thing wherever it is written down.

mapper.h states each bank's physical base; mapper.s states it as MAP register
values; _ram-banked-sd.ld states the slot each bank occupies in the image.
Nothing makes them agree, and a disagreement maps the window somewhere the
loader did not write, which shows up as a bank of zeroes rather than an error.

Needs no emulator, so run it after any edit to the layout.
"""

import re
import sys
from pathlib import Path

PLATFORM = Path(__file__).resolve().parents[2] / "mos-platform" / "mega65-banked-sd"

BANKS = 16
MEGABYTE = 0x100000

# Regions a bank must not sit in.
RESERVED = {
    "bank 0 and the fixed region": (0x00000, 0x10000),
    "the C65 DOS work area": (0x10000, 0x12000),
    "the colour RAM window": (0x1F800, 0x20000),
    "past the RAM the map can reach": (0x60000, 0x8000000),
}


def read_table(text, label):
    """The 16 .byte values following `label:` in mapper.s."""
    body = text.split(label + ":", 1)[1]
    values = []
    for line in body.splitlines()[1:]:
        match = re.match(r"\s*\.byte\s+(.*)", line)
        if not match:
            break
        values += [int(v.strip().lstrip("$"), 16) for v in match.group(1).split(",")]
    if len(values) != BANKS:
        raise SystemExit(f"{label}: expected {BANKS} entries, found {len(values)}")
    return values


def linker_value(text, symbol):
    """A plain `sym = 0x...;` assignment from a linker script."""
    m = re.search(rf"^{re.escape(symbol)}\s*=\s*(0x[0-9A-Fa-f]+);", text, re.M)
    if not m:
        raise SystemExit(f"{symbol} not found")
    return int(m.group(1), 16)


def main():
    header = (PLATFORM / "mapper.h").read_text()
    asm = (PLATFORM / "mapper.s").read_text()
    script = (PLATFORM / "_ram-banked-sd.ld").read_text()
    link = (PLATFORM / "link.ld").read_text()

    # Taken from link.ld rather than restated: the window is what the MAP
    # offsets are relative to, and what each bank occupies.
    window = linker_value(link, "__bank_window_start")
    bank_size = linker_value(link, "__bank_window_size")

    declared = {}
    for n, value in re.findall(r"#define BANK_PHYS_BASE_(\d+)\s+(0x[0-9A-Fa-f]+)ul", header):
        declared[int(n)] = int(value, 16)
    if sorted(declared) != list(range(BANKS)):
        raise SystemExit(f"mapper.h declares banks {sorted(declared)}")

    offset_lo = read_table(asm, "bank_offset_lo")
    maplo = read_table(asm, "bank_maplo_sel")
    megabyte = read_table(asm, "bank_megabyte")

    problems = []

    # Bank 0 is the window with nothing mapped, so every register is zero.
    if (offset_lo[0], maplo[0], megabyte[0]) != (0, 0, 0):
        problems.append("bank 0 must select no blocks, so the window falls back to chip RAM")
    if declared[0] != window:
        problems.append(f"bank 0 is ${declared[0]:05X}, not the window at ${window:05X}")

    for bank in range(1, BANKS):
        want = declared[bank]
        if maplo[bank] >> 4 != 0x0E:
            problems.append(f"bank {bank}: MAPLO selects ${maplo[bank] >> 4:X}, not blocks 1-3")
        offset = ((maplo[bank] & 0x0F) << 16) | (offset_lo[bank] << 8)
        low = (window + offset) % MEGABYTE
        base = (megabyte[bank] << 20) | low
        if base != want:
            problems.append(f"bank {bank}: MAP registers give ${base:07X}, mapper.h says ${want:07X}")

        for what, (start, end) in RESERVED.items():
            if want < end and want + bank_size > start:
                problems.append(f"bank {bank} at ${want:07X} overlaps {what}")

    bases = sorted((base, bank) for bank, base in declared.items())
    for (a, first), (b, second) in zip(bases, bases[1:]):
        if a + bank_size > b:
            problems.append(f"banks {first} and {second} overlap at ${b:07X}")

    # The linker slots only have to give every bank a distinct home whose low
    # 16 bits are the window, so that symbols resolve into it.
    slots = {int(n): int(v, 16) for n, v in re.findall(r"__bank_(\d+)_lma\s*=\s*(0x[0-9A-Fa-f]+);", script)}
    if sorted(slots) != list(range(1, BANKS)):
        raise SystemExit(f"_ram-banked-sd.ld declares slots {sorted(slots)}")
    for bank, lma in slots.items():
        if lma & 0xFFFF != window:
            problems.append(f"bank {bank} slot ${lma:06X} does not resolve into the window")
    if len(set(slots.values())) != len(slots):
        problems.append("two banks share a slot")

    if problems:
        print("\n".join(problems), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
