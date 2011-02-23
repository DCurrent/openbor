/*
	Code for handling 'screen' structures.
	(Memory allocation and copy functions)
	Last update: 10 feb 2003
*/
#include "ps2port.h"
#include "types.h"


s_screen * allocscreen(int width, int height){
	s_screen * screen;

	screen = (s_screen*)tracemalloc("allocscreen", sizeof(s_screen));
	if(screen==NULL) return NULL;

	width &= (0xFFFFFFFF-3);

	screen->data = (char*)tracemalloc("allocscreen", width*height);
	if(screen->data == NULL){
		tracefree(screen);
		return 0;
	}

	screen->width = width;
	screen->height = height;
	return screen;
}


void freescreen(s_screen * screen){
	if(screen){
		if(screen->data) tracefree(screen->data);
		tracefree(screen);
	}
}



// Screen copy func. Supports clipping.
void copyscreen(s_screen * dest, s_screen * src){
	char *sp = src->data;
	char *dp = dest->data;
	int width = src->width;
	int height = src->height;

	if(height > dest->height) height = dest->height;
	if(width > dest->width) width = dest->width;

	// Copy unclipped
	if(dest->width == src->width){
		memcpy(dest->data, src->data, width * height);
		return;
	}

	// Copy clipped
	do{
		memcpy(dp, sp, width);
		sp += src->width;
		dp += dest->width;
	}while(--height);
}



void clearscreen(s_screen * s){
	if(s == NULL) return;
	memset(s->data, 0, s->width*s->height);
}




// Screen copy function with offset options. Supports clipping.
void copyscreen_o(s_screen * dest, s_screen * src, int x, int y){
	char *sp = src->data;
	char *dp = dest->data;
	int sw = src->width;
	int sh = src->height;
	int dw = dest->width;
	int dh = dest->height;
	int cw = sw, ch = sh;
	int sox, soy;

	// Copy anything at all?
	if(x >= dw) return;
	if(sw+x <= 0) return;
	if(y >= dh) return;
	if(sh+y <= 0) return;

	sox = 0;
	soy = 0;

	// Clip?
	if(x<0){
		sox = -x;
		cw += x;
	}
	if(y<0){
		soy = -y;
		ch += y;
	}

	if(x+sw > dw){
		cw -= (x+sw) - dw;
	}
	if(y+sh > dh){
		ch -= (y+sh) - dh;
	}


	if(x<0) x = 0;
	if(y<0) y = 0;

	sp += soy*sw + sox;
	dp += y*dw + x;

	// Copy data
	do{
		memcpy(dp, sp, cw);
		sp += sw;
		dp += dw;
	}while(--ch);
}





// Scale screen
void scalescreen(s_screen * dest, s_screen * src){
	int sw, sh;
	int dw, dh;
//	int sx, sy;
	int dx, dy;
	char *sp;
	char *dp;
	char *lineptr;
	unsigned gamelib_long xstep, ystep, xpos, ypos;

	if(src==NULL || dest==NULL) return;
	sp = src->data;
	dp = dest->data;

	sw = src->width;
	sh = src->height;
	dw = dest->width;
	dh = dest->height;

	xstep = (sw<<16) / dw;
	ystep = (sh<<16) / dh;

	ypos = 0;
	for(dy=0; dy<dh; dy++){
		lineptr = sp + ((ypos>>16) * sw);
		ypos += ystep;
		xpos = 0;
		for(dx=0; dx<dw; dx++){
			*dp = lineptr[xpos>>16];
			++dp;
			xpos += xstep;
		}
	}
}



