#!/usr/bin/env python3
# Copyright 2026 LLVM-MOS Project
# Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
# See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
# information.
"""Check the bank tables in built images against the layout each records.

<mapper.h> derives the MAP and loader tables with preprocessor arithmetic; this
recomputes them independently from the recorded bases and compares bytes.
Usage: check-bank-tables.py IMAGE.prg...  (each beside its .prg.elf)
"""

import sys
from pathlib import Path

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "mos-platform" / "mega65-common"))
import bank_image  # noqa: E402


SELECT = {24 * 1024: 0xE0, 16 * 1024: 0x60, 8 * 1024: 0x20}
MAP_TABLES = ("__bank_offset_lo", "__bank_maplo_sel", "__bank_megabyte")


def expected(bases, sizes):
    off = [((b & 0xFFFFF) - 0x2000) & 0xFFFFF for b in bases]
    return {
        "__bank_offset_lo": [0] + [o >> 8 & 0xFF for o in off[1:]],
        "__bank_maplo_sel": [0] + [SELECT[size] | o >> 16 for size, o in zip(sizes[1:], off[1:])],
        "__bank_megabyte": [b >> 20 & 0xFF for b in bases],
        "__bank_addr_mid": [b >> 16 & 0xFF for b in bases],
        "__bank_addr_page": [b >> 8 & 0xFF for b in bases],
    }


def check(prg):
    elf = Path(str(prg) + ".elf")
    image = Path(prg).read_bytes()
    load = int.from_bytes(image[:2], "little")
    found = bank_image.layouts(elf)
    if len(found) != 1:
        return [f"{prg}: {len(found)} distinct layouts recorded, expected 1"]
    record = next(iter(found))
    bases, sizes = bank_image.split_layout(record)
    count = bank_image.layout_count(record)
    problems = [f"{prg}: {p}" for p in bank_image.layout_problems(bases, sizes, count)]
    addr = {name: value for name, (value, _) in bank_image.symbol_table(elf).items()}
    for name, want in expected(bases, sizes).items():
        want = want[: count + 1]
        if name not in addr:
            # LTO folds a C loader's reads into constants and drops the table,
            # but set_bank reads the MAP tables from assembly.
            if name in MAP_TABLES and "__set_bank_asm" in addr:
                problems.append(f"{prg}: {name} not linked")
            continue
        start = 2 + addr[name] - load
        got = list(image[start : start + count + 1])
        if got != want:
            problems.append(f"{prg}: {name} is {bytes(got).hex(' ')}, expected {bytes(want).hex(' ')}")
    return problems


problems = [p for prg in sys.argv[1:] for p in check(prg)]
for p in problems:
    print(f"FAIL: {p}", file=sys.stderr)
sys.exit(1 if problems or len(sys.argv) < 2 else 0)
