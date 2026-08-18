// Shared xemu test utilities for MEGA65 tests.
// Uses the xemu $D6CF control register to signal test exit codes.

#ifndef XEMU_TEST_H
#define XEMU_TEST_H

#include <mega65.h>
#include <stdint.h>

#define XEMU_CONTROL (*(volatile uint8_t *)0xd6cf)
#define XEMU_QUIT 0x42

static void xemu_exit(uint8_t code) {
  // Workaround: xemu's apply_cpu_memory_policy() mis-routes $D6CF writes
  // when any MAP block is selected (even $E000-$FFFF only).  Clear MAP
  // entirely so $D000-$DFFF falls through to normal I/O routing.
  asm volatile("lda #$00\n\t"
               "ldx #$00\n\t"
               "ldy #$00\n\t"
               "ldz #$00\n\t"
               "map\n\t"
               "eom\n\t"
               "sei" ::: "a", "x", "y", "p");
  // One pair suffices whatever came before: each write stores its byte as the
  // new key, so the first sets up the second.
  VICIV.key = VIC4_KEY_VICIV_A;
  VICIV.key = VIC4_KEY_VICIV_B;
  XEMU_CONTROL = code;
  XEMU_CONTROL = XEMU_QUIT;
  // Wait for the emulator to go. An empty loop would be undefined behaviour
  // and may be deleted, letting control run off the end of this function.
  for (;;)
    asm volatile("");
}

// Assert condition; on failure exits with source line number as exit code.
#define xemu_assert(cond)                                                      \
  do {                                                                         \
    if (!(cond))                                                               \
      xemu_exit((uint8_t)__LINE__);                                            \
  } while (0)

#endif // XEMU_TEST_H
