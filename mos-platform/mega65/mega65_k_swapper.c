// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/// Toggle between 40x25 and 80x25 text modes (SWAPPER, $FF65).
/// @note KERNAL SWAPPER modifies Z via set_screen_mode; ldz #0 required.
void mega65_k_swapper(void) {
  __attribute__((leaf)) asm volatile("jsr __SWAPPER\n"
                                     "ldz #0" : /* no outputs */
                                     :          /* no inputs */
                                     : "a",
                                     "x", "y", "p");
}
