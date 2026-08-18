; Copyright 2026 LLVM-MOS Project
; Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
; See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
; information.

; The sample is larger than one 24 KB bank, so it is split across banks 1 and
; 2, whose physical addresses are adjacent ($12000 and $18000). Assembled
; rather than written as two C arrays because #embed is a directive and cannot
; come from a macro, so C would put the split and its offset in the caller's
; face; here one symbol covers the whole sample.

.section .bank_1.rodata,"aR",@progbits
.globl drums
drums:
        .incbin "drums.s8", 0, 24576

.section .bank_2.rodata,"aR",@progbits
        .incbin "drums.s8", 24576
