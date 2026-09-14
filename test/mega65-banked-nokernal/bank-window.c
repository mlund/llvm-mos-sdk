// The whole 24 KB window follows the bank, including its last byte -- an
// offset that is right at $2000 and wrong higher up would otherwise pass.

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

MAPPER_BANK_COUNT(3);

// The first is where the boot code sits in bank 0, so it is only ever written
// with a bank mapped.
static const uint16_t PROBE[] = {0x2000, 0x5000, 0x7fff};
#define PROBES (sizeof PROBE / sizeof *PROBE)

static uint8_t mark(uint8_t bank, uint8_t i) { return (bank << 4) | i; }

int main(void) {
  for (uint8_t bank = 1; bank <= 3; ++bank) {
    set_bank(bank);
    for (uint8_t i = 0; i < PROBES; ++i)
      *(volatile uint8_t *)PROBE[i] = mark(bank, i);
  }

  // Read back only after both are written, so a window that never moved shows
  // up as the second bank's marks under the first.
  for (uint8_t bank = 1; bank <= 3; ++bank) {
    set_bank(bank);
    for (uint8_t i = 0; i < PROBES; ++i)
      xemu_assert(*(volatile uint8_t *)PROBE[i] == mark(bank, i));
  }

  set_bank(0);
  xemu_exit(0);
}
