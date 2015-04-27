/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2015 OpenBOR Team
 */

#ifndef SAVEDATA_H
#define SAVEDATA_H

#define     MAX_PLAYERS         4

typedef struct
{
    unsigned compatibleversion;
    int gamma;
    int brightness;
    int usesound; // Use SB
    unsigned soundrate; // SB freq
    int soundvol; // SB volume
    int usemusic; // Play music
    int musicvol; // Music volume
    int effectvol; // Sound fx volume
    int soundbits; // SB bits
    int usejoy;
    int mode; // Mode now saves
    int windowpos;
    int keys[MAX_PLAYERS][12];
    int showtitles;
    int videoNTSC;
    int swfilter; // Software scaling filter
    int logo;
    int uselog;
    int debuginfo; // FPS, Memory, etc...
    int fullscreen; // Window or Full Screen Mode
    int stretch; // Stretch (1) or preserve aspect ratio (0) in fullscreen mode
#if SDL
    int usegl[2]; // 1 if OpenGL is preferred over SDL software blitting
    float glscale; // Scale factor for OpenGL
    int glfilter[2]; // Simple or bilinear scaling
#endif
#if PSP
    int pspcpuspeed; // PSP CPU Speed
    int overscan[4]; // Control TV Overscan
    int usetv; // Initilize TV at bootup
#endif

} s_savedata;

extern s_savedata     savedata;

#endif
