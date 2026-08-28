/* Bank section layout for mega65-banked-sd.
 *
 * On MOS a symbol's address is the low 16 bits of its LMA, so every bank LMA
 * ends in $2000 and bank symbols resolve into the $2000-$9FFF window whatever
 * bank they belong to. The high bits only have to be unique.
 *
 * The physical addresses these stand for live in mapper.s and mapper.h;
 * test/mega65-banked-sd/check-bank-tables.py recomputes each bank's base from
 * both and fails if they disagree. It needs no emulator, so run it after any
 * edit here.
 *
 * Slot length is __bank_window_size from link.ld, and prg-to-sd.py takes that
 * and __bank_N_size out of the ELF, so nothing here is restated anywhere.
 */

__bank_1_lma   = 0x012000;
__bank_2_lma   = 0x022000;
__bank_3_lma   = 0x032000;
__bank_4_lma   = 0x042000;
__bank_5_lma   = 0x052000;
__bank_6_lma   = 0x062000;
__bank_7_lma   = 0x072000;
__bank_8_lma   = 0x082000;
__bank_9_lma   = 0x092000;
__bank_10_lma  = 0x0A2000;
__bank_11_lma  = 0x0B2000;
__bank_12_lma  = 0x0C2000;
__bank_13_lma  = 0x0D2000;
__bank_14_lma  = 0x0E2000;
__bank_15_lma  = 0x0F2000;

/* Banks past __ram_bank_count collapse to zero length, so nothing is reserved
 * for them and OUTPUT_FORMAT emits nothing. Declare it with MAPPER_BANK_COUNT()
 * from <mapper.h>; without one, all 15 are reserved.
 */
__bank_count = DEFINED(__ram_bank_count) ? __ram_bank_count : 15;

MEMORY {
  bank_1   : ORIGIN = __bank_1_lma,  LENGTH = __bank_count >=  1 ? __bank_window_size : 0
  bank_2   : ORIGIN = __bank_2_lma,  LENGTH = __bank_count >=  2 ? __bank_window_size : 0
  bank_3   : ORIGIN = __bank_3_lma,  LENGTH = __bank_count >=  3 ? __bank_window_size : 0
  bank_4   : ORIGIN = __bank_4_lma,  LENGTH = __bank_count >=  4 ? __bank_window_size : 0
  bank_5   : ORIGIN = __bank_5_lma,  LENGTH = __bank_count >=  5 ? __bank_window_size : 0
  bank_6   : ORIGIN = __bank_6_lma,  LENGTH = __bank_count >=  6 ? __bank_window_size : 0
  bank_7   : ORIGIN = __bank_7_lma,  LENGTH = __bank_count >=  7 ? __bank_window_size : 0
  bank_8   : ORIGIN = __bank_8_lma,  LENGTH = __bank_count >=  8 ? __bank_window_size : 0
  bank_9   : ORIGIN = __bank_9_lma,  LENGTH = __bank_count >=  9 ? __bank_window_size : 0
  bank_10  : ORIGIN = __bank_10_lma, LENGTH = __bank_count >= 10 ? __bank_window_size : 0
  bank_11  : ORIGIN = __bank_11_lma, LENGTH = __bank_count >= 11 ? __bank_window_size : 0
  bank_12  : ORIGIN = __bank_12_lma, LENGTH = __bank_count >= 12 ? __bank_window_size : 0
  bank_13  : ORIGIN = __bank_13_lma, LENGTH = __bank_count >= 13 ? __bank_window_size : 0
  bank_14  : ORIGIN = __bank_14_lma, LENGTH = __bank_count >= 14 ? __bank_window_size : 0
  bank_15  : ORIGIN = __bank_15_lma, LENGTH = __bank_count >= 15 ? __bank_window_size : 0
}

SECTIONS {
  .bank_1  : { *(.bank_1 .bank_1.*) } >bank_1
  .bank_2  : { *(.bank_2 .bank_2.*) } >bank_2
  .bank_3  : { *(.bank_3 .bank_3.*) } >bank_3
  .bank_4  : { *(.bank_4 .bank_4.*) } >bank_4
  .bank_5  : { *(.bank_5 .bank_5.*) } >bank_5
  .bank_6  : { *(.bank_6 .bank_6.*) } >bank_6
  .bank_7  : { *(.bank_7 .bank_7.*) } >bank_7
  .bank_8  : { *(.bank_8 .bank_8.*) } >bank_8
  .bank_9  : { *(.bank_9 .bank_9.*) } >bank_9
  .bank_10 : { *(.bank_10 .bank_10.*) } >bank_10
  .bank_11 : { *(.bank_11 .bank_11.*) } >bank_11
  .bank_12 : { *(.bank_12 .bank_12.*) } >bank_12
  .bank_13 : { *(.bank_13 .bank_13.*) } >bank_13
  .bank_14 : { *(.bank_14 .bank_14.*) } >bank_14
  .bank_15 : { *(.bank_15 .bank_15.*) } >bank_15
}

__bank_1_size    = SIZEOF(.bank_1);
__bank_2_size    = SIZEOF(.bank_2);
__bank_3_size    = SIZEOF(.bank_3);
__bank_4_size    = SIZEOF(.bank_4);
__bank_5_size    = SIZEOF(.bank_5);
__bank_6_size    = SIZEOF(.bank_6);
__bank_7_size    = SIZEOF(.bank_7);
__bank_8_size    = SIZEOF(.bank_8);
__bank_9_size    = SIZEOF(.bank_9);
__bank_10_size   = SIZEOF(.bank_10);
__bank_11_size   = SIZEOF(.bank_11);
__bank_12_size   = SIZEOF(.bank_12);
__bank_13_size   = SIZEOF(.bank_13);
__bank_14_size   = SIZEOF(.bank_14);
__bank_15_size   = SIZEOF(.bank_15);

/* And whether each holds anything, which is all the loader needs.
 *
 * A byte, deliberately.  The size itself is a linker-defined absolute, and
 * llvm-mos relocates a sixteen-bit initialiser built from one as two low-byte
 * relocations rather than a low and a high -- so a table of sizes links only
 * while every bank is under 256 bytes, and fails on the first real one with
 * "R_MOS_ADDR8 out of range".  prg-to-sd.py still reads the sizes out of the
 * ELF, where they are exact.
 */
__bank_1_used    = SIZEOF(.bank_1)  > 0 ? 1 : 0;
__bank_2_used    = SIZEOF(.bank_2)  > 0 ? 1 : 0;
__bank_3_used    = SIZEOF(.bank_3)  > 0 ? 1 : 0;
__bank_4_used    = SIZEOF(.bank_4)  > 0 ? 1 : 0;
__bank_5_used    = SIZEOF(.bank_5)  > 0 ? 1 : 0;
__bank_6_used    = SIZEOF(.bank_6)  > 0 ? 1 : 0;
__bank_7_used    = SIZEOF(.bank_7)  > 0 ? 1 : 0;
__bank_8_used    = SIZEOF(.bank_8)  > 0 ? 1 : 0;
__bank_9_used    = SIZEOF(.bank_9)  > 0 ? 1 : 0;
__bank_10_used   = SIZEOF(.bank_10) > 0 ? 1 : 0;
__bank_11_used   = SIZEOF(.bank_11) > 0 ? 1 : 0;
__bank_12_used   = SIZEOF(.bank_12) > 0 ? 1 : 0;
__bank_13_used   = SIZEOF(.bank_13) > 0 ? 1 : 0;
__bank_14_used   = SIZEOF(.bank_14) > 0 ? 1 : 0;
__bank_15_used   = SIZEOF(.bank_15) > 0 ? 1 : 0;
