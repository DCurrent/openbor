/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef NETCOMM_H
#define NETCOMM_H

#include <pspkerneltypes.h>

#define MAX_APS	20
int		chooseAPS;
int		wifiMode;
int		wifiError;
SceUID  NetThid;

typedef struct
{
	int  index;
	int  color;
	union SceNetApctlInfo *pInfo;
}
APS;

typedef struct
{
	int  index;
	char name[128];
	int  server;
	char srcMac[32];
	char srcWan[32];
	char srcIP[32];
	int  srcPort;
	char dstMac[32];
	char dstWan[32];
	char dstIP[32];
	int  dstPort;
	unsigned long buttons;
	unsigned long ticks;
}
PSPdata;

int listAccessPoint();
int startWifi();
unsigned long getNetPad(int port);

#endif
