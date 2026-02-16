// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/// Enable or disable KERNAL messages (SETMSG, $FF90).
/// Bit 7 = control messages, bit 6 = error messages.
/// @note KERNAL SETMSG preserves Z; no ldz #0 needed.
void mega65_k_setmsg(unsigned char mode) {
  __attribute__((leaf)) asm volatile("jsr __SETMSG" : /* no outputs */
                                     : "a"(mode) : "x", "y", "p");
}
