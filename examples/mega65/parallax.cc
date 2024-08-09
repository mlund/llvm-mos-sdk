// Copyright 2024 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

#include <dma.hpp>
#include <mega65.h>
#include <stdio.h>

using namespace mega65::dma;

constexpr uint32_t SCREEN_ADDR = 0x0800; // Screen area

const uint8_t sprite[] = {
    0, 90, 0, 1, 255, 128, 7, 239, 224, 15, 24, 240, 28, 0, 56, 63, 255, 28, 127, 255, 158, 127,
    255, 222, 235, 195, 215, 115, 255, 142, 227, 255, 135, 99, 199, 142, 247, 195, 223, 127, 243,
    254, 127, 241, 254, 62, 0, 60, 29, 0, 184, 15, 129, 240, 7, 255, 224, 1, 255, 128, 0, 90, 0,
};

int main(void) {
    //static const dmajob = dma::make_dma_copy(&sprite, 

//    auto sprite_ptr_addr = *(uint16_t)VICIV.spr_ptradr
    printf("%lx", (VICIV.spr_ptradr));
    VICIV.spr_ena = 0x00000001;
    VICIV.spr_exp_x = 0;
    VICIV.spr_exp_y = 0;
    VICIV.spr_pos[0].x = 0;
    VICIV.spr_pos[0].y = 80;
    VICIV.textxpos_msb = 0b00010000;
    //VICIV.textypos_msb ^= 0b11110000;
    VICIV.spr_color[0] = 2;

}
