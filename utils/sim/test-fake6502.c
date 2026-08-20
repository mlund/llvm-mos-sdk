/*
Copyright 2021 LLVM-MOS Project
Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
information.
*/

/*
 * Tests for the emulator core, driven through fake6502.h rather than by running
 * a program: these turn on where a byte sits in memory, or on opcodes the
 * compiler does not emit, neither of which a test can ask the compiler for.
 *
 * Indirect JMP: which fetches wrap within a page. JMP (abs) on the NMOS 6502
 * does; JMP (abs,X) does not, being a 65C02 addition. A pointer at $xxff is the
 * only place the two differ, so that is where those checks sit.
 *
 * Indexed stores: STX and STY take the index from the register the other
 * instruction stores, so the checks pin which register does which.
 *
 * Word operations: ASW, ROW, INW and DEW take N and Z from all sixteen bits,
 * which is where they part company with their 8-bit counterparts.
 *
 * Z: that ($nn) is really ($nn),Z, that it defaults to zero so compiled code is
 * unaffected, and that a transfer of control with Z set is refused.
 */

#include "fake6502.h"

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t memory[65536];
static int failures;

uint8_t read6502(uint16_t address) { return memory[address]; }
void write6502(uint16_t address, uint8_t value) { memory[address] = value; }

/* Set when a check expects an abort; sim_abort() then longjmps back instead of
 * ending the run, so a check can assert that the emulator refused. */
static jmp_buf abort_jmp;
static int expect_abort;

void sim_abort(const char *msg) {
    if (expect_abort)
        longjmp(abort_jmp, 1);
    fprintf(stderr, "unexpected sim_abort: %s\n", msg);
    exit(2);
}

static void check(const char *what, uint16_t got, uint16_t want) {
    if (got != want) {
        fprintf(stderr, "FAIL %-46s got $%04x, want $%04x\n", what, got, want);
        ++failures;
    } else {
        printf("ok   %-46s $%04x\n", what, got);
    }
}

/* Run one JMP (abs,X): table base at `base`, index `index`, and the pointer
 * written at base+index reading `target`. Returns where the CPU went. */
static uint16_t jmp_indirect_x(uint8_t cpu, uint16_t base, uint8_t index,
                               uint16_t target) {
    memset(memory, 0, sizeof memory);
    uint16_t entry = (uint16_t)(base + index);
    memory[entry] = (uint8_t)(target & 0xFF);
    memory[(uint16_t)(entry + 1)] = (uint8_t)(target >> 8);

    memory[0x0800] = 0x7C; // jmp (abs,x)
    memory[0x0801] = (uint8_t)(base & 0xFF);
    memory[0x0802] = (uint8_t)(base >> 8);
    memory[0xFFFC] = 0x00; // reset vector -> $0800
    memory[0xFFFD] = 0x08;

    reset6502(cpu);
    x = index;
    step6502();
    return pc;
}

/* Run one JMP (abs) whose pointer sits at `ptr`, reading $08e0 linearly and
 * $02e0 if the high byte is taken with a page wraparound. */
static uint16_t jmp_indirect(uint8_t cpu, uint16_t ptr) {
    memset(memory, 0, sizeof memory);
    memory[ptr] = 0xE0;
    memory[(uint16_t)(ptr + 1)] = 0x08;              // linear high byte
    memory[(ptr & 0xFF00) | ((ptr + 1) & 0x00FF)] = 0x02; // wrapped high byte

    memory[0x0800] = 0x6C; // jmp (abs)
    memory[0x0801] = (uint8_t)(ptr & 0xFF);
    memory[0x0802] = (uint8_t)(ptr >> 8);
    memory[0xFFFC] = 0x00;
    memory[0xFFFD] = 0x08;

    reset6502(cpu);
    step6502();
    return pc;
}

/* Clear memory, load `code` at $0800, point the reset vector there and reset.
 * Seed any further memory or registers after this returns. */
static void load_ce02(const uint8_t *code, size_t len) {
    memset(memory, 0, sizeof memory);
    memcpy(&memory[0x0800], code, len);
    memory[0xFFFC] = 0x00;
    memory[0xFFFD] = 0x08;
    reset6502(CPU_65CE02);
}

static uint16_t word_at(uint16_t addr) {
    return (uint16_t)(memory[addr] | (memory[addr + 1] << 8));
}

/* Step once expecting the emulator to refuse, and report whether it did. */
static int step_aborts(void) {
    int refused;

    expect_abort = 1;
    if (!(refused = setjmp(abort_jmp)))
        step6502();
    expect_abort = 0;
    return refused;
}

/* Run one indexed store with both index registers loaded, and return the byte
 * left at `probe`. Loading both is what lets a check distinguish which register
 * the instruction used as its index. */
static uint8_t indexed_store(uint8_t opcode, uint16_t base, uint8_t xval,
                             uint8_t yval, uint16_t probe) {
    const uint8_t code[] = {opcode, (uint8_t)(base & 0xFF), (uint8_t)(base >> 8)};

    load_ce02(code, sizeof code);
    x = xval;
    y = yval;
    step6502();
    return memory[probe];
}

int main(void) {
    /* Entirely within one page: the wrapping and linear fetches agree here, so
     * this only guards against breaking the ordinary case. */
    check("65C02 jmp ($1200,x) x=2  [same page]",
          jmp_indirect_x(CPU_65C02, 0x1200, 2, 0x08e0), 0x08e0);

    /* Pointer at $xxfe: last position that still fits in the page. */
    check("65C02 jmp ($12fc,x) x=2  [pointer at $12fe]",
          jmp_indirect_x(CPU_65C02, 0x12fc, 2, 0x08e0), 0x08e0);

    /* The discriminating case. The pointer sits at $11ff, so its high byte is
     * at $1200; a wrapping fetch would take it from $1100. */
    check("65C02 jmp ($11fd,x) x=2  [pointer straddles page]",
          jmp_indirect_x(CPU_65C02, 0x11fd, 2, 0x08e0), 0x08e0);
    check("65CE02 jmp ($11fd,x) x=2 [pointer straddles page]",
          jmp_indirect_x(CPU_65CE02, 0x11fd, 2, 0x08e0), 0x08e0);

    /* Same straddle reached with a large index rather than a large base, since
     * the sum is what matters. */
    check("65C02 jmp ($1100,x) x=255 [pointer straddles page]",
          jmp_indirect_x(CPU_65C02, 0x1100, 0xFF, 0x08e0), 0x08e0);

    /* At the very top of memory the fetch does wrap, but around the whole
     * address space: the high byte comes from $0000, not from $ff00. */
    memset(memory, 0, sizeof memory);
    memory[0xFFFF] = 0xE0;
    memory[0x0000] = 0x08;
    memory[0x0800] = 0x7C;
    memory[0x0801] = 0xFD;
    memory[0x0802] = 0xFF;
    memory[0xFFFC] = 0x00;
    memory[0xFFFD] = 0x08;
    reset6502(CPU_65C02);
    x = 2;
    step6502();
    check("65C02 jmp ($fffd,x) x=2  [wraps at $ffff]", pc, 0x08e0);

    /* JMP (abs) is the other half of the distinction: the NMOS wraparound is
     * real hardware behaviour and must stay, while the 65C02 fixed it. */
    check("6502 jmp ($11ff)   [NMOS wraps]", jmp_indirect(CPU_6502, 0x11ff), 0x02e0);
    check("65C02 jmp ($11ff)  [CMOS linear]", jmp_indirect(CPU_65C02, 0x11ff), 0x08e0);
    check("65CE02 jmp ($11ff) [CMOS linear]", jmp_indirect(CPU_65CE02, 0x11ff), 0x08e0);

    /* STY $nnnn,X and STX $nnnn,Y. Both index registers hold a distinct value,
     * so swapping the two addressing modes lands the byte somewhere else. */
    check("65CE02 sty $1234,x  x=3 y=$5a",
          indexed_store(0x8b, 0x1234, 3, 0x5a, 0x1237), 0x5a);
    check("65CE02 stx $1234,y  y=3 x=$5a",
          indexed_store(0x9b, 0x1234, 0x5a, 3, 0x1237), 0x5a);

    /* Nothing lands at the unindexed address, which a zero index would hide. */
    check("65CE02 sty $1234,x  leaves $1234 alone",
          indexed_store(0x8b, 0x1234, 3, 0x5a, 0x1234), 0x00);

    /* The index is added across the full 16 bits, not within the page. */
    check("65CE02 sty $12fe,x  x=3 [crosses page]",
          indexed_store(0x8b, 0x12fe, 3, 0x5a, 0x1301), 0x5a);
    check("65CE02 stx $12fe,y  y=3 [crosses page]",
          indexed_store(0x9b, 0x12fe, 0x5a, 3, 0x1301), 0x5a);

    /* ASW $1234. The worked example from the MEGA65 user guide: $1234 holding
     * $87 and $1235 holding $a9 becomes $0e / $53 with carry set. */
    {
        const uint8_t asw[] = {0xcb, 0x34, 0x12};
        load_ce02(asw, sizeof asw);
        memory[0x1234] = 0x87;
        memory[0x1235] = 0xa9;
        step6502();
        check("65CE02 asw $1234 [user guide example]",
              word_at(0x1234), 0x530e);
        check("65CE02 asw $1234 carry out", status & 0x01, 0x01);
    }

    /* Flags come from the whole word: a result with the low byte zero must not
     * set Z, and one with bit 15 set must set N. */
    {
        const uint8_t asw[] = {0xcb, 0x00, 0x20};
        load_ce02(asw, sizeof asw);
        memory[0x2000] = 0x80;
        memory[0x2001] = 0x40; // $4080 << 1 = $8100
        step6502();
        check("65CE02 asw sets N from bit 15", status & 0x80, 0x80);
        check("65CE02 asw leaves Z clear when only the low byte is zero",
              status & 0x02, 0x00);
    }

    /* ROW rotates left through carry: bit 0 takes the incoming carry, bit 15
     * becomes the new carry. */
    {
        const uint8_t row[] = {0xeb, 0x00, 0x20};

        load_ce02(row, sizeof row);
        status = 0x21; // carry set
        step6502();
        check("65CE02 row $2000 shifts carry into bit 0", word_at(0x2000), 0x0001);

        load_ce02(row, sizeof row);
        status = 0x20; // carry clear
        step6502();
        check("65CE02 row $2000 [carry clear] leaves bit 0 clear",
              word_at(0x2000), 0x0000);
    }

    /* JSR ($nnnn) and JSR ($nnnn,X) take their target through the pointer, and
     * push the address of the instruction's last byte as any JSR does. */
    {
        const uint8_t code[] = {0x22, 0x00, 0x20};
        load_ce02(code, sizeof code);
        memory[0x2000] = 0x34; memory[0x2001] = 0x12;
        step6502();
        check("65CE02 jsr ($2000)", pc, 0x1234);
        check("65CE02 jsr ($2000) pushed return",
              word_at(0x01FC), 0x0802);
    }
    {
        const uint8_t code[] = {0x23, 0x00, 0x20};
        load_ce02(code, sizeof code);
        memory[0x2004] = 0x34; memory[0x2005] = 0x12;
        x = 4;
        step6502();
        check("65CE02 jsr ($2000,x) x=4", pc, 0x1234);
    }

    /* RTS #$nn returns and additionally drops the caller's pushed arguments. */
    {
        const uint8_t code[] = {0x62, 0x03};
        load_ce02(code, sizeof code);
        memory[0x01FE] = 0x02; memory[0x01FF] = 0x08; // return address $0802
        step6502();
        check("65CE02 rts #3 returns past the pushed word", pc, 0x0803);
        check("65CE02 rts #3 drops 3 argument bytes", sp, 0x02);
    }

    /* PHW pushes the low byte first, the opposite order to JSR. */
    {
        const uint8_t code[] = {0xf4, 0x34, 0x12};
        load_ce02(code, sizeof code);
        step6502();
        check("65CE02 phw #$1234 pushes low byte first", memory[0x01FD], 0x34);
        check("65CE02 phw #$1234 then high", memory[0x01FC], 0x12);
    }
    {
        const uint8_t code[] = {0xfc, 0x00, 0x20};
        load_ce02(code, sizeof code);
        memory[0x2000] = 0x34; memory[0x2001] = 0x12;
        step6502();
        check("65CE02 phw $2000 pushes the word held there",
              (uint16_t)(memory[0x01FD] | (memory[0x01FC] << 8)) /* reversed: PHW pushes low first */, 0x1234);
    }


    /* LDZ, and the point of emulating Z at all: ($nn) is ($nn),Z. With Z set,
     * an LDA ($10) must read one byte further on. */
    {
        const uint8_t code[] = {0xa3, 0x01, 0xb2, 0x10};
        load_ce02(code, sizeof code);
        memory[0x0010] = 0x00; memory[0x0011] = 0x30; // pointer -> $3000
        memory[0x3000] = 0x11; memory[0x3001] = 0x22;
        step6502();
        check("65CE02 ldz #1", z, 0x01);
        step6502();
        check("65CE02 lda ($10) is ($10),z", a, 0x22);
    }

    /* Z defaults to zero, so the same code without an LDZ reads the pointer
     * itself: this is what the compiler relies on. */
    {
        const uint8_t code[] = {0xb2, 0x10};
        load_ce02(code, sizeof code);
        memory[0x0010] = 0x00; memory[0x0011] = 0x30;
        memory[0x3000] = 0x11; memory[0x3001] = 0x22;
        step6502();
        check("65CE02 lda ($10) with z=0", a, 0x11);
    }

    /* TAZ/TZA/INZ/DEZ/PHZ/PLZ move Z about and set N and Z from the result. */
    {
        const uint8_t code[] = {0xa9, 0x7f, 0x4b, 0x1b, 0x6b};
        load_ce02(code, sizeof code);
        step6502(); step6502();
        check("65CE02 taz", z, 0x7f);
        step6502();
        check("65CE02 inz", z, 0x80);
        check("65CE02 inz sets N", status & 0x80, 0x80);
        step6502();
        check("65CE02 tza", a, 0x80);
    }
    {
        const uint8_t code[] = {0xa3, 0x5a, 0xdb, 0xa3, 0x00, 0xfb};
        load_ce02(code, sizeof code);
        step6502(); step6502();
        check("65CE02 phz", memory[0x01FD], 0x5a);
        step6502(); step6502();
        check("65CE02 plz", z, 0x5a);
    }

    /* CPZ compares without disturbing Z. */
    {
        const uint8_t code[] = {0xa3, 0x40, 0xc2, 0x40};
        load_ce02(code, sizeof code);
        step6502(); step6502();
        check("65CE02 cpz equal sets Z and C", status & 0x03, 0x03);
        check("65CE02 cpz leaves Z register alone", z, 0x40);
    }

    /* The boundary check: llvm-mos requires Z to be zero wherever control
     * reaches compiled code, so a return with Z set is refused. */
    {
        const uint8_t code[] = {0xa3, 0x01, 0x60};
        load_ce02(code, sizeof code);
        memory[0x01FE] = 0x02; memory[0x01FF] = 0x08;
        step6502();
        check("65CE02 rts with z set aborts", (uint16_t)step_aborts(), 1);
    }

    /* BSR is the 65CE02's own call instruction, so the guard covers it as well
     * as JSR. */
    {
        const uint8_t code[] = {0xa3, 0x01, 0x63, 0x10, 0x00};
        load_ce02(code, sizeof code);
        step6502();
        check("65CE02 bsr with z set aborts", (uint16_t)step_aborts(), 1);
    }

    /* --keep-z is the way out for assembly that keeps Z live across a call. */
    {
        const uint8_t code[] = {0xa3, 0x01, 0x60};
        load_ce02(code, sizeof code);
        memory[0x01FE] = 0x02; memory[0x01FF] = 0x08;
        fake6502_check_z = 0;
        step6502(); step6502();
        fake6502_check_z = 1;
        check("65CE02 rts with z set allowed by --keep-z", pc, 0x0803);
    }

    if (failures) {
        fprintf(stderr, "\n%d check(s) failed\n", failures);
        return 1;
    }
    printf("\nall checks passed\n");
    return 0;
}
