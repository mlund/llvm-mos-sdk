// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// CRT init module: load non-empty bank files into physical RAM at startup.
//
// Runs at .init.150 — soft stack is initialized but BSS is not yet zeroed.
//
// Each bank is loaded via __do_kernal_load (in load-banks-kernal.S), which
// checks the linker-provided bank size, skips empty banks, and calls KERNAL
// LOAD. Before loading, BASIC ROM is unmapped ($01=$3E, LORAM=0) so
// ram_fixed ($8000-$BFFF, 16KB) is available as RAM. $C000-$CFFF remains
// mapped to C65 Interface ROM (ROMC) which the KERNAL needs for CHROUT.

// VIC-IV registers used below. We avoid #include <mega65.h> because the
// full header chain requires include paths not available to CRT objects.
#define VIC_KEY   (*(volatile unsigned char *)0xD02F)
#define VIC_CTRLA (*(volatile unsigned char *)0xD030)

// C64-style CPU I/O port at address $01.
#define CPU_PORT  (*(volatile unsigned char *)0x01)

// Defined in load-banks-kernal.S. Uses 28-bit SETBNK for all banks
// (chip, fast, and attic RAM). The empty-bank size check lives there
// because the C compiler assumes &extern_symbol != 0, which is wrong for
// linker-defined "address = value" symbols.
extern void __do_kernal_load(unsigned char bank_idx);

// Clear all MAP state after KERNAL LOAD operations.
extern void __clear_map(void);

asm(".section .init.150,\"ax\",@progbits\n"
    "jsr __load_banks\n");

__attribute__((weak)) void __load_banks(void) {
  // Unmap BASIC ROM at $A000-$BFFF. The BASIC SYS bootstrap sets $01=$37
  // (LORAM=1), which maps C64 BASIC ROM there. We only need KERNAL
  // ($E000-$FFFF), not BASIC. $3E clears LORAM while keeping HIRAM and
  // CHAREN (KERNAL + I/O).
  CPU_PORT = 0x3E;

  // When booted via AUTOBOOT.C65, the KERNAL has just finished loading the
  // main PRG and may have left I/O channels in a dirty state.  Reset them
  // before our first LOAD to avoid conflicts.
  asm volatile("cli\n\t"
               "jsr $FFCC\n\t"  // CLRCHN
               "sei" ::: "a", "x", "y", "p");

  // Load all banks (1-15) via 28-bit SETBNK. Works for chip, fast,
  // and attic RAM — KERNAL loads directly to the target address.
  for (unsigned char i = 0; i < 15; ++i)
    __do_kernal_load(i);

  // KERNAL LOAD uses MAP internally for 28-bit memory access. Clear
  // stale MAP state so subsequent KERNAL calls work correctly.
  // Preserves MAPHI Z=$83 for MEGA65 KERNAL at $E000-$FFFF.
  __clear_map();

  // KERNAL disk I/O resets the KEY register ($D02F) to VIC-II mode,
  // locking out VIC-III/IV registers.
  // Unlock VIC-IV so main() has full access to all I/O registers.
  // The KEY sequence ($47,$53) at $D02F promotes VIC-II → VIC-IV directly.
  // Written twice to handle any prior KEY state.
  VIC_KEY = 0x47;
  VIC_KEY = 0x53;
  VIC_KEY = 0x47;
  VIC_KEY = 0x53;
  // Restore $D030 to boot default ($64). Keeps ROMC (bit 5) set so the
  // KERNAL can access C65 Interface ROM at $C000-$CFFF (needed for CHROUT
  // screen editor). Bit 6 preserves C65 character set. Bit 2 = FAST.
  // ram_fixed usable range: $8000-$BFFF (16KB). $C000-$CFFF is Interface ROM.
  VIC_CTRLA = 0x64;

  // Re-enable interrupts for KERNAL screen I/O. The MEGA65 KERNAL's CHROUT
  // (used by printf/putchar) requires interrupts enabled. The CRT init and
  // mapper leave SEI active; restore CLI so KERNAL calls work in main().
  asm volatile("cli" ::: "p");
}
