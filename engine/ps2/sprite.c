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

#include "ps2port.h"

#include "types.h"



#define		TRANSPARENT_IDX		0x00




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
static void ps_bothclip(int x, int y, int width, int height, unsigned gamelib_long *linetab, char *dest_c){

    int viscount, viscount_dwords, viscount_bytes;
//    unsigned gamelib_long pixelblock;
    unsigned gamelib_long *dest_l;
    unsigned gamelib_long *data = linetab;
    int clipcount;
    char * charptr;
    int widthcount;
    char *dest_old;


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

	    // Move DWORDS
	    dest_l = (void*)dest_c;
	    while(viscount_dwords){
                ((unsigned char*)dest_l)[0] = ((unsigned char*)data)[0];
                ((unsigned char*)dest_l)[1] = ((unsigned char*)data)[1];
                ((unsigned char*)dest_l)[2] = ((unsigned char*)data)[2];
                ((unsigned char*)dest_l)[3] = ((unsigned char*)data)[3];

//		*dest_l = *data;
		++data;
		++dest_l;
		--viscount_dwords;
	    }
	    dest_c = (void*)dest_l;

	    // Move (max. 3) single bytes?
	    if((--viscount_bytes)<0) goto bclip_finalcheck;
	    ++data;

	    *dest_c = ((unsigned char*)data)[0-4];
	    ++dest_c;

	    if((--viscount_bytes)<0) goto bclip_finalcheck;
	    *dest_c = ((unsigned char*)data)[1-4];
	    ++dest_c;

	    if((--viscount_bytes)<0) goto bclip_finalcheck;
	    *dest_c = ((unsigned char*)data)[2-4];
	    ++dest_c;

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
static void ps_rightclip(int x, int y, int width, int height, unsigned gamelib_long *linetab, char *dest_c){

    int viscount, viscount_dwords, viscount_bytes;
 //   unsigned gamelib_long pixelblock;
    unsigned gamelib_long *dest_l;
    unsigned gamelib_long *data = linetab;
    int widthcount;
    char *dest_old;


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

	    // Move DWORDS
	    dest_l = (void*)dest_c;
	    while(viscount_dwords){
                ((unsigned char*)dest_l)[0] = ((unsigned char*)data)[0];
                ((unsigned char*)dest_l)[1] = ((unsigned char*)data)[1];
                ((unsigned char*)dest_l)[2] = ((unsigned char*)data)[2];
                ((unsigned char*)dest_l)[3] = ((unsigned char*)data)[3];
//		*dest_l = *data;
		++data;
		++dest_l;
		--viscount_dwords;
	    }
	    dest_c = (void*)dest_l;

	    // Move (max. 3) single bytes?
	    if((--viscount_bytes)<0) goto rclip_finalcheck;
//	    pixelblock = *data;
	    ++data;

	    *dest_c = ((unsigned char*)data)[0-4];
	    ++dest_c;

	    if((--viscount_bytes)<0) goto rclip_finalcheck;
//	    pixelblock >>= 8;
	    *dest_c = ((unsigned char*)data)[1-4];
	    ++dest_c;

	    if((--viscount_bytes)<0) goto rclip_finalcheck;
//	    pixelblock >>= 8;
	    *dest_c = ((unsigned char*)data)[2-4];
	    ++dest_c;

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
static void ps_leftclip(int x, int y, int width, int height, unsigned gamelib_long *linetab, char *dest_c){

    int viscount, viscount_dwords, viscount_bytes;
//    unsigned gamelib_long pixelblock;
    unsigned gamelib_long *dest_l;
    unsigned gamelib_long *data = linetab;
    int clipcount;
    char * charptr;


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

	    // Move DWORDS
	    dest_l = (void*)dest_c;
	    while(viscount_dwords){
                ((unsigned char*)dest_l)[0] = ((unsigned char*)data)[0];
                ((unsigned char*)dest_l)[1] = ((unsigned char*)data)[1];
                ((unsigned char*)dest_l)[2] = ((unsigned char*)data)[2];
                ((unsigned char*)dest_l)[3] = ((unsigned char*)data)[3];
//		*dest_l = *data;
		++data;
		++dest_l;
		--viscount_dwords;
	    }
	    dest_c = (void*)dest_l;

	    // Move (max. 3) single bytes?
	    if((--viscount_bytes)<0) continue;
	    //pixelblock = *data;
	    ++data;

	    *dest_c = ((unsigned char*)data)[0-4];
	    ++dest_c;

	    if((--viscount_bytes)<0) continue;
//	    pixelblock >>= 8;
	    *dest_c = ((unsigned char*)data)[1-4];
	    ++dest_c;

	    if((--viscount_bytes)<0) continue;
//	    pixelblock >>= 8;
	    *dest_c = ((unsigned char*)data)[2-4];
	    ++dest_c;
	};

    lclip_nextline_entry:

	dest_c += screenwidth;

    }while(--height);
}




// This seems OK...
void putsprite(int x, int y, s_sprite *frame, s_screen *screen){

	unsigned gamelib_long *linetab;
	int width, height;
	int viscount, viscount_dwords, viscount_bytes;
//	unsigned gamelib_long pixelblock;
	unsigned gamelib_long *data;
	unsigned gamelib_long *dest_l;
	unsigned char *dest_c;


	// Get screen size
	screenwidth = screen->width;
	screenheight = screen->height;

	dest_c = screen->data;
	dest_l = (void*)dest_c;


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

			// Move DWORDS
			dest_l = (void*)dest_c;
			while(viscount_dwords){
			//	*dest_l = *data;
				((unsigned char*)dest_l)[0] = ((unsigned char*)data)[0];
                                ((unsigned char*)dest_l)[1] = ((unsigned char*)data)[1];
                                ((unsigned char*)dest_l)[2] = ((unsigned char*)data)[2];
                                ((unsigned char*)dest_l)[3] = ((unsigned char*)data)[3];

				++data;
				++dest_l;
				--viscount_dwords;
			}
			dest_c = (void*)dest_l;

			// Move (max. 3) single bytes?
			if((--viscount_bytes)<0) continue;
			++data;

			*dest_c = ((unsigned char*)data)[0-4];
			++dest_c;

			if((--viscount_bytes)<0) continue;
			*dest_c = ((unsigned char*)data)[1-4];
			++dest_c;

			if((--viscount_bytes)<0) continue;
			*dest_c = ((unsigned char*)data)[2-4];
			++dest_c;
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
	char *cdest = (void*)dest->data;
	gamelib_long *linetab = (void*)dest->data;


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
static void ps_remap_bothclip(int x, int y, int width, int height, unsigned gamelib_long *linetab, char *dest_c, char *lut){

    int viscount;
//   int viscount_dwords;
   int viscount_bytes;
//    unsigned gamelib_long pixelblock;
//    unsigned gamelib_long *dest_l;
    unsigned gamelib_long *data = linetab;
    int clipcount;
    char * charptr;
    int widthcount;
    char *dest_old;


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
static void ps_remap_rightclip(int x, int y, int width, int height, unsigned gamelib_long *linetab, char *dest_c, char *lut){

    int viscount, viscount_bytes;
//    unsigned gamelib_long pixelblock;
//    unsigned gamelib_long *dest_l;
    unsigned gamelib_long *data = linetab;
    int widthcount;
    char *dest_old;
    char *charptr;


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
static void ps_remap_leftclip(int x, int y, int width, int height, unsigned gamelib_long *linetab, char *dest_c, char *lut){

    int viscount, viscount_bytes;
//    unsigned gamelib_long pixelblock;
//    unsigned gamelib_long *dest_l;
    unsigned gamelib_long *data = linetab;
    int clipcount;
    char * charptr;


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
void putsprite_remap(int x, int y, s_sprite *frame, s_screen *screen, char *lut){

	unsigned gamelib_long *linetab;
	int width, height;
	int viscount, viscount_bytes;
//	unsigned gamelib_long pixelblock;
	unsigned gamelib_long *data;
	unsigned gamelib_long *dest_l;
	unsigned char *dest_c;
	char *charptr;


	// Get screen size
	screenwidth = screen->width;
	screenheight = screen->height;

	dest_c = screen->data;
	dest_l = (void*)dest_c;


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
static void ps_blend_bothclip(int x, int y, int width, int height, unsigned gamelib_long *linetab, char *dest_c, char *lut){

    int viscount, viscount_bytes;
//    unsigned gamelib_long pixelblock;
//    unsigned gamelib_long *dest_l;
    unsigned gamelib_long *data = linetab;
    int clipcount;
    char * charptr;
    int widthcount;
    char *dest_old;


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
static void ps_blend_rightclip(int x, int y, int width, int height, unsigned gamelib_long *linetab, char *dest_c, char *lut){

    int viscount, viscount_bytes;
//    unsigned gamelib_long pixelblock;
//    unsigned gamelib_long *dest_l;
    unsigned gamelib_long *data = linetab;
    int widthcount;
    char *dest_old;
    char *charptr;


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
static void ps_blend_leftclip(int x, int y, int width, int height, unsigned gamelib_long *linetab, char *dest_c, char *lut){

    int viscount, viscount_bytes;
//    unsigned gamelib_long pixelblock;
//    unsigned gamelib_long *dest_l;
    unsigned gamelib_long *data = linetab;
    int clipcount;
    char * charptr;


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
void putsprite_blend(int x, int y, s_sprite *frame, s_screen *screen, char *lut){

	unsigned gamelib_long *linetab;
	int width, height;
	int viscount, viscount_bytes;
//	unsigned gamelib_long pixelblock;
	unsigned gamelib_long *data;
	unsigned gamelib_long *dest_l;
	unsigned char *dest_c;
	char *charptr;


	// Get screen size
	screenwidth = screen->width;
	screenheight = screen->height;

	dest_c = screen->data;
	dest_l = (void*)dest_c;


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
			dest_c += *data;		// Add clearcount
			++data;

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

		dest_c += screenwidth;

	}while(--height);
}





