// A soundblaster interface. No bugs?

#ifndef SBLASTER_H
#define SBLASTER_H

#define		MAXDMABUFSIZE	0x10000
#define		MONO		0
#define		STEREO		1
#define		LOWQ		0
#define		HIGHQ		2
#define		SBDETECT	-1

#define		SB_MASTERVOL	0x22
#define		SB_VOICEVOL	0x04
#define		SB_CDVOL	0x28



// Globals

extern char *errptr;


// The interface

void * SB_init(int port, int dma, int hdma, int irq, int buffersize);
int SB_playstart(int stereo, int bits, int samplerate);
int SB_getpos();
void SB_playstop();
void SB_exit();
void SB_hook(void (*func)());
void getSBvars(int *a, int *i, int *d, int *h, int *type);
char SB_getvolume(char dev);
char SB_setvolume(char dev, char volume);


#endif
