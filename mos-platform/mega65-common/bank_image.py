# Copyright 2026 LLVM-MOS Project
# Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
# See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
# information.
"""Helpers for prg-to-mega65.py, which converts a banked MEGA65 image into an
SD card directory or a D81.

Sizes come from the ELF rather than being restated: getting them out of step
slices at the wrong offset and the banks load as garbage.
"""

import struct
from pathlib import Path

BANKS = 31
# A file's layout record: 32 bases, window KB, bank 1-31 KB, loader, count.
RECORD = "<66I"

# Regions no bank may overlap, beyond chip RAM's end and attic's bounds.
RESERVED = {
    "bank 0 and the fixed region": (0x00000, 0x10000),
    "the C65 DOS work area": (0x10000, 0x12000),
    "the colour RAM window": (0x1F800, 0x20000),
}


def symbols(elf):
    """The linker-defined absolute symbols, by name."""
    table = symbol_table(elf)
    return {name: value for name, (value, ndx) in table.items() if ndx == SHN_ABS}


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


def bank_rows(sym):
    """(bank, used, free) for each bank the link put something in.

    A slot is the bank's own length, so free is what is left before the next
    thing added to that bank stops linking.
    """
    window = sym["__bank_window_size"]
    for i in range(1, BANKS + 1):
        used = sym.get(f"__bank_{i}_size", 0)
        if used:
            yield i, used, sym.get(f"__bank_{i}_length", window) - used


def print_report(sym):
    """How full each bank is, while there is still room to act on it."""
    print("bank     used     free   fill")
    for bank, used, free in bank_rows(sym):
        pct = 100 * used // (used + free)
        print(f"{bank:4d} {used:8d} {free:8d}   {pct:3d}%")


# ELF32 constants: the symbol table's section type, and an absolute symbol.
SHT_SYMTAB = 2
SHN_ABS = 0xFFF1


def _headers(data):
    """An ELF32 image's section headers, and where its section names start."""
    (shoff,) = struct.unpack_from("<I", data, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from("<HHH", data, 0x2E)
    headers = [struct.unpack_from("<10I", data, shoff + i * shentsize)
               for i in range(shnum)]
    return headers, headers[shstrndx][4]


def _string(data, at):
    return data[at : data.index(b"\0", at)].decode()


def section(elf, name):
    """The bytes of an ELF32 section, or b"" if absent."""
    data = Path(elf).read_bytes()
    headers, names = _headers(data)
    for h in headers:
        if _string(data, names + h[0]) == name:
            return data[h[4] : h[4] + h[5]]
    return b""


def symbol_table(elf):
    """Every named symbol's (value, section index), by name.

    Read here rather than through nm, which an LLVM-MOS install does not ship.
    """
    data = Path(elf).read_bytes()
    headers, _ = _headers(data)
    table = {}
    for h in headers:
        if h[1] != SHT_SYMTAB:
            continue
        strings = headers[h[6]][4]
        for at in range(h[4], h[4] + h[5], 16):
            name, value, _, _, _, ndx = struct.unpack_from("<IIIBBH", data, at)
            if name:
                table[_string(data, strings + name)] = (value, ndx)
    return table


def layouts(elf):
    """The distinct layouts the program's files recorded.

    Each is 32 bases, the window in KB, banks 1-31 in KB, the loader, then
    MAPPER_BANK_COUNT.
    """
    data = section(elf, ".mapper_layout")
    size = struct.calcsize(RECORD)
    starts = range(0, len(data) // size * size, size)
    return {struct.unpack_from(RECORD, data, i) for i in starts}


def split_layout(record):
    """(bases, sizes in bytes), both indexed by bank; bank 0 is the window."""
    return record[:32], [kb * 1024 for kb in record[32:64]]


def layout_count(record):
    """The MAPPER_BANK_COUNT a record was made with."""
    return record[65]


def layout_problems(bases, sizes, count=BANKS):
    """Why the first count banks cannot work, or [] if they can.

    <mapper.h> checks each bank on its own at compile time; this checks what
    needs every bank at once, and regions it does not know.
    """
    problems = []
    for n in range(1, count + 1):
        base, end = bases[n], bases[n] + sizes[n]
        for what, (start, stop) in RESERVED.items():
            if base < stop and end > start:
                problems.append(f"bank {n} at ${base:07X} overlaps {what}")
        for m in range(n + 1, count + 1):
            if base < bases[m] + sizes[m] and bases[m] < end:
                problems.append(f"banks {n} and {m} overlap")
    return problems


def check_layout(found, count=BANKS):
    """Problems with the layout the program's files recorded."""
    if len(found) > 1:
        return ["files disagree on the bank layout; define MAPPER_BANK_n the same in every file"]
    if not found:
        return []
    bases, sizes = split_layout(next(iter(found)))
    return layout_problems(bases, sizes, count)


LOADER_HYPPO, LOADER_FLOPPY, LOADER_KERNAL = 0, 1, 2


def loader(found):
    """The loader the program's files recorded, or None without one record."""
    return next(iter(found))[64] if len(found) == 1 else None
