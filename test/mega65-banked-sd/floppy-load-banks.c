// Banks load off the mounted D81 through the F011, into chip and attic RAM,
// with no Hyppo trap and no ROM.

#define MAPPER_LOADER_FLOPPY

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

MAPPER_BANK_COUNT(13);

RODATA_BANK(1) static const uint8_t chip[] = {0xb0 | 1, 1};
// Four logical sectors: the chain is followed, both halves of a physical
// sector are used, and the last byte checks the final sector's count.
RODATA_BANK(3) static const uint8_t spans[1000] = {0xb0 | 3, 3, [999] = 0xe3};
RODATA_BANK(13) static const uint8_t attic[] = {0xb0 | 13, 13};

static volatile uint8_t seen;
CODE_BANK(13) static void from_attic(void) { seen = 0xd6; }

static void check(uint8_t bank, const uint8_t *payload) {
  set_bank(bank);
  // Through a volatile pointer: the compiler would otherwise answer from the
  // initialiser rather than from the bank the loader filled.
  xemu_assert(*(const volatile uint8_t *)payload == (0xb0 | bank));
  xemu_assert(*(const volatile uint8_t *)(payload + 1) == bank);
  set_bank(0);
}

int main(void) {
  check(1, chip);
  check(3, spans);
  set_bank(3);
  xemu_assert(*(const volatile uint8_t *)&spans[999] == 0xe3);
  set_bank(0);
  check(13, attic);

  seen = 0;
  banked_call(13, from_attic);
  xemu_assert(seen == 0xd6);
  xemu_exit(0);
}
