// The ROMs are gone: $A000-$BFFF and $E000-$FFF9 are RAM, $D000 is still I/O,
// and the vectors are RAM holding the default handler.

#include <mega65.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

#define IRQ_VECTOR (*(volatile uint16_t *)0xfffe)

// Compared against the symbol rather than an address range, so moving the
// fixed region does not silently turn this into a weaker test.
extern void __default_isr(void);

int main(void) {
  // Under BASIC these are ROM for reading, so a write that reads back is the
  // whole test.
  *(volatile uint8_t *)0xbf00 = 0x5a;
  xemu_assert(*(volatile uint8_t *)0xbf00 == 0x5a);
  *(volatile uint8_t *)0xfff0 = 0xa5;
  xemu_assert(*(volatile uint8_t *)0xfff0 == 0xa5);

  VICII.bordercolor = 6;
  xemu_assert((VICII.bordercolor & 0x0f) == 6);

  xemu_assert(IRQ_VECTOR == (uint16_t)(uintptr_t)&__default_isr);

  xemu_exit(0);
}
