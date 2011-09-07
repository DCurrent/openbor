/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

// Animated GIF player.

#include <stdio.h>
#include <string.h>
#include "packfile.h"
#include "borendian.h"
#include "types.h"
#include "screen.h"
#include "anigif.h"

// Animated GIF player.


static unsigned char readbyte(int handle){
	unsigned char c = 0;
	readpackfile(handle, &c, 1);
	return c;
}

static void passgifblock(anigif_info* info){
	int len;

	// Skip all contained blocks
	while((len=readbyte(info->handle))!=0) seekpackfile(info->handle, len, SEEK_CUR);
}



static void handle_gfx_control(anigif_info* info){
	int len;
	int skip;
	unsigned char buf[4];


	// Handle all contained blocks
	while((len=readbyte(info->handle))!=0){
		skip = len - 4;
		if(len>4) len = 4;
		readpackfile(info->handle, buf, len);
		if(skip>0) seekpackfile(info->handle, skip, SEEK_CUR);

		if(buf[0]&1) info->transparent = buf[3];
		info->lastdelay = (buf[2]<<8) | buf[1];

		// disposal = (buf[0]>>2) & 7;
		// inputflag = (buf[0]>>1) & 1;
	}
}



static void gifextension(anigif_info* info){
	int function;

	// Get extension function code
	function = readbyte(info->handle);

	// Note: function may be repeated multiple times (size, data, size, data)
	switch(function){
		case 0xF9:
			// Graphic control
			handle_gfx_control(info);
			return;
		default:
			// 0x01 = text
			// 0xFF = app. extension
			// 0xFE = comment
			passgifblock(info);
			return;
	}
}



/*
int readnonzero(int handle){
	char b = 0;
	while(b==0){
		if(readpackfile(handle, &b, 1) < 1) return 0;
	}
	return b;
}
*/




static int decodegifblock(anigif_info* info, unsigned char *buf, int width, int height, gifblockstruct *gb){

	unsigned char bits;
	short bits2;
	short codesize;
	short codesize2;
	short nextcode;
	short thiscode;
	short oldtoken;
	short currentcode;
	short oldcode;
	short bitsleft;
	short blocksize;
	int line = 0;
	int byte = gb->left;
	int pass = 0;

	unsigned char *p;
	unsigned char *u;

	unsigned char *q;
	unsigned char b[255];
	unsigned char *linebuffer;

	static unsigned char firstcodestack[4096];
	static unsigned char lastcodestack[4096];
	static short codestack[4096];

	static short wordmasktable[] = {0x0000, 0x0001, 0x0003, 0x0007,
					0x000f, 0x001f, 0x003f, 0x007f,
					0x00ff, 0x01ff, 0x03ff, 0x07ff,
					0x0fff, 0x1fff, 0x3fff, 0x7fff };

	static short inctable[] = { 8, 8, 4, 2, 0 };
	static short startable[] = { 0, 4, 2, 1, 0 };


	// get the initial LZW code bits
	bits = readbyte(info->handle);
	if(bits<2 || bits>8) return 0;


	p = q = b;
	bitsleft = 8;

	bits2 = 1 << bits;
	nextcode = bits2 + 2;
	codesize2 = 1 << (codesize = bits + 1);
	oldcode = oldtoken = NO_CODE;

	linebuffer = buf + (gb->top * width);

	// loop until something breaks
	for(;;){
		if(bitsleft == 8){
			if(++p >= q && (((blocksize = readbyte(info->handle)) < 1) ||
				(q=(p=b) + readpackfile(info->handle, b, blocksize)) < (b+blocksize))){
				return 1;	// Done
			}
			bitsleft = 0;
		}
		thiscode = *p;
		if((currentcode=(codesize+bitsleft)) <= 8){
			*p >>= codesize;
			bitsleft = currentcode;
		}
		else{
			if(++p >= q && (((blocksize = readbyte(info->handle)) < 1) ||
				(q=(p=b)+readpackfile(info->handle, b, blocksize)) < (b+blocksize))){
				return 1;	// Done
			}

			thiscode |= *p << (8 - bitsleft);
			if(currentcode<=16) *p >>= (bitsleft = currentcode - 8);
			else{
				if(++p >= q && (((blocksize = readbyte(info->handle)) < 1) ||
					(q=(p=b) + readpackfile(info->handle, b, blocksize)) < (b+blocksize))){
					return 1;	// Done
				}

				thiscode |= *p << (16 - bitsleft);
				*p >>= (bitsleft = currentcode - 16);
			}
		}
		thiscode &= wordmasktable[codesize];
		currentcode = thiscode;

		if(thiscode==(bits2+1)) break;
		if(thiscode>nextcode){
			return 0;			// Bad code
		}

		if(thiscode==bits2){
			nextcode = bits2 + 2;
			codesize2 = 1 << (codesize = (bits+1));
			oldtoken = oldcode = NO_CODE;
			continue;
		}

		u = firstcodestack;

		if(thiscode==nextcode){
			if(oldcode==NO_CODE){
				return 0;		// Bad code
			}
			*u++ = oldtoken;
			thiscode = oldcode;
		}

		while(thiscode>=bits2){
			*u++ = lastcodestack [thiscode];
			thiscode = codestack[thiscode];
		}

		oldtoken = thiscode;
		do{
			if(byte<width && byte>=0 && gb->top+line>=0 && line<(height - gb->top) && thiscode!=info->transparent) linebuffer[byte] = thiscode;
			byte++;
			if(byte >= gb->left + gb->width){
				byte = gb->left;
				// check for interlaced image
				if(gb->flags&0x40){
					line += inctable[pass];
					if(line >= gb->height) line = startable[++pass];
				}
				else ++line;
				linebuffer = buf + (width*(gb->top+line));
			}
			if (u<=firstcodestack) break;
			thiscode = *--u;
		}while(1);

		if(nextcode<4096 && oldcode!=NO_CODE){
			codestack[nextcode] = oldcode;
			lastcodestack[nextcode] = oldtoken;
			if(++nextcode>=codesize2 && codesize<12) codesize2 = 1<<++codesize;
		}
		oldcode = currentcode;
	}

	return 1;
}








void anigif_close(anigif_info* info){
	closepackfile(info->handle);
	info->handle = -1;
}



// Returns true on success
int anigif_open(char *filename, char *packfilename, unsigned char *pal, anigif_info* info){
	unsigned char tpal[1024];
	int i, j;

	memset(info, 0, sizeof(anigif_info));

	info->handle = -1;
	info->transparent = -1;

#if PSP || PS2 || DC
	if((info->handle=openreadaheadpackfile(filename,packfilename,1*1024*1024, 131072))==-1) return 0;
#else
	if((info->handle=openpackfile(filename,packfilename))==-1) return 0;
#endif

#if PSP || PS2 || DC
	if(readpackfile(info->handle,&info->gif_header,sizeof_gifheaderstruct)!=sizeof_gifheaderstruct){
#else
	if(readpackfile(info->handle,&info->gif_header,sizeof(gifheaderstruct))!=sizeof(gifheaderstruct)){
#endif
		anigif_close(info);
		return 0;
	}

	if(info->gif_header.magic[0]!='G' || info->gif_header.magic[1]!='I' || info->gif_header.magic[2]!='F'){
		// Not a GIF file!
		anigif_close(info);
		return 0;
	}

	//info->current_res[0] = SwapLSB16(info->gif_header.screenwidth);
	//info->current_res[1] = SwapLSB16(info->gif_header.screenheight);

	info->bitdepth = (info->gif_header.flags&7)+1;
	info->numcolours = (1<<info->bitdepth);
	info->lastdelay = 1;


	// Get global palette, if present and if wanted
	if(info->gif_header.flags&0x80){
		if(pal){
			if(readpackfile(info->handle, tpal, info->numcolours*3) != info->numcolours*3){

				anigif_close(info);
				return 0;
			}
			//*tpal &= 0xFF000000;
			switch(PAL_BYTES)
			{
			case 768:
				memcpy(pal, tpal, 768);
				break;
			case 1024:
				for(i=0, j=0; i<1024; i+=4, j+=3)
				{
					*(unsigned*)(pal+i) = colour32(tpal[j], tpal[j+1], tpal[j+2]);
				}
				break;
			case 512:
				for(i=0, j=0; i<512; i+=2, j+=3)
				{
					*(unsigned short*)(pal+i) = colour16(tpal[j], tpal[j+1], tpal[j+2]);
				}
				break;
			}
		}
		else seekpackfile(info->handle, info->numcolours*3, SEEK_CUR);
	}

	return 1;
}




// Returns type of action to take (frame, retry, end)
int anigif_decode(s_screen * screen, int *delay, int x, int y, anigif_info* info){

	gifblockstruct iblock;
	unsigned char c;


	if(info->handle<0) return ANIGIF_DECODE_END;
	if(screen==NULL){
		anigif_close(info);
		return ANIGIF_DECODE_END;
	}

	if(readpackfile(info->handle,&c,1)!=1){
		anigif_close(info);
		return ANIGIF_DECODE_END;
	}


	switch(c){
		case ',':
			// An image block
#if PSP || PS2 || DC
			if(readpackfile(info->handle, &iblock, sizeof_iblock)!=sizeof_iblock){
#else
			if(readpackfile(info->handle, &iblock, sizeof(iblock))!=sizeof(iblock)){
#endif
				anigif_close(info);
				return ANIGIF_DECODE_END;
			}

			// We don't do local palettes
			if(iblock.flags&0x80){
				seekpackfile(info->handle, info->numcolours*3, SEEK_CUR);
			}

			iblock.left = SwapLSB16(iblock.left);
			iblock.top = SwapLSB16(iblock.top);
			iblock.width = SwapLSB16(iblock.width);
			iblock.height = SwapLSB16(iblock.height);
			iblock.left += x;
			iblock.top += y;

			decodegifblock(info, (unsigned char*)screen->data, screen->width, screen->height, &iblock);
/*
			if(!decodegifblock(handle, screen->data, screen->width, screen->height, &iblock)){
				anigif_close();
				return ANIGIF_DECODE_END;
			}
*/

			if(delay) *delay = info->lastdelay;
			// lastdelay = 0;
			return ANIGIF_DECODE_FRAME;

		case '!':
			// Handle GIF extension
			gifextension(info);
			// if(delay) *delay = lastdelay;
			// lastdelay = 0;
			return ANIGIF_DECODE_RETRY;

//		case 0:
		default:
			// Isn't this an EOF?
			return ANIGIF_DECODE_RETRY;
//			anigif_close();
//			return ANIGIF_DECODE_END;
	}

	anigif_close(info);
	return ANIGIF_DECODE_END;
}








