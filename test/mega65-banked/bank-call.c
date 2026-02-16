// Test all 8 banks (0-7) via banked_call().
//
// Phase 1: manually switch each bank, inject unique machine code at $2000.
// Phase 2: banked_call() each bank, verify ZP signature at $FC.
//
// Exit codes (via xemu $D6CF protocol):
//   0        = all pass
//   1-8      = inject failed for bank N-1
//   0x10+N   = banked_call to bank N returned wrong signature
//   0x80+val = bank 0 restore debug (saw val instead of 0x40)

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

// Signal address outside the banked window ($2000-$7FFF).
// ZP byte $FC is free from compiler ($02-$8F) and never remapped.
#define SIG_ADDR ((volatile uint8_t *)0x00FC)

static void inject_code(uint8_t magic) {
  volatile uint8_t *base = (volatile uint8_t *)0x2000;
  base[0] = 0xA9; // LDA #imm
  base[1] = magic;
  base[2] = 0x85; // STA zp
  base[3] = 0xFC;
  base[4] = 0x60; // RTS
}

static void (*const banked_func)(void) = (void (*)(void))0x2000;

int main(void) {
  // Phase 1: Inject code into each bank.
  for (uint8_t bank = 0; bank <= 7; bank++) {
    set_bank(bank);
    inject_code(0x40 + bank);
    if (*(volatile uint8_t *)0x2001 != (0x40 + bank))
      xemu_exit(bank + 1);
  }

  // Restore bank 0 and verify.
  set_bank(0);
  uint8_t code_byte = *(volatile uint8_t *)0x2001;
  if (code_byte != 0x40)
    xemu_exit(0x80 | code_byte);

  // Phase 2: Call each bank via banked_call() and verify signature.
  for (uint8_t bank = 0; bank <= 7; bank++) {
    *SIG_ADDR = 0;
    banked_call(bank, banked_func);
    uint8_t got = *SIG_ADDR;
    if (got != (0x40 + bank))
      xemu_exit(0x10 + bank);
  }

  xemu_exit(0);
}
