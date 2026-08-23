// A bank id outside 0-15 must not reach the MAP registers unchecked.
//
// The bank tables hold 16 entries, so a caller passing 16 or more reads past
// the end of them and hands whatever follows to MAP. MAPLO covers
// $0000-$7FFF, which includes the compiler's imaginary registers, so a wild
// offset is not confined to the window.
//
// The contract is that the id is masked: set_bank(0x11) does what set_bank(1)
// does. Masking rather than rejecting, because there is no error to return
// and a branch would cost more than the AND that avoids it.

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

MAPPER_BANK_COUNT(1);

// Past the boot code, so bank 0 can be written here too.
#define WINDOW (*(volatile uint8_t *)0x3000)

int main(void) {
  set_bank(1);
  WINDOW = 0x5a;
  // A different value in bank 0, so reading the mark back cannot be an
  // accident of the window never having moved.
  set_bank(0);
  WINDOW = 0x00;

  set_bank(0x11);
  xemu_assert(WINDOW == 0x5a);

  set_bank(0);
  xemu_assert(WINDOW == 0x00);

  xemu_exit(0);
}
