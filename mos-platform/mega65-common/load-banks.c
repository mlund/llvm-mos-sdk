// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// CRT init: load every non-empty bank, after .data and .bss are in place and
// before constructors, which may already call into one. __load_banks is the
// platform's default loader unless the program chose another; each is a
// library member, so the choice is not left to link order.

asm(".section .init.250,\"ax\",@progbits\n"
    "jsr __load_banks\n");
