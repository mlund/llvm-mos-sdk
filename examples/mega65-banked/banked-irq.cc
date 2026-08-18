// Interrupt handling while banks are being switched.
//
// The map is global state. An interrupt arrives whenever it likes, so a
// handler runs with whichever bank the interrupted code happened to have
// mapped -- and has no way to find out which. Two rules follow:
//
//   1. The handler lives in ram_fixed. That is the default placement, so the
//      rule is really "never give a handler CODE_BANK()".
//   2. The handler must not touch $2000-$7FFF. Everything it reads or writes
//      has to be in ram_fixed, zero page, or I/O.
//
// Nothing enforces either one; breaking them gives you a handler that reads
// whichever bank was live, which looks like intermittent corruption.
//
// Bank switching itself leaves interrupts alone: MAP inhibits them through a
// dedicated signal rather than the I flag (gs4510.vhdl), so set_bank() and
// banked_call() return with the caller's interrupt state intact and an
// interrupt can land at any point between them -- including with a bank
// mapped. That is what this demonstrates: the border keeps changing while the
// main loop is off in banks 1 and 2.
//
// On the attribute: llvm-mos has __attribute__((interrupt)) and
// ((interrupt_norecurse)), which save registers and return with RTI. Neither
// fits here, because $0314 is not the hardware vector -- the KERNAL has
// already saved the registers and expects the chain to end in its own restore
// and RTI, so an RTI of our own would unbalance the stack. Hence the shim
// below is assembly, kept small enough to touch no imaginary registers.
//
// Once you own the hardware vector -- a platform with the KERNAL mapped out --
// interrupt_norecurse on a plain C function is the right answer, and much
// nicer. Prefer it to ((interrupt)) when the source is masked while handling,
// since it keeps static stack allocation instead of pushing callees onto the
// soft stack.

#include <mapper.h>
#include <mega65.h>
#include <stdint.h>

MAPPER_BANK_COUNT(2);

// The KERNAL's indirect IRQ vector (iirq, mega65-rom/system.src).
#define IIRQ (*(void (**)(void))0x0314)

// In ram_fixed, so the handler can reach them whatever is mapped.
__attribute__((used, retain)) volatile uint8_t ticks;
__attribute__((used, retain)) void (*irq_chain)(void);

// Bump the counter, tint the border, then hand on to the KERNAL so its own
// timekeeping and keyboard scanning still happen. Deliberately touches only
// ram_fixed and I/O -- never the banked window.
asm(".section .text.irq_shim,\"axR\",@progbits\n"
    ".globl irq_shim\n"
    "irq_shim:\n"
    "  inc ticks\n"
    "  lda ticks\n"
    "  lsr\n"
    "  lsr\n"
    "  sta $d020\n"
    "  jmp (irq_chain)\n");
extern "C" void irq_shim(void); // defined by the asm above, so unmangled

// Bank 1: a table and the code that reads it, kept together so one
// banked_call covers both.
RODATA_BANK(1) const uint8_t shades[8] = {6, 14, 3, 13, 1, 13, 3, 14};

__attribute__((used, retain)) volatile uint8_t colour_index;
__attribute__((used, retain)) volatile uint8_t screen_colour;

CODE_BANK(1) void pick_colour() {
  screen_colour = shades[colour_index & 7];
  ++colour_index;
}

// Bank 2: applies what bank 1 chose. A separate bank on purpose -- bank 1 is
// not mapped while this runs, which is why the shared state lives in
// ram_fixed rather than in either bank.
CODE_BANK(2) void apply_colour() { VICIV.screencol = screen_colour; }

int main() {
  // Install the shim with interrupts masked, so the vector is never half
  // written when one arrives.
  asm volatile("sei" ::: "p");
  irq_chain = IIRQ;
  IIRQ = irq_shim;
  asm volatile("cli" ::: "p");

  // Interrupts keep arriving throughout, including while a bank is mapped.
  for (;;) {
    banked_call(1, pick_colour);
    banked_call(2, apply_colour);

    // Slow the cycle down enough to see it.
    for (uint16_t wait = 0; wait < 20000; ++wait)
      asm volatile("");
  }
}
