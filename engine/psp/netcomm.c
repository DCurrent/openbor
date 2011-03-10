/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <pspnet.h>
#include <psppower.h>
#include <pspkernel.h>
#include <pspnet_inet.h>
#include <pspnet_apctl.h>
#include <pspnet_resolver.h>
#include <psputility_netparam.h>
#include <psputility_netmodules.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include "utils.h"
#include "netcomm.h"
#include "control.h"
#include "graphics.h"

#define	SERVER_PORT 5060
#define BUFFER_SIZE 65536

int		pspTotal;
APS		AccessPoints[MAX_APS];
PSPdata	pspList[MAX_APS];

unsigned long getNetPad(int port)
{
	return pspList[port].buttons;
}

int addPSP(char *name, char *srcMac, char *srcWan, char *srcIP, int srcPort, char *dstMac, char *dstWan, char *dstIP, int dstPort, int server)
{
	int i;
	/* Only want to show up servers */
	if(server == 1) return 0;
	for(i=0; i<MAX_APS; i++)
	{
		/* Don't add duplicate entries */
		if(strlen(pspList[i].name))
		{
			if(stricmp(pspList[i].srcMac, srcMac)==0) return 0;
			else break;
		}
		else break;
	}
	if(pspTotal == MAX_APS) return 0;
	strncpy(pspList[pspTotal].name,    name,    strlen(name));
	strncpy(pspList[pspTotal].srcMac,  srcMac,  8);
	strncpy(pspList[pspTotal].srcWan,  srcWan,  8);
	strncpy(pspList[pspTotal].srcIP,   srcIP,   12);
	pspList[pspTotal].srcPort = srcPort;
	strncpy(pspList[pspTotal].dstMac,  dstMac,  8);
	strncpy(pspList[pspTotal].dstWan,  dstWan,  8);
	strncpy(pspList[pspTotal].dstIP,   dstIP,   12);
	pspList[pspTotal].dstPort = dstPort;
	pspList[pspTotal].server  = server;
	pspList[pspTotal].buttons = 0;
	pspTotal++;
	return 1;
}


void startServer()
{
	int             i, sock;
	struct			sockaddr_in	  server;
	struct          sockaddr_in   client;
	char			buffer[BUFFER_SIZE];
	unsigned int	buffer_len = 0;
	unsigned int    client_len = 0;
	unsigned int	server_len = 0;
	int             bytesTX = 0;
	int             bytesRX = 0;
	uint16_t		port;

	if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) goto Error;

	port = SERVER_PORT;

	/* Create sockaddr structure */
	memset(&server, 0, sizeof(server));
	server.sin_family      = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port        = htons(port);

	/* Bind the socket */
	server_len = sizeof(server);
	if(bind(sock, (struct sockaddr *) &server, server_len) < 0) goto Error;

	/* Run until cancelled */
	while(wifiMode)
	{
		/* Keeps Wifi from turning off */
		scePowerTick(1);
		/* Receive datagram from client */
		client_len = sizeof(client);
		bytesRX = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &client, (void*) &client_len);
		if(bytesRX < 0)
		{
			writeToLogFile("Critical Error failed to receive message from client!\n");
			wifiError = 1;
		}

		//if(bytesRX > 0) pspList[1].buttons = atol(buffer);


		//if(bytesRX == 3)
		//{
		//	strcat(buffer,"\n\nAppended!!!");
		//	strcat(buffer,"\n\nI");
		//	strcat(buffer,"\n\nLove My Muffin and My Chloe!!!");
		//	strcat(buffer,"\n\nXoXoXoXoXoXoX");
		//}

		//sprintf(buffer, "%lu", getPad(0));
		buffer_len = strlen(buffer);
		if(buffer_len > 0) writeToLogFile("Buffer: %lu\n",buffer);
		bytesTX = sendto(sock, buffer, buffer_len, 0, (struct sockaddr *) &client, sizeof(client));

		if(bytesTX < buffer_len)
		{
			writeToLogFile("Data Sent: %d\nBuffer Size: %d\nBuffer Data: %s\n", bytesTX, buffer_len, buffer);
			wifiError = 1;
		}
		for(i=0; i<buffer_len; i++) buffer[i] = 0;
	}

Error:
	wifiError = 1;
	return;
}

int listAccessPoint()
{
	int pick_count = 0;
	int iNetIndex;

	// skip the 0th connection
	for(iNetIndex = 1; iNetIndex < 100; iNetIndex++)
	{
		if(sceUtilityCheckNetParam(iNetIndex) == 0)
		{
			sceUtilityGetNetParam(iNetIndex, PSP_NETPARAM_NAME, (void*)AccessPoints[pick_count].pInfo->name);
			strcpy(AccessPoints[pick_count].pInfo->ip,"Acquiring...");
			AccessPoints[pick_count].color = ORANGE;
			AccessPoints[pick_count].index = iNetIndex;
			pick_count++;
			if(pick_count >= MAX_APS)
			{
				wifiError = 1;
				break;
			}
		}
		else break;
	}

	if(pick_count == 0)
	{
		writeToLogFile("No connections\n");
		writeToLogFile("Please try Network Settings\n");
		sceKernelDelayThread(1000000); // 1sec to read before exit
		wifiError = 1;
		return -1;
	}
	return 0;
}

/* Select && Connect to an Access Point */
int selectAccessPoint(int selected)
{
	int err;
	int stateLast = -1;
	int state;

	/* Connect using the first profile */
	err = sceNetApctlConnect(selected);
	if (err != 0)
	{
		writeToLogFile("Error, sceNetApctlConnect returns %08X\n", err);
		wifiError = 1;
		return -1;
	}

	//printf("Connecting...\n");
	while(1)
	{
		err = sceNetApctlGetState(&state);
		if(err != 0)
		{
			writeToLogFile("sceNetApctlGetState returns $%x\n", err);
			wifiError = 1;
			break;
		}
		if(state > stateLast)
		{
			//printf("Connection state %d of 4\n", state);
			stateLast = state;
		}
		/* Connected with IP Address*/
		if(state == 4) break;
		/* Wait 50 ms before polling again */
		sceKernelDelayThread(50*1000);
	}
	/* Now obtain IP Address */
	while(1)
	{
		if(sceNetApctlGetInfo(8, AccessPoints[selected-1].pInfo) == 0)
		{
			AccessPoints[selected-1].color = DARK_GREEN;
			addPSP("SamuraiX", "11:11:11:11", "22:22:22:22", AccessPoints[selected-1].pInfo->ip, 5060, "88:88:88:88", "99:99:99:99", AccessPoints[selected-1].pInfo->ip, 5060, 0);
			break;
		}
		sceKernelDelayThread(1000 * 1000);
	}
	if(err != 0)
	{
		wifiError = 1;
		return -1;
	}
	return 1;
}

int net_thread(SceSize args, void *argp)
{
	while(1)
	{
		/* Offset 1 needed for sceAPI */
		if(selectAccessPoint(chooseAPS+1)) startServer();
	}
	return 0;
}

/* Simple user thread to do the real work */
int startWifi()
{
	NetThid = sceKernelCreateThread("net_thread", net_thread, 0x12, 0x20000, PSP_THREAD_ATTR_USER, NULL);
	if(NetThid < 0)
	{
		writeToLogFile("Error, could not create thread\n");
		wifiError = 1;
		return -1;
	}
	sceKernelStartThread(NetThid, 0, NULL);
	return 1;
}
