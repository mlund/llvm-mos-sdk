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
#define BANK_PHYS_BASE_1  0x10800ul
#define BANK_PHYS_BASE_2  0x16800ul
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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Call a function in the given bank.
 *
 * Maps the bank into $2000-$7FFF via the MAP instruction, calls the function,
 * then restores the previous bank. The caller and this trampoline must reside
 * in the fixed bank ($8000-$BFFF).
 *
 * @param bank_id Bank number (0-15). Physical mapping skips ROM at $20000:
 *                 Chip RAM:  0=$02000, 1=$10800, 2=$16800
 *                 Fast RAM:  3=$40800, 4=$46800, 5=$4C800, 6=$52800, 7=$58800
 *                 Attic RAM: 8=$8000800 ... 15=$8036800
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
