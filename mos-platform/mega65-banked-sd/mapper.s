; Copyright 2026 LLVM-MOS Project
; Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
; See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
; information.

.include "imag.inc"

.zeropage _BANK_SHADOW

; --------------------------------------------------------------------------
; __set_bank_asm — map a bank into $2000-$7FFF.
;
; Input: A = bank_id (0-15)
; Clobbers: A, X, Y, Z
;
; MAPLO alone covers the window, so MAPHI selects nothing: $8000-$FFFF stays
; unmapped, which is what keeps the fixed region and the interrupt vectors
; where the linker put them.
;
; The first MAP sets the megabyte bytes, which the second cannot reach: X=$0F
; with Z=$0F is the encoding for that. Without it a stale megabyte byte from
; before would combine with a fresh offset and land somewhere else entirely.
;
; MAP inhibits interrupts, NMI included, until EOM -- through a dedicated
; signal and not the I flag (gs4510.vhdl sets map_interrupt_inhibit; EOM
; clears it; neither touches flag_i). The caller's interrupt state survives
; untouched, so nothing has to be restored. The MEGA65 Book calls MAP
; "similar to SEI", which is wrong here: following it and re-asserting SEI
; after EOM would leave interrupts off for good.
; --------------------------------------------------------------------------
.section .text.__set_bank_asm,"ax",@progbits
.globl __set_bank_asm
__set_bank_asm:
    and #$0f                ; ids past 15 would index off the end of the
                            ; tables and hand junk to MAP, which covers
                            ; $0000-$7FFF and so could move zero page
    tax

    lda bank_offset_lo,x
    pha
    lda bank_maplo_sel,x
    pha

    lda bank_megabyte,x
    ldy #$00                ; MAPHI is unused, so its megabyte byte is too
    ldx #$0f
    ldz #$0f
    map

    pla
    tax
    pla                     ; A = MAPLO offset low byte
    ldy #$00
    ldz #$00                ; MAPHI selects no block
    map
    eom                     ; Z is still $00, which compiled code depends on
    rts

; --------------------------------------------------------------------------
; Bank tables, indexed by bank id. mapper.h holds the same layout as physical
; addresses; check-bank-tables.py recomputes one from the other.
;
; offset = bank base - $2000, taken modulo the megabyte: the addition wraps
; inside it and the megabyte byte is applied afterwards, so bank 13's $FE000
; carry never reaches it.
;
; The MAPLO select nibble $E covers blocks 1-3 ($2000-$7FFF); its low nibble is
; offset[19:16]. Bank 0 selects nothing, which is the only way to hand the
; window back to the unmapped default.
; --------------------------------------------------------------------------
.section .rodata.bank_tables,"a",@progbits
bank_offset_lo:
    .byte $00, $00, $60, $e0, $40, $a0, $00, $60
    .byte $e0, $40, $a0, $00, $60, $e0, $40, $a0
bank_maplo_sel:
    .byte $00, $e1, $e1, $e1, $e2, $e2, $e3, $e3
    .byte $e3, $e4, $e4, $e5, $e5, $ef, $e0, $e0
bank_megabyte:
    .byte $00, $00, $00, $00, $00, $00, $00, $00
    .byte $00, $00, $00, $00, $00, $80, $80, $80

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
    sta _BANK_SHADOW
    jsr __set_bank_asm
    lda __rc2               ; __call_indir wants the pointer in __rc18:__rc19
    sta __rc18
    lda __rc3
    sta __rc19
    jsr __call_indir
    pla
    sta _BANK_SHADOW
    jmp __set_bank_asm
