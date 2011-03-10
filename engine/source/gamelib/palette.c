/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

/*
	Code to fiddle about with palettes.

	This file features code for:
	palette search for RGB values
	creation of lookup tables

	Lookup tables must be formatted so that a single foreground colour
	can be used to find an array of background colours:
	new = lookup[(fg<<8)+bg]
	bg_array = lookup + (fg<<8);


	Last update: 27-jan-2003
*/

#include "tracemalloc.h"

#include "vga.h"
#include "palette.h"


#ifndef NULL
#define NULL 0L
#endif


// Basic bamma correction: ((v*(255+(((255-v)*(g-255))/255)))/255)
//			= ((v*(65025+((255-v)*(g-255))))/65025)
// Basic brightness correction: (b+((v*(255-b))/255))

// Gamma correction, range -255 to 255:
// (g<=0?((v*(65025+((255-v)*g)))/65025):(255-(((255-v)*(65025+(v*-g)))/65025)))

// Brightness correction, same range:
// b<0 ? ((v*(255+b))/255) : (b+((v*(255-b))/255))


#define		gammacorrect(v,g)	(g<=0?((v*(65025+((255-v)*g)))/65025):(255-(((255-v)*(65025+(v*-g)))/65025)))
#define		brightnesscorrect(v,b)	(b<0?((v*(255+b))/255):(b+((v*(255-b))/255)))
#define		gbcorrect(vx,gx,bx)	(gammacorrect(brightnesscorrect(vx,bx),gx))


// Set gamma/brightness corrected palette.
// Valid values range between -255 and 255, where 0 is normal.
void palette_set_corrected(unsigned char *pal, int gr, int gg, int gb, int br, int bg, int bb){
	unsigned char pal2[768];
	int i;

	if(gr<-255) gr = -255;
	else if(gr>255) gr = 255;
	if(gg<-255) gg = -255;
	else if(gg>255) gg = 255;
	if(gb<-255) gb = -255;
	else if(gb>255) gb = 255;

	if(br<-255) br = -255;
	else if(br>255) br = 255;
	if(bg<-255) bg = -255;
	else if(bg>255) bg = 255;
	if(bb<-255) bb = -255;
	else if(bb>255) bb = 255;

	for(i=0; i<256; i++){
		pal2[i*3] =   gbcorrect(pal[i*3],   gr, br);
		pal2[i*3+1] = gbcorrect(pal[i*3+1], gg, bg);
		pal2[i*3+2] = gbcorrect(pal[i*3+2], gb, bb);
	}
	vga_setpalette(pal2);
}





// Search the palette for a colour most like the RGB values specified.
int palette_find(unsigned char *pal, int r, int g, int b){
	int i,j;
	int diff, olddiff, nearindex = 0;
	int diffred, diffgreen, diffblue;

	olddiff = 1000000;	// Set olddiff to an impossibly high value

	for(i=0,j=0; j<256; j++){
		if((diffred = r-pal[i])<0) diffred = -diffred;
		++i;
		if((diffgreen = g-pal[i])<0) diffgreen = -diffgreen;
		++i;
		if((diffblue = b-pal[i])<0) diffblue = -diffblue;
		++i;

		diff = diffred+diffgreen+diffblue;

		if(diff<olddiff){
			nearindex = j;
			olddiff = diff;
		}
	}

	return nearindex;
}





#define		multiply(a,b)		(a*b/255)
#define		screen(a,b)		(((a^255)*(b^255)/255)^255)
#define		hardlight(fg,bg)	(fg<128?multiply(fg*2,bg):screen((fg-128)*2,bg))
#define		overlay(fg,bg)		(bg<128?multiply(bg*2,fg):screen((bg-128)*2,fg))





// Create a lookup table used for shadows (multiply in photoshop)
unsigned char * palette_table_multiply(unsigned char *pal){
	int fg, bg;
	int red, green, blue;
	unsigned char * lut;

	if(pal==NULL) return NULL;

	lut = (unsigned char*)tracemalloc("palette_table_multiply", 256*256);
	if(lut==NULL) return NULL;

	for(fg=0; fg<256; fg++){
		for(bg=fg; bg<256; bg++){
			red = pal[bg*3] * pal[fg*3] / 255;
			green = pal[bg*3+1] * pal[fg*3+1] / 255;
			blue = pal[bg*3+2] * pal[fg*3+2] / 255;

			lut[(fg<<8)+bg] = palette_find(pal, red, green, blue);
			lut[(bg<<8)+fg] = lut[(fg<<8)+bg];
		}
	}
	return lut;
}





// Create a lookup table for 'screen'
unsigned char * palette_table_screen(unsigned char *pal){
	int fg, bg;
	int red, green, blue;
	unsigned char * lut;

	if(pal==NULL) return NULL;

	lut = (unsigned char*)tracemalloc("palette_table_screen", 256*256);
	if(lut==NULL) return NULL;

	for(fg=0; fg<256; fg++){
		for(bg=fg; bg<256; bg++){
			red =   ((pal[fg*3] * (255 - pal[bg*3])) / 255) + pal[bg*3];
			green = ((pal[fg*3+1] * (255 - pal[bg*3+1])) / 255) + pal[bg*3+1];
			blue =  ((pal[fg*3+2] * (255 - pal[bg*3+2])) / 255) + pal[bg*3+2];

			lut[(fg<<8)+bg] = palette_find(pal, red, green, blue);
			lut[(bg<<8)+fg] = lut[(fg<<8)+bg];
		}
	}
	return lut;
}





// Create "overlay" lookup table (multiply and screen combined)
unsigned char * palette_table_overlay(unsigned char *pal){
	int fg, bg;
	int red, green, blue;
	unsigned char * lut;

	if(pal==NULL) return NULL;

	lut = (unsigned char*)tracemalloc("palette_table_overlay", 256*256);
	if(lut==NULL) return NULL;

	for(fg=0; fg<256; fg++){
		for(bg=fg; bg<256; bg++){
			red = overlay(pal[bg*3], pal[fg*3]);
			green = overlay(pal[bg*3+1], pal[fg*3+1]);
			blue = overlay(pal[bg*3+2], pal[fg*3+2]);

			lut[(fg<<8)+bg] = palette_find(pal, red, green, blue);
			lut[(bg<<8)+fg] = lut[(fg<<8)+bg];
		}
	}
	return lut;
}


// Create "hard light" lookup table (multiply and screen combined)
unsigned char * palette_table_hardlight(unsigned char *pal){
	int fg, bg;
	int red, green, blue;
	unsigned char * lut;

	if(pal==NULL) return NULL;

	lut = (unsigned char*)tracemalloc("palette_table_hardlight", 256*256);
	if(lut==NULL) return NULL;

	for(fg=0; fg<256; fg++){
		for(bg=fg; bg<256; bg++){
			red = hardlight(pal[bg*3], pal[fg*3]);
			green = hardlight(pal[bg*3+1], pal[fg*3+1]);
			blue = hardlight(pal[bg*3+2], pal[fg*3+2]);

			lut[(fg<<8)+bg] = palette_find(pal, red, green, blue);
			lut[(bg<<8)+fg] = lut[(fg<<8)+bg];
		}
	}
	return lut;
}



// Create a lookup table for "dodge".
// Very nice for a colourful boost of light.
unsigned char * palette_table_dodge(unsigned char *pal){
	int fg, bg;
	int c1, c2;
	int t1, t2;
	int red, green, blue;
	unsigned char * lut;

	if(pal==NULL) return NULL;

	lut = (unsigned char*)tracemalloc("palette_table_dodgs", 256*256);
	if(lut==NULL) return NULL;


	for(fg=0, c2=0; fg<256; fg++, c2+=3){
		for(bg=0, c1=0; bg<256; bg++){
			t2 = pal[c2];
			t1 = pal[c1++];
			red = (t1 * 256) / (256 - t2);
			if(red>255) red = 255;

			t2 = pal[c2+1];
			t1 = pal[c1++];
			green = (t1 * 256) / (256 - t2);
			if(green>255) green = 255;

			t2 = pal[c2+2];
			t1 = pal[c1++];
			blue = (t1 * 256) / (256 - t2);
			if(blue>255) blue = 255;

			lut[(fg<<8)+bg] = palette_find(pal, red, green, blue);
		}
	}
	return lut;
}





// Create a lookup table for 50% opacity
unsigned char * palette_table_half(unsigned char *pal){
	int fg, bg;
	int red, green, blue;
	unsigned char * lut;
	int dither = 0;

	if(pal==NULL) return NULL;

	lut = (unsigned char*)tracemalloc("palette_table_half", 256*256);
	if(lut==NULL) return NULL;

	for(fg=0; fg<256; fg++){
		for(bg=fg; bg<256; bg++){
			red = (pal[bg*3] + pal[fg*3] + (++dither&1)) / 2;
			green = (pal[bg*3+1] + pal[fg*3+1] + (++dither&1)) / 2;
			blue = (pal[bg*3+2] + pal[fg*3+2] + (++dither&1)) / 2;

			lut[(fg<<8)+bg] = palette_find(pal, red, green, blue);
			lut[(bg<<8)+fg] = lut[(fg<<8)+bg];
		}
	}
	return lut;
}
