// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// The default bank loader: the SD card.

void __load_banks_hyppo(void);

__attribute__((weak, section(".bank_0"))) void __load_banks(void) {
  __load_banks_hyppo();
}
