// banked_call must nest, and get_bank() must tell the truth at every depth.
//
// The trampoline pushes the previous shadow on the hardware stack before
// switching (mapper.s, banked_call), so nesting is structurally safe -- but
// nothing exercised it, and get_bank() had no coverage anywhere in the repo.
//
// The call graph is the shape banked code is supposed to use: a banked
// function never calls into another bank directly, it returns to fixed code
// which makes the next banked_call. Bank 1 is unmapped while bank 2 runs, so
// outer()'s own code is absent from the window for the duration -- it is only
// safe because banked_call restores bank 1 before returning into it.
//
//   main (fixed) -> outer (bank 1) -> middle (fixed) -> inner (bank 2)
//
// Exit codes (via xemu $D6CF protocol):
//   0 = ran to completion; the dump carries the bank seen at each step

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

MAPPER_BANK_COUNT(2);

// In ram_fixed, which is never remapped, so it reads back the same whichever
// bank is live. Well clear of the code below (~$8200) and of the soft stack,
// which grows down from $D000.
#define PROBE ((volatile uint8_t *)0xC000)

CODE_BANK(2) static void inner(void) {
  PROBE[2] = get_bank();
}

// Deliberately in ram_fixed: this is the hop that makes the nesting legal.
__attribute__((noinline)) static void middle(void) { banked_call(2, inner); }

CODE_BANK(1) static void outer(void) {
  PROBE[1] = get_bank();
  middle();
  PROBE[3] = get_bank(); // bank 1 must be back
}

int main(void) {
  PROBE[0] = get_bank(); // 0 at boot
  banked_call(1, outer);
  PROBE[4] = get_bank(); // and restored on unwind
  PROBE[5] = 0xA5;

  xemu_exit(0);
}
