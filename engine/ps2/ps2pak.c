/////////////////////////////////////////////////////////////////////////////

#include <fileio.h>

#include "ps2port.h"
#include "ps2pak.h"
#include "packfile.h"
#include "filecache.h"

/////////////////////////////////////////////////////////////////////////////
//
// CACHEBLOCKSIZE*CACHEBLOCKS is the size of the ever-present file cache
// cacheblocks must be 255 or less!
//
#define CACHEBLOCKSIZE (16384)
#define CACHEBLOCKS    (255)

/////////////////////////////////////////////////////////////////////////////

const char ps2pakfilename[] = "bor.pak";

#define PS2PAK_VFDS 8

int blocking = 1;

static int ps2pakfd;
static int ps2paksize;

static int ps2pak_vfdexists[PS2PAK_VFDS];
static int ps2pak_vfdstart[PS2PAK_VFDS];
static int ps2pak_vfdsize[PS2PAK_VFDS];
static int ps2pak_vfdpos[PS2PAK_VFDS];
static int ps2pak_vfdreadahead[PS2PAK_VFDS];

/////////////////////////////////////////////////////////////////////////////

char myfilenametolower(char c) {
  if(c >= 'A' && c <= 'Z') c += 'a' - 'A';
  if(c == '\\') c = '/';
  return c;
}

int myfilenamecmp(const char *a, const char *b) {
  for(;;) {
    char ca = *a++;
    char cb = *b++;
    if((!ca) && (!cb)) break;
    ca = myfilenametolower(ca);
    cb = myfilenametolower(cb);
    if(ca < cb) return -1;
    if(ca > cb) return 1;
  }
  return 0;
}

/////////////////////////////////////////////////////////////////////////////

static int ps2pak_headerstart;
static int ps2pak_headersize;

static unsigned char *ps2pak_cdheader;
static unsigned char *ps2pak_header;

static void flushfd(int fd) {
  fioSync(FIO_WAIT, NULL);
}

/////////////////////////////////////////////////////////////////////////////
//
// returns number of sectors read successfully
//
static int ps2pak_getsectors(void *dest, int lba, int n) {
  fioLseek(ps2pakfd, lba << 11, SEEK_SET);
  fioSync(FIO_WAIT, 0);
  fioRead(ps2pakfd, dest, n << 11);
  fioSync(FIO_WAIT, 0);
  return n;
}

/////////////////////////////////////////////////////////////////////////////

void ps2pak_init(void) {
  int i;
  unsigned char *sectors;
  
  ps2pakfd = fioOpen(ps2gethostfilename(ps2pakfilename), O_RDONLY);
  if(ps2pakfd < 0) {
    debug_printf("error opening %s (%d); halting\n", ps2pakfilename, ps2pakfd);
    SleepThread();
  }
  ps2paksize = fioLseek(ps2pakfd, 0, SEEK_END);

  // reopen as nowait
//  fioClose(ps2pakfd);
//  fioSetBlockMode(FIO_NOWAIT);
//  blocking = 0;
//  if (fioOpen(ps2gethostfilename(ps2pakfilename), O_RDONLY) != 0) {
//    debug_printf("error opening %s; halting\n", ps2pakfilename);
//    SleepThread();
//  }
//  fioSync(FIO_WAIT, &ps2pakfd);
  if(ps2pakfd < 0) {
    debug_printf("error opening %s (%d); halting\n", ps2pakfilename, ps2pakfd);
    SleepThread();
  }
  flushfd(ps2pakfd);

  //debug_printf("(ps2paksize = %d)\n", ps2paksize);

  sectors = tracemalloc("ps2pak_init", 4096);
  if(!sectors) { debug_printf("alloc failed\n"); SleepThread(); }

  { int getptrfrom = ps2paksize - 4;
    if(ps2pak_getsectors(sectors, getptrfrom >> 11, 2) < 1) {
      debug_printf("unable to read pak header pointer; halting\n");
      SleepThread();
    }
    ps2pak_headerstart = readlsb32(sectors + (getptrfrom & 0x7FF));
  }
  
  tracefree(sectors);

  if(ps2pak_headerstart >= ps2paksize) {
    debug_printf("invalid pak header pointer; halting\n");
    SleepThread();
  }

  ps2pak_headersize = ps2paksize - ps2pak_headerstart;

  {
    // let's cache it on CD sector boundaries
    int ps2pak_cdheaderstart = ps2pak_headerstart & (~0x7FF);
    int ps2pak_cdheadersize = ((ps2paksize - ps2pak_cdheaderstart) + 0x7FF) & (~0x7FF);

    if(ps2pak_cdheadersize > 262144) {
      debug_printf("pak header is too large; halting\n");
      SleepThread();
    }

    ps2pak_cdheader = tracemalloc("ps2pak_init", ps2pak_cdheadersize);
    if(!ps2pak_cdheader) { debug_printf("alloc failed\n"); SleepThread(); }
    if(ps2pak_getsectors(ps2pak_cdheader, ps2pak_cdheaderstart >> 11, ps2pak_cdheadersize >> 11) != (ps2pak_cdheadersize >> 11)) {
      debug_printf("unable to read pak header; halting\n");
      SleepThread();
    }
    // ok, header is now cached

    ps2pak_header = ps2pak_cdheader + (ps2pak_headerstart & 0x7FF);
  }

  // header does not include the last 4-byte stuff
  if(ps2pak_headersize >= 4) {
    ps2pak_headersize -= 4;
    // add a trailing null o/~
    ps2pak_header[ps2pak_headersize] = 0;
  }

//debug_printf("ps2pak cached header (%d bytes)\n", ps2pak_headersize);

  // initialize vfd table
  for(i = 0; i < PS2PAK_VFDS; i++) {
    ps2pak_vfdexists[i] = 0;
  }

  // finally, initialize the filecache

  filecache_init(
    ps2pakfd,
    (ps2paksize + (CACHEBLOCKSIZE-1)) / CACHEBLOCKSIZE,
    CACHEBLOCKSIZE,
    CACHEBLOCKS,
    PS2PAK_VFDS
  );

}

/////////////////////////////////////////////////////////////////////////////

void update_filecache_vfd(int vfd) {
  if(ps2pak_vfdexists[vfd]) {
    filecache_setvfd(vfd,
      (ps2pak_vfdstart[vfd] + ps2pak_vfdpos[vfd]) / CACHEBLOCKSIZE,
      (ps2pak_vfdreadahead[vfd] + (CACHEBLOCKSIZE-1)) / CACHEBLOCKSIZE
    );
  } else {
    filecache_setvfd(vfd, -1, 0);
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// ONLY USE THIS AFTER THE INIT IS FINISHED
//
static int ps2pak_rawread(int fd, unsigned char *dest, int len, int blocking) {
  int end;
  int r;
  int total = 0;
  
  int pos = ps2pak_vfdstart[fd] + ps2pak_vfdpos[fd];

  if(pos < 0) return 0;
  if(pos >= ps2paksize) return 0;
  if((pos + len) > ps2paksize) { len = ps2paksize - pos; }
  end = pos + len;

  update_filecache_vfd(fd);

  while(pos < end) {
    int b = pos / CACHEBLOCKSIZE;
    int startthisblock = pos % CACHEBLOCKSIZE;
    int sizethisblock = CACHEBLOCKSIZE - startthisblock;
    if(sizethisblock > (end-pos)) sizethisblock = (end-pos);
//debug_printf("a");
    r = filecache_readpakblock(dest, b, startthisblock, sizethisblock, blocking);
//debug_printf("b(%d)",r);
    if(r >= 0) {
      total += r;
      ps2pak_vfdpos[fd] += r;
      update_filecache_vfd(fd);
    }
    if(r < sizethisblock) break;

    dest += sizethisblock;
    pos += sizethisblock;
  }

  return total;
}

/////////////////////////////////////////////////////////////////////////////

int openreadaheadpackfile(char *filename, char *packfilename, int readaheadsize, int prebuffersize) {
  int hpos;
  int vfd;
  
  if(myfilenamecmp(packfilename, ps2pakfilename)) {
    debug_printf("tried to open from unknown pack file (%s)\n", packfilename);
    SleepThread();
  }

  // find a free vfd
  for(vfd = 0; vfd < PS2PAK_VFDS; vfd++) {
    if(!ps2pak_vfdexists[vfd]) break;
  }
  if(vfd >= PS2PAK_VFDS) return -1;

  // look for filename in the header
  hpos = 0;
  for(;;) {
    if((hpos + 12) >= ps2pak_headersize) return -1;
    if(myfilenamecmp(ps2pak_header + hpos + 12, filename)) {
      hpos += readlsb32(ps2pak_header + hpos);
      continue;
    }
    // found!
    ps2pak_vfdstart[vfd] = readlsb32(ps2pak_header + hpos + 4);
    ps2pak_vfdsize[vfd] = readlsb32(ps2pak_header + hpos + 8);
    break;
  }

  // have a little read-ahead
  if(readaheadsize < 131072) readaheadsize = 131072;

  ps2pak_vfdpos[vfd] = 0;
  ps2pak_vfdexists[vfd] = 1;
  ps2pak_vfdreadahead[vfd] = readaheadsize;

  // notify filecache that we have a new vfd
  update_filecache_vfd(vfd);

  // if we want prebuffering, wait for it
  if(prebuffersize > 0) {
    filecache_wait_for_prebuffer(vfd, (prebuffersize + ((CACHEBLOCKSIZE)-1)) / CACHEBLOCKSIZE);
  }

  return vfd;
}

int openpackfile(char *filename, char *packfilename) {
  return openreadaheadpackfile(filename, packfilename, 0, 0);
}

/////////////////////////////////////////////////////////////////////////////

int ps2pak_isvalidfd(int fd) {
  if(fd < 0 || fd >= PS2PAK_VFDS) return 0;
  if(!ps2pak_vfdexists[fd]) return 0;
  return 1;
}

/////////////////////////////////////////////////////////////////////////////

int readpackfileblocking(int fd, void *buf, int len, int blocking) {
  int n;
  
  if(!ps2pak_isvalidfd(fd)) return -1;

  if(ps2pak_vfdpos[fd] < 0) return 0;
  if(ps2pak_vfdpos[fd] > ps2pak_vfdsize[fd]) ps2pak_vfdpos[fd] = ps2pak_vfdsize[fd];
  if((len + ps2pak_vfdpos[fd]) > ps2pak_vfdsize[fd]) { len = ps2pak_vfdsize[fd] - ps2pak_vfdpos[fd]; }
  if(len < 1) return 0;

  update_filecache_vfd(fd);

  n = ps2pak_rawread(fd, buf, len, blocking);

  if(n < 0) n = 0;

  if(ps2pak_vfdpos[fd] > ps2pak_vfdsize[fd]) ps2pak_vfdpos[fd] = ps2pak_vfdsize[fd];

  update_filecache_vfd(fd);

  return n;
}

int readpackfile(int fd, void *buf, int len) {
  return readpackfileblocking(fd, buf, len, 1);
}

int readpackfile_noblock(int fd, void *buf, int len) {
  return readpackfileblocking(fd, buf, len, 0);
}

int packfileeof(int fd) {
  if(!ps2pak_isvalidfd(fd)) return -1;
  return (ps2pak_vfdpos[fd] >= ps2pak_vfdsize[fd]);
}

/////////////////////////////////////////////////////////////////////////////

int closepackfile(int fd) {
  if(!ps2pak_isvalidfd(fd)) return -1;
  ps2pak_vfdexists[fd] = 0;
  update_filecache_vfd(fd);
  return 0;
}

/////////////////////////////////////////////////////////////////////////////

int seekpackfile(int fd, int n, int whence) {
  if(!ps2pak_isvalidfd(fd)) return -1;
  switch(whence) {
  case 0: ps2pak_vfdpos[fd] = n; break; // set
  case 1: ps2pak_vfdpos[fd] += n; break; // cur
  case 2: ps2pak_vfdpos[fd] = ps2pak_vfdsize[fd] + n; break; // end
  default: return -1;
  }
  // original code had this check too, so do it here
  if(ps2pak_vfdpos[fd] < 0) ps2pak_vfdpos[fd] = 0;
  if(ps2pak_vfdpos[fd] > ps2pak_vfdsize[fd]) ps2pak_vfdpos[fd] = ps2pak_vfdsize[fd];

  update_filecache_vfd(fd);

  return ps2pak_vfdpos[fd];
}

/////////////////////////////////////////////////////////////////////////////

