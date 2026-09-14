// banked_call_r evaluates its arguments before switching, so an argument may
// read the bank mapped at the call.

#define MAPPER_BANK_COUNT 2

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

RODATA_BANK(1) static const uint8_t in_bank_1[] = {0x5a};

CODE_BANK(2) static uint8_t echo(uint8_t v) { return v; }

int main(void) {
  set_bank(1);
  uint8_t got = banked_call_r(2, echo, *(const volatile uint8_t *)in_bank_1);
  xemu_assert(got == 0x5a);
  xemu_assert(get_bank() == 1);
  set_bank(0);
  xemu_exit(0);
}
