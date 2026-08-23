// The ROMs are gone: $A000-$BFFF and $E000-$FFF9 are RAM, $D000 is still I/O,
// and the vectors are RAM holding the default handler.

#include <mega65.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

#define IRQ_VECTOR (*(volatile uint16_t *)0xfffe)

int main(void) {
  // Under BASIC these are ROM for reading, so a write that reads back is the
  // whole test.
  *(volatile uint8_t *)0xbf00 = 0x5a;
  xemu_assert(*(volatile uint8_t *)0xbf00 == 0x5a);
  *(volatile uint8_t *)0xfff0 = 0xa5;
  xemu_assert(*(volatile uint8_t *)0xfff0 == 0xa5);

  VICII.bordercolor = 6;
  xemu_assert((VICII.bordercolor & 0x0f) == 6);

  // Both vectors point at the default handler, which is in the fixed region.
  xemu_assert(IRQ_VECTOR >= 0xa000 && IRQ_VECTOR < 0xd000);

  xemu_exit(0);
}
