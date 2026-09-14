// The SD card loader links beside the KERNAL.

#define MAPPER_LOADER_SD

#include <mapper.h>

MAPPER_BANK_COUNT(1);

RODATA_BANK(1) static const unsigned char table[4] = {1, 2, 3, 4};

int main(void) { return 0; }
