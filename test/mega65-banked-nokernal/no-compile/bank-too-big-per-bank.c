// A 16 KB bank is held to 16 KB, even inside a 24 KB window.

#define MAPPER_BANK_1_KB 16

#include <mapper.h>
#include <stdint.h>

__attribute__((used, retain, section(".bank_1")))
const uint8_t too_big[17 * 1024] = {0x55};

int main(void) { return 0; }
