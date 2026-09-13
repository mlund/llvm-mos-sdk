#!/usr/bin/env python3
# Copyright 2026 LLVM-MOS Project
# Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
# See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
# information.
"""Split a mega65-banked PRG into main + bank files and build a bootable D81.

link.ld emits one flat image: a 2-byte load address, the $2001-$7FFF window,
__ram_fixed_size bytes of ram_fixed, then one 24 KB slot per declared bank.
Only the used part of each bank is written, so an image with three banks does
not carry twelve empty ones.
"""

import argparse
import sys
from pathlib import Path

# d81.py and bank_image.py sit beside this script once installed, and in
# mega65-common in the source tree.
_HERE = Path(__file__).resolve().parent
sys.path[:0] = [str(_HERE)] + [str(p.parent) for p in _HERE.parent.glob("*/d81.py")]

import bank_image  # noqa: E402
import d81  # noqa: E402

RAM_SIZE = 24575  # $2001-$7FFF, the bank 0 window
PRG_HEADER = b"\x00\x20"  # KERNAL LOAD with SA=0 discards it and uses our address

# Banks 1-9 take a decimal digit, 10-15 a hex letter, matching the filename
# __do_kernal_load builds.
SUFFIXES = "123456789abcdef"


def main(argv=None):
    ap = argparse.ArgumentParser(description="Build a bootable mega65-banked D81.")
    ap.add_argument("prg", help="combined image from the linker")
    ap.add_argument("outdir")
    ap.add_argument("basename")
    ap.add_argument("-n", "--name", help="disk name; defaults to the basename")
    ap.add_argument(
        "--no-autoboot",
        action="store_true",
        help="name the program after the disk instead of autoboot.c65, so it "
        'starts with RUN"<name>" rather than on reset',
    )
    a = ap.parse_intermixed_args(argv)

    prg, outdir = Path(a.prg), Path(a.outdir)
    elf = Path(str(prg) + ".elf")
    image = prg.read_bytes()

    sym = bank_image.symbols(elf)
    problems = bank_image.check_layout(elf, kernal=True)
    if problems:
        raise SystemExit("prg-to-d81.py: " + "; ".join(problems))
    ram_fixed = sym.get("__ram_fixed_size", 0)
    if not ram_fixed:
        raise SystemExit(f"prg-to-d81.py: __ram_fixed_size not found in {elf}")
    main_size = 2 + RAM_SIZE + ram_fixed

    main_prg = outdir / f"{a.basename}-main.prg"
    main_prg.write_bytes(image[:main_size])

    disk_name = (a.name or a.basename)[:16]
    disk = d81.D81(disk_name, "01")
    prg_name = "autoboot.c65" if not a.no_autoboot else disk_name.lower()
    disk.add_file(prg_name, main_prg.read_bytes())

    for i, data in bank_image.banks(image, sym, main_size, sym["__bank_window_size"]):
        suffix = SUFFIXES[i - 1]
        bank = outdir / f"{a.basename}-BANK{suffix.upper()}"
        bank.write_bytes(PRG_HEADER + data)
        disk.add_file(f"bank{suffix}", bank.read_bytes())

    disk.save(str(outdir / f"{a.basename}.d81"))


if __name__ == "__main__":
    main()
