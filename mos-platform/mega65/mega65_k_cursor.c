// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/// Enable blinking cursor at the current screen editor position (CURSOR,
/// $FF35).
/// @note KERNAL CURSOR (C=0) preserves Z; no ldz #0 needed.
void mega65_k_cursor_enable(void) {
  __attribute__((leaf)) asm volatile("clc\n"
                                     "jsr __CURSOR" : /* no outputs */
                                     :                /* no inputs */
                                     : "a",
                                     "x", "y", "p");
}

/// Disable blinking cursor (CURSOR, $FF35).
/// @note KERNAL CURSOR (C=1) sets Z=pntr (cursor column); ldz #0 required.
void mega65_k_cursor_disable(void) {
  __attribute__((leaf)) asm volatile("sec\n"
                                     "jsr __CURSOR\n"
                                     "ldz #0" : /* no outputs */
                                     :          /* no inputs */
                                     : "a",
                                     "x", "y", "p");
}
