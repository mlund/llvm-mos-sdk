// MEGA65 KERNAL FAR memory wrapper test — exercises LDA_FAR, STA_FAR,
// and CMP_FAR by writing data to bank 4 (fast RAM) and reading it back.
//
// Test plan:
//   1. STA_FAR: write 8 bytes to bank 4:$2000
//   2. LDA_FAR: read each byte back and verify
//   3. CMP_FAR: compare each byte and verify equality + inequality
//
// TODO: JSRFAR is not tested here. Attempts to inject code via STA_FAR and
// call it with mega65_k_jsrfar hung in xemu; needs further investigation.

#include <mega65.h>
#include <stdint.h>

#include "../mega65-common/xemu-test.h"

// Test pattern: 8 distinct non-zero bytes
static const unsigned char pattern[] = {0xDE, 0xAD, 0xBE, 0xEF,
                                        0xCA, 0xFE, 0xBA, 0xBE};

// Bank 4 = fast RAM at physical $42000; address $2000 within the bank.
#define FAR_BANK 4
#define FAR_ADDR 0x2000

int main(void) {
  // Unlock VIC-IV registers for KERNAL extensions.
  VICIV.key = 0x47;
  VICIV.key = 0x53;
  VICIV.ctrla = VIC3_PAL_MASK | VIC3_ROMC_MASK | VIC3_CROM9_MASK;

  // --- Phase 1: STA_FAR — write pattern bytes to bank 4:$2000 ---
  for (unsigned char i = 0; i < sizeof(pattern); i++) {
    mega65_k_sta_far(FAR_BANK, FAR_ADDR, i, pattern[i]);
  }

  // --- Phase 2: LDA_FAR — read back and verify each byte ---
  for (unsigned char i = 0; i < sizeof(pattern); i++) {
    unsigned char got = mega65_k_lda_far(FAR_BANK, FAR_ADDR, i);
    if (got != pattern[i])
      xemu_exit(10 + i); // exit 10-17: LDA_FAR mismatch at byte i
  }

  // --- Phase 3: CMP_FAR — compare each byte for equality ---
  for (unsigned char i = 0; i < sizeof(pattern); i++) {
    if (mega65_k_cmp_far(FAR_BANK, FAR_ADDR, i, pattern[i]) != 0)
      xemu_exit(20 + i); // exit 20-27: CMP_FAR mismatch at byte i
  }

  // Verify CMP_FAR returns non-zero for a wrong value
  if (mega65_k_cmp_far(FAR_BANK, FAR_ADDR, 0, 0x00) == 0)
    xemu_exit(30); // exit 30: CMP_FAR false positive

  // Restore startup ROM banking.
  VICIV.ctrla = VIC3_PAL_MASK | VIC3_CROM9_MASK;

  xemu_exit(0);
}
