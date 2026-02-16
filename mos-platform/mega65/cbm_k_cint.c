// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/// @file
/// MEGA65 override of commodore cbm_k_cint.
/// The KERNAL CINT routine uses taz during screen initialization and
/// leaves Z indeterminate. On 45GS02, Z is the implicit index for
/// (zp),Z indirect addressing, so a stale Z corrupts all subsequent
/// compiler-generated STZ instructions (which assume Z=0).

/// Initialize the screen editor and VIC-IV (CINT, $FF81).
/// Resets screen geometry, cursor position, and character set.
/// @note KERNAL CINT modifies Z; this wrapper restores Z=0.
void cbm_k_cint(void) {
  __attribute__((leaf)) asm volatile("jsr __CINT\n"
                                     "ldz #0" : /* no outputs */
                                     :          /* no inputs */
                                     : "a",
                                     "x", "y", "p");
}
