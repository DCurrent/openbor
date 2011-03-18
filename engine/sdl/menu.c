/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */


#include <unistd.h>
#include "SDL.h"
#include "sdlport.h"
#include "video.h"
#include "openbor.h"
#include "soundmix.h"
#include "packfile.h"
#include "gfx.h"
#include "hankaku.h"
#include "stristr.h"
#include "stringptr.h"

#include "pngdec.h"
#include "../resources/OpenBOR_Menu_480x272_png.h"
#include "../resources/OpenBOR_Menu_320x240_png.h"

#include <dirent.h>

extern int videoMode;

#define RGB32(B,G,R) ((R) | ((G) << 8) | ((B) << 16))
#define RGB16(B,G,R) ((B&0xF8)<<8) | ((G&0xFC)<<3) | (R>>3)
#define RGB(B,G,R)   (bpp==16?RGB16(B,G,R):RGB32(B,G,R))

#define BLACK		RGB(  0,   0,   0)
#define WHITE		RGB(255, 255, 255)
#define RED			RGB(255,   0,   0)
#define	GREEN		RGB(  0, 255,   0)
#define BLUE		RGB(  0,   0, 255)
#define YELLOW		RGB(255, 255,   0)
#define PURPLE		RGB(255,   0, 255)
#define ORANGE		RGB(255, 128,   0)
#define GRAY		RGB(112, 128, 144)
#define LIGHT_GRAY  RGB(223, 223, 223)
#define DARK_RED	RGB(128,   0,   0)
#define DARK_GREEN	RGB(  0, 128,   0)
#define DARK_BLUE	RGB(  0,   0, 128)

#define LOG_SCREEN_TOP 2
#define LOG_SCREEN_END (isWide ? 26 : 23)

SDL_Surface *Source = NULL;
SDL_Surface *Scaler = NULL;
SDL_Surface *Screen = NULL;
int bpp = 16;
int factor = 1;
int isFull = 0;
int isWide = 0;
int flags;
int dListTotal;
int dListCurrentPosition;
int dListScrollPosition;
int which_logfile = OPENBOR_LOG;
FILE *bgmFile = NULL;
unsigned int bgmPlay = 0, bgmLoop = 0, bgmCycle = 0, bgmCurrent = 0, bgmStatus = 0;
extern u32 bothkeys, bothnewkeys;
fileliststruct *filelist;

typedef struct{
	stringptr *buf;
	int *pos;
	int line;
	int rows;
	char ready;
}s_logfile;
s_logfile logfile[2];

typedef int (*ControlInput)();

int ControlMenu();
int ControlBGM();
void PlayBGM();
void StopBGM();
static ControlInput pControl;

int Control()
{
	return pControl();
}

void getAllLogs()
{
	ptrdiff_t i, j, k;
	for(i=0; i<2; i++)
	{
		logfile[i].buf = readFromLogFile(i);
		if(logfile[i].buf != NULL)
		{            
			logfile[i].pos = malloc(++logfile[i].rows * sizeof(int));
			if(logfile[i].pos == NULL) return;
			memset(logfile[i].pos, 0, logfile[i].rows * sizeof(int));

			for(k=0, j=0; j<logfile[i].buf->size; j++)
			{
				if(!k)
				{
					logfile[i].pos[logfile[i].rows - 1] = j;
					k = 1;
				}
				if(logfile[i].buf->ptr[j]=='\n')
				{
					int *_pos = malloc(++logfile[i].rows * sizeof(int));
					if(_pos == NULL) return;
					memcpy(_pos, logfile[i].pos, (logfile[i].rows - 1) * sizeof(int));
					_pos[logfile[i].rows - 1] = 0;
					free(logfile[i].pos);
					logfile[i].pos = NULL;
					logfile[i].pos = malloc(logfile[i].rows * sizeof(int));
					if(logfile[i].pos == NULL) return;
					memcpy(logfile[i].pos, _pos, logfile[i].rows * sizeof(int));
					free(_pos);
					_pos = NULL;
					logfile[i].buf->ptr[j] = 0;
					k = 0;
				}
				if(logfile[i].buf->ptr[j]=='\r') logfile[i].buf->ptr[j] = 0;
				if(logfile[i].rows>0xFFFFFFFE) break;
			}
			logfile[i].ready = 1;
		}
	}
}

void freeAllLogs()
{
	int i;
	for(i=0; i<2; i++)
	{
		if(logfile[i].ready)
		{
			free_string(logfile[i].buf);
			logfile[i].buf = NULL;
			free(logfile[i].pos);
			logfile[i].pos = NULL;
		}
	}
}

void sortList()
{
	int i, j;
	fileliststruct temp;
	if(dListTotal<2) return;
	for(j=dListTotal-1; j>0; j--)
	{
		for(i=0; i<j; i++)
		{
			if(stricmp(filelist[i].filename, filelist[i+1].filename)>0)
			{
				temp = filelist[i];
				filelist[i] = filelist[i+1];
				filelist[i+1] = temp;
			}
		}
	}
}

int findPaks(void)
{
	int i = 0;
	DIR* dp = NULL;
	struct dirent* ds;
#ifdef WII
	dp = opendir("sd:/apps/OpenBOR/Paks");
#else
	dp = opendir("./Paks");
#endif
	if(dp != NULL)
   	{
		while((ds = readdir(dp)) != NULL)
		{
			if(packfile_supported(ds))
			{
				fileliststruct *copy = NULL;
				if(filelist == NULL) filelist = malloc(sizeof(fileliststruct));
				else
				{
					copy = malloc(i * sizeof(fileliststruct));
					memcpy(copy, filelist, i * sizeof(fileliststruct));
					free(filelist);
					filelist = malloc((i + 1) * sizeof(fileliststruct));
					memcpy(filelist, copy, i * sizeof(fileliststruct));
					free(copy); copy = NULL;
				}
				memset(&filelist[i], 0, sizeof(fileliststruct));
				strncpy(filelist[i].filename, ds->d_name, strlen(ds->d_name));
				i++;
			}
		}
		closedir(dp);
   	}
	return i;
}

void copyScreens(SDL_Surface *Image)
{
	// Copy Logo or Menu from Source to Scaler to give us a background
	// prior to printing to this SDL_Surface.
	if(factor > 1)
	{
		// Center Text Surface within Scaler Image prior to final Blitting.
		Scaler->clip_rect.x = 1;
		Scaler->clip_rect.y = factor == 2 ? 4 : 5;
		Scaler->clip_rect.w = Image->w;
		Scaler->clip_rect.h = Image->h;
		SDL_BlitSurface(Image, NULL, Scaler, &Scaler->clip_rect);
	}
	else
		SDL_BlitSurface(Image, NULL, Scaler, &Scaler->clip_rect);
}

void writeToScreen(unsigned char *src, int pitch)
{
	int i;
	// pitch = bpp;
	unsigned char *dst = Screen->pixels;
	for(i=0; i<Screen->h; i++)
	{
		memcpy(dst, src, pitch);
		src += pitch;
		dst += pitch;
	}
}

void drawScreens(SDL_Surface *Image)
{
	SDL_Surface *FirstPass = NULL;
	SDL_Surface *SecondPass = NULL;

	if(SDL_MUSTLOCK(Screen)) SDL_LockSurface(Screen);
	switch(factor)
	{
		case 2:
			FirstPass = SDL_AllocSurface(SDL_SWSURFACE, Screen->w,Screen->h, bpp, 0, 0, 0, 0);
			(*GfxBlitters[(int)savedata.screen[videoMode][1]])((u8*)Scaler->pixels+Scaler->pitch*4+4, Scaler->pitch, pDeltaBuffer+Scaler->pitch, (u8*)FirstPass->pixels, FirstPass->pitch, FirstPass->w>>1, FirstPass->h>>1);
			if(Image) SDL_BlitSurface(Image, NULL, FirstPass, &Image->clip_rect);
			writeToScreen(FirstPass->pixels, FirstPass->pitch);
			SDL_FreeSurface(FirstPass);
			FirstPass = NULL;
			break;

		case 4:
			FirstPass = SDL_AllocSurface(SDL_SWSURFACE, Screen->w, Screen->h, bpp, 0, 0, 0, 0);
			(*GfxBlitters[(int)savedata.screen[videoMode][1]])((u8*)Scaler->pixels+Scaler->pitch*4+4, Scaler->pitch, pDeltaBuffer+Scaler->pitch, (u8*)FirstPass->pixels, FirstPass->pitch, FirstPass->w>>1, FirstPass->h>>1);
			SecondPass = SDL_AllocSurface(SDL_SWSURFACE, FirstPass->w, FirstPass->h, bpp, 0, 0, 0, 0);
			(*GfxBlitters[(int)savedata.screen[videoMode][1]])((u8*)FirstPass->pixels+FirstPass->pitch, FirstPass->pitch, pDeltaBuffer+FirstPass->pitch, (u8*)SecondPass->pixels, SecondPass->pitch, SecondPass->w>>1, SecondPass->h>>1);
			if(Image) SDL_BlitSurface(Image, NULL, SecondPass, &Image->clip_rect);
			writeToScreen(SecondPass->pixels, SecondPass->pitch);
			SDL_FreeSurface(FirstPass);
			FirstPass = NULL;
			SDL_FreeSurface(SecondPass);
			SecondPass = NULL;
			break;

		default:
			if(Image) SDL_BlitSurface(Image, NULL, Scaler, &Image->clip_rect);
			writeToScreen(Scaler->pixels, Scaler->pitch);
			break;
	}
	if(SDL_MUSTLOCK(Screen)) SDL_UnlockSurface(Screen);

	SDL_Flip(Screen);
}

void printText(int x, int y, int col, int backcol, int fill, char *format, ...)
{
	int x1, y1, i;
	u32 data;
	u16 *line16 = NULL;
	u32 *line32 = NULL;
	u8 *font;
	u8 ch = 0;
	char buf[128] = {""};
	va_list arglist;
		va_start(arglist, format);
		vsprintf(buf, format, arglist);
		va_end(arglist);

	if(factor > 1){ y += 5; }

	for(i=0; i<sizeof(buf); i++)
	{
		ch = buf[i];
		// mapping
		if (ch<0x20) ch = 0;
		else if (ch<0x80) { ch -= 0x20; }
		else if (ch<0xa0) {	ch = 0;	}
		else ch -= 0x40;
		font = (u8 *)&hankaku_font10[ch*10];
		// draw
		if (bpp == 16) line16 = (u16*)Scaler->pixels + x + y * Scaler->w;
		else           line32 = (u32*)Scaler->pixels + x + y * Scaler->w;

		for (y1=0; y1<10; y1++)
		{
			data = *font++;
			for (x1=0; x1<5; x1++)
			{
				if (data & 1)
				{
					if (bpp == 16) *line16 = col;
				    else           *line32 = col;
				}
				else if (fill)
				{
					if (bpp == 16) *line16 = backcol;
					else           *line32 = backcol;
				}

				if (bpp == 16) line16++;
				else           line32++;

				data = data >> 1;
			}
			if (bpp == 16) line16 += Scaler->w-5;
			else           line32 += Scaler->w-5;
		}
		x+=5;
	}
}

SDL_Surface *getPreview(char *filename)
{
	int i;
	int width = factor == 4 ? 640 : (factor == 2 ? 320 : 160);
	int height = factor == 4 ? 480 : (factor == 2 ? 240 : 120);
	unsigned char *sp;
	unsigned char *dp;
	unsigned char *tempPal;
	unsigned char realPal[1024];
	SDL_Color newPal[256];
	SDL_Surface *image = NULL;
	s_screen *title = NULL;
	s_screen *scale = NULL;

	// Grab current path and filename
	getBasePath(packfile, filename, 1);

	// Create & Load & Scale Image
	if(!loadscreen("data/bgs/title", packfile, realPal, PIXEL_8, &title)) return NULL;
	if((scale = allocscreen(width, height, title->pixelformat)) == NULL) return NULL;
	if((image = SDL_AllocSurface(SDL_SWSURFACE, width, height, 8, 0, 0, 0, 0)) == NULL) return NULL;

	scalescreen(scale, title);

	sp = (unsigned char*)scale->data;
	dp = (unsigned char*)image->pixels;

	do{
		memcpy(dp, sp, width);
		sp += scale->width;
		dp += image->pitch;
	}while(--height);

	tempPal = realPal;
	for(i=0;i<256;i++)
	{
		newPal[i].r=tempPal[0];
		newPal[i].g=tempPal[1];
		newPal[i].b=tempPal[2];
		tempPal+=3;
	}
	SDL_SetColors(image, newPal, 0, 256);

	// Free Images and Terminate FileCaching
	freescreen(&title);
	freescreen(&scale);

	// ScreenShots within Menu will be saved as "Menu"
	strncpy(packfile,"Menu.xxx",128);

	return image;
}

void StopBGM()
{
	sound_close_music();
	if (bgmFile)
	{
		fclose(bgmFile);
		bgmFile = NULL;
	}
	bgmPlay = 0;
}

void PlayBGM()
{
	bgmPlay = packfile_music_play(filelist, bgmFile, bgmLoop, dListCurrentPosition, dListScrollPosition);
}

int ControlMenu()
{
	int status = -1;
	int dListMaxDisplay = 17;
	bothnewkeys = 0;
	inputrefresh();
	switch(bothnewkeys)
	{
		case FLAG_MOVEUP:
			dListScrollPosition--;
			if(dListScrollPosition < 0)
			{
				dListScrollPosition = 0;
				dListCurrentPosition--;
			}
			if(dListCurrentPosition < 0) dListCurrentPosition = 0;
			break;

		case FLAG_MOVEDOWN:
			dListCurrentPosition++;
			if(dListCurrentPosition > dListTotal - 1) dListCurrentPosition = dListTotal - 1;
			if(dListCurrentPosition > dListMaxDisplay)
	        {
		        if((dListCurrentPosition+dListScrollPosition) < dListTotal) dListScrollPosition++;
			    dListCurrentPosition = dListMaxDisplay;
			}
			break;

		case FLAG_MOVELEFT:
			break;

		case FLAG_MOVERIGHT:
			break;

		case FLAG_START:
		case FLAG_ATTACK:
			// Start Engine!
			status = 1;
			break;

		case FLAG_ATTACK2:
			pControl = ControlBGM;
			status = -2;
			break;

		case FLAG_SPECIAL:
		case FLAG_ESC:
			// Exit Engine!
			status = 2;
			break;

		case FLAG_JUMP:
			//drawLogs();
			status = 3;
			break;

		default:
			// No Update Needed!
			status = 0;
			break;
	}
	return status;
}

int ControlBGM()
{
	int status = -2;
	int dListMaxDisplay = 17;
	bothnewkeys = 0;
	inputrefresh();
	switch(bothnewkeys)
	{
		case FLAG_MOVEUP:
			dListScrollPosition--;
			if(dListScrollPosition < 0)
			{
				dListScrollPosition = 0;
				dListCurrentPosition--;
			}
			if(dListCurrentPosition < 0) dListCurrentPosition = 0;
			break;

		case FLAG_MOVEDOWN:
			dListCurrentPosition++;
			if(dListCurrentPosition > dListTotal - 1) dListCurrentPosition = dListTotal - 1;
			if(dListCurrentPosition > dListMaxDisplay)
	        {
		        if((dListCurrentPosition+dListScrollPosition) < dListTotal) dListScrollPosition++;
			    dListCurrentPosition = dListMaxDisplay;
			}
			break;

		case FLAG_MOVELEFT:
			if(!bgmStatus || (bgmPlay && bgmCurrent == dListCurrentPosition+dListScrollPosition))
			{
				filelist[bgmCurrent].bgmTrack--;
				if(filelist[bgmCurrent].bgmTrack < 0) filelist[bgmCurrent].bgmTrack = filelist[bgmCurrent].nTracks-1;
				if(bgmStatus) PlayBGM();
			}
			break;

		case FLAG_MOVERIGHT:
			if(!bgmStatus || (bgmPlay && bgmCurrent == dListCurrentPosition+dListScrollPosition))
			{
				filelist[bgmCurrent].bgmTrack++;
				if(filelist[bgmCurrent].bgmTrack > filelist[bgmCurrent].nTracks - 1) filelist[bgmCurrent].bgmTrack = 0;
				if(bgmStatus) PlayBGM();
			}
			break;

		case FLAG_START:
		case FLAG_ATTACK:
			if(bgmPlay) StopBGM();
			else PlayBGM();
			break;

		case FLAG_ATTACK2:
			if(!bgmPlay)
			{
				if(bgmLoop) bgmLoop = 0;
				else bgmLoop = 1;
			}
			break;

		case FLAG_JUMP:
			if(!bgmPlay)
			{
				if(bgmCycle) bgmCycle = 0;
				else bgmCycle = 1;
			}
			break;

		case FLAG_SPECIAL:
		case FLAG_ESC:
			pControl = ControlMenu;
			status = -1;
			break;

		default:
			// No Update Needed!
			status = 0;
			break;
	}
	return status;
}

void initMenu(int type)
{	
#if WIN || LINUX
	factor = savedata.screen[videoMode][0] ? savedata.screen[videoMode][0] : 1;
	isFull = savedata.fullscreen;
	//isWide = savedata.fullscreen && (((float)nativeWidth / (float)nativeHeight) > 1.54);
#ifndef DARWIN
	bpp = 32;
#endif
#endif

	Init_Gfx(bpp==32 ? 888 : 565, bpp);
	memset(pDeltaBuffer, 0x00, 1244160);
#if !defined(DINGOO) && !defined(WIZ)
	flags = isFull?(SDL_SWSURFACE|SDL_DOUBLEBUF|SDL_FULLSCREEN):(SDL_SWSURFACE|SDL_DOUBLEBUF);
#else
	flags = SDL_SWSURFACE;
#endif

	// Read Logo or Menu from Array.
	if(type) {
		Source = pngToSurface(isWide ? (void*) openbor_menu_480x272_png.data : (void*) openbor_menu_320x240_png.data);

		// Depending on which mode we are in (WideScreen/FullScreen)
		// allocate proper size for SDL_Surface to perform final Blitting.
		Screen = SDL_SetVideoMode(Source->w * factor, Source->h * factor, bpp, flags);

		// Allocate Scaler with extra space for upscaling.
		Scaler = SDL_AllocSurface(SDL_SWSURFACE,
		                      factor > 1 ? Screen->w + 4 : Screen->w,
							  factor > 1 ? Screen->h + 8 : Screen->h,
							  bpp, 0, 0, 0, 0);
	}

	control_init(2);
	apply_controls();
	sound_init(12);
	sound_start_playback(savedata.soundbits,savedata.soundrate);
}

void termMenu()
{
	SDL_FreeSurface(Source);
	Source = NULL;
	SDL_FreeSurface(Scaler);
	Scaler = NULL;
#ifndef SDL13
	SDL_FreeSurface(Screen);
	Screen = NULL;
#endif
	sound_exit();
	control_exit();
	Term_Gfx();
}

void drawMenu()
{
	SDL_Surface *Image = NULL;
	char listing[45] = {""};
	int list = 0;
	int shift = 0;
	int colors = 0;

	copyScreens(Source);
	if(dListTotal < 1) printText((isWide ? 30 : 8), (isWide ? 33 : 24), RED, 0, 0, "No Mods In Paks Folder!");
	for(list=0; list<dListTotal; list++)
	{
		if(list<18)
		{
			shift = 0;
			colors = GRAY;
			strncpy(listing, "", (isWide ? 44 : 28));
			if(strlen(filelist[list+dListScrollPosition].filename)-4 < (isWide ? 44 : 28))
				strncpy(listing, filelist[list+dListScrollPosition].filename, strlen(filelist[list+dListScrollPosition].filename)-4);
			if(strlen(filelist[list+dListScrollPosition].filename)-4 > (isWide ? 44 : 28))
				strncpy(listing, filelist[list+dListScrollPosition].filename, (isWide ? 44 : 28));
			if(list == dListCurrentPosition)
			{
				shift = 2;
				colors = RED;
#ifdef WII
				Image = NULL;
#else
				Image = getPreview(filelist[list+dListScrollPosition].filename);
#endif
				if(Image)
				{
					Image->clip_rect.x = factor * (isWide ? 286 : 155);
					Image->clip_rect.y = factor * (isWide ? (factor == 4 ? (Sint16)32.5 : 32) : (factor == 4 ? (Sint16)21.5 : 21));
				}
				else
					printText((isWide ? 288 : 157), (isWide ? 141 : 130), RED, 0, 0, "No Preview Available!");
			}
			printText((isWide ? 30 : 7) + shift, (isWide ? 33 : 22)+(11*list) , colors, 0, 0, "%s", listing);
		}
	}

	printText((isWide ? 26 : 5), (isWide ? 11 : 4), WHITE, 0, 0, "OpenBoR %s", VERSION);
	printText((isWide ? 392 : 261),(isWide ? 11 : 4), WHITE, 0, 0, __DATE__);
	printText((isWide ? 23 : 4),(isWide ? 251 : 226), WHITE, 0, 0, "%s: Start Game", control_getkeyname(savedata.keys[0][SDID_ATTACK]));
	printText((isWide ? 150 : 84),(isWide ? 251 : 226), WHITE, 0, 0, "%s: BGM Player", control_getkeyname(savedata.keys[0][SDID_ATTACK2]));
	printText((isWide ? 270 : 164),(isWide ? 251 : 226), WHITE, 0, 0, "%s: View Logs", control_getkeyname(savedata.keys[0][SDID_JUMP]));
	printText((isWide ? 390 : 244),(isWide ? 251 : 226), WHITE, 0, 0, "%s: Quit Game", control_getkeyname(savedata.keys[0][SDID_SPECIAL]));
   	printText((isWide ? 330 : 197),(isWide ? 170 : 155), BLACK, 0, 0, "www.LavaLit.com");
	printText((isWide ? 322 : 190),(isWide ? 180 : 165), BLACK, 0, 0, "www.SenileTeam.com");

#ifdef SPK_SUPPORTED
	printText((isWide ? 324 : 192),(isWide ? 191 : 176), DARK_RED, 0, 0, "SecurePAK Edition");
#endif

	drawScreens(Image);
	if(Image)
	{
		SDL_FreeSurface(Image);
		Image = NULL;
	}
}

void drawBGMPlayer()
{
	SDL_Surface *Image = NULL;
	SDL_Rect rect;
	char listing[45] = {""}, bgmListing[25] = {""};
	char t1[64] = "", t2[25] = "Unknown";
	char a1[64] = "", a2[25] = "Unknown";
	int list = 0, colors = 0, shift = 0;

	// Allocate Preview Box for Music Text Info.
	Image = SDL_AllocSurface(SDL_SWSURFACE, Source->w, Source->h, bpp, 0, 0, 0, 0);
	if (Image != NULL)
	{
		SDL_BlitSurface(Source, NULL, Image, NULL);
		rect.x = (isWide ? 286 : 155);
		rect.y = (isWide ? (factor == 4 ? (Sint16)32.5 : 32) : (factor == 4 ? (Sint16)21.5 : 21));
		rect.w = 160;
		rect.h = 120;
		SDL_FillRect(Image, &rect, LIGHT_GRAY);
		copyScreens(Image);
		SDL_FreeSurface(Image);
		Image = NULL;
	}

	for(list=0; list<dListTotal; list++)
	{
		if(list<18)
		{
			shift = 0;
			colors = GRAY;
			strncpy(listing, "", (isWide ? 44 : 28));
			if(strlen(filelist[list+dListScrollPosition].filename)-4 < (isWide ? 44 : 28))
				strncpy(listing, filelist[list+dListScrollPosition].filename, strlen(filelist[list+dListScrollPosition].filename)-4);
			if(strlen(filelist[list+dListScrollPosition].filename)-4 > (isWide ? 44 : 28))
				strncpy(listing, filelist[list+dListScrollPosition].filename, (isWide ? 44 : 28));
			if(list==dListCurrentPosition) { shift = 2; colors = RED; }
			printText((isWide ? 30 : 7) + shift, (isWide ? 33 : 22)+(11*list) , colors, 0, 0, "%s", listing);
		}
	}

	printText((isWide ? 26 : 5), (isWide ? 11 : 4), WHITE, 0, 0, "OpenBoR %s", VERSION);
	printText((isWide ? 392 : 261),(isWide ? 11 : 4), WHITE, 0, 0, __DATE__);
	printText((isWide ? 23 : 4),(isWide ? 251 : 226), WHITE, 0, 0, "%s: %s", control_getkeyname(savedata.keys[0][SDID_ATTACK]), bgmPlay ? "Stop" : "Play");
	printText((isWide ? 150 : 84),(isWide ? 251 : 226), WHITE, 0, 0, "%s: %s", control_getkeyname(savedata.keys[0][SDID_ATTACK2]), bgmLoop ? "Repeat On" : "Repeat Off");
	printText((isWide ? 270 : 164),(isWide ? 251 : 226), WHITE, 0, 0, "%s: %s", control_getkeyname(savedata.keys[0][SDID_JUMP]), bgmCycle ? "Cycle On" : "Cycle Off");
	printText((isWide ? 390 : 244),(isWide ? 251 : 226), WHITE, 0, 0, "%s: Exit Player", control_getkeyname(savedata.keys[0][SDID_SPECIAL]));
	printText((isWide ? 330 : 197),(isWide ? 170 : 155), BLACK, 0, 0, "www.LavaLit.com");
	printText((isWide ? 322 : 190),(isWide ? 180 : 165), BLACK, 0, 0, "www.SenileTeam.com");

#ifdef SPK_SUPPORTED
	printText((isWide ? 324 : 192),(isWide ? 191 : 176), DARK_RED, 0, 0, "SecurePAK Edition");
#endif

	if(!bgmPlay) bgmCurrent = dListCurrentPosition+dListScrollPosition;
	if(strlen(filelist[bgmCurrent].filename)-4 < 24)
		strncpy(bgmListing, filelist[bgmCurrent].filename, strlen(filelist[bgmCurrent].filename)-4);
	if(strlen(filelist[bgmCurrent].filename)-4 > 24)
		strncpy(bgmListing, filelist[bgmCurrent].filename, 24);
	if(!sound_query_music(a1, t1))
	{
		PlayBGM();
		sound_query_music(a1, t1);
		StopBGM();
	}
	if(t1[0]) strncpy(t2, t1, 25);
	if(a1[0]) strncpy(a2, a1, 25);
	printText((isWide ? 288 : 157),(isWide ? 35 : 23) + (11 * 0), DARK_RED, 0, 0, "Game: %s", bgmListing);
	printText((isWide ? 288 : 157),(isWide ? 35 : 23) + (11 * 1), bgmPlay ? DARK_GREEN : DARK_BLUE, 0, 0, "Total Tracks: %d", filelist[bgmCurrent].nTracks-1);
	printText((isWide ? 288 : 157),(isWide ? 35 : 23) + (11 * 2), bgmPlay ? DARK_GREEN : DARK_BLUE, 0, 0, "Current Track: %d", filelist[bgmCurrent].bgmTrack);
	printText((isWide ? 288 : 157),(isWide ? 35 : 23) + (11 * 3), bgmPlay ? DARK_GREEN : DARK_BLUE, 0, 0, "File: %s", filelist[bgmCurrent].bgmFileName[filelist[bgmCurrent].bgmTrack]);
	printText((isWide ? 288 : 157),(isWide ? 35 : 23) + (11 * 4), bgmPlay ? DARK_GREEN : DARK_BLUE, 0, 0, "Track: %s", t2);
	printText((isWide ? 288 : 157),(isWide ? 35 : 23) + (11 * 5), bgmPlay ? DARK_GREEN : DARK_BLUE, 0, 0, "Artist: %s", a2);

	drawScreens(NULL);
}

void drawLogs()
{
	int i=which_logfile, j, k, l, done=0;
	SDL_Surface *Viewer = NULL;

	bothkeys = bothnewkeys = 0;
	Viewer = SDL_AllocSurface(SDL_SWSURFACE, Source->w, Source->h, bpp, 0, 0, 0, 0);
	bothkeys = bothnewkeys = 0;

	while(!done)
	{
	    copyScreens(Viewer);
	    inputrefresh();
	    sound_update_music();
	    printText((isWide ? 410 : 250), 3, RED, 0, 0, "Quit : Escape");
		if(bothnewkeys & FLAG_ESC) done = 1;

		if(logfile[i].ready)
		{
			printText(5, 3, RED, 0, 0, "OpenBorLog.txt");
			if(bothkeys & FLAG_MOVEUP) --logfile[i].line;
	        if(bothkeys & FLAG_MOVEDOWN) ++logfile[i].line;
			if(bothkeys & FLAG_MOVELEFT) logfile[i].line = 0;
			if(bothkeys & FLAG_MOVERIGHT) logfile[i].line = logfile[i].rows - (LOG_SCREEN_END - LOG_SCREEN_TOP);
			if(logfile[i].line > logfile[i].rows - (LOG_SCREEN_END - LOG_SCREEN_TOP) - 1) logfile[i].line = logfile[i].rows - (LOG_SCREEN_END - LOG_SCREEN_TOP) - 1;
			if(logfile[i].line < 0) logfile[i].line = 0;
			for(l=LOG_SCREEN_TOP, j=logfile[i].line; j<logfile[i].rows-1; l++, j++)
			{
				if(l<LOG_SCREEN_END)
				{
					char textpad[480] = {""};
					for(k=0; k<480; k++)
					{
						if(!logfile[i].buf->ptr[logfile[i].pos[j]+k]) break;
						textpad[k] = logfile[i].buf->ptr[logfile[i].pos[j]+k];
					}
					if(logfile[i].rows>0xFFFF)
						printText(5, l*10, WHITE, 0, 0, "0x%08x:  %s", j, textpad);
					else
						printText(5, l*10, WHITE, 0, 0, "0x%04x:  %s", j, textpad);
				}
				else break;
			}
		}
		else if(i == SCRIPT_LOG) printText(5, 3, RED, 0, 0, "Log NOT Found: ScriptLog.txt");
		else                     printText(5, 3, RED, 0, 0, "Log NOT Found: OpenBorLog.txt");

	    drawScreens(NULL);
	}
	SDL_FreeSurface(Viewer);
	Viewer = NULL;
	drawMenu();
}

void Menu()
{
	int done = 0;
	int ctrl = 0;
	loadsettings();
	dListCurrentPosition = 0;
	if((dListTotal = findPaks()) != 1)
	{
		sortList();
		getAllLogs();
#ifndef WII
		packfile_music_read(filelist, dListTotal);
#endif
		initMenu(1);
		drawMenu();
		pControl = ControlMenu;

		while(!done)
		{
			sound_update_music();
			bgmStatus = sound_query_music(NULL, NULL);
			if(bgmPlay && !bgmStatus)
			{
				if(bgmCycle)
				{
					filelist[bgmCurrent].bgmTrack++;
					if(filelist[bgmCurrent].bgmTrack > filelist[bgmCurrent].nTracks - 1) filelist[bgmCurrent].bgmTrack = 0;
					PlayBGM();
				}
				else StopBGM();
				drawBGMPlayer();
			}

			ctrl = Control();
			switch(ctrl)
			{
				case 1:
				case 2:
					done = 1;
					break;

				case 3:
					drawLogs();
					break;

				case -1:
					drawMenu();
					break;

				case -2:
					drawBGMPlayer();
					break;
			}
		}
		freeAllLogs();
		termMenu();
		if(ctrl == 2)
		{
			if (filelist)
			{
				free(filelist);
				filelist = NULL;
			}
			borExit(0);
		}
	}
	getBasePath(packfile, filelist[dListCurrentPosition+dListScrollPosition].filename, 1);
	free(filelist);
}
