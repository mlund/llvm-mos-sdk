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

Sizes come from the ELF rather than being restated here: getting them out of
step slices at the wrong offset and the banks load as garbage.
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path

# d81.py sits beside this script once installed, and in another platform's
# directory in the source tree.
_HERE = Path(__file__).resolve().parent
sys.path[:0] = [str(_HERE)] + [str(p.parent) for p in _HERE.parent.glob("*/d81.py")]

import d81  # noqa: E402

RAM_SIZE = 24575  # $2001-$7FFF, the bank 0 window
BANK_SLOT = 24576  # each slot in the combined image, from the FULL() padding
PRG_HEADER = b"\x00\x20"  # KERNAL LOAD with SA=0 discards it and uses our address

# Banks 1-9 take a decimal digit, 10-15 a hex letter, matching the filename
# __do_kernal_load builds.
SUFFIXES = "123456789abcdef"


def elf_size(elf, symbol):
    """Value of a linker-defined absolute symbol, or 0 if absent."""
    out = subprocess.run(["nm", str(elf)], capture_output=True, text=True).stdout
    m = re.search(rf"^([0-9a-fA-F]+) A {re.escape(symbol)}$", out, re.M)
    return int(m.group(1), 16) if m else 0


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

    ram_fixed = elf_size(elf, "__ram_fixed_size")
    if not ram_fixed:
        raise SystemExit(f"prg-to-d81.py: __ram_fixed_size not found in {elf}")
    main_size = 2 + RAM_SIZE + ram_fixed

    main_prg = outdir / f"{a.basename}-main.prg"
    main_prg.write_bytes(image[:main_size])

    disk_name = (a.name or a.basename)[:16]
    disk = d81.D81(disk_name, "01")
    prg_name = "autoboot.c65" if not a.no_autoboot else disk_name.lower()
    disk.add_file(prg_name, main_prg.read_bytes())

    for i, suffix in enumerate(SUFFIXES, start=1):
        size = elf_size(elf, f"__bank_{i}_size")
        if not size:
            continue
        start = main_size + (i - 1) * BANK_SLOT
        bank = outdir / f"{a.basename}-BANK{suffix.upper()}"
        bank.write_bytes(PRG_HEADER + image[start : start + size])
        disk.add_file(f"bank{suffix}", bank.read_bytes())

    disk.save(str(outdir / f"{a.basename}.d81"))


if __name__ == "__main__":
    main()
