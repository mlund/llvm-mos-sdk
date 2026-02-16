// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/// Store a byte to any MEGA65 bank via KERNAL STA_FAR ($FF77).
/// Uses $FB:$FC as a temporary ZP pointer pair (outside compiler RC range).
void mega65_k_sta_far(unsigned char bank, unsigned int addr,
                      unsigned char y_offset, unsigned char value) {
  unsigned char addr_lo = (unsigned char)addr;
  unsigned char addr_hi = (unsigned char)(addr >> 8);
  __attribute__((leaf)) asm volatile(
      "stx $fb\n"
      "sty $fc\n"
      "taz\n"
      "lda %[val]\n"
      "ldy %[idx]\n"
      "ldx #$fb\n"
      "jsr __STA_FAR\n"
      "ldz #0\n"
      : /* no outputs */
      : "a"(bank), "x"(addr_lo), "y"(addr_hi), [idx] "r"(y_offset),
        [val] "r"(value)
      : "p");
}
