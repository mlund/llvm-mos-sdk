// mega65_d81_load() needs neither the KERNAL nor Hyppo, so it works here too:
// it reads bank 1's own file back off the autoboot disk.

#include <mapper.h>
#include <mega65.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

MAPPER_BANK_COUNT(2);

RODATA_BANK(1) static const uint8_t signature[] = {0xb1, 0x01};

int main(void) {
  // The converter's two-byte PRG header, then the bank's content.
  xemu_assert(mega65_d81_load("bank1", BANK_PHYS_BASE_2) == 4);
  set_bank(2);
  xemu_assert(*(const volatile uint8_t *)0x2002 == 0xb1);
  xemu_assert(*(const volatile uint8_t *)0x2003 == 0x01);
  set_bank(0);
  xemu_exit(0);
}
