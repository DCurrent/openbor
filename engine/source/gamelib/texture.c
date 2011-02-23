/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

// Some texture functions (nothing special)
// Last update: saturday, 10 jan 2004
// To do: optimize

#include <string.h>
#include "types.h"

//static int distortion[256];
float _sinfactors[256] = {1.0f,1.0245412285229123f,1.049067674327418f,1.0735645635996673f,1.0980171403295606f,1.1224106751992162f,1.1467304744553617f,1.1709618887603012f,1.1950903220161282f,1.2191012401568697f,1.2429801799032638f,1.2667127574748984f,1.2902846772544622f,1.3136817403988914f,1.33688985339222f,1.3598950365349882f,1.3826834323650898f,1.4052413140049897f,1.427555093430282f,1.4496113296546064f,1.4713967368259977f,1.492898192229784f,1.5141027441932215f,1.5349976198870971f,1.5555702330196021f,1.5758081914178454f,1.5956993044924332f,1.6152315905806267f,1.6343932841636454f,1.6531728429537766f,1.6715589548470184f,1.6895405447370668f,1.7071067811865474f,1.724247082951467f,1.7409511253549592f,1.7572088465064843f,1.7730104533627368f,1.7883464276266063f,1.8032075314806448f,1.8175848131515837f,1.8314696123025453f,1.8448535652497071f,1.8577286100002721f,1.8700869911087112f,1.881921264348355f,1.8932243011955152f,1.9039892931234434f,1.9142097557035307f,1.9238795325112867f,1.9329927988347388f,1.9415440651830207f,1.9495281805930366f,1.956940335732209f,1.96377606579544f,1.970031253194544f,1.9757021300385284f,1.9807852804032304f,1.985277642388941f,1.9891765099647811f,1.99247953459871f,1.9951847266721967f,1.9972904566786902f,1.9987954562051724f,1.9996988186962041f,2.0f,1.9996988186962041f,1.9987954562051724f,1.9972904566786902f,1.995184726672197f,1.99247953459871f,1.9891765099647811f,1.985277642388941f,1.9807852804032304f,1.9757021300385284f,1.970031253194544f,1.96377606579544f,1.956940335732209f,1.9495281805930366f,1.9415440651830207f,1.9329927988347388f,1.9238795325112867f,1.9142097557035307f,1.9039892931234434f,1.8932243011955152f,1.881921264348355f,1.8700869911087114f,1.8577286100002721f,1.8448535652497071f,1.8314696123025453f,1.8175848131515837f,1.8032075314806448f,1.7883464276266063f,1.773010453362737f,1.7572088465064848f,1.740951125354959f,1.724247082951467f,1.7071067811865474f,1.689540544737067f,1.6715589548470184f,1.6531728429537766f,1.6343932841636454f,1.615231590580627f,1.5956993044924334f,1.5758081914178454f,1.5555702330196021f,1.5349976198870971f,1.5141027441932217f,1.4928981922297841f,1.471396736825998f,1.449611329654607f,1.427555093430282f,1.40524131400499f,1.3826834323650898f,1.3598950365349882f,1.3368898533922202f,1.3136817403988914f,1.2902846772544625f,1.2667127574748984f,1.242980179903264f,1.2191012401568701f,1.1950903220161286f,1.1709618887603012f,1.146730474455362f,1.1224106751992164f,1.0980171403295608f,1.0735645635996677f,1.0490676743274178f,1.0245412285229123f,1.0000000000000002f,0.9754587714770879f,0.9509323256725822f,0.9264354364003325f,0.9019828596704394f,0.8775893248007839f,0.8532695255446384f,0.8290381112396991f,0.8049096779838716f,0.7808987598431302f,0.7570198200967362f,0.7332872425251018f,0.709715322745538f,0.6863182596011088f,0.6631101466077799f,0.6401049634650119f,0.6173165676349104f,0.5947586859950102f,0.5724449065697181f,0.5503886703453933f,0.5286032631740023f,0.5071018077702161f,0.48589725580677845f,0.46500238011290307f,0.44442976698039804f,0.42419180858215466f,0.40430069550756675f,0.3847684094193733f,0.36560671583635473f,0.34682715704622346f,0.32844104515298156f,0.31045945526293317f,0.29289321881345254f,0.2757529170485332f,0.2590488746450411f,0.24279115349351576f,0.22698954663726334f,0.2116535723733941f,0.19679246851935494f,0.18241518684841618f,0.16853038769745476f,0.155146434750293f,0.142271389999728f,0.12991300889128865f,0.11807873565164506f,0.10677569880448478f,0.09601070687655688f,0.08579024429646953f,0.07612046748871348f,0.06700720116526104f,0.058455934816979194f,0.050471819406963325f,0.043059664267791176f,0.03622393420456016f,0.029968746805456026f,0.02429786996147154f,0.01921471959676968f,0.01472235761105889f,0.010823490035219096f,0.007520465401289922f,0.004815273327803071f,0.002709543321309793f,0.001204543794827595f,0.00030118130379575003f,0.0f,0.00030118130379575003f,0.001204543794827595f,0.002709543321309793f,0.004815273327803071f,0.007520465401289922f,0.010823490035219096f,0.014722357611058778f,0.01921471959676957f,0.02429786996147143f,0.029968746805456026f,0.03622393420456005f,0.043059664267791065f,0.050471819406963214f,0.05845593481697908f,0.06700720116526093f,0.07612046748871337f,0.08579024429646942f,0.09601070687655666f,0.10677569880448467f,0.11807873565164495f,0.12991300889128854f,0.14227138999972777f,0.15514643475029277f,0.16853038769745454f,0.18241518684841595f,0.19679246851935472f,0.21165357237339388f,0.22698954663726312f,0.24279115349351543f,0.2590488746450409f,0.275752917048533f,0.2928932188134523f,0.31045945526293283f,0.32844104515298133f,0.3468271570462229f,0.36560671583635407f,0.3847684094193726f,0.40430069550756675f,0.4241918085821548f,0.4444297669803978f,0.46500238011290273f,0.4858972558067781f,0.5071018077702157f,0.5286032631740021f,0.5503886703453931f,0.5724449065697175f,0.5947586859950096f,0.6173165676349096f,0.640104963465012f,0.66311014660778f,0.6863182596011085f,0.7097153227455375f,0.7332872425251014f,0.7570198200967358f,0.7808987598431298f,0.8049096779838712f,0.8290381112396983f,0.8532695255446376f,0.877589324800784f,0.9019828596704394f,0.9264354364003325f,0.9509323256725819f,0.9754587714770876f};
float _amp = 1.0f;
#define distortion(x) ((int)(_sinfactors[x]*_amp+0.5))

// Fill the distortion table
void texture_set_wave(float amp){
    _amp = amp;
	//int i;
	//for(i=0;i<256;i++) distortion[i] = amp*(sin(i*M_PI/128.0)+1.0)+0.5;
    // _sinfactor = sin(i*M_PI/128.0)+1.0
}




/*

void texture_wave(s_screen *screen, int x, int y, int width, int height, int offsx, int offsy, s_bitmap *bitmap, int offsd, int step){

	int i;
	char *src;
	char *dest;
	int s;
	int sy;
	int xmask, ymask;
	int twidth;


	// Check dimensions
	if(x >= screen->width) return;
	if(y >= screen->height) return;
	if(x<0){
		width += x;
		x = 0;
	}
	if(y<0){
		height += y;
		y=0;
	}
	if(x+width > screen->width){
		width = screen->width - x;
	}
	if(y+height > screen->height){
		height = screen->height - y;
	}
	if(width<=0) return;
	if(height<=0) return;


	// Fill area
	xmask = bitmap->width-1;
	ymask = bitmap->height-1;

	sy = offsy;

	dest = screen->data + (y * screen->width) + x;
	do{
		// Get source line pointer
		sy &= ymask;
		src = bitmap->data + (sy*256);
		++sy;

		// Get start offset (distortion)
		offsd &= 255;
		s = offsx + distortion[offsd];
		offsd += step;

		// Copy pixels
		twidth = width;
		do{
			s &= xmask;
			*dest = src[s];
			++dest;
			++s;
		}while(--twidth);

		// Advance destination line pointer
		dest -= width;
		dest += screen->width;

	}while(--height);
}

*/



void texture_wave(s_screen *screen, int x, int y, int width, int height, int offsx, int offsy, s_bitmap *bitmap, int offsd, int step){

	unsigned char *src;
	unsigned char *dest;
	int s;
	int sy;
	int twidth;
	int tx;

	// Check dimensions
	if(x >= screen->width) return;
	if(y >= screen->height) return;
	if(x<0){
		width += x;
		x = 0;
	}
	if(y<0){
		height += y;
		y=0;
	}
	if(x+width > screen->width){
		width = screen->width - x;
	}
	if(y+height > screen->height){
		height = screen->height - y;
	}
	if(width<=0) return;
	if(height<=0) return;


	// Dest ptr
	dest = screen->data + ((y * screen->width) + x);

	// Fill area
	do{
		// Source line ptr
		sy = offsy % bitmap->height;
		src = bitmap->data + (sy * bitmap->width);
		offsy++;


		// Adjust distortion stuff
		offsd &= 255;
		s = (offsx + distortion(offsd)) % bitmap->width;
		offsd += step;

		// Copy loop
		tx = 0;
		twidth = bitmap->width - s;
		if(twidth > width) twidth = width;
		while(twidth > 0){
		    memcpy(dest+tx, src+s, twidth);
			s = 0;
			tx += twidth;
			twidth = width - tx;
			if(twidth > bitmap->width) twidth = bitmap->width;
		}

		// Advance destination line pointer
		dest += screen->width;
	}while(--height);

}





static void draw_plane_line(unsigned char *destline, unsigned char *srcline, int destlen, int srclen, int stretchto, int texture_offset){
	int i;
	unsigned int s, s_pos, s_step;
	int center_offset = destlen / 2;

	s_pos = texture_offset + (256 * srclen);
	s_step = srclen * 256 / stretchto;
	s_pos -= center_offset * s_step;

	for(i=0; i<destlen; i++){
		s = s_pos >> 8;
		if(s > srclen){
			s %= srclen;
			s_pos = (s_pos & 0xFF) | (s << 8);
		}
		destline[i] = srcline[s];
		s_pos += s_step;
	}
}



// Draw a plane (like the sea)
void texture_plane(s_screen *screen, int x, int y, int width, int height, int fixp_offs, int factor, s_bitmap *bitmap){

	int i;
	unsigned char *dest;
	unsigned char *src;
	int sy;

	if(factor < 0) return;
	factor++;


	// Check dimensions
	if(x >= screen->width) return;
	if(y >= screen->height) return;
	if(x<0){
		width += x;
		x = 0;
	}
	if(y<0){
		height += y;
		y=0;
	}
	if(x+width > screen->width){
		width = screen->width - x;
	}
	if(y+height > screen->height){
		height = screen->height - y;
	}
	if(width<=0) return;
	if(height<=0) return;


	dest = screen->data + ((y*screen->width) + x);
	sy = 0;
	for(i=0; i<height; i++)
    {
		sy = i % bitmap->height;
		src = bitmap->data + (sy * bitmap->width);

		draw_plane_line(dest,src, width,bitmap->width, bitmap->width + ((bitmap->width * i) / factor), fixp_offs);

		dest += screen->width;
	}
}

extern void texture_wavex8p16(s_screen *screen, int x, int y, int width, int height, int offsx, int offsy, s_bitmap *bitmap, int offsd, int step, unsigned short* pal16);
extern void texture_planex8p16(s_screen *screen, int x, int y, int width, int height, int fixp_offs, int factor, s_bitmap *bitmap, unsigned short* pal16);
extern void texture_wavex8p24(s_screen *screen, int x, int y, int width, int height, int offsx, int offsy, s_bitmap *bitmap, int offsd, int step, unsigned char* pal24);
extern void texture_planex8p24(s_screen *screen, int x, int y, int width, int height, int fixp_offs, int factor, s_bitmap *bitmap, unsigned char* pal24);
extern void texture_wavex8p32(s_screen *screen, int x, int y, int width, int height, int offsx, int offsy, s_bitmap *bitmap, int offsd, int step, unsigned* pal32);
extern void texture_planex8p32(s_screen *screen, int x, int y, int width, int height, int fixp_offs, int factor, s_bitmap *bitmap, unsigned* pal32);

void apply_texture_wave(s_screen *screen, int x, int y, int width, int height, int offsx, int offsy, s_bitmap *bitmap, int offsd, int step, s_drawmethod* drawmethod)
{
    unsigned char* table;
    if(drawmethod || drawmethod->flag==0) table = NULL;
    else table = drawmethod->table;

    switch(screen->pixelformat)
    {
    case PIXEL_8:
        texture_wave(screen, x, y, width, height, offsx, offsy, bitmap, offsd, step);
        break;
    case PIXEL_16:
        texture_wavex8p16(screen, x, y, width, height, offsx, offsy, bitmap, offsd, step, (unsigned short*)table);
        break;
    case PIXEL_32:
        texture_wavex8p32(screen, x, y, width, height, offsx, offsy, bitmap, offsd, step, (unsigned*)table);
        break;
    }
}

void apply_texture_plane(s_screen *screen, int x, int y, int width, int height, int fixp_offs, int factor, s_bitmap *bitmap, s_drawmethod* drawmethod)
{
    unsigned char* table;
    if(drawmethod || drawmethod->flag==0) table = NULL;
    else table = drawmethod->table;

    switch(screen->pixelformat)
    {
    case PIXEL_8:
        texture_plane(screen, x, y, width, height, fixp_offs, factor, bitmap);
        break;
    case PIXEL_16:
        texture_planex8p16(screen, x, y, width, height, fixp_offs, factor, bitmap, (unsigned short*)table);
        break;
    case PIXEL_32:
        texture_planex8p32(screen, x, y, width, height, fixp_offs, factor, bitmap, (unsigned*)table);
        break;
    }
}

