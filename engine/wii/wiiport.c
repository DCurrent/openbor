/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
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
	video_init();

// use libfat for FAT filesystem access
	fatInitDefault();

	setSystemRam();
	packfile_mode(0);

	
//new system to get base directory on usb or sd.

	char root[MAX_FILENAME_LEN];
	memset(root, '\0', sizeof(root));
	strncpy(root, argv[0], strrchr(argv[0], '/') - argv[0]);
	
		sprintf(rootDir, "%s/", root);
		sprintf(savesDir, "%s/Saves", root);
		sprintf(paksDir,"%s/Paks", root);
		sprintf(logsDir, "%s/Logs", root);
		sprintf(screenShotsDir, "%s/ScreenShots", root);
		

	dirExists(paksDir, 1);
	dirExists(savesDir, 1);
	dirExists(logsDir, 1);
	dirExists(screenShotsDir, 1);

	Menu();
	openborMain(argc, argv);
	borExit(0);
	return 0;
}

