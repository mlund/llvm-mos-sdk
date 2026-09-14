// A caller's own names reach the banked function unchanged: the macros keep
// their temporaries in the reserved namespace.

#include <mapper.h>

MAPPER_BANK_COUNT(1);

CODE_BANK(1) static int add(int a, int b) { return a + b; }

int main(void) {
  int _a1 = 7, _result = 2;
  char _prev_bank = 3;
  banked_call_v(1, add, _result, _prev_bank);
  return banked_call_r(1, add, _a1, _result + _prev_bank);
}
