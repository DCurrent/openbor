// Sprite queueing and sorting

#include "ps2port.h"
#include "types.h"
#include "sprite.h"

#ifndef NULL
#define NULL ((void*)0)
#endif


// This should be enough for most games...
// But bear in mind that text is also composed of sprites!
#define			MAXQSPRITES		500

#define			SFX_NONE		0
#define			SFX_REMAP		1
#define			SFX_BLEND		2



typedef struct{
	int		x;
	int		y;
	unsigned int	z;
	void *		frame;
	int		effect;
	char *		lut;
}qstruct;


static qstruct queue[MAXQSPRITES];
static qstruct * order[MAXQSPRITES];
static int spritequeue_len = 0;



void spriteq_add(int x, int y, int z, void *frame, int effect, char *lut){
	if(spritequeue_len>=MAXQSPRITES) return;
	if(frame==NULL) return;
	if(lut==NULL) effect = SFX_NONE;
	queue[spritequeue_len].x = x;
	queue[spritequeue_len].y = y;
	queue[spritequeue_len].z = z;
	queue[spritequeue_len].frame = frame;
	queue[spritequeue_len].effect = effect;
	queue[spritequeue_len].lut = lut;
	order[spritequeue_len] = &queue[spritequeue_len];
	++spritequeue_len;
}




// Double sort code (sorts high and low simultaneously from 2 directions)
// Can't get much faster than this - I think
static void spriteq_sort(){
	int i, lidx, hidx, lz, hz, start, end;
	void * tempp;

	start = 0;
	end = spritequeue_len - 1;

	while(start < end){

		lidx = end;
		hidx = start;

		lz = order[lidx]->z;
		hz = order[hidx]->z;

		// Search for lowest and highest Z coord
		for(i=start; i<=end; i++){
			if(order[i]->z < lz){
				lidx = i;
				lz = order[i]->z;
			}
			if(order[i]->z > hz){
				hidx = i;
				hz = order[i]->z;
			}
		}


		// No need to sort equal values!
		if(hz==lz) return;


		// Exchange values (low)
		tempp = order[start];
		order[start] = order[lidx];
		order[lidx] = tempp;


		// Prevent confusion:
		// This value may already have been exchanged!
		if(hidx==start) hidx = lidx;


		// Exchange values (high)
		tempp = order[end];
		order[end] = order[hidx];
		order[hidx] = tempp;

		++start;
		--end;
	}
}



void spriteq_draw(s_screen *screen){
	int i;

	spriteq_sort();

	for(i=0;i<spritequeue_len;i++){
		if(order[i]->effect==SFX_REMAP) putsprite_remap(order[i]->x, order[i]->y, order[i]->frame, screen, order[i]->lut);
		else if(order[i]->effect==SFX_BLEND) putsprite_blend(order[i]->x, order[i]->y, order[i]->frame, screen, order[i]->lut);
		else putsprite(order[i]->x, order[i]->y, order[i]->frame, screen);
	}
}


void spriteq_clear(void){
	spritequeue_len = 0;
}




