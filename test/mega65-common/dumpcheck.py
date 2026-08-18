#!/usr/bin/env python3
# Copyright 2026 LLVM-MOS Project
# Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
# See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
# information.
"""Run a MEGA65 test under xemu and assert on the memory it left behind.

The $D6CF exit-code protocol in xemu-test.h can say one byte about why a test
failed.  This says as much as the test cared to write down: `-dumpmem` saves
the whole of main_ram on exit (targets/mega65/mega65.c, dump_memory), and
main_ram is indexed by physical address, so a test that stores a value at
$CF00 is checked by reading offset 0xCF00 of the dump.

That covers chip and fast RAM.  Attic RAM is a separate array in xemu and
never appears here; a test that needs it wants the serial monitor instead.

    dumpcheck.py --emulator xmega65 --d81 t.d81 --dump t.bin CF00=00 CF01=A5

Each check is ADDR=BYTES, both hex, BYTES as long as you like:
`CF00=00A5` asserts two bytes.  A failure prints what was there instead.
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


def parse_check(text: str) -> tuple[int, bytes]:
    """`CF00=00A5` -> (0xCF00, b'\\x00\\xa5')."""
    address, _, expected = text.partition("=")
    if not _:
        raise argparse.ArgumentTypeError(f"expected ADDR=BYTES, got {text!r}")
    if len(expected) % 2:
        raise argparse.ArgumentTypeError(f"odd hex digit count in {text!r}")
    return int(address, 16), bytes.fromhex(expected)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--emulator", required=True)
    parser.add_argument("--d81", required=True)
    parser.add_argument("--dump", required=True, help="where xemu writes main_ram")
    parser.add_argument("--timeout", type=float, default=30.0)
    parser.add_argument("checks", nargs="+", type=parse_check, metavar="ADDR=BYTES")
    args = parser.parse_args()

    dump = Path(args.dump)
    # A dump left by an earlier run would be read as this run's result, and a
    # test that never got as far as writing would look like it passed.
    dump.unlink(missing_ok=True)

    argv = [
        args.emulator,
        "-headless",
        "-sleepless",
        "-testing",
        "-8", args.d81,
        "-dumpmem", str(dump),
    ]
    # Held back rather than inherited: xemu narrates its whole boot, which buries
    # the one line that says what went wrong.  Shown below only if something did.
    try:
        completed = subprocess.run(
            argv, timeout=args.timeout, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, text=True,
        )
    except subprocess.TimeoutExpired as expired:
        print(f"FAIL: {args.emulator} did not exit within {args.timeout:g}s", file=sys.stderr)
        print(expired.stdout or "", file=sys.stderr)
        return 1

    def failed(message: str) -> int:
        print(f"FAIL: {message}", file=sys.stderr)
        print(completed.stdout, file=sys.stderr)
        return 1

    # The program's own $D6CF exit code still matters -- it is how a test says
    # it failed before reaching the point where it would have written anything.
    if completed.returncode != 0:
        return failed(f"exit code {completed.returncode} from the test program")

    if not dump.exists():
        return failed(f"no memory dump at {dump}")
    memory = dump.read_bytes()

    wrong = []
    for address, expected in args.checks:
        end = address + len(expected)
        if end > len(memory):
            wrong.append(f"${address:05X} is past the {len(memory)}-byte dump")
            continue
        found = memory[address:end]
        if found != expected:
            wrong.append(
                f"${address:05X} is {found.hex().upper()}, expected {expected.hex().upper()}"
            )
    if wrong:
        # Every mismatch, not just the first: which of them are wrong together
        # is usually what says where the run went off course.
        return failed("; ".join(wrong))
    return 0


if __name__ == "__main__":
    sys.exit(main())
