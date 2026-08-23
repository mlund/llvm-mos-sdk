// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// Minimal banked example: change the border colour from two banks.

#include <cstdint>
#include <mapper.h>
#include <mega65.h>

MAPPER_BANK_COUNT(2);

CODE_BANK(1) void set_red_border() { VICIV.bordercol = COLOR_RED; }

CODE_BANK(2) void set_blue_border() { VICIV.bordercol = COLOR_BLUE; }

int main() {
  while (true) {
    banked_call(1, set_red_border);
    banked_call(2, set_blue_border);
  }
}
