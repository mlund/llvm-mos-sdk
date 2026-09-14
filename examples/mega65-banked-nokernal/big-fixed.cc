// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// A 16 KB window for a program that outgrows the 20 KB fixed region: the fixed
// region starts at $6000 and holds 28 KB. Bank 1 is 8 KB, which leaves
// $4000-$5FFF as WINDOW_TAIL memory that fixed code and bank 1 both see; bank 2
// keeps 16 KB, covers it, and so takes the colour as an argument.

#define MAPPER_WINDOW_KB 16
#define MAPPER_BANK_1_KB 8
#define MAPPER_BANK_COUNT 2

#include <cstdint>
#include <mapper.h>
#include <mega65.h>

// Stands in for code and data past 20 KB: with a 24 KB window this alone would
// not fit the fixed region.
static const uint8_t ramp[22 * 1024] = {COLOR_RED, COLOR_ORANGE, COLOR_YELLOW,
                                        COLOR_GREEN};

WINDOW_TAIL static uint8_t step;

CODE_BANK(1) void advance() { step = (step + 1) & 3; }

CODE_BANK(2) void show(uint8_t colour) { VICIV.bordercol = colour; }

int main() {
  while (true) {
    banked_call(1, advance);
    banked_call_v(2, show, ramp[step]);
  }
}
