// Hyppo directory service test — exercises mega65_h_findfile,
// mega65_h_opendir, mega65_h_readdir, mega65_h_closedir.
//
// Requires xemu with -hdosvirt -hdosdir containing FILE1.TXT and FILE2.TXT.
//
// Exit codes (line numbers via xemu_assert):
//   0 = all tests passed
//   non-zero = line number of failed assertion

#include <mega65.h>
#include <stdint.h>

#include "../mega65-common/xemu-test.h"

// readdir requires a 256-byte aligned buffer
static mega65_h_dirent dirent_buf __attribute__((aligned(256)));

int main(void) {
  // --- Test 1: findfile succeeds for existing file ---
  xemu_assert(mega65_h_setname("FILE1.TXT") == 0);
  xemu_assert(mega65_h_findfile() == 0);

  // --- Test 2: findfile fails for non-existent file ---
  xemu_assert(mega65_h_setname("NOFILE.BIN") == 0);
  xemu_assert(mega65_h_findfile() != 0);

  // --- Test 3: opendir + readdir + closedir ---
  uint8_t fd;
  xemu_assert(mega65_h_opendir(&fd) == 0);

  uint8_t count = 0;
  while (mega65_h_readdir(fd, &dirent_buf) == 0)
    count++;

  // HDOS dir should contain at least FILE1.TXT and FILE2.TXT
  xemu_assert(count >= 2);

  mega65_h_closedir(fd);

  xemu_exit(0);
}
