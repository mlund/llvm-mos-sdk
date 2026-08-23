// Zero page runs to $FF with the KERNAL gone. The linker script asserts the
// region size at link time; this checks the far end is really memory.

#include <mega65.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

int main(void) {
  // Below __rc0 is the CPU port, so $22 is the first byte past the imaginary
  // registers and $FF the last of the page.
  *(volatile uint8_t *)0x22 = 0x5a;
  *(volatile uint8_t *)0xff = 0xa5;
  xemu_assert(*(volatile uint8_t *)0x22 == 0x5a);
  xemu_assert(*(volatile uint8_t *)0xff == 0xa5);
  xemu_exit(0);
}
