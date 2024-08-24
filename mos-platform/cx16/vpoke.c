#include <stdint.h>
#include <cx16.h>

void vpoke(const uint8_t value, const uint32_t address) {
  VERA.control = 0;
  VERA.address = (uint16_t)address;
  VERA.address_hi = (uint8_t)(address >> 16);
  VERA.data0 = value;
}
