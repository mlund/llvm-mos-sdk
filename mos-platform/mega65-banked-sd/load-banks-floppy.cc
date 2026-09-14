// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// Bank loader for MAPPER_LOADER_FLOPPY: every non-empty bank off the mounted
// D81, through the F011. No ROM and no Hyppo; DMA lands each sector's data at
// the bank's 28-bit address, so nothing is ever mapped.

#define __MAPPER_NO_TABLES
#include <dma.hpp>
#include <mapper.h>
#include <mega65.h>

extern "C" {
extern const unsigned char __bank_used[15];
extern const unsigned char __bank_megabyte[16];
extern const unsigned char __bank_addr_mid[16];
extern const unsigned char __bank_addr_page[16];
void __load_banks_floppy(void);
}

namespace {

constexpr uint32_t SECTOR_BUFFER = 0xFFD6C00;
constexpr unsigned char DIR_TRACK = 40;
constexpr unsigned char FIRST_DIR_SECTOR = 3;
constexpr unsigned char DIR_ENTRIES = 8;
constexpr unsigned char PRG_CLOSED = 0x82;
constexpr unsigned char NAME_PAD = 0xA0;

// One 512-byte physical sector: two 256-byte CBM logical sectors.
unsigned char buffer[512];
unsigned char cached_track = 0xFF, cached_sector, cached_side;

// The 256 bytes of a logical sector (track from 1, sector 0-39), or nullptr
// on a controller error. Each physical sector is read once, since a file's
// consecutive logical sectors share one.
__attribute__((section(".bank_0")))
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

// Whether a 16-byte directory name is BANKn, as the converter writes it.
__attribute__((section(".bank_0")))
bool is_bank_name(const unsigned char *name, unsigned char bank) {
  static const char prefix[] = "BANK";
  for (unsigned char i = 0; i < 4; ++i)
    if (name[i] != prefix[i])
      return false;
  if (name[4] != (bank <= 9 ? '0' + bank : 'A' + bank - 10))
    return false;
  for (unsigned char i = 5; i < 16; ++i)
    if (name[i] != NAME_PAD)
      return false;
  return true;
}

// The first track and sector of BANKn, walking the directory chain.
__attribute__((section(".bank_0")))
bool find_bank(unsigned char bank, unsigned char &track,
               unsigned char &sector) {
  unsigned char t = DIR_TRACK, s = FIRST_DIR_SECTOR;
  while (t) {
    unsigned char *dir = read_logical(t, s);
    if (!dir)
      return false;
    for (unsigned char i = 0; i < DIR_ENTRIES; ++i) {
      unsigned char *entry = dir + 2 + i * 32;
      if (entry[0] == PRG_CLOSED && is_bank_name(entry + 3, bank)) {
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
__attribute__((section(".bank_0")))
bool load_chain(unsigned char track, unsigned char sector, uint32_t dest) {
  for (;;) {
    unsigned char *block = read_logical(track, sector);
    if (!block)
      return false;
    unsigned char next_track = block[0], next_sector = block[1];
    unsigned char count = next_track ? 254 : next_sector - 1;
    mega65::dma::trigger_dma(mega65::dma::make_dma_copy(
        SECTOR_BUFFER + (uint16_t)(block - buffer) + 2, dest, count));
    if (!next_track)
      return true;
    dest += count;
    track = next_track;
    sector = next_sector;
  }
}

} // namespace

// In .bank_0, like the rest: it runs only at startup, with bank 0 mapped,
// so the window holds it and the fixed region stays free.
__attribute__((noinline, section(".bank_0")))
void __load_banks_floppy(void) {
  SDCARD.control &= (uint8_t)~SD_BUFFSEL_MASK;
  F011.control = F011_MOTOR_MASK;
  for (unsigned char i = 0; i < 15; ++i) {
    unsigned char bank = i + 1, track, sector;
    if (!__bank_used[i])
      continue;
    uint32_t base = (uint32_t)__bank_megabyte[bank] << 20 |
                    (uint32_t)(__bank_addr_mid[bank] & 0x0F) << 16 |
                    (uint16_t)(__bank_addr_page[bank] << 8);
    if (!find_bank(bank, track, sector) || !load_chain(track, sector, base))
      __bank_load_failed(bank);
  }
  F011.control = 0;
}
