// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// Banked plasma demo for mega65-banked platform.
//
// Demonstrates how to split a program across multiple banks:
//   - Bank 1: sine lookup + plasma sum computation
//   - Bank 2: character set generator
//   - Bank 3: plasma screen renderer
//   - Fixed region ($8000-$CFFF): main loop, shared state, screen I/O
//
// banked_call() safely switches banks via the MAP register and calls a
// function pointer. Shared state in the fixed region is accessible from
// any bank since $8000-$CFFF is never remapped.
//
// The CRT bank loader (.init.150) loads BANK1-BANK3 from the D81 disk
// into physical RAM at startup. Build produces both a PRG and a D81.

#include <array>
#include <cstdint>
#include <mapper.h>
#include <mega65.h>

// Only banks 1-3 are used; the rest are left out of the image.
MAPPER_BANK_COUNT(3);

// ---------------------------------------------------------------------------
// Shared state — lives in fixed region, accessible from all banks.
// ---------------------------------------------------------------------------

static uint8_t c1a, c1b, c2a, c2b;
static std::array<uint8_t, 80> xbuf;
static std::array<uint8_t, 25> ybuf;

// ---------------------------------------------------------------------------
// Bank 1: Sine table and plasma sum computation.
//
// The 256-entry sine table and the function that reads it live in the same
// bank so the table is accessible when the function executes. Placing large
// const data in a bank keeps the 20KB fixed region free for code.
// ---------------------------------------------------------------------------

RODATA_BANK(1)
static const uint8_t sine_table[256] = {
    0x80, 0x7d, 0x7a, 0x77, 0x74, 0x70, 0x6d, 0x6a, 0x67, 0x64, 0x61, 0x5e,
    0x5b, 0x58, 0x55, 0x52, 0x4f, 0x4d, 0x4a, 0x47, 0x44, 0x41, 0x3f, 0x3c,
    0x39, 0x37, 0x34, 0x32, 0x2f, 0x2d, 0x2b, 0x28, 0x26, 0x24, 0x22, 0x20,
    0x1e, 0x1c, 0x1a, 0x18, 0x16, 0x15, 0x13, 0x11, 0x10, 0x0f, 0x0d, 0x0c,
    0x0b, 0x0a, 0x08, 0x07, 0x06, 0x06, 0x05, 0x04, 0x03, 0x03, 0x02, 0x02,
    0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03,
    0x03, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x0a, 0x0b, 0x0c, 0x0d, 0x0f,
    0x10, 0x11, 0x13, 0x15, 0x16, 0x18, 0x1a, 0x1c, 0x1e, 0x20, 0x22, 0x24,
    0x26, 0x28, 0x2b, 0x2d, 0x2f, 0x32, 0x34, 0x37, 0x39, 0x3c, 0x3f, 0x41,
    0x44, 0x47, 0x4a, 0x4d, 0x4f, 0x52, 0x55, 0x58, 0x5b, 0x5e, 0x61, 0x64,
    0x67, 0x6a, 0x6d, 0x70, 0x74, 0x77, 0x7a, 0x7d, 0x80, 0x83, 0x86, 0x89,
    0x8c, 0x90, 0x93, 0x96, 0x99, 0x9c, 0x9f, 0xa2, 0xa5, 0xa8, 0xab, 0xae,
    0xb1, 0xb3, 0xb6, 0xb9, 0xbc, 0xbf, 0xc1, 0xc4, 0xc7, 0xc9, 0xcc, 0xce,
    0xd1, 0xd3, 0xd5, 0xd8, 0xda, 0xdc, 0xde, 0xe0, 0xe2, 0xe4, 0xe6, 0xe8,
    0xea, 0xeb, 0xed, 0xef, 0xf0, 0xf1, 0xf3, 0xf4, 0xf5, 0xf6, 0xf8, 0xf9,
    0xfa, 0xfa, 0xfb, 0xfc, 0xfd, 0xfd, 0xfe, 0xfe, 0xfe, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xfe, 0xfe, 0xfe, 0xfd, 0xfd, 0xfc, 0xfb, 0xfa,
    0xfa, 0xf9, 0xf8, 0xf6, 0xf5, 0xf4, 0xf3, 0xf1, 0xf0, 0xef, 0xed, 0xeb,
    0xea, 0xe8, 0xe6, 0xe4, 0xe2, 0xe0, 0xde, 0xdc, 0xda, 0xd8, 0xd5, 0xd3,
    0xd1, 0xce, 0xcc, 0xc9, 0xc7, 0xc4, 0xc1, 0xbf, 0xbc, 0xb9, 0xb6, 0xb3,
    0xb1, 0xae, 0xab, 0xa8, 0xa5, 0xa2, 0x9f, 0x9c, 0x99, 0x96, 0x93, 0x90,
    0x8c, 0x89, 0x86, 0x83};

// Compute per-axis sine sums into the shared xbuf/ybuf arrays.
// Must run while bank 1 is mapped so sine_table is accessible.
CODE_BANK(1)
void compute_plasma_sums() {
  uint8_t i = c1a, j = c1b;
  for (auto &y : ybuf) {
    y = sine_table[i] + sine_table[j];
    i += 4;
    j += 9;
  }
  i = c2a;
  j = c2b;
  for (auto &x : xbuf) {
    x = sine_table[i] + sine_table[j];
    i += 3;
    j += 7;
  }
}

// ---------------------------------------------------------------------------
// Bank 2: Character set generator.
//
// Generates 256 dithered 8x8 characters at $C000. The charset must be
// outside the banked window ($2000-$7FFF) so VIC-IV can read it from
// bank 0 RAM regardless of which bank is currently mapped.
//
// IMPORTANT: The VIC-IV's internal PETSCII character ROM occupies the
// CHARPTR range $001000-$001FFF. Any CHARPTR in that range reads ROM,
// not chip RAM. We use $C000 (in the fixed region) to avoid this.
//
// Each character's pixel density scales linearly with its index, so
// summing two sine waves as a character index produces smooth gradients.
// ---------------------------------------------------------------------------

static uint32_t rng_state = 7;
static uint8_t rng_rand8() {
  rng_state ^= rng_state << 13;
  rng_state ^= rng_state >> 17;
  rng_state ^= rng_state << 5;
  return static_cast<uint8_t>(rng_state);
}

CODE_BANK(2)
void generate_charset() {
  auto charset = reinterpret_cast<volatile uint8_t *>(0xC000);
  for (uint16_t ch = 0; ch < 256; ch++) {
    auto density = static_cast<uint8_t>(ch);
    for (uint8_t row = 0; row < 8; row++) {
      uint8_t pattern = 0;
      for (uint8_t bit = 0; bit < 8; bit++) {
        if (rng_rand8() < density)
          pattern |= (1 << bit);
      }
      *(charset++) = pattern;
    }
  }
}

// ---------------------------------------------------------------------------
// Bank 3: Plasma screen renderer.
//
// Writes pre-computed xbuf + ybuf sums to screen memory as character indices.
// The shared buffers live in the fixed region, so they're readable from any
// bank. main() calls bank 1 first to fill the buffers, then bank 3 to render.
// ---------------------------------------------------------------------------

CODE_BANK(3)
void render_plasma() {
  auto screen = reinterpret_cast<volatile uint8_t *>(&DEFAULT_SCREEN);
  for (const auto y : ybuf) {
    for (const auto x : xbuf) {
      *(screen++) = y + x;
    }
  }
}

// ---------------------------------------------------------------------------
// Fixed region: main loop.
//
// Orchestrates bank switches each frame:
//   1. banked_call(2, ...) once at startup to generate the charset
//   2. banked_call(1, ...) each frame to compute sine sums
//   3. banked_call(3, ...) each frame to render plasma to screen
// ---------------------------------------------------------------------------

int main() {
  // Unlock VIC-IV I/O personality. KERNAL disk I/O (bank loading) resets
  // the KEY register to VIC-II mode; re-unlock so charptr is accessible.
  VICIV.key = 0x47;
  VICIV.key = 0x53;

  // 3.5 MHz for smooth animation.
  VICIV.ctrlb |= VIC3_FAST_MASK;
  VICIV.ctrlc &= ~VIC4_VFAST_MASK;

  // One-time charset generation in bank 2.
  banked_call(2, generate_charset);

  // Point VIC-IV at our custom charset at $C000.
  VICIV.charptr = 0xC000;

  while (true) {
    banked_call(1, compute_plasma_sums);
    banked_call(3, render_plasma);

    c1a += 3;
    c1b -= 5;
    c2a += 2;
    c2b -= 3;
  }
}
