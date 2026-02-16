// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/**
 * @file mega65_h_loadfile.c
 * @brief Hyppo hypervisor whole-file loading services.
 *
 * Bypasses KERNAL entirely. Operates on the SD card FAT filesystem,
 * not D81 disk images. No MAP state interaction: safe for use with banking.
 *
 * Trap $D640: loadfile ($36), loadfile_attic ($3E).
 */

#include <stdint.h>

/// Load a file from the SD card into chip memory at a 28-bit address.
/// Call mega65_h_setname() with the filename first.
/// Loads the entire file; no partial load or seek capability.
///
/// @param addr  28-bit destination in chip memory ($000000-$FFFFFF)
/// @return 0 on success, Hyppo error code on failure
/// @note Wrapper sets Z=addr_hi via taz before the trap; ldz #0 required.
uint8_t mega65_h_loadfile(uint32_t addr) {
  uint8_t addr_lo = (uint8_t)(addr);
  uint8_t addr_mid = (uint8_t)(addr >> 8);
  uint8_t addr_hi = (uint8_t)(addr >> 16);
  uint8_t err, carry;

  // No pha/pla needed: A is output-only (no "a" input constraint),
  // unlike KERNAL wrappers where A carries an input through the Z load.
  __attribute__((leaf)) asm volatile(
      "lda %[hi]\n" // load addr bits 16-23
      "taz\n"       // A -> Z
      "lda #$36\n"  // loadfile trap
      "sta $d640\n"
      "clv\n"
      "ldz #0\n" // must clear Z before returning to C
      : "=a"(err),
      "=x"(addr_lo), "=y"(addr_mid), "=c"(carry) : "x"(addr_lo),
      "y"(addr_mid), [hi] "r"(addr_hi) : "p");
  return carry ? 0 : err;
}

/// Load a file from the SD card into attic/hyper RAM.
/// Call mega65_h_setname() with the filename first.
/// Loads the entire file; no partial load or seek capability.
/// Hardware adds $08000000 base, so addr $000000 targets attic byte 0.
///
/// @param addr  24-bit offset in attic RAM
/// @return 0 on success, Hyppo error code on failure
/// @note Wrapper sets Z=addr_hi via taz before the trap; ldz #0 required.
uint8_t mega65_h_loadfile_attic(uint32_t addr) {
  uint8_t addr_lo = (uint8_t)(addr);
  uint8_t addr_mid = (uint8_t)(addr >> 8);
  uint8_t addr_hi = (uint8_t)(addr >> 16);
  uint8_t err, carry;

  __attribute__((leaf)) asm volatile("lda %[hi]\n"
                                     "taz\n"
                                     "lda #$3e\n" // loadfile_attic trap
                                     "sta $d640\n"
                                     "clv\n"
                                     "ldz #0\n" : "=a"(err),
                                     "=x"(addr_lo), "=y"(addr_mid),
                                     "=c"(carry) : "x"(addr_lo),
                                     "y"(addr_mid), [hi] "r"(addr_hi) : "p");
  return carry ? 0 : err;
}
