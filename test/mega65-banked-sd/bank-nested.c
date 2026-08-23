// banked_call nests, and get_bank() tells the truth at every depth.
//
// The call graph is the shape banked code is supposed to use: a banked
// function never calls into another bank directly, it returns to fixed code
// which makes the next call. Bank 1 is unmapped while bank 2 runs, so
// outer()'s own code is absent from the window for the duration -- it is only
// safe because banked_call restores bank 1 before returning into it.
//
//   main (fixed) -> outer (bank 1) -> middle (fixed) -> inner (bank 2)

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

MAPPER_BANK_COUNT(2);

// In .bss, which is never remapped, so it reads the same whichever bank is live.
static volatile uint8_t depth[5];

CODE_BANK(2) static void inner(void) { depth[2] = get_bank(); }

// Deliberately in the fixed region: this is the hop that makes nesting legal.
__attribute__((noinline)) static void middle(void) { banked_call(2, inner); }

CODE_BANK(1) static void outer(void) {
  depth[1] = get_bank();
  middle();
  depth[3] = get_bank();
}

int main(void) {
  depth[0] = get_bank();
  banked_call(1, outer);
  depth[4] = get_bank();

  xemu_assert(depth[0] == 0);
  xemu_assert(depth[1] == 1);
  xemu_assert(depth[2] == 2);
  xemu_assert(depth[3] == 1);
  xemu_assert(depth[4] == 0);

  xemu_exit(0);
}
