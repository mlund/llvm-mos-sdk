// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/// Set the CIA1 time-of-day clock (SETTIM, $FFDB).
/// All values are in BCD format (e.g. 0x59 = 59 decimal).
/// @note KERNAL SETTIM preserves Z, but this wrapper sets Z=tenths via taz
///       before the call; ldz #0 required to restore Z=0.
void mega65_k_settim(unsigned char hours, unsigned char minutes,
                     unsigned char seconds, unsigned char tenths) {
  __attribute__((leaf)) asm volatile("pha\n"
                                     "lda %3\n"
                                     "taz\n"
                                     "pla\n"
                                     "jsr __SETTIM\n"
                                     "ldz #0\n" : /* no outputs */
                                     : "a"(seconds),
                                     "x"(minutes), "y"(hours),
                                     "r"(tenths) : "p");
}
