// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// Banked FCM (Full Color Mode) image demo for mega65-banked platform.
//
// Displays the LLVM-MOS logo as a full-screen 320x200 FCM image:
//   - Bank 4: 380 unique 8x8 FCM tiles in chip RAM (BANK_PHYS_BASE_4)
//   - Pre-converted from PNG via k-means tile quantization (380 tiles)
//   - Screen map assigns each of the 40x25 positions to its best tile
//   - 256-colour palette extracted from the source image
//   - Bank 5: screen map + palette in chip RAM (BANK_PHYS_BASE_5)
//   - VIC-IV scrnptr points directly to screen map in chip RAM
//   - DMA copies palette from chip RAM to VIC-IV palette registers
//
// FCM: char_address = screen_value * 64 (charptr ignored). The screen map
// holds tile indices; startup adds BANK_PHYS_BASE_4 / 64 so the image follows
// the bank layout.
//
// Binary data files are generated offline by convert-fcm.py and included
// via C23 #embed. The CRT bank loader places them in chip RAM at startup.

#include <cstdint>
#include <dma.hpp>
#include <mapper.h>
#include <mega65.h>

// Only banks 1-5 are used; the rest are left out of the image.
MAPPER_BANK_COUNT(5);

using namespace mega65::dma;

// Bank 4: 380 FCM tiles (8x8 pixels, 1 byte/pixel) = 24,320 bytes.
// Nearly fills the 24 KB bank (256 bytes to spare).
__attribute__((section(".bank_4"), retain, used))
const uint8_t fcm_tile_data[] = {
#embed "fcm-tiles.bin"
};

// Bank 5: screen map + palette (2,768 bytes of 24 KB). The map is writable
// because relocate_screen_map() updates it.
__attribute__((section(".bank_5.data"), retain, used))
uint8_t fcm_screen_map[] = {
#embed "fcm-screen.bin"
};

__attribute__((section(".bank_5"), retain, used))
const uint8_t fcm_palette_data[] = {
#embed "fcm-palette.bin"
};

// VIC-IV and DMA read physical memory; linked addresses are window addresses.
static uint32_t in_bank_5(const uint8_t *linked) {
  return BANK_PHYS_BASE_5 + ((uint16_t)linked - 0x2000);
}

// VIC-IV fetches characters only from chip RAM.
static_assert(BANK_PHYS_BASE_4 + sizeof(fcm_tile_data) <= 0x60000,
              "banked-fcm needs bank 4 in chip RAM");

static constexpr uint8_t CELL_COLS = 40;
static constexpr uint8_t CELL_ROWS = 25;
static constexpr uint16_t NUM_CELLS = CELL_COLS * CELL_ROWS;
static constexpr uint8_t CHR16_BYTES_PER_CHAR = 2;

static void setup_vic() {
  // Unlock VIC-IV registers (knock sequence).
  VICIV.key = VIC4_KEY_VICIV_A;
  VICIV.key = VIC4_KEY_VICIV_B;

  // Disable hot registers to prevent VIC-II writes from resetting state.
  VICIV.sdbdrwd_msb &= ~VIC4_HOTREG_MASK;

  // Extended attributes (required for FCM), 3.5 MHz, 40-column.
  VICIV.ctrlb = (VICIV.ctrlb | VIC3_FAST_MASK | VIC3_ATTR_MASK) &
                ~(VIC3_H640_MASK | VIC3_V400_MASK);

  // FCM: CHR16 for 16-bit screen values, FCLRHI for full-colour chars >= 256.
  VICIV.ctrlc =
      (VICIV.ctrlc & ~VIC4_FCLRLO_MASK) | VIC4_CHR16_MASK | VIC4_FCLRHI_MASK;

  // Point screen RAM directly at the screen map in bank 5.
  VICIV.scrnptr = in_bank_5(fcm_screen_map);

  // FCM ignores charptr — char_address = screen_value * 64 always.

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
      make_dma_copy(in_bank_5(fcm_palette_data), 0xFFD3100,
                    sizeof(fcm_palette_data));
  trigger_dma(copy);
}

// Clear colour RAM so FCM attributes (flip/trim) don't interfere.
// SEAM uses 2 bytes of colour RAM per character (byte 0: attributes like
// flip/alpha/GOTOX/NCM; byte 1: foreground colour), so clear 2000 bytes.
static void setup_colour_ram() {
  // Colour RAM is at $FF80000 in the 28-bit DMA address space.
  const auto fill = make_dma_fill(0xFF80000, 0, NUM_CELLS * 2);
  trigger_dma(fill);
}

// Translate tile indices to character numbers (tiles' physical address / 64).
static void relocate_screen_map() {
  const uint16_t base = BANK_PHYS_BASE_4 / 64;
  set_bank(5);
  for (uint16_t i = 0; i < sizeof(fcm_screen_map); i += 2) {
    const uint16_t tile = fcm_screen_map[i] | fcm_screen_map[i + 1] << 8;
    fcm_screen_map[i] = (uint8_t)(tile + base);
    fcm_screen_map[i + 1] = (uint8_t)((tile + base) >> 8);
  }
  set_bank(0);
}

int main() {
  relocate_screen_map();
  setup_vic();
  setup_palette();
  setup_colour_ram();

  for (;;)
    ;
}
