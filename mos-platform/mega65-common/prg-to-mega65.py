#!/usr/bin/env python3
# Copyright 2026 LLVM-MOS Project
# Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
# See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
# information.
"""Convert a banked MEGA65 image into an SD card directory or a D81.

The linked image is a 2-byte load address, the window region, the used part of
ram_fixed, then one slot per declared bank. What is written follows the loader
the program's files recorded:

  KERNAL LOAD (mega65-banked)   BASE.d81, autobooting, holding bank1-bankf
  Hyppo (mega65-banked-nokernal)      OUTDIR as the SD card: BASE.PRG, BANKn.BIN
  F011 (MAPPER_LOADER_FLOPPY)   BASE.PRG, and BASE.D81 holding BANKn and assets
"""

import argparse
import sys
from pathlib import Path

# bank_image.py and d81.py sit beside this script, installed or in the source.
sys.path.insert(0, str(Path(__file__).resolve().parent))

import bank_image  # noqa: E402
import d81  # noqa: E402

PROG = "prg-to-mega65.py"
KERNAL_LOAD_ADDRESS = 0x2001
# KERNAL LOAD with SA=0 discards this header and uses our address.
PRG_HEADER = b"\x00\x20"

# Banks 1-9 take a decimal digit, 10-15 a hex letter, matching the filename
# __do_kernal_load builds.
SUFFIXES = "123456789abcdef"

# Hyppo takes names up to 63 characters, and FAT rejects these outright.
NAME_MAX = 63
FORBIDDEN = set('"*/:<>?\\|')


def fail(message):
    raise SystemExit(f"{PROG}: {message}")


def card_name(name):
    """Upper-case form of a name, since Hyppo cannot find any other."""
    name = name.upper()
    if not name or len(name) > NAME_MAX or set(name) & FORBIDDEN:
        fail(f"{name!r} cannot be a name on the card")
    return name


def write_kernal(a, outdir, image, sym, main_size):
    """A bootable D81, and the files on it beside the image for inspection."""
    main_prg = image[:main_size]
    (outdir / f"{a.basename}-main.prg").write_bytes(main_prg)
    disk_name = (a.name or a.basename)[:16]
    disk = d81.D81(disk_name, "01")
    boot_name = disk_name.lower() if a.no_autoboot else "autoboot.c65"
    disk.add_file(boot_name, main_prg)
    for i, data in bank_image.banks(image, sym, main_size):
        suffix = SUFFIXES[i - 1]
        bank = PRG_HEADER + data
        (outdir / f"{a.basename}-BANK{suffix.upper()}").write_bytes(bank)
        disk.add_file(f"bank{suffix}", bank)
    for asset in a.asset:
        disk.add_file(Path(asset).stem, Path(asset).read_bytes())
    disk.save(str(outdir / f"{a.basename}.d81"))


def write_card(a, outdir, image, sym, main_size, floppy):
    """The SD card directory; for the F011 loader, banks go on a D81 in it."""
    written = set()

    def emit(name, data):
        name = card_name(name)
        if name in written:
            fail(f"{name} written twice")
        written.add(name)
        (outdir / name).write_bytes(data)

    emit(f"{a.basename}.prg", image[:main_size])
    # Raw, no PRG header: both loaders place the whole file at the bank's base.
    if floppy:
        disk = d81.D81((a.name or a.basename)[:16], "01")
        for i, data in bank_image.banks(image, sym, main_size):
            disk.add_file(f"BANK{i:X}", data)
        # Assets go where the program can reach them without a ROM.
        for asset in a.asset:
            disk.add_file(Path(asset).stem, Path(asset).read_bytes())
        name = card_name(f"{a.basename}.d81")
        written.add(name)
        disk.save(str(outdir / name))
    else:
        for i, data in bank_image.banks(image, sym, main_size):
            emit(f"BANK{i:X}.BIN", data)
        for asset in a.asset:
            src = Path(asset)
            emit(src.name, src.read_bytes())


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("prg", help="combined image from the linker")
    ap.add_argument("outdir", help="directory to write into; created if absent")
    ap.add_argument("basename", help="names the main program and any disk")
    ap.add_argument("--asset", action="append", default=[], metavar="PATH",
                    help="extra file to put beside the banks; repeatable")
    ap.add_argument("--report", action="store_true",
                    help="print how full each bank is")
    ap.add_argument("-n", "--name", help="disk name; defaults to the basename")
    ap.add_argument("--no-autoboot", action="store_true",
                    help="mega65-banked: name the program after the disk, so "
                    'it starts with RUN"<name>" and not on reset')
    a = ap.parse_intermixed_args()

    prg, outdir = Path(a.prg), Path(a.outdir)
    elf = Path(str(prg) + ".elf")
    image = prg.read_bytes()
    sym = bank_image.symbols(elf)
    load = int.from_bytes(image[:2], "little")
    kernal = load == KERNAL_LOAD_ADDRESS

    problems = bank_image.check_layout(elf, kernal)
    if problems:
        fail("; ".join(problems))
    loader = bank_image.loader(elf)
    if loader is None:
        loader = bank_image.LOADER_KERNAL if kernal else bank_image.LOADER_HYPPO
    if kernal != (loader == bank_image.LOADER_KERNAL):
        fail("the image's platform and its recorded loader disagree")
    for name in ("__ram_fixed_start", "__ram_fixed_size", "__bank_window_size"):
        if not sym.get(name):
            fail(f"{name} not found in {elf}")
    main_size = 2 + sym["__ram_fixed_start"] - load + sym["__ram_fixed_size"]

    if a.report:
        bank_image.print_report(sym)
    outdir.mkdir(parents=True, exist_ok=True)
    if kernal:
        write_kernal(a, outdir, image, sym, main_size)
    else:
        write_card(a, outdir, image, sym, main_size,
                   loader == bank_image.LOADER_FLOPPY)


if __name__ == "__main__":
    main()
