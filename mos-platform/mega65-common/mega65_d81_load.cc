// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// mega65_d81_load: a file off the mounted D81, through the F011, with no ROM
// and no Hyppo. The directory and the sector links are read in place in the
// controller's buffer, and DMA lands each sector at the destination's 28-bit
// address, so nothing is mapped and no copy of a sector is kept.

#include <dma.hpp>
#include <mega65.h>

namespace {

constexpr uint32_t SECTOR_BUFFER = 0xFFD6C00;
constexpr uint8_t DIR_TRACK = 40;
constexpr uint8_t FIRST_DIR_SECTOR = 3;
constexpr uint8_t DIR_ENTRIES = 8;
constexpr uint8_t NAME_LEN = 16;
constexpr uint8_t NAME_PAD = 0xA0;
constexpr uint8_t CLOSED = 0x80;
constexpr uint8_t TYPE_MASK = 0x07; // 0 is DEL

// The physical sector in the controller's buffer. It holds two logical
// sectors, so a file's consecutive sectors come off the disk once.
uint8_t cached_track, cached_sector, cached_side;

// Where a logical sector (track from 1, sector 0-39) lies in the controller's
// buffer, reading it off the disk if need be; 0 on a controller error.
uint32_t read_logical(uint8_t track, uint8_t sector) {
  uint8_t phys = (sector >> 1) + 1, side = 0;
  if (phys > 10) {
    phys -= 10;
    side = 1;
  }
  if (track - 1 != cached_track || phys != cached_sector ||
      side != cached_side) {
    F011.track = track - 1;
    F011.sector = phys;
    F011.side = side;
    F011.command = F011_CMD_READ | F011_CMD_NOBUF;
    while (F011.status1 & F011_BUSY_MASK)
      ;
    if (F011.status1 & (F011_RNF_MASK | F011_CRC_MASK)) {
      cached_track = 0xFF;
      return 0;
    }
    cached_track = track - 1;
    cached_sector = phys;
    cached_side = side;
  }
  return SECTOR_BUFFER + (sector & 1 ? 256 : 0);
}

// A name as d81.py stores it: PETSCII upper case, $A0-padded to 16.
void to_cbm_name(const char *name, uint8_t *out) {
  uint8_t i = 0;
  for (; i < NAME_LEN && name[i]; ++i) {
    uint8_t c = name[i];
    if (c >= 'a' && c <= 'z')
      c -= 0x20;
    else if (c < 0x20 || c > 0x5F)
      c = '?';
    out[i] = c;
  }
  for (; i < NAME_LEN; ++i)
    out[i] = NAME_PAD;
}

// The first track and sector of the named file, walking the directory chain.
bool find_file(const uint8_t *want, uint8_t &track, uint8_t &sector) {
  uint8_t t = DIR_TRACK, s = FIRST_DIR_SECTOR;
  while (t) {
    uint32_t dir = read_logical(t, s);
    if (!dir)
      return false;
    for (uint8_t i = 0; i < DIR_ENTRIES; ++i) {
      uint32_t entry = dir + 2 + i * 32;
      uint8_t type = mega65_peek_far(entry);
      if (!(type & CLOSED) || !(type & TYPE_MASK))
        continue;
      uint8_t j = 0;
      while (j < NAME_LEN && mega65_peek_far(entry + 3 + j) == want[j])
        ++j;
      if (j == NAME_LEN) {
        track = mega65_peek_far(entry + 1);
        sector = mega65_peek_far(entry + 2);
        return true;
      }
    }
    t = mega65_peek_far(dir);
    s = mega65_peek_far(dir + 1);
  }
  return false;
}

// Follow a file's sector chain, copying each sector's data to dest. A link
// track of 0 ends it, and the link sector is then the offset of the last byte.
uint32_t load_chain(uint8_t track, uint8_t sector, uint32_t dest) {
  uint32_t total = 0;
  for (;;) {
    uint32_t block = read_logical(track, sector);
    if (!block)
      return 0;
    uint8_t next_track = mega65_peek_far(block);
    uint8_t next_sector = mega65_peek_far(block + 1);
    uint8_t count = next_track ? 254 : next_sector - 1;
    mega65::dma::trigger_dma(
        mega65::dma::make_dma_copy(block + 2, dest + total, count));
    total += count;
    if (!next_track)
      return total;
    track = next_track;
    sector = next_sector;
  }
}

} // namespace

uint32_t mega65_d81_load(const char *name, uint32_t address) {
  uint8_t want[NAME_LEN], track, sector;
  to_cbm_name(name, want);
  cached_track = 0xFF; // the disk may have changed since the last call
  SDCARD.control &= (uint8_t)~SD_BUFFSEL_MASK;
  F011.control = F011_MOTOR_MASK;
  uint32_t loaded =
      find_file(want, track, sector) ? load_chain(track, sector, address) : 0;
  F011.control = 0;
  return loaded;
}
