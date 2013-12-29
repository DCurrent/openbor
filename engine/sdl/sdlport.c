/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include "sdlport.h"
#include "packfile.h"
#include "ram.h"
#include "video.h"
#include "menu.h"

#ifdef DARWIN
#include <CoreFoundation/CoreFoundation.h>
#elif WII
#include <fat.h>
#elif WIN
#undef main
#endif

#ifdef SDL
#define appExit exit
#undef exit
#endif

char packfile[128] = {"bor.pak"};
#if ANDROID
#include <unistd.h>
char rootDir[128] = {"/mnt/sdcard/OpenBOR"};
#endif
char paksDir[128] = {"Paks"};
char savesDir[128] = {"Saves"};
char logsDir[128] = {"Logs"};
char screenShotsDir[128] = {"ScreenShots"};

void borExit(int reset)
{

#ifdef GP2X
	gp2x_end();
	chdir("/usr/gp2x");
	execl("/usr/gp2x/gp2xmenu", "/usr/gp2x/gp2xmenu", NULL);
#else
	SDL_Delay(1000);
#endif

	appExit(0);
}

int main(int argc, char *argv[])
{
#ifndef SKIP_CODE
	char pakname[256];
#endif
#ifdef CUSTOM_SIGNAL_HANDLER
	struct sigaction sigact;
#endif

#ifdef DARWIN
	char resourcePath[PATH_MAX];
	CFBundleRef mainBundle;
	CFURLRef resourcesDirectoryURL;
	mainBundle = CFBundleGetMainBundle();
	resourcesDirectoryURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
	if(!CFURLGetFileSystemRepresentation(resourcesDirectoryURL, true, (UInt8 *) resourcePath, PATH_MAX))
	{
		borExit(0);
	}
	CFRelease(resourcesDirectoryURL);
	chdir(resourcePath);
#elif WII
	fatInitDefault();
#endif

#ifdef CUSTOM_SIGNAL_HANDLER
	sigact.sa_sigaction = handleFatalSignal;
	sigact.sa_flags = SA_RESTART | SA_SIGINFO;

	if(sigaction(SIGSEGV, &sigact, NULL) != 0)
	{
		printf("Error setting signal handler for %d (%s)\n", SIGSEGV, strsignal(SIGSEGV));
		exit(EXIT_FAILURE);
	}
#endif

	setSystemRam();
	initSDL();

	packfile_mode(0);
#ifdef ANDROID
	dirExists(rootDir, 1);
    chdir(rootDir);
#endif
	dirExists(paksDir, 1);
	dirExists(savesDir, 1);
	dirExists(logsDir, 1);
	dirExists(screenShotsDir, 1);

#ifdef ANDROID
    if(dirExists("/mnt/usbdrive/OpenBOR/Paks", 0))
        strcpy(paksDir, "/mnt/usbdrive/OpenBOR/Paks");
    else if(dirExists("/usbdrive/OpenBOR/Paks", 0))
        strcpy(paksDir, "/usbdrive/OpenBOR/Paks");
    else if(dirExists("/mnt/extsdcard/OpenBOR/Paks", 0))
        strcpy(paksDir, "/mnt/extsdcard/OpenBOR/Paks");
#endif

	Menu();
#ifndef SKIP_CODE
	getPakName(pakname, -1);
	video_set_window_title(pakname);
#endif
	openborMain(argc, argv);
	borExit(0);
	return 0;
}

