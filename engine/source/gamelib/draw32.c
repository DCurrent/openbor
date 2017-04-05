/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

// draw functions with 32bit pixel format, with alpha blending

#include <string.h>
#include "types.h"

#ifndef		NULL
#define		NULL	 ((void*)0)
#endif

#define		abso(x)		(x<0?-x:x)

#define __putpixel32(p) \
			if(blendfp )\
			{\
				*(p) = blendfp(colour, *(p));\
			}\
			else\
			{\
				*(p) = colour;\
			}

// same as the one in draw.c, this is 32bit version
// blendfp is the blending function pointer
void line32(int sx, int sy, int ex, int ey, unsigned colour, s_screen *screen, int alpha)
{
    int diffx, diffy;
    int absdiffx, absdiffy;
    int xdir, ydir;
    int thres;
    int d;
    unsigned *data;
    unsigned(*blendfp)(unsigned, unsigned);

    // Some off-screen lines may slip through this test!
    if(sx < 0 && ex < 0)
    {
        return;
    }
    if(sy < 0 && ey < 0)
    {
        return;
    }
    if(sx >= screen->width && ex >= screen->width)
    {
        return;
    }
    if(sy >= screen->height && ey >= screen->height)
    {
        return;
    }


    // Check clipping and calculate new coords if necessary

    diffx = ex - sx;
    diffy = ey - sy;

    if(sx < 0)
    {
        sy -= (sx * diffy / diffx);
        sx = 0;
    }
    if(sy < 0)
    {
        sx -= (sy * diffx / diffy);
        sy = 0;
    }
    if(sx >= screen->width)
    {
        sy -= ((sx - screen->width) * diffy / diffx);
        sx = screen->width - 1;
    }
    if(sy >= screen->height)
    {
        sx -= ((sy - screen->height) * diffx / diffy);
        sy = screen->height - 1;
    }

    if(ex < 0)
    {
        ey -= (ex * diffy / diffx);
        ex = 0;
    }
    if(ey < 0)
    {
        ex -= (ey * diffx / diffy);
        ey = 0;
    }
    if(ex >= screen->width)
    {
        ey -= ((ex - screen->width) * diffy / diffx);
        ex = screen->width - 1;
    }
    if(ey >= screen->height)
    {
        ex -= ((ey - screen->height) * diffx / diffy);
        ey = screen->height - 1;
    }


    // Second test: the lines that passed test 1 won't pass this time!
    if(sx < 0 || ex < 0)
    {
        return;
    }
    if(sy < 0 || ey < 0)
    {
        return;
    }
    if(sx >= screen->width || ex >= screen->width)
    {
        return;
    }
    if(sy >= screen->height || ey >= screen->height)
    {
        return;
    }


    // Recalculate directions
    diffx = ex - sx;
    diffy = ey - sy;

    absdiffx = abso(diffx);
    absdiffy = abso(diffy);

    sy *= screen->width;
    ey *= screen->width;

    data = (unsigned *)screen->data;

    colour &= 0x00FFFFFF;

    blendfp = getblendfunction32(alpha);

    if(absdiffx > absdiffy)
    {
        // Draw a flat line
        thres = absdiffx >> 1;
        xdir = 1;
        if(diffx < 0)
        {
            xdir = -xdir;
        }
        ydir = screen->width;
        if(diffy < 0)
        {
            ydir = -ydir;
        }
        while(sx != ex)
        {
            d = sx + sy;
            __putpixel32(data + d);
            sx += xdir;
            if((thres -= absdiffy) <= 0)
            {
                sy += ydir;
                thres += absdiffx;
            }
        }
        d = ex + ey;
        __putpixel32(data + d);
        return;
    }

    // Draw a high line
    thres = absdiffy >> 1;
    xdir = 1;
    if(diffx < 0)
    {
        xdir = -1;
    }
    ydir = screen->width;
    if(diffy < 0)
    {
        ydir = -ydir;
    }
    while(sy != ey)
    {
        d = sx + sy;
        __putpixel32(data + d);
        sy += ydir;
        if((thres -= absdiffx) <= 0)
        {
            sx += xdir;
            thres += absdiffy;
        }
    }
    d = ex + ey;
    __putpixel32(data + d);
}




// drawbox, 32bit version
void drawbox32(int x, int y, int width, int height, unsigned colour, s_screen *screen, int alpha)
{
    unsigned *cp;
    unsigned(*blendfp)(unsigned, unsigned);

    if(width <= 0)
    {
        return;
    }
    if(height <= 0)
    {
        return;
    }
    if(screen == NULL)
    {
        return;
    }

    if(x < 0)
    {
        if((width += x) <= 0)
        {
            return;
        }
        x = 0;
    }
    else if(x >= screen->width)
    {
        return;
    }
    if(y < 0)
    {
        if((height += y) <= 0)
        {
            return;
        }
        y = 0;
    }
    else if(y >= screen->height)
    {
        return;
    }
    if(x + width > screen->width)
    {
        width = screen->width - x;
    }
    if(y + height > screen->height)
    {
        height = screen->height - y;
    }

    cp = ((unsigned *)screen->data) + (y * screen->width + x);
    colour &= 0x00FFFFFF;

    blendfp = getblendfunction32(alpha);

    while(--height >= 0)
    {
        for(x = 0; x < width; x++)
        {
            __putpixel32(cp);
            cp++;
        }
        cp += (screen->width - width);
    }
}



// Putpixel 32bit version
void _putpixel32(unsigned x, unsigned y, unsigned colour, s_screen *screen, int alpha)
{
    int pixind;
    unsigned *data ;
    unsigned(*blendfp)(unsigned, unsigned);
    if(x > screen->width || y > screen->height)
    {
        return;
    }
    pixind = x + y * screen->width;
    data = (unsigned *)screen->data + pixind;
    colour &= 0x00FFFFFF;
    blendfp = getblendfunction32(alpha);
    __putpixel32(data);
}


