/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <ogcsys.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include "types.h"
#include "video.h"
#include "vga.h"

#define mult2(X)			((X)<<1)
#define mult3(X)			(((X)<<1)+X)
#define mult4(X)			((X)<<2)
#define DEFAULT_FIFO_SIZE	(256*1024)
#define STACK_SIZE			16384

#define USEASM 1

static unsigned char gp_fifo[DEFAULT_FIFO_SIZE] ATTRIBUTE_ALIGN(32);
static GXRModeObj* vmode;
static void* xfb[2] = {NULL, NULL};
static int whichbuffer = 0;
static int whichtexture = 0;

static Mtx44 perspective;
static Mtx modelview;

static GXTexObj texture[2];
static void* texturemem[2] = {NULL, NULL};
static int texturemem_size;

static GXTlutObj tlut;
static unsigned short palette[256] ATTRIBUTE_ALIGN(32);

static int inited = 0;
static int stretch = 0;

static int brightness = 0;
static int brightness_update = 0;
static int gamma = 0;
static int gamma_update = 0;

int xoffset, yoffset;
int viewportWidth, viewportHeight; // resolution of TV screen
int scaledWidth, scaledHeight;
int textureWidth, textureHeight; // dimensions of game screen

static int bytes_per_pixel;

/**
 * Initializes GX, the API used for communicating with the Flipper/Hollywood GPU.
 */
void video_gx_init()
{
	GXColor background = {0, 0, 0, 0xff};
	
	// clear out FIFO area and initialize GX
	memset(gp_fifo, 0, DEFAULT_FIFO_SIZE);
	GX_Init(gp_fifo, DEFAULT_FIFO_SIZE);
 
	// clear the screen to black when GX_CopyDisp is called with clear=GX_TRUE
	GX_SetCopyClear(background, 0x00ffffff);
	
	// set up viewport
	GX_SetViewport(0, 0, vmode->fbWidth, vmode->efbHeight, 0, 1);
	GX_SetScissor(0, 0, vmode->fbWidth, vmode->efbHeight);
	
	// other GX setup
	GX_SetDispCopySrc(0, 0, vmode->fbWidth, vmode->efbHeight);
	GX_SetDispCopyDst(vmode->fbWidth, vmode->xfbHeight);
	GX_SetDispCopyYScale(GX_GetYScaleFactor(vmode->efbHeight, vmode->xfbHeight));
	GX_SetCopyFilter(vmode->aa, vmode->sample_pattern, GX_TRUE, vmode->vfilter);
	GX_SetPixelFmt(vmode->aa ? GX_PF_RGB565_Z16 : GX_PF_RGB8_Z24, GX_ZC_LINEAR);
	GX_SetCullMode(GX_CULL_NONE);
	GX_SetFieldMode(vmode->field_rendering, ((vmode->viHeight == 2*vmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));
	GX_SetColorUpdate(GX_TRUE);
	GX_SetZMode(GX_DISABLE, GX_ALWAYS, GX_DISABLE);
	
	// set up the vertex descriptors - tell the Flipper/Hollywood to expect direct, floating-point data
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	
	/* set up Texture Environment (TEV), although the brightness/gamma adjustment
	 * in video_render_screen() will override parts of this configuration later */
	GX_SetNumChans(0);
	GX_SetNumTexGens(1);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
	GX_SetNumTevStages(1);
	GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
	GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	vga_set_color_correction(0, 0);
	
	// set up an orthographic perspective
	guOrtho(perspective, 0, viewportHeight-1, 0, viewportWidth-1, 0, 300);
	GX_LoadProjectionMtx(perspective, GX_ORTHOGRAPHIC);
	
	// set up the model view
	guMtxIdentity(modelview);
	GX_LoadPosMtxImm(modelview, GX_PNMTX0);
	
	// initialize texture look-up table (TLUT) used in 8-bit (indexed) color mode
	GX_InitTlutObj(&tlut, palette, GX_TL_RGB565, 256);
}

/**
 * Initialize the video, including GX.  This functon calls VIDEO_Init(), which 
 * in turn initializes libogc, so this function must be called before making any
 * libogc calls.
 */
void video_init()
{
	if(inited) return;
	
	VIDEO_Init();
	vmode = VIDEO_GetPreferredMode(NULL);

	// widescreen fix
	viewportHeight = vmode->xfbHeight;
	viewportWidth = (CONF_GetAspectRatio() == CONF_ASPECT_16_9) ? viewportHeight*16/9 : vmode->fbWidth;
	
	VIDEO_Configure(vmode);
	
	// allocate 2 video buffers for double buffering
	xfb[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));
	xfb[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));
	
	// clear framebuffers, etc.
	VIDEO_ClearFrameBuffer(vmode, xfb[0], COLOR_BLACK);
	VIDEO_ClearFrameBuffer(vmode, xfb[1], COLOR_BLACK);
	VIDEO_SetNextFramebuffer(xfb[whichbuffer]);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(vmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

	// initialize GX
	video_gx_init();
	
	inited = 1;
	// finally, the video is up and ready for use :)
}

/**
 * Uninitialize the video.
 */
void video_exit()
{
	if(!inited) return;
	if(texturemem[0]) free(texturemem[0]);
	if(texturemem[1]) free(texturemem[1]);
	VIDEO_SetBlack(TRUE);
	VIDEO_Flush();
	inited = 0;
}

/**
 * Set the video mode.  This includes the width and height of the game screen, 
 * as well as the color mode: 8-bit indexed, 16-bit RGB565, or 32-bit RGB888.
 * @param videomodes the video mode structure (see source/gamelib/types.h)
 * @return 1 on success, 0 on error
 */
int video_set_mode(s_videomodes videomodes)
{
	float texscale;
	static const int texture_formats[4] = {GX_TF_CI8, GX_TF_RGB565, GX_TF_RGBA8, GX_TF_RGBA8};
	
	bytes_per_pixel = videomodes.pixel;
	
	// clear the embedded framebuffer (EFB) by copying its contents to the unused XFB :)
	GX_CopyDisp(xfb[whichbuffer^1], GX_TRUE);
	GX_Flush();
	
	// free existing texture memory
	if(texturemem[0]) { free(texturemem[0]); texturemem[0]=NULL; }
	if(texturemem[1]) { free(texturemem[1]); texturemem[1]=NULL; }
	
	// allocate memory for new texture
	textureWidth = videomodes.hRes;
	textureHeight = videomodes.vRes;
	texturemem_size = GX_GetTexBufferSize(textureWidth, textureHeight, texture_formats[bytes_per_pixel-1], 0, 0);
	texturemem[0] = memalign(32, texturemem_size);
	texturemem[1] = memalign(32, texturemem_size);

	// determine scale factor and on-screen dimensions
	texscale = MIN((float)viewportWidth/(float)textureWidth, (float)viewportHeight/(float)textureHeight);
	scaledWidth = (int)(textureWidth * texscale);
	scaledHeight = (int)(textureHeight * texscale);

	// determine offsets
	xoffset = (viewportWidth - scaledWidth) / 2;
	yoffset = (viewportHeight - scaledHeight) / 2;

	// initialize the texture object
	if(bytes_per_pixel == 1)
	{
		GX_InitTexObjCI(&texture[0], texturemem[0], textureWidth, textureHeight, GX_TF_CI8, GX_CLAMP, GX_CLAMP, GX_FALSE, GX_TLUT0);
		GX_InitTexObjCI(&texture[1], texturemem[1], textureWidth, textureHeight, GX_TF_CI8, GX_CLAMP, GX_CLAMP, GX_FALSE, GX_TLUT0);
	}
	else
	{
		GX_InitTexObj(&texture[0], texturemem[0], textureWidth, textureHeight, texture_formats[bytes_per_pixel-1], GX_CLAMP, GX_CLAMP, GX_FALSE);
		GX_InitTexObj(&texture[1], texturemem[1], textureWidth, textureHeight, texture_formats[bytes_per_pixel-1], GX_CLAMP, GX_CLAMP, GX_FALSE);
	}
	
	return 1;
}

/**
 * Renders a quad on the screen with the currently loaded texture and with the 
 * specified size and offset.
 * @param x the X offset in pixels
 * @param y the Y offset in pixels
 * @param width the width in pixels
 * @param height the height in pixels
 */
void video_draw_quad(int x, int y, int width, int height)
{
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		GX_Position2f32(x, y);					// top left
		GX_TexCoord2f32(0.0, 0.0);
		GX_Position2f32(x+width-1, y);			// top right
		GX_TexCoord2f32(1.0, 0.0);
		GX_Position2f32(x+width-1, y+height-1);	// bottom right
		GX_TexCoord2f32(1.0, 1.0);
		GX_Position2f32(x, y+height-1);			// bottom left
		GX_TexCoord2f32(0.0, 1.0);
	GX_End();
}

typedef void (*copyscreen_func)(s_screen*);

/**
 * Blits the game screen to the monitor/TV screen.
 * @param src the source image
 * @return 1 on success, 0 on error
 * TODO: stop using copyscreen_func and reduce function selection to a simple 
 * if/else which calls either video_swizzle_simple() or video_swizzle_rgba().
 */
int video_copy_screen(s_screen* src)
{
	static const copyscreen_func copyscreen_funcs[4] = {copyscreen8, copyscreen16, copyscreen32, copyscreen32};
	copyscreen_func copyscreen = copyscreen_funcs[bytes_per_pixel-1];
	
	whichtexture ^= 1;
	copyscreen(src);
	GX_DrawDone();
	
	// use the currently inactive video buffer for the next frame
	whichbuffer ^= 1;
	
	// upload the texture to the Flipper/Hollywood
	DCFlushRange(texturemem[whichbuffer], texturemem_size);
	GX_LoadTexObj(&texture[whichbuffer], GX_TEXMAP0);
	
	// draw the screen texture onto the EFB as a textured quad
	if(stretch)
		video_draw_quad(0, 0, viewportWidth, viewportHeight);
	else
		video_draw_quad(xoffset, yoffset, scaledWidth, scaledHeight);
	
	// blit the contents of the EFB to the XFB, and swap video buffers
	GX_CopyDisp(xfb[whichbuffer], GX_FALSE);
	GX_Flush();
	VIDEO_SetNextFramebuffer(xfb[whichbuffer]);
	VIDEO_Flush();
	
	return 1;
}

/**
 * Clear the next external framebuffer (XFB) to black.
 */
void video_clearscreen()
{
	whichbuffer ^= 1;
	whichtexture ^= 1;
	VIDEO_ClearFrameBuffer(vmode, xfb[whichbuffer], COLOR_BLACK);
	VIDEO_SetNextFramebuffer(xfb[whichbuffer]);
	VIDEO_Flush();
}

/**
 * Toggle whether the video should be stretched to take up the entire screen or 
 * use black bars when necessary to preserve the original aspect ratio.
 * 
 * @param enable 1 to stretch to screen, 0 to preserve aspect ratio
 * TODO: call GX_SetDispCopySrc() to prevent copying black bars from the EFB on 
 *       every update (maybe?)
 */
void video_stretch(int enable)
{
	GX_CopyDisp(xfb[whichbuffer^1], GX_TRUE); // clear the EFB
	GX_Flush();
	stretch = enable;
}

/**
 * Sets brightness and gamma correction.  Both are implemented using the 
 * Flipper/Hollywood's texture environment (TEV) unit.
 * 
 * @param br the desired brightness
 * @param gm the desired gamma
 */
void vga_set_color_correction(int gm, int br)
{
	if(br < -255) br = -255;
	if(br > 255) br = 255;
	if(gm < -255) gm = -255;
	if(gm > 255) gm = 255;
	
	if(br != brightness)
	{
		brightness = br;
		brightness_update = 1;
	}
	
	if(gm != gamma)
	{
		gamma = gm;
		gamma_update = 1;
	}
	
	// update brightness correction if changed since last frame
	if(brightness_update)
	{
		GXColor correction_color = {abs(brightness),abs(brightness),abs(brightness),abs(brightness)};
		brightness_update = 0;
		if(brightness == 0) GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
		else
		{
			// for brightness > 0, brightnesscorrect(c,b) = b+(c*(1.0-b))
			// for brightness < 0, brightnesscorrect(c,b) = c*(1.0+b)
			GX_SetTevColor(GX_TEVREG0, correction_color);
			GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_C0, brightness > 0 ? GX_CC_C0 : GX_CC_ZERO);
		}
		GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	}

	// update gamma correction if changed since last frame
	if(gamma_update)
	{
		GXColor correction_color = {abs(gamma),abs(gamma),abs(gamma),abs(gamma)};
		gamma_update = 0;
		if(gamma == 0) GX_SetNumTevStages(1);
		else
		{
			GX_SetNumTevStages(3);
			GX_SetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
			GX_SetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			GX_SetTevAlphaIn(GX_TEVSTAGE2, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
			GX_SetTevAlphaOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			GX_SetTevColor(GX_TEVREG1, correction_color);
	
			if(gamma > 0)
			{
				// for gamma > 0, gammacorrect(c,g) = 1.0-((1.0-c)*(1.0-(c*g)))
				// Stage 1: (1.0 - ((1.0-c)*0.0 + g*c))  = 1.0-(c*g)
				// Stage 2: (1.0 - ((1.0-c)*prev + 0*c)) = 1.0-((1.0-c)*(1.0-(c*g)))
				GX_SetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_C1, GX_CC_CPREV, GX_CC_ONE);
				GX_SetTevColorOp(GX_TEVSTAGE1, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVREG2);
				GX_SetTevColorIn(GX_TEVSTAGE2, GX_CC_C2, GX_CC_ZERO, GX_CC_CPREV, GX_CC_ONE);
				GX_SetTevColorOp(GX_TEVSTAGE2, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			}
			else
			{
				// for gamma < 0, gammacorrect(c,g) = c*(1.0-((1.0-c)*g))
				// Stage 1: (1.0 - ((1.0-c)*g + 0.0*c))  =    1.0-((1.0-c)*g)
				// Stage 2: (0.0 + ((1.0-c)*0 + prev*c)) = c*(1.0-((1.0-c)*g))
				GX_SetTevColorIn(GX_TEVSTAGE1, GX_CC_C1, GX_CC_ZERO, GX_CC_CPREV, GX_CC_ONE);
				GX_SetTevColorOp(GX_TEVSTAGE1, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVREG2);
				GX_SetTevColorIn(GX_TEVSTAGE2, GX_CC_ZERO, GX_CC_C2, GX_CC_CPREV, GX_CC_ZERO);
				GX_SetTevColorOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			}
		}
	}
}

/*
 * Blocks until a vertical retrace occurs.
 */
void vga_vwait(void)
{
	VIDEO_WaitVSync();
}

/**
 * Sets the palette used in 8-bit color mode.  The Wii supports indexed-color 
 * textures, so this function updates the texture lookup table (colormap) with 
 * the new palette.<br />
 * The palette is sent to this function as an array of 768 bytes.  Each color is 
 * 3 bytes: red, then green, then blue.
 * @param pal the new palette
 */
void vga_setpalette(unsigned char* pal)
{
	int i;
	for(i=0;i<256;i++)
	{
		palette[i] = (pal[0]>>3<<11) | (pal[1]>>2<<5) | (pal[2]>>3);
		pal+=3;
	}
	
	// upload the palette data to the Flipper
	GX_LoadTlut(&tlut, GX_TLUT0);
	DCFlushRange(palette, sizeof(palette));
}

/************ Functions to convert s_screen objects to textures ***************/

// TODO: move addition operations out of the copy loop for performance
// TODO: move this code into video_swizzle_simple()
// TODO: remove in favor of video_swizzle_simple
void copyscreen8(s_screen* src)
{
	u8* data = (u8*)src->data;
	u8* dest = (u8*)texturemem[whichtexture];
#if USEASM
	video_swizzle_simple(data, dest, src->width * sizeof(u8), src->height);
#else
	u16 x, y;
	
	// CI8 textures in GX are stored in 8x4 tiles
	for(y=0; y<textureHeight; y+=4)
	{
		for(x=0; x<textureWidth; x+=8)
		{
			memcpy(dest, data, 8);
			memcpy(dest+8, data+textureWidth, 8);
			memcpy(dest+16, data+mult2(textureWidth), 8);
			memcpy(dest+24, data+mult3(textureWidth), 8);
			data += 8;
			dest += 32;
		}
		data += mult3(textureWidth); // we're already at the end of the first line
	}
#endif
}

// TODO: move addition operations out of the copy loop for performance
// TODO: remove in favor of video_swizzle_simple
void copyscreen16(s_screen* src)
{
	u16* data = (u16*)src->data;
	u16* dest = (u16*)texturemem[whichtexture];
#if USEASM
	video_swizzle_simple(data, dest, src->width * sizeof(u16), src->height);
#else
	u16 x, y;
	
	// RGB565 textures in GX are stored in 4x4 tiles
	for(y=0; y<textureHeight; y+=4)
	{
		for(x=0; x<textureWidth; x+=4)
		{
			memcpy(dest, data, 8);
			memcpy(dest+4, data+textureWidth, 8);
			memcpy(dest+8, data+mult2(textureWidth), 8);
			memcpy(dest+12, data+mult3(textureWidth), 8);
			data += 4;
			dest += 16;
		}
		data += mult3(textureWidth); // we're already at the end of the first line
	}
#endif
}

// copies four 32-bit colors from an s_screen structure to texture memory
// TODO: remove when copyscreen32 is removed
static inline void memcpy32(u16* dest, u32* src)
{
	int i;
	u32 color;
	for(i=0; i<4; i++)
	{
		color = *(src+i);
		*(dest+i) = color >> 16; // AR cache line
		*(dest+i+16) = color; // GB cache line
	}
}

/* TODO: remove in favor of video_swizzle_rgba() */
void copyscreen32(s_screen* src)
{
	u16 x, y;
	u32* data = (u32*)src->data;
	u16* dest = (u16*)texturemem[whichtexture];
	
	// RGBA8 textures in GX are stored in 4x4 tiles in two cache lines, AR and GB
	for(y=0; y<textureHeight; y+=4)
	{
		for(x=0; x<textureWidth; x+=4)
		{
			memcpy32(dest, data);
			memcpy32(dest+4, data+textureWidth);
			memcpy32(dest+8, data+mult2(textureWidth));
			memcpy32(dest+12, data+mult3(textureWidth));
			data += 4;
			dest += 32;
		}
		data += mult3(textureWidth); // we're already at the end of the first line
	}
}

/**
 * Copies and swizzles an image to texture memory.  Works for 8-bit (GX_TF_CI8) 
 * and 16-bit (GX_TF_RGB565) texture formats.  Both formats are divided into tiles 
 * containing 4 rows of 8 bytes.
 * 
 * FIXME: test new C code imported from copyscreen8()
 * TODO: does the ASM version really even give a performance advantage?
 * @param src - the image
 * @param dest - the texture memory
 * @param width - the width of each row in *BYTES*, i.e. not in pixels; must be a multiple of 8
 * @param height - number of rows; must be a multiple of 4
 */
void video_swizzle_simple(const void* src, void* dst, s32 width, s32 height)
{
#if !USEASM
	u8* data = (u8*)src;
	u8* dest = (u8*)dst;
	s32 widthx2 = width * 2;
	s32 widthx3 = width * 3;
	int x, y;
	
	/**
	 * GX_TF_CI8 textures are stored with 1 byte per pixel in tiles 8 pixels 
	 * by 4 pixels.  GX_TF_RGB565 textures have 2 bytes per pixel, with 4 pixels 
	 * in each row.  So tiles in both texture formats are 4 rows of 8 bytes each.
	 */
	for(y=0; y<height; y+=4)
	{
		for(x=0; x<width; x+=8)
		{
			memcpy(dest, data, 8);
			memcpy(dest+8, data+width, 8);
			memcpy(dest+16, data+widthx2, 8);
			memcpy(dest+24, data+widthx3, 8);
			data += 8;
			dest += 32;
		}
		data += widthx3; // we're already at the end of the first line
	}
#else
	register u32 tmp0=0,tmp1=0,tmp2=0,tmp3=0,tmp4=0,tmp5=0,tmp6=0,tmp7=0,tmp8=0,tmp9=0,tmp10=0;

	__asm__ __volatile__ (
		"	srwi		%13,%13,3\n"		// width = width >> 3				[ divide width by 8 due to 8-byte tile width ]
		"	srwi		%14,%14,2\n"		// height = height >> 2				[ divide height by 4 since each tile has 4 rows ]
		"	mulli		%4,%13,8\n"			// tmp4 = width * 8					[ calculate 1x row width in bytes ]
		"	mulli		%5,%13,16\n"		// tmp5 = width * 16				[ calculate 2x row width in bytes ]
		"	mulli		%6,%13,24\n"		// tmp6 = width * 24				[ calculate 3x row width in bytes ]
		"	mulli		%7,%13,32\n"		// tmp7 = width * 32				[ calculate 4x row width in bytes ]
		"	addi		%8,%4,4\n"			// tmp8 = tmp4 + 4					[ calculate 4+1x row width in bytes ]
		"	addi		%9,%5,4\n"			// tmp9 = tmp5 + 4					[ calculate 4+2x row width in bytes ]
		"	addi		%10,%6,4\n"			// tmp10 = tmp6 + 4					[ calculate 4+3x row width in bytes ]
		"	subi		%3,%11,4\n"			// tmp3 = dst - 4
		"	subi		%11,%11,8\n"		// dst -= 8

		"2:	mtctr		%13\n"				// count = width					[ set loop variable (count) equal to width ]
		"	mr			%0,%12\n"			// tmp0 = src

		"1:	lwz			%1,0(%12)\n"		// tmp1 = *src						[ copy 8 bytes from first row to texture ]
		"	stwu		%1,8(%11)\n"		// *(dst+8) = tmp1; dst += 8
		"	lwz			%2,4(%12)\n"		// tmp2 = *(src+4)
		"	stwu		%2,8(%3)\n"			// *(tmp3+8) = tmp2; tmp3 += 8

		"	lwzx		%1,%12,%4\n"		// tmp1 = *(src+tmp4)				[ copy 8 bytes from second row to texture ]
		"	stwu		%1,8(%11)\n"		// *(dst+8) = tmp1; dst += 8
		"	lwzx		%2,%12,%8\n"		// tmp2 = *(src+tmp8)
		"	stwu		%2,8(%3)\n"			// *(tmp3+8) = tmp2; tmp3 += 8

		"	lwzx		%1,%12,%5\n"		// tmp1 = *(src+tmp5)				[ copy 8 bytes from third row to texture ]
		"	stwu		%1,8(%11)\n"		// *(dst+8) = tmp1; dst += 8
		"	lwzx		%2,%12,%9\n"		// tmp2 = *(src+tmp9)
		"	stwu		%2,8(%3)\n"			// *(tmp3+8) = tmp2; tmp3 += 8

		"	lwzx		%1,%12,%6\n"		// tmp1 = *(src+tmp6)				[ copy 8 bytes from fourth row to texture ]
		"	stwu		%1,8(%11)\n"		// *(dst+8) = tmp1; dst += 8
		"	lwzx		%2,%12,%10\n"		// tmp2 = *(src+tmp10)
		"	stwu		%2,8(%3)\n"			// *(tmp3+8) = tmp2; tmp3 += 8

		"	addi		%12,%12,8\n"		// src += 8							[ move right by 8 bytes ]
		"	bdnz		1b\n"				// count--; if(count) goto 1		[ return to "1" if more pixels in this row ]

		"	add			%12,%0,%7\n"		// src = tmp0 + tmp7				[ move down by 4 rows of pixels ]
		"	subic.		%14,%14,1\n"		// height -= 1						[ decrement height and record to CR0 ]
		"	bne			2b"					// if(height != 0) goto 2			[ return to "2" if more rows remaining ]
		//		 0			  1			   2		    3            4            5
		: "=&b"(tmp0), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3), "=&r"(tmp4), "=&r"(tmp5), 
		//       6            7            8            9            10
		  "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8), "=&r"(tmp9), "=&r"(tmp10)
		//	   11		 12		   13		   14
		: "b"(dst), "b"(src), "r"(width), "r"(height)
		: "memory"
	);
#endif
}

/**
 * Copies and swizzles an image to texture memory.  Works for 32-bit (GX_TF_RGBA8)
 * texture format.  Textures are stored into tiles containing 4 rows of 8 bytes.
 * However, each tile is 64 bytes in size because each tile is divided across two 
 * cache lines.  So each 4x4 tile is a 32-byte AR block followed by a 32-byte GB block.
 * 
 * TODO: test code imported from copyscreen32()!!
 * TODO: implement in ASM
 * @param src the image
 * @param dest the texture memory
 * @param width the width of each row in *BYTES*, i.e. not in pixels; must be a multiple of 16
 * @param height number of rows; must be a multiple of 4
 */
void video_swizzle_rgba(const void* src, void* dst, s32 width, s32 height)
{
	// copies four 32-bit colors to texture memory
	// FIXME: code is ugly
	void memcpy32(void* vdest, void* vsource)
	{
		int i;
		u32* source = (u32*)vsource;
		u16* dest = (u16*)vdest;
		u32 color;
		for(i=0; i<4; i++)
		{
			color = *(source+i);
			*(dest+i) = color >> 16; // AR cache line
			*(dest+i+16) = color; // GB cache line
		}
	}
	
	u8* data = (u8*)src;
	u8* dest = (u8*)dst;
	s32 widthx2 = width * 2;
	s32 widthx3 = width * 3;
	int x, y;
	
	// GX_TF_RGBA8 textures are stored in 4x4 tiles in two cache lines, AR and GB
	for(y=0; y<height; y+=4)
	{
		for(x=0; x<width; x+=16)
		{
			memcpy32(dest, data);
			memcpy32(dest+16, data+width);
			memcpy32(dest+32, data+widthx2);
			memcpy32(dest+48, data+widthx3);
			data += 16;
			dest += 64;
		}
		data += widthx3; // we're already at the end of the first line
	}
}

