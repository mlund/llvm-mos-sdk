; Copyright 2026 LLVM-MOS Project
; Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
; See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
; information.

.include "imag.inc"

; __set_bank_asm — map a bank into $2000-$7FFF.
;
; Input: A = bank_id; above MAPPER_BANK_COUNT maps bank 0
; Clobbers: A, X, Y, Z
;
; The first MAP sets the megabyte bytes, which the second cannot reach: X=$0F
; with Z=$0F is the encoding for that. Without it a stale megabyte byte, such as
; one the KERNAL's own 28-bit MAPs leave behind, would combine with a fresh
; offset and land somewhere else entirely.
;
; A MAP writes MAPHI too, so the platform's link.ld supplies it as
; __bank_maphi_sel.
;
; MAP inhibits interrupts, NMI included, until EOM -- through a dedicated
; signal and not the I flag (gs4510.vhdl sets map_interrupt_inhibit; EOM
; clears it; neither touches flag_i). The caller's interrupt state survives
; untouched, so nothing has to be restored. The MEGA65 Book calls MAP
; "similar to SEI", which is wrong here: following it and re-asserting SEI
; after EOM would leave interrupts off for good.
.zeropage _BANK_SHADOW

.section .text.__set_bank_asm,"ax",@progbits
.globl __set_bank_asm
__set_bank_asm:
    cmp #__bank_count+1     ; past MAPPER_BANK_COUNT the tables have ended,
    bcc .Lin_range          ; and junk handed to MAP, which covers
    lda #0                  ; $0000-$7FFF, could move zero page: map bank 0
.Lin_range:
    sta _BANK_SHADOW        ; the bank mapped, so get_bank() agrees with MAP
    tax

    lda __bank_offset_lo,x
    pha
    lda __bank_maplo_sel,x
    pha

    lda __bank_megabyte,x
    ldy #$00                ; MAPHI megabyte, and MAPHI offset for the
                            ; second MAP: chip RAM on both platforms
    ldx #$0f
    ldz #$0f
    map

    pla
    tax
    pla                     ; A = MAPLO offset low byte
    ldz #__bank_maphi_sel
    map
    eom
    ldz #$00                ; compiled code depends on Z = 0
    rts
