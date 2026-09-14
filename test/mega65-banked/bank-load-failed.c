// A bank on the link but not on the disk halts, and names the bank.
//
// __bank_load_failed is weak, so this replaces the default that spins with a
// red border. The D81 is rebuilt without BANK2, so the image still says bank
// 2 holds something while the disk does not.

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

MAPPER_BANK_COUNT(2);

#define MISSING_BANK 2

RODATA_BANK(1) static const uint8_t first[16] = {1};
RODATA_BANK(2) static const uint8_t second[16] = {2};

void __bank_load_failed(unsigned char bank) {
  // ROMC is off again, so the hook may lie anywhere in the fixed region.
  if (*(volatile uint8_t *)0xD030 & 0x20)
    xemu_exit(201);
  xemu_exit(bank == MISSING_BANK ? 0 : bank);
}

int main(void) {
  // Reached only if the absent bank went unnoticed.
  xemu_exit(200);
}
