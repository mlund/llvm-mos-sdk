// Tracer bullet for the -dumpmem test path.
//
// Asserts nothing about banking that bank-call.c does not already cover; its
// job is to prove that a value written by the test program can be read back
// out of xemu's memory dump, so that later tests can report something richer
// than the single byte the $D6CF exit protocol carries.
//
// Exit codes (via xemu $D6CF protocol):
//   0 = reached the end; the dump is what says whether the values were right

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

// $FB-$FE are unallocated by the MEGA65 KERNAL and outside the compiler's
// zero page ($02-$8F), which is why bank-call.c already signals through $FC.
// Zero page is bank-0 chip RAM at the same physical address, so these are
// offsets $FB.. in a -dumpmem image.
#define PROBE ((volatile uint8_t *)0x00FB)

int main(void) {
  // Nothing has switched banks yet, so the shadow must still read 0.
  PROBE[0] = get_bank();
  // Sentinel: without it, a program that never ran would leave zeroes that
  // look exactly like a correct answer for the byte above.
  PROBE[1] = 0xA5;

  xemu_exit(0);
}
