// A KERNAL disk call must not silently move the banked window.
//
// The internal CBDOS runs under its own MAP: Get_DOS installs it and
// Leave_DOS restores from `current_map` (mega65-rom/system.src, "map back to
// kernal"), which holds the KERNAL's idea of the mapping and not whatever the
// caller had set.  A program that had bank 3 in the window before the call
// therefore comes back with something else in it, while _BANK_SHADOW -- and
// so get_bank() -- still reports bank 3.
//
// This records whether that actually happens, rather than assuming it: the
// answer decides whether the platform needs a way to put the map back.
//
// Exit codes (via xemu $D6CF protocol):
//   0 = ran to completion; the dump carries the findings
//
// Only read-side disk I/O here.  KERNAL write I/O (BSOUT/CHKOUT) writes to
// $D6CF, which under xemu -testing is the exit register -- see test/mega65/disk.c.

#include <cbm.h>
#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

// $FB-$FE are unallocated by the MEGA65 KERNAL and outside the compiler's
// zero page ($02-$8F).
#define PROBE ((volatile uint8_t *)0x00FB)

// First byte of the banked window.
#define WINDOW (*(volatile uint8_t *)0x2000)

#define MARK 0x5A
#define BANK 0

int main(void) {
  set_bank(BANK);
  WINDOW = MARK;

  // Touch the drive through the KERNAL. Whether the file opens is beside the
  // point; reaching CBDOS at all is what exercises Get_DOS/Leave_DOS.
  cbm_k_setlfs(2, 8, 0);
  cbm_k_setnam("AUTOBOOT.C65");
  cbm_k_open();
  cbm_k_close(2);

  // Did the window move out from under us?
  PROBE[0] = (WINDOW != MARK) ? 1 : 0;
  // ...while the shadow carries on claiming otherwise.
  PROBE[1] = get_bank();

  PROBE[2] = 0xA5;
  xemu_exit(0);
}
