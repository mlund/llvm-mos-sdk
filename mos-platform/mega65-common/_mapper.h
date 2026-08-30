// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// The half of <mapper.h> that does not depend on where the banks are. Each
// banked platform's own mapper.h supplies the addresses and includes this.

#ifndef _MEGA65_MAPPER_COMMON_H_
#define _MEGA65_MAPPER_COMMON_H_

/**
 * @brief Declare how many banks this program uses.
 *
 * Banks above @p n are given zero length by the linker script, so nothing is
 * reserved for them and they are left out of the output. Without this every
 * program reserves all 15, nearly all of it zeroes that still have to be read
 * back at boot.
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
 * @brief Interrupt handlers and banking.
 *
 * The map is global state: a handler runs with whichever bank the interrupted
 * code had mapped, and cannot know which that was. Handlers must live in the
 * fixed region and must not touch the bank window.
 *
 * Bank switching leaves the I flag alone, so set_bank() and banked_call()
 * return with the caller's interrupt state intact and an interrupt can arrive
 * at any point between them.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Call a function in the given bank.
 *
 * Maps the bank into the window, calls the function, then restores the
 * previous bank.
 *
 * A banked function may call this too. The trampoline is in the fixed region
 * and the previous bank rides on the hardware stack, so the caller's bank is
 * back in the window before control returns to it -- it is absent only while
 * its own code is not running. Nesting is bounded by the hardware stack.
 *
 * @param bank_id Bank number (0-15).
 * @param method  Function pointer (address within the window).
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

#endif // _MEGA65_MAPPER_COMMON_H_
