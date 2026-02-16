; Copyright 2025 LLVM-MOS Project
; Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
; See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
; information.

; Call a subroutine in another MEGA65 bank via KERNAL JSRFAR ($FF6E).
;
; C prototype:
;   unsigned char mega65_k_jsrfar(
;       unsigned char bank,    // A        -> $02 (RC0)
;       unsigned int  addr,    // X:RC2    -> $04:$03 (swap needed)
;       unsigned char s_reg,   // RC3=$05  (already there)
;       unsigned char a_reg,   // RC4=$06  (already there)
;       unsigned char x_reg,   // RC5=$07  (already there)
;       unsigned char y_reg,   // RC6=$08  (already there)
;       unsigned char z_reg    // RC7=$09  (already there)
;   );
;
; Returns the called routine's A register value.
;
; llvm-mos C calling convention: params in A, X, RC2, RC3, RC4, RC5, RC6, RC7.
; JSRFAR expects parameters at ZP $02-$09 (bank, pc_hi, pc_lo, s, a, x, y, z).
; uint16_t addr arrives as lo=X, hi=RC2; JSRFAR wants hi=$03, lo=$04.
; RC3-RC7 map to $05-$09, so params 3-7 land exactly where JSRFAR needs them.
; RC0:RC1 are callee-saved; RC2-RC7 are caller-saved.

.text
.globl mega65_k_jsrfar
.section .text.mega65_k_jsrfar,"ax",@progbits
mega65_k_jsrfar:
  ; Save callee-saved RC0:RC1 ($02:$03) via Y
  ldy __rc0
  phy
  ldy __rc1
  phy

  ; Route bank (A) → $02, addr_hi (RC2) → $03, addr_lo (X) → $04
  sta __rc0               ; $02 = bank
  ldy __rc2               ; Y = addr_hi
  stx __rc2               ; $04 = addr_lo (pc_lo)
  sty __rc1               ; $03 = addr_hi (pc_hi)
  ; RC3-RC7 ($05-$09) already correct from calling convention

  jsr __JSRFAR            ; KERNAL JSRFAR

  ; A = called routine's return A value
  ldz #0                  ; restore Z=0 (compiler invariant)

  ; Restore callee-saved RC0:RC1
  ply
  sty __rc1
  ply
  sty __rc0
  ; A still has the called routine's return value
  rts
