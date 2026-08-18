// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

#ifndef _MEGA65_MAPPER_H_
#define _MEGA65_MAPPER_H_

/**
 * @brief Physical base address of each bank.
 *
 * Platform banks are 24 KB each (the MAP window at $2000-$7FFF), not the
 * 64 KB "banks" described in the MEGA65 memory map. Each bank's content
 * starts at the physical address below. ROM at $20000-$3FFFF creates a
 * gap between chip RAM banks (0-2) and fast RAM banks (3-7).
 *
 * All banks are offset +$800 from 64KB page boundaries to avoid a KERNAL
 * LOAD bug that corrupts data when load_addr_hi=$00.
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
 * @brief Declare how many banks this program uses.
 *
 * Banks above @p n are given zero length by the linker script, so nothing is
 * reserved for them and they are left out of the output entirely. Without this
 * every program reserves all 15 -- around 414 KB, nearly all of it zeroes that
 * still have to be read off the disk at boot.
 *
 * Place it once at file scope: MAPPER_BANK_COUNT(3);
 *
 * @param n Highest bank number used (1-15). Bank 0 needs no declaration; it is
 *          the default mapping and lives in chip RAM rather than a bank slot.
 */
#define MAPPER_BANK_COUNT(n)                                                   \
  asm(".globl __ram_bank_count\n__ram_bank_count = " #n)

/**
 * @brief Place a function or read-only data in bank @p n.
 *
 * Wraps the section attribute so the bank number can be a macro rather than a
 * string that has to be spelled correctly by hand:
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
 * fixed region and must not touch $2000-$7FFF.
 *
 * Bank switching itself leaves the I flag alone -- MAP inhibits interrupts
 * through a dedicated signal rather than the I flag (gs4510.vhdl), so
 * set_bank() and banked_call() return with the caller's interrupt state
 * intact and an interrupt can arrive at any point between them.
 *
 * Hyppo traps are unaffected: entering hypervisor mode saves and restores the
 * map, megabyte bytes and CPU port in hardware. KERNAL disk I/O is not -- it
 * installs its own map and restores the KERNAL's, not yours.
 *
 * KERNAL disk calls also need the C65 interface ROM at $C000-$CFFF, which the
 * CRT unmaps to extend the fixed region. Set VIC3_ROMC_MASK in VICIV.ctrla for
 * the duration of such a call or it will not return. See the README.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Call a function in the given bank.
 *
 * Maps the bank into $2000-$7FFF via the MAP instruction, calls the function,
 * then restores the previous bank. The caller and this trampoline must reside
 * in the fixed region ($8000-$CFFF; the CRT clears ROMC, so $C000-$CFFF is
 * RAM).
 *
 * Nesting is safe: the previous bank is pushed on the hardware stack, so a
 * banked function may return to fixed code that makes a further banked_call.
 * It may not call another bank directly -- while the inner bank is mapped,
 * the outer bank's own code is not in the window.
 *
 * @param bank_id Bank number (0-15). Physical mapping skips ROM at $20000:
 *                 Chip RAM:  0=$02000, 1=$12000, 2=$18000
 *                 Fast RAM:  3=$40800, 4=$46800, 5=$4C800, 6=$52800, 7=$58800
 *                 Attic RAM: 8=$8000800 ... 15=$8036800
 * @param method  Function pointer (address within $2000-$7FFF).
 */
/* "leaf" would normally be a lie here -- this re-enters C through
 * __call_indir. It is sound only because callback(2) restores the call edge
 * that leaf severs, so LLVM still sees the indirect target when it lays out
 * static stack frames. Measured with both: two distinct 8-byte frames
 * (.zp.noinit 16 bytes, .text 0x113). With callback(2) alone one function
 * falls back to the soft stack (8 bytes, 0x141). Do not drop either. */
__attribute__((leaf, callback(2))) void banked_call(char bank_id,
                                                    void (*method)(void));

/**
 * @brief Get the currently mapped bank number.
 *
 * @return The current bank ID (0-15).
 */
__attribute__((leaf)) char get_bank(void);

/**
 * @brief Put the hardware map back in step with get_bank().
 *
 * A KERNAL disk call installs its own mapping and restores the KERNAL's, not
 * yours, so the window comes back holding a different bank while get_bank()
 * still reports the old one. Call this afterwards.
 */
__attribute__((leaf)) void resync_bank(void);

/**
 * @brief Switch to the given bank.
 *
 * Experts only — prefer banked_call() for safe bank-switched function calls.
 * Maps the given bank into $2000-$7FFF. The caller must be in fixed code.
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

#endif // _MEGA65_MAPPER_H_
