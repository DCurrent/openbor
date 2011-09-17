/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

/////////////////////////////////////////////////////////////////////////////
/*#include <stdio.h>
#include <string.h>*/
#include "globals.h"
#include "types.h"
#include "sprite.h"
#include "transform.h"
/////////////////////////////////////////////////////////////////////////////


static void putsprite_(
  unsigned char *dest, int x, int *linetab, int h, int screenwidth
) {
  for(; h > 0; h--, dest += screenwidth) {
	register int lx = x;
	unsigned char *data = ((unsigned char*)linetab) + (*linetab); linetab++;
	while(lx < screenwidth) {
	  register int count = *data++;
	  if(count == 0xFF) break;
	  lx += count;
	  if(lx >= screenwidth) break;
	  count = *data++;
	  if(!count) continue;
	  if((lx + count) <= 0) { lx += count; data += count; continue; }
	  if(lx < 0) { count += lx; data -= lx; lx = 0; }
	  if((lx + count) > screenwidth) { count = screenwidth - lx; }
	  memcpy(dest+lx, data, count);
	  data+=count;lx+=count;
	}
  }
}

static void putsprite_flip_(
  unsigned char *dest, int x, int *linetab, int h, int screenwidth
) {
  for(; h > 0; h--, dest += screenwidth) {
	register int lx = x;
	unsigned char *data = ((unsigned char*)linetab) + (*linetab); linetab++;
	while(lx > 0) {
	  register int count = *data++;
	  if(count == 0xFF) break;
	  lx -= count;
	  if(lx <= 0) break;
	  count = *data++;
	  if(!count) continue;
	  if((lx - count) >= screenwidth) { lx -= count; data += count; continue; }
	  if(lx > screenwidth) { int diff = (lx - screenwidth); count -= diff; data += diff; lx = screenwidth; }
	  if((lx - count) < 0) { count = lx; }
	  //for(; count > 0; count--) dest[--lx] = *data++;
	  lx--;
	  u8revcpy(dest+lx, data, count);
	  lx-=count-1;
	  data+=count;
	}
  }
}

static void putsprite_remap_(
  unsigned char *dest, int x, int *linetab, int h, int screenwidth,
  unsigned char *remap
) {
  for(; h > 0; h--, dest += screenwidth) {
	register int lx = x;
	unsigned char *data = ((unsigned char*)linetab) + (*linetab); linetab++;
	while(lx < screenwidth) {
	  register int count = *data++;
	  if(count == 0xFF) break;
	  lx += count;
	  if(lx >= screenwidth) break;
	  count = *data++;
	  if(!count) continue;
	  if((lx + count) <= 0) { lx += count; data += count; continue; }
	  if(lx < 0) { count += lx; data -= lx; lx = 0; }
	  if((lx + count) > screenwidth) { count = screenwidth - lx; }
	  //for(; count > 0; count--) dest[lx++] = remap[((int)(*data++))&0xFF];
	  u8pcpy(dest+lx, data, remap, count);
	  lx+=count;
	  data+=count;
	}
  }
}

static void putsprite_remap_flip_(
  unsigned char *dest, int x, int *linetab, int h, int screenwidth,
  unsigned char *remap
) {
  for(; h > 0; h--, dest += screenwidth) {
	register int lx = x;
	unsigned char *data = ((unsigned char*)linetab) + (*linetab); linetab++;
	while(lx > 0) {
	  register int count = *data++;
	  if(count == 0xFF) break;
	  lx -= count;
	  if(lx <= 0) break;
	  count = *data++;
	  if(!count) continue;
	  if((lx - count) >= screenwidth) { lx -= count; data += count; continue; }
	  if(lx > screenwidth) { int diff = (lx - screenwidth); count -= diff; data += diff; lx = screenwidth; }
	  if((lx - count) < 0) { count = lx; }
	  //for(; count > 0; count--) dest[--lx] = remap[((int)(*data++))&0xFF];
	  lx--;
	  u8revpcpy(dest+lx, data, remap, count);
	  lx-=count-1;
	  data+=count;
	}
  }
}

//src high dest low
static void putsprite_remapblend_(
  unsigned char *dest, int x, int *linetab, int h, int screenwidth,
  unsigned char*remap, unsigned char *blend
) {
  for(; h > 0; h--, dest += screenwidth) {
	register int lx = x;
	unsigned char *data = ((unsigned char*)linetab) + (*linetab); linetab++;
	while(lx < screenwidth) {
	  register int count = *data++;
	  if(count == 0xFF) break;
	  lx += count;
	  if(lx >= screenwidth) break;
	  count = *data++;
	  if(!count) continue;
	  if((lx + count) <= 0) { lx += count; data += count; continue; }
	  if(lx < 0) { count += lx; data -= lx; lx = 0; }
	  if((lx + count) > screenwidth) { count = screenwidth - lx; }
	  for(; count > 0; count--) { dest[lx] = blend[(remap[(((int)(*data++))&0xFF)]<<8)|dest[lx]]; lx++; }
	}
  }
}

static void putsprite_remapblend_flip_(
  unsigned char *dest, int x, int *linetab, int h, int screenwidth,
  unsigned char* remap, unsigned char *blend
) {
  for(; h > 0; h--, dest += screenwidth) {
	register int lx = x;
	unsigned char *data = ((unsigned char*)linetab) + (*linetab); linetab++;
	while(lx > 0) {
	  register int count = *data++;
	  if(count == 0xFF) break;
	  lx -= count;
	  if(lx <= 0) break;
	  count = *data++;
	  if(!count) continue;
	  if((lx - count) >= screenwidth) { lx -= count; data += count; continue; }
	  if(lx > screenwidth) { int diff = (lx - screenwidth); count -= diff; data += diff; lx = screenwidth; }
	  if((lx - count) < 0) { count = lx; }
	  for(; count > 0; count--) { --lx; dest[lx] = blend[(remap[(((int)(*data++))&0xFF)]<<8)|dest[lx]]; }
	}
  }
}

static void putsprite_blend_(
  unsigned char *dest, int x, int *linetab, int h, int screenwidth,
  unsigned char *blend
) {
  for(; h > 0; h--, dest += screenwidth) {
	register int lx = x;
	unsigned char *data = ((unsigned char*)linetab) + (*linetab); linetab++;
	while(lx < screenwidth) {
	  register int count = *data++;
	  if(count == 0xFF) break;
	  lx += count;
	  if(lx >= screenwidth) break;
	  count = *data++;
	  if(!count) continue;
	  if((lx + count) <= 0) { lx += count; data += count; continue; }
	  if(lx < 0) { count += lx; data -= lx; lx = 0; }
	  if((lx + count) > screenwidth) { count = screenwidth - lx; }
	  for(; count > 0; count--) { dest[lx] = blend[((((int)(*data++))&0xFF)<<8)|dest[lx]]; lx++; }
	}
  }
}

static void putsprite_blend_flip_(
  unsigned char *dest, int x, int *linetab, int h, int screenwidth,
  unsigned char *blend
) {
  for(; h > 0; h--, dest += screenwidth) {
	register int lx = x;
	unsigned char *data = ((unsigned char*)linetab) + (*linetab); linetab++;
	while(lx > 0) {
	  register int count = *data++;
	  if(count == 0xFF) break;
	  lx -= count;
	  if(lx <= 0) break;
	  count = *data++;
	  if(!count) continue;
	  if((lx - count) >= screenwidth) { lx -= count; data += count; continue; }
	  if(lx > screenwidth) { int diff = (lx - screenwidth); count -= diff; data += diff; lx = screenwidth; }
	  if((lx - count) < 0) { count = lx; }
	  for(; count > 0; count--) { --lx; dest[lx] = blend[((((int)(*data++))&0xFF)<<8)|dest[lx]]; }
	}
  }
}

/////////////////////////////////////////////////////////////////////////////

void putsprite_8(
  int x, int y, int is_flip, s_sprite *sprite, s_screen *screen,
  unsigned char *remap, unsigned char *blend
) {
  int *linetab;
  int w, h;
  unsigned char *dest;
  // Get screen size
  int screenwidth = screen->width;
  int screenheight = screen->height;
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
	  if(x-w >= screenwidth) return;
	  if(x <= 0) return;
  }
  else
  {
	  if(x >= screenwidth) return;
	  if((x+w) <= 0) return;
  }
  if(y >= screenheight) return;
  if((y+h) <= 0) return;
  // Init line table pointer
  linetab = (int*)(sprite->data);
  // clip top
  if(y < 0) {
	h += y; // subtract from height
	linetab -= y; // add to linetab
	y = 0; // add to y
  }
  // clip bottom
  if((y+h) > screenheight) {
	h = screenheight - y;
  }
  // calculate destination pointer
  dest = ((unsigned char*)(screen->data)) + y*screenwidth;

  if(blend&&remap){
	if(is_flip) putsprite_remapblend_flip_(dest, x, linetab, h, screenwidth, remap, blend);
	else        putsprite_remapblend_     (dest, x  , linetab, h, screenwidth,remap, blend);
  } else if(blend) {
	if(is_flip) putsprite_blend_flip_(dest, x, linetab, h, screenwidth, blend);
	else        putsprite_blend_     (dest, x  , linetab, h, screenwidth, blend);
  } else if(remap) {
	if(is_flip) putsprite_remap_flip_(dest, x, linetab, h, screenwidth, remap);
	else        putsprite_remap_     (dest, x  , linetab, h, screenwidth, remap);
  } else {
	if(is_flip) putsprite_flip_      (dest, x, linetab, h, screenwidth);
	else        putsprite_           (dest, x  , linetab, h, screenwidth);
  }
}

/////////////////////////////////////////////////////////////////////////////

// scalex scaley flipy ...
void putsprite_ex(int x, int y, s_sprite *frame, s_screen *screen, s_drawmethod* drawmethod)
{
	int centerx, centery;
	gfx_entry gfx;

	if(!drawmethod->scalex || !drawmethod->scaley) return; // zero size

	// no scale, no shift, no flip, no fill, so use common method
	if(drawmethod->scalex==256 && drawmethod->scaley==256 && !drawmethod->flipy && !drawmethod->shiftx && drawmethod->fillcolor==TRANSPARENT_IDX && !drawmethod->rotate && !drawmethod->centerx && !drawmethod->centery)
	{
		switch(screen->pixelformat)
		{
		case PIXEL_8:
			putsprite_8(x, y, drawmethod->flipx, frame, screen, drawmethod->table, drawmethod->alpha>0?blendtables[drawmethod->alpha-1]:NULL);
			break;
		case PIXEL_16:
			putsprite_x8p16(x, y, drawmethod->flipx, frame, screen, (unsigned short*)drawmethod->table, drawmethod->alpha>0?blendfunctions16[drawmethod->alpha-1]:NULL);
			break;
		case PIXEL_32:
			putsprite_x8p32(x, y, drawmethod->flipx, frame, screen, (unsigned*)drawmethod->table, drawmethod->alpha>0?blendfunctions32[drawmethod->alpha-1]:NULL);
			break;
		}
		return;
	}

	if(drawmethod->centerx) centerx = drawmethod->centerx+frame->centerx;
	else                    centerx = frame->centerx;
	if(drawmethod->centery) centery = drawmethod->centery+frame->centery;
	else                    centery = frame->centery;

	gfx.type = gfx_sprite;
	gfx.sprite = frame;

	if(drawmethod->rotate){
		gfx_draw_rotate(screen, &gfx, x, y, centerx, centery, drawmethod);
	}else{
		gfx_draw_scale(screen, &gfx, x, y, centerx, centery, drawmethod);
	}
}

void putsprite(int x, int y, s_sprite* sprite, s_screen* screen, s_drawmethod* drawmethod)
{
	if(!drawmethod || drawmethod->flag==0)
	{
			goto plainsprite;
	}
	
	putsprite_ex(x, y, sprite, screen, drawmethod);
	return;
plainsprite:
	switch(screen->pixelformat)
	{
	case PIXEL_8:
			putsprite_8(x, y, 0, sprite, screen, NULL, NULL);
			break;
	case PIXEL_16:
			putsprite_x8p16(x, y, 0, sprite, screen, (unsigned short*)sprite->palette, NULL);
			break;
	case PIXEL_32:
			putsprite_x8p32(x, y, 0, sprite, screen, (unsigned*)sprite->palette, NULL);
			break;
	}
}



/////////////////////////////////////////////////////////////////////////////
//
// NULL for dest means do not actually encode
//
unsigned encodesprite(
  int centerx, int centery,
  s_bitmap *bitmap, s_sprite *dest
) {
  int x, x0, y, w, h;
  unsigned char *data;
  int *linetab;
  unsigned char *src = bitmap->data;
  int pb = PAL_BYTES, extrab;

  if(bitmap->width <= 0 || bitmap->height <= 0){
	// Image is empty (or bad), create an empty sprite
	if(dest) {
	  //dest->is_flip_of = NULL;
	  dest->centerx = 0;
	  dest->centery = 0;
	  dest->width = 0;
	  dest->height = 0;
	  dest->pixelformat = bitmap->pixelformat;
	  dest->mask = NULL;
	  dest->palette = NULL;
	}
	return sizeof(s_sprite);
  }

  w = bitmap->width;
  h = bitmap->height;

  if(dest) {
	//dest->is_flip_of = NULL;
	dest->centerx = centerx;
	dest->centery = centery;
	dest->width = w;
	dest->height = h;
	dest->pixelformat = bitmap->pixelformat;
	dest->mask = NULL;
  }
  linetab = (int*)(dest->data);
  data = (unsigned char*)(linetab+h);

  for(y = 0; y < h; y++, src += w) {
	if(dest) { linetab[y] = ((size_t)data)-((size_t)(linetab+y)); }
	x = 0;
	for(;;) {
	  // search for the first visible pixel
	  x0 = x;
	  for(; (x < w) && ((x-x0)<0xFE); x++) { if(src[x]) break; }
	  // handle EOL
	  if(x >= w) { if(dest) { *data = 0xFF; } data++; break; }
	  // encode clearcount
	  if(dest) { *data = x-x0; } data++;
	  // if we're still not visible, encode a null visible count and continue
	  if(!src[x]) { if(dest) { *data = 0; } data++; continue; }
	  // search for the first invisible pixel
	  x0 = x;
	  for(; (x < w) && ((x-x0)<0xFF); x++) { if(!src[x]) break; }
	  // encode viscount and visible pixels
	  if(dest) {
		*data++ = x-x0;
		memcpy(data, src+x0, x-x0);
		data += x-x0;
	  } else {
		data += 1+(x-x0);
	  }
	}
  }

  if(!bitmap->palette) pb = extrab = 0;
  else
  {
	 extrab = ((size_t)data)-((size_t)dest);
	 extrab %=4;
	 extrab = 4 - extrab;
	 extrab %=4;
  }

  //point palette to the last byte of the pixel data
  if(dest)
  {
	  if(bitmap->palette) // if the bitmap contains palette, copy it
	  {
		  dest->palette = ((unsigned char*)data) + extrab ;
		  memcpy(dest->palette, bitmap->palette, pb);
	  }
	  else dest->palette = NULL;
  }
  return ((size_t)data)-((size_t)dest)+ extrab + pb+ANYNUMBER;
}

/////////////////////////////////////////////////////////////////////////////

unsigned fakey_encodesprite(s_bitmap *bitmap) {
  return encodesprite(0, 0, bitmap, NULL);
}

/////////////////////////////////////////////////////////////////////////////

