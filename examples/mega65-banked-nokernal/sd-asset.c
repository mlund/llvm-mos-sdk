// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// Loading an asset the build put on the card beside the banks. The CRT does
// this for banks; anything else is the program's own call.

#include <mapper.h>
#include <mega65.h>
#include <stdint.h>

// Bank 1 carries no linked content, only the file loaded into it below.
MAPPER_BANK_COUNT(1);

#define WINDOW ((const volatile uint8_t *)0x2000)

int main(void) {
  // Upper case: Hyppo upper-cases the name it is asked for, never the one on
  // the card.
  if (mega65_h_setname("STRIPES.BIN") || mega65_h_loadfile(BANK_PHYS_BASE_1)) {
    VICIV.bordercol = COLOR_RED;
    for (;;)
      asm volatile("");
  }

  set_bank(1);
  for (uint8_t i = 0;; ++i)
    VICIV.bordercol = WINDOW[i];
}
