// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

#include "_mapper.h"

// In ZP for fast access from the banked_call trampoline hot path.
// Also referenced as .zeropage import in banked-call.s.
__attribute__((section(".zp.bss"))) volatile uint8_t _BANK_SHADOW;

__attribute__((leaf)) uint8_t get_bank(void) { return _BANK_SHADOW; }

// banked_call_r and banked_call_v switch through this. In a section of its own,
// so the linker refuses a caller in a bank, which the switch would unmap.
__attribute__((leaf, noinline, section(".banked_call_r_needs_fixed_caller")))
uint8_t __banked_call_enter(uint8_t bank_id) {
  uint8_t prev = _BANK_SHADOW;
  set_bank(bank_id);
  return prev;
}
