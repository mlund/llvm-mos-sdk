// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

#include <stdint.h>

/// Load or verify a file (LOAD, $FFD5). Supports MEGA65 raw mode (flag bit 6).
/// flag: 0x00=load, 0x01=verify, 0x40=raw load, 0x41=raw verify.
/// Raw mode treats the first two bytes as data instead of a PRG header.
/// Requires SETBNK, SETLFS, SETNAM called first.
/// @return 0 on success (writes *end_addr), or KERNAL error code (1-9).
/// @note KERNAL LOAD modifies Z (0 or 3 depending on code path); ldz #0
/// required.
unsigned char mega65_k_load(unsigned char flag, void *load_addr,
                            void **end_addr) {
  unsigned char addr_lo = (unsigned char)(uint16_t)load_addr;
  unsigned char addr_hi = (unsigned char)((uint16_t)load_addr >> 8);
  unsigned char end_lo, end_hi, err;

  // Carry must be tested inside the asm block — using "=c" with the "p"
  // clobber is unreliable as the compiler may insert flag-clobbering
  // instructions between the JSR and the carry read.
  __attribute__((leaf)) asm volatile("jsr __LOAD\n"
                                     "bcs 1f\n"
                                     "lda #0\n" // success: err = 0
                                     "1:\n"
                                     "ldz #0" : "=a"(err),
                                     "=x"(end_lo), "=y"(end_hi) : "a"(flag),
                                     "x"(addr_lo), "y"(addr_hi) : "p");
  if (!err) {
    *end_addr = (void *)((uint16_t)end_hi << 8 | end_lo);
  }
  return err;
}
