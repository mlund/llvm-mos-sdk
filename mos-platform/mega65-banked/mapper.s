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
; Note: MAP inhibits all interrupts, NMI included, until EOM -- through a
; dedicated signal and not the I flag. gs4510.vhdl's c65_map_instruction sets
; map_interrupt_inhibit, EOM (opcode $EA) clears it, and neither touches
; flag_i. The caller's interrupt state therefore survives the sequence
; untouched, so nothing has to be restored afterwards.
;
; (The MEGA65 Book calls MAP "similar to SEI" and EOM "similar to CLI". That
; is wrong for IRQ: an SEI before the MAP still holds after the EOM. Following
; the Book here and re-asserting SEI would leave interrupts off for good.)
; --------------------------------------------------------------------------
.section .text.__set_bank_asm,"ax",@progbits
.globl __set_bank_asm
__set_bank_asm:
    and #$0f                ; ids past 15 would index off the end of both
                            ; tables and hand junk to MAP; MAPLO covers
                            ; $0000-$7FFF, so that reaches zero page too
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
    ldz #0                  ; restore Z for C code (MAP left it at $83)
    rts

; Bank-to-MAP parameter lookup table.
; Each entry: 2 bytes = { A_value (offset low), X_value (selection|offset high) }
; SYNC: see _ram-banked.ld header for the full list of files encoding bank layout.
;
; Physical memory layout:
;   $00000-$0FFFF  Chip RAM first 64KB (bank 0 + ram_fixed + ROM/IO)
;   $10000-$11FFF  Chip RAM — C65 DOS work area, mapped whenever CBDOS runs
;   $12000-$1FFFF  Chip RAM (banks 1-2; $1F800 = colour RAM window)
;   $20000-$3FFFF  ROM — NOT WRITABLE
;   $40000-$5FFFF  Fast RAM (128KB, banks 3-7 with $6000 spacing)
;   $8000000+      Attic RAM (HyperRAM, 8MB; megabyte byte $80)
;
; All banks offset +$800 from 64KB page boundaries to avoid a KERNAL LOAD
; bug that corrupts the first 1-12KB at addresses with load_addr_hi=$00.
;
; Bank → offset → physical base ($2000 + offset):
;   0: $00000 → $02000  (Chip RAM — default, no MAP needed)
;   1: $10000 → $12000  (Chip RAM — clear of the C65 DOS work area below)
;   2: $16000 → $18000  (Chip RAM — ends at $1DFFF, below colour RAM at $1F800)
;   3: $3E800 → $40800  (Fast RAM — skip ROM at $20000)
;   4: $44800 → $46800  (Fast RAM)
;   5: $4A800 → $4C800  (Fast RAM)
;   6: $50800 → $52800  (Fast RAM)
;   7: $56800 → $58800  (Fast RAM — ends at $5E7FF)
;
; Attic RAM banks (megabyte byte $80, 2 per 64KB page at +$0800 and +$6800):
;   8:  $FE800 → $8000800  (Attic RAM)
;   9:  $04800 → $8006800  (Attic RAM)
;   10: $0E800 → $8010800  (Attic RAM)
;   11: $14800 → $8016800  (Attic RAM)
;   12: $1E800 → $8020800  (Attic RAM)
;   13: $24800 → $8026800  (Attic RAM)
;   14: $2E800 → $8030800  (Attic RAM)
;   15: $34800 → $8036800  (Attic RAM)
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
    .byte $00, $e1          ; bank 1: offset $10000 → physical $12000
    .byte $60, $e1          ; bank 2: offset $16000 → physical $18000
    .byte $e8, $e3          ; bank 3: offset $3E800 → physical $40800
    .byte $48, $e4          ; bank 4: offset $44800 → physical $46800
    .byte $a8, $e4          ; bank 5: offset $4A800 → physical $4C800
    .byte $08, $e5          ; bank 6: offset $50800 → physical $52800
    .byte $68, $e5          ; bank 7: offset $56800 → physical $58800
    ; Bank 8 is the only entry that relies on the offset addition wrapping:
    ; $2000 + $FE800 = $100800, truncated to 20 bits gives $00800. The
    ; megabyte byte is applied after that, so the carry does not reach it.
    .byte $e8, $ef          ; bank 8:  offset $FE800 (attic, mega=$80)
    .byte $48, $e0          ; bank 9:  offset $04800 (attic, mega=$80)
    .byte $e8, $e0          ; bank 10: offset $0E800 (attic, mega=$80)
    .byte $48, $e1          ; bank 11: offset $14800 (attic, mega=$80)
    .byte $e8, $e1          ; bank 12: offset $1E800 (attic, mega=$80)
    .byte $48, $e2          ; bank 13: offset $24800 (attic, mega=$80)
    .byte $e8, $e2          ; bank 14: offset $2E800 (attic, mega=$80)
    .byte $48, $e3          ; bank 15: offset $34800 (attic, mega=$80)

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
