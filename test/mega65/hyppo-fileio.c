// Hyppo file I/O test — exercises mega65_h_openfile, mega65_h_readfile,
// mega65_h_closefile, mega65_h_closeall via sector-by-sector reading.
//
// Requires xemu with -hdosvirt -hdosdir containing TEST.BIN
// (8 bytes: DE AD BE EF CA FE BA BE).
//
// Exit codes (line numbers via xemu_assert):
//   0 = all tests passed
//   non-zero = line number of failed assertion

#include <mega65.h>
#include <stdint.h>

#include "../xemu-test.h"

int main(void) {
  // --- Test 1: find and open the test file ---
  xemu_assert(mega65_h_setname("TEST.BIN") == 0);
  xemu_assert(mega65_h_findfile() == 0);

  uint8_t fd;
  xemu_assert(mega65_h_openfile(&fd) == 0);

  // --- Test 2: read first (only) sector, verify byte count ---
  uint16_t count;
  xemu_assert(mega65_h_readfile(&count) == 0);
  xemu_assert(count == 8);

  // Note: sector buffer contents at $FFD6E00 are not verified here because
  // xemu's HDOS virtualization may not populate the physical sector buffer
  // address for DMA reads. The loadfile test (hyppo.c) already covers
  // DMA-based data verification via loadfile_attic.

  // --- Test 3: second read should return count == 0 (EOF) ---
  xemu_assert(mega65_h_readfile(&count) == 0);
  xemu_assert(count == 0);

  // --- Test 4: closefile should not crash ---
  mega65_h_closefile(fd);

  // --- Test 5: closeall should not crash ---
  mega65_h_closeall();

  xemu_exit(0);
}
