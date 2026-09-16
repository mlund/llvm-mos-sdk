// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// The half of <mapper.h> that does not depend on where the banks are. Each
// banked platform's own mapper.h supplies the addresses and includes this.

#ifndef _MEGA65_MAPPER_COMMON_H_
#define _MEGA65_MAPPER_COMMON_H_

/**
 * @brief How many banks this program uses: MAPPER_BANK_COUNT, 0-31.
 *
 * A plain number, defined before <mapper.h> and the same in every file, like
 * the layout macros below:
 *
 *     #define MAPPER_BANK_COUNT 3
 *
 * Banks above it get no slot, no file and no table entries, and set_bank()
 * maps bank 0 for them. Without it every program reserves all 31, nearly all
 * of it zeroes that still have to be read back at boot.
 */

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

#include <stdint.h>

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
 * @param bank_id Bank number, 0 to MAPPER_BANK_COUNT.
 * @param method  Function pointer (address within the window).
 */
/* "leaf" would normally be a lie here -- this re-enters C through
 * __call_indir. It is sound only because callback(2) restores the call edge
 * that leaf severs, so LLVM still sees the indirect target when it lays out
 * static stack frames. Do not drop either attribute. */
__attribute__((leaf, callback(2))) void banked_call(uint8_t bank_id,
                                                    void (*method)(void));

/**
 * @brief Get the currently mapped bank number.
 */
__attribute__((leaf)) uint8_t get_bank(void);

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
 * @param bank_id Bank number. One above MAPPER_BANK_COUNT maps bank 0: it
 *                would index past the bank tables and hand junk to MAP,
 *                which covers $0000-$7FFF and so could move zero page out
 *                from under the compiler.
 */
__attribute__((leaf)) void set_bank(uint8_t bank_id);

/* The switch banked_call_r and banked_call_v use; see there. */
__attribute__((leaf)) uint8_t __banked_call_enter(uint8_t bank_id);

/**
 * @brief Called when startup cannot load a bank the program declared.
 *
 * Weak: define it to handle the failure. The default halts with a red border,
 * since the bank would read back as zeroes. Returning goes on to the next bank.
 */
void __bank_load_failed(uint8_t bank);

/* What the loaders read: the tables below, emitted by the program's own files
 * so a layout it overrides is the one loaded, and bank-sizes.s with bank n at
 * bit n % 8 of byte n / 8. */
extern const uint8_t __bank_used[4];
extern const uint8_t __bank_megabyte[];
extern const uint8_t __bank_addr_mid[];
extern const uint8_t __bank_addr_page[];

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
 * at the call. Each argument binds as the callee's own parameter type, so a
 * reference parameter reaches the caller's object rather than a temporary; in
 * C++ that means the callee must name one function, not an overload set.
 */
#define _BANKED_NARGS(...)                                                     \
  _BANKED_NARGS_(__VA_OPT__(, ) __VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define _BANKED_NARGS_(z, a, b, c, d, e, f, g, h, n, ...) n
#define _BANKED_CAT(a, b) _BANKED_CAT_(a, b)
#define _BANKED_CAT_(a, b) a##b
#ifdef __cplusplus
extern "C++" {
/* The callee's own parameter type, so each argument binds the way the callee
 * takes it: a value is copied before the switch, a reference binds and the
 * callee reaches the caller's object rather than a temporary. */
template <unsigned _I, class _F> struct __banked_param;
template <unsigned _I, class _R, class... _A>
struct __banked_param<_I, _R(_A...)> {
  using type = __type_pack_element<_I, _A...>;
};
template <unsigned _I, class _R, class... _A>
struct __banked_param<_I, _R (*)(_A...)> : __banked_param<_I, _R(_A...)> {};
#if __cpp_noexcept_function_type
template <unsigned _I, class _R, class... _A>
struct __banked_param<_I, _R(_A...) noexcept> : __banked_param<_I, _R(_A...)> {
};
template <unsigned _I, class _R, class... _A>
struct __banked_param<_I, _R (*)(_A...) noexcept>
    : __banked_param<_I, _R(_A...)> {};
#endif
}
#define _BANKED_BIND(fn, i) typename __banked_param<i, decltype(fn)>::type
#else
/* C has no reference parameters, so a copy is always what the callee takes. */
#define _BANKED_BIND(fn, i) __auto_type
#endif
#define _BANKED_T0(fn)
#define _BANKED_T1(fn, a) _BANKED_T0(fn) _BANKED_BIND(fn, 0) __banked_a1 = (a);
#define _BANKED_T2(fn, a, b)                                                   \
  _BANKED_T1(fn, a) _BANKED_BIND(fn, 1) __banked_a2 = (b);
#define _BANKED_T3(fn, a, b, c)                                                \
  _BANKED_T2(fn, a, b) _BANKED_BIND(fn, 2) __banked_a3 = (c);
#define _BANKED_T4(fn, a, b, c, d)                                             \
  _BANKED_T3(fn, a, b, c) _BANKED_BIND(fn, 3) __banked_a4 = (d);
#define _BANKED_T5(fn, a, b, c, d, e)                                          \
  _BANKED_T4(fn, a, b, c, d) _BANKED_BIND(fn, 4) __banked_a5 = (e);
#define _BANKED_T6(fn, a, b, c, d, e, f)                                       \
  _BANKED_T5(fn, a, b, c, d, e) _BANKED_BIND(fn, 5) __banked_a6 = (f);
#define _BANKED_T7(fn, a, b, c, d, e, f, g)                                    \
  _BANKED_T6(fn, a, b, c, d, e, f) _BANKED_BIND(fn, 6) __banked_a7 = (g);
#define _BANKED_T8(fn, a, b, c, d, e, f, g, h)                                 \
  _BANKED_T7(fn, a, b, c, d, e, f, g) _BANKED_BIND(fn, 7) __banked_a8 = (h);
#define _BANKED_A0()
#define _BANKED_A1(a) __banked_a1
#define _BANKED_A2(a, b) _BANKED_A1(a), __banked_a2
#define _BANKED_A3(a, b, c) _BANKED_A2(a, b), __banked_a3
#define _BANKED_A4(a, b, c, d) _BANKED_A3(a, b, c), __banked_a4
#define _BANKED_A5(a, b, c, d, e) _BANKED_A4(a, b, c, d), __banked_a5
#define _BANKED_A6(a, b, c, d, e, f) _BANKED_A5(a, b, c, d, e), __banked_a6
#define _BANKED_A7(a, b, c, d, e, f, g)                                        \
  _BANKED_A6(a, b, c, d, e, f), __banked_a7
#define _BANKED_A8(a, b, c, d, e, f, g, h)                                     \
  _BANKED_A7(a, b, c, d, e, f, g), __banked_a8
#define _BANKED_TEMPS(fn, ...)                                                 \
  _BANKED_CAT(_BANKED_T, _BANKED_NARGS(__VA_ARGS__))                           \
  (fn __VA_OPT__(, ) __VA_ARGS__)
#define _BANKED_ARGS(...)                                                      \
  _BANKED_CAT(_BANKED_A, _BANKED_NARGS(__VA_ARGS__))(__VA_ARGS__)

#define banked_call_r(bank, fn, ...)                                           \
  ({                                                                           \
    _BANKED_TEMPS(fn __VA_OPT__(, ) __VA_ARGS__)                               \
    uint8_t __banked_prev = __banked_call_enter(bank);                         \
    __auto_type __banked_result = (fn)(_BANKED_ARGS(__VA_ARGS__));             \
    set_bank(__banked_prev);                                                   \
    __banked_result;                                                           \
  })

#define banked_call_v(bank, fn, ...)                                           \
  ({                                                                           \
    _BANKED_TEMPS(fn __VA_OPT__(, ) __VA_ARGS__)                               \
    uint8_t __banked_prev = __banked_call_enter(bank);                         \
    (fn)(_BANKED_ARGS(__VA_ARGS__));                                           \
    set_bank(__banked_prev);                                                   \
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

#ifdef MAPPER_BANK_COUNT
#define _MAPPER_COUNT MAPPER_BANK_COUNT
#else
#define _MAPPER_COUNT 31
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
#ifdef MAPPER_BANK_16
#define BANK_PHYS_BASE_16 (MAPPER_BANK_16)
#else
#define BANK_PHYS_BASE_16 _MAPPER_DEFAULT_BANK_16
#endif
#ifdef MAPPER_BANK_16_KB
#define _MAPPER_KB_16 (MAPPER_BANK_16_KB)
#else
#define _MAPPER_KB_16 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_17
#define BANK_PHYS_BASE_17 (MAPPER_BANK_17)
#else
#define BANK_PHYS_BASE_17 _MAPPER_DEFAULT_BANK_17
#endif
#ifdef MAPPER_BANK_17_KB
#define _MAPPER_KB_17 (MAPPER_BANK_17_KB)
#else
#define _MAPPER_KB_17 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_18
#define BANK_PHYS_BASE_18 (MAPPER_BANK_18)
#else
#define BANK_PHYS_BASE_18 _MAPPER_DEFAULT_BANK_18
#endif
#ifdef MAPPER_BANK_18_KB
#define _MAPPER_KB_18 (MAPPER_BANK_18_KB)
#else
#define _MAPPER_KB_18 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_19
#define BANK_PHYS_BASE_19 (MAPPER_BANK_19)
#else
#define BANK_PHYS_BASE_19 _MAPPER_DEFAULT_BANK_19
#endif
#ifdef MAPPER_BANK_19_KB
#define _MAPPER_KB_19 (MAPPER_BANK_19_KB)
#else
#define _MAPPER_KB_19 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_20
#define BANK_PHYS_BASE_20 (MAPPER_BANK_20)
#else
#define BANK_PHYS_BASE_20 _MAPPER_DEFAULT_BANK_20
#endif
#ifdef MAPPER_BANK_20_KB
#define _MAPPER_KB_20 (MAPPER_BANK_20_KB)
#else
#define _MAPPER_KB_20 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_21
#define BANK_PHYS_BASE_21 (MAPPER_BANK_21)
#else
#define BANK_PHYS_BASE_21 _MAPPER_DEFAULT_BANK_21
#endif
#ifdef MAPPER_BANK_21_KB
#define _MAPPER_KB_21 (MAPPER_BANK_21_KB)
#else
#define _MAPPER_KB_21 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_22
#define BANK_PHYS_BASE_22 (MAPPER_BANK_22)
#else
#define BANK_PHYS_BASE_22 _MAPPER_DEFAULT_BANK_22
#endif
#ifdef MAPPER_BANK_22_KB
#define _MAPPER_KB_22 (MAPPER_BANK_22_KB)
#else
#define _MAPPER_KB_22 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_23
#define BANK_PHYS_BASE_23 (MAPPER_BANK_23)
#else
#define BANK_PHYS_BASE_23 _MAPPER_DEFAULT_BANK_23
#endif
#ifdef MAPPER_BANK_23_KB
#define _MAPPER_KB_23 (MAPPER_BANK_23_KB)
#else
#define _MAPPER_KB_23 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_24
#define BANK_PHYS_BASE_24 (MAPPER_BANK_24)
#else
#define BANK_PHYS_BASE_24 _MAPPER_DEFAULT_BANK_24
#endif
#ifdef MAPPER_BANK_24_KB
#define _MAPPER_KB_24 (MAPPER_BANK_24_KB)
#else
#define _MAPPER_KB_24 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_25
#define BANK_PHYS_BASE_25 (MAPPER_BANK_25)
#else
#define BANK_PHYS_BASE_25 _MAPPER_DEFAULT_BANK_25
#endif
#ifdef MAPPER_BANK_25_KB
#define _MAPPER_KB_25 (MAPPER_BANK_25_KB)
#else
#define _MAPPER_KB_25 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_26
#define BANK_PHYS_BASE_26 (MAPPER_BANK_26)
#else
#define BANK_PHYS_BASE_26 _MAPPER_DEFAULT_BANK_26
#endif
#ifdef MAPPER_BANK_26_KB
#define _MAPPER_KB_26 (MAPPER_BANK_26_KB)
#else
#define _MAPPER_KB_26 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_27
#define BANK_PHYS_BASE_27 (MAPPER_BANK_27)
#else
#define BANK_PHYS_BASE_27 _MAPPER_DEFAULT_BANK_27
#endif
#ifdef MAPPER_BANK_27_KB
#define _MAPPER_KB_27 (MAPPER_BANK_27_KB)
#else
#define _MAPPER_KB_27 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_28
#define BANK_PHYS_BASE_28 (MAPPER_BANK_28)
#else
#define BANK_PHYS_BASE_28 _MAPPER_DEFAULT_BANK_28
#endif
#ifdef MAPPER_BANK_28_KB
#define _MAPPER_KB_28 (MAPPER_BANK_28_KB)
#else
#define _MAPPER_KB_28 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_29
#define BANK_PHYS_BASE_29 (MAPPER_BANK_29)
#else
#define BANK_PHYS_BASE_29 _MAPPER_DEFAULT_BANK_29
#endif
#ifdef MAPPER_BANK_29_KB
#define _MAPPER_KB_29 (MAPPER_BANK_29_KB)
#else
#define _MAPPER_KB_29 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_30
#define BANK_PHYS_BASE_30 (MAPPER_BANK_30)
#else
#define BANK_PHYS_BASE_30 _MAPPER_DEFAULT_BANK_30
#endif
#ifdef MAPPER_BANK_30_KB
#define _MAPPER_KB_30 (MAPPER_BANK_30_KB)
#else
#define _MAPPER_KB_30 MAPPER_WINDOW_KB
#endif
#ifdef MAPPER_BANK_31
#define BANK_PHYS_BASE_31 (MAPPER_BANK_31)
#else
#define BANK_PHYS_BASE_31 _MAPPER_DEFAULT_BANK_31
#endif
#ifdef MAPPER_BANK_31_KB
#define _MAPPER_KB_31 (MAPPER_BANK_31_KB)
#else
#define _MAPPER_KB_31 MAPPER_WINDOW_KB
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
#define BANK_SIZE_16 (_MAPPER_KB_16 * _MAPPER_UL(1024))
#define BANK_SIZE_17 (_MAPPER_KB_17 * _MAPPER_UL(1024))
#define BANK_SIZE_18 (_MAPPER_KB_18 * _MAPPER_UL(1024))
#define BANK_SIZE_19 (_MAPPER_KB_19 * _MAPPER_UL(1024))
#define BANK_SIZE_20 (_MAPPER_KB_20 * _MAPPER_UL(1024))
#define BANK_SIZE_21 (_MAPPER_KB_21 * _MAPPER_UL(1024))
#define BANK_SIZE_22 (_MAPPER_KB_22 * _MAPPER_UL(1024))
#define BANK_SIZE_23 (_MAPPER_KB_23 * _MAPPER_UL(1024))
#define BANK_SIZE_24 (_MAPPER_KB_24 * _MAPPER_UL(1024))
#define BANK_SIZE_25 (_MAPPER_KB_25 * _MAPPER_UL(1024))
#define BANK_SIZE_26 (_MAPPER_KB_26 * _MAPPER_UL(1024))
#define BANK_SIZE_27 (_MAPPER_KB_27 * _MAPPER_UL(1024))
#define BANK_SIZE_28 (_MAPPER_KB_28 * _MAPPER_UL(1024))
#define BANK_SIZE_29 (_MAPPER_KB_29 * _MAPPER_UL(1024))
#define BANK_SIZE_30 (_MAPPER_KB_30 * _MAPPER_UL(1024))
#define BANK_SIZE_31 (_MAPPER_KB_31 * _MAPPER_UL(1024))

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
_MAPPER_ASSERT(_MAPPER_COUNT >= 0 && _MAPPER_COUNT <= 31,
               "MAPPER_BANK_COUNT must be 0-31");
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
_MAPPER_CHECK(16)
_MAPPER_CHECK(17)
_MAPPER_CHECK(18)
_MAPPER_CHECK(19)
_MAPPER_CHECK(20)
_MAPPER_CHECK(21)
_MAPPER_CHECK(22)
_MAPPER_CHECK(23)
_MAPPER_CHECK(24)
_MAPPER_CHECK(25)
_MAPPER_CHECK(26)
_MAPPER_CHECK(27)
_MAPPER_CHECK(28)
_MAPPER_CHECK(29)
_MAPPER_CHECK(30)
_MAPPER_CHECK(31)

#define _MAPPER_OFF(n)                                                         \
  (((BANK_PHYS_BASE_##n & 0xFFFFFul) - 0x2000ul) & 0xFFFFFul)
#define _MAPPER_OFFSET_LO(n) (uint8_t)(_MAPPER_OFF(n) >> 8)
/* MAPLO selects 8 KB blocks from $2000: 1-3, 1-2 or just 1. */
#define _MAPPER_SELECT(kb) ((kb) == 24 ? 0xE0 : (kb) == 16 ? 0x60 : 0x20)
#define _MAPPER_MAPLO_SEL(n)                                                   \
  (uint8_t)(n ? _MAPPER_SELECT(_MAPPER_KB_##n) | _MAPPER_OFF(n) >> 16 : 0)
#define _MAPPER_MEGABYTE(n) (uint8_t)(BANK_PHYS_BASE_##n >> 20)
#define _MAPPER_ADDR_MID(n) (uint8_t)(BANK_PHYS_BASE_##n >> 16)
#define _MAPPER_ADDR_PAGE(n) (uint8_t)(BANK_PHYS_BASE_##n >> 8)
#define _MAPPER_TABLE                                                          \
  __attribute__((weak, section(".rodata.bank_tables")))

/* F(0) to F(n): one table entry per bank in use. n must be a plain number. */
#define _MAPPER_ROWS_0(F) F(0)
#define _MAPPER_ROWS_1(F) _MAPPER_ROWS_0(F), F(1)
#define _MAPPER_ROWS_2(F) _MAPPER_ROWS_1(F), F(2)
#define _MAPPER_ROWS_3(F) _MAPPER_ROWS_2(F), F(3)
#define _MAPPER_ROWS_4(F) _MAPPER_ROWS_3(F), F(4)
#define _MAPPER_ROWS_5(F) _MAPPER_ROWS_4(F), F(5)
#define _MAPPER_ROWS_6(F) _MAPPER_ROWS_5(F), F(6)
#define _MAPPER_ROWS_7(F) _MAPPER_ROWS_6(F), F(7)
#define _MAPPER_ROWS_8(F) _MAPPER_ROWS_7(F), F(8)
#define _MAPPER_ROWS_9(F) _MAPPER_ROWS_8(F), F(9)
#define _MAPPER_ROWS_10(F) _MAPPER_ROWS_9(F), F(10)
#define _MAPPER_ROWS_11(F) _MAPPER_ROWS_10(F), F(11)
#define _MAPPER_ROWS_12(F) _MAPPER_ROWS_11(F), F(12)
#define _MAPPER_ROWS_13(F) _MAPPER_ROWS_12(F), F(13)
#define _MAPPER_ROWS_14(F) _MAPPER_ROWS_13(F), F(14)
#define _MAPPER_ROWS_15(F) _MAPPER_ROWS_14(F), F(15)
#define _MAPPER_ROWS_16(F) _MAPPER_ROWS_15(F), F(16)
#define _MAPPER_ROWS_17(F) _MAPPER_ROWS_16(F), F(17)
#define _MAPPER_ROWS_18(F) _MAPPER_ROWS_17(F), F(18)
#define _MAPPER_ROWS_19(F) _MAPPER_ROWS_18(F), F(19)
#define _MAPPER_ROWS_20(F) _MAPPER_ROWS_19(F), F(20)
#define _MAPPER_ROWS_21(F) _MAPPER_ROWS_20(F), F(21)
#define _MAPPER_ROWS_22(F) _MAPPER_ROWS_21(F), F(22)
#define _MAPPER_ROWS_23(F) _MAPPER_ROWS_22(F), F(23)
#define _MAPPER_ROWS_24(F) _MAPPER_ROWS_23(F), F(24)
#define _MAPPER_ROWS_25(F) _MAPPER_ROWS_24(F), F(25)
#define _MAPPER_ROWS_26(F) _MAPPER_ROWS_25(F), F(26)
#define _MAPPER_ROWS_27(F) _MAPPER_ROWS_26(F), F(27)
#define _MAPPER_ROWS_28(F) _MAPPER_ROWS_27(F), F(28)
#define _MAPPER_ROWS_29(F) _MAPPER_ROWS_28(F), F(29)
#define _MAPPER_ROWS_30(F) _MAPPER_ROWS_29(F), F(30)
#define _MAPPER_ROWS_31(F) _MAPPER_ROWS_30(F), F(31)
#define _MAPPER_ROWS_(n, F) _MAPPER_ROWS_##n(F)
#define _MAPPER_ROWS(n, F) _MAPPER_ROWS_(n, F)

/* The linker reads each size off a marker's alignment (8 KB as 2, 16 KB as 4,
 * 24 KB as 8), the one section property that combines alike with LTO and
 * without. */
#define _MAPPER_ALIGN(kb) ((kb) == 24 ? 8 : (kb) / 4)
#define _MAPPER_MARKER(name, sect, kb)                                         \
  __attribute__((weak, used, section(sect), aligned(_MAPPER_ALIGN(kb))))       \
  _MAPPER_EXTERN const uint8_t name = 0;

#ifdef __cplusplus
extern "C" {
#endif
_MAPPER_TABLE _MAPPER_EXTERN const uint8_t __bank_offset_lo[] = {
    _MAPPER_ROWS(_MAPPER_COUNT, _MAPPER_OFFSET_LO)};
_MAPPER_TABLE _MAPPER_EXTERN const uint8_t __bank_maplo_sel[] = {
    _MAPPER_ROWS(_MAPPER_COUNT, _MAPPER_MAPLO_SEL)};
_MAPPER_TABLE _MAPPER_EXTERN const uint8_t __bank_megabyte[] = {
    _MAPPER_ROWS(_MAPPER_COUNT, _MAPPER_MEGABYTE)};
_MAPPER_TABLE _MAPPER_EXTERN const uint8_t __bank_addr_mid[] = {
    _MAPPER_ROWS(_MAPPER_COUNT, _MAPPER_ADDR_MID)};
_MAPPER_TABLE _MAPPER_EXTERN const uint8_t __bank_addr_page[] = {
    _MAPPER_ROWS(_MAPPER_COUNT, _MAPPER_ADDR_PAGE)};
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
_MAPPER_MARKER(__bank_16_marker, ".mapper_bank_16", _MAPPER_KB_16)
_MAPPER_MARKER(__bank_17_marker, ".mapper_bank_17", _MAPPER_KB_17)
_MAPPER_MARKER(__bank_18_marker, ".mapper_bank_18", _MAPPER_KB_18)
_MAPPER_MARKER(__bank_19_marker, ".mapper_bank_19", _MAPPER_KB_19)
_MAPPER_MARKER(__bank_20_marker, ".mapper_bank_20", _MAPPER_KB_20)
_MAPPER_MARKER(__bank_21_marker, ".mapper_bank_21", _MAPPER_KB_21)
_MAPPER_MARKER(__bank_22_marker, ".mapper_bank_22", _MAPPER_KB_22)
_MAPPER_MARKER(__bank_23_marker, ".mapper_bank_23", _MAPPER_KB_23)
_MAPPER_MARKER(__bank_24_marker, ".mapper_bank_24", _MAPPER_KB_24)
_MAPPER_MARKER(__bank_25_marker, ".mapper_bank_25", _MAPPER_KB_25)
_MAPPER_MARKER(__bank_26_marker, ".mapper_bank_26", _MAPPER_KB_26)
_MAPPER_MARKER(__bank_27_marker, ".mapper_bank_27", _MAPPER_KB_27)
_MAPPER_MARKER(__bank_28_marker, ".mapper_bank_28", _MAPPER_KB_28)
_MAPPER_MARKER(__bank_29_marker, ".mapper_bank_29", _MAPPER_KB_29)
_MAPPER_MARKER(__bank_30_marker, ".mapper_bank_30", _MAPPER_KB_30)
_MAPPER_MARKER(__bank_31_marker, ".mapper_bank_31", _MAPPER_KB_31)
/* The count, one marker per set bit. */
#if _MAPPER_COUNT & 1
_MAPPER_MARKER(__bank_count_1_marker, ".mapper_count_1", 8)
#endif
#if _MAPPER_COUNT & 2
_MAPPER_MARKER(__bank_count_2_marker, ".mapper_count_2", 8)
#endif
#if _MAPPER_COUNT & 4
_MAPPER_MARKER(__bank_count_4_marker, ".mapper_count_4", 8)
#endif
#if _MAPPER_COUNT & 8
_MAPPER_MARKER(__bank_count_8_marker, ".mapper_count_8", 8)
#endif
#if _MAPPER_COUNT & 16
_MAPPER_MARKER(__bank_count_16_marker, ".mapper_count_16", 8)
#endif
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

#define _MAPPER_BASE(n) BANK_PHYS_BASE_##n
#define _MAPPER_KB(n) _MAPPER_KB_##n
__attribute__((used, section(".mapper_layout")))
static const uint32_t __mapper_layout[66] = {
    _MAPPER_ROWS(31, _MAPPER_BASE), _MAPPER_ROWS(31, _MAPPER_KB),
    _MAPPER_LOADER_ID, _MAPPER_COUNT};

#endif /* __ASSEMBLER__ */
#endif /* _MAPPER_DEFAULT_BANK_1 */

#endif // _MEGA65_MAPPER_COMMON_H_
