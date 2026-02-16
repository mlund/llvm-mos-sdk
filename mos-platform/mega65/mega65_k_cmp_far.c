// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/// Compare a byte with any MEGA65 bank via KERNAL CMP_FAR ($FF7A).
/// Returns 0 if equal, 1 if not equal.
/// Uses $FB:$FC as a temporary ZP pointer pair (outside compiler RC range).
unsigned char mega65_k_cmp_far(unsigned char bank, unsigned int addr,
                               unsigned char y_offset, unsigned char value) {
  unsigned char addr_lo = (unsigned char)addr;
  unsigned char addr_hi = (unsigned char)(addr >> 8);
  unsigned char result;
  __attribute__((leaf)) asm volatile(
      "stx $fb\n"
      "sty $fc\n"
      "taz\n"
      "lda %[val]\n"
      "ldy %[idx]\n"
      "ldx #$fb\n"
      "jsr __CMP_FAR\n"
      "beq 1f\n"
      "ldz #0\n"
      "lda #1\n"
      "bne 2f\n"
      "1: ldz #0\n"
      "lda #0\n"
      "2:\n"
      : "=a"(result)
      : "a"(bank), "x"(addr_lo), "y"(addr_hi), [idx] "r"(y_offset),
        [val] "r"(value)
      : "p");
  return result;
}
