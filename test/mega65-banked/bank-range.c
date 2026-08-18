// A bank id outside 0-15 must not reach the MAP registers unchecked.
//
// bank_map_table holds 16 entries and is indexed by bank_id * 2, so a caller
// passing 16 or more reads past the end of it and hands whatever follows to
// the MAP instruction.  MAPLO covers $0000-$7FFF, which includes zero page
// and therefore the compiler's imaginary registers, so the damage from a wild
// offset is not confined to the banked window.
//
// The contract this pins down is that the id is masked to its low four bits:
// set_bank(0x11) does what set_bank(1) does.  Masking rather than rejecting,
// because there is no error to return and a branch would cost more than the
// AND that avoids it.
//
// Exit codes (via xemu $D6CF protocol):
//   0 = ran to completion; the dump says which bank was actually mapped

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

  // The out-of-range id: low four bits select bank 1.
  set_bank(0x11);
  PROBE[0] = WINDOW;

  set_bank(0);
  PROBE[1] = 0xA5;

  xemu_exit(0);
}
