/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <psptypes.h>

typedef u32 Color;
typedef struct
{
	int textureWidth;
	int textureHeight;
	int imageWidth;
	int imageHeight;
	Color* data;
}
Image;
Image *text;

Image* loadImage(const char* filename);
void saveImage(const char* filename);
Image* createImage(int width, int height);
void freeImage(Image* image);
void clearImage(Image* image, Color color);
void putPixelToImage(Image* image, Color color, int x, int y);
Color getPixelFromImage(Image* image, int x, int y);
void drawLineInImage(Image* image, Color color, int x0, int y0, int x1, int y1);
void fillImageRect(Image* image, Color color, int x0, int y0, int width, int height);
void fillImageEllipse(Image* image, Color color, int x0, int y0, int width, int height, int r);
void copyImageToImage(int sx, int sy, int width, int height, Image* source, int dx, int dy, Image* destination);
void drawImageBox(Image *source, Color background, Color border, int borderwidth);

#endif
