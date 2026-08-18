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
// ram_fixed ($8000-$BFFF) is available as RAM. After loading, ROMC is
// cleared to expose $C000-$CFFF as RAM, extending ram_fixed to 20KB.
//
// KERNAL bug: LOAD corrupts the first ~1-12KB at the destination address
// when load_addr_hi=$00 (Y=0 in the KLOAD call). All bank physical
// addresses use a +$800 offset to ensure non-zero high bytes.

// Registers used below. We avoid #include <mega65.h> because the full header
// chain requires include paths not available to CRT objects, so the names are
// restated here. They mirror VICIV.key, VICIV.ctrla and the VIC3_*/VIC4_*
// masks in <mega65.h>; keep them in step.
#define VIC_KEY   (*(volatile unsigned char *)0xD02F)
#define VIC_CTRLA (*(volatile unsigned char *)0xD030)

// The pair that makes the VIC-IV registers visible at $D000-$DFFF. Any other
// value written to the key returns to VIC-II.
#define VIC_KEY_VICIV_A 0x47
#define VIC_KEY_VICIV_B 0x53

// Bits of VIC_CTRLA. The four ROM bits overlay C65 ROM on the matching 8 KB
// of RAM; leaving them clear is what keeps ram_fixed addressable.
#define VIC3_PAL   0x04 // colours 0-15 from palette RAM rather than ROM
#define VIC3_ROM8  0x08 // C65 ROM over $8000
#define VIC3_ROMA  0x10 // C65 ROM over $A000
#define VIC3_ROMC  0x20 // C65 ROM over $C000
#define VIC3_CROM9 0x40 // C65 character set
#define VIC3_ROME  0x80 // C65 ROM over $E000

// C64-style CPU I/O port at address $01.
#define CPU_PORT  (*(volatile unsigned char *)0x01)
#define CPU_PORT_LORAM    0x01 // BASIC ROM over $A000-$BFFF
#define CPU_PORT_HIRAM    0x02 // KERNAL ROM over $E000-$FFFF
#define CPU_PORT_CHAREN   0x04 // I/O at $D000 rather than the character ROM
#define CPU_PORT_CASSETTE 0x38 // cassette lines, left driven high

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
  CPU_PORT = CPU_PORT_CASSETTE | CPU_PORT_CHAREN | CPU_PORT_HIRAM;

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
  // One pair is enough whatever came before: each write to $D02F stores the
  // byte as the new key regardless (viciv.vhdl), so the $47 sets up the $53
  // no matter what the register last saw.
  VIC_KEY = VIC_KEY_VICIV_A;
  VIC_KEY = VIC_KEY_VICIV_B;
  // Clear ROMC so $C000-$CFFF is RAM rather than the C65 Interface ROM,
  // which is what makes ram_fixed 20 KB. Writing all four ROM bits clear at
  // once also keeps ROM8/ROMA/ROME off. KERNAL CHROUT/printf still work: the
  // 40-column editor lives at $E000-$FFFF, which this does not touch.
  VIC_CTRLA = VIC3_CROM9 | VIC3_PAL;

  // Re-enable interrupts for KERNAL screen I/O. The MEGA65 KERNAL's CHROUT
  // (used by printf/putchar) requires interrupts enabled. The bank loading
  // above runs with them off (see __do_kernal_load), so turn them back on
  // before main(). Bank switching itself leaves the I flag alone.
  asm volatile("cli" ::: "p");
}
