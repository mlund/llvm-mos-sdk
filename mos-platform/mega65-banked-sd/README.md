# mega65-banked-sd

A banked MEGA65 target with BASIC and the KERNAL mapped out. Banks and assets
are plain files on the SD card, loaded through Hyppo.

Use `mega65-banked` instead if the program must load from a D81 image or call
the KERNAL.

## Memory map

| Range | Contents |
|---|---|
| `$0002-$0021` | imaginary registers |
| `$0022-$00FF` | compiler zero page, 222 bytes |
| `$0100-$01FF` | hardware stack |
| `$0200-$02FF` | Hyppo filename page |
| `$0300-$1FFF` | `RAM_LOW`, uninitialised, 7,424 B |
| `$2000-$7FFF` | **bank window** — one bank at a time, 24 KB |
| `$8000-$CFFF` | **fixed region** — always visible, 20 KB |
| `$D000-$DFFF` | I/O |
| `$E000-$FFF9` | `.data`, `.bss`, `.noinit`, and the soft stack growing down |
| `$FFFA-$FFFF` | interrupt vectors |

Only the window is remapped, so everything from `$8000` up stays where the
linker put it. About 19.5 KB of the fixed region is left after the runtime.

## Banks

| Bank | Address | Memory |
|---|---|---|
| 0 | `$02000` | The default window; needs no loading |
| 1-2 | `$12000`, `$18000` | Chip RAM |
| 3-7 | `$20000` … `$38000` | Where the C65 ROMs would be |
| 8-12 | `$40000` … `$58000` | Fast RAM |
| 13-15 | `$8000000` … `$800C000` | Attic RAM |

Bank 0 is the window unmapped, so `.bank_0` content ships inside the main
program rather than a bank file.

Banks 1-12 are full speed and reachable by VIC-IV, which fetches anything
below `$60000`, so any of them can hold a screen, a charset or sprite data.
Banks 3-7 sit where the C65 ROMs would be, including the character generator
at `$2D000`; startup lifts the write protection there. Bring your own charset,
as there is no longer one to fall back on.

Attic RAM is about ten times slower, is invisible to VIC-IV and SID, and is
absent on boards without it. Use it for tables and logic, not graphics or
audio.

## Using banks

```c
#include <mapper.h>

MAPPER_BANK_COUNT(2);          // highest bank used

RODATA_BANK(1) const unsigned char sine[256] = { ... };

CODE_BANK(1) void animate(void) { ... }

int main(void) {
  banked_call(1, animate);
}
```

`MAPPER_BANK_COUNT(n)` keeps banks above `n` out of the image. Without it all
15 are reserved, nearly all of it empty space read off the card at boot.

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

```c
int n = banked_call_r(1, measure, text, len);
banked_call_v(1, draw, x, y);
```

The `_r` and `_v` forms switch the bank around a direct call, so the compiler
marshals the real signature. Two rules, neither diagnosed:

- The caller must be in the fixed region. From a bank the switch unmaps the
  caller mid-call.
- Arguments are evaluated with the target bank mapped, so none may read the
  outgoing bank.

## Rules

**Bank data is readable only while its bank is mapped.** Anything shared or
long-lived belongs in the fixed region.

**Everything not given a bank goes in the 20 KB fixed region** — code, string
literals, static variables, the soft stack. This is the usual limit a program
hits first.

**A bank may call another bank.** `banked_call` is in the fixed region, so the
caller's bank is back before control returns to it.

**Do not pass `-T`.** A supplementary linker script suppresses the platform's
`OUTPUT_FORMAT`, producing an ELF instead of the flat image the split needs.
For large buffers, use a fixed address instead:

```c
static int16_t *const buffer = (int16_t *)0x2100;
```

**No KERNAL.** The `cbm_k_*` and `mega65_k_*` wrappers link but jump into RAM.
The `mega65_h_*` Hyppo wrappers are the ones that work.

**Nothing writes to the screen for you.** There is no `CHROUT`, no character
ROM and no BASIC. A program brings its own charset, or draws without one.

**The card is the only storage, both ways.** Read with `mega65_h_setname` and
`mega65_h_loadfile`, or `mega65_h_loadfile_attic` above `$8000000`; write with
`mega65_h_mkfile` then `mega65_h_writefile`. `mkfile` takes 8.3 names only and
allocates contiguously, so create a save file at full size once and rewrite it
in place.

**Hyppo needs a page below `$7F00`** for a filename, which is why
`__mega65_h_name_buf` sits at `$0200` and cannot move into the fixed region.

**Hyppo traps need the MEGA65 I/O personality.** Startup unlocks it; code that
switches `$D02F` back for VIC-II graphics must unlock it again before the next
`mega65_h_*` call, or the trap write lands in a SID mirror and the call
reports a success that never happened.

**Hyppo clears `$D030` bit 0** on every file call, moving colour RAM away from
`$DC00` if that is where it was.

## Interrupts

Bank switching leaves the interrupt flag alone, so an interrupt can arrive at
any point, including with a bank mapped. Two rules follow:

- **Handlers live in the fixed region.** That is the default placement, so
  simply never give a handler `CODE_BANK()`.
- **Handlers must not touch `$2000-$7FFF`.** A handler runs with whichever
  bank the interrupted code had mapped and cannot find out which.

Neither is checked. Breaking them reads whichever bank was live, which looks
like intermittent corruption.

Interrupts start disabled and the vectors at `$FFFA`/`$FFFE` point at an
`RTI`. A program that wants them writes its own handler address there and
clears the flag. See `examples/mega65-banked-sd/banked-irq.cc`.

## Building

```sh
mos-mega65-banked-sd-clang -Os -o game.prg game.c
python3 prg-to-sd.py game.prg game_sd game --asset tiles.bin
```

This gives `game_sd/` holding `GAME.PRG`, `BANK1.BIN`…, and `TILES.BIN`. Copy
the directory onto the card and start `GAME.PRG`; startup loads the banks
before `main`.

`--report` adds a table of how full each bank is:

```
bank     used     free   fill
   1    18402     6174    74%
   2       44    24532     0%
```

Names are upper-cased because Hyppo upper-cases the name it is asked for but
not the one on the card, so a lower-case file can never be found.

## Changing the bank layout

Three files state where banks live, in different forms:

| File | Holds |
|---|---|
| `mapper.h` | `BANK_PHYS_BASE_n` |
| `mapper.s` | MAP register values |
| `_ram-banked-sd.ld` | Slot addresses |

`test/mega65-banked-sd/check-bank-tables.py` recomputes each base from the
others and fails if they disagree. It needs no emulator. Run it after any edit.
