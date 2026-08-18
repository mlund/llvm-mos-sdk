#!/usr/bin/env python3
# Copyright 2026 LLVM-MOS Project
# Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
# See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
# information.
"""Assert a built artifact is no larger than it should be.

A banked layout emits one slot per declared bank whether or not anything was
put in it, so the size of the output is the clearest statement of how many
banks the link actually reserved.  Checking it is how a test says "this
program declared one bank" without reading the linker script back.

    sizecheck.py --max 100000 hello.prg
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--max", type=int, required=True, help="largest acceptable size")
    parser.add_argument("path")
    args = parser.parse_args()

    artifact = Path(args.path)
    if not artifact.exists():
        print(f"FAIL: {artifact} was not built", file=sys.stderr)
        return 1

    size = artifact.stat().st_size
    if size > args.max:
        print(
            f"FAIL: {artifact.name} is {size} bytes, over the {args.max}-byte budget",
            file=sys.stderr,
        )
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
