// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// Bank loader for the SD card: every non-empty bank, as BANK1.BIN-BANK1F.BIN,
// through Hyppo.

#include "_mapper.h"
#include <mega65.h>

// In .bank_0: it runs only at startup, with bank 0 mapped, so without the
// KERNAL the window holds it and the fixed region stays free.
__attribute__((section(".bank_0"))) void __load_banks_hyppo(void) {
  char name[11];
  uint8_t bit = 1;

  name[0] = 'B';
  name[1] = 'A';
  name[2] = 'N';
  name[3] = 'K';
  for (uint8_t bank = 1; bank < 32; ++bank) {
    char *p = name + 4;
    uint8_t digit = bank & 15;
    uint8_t megabyte;
    uint32_t addr;

    bit = (uint8_t)(bit << 1);
    if (!bit)
      bit = 1;
    if (!(__bank_used[bank >> 3] & bit))
      continue;
    // Attic RAM, megabyte bit 7, takes its own trap and an offset into attic:
    // loadfile forces the top address byte to zero and still reports success.
    megabyte = __bank_megabyte[bank];
    addr = (uint32_t)(megabyte & 0x7F) << 20 |
           (uint32_t)(__bank_addr_mid[bank] & 0x0F) << 16 |
           (uint16_t)__bank_addr_page[bank] << 8;

    // BANK1-BANKF, then BANK10-BANK1F. Hyppo upper-cases the name it is asked
    // for but not the one on the card, so a lower-case file is never found.
    if (bank >= 16)
      *p++ = '1';
    *p++ = digit < 10 ? '0' + digit : 'A' + (digit - 10);
    p[0] = '.';
    p[1] = 'B';
    p[2] = 'I';
    p[3] = 'N';
    p[4] = 0;
    if (mega65_h_setname(name)) {
      __bank_load_failed(bank);
      continue;
    }

    if ((megabyte & 0x80) ? mega65_h_loadfile_attic(addr)
                          : mega65_h_loadfile(addr))
      __bank_load_failed(bank);
  }
}
