# mega65-banked

A banked MEGA65 target with the KERNAL still available, for programs larger
than 64 KB. Banks are separate files loaded at startup, off a D81 image or the
SD card, so the `.prg` alone will not run.

Use `mega65-banked-nokernal` instead if the program needs neither BASIC nor the
KERNAL.

Using banks, the API, and the rules both banked platforms share are in
[mega65-common/README.md](../mega65-common/README.md#banking).

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
| 8-31 | `$8000800` … `$80B6800` | Attic RAM |

Chip RAM is all of `$00000-$5FFFF`, ROMs included: full speed, and fetched
from directly by VIC-IV, so any of banks 0-7 can hold graphics.

Banks 1-2 start at `$12000` rather than `$10000`: `$10000-$11FFF` is the C65
DOS work area, mapped whenever the KERNAL touches a disk, so a bank placed
there loses those bytes on the next disk call.

Banks 3-15 sit `$800` into a 64 KB page, so KERNAL LOAD never sees a zero
address high byte, which makes it corrupt the destination.

## Rules

**The C65 ROM clears `$200E-$7FFF` before your code runs.** Only the BASIC
header at `$2001-$200D` survives. The fixed region is untouched.

**Interrupts start disabled**, as on `mega65`. The KERNAL's keyboard scan and
clock need them, so a program that uses either, or its own handler, clears the
flag.

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

An interrupt handler beside banked code: `examples/mega65-banked/banked-irq.cc`.

## Building

```sh
mos-mega65-banked-clang -Os -o game.prg game.c
python3 prg-to-mega65.py game.prg . game
```

This writes `game-main.prg`, one file per non-empty bank, and `game.d81` with
the main program as `AUTOBOOT.C65`. Run the disk image, not the `.prg`.

Define `MAPPER_LOADER_SD` in every file to load the banks off the SD card
instead:

```sh
mos-mega65-banked-clang -DMAPPER_LOADER_SD -Os -o game.prg game.c
python3 prg-to-mega65.py game.prg game_sd game
```

`game_sd/` holds `GAME.D81`, which autoboots the main program, and
`BANK1.BIN`…. Copy it onto the card, mount `GAME.D81` and reset.

## Changing the bank layout

```sh
mos-mega65-banked-clang -DMAPPER_BANK_2=0x8040800 -Os -o game.prg game.c
```

A 16 or 8 KB window moves the soft stack into the freed top, which autoboot
has already cleared, and leaves the fixed region to code and data.

A bank's address high byte must not be `$00`, and the C65 ROMs at
`$20000-$3FFFF` stay write-protected.
