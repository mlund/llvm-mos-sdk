// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/**
 * @file mega65_h_drive.c
 * @brief Hyppo hypervisor drive/partition services.
 *
 * "Drives" in Hyppo are SD card partitions, not F011 floppy drives.
 *
 * Trap $D640: getcurrentdrive ($04), getdefaultdrive ($02), selectdrive ($06).
 */

#include <stdint.h>

/// @brief Get the currently selected SD card drive number.
uint8_t mega65_h_getcurrentdrive(void) {
  uint8_t drive;
  __attribute__((leaf)) asm volatile("lda #$04\n" // getcurrentdrive trap
                                     "sta $d640\n"
                                     "clv\n" : "=a"(drive) : : "p");
  return drive;
}

/// @brief Get the default SD card drive number (set at boot).
uint8_t mega65_h_getdefaultdrive(void) {
  uint8_t drive;
  __attribute__((leaf)) asm volatile("lda #$02\n" // getdefaultdrive trap
                                     "sta $d640\n"
                                     "clv\n" : "=a"(drive) : : "p");
  return drive;
}

/// @brief Select the active SD card drive/partition.
uint8_t mega65_h_selectdrive(uint8_t drive) {
  uint8_t err, carry;
  __attribute__((leaf)) asm volatile("lda #$06\n" // selectdrive trap
                                     "sta $d640\n"
                                     "clv\n" : "=a"(err),
                                     "=c"(carry) : "x"(drive) : "p");
  return carry ? 0 : err;
}
