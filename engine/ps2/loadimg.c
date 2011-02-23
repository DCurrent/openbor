// Functions to load GIF, PCX and BMP files.
// Last update: 26-jan-2003
// Now loading to screens or bitmaps,
// creating them on-the-fly if necessary.


#include "ps2port.h"


#include "packfile.h"
#include "types.h"
#include "bitmap.h"
#include "screen.h"


#pragma pack (1)




// ============================== Globals ===============================

static int handle = -1;
static int current_res[2];	// Resolution of opened image


// ============================== BMP loading ===============================


typedef struct{
	unsigned short	bm;
	int		filesize;
	int		reserved;
	int		picstart;
	int		headersize;
	int		xsize;
	int		ysize;
	unsigned short	numplanes;
	unsigned short	bpp;
	int		compression;
	int		picsize;
	int		hres;
	int		vres;
	int		numcolors_used;
	int		numcolors_important;
}s_bmpheader;


static s_bmpheader bmp_header;

static void build_bmp_header(s_bmpheader *h, const unsigned char *s) {

  h->bm          = readlsb32(s+0x00);
  h->filesize    = readlsb32(s+0x02);
  h->reserved    = readlsb32(s+0x06);
  h->picstart    = readlsb32(s+0x0A);
  h->headersize  = readlsb32(s+0x0E);
  h->xsize       = readlsb32(s+0x12);
  h->ysize       = readlsb32(s+0x16);
  h->numplanes   = readlsb32(s+0x1A);
  h->bpp         = readlsb32(s+0x1C);
  h->compression = readlsb32(s+0x1E);
  h->picsize     = readlsb32(s+0x22);
  h->hres        = readlsb32(s+0x26);
  h->vres        = readlsb32(s+0x2A);
  h->numcolors_used      = readlsb32(s+0x2E);
  h->numcolors_important = readlsb32(s+0x32);
}

// Open a BMP stream
static int openbmp(char *filename, char *packfilename){

  unsigned char mybmpheader[0x36];

	if((handle=openpackfile(filename,packfilename))==-1) return 0;
	if(readpackfile(handle,&mybmpheader,0x36)!=0x36){
		closepackfile(handle);
		return 0;
	}

      build_bmp_header(&bmp_header,mybmpheader);

	if(bmp_header.bm!=0x4D42 || bmp_header.bpp!=8 || bmp_header.compression){
		closepackfile(handle);
		return 0;
	}
	current_res[0] = bmp_header.xsize;
	current_res[1] = bmp_header.ysize;
	return 1;
}



// Read data from the bitmap file
static int readbmp(char *buf, char *pal, int maxwidth, int maxheight){

	char *linebuffer;
//	s_bitmap * bitmap;

	int y,s,d;
//,x,filepos;


	if(buf){
		y = 0;
		while(y<maxheight && y<bmp_header.ysize){
			linebuffer = buf + y*maxwidth;
			seekpackfile(handle,bmp_header.picstart+((bmp_header.ysize-y-1)*bmp_header.xsize),SEEK_SET);
			readpackfile(handle,linebuffer,bmp_header.xsize);
			++y;
		}
	}

	if(pal && (linebuffer=(char*)tracemalloc("readbmp", 1024))){
		seekpackfile(handle,bmp_header.picstart-1024, SEEK_SET);
		readpackfile(handle,linebuffer,1024);
		for(s=0,d=0;s<1024;s+=4,d+=3){
		    pal[d] = linebuffer[s+2];
		    pal[d+1] = linebuffer[s+1];
		    pal[d+2] = linebuffer[s];
		}
		tracefree(linebuffer);
	}

	return 1;
}



// ============================== GIF loading ===============================



typedef struct{
	char		magic[6];
	unsigned short	screenwidth, screenheight;
        unsigned char	flags;
	unsigned char	background;
	unsigned char	aspect;
}gifheaderstruct;

#define sizeof_gifheaderstruct 13

typedef struct {
	unsigned short	left, top, width, height;
	unsigned char	flags;
}gifblockstruct;

#define sizeof_iblock 9

static gifheaderstruct gif_header;





static int readbyte(int handle){
	char c = 0;
	readpackfile(handle,&c,1);
	return c;
}


#define		NO_CODE		-1


static int decodegifblock(int handle, char *buf, int width, int height, char bits, gifblockstruct *gb){
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

    char *p;
    char *q;
    char b[255];
    char *u;
    char *linebuffer;

    static char firstcodestack[4096];
    static char lastcodestack[4096];
    static short codestack[4096];

    static short wordmasktable[] = {	0x0000, 0x0001, 0x0003, 0x0007,
					0x000f, 0x001f, 0x003f, 0x007f,
					0x00ff, 0x01ff, 0x03ff, 0x07ff,
					0x0fff, 0x1fff, 0x3fff, 0x7fff };

    static short inctable[] = { 8, 8, 4, 2, 0 };
    static short startable[] = { 0, 4, 2, 1, 0 };

    p = q = b;
    bitsleft = 8;

    if (bits < 2 || bits > 8) return 0;		// Bad symbol
    bits2 = 1 << bits;
    nextcode = bits2 + 2;
    codesize2 = 1 << (codesize = bits + 1);
    oldcode = oldtoken = NO_CODE;

    linebuffer = buf + (gb->top * width);

    // loop until something breaks
    for(;;){
	if(bitsleft == 8){
	    if(++p >= q && (((blocksize = readbyte(handle)) < 1) ||
		(q=(p=b) + readpackfile(handle, b, blocksize)) < (b+blocksize))){
		return 0;		// Unexpected EOF
	    }
	    bitsleft = 0;
	}
	thiscode = *p;
	if((currentcode=(codesize+bitsleft)) <= 8){
	    *p >>= codesize;
	    bitsleft = currentcode;
	}
	else{
	    if(++p >= q && (((blocksize = readbyte(handle)) < 1) ||
		(q=(p=b)+readpackfile(handle, b, blocksize)) < (b+blocksize)))
			return 0;		// Unexpected EOF

	    thiscode |= *p << (8 - bitsleft);
	    if(currentcode<=16) *p >>= (bitsleft = currentcode - 8);
	    else{
		if(++p >= q && (((blocksize = readbyte(handle)) < 1) ||
		  (q=(p=b) + readpackfile(handle, b, blocksize)) < (b+blocksize)))
			    return 0;		// Unexpected EOF

		thiscode |= *p << (16 - bitsleft);
		*p >>= (bitsleft = currentcode - 16);
	    }
	}
	thiscode &= wordmasktable[codesize];
	currentcode = thiscode;

	if(thiscode==(bits2+1)) break;
	if(thiscode>nextcode) return 0;			// Bad code

	if(thiscode==bits2){
	    nextcode = bits2 + 2;
	    codesize2 = 1 << (codesize = (bits+1));
	    oldtoken = oldcode = NO_CODE;
	    continue;
	}

	u = firstcodestack;

	if(thiscode==nextcode){
	    if(oldcode==NO_CODE) return 0;		// Bad code
	    *u++ = oldtoken;
	    thiscode = oldcode;
	}

	while(thiscode>=bits2){
	    *u++ = lastcodestack [thiscode];
	    thiscode = codestack[thiscode];
	}

	oldtoken = thiscode;
	do{
	    if(byte<width && line<(height - gb->top)) linebuffer[byte] = thiscode;
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



static void passgifblock(int handle){
	int len;

	// Discard extension function code
	len = readbyte(handle);
	// Skip all contained blocks
	while((len=readbyte(handle))!=0) seekpackfile(handle,len,SEEK_CUR);
}




static int opengif(char *filename, char *packfilename){

//debug_printf("opengif(filename=\"%s\", packfilename=\"%s\")\n", filename, packfilename);

	if((handle=openpackfile(filename,packfilename))==-1) {
//debug_printf("openpackfile failed\n");
          return 0;
        }

//debug_printf("ok 1\n");
//debug_printf("incidentally, sizeof(gifheaderstruct) = %d and sizeof_gifheaderstruct = %d\n", sizeof(gifheaderstruct), sizeof_gifheaderstruct);
	if(readpackfile(handle,&gif_header,sizeof_gifheaderstruct)!=sizeof_gifheaderstruct){
		closepackfile(handle);
		return 0;
	}
//debug_printf("ok 2\n");
	if(gif_header.magic[0]!='G' || gif_header.magic[1]!='I' || gif_header.magic[2]!='F'){
		// Not a GIF file!
		closepackfile(handle);
		return 0;
	}
//debug_printf("ok 3\n");
	current_res[0] = gif_header.screenwidth;
	current_res[1] = gif_header.screenheight;

	return 1;
}





static int readgif(char *buf, char *pal, int maxwidth, int maxheight){

	gifblockstruct iblock;
	int bitdepth;
	int numcolours;
	int i;
	int done = 0;
	char c;



	bitdepth = (gif_header.flags&7)+1;
	numcolours = (1<<bitdepth);


	// get palette if present and if wanted
	if(gif_header.flags&0x80){
		if(pal){
			if(readpackfile(handle,pal,numcolours*3) != numcolours*3){
				closepackfile(handle);
				return 0;
			}
		}
		else seekpackfile(handle,numcolours*3,SEEK_CUR);
	}


	if(!buf) return 1;


	// step through the blocks while(c==',' || c=='!' || c==0)
	while(!done){
		if(readpackfile(handle,&c,1)!=1) break;
		switch(c){
		    case ',':		// An image block
			if(readpackfile(handle, &iblock, sizeof_iblock)!=sizeof_iblock){
				return 0;
			}

			// get local palette if present and wanted
			if((iblock.flags&0x80) && pal){
			    if(pal){
				i = 3*(1<<((iblock.flags&0x0007)+1));
				if(readpackfile(handle,pal,i)!=numcolours){
					return 0;
				}
			    }
			    else seekpackfile(handle,numcolours*3,SEEK_CUR);
			}

			// get the initial LZW code bits
			if(readpackfile(handle,&c,1)!=1){
				return 0;
			}
			if(c<2 || c>8){
				return 0;
			}
			if(!decodegifblock(handle, buf, maxwidth, maxheight, c, &iblock)){
				return 0;
			}
			break;
		    case '!':
			// Extension block, read past it
			passgifblock(handle);
			break;
		    case 0:
			// Isn't this an EOF?
			break;
		    default:
			done = 1;
		}
	}
	return 1;
}



static void closegif(){
	closepackfile(handle);
	handle = -1;
}





// ============================== PCX loading ===============================





typedef struct{
	char	manufacturer;
	char	version;
	char	encoding;
	char	bitspp;	
	ushort	xmin, ymin;
	ushort	xmax, ymax;
	ushort	hres, vres;
	char	unused[48];
	char	reserved;
	char	colorplanes;
	ushort	bytesperline;
	ushort	paltype;
	char	unused2[58];
}s_pcxheader;

static s_pcxheader pcx_header;


static int openpcx(char *filename, char *packname){
	if((handle=openpackfile(filename, packname))==-1){
		return 0;
	}
	if(readpackfile(handle, &pcx_header, 128)!=128){
		closepackfile(handle);
		return 0;
	}
	if(pcx_header.colorplanes==3){
		closepackfile(handle);
		return 0;
	}

	current_res[0] = pcx_header.xmax;
	current_res[1] = pcx_header.ymax;

	return 1;
}



static int readpcx(char *buf, char *pal, int maxwidth, int maxheight){

	unsigned char *codebuffer;
	unsigned int i;
	unsigned x,y;
	unsigned spos;		// Searchpos (file)
	unsigned cpos;		// codepos
	unsigned dpos;		// Destination
	char repbyte;
//	s_bitmap * bitmap;



	if(buf){

		if(!(codebuffer=(char*)tracemalloc("readpcx", 64000))) return 0;

		x = 0;
		y = 0;
		spos = 0;
		dpos = 0;

		while(y<=pcx_header.ymax && y<maxheight){

			seekpackfile(handle, 128+spos, SEEK_SET);

			if((readpackfile(handle, codebuffer, 64000))==-1){
				tracefree(codebuffer);
				return 0;
			}

			cpos = 0;

			while(cpos<63990 && y<=pcx_header.ymax && y<maxheight){
				if(codebuffer[cpos]>192){
					i = codebuffer[cpos]-192;
					repbyte = codebuffer[cpos+1];
					do{
						if(x<maxwidth) buf[dpos] = repbyte;
						++dpos;
						if((++x)>=pcx_header.bytesperline){
							x=0;
							++y;
							dpos = y*maxwidth;
						}
					}while(--i);
					cpos+=2;
				}
				else{
					if(x<maxwidth) buf[dpos] = codebuffer[cpos];
					++cpos;
					++dpos;
					if((++x)>=pcx_header.bytesperline){
						x=0;
						++y;
						dpos = y*maxwidth;
					}
				}
			}
			spos+=cpos;
		}
		tracefree(codebuffer);
	}

	if(pal){
		seekpackfile(handle, -768, SEEK_END);
		readpackfile(handle, pal, 768);
	}
	return 1;
}



// ============================== auto loading ===============================

#define		OT_GIF		1
#define		OT_BMP		2
#define		OT_PCX		3


static int open_type = 0;

static int openimage(char *filename, char *packfile){
	char fnam[128];

	open_type = 0;

//debug_printf("openimage(filename=\"%s\", packfile=\"%s\"\n", filename, packfile);

	if(strlen(filename)>=128-4) return 0;

//debug_printf("trying gif\n");
	if(opengif(filename, packfile)){
		open_type = OT_GIF;
//debug_printf("ok\n");
		return 1;
	}
	sprintf(fnam, "%s.GIF", filename);
	if(opengif(fnam, packfile)){
		open_type = OT_GIF;
//debug_printf("ok\n");
		return 1;
	}

//debug_printf("trying pcx\n");
	if(openpcx(filename, packfile)){
		open_type = OT_PCX;
//debug_printf("ok\n");
		return 1;
	}
	sprintf(fnam, "%s.PCX", filename);
	if(openpcx(fnam, packfile)){
		open_type = OT_PCX;
//debug_printf("ok\n");
		return 1;
	}

//debug_printf("trying bmp\n");
	if(openbmp(filename, packfile)){
		open_type = OT_BMP;
//debug_printf("ok\n");
		return 1;
	}
	sprintf(fnam, "%s.BMP", filename);
	if(openbmp(fnam, packfile)){
		open_type = OT_BMP;
//debug_printf("ok\n");
		return 1;
	}

	return 0;
}


static int readimage(char *buf, char *pal, int maxwidth, int maxheight){
	switch(open_type){
		case OT_GIF:
			return readgif(buf, pal, maxwidth, maxheight);
		case OT_PCX:
			return readpcx(buf, pal, maxwidth, maxheight);
		case OT_BMP:
			return readbmp(buf, pal, maxwidth, maxheight);
	}
	return 0;
}


static void closeimage(){
	// All the same anyway...
	closegif();
}



// ============================== Interface ===============================




s_screen * loadscreen(char *filename, char *packfile, char *pal){
	int result;
	s_screen * screen;

//debug_printf("loadscreen(filename=\"%s\", packfile=\"%s\", pal)\n", filename, packfile);

	if(!openimage(filename, packfile)) {
//          debug_printf("loadscreen: openimage failed\n");
          return NULL;
        }

//debug_printf("loadscreen 2\n");

	screen = allocscreen(current_res[0], current_res[1]);
	if(screen == NULL){
		closeimage();
		return NULL;
	}
//debug_printf("loadscreen 3\n");
	result = readimage(screen->data, pal, screen->width, screen->height);
//debug_printf("loadscreen 4\n");
	closeimage();
	if(!result){
		freescreen(screen);
		return NULL;
	}
	return screen;
}




s_bitmap * loadbitmap(char *filename, char *packfile){
	int result;
	s_bitmap * bitmap;
	int maxwidth, maxheight;

	if(!openimage(filename, packfile)) return NULL;

	maxwidth = current_res[0];
	maxheight = current_res[1];
	if(maxwidth > 4096) maxwidth = 4096;
	if(maxheight > 4096) maxheight = 4096;

//debug_printf("loadbitmap %s w=%d h=%d\n", filename, maxwidth, maxheight);

	bitmap = allocbitmap(maxwidth, maxheight);
	if(!bitmap){
		closeimage();
		return NULL;
	}

	result = readimage(bitmap->data, NULL, maxwidth, maxheight);
	closeimage();
	if(!result){
		freebitmap(bitmap);
		return NULL;
	}
	return bitmap;
}





