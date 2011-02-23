// Primitive drawing functions. Should really be done in ASM...
// Last update: 24-jun-2002

#include "ps2port.h"

#include "types.h"

#ifndef		NULL
#define		NULL	 ((void*)0)
#endif


#define		abso(x)		(x<0?-x:x)


// Not a particularly fast line function, but it works, and clips!
void line(int sx, int sy, int ex, int ey, int colour, s_screen *screen){

	int diffx, diffy;
	int absdiffx, absdiffy;
	int xdir, ydir;
	int thres;


	// Some off-screen lines may slip through this test!
	if(sx<0 && ex<0) return;
	if(sy<0 && ey<0) return;
	if(sx>=screen->width && ex>=screen->width) return;
	if(sy>=screen->height && ey>=screen->height) return;


	// Check clipping and calculate new coords if necessary

	diffx = ex - sx;
	diffy = ey - sy;

	if(sx<0){
		sy -= (sx*diffy/diffx);
		sx = 0;
	}
	if(sy<0){
		sx -= (sy*diffx/diffy);
		sy = 0;
	}
	if(sx>=screen->width){
		sy -= ((sx-screen->width)*diffy/diffx);
		sx = screen->width-1;
	}
	if(sy>=screen->height){
		sx -= ((sy-screen->height)*diffx/diffy);
		sy = screen->height-1;
	}

	if(ex<0){
		ey -= (ex*diffy/diffx);
		ex = 0;
	}
	if(ey<0){
		ex -= (ey*diffx/diffy);
		ey = 0;
	}
	if(ex>=screen->width){
		ey -= ((ex-screen->width)*diffy/diffx);
		ex = screen->width-1;
	}
	if(ey>=screen->height){
		ex -= ((ey-screen->height)*diffx/diffy);
		ey = screen->height-1;
	}


	// Second test: the lines that passed test 1 won't pass this time!
	if(sx<0 || ex<0) return;
	if(sy<0 || ey<0) return;
	if(sx>=screen->width || ex>=screen->width) return;
	if(sy>=screen->height || ey>=screen->height) return;


	// Recalculate directions
	diffx = ex - sx;
	diffy = ey - sy;

	absdiffx = abso(diffx);
	absdiffy = abso(diffy);

	sy *= screen->width;
	ey *= screen->width;


	if(absdiffx > absdiffy){
		// Draw a flat line
		thres = absdiffx >> 1;
		xdir = 1;
		if(diffx<0) xdir = -xdir;
		ydir = screen->width;
		if(diffy<0) ydir = -ydir;
		while(sx!=ex){
			screen->data[sx+sy] = colour;
			sx += xdir;
			if((thres-=absdiffy) <= 0){
				sy += ydir;
				thres += absdiffx;
			}
		}
		screen->data[ex+ey] = colour;
		return;
	}

	// Draw a high line
	thres = absdiffy >> 1;
	xdir = 1;
	if(diffx<0) xdir = -1;
	ydir = screen->width;
	if(diffy<0) ydir = -ydir;
	while(sy!=ey){
		screen->data[sx+sy] = colour;
		sy += ydir;
		if((thres-=absdiffx) <= 0){
			sx += xdir;
			thres += absdiffy;
		}
	}
	screen->data[ex+ey] = colour;
}





void drawbox(int x, int y, int width, int height, char colour, s_screen *screen){
	char *cp;

	if(width<=0) return;
	if(height<=0) return;
	if(screen==NULL) return;

	if(x<0){
		if((width+=x)<=0) return;
		x = 0;
	}
	else if(x>=screen->width) return;
	if(y<0){
		if((height+=y)<=0) return;
		y = 0;
	}
	else if(y>=screen->height) return;
	if(x+width>screen->width) width = screen->width-x;
	if(y+height>screen->height) height = screen->height-y;

	cp = screen->data + y*screen->width + x;
	while(--height>=0){
		for(x=0;x<width;x++){
			*cp = colour;
			++cp;
		}
		cp += screen->width - width;
	}
}




void drawbox_trans(int x, int y, int width, int height, char colour, s_screen *screen, char *lut){
	char *cp;

	if(width<=0) return;
	if(height<=0) return;
	if(screen==NULL) return;
	if(lut==NULL) return;

	lut += (colour<<8);

	if(x<0){
		if((width+=x)<=0) return;
		x = 0;
	}
	else if(x>=screen->width) return;
	if(y<0){
		if((height+=y)<=0) return;
		y = 0;
	}
	else if(y>=screen->height) return;
	if(x+width>screen->width) width = screen->width-x;
	if(y+height>screen->height) height = screen->height-y;

	cp = screen->data + y*screen->width + x;
	while(--height>=0){
		for(x=0;x<width;x++){
			*cp = lut[((int)(*cp)) & 0xFF];
			++cp;
		}
		cp += screen->width - width;
	}
}




// Putpixel used by circle function
void putpixel(unsigned x, unsigned y, char colour, s_screen *screen){
	if(x>screen->width || y>screen->height) return;
	screen->data[x+y*screen->width] = colour;
}




// Code to draw a circle.
// I ripped this, not sure how it works...
// It seems it devides the circle into 8 parts, which are drawn
// simultaneously.
// Not much optimization, though, since every pixel is clipped
// separately.

void circle(int x, int y, int rad, char col, s_screen *screen){

	int cx = 0;				// 'Circle X'
	int cy = rad;				// 'Circle Y'
	int df = 1 - rad;
	int d_e = 3;
	int d_se = -2 * rad + 5;

	do{
		putpixel(x+cx, y+cy, col, screen);
		if(cx) putpixel(x-cx, y+cy, col, screen);
		if(cy) putpixel(x+cx, y-cy, col, screen);
		if(cx && cy) putpixel(x-cx, y-cy, col, screen);

		if(cx != cy){
			putpixel(x+cy, y+cx, col, screen);
			if(cx) putpixel(x+cy, y-cx, col, screen);
			if(cy) putpixel(x-cy, y+cx, col, screen);
			if(cx && cy) putpixel(x-cy, y-cx, col, screen);
		}

		if(df<0){
			df   += d_e;
			d_e  += 2;
			d_se += 2;
		}
		else{
			df   += d_se;
			d_e  += 2;
			d_se += 4;
			--cy;
		}
		++cx;
	}while(cx<=cy);
}








