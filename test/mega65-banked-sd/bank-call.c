// Code runs from every chip and fast RAM bank, loaded off the card by the CRT.

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

MAPPER_BANK_COUNT(5);

// In .bss, which is never remapped, so a banked function can report through it.
static volatile uint8_t seen;

CODE_BANK(1) static void from1(void) { seen = 0xb1; }
CODE_BANK(2) static void from2(void) { seen = 0xb2; }
CODE_BANK(3) static void from3(void) { seen = 0xb3; }
CODE_BANK(4) static void from4(void) { seen = 0xb4; }
CODE_BANK(5) static void from5(void) { seen = 0xb5; }

static void (*const BANKED[])(void) = {from1, from2, from3, from4, from5};

int main(void) {
  for (uint8_t bank = 1; bank <= 5; ++bank) {
    seen = 0;
    banked_call(bank, BANKED[bank - 1]);
    xemu_assert(seen == 0xb0 + bank);
    xemu_assert(get_bank() == 0);
  }
  xemu_exit(0);
}
