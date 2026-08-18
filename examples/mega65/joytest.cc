// Copyright 2025 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

//
// Joystick and paddle tester for MEGA65
//
// C++ port of joytest65 (https://github.com/dansanderson/joytest65)
// by Dan Sanderson. 80-column mode layout. Reads and displays:
//   - Two joystick ports: 8-way d-pad with directional indicators
//   - Five-button protocol: fire + paddle buttons + up+down / left+right
//   - Four paddle analog values with visual bar graphs
//   - Raw CIA register values in binary and hex
//

#include <cstdint>
#include <mega65.h>

// --- Screen access ---

static constexpr uint8_t COLS = 80;
static volatile uint8_t *const SCREEN = &DEFAULT_SCREEN;
static volatile uint8_t *const COLRAM =
    reinterpret_cast<volatile uint8_t *>(0xD800);

// --- CRAM2K helpers ---
// In 80-column mode, colour RAM offsets >= 1024 require the VIC-III CRAM2K
// bit ($D030 bit 0) to extend the colour RAM window from $D800-$DBFF to
// $D800-$DFFF. While enabled, CIA and other I/O at $DC00-$DFFF is
// inaccessible, so interrupts must be disabled.

static void cram2k_on() {
  asm volatile("sei");
  VICIV.ctrla |= 0x01;
}

static void cram2k_off() {
  VICIV.ctrla &= ~0x01;
  asm volatile("cli");
}

// --- Layout (80x25, matching joytest65 column positions) ---

static constexpr uint8_t DPAD_ROW = 4;
static constexpr uint8_t BTN_ROW = 11;
static constexpr uint8_t CIA_ROW = 14;
static constexpr uint8_t PADDLE_ROW = 17;

// Per-port columns. Joy 2 is offset +23 from joy 1 (same as joytest65).
static constexpr uint8_t JOY1_DPAD_COL = 25, JOY2_DPAD_COL = 48;
static constexpr uint8_t JOY1_BTN_COL = 23, JOY2_BTN_COL = 46;

// --- Helpers ---

static constexpr uint8_t ascii_to_screencode(char c) {
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 1;
  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 1;
  if (c == '[')
    return 0x1B;
  if (c == ']')
    return 0x1D;
  return c;
}

// Caller must enable CRAM2K for rows >= 13 (offset 1040+ exceeds the
// 1024-byte default colour RAM window).
static void plot(uint8_t col, uint8_t row, uint8_t screencode, uint8_t color) {
  uint16_t off = static_cast<uint16_t>(row) * COLS + col;
  SCREEN[off] = screencode;
  COLRAM[off] = color;
}

static void text(uint8_t col, uint8_t row, const char *s, uint8_t color) {
  uint16_t off = static_cast<uint16_t>(row) * COLS + col;
  while (*s) {
    SCREEN[off] = ascii_to_screencode(*s++);
    COLRAM[off] = color;
    ++off;
  }
}

static void plot_hex(uint8_t col, uint8_t row, uint8_t val, uint8_t color) {
  // Screen codes: 0x30='0', 1='A' (so 'A'-'F' = 1..6)
  auto hex_digit = [](uint8_t v) -> uint8_t {
    return (v < 10) ? (0x30 + v) : (v - 10 + 1);
  };
  plot(col, row, hex_digit(val >> 4), color);
  plot(col + 1, row, hex_digit(val & 0x0F), color);
}

static void plot_binary(uint8_t col, uint8_t row, uint8_t val, uint8_t color) {
  for (uint8_t i = 0; i < 5; ++i)
    plot(col + i, row, (val & (0x10 >> i)) ? 0x31 : 0x30, color);
}

// --- D-pad indicator data ---
// Screen code + colour for ON/OFF states, reusing the exact screen codes
// and VIC-III extended attributes (bit 5 = reverse) from joytest65.

// VIC-III extended attribute bit (OR with COLOR_* values).
static constexpr uint8_t ATTR_REVERSE = 0x20;

struct Indicator {
  uint8_t screencode, color;
};

static void plot_indicator(uint8_t col, uint8_t row, bool on,
                           Indicator on_ind, Indicator off_ind) {
  const Indicator &ind = on ? on_ind : off_ind;
  plot(col, row, ind.screencode, ind.color);
}

// clang-format off
static constexpr uint8_t ACTIVE     = COLOR_YELLOW;
static constexpr uint8_t ACTIVE_REV = ATTR_REVERSE | COLOR_YELLOW;
static constexpr uint8_t INACTIVE   = COLOR_LIGHTBLUE;

static constexpr Indicator UL_ON = {0x69, ACTIVE},     UL_OFF = {0x4F, INACTIVE};
static constexpr Indicator U1_ON = {0x69, ACTIVE_REV}, U1_OFF = {0x4E, INACTIVE};
static constexpr Indicator U2_ON = {0x5F, ACTIVE_REV}, U2_OFF = {0x4D, INACTIVE};
static constexpr Indicator UR_ON = {0x5F, ACTIVE},     UR_OFF = {0x50, INACTIVE};
static constexpr Indicator L1_ON = {0x69, ACTIVE_REV}, L1_OFF = {0x4E, INACTIVE};
static constexpr Indicator L2_ON = {0x5F, ACTIVE},     L2_OFF = {0x4D, INACTIVE};
static constexpr Indicator R1_ON = {0x5F, ACTIVE_REV}, R1_OFF = {0x4D, INACTIVE};
static constexpr Indicator R2_ON = {0x69, ACTIVE},     R2_OFF = {0x4E, INACTIVE};
static constexpr Indicator DL_ON = {0x5F, ACTIVE_REV}, DL_OFF = {0x4C, INACTIVE};
static constexpr Indicator D1_ON = {0x5F, ACTIVE},     D1_OFF = {0x4D, INACTIVE};
static constexpr Indicator D2_ON = {0x69, ACTIVE},     D2_OFF = {0x4E, INACTIVE};
static constexpr Indicator DR_ON = {0x69, ACTIVE_REV}, DR_OFF = {0x7A, INACTIVE};
static constexpr Indicator BT_ON = {0x51, ACTIVE},     BT_OFF = {0x57, INACTIVE};
// clang-format on

// --- D-pad drawing ---
// Each direction indicator only lights for its specific direction:
// e.g. UL = up && left && !right, U = up && !left && !right

static void draw_dpad(uint8_t col, uint8_t row,
                      const mega65_joy_state_t &joy) {
  bool u = joy.up();
  bool d = joy.down();
  bool l = joy.left();
  bool ri = joy.right();

  plot_indicator(col, row, u && l && !ri, UL_ON, UL_OFF);
  plot_indicator(col + 2, row, u && !l && !ri, U1_ON, U1_OFF);
  plot_indicator(col + 3, row, u && !l && !ri, U2_ON, U2_OFF);
  plot_indicator(col + 5, row, u && ri && !l, UR_ON, UR_OFF);

  plot_indicator(col, row + 2, l && !u && !d, L1_ON, L1_OFF);
  plot_indicator(col, row + 3, l && !u && !d, L2_ON, L2_OFF);
  plot_indicator(col + 5, row + 2, ri && !u && !d, R1_ON, R1_OFF);
  plot_indicator(col + 5, row + 3, ri && !u && !d, R2_ON, R2_OFF);

  plot_indicator(col, row + 5, d && l && !ri, DL_ON, DL_OFF);
  plot_indicator(col + 2, row + 5, d && !l && !ri, D1_ON, D1_OFF);
  plot_indicator(col + 3, row + 5, d && !l && !ri, D2_ON, D2_OFF);
  plot_indicator(col + 5, row + 5, d && ri && !l, DR_ON, DR_OFF);
}

// --- Button drawing ---
// B1=fire, B2/B3=paddle buttons (<$10), B4=up+down, B5=left+right
// Spacing matches joytest65 80-column layout (6-col between B1/B2/B3).

static void draw_buttons(uint8_t col, uint8_t row,
                         const mega65_joy_state_t &joy) {
  plot_indicator(col, row, joy.button1(), BT_ON, BT_OFF);
  plot_indicator(col + 6, row, joy.button2(), BT_ON, BT_OFF);
  plot_indicator(col + 12, row, joy.button3(), BT_ON, BT_OFF);
  plot_indicator(col + 3, row + 1, joy.button4(), BT_ON, BT_OFF);
  plot_indicator(col + 9, row + 1, joy.button5(), BT_ON, BT_OFF);
}

// --- Paddle bar ---

static constexpr uint8_t BAR_COL = 6;
static constexpr uint8_t BAR_W = 50;

static constexpr uint8_t BAR_HEX_COL = BAR_COL + BAR_W + 3; // after "] $"

static void draw_paddle_bar(uint8_t row, uint8_t val) {
  const uint16_t base = static_cast<uint16_t>(row) * COLS;
  uint8_t fill = static_cast<uint8_t>(static_cast<uint16_t>(val) * BAR_W >> 8);
  for (uint8_t i = 0; i < BAR_W; ++i) {
    uint16_t off = base + BAR_COL + i;
    SCREEN[off] = 0x20;
    COLRAM[off] = (i < fill) ? (ATTR_REVERSE | COLOR_LIGHTGREEN) : COLOR_BLACK;
  }
  plot_hex(BAR_HEX_COL, row, val, COLOR_LIGHTGREY);
}

// --- Setup ---

static void setup() {
  // Unlock VIC-IV, enable VIC-III fast mode + extended attributes + 80 columns
  VICIV.key = 0x47;
  VICIV.key = 0x53;
  VICIV.ctrlb = (VICIV.ctrlb | VIC3_FAST_MASK | VIC3_ATTR_MASK |
                 VIC3_H640_MASK) &
                ~VIC3_V400_MASK;

  // Uppercase/graphics charset — d-pad screen codes require this set
  *(volatile uint8_t *)0xD018 &= ~0x02;

  VICIV.bordercol = COLOR_BLUE;

  // Clear screen and colour RAM (2000 chars for 80x25, CRAM2K already enabled)
  cram2k_on();

  for (uint16_t i = 0; i < 2000; ++i) {
    SCREEN[i] = 0x20;
    COLRAM[i] = 0x00;
  }

  // Title
  text(28, 0, "JOYSTICK & PADDLE TESTER", COLOR_YELLOW);

  // Joystick labels (centered above d-pads)
  text(JOY1_DPAD_COL, 2, "JOY 1", COLOR_WHITE);
  text(JOY2_DPAD_COL, 2, "JOY 2", COLOR_WHITE);

  // Button labels (6-col spacing matching joytest65 80-col layout)
  auto draw_button_labels = [](uint8_t col, uint8_t row) {
    text(col - 2, row, "1:", COLOR_LIGHTGREY);
    text(col + 4, row, "2:", COLOR_LIGHTGREY);
    text(col + 10, row, "3:", COLOR_LIGHTGREY);
    text(col + 1, row + 1, "4:", COLOR_LIGHTGREY);
    text(col + 7, row + 1, "5:", COLOR_LIGHTGREY);
  };
  draw_button_labels(JOY1_BTN_COL, BTN_ROW);
  draw_button_labels(JOY2_BTN_COL, BTN_ROW);

  // Raw CIA value labels (needs CRAM2K — already enabled)
  text(20, CIA_ROW, "DC01:%", COLOR_LIGHTGREY);
  text(32, CIA_ROW, "$", COLOR_LIGHTGREY);
  text(43, CIA_ROW, "DC00:%", COLOR_LIGHTGREY);
  text(55, CIA_ROW, "$", COLOR_LIGHTGREY);

  // Paddle labels
  constexpr const char *paddle_names[] = {"1A:", "1B:", "2A:", "2B:"};
  for (uint8_t i = 0; i < 4; ++i) {
    uint8_t row = PADDLE_ROW + i;
    text(1, row, paddle_names[i], COLOR_LIGHTGREY);
    text(5, row, "[", COLOR_LIGHTGREY);
    text(BAR_COL + BAR_W, row, "]", COLOR_LIGHTGREY);
    text(BAR_COL + BAR_W + 2, row, "$", COLOR_LIGHTGREY);
  }

  cram2k_off();
}

int main() {
  setup();

  mega65_joy_state_t joy1, joy2;
  while (true) {
    // Wait for rasterline 255 (vertical blank) to avoid mid-frame tearing
    while (VICIV.rasterline != 0xFF)
      ;

    // Border colour change shows how long the paddle read takes
    ++VICIV.bordercol;
    mega65_joy_paddles(1, &joy1);
    mega65_joy_paddles(2, &joy2);
    --VICIV.bordercol;

    // Upper screen (colour RAM offsets < 1024): no CRAM2K needed
    draw_dpad(JOY1_DPAD_COL, DPAD_ROW, joy1);
    draw_buttons(JOY1_BTN_COL, BTN_ROW, joy1);
    draw_dpad(JOY2_DPAD_COL, DPAD_ROW, joy2);
    draw_buttons(JOY2_BTN_COL, BTN_ROW, joy2);

    // Lower screen (offsets >= 1024): batch with CRAM2K enabled
    cram2k_on();

    plot_binary(26, CIA_ROW, joy1.cia, COLOR_WHITE);
    plot_hex(33, CIA_ROW, joy1.cia, COLOR_WHITE);
    plot_binary(49, CIA_ROW, joy2.cia, COLOR_WHITE);
    plot_hex(56, CIA_ROW, joy2.cia, COLOR_WHITE);

    draw_paddle_bar(PADDLE_ROW, joy1.paddle_a);
    draw_paddle_bar(PADDLE_ROW + 1, joy1.paddle_b);
    draw_paddle_bar(PADDLE_ROW + 2, joy2.paddle_a);
    draw_paddle_bar(PADDLE_ROW + 3, joy2.paddle_b);

    cram2k_off();
  }
}
