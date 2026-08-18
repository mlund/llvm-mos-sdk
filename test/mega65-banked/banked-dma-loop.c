// Test repeated DMA from alternating banks, mimicking a game main loop.
//
// Bank 1 DMA-copies src1 -> dst, bank 2 DMA-copies src2 -> dst. The main
// loop alternates 500 times, verifying the result after each call. This
// catches IRQ interference, _BANK_SHADOW corruption, and soft stack issues.
//
// Exit codes (xemu $D6CF protocol):
//   0     = PASS (500 iterations completed)
//   1     = mismatch after bank 1 DMA
//   2     = mismatch after bank 2 DMA

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

#define VIC_KEY   (*(volatile uint8_t *)0xD02F)
#define DMA_ENF018B (*(volatile uint8_t *)0xD703)
#define DMA_ADDRBNK (*(volatile uint8_t *)0xD702)
#define DMA_ADDRMSB (*(volatile uint8_t *)0xD701)
#define DMA_ETRIG   (*(volatile uint8_t *)0xD705)

struct DMAJob {
  uint8_t opt_enable;
  uint8_t opt_src_mb_tag;
  uint8_t src_mb;
  uint8_t opt_dst_mb_tag;
  uint8_t dst_mb;
  uint8_t opt_dst_skip_tag;
  uint8_t dst_skip;
  uint8_t opt_end;
  uint8_t command;
  uint16_t count;
  uint16_t src_addr;
  uint8_t src_bank;
  uint16_t dst_addr;
  uint8_t dst_bank;
  uint8_t cmd_msb;
  uint16_t modulo;
};

static void trigger_dma(struct DMAJob *job) {
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

static uint8_t src1[128];
static uint8_t src2[128];
static uint8_t dst[128];

// Bank 1: DMA copy src1 -> dst.
__attribute__((noinline, section(".bank_1")))
static void dma_from_bank1(void) {
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
      .command = 0x00,
      .count = 128,
      .src_addr = (uint16_t)src1,
      .src_bank = 0,
      .dst_addr = (uint16_t)dst,
      .dst_bank = 0,
      .cmd_msb = 0,
      .modulo = 0,
  };
  trigger_dma(&job);
}

// Bank 2: DMA copy src2 -> dst.
__attribute__((noinline, section(".bank_2")))
static void dma_from_bank2(void) {
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
      .command = 0x00,
      .count = 128,
      .src_addr = (uint16_t)src2,
      .src_bank = 0,
      .dst_addr = (uint16_t)dst,
      .dst_bank = 0,
      .cmd_msb = 0,
      .modulo = 0,
  };
  trigger_dma(&job);
}

static uint8_t verify(const uint8_t *expected) {
  for (uint8_t i = 0; i < 128; i++) {
    if (dst[i] != expected[i])
      return 0;
  }
  return 1;
}

int main(void) {
  // Fill source buffers with distinct patterns.
  for (uint8_t i = 0; i < 128; i++) {
    src1[i] = i ^ 0xAA;
    src2[i] = (128 - i) ^ 0x55;
  }

  for (uint16_t iter = 0; iter < 500; iter++) {
    // Bank 1 DMA, then verify.
    banked_call(1, dma_from_bank1);
    if (!verify(src1))
      xemu_exit(1);

    // Bank 2 DMA, then verify.
    banked_call(2, dma_from_bank2);
    if (!verify(src2))
      xemu_exit(2);
  }

  xemu_exit(0); // PASS
}
