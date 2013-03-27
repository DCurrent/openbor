/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

///////////////////////////////////////////////////////////////////////////
//         This file defines some commmon methods used by the gamelib
////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include "globals.h"
#include "types.h"
#include "transform.h"

static transpixelfunc pfp;
static unsigned int fillcolor;
static blend16fp pfp16;
static blend32fp pfp32;
static unsigned char* table;
static unsigned char* remaptable;
static int transbg;
int trans_sw, trans_sh, trans_dw, trans_dh;
static void (*drawfp)(s_screen* dest, gfx_entry* src, int dx, int dy, int sx, int sy);

static s_screen* handle_dest;

static int y_dest;
static int x_dest;
static int span_dest;

static unsigned char* ptr_dest;
static unsigned char* cur_dest;

static gfx_entry* handle_src;

static int y_src;
static int x_src;
static int span_src;

static unsigned char* ptr_src;
static unsigned char* cur_src;
static unsigned char* cur_spr; // for sprite only

static int spf, dpf; //pixelformat

static int xmin, xmax, ymin, ymax;


/*transpixelfunc, 8bit*/
static unsigned char remapcolor(unsigned char* unusedt, unsigned char color, unsigned char unused)
{
	return remaptable[color];
}

static unsigned char blendcolor(unsigned char* t, unsigned char color1, unsigned char color2)
{
	if(remaptable) color1 = remaptable[color1];
	if(!t) return color1;
	return t[color1<<8|color2];
}

static unsigned char blendfillcolor(unsigned char* t, unsigned char unused, unsigned char color)
{
	if(!t) return fillcolor;
	return t[fillcolor<<8|color];
}


/**

 draw a pixel from source gfx surface to destination screen
 complex

*/

void draw_pixel_dummy(s_screen* dest, gfx_entry* src, int dx, int dy, int sx, int sy)
{
	int pb = pixelbytes[(int)dest->pixelformat];
	unsigned char* pd = ((unsigned char*)(dest->data)) + (dx + dy*dest->width)*pb; 
	memset(pd, 0, pb);
}

void draw_pixel_screen(s_screen* dest, gfx_entry* src, int dx, int dy, int sx, int sy)
{
	unsigned char *ptrd8, *ptrs8, ps8;
	unsigned short *ptrd16, *ptrs16, pd16, ps16;
	unsigned int *ptrd32, *ptrs32, pd32, ps32;
	switch(dest->pixelformat)
	{
		case PIXEL_8:
			ps8 = *(((unsigned char*)src->screen->data) + sx + sy * src->screen->width);
			if(transbg && !ps8) return;
			else if(fillcolor) ps8 = fillcolor;
			ptrd8 = ((unsigned char*)dest->data) + dx + dy * dest->width;
			*ptrd8 =pfp?pfp(table, ps8, *ptrd8):ps8;
			break;
		case PIXEL_16:
			ptrd16 = ((unsigned short*)dest->data) + dx + dy * dest->width;
			pd16 = *ptrd16;
			switch(src->screen->pixelformat)
			{
			case PIXEL_16:
				ptrs16 = ((unsigned short*)src->screen->data) + sx + sy * src->screen->width;
				ps16 = *ptrs16;
				if(transbg && !ps16) return;
				break;
			case PIXEL_x8:
				ptrs8 = ((unsigned char*)src->screen->data) + sx + sy * src->screen->width;
				if(transbg && !*ptrs8) return;
				ps16 = table?((unsigned short*)table)[*ptrs8]:((unsigned short*)src->screen->palette)[*ptrs8];
				break;
			default:
				return;
			}
			if(fillcolor) ps16 = fillcolor;
			if(!pfp16) *ptrd16 = ps16;
			else       *ptrd16 = pfp16(ps16, pd16);
			break;
		case PIXEL_32:
			ptrd32 = ((unsigned int*)dest->data) + dx + dy * dest->width;
			pd32 = *ptrd32;
			switch(src->screen->pixelformat)
			{
			case PIXEL_32:
				ptrs32 = ((unsigned int*)src->screen->data) + sx + sy * src->screen->width;
				ps32 = *ptrs32;
				if(transbg && !ps32) return;
				break;
			case PIXEL_x8:
				ptrs8 = ((unsigned char*)src->screen->data) + sx + sy * src->screen->width;
				if(transbg && !*ptrs8) return;
				ps32 = table?((unsigned int*)table)[*ptrs8]:((unsigned int*)src->screen->palette)[*ptrs8];
				break;
			default:
				return;
			}
			if(fillcolor) ps32 = fillcolor;
			if(!pfp32) *ptrd32 = ps32;
			else       *ptrd32 = pfp32(ps32, pd32);
			break;

	}
}


void draw_pixel_bitmap(s_screen* dest, gfx_entry* src, int dx, int dy, int sx, int sy)
{
	//stub
	// should be OK for now since s_screen and s_bitmap are identical to each other
	assert(sizeof(s_screen)!=sizeof(s_bitmap));
	draw_pixel_screen(dest, src, dx, dy, sx, sy);
}

// get a pixel from specific sprite
// should be fairly slow due to the RLE compression
char sprite_get_pixel(s_sprite* sprite, int x, int y){
	int *linetab;
	register int lx = 0, count;
	unsigned char * data;

	//should we check? 
	//if(y<0 || y>=sprite->height || x<0 || x>=sprite->width)
	//	return 0;


	linetab = ((int*)sprite->data) + y;

	data = ((unsigned char*)linetab) + (*linetab);

	while(1) {
		count = *data++;
		if(count == 0xFF) return 0;
		if(lx+count>x) return 0; // transparent pixel
		lx += count;
		count = *data++;
		if(!count) continue;
		if(lx + count > x)
		{
			return data[x-lx]; // not transparent pixel
		}
		lx+=count;
		data+=count;
	}

	return 0;

}

void draw_pixel_sprite(s_screen* dest, gfx_entry* src, int dx, int dy, int sx, int sy)
{
	unsigned char *ptrd8, ps8;
	unsigned short *ptrd16, pd16, ps16;
	unsigned int *ptrd32, pd32, ps32;
	switch(dest->pixelformat)
	{
		case PIXEL_8:
			if(!(ps8 = sprite_get_pixel(src->sprite, sx, sy)))
				return;
			ptrd8 = ((unsigned char*)dest->data) + dx + dy * dest->width;
			if(fillcolor) ps8 = fillcolor;
			*ptrd8 =pfp?pfp(table, ps8, *ptrd8):ps8;
			break;
		case PIXEL_16:
			if(!(ps8 = sprite_get_pixel(src->sprite, sx, sy)))
				return;
			ptrd16 = ((unsigned short*)dest->data) + dx + dy * dest->width;
			pd16 = *ptrd16;
			if(fillcolor) ps16 = fillcolor;
			else ps16 = table?((unsigned short*)table)[ps8]:((unsigned short*)src->sprite->palette)[ps8];
			if(!pfp16) *ptrd16 = ps16;
			else       *ptrd16 = pfp16(ps16, pd16);
			break;
		case PIXEL_32:
			if(!(ps8 = sprite_get_pixel(src->sprite, sx, sy)))
				return;
			ptrd32 = ((unsigned int*)dest->data) + dx + dy * dest->width;
			pd32 = *ptrd32;
			if(fillcolor) ps32 = fillcolor;
			else ps32 = table?((unsigned int*)table)[ps8]:((unsigned int*)src->sprite->palette)[ps8];
			if(!pfp32) *ptrd32 = ps32;
			else       *ptrd32 = pfp32(ps32, pd32);
			break;
	}

}

void draw_pixel_gfx(s_screen* dest, gfx_entry* src, int dx, int dy, int sx, int sy){
	//drawfp(dest, src, dx, dy, sx, sy);
	switch(src->screen->magic){
	case sprite_magic:
		draw_pixel_sprite(dest, src, dx, dy, sx, sy);
		break;
	case screen_magic:
		draw_pixel_screen(dest, src, dx, dy, sx, sy);
		break;
	case bitmap_magic:
		draw_pixel_bitmap(dest, src, dx, dy, sx, sy);
		break;
	default:
		draw_pixel_dummy(dest, src, dx, dy, sx, sy);//debug purpose
		break;
	}
}

void copy_pixel_block(int bytes){
	memcpy(cur_dest, cur_src, bytes);
}

void write_pixel(){
	unsigned char ps8;
	unsigned short ps16;
	unsigned ps32;
	if(!cur_src) return;
	switch(dpf)
	{
		case PIXEL_8:
			ps8 = *cur_src;
			if(transbg && !ps8) return;
			else if(fillcolor) ps8 = fillcolor;
			*cur_dest =pfp?pfp(table, ps8, *cur_dest):ps8;
			break;
		case PIXEL_16:
			switch(spf)
			{
			case PIXEL_16:
				ps16 = *(unsigned short*)cur_src;
				if(transbg && !ps16) return;
				break;
			case PIXEL_x8:
				if(!(ps8=*cur_src) && transbg) return;
				ps16 = ((unsigned short*)table)[ps8];
				break;
			default:
				return;
			}
			if(fillcolor) ps16 = fillcolor;
			if(!pfp16) *(unsigned short*)cur_dest = ps16;
			else       *(unsigned short*)cur_dest = pfp16(ps16, *(unsigned short*)cur_dest);
			break;
		case PIXEL_32:
			switch(spf)
			{
			case PIXEL_32:
				ps32 = *(unsigned*)cur_src;
				if(transbg && !ps32) return;
				break;
			case PIXEL_x8:
				if(!(ps8=*cur_src) && transbg) return;
				ps32 = ((unsigned*)table)[ps8];
				break;
			default:
				return;
			}
			if(fillcolor) ps32 = fillcolor;
			if(!pfp32) *(unsigned*)cur_dest = ps32;
			else       *(unsigned*)cur_dest = pfp32(ps32, *(unsigned*)cur_dest);
			break;
		default:
			break;
	}

}

void dest_seek(int x, int y){
	//x_dest = x; y_dest = y;
	cur_dest = ptr_dest + (y * trans_dw + x)*pixelbytes[dpf];	
}

void dest_line_inc(){
	//y_dest++;
	cur_dest += span_dest;
}

void dest_line_dec(){
	//y_dest--;
	cur_dest -= span_dest;
}

//should be within a line
void dest_inc(){
	//x_dest++;
	cur_dest  += pixelbytes[dpf];
}

//should be within a line
void dest_dec(){
	//x_dest--;
	cur_dest -= pixelbytes[dpf];
}

void _sprite_seek(int x, int y){
	int* linetab;
	unsigned char* data = NULL;
	register int lx = 0, count;

	linetab = ((int*)ptr_src) + y;

	data = ((unsigned char*)linetab) + (*linetab);

	while(1) {
		count = *data++;
		if(count == 0xFF) {
			cur_src = NULL;
			goto quit;
		}
		if(lx+count>x) { // transparent pixel
			cur_src = NULL;
			goto quit;
		}
		lx += count;
		count = *data++;
		if(!count) continue;
		if(lx + count > x){
			cur_src = data + x - lx;
			goto quit;
		}
		lx+=count;
		data+=count;
	}

quit:
	cur_spr = data-1; // current block head
	return ;

}

void src_seek(int x, int y){
	switch(handle_src->screen->magic){
	case sprite_magic:
		x_src = x; y_src = y;
		_sprite_seek(x,y);
		break;
	case screen_magic:
	case bitmap_magic:
		cur_src = ptr_src + (y * trans_sw + x)*pixelbytes[spf];
		break;
	default:
		break;
	}
}

void src_line_inc(){
	switch(handle_src->screen->magic){
	case sprite_magic:
		y_src++;
		_sprite_seek(x_src, y_src);
		break;
	case screen_magic:
	case bitmap_magic:
		cur_src += span_src;
		break;
	default:
		break;
	}
}

void src_line_dec(){
	switch(handle_src->screen->magic){
	case sprite_magic:
		y_src--;
		_sprite_seek(x_src, y_src);
		break;
	case screen_magic:
	case bitmap_magic:
		cur_src -= span_src;
		break;
	default:
		break;
	}
}

//should be within a line
void src_inc(){
	//int cnt;
	x_src++;
	switch(handle_src->screen->magic){
	case sprite_magic:
		//_sprite_seek(x,y);
		if(cur_src && cur_spr + *cur_spr > cur_src){
			cur_src++;
		}else _sprite_seek(x_src, y_src);
		break;
	case screen_magic:
	case bitmap_magic:
		cur_src += pixelbytes[spf];
		break;
	default:
		break;
	}
}

//should be within a line
void src_dec(){
	x_src--;
	switch(handle_src->screen->magic){
	case sprite_magic:
		//_sprite_seek(x,y);
		if(cur_src && cur_spr + 1 < cur_src){
			cur_src--;
		}else _sprite_seek(x_src, y_src);
		break;
	case screen_magic:
	case bitmap_magic:
		cur_src -= pixelbytes[spf];
		break;
	default:
		break;
	}
}

void init_gfx_global_draw_stuff(s_screen* dest, gfx_entry* src, s_drawmethod* drawmethod){

	spf = dpf = 0; //source pixel format
	drawfp = draw_pixel_dummy;
	pfp = NULL; fillcolor = 0; pfp16 = NULL;pfp32 = NULL; table = NULL; transbg = 0;

	handle_dest = dest; handle_src = src;
	cur_dest = ptr_dest = (unsigned char*)dest->data;
	x_dest = y_dest = x_src = y_src = 0;

	//nasty checkings due to those different pixel formats
	switch(src->screen->magic)
	{
	case screen_magic:
		//printf("gfx_screen\n");
		spf = src->screen->pixelformat;
		drawfp = draw_pixel_screen;
		trans_sw = src->screen->width;
		trans_sh = src->screen->height;
		cur_src = ptr_src = (unsigned char*)src->screen->data;
		table = drawmethod->table?drawmethod->table:src->screen->palette;
		break;
	case bitmap_magic:
		//printf("gfx_bitmap\n");
		spf = src->bitmap->pixelformat;
		drawfp = draw_pixel_bitmap;
		trans_sw = src->bitmap->width;
		trans_sh = src->bitmap->height;
		cur_src = ptr_src = (unsigned char*)src->bitmap->data;
		table = drawmethod->table?drawmethod->table:src->bitmap->palette;
		break;
	case sprite_magic:
		//printf("gfx_sprite\n");
		spf = src->sprite->pixelformat;
		drawfp = draw_pixel_sprite;
		trans_sw = src->sprite->width;
		trans_sh = src->sprite->height;
		if(trans_sw){
			ptr_src = (unsigned char*)src->sprite->data;
			cur_spr = ptr_src + (*(int*)ptr_src);
		}
		table = drawmethod->table?drawmethod->table:src->sprite->palette;
		break;
	default:
		//printf("gfx_unknown\n");
		return;
	}

	trans_dw = dest->width;
	trans_dh = dest->height;

	xmin=useclip?clipx1:0;
	xmax=useclip?clipx2:dest->width;
	ymin=useclip?clipy1:0;
	ymax=useclip?clipy2:dest->height;

	dpf = dest->pixelformat;

	switch(dest->pixelformat)
	{
	case PIXEL_8:
		if(drawmethod->fillcolor) fillcolor = drawmethod->fillcolor&0xFF;
		else fillcolor = 0;

		table = NULL;
		remaptable = NULL;

		if(drawmethod->alpha>0)
		{
			table = blendtables[drawmethod->alpha-1];
			remaptable = drawmethod->table;
			pfp = (fillcolor==TRANSPARENT_IDX?blendcolor:blendfillcolor);
		}
		else if(drawmethod->table)
		{
			remaptable = drawmethod->table;
			pfp = (fillcolor==TRANSPARENT_IDX?remapcolor:blendfillcolor);
		}
		else pfp = (fillcolor==TRANSPARENT_IDX?NULL:blendfillcolor);
		break;
	case PIXEL_16:
		fillcolor = drawmethod->fillcolor;
		pfp16 = getblendfunction16(drawmethod->alpha);
		break;
	case PIXEL_32:
		fillcolor = drawmethod->fillcolor;
		pfp32 = getblendfunction32(drawmethod->alpha);
		break;
	default: 
		return;
	}

	span_src = pixelbytes[spf]*trans_sw;
	span_dest = pixelbytes[dpf]*trans_dw;
	transbg = (drawmethod->transbg || src->screen->magic==sprite_magic); // check color key, we'll need this for screen and bitmap

	if(!trans_sw) return;
	src_seek(0,0);
}

void gfx_draw_rotate(s_screen* dest, gfx_entry* src, int x, int y, int centerx, int centery, s_drawmethod* drawmethod)
{
	float zoomx, zoomy, rzoomx, rzoomy, sina, cosa, ax, ay, bx, by, rx0, ry0, cx, cy, srcx0_f, srcx_f, srcy0_f, srcy_f, angle;
	int i, j, srcx, srcy;
	int xbound[4], ybound[4];
	float xboundf[4], yboundf[4];
	zoomx = drawmethod->scalex / 256.0;
	zoomy = drawmethod->scaley / 256.0;
	angle = drawmethod->rotate;
    sina = sin_table[(int)angle];
    cosa = cos_table[(int)angle];

	init_gfx_global_draw_stuff(dest, src, drawmethod);
	if(!trans_sw) return;

	centerx += drawmethod->centerx;
	centery += drawmethod->centery;
	
	/////////////////begin clipping////////////////////////////
	xboundf[0] = drawmethod->flipx? (centerx-trans_sw)*zoomx : -centerx*zoomx;
	yboundf[0] = drawmethod->flipy? (centery-trans_sh)*zoomy : -centery*zoomy;
	xboundf[1] = xboundf[0] + trans_sw*zoomx;
	yboundf[1] = yboundf[0];
	xboundf[2] = xboundf[0];
	yboundf[2] = yboundf[0] + trans_sh*zoomy;
	xboundf[3] = xboundf[1];
	yboundf[3] = yboundf[2];

	for(i=0; i<4; i++){
		xbound[i] =  (int)(x + xboundf[i]*cosa - yboundf[i]*sina);
        ybound[i] =  (int)(y + xboundf[i]*sina + yboundf[i]*cosa);
	}

	xmin = MAX(MIN(MIN(xbound[0],xbound[1]), MIN(xbound[2],xbound[3])), xmin);
	xmax = MIN(MAX(MAX(xbound[0],xbound[1]), MAX(xbound[2],xbound[3])), xmax);
	ymin = MAX(MIN(MIN(ybound[0],ybound[1]), MIN(ybound[2],ybound[3])), ymin);
	ymax = MIN(MAX(MAX(ybound[0],ybound[1]), MAX(ybound[2],ybound[3])), ymax);
	/////////////////end clipping////////////////////////////

	// tricks to keep rotate not affected by flip
	if(drawmethod->flipx) {zoomx = -zoomx;}
	else  {angle = -angle;}
	if(drawmethod->flipy) {	zoomy = -zoomy; angle = -angle;}

	angle = (((int)angle)%360+360)%360;
	//if(!zoomx || !zoomy) return; //should be checked already
    rzoomx = 1.0 / zoomx;
    rzoomy = 1.0 / zoomy;
    sina = sin_table[(int)angle];
    cosa = cos_table[(int)angle];
    ax = rzoomx*cosa; 
    ay = rzoomx*sina; 
    bx = -rzoomy*sina; 
    by = rzoomy*cosa; 
    rx0 = centerx;
    ry0 = centery;
	x -= rx0;
	y -= ry0;
    cx = -(rx0+x)*rzoomx*cosa+(ry0+y)*rzoomy*sina+rx0;
    cy = -(rx0+x)*rzoomx*sina-(ry0+y)*rzoomy*cosa+ry0; 
	srcx0_f= cx+ymin*bx+xmin*ax, srcx_f;
    srcy0_f= cy+ymin*by+xmin*ay, srcy_f;

    for (j=ymin; j<ymax; j++)
    {
        srcx_f = srcx0_f;
        srcy_f = srcy0_f;
        for (i=xmin;i<xmax;i++)
        {
            srcx=(int)(srcx_f);
            srcy=(int)(srcy_f);
			if(srcx>=0 && srcx<trans_sw && srcy>=0 && srcy<trans_sh){
				draw_pixel_gfx(dest, src, i, j, srcx, srcy);
			}
            srcx_f+=ax;
            srcy_f+=ay;
        }
        srcx0_f+=bx;
        srcy0_f+=by;
    }
}

void gfx_quad(s_screen *dest, gfx_entry* src, 
	int x0, int y0, 
	int x1, int y1, 
	int x2, int y2, 
	int x3, int y3,
	s_drawmethod* drawmethod)
{
	int a[4] = { y1-y0, y2-y1, y3-y2, y0-y3};
	int b[4] = { x0-x1, x1-x2, x2-x3, x3-x0};
	float r[4] = {
		invsqrt(a[0]*a[0]+b[0]*b[0]),
		invsqrt(a[1]*a[1]+b[1]*b[1]),
		invsqrt(a[2]*a[2]+b[2]*b[2]),
		invsqrt(a[3]*a[3]+b[3]*b[3])
	};
	int c[4] = {
		-x0*a[0]-y0*b[0],
		-x1*a[1]-y1*b[1],
		-x2*a[2]-y2*b[2],
		-x3*a[3]-y3*b[3]
	};
	int x, y, i;
	float dist[4];
	unsigned sx, sy;
	for(y=ymin; y<ymax; y++)
	{
		dest_seek(xmin,y);
		for(x=xmin; x<xmax; x++, dest_inc())
		{
			for(i=0; i<4; i++)
			{
				dist[i] = (x*a[i]+y*b[i]+c[i])*r[i];
			}
			sx = (unsigned)(dist[3]*(trans_sw-1)/(dist[3]+dist[1]));
			sy = (unsigned)(dist[0]*(trans_sh-1)/(dist[0]+dist[2]));
			if(sx>=trans_sw || sy>=trans_sh) continue;
			src_seek(sx, sy);
			write_pixel();
		}
	}
}

#if 0
// scalex scaley flipy ...
void gfx_draw_scale(s_screen *dest, gfx_entry* src, int x, int y, int centerx, int centery, s_drawmethod* drawmethod)
{
	float beginy, endy, beginx, endx, w, h, cx, cy;
	int i;
	float zoomx = drawmethod->scalex / 256.0;
	float zoomy = drawmethod->scaley / 256.0;
	float shiftf = drawmethod->shiftx / 256.0;
	float qx[4], qy[4];

	init_gfx_global_draw_stuff(dest, src, drawmethod);
	if(!trans_sw) return;

	centerx += drawmethod->centerx;
	centery += drawmethod->centery;

	w = trans_sw * zoomx;
	h = trans_sh * zoomy;
	cx = centerx * zoomx;
	cy = centery * zoomy;

	qx[0] = x-cx; qy[0] = y-cy;
	qx[1] = qx[0]+w; qy[1] = qy[0];
	qx[2] = qx[1]; qy[2] = qy[0]+h;
	qx[3] = qx[0]; qy[3] = qy[2];

	if(drawmethod->flipy) {
		shiftf = -shiftf;
		for(i=0; i<4; i++)
			qy[i] = 2*y-qy[i];
	}
	if(drawmethod->flipx) {
		for(i=0; i<4; i++)
			qx[i] = 2*x-qx[i];
	}
	if(shiftf)
	{
		for(i=0; i<4; i++)
			qx[i] += (qy[i]-y)*shiftf;
	}

	beginx = MIN(MIN(qx[0], qx[1]), MIN(qx[2], qx[3]));
	endx = MAX(MAX(qx[0], qx[1]), MAX(qx[2], qx[3]));
	beginy = MIN(qy[0], qy[3]);
	endy = MAX(qy[0], qy[3]);

	if(endy<=ymin || beginy>=ymax) return;
	if(endx<=xmin || beginx>=xmax) return;

	ymin = MAX(ymin, beginy);
	ymax = MIN(ymax, endy);
	xmin = MAX(xmin, beginx);
	xmax = MIN(xmax, endx);

	gfx_quad(dest, src, qx[0], qy[0], qx[1], qy[1], qx[2], qy[2], qx[3], qy[3], drawmethod);

}
#endif

#if 0
// scalex scaley flipy ...
void gfx_draw_scale(s_screen *dest, gfx_entry* src, int x, int y, int centerx, int centery, s_drawmethod* drawmethod)
{
	float osx, sx, sy, dx, dy, w, h, cx, cy, stepdx, stepdy, beginy, endy, beginx, endx;
	int i, j;
	float zoomx = drawmethod->scalex / 256.0;
	float zoomy = drawmethod->scaley / 256.0;
	float shiftf = drawmethod->shiftx / 256.0;
	
	//if(!zoomy || !zoomx) return; //should be checked already

	//printf("==%d %d %d %d %d\n", drawmethod->scalex, drawmethod->scaley, drawmethod->flipx, drawmethod->flipy, drawmethod->shiftx);

	init_gfx_global_draw_stuff(dest, src, drawmethod);
	if(!trans_sw) return;

	centerx += drawmethod->centerx;
	centery += drawmethod->centery;

	w = trans_sw * zoomx;
	h = trans_sh * zoomy;
	cx = centerx * zoomx;
	cy = centery * zoomy;
	
	if(drawmethod->flipx) dx = cx - w + x;
	else dx = x - cx;

	if(drawmethod->flipy){
		dy = cy - h + y; 
		shiftf = - shiftf;
	}
	else dy = y - cy; 

	dx += (dy + h - y) * shiftf;

	if(drawmethod->flipx) {
		stepdx = 1.0/zoomx;
		osx = 0.0;
	}else{
		stepdx = -1.0/zoomx;
		osx = trans_sw + stepdx;
	}
	if(drawmethod->flipy){
		stepdy = 1.0/zoomy;
		sy = 0.0;
	}else{
		stepdy = -1.0/zoomy;
		sy = trans_sh + stepdy;
	}

	if(MAX(dx+w, dx+w-shiftf*h)<=xmin) return;
	if(MIN(dx, dx-shiftf*h)>=xmax) return;
	if(dy>=ymax) return;
	if(dy+h<=ymin) return;

	if(dy+h>ymax) {
		endy = ymax;
		dx -= shiftf*(dy+h-endy);
		sy += stepdy*(dy+h-endy);
	} else endy = dy+h;

	if(dy<ymin) beginy = ymin;
	else beginy = dy;

	//printf("=%d, %d, %lf, %lf, %lf, %lf, %lf, %lf\n ",x, y, w, h, osx, sy, dx, dy);
	// =64, 144, 44.000000, 36.500000, 43.000000, 0.000000, 38.000000, 143.000000

	for(j=endy-1; j>=beginy; j--, sy+=stepdy, dx -= shiftf){
		if(dx>=xmax) continue;
		if(dx+w<=xmin) continue;
		sx = osx;
		beginx = dx;
		endx = dx+w;
		if(dx<xmin) beginx = xmin;
		else beginx = dx;
		if(dx+w>xmax) {
			endx = xmax;
			sx += stepdx*(dx+w-xmax);
		} else endx = dx+w;
		dest_seek(endx-1, j);
		for(i=endx-1; i>=beginx; i--, sx+=stepdx){
			//draw_pixel_gfx(dest, src, i, j, (int)sx, (int)sy);
			src_seek((int)sx, (int)sy);
			write_pixel();
			dest_dec();
		}
	}

}
#endif

#if 1
//same as above, fix point version
// scalex scaley flipy ...
void gfx_draw_scale(s_screen *dest, gfx_entry* src, int x, int y, int centerx, int centery, s_drawmethod* drawmethod)
{
	int osx, sx, sy, dx, dy, w, h, cx, cy, stepdx, stepdy, beginy, endy, beginx, endx;
	int i, j;
	int zoomx = drawmethod->scalex;
	int zoomy = drawmethod->scaley;
	int shiftf = drawmethod->shiftx;
	
	init_gfx_global_draw_stuff(dest, src, drawmethod);
	if(!trans_sw) return;

	centerx += drawmethod->centerx;
	centery += drawmethod->centery;

	xmin <<= 8; xmax <<= 8; ymin <<= 8; ymax <<= 8;
	x <<= 8; y<<= 8;

	w = trans_sw * zoomx;
	h = trans_sh * zoomy;
	cx = centerx * zoomx;
	cy = centery * zoomy;
	
	if(drawmethod->flipx) dx = cx - w + x;
	else dx = x - cx;

	if(drawmethod->flipy){
		dy = cy - h + y; 
		shiftf = - shiftf;
	}
	else dy = y - cy; 

	dx += ((dy + h - y) * shiftf)>>8;

	if(drawmethod->flipx) {
		stepdx = 65536/zoomx;
		osx = 0;
	}else{
		stepdx = -65536/zoomx;
		osx = (trans_sw<<8) + stepdx;
	}
	if(drawmethod->flipy){
		stepdy = 65536/zoomy;
		sy = 0;
	}else{
		stepdy = -65536/zoomy;
		sy = (trans_sh<<8) + stepdy;
	}

	if(MAX(dx+w, dx+w-(shiftf*h>>8))<=xmin) return;
	if(MIN(dx, dx-(shiftf*h>>8))>=xmax) return;
	if(dy>=ymax) return;
	if(dy+h<=ymin) return;

	if(dy+h>ymax) {
		endy = ymax;
		dx -= shiftf*(dy+h-endy)>>8;
		sy += stepdy*(dy+h-endy)>>8;
	} else endy = dy+h;

	if(dy<ymin) beginy = ymin;
	else beginy = dy;

	//printf("=%d, %d, %lf, %lf, %lf, %lf, %lf, %lf\n ",x, y, w, h, osx, sy, dx, dy);
	// =64, 144, 44.000000, 36.500000, 43.000000, 0.000000, 38.000000, 143.000000

	for(j=endy-256; j>=beginy; j-=256, sy+=stepdy, dx -= shiftf){
		if(dx>=xmax) continue;
		if(dx+w<=xmin) continue;
		sx = osx;
		beginx = dx;
		endx = dx+w;
		if(dx<xmin) beginx = xmin;
		else beginx = dx;
		if(dx+w>xmax) {
			endx = xmax;
			sx += stepdx*(dx+w-xmax)>>8;
		} else endx = dx+w;
		dest_seek((endx>>8)-1, j>>8);
		for(i=endx-256; i>=beginx; i-=256, sx+=stepdx){
			//draw_pixel_gfx(dest, src, i, j, (int)sx, (int)sy);
			//if(sy<0) printf("trans_sh %d sy %d stepdy %d\n", trans_sh, sy, stepdy);
			src_seek(sx>>8, sy>>8);
			write_pixel();
			dest_dec();
		}
	}

}
#endif

float _sinfactors[256] = {1.0f,1.0245412285229123f,1.049067674327418f,1.0735645635996673f,1.0980171403295606f,1.1224106751992162f,1.1467304744553617f,1.1709618887603012f,1.1950903220161282f,1.2191012401568697f,1.2429801799032638f,1.2667127574748984f,1.2902846772544622f,1.3136817403988914f,1.33688985339222f,1.3598950365349882f,1.3826834323650898f,1.4052413140049897f,1.427555093430282f,1.4496113296546064f,1.4713967368259977f,1.492898192229784f,1.5141027441932215f,1.5349976198870971f,1.5555702330196021f,1.5758081914178454f,1.5956993044924332f,1.6152315905806267f,1.6343932841636454f,1.6531728429537766f,1.6715589548470184f,1.6895405447370668f,1.7071067811865474f,1.724247082951467f,1.7409511253549592f,1.7572088465064843f,1.7730104533627368f,1.7883464276266063f,1.8032075314806448f,1.8175848131515837f,1.8314696123025453f,1.8448535652497071f,1.8577286100002721f,1.8700869911087112f,1.881921264348355f,1.8932243011955152f,1.9039892931234434f,1.9142097557035307f,1.9238795325112867f,1.9329927988347388f,1.9415440651830207f,1.9495281805930366f,1.956940335732209f,1.96377606579544f,1.970031253194544f,1.9757021300385284f,1.9807852804032304f,1.985277642388941f,1.9891765099647811f,1.99247953459871f,1.9951847266721967f,1.9972904566786902f,1.9987954562051724f,1.9996988186962041f,2.0f,1.9996988186962041f,1.9987954562051724f,1.9972904566786902f,1.995184726672197f,1.99247953459871f,1.9891765099647811f,1.985277642388941f,1.9807852804032304f,1.9757021300385284f,1.970031253194544f,1.96377606579544f,1.956940335732209f,1.9495281805930366f,1.9415440651830207f,1.9329927988347388f,1.9238795325112867f,1.9142097557035307f,1.9039892931234434f,1.8932243011955152f,1.881921264348355f,1.8700869911087114f,1.8577286100002721f,1.8448535652497071f,1.8314696123025453f,1.8175848131515837f,1.8032075314806448f,1.7883464276266063f,1.773010453362737f,1.7572088465064848f,1.740951125354959f,1.724247082951467f,1.7071067811865474f,1.689540544737067f,1.6715589548470184f,1.6531728429537766f,1.6343932841636454f,1.615231590580627f,1.5956993044924334f,1.5758081914178454f,1.5555702330196021f,1.5349976198870971f,1.5141027441932217f,1.4928981922297841f,1.471396736825998f,1.449611329654607f,1.427555093430282f,1.40524131400499f,1.3826834323650898f,1.3598950365349882f,1.3368898533922202f,1.3136817403988914f,1.2902846772544625f,1.2667127574748984f,1.242980179903264f,1.2191012401568701f,1.1950903220161286f,1.1709618887603012f,1.146730474455362f,1.1224106751992164f,1.0980171403295608f,1.0735645635996677f,1.0490676743274178f,1.0245412285229123f,1.0000000000000002f,0.9754587714770879f,0.9509323256725822f,0.9264354364003325f,0.9019828596704394f,0.8775893248007839f,0.8532695255446384f,0.8290381112396991f,0.8049096779838716f,0.7808987598431302f,0.7570198200967362f,0.7332872425251018f,0.709715322745538f,0.6863182596011088f,0.6631101466077799f,0.6401049634650119f,0.6173165676349104f,0.5947586859950102f,0.5724449065697181f,0.5503886703453933f,0.5286032631740023f,0.5071018077702161f,0.48589725580677845f,0.46500238011290307f,0.44442976698039804f,0.42419180858215466f,0.40430069550756675f,0.3847684094193733f,0.36560671583635473f,0.34682715704622346f,0.32844104515298156f,0.31045945526293317f,0.29289321881345254f,0.2757529170485332f,0.2590488746450411f,0.24279115349351576f,0.22698954663726334f,0.2116535723733941f,0.19679246851935494f,0.18241518684841618f,0.16853038769745476f,0.155146434750293f,0.142271389999728f,0.12991300889128865f,0.11807873565164506f,0.10677569880448478f,0.09601070687655688f,0.08579024429646953f,0.07612046748871348f,0.06700720116526104f,0.058455934816979194f,0.050471819406963325f,0.043059664267791176f,0.03622393420456016f,0.029968746805456026f,0.02429786996147154f,0.01921471959676968f,0.01472235761105889f,0.010823490035219096f,0.007520465401289922f,0.004815273327803071f,0.002709543321309793f,0.001204543794827595f,0.00030118130379575003f,0.0f,0.00030118130379575003f,0.001204543794827595f,0.002709543321309793f,0.004815273327803071f,0.007520465401289922f,0.010823490035219096f,0.014722357611058778f,0.01921471959676957f,0.02429786996147143f,0.029968746805456026f,0.03622393420456005f,0.043059664267791065f,0.050471819406963214f,0.05845593481697908f,0.06700720116526093f,0.07612046748871337f,0.08579024429646942f,0.09601070687655666f,0.10677569880448467f,0.11807873565164495f,0.12991300889128854f,0.14227138999972777f,0.15514643475029277f,0.16853038769745454f,0.18241518684841595f,0.19679246851935472f,0.21165357237339388f,0.22698954663726312f,0.24279115349351543f,0.2590488746450409f,0.275752917048533f,0.2928932188134523f,0.31045945526293283f,0.32844104515298133f,0.3468271570462229f,0.36560671583635407f,0.3847684094193726f,0.40430069550756675f,0.4241918085821548f,0.4444297669803978f,0.46500238011290273f,0.4858972558067781f,0.5071018077702157f,0.5286032631740021f,0.5503886703453931f,0.5724449065697175f,0.5947586859950096f,0.6173165676349096f,0.640104963465012f,0.66311014660778f,0.6863182596011085f,0.7097153227455375f,0.7332872425251014f,0.7570198200967358f,0.7808987598431298f,0.8049096779838712f,0.8290381112396983f,0.8532695255446376f,0.877589324800784f,0.9019828596704394f,0.9264354364003325f,0.9509323256725819f,0.9754587714770876f};
#define distortion(x, a) ((int)(_sinfactors[x]*a+0.5))

void gfx_draw_water(s_screen *dest, gfx_entry* src, int x, int y, int centerx, int centery, s_drawmethod* drawmethod){
	int sw, sh, dw, dh, ch, sy, t, u, sbeginx, sendx, bytestocopy, dbeginx, dendx, amplitude, time;
	float s, wavelength;

	init_gfx_global_draw_stuff(dest, src, drawmethod);
	if(!trans_sw) return;

	centerx += drawmethod->centerx;
	centery += drawmethod->centery;

	sw = trans_sw;
	sh = trans_sh;
	dw = trans_dw;
	dh = trans_dh;
	ch = sh;
	x -= centerx;
	y -= centery;

	amplitude = drawmethod->water.amplitude;
	time = drawmethod->water.wavetime;

	s = time % 256;

	// Clip x
	if(x + amplitude*2 + sw <= 0 || x - amplitude*2  >= dw) return;
	if(y + sh <=0 || y >= dh) return;

	sy = 0;
	if(s<0) s+=256;

	// Clip y
	if(y<0) {sy = -y; ch += y;}
	if(y+sh > dh) ch -= (y+sh) - dh;

	if(y<0) y = 0;

	u = (drawmethod->water.watermode==1)?distortion((int)s, amplitude):amplitude;
	wavelength = 256 / drawmethod->water.wavelength;
	s += sy*wavelength;

	// Copy data
	do{
		s = s - (int)s + (int)s % 256;
		t = distortion((int)s, amplitude) - u;

		dbeginx = x+t;
		dendx = x+t+sw;
		
		if(dbeginx>=dw || dendx<=0) {dbeginx = dendx = sbeginx = sendx = 0;} //Nothing to draw
		//clip both
		else if(dbeginx<0 && dendx>dw){ 
			sbeginx = -dbeginx; sendx = sbeginx + dw;
			dbeginx = 0; dendx = dw;
		}
		//clip left
		else if(dbeginx<0) {
			sbeginx = -dbeginx; sendx = sw;
			dbeginx = 0;
		}
		// clip right
		else if(dendx>dw) {
			sbeginx = 0; sendx = dw - dbeginx;	
			dendx = dw;
		}
		// clip none
		else{
			sbeginx = 0; sendx = sw;
		}
		if(pixelformat==PIXEL_8 && !transbg && src->screen->magic==screen_magic){
			dest_seek(dbeginx, y); src_seek(sbeginx, sy);
			copy_pixel_block(dendx-dbeginx);
		}else{
			bytestocopy = dendx-dbeginx;
			//TODO: optimize this if necessary
			for(t=0, dest_seek(dbeginx, y), src_seek(sbeginx, sy); t<bytestocopy; t++, dest_inc(), src_inc()){
				//draw_pixel_gfx(dest, src, dbeginx, y, sbeginx, sy);
				write_pixel();
			}
		}

		s += wavelength;
		y++;
		sy++;
	}while(--ch);
}

void gfx_draw_plane(s_screen *dest, gfx_entry* src, int x, int y, int centerx, int centery, s_drawmethod* drawmethod){

	int i, j, dx, dy, sx, sy, width, height, copyh;
	float osxpos, sxpos, sypos, sxstep, systep, xpos, ypos, factor, size, cx, beginsize, endsize;

	init_gfx_global_draw_stuff(dest, src, drawmethod);
	if(!trans_sw) return;

	centerx += drawmethod->centerx;
	centery += drawmethod->centery;

	x -= centerx;
	y -= centery;

	xpos = x;
	ypos = y;

	beginsize = drawmethod->water.beginsize;
	endsize = drawmethod->water.endsize;

	if(beginsize<0 || endsize<0) return;

	width = trans_sw;
	height = trans_sh;

	// Check dimensions
	if(x >= trans_dw) return;
	if(y >= trans_dh) return;
	if(x<0){
		width += x;
		x = 0;
	}
	if(y<0){
		copyh = -y;
		height += y;
		y=0;
	}else copyh = 0;
	if(x+width > trans_dw){
		width = trans_dw - x;
	}
	if(y+height > trans_dh){
		height = trans_dh - y;
	}
	if(width<=0) return;
	if(height<=0) return;

	copyh += height;

	dy = ypos;
	dx = x;

	cx = trans_dw / 2.0 - x;

	osxpos = drawmethod->water.wavetime - xpos + trans_sw;

	factor = (endsize - beginsize) / trans_sh;
	size = beginsize;

	sypos = 0.0;
	for(i=0; i<copyh; i++, dy++, size+=factor, sypos+=systep)
	{
		sy = (int)sypos;
		sy %= trans_sh;
		if(sy<0) sy += trans_sh;
		sxstep = 1 / size;
		switch(drawmethod->water.perspective){
		case 1: // tile
			systep = sxstep;
			break;
		case 2: //stretch
			systep = sxstep*trans_sh/(float)trans_sw;
			break;
		default:
			systep = 1.0;
		}
		if(dy<0) continue;
		sxpos = osxpos - cx * sxstep;

		//dest_seek(dx, dy);
		for(j=0; j<width; j++, sxpos += sxstep){
			sx = (int)sxpos;
			sx %= trans_sw;
			if(sx<0) sx += trans_sw;
			//src_seek(sx, sy);
			draw_pixel_gfx(dest, src, dx+j, dy, sx, sy);
			//write_pixel();
			//dest_inc();
		}
	}

}

