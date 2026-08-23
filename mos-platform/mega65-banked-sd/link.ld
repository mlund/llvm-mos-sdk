/* MEGA65 banked PRG linker script, SD-card variant.
 *
 * BASIC and the KERNAL are mapped out for good, so the ROM windows are RAM
 * and the bank window gains a MAPHI block:
 *
 *   $0002-$00FF  imaginary registers, then 222 bytes of compiler zero page
 *   $0100-$01FF  hardware stack
 *   $0200-$02FF  Hyppo filename staging page
 *   $0300-$1FFF  ram_low, uninitialised
 *   $2000-$9FFF  BANKED window (32KB, MAPLO blocks 1-3 + MAPHI block 0)
 *   $A000-$CFFF  FIXED code, rodata and .data initialisers
 *   $D000-$DFFF  I/O
 *   $E000-$FFF9  FIXED .data, .bss, .noinit
 *   $FFFA-$FFFF  interrupt vectors, installed by boot.S
 *
 * $C000-$DFFF is one MAPHI block, so a window reaching past $BFFF would take
 * I/O with it.
 *
 * Physical bank bases (ROM at $20000-$3FFFF is not writable):
 *   Chip RAM: 0: $02000  1: $12000
 *   Fast RAM: 2: $40000  3: $48000  4: $50000  5: $58000
 *   Attic:    6: $8000000 ... 15: $8048000  (HyperRAM, ~10x slower)
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

MEMORY {
    zp             : ORIGIN = __rc31 + 1, LENGTH = __basic_zp_end - (__rc31 + 1)
    ram_low   (rw) : ORIGIN = 0x0300, LENGTH = 0x1d00
    ram       (rw) : ORIGIN = 0x2000, LENGTH = 0x8000
    ram_fixed (rw) : ORIGIN = 0xa000, LENGTH = 0x3000
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

/* Soft stack grows down from the I/O region through the free part of
 * ram_fixed. */
PROVIDE(__stack = 0xd000);

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
