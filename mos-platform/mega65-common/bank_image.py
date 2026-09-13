# Copyright 2026 LLVM-MOS Project
# Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
# See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
# information.
"""The half of the banked MEGA65 converters that does not depend on the output.

Sizes come from the ELF rather than being restated: getting them out of step
slices at the wrong offset and the banks load as garbage.
"""

import re
import subprocess

BANKS = 15


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
