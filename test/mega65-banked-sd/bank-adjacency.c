// Banks do not overlap each other.
//
// bank-window.c probes three addresses per bank, so an overlap that misses
// all three reads clean. Every byte of each bank is filled with a value that
// depends on the bank, all banks written before any is read, so any aliasing
// between two of them shows up as a mismatch wherever it happens to be.
//
// Bank 0 is left alone: it is the window unmapped, where this program lives.

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

MAPPER_BANK_COUNT(3);

#define WINDOW ((volatile uint8_t *)0x2000)
#define WINDOW_SIZE 0x6000

static uint8_t pattern(uint8_t bank, uint16_t off) {
  return (uint8_t)(bank * 0x11u) ^ (uint8_t)(off >> 8);
}

// Fixed region, so the loop survives the bank it is writing to.
__attribute__((noinline)) static void fill(uint8_t bank) {
  set_bank(bank);
  for (uint16_t i = 0; i < WINDOW_SIZE; ++i)
    WINDOW[i] = pattern(bank, i);
}

__attribute__((noinline)) static uint8_t intact(uint8_t bank) {
  set_bank(bank);
  for (uint16_t i = 0; i < WINDOW_SIZE; ++i)
    if (WINDOW[i] != pattern(bank, i))
      return 0;
  return 1;
}

int main(void) {
  for (uint8_t b = 1; b <= 3; ++b)
    fill(b);

  xemu_assert(intact(1));
  xemu_assert(intact(2));
  xemu_assert(intact(3));

  set_bank(0);
  xemu_exit(0);
}
