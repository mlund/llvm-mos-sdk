// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// Hyppo hypervisor file loading — bypasses KERNAL entirely.
// Operates on the SD card FAT filesystem, not D81 disk images.
// No MAP state interaction: safe for use with banking.
//
// Trap convention: LDA #trap : STA $D640 : CLV
// Success: C=1. Error: C=0, A=error code.
// Error codes: $10=invalid address, $81=name too long,
//              $84=too many open files, $88=file not found.

#include <stdint.h>

/// Set the Hyppo filename for subsequent find/load operations.
/// Filename is ASCII (not PETSCII), null-terminated, max 63 characters.
///
/// @param filename  ASCII filename string
/// @return 0 on success, Hyppo error code on failure
uint8_t mega65_h_setname(const char *filename) {
  // Copy to page-aligned buffer at $0100 (hardware stack page).
  // Safe because Hyppo copies the name into its own area before returning,
  // and the MOS software stack doesn't overlap the hardware stack page.
  volatile char *buf = (volatile char *)0x0100;
  uint8_t i = 0;
  while (filename[i] && i < 63) {
    buf[i] = filename[i];
    ++i;
  }
  buf[i] = 0;

  uint8_t err, carry;
  __attribute__((leaf)) asm volatile(
      "ldy #$01\n"        // Y = page of filename ($0100)
      "lda #$2e\n"        // setname trap
      "sta $d640\n"
      "clv\n"
      : "=a"(err), "=c"(carry)
      :
      : "x", "y", "p");
  // Hyppo carry convention: C=1 success, C=0 error (opposite of KERNAL)
  return carry ? 0 : err;
}

/// Load a file from the SD card into chip memory at a 28-bit address.
/// Call mega65_h_setname() with the filename first.
/// Loads the entire file; no partial load or seek capability.
///
/// @param addr  28-bit destination in chip memory ($000000-$FFFFFF)
/// @return 0 on success, Hyppo error code on failure
/// @note Wrapper sets Z=addr_hi via taz before the trap; ldz #0 required.
uint8_t mega65_h_loadfile(uint32_t addr) {
  uint8_t addr_lo  = (uint8_t)(addr);
  uint8_t addr_mid = (uint8_t)(addr >> 8);
  uint8_t addr_hi  = (uint8_t)(addr >> 16);
  uint8_t err, carry;

  // No pha/pla needed: A is output-only (no "a" input constraint),
  // unlike KERNAL wrappers where A carries an input through the Z load.
  __attribute__((leaf)) asm volatile(
      "lda %[hi]\n"       // load addr bits 16-23
      "taz\n"             // A -> Z
      "lda #$36\n"        // loadfile trap
      "sta $d640\n"
      "clv\n"
      "ldz #0\n"          // must clear Z before returning to C
      : "=a"(err), "=x"(addr_lo), "=y"(addr_mid), "=c"(carry)
      : "x"(addr_lo), "y"(addr_mid), [hi] "r"(addr_hi)
      : "p");
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
  uint8_t addr_lo  = (uint8_t)(addr);
  uint8_t addr_mid = (uint8_t)(addr >> 8);
  uint8_t addr_hi  = (uint8_t)(addr >> 16);
  uint8_t err, carry;

  __attribute__((leaf)) asm volatile(
      "lda %[hi]\n"
      "taz\n"
      "lda #$3e\n"        // loadfile_attic trap
      "sta $d640\n"
      "clv\n"
      "ldz #0\n"
      : "=a"(err), "=x"(addr_lo), "=y"(addr_mid), "=c"(carry)
      : "x"(addr_lo), "y"(addr_mid), [hi] "r"(addr_hi)
      : "p");
  return carry ? 0 : err;
}
