#!/usr/bin/env python3
# Copyright 2026 LLVM-MOS Project
# Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
# See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
# information.
"""Convert a PNG image to MEGA65 FCM tile data for banked-fcm example.

Produces binary files for use with C23 #embed in banked-fcm.cc:
  - fcm-tiles.bin:   380 unique 8x8 FCM tiles (each 64 bytes, palette indices)
  - fcm-screen.bin:  2000-byte CHR16 screen map (40x25, 16-bit LE tile numbers)
  - fcm-palette.bin: 768-byte palette (256 R + 256 G + 256 B, nybble-reversed)
  - fcm-preview.png: reconstructed preview image for visual verification

Pipeline:
  1. Resize input to 320x200 and quantize to 255 colours (reserving index 0).
  2. Shift all pixel indices +1 so no tile pixel uses value 0, which VIC-IV FCM
     treats as transparent (showing the screen background colour).
  3. K-means clustering in RGB space reduces the 1000 screen tiles to 380
     unique tiles. Medoids (real source tiles) are used as cluster
     representatives to avoid palette-index averaging artifacts.
  4. Palette entry 0 is set to the darkest colour for use as border/background.
  5. Output binary files use VIC-IV nybble-reversed palette format and CHR16
     absolute addressing (tile base = BANK_PHYS_BASE_4 / 64, read from mapper.h).

Usage: python3 convert-fcm.py <input.png> [output_dir]
"""

import os
import re
import sys
import numpy as np
from PIL import Image

# Parse BANK_PHYS_BASE_N from mapper.h so tile addresses stay in sync
# with the platform's bank layout (including the +$800 KERNAL LOAD offset).
MAPPER_H = os.path.join(os.path.dirname(__file__),
                        "../../mos-platform/mega65-banked/mapper.h")

def read_bank_phys_base(bank):
    """Read BANK_PHYS_BASE_N from mapper.h."""
    pattern = rf"#define\s+BANK_PHYS_BASE_{bank}\s+(0x[0-9A-Fa-f]+)"
    with open(MAPPER_H) as f:
        for line in f:
            m = re.match(pattern, line)
            if m:
                return int(m.group(1), 16)
    raise ValueError(f"BANK_PHYS_BASE_{bank} not found in {MAPPER_H}")

TILE_BANK = 4
TILE_PHYS_BASE = read_bank_phys_base(TILE_BANK)
TILE_BASE = TILE_PHYS_BASE // 64  # FCM absolute addressing: screen value = phys / 64

MAX_TILES = 380       # Must fit in bank 4 (380 * 64 = 24320 < 24576)
SCREEN_W, SCREEN_H = 320, 200
TILE_W, TILE_H = 8, 8
COLS, ROWS = SCREEN_W // TILE_W, SCREEN_H // TILE_H  # 40x25


def nybble_swap(v):
    """VIC-IV palette uses reversed nybble order."""
    return ((v & 0x0F) << 4) | ((v & 0xF0) >> 4)


def tiles_to_rgb(tiles_idx, palette_rgb):
    """Convert tile palette indices to RGB vectors for distance computation.

    tiles_idx: (N, 64) uint8 palette indices
    palette_rgb: (256, 3) uint8 RGB values
    Returns: (N, 192) float64 — each tile as 64 RGB triplets flattened
    """
    n = tiles_idx.shape[0]
    rgb = palette_rgb[tiles_idx.flatten()].reshape(n, 64 * 3)
    return rgb.astype(np.float64)


def kmeans_tiles_rgb(tiles_idx, palette_rgb, k, max_iter=50):
    """K-means in RGB space, returning medoids (actual source tiles).

    Clustering uses RGB color values for meaningful distances. After
    convergence, each cluster's representative is the real source tile
    (medoid) closest to the RGB centroid — never an averaged tile.
    """
    tiles_rgb = tiles_to_rgb(tiles_idx, palette_rgb)
    n = tiles_rgb.shape[0]
    rng = np.random.default_rng(42)

    # K-means++ initialization in RGB space.
    indices = [int(rng.choice(n))]
    for _ in range(k - 1):
        dists = np.min(
            np.stack([np.sum((tiles_rgb - tiles_rgb[i]) ** 2, axis=1)
                      for i in indices]),
            axis=0)
        probs = dists / dists.sum()
        indices.append(int(rng.choice(n, p=probs)))

    centroids = tiles_rgb[indices].copy()
    assignments = np.zeros(n, dtype=np.int32)

    for iteration in range(max_iter):
        # Assign each tile to nearest centroid (RGB distance).
        dists = np.stack([np.sum((tiles_rgb - c) ** 2, axis=1) for c in centroids])
        new_assignments = np.argmin(dists, axis=0).astype(np.int32)

        if np.array_equal(new_assignments, assignments):
            print(f"  k-means converged at iteration {iteration}")
            break
        assignments = new_assignments

        # Update centroids as mean of cluster members (in RGB space).
        for j in range(k):
            members = tiles_rgb[assignments == j]
            if len(members) > 0:
                centroids[j] = members.mean(axis=0)

    # Select medoids: for each cluster, pick the real tile closest to centroid.
    medoid_indices = np.zeros(k, dtype=np.int32)
    for j in range(k):
        member_mask = (assignments == j)
        member_indices = np.where(member_mask)[0]
        if len(member_indices) == 0:
            # Empty cluster — pick any tile.
            medoid_indices[j] = 0
            continue
        member_rgb = tiles_rgb[member_indices]
        dists_to_centroid = np.sum((member_rgb - centroids[j]) ** 2, axis=1)
        best = np.argmin(dists_to_centroid)
        medoid_indices[j] = member_indices[best]

    # The final tiles are the actual source tiles (palette indices preserved).
    final_tiles = tiles_idx[medoid_indices]

    # Re-assign all 1000 positions to nearest medoid (in RGB space).
    medoid_rgb = tiles_rgb[medoid_indices]
    dists = np.stack([np.sum((tiles_rgb - m) ** 2, axis=1) for m in medoid_rgb])
    assignments = np.argmin(dists, axis=0).astype(np.int32)

    return final_tiles, assignments


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <input.png> [output_dir]")
        sys.exit(1)

    input_path = sys.argv[1]
    output_dir = sys.argv[2] if len(sys.argv) > 2 else os.path.dirname(input_path) or "."

    print(f"Loading {input_path}...")
    img = Image.open(input_path).convert("RGB")
    print(f"  Original size: {img.size}")

    # Downscale to 320x200.
    img = img.resize((SCREEN_W, SCREEN_H), Image.LANCZOS)
    print(f"  Resized to: {img.size}")

    # Quantize to 255 colors (not 256) so we can reserve index 0 for
    # background. VIC-IV FCM treats pixel value 0 as transparent (shows
    # screencol), so no tile pixel may use index 0.
    img_q = img.quantize(colors=255, method=Image.Quantize.MEDIANCUT)
    orig_palette = list(img_q.getpalette()[:255 * 3])
    pixels = np.array(img_q, dtype=np.uint8)
    print(f"  Quantized to {len(set(pixels.flatten()))} colors")

    # Shift all pixel indices by +1 (range 0-254 → 1-255) to avoid index 0.
    pixels = pixels.astype(np.uint16) + 1
    pixels = pixels.astype(np.uint8)

    # Build 256-entry palette: entry 0 = background, entries 1-255 = image colors.
    # Find the darkest quantized color for the background.
    darkest_orig = 0
    darkest_lum = float('inf')
    for i in range(255):
        r, g, b = orig_palette[i*3], orig_palette[i*3+1], orig_palette[i*3+2]
        lum = r * 0.299 + g * 0.587 + b * 0.114
        if lum < darkest_lum:
            darkest_lum = lum
            darkest_orig = i
    bg_r = orig_palette[darkest_orig*3]
    bg_g = orig_palette[darkest_orig*3+1]
    bg_b = orig_palette[darkest_orig*3+2]

    # Assemble the shifted 256-entry palette.
    palette_flat = [bg_r, bg_g, bg_b]  # entry 0 = background
    palette_flat.extend(orig_palette)   # entries 1-255 = original 0-254
    palette_rgb = np.array(palette_flat, dtype=np.uint8).reshape(256, 3)
    print(f"  Background (entry 0): RGB [{bg_r},{bg_g},{bg_b}]")

    # Extract 1000 tiles of 8x8 pixels (row-major within each tile).
    tiles = np.zeros((ROWS * COLS, TILE_W * TILE_H), dtype=np.uint8)
    for ty in range(ROWS):
        for tx in range(COLS):
            tile = pixels[ty * TILE_H:(ty + 1) * TILE_H,
                          tx * TILE_W:(tx + 1) * TILE_W]
            tiles[ty * COLS + tx] = tile.flatten()

    print(f"  Extracted {len(tiles)} tiles ({np.unique(tiles, axis=0).shape[0]} unique)")

    # K-means cluster in RGB space, using medoids as final tiles.
    print(f"  Clustering to {MAX_TILES} tiles (RGB space, medoids)...")
    final_tiles, assignments = kmeans_tiles_rgb(tiles, palette_rgb, MAX_TILES)

    # Verify.
    assert assignments.min() >= 0 and assignments.max() < MAX_TILES
    print(f"  Assignments range: {assignments.min()}-{assignments.max()}")
    used_tiles = len(set(assignments.flatten()))
    print(f"  Tiles actually used: {used_tiles}/{MAX_TILES}")

    # Build VIC-IV palette (nybble-reversed).
    palette = bytearray(768)
    for i in range(256):
        palette[i] = nybble_swap(palette_flat[i * 3])            # red
        palette[256 + i] = nybble_swap(palette_flat[i * 3 + 1])  # green
        palette[512 + i] = nybble_swap(palette_flat[i * 3 + 2])  # blue

    # Flatten tile data.
    tile_data = final_tiles.flatten()
    total_tile_bytes = len(tile_data)
    print(f"  Tile data: {total_tile_bytes} bytes ({MAX_TILES} tiles x 64)")
    print(f"  Screen map: 2000 bytes")
    print(f"  Palette: {len(palette)} bytes")

    # Build CHR16 screen map: FCM absolute addressing (screen value = phys / 64).
    # TILE_BASE is derived from BANK_PHYS_BASE_4 in mapper.h (currently
    # $46800 / 64 = $11A0).
    screen_map = bytearray(2000)
    for i in range(ROWS * COLS):
        char_num = TILE_BASE + int(assignments[i])
        screen_map[i * 2] = char_num & 0xFF
        screen_map[i * 2 + 1] = (char_num >> 8) & 0xFF
    print(f"  Screen values range: {TILE_BASE}-{TILE_BASE + int(assignments.max())}")

    # Write binary files.
    tiles_path = os.path.join(output_dir, "fcm-tiles.bin")
    screen_path = os.path.join(output_dir, "fcm-screen.bin")
    palette_path = os.path.join(output_dir, "fcm-palette.bin")

    with open(tiles_path, 'wb') as f:
        f.write(bytes(tile_data))
    print(f"  Wrote {tiles_path} ({len(tile_data)} bytes)")

    with open(screen_path, 'wb') as f:
        f.write(bytes(screen_map))
    print(f"  Wrote {screen_path} ({len(screen_map)} bytes)")

    with open(palette_path, 'wb') as f:
        f.write(bytes(palette))
    print(f"  Wrote {palette_path} ({len(palette)} bytes)")

    # Save a PNG preview of the reconstructed image (for visual verification).
    preview = np.zeros((SCREEN_H, SCREEN_W, 3), dtype=np.uint8)
    for ty in range(ROWS):
        for tx in range(COLS):
            tile_idx = int(assignments[ty * COLS + tx])
            tile = final_tiles[tile_idx].reshape(TILE_H, TILE_W)
            for py in range(TILE_H):
                for px in range(TILE_W):
                    ci = tile[py, px]
                    preview[ty*TILE_H+py, tx*TILE_W+px] = palette_rgb[ci]
    preview_path = os.path.join(output_dir, "fcm-preview.png")
    Image.fromarray(preview).save(preview_path)
    print(f"  Wrote {preview_path} (preview)")

    print("Done!")


if __name__ == "__main__":
    main()
