// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// Bank loader for MAPPER_LOADER_FLOPPY: every non-empty bank off the mounted
// D81, as BANK1-BANKF, with mega65_d81_load().

#define __MAPPER_NO_TABLES
#include <mapper.h>
#include <mega65.h>

extern const unsigned char __bank_used[15];
extern const unsigned char __bank_megabyte[16];
extern const unsigned char __bank_addr_mid[16];
extern const unsigned char __bank_addr_page[16];

void __load_banks_floppy(void);

// In .bank_0: it runs only at startup, with bank 0 mapped, so the window holds
// it. mega65_d81_load() is fixed code, callable at any time.
__attribute__((noinline, section(".bank_0")))
void __load_banks_floppy(void) {
  char name[] = "BANK0";

  for (unsigned char i = 0; i < 15; ++i) {
    unsigned char bank = i + 1;
    if (!__bank_used[i])
      continue;
    name[4] = bank <= 9 ? '0' + bank : 'A' + (bank - 10);
    uint32_t base = (uint32_t)__bank_megabyte[bank] << 20 |
                    (uint32_t)(__bank_addr_mid[bank] & 0x0F) << 16 |
                    (uint16_t)(__bank_addr_page[bank] << 8);
    if (!mega65_d81_load(name, base))
      __bank_load_failed(bank);
  }
}
