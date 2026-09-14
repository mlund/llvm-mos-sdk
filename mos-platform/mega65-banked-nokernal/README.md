# mega65-banked-nokernal

A banked MEGA65 target with BASIC and the KERNAL mapped out. Banks and assets
are plain files on the SD card, loaded through Hyppo.

Use `mega65-banked` instead if the program must load from a D81 image through
the KERNAL or call the KERNAL.

Using banks, the API, and the rules both banked platforms share are in
[BANKING.md](../mega65-common/BANKING.md).

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
| 3-7 | `$20000` … `$38000` | Chip RAM, where the C65 ROMs would be |
| 8-12 | `$40000` … `$58000` | Chip RAM |
| 13-15 | `$8000000` … `$800C000` | Attic RAM |

Bank 0 is the window unmapped, so `.bank_0` content ships inside the main
program rather than a bank file.

Chip RAM is all of `$00000-$5FFFF`: full speed, and fetched from directly by
VIC-IV, so any of banks 0-12 can hold a screen, a charset or sprite data.
Banks 3-7 sit where the C65 ROMs would be, including the character generator
at `$2D000`; startup lifts the write protection there. Bring your own charset,
as there is no longer one to fall back on.

## Rules

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

**Interrupts start disabled** and the vectors at `$FFFA`/`$FFFE` point at an
`RTI`. A program that wants them writes its own handler address there and
clears the flag. See `examples/mega65-banked-nokernal/banked-irq.cc`.

## Building

```sh
mos-mega65-banked-nokernal-clang -Os -o game.prg game.c
python3 prg-to-mega65.py game.prg game_sd game --asset tiles.bin
```

This gives `game_sd/` holding `GAME.PRG`, `BANK1.BIN`…, and `TILES.BIN`. Copy
the directory onto the card and start `GAME.PRG`; startup loads the banks
before `main`.

Names are upper-cased because Hyppo upper-cases the name it is asked for but
not the one on the card, so a lower-case file can never be found.

## Changing the bank layout

```sh
mos-mega65-banked-nokernal-clang -DMAPPER_BANK_2=0x8012000 \
  -Os -o game.prg game.c
```

A 16 or 8 KB window moves the fixed region down to meet it: 28 KB or 36 KB of
it.

## Loading banks from a D81

Define `MAPPER_LOADER_FLOPPY`, the same in every file, to load banks off the D81
mounted as drive 8, through the F011. `prg-to-mega65.py` then writes the banks
into `NAME.D81` beside `NAME.PRG`, with any `--asset` files. Tested on mounted
images in xemu; a real drive is untested.

`mega65_d81_load(name, address)` loads any other file off that disk the same
way, into any 28-bit address, at any time.
