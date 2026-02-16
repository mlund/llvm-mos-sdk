// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

#include <stdbool.h>
#include <stdint.h>

/// Save memory to a file with optional raw mode (SAVEFL, $FF3B).
/// In raw mode, the two-byte PRG address header is omitted.
/// The memory region must fit within a single bank.
/// Requires SETBNK, SETLFS, SETNAM called first.
/// @return 0 on success, or KERNAL error code (1-9).
/// @note KERNAL SAVEFL preserves Z, but this wrapper sets Z=flags via taz
///       before the call; ldz #0 required to restore Z=0.
unsigned char mega65_k_savefl(const void *start_addr,
                              const void *end_addr_plus1, bool raw) {
  unsigned char end_lo = (unsigned char)(uint16_t)end_addr_plus1;
  unsigned char end_hi = (unsigned char)((uint16_t)end_addr_plus1 >> 8);
  unsigned char flags = raw ? 0x40 : 0x00;
  unsigned char err;
  // Place start address in a ZP pair for the KERNAL to read via pointer.
  uint16_t zp_ptr = (uint16_t)start_addr;

  // SAVEFL: A=ZP addr of start pointer, X=end+1 lo, Y=end+1 hi, Z=flags
  // Returns: C set on error, A=error code
  __attribute__((leaf)) asm volatile("lda %[flags]\n\t"
                                     "taz\n\t"
                                     "lda #%[zp]\n\t"
                                     "jsr __SAVEFL\n\t"
                                     "ldz #0\n\t"
                                     "bcs 1f\n\t"
                                     "lda #0\n\t"
                                     "1:" : "=&a"(err),
                                     [zp] "+r"(zp_ptr) : [flags] "r"(flags),
                                     "x"(end_lo), "y"(end_hi) : "p");

  return err;
}
