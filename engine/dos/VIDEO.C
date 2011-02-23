#include "types.h"
#include "asmcopy.h"
#include "vga.h"
#include "vesa.h"


s_vram current_videomode;




int video_set_mode(int width, int height, int unused){
	static int vesa_inited = 0;
	int m;


	if(width<1 || height<1){
		vga_setmode(TEXTMODE);
		current_videomode.width = 0;
		current_videomode.height = 0;
		current_videomode.planar = 0;
		current_videomode.banked = 0;
		VESA_exit();
		vesa_inited = 0;
		return 1;
	}

	if(!vesa_inited){
		vesa_inited = VESA_init();
	}

	// First try to find a VESA mode
	m = VESA_findmode(width, height, 8, 1);
	if(m>-1){
		current_videomode.data = VESA_setmode(m);
		if(current_videomode.data){
			current_videomode.width = width;
			current_videomode.height = height;
			current_videomode.planar = 0;
			current_videomode.banked = !(m & VESA_LINEAR);
			return 1;
		}
	}

	if(width==320 && height==240){
		vga_setmodex();
		current_videomode.width = width;
		current_videomode.height = height;
		current_videomode.planar = 1;
		current_videomode.banked = 0;
		current_videomode.data = VGARAM;
		return 1;
	}

	if(width==320 && height==204){
		vga_setwidemode();
		current_videomode.width = width;
		current_videomode.height = height;
		current_videomode.planar = 0;
		current_videomode.banked = 0;
		current_videomode.data = VGARAM;
		return 1;
	}

	if(width==320 && height==200){
		vga_setmode(MODE_13H);
		current_videomode.width = 320;
		current_videomode.height = 200;
		current_videomode.planar = 0;
		current_videomode.banked = 0;
		current_videomode.data = VGARAM;
		return 1;
	}

	if(width==288 && height==216){
		vga_setmodet();
		current_videomode.width = 288;
		current_videomode.height = 216;
		current_videomode.planar = 0;
		current_videomode.banked = 0;
		current_videomode.data = VGARAM;
		return 1;
	}

	return 0;
}



static int video_copy_screen_clip(s_screen * src){
	int bytes_to_copy, more_bytes_to_copy;
	char *sp;
	char *dp;
	int pos, b;
	int width, height;
	int y;

	// Determine width and height
	width = current_videomode.width;
	if(width > src->width) width = src->width;
	height = current_videomode.height;
	if(height > src->height) height = src->height;

	if(!width || !height) return 0;


	if(current_videomode.planar){

		// Can't copy all types of clipped regions to planar
		// video ram, not yet!
		if(src->width < current_videomode.width) return 0;

		y = 0;
		sp = src->data;
		do{
			asm_planarvcopy(current_videomode.data, sp, y, 1);
			++y;
			sp += src->width;
		}while(--height);

		return 1;
	}

	if(current_videomode.banked){
		pos = 0;
		b = -1;
		sp = src->data;

		do{
			// Set bank if changed
			if(pos/0x10000 != b){
				b = pos / 0x10000;
				VESA_setbank(b);
			}

			bytes_to_copy = width;
			if((pos&0xFFFF) + bytes_to_copy > 0x10000) bytes_to_copy = 0x10000 - (pos&0xFFFF);
			asm_copy(current_videomode.data+(pos&0xFFFF), sp, bytes_to_copy);

			if(bytes_to_copy < width){
				// Line not completed?

				// Must be because of bank change, set new bank.
				b = (pos+bytes_to_copy) / 0x10000;
				VESA_setbank(b);

				// Copy the rest of the line
				more_bytes_to_copy = width - bytes_to_copy;
				asm_copy(current_videomode.data, sp+bytes_to_copy, more_bytes_to_copy);
			}

			pos += current_videomode.width;
			sp += src->width;

		}while(--height);

		return 0;
	}


	// Copy to linear video ram
	sp = src->data;
	dp = current_videomode.data;
	do{
		asm_copy(dp, sp, width);
		sp += src->width;
		dp += current_videomode.width;
	}while(--height);

	return 1;
}



int video_copy_screen(s_screen * src){
	int bytes_to_copy;
	char *cp;
	int b;
	int height;

	// Clip width?
	if(current_videomode.width != src->width) return video_copy_screen_clip(src);

	// Determine height
	height = current_videomode.height;
	if(height > src->height) height = src->height;

	if(current_videomode.planar){
		asm_planarvcopy(current_videomode.data, src->data, 0, height);
		return 1;
	}

	if(current_videomode.banked){

		bytes_to_copy = src->width * height;
		cp = src->data;
		b = 0;
		while(bytes_to_copy > 0){
			VESA_setbank(b++);
			asm_copy(current_videomode.data, cp, (bytes_to_copy<0x10000 ? bytes_to_copy : 0x10000));
			bytes_to_copy -= 0x10000;
			cp += 0x10000;
		}
		VESA_setbank(0);

		return 1;
	}

	asm_copy(current_videomode.data, src->data, src->width * height);
	return 1;
}



void video_clearscreen(){
	if(current_videomode.planar){
		vga_clearmodex();
		return;
	}
	asm_clear(current_videomode.data, current_videomode.width*current_videomode.height);
}


