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

        dest_c += dx;

        // Get ready to draw a line
        data = (unsigned char*)linetab + (*linetab);

        for(;;)
        {
                cleft = *data++;
                if(cleft==0xFF) return;// end of line
                scale_d += cleft*scale;  // dest scale, scale
                dx += (cleft*scale)>>8;
                if(dx>=screenwidth) return; // out of right border? exit
                d = scale_d - old_scale_d;
                if(d >= 256) // skip some blank pixels
                {
                        dest_c += (d>>8);
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
                                        if(dx>=0) // pass left border?
                                        {
                                                if(!fp)
                                                {
                                                        *dest_c = fillcolor?fillcolor:palette[*charptr];
                                                }
                                                else
                                                {
                                                        *dest_c = fp(fillcolor?fillcolor:palette[*charptr], *dest_c);
                                                }
                                        }
                                        if(++dx>=screenwidth) return; // out of right border? exit
                                        dest_c++; // position move to right one pixel
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

        dest_c += dx;

        // Get ready to draw a line
        data = (unsigned char*)linetab + (*linetab);

        for(;;)
        {
                cleft = *data++;
                if(cleft==0xFF) return; // end of line
                scale_d += cleft*scale;  // dest scale, scale
                dx -= (cleft*scale)>>8;  // move left , because it is flipped
                if(dx<0) return; // out of left border? exit
                d = scale_d - old_scale_d;
                if(d >= 256) // skip some blank pixels
                {
                        dest_c -= (d>>8);
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
                                        if(dx<screenwidth) // pass right border?
                                        {
                                                if(!fp)
                                                {
                                                        *dest_c = fillcolor?fillcolor:palette[*charptr];
                                                }
                                                else
                                                {
                                                         *dest_c = fp(fillcolor?fillcolor:palette[*charptr], *dest_c);
                                                }
                                        }
                                        if(--dx<0) return; // out of left border? exit
                                        dest_c--; // position move to left one pixel
                                }
                                old_scale_d = scale_d & 0xFFFFFF00; //truncate those less than 256
                        }
                        charptr++; // src ptr move right one pixel
                }
        }

}

//sin cos tables
static const double sin_table[] = //360
{
0, 0.01745240643728351, 0.03489949670250097, 0.05233595624294383, 0.0697564737441253, 0.08715574274765824, 0.10452846326765346, 0.12186934340514747, 0.13917310096006544, 0.15643446504023087, 0.17364817766693033, 0.1908089953765448, 0.20791249081775931, 0.224951054343865, 0.24192189559966773, 0.25881904510252074, 0.27563735582499924, 0.29237170472273677, 0.3090249943749474, 0.32556815445715664, 0.3420201433256687, 0.35836794954530027, 0.374606593415912, 0.3907311284892737, 0.40673664307580015, 0.42261826174069944, 0.4383711467890774, 0.45399049973954675, 0.4694715627858908, 0.48480962024633706, 0.49999999999999994, 0.5150380749100542, 0.5299192642332049, 0.5446390350150271, 0.5591929034707469, 0.573576436351046, 0.5877852522924731, 0.6018150231520483, 0.6156614753256582, 0.6293203910498374, 0.6427876096865392, 0.6560590289905072, 0.6691306063588582, 0.6819983600624985, 0.6946583704589972, 0.7071067811865475, 0.7193398003386511, 0.7313537024191705, 0.7431448254773941, 0.754709580222772, 0.766044443118978, 0.7771459614569708, 0.788010753606722, 0.7986355100472928, 0.8090249943749474, 0.8191520442889918, 0.8290375725550417, 0.8386705679454239, 0.848048096156426, 0.8572473007021122, 0.8660254037844386, 0.8746197071393957, 0.8829475928589269, 0.8910065241883678, 0.898794046299247, 0.9063077870366499, 0.9135454576426009, 0.9205048534524403, 0.9271838545667874, 0.9335804264972017, 0.9396926207859083, 0.9455185755993247, 0.9510565242951535, 0.9563047559630354, 0.9612624959383189, 0.9659258262890683, 0.9702957262759965, 0.9743700647852352, 0.9781476007338056, 0.982427183447664, 0.984807753012208, 0.9876883405951378, 0.9902680687415702, 0.992546152441322, 0.9945218953682733, 0.9961946980917455, 0.9975640502598242, 0.9986295347545738, 0.9993908270190958, 0.9998476951563913, 1, 0.9998476951563913, 0.9993908270190958, 0.9986295347545738, 0.9975640502598242, 0.9961946980917455, 0.9945218953682734, 0.9925461524413221, 0.9902680687415704, 0.9876883405951377, 0.984807753012208, 0.982427183447664, 0.9781476007338057, 0.9743700647852352, 0.9702957262759965, 0.9659258262890683, 0.9612624959383189, 0.9563047559630355, 0.9510565242951536, 0.9455185755993248, 0.9396926207859084, 0.9335804264972017, 0.9271838545667874, 0.9205048534524404, 0.913545457642601, 0.90630778703665, 0.8987940462992469, 0.8910065241883679, 0.8829475928589271, 0.8746197071393958, 0.8660254037844387, 0.8572473007021123, 0.8480480961564261, 0.8386705679454239, 0.8290375725550417, 0.819152044288992, 0.8090249943749474, 0.7986355100472927, 0.788010753606722, 0.777145961456971, 0.766044443118978, 0.7547095802227718, 0.7431448254773942, 0.7313537024191706, 0.7193398003386514, 0.7071067811865476, 0.6946583704589971, 0.6819983600624986, 0.6691306063588583, 0.6560590289905073, 0.6427876096865395, 0.6293203910498377, 0.6156614753256584, 0.6018150231520482, 0.5877852522924732, 0.5735764363510464, 0.5591929034707469, 0.544639035015027, 0.5299192642332049, 0.5150380749100544, 0.49999999999999994, 0.48480962024633717, 0.4694715627858911, 0.45399049973954686, 0.4383711467890773, 0.4226182617406995, 0.40673664307580043, 0.39073112848927424, 0.37460659341591223, 0.3583679495453002, 0.3420201433256689, 0.32556815445715703, 0.3090249943749475, 0.29237170472273704, 0.27563735582499966, 0.258819045102521, 0.24192189559966773, 0.22495105434386478, 0.20791249081775931, 0.19080899537654497, 0.17364817766693027, 0.15643446504023098, 0.13917310096006574, 0.12186934340514754, 0.10452846326765373, 0.08715574274765864, 0.06975647374412552, 0.05233595624294381, 0.0348994967025007, 0.01745240643728344, 1.2246063538223772e-24, -0.017452406437283192, -0.0348994967025009, -0.052335956242943564, -0.06975647374412483, -0.08715574274765794, -0.10452846326765305, -0.12186934340514774, -0.13917310096006552, -0.15643446504023073, -0.17364817766693047, -0.19080899537654472, -0.20791249081775906, -0.22495105434386497, -0.2419218955996675, -0.25881904510252035, -0.275637355824999, -0.2923717047227364, -0.30902499437494773, -0.32556815445715675, -0.34202014332566865, -0.35836794954530043, -0.374606593415912, -0.39073112848927355, -0.4067366430757998, -0.4226182617406993, -0.43837114678907707, -0.45399049973954625, -0.46947156278589086, -0.48480962024633694, -0.5000000000000001, -0.5150380749100542, -0.5299192642332048, -0.5446390350150271, -0.5591929034707467, -0.5735764363510458, -0.587785252292473, -0.601815023152048, -0.6156614753256578, -0.6293203910498376, -0.6427876096865392, -0.6560590289905074, -0.6691306063588582, -0.6819983600624984, -0.6946583704589974, -0.7071067811865475, -0.7193398003386509, -0.7313537024191701, -0.743144825477394, -0.7547095802227717, -0.7660444431189779, -0.7771459614569711, -0.7880107536067221, -0.7986355100472928, -0.8090249943749473, -0.8191520442889924, -0.8290375725550414, -0.838670567945424, -0.848048096156426, -0.8572473007021121, -0.8660254037844384, -0.874619707139396, -0.882947592858927, -0.8910065241883678, -0.8987940462992468, -0.9063077870366497, -0.913545457642601, -0.9205048534524403, -0.9271838545667873, -0.9335804264972024, -0.9396926207859082, -0.9455185755993248, -0.9510565242951535, -0.9563047559630353, -0.961262495938319, -0.9659258262890683, -0.9702957262759965, -0.9743700647852351, -0.9781476007338056, -0.9824271834476639, -0.984807753012208, -0.9876883405951377, -0.9902680687415704, -0.9925461524413221, -0.9945218953682734, -0.9961946980917455, -0.9975640502598242, -0.9986295347545738, -0.9993908270190956, -0.9998476951563913, -1, -0.9998476951563913, -0.9993908270190958, -0.9986295347545738, -0.9975640502598243, -0.9961946980917455, -0.9945218953682734, -0.992546152441322, -0.9902680687415704, -0.9876883405951378, -0.9848077530122081, -0.9824271834476641, -0.9781476007338058, -0.9743700647852352, -0.9702957262759966, -0.9659258262890682, -0.9612624959383188, -0.9563047559630354, -0.9510565242951536, -0.945518575599317, -0.9396926207859085, -0.9335804264972021, -0.9271838545667874, -0.9205048534524405, -0.9135454576426008, -0.9063077870366499, -0.898794046299247, -0.8910065241883679, -0.8829475928589271, -0.8746197071393961, -0.8660254037844386, -0.8572473007021123, -0.8480480961564262, -0.8386705679454243, -0.8290375725550421, -0.8191520442889918, -0.8090249943749476, -0.798635510047293, -0.7880107536067218, -0.7771459614569708, -0.7660444431189781, -0.7547095802227722, -0.7431448254773946, -0.731353702419171, -0.7193398003386517, -0.7071067811865477, -0.6946583704589976, -0.6819983600624982, -0.6691306063588581, -0.6560590289905074, -0.6427876096865396, -0.6293203910498378, -0.6156614753256588, -0.6018150231520483, -0.5877852522924734, -0.5735764363510465, -0.5591929034707473, -0.544639035015027, -0.5299192642332058, -0.5150380749100545, -0.5000000000000004, -0.4848096202463369, -0.4694715627858908, -0.45399049973954697, -0.438371146789077, -0.4226182617407, -0.40673664307580015, -0.3907311284892747, -0.37460659341591235, -0.35836794954530077, -0.3420201433256686, -0.32556815445715753, -0.3090249943749476, -0.29237170472273627, -0.2756373558249998, -0.2588190451025207, -0.24192189559966787, -0.22495105434386534, -0.20791249081775987, -0.19080899537654466, -0.17364817766693127, -0.1564344650402311, -0.13917310096006588, -0.12186934340514811, -0.10452846326765341, -0.08715574274765832, -0.06975647374412476, -0.05233595624294437, -0.034899496702500823, -0.01745240643728445
};
static const double cos_table[] = //360
{
1, 0.9998476951563913, 0.9993908270190958, 0.9986295347545738, 0.9975640502598242, 0.9961946980917455, 0.9945218953682733, 0.992546152441322, 0.9902680687415704, 0.9876883405951378, 0.984807753012208, 0.982427183447664, 0.9781476007338057, 0.9743700647852352, 0.9702957262759965, 0.9659258262890683, 0.9612624959383189, 0.9563047559630354, 0.9510565242951535, 0.9455185755993248, 0.9396926207859084, 0.9335804264972017, 0.9271838545667874, 0.9205048534524404, 0.9135454576426009, 0.9063077870366499, 0.898794046299247, 0.8910065241883679, 0.882947592858927, 0.8746197071393957, 0.8660254037844387, 0.8572473007021123, 0.848048096156426, 0.838670567945424, 0.8290375725550424, 0.8191520442889918, 0.8090249943749474, 0.7986355100472928, 0.788010753606722, 0.7771459614569709, 0.766044443118978, 0.7547095802227721, 0.7431448254773942, 0.7313537024191706, 0.7193398003386512, 0.7071067811865476, 0.6946583704589974, 0.6819983600624985, 0.6691306063588582, 0.6560590289905073, 0.6427876096865394, 0.6293203910498375, 0.6156614753256583, 0.6018150231520484, 0.5877852522924731, 0.5735764363510462, 0.5591929034707468, 0.5446390350150272, 0.5299192642332049, 0.5150380749100544, 0.5000000000000001, 0.4848096202463371, 0.46947156278589086, 0.4539904997395468, 0.43837114678907746, 0.42261826174069944, 0.4067366430758002, 0.39073112848927394, 0.37460659341591196, 0.3583679495453004, 0.3420201433256688, 0.32556815445715675, 0.30902499437494745, 0.29237170472273677, 0.27563735582499924, 0.25881904510252074, 0.2419218955996679, 0.22495105434386492, 0.20791249081775945, 0.19080899537654491, 0.17364817766693041, 0.15643446504023092, 0.1391731009600657, 0.12186934340514749, 0.10452846326765346, 0.08715574274765814, 0.06975647374412545, 0.052335956242943966, 0.03489949670250108, 0.017452406437283376, 6.123031769111886e-17, -0.017452406437283477, -0.03489949670250073, -0.05233595624294362, -0.06975647374412533, -0.08715574274765823, -0.10452846326765333, -0.12186934340514736, -0.13917310096006535, -0.15643446504023103, -0.1736481776669303, -0.1908089953765448, -0.20791249081775912, -0.2249510543438648, -0.24192189559966778, -0.25881904510252085, -0.27563735582499905, -0.29237170472273666, -0.30902499437494734, -0.3255681544571564, -0.3420201433256687, -0.35836794954530027, -0.37460659341591207, -0.3907311284892736, -0.40673664307580004, -0.42261826174069933, -0.4383711467890775, -0.4539904997395467, -0.46947156278589053, -0.484809620246337, -0.4999999999999998, -0.5150380749100543, -0.5299192642332048, -0.5446390350150271, -0.5591929034707467, -0.5735764363510458, -0.587785252292473, -0.6018150231520484, -0.6156614753256583, -0.6293203910498373, -0.6427876096865394, -0.6560590289905075, -0.6691306063588582, -0.6819983600624984, -0.694658370458997, -0.7071067811865475, -0.7193398003386512, -0.7313537024191705, -0.743144825477394, -0.754709580222772, -0.7660444431189779, -0.7771459614569707, -0.7880107536067219, -0.7986355100472929, -0.8090249943749473, -0.8191520442889924, -0.8290375725550424, -0.8386705679454242, -0.848048096156426, -0.8572473007021122, -0.8660254037844387, -0.8746197071393957, -0.8829475928589268, -0.8910065241883678, -0.898794046299247, -0.9063077870366499, -0.9135454576426008, -0.9205048534524401, -0.9271838545667873, -0.9335804264972017, -0.9396926207859083, -0.9455185755993247, -0.9510565242951535, -0.9563047559630354, -0.9612624959383187, -0.9659258262890682, -0.9702957262759965, -0.9743700647852352, -0.9781476007338057, -0.982427183447664, -0.984807753012208, -0.9876883405951377, -0.9902680687415702, -0.992546152441322, -0.9945218953682733, -0.9961946980917455, -0.9975640502598242, -0.9986295347545738, -0.9993908270190958, -0.9998476951563913, -1, -0.9998476951563913, -0.9993908270190958, -0.9986295347545738, -0.9975640502598243, -0.9961946980917455, -0.9945218953682734, -0.992546152441322, -0.9902680687415702, -0.9876883405951378, -0.984807753012208, -0.982427183447664, -0.9781476007338057, -0.9743700647852352, -0.9702957262759965, -0.9659258262890684, -0.9612624959383189, -0.9563047559630355, -0.9510565242951535, -0.9455185755993247, -0.9396926207859084, -0.9335804264972017, -0.9271838545667874, -0.9205048534524404, -0.9135454576426011, -0.90630778703665, -0.8987940462992471, -0.8910065241883681, -0.8829475928589269, -0.8746197071393958, -0.8660254037844386, -0.8572473007021123, -0.8480480961564261, -0.838670567945424, -0.8290375725550418, -0.819152044288992, -0.8090249943749476, -0.798635510047293, -0.7880107536067222, -0.7771459614569708, -0.766044443118978, -0.7547095802227719, -0.7431448254773942, -0.7313537024191706, -0.7193398003386511, -0.7071067811865477, -0.6946583704589976, -0.6819983600624989, -0.6691306063588585, -0.6560590289905076, -0.6427876096865395, -0.6293203910498372, -0.6156614753256581, -0.6018150231520483, -0.5877852522924732, -0.5735764363510464, -0.5591929034707472, -0.544639035015027, -0.529919264233205, -0.5150380749100545, -0.5000000000000004, -0.48480962024633683, -0.46947156278589075, -0.4539904997395469, -0.43837114678907773, -0.42261826174069994, -0.4067366430758001, -0.3907311284892738, -0.3746065934159123, -0.3583679495453007, -0.3420201433256694, -0.32556815445715664, -0.30902499437494756, -0.2923717047227371, -0.2756373558249989, -0.25881904510252063, -0.24192189559966778, -0.22495105434386525, -0.2079124908177598, -0.19080899537654547, -0.17364817766693033, -0.15643446504023103, -0.13917310096006494, -0.12186934340514717, -0.10452846326765336, -0.08715574274765825, -0.06975647374412558, -0.052335956242944306, -0.03489949670250245, -0.017452406437283498, -1.836909530733566e-24, 0.01745240643728313, 0.03489949670250128, 0.052335956242943946, 0.06975647374412522, 0.08715574274765789, 0.10452846326765298, 0.12186934340514768, 0.13917310096006546, 0.15643446504023067, 0.17364817766692997, 0.19080899537654425, 0.20791249081775856, 0.22495105434386492, 0.24192189559966745, 0.25881904510252113, 0.2756373558249994, 0.2923717047227367, 0.30902499437494723, 0.3255681544571563, 0.34202014332566824, 0.35836794954529954, 0.37460659341591196, 0.3907311284892735, 0.40673664307580054, 0.4226182617406996, 0.4383711467890774, 0.45399049973954664, 0.4694715627858904, 0.4848096202463365, 0.5000000000000001, 0.5150380749100542, 0.5299192642332047, 0.5446390350150266, 0.5591929034707462, 0.573576436351046, 0.5877852522924729, 0.6018150231520479, 0.6156614753256585, 0.6293203910498375, 0.6427876096865392, 0.656059028990507, 0.6691306063588578, 0.681998360062498, 0.6946583704589966, 0.7071067811865473, 0.7193398003386509, 0.7313537024191707, 0.7431448254773942, 0.7547095802227719, 0.7660444431189778, 0.7771459614569706, 0.7880107536067224, 0.7986355100472928, 0.8090249943749473, 0.8191520442889924, 0.8290375725550414, 0.838670567945424, 0.8480480961564254, 0.8572473007021121, 0.8660254037844384, 0.8746197071393958, 0.8829475928589269, 0.8910065241883678, 0.8987940462992471, 0.9063077870366497, 0.913545457642601, 0.9205048534524399, 0.9271838545667873, 0.9335804264972015, 0.9396926207859084, 0.9455185755993245, 0.9510565242951535, 0.9563047559630357, 0.9612624959383187, 0.9659258262890683, 0.9702957262759965, 0.9743700647852351, 0.9781476007338056, 0.982427183447664, 0.9848077530122079, 0.9876883405951377, 0.9902680687415702, 0.992546152441322, 0.9945218953682733, 0.9961946980917455, 0.9975640502598243, 0.9986295347545738, 0.9993908270190958, 0.9998476951563913
};

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
