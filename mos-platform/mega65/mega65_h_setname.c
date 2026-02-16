// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/**
 * @file mega65_h_setname.c
 * @brief Hyppo hypervisor filename service.
 *
 * Sets the filename used by subsequent find/load/attach operations.
 * Shared prerequisite for loadfile, findfile, dirio, and attach workflows.
 *
 * Trap $D640: setname ($2E).
 */

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
      "ldy #$01\n" // Y = page of filename ($0100)
      "lda #$2e\n" // setname trap
      "sta $d640\n"
      "clv\n" : "=a"(err),
      "=c"(carry) : : "y", "p");
  // Hyppo carry convention: C=1 success, C=0 error (opposite of KERNAL)
  return carry ? 0 : err;
}
