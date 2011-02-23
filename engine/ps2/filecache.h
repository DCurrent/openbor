/////////////////////////////////////////////////////////////////////////////
//
// filecache - code for background file reading/caching
//
/////////////////////////////////////////////////////////////////////////////

#ifndef FILECACHE_H
#define FILECACHE_H

/////////////////////////////////////////////////////////////////////////////
//
// BLOCKS MUST BE 255 OR LESS
//
void filecache_init(
  int realfd,
  int pakblocks,
  int blocksize,
  unsigned char blocks,
  int vfds
);

//
// attempt to read a block
// returns the number of bytes read or 0 on error
//
int filecache_readpakblock(
  unsigned char *dest,
  int pakblock,
  int startofs,
  int bytes,
  int blocking
);

//
// set up where the vfd pointers are
//
void filecache_setvfd(int vfd, int block, int readahead);

//
// call this every now and then
//
void filecache_process(void);

void filecache_wait_for_prebuffer(int vfd, int nblocks);

/////////////////////////////////////////////////////////////////////////////

#endif

