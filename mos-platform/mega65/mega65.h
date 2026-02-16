// Copyright 2023 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

#ifndef _MEGA65_H
#define _MEGA65_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#pragma clang diagnostic ignored "-Wgnu-binary-literal"
#pragma clang diagnostic ignored "-Wfixed-enum-extension"
#endif

#include <_45E100.h>
#include <_6526.h>
#include <_dmagic.h>
#include <_sid.h>
#include <_vic2.h>
#include <_vic3.h>
#include <_vic4.h>

/// Hypervisor registers (0xD640-0xD67F)
struct __hypervisor {
  union {
    uint8_t htrap[64];
    struct {
      /// Hypervisor A register storage
      uint8_t rega; // 0xD640
      /// Hypervisor X register storage
      uint8_t regx;    // 0xD641
      uint8_t unused1; // 0xD642
      /// Hypervisor Z register storage
      uint8_t regz; // 0xD643
      /// Hypervisor B register storage
      uint8_t regb; // 0xD644
      /// Hypervisor SPL register storage
      uint8_t spl; // 0xD645
      /// Hypervisor SPH register storage
      uint8_t sph; // 0xD646
      /// Hypervisor P register storage
      uint8_t pflags; // 0xD647
      /// Hypervisor PC-low register storage
      uint8_t pcl; // 0xD648
      /// Hypervisor PC-high register storage
      uint8_t pch;     // 0xD649
      uint8_t maplo1;  // 0xD64A
      uint8_t maplo2;  // 0xD64B
      uint8_t maphi1;  // 0xD64C
      uint8_t maphi2;  // 0xD64D
      uint8_t maplomb; // 0xD64E
      uint8_t maphimb; // 0xD64F
      /// Hypervisor CPU port $00 value
      uint8_t port00; // 0xD650
      /// Hypervisor CPU port $01 value
      uint8_t port01;     // 0xD651
      uint8_t vicmode;    // 0xD652
      uint8_t dma_src_mb; // 0xD653
      /// Hypervisor DMAgic destination MB
      uint8_t dma_dst_hb; // 0xD654
      /// Hypervisor DMAGic list address
      uint32_t dmaladdr;   // 0xD655
      uint8_t vflop;       // 0xD659
      uint8_t unused2[22]; // 0xD65A
      /// Hypervisor GeoRAM base address (x MB)
      uint8_t georambase; // 0xD670
      /// Hypervisor GeoRAM address mask (applied to GeoRAM block register)
      uint8_t georammask; // 0xD671
      /// Enable composited Matrix Mode, and disable UART access to serial
      /// monitor.
      uint8_t matrixen;   // 0xD672
      uint8_t unused3[9]; // 0xD673
      /// Hypervisor write serial output to UART monitor
      uint8_t uartdata; // 0xD67C
      uint8_t watchdog; // 0xD67D
      /// Hypervisor already-upgraded bit (writing sets permanently)
      uint8_t hicked; // 0xD67E
      /// Writing trigger return from hypervisor
      uint8_t enterexit; // 0xD67F
    };
  };
};
#ifdef __cplusplus
static_assert(sizeof(struct __hypervisor) == 64);
#endif

/// Registers for the MEGA65 math accelerator
struct __cpu_math {
  union {
    uint8_t divout_fract8;  //!< Fractional part of MULTINA / MULTINB (0xD768)
    uint16_t divout_fract16;//!< Fractional part of MULTINA / MULTINB (0xD768)
    uint32_t divout_fract32;//!< Fractional part of MULTINA / MULTINB (0xD768)
  };
  union {
    uint8_t divout_whole8;  //!< Whole part of MULTINA / MULTINB (0xD76C)
    uint16_t divout_whole16;//!< Whole part of MULTINA / MULTINB (0xD76C)
    uint32_t divout_whole32;//!< Whole part of MULTINA / MULTINB (0xD76C)
  };
  union {
    uint8_t multina8;   //!< 8-bit Multiplier input A (0xD770)
    uint16_t multina16; //!< 16-bit Multiplier input A (0xD770)
    uint32_t multina32; //!< 32-bit Multiplier input A (0xD770)
  };
  union {
    uint8_t multinb8;   //!< 8-bit Multiplier input B (0xD774)
    uint16_t multinb16; //!< 16-bit Multiplier input B (0xD774)
    uint32_t multinb32; //!< 32-bit Multiplier input B (0xD774)
  };
  /// 64-bit product of MULTINA and MULTINB (0xD778)
  union {
    uint8_t multout8;
    uint16_t multout16;
    uint32_t multout32;
#ifdef __clang__
    uint64_t multout64;
#endif
    uint8_t multout[8];
  };
  uint32_t mathin[16]; //!< 32-bit programmable input (0xD780)
};
#ifdef __cplusplus
static_assert(sizeof(__cpu_math) == 88);
#endif

/// RGB color palette
struct __color_palette {
  uint8_t red[256];   //!< Red palette values (reversed nybl order)
  uint8_t green[256]; //!< Green palette values (reversed nybl order)
  uint8_t blue[256];  //!< Blue palette values (reversed nybl order)
};

/// 6510/45GS10 CPU port DDR
#define CPU_PORTDDR (*(volatile uint8_t *)0x0000)
/// 6510/45GS10 CPU port data
#define CPU_PORT (*(volatile uint8_t *)0x0001)
/// Default address of screen character matrix
#define DEFAULT_SCREEN (*(volatile uint8_t *)0x0800)
/// The VIC-II
#define VICII (*(volatile struct __vic2 *)0xd000)
/// The VIC IV
#define VICIV (*(volatile struct __vic4 *)0xd000)
/// Color palette
#define PALETTE (*(volatile struct __color_palette *)0xd100)
/// SID MOS 6581/8580
#define SID1 (*(volatile struct __sid *)0xd400)
/// SID MOS 6581/8580
#define SID2 (*(volatile struct __sid *)0xd420)
/// SID MOS 6581/8580
#define SID3 (*(volatile struct __sid *)0xd440)
/// SID MOS 6581/8580
#define SID4 (*(volatile struct __sid *)0xd460)
/// SID select mode (0=6581, 1=8580)
#define SIDMODE (*(volatile uint8_t *)0xd63c)
/// Hypervisor traps
#define HYPERVISOR (*(volatile struct __hypervisor *)0xd640)
/// Ethernet controller
#define ETHERNET (*(volatile struct __45E100 *)0xd6e0)
/// DMAgic DMA controller
#define DMA (*(volatile struct DMAgicController *)0xd700)
/// Math busy flag
#define MATHBUSY (*(volatile uint8_t *)0xd70f)
/// Math accelerator
#define MATH (*(volatile struct __cpu_math *)0xd768)
/// The CIA 1
#define CIA1 (*(volatile struct __6526 *)0xdc00)
/// The CIA 2
#define CIA2 (*(volatile struct __6526 *)0xdd00)

/// Default color palette
enum
#ifdef __clang__
    : uint8_t
#endif
{
  COLOR_BLACK = 0x00,
  COLOR_WHITE = 0x01,
  COLOR_RED = 0x02,
  COLOR_CYAN = 0x03,
  COLOR_PURPLE = 0x04,
  COLOR_GREEN = 0x05,
  COLOR_BLUE = 0x06,
  COLOR_YELLOW = 0x07,
  COLOR_ORANGE = 0x08,
  COLOR_BROWN = 0x09,
  COLOR_LIGHTRED = 0x0A,
  COLOR_DARKGREY = 0x0B,
  COLOR_MIDGREY = 0x0C,
  COLOR_LIGHTGREEN = 0x0D,
  COLOR_LIGHTBLUE = 0x0E,
  COLOR_LIGHTGREY = 0x0F,
  COLOR_GURUMEDITATION = 0x10,
  COLOR_RAMBUTAN = 0x11,
  COLOR_CARROT = 0x12,
  COLOR_LEMONTART = 0x13,
  COLOR_PANDAN = 0x14,
  COLOR_SEASICKGREEN = 0x15,
  COLOR_SOYLENTGREEN = 0x16,
  COLOR_SLIMERGREEN = 0x17,
  COLOR_THEOTHERCYAN = 0x18,
  COLOR_SEASKY = 0x19,
  COLOR_SMURFBLUE = 0x1A,
  COLOR_SCREENOFDEATH = 0x1B,
  COLOR_PLUMSAUCE = 0x1C,
  COLOR_SOURGRAPE = 0x1D,
  COLOR_BUBBLEGUM = 0x1E,
  COLOR_HOTTAMALES = 0x1F
};

/*****************************************************************************/
/*                      MEGA65 KERNAL function wrappers                      */
/*****************************************************************************/

/// Return type for mega65_k_scrorg()
typedef struct {
  unsigned char width;    ///< Window width in columns
  unsigned char height;   ///< Window height in rows
  unsigned char addr_lo;  ///< Window top-left screen address, low byte
  unsigned char addr_hi;  ///< Window top-left screen address, high byte
  unsigned char is_40col; ///< 0=80 column mode, 1=40 column mode
} mega65_screen_info_t;

/// Return type for mega65_k_rdtim()
typedef struct {
  unsigned char hours;   ///< Hours 0-23 in BCD (e.g. 0x14 = 14)
  unsigned char minutes; ///< Minutes 0-59 in BCD (e.g. 0x30 = 30)
  unsigned char seconds; ///< Seconds 0-59 in BCD (e.g. 0x57 = 57)
  unsigned char tenths;  ///< Tenths of a second 0-9
} mega65_tod_t;

/// Add a PETSCII character to the keyboard input buffer.
/// The buffer is consumed by GETIN calls, not by the KERNAL IRQ.
///
/// @param petscii_char  PETSCII character code
/// @return 0 on success, 1 if the buffer is full
unsigned char mega65_k_addkey(unsigned char petscii_char);

/// Close all open files on the specified device.
/// Restores default I/O channels if the current channel was on that device.
///
/// @param device  Device number (0-31)
void mega65_k_close_all(unsigned char device);

/// Enable blinking cursor at the current screen editor position.
/// Cursor animation is handled by the screen editor IRQ.
void mega65_k_cursor_enable(void);

/// Disable blinking cursor.
void mega65_k_cursor_disable(void);

/// Read the current input and output devices.
/// Modified by CHKIN and CKOUT.
///
/// @param input_dev   Pointer to receive input device (0=keyboard)
/// @param output_dev  Pointer to receive output device (3=screen)
void mega65_k_getio(unsigned char *input_dev, unsigned char *output_dev);

/// Read the current file parameters (logical address, device, secondary address).
/// Useful to determine the boot device before performing other disk I/O.
///
/// @param la  Pointer to receive logical file number
/// @param fa  Pointer to receive device number
/// @param sa  Pointer to receive secondary address (0xFF if not set)
void mega65_k_getlfs(unsigned char *la, unsigned char *fa, unsigned char *sa);

/// Search for a logical file number in use.
///
/// @param la  Logical file number to search for
/// @param fa  Pointer to receive device number (if found)
/// @param sa  Pointer to receive secondary address (if found)
/// @return 0 if found, 1 if not found
unsigned char mega65_k_lkupla(unsigned char la, unsigned char *fa,
                               unsigned char *sa);

/// Search for a secondary address in use.
///
/// @param sa  Secondary address to search for
/// @param la  Pointer to receive logical file number (if found)
/// @param fa  Pointer to receive device number (if found)
/// @return 0 if found, 1 if not found
unsigned char mega65_k_lkupsa(unsigned char sa, unsigned char *la,
                               unsigned char *fa);

/// Load or verify a file. Supports MEGA65 raw mode (flag bit 6).
/// flag: 0x00=load to address, 0x01=verify, 0x40=raw load, 0x41=raw verify.
/// Raw mode treats the first two bytes as data instead of a PRG header.
/// Requires SETBNK, SETLFS, SETNAM called first.
///
/// @param flag       Load flags (0=load, 1=verify, 0x40=raw load, 0x41=raw
/// verify)
/// @param load_addr  Destination address (used if SA=0; ignored if SA!=0)
/// @param end_addr   Pointer to receive end address+1 on success
/// @return 0 on success, or KERNAL error code (1-9)
unsigned char mega65_k_load(unsigned char flag, void *load_addr,
                             void **end_addr);

/// Get cursor position relative to the active window.
///
/// @param line  Pointer to receive cursor line (0-based)
/// @param col   Pointer to receive cursor column (0-based)
void mega65_k_plot_get(unsigned char *line, unsigned char *col);

/// Set cursor position relative to the active window.
///
/// @param line  Cursor line (0-based)
/// @param col   Cursor column (0-based)
void mega65_k_plot_set(unsigned char line, unsigned char col);

/// Read the CIA1 time-of-day clock.
/// Returns hours, minutes, seconds (BCD), and tenths of a second.
/// This reads the CIA1 TOD, not the battery-backed RTC.
mega65_tod_t mega65_k_rdtim(void);

/// Get screen window size and properties (SCRORG).
/// Returns the dimensions and memory address of the active text window.
mega65_screen_info_t mega65_k_scrorg(void);

/// Set memory bank and filename bank for I/O operations (LOAD, SAVE, OPEN).
/// For simple bank mode (banks 0-5 in the first megabyte).
/// Must be called before LOAD, SAVE, or OPEN.
///
/// @param mem_bank Memory bank (0-5)
/// @param fn_bank  Filename bank (0-5)
void mega65_k_setbnk(unsigned char mem_bank, unsigned char fn_bank);

/// Set 28-bit memory and filename addresses for I/O operations.
/// For accessing addresses beyond the first megabyte.
/// The lower 16 bits of each address are set via other calls (SETNAM, LOAD).
///
/// @param mem_mb  Memory megabyte (bits 24-27, 0x0-0xF)
/// @param mem_hi  Memory address bits 16-23
/// @param fn_mb   Filename megabyte (bits 24-27, 0x0-0xF)
/// @param fn_hi   Filename address bits 16-23
void mega65_k_setbnk_28(unsigned char mem_mb, unsigned char mem_hi,
                         unsigned char fn_mb, unsigned char fn_hi);

/// Save memory to a file with optional raw mode (MEGA65 SAVEFL, $FF3B).
/// In raw mode, the two-byte PRG address header is omitted.
/// The memory region must fit within a single bank.
/// Requires SETBNK, SETLFS, SETNAM called first.
///
/// @param start_addr       Start of memory region to save
/// @param end_addr_plus1   End address + 1 (first byte NOT saved)
/// @param raw              true for raw mode (omit PRG header)
/// @return 0 on success, or KERNAL error code (1-9)
unsigned char mega65_k_savefl(const void *start_addr,
                               const void *end_addr_plus1, bool raw);

/// Enable or disable KERNAL messages (LOADING, SAVING, I/O ERROR).
/// These are disabled by default.
///
/// @param mode  Bit 7 = control messages, bit 6 = error messages
void mega65_k_setmsg(unsigned char mode);

/// Set the CIA1 time-of-day clock.
/// All values are in BCD format (e.g. 0x59 = 59 decimal).
/// This sets the CIA1 TOD, not the battery-backed RTC.
///
/// @param hours    Hours 0-23 in BCD
/// @param minutes  Minutes 0-59 in BCD
/// @param seconds  Seconds 0-59 in BCD
/// @param tenths   Tenths of a second 0-9
void mega65_k_settim(unsigned char hours, unsigned char minutes,
                      unsigned char seconds, unsigned char tenths);

/// Toggle between 40x25 and 80x25 text modes.
void mega65_k_swapper(void);

/*****************************************************************************/
/*                   Hyppo hypervisor service wrappers                       */
/*****************************************************************************/

/// Set the Hyppo filename for subsequent find/load operations.
/// Filename is ASCII (not PETSCII), null-terminated, max 63 characters.
/// Operates on the SD card FAT filesystem, not D81 disk images.
///
/// @param filename  ASCII filename string
/// @return 0 on success, Hyppo error code on failure
uint8_t mega65_h_setname(const char *filename);

/// Load a file from the SD card into chip memory at a 28-bit address.
/// Call mega65_h_setname() first. Loads the entire file at once.
///
/// @param addr  28-bit destination in chip memory ($000000-$FFFFFF)
/// @return 0 on success, Hyppo error code on failure
uint8_t mega65_h_loadfile(uint32_t addr);

/// Load a file from the SD card into attic/hyper RAM.
/// Call mega65_h_setname() first. Loads the entire file at once.
///
/// @param addr  24-bit offset in attic RAM (base $08000000 added by hardware)
/// @return 0 on success, Hyppo error code on failure
uint8_t mega65_h_loadfile_attic(uint32_t addr);

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef __cplusplus
} // extern block
#endif
#endif // _MEGA65_H
