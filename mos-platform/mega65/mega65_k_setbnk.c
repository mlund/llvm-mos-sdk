// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/// Set bank for LOAD/SAVE/VERIFY/OPEN data and filename addresses (SETBNK,
/// $FF6B).
/// @note KERNAL SETBNK preserves Z; no ldz #0 needed.
void mega65_k_setbnk(unsigned char mem_bank, unsigned char fn_bank) {
  __attribute__((leaf)) asm volatile("jsr __SETBNK" : /* no outputs */
                                     : "a"(mem_bank), "x"(fn_bank) : "y", "p");
}

/// Set 28-bit bank for LOAD/SAVE/VERIFY/OPEN data and filename addresses
/// (SETBNK, $FF6B).
/// @note KERNAL SETBNK preserves Z, but this wrapper sets Z=fn_hi via taz
///       before the call; ldz #0 required to restore Z=0.
void mega65_k_setbnk_28(unsigned char mem_mb, unsigned char mem_hi,
                        unsigned char fn_mb, unsigned char fn_hi) {
  // Bit 7 signals 28-bit mode to the KERNAL
  unsigned char a = 0x80 | mem_mb;
  unsigned char x = 0x80 | fn_mb;
  __attribute__((leaf)) asm volatile("pha\n"
                                     "lda %3\n"
                                     "taz\n"
                                     "pla\n"
                                     "jsr __SETBNK\n"
                                     "ldz #0\n" : /* no outputs */
                                     : "a"(a),
                                     "x"(x), "y"(mem_hi), "r"(fn_hi) : "p");
}
