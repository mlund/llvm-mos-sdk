// MEGA65 KERNAL wrapper test — exercises the C-callable KERNAL wrappers
// added in the mega65-kernal-setbnk branch.
//
// Tests run under xemu in headless mode. Each test uses a unique exit code
// so failures pinpoint the exact broken wrapper. All tests use KERNAL
// functions that work without disk hardware (no FDC/D81 dependency).
//
// Note: disk I/O tests are not included because xemu's FDC SWAP
// emulation detaches D81 images during C65 boot, before user code runs.

#include <cbm.h>
#include <mega65.h>
#include <stdint.h>

#include "../mega65-common/xemu-test.h"

enum {
  EXIT_OK = 0,

  EXIT_SCRORG_WIDTH = 1,
  EXIT_SCRORG_HEIGHT = 2,

  EXIT_PLOT_LINE = 3,
  EXIT_PLOT_COL = 4,

  EXIT_GETIO_IN = 5,
  EXIT_GETIO_OUT = 6,

  EXIT_RDTIM_HOURS = 7,
  EXIT_RDTIM_MINUTES = 8,
  EXIT_RDTIM_SECONDS = 9,

  EXIT_GETLFS_LA = 10,
  EXIT_GETLFS_FA = 11,
  EXIT_GETLFS_SA = 12,

  EXIT_LKUPLA_GHOST = 13,
  EXIT_READST = 14,
};

int main(void) {
  // Unlock VIC-IV registers and enable C65 ROMs for KERNAL extensions.
  // The startup code (unmap-basic.S) sets ctrla=0x44 (no $C000 ROM).
  // We need the $C000 ROM for MEGA65 KERNAL extensions.
  VICIV.key = 0x47;
  VICIV.key = 0x53;
  VICIV.ctrla = VIC3_PAL_MASK | VIC3_ROMC_MASK | VIC3_CROM9_MASK;

  // --- Test SCRORG: screen dimensions should be valid ---
  // MEGA65 KERNAL SCREEN returns max indices (0-based), not counts.
  mega65_screen_info_t scr = mega65_k_scrorg();
  // C65 boots in 80-column mode (max_col=79) or 40-column (max_col=39)
  if (scr.width != 79 && scr.width != 39)
    xemu_exit(EXIT_SCRORG_WIDTH);
  if (scr.height != 24) // 25 rows -> max_row=24
    xemu_exit(EXIT_SCRORG_HEIGHT);

  // --- Test PLOT: set cursor position, read it back ---
  mega65_k_plot_set(10, 20);
  unsigned char line, col;
  mega65_k_plot_get(&line, &col);
  if (line != 10)
    xemu_exit(EXIT_PLOT_LINE);
  if (col != 20)
    xemu_exit(EXIT_PLOT_COL);

  // --- Test GETIO: default I/O devices after boot ---
  unsigned char in_dev, out_dev;
  mega65_k_getio(&in_dev, &out_dev);
  if (in_dev != 0) // 0 = keyboard
    xemu_exit(EXIT_GETIO_IN);
  if (out_dev != 3) // 3 = screen
    xemu_exit(EXIT_GETIO_OUT);

  // --- Test SETTIM/RDTIM: set TOD clock and read back ---
  // BCD values: 0x12 = 12, 0x30 = 30, 0x45 = 45
  mega65_k_settim(0x12, 0x30, 0x45, 0x00);
  mega65_tod_t tod = mega65_k_rdtim();
  if (tod.hours != 0x12)
    xemu_exit(EXIT_RDTIM_HOURS);
  if (tod.minutes != 0x30)
    xemu_exit(EXIT_RDTIM_MINUTES);
  // Seconds may have ticked; accept 0x45 or 0x46
  if (tod.seconds != 0x45 && tod.seconds != 0x46)
    xemu_exit(EXIT_RDTIM_SECONDS);

  // --- Test SETLFS/GETLFS: set file params and read back ---
  cbm_k_setlfs(5, 8, 2);
  unsigned char la, fa, sa;
  mega65_k_getlfs(&la, &fa, &sa);
  if (la != 5)
    xemu_exit(EXIT_GETLFS_LA);
  if (fa != 8)
    xemu_exit(EXIT_GETLFS_FA);
  if (sa != 2)
    xemu_exit(EXIT_GETLFS_SA);

  // --- Test LKUPLA: search for non-existent logical file ---
  unsigned char lk_fa, lk_sa;
  if (mega65_k_lkupla(99, &lk_fa, &lk_sa) == 0)
    xemu_exit(EXIT_LKUPLA_GHOST); // should NOT be found

  // --- Test READST: status should be 0 after fresh boot ---
  if (cbm_k_readst() != 0)
    xemu_exit(EXIT_READST);

  // Restore startup ROM banking (C65 ROMs unmapped for max RAM).
  VICIV.ctrla = VIC3_PAL_MASK | VIC3_CROM9_MASK;

  xemu_exit(EXIT_OK);
}
