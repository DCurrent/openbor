/*
 * OpenBOR - https://www.chronocrash.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#include <ogcsys.h>
#include <stdlib.h>
#include <string.h>
#include "wiiport.h"
#include "packfile.h"
#include "video.h"
#include "control.h"
#include "utils.h"
#include "ram.h"
#include "menu.h"
#include <fat.h>

extern void __exception_setreload(int t);

char packfile[MAX_FILENAME_LEN];
char paksDir[MAX_FILENAME_LEN];
char savesDir[MAX_FILENAME_LEN];
char logsDir[MAX_FILENAME_LEN];
char screenShotsDir[MAX_FILENAME_LEN];
char rootDir[MAX_FILENAME_LEN]; // note: this one ends with a slash

/*
 * Given a file's path relative to the OpenBOR executable;
 */
char* getFullPath(char *relPath)
{
	static char filename[MAX_FILENAME_LEN];
	strcpy(filename, rootDir);
	strcat(filename, relPath);
	return filename;
}

void borExit(int reset)
{
#if 0
	if(reset == WII_SHUTDOWN)   SYS_ResetSystem(SYS_POWEROFF, 0, 0);
	else if(reset == WII_RESET) SYS_ResetSystem(SYS_HOTRESET, 0, 0);
	else exit(reset);
#else
    exit(reset); //SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
#endif
}

// TODO merge into getFullPath
void initDirPath(char* dest, char* relPath)
{
	strcpy(dest, rootDir);
	strcat(dest, relPath);
}

int main(int argc, char * argv[])
{
	// Launch a pak directly from a loader with plugin ability(Wiiflow, Postloader etc).
	// With WiiFlow, the first definable plugin's argument in openbor.ini is argv[1].
	int directlaunch = (argc > 1 && (argv[1][0] == 'u' || argv[1][0] == 's')) ? 1 : 0;
	
	video_init();

	// Reset after 8 seconds after a crash
	__exception_setreload(8);

	// reload to IOS58 for USB2 support
	if (IOS_GetVersion() != 58)
	{       
        IOS_ReloadIOS(58);
	}

	// use libfat for FAT filesystem access
	int retry = 0;
	int fatMounted = 0;

	// try to mount FAT devices during 3 seconds
	while (!fatMounted && (retry < 12))
	{
		fatMounted = fatInitDefault();
		usleep(250000);
		retry++;
	}

	setSystemRam();
	packfile_mode(0);
	
	//new system to get base directory on usb or sd.
	char root[MAX_FILENAME_LEN];
	memset(root, '\0', sizeof(root));
	
	// Root path sent by the loader's argument(apps/OpenBOR by default)
	if(directlaunch)
	{
		strncpy(root, argv[1], strrchr(argv[1], '/') - argv[1]);
	}
	else
	{
		strncpy(root, argv[0], strrchr(argv[0], '/') - argv[0]);
	}

	snprintf(rootDir, sizeof(rootDir), "%s/", root);
	snprintf(savesDir, sizeof(savesDir), "%s/Saves", root);
	snprintf(paksDir, sizeof(paksDir), "%s/Paks", root);
	snprintf(logsDir, sizeof(logsDir), "%s/Logs", root);
	snprintf(screenShotsDir, sizeof(screenShotsDir), "%s/ScreenShots", root);
		

	dirExists(paksDir, 1);
	dirExists(savesDir, 1);
	dirExists(logsDir, 1);
	dirExists(screenShotsDir, 1);
	
	// Pack's name sent by the loader's argument
	if(directlaunch)
	{
		getBasePath(packfile, argv[2], 1);
	}

	Menu();
	openborMain(argc, argv);
	borExit(0);
	return 0;
}

