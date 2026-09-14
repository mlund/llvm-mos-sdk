// A bank whose file is not on the card halts, and names the bank.
//
// __bank_load_failed is weak, so this replaces the default that spins with a
// red border. It runs from .init.250, before main.
//
// The card directory is built from the link and BANK2.BIN then removed, so the
// image still says bank 2 holds something while the card does not.

#include <mapper.h>
#include <mega65.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

MAPPER_BANK_COUNT(2);

#define MISSING_BANK 2

RODATA_BANK(1) static const uint8_t first[16] = {1};
RODATA_BANK(2) static const uint8_t second[16] = {2};

void __bank_load_failed(unsigned char bank) {
  xemu_exit(bank == MISSING_BANK ? 0 : bank);
}

int main(void) {
  // Reached only if the absent bank went unnoticed.
  xemu_exit(200);
}
