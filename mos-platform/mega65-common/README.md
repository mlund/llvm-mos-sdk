# mega65-common

Documentation shared by the MEGA65 targets.

## Banking

This covers both `mega65-banked` and `mega65-banked-nokernal`. Each platform's
README gives its memory map, bank addresses and its own rules.

### Using banks

```c
#define MAPPER_BANK_COUNT 2    // highest bank used
#include <mapper.h>

RODATA_BANK(1) const uint8_t sine[256] = { ... };

CODE_BANK(1) void animate(void) { ... }

int main(void) {
  banked_call(1, animate);
}
```

`MAPPER_BANK_COUNT`, 0-31, excludes higher banks from the image and the bank
tables; define it the same in every file. Without it all 31 are included
(mostly empty, read off disk).

`CODE_BANK` adds `noinline` to prevent inlining back into the fixed region.
`RODATA_BANK` adds `used` and `retain` to prevent discarding unreferenced data.

Keep a table in the same bank as the code reading it, so one call covers both.

### API

| Function | Purpose |
|---|---|
| `banked_call(bank, fn)` | Map `bank`, call `fn`, restore the previous bank |
| `banked_call_r(bank, fn, ...)` | As above, with arguments and a return value |
| `banked_call_v(bank, fn, ...)` | As above, for `void` functions |
| `get_bank()` | The currently mapped bank |
| `set_bank(bank)` | Map a bank directly; prefer `banked_call` |

A `bank` above `MAPPER_BANK_COUNT` maps bank 0.

```c
int n = banked_call_r(1, measure, text, len);
banked_call_v(1, draw, x, y);
```

The `_r` and `_v` forms switch the bank around a direct call, marshalling the
real signature. The caller must be in the fixed region, or the switch would
unmap it (link fails). Up to eight arguments are evaluated before the switch.

### Rules

**`banked_call` takes `void(void)` only.** For arguments or a return value, use
`banked_call_r` or `banked_call_v`.

**Bank data is readable only while its bank is mapped.** Anything shared or
long-lived belongs in the fixed region.

**Everything not given a bank goes in the fixed region** — code, string
literals, static variables. This is the usual limit a program hits first.

**A bank may call another bank.** `banked_call` is in the fixed region, so the
caller's bank is back before control returns to it.

**Do not pass `-T`.** A supplementary linker script suppresses the platform's
`OUTPUT_FORMAT`, producing an ELF instead of the flat image the converter
needs. For large buffers, use a fixed address instead:

```c
static int16_t *const buffer = (int16_t *)0x2100;
```

**Attic RAM is for tables and logic.** It is about ten times slower than chip
RAM, out of reach of VIC-IV and audio DMA, and absent on Nexys A7 boards.

### Interrupts

Bank switching leaves the interrupt flag alone, so an interrupt can arrive at
any point, including with a bank mapped. Two rules follow:

- **Handlers live in the fixed region.** That is the default; never give a
  handler `CODE_BANK()`.
- **Handlers must not touch the window.** A handler runs with whichever bank
  the interrupted code had mapped and cannot find out which.

Neither is checked. Breaking them reads whichever bank was live, which looks
like intermittent corruption.

### Changing the bank layout

Define uniformly in every file via `-D` or a header (`-include`).

| Macro | Effect |
|---|---|
| `MAPPER_BANK_COUNT` | Highest bank used: 0-31 (default 31) |
| `MAPPER_BANK_n` | Physical address of bank *n* |
| `MAPPER_WINDOW_KB` | Window size: 24 (default), 16 or 8 KB |
| `MAPPER_BANK_n_KB` | Bank *n* smaller than the window: 16 or 8 KB |

`BANK_PHYS_BASE_n` and `BANK_SIZE_n` give the result at compile time.

A smaller window frees its top; each platform README says what that space
becomes.

While a smaller bank is mapped, the rest of the window is the window's own RAM.
`WINDOW_TAIL` places uninitialised data there, readable with bank 0 or a
smallest bank mapped.

`<mapper.h>` checks each address at compile time; the converter rejects
overlapping banks and files that disagree.

### Loading banks

Startup loads every non-empty bank before constructors run. Choose another
loader with a macro, defined the same in every file.

| Platform | Default | Alternative |
|---|---|---|
| `mega65-banked` | D81, through KERNAL LOAD | SD card: `MAPPER_LOADER_SD` |
| `mega65-banked-nokernal` | SD card, through Hyppo | D81 through the F011: `MAPPER_LOADER_FLOPPY` |

A failed load calls `__bank_load_failed(bank)`, which halts with the border red.
Override it to report the failure.

### Converting

`prg-to-mega65.py` turns the linked image into the files that ship, reading
the platform and loader from the image itself. `--report` adds a table of how
full each bank is:

```
bank     used     free   fill
   1    18402     6174    74%
   2       44    24532     0%
```
