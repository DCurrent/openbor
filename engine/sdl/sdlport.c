/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include "sdlport.h"
#include "packfile.h"
#include "ram.h"
#include "video.h"
#include "menu.h"
#include "stacktrace.h"

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
char paksDir[128] = {"Paks"};
char savesDir[128] = {"Saves"};
char logsDir[128] = {"Logs"};
char screenShotsDir[128] = {"ScreenShots"};

void borExit(int reset)
{
	tracemalloc_dump();

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
	
	dirExists(paksDir, 1);
	dirExists(savesDir, 1);
	dirExists(logsDir, 1);
	dirExists(screenShotsDir, 1);
	
	Menu();
	openborMain();
	borExit(0);
	return 0;
}

