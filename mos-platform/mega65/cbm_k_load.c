// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

/// @file
/// MEGA65 override of commodore cbm_k_load.
/// The KERNAL LOAD routine may set the Z register to a non-zero value
/// (0 or 3 depending on the code path). On 45GS02, Z is the implicit
/// index for (zp),Z indirect addressing, so a stale Z corrupts all
/// subsequent zero-page pointer operations.
/// Delegates to mega65_k_load() which restores Z=0 after the KERNAL call.

unsigned char mega65_k_load(unsigned char flag, void *load_addr,
                            void **end_addr);

/// Load or verify a file (LOAD, $FFD5).
/// Requires cbm_k_setlfs() and cbm_k_setnam() called first.
/// On MEGA65, also call mega65_k_setbnk() to set the data/filename banks.
/// @param flag       0=load, 1=verify.
/// @param load_addr  Destination address (used when SA=0).
/// @return Pointer past the last byte loaded, or error code as pointer.
/// @note KERNAL LOAD modifies Z; this wrapper restores Z=0.
void *cbm_k_load(const unsigned char flag, void *load_addr) {
  void *end_addr;
  mega65_k_load(flag, load_addr, &end_addr);
  return end_addr;
}
