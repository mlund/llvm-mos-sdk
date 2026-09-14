// A bank above MAPPER_BANK_COUNT must not reach the MAP registers unchecked.
//
// The bank tables end at the declared count, so a larger id would read past
// them and hand whatever follows to MAP. MAPLO covers $0000-$7FFF, which
// includes the compiler's imaginary registers, so a wild offset is not
// confined to the window.
//
// The contract is that such an id maps bank 0, and get_bank() reports 0.

#define MAPPER_BANK_COUNT 1

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

// Past the boot code, so bank 0 can be written here too.
#define WINDOW (*(volatile uint8_t *)0x3000)

int main(void) {
  set_bank(1);
  WINDOW = 0x5a;
  // A different value in bank 0, so reading the mark back cannot be an
  // accident of the window never having moved.
  set_bank(0);
  WINDOW = 0x00;

  // Just past the count, then far past it, each from bank 1.
  set_bank(1);
  set_bank(2);
  xemu_assert(WINDOW == 0x00);
  xemu_assert(get_bank() == 0);

  set_bank(1);
  set_bank(0xff);
  xemu_assert(WINDOW == 0x00);
  xemu_assert(get_bank() == 0);

  set_bank(1);
  xemu_assert(WINDOW == 0x5a);
  set_bank(0);
  xemu_exit(0);
}
