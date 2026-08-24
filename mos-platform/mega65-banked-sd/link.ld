/* MEGA65 banked PRG linker script, SD-card variant.
 *
 * BASIC and the KERNAL are mapped out for good, so the ROM windows are RAM
 * and the bank window gains a MAPHI block:
 *
 *   $0002-$00FF  imaginary registers, then 222 bytes of compiler zero page
 *   $0100-$01FF  hardware stack
 *   $0200-$02FF  Hyppo filename staging page
 *   $0300-$1FFF  ram_low, uninitialised
 *   $2000-$7FFF  BANKED window (24KB, MAPLO blocks 1-3)
 *   $8000-$CFFF  FIXED code, rodata and .data initialisers
 *   $D000-$DFFF  I/O
 *   $E000-$FFF9  FIXED .data, .bss, .noinit, and the soft stack
 *   $FFFA-$FFFF  interrupt vectors, installed by boot.S
 *
 * MAPHI is left alone, so everything from $8000 up stays where the linker put
 * it and the window costs nothing above $7FFF.
 *
 * Physical bank bases. Banks 3-7 sit where the C65 ROMs would be; startup
 * lifts the write protection over that region because this platform runs
 * none of them:
 *   0: $02000    1: $12000    2: $18000
 *   3: $20000    4: $26000    5: $2C000    6: $32000    7: $38000
 *   8: $40000    9: $46000   10: $4C000   11: $52000   12: $58000
 *   13: $8000000  14: $8006000  15: $800C000  (attic, ~10x slower)
 *
 * Bank 1 sits above the C65 DOS work area ($10000-$11FFF) and ends at
 * $19FFF, below the colour RAM window at $1F800.
 */

__basic_zp_start = 0x0002;
__basic_zp_end = 0x0100;
/* Size of BASIC ZP region for save-basic library. This script does what
 * commodore.ld does for the other Commodore targets, but cannot include it:
 * that file aliases the C regions to ram and declares its own SECTIONS, both
 * of which the banked layout replaces. */
__basic_zp_size = __basic_zp_end - __basic_zp_start;

__rc0 = __basic_zp_start;
INCLUDE imag-regs.ld

/* The zp region and -mlto-zp describe the same bytes to the linker and to the
 * compiler; nothing else makes them agree. */
ASSERT(__basic_zp_end - (__rc31 + 1) == 222,
       "mega65-banked-sd: zp region size must match -mlto-zp=222")

/* Hyppo reads the filename by page number, and only accepts a page below
 * $7F00, so this cannot move into the fixed region. */
PROVIDE(__mega65_h_name_buf = 0x0200);
ASSERT((__mega65_h_name_buf & 0xFF) == 0 && __mega65_h_name_buf >= 0x0100 &&
       __mega65_h_name_buf < 0x7F00,
       "mega65-banked-sd: __mega65_h_name_buf must be a page in $0100-$7E00")

/* The window geometry, in one place. _ram-banked-sd.ld gives every bank slot
 * this length, and prg-to-sd.py reads it out of the ELF, so the three cannot
 * disagree about where a bank starts in the image. */
__bank_window_start = 0x2000;
__bank_window_size = 0x6000;

MEMORY {
    zp             : ORIGIN = __rc31 + 1, LENGTH = __basic_zp_end - (__rc31 + 1)
    ram_low   (rw) : ORIGIN = 0x0300, LENGTH = 0x1d00
    ram       (rw) : ORIGIN = __bank_window_start, LENGTH = __bank_window_size
    ram_fixed (rw) : ORIGIN = 0x8000, LENGTH = 0x5000
    ram_high  (rw) : ORIGIN = 0xe000, LENGTH = 0x1ffa
}

INCLUDE _ram-banked-sd.ld

INPUT(boot.o)
INPUT(load-banks-sd.o)

REGION_ALIAS("c_readonly", ram_fixed)
REGION_ALIAS("c_writeable", ram_high)

SECTIONS {
    /* The image entry point, and the only code that runs before the ROMs are
     * mapped out, so it has to be below $A000. */
    .boot : { KEEP(*(.text.boot)) } >ram

    .bank_0 : { *(.bank_0 .bank_0.*) } >ram

    .ram_low (NOLOAD) : { *(.ram_low .ram_low.*) } >ram_low

    INCLUDE c.ld
}

/* Bytes of ram_fixed that carry content; the rest is soft stack. */
__ram_fixed_size = LOADADDR(.data) + SIZEOF(.data) - ORIGIN(ram_fixed);

/* Soft stack grows down from below the vectors, through ram_high, so it
 * competes with .bss rather than with code. */
PROVIDE(__stack = 0xfffa);

OUTPUT_FORMAT {
    SHORT(0x2000)
    FULL(ram)
    FULL(ram_fixed, 0, __ram_fixed_size)
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
