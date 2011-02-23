#ifndef	SPRITEQ_H	
#define	SPRITEQ_H	


#define			SFX_NONE		0
#define			SFX_REMAP		1
#define			SFX_BLEND		2


// Sprite queueing and sorting

void spriteq_add(int x, int y, int z, void *frame, int effect, char *lut);
void spriteq_draw(s_screen *screen);
void spriteq_clear(void);


#endif

