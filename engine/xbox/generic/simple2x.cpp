/*
 * VisualBoyAdvanced - Nintendo Gameboy/GameboyAdvance (TM) emulator
 * Copyrigh(c) 1999-2002 Forgotten (vb@emuhq.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <xtl.h>
//#include "System.h"

#define u8 unsigned char
#define u32 DWORD
#define u16 WORD

#ifdef __cplusplus
extern "C" {
#endif

void Simple2x(u8 *srcPtr, u32 srcPitch, u8 * /* deltaPtr */,
              u8 *dstPtr, u32 dstPitch, int width, int height, int scanlines)
{
  u8 *nextLine, *finish;
  
  nextLine = dstPtr + dstPitch;
  
  do {
    u32 *bP = (u32 *) srcPtr;
    u32 *dP = (u32 *) dstPtr;
    u32 *nL = (u32 *) nextLine;
    u32 currentPixel;
    
    finish = (u8 *) bP + ((width+2) << 1);
    currentPixel = *bP++;
    
    do {
#ifdef WORDS_BIGENDIAN
      u32 color = currentPixel >> 16;
#else
      u32 color = currentPixel & 0xffff;
#endif

      color = color | (color << 16);

      *(dP) = color;
      *(nL) = color;

#ifdef WORDS_BIGENDIAN
      color = (currentPixel << 16) >> 16;
#else
      color = currentPixel >> 16;
#endif
      color = color| (color << 16);      
      *(dP + 1) = color;
      *(nL + 1) = color;
      
      currentPixel = *bP++;
      
      dP += 2;
      nL += 2;
    } while ((u8 *) bP < finish);
    
    srcPtr += srcPitch;
    dstPtr += dstPitch * 2;
    nextLine += dstPitch * 2;
  }
  while (--height);
}

void Simple2x32(u8 *srcPtr, u32 srcPitch, u8 * /* deltaPtr */,
                u8 *dstPtr, u32 dstPitch, int width, int height)
{
  u8 *nextLine, *finish;
  
  nextLine = dstPtr + dstPitch;
  
  do {
    u32 *bP = (u32 *) srcPtr;
    u32 *dP = (u32 *) dstPtr;
    u32 *nL = (u32 *) nextLine;
    u32 currentPixel;
    
    finish = (u8 *) bP + ((width+1) << 2);
    currentPixel = *bP++;
    
    do {
      u32 color = currentPixel;

      *(dP) = color;
      *(dP+1) = color;
      *(nL) = color;
      *(nL + 1) = color;
      
      currentPixel = *bP++;
      
      dP += 2;
      nL += 2;
    } while ((u8 *) bP < finish);
    
    srcPtr += srcPitch;
    dstPtr += dstPitch * 2;
    nextLine += dstPitch * 2;
  }
  while (--height);
}
#ifdef __cplusplus
}
#endif
