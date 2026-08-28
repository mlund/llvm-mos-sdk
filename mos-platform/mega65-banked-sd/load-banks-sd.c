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

/* Whether each bank holds anything, one byte apiece.  Not the sizes: a
 * sixteen-bit initialiser built from a linker-defined absolute relocates as two
 * low bytes rather than a low and a high, so a table of them links only while
 * every bank is under 256 bytes.  Nothing here wants the size anyway. */
extern const unsigned char __bank_used[15];

#define ATTIC_BASE 0x8000000ul

// Which banks need the attic trap, derived from the addresses rather than
// stated as a threshold: loadfile forces the top address byte to zero and
// still reports success, so picking the wrong trap loads nothing and says so
// to nobody.
#define ATTIC_BIT(n) ((BANK_PHYS_BASE_##n >= ATTIC_BASE) << n)
#define ATTIC_BANKS                                                            \
  (ATTIC_BIT(1) | ATTIC_BIT(2) | ATTIC_BIT(3) | ATTIC_BIT(4) |                 \
   ATTIC_BIT(5) | ATTIC_BIT(6) | ATTIC_BIT(7) | ATTIC_BIT(8) |                 \
   ATTIC_BIT(9) | ATTIC_BIT(10) | ATTIC_BIT(11) | ATTIC_BIT(12) |              \
   ATTIC_BIT(13) | ATTIC_BIT(14) | ATTIC_BIT(15))
#define IS_ATTIC(bank) ((ATTIC_BANKS >> (bank)) & 1)

// What each trap wants: the base for chip and fast RAM, the offset into attic
// for the rest. Every one is 4 KB-aligned, so a byte holds it and the table
// costs a quarter of what the addresses would.
#define SLOT_SHIFT 12
#define SLOT(addr) (unsigned char)((addr) >> SLOT_SHIFT)
// Attic wants an offset into attic, everything else the base itself. Picking
// per bank from the address, so a bank that moves needs no second edit here.
#define BANK_SLOT_OF(n)                                                        \
  SLOT(BANK_PHYS_BASE_##n >= ATTIC_BASE ? BANK_PHYS_BASE_##n - ATTIC_BASE      \
                                        : BANK_PHYS_BASE_##n)

static const unsigned char BANK_SLOT[15] = {
    BANK_SLOT_OF(1),  BANK_SLOT_OF(2),  BANK_SLOT_OF(3),  BANK_SLOT_OF(4),
    BANK_SLOT_OF(5),  BANK_SLOT_OF(6),  BANK_SLOT_OF(7),  BANK_SLOT_OF(8),
    BANK_SLOT_OF(9),  BANK_SLOT_OF(10), BANK_SLOT_OF(11), BANK_SLOT_OF(12),
    BANK_SLOT_OF(13), BANK_SLOT_OF(14), BANK_SLOT_OF(15),
};

// A base that does not fit the table would be silently truncated, and the
// bank would load somewhere else.
_Static_assert(((BANK_PHYS_BASE_1 | BANK_PHYS_BASE_12 |
                 (BANK_PHYS_BASE_15 - ATTIC_BASE)) &
                ((1ul << SLOT_SHIFT) - 1)) == 0,
               "bank addresses must be 4 KB-aligned");
_Static_assert((BANK_PHYS_BASE_12 >> SLOT_SHIFT) <= 0xff &&
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

    if (!__bank_used[i])
      continue;
    addr = (unsigned long)BANK_SLOT[i] << SLOT_SHIFT;

    // Hyppo upper-cases the name it is asked for but not the one on the card,
    // so a lower-case file can never be found.
    name[4] = bank <= 9 ? '0' + bank : 'A' + (bank - 10);
    if (mega65_h_setname(name))
      __bank_load_failed(bank);

    if (IS_ATTIC(bank) ? mega65_h_loadfile_attic(addr) : mega65_h_loadfile(addr))
      __bank_load_failed(bank);
  }
}
