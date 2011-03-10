/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

/////////////////////////////////////////////////////////////////////////////

#include "dcport.h"

#include "bios.h"
#include "gdrom.h"

/////////////////////////////////////////////////////////////////////////////

#define CMD_PIOREAD     16
#define CMD_DMAREAD     17
#define CMD_GETTOC      18
#define CMD_GETTOC2     19
#define CMD_PLAY        20
#define CMD_PLAY2       21
#define CMD_PAUSE       22
#define CMD_RELEASE     23
#define CMD_INIT        24
#define CMD_SEEK        27
#define CMD_READ        28
#define CMD_STOP        33
#define CMD_GETSCD      34
#define CMD_GETSES      35

#define GDROM_STATUS_FAILED  (-1)
#define GDROM_STATUS_BUSY     (0)
#define GDROM_STATUS_PAUSED   (1)
#define GDROM_STATUS_STANDBY  (2)
#define GDROM_STATUS_PLAYING  (3)
#define GDROM_STATUS_SEEKING  (4)
#define GDROM_STATUS_SCANNING (5)
#define GDROM_STATUS_LIDOPEN  (6)
#define GDROM_STATUS_NODISC   (7)

static struct {
  unsigned entry[99];
  unsigned first, last;
  unsigned dunno;
} gdrom_toc;

// Split TOC->entry into its components
#define GDROM_TOC_LBA(n) ((n)&0x00FFFFFF)
#define GDROM_TOC_ADR(n) (((n)>>24)&0xF)
#define GDROM_TOC_CTRL(n) (((n)>>28)&0xF)
// Use TOC_TRACK() to convert TOC->first and TOC->last entry to track #
#define GDROM_TOC_TRACK(n) (((n)>>16)&0xFF)

/////////////////////////////////////////////////////////////////////////////

#define MAXREAD (8)
static char gdrom_readbuffer[(2048*MAXREAD)+32];
#define GDROM_READBUFFER_P2 ((char*)((((int)gdrom_readbuffer)|0xA000001F)+1))

/////////////////////////////////////////////////////////////////////////////

static int   currently_executing_read_lba     = -1;
static int   currently_executing_read_size    = 0;
static char *currently_executing_read_dest    = NULL;
static int   currently_executing_read_q       = 0;
static int   currently_executing_read_retries = 0;
//static int   currently_executing_read_reinit  = 0;

/////////////////////////////////////////////////////////////////////////////

static void gdrom_queue_current_read(void) {
  int param[4];
  int s = currently_executing_read_size;
  if(s > MAXREAD) s = MAXREAD;
  param[0] = currently_executing_read_lba;
  param[1] = s;
  param[2] = (int)GDROM_READBUFFER_P2;
  param[3] = 0; // dunno
//  currently_executing_read_q = bios_gdGdcReqCmd(CMD_PIOREAD, param);
  currently_executing_read_q = bios_gdGdcReqCmd(CMD_DMAREAD, param);
}

/////////////////////////////////////////////////////////////////////////////
//
// Returns nonzero if busy
//
int gdrom_poll(void) {
  int param[4];
  int r, s;
  bios_gdGdcExecServer();
  if(currently_executing_read_lba < 0) return 0;
  param[0] = 0; param[1] = 0; param[2] = 0; param[3] = 0;
  r = bios_gdGdcGetCmdStat(currently_executing_read_q, param);
  if(r == 1) return 1;

  // retry needed
  if(r != 2) {
//printf("gdrom retry needed(r=%d, lba=%d, size=%d)\n",r,currently_executing_read_lba,currently_executing_read_size);
	currently_executing_read_retries++;
	gdrom_queue_current_read();
	return 1;
  }

  // last read op was a success
  currently_executing_read_retries = 0;

  // done with one piece
  s = currently_executing_read_size;
  if(s > MAXREAD) s = MAXREAD;
  memcpy(
	currently_executing_read_dest,
	GDROM_READBUFFER_P2,
	2048*s
  );
  currently_executing_read_lba += s;
  currently_executing_read_size -= s;
  currently_executing_read_dest += 2048*s;
  // done?
  if(currently_executing_read_size <= 0) {
	currently_executing_read_lba = -1;
	return 0;
  }

  // otherwise queue up the next piece
  gdrom_queue_current_read();
  return 1;
}

/////////////////////////////////////////////////////////////////////////////
//
// Blocks until the prior read is done (you can ensure it won't block if you
// check gdrom_poll yourself), and then queues up a new read
//
// Danger: you do not want to try doing this on out-of-range sectors
//
void gdrom_readsectors(void *dest, int lba, int num) {
  while(gdrom_poll());
//printf("gdrom_readsectors(dest,lba=%d,num=%d)\n",lba,num);

  currently_executing_read_dest = dest;
  currently_executing_read_lba  = lba;
  currently_executing_read_size = num;
  gdrom_queue_current_read();
}

/////////////////////////////////////////////////////////////////////////////
//
// quick and dirty blocking read
//
static int quickblockread(int lba, int num) {
  int i, j;
  int param[4];
  param[0] = lba;
  param[1] = num;
  param[2] = (int)GDROM_READBUFFER_P2;
  param[3] = 0;
  i = bios_gdGdcReqCmd(CMD_PIOREAD, param);
  for(;;) {
	bios_gdGdcExecServer();
	param[0] = 0; param[1] = 0; param[2] = 0; param[3] = 0;
	j = bios_gdGdcGetCmdStat(i, param);
	if(j != 1) break;
  }
  return j;
}

/////////////////////////////////////////////////////////////////////////////
//
// Initialize gdrom system
// returns the starting LBA of the main data track
//
int gdrom_init(void) {
  int param[4];
  int i, j;
  int first, last, track;
  int probable_lba = -1;
  int probable2_lba = -1;
  int definite_lba = -1;

  // Reactivate GD-ROM drive
  *((volatile int*)0xA05F74E4) = 0x001FFFFF;
  for(i = 0; i<(0x200000/4); i++) {
	j += ((volatile int*)0xA0000000)[i];
  }

  // Reset GD system functions
  bios_gdGdcInitSystem();

  // Try to initialize the disc
tryinit:

  i = bios_gdGdcReqCmd(CMD_INIT, NULL);
  for(;;) {
	bios_gdGdcExecServer();
	param[0] = 0; param[1] = 0; param[2] = 0; param[3] = 0;
	j = bios_gdGdcGetCmdStat(i, param);
	if(j != 1) break;
  }
  if(j != 2) goto tryinit;

  // Set the read mode
  param[0] = 0;     // 0 = set, 1 = get
  param[1] = 8192;  // ?
  param[2] = 0;     // autodetect read mode
  param[3] = 2048;  // sector size
  if(bios_gdGdcChangeDataType(param) < 0) goto tryinit;

  // Try to read the TOC
  param[0] = 0; // session
  param[1] = (int)(&gdrom_toc); // toc
  i = bios_gdGdcReqCmd(CMD_GETTOC2, param);
  for(;;) {
	bios_gdGdcExecServer();
	param[0] = 0; param[1] = 0; param[2] = 0; param[3] = 0;
	j = bios_gdGdcGetCmdStat(i, param);
	if(j != 1) break;
  }
  if(j != 2) goto tryinit;

  first = GDROM_TOC_TRACK(gdrom_toc.first);
  last  = GDROM_TOC_TRACK(gdrom_toc.last);
  if(first < 1 || last > 99 || first > last) {
	// corrupt toc data, maybe
	goto tryinit;
  }
  for(track = last; track >= first; track--) {
	if(GDROM_TOC_CTRL(gdrom_toc.entry[track-1]) & 4) {
	  int lba = GDROM_TOC_LBA(gdrom_toc.entry[track-1]);
	  if(probable_lba < 0) probable_lba = lba;
//      // try to read the boot sector
//      if(quickblockread(lba, 1) != 2) continue;
//      // look for "SEGA" in the boot sector
//      if(memcmp(GDROM_READBUFFER_P2, "SEGA", 4)) continue;
//      if(probable2_lba < 0) probable2_lba = lba;
	  // try to read the first sector after the boot sector
	  if(quickblockread(lba+16, 1) != 2) continue;
	  // look for "CD001"
	  if(memcmp(GDROM_READBUFFER_P2+1, "CD001", 5)) continue;
	  // success!
	  definite_lba = lba;
	  break;
	}
  }

  // now return either a definite or a probable lba
  if(definite_lba >= 0) return definite_lba;
  if(probable2_lba >= 0) return probable2_lba;
  if(probable_lba >= 0) return probable_lba;

  // and if we don't have any of those, we're really screwed, oh well
  // we could try running through tryinit again, but that would probably be
  // fruitless
  return -1;
}

/////////////////////////////////////////////////////////////////////////////
