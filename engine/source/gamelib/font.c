/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "types.h"
#include "screen.h"
#include "loadimg.h"
#include "bitmap.h"
#include "sprite.h"
#include "spriteq.h"
#include "font.h"


s_font **fonts[MAX_FONTS];

static char b[1024];

void _font_unload(s_font *font)
{
    int i;
    for(i = 0; i < 256; i++)
    {
        if(font->token[i] != NULL)
        {
            if(font->token[i]->mask != NULL)
            {
                free(font->token[i]->mask);
            }
            free(font->token[i]);
        }
        font->token[i] = NULL;
    }
    free(font);
}

void font_unload(int which)
{
    int j, max;
    s_font **sets, *font;

    which %= MAX_FONTS;

    sets = fonts[which];

    if(!sets)
    {
        return;
    }

    max = (sets[0] && sets[0]->mbs) ? 256 : 1;

    for(j = 0; j < max; j++)
    {
        font = sets[j];
        if(!font)
        {
            continue;
        }
        _font_unload(font);
        sets[j] = NULL;
    }

    free(sets);
    fonts[which] = NULL;
}

int _font_load(s_font *font, char *filename, char *packfile)
{
    int x, y;
    int index = 0;
    int size;
    int cx = 0, cy = 0;
    s_bitmap *bitmap = NULL;
    s_screen *screen = NULL;
    int rval = 0;
    int tw, th;

    if(!loadscreen(filename, packfile, NULL, pixelformat, &screen))
    {
        goto err;
    }
    font->width = tw = screen->width / 16;
    font->height = th = screen->height / 16;
    if(!(bitmap = allocbitmap(tw, th, pixelformat)))
    {
        goto err;
    }

    if(bitmap->palette && screen->palette)
    {
        memcpy(bitmap->palette, screen->palette, PAL_BYTES);
    }

    // grab tokens
    for(y = 0; y < 16; y++)
    {
        for(x = 0; x < 16; x++)
        {
            getbitmap(x * tw, y * th, tw, th, bitmap, screen);
            clipbitmap(bitmap, &cx, NULL, &cy, NULL);
            if(index > 0)
            {
                bitmap->palette = NULL;
            }
            size = fakey_encodesprite(bitmap);
            font->token[index] = (s_sprite *)malloc(size);
            if(!font->token[index])
            {
                goto err;
            }
            encodesprite(-cx, -cy, bitmap, font->token[index]);
            font->token_width[index] = font->mono ? tw : (font->token[index]->width + (tw / 10));
            if(font->token_width[index] <= 1)
            {
                font->token_width[index] = tw / 3;
            }
            if(index > 0)
            {
                font->token[index]->palette = font->token[0]->palette ;
                font->token[index]->pixelformat = screen->pixelformat ;
            }
            ++index;
        }
    }

    rval = 1;

err:
    freebitmap(bitmap);
    freescreen(&screen);

    return rval;
}

int _font_loadmask(s_font *font, char *filename, char *packfile)
{
    int x, y;
    int index = 0;
    int size;
    int cx = 0, cy = 0;
    s_bitmap *bitmap = NULL;
    s_screen *screen = NULL;
    int rval = 0;
    int tw, th;
    int cw;

    if(!loadscreen(filename, packfile, NULL, pixelformat, &screen))
    {
        goto err;
    }
    tw = screen->width / 16;
    th = screen->height / 16;
    if(tw != font->width || th != font->height)
    {
        goto err;
    }
    if(!(bitmap = allocbitmap(tw, th, pixelformat)))
    {
        goto err;
    }

    if(bitmap->palette && screen->palette)
    {
        memcpy(bitmap->palette, screen->palette, PAL_BYTES);
    }

    // grab tokens
    for(y = 0; y < 16; y++)
    {
        for(x = 0; x < 16; x++)
        {
            getbitmap(x * tw, y * th, tw, th, bitmap, screen);
            clipbitmap(bitmap, &cx, NULL, &cy, NULL);
            if(index > 0)
            {
                bitmap->palette = NULL;
            }
            size = fakey_encodesprite(bitmap);
            if(!font->token[index])
            {
                goto err;
            }
            if(font->token[index]->mask)
            {
                goto err;
            }
            font->token[index]->mask = (s_sprite *)malloc(size);
            if(!font->token[index]->mask)
            {
                goto err;
            }
            encodesprite(-cx, -cy, bitmap, font->token[index]->mask);
            cw = font->mono ? tw : (font->token[index]->mask->width + (tw / 10));
            if(cw <= 1)
            {
                cw = tw / 3;
            }
            if(cw != font->token_width[index])
            {
                goto err;
            }
            if(index > 0)
            {
                //font->token[index]->mask->palette = font->token[0]->mask->palette ;
                font->token[index]->mask->pixelformat = screen->pixelformat ;
            }
            ++index;
        }
    }

    rval = 1;

err:
    freebitmap(bitmap);
    freescreen(&screen);

    return rval;
}

int font_load(int which, char *filename, char *packfile, int flags)
{
    s_font **sets, *font;
    int i, max;

    which %= MAX_FONTS;

    font_unload(which);

    max = (flags & FONT_MBS) ? 256 : 1;
    // UT: 129 should be enough for mbs, use 256 to keep the logic simpler

    fonts[which] = sets = malloc(sizeof(s_font *)*max);
    memset(sets, 0, sizeof(s_font *)*max);

    for(i = 0; i < max; i++)
    {
        if(i == 1)
        {
            i = 128;
        }
        font = malloc(sizeof(s_font));
        memset(font, 0, sizeof(s_font));
        font->mono = ((flags & FONT_MONO) != 0);
        font->mbs = ((flags & FONT_MBS) != 0);
        if(font->mbs)
        {
            sprintf(b, "%s/%02x", filename, i);
        }
        else
        {
            strcpy(b, filename);
        }

        if(!_font_load(font, b, packfile))
        {
            _font_unload(font);
            font = NULL;
        }
        sets[i] = font;
    }

    if(sets[0] == NULL)
    {
        font_unload(which);
        return 0;
    }

    return 1;

}

// loads an alpha mask for an already-loaded font
int font_loadmask(int which, char *filename, char *packfile, int flags)
{
    s_font **sets, *font;
    int i, max, ret = 0;

    which %= MAX_FONTS;
    sets = fonts[which];
    if(!sets)
    {
        return 0;
    }
    max = (flags & FONT_MBS) ? 256 : 1;

    for(i = 0; i < max; i++)
    {
        if(i == 1)
        {
            i = 128;
        }
        font = sets[i];
        if(!font)
        {
            continue;
        }
        if(font->mbs)
        {
            sprintf(b, "%s/%02x", filename, i);
        }
        else
        {
            strcpy(b, filename);
        }

        if(!_font_loadmask(font, b, packfile))
        {
            ;//return 0;
        }
        else
        {
            ret = 1;
        }
    }

    return ret;

}

int fontmonowidth(int which)
{

    s_font **sets;
    which %= MAX_FONTS;

    sets = fonts[which];

    if(!sets || !sets[0])
    {
        return 0;
    }

    return sets[0]->width;
}

int fontheight(int which)
{
    s_font **sets;
    which %= MAX_FONTS;

    sets = fonts[which];

    if(!sets || !sets[0])
    {
        return 0;
    }

    return sets[0]->height;
}


int font_string_width(int which, char *format, ...)
{
    int w = 0;
    char *buf = b, c;
    va_list arglist;
    s_font **sets, *font;
    int mbs, index;

    which %= MAX_FONTS;

    sets = fonts[which];

    if(!sets || !format)
    {
        return 0;
    }

    mbs = sets[0]->mbs;

    va_start(arglist, format);
    vsprintf(buf, format, arglist);
    va_end(arglist);

    if(!mbs)
    {
        font = sets[0];

        if(font)
            while(*buf)
            {
                w += font->token_width[((int)(*buf)) & 0xFF];
                buf++;
            }
    }
    else
    {
        while((c = *buf))
        {
            if((c & 0x80) && buf[1])
            {
                index = (unsigned char)c;
                buf++;
            }
            else
            {
                index = 0;
            }

            font = sets[index];

            if(font)
            {
                w += font->token_width[((int)(*buf)) & 0xFF];
            }
            buf++;
        }
    }
    return w;
}

void font_printf(int x, int y, int which, int layeroffset, char *format, ...)
{
    char *buf = b, c;
    va_list arglist;
    int ox = x;
    s_font **sets, *font;
    int mbs, index, w, lf;

    which %= MAX_FONTS;

    sets = fonts[which];

    if(!sets)
    {
        return;
    }

    mbs = sets[0]->mbs;

    va_start(arglist, format);
    vsprintf(buf, format, arglist);
    va_end(arglist);

    while((c = *buf))
    {
        lf = (c == '\n');

        if(mbs && (c & 0x80) && buf[1])
        {
            index = (unsigned char)c;
            buf++;
        }
        else
        {
            index = 0;
        }

        font = sets[index];

        if(font)
        {
            if(lf)
            {
                x = ox;
                y += font->height;
            }
            else
            {
                w = font->token_width[((int)(*buf)) & 0xFF];
                spriteq_add_frame(x, y, FONT_LAYER + layeroffset, font->token[((int)(*buf)) & 0xFF], NULL, 0);
                x += w;
            }
        }
        buf++;
    }
}


// Print to a screen rather than queueing the sprites
void screen_printf(s_screen *screen, int x, int y, int which, char *format, ...)
{
    char *buf = b, c;
    va_list arglist;
    int ox = x;
    s_font **sets, *font;
    int mbs, index, w, lf;

    which %= MAX_FONTS;

    sets = fonts[which];

    if(!sets)
    {
        return;
    }

    mbs = sets[0]->mbs;

    va_start(arglist, format);
    vsprintf(buf, format, arglist);
    va_end(arglist);

    while((c = *buf))
    {
        lf = (c == '\n');

        if(mbs && (c & 0x80) && buf[1])
        {
            index = (unsigned char)c;
            buf++;
        }
        else
        {
            index = 0;
        }

        font = sets[index];

        if(font)
        {
            if(lf)
            {
                x = ox;
                y += font->height;
            }
            else
            {
                w = font->token_width[((int)(*buf)) & 0xFF];
                putsprite(x, y, font->token[((int)(*buf)) & 0xFF], screen, NULL);
                x += w;
            }
        }
        buf++;
    }
}



