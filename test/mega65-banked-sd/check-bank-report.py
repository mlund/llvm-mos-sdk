#!/usr/bin/env python3
# Copyright 2026 LLVM-MOS Project
# Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
# See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
# information.
"""Check the fill report the converter derives from the linker's symbols.

The arithmetic is fed a symbol table directly, so this needs no ELF, no
toolchain and no emulator.
"""

import sys
from pathlib import Path

# Importing from the platform tree would otherwise leave __pycache__ there.
sys.dont_write_bytecode = True
COMMON = Path(__file__).resolve().parents[2] / "mos-platform" / "mega65-common"
sys.path.insert(0, str(COMMON))
import bank_image  # noqa: E402

WINDOW = 0x6000


def main():
    failures = []

    def check(cond, what):
        if not cond:
            failures.append(what)

    sizes = {"__bank_window_size": WINDOW, "__bank_1_size": 0x100, "__bank_4_size": WINDOW}
    rows = list(bank_image.bank_rows(sizes))

    # Only banks the link put something in, in bank order.
    check([r[0] for r in rows] == [1, 4], f"reported banks {[r[0] for r in rows]}")

    for bank, used, free in rows:
        check(used + free == WINDOW, f"bank {bank}: {used} + {free} != {WINDOW}")
        check(free >= 0, f"bank {bank}: negative free {free}")

    # A bank filled to the window is full, not overflowing.
    check(rows[-1][2] == 0, f"full bank reported {rows[-1][2]} free")

    # Nothing declared means nothing to report, not a crash.
    empty = list(bank_image.bank_rows({"__bank_window_size": WINDOW}))
    check(empty == [], "empty link reported banks")

    for f in failures:
        print(f"FAIL: {f}", file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
