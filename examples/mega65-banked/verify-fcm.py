#!/usr/bin/env python3
"""Reconstruct the image from fcm-data.h arrays to verify correctness.

Reads the C header, decodes tile data + screen map + palette, and saves
a 320x200 preview PNG. If this preview matches the original, the Python
conversion is correct and the bug is in VIC-IV setup/addressing.
"""

import re
import struct
import numpy as np
from PIL import Image

def parse_hex_array(text, name):
    """Extract a C array of hex bytes from header text."""
    # Find the array definition
    pattern = rf'{name}\[.*?\]\s*(?:__attribute__\(\(.*?\)\))?\s*=\s*\{{(.*?)\}};'
    m = re.search(pattern, text, re.DOTALL)
    if not m:
        raise ValueError(f"Array {name} not found")
    hex_str = m.group(1)
    values = [int(x, 16) for x in re.findall(r'0x[0-9A-Fa-f]+', hex_str)]
    return bytes(values)

def nybble_swap(v):
    return ((v & 0x0F) << 4) | ((v & 0xF0) >> 4)

def main():
    with open("fcm-data.h") as f:
        text = f.read()

    tile_data = parse_hex_array(text, "fcm_tile_data")
    screen_map = parse_hex_array(text, "fcm_screen_map")
    palette_data = parse_hex_array(text, "fcm_palette_data")

    print(f"Tile data: {len(tile_data)} bytes ({len(tile_data)//64} tiles)")
    print(f"Screen map: {len(screen_map)} bytes")
    print(f"Palette: {len(palette_data)} bytes")

    # Decode palette (nybble-reversed VIC-IV format back to RGB).
    palette_rgb = np.zeros((256, 3), dtype=np.uint8)
    for i in range(256):
        palette_rgb[i, 0] = nybble_swap(palette_data[i])         # red
        palette_rgb[i, 1] = nybble_swap(palette_data[256 + i])   # green
        palette_rgb[i, 2] = nybble_swap(palette_data[512 + i])   # blue

    # Decode screen map (CHR16 little-endian).
    TILE_BASE = 0x1000
    chars = []
    for i in range(0, 2000, 2):
        char_num = screen_map[i] | (screen_map[i+1] << 8)
        chars.append(char_num)
    print(f"Screen chars range: {min(chars)}-{max(chars)}")
    print(f"Tile indices range: {min(chars)-TILE_BASE}-{max(chars)-TILE_BASE}")

    # Reconstruct image.
    img = np.zeros((200, 320, 3), dtype=np.uint8)
    for row in range(25):
        for col in range(40):
            char_num = chars[row * 40 + col]
            tile_idx = char_num - TILE_BASE
            if tile_idx < 0 or tile_idx >= len(tile_data) // 64:
                print(f"  WARNING: position ({row},{col}) tile_idx={tile_idx} out of range!")
                continue
            tile = tile_data[tile_idx * 64:(tile_idx + 1) * 64]
            for py in range(8):
                for px in range(8):
                    pixel_idx = tile[py * 8 + px]
                    img[row * 8 + py, col * 8 + px] = palette_rgb[pixel_idx]

    out = Image.fromarray(img)
    out.save("fcm-preview.png")
    print("Saved fcm-preview.png")

if __name__ == "__main__":
    main()
