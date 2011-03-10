/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

/////////////////////////////////////////////////////////////////////////////

#include "dcport.h"
#include "timer.h"
#include "openbor.h"
#include "packfile.h"

/////////////////////////////////////////////////////////////////////////////

char packfile[128] = {"bor.pak"};
int cd_lba;

/////////////////////////////////////////////////////////////////////////////

unsigned readmsb32(const unsigned char *src)
{
	return
		((((unsigned)(src[0])) & 0xFF) << 24) |
		((((unsigned)(src[1])) & 0xFF) << 16) |
		((((unsigned)(src[2])) & 0xFF) <<  8) |
		((((unsigned)(src[3])) & 0xFF) <<  0);
}

/////////////////////////////////////////////////////////////////////////////

void borExit(int reset)
{
	tracemalloc_dump();
	arch_reboot();
}

/////////////////////////////////////////////////////////////////////////////

int main()
{
	setSystemRam();
	getRamStatus(BYTES);
	packfile_mode(0);
	if((cd_lba = gdrom_init()) <= 0)
	{
		printf("gdrom_init failed\n");
		arch_reboot();
	}
	openborMain();
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
