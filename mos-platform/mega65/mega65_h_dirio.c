// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/**
 * @file mega65_h_dirio.c
 * @brief Hyppo hypervisor directory I/O and navigation services.
 *
 * Trap $D640: opendir ($12), readdir ($14), closedir ($16),
 *             chdir ($0C), cdrootdir ($3C).
 */

#include <stdint.h>

/// @brief Open the current working directory for reading.
/// @param fd  Pointer to receive the directory file descriptor.
uint8_t mega65_h_opendir(uint8_t *fd) {
  uint8_t result, carry;
  __attribute__((leaf)) asm volatile("lda #$12\n" // opendir trap
                                     "sta $d640\n"
                                     "clv\n" : "=a"(result),
                                     "=c"(carry) : : "p");
  if (carry) {
    *fd = result;
    return 0;
  }
  return result;
}

/// @brief Read the next directory entry.
/// @param fd    Directory file descriptor.
/// @param dest  Page-aligned buffer (only high byte used as page number).
uint8_t mega65_h_readdir(uint8_t fd, void *dest) {
  uint8_t page = (uint16_t)dest >> 8;
  uint8_t err, carry;
  __attribute__((leaf)) asm volatile("lda #$14\n" // readdir trap
                                     "sta $d640\n"
                                     "clv\n" : "=a"(err),
                                     "=c"(carry) : "x"(fd), "y"(page) : "p");
  return carry ? 0 : err;
}

/// @brief Close a directory file descriptor.
void mega65_h_closedir(uint8_t fd) {
  __attribute__((leaf)) asm volatile("lda #$16\n" // closedir trap
                                     "sta $d640\n"
                                     "clv\n" : : "x"(fd) : "a",
                                     "p");
}

/// @brief Change to a subdirectory found via findfile/readdir.
uint8_t mega65_h_chdir(void) {
  uint8_t err, carry;
  __attribute__((leaf)) asm volatile("lda #$0c\n" // chdir trap
                                     "sta $d640\n"
                                     "clv\n" : "=a"(err),
                                     "=c"(carry) : : "p");
  return carry ? 0 : err;
}

/// @brief Change to the root directory of a drive.
uint8_t mega65_h_cdrootdir(uint8_t drive) {
  uint8_t err, carry;
  __attribute__((leaf)) asm volatile("lda #$3c\n" // cdrootdir trap
                                     "sta $d640\n"
                                     "clv\n" : "=a"(err),
                                     "=c"(carry) : "x"(drive) : "p");
  return carry ? 0 : err;
}
