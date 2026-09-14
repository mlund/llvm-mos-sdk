// MAPPER_BANK_COUNT keeps the undeclared banks out of the image: one slot,
// not thirty-one.

#define MAPPER_BANK_COUNT 1

#include <mapper.h>
#include <stdint.h>

RODATA_BANK(1) static const uint8_t payload[16] = {0};

int main(void) { return 0; }
