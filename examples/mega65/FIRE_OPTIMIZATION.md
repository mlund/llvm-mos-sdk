# Fire FCM — How It Works

Classic demoscene fire effect rendered in VIC-IV Full Color Mode (FCM).

## Resolution Mapping

The fire simulation runs at quarter resolution (80x50 pixels). Each fire
pixel maps to a 4x4 block on the 320x200 screen. Since FCM tiles are 8x8,
each tile contains exactly 2x2 fire pixels — no partial tiles or scaling
logic needed.

## Fire Simulation

The bottom two rows are seeded with random values in the range 192-255.
Each frame, every pixel is replaced by the average of its four neighbors
(left, center, right from the row below, plus the pixel two rows below),
minus a decay constant. This propagates heat upward with gradual cooling.

## Tile Conversion

`convert_and_dma()` converts one screen row (40 tiles) at a time into a
CPU-side buffer, then DMA-copies it to tile data in fast RAM at $40000.
Each 2x2 block of fire pixels is expanded to fill the 8x8 tile:

```
AAAABBBB    A = fire[fy][fx]     B = fire[fy][fx+1]
AAAABBBB    C = fire[fy+1][fx]   D = fire[fy+1][fx+1]
AAAABBBB
AAAABBBB
CCCCDDDD
CCCCDDDD
CCCCDDDD
CCCCDDDD
```

## STQ Inline Assembly

The `fill4()` helper uses the 45GS02 STQ instruction to write 4 identical
bytes in a single store. STQ stores the 32-bit Q register {A, X, Y, Z}
through a zero-page pointer. The helper sets all four registers to the
same pixel value, stores via `stq (zp)`, then restores Z to 0 (the
compiler assumes Z is always 0).

16 `fill4` calls fill one 64-byte tile. The compiler handles all pointer
arithmetic (`dst + 0`, `dst + 4`, ..., `dst + 60`).
