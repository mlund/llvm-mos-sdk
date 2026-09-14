; Copyright 2026 LLVM-MOS Project
; Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
; See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
; information.

; uint8_t mega65_peek_far(uint32_t address)
;
; [zp],z reads a 28-bit address whatever the memory map says, but wants the
; address in four consecutive base-page bytes, and the ABI hands a lone 32-bit
; argument over as A, X, __rc2, __rc3.

.include "imag.inc"

.section .text.mega65_peek_far,"ax",@progbits
.globl mega65_peek_far
mega65_peek_far:
    sta __rc4
    stx __rc5
    lda __rc2
    sta __rc6
    lda __rc3
    sta __rc7
    lda [__rc4],z               ; compiled code keeps Z at 0
    rts
