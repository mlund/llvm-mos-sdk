# Copyright 2026 LLVM-MOS Project
# Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
# See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
# information.
"""The half of the banked MEGA65 converters that does not depend on the output.

Sizes come from the ELF rather than being restated: getting them out of step
slices at the wrong offset and the banks load as garbage.
"""

import re
import struct
import subprocess
from pathlib import Path

BANKS = 15
WINDOW = 0x6000

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


def banks(image, sym, main_size, slot):
    """(bank, bytes) for each bank the link put something in.

    Only the used part of a slot is returned, so three banks do not carry
    twelve empty ones.
    """
    for i in range(1, BANKS + 1):
        used = sym.get(f"__bank_{i}_size", 0)
        if used:
            start = main_size + (i - 1) * slot
            yield i, image[start : start + used]


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
    """The distinct layouts the program's files recorded: 16 bases each."""
    data = section(elf, ".mapper_layout")
    return {struct.unpack_from("<16I", data, i) for i in range(0, len(data) // 64 * 64, 64)}


def layout_problems(bases, kernal):
    """Why a layout cannot work, or [] if it can."""
    problems = []
    reserved = dict(RESERVED, **(KERNAL_RESERVED if kernal else {}))
    for n in range(1, BANKS + 1):
        base, end = bases[n], bases[n] + WINDOW
        if base & 0xFF:
            problems.append(f"bank {n} at ${base:07X} is not page-aligned")
        if not (end <= 0x60000 or (0x8000000 <= base and end <= 0x8800000)):
            problems.append(f"bank {n} at ${base:07X} is outside chip and attic RAM")
        for what, (start, stop) in reserved.items():
            if base < stop and end > start:
                problems.append(f"bank {n} at ${base:07X} overlaps {what}")
        if kernal and not base & 0xFF00:
            problems.append(f"bank {n} at ${base:07X} has a $00 high byte, which KERNAL LOAD corrupts")
        for m in range(n + 1, BANKS + 1):
            if base < bases[m] + WINDOW and bases[m] < end:
                problems.append(f"banks {n} and {m} overlap")
    return problems


def check_layout(elf, kernal):
    """Problems with the layout the program's files recorded."""
    found = layouts(elf)
    if len(found) > 1:
        return ["files disagree on the bank layout; define MAPPER_BANK_n the same in every file"]
    return layout_problems(next(iter(found)), kernal) if found else []
