// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/// Get cursor position relative to the active window (PLOT, $FFF0).
/// @note KERNAL PLOT preserves Z; no ldz #0 needed.
void mega65_k_plot_get(unsigned char *line, unsigned char *col) {
  unsigned char l, c;
  __attribute__((leaf)) asm volatile("sec\n"
                                     "jsr __PLOT" : "=x"(l),
                                     "=y"(c) : /* no inputs */
                                     : "a", "p");
  *line = l;
  *col = c;
}

/// Set cursor position relative to the active window (PLOT, $FFF0).
/// @note KERNAL PLOT preserves Z; no ldz #0 needed.
void mega65_k_plot_set(unsigned char line, unsigned char col) {
  __attribute__((leaf)) asm volatile("clc\n"
                                     "jsr __PLOT" : /* no outputs */
                                     : "x"(line),
                                     "y"(col) : "a", "p");
}
