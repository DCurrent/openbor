/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

/*
	Save an 8-bit PCX file... Useful for saving screenshots.

	This code is pretty good, I spent quite some time making sure
	the header is filled correctly. I found that many PCX writers
	'forget' certain bytes, such as the palette ID byte.

	Last update: 5-feb-2003

	Changes:
	- Used per-line encoding, which is more compatible.
	- Added byte-padding for odd-width images (untested).
*/


#include "borendian.h"
#include "types.h"
#include "packfile.h"
#include <fcntl.h>
#include <malloc.h>
#include <string.h>


#define	RLE_BUF_SIZE 2048


#pragma pack (1)


typedef struct pcx_header{
	char  manufacturer;    // Always 0x0A!
	char  version;         // 5?
	char  encoding;        // 1 (RLE-encoded)
	char  bpp;             // 8 (our pics are always 8-bit)
	short xmin, ymin;      // Always 0
	short xmax, ymax;      // Size-1
	short hres, vres;      // Screen resolution (DPI)
	char  lowcolorpal[48]; // Palette for 16-color images, unused
	char  reserved;
	char  colorplanes;     // 1 for 8-bit
	short bpl;             // Bytes per line, width padded even
	short paltype;         // 0
	short hscreensize, vscreensize;
	char  unused2[54];
}pcx_header;


int savepcx24(char* filename, s_screen * screen)
{
	int success = 1;
	unsigned char *RLEbuf = NULL;
	unsigned char *pixelbuf = NULL, *srcptr;
	pcx_header * head = NULL;
	int r, p, i;
	int handle = -1;
	unsigned char *lineptr;
	unsigned c;
	int x, y;

	if(!screen) return 0;

	// Allocate buffers
	if((RLEbuf=malloc(RLE_BUF_SIZE*3))==NULL){
		success = 0;
		goto END;
	}
	if((head=(pcx_header *)malloc(sizeof(pcx_header)))==NULL){
		success = 0;
		goto END;
	}
	if(screen->pixelformat==PIXEL_16)
	{
		if((pixelbuf=malloc(screen->width*screen->height*4))==NULL)
		{
			success = 0;
			goto END;
		}
	}
	else if(screen->pixelformat!=PIXEL_32)
	{
		success = 0;
		goto END;
	}

	// Fill the header

	memset(head, 0, sizeof(pcx_header));
	head->manufacturer = 0x0A;
	head->version = 5;
	head->encoding = 1;
	head->bpp = 8;
	head->xmin = 0;
	head->ymin = 0;
	head->xmax = SwapLSB16(screen->width - 1);
	head->ymax = SwapLSB16(screen->height - 1);
	head->hres = SwapLSB16(screen->width);
	head->vres = SwapLSB16(screen->height);
	head->hscreensize = SwapLSB16(screen->width);
	head->vscreensize = SwapLSB16(screen->height);
	head->colorplanes = 3;
	head->bpl = SwapLSB16((screen->width+1) & 0xFFFE);
	head->paltype = 1;


	if((handle=open(filename, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0777))<=0){
		success = 0;
		goto END;
	}
	if(write(handle, head, sizeof(pcx_header))!=sizeof(pcx_header)){
		success = 0;
		goto END;
	}

	if(pixelbuf)
	{
		for(i=screen->height*screen->width-1; i>=0; i--)
		{
			c = ((unsigned short*)screen->data)[i];
			((unsigned*)pixelbuf)[i] = colour32((c&0x1F)*0xFF/0x1F, ((c>>5)&0x3F)*0xFF/0x3F, ((c>>11)&0x1F)*0xFF/0x1F);
		}
	}


	// Start the coding loop

	x = 0;
	y = 0;
	srcptr = pixelbuf?pixelbuf:screen->data;
	while(y < screen->height){
	    lineptr = srcptr + y*screen->width*4;
	    // wirte 3 colour planes
		for(i=0; i<3; i++)
		{
			// To start of RLE buffer
			r = 0;
			x = 0;
#define px ((x<<2)+i)
			while(x<screen->width){
				if(lineptr[px]>=0xC0 || (x<screen->width-1 && lineptr[px+4]==lineptr[px])){
					RLEbuf[r+1] = lineptr[px];
					RLEbuf[r] = 0;
					while(x<screen->width && lineptr[px]==RLEbuf[r+1] && RLEbuf[r]<63){
						++RLEbuf[r];
						++x;
					}
					RLEbuf[r] |= 0xC0;
					r+=2;
				}
				else{
					RLEbuf[r] = lineptr[px];
					++r;
					++x;
				}
			}
#undef px
			// Reached end of line?
			if(x>=screen->width){
				// Odd-width byte padding?
				if(screen->width&1){
					RLEbuf[r] = 0;
					++r;
				}
			}

			// Write RLE code to file
			if(write(handle, RLEbuf, r)!=r){
				success = 0;
				goto END;
			}
		}
		++y;
	}


	// Write palette ID
	p = 0x0C;/*
	if(write(handle, &p, 1)!=1){
		success = 0;
		goto END;
	}

	// Write palette
	if(write(handle, pal, 768)!=768){
		success = 0;
		goto END;
	}
*/
END:
	if(handle != -1) close(handle);
	if(RLEbuf != NULL){
		free(RLEbuf);
		RLEbuf = NULL;
	}
	if(head != NULL){
		free(head);
		head = NULL;
	}
	if(pixelbuf != NULL)
	{
		free(pixelbuf);
		pixelbuf = NULL;
	}

	return success;
}


int savepcx8(char *filename, s_screen *screen, unsigned char *pal){
	int success = 1;
	unsigned char *RLEbuf = NULL;
	pcx_header * head = NULL;
	int r, p;
	int handle = -1;
	unsigned char *lineptr;
	int x, y;


	if(!screen) return 0;
	if(!pal) return 0;


	// Allocate buffers
	if((RLEbuf=malloc(RLE_BUF_SIZE))==NULL){
		success = 0;
		goto END;
	}
	if((head=(pcx_header *)malloc(sizeof(pcx_header)))==NULL){
		success = 0;
		goto END;
	}


	// Fill the header

	memset(head, 0, sizeof(pcx_header));
	head->manufacturer = 0x0A;
	head->version = 5;
	head->encoding = 1;
	head->bpp = 8;
	head->xmin = 0;
	head->ymin = 0;
	head->xmax = SwapLSB16(screen->width - 1);
	head->ymax = SwapLSB16(screen->height - 1);
	head->hres = SwapLSB16(screen->width);
	head->vres = SwapLSB16(screen->height);
	head->hscreensize = SwapLSB16(screen->width);
	head->vscreensize = SwapLSB16(screen->height);
	head->colorplanes = 1;
	head->bpl = SwapLSB16((screen->width+1) & 0xFFFE);
	head->paltype = 0;



	if((handle=open(filename, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0777))<=0){
		success = 0;
		goto END;
	}
	if(write(handle, head, sizeof(pcx_header))!=sizeof(pcx_header)){
		success = 0;
		goto END;
	}


	// Start the coding loop

	y = 0;
	x = 0;
	lineptr = screen->data + y*screen->width;
	while(y < screen->height){

		// To start of RLE buffer
		r = 0;

		while(r<RLE_BUF_SIZE-4 && x<screen->width){
			if(lineptr[x]>=0xC0 || (x<screen->width-1 && lineptr[x+1]==lineptr[x])){
				RLEbuf[r+1] = lineptr[x];
				RLEbuf[r] = 0;
				while(x<screen->width && lineptr[x]==RLEbuf[r+1] && RLEbuf[r]<63){
					++RLEbuf[r];
					++x;
				}
				RLEbuf[r] |= 0xC0;
				r+=2;
			}
			else{
				RLEbuf[r] = lineptr[x];
				++r;
				++x;
			}
		}

		// Reached end of line?
		if(x>=screen->width){
			++y;
			lineptr = screen->data + y*screen->width;
			x = 0;

			// Odd-width byte padding?
			if(screen->width&1){
				RLEbuf[r] = 0;
				++r;
			}
		}

		// Write RLE code to file
		if(write(handle, RLEbuf, r)!=r){
			success = 0;
			goto END;
		}
	}


	// Write palette ID
	p = 0x0C;
	if(write(handle, &p, 1)!=1){
		success = 0;
		goto END;
	}

	// Write palette
	if(write(handle, pal, 768)!=768){
		success = 0;
		goto END;
	}

END:
	if(handle != -1) close(handle);
	if(RLEbuf != NULL){
		free(RLEbuf);
		RLEbuf = NULL;
	}
	if(head != NULL){
		free(head);
		head = NULL;
	}

	return success;
}


int savepcx(char *filename, s_screen *screen, unsigned char *pal)
{
	if(screen->pixelformat==PIXEL_8) return savepcx8(filename, screen, pal);
	else                             return savepcx24(filename, screen);
}

