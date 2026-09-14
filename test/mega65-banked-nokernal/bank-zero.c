// Bank 0 is the window unmapped, so .bank_0 content ships inside the main
// image rather than a bank file, and banked_call(0, ...) reaches it with no
// bank mapped at all.
//
// MAPPER_BANK_COUNT(0) reserves no slots, which is the low end of the range.

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

MAPPER_BANK_COUNT(0);

static volatile uint8_t idx;
static volatile uint8_t seen;

RODATA_BANK(0) static const uint8_t table[4] = {0x10, 0x20, 0x30, 0x40};

CODE_BANK(0) static void run(void) { seen = table[idx]; }

int main(void) {
  idx = 2;
  banked_call(0, run);

  xemu_assert(seen == 0x30);
  xemu_assert(get_bank() == 0);

  xemu_exit(0);
}
