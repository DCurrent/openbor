/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <pspdisplay.h>
#include <psputils.h>
#include <pspgu.h>
#include "hankaku.c"
#include "image.h"
#include "types.h"
#include "vertex.h"
#include "graphics.h"
#include "kernel/kernel.h"
#include "dvemgr/dvemgr.h"

#define PSP_VRAM_BASE ((u32*)(0x40000000|0x04000000))
#define PSP_VRAM_ADDRESS(x) (x ? 0x0a000000 : 0x44000000)
#define	PSP_LCD_TEXTURE_WIDTH 512
#define	PSP_TV_TEXTURE_WIDTH 768
#define PSP_TV_WIDTH 720
#define PSP_TV_HEIGHT 480

typedef struct{
	unsigned short u, v;
	short x, y, z;
}Vertex;

typedef void (*BlitScreenToScreen)(int, int, s_screen*);
typedef void (*FlipScreen)();

void blitScreenToScreenRaw(int width, int height, s_screen* source);
void blitScreenToScreenPal(int width, int height, s_screen* source);
void flipScreenLCD();
void flipScreenTVI();

static BlitScreenToScreen pBlitScreenToScreen;
static FlipScreen pFlipScreen;

unsigned int __attribute__((aligned(16))) list[262144];

static int PSP_FRAMEBUFFER_SIZE;
static int PSP_TEXTURE_FILTER;
static int PSP_TEXTURE_WIDTH;
static int PSP_DISPLAY_HEIGHT;
static int PSP_DISPLAY_WIDTH;
static int PSP_BUFFER_SIZE;
static int PSP_COLOR_FORMAT;
static int tvOverScanBottom;
static int tvOverScanRight;
static int tvOverScanLeft;
static int tvOverScanTop;
static int tvAspectRatio;
static int screenHeight;
static int screenWidth;
static int screenLeft;
static int screenTop;
static int initialized;
static void* drawBuffer;
static void* dispBuffer;

char* displayName[PSP_DISPLAY_MODES] = {"LCD", "Composite", "Progressive", "Interlaced"};
int displayMode;

char* filterName[PSP_DISPLAY_FILTERS] = {"Linear", "Bilinear"};
DisplayFormat displayFormat[PSP_DISPLAY_FORMATS] = {{"320x240 (4x3)",   80, 16, 320, 240},
						  						    {"360x270 (4x3)",   60, 01, 360, 270},
												    {"384x272 (24x17)", 48, 00, 384, 272},
													{"480x272 (16x9)",  00, 00, 480, 272}};
int checkCable(int mode)
{
	if(getHardwareModel()==1)
	{
		if(pspDveMgrCheckVideoOut() > 0) return mode;
		else return 0;
	}
	return 0;
}

void guStart()
{
	sceGuStart(GU_DIRECT, list);
}

static void guSetup()
{
	sceKernelDcacheWritebackInvalidateAll();
	guStart();
	sceGuDrawBuffer(GU_PSM_8888, 0, PSP_TEXTURE_WIDTH);
	sceGuDispBuffer(PSP_DISPLAY_WIDTH, PSP_DISPLAY_HEIGHT, 0, PSP_TEXTURE_WIDTH);
	sceGuOffset(2048 - (PSP_DISPLAY_WIDTH >> 1), 2048 - (PSP_DISPLAY_HEIGHT >> 1));
	sceGuViewport(2048, 2048, PSP_DISPLAY_WIDTH, PSP_DISPLAY_HEIGHT);
	sceGuScissor(0, 0, PSP_DISPLAY_WIDTH, PSP_DISPLAY_HEIGHT);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuDisable(GU_CULL_FACE);
	sceGuDisable(GU_DEPTH_TEST);
	sceGuDepthMask(GU_TRUE);
	sceGuDisable(GU_COLOR_TEST);
	sceGuDisable(GU_ALPHA_TEST);
	sceGuDisable(GU_LIGHTING);
	sceGuEnable(GU_TEXTURE_2D);
	sceGuClearColor(0);
	sceGuClear(GU_COLOR_BUFFER_BIT);
	sceGuFinish();
	sceGuSync(0, 0);
	memset(drawBuffer, 0, PSP_BUFFER_SIZE);
	memset(dispBuffer, 0, PSP_BUFFER_SIZE);
	sceDisplayWaitVblankStart();
	sceDisplaySetFrameBuf(drawBuffer, PSP_TEXTURE_WIDTH, PSP_DISPLAY_PIXEL_FORMAT_8888, PSP_DISPLAY_SETBUF_NEXTFRAME);
}

void setGraphicsVideoMode(int mode)
{
	if(mode && getHardwareModel()==1)
	{
		PSP_BUFFER_SIZE      = PSP_TV_TEXTURE_WIDTH * 4;
		PSP_FRAMEBUFFER_SIZE = PSP_TV_TEXTURE_WIDTH * (mode%2?PSP_LCD_TEXTURE_WIDTH:PSP_TV_HEIGHT) * 4;
		PSP_TEXTURE_WIDTH    = PSP_TV_TEXTURE_WIDTH;
		PSP_DISPLAY_WIDTH    = PSP_TV_WIDTH;
		PSP_DISPLAY_HEIGHT   = PSP_TV_HEIGHT;
		pspDveMgrSetVideoOut(mode!=1?0:2, mode%2?0x1D1:0x1D2, 720, mode%2?503:480, 1, 15, 0);
	}
	else
	{
		PSP_BUFFER_SIZE      = PSP_LCD_TEXTURE_WIDTH * 4;
		PSP_FRAMEBUFFER_SIZE = PSP_LCD_TEXTURE_WIDTH * PSP_LCD_HEIGHT * 4;
		PSP_TEXTURE_WIDTH    = PSP_LCD_TEXTURE_WIDTH;
		PSP_DISPLAY_WIDTH    = PSP_LCD_WIDTH;
		PSP_DISPLAY_HEIGHT   = PSP_LCD_HEIGHT;
		if(getHardwareModel()==1) pspDveMgrSetVideoOut(0, 0, 480, 272, 1, 15, 0);
	}
}

void setGraphics(int mode, int pixel)
{
	if(mode%2) pFlipScreen = flipScreenTVI;
	else pFlipScreen = flipScreenLCD;
	switch(pixel)
	{
		case PIXEL_32:
			pBlitScreenToScreen = blitScreenToScreenRaw;
			PSP_COLOR_FORMAT = GU_PSM_8888;
			break;
		case PIXEL_16:
	  		pBlitScreenToScreen = blitScreenToScreenRaw;
			PSP_COLOR_FORMAT = GU_PSM_5650;
			break;
		default:
			pBlitScreenToScreen = blitScreenToScreenPal;
			PSP_COLOR_FORMAT = GU_PSM_8888;
			break;
	}
}

void initGraphics(int mode, int pixel)
{
	displayMode = checkCable(mode);
	setGraphicsTVAspectRatio(0);
	setGraphicsVideoMode(displayMode);
	drawBuffer = (void*)PSP_VRAM_ADDRESS(displayMode%2);
	dispBuffer = (void*)PSP_VRAM_ADDRESS(displayMode%2) + PSP_FRAMEBUFFER_SIZE;
	initialized = 1;
	guSetup();
	setGraphicsScreen(displayFormat[3], pixel, 0);
}

void disableGraphics()
{
	initialized = 0;
}

void drawTexture(tVertexTexture *t)
{
	Vertex *vertex = sceGuGetMemory(2 * sizeof(Vertex));
	vertex[0].u = t->output_texture_x_start;
	vertex[0].v = t->output_texture_y_start;
	vertex[0].x = t->output_vertex_x_start;
	vertex[0].y = t->output_vertex_y_start;
	vertex[0].z = 0;
	vertex[1].u = t->output_texture_x_end;
	vertex[1].v = t->output_texture_y_end;
	vertex[1].x = t->output_vertex_x_end;
	vertex[1].y = t->output_vertex_y_end;
	vertex[1].z = 0;
	sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vertex);
}

static inline void drawVertex(tVertexTexture t)
{
	do{
		getVertexTexture(&t);
		drawTexture(&t);
	}while(t.output_last == 0);
}

void setGraphicsTVAspectRatio(int ar)
{
	tvAspectRatio = ar;
}

void setGraphicsTVOverScan(int left, int top, int right, int bottom)
{
	tvOverScanLeft = left;
	tvOverScanTop = top;
	tvOverScanRight = right;
	tvOverScanBottom = bottom;
}

void setGraphicsScreen(DisplayFormat display, int pixel, int filter)
{
	setGraphics(displayMode, pixel);
	PSP_TEXTURE_FILTER = filter;
	screenTop = display.top;
	screenLeft = display.left;
	screenWidth = display.width;
	screenHeight = display.height;
}

void clearScreen(Color color)
{
	if(!initialized) return;
	guStart();
	sceGuClearColor(color);
	sceGuClearDepth(color);
	sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
	sceGuFinish();
	sceGuSync(0, 0);
}

void flipScreen()
{
	pFlipScreen();
}

void flipScreenLCD()
{
	if(!initialized) return;
	sceGuSwapBuffers();
}

void flipScreenTVI()
{
	if(!initialized) return;
	sceKernelDcacheWritebackInvalidateAll();
	guStart();
	sceGuCopyImage(GU_PSM_8888, 0, 0, PSP_DISPLAY_WIDTH, PSP_DISPLAY_HEIGHT>>1, PSP_TEXTURE_WIDTH<<1, (void*)PSP_VRAM_BASE+PSP_BUFFER_SIZE, 0, 0, PSP_TEXTURE_WIDTH, drawBuffer);
	sceGuCopyImage(GU_PSM_8888, 0, 0, PSP_DISPLAY_WIDTH, PSP_DISPLAY_HEIGHT>>1, PSP_TEXTURE_WIDTH<<1, PSP_VRAM_BASE, 0, 0, PSP_TEXTURE_WIDTH, (void*)drawBuffer+PSP_BUFFER_SIZE*262);
	sceGuFinish();
	sceGuSync(0, 0);
	sceGuSwapBuffers();
}

void blitImageToScreen(int sx, int sy, int width, int height, Image* source, int dx, int dy)
{
	if(!initialized) return;
	sceKernelDcacheWritebackInvalidateAll();
	guStart();
	sceGuDisable(GU_BLEND);
	sceGuTexMode(GU_PSM_8888, 0, 0, 0);
	sceGuTexImage(0, source->textureWidth, source->textureHeight, source->textureWidth, source->data);
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);
	sceGuTexFilter(PSP_TEXTURE_FILTER, PSP_TEXTURE_FILTER);
	sceGuTexWrap(GU_CLAMP, GU_CLAMP);
	tVertexTexture texture;
	setVertexTexture(&texture, width, height, 16, screenWidth+sx, screenHeight+sy, screenLeft+dx, screenTop+dy);
	drawVertex(texture);
	sceGuFinish();
	sceGuSync(0, 0);
}

void blitAlphaImageToScreen(int sx, int sy, int width, int height, Image* source, int dx, int dy)
{
	if(!initialized) return;
	sceKernelDcacheWritebackInvalidateAll();
	guStart();
	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	sceGuTexMode(GU_PSM_8888, 0, 0, 0);
	sceGuTexImage(0, source->textureWidth, source->textureHeight, source->textureWidth, source->data);
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
	sceGuTexFilter(PSP_TEXTURE_FILTER, PSP_TEXTURE_FILTER);
	sceGuTexWrap(GU_CLAMP, GU_CLAMP);
	tVertexTexture texture;
	setVertexTexture(&texture, width, height, 16, screenWidth+sx, screenHeight+sy, screenLeft+dx, screenTop+dy);
	drawVertex(texture);
	sceGuFinish();
	sceGuSync(0, 0);
}

void blitScreenToScreen(int width, int height, s_screen* source)
{
	pBlitScreenToScreen(width, height, source);
}

void blitScreenToScreenRaw(int width, int height, s_screen* source)
{
	if(!initialized) return;
	sceKernelDcacheWritebackInvalidateAll();
	guStart();
	sceGuCopyImage(PSP_COLOR_FORMAT, 0, 0, width, height, source->width, source->data, 0, 0, PSP_LCD_TEXTURE_WIDTH, dispBuffer);
	sceGuTexMode(PSP_COLOR_FORMAT, 0, 0, 0);
	sceGuTexImage(0, 512, 512, 512, dispBuffer);
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);
	sceGuTexFilter(PSP_TEXTURE_FILTER, PSP_TEXTURE_FILTER);
	tVertexTexture texture;
	setVertexTexture(&texture, width, height, 16, screenWidth, screenHeight, screenLeft, screenTop);
	drawVertex(texture);
	sceGuFinish();
}

void blitScreenToScreenPal(int width, int height, s_screen* source)
{
	if(!initialized) return;
	sceKernelDcacheWritebackInvalidateAll();
	guStart();
	sceGuClutMode(PSP_COLOR_FORMAT, 0, 255, 0);
	sceGuClutLoad((32), palette);
	sceGuTexMode(GU_PSM_T8, 0, 0, 0);
	sceGuTexImage(0, 512, 512, source->width, source->data);
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);
	sceGuTexFilter(PSP_TEXTURE_FILTER, PSP_TEXTURE_FILTER);
	tVertexTexture texture;
	setVertexTexture(&texture, width, height, 16, screenWidth, screenHeight, screenLeft, screenTop);
	drawVertex(texture);
	sceGuFinish();
	sceGuSync(0, 0);
}

void printText(Image* source, int x,int y,int col,int backcol,int fill,char *format, ...)
{
	Color data_ptr;
	Color *data;
	u8  *font;
	int x1,y1,i;
	unsigned char ch = 0;
	char buf[128] = {""};
	va_list arglist;
	va_start(arglist, format);
	vsprintf(buf, format, arglist);
	va_end(arglist);
	for(i=0; i<sizeof(buf); i++)
	{
		ch = buf[i];
		// mapping
		if (ch<0x20) ch = 0;
		else if (ch<0x80) { ch -= 0x20; }
		else if (ch<0xa0) {	ch = 0;	}
		else ch -= 0x40;
		font = (u8 *)&hankaku_font10[ch*10];
		// draw
		data = source->data + x + y * source->textureWidth;
		for(y1=0;y1<10;y1++)
		{
			data_ptr = *font++;
			for(x1=0;x1<5;x1++)
			{
				if (data_ptr & 1) *data = col;
				else if (fill)    *data = backcol;
				data++;
				data_ptr = data_ptr >> 1;
			}
			data += source->textureWidth-5;;
		}
		x+=5;
	}
}
