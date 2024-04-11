/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef SAVEDATA_H
#define SAVEDATA_H

#define     MAX_PLAYERS         4
#define     MAX_BTN_NUM         13

/**
 *  Only structures written to disk need to be packed.
 */
#pragma pack(1)

typedef struct
{
    unsigned compatibleversion;
    int gamma;
    int brightness;
    int soundvol; // SB volume
    int usemusic; // Play music
    int musicvol; // Music volume
    int effectvol; // Sound fx volume
    int usejoy;
    int mode; // Mode now saves
    int windowpos;
    int keys[MAX_PLAYERS][MAX_BTN_NUM];
    int joyrumble[MAX_PLAYERS];
    int showtitles;
    int videoNTSC;
    int swfilter; // Software scaling filter
    int logo;
    int uselog;
    int debuginfo; // FPS, Memory, etc...
    int fullscreen; // Window or Full Screen Mode
    int stretch; // Stretch (1) or preserve aspect ratio (0) in fullscreen mode
    int screen[1][2];
    int fpslimit; // Sync to monitor refresh (1), limit to 200/500 FPS (2, 3) or don't (0)
#if SDL
    int usegl; // 1 if OpenGL is preferred over SDL software blitting
    float hwscale; // Scale factor for OpenGL
    int hwfilter; // Simple or bilinear scaling
#endif

#if ANDROID
    int is_touchpad_vibration_enabled;
#endif
} s_savedata;

#pragma pack()

extern s_savedata     savedata;

#endif
