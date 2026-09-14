// The counterpart to no-compile/banked-call-r-in-bank.c: from fixed code it
// links.

#include <mapper.h>

MAPPER_BANK_COUNT(2);

CODE_BANK(2) static char target(char v) { return v; }

char caller(void) { return banked_call_r(2, target, 1); }

int main(void) {
  banked_call(1, (void (*)(void))caller);
  return 0;
}
