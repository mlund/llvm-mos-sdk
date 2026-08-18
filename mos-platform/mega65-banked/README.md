# mega65-banked

MEGA65 target with a 24 KB banked window, for programs larger than 64 KB.

Requires the 45GS02 (`-mcpu=mos45gs02`, set by the target) and a D81 disk
image: banks are separate files loaded at startup, so the `.prg` alone will
not run.

## Memory map

| Range | Size | Contents |
|---|---|---|
| `$0000-$01FF` | 512 B | Zero page, hardware stack |
| `$0200-$1FFF` | 7.5 KB | KERNAL workspace |
| `$2000-$7FFF` | 24 KB | **Banked window** — one bank at a time |
| `$8000-$CFFF` | 20 KB | **Fixed region** — always visible |
| `$D000-$DFFF` | 4 KB | I/O |
| `$E000-$FFFF` | 8 KB | KERNAL |

The soft stack starts at `$D000` and grows down into the fixed region. There
is no overflow detection.

## Banks

| Bank | Address | Memory |
|---|---|---|
| 0 | `$02000` | Chip RAM. The default window; needs no loading |
| 1-2 | `$10800`, `$16800` | Chip RAM |
| 3-7 | `$40800` … `$58800` | Fast RAM |
| 8-15 | `$8000800` … `$8036800` | Attic RAM |

Attic RAM is about ten times slower, is invisible to VIC-IV and SID, and is
absent on Nexys A7 boards. Use it for tables and logic, not graphics or audio.

## Using banks

```c
#include <mapper.h>

MAPPER_BANK_COUNT(2);          // highest bank used

RODATA_BANK(1) const uint8_t table[256] = { ... };

CODE_BANK(1) void compute() { result = table[index]; }

int main() {
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
| `get_bank()` | The currently mapped bank |
| `set_bank(bank)` | Map a bank directly; prefer `banked_call` |

`bank_id` is masked to its low four bits, so 0x11 selects bank 1.

## Rules

**`banked_call` takes `void(void)` only.** No arguments, no return value. Pass
values through variables in the fixed region.

**A bank may not call another bank directly.** Return to the fixed region
first, then make the next `banked_call`. Nesting that way is safe: the
previous bank is restored before control returns to the outer one.

**Bank data is readable only while its bank is mapped.** Anything shared or
long-lived belongs in the fixed region.

**Everything not given a bank goes in the 20 KB fixed region** — code, string
literals, static variables, vtables, the soft stack. This is the usual limit
a program hits first.

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
sh make-d81.sh game.prg . game "$(command -v c1541)"
```

This writes `game-main.prg`, one file per non-empty bank, and `game.d81` with
the main program as `AUTOBOOT.C65`. Run the disk image, not the `.prg`.

In CMake, `add_banked_example(name source)` does the same.

## Changing the bank layout

Four files state where banks live, in different forms:

| File | Holds |
|---|---|
| `mapper.s` | MAP offsets and megabyte bytes |
| `load-banks-kernal.S` | Load addresses for the disk loader |
| `mapper.h` | `BANK_PHYS_BASE_n` |
| `_ram-banked.ld` | Section addresses |

`test/mega65-banked/check-bank-tables.py` compares the first three and needs
no emulator. Run it after any edit.

`_ram-banked.ld` is generated by `misc/generate-banked-sections.lua`.
