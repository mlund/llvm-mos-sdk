// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// Full Color Mode (FCM) fire effect for MEGA65
//
// Classic demoscene fire at 80x100 resolution, upscaled to 320x200 via
// FCM tiles (4:1 horizontal, 2:1 vertical). Each 8x8 tile maps to a
// 2x4 block of fire pixels, keeping fill4 (STQ) efficient.
//
// MEGA65 features used:
//   - VIC-IV Full Color Mode: per-pixel 8-bit palette index
//   - Enhanced DMA: copies tile rows to $40000 (fast RAM)
//   - Custom 256-entry fire palette (black -> red -> yellow -> white)
//   - Full speed mode (~40 MHz)

#include <cstdint>
#include <dma.hpp>
#include <mega65.h>

using namespace mega65::dma;

// --- Constants ---

static constexpr uint8_t CELL_COLS = 40;
static constexpr uint8_t CELL_ROWS = 25;
static constexpr uint16_t TILE_BYTES = 64;
static constexpr uint16_t TILE_ROW_BYTES = CELL_COLS * TILE_BYTES; // 2560
static constexpr uint32_t GFX_ADDR = 0x40000UL; // Fast RAM, above chip RAM

// Fire grid: 80 wide x 102 tall (100 visible + 2 seed rows)
// Each fire pixel maps to a 4x2 block on screen (320/80=4, 200/100=2).
// Each 8x8 tile = 2 fire pixels wide x 4 fire pixels tall.
static constexpr uint8_t FIRE_W = 80;
static constexpr uint8_t FIRE_H = 102;

// --- Fire palette (constexpr) ---

struct FirePalette {
  uint8_t r[256]{}, g[256]{}, b[256]{};

  static constexpr uint8_t nyb(uint8_t n) { return (n << 4) | n; }

  constexpr FirePalette() {
    // 5-segment gradient: black -> dark red -> red -> orange -> yellow -> white
    for (uint16_t i = 0; i < 256; ++i) {
      uint8_t rv = 0, gv = 0, bv = 0;
      if (i < 48) {
        rv = static_cast<uint8_t>(i * 8 / 48);
      } else if (i < 96) {
        rv = static_cast<uint8_t>(8 + (i - 48) * 7 / 48);
      } else if (i < 144) {
        rv = 15;
        gv = static_cast<uint8_t>((i - 96) * 10 / 48);
      } else if (i < 192) {
        rv = 15;
        gv = static_cast<uint8_t>(10 + (i - 144) * 5 / 48);
      } else {
        rv = 15;
        gv = 15;
        bv = static_cast<uint8_t>((i - 192) * 15 / 63);
      }
      r[i] = nyb(rv);
      g[i] = nyb(gv);
      b[i] = nyb(bv);
    }
  }
};

// Computed at compile time, placed in .rodata
static constexpr FirePalette fire_palette{};

// --- Buffers ---

static uint8_t fire[FIRE_H][FIRE_W];

// 16-bit LFSR for fast pseudo-random numbers
static uint8_t rand8() {
  static uint16_t lfsr = 0xACE1;
  uint16_t bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1;
  lfsr = (lfsr >> 1) | (bit << 15);
  return static_cast<uint8_t>(lfsr);
}

// --- VIC-IV setup (same pattern as mandelbrot_fcm) ---

static void setup_vic() {
  constexpr uint8_t VICIV_KEY1 = 0x47;
  constexpr uint8_t VICIV_KEY2 = 0x53;
  constexpr uint8_t CPU_DDR_40MHZ = 65;
  constexpr uint8_t CHR16_BYTES_PER_CHAR = 2;

  asm volatile("sei"); // Disable IRQs during VIC-IV configuration

  // Unlock VIC-IV registers (active after writing both keys in sequence)
  VICIV.key = VICIV_KEY1;
  VICIV.key = VICIV_KEY2;

  // Prevent VIC-IV from applying changes until all registers are set
  VICIV.sdbdrwd_msb &= ~VIC4_HOTREG_MASK;

  // Switch to ~40 MHz for enough CPU time to run the fire simulation
  CPU_PORTDDR = CPU_DDR_40MHZ;

  // Enable VIC-III features; 320x200 mode (not 640x400)
  VICIV.ctrlb = (VICIV.ctrlb | VIC3_FAST_MASK | VIC3_ATTR_MASK) &
                ~(VIC3_H640_MASK | VIC3_V400_MASK);

  // Enable FCM: 16-bit character indices, full-colour high nibble
  VICIV.ctrlc =
      (VICIV.ctrlc & ~VIC4_FCLRLO_MASK) | VIC4_CHR16_MASK | VIC4_FCLRHI_MASK;

  constexpr uint16_t SCREEN_ADDR = 0x0800;
  constexpr uint16_t CHARPTR_BASE = 0x0000;

  VICIV.scrnptr = SCREEN_ADDR;
  VICIV.charptr = CHARPTR_BASE;

  VICIV.linestep = CELL_COLS * CHR16_BYTES_PER_CHAR;
  VICIV.chrcount = CELL_COLS;
  VICIV.disp_rows = CELL_ROWS;

  constexpr uint8_t VIC4_DEN_MASK = 0x10;
  constexpr uint8_t VIC4_RSEL_MASK = 0x08;
  constexpr uint8_t VIC4_RST8_MASK = 0x80;
  constexpr uint8_t VIC4_ECM_MASK = 0x40;
  constexpr uint8_t VIC4_CSEL_MASK = 0x08;

  constexpr uint8_t YSCROLL = 3;
  constexpr uint8_t CTRL2_UPPER_MASK = 0xE0; // MCM, res, unused bits

  VICIV.ctrl1 = (VICIV.ctrl1 & (VIC4_RST8_MASK | VIC4_ECM_MASK)) |
                VIC4_DEN_MASK | VIC4_RSEL_MASK | YSCROLL;
  VICIV.ctrl2 = (VICIV.ctrl2 & CTRL2_UPPER_MASK) | VIC4_CSEL_MASK;

  // Black background; FCM pixel value 0 is transparent and shows screencol
  VICIV.bordercol = 0;
  VICIV.screencol = 0;

  // Use PAL palette bank (256 entries)
  VICIV.ctrla |= VIC3_PAL_MASK;
}

// clang-format off
static void setup_palette() {
  for (uint16_t i = 0; i < 256; ++i) {
    PALETTE.red[i]   = fire_palette.r[i];
    PALETTE.green[i] = fire_palette.g[i];
    PALETTE.blue[i]  = fire_palette.b[i];
  }
}
// clang-format on

static void setup_screen() {
  constexpr uint16_t NUM_CELLS = CELL_COLS * CELL_ROWS;
  constexpr uint16_t TILE_BASE = GFX_ADDR / TILE_BYTES;
  constexpr uint16_t COLOR_RAM_ADDR = 0xD800;
  auto *const SCREEN16 = reinterpret_cast<volatile uint16_t *>(&DEFAULT_SCREEN);
  auto *const COLOR_RAM = reinterpret_cast<volatile uint8_t *>(COLOR_RAM_ADDR);

  setup_palette();

  // Each screen cell gets a unique tile index (1:1 mapping)
  for (uint16_t i = 0; i < NUM_CELLS; ++i)
    SCREEN16[i] = TILE_BASE + i;

  // Zero colour RAM (2 bytes per cell in CHR16 mode)
  for (uint16_t i = 0; i < NUM_CELLS * 2; ++i)
    COLOR_RAM[i] = 0;

  // Clear tile data in fast RAM via DMA (too large for CPU fill)
  const auto dma = make_dma_fill(GFX_ADDR, 0, NUM_CELLS * TILE_BYTES);
  trigger_dma(dma);

  // Zero the fire buffer
  for (uint16_t y = 0; y < FIRE_H; ++y)
    for (uint8_t x = 0; x < FIRE_W; ++x)
      fire[y][x] = 0;
}

// --- Fire simulation ---

static void seed_bottom_rows() {
  for (uint8_t y = FIRE_H - 2; y < FIRE_H; ++y) {
    uint8_t *row = fire[y];
    constexpr uint8_t SEED_MIN = 0xC0; // seed range: 192-255
    constexpr uint8_t SEED_MASK = 0x3F;
    for (uint8_t x = 0; x < FIRE_W; ++x)
      row[x] = (rand8() & SEED_MASK) | SEED_MIN;
  }
}

static void simulate_fire() {
  for (uint8_t y = 0; y < FIRE_H - 2; ++y) {
    uint8_t *dst = fire[y];
    const uint8_t *r1 = fire[y + 1];
    const uint8_t *r2 = fire[y + 2];

    // Decay of 2 per row: with 100 visible rows and seed values 192-255,
    // hottest pixels cool to zero around row 30 from the top.
    constexpr uint8_t DECAY = 2;

    // Left edge (clamp x-1 to 0)
    uint16_t sum = r1[0] + r1[0] + r1[1] + r2[0];
    uint8_t avg = static_cast<uint8_t>(sum >> 2);
    dst[0] = (avg > DECAY) ? static_cast<uint8_t>(avg - DECAY) : 0;

    // Inner pixels — no bounds checks
    for (uint8_t x = 1; x < FIRE_W - 1; ++x) {
      sum = r1[x - 1] + r1[x] + r1[x + 1] + r2[x];
      avg = static_cast<uint8_t>(sum >> 2);
      dst[x] = (avg > DECAY) ? static_cast<uint8_t>(avg - DECAY) : 0;
    }

    // Right edge (clamp x+1 to FIRE_W-1)
    sum = r1[FIRE_W - 2] + r1[FIRE_W - 1] + r1[FIRE_W - 1] + r2[FIRE_W - 1];
    avg = static_cast<uint8_t>(sum >> 2);
    dst[FIRE_W - 1] = (avg > DECAY) ? static_cast<uint8_t>(avg - DECAY) : 0;
  }
}

// Fill 4 consecutive bytes with the same value using 45GS02 STQ instruction.
// STQ stores the 32-bit Q register {A, X, Y, Z} via a ZP pointer.
// 16 fill4 calls per tile vs 64 individual STA stores saves ~73 cycles/tile
// (~73K cycles/frame across 1000 tiles at 40 MHz).
// ldz #0 at the end restores the compiler's assumption that Z is always 0.
static inline void fill4(uint8_t *dst, uint8_t val) {
  asm volatile("tax\n\t"
               "tay\n\t"
               "taz\n\t"
               "stq (%[ptr])\n\t"
               "ldz #0"
               :
               : "a"(val), [ptr] "r"(dst)
               : "x", "y", "p", "memory");
}

// --- Convert fire to FCM tiles and DMA to graphics memory ---
// Each 8x8 tile = 2 wide x 4 tall fire pixels, each expanded to 4x2:
//   AAAABBBB    (A = fire[fy][fx],   B = fire[fy][fx+1])
//   AAAABBBB
//   CCCCDDDD    (C = fire[fy+1][fx], D = fire[fy+1][fx+1])
//   CCCCDDDD
//   EEEEFFFF    (E = fire[fy+2][fx], F = fire[fy+2][fx+1])
//   EEEEFFFF
//   GGGGHHHH    (G = fire[fy+3][fx], H = fire[fy+3][fx+1])
//   GGGGHHHH

static void convert_and_dma() {
  static uint8_t tile_row_buf[TILE_ROW_BYTES];

  // Init DMA descriptor once; only dest_addr changes per row
  auto dma = make_dma_copy(
      static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tile_row_buf)),
      GFX_ADDR, TILE_ROW_BYTES);

  for (uint8_t ty = 0; ty < CELL_ROWS; ++ty) {
    const uint8_t fy = ty * 4;
    const uint8_t *row0 = fire[fy];
    const uint8_t *row1 = fire[fy + 1];
    const uint8_t *row2 = fire[fy + 2];
    const uint8_t *row3 = fire[fy + 3];
    uint8_t *dst = tile_row_buf;

    for (uint8_t tx = 0; tx < CELL_COLS; ++tx) {
      const uint8_t fx = tx * 2;
      const uint8_t a = row0[fx], b = row0[fx + 1];
      const uint8_t c = row1[fx], d = row1[fx + 1];
      const uint8_t e = row2[fx], f = row2[fx + 1];
      const uint8_t g = row3[fx], h = row3[fx + 1];

      // clang-format off
      fill4(dst +  0, a); fill4(dst +  4, b);
      fill4(dst +  8, a); fill4(dst + 12, b);
      fill4(dst + 16, c); fill4(dst + 20, d);
      fill4(dst + 24, c); fill4(dst + 28, d);
      fill4(dst + 32, e); fill4(dst + 36, f);
      fill4(dst + 40, e); fill4(dst + 44, f);
      fill4(dst + 48, g); fill4(dst + 52, h);
      fill4(dst + 56, g); fill4(dst + 60, h);
      // clang-format on
      dst += TILE_BYTES;
    }

    // DMA tile row to fast RAM (CPU cannot directly address $40000+)
    trigger_dma(dma);
    dma.dmalist.dest_addr += TILE_ROW_BYTES;
  }
}

int main() {
  setup_vic();
  setup_screen();

  while (true) {
    seed_bottom_rows();
    simulate_fire();
    convert_and_dma();
  }
}
