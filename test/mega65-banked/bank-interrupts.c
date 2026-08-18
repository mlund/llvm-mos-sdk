// Switching banks must leave the caller's interrupt state alone.
//
// MAP inhibits interrupts until EOM through a dedicated signal
// (gs4510.vhdl, c65_map_instruction: map_interrupt_inhibit <= '1', cleared by
// EOM at opcode $EA).  Neither instruction touches flag_i, so a MAP/EOM pair
// returns with the I flag exactly as the caller left it.  The MEGA65 Book
// describes MAP as "similar to SEI" and EOM as "similar to CLI", which is the
// erratum this test exists to pin down: code written to that description
// re-asserts SEI after EOM and silently disables interrupts for good.
//
// Only bit 2 of the status register is recorded.  The rest of P carries
// whatever the last flag-setting instruction left behind, which is not
// something a test should be asserting on.
//
// Exit codes (via xemu $D6CF protocol):
//   0 = ran to completion; the dump says whether the flags were right

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

// $FB-$FE are unallocated by the MEGA65 KERNAL and outside the compiler's
// zero page ($02-$8F).  Zero page is bank-0 chip RAM at the same physical
// address, so these are offsets $FB.. in a -dumpmem image.
#define PROBE ((volatile uint8_t *)0x00FB)

#define FLAG_I 0x04

// Injected at runtime rather than linked into .bank_1, so this test needs
// only the main PRG on the disk -- same approach as bank-call.c.
static void (*const banked_func)(void) = (void (*)(void))0x2000;

static uint8_t interrupt_flag(void) {
  uint8_t status;
  // Nothing between here and the caller's set_bank() can disturb bit 2:
  // only CLI, SEI, PLP and RTI write it.
  asm volatile("php\n\tpla" : "=a"(status));
  return status & FLAG_I;
}

int main(void) {
  // Interrupts on: a bank switch must leave them on.
  asm volatile("cli" ::: "p");
  set_bank(1);
  PROBE[0] = interrupt_flag();

  // Interrupts off: a bank switch must leave them off.  Without this the
  // test would pass against an implementation that unconditionally cleared I.
  asm volatile("sei" ::: "p");
  set_bank(2);
  PROBE[1] = interrupt_flag();

  // Same again through the trampoline, which switches twice per call and is
  // what a real program uses.  Repeated, because a single call could not tell
  // a preserved flag apart from one restored by luck.
  set_bank(1);
  *(volatile uint8_t *)0x2000 = 0x60; // RTS, so the trampoline lands on something
  set_bank(0);
  asm volatile("cli" ::: "p");
  for (uint8_t i = 0; i < 200; ++i)
    banked_call(1, banked_func);
  PROBE[2] = interrupt_flag();

  // Sentinel: tells "ran and recorded 0" apart from "never got here".
  PROBE[3] = 0xA5;

  // Leave the machine as we found it before the exit sequence.
  asm volatile("cli" ::: "p");
  set_bank(0);
  xemu_exit(0);
}
