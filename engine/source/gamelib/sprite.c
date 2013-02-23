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
  unsigned char *dest, int x, int xmin, int xmax, int *linetab, int h, int screenwidth
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
	  memcpy(dest+lx, data, count);
	  data+=count;lx+=count;
	}
  }
}

static void putsprite_flip_(
  unsigned char *dest, int x, int xmin, int xmax, int *linetab, int h, int screenwidth
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
	  for(; count > 0; count--) dest[--lx] = *data++;
	  //lx--;
	  //u8revcpy(dest+lx, data, count);
	  //lx-=count-1;
	  //data+=count;
	}
  }
}

static void putsprite_remap_(
  unsigned char *dest, int x, int xmin, int xmax, int *linetab, int h, int screenwidth,
  unsigned char *remap
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
	  for(; count > 0; count--) dest[lx++] = remap[((int)(*data++))&0xFF];
	  //u8pcpy(dest+lx, data, remap, count);
	  //lx+=count;
	  //data+=count;
	}
  }
}

static void putsprite_remap_flip_(
  unsigned char *dest, int x, int xmin, int xmax, int *linetab, int h, int screenwidth,
  unsigned char *remap
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
	  for(; count > 0; count--) dest[--lx] = remap[((int)(*data++))&0xFF];
	  //lx--;
	  //u8revpcpy(dest+lx, data, remap, count);
	  //lx-=count-1;
	  //data+=count;
	}
  }
}

//src high dest low
static void putsprite_remapblend_(
  unsigned char *dest, int x, int xmin, int xmax, int *linetab, int h, int screenwidth,
  unsigned char*remap, unsigned char *blend
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
	  if(lx < xmin) {int diff=lx-xmin; count += diff; data -= diff; lx = xmin; }
	  if((lx + count) > xmax) { count = xmax - lx; }
	  for(; count > 0; count--) { dest[lx] = blend[(remap[(((int)(*data++))&0xFF)]<<8)|dest[lx]]; lx++; }
	}
  }
}

static void putsprite_remapblend_flip_(
  unsigned char *dest, int x, int xmin, int xmax, int *linetab, int h, int screenwidth,
  unsigned char* remap, unsigned char *blend
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
	  for(; count > 0; count--) { --lx; dest[lx] = blend[(remap[(((int)(*data++))&0xFF)]<<8)|dest[lx]]; }
	}
  }
}

static void putsprite_blend_(
  unsigned char *dest, int x, int xmin, int xmax, int *linetab, int h, int screenwidth,
  unsigned char *blend
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
	  if(lx < xmin) {int diff=lx-xmin; count += diff; data -= diff; lx = xmin; }
	  if((lx + count) > xmax) { count = xmax - lx; }
	  for(; count > 0; count--) { dest[lx] = blend[((((int)(*data++))&0xFF)<<8)|dest[lx]]; lx++; }
	}
  }
}

static void putsprite_blend_flip_(
  unsigned char *dest, int x, int xmin, int xmax, int *linetab, int h, int screenwidth,
  unsigned char *blend
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
	  if(lx > xmax) { int diff = (lx - screenwidth); count -= diff; data += diff; lx = screenwidth; }
	  if((lx - count) < xmin) { count = lx-xmin; }
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
  // clip top
  if(y < ymin) {
	h += y-ymin; // subtract from height
	linetab -= y-ymin; // add to linetab
	y = ymin; // add to y
  }
  // clip bottom
  if((y+h) > ymax) {
	h = ymax - y;
  }
  // calculate destination pointer
  dest = ((unsigned char*)(screen->data)) + y*screenwidth;

  if(blend&&remap){
	if(is_flip) putsprite_remapblend_flip_(dest, x, xmin, xmax, linetab, h, screenwidth, remap, blend);
	else        putsprite_remapblend_     (dest, x, xmin, xmax  , linetab, h, screenwidth,remap, blend);
  } else if(blend) {
	if(is_flip) putsprite_blend_flip_(dest, x, xmin, xmax, linetab, h, screenwidth, blend);
	else        putsprite_blend_     (dest, x, xmin, xmax  , linetab, h, screenwidth, blend);
  } else if(remap) {
	if(is_flip) putsprite_remap_flip_(dest, x, xmin, xmax, linetab, h, screenwidth, remap);
	else        putsprite_remap_     (dest, x, xmin, xmax  , linetab, h, screenwidth, remap);
  } else {
	if(is_flip) putsprite_flip_      (dest, x, xmin, xmax, linetab, h, screenwidth);
	else        putsprite_           (dest, x, xmin, xmax  , linetab, h, screenwidth);
  }
}

/////////////////////////////////////////////////////////////////////////////

// scalex scaley flipy ...
void putsprite_ex(int x, int y, s_sprite *frame, s_screen *screen, s_drawmethod* drawmethod)
{
	gfx_entry gfx;

	if(!drawmethod->scalex || !drawmethod->scaley) return; // zero size

	// no scale, no shift, no flip, no fill, so use common method
	if(!drawmethod->water.watermode && drawmethod->scalex==256 && drawmethod->scaley==256 && !drawmethod->flipy && !drawmethod->shiftx && drawmethod->fillcolor==TRANSPARENT_IDX && !drawmethod->rotate)
	{
		if(drawmethod->flipx) x += drawmethod->centerx;
		else x -= drawmethod->centerx;
		y -= drawmethod->centery;
		switch(screen->pixelformat)
		{
		case PIXEL_8:
			putsprite_8(x, y, drawmethod->flipx, frame, screen, drawmethod->table, drawmethod->alpha>0?blendtables[drawmethod->alpha-1]:NULL);
			break;
		case PIXEL_16:
			putsprite_x8p16(x, y, drawmethod->flipx, frame, screen, (unsigned short*)drawmethod->table, getblendfunction16(drawmethod->alpha));
			break;
		case PIXEL_32:
			putsprite_x8p32(x, y, drawmethod->flipx, frame, screen, (unsigned*)drawmethod->table, getblendfunction32(drawmethod->alpha));
			break;
		}
		return;
	}

	gfx.sprite = frame;

	if(drawmethod->water.watermode==3 && drawmethod->water.beginsize>0){
		gfx_draw_plane(screen, &gfx, x, y, frame->centerx, frame->centery, drawmethod);
	}else if(drawmethod->water.watermode && drawmethod->water.amplitude){
		gfx_draw_water(screen, &gfx, x, y, frame->centerx, frame->centery, drawmethod);
	}else if(drawmethod->rotate){
		gfx_draw_rotate(screen, &gfx, x, y, frame->centerx, frame->centery, drawmethod);
	}else{
		gfx_draw_scale(screen, &gfx, x, y, frame->centerx, frame->centery, drawmethod);
	}
}

static void _putsprite(int x, int y, s_sprite* sprite, s_screen* screen, s_drawmethod* drawmethod)
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

void putsprite(int x, int y, s_sprite* sprite, s_screen* screen, s_drawmethod* drawmethod){
	int xrepeat, yrepeat, xspan, yspan, i, j, dx,dy;

	drawmethod_global_init(drawmethod);

	if(drawmethod && drawmethod->flag){
		xrepeat = drawmethod->xrepeat;
		yrepeat = drawmethod->yrepeat;
		xspan = drawmethod->xspan;
		yspan = drawmethod->yspan;
	} else {
		xrepeat = yrepeat = 1;
		xspan = yspan = 0;
	}

	for(j=0, dy=y; j<yrepeat; j++, dy+=yspan){
		for(i=0, dx=x; i<xrepeat; i++, dx+=xspan){
			_putsprite(dx, dy, sprite, screen, drawmethod);
		}
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

  if(dest) dest->magic = sprite_magic;

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

