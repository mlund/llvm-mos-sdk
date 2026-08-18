// A program that declares one bank must not pay for fifteen.
//
// Each bank is a MEMORY region emitted by OUTPUT_FORMAT whether or not
// anything was linked into it, so with every region declared unconditionally
// the smallest possible program is the size of the largest possible one --
// around 414 KB, nearly all of it zeroes, all of it read off the D81 at boot.
//
// Every other banked target in the SDK gates each region's LENGTH on a size
// symbol so unused banks collapse to nothing; this asks for the same here.

#include <mapper.h>
#include <stdint.h>

MAPPER_BANK_COUNT(1);

__attribute__((used, retain, section(".bank_1")))
const uint8_t bank_1_payload[16] = {0xB1};

int main(void) { return 0; }
