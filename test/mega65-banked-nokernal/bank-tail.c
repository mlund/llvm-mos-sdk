// Banks of different sizes: bank 1 is 16 KB, so while it is mapped the top of
// the window is the window's own RAM, where WINDOW_TAIL data lives; bank 2
// keeps all 24 KB and covers it.

#define MAPPER_BANK_1_KB 16

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

MAPPER_BANK_COUNT(2);

WINDOW_TAIL static uint8_t shared[4];

RODATA_BANK(1) static const uint8_t first[] = {0xb1};
// Its last byte sits at $6FFF, where only a 24 KB bank maps.
RODATA_BANK(2) static const uint8_t second[0x5000] = {[0x4FFF] = 0xb2};

int main(void) {
  xemu_assert(BANK_SIZE_1 == 0x4000ul && BANK_SIZE_2 == 0x6000ul);
  xemu_assert((uintptr_t)shared == 0x6000);

  shared[0] = 0x77;
  set_bank(1);
  xemu_assert(*(const volatile uint8_t *)first == 0xb1);
  xemu_assert(*(volatile uint8_t *)shared == 0x77);

  set_bank(2);
  xemu_assert(*(const volatile uint8_t *)&second[0x4FFF] == 0xb2);

  set_bank(0);
  xemu_assert(*(volatile uint8_t *)shared == 0x77);
  xemu_exit(0);
}
