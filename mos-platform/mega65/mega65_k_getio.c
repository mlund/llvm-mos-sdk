// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/// Read the current input and output devices (GETIO, $FF41).
/// Input device 0 = keyboard. Output device 3 = screen.
/// @note KERNAL GETIO preserves Z; no ldz #0 needed.
void mega65_k_getio(unsigned char *input_dev, unsigned char *output_dev) {
  unsigned char in, out;
  __attribute__((leaf)) asm volatile("jsr __GETIO" : "=x"(in),
                                     "=y"(out) : /* no inputs */
                                     : "a", "p");
  *input_dev = in;
  *output_dev = out;
}
