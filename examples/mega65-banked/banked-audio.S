; Copyright 2026 LLVM-MOS Project
; Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
; See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
; information.

; The sample is larger than one bank, so it is split at 24 KB across banks 1
; and 2; banked-audio.cc asserts that is bank 1's size and that the two are
; adjacent. Assembled rather than written as two C arrays because #embed is a
; directive and cannot come from a macro, so C would put the split and its
; offset in the caller's face; here one symbol covers the whole sample.

.section .bank_1.rodata,"aR",@progbits
.globl drums
drums:
        .incbin "drums.s8", 0, 24576

.section .bank_2.rodata,"aR",@progbits
        .incbin "drums.s8", 24576
