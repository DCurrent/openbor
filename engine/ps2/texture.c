// Some texture functions (nothing special)
// Last update: saturday, 10 jan 2004
// To do: optimize


#include "ps2port.h"

#include "types.h"
#include "math.h"


//#define		M_PI		3.1415926


static int distortion[256];



// Fill the distortion table
void texture_set_wave(float amp){
	int i;
	for(i=0;i<256;i++) distortion[i] = amp*(sin(i*M_PI/128)+1)+0.5;
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

//	int i;
	char *src;
	char *dest;
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
	dest = screen->data + (y * screen->width) + x;

	// Fill area
	do{
		// Source line ptr
		sy = offsy % bitmap->height;
		src = bitmap->data + (sy * bitmap->width);
		offsy++;


		// Adjust distortion stuff
		offsd &= 255;
		s = (offsx + distortion[offsd]) % bitmap->width;
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








static void draw_plane_line(char *destline, char *srcline, int destlen, int srclen, int stretchto, int texture_offset){
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
	char *dest;
	char *src;
	int sy;
//	int cur_fixp_step;
//	int cur_fixp_offs;

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
	

	dest = screen->data + (y*screen->width) + x;
	sy = 0;
	for(i=0; i<height; i++){

		sy = i % bitmap->height;
		src = bitmap->data + (sy * bitmap->width);

		draw_plane_line(dest,src, width,bitmap->width, bitmap->width + ((bitmap->width * i) / factor), fixp_offs);

		dest += screen->width;
	}
}





