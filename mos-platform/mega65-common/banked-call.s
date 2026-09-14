; Copyright 2026 LLVM-MOS Project
; Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
; See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
; information.

.include "imag.inc"

.zeropage _BANK_SHADOW

; --------------------------------------------------------------------------
; banked_call — switch bank, call function, restore previous bank.
;
; Calling convention: A = bank_id, __rc2:__rc3 = function pointer.
; Must reside in fixed code.
; --------------------------------------------------------------------------
.section .text.banked_call,"ax",@progbits
.weak banked_call
banked_call:
    tay                     ; free A for the shadow load
    lda _BANK_SHADOW
    pha                     ; on the hardware stack, so nesting is safe
    tya
    jsr __set_bank_asm      ; which updates the shadow
    lda __rc2               ; __call_indir wants the pointer in __rc18:__rc19
    sta __rc18
    lda __rc3
    sta __rc19
    jsr __call_indir
    pla
    jmp __set_bank_asm
