// A 16 KB window: banks still load and map, and the soft stack takes
// $6000-$7FFF, outside every mapped bank.

#define MAPPER_WINDOW_KB 16
#define MAPPER_BANK_COUNT 1

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

extern const char __bank_window_size[];
extern const char __stack[];

RODATA_BANK(1) static const uint8_t payload[] = {0xb1, 0x01};

static volatile uint8_t seen;
CODE_BANK(1) static void run(void) { seen = 0xd1; }

int main(void) {
  xemu_assert((uintptr_t)__bank_window_size == 0x4000);
  xemu_assert((uintptr_t)__stack == 0x8000);

  // Written with bank 1 mapped, read back with bank 0: a 24 KB window would
  // have put the write into bank 1.
  uint8_t *slot = __builtin_alloca(4);
  xemu_assert((uintptr_t)slot >= 0x6000);
  set_bank(1);
  slot[0] = 0x5a;
  xemu_assert(*(const volatile uint8_t *)payload == 0xb1);
  set_bank(0);
  xemu_assert(*(volatile uint8_t *)slot == 0x5a);

  banked_call(1, run);
  xemu_assert(seen == 0xd1);
  xemu_exit(0);
}
