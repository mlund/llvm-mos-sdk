// The counterpart to no-compile/bank-too-big-16.c: 15 KB fits a 16 KB bank.

#define MAPPER_WINDOW_KB 16

#include <mapper.h>
#include <stdint.h>

__attribute__((used, retain, section(".bank_1")))
const uint8_t fits[15 * 1024] = {0x55};

int main(void) { return 0; }
