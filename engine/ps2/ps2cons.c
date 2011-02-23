
#include "ps2port.h"

#include "gsvga.h"

unsigned char *consfont_get(int c);

static unsigned char *consoleframe = NULL;
static unsigned char consolepal[768];

static unsigned console_x = 0;
static unsigned console_y = 0;

static unsigned char *consoleframe_touch(void) {
  if(!consoleframe) {
    consoleframe = tracemalloc("consoleframe_touch", 320*240);
    if(!consoleframe) return NULL;
    memset(consoleframe, 0, 320*240);
    memset(consolepal, 0, 768);
    consolepal[3] = consolepal[4] = consolepal[5] = 0xFF;
  }
  return consoleframe;
}

static void updatescreen(void) {
//int i;
  if(!consoleframe_touch()) return;
  gsvga_setpalette(consolepal);
//for(i=0;i<320*240;i++)consoleframe[i]=(i%37)&1;
  gsvga_draw(consoleframe);
}

static void consoleframe_scrollup(void) {
  int y;
  if(!consoleframe_touch()) return;
  for(y = 0; y < 232; y++) {
    memcpy(consoleframe+320*y, consoleframe+320*(y+8), 320);
  }
  memset(consoleframe+320*232, 0, 8*320);
}

static void drawchar(unsigned x, unsigned y, unsigned c) {
  unsigned char *pat;
  unsigned ofs;
  if(!consoleframe_touch()) return;
  if(x >= 40) return;
  if(y >= 30) return;
  pat = consfont_get(c & 0xFF);
  ofs = 320 * (8*y) + (8*x);
  for(y = 0; y < 8; y++, ofs += 320-8) {
    unsigned char bits = *pat++;
    for(x = 0; x < 8; x++) {
      consoleframe[ofs++] = (bits & 0x80) ? 1 : 0;
      bits <<= 1;
    }
  }
}

static void newline(void) {
  console_x = 0;
  console_y++;
  if(console_y >= 28) {
    console_y--;
    consoleframe_scrollup();
  }
}

static void ps2cons_putc(int c) {
  c &= 0xFF;
  if(c == 10) { newline(); return; }
  if(c == 9) c = 32;
  drawchar(console_x, console_y, c);
  console_x++;
  if(console_x >= 40) newline();
}

static void ps2cons_puts(const char *s) {
  for(; *s; s++) ps2cons_putc(*s);
}

void ps2cons_printf(const char *fmt, ...) {
#ifndef PS2RELEASE
  char buf[2048];
  va_list arglist;

  va_start(arglist, fmt);
  vsprintf(buf, fmt, arglist);
  va_end(arglist);

  ps2cons_puts(buf);

  updatescreen();
#endif
}

