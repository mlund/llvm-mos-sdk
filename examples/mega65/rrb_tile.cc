// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// Raster Rewrite Buffer (RRB) tiling demo with smooth X-Y scrolling
//
// Demonstrates seamless tile pattern repetition using RRB GOTOX entries
// for horizontal positioning and FCM Y-offset for vertical sub-pixel scroll.
// The tile pattern is a "tumbling blocks" isometric cube illusion using
// 3 colours: cream top face, pink left face, maroon right face.
//
// MEGA65 features used:
//   - VIC-IV Full Color Mode (FCM) with CHR16 + FCLRHI
//   - Raster Rewrite Buffer (RRB) GOTOX for pixel-precise X positioning
//   - FCM Y-offset (fcm_yoffs) for fine vertical scrolling
//   - Enhanced DMA for colour RAM and tile data setup
//   - 40 MHz CPU mode

#include <cstdint>
#include <dma.hpp>
#include <mega65.h>

using namespace mega65::dma;

// --- Constants ---

static constexpr uint8_t TILE_W = 4;  // tile width in FCM chars (32px)
static constexpr uint8_t TILE_H = 4;  // tile height in FCM chars (32px)
static constexpr uint8_t VISIBLE_ROWS = 25;                      // 200px
static constexpr uint8_t TOTAL_ROWS = VISIBLE_ROWS + (TILE_H - 1); // 28
static constexpr uint8_t VIS_CHARS = 44;  // 44 * 8 = 352px coverage
static constexpr uint8_t ROW_CHARS = VIS_CHARS + 1; // +1 for GOTOX = 45
static constexpr uint16_t LINESTEP_BYTES = ROW_CHARS * 2; // 90 bytes/row
static constexpr uint16_t SCREEN_ADDR = 0x0800;
static constexpr uint32_t TILE_ADDR = 0x40000UL;       // fast RAM
static constexpr uint16_t TILE_BASE = TILE_ADDR / 64;  // char number 4096
static constexpr uint32_t CRAM_ADDR = 0xFF80000UL;     // colour RAM base
static constexpr uint16_t GOTOX_BASE = 0;

// Palette indices (1-3, avoiding 0 = transparent in FCM)
static constexpr uint8_t COL_CREAM = 1;  // top face
static constexpr uint8_t COL_PINK = 2;   // left face
static constexpr uint8_t COL_MAROON = 3; // right face

// Character index for a given column and row within the tile.
// Memory layout per column: R0, R1, R2, R3, R0_copy (5 chars).
// fcm_yoffs bleed goes R0->R1->R2->R3->R0_copy for seamless Y scroll.
static constexpr uint16_t char_idx(uint8_t col, uint8_t row) {
  return TILE_BASE + col * 5 + row;
}

// --- Tile pattern generation ---
//
// 32x32 tumbling blocks tile with cream diamonds at corners, colored X-cross.
// Shift (16,8) places diamond top-face at tile corners.
// Two junctions per tile: (0,0) stem at sx=0, (16,16) stem at sx=16.
// Face boundaries follow diagonal arm lines (a=48, b=16), not horizontal cuts.

static uint8_t tile_pixel(uint8_t px, uint8_t py) {
  const uint8_t sx = (px + 16) & 31;
  const uint8_t sy = (py + 8) & 31;
  const int a = 2 * static_cast<int>(sy) + static_cast<int>(sx);
  const int b = 2 * static_cast<int>(sy) - static_cast<int>(sx);

  // Cream diamond (top face)
  if (a >= 16 && a <= 48 && b >= -16 && b <= 16)
    return COL_CREAM;

  // Below junction (16,16)'s arms: past both diagonal edges of diamond
  if (a > 48 && b > 16)
    return (sx < 16) ? COL_PINK : COL_MAROON;

  // All other non-cream: junction (0,0)/(32,0) territory (colors swapped)
  return (sx < 16) ? COL_MAROON : COL_PINK;
}

// Build 20 FCM characters (1280 bytes) from the 32x32 tile pattern.
// 4 columns x (4 rows + 1 copy) = 20 chars.
static uint8_t tile_buf[20 * 64];

static void generate_tile() {
  auto *dst = tile_buf;
  for (uint8_t col = 0; col < 4; ++col) {
    uint8_t *col_start = dst;
    for (uint8_t crow = 0; crow < 4; ++crow) {
      for (uint8_t y = 0; y < 8; ++y)
        for (uint8_t x = 0; x < 8; ++x)
          *dst++ = tile_pixel(col * 8 + x, crow * 8 + y);
    }
    // R0 copy for R3->R0 bleed during Y-scroll
    __builtin_memcpy(dst, col_start, 64);
    dst += 64;
  }

  auto dma = make_dma_copy(
      static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tile_buf)),
      TILE_ADDR, sizeof(tile_buf));
  trigger_dma(dma);
}

// --- VIC-IV setup ---

static void setup_vic() {
  constexpr uint8_t VICIV_KEY1 = 0x47;
  constexpr uint8_t VICIV_KEY2 = 0x53;
  constexpr uint8_t CPU_DDR_40MHZ = 65;

  asm volatile("sei");

  VICIV.key = VICIV_KEY1;
  VICIV.key = VICIV_KEY2;

  VICIV.sdbdrwd_msb &= ~VIC4_HOTREG_MASK;

  CPU_PORTDDR = CPU_DDR_40MHZ;

  VICIV.ctrlb = (VICIV.ctrlb | VIC3_FAST_MASK | VIC3_ATTR_MASK) &
                ~(VIC3_H640_MASK | VIC3_V400_MASK);

  VICIV.ctrlc =
      (VICIV.ctrlc & ~VIC4_FCLRLO_MASK) | VIC4_CHR16_MASK | VIC4_FCLRHI_MASK;

  VICIV.scrnptr = SCREEN_ADDR;
  VICIV.charptr = 0;

  VICIV.linestep = LINESTEP_BYTES;
  VICIV.chrcount = ROW_CHARS;
  VICIV.disp_rows = TOTAL_ROWS;

  VICIV.colptr = 0;
  VICIV.chryscl = 0;

  constexpr uint8_t VIC4_DEN_MASK = 0x10;
  constexpr uint8_t VIC4_RSEL_MASK = 0x08;
  constexpr uint8_t VIC4_RST8_MASK = 0x80;
  constexpr uint8_t VIC4_ECM_MASK = 0x40;
  constexpr uint8_t VIC4_CSEL_MASK = 0x08;

  VICIV.ctrl1 = (VICIV.ctrl1 & (VIC4_RST8_MASK | VIC4_ECM_MASK)) |
                VIC4_DEN_MASK | VIC4_RSEL_MASK | 3;
  VICIV.ctrl2 = (VICIV.ctrl2 & 0xE0) | VIC4_CSEL_MASK;

  VICIV.bordercol = 0;
  VICIV.screencol = 0;

  VICIV.ctrla |= VIC3_PAL_MASK;
}

// --- Palette ---

static constexpr uint8_t nyb(uint8_t n) { return (n << 4) | n; }

static void setup_palette() {
  PALETTE.red[0] = 0;
  PALETTE.green[0] = 0;
  PALETTE.blue[0] = 0;

  // Cream — top face
  PALETTE.red[COL_CREAM] = nyb(15);
  PALETTE.green[COL_CREAM] = nyb(14);
  PALETTE.blue[COL_CREAM] = nyb(11);

  // Pink/mauve — left face
  PALETTE.red[COL_PINK] = nyb(13);
  PALETTE.green[COL_PINK] = nyb(7);
  PALETTE.blue[COL_PINK] = nyb(9);

  // Maroon — right face
  PALETTE.red[COL_MAROON] = nyb(7);
  PALETTE.green[COL_MAROON] = nyb(3);
  PALETTE.blue[COL_MAROON] = nyb(4);
}

// --- Screen RAM setup ---
//
// Each row: [GOTOX (2B)] [44 visible chars (88B)] = 90 bytes
// Tile pattern repeats every 4 columns and 4 rows.

static void setup_screen() {
  auto *scr = reinterpret_cast<volatile uint8_t *>(SCREEN_ADDR);

  for (uint8_t row = 0; row < TOTAL_ROWS; ++row) {
    uint16_t off = static_cast<uint16_t>(row) * LINESTEP_BYTES;

    scr[off + 0] = static_cast<uint8_t>(GOTOX_BASE & 0xFF);
    scr[off + 1] = static_cast<uint8_t>((GOTOX_BASE >> 8) & 0x03);

    uint8_t tile_row = row & 3;

    for (uint8_t c = 0; c < VIS_CHARS; ++c) {
      uint8_t tile_col = c & 3;
      uint16_t ch = char_idx(tile_col, tile_row);
      uint16_t entry_off = off + 2 + c * 2;
      scr[entry_off + 0] = static_cast<uint8_t>(ch & 0xFF);
      scr[entry_off + 1] = static_cast<uint8_t>(ch >> 8);
    }
  }
}

// --- Colour RAM setup (via DMA) ---

static uint8_t cram_row_buf[LINESTEP_BYTES];

static void setup_colour_ram() {
  cram_row_buf[0] = 0x10; // GOTOX flag
  cram_row_buf[1] = 0xFF; // all pixel rows visible
  for (uint8_t i = 2; i < LINESTEP_BYTES; ++i)
    cram_row_buf[i] = 0x00;

  auto dma = make_dma_copy(
      static_cast<uint32_t>(reinterpret_cast<uintptr_t>(cram_row_buf)),
      CRAM_ADDR, LINESTEP_BYTES);

  for (uint8_t row = 0; row < TOTAL_ROWS; ++row) {
    trigger_dma(dma);
    dma.dmalist.dest_addr += LINESTEP_BYTES;
  }
}

// --- Scrolling ---
//
// X scroll: GOTOX position (0 to -31), wraps at 32px (one tile width)
// Y scroll: fcm_yoffs 0-7 for fine, SCRNPTR/COLPTR for coarse (0-3 rows)

static uint16_t scroll_x;
static uint16_t scroll_y;

static void update_scroll() {
  auto *scr = reinterpret_cast<volatile uint8_t *>(SCREEN_ADDR);

  const uint8_t fine_x = scroll_x & 0x1F;
  const uint8_t fine_y = scroll_y & 0x07;
  const uint8_t coarse_y = (scroll_y >> 3) & 3;

  const uint16_t gotox_pos = (GOTOX_BASE - fine_x) & 0x3FF;

  for (uint8_t row = 0; row < TOTAL_ROWS; ++row) {
    uint16_t off = static_cast<uint16_t>(row) * LINESTEP_BYTES;
    scr[off + 0] = static_cast<uint8_t>(gotox_pos & 0xFF);
    scr[off + 1] = static_cast<uint8_t>((fine_y << 5) |
                                         ((gotox_pos >> 8) & 0x03));
  }

  uint16_t scr_offset = static_cast<uint16_t>(coarse_y) * LINESTEP_BYTES;
  VICIV.scrnptr = SCREEN_ADDR + scr_offset;
  VICIV.colptr = scr_offset;
}

static void wait_vblank() {
  while ((VICIV.rasterline | ((VICIV.ctrl1 & 0x80) << 1)) < 250)
    ;
  while ((VICIV.rasterline | ((VICIV.ctrl1 & 0x80) << 1)) >= 250)
    ;
}

// --- Main ---

int main() {
  setup_vic();
  setup_palette();
  generate_tile();

  {
    constexpr uint16_t TOTAL_BYTES = TOTAL_ROWS * LINESTEP_BYTES;
    auto fill_scr = make_dma_fill(SCREEN_ADDR, 0, TOTAL_BYTES);
    trigger_dma(fill_scr);
    auto fill_cram = make_dma_fill(CRAM_ADDR, 0, TOTAL_BYTES);
    trigger_dma(fill_cram);
  }

  setup_screen();
  setup_colour_ram();

  scroll_x = 0;
  scroll_y = 0;

  while (true) {
    wait_vblank();
    update_scroll();

    scroll_x = (scroll_x + 1) & 0x1F;
    scroll_y = (scroll_y + 1) & 0x1F;
  }
}
