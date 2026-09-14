// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// mega65_d81_load: a file off the mounted D81, through the F011, with no ROM
// and no Hyppo. DMA lands each sector at the destination's 28-bit address, so
// nothing is ever mapped.

#include <dma.hpp>
#include <mega65.h>

namespace {

constexpr uint32_t SECTOR_BUFFER = 0xFFD6C00;
constexpr unsigned char DIR_TRACK = 40;
constexpr unsigned char FIRST_DIR_SECTOR = 3;
constexpr unsigned char DIR_ENTRIES = 8;
constexpr unsigned char NAME_LEN = 16;
constexpr unsigned char NAME_PAD = 0xA0;
constexpr unsigned char CLOSED = 0x80;
constexpr unsigned char TYPE_MASK = 0x07; // 0 is DEL

// One 512-byte physical sector: two 256-byte CBM logical sectors.
unsigned char buffer[512];
unsigned char cached_track, cached_sector, cached_side;

// Returns 256 bytes of a logical sector (track from 1, sector 0-39), or
// nullptr on error. Caches to avoid re-reading (two logical sectors per
// physical sector).
unsigned char *read_logical(unsigned char track, unsigned char sector) {
  unsigned char phys = (sector >> 1) + 1, side = 0;
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
      return nullptr;
    }
    mega65::dma::trigger_dma(mega65::dma::make_dma_copy(
        SECTOR_BUFFER, (uint16_t)buffer, sizeof buffer));
    cached_track = track - 1;
    cached_sector = phys;
    cached_side = side;
  }
  return buffer + (sector & 1 ? 256 : 0);
}

// A name as d81.py stores it: PETSCII upper case, $A0-padded to 16.
void to_cbm_name(const char *name, unsigned char *out) {
  unsigned char i = 0;
  for (; i < NAME_LEN && name[i]; ++i) {
    unsigned char c = name[i];
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
bool find_file(const unsigned char *want, unsigned char &track,
               unsigned char &sector) {
  unsigned char t = DIR_TRACK, s = FIRST_DIR_SECTOR;
  while (t) {
    unsigned char *dir = read_logical(t, s);
    if (!dir)
      return false;
    for (unsigned char i = 0; i < DIR_ENTRIES; ++i) {
      const unsigned char *entry = dir + 2 + i * 32;
      if (!(entry[0] & CLOSED) || !(entry[0] & TYPE_MASK))
        continue;
      unsigned char j = 0;
      while (j < NAME_LEN && entry[3 + j] == want[j])
        ++j;
      if (j == NAME_LEN) {
        track = entry[1];
        sector = entry[2];
        return true;
      }
    }
    t = dir[0];
    s = dir[1];
  }
  return false;
}

// Follow a file's sector chain, copying each sector's data to dest. A link
// track of 0 ends it, and the link sector is then the offset of the last byte.
uint32_t load_chain(unsigned char track, unsigned char sector, uint32_t dest) {
  uint32_t total = 0;
  for (;;) {
    unsigned char *block = read_logical(track, sector);
    if (!block)
      return 0;
    unsigned char next_track = block[0], next_sector = block[1];
    unsigned char count = next_track ? 254 : next_sector - 1;
    mega65::dma::trigger_dma(mega65::dma::make_dma_copy(
        SECTOR_BUFFER + (uint16_t)(block - buffer) + 2, dest + total, count));
    total += count;
    if (!next_track)
      return total;
    track = next_track;
    sector = next_sector;
  }
}

} // namespace

uint32_t mega65_d81_load(const char *name, uint32_t address) {
  unsigned char want[NAME_LEN], track, sector;
  to_cbm_name(name, want);
  cached_track = 0xFF; // the disk may have changed since the last call
  SDCARD.control &= (uint8_t)~SD_BUFFSEL_MASK;
  F011.control = F011_MOTOR_MASK;
  uint32_t loaded =
      find_file(want, track, sector) ? load_chain(track, sector, address) : 0;
  F011.control = 0;
  return loaded;
}
