// Interrupts must actually be delivered while a bank is mapped.
//
// bank-interrupts.c shows the I flag survives a bank switch, which is
// necessary but not sufficient: it says interrupts are permitted, not that
// any arrive. Nothing proved delivery, and the obvious probe -- the KERNAL
// clock -- cannot: MEGA65 keeps a BCD TOD clock driven by the CIA, which
// ticks whether or not the CPU is taking interrupts.
//
// So count them. A shim on the KERNAL's indirect IRQ vector (iirq, $0314 --
// mega65-rom/system.src) bumps a counter and chains to whatever was there,
// leaving the KERNAL's own handling intact.
//
// The shim only touches ram_fixed, which is the rule banked code must follow:
// a handler runs with whichever bank the interrupted code had mapped and
// cannot know which, so it must not touch $2000-$7FFF. This test also puts
// that rule under load -- interrupts arrive while bank 1 is mapped, so the
// KERNAL's handler runs with a non-zero bank in the window.
//
// Exit codes (via xemu $D6CF protocol):
//   0 = ran to completion; the dump says whether interrupts arrived

#include <mapper.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

MAPPER_BANK_COUNT(1);

#define PROBE ((volatile uint8_t *)0xC000)

// The KERNAL's indirect IRQ vector.
#define IIRQ (*(void (**)(void)) 0x0314)

// Referenced only from the asm below, so the compiler must be told to keep
// them; they are in ram_fixed and safe to touch from an interrupt.
__attribute__((used, retain)) volatile uint8_t irq_count;
__attribute__((used, retain)) void (*irq_chain)(void);

// Not an interrupt function in the C sense: the KERNAL has already saved the
// registers by the time it dispatches through $0314, and expects the chain to
// end in its own restore/RTI rather than ours.
asm(".section .text.irq_shim,\"axR\",@progbits\n"
    ".globl irq_shim\n"
    "irq_shim:\n"
    "  inc irq_count\n"
    "  jmp (irq_chain)\n");
extern void irq_shim(void);

CODE_BANK(1) static void in_bank(void) { asm volatile(""); }

// Spin until the counter moves, or give up. Bounded rather than timed, so a
// failure returns and lets the dump be written instead of hanging until
// CTest's timeout kills xemu before it can say anything.
static uint8_t interrupt_arrived(void) {
  uint8_t start = irq_count;
  for (uint16_t outer = 0; outer < 2000; ++outer)
    for (uint16_t inner = 0; inner < 1000; ++inner)
      if (irq_count != start)
        return 1;
  return 0;
}

int main(void) {
  asm volatile("sei" ::: "p");
  irq_chain = IIRQ;
  IIRQ = irq_shim;
  asm volatile("cli" ::: "p");

  // With a bank mapped: the interrupt lands while $2000-$7FFF is not bank 0.
  set_bank(1);
  PROBE[0] = interrupt_arrived();

  // And still delivered after going through the trampoline repeatedly.
  set_bank(0);
  for (uint8_t i = 0; i < 200; ++i)
    banked_call(1, in_bank);
  PROBE[1] = interrupt_arrived();

  // Put the vector back before the exit sequence runs.
  asm volatile("sei" ::: "p");
  IIRQ = irq_chain;
  asm volatile("cli" ::: "p");

  PROBE[2] = 0xA5;
  xemu_exit(0);
}
