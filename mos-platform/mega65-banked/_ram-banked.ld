/* Bank section layout for mega65-banked. Maintained by hand; the physical
 * addresses it must agree with are checked by test-bank-tables.
 *
 * On MOS, symbol addresses (VMA) are the lower 16 bits of the LMA.
 * All bank LMAs use lower-16 = $2000 so symbols resolve to $2000-$7FFF,
 * matching the banked window where the CPU accesses mapped bank content.
 * The upper bits just need to be unique and non-overlapping.
 *
 * (The actual physical RAM addresses used by SETBNK/LOAD are in the
 * lookup tables in load-banks-kernal.S — these LMAs only affect the
 * combined PRG file layout and symbol resolution.)
 *
 * SYNC NOTE: The bank physical address mapping is defined across several
 * files that must stay consistent when the layout changes:
 *   - mapper.s:           bank_map_table (MAP register A/X pairs)
 *   - load-banks-kernal.S: setbnk_a/setbnk_y + load_addr_hi (KERNAL LOAD params)
 *   - _ram-banked.ld:     LMA values (this file)
 *   - link.ld:            MEMORY regions (ram, ram_fixed) and OUTPUT_FORMAT
 *   - mapper.h:           BANK_PHYS_BASE_n
 *
 * The first three are checked against each other by
 * test/mega65-banked/check-bank-tables.py (ctest: test-bank-tables), which
 * recomputes each bank's physical base from every form. It needs no emulator,
 * so run it after any edit here.
 *
 * make-d81.sh no longer hardcodes a split offset: it reads __ram_fixed_size
 * and __bank_N_size out of the ELF.
 */

__bank_1_lma  = 0x012000;
__bank_2_lma  = 0x022000;
__bank_3_lma  = 0x032000;
__bank_4_lma  = 0x042000;
__bank_5_lma  = 0x052000;
__bank_6_lma  = 0x062000;
__bank_7_lma  = 0x072000;
__bank_8_lma  = 0x082000;
__bank_9_lma  = 0x092000;
__bank_10_lma = 0x0A2000;
__bank_11_lma = 0x0B2000;
__bank_12_lma = 0x0C2000;
__bank_13_lma = 0x0D2000;
__bank_14_lma = 0x0E2000;
__bank_15_lma = 0x0F2000;

/* Banks past __ram_bank_count collapse to zero length, so nothing is reserved
 * for them and OUTPUT_FORMAT emits nothing. Declare it with MAPPER_BANK_COUNT()
 * from <mapper.h>; without one, all 15 are reserved as before.
 */
MEMORY {
  bank_1  : ORIGIN = __bank_1_lma,  LENGTH = (DEFINED(__ram_bank_count) ? __ram_bank_count : 15) >= 1 ? 0x6000 : 0
  bank_2  : ORIGIN = __bank_2_lma,  LENGTH = (DEFINED(__ram_bank_count) ? __ram_bank_count : 15) >= 2 ? 0x6000 : 0
  bank_3  : ORIGIN = __bank_3_lma,  LENGTH = (DEFINED(__ram_bank_count) ? __ram_bank_count : 15) >= 3 ? 0x6000 : 0
  bank_4  : ORIGIN = __bank_4_lma,  LENGTH = (DEFINED(__ram_bank_count) ? __ram_bank_count : 15) >= 4 ? 0x6000 : 0
  bank_5  : ORIGIN = __bank_5_lma,  LENGTH = (DEFINED(__ram_bank_count) ? __ram_bank_count : 15) >= 5 ? 0x6000 : 0
  bank_6  : ORIGIN = __bank_6_lma,  LENGTH = (DEFINED(__ram_bank_count) ? __ram_bank_count : 15) >= 6 ? 0x6000 : 0
  bank_7  : ORIGIN = __bank_7_lma,  LENGTH = (DEFINED(__ram_bank_count) ? __ram_bank_count : 15) >= 7 ? 0x6000 : 0
  bank_8  : ORIGIN = __bank_8_lma,  LENGTH = (DEFINED(__ram_bank_count) ? __ram_bank_count : 15) >= 8 ? 0x6000 : 0
  bank_9  : ORIGIN = __bank_9_lma,  LENGTH = (DEFINED(__ram_bank_count) ? __ram_bank_count : 15) >= 9 ? 0x6000 : 0
  bank_10 : ORIGIN = __bank_10_lma, LENGTH = (DEFINED(__ram_bank_count) ? __ram_bank_count : 15) >= 10 ? 0x6000 : 0
  bank_11 : ORIGIN = __bank_11_lma, LENGTH = (DEFINED(__ram_bank_count) ? __ram_bank_count : 15) >= 11 ? 0x6000 : 0
  bank_12 : ORIGIN = __bank_12_lma, LENGTH = (DEFINED(__ram_bank_count) ? __ram_bank_count : 15) >= 12 ? 0x6000 : 0
  bank_13 : ORIGIN = __bank_13_lma, LENGTH = (DEFINED(__ram_bank_count) ? __ram_bank_count : 15) >= 13 ? 0x6000 : 0
  bank_14 : ORIGIN = __bank_14_lma, LENGTH = (DEFINED(__ram_bank_count) ? __ram_bank_count : 15) >= 14 ? 0x6000 : 0
  bank_15 : ORIGIN = __bank_15_lma, LENGTH = (DEFINED(__ram_bank_count) ? __ram_bank_count : 15) >= 15 ? 0x6000 : 0
}

SECTIONS {
  .bank_1  : { *(.bank_1  .bank_1.*)  } >bank_1
  .bank_2  : { *(.bank_2  .bank_2.*)  } >bank_2
  .bank_3  : { *(.bank_3  .bank_3.*)  } >bank_3
  .bank_4  : { *(.bank_4  .bank_4.*)  } >bank_4
  .bank_5  : { *(.bank_5  .bank_5.*)  } >bank_5
  .bank_6  : { *(.bank_6  .bank_6.*)  } >bank_6
  .bank_7  : { *(.bank_7  .bank_7.*)  } >bank_7
  .bank_8  : { *(.bank_8  .bank_8.*)  } >bank_8
  .bank_9  : { *(.bank_9  .bank_9.*)  } >bank_9
  .bank_10 : { *(.bank_10 .bank_10.*) } >bank_10
  .bank_11 : { *(.bank_11 .bank_11.*) } >bank_11
  .bank_12 : { *(.bank_12 .bank_12.*) } >bank_12
  .bank_13 : { *(.bank_13 .bank_13.*) } >bank_13
  .bank_14 : { *(.bank_14 .bank_14.*) } >bank_14
  .bank_15 : { *(.bank_15 .bank_15.*) } >bank_15
}

__bank_1_size  = SIZEOF(.bank_1);
__bank_2_size  = SIZEOF(.bank_2);
__bank_3_size  = SIZEOF(.bank_3);
__bank_4_size  = SIZEOF(.bank_4);
__bank_5_size  = SIZEOF(.bank_5);
__bank_6_size  = SIZEOF(.bank_6);
__bank_7_size  = SIZEOF(.bank_7);
__bank_8_size  = SIZEOF(.bank_8);
__bank_9_size  = SIZEOF(.bank_9);
__bank_10_size = SIZEOF(.bank_10);
__bank_11_size = SIZEOF(.bank_11);
__bank_12_size = SIZEOF(.bank_12);
__bank_13_size = SIZEOF(.bank_13);
__bank_14_size = SIZEOF(.bank_14);
__bank_15_size = SIZEOF(.bank_15);
