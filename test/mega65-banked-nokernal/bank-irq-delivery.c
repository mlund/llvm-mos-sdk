// Interrupts arrive while a bank is mapped, and the bank survives them.
//
// bank-interrupts.c shows the I flag survives a bank switch, which says
// interrupts are permitted, not that any are delivered. There is no KERNAL
// here to chain through: $FFFA and $FFFE are RAM the program owns, so the
// handler goes straight into the vector.
//
// The wait is bounded, so a handler that never runs fails an assertion with a
// line number rather than hanging until the test times out.

#include <mapper.h>
#include <mega65.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

MAPPER_BANK_COUNT(1);

#define IRQ_VECTOR (*(volatile uint16_t *)0xfffe)
#define VIC_IRQ_RASTER 0x01

// .bss, so the handler and the banked code both reach them whatever is mapped.
static volatile uint8_t irq_count;
static volatile uint8_t bank_mark;
static volatile uint8_t idx;

RODATA_BANK(1) static const uint8_t mark[2] = {0xa5, 0x5a};

// Fixed region by default, and touches nothing in $2000-$7FFF: it runs with
// whichever bank the interrupted code had mapped and cannot know which.
__attribute__((interrupt_norecurse)) static void on_raster(void) {
  VICII.irr = VIC_IRQ_RASTER;
  ++irq_count;
}

CODE_BANK(1) static void wait_in_bank(void) {
  // Count from here, or interrupts taken before the bank was mapped would
  // satisfy the loop and prove nothing about delivery under a mapped bank.
  irq_count = 0;
  for (uint32_t i = 0; i < 500000 && irq_count < 2; ++i)
    ;
  // Read after the interrupts, so the value says bank 1 is still the window.
  bank_mark = mark[idx];
}

int main(void) {
  idx = 1;

  IRQ_VECTOR = (uint16_t)(uintptr_t) & on_raster;
  VICII.ctrl1 &= 0x7f; // raster compare is 9 bits; keep the line below 256
  VICII.rasterline = 100;
  VICII.irr = 0x0f; // drop anything pending before unmasking
  VICII.imr = VIC_IRQ_RASTER;

  asm volatile("cli" ::: "p");
  banked_call(1, wait_in_bank);
  asm volatile("sei" ::: "p");
  VICII.imr = 0;

  xemu_assert(irq_count >= 2);
  xemu_assert(bank_mark == 0x5a);

  xemu_exit(0);
}
