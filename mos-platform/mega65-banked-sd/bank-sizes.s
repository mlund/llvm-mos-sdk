; Copyright 2026 LLVM-MOS Project
; Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
; See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
; information.

; Which banks hold anything, as data rather than as the linker symbols it comes
; from: C takes the address of an extern to be non-null, which is wrong for a
; symbol the linker defines as a plain value.

; One byte a bank, and a flag rather than a size.  `mos16hi` of a
; linker-defined absolute assembles to a *low* byte relocation, so a table of
; sizes emits R_MOS_ADDR8 for both halves of every entry and the link fails on
; the first bank over 255 bytes with "relocation R_MOS_ADDR8 out of range".
; The loader only asks whether a bank is empty, and prg-to-sd.py reads the real
; sizes out of the ELF, so nothing wants the sixteen-bit form.

.section .rodata.bank_sizes,"a",@progbits
.globl __bank_used
__bank_used:
    .byte __bank_1_used,  __bank_2_used,  __bank_3_used,  __bank_4_used
    .byte __bank_5_used,  __bank_6_used,  __bank_7_used,  __bank_8_used
    .byte __bank_9_used,  __bank_10_used, __bank_11_used, __bank_12_used
    .byte __bank_13_used, __bank_14_used, __bank_15_used
