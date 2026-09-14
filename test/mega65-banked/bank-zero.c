// banked_call(0, ...) reaches .bank_0 content, which this platform keeps in the
// fixed region.
//
// MAPPER_BANK_COUNT 0 reserves no slots, which is the low end of the range.

#define MAPPER_BANK_COUNT 0

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

static volatile uint8_t idx;
static volatile uint8_t seen;

RODATA_BANK(0) static const uint8_t table[4] = {0x10, 0x20, 0x30, 0x40};

// Big enough that bank 0 placed ahead of .text would push startup past $A000,
// which is still BASIC ROM when the stub SYSes in.
RODATA_BANK(0) static const uint8_t big[0x2000] = {[0x1FFF] = 0x5A};

CODE_BANK(0) static void run(void) { seen = table[idx]; }

int main(void) {
  idx = 2;
  banked_call(0, run);

  xemu_assert(seen == 0x30);
  xemu_assert(get_bank() == 0);
  xemu_assert(*(const volatile uint8_t *)&big[0x1FFF] == 0x5A);

  xemu_exit(0);
}
