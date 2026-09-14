/* Bank slots for the banked MEGA65 platforms.
 *
 * On MOS a symbol's address is the low 16 bits of its LMA, so every bank LMA
 * ends in $2000 and bank symbols resolve into the $2000-$7FFF window whatever
 * bank they belong to. The high bits only have to be unique.
 *
 * Each slot is as long as its bank, __bank_N_length below; the converter reads
 * that and __bank_N_size out of the ELF.
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
__bank_16_lma  = 0x102000;
__bank_17_lma  = 0x112000;
__bank_18_lma  = 0x122000;
__bank_19_lma  = 0x132000;
__bank_20_lma  = 0x142000;
__bank_21_lma  = 0x152000;
__bank_22_lma  = 0x162000;
__bank_23_lma  = 0x172000;
__bank_24_lma  = 0x182000;
__bank_25_lma  = 0x192000;
__bank_26_lma  = 0x1A2000;
__bank_27_lma  = 0x1B2000;
__bank_28_lma  = 0x1C2000;
__bank_29_lma  = 0x1D2000;
__bank_30_lma  = 0x1E2000;
__bank_31_lma  = 0x1F2000;

/* <mapper.h> gives MAPPER_BANK_COUNT as one marker per set bit. Banks past it
 * collapse to zero length, so nothing is reserved for them and OUTPUT_FORMAT
 * emits nothing.
 */
__bank_count = (ALIGNOF(.mapper_count_1) == 2 ? 1 : 0) +
               (ALIGNOF(.mapper_count_2) == 2 ? 2 : 0) +
               (ALIGNOF(.mapper_count_4) == 2 ? 4 : 0) +
               (ALIGNOF(.mapper_count_8) == 2 ? 8 : 0) +
               (ALIGNOF(.mapper_count_16) == 2 ? 16 : 0);

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
  bank_16  : ORIGIN = __bank_16_lma, LENGTH = __bank_count >= 16 ? __bank_16_length : 0
  bank_17  : ORIGIN = __bank_17_lma, LENGTH = __bank_count >= 17 ? __bank_17_length : 0
  bank_18  : ORIGIN = __bank_18_lma, LENGTH = __bank_count >= 18 ? __bank_18_length : 0
  bank_19  : ORIGIN = __bank_19_lma, LENGTH = __bank_count >= 19 ? __bank_19_length : 0
  bank_20  : ORIGIN = __bank_20_lma, LENGTH = __bank_count >= 20 ? __bank_20_length : 0
  bank_21  : ORIGIN = __bank_21_lma, LENGTH = __bank_count >= 21 ? __bank_21_length : 0
  bank_22  : ORIGIN = __bank_22_lma, LENGTH = __bank_count >= 22 ? __bank_22_length : 0
  bank_23  : ORIGIN = __bank_23_lma, LENGTH = __bank_count >= 23 ? __bank_23_length : 0
  bank_24  : ORIGIN = __bank_24_lma, LENGTH = __bank_count >= 24 ? __bank_24_length : 0
  bank_25  : ORIGIN = __bank_25_lma, LENGTH = __bank_count >= 25 ? __bank_25_length : 0
  bank_26  : ORIGIN = __bank_26_lma, LENGTH = __bank_count >= 26 ? __bank_26_length : 0
  bank_27  : ORIGIN = __bank_27_lma, LENGTH = __bank_count >= 27 ? __bank_27_length : 0
  bank_28  : ORIGIN = __bank_28_lma, LENGTH = __bank_count >= 28 ? __bank_28_length : 0
  bank_29  : ORIGIN = __bank_29_lma, LENGTH = __bank_count >= 29 ? __bank_29_length : 0
  bank_30  : ORIGIN = __bank_30_lma, LENGTH = __bank_count >= 30 ? __bank_30_length : 0
  bank_31  : ORIGIN = __bank_31_lma, LENGTH = __bank_count >= 31 ? __bank_31_length : 0
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
  .bank_16 : { *(.bank_16 .bank_16.*) } >bank_16
  .bank_17 : { *(.bank_17 .bank_17.*) } >bank_17
  .bank_18 : { *(.bank_18 .bank_18.*) } >bank_18
  .bank_19 : { *(.bank_19 .bank_19.*) } >bank_19
  .bank_20 : { *(.bank_20 .bank_20.*) } >bank_20
  .bank_21 : { *(.bank_21 .bank_21.*) } >bank_21
  .bank_22 : { *(.bank_22 .bank_22.*) } >bank_22
  .bank_23 : { *(.bank_23 .bank_23.*) } >bank_23
  .bank_24 : { *(.bank_24 .bank_24.*) } >bank_24
  .bank_25 : { *(.bank_25 .bank_25.*) } >bank_25
  .bank_26 : { *(.bank_26 .bank_26.*) } >bank_26
  .bank_27 : { *(.bank_27 .bank_27.*) } >bank_27
  .bank_28 : { *(.bank_28 .bank_28.*) } >bank_28
  .bank_29 : { *(.bank_29 .bank_29.*) } >bank_29
  .bank_30 : { *(.bank_30 .bank_30.*) } >bank_30
  .bank_31 : { *(.bank_31 .bank_31.*) } >bank_31
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
__bank_16_size   = SIZEOF(.bank_16);
__bank_17_size   = SIZEOF(.bank_17);
__bank_18_size   = SIZEOF(.bank_18);
__bank_19_size   = SIZEOF(.bank_19);
__bank_20_size   = SIZEOF(.bank_20);
__bank_21_size   = SIZEOF(.bank_21);
__bank_22_size   = SIZEOF(.bank_22);
__bank_23_size   = SIZEOF(.bank_23);
__bank_24_size   = SIZEOF(.bank_24);
__bank_25_size   = SIZEOF(.bank_25);
__bank_26_size   = SIZEOF(.bank_26);
__bank_27_size   = SIZEOF(.bank_27);
__bank_28_size   = SIZEOF(.bank_28);
__bank_29_size   = SIZEOF(.bank_29);
__bank_30_size   = SIZEOF(.bank_30);
__bank_31_size   = SIZEOF(.bank_31);

/* Which banks hold anything, which is all a loader needs, as bank-sizes.s
 * stores it: bank n at bit n % 8 of byte n / 8. */
__bank_used_0 =
    (SIZEOF(.bank_1) > 0) << 1 | (SIZEOF(.bank_2) > 0) << 2 |
    (SIZEOF(.bank_3) > 0) << 3 | (SIZEOF(.bank_4) > 0) << 4 |
    (SIZEOF(.bank_5) > 0) << 5 | (SIZEOF(.bank_6) > 0) << 6 |
    (SIZEOF(.bank_7) > 0) << 7;
__bank_used_1 =
    (SIZEOF(.bank_8) > 0) << 0 | (SIZEOF(.bank_9) > 0) << 1 |
    (SIZEOF(.bank_10) > 0) << 2 | (SIZEOF(.bank_11) > 0) << 3 |
    (SIZEOF(.bank_12) > 0) << 4 | (SIZEOF(.bank_13) > 0) << 5 |
    (SIZEOF(.bank_14) > 0) << 6 | (SIZEOF(.bank_15) > 0) << 7;
__bank_used_2 =
    (SIZEOF(.bank_16) > 0) << 0 | (SIZEOF(.bank_17) > 0) << 1 |
    (SIZEOF(.bank_18) > 0) << 2 | (SIZEOF(.bank_19) > 0) << 3 |
    (SIZEOF(.bank_20) > 0) << 4 | (SIZEOF(.bank_21) > 0) << 5 |
    (SIZEOF(.bank_22) > 0) << 6 | (SIZEOF(.bank_23) > 0) << 7;
__bank_used_3 =
    (SIZEOF(.bank_24) > 0) << 0 | (SIZEOF(.bank_25) > 0) << 1 |
    (SIZEOF(.bank_26) > 0) << 2 | (SIZEOF(.bank_27) > 0) << 3 |
    (SIZEOF(.bank_28) > 0) << 4 | (SIZEOF(.bank_29) > 0) << 5 |
    (SIZEOF(.bank_30) > 0) << 6 | (SIZEOF(.bank_31) > 0) << 7;

SECTIONS {
  /* Per-file layout records and the size and count markers; not in the image. */
  .mapper_layout (INFO) : { KEEP(*(.mapper_layout)) }
  .mapper_window (INFO) : { KEEP(*(.mapper_window)) }
  .mapper_count_1 (INFO) : { KEEP(*(.mapper_count_1)) }
  .mapper_count_2 (INFO) : { KEEP(*(.mapper_count_2)) }
  .mapper_count_4 (INFO) : { KEEP(*(.mapper_count_4)) }
  .mapper_count_8 (INFO) : { KEEP(*(.mapper_count_8)) }
  .mapper_count_16 (INFO) : { KEEP(*(.mapper_count_16)) }
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
  .mapper_bank_16 (INFO) : { KEEP(*(.mapper_bank_16)) }
  .mapper_bank_17 (INFO) : { KEEP(*(.mapper_bank_17)) }
  .mapper_bank_18 (INFO) : { KEEP(*(.mapper_bank_18)) }
  .mapper_bank_19 (INFO) : { KEEP(*(.mapper_bank_19)) }
  .mapper_bank_20 (INFO) : { KEEP(*(.mapper_bank_20)) }
  .mapper_bank_21 (INFO) : { KEEP(*(.mapper_bank_21)) }
  .mapper_bank_22 (INFO) : { KEEP(*(.mapper_bank_22)) }
  .mapper_bank_23 (INFO) : { KEEP(*(.mapper_bank_23)) }
  .mapper_bank_24 (INFO) : { KEEP(*(.mapper_bank_24)) }
  .mapper_bank_25 (INFO) : { KEEP(*(.mapper_bank_25)) }
  .mapper_bank_26 (INFO) : { KEEP(*(.mapper_bank_26)) }
  .mapper_bank_27 (INFO) : { KEEP(*(.mapper_bank_27)) }
  .mapper_bank_28 (INFO) : { KEEP(*(.mapper_bank_28)) }
  .mapper_bank_29 (INFO) : { KEEP(*(.mapper_bank_29)) }
  .mapper_bank_30 (INFO) : { KEEP(*(.mapper_bank_30)) }
  .mapper_bank_31 (INFO) : { KEEP(*(.mapper_bank_31)) }

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
__bank_16_length = ALIGNOF(.mapper_bank_16) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_16) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_16) == 8 ? 0x6000 : __bank_window_size;
__bank_17_length = ALIGNOF(.mapper_bank_17) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_17) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_17) == 8 ? 0x6000 : __bank_window_size;
__bank_18_length = ALIGNOF(.mapper_bank_18) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_18) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_18) == 8 ? 0x6000 : __bank_window_size;
__bank_19_length = ALIGNOF(.mapper_bank_19) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_19) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_19) == 8 ? 0x6000 : __bank_window_size;
__bank_20_length = ALIGNOF(.mapper_bank_20) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_20) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_20) == 8 ? 0x6000 : __bank_window_size;
__bank_21_length = ALIGNOF(.mapper_bank_21) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_21) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_21) == 8 ? 0x6000 : __bank_window_size;
__bank_22_length = ALIGNOF(.mapper_bank_22) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_22) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_22) == 8 ? 0x6000 : __bank_window_size;
__bank_23_length = ALIGNOF(.mapper_bank_23) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_23) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_23) == 8 ? 0x6000 : __bank_window_size;
__bank_24_length = ALIGNOF(.mapper_bank_24) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_24) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_24) == 8 ? 0x6000 : __bank_window_size;
__bank_25_length = ALIGNOF(.mapper_bank_25) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_25) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_25) == 8 ? 0x6000 : __bank_window_size;
__bank_26_length = ALIGNOF(.mapper_bank_26) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_26) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_26) == 8 ? 0x6000 : __bank_window_size;
__bank_27_length = ALIGNOF(.mapper_bank_27) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_27) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_27) == 8 ? 0x6000 : __bank_window_size;
__bank_28_length = ALIGNOF(.mapper_bank_28) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_28) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_28) == 8 ? 0x6000 : __bank_window_size;
__bank_29_length = ALIGNOF(.mapper_bank_29) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_29) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_29) == 8 ? 0x6000 : __bank_window_size;
__bank_30_length = ALIGNOF(.mapper_bank_30) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_30) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_30) == 8 ? 0x6000 : __bank_window_size;
__bank_31_length = ALIGNOF(.mapper_bank_31) == 2 ? 0x2000 : ALIGNOF(.mapper_bank_31) == 4 ? 0x4000 :
                  ALIGNOF(.mapper_bank_31) == 8 ? 0x6000 : __bank_window_size;

__window_tail_start = __bank_window_start +
    MIN(__bank_1_length, MIN(__bank_2_length, MIN(__bank_3_length,
    MIN(__bank_4_length, MIN(__bank_5_length, MIN(__bank_6_length,
    MIN(__bank_7_length, MIN(__bank_8_length, MIN(__bank_9_length,
    MIN(__bank_10_length, MIN(__bank_11_length, MIN(__bank_12_length,
    MIN(__bank_13_length, MIN(__bank_14_length, MIN(__bank_15_length,
    MIN(__bank_16_length, MIN(__bank_17_length, MIN(__bank_18_length,
    MIN(__bank_19_length, MIN(__bank_20_length, MIN(__bank_21_length,
    MIN(__bank_22_length, MIN(__bank_23_length, MIN(__bank_24_length,
    MIN(__bank_25_length, MIN(__bank_26_length, MIN(__bank_27_length,
    MIN(__bank_28_length, MIN(__bank_29_length, MIN(__bank_30_length,
    __bank_31_length))))))))))))))))))))))))))))));

/* banked_call_r and _v switch through a section of their own, after .text; no
 * bank may reference it, since the switch would unmap a caller in a bank. */
SECTIONS {
  .banked_call_r_needs_fixed_caller :
    { *(.banked_call_r_needs_fixed_caller) } >ram_fixed
} INSERT AFTER .text;
NOCROSSREFS_TO(.banked_call_r_needs_fixed_caller
  .bank_0 .bank_1 .bank_2 .bank_3 .bank_4 .bank_5 .bank_6 .bank_7
  .bank_8 .bank_9 .bank_10 .bank_11 .bank_12 .bank_13 .bank_14 .bank_15
  .bank_16 .bank_17 .bank_18 .bank_19 .bank_20 .bank_21 .bank_22 .bank_23
  .bank_24 .bank_25 .bank_26 .bank_27 .bank_28 .bank_29 .bank_30 .bank_31)
