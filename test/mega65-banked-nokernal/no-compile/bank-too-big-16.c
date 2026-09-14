// A 16 KB window makes every bank 16 KB, and the linker must hold it to that.

#define MAPPER_WINDOW_KB 16

#include <mapper.h>
#include <stdint.h>

__attribute__((used, retain, section(".bank_1")))
const uint8_t too_big[17 * 1024] = {0x55};

int main(void) { return 0; }
