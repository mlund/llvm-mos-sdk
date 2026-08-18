#!/usr/bin/env python3
# Copyright 2026 LLVM-MOS Project
# Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
# See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
# information.
"""Check that the bank layout agrees across the files that each encode it.

Three files state where a bank lives, in three different forms, and nothing but
a SYNC comment holds them together:

  mapper.s            bank_map_table  -- MAP offset, as the MAPLO A/X pair
                      bank_mega_table -- megabyte byte
  load-banks-kernal.S setbnk_y        -- address bits [23:16] for KERNAL LOAD
                      load_addr_hi    -- address bits [15:8]
  mapper.h            BANK_PHYS_BASE_n

Editing one and not the others gives a program that loads its banks to one
address and maps a different one, which shows up as silent garbage rather than
a failure. This recomputes the physical base from each form and compares.

Needs no emulator and no build -- it reads the sources.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

PLATFORM = Path(__file__).resolve().parents[2] / "mos-platform" / "mega65-banked"

# The MAP offset is added to the 16-bit address and truncated to 20 bits, then
# the megabyte byte supplies bits 27-20. Bank 8 relies on that truncation:
# $2000 + $FE800 overflows to $00800. Verified in gs4510.vhdl (resolve_address_
# to_long), where reg_offset_low is unsigned(11 downto 0) -- a 12-bit add whose
# carry is discarded.
WINDOW = 0x2000


def bytes_after(text: str, label: str) -> list[int]:
    """The .byte values following `label:`, up to the next label or directive."""
    start = re.search(rf"^{re.escape(label)}:\s*$", text, re.M)
    if not start:
        raise SystemExit(f"FAIL: no label {label!r} found")
    values: list[int] = []
    for line in text[start.end():].splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith((";", "//")):
            continue
        if not stripped.startswith(".byte"):
            break  # next label or directive ends the table
        body = stripped[len(".byte"):].split(";")[0]
        values += [int(v.strip().lstrip("$"), 16) for v in body.split(",") if v.strip()]
    return values


def main() -> int:
    mapper_s = (PLATFORM / "mapper.s").read_text()
    kernal_s = (PLATFORM / "load-banks-kernal.S").read_text()
    mapper_h = (PLATFORM / "mapper.h").read_text()

    map_table = bytes_after(mapper_s, "bank_map_table")
    mega_table = bytes_after(mapper_s, "bank_mega_table")
    setbnk_y = bytes_after(kernal_s, "setbnk_y")
    load_addr_hi = bytes_after(kernal_s, "load_addr_hi")

    declared = {
        int(n): int(v, 16)
        for n, v in re.findall(r"#define BANK_PHYS_BASE_(\d+)\s+0x([0-9A-Fa-f]+)ul", mapper_h)
    }

    problems: list[str] = []
    for expected, count, name in ((32, len(map_table), "bank_map_table"),
                                  (16, len(mega_table), "bank_mega_table"),
                                  (15, len(setbnk_y), "setbnk_y"),
                                  (15, len(load_addr_hi), "load_addr_hi"),
                                  (16, len(declared), "BANK_PHYS_BASE_n")):
        if count != expected:
            problems.append(f"{name} has {count} entries, expected {expected}")
    if problems:
        print("FAIL: " + "; ".join(problems), file=sys.stderr)
        return 1

    for bank in range(16):
        # From the MAP tables: A is offset[15:8], the low nibble of X is
        # offset[19:16], and the megabyte byte lands at bits 27-20.
        offset = ((map_table[bank * 2 + 1] & 0x0F) << 16) | (map_table[bank * 2] << 8)
        mapped = (mega_table[bank] << 20) | ((WINDOW + offset) & 0xFFFFF)

        if mapped != declared[bank]:
            problems.append(
                f"bank {bank}: mapper.s gives ${mapped:07X}, "
                f"mapper.h declares ${declared[bank]:07X}")

        if bank == 0:
            continue  # bank 0 is the default mapping; the loader has no entry

        # From the loader tables: banks 1-7 load with A=$80 (bits 27-24 = 0),
        # banks 8-15 with A=$88, matching the cpx #7 in __do_kernal_load.
        i = bank - 1
        megabyte = 0x00 if i < 7 else 0x08
        loaded = (megabyte << 24) | (setbnk_y[i] << 16) | (load_addr_hi[i] << 8)
        if loaded != declared[bank]:
            problems.append(
                f"bank {bank}: load-banks-kernal.S loads to ${loaded:07X}, "
                f"mapper.h declares ${declared[bank]:07X}")

    if problems:
        for problem in problems:
            print(f"FAIL: {problem}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
