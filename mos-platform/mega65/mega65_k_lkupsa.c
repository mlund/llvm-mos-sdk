// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/// Search for a secondary address in use (LKUPSA, $FF62).
/// @return 0 if found (writes LA and device to output params), 1 if not found.
/// @note KERNAL LKUPSA preserves Z; no ldz #0 needed.
unsigned char mega65_k_lkupsa(unsigned char sa, unsigned char *la,
                              unsigned char *fa) {
  unsigned char a, x, result;
  __attribute__((leaf)) asm volatile("jsr __LKUPSA" : "=a"(a), "=x"(x),
                                     "=c"(result) : "y"(sa) : "p");
  *la = a;
  *fa = x;
  return result;
}
