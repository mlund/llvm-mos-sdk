// WINDOW_TAIL needs a bank smaller than the window, or there is no tail.

#include <mapper.h>
#include <stdint.h>

WINDOW_TAIL __attribute__((used)) static uint8_t tail[1];

int main(void) { return *(volatile uint8_t *)tail; }
