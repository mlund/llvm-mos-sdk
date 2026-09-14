/* Bank slots for the banked MEGA65 platforms.
 *
 * On MOS a symbol's address is the low 16 bits of its LMA, so every bank LMA
 * ends in $2000 and bank symbols resolve into the $2000-$7FFF window whatever
 * bank they belong to. The high bits only have to be unique.
 *
 * Slot length is __bank_window_size, below; the converters read it and
 * __bank_N_size out of the ELF.
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
  bank_1   : ORIGIN = __bank_1_lma,  LENGTH = __bank_count >=  1 ? __bank_1_length : 0
  bank_2   : ORIGIN = __bank_2_lma,  LENGTH = __bank_count >=  2 ? __bank_2_length : 0
  bank_3   : ORIGIN = __bank_3_lma,  LENGTH = __bank_count >=  3 ? __bank_3_length : 0
  bank_4   : ORIGIN = __bank_4_lma,  LENGTH = __bank_count >=  4 ? __bank_4_length : 0
  bank_5   : ORIGIN = __bank_5_lma,  LENGTH = __bank_count >=  5 ? __bank_5_length : 0
  bank_6   : ORIGIN = __bank_6_lma,  LENGTH = __bank_count >=  6 ? __bank_6_length : 0
  bank_7   : ORIGIN = __bank_7_lma,  LENGTH = __bank_count >=  7 ? __bank_7_length : 0
  bank_8   : ORIGIN = __bank_8_lma,  LENGTH = __bank_count >=  8 ? __bank_8_length : 0
  bank_9   : ORIGIN = __bank_9_lma,  LENGTH = __bank_count >=  9 ? __bank_9_length : 0
  bank_10  : ORIGIN = __bank_10_lma, LENGTH = __bank_count >= 10 ? __bank_10_length : 0
  bank_11  : ORIGIN = __bank_11_lma, LENGTH = __bank_count >= 11 ? __bank_11_length : 0
  bank_12  : ORIGIN = __bank_12_lma, LENGTH = __bank_count >= 12 ? __bank_12_length : 0
  bank_13  : ORIGIN = __bank_13_lma, LENGTH = __bank_count >= 13 ? __bank_13_length : 0
  bank_14  : ORIGIN = __bank_14_lma, LENGTH = __bank_count >= 14 ? __bank_14_length : 0
  bank_15  : ORIGIN = __bank_15_lma, LENGTH = __bank_count >= 15 ? __bank_15_length : 0
  /* Above the smallest bank, up to the window's top; empty without one. */
  window_tail : ORIGIN = __window_tail_start,
                LENGTH = __bank_window_start + __bank_window_size - __window_tail_start
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

/* And whether each holds anything, which is all a loader needs. */
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

SECTIONS {
  /* Per-file layout records and the window-size marker; not in the image. */
  .mapper_layout (INFO) : { KEEP(*(.mapper_layout)) }
  .mapper_window (INFO) : { KEEP(*(.mapper_window)) }
  .mapper_bank_1  (INFO) : { KEEP(*(.mapper_bank_1)) }
  .mapper_bank_2  (INFO) : { KEEP(*(.mapper_bank_2)) }
  .mapper_bank_3  (INFO) : { KEEP(*(.mapper_bank_3)) }
  .mapper_bank_4  (INFO) : { KEEP(*(.mapper_bank_4)) }
  .mapper_bank_5  (INFO) : { KEEP(*(.mapper_bank_5)) }
  .mapper_bank_6  (INFO) : { KEEP(*(.mapper_bank_6)) }
  .mapper_bank_7  (INFO) : { KEEP(*(.mapper_bank_7)) }
  .mapper_bank_8  (INFO) : { KEEP(*(.mapper_bank_8)) }
  .mapper_bank_9  (INFO) : { KEEP(*(.mapper_bank_9)) }
  .mapper_bank_10 (INFO) : { KEEP(*(.mapper_bank_10)) }
  .mapper_bank_11 (INFO) : { KEEP(*(.mapper_bank_11)) }
  .mapper_bank_12 (INFO) : { KEEP(*(.mapper_bank_12)) }
  .mapper_bank_13 (INFO) : { KEEP(*(.mapper_bank_13)) }
  .mapper_bank_14 (INFO) : { KEEP(*(.mapper_bank_14)) }
  .mapper_bank_15 (INFO) : { KEEP(*(.mapper_bank_15)) }

  .window_tail (NOLOAD) : { *(.window_tail .window_tail.*) } >window_tail
}

/* <mapper.h> encodes MAPPER_WINDOW_KB as its marker's alignment: 8 KB as 2,
 * 16 KB as 4. No marker means the 24 KB default. */
__bank_window_start = 0x2000;
__bank_window_size = ALIGNOF(.mapper_window) == 2 ? 0x2000 :
                     ALIGNOF(.mapper_window) == 4 ? 0x4000 : 0x6000;

/* Each bank's own size, off its marker; the window's without one. */
__bank_1_length = ALIGNOF(.mapper_bank_1) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_1) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_1) == 8 ? 0x6000 : __bank_window_size;
__bank_2_length = ALIGNOF(.mapper_bank_2) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_2) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_2) == 8 ? 0x6000 : __bank_window_size;
__bank_3_length = ALIGNOF(.mapper_bank_3) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_3) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_3) == 8 ? 0x6000 : __bank_window_size;
__bank_4_length = ALIGNOF(.mapper_bank_4) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_4) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_4) == 8 ? 0x6000 : __bank_window_size;
__bank_5_length = ALIGNOF(.mapper_bank_5) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_5) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_5) == 8 ? 0x6000 : __bank_window_size;
__bank_6_length = ALIGNOF(.mapper_bank_6) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_6) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_6) == 8 ? 0x6000 : __bank_window_size;
__bank_7_length = ALIGNOF(.mapper_bank_7) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_7) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_7) == 8 ? 0x6000 : __bank_window_size;
__bank_8_length = ALIGNOF(.mapper_bank_8) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_8) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_8) == 8 ? 0x6000 : __bank_window_size;
__bank_9_length = ALIGNOF(.mapper_bank_9) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_9) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_9) == 8 ? 0x6000 : __bank_window_size;
__bank_10_length = ALIGNOF(.mapper_bank_10) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_10) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_10) == 8 ? 0x6000 : __bank_window_size;
__bank_11_length = ALIGNOF(.mapper_bank_11) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_11) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_11) == 8 ? 0x6000 : __bank_window_size;
__bank_12_length = ALIGNOF(.mapper_bank_12) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_12) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_12) == 8 ? 0x6000 : __bank_window_size;
__bank_13_length = ALIGNOF(.mapper_bank_13) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_13) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_13) == 8 ? 0x6000 : __bank_window_size;
__bank_14_length = ALIGNOF(.mapper_bank_14) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_14) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_14) == 8 ? 0x6000 : __bank_window_size;
__bank_15_length = ALIGNOF(.mapper_bank_15) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_15) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_15) == 8 ? 0x6000 : __bank_window_size;

__window_tail_start = __bank_window_start +
    MIN(__bank_1_length, MIN(__bank_2_length, MIN(__bank_3_length, MIN(__bank_4_length, MIN(__bank_5_length, MIN(__bank_6_length, MIN(__bank_7_length, MIN(__bank_8_length, MIN(__bank_9_length, MIN(__bank_10_length, MIN(__bank_11_length, MIN(__bank_12_length, MIN(__bank_13_length, MIN(__bank_14_length, __bank_15_length))))))))))))));
