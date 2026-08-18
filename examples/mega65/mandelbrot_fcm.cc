// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

//
// Full Color Mode (FCM) Mandelbrot set for MEGA65
//
// Renders the escape-time fractal at 320x200 with per-pixel palette
// coloring using VIC-IV Full Color Mode. Unlike the hires bitmap version
// (limited to 2 colors per 8x8 cell), each FCM pixel independently
// indexes a palette entry — eliminating the color blockiness inherent
// in character-cell-based graphics.
//
// MEGA65 features used:
//   - VIC-IV Full Color Mode: CHR16 + FCLRHI give each pixel in an
//     8x8 tile its own 8-bit palette index (256 colors per pixel)
//   - Hardware math accelerator ($D768): 32x32->64 bit multiply
//     replaces the slow __mulsi3 software multiply in the hot loop
//   - Enhanced DMA controller: copies rendered tile rows from CPU RAM
//     to graphics memory at $40000 (outside 16-bit address space)
//   - Custom palette RAM ($D100): 33-color gradient
//   - Full speed mode (~40 MHz)
//
// Memory layout:
//   Screen RAM:  $0800-$0FA7  (2000 bytes, 16-bit tile indices)
//   Color RAM:   $D800-$DFCF  (2000 bytes, 2 per tile in SEAM mode)
//   Tile buffer: (BSS)        (2560 bytes, rendered one row at a time)
//   Tile data:   $40000+      (64000 bytes, 1000 tiles x 64 bytes each)

#include <cstdint>
#include <dma.hpp>
#include <mega65.h>

using namespace mega65::dma;

// 8.8 fixed-point: high byte = integer, low byte = fraction
using fix16 = int16_t;

static constexpr fix16 FP_ONE = 256;
static constexpr uint8_t MAX_ITER = 32;

static constexpr uint8_t CELL_COLS = 40;
static constexpr uint8_t CELL_ROWS = 25;
static constexpr uint8_t TILE_PIXELS = 8;
static constexpr uint16_t TILE_BYTES = TILE_PIXELS * TILE_PIXELS; // 64
static constexpr uint16_t TILE_ROW_BYTES = CELL_COLS * TILE_BYTES;
// FCM tile addressing: charptr=0, so tile N maps to bytes [N*64, N*64+63].
// Graphics at $40000 → first tile index = $40000/64 = 4096.
static constexpr uint32_t GFX_ADDR = 0x40000UL;

// CPU-side buffer for one tile row (40 tiles x 64 bytes = 2560 bytes),
// DMA-copied to graphics memory after each row is computed.
static uint8_t tile_row_buf[TILE_ROW_BYTES];

static inline fix16 fp_mul(fix16 a, fix16 b) {
  // Set to false to use software multiply (__mulsi3) instead of the
  // hardware math accelerator — useful for benchmarking the speedup.
  constexpr bool USE_HW_MATH = true;
  if constexpr (USE_HW_MATH) {
    // Sign-extend 16-bit inputs to 32 bits. The hardware performs unsigned
    // 32x32->64 multiply, but the lower 32 bits match the signed result
    // when both operands have the same width.
    MATH.multina32 = static_cast<int32_t>(a);
    MATH.multinb32 = static_cast<int32_t>(b);
    // Compiler barrier: ensure both MMIO writes complete before the read.
    asm volatile("" ::: "memory");
    return static_cast<fix16>(static_cast<int32_t>(MATH.multout32) >> 8);
  } else {
    return static_cast<fix16>(static_cast<int32_t>(a) * b >> 8);
  }
}

// Skip points inside the main cardioid or period-2 bulb — these always
// reach MAX_ITER. Detecting them with ~4 multiplies avoids 32 wasted
// iterations (~96 multiplies) for every pixel in the large black region.
static bool inside_cardioid_or_bulb(fix16 cr, fix16 ci) {
  constexpr fix16 FP_QUARTER = FP_ONE / 4;
  constexpr fix16 FP_SIXTEENTH = FP_ONE / 16;

  // Main cardioid: q*(q + cr - 1/4) < ci²/4
  const fix16 cr_shifted = cr - FP_QUARTER;
  const fix16 ci2 = fp_mul(ci, ci);
  const fix16 q = fp_mul(cr_shifted, cr_shifted) + ci2;
  if (fp_mul(q, static_cast<fix16>(q + cr_shifted)) < (ci2 >> 2))
    return true;

  // Period-2 bulb: (cr + 1)² + ci² < 1/16
  const fix16 cr1 = static_cast<fix16>(cr + FP_ONE);
  if (fp_mul(cr1, cr1) + ci2 < FP_SIXTEENTH)
    return true;

  return false;
}

static uint8_t mandelbrot(fix16 cr, fix16 ci) {
  if (inside_cardioid_or_bulb(cr, ci))
    return MAX_ITER;
  constexpr fix16 FP_FOUR = 4 * FP_ONE;
  fix16 zr = 0, zi = 0;
  for (uint8_t i = 0; i < MAX_ITER; ++i) {
    fix16 zr2 = fp_mul(zr, zr);
    fix16 zi2 = fp_mul(zi, zi);
    if (zr2 + zi2 > FP_FOUR)
      return i;
    zi = fp_mul(zr, zi);
    zi += zi; // 2 * zr * zi
    zi += ci;
    zr = zr2 - zi2 + cr;
  }
  return MAX_ITER;
}

// Blue -> cyan -> green -> yellow -> red gradient (32 steps).
// Four segments of SEGMENT_LEN, each interpolating one channel 0->MAX or
// MAX->0.
struct PaletteData {
  static constexpr uint8_t SEGMENT_LEN = 8;    // iterations per color segment
  static constexpr uint8_t MAX_INTENSITY = 15; // 4-bit palette channel max

  uint8_t r[MAX_ITER + 1]{}, g[MAX_ITER + 1]{}, b[MAX_ITER + 1]{};

  // Palette register uses reversed-nybble encoding: replicate
  // 4-bit intensity into both nybbles for full brightness.
  static constexpr uint8_t nyb(uint8_t n) { return (n << 4) | n; }

  constexpr PaletteData() {
    for (uint8_t i = 0; i < MAX_ITER; ++i) {
      // Interpolate 0 -> MAX_INTENSITY over each segment
      const uint8_t v =
          (i & (SEGMENT_LEN - 1)) * MAX_INTENSITY / (SEGMENT_LEN - 1);
      uint8_t rv = 0, gv = 0, bv = 0;
      if (i < SEGMENT_LEN) {
        gv = v;
        bv = MAX_INTENSITY;
      } else if (i < SEGMENT_LEN * 2) {
        gv = MAX_INTENSITY;
        bv = MAX_INTENSITY - v;
      } else if (i < SEGMENT_LEN * 3) {
        rv = v;
        gv = MAX_INTENSITY;
      } else {
        rv = MAX_INTENSITY;
        gv = MAX_INTENSITY - v;
      }
      r[i + 1] = nyb(rv);
      g[i + 1] = nyb(gv);
      b[i + 1] = nyb(bv);
    }
  }
};

static constexpr PaletteData palette{};

static void setup_palette() {
  for (uint8_t i = 0; i <= MAX_ITER; ++i) {
    PALETTE.red[i] = palette.r[i];
    PALETTE.green[i] = palette.g[i];
    PALETTE.blue[i] = palette.b[i];
  }
}

static void setup_vic() {
  constexpr uint8_t VICIV_KEY1 = 0x47; // VIC-IV unlock knock sequence
  constexpr uint8_t VICIV_KEY2 = 0x53;
  constexpr uint8_t CPU_DDR_40MHZ = 65; // POKE 0,65 for full speed
  constexpr uint8_t CHR16_BYTES_PER_CHAR = 2;

  // Prevent KERNAL IRQ handler from corrupting VIC-IV register state.
  asm volatile("sei");

  // Unlock VIC-IV registers (knock sequence)
  VICIV.key = VICIV_KEY1;
  VICIV.key = VICIV_KEY2;

  // Disable VIC-II hot registers so we can program VIC-IV directly
  VICIV.sdbdrwd_msb &= ~VIC4_HOTREG_MASK;

  // 40 MHz: POKE 0,65 sets full speed via CPU port DDR.
  // VIC3_FAST alone only gives 3.5 MHz (C65 speed).
  CPU_PORTDDR = CPU_DDR_40MHZ;

  // Extended attributes (required for FCM), 40-column
  VICIV.ctrlb = (VICIV.ctrlb | VIC3_FAST_MASK | VIC3_ATTR_MASK) &
                ~(VIC3_H640_MASK | VIC3_V400_MASK);

  // FCM: 16-bit character indices + full color for chars > $FF.
  // FCLRHI without FCLRLO means only tiles with index > 255 use
  // full-color mode — our tile indices start at 4096, so all qualify.
  VICIV.ctrlc =
      (VICIV.ctrlc & ~VIC4_FCLRLO_MASK) | VIC4_CHR16_MASK | VIC4_FCLRHI_MASK;

  // Reuse KERNAL's default screen area — avoids relocating 2KB of RAM.
  VICIV.scrnptr = 0x0800;

  // Character data base at 0 — tile index N maps to address N*64
  VICIV.charptr = 0x0000;

  // 80 bytes per screen row (2 bytes per char in CHR16 mode)
  VICIV.linestep = CELL_COLS * CHR16_BYTES_PER_CHAR;
  VICIV.chrcount = CELL_COLS;
  VICIV.disp_rows = CELL_ROWS;

  // VIC-IV ctrl1/ctrl2 bit masks (VIC-II compatible, no SDK definitions yet)
  constexpr uint8_t VIC4_RST8_MASK = 0x80; // Raster MSB
  constexpr uint8_t VIC4_ECM_MASK = 0x40;  // Extended color mode
  constexpr uint8_t VIC4_DEN_MASK = 0x10;  // Display enable
  constexpr uint8_t VIC4_RSEL_MASK = 0x08; // 25-row select
  constexpr uint8_t VIC4_CSEL_MASK = 0x08; // 40-column select (ctrl2)

  // FCM is a text-mode extension — BMM must be off.
  VICIV.ctrl1 = (VICIV.ctrl1 & (VIC4_RST8_MASK | VIC4_ECM_MASK)) |
                VIC4_DEN_MASK | VIC4_RSEL_MASK | 3; // YSCROLL=3
  VICIV.ctrl2 = (VICIV.ctrl2 & 0xE0) | VIC4_CSEL_MASK;

  VICIV.bordercol = 0;
  VICIV.screencol = 0;

  // Use palette RAM for colors 0-15 (16+ always use palette RAM)
  VICIV.ctrla |= VIC3_PAL_MASK;
}

static void setup_screen() {
  constexpr uint16_t NUM_CELLS = CELL_COLS * CELL_ROWS;
  constexpr uint16_t TILE_BASE = GFX_ADDR / TILE_BYTES;
  constexpr uint16_t COLOR_RAM_ADDR = 0xD800;
  auto *const SCREEN16 = reinterpret_cast<volatile uint16_t *>(&DEFAULT_SCREEN);
  auto *const COLOR_RAM = reinterpret_cast<volatile uint8_t *>(COLOR_RAM_ADDR);

  setup_palette();

  // Each screen cell maps to a unique tile in the $40000 graphics area.
  for (uint16_t i = 0; i < NUM_CELLS; ++i)
    SCREEN16[i] = TILE_BASE + i;

  // SEAM uses 2 bytes of colour RAM per character (byte 0: attributes like
  // flip/alpha/GOTOX/NCM; byte 1: foreground colour). Zero both to prevent
  // unwanted FCM attributes.
  for (uint16_t i = 0; i < NUM_CELLS * 2; ++i)
    COLOR_RAM[i] = 0;

  // Zero the graphics area so partially-rendered rows display as black.
  const auto dma = make_dma_fill(GFX_ADDR, 0, NUM_CELLS * TILE_BYTES);
  trigger_dma(dma);
}

// Render fractal one tile row at a time, DMA-copying each
// completed row to graphics memory at $40000+.
static void render_fractal() {
  constexpr uint16_t SCREEN_COLS = 320;
  constexpr uint16_t SCREEN_ROWS = 200;

  // View window: real [-2.0, 0.6], imag [-1.0, 1.0]
  constexpr fix16 RE_MIN = -2 * FP_ONE;
  constexpr fix16 RE_MAX = static_cast<fix16>(0.6 * FP_ONE);
  constexpr fix16 IM_MIN = -1 * FP_ONE;
  constexpr fix16 IM_MAX = 1 * FP_ONE;

  constexpr fix16 RE_STEP = (RE_MAX - RE_MIN) / SCREEN_COLS;
  constexpr fix16 IM_STEP = (IM_MAX - IM_MIN) / SCREEN_ROWS;

  fix16 ci = IM_MIN;

  for (uint8_t cy = 0; cy < CELL_ROWS; ++cy) {
    // Scanline-order: iterate py before cx so ci advances once per row.
    for (uint8_t py = 0; py < TILE_PIXELS; ++py) {
      fix16 cr = RE_MIN;

      for (uint8_t cx = 0; cx < CELL_COLS; ++cx) {
        const uint16_t tile_off =
            static_cast<uint16_t>(cx) * TILE_BYTES + py * TILE_PIXELS;

        for (uint8_t px = 0; px < TILE_PIXELS; ++px) {
          const uint8_t iter = mandelbrot(cr, ci);

          tile_row_buf[tile_off + px] =
              (iter >= MAX_ITER) ? 0 : static_cast<uint8_t>(iter + 1);
          cr += RE_STEP;
        }
      }
      ci += IM_STEP;
    }

    const auto dma = make_dma_copy(
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tile_row_buf)),
        GFX_ADDR + static_cast<uint32_t>(cy) * TILE_ROW_BYTES, TILE_ROW_BYTES);
    trigger_dma(dma);
  }
}

int main() {
  setup_vic();
  setup_screen();
  render_fractal();

  while (true)
    ;
}
