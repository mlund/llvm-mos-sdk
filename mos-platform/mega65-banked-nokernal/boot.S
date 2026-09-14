; Copyright 2026 LLVM-MOS Project
; Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
; See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
; information.

; Image entry point. Runs at $2000, below $A000, because whatever launches the
; PRG still has the BASIC ROM over the fixed region.

.section .text.boot,"ax",@progbits
.globl __boot
__boot:
    sei

    ; MAP first, and before any I/O or high-memory access: the launcher hands
    ; over with $E000-$FFFF mapped to the KERNAL ROM, where writes are
    ; discarded. Two MAPs, because the first clears the megabyte bytes that
    ; the second cannot reach.
    lda #$00
    ldx #$0f
    ldy #$00
    ldz #$0f
    map
    tax
    taz
    map
    eom

    ; A line driven as input floats high, so the direction register decides
    ; whether $01 means anything at all.
    lda #$3f
    sta $00
    ; I/O at $D000; RAM everywhere the ROMs would otherwise sit.
    lda #$35
    sta $01

    ; $D640 traps answer only in the MEGA65 I/O personality.
    lda #$47
    sta $d02f
    lda #$53
    sta $d02f

    ; C65 character set, no ROM overlaid on RAM.
    lda #$44
    sta $d030

    ; The second 128KB holds the C65 ROMs, which this platform does not use.
    ; Writes to it are discarded until the hypervisor lifts the protection, and
    ; Hyppo's own loader reports success either way -- so banks placed there
    ; would silently come up empty. Needs the I/O personality set above.
    lda #$02
    sta $d641
    clv

    ; Sources BASIC left running would fire into vectors nobody has written yet.
    lda #$00
    sta $d01a
    lda #$7f
    sta $dc0d
    sta $dd0d
    lda $dc0d
    lda $dd0d

    ; The vectors are RAM now, so they are ours to fill.
    lda #mos16lo(__default_isr)
    sta $fffa
    sta $fffe
    lda #mos16hi(__default_isr)
    sta $fffb
    sta $ffff

    jmp _start

; Replaceable, so a program that wants interrupts can own the vectors without
; rewriting them at run time.
.section .text.__default_isr,"ax",@progbits
.weak __default_isr
__default_isr:
    rti
