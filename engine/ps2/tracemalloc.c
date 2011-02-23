/////////////////////////////////////////////////////////////////////////////

#define I_AM_TRACEMALLOC

#include <stdio.h>
#include <kernel.h>
#include <malloc.h>

#include "ps2port.h"

/////////////////////////////////////////////////////////////////////////////

static int *tracehead = NULL;

int tracemalloc_total = 0;

/////////////////////////////////////////////////////////////////////////////

static void tracemalloc_dump_collect(int *p, int *len, int *nalloc) {
  int name = p[2];
  *len = 0;
  *nalloc = 0;
  for(; p; p = (int*)(p[1])) {
    if(p[2] == name && p[3] > 0) {
      (*len) += p[3];
      (*nalloc) += 1;
      p[3] = -p[3];
    }
  }
}

static void tracemalloc_dump(void) {
  int totalbytes = 0;
  int *p;
  for(p = tracehead; p; p = (int*)(p[1])) {
    if(p[3] > 0) {
      const char *name = (const char*)(p[2]);
      int len = 0;
      int nalloc = 0;
      tracemalloc_dump_collect(p, &len, &nalloc);
      debug_printf("%s: %d bytes in %d allocs\n", name, len, nalloc);
      totalbytes += len;
    } 
  }
  debug_printf("total bytes %d\n", totalbytes);
}

/////////////////////////////////////////////////////////////////////////////

void *tracemalloc(const char *name, int len) {
  int *p;

  p = malloc(16 + len);

  if(!p) {
    debug_printf("out of memory!\n");
    tracemalloc_dump();
    debug_printf("tracemalloc_dump complete\n");
    SleepThread();
  }

  if(tracehead) { tracehead[0] = (int)p; }
  p[0] = 0;
  p[1] = (int)tracehead;
  p[2] = (int)name;
  p[3] = len;
  tracehead = p;

  tracemalloc_total += 16 + len;

  return (void*)(p + 4);
}

/////////////////////////////////////////////////////////////////////////////

void tracefree(void *vp) {
  int *p = ((int*)vp) - 4;
  int *p_from_prev = NULL;
  int *p_from_next = NULL;

  tracemalloc_total -= 16 + p[3];

  if(p == tracehead) {
    p_from_prev = (int*)(&tracehead);
  } else {
    p_from_prev = (int*)(p[0] + 4);
  }
  p_from_next = (int*)(p[1]);

  if(p_from_prev) *p_from_prev = p[1];
  if(p_from_next) *p_from_next = p[0];

  free(p);
}

/////////////////////////////////////////////////////////////////////////////

