// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// CRT init: load every non-empty bank off the SD card.
//
// Runs at .init.250, after .data and .bss are in place and before
// constructors, which may already call into a bank.

#include <mapper.h>
#include <mega65.h>

extern const unsigned short __bank_sizes[15];

#define ATTIC_BASE 0x8000000ul
#define FIRST_ATTIC_BANK 6

// What each trap wants: the base for chip and fast RAM, the offset into attic
// for the rest. Every one is 4 KB-aligned, so a byte holds it and the table
// costs a quarter of what the addresses would.
#define SLOT_SHIFT 12
#define SLOT(addr) (unsigned char)((addr) >> SLOT_SHIFT)
#define ATTIC_SLOT(base) SLOT((base) - ATTIC_BASE)

static const unsigned char BANK_SLOT[15] = {
    SLOT(BANK_PHYS_BASE_1),        SLOT(BANK_PHYS_BASE_2),
    SLOT(BANK_PHYS_BASE_3),        SLOT(BANK_PHYS_BASE_4),
    SLOT(BANK_PHYS_BASE_5),        ATTIC_SLOT(BANK_PHYS_BASE_6),
    ATTIC_SLOT(BANK_PHYS_BASE_7),  ATTIC_SLOT(BANK_PHYS_BASE_8),
    ATTIC_SLOT(BANK_PHYS_BASE_9),  ATTIC_SLOT(BANK_PHYS_BASE_10),
    ATTIC_SLOT(BANK_PHYS_BASE_11), ATTIC_SLOT(BANK_PHYS_BASE_12),
    ATTIC_SLOT(BANK_PHYS_BASE_13), ATTIC_SLOT(BANK_PHYS_BASE_14),
    ATTIC_SLOT(BANK_PHYS_BASE_15),
};

// A base that does not fit the table would be silently truncated, and the
// bank would load somewhere else.
_Static_assert(((BANK_PHYS_BASE_1 | BANK_PHYS_BASE_2 | BANK_PHYS_BASE_3 |
                 BANK_PHYS_BASE_4 | BANK_PHYS_BASE_5 |
                 (BANK_PHYS_BASE_15 - ATTIC_BASE)) &
                ((1ul << SLOT_SHIFT) - 1)) == 0,
               "bank addresses must be 4 KB-aligned");
_Static_assert((BANK_PHYS_BASE_5 >> SLOT_SHIFT) <= 0xff &&
                   ((BANK_PHYS_BASE_15 - ATTIC_BASE) >> SLOT_SHIFT) <= 0xff,
               "bank addresses must fit a byte once shifted");

/// Replaceable. The border is the only output that needs no setup, and
/// returning would run a program whose code is silently absent.
__attribute__((weak)) void __bank_load_failed(unsigned char bank) {
  (void)bank;
  VICII.bordercolor = 2;
  for (;;)
    asm volatile("");
}

asm(".section .init.250,\"ax\",@progbits\n"
    "jsr __load_banks\n");

__attribute__((weak)) void __load_banks(void) {
  char name[] = "BANK0.BIN";

  for (unsigned char i = 0; i < 15; ++i) {
    unsigned char bank = i + 1;
    unsigned long addr;

    if (!__bank_sizes[i])
      continue;
    addr = (unsigned long)BANK_SLOT[i] << SLOT_SHIFT;

    // Hyppo upper-cases the name it is asked for but not the one on the card,
    // so a lower-case file can never be found.
    name[4] = bank <= 9 ? '0' + bank : 'A' + (bank - 10);
    if (mega65_h_setname(name))
      __bank_load_failed(bank);

    if (bank >= FIRST_ATTIC_BANK ? mega65_h_loadfile_attic(addr)
                                 : mega65_h_loadfile(addr))
      __bank_load_failed(bank);
  }
}
