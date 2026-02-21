// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// Per-port joystick/paddle read functions, tiered by controller type.

#include <mega65.h>

// CIA PRA keyboard matrix control.
#define CIA_LOCK_KEYBOARD 0xFF   // All PRA bits high — disable keyboard scan
#define CIA_UNLOCK_KEYBOARD 0x7F // Bit 7 low — re-enable keyboard scan

// CIA joystick data mask (bits 0-4: up, down, left, right, fire).
#define JOY_CIA_MASK 0x1F

// CIA PRA paddle select masks.
#define PADDLE_SELECT_PORT1 0x40 // PRA bit 6
#define PADDLE_SELECT_PORT2 0x80 // PRA bit 7

// Caller must have interrupts disabled and keyboard locked.
static void read_paddle_analog(unsigned char select_mask,
                               mega65_joy_state_t *joy) {

  // SID A/D conversion timing is calibrated for 1 MHz.
  VICIV.ctrlb &= ~VIC3_FAST_MASK;

  // Wait for the SID to sample the potentiometer resistance (~512 µs).
  // 256 iterations × (2 in_ + 3 bne) = ~1280 cycles at 1 MHz.
  // TODO: verify actual SID A/D conversion time — 1280 cycles may overshoot.
  CIA1.pra = select_mask;
  uint8_t counter = 0;
  asm volatile("1: in%0\n"
               "   bne 1b"
               :
               : "d"(counter));

  // Read twice and compare: the A/D converter can return stale values
  // if read during a conversion cycle.
  uint8_t v;
  do {
    v = SID1.ad1;
  } while (v != SID1.ad1);
  joy->paddle_a = v;
  do {
    v = SID1.ad2;
  } while (v != SID1.ad2);
  joy->paddle_b = v;

  VICIV.ctrlb |= VIC3_FAST_MASK;
}

void mega65_joy_read(unsigned char port, mega65_joy_state_t *joy) {
  // Disable interrupts and lock keyboard — the IRQ keyboard scan
  // writes to CIA PRA, which would corrupt joystick reads.
  asm volatile("sei");
  CIA1.pra = CIA_LOCK_KEYBOARD;
  joy->cia = ((port == 1) ? CIA1.prb : CIA1.pra) & JOY_CIA_MASK;
  CIA1.pra = CIA_UNLOCK_KEYBOARD;
  asm volatile("cli");
}

void mega65_joy_paddles(unsigned char port, mega65_joy_state_t *joy) {
  // Disable interrupts and lock keyboard — the IRQ keyboard scan
  // writes to CIA PRA, which would corrupt joystick reads.
  asm volatile("sei");
  CIA1.pra = CIA_LOCK_KEYBOARD;
  joy->cia = ((port == 1) ? CIA1.prb : CIA1.pra) & JOY_CIA_MASK;
  read_paddle_analog((port == 1) ? PADDLE_SELECT_PORT1 : PADDLE_SELECT_PORT2,
                     joy);
  CIA1.pra = CIA_UNLOCK_KEYBOARD;
  asm volatile("cli");
}
