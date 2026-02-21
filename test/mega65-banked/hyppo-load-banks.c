// Test Hyppo SD-card bank loading: banked functions are loaded from the
// virtual SD card via mega65_h_loadfile into their physical addresses.
// The main PRG autoboots from D81; bank data lives on the HDOS virtual SD.
//
// Exit codes (xemu $D6CF protocol):
//   0   = all OK
//   1   = bank 1 file load failed
//   2   = bank 2 file load failed
//   3   = bank 1 function returned wrong signature
//   4   = bank 2 function returned wrong signature

#include <mapper.h>
#include <mega65.h>
#include <stdint.h>
#include "../mega65-common/xemu-test.h"

// Override the CRT KERNAL bank loader — we load via Hyppo instead.
void __load_banks(void) {}

// Signal address outside the banked window.
// ZP byte $FC is free from compiler ($02-$8F) and never remapped.
#define SIG ((volatile uint8_t *)0x00FC)

// Banked functions - each writes a unique signature to the ZP signal byte.
__attribute__((noinline, section(".bank_1")))
void modify_bank1(void) {
    *SIG = 0xB1;
}

__attribute__((noinline, section(".bank_2")))
void modify_bank2(void) {
    *SIG = 0xB2;
}

int main(void) {
    // Load bank data from SD card to physical addresses.
    mega65_h_setname("BANK1.BIN");
    if (mega65_h_loadfile(0x10800UL))
        xemu_exit(1);

    mega65_h_setname("BANK2.BIN");
    if (mega65_h_loadfile(0x16800UL))
        xemu_exit(2);

    // Call bank 1 function and verify.
    *SIG = 0;
    banked_call(1, modify_bank1);
    if (*SIG != 0xB1)
        xemu_exit(3);

    // Call bank 2 function and verify.
    *SIG = 0;
    banked_call(2, modify_bank2);
    if (*SIG != 0xB2)
        xemu_exit(4);

    xemu_exit(0);
}
