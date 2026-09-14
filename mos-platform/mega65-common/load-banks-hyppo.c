// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// Bank loader for the SD card: every non-empty bank, as BANK1.BIN-BANKF.BIN,
// through Hyppo.

#include "_mapper.h"
#include <mega65.h>

// Addresses come from the tables the program's own files emit, so a layout it
// overrides is the one loaded.

// In .bank_0: it runs only at startup, with bank 0 mapped, so without the
// KERNAL the window holds it and the fixed region stays free.
__attribute__((section(".bank_0"))) void __load_banks_hyppo(void) {
  char name[] = "BANK0.BIN";

  for (unsigned char i = 0; i < 15; ++i) {
    unsigned char bank = i + 1;
    unsigned char megabyte;
    unsigned long addr;

    if (!__bank_used[i])
      continue;
    // Attic RAM, megabyte bit 7, takes its own trap and an offset into attic:
    // loadfile forces the top address byte to zero and still reports success.
    megabyte = __bank_megabyte[bank];
    addr = (unsigned long)(megabyte & 0x7F) << 20 |
           (unsigned long)(__bank_addr_mid[bank] & 0x0F) << 16 |
           (unsigned)__bank_addr_page[bank] << 8;

    // Hyppo upper-cases the name it is asked for but not the one on the card,
    // so a lower-case file can never be found.
    name[4] = bank <= 9 ? '0' + bank : 'A' + (bank - 10);
    if (mega65_h_setname(name))
      __bank_load_failed(bank);

    if ((megabyte & 0x80) ? mega65_h_loadfile_attic(addr)
                          : mega65_h_loadfile(addr))
      __bank_load_failed(bank);
  }
}
