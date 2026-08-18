// Code must run from attic RAM, not just data sit there.
//
// Banks 8-15 live in HyperRAM at $8000800 and up, reached with a different
// megabyte byte and -- for bank 8 alone -- a MAP offset that relies on the
// 20-bit addition wrapping ($2000 + $FE800 -> $00800). attic-load-banks.c
// only ever read passive signature bytes through set_bank, so the path that
// fetches instructions from attic RAM has never been exercised.
//
// The probes live in ram_fixed rather than attic, which is what lets this run
// under -dumpmem: xemu's memory dump covers main_ram (chip and fast RAM) only,
// and attic is a separate array that never appears in it.
//
// Exit codes (via xemu $D6CF protocol):
//   0 = ran to completion; the dump says which banks answered

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

MAPPER_BANK_COUNT(15);

#define PROBE ((volatile uint8_t *)0xC000)

// Each records the bank it believes it is running in, at its own slot, so a
// bank that never ran leaves its slot untouched rather than being masked by a
// neighbour.
#define ATTIC_FN(n)                                                            \
  CODE_BANK(n) static void fn_##n(void) { PROBE[n - 8] = get_bank(); }

ATTIC_FN(8)
ATTIC_FN(9)
ATTIC_FN(10)
ATTIC_FN(11)
ATTIC_FN(12)
ATTIC_FN(13)
ATTIC_FN(14)
ATTIC_FN(15)

int main(void) {
  banked_call(8, fn_8);
  banked_call(9, fn_9);
  banked_call(10, fn_10);
  banked_call(11, fn_11);
  banked_call(12, fn_12);
  banked_call(13, fn_13);
  banked_call(14, fn_14);
  banked_call(15, fn_15);

  PROBE[8] = 0xA5;
  xemu_exit(0);
}
