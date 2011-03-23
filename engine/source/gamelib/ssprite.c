/*
	Scaled sprite code.
*/


#include "types.h"
#include "sprite.h"




static int screenwidth = 16;
static int screenheight = 16;


static int s_pos;
static int g_scale;
static int g_step;


// Returns 1 if end of screen was reached.
static int scale_span(char *dest, int x, char *src, int width){

	int ss_pos = s_pos & 255;

	if(x >= screenwidth) return 1;
	if(x < 0){
		// Start left of screen
		ss_pos -= (x*g_step);
		x = 0;
	}

	while(ss_pos/256<width && x<screenwidth){
		dest[x] = src[ss_pos/256];
		ss_pos += g_step;
		++x;
	}
	return x>=screenwidth;
}



// Dest = pointer to line in screen
static void scale_line(int x_pos, unsigned long * linedata, char *dest){
	unsigned long clearcount, viscount;

	if(x_pos/256 >= screenwidth) return;

	for(;;){
		clearcount = *linedata;
		linedata++;
		x_pos += clearcount * g_scale;
		s_pos += clearcount * g_step;

		viscount = *linedata;
		if(viscount<1) return;
		linedata++;
		if(scale_span(dest, x_pos/256, (void*)linedata, viscount)) return;

		x_pos += viscount * g_scale;
		s_pos += viscount * g_step;

		linedata += (viscount+3) / 4;
	}
}




void putsprite_scaled(int x, int y, s_sprite *frame, s_screen *screen, int scale){
	int top, left, bottom;
	unsigned long *linetab;
	unsigned long *linedata;
	int curline;
	int line_pos;
	// int pix_pos; unused variable, makes a warning
	char *destline;

	if(scale==256){
		putsprite(x, y, frame, screen);
		return;
	}

	if(scale <= 0) return;
	g_step = (256*256) / scale;
	if(g_step <= 0) return;
	g_scale = scale;

	// Get screen size
	screenwidth = screen->width;
	screenheight = screen->height;

	left = x*256 - frame->centerx*scale;
	top = y - ((frame->centery * scale)/256);
	bottom = top + ((frame->height * scale)/256);

	// Bottom = line _under_ sprite!
	if(top>=bottom) return;

	line_pos = g_step/2;
	if(top<0){
		line_pos = -top * g_step;
		top = 0;
	}
	if(bottom > screenheight){
		bottom = screenheight;
	}

	destline = screen->data + top*screenwidth;
	linetab = (void*)frame->data;
	do{
		// Get ready to draw a line
		curline = (line_pos/256);
		linedata = linetab + curline + (linetab[curline] / 4);
		line_pos += g_step;

		s_pos = g_step/2;

		scale_line(left, linedata, destline);
		destline += screenwidth;
	}while((++top) < bottom);
}


