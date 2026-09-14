// .data lives in ram_high but loads from ram_fixed, so its initialisers only
// arrive if the copy routine is linked and the image carries them.

#include <mega65.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

// Pinned to .data: left alone, the zero-page allocator promotes an array
// this small and the copy under test never runs.
__attribute__((section(".data")))
uint8_t table[8] = {0xde, 0xad, 0xbe, 0xef, 0xca, 0xfe, 0xba, 0xbe};
uint8_t zeroed[8];

int main(void) {
  static const uint8_t expected[8] = {0xde, 0xad, 0xbe, 0xef,
                                      0xca, 0xfe, 0xba, 0xbe};
  xemu_assert((uint16_t)table >= 0xe000);
  for (uint8_t i = 0; i < 8; ++i) {
    xemu_assert(table[i] == expected[i]);
    xemu_assert(zeroed[i] == 0);
  }
  xemu_exit(0);
}
