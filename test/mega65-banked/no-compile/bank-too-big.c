// A bank section larger than the 24 KB window must be diagnosed rather than
// spilling into the next bank's load address, where it would be written to
// the wrong file on the D81 and quietly appear as corruption in bank 2.
//
// ExpectFailure: ld.lld: error: section '.bank_1' will not fit in region 'bank_1': overflowed

#include <stdint.h>

__attribute__((used, retain, section(".bank_1")))
const uint8_t too_big[25 * 1024] = {0x55};

int main(void) { return 0; }
