; Copyright 2026 LLVM-MOS Project
; Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
; See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
; information.

; The default bank loader: every non-empty bank off drive 8, through KERNAL
; LOAD.
;
; KERNAL LOAD needs the C65 interface ROM at $C000-$CFFF, which startup mapped
; out, so each load maps it back in. Everything this reads is read before then,
; and the code itself lies below $C000 in .text.kernal_load.

#include <cbm_kernal.inc>

VIC_KEY   = $D02F
VIC_CTRLA = $D030
VIC3_ROMC = $20

.section .text.kernal_load,"ax",@progbits

.weak __load_banks
__load_banks:
    ldx #1
.Lnext:
    phx
    jsr load_bank
    plx
    inx
    cpx #32
    bne .Lnext
    rts

; Loads bank X if the link put anything in it. Clobbers A, X, Y and flags;
; leaves the Z register 0.
load_bank:
    ; __bank_used holds bank X at bit X % 8 of byte X / 8.
    txa
    and #7
    tay
    lda #1
    cpy #0
    beq .Lmask
.Lshift:
    asl a
    dey
    bne .Lshift
.Lmask:
    pha
    txa
    lsr a
    lsr a
    lsr a
    tay
    pla
    and __bank_used,y
    beq .Lskip
    phx

    ; BANK1-BANKF, then BANK10-BANK1F, in PETSCII.
    ldy #4
    txa
    cmp #16
    bcc .Ldigit
    lda #$31                    ; '1'
    sta name_buf,y
    iny
    txa
    and #15
.Ldigit:
    cmp #10
    bcc .Ldecimal
    adc #6                      ; carry set from cmp; +7 skips ':'-'@'
.Ldecimal:
    adc #$30                    ; carry clear
    sta name_buf,y
    iny
    sty name_len

    lda __bank_addr_page,x
    pha
    ldy __bank_addr_mid,x       ; SETBNK 28-bit form: Y = address[23:16],
    lda __bank_megabyte,x
    lsr a
    lsr a
    lsr a
    lsr a
    ora #$80                    ; A = $80 | address[27:24]
    pha

    lda #VIC3_ROMC
    tsb VIC_CTRLA

    ; Autoboot's LOAD of this program can leave the channels dirty, and
    ; clearing a disk channel needs the ROM just mapped in.
    phy
    jsr CLRCHN
    ply

    pla
    ldx #$00                    ; filename bank
    jsr SETBNK

    lda #0                      ; LFN
    tay                         ; SA 0: LOAD uses our address, not the header
    ldx #8                      ; device
    jsr SETLFS

    lda name_len
    ldx #<name_buf
    ldy #>name_buf
    jsr SETNAM

    pla
    tay                         ; address[15:8]
    lda #0                      ; load, not verify
    tax                         ; address[7:0]
    jsr LOAD
    php                         ; C set: failed

    ; Or CHROUT stays on the disk, and a program that never calls SETBNK
    ; itself loads into this bank.
    jsr CLRCHN
    lda #$00
    tax
    jsr SETBNK

    ; LOAD leaves its own map. Put back what __set_bank_asm(0) sets: no MAPLO
    ; blocks, megabyte bytes zero, MAPHI $83 for the MEGA65 KERNAL at $3E000.
    lda #$00
    tay
    ldx #$0f                    ; megabyte byte mode
    ldz #$0f
    map
    ldx #$00
    ldz #__bank_maphi_sel
    map
    eom
    ldz #$00

    ; Disk I/O drops VIC-IV to VIC-II, which hides VIC_CTRLA.
    lda #$47
    sta VIC_KEY
    lda #$53
    sta VIC_KEY
    lda #VIC3_ROMC
    trb VIC_CTRLA

    plp
    pla                         ; A = bank number; leaves C alone
    bcs .Lfail
.Lskip:
    rts
.Lfail:
    jmp __bank_load_failed

; PETSCII as d81.py writes names: $41-$5A, not the shifted $C1-$DA.
name_buf:
    .byte $42, $41, $4E, $4B, 0, 0
name_len:
    .byte 0
