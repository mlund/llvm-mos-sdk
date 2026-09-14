# Copyright 2026 LLVM-MOS Project
# Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
# See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
# information.
"""Helpers for prg-to-mega65.py, which converts a banked MEGA65 image into an
SD card directory or a D81.

Sizes come from the ELF rather than being restated: getting them out of step
slices at the wrong offset and the banks load as garbage.
"""

import re
import struct
import subprocess
from pathlib import Path

BANKS = 15
# A file's layout record: 16 bases, window KB, bank 1-15 KB, loader.
RECORD = "<33I"

# Regions no bank may overlap, beyond chip RAM's end and attic's bounds.
RESERVED = {
    "bank 0 and the fixed region": (0x00000, 0x10000),
    "the C65 DOS work area": (0x10000, 0x12000),
    "the colour RAM window": (0x1F800, 0x20000),
}
KERNAL_RESERVED = {"the write-protected C65 ROMs": (0x20000, 0x40000)}


def symbols(elf):
    """The linker-defined absolute symbols, by name."""
    out = subprocess.run(["nm", str(elf)], capture_output=True, text=True).stdout
    return {m[1]: int(m[0], 16) for m in re.findall(r"^([0-9a-fA-F]+) A (\S+)$", out, re.M)}


def banks(image, sym, main_size):
    """(bank, bytes) for each bank the link put something in.

    Only the used part of a slot is returned, so three banks do not carry
    twelve empty ones. Each slot is its bank's own length.
    """
    start = main_size
    window, count = sym["__bank_window_size"], sym.get("__bank_count", BANKS)
    for i in range(1, BANKS + 1):
        used = sym.get(f"__bank_{i}_size", 0)
        if used:
            yield i, image[start : start + used]
        if i <= count:
            start += sym.get(f"__bank_{i}_length", window)


def bank_rows(sym, banks=BANKS):
    """(bank, used, free) for each bank the link put something in.

    A slot is the bank's own length, so free is what is left before the next
    thing added to that bank stops linking.
    """
    window = sym["__bank_window_size"]
    for i in range(1, banks + 1):
        used = sym.get(f"__bank_{i}_size", 0)
        if used:
            yield i, used, sym.get(f"__bank_{i}_length", window) - used


def print_report(sym):
    """How full each bank is, while there is still room to act on it."""
    print("bank     used     free   fill")
    for bank, used, free in bank_rows(sym):
        pct = 100 * used // (used + free)
        print(f"{bank:4d} {used:8d} {free:8d}   {pct:3d}%")


def section(elf, name):
    """The bytes of an ELF32 section, or b"" if absent."""
    data = Path(elf).read_bytes()
    (shoff,) = struct.unpack_from("<I", data, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from("<HHH", data, 0x2E)
    header = lambda i: struct.unpack_from("<10I", data, shoff + i * shentsize)
    strings = header(shstrndx)[4]
    for i in range(shnum):
        h = header(i)
        start = strings + h[0]
        if data[start : data.index(b"\0", start)] == name.encode():
            return data[h[4] : h[4] + h[5]]
    return b""


def layouts(elf):
    """The distinct layouts the program's files recorded.

    Each is 16 bases, the window in KB, banks 1-15 in KB, then the loader.
    """
    data = section(elf, ".mapper_layout")
    size = struct.calcsize(RECORD)
    starts = range(0, len(data) // size * size, size)
    return {struct.unpack_from(RECORD, data, i) for i in starts}


def split_layout(record):
    """(bases, sizes in bytes), both indexed by bank; bank 0 is the window."""
    return record[:16], [kb * 1024 for kb in record[16:32]]


def layout_problems(bases, kernal, sizes):
    """Why a layout cannot work, or [] if it can."""
    problems = []
    reserved = dict(RESERVED, **(KERNAL_RESERVED if kernal else {}))
    for n in range(1, BANKS + 1):
        base, end = bases[n], bases[n] + sizes[n]
        if base & 0xFF:
            problems.append(f"bank {n} at ${base:07X} is not page-aligned")
        if base >> 20 != (end - 1) >> 20:
            problems.append(
                f"bank {n} at ${base:07X} crosses a megabyte boundary")
        if not (end <= 0x60000 or (0x8000000 <= base and end <= 0x8800000)):
            problems.append(f"bank {n} at ${base:07X} is outside chip and attic RAM")
        for what, (start, stop) in reserved.items():
            if base < stop and end > start:
                problems.append(f"bank {n} at ${base:07X} overlaps {what}")
        if kernal and not base & 0xFF00:
            problems.append(f"bank {n} at ${base:07X} has a $00 high byte, which KERNAL LOAD corrupts")
        for m in range(n + 1, BANKS + 1):
            if base < bases[m] + sizes[m] and bases[m] < end:
                problems.append(f"banks {n} and {m} overlap")
    return problems


def check_layout(elf, kernal):
    """Problems with the layout the program's files recorded."""
    found = layouts(elf)
    if len(found) > 1:
        return ["files disagree on the bank layout; define MAPPER_BANK_n the same in every file"]
    if not found:
        return []
    bases, sizes = split_layout(next(iter(found)))
    return layout_problems(bases, kernal, sizes)


LOADER_HYPPO, LOADER_FLOPPY, LOADER_KERNAL = 0, 1, 2


def loader(elf):
    """The loader the program's files recorded, or None without one record."""
    found = layouts(elf)
    return next(iter(found))[32] if len(found) == 1 else None
