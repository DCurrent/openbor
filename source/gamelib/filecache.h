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
// hint: if you set realfd to a negative number, that means it'll use the
// gdrom routines to read a pak file starting at that (positive) lba
//
void filecache_init(
    int realfd,
    int pakcdsectors,
    int blocksize,
    unsigned char blocks,
    int vfds
);

//
// Clear All Allocations
//
void filecache_term(void);

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
void filecache_setvfd(int vfd, int start, int block, int readahead);

//
// call this every now and then
//
void filecache_process(void);

void filecache_wait_for_prebuffer(int vfd, int nblocks);

/////////////////////////////////////////////////////////////////////////////

#endif

