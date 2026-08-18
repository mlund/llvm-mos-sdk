// The counterpart to no-compile/bank-too-big.c: a bank holds 24 KB, so 23 KB
// of it must link. Without this, a diagnostic that fired on everything would
// still pass the negative test.

#include <stdint.h>

__attribute__((used, retain, section(".bank_1")))
const uint8_t fits[23 * 1024] = {0x55};

int main(void) { return 0; }
