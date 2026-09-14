# mega65-banked

A banked MEGA65 target with the KERNAL still available, for programs larger
than 64 KB. Banks are separate files on a D81 image, loaded at startup, so the
`.prg` alone will not run.

Use `mega65-banked-sd` instead if the program needs neither BASIC nor the
KERNAL and can load from the SD card.

## Memory map

| Range | Contents |
|---|---|
| `$0000-$01FF` | Zero page, hardware stack |
| `$0200-$1FFF` | KERNAL workspace |
| `$2000-$7FFF` | **bank window** — one bank at a time, 24 KB |
| `$8000-$CFFF` | **fixed region** — always visible, 20 KB |
| `$D000-$DFFF` | I/O |
| `$E000-$FFFF` | KERNAL |

The soft stack starts at `$D000` and grows down into the fixed region. There
is no overflow detection.

## Banks

| Bank | Address | Memory |
|---|---|---|
| 0 | `$02000` | The default window; needs no loading |
| 1-2 | `$12000`, `$18000` | Chip RAM |
| 3-7 | `$40800` … `$58800` | Chip RAM, above the ROMs |
| 8-15 | `$8000800` … `$8036800` | Attic RAM |

Chip RAM is all of `$00000-$5FFFF`, ROMs included: full speed, and fetched
from directly by VIC-IV, so any of banks 0-7 can hold graphics.

Banks 1-2 start at `$12000` rather than `$10000`: `$10000-$11FFF` is the C65
DOS work area, mapped whenever the KERNAL touches a disk, so a bank placed
there loses those bytes on the next disk call.

Banks 3-15 sit `$800` into a 64 KB page, so KERNAL LOAD never sees a zero
address high byte, which makes it corrupt the destination.

Attic RAM is about ten times slower, out of reach of VIC-IV and audio DMA,
and absent on Nexys A7 boards. Use it for tables and logic, not graphics or
audio.

## Using banks

```c
#include <mapper.h>

MAPPER_BANK_COUNT(2);          // highest bank used

RODATA_BANK(1) const uint8_t table[256] = { ... };

CODE_BANK(1) void compute(void) { result = table[index]; }

int main(void) {
  banked_call(1, compute);
}
```

`MAPPER_BANK_COUNT(n)` keeps banks above `n` out of the image. Without it all
15 are reserved, around 414 KB of mostly empty space read off the disk at boot.

`CODE_BANK` supplies `noinline`, without which a function may be inlined back
into the fixed region leaving the bank empty. `RODATA_BANK` supplies `used`
and `retain`, without which unreferenced data is discarded.

Keep a table in the same bank as the code reading it, so one call covers both.

### API

| Function | Purpose |
|---|---|
| `banked_call(bank, fn)` | Map `bank`, call `fn`, restore the previous bank |
| `banked_call_r(bank, fn, ...)` | As above, with arguments and a return value |
| `banked_call_v(bank, fn, ...)` | As above, for `void` functions |
| `get_bank()` | The currently mapped bank |
| `set_bank(bank)` | Map a bank directly; prefer `banked_call` |

`bank` is masked to its low four bits, so 0x11 selects bank 1.

The `_r` and `_v` forms switch the bank around a direct call, so the compiler
marshals the real signature. The caller must be in the fixed region: from a
bank the switch would unmap it, and the link fails. Up to eight arguments are
evaluated before the switch.

## Rules

**`banked_call` takes `void(void)` only.** For arguments or a return value, use
`banked_call_r` or `banked_call_v`.

**Bank data is readable only while its bank is mapped.** Anything shared or
long-lived belongs in the fixed region.

**Everything not given a bank goes in the 20 KB fixed region** — code, string
literals, static variables, the soft stack. This is the usual limit a program
hits first.

**A bank may call another bank.** `banked_call` is in the fixed region, so the
caller's bank is back before control returns to it.

**Do not pass `-T`.** A supplementary linker script suppresses the platform's
`OUTPUT_FORMAT`, producing an ELF instead of the flat image the disk build
needs. For large buffers, use a fixed address instead:

```c
static int16_t *const buffer = (int16_t *)0x2100;
```

**The C65 ROM clears `$200E-$7FFF` before your code runs.** Only the BASIC
header at `$2001-$200D` survives. The fixed region is untouched.

**Re-unlock VIC-IV after disk I/O.** Any KERNAL disk operation resets
`VICIV.key` to VIC-II mode, hiding the VIC-IV registers:

```c
VICIV.key = VIC4_KEY_VICIV_A;
VICIV.key = VIC4_KEY_VICIV_B;
```

**KERNAL disk I/O needs the C65 interface ROM at `$C000-$CFFF`, and it is not
mapped.** Startup unmaps it to extend the fixed region, so a `cbm_k_load()`
from your own code hangs. Map it for the call:

```c
VICIV.ctrla |= VIC3_ROMC_MASK;
mega65_k_setbnk(0, 0);           // startup aims LOAD at bank 0; say which
cbm_k_setlfs(0, 8, 0);
cbm_k_setnam("DATA");
cbm_k_load(0, (void *)0xB000);   // in the fixed region, not the window
VICIV.ctrla &= (unsigned char)~VIC3_ROMC_MASK;
```

While it is mapped, reads from `$C000-$CFFF` give ROM rather than what you put
there; writes still reach the RAM underneath. Keep anything the call needs
below `$C000`. A destination inside `$2000-$7FFF` lands in whichever bank is
mapped, so prefer the fixed region unless that is what you meant.

**KERNAL disk I/O leaves a different bank mapped.** It installs its own
mapping and restores the KERNAL's, not yours. Hypervisor calls are unaffected:
the map is saved and restored in hardware.

**Some VIC registers are hot.** Writing `VICIV.ctrlb` or a VIC-II register
makes VIC-IV recompute its screen, character and colour pointers. If you set
those yourself, turn that off first:

```c
VICIV.sdbdrwd_msb &= ~VIC4_HOTREG_MASK;
```

`VICIV.ctrla`, which carries the ROM banking bits, is not one of them.

## Interrupts

Bank switching leaves the interrupt flag alone, so an interrupt can arrive at
any point, including with a bank mapped. Two rules follow:

- **Handlers live in the fixed region.** That is the default placement, so
  simply never give a handler `CODE_BANK()`.
- **Handlers must not touch `$2000-$7FFF`.** A handler runs with whichever
  bank the interrupted code had mapped and cannot find out which.

Neither is checked. Breaking them reads whichever bank was live, which looks
like intermittent corruption.

See `examples/mega65-banked/banked-irq.cc`.

## Building

```sh
mos-mega65-banked-clang -Os -o game.prg game.c
python3 prg-to-mega65.py game.prg . game
```

This writes `game-main.prg`, one file per non-empty bank, and `game.d81` with
the main program as `AUTOBOOT.C65`. Run the disk image, not the `.prg`.

## Changing the bank layout

Define `MAPPER_BANK_n` to move bank *n*, the same in every file: with `-D`, or
in a header passed with `-include`.

```sh
mos-mega65-banked-clang -DMAPPER_BANK_2=0x8040800 -Os -o game.prg game.c
```

`MAPPER_WINDOW_KB` set to 16 or 8 shrinks every bank to that size. Autoboot
clears the freed top of the window before your code runs, so the soft stack
moves there and leaves the fixed region to code and data.

`MAPPER_BANK_n_KB` makes one bank smaller than the window. While it is mapped,
the rest of the window is the window's own RAM: `WINDOW_TAIL` places
uninitialised data there, readable with bank 0 or a smallest bank mapped.

`<mapper.h>` checks each address at compile time; the converter rejects
overlapping banks and files that disagree. A bank's address high byte must not be `$00`, and the C65 ROMs at
`$20000-$3FFFF` stay write-protected.
