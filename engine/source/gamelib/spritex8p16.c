/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

// sprite with individual 16bit palette
/////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include "globals.h"
#include "types.h"
#include "sprite.h"
/////////////////////////////////////////////////////////////////////////////

static void putsprite_(
  unsigned short *dest, int x, int xmin, int xmax, int *linetab, unsigned short *palette, int h, int screenwidth
) {
  for(; h > 0; h--, dest += screenwidth) {
	register int lx = x;
	unsigned char *data = ((unsigned char*)linetab) + (*linetab); linetab++;
	while(lx < xmax) {
	  register int count = *data++;
	  if(count == 0xFF) break;
	  lx += count;
	  if(lx >= xmax) break;
	  count = *data++;
	  if(!count) continue;
	  if((lx + count) <= xmin) { lx += count; data += count; continue; }
	  if(lx < xmin) { int diff=lx-xmin; count += diff; data -= diff; lx = xmin; }
	  if((lx + count) > xmax) { count = xmax - lx; }
	  for(; count > 0; count--)  dest[lx++] = palette[*data++];
	  //u16pcpy(dest+lx, data, palette, count);
	  //lx+=count;
	  //data+=count;
	}
  }
}

static void putsprite_flip_(
  unsigned short *dest, int x, int xmin, int xmax, int *linetab, unsigned short* palette, int h, int screenwidth
) {
  for(; h > 0; h--, dest += screenwidth) {
	register int lx = x;
	unsigned char *data = ((unsigned char*)linetab) + (*linetab); linetab++;
	while(lx > xmin) {
	  register int count = *data++;
	  if(count == 0xFF) break;
	  lx -= count;
	  if(lx <= xmin) break;
	  count = *data++;
	  if(!count) continue;
	  if((lx - count) >= xmax) { lx -= count; data += count; continue; }
	  if(lx > xmax) { int diff = (lx - xmax); count -= diff; data += diff; lx = xmax; }
	  if((lx - count) < xmin) { count = lx-xmin; }
	  for(; count > 0; count--) dest[--lx] = palette[*data++];
	  //--lx;
	  //u16revpcpy(dest+lx, data, palette, count);
	  //lx-=count-1;
	  //data+=count;
	}
  }
}


//src high dest low
static void putsprite_blend_(
  unsigned short *dest, int x, int xmin, int xmax, int *linetab, unsigned short* palette, int h, int screenwidth,
  unsigned short (*blendfp)(unsigned short, unsigned short)
) {
  for(; h > 0; h--, dest += screenwidth) {
	register int lx = x;
	unsigned char *data = ((unsigned char*)linetab) + (*linetab); linetab++;
	while(lx < xmax) {
	  register int count = *data++;
	  if(count == 0xFF) break;
	  lx += count;
	  if(lx >= xmax) break;
	  count = *data++;
	  if(!count) continue;
	  if((lx + count) <= xmin) { lx += count; data += count; continue; }
	  if(lx < xmin) { int diff=lx-xmin; count += diff; data -= diff; lx = xmin; }
	  if((lx + count) > xmax) { count = xmax - lx; }
	  for(; count > 0; count--, lx++)
	  {
		 dest[lx] = blendfp(palette[*data++], dest[lx]);
	  }
	}
  }
}

static void putsprite_blend_flip_(
  unsigned short *dest, int x, int xmin, int xmax, int *linetab, unsigned short * palette, int h, int screenwidth,
  unsigned short (*blendfp)(unsigned short, unsigned short)
) {
  for(; h > 0; h--, dest += screenwidth) {
	register int lx = x; // destination x position
	unsigned char *data = ((unsigned char*)linetab) + (*linetab); linetab++;
	while(lx > xmin) {
	  register int count = *data++; // clearcount - number of transparent pixels
	  if(count == 0xFF) break; // end-of-line indicator
	  lx -= count;
	  if(lx <= xmin) break;
	  count = *data++; // viscount - number of visible pixels following
	  if(!count) continue;
	  if((lx - count) >= xmax) { lx -= count; data += count; continue; }
	  if(lx > xmax) { int diff = (lx - xmax); count -= diff; data += diff; lx = xmax; }
	  if((lx - count) < xmin) { count = lx-xmin; }
	  for(; count > 0; count--)
	  {   --lx;
		  dest[lx] = blendfp(palette[*data++], dest[lx]);
	  }
	}
  }
}

//src high dest low
static void putsprite_mask_(
  unsigned short *dest, int x, int xmin, int xmax, int *linetab, unsigned short* palette, int h, int screenwidth,
  int *masklinetab
) {
  for(; h > 0; h--, dest += screenwidth) {
	register int lx = x;
	unsigned char *data = ((unsigned char*)linetab) + (*linetab); linetab++;
	unsigned char *maskdata = ((unsigned char*)masklinetab) + (*masklinetab); masklinetab++;
	while(lx < xmax) {
	  register int count = *data++; maskdata++;
	  if(count == 0xFF) break;
	  lx += count;
	  if(lx >= xmax) break;
	  count = *data++; maskdata++;
	  if(!count) continue;
	  if((lx + count) <= xmin) { lx += count; data += count; maskdata += count; continue; }
	  if(lx < xmin) { int diff = lx-xmin; count += diff; data -= diff; maskdata -= diff; lx = xmin; }
	  if((lx + count) > xmax) { count = xmax - lx; }
	  for(; count > 0; count--, lx++)
	  {
		 dest[lx] = blend_channel16(palette[*data++], dest[lx], *maskdata++);
	  }
	}
  }
}

static void putsprite_mask_flip_(
  unsigned short *dest, int x, int xmin, int xmax, int *linetab, unsigned short * palette, int h, int screenwidth,
  int *masklinetab
) {
  for(; h > 0; h--, dest += screenwidth) {
	register int lx = x; // destination x position
	unsigned char *data = ((unsigned char*)linetab) + (*linetab); linetab++;
	unsigned char *maskdata = ((unsigned char*)masklinetab) + (*masklinetab); masklinetab++;
	while(lx > xmin) {
	  register int count = *data++; maskdata++; // clearcount - number of transparent pixels
	  if(count == 0xFF) break; // end-of-line indicator
	  lx -= count;
	  if(lx <= xmin) break;
	  count = *data++; maskdata++; // viscount - number of visible pixels following
	  if(!count) continue;
	  if((lx - count) >= xmax) { lx -= count; data += count; maskdata += count; continue; } // not visible yet; skip this viscount block
	  if(lx > xmax) { int diff = (lx - xmax); count -= diff; data += diff; maskdata += diff; lx = xmax; }
	  if((lx - count) < xmin) { count = lx-xmin; }
	  for(; count > 0; count--)
	  {   --lx;
		  dest[lx] = blend_channel16(palette[*data++], dest[lx], *maskdata++);
	  }
	}
  }
}

/////////////////////////////////////////////////////////////////////////////

void putsprite_x8p16(
  int x, int y, int is_flip, s_sprite *sprite, s_screen *screen,
  unsigned short *remap, blend16fp blend
) {
  int *linetab, *masklinetab = NULL;
  int w, h;
  unsigned short *dest;
  unsigned short * m;
  // Get screen size
  int screenwidth = screen->width;
  int xmin=useclip?MAX(clipx1,0):0,
	  xmax=useclip?MIN(clipx2,screen->width):screen->width,
	  ymin=useclip?MAX(clipy1,0):0,
	  ymax=useclip?MIN(clipy2,screen->height):screen->height;
  // Adjust coords for centering
  if(is_flip) x += sprite->centerx;
  else x -= sprite->centerx;
  y -= sprite->centery;
  // Get sprite dimensions
  w = sprite->width;
  h = sprite->height;
  // trivial clip all directions
  if(is_flip)
  {
	  if(x-w >= xmax) return;
	  if(x <= xmin) return;
  }
  else
  {
	  if(x >= xmax) return;
	  if((x+w) <= xmin) return;
  }
  if(y >= ymax) return;
  if((y+h) <= ymin) return;
  // Init line table pointer
  linetab = (int*)(sprite->data);
  if(sprite->mask) {
	masklinetab = (int*)(sprite->mask->data);
	//if(w!=sprite->mask->width) { printf("Wrong mask width. %i %i\n", w, sprite->mask->width); return; }
	//if(h!=sprite->mask->height) { printf("Wrong mask height. %i %i\n", h, sprite->mask->height); return; }
  }
  // clip top
  if(y < ymin) {
	int diff = y-ymin;
	h += diff; // subtract from height
	linetab -= diff; // add to linetab
	masklinetab -= diff;
	y = ymin; // add to y
  }
  // clip bottom
  if((y+h) > ymax) {
	h = ymax - y;
  }
  // calculate destination pointer
  dest = ((unsigned short*)(screen->data)) + y*screenwidth;
  if(remap) m = remap;
  else      m = (unsigned short*)sprite->palette;
  if(sprite->mask) {
	if(is_flip) putsprite_mask_flip_ (dest, x, xmin, xmax, linetab, m, h, screenwidth, masklinetab);
	else        putsprite_mask_      (dest, x, xmin, xmax, linetab, m, h, screenwidth, masklinetab);
  } else if(blend) {
	if(is_flip) putsprite_blend_flip_(dest, x, xmin, xmax, linetab, m , h, screenwidth, blend);
	else        putsprite_blend_     (dest, x, xmin, xmax, linetab, m , h, screenwidth, blend);
  } else {
	if(is_flip) putsprite_flip_      (dest, x, xmin, xmax, linetab, m , h, screenwidth);
	else        putsprite_           (dest, x, xmin, xmax, linetab, m , h, screenwidth);
  }
}

/////////////////////////////////////////////////////////////////////////////


