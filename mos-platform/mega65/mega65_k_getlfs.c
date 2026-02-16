// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/// Read the current file parameters (GETLFS, $FF44).
/// Useful to determine the boot device before performing other disk I/O.
/// @note KERNAL GETLFS preserves Z; no ldz #0 needed.
void mega65_k_getlfs(unsigned char *la, unsigned char *fa, unsigned char *sa) {
  unsigned char a, x, y;
  __attribute__((leaf)) asm volatile("jsr __GETLFS" : "=a"(a), "=x"(x),
                                     "=y"(y) : /* no inputs */
                                     : "p");
  *la = a;
  *fa = x;
  *sa = y;
}
