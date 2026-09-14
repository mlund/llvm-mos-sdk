// banked_call_r from banked code would unmap its caller mid-call, so it
// must not link.

#define MAPPER_BANK_COUNT 2

#include <mapper.h>

CODE_BANK(2) static char target(char v) { return v; }

CODE_BANK(1) char caller(void) { return banked_call_r(2, target, 1); }

int main(void) {
  banked_call(1, (void (*)(void))caller);
  return 0;
}
