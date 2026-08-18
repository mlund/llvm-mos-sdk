// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// DMA audio playing a sample too large to hold anywhere else.
//
// drums.s8 is 44031 bytes: more than the 20 KB fixed region and more than one
// 24 KB bank, so banked-audio.S splits it across banks 1 and 2. Those two are
// adjacent in physical memory, which is what lets the hardware read the halves
// as one buffer.
//
// Nothing is ever mapped here. DMA reads memory directly, so it wants the
// physical address: the linked address of `drums` is a window address around
// $2000 and means nothing to the DMA controller. That is the same rule
// banked-fcm.cc follows for VIC-IV's scrnptr.
//
// Attic RAM cannot serve this. Banks 8-15 are invisible to DMA audio and SID
// however much room they have, so a sample belongs in a chip or fast RAM bank.

#include <mapper.h>
#include <mega65.h>
#include <stdint.h>

MAPPER_BANK_COUNT(2);

typedef unsigned _BitInt(24) uint24_t;

// Defined in banked-audio.S; declared only so the size below can refer to it.
extern "C" const uint8_t drums[];

constexpr uint32_t SAMPLE_ADDR = BANK_PHYS_BASE_1;
constexpr uint16_t SAMPLE_SIZE = 44031;

int main() {
  DMA.auden = DMA_AUDEN;
  DMA.ch0rvol = 0;
  DMA.ch0.enable = 0;

  DMA.ch0.baddr = (uint24_t)SAMPLE_ADDR;
  DMA.ch0.curaddr = (uint24_t)SAMPLE_ADDR;
  // Top address is 16-bit, so the sample must not cross a 64 KB page:
  // $10800 + 44031 = $1B5FF stays inside one.
  DMA.ch0.taddr = (uint16_t)(SAMPLE_ADDR + SAMPLE_SIZE);
  DMA.ch0.freq = 0x001a88;
  DMA.ch0.volume = 0x3f;
  DMA.ch0.enable = DMA_CHENABLE ^ DMA_CHSBITS_8 ^ DMA_CHLOOP;

  for (;;)
    VICIV.bordercol += 1; // leave a cycle for the DMA to steal
}
