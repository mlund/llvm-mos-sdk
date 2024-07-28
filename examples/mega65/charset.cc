#include <charset.h>
#include <stdio.h>

int main() {
  printf("%s %s\n", U"UNSHIFTED↑"_u, U"SHIFTED"_s);
}
