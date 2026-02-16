// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/// Close all open files on the specified device (CLOSE_ALL, $FF50).
/// Restores default I/O channels if the current channel was on that device.
/// @note KERNAL CLOSE_ALL preserves Z; no ldz #0 needed.
void mega65_k_close_all(unsigned char device) {
  __attribute__((leaf)) asm volatile("jsr __CLOSE_ALL" : /* no outputs */
                                     : "a"(device) : "x", "y", "p");
}
