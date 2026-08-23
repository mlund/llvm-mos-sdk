// Bank content reaches chip, fast and attic RAM. Attic needs the other Hyppo
// trap, since loadfile forces the top address byte to zero.

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

MAPPER_BANK_COUNT(7);

// Each payload spells out its own bank, so what is expected comes from the
// bank number rather than from a second copy of the bytes.
RODATA_BANK(1) static const uint8_t chip[] = {0xb0 | 1, 1};
RODATA_BANK(2) static const uint8_t fast[] = {0xb0 | 2, 2};
RODATA_BANK(6) static const uint8_t attic_low[] = {0xb0 | 6, 6};
RODATA_BANK(7) static const uint8_t attic_high[] = {0xb0 | 7, 7};

static volatile uint8_t seen;
CODE_BANK(6) static void from_attic(void) { seen = 0xd6; }

static void check(uint8_t bank, const uint8_t *payload) {
  set_bank(bank);
  // Through a volatile pointer: the compiler would otherwise answer from the
  // initialiser rather than from the bank the card filled.
  xemu_assert(*(const volatile uint8_t *)payload == (0xb0 | bank));
  xemu_assert(*(const volatile uint8_t *)(payload + 1) == bank);
  set_bank(0);
}

int main(void) {
  check(1, chip);
  check(2, fast);
  check(6, attic_low);
  check(7, attic_high);

  seen = 0;
  banked_call(6, from_attic);
  xemu_assert(seen == 0xd6);

  xemu_exit(0);
}
