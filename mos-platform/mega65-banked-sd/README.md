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
| `$0300-$1FFF` | `RAM_LOW`, uninitialised |
| `$2000-$9FFF` | bank window, 32 KB |
| `$A000-$CFFF` | code, rodata, `.data` initialisers; soft stack grows down from `$D000` |
| `$D000-$DFFF` | I/O |
| `$E000-$FFF9` | `.data`, `.bss`, `.noinit` |
| `$FFFA-$FFFF` | interrupt vectors |

`$C000-$DFFF` is one MAPHI block, so the window cannot reach past `$BFFF`
without taking I/O with it.

## Banks

Bank 0 is the window unmapped. It is part of the main PRG, so `.bank_0`
sections are storage rather than padding — but only while bank 0 is selected.

| Bank | Physical | Memory |
|---|---|---|
| 0 | `$02000` | chip |
| 1 | `$12000` | chip |
| 2-5 | `$40000`, `$48000`, `$50000`, `$58000` | fast |
| 6-15 | `$8000000` … `$8048000` | attic |

Attic RAM is HyperRAM: roughly ten times slower, invisible to VIC-IV and SID,
and absent on boards without it.

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
- **Nothing writes to the screen for you.** There is no `CHROUT`, so `printf`
  and `putchar` have nowhere to go.
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
