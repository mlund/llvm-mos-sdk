// mega65_d81_load() reads a named file off the mounted D81 into any 28-bit
// address, with no ROM and no Hyppo. The converter put pattern.bin on the disk
// as PATTERN: 700 bytes of i % 251.

#define MAPPER_LOADER_FLOPPY

#include <mapper.h>
#include <mega65.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

MAPPER_BANK_COUNT(0);

static void check_pattern(uint8_t bank, uint16_t offset) {
  const volatile uint8_t *p = (const volatile uint8_t *)(0x2000 + offset);
  set_bank(bank);
  xemu_assert(p[0] == 0 && p[250] == 250 && p[251] == 0);
  xemu_assert(p[699] == 699 % 251);
  set_bank(0);
}

int main(void) {
  // Lower case is folded to the name on the disk.
  xemu_assert(mega65_d81_load("pattern", BANK_PHYS_BASE_1 + 0x100) == 700);
  check_pattern(1, 0x100);
  xemu_assert(mega65_d81_load("PATTERN", BANK_PHYS_BASE_13) == 700);
  check_pattern(13, 0);
  xemu_assert(mega65_d81_load("MISSING", BANK_PHYS_BASE_2) == 0);
  xemu_exit(0);
}
