/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <stdio.h>
#include <string.h>
#include "types.h"

extern float _sinfactors[256];
#define distortion(x, a) ((int)(_sinfactors[x]*a+0.5))

//with remap, work only under 8bit pixel format
void putscreenx8p32_water(s_screen * dest, s_screen * src, int x, int y, int key, u32* remap, u32 (*blendfp)(u32,u32), int amplitude, float wavelength, int time, int watermode)
{
	unsigned char *sp = src->data;
	u32 *dp = (u32*)dest->data;
	int i;
	int sw = src->width;
	int sh = src->height;
	int dw = dest->width;
	int dh = dest->height;
	int ch = sh;
	float s = (float)(time % 255);
	int sox, soy;
	int t, u;

	// Copy anything at all?
	if(x + amplitude*2 + sw <= 0 || x - amplitude*2  >= dw) return;
	//if(x >= dw) return;
	//if(sw+x <= 0) return;
	//if(y >= dh) return;
	//if(sh+y <= 0) return;

	if(!remap) remap = (u32*)src->palette;

	if(!remap) return;

	sox = 0;
	soy = 0;

	// Clip?
	//if(x<0) {sox = -x; cw += x;}
	if(y<0) {soy = -y; ch += y;}

	//if(x+sw > dw) cw -= (x+sw) - dw;
	if(y+sh > dh) ch -= (y+sh) - dh;

	//if(x<0) x = 0;
	if(y<0) y = 0;

	sp += soy*sw;
	dp += (y*dw + x);

	u = (watermode==1)?distortion((int)s, amplitude):amplitude;
    wavelength = 255 / wavelength;
    s += soy*wavelength;

    if(blendfp)
    {
        if(key)
        {
    	    // blend
            do{
                s = s - (int)s + (int)s % 255;
                t = (distortion((int)s, amplitude)) - u;

                        // Nothing to display
                if(x + t + sw - 1 < 0 || x + t + 1 > dw){}

                // layer is cropped off at the left
                else if(x + t < 0)
                {
                      i=sw+t+x-1;
                      do{
                           if(sp[i-x-t]==0)continue;
                           dp[i-x] = blendfp(remap[sp[i-x-t]], dp[i-x]);
                      }while(i--);
                      //memcpy(dp - x, sp - x - t, slinew + t + x);
                }

                // layer is cropped off at the right
                else if(x + sw + t > dw)
                {
                     i=dw-x-t-1;
                     do{
                          if(sp[i]==0)continue;
                          dp[i+t] = blendfp(remap[sp[i]], dp[i+t]);
                     }while(i--);
                     //memcpy(dp + t, sp, dlinew - x - t);
                }


                // formula for all other cases
                else
                {
                     i=sw-1;
                     do{
                           if(sp[i]==0)continue;
                           dp[i+t] = blendfp(remap[sp[i]], dp[i+t]);
                     }while(i--);
                     //memcpy(dp + t, sp, sw);
                }

                s += wavelength;
                /*i=cw-1;
                do
                {
                   if(!sp[i])continue;
                   dp[i] = blendfp(remap[sp[i]], dp[i]);
                }while(i--);*/
                sp += sw;
                dp += dw;
            }while(--ch);
        }
        else
        {
    	    // blend
            do{
                s = s - (int)s + (int)s % 255;
                t = (distortion((int)s, amplitude)) - u;

                        // Nothing to display
                if(x + t + sw < 0 || x + t + 1 > dw){}

                // layer is cropped off at the left
                else if(x + t < 0)
                {
                      i=sw+t+x-1;
                      do{
                           //if(sp[i-x-t+1]==0)continue;
                           dp[i-x] = blendfp(remap[sp[i-x-t]], dp[i-x]);
                      }while(i--);
                      //memcpy(dp - x, sp - x - t, slinew + t + x);
                }

                // layer is cropped off at the right
                else if(x + sw + t > dw)
                {
                     i=dw-x-t-1;
                     do{
                          //if(sp[i+1]==0)continue;
                          dp[i+t] = blendfp(remap[sp[i]], dp[i+t]);
                     }while(i--);
                     //memcpy(dp + t, sp, dlinew - x - t);
                }


                // formula for all other cases
                else
                {
                     i=sw-1;
                     do{
                           //if(sp[i+1]==0)continue;
                           dp[i+t] = blendfp(remap[sp[i]], dp[i+t]);
                     }while(i--);
                     //memcpy(dp + t, sp, sw);
                }

                s += wavelength;
                /*i=cw-1;
                do
                {
                   dp[i] = blendfp(remap[sp[i]], dp[i]);
                }while(i--);*/
                sp += sw;
                dp += dw;
            }while(--ch);
        }
    }
    else
    {
        if(key)
        {
        	// Copy data
            do{
                s = s - (int)s + (int)s % 255;
                t = (distortion((int)s, amplitude)) - u;

                        // Nothing to display
                if(x + t + sw - 1 < 0 || x + t + 1 > dw){}

                // layer is cropped off at the left
                else if(x + t < 0)
                {
                      i=sw+t+x-1;
                      do{
                           if(sp[i-x-t]==0)continue;
                           dp[i-x] = remap[sp[i-x-t]];
                      }while(i--);
                      //memcpy(dp - x, sp - x - t, slinew + t + x);
                }

                // layer is cropped off at the right
                else if(x + sw + t > dw)
                {
                     i=dw-x-t-1;
                     do{
                          if(sp[i]==0)continue;
                          dp[i+t] = remap[sp[i]];
                     }while(i--);
                     //memcpy(dp + t, sp, dlinew - x - t);
                }


                // formula for all other cases
                else
                {
                     i=sw-1;
                     do{
                           if(sp[i]==0)continue;
                           dp[i+t] = remap[sp[i]];
                     }while(i--);
                     //memcpy(dp + t, sp, sw);
                }

                s += wavelength;
                /*i=cw-1;
                do
                {
                   if(!sp[i])continue;
                   dp[i] = remap[sp[i]];
                }while(i--); */
                sp += sw;
                dp += dw;
            }while(--ch);
        }
        else
        {
        	// Copy data
            do{
                s = s - (int)s + (int)s % 255;
                t = (distortion((int)s, amplitude)) - u;

                // Nothing to display
                if(x + t + sw < 0 || x + t + 1> dw){}

                // layer is cropped off at the left
                else if(x + t < 0)
                     u32pcpy(dp - x, sp - x - t, remap, sw + t + x);

                // layer is cropped off at the right
                else if(x + sw + t > dw)
                     u32pcpy(dp + t, sp, remap, dw - x - t);

                // formula for all other cases
                else
                     u32pcpy(dp + t, sp, remap, sw);

                s += wavelength;
                //u16pcpy(dp, sp, remap, cw);
                sp += sw;
                dp += dw;
            }while(--ch);
        }
    }
}

//with remap, work only under 8bit pixel format
void putscreenx8p32(s_screen * dest, s_screen * src, int x, int y, int key, u32* remap, u32(*blendfp)(u32,u32))
{
	unsigned char *sp = src->data;
	u32 *dp = (u32*)dest->data;
	int i;
	int sw = src->width;
	int sh = src->height;
	int dw = dest->width;
	int dh = dest->height;
	int cw = sw, ch = sh;
	int sox, soy;

	// Copy anything at all?
	if(x >= dw) return;
	if(sw+x <= 0) return;
	if(y >= dh) return;
	if(sh+y <= 0) return;

	if(!remap) remap = (u32*)src->palette;

	if(!remap) return;

	sox = 0;
	soy = 0;

	// Clip?
	if(x<0) {sox = -x; cw += x;}
	if(y<0) {soy = -y; ch += y;}

	if(x+sw > dw) cw -= (x+sw) - dw;
	if(y+sh > dh) ch -= (y+sh) - dh;

	if(x<0) x = 0;
	if(y<0) y = 0;

	sp += (soy*sw + sox);
	dp += (y*dw + x);

    if(blendfp)
    {
        if(key) // color key, should be slower
        {
    	    // blend
            do{
                i=cw-1;
                do
                {
                   if(sp[i])continue;
                   dp[i] = blendfp(remap[sp[i]], dp[i]);
                }while(i--);
                sp += sw;
                dp += dw;
            }while(--ch);
        }
        else // without colorkey
        {
    	    // blend
            do{
                i=cw-1;
                do
                {
                   dp[i] = blendfp(remap[sp[i]], dp[i]);
                }while(i--);
                sp += sw;
                dp += dw;
            }while(--ch);
        }
    }
    else //without blend
    {
        if(key) // with color key
        {
        	// Copy data
            do{
                i=cw-1;
                do
                {
                   if(!sp[i])continue;
                   dp[i] = remap[sp[i]];
                }while(i--);
                sp += sw;
                dp += dw;
            }while(--ch);
        }
        else // without colorkey
        {
        	// Copy data
            do{
                u32pcpy(dp, sp, remap, cw);
                sp += sw;
                dp += dw;
            }while(--ch);
        }
    }
}

//32 to 32
void blendscreen32(s_screen * dest, s_screen * src, int x, int y, int key, u32(*blendfp)(u32, u32))
{
	u32 *sp = (u32*)src->data;
	u32 *dp = (u32*)dest->data;
	int i;
	int sw = src->width;
	int sh = src->height;
	int dw = dest->width;
	int dh = dest->height;
	int cw = sw, ch = sh;
	int sox, soy;

	// Copy anything at all?
	if(x >= dw) return;
	if(sw+x <= 0) return;
	if(y >= dh) return;
	if(sh+y <= 0) return;

	sox = 0;
	soy = 0;

	// Clip?
	if(x<0) {sox = -x; cw += x;}
	if(y<0) {soy = -y; ch += y;}

	if(x+sw > dw) cw -= (x+sw) - dw;
	if(y+sh > dh) ch -= (y+sh) - dh;

	if(x<0) x = 0;
	if(y<0) y = 0;

	sp += (soy*sw + sox);
	dp += (y*dw + x);

    if(blendfp)
    {
        if(key) // with colour key
        {
        	// Copy data
        	do{
                i=cw-1;
                do
                {
                   if(sp[i]==0) continue;
                   dp[i] = blendfp(sp[i], dp[i]);
                }while(i--);
        		sp += sw;
        		dp += dw;
        	}while(--ch);
        }
        else //without colour key
        {
        	// Copy data
        	do{
                i=cw-1;
                do
                {
                   dp[i] = blendfp(sp[i], dp[i]);
                }while(i--);
        		sp += sw;
        		dp += dw;
        	}while(--ch);
        }
    }
    else // without blend
    {
        if(key) // with colour key
        {
        	// Copy data
        	do{
                i=cw-1;
                do
                {
                   if(sp[i]==0) continue;
                   dp[i] = sp[i];
                }while(i--);
        		sp += sw;
        		dp += dw;
        	}while(--ch);
        }
        else //without colour key
        {
        	// Copy data
        	do{
                memcpy(dp, sp, cw<<2);
        		sp += sw;
        		dp += dw;
        	}while(--ch);
        }
    }

}

// Scale screen
void scalescreen32(s_screen * dest, s_screen * src)
{
	int sw, sh;
	int dw, dh;
	int dx, dy;
	u32 *sp;
	u32 *dp;
	u32 *lineptr;
	unsigned int xstep, ystep, xpos, ypos;
	int pixelformat = src->pixelformat;

	//if(dest->pixelformat!=pixelformat || pixelformat!=PIXEL_16) return;
	if(dest->pixelformat!=pixelformat) return;

	if(src==NULL || dest==NULL) return;
	sp = (u32*)src->data;
	dp = (u32*)dest->data;

	sw = src->width;
	sh = src->height;
	dw = dest->width;
	dh = dest->height;

	xstep = (sw<<16) / dw;
	ystep = (sh<<16) / dh;

	ypos = 0;
	for(dy=0; dy<dh; dy++)
	{
		lineptr = sp + ((ypos>>16) * sw);
		ypos += ystep;
		xpos = 0;
		for(dx=0; dx<dw; dx++)
		{
			*dp = lineptr[xpos>>16];
			++dp;
			xpos += xstep;
		}
	}
}
