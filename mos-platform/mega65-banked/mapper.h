// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

#ifndef _MEGA65_BANKED_MAPPER_H_
#define _MEGA65_BANKED_MAPPER_H_

/**
 * @brief Default physical base of each bank, overridden by MAPPER_BANK_n.
 *
 * Platform banks are 24 KB each (the MAP window at $2000-$7FFF), not the
 * 64 KB "banks" described in the MEGA65 memory map. Each bank's content
 * starts at the physical address below. Banks 0-7 are all chip RAM, which
 * VIC-IV fetches from directly; the ROMs at $20000-$3FFFF split them into
 * 0-2 and 3-7.
 *
 * Banks 3-15 sit +$800 into a 64KB page so KERNAL LOAD never sees a zero
 * address high byte, which makes it corrupt the destination. Banks 1 and 2
 * clear that by starting at $12000 and $18000, above the C65 DOS work area.
 *
 * Banks 8-15 are in attic RAM (HyperRAM at $8000000+). Attic RAM is
 * ~10x slower than chip RAM and out of reach of VIC-IV and audio DMA.
 * It is useful for large data sets, lookup tables, and code that does
 * not need peak performance. Two banks are packed per 64KB page
 * (at offsets +$0800 and +$6800) to avoid crossing 64KB boundaries.
 * Not available on Nexys A7 boards (no HyperRAM).
 */
#define _MAPPER_DEFAULT_BANK_1  _MAPPER_UL(0x12000)
#define _MAPPER_DEFAULT_BANK_2  _MAPPER_UL(0x18000)
#define _MAPPER_DEFAULT_BANK_3  _MAPPER_UL(0x40800)
#define _MAPPER_DEFAULT_BANK_4  _MAPPER_UL(0x46800)
#define _MAPPER_DEFAULT_BANK_5  _MAPPER_UL(0x4C800)
#define _MAPPER_DEFAULT_BANK_6  _MAPPER_UL(0x52800)
#define _MAPPER_DEFAULT_BANK_7  _MAPPER_UL(0x58800)
#define _MAPPER_DEFAULT_BANK_8  _MAPPER_UL(0x8000800)
#define _MAPPER_DEFAULT_BANK_9  _MAPPER_UL(0x8006800)
#define _MAPPER_DEFAULT_BANK_10 _MAPPER_UL(0x8010800)
#define _MAPPER_DEFAULT_BANK_11 _MAPPER_UL(0x8016800)
#define _MAPPER_DEFAULT_BANK_12 _MAPPER_UL(0x8020800)
#define _MAPPER_DEFAULT_BANK_13 _MAPPER_UL(0x8026800)
#define _MAPPER_DEFAULT_BANK_14 _MAPPER_UL(0x8030800)
#define _MAPPER_DEFAULT_BANK_15 _MAPPER_UL(0x8036800)

/* KERNAL LOAD corrupts a destination whose address high byte is $00, and the
 * C65 ROMs stay write-protected here. */
#define _MAPPER_PLATFORM_CHECK(n)                                              \
  _MAPPER_ASSERT((BANK_PHYS_BASE_##n & 0xFF00) != 0,                           \
                 "MAPPER_BANK_" #n " must not have a $00 high byte");          \
  _MAPPER_ASSERT(BANK_PHYS_BASE_##n + BANK_SIZE_##n <= 0x20000ul ||                 \
                     BANK_PHYS_BASE_##n >= 0x40000ul,                          \
                 "MAPPER_BANK_" #n " must avoid the ROMs at $20000-$3FFFF");

#ifdef MAPPER_LOADER_FLOPPY
#error "MAPPER_LOADER_FLOPPY needs mega65-banked-nokernal"
#endif
#define _MAPPER_DEFAULT_LOADER 2

#include <_mapper.h>

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
