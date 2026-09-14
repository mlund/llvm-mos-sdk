// With MAPPER_LOADER_SD, banks load off the SD card through Hyppo while the
// KERNAL stays: into chip RAM below and above the ROMs, and into attic RAM up
// to the last bank, whose file name has two digits.

#define MAPPER_LOADER_SD
#define MAPPER_BANK_COUNT 31

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

// Each payload spells out its own bank, so what is expected comes from the
// bank number rather than from a second copy of the bytes.
RODATA_BANK(1) static const uint8_t chip[] = {0xb0 | 1, 1};
RODATA_BANK(3) static const uint8_t above_roms[] = {0xb0 | 3, 3};
RODATA_BANK(8) static const uint8_t attic[] = {0xb0 | 8, 8};
RODATA_BANK(31) static const uint8_t last[] = {0xb0 | 31, 31};

static volatile uint8_t seen;
CODE_BANK(8) static void from_attic(void) { seen = 0xd8; }

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
  check(3, above_roms);
  check(8, attic);
  check(31, last);

  seen = 0;
  banked_call(8, from_attic);
  xemu_assert(seen == 0xd8);

  xemu_exit(0);
}
