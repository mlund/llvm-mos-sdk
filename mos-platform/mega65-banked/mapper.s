; Copyright 2026 LLVM-MOS Project
; Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
; See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
; information.

.include "imag.inc"

.zeropage _BANK_SHADOW

; --------------------------------------------------------------------------
; __set_bank_asm — perform MAP/EOM to switch the $2000-$7FFF window.
;
; Input: A = bank_id (0-15)
; Clobbers: A, X, Y, Z
;
; The MEGA65 physical memory has ROM at $20000-$3FFFF, so the bank-to-offset
; mapping must skip that region. A lookup table translates bank_id to the
; MAPLO X register value (selection flags + offset high nibble) and A
; register value (offset low byte).
;
; MAPHI: Y = $00, Z = $83 (keep MEGA65 KERNAL at $E000-$FFFF).
; Z=$83 selects only the $E000-$FFFF block (bit 3) with offset $30000,
; matching the boot MAP state.  Without this, $E000-$FFFF falls through
; to $01/$D030 banking, exposing C64/C65 KERNAL instead of the MEGA65
; KERNAL — breaking any KERNAL jump-table call from banked code.
;
; A two-MAP sequence is used to first set the megabyte byte (bits
; [27:20] of the physical address) from bank_mega_table ($00 for
; chip/fast RAM, $80 for attic RAM). The KERNAL boot and disk I/O
; may use 28-bit MAP operations internally, leaving stale megabyte
; bytes that persist across single-MAP instructions. Without
; explicitly setting them, the 20-bit bank offset would be combined
; with wrong megabyte bytes, mapping $2000-$7FFF to the wrong
; physical address.
;
; Note: MAP disables interrupts until EOM, and EOM clears the I flag.
; We re-assert SEI after EOM to prevent KERNAL IRQs from corrupting
; compiler state.
; --------------------------------------------------------------------------
.section .text.__set_bank_asm,"ax",@progbits
.globl __set_bank_asm
__set_bank_asm:
    tax                     ; X = bank_id (for mega table lookup)
    asl                     ; table index = bank_id * 2
    pha                     ; save index for second MAP

    ; First MAP: set megabyte byte from table.
    ; X=$0F / Z=$0F is the special "set megabyte byte" encoding;
    ; A provides the MAPLO megabyte value ($00 for chip/fast, $80 for attic).
    lda bank_mega_table,x   ; MAPLO megabyte from table
    ldx #$0f
    ldy #$00                ; MAPHI megabyte = $00 (KERNAL at $3E000)
    ldz #$0f
    map

    ; Second MAP: set the actual 20-bit bank offset and selection.
    pla                     ; restore table index
    tay
    lda bank_map_table+1,y  ; MAPLO X: block selection + offset high nibble
    tax
    lda bank_map_table,y    ; MAPLO A: offset low byte
    ldy #$00                ; MAPHI offset low: $00
    ldz #$83                ; MAPHI select $E000-$FFFF, offset high $3 → $30000
    map
    eom
    sei                     ; EOM clears I flag; re-disable to prevent KERNAL IRQs
    ldz #0                  ; restore Z for C code (MAP left it at $83)
    rts

; Bank-to-MAP parameter lookup table.
; Each entry: 2 bytes = { A_value (offset low), X_value (selection|offset high) }
; SYNC: see _ram-banked.ld header for the full list of files encoding bank layout.
;
; Physical memory layout:
;   $00000-$1FFFF  Chip RAM (128KB)
;   $20000-$3FFFF  ROM — NOT WRITABLE
;   $40000-$5FFFF  Fast RAM (128KB)
;   $8000000+      Attic RAM (HyperRAM, 8MB; megabyte byte $80)
;
; Bank → offset → physical base ($2000 + offset):
;   0: $00000 → $02000  (Chip RAM)
;   1: $08000 → $0A000  (Chip RAM)
;   2: $10000 → $12000  (Chip RAM)
;   3: $18000 → $1A000  (Chip RAM — NOTE: extends to $1FFFF, overlaps
;                         colour RAM window at $1F800. See link.ld.)
;   4: $3E000 → $40000  (Fast RAM — skip ROM at $20000)
;   5: $46000 → $48000  (Fast RAM)
;   6: $4E000 → $50000  (Fast RAM)
;   7: $56000 → $58000  (Fast RAM)
;
; Attic RAM banks (megabyte byte $80, 2 per 64KB page at +$0000 and +$6000):
;   8:  $FE000 → $8000000  (Attic RAM)
;   9:  $04000 → $8006000  (Attic RAM)
;   10: $0E000 → $8010000  (Attic RAM)
;   11: $14000 → $8016000  (Attic RAM)
;   12: $1E000 → $8020000  (Attic RAM)
;   13: $24000 → $8026000  (Attic RAM)
;   14: $2E000 → $8030000  (Attic RAM)
;   15: $34000 → $8036000  (Attic RAM)
;
; X register encoding: X[7:4] = $E (select $2000-$7FFF), X[3:0] = offset[19:16]
; A register = offset[15:8]
;
; Bank 0 uses X=$00 (no blocks selected) to unmap, exposing default Chip RAM.
; MAP with selection bits + offset 0 does not reliably override prior mappings
; on the 45GS02.
.section .rodata.bank_map_table,"a",@progbits
bank_map_table:
    .byte $00, $00          ; bank 0: unmap (default Chip RAM at $02000)
    .byte $80, $e0          ; bank 1: offset $08000
    .byte $00, $e1          ; bank 2: offset $10000
    .byte $80, $e1          ; bank 3: offset $18000
    .byte $e0, $e3          ; bank 4: offset $3E000
    .byte $60, $e4          ; bank 5: offset $46000
    .byte $e0, $e4          ; bank 6: offset $4E000
    .byte $60, $e5          ; bank 7: offset $56000
    .byte $e0, $ef          ; bank 8:  offset $FE000 (attic, mega=$80)
    .byte $40, $e0          ; bank 9:  offset $04000 (attic, mega=$80)
    .byte $e0, $e0          ; bank 10: offset $0E000 (attic, mega=$80)
    .byte $40, $e1          ; bank 11: offset $14000 (attic, mega=$80)
    .byte $e0, $e1          ; bank 12: offset $1E000 (attic, mega=$80)
    .byte $40, $e2          ; bank 13: offset $24000 (attic, mega=$80)
    .byte $e0, $e2          ; bank 14: offset $2E000 (attic, mega=$80)
    .byte $40, $e3          ; bank 15: offset $34000 (attic, mega=$80)

; MAPLO megabyte byte lookup table (1 byte per bank).
; Chip/fast RAM banks use $00, attic RAM banks use $80.
.section .rodata.bank_mega_table,"a",@progbits
bank_mega_table:
    .byte $00, $00, $00, $00, $00, $00, $00, $00  ; banks 0-7: chip/fast RAM
    .byte $80, $80, $80, $80, $80, $80, $80, $80  ; banks 8-15: attic RAM

; --------------------------------------------------------------------------
; banked_call — switch bank, call function, restore previous bank.
;
; Calling convention: A = bank_id, __rc2:__rc3 = function pointer.
; This trampoline must reside in fixed code ($8000-$CFFF).
; --------------------------------------------------------------------------
.section .text.banked_call,"ax",@progbits
.weak banked_call
banked_call:
    tay                     ; free A for shadow load
    lda _BANK_SHADOW        ; save current bank for restore after call
    pha
    tya
    sta _BANK_SHADOW
    jsr __set_bank_asm
    lda __rc2               ; __call_indir expects pointer in __rc18:__rc19
    sta __rc18
    lda __rc3
    sta __rc19
    jsr __call_indir
    pla                     ; restore previous bank
    sta _BANK_SHADOW
    jmp __set_bank_asm
