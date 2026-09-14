// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// Bank loader for MAPPER_LOADER_FLOPPY: every non-empty bank off the mounted
// D81, as BANK1-BANK1F, with mega65_d81_load().

#include "_mapper.h"
#include <mega65.h>

// In .bank_0: it runs only at startup, with bank 0 mapped, so the window holds
// it. mega65_d81_load() is fixed code, callable at any time.
__attribute__((noinline, section(".bank_0")))
void __load_banks_floppy(void) {
  for (unsigned char bank = 1; bank < 32; ++bank) {
    char name[7] = "BANK";
    char *p = name + 4;
    unsigned char digit = bank & 15;
    if (!(__bank_used[bank >> 3] & 1 << (bank & 7)))
      continue;
    if (bank >= 16)
      *p++ = '1';
    *p = digit < 10 ? '0' + digit : 'A' + (digit - 10);
    uint32_t base = (uint32_t)__bank_megabyte[bank] << 20 |
                    (uint32_t)(__bank_addr_mid[bank] & 0x0F) << 16 |
                    (uint16_t)(__bank_addr_page[bank] << 8);
    if (!mega65_d81_load(name, base))
      __bank_load_failed(bank);
  }
}
