// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/**
 * @file mega65_h_system.c
 * @brief Hyppo hypervisor system services.
 *
 * Trap $D640: getversion ($00), geterrorcode ($38).
 * These correspond to __hypervisor.rega / HYPERVISOR.htrap[0].
 */

#include <stdint.h>

/// @brief Get Hyppo and HDOS version numbers.
/// Output struct: [0]=hyppo_major, [1]=hyppo_minor,
///                [2]=hdos_major,  [3]=hdos_minor.
void mega65_h_getversion(void *ver) {
  uint8_t a_val, x_val, y_val, z_val;
  __attribute__((leaf)) asm volatile(
      "lda #$00\n" // getversion trap
      "sta $d640\n"
      "clv\n"
      // A=hyppo_major, X=hyppo_minor, Y=hdos_major, Z=hdos_minor
      "sta %[a]\n"
      "stx %[x]\n"
      "sty %[y]\n"
      "tza\n" // Z -> A
      "sta %[z]\n"
      "ldz #0\n" : [a] "=m"(a_val),
      [x] "=m"(x_val), [y] "=m"(y_val), [z] "=m"(z_val) : : "a", "x", "y", "p");
  uint8_t *v = (uint8_t *)ver;
  v[0] = a_val;
  v[1] = x_val;
  v[2] = y_val;
  v[3] = z_val;
}

/// @brief Get the error code from the last failed Hyppo service call.
uint8_t mega65_h_geterrorcode(void) {
  uint8_t err;
  __attribute__((leaf)) asm volatile("lda #$38\n" // geterrorcode trap
                                     "sta $d640\n"
                                     "clv\n" : "=a"(err) : : "p");
  return err;
}
