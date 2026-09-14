// mega65_peek_far and mega65_poke_far reach a 28-bit address without touching
// the map. Each is checked against the KERNAL's own far calls in chip RAM, and
// against each other in attic RAM, which the KERNAL cannot reach. Each address
// byte differs, so a mixed-up argument byte lands somewhere else.

#include <mega65.h>
#include <stdint.h>

#include "../mega65-common/xemu-test.h"

static const uint8_t pattern[] = {0x12, 0x34, 0x56, 0x78};

// Bank 4 is chip RAM at physical $40000.
#define FAR_BANK 4
#define FAR_ADDR 0x2A5C
#define ATTIC_ADDR 0x8013A5Cul

static uint8_t near_byte = 0xA5;

int main(void) {
  // KERNAL far calls need the VIC-IV registers and the ROM at $C000.
  VICIV.key = VIC4_KEY_VICIV_A;
  VICIV.key = VIC4_KEY_VICIV_B;
  VICIV.ctrla = VIC3_PAL_MASK | VIC3_ROMC_MASK | VIC3_CROM9_MASK;

  // KERNAL writes, far reads.
  for (uint8_t i = 0; i < sizeof pattern; ++i)
    mega65_k_sta_far(FAR_BANK, FAR_ADDR, i, pattern[i]);
  for (uint8_t i = 0; i < sizeof pattern; ++i)
    xemu_assert(mega65_peek_far(0x40000ul + FAR_ADDR + i) == pattern[i]);

  // Far writes, KERNAL reads, over the same bytes.
  for (uint8_t i = 0; i < sizeof pattern; ++i)
    mega65_poke_far(0x40000ul + FAR_ADDR + i, (uint8_t)~pattern[i]);
  for (uint8_t i = 0; i < sizeof pattern; ++i)
    xemu_assert(mega65_k_lda_far(FAR_BANK, FAR_ADDR, i) ==
                (uint8_t)~pattern[i]);

  // Attic RAM, both ways.
  for (uint8_t i = 0; i < sizeof pattern; ++i)
    mega65_poke_far(ATTIC_ADDR + i, pattern[i]);
  for (uint8_t i = 0; i < sizeof pattern; ++i)
    xemu_assert(mega65_peek_far(ATTIC_ADDR + i) == pattern[i]);

  // Bank 0 is the CPU's own view, so the byte's address is its physical one.
  uint8_t *p = &near_byte;
  xemu_assert(mega65_peek_far((uintptr_t)p) == 0xA5);
  mega65_poke_far((uintptr_t)p, 0x5A);
  // Z is still 0, or this dereference would read the wrong byte.
  xemu_assert(*p == 0x5A);

  xemu_exit(0);
}
