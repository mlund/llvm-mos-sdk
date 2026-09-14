// A 16 KB window: banks still load and map, and the fixed region starts just
// above the window, at $6000.

#define MAPPER_WINDOW_KB 16
#define MAPPER_BANK_COUNT 1

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

extern const char __bank_window_size[];

RODATA_BANK(1) static const uint8_t payload[] = {0xb1, 0x01};

static volatile uint8_t seen;
CODE_BANK(1) static void run(void) { seen = 0xd1; }

int main(void) {
  xemu_assert((uintptr_t)__bank_window_size == 0x4000);
  // main runs at $6000 or above, so a window still reaching $7FFF would
  // unmap it here.
  xemu_assert((uintptr_t)&main < 0x8000);
  set_bank(1);
  xemu_assert(*(const volatile uint8_t *)payload == 0xb1);
  set_bank(0);

  banked_call(1, run);
  xemu_assert(seen == 0xd1);
  xemu_exit(0);
}
