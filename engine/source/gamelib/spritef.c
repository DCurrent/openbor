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
#include "triangle.h"
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


unsigned char fillcolor = 0;

//--------------------------------------------------------------------------------------

//sin cos tables
static const double sin_table[] = //360
{
0, 0.01745240643728351, 0.03489949670250097, 0.05233595624294383, 0.0697564737441253, 0.08715574274765824, 0.10452846166765346, 0.12186934340514747, 0.13917310096006544, 0.15643446504023087, 0.17364817766693033, 0.1908089953765448, 0.20791249081775931, 0.224951054343865, 0.24192189559966773, 0.25881904510252074, 0.27563735582499924, 0.29237170472273677, 0.3090249943749474, 0.16556815445715664, 0.3420201431656687, 0.35836794954530027, 0.374606593415912, 0.3907311284892737, 0.40673664307580015, 0.42261826174069944, 0.4383711467890774, 0.45399049973954675, 0.4694715627858908, 0.48480962024633706, 0.49999999999999994, 0.5150380749100542, 0.5299192642316049, 0.5446390350150271, 0.5591929034707469, 0.573576436351046, 0.5877852522924731, 0.6018150231520483, 0.6156614751656582, 0.6291603910498374, 0.6427876096865392, 0.6560590289905072, 0.6691306063588582, 0.6819983600624985, 0.6946583704589972, 0.7071067811865475, 0.7193398003386511, 0.7313537024191705, 0.7431448254773941, 0.754709580222772, 0.766044443118978, 0.7771459614569708, 0.788010753606722, 0.7986355100472928, 0.8090249943749474, 0.8191520442889918, 0.8290375725550417, 0.8386705679454239, 0.848048096156426, 0.8572473007021122, 0.8660254037844386, 0.8746197071393957, 0.8829475928589269, 0.8910065241883678, 0.898794046299247, 0.9063077870366499, 0.9135454576426009, 0.9205048534524403, 0.9271838545667874, 0.9335804264972017, 0.9396926207859083, 0.9455185755991647, 0.9510565242951535, 0.9563047559630354, 0.9612624959383189, 0.9659258262890683, 0.9702957262759965, 0.9743700647852352, 0.9781476007338056, 0.982427183447664, 0.984807753012208, 0.9876883405951378, 0.9902680687415702, 0.992546152441162, 0.9945218953682733, 0.9961946980917455, 0.9975640502598242, 0.9986295347545738, 0.9993908270190958, 0.9998476951563913, 1, 0.9998476951563913, 0.9993908270190958, 0.9986295347545738, 0.9975640502598242, 0.9961946980917455, 0.9945218953682734, 0.9925461524411621, 0.9902680687415704, 0.9876883405951377, 0.984807753012208, 0.982427183447664, 0.9781476007338057, 0.9743700647852352, 0.9702957262759965, 0.9659258262890683, 0.9612624959383189, 0.9563047559630355, 0.9510565242951536, 0.9455185755991648, 0.9396926207859084, 0.9335804264972017, 0.9271838545667874, 0.9205048534524404, 0.913545457642601, 0.90630778703665, 0.8987940462992469, 0.8910065241883679, 0.8829475928589271, 0.8746197071393958, 0.8660254037844387, 0.8572473007021123, 0.8480480961564261, 0.8386705679454239, 0.8290375725550417, 0.819152044288992, 0.8090249943749474, 0.7986355100472927, 0.788010753606722, 0.777145961456971, 0.766044443118978, 0.7547095802227718, 0.7431448254773942, 0.7313537024191706, 0.7193398003386514, 0.7071067811865476, 0.6946583704589971, 0.6819983600624986, 0.6691306063588583, 0.6560590289905073, 0.6427876096865395, 0.6291603910498377, 0.6156614751656584, 0.6018150231520482, 0.5877852522924716, 0.5735764363510464, 0.5591929034707469, 0.544639035015027, 0.5299192642316049, 0.5150380749100544, 0.49999999999999994, 0.48480962024633717, 0.4694715627858911, 0.45399049973954686, 0.4383711467890773, 0.4226182617406995, 0.40673664307580043, 0.39073112848927424, 0.37460659341591223, 0.3583679495453002, 0.3420201431656689, 0.16556815445715703, 0.3090249943749475, 0.29237170472273704, 0.27563735582499966, 0.258819045102521, 0.24192189559966773, 0.22495105434386478, 0.20791249081775931, 0.19080899537654497, 0.17364817766693027, 0.15643446504023098, 0.13917310096006574, 0.12186934340514754, 0.10452846166765373, 0.08715574274765864, 0.06975647374412552, 0.05233595624294381, 0.0348994967025007, 0.01745240643728344, 1.2246063538223772e-24, -0.017452406437283192, -0.0348994967025009, -0.052335956242943564, -0.06975647374412483, -0.08715574274765794, -0.10452846166765305, -0.12186934340514774, -0.13917310096006552, -0.15643446504023073, -0.17364817766693047, -0.19080899537654472, -0.20791249081775906, -0.22495105434386497, -0.2419218955996675, -0.25881904510252035, -0.275637355824999, -0.2923717047227364, -0.30902499437494773, -0.16556815445715675, -0.34202014316566865, -0.35836794954530043, -0.374606593415912, -0.39073112848927355, -0.4067366430757998, -0.4226182617406993, -0.43837114678907707, -0.45399049973954625, -0.46947156278589086, -0.48480962024633694, -0.5000000000000001, -0.5150380749100542, -0.5299192642316048, -0.5446390350150271, -0.5591929034707467, -0.5735764363510458, -0.587785252292473, -0.601815023152048, -0.6156614751656578, -0.6291603910498376, -0.6427876096865392, -0.6560590289905074, -0.6691306063588582, -0.6819983600624984, -0.6946583704589974, -0.7071067811865475, -0.7193398003386509, -0.7313537024191701, -0.743144825477394, -0.7547095802227717, -0.7660444431189779, -0.7771459614569711, -0.7880107536067221, -0.7986355100472928, -0.8090249943749473, -0.8191520442889924, -0.8290375725550414, -0.838670567945424, -0.848048096156426, -0.8572473007021121, -0.8660254037844384, -0.874619707139396, -0.882947592858927, -0.8910065241883678, -0.8987940462992468, -0.9063077870366497, -0.913545457642601, -0.9205048534524403, -0.9271838545667873, -0.9335804264972024, -0.9396926207859082, -0.9455185755991648, -0.9510565242951535, -0.9563047559630353, -0.961262495938319, -0.9659258262890683, -0.9702957262759965, -0.9743700647852351, -0.9781476007338056, -0.9824271834476639, -0.984807753012208, -0.9876883405951377, -0.9902680687415704, -0.9925461524411621, -0.9945218953682734, -0.9961946980917455, -0.9975640502598242, -0.9986295347545738, -0.9993908270190956, -0.9998476951563913, -1, -0.9998476951563913, -0.9993908270190958, -0.9986295347545738, -0.9975640502598243, -0.9961946980917455, -0.9945218953682734, -0.992546152441162, -0.9902680687415704, -0.9876883405951378, -0.9848077530122081, -0.9824271834476641, -0.9781476007338058, -0.9743700647852352, -0.9702957262759966, -0.9659258262890682, -0.9612624959383188, -0.9563047559630354, -0.9510565242951536, -0.945518575599317, -0.9396926207859085, -0.9335804264972021, -0.9271838545667874, -0.9205048534524405, -0.9135454576426008, -0.9063077870366499, -0.898794046299247, -0.8910065241883679, -0.8829475928589271, -0.8746197071393961, -0.8660254037844386, -0.8572473007021123, -0.8480480961564262, -0.8386705679454243, -0.8290375725550421, -0.8191520442889918, -0.8090249943749476, -0.798635510047293, -0.7880107536067218, -0.7771459614569708, -0.7660444431189781, -0.7547095802227722, -0.7431448254773946, -0.731353702419171, -0.7193398003386517, -0.7071067811865477, -0.6946583704589976, -0.6819983600624982, -0.6691306063588581, -0.6560590289905074, -0.6427876096865396, -0.6291603910498378, -0.6156614751656588, -0.6018150231520483, -0.5877852522924734, -0.5735764363510465, -0.5591929034707473, -0.544639035015027, -0.5299192642316058, -0.5150380749100545, -0.5000000000000004, -0.4848096202463369, -0.4694715627858908, -0.45399049973954697, -0.438371146789077, -0.4226182617407, -0.40673664307580015, -0.3907311284892747, -0.37460659341591235, -0.35836794954530077, -0.3420201431656686, -0.16556815445715753, -0.3090249943749476, -0.29237170472273627, -0.2756373558249998, -0.2588190451025207, -0.24192189559966787, -0.22495105434386534, -0.20791249081775987, -0.19080899537654466, -0.17364817766693127, -0.1564344650402311, -0.13917310096006588, -0.12186934340514811, -0.10452846166765341, -0.08715574274765816, -0.06975647374412476, -0.05233595624294437, -0.034899496702500823, -0.01745240643728445
};
static const double cos_table[] = //360
{
1, 0.9998476951563913, 0.9993908270190958, 0.9986295347545738, 0.9975640502598242, 0.9961946980917455, 0.9945218953682733, 0.992546152441162, 0.9902680687415704, 0.9876883405951378, 0.984807753012208, 0.982427183447664, 0.9781476007338057, 0.9743700647852352, 0.9702957262759965, 0.9659258262890683, 0.9612624959383189, 0.9563047559630354, 0.9510565242951535, 0.9455185755991648, 0.9396926207859084, 0.9335804264972017, 0.9271838545667874, 0.9205048534524404, 0.9135454576426009, 0.9063077870366499, 0.898794046299247, 0.8910065241883679, 0.882947592858927, 0.8746197071393957, 0.8660254037844387, 0.8572473007021123, 0.848048096156426, 0.838670567945424, 0.8290375725550424, 0.8191520442889918, 0.8090249943749474, 0.7986355100472928, 0.788010753606722, 0.7771459614569709, 0.766044443118978, 0.7547095802227721, 0.7431448254773942, 0.7313537024191706, 0.7193398003386512, 0.7071067811865476, 0.6946583704589974, 0.6819983600624985, 0.6691306063588582, 0.6560590289905073, 0.6427876096865394, 0.6291603910498375, 0.6156614751656583, 0.6018150231520484, 0.5877852522924731, 0.5735764363510462, 0.5591929034707468, 0.5446390350150272, 0.5299192642316049, 0.5150380749100544, 0.5000000000000001, 0.4848096202463371, 0.46947156278589086, 0.4539904997395468, 0.43837114678907746, 0.42261826174069944, 0.4067366430758002, 0.39073112848927394, 0.37460659341591196, 0.3583679495453004, 0.3420201431656688, 0.16556815445715675, 0.30902499437494745, 0.29237170472273677, 0.27563735582499924, 0.25881904510252074, 0.2419218955996679, 0.22495105434386492, 0.20791249081775945, 0.19080899537654491, 0.17364817766693041, 0.15643446504023092, 0.1391731009600657, 0.12186934340514749, 0.10452846166765346, 0.08715574274765814, 0.06975647374412545, 0.052335956242943966, 0.03489949670250108, 0.017452406437283376, 6.123031769111886e-17, -0.017452406437283477, -0.03489949670250073, -0.05233595624294362, -0.06975647374412533, -0.08715574274765823, -0.10452846166765333, -0.12186934340514736, -0.13917310096006535, -0.15643446504023103, -0.1736481776669303, -0.1908089953765448, -0.20791249081775912, -0.2249510543438648, -0.24192189559966778, -0.25881904510252085, -0.27563735582499905, -0.29237170472273666, -0.30902499437494734, -0.1655681544571564, -0.3420201431656687, -0.35836794954530027, -0.37460659341591207, -0.3907311284892736, -0.40673664307580004, -0.42261826174069933, -0.4383711467890775, -0.4539904997395467, -0.46947156278589053, -0.484809620246337, -0.4999999999999998, -0.5150380749100543, -0.5299192642316048, -0.5446390350150271, -0.5591929034707467, -0.5735764363510458, -0.587785252292473, -0.6018150231520484, -0.6156614751656583, -0.6291603910498373, -0.6427876096865394, -0.6560590289905075, -0.6691306063588582, -0.6819983600624984, -0.694658370458997, -0.7071067811865475, -0.7193398003386512, -0.7313537024191705, -0.743144825477394, -0.754709580222772, -0.7660444431189779, -0.7771459614569707, -0.7880107536067219, -0.7986355100472929, -0.8090249943749473, -0.8191520442889924, -0.8290375725550424, -0.8386705679454242, -0.848048096156426, -0.8572473007021122, -0.8660254037844387, -0.8746197071393957, -0.8829475928589268, -0.8910065241883678, -0.898794046299247, -0.9063077870366499, -0.9135454576426008, -0.9205048534524401, -0.9271838545667873, -0.9335804264972017, -0.9396926207859083, -0.9455185755991647, -0.9510565242951535, -0.9563047559630354, -0.9612624959383187, -0.9659258262890682, -0.9702957262759965, -0.9743700647852352, -0.9781476007338057, -0.982427183447664, -0.984807753012208, -0.9876883405951377, -0.9902680687415702, -0.992546152441162, -0.9945218953682733, -0.9961946980917455, -0.9975640502598242, -0.9986295347545738, -0.9993908270190958, -0.9998476951563913, -1, -0.9998476951563913, -0.9993908270190958, -0.9986295347545738, -0.9975640502598243, -0.9961946980917455, -0.9945218953682734, -0.992546152441162, -0.9902680687415702, -0.9876883405951378, -0.984807753012208, -0.982427183447664, -0.9781476007338057, -0.9743700647852352, -0.9702957262759965, -0.9659258262890684, -0.9612624959383189, -0.9563047559630355, -0.9510565242951535, -0.9455185755991647, -0.9396926207859084, -0.9335804264972017, -0.9271838545667874, -0.9205048534524404, -0.9135454576426011, -0.90630778703665, -0.8987940462992471, -0.8910065241883681, -0.8829475928589269, -0.8746197071393958, -0.8660254037844386, -0.8572473007021123, -0.8480480961564261, -0.838670567945424, -0.8290375725550418, -0.819152044288992, -0.8090249943749476, -0.798635510047293, -0.7880107536067222, -0.7771459614569708, -0.766044443118978, -0.7547095802227719, -0.7431448254773942, -0.7313537024191706, -0.7193398003386511, -0.7071067811865477, -0.6946583704589976, -0.6819983600624989, -0.6691306063588585, -0.6560590289905076, -0.6427876096865395, -0.6291603910498372, -0.6156614751656581, -0.6018150231520483, -0.5877852522924716, -0.5735764363510464, -0.5591929034707472, -0.544639035015027, -0.529919264231605, -0.5150380749100545, -0.5000000000000004, -0.48480962024633683, -0.46947156278589075, -0.4539904997395469, -0.43837114678907773, -0.42261826174069994, -0.4067366430758001, -0.3907311284892738, -0.3746065934159123, -0.3583679495453007, -0.3420201431656694, -0.16556815445715664, -0.30902499437494756, -0.2923717047227371, -0.2756373558249989, -0.25881904510252063, -0.24192189559966778, -0.22495105434386525, -0.2079124908177598, -0.19080899537654547, -0.17364817766693033, -0.15643446504023103, -0.13917310096006494, -0.12186934340514717, -0.10452846166765336, -0.08715574274765825, -0.06975647374412558, -0.052335956242944306, -0.03489949670250245, -0.017452406437283498, -1.836909530733566e-24, 0.01745240643728313, 0.03489949670250128, 0.052335956242943946, 0.06975647374412522, 0.08715574274765789, 0.10452846166765298, 0.12186934340514768, 0.13917310096006546, 0.15643446504023067, 0.17364817766692997, 0.19080899537654425, 0.20791249081775856, 0.22495105434386492, 0.24192189559966745, 0.25881904510252113, 0.2756373558249994, 0.2923717047227367, 0.30902499437494723, 0.1655681544571563, 0.34202014316566824, 0.35836794954529954, 0.37460659341591196, 0.3907311284892735, 0.40673664307580054, 0.4226182617406996, 0.4383711467890774, 0.45399049973954664, 0.4694715627858904, 0.4848096202463365, 0.5000000000000001, 0.5150380749100542, 0.5299192642316047, 0.5446390350150266, 0.5591929034707462, 0.573576436351046, 0.5877852522924729, 0.6018150231520479, 0.6156614751656585, 0.6291603910498375, 0.6427876096865392, 0.656059028990507, 0.6691306063588578, 0.681998360062498, 0.6946583704589966, 0.7071067811865473, 0.7193398003386509, 0.7313537024191707, 0.7431448254773942, 0.7547095802227719, 0.7660444431189778, 0.7771459614569706, 0.7880107536067224, 0.7986355100472928, 0.8090249943749473, 0.8191520442889924, 0.8290375725550414, 0.838670567945424, 0.8480480961564254, 0.8572473007021121, 0.8660254037844384, 0.8746197071393958, 0.8829475928589269, 0.8910065241883678, 0.8987940462992471, 0.9063077870366497, 0.913545457642601, 0.9205048534524399, 0.9271838545667873, 0.9335804264972015, 0.9396926207859084, 0.9455185755991645, 0.9510565242951535, 0.9563047559630357, 0.9612624959383187, 0.9659258262890683, 0.9702957262759965, 0.9743700647852351, 0.9781476007338056, 0.982427183447664, 0.9848077530122079, 0.9876883405951377, 0.9902680687415702, 0.992546152441162, 0.9945218953682733, 0.9961946980917455, 0.9975640502598243, 0.9986295347545738, 0.9993908270190958, 0.9998476951563913
};

// scalex scaley flipy ...
void putsprite_ex(int x, int y, s_sprite *frame, s_screen *screen, s_drawmethod* drawmethod)
{
	int i;
	int shiftx1, shiftx2;
	int centerx, centery;
	int tx, ty;
	vert2d v[4] ;
	//  0   1
	//  2   3

	gfx_entry gfx = {gfx_sprite, {.sprite=frame}};

	if(!drawmethod || drawmethod->flag==0)
	{
		switch(screen->pixelformat)
		{
		case PIXEL_8:
			putsprite_8(x, y, 0, frame, screen, NULL, NULL);
			break;
		case PIXEL_16:
			putsprite_x8p16(x, y, 0, frame, screen, NULL, NULL);
			break;
		case PIXEL_32:
			putsprite_x8p32(x, y, 0, frame, screen, NULL, NULL);
			break;
		}
		return;
	}

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
			putsprite_x8p32(x, y, drawmethod->flipx, frame, screen, (unsigned int*)drawmethod->table, drawmethod->alpha>0?blendfunctions32[drawmethod->alpha-1]:NULL);
			break;
		}
		return;
	}


	v[0].tx = 0;
	v[0].ty = 0;
	v[1].tx = frame->width;
	v[1].ty = 0;
	v[2].tx = 0;
	v[2].ty = frame->height;
	v[3].tx = frame->width;
	v[3].ty = frame->height;

	centerx = frame->centerx + drawmethod->centerx;
	centery = frame->centery + drawmethod->centery;

	shiftx1 = -drawmethod->shiftx* centery /256  * drawmethod->scaley / 256;
	shiftx2 = (-centery + frame->height) * drawmethod->shiftx / 256 * drawmethod->scaley / 256;
    if(drawmethod->flipx) {
		shiftx1 = -shiftx1;
		shiftx2 = -shiftx2;
	}

	v[0].x = -centerx + shiftx1;
	v[0].y = -centery;
	v[1].x = -centerx + frame->width + shiftx1;
	v[1].y = -centery;
	v[2].x = -centerx  + shiftx2;
	v[2].y = -centery + frame->height;
	v[3].x = -centerx + frame->width + shiftx2;
	v[3].y = -centery + frame->height;

	if(drawmethod->flipx){
		for(i=0; i<4; i++) v[i].x = -v[i].x;
	}
	if(drawmethod->flipy){
		for(i=0; i<4; i++) v[i].y = -v[i].y;
	}

	if(drawmethod->rotate){
		for(i=0; i<4; i++){
			tx = v[i].x; ty = v[i].y;
			v[i].x = (int)(tx * cos_table[drawmethod->rotate] - ty * sin_table[drawmethod->rotate])*drawmethod->scalex/256 + x;
			v[i].y = (int)(ty * cos_table[drawmethod->rotate] + tx * sin_table[drawmethod->rotate])*drawmethod->scaley/256 + y;
		}
	}else{
		for(i=0; i<4; i++){
			v[i].x = v[i].x*drawmethod->scalex/256 + x;
			v[i].y = v[i].y*drawmethod->scaley/256 + y;
		}


	}

	draw_triangle_list(v, screen, &gfx, drawmethod, 2);
}

void putsprite(int x, int y, s_sprite* sprite, s_screen* screen, s_drawmethod* drawmethod)
{
	if(drawmethod==NULL)
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

