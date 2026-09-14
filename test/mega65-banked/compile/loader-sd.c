// The SD card loader links beside the KERNAL.

#define MAPPER_LOADER_SD
#define MAPPER_BANK_COUNT 1

#include <mapper.h>

RODATA_BANK(1) static const uint8_t table[4] = {1, 2, 3, 4};

int main(void) { return 0; }
