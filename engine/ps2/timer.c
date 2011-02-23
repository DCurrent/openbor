#include "ps2port.h"

#define T0_COUNT ((u32 *)(0x10000000))
#define T0_MODE ((u32 *)(0x10000010))
#define T_MODE_CLKS_M 3
#define T_MODE_CUE_M 128

// cpu frequency (TODO: find the exact number)
#define GETTIME_FREQ (15750)

static unsigned lastinterval;

unsigned timer_gettime(void) {
  return (*T0_COUNT) & 0xFFFF;
}

void timer_init(void) {
  *T0_MODE = T_MODE_CLKS_M | T_MODE_CUE_M;
  *T0_COUNT = 0;
  lastinterval = timer_gettime();
}

void timer_exit(void) { }

unsigned timer_getinterval(unsigned freq) {
  unsigned tickspassed,ebx,blocksize;
  ebx=(timer_gettime()-lastinterval) & 0xFFFF;
  blocksize=GETTIME_FREQ/freq;
  ebx+=GETTIME_FREQ%freq;
  tickspassed=ebx/blocksize;
  ebx-=ebx%blocksize;
  lastinterval+=ebx;
  return tickspassed;
}
