; Copyright 2026 LLVM-MOS Project
; Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
; See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
; information.

; Which banks hold anything, as data rather than as the linker symbols it comes
; from: C takes the address of an extern to be non-null, which is wrong for a
; symbol the linker defines as a plain value. One bit per bank, bank n at bit
; n % 8 of byte n / 8, since the loader only asks whether a bank is empty.

.section .rodata.bank_sizes,"a",@progbits
.globl __bank_used
__bank_used:
    .byte __bank_used_0, __bank_used_1, __bank_used_2, __bank_used_3
