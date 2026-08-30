// A banked function reaches another bank with no fixed-region hop.
//
// banked_call() lives in the fixed region, so the outer bank leaves the
// window only while its own code is not executing: the trampoline maps the
// inner bank, calls, and restores before returning. Each function reads a
// table in its own bank, so the values say which bank was live rather than
// what the shadow believes.

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

MAPPER_BANK_COUNT(2);

static volatile uint8_t seen[3];
static volatile uint8_t idx; // a literal index would fold and read nothing

RODATA_BANK(1) static const uint8_t mark1[2] = {0x11, 0x22};
RODATA_BANK(2) static const uint8_t mark2[2] = {0x33, 0x44};

CODE_BANK(2) static void inner(void) { seen[1] = mark2[idx]; }

CODE_BANK(1) static void outer(void) {
  seen[0] = mark1[idx];
  banked_call(2, inner);
  seen[2] = mark1[idx];
}

int main(void) {
  banked_call(1, outer);

  xemu_assert(seen[0] == 0x11);
  xemu_assert(seen[1] == 0x33);
  xemu_assert(seen[2] == 0x11);
  xemu_assert(get_bank() == 0);

  xemu_exit(0);
}
