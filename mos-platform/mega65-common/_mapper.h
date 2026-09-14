// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// The half of <mapper.h> that does not depend on where the banks are. Each
// banked platform's own mapper.h supplies the addresses and includes this.

#ifndef _MEGA65_MAPPER_COMMON_H_
#define _MEGA65_MAPPER_COMMON_H_

/**
 * @brief Declare how many banks this program uses.
 *
 * Banks above @p n are given zero length by the linker script, so nothing is
 * reserved for them and they are left out of the output. Without this every
 * program reserves all 15, nearly all of it zeroes that still have to be read
 * back at boot.
 *
 * Place it once at file scope: MAPPER_BANK_COUNT(3);
 *
 * @param n Highest bank number used (1-15). Bank 0 needs no declaration; it is
 *          the unmapped default.
 */
#define MAPPER_BANK_COUNT(n)                                                   \
  asm(".globl __ram_bank_count\n__ram_bank_count = " #n)

/**
 * @brief Place a function or read-only data in bank @p n.
 *
 *     CODE_BANK(1) void draw(void) { ... }
 *     RODATA_BANK(1) const uint8_t sine[256] = { ... };
 *
 * Functions need "noinline" or the compiler may inline them back into the
 * fixed region and leave the bank empty; CODE_BANK supplies it. Data that
 * nothing references needs "used" and "retain" to survive the compiler and
 * then --gc-sections; RODATA_BANK supplies both.
 *
 * Keep a table in the same bank as the code that reads it, so one banked_call
 * covers both.
 */
/* Two-step stringify, as atari2600-common/mapper_macros.h does it, so the
 * bank number may itself be a macro rather than only a literal. */
#define _BANK_STRINGIFY(n) #n
#define _BANK_SECTION(n) ".bank_" _BANK_STRINGIFY(n)
#define _CODE_BANK(sect) __attribute__((noinline, section(sect)))
#define _RODATA_BANK(sect) __attribute__((used, retain, section(sect)))
#define CODE_BANK(n) _CODE_BANK(_BANK_SECTION(n))
#define RODATA_BANK(n) _RODATA_BANK(_BANK_SECTION(n) ".rodata")

/**
 * @brief Interrupt handlers and banking.
 *
 * The map is global state: a handler runs with whichever bank the interrupted
 * code had mapped, and cannot know which that was. Handlers must live in the
 * fixed region and must not touch the bank window.
 *
 * Bank switching leaves the I flag alone, so set_bank() and banked_call()
 * return with the caller's interrupt state intact and an interrupt can arrive
 * at any point between them.
 */

#ifndef __ASSEMBLER__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Call a function in the given bank.
 *
 * Maps the bank into the window, calls the function, then restores the
 * previous bank.
 *
 * A banked function may call this too. The trampoline is in the fixed region
 * and the previous bank rides on the hardware stack, so the caller's bank is
 * back in the window before control returns to it -- it is absent only while
 * its own code is not running. Nesting is bounded by the hardware stack.
 *
 * @param bank_id Bank number (0-15).
 * @param method  Function pointer (address within the window).
 */
/* "leaf" would normally be a lie here -- this re-enters C through
 * __call_indir. It is sound only because callback(2) restores the call edge
 * that leaf severs, so LLVM still sees the indirect target when it lays out
 * static stack frames. Do not drop either attribute. */
__attribute__((leaf, callback(2))) void banked_call(char bank_id,
                                                    void (*method)(void));

/**
 * @brief Get the currently mapped bank number.
 */
__attribute__((leaf)) char get_bank(void);

/**
 * @brief Put the hardware map back in step with get_bank().
 *
 * Only needed after something else has installed its own map and restored a
 * different one.
 */
__attribute__((leaf)) void resync_bank(void);

/**
 * @brief Switch to the given bank.
 *
 * Experts only -- prefer banked_call(). The caller must be in fixed code.
 *
 * @param bank_id Bank number, masked to its low four bits: 0x11 selects
 *                bank 1. An unmasked id would index past the bank tables
 *                and hand junk to MAP, which covers $0000-$7FFF and so
 *                could move zero page out from under the compiler.
 */
__attribute__((leaf)) void set_bank(char bank_id);

/* The switch banked_call_r and banked_call_v use; see there. */
__attribute__((leaf)) char __banked_call_enter(char bank_id);

/**
 * @brief Called when startup cannot load a bank the program declared.
 *
 * Weak: define it to handle the failure. The default halts with a red border,
 * since the bank would read back as zeroes. Returning goes on to the next bank.
 */
void __bank_load_failed(unsigned char bank);

/* What the loaders read: from the tables below and bank-sizes.s. */
extern const unsigned char __bank_used[15];
extern const unsigned char __bank_megabyte[16];
extern const unsigned char __bank_addr_mid[16];
extern const unsigned char __bank_addr_page[16];

#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLER__ */

/**
 * @brief Call a banked function with arguments, and with a return value.
 *
 *     int n = banked_call_r(1, measure, text, len);
 *     banked_call_v(1, draw, x, y);
 *
 * Two macros because a statement expression cannot hold a void temporary:
 * _r yields the call's value, _v is for functions returning void.
 *
 * The call is direct, so the compiler marshals the real signature and sees
 * the call edge -- unlike banked_call(), which hides both behind a function
 * pointer. This works because the fixed region is above the window: MAPLO
 * moves nothing at $8000 and up, so the caller's own code survives the
 * switch.
 *
 * So the caller must be in the fixed region; from a bank the switch would unmap
 * it mid-call, and the link fails. Bank callers want banked_call(). Up to eight
 * arguments are evaluated before the switch, so they may read the bank mapped
 * at the call.
 */
#define _BANKED_NARGS(...)                                                     \
  _BANKED_NARGS_(__VA_OPT__(, ) __VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define _BANKED_NARGS_(z, a, b, c, d, e, f, g, h, n, ...) n
#define _BANKED_CAT(a, b) _BANKED_CAT_(a, b)
#define _BANKED_CAT_(a, b) a##b
#define _BANKED_T0()
#define _BANKED_T1(a) _BANKED_T0() __auto_type _a1 = (a);
#define _BANKED_T2(a, b) _BANKED_T1(a) __auto_type _a2 = (b);
#define _BANKED_T3(a, b, c) _BANKED_T2(a, b) __auto_type _a3 = (c);
#define _BANKED_T4(a, b, c, d) _BANKED_T3(a, b, c) __auto_type _a4 = (d);
#define _BANKED_T5(a, b, c, d, e) _BANKED_T4(a, b, c, d) __auto_type _a5 = (e);
#define _BANKED_T6(a, b, c, d, e, f)                                           \
  _BANKED_T5(a, b, c, d, e) __auto_type _a6 = (f);
#define _BANKED_T7(a, b, c, d, e, f, g)                                        \
  _BANKED_T6(a, b, c, d, e, f) __auto_type _a7 = (g);
#define _BANKED_T8(a, b, c, d, e, f, g, h)                                     \
  _BANKED_T7(a, b, c, d, e, f, g) __auto_type _a8 = (h);
#define _BANKED_A0()
#define _BANKED_A1(a) _a1
#define _BANKED_A2(a, b) _a1, _a2
#define _BANKED_A3(a, b, c) _a1, _a2, _a3
#define _BANKED_A4(a, b, c, d) _a1, _a2, _a3, _a4
#define _BANKED_A5(a, b, c, d, e) _a1, _a2, _a3, _a4, _a5
#define _BANKED_A6(a, b, c, d, e, f) _a1, _a2, _a3, _a4, _a5, _a6
#define _BANKED_A7(a, b, c, d, e, f, g) _a1, _a2, _a3, _a4, _a5, _a6, _a7
#define _BANKED_A8(a, b, c, d, e, f, g, h)                                     \
  _a1, _a2, _a3, _a4, _a5, _a6, _a7, _a8
#define _BANKED_TEMPS(...)                                                     \
  _BANKED_CAT(_BANKED_T, _BANKED_NARGS(__VA_ARGS__))(__VA_ARGS__)
#define _BANKED_ARGS(...)                                                      \
  _BANKED_CAT(_BANKED_A, _BANKED_NARGS(__VA_ARGS__))(__VA_ARGS__)

#define banked_call_r(bank, fn, ...)                                           \
  ({                                                                           \
    _BANKED_TEMPS(__VA_ARGS__)                                                 \
    char _prev_bank = __banked_call_enter(bank);                               \
    __auto_type _result = (fn)(_BANKED_ARGS(__VA_ARGS__));                     \
    set_bank(_prev_bank);                                                      \
    _result;                                                                   \
  })

#define banked_call_v(bank, fn, ...)                                           \
  ({                                                                           \
    _BANKED_TEMPS(__VA_ARGS__)                                                 \
    char _prev_bank = __banked_call_enter(bank);                               \
    (fn)(_BANKED_ARGS(__VA_ARGS__));                                           \
    set_bank(_prev_bank);                                                      \
  })


/**
 * @brief Where each bank lives, and how big it is.
 *
 * BANK_PHYS_BASE_n is MAPPER_BANK_n where the program defines it, else the
 * platform default. Define overrides the same way in every file, before
 * <mapper.h>: with -D, or in a header passed with -include.
 *
 *     -DMAPPER_BANK_13=0x8010000
 *
 * MAPPER_WINDOW_KB (24, 16 or 8) sizes the window. MAPPER_BANK_n_KB makes bank
 * n smaller than that; BANK_SIZE_n is the result in bytes.
 *
 * MAPPER_LOADER_SD loads the banks off the SD card through Hyppo, the default
 * without the KERNAL. MAPPER_LOADER_FLOPPY, KERNAL-free only, loads them off
 * the mounted D81 through the F011. The converter writes what the loader reads.
 *
 * Each file including <mapper.h> emits the tables the bank switch and the
 * loaders read, as identical weak definitions, and records the layout it saw;
 * the converter rejects a program whose files disagree.
 */
#ifdef _MAPPER_DEFAULT_BANK_1

#ifdef __ASSEMBLER__
#define _MAPPER_UL(x) x
#else
#define _MAPPER_UL(x) x##ul
#endif

#ifndef MAPPER_WINDOW_KB
#define MAPPER_WINDOW_KB 24
#endif

#if defined(MAPPER_LOADER_SD) && defined(MAPPER_LOADER_FLOPPY)
#error "define one of MAPPER_LOADER_SD and MAPPER_LOADER_FLOPPY"
#elif defined(MAPPER_LOADER_SD)
#define _MAPPER_LOADER_ID 0
#elif defined(MAPPER_LOADER_FLOPPY)
#define _MAPPER_LOADER_ID 1
#else
#define _MAPPER_LOADER_ID _MAPPER_DEFAULT_LOADER
#endif

#define BANK_PHYS_BASE_0 _MAPPER_UL(0x02000)
#define _MAPPER_KB_0 MAPPER_WINDOW_KB
#ifdef MAPPER_BANK_1
#define BANK_PHYS_BASE_1 (MAPPER_BANK_1)
#else
#define BANK_PHYS_BASE_1 _MAPPER_DEFAULT_BANK_1
#endif
#ifdef MAPPER_BANK_1_KB
#define _MAPPER_KB_1 (MAPPER_BANK_1_KB)
#else
#define _MAPPER_KB_1 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_2
#define BANK_PHYS_BASE_2 (MAPPER_BANK_2)
#else
#define BANK_PHYS_BASE_2 _MAPPER_DEFAULT_BANK_2
#endif
#ifdef MAPPER_BANK_2_KB
#define _MAPPER_KB_2 (MAPPER_BANK_2_KB)
#else
#define _MAPPER_KB_2 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_3
#define BANK_PHYS_BASE_3 (MAPPER_BANK_3)
#else
#define BANK_PHYS_BASE_3 _MAPPER_DEFAULT_BANK_3
#endif
#ifdef MAPPER_BANK_3_KB
#define _MAPPER_KB_3 (MAPPER_BANK_3_KB)
#else
#define _MAPPER_KB_3 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_4
#define BANK_PHYS_BASE_4 (MAPPER_BANK_4)
#else
#define BANK_PHYS_BASE_4 _MAPPER_DEFAULT_BANK_4
#endif
#ifdef MAPPER_BANK_4_KB
#define _MAPPER_KB_4 (MAPPER_BANK_4_KB)
#else
#define _MAPPER_KB_4 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_5
#define BANK_PHYS_BASE_5 (MAPPER_BANK_5)
#else
#define BANK_PHYS_BASE_5 _MAPPER_DEFAULT_BANK_5
#endif
#ifdef MAPPER_BANK_5_KB
#define _MAPPER_KB_5 (MAPPER_BANK_5_KB)
#else
#define _MAPPER_KB_5 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_6
#define BANK_PHYS_BASE_6 (MAPPER_BANK_6)
#else
#define BANK_PHYS_BASE_6 _MAPPER_DEFAULT_BANK_6
#endif
#ifdef MAPPER_BANK_6_KB
#define _MAPPER_KB_6 (MAPPER_BANK_6_KB)
#else
#define _MAPPER_KB_6 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_7
#define BANK_PHYS_BASE_7 (MAPPER_BANK_7)
#else
#define BANK_PHYS_BASE_7 _MAPPER_DEFAULT_BANK_7
#endif
#ifdef MAPPER_BANK_7_KB
#define _MAPPER_KB_7 (MAPPER_BANK_7_KB)
#else
#define _MAPPER_KB_7 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_8
#define BANK_PHYS_BASE_8 (MAPPER_BANK_8)
#else
#define BANK_PHYS_BASE_8 _MAPPER_DEFAULT_BANK_8
#endif
#ifdef MAPPER_BANK_8_KB
#define _MAPPER_KB_8 (MAPPER_BANK_8_KB)
#else
#define _MAPPER_KB_8 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_9
#define BANK_PHYS_BASE_9 (MAPPER_BANK_9)
#else
#define BANK_PHYS_BASE_9 _MAPPER_DEFAULT_BANK_9
#endif
#ifdef MAPPER_BANK_9_KB
#define _MAPPER_KB_9 (MAPPER_BANK_9_KB)
#else
#define _MAPPER_KB_9 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_10
#define BANK_PHYS_BASE_10 (MAPPER_BANK_10)
#else
#define BANK_PHYS_BASE_10 _MAPPER_DEFAULT_BANK_10
#endif
#ifdef MAPPER_BANK_10_KB
#define _MAPPER_KB_10 (MAPPER_BANK_10_KB)
#else
#define _MAPPER_KB_10 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_11
#define BANK_PHYS_BASE_11 (MAPPER_BANK_11)
#else
#define BANK_PHYS_BASE_11 _MAPPER_DEFAULT_BANK_11
#endif
#ifdef MAPPER_BANK_11_KB
#define _MAPPER_KB_11 (MAPPER_BANK_11_KB)
#else
#define _MAPPER_KB_11 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_12
#define BANK_PHYS_BASE_12 (MAPPER_BANK_12)
#else
#define BANK_PHYS_BASE_12 _MAPPER_DEFAULT_BANK_12
#endif
#ifdef MAPPER_BANK_12_KB
#define _MAPPER_KB_12 (MAPPER_BANK_12_KB)
#else
#define _MAPPER_KB_12 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_13
#define BANK_PHYS_BASE_13 (MAPPER_BANK_13)
#else
#define BANK_PHYS_BASE_13 _MAPPER_DEFAULT_BANK_13
#endif
#ifdef MAPPER_BANK_13_KB
#define _MAPPER_KB_13 (MAPPER_BANK_13_KB)
#else
#define _MAPPER_KB_13 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_14
#define BANK_PHYS_BASE_14 (MAPPER_BANK_14)
#else
#define BANK_PHYS_BASE_14 _MAPPER_DEFAULT_BANK_14
#endif
#ifdef MAPPER_BANK_14_KB
#define _MAPPER_KB_14 (MAPPER_BANK_14_KB)
#else
#define _MAPPER_KB_14 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_15
#define BANK_PHYS_BASE_15 (MAPPER_BANK_15)
#else
#define BANK_PHYS_BASE_15 _MAPPER_DEFAULT_BANK_15
#endif
#ifdef MAPPER_BANK_15_KB
#define _MAPPER_KB_15 (MAPPER_BANK_15_KB)
#else
#define _MAPPER_KB_15 MAPPER_WINDOW_KB
#endif

#define BANK_SIZE_0 (_MAPPER_KB_0 * _MAPPER_UL(1024))
#define BANK_SIZE_1 (_MAPPER_KB_1 * _MAPPER_UL(1024))
#define BANK_SIZE_2 (_MAPPER_KB_2 * _MAPPER_UL(1024))
#define BANK_SIZE_3 (_MAPPER_KB_3 * _MAPPER_UL(1024))
#define BANK_SIZE_4 (_MAPPER_KB_4 * _MAPPER_UL(1024))
#define BANK_SIZE_5 (_MAPPER_KB_5 * _MAPPER_UL(1024))
#define BANK_SIZE_6 (_MAPPER_KB_6 * _MAPPER_UL(1024))
#define BANK_SIZE_7 (_MAPPER_KB_7 * _MAPPER_UL(1024))
#define BANK_SIZE_8 (_MAPPER_KB_8 * _MAPPER_UL(1024))
#define BANK_SIZE_9 (_MAPPER_KB_9 * _MAPPER_UL(1024))
#define BANK_SIZE_10 (_MAPPER_KB_10 * _MAPPER_UL(1024))
#define BANK_SIZE_11 (_MAPPER_KB_11 * _MAPPER_UL(1024))
#define BANK_SIZE_12 (_MAPPER_KB_12 * _MAPPER_UL(1024))
#define BANK_SIZE_13 (_MAPPER_KB_13 * _MAPPER_UL(1024))
#define BANK_SIZE_14 (_MAPPER_KB_14 * _MAPPER_UL(1024))
#define BANK_SIZE_15 (_MAPPER_KB_15 * _MAPPER_UL(1024))

#ifndef __ASSEMBLER__

/**
 * @brief Place uninitialised data at the top of the window, above the smallest
 * bank.
 *
 * Those bytes are the window's own RAM: readable while bank 0 or a smallest
 * bank is mapped, and covered by any larger bank. Without a bank smaller than
 * MAPPER_WINDOW_KB there is no room, and using it fails to link.
 */
#define WINDOW_TAIL __attribute__((section(".window_tail")))

#ifdef __cplusplus
#define _MAPPER_ASSERT(c, m) static_assert(c, m)
#define _MAPPER_EXTERN extern
#else
#define _MAPPER_ASSERT(c, m) _Static_assert(c, m)
#define _MAPPER_EXTERN
#endif

#define _MAPPER_VALID_KB(kb) ((kb) == 24 || (kb) == 16 || (kb) == 8)
_MAPPER_ASSERT(_MAPPER_VALID_KB(MAPPER_WINDOW_KB),
               "MAPPER_WINDOW_KB must be 24, 16 or 8");

/* MAP offsets are page-granular, and a bank must fit in chip RAM, which ends
 * at $60000, or in attic RAM. */
#define _MAPPER_CHECK(n)                                                       \
  _MAPPER_ASSERT(_MAPPER_VALID_KB(_MAPPER_KB_##n) &&                           \
                     _MAPPER_KB_##n <= MAPPER_WINDOW_KB,                       \
                 "MAPPER_BANK_" #n "_KB must be 24, 16 or 8, and at most "     \
                 "MAPPER_WINDOW_KB");                                          \
  _MAPPER_ASSERT((BANK_PHYS_BASE_##n & 0xFF) == 0,                             \
                 "MAPPER_BANK_" #n " must be page-aligned");                   \
  _MAPPER_ASSERT(BANK_PHYS_BASE_##n + BANK_SIZE_##n <= 0x60000ul ||            \
                     (BANK_PHYS_BASE_##n >= 0x8000000ul &&                     \
                      BANK_PHYS_BASE_##n + BANK_SIZE_##n <= 0x8800000ul),      \
                 "MAPPER_BANK_" #n " must lie below $60000 or in attic RAM");  \
  _MAPPER_ASSERT(BANK_PHYS_BASE_##n >> 20 ==                                   \
                     (BANK_PHYS_BASE_##n + BANK_SIZE_##n - 1) >> 20,           \
                 "MAPPER_BANK_" #n " must not cross a megabyte boundary");     \
  _MAPPER_PLATFORM_CHECK(n)
_MAPPER_CHECK(1)
_MAPPER_CHECK(2)
_MAPPER_CHECK(3)
_MAPPER_CHECK(4)
_MAPPER_CHECK(5)
_MAPPER_CHECK(6)
_MAPPER_CHECK(7)
_MAPPER_CHECK(8)
_MAPPER_CHECK(9)
_MAPPER_CHECK(10)
_MAPPER_CHECK(11)
_MAPPER_CHECK(12)
_MAPPER_CHECK(13)
_MAPPER_CHECK(14)
_MAPPER_CHECK(15)

#define _MAPPER_OFF(n)                                                         \
  (((BANK_PHYS_BASE_##n & 0xFFFFFul) - 0x2000ul) & 0xFFFFFul)
#define _MAPPER_OFFSET_LO(n) (unsigned char)(_MAPPER_OFF(n) >> 8)
/* MAPLO selects 8 KB blocks from $2000: 1-3, 1-2 or just 1. */
#define _MAPPER_SELECT(kb) ((kb) == 24 ? 0xE0 : (kb) == 16 ? 0x60 : 0x20)
#define _MAPPER_MAPLO_SEL(n)                                                   \
  (unsigned char)(n ? _MAPPER_SELECT(_MAPPER_KB_##n) | _MAPPER_OFF(n) >> 16 : 0)
#define _MAPPER_MEGABYTE(n) (unsigned char)(BANK_PHYS_BASE_##n >> 20)
#define _MAPPER_ADDR_MID(n) (unsigned char)(BANK_PHYS_BASE_##n >> 16)
#define _MAPPER_ADDR_PAGE(n) (unsigned char)(BANK_PHYS_BASE_##n >> 8)
#define _MAPPER_TABLE                                                          \
  __attribute__((weak, used, section(".rodata.bank_tables")))

/* The linker reads each size off a marker's alignment (8 KB as 2, 16 KB as 4,
 * 24 KB as 8), the one section property that combines alike with LTO and
 * without. */
#define _MAPPER_ALIGN(kb) ((kb) == 24 ? 8 : (kb) / 4)
#define _MAPPER_MARKER(name, sect, kb)                                         \
  __attribute__((weak, used, section(sect), aligned(_MAPPER_ALIGN(kb))))       \
  _MAPPER_EXTERN const unsigned char name = 0;

#ifdef __cplusplus
extern "C" {
#endif
_MAPPER_TABLE _MAPPER_EXTERN const unsigned char __bank_offset_lo[16] = {
    _MAPPER_OFFSET_LO(0), _MAPPER_OFFSET_LO(1), _MAPPER_OFFSET_LO(2),
    _MAPPER_OFFSET_LO(3), _MAPPER_OFFSET_LO(4), _MAPPER_OFFSET_LO(5),
    _MAPPER_OFFSET_LO(6), _MAPPER_OFFSET_LO(7), _MAPPER_OFFSET_LO(8),
    _MAPPER_OFFSET_LO(9), _MAPPER_OFFSET_LO(10), _MAPPER_OFFSET_LO(11),
    _MAPPER_OFFSET_LO(12), _MAPPER_OFFSET_LO(13), _MAPPER_OFFSET_LO(14),
    _MAPPER_OFFSET_LO(15)};
_MAPPER_TABLE _MAPPER_EXTERN const unsigned char __bank_maplo_sel[16] = {
    _MAPPER_MAPLO_SEL(0), _MAPPER_MAPLO_SEL(1), _MAPPER_MAPLO_SEL(2),
    _MAPPER_MAPLO_SEL(3), _MAPPER_MAPLO_SEL(4), _MAPPER_MAPLO_SEL(5),
    _MAPPER_MAPLO_SEL(6), _MAPPER_MAPLO_SEL(7), _MAPPER_MAPLO_SEL(8),
    _MAPPER_MAPLO_SEL(9), _MAPPER_MAPLO_SEL(10), _MAPPER_MAPLO_SEL(11),
    _MAPPER_MAPLO_SEL(12), _MAPPER_MAPLO_SEL(13), _MAPPER_MAPLO_SEL(14),
    _MAPPER_MAPLO_SEL(15)};
_MAPPER_TABLE _MAPPER_EXTERN const unsigned char __bank_megabyte[16] = {
    _MAPPER_MEGABYTE(0), _MAPPER_MEGABYTE(1), _MAPPER_MEGABYTE(2),
    _MAPPER_MEGABYTE(3), _MAPPER_MEGABYTE(4), _MAPPER_MEGABYTE(5),
    _MAPPER_MEGABYTE(6), _MAPPER_MEGABYTE(7), _MAPPER_MEGABYTE(8),
    _MAPPER_MEGABYTE(9), _MAPPER_MEGABYTE(10), _MAPPER_MEGABYTE(11),
    _MAPPER_MEGABYTE(12), _MAPPER_MEGABYTE(13), _MAPPER_MEGABYTE(14),
    _MAPPER_MEGABYTE(15)};
_MAPPER_TABLE _MAPPER_EXTERN const unsigned char __bank_addr_mid[16] = {
    _MAPPER_ADDR_MID(0), _MAPPER_ADDR_MID(1), _MAPPER_ADDR_MID(2),
    _MAPPER_ADDR_MID(3), _MAPPER_ADDR_MID(4), _MAPPER_ADDR_MID(5),
    _MAPPER_ADDR_MID(6), _MAPPER_ADDR_MID(7), _MAPPER_ADDR_MID(8),
    _MAPPER_ADDR_MID(9), _MAPPER_ADDR_MID(10), _MAPPER_ADDR_MID(11),
    _MAPPER_ADDR_MID(12), _MAPPER_ADDR_MID(13), _MAPPER_ADDR_MID(14),
    _MAPPER_ADDR_MID(15)};
_MAPPER_TABLE _MAPPER_EXTERN const unsigned char __bank_addr_page[16] = {
    _MAPPER_ADDR_PAGE(0), _MAPPER_ADDR_PAGE(1), _MAPPER_ADDR_PAGE(2),
    _MAPPER_ADDR_PAGE(3), _MAPPER_ADDR_PAGE(4), _MAPPER_ADDR_PAGE(5),
    _MAPPER_ADDR_PAGE(6), _MAPPER_ADDR_PAGE(7), _MAPPER_ADDR_PAGE(8),
    _MAPPER_ADDR_PAGE(9), _MAPPER_ADDR_PAGE(10), _MAPPER_ADDR_PAGE(11),
    _MAPPER_ADDR_PAGE(12), _MAPPER_ADDR_PAGE(13), _MAPPER_ADDR_PAGE(14),
    _MAPPER_ADDR_PAGE(15)};
_MAPPER_MARKER(__bank_window_marker, ".mapper_window", MAPPER_WINDOW_KB)
_MAPPER_MARKER(__bank_1_marker, ".mapper_bank_1", _MAPPER_KB_1)
_MAPPER_MARKER(__bank_2_marker, ".mapper_bank_2", _MAPPER_KB_2)
_MAPPER_MARKER(__bank_3_marker, ".mapper_bank_3", _MAPPER_KB_3)
_MAPPER_MARKER(__bank_4_marker, ".mapper_bank_4", _MAPPER_KB_4)
_MAPPER_MARKER(__bank_5_marker, ".mapper_bank_5", _MAPPER_KB_5)
_MAPPER_MARKER(__bank_6_marker, ".mapper_bank_6", _MAPPER_KB_6)
_MAPPER_MARKER(__bank_7_marker, ".mapper_bank_7", _MAPPER_KB_7)
_MAPPER_MARKER(__bank_8_marker, ".mapper_bank_8", _MAPPER_KB_8)
_MAPPER_MARKER(__bank_9_marker, ".mapper_bank_9", _MAPPER_KB_9)
_MAPPER_MARKER(__bank_10_marker, ".mapper_bank_10", _MAPPER_KB_10)
_MAPPER_MARKER(__bank_11_marker, ".mapper_bank_11", _MAPPER_KB_11)
_MAPPER_MARKER(__bank_12_marker, ".mapper_bank_12", _MAPPER_KB_12)
_MAPPER_MARKER(__bank_13_marker, ".mapper_bank_13", _MAPPER_KB_13)
_MAPPER_MARKER(__bank_14_marker, ".mapper_bank_14", _MAPPER_KB_14)
_MAPPER_MARKER(__bank_15_marker, ".mapper_bank_15", _MAPPER_KB_15)
#ifdef __cplusplus
}
#endif

/* KERNAL LOAD, loader 2, is a library member of its own. */
#if _MAPPER_LOADER_ID != 2
#if _MAPPER_LOADER_ID == 0
#define _MAPPER_LOADER_FN __load_banks_hyppo
#else
#define _MAPPER_LOADER_FN __load_banks_floppy
#endif
#ifdef __cplusplus
extern "C" {
#endif
void _MAPPER_LOADER_FN(void);
__attribute__((weak, section(".bank_0")))
void __load_banks(void) { _MAPPER_LOADER_FN(); }
#ifdef __cplusplus
}
#endif
#endif

__attribute__((used, section(".mapper_layout")))
static const unsigned long __mapper_layout[33] = {
    BANK_PHYS_BASE_0, BANK_PHYS_BASE_1, BANK_PHYS_BASE_2, BANK_PHYS_BASE_3,
    BANK_PHYS_BASE_4, BANK_PHYS_BASE_5, BANK_PHYS_BASE_6, BANK_PHYS_BASE_7,
    BANK_PHYS_BASE_8, BANK_PHYS_BASE_9, BANK_PHYS_BASE_10, BANK_PHYS_BASE_11,
    BANK_PHYS_BASE_12, BANK_PHYS_BASE_13, BANK_PHYS_BASE_14, BANK_PHYS_BASE_15,
    MAPPER_WINDOW_KB, _MAPPER_KB_1, _MAPPER_KB_2, _MAPPER_KB_3,
    _MAPPER_KB_4, _MAPPER_KB_5, _MAPPER_KB_6, _MAPPER_KB_7,
    _MAPPER_KB_8, _MAPPER_KB_9, _MAPPER_KB_10, _MAPPER_KB_11,
    _MAPPER_KB_12, _MAPPER_KB_13, _MAPPER_KB_14, _MAPPER_KB_15,
    _MAPPER_LOADER_ID};

#endif /* __ASSEMBLER__ */
#endif /* _MAPPER_DEFAULT_BANK_1 */

#endif // _MEGA65_MAPPER_COMMON_H_
