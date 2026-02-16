// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

typedef struct {
  unsigned char hours;
  unsigned char minutes;
  unsigned char seconds;
  unsigned char tenths;
} mega65_tod_t;

/// Read the CIA1 time-of-day clock (RDTIM, $FFDE).
/// All values are in BCD format (e.g. 0x59 = 59 decimal).
/// @note KERNAL RDTIM returns tenths in Z; ldz #0 required.
mega65_tod_t mega65_k_rdtim(void) {
  mega65_tod_t tod;
  __attribute__((leaf)) asm volatile(
      "jsr __RDTIM\n"
      "pha\n"
      "tza\n"
      "ldz #0\n"
      "sta %[tenths]\n"
      "pla\n" : "=a"(tod.seconds),
      "=x"(tod.minutes), "=y"(tod.hours),
      [tenths] "=&r"(tod.tenths) : /* no inputs */
      :);
  return tod;
}
