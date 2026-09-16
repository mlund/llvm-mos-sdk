// A reference argument must stay a reference: a banked callee writing through
// T& has to reach the caller's own object, not a copy made before the switch.

#define MAPPER_BANK_COUNT 1

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

struct Counter {
  uint16_t hits;
  uint8_t last;
};

// In the fixed region, which the callee can reach with its own bank mapped.
static Counter counter;

CODE_BANK(1) static void bump(Counter &c, uint8_t value) {
  c.hits += 1;
  c.last = value;
}

CODE_BANK(1) static uint8_t total(const Counter &c) {
  return (uint8_t)c.hits + c.last;
}

int main(void) {
  counter.hits = 0;
  counter.last = 0;

  banked_call_v(1, bump, counter, 0x5a);
  xemu_assert(counter.hits == 1);
  xemu_assert(counter.last == 0x5a);

  // A const reference reads the caller's object too, not a copy of it.
  counter.hits = 2;
  xemu_assert(banked_call_r(1, total, counter) == (uint8_t)(2 + 0x5a));

  xemu_exit(0);
}
