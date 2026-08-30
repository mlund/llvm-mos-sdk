// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

#include "_mapper.h"

// Separate asm routine because MAP/EOM are 45GS02 instructions unavailable in C.
void __set_bank_asm(char bank_id);

// In ZP for fast access from the banked_call trampoline hot path.
// Also referenced as .zeropage import in mapper.s (banked_call).
__attribute__((section(".zp.bss"))) volatile char _BANK_SHADOW;

__attribute__((leaf)) char get_bank(void) { return _BANK_SHADOW; }

__attribute__((leaf)) void resync_bank(void) { __set_bank_asm(_BANK_SHADOW); }

__attribute__((leaf)) void set_bank(char bank_id) {
  _BANK_SHADOW = bank_id;
  __set_bank_asm(bank_id);
}
