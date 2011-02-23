/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <pspsdk.h>
#include <pspdisplay.h>
#include "menu.h"
#include "version.h"
#include "stristr.h"
#include "netcomm.h"
#include "openbor.h"
#include "packfile.h"
#include "graphics.h"
#include "kernel/kernel.h"

#define LOG_SCREEN_TOP 3
#define LOG_SCREEN_END 24

char wMode[128] = {""};
char wStatus[128] = {"WiFi Disabled"};
char dListPath[256] = {""};
int dListTotal;
int dListCurrentPosition;
int dListScrollPosition;
int which_logfile = OPENBOR_LOG;
Image *pMenu = NULL;
FILE *bgmFile = NULL;
u32	lastPad = 0;
u32 bgmPlay = 0, bgmLoop = 0, bgmCycle = 0, bgmCurrent = 0, bgmStatus = 0;
fileliststruct *filelist;

typedef struct{
    char *buf;
    int *pos;
    int line;
    int rows;
    int size;
    char ready;
}s_logfile;
s_logfile logfile[2];

typedef int (*ControlInput)();
int ControlMenu();
int ControlBGM();
int ControlLOG();
void PlayBGM();
void StopBGM();
static ControlInput pControl;

int Control()
{
	return pControl();
}

u32 getInput(int delay, int update)
{
	u32 pad = getPad(0);
	if(pad == lastPad && delay) return 0;
	if(update) return lastPad = pad;
    else return pad;
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

Image *getPreview(char *filename)
{
	int x, y;
	int width = 160;
	int height = 120;
	unsigned char pal[768];
	unsigned char *sp;
	Color *dp;
	s_screen *title = NULL;
	s_screen *scaledown = NULL;
	Image *preview = NULL;

	// Grab current path and filename
	strncpy(packfile, dListPath, 256);
	strncat(packfile, filename, strlen(filename));

	// Create & Load & Scale Image
	if(!loadscreen("data/bgs/title.gif", packfile, pal, pixelformat, &title)) return NULL;
	if((scaledown = allocscreen(width, height, pixelformat==PIXEL_x8?PIXEL_32:PIXEL_8)) == NULL) return NULL;
	if((preview = createImage(width, height)) == NULL) return NULL;
	scalescreen(scaledown, title);

	// Load Pallete for preview
	pal[0] = pal[1] = pal[2] = 0;
	palette_set_corrected(pal, 0,0,0, 0,0,0);

	// Apply Pallete for preview then blit
	sp = scaledown->data;
   	dp = (void*)preview->data + 512 * 2;
	for(y=0; y<height; y++)
	{
   		for(x=0; x<width; x++) dp[x] = palette[((int)(sp[x])) & 0xFF];
   		sp += scaledown->width;
   		dp += preview->textureWidth;
   	}

	// Free Images and Terminiate Filecaching
	freescreen(&title);
	freescreen(&scaledown);

	// ScreenShots within Menu will be saved as "Menu"
	strncpy(packfile,"Menu.xxx",128);

	return preview;
}

void getAllPreviews()
{
	int i;
	for(i=0; i<dListTotal; i++)
	{
		filelist[i].preview = getPreview(filelist[i].filename);
	}
}

void freeAllPreviews()
{
	int i;
	for(i=0; i<dListTotal; i++)
	{
		if(filelist[i].preview != NULL) freeImage(filelist[i].preview);
	}
}

void freeAllImages()
{
	freeImage(pMenu);
	freeImage(text);
	freeAllPreviews();
}

void getAllLogs()
{
    int i, j, k;
    for(i=0; i<2; i++)
    {
        logfile[i].buf = readFromLogFile(i);
        if(logfile[i].buf != NULL)
        {
            logfile[i].size = strlen(logfile[i].buf);
            logfile[i].pos = tracemalloc("pos #1", ++logfile[i].rows * sizeof(int));
            if(logfile[i].pos == NULL) return;
            memset(logfile[i].pos, 0, logfile[i].rows * sizeof(int));

            for(k=0, j=0; j<logfile[i].size; j++)
            {
                if(!k)
                {
                    logfile[i].pos[logfile[i].rows - 1] = j;
                    k = 1;
                }
                if(logfile[i].buf[j]=='\n')
                {
                    int *_pos = tracemalloc("_pos", ++logfile[i].rows * sizeof(int));
                    if(_pos == NULL) return;
                    memcpy(_pos, logfile[i].pos, (logfile[i].rows - 1) * sizeof(int));
                    _pos[logfile[i].rows - 1] = 0;
                    tracefree(logfile[i].pos);
                    logfile[i].pos = NULL;
                    logfile[i].pos = tracemalloc("pos #2", logfile[i].rows * sizeof(int));
                    if(logfile[i].pos == NULL) return;
                    memcpy(logfile[i].pos, _pos, logfile[i].rows * sizeof(int));
                    tracefree(_pos);
                    _pos = NULL;
                    logfile[i].buf[j] = 0;
                    k = 0;
                }
                if(logfile[i].buf[j]=='\r') logfile[i].buf[j] = 0;
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
            tracefree(logfile[i].buf);
            logfile[i].buf = NULL;
            tracefree(logfile[i].pos);
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

int findPaks()
{
    int i = 0;
    DIR* dp = NULL;
    struct dirent* ds;
    dp = opendir(dListPath);
    while((ds = readdir(dp)) != NULL)
    {
        if(packfile_supported(ds))
        {
            fileliststruct *copy = NULL;
            if(filelist == NULL) filelist = tracemalloc("filelist", sizeof(fileliststruct));
            else
            {
                copy = tracemalloc("filelistcopy", i * sizeof(fileliststruct));
                memcpy(copy, filelist, i * sizeof(fileliststruct));
                tracefree(filelist);
                filelist = tracemalloc("filelist", (i + 1) * sizeof(fileliststruct));
                memcpy(filelist, copy, i * sizeof(fileliststruct));
                tracefree(copy); copy = NULL;
            }
            memset(&filelist[i], 0, sizeof(fileliststruct));
            strncpy(filelist[i].filename, ds->d_name, strlen(ds->d_name));
            i++;
        }
    }
    closedir(dp);
    return i;
}

void drawMenu()
{
	char listing[45] = {""};
	int list = 0;
	int shift = 0;
	int colors = 0;

	copyImageToImage(0, 0, PSP_LCD_WIDTH, PSP_LCD_HEIGHT, pMenu, 0, 0, text);
	if(dListTotal < 1) printText(text, 30, 33, RED, 0, 0, "No Mods In Paks Folder!");
	for(list=0; list<dListTotal; list++)
	{
		if(list<18)
		{
			shift = 0;
			colors = BLACK;
			strncpy(listing, "", 44);
			if(strlen(filelist[list+dListScrollPosition].filename)-4 < 44)
				strncpy(listing, filelist[list+dListScrollPosition].filename, strlen(filelist[list+dListScrollPosition].filename)-4);
			if(strlen(filelist[list+dListScrollPosition].filename)-4 > 44)
				strncpy(listing, filelist[list+dListScrollPosition].filename, 44);
			if(list==dListCurrentPosition)
			{
				shift = 2;
				colors = RED;
				if(filelist[list].preview != NULL)
					copyImageToImage(0, 1, 160, 120, filelist[list+dListScrollPosition].preview, 286, 32, text);
				else
					copyImageToImage(286, 32, 160, 120, pMenu, 286, 32, text);
			}
			printText(text, 30 + shift, 33 + (11 * list), colors, 0, 0, "%s", listing);
		}
	}
	printText(text, 185,  11, WHITE, 0, 0, "OpenBOR %s", VERSION);
    printText(text, 20,   11, WHITE, 0, 0, ":");
	printText(text, 27,   11, WHITE, 0, 0, "About");
	printText(text, 453,  11, WHITE, 0, 0, ":");
	printText(text, 431,  11, WHITE, 0, 0, "Help");
	printText(text, 28,  251, WHITE, 0, 0, "WiFi Menu");     // Square
    printText(text, 150, 251, WHITE, 0, 0, "BGM Player");    // Triangle
	printText(text, 268, 251, WHITE, 0, 0, "Image Viewer");  // Circle
	printText(text, 392, 251, WHITE, 0, 0, "View Logs");     // Cross
	printText(text, 328, 170, BLACK, 0, 0, "www.LavaLit.com");
	printText(text, 320, 180, BLACK, 0, 0, "www.SenileTeam.com");

#ifdef SPK_SUPPORTED
	printText(text, 322, 191, DARK_RED, 0, 0, "SecurePAK Edition");
#endif

	blitImageToScreen(0, 0, PSP_LCD_WIDTH, PSP_LCD_HEIGHT, text, 0, 0);
	sceDisplayWaitVblankStart();
	flipScreen();
}

void drawBGMPlayer()
{
	char listing[45] = {""}, bgmListing[25] = {""};
	char t1[64] = "", t2[25] = "Unknown";
	char a1[64] = "", a2[25] = "Unknown";
	int list = 0, colors = 0, shift = 0;
    Image *pBox = createImage(160, 120);

	copyImageToImage(0, 0, PSP_LCD_WIDTH, PSP_LCD_HEIGHT, pMenu, 0, 0, text);
    if(pBox != NULL)
    {
        fillImageRect(pBox, LIGHT_GRAY, 0, 0, 160, 120);
        copyImageToImage(0, 0, 160, 120, pBox, 286, 32, text);
        freeImage(pBox);
        pBox = NULL;
    }
	for(list=0; list<dListTotal; list++)
	{
		if(list<18)
		{
			shift = 0;
			colors = BLACK;
			strncpy(listing, "", 44);
			if(strlen(filelist[list+dListScrollPosition].filename)-4 < 44)
				strncpy(listing, filelist[list+dListScrollPosition].filename, strlen(filelist[list+dListScrollPosition].filename)-4);
			if(strlen(filelist[list+dListScrollPosition].filename)-4 > 44)
				strncpy(listing, filelist[list+dListScrollPosition].filename, 44);
			if(list==dListCurrentPosition) { shift = 2; colors = RED; }
			printText(text, 30 + shift, 33 + (11 * list), colors, 0, 0, "%s", listing);
		}
	}
	printText(text, 185,  11, WHITE, 0, 0, "OpenBOR %s", VERSION);
	printText(text, 20,   11, WHITE, 0, 0, ":");
	printText(text, 27,   11, WHITE, 0, 0, "Prev");
	printText(text, 453,  11, WHITE, 0, 0, ":");
	printText(text, 431,  11, WHITE, 0, 0, "Next");
	printText(text, 28,  251, WHITE, 0, 0, "%s", bgmPlay ? "Stop Track" : "Play Track"); // Square
	printText(text, 151, 251, WHITE, 0, 0, "%s", bgmLoop ? "Repeat On" : "Repeat Off");  // Triangle
	printText(text, 274, 251, WHITE, 0, 0, "%s", bgmCycle ? "Cycle On" : "Cycle Off");   // Circle
	printText(text, 390, 251, WHITE, 0, 0, "Exit Player");                               // Cross
	printText(text, 328, 170, BLACK, 0, 0, "www.LavaLit.com");
	printText(text, 320, 180, BLACK, 0, 0, "www.SenileTeam.com");

#ifdef SPK_SUPPORTED
	printText(text, 322, 191, DARK_RED, 0, 0, "SecurePAK Edition");
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
    printText(text, 290, 35 + (11 * 0), DARK_RED, 0, 0, "Game: %s", bgmListing);
	printText(text, 290, 35 + (11 * 1), bgmPlay ? DARK_GREEN : DARK_BLUE, 0, 0, "Total Tracks: %d", filelist[bgmCurrent].nTracks-1);
	printText(text, 290, 35 + (11 * 2), bgmPlay ? DARK_GREEN : DARK_BLUE, 0, 0, "Current Track: %d", filelist[bgmCurrent].bgmTrack);
	printText(text, 290, 35 + (11 * 3), bgmPlay ? DARK_GREEN : DARK_BLUE, 0, 0, "File: %s", filelist[bgmCurrent].bgmFileName[filelist[bgmCurrent].bgmTrack]);
	printText(text, 290, 35 + (11 * 4), bgmPlay ? DARK_GREEN : DARK_BLUE, 0, 0, "Track: %s", t2);
	printText(text, 290, 35 + (11 * 5), bgmPlay ? DARK_GREEN : DARK_BLUE, 0, 0, "Artist: %s", a2);

	blitImageToScreen(0, 0, PSP_LCD_WIDTH, PSP_LCD_HEIGHT, text, 0, 0);
	sceDisplayWaitVblankStart();
	flipScreen();
}

void drawLogs()
{
	int i=which_logfile, j, k, l;
    Image *box = createImage(PSP_LCD_WIDTH, 224);
	copyImageToImage(0, 0, PSP_LCD_WIDTH, PSP_LCD_HEIGHT, pMenu, 0, 0, text);
	drawImageBox(box, DARK_GRAY, BLACK, 2);
	copyImageToImage(0, 0, box->imageWidth, box->imageHeight, box, 0, 24, text);
	freeImage(box);
	box = NULL;

    if(logfile[i].ready)
    {
        if(logfile[i].line > logfile[i].rows - (LOG_SCREEN_END - LOG_SCREEN_TOP) - 1) logfile[i].line = logfile[i].rows - (LOG_SCREEN_END - LOG_SCREEN_TOP) - 1;
        if(logfile[i].line < 0) logfile[i].line = 0;
        for(l=LOG_SCREEN_TOP, j=logfile[i].line; j<logfile[i].rows-1; j++)
        {
            if(l<LOG_SCREEN_END)
            {
                char textpad[PSP_LCD_WIDTH] = {""};
                for(k=0; k<PSP_LCD_WIDTH; k++)
                {
                    if(!logfile[i].buf[logfile[i].pos[j]+k]) break;
                    textpad[k] = logfile[i].buf[logfile[i].pos[j]+k];
                }
                if(logfile[i].rows>0xFFFF)
                    printText(text, 5, l*10, WHITE, 0, 0, "0x%08x:  %s", j, textpad);
                else
                    printText(text, 5, l*10, WHITE, 0, 0, "0x%04x:  %s", j, textpad);
                l++;
            }
            else break;
        }
    }
    else if(i == SCRIPT_LOG) printText(text, 5, 30, WHITE, 0, 0, "Log NOT Found: ScriptLog.txt");
    else                     printText(text, 5, 30, WHITE, 0, 0, "Log NOT Found: OpenBorLog.txt");

    printText(text, 185,  11, WHITE, 0, 0, "OpenBOR %s", VERSION);
	printText(text, 20,   11, WHITE, 0, 0, ":");
	printText(text, 27,   11, WHITE, 0, 0, "Home");
	printText(text, 453,  11, WHITE, 0, 0, ":");
	printText(text, 436,  11, WHITE, 0, 0, "End");
	printText(text, 30,  251, WHITE, 0, 0, "Main Log");      // Square
	printText(text, 150, 251, WHITE, 0, 0, "Script Log");    // Triangle
	printText(text, 268, 251, WHITE, 0, 0, "Image Viewer");  // Circle
	printText(text, 392, 251, WHITE, 0, 0, "Exit Logs");     // Cross
	blitImageToScreen(0, 0, PSP_LCD_WIDTH, PSP_LCD_HEIGHT, text, 0, 0);
	sceDisplayWaitVblankStart();
	flipScreen();
}

void drawStartGameBox()
{
	Image *box = createImage(110, 52);
	drawImageBox(box, DARK_GRAY, BLACK, 2);
	printText(box, 34,   8, DARK_YELLOW, 0, 0, "Start Game");
	printText(box, 43,  22, WHITE,  0, 0, "X: Yes");
	printText(box, 43,  34, WHITE,  0, 0, "O: No");
	copyImageToImage(0, 0, box->imageWidth, box->imageHeight, box, 190, 100, text);
	blitImageToScreen(0, 0, PSP_LCD_WIDTH, PSP_LCD_HEIGHT, text, 0, 0);
	sceDisplayWaitVblankStart();
	flipScreen();
	freeImage(box);
	box = NULL;
}

void drawQuitGameBox()
{
	Image *box = createImage(110, 52);
	drawImageBox(box, DARK_GRAY, BLACK, 2);
	printText(box, 36,   8, DARK_YELLOW, 0, 0, "Exit Game");
	printText(box, 43,  22, WHITE,       0, 0, "X: Yes");
	printText(box, 43,  34, WHITE,       0, 0, "O: No");
	copyImageToImage(0, 0, box->imageWidth, box->imageHeight, box, 190, 100, text);
	blitImageToScreen(0, 0, PSP_LCD_WIDTH, PSP_LCD_HEIGHT, text, 0, 0);
	sceDisplayWaitVblankStart();
	flipScreen();
	freeImage(box);
	box = NULL;
}

int ControlBox()
{
	int done = -1;
	while(done == -1)
	{
		switch(getInput(1, 1))
		{
			case PSP_CROSS:
				done = 1;
				break;
			case PSP_CIRCLE:
				done = 0;
				break;
		}
	}
	return done;
}

int ControlMenu()
{
	int status = -1;
	int dListMaxDisplay = 17;
	switch(getInput(1, 1))
	{
		case PSP_DPAD_UP:
			dListScrollPosition--;
			if(dListScrollPosition < 0)
			{
				dListScrollPosition = 0;
				dListCurrentPosition--;
			}
			if(dListCurrentPosition < 0) dListCurrentPosition = 0;
			break;

		case PSP_DPAD_DOWN:
			dListCurrentPosition++;
			if(dListCurrentPosition > dListTotal - 1) dListCurrentPosition = dListTotal - 1;
			if(dListCurrentPosition > dListMaxDisplay)
			{
				if((dListCurrentPosition+dListScrollPosition) < dListTotal) dListScrollPosition++;
				dListCurrentPosition = dListMaxDisplay;
			}
			break;

		case PSP_SQUARE:
			// WiFi Menu!
			status = -2;
			break;

		case PSP_TRIANGLE:
			// BGM Player!
			pControl = ControlBGM;
			status = -3;
			break;

		case PSP_CIRCLE:
			// Image Viewer!
			status = -4;
			break;

		case PSP_CROSS:
			// Log Viewer!
			pControl = ControlLOG;
            which_logfile = OPENBOR_LOG;
			status = -5;
			break;

		case PSP_HOME:
			{
				drawQuitGameBox();
                status = ControlBox() ? 2 : -1;
			}
			break;

		case PSP_START:
			{
				drawStartGameBox();
				status = ControlBox() ? 1 : -1;
			}
			break;

		case PSP_SELECT:
			screenshot(NULL, NULL, 0);
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
	int status = -3;
	int dListMaxDisplay = 17;
	switch(getInput(1, 1))
	{
		case PSP_DPAD_UP:
			dListScrollPosition--;
			if(dListScrollPosition < 0)
			{
				dListScrollPosition = 0;
				dListCurrentPosition--;
			}
			if(dListCurrentPosition < 0) dListCurrentPosition = 0;
			break;

		case PSP_DPAD_DOWN:
			dListCurrentPosition++;
			if(dListCurrentPosition > dListTotal - 1) dListCurrentPosition = dListTotal - 1;
			if(dListCurrentPosition > dListMaxDisplay)
			{
				if((dListCurrentPosition+dListScrollPosition) < dListTotal) dListScrollPosition++;
				dListCurrentPosition = dListMaxDisplay;
			}
			break;

		case PSP_LEFT_TRIGGER:
			if(!bgmStatus || (bgmPlay && bgmCurrent == dListCurrentPosition+dListScrollPosition))
			{
				filelist[bgmCurrent].bgmTrack--;
				if(filelist[bgmCurrent].bgmTrack < 0) filelist[bgmCurrent].bgmTrack = filelist[bgmCurrent].nTracks-1;
				if(bgmStatus) PlayBGM();
			}
			break;

		case PSP_RIGHT_TRIGGER:
			if(!bgmStatus || (bgmPlay && bgmCurrent == dListCurrentPosition+dListScrollPosition))
			{
				filelist[bgmCurrent].bgmTrack++;
				if(filelist[bgmCurrent].bgmTrack > filelist[bgmCurrent].nTracks - 1) filelist[bgmCurrent].bgmTrack = 0;
				if(bgmStatus) PlayBGM();
			}
			break;

		case PSP_SQUARE:
			if(bgmPlay) StopBGM();
			else PlayBGM(filelist[bgmCurrent].bgmTrack);
			break;

		case PSP_TRIANGLE:
			if(!bgmPlay)
			{
				if(bgmLoop) bgmLoop = 0;
				else bgmLoop = 1;
			}
			break;

		case PSP_CIRCLE:
			if(!bgmPlay)
			{
				if(bgmCycle) bgmCycle = 0;
				else bgmCycle = 1;
			}
			break;

		case PSP_CROSS:
			// Return to Main Menu!
			pControl = ControlMenu;
			status = -1;
			break;

		case PSP_HOME:
			{
				drawQuitGameBox();
				status = ControlBox() ? 2 : -3;
			}
			break;

		case PSP_START:
			{
				drawStartGameBox();
				status = ControlBox() ? 1 : -3;
			}
			break;

		case PSP_SELECT:
			screenshot(NULL, NULL, 0);
			break;

		default:
			// No Update Needed!
			status = 0;
			break;
	}
	return status;
}

int ControlLOG()
{
	int status = -5;
	switch(getInput(1, 1))
    {
        case PSP_LEFT_TRIGGER:
            logfile[which_logfile].line=0;
            break;

        case PSP_RIGHT_TRIGGER:
            logfile[which_logfile].line=logfile[which_logfile].rows;
            break;

        case PSP_SQUARE:
            which_logfile = OPENBOR_LOG;
            break;

        case PSP_TRIANGLE:
            which_logfile = SCRIPT_LOG;
            break;

        case PSP_CIRCLE:
            break;

        case PSP_CROSS:
            // Return to Main Menu!
            pControl = ControlMenu;
            status = -1;
            break;

        case PSP_HOME:
            drawQuitGameBox();
            status = ControlBox() ? 2 : -5;
            break;

        case PSP_SELECT:
            screenshot(NULL, NULL, 0);
            break;

        default:
            switch(getInput(0, 0))
            {
                case PSP_DPAD_UP:
                    --logfile[which_logfile].line;
                    break;

                case PSP_DPAD_DOWN:
                    ++logfile[which_logfile].line;
                    break;

                case PSP_DPAD_LEFT:
                    logfile[which_logfile].line-=LOG_SCREEN_END;
                    break;

                case PSP_DPAD_RIGHT:
                    logfile[which_logfile].line+=LOG_SCREEN_END;
                    break;

                default:
                    // No Update Needed!
                    status = 0;
                    break;
            }
            break;
    }
	return status;
}

void menu(char *path)
{
	int done = 0;
	int ctrl = 0;
	char buffer[256] = {""};
	Image *pLoading = NULL;

	text = createImage(PSP_LCD_WIDTH, PSP_LCD_HEIGHT);
	fillImageRect(text, BLACK, 0, 0, PSP_LCD_WIDTH, PSP_LCD_HEIGHT);

	if(getDevkitVersion() < 0x03070110)
	{
		printText(text, 80,  130, RED, 0, 0, "OpenBOR %s, Requires Custom Firmware >= 3.71 M33-3", VERSION);
		blitImageToScreen(0, 0, PSP_LCD_WIDTH, PSP_LCD_HEIGHT, text, 0, 0);
		sceDisplayWaitVblankStart();
		flipScreen();
		freeImage(text);
		text = NULL;
		sceKernelDelayThread(10000000);
		borExit(0);
	}

	sprintf(buffer, "%s/Images/Loading.png", path);
	if(fileExists(buffer)) pLoading = loadImage(buffer);
	if(pLoading != NULL)
	{
		blitImageToScreen(0, 0, PSP_LCD_WIDTH, PSP_LCD_HEIGHT, pLoading, 0, 0);
		sceDisplayWaitVblankStart();
		flipScreen();
		freeImage(pLoading);
		pLoading = NULL;
	}

	sprintf(buffer, "%s/Images/Menu.png", path);
	sprintf(dListPath, "%s/Paks/", path);
	if(fileExists(buffer)) pMenu = loadImage(buffer);
	else
	{
		pMenu = createImage(PSP_LCD_WIDTH, PSP_LCD_HEIGHT);
		fillImageRect(pMenu, ORANGE, 0, 0, PSP_LCD_WIDTH, PSP_LCD_HEIGHT);
	}

	dListCurrentPosition = 0;
	if((dListTotal = findPaks())!=1)
	{
        sortList();
        getAllLogs();
		getAllPreviews();
		packfile_music_read(filelist, dListTotal);
		sound_init(12);
		sound_start_playback(savedata.soundbits, savedata.soundrate);

		pControl = ControlMenu;
		drawMenu();
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

				case -1:
					drawMenu();
					break;

				case -2:
					// Wifi Menu
					break;

				case -3:
					drawBGMPlayer();
					break;

				case -4:
					// ImageViewer
					break;

				case -5:
					drawLogs();
					break;

			}
		}
		sound_exit();
	}
    freeAllLogs();
	freeAllImages();
	tracefree(filelist);
	if(ctrl == 2) borExit(0);
	strncpy(packfile, dListPath, 256);
	strncat(packfile, filelist[dListCurrentPosition+dListScrollPosition].filename,
            strlen(filelist[dListCurrentPosition+dListScrollPosition].filename)+1);
}
