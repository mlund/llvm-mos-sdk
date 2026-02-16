// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// Banked example: each bank prints a greeting via printf().
//
// printf() lives in the fixed region ($8000-$BFFF), so it's callable
// from any bank. The string literals are placed in each bank's section
// and are only accessible while that bank is mapped.
//
// Border color changes track execution progress:
//   red(2) -> bank 1, green(5) -> bank 2, yellow(7) -> bank 4.
// Bank 4 uses fast RAM ($40000), avoiding the colour RAM window at
// $1F800-$1FFFF that overlaps chip RAM bank 3 ($1A000-$1FFFF).

#include <cstdio>
#include <mapper.h>

#define BORDERCOLOR (*(volatile unsigned char *)0xD020)

__attribute__((noinline, section(".bank_1")))
void hello_bank_1() {
  BORDERCOLOR = 2;  // red border = entered bank 1
  printf("Hello from bank 1!\n");
}

__attribute__((noinline, section(".bank_2")))
void hello_bank_2() {
  BORDERCOLOR = 5;  // green border = entered bank 2
  printf("Hello from bank 2!\n");
}

__attribute__((noinline, section(".bank_4")))
void hello_bank_4() {
  BORDERCOLOR = 7;  // yellow border = entered bank 4
  printf("Hello from bank 4!\n");
}

int main() {
  printf("Hello from main!\n");

  banked_call(1, hello_bank_1);
  banked_call(2, hello_bank_2);
  banked_call(4, hello_bank_4);

  for (;;) ;
}
