// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// Minimal banked example: change VIC-IV border color from two banks.

#include <cstdint>
#include <mapper.h>
#include <mega65.h>

__attribute__((noinline, section(".bank_1")))
void set_red_border() {
  VICIV.bordercol = COLOR_RED;
}

__attribute__((noinline, section(".bank_2")))
void set_blue_border() {
  VICIV.bordercol = COLOR_BLUE;
}

int main() {
  while (true) {
    banked_call(1, set_red_border);
    banked_call(2, set_blue_border);
  }
}
