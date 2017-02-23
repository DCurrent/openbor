/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * Licensed under a BSD-style license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2017 OpenBOR Team
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <psp2/ctrl.h>
#include <psp2/io/dirent.h>
#include <psp2/kernel/threadmgr.h>
#include "vitaport.h"
#include "video.h"
#include "types.h"
#include "utils.h"
#include "packfile.h"
#include "hankaku.h"
#include "stristr.h"
#include "screen.h"
#include "loadimg.h"
#include "timer.h"
#include "version.h"

#include "pngdec.h"
#include "../resources/OpenBOR_Logo_480x272_png.h"
#include "../resources/OpenBOR_Menu_480x272_Sony_png.h"

#define RGB32(R,G,B) ((B << 16) | ((G) << 8) | (R))
#define RGB(R,G,B)   RGB32(R,G,B)

#define BLACK        RGB(  0,   0,   0)
#define WHITE        RGB(255, 255, 255)
#define RED            RGB(255,   0,   0)
#define    GREEN        RGB(  0, 255,   0)
#define BLUE        RGB(  0,   0, 255)
#define YELLOW        RGB(255, 255,   0)
#define PURPLE        RGB(255,   0, 255)
#define ORANGE        RGB(255, 128,   0)
#define GRAY        RGB(112, 128, 144)
#define LIGHT_GRAY  RGB(223, 223, 223)
#define DARK_RED    RGB(128,   0,   0)
#define DARK_GREEN    RGB(  0, 128,   0)
#define DARK_BLUE    RGB(  0,   0, 128)

#define LOG_SCREEN_TOP 2
#define LOG_SCREEN_END 26

typedef struct{
    stringptr *buf;
    int *pos;
    int line;
    int rows;
    char ready;
}s_logfile;

static s_screen *Source = NULL;
static s_screen *Screen = NULL;
static int dListTotal;
static int dListCurrentPosition;
static int dListScrollPosition;
static int which_logfile = OPENBOR_LOG;
static unsigned int buttonsHeld = 0;
static unsigned int buttonsPressed = 0;
static fileliststruct *filelist;
static s_logfile logfile[2];

typedef int (*ControlInput)();
static ControlInput pControl;

static int ControlMenu();

static int Control()
{
    return pControl();
}

static void refreshInput()
{
    SceCtrlData pad;
    memset(&pad, 0, sizeof(pad));
    sceCtrlPeekBufferPositive(0, &pad, 1);

    // read left analog stick
    if (pad.ly <= 0x30) pad.buttons |= SCE_CTRL_UP;
    if (pad.ly >= 0xC0) pad.buttons |= SCE_CTRL_DOWN;
    if (pad.lx <= 0x30) pad.buttons |= SCE_CTRL_LEFT;
    if (pad.lx >= 0xC0) pad.buttons |= SCE_CTRL_RIGHT;

    // update buttons pressed (not held)
    buttonsPressed = pad.buttons & ~buttonsHeld;
    buttonsHeld = pad.buttons;
}

static void getAllLogs()
{
    int i, j, k;
    for (i=0; i<2; i++)
    {
        logfile[i].buf = readFromLogFile(i);
        if (logfile[i].buf != NULL)
        {
            logfile[i].pos = malloc(++logfile[i].rows * sizeof(int));
            if (logfile[i].pos == NULL) return;
            memset(logfile[i].pos, 0, logfile[i].rows * sizeof(int));

            for (k=0, j=0; j<logfile[i].buf->size; j++)
            {
                if (!k)
                {
                    logfile[i].pos[logfile[i].rows - 1] = j;
                    k = 1;
                }
                if (logfile[i].buf->ptr[j]=='\n')
                {
                    int *pos = malloc(++logfile[i].rows * sizeof(int));
                    if (pos == NULL) return;
                    memcpy(pos, logfile[i].pos, (logfile[i].rows - 1) * sizeof(int));
                    pos[logfile[i].rows - 1] = 0;
                    free(logfile[i].pos);
                    logfile[i].pos = NULL;
                    logfile[i].pos = malloc(logfile[i].rows * sizeof(int));
                    if (logfile[i].pos == NULL) return;
                    memcpy(logfile[i].pos, pos, logfile[i].rows * sizeof(int));
                    free(pos);
                    pos = NULL;
                    logfile[i].buf->ptr[j] = 0;
                    k = 0;
                }
                if (logfile[i].buf->ptr[j]=='\r') logfile[i].buf->ptr[j] = 0;
                if (logfile[i].rows>0xFFFFFFFE) break;
            }
            logfile[i].ready = 1;
        }
    }
}

static void freeAllLogs()
{
    int i;
    for (i=0; i<2; i++)
    {
        if (logfile[i].ready)
        {
            free(logfile[i].buf);
            logfile[i].buf = NULL;
            free(logfile[i].pos);
            logfile[i].pos = NULL;
        }
    }
}

static void sortList()
{
    int i, j;
    fileliststruct temp;
    if (dListTotal<2) return;
    for (j=dListTotal-1; j>0; j--)
    {
        for (i=0; i<j; i++)
        {
            if (stricmp(filelist[i].filename, filelist[i+1].filename)>0)
            {
                temp = filelist[i];
                filelist[i] = filelist[i+1];
                filelist[i+1] = temp;
            }
        }
    }
}

static int findPaks(void)
{
    int i = 0;
    SceUID dp = NULL;
    SceIoDirent ds;

    dp = sceIoDopen(paksDir);

    while (sceIoDread(dp, &ds) > 0)
    {
        if (packfile_supported(ds.d_name))
        {
            fileliststruct *copy = NULL;
            if (filelist == NULL) filelist = malloc(sizeof(fileliststruct));
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
            strncpy(filelist[i].filename, ds.d_name, strlen(ds.d_name));
            i++;
        }
    }
    sceIoDclose(dp);
    return i;
}

static void printText(int x, int y, int col, int backcol, int fill, const char *format, ...)
{
    int x1, y1, i;
    uint32_t data;
    uint32_t *line32 = NULL;
    unsigned char *font;
    unsigned char ch = 0;
    char buf[128] = {""};

    va_list arglist;
    va_start(arglist, format);
    vsprintf(buf, format, arglist);
    va_end(arglist);

    for (i = 0; i < sizeof(buf); i++)
    {
        ch = buf[i];
        // mapping
        if (ch < 0x20)      ch = 0;
        else if (ch < 0x80) ch -= 0x20;
        else if (ch < 0xa0) ch = 0;
        else                ch -= 0x40;
        font = (u8*)&hankaku_font10[ch*10];
        // draw
        line32 = (uint32_t*)Screen->data + x + y * Screen->width;

        for (y1 = 0; y1 < 10; y1++)
        {
            data = *font++;
            for (x1 = 0; x1 < 5; x1++)
            {
                if (data & 1)  *line32 = col;
                else if (fill) *line32 = backcol;

                line32++;
                data = data >> 1;
            }
            line32 += Screen->width-5;
        }
        x += 5;
    }
}

static s_screen *getPreview(char *filename)
{
    s_screen *title = NULL;
    s_screen *scale = NULL;
    // Grab current path and filename
    getBasePath(packfile, filename, 1);
    // Create & Load & Scale Image
    if (!loadscreen("data/bgs/title", packfile, NULL, PIXEL_x8, &title)) return NULL;
    scale = allocscreen(160, 120, PIXEL_x8);
    scalescreen(scale, title);
    memcpy(scale->palette, title->palette, PAL_BYTES);
    // ScreenShots within Menu will be saved as "Menu"
    strncpy(packfile,"Menu.xxx",128);
    freescreen(&title);
    return scale;
}

static void getAllPreviews()
{
    int i;
    for (i = 0; i < dListTotal; i++)
    {
        filelist[i].preview = getPreview(filelist[i].filename);
    }
}

static void freeAllPreviews()
{
    int i;
    for (i = 0; i < dListTotal; i++)
    {
        if (filelist[i].preview != NULL)
            freescreen(&filelist[i].preview);
    }
}

static int ControlMenu()
{
    int status = -1;
    int dListMaxDisplay = 17;

    refreshInput();
    switch(buttonsPressed)
    {
        case SCE_CTRL_UP:
            dListScrollPosition--;
            if (dListScrollPosition < 0)
            {
                dListScrollPosition = 0;
                dListCurrentPosition--;
            }
            if (dListCurrentPosition < 0) dListCurrentPosition = 0;
            break;

        case SCE_CTRL_DOWN:
            dListCurrentPosition++;
            if (dListCurrentPosition > dListTotal - 1) dListCurrentPosition = dListTotal - 1;
            if (dListCurrentPosition > dListMaxDisplay)
            {
                if ((dListCurrentPosition+dListScrollPosition) < dListTotal) dListScrollPosition++;
                dListCurrentPosition = dListMaxDisplay;
            }
            break;

        case SCE_CTRL_LEFT:
            break;

        case SCE_CTRL_RIGHT:
            break;

        case SCE_CTRL_CROSS:
            // Start Engine!
            status = 1;
            break;

        case SCE_CTRL_SQUARE:
            // Show Logs!
            status = 3;
            break;

        default:
            // No Update Needed!
            status = 0;
            break;
    }

    return status;
}

static void initMenu(int type)
{
    // Read Logo or Menu from Array.
    if (!type) Source = pngToScreen((void*) openbor_logo_480x272_png.data);
    else Source = pngToScreen((void*) openbor_menu_480x272_sony_png.data);

    Screen = allocscreen(480, 272, PIXEL_32);
}

static void termMenu()
{
    freescreen(&Source);
    freescreen(&Screen);
}

static void drawMenu()
{
    s_screen *Image = NULL;
    char listing[45] = {""}, *extension;
    int list = 0;
    int shift = 0;
    int colors = 0;

    copyscreen(Screen, Source);
    if (dListTotal < 1) printText(30, 33, RED, 0, 0, "No Games In Paks Folder!");
    for (list = 0; list < dListTotal; list++)
    {
        if (list < 18)
        {
            shift = 0;
            colors = GRAY;
            listing[0] = '\0';
            strncat(listing, filelist[list+dListScrollPosition].filename, sizeof(listing));
            extension = strrchr(listing, '.');
            if (extension) *extension = '\0';
            
            if (list == dListCurrentPosition)
            {
                shift = 2;
                colors = RED;
                Image = filelist[list+dListScrollPosition].preview;
                if (Image) putscreen(Screen, Image, 286, 32, NULL);
                else printText(288, 141, RED, 0, 0, "No Preview Available!");
            }
            printText(30 + shift, 33+(11*list), colors, 0, 0, "%s", listing);
        }
    }

    printText( 26,  11, WHITE, 0, 0, "OpenBoR %s", VERSION);
    printText(392,  11, WHITE, 0, 0, __DATE__);
    printText( 28, 251, WHITE, 0, 0, "View Logs");   // Square
    printText(150, 251, WHITE, 0, 0, "");            // Triangle
    printText(268, 251, WHITE, 0, 0, "");            // Circle
    printText(392, 251, WHITE, 0, 0, "Start Game");  // Cross
    printText(320, 175, BLACK, 0, 0, "www.chronocrash.com");
    printText(322, 185, BLACK, 0, 0, "www.senileteam.com");

#ifdef SPK_SUPPORTED
    printText(324,191, DARK_RED, 0, 0, "SecurePAK Edition");
#endif

    video_copy_screen(Screen);
}

static void drawLogs()
{
    int i = which_logfile, j, k, l, done = 0;

    while (!done)
    {
        clearscreen(Screen);
        refreshInput();
        printText(410, 3, RED, 0, 0, "Quit : Circle");
        if (buttonsPressed & SCE_CTRL_CIRCLE) done = 1;

        if (logfile[i].ready)
        {
            printText(5, 3, RED, 0, 0, "OpenBorLog.txt");
            if (buttonsHeld & SCE_CTRL_UP)    --logfile[i].line;
            if (buttonsHeld & SCE_CTRL_DOWN)  ++logfile[i].line;
            if (buttonsHeld & SCE_CTRL_LEFT)  logfile[i].line = 0;
            if (buttonsHeld & SCE_CTRL_RIGHT) logfile[i].line = logfile[i].rows - (LOG_SCREEN_END - LOG_SCREEN_TOP);
            if (logfile[i].line > logfile[i].rows - (LOG_SCREEN_END - LOG_SCREEN_TOP) - 1) logfile[i].line = logfile[i].rows - (LOG_SCREEN_END - LOG_SCREEN_TOP) - 1;
            if (logfile[i].line < 0) logfile[i].line = 0;
            for (l=LOG_SCREEN_TOP, j=logfile[i].line; j<logfile[i].rows-1; l++, j++)
            {
                if (l < LOG_SCREEN_END)
                {
                    char textpad[480] = {""};
                    for (k = 0; k < 480; k++)
                    {
                        if (!logfile[i].buf->ptr[logfile[i].pos[j]+k]) break;
                        textpad[k] = logfile[i].buf->ptr[logfile[i].pos[j]+k];
                    }
                    if (logfile[i].rows > 0xFFFF)
                        printText(5, l*10, WHITE, 0, 0, "0x%08x:  %s", j, textpad);
                    else
                        printText(5, l*10, WHITE, 0, 0, "0x%04x:  %s", j, textpad);
                }
                else break;
            }
        }
        else if (i == SCRIPT_LOG) printText(5, 3, RED, 0, 0, "Log NOT Found: ScriptLog.txt");
        else                      printText(5, 3, RED, 0, 0, "Log NOT Found: OpenBorLog.txt");

        video_copy_screen(Screen);
    }
    drawMenu();
}

static void drawLogo()
{
    initMenu(0);
    //copyscreen(Screen, Source);
    video_copy_screen(Source);
    unsigned int startTime = timer_gettick();

    // The logo displays for 2 seconds.  Let's put that time to good use.
    dListTotal = findPaks();
    getAllPreviews();
    sortList();
    getAllLogs();

    // Display logo for the rest of the 2 seconds
    while (timer_gettick() - startTime < 2000)
    {
        sceKernelDelayThread(1000);
    }
    termMenu();
}

static void setVideoMode()
{
    s_videomodes videomodes;
    videomodes.hRes  = 480;
    videomodes.vRes  = 272;
    videomodes.pixel = 4;
    video_set_mode(videomodes);
    screenformat = PIXEL_32;
}

void Menu()
{
    int done = 0;
    int ctrl = 0;

    setVideoMode();
    drawLogo();

    dListCurrentPosition = 0;
    if (dListTotal != 1)
    {
        initMenu(1);
        drawMenu();
        pControl = ControlMenu;

        while (!done)
        {
            ctrl = Control();
            switch (ctrl)
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
            }
        }
        freeAllLogs();
        freeAllPreviews();
        termMenu();
        if (ctrl == 2)
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
    screenformat = PIXEL_8;
}

