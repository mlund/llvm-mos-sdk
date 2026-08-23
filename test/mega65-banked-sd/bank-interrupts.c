// Switching banks leaves the caller's interrupt state alone. The erratum
// this pins down is written up in mega65-banked-sd/mapper.s.

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

MAPPER_BANK_COUNT(1);

#define FLAG_I 0x04

static uint8_t interrupt_flag(void) {
  uint8_t status;
  // Only CLI, SEI, PLP and RTI write bit 2, so nothing between the caller's
  // set_bank() and here can disturb it.
  asm volatile("php\n\tpla" : "=a"(status));
  return status & FLAG_I;
}

int main(void) {
  // Startup leaves interrupts off and the vectors pointing at an RTI, so
  // enabling them here is safe.
  asm volatile("cli" ::: "p");
  set_bank(1);
  xemu_assert(interrupt_flag() == 0);

  asm volatile("sei" ::: "p");
  set_bank(0);
  xemu_assert(interrupt_flag() == FLAG_I);

  xemu_exit(0);
}
