// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// Banked FCM (Full Color Mode) image demo for mega65-banked platform.
//
// Displays the LLVM-MOS logo as a full-screen 320x200 FCM image:
//   - Bank 4: 380 unique 8x8 FCM tiles in fast RAM at physical $40000
//   - Pre-converted from PNG via k-means tile quantization (380 tiles)
//   - Screen map assigns each of the 40x25 positions to its best tile
//   - 256-colour palette extracted from the source image
//   - Bank 5: screen map + palette in fast RAM at physical $48000
//   - VIC-IV scrnptr points directly to screen map in fast RAM
//   - DMA copies palette from fast RAM to VIC-IV palette registers
//
// FCM uses absolute addressing: screen value = (charptr + physical_address)
// / 64. With charptr = 0, tile N at $40000 -> screen value $40000/64 + N =
// $1000 + N.
//
// Binary data files are generated offline by convert-fcm.py and included
// via C23 #embed. The CRT bank loader places them in fast RAM at startup.

#include <cstdint>
#include <dma.hpp>
#include <mapper.h>
#include <mega65.h>

using namespace mega65::dma;

// Bank 4 ($40000): 380 FCM tiles (8x8 pixels, 1 byte/pixel) = 24,320 bytes.
// Nearly fills the 24 KB bank (256 bytes to spare).
__attribute__((section(".bank_4"), retain, used))
const uint8_t fcm_tile_data[] = {
#embed "fcm-tiles.bin"
};

// Bank 5 ($48000): screen map + palette (2,768 bytes of 24 KB).
__attribute__((section(".bank_5"), retain, used))
const uint8_t fcm_screen_map[] = {
#embed "fcm-screen.bin"
};

__attribute__((section(".bank_5"), retain, used))
const uint8_t fcm_palette_data[] = {
#embed "fcm-palette.bin"
};

// Physical addresses for VIC-IV scrnptr and DMA palette source.
static constexpr uint32_t SCREEN_MAP_ADDR = BANK_PHYS_BASE_5;
static constexpr uint32_t PALETTE_ADDR = BANK_PHYS_BASE_5 + sizeof(fcm_screen_map);

static constexpr uint8_t CELL_COLS = 40;
static constexpr uint8_t CELL_ROWS = 25;
static constexpr uint16_t NUM_CELLS = CELL_COLS * CELL_ROWS;
static constexpr uint8_t CHR16_BYTES_PER_CHAR = 2;

static void setup_vic() {
  // The CRT loader enables interrupts (CLI) before main(). Disable them
  // to prevent the KERNAL IRQ handler from writing VIC registers mid-setup.
  asm volatile("sei" ::: "p");

  // Unlock VIC-IV registers (knock sequence).
  VICIV.key = 0x47;
  VICIV.key = 0x53;

  // Disable hot registers to prevent VIC-II writes from resetting state.
  VICIV.sdbdrwd_msb &= ~VIC4_HOTREG_MASK;

  // Extended attributes (required for FCM), 3.5 MHz, 40-column.
  VICIV.ctrlb = (VICIV.ctrlb | VIC3_FAST_MASK | VIC3_ATTR_MASK) &
                ~(VIC3_H640_MASK | VIC3_V400_MASK);

  // FCM: CHR16 for 16-bit screen values, FCLRHI for full-colour chars >= 256.
  VICIV.ctrlc =
      (VICIV.ctrlc & ~VIC4_FCLRLO_MASK) | VIC4_CHR16_MASK | VIC4_FCLRHI_MASK;

  // Point screen RAM directly at the screen map in bank 4.
  VICIV.scrnptr = SCREEN_MAP_ADDR;

  // Character data base at 0 — tile index N maps to address N*64.
  VICIV.charptr = 0;

  // 80 bytes per screen row (40 chars x 2 bytes in CHR16).
  VICIV.linestep = CELL_COLS * CHR16_BYTES_PER_CHAR;
  VICIV.chrcount = CELL_COLS;
  VICIV.disp_rows = CELL_ROWS;

  // FCM is a text-mode extension -- BMM and MCM must be off.
  VICIV.ctrl1 = (VICIV.ctrl1 & 0xC0) | 0x1B; // DEN | RSEL | YSCROLL=3
  VICIV.ctrl2 = (VICIV.ctrl2 & 0xE0) | 0x08; // CSEL

  // FCM treats pixel value 0 as transparent (shows screencol).
  // convert-fcm.py reserves entry 0 for the darkest colour and shifts all
  // image indices to 1-255, so no tile pixel is ever transparent.
  VICIV.bordercol = 0;
  VICIV.screencol = 0;

  // Use palette RAM for entries 0-15 (16+ always use palette RAM).
  VICIV.ctrla |= VIC3_PAL_MASK;
}

// DMA palette data from bank 5 to VIC-IV palette registers ($FFD3100).
static void setup_palette() {
  const auto copy =
      make_dma_copy(PALETTE_ADDR, 0xFFD3100, sizeof(fcm_palette_data));
  trigger_dma(copy);
}

// Clear colour RAM so FCM attributes (flip/trim) don't interfere.
static void setup_colour_ram() {
  // Colour RAM is at $FF80000 in the 28-bit DMA address space.
  const auto fill = make_dma_fill(0xFF80000, 0, NUM_CELLS);
  trigger_dma(fill);
}

int main() {
  setup_vic();
  setup_palette();
  setup_colour_ram();

  for (;;)
    ;
}
