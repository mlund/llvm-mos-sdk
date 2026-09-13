// A bank moved with MAPPER_BANK_n loads and maps at its new address: the
// loader and the bank switch both read the tables this layout emits.

#define MAPPER_BANK_2 0x8040800

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

MAPPER_BANK_COUNT(2);

RODATA_BANK(2) static const uint8_t payload[] = {0xb2, 0x02};

static volatile uint8_t seen;
CODE_BANK(2) static void run(void) { seen = 0xd2; }

int main(void) {
  xemu_assert(BANK_PHYS_BASE_2 == 0x8040800ul);

  set_bank(2);
  // Through a volatile pointer: the compiler would otherwise answer from the
  // initialiser rather than from the bank the loader filled.
  xemu_assert(*(const volatile uint8_t *)payload == 0xb2);
  xemu_assert(*(const volatile uint8_t *)(payload + 1) == 0x02);
  set_bank(0);

  banked_call(2, run);
  xemu_assert(seen == 0xd2);
  xemu_exit(0);
}
