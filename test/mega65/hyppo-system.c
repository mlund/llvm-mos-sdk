// Hyppo system and drive service test — exercises mega65_h_getversion,
// mega65_h_geterrorcode, mega65_h_getcurrentdrive, mega65_h_getdefaultdrive.
//
// Pure -prg injection test (no filesystem needed).
//
// Exit codes (line numbers via xemu_assert):
//   0 = all tests passed
//   non-zero = line number of failed assertion

#include <mega65.h>
#include <stdint.h>

#include "../xemu-test.h"

int main(void) {
  // --- Test 1: getversion returns nonzero Hyppo version ---
  mega65_h_version ver;
  mega65_h_getversion(&ver);
  xemu_assert(ver.hyppo_major != 0);

  // --- Test 2: geterrorcode does not crash ---
  // After a successful getversion there is no meaningful error code,
  // but calling it should not hang or crash.
  (void)mega65_h_geterrorcode();

  // --- Test 3: getcurrentdrive returns 0 (xemu default) ---
  xemu_assert(mega65_h_getcurrentdrive() == 0);

  // --- Test 4: getdefaultdrive returns 0 (xemu default) ---
  xemu_assert(mega65_h_getdefaultdrive() == 0);

  xemu_exit(0);
}
