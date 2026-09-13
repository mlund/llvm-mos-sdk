; Copyright 2026 LLVM-MOS Project
; Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
; See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
; information.

; Which banks hold anything, as data rather than as the linker symbols it comes
; from: C takes the address of an extern to be non-null, which is wrong for a
; symbol the linker defines as a plain value. A flag rather than a size, since
; the loader only asks whether a bank is empty.

.section .rodata.bank_sizes,"a",@progbits
.globl __bank_used
__bank_used:
    .byte __bank_1_used,  __bank_2_used,  __bank_3_used,  __bank_4_used
    .byte __bank_5_used,  __bank_6_used,  __bank_7_used,  __bank_8_used
    .byte __bank_9_used,  __bank_10_used, __bank_11_used, __bank_12_used
    .byte __bank_13_used, __bank_14_used, __bank_15_used
