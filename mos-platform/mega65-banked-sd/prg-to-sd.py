#!/usr/bin/env python3
# Copyright 2026 LLVM-MOS Project
# Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
# See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
# information.
"""Split a mega65-banked-sd PRG into the directory that goes on the SD card.

link.ld emits one flat image: a 2-byte load address, the $2000-$9FFF window,
__ram_fixed_size bytes of ram_fixed, then one 32 KB slot per declared bank.
Only the used part of each bank is written, so a program with three banks does
not carry twelve empty ones.

Sizes come from the ELF rather than being restated here: getting them out of
step slices at the wrong offset and the banks load as garbage.
"""

import argparse
import re
import subprocess
from pathlib import Path

RAM_SIZE = 0x8000  # $2000-$9FFF, the bank 0 window
BANK_SLOT = 0x8000  # each slot in the combined image, from the FULL() padding
BANKS = 15


def elf_sizes(elf):
    """The linker-defined absolute symbols, by name."""
    out = subprocess.run(["nm", str(elf)], capture_output=True, text=True).stdout
    return {m[1]: int(m[0], 16) for m in re.findall(r"^([0-9a-fA-F]+) A (\S+)$", out, re.M)}


# Hyppo takes names up to 63 characters, and FAT rejects these outright.
NAME_MAX = 63
FORBIDDEN = set('"*/:<>?\\|')


def card_name(name):
    """Upper-case form of a name, since Hyppo cannot find any other."""
    name = name.upper()
    if not name or len(name) > NAME_MAX or set(name) & FORBIDDEN:
        raise SystemExit(f"prg-to-sd.py: {name!r} cannot be a name on the card")
    return name


def main():
    ap = argparse.ArgumentParser(
        description="Split a mega65-banked-sd PRG into an SD card directory."
    )
    ap.add_argument("prg", help="combined image from the linker")
    ap.add_argument("outdir", help="directory to fill; created if absent")
    ap.add_argument("basename", help="names the main PRG")
    ap.add_argument(
        "--asset",
        action="append",
        default=[],
        metavar="PATH",
        help="extra file to copy onto the card; repeatable",
    )
    a = ap.parse_args()

    prg, outdir = Path(a.prg), Path(a.outdir)
    elf = Path(str(prg) + ".elf")
    image = prg.read_bytes()

    size = elf_sizes(elf)
    ram_fixed = size.get("__ram_fixed_size", 0)
    if not ram_fixed:
        raise SystemExit(f"prg-to-sd.py: __ram_fixed_size not found in {elf}")
    main_size = 2 + RAM_SIZE + ram_fixed

    outdir.mkdir(parents=True, exist_ok=True)
    written = set()

    def emit(name, data):
        name = card_name(name)
        if name in written:
            raise SystemExit(f"prg-to-sd.py: {name} written twice")
        written.add(name)
        (outdir / name).write_bytes(data)

    emit(f"{a.basename}.prg", image[:main_size])

    for i in range(1, BANKS + 1):
        used = size.get(f"__bank_{i}_size", 0)
        if not used:
            continue
        start = main_size + (i - 1) * BANK_SLOT
        # Raw, no PRG header: Hyppo loadfile places the whole file at the
        # address it is given.
        emit(f"BANK{i:X}.BIN", image[start : start + used])

    for asset in a.asset:
        src = Path(asset)
        emit(src.name, src.read_bytes())


if __name__ == "__main__":
    main()
