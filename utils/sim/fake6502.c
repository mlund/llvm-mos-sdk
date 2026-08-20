// clang-format off

/*
Copyright 2021 LLVM-MOS Project
Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
information.
*/

/* Fake6502 CPU emulator core v1.1 *******************
 * Originally by Mike Chambers (miker00lz@gmail.com) *
 * Modified by LLVM-MOS project.                     *
 *****************************************************
 * v1.1 - Small bugfix in BIT opcode, but it was the *
 *        difference between a few games in my NES   *
 *        emulator working and being broken!         *
 *        I went through the rest carefully again    *
 *        after fixing it just to make sure I didn't *
 *        have any other typos! (Dec. 17, 2011)      *
 *                                                   *
 * v1.0 - First release (Nov. 24, 2011)              *
 *****************************************************
 * Fake6502 is a MOS Technology 6502 CPU emulation   *
 * engine in C. It was written as part of a Nintendo *
 * Entertainment System emulator I've been writing.  *
 *                                                   *
 * A couple important things to know about are two   *
 * defines in the code. One is "UNDOCUMENTED" which, *
 * when defined, allows Fake6502 to compile with     *
 * full support for the more predictable             *
 * undocumented instructions of the 6502. If it is   *
 * undefined, undocumented opcodes just act as NOPs. *
 *                                                   *
 * The other define is "NES_CPU", which causes the   *
 * code to compile without support for binary-coded  *
 * decimal (BCD) support for the ADC and SBC         *
 * opcodes. The Ricoh 2A03 CPU in the NES does not   *
 * support BCD, but is otherwise identical to the    *
 * standard MOS 6502. (Note that this define is      *
 * enabled in this file if you haven't changed it    *
 * yourself. If you're not emulating a NES, you      *
 * should comment it out.)                           *
 *                                                   *
 * If you do discover an error in timing accuracy,   *
 * or operation in general please e-mail me at the   *
 * address above so that I can fix it. Thank you!    *
 *                                                   *
 *****************************************************
 * Usage:                                            *
 *                                                   *
 * Fake6502 requires you to provide two external     *
 * functions:                                        *
 *                                                   *
 * uint8_t read6502(uint16_t address)                *
 * void write6502(uint16_t address, uint8_t value)   *
 *                                                   *
 * You may optionally pass Fake6502 the pointer to a *
 * function which you want to be called after every  *
 * emulated instruction. This function should be a   *
 * void with no parameters expected to be passed to  *
 * it.                                               *
 *                                                   *
 * This can be very useful. For example, in a NES    *
 * emulator, you check the number of clock ticks     *
 * that have passed so you can know when to handle   *
 * APU events.                                       *
 *                                                   *
 * To pass Fake6502 this pointer, use the            *
 * hookexternal(void *funcptr) function provided.    *
 *                                                   *
 * To disable the hook later, pass NULL to it.       *
 *****************************************************
 * Useful functions in this emulator:                *
 *                                                   *
 * void reset6502(uint8_t cpu)                       *
 *   - Call this once before you begin execution.    *
 *   - cpu selects the variant to emulate; see the   *
 *     CPU_* constants in fake6502.h.                *
 *                                                   *
 * void exec6502(uint32_t tickcount)                 *
 *   - Execute 6502 code up to the next specified    *
 *     count of clock ticks.                         *
 *                                                   *
 * void step6502()                                   *
 *   - Execute a single instrution.                  *
 *                                                   *
 * void irq6502()                                    *
 *   - Trigger a hardware IRQ in the 6502 core.      *
 *                                                   *
 * void nmi6502()                                    *
 *   - Trigger an NMI in the 6502 core.              *
 *                                                   *
 * void hookexternal(void *funcptr)                  *
 *   - Pass a pointer to a void function taking no   *
 *     parameters. This will cause Fake6502 to call  *
 *     that function once after each emulated        *
 *     instruction.                                  *
 *                                                   *
 *****************************************************
 * Useful variables in this emulator:                *
 *                                                   *
 * uint64_t clockticks6502                           *
 *   - A running total of the emulated cycle count.  *
 *                                                   *
 * uint32_t instructions                             *
 *   - A running total of the total emulated         *
 *     instruction count. This is not related to     *
 *     clock cycle timing.                           *
 *                                                   *
 *****************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fake6502.h"

//6502 defines
#define UNDOCUMENTED //when this is defined, undocumented opcodes are handled.
                     //otherwise, they're simply treated as NOPs.

//#define NES_CPU    //when this is defined, the binary-coded decimal (BCD)
                     //status flag is not honored by ADC and SBC. the 2A03
                     //CPU in the Nintendo Entertainment System does not
                     //support BCD operation.

#define FLAG_CARRY     0x01
#define FLAG_ZERO      0x02
#define FLAG_INTERRUPT 0x04
#define FLAG_DECIMAL   0x08
#define FLAG_BREAK     0x10
#define FLAG_CONSTANT  0x20
#define FLAG_OVERFLOW  0x40
#define FLAG_SIGN      0x80

#define BASE_STACK     0x100

//6502 CPU registers. z is the 65CE02 index register, unused by other variants.
uint16_t pc;
uint8_t sp, a, x, y, z, status;


//helper variables
uint32_t instructions = 0; //keep track of total instructions executed
uint64_t clockticks6502 = 0, clockgoal6502 = 0;
uint16_t oldpc, ea, reladdr, value, result;
static uint16_t opcode_pc; //the fetched opcode's address, for diagnostics
uint8_t opcode, oldstatus;

static inline void saveaccum(uint16_t result) {
  a = (uint8_t)(result & 0x00FF);
}

//flag modifier functions
static inline void setcarry(void) { status |= FLAG_CARRY; }
static inline void clearcarry(void) { status &= ~FLAG_CARRY; }
static inline void setzero(void) { status |= FLAG_ZERO; }
static inline void clearzero(void) { status &= ~FLAG_ZERO; }
static inline void setinterrupt(void) { status |= FLAG_INTERRUPT; }
static inline void clearinterrupt(void) { status &= ~FLAG_INTERRUPT; }
static inline void setdecimal(void) { status |= FLAG_DECIMAL; }
static inline void cleardecimal(void) { status &= ~FLAG_DECIMAL; }
static inline void setoverflow(void) { status |= FLAG_OVERFLOW; }
static inline void clearoverflow(void) { status &= ~FLAG_OVERFLOW; }
static inline void setsign(void) { status |= FLAG_SIGN; }
static inline void clearsign(void) { status &= ~FLAG_SIGN; }

//flag calculation functions
static inline void zerocalc(uint16_t result) {
  if (result & 0x00FF)
    clearzero();
  else
    setzero();
}

static inline void signcalc(uint16_t result) {
  if (result & 0x0080)
    setsign();
  else
    clearsign();
}

static inline void carrycalc(uint16_t result) {
  if (result & 0xFF00)
    setcarry();
  else
    clearcarry();
}

static inline void overflowcalc(uint16_t result, uint16_t memory) {
  if ((result ^ (uint16_t)a) & (result ^ memory) & 0x0080)
    setoverflow();
  else
    clearoverflow();
}

//externally supplied functions are declared in fake6502.h

//a few general functions used by various other functions
void push16(uint16_t pushval) {
    write6502(BASE_STACK + sp, (pushval >> 8) & 0xFF);
    write6502(BASE_STACK + ((sp - 1) & 0xFF), pushval & 0xFF);
    sp -= 2;
}

void push8(uint8_t pushval) {
    write6502(BASE_STACK + sp--, pushval);
}

uint16_t pull16() {
    uint16_t temp16;
    temp16 = read6502(BASE_STACK + ((sp + 1) & 0xFF)) | ((uint16_t)read6502(BASE_STACK + ((sp + 2) & 0xFF)) << 8);
    sp += 2;
    return(temp16);
}

uint8_t pull8() {
    return (read6502(BASE_STACK + ++sp));
}


static void (**addrtable)() = NULL;
static void (**optable)() = NULL;
static const uint32_t *ticktable = NULL;
uint8_t penaltyop, penaltyaddr;

//addressing mode functions, calculates effective addresses
static void imp() { //implied
}

static void acc() { //accumulator
}

static void imm() { //immediate
    ea = pc++;
}

static void zp() { //zero-page
    ea = (uint16_t)read6502((uint16_t)pc++);
}

static void zpx() { //zero-page,X
    ea = ((uint16_t)read6502((uint16_t)pc++) + (uint16_t)x) & 0xFF; //zero-page wraparound
}

static void zpy() { //zero-page,Y
    ea = ((uint16_t)read6502((uint16_t)pc++) + (uint16_t)y) & 0xFF; //zero-page wraparound
}

static void rel() { //relative for branch ops (8-bit immediate value, sign-extended)
    reladdr = (uint16_t)read6502(pc++);
    if (reladdr & 0x80) reladdr |= 0xFF00;
}

static void zpr() { //combined zp, rel for bbr/bbs
    ea = (uint16_t)read6502((uint16_t)pc++);
    reladdr = (uint16_t)read6502(pc++);
    if (reladdr & 0x80) reladdr |= 0xFF00;
}

static void immw() { //16-bit immediate, leaving the literal in ea
    ea = (uint16_t)read6502(pc) | ((uint16_t)read6502(pc+1) << 8);
    pc += 2;
}

static void abso() { //absolute
    ea = (uint16_t)read6502(pc) | ((uint16_t)read6502(pc+1) << 8);
    pc += 2;
}

static void absx() { //absolute,X
    uint16_t startpage;
    ea = ((uint16_t)read6502(pc) | ((uint16_t)read6502(pc+1) << 8));
    startpage = ea & 0xFF00;
    ea += (uint16_t)x;

    if (startpage != (ea & 0xFF00)) { //one cycle penlty for page-crossing on some opcodes
        penaltyaddr = 1;
    }

    pc += 2;
}

static void absy() { //absolute,Y
    uint16_t startpage;
    ea = ((uint16_t)read6502(pc) | ((uint16_t)read6502(pc+1) << 8));
    startpage = ea & 0xFF00;
    ea += (uint16_t)y;

    if (startpage != (ea & 0xFF00)) { //one cycle penlty for page-crossing on some opcodes
        penaltyaddr = 1;
    }

    pc += 2;
}

static void ind() { //indirect, NMOS: replicate the page-boundary wraparound bug
    uint16_t eahelp, eahelp2;
    eahelp = (uint16_t)read6502(pc) | (uint16_t)((uint16_t)read6502(pc+1) << 8);
    eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF);
    ea = (uint16_t)read6502(eahelp) | ((uint16_t)read6502(eahelp2) << 8);
    pc += 2;
}

static void indl() { //indirect, CMOS: the 65C02 fixed the wraparound bug
    uint16_t eahelp;
    eahelp = (uint16_t)read6502(pc) | (uint16_t)((uint16_t)read6502(pc+1) << 8);
    ea = (uint16_t)read6502(eahelp) | ((uint16_t)read6502((uint16_t)(eahelp + 1)) << 8);
    pc += 2;
}

static void inzp() { //indirectZP
    uint16_t eahelp;
    eahelp = (uint16_t)(((uint16_t)read6502(pc++)) & 0xFF); //zero-page wraparound for table pointer
    ea = (uint16_t)read6502(eahelp & 0x00FF) | ((uint16_t)read6502((eahelp+1) & 0x00FF) << 8);
}

static void inzpz() { //indirectZP, CE02: every ($nn) is ($nn),Z
    inzp();
    ea += z;
}

static void indx() { // (indirect,X)
    uint16_t eahelp;
    eahelp = (uint16_t)(((uint16_t)read6502(pc++) + (uint16_t)x) & 0xFF); //zero-page wraparound for table pointer
    ea = (uint16_t)read6502(eahelp & 0x00FF) | ((uint16_t)read6502((eahelp+1) & 0x00FF) << 8);
}

static void inax() { // (indirectABS,X)
    uint16_t eahelp;
    eahelp = ((uint16_t)read6502(pc) | (uint16_t)((uint16_t)read6502(pc+1) << 8)) + (uint16_t)x;
    //No page-boundary wraparound: JMP (abs,X) is a 65C02 addition and jams on
    //the NMOS 6502, so it never runs on a CPU that has the JMP (abs) bug. That
    //bug belongs to ind(), which still replicates it.
    ea = (uint16_t)read6502(eahelp) | ((uint16_t)read6502((uint16_t)(eahelp + 1)) << 8);
    pc += 2;
}

static void indy() { // (indirect),Y
    uint16_t eahelp, eahelp2, startpage;
    eahelp = (uint16_t)read6502(pc++);
    eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF); //zero-page wraparound
    ea = (uint16_t)read6502(eahelp) | ((uint16_t)read6502(eahelp2) << 8);
    startpage = ea & 0xFF00;
    ea += (uint16_t)y;

    if (startpage != (ea & 0xFF00)) { //one cycle penlty for page-crossing on some opcodes
        penaltyaddr = 1;
    }
}

static uint16_t getvalue() {
    if (addrtable[opcode] == acc) return((uint16_t)a);
        else return((uint16_t)read6502(ea));
}

static void putvalue(uint16_t saveval) {
    if (addrtable[opcode] == acc) a = (uint8_t)(saveval & 0x00FF);
        else write6502(ea, (saveval & 0x00FF));
}


//instruction handler functions
static void adc() {
    penaltyop = 1;
    value = getvalue();
    result = (uint16_t)a + value + (uint16_t)(status & FLAG_CARRY);

    zerocalc(result);
    overflowcalc(result, value);
    signcalc(result);

    #ifndef NES_CPU
    if (status & FLAG_DECIMAL)       /* detect and apply BCD nybble carries */
        result += ((((result + 0x66) ^ (uint16_t)a ^ value) >> 3) & 0x22) * 3;
    #endif

    carrycalc(result);
    saveaccum(result);
}

static void and() {
    penaltyop = 1;
    value = getvalue();
    result = (uint16_t)a & value;

    zerocalc(result);
    signcalc(result);

    saveaccum(result);
}

static void asl() {
    value = getvalue();
    result = value << 1;

    carrycalc(result);
    zerocalc(result);
    signcalc(result);

    putvalue(result);
}

static void bcc() {
    if ((status & FLAG_CARRY) == 0) {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
            else clockticks6502++;
    }
}

static void bcs() {
    if ((status & FLAG_CARRY) == FLAG_CARRY) {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
            else clockticks6502++;
    }
}

static void beq() {
    if ((status & FLAG_ZERO) == FLAG_ZERO) {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
            else clockticks6502++;
    }
}

static void bra() {
    oldpc = pc;
    pc += reladdr;
    if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 1; //check if jump crossed a page boundary
}

static void bit() {
    value = getvalue();
    result = (uint16_t)a & value;

    zerocalc(result);
    // Immediate addressing mode only affects Z.
    if (opcode != 0x89)
      status = (status & 0x3F) | (uint8_t)(value & 0xC0);
}

static void bmi() {
    if ((status & FLAG_SIGN) == FLAG_SIGN) {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
            else clockticks6502++;
    }
}

static void bne() {
    if ((status & FLAG_ZERO) == 0) {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
            else clockticks6502++;
    }
}

static void bpl() {
    if ((status & FLAG_SIGN) == 0) {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
            else clockticks6502++;
    }
}

static void brk() {
    pc++;
    push16(pc); //push next instruction address onto stack
    push8(status | FLAG_BREAK); //push CPU status to stack
    setinterrupt(); //set interrupt flag
    pc = (uint16_t)read6502(0xFFFE) | ((uint16_t)read6502(0xFFFF) << 8);
}

static void bvc() {
    if ((status & FLAG_OVERFLOW) == 0) {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
            else clockticks6502++;
    }
}

static void bvs() {
    if ((status & FLAG_OVERFLOW) == FLAG_OVERFLOW) {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
            else clockticks6502++;
    }
}

static void clc() {
    clearcarry();
}

static void cld() {
    cleardecimal();
}

static void cli() {
    clearinterrupt();
}

static void clv() {
    clearoverflow();
}

static void cmp() {
    penaltyop = 1;
    value = getvalue();
    result = (uint16_t)a - value;

    if (a >= (uint8_t)(value & 0x00FF)) setcarry();
        else clearcarry();
    if (a == (uint8_t)(value & 0x00FF)) setzero();
        else clearzero();
    signcalc(result);
}

static void cpx() {
    value = getvalue();
    result = (uint16_t)x - value;

    if (x >= (uint8_t)(value & 0x00FF)) setcarry();
        else clearcarry();
    if (x == (uint8_t)(value & 0x00FF)) setzero();
        else clearzero();
    signcalc(result);
}

static void cpy() {
    value = getvalue();
    result = (uint16_t)y - value;

    if (y >= (uint8_t)(value & 0x00FF)) setcarry();
        else clearcarry();
    if (y == (uint8_t)(value & 0x00FF)) setzero();
        else clearzero();
    signcalc(result);
}

static void dec() {
    value = getvalue();
    result = value - 1;

    zerocalc(result);
    signcalc(result);

    putvalue(result);
}

static void dex() {
    x--;

    zerocalc(x);
    signcalc(x);
}

static void dey() {
    y--;

    zerocalc(y);
    signcalc(y);
}

static void eor() {
    penaltyop = 1;
    value = getvalue();
    result = (uint16_t)a ^ value;

    zerocalc(result);
    signcalc(result);

    saveaccum(result);
}

static void inc() {
    value = getvalue();
    result = value + 1;

    zerocalc(result);
    signcalc(result);

    putvalue(result);
}

static void inx() {
    x++;

    zerocalc(x);
    signcalc(x);
}

static void iny() {
    y++;

    zerocalc(y);
    signcalc(y);
}

static void jmp() {
    pc = ea;
}

//llvm-mos requires Z to be zero wherever control reaches compiled code. The
//guard sits at those transfers rather than at the write, so that assembly is
//free to use Z in between. Assembly that keeps Z live across a call trips it,
//which --keep-z exists for.
int fake6502_check_z = 1;

static void checkz(const char *what) {
    char msg[80];

    if (!z || !fake6502_check_z)
        return;
    snprintf(msg, sizeof msg, "65CE02 Z is $%02x, not 0, at %s at $%04x",
             z, what, opcode_pc);
    sim_abort(msg);
}

static void jsr() {
    checkz("jsr");
    push16(pc - 1);
    pc = ea;
}

static void lda() {
    penaltyop = 1;
    value = getvalue();
    a = (uint8_t)(value & 0x00FF);

    zerocalc(a);
    signcalc(a);
}

static void ldx() {
    penaltyop = 1;
    value = getvalue();
    x = (uint8_t)(value & 0x00FF);

    zerocalc(x);
    signcalc(x);
}

static void ldy() {
    penaltyop = 1;
    value = getvalue();
    y = (uint8_t)(value & 0x00FF);

    zerocalc(y);
    signcalc(y);
}

static void lsr() {
    value = getvalue();
    result = value >> 1;

    if (value & 1) setcarry();
        else clearcarry();
    zerocalc(result);
    signcalc(result);

    putvalue(result);
}

static void nop() {
    switch (opcode) {
        case 0x1C:
        case 0x3C:
        case 0x5C:
        case 0x7C:
        case 0xDC:
        case 0xFC:
            penaltyop = 1;
            break;
    }
}

static void ora() {
    penaltyop = 1;
    value = getvalue();
    result = (uint16_t)a | value;

    zerocalc(result);
    signcalc(result);

    saveaccum(result);
}

static void pha() {
    push8(a);
}

static void php() {
    push8(status | FLAG_BREAK);
}

static void phx() {
    push8(x);
}

static void phy() {
    push8(y);
}

static void pla() {
    a = pull8();

    zerocalc(a);
    signcalc(a);
}

static void plp() {
    status = pull8() | FLAG_CONSTANT;
}

static void plx() {
    x = pull8();

    zerocalc(x);
    signcalc(x);
}

static void ply() {
    y = pull8();

    zerocalc(y);
    signcalc(y);
}

static void rol() {
    value = getvalue();
    result = (value << 1) | (status & FLAG_CARRY);

    carrycalc(result);
    zerocalc(result);
    signcalc(result);

    putvalue(result);
}

static void ror() {
    value = getvalue();
    result = (value >> 1) | ((status & FLAG_CARRY) << 7);

    if (value & 1) setcarry();
        else clearcarry();
    zerocalc(result);
    signcalc(result);

    putvalue(result);
}

static void rti() {
    checkz("rti");
    status = pull8();
    value = pull16();
    pc = value;
}

static void rts() {
    checkz("rts");
    value = pull16();
    pc = value + 1;
}

static void sbc() {
  penaltyop = 1;
  value = getvalue() ^ 0x00FF; /* ones complement */

#ifndef NES_CPU
  if (status & FLAG_DECIMAL) /* use nines complement for BCD */
    value -= 0x0066;
#endif

  result = (uint16_t)a + value + (uint16_t)(status & FLAG_CARRY);

  zerocalc(result);
  overflowcalc(result, value);
  signcalc(result);

#ifndef NES_CPU
  if (status & FLAG_DECIMAL) /* detect and apply BCD nybble carries */
    result += ((((result + 0x66) ^ (uint16_t)a ^ value) >> 3) & 0x22) * 3;
#endif

  carrycalc(result);
  saveaccum(result);
}

static void sec() {
    setcarry();
}

static void sed() {
    setdecimal();
}

static void sei() {
    setinterrupt();
}

static void sta() {
    putvalue(a);
}

static void stx() {
    putvalue(x);
}

static void sty() {
    putvalue(y);
}

static void stz() {
    putvalue(0);
}

static void tax() {
    x = a;

    zerocalc(x);
    signcalc(x);
}

static void tay() {
    y = a;

    zerocalc(y);
    signcalc(y);
}

static void tsx() {
    x = sp;

    zerocalc(x);
    signcalc(x);
}

static void txa() {
    a = x;

    zerocalc(a);
    signcalc(a);
}

static void txs() {
    sp = x;
}

static void tya() {
    a = y;

    zerocalc(a);
    signcalc(a);
}

static void tsb() {
    value = getvalue();
    zerocalc(value & a);
    putvalue(value | a);
}

static void trb() {
    value = getvalue();
    zerocalc(value & a);
    putvalue(value & ~a);
}

#define DEF_BBR(idx)                                                           \
static void bbr##idx() {                                                       \
    value = getvalue();                                                        \
    if ((value & (1 << (idx))) == 0) {                                         \
        oldpc = pc;                                                            \
        pc += reladdr;                                                         \
        if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2;            \
            else clockticks6502++;                                             \
    }                                                                          \
}
DEF_BBR(0)
DEF_BBR(1)
DEF_BBR(2)
DEF_BBR(3)
DEF_BBR(4)
DEF_BBR(5)
DEF_BBR(6)
DEF_BBR(7)

#define DEF_BBS(idx)                                                           \
static void bbs##idx() {                                                       \
    value = getvalue();                                                        \
    if ((value & (1 << (idx))) != 0) {                                         \
        oldpc = pc;                                                            \
        pc += reladdr;                                                         \
        if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2;            \
            else clockticks6502++;                                             \
    }                                                                          \
}
DEF_BBS(0)
DEF_BBS(1)
DEF_BBS(2)
DEF_BBS(3)
DEF_BBS(4)
DEF_BBS(5)
DEF_BBS(6)
DEF_BBS(7)

#define DEF_RMB(idx)                                                           \
static void rmb##idx() {                                                       \
    value = getvalue();                                                        \
    value &= ~(1 << (idx));                                                    \
    putvalue(value);                                                           \
}
DEF_RMB(0)
DEF_RMB(1)
DEF_RMB(2)
DEF_RMB(3)
DEF_RMB(4)
DEF_RMB(5)
DEF_RMB(6)
DEF_RMB(7)

#define DEF_SMB(idx)                                                           \
static void smb##idx() {                                                       \
    value = getvalue();                                                        \
    value |= 1 << (idx);                                                       \
    putvalue(value);                                                           \
}
DEF_SMB(0)
DEF_SMB(1)
DEF_SMB(2)
DEF_SMB(3)
DEF_SMB(4)
DEF_SMB(5)
DEF_SMB(6)
DEF_SMB(7)

// TODO: Implement these by adding emulation wait and stop states.
static void wai() {}
static void stp() {}

//undocumented instructions
#ifdef UNDOCUMENTED
    static void lax() {
        lda();
        ldx();
    }

    static void sax() {
        sta();
        stx();
        putvalue(a & x);
        if (penaltyop && penaltyaddr) clockticks6502--;
    }

    static void dcp() {
        dec();
        cmp();
        if (penaltyop && penaltyaddr) clockticks6502--;
    }

    static void isb() {
        inc();
        sbc();
        if (penaltyop && penaltyaddr) clockticks6502--;
    }

    static void slo() {
        asl();
        ora();
        if (penaltyop && penaltyaddr) clockticks6502--;
    }

    static void rla() {
        rol();
        and();
        if (penaltyop && penaltyaddr) clockticks6502--;
    }

    static void sre() {
        lsr();
        eor();
        if (penaltyop && penaltyaddr) clockticks6502--;
    }

    static void rra() {
        ror();
        adc();
        if (penaltyop && penaltyaddr) clockticks6502--;
    }
#else
    #define lax nop
    #define sax nop
    #define dcp nop
    #define isb nop
    #define slo nop
    #define rla nop
    #define sre nop
    #define rra nop
#endif


static void (*addrtable_nmos[256])() = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
/* 0 */     imp, indx,  imp, indx,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imm, abso, abso, abso, abso, /* 0 */
/* 1 */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* 1 */
/* 2 */    abso, indx,  imp, indx,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imm, abso, abso, abso, abso, /* 2 */
/* 3 */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* 3 */
/* 4 */     imp, indx,  imp, indx,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imm, abso, abso, abso, abso, /* 4 */
/* 5 */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* 5 */
/* 6 */     imp, indx,  imp, indx,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imm,  ind, abso, abso, abso, /* 6 */
/* 7 */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* 7 */
/* 8 */     imm, indx,  imm, indx,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imm, abso, abso, abso, abso, /* 8 */
/* 9 */     rel, indy,  imp, indy,  zpx,  zpx,  zpy,  zpy,  imp, absy,  imp, absy, absx, absx, absy, absy, /* 9 */
/* A */     imm, indx,  imm, indx,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imm, abso, abso, abso, abso, /* A */
/* B */     rel, indy,  imp, indy,  zpx,  zpx,  zpy,  zpy,  imp, absy,  imp, absy, absx, absx, absy, absy, /* B */
/* C */     imm, indx,  imm, indx,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imm, abso, abso, abso, abso, /* C */
/* D */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* D */
/* E */     imm, indx,  imm, indx,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imm, abso, abso, abso, abso, /* E */
/* F */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx  /* F */
};

static void (*optable_nmos[256])() = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
/* 0 */     brk,  ora,  nop,  slo,  nop,  ora,  asl,  slo,  php,  ora,  asl,  nop,  nop,  ora,  asl,  slo, /* 0 */
/* 1 */     bpl,  ora,  nop,  slo,  nop,  ora,  asl,  slo,  clc,  ora,  nop,  slo,  nop,  ora,  asl,  slo, /* 1 */
/* 2 */     jsr,  and,  nop,  rla,  bit,  and,  rol,  rla,  plp,  and,  rol,  nop,  bit,  and,  rol,  rla, /* 2 */
/* 3 */     bmi,  and,  nop,  rla,  nop,  and,  rol,  rla,  sec,  and,  nop,  rla,  nop,  and,  rol,  rla, /* 3 */
/* 4 */     rti,  eor,  nop,  sre,  nop,  eor,  lsr,  sre,  pha,  eor,  lsr,  nop,  jmp,  eor,  lsr,  sre, /* 4 */
/* 5 */     bvc,  eor,  nop,  sre,  nop,  eor,  lsr,  sre,  cli,  eor,  nop,  sre,  nop,  eor,  lsr,  sre, /* 5 */
/* 6 */     rts,  adc,  nop,  rra,  nop,  adc,  ror,  rra,  pla,  adc,  ror,  nop,  jmp,  adc,  ror,  rra, /* 6 */
/* 7 */     bvs,  adc,  nop,  rra,  nop,  adc,  ror,  rra,  sei,  adc,  nop,  rra,  nop,  adc,  ror,  rra, /* 7 */
/* 8 */     nop,  sta,  nop,  sax,  sty,  sta,  stx,  sax,  dey,  nop,  txa,  nop,  sty,  sta,  stx,  sax, /* 8 */
/* 9 */     bcc,  sta,  nop,  nop,  sty,  sta,  stx,  sax,  tya,  sta,  txs,  nop,  nop,  sta,  nop,  nop, /* 9 */
/* A */     ldy,  lda,  ldx,  lax,  ldy,  lda,  ldx,  lax,  tay,  lda,  tax,  nop,  ldy,  lda,  ldx,  lax, /* A */
/* B */     bcs,  lda,  nop,  lax,  ldy,  lda,  ldx,  lax,  clv,  lda,  tsx,  lax,  ldy,  lda,  ldx,  lax, /* B */
/* C */     cpy,  cmp,  nop,  dcp,  cpy,  cmp,  dec,  dcp,  iny,  cmp,  dex,  nop,  cpy,  cmp,  dec,  dcp, /* C */
/* D */     bne,  cmp,  nop,  dcp,  nop,  cmp,  dec,  dcp,  cld,  cmp,  nop,  dcp,  nop,  cmp,  dec,  dcp, /* D */
/* E */     cpx,  sbc,  nop,  isb,  cpx,  sbc,  inc,  isb,  inx,  sbc,  nop,  sbc,  cpx,  sbc,  inc,  isb, /* E */
/* F */     beq,  sbc,  nop,  isb,  nop,  sbc,  inc,  isb,  sed,  sbc,  nop,  isb,  nop,  sbc,  inc,  isb  /* F */
};

static const uint32_t ticktable_nmos[256] = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
/* 0 */      7,    6,    2,    8,    3,    3,    5,    5,    3,    2,    2,    2,    4,    4,    6,    6,  /* 0 */
/* 1 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 1 */
/* 2 */      6,    6,    2,    8,    3,    3,    5,    5,    4,    2,    2,    2,    4,    4,    6,    6,  /* 2 */
/* 3 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 3 */
/* 4 */      6,    6,    2,    8,    3,    3,    5,    5,    3,    2,    2,    2,    3,    4,    6,    6,  /* 4 */
/* 5 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 5 */
/* 6 */      6,    6,    2,    8,    3,    3,    5,    5,    4,    2,    2,    2,    5,    4,    6,    6,  /* 6 */
/* 7 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 7 */
/* 8 */      2,    6,    2,    6,    3,    3,    3,    3,    2,    2,    2,    2,    4,    4,    4,    4,  /* 8 */
/* 9 */      2,    6,    2,    6,    4,    4,    4,    4,    2,    5,    2,    5,    5,    5,    5,    5,  /* 9 */
/* A */      2,    6,    2,    6,    3,    3,    3,    3,    2,    2,    2,    2,    4,    4,    4,    4,  /* A */
/* B */      2,    5,    2,    5,    4,    4,    4,    4,    2,    4,    2,    4,    4,    4,    4,    4,  /* B */
/* C */      2,    6,    2,    8,    3,    3,    5,    5,    2,    2,    2,    2,    4,    4,    6,    6,  /* C */
/* D */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* D */
/* E */      2,    6,    2,    8,    3,    3,    5,    5,    2,    2,    2,    2,    4,    4,    6,    6,  /* E */
/* F */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7   /* F */
};

static void (*addrtable_cmos[256])() = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
/* 0 */     imp, indx,  imm,  imp,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imp, abso, abso, abso,  zpr, /* 0 */
/* 1 */     rel, indy, inzp,  imp,   zp,  zpx,  zpx,   zp,  imp, absy,  acc,  imp, abso, absx, absx,  zpr, /* 1 */
/* 2 */    abso, indx,  imm,  imp,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imp, abso, abso, abso,  zpr, /* 2 */
/* 3 */     rel, indy, inzp,  imp,  zpx,  zpx,  zpx,   zp,  imp, absy,  acc,  imp, absx, absx, absx,  zpr, /* 3 */
/* 4 */     imp, indx,  imm,  imp,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imp, abso, abso, abso,  zpr, /* 4 */
/* 5 */     rel, indy, inzp,  imp,  zpx,  zpx,  zpx,   zp,  imp, absy,  imp,  imp, abso, absx, absx,  zpr, /* 5 */
/* 6 */     imp, indx,  imm,  imp,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imp, indl, abso, abso,  zpr, /* 6 */
/* 7 */     rel, indy, inzp,  imp,  zpx,  zpx,  zpx,   zp,  imp, absy,  imp,  imp, inax, absx, absx,  zpr, /* 7 */
/* 8 */     rel, indx,  imm,  imp,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imp, abso, abso, abso,  zpr, /* 8 */
/* 9 */     rel, indy, inzp,  imp,  zpx,  zpx,  zpy,   zp,  imp, absy,  imp,  imp, abso, absx, absx,  zpr, /* 9 */
/* A */     imm, indx,  imm,  imp,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imp, abso, abso, abso,  zpr, /* A */
/* B */     rel, indy, inzp,  imp,  zpx,  zpx,  zpy,   zp,  imp, absy,  imp,  imp, absx, absx, absy,  zpr, /* B */
/* C */     imm, indx,  imm,  imp,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imp, abso, abso, abso,  zpr, /* C */
/* D */     rel, indy, inzp,  imp,  zpx,  zpx,  zpx,   zp,  imp, absy,  imp,  imp, abso, absx, absx,  zpr, /* D */
/* E */     imm, indx,  imm,  imp,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imp, abso, abso, abso,  zpr, /* E */
/* F */     rel, indy, inzp,  imp,  zpx,  zpx,  zpx,   zp,  imp, absy,  imp,  imp, abso, absx, absx,  zpr  /* F */
};

static void (*optable_cmos[256])() = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7   |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F   |     */
/* 0 */     brk,  ora,  nop,  nop,  tsb,  ora,  asl,  rmb0,  php,  ora,  asl,  nop,  tsb,  ora,  asl,  bbr0, /* 0 */
/* 1 */     bpl,  ora,  ora,  nop,  trb,  ora,  asl,  rmb1,  clc,  ora,  inc,  nop,  trb,  ora,  asl,  bbr1, /* 1 */
/* 2 */     jsr,  and,  nop,  nop,  bit,  and,  rol,  rmb2,  plp,  and,  rol,  nop,  bit,  and,  rol,  bbr2, /* 2 */
/* 3 */     bmi,  and,  and,  nop,  bit,  and,  rol,  rmb3,  sec,  and,  dec,  nop,  bit,  and,  rol,  bbr3, /* 3 */
/* 4 */     rti,  eor,  nop,  nop,  nop,  eor,  lsr,  rmb4,  pha,  eor,  lsr,  nop,  jmp,  eor,  lsr,  bbr4, /* 4 */
/* 5 */     bvc,  eor,  eor,  nop,  nop,  eor,  lsr,  rmb5,  cli,  eor,  phy,  nop,  nop,  eor,  lsr,  bbr5, /* 5 */
/* 6 */     rts,  adc,  nop,  nop,  stz,  adc,  ror,  rmb6,  pla,  adc,  ror,  nop,  jmp,  adc,  ror,  bbr6, /* 6 */
/* 7 */     bvs,  adc,  adc,  nop,  stz,  adc,  ror,  rmb7,  sei,  adc,  ply,  nop,  jmp,  adc,  ror,  bbr7, /* 7 */
/* 8 */     bra,  sta,  nop,  nop,  sty,  sta,  stx,  smb0,  dey,  bit,  txa,  nop,  sty,  sta,  stx,  bbs0, /* 8 */
/* 9 */     bcc,  sta,  sta,  nop,  sty,  sta,  stx,  smb1,  tya,  sta,  txs,  nop,  stz,  sta,  stz,  bbs1, /* 9 */
/* A */     ldy,  lda,  ldx,  nop,  ldy,  lda,  ldx,  smb2,  tay,  lda,  tax,  nop,  ldy,  lda,  ldx,  bbs2, /* A */
/* B */     bcs,  lda,  lda,  nop,  ldy,  lda,  ldx,  smb3,  clv,  lda,  tsx,  nop,  ldy,  lda,  ldx,  bbs3, /* B */
/* C */     cpy,  cmp,  nop,  nop,  cpy,  cmp,  dec,  smb4,  iny,  cmp,  dex,  wai,  cpy,  cmp,  dec,  bbs4, /* C */
/* D */     bne,  cmp,  cmp,  nop,  nop,  cmp,  dec,  smb5,  cld,  cmp,  phx,  stp,  nop,  cmp,  dec,  bbs5, /* D */
/* E */     cpx,  sbc,  nop,  nop,  cpx,  sbc,  inc,  smb6,  inx,  sbc,  nop,  nop,  cpx,  sbc,  inc,  bbs6, /* E */
/* F */     beq,  sbc,  sbc,  nop,  nop,  sbc,  inc,  smb7,  sed,  sbc,  plx,  nop,  nop,  sbc,  inc,  bbs7  /* F */
};

static const uint32_t ticktable_cmos[256] = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
/* 0 */      7,    6,    2,    1,    5,    3,    5,    5,    3,    2,    2,    1,    6,    4,    6,    5,  /* 0 */
/* 1 */      2,    5,    5,    1,    5,    4,    6,    5,    2,    4,    2,    1,    6,    4,    6,    5,  /* 1 */
/* 2 */      6,    6,    2,    1,    3,    3,    5,    5,    4,    2,    2,    1,    4,    4,    6,    5,  /* 2 */
/* 3 */      2,    5,    5,    1,    4,    4,    6,    5,    2,    4,    2,    1,    4,    4,    6,    5,  /* 3 */
/* 4 */      6,    6,    2,    1,    3,    3,    5,    5,    3,    2,    2,    1,    3,    4,    6,    5,  /* 4 */
/* 5 */      2,    5,    5,    1,    4,    4,    6,    5,    2,    4,    3,    1,    8,    4,    6,    5,  /* 5 */
/* 6 */      6,    6,    2,    1,    3,    3,    5,    5,    4,    2,    2,    1,    6,    4,    6,    5,  /* 6 */
/* 7 */      2,    5,    5,    1,    4,    4,    6,    5,    2,    4,    4,    1,    6,    4,    6,    5,  /* 7 */
/* 8 */      3,    6,    2,    1,    3,    3,    3,    5,    2,    2,    2,    1,    4,    4,    4,    5,  /* 8 */
/* 9 */      2,    6,    5,    1,    4,    4,    4,    5,    2,    5,    2,    1,    4,    5,    5,    5,  /* 9 */
/* A */      2,    6,    2,    1,    3,    3,    3,    5,    2,    2,    2,    1,    4,    4,    4,    5,  /* A */
/* B */      2,    5,    5,    1,    4,    4,    4,    5,    2,    4,    2,    1,    4,    4,    4,    5,  /* B */
/* C */      2,    6,    2,    1,    3,    3,    5,    5,    2,    2,    2,    1,    4,    4,    6,    5,  /* C */
/* D */      2,    5,    5,    1,    4,    4,    6,    5,    2,    4,    3,    1,    4,    4,    7,    5,  /* D */
/* E */      2,    6,    2,    1,    3,    3,    5,    5,    2,    2,    2,    1,    4,    4,    6,    5,  /* E */
/* F */      2,    5,    5,    1,    4,    4,    6,    5,    2,    4,    4,    1,    4,    4,    7,    5   /* F */
};

/* 65CE02.
 *
 * The 65CE02 tables are the 65C02 tables with the instructions llvm-mos emits
 * patched in. Every other opcode that differs from the 65C02 is pointed at
 * ce02_unimplemented(), which aborts. That is deliberate: an emulator that
 * silently executes something plausible is worse than one that stops, because
 * the wrong answer gets trusted.
 *
 * Aborting also keeps an invariant the rest of this file relies on: the base
 * page stays at zero and the stack stays 8-bit, because TAB, TBA, CLE, SEE,
 * TSY, TYS and the ($nn,S),Y forms all abort. The zero-page addressing helpers
 * and BASE_STACK above therefore remain valid.
 *
 * Z is emulated rather than forbidden, so that assembly using it can run here.
 * That is why the CE02 table swaps inzp() for inzpz(), and why the guard sits
 * at the transfers of control where Z must be zero rather than at the write;
 * see checkz().
 *
 * Cycle counts come from the 65C02 table and are not corrected for this CPU.
 * The 65CE02 is faster throughout, and the opcodes added below sit in slots
 * the 65C02 leaves unused, where that table holds a filler count of 1. So
 * --cycles and --profile are indicative for 65C02 instructions and simply
 * wrong for the 65CE02-specific ones.
 */

static void (*addrtable_ce02[256])();
static void (*optable_ce02[256])();

static void ce02_unimplemented() {
    char msg[64];

    snprintf(msg, sizeof msg, "65CE02 opcode $%02x at $%04x is not emulated",
             opcode, pc - 1);
    sim_abort(msg);
}

static void neg() { //negate accumulator
    value = getvalue();
    result = 0 - value;

    zerocalc(result);
    signcalc(result);

    putvalue(result);
}

static void asr() { //arithmetic shift right, sign bit preserved
    value = getvalue();
    result = (value >> 1) | (value & 0x80);

    if (value & 1) setcarry();
        else clearcarry();
    zerocalc(result);
    signcalc(result);

    putvalue(result);
}

static uint16_t readword(void) {
    return (uint16_t)read6502(ea) | ((uint16_t)read6502(ea + 1) << 8);
}

static void writeword(uint16_t word) {
    write6502(ea, word & 0x00FF);
    write6502(ea + 1, (word >> 8) & 0x00FF);
}

//The word operations take N and Z from all sixteen bits, not from the low byte
//as zerocalc and signcalc would. llvm-mos depends on it: it chains INW to the
//next word on Z.
static void wordflags(uint16_t word) {
    if (word == 0) setzero();
        else clearzero();
    if (word & 0x8000) setsign();
        else clearsign();
}

static void incdecw(int delta) {
    char msg[64];
    uint16_t word;

    if (ea == 0xFF) { //would straddle the base page; hardware behavior unclear
        snprintf(msg, sizeof msg, "65CE02 %s $ff at $%04x crosses the base page",
                 delta > 0 ? "inw" : "dew", pc - 2);
        sim_abort(msg);
    }

    word = readword() + delta;
    writeword(word);
    wordflags(word);
}

static void inw() {
    incdecw(1);
}

static void dew() {
    incdecw(-1);
}

static void shiftw(uint16_t carryin) {
    uint32_t word = ((uint32_t)readword() << 1) | carryin;

    if (word & 0x10000) setcarry();
        else clearcarry();

    writeword((uint16_t)word);
    wordflags((uint16_t)word);
}

static void asw() {
    shiftw(0);
}

static void row() {
    shiftw((status & FLAG_CARRY) ? 1 : 0);
}

//RTS #$nn. The immediate is the number of argument bytes the caller pushed
//before the call, dropped here so the caller does not have to.
static void rtn() {
    uint8_t drop = (uint8_t)getvalue();

    checkz("rts #nn");

    pc = pull16() + 1;
    sp += drop;
}

//PHW pushes the low byte first, opposite to push16() and every other push here.
static void pushword(uint16_t word) {
    push8(word & 0x00FF);
    push8((word >> 8) & 0x00FF);
}

//PHW #$nnnn: a 16-bit immediate, which immw() leaves in ea.
static void phwi() {
    pushword(ea);
}

//PHW $nnnn pushes the word held at the address, not the address.
static void phw() {
    pushword(readword());
}

static void ldz() {
    value = getvalue();
    z = (uint8_t)(value & 0x00FF);

    zerocalc(z);
    signcalc(z);
}

static void taz() {
    z = a;

    zerocalc(z);
    signcalc(z);
}

static void tza() {
    a = z;

    zerocalc(a);
    signcalc(a);
}

static void inz() {
    z++;

    zerocalc(z);
    signcalc(z);
}

static void dez() {
    z--;

    zerocalc(z);
    signcalc(z);
}

static void phz() {
    push8(z);
}

static void plz() {
    z = pull8();

    zerocalc(z);
    signcalc(z);
}

static void cpz() {
    value = getvalue();
    result = (uint16_t)z - value;

    if (z >= (uint8_t)(value & 0x00FF)) setcarry();
        else clearcarry();
    if (z == (uint8_t)(value & 0x00FF)) setzero();
        else clearzero();
    signcalc(result);
}

//The 65CE02 measures a 16-bit branch displacement from the address of the
//second displacement byte, not from the end of the instruction. By this point
//the addressing mode has advanced pc past all three bytes, so the target is
//pc - 1 + reladdr.
static void rel16() { //relative for 65CE02 16-bit branch ops
    reladdr = (uint16_t)read6502(pc) | ((uint16_t)read6502(pc + 1) << 8);
    pc += 2;
}

static void branch16(int taken) {
    if (!taken)
        return;
    pc = (uint16_t)(pc - 1 + reladdr);
    clockticks6502++;
}

static void bpl16() { branch16(!(status & FLAG_SIGN)); }
static void bmi16() { branch16( (status & FLAG_SIGN)); }
static void bvc16() { branch16(!(status & FLAG_OVERFLOW)); }
static void bvs16() { branch16( (status & FLAG_OVERFLOW)); }
static void bcc16() { branch16(!(status & FLAG_CARRY)); }
static void bcs16() { branch16( (status & FLAG_CARRY)); }
static void bne16() { branch16(!(status & FLAG_ZERO)); }
static void beq16() { branch16( (status & FLAG_ZERO)); }
static void bra16() { branch16(1); }

static void bsr16() { //branch to subroutine, 16-bit displacement
    checkz("bsr");
    push16(pc - 1);
    branch16(1);
}

//Every opcode where the 65CE02 differs from the 65C02. Anything not listed
//here keeps its 65C02 behavior. Entries marked ce02_unimplemented use imp so
//that no operand byte is consumed and pc - 1 still names the opcode when the
//abort reports it.
static const struct {
    uint8_t opcode;
    void (*addrmode)();
    void (*handler)();
} ce02_opcodes[] = {
    {0x02, imp,   ce02_unimplemented}, //cle
    {0x03, imp,   ce02_unimplemented}, //see
    {0x0b, imp,   ce02_unimplemented}, //tsy
    {0x13, rel16, bpl16},
    {0x1b, imp,   inz},
    {0x22, indl,  jsr},
    {0x23, inax,  jsr},
    {0x2b, imp,   ce02_unimplemented}, //tys
    {0x33, rel16, bmi16},
    {0x3b, imp,   dez},
    {0x42, acc,   neg},
    {0x43, acc,   asr},
    {0x44, zp,    asr},
    {0x4b, imp,   taz},
    {0x53, rel16, bvc16},
    {0x54, zpx,   asr},
    {0x5b, imp,   ce02_unimplemented}, //tab
    {0x62, imm,   rtn},
    {0x63, rel16, bsr16},
    {0x6b, imp,   tza},
    {0x73, rel16, bvs16},
    {0x7b, imp,   ce02_unimplemented}, //tba
    {0x82, imp,   ce02_unimplemented}, //sta ($nn,s),y
    {0x83, rel16, bra16},
    {0x8b, absx,  sty},
    {0x93, rel16, bcc16},
    {0x9b, absy,  stx},
    {0xa3, imm,   ldz},
    {0xab, abso,  ldz},
    {0xb3, rel16, bcs16},
    {0xbb, absx,  ldz},
    {0xc2, imm,   cpz},
    {0xc3, zp,    dew},
    {0xcb, abso,  asw},
    {0xd3, rel16, bne16},
    {0xd4, zp,    cpz},
    {0xdb, imp,   phz},
    {0xdc, abso,  cpz},
    {0xe2, imp,   ce02_unimplemented}, //lda ($nn,s),y
    {0xe3, zp,    inw},
    {0xeb, abso,  row},
    {0xf3, rel16, beq16},
    {0xf4, immw,  phwi},
    {0xfb, imp,   plz},
    {0xfc, abso,  phw},
};

static void init_ce02_tables() {
    size_t i;

    memcpy(addrtable_ce02, addrtable_cmos, sizeof addrtable_ce02);
    memcpy(optable_ce02, optable_cmos, sizeof optable_ce02);

    for (i = 0; i < 256; i++)
        if (addrtable_ce02[i] == inzp)
            addrtable_ce02[i] = inzpz;

    for (i = 0; i < sizeof ce02_opcodes / sizeof ce02_opcodes[0]; i++) {
        addrtable_ce02[ce02_opcodes[i].opcode] = ce02_opcodes[i].addrmode;
        optable_ce02[ce02_opcodes[i].opcode] = ce02_opcodes[i].handler;
    }
}

void nmi6502() {
    push16(pc);
    push8(status);
    status |= FLAG_INTERRUPT;
    pc = (uint16_t)read6502(0xFFFA) | ((uint16_t)read6502(0xFFFB) << 8);
}

void irq6502() {
    push16(pc);
    push8(status);
    status |= FLAG_INTERRUPT;
    pc = (uint16_t)read6502(0xFFFE) | ((uint16_t)read6502(0xFFFF) << 8);
}

uint8_t callexternal = 0;
void (*loopexternal)();

void exec6502(uint32_t tickcount) {
    clockgoal6502 += tickcount;

    while (clockticks6502 < clockgoal6502) {
        opcode_pc = pc;
        opcode = read6502(pc++);
        status |= FLAG_CONSTANT;

        penaltyop = 0;
        penaltyaddr = 0;

        (*addrtable[opcode])();
        (*optable[opcode])();
        clockticks6502 += ticktable[opcode];
        if (penaltyop && penaltyaddr) clockticks6502++;

        instructions++;

        if (callexternal) (*loopexternal)();
    }

}

void reset6502(uint8_t cpu) {
    if (cpu == CPU_65CE02) {
        init_ce02_tables();
        addrtable = addrtable_ce02;
        optable = optable_ce02;
        ticktable = ticktable_cmos; //not corrected for the 65CE02; see above
    } else if (cpu == CPU_65C02) {
        addrtable = addrtable_cmos;
        optable = optable_cmos;
        ticktable = ticktable_cmos;
    } else if (cpu == CPU_6502) {
        addrtable = addrtable_nmos;
        optable = optable_nmos;
        ticktable = ticktable_nmos;
    } else {
        sim_abort("reset6502 called with an unknown CPU variant");
    }

    pc = (uint16_t)read6502(0xFFFC) | ((uint16_t)read6502(0xFFFD) << 8);
    a = 0;
    x = 0;
    y = 0;
    z = 0;
    sp = 0xFD;
    status |= FLAG_CONSTANT;
}

void step6502() {
    opcode_pc = pc;
    opcode = read6502(pc++);
    status |= FLAG_CONSTANT;

    penaltyop = 0;
    penaltyaddr = 0;

    (*addrtable[opcode])();
    (*optable[opcode])();
    clockticks6502 += ticktable[opcode];
    if (penaltyop && penaltyaddr) clockticks6502++;
    clockgoal6502 = clockticks6502;

    instructions++;

    if (callexternal) (*loopexternal)();
}

void hookexternal(void *funcptr) {
    if (funcptr != (void *)NULL) {
        loopexternal = funcptr;
        callexternal = 1;
    } else callexternal = 0;
}
