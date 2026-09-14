// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

#ifndef _F011_H
#define _F011_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// F011 floppy controller, the internal drive or a mounted D81 image.
///
/// Sector data does not pass through these registers: a read fills a 512-byte
/// buffer at 0xFFD6C00, visible only while SDCARD.control has SD_BUFFSEL_MASK
/// clear.
struct __f011 {
  uint8_t control; //!< Drive select, SIDE, SWAP, motor and LED (offset 0x00)
  uint8_t command; //!< F011_CMD_*; writing starts it (offset 0x01)
  uint8_t status1; //!< F011_BUSY_MASK and error bits (offset 0x02)
  uint8_t status2; //!< Disk change, index and write-protect sense (offset 0x03)
  uint8_t track;   //!< Track to act on, from 0 (offset 0x04)
  uint8_t sector;  //!< Sector to act on, 1-10 per side (offset 0x05)
  uint8_t side;    //!< Side to act on, 0 or 1 (offset 0x06)
  uint8_t data;    //!< Sequential port into the sector buffer (offset 0x07)
};

#ifdef __cplusplus
static_assert(sizeof(struct __f011) == 8);
#else
_Static_assert(sizeof(struct __f011) == 8, "F011 is $D080-$D087");
#endif

/* Bits from sdcardio.vhdl. */
enum {
  F011_MOTOR_MASK = 0b00100000, /** control: motor and steady LED */
  F011_CMD_READ = 0x40,         /** command: read the requested sector */
  F011_CMD_NOBUF = 0x01,        /** command: reset the buffer pointer first */
  F011_BUSY_MASK = 0b10000000,  /** status1: a command is running */
  F011_RNF_MASK = 0b00010000,   /** status1: sector not found, or no disk */
  F011_CRC_MASK = 0b00001000,   /** status1: read error */
};

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _F011_H
