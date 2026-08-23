; Copyright 2026 LLVM-MOS Project
; Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
; See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
; information.

; Bank sizes as data rather than as the linker symbols they come from: C takes
; the address of an extern to be non-null, which is wrong for a symbol the
; linker defines as a plain value.

.section .rodata.bank_sizes,"a",@progbits
.globl __bank_sizes
__bank_sizes:
    .byte mos16lo(__bank_1_size), mos16hi(__bank_1_size)
    .byte mos16lo(__bank_2_size), mos16hi(__bank_2_size)
    .byte mos16lo(__bank_3_size), mos16hi(__bank_3_size)
    .byte mos16lo(__bank_4_size), mos16hi(__bank_4_size)
    .byte mos16lo(__bank_5_size), mos16hi(__bank_5_size)
    .byte mos16lo(__bank_6_size), mos16hi(__bank_6_size)
    .byte mos16lo(__bank_7_size), mos16hi(__bank_7_size)
    .byte mos16lo(__bank_8_size), mos16hi(__bank_8_size)
    .byte mos16lo(__bank_9_size), mos16hi(__bank_9_size)
    .byte mos16lo(__bank_10_size), mos16hi(__bank_10_size)
    .byte mos16lo(__bank_11_size), mos16hi(__bank_11_size)
    .byte mos16lo(__bank_12_size), mos16hi(__bank_12_size)
    .byte mos16lo(__bank_13_size), mos16hi(__bank_13_size)
    .byte mos16lo(__bank_14_size), mos16hi(__bank_14_size)
    .byte mos16lo(__bank_15_size), mos16hi(__bank_15_size)
