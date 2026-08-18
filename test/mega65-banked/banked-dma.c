// Test DMA copy triggered from banked code (MAP active).
//
// A function in bank 1 creates an enhanced F018B DMA job on the soft stack
// and copies 256 bytes between two ram_fixed buffers. Main verifies the
// copied data matches. This isolates the DMA-from-bank pattern that fails
// in complex programs.
//
// Exit codes (xemu $D6CF protocol):
//   0     = PASS
//   1     = source buffer not filled correctly (sanity check)
//   2     = destination not zeroed before DMA
//   3     = DMA copy produced wrong data (mismatch at dst[exit_detail])
//   0xFE  = banked function returned but _BANK_SHADOW is wrong

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

#define VIC_KEY   (*(volatile uint8_t *)0xD02F)
#define DMA_ENF018B (*(volatile uint8_t *)0xD703)
#define DMA_ADDRBNK (*(volatile uint8_t *)0xD702)
#define DMA_ADDRMSB (*(volatile uint8_t *)0xD701)
#define DMA_ETRIG   (*(volatile uint8_t *)0xD705)

// F018B enhanced DMA job: options prefix + DMA list.
// Must match the layout that trigger_dma() in dma.hpp produces.
struct DMAJob {
  uint8_t opt_enable;       // 0x0B = ENABLE_F018B
  uint8_t opt_src_mb_tag;   // 0x80 = SRC_ADDR_BITS
  uint8_t src_mb;           // source megabyte (bits [27:20])
  uint8_t opt_dst_mb_tag;   // 0x81 = DST_ADDR_BITS
  uint8_t dst_mb;           // dest megabyte (bits [27:20])
  uint8_t opt_dst_skip_tag; // 0x85 = DST_SKIP_RATE
  uint8_t dst_skip;         // 1
  uint8_t opt_end;          // 0x00 = end of options
  // F018B DMA list (12 bytes):
  uint8_t command;          // 0x00 = COPY
  uint16_t count;
  uint16_t src_addr;
  uint8_t src_bank;         // bits [19:16]
  uint16_t dst_addr;
  uint8_t dst_bank;         // bits [19:16]
  uint8_t cmd_msb;          // 0
  uint16_t modulo;          // 0
};

static void trigger_dma(struct DMAJob *job) {
  // Disable IRQs to prevent KERNAL IRQ from changing DMA revision.
  asm volatile("php\nsei" ::: "p");
  DMA_ENF018B = 1;
  DMA_ADDRBNK = 0;
  DMA_ADDRMSB = ((uint16_t)job) >> 8;
  DMA_ETRIG = ((uint16_t)job) & 0xff;
  asm volatile("plp" ::: "p");
}

static void unlock_viciv(void) {
  VIC_KEY = 0x47;
  VIC_KEY = 0x53;
}

// Buffers in ram_fixed BSS.
static uint8_t src_buf[256];
static uint8_t dst_buf[256];

// Bank 1 function: DMA copy src_buf -> dst_buf.
__attribute__((noinline, section(".bank_1")))
static void dma_copy_in_bank(void) {
  unlock_viciv();
  struct DMAJob job = {
      .opt_enable = 0x0B,
      .opt_src_mb_tag = 0x80,
      .src_mb = 0,
      .opt_dst_mb_tag = 0x81,
      .dst_mb = 0,
      .opt_dst_skip_tag = 0x85,
      .dst_skip = 1,
      .opt_end = 0x00,
      .command = 0x00, // COPY
      .count = 256,
      .src_addr = (uint16_t)src_buf,
      .src_bank = 0,
      .dst_addr = (uint16_t)dst_buf,
      .dst_bank = 0,
      .cmd_msb = 0,
      .modulo = 0,
  };
  trigger_dma(&job);
}

int main(void) {
  // Fill source with known pattern.
  for (uint16_t i = 0; i < 256; i++)
    src_buf[i] = (uint8_t)(i ^ 0xA5);

  // Sanity check source.
  if (src_buf[0] != 0xA5 || src_buf[1] != 0xA4)
    xemu_exit(1);

  // Clear destination.
  for (uint16_t i = 0; i < 256; i++)
    dst_buf[i] = 0;
  if (dst_buf[0] != 0)
    xemu_exit(2);

  // Call bank 1 DMA function.
  banked_call(1, dma_copy_in_bank);

  // Verify copied data.
  for (uint16_t i = 0; i < 256; i++) {
    if (dst_buf[i] != (uint8_t)(i ^ 0xA5))
      xemu_exit(3);
  }

  xemu_exit(0); // PASS
}
