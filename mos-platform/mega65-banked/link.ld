/* MEGA65 banked PRG file linker script.
 *
 * Memory layout:
 *   $0000-$01FF  ZP + stack (unmapped)
 *   $0200-$1FFF  KERNAL variables (unmapped)
 *   $2000-$7FFF  BANKED window (24KB, switched via MAPLO)
 *   $8000-$CFFF  FIXED code/data (20KB, always bank 0)
 *   $D000-$DFFF  I/O registers
 *   $E000-$FFFF  KERNAL ROM
 *
 * The CRT init clears ROMC ($D030 bit 5) after loading banks, making
 * $C000-$CFFF available as RAM instead of C65 Interface ROM.
 * KERNAL CHROUT/printf still work (40-column editor is in $E000-$FFFF).
 *
 * Bank 0 occupies the $2000-$7FFF window at startup (no MAP needed).
 * All non-annotated code and data goes into ram_fixed ($8000-$CFFF).
 * Functions placed in .bank_N sections are called via banked_call().
 *
 * Physical memory (ROM at $20000-$3FFFF must be skipped; all banks offset
 * +$800 from 64KB page boundaries to avoid a KERNAL LOAD bug):
 *   Chip RAM:   Bank 0: $02000  Bank 1: $10800  Bank 2: $16800
 *   Fast RAM:   Bank 3: $40800  Bank 4: $46800  Bank 5: $4C800
 *               Bank 6: $52800  Bank 7: $58800
 *   Attic RAM:  Bank 8: $8000800 ... Bank 15: $8036800 (HyperRAM, ~10x slower)
 *
 * Chip RAM banks start in the second 64KB page ($10000+) to avoid
 * overlapping ram_fixed ($8000-$CFFF). Bank 2 ends at $1C7FF, safely
 * below the colour RAM window at $1F800-$1FFFF. Fast RAM banks use
 * $6000 spacing (= 24KB bank size) to maximize utilization.
 */

__basic_zp_start = 0x0002;
__basic_zp_end = 0x0090;

/* Imaginary registers in the BASIC zero-page area. */
__rc0 = __basic_zp_start;
INCLUDE imag-regs.ld

MEMORY {
    zp       : ORIGIN = __rc31 + 1, LENGTH = __basic_zp_end - (__rc31 + 1)
    ram (rw) : ORIGIN = 0x2001, LENGTH = 0x5fff
    ram_fixed (rw) : ORIGIN = 0x8000, LENGTH = 0x5000
}

INCLUDE _ram-banked.ld

INPUT(basic-header.o)
INPUT(unmap-basic.o)
INPUT(load-banks.o)
INPUT(load-banks-kernal.o)

/* All code and data defaults to the fixed bank. */
REGION_ALIAS("c_readonly", ram_fixed)
REGION_ALIAS("c_writeable", ram_fixed)

SECTIONS {
    .basic_header : { *(.basic_header) } >ram

    /* KERNAL loader in ram_fixed, before .text so it lands at $8000-$9FFF.
     * Must NOT be in ram ($2000-$7FFF) — the C65 ROM's autoboot/CLR wipes
     * everything after the BASIC stub ($200E-$7FFF) before SYS runs. */
    .kernal_load : { *(.text.kernal_load) } >ram_fixed

    INCLUDE c.ld
}

/* Set initial soft stack address to start of I/O region. (It grows down.)
 * ROMC is cleared during CRT init, making $C000-$CFFF RAM. */
__stack = 0xd000;

OUTPUT_FORMAT {
    SHORT(0x2001)
    FULL(ram)
    FULL(ram_fixed)
    FULL(bank_1)
    FULL(bank_2)
    FULL(bank_3)
    FULL(bank_4)
    FULL(bank_5)
    FULL(bank_6)
    FULL(bank_7)
    FULL(bank_8)
    FULL(bank_9)
    FULL(bank_10)
    FULL(bank_11)
    FULL(bank_12)
    FULL(bank_13)
    FULL(bank_14)
    FULL(bank_15)
}
