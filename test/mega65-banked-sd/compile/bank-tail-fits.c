// The counterpart to no-compile/bank-tail-no-room.c: a 16 KB bank leaves 8 KB.

#define MAPPER_BANK_1_KB 16

#include <mapper.h>
#include <stdint.h>

WINDOW_TAIL __attribute__((used)) static uint8_t tail[8 * 1024];

int main(void) { return *(volatile uint8_t *)tail; }
