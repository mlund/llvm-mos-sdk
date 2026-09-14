// A bank above MAPPER_BANK_COUNT must not reach the MAP registers unchecked.
//
// The bank tables end at the declared count, so a larger id would read past
// them and hand whatever follows to MAP. MAPLO covers $0000-$7FFF, which
// includes zero page and therefore the compiler's imaginary registers, so the
// damage from a wild offset is not confined to the banked window.
//
// The contract this pins down is that such an id maps bank 0, and get_bank()
// reports 0.
//
// Exit codes (via xemu $D6CF protocol):
//   0 = ran to completion; the dump says which bank was actually mapped

#define MAPPER_BANK_COUNT 1

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

// $FB-$FE are unallocated by the MEGA65 KERNAL and outside the compiler's
// zero page ($02-$8F).
#define PROBE ((volatile uint8_t *)0x00FB)

// First byte of the banked window.
#define WINDOW (*(volatile uint8_t *)0x2000)

int main(void) {
  // Mark bank 1, then leave a different value in bank 0 so that reading the
  // mark back cannot be an accident of the window never having moved.
  set_bank(1);
  WINDOW = 0x5A;
  set_bank(0);
  WINDOW = 0x00;

  // From bank 1, the id just past the count.
  set_bank(1);
  set_bank(2);
  PROBE[0] = WINDOW;
  PROBE[1] = get_bank();

  set_bank(0);
  PROBE[2] = 0xA5;

  xemu_exit(0);
}
