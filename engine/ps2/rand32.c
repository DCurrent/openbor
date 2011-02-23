#include "ps2port.h"

static unsigned int seed = 1234567890;

unsigned int rand32(void) {
  unsigned long long t = seed;
  t *= 1103515245ull;
  t += 12345ull;
  seed = t;
  return (t >> 16) & 0xFFFFFFFF;
}

void srand32(int n) { seed = n; }

