// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/// Search for a logical file number in use (LKUPLA, $FF5F).
/// @return 0 if found (writes device and SA to output params), 1 if not found.
/// @note KERNAL LKUPLA preserves Z; no ldz #0 needed.
unsigned char mega65_k_lkupla(unsigned char la, unsigned char *fa,
                              unsigned char *sa) {
  unsigned char x, y, result;
  __attribute__((leaf)) asm volatile("jsr __LKUPLA" : "=x"(x), "=y"(y),
                                     "=c"(result) : "a"(la) : "p");
  *fa = x;
  *sa = y;
  return result;
}
