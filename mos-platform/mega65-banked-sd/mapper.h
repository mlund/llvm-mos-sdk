// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

#ifndef _MEGA65_BANKED_SD_MAPPER_H_
#define _MEGA65_BANKED_SD_MAPPER_H_

/**
 * @brief Default physical base of each bank, overridden by MAPPER_BANK_n.
 *
 * A bank is 24 KB -- the MAP window at $2000-$7FFF -- not one of the 64 KB
 * "banks" of the MEGA65 memory map.
 *
 * Banks 1-12 are full speed and reachable by VIC-IV, which fetches anything
 * below $60000. Banks 3-7 lie where the C65 ROMs would be; startup lifts the
 * write protection over that region, which also costs the character generator
 * at $2D000, so a program that wants a charset brings its own.
 *
 * Banks 13-15 are attic RAM (HyperRAM at $8000000+): roughly ten times slower,
 * out of reach of VIC-IV and audio DMA, and absent on boards without HyperRAM.
 * Good for large tables and code off the hot path.
 */
#define _MAPPER_DEFAULT_BANK_1  _MAPPER_UL(0x12000)
#define _MAPPER_DEFAULT_BANK_2  _MAPPER_UL(0x18000)
#define _MAPPER_DEFAULT_BANK_3  _MAPPER_UL(0x20000)
#define _MAPPER_DEFAULT_BANK_4  _MAPPER_UL(0x26000)
#define _MAPPER_DEFAULT_BANK_5  _MAPPER_UL(0x2C000)
#define _MAPPER_DEFAULT_BANK_6  _MAPPER_UL(0x32000)
#define _MAPPER_DEFAULT_BANK_7  _MAPPER_UL(0x38000)
#define _MAPPER_DEFAULT_BANK_8  _MAPPER_UL(0x40000)
#define _MAPPER_DEFAULT_BANK_9  _MAPPER_UL(0x46000)
#define _MAPPER_DEFAULT_BANK_10 _MAPPER_UL(0x4C000)
#define _MAPPER_DEFAULT_BANK_11 _MAPPER_UL(0x52000)
#define _MAPPER_DEFAULT_BANK_12 _MAPPER_UL(0x58000)
#define _MAPPER_DEFAULT_BANK_13 _MAPPER_UL(0x8000000)
#define _MAPPER_DEFAULT_BANK_14 _MAPPER_UL(0x8006000)
#define _MAPPER_DEFAULT_BANK_15 _MAPPER_UL(0x800C000)

#define _MAPPER_PLATFORM_CHECK(n)
#define _MAPPER_DEFAULT_LOADER 0

#include <_mapper.h>

/**
 * @brief Place uninitialised data in $0300-$1FFF, outside the bank window.
 *
 * The one region Hyppo can be handed a pointer into, since it only accepts a
 * staging page below $7F00.
 */
#define RAM_LOW __attribute__((section(".ram_low")))

/**
 * @brief Interrupts on this platform.
 *
 * The vectors at $FFFA-$FFFF are RAM. Startup points them at an RTI and
 * leaves interrupts disabled; a program that wants them writes its own
 * handler address there and clears the I flag.
 *
 * Hyppo traps need no care: entering hypervisor mode saves and restores the
 * map, megabyte bytes and CPU port in hardware.
 */

#endif // _MEGA65_BANKED_SD_MAPPER_H_
