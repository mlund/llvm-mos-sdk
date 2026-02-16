// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/**
 * @file mega65_h_findfile.c
 * @brief Hyppo hypervisor file search services.
 *
 * Trap $D640: findfile ($34), findfirst ($30), findnext ($32).
 * All require mega65_h_setname() to be called first.
 */

#include <stdint.h>

/// @brief Find a file by name in the current directory.
uint8_t mega65_h_findfile(void) {
  uint8_t err, carry;
  __attribute__((leaf)) asm volatile("lda #$34\n" // findfile trap
                                     "sta $d640\n"
                                     "clv\n" : "=a"(err),
                                     "=c"(carry) : : "p");
  return carry ? 0 : err;
}

/// @brief Begin searching for files matching the name set by setname.
/// @param fd  Pointer to receive the directory file descriptor.
uint8_t mega65_h_findfirst(uint8_t *fd) {
  uint8_t result, carry;
  __attribute__((leaf)) asm volatile("lda #$30\n" // findfirst trap
                                     "sta $d640\n"
                                     "clv\n" : "=a"(result),
                                     "=c"(carry) : : "p");
  if (carry) {
    *fd = result;
    return 0;
  }
  return result;
}

/// @brief Find the next matching file after findfirst.
uint8_t mega65_h_findnext(void) {
  uint8_t err, carry;
  __attribute__((leaf)) asm volatile("lda #$32\n" // findnext trap
                                     "sta $d640\n"
                                     "clv\n" : "=a"(err),
                                     "=c"(carry) : : "p");
  return carry ? 0 : err;
}
