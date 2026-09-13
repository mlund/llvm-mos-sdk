; Copyright 2026 LLVM-MOS Project
; Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
; See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
; information.

; KERNAL LOAD helper for CRT bank loader.
;
; This function resides in ram_fixed ($8000-$9FFF) via .text.kernal_load.
; It must NOT be in ram ($2000-$7FFF) because the C65 ROM's autoboot/CLR
; wipes that region before SYS runs. It must also stay below $A000 because
; the KERNAL ROM state ($01=$37) maps BASIC ROM over $A000-$BFFF.

.include "imag.inc"

SETBNK = $FF6B
SETLFS = $FFBA
SETNAM = $FFBD
KLOAD  = $FFD5
CLRCHN = $FFCC

; __do_kernal_load — load one bank file from D81 via KERNAL.
;
; Uses 28-bit SETBNK mode for all banks (chip, fast, and attic RAM).
; Skips empty banks (size == 0). For non-empty banks, enables interrupts
; for KERNAL I/O, then restores SEI state. BASIC ROM is already unmapped
; ($01=$3E) by load-banks.c before the loading loop. KERNAL routines do
; not require BASIC ROM.
;
; 28-bit SETBNK: when A bit 7 is set, A[3:0] = address bits [27:24],
; Y = address bits [23:16], X = filename bank (legacy, $00).
;
; Input: A = bank index (0-14, for logical banks 1-15)
; Clobbers: A, X, Y, Z, flags
.section .text.kernal_load,"ax",@progbits
.globl __do_kernal_load
__do_kernal_load:
    tax
    lda __bank_used,x
    beq .Lskip

    stx __rc4

    ; KERNAL disk I/O requires interrupts enabled.
    cli

    ; SETBNK 28-bit mode: A bit 7 set, A[3:0] = address[27:24], Y = [23:16].
    ; The tables are indexed by bank, one past this loop's index.
    ldx __rc4
    ldy __bank_addr_mid+1,x
    lda __bank_megabyte+1,x
    lsr a
    lsr a
    lsr a
    lsr a
    ora #$80
    ldx #$00
    jsr SETBNK

    ; SETLFS: SA=0 so LOAD uses our address, not the PRG file header.
    lda #0                      ; LFN
    tay                         ; SA = 0
    ldx #8                      ; device
    jsr SETLFS

    ; All filenames share the "BANK" prefix — only the digit differs.
    ; Indices 0-8 → '1'-'9' ($31+index), 9-14 → 'A'-'F' (skip $3A-$40 gap).
    ldx __rc4
    txa
    clc
    adc #$31                    ; PETSCII '1' + index
    cpx #9
    bcc .Lname_ok
    adc #6                      ; carry set from cpx; +7 skips ':'-'@'
.Lname_ok:
    sta name_buf+4

    ; SETNAM: PETSCII encoding required — KERNAL does exact byte matching.
    lda #5
    ldx #<name_buf
    ldy #>name_buf
    jsr SETNAM

    ; LOAD: A=0 selects load (not verify), X/Y = destination address.
    ldx __rc4
    ldy __bank_addr_page+1,x   ; Y = addr hi
    lda #0                      ; load flag
    tax                         ; X = addr lo (always $00)
    jsr KLOAD
    bcs .Lfail                  ; C set: A holds the KERNAL error code

    ; Reset I/O channels to default (screen/keyboard). Without this,
    ; CHROUT remains directed to the disk device, breaking printf().
    jsr CLRCHN

    ; Point LOAD back at bank 0. A program that never calls SETBNK itself
    ; works on the non-banked target and would otherwise load into whichever
    ; bank was filled last.
    lda #$00                    ; bit 7 clear: legacy form, A = memory bank
    ldx #$00                    ; filename bank
    jsr SETBNK

    ; Restore interrupt disable state and Z register for C code.
    sei
    ldz #0
.Lskip:
    rts

.Lfail:
    sei                         ; the state C runs with on success
    ldz #0
    ldx __rc4
    inx
    txa                         ; A = bank number
    jmp __bank_load_failed

; __clear_map — reset MAP to bank 0 boot state.
;
; KERNAL LOAD uses MAP internally (for 28-bit memory access and its own
; dispatch routines at $1600-$1EFF). After the loading loop, stale MAP
; configuration may remain, which can interfere with subsequent KERNAL
; calls like CHROUT.
;
; This restores the same MAP state as __set_bank_asm(0) in mega65-common/mapper.s:
;   MAPLO: cleared (no blocks selected) — $2000-$7FFF = default chip RAM
;   MAPHI: Z=$83 — $E000-$FFFF mapped to $3E000 (MEGA65 KERNAL ROM)
;
; The MEGA65 KERNAL ROM lives at $3E000, NOT at $2E000 (which is C64/C65
; compatibility ROM). MAPHI must preserve Z=$83 or KERNAL calls will
; execute wrong code and crash.
;
; Clobbers: A, X, Y, Z, flags
.globl __clear_map
__clear_map:
    ; First MAP: clear megabyte bytes to zero.
    ; MAP does not clobber registers, so A=$00 and Y=$00 carry over.
    tza                 ; A = 0 (Z is always 0 on entry from C)
    ldx #$0f            ; special: set megabyte byte mode
    tay                 ; Y = 0
    ldz #$0f
    map
    ; Second MAP: clear MAPLO, keep MEGA65 KERNAL via MAPHI.
    ; A=$00, Y=$00 still valid from above.
    ldx #$00            ; MAPLO: no blocks selected (unmap $2000-$7FFF)
    ldz #$83            ; MAPHI: select $E000-$FFFF, offset $30000
    map
    eom
    ; No SEI here: MAP/EOM inhibit interrupts through a dedicated signal, not
    ; the I flag (gs4510.vhdl, c65_map_instruction / opcode $EA), so the
    ; caller's interrupt state comes back unchanged. See mega65-common/mapper.s.
    ldz #0              ; restore Z for C code
    rts

; PETSCII filename for directory matching. d81.py writes names as
; $41-$5A (standard uppercase), not $C1-$DA (shifted).
name_buf:
    .byte $42, $41, $4E, $4B, $31
