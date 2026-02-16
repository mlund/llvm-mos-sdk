// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/// @file
/// MEGA65 override of commodore cbm_k_save.
/// The KERNAL SAVE routine uses ldz #0 internally for [sal],z
/// addressing, so Z happens to be 0 on return -- but this is an
/// implementation detail, not a documented guarantee.
/// This wrapper adds an explicit ldz #0 for safety.

#include <stdint.h>

/// Save memory to a file (SAVE, $FFD8).
/// Requires cbm_k_setlfs() and cbm_k_setnam() called first.
/// On MEGA65, also call mega65_k_setbnk() to set the data/filename banks.
/// @param startaddr        Start of memory region to save.
/// @param endaddr_plusone  First byte past the end (not saved).
/// @return 0 on success, or KERNAL error code (1-9).
/// @note KERNAL SAVE modifies Z; this wrapper restores Z=0.
unsigned char cbm_k_save(void *startaddr, void *endaddr_plusone) {
  unsigned char end_lo = (unsigned char)(uint16_t)endaddr_plusone;
  unsigned char end_hi = (unsigned char)((uint16_t)endaddr_plusone >> 8);
  unsigned char err;
  uint16_t zp_ptr = (uint16_t)startaddr;

  // SAVE: A=ZP addr of start pointer, X/Y=end addr+1. C set on error.
  __attribute__((leaf)) asm volatile("lda #%[zp]\n\t"
                                     "jsr __SAVE\n\t"
                                     "ldz #0\n\t"
                                     "bcs 1f\n\t"
                                     "lda #0\n\t"
                                     "1:" : "=&a"(err),
                                     [zp] "+r"(zp_ptr) : "x"(end_lo),
                                     "y"(end_hi) : "p");

  return err;
}
