// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// An interrupt handler alongside banked code. There is no KERNAL vector to
// chain through here: the program writes $FFFE itself.
//
// The handler stays in the fixed region and touches only .bss and I/O. It runs
// with whichever bank the interrupted code had mapped and cannot find out
// which, so reaching into $2000-$7FFF from it would read whatever happened to
// be there.

#include <cstdint>
#include <mapper.h>
#include <mega65.h>

MAPPER_BANK_COUNT(1);

#define IRQ_VECTOR (*(volatile uint16_t *)0xfffe)

constexpr uint8_t VIC_IRQ_RASTER = 0x01;

// .bss, so the handler and the banked function see the same bytes whatever is
// mapped.
volatile uint8_t ticks;

__attribute__((interrupt_norecurse)) void on_raster() {
  VICII.irr = VIC_IRQ_RASTER; // ack, or it re-fires on the way out
  ++ticks;
}

RODATA_BANK(1)
const uint8_t ramp[8] = {COLOR_BLACK,     COLOR_BLUE,  COLOR_PURPLE,
                         COLOR_LIGHTBLUE, COLOR_WHITE, COLOR_LIGHTBLUE,
                         COLOR_PURPLE,    COLOR_BLUE};

CODE_BANK(1) uint8_t shade(uint8_t step) { return ramp[(step >> 3) & 7]; }

int main() {
  IRQ_VECTOR = reinterpret_cast<uint16_t>(&on_raster);
  VICII.ctrl1 &= 0x7f; // raster compare is 9 bits; keep the line below 256
  VICII.rasterline = 100;
  VICII.irr = 0x0f; // drop anything pending before unmasking
  VICII.imr = VIC_IRQ_RASTER;
  asm volatile("cli" ::: "p");

  // The frame counter drives the colour, so the border only moves because
  // interrupts are arriving.
  while (true)
    VICIV.bordercol = banked_call_r(1, shade, ticks);
}
