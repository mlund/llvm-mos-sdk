// Copyright 2023 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// Notes:
// - Requires `-std=c++20`
// - Manual sprite attribute setup using VERA
// - Compile time sprite generation
// - Inspiration from https://github.com/mwiedmann/cx16CodingInC

#include <cstdio>
#include <cx16.h>

// Compile-time integer square root; see
// https://gist.github.com/kimwalisch/d249cf684a58e1d892e1
consteval uint16_t sqrt_helper(uint16_t x, uint16_t lo, uint16_t hi) {
  if (lo == hi) {
    return lo;
  }
  const auto mid = (lo + hi + 1) / 2;
  if (x / mid < mid) {
    return sqrt_helper(x, lo, mid - 1);
  }
  return sqrt_helper(x, mid, hi);
}
consteval uint16_t sqrt(uint16_t x) { return sqrt_helper(x, 0, x / 2 + 1); }

// Set VRAM destination address and a stride (of 1 by default)
inline void vram_destination(const uint32_t address,
                             const uint8_t stride = VERA_INC_1) {
  VERA.address = (uint16_t)address;
  VERA.address_hi = (uint8_t)(address >> 16) | stride;
}

// Copy bytes to VRAM address
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

// Compile-time generation of a H x W pixels colored sprite
template <int H, int W> struct ColorBob {
  uint8_t bytes[H * W];
  consteval ColorBob() : bytes() {
    // Pick grey-scale colors
    auto get_color = [](auto radius) {
      constexpr auto LAST_GREY = 31;
      return radius >= 32 ? 0 : LAST_GREY - radius / 2;
    };
    size_t i = 0;
    for (int h = 0; h < H; h++) {
      for (int w = 0; w < W; w++) {
        auto x = w - W / 2;
        auto y = h - H / 2;
        auto radius = sqrt(x * x + y * y);
        bytes[i++] = get_color(radius);
      }
    }
  }
};

// Compile-time constants
constexpr uint16_t SCREEN_WIDTH = 640;
constexpr uint16_t SCREEN_HEIGHT = 480;
constexpr uint32_t SPR_ADDR = 0x13000;
constexpr auto SPR_IMG = ColorBob<64, 64>();
constexpr auto SPR1_ATTR_ADDR = sprite_attribute_addr(1);
constexpr uint8_t SPR_ATTR[] = {(uint8_t)(SPR_ADDR >> 5),
                                (uint8_t)(SPR_ADDR >> 13) | SPR_8BPP,
                                0, // xpos
                                0, // xpos
                                0, // ypos
                                0, // ypos
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
