// Test CRT bank loader: verify that bank data loaded from D81 at startup.
//
// Each bank section contains a known 2-byte signature. The CRT init
// __load_banks (.init.150) loads BANK1-BANK7 from the D81 disk via
// KERNAL SETBNK + LOAD. main() switches to each bank and verifies the
// signature is present at virtual address $2000 (the banked window start).
//
// Exit codes (xemu $D6CF protocol):
//   0      = all banks verified OK
//   1-7    = bank N signature mismatch

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

// "retain" prevents the linker from discarding these unreferenced sections.
// "used" prevents the compiler from dropping them before the linker sees them.
__attribute__((used, retain, section(".bank_1")))
static const uint8_t bank1_sig[] = {0xB1, 0x01};
__attribute__((used, retain, section(".bank_2")))
static const uint8_t bank2_sig[] = {0xB2, 0x02};
__attribute__((used, retain, section(".bank_3")))
static const uint8_t bank3_sig[] = {0xB3, 0x03};
__attribute__((used, retain, section(".bank_4")))
static const uint8_t bank4_sig[] = {0xB4, 0x04};
__attribute__((used, retain, section(".bank_5")))
static const uint8_t bank5_sig[] = {0xB5, 0x05};
__attribute__((used, retain, section(".bank_6")))
static const uint8_t bank6_sig[] = {0xB6, 0x06};
__attribute__((used, retain, section(".bank_7")))
static const uint8_t bank7_sig[] = {0xB7, 0x07};

int main(void) {
  // Verify each bank was loaded from D81 by checking the known signature.
  for (uint8_t bank = 1; bank <= 7; bank++) {
    set_bank(bank);
    volatile uint8_t *data = (volatile uint8_t *)0x2000;
    if (data[0] != (0xB0 + bank) || data[1] != bank)
      xemu_exit(bank);
  }

  set_bank(0);
  xemu_exit(0);
}
