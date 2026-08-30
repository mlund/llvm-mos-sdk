// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

#ifndef _MEGA65_BANKED_MAPPER_H_
#define _MEGA65_BANKED_MAPPER_H_

#include <_mapper.h>

/**
 * @brief Physical base address of each bank.
 *
 * Platform banks are 24 KB each (the MAP window at $2000-$7FFF), not the
 * 64 KB "banks" described in the MEGA65 memory map. Each bank's content
 * starts at the physical address below. ROM at $20000-$3FFFF creates a
 * gap between chip RAM banks (0-2) and fast RAM banks (3-7).
 *
 * Banks 3-15 sit +$800 into a 64KB page so KERNAL LOAD never sees a zero
 * address high byte, which makes it corrupt the destination. Banks 1 and 2
 * clear that by starting at $12000 and $18000, above the C65 DOS work area.
 *
 * Banks 8-15 are in attic RAM (HyperRAM at $8000000+). Attic RAM is
 * ~10x slower than chip/fast RAM and is not visible to VIC-IV or SID.
 * It is useful for large data sets, lookup tables, and code that does
 * not need peak performance. Two banks are packed per 64KB page
 * (at offsets +$0800 and +$6800) to avoid crossing 64KB boundaries.
 * Not available on Nexys A7 boards (no HyperRAM).
 *
 * SYNC: see _ram-banked.ld header for the full list of files encoding
 * bank layout.
 */
#define BANK_PHYS_BASE_0  0x02000ul
#define BANK_PHYS_BASE_1  0x12000ul
#define BANK_PHYS_BASE_2  0x18000ul
#define BANK_PHYS_BASE_3  0x40800ul
#define BANK_PHYS_BASE_4  0x46800ul
#define BANK_PHYS_BASE_5  0x4C800ul
#define BANK_PHYS_BASE_6  0x52800ul
#define BANK_PHYS_BASE_7  0x58800ul
#define BANK_PHYS_BASE_8  0x8000800ul
#define BANK_PHYS_BASE_9  0x8006800ul
#define BANK_PHYS_BASE_10 0x8010800ul
#define BANK_PHYS_BASE_11 0x8016800ul
#define BANK_PHYS_BASE_12 0x8020800ul
#define BANK_PHYS_BASE_13 0x8026800ul
#define BANK_PHYS_BASE_14 0x8030800ul
#define BANK_PHYS_BASE_15 0x8036800ul

/**
 * @brief Interrupts and the ROMs on this platform.
 *
 * MAP inhibits interrupts through a dedicated signal rather than the I flag
 * (gs4510.vhdl), which is why a bank switch returns with the caller's
 * interrupt state intact.
 *
 * Hyppo traps are unaffected by banking: entering hypervisor mode saves and
 * restores the map, megabyte bytes and CPU port in hardware. KERNAL disk I/O
 * is not -- it installs its own map and restores the KERNAL's, not yours.
 *
 * KERNAL disk calls also need the C65 interface ROM at $C000-$CFFF, which the
 * CRT unmaps to extend the fixed region. Set VIC3_ROMC_MASK in VICIV.ctrla for
 * the duration of such a call or it will not return. See the README.
 */

#endif // _MEGA65_BANKED_MAPPER_H_
