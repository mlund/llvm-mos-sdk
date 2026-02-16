// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/// Read a byte from any MEGA65 bank via KERNAL LDA_FAR ($FF74).
/// Uses $FB:$FC as a temporary ZP pointer pair (outside compiler RC range).
unsigned char mega65_k_lda_far(unsigned char bank, unsigned int addr,
                               unsigned char y_offset) {
  unsigned char addr_lo = (unsigned char)addr;
  unsigned char addr_hi = (unsigned char)(addr >> 8);
  unsigned char result;
  __attribute__((leaf)) asm volatile(
      "stx $fb\n"
      "sty $fc\n"
      "taz\n"
      "ldy %[idx]\n"
      "ldx #$fb\n"
      "jsr __LDA_FAR\n"
      "ldz #0\n"
      : "=a"(result)
      : "a"(bank), "x"(addr_lo), "y"(addr_hi), [idx] "r"(y_offset)
      : "p");
  return result;
}
