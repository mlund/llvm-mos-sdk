/*
Copyright 2021 LLVM-MOS Project
Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
information.
*/

#ifndef FAKE6502_H
#define FAKE6502_H

#include <stdint.h>

// CPU variants understood by reset6502().
enum {
  CPU_6502 = 0,
  CPU_65C02 = 1,
  CPU_65CE02 = 2,
};

void reset6502(uint8_t cpu);
void step6502(void);

extern uint64_t clockticks6502;
extern uint16_t pc;
extern uint8_t a, x, y, z, sp, status;

// Abort when Z is non-zero at a transfer of control into compiled code, which
// llvm-mos requires it to be. Clear for assembly that keeps Z live across a
// call. Only the 65CE02 can reach a non-zero Z.
extern int fake6502_check_z;

// Callbacks the emulator's host must provide. sim_abort() reports a fault the
// emulator cannot continue past and does not return.
uint8_t read6502(uint16_t address);
void write6502(uint16_t address, uint8_t value);
void sim_abort(const char *msg);

#endif // FAKE6502_H
