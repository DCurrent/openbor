/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

///////////////////////////////////////////////////////////////////////////
//         This file defines some commmon methods used by the gamelib
////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "globals.h"
#include "borendian.h"

#ifndef NULL
#define NULL (void*)0
#endif

#pragma pack(1)
typedef struct
{
    union
    {
        struct
        {
#ifdef BOR_BIG_ENDIAN
            unsigned char a; //unused
            unsigned char b;
            unsigned char g;
            unsigned char r;
#else
	        unsigned char r;
            unsigned char g;
            unsigned char b;
            unsigned char a; //unused
#endif
        } C;
        unsigned c;
    };
} RGB32;

typedef struct
{
    union
    {
        struct
        {
            unsigned r: 5;
            unsigned g: 6;
            unsigned b: 5;
            unsigned a: 16; //unused
        } C;
        unsigned c;
    };
} RGB16;

int pixelformat = PIXEL_8;
int screenformat = PIXEL_8;
int pixelbytes[(int)5] = {1, 1, 2, 3, 4};

unsigned channelr, channelg, channelb, tintcolor, tintmode;
blend16fp tint16fp1, tint16fp2;
blend32fp tint32fp1, tint32fp2;
int usechannel;
int useclip;
int clipx1, clipy1, clipx2, clipy2;

//may be used many time so make a function
void drawmethod_global_init(s_drawmethod *drawmethod)
{
    if(screenformat != PIXEL_8)
    {
        if(drawmethod && drawmethod->flag)
        {
#if REVERSE_COLOR
            channelr = drawmethod->channelb;
            channelb = drawmethod->channelr;
#else
            channelr = drawmethod->channelr;
            channelb = drawmethod->channelb;
#endif
            channelg = drawmethod->channelg;
			tintmode = drawmethod->tintmode;
            tintcolor = drawmethod->tintcolor;
            usechannel = (channelr < 255) || (channelg < 255) || (channelb < 255);
        }
        else
        {
            usechannel = tintmode = 0;
        }
    }

    if((useclip = drawmethod && drawmethod->flag && drawmethod->clipw > 0 && drawmethod->cliph > 0))
    {
        clipx1 = drawmethod->clipx;
        clipy1 = drawmethod->clipy;
        clipx2 = clipx1 + drawmethod->clipw;
        clipy2 = clipy1 + drawmethod->cliph;
    }
}


unsigned short colour16(unsigned char r, unsigned char g, unsigned char b)
{
#if REVERSE_COLOR
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
#else
    return ((b & 0xF8) << 8) | ((g & 0xFC) << 3) | (r >> 3);
#endif
}

unsigned colour32(unsigned char r, unsigned char g, unsigned char b)
{
#if REVERSE_COLOR
    return ((r << 16) | (g << 8) | b);
#else
    return ((b << 16) | (g << 8) | r);
#endif
}

/*
#define bs ((color1&0xff0000)>>8)
#define gs (color1&0xFF00)
#define rs ((color1&0xFF)<<8)
#define bd (color2>>16)
#define gd ((color2&0xFF00)>>8)
#define rd (color2&0xFF)
#define bi (bs|bd)
#define gi (gs|gd)
#define ri (rs|rd)*/
#define bs (((RGB32*)&color1)->C.b)
#define gs (((RGB32*)&color1)->C.g)
#define rs (((RGB32*)&color1)->C.r)
#define bd (((RGB32*)&color2)->C.b)
#define gd (((RGB32*)&color2)->C.g)
#define rd (((RGB32*)&color2)->C.r)
#define bi ((bs<<8)|bd)
#define gi ((gs<<8)|gd)
#define ri ((rs<<8)|rd)
#define _multiply(c1,c2) (((c1)*(c2))>>8)
#define _screen(c1,c2) ((((c1)^255)*((c2)^255)/255)^255)
#define _hardlight(c1,c2) ((c1)<128?_multiply((c1)<<1,(c2)):_screen(((c1)-128)<<1,(c2)))
#define _overlay(c1,c2) ((c2)<128?_multiply((c2)<<1,(c1)):_screen(((c2)-128)<<1,(c1)))
#define _dodge(c1,c2) (((c2)<<8)/(256-(c1)))
#define _channel(src,dest,alpha) (((src*alpha)+(dest*(255-alpha)))>>8)
#define _color(r,g,b) (((b)<<16)|((g)<<8)|(r))


// common blend function
unsigned blend_multiply(unsigned color1, unsigned color2)
{
    return _multiply(color1, color2);
}

unsigned blend_screen(unsigned color1, unsigned color2)
{
    return _screen(color1, color2);
}

unsigned blend_overlay(unsigned color1, unsigned color2)
{
    return _overlay(color1, color2);
}

unsigned blend_hardlight(unsigned color1, unsigned color2)
{
    return _hardlight(color1, color2);
}

unsigned blend_dodge(unsigned color1, unsigned color2)
{
    unsigned c = _dodge(color1, color2);
    return c > 255 ? 255 : c;
}

unsigned blend_half(unsigned color1, unsigned color2)
{
    return (color1 + color2) >> 1;
}

/////////////////////// blend 2 32bit colours //////////////////////
// color1 front colour,
// color2 bg colour
////////////////////////////////////////////////////////////////////

unsigned char *create_multiply32_tbl()
{
    unsigned i, j;
    unsigned char *tbl = (unsigned char *)malloc(256 * 256);
    for(i = 0; i < 256; i++)
        for(j = 0; j < 256; j++)
        {
            tbl[(i << 8) | j] = (unsigned char)blend_multiply(i, j);
        }

    return tbl;
}


unsigned blend_multiply32(unsigned color1, unsigned color2)
{
    unsigned char *tbl;
    if((tbl = blendtables[BLEND_MULTIPLY]))
    {
        return _color(tbl[ri], tbl[gi], tbl[bi]);
    }
    return _color(_multiply(color1 >> 16, color2 >> 16),
                  _multiply((color1 & 0xFF00) >> 8, (color2 & 0xFF00) >> 8),
                  _multiply(color1 & 0xFF, color2 & 0xFF));
}

unsigned char *create_screen32_tbl()
{
    unsigned i, j;
    unsigned char *tbl = (unsigned char *)malloc(256 * 256);
    for(i = 0; i < 256; i++)
        for(j = 0; j < 256; j++)
        {
            tbl[(i << 8) | j] = (unsigned char)blend_screen(i, j);
        }

    return tbl;
}

unsigned blend_screen32(unsigned color1, unsigned color2)
{
    unsigned char *tbl;
    if((tbl = blendtables[BLEND_SCREEN]))
    {
        return _color(tbl[ri], tbl[gi], tbl[bi]);
    }
    return _color(_screen(color1 >> 16, color2 >> 16),
                  _screen((color1 & 0xFF00) >> 8, (color2 & 0xFF00) >> 8),
                  _screen(color1 & 0xFF, color2 & 0xFF));
}

unsigned char *create_overlay32_tbl()
{
    unsigned i, j;
    unsigned char *tbl = (unsigned char *)malloc(256 * 256);
    for(i = 0; i < 256; i++)
        for(j = 0; j < 256; j++)
        {
            tbl[(i << 8) | j] = (unsigned char)blend_overlay(i, j);
        }

    return tbl;
}

unsigned blend_overlay32(unsigned color1, unsigned color2)
{
    int r1, g1, b1, r2, g2, b2;
    unsigned char *tbl;
    if((tbl = blendtables[BLEND_OVERLAY]))
    {
        return _color(tbl[ri], tbl[gi], tbl[bi]);
    }
    b1 = color1 >> 16, b2 = color2 >> 16;
    g1 = (color1 & 0xFF00) >> 8, g2 = (color2 & 0xFF00) >> 8;
    r1 = color1 & 0xFF, r2 = color2 & 0xFF;
    return _color(_overlay(r1, r2),
                  _overlay(g1, g2),
                  _overlay(b1, b2));
}

unsigned char *create_hardlight32_tbl()
{
    unsigned i, j;
    unsigned char *tbl = (unsigned char *)malloc(256 * 256);
    for(i = 0; i < 256; i++)
        for(j = 0; j < 256; j++)
        {
            tbl[(i << 8) | j] = (unsigned char)blend_hardlight(i, j);
        }

    return tbl;
}

unsigned blend_hardlight32(unsigned color1, unsigned color2)
{
    int r1, g1, b1, r2, g2, b2;
    unsigned char *tbl;
    if((tbl = blendtables[BLEND_HARDLIGHT]))
    {
        return _color(tbl[ri], tbl[gi], tbl[bi]);
    }
    b1 = color1 >> 16, b2 = color2 >> 16;
    g1 = (color1 & 0xFF00) >> 8, g2 = (color2 & 0xFF00) >> 8;
    r1 = color1 & 0xFF, r2 = color2 & 0xFF;
    return _color(_hardlight(r1, r2),
                  _hardlight(g1, g2),
                  _hardlight(b1, b2));
}

unsigned char *create_dodge32_tbl()
{
    unsigned i, j;
    unsigned char *tbl = (unsigned char *)malloc(256 * 256);
    for(i = 0; i < 256; i++)
        for(j = 0; j < 256; j++)
        {
            tbl[(i << 8) | j] = (unsigned char)blend_dodge(i, j);
        }
    return tbl;
}

unsigned blend_dodge32(unsigned color1, unsigned color2)
{
    unsigned r, g, b;
    unsigned char *tbl;
    if((tbl = blendtables[BLEND_DODGE]))
    {
        return _color(tbl[ri], tbl[gi], tbl[bi]);
    }
    b = _dodge(color1 >> 16, color2 >> 16);
    g = _dodge((color1 & 0xFF00) >> 8, (color2 & 0xFF00) >> 8);
    r = _dodge(color1 & 0xFF, color2 & 0xFF);
    return _color(r > 255 ? 255 : r, g > 255 ? 255 : g, b > 255 ? 255 : b);
}

unsigned char *create_half32_tbl()
{
    unsigned i, j;
    unsigned char *tbl = (unsigned char *)malloc(256 * 256);
    for(i = 0; i < 256; i++)
        for(j = 0; j < 256; j++)
        {
            tbl[(i << 8) | j] = (unsigned char)blend_half(i, j);
        }

    return tbl;
}

unsigned blend_half32(unsigned color1, unsigned color2)
{
    unsigned char *tbl;
    if((tbl = blendtables[BLEND_HALF]))
    {
        return _color(tbl[ri], tbl[gi], tbl[bi]);
    }
    return _color(((color1 >> 16) + (color2 >> 16)) >> 1,
                  (((color1 & 0xFF00) >> 8) + ((color2 & 0xFF00) >> 8)) >> 1,
                  ((color1 & 0xFF) + (color2 & 0xFF)) >> 1);
}

unsigned blend_tint32(unsigned color1, unsigned color2)
{
    unsigned c = tint32fp1(tintcolor, color1);
    return tint32fp2 ? tint32fp2(c, color2) : c;
}

//copy from below
unsigned blend_rgbchannel32(unsigned color1, unsigned color2)
{
    unsigned b1 = color1 >> 16, b2 = color2 >> 16;
    unsigned g1 = (color1 & 0xFF00) >> 8, g2 = (color2 & 0xFF00) >> 8;
    unsigned r1 = color1 & 0xFF, r2 = color2 & 0xFF;
    return _color(	_channel(r1, r2, channelr),
                    _channel(g1, g2, channelg),
                    _channel(b1, b2, channelb));
}

unsigned blend_channel32(unsigned color1, unsigned color2, unsigned a)
{
    int b1 = color1 >> 16, b2 = color2 >> 16;
    int g1 = (color1 & 0xFF00) >> 8, g2 = (color2 & 0xFF00) >> 8;
    int r1 = color1 & 0xFF, r2 = color2 & 0xFF;
    return _color(	_channel(r1, r2, a),
                    _channel(g1, g2, a),
                    _channel(b1, b2, a));
}

/////////////////////// blend 2 16bit colours //////////////////////
// color1 front colour,
// color2 bg colour
////////////////////////////////////////////////////////////////////

#define _b1 (color1>>11)
#define _g1 ((color1&0x7E0)>>5)
#define _r1 (color1&0x1F)
#define _b2 (color2>>11)
#define _g2 ((color2&0x7E0)>>5)
#define _r2 (color2&0x1F)
#define _bi ((_b1<<5)|_b2)
#define _gi (((_g1<<6)|_g2)+1024)
#define _ri ((_r1<<5)|_r2)
#define _multiply16(c1,c2,m) ((c1)*(c2)/(m))
#define _screen16(c1,c2,m) ((((c1)^(m))*((c2)^(m))/(m))^(m))
#define _hardlight16(c1,c2,m,m2) ((c1)<(m)?_multiply16(((c1)<<1),(c2),(m2)):_screen16((((c1)-(m))<<1),(c2),(m2)))
#define _overlay16(c1,c2,m,m2) ((c2)<(m)?_multiply16(((c2)<<1),(c1),(m2)):_screen16((((c2)-(m))<<1),(c1),(m2)))
#define _dodge16(c1,c2,m) ((c2)*(m)/((m)-(c1)))
#define _channel16(src,dest,alpha) (((src*alpha)+(dest*(255-alpha)))>>8)
#define _color16(r,g,b) ( ((b)<<11)|((g)<<5)|(r) )

unsigned char *create_multiply16_tbl()
{
    unsigned i, j;
    unsigned char *tbl = (unsigned char *)malloc(32 * 32 + 64 * 64);
    for(i = 0; i < 32; i++)
        for(j = 0; j < 32; j++)
        {
            tbl[(i << 5) | j] = (unsigned char)_multiply16(i, j, 0x1f);
        }
    for(i = 0; i < 64; i++)
        for(j = 0; j < 64; j++)
        {
            tbl[((i << 6) | j) + 1024] = (unsigned char)_multiply16(i, j, 0x3f);
        }
    return tbl;
}

//16bit blending, bit565
unsigned short blend_multiply16(unsigned short color1, unsigned short color2)
{
    unsigned char *tbl;
    if((tbl = blendtables[BLEND_MULTIPLY]))
    {
        return _color16(tbl[_ri], tbl[_gi], tbl[_bi]);
    }
    return _color16(_multiply16(_r1, _r2, 0x1F), _multiply16(_g1, _g2, 0x3F), _multiply16(_b1, _b2, 0x1F));
}

unsigned char *create_screen16_tbl()
{
    unsigned i, j;
    unsigned char *tbl = (unsigned char *)malloc(32 * 32 + 64 * 64);
    for(i = 0; i < 32; i++)
        for(j = 0; j < 32; j++)
        {
            tbl[(i << 5) | j] = (unsigned char)_screen16(i, j, 0x1f);
        }
    for(i = 0; i < 64; i++)
        for(j = 0; j < 64; j++)
        {
            tbl[((i << 6) | j) + 1024] = (unsigned char)_screen16(i, j, 0x3f);
        }
    return tbl;
}

unsigned short blend_screen16(unsigned short color1, unsigned short color2)
{
    unsigned char *tbl;
    if((tbl = blendtables[BLEND_SCREEN]))
    {
        return _color16(tbl[_ri], tbl[_gi], tbl[_bi]);
    }
    return _color16(_screen16(_r1, _r2, 0x1F), _screen16(_g1, _g2, 0x3F), _screen16(_b1, _b2, 0x1F));
}

unsigned char *create_overlay16_tbl()
{
    unsigned i, j;
    unsigned char *tbl = (unsigned char *)malloc(32 * 32 + 64 * 64);
    for(i = 0; i < 32; i++)
        for(j = 0; j < 32; j++)
        {
            tbl[(i << 5) | j] = (unsigned char)_overlay16(i, j, 0x10, 0x1f);
        }
    for(i = 0; i < 64; i++)
        for(j = 0; j < 64; j++)
        {
            tbl[((i << 6) | j) + 1024] = (unsigned char)_overlay16(i, j, 0x20, 0x3f);
        }
    return tbl;
}

unsigned short blend_overlay16(unsigned short color1, unsigned short color2)
{
    unsigned r1, g1, b1, r2, g2, b2;
    unsigned char *tbl;
    if((tbl = blendtables[BLEND_OVERLAY]))
    {
        return _color16(tbl[_ri], tbl[_gi], tbl[_bi]);
    }
    r1 = _r1, r2 = _r2;
    g1 = _g1, g2 = _g2;
    b1 = _b1, b2 = _b2;
    return _color16(_overlay16(r1, r2, 0x10, 0x1F), _overlay16(g1, g2, 0x20, 0x3F), _overlay16(b1, b2, 0x10, 0x1F));
}

unsigned char *create_hardlight16_tbl()
{
    unsigned i, j;
    unsigned char *tbl = (unsigned char *)malloc(32 * 32 + 64 * 64);
    for(i = 0; i < 32; i++)
        for(j = 0; j < 32; j++)
        {
            tbl[(i << 5) | j] = (unsigned char)_hardlight16(i, j, 0x10, 0x1f);
        }
    for(i = 0; i < 64; i++)
        for(j = 0; j < 64; j++)
        {
            tbl[((i << 6) | j) + 1024] = (unsigned char)_hardlight16(i, j, 0x20, 0x3f);
        }
    return tbl;
}

unsigned short blend_hardlight16(unsigned short color1, unsigned short color2)
{
    unsigned r1, g1, b1, r2, g2, b2;
    unsigned char *tbl;
    if((tbl = blendtables[BLEND_HARDLIGHT]))
    {
        return _color16(tbl[_ri], tbl[_gi], tbl[_bi]);
    }
    r1 = _r1, r2 = _r2;
    g1 = _g1, g2 = _g2;
    b1 = _b1, b2 = _b2;
    return _color16(_hardlight16(r1, r2, 0x10, 0x1F), _hardlight16(g1, g2, 0x20, 0x3F), _hardlight16(b1, b2, 0x10, 0x1F));
}

unsigned char *create_dodge16_tbl()
{
    unsigned i, j;
    unsigned char *tbl = (unsigned char *)malloc(32 * 32 + 64 * 64);
    unsigned t;
    for(i = 0; i < 32; i++)
        for(j = 0; j < 32; j++)
        {
            t = _dodge16(i, j, 0x20);
            if(t > 0x1f)
            {
                t = 0x1f;
            }
            tbl[(i << 5) | j] = t;
        }
    for(i = 0; i < 64; i++)
        for(j = 0; j < 64; j++)
        {
            t = _dodge16(i, j, 0x40);
            if(t > 0x3f)
            {
                t = 0x3f;
            }
            tbl[((i << 6) | j) + 1024] = t;
        }
    return tbl;
}

unsigned short blend_dodge16(unsigned short color1, unsigned short color2)
{
    int r, g, b;
    unsigned char *tbl;
    if((tbl = blendtables[BLEND_DODGE]))
    {
        return _color16(tbl[_ri], tbl[_gi], tbl[_bi]);
    }
    r = _dodge16(_r1, _r2, 0x20);
    g = _dodge16(_g1, _g2, 0x40);
    b = _dodge16(_b1, _b2, 0x20);
    if(r > 0x1F)
    {
        r = 0x1F;
    }
    if(g > 0x3F)
    {
        g = 0x3F;
    }
    if(b > 0x1F)
    {
        b = 0x1F;
    }
    return _color16(r, g, b);
}

unsigned char *create_half16_tbl()
{
    unsigned i, j;
    unsigned char *tbl = (unsigned char *)malloc(32 * 32 + 64 * 64);
    for(i = 0; i < 32; i++)
        for(j = 0; j < 32; j++)
        {
            tbl[(i << 5) | j] = (i + j) >> 1;
        }
    for(i = 0; i < 64; i++)
        for(j = 0; j < 64; j++)
        {
            tbl[((i << 6) | j) + 1024] = (i + j) >> 1;
        }
    return tbl;
}

unsigned short blend_half16(unsigned short color1, unsigned short color2)
{
    unsigned char *tbl;
    if((tbl = blendtables[BLEND_HALF]))
    {
        return _color16(tbl[_ri], tbl[_gi], tbl[_bi]);
    }
    return _color16((_r1 + _r2) >> 1, (_g1 + _g2) >> 1, (_b1 + _b2) >> 1);
}

unsigned short blend_tint16(unsigned short color1, unsigned short color2)
{
    unsigned short c = tint16fp1(tintcolor, color1);
    return tint16fp2 ? tint16fp2(c, color2) : c;
}

//copy from below
unsigned short blend_rgbchannel16(unsigned short color1, unsigned short color2)
{
    return _color16(_channel16(_r1, _r2, channelr), _channel16(_g1, _g2, channelg), _channel16(_b1, _b2, channelb));
}

unsigned short blend_channel16(unsigned short color1, unsigned short color2, unsigned a)
{
    return _color16(_channel16(_r1, _r2, a), _channel16(_g1, _g2, a), _channel16(_b1, _b2, a));
}

unsigned char *blendtables[MAX_BLENDINGS] = {NULL, NULL, NULL, NULL, NULL, NULL};

blend16fp blendfunctions16[MAX_BLENDINGS] = {blend_screen16, blend_multiply16, blend_overlay16, blend_hardlight16, blend_dodge16, blend_half16};
blend32fp blendfunctions[MAX_BLENDINGS] = {blend_screen, blend_multiply, blend_overlay, blend_hardlight, blend_dodge, blend_half};
blend32fp blendfunctions32[MAX_BLENDINGS] = {blend_screen32, blend_multiply32, blend_overlay32, blend_hardlight32, blend_dodge32, blend_half32};

// This method set blending tables for 8bit mode
// Need a list of blending table handles which
// are created by palette.c
void set_blendtables(unsigned char *tables[])
{
    int i;
    for(i = 0; i < MAX_BLENDINGS; i++)
    {
        blendtables[i] = tables[i];
    }
}

//getting too long so make 2 functions
blend16fp getblendfunction16(int alpha)
{
    blend16fp fp = (alpha > 0) ? blendfunctions16[alpha - 1] : NULL;
    //tint mode means tint the sprite with color
    if(tintmode > 0)
    {
        tint16fp1 = blendfunctions16[tintmode - 1];
        tint16fp2 = fp;
        fp = blend_tint16;
    }
    //trick, alpha 6 and rgb channel are of the same group
    else if(fp == blend_half16 && usechannel)
    {
        fp = blend_rgbchannel16;
    }

    return fp;
}

blend32fp getblendfunction32(int alpha)
{
    blend32fp fp = (alpha > 0) ? blendfunctions32[alpha - 1] : NULL;
    if(tintmode > 0)
    {
        tint32fp1 = blendfunctions32[tintmode - 1];
        tint32fp2 = fp;
        fp = blend_tint32;
    }
    //trick, alpha 6 and rgb channel are of the same group
    else if(fp == blend_half32 && usechannel)
    {
        fp = blend_rgbchannel32;
    }
    return fp;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      copy/reverse copy methods, extremely big function, but may improve the speed
//      assume the size is never greater than 480(screen size in some platforms)
//      TODO: test on other platforms besides windows
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
#define _revcpy case 960:pdest[959]=psrc[0];case 959:pdest[958]=psrc[1];case 958:pdest[957]=psrc[2];case 957:pdest[956]=psrc[3];case 956:pdest[955]=psrc[4];case 955:pdest[954]=psrc[5];case 954:pdest[953]=psrc[6];case 953:pdest[952]=psrc[7];case 952:pdest[951]=psrc[8];case 951:pdest[950]=psrc[9];case 950:pdest[949]=psrc[10];case 949:pdest[948]=psrc[11];case 948:pdest[947]=psrc[12];case 947:pdest[946]=psrc[13];case 946:pdest[945]=psrc[14];case 945:pdest[944]=psrc[15];case 944:pdest[943]=psrc[16];case 943:pdest[942]=psrc[17];case 942:pdest[941]=psrc[18];case 941:pdest[940]=psrc[19];case 940:pdest[939]=psrc[20];case 939:pdest[938]=psrc[21];case 938:pdest[937]=psrc[22];case 937:pdest[936]=psrc[23];case 936:pdest[935]=psrc[24];case 935:pdest[934]=psrc[25];case 934:pdest[933]=psrc[26];case 933:pdest[932]=psrc[27];case 932:pdest[931]=psrc[28];case 931:pdest[930]=psrc[29];case 930:pdest[929]=psrc[30];case 929:pdest[928]=psrc[31];case 928:pdest[927]=psrc[32];case 927:pdest[926]=psrc[33];case 926:pdest[925]=psrc[34];case 925:pdest[924]=psrc[35];case 924:pdest[923]=psrc[36];case 923:pdest[922]=psrc[37];case 922:pdest[921]=psrc[38];case 921:pdest[920]=psrc[39];case 920:pdest[919]=psrc[40];case 919:pdest[918]=psrc[41];case 918:pdest[917]=psrc[42];case 917:pdest[916]=psrc[43];case 916:pdest[915]=psrc[44];case 915:pdest[914]=psrc[45];case 914:pdest[913]=psrc[46];case 913:pdest[912]=psrc[47];case 912:pdest[911]=psrc[48];case 911:pdest[910]=psrc[49];case 910:pdest[909]=psrc[50];case 909:pdest[908]=psrc[51];case 908:pdest[907]=psrc[52];case 907:pdest[906]=psrc[53];case 906:pdest[905]=psrc[54];case 905:pdest[904]=psrc[55];case 904:pdest[903]=psrc[56];case 903:pdest[902]=psrc[57];case 902:pdest[901]=psrc[58];case 901:pdest[900]=psrc[59];case 900:pdest[899]=psrc[60];case 899:pdest[898]=psrc[61];case 898:pdest[897]=psrc[62];case 897:pdest[896]=psrc[63];case 896:pdest[895]=psrc[64];case 895:pdest[894]=psrc[65];case 894:pdest[893]=psrc[66];case 893:pdest[892]=psrc[67];case 892:pdest[891]=psrc[68];case 891:pdest[890]=psrc[69];case 890:pdest[889]=psrc[70];case 889:pdest[888]=psrc[71];case 888:pdest[887]=psrc[72];case 887:pdest[886]=psrc[73];case 886:pdest[885]=psrc[74];case 885:pdest[884]=psrc[75];case 884:pdest[883]=psrc[76];case 883:pdest[882]=psrc[77];case 882:pdest[881]=psrc[78];case 881:pdest[880]=psrc[79];case 880:pdest[879]=psrc[80];case 879:pdest[878]=psrc[81];case 878:pdest[877]=psrc[82];case 877:pdest[876]=psrc[83];case 876:pdest[875]=psrc[84];case 875:pdest[874]=psrc[85];case 874:pdest[873]=psrc[86];case 873:pdest[872]=psrc[87];case 872:pdest[871]=psrc[88];case 871:pdest[870]=psrc[89];case 870:pdest[869]=psrc[90];case 869:pdest[868]=psrc[91];case 868:pdest[867]=psrc[92];case 867:pdest[866]=psrc[93];case 866:pdest[865]=psrc[94];case 865:pdest[864]=psrc[95];case 864:pdest[863]=psrc[96];case 863:pdest[862]=psrc[97];case 862:pdest[861]=psrc[98];case 861:pdest[860]=psrc[99];case 860:pdest[859]=psrc[100];case 859:pdest[858]=psrc[101];case 858:pdest[857]=psrc[102];case 857:pdest[856]=psrc[103];case 856:pdest[855]=psrc[104];case 855:pdest[854]=psrc[105];case 854:pdest[853]=psrc[106];case 853:pdest[852]=psrc[107];case 852:pdest[851]=psrc[108];case 851:pdest[850]=psrc[109];case 850:pdest[849]=psrc[110];case 849:pdest[848]=psrc[111];case 848:pdest[847]=psrc[112];case 847:pdest[846]=psrc[113];case 846:pdest[845]=psrc[114];case 845:pdest[844]=psrc[115];case 844:pdest[843]=psrc[116];case 843:pdest[842]=psrc[117];case 842:pdest[841]=psrc[118];case 841:pdest[840]=psrc[119];case 840:pdest[839]=psrc[120];case 839:pdest[838]=psrc[121];case 838:pdest[837]=psrc[122];case 837:pdest[836]=psrc[123];case 836:pdest[835]=psrc[124];case 835:pdest[834]=psrc[125];case 834:pdest[833]=psrc[126];case 833:pdest[832]=psrc[127];case 832:pdest[831]=psrc[128];case 831:pdest[830]=psrc[129];case 830:pdest[829]=psrc[130];case 829:pdest[828]=psrc[131];case 828:pdest[827]=psrc[132];case 827:pdest[826]=psrc[133];case 826:pdest[825]=psrc[134];case 825:pdest[824]=psrc[135];case 824:pdest[823]=psrc[136];case 823:pdest[822]=psrc[137];case 822:pdest[821]=psrc[138];case 821:pdest[820]=psrc[139];case 820:pdest[819]=psrc[140];case 819:pdest[818]=psrc[141];case 818:pdest[817]=psrc[142];case 817:pdest[816]=psrc[143];case 816:pdest[815]=psrc[144];case 815:pdest[814]=psrc[145];case 814:pdest[813]=psrc[146];case 813:pdest[812]=psrc[147];case 812:pdest[811]=psrc[148];case 811:pdest[810]=psrc[149];case 810:pdest[809]=psrc[150];case 809:pdest[808]=psrc[151];case 808:pdest[807]=psrc[152];case 807:pdest[806]=psrc[153];case 806:pdest[805]=psrc[154];case 805:pdest[804]=psrc[155];case 804:pdest[803]=psrc[156];case 803:pdest[802]=psrc[157];case 802:pdest[801]=psrc[158];case 801:pdest[800]=psrc[159];case 800:pdest[799]=psrc[160];case 799:pdest[798]=psrc[161];case 798:pdest[797]=psrc[162];case 797:pdest[796]=psrc[163];case 796:pdest[795]=psrc[164];case 795:pdest[794]=psrc[165];case 794:pdest[793]=psrc[166];case 793:pdest[792]=psrc[167];case 792:pdest[791]=psrc[168];case 791:pdest[790]=psrc[169];case 790:pdest[789]=psrc[170];case 789:pdest[788]=psrc[171];case 788:pdest[787]=psrc[172];case 787:pdest[786]=psrc[173];case 786:pdest[785]=psrc[174];case 785:pdest[784]=psrc[175];case 784:pdest[783]=psrc[176];case 783:pdest[782]=psrc[177];case 782:pdest[781]=psrc[178];case 781:pdest[780]=psrc[179];case 780:pdest[779]=psrc[180];case 779:pdest[778]=psrc[181];case 778:pdest[777]=psrc[182];case 777:pdest[776]=psrc[183];case 776:pdest[775]=psrc[184];case 775:pdest[774]=psrc[185];case 774:pdest[773]=psrc[186];case 773:pdest[772]=psrc[187];case 772:pdest[771]=psrc[188];case 771:pdest[770]=psrc[189];case 770:pdest[769]=psrc[190];case 769:pdest[768]=psrc[191];case 768:pdest[767]=psrc[192];case 767:pdest[766]=psrc[193];case 766:pdest[765]=psrc[194];case 765:pdest[764]=psrc[195];case 764:pdest[763]=psrc[196];case 763:pdest[762]=psrc[197];case 762:pdest[761]=psrc[198];case 761:pdest[760]=psrc[199];case 760:pdest[759]=psrc[200];case 759:pdest[758]=psrc[201];case 758:pdest[757]=psrc[202];case 757:pdest[756]=psrc[203];case 756:pdest[755]=psrc[204];case 755:pdest[754]=psrc[205];case 754:pdest[753]=psrc[206];case 753:pdest[752]=psrc[207];case 752:pdest[751]=psrc[208];case 751:pdest[750]=psrc[209];case 750:pdest[749]=psrc[210];case 749:pdest[748]=psrc[211];case 748:pdest[747]=psrc[212];case 747:pdest[746]=psrc[213];case 746:pdest[745]=psrc[214];case 745:pdest[744]=psrc[215];case 744:pdest[743]=psrc[216];case 743:pdest[742]=psrc[217];case 742:pdest[741]=psrc[218];case 741:pdest[740]=psrc[219];case 740:pdest[739]=psrc[220];case 739:pdest[738]=psrc[221];case 738:pdest[737]=psrc[222];case 737:pdest[736]=psrc[223];case 736:pdest[735]=psrc[224];case 735:pdest[734]=psrc[225];case 734:pdest[733]=psrc[226];case 733:pdest[732]=psrc[227];case 732:pdest[731]=psrc[228];case 731:pdest[730]=psrc[229];case 730:pdest[729]=psrc[230];case 729:pdest[728]=psrc[231];case 728:pdest[727]=psrc[232];case 727:pdest[726]=psrc[233];case 726:pdest[725]=psrc[234];case 725:pdest[724]=psrc[235];case 724:pdest[723]=psrc[236];case 723:pdest[722]=psrc[237];case 722:pdest[721]=psrc[238];case 721:pdest[720]=psrc[239];case 720:pdest[719]=psrc[240];case 719:pdest[718]=psrc[241];case 718:pdest[717]=psrc[242];case 717:pdest[716]=psrc[243];case 716:pdest[715]=psrc[244];case 715:pdest[714]=psrc[245];case 714:pdest[713]=psrc[246];case 713:pdest[712]=psrc[247];case 712:pdest[711]=psrc[248];case 711:pdest[710]=psrc[249];case 710:pdest[709]=psrc[250];case 709:pdest[708]=psrc[251];case 708:pdest[707]=psrc[252];case 707:pdest[706]=psrc[253];case 706:pdest[705]=psrc[254];case 705:pdest[704]=psrc[255];case 704:pdest[703]=psrc[256];case 703:pdest[702]=psrc[257];case 702:pdest[701]=psrc[258];case 701:pdest[700]=psrc[259];case 700:pdest[699]=psrc[260];case 699:pdest[698]=psrc[261];case 698:pdest[697]=psrc[262];case 697:pdest[696]=psrc[263];case 696:pdest[695]=psrc[264];case 695:pdest[694]=psrc[265];case 694:pdest[693]=psrc[266];case 693:pdest[692]=psrc[267];case 692:pdest[691]=psrc[268];case 691:pdest[690]=psrc[269];case 690:pdest[689]=psrc[270];case 689:pdest[688]=psrc[271];case 688:pdest[687]=psrc[272];case 687:pdest[686]=psrc[273];case 686:pdest[685]=psrc[274];case 685:pdest[684]=psrc[275];case 684:pdest[683]=psrc[276];case 683:pdest[682]=psrc[277];case 682:pdest[681]=psrc[278];case 681:pdest[680]=psrc[279];case 680:pdest[679]=psrc[280];case 679:pdest[678]=psrc[281];case 678:pdest[677]=psrc[282];case 677:pdest[676]=psrc[283];case 676:pdest[675]=psrc[284];case 675:pdest[674]=psrc[285];case 674:pdest[673]=psrc[286];case 673:pdest[672]=psrc[287];case 672:pdest[671]=psrc[288];case 671:pdest[670]=psrc[289];case 670:pdest[669]=psrc[290];case 669:pdest[668]=psrc[291];case 668:pdest[667]=psrc[292];case 667:pdest[666]=psrc[293];case 666:pdest[665]=psrc[294];case 665:pdest[664]=psrc[295];case 664:pdest[663]=psrc[296];case 663:pdest[662]=psrc[297];case 662:pdest[661]=psrc[298];case 661:pdest[660]=psrc[299];case 660:pdest[659]=psrc[300];case 659:pdest[658]=psrc[301];case 658:pdest[657]=psrc[302];case 657:pdest[656]=psrc[303];case 656:pdest[655]=psrc[304];case 655:pdest[654]=psrc[305];case 654:pdest[653]=psrc[306];case 653:pdest[652]=psrc[307];case 652:pdest[651]=psrc[308];case 651:pdest[650]=psrc[309];case 650:pdest[649]=psrc[310];case 649:pdest[648]=psrc[311];case 648:pdest[647]=psrc[312];case 647:pdest[646]=psrc[313];case 646:pdest[645]=psrc[314];case 645:pdest[644]=psrc[315];case 644:pdest[643]=psrc[316];case 643:pdest[642]=psrc[317];case 642:pdest[641]=psrc[318];case 641:pdest[640]=psrc[319];case 640:pdest[639]=psrc[320];case 639:pdest[638]=psrc[321];case 638:pdest[637]=psrc[322];case 637:pdest[636]=psrc[323];case 636:pdest[635]=psrc[324];case 635:pdest[634]=psrc[325];case 634:pdest[633]=psrc[326];case 633:pdest[632]=psrc[327];case 632:pdest[631]=psrc[328];case 631:pdest[630]=psrc[329];case 630:pdest[629]=psrc[330];case 629:pdest[628]=psrc[331];case 628:pdest[627]=psrc[332];case 627:pdest[626]=psrc[333];case 626:pdest[625]=psrc[334];case 625:pdest[624]=psrc[335];case 624:pdest[623]=psrc[336];case 623:pdest[622]=psrc[337];case 622:pdest[621]=psrc[338];case 621:pdest[620]=psrc[339];case 620:pdest[619]=psrc[340];case 619:pdest[618]=psrc[341];case 618:pdest[617]=psrc[342];case 617:pdest[616]=psrc[343];case 616:pdest[615]=psrc[344];case 615:pdest[614]=psrc[345];case 614:pdest[613]=psrc[346];case 613:pdest[612]=psrc[347];case 612:pdest[611]=psrc[348];case 611:pdest[610]=psrc[349];case 610:pdest[609]=psrc[350];case 609:pdest[608]=psrc[351];case 608:pdest[607]=psrc[352];case 607:pdest[606]=psrc[353];case 606:pdest[605]=psrc[354];case 605:pdest[604]=psrc[355];case 604:pdest[603]=psrc[356];case 603:pdest[602]=psrc[357];case 602:pdest[601]=psrc[358];case 601:pdest[600]=psrc[359];case 600:pdest[599]=psrc[360];case 599:pdest[598]=psrc[361];case 598:pdest[597]=psrc[362];case 597:pdest[596]=psrc[363];case 596:pdest[595]=psrc[364];case 595:pdest[594]=psrc[365];case 594:pdest[593]=psrc[366];case 593:pdest[592]=psrc[367];case 592:pdest[591]=psrc[368];case 591:pdest[590]=psrc[369];case 590:pdest[589]=psrc[370];case 589:pdest[588]=psrc[371];case 588:pdest[587]=psrc[372];case 587:pdest[586]=psrc[373];case 586:pdest[585]=psrc[374];case 585:pdest[584]=psrc[375];case 584:pdest[583]=psrc[376];case 583:pdest[582]=psrc[377];case 582:pdest[581]=psrc[378];case 581:pdest[580]=psrc[379];case 580:pdest[579]=psrc[380];case 579:pdest[578]=psrc[381];case 578:pdest[577]=psrc[382];case 577:pdest[576]=psrc[383];case 576:pdest[575]=psrc[384];case 575:pdest[574]=psrc[385];case 574:pdest[573]=psrc[386];case 573:pdest[572]=psrc[387];case 572:pdest[571]=psrc[388];case 571:pdest[570]=psrc[389];case 570:pdest[569]=psrc[390];case 569:pdest[568]=psrc[391];case 568:pdest[567]=psrc[392];case 567:pdest[566]=psrc[393];case 566:pdest[565]=psrc[394];case 565:pdest[564]=psrc[395];case 564:pdest[563]=psrc[396];case 563:pdest[562]=psrc[397];case 562:pdest[561]=psrc[398];case 561:pdest[560]=psrc[399];case 560:pdest[559]=psrc[400];case 559:pdest[558]=psrc[401];case 558:pdest[557]=psrc[402];case 557:pdest[556]=psrc[403];case 556:pdest[555]=psrc[404];case 555:pdest[554]=psrc[405];case 554:pdest[553]=psrc[406];case 553:pdest[552]=psrc[407];case 552:pdest[551]=psrc[408];case 551:pdest[550]=psrc[409];case 550:pdest[549]=psrc[410];case 549:pdest[548]=psrc[411];case 548:pdest[547]=psrc[412];case 547:pdest[546]=psrc[413];case 546:pdest[545]=psrc[414];case 545:pdest[544]=psrc[415];case 544:pdest[543]=psrc[416];case 543:pdest[542]=psrc[417];case 542:pdest[541]=psrc[418];case 541:pdest[540]=psrc[419];case 540:pdest[539]=psrc[420];case 539:pdest[538]=psrc[421];case 538:pdest[537]=psrc[422];case 537:pdest[536]=psrc[423];case 536:pdest[535]=psrc[424];case 535:pdest[534]=psrc[425];case 534:pdest[533]=psrc[426];case 533:pdest[532]=psrc[427];case 532:pdest[531]=psrc[428];case 531:pdest[530]=psrc[429];case 530:pdest[529]=psrc[430];case 529:pdest[528]=psrc[431];case 528:pdest[527]=psrc[432];case 527:pdest[526]=psrc[433];case 526:pdest[525]=psrc[434];case 525:pdest[524]=psrc[435];case 524:pdest[523]=psrc[436];case 523:pdest[522]=psrc[437];case 522:pdest[521]=psrc[438];case 521:pdest[520]=psrc[439];case 520:pdest[519]=psrc[440];case 519:pdest[518]=psrc[441];case 518:pdest[517]=psrc[442];case 517:pdest[516]=psrc[443];case 516:pdest[515]=psrc[444];case 515:pdest[514]=psrc[445];case 514:pdest[513]=psrc[446];case 513:pdest[512]=psrc[447];case 512:pdest[511]=psrc[448];case 511:pdest[510]=psrc[449];case 510:pdest[509]=psrc[450];case 509:pdest[508]=psrc[451];case 508:pdest[507]=psrc[452];case 507:pdest[506]=psrc[453];case 506:pdest[505]=psrc[454];case 505:pdest[504]=psrc[455];case 504:pdest[503]=psrc[456];case 503:pdest[502]=psrc[457];case 502:pdest[501]=psrc[458];case 501:pdest[500]=psrc[459];case 500:pdest[499]=psrc[460];case 499:pdest[498]=psrc[461];case 498:pdest[497]=psrc[462];case 497:pdest[496]=psrc[463];case 496:pdest[495]=psrc[464];case 495:pdest[494]=psrc[465];case 494:pdest[493]=psrc[466];case 493:pdest[492]=psrc[467];case 492:pdest[491]=psrc[468];case 491:pdest[490]=psrc[469];case 490:pdest[489]=psrc[470];case 489:pdest[488]=psrc[471];case 488:pdest[487]=psrc[472];case 487:pdest[486]=psrc[473];case 486:pdest[485]=psrc[474];case 485:pdest[484]=psrc[475];case 484:pdest[483]=psrc[476];case 483:pdest[482]=psrc[477];case 482:pdest[481]=psrc[478];case 481:pdest[480]=psrc[479];case 480:pdest[479]=psrc[480];case 479:pdest[478]=psrc[481];case 478:pdest[477]=psrc[482];case 477:pdest[476]=psrc[483];case 476:pdest[475]=psrc[484];case 475:pdest[474]=psrc[485];case 474:pdest[473]=psrc[486];case 473:pdest[472]=psrc[487];case 472:pdest[471]=psrc[488];case 471:pdest[470]=psrc[489];case 470:pdest[469]=psrc[490];case 469:pdest[468]=psrc[491];case 468:pdest[467]=psrc[492];case 467:pdest[466]=psrc[493];case 466:pdest[465]=psrc[494];case 465:pdest[464]=psrc[495];case 464:pdest[463]=psrc[496];case 463:pdest[462]=psrc[497];case 462:pdest[461]=psrc[498];case 461:pdest[460]=psrc[499];case 460:pdest[459]=psrc[500];case 459:pdest[458]=psrc[501];case 458:pdest[457]=psrc[502];case 457:pdest[456]=psrc[503];case 456:pdest[455]=psrc[504];case 455:pdest[454]=psrc[505];case 454:pdest[453]=psrc[506];case 453:pdest[452]=psrc[507];case 452:pdest[451]=psrc[508];case 451:pdest[450]=psrc[509];case 450:pdest[449]=psrc[510];case 449:pdest[448]=psrc[511];case 448:pdest[447]=psrc[512];case 447:pdest[446]=psrc[513];case 446:pdest[445]=psrc[514];case 445:pdest[444]=psrc[515];case 444:pdest[443]=psrc[516];case 443:pdest[442]=psrc[517];case 442:pdest[441]=psrc[518];case 441:pdest[440]=psrc[519];case 440:pdest[439]=psrc[520];case 439:pdest[438]=psrc[521];case 438:pdest[437]=psrc[522];case 437:pdest[436]=psrc[523];case 436:pdest[435]=psrc[524];case 435:pdest[434]=psrc[525];case 434:pdest[433]=psrc[526];case 433:pdest[432]=psrc[527];case 432:pdest[431]=psrc[528];case 431:pdest[430]=psrc[529];case 430:pdest[429]=psrc[530];case 429:pdest[428]=psrc[531];case 428:pdest[427]=psrc[532];case 427:pdest[426]=psrc[533];case 426:pdest[425]=psrc[534];case 425:pdest[424]=psrc[535];case 424:pdest[423]=psrc[536];case 423:pdest[422]=psrc[537];case 422:pdest[421]=psrc[538];case 421:pdest[420]=psrc[539];case 420:pdest[419]=psrc[540];case 419:pdest[418]=psrc[541];case 418:pdest[417]=psrc[542];case 417:pdest[416]=psrc[543];case 416:pdest[415]=psrc[544];case 415:pdest[414]=psrc[545];case 414:pdest[413]=psrc[546];case 413:pdest[412]=psrc[547];case 412:pdest[411]=psrc[548];case 411:pdest[410]=psrc[549];case 410:pdest[409]=psrc[550];case 409:pdest[408]=psrc[551];case 408:pdest[407]=psrc[552];case 407:pdest[406]=psrc[553];case 406:pdest[405]=psrc[554];case 405:pdest[404]=psrc[555];case 404:pdest[403]=psrc[556];case 403:pdest[402]=psrc[557];case 402:pdest[401]=psrc[558];case 401:pdest[400]=psrc[559];case 400:pdest[399]=psrc[560];case 399:pdest[398]=psrc[561];case 398:pdest[397]=psrc[562];case 397:pdest[396]=psrc[563];case 396:pdest[395]=psrc[564];case 395:pdest[394]=psrc[565];case 394:pdest[393]=psrc[566];case 393:pdest[392]=psrc[567];case 392:pdest[391]=psrc[568];case 391:pdest[390]=psrc[569];case 390:pdest[389]=psrc[570];case 389:pdest[388]=psrc[571];case 388:pdest[387]=psrc[572];case 387:pdest[386]=psrc[573];case 386:pdest[385]=psrc[574];case 385:pdest[384]=psrc[575];case 384:pdest[383]=psrc[576];case 383:pdest[382]=psrc[577];case 382:pdest[381]=psrc[578];case 381:pdest[380]=psrc[579];case 380:pdest[379]=psrc[580];case 379:pdest[378]=psrc[581];case 378:pdest[377]=psrc[582];case 377:pdest[376]=psrc[583];case 376:pdest[375]=psrc[584];case 375:pdest[374]=psrc[585];case 374:pdest[373]=psrc[586];case 373:pdest[372]=psrc[587];case 372:pdest[371]=psrc[588];case 371:pdest[370]=psrc[589];case 370:pdest[369]=psrc[590];case 369:pdest[368]=psrc[591];case 368:pdest[367]=psrc[592];case 367:pdest[366]=psrc[593];case 366:pdest[365]=psrc[594];case 365:pdest[364]=psrc[595];case 364:pdest[363]=psrc[596];case 363:pdest[362]=psrc[597];case 362:pdest[361]=psrc[598];case 361:pdest[360]=psrc[599];case 360:pdest[359]=psrc[600];case 359:pdest[358]=psrc[601];case 358:pdest[357]=psrc[602];case 357:pdest[356]=psrc[603];case 356:pdest[355]=psrc[604];case 355:pdest[354]=psrc[605];case 354:pdest[353]=psrc[606];case 353:pdest[352]=psrc[607];case 352:pdest[351]=psrc[608];case 351:pdest[350]=psrc[609];case 350:pdest[349]=psrc[610];case 349:pdest[348]=psrc[611];case 348:pdest[347]=psrc[612];case 347:pdest[346]=psrc[613];case 346:pdest[345]=psrc[614];case 345:pdest[344]=psrc[615];case 344:pdest[343]=psrc[616];case 343:pdest[342]=psrc[617];case 342:pdest[341]=psrc[618];case 341:pdest[340]=psrc[619];case 340:pdest[339]=psrc[620];case 339:pdest[338]=psrc[621];case 338:pdest[337]=psrc[622];case 337:pdest[336]=psrc[623];case 336:pdest[335]=psrc[624];case 335:pdest[334]=psrc[625];case 334:pdest[333]=psrc[626];case 333:pdest[332]=psrc[627];case 332:pdest[331]=psrc[628];case 331:pdest[330]=psrc[629];case 330:pdest[329]=psrc[630];case 329:pdest[328]=psrc[631];case 328:pdest[327]=psrc[632];case 327:pdest[326]=psrc[633];case 326:pdest[325]=psrc[634];case 325:pdest[324]=psrc[635];case 324:pdest[323]=psrc[636];case 323:pdest[322]=psrc[637];case 322:pdest[321]=psrc[638];case 321:pdest[320]=psrc[639];case 320:pdest[319]=psrc[640];case 319:pdest[318]=psrc[641];case 318:pdest[317]=psrc[642];case 317:pdest[316]=psrc[643];case 316:pdest[315]=psrc[644];case 315:pdest[314]=psrc[645];case 314:pdest[313]=psrc[646];case 313:pdest[312]=psrc[647];case 312:pdest[311]=psrc[648];case 311:pdest[310]=psrc[649];case 310:pdest[309]=psrc[650];case 309:pdest[308]=psrc[651];case 308:pdest[307]=psrc[652];case 307:pdest[306]=psrc[653];case 306:pdest[305]=psrc[654];case 305:pdest[304]=psrc[655];case 304:pdest[303]=psrc[656];case 303:pdest[302]=psrc[657];case 302:pdest[301]=psrc[658];case 301:pdest[300]=psrc[659];case 300:pdest[299]=psrc[660];case 299:pdest[298]=psrc[661];case 298:pdest[297]=psrc[662];case 297:pdest[296]=psrc[663];case 296:pdest[295]=psrc[664];case 295:pdest[294]=psrc[665];case 294:pdest[293]=psrc[666];case 293:pdest[292]=psrc[667];case 292:pdest[291]=psrc[668];case 291:pdest[290]=psrc[669];case 290:pdest[289]=psrc[670];case 289:pdest[288]=psrc[671];case 288:pdest[287]=psrc[672];case 287:pdest[286]=psrc[673];case 286:pdest[285]=psrc[674];case 285:pdest[284]=psrc[675];case 284:pdest[283]=psrc[676];case 283:pdest[282]=psrc[677];case 282:pdest[281]=psrc[678];case 281:pdest[280]=psrc[679];case 280:pdest[279]=psrc[680];case 279:pdest[278]=psrc[681];case 278:pdest[277]=psrc[682];case 277:pdest[276]=psrc[683];case 276:pdest[275]=psrc[684];case 275:pdest[274]=psrc[685];case 274:pdest[273]=psrc[686];case 273:pdest[272]=psrc[687];case 272:pdest[271]=psrc[688];case 271:pdest[270]=psrc[689];case 270:pdest[269]=psrc[690];case 269:pdest[268]=psrc[691];case 268:pdest[267]=psrc[692];case 267:pdest[266]=psrc[693];case 266:pdest[265]=psrc[694];case 265:pdest[264]=psrc[695];case 264:pdest[263]=psrc[696];case 263:pdest[262]=psrc[697];case 262:pdest[261]=psrc[698];case 261:pdest[260]=psrc[699];case 260:pdest[259]=psrc[700];case 259:pdest[258]=psrc[701];case 258:pdest[257]=psrc[702];case 257:pdest[256]=psrc[703];case 256:pdest[255]=psrc[704];case 255:pdest[254]=psrc[705];case 254:pdest[253]=psrc[706];case 253:pdest[252]=psrc[707];case 252:pdest[251]=psrc[708];case 251:pdest[250]=psrc[709];case 250:pdest[249]=psrc[710];case 249:pdest[248]=psrc[711];case 248:pdest[247]=psrc[712];case 247:pdest[246]=psrc[713];case 246:pdest[245]=psrc[714];case 245:pdest[244]=psrc[715];case 244:pdest[243]=psrc[716];case 243:pdest[242]=psrc[717];case 242:pdest[241]=psrc[718];case 241:pdest[240]=psrc[719];case 240:pdest[239]=psrc[720];case 239:pdest[238]=psrc[721];case 238:pdest[237]=psrc[722];case 237:pdest[236]=psrc[723];case 236:pdest[235]=psrc[724];case 235:pdest[234]=psrc[725];case 234:pdest[233]=psrc[726];case 233:pdest[232]=psrc[727];case 232:pdest[231]=psrc[728];case 231:pdest[230]=psrc[729];case 230:pdest[229]=psrc[730];case 229:pdest[228]=psrc[731];case 228:pdest[227]=psrc[732];case 227:pdest[226]=psrc[733];case 226:pdest[225]=psrc[734];case 225:pdest[224]=psrc[735];case 224:pdest[223]=psrc[736];case 223:pdest[222]=psrc[737];case 222:pdest[221]=psrc[738];case 221:pdest[220]=psrc[739];case 220:pdest[219]=psrc[740];case 219:pdest[218]=psrc[741];case 218:pdest[217]=psrc[742];case 217:pdest[216]=psrc[743];case 216:pdest[215]=psrc[744];case 215:pdest[214]=psrc[745];case 214:pdest[213]=psrc[746];case 213:pdest[212]=psrc[747];case 212:pdest[211]=psrc[748];case 211:pdest[210]=psrc[749];case 210:pdest[209]=psrc[750];case 209:pdest[208]=psrc[751];case 208:pdest[207]=psrc[752];case 207:pdest[206]=psrc[753];case 206:pdest[205]=psrc[754];case 205:pdest[204]=psrc[755];case 204:pdest[203]=psrc[756];case 203:pdest[202]=psrc[757];case 202:pdest[201]=psrc[758];case 201:pdest[200]=psrc[759];case 200:pdest[199]=psrc[760];case 199:pdest[198]=psrc[761];case 198:pdest[197]=psrc[762];case 197:pdest[196]=psrc[763];case 196:pdest[195]=psrc[764];case 195:pdest[194]=psrc[765];case 194:pdest[193]=psrc[766];case 193:pdest[192]=psrc[767];case 192:pdest[191]=psrc[768];case 191:pdest[190]=psrc[769];case 190:pdest[189]=psrc[770];case 189:pdest[188]=psrc[771];case 188:pdest[187]=psrc[772];case 187:pdest[186]=psrc[773];case 186:pdest[185]=psrc[774];case 185:pdest[184]=psrc[775];case 184:pdest[183]=psrc[776];case 183:pdest[182]=psrc[777];case 182:pdest[181]=psrc[778];case 181:pdest[180]=psrc[779];case 180:pdest[179]=psrc[780];case 179:pdest[178]=psrc[781];case 178:pdest[177]=psrc[782];case 177:pdest[176]=psrc[783];case 176:pdest[175]=psrc[784];case 175:pdest[174]=psrc[785];case 174:pdest[173]=psrc[786];case 173:pdest[172]=psrc[787];case 172:pdest[171]=psrc[788];case 171:pdest[170]=psrc[789];case 170:pdest[169]=psrc[790];case 169:pdest[168]=psrc[791];case 168:pdest[167]=psrc[792];case 167:pdest[166]=psrc[793];case 166:pdest[165]=psrc[794];case 165:pdest[164]=psrc[795];case 164:pdest[163]=psrc[796];case 163:pdest[162]=psrc[797];case 162:pdest[161]=psrc[798];case 161:pdest[160]=psrc[799];case 160:pdest[159]=psrc[800];case 159:pdest[158]=psrc[801];case 158:pdest[157]=psrc[802];case 157:pdest[156]=psrc[803];case 156:pdest[155]=psrc[804];case 155:pdest[154]=psrc[805];case 154:pdest[153]=psrc[806];case 153:pdest[152]=psrc[807];case 152:pdest[151]=psrc[808];case 151:pdest[150]=psrc[809];case 150:pdest[149]=psrc[810];case 149:pdest[148]=psrc[811];case 148:pdest[147]=psrc[812];case 147:pdest[146]=psrc[813];case 146:pdest[145]=psrc[814];case 145:pdest[144]=psrc[815];case 144:pdest[143]=psrc[816];case 143:pdest[142]=psrc[817];case 142:pdest[141]=psrc[818];case 141:pdest[140]=psrc[819];case 140:pdest[139]=psrc[820];case 139:pdest[138]=psrc[821];case 138:pdest[137]=psrc[822];case 137:pdest[136]=psrc[823];case 136:pdest[135]=psrc[824];case 135:pdest[134]=psrc[825];case 134:pdest[133]=psrc[826];case 133:pdest[132]=psrc[827];case 132:pdest[131]=psrc[828];case 131:pdest[130]=psrc[829];case 130:pdest[129]=psrc[830];case 129:pdest[128]=psrc[831];case 128:pdest[127]=psrc[832];case 127:pdest[126]=psrc[833];case 126:pdest[125]=psrc[834];case 125:pdest[124]=psrc[835];case 124:pdest[123]=psrc[836];case 123:pdest[122]=psrc[837];case 122:pdest[121]=psrc[838];case 121:pdest[120]=psrc[839];case 120:pdest[119]=psrc[840];case 119:pdest[118]=psrc[841];case 118:pdest[117]=psrc[842];case 117:pdest[116]=psrc[843];case 116:pdest[115]=psrc[844];case 115:pdest[114]=psrc[845];case 114:pdest[113]=psrc[846];case 113:pdest[112]=psrc[847];case 112:pdest[111]=psrc[848];case 111:pdest[110]=psrc[849];case 110:pdest[109]=psrc[850];case 109:pdest[108]=psrc[851];case 108:pdest[107]=psrc[852];case 107:pdest[106]=psrc[853];case 106:pdest[105]=psrc[854];case 105:pdest[104]=psrc[855];case 104:pdest[103]=psrc[856];case 103:pdest[102]=psrc[857];case 102:pdest[101]=psrc[858];case 101:pdest[100]=psrc[859];case 100:pdest[99]=psrc[860];case 99:pdest[98]=psrc[861];case 98:pdest[97]=psrc[862];case 97:pdest[96]=psrc[863];case 96:pdest[95]=psrc[864];case 95:pdest[94]=psrc[865];case 94:pdest[93]=psrc[866];case 93:pdest[92]=psrc[867];case 92:pdest[91]=psrc[868];case 91:pdest[90]=psrc[869];case 90:pdest[89]=psrc[870];case 89:pdest[88]=psrc[871];case 88:pdest[87]=psrc[872];case 87:pdest[86]=psrc[873];case 86:pdest[85]=psrc[874];case 85:pdest[84]=psrc[875];case 84:pdest[83]=psrc[876];case 83:pdest[82]=psrc[877];case 82:pdest[81]=psrc[878];case 81:pdest[80]=psrc[879];case 80:pdest[79]=psrc[880];case 79:pdest[78]=psrc[881];case 78:pdest[77]=psrc[882];case 77:pdest[76]=psrc[883];case 76:pdest[75]=psrc[884];case 75:pdest[74]=psrc[885];case 74:pdest[73]=psrc[886];case 73:pdest[72]=psrc[887];case 72:pdest[71]=psrc[888];case 71:pdest[70]=psrc[889];case 70:pdest[69]=psrc[890];case 69:pdest[68]=psrc[891];case 68:pdest[67]=psrc[892];case 67:pdest[66]=psrc[893];case 66:pdest[65]=psrc[894];case 65:pdest[64]=psrc[895];case 64:pdest[63]=psrc[896];case 63:pdest[62]=psrc[897];case 62:pdest[61]=psrc[898];case 61:pdest[60]=psrc[899];case 60:pdest[59]=psrc[900];case 59:pdest[58]=psrc[901];case 58:pdest[57]=psrc[902];case 57:pdest[56]=psrc[903];case 56:pdest[55]=psrc[904];case 55:pdest[54]=psrc[905];case 54:pdest[53]=psrc[906];case 53:pdest[52]=psrc[907];case 52:pdest[51]=psrc[908];case 51:pdest[50]=psrc[909];case 50:pdest[49]=psrc[910];case 49:pdest[48]=psrc[911];case 48:pdest[47]=psrc[912];case 47:pdest[46]=psrc[913];case 46:pdest[45]=psrc[914];case 45:pdest[44]=psrc[915];case 44:pdest[43]=psrc[916];case 43:pdest[42]=psrc[917];case 42:pdest[41]=psrc[918];case 41:pdest[40]=psrc[919];case 40:pdest[39]=psrc[920];case 39:pdest[38]=psrc[921];case 38:pdest[37]=psrc[922];case 37:pdest[36]=psrc[923];case 36:pdest[35]=psrc[924];case 35:pdest[34]=psrc[925];case 34:pdest[33]=psrc[926];case 33:pdest[32]=psrc[927];case 32:pdest[31]=psrc[928];case 31:pdest[30]=psrc[929];case 30:pdest[29]=psrc[930];case 29:pdest[28]=psrc[931];case 28:pdest[27]=psrc[932];case 27:pdest[26]=psrc[933];case 26:pdest[25]=psrc[934];case 25:pdest[24]=psrc[935];case 24:pdest[23]=psrc[936];case 23:pdest[22]=psrc[937];case 22:pdest[21]=psrc[938];case 21:pdest[20]=psrc[939];case 20:pdest[19]=psrc[940];case 19:pdest[18]=psrc[941];case 18:pdest[17]=psrc[942];case 17:pdest[16]=psrc[943];case 16:pdest[15]=psrc[944];case 15:pdest[14]=psrc[945];case 14:pdest[13]=psrc[946];case 13:pdest[12]=psrc[947];case 12:pdest[11]=psrc[948];case 11:pdest[10]=psrc[949];case 10:pdest[9]=psrc[950];case 9:pdest[8]=psrc[951];case 8:pdest[7]=psrc[952];case 7:pdest[6]=psrc[953];case 6:pdest[5]=psrc[954];case 5:pdest[4]=psrc[955];case 4:pdest[3]=psrc[956];case 3:pdest[2]=psrc[957];case 2:pdest[1]=psrc[958];case 1:pdest[0]=psrc[959];
#define _revpcpy case 960:pdest[959]=pp[psrc[0]];case 959:pdest[958]=pp[psrc[1]];case 958:pdest[957]=pp[psrc[2]];case 957:pdest[956]=pp[psrc[3]];case 956:pdest[955]=pp[psrc[4]];case 955:pdest[954]=pp[psrc[5]];case 954:pdest[953]=pp[psrc[6]];case 953:pdest[952]=pp[psrc[7]];case 952:pdest[951]=pp[psrc[8]];case 951:pdest[950]=pp[psrc[9]];case 950:pdest[949]=pp[psrc[10]];case 949:pdest[948]=pp[psrc[11]];case 948:pdest[947]=pp[psrc[12]];case 947:pdest[946]=pp[psrc[13]];case 946:pdest[945]=pp[psrc[14]];case 945:pdest[944]=pp[psrc[15]];case 944:pdest[943]=pp[psrc[16]];case 943:pdest[942]=pp[psrc[17]];case 942:pdest[941]=pp[psrc[18]];case 941:pdest[940]=pp[psrc[19]];case 940:pdest[939]=pp[psrc[20]];case 939:pdest[938]=pp[psrc[21]];case 938:pdest[937]=pp[psrc[22]];case 937:pdest[936]=pp[psrc[23]];case 936:pdest[935]=pp[psrc[24]];case 935:pdest[934]=pp[psrc[25]];case 934:pdest[933]=pp[psrc[26]];case 933:pdest[932]=pp[psrc[27]];case 932:pdest[931]=pp[psrc[28]];case 931:pdest[930]=pp[psrc[29]];case 930:pdest[929]=pp[psrc[30]];case 929:pdest[928]=pp[psrc[31]];case 928:pdest[927]=pp[psrc[32]];case 927:pdest[926]=pp[psrc[33]];case 926:pdest[925]=pp[psrc[34]];case 925:pdest[924]=pp[psrc[35]];case 924:pdest[923]=pp[psrc[36]];case 923:pdest[922]=pp[psrc[37]];case 922:pdest[921]=pp[psrc[38]];case 921:pdest[920]=pp[psrc[39]];case 920:pdest[919]=pp[psrc[40]];case 919:pdest[918]=pp[psrc[41]];case 918:pdest[917]=pp[psrc[42]];case 917:pdest[916]=pp[psrc[43]];case 916:pdest[915]=pp[psrc[44]];case 915:pdest[914]=pp[psrc[45]];case 914:pdest[913]=pp[psrc[46]];case 913:pdest[912]=pp[psrc[47]];case 912:pdest[911]=pp[psrc[48]];case 911:pdest[910]=pp[psrc[49]];case 910:pdest[909]=pp[psrc[50]];case 909:pdest[908]=pp[psrc[51]];case 908:pdest[907]=pp[psrc[52]];case 907:pdest[906]=pp[psrc[53]];case 906:pdest[905]=pp[psrc[54]];case 905:pdest[904]=pp[psrc[55]];case 904:pdest[903]=pp[psrc[56]];case 903:pdest[902]=pp[psrc[57]];case 902:pdest[901]=pp[psrc[58]];case 901:pdest[900]=pp[psrc[59]];case 900:pdest[899]=pp[psrc[60]];case 899:pdest[898]=pp[psrc[61]];case 898:pdest[897]=pp[psrc[62]];case 897:pdest[896]=pp[psrc[63]];case 896:pdest[895]=pp[psrc[64]];case 895:pdest[894]=pp[psrc[65]];case 894:pdest[893]=pp[psrc[66]];case 893:pdest[892]=pp[psrc[67]];case 892:pdest[891]=pp[psrc[68]];case 891:pdest[890]=pp[psrc[69]];case 890:pdest[889]=pp[psrc[70]];case 889:pdest[888]=pp[psrc[71]];case 888:pdest[887]=pp[psrc[72]];case 887:pdest[886]=pp[psrc[73]];case 886:pdest[885]=pp[psrc[74]];case 885:pdest[884]=pp[psrc[75]];case 884:pdest[883]=pp[psrc[76]];case 883:pdest[882]=pp[psrc[77]];case 882:pdest[881]=pp[psrc[78]];case 881:pdest[880]=pp[psrc[79]];case 880:pdest[879]=pp[psrc[80]];case 879:pdest[878]=pp[psrc[81]];case 878:pdest[877]=pp[psrc[82]];case 877:pdest[876]=pp[psrc[83]];case 876:pdest[875]=pp[psrc[84]];case 875:pdest[874]=pp[psrc[85]];case 874:pdest[873]=pp[psrc[86]];case 873:pdest[872]=pp[psrc[87]];case 872:pdest[871]=pp[psrc[88]];case 871:pdest[870]=pp[psrc[89]];case 870:pdest[869]=pp[psrc[90]];case 869:pdest[868]=pp[psrc[91]];case 868:pdest[867]=pp[psrc[92]];case 867:pdest[866]=pp[psrc[93]];case 866:pdest[865]=pp[psrc[94]];case 865:pdest[864]=pp[psrc[95]];case 864:pdest[863]=pp[psrc[96]];case 863:pdest[862]=pp[psrc[97]];case 862:pdest[861]=pp[psrc[98]];case 861:pdest[860]=pp[psrc[99]];case 860:pdest[859]=pp[psrc[100]];case 859:pdest[858]=pp[psrc[101]];case 858:pdest[857]=pp[psrc[102]];case 857:pdest[856]=pp[psrc[103]];case 856:pdest[855]=pp[psrc[104]];case 855:pdest[854]=pp[psrc[105]];case 854:pdest[853]=pp[psrc[106]];case 853:pdest[852]=pp[psrc[107]];case 852:pdest[851]=pp[psrc[108]];case 851:pdest[850]=pp[psrc[109]];case 850:pdest[849]=pp[psrc[110]];case 849:pdest[848]=pp[psrc[111]];case 848:pdest[847]=pp[psrc[112]];case 847:pdest[846]=pp[psrc[113]];case 846:pdest[845]=pp[psrc[114]];case 845:pdest[844]=pp[psrc[115]];case 844:pdest[843]=pp[psrc[116]];case 843:pdest[842]=pp[psrc[117]];case 842:pdest[841]=pp[psrc[118]];case 841:pdest[840]=pp[psrc[119]];case 840:pdest[839]=pp[psrc[120]];case 839:pdest[838]=pp[psrc[121]];case 838:pdest[837]=pp[psrc[122]];case 837:pdest[836]=pp[psrc[123]];case 836:pdest[835]=pp[psrc[124]];case 835:pdest[834]=pp[psrc[125]];case 834:pdest[833]=pp[psrc[126]];case 833:pdest[832]=pp[psrc[127]];case 832:pdest[831]=pp[psrc[128]];case 831:pdest[830]=pp[psrc[129]];case 830:pdest[829]=pp[psrc[130]];case 829:pdest[828]=pp[psrc[131]];case 828:pdest[827]=pp[psrc[132]];case 827:pdest[826]=pp[psrc[133]];case 826:pdest[825]=pp[psrc[134]];case 825:pdest[824]=pp[psrc[135]];case 824:pdest[823]=pp[psrc[136]];case 823:pdest[822]=pp[psrc[137]];case 822:pdest[821]=pp[psrc[138]];case 821:pdest[820]=pp[psrc[139]];case 820:pdest[819]=pp[psrc[140]];case 819:pdest[818]=pp[psrc[141]];case 818:pdest[817]=pp[psrc[142]];case 817:pdest[816]=pp[psrc[143]];case 816:pdest[815]=pp[psrc[144]];case 815:pdest[814]=pp[psrc[145]];case 814:pdest[813]=pp[psrc[146]];case 813:pdest[812]=pp[psrc[147]];case 812:pdest[811]=pp[psrc[148]];case 811:pdest[810]=pp[psrc[149]];case 810:pdest[809]=pp[psrc[150]];case 809:pdest[808]=pp[psrc[151]];case 808:pdest[807]=pp[psrc[152]];case 807:pdest[806]=pp[psrc[153]];case 806:pdest[805]=pp[psrc[154]];case 805:pdest[804]=pp[psrc[155]];case 804:pdest[803]=pp[psrc[156]];case 803:pdest[802]=pp[psrc[157]];case 802:pdest[801]=pp[psrc[158]];case 801:pdest[800]=pp[psrc[159]];case 800:pdest[799]=pp[psrc[160]];case 799:pdest[798]=pp[psrc[161]];case 798:pdest[797]=pp[psrc[162]];case 797:pdest[796]=pp[psrc[163]];case 796:pdest[795]=pp[psrc[164]];case 795:pdest[794]=pp[psrc[165]];case 794:pdest[793]=pp[psrc[166]];case 793:pdest[792]=pp[psrc[167]];case 792:pdest[791]=pp[psrc[168]];case 791:pdest[790]=pp[psrc[169]];case 790:pdest[789]=pp[psrc[170]];case 789:pdest[788]=pp[psrc[171]];case 788:pdest[787]=pp[psrc[172]];case 787:pdest[786]=pp[psrc[173]];case 786:pdest[785]=pp[psrc[174]];case 785:pdest[784]=pp[psrc[175]];case 784:pdest[783]=pp[psrc[176]];case 783:pdest[782]=pp[psrc[177]];case 782:pdest[781]=pp[psrc[178]];case 781:pdest[780]=pp[psrc[179]];case 780:pdest[779]=pp[psrc[180]];case 779:pdest[778]=pp[psrc[181]];case 778:pdest[777]=pp[psrc[182]];case 777:pdest[776]=pp[psrc[183]];case 776:pdest[775]=pp[psrc[184]];case 775:pdest[774]=pp[psrc[185]];case 774:pdest[773]=pp[psrc[186]];case 773:pdest[772]=pp[psrc[187]];case 772:pdest[771]=pp[psrc[188]];case 771:pdest[770]=pp[psrc[189]];case 770:pdest[769]=pp[psrc[190]];case 769:pdest[768]=pp[psrc[191]];case 768:pdest[767]=pp[psrc[192]];case 767:pdest[766]=pp[psrc[193]];case 766:pdest[765]=pp[psrc[194]];case 765:pdest[764]=pp[psrc[195]];case 764:pdest[763]=pp[psrc[196]];case 763:pdest[762]=pp[psrc[197]];case 762:pdest[761]=pp[psrc[198]];case 761:pdest[760]=pp[psrc[199]];case 760:pdest[759]=pp[psrc[200]];case 759:pdest[758]=pp[psrc[201]];case 758:pdest[757]=pp[psrc[202]];case 757:pdest[756]=pp[psrc[203]];case 756:pdest[755]=pp[psrc[204]];case 755:pdest[754]=pp[psrc[205]];case 754:pdest[753]=pp[psrc[206]];case 753:pdest[752]=pp[psrc[207]];case 752:pdest[751]=pp[psrc[208]];case 751:pdest[750]=pp[psrc[209]];case 750:pdest[749]=pp[psrc[210]];case 749:pdest[748]=pp[psrc[211]];case 748:pdest[747]=pp[psrc[212]];case 747:pdest[746]=pp[psrc[213]];case 746:pdest[745]=pp[psrc[214]];case 745:pdest[744]=pp[psrc[215]];case 744:pdest[743]=pp[psrc[216]];case 743:pdest[742]=pp[psrc[217]];case 742:pdest[741]=pp[psrc[218]];case 741:pdest[740]=pp[psrc[219]];case 740:pdest[739]=pp[psrc[220]];case 739:pdest[738]=pp[psrc[221]];case 738:pdest[737]=pp[psrc[222]];case 737:pdest[736]=pp[psrc[223]];case 736:pdest[735]=pp[psrc[224]];case 735:pdest[734]=pp[psrc[225]];case 734:pdest[733]=pp[psrc[226]];case 733:pdest[732]=pp[psrc[227]];case 732:pdest[731]=pp[psrc[228]];case 731:pdest[730]=pp[psrc[229]];case 730:pdest[729]=pp[psrc[230]];case 729:pdest[728]=pp[psrc[231]];case 728:pdest[727]=pp[psrc[232]];case 727:pdest[726]=pp[psrc[233]];case 726:pdest[725]=pp[psrc[234]];case 725:pdest[724]=pp[psrc[235]];case 724:pdest[723]=pp[psrc[236]];case 723:pdest[722]=pp[psrc[237]];case 722:pdest[721]=pp[psrc[238]];case 721:pdest[720]=pp[psrc[239]];case 720:pdest[719]=pp[psrc[240]];case 719:pdest[718]=pp[psrc[241]];case 718:pdest[717]=pp[psrc[242]];case 717:pdest[716]=pp[psrc[243]];case 716:pdest[715]=pp[psrc[244]];case 715:pdest[714]=pp[psrc[245]];case 714:pdest[713]=pp[psrc[246]];case 713:pdest[712]=pp[psrc[247]];case 712:pdest[711]=pp[psrc[248]];case 711:pdest[710]=pp[psrc[249]];case 710:pdest[709]=pp[psrc[250]];case 709:pdest[708]=pp[psrc[251]];case 708:pdest[707]=pp[psrc[252]];case 707:pdest[706]=pp[psrc[253]];case 706:pdest[705]=pp[psrc[254]];case 705:pdest[704]=pp[psrc[255]];case 704:pdest[703]=pp[psrc[256]];case 703:pdest[702]=pp[psrc[257]];case 702:pdest[701]=pp[psrc[258]];case 701:pdest[700]=pp[psrc[259]];case 700:pdest[699]=pp[psrc[260]];case 699:pdest[698]=pp[psrc[261]];case 698:pdest[697]=pp[psrc[262]];case 697:pdest[696]=pp[psrc[263]];case 696:pdest[695]=pp[psrc[264]];case 695:pdest[694]=pp[psrc[265]];case 694:pdest[693]=pp[psrc[266]];case 693:pdest[692]=pp[psrc[267]];case 692:pdest[691]=pp[psrc[268]];case 691:pdest[690]=pp[psrc[269]];case 690:pdest[689]=pp[psrc[270]];case 689:pdest[688]=pp[psrc[271]];case 688:pdest[687]=pp[psrc[272]];case 687:pdest[686]=pp[psrc[273]];case 686:pdest[685]=pp[psrc[274]];case 685:pdest[684]=pp[psrc[275]];case 684:pdest[683]=pp[psrc[276]];case 683:pdest[682]=pp[psrc[277]];case 682:pdest[681]=pp[psrc[278]];case 681:pdest[680]=pp[psrc[279]];case 680:pdest[679]=pp[psrc[280]];case 679:pdest[678]=pp[psrc[281]];case 678:pdest[677]=pp[psrc[282]];case 677:pdest[676]=pp[psrc[283]];case 676:pdest[675]=pp[psrc[284]];case 675:pdest[674]=pp[psrc[285]];case 674:pdest[673]=pp[psrc[286]];case 673:pdest[672]=pp[psrc[287]];case 672:pdest[671]=pp[psrc[288]];case 671:pdest[670]=pp[psrc[289]];case 670:pdest[669]=pp[psrc[290]];case 669:pdest[668]=pp[psrc[291]];case 668:pdest[667]=pp[psrc[292]];case 667:pdest[666]=pp[psrc[293]];case 666:pdest[665]=pp[psrc[294]];case 665:pdest[664]=pp[psrc[295]];case 664:pdest[663]=pp[psrc[296]];case 663:pdest[662]=pp[psrc[297]];case 662:pdest[661]=pp[psrc[298]];case 661:pdest[660]=pp[psrc[299]];case 660:pdest[659]=pp[psrc[300]];case 659:pdest[658]=pp[psrc[301]];case 658:pdest[657]=pp[psrc[302]];case 657:pdest[656]=pp[psrc[303]];case 656:pdest[655]=pp[psrc[304]];case 655:pdest[654]=pp[psrc[305]];case 654:pdest[653]=pp[psrc[306]];case 653:pdest[652]=pp[psrc[307]];case 652:pdest[651]=pp[psrc[308]];case 651:pdest[650]=pp[psrc[309]];case 650:pdest[649]=pp[psrc[310]];case 649:pdest[648]=pp[psrc[311]];case 648:pdest[647]=pp[psrc[312]];case 647:pdest[646]=pp[psrc[313]];case 646:pdest[645]=pp[psrc[314]];case 645:pdest[644]=pp[psrc[315]];case 644:pdest[643]=pp[psrc[316]];case 643:pdest[642]=pp[psrc[317]];case 642:pdest[641]=pp[psrc[318]];case 641:pdest[640]=pp[psrc[319]];case 640:pdest[639]=pp[psrc[320]];case 639:pdest[638]=pp[psrc[321]];case 638:pdest[637]=pp[psrc[322]];case 637:pdest[636]=pp[psrc[323]];case 636:pdest[635]=pp[psrc[324]];case 635:pdest[634]=pp[psrc[325]];case 634:pdest[633]=pp[psrc[326]];case 633:pdest[632]=pp[psrc[327]];case 632:pdest[631]=pp[psrc[328]];case 631:pdest[630]=pp[psrc[329]];case 630:pdest[629]=pp[psrc[330]];case 629:pdest[628]=pp[psrc[331]];case 628:pdest[627]=pp[psrc[332]];case 627:pdest[626]=pp[psrc[333]];case 626:pdest[625]=pp[psrc[334]];case 625:pdest[624]=pp[psrc[335]];case 624:pdest[623]=pp[psrc[336]];case 623:pdest[622]=pp[psrc[337]];case 622:pdest[621]=pp[psrc[338]];case 621:pdest[620]=pp[psrc[339]];case 620:pdest[619]=pp[psrc[340]];case 619:pdest[618]=pp[psrc[341]];case 618:pdest[617]=pp[psrc[342]];case 617:pdest[616]=pp[psrc[343]];case 616:pdest[615]=pp[psrc[344]];case 615:pdest[614]=pp[psrc[345]];case 614:pdest[613]=pp[psrc[346]];case 613:pdest[612]=pp[psrc[347]];case 612:pdest[611]=pp[psrc[348]];case 611:pdest[610]=pp[psrc[349]];case 610:pdest[609]=pp[psrc[350]];case 609:pdest[608]=pp[psrc[351]];case 608:pdest[607]=pp[psrc[352]];case 607:pdest[606]=pp[psrc[353]];case 606:pdest[605]=pp[psrc[354]];case 605:pdest[604]=pp[psrc[355]];case 604:pdest[603]=pp[psrc[356]];case 603:pdest[602]=pp[psrc[357]];case 602:pdest[601]=pp[psrc[358]];case 601:pdest[600]=pp[psrc[359]];case 600:pdest[599]=pp[psrc[360]];case 599:pdest[598]=pp[psrc[361]];case 598:pdest[597]=pp[psrc[362]];case 597:pdest[596]=pp[psrc[363]];case 596:pdest[595]=pp[psrc[364]];case 595:pdest[594]=pp[psrc[365]];case 594:pdest[593]=pp[psrc[366]];case 593:pdest[592]=pp[psrc[367]];case 592:pdest[591]=pp[psrc[368]];case 591:pdest[590]=pp[psrc[369]];case 590:pdest[589]=pp[psrc[370]];case 589:pdest[588]=pp[psrc[371]];case 588:pdest[587]=pp[psrc[372]];case 587:pdest[586]=pp[psrc[373]];case 586:pdest[585]=pp[psrc[374]];case 585:pdest[584]=pp[psrc[375]];case 584:pdest[583]=pp[psrc[376]];case 583:pdest[582]=pp[psrc[377]];case 582:pdest[581]=pp[psrc[378]];case 581:pdest[580]=pp[psrc[379]];case 580:pdest[579]=pp[psrc[380]];case 579:pdest[578]=pp[psrc[381]];case 578:pdest[577]=pp[psrc[382]];case 577:pdest[576]=pp[psrc[383]];case 576:pdest[575]=pp[psrc[384]];case 575:pdest[574]=pp[psrc[385]];case 574:pdest[573]=pp[psrc[386]];case 573:pdest[572]=pp[psrc[387]];case 572:pdest[571]=pp[psrc[388]];case 571:pdest[570]=pp[psrc[389]];case 570:pdest[569]=pp[psrc[390]];case 569:pdest[568]=pp[psrc[391]];case 568:pdest[567]=pp[psrc[392]];case 567:pdest[566]=pp[psrc[393]];case 566:pdest[565]=pp[psrc[394]];case 565:pdest[564]=pp[psrc[395]];case 564:pdest[563]=pp[psrc[396]];case 563:pdest[562]=pp[psrc[397]];case 562:pdest[561]=pp[psrc[398]];case 561:pdest[560]=pp[psrc[399]];case 560:pdest[559]=pp[psrc[400]];case 559:pdest[558]=pp[psrc[401]];case 558:pdest[557]=pp[psrc[402]];case 557:pdest[556]=pp[psrc[403]];case 556:pdest[555]=pp[psrc[404]];case 555:pdest[554]=pp[psrc[405]];case 554:pdest[553]=pp[psrc[406]];case 553:pdest[552]=pp[psrc[407]];case 552:pdest[551]=pp[psrc[408]];case 551:pdest[550]=pp[psrc[409]];case 550:pdest[549]=pp[psrc[410]];case 549:pdest[548]=pp[psrc[411]];case 548:pdest[547]=pp[psrc[412]];case 547:pdest[546]=pp[psrc[413]];case 546:pdest[545]=pp[psrc[414]];case 545:pdest[544]=pp[psrc[415]];case 544:pdest[543]=pp[psrc[416]];case 543:pdest[542]=pp[psrc[417]];case 542:pdest[541]=pp[psrc[418]];case 541:pdest[540]=pp[psrc[419]];case 540:pdest[539]=pp[psrc[420]];case 539:pdest[538]=pp[psrc[421]];case 538:pdest[537]=pp[psrc[422]];case 537:pdest[536]=pp[psrc[423]];case 536:pdest[535]=pp[psrc[424]];case 535:pdest[534]=pp[psrc[425]];case 534:pdest[533]=pp[psrc[426]];case 533:pdest[532]=pp[psrc[427]];case 532:pdest[531]=pp[psrc[428]];case 531:pdest[530]=pp[psrc[429]];case 530:pdest[529]=pp[psrc[430]];case 529:pdest[528]=pp[psrc[431]];case 528:pdest[527]=pp[psrc[432]];case 527:pdest[526]=pp[psrc[433]];case 526:pdest[525]=pp[psrc[434]];case 525:pdest[524]=pp[psrc[435]];case 524:pdest[523]=pp[psrc[436]];case 523:pdest[522]=pp[psrc[437]];case 522:pdest[521]=pp[psrc[438]];case 521:pdest[520]=pp[psrc[439]];case 520:pdest[519]=pp[psrc[440]];case 519:pdest[518]=pp[psrc[441]];case 518:pdest[517]=pp[psrc[442]];case 517:pdest[516]=pp[psrc[443]];case 516:pdest[515]=pp[psrc[444]];case 515:pdest[514]=pp[psrc[445]];case 514:pdest[513]=pp[psrc[446]];case 513:pdest[512]=pp[psrc[447]];case 512:pdest[511]=pp[psrc[448]];case 511:pdest[510]=pp[psrc[449]];case 510:pdest[509]=pp[psrc[450]];case 509:pdest[508]=pp[psrc[451]];case 508:pdest[507]=pp[psrc[452]];case 507:pdest[506]=pp[psrc[453]];case 506:pdest[505]=pp[psrc[454]];case 505:pdest[504]=pp[psrc[455]];case 504:pdest[503]=pp[psrc[456]];case 503:pdest[502]=pp[psrc[457]];case 502:pdest[501]=pp[psrc[458]];case 501:pdest[500]=pp[psrc[459]];case 500:pdest[499]=pp[psrc[460]];case 499:pdest[498]=pp[psrc[461]];case 498:pdest[497]=pp[psrc[462]];case 497:pdest[496]=pp[psrc[463]];case 496:pdest[495]=pp[psrc[464]];case 495:pdest[494]=pp[psrc[465]];case 494:pdest[493]=pp[psrc[466]];case 493:pdest[492]=pp[psrc[467]];case 492:pdest[491]=pp[psrc[468]];case 491:pdest[490]=pp[psrc[469]];case 490:pdest[489]=pp[psrc[470]];case 489:pdest[488]=pp[psrc[471]];case 488:pdest[487]=pp[psrc[472]];case 487:pdest[486]=pp[psrc[473]];case 486:pdest[485]=pp[psrc[474]];case 485:pdest[484]=pp[psrc[475]];case 484:pdest[483]=pp[psrc[476]];case 483:pdest[482]=pp[psrc[477]];case 482:pdest[481]=pp[psrc[478]];case 481:pdest[480]=pp[psrc[479]];case 480:pdest[479]=pp[psrc[480]];case 479:pdest[478]=pp[psrc[481]];case 478:pdest[477]=pp[psrc[482]];case 477:pdest[476]=pp[psrc[483]];case 476:pdest[475]=pp[psrc[484]];case 475:pdest[474]=pp[psrc[485]];case 474:pdest[473]=pp[psrc[486]];case 473:pdest[472]=pp[psrc[487]];case 472:pdest[471]=pp[psrc[488]];case 471:pdest[470]=pp[psrc[489]];case 470:pdest[469]=pp[psrc[490]];case 469:pdest[468]=pp[psrc[491]];case 468:pdest[467]=pp[psrc[492]];case 467:pdest[466]=pp[psrc[493]];case 466:pdest[465]=pp[psrc[494]];case 465:pdest[464]=pp[psrc[495]];case 464:pdest[463]=pp[psrc[496]];case 463:pdest[462]=pp[psrc[497]];case 462:pdest[461]=pp[psrc[498]];case 461:pdest[460]=pp[psrc[499]];case 460:pdest[459]=pp[psrc[500]];case 459:pdest[458]=pp[psrc[501]];case 458:pdest[457]=pp[psrc[502]];case 457:pdest[456]=pp[psrc[503]];case 456:pdest[455]=pp[psrc[504]];case 455:pdest[454]=pp[psrc[505]];case 454:pdest[453]=pp[psrc[506]];case 453:pdest[452]=pp[psrc[507]];case 452:pdest[451]=pp[psrc[508]];case 451:pdest[450]=pp[psrc[509]];case 450:pdest[449]=pp[psrc[510]];case 449:pdest[448]=pp[psrc[511]];case 448:pdest[447]=pp[psrc[512]];case 447:pdest[446]=pp[psrc[513]];case 446:pdest[445]=pp[psrc[514]];case 445:pdest[444]=pp[psrc[515]];case 444:pdest[443]=pp[psrc[516]];case 443:pdest[442]=pp[psrc[517]];case 442:pdest[441]=pp[psrc[518]];case 441:pdest[440]=pp[psrc[519]];case 440:pdest[439]=pp[psrc[520]];case 439:pdest[438]=pp[psrc[521]];case 438:pdest[437]=pp[psrc[522]];case 437:pdest[436]=pp[psrc[523]];case 436:pdest[435]=pp[psrc[524]];case 435:pdest[434]=pp[psrc[525]];case 434:pdest[433]=pp[psrc[526]];case 433:pdest[432]=pp[psrc[527]];case 432:pdest[431]=pp[psrc[528]];case 431:pdest[430]=pp[psrc[529]];case 430:pdest[429]=pp[psrc[530]];case 429:pdest[428]=pp[psrc[531]];case 428:pdest[427]=pp[psrc[532]];case 427:pdest[426]=pp[psrc[533]];case 426:pdest[425]=pp[psrc[534]];case 425:pdest[424]=pp[psrc[535]];case 424:pdest[423]=pp[psrc[536]];case 423:pdest[422]=pp[psrc[537]];case 422:pdest[421]=pp[psrc[538]];case 421:pdest[420]=pp[psrc[539]];case 420:pdest[419]=pp[psrc[540]];case 419:pdest[418]=pp[psrc[541]];case 418:pdest[417]=pp[psrc[542]];case 417:pdest[416]=pp[psrc[543]];case 416:pdest[415]=pp[psrc[544]];case 415:pdest[414]=pp[psrc[545]];case 414:pdest[413]=pp[psrc[546]];case 413:pdest[412]=pp[psrc[547]];case 412:pdest[411]=pp[psrc[548]];case 411:pdest[410]=pp[psrc[549]];case 410:pdest[409]=pp[psrc[550]];case 409:pdest[408]=pp[psrc[551]];case 408:pdest[407]=pp[psrc[552]];case 407:pdest[406]=pp[psrc[553]];case 406:pdest[405]=pp[psrc[554]];case 405:pdest[404]=pp[psrc[555]];case 404:pdest[403]=pp[psrc[556]];case 403:pdest[402]=pp[psrc[557]];case 402:pdest[401]=pp[psrc[558]];case 401:pdest[400]=pp[psrc[559]];case 400:pdest[399]=pp[psrc[560]];case 399:pdest[398]=pp[psrc[561]];case 398:pdest[397]=pp[psrc[562]];case 397:pdest[396]=pp[psrc[563]];case 396:pdest[395]=pp[psrc[564]];case 395:pdest[394]=pp[psrc[565]];case 394:pdest[393]=pp[psrc[566]];case 393:pdest[392]=pp[psrc[567]];case 392:pdest[391]=pp[psrc[568]];case 391:pdest[390]=pp[psrc[569]];case 390:pdest[389]=pp[psrc[570]];case 389:pdest[388]=pp[psrc[571]];case 388:pdest[387]=pp[psrc[572]];case 387:pdest[386]=pp[psrc[573]];case 386:pdest[385]=pp[psrc[574]];case 385:pdest[384]=pp[psrc[575]];case 384:pdest[383]=pp[psrc[576]];case 383:pdest[382]=pp[psrc[577]];case 382:pdest[381]=pp[psrc[578]];case 381:pdest[380]=pp[psrc[579]];case 380:pdest[379]=pp[psrc[580]];case 379:pdest[378]=pp[psrc[581]];case 378:pdest[377]=pp[psrc[582]];case 377:pdest[376]=pp[psrc[583]];case 376:pdest[375]=pp[psrc[584]];case 375:pdest[374]=pp[psrc[585]];case 374:pdest[373]=pp[psrc[586]];case 373:pdest[372]=pp[psrc[587]];case 372:pdest[371]=pp[psrc[588]];case 371:pdest[370]=pp[psrc[589]];case 370:pdest[369]=pp[psrc[590]];case 369:pdest[368]=pp[psrc[591]];case 368:pdest[367]=pp[psrc[592]];case 367:pdest[366]=pp[psrc[593]];case 366:pdest[365]=pp[psrc[594]];case 365:pdest[364]=pp[psrc[595]];case 364:pdest[363]=pp[psrc[596]];case 363:pdest[362]=pp[psrc[597]];case 362:pdest[361]=pp[psrc[598]];case 361:pdest[360]=pp[psrc[599]];case 360:pdest[359]=pp[psrc[600]];case 359:pdest[358]=pp[psrc[601]];case 358:pdest[357]=pp[psrc[602]];case 357:pdest[356]=pp[psrc[603]];case 356:pdest[355]=pp[psrc[604]];case 355:pdest[354]=pp[psrc[605]];case 354:pdest[353]=pp[psrc[606]];case 353:pdest[352]=pp[psrc[607]];case 352:pdest[351]=pp[psrc[608]];case 351:pdest[350]=pp[psrc[609]];case 350:pdest[349]=pp[psrc[610]];case 349:pdest[348]=pp[psrc[611]];case 348:pdest[347]=pp[psrc[612]];case 347:pdest[346]=pp[psrc[613]];case 346:pdest[345]=pp[psrc[614]];case 345:pdest[344]=pp[psrc[615]];case 344:pdest[343]=pp[psrc[616]];case 343:pdest[342]=pp[psrc[617]];case 342:pdest[341]=pp[psrc[618]];case 341:pdest[340]=pp[psrc[619]];case 340:pdest[339]=pp[psrc[620]];case 339:pdest[338]=pp[psrc[621]];case 338:pdest[337]=pp[psrc[622]];case 337:pdest[336]=pp[psrc[623]];case 336:pdest[335]=pp[psrc[624]];case 335:pdest[334]=pp[psrc[625]];case 334:pdest[333]=pp[psrc[626]];case 333:pdest[332]=pp[psrc[627]];case 332:pdest[331]=pp[psrc[628]];case 331:pdest[330]=pp[psrc[629]];case 330:pdest[329]=pp[psrc[630]];case 329:pdest[328]=pp[psrc[631]];case 328:pdest[327]=pp[psrc[632]];case 327:pdest[326]=pp[psrc[633]];case 326:pdest[325]=pp[psrc[634]];case 325:pdest[324]=pp[psrc[635]];case 324:pdest[323]=pp[psrc[636]];case 323:pdest[322]=pp[psrc[637]];case 322:pdest[321]=pp[psrc[638]];case 321:pdest[320]=pp[psrc[639]];case 320:pdest[319]=pp[psrc[640]];case 319:pdest[318]=pp[psrc[641]];case 318:pdest[317]=pp[psrc[642]];case 317:pdest[316]=pp[psrc[643]];case 316:pdest[315]=pp[psrc[644]];case 315:pdest[314]=pp[psrc[645]];case 314:pdest[313]=pp[psrc[646]];case 313:pdest[312]=pp[psrc[647]];case 312:pdest[311]=pp[psrc[648]];case 311:pdest[310]=pp[psrc[649]];case 310:pdest[309]=pp[psrc[650]];case 309:pdest[308]=pp[psrc[651]];case 308:pdest[307]=pp[psrc[652]];case 307:pdest[306]=pp[psrc[653]];case 306:pdest[305]=pp[psrc[654]];case 305:pdest[304]=pp[psrc[655]];case 304:pdest[303]=pp[psrc[656]];case 303:pdest[302]=pp[psrc[657]];case 302:pdest[301]=pp[psrc[658]];case 301:pdest[300]=pp[psrc[659]];case 300:pdest[299]=pp[psrc[660]];case 299:pdest[298]=pp[psrc[661]];case 298:pdest[297]=pp[psrc[662]];case 297:pdest[296]=pp[psrc[663]];case 296:pdest[295]=pp[psrc[664]];case 295:pdest[294]=pp[psrc[665]];case 294:pdest[293]=pp[psrc[666]];case 293:pdest[292]=pp[psrc[667]];case 292:pdest[291]=pp[psrc[668]];case 291:pdest[290]=pp[psrc[669]];case 290:pdest[289]=pp[psrc[670]];case 289:pdest[288]=pp[psrc[671]];case 288:pdest[287]=pp[psrc[672]];case 287:pdest[286]=pp[psrc[673]];case 286:pdest[285]=pp[psrc[674]];case 285:pdest[284]=pp[psrc[675]];case 284:pdest[283]=pp[psrc[676]];case 283:pdest[282]=pp[psrc[677]];case 282:pdest[281]=pp[psrc[678]];case 281:pdest[280]=pp[psrc[679]];case 280:pdest[279]=pp[psrc[680]];case 279:pdest[278]=pp[psrc[681]];case 278:pdest[277]=pp[psrc[682]];case 277:pdest[276]=pp[psrc[683]];case 276:pdest[275]=pp[psrc[684]];case 275:pdest[274]=pp[psrc[685]];case 274:pdest[273]=pp[psrc[686]];case 273:pdest[272]=pp[psrc[687]];case 272:pdest[271]=pp[psrc[688]];case 271:pdest[270]=pp[psrc[689]];case 270:pdest[269]=pp[psrc[690]];case 269:pdest[268]=pp[psrc[691]];case 268:pdest[267]=pp[psrc[692]];case 267:pdest[266]=pp[psrc[693]];case 266:pdest[265]=pp[psrc[694]];case 265:pdest[264]=pp[psrc[695]];case 264:pdest[263]=pp[psrc[696]];case 263:pdest[262]=pp[psrc[697]];case 262:pdest[261]=pp[psrc[698]];case 261:pdest[260]=pp[psrc[699]];case 260:pdest[259]=pp[psrc[700]];case 259:pdest[258]=pp[psrc[701]];case 258:pdest[257]=pp[psrc[702]];case 257:pdest[256]=pp[psrc[703]];case 256:pdest[255]=pp[psrc[704]];case 255:pdest[254]=pp[psrc[705]];case 254:pdest[253]=pp[psrc[706]];case 253:pdest[252]=pp[psrc[707]];case 252:pdest[251]=pp[psrc[708]];case 251:pdest[250]=pp[psrc[709]];case 250:pdest[249]=pp[psrc[710]];case 249:pdest[248]=pp[psrc[711]];case 248:pdest[247]=pp[psrc[712]];case 247:pdest[246]=pp[psrc[713]];case 246:pdest[245]=pp[psrc[714]];case 245:pdest[244]=pp[psrc[715]];case 244:pdest[243]=pp[psrc[716]];case 243:pdest[242]=pp[psrc[717]];case 242:pdest[241]=pp[psrc[718]];case 241:pdest[240]=pp[psrc[719]];case 240:pdest[239]=pp[psrc[720]];case 239:pdest[238]=pp[psrc[721]];case 238:pdest[237]=pp[psrc[722]];case 237:pdest[236]=pp[psrc[723]];case 236:pdest[235]=pp[psrc[724]];case 235:pdest[234]=pp[psrc[725]];case 234:pdest[233]=pp[psrc[726]];case 233:pdest[232]=pp[psrc[727]];case 232:pdest[231]=pp[psrc[728]];case 231:pdest[230]=pp[psrc[729]];case 230:pdest[229]=pp[psrc[730]];case 229:pdest[228]=pp[psrc[731]];case 228:pdest[227]=pp[psrc[732]];case 227:pdest[226]=pp[psrc[733]];case 226:pdest[225]=pp[psrc[734]];case 225:pdest[224]=pp[psrc[735]];case 224:pdest[223]=pp[psrc[736]];case 223:pdest[222]=pp[psrc[737]];case 222:pdest[221]=pp[psrc[738]];case 221:pdest[220]=pp[psrc[739]];case 220:pdest[219]=pp[psrc[740]];case 219:pdest[218]=pp[psrc[741]];case 218:pdest[217]=pp[psrc[742]];case 217:pdest[216]=pp[psrc[743]];case 216:pdest[215]=pp[psrc[744]];case 215:pdest[214]=pp[psrc[745]];case 214:pdest[213]=pp[psrc[746]];case 213:pdest[212]=pp[psrc[747]];case 212:pdest[211]=pp[psrc[748]];case 211:pdest[210]=pp[psrc[749]];case 210:pdest[209]=pp[psrc[750]];case 209:pdest[208]=pp[psrc[751]];case 208:pdest[207]=pp[psrc[752]];case 207:pdest[206]=pp[psrc[753]];case 206:pdest[205]=pp[psrc[754]];case 205:pdest[204]=pp[psrc[755]];case 204:pdest[203]=pp[psrc[756]];case 203:pdest[202]=pp[psrc[757]];case 202:pdest[201]=pp[psrc[758]];case 201:pdest[200]=pp[psrc[759]];case 200:pdest[199]=pp[psrc[760]];case 199:pdest[198]=pp[psrc[761]];case 198:pdest[197]=pp[psrc[762]];case 197:pdest[196]=pp[psrc[763]];case 196:pdest[195]=pp[psrc[764]];case 195:pdest[194]=pp[psrc[765]];case 194:pdest[193]=pp[psrc[766]];case 193:pdest[192]=pp[psrc[767]];case 192:pdest[191]=pp[psrc[768]];case 191:pdest[190]=pp[psrc[769]];case 190:pdest[189]=pp[psrc[770]];case 189:pdest[188]=pp[psrc[771]];case 188:pdest[187]=pp[psrc[772]];case 187:pdest[186]=pp[psrc[773]];case 186:pdest[185]=pp[psrc[774]];case 185:pdest[184]=pp[psrc[775]];case 184:pdest[183]=pp[psrc[776]];case 183:pdest[182]=pp[psrc[777]];case 182:pdest[181]=pp[psrc[778]];case 181:pdest[180]=pp[psrc[779]];case 180:pdest[179]=pp[psrc[780]];case 179:pdest[178]=pp[psrc[781]];case 178:pdest[177]=pp[psrc[782]];case 177:pdest[176]=pp[psrc[783]];case 176:pdest[175]=pp[psrc[784]];case 175:pdest[174]=pp[psrc[785]];case 174:pdest[173]=pp[psrc[786]];case 173:pdest[172]=pp[psrc[787]];case 172:pdest[171]=pp[psrc[788]];case 171:pdest[170]=pp[psrc[789]];case 170:pdest[169]=pp[psrc[790]];case 169:pdest[168]=pp[psrc[791]];case 168:pdest[167]=pp[psrc[792]];case 167:pdest[166]=pp[psrc[793]];case 166:pdest[165]=pp[psrc[794]];case 165:pdest[164]=pp[psrc[795]];case 164:pdest[163]=pp[psrc[796]];case 163:pdest[162]=pp[psrc[797]];case 162:pdest[161]=pp[psrc[798]];case 161:pdest[160]=pp[psrc[799]];case 160:pdest[159]=pp[psrc[800]];case 159:pdest[158]=pp[psrc[801]];case 158:pdest[157]=pp[psrc[802]];case 157:pdest[156]=pp[psrc[803]];case 156:pdest[155]=pp[psrc[804]];case 155:pdest[154]=pp[psrc[805]];case 154:pdest[153]=pp[psrc[806]];case 153:pdest[152]=pp[psrc[807]];case 152:pdest[151]=pp[psrc[808]];case 151:pdest[150]=pp[psrc[809]];case 150:pdest[149]=pp[psrc[810]];case 149:pdest[148]=pp[psrc[811]];case 148:pdest[147]=pp[psrc[812]];case 147:pdest[146]=pp[psrc[813]];case 146:pdest[145]=pp[psrc[814]];case 145:pdest[144]=pp[psrc[815]];case 144:pdest[143]=pp[psrc[816]];case 143:pdest[142]=pp[psrc[817]];case 142:pdest[141]=pp[psrc[818]];case 141:pdest[140]=pp[psrc[819]];case 140:pdest[139]=pp[psrc[820]];case 139:pdest[138]=pp[psrc[821]];case 138:pdest[137]=pp[psrc[822]];case 137:pdest[136]=pp[psrc[823]];case 136:pdest[135]=pp[psrc[824]];case 135:pdest[134]=pp[psrc[825]];case 134:pdest[133]=pp[psrc[826]];case 133:pdest[132]=pp[psrc[827]];case 132:pdest[131]=pp[psrc[828]];case 131:pdest[130]=pp[psrc[829]];case 130:pdest[129]=pp[psrc[830]];case 129:pdest[128]=pp[psrc[831]];case 128:pdest[127]=pp[psrc[832]];case 127:pdest[126]=pp[psrc[833]];case 126:pdest[125]=pp[psrc[834]];case 125:pdest[124]=pp[psrc[835]];case 124:pdest[123]=pp[psrc[836]];case 123:pdest[122]=pp[psrc[837]];case 122:pdest[121]=pp[psrc[838]];case 121:pdest[120]=pp[psrc[839]];case 120:pdest[119]=pp[psrc[840]];case 119:pdest[118]=pp[psrc[841]];case 118:pdest[117]=pp[psrc[842]];case 117:pdest[116]=pp[psrc[843]];case 116:pdest[115]=pp[psrc[844]];case 115:pdest[114]=pp[psrc[845]];case 114:pdest[113]=pp[psrc[846]];case 113:pdest[112]=pp[psrc[847]];case 112:pdest[111]=pp[psrc[848]];case 111:pdest[110]=pp[psrc[849]];case 110:pdest[109]=pp[psrc[850]];case 109:pdest[108]=pp[psrc[851]];case 108:pdest[107]=pp[psrc[852]];case 107:pdest[106]=pp[psrc[853]];case 106:pdest[105]=pp[psrc[854]];case 105:pdest[104]=pp[psrc[855]];case 104:pdest[103]=pp[psrc[856]];case 103:pdest[102]=pp[psrc[857]];case 102:pdest[101]=pp[psrc[858]];case 101:pdest[100]=pp[psrc[859]];case 100:pdest[99]=pp[psrc[860]];case 99:pdest[98]=pp[psrc[861]];case 98:pdest[97]=pp[psrc[862]];case 97:pdest[96]=pp[psrc[863]];case 96:pdest[95]=pp[psrc[864]];case 95:pdest[94]=pp[psrc[865]];case 94:pdest[93]=pp[psrc[866]];case 93:pdest[92]=pp[psrc[867]];case 92:pdest[91]=pp[psrc[868]];case 91:pdest[90]=pp[psrc[869]];case 90:pdest[89]=pp[psrc[870]];case 89:pdest[88]=pp[psrc[871]];case 88:pdest[87]=pp[psrc[872]];case 87:pdest[86]=pp[psrc[873]];case 86:pdest[85]=pp[psrc[874]];case 85:pdest[84]=pp[psrc[875]];case 84:pdest[83]=pp[psrc[876]];case 83:pdest[82]=pp[psrc[877]];case 82:pdest[81]=pp[psrc[878]];case 81:pdest[80]=pp[psrc[879]];case 80:pdest[79]=pp[psrc[880]];case 79:pdest[78]=pp[psrc[881]];case 78:pdest[77]=pp[psrc[882]];case 77:pdest[76]=pp[psrc[883]];case 76:pdest[75]=pp[psrc[884]];case 75:pdest[74]=pp[psrc[885]];case 74:pdest[73]=pp[psrc[886]];case 73:pdest[72]=pp[psrc[887]];case 72:pdest[71]=pp[psrc[888]];case 71:pdest[70]=pp[psrc[889]];case 70:pdest[69]=pp[psrc[890]];case 69:pdest[68]=pp[psrc[891]];case 68:pdest[67]=pp[psrc[892]];case 67:pdest[66]=pp[psrc[893]];case 66:pdest[65]=pp[psrc[894]];case 65:pdest[64]=pp[psrc[895]];case 64:pdest[63]=pp[psrc[896]];case 63:pdest[62]=pp[psrc[897]];case 62:pdest[61]=pp[psrc[898]];case 61:pdest[60]=pp[psrc[899]];case 60:pdest[59]=pp[psrc[900]];case 59:pdest[58]=pp[psrc[901]];case 58:pdest[57]=pp[psrc[902]];case 57:pdest[56]=pp[psrc[903]];case 56:pdest[55]=pp[psrc[904]];case 55:pdest[54]=pp[psrc[905]];case 54:pdest[53]=pp[psrc[906]];case 53:pdest[52]=pp[psrc[907]];case 52:pdest[51]=pp[psrc[908]];case 51:pdest[50]=pp[psrc[909]];case 50:pdest[49]=pp[psrc[910]];case 49:pdest[48]=pp[psrc[911]];case 48:pdest[47]=pp[psrc[912]];case 47:pdest[46]=pp[psrc[913]];case 46:pdest[45]=pp[psrc[914]];case 45:pdest[44]=pp[psrc[915]];case 44:pdest[43]=pp[psrc[916]];case 43:pdest[42]=pp[psrc[917]];case 42:pdest[41]=pp[psrc[918]];case 41:pdest[40]=pp[psrc[919]];case 40:pdest[39]=pp[psrc[920]];case 39:pdest[38]=pp[psrc[921]];case 38:pdest[37]=pp[psrc[922]];case 37:pdest[36]=pp[psrc[923]];case 36:pdest[35]=pp[psrc[924]];case 35:pdest[34]=pp[psrc[925]];case 34:pdest[33]=pp[psrc[926]];case 33:pdest[32]=pp[psrc[927]];case 32:pdest[31]=pp[psrc[928]];case 31:pdest[30]=pp[psrc[929]];case 30:pdest[29]=pp[psrc[930]];case 29:pdest[28]=pp[psrc[931]];case 28:pdest[27]=pp[psrc[932]];case 27:pdest[26]=pp[psrc[933]];case 26:pdest[25]=pp[psrc[934]];case 25:pdest[24]=pp[psrc[935]];case 24:pdest[23]=pp[psrc[936]];case 23:pdest[22]=pp[psrc[937]];case 22:pdest[21]=pp[psrc[938]];case 21:pdest[20]=pp[psrc[939]];case 20:pdest[19]=pp[psrc[940]];case 19:pdest[18]=pp[psrc[941]];case 18:pdest[17]=pp[psrc[942]];case 17:pdest[16]=pp[psrc[943]];case 16:pdest[15]=pp[psrc[944]];case 15:pdest[14]=pp[psrc[945]];case 14:pdest[13]=pp[psrc[946]];case 13:pdest[12]=pp[psrc[947]];case 12:pdest[11]=pp[psrc[948]];case 11:pdest[10]=pp[psrc[949]];case 10:pdest[9]=pp[psrc[950]];case 9:pdest[8]=pp[psrc[951]];case 8:pdest[7]=pp[psrc[952]];case 7:pdest[6]=pp[psrc[953]];case 6:pdest[5]=pp[psrc[954]];case 5:pdest[4]=pp[psrc[955]];case 4:pdest[3]=pp[psrc[956]];case 3:pdest[2]=pp[psrc[957]];case 2:pdest[1]=pp[psrc[958]];case 1:pdest[0]=pp[psrc[959]];
#define _pcpy case 960:pdest[959]=pp[psrc[959]];case 959:pdest[958]=pp[psrc[958]];case 958:pdest[957]=pp[psrc[957]];case 957:pdest[956]=pp[psrc[956]];case 956:pdest[955]=pp[psrc[955]];case 955:pdest[954]=pp[psrc[954]];case 954:pdest[953]=pp[psrc[953]];case 953:pdest[952]=pp[psrc[952]];case 952:pdest[951]=pp[psrc[951]];case 951:pdest[950]=pp[psrc[950]];case 950:pdest[949]=pp[psrc[949]];case 949:pdest[948]=pp[psrc[948]];case 948:pdest[947]=pp[psrc[947]];case 947:pdest[946]=pp[psrc[946]];case 946:pdest[945]=pp[psrc[945]];case 945:pdest[944]=pp[psrc[944]];case 944:pdest[943]=pp[psrc[943]];case 943:pdest[942]=pp[psrc[942]];case 942:pdest[941]=pp[psrc[941]];case 941:pdest[940]=pp[psrc[940]];case 940:pdest[939]=pp[psrc[939]];case 939:pdest[938]=pp[psrc[938]];case 938:pdest[937]=pp[psrc[937]];case 937:pdest[936]=pp[psrc[936]];case 936:pdest[935]=pp[psrc[935]];case 935:pdest[934]=pp[psrc[934]];case 934:pdest[933]=pp[psrc[933]];case 933:pdest[932]=pp[psrc[932]];case 932:pdest[931]=pp[psrc[931]];case 931:pdest[930]=pp[psrc[930]];case 930:pdest[929]=pp[psrc[929]];case 929:pdest[928]=pp[psrc[928]];case 928:pdest[927]=pp[psrc[927]];case 927:pdest[926]=pp[psrc[926]];case 926:pdest[925]=pp[psrc[925]];case 925:pdest[924]=pp[psrc[924]];case 924:pdest[923]=pp[psrc[923]];case 923:pdest[922]=pp[psrc[922]];case 922:pdest[921]=pp[psrc[921]];case 921:pdest[920]=pp[psrc[920]];case 920:pdest[919]=pp[psrc[919]];case 919:pdest[918]=pp[psrc[918]];case 918:pdest[917]=pp[psrc[917]];case 917:pdest[916]=pp[psrc[916]];case 916:pdest[915]=pp[psrc[915]];case 915:pdest[914]=pp[psrc[914]];case 914:pdest[913]=pp[psrc[913]];case 913:pdest[912]=pp[psrc[912]];case 912:pdest[911]=pp[psrc[911]];case 911:pdest[910]=pp[psrc[910]];case 910:pdest[909]=pp[psrc[909]];case 909:pdest[908]=pp[psrc[908]];case 908:pdest[907]=pp[psrc[907]];case 907:pdest[906]=pp[psrc[906]];case 906:pdest[905]=pp[psrc[905]];case 905:pdest[904]=pp[psrc[904]];case 904:pdest[903]=pp[psrc[903]];case 903:pdest[902]=pp[psrc[902]];case 902:pdest[901]=pp[psrc[901]];case 901:pdest[900]=pp[psrc[900]];case 900:pdest[899]=pp[psrc[899]];case 899:pdest[898]=pp[psrc[898]];case 898:pdest[897]=pp[psrc[897]];case 897:pdest[896]=pp[psrc[896]];case 896:pdest[895]=pp[psrc[895]];case 895:pdest[894]=pp[psrc[894]];case 894:pdest[893]=pp[psrc[893]];case 893:pdest[892]=pp[psrc[892]];case 892:pdest[891]=pp[psrc[891]];case 891:pdest[890]=pp[psrc[890]];case 890:pdest[889]=pp[psrc[889]];case 889:pdest[888]=pp[psrc[888]];case 888:pdest[887]=pp[psrc[887]];case 887:pdest[886]=pp[psrc[886]];case 886:pdest[885]=pp[psrc[885]];case 885:pdest[884]=pp[psrc[884]];case 884:pdest[883]=pp[psrc[883]];case 883:pdest[882]=pp[psrc[882]];case 882:pdest[881]=pp[psrc[881]];case 881:pdest[880]=pp[psrc[880]];case 880:pdest[879]=pp[psrc[879]];case 879:pdest[878]=pp[psrc[878]];case 878:pdest[877]=pp[psrc[877]];case 877:pdest[876]=pp[psrc[876]];case 876:pdest[875]=pp[psrc[875]];case 875:pdest[874]=pp[psrc[874]];case 874:pdest[873]=pp[psrc[873]];case 873:pdest[872]=pp[psrc[872]];case 872:pdest[871]=pp[psrc[871]];case 871:pdest[870]=pp[psrc[870]];case 870:pdest[869]=pp[psrc[869]];case 869:pdest[868]=pp[psrc[868]];case 868:pdest[867]=pp[psrc[867]];case 867:pdest[866]=pp[psrc[866]];case 866:pdest[865]=pp[psrc[865]];case 865:pdest[864]=pp[psrc[864]];case 864:pdest[863]=pp[psrc[863]];case 863:pdest[862]=pp[psrc[862]];case 862:pdest[861]=pp[psrc[861]];case 861:pdest[860]=pp[psrc[860]];case 860:pdest[859]=pp[psrc[859]];case 859:pdest[858]=pp[psrc[858]];case 858:pdest[857]=pp[psrc[857]];case 857:pdest[856]=pp[psrc[856]];case 856:pdest[855]=pp[psrc[855]];case 855:pdest[854]=pp[psrc[854]];case 854:pdest[853]=pp[psrc[853]];case 853:pdest[852]=pp[psrc[852]];case 852:pdest[851]=pp[psrc[851]];case 851:pdest[850]=pp[psrc[850]];case 850:pdest[849]=pp[psrc[849]];case 849:pdest[848]=pp[psrc[848]];case 848:pdest[847]=pp[psrc[847]];case 847:pdest[846]=pp[psrc[846]];case 846:pdest[845]=pp[psrc[845]];case 845:pdest[844]=pp[psrc[844]];case 844:pdest[843]=pp[psrc[843]];case 843:pdest[842]=pp[psrc[842]];case 842:pdest[841]=pp[psrc[841]];case 841:pdest[840]=pp[psrc[840]];case 840:pdest[839]=pp[psrc[839]];case 839:pdest[838]=pp[psrc[838]];case 838:pdest[837]=pp[psrc[837]];case 837:pdest[836]=pp[psrc[836]];case 836:pdest[835]=pp[psrc[835]];case 835:pdest[834]=pp[psrc[834]];case 834:pdest[833]=pp[psrc[833]];case 833:pdest[832]=pp[psrc[832]];case 832:pdest[831]=pp[psrc[831]];case 831:pdest[830]=pp[psrc[830]];case 830:pdest[829]=pp[psrc[829]];case 829:pdest[828]=pp[psrc[828]];case 828:pdest[827]=pp[psrc[827]];case 827:pdest[826]=pp[psrc[826]];case 826:pdest[825]=pp[psrc[825]];case 825:pdest[824]=pp[psrc[824]];case 824:pdest[823]=pp[psrc[823]];case 823:pdest[822]=pp[psrc[822]];case 822:pdest[821]=pp[psrc[821]];case 821:pdest[820]=pp[psrc[820]];case 820:pdest[819]=pp[psrc[819]];case 819:pdest[818]=pp[psrc[818]];case 818:pdest[817]=pp[psrc[817]];case 817:pdest[816]=pp[psrc[816]];case 816:pdest[815]=pp[psrc[815]];case 815:pdest[814]=pp[psrc[814]];case 814:pdest[813]=pp[psrc[813]];case 813:pdest[812]=pp[psrc[812]];case 812:pdest[811]=pp[psrc[811]];case 811:pdest[810]=pp[psrc[810]];case 810:pdest[809]=pp[psrc[809]];case 809:pdest[808]=pp[psrc[808]];case 808:pdest[807]=pp[psrc[807]];case 807:pdest[806]=pp[psrc[806]];case 806:pdest[805]=pp[psrc[805]];case 805:pdest[804]=pp[psrc[804]];case 804:pdest[803]=pp[psrc[803]];case 803:pdest[802]=pp[psrc[802]];case 802:pdest[801]=pp[psrc[801]];case 801:pdest[800]=pp[psrc[800]];case 800:pdest[799]=pp[psrc[799]];case 799:pdest[798]=pp[psrc[798]];case 798:pdest[797]=pp[psrc[797]];case 797:pdest[796]=pp[psrc[796]];case 796:pdest[795]=pp[psrc[795]];case 795:pdest[794]=pp[psrc[794]];case 794:pdest[793]=pp[psrc[793]];case 793:pdest[792]=pp[psrc[792]];case 792:pdest[791]=pp[psrc[791]];case 791:pdest[790]=pp[psrc[790]];case 790:pdest[789]=pp[psrc[789]];case 789:pdest[788]=pp[psrc[788]];case 788:pdest[787]=pp[psrc[787]];case 787:pdest[786]=pp[psrc[786]];case 786:pdest[785]=pp[psrc[785]];case 785:pdest[784]=pp[psrc[784]];case 784:pdest[783]=pp[psrc[783]];case 783:pdest[782]=pp[psrc[782]];case 782:pdest[781]=pp[psrc[781]];case 781:pdest[780]=pp[psrc[780]];case 780:pdest[779]=pp[psrc[779]];case 779:pdest[778]=pp[psrc[778]];case 778:pdest[777]=pp[psrc[777]];case 777:pdest[776]=pp[psrc[776]];case 776:pdest[775]=pp[psrc[775]];case 775:pdest[774]=pp[psrc[774]];case 774:pdest[773]=pp[psrc[773]];case 773:pdest[772]=pp[psrc[772]];case 772:pdest[771]=pp[psrc[771]];case 771:pdest[770]=pp[psrc[770]];case 770:pdest[769]=pp[psrc[769]];case 769:pdest[768]=pp[psrc[768]];case 768:pdest[767]=pp[psrc[767]];case 767:pdest[766]=pp[psrc[766]];case 766:pdest[765]=pp[psrc[765]];case 765:pdest[764]=pp[psrc[764]];case 764:pdest[763]=pp[psrc[763]];case 763:pdest[762]=pp[psrc[762]];case 762:pdest[761]=pp[psrc[761]];case 761:pdest[760]=pp[psrc[760]];case 760:pdest[759]=pp[psrc[759]];case 759:pdest[758]=pp[psrc[758]];case 758:pdest[757]=pp[psrc[757]];case 757:pdest[756]=pp[psrc[756]];case 756:pdest[755]=pp[psrc[755]];case 755:pdest[754]=pp[psrc[754]];case 754:pdest[753]=pp[psrc[753]];case 753:pdest[752]=pp[psrc[752]];case 752:pdest[751]=pp[psrc[751]];case 751:pdest[750]=pp[psrc[750]];case 750:pdest[749]=pp[psrc[749]];case 749:pdest[748]=pp[psrc[748]];case 748:pdest[747]=pp[psrc[747]];case 747:pdest[746]=pp[psrc[746]];case 746:pdest[745]=pp[psrc[745]];case 745:pdest[744]=pp[psrc[744]];case 744:pdest[743]=pp[psrc[743]];case 743:pdest[742]=pp[psrc[742]];case 742:pdest[741]=pp[psrc[741]];case 741:pdest[740]=pp[psrc[740]];case 740:pdest[739]=pp[psrc[739]];case 739:pdest[738]=pp[psrc[738]];case 738:pdest[737]=pp[psrc[737]];case 737:pdest[736]=pp[psrc[736]];case 736:pdest[735]=pp[psrc[735]];case 735:pdest[734]=pp[psrc[734]];case 734:pdest[733]=pp[psrc[733]];case 733:pdest[732]=pp[psrc[732]];case 732:pdest[731]=pp[psrc[731]];case 731:pdest[730]=pp[psrc[730]];case 730:pdest[729]=pp[psrc[729]];case 729:pdest[728]=pp[psrc[728]];case 728:pdest[727]=pp[psrc[727]];case 727:pdest[726]=pp[psrc[726]];case 726:pdest[725]=pp[psrc[725]];case 725:pdest[724]=pp[psrc[724]];case 724:pdest[723]=pp[psrc[723]];case 723:pdest[722]=pp[psrc[722]];case 722:pdest[721]=pp[psrc[721]];case 721:pdest[720]=pp[psrc[720]];case 720:pdest[719]=pp[psrc[719]];case 719:pdest[718]=pp[psrc[718]];case 718:pdest[717]=pp[psrc[717]];case 717:pdest[716]=pp[psrc[716]];case 716:pdest[715]=pp[psrc[715]];case 715:pdest[714]=pp[psrc[714]];case 714:pdest[713]=pp[psrc[713]];case 713:pdest[712]=pp[psrc[712]];case 712:pdest[711]=pp[psrc[711]];case 711:pdest[710]=pp[psrc[710]];case 710:pdest[709]=pp[psrc[709]];case 709:pdest[708]=pp[psrc[708]];case 708:pdest[707]=pp[psrc[707]];case 707:pdest[706]=pp[psrc[706]];case 706:pdest[705]=pp[psrc[705]];case 705:pdest[704]=pp[psrc[704]];case 704:pdest[703]=pp[psrc[703]];case 703:pdest[702]=pp[psrc[702]];case 702:pdest[701]=pp[psrc[701]];case 701:pdest[700]=pp[psrc[700]];case 700:pdest[699]=pp[psrc[699]];case 699:pdest[698]=pp[psrc[698]];case 698:pdest[697]=pp[psrc[697]];case 697:pdest[696]=pp[psrc[696]];case 696:pdest[695]=pp[psrc[695]];case 695:pdest[694]=pp[psrc[694]];case 694:pdest[693]=pp[psrc[693]];case 693:pdest[692]=pp[psrc[692]];case 692:pdest[691]=pp[psrc[691]];case 691:pdest[690]=pp[psrc[690]];case 690:pdest[689]=pp[psrc[689]];case 689:pdest[688]=pp[psrc[688]];case 688:pdest[687]=pp[psrc[687]];case 687:pdest[686]=pp[psrc[686]];case 686:pdest[685]=pp[psrc[685]];case 685:pdest[684]=pp[psrc[684]];case 684:pdest[683]=pp[psrc[683]];case 683:pdest[682]=pp[psrc[682]];case 682:pdest[681]=pp[psrc[681]];case 681:pdest[680]=pp[psrc[680]];case 680:pdest[679]=pp[psrc[679]];case 679:pdest[678]=pp[psrc[678]];case 678:pdest[677]=pp[psrc[677]];case 677:pdest[676]=pp[psrc[676]];case 676:pdest[675]=pp[psrc[675]];case 675:pdest[674]=pp[psrc[674]];case 674:pdest[673]=pp[psrc[673]];case 673:pdest[672]=pp[psrc[672]];case 672:pdest[671]=pp[psrc[671]];case 671:pdest[670]=pp[psrc[670]];case 670:pdest[669]=pp[psrc[669]];case 669:pdest[668]=pp[psrc[668]];case 668:pdest[667]=pp[psrc[667]];case 667:pdest[666]=pp[psrc[666]];case 666:pdest[665]=pp[psrc[665]];case 665:pdest[664]=pp[psrc[664]];case 664:pdest[663]=pp[psrc[663]];case 663:pdest[662]=pp[psrc[662]];case 662:pdest[661]=pp[psrc[661]];case 661:pdest[660]=pp[psrc[660]];case 660:pdest[659]=pp[psrc[659]];case 659:pdest[658]=pp[psrc[658]];case 658:pdest[657]=pp[psrc[657]];case 657:pdest[656]=pp[psrc[656]];case 656:pdest[655]=pp[psrc[655]];case 655:pdest[654]=pp[psrc[654]];case 654:pdest[653]=pp[psrc[653]];case 653:pdest[652]=pp[psrc[652]];case 652:pdest[651]=pp[psrc[651]];case 651:pdest[650]=pp[psrc[650]];case 650:pdest[649]=pp[psrc[649]];case 649:pdest[648]=pp[psrc[648]];case 648:pdest[647]=pp[psrc[647]];case 647:pdest[646]=pp[psrc[646]];case 646:pdest[645]=pp[psrc[645]];case 645:pdest[644]=pp[psrc[644]];case 644:pdest[643]=pp[psrc[643]];case 643:pdest[642]=pp[psrc[642]];case 642:pdest[641]=pp[psrc[641]];case 641:pdest[640]=pp[psrc[640]];case 640:pdest[639]=pp[psrc[639]];case 639:pdest[638]=pp[psrc[638]];case 638:pdest[637]=pp[psrc[637]];case 637:pdest[636]=pp[psrc[636]];case 636:pdest[635]=pp[psrc[635]];case 635:pdest[634]=pp[psrc[634]];case 634:pdest[633]=pp[psrc[633]];case 633:pdest[632]=pp[psrc[632]];case 632:pdest[631]=pp[psrc[631]];case 631:pdest[630]=pp[psrc[630]];case 630:pdest[629]=pp[psrc[629]];case 629:pdest[628]=pp[psrc[628]];case 628:pdest[627]=pp[psrc[627]];case 627:pdest[626]=pp[psrc[626]];case 626:pdest[625]=pp[psrc[625]];case 625:pdest[624]=pp[psrc[624]];case 624:pdest[623]=pp[psrc[623]];case 623:pdest[622]=pp[psrc[622]];case 622:pdest[621]=pp[psrc[621]];case 621:pdest[620]=pp[psrc[620]];case 620:pdest[619]=pp[psrc[619]];case 619:pdest[618]=pp[psrc[618]];case 618:pdest[617]=pp[psrc[617]];case 617:pdest[616]=pp[psrc[616]];case 616:pdest[615]=pp[psrc[615]];case 615:pdest[614]=pp[psrc[614]];case 614:pdest[613]=pp[psrc[613]];case 613:pdest[612]=pp[psrc[612]];case 612:pdest[611]=pp[psrc[611]];case 611:pdest[610]=pp[psrc[610]];case 610:pdest[609]=pp[psrc[609]];case 609:pdest[608]=pp[psrc[608]];case 608:pdest[607]=pp[psrc[607]];case 607:pdest[606]=pp[psrc[606]];case 606:pdest[605]=pp[psrc[605]];case 605:pdest[604]=pp[psrc[604]];case 604:pdest[603]=pp[psrc[603]];case 603:pdest[602]=pp[psrc[602]];case 602:pdest[601]=pp[psrc[601]];case 601:pdest[600]=pp[psrc[600]];case 600:pdest[599]=pp[psrc[599]];case 599:pdest[598]=pp[psrc[598]];case 598:pdest[597]=pp[psrc[597]];case 597:pdest[596]=pp[psrc[596]];case 596:pdest[595]=pp[psrc[595]];case 595:pdest[594]=pp[psrc[594]];case 594:pdest[593]=pp[psrc[593]];case 593:pdest[592]=pp[psrc[592]];case 592:pdest[591]=pp[psrc[591]];case 591:pdest[590]=pp[psrc[590]];case 590:pdest[589]=pp[psrc[589]];case 589:pdest[588]=pp[psrc[588]];case 588:pdest[587]=pp[psrc[587]];case 587:pdest[586]=pp[psrc[586]];case 586:pdest[585]=pp[psrc[585]];case 585:pdest[584]=pp[psrc[584]];case 584:pdest[583]=pp[psrc[583]];case 583:pdest[582]=pp[psrc[582]];case 582:pdest[581]=pp[psrc[581]];case 581:pdest[580]=pp[psrc[580]];case 580:pdest[579]=pp[psrc[579]];case 579:pdest[578]=pp[psrc[578]];case 578:pdest[577]=pp[psrc[577]];case 577:pdest[576]=pp[psrc[576]];case 576:pdest[575]=pp[psrc[575]];case 575:pdest[574]=pp[psrc[574]];case 574:pdest[573]=pp[psrc[573]];case 573:pdest[572]=pp[psrc[572]];case 572:pdest[571]=pp[psrc[571]];case 571:pdest[570]=pp[psrc[570]];case 570:pdest[569]=pp[psrc[569]];case 569:pdest[568]=pp[psrc[568]];case 568:pdest[567]=pp[psrc[567]];case 567:pdest[566]=pp[psrc[566]];case 566:pdest[565]=pp[psrc[565]];case 565:pdest[564]=pp[psrc[564]];case 564:pdest[563]=pp[psrc[563]];case 563:pdest[562]=pp[psrc[562]];case 562:pdest[561]=pp[psrc[561]];case 561:pdest[560]=pp[psrc[560]];case 560:pdest[559]=pp[psrc[559]];case 559:pdest[558]=pp[psrc[558]];case 558:pdest[557]=pp[psrc[557]];case 557:pdest[556]=pp[psrc[556]];case 556:pdest[555]=pp[psrc[555]];case 555:pdest[554]=pp[psrc[554]];case 554:pdest[553]=pp[psrc[553]];case 553:pdest[552]=pp[psrc[552]];case 552:pdest[551]=pp[psrc[551]];case 551:pdest[550]=pp[psrc[550]];case 550:pdest[549]=pp[psrc[549]];case 549:pdest[548]=pp[psrc[548]];case 548:pdest[547]=pp[psrc[547]];case 547:pdest[546]=pp[psrc[546]];case 546:pdest[545]=pp[psrc[545]];case 545:pdest[544]=pp[psrc[544]];case 544:pdest[543]=pp[psrc[543]];case 543:pdest[542]=pp[psrc[542]];case 542:pdest[541]=pp[psrc[541]];case 541:pdest[540]=pp[psrc[540]];case 540:pdest[539]=pp[psrc[539]];case 539:pdest[538]=pp[psrc[538]];case 538:pdest[537]=pp[psrc[537]];case 537:pdest[536]=pp[psrc[536]];case 536:pdest[535]=pp[psrc[535]];case 535:pdest[534]=pp[psrc[534]];case 534:pdest[533]=pp[psrc[533]];case 533:pdest[532]=pp[psrc[532]];case 532:pdest[531]=pp[psrc[531]];case 531:pdest[530]=pp[psrc[530]];case 530:pdest[529]=pp[psrc[529]];case 529:pdest[528]=pp[psrc[528]];case 528:pdest[527]=pp[psrc[527]];case 527:pdest[526]=pp[psrc[526]];case 526:pdest[525]=pp[psrc[525]];case 525:pdest[524]=pp[psrc[524]];case 524:pdest[523]=pp[psrc[523]];case 523:pdest[522]=pp[psrc[522]];case 522:pdest[521]=pp[psrc[521]];case 521:pdest[520]=pp[psrc[520]];case 520:pdest[519]=pp[psrc[519]];case 519:pdest[518]=pp[psrc[518]];case 518:pdest[517]=pp[psrc[517]];case 517:pdest[516]=pp[psrc[516]];case 516:pdest[515]=pp[psrc[515]];case 515:pdest[514]=pp[psrc[514]];case 514:pdest[513]=pp[psrc[513]];case 513:pdest[512]=pp[psrc[512]];case 512:pdest[511]=pp[psrc[511]];case 511:pdest[510]=pp[psrc[510]];case 510:pdest[509]=pp[psrc[509]];case 509:pdest[508]=pp[psrc[508]];case 508:pdest[507]=pp[psrc[507]];case 507:pdest[506]=pp[psrc[506]];case 506:pdest[505]=pp[psrc[505]];case 505:pdest[504]=pp[psrc[504]];case 504:pdest[503]=pp[psrc[503]];case 503:pdest[502]=pp[psrc[502]];case 502:pdest[501]=pp[psrc[501]];case 501:pdest[500]=pp[psrc[500]];case 500:pdest[499]=pp[psrc[499]];case 499:pdest[498]=pp[psrc[498]];case 498:pdest[497]=pp[psrc[497]];case 497:pdest[496]=pp[psrc[496]];case 496:pdest[495]=pp[psrc[495]];case 495:pdest[494]=pp[psrc[494]];case 494:pdest[493]=pp[psrc[493]];case 493:pdest[492]=pp[psrc[492]];case 492:pdest[491]=pp[psrc[491]];case 491:pdest[490]=pp[psrc[490]];case 490:pdest[489]=pp[psrc[489]];case 489:pdest[488]=pp[psrc[488]];case 488:pdest[487]=pp[psrc[487]];case 487:pdest[486]=pp[psrc[486]];case 486:pdest[485]=pp[psrc[485]];case 485:pdest[484]=pp[psrc[484]];case 484:pdest[483]=pp[psrc[483]];case 483:pdest[482]=pp[psrc[482]];case 482:pdest[481]=pp[psrc[481]];case 481:pdest[480]=pp[psrc[480]];case 480:pdest[479]=pp[psrc[479]];case 479:pdest[478]=pp[psrc[478]];case 478:pdest[477]=pp[psrc[477]];case 477:pdest[476]=pp[psrc[476]];case 476:pdest[475]=pp[psrc[475]];case 475:pdest[474]=pp[psrc[474]];case 474:pdest[473]=pp[psrc[473]];case 473:pdest[472]=pp[psrc[472]];case 472:pdest[471]=pp[psrc[471]];case 471:pdest[470]=pp[psrc[470]];case 470:pdest[469]=pp[psrc[469]];case 469:pdest[468]=pp[psrc[468]];case 468:pdest[467]=pp[psrc[467]];case 467:pdest[466]=pp[psrc[466]];case 466:pdest[465]=pp[psrc[465]];case 465:pdest[464]=pp[psrc[464]];case 464:pdest[463]=pp[psrc[463]];case 463:pdest[462]=pp[psrc[462]];case 462:pdest[461]=pp[psrc[461]];case 461:pdest[460]=pp[psrc[460]];case 460:pdest[459]=pp[psrc[459]];case 459:pdest[458]=pp[psrc[458]];case 458:pdest[457]=pp[psrc[457]];case 457:pdest[456]=pp[psrc[456]];case 456:pdest[455]=pp[psrc[455]];case 455:pdest[454]=pp[psrc[454]];case 454:pdest[453]=pp[psrc[453]];case 453:pdest[452]=pp[psrc[452]];case 452:pdest[451]=pp[psrc[451]];case 451:pdest[450]=pp[psrc[450]];case 450:pdest[449]=pp[psrc[449]];case 449:pdest[448]=pp[psrc[448]];case 448:pdest[447]=pp[psrc[447]];case 447:pdest[446]=pp[psrc[446]];case 446:pdest[445]=pp[psrc[445]];case 445:pdest[444]=pp[psrc[444]];case 444:pdest[443]=pp[psrc[443]];case 443:pdest[442]=pp[psrc[442]];case 442:pdest[441]=pp[psrc[441]];case 441:pdest[440]=pp[psrc[440]];case 440:pdest[439]=pp[psrc[439]];case 439:pdest[438]=pp[psrc[438]];case 438:pdest[437]=pp[psrc[437]];case 437:pdest[436]=pp[psrc[436]];case 436:pdest[435]=pp[psrc[435]];case 435:pdest[434]=pp[psrc[434]];case 434:pdest[433]=pp[psrc[433]];case 433:pdest[432]=pp[psrc[432]];case 432:pdest[431]=pp[psrc[431]];case 431:pdest[430]=pp[psrc[430]];case 430:pdest[429]=pp[psrc[429]];case 429:pdest[428]=pp[psrc[428]];case 428:pdest[427]=pp[psrc[427]];case 427:pdest[426]=pp[psrc[426]];case 426:pdest[425]=pp[psrc[425]];case 425:pdest[424]=pp[psrc[424]];case 424:pdest[423]=pp[psrc[423]];case 423:pdest[422]=pp[psrc[422]];case 422:pdest[421]=pp[psrc[421]];case 421:pdest[420]=pp[psrc[420]];case 420:pdest[419]=pp[psrc[419]];case 419:pdest[418]=pp[psrc[418]];case 418:pdest[417]=pp[psrc[417]];case 417:pdest[416]=pp[psrc[416]];case 416:pdest[415]=pp[psrc[415]];case 415:pdest[414]=pp[psrc[414]];case 414:pdest[413]=pp[psrc[413]];case 413:pdest[412]=pp[psrc[412]];case 412:pdest[411]=pp[psrc[411]];case 411:pdest[410]=pp[psrc[410]];case 410:pdest[409]=pp[psrc[409]];case 409:pdest[408]=pp[psrc[408]];case 408:pdest[407]=pp[psrc[407]];case 407:pdest[406]=pp[psrc[406]];case 406:pdest[405]=pp[psrc[405]];case 405:pdest[404]=pp[psrc[404]];case 404:pdest[403]=pp[psrc[403]];case 403:pdest[402]=pp[psrc[402]];case 402:pdest[401]=pp[psrc[401]];case 401:pdest[400]=pp[psrc[400]];case 400:pdest[399]=pp[psrc[399]];case 399:pdest[398]=pp[psrc[398]];case 398:pdest[397]=pp[psrc[397]];case 397:pdest[396]=pp[psrc[396]];case 396:pdest[395]=pp[psrc[395]];case 395:pdest[394]=pp[psrc[394]];case 394:pdest[393]=pp[psrc[393]];case 393:pdest[392]=pp[psrc[392]];case 392:pdest[391]=pp[psrc[391]];case 391:pdest[390]=pp[psrc[390]];case 390:pdest[389]=pp[psrc[389]];case 389:pdest[388]=pp[psrc[388]];case 388:pdest[387]=pp[psrc[387]];case 387:pdest[386]=pp[psrc[386]];case 386:pdest[385]=pp[psrc[385]];case 385:pdest[384]=pp[psrc[384]];case 384:pdest[383]=pp[psrc[383]];case 383:pdest[382]=pp[psrc[382]];case 382:pdest[381]=pp[psrc[381]];case 381:pdest[380]=pp[psrc[380]];case 380:pdest[379]=pp[psrc[379]];case 379:pdest[378]=pp[psrc[378]];case 378:pdest[377]=pp[psrc[377]];case 377:pdest[376]=pp[psrc[376]];case 376:pdest[375]=pp[psrc[375]];case 375:pdest[374]=pp[psrc[374]];case 374:pdest[373]=pp[psrc[373]];case 373:pdest[372]=pp[psrc[372]];case 372:pdest[371]=pp[psrc[371]];case 371:pdest[370]=pp[psrc[370]];case 370:pdest[369]=pp[psrc[369]];case 369:pdest[368]=pp[psrc[368]];case 368:pdest[367]=pp[psrc[367]];case 367:pdest[366]=pp[psrc[366]];case 366:pdest[365]=pp[psrc[365]];case 365:pdest[364]=pp[psrc[364]];case 364:pdest[363]=pp[psrc[363]];case 363:pdest[362]=pp[psrc[362]];case 362:pdest[361]=pp[psrc[361]];case 361:pdest[360]=pp[psrc[360]];case 360:pdest[359]=pp[psrc[359]];case 359:pdest[358]=pp[psrc[358]];case 358:pdest[357]=pp[psrc[357]];case 357:pdest[356]=pp[psrc[356]];case 356:pdest[355]=pp[psrc[355]];case 355:pdest[354]=pp[psrc[354]];case 354:pdest[353]=pp[psrc[353]];case 353:pdest[352]=pp[psrc[352]];case 352:pdest[351]=pp[psrc[351]];case 351:pdest[350]=pp[psrc[350]];case 350:pdest[349]=pp[psrc[349]];case 349:pdest[348]=pp[psrc[348]];case 348:pdest[347]=pp[psrc[347]];case 347:pdest[346]=pp[psrc[346]];case 346:pdest[345]=pp[psrc[345]];case 345:pdest[344]=pp[psrc[344]];case 344:pdest[343]=pp[psrc[343]];case 343:pdest[342]=pp[psrc[342]];case 342:pdest[341]=pp[psrc[341]];case 341:pdest[340]=pp[psrc[340]];case 340:pdest[339]=pp[psrc[339]];case 339:pdest[338]=pp[psrc[338]];case 338:pdest[337]=pp[psrc[337]];case 337:pdest[336]=pp[psrc[336]];case 336:pdest[335]=pp[psrc[335]];case 335:pdest[334]=pp[psrc[334]];case 334:pdest[333]=pp[psrc[333]];case 333:pdest[332]=pp[psrc[332]];case 332:pdest[331]=pp[psrc[331]];case 331:pdest[330]=pp[psrc[330]];case 330:pdest[329]=pp[psrc[329]];case 329:pdest[328]=pp[psrc[328]];case 328:pdest[327]=pp[psrc[327]];case 327:pdest[326]=pp[psrc[326]];case 326:pdest[325]=pp[psrc[325]];case 325:pdest[324]=pp[psrc[324]];case 324:pdest[323]=pp[psrc[323]];case 323:pdest[322]=pp[psrc[322]];case 322:pdest[321]=pp[psrc[321]];case 321:pdest[320]=pp[psrc[320]];case 320:pdest[319]=pp[psrc[319]];case 319:pdest[318]=pp[psrc[318]];case 318:pdest[317]=pp[psrc[317]];case 317:pdest[316]=pp[psrc[316]];case 316:pdest[315]=pp[psrc[315]];case 315:pdest[314]=pp[psrc[314]];case 314:pdest[313]=pp[psrc[313]];case 313:pdest[312]=pp[psrc[312]];case 312:pdest[311]=pp[psrc[311]];case 311:pdest[310]=pp[psrc[310]];case 310:pdest[309]=pp[psrc[309]];case 309:pdest[308]=pp[psrc[308]];case 308:pdest[307]=pp[psrc[307]];case 307:pdest[306]=pp[psrc[306]];case 306:pdest[305]=pp[psrc[305]];case 305:pdest[304]=pp[psrc[304]];case 304:pdest[303]=pp[psrc[303]];case 303:pdest[302]=pp[psrc[302]];case 302:pdest[301]=pp[psrc[301]];case 301:pdest[300]=pp[psrc[300]];case 300:pdest[299]=pp[psrc[299]];case 299:pdest[298]=pp[psrc[298]];case 298:pdest[297]=pp[psrc[297]];case 297:pdest[296]=pp[psrc[296]];case 296:pdest[295]=pp[psrc[295]];case 295:pdest[294]=pp[psrc[294]];case 294:pdest[293]=pp[psrc[293]];case 293:pdest[292]=pp[psrc[292]];case 292:pdest[291]=pp[psrc[291]];case 291:pdest[290]=pp[psrc[290]];case 290:pdest[289]=pp[psrc[289]];case 289:pdest[288]=pp[psrc[288]];case 288:pdest[287]=pp[psrc[287]];case 287:pdest[286]=pp[psrc[286]];case 286:pdest[285]=pp[psrc[285]];case 285:pdest[284]=pp[psrc[284]];case 284:pdest[283]=pp[psrc[283]];case 283:pdest[282]=pp[psrc[282]];case 282:pdest[281]=pp[psrc[281]];case 281:pdest[280]=pp[psrc[280]];case 280:pdest[279]=pp[psrc[279]];case 279:pdest[278]=pp[psrc[278]];case 278:pdest[277]=pp[psrc[277]];case 277:pdest[276]=pp[psrc[276]];case 276:pdest[275]=pp[psrc[275]];case 275:pdest[274]=pp[psrc[274]];case 274:pdest[273]=pp[psrc[273]];case 273:pdest[272]=pp[psrc[272]];case 272:pdest[271]=pp[psrc[271]];case 271:pdest[270]=pp[psrc[270]];case 270:pdest[269]=pp[psrc[269]];case 269:pdest[268]=pp[psrc[268]];case 268:pdest[267]=pp[psrc[267]];case 267:pdest[266]=pp[psrc[266]];case 266:pdest[265]=pp[psrc[265]];case 265:pdest[264]=pp[psrc[264]];case 264:pdest[263]=pp[psrc[263]];case 263:pdest[262]=pp[psrc[262]];case 262:pdest[261]=pp[psrc[261]];case 261:pdest[260]=pp[psrc[260]];case 260:pdest[259]=pp[psrc[259]];case 259:pdest[258]=pp[psrc[258]];case 258:pdest[257]=pp[psrc[257]];case 257:pdest[256]=pp[psrc[256]];case 256:pdest[255]=pp[psrc[255]];case 255:pdest[254]=pp[psrc[254]];case 254:pdest[253]=pp[psrc[253]];case 253:pdest[252]=pp[psrc[252]];case 252:pdest[251]=pp[psrc[251]];case 251:pdest[250]=pp[psrc[250]];case 250:pdest[249]=pp[psrc[249]];case 249:pdest[248]=pp[psrc[248]];case 248:pdest[247]=pp[psrc[247]];case 247:pdest[246]=pp[psrc[246]];case 246:pdest[245]=pp[psrc[245]];case 245:pdest[244]=pp[psrc[244]];case 244:pdest[243]=pp[psrc[243]];case 243:pdest[242]=pp[psrc[242]];case 242:pdest[241]=pp[psrc[241]];case 241:pdest[240]=pp[psrc[240]];case 240:pdest[239]=pp[psrc[239]];case 239:pdest[238]=pp[psrc[238]];case 238:pdest[237]=pp[psrc[237]];case 237:pdest[236]=pp[psrc[236]];case 236:pdest[235]=pp[psrc[235]];case 235:pdest[234]=pp[psrc[234]];case 234:pdest[233]=pp[psrc[233]];case 233:pdest[232]=pp[psrc[232]];case 232:pdest[231]=pp[psrc[231]];case 231:pdest[230]=pp[psrc[230]];case 230:pdest[229]=pp[psrc[229]];case 229:pdest[228]=pp[psrc[228]];case 228:pdest[227]=pp[psrc[227]];case 227:pdest[226]=pp[psrc[226]];case 226:pdest[225]=pp[psrc[225]];case 225:pdest[224]=pp[psrc[224]];case 224:pdest[223]=pp[psrc[223]];case 223:pdest[222]=pp[psrc[222]];case 222:pdest[221]=pp[psrc[221]];case 221:pdest[220]=pp[psrc[220]];case 220:pdest[219]=pp[psrc[219]];case 219:pdest[218]=pp[psrc[218]];case 218:pdest[217]=pp[psrc[217]];case 217:pdest[216]=pp[psrc[216]];case 216:pdest[215]=pp[psrc[215]];case 215:pdest[214]=pp[psrc[214]];case 214:pdest[213]=pp[psrc[213]];case 213:pdest[212]=pp[psrc[212]];case 212:pdest[211]=pp[psrc[211]];case 211:pdest[210]=pp[psrc[210]];case 210:pdest[209]=pp[psrc[209]];case 209:pdest[208]=pp[psrc[208]];case 208:pdest[207]=pp[psrc[207]];case 207:pdest[206]=pp[psrc[206]];case 206:pdest[205]=pp[psrc[205]];case 205:pdest[204]=pp[psrc[204]];case 204:pdest[203]=pp[psrc[203]];case 203:pdest[202]=pp[psrc[202]];case 202:pdest[201]=pp[psrc[201]];case 201:pdest[200]=pp[psrc[200]];case 200:pdest[199]=pp[psrc[199]];case 199:pdest[198]=pp[psrc[198]];case 198:pdest[197]=pp[psrc[197]];case 197:pdest[196]=pp[psrc[196]];case 196:pdest[195]=pp[psrc[195]];case 195:pdest[194]=pp[psrc[194]];case 194:pdest[193]=pp[psrc[193]];case 193:pdest[192]=pp[psrc[192]];case 192:pdest[191]=pp[psrc[191]];case 191:pdest[190]=pp[psrc[190]];case 190:pdest[189]=pp[psrc[189]];case 189:pdest[188]=pp[psrc[188]];case 188:pdest[187]=pp[psrc[187]];case 187:pdest[186]=pp[psrc[186]];case 186:pdest[185]=pp[psrc[185]];case 185:pdest[184]=pp[psrc[184]];case 184:pdest[183]=pp[psrc[183]];case 183:pdest[182]=pp[psrc[182]];case 182:pdest[181]=pp[psrc[181]];case 181:pdest[180]=pp[psrc[180]];case 180:pdest[179]=pp[psrc[179]];case 179:pdest[178]=pp[psrc[178]];case 178:pdest[177]=pp[psrc[177]];case 177:pdest[176]=pp[psrc[176]];case 176:pdest[175]=pp[psrc[175]];case 175:pdest[174]=pp[psrc[174]];case 174:pdest[173]=pp[psrc[173]];case 173:pdest[172]=pp[psrc[172]];case 172:pdest[171]=pp[psrc[171]];case 171:pdest[170]=pp[psrc[170]];case 170:pdest[169]=pp[psrc[169]];case 169:pdest[168]=pp[psrc[168]];case 168:pdest[167]=pp[psrc[167]];case 167:pdest[166]=pp[psrc[166]];case 166:pdest[165]=pp[psrc[165]];case 165:pdest[164]=pp[psrc[164]];case 164:pdest[163]=pp[psrc[163]];case 163:pdest[162]=pp[psrc[162]];case 162:pdest[161]=pp[psrc[161]];case 161:pdest[160]=pp[psrc[160]];case 160:pdest[159]=pp[psrc[159]];case 159:pdest[158]=pp[psrc[158]];case 158:pdest[157]=pp[psrc[157]];case 157:pdest[156]=pp[psrc[156]];case 156:pdest[155]=pp[psrc[155]];case 155:pdest[154]=pp[psrc[154]];case 154:pdest[153]=pp[psrc[153]];case 153:pdest[152]=pp[psrc[152]];case 152:pdest[151]=pp[psrc[151]];case 151:pdest[150]=pp[psrc[150]];case 150:pdest[149]=pp[psrc[149]];case 149:pdest[148]=pp[psrc[148]];case 148:pdest[147]=pp[psrc[147]];case 147:pdest[146]=pp[psrc[146]];case 146:pdest[145]=pp[psrc[145]];case 145:pdest[144]=pp[psrc[144]];case 144:pdest[143]=pp[psrc[143]];case 143:pdest[142]=pp[psrc[142]];case 142:pdest[141]=pp[psrc[141]];case 141:pdest[140]=pp[psrc[140]];case 140:pdest[139]=pp[psrc[139]];case 139:pdest[138]=pp[psrc[138]];case 138:pdest[137]=pp[psrc[137]];case 137:pdest[136]=pp[psrc[136]];case 136:pdest[135]=pp[psrc[135]];case 135:pdest[134]=pp[psrc[134]];case 134:pdest[133]=pp[psrc[133]];case 133:pdest[132]=pp[psrc[132]];case 132:pdest[131]=pp[psrc[131]];case 131:pdest[130]=pp[psrc[130]];case 130:pdest[129]=pp[psrc[129]];case 129:pdest[128]=pp[psrc[128]];case 128:pdest[127]=pp[psrc[127]];case 127:pdest[126]=pp[psrc[126]];case 126:pdest[125]=pp[psrc[125]];case 125:pdest[124]=pp[psrc[124]];case 124:pdest[123]=pp[psrc[123]];case 123:pdest[122]=pp[psrc[122]];case 122:pdest[121]=pp[psrc[121]];case 121:pdest[120]=pp[psrc[120]];case 120:pdest[119]=pp[psrc[119]];case 119:pdest[118]=pp[psrc[118]];case 118:pdest[117]=pp[psrc[117]];case 117:pdest[116]=pp[psrc[116]];case 116:pdest[115]=pp[psrc[115]];case 115:pdest[114]=pp[psrc[114]];case 114:pdest[113]=pp[psrc[113]];case 113:pdest[112]=pp[psrc[112]];case 112:pdest[111]=pp[psrc[111]];case 111:pdest[110]=pp[psrc[110]];case 110:pdest[109]=pp[psrc[109]];case 109:pdest[108]=pp[psrc[108]];case 108:pdest[107]=pp[psrc[107]];case 107:pdest[106]=pp[psrc[106]];case 106:pdest[105]=pp[psrc[105]];case 105:pdest[104]=pp[psrc[104]];case 104:pdest[103]=pp[psrc[103]];case 103:pdest[102]=pp[psrc[102]];case 102:pdest[101]=pp[psrc[101]];case 101:pdest[100]=pp[psrc[100]];case 100:pdest[99]=pp[psrc[99]];case 99:pdest[98]=pp[psrc[98]];case 98:pdest[97]=pp[psrc[97]];case 97:pdest[96]=pp[psrc[96]];case 96:pdest[95]=pp[psrc[95]];case 95:pdest[94]=pp[psrc[94]];case 94:pdest[93]=pp[psrc[93]];case 93:pdest[92]=pp[psrc[92]];case 92:pdest[91]=pp[psrc[91]];case 91:pdest[90]=pp[psrc[90]];case 90:pdest[89]=pp[psrc[89]];case 89:pdest[88]=pp[psrc[88]];case 88:pdest[87]=pp[psrc[87]];case 87:pdest[86]=pp[psrc[86]];case 86:pdest[85]=pp[psrc[85]];case 85:pdest[84]=pp[psrc[84]];case 84:pdest[83]=pp[psrc[83]];case 83:pdest[82]=pp[psrc[82]];case 82:pdest[81]=pp[psrc[81]];case 81:pdest[80]=pp[psrc[80]];case 80:pdest[79]=pp[psrc[79]];case 79:pdest[78]=pp[psrc[78]];case 78:pdest[77]=pp[psrc[77]];case 77:pdest[76]=pp[psrc[76]];case 76:pdest[75]=pp[psrc[75]];case 75:pdest[74]=pp[psrc[74]];case 74:pdest[73]=pp[psrc[73]];case 73:pdest[72]=pp[psrc[72]];case 72:pdest[71]=pp[psrc[71]];case 71:pdest[70]=pp[psrc[70]];case 70:pdest[69]=pp[psrc[69]];case 69:pdest[68]=pp[psrc[68]];case 68:pdest[67]=pp[psrc[67]];case 67:pdest[66]=pp[psrc[66]];case 66:pdest[65]=pp[psrc[65]];case 65:pdest[64]=pp[psrc[64]];case 64:pdest[63]=pp[psrc[63]];case 63:pdest[62]=pp[psrc[62]];case 62:pdest[61]=pp[psrc[61]];case 61:pdest[60]=pp[psrc[60]];case 60:pdest[59]=pp[psrc[59]];case 59:pdest[58]=pp[psrc[58]];case 58:pdest[57]=pp[psrc[57]];case 57:pdest[56]=pp[psrc[56]];case 56:pdest[55]=pp[psrc[55]];case 55:pdest[54]=pp[psrc[54]];case 54:pdest[53]=pp[psrc[53]];case 53:pdest[52]=pp[psrc[52]];case 52:pdest[51]=pp[psrc[51]];case 51:pdest[50]=pp[psrc[50]];case 50:pdest[49]=pp[psrc[49]];case 49:pdest[48]=pp[psrc[48]];case 48:pdest[47]=pp[psrc[47]];case 47:pdest[46]=pp[psrc[46]];case 46:pdest[45]=pp[psrc[45]];case 45:pdest[44]=pp[psrc[44]];case 44:pdest[43]=pp[psrc[43]];case 43:pdest[42]=pp[psrc[42]];case 42:pdest[41]=pp[psrc[41]];case 41:pdest[40]=pp[psrc[40]];case 40:pdest[39]=pp[psrc[39]];case 39:pdest[38]=pp[psrc[38]];case 38:pdest[37]=pp[psrc[37]];case 37:pdest[36]=pp[psrc[36]];case 36:pdest[35]=pp[psrc[35]];case 35:pdest[34]=pp[psrc[34]];case 34:pdest[33]=pp[psrc[33]];case 33:pdest[32]=pp[psrc[32]];case 32:pdest[31]=pp[psrc[31]];case 31:pdest[30]=pp[psrc[30]];case 30:pdest[29]=pp[psrc[29]];case 29:pdest[28]=pp[psrc[28]];case 28:pdest[27]=pp[psrc[27]];case 27:pdest[26]=pp[psrc[26]];case 26:pdest[25]=pp[psrc[25]];case 25:pdest[24]=pp[psrc[24]];case 24:pdest[23]=pp[psrc[23]];case 23:pdest[22]=pp[psrc[22]];case 22:pdest[21]=pp[psrc[21]];case 21:pdest[20]=pp[psrc[20]];case 20:pdest[19]=pp[psrc[19]];case 19:pdest[18]=pp[psrc[18]];case 18:pdest[17]=pp[psrc[17]];case 17:pdest[16]=pp[psrc[16]];case 16:pdest[15]=pp[psrc[15]];case 15:pdest[14]=pp[psrc[14]];case 14:pdest[13]=pp[psrc[13]];case 13:pdest[12]=pp[psrc[12]];case 12:pdest[11]=pp[psrc[11]];case 11:pdest[10]=pp[psrc[10]];case 10:pdest[9]=pp[psrc[9]];case 9:pdest[8]=pp[psrc[8]];case 8:pdest[7]=pp[psrc[7]];case 7:pdest[6]=pp[psrc[6]];case 6:pdest[5]=pp[psrc[5]];case 5:pdest[4]=pp[psrc[4]];case 4:pdest[3]=pp[psrc[3]];case 3:pdest[2]=pp[psrc[2]];case 2:pdest[1]=pp[psrc[1]];case 1:pdest[0]=pp[psrc[0]];

//copy psrc to pdest, pdest reversed order
void u8revcpy(unsigned char *pdest, const unsigned char *psrc, unsigned len)
{
    psrc -= 960 - (int)len;
    pdest -= (int)len - 1;
    switch(len)
    {
        _revcpy
    }
    len -= 255;
}

// reverse copy with a remap table
void u8revpcpy(unsigned char *pdest, const unsigned char *psrc, unsigned char *pp, unsigned len)
{
    psrc -= 960 - (int)len;
    pdest -= (int)len - 1;
    switch(len)
    {
        _revpcpy
    }
}

//copy with a remap table
void u8pcpy(unsigned char *pdest, const unsigned char *psrc, unsigned char *pp, unsigned len)
{
    switch(len)
    {
        _pcpy
    }
}

//----------16bit
// reverse copy with a remap table
void u16revpcpy(unsigned short *pdest, const unsigned char *psrc, unsigned short *pp, unsigned len)
{
    psrc -= 960 - (int)len;
    pdest -= len - 1;
    switch(len)
    {
        _revpcpy
    }
}

//copy with a remap table
void u16pcpy(unsigned short *pdest, const unsigned char *psrc, unsigned short *pp, unsigned len)
{
    /*
    	// plain testcode to measure if that fugly macro actually is that fast or just slows down compile time, and freezing my texteditor when selecting that line
    	unsigned l;
    	if (len) {
    		l = len;
    		if(l < 961)
    			do {
    				l--;
    				pdest[l] = pp[psrc[l]];
    			} while (l);
    	}
    	*/
    switch(len)
    {
        _pcpy
    }
}

void u32revpcpy(unsigned *pdest, const unsigned char *psrc, unsigned *pp, unsigned len)
{
    psrc -= 960 - (int)len;
    pdest -= (int)len - 1;
    switch(len)
    {
        _revpcpy
    }
}

//copy with a remap table
void u32pcpy(unsigned *pdest, const unsigned char *psrc, unsigned *pp, unsigned len)
{
    switch(len)
    {
        _pcpy
    }
}


#endif
