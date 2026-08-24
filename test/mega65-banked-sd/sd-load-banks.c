// Bank content reaches chip, fast and attic RAM. Attic needs the other Hyppo
// trap, since loadfile forces the top address byte to zero.

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

MAPPER_BANK_COUNT(14);

// Each payload spells out its own bank, so what is expected comes from the
// bank number rather than from a second copy of the bytes.
RODATA_BANK(1) static const uint8_t chip[] = {0xb0 | 1, 1};
// Where the C65 ROMs would be: only reachable because startup lifted the
// hypervisor's write protection, and Hyppo would have reported success anyway.
RODATA_BANK(3) static const uint8_t rom_low[] = {0xb0 | 3, 3};
RODATA_BANK(7) static const uint8_t rom_high[] = {0xb0 | 7, 7};
RODATA_BANK(8) static const uint8_t fast[] = {0xb0 | 8, 8};
RODATA_BANK(13) static const uint8_t attic[] = {0xb0 | 13, 13};
RODATA_BANK(14) static const uint8_t attic_next[] = {0xb0 | 14, 14};

static volatile uint8_t seen;
CODE_BANK(13) static void from_attic(void) { seen = 0xd6; }
CODE_BANK(4) static void from_rom_region(void) { seen = 0xd3; }

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
  check(3, rom_low);
  check(7, rom_high);
  check(8, fast);
  check(13, attic);
  check(14, attic_next);

  seen = 0;
  banked_call(4, from_rom_region);
  xemu_assert(seen == 0xd3);

  seen = 0;
  banked_call(13, from_attic);
  xemu_assert(seen == 0xd6);

  xemu_exit(0);
}
