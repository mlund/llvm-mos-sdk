// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/**
 * @file mega65_h_fileio.c
 * @brief Hyppo hypervisor file I/O services.
 *
 * readfile/writefile operate on the "current file" set by openfile.
 * Data is transferred via the Hyppo sector buffer at $FFD6E00-$FFD6FFF.
 *
 * Trap $D640: openfile ($18), readfile ($1A), closefile ($20),
 *             closeall ($22), rmfile ($26).
 */

#include <stdint.h>

/// @brief Open a file for reading/writing.
/// @param fd  Pointer to receive the file descriptor.
uint8_t mega65_h_openfile(uint8_t *fd) {
  uint8_t result, carry;
  __attribute__((leaf)) asm volatile("lda #$18\n" // openfile trap
                                     "sta $d640\n"
                                     "clv\n" : "=a"(result),
                                     "=c"(carry) : : "p");
  if (carry) {
    *fd = result;
    return 0;
  }
  return result;
}

/// @brief Read the next sector from the current file into the sector buffer.
/// @param count  Pointer to receive bytes read (0 = EOF).
uint8_t mega65_h_readfile(uint16_t *count) {
  uint8_t err, lo, hi, carry;
  __attribute__((leaf)) asm volatile("lda #$1a\n" // readfile trap
                                     "sta $d640\n"
                                     "clv\n" : "=a"(err),
                                     "=x"(lo), "=y"(hi), "=c"(carry) : : "p");
  if (carry) {
    *count = ((uint16_t)hi << 8) | lo;
    return 0;
  }
  return err;
}

/// @brief Close a file descriptor.
void mega65_h_closefile(uint8_t fd) {
  __attribute__((leaf)) asm volatile("lda #$20\n" // closefile trap
                                     "sta $d640\n"
                                     "clv\n" : : "x"(fd) : "a",
                                     "p");
}

/// @brief Close all open file and directory descriptors.
void mega65_h_closeall(void) {
  __attribute__((leaf)) asm volatile("lda #$22\n" // closeall trap
                                     "sta $d640\n"
                                     "clv\n" : : : "a",
                                     "p");
}

/// @brief Delete a file found via findfile/findfirst/findnext/readdir.
uint8_t mega65_h_rmfile(void) {
  uint8_t err, carry;
  __attribute__((leaf)) asm volatile("lda #$26\n" // rmfile trap
                                     "sta $d640\n"
                                     "clv\n" : "=a"(err),
                                     "=c"(carry) : : "p");
  return carry ? 0 : err;
}
