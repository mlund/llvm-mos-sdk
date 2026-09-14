#!/usr/bin/env python3
# Copyright 2026 LLVM-MOS Project
# Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
# See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
# information.
"""Split a mega65-banked-sd PRG into the directory that goes on the SD card.

link.ld emits one flat image: a 2-byte load address, the bank 0 window,
__ram_fixed_size bytes of ram_fixed, then one window-sized slot per bank.
Only the used part of each bank is written, so a program with three banks does
not carry twelve empty ones.
"""

import argparse
import sys
from pathlib import Path

# bank_image.py sits beside this script once installed, and in mega65-common in
# the source tree.
_HERE = Path(__file__).resolve().parent
sys.path[:0] = [str(_HERE)] + [str(p.parent) for p in _HERE.parent.glob("*/bank_image.py")]

import bank_image  # noqa: E402


def bank_rows(size, banks=bank_image.BANKS):
    """(bank, used, free) for each bank the link put something in.

    A slot is the bank's own length, so free is what is left before the next
    thing added to that bank stops linking.
    """
    window = size["__bank_window_size"]
    for i in range(1, banks + 1):
        used = size.get(f"__bank_{i}_size", 0)
        if used:
            yield i, used, size.get(f"__bank_{i}_length", window) - used


def print_report(size):
    """How full each bank is, while there is still room to act on it."""
    print("bank     used     free   fill")
    for bank, used, free in bank_rows(size):
        pct = 100 * used // (used + free)
        print(f"{bank:4d} {used:8d} {free:8d}   {pct:3d}%")


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
    ap.add_argument(
        "--report",
        action="store_true",
        help="print how full each bank is",
    )
    a = ap.parse_args()

    prg, outdir = Path(a.prg), Path(a.outdir)
    elf = Path(str(prg) + ".elf")
    image = prg.read_bytes()

    size = bank_image.symbols(elf)
    problems = bank_image.check_layout(elf, kernal=False)
    if problems:
        raise SystemExit("prg-to-sd.py: " + "; ".join(problems))
    ram_fixed = size.get("__ram_fixed_size", 0)
    if not ram_fixed:
        raise SystemExit(f"prg-to-sd.py: __ram_fixed_size not found in {elf}")
    # The window is also the padded size of every bank slot, since both come
    # from FULL() over a region of that length.
    window = size.get("__bank_window_size", 0)
    if not window:
        raise SystemExit(f"prg-to-sd.py: __bank_window_size not found in {elf}")
    main_size = 2 + window + ram_fixed

    if a.report:
        print_report(size)

    outdir.mkdir(parents=True, exist_ok=True)
    written = set()

    def emit(name, data):
        name = card_name(name)
        if name in written:
            raise SystemExit(f"prg-to-sd.py: {name} written twice")
        written.add(name)
        (outdir / name).write_bytes(data)

    emit(f"{a.basename}.prg", image[:main_size])

    # Raw, no PRG header: Hyppo loadfile places the whole file at the address
    # it is given.
    for i, data in bank_image.banks(image, size, main_size):
        emit(f"BANK{i:X}.BIN", data)

    for asset in a.asset:
        src = Path(asset)
        emit(src.name, src.read_bytes())


if __name__ == "__main__":
    main()
