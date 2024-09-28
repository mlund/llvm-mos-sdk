// Copyright 2024 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

#include <cx16.h>

void waitvsync() {
  [[maybe_unused]] unsigned char tmp; // avoid hard-coded imaginary register
  __attribute__((leaf)) __asm__ volatile(
      R"ASM(
               jsr __RDTIM
               sta %0
            1: jsr __RDTIM
               cmp %0
               beq 1b
      )ASM"
      : "=r"(tmp)::"a", "x", "y", "p");
}
