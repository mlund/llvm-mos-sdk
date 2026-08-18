#!/usr/bin/env python3
# Copyright 2026 LLVM-MOS Project
# Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
# See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
# information.
"""Create a Commodore 1581 (.d81) disk image. Pure stdlib.

Exists so building a bootable disk needs only Python, which the tests already
require, rather than VICE's c1541.

Geometry: 80 tracks x 40 sectors x 256 bytes. Track 40 holds the header (40/0),
the BAM (40/1 tracks 1-40, 40/2 tracks 41-80) and the directory (40/3..40/39),
and is allocated whole at format time, leaving 3160 blocks free.

On-disk structures follow Peter Schepers, "D81 (Disk Image) Format", shipped
with VICE.
"""

import argparse
import itertools
import os
import sys
from typing import Iterator

TRACKS = 80
SECTORS_PER_TRACK = 40
SECTOR_SIZE = 256
IMAGE_SIZE = TRACKS * SECTORS_PER_TRACK * SECTOR_SIZE

# Track 40 is the directory track: reserved whole, so it never holds data.
DIR_TRACK = 40
HEADER_SECTOR = 0
BAM_SECTOR_LO = 1
BAM_SECTOR_HI = 2
FIRST_DIR_SECTOR = 3
LAST_DIR_SECTOR = 39

# Two of every sector go to the next-block link, leaving 254 for data.
DATA_PER_SECTOR = 254
ENTRIES_PER_SECTOR = 8
ENTRY_SIZE = 32
PAD = 0xA0  # CBM pads names with $A0, not spaces

FILE_TYPES = {"DEL": 0, "SEQ": 1, "PRG": 2, "USR": 3, "REL": 4}
EXT_TO_TYPE = {".prg": "PRG", ".seq": "SEQ", ".usr": "USR", ".del": "DEL"}


def to_petscii(s: str) -> bytes:
    """ASCII to PETSCII for disk and file names.

    Both cases map to $41-$5A, which is what the KERNAL's directory match
    compares against. Deliberately not the reversible c1541 mapping, which
    sends 'A'-'Z' to $C1-$DA instead and so depends on the caller's case.
    """
    out = bytearray()
    for ch in s:
        c = ord(ch)
        if 0x61 <= c <= 0x7A:
            c -= 0x20
        elif c < 0x20 or c > 0x5F:
            c = 0x3F  # '?'
        out.append(c)
    return bytes(out)


def petscii_field(s: str, length: int) -> bytes:
    b = to_petscii(s)[:length]
    return b + b"\xa0" * (length - len(b))


class D81:
    def __init__(self, name: str = "EMPTY", disk_id: str = "00") -> None:
        self.data = bytearray(IMAGE_SIZE)
        self.format(name, disk_id)

    def sector(self, track: int, sec: int) -> memoryview:
        if not (1 <= track <= TRACKS) or not (0 <= sec < SECTORS_PER_TRACK):
            raise ValueError(f"bad t/s {track}/{sec}")
        off = ((track - 1) * SECTORS_PER_TRACK + sec) * SECTOR_SIZE
        return memoryview(self.data)[off : off + SECTOR_SIZE]

    @staticmethod
    def _bam_loc(track: int) -> tuple:
        """BAM sector on track 40, and the offset of this track's 6-byte entry."""
        if 1 <= track <= 40:
            return BAM_SECTOR_LO, 0x10 + (track - 1) * 6
        if 41 <= track <= 80:
            return BAM_SECTOR_HI, 0x10 + (track - 41) * 6
        raise ValueError(f"bad track {track}")

    def is_free(self, track: int, sec: int) -> bool:
        bs, off = self._bam_loc(track)
        bam = self.sector(DIR_TRACK, bs)
        return bool(bam[off + 1 + (sec >> 3)] & (1 << (sec & 7)))

    def allocate(self, track: int, sec: int) -> None:
        bs, off = self._bam_loc(track)
        bam = self.sector(DIR_TRACK, bs)
        i, mask = off + 1 + (sec >> 3), 1 << (sec & 7)
        if not bam[i] & mask:
            raise RuntimeError(f"sector {track}/{sec} already allocated")
        bam[i] &= ~mask & 0xFF
        bam[off] -= 1

    def blocks_free(self) -> int:
        total = 0
        for t in range(1, TRACKS + 1):
            bs, off = self._bam_loc(t)
            total += self.sector(DIR_TRACK, bs)[off]
        return total

    def format(self, name: str, disk_id: str = "00") -> None:
        # Zeroing marks every directory entry unused, since a file type of 0
        # is what _alloc_dir_entry treats as free.
        self.data[:] = bytes(IMAGE_SIZE)
        did = petscii_field(disk_id, 2)

        h = self.sector(DIR_TRACK, HEADER_SECTOR)
        h[0x00], h[0x01] = DIR_TRACK, FIRST_DIR_SECTOR
        h[0x02] = 0x44  # 'D', disk format type
        h[0x03] = 0x00
        h[0x04:0x14] = petscii_field(name, 16)
        h[0x14:0x16] = b"\xa0\xa0"
        h[0x16:0x18] = did
        h[0x18] = PAD
        h[0x19] = 0x33  # DOS version '3'
        h[0x1A] = 0x44  # disk version 'D'
        h[0x1B:0x1D] = b"\xa0\xa0"

        lo = self.sector(DIR_TRACK, BAM_SECTOR_LO)
        lo[0x00], lo[0x01] = DIR_TRACK, BAM_SECTOR_HI
        hi = self.sector(DIR_TRACK, BAM_SECTOR_HI)
        hi[0x00], hi[0x01] = 0x00, 0xFF
        for b in (lo, hi):
            b[0x02] = 0x44  # version 'D'
            b[0x03] = 0xBB  # its one's complement
            b[0x04:0x06] = did
            b[0x06] = 0xC0  # I/O byte: verify on, check CRC
            b[0x07] = 0x00  # autoloader flag

        for t in range(1, TRACKS + 1):
            bs, off = self._bam_loc(t)
            b = self.sector(DIR_TRACK, bs)
            b[off] = SECTORS_PER_TRACK
            b[off + 1 : off + 6] = b"\xff" * 5

        bs, off = self._bam_loc(DIR_TRACK)  # track 40 is reserved whole
        b = self.sector(DIR_TRACK, bs)
        b[off] = 0
        b[off + 1 : off + 6] = b"\x00" * 5

        d = self.sector(DIR_TRACK, FIRST_DIR_SECTOR)
        d[0x00], d[0x01] = 0x00, 0xFF

    def _free_blocks(self) -> Iterator[tuple]:
        return (
            (t, s)
            for t in range(1, TRACKS + 1)
            if t != DIR_TRACK
            for s in range(SECTORS_PER_TRACK)
            if self.is_free(t, s)
        )

    def _alloc_chain(self, count: int) -> list:
        # No interleave: an image file has no rotational latency to hide.
        blocks = list(itertools.islice(self._free_blocks(), count))
        if len(blocks) < count:
            raise RuntimeError(
                f"disk full: need {count} blocks, {self.blocks_free()} free"
            )
        for t, s in blocks:
            self.allocate(t, s)
        return blocks

    def _alloc_dir_entry(self) -> tuple:
        """Directory sector number, and the offset of a free 32-byte entry."""
        cur, seen = FIRST_DIR_SECTOR, []
        while True:
            seen.append(cur)
            d = self.sector(DIR_TRACK, cur)
            for i in range(ENTRIES_PER_SECTOR):
                off = i * ENTRY_SIZE
                if d[off + 2] == 0x00:  # unused file-type byte
                    return cur, off
            if d[0] == DIR_TRACK:  # a further sector already exists
                cur = d[1]
                continue
            nxt = max(seen) + 1
            if nxt > LAST_DIR_SECTOR:
                raise RuntimeError("directory full (296 entries)")
            d[0], d[1] = DIR_TRACK, nxt
            nd = self.sector(DIR_TRACK, nxt)
            nd[0], nd[1] = 0x00, 0xFF
            cur = nxt

    def add_file(self, cbm_name: str, data: bytes, ftype: str = "PRG") -> int:
        if not data:
            raise ValueError(f"{cbm_name}: refusing to write an empty file")
        code = FILE_TYPES[ftype.upper()]
        chunks = [
            bytes(data[i : i + DATA_PER_SECTOR])
            for i in range(0, len(data), DATA_PER_SECTOR)
        ]
        blocks = self._alloc_chain(len(chunks))

        for i, (t, s) in enumerate(blocks):
            sec = self.sector(t, s)
            if i + 1 < len(blocks):
                sec[0], sec[1] = blocks[i + 1]
            else:
                # End of chain: byte 1 is the offset of the last valid byte.
                sec[0], sec[1] = 0x00, len(chunks[i]) + 1
            sec[2 : 2 + len(chunks[i])] = chunks[i]

        ds, off = self._alloc_dir_entry()
        e = self.sector(DIR_TRACK, ds)
        e[off + 2] = 0x80 | code  # bit 7 = closed
        e[off + 3], e[off + 4] = blocks[0]
        e[off + 5 : off + 21] = petscii_field(cbm_name, 16)
        e[off + 30] = len(blocks) & 0xFF
        e[off + 31] = (len(blocks) >> 8) & 0xFF
        return len(blocks)

    def save(self, path: str) -> None:
        with open(path, "wb") as f:
            f.write(self.data)


def parse_spec(spec: str) -> tuple:
    """PATH, or PATH=CBMNAME, or PATH=CBMNAME,TYPE."""
    path, _, rest = spec.partition("=")
    name, _, ftype = rest.partition(",")
    if not name:
        name = os.path.splitext(os.path.basename(path))[0]
    if not ftype:
        ftype = EXT_TO_TYPE.get(os.path.splitext(path)[1].lower(), "PRG")
    return path, name, ftype.upper()


def main(argv=None) -> None:
    ap = argparse.ArgumentParser(description="Create a .d81 disk image.")
    ap.add_argument("image")
    ap.add_argument(
        "files", nargs="*", help="PATH[=CBMNAME[,TYPE]] with TYPE in PRG SEQ USR DEL"
    )
    ap.add_argument("-n", "--name", default="EMPTY", help="disk name, 16 chars")
    ap.add_argument("-i", "--id", default="00", dest="disk_id", help="disk ID, 2 chars")
    # Intermixed so options may follow the file list, which a caller building
    # the command line by appending naturally produces.
    a = ap.parse_intermixed_args(argv)

    d = D81(a.name, a.disk_id)
    for spec in a.files:
        path, cbm, ftype = parse_spec(spec)
        if ftype not in FILE_TYPES:
            ap.error(f"unknown file type {ftype!r}")
        with open(path, "rb") as f:
            blocks = d.add_file(cbm, f.read(), ftype)
        print(f"{cbm:<16} {ftype:<3} {blocks:4d} blocks", file=sys.stderr)
    d.save(a.image)
    print(f"{d.blocks_free()} blocks free", file=sys.stderr)


if __name__ == "__main__":
    main()
