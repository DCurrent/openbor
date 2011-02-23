/*
 New sprite code, should work in all resolutions.

 To do:
 - Rewrite optimized code in ASM.


 Format description:

	Offs	Size		Description
	--------------------------------------------
	0	4		Center x coordinate
	4	4		Center y coordinate
	8	4		Image width
	12	4		Image height
	16	height*4	Line offset table (see below)
	?	?		RLE-encoded image data (see below)


 Format of the line offset table:

	Every DWORD in the line offset table points to the start of the
	corresponding line as follows:

	lineptr = tableptr + *tableptr;


 Format of the RLE-encoded image data:

	* The data is encoded per line.
	* The data is DWORD-aligned.
	* When decoding a line, the destination pointer will be reset
	  to it's original position when the end of the line is reached.
	  The data must be formatted properly for this.


	RLE data block format:
	DWORD		advance pointer	"clearcount"
	DWORD		pixel count "viscount"
	(4*?) BYTES	pixels

	The final clearcount in a line will have a zero or negative value.
	This value is used to reset the destination pointer to the start
	of the line. The decoding algorithm then adds the screenwidth to
	this pointer, to advance to the next line.

	The final viscount will have a zero value, serving as an
	end-of-line indicator.

	There is no end-of-sprite indicator; the decoding algorithm
	simply uses a height counter.

	Invalid sprite dimensions (<=0) are checked by the decoding
	algorithm.
*/


#include "types.h"
#include "sprite.h"



static int screenwidth = 16;
static int screenheight = 16;


/*
	Left AND right clipping, seems OK.

	This code continues where the other code cannot.

	PRE:
	- X and Y coords have been adjusted for sprite centering.
	- Vertical clipping adjustments have been performed, if necessary.
	- The frame pointer points at the line offset table of the sprite.
*/
static void ps_bothclip(int x, int y, int width, int height, unsigned long *linetab, unsigned char *dest_c){

    int viscount, viscount_dwords, viscount_bytes;
    unsigned long pixelblock;
    unsigned long *data = linetab;
    int clipcount;
    unsigned char * charptr;
    int widthcount;
    unsigned char *dest_old;


    // I know the x coord is negative! But this is OK anyway.
    dest_c += y*screenwidth + x;

    do{
	// Get ready to draw a line
	data = linetab + (*linetab / 4);
	++linetab;

	dest_old = dest_c;
	widthcount = screenwidth;

	// The following for-loop contains code to skip the left offscreen
	// area of the sprite. Complex shit, especially since it also has
	// to clip on the right side now!

	clipcount = x;
	for(;;){

	    clipcount += *data;		// Add clearcount
	    ++data;
	    if(clipcount >= 0){
		// Reached on-screen area?
		// Check if we've passed the screen entirely!
		if(clipcount >= screenwidth) goto bclip_nextline_entry;

		// Move the screen pointer to the left-most pixel on-screen.
		dest_c -= x;
		// Now add the clipcount overflow.
		dest_c += clipcount;

		// Keep the width counter in check.
		widthcount -= clipcount;

		goto bclip_entry1;
	    }


	    viscount = *data;			// Get viscount
	    ++data;


	    if(viscount <= 0) goto bclip_nextline_entry;	// Reached EOL?


	    if((viscount + clipcount) <= 0){
		// These pixels can be safely skipped.
		clipcount += viscount;

		// Skip the pixels.
		viscount += 3;
		data += viscount>>2;

		continue;
	    }


	    // The pixel run crosses the screen boundary!

	    charptr = (void*)data;

	    // Locate aligned position for frame pointer, past the pixels.
	    data += (viscount+3) >> 2;

	    // Move the screen pointer to the left-most pixel on-screen.
	    dest_c -= x;


	    charptr -= clipcount;	// Find pixels to draw
	    viscount += clipcount;	// How many pixels left to draw?

	    if(viscount >= screenwidth){
		// Fill entire width of screen (dest-aligned).
		// In ASM, this will copy DWORDs.
		viscount = screenwidth;
		do{
		    *dest_c = *charptr;
		    ++dest_c;
		    ++charptr;
		}while(--viscount);
		goto bclip_nextline_entry;
	    }

	    // Keep the width counter updated.
	    widthcount -= viscount;

	    // Draw this run's remaining pixels to the screen.
	    // Dest-aligned, will copy DWORDs in ASM.
	    do{
		*dest_c = *charptr;
		++dest_c;
		++charptr;
	    }while(--viscount);

	    break;		// Continue with right-clipped draw loop.
	}


	for(;;){
	    dest_c += *data;		// Add clearcount
	    widthcount -= *data;
	    if(widthcount<=0) break;	// Clip and go do the next line
	    ++data;

	bclip_entry1:

	    viscount = *data;
	    ++data;


	    if(viscount<=0) break;	// EOL


	    // If too many pixels to draw, cap run.
	    widthcount -= viscount;
	    if(widthcount<0) viscount += widthcount;


	    viscount_dwords = viscount >> 2;
	    viscount_bytes = viscount & 3;

#if 0
	    // Move DWORDS
	    if ((int)dest_c & 3) {
		while(viscount_dwords){
		    pixelblock = *data++;
		    dest_c[0] = pixelblock;
		    pixelblock >>= 8;
		    dest_c[1] = pixelblock;
		    pixelblock >>= 8;
		    dest_c[2] = pixelblock;
		    pixelblock >>= 8;
		    dest_c[3] = pixelblock;
		    dest_c += 4;
		    --viscount_dwords;
		}
	    } else
#endif
    {
		while(viscount_dwords){
		    *(unsigned long*)dest_c = *data++;
		    dest_c += 4;
		    --viscount_dwords;
		}
	    }

	    // Move (max. 3) single bytes?
	    if((--viscount_bytes)<0) goto bclip_finalcheck;
	    pixelblock = *data++;

	    *dest_c++ = pixelblock;

	    if((--viscount_bytes)<0) goto bclip_finalcheck;
	    pixelblock >>= 8;
	    *dest_c++ = pixelblock;

	    if((--viscount_bytes)<0) goto bclip_finalcheck;
	    pixelblock >>= 8;
	    *dest_c++ = pixelblock;

	bclip_finalcheck:
	    if(widthcount<=0) break;	// Clip and go do the next line
	};


    bclip_nextline_entry:

	dest_c = dest_old + screenwidth;

    }while(--height);
}





/*
	Right-side clipping, seems OK.

	This code continues where the unclipped putsprite code cannot.

	PRE:
	- X and Y coords have been adjusted for sprite centering.
	- Vertical clipping adjustments have been performed, if necessary.
*/
static void ps_rightclip(int x, int y, int width, int height, unsigned long *linetab, unsigned char *dest_c){

    int viscount, viscount_dwords, viscount_bytes;
    unsigned long pixelblock;
    unsigned long *data = linetab;
    int widthcount;
    unsigned char *dest_old;


    // Still visible?
    if(x >= screenwidth) return;


    // No need to check left-side clipping, should be done by leftclip code!


    // Get the screen pointer ready...
    dest_c += y*screenwidth + x;


    // Calculate the remaining width.
    width = screenwidth - x;


    do{
	// Get ready to draw a line
	data = linetab + (*linetab / 4);
	++linetab;

	dest_old = dest_c;
	widthcount = width;

	for(;;){
	    dest_c += *data;		// Add clearcount
	    widthcount -= *data;
	    if(widthcount<=0) break;	// Clip and go do the next line
	    ++data;

	    viscount = *data;
	    ++data;


	    if(viscount<=0) break;	// EOL


	    // If too many pixels to draw, cap run.
	    widthcount -= viscount;
	    if(widthcount<0) viscount += widthcount;


	    viscount_dwords = viscount >> 2;
	    viscount_bytes = viscount & 3;
#if 0
	    if ((int)dest_c & 3) {
		while(viscount_dwords){
		    pixelblock = *data++;
		    dest_c[0] = pixelblock;
		    pixelblock >>= 8;
		    dest_c[1] = pixelblock;
		    pixelblock >>= 8;
		    dest_c[2] = pixelblock;
		    pixelblock >>= 8;
		    dest_c[3] = pixelblock;
		    dest_c += 4;
		    --viscount_dwords;
		}
	    } else
#endif
    {
		while(viscount_dwords){
		    *(unsigned long*)dest_c = *data++;
		    dest_c += 4;
		    --viscount_dwords;
		}
	    }

	    // Move (max. 3) single bytes?
	    if((--viscount_bytes)<0) goto rclip_finalcheck;
	    pixelblock = *data++;

	    *dest_c++ = pixelblock;

	    if((--viscount_bytes)<0) goto rclip_finalcheck;
	    pixelblock >>= 8;
	    *dest_c++ = pixelblock;

	    if((--viscount_bytes)<0) goto rclip_finalcheck;
	    pixelblock >>= 8;
	    *dest_c++ = pixelblock;

	rclip_finalcheck:
	    if(widthcount<=0) break;	// Clip and go do the next line
	};

	dest_c = dest_old + screenwidth;

    }while(--height);
}




/*
	Left clipping, seems OK.

	This code continues where the unclipped putsprite code cannot.

	PRE:
	- X and Y coords have been adjusted for sprite centering.
	- Vertical clipping adjustments have been performed, if necessary.
*/
static void ps_leftclip(int x, int y, int width, int height, unsigned long *linetab, unsigned char *dest_c){

    int viscount, viscount_dwords, viscount_bytes;
    unsigned long pixelblock;
    unsigned long *data = linetab;
    int clipcount;
    unsigned char * charptr;


    // Still visible?
    if(-x >= width) return;


    // Check right clipping
    if(x+width > screenwidth){
	   ps_bothclip(x, y, width, height, data, dest_c);
	   return;
    }


    // I know the x coord is negative! But this is OK anyway.
    dest_c += y*screenwidth + x;

    do{
	// Get ready to draw a line
	data = linetab + (*linetab / 4);
	++linetab;


	// The following for-loop contains code to skip the left offscreen
	// area of the sprite. Complex shit!

	clipcount = x;
	for(;;){

	    clipcount += *data;		// Add clearcount
	    ++data;
	    if(clipcount >= 0){
		// Reached on-screen area!
		// Move the screen pointer to the left-most pixel on-screen.
		dest_c -= x;
		// Now add the clipcount overflow.
		dest_c += clipcount;
		goto lclip_entry1;
	    }


	    viscount = *data;			// Get viscount
	    ++data;


	    if(viscount <= 0) goto lclip_nextline_entry;	// Reached EOL?


	    if((viscount + clipcount) <= 0){
		// These pixels can be safely skipped.
		clipcount += viscount;

		// Skip the pixels.
		viscount += 3;
		data += viscount>>2;

		continue;
	    }


	    // The pixel run crosses the screen boundary!

	    charptr = (void*)data;

	    // Locate aligned position for source data pointer, past the pixels.
	    data += (viscount+3) >> 2;

	    charptr -= clipcount;	// Find pixels to draw
	    viscount += clipcount;	// How many pixels left to draw?

	    // Move the screen pointer to the left-most pixel on-screen.
	    dest_c -= x;

	    // Draw this run's remaining pixels to the screen.
	    // Dest-aligned, will copy DWORDs in ASM.
	    do{
		*dest_c = *charptr;
		++dest_c;
		++charptr;
	    }while(--viscount);

	    break;		// Continue with normal draw loop.
	}


	// Normal draw loop...
	for(;;){
	    dest_c += *data;		// Add clearcount
	    ++data;

	lclip_entry1:

	    viscount = *data;
	    ++data;

	    if(viscount<=0) break;

	    viscount_dwords = viscount >> 2;
	    viscount_bytes = viscount & 3;
#if 0
	    // Move DWORDS
	    if ((int)dest_c & 3) {
		while(viscount_dwords){
		    pixelblock = *data++;
		    dest_c[0] = pixelblock;
		    pixelblock >>= 8;
		    dest_c[1] = pixelblock;
		    pixelblock >>= 8;
		    dest_c[2] = pixelblock;
		    pixelblock >>= 8;
		    dest_c[3] = pixelblock;
		    dest_c += 4;
		    --viscount_dwords;
		}
	    } else
#endif
    	{
		while(viscount_dwords){
		    *(unsigned long*)dest_c = *data++;
		    dest_c += 4;
		    --viscount_dwords;
		}
	    }

	    // Move (max. 3) single bytes?
	    if((--viscount_bytes)<0) continue;
	    pixelblock = *data++;

	    *dest_c++ = pixelblock;

	    if((--viscount_bytes)<0) continue;
	    pixelblock >>= 8;
	    *dest_c++ = pixelblock;

	    if((--viscount_bytes)<0) continue;
	    pixelblock >>= 8;
	    *dest_c++ = pixelblock;
	};

    lclip_nextline_entry:

	dest_c += screenwidth;

    }while(--height);
}


// This seems OK...
void putsprite(int x, int y, s_sprite *frame, s_screen *screen){

	unsigned long *linetab;
	int width, height;
	int viscount, viscount_dwords, viscount_bytes;
	unsigned long pixelblock;
	unsigned long *data;
	unsigned char *dest_c;


	// Get screen size
	screenwidth = screen->width;
	screenheight = screen->height;

	dest_c = screen->data;


	// Adjust coords for centering
	x -= frame->centerx;
	y -= frame->centery;


	// Get sprite dimensions
	width = frame->width;
	height = frame->height;


	// Check if sprite dimensions are valid
	if(width<=0 || height<=0) return;


	// Init line table pointer
	linetab = (void*)frame->data;


	// Check clipping, vertical first
	if(y < 0){
		// Clip top
		height += y;		// Make sprite shorter
		if(height <=0 ) return;
		linetab -= y;		// Advance -y lines
		y = 0;
	}
	if(y+height > screenheight){
		// Clip bottom (make sprite shorter)
		height = screenheight - y;
		if(height <= 0) return;
	}
	if(x < 0){
		// Clip left
		ps_leftclip(x, y, width, height, linetab, dest_c);
		return;
	}
	if(x+width > screenwidth){
		// Clip right
		ps_rightclip(x, y, width, height, linetab, dest_c);
		return;
	}


	dest_c += y*screenwidth + x;


	do{
		// Get ready to draw a line
		data = linetab + (*linetab / 4);
		++linetab;

		for(;;){
			dest_c += *data;		// Add clearcount
			++data;

			viscount = *data;
			++data;

			if(viscount<=0) break;

			viscount_dwords = viscount >> 2;
			viscount_bytes = viscount & 3;

#if 0
			if ((int)dest_c & 3) {
				while(viscount_dwords) {
					pixelblock = *data++;
					dest_c[0] = pixelblock;
					pixelblock >>= 8;
					dest_c[1] = pixelblock;
					pixelblock >>= 8;
					dest_c[2] = pixelblock;
					pixelblock >>= 8;
					dest_c[3] = pixelblock;
					dest_c += 4;
					--viscount_dwords;
				}
			} else 
#endif
			{
				// Move DWORDS
				while(viscount_dwords){
					*(unsigned long*)dest_c = *data++;
					dest_c += 4;
					--viscount_dwords;
				}
			}

			// Move (max. 3) single bytes?
			if((--viscount_bytes)<0) continue;
			pixelblock = *data++;

			*dest_c++ = pixelblock;

			if((--viscount_bytes)<0) continue;
			pixelblock >>= 8;
			*dest_c++ = pixelblock;

			if((--viscount_bytes)<0) continue;
			pixelblock >>= 8;
			*dest_c++ = pixelblock;
		};

		dest_c += screenwidth;

	}while(--height);
}






// To know size of sprite without actually creating one
unsigned int fakey_encodesprite(s_bitmap *bitmap){
	unsigned int width, height, s, d;
	unsigned int vispix, xpos, ypos, pos;

	if(bitmap->width <= 0 || bitmap->height <= 0){
		// Image is empty (or bad), return size of empty sprite
		return 8*4;
	}

	width = bitmap->width;
	height = bitmap->height;

	xpos=0;
	ypos=0;

	s = 0;			// Source pixels start pos
	d = 16+(height<<2);	// Destination pixels start pos

ctn:
	while(ypos<height){
		while(bitmap->data[s]==TRANSPARENT_IDX){
			++s;
			++xpos;
			if(xpos==width){
				d += 8;
				xpos = 0;
				++ypos;
				goto ctn;		// Re-enter loop
			}
		}

		d+=4;

		pos = s;
		vispix = 0;

		while(bitmap->data[pos]!=TRANSPARENT_IDX && xpos<width){
			++vispix;
			++xpos;
			++pos;
		}
		d+=4;

		d += vispix;
		s += vispix;

		// Add alignment
		while(d&3) d++;

		if(xpos>=width){		// Stopped at end of line?
			d += 8;
			xpos = 0;
			++ypos;
		}
	}
	return d;		// Return size of encoded sprite
}




// Bitmap-to-sprite converter, now screensize-independent!
unsigned int encodesprite(int centerx, int centery, s_bitmap *bitmap, s_sprite *dest){

	unsigned int width, height, s, d;
	unsigned int vispix, transpix, xpos, ypos, pos;
	unsigned char *cdest = (void*)dest->data;
	long *linetab = (void*)dest->data;


	if(bitmap->width <= 0 || bitmap->height <= 0){
		// Image is empty (or bad), create an empty sprite
		dest->centerx = 0;
		dest->centery = 0;
		dest->width = 0;
		dest->height = 0;
		for(d=0; d<4; d++) dest->data[d] = 0;
		return 8*4;
	}

	width = bitmap->width;
	height = bitmap->height;


	dest->centerx = centerx;
	dest->centery = centery;
	dest->width = width;
	dest->height = height;

	xpos = 0;
	ypos = 0;

	s = 0;			// Source pixels start pos
	d = height<<2;		// Destination pixels start pos

ctn:

	while(ypos < height){

		if(xpos==0){
			// Update line offset table
			linetab[ypos] = d-(ypos<<2);
		}


		transpix = 0;

		while(bitmap->data[s]==TRANSPARENT_IDX){
			++transpix;
			++s;
			++xpos;
			if(xpos==width){
				transpix -= width;	// Return to startpos of line
				dest->data[d>>2] = transpix;
				d += 4;
				dest->data[d>>2] = 0;	// EOL marker (0 visible)
				d += 4;
				xpos = 0;
				++ypos;
				goto ctn;		// Re-enter loop
			}
		}

		dest->data[d>>2] = transpix;
		d+=4;


		pos = s;
		vispix = 0;
		while(bitmap->data[pos]!=TRANSPARENT_IDX && xpos<width){
			++vispix;
			++xpos;
			++pos;
		}
		dest->data[d>>2] = vispix;	// Store visible pixel count
		d+=4;

		for(pos=0; pos<vispix; pos++){	// Copy pixels
			cdest[d++] = bitmap->data[s++];
		}

		while(d&3){			// Add alignment
			cdest[d++] = 0;
		}

		if(xpos>=width){		// Stopped at end of line?
			transpix = -width;	// Back to startpos
			dest->data[d>>2] = transpix;
			d += 4;
			dest->data[d>>2] = 0;	// EOL marker (0 visible)
			d += 4;
			xpos = 0;
			++ypos;
		}
	}
	return d;		// Return size of new encoded sprite
}










// -------------------------- NEW: EFFECTS -------------------------------






/*
	Same as above, but uses translation table.
*/
static void ps_remap_bothclip(int x, int y, int width, int height, unsigned long *linetab, unsigned char *dest_c, unsigned char *lut){

    int viscount, viscount_bytes;
    unsigned long *data = linetab;
    int clipcount;
    unsigned char * charptr;
    int widthcount;
    unsigned char *dest_old;


    // I know the x coord is negative! But this is OK anyway.
    dest_c += y*screenwidth + x;

    do{
	// Get ready to draw a line
	data = linetab + (*linetab / 4);
	++linetab;

	dest_old = dest_c;
	widthcount = screenwidth;

	// The following for-loop contains code to skip the left offscreen
	// area of the sprite. Complex shit, especially since it also has
	// to clip on the right side now!

	clipcount = x;
	for(;;){

	    clipcount += *data;		// Add clearcount
	    ++data;
	    if(clipcount >= 0){
		// Reached on-screen area?
		// Check if we've passed the screen entirely!
		if(clipcount >= screenwidth) goto bclip_nextline_entry;

		// Move the screen pointer to the left-most pixel on-screen.
		dest_c -= x;
		// Now add the clipcount overflow.
		dest_c += clipcount;

		// Keep the width counter in check.
		widthcount -= clipcount;

		goto bclip_entry1;
	    }


	    viscount = *data;			// Get viscount
	    ++data;


	    if(viscount <= 0) goto bclip_nextline_entry;	// Reached EOL?


	    if((viscount + clipcount) <= 0){
		// These pixels can be safely skipped.
		clipcount += viscount;

		// Skip the pixels.
		viscount += 3;
		data += viscount>>2;

		continue;
	    }


	    // The pixel run crosses the screen boundary!

	    charptr = (void*)data;

	    // Locate aligned position for frame pointer, past the pixels.
	    data += (viscount+3) >> 2;

	    // Move the screen pointer to the left-most pixel on-screen.
	    dest_c -= x;


	    charptr -= clipcount;	// Find pixels to draw
	    viscount += clipcount;	// How many pixels left to draw?

	    if(viscount >= screenwidth){
		// Fill entire width of screen
		viscount = screenwidth;
		do{
		    *dest_c = lut[((int)(*charptr)) & 0xFF];
		    ++dest_c;
		    ++charptr;
		}while(--viscount);

		goto bclip_nextline_entry;
	    }

	    // Keep the width counter updated.
	    widthcount -= viscount;

	    // Draw this run's remaining pixels to the screen.
	    do{
		*dest_c = lut[((int)(*charptr)) & 0xFF];
		++dest_c;
		++charptr;
	    }while(--viscount);

	    break;		// Continue with right-clipped draw loop.
	}


	for(;;){
	    dest_c += *data;		// Add clearcount
	    widthcount -= *data;
	    if(widthcount<=0) break;	// Clip and go do the next line
	    ++data;

	bclip_entry1:

	    viscount = *data;
	    ++data;


	    if(viscount<=0) break;	// EOL


	    // If too many pixels to draw, cap run.
	    widthcount -= viscount;
	    if(widthcount<0) viscount += widthcount;

	    viscount_bytes = viscount;
	    charptr = (void*)data;
	    do{
		*dest_c = lut[((int)(*charptr)) & 0xFF];
		++dest_c;
		++charptr;
	    }while(--viscount_bytes);
	    data += (viscount+3)>>2;


	    if(widthcount<=0) break;	// Clip and go do the next line
	};


    bclip_nextline_entry:

	dest_c = dest_old + screenwidth;

    }while(--height);
}





/*
	Right-side clipping.
	Same as above, blah blah.
*/
static void ps_remap_rightclip(int x, int y, int width, int height, unsigned long *linetab, unsigned char *dest_c, unsigned char *lut){

    int viscount, viscount_bytes;
    unsigned long *data = linetab;
    int widthcount;
    unsigned char *dest_old;
    unsigned char *charptr;


    // Still visible?
    if(x >= screenwidth) return;


    // No need to check left-side clipping, should be done by leftclip code!


    // Get the screen pointer ready...
    dest_c += y*screenwidth + x;


    // Calculate the remaining width.
    width = screenwidth - x;


    do{
	// Get ready to draw a line
	data = linetab + (*linetab / 4);
	++linetab;

	dest_old = dest_c;
	widthcount = width;

	for(;;){
	    dest_c += *data;		// Add clearcount
	    widthcount -= *data;
	    if(widthcount<=0) break;	// Clip and go do the next line
	    ++data;

	    viscount = *data;
	    ++data;


	    if(viscount<=0) break;	// EOL


	    // If too many pixels to draw, cap run.
	    widthcount -= viscount;
	    if(widthcount<0) viscount += widthcount;


	    viscount_bytes = viscount;
	    charptr = (void*)data;
	    do{
		*dest_c = lut[((int)(*charptr)) & 0xFF];
		++dest_c;
		++charptr;
	    }while(--viscount_bytes);
	    data += (viscount+3)>>2;


	    if(widthcount<=0) break;	// Clip and go do the next line
	};

	dest_c = dest_old + screenwidth;

    }while(--height);
}




/*
	Left clipping for remap.
*/
static void ps_remap_leftclip(int x, int y, int width, int height, unsigned long *linetab, unsigned char *dest_c, unsigned char *lut){

    int viscount, viscount_bytes;
    unsigned long *data = linetab;
    int clipcount;
    unsigned char * charptr;


    // Still visible?
    if(-x >= width) return;


    // Check right clipping
    if(x+width > screenwidth){
	ps_remap_bothclip(x, y, width, height, data, dest_c, lut);
	return;
    }


    // I know the x coord is negative! But this is OK anyway.
    dest_c += y*screenwidth + x;

    do{
	// Get ready to draw a line
	data = linetab + (*linetab / 4);
	++linetab;


	// The following for-loop contains code to skip the left offscreen
	// area of the sprite. Complex shit!

	clipcount = x;
	for(;;){

	    clipcount += *data;		// Add clearcount
	    ++data;
	    if(clipcount >= 0){
		// Reached on-screen area!
		// Move the screen pointer to the left-most pixel on-screen.
		dest_c -= x;
		// Now add the clipcount overflow.
		dest_c += clipcount;
		goto lclip_entry1;
	    }


	    viscount = *data;			// Get viscount
	    ++data;


	    if(viscount <= 0) goto lclip_nextline_entry;	// Reached EOL?


	    if((viscount + clipcount) <= 0){
		// These pixels can be safely skipped.
		clipcount += viscount;

		// Skip the pixels.
		viscount += 3;
		data += viscount>>2;

		continue;
	    }


	    // The pixel run crosses the screen boundary!

	    charptr = (void*)data;

	    // Locate aligned position for source data pointer, past the pixels.
	    data += (viscount+3) >> 2;

	    charptr -= clipcount;	// Find pixels to draw
	    viscount += clipcount;	// How many pixels left to draw?

	    // Move the screen pointer to the left-most pixel on-screen.
	    dest_c -= x;

	    // Draw this run's remaining pixels to the screen.
	    do{
		*dest_c = lut[((int)(*charptr)) & 0xFF];
		++dest_c;
		++charptr;
	    }while(--viscount);

	    break;		// Continue with normal draw loop.
	}


	// Normal draw loop...
	for(;;){
	    dest_c += *data;		// Add clearcount
	    ++data;

	lclip_entry1:

	    viscount = *data;
	    ++data;

	    if(viscount<=0) break;

	    viscount_bytes = viscount;
	    charptr = (void*)data;
	    do{
		*dest_c = lut[((int)(*charptr)) & 0xFF];
		++dest_c;
		++charptr;
	    }while(--viscount_bytes);
	    data += (viscount+3)>>2;
	};

    lclip_nextline_entry:

	dest_c += screenwidth;

    }while(--height);
}




// Putsprite function with remapping
void putsprite_remap(int x, int y, s_sprite *frame, s_screen *screen, unsigned char *lut){

	unsigned long *linetab;
	int width, height;
	int viscount, viscount_bytes;
	unsigned long *data;
	unsigned char *dest_c;
	unsigned char *charptr;


	// Get screen size
	screenwidth = screen->width;
	screenheight = screen->height;

	dest_c = screen->data;


	// Adjust coords for centering
	x -= frame->centerx;
	y -= frame->centery;


	// Get sprite dimensions
	width = frame->width;
	height = frame->height;


	// Check if sprite dimensions are valid
	if(width<=0 || height<=0) return;


	// Init line table pointer
	linetab = (void*)frame->data;


	// Check clipping, vertical first
	if(y < 0){
		// Clip top
		height += y;		// Make sprite shorter
		if(height <=0 ) return;
		linetab -= y;		// Advance -y lines
		y = 0;
	}
	if(y+height > screenheight){
		// Clip bottom (make sprite shorter)
		height = screenheight - y;
		if(height <= 0) return;
	}
	if(x < 0){
		// Clip left
		ps_remap_leftclip(x, y, width, height, linetab, dest_c, lut);
		return;
	}
	if(x+width > screenwidth){
		// Clip right
		ps_remap_rightclip(x, y, width, height, linetab, dest_c, lut);
		return;
	}


	dest_c += y*screenwidth + x;


	do{
		// Get ready to draw a line
		data = linetab + (*linetab / 4);
		++linetab;

		for(;;){
			dest_c += *data;		// Add clearcount
			++data;

			viscount = *data;
			++data;

			if(viscount<=0) break;

			viscount_bytes = viscount;
			charptr = (void*)data;
			do{
				*dest_c = lut[((int)(*charptr)) & 0xFF];
				++dest_c;
				++charptr;
			}while(--viscount_bytes);
			data += (viscount+3)>>2;
		};

		dest_c += screenwidth;

	}while(--height);
}








/*
	Now with blending...
*/
static void ps_blend_bothclip(int x, int y, int width, int height, unsigned long *linetab, unsigned char *dest_c, unsigned char *lut){

    int viscount, viscount_bytes;
    unsigned long *data = linetab;
    int clipcount;
    unsigned char * charptr;
    int widthcount;
    unsigned char *dest_old;


    // I know the x coord is negative! But this is OK anyway.
    dest_c += y*screenwidth + x;

    do{
	// Get ready to draw a line
	data = linetab + (*linetab / 4);
	++linetab;

	dest_old = dest_c;
	widthcount = screenwidth;

	// The following for-loop contains code to skip the left offscreen
	// area of the sprite. Complex shit, especially since it also has
	// to clip on the right side now!

	clipcount = x;
	for(;;){

	    clipcount += *data;		// Add clearcount
	    ++data;
	    if(clipcount >= 0){
		// Reached on-screen area?
		// Check if we've passed the screen entirely!
		if(clipcount >= screenwidth) goto bclip_nextline_entry;

		// Move the screen pointer to the left-most pixel on-screen.
		dest_c -= x;
		// Now add the clipcount overflow.
		dest_c += clipcount;

		// Keep the width counter in check.
		widthcount -= clipcount;

		goto bclip_entry1;
	    }


	    viscount = *data;			// Get viscount
	    ++data;


	    if(viscount <= 0) goto bclip_nextline_entry;	// Reached EOL?


	    if((viscount + clipcount) <= 0){
		// These pixels can be safely skipped.
		clipcount += viscount;

		// Skip the pixels.
		viscount += 3;
		data += viscount>>2;

		continue;
	    }


	    // The pixel run crosses the screen boundary!

	    charptr = (void*)data;

	    // Locate aligned position for frame pointer, past the pixels.
	    data += (viscount+3) >> 2;

	    // Move the screen pointer to the left-most pixel on-screen.
	    dest_c -= x;


	    charptr -= clipcount;	// Find pixels to draw
	    viscount += clipcount;	// How many pixels left to draw?

	    if(viscount >= screenwidth){
		// Fill entire width of screen
		viscount = screenwidth;
		do{
		    *dest_c = lut[(*charptr<<8) | *dest_c];
		    ++dest_c;
		    ++charptr;
		}while(--viscount);

		goto bclip_nextline_entry;
	    }

	    // Keep the width counter updated.
	    widthcount -= viscount;

	    // Draw this run's remaining pixels to the screen.
	    do{
		*dest_c = lut[(*charptr<<8) | *dest_c];
		++dest_c;
		++charptr;
	    }while(--viscount);

	    break;		// Continue with right-clipped draw loop.
	}


	for(;;){
	    dest_c += *data;		// Add clearcount
	    widthcount -= *data;
	    if(widthcount<=0) break;	// Clip and go do the next line
	    ++data;

	bclip_entry1:

	    viscount = *data;
	    ++data;


	    if(viscount<=0) break;	// EOL


	    // If too many pixels to draw, cap run.
	    widthcount -= viscount;
	    if(widthcount<0) viscount += widthcount;

	    viscount_bytes = viscount;
	    charptr = (void*)data;
	    do{
		*dest_c = lut[(*charptr<<8) | *dest_c];
		++dest_c;
		++charptr;
	    }while(--viscount_bytes);
	    data += (viscount+3)>>2;


	    if(widthcount<=0) break;	// Clip and go do the next line
	};


    bclip_nextline_entry:

	dest_c = dest_old + screenwidth;

    }while(--height);
}





/*
	Right-side clipping.
	Same as above, blah blah.
*/
static void ps_blend_rightclip(int x, int y, int width, int height, unsigned long *linetab, unsigned char *dest_c, unsigned char *lut){

    int viscount, viscount_bytes;
    unsigned long *data = linetab;
    int widthcount;
    unsigned char *dest_old;
    unsigned char *charptr;


    // Still visible?
    if(x >= screenwidth) return;


    // No need to check left-side clipping, should be done by leftclip code!


    // Get the screen pointer ready...
    dest_c += y*screenwidth + x;


    // Calculate the remaining width.
    width = screenwidth - x;


    do{
	// Get ready to draw a line
	data = linetab + (*linetab / 4);
	++linetab;

	dest_old = dest_c;
	widthcount = width;

	for(;;){
	    dest_c += *data;		// Add clearcount
	    widthcount -= *data;
	    if(widthcount<=0) break;	// Clip and go do the next line
	    ++data;

	    viscount = *data;
	    ++data;


	    if(viscount<=0) break;	// EOL


	    // If too many pixels to draw, cap run.
	    widthcount -= viscount;
	    if(widthcount<0) viscount += widthcount;


	    viscount_bytes = viscount;
	    charptr = (void*)data;
	    do{
		*dest_c = lut[(*charptr<<8) | *dest_c];
		++dest_c;
		++charptr;
	    }while(--viscount_bytes);
	    data += (viscount+3)>>2;


	    if(widthcount<=0) break;	// Clip and go do the next line
	};

	dest_c = dest_old + screenwidth;

    }while(--height);
}




/*
	Left clipping for remap.
*/
static void ps_blend_leftclip(int x, int y, int width, int height, unsigned long *linetab, unsigned char *dest_c, unsigned char *lut){

    int viscount, viscount_bytes;
    unsigned long *data = linetab;
    int clipcount;
    unsigned char * charptr;


    // Still visible?
    if(-x >= width) return;


    // Check right clipping
    if(x+width > screenwidth){
	ps_blend_bothclip(x, y, width, height, data, dest_c, lut);
	return;
    }


    // I know the x coord is negative! But this is OK anyway.
    dest_c += y*screenwidth + x;

    do{
	// Get ready to draw a line
	data = linetab + (*linetab / 4);
	++linetab;


	// The following for-loop contains code to skip the left offscreen
	// area of the sprite. Complex shit!

	clipcount = x;
	for(;;){

	    clipcount += *data;		// Add clearcount
	    ++data;
	    if(clipcount >= 0){
		// Reached on-screen area!
		// Move the screen pointer to the left-most pixel on-screen.
		dest_c -= x;
		// Now add the clipcount overflow.
		dest_c += clipcount;
		goto lclip_entry1;
	    }


	    viscount = *data;			// Get viscount
	    ++data;


	    if(viscount <= 0) goto lclip_nextline_entry;	// Reached EOL?


	    if((viscount + clipcount) <= 0){
		// These pixels can be safely skipped.
		clipcount += viscount;

		// Skip the pixels.
		viscount += 3;
		data += viscount>>2;

		continue;
	    }


	    // The pixel run crosses the screen boundary!

	    charptr = (void*)data;

	    // Locate aligned position for source data pointer, past the pixels.
	    data += (viscount+3) >> 2;

	    charptr -= clipcount;	// Find pixels to draw
	    viscount += clipcount;	// How many pixels left to draw?

	    // Move the screen pointer to the left-most pixel on-screen.
	    dest_c -= x;

	    // Draw this run's remaining pixels to the screen.
	    do{
		*dest_c = lut[(*charptr<<8) | *dest_c];
		++dest_c;
		++charptr;
	    }while(--viscount);

	    break;		// Continue with normal draw loop.
	}


	// Normal draw loop...
	for(;;){
	    dest_c += *data;		// Add clearcount
	    ++data;

	lclip_entry1:

	    viscount = *data;
	    ++data;

	    if(viscount<=0) break;

	    viscount_bytes = viscount;
	    charptr = (void*)data;
	    do{
		*dest_c = lut[(*charptr<<8) | *dest_c];
		++dest_c;
		++charptr;
	    }while(--viscount_bytes);
	    data += (viscount+3)>>2;
	};

    lclip_nextline_entry:

	dest_c += screenwidth;

    }while(--height);
}




// Putsprite function with remapping
void putsprite_blend(int x, int y, s_sprite *frame, s_screen *screen, unsigned char *lut){

	unsigned long *linetab;
	int width, height;
	int viscount, viscount_bytes;
	unsigned long *data;
	unsigned char *dest_c;
	unsigned char *charptr;


	// Get screen size
	screenwidth = screen->width;
	screenheight = screen->height;

	dest_c = screen->data;


	// Adjust coords for centering
	x -= frame->centerx;
	y -= frame->centery;


	// Get sprite dimensions
	width = frame->width;
	height = frame->height;


	// Check if sprite dimensions are valid
	if(width<=0 || height<=0) return;


	// Init line table pointer
	linetab = (void*)frame->data;


	// Check clipping, vertical first
	if(y < 0){
		// Clip top
		height += y;		// Make sprite shorter
		if(height <=0 ) return;
		linetab -= y;		// Advance -y lines
		y = 0;
	}
	if(y+height > screenheight){
		// Clip bottom (make sprite shorter)
		height = screenheight - y;
		if(height <= 0) return;
	}
	if(x < 0){
		// Clip left
		ps_blend_leftclip(x, y, width, height, linetab, dest_c, lut);
		return;
	}
	if(x+width > screenwidth){
		// Clip right
		ps_blend_rightclip(x, y, width, height, linetab, dest_c, lut);
		return;
	}


	dest_c += y*screenwidth + x;


	do{
		// Get ready to draw a line
		data = linetab + (*linetab / 4);
		++linetab;

		for(;;){
			dest_c += *data;		// Add clearcount, startx
			++data;

			viscount = *data;       // length
			++data;

			if(viscount<=0) break;

			viscount_bytes = viscount;
			charptr = (void*)data;
			do{
				*dest_c = lut[(*charptr<<8) | *dest_c];
				++dest_c;
				++charptr;
			}while(--viscount_bytes);
			data += (viscount+3)>>2;  // next block
		};

		dest_c += screenwidth;

	}while(--height);
}

static unsigned char fillcolor=0;

unsigned char remapcolor(unsigned char* table, unsigned char color, unsigned char unused)
{
    return table[color];
}

unsigned char blendcolor(unsigned char* table, unsigned char color1, unsigned char color2)
{
    if(!table) return color1;
    return table[color1<<8|color2];
}

unsigned char blendfillcolor(unsigned char* table, unsigned char unused, unsigned char color)
{
    if(!table) return fillcolor;
    return table[fillcolor<<8|color];
}


//--------------------------------------------------------------------------------------

// x: centerx on screen cx: centerx of this line
static void scaleline(int x, int cx, int width, unsigned long *linetab, unsigned char *dest_c, unsigned char *lut, transpixelfunc fp, unsigned int scale)
{
    unsigned long *data = linetab;
    int dx, i, d;
    unsigned char * charptr;
    unsigned int scale_d=0, old_scale_d=0, cleft, cwidth;

    dx = x - ((cx*scale)>>8); //draw start x
    
//    if(dx>=screenwidth || dx+((width*scale)>>8)<0) return; it should be check in the function that called this

    dest_c += dx;

	// Get ready to draw a line
	data = linetab + (*linetab / 4);
    
    for(;;)
    {
        cleft = *data++;
        cwidth = *data++;
        if(cwidth<=0) return; // end of line
        //scale_s += cleft<<8;     // src scale, 256
        charptr = (unsigned char*)data;
        data += (cwidth+3)>>2; // skip some bytes to next block
        scale_d += cleft*scale;  // dest scale, scale
        dx += cleft;
        if(dx>=screenwidth) return; // out of right border? exit
        d = scale_d - old_scale_d;
        if(d >= 256) // skip some blank pixels
        {
            dest_c += d>>8;
            old_scale_d = (scale_d>>8)<<8;
        }
        while(cwidth--) // draw these pixels
        {
            scale_d += scale;
            d = scale_d - old_scale_d; // count scale added
            if(d >= 256) // > 1pixel, so draw these
            {
                for(i=d>>8; i>0; i--) // draw a pixel
                {
                    if(dx>=0) // pass left border? 
                    {
                        *dest_c = fp(lut, *charptr, *dest_c);
                    }
                    if(++dx>=screenwidth) return; // out of right border? exit
                    dest_c++; // position move to right one pixel
                }
                old_scale_d = (scale_d>>8)<<8; //truncate those less than 256
            }
            charptr++; // src ptr move right one pixel
        }
    }
     
}




// scalex scaley flipy ... 
void putsprite_ex(int x, int y, s_sprite *frame, s_screen *screen, s_drawmethod* drawmethod)
{
	unsigned long *linetab;
	int height, dx, d, i, cy;
	unsigned char *dest_c;
	int scale=0, old_scale=0;

    if(!drawmethod)
    {
        putsprite(x, y, frame, screen);
        return;
    }
    
	if(!drawmethod->scalex || !drawmethod->scaley) return; // zero size
	
	screenheight = screen->height;
	screenwidth = screen->width;
	
    dx = x - ((frame->centerx*drawmethod->scalex)>>8); //draw start x
    
    if(dx>=screenwidth || dx+((frame->width*drawmethod->scalex)>>8)<0) return; // out of left or right border
	
	cy = y;
    height = frame->height;
    linetab = (unsigned long*)(frame->data);
    
    if(drawmethod->fillcolor) fillcolor = drawmethod->fillcolor;
    
    // flip in y direction, from centery
    if(drawmethod->flipy)
    {
        y += (frame->centery*drawmethod->scaley)>>8; // lowest
        dest_c = (unsigned char*)(screen->data)+y*screenwidth;
        if(y<0) return;
     
        while(height--)
        {
            scale += drawmethod->scaley;
            d = scale - old_scale; // count scale added
            if(d >= 256) // > 1pixel, so draw these
            {
                for(i=d>>8; i>0; i--) // draw a line
                {
                    if(y<screenheight) // pass lower border? 
                    {
                        scaleline(x+((drawmethod->shiftx*(cy-y))/256), frame->centerx, frame->width, linetab, dest_c, drawmethod->table, drawmethod->fp, drawmethod->scalex);
                    }
                    if(--y<0) return; // out of lower border? exit
                    dest_c -= screenwidth; // position move down one line
                }
                old_scale = (scale>>8)<<8; //truncate those less than 256
            }
            linetab++; //src line shift
        }
    }
    else // un-flipped version
    {
        y -= (frame->centery*drawmethod->scaley)>>8; // topmost
        dest_c = (unsigned char*)(screen->data)+y*screenwidth;
        if(y>=screenheight) return;
     
        while(height--)
        {
            scale += drawmethod->scaley;
            d = scale - old_scale; // count scale added
            if(d >= 256) // > 1pixel, so draw these
            {
                for(i=d>>8; i>0; i--) // draw a line
                {
                    if(y>=0) // pass upper border? 
                    {
                        scaleline(x+((drawmethod->shiftx*(y-cy))/256), frame->centerx, frame->width, linetab, dest_c, drawmethod->table, drawmethod->fp, drawmethod->scalex);
                    }
                    if(++y>=screenheight) return; // out of lower border? exit
                    dest_c += screenwidth; // position move down one line
                }
                old_scale = (scale>>8)<<8; //truncate those less than 256
            }
            linetab++; //src line shift
        }
     }
}
