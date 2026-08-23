// The window is 32 KB and reaches across both halves of the map: MAPLO covers
// $2000-$7FFF, MAPHI block 0 covers $8000-$9FFF, and the two have separate
// offsets and megabyte bytes that have to agree.

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

MAPPER_BANK_COUNT(2);

// The last is in the MAPHI half; the first is where the boot code sits in
// bank 0, so it is only ever written with a bank mapped.
static const uint16_t PROBE[] = {0x2000, 0x6000, 0x9fff};
#define PROBES (sizeof PROBE / sizeof *PROBE)

static uint8_t mark(uint8_t bank, uint8_t i) { return (bank << 4) | i; }

int main(void) {
  for (uint8_t bank = 1; bank <= 2; ++bank) {
    set_bank(bank);
    for (uint8_t i = 0; i < PROBES; ++i)
      *(volatile uint8_t *)PROBE[i] = mark(bank, i);
  }

  // Read back only after both are written, so a window that never moved shows
  // up as the second bank's marks under the first.
  for (uint8_t bank = 1; bank <= 2; ++bank) {
    set_bank(bank);
    for (uint8_t i = 0; i < PROBES; ++i)
      xemu_assert(*(volatile uint8_t *)PROBE[i] == mark(bank, i));
  }

  set_bank(0);
  xemu_exit(0);
}
