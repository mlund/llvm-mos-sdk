// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

#include "_mapper.h"
#include <mega65.h>

// The border is the only output that needs no setup, and returning would run
// a program whose code is silently absent. In .text.kernal_load because
// mega65-banked calls it before the ROMs over $A000-$CFFF are mapped out.
__attribute__((weak, section(".text.kernal_load"))) void __bank_load_failed(unsigned char bank) {
  (void)bank;
  VICII.bordercolor = 2;
  for (;;)
    asm volatile("");
}
