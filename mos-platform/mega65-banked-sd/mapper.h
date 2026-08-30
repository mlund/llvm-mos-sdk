// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

#ifndef _MEGA65_BANKED_SD_MAPPER_H_
#define _MEGA65_BANKED_SD_MAPPER_H_

#include <_mapper.h>

/**
 * @brief Physical base address of each bank.
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
 * invisible to VIC-IV and SID, and absent on boards without HyperRAM. Good for
 * large tables and code off the hot path.
 *
 * mapper.s holds the same layout as MAP register values;
 * check-bank-tables.py fails the build if the two drift apart.
 */
#define BANK_PHYS_BASE_0  0x02000ul
#define BANK_PHYS_BASE_1  0x12000ul
#define BANK_PHYS_BASE_2  0x18000ul
#define BANK_PHYS_BASE_3  0x20000ul
#define BANK_PHYS_BASE_4  0x26000ul
#define BANK_PHYS_BASE_5  0x2C000ul
#define BANK_PHYS_BASE_6  0x32000ul
#define BANK_PHYS_BASE_7  0x38000ul
#define BANK_PHYS_BASE_8  0x40000ul
#define BANK_PHYS_BASE_9  0x46000ul
#define BANK_PHYS_BASE_10 0x4C000ul
#define BANK_PHYS_BASE_11 0x52000ul
#define BANK_PHYS_BASE_12 0x58000ul
#define BANK_PHYS_BASE_13 0x8000000ul
#define BANK_PHYS_BASE_14 0x8006000ul
#define BANK_PHYS_BASE_15 0x800C000ul

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

/**
 * @brief Call a banked function with arguments, and with a return value.
 *
 *     int n = banked_call_r(1, measure, text, len);
 *     banked_call_v(1, draw, x, y);
 *
 * Two macros because a statement expression cannot hold a void temporary:
 * _r yields the call's value, _v is for functions returning void.
 *
 * The call is direct, so the compiler marshals the real signature and sees
 * the call edge -- unlike banked_call(), which hides both behind a function
 * pointer. This works because the fixed region is above the window: MAPLO
 * moves nothing at $8000 and up, so the caller's own code survives the
 * switch.
 *
 * Two rules follow from that, and neither is diagnosed:
 *
 * - The caller must be in the fixed region. From a banked function the first
 *   set_bank() returns into a window that no longer holds the caller. Bank
 *   callers want banked_call().
 * - The argument expressions are evaluated with the target bank already
 *   mapped, so none of them may read the outgoing bank.
 */
#define banked_call_r(bank, fn, ...)                                           \
  ({                                                                           \
    char _prev_bank = get_bank();                                              \
    set_bank(bank);                                                            \
    __auto_type _result = (fn)(__VA_ARGS__);                                   \
    set_bank(_prev_bank);                                                      \
    _result;                                                                   \
  })

#define banked_call_v(bank, fn, ...)                                           \
  ({                                                                           \
    char _prev_bank = get_bank();                                              \
    set_bank(bank);                                                            \
    (fn)(__VA_ARGS__);                                                         \
    set_bank(_prev_bank);                                                      \
  })

#endif // _MEGA65_BANKED_SD_MAPPER_H_
