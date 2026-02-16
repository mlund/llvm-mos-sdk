// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

#include <stdint.h>

typedef struct {
  unsigned char width;
  unsigned char height;
  unsigned char addr_lo;
  unsigned char addr_hi;
  unsigned char is_40col;
} mega65_screen_info_t;

/// Get screen window size and properties (SCRORG, $FFED).
/// @note KERNAL SCRORG returns window address high byte in Z; ldz #0 required.
mega65_screen_info_t mega65_k_scrorg(void) {
  mega65_screen_info_t info;
  __attribute__((leaf)) asm volatile(
      "jsr __SCREEN\n"
      "pha\n"
      "tza\n"
      "ldz #0\n"
      "sta %[hi]\n"
      "pla\n" : "=a"(info.addr_lo),
      "=x"(info.width), "=y"(info.height), [hi] "=&r"(info.addr_hi),
      "=c"(info.is_40col) : /* no inputs */
      :);
  return info;
}
