// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// Passing arguments to a bank, and taking a value back. The table and the code
// that reads it stay in bank 1; the fixed region only asks for a colour, so
// nothing about the table has to be visible outside the bank.

#include <cstdint>
#include <mapper.h>
#include <mega65.h>

MAPPER_BANK_COUNT(1);

RODATA_BANK(1)
const uint8_t ramp[8] = {COLOR_BLACK,     COLOR_BLUE,  COLOR_PURPLE,
                         COLOR_LIGHTBLUE, COLOR_WHITE, COLOR_LIGHTBLUE,
                         COLOR_PURPLE,    COLOR_BLUE};

CODE_BANK(1) uint8_t shade(uint8_t step, uint8_t offset) {
  return ramp[static_cast<uint8_t>(step + offset) & 7];
}

int main() {
  // Border and screen run half the ramp apart, so the argument is doing the
  // work rather than the call.
  for (uint8_t t = 0;; ++t) {
    VICIV.bordercol = banked_call_r(1, shade, t, 0);
    VICIV.screencol = banked_call_r(1, shade, t, 4);

    for (volatile uint16_t delay = 0; delay < 6000; ++delay)
      ;
  }
}
