#include "dosport.h"
#include "packfile.h"

char packfile[128] = {"bor.pak"};

void borExit(int reset)
{
	tracemalloc_dump();
	exit(reset);
}

void main(int argc, char *argv[])
{
	TOTAL_SYSTEM_RAM = memoryAvailable();
	dirExists("Paks", 1);
	dirExists("Saves", 1);
	dirExists("Logs", 1);
	dirExists("SShots", 1);
	packfile_mode(0);
	openborMain();
	borExit(0);
}

