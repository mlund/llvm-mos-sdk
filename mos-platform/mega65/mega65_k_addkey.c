// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/// Add a PETSCII character to the keyboard input buffer (ADDKEY, $FF4A).
/// @return 0 on success, 1 if the buffer is full.
/// @note KERNAL ADDKEY preserves Z; no ldz #0 needed.
unsigned char mega65_k_addkey(unsigned char petscii_char) {
  unsigned char result;
  __attribute__((leaf)) asm volatile(
      "jsr __ADDKEY" : "=c"(result) : "a"(petscii_char) : "x", "y", "p");
  return result;
}
