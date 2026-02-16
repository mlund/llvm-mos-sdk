// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/**
 * @file mega65_h_attach.c
 * @brief Hyppo hypervisor disk image attach/detach service.
 *
 * Virtualises the F011 floppy controller with D81 images from SD card.
 * Drive 0 = unit 8, drive 1 = unit 9 (by default).
 *
 * Trap $D640: attach ($4A).
 * For attach: call mega65_h_setname() with the image filename first.
 * Cannot open files inside disk images -- use KERNAL/F011 for that.
 */

#include <stdint.h>

/// @brief Attach or detach a D81 disk image to an F011 floppy drive.
uint8_t mega65_h_attach(uint8_t flags) {
  uint8_t err, carry;
  __attribute__((leaf)) asm volatile("lda #$4a\n" // attach trap
                                     "sta $d640\n"
                                     "clv\n" : "=a"(err),
                                     "=c"(carry) : "x"(flags) : "p");
  return carry ? 0 : err;
}
