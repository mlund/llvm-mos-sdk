// A banked call carries arguments and a return value.
//
// The arguments come from volatile globals and the callee reads a table that
// only exists in bank 1, so neither the call nor the bank switch can be folded
// away -- a constant-folded answer would pass without either happening.

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

MAPPER_BANK_COUNT(2);

static volatile uint8_t arg_index = 2;
static volatile int arg_bias = 100;

// In .bss, so the banked callee can still reach it through a pointer. The
// pointer is itself volatile, or the constant address folds into the callee
// and the argument stops being tested.
static volatile uint8_t sink;
static volatile uint8_t *volatile sink_ptr = &sink;

RODATA_BANK(1) static const uint8_t table[4] = {10, 20, 30, 40};

CODE_BANK(1) static int lookup(uint8_t i, int bias) { return table[i] + bias; }

CODE_BANK(1) static void store(volatile uint8_t *dst, uint8_t i) {
  *dst = table[i];
}

CODE_BANK(2) static int twice(int v) { return v * 2; }

// Fixed region: the hop a banked function makes to reach another bank.
__attribute__((noinline)) static int hop(int v) {
  return banked_call_r(2, twice, v);
}

// Reads table after the inner call, so the value proves bank 1 came back.
// A literal index would fold to its value and prove nothing.
CODE_BANK(1) static int outer(int v) { return hop(v) + table[arg_index]; }

int main(void) {
  int r = banked_call_r(1, lookup, arg_index, arg_bias);

  xemu_assert(r == 130);
  xemu_assert(get_bank() == 0);

  banked_call_v(1, store, sink_ptr, arg_index);

  xemu_assert(sink == 30);
  xemu_assert(get_bank() == 0);

  int nested = banked_call_r(1, outer, arg_bias);

  xemu_assert(nested == 230);
  xemu_assert(get_bank() == 0);

  xemu_exit(0);
}
