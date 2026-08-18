// Test CRT bank loader: verify that attic RAM bank data loaded from D81.
//
// Each attic bank section (.bank_8 through .bank_15) contains a known
// 2-byte signature. The CRT init __load_banks (.init.150) loads BANK8-BANKF
// from the D81 disk via KERNAL 28-bit SETBNK + LOAD. main() switches to
// each bank and verifies the signature at virtual address $2000 (the banked
// window start, mapped to attic RAM via MAP with megabyte byte $80).
//
// Exit codes (xemu $D6CF protocol):
//   0       = all banks verified OK
//   8-15    = bank N signature mismatch

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

// "retain" prevents the linker from discarding these unreferenced sections.
// "used" prevents the compiler from dropping them before the linker sees them.
__attribute__((used, retain, section(".bank_8")))
static const uint8_t bank8_sig[] = {0xB8, 0x08};
__attribute__((used, retain, section(".bank_9")))
static const uint8_t bank9_sig[] = {0xB9, 0x09};
__attribute__((used, retain, section(".bank_10")))
static const uint8_t bank10_sig[] = {0xBA, 0x0A};
__attribute__((used, retain, section(".bank_11")))
static const uint8_t bank11_sig[] = {0xBB, 0x0B};
__attribute__((used, retain, section(".bank_12")))
static const uint8_t bank12_sig[] = {0xBC, 0x0C};
__attribute__((used, retain, section(".bank_13")))
static const uint8_t bank13_sig[] = {0xBD, 0x0D};
__attribute__((used, retain, section(".bank_14")))
static const uint8_t bank14_sig[] = {0xBE, 0x0E};
__attribute__((used, retain, section(".bank_15")))
static const uint8_t bank15_sig[] = {0xBF, 0x0F};

int main(void) {
  // Verify each attic bank was loaded from D81 by checking the known signature.
  for (uint8_t bank = 8; bank <= 15; bank++) {
    set_bank(bank);
    volatile uint8_t *data = (volatile uint8_t *)0x2000;
    if (data[0] != (0xB0 + bank) || data[1] != bank)
      xemu_exit(bank);
  }

  set_bank(0);
  xemu_exit(0);
}
