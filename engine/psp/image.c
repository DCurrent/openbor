/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <pspdisplay.h>
#include <malloc.h>
#include <png.h>
#include "image.h"
#include "graphics.h"
#include "tracemalloc.h"

static int getNextPower2(int width)
{
	int b = width;
	int n;
	for(n = 0; b != 0; n++) b >>= 1;
	b = 1 << n;
	if(b == 2 * width) b >>= 1;
	return b;
}

static void drawLine(int x0, int y0, int x1, int y1, int color, Color* destination, int width)
{
	int dy = y1 - y0;
	int dx = x1 - x0;
	int stepx, stepy;

	if(dy < 0)
	{
		dy = -dy;
		stepy = -width;
	}
	else stepy = width;

	if(dx < 0)
	{
		dx = -dx;
		stepx = -1;
	}
	else stepx = 1;

	dy <<= 1;
	dx <<= 1;

	y0 *= width;
	y1 *= width;
	destination[x0+y0] = color;
	if(dx > dy)
	{
		int fraction = dy - (dx >> 1);
		while(x0 != x1)
		{
			if (fraction >= 0)
			{
				y0 += stepy;
				fraction -= dx;
			}
			x0 += stepx;
			fraction += dy;
			destination[x0+y0] = color;
		}
	}
	else
	{
		int fraction = dx - (dy >> 1);
		while(y0 != y1)
		{
			if(fraction >= 0)
			{
				x0 += stepx;
				fraction -= dy;
			}
			y0 += stepy;
			fraction += dx;
			destination[x0+y0] = color;
		}
	}
}

void user_warning_fn(png_structp png_ptr, png_const_charp warning_msg)
{
}

Image *loadPNGImage(const char* filename)
{
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned int sig_read = 0;
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type, x, y;
	u32* line;
	FILE *fp;

	if((fp = fopen(filename, "rb")) == NULL) return NULL;
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(png_ptr == NULL)
	{
		fclose(fp);
		return NULL;;
	}
	png_set_error_fn(png_ptr, (png_voidp) NULL, (png_error_ptr) NULL, user_warning_fn);
	info_ptr = png_create_info_struct(png_ptr);
	if(info_ptr == NULL)
	{
		fclose(fp);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return NULL;
	}
	if(setjmp(png_jmpbuf(png_ptr)))
	{
		fclose(fp);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return NULL;
	}

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, sig_read);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, int_p_NULL, int_p_NULL);

	png_set_strip_16(png_ptr);
	png_set_packing(png_ptr);
	if(color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
	if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_gray_1_2_4_to_8(png_ptr);
	if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
	png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);

	Image *image = createImage(width, height);
	if(image == NULL)
	{
		fclose(fp);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return NULL;
	}

	line = (u32*)malloc(width * 4);
	if(!line)
	{
		freeImage(image);
		fclose(fp);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return NULL;
	}
	for(y=0; y<height; y++)
	{
		png_read_row(png_ptr, (u8*) line, png_bytep_NULL);
		for(x=0; x<width; x++)
		{
			u32 color = line[x];
			image->data[x + y * image->textureWidth] =  color;
		}
	}
	free(line);
	png_read_end(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
	fclose(fp);
	return image;
}

Image* loadImage(const char* filename)
{
	return loadPNGImage(filename);
}

void savePNGImage(const char* filename)
{
	u32* vram32;
	u16* vram16;
	int bufferwidth;
	int pixelformat;
	int sync = 0;
	int i, x, y;
	png_structp png_ptr;
	png_infop info_ptr;
	FILE* fp;
	u8* line;

	fp = fopen(filename, "wb");
	if(!fp) return;
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png_ptr) return;
	info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr)
	{
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		fclose(fp);
		return;
	}
	png_init_io(png_ptr, fp);
	png_set_IHDR(png_ptr, info_ptr, PSP_LCD_WIDTH, PSP_LCD_HEIGHT,
		         8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		         PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);
	line = (u8*)malloc(PSP_LCD_WIDTH * 3);
	sceDisplayWaitVblankStart();  // if framebuf was set with PSP_LCD_SETBUF_NEXTFRAME, wait until it is changed
	sceDisplayGetFrameBuf((void*)&vram32, &bufferwidth, &pixelformat, sync);
	vram16 = (u16*)vram32;
	for(y=0; y<PSP_LCD_HEIGHT; y++)
	{
		for(i=0, x=0; x<PSP_LCD_WIDTH; x++)
		{
			u32 color = 0;
			u8 r = 0, g = 0, b = 0;
			switch (pixelformat)
			{
				case PSP_DISPLAY_PIXEL_FORMAT_565:
					color = vram16[x + y * bufferwidth];
					r = (color & 0x1f) << 3;
					g = ((color >> 5) & 0x3f) << 2 ;
					b = ((color >> 11) & 0x1f) << 3 ;
					break;
				case PSP_DISPLAY_PIXEL_FORMAT_5551:
					color = vram16[x + y * bufferwidth];
					r = (color & 0x1f) << 3;
					g = ((color >> 5) & 0x1f) << 3 ;
					b = ((color >> 10) & 0x1f) << 3 ;
					break;
				case PSP_DISPLAY_PIXEL_FORMAT_4444:
					color = vram16[x + y * bufferwidth];
					r = (color & 0xf) << 4;
					g = ((color >> 4) & 0xf) << 4 ;
					b = ((color >> 8) & 0xf) << 4 ;
					break;
				case PSP_DISPLAY_PIXEL_FORMAT_8888:
					color = vram32[x + y * bufferwidth];
					r = color & 0xff;
					g = (color >> 8) & 0xff;
					b = (color >> 16) & 0xff;
					break;
			}
			line[i++] = r;
			line[i++] = g;
			line[i++] = b;
		}
		png_write_row(png_ptr, line);
	}
	free(line);
	line = NULL;
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	fclose(fp);
}

void saveImage(const char* filename)
{
	savePNGImage(filename);
}

Image* createImage(int width, int height)
{
	Image* image = (Image*) malloc(sizeof(Image));
	if(!image) return NULL;
	image->imageWidth = width;
	image->imageHeight = height;
	image->textureWidth = getNextPower2(width);
	image->textureHeight = getNextPower2(height);
	image->data = (Color*) memalign(16, image->textureWidth * image->textureHeight * sizeof(Color));
	if(!image->data)
	{
		free(image);
		image = NULL;
		return NULL;
	}
	memset(image->data, 0, image->textureWidth * image->textureHeight * sizeof(Color));
	return image;
}

void freeImage(Image* image)
{
	free(image->data);
	image->data = NULL;
	free(image);
	image = NULL;
}

void clearImage(Image* image, Color color)
{
	int i;
	int size = image->textureWidth * image->textureHeight;
	Color* data = image->data;
	for(i=0; i<size; i++, data++) *data = color;
}

void putPixelToImage(Image* image, Color color, int x, int y)
{
	image->data[x + y * image->textureWidth] = color;
}

Color getPixelFromImage(Image* image, int x, int y)
{
	return image->data[x + y * image->textureWidth];
}

void drawLineInImage(Image* image, Color color, int x0, int y0, int x1, int y1)
{
	drawLine(x0, y0, x1, y1, color, image->data, image->textureWidth);
}

void fillImageRect(Image* image, Color color, int x0, int y0, int width, int height)
{
	int skipX = image->textureWidth - width;
	int x, y;
	Color* data = image->data + x0 + y0 * image->textureWidth;
	for(y=0; y<height; y++, data+=skipX)
	{
		for(x=0; x<width; x++, data++) *data = color;
	}
}

void fillImageEllipse(Image* image, Color color, int x0, int y0, int width, int height, int r)
{
	int skipX = image->textureWidth - width;
	int x, y;
	Color* data = image->data + x0 + y0 * image->textureWidth;
	for(y=0; y<height; y++, data+=skipX)
	{
		for(x=0; x<width; x++, data++)
		{
			if((x < r) && (y < r)  && ((r-x)*(r-x)+(r-y)*(r-y) > r*r))
				continue;
			else if ((x < r) && (y > height-r-1)  && ((r-x)*(r-x)+(y-height+r+1)*(y-height+r+1) > r*r))
				continue;
			else if ((x > width-r-1) && (y < r)  && ((x-width+r+1)*(x-width+r+1)+(r-y)*(r-y) > r*r))
				continue;
			else if ((x > width-r-1) && (y > height-r-1)  && ((x-width+r+1)*(x-width+r+1)+(y-height+r+1)*(y-height+r+1) > r*r))
				continue;
			else
				*data = color;
		}
	}
}

void copyImageToImage(int sx, int sy, int width, int height, Image* source, int dx, int dy, Image* destination)
{
	Color* destinationData = &destination->data[destination->textureWidth * dy + dx];
	int destinationSkipX = destination->textureWidth - width;
	Color* sourceData = &source->data[source->textureWidth * sy + sx];
	int sourceSkipX = source->textureWidth - width;
	int x, y;
	for(y=0; y<height; y++, destinationData+=destinationSkipX, sourceData+=sourceSkipX)
	{
		for(x=0; x<width; x++, destinationData++, sourceData++) *destinationData = *sourceData;
	}
}

void drawImageBox(Image *source, Color background, Color border, int borderwidth)
{
	fillImageRect(source, background, 0, 0, source->imageWidth, source->imageHeight);
	int i, x = source->imageWidth - 1, y = source->imageHeight - 1;
	for(i=0; i<borderwidth; i++)
	{
	    drawLineInImage(source, border, i,         i,     x,     i);  // Top
		drawLineInImage(source, border, i,     y - i,     x, y - i);  // Bottom
		drawLineInImage(source, border, i,         i,     i,     y);  // Left
		drawLineInImage(source, border, x - i,     i, x - i, y - i);  // Right
	}
}