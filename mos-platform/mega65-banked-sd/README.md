# mega65-banked-sd

A banked MEGA65 target with BASIC and the KERNAL mapped out. Banks and assets
are plain files on the SD card, loaded through Hyppo's file traps.

Use `mega65-banked` instead if the program must load from a D81 image or call
the KERNAL.

## Memory map

| Range | Contents |
|---|---|
| `$0002-$0021` | imaginary registers |
| `$0022-$00FF` | compiler zero page, 222 bytes |
| `$0100-$01FF` | hardware stack |
| `$0200-$02FF` | Hyppo filename staging page |
| `$0300-$1FFF` | `RAM_LOW`, uninitialised, 7,424 B |
| `$2000-$7FFF` | bank window, 24 KB |
| `$8000-$CFFF` | code, rodata, `.data` initialisers, 20,480 B |
| `$D000-$DFFF` | I/O |
| `$E000-$FFF9` | `.data`, `.bss`, `.noinit`, and the soft stack growing down |
| `$FFFA-$FFFF` | interrupt vectors |

MAPHI is untouched, so everything from `$8000` up stays where the linker put
it. About 19.5 KB of the fixed region is left after the runtime.

## Banks

Bank 0 is the window unmapped. It is part of the main PRG, so `.bank_0`
sections are storage rather than padding — but only while bank 0 is selected.

| Bank | Physical | Memory |
|---|---|---|
| 0 | `$02000` | the window itself |
| 1-2 | `$12000`, `$18000` | chip |
| 3-7 | `$20000`, `$26000`, `$2C000`, `$32000`, `$38000` | where the C65 ROMs would be |
| 8-12 | `$40000`, `$46000`, `$4C000`, `$52000`, `$58000` | |
| 13-15 | `$8000000`, `$8006000`, `$800C000` | attic |

Banks 1-12 are all full speed and all reachable by VIC-IV, which fetches
anything below `$60000` — so any of them can hold a screen, a charset or
sprite data. Attic RAM is HyperRAM: roughly ten times slower, invisible to
VIC-IV and SID, and absent on boards without it.

Banks 3-7 occupy the second 128 KB, which on a stock machine holds the C65
ROMs — including the character generator at `$2D000` and the KERNAL at
`$3E000`. This platform runs none of them, so startup lifts the hypervisor's
write protection over that region and the banks are ordinary RAM. Supply your
own charset; there is no longer one to fall back on.

## Using it

```c
#include <mapper.h>

MAPPER_BANK_COUNT(2);

RODATA_BANK(1) const unsigned char sine[256] = { ... };
CODE_BANK(1) void animate(void) { ... }

int main(void) {
  banked_call(1, animate);
}
```

`MAPPER_BANK_COUNT` is what keeps the unused banks out of the build. Without
it every program reserves all 15.

Code that runs in a bank must not assume any other bank is mapped, and
`banked_call` must be reached from fixed code, not from another bank.

To pass arguments or take a return value, call from the fixed region with
`banked_call_r` or `banked_call_v`:

```c
int n = banked_call_r(1, measure, text, len);
banked_call_v(1, draw, x, y);
```

These switch the bank around a direct call, so the compiler marshals the real
signature. Two rules come with that, neither of them diagnosed: the caller
must be in the fixed region, since from a bank the switch would unmap the
caller mid-call; and the argument expressions are evaluated with the target
bank already mapped, so none may read the outgoing bank.

## Building

The linker emits one flat image; `prg-to-sd.py` splits it into the files that
go on the card.

```
prg-to-sd.py game.prg game_sd game --asset tiles.bin
```

gives `game_sd/` holding `GAME.PRG`, `BANK1.BIN`…, and `TILES.BIN`. Copy the
directory onto the card and start `GAME.PRG`; the CRT loads the banks before
`main`.

Names are upper-cased because Hyppo upper-cases the name it is asked for but
not the one on the card, so a lower-case file can never be found.

**Do not pass `-T`.** A supplementary linker script suppresses the platform's
`OUTPUT_FORMAT`, producing an ELF instead of the flat image the split needs.

## Rules

- **No KERNAL.** The `cbm_k_*` and `mega65_k_*` wrappers link but jump into
  RAM. The `mega65_h_*` Hyppo wrappers are the ones that work, and they are how
  a program loads its own assets: `mega65_h_setname` then
  `mega65_h_loadfile(addr)`, or `mega65_h_loadfile_attic` above `$8000000`.
- **Nothing writes to the screen for you.** There is no `CHROUT`, no character
  ROM and no BASIC. A program brings its own charset, or draws without one.
- **The card is the only storage, both ways.** Read with `mega65_h_setname` and
  `mega65_h_loadfile`; write with `mega65_h_mkfile` then `mega65_h_writefile`.
  `mkfile` takes 8.3 names only and allocates contiguously, so create a save
  file at its full size once and rewrite it in place.
- **Interrupts start disabled** and the vectors at `$FFFA`/`$FFFE` point at an
  `RTI`. A handler must live in the fixed region and must not touch the bank
  window: it runs with whichever bank the interrupted code had mapped.
- **Hyppo needs a page below `$7F00`** for a filename, which is why
  `__mega65_h_name_buf` sits at `$0200` and cannot move into the fixed region.
- **Hyppo traps need the MEGA65 I/O personality.** Startup unlocks it; code
  that switches `$D02F` back for VIC-II graphics must unlock it again before
  the next `mega65_h_*` call, or the trap write lands in a SID mirror and the
  call reports a success that never happened.
- **Hyppo clears `$D030` bit 0** on every file call, moving colour RAM away
  from `$DC00` if that is where it was.

## Changing the layout

The bank layout is written down three times: `mapper.h` as physical bases,
`mapper.s` as MAP register values, and `_ram-banked-sd.ld` as slot addresses.
`test/mega65-banked-sd/check-bank-tables.py` recomputes each base from the
first two and fails if they disagree. It needs no emulator.
