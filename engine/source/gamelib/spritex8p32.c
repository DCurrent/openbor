/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

// sprite with individual 32bit palette
/////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "sprite.h"
/////////////////////////////////////////////////////////////////////////////


static void putsprite_(
  unsigned *dest, int x, int *linetab, unsigned *palette, int h, int screenwidth
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
	  //for(; count > 0; count--)  dest[lx++] = palette[*data++];
	  u32pcpy(dest+lx, data, palette, count);
	  lx+=count;
	  data+=count;
	}
  }
}

static void putsprite_flip_(
  unsigned *dest, int x, int *linetab, unsigned* palette, int h, int screenwidth
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
	  //for(; count > 0; count--) dest[--lx] = palette[*data++];
	  --lx;
	  u32revpcpy(dest+lx, data, palette, count);
	  lx-=count-1;
	  data+=count;
	}
  }
}


//src high dest low
static void putsprite_blend_(
  unsigned *dest, int x, int *linetab, unsigned* palette, int h, int screenwidth,
  unsigned (*blendfp)(unsigned, unsigned)
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
	  for(; count > 0; count--, lx++)
	  {
		 dest[lx] = blendfp(palette[*data++], dest[lx]);
	  }
	}
  }
}

static void putsprite_blend_flip_(
  unsigned *dest, int x, int *linetab, unsigned * palette, int h, int screenwidth,
  unsigned (*blendfp)(unsigned, unsigned)
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
	  for(; count > 0; count--)
	  {   --lx;
		  dest[lx] = blendfp(palette[*data++], dest[lx]);
	  }
	}
  }
}

//src high dest low
static void putsprite_mask_(
  unsigned *dest, int x, int *linetab, unsigned* palette, int h, int screenwidth,
  int* masklinetab
) {
  for(; h > 0; h--, dest += screenwidth) {
	register int lx = x;
	unsigned char *data = ((unsigned char*)linetab) + (*linetab); linetab++;
	unsigned char *maskdata = ((unsigned char*)masklinetab) + (*masklinetab); masklinetab++;
	while(lx < screenwidth) {
	  register int count = *data++; maskdata++;
	  if(count == 0xFF) break;
	  lx += count;
	  if(lx >= screenwidth) break;
	  count = *data++; maskdata++;
	  if(!count) continue;
	  if((lx + count) <= 0) { lx += count; data += count; maskdata += count; continue; }
	  if(lx < 0) { count += lx; data -= lx; maskdata -= lx; lx = 0; }
	  if((lx + count) > screenwidth) { count = screenwidth - lx; }
	  for(; count > 0; count--, lx++)
	  {
		 dest[lx] = blend_channel32(palette[*data++], dest[lx], *maskdata++);
	  }
	}
  }
}

static void putsprite_mask_flip_(
  unsigned *dest, int x, int *linetab, unsigned * palette, int h, int screenwidth,
  int *masklinetab
) {
  for(; h > 0; h--, dest += screenwidth) {
	register int lx = x;
	unsigned char *data = ((unsigned char*)linetab) + (*linetab); linetab++;
	unsigned char *maskdata = ((unsigned char*)masklinetab) + (*masklinetab); masklinetab++;
	while(lx > 0) {
	  register int count = *data++; maskdata++;
	  if(count == 0xFF) break;
	  lx -= count;
	  if(lx <= 0) break;
	  count = *data++; maskdata++;
	  if(!count) continue;
	  if((lx - count) >= screenwidth) { lx -= count; data += count; maskdata += count; continue; }
	  if(lx > screenwidth) { int diff = (lx - screenwidth); count -= diff; data += diff; maskdata += diff; lx = screenwidth; }
	  if((lx - count) < 0) { count = lx; }
	  for(; count > 0; count--)
	  {   --lx;
		  dest[lx] = blend_channel32(palette[*data++], dest[lx], *maskdata++);
	  }
	}
  }
}

/////////////////////////////////////////////////////////////////////////////

void putsprite_x8p32(
  int x, int y, int is_flip, s_sprite *sprite, s_screen *screen,
  unsigned *remap, blend32fp blend
) {
  int *linetab, *masklinetab = NULL;
  int w, h;
  unsigned *dest;
  unsigned * m;
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
  if(sprite->mask) {
	masklinetab = (int*)(sprite->mask->data);
	//if(w!=sprite->mask->width) { printf("Wrong mask width. %i %i\n", w, sprite->mask->width); return; }
	//if(h!=sprite->mask->height) { printf("Wrong mask height. %i %i\n", h, sprite->mask->height); return; }
  }
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
  dest = ((unsigned*)(screen->data)) + y*screenwidth;
  if(remap) m = remap;
  else      m = (unsigned*)sprite->palette;
  if(sprite->mask) {
	if(is_flip) putsprite_mask_flip_ (dest, x, linetab, m, h, screenwidth, masklinetab);
	else        putsprite_mask_      (dest, x, linetab, m, h, screenwidth, masklinetab);
  } else if(blend) {
	if(is_flip) putsprite_blend_flip_(dest, x, linetab, m , h, screenwidth, blend);
	else        putsprite_blend_     (dest, x  , linetab, m , h, screenwidth, blend);
  } else {
	if(is_flip) putsprite_flip_      (dest, x, linetab, m , h, screenwidth);
	else        putsprite_           (dest, x  , linetab, m , h, screenwidth);
  }
}

/////////////////////////////////////////////////////////////////////////////


static unsigned fillcolor = 0;


//--------------------------------------------------------------------------------------

static int screenwidth, screenheight;

// x: centerx on screen cx: centerx of this line
static void scaleline(int x, int cx, int width, int *linetab, unsigned* palette, unsigned *dest_c, blend32fp fp,  int scale)
{
        unsigned char* data;
        int dx, i, d;
        unsigned char * charptr;
        int scale_d=0, old_scale_d=0, cleft, cwidth;

        dx = x - ((cx*scale)/256); //draw start x

//    if(dx>=screenwidth || dx+((width*scale)>>8)<0) return; it should be check in the function that called this

        // Get ready to draw a line
        data = (unsigned char*)linetab + (*linetab);

        for(;;)
        {
                cleft = *data++;
                if(cleft==0xFF) return;// end of line
                scale_d += cleft*scale;  // dest scale, scale
                d = scale_d - old_scale_d;
                if(d >= 256) // skip some blank pixels
                {
                        dx += (d>>8);if(dx>=screenwidth) return; // out of right border? exit
                        old_scale_d = scale_d & 0xFFFFFF00;
                }
                cwidth = *data++;
                if(!cwidth) continue;
                //scale_s += cleft<<8;     // src scale, 256
                charptr = data;
                data += cwidth; // skip some bytes to next block
                while(cwidth--) // draw these pixels
                {
                        scale_d += scale;
                        d = scale_d - old_scale_d; // count scale added
                        if(d >= 256) // > 1pixel, so draw these
                        {
                                for(i=d>>8; i>0; i--, dx++) // draw a pixel
                                {
                                        if(dx>=screenwidth) return; // out of right border? exit
                                        if(dx>=0) // pass left border?
                                        {
                                                if(!fp)
                                                {
                                                        dest_c[dx] = fillcolor?fillcolor:palette[*charptr];
                                                }
                                                else
                                                {
                                                       dest_c[dx] = fp(fillcolor?fillcolor:palette[*charptr], dest_c[dx]);
                                                }
                                        }
                                }
                                old_scale_d = scale_d & 0xFFFFFF00; //truncate those less than 256
                        }
                        charptr++; // src ptr move right one pixel
                }
        }
}


// x: centerx on screen cx: centerx of this line, flip version
static void scaleline_flip(int x, int cx, int width, int *linetab, unsigned* palette, unsigned *dest_c, blend32fp fp,  int scale)
{
        unsigned char* data;
        int dx, i, d;
        unsigned char * charptr;
        int scale_d=0, old_scale_d=0, cleft, cwidth;

        dx = x + ((cx*scale)/256); //draw start x, flipped, so use + instead of -

//    if(dx>=screenwidth || dx+((width*scale)>>8)<0) return; it should be check in the function that called this

        // Get ready to draw a line
        data = (unsigned char*)linetab + (*linetab);

        for(;;)
        {
                cleft = *data++;
                if(cleft==0xFF) return; // end of line
                scale_d += cleft*scale;  // dest scale, scale
                d = scale_d - old_scale_d;
                if(d >= 256) // skip some blank pixels
                {
                        dx -= (d>>8);if(dx<0) return; // out of left border? exit
                        old_scale_d = scale_d & 0xFFFFFF00;
                }
                cwidth = *data++;
                if(!cwidth) continue;
                //scale_s += cleft<<8;     // src scale, 256
                charptr = data;
                data += cwidth; // skip some bytes to next block
                while(cwidth--) // draw these pixels
                {
                        scale_d += scale;
                        d = scale_d - old_scale_d; // count scale added
                        if(d >= 256) // > 1pixel, so draw these
                        {
                                for(i=d>>8; i>0; i--, dx--) // draw a pixel
                                {
                                        if(dx<0) return; // out of left border? exit
                                        if(dx<screenwidth) // pass right border?
                                        {
                                                if(!fp)
                                                {
                                                        dest_c[dx] = fillcolor?fillcolor:palette[*charptr];
                                                }
                                                else
                                                {
                                                        dest_c[dx] = fp(fillcolor?fillcolor:palette[*charptr], dest_c[dx]);
                                                }
                                        }
                                }
                                old_scale_d = scale_d & 0xFFFFFF00; //truncate those less than 256
                        }
                        charptr++; // src ptr move right one pixel
                }
        }

}

//sin cos tables
extern const double sin_table[];
extern const double cos_table[];

// x: centerx on screen; cx: centerx of this line; y: centery on screen; dy: the line's y coordination
// angle: rotate angle, should be a positive value from 0 - 359, check it before use this function
static void scaleline_rot(int x, int y, int cx, int dy, int width, int *linetab, unsigned* palette, unsigned *dest_c, blend32fp fp, int scale, int angle)
{
        unsigned char* data;
        int dx, rx, ry, i, d;
        unsigned char * charptr;
        unsigned *destptr;
        int scale_d=0, old_scale_d=0, cleft, cwidth;
        int j;

        double deltax, deltay;

        dx = x - ((cx*scale)/256); //draw start x
//    if(dx>=screenwidth || dx+((width*scale)>>8)<0) return; it should be check in the function that called this

        // Get ready to draw a line
        data = (unsigned char*)linetab + (*linetab);

        for(;;)
        {
                cleft = *data++;
                if(cleft==0xFF) return;// end of line
                scale_d += cleft*scale;  // dest scale, scale
                dx += (cleft*scale)>>8;
                //if(dx>=screenwidth) return; // out of right border? exit
                d = scale_d - old_scale_d;
                if(d >= 256) // skip some blank pixels
                {
                        old_scale_d = scale_d & 0xFFFFFF00;
                }
                cwidth = *data++;
                if(!cwidth) continue;
                //scale_s += cleft<<8;     // src scale, 256
                charptr = data;
                data += cwidth; // skip some bytes to next block
                while(cwidth--) // draw these pixels
                {
                        scale_d += scale;
                        d = scale_d - old_scale_d; // count scale added
                        if(d >= 256) // > 1pixel, so draw these
                        {
                                for(i=d>>8; i>0; i--) // draw a pixel
                                {
                                        // rotate,
                                        deltax = dx - x;
                                        deltay = dy - y;

                                        if(angle%90)
                                        {
                                                deltay -=0.5;
                                                for(j=0; j<4; j++)
                                                {
                                                        deltay += 0.2;
                                                        rx = (int)(x + deltax * cos_table[angle] - deltay* sin_table[angle]);
                                                        ry = (int)(y + deltay * cos_table[angle] + deltax* sin_table[angle]);

                                                        if(rx >= 0 && rx < screenwidth && ry >=0 && ry < screenheight)
                                                        {
                                                                destptr = dest_c + (screenwidth * ry + rx);
                                                                if(!fp)
                                                                {
                                                                        *destptr = fillcolor?fillcolor:palette[*charptr];
                                                                }
                                                                else
                                                                {
                                                                        *destptr = fp(fillcolor?fillcolor:palette[*charptr], *destptr);
                                                                }
                                                        }
                                                }
                                        }
                                        else
                                        {
                                                rx = (int)(x + deltax * cos_table[angle] - deltay* sin_table[angle]);
                                                ry = (int)(y + deltay * cos_table[angle] + deltax* sin_table[angle]);

                                                if(rx >= 0 && rx < screenwidth && ry >=0 && ry < screenheight)
                                                {
                                                        destptr = dest_c + (screenwidth * ry + rx);
                                                        if(!fp)
                                                        {
                                                                *destptr = fillcolor?fillcolor:palette[*charptr];
                                                        }
                                                        else
                                                        {
                                                                *destptr = fp(fillcolor?fillcolor:palette[*charptr], *destptr);
                                                        }
                                                }
                                        }

                                        ++dx;
                                }
                                old_scale_d = scale_d & 0xFFFFFF00; //truncate those less than 256
                        }
                        charptr++; // src ptr move right one pixel
                }
        }
}

// flip version of above
// x: centerx on screen; cx: centerx of this line; y: centery on screen; dy: the line's y coordination
// angle: rotate angle, should be a positive value from 0 - 359, check it before use this function
static void scaleline_rotflip(int x, int y, int cx, int dy, int width, int *linetab, unsigned* palette, unsigned *dest_c, blend32fp fp,  int scale, int angle)
{
        unsigned char* data;
        int dx, rx, ry, i, d;
        unsigned char * charptr;
        unsigned *destptr;
        int scale_d=0, old_scale_d=0, cleft, cwidth;

        int j;
        double deltax, deltay;

        dx = x + ((cx*scale)/256); //draw start x, flipped, so use + instead of -

//    if(dx>=screenwidth || dx+((width*scale)>>8)<0) return; it should be check in the function that called this

        // Get ready to draw a line
        data = (unsigned char*)linetab + (*linetab);

        for(;;)
        {
                cleft = *data++;
                if(cleft==0xFF) return;
                scale_d += cleft*scale;  // dest scale, scale
                dx -= (cleft*scale)>>8;  // move left , because it is flipped
                //if(dx<0) return; // out of left border? exit
                d = scale_d - old_scale_d;
                if(d >= 256) // skip some blank pixels
                {
                        old_scale_d = scale_d & 0xFFFFFF00;
                }
                cwidth = *data++;
                if(!cwidth) continue; // end of line
                //scale_s += cleft<<8;     // src scale, 256
                charptr = data;
                data += cwidth; // skip some bytes to next block
                while(cwidth--) // draw these pixels
                {
                        scale_d += scale;
                        d = scale_d - old_scale_d; // count scale added
                        if(d >= 256) // > 1pixel, so draw these
                        {
                                for(i=d>>8; i>0; i--) // draw a pixel
                                {
                                        // rotate,
                                        deltax = dx - x;
                                        deltay = dy - y;

                                        if(angle%90)
                                        {
                                                deltay -=0.5;
                                                for(j=0; j<4; j++)
                                                {
                                                        deltay += 0.2;
                                                        rx = (int)(x + deltax * cos_table[angle] - deltay* sin_table[angle]);
                                                        ry = (int)(y + deltay * cos_table[angle] + deltax* sin_table[angle]);

                                                        if(rx >= 0 && rx < screenwidth && ry >=0 && ry < screenheight)
                                                        {
                                                                destptr = dest_c + (screenwidth * ry + rx);
                                                                if(!fp)
                                                                {
                                                                        *destptr = fillcolor?fillcolor:palette[*charptr];
                                                                }
                                                                else
                                                                {
                                                                        *destptr = fp(fillcolor?fillcolor:palette[*charptr], *destptr);
                                                                }
                                                        }
                                                }
                                        }
                                        else
                                        {
                                                rx = (int)(x + deltax * cos_table[angle] - deltay* sin_table[angle]);
                                                ry = (int)(y + deltay * cos_table[angle] + deltax* sin_table[angle]);

                                                if(rx >= 0 && rx < screenwidth && ry >=0 && ry < screenheight)
                                                {
                                                        destptr = dest_c + (screenwidth * ry + rx);
                                                        if(!fp)
                                                        {
                                                                *destptr = fillcolor?fillcolor:palette[*charptr];
                                                        }
                                                        else
                                                        {
                                                                *destptr = fp(fillcolor?fillcolor:palette[*charptr], *destptr);
                                                        }
                                                }
                                        }
                                        --dx;
                                }
                                old_scale_d = scale_d & 0xFFFFFF00; //truncate those less than 256
                        }
                        charptr++; // src ptr move right one pixel
                }
        }

}


// scalex scaley flipy ...
void putsprite_ex_x8p32(int x, int y, s_sprite *frame, s_screen *screen, s_drawmethod* drawmethod)
{
        int *linetab;
        int height, dx, d, i, cy, t, centerx, centery;
        unsigned *dest_c;
        int scale=0, old_scale=0;
        void (*scalelinefp)(int x, int cx, int width, int *linetab, unsigned* palette, unsigned *dest_c, blend32fp fp,  int scale);
        void (*scalelinerotfp)(int x, int y, int cx, int dy, int width, int *linetab, unsigned* palette, unsigned *dest_c, blend32fp fp,  int scale, int angle);

        if(!drawmethod || drawmethod->flag==0)
        {
                putsprite_x8p32(x, y, 0, frame, screen, NULL, NULL);
                return;
        }

        if(!drawmethod->scalex || !drawmethod->scaley) return; // zero size

        // no scale, no shift, no flip, no fill, so use common method
        if(drawmethod->scalex==256 && drawmethod->scaley==256 && !drawmethod->flipy && !drawmethod->shiftx && drawmethod->fillcolor==TRANSPARENT_IDX && !drawmethod->rotate && !drawmethod->centerx && !drawmethod->centery)
        {
                putsprite_x8p32(x, y, drawmethod->flipx, frame, screen, (unsigned*)drawmethod->table, drawmethod->alpha>0?blendfunctions32[drawmethod->alpha-1]:NULL);
                return;
        }

        if(drawmethod->centerx) centerx = drawmethod->centerx+frame->centerx;
        else                    centerx = frame->centerx;
        if(drawmethod->centery) centery = drawmethod->centery+frame->centery;
        else                    centery = frame->centery;

        screenheight = screen->height;
        screenwidth = screen->width;

        cy = y;
        height = frame->height;
        linetab = (int*)(frame->data);

        if(drawmethod->fillcolor) fillcolor = drawmethod->fillcolor;
        else                      fillcolor = 0;
        if(drawmethod->alpha>0) drawmethod->fp = blendfunctions32[drawmethod->alpha-1];
        else drawmethod->fp = NULL;
//-----------------------------------------------------------------------------------------------
// rotate version, quite a large-scale if-else,
// rotate should be 0 to 359, and it should be checked before this function is called
// TODO: enable clip
        if(drawmethod->rotate)
        {
                if(drawmethod->flipx)
                {
                        dx = x + ((centerx*drawmethod->scalex)/256); //draw start x
                        scalelinerotfp = scaleline_rotflip;
                }
                else
                {
                        dx = x - ((centerx*drawmethod->scalex)/256); //draw start x
                        scalelinerotfp = scaleline_rot;
                }
                // flip in y direction, from centery
                if(drawmethod->flipy)
                {
                        t = centery*drawmethod->scaley;
                        y += t/256; // lowest
                        scale -= t<0?((-t)%256):(t%256);
                        while(height--)
                        {
                                scale += drawmethod->scaley;
                                d = scale - old_scale; // count scale added
                                if(d >= 256) // > 1pixel, so draw these
                                {
                                        for(i=d>>8; i>0; i--) // draw a line
                                        {
                                                scalelinerotfp(x+((drawmethod->shiftx*(cy-y))/256), cy, centerx, y, frame->width, linetab, (unsigned*)(drawmethod->table?drawmethod->table:frame->palette), (unsigned*)(screen->data), (blend32fp)drawmethod->fp, drawmethod->scalex, drawmethod->rotate);
                                                --y; // out of lower border? exit
                                        }
                                        old_scale = scale & 0xFFFFFF00; //truncate those less than 256
                                }
                                linetab++; //src line shift
                        }
                }
                else // un-flipped version
                {
                        t = centery*drawmethod->scaley;
                        y -= t/256; // topmost
                        scale -= t<0?((-t)%256):(t%256);

                        while(height--)
                        {
                                scale += drawmethod->scaley;
                                d = scale - old_scale; // count scale added
                                if(d >= 256) // > 1pixel, so draw these
                                {
                                        for(i=d>>8; i>0; i--) // draw a line
                                        {
                                                scalelinerotfp(x+((drawmethod->shiftx*(y-cy))/256), cy, centerx, y, frame->width, linetab, (unsigned*)(drawmethod->table?drawmethod->table:frame->palette), (unsigned*)(screen->data), (blend32fp)drawmethod->fp, drawmethod->scalex, drawmethod->rotate);
                                                ++y;
                                        }
                                        old_scale = scale & 0xFFFFFF00; //truncate those less than 256
                                }
                                linetab++; //src line shift
                        }
                 }
        } // end of rotate code
        else
// non-rotate version
//----------------------------------------------------------------------------------------------------------
        {

                if(drawmethod->flipx)
                {
					if(!drawmethod->shiftx){
                        dx = x + ((centerx*drawmethod->scalex)/256); //draw start x
                        if(dx<0 || dx-((frame->width*drawmethod->scalex)>>8)>=screenwidth) return; // out of left or right border
					}
                    scalelinefp = scaleline_flip;
                }
                else
                {
					if(!drawmethod->shiftx){
                        dx = x - ((centerx*drawmethod->scalex)/256); //draw start x
                        if(dx>=screenwidth || dx+((frame->width*drawmethod->scalex)>>8)<0) return; // out of left or right border
					}
                    scalelinefp = scaleline;
                }

                // flip in y direction, from centery
                if(drawmethod->flipy)
                {
                        t = centery*drawmethod->scaley;
                        y += t/256; // lowest
                        if(y<0) return;
                        scale -= t<0?((-t)%256):(t%256);
                        dest_c = (unsigned*)(screen->data)+y*screenwidth;
                        while(height--)
                        {
                                scale += drawmethod->scaley;
                                d = scale - old_scale; // count scale added
                                if(d >= 256) // > 1pixel, so draw these
                                {
                                        for(i=d>>8; i>0; i--) // draw a line
                                        {
                                                if(y<screenheight) // pass lower border?
                                                {
                                                        scalelinefp(x+((drawmethod->shiftx*(cy-y))/256), centerx, frame->width, linetab, (unsigned*)(drawmethod->table?drawmethod->table:frame->palette), dest_c, (blend32fp)drawmethod->fp, drawmethod->scalex);
                                                }
                                                if(--y<0) return; // out of lower border? exit
                                                dest_c -= screenwidth; // position move down one line
                                        }
                                        old_scale = scale & 0xFFFFFF00; //truncate those less than 256
                                }
                                linetab++; //src line shift
                        }
                }
                else // un-flipped version
                {
                        t = centery*drawmethod->scaley;
                        y -= t/256; // topmost
                        if(y>=screenheight) return;
                        scale -= t<0?((-t)%256):(t%256);
                        dest_c = (unsigned*)(screen->data)+y*screenwidth;

                        while(height--)
                        {
                                scale += drawmethod->scaley;
                                d = scale - old_scale; // count scale added
                                if(d >= 256) // > 1pixel, so draw these
                                {
                                        for(i=d>>8; i>0; i--) // draw a line
                                        {
                                                if(y>=0) // pass upper border?
                                                {
                                                        scalelinefp(x+((drawmethod->shiftx*(y-cy))/256), centerx, frame->width, linetab, (unsigned*)(drawmethod->table?drawmethod->table:frame->palette), dest_c, (blend32fp)drawmethod->fp, drawmethod->scalex);
                                                }
                                                if(++y>=screenheight) return; // out of lower border? exit
                                                dest_c += screenwidth; // position move down one line
                                        }
                                        old_scale = scale & 0xFFFFFF00; //truncate those less than 256
                                }
                                linetab++; //src line shift
                        }
                 }
        }// end of non-rotate code
}
