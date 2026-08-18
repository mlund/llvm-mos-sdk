// A KERNAL disk call must not leave a different bank in the window.
//
// The KERNAL installs its own mapping for disk work and restores the
// KERNAL's on the way out, not the caller's, so the window can come back
// pointing somewhere else while _BANK_SHADOW still claims the old bank.
//
// Reaching the disk routines at all needs the C65 interface ROM at
// $C000-$CFFF, which the CRT unmaps; without it the call never returns.
//
// Exit codes (via xemu $D6CF protocol):
//   0 = ran to completion; the dump says what the window held

#include <cbm.h>
#include <mapper.h>
#include <mega65.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

MAPPER_BANK_COUNT(1);

// Below $C000: the ROM mapped for the call covers $C000-$CFFF, so probes
// there would read ROM rather than what was written.
#define PROBE ((volatile uint8_t *)0xBF00)

#define WINDOW (*(volatile uint8_t *)0x2000)

// In ram_fixed, clear of the code, so its physical address is unambiguous --
// an address inside the window would depend on which bank is mapped.
#define LOAD_AT 0xB000
#define MARK 0x5A
#define BANK 1

// Gives prg-to-d81.py a bank file to put on the disk, which is then what the
// KERNAL call below loads.
RODATA_BANK(1) const uint8_t payload[16] = {0xB1};

int main(void) {
  set_bank(BANK);
  WINDOW = MARK;

  VICIV.ctrla |= VIC3_ROMC_MASK;
  cbm_k_setlfs(0, 8, 0);
  cbm_k_setnam("BANK1");
  cbm_k_load(0, (void *)LOAD_AT);
  VICIV.ctrla &= (unsigned char)~VIC3_ROMC_MASK;

  PROBE[0] = 0x33;                        // the call returned
  PROBE[1] = get_bank();                  // the shadow still claims BANK
  PROBE[2] = (WINDOW != MARK) ? 1 : 0;    // but the window moved

  resync_bank();
  PROBE[3] = WINDOW;                      // and is put back

  // The loader aims the KERNAL at a bank per load; unless it aims it back at
  // bank 0, this lands at $01B000 rather than $00B000.
  PROBE[4] = *(volatile uint8_t *)LOAD_AT;

  PROBE[5] = 0xA5;

  xemu_exit(0);
}
