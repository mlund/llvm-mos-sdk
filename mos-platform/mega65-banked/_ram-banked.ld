/* Bank section layout for mega65-banked.
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
 *   - make-d81.sh:        MAIN_SIZE split offset
 *   - link.ld:            MEMORY regions (ram, ram_fixed) and OUTPUT_FORMAT
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

MEMORY {
  bank_1  : ORIGIN = __bank_1_lma,  LENGTH = 0x6000
  bank_2  : ORIGIN = __bank_2_lma,  LENGTH = 0x6000
  bank_3  : ORIGIN = __bank_3_lma,  LENGTH = 0x6000
  bank_4  : ORIGIN = __bank_4_lma,  LENGTH = 0x6000
  bank_5  : ORIGIN = __bank_5_lma,  LENGTH = 0x6000
  bank_6  : ORIGIN = __bank_6_lma,  LENGTH = 0x6000
  bank_7  : ORIGIN = __bank_7_lma,  LENGTH = 0x6000
  bank_8  : ORIGIN = __bank_8_lma,  LENGTH = 0x6000
  bank_9  : ORIGIN = __bank_9_lma,  LENGTH = 0x6000
  bank_10 : ORIGIN = __bank_10_lma, LENGTH = 0x6000
  bank_11 : ORIGIN = __bank_11_lma, LENGTH = 0x6000
  bank_12 : ORIGIN = __bank_12_lma, LENGTH = 0x6000
  bank_13 : ORIGIN = __bank_13_lma, LENGTH = 0x6000
  bank_14 : ORIGIN = __bank_14_lma, LENGTH = 0x6000
  bank_15 : ORIGIN = __bank_15_lma, LENGTH = 0x6000
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
