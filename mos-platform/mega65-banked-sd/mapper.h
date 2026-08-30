// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

#ifndef _MEGA65_MAPPER_H_
#define _MEGA65_MAPPER_H_

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
 * @brief Declare how many banks this program uses.
 *
 * Banks above @p n are given zero length by the linker script, so nothing is
 * reserved for them and they are left out of the output. Without this every
 * program reserves all 15 -- 480 KB, nearly all of it zeroes that still have
 * to be read off the card at boot.
 *
 * Place it once at file scope: MAPPER_BANK_COUNT(3);
 *
 * @param n Highest bank number used (1-15). Bank 0 needs no declaration; it is
 *          the unmapped default.
 */
#define MAPPER_BANK_COUNT(n)                                                   \
  asm(".globl __ram_bank_count\n__ram_bank_count = " #n)

/**
 * @brief Place a function or read-only data in bank @p n.
 *
 *     CODE_BANK(1) void draw(void) { ... }
 *     RODATA_BANK(1) const uint8_t sine[256] = { ... };
 *
 * Functions need "noinline" or the compiler may inline them back into the
 * fixed region and leave the bank empty; CODE_BANK supplies it. Data that
 * nothing references needs "used" and "retain" to survive the compiler and
 * then --gc-sections; RODATA_BANK supplies both.
 *
 * Keep a table in the same bank as the code that reads it, so one banked_call
 * covers both.
 */
/* Two-step stringify, as atari2600-common/mapper_macros.h does it, so the
 * bank number may itself be a macro rather than only a literal. */
#define _BANK_STRINGIFY(n) #n
#define _BANK_SECTION(n) ".bank_" _BANK_STRINGIFY(n)
#define _CODE_BANK(sect) __attribute__((noinline, section(sect)))
#define _RODATA_BANK(sect) __attribute__((used, retain, section(sect)))
#define CODE_BANK(n) _CODE_BANK(_BANK_SECTION(n))
#define RODATA_BANK(n) _RODATA_BANK(_BANK_SECTION(n) ".rodata")

/**
 * @brief Place uninitialised data in $0300-$1FFF, outside the bank window.
 *
 * The one region Hyppo can be handed a pointer into, since it only accepts a
 * staging page below $7F00.
 */
#define RAM_LOW __attribute__((section(".ram_low")))

/**
 * @brief Interrupt handlers and banking.
 *
 * The map is global state: a handler runs with whichever bank the interrupted
 * code had mapped, and cannot know which that was. Handlers must live in the
 * fixed region and must not touch $2000-$7FFF.
 *
 * Bank switching leaves the I flag alone, so set_bank() and banked_call()
 * return with the caller's interrupt state intact and an interrupt can arrive
 * at any point between them.
 *
 * The vectors at $FFFA-$FFFF are RAM. Startup points them at an RTI and
 * leaves interrupts disabled; a program that wants them writes its own
 * handler address there and clears the I flag.
 *
 * Hyppo traps need no care: entering hypervisor mode saves and restores the
 * map, megabyte bytes and CPU port in hardware.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Call a function in the given bank.
 *
 * Maps the bank into $2000-$7FFF, calls the function, then restores the
 * previous bank.
 *
 * A banked function may call this too. The trampoline is in the fixed region
 * and the previous bank rides on the hardware stack, so the caller's bank is
 * back in the window before control returns to it -- it is absent only while
 * its own code is not running. Nesting is bounded by the hardware stack.
 *
 * @param bank_id Bank number (0-15).
 * @param method  Function pointer (address within $2000-$7FFF).
 */
/* "leaf" would normally be a lie here -- this re-enters C through
 * __call_indir. It is sound only because callback(2) restores the call edge
 * that leaf severs, so LLVM still sees the indirect target when it lays out
 * static stack frames. Do not drop either attribute. */
__attribute__((leaf, callback(2))) void banked_call(char bank_id,
                                                    void (*method)(void));

/**
 * @brief Get the currently mapped bank number.
 */
__attribute__((leaf)) char get_bank(void);

/**
 * @brief Put the hardware map back in step with get_bank().
 *
 * Only needed after something else has installed its own map and restored a
 * different one.
 */
__attribute__((leaf)) void resync_bank(void);

/**
 * @brief Switch to the given bank.
 *
 * Experts only -- prefer banked_call(). The caller must be in fixed code.
 *
 * @param bank_id Bank number, masked to its low four bits: 0x11 selects
 *                bank 1. An unmasked id would index past the bank tables
 *                and hand junk to MAP, which covers $0000-$7FFF and so
 *                could move zero page out from under the compiler.
 */
__attribute__((leaf)) void set_bank(char bank_id);

#ifdef __cplusplus
}
#endif

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

#endif // _MEGA65_MAPPER_H_
