// A caller's own names reach the banked function unchanged: the macros keep
// their temporaries in the reserved namespace.

#define MAPPER_BANK_COUNT 1

#include <mapper.h>

CODE_BANK(1) static int16_t add(int16_t a, int16_t b) { return a + b; }

int main(void) {
  int16_t _a1 = 7, _result = 2;
  uint8_t _prev_bank = 3;
  banked_call_v(1, add, _result, _prev_bank);
  return banked_call_r(1, add, _a1, _result + _prev_bank);
}
