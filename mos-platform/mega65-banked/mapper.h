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
 * gap between chip RAM banks (0-3) and fast RAM banks (4-7).
 *
 * Banks 8-15 are in attic RAM (HyperRAM at $8000000+). Attic RAM is
 * ~10x slower than chip/fast RAM and is not visible to VIC-IV or SID.
 * It is useful for large data sets, lookup tables, and code that does
 * not need peak performance. Two banks are packed per 64KB page
 * (at offsets $0000 and $6000) to avoid crossing 64KB boundaries.
 * Not available on Nexys A7 boards (no HyperRAM).
 */
#define BANK_PHYS_BASE_0  0x02000ul
#define BANK_PHYS_BASE_1  0x0A000ul
#define BANK_PHYS_BASE_2  0x12000ul
#define BANK_PHYS_BASE_3  0x1A000ul
#define BANK_PHYS_BASE_4  0x40000ul
#define BANK_PHYS_BASE_5  0x48000ul
#define BANK_PHYS_BASE_6  0x50000ul
#define BANK_PHYS_BASE_7  0x58000ul
#define BANK_PHYS_BASE_8  0x8000000ul
#define BANK_PHYS_BASE_9  0x8006000ul
#define BANK_PHYS_BASE_10 0x8010000ul
#define BANK_PHYS_BASE_11 0x8016000ul
#define BANK_PHYS_BASE_12 0x8020000ul
#define BANK_PHYS_BASE_13 0x8026000ul
#define BANK_PHYS_BASE_14 0x8030000ul
#define BANK_PHYS_BASE_15 0x8036000ul

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Call a function in the given bank.
 *
 * Maps the bank into $2000-$7FFF via the MAP instruction, calls the function,
 * then restores the previous bank. The caller and this trampoline must reside
 * in the fixed bank ($8000-$CFFF).
 *
 * @param bank_id Bank number (0-15). Physical mapping skips ROM at $20000:
 *                 Chip RAM:  0=$02000, 1=$0A000, 2=$12000, 3=$1A000
 *                 Fast RAM:  4=$40000, 5=$48000, 6=$50000, 7=$58000
 *                 Attic RAM: 8=$8000000 ... 15=$8036000
 * @param method  Function pointer (address within $2000-$7FFF).
 */
__attribute__((leaf, callback(2))) void banked_call(char bank_id,
                                                    void (*method)(void));

/**
 * @brief Get the currently mapped bank number.
 *
 * @return The current bank ID (0-15).
 */
__attribute__((leaf)) char get_bank(void);

/**
 * @brief Switch to the given bank.
 *
 * Experts only — prefer banked_call() for safe bank-switched function calls.
 * Maps the given bank into $2000-$7FFF. The caller must be in fixed code.
 *
 * @param bank_id Bank number (0-15).
 */
__attribute__((leaf)) void set_bank(char bank_id);

#ifdef __cplusplus
}
#endif

#endif // _MEGA65_MAPPER_H_
