// Copyright 2023 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// Notes:
// - Manual sprite attribute setup using VERA
// - Compile time sprite generation
// - Inspired by https://github.com/mwiedmann/cx16CodingInC

#include <array>
#include <cstdio>
#include <cx16.h>

// Set VRAM destination address and a stride of 1
inline void vram_destination(const uint32_t address) {
  VERA.address = (uint16_t)address;
  VERA.address_hi = (uint8_t)(address >> 16) | VERA_INC_1;
}

// Copy bytes to VRAM
template <typename T> void to_vram(const T &bytes, const uint32_t dst_address) {
  vram_destination(dst_address);
  for (const auto byte : bytes) {
    VERA.data0 = byte;
  }
}

// Compile-time evaluation of sprite attribute VRAM address
constexpr uint32_t sprite_attribute_addr(const uint8_t sprite_id) {
  return 0x1fc00 + sprite_id * 8;
}

// Compile-time generation a H x W pixels colored sprite
template <size_t H, size_t W> struct ColoredSprite {
  uint8_t bytes[H * W];
  consteval ColoredSprite() : bytes() {
    uint8_t color = 0;
    for (size_t i = 0; i < sizeof(bytes); i++) {
      bytes[i] = color;
      if (i % 16 == 0) {
        color++;
      }
    }
  }
};

/*
 * Compile time constants
 */
constexpr uint16_t SCREEN_WIDTH = 640;
constexpr uint16_t SCREEN_HEIGHT = 480;
constexpr uint32_t SPR_ADDR = 0x12c00;
constexpr auto SPR_IMG = ColoredSprite<64, 64>();
constexpr auto SPR1_ATTR_ADDR = sprite_attribute_addr(1);
constexpr uint8_t SPR_ATTR[] = {(uint8_t)(SPR_ADDR >> 5),
                                (uint8_t)(SPR_ADDR >> 13) | SPR_8BPP_MODE,
                                0, // xpos
                                0,
                                0, // ypos
                                0,
                                SPR_IN_FRONT_OF_LAYER1,
                                SPR_HEIGHT64 | SPR_WIDTH64};

int main(void) {

  // Copy sprite image and attributes to VRAM
  to_vram(SPR_IMG.bytes, SPR_ADDR);
  to_vram(SPR_ATTR, SPR1_ATTR_ADDR);

  vera_sprites_enable(1);

  // Move sprite
  uint16_t x = 0;
  uint16_t y = 200;
  while (true) {
    vram_destination(SPR1_ATTR_ADDR + 2);
    VERA.data0 = x;
    VERA.data0 = x >> 8;
    VERA.data0 = y;
    VERA.data0 = y >> 8;

    x += 2;
    y += 2;

    if (x >= SCREEN_WIDTH) {
      x = 0;
    }
    if (y >= SCREEN_HEIGHT) {
      y = 0;
    }

    waitvsync();
  }
}
