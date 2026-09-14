; Copyright 2026 LLVM-MOS Project
; Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
; See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
; information.

; void mega65_poke_far(uint32_t address, uint8_t value)
;
; As mega65_peek_far: [zp],z wants the address in four consecutive base-page
; bytes, and the ABI hands it over as A, X, __rc2, __rc3, with the value
; after it in __rc4.

.include "imag.inc"

.section .text.mega65_poke_far,"ax",@progbits
.globl mega65_poke_far
mega65_poke_far:
    sta __rc5
    stx __rc6
    lda __rc2
    sta __rc7
    lda __rc3
    sta __rc8
    lda __rc4
    sta [__rc5],z               ; compiled code keeps Z at 0
    rts
