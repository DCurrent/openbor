/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

#include <psptypes.h>
#include "types.h"
#include "image.h"

typedef struct{
	char name[16];
	int left;
	int top;
	int width;
	int height;
}DisplayFormat;

#define A(color) ((u8)(color >> 24 & 0x000000FF))
#define B(color) ((u8)(color >> 16 & 0x000000FF))
#define G(color) ((u8)(color >> 8 & 0x000000FF))
#define R(color) ((u8)(color & 0x000000FF))
#define RGB(r,g,b) ((r) | ((g) << 8) | ((b) << 16))
#define BLACK		RGB(0x00, 0x00, 0x00)
#define WHITE		RGB(0xFF, 0xFF, 0xFF)
#define RED			RGB(0xFF, 0x00, 0x00)
#define	GREEN		RGB(0x00, 0xFF, 0x00)
#define BLUE		RGB(0x00, 0x00, 0xFF)
#define YELLOW		RGB(0xFF, 0xFF, 0x00)
#define PURPLE		RGB(0xFF, 0x00, 0xFF)
#define ORANGE		RGB(0xFF, 0x80, 0x00)
#define GRAY		RGB(0x70, 0x80, 0x90)
#define LIGHT_GRAY  RGB(0xDF, 0xDF, 0xDF)
#define DARK_GRAY   RGB(0x52, 0x52, 0x51)
#define DARK_YELLOW RGB(0xF1, 0xB5, 0x43)
#define DARK_RED	RGB(0x80, 0x00, 0x00)
#define DARK_GREEN	RGB(0x00, 0x80, 0x00)
#define DARK_BLUE	RGB(0x00, 0x00, 0x80)

#define PSP_LCD_WIDTH       480
#define PSP_LCD_HEIGHT      272
#define PSP_DISPLAY_MODES   4
#define PSP_DISPLAY_FILTERS 2
#define PSP_DISPLAY_FORMATS 4

unsigned int __attribute__((aligned(16))) palette[256];
extern char* displayName[PSP_DISPLAY_MODES];
extern int displayMode;
extern char* filterName[PSP_DISPLAY_FILTERS];
extern DisplayFormat displayFormat[PSP_DISPLAY_FORMATS];

void initGraphics(int mode, int pixel);
void disableGraphics();
void guStart();
void clearScreen(Color color);
void flipScreen();
void setGraphicsScreen(DisplayFormat display, int pixel, int filter);
void setGraphicsTVOverScan(int left, int top, int right, int bottom);
void setGraphicsTVAspectRatio(int ar);
void blitAlphaImageToScreen(int sx, int sy, int width, int height, Image* source, int dx, int dy);
void blitImageToScreen(int sx, int sy, int width, int height, Image* source, int dx, int dy);
void blitScreenToScreen(int width, int height, s_screen* source);
void printText(Image* source, int x,int y,int col,int backcol,int fill,char *format, ...);
int pspSlimModel();

#endif
