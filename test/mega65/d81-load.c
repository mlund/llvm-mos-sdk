// mega65_d81_load() reads a file off the mounted D81 through the F011, beside
// the KERNAL's own disk code. TEST is the disk fixture: a load address, then
// DE AD BE EF CA FE BA BE.

#include <mega65.h>
#include <stdint.h>

#include "../mega65-common/xemu-test.h"

static uint8_t dst[16];

int main(void) {
  // Below $10000 the CPU and DMA see the same memory, so read it back directly.
  xemu_assert(mega65_d81_load("test", (uint16_t)dst) == 10);
  xemu_assert(*(volatile uint8_t *)&dst[2] == 0xde);
  xemu_assert(*(volatile uint8_t *)&dst[9] == 0xbe);
  xemu_assert(mega65_d81_load("missing", (uint16_t)dst) == 0);
  xemu_exit(0);
}
