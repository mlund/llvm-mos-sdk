; Copyright 2026 LLVM-MOS Project
; Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
; See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
; information.

; Bank tables, indexed by bank id: offset[15:8]; the MAPLO select nibble $E
; (blocks 1-3) with offset[19:16]; and the megabyte byte. offset = base - $2000
; modulo the megabyte, so the carry of an offset like $FE000 never reaches it.
; Bank 0 selects nothing, which is the only way to hand the window back to its
; own RAM.
;
; Banks 3-15 sit +$800 into a 64 KB page: KERNAL LOAD corrupts a destination
; whose address high byte is $00.
;
; SYNC: bank-tables.s, mapper.h and load-banks-kernal.S; check-bank-tables.py compares them.
.section .rodata.bank_tables,"a",@progbits
.globl __bank_offset_lo, __bank_maplo_sel, __bank_megabyte
__bank_offset_lo:
    .byte $00, $00, $60, $e8, $48, $a8, $08, $68
    .byte $e8, $48, $e8, $48, $e8, $48, $e8, $48
__bank_maplo_sel:
    .byte $00, $e1, $e1, $e3, $e4, $e4, $e5, $e5
    .byte $ef, $e0, $e0, $e1, $e1, $e2, $e2, $e3
__bank_megabyte:
    .byte $00, $00, $00, $00, $00, $00, $00, $00
    .byte $80, $80, $80, $80, $80, $80, $80, $80
