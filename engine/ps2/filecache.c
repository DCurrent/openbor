/////////////////////////////////////////////////////////////////////////////
//
// filecache - code for background file reading/caching
//
/////////////////////////////////////////////////////////////////////////////

#include <fileio.h>

#include "ps2port.h"
#include "filecache.h"

/////////////////////////////////////////////////////////////////////////////

static int filecache_blocksize = 16384;
static int filecache_blocks    = 255;
static int max_vfds            = 8;

/////////////////////////////////////////////////////////////////////////////

// fd for pak file
static int real_pakfd;

// total number of blocks in the pak
static int total_pakblocks;

// BLOCKSIZE * BLOCKS; be sure to 64-byte-align
static unsigned char *filecache;

// which pakblock is cached in each cacheblock?
// -1 means invalid
static int *filecache_pakmap;

// where is this pakblock cached?
// a value of FILECACHE_BLOCKS (255) means not cached.
// one byte for every block in the entire pak.
static unsigned char *where_is_this_pakblock_cached;

/////////////////////////////////////////////////////////////////////////////

// current read pointer (round down)
// -1 if not open
static int *vfd_readptr_pakblock;
// desired readahead in blocks
static int *vfd_desired_readahead_blocks;

// THIS IS TEMPORARY; DO NOT RELY ON THIS VALUE
static int *vfd_blocks_available;

// requested read block
static volatile int request_read_pakblock = -1;

/////////////////////////////////////////////////////////////////////////////
//
// make sure that the total of all desired readahead is _less_ than the cache size!
// just to make things work more smoothly
//
// when finding a freeable cacheblock, select the one with the greatest extraneity
// defined as the greatest distance past the desired readahead for any vfd
//

/////////////////////////////////////////////////////////////////////////////
//
// find which cacheblock is the least useful (has the highest extraneity)
// also returns its extraneity, for when that matters
//

static int least_ucb_start = 0;

int find_least_useful_cacheblock(int *out_biggest_extraneity) {
  int i, vfd;
  int bestblock = 0;
  int bestblock_extraneity = 0;
  for(i = 0; i < filecache_blocks; i++) {
    int cacheblock = (i + least_ucb_start) % filecache_blocks;
    int least_extraneity = 0x7FFFFFFF; // extraneity value of "before all vfds"
    int extraneity;
    int pakblock;
    pakblock = filecache_pakmap[cacheblock];
    dbg_printf("trying cacheblock %d (pak=%d)\n",cacheblock, pakblock);
    // if this cacheblock is invalid, it's not useful at all, so return it immediately
    if(pakblock < 0) {
      bestblock_extraneity = 0x7FFFFFFF;
      bestblock = cacheblock;
      break;
    }
    // compute least extraneity among all used vfds
    for(vfd = 0; vfd < max_vfds; vfd++) {
      int readptr = vfd_readptr_pakblock[vfd];
      dbg_printf("vfd=%d readptr=%d\n",vfd,readptr);
      if(readptr < 0) continue;
      // if this pakblock is before the vfd start, we don't care
      if(pakblock < readptr) continue;
      // otherwise compute extraneity
      extraneity = pakblock - (readptr + vfd_desired_readahead_blocks[vfd]);
      dbg_printf("extraneity=%d\n",extraneity);
      // is it the least?
      if(extraneity < least_extraneity) least_extraneity = extraneity;
    }
    // is this the best block?
    if(least_extraneity > bestblock_extraneity) {
      bestblock_extraneity = least_extraneity;
      bestblock = cacheblock;
    }
    // is least_extraneity so big that we're like, done?
    if(least_extraneity == 0x7FFFFFFF) break;
  }
  dbg_printf("least useful = %d extra = %d\n",bestblock, bestblock_extraneity);

  if(out_biggest_extraneity) { *out_biggest_extraneity = bestblock_extraneity; }
  return bestblock;
}

/////////////////////////////////////////////////////////////////////////////
//
// get the number of blocks available for this vfd
// (short loop)
//
static int get_vfd_blocks_available(int vfd) {
  int i;
  // can't have more blocks than what exists in the cache
  int max = filecache_blocks;
  int ptr = vfd_readptr_pakblock[vfd];
  if(ptr < 0) return 0; // no blocks available for a vfd that doesn't exist
  max += ptr;
  if(max > total_pakblocks) { max = total_pakblocks; }
  for(i = ptr; i < max; i++) {
    if(where_is_this_pakblock_cached[i] >= filecache_blocks) break;
  }
  return i - ptr;
}

/////////////////////////////////////////////////////////////////////////////
//
// top priority: emergency stream reads
//   any vfds with desired readahead > 0, and less than (some number) blocks available
//   the vfd with the least available blocks is serviced first
// normal priority: blocks needed for read calls
//   any vfd blocked on a read call (must have 0 blocks available) is serviced
//   ideally this is a first-come first-serve queue, but they can probably just
//   be serviced in any order
// low priority: stream reads
//   any vfds with desired readahead > 0, and less than that many bytes available
//   the vfd with the least available blocks is serviced first
// lowest priority: extraneity replacement
//   if there are any vfds for which the extraneity may be improved, improve them
//   the vfd with the least available blocks is serviced first
//
int which_pakblock_to_read(int cache_extraneity, int *important) {
  int vfd;
  int least_avail;
  int pakblock;
  int emergency_some = 2;

//return request_read_pakblock;

  if(emergency_some < 2) emergency_some = 2;

  // assume unimportant at first
  *important = 0;

  // top priority: emergency stream reads
  //   any vfds with desired readahead > 0, and less than (some number) blocks available
  //   the vfd with the least available blocks is serviced first
  least_avail = -1;
  for(vfd = 0; vfd < max_vfds; vfd++) {
    if(vfd_readptr_pakblock[vfd] >= 0 && vfd_desired_readahead_blocks[vfd] > 0) {
      int emergency = vfd_desired_readahead_blocks[vfd];
      if(emergency > emergency_some) emergency = emergency_some;
      if(vfd_blocks_available[vfd] < emergency) {
        if(least_avail < 0 || vfd_blocks_available[vfd] < vfd_blocks_available[least_avail]) {
          pakblock = vfd_readptr_pakblock[vfd] + vfd_blocks_available[vfd];
          if(pakblock >= 0 && pakblock < total_pakblocks) {
            least_avail = vfd;
          }
        }
      }
    }
  }
  if(least_avail >= 0) {
  dbg_printf("emergency on fd=%d\n",vfd);
    *important = 1;
    return vfd_readptr_pakblock[least_avail] + vfd_blocks_available[least_avail];
  }

  // normal priority: blocks needed for read calls
  //   any vfd blocked on a read call (must have 0 blocks available) is serviced
  //   ideally this is a first-come first-serve queue, but they can probably just
  //   be serviced in any order
  if(request_read_pakblock >= 0) {
    *important = 1;
    return request_read_pakblock;
  }
//return 0;

  // low priority: stream reads
  //   any vfds with desired readahead > 0, and less than that many bytes available
  //   the vfd with the least available blocks is serviced first
  least_avail = -1;
  for(vfd = 0; vfd < max_vfds; vfd++) {
    if(vfd_readptr_pakblock[vfd] >= 0 && vfd_desired_readahead_blocks[vfd] > 0) {
      if(vfd_blocks_available[vfd] < vfd_desired_readahead_blocks[vfd]) {
        if(least_avail < 0 || vfd_blocks_available[vfd] < vfd_blocks_available[least_avail]) {
          pakblock = vfd_readptr_pakblock[vfd] + vfd_blocks_available[vfd];
          if(pakblock >= 0 && pakblock < total_pakblocks) {
            least_avail = vfd;
          }
        }
      }
    }
  }
  if(least_avail >= 0) return vfd_readptr_pakblock[least_avail] + vfd_blocks_available[least_avail];

  // lowest priority: extraneity replacement
  //   if there are any vfds for which the extraneity may be improved, improve them
  //   the vfd with the least available blocks is serviced first
  // (TODO)
  // actually the read_run will probably work fine for this

  return -1;
}

/////////////////////////////////////////////////////////////////////////////

static volatile int last_cacheblock_read = -1;
static volatile int last_pakblock_read = -1;

static volatile int filecache_ready = 0;

void filecache_process(void) {
  int vfd;
  int least_useful_cacheblock;
  int least_useful_cacheblock_extraneity;
  int important;
  int cacheblock_read;
  int pakblock_read;
  int busy;

  if(!filecache_ready) return;

  busy = fioSync(FIO_NOWAIT, NULL);
  if(busy == FIO_INCOMPLETE) return;

  cacheblock_read = last_cacheblock_read;
  pakblock_read = last_pakblock_read;

  // if we just updated the cache, reflect the new changes
  if(cacheblock_read >= 0 && pakblock_read >= 0) {
    filecache_pakmap[cacheblock_read] = pakblock_read;
    where_is_this_pakblock_cached[pakblock_read] = cacheblock_read;
    cacheblock_read = -1;
    pakblock_read = -1;
  }

  // make sure request_read_pakblock isn't out of range
  if(request_read_pakblock >= total_pakblocks) request_read_pakblock = 0;

  // if the requested read block is available, signal so
  if(request_read_pakblock >= 0 && where_is_this_pakblock_cached[request_read_pakblock] < filecache_blocks) {
    request_read_pakblock = -1;
  }

  // get the least useful cacheblock
  least_useful_cacheblock = find_least_useful_cacheblock(&least_useful_cacheblock_extraneity);

  // get how many blocks are available to each vfd
  for(vfd = 0; vfd < max_vfds; vfd++) {
    vfd_blocks_available[vfd] = get_vfd_blocks_available(vfd);
  }

  //
  // now decide what pakblock to read next
  //
  important = 0;
  pakblock_read = which_pakblock_to_read(least_useful_cacheblock_extraneity, &important);

  dbg_printf("which_pakblock=%d request=%d leastucb=%d\n",pakblock_read, request_read_pakblock, least_useful_cacheblock);

  //
  // nullify pakblock_read if it's out of bounds or already cached
  //
  // if pakblock_read is out of range, nullify it
  if(pakblock_read >= 0) { if(pakblock_read >= total_pakblocks) { pakblock_read = -1; } }
  // if the pakblock is already cached, don't read it!
  if(pakblock_read >= 0 && where_is_this_pakblock_cached[pakblock_read] < filecache_blocks) { pakblock_read = -1; }

  // if we're reading a pakblock, read it into the least useful cacheblock
  // and invalidate that part of the cache
  if(pakblock_read >= 0) {
    int oldpak;
    cacheblock_read = least_useful_cacheblock;
    oldpak = filecache_pakmap[cacheblock_read];
    if(oldpak >= 0 && oldpak < total_pakblocks) {
      where_is_this_pakblock_cached[oldpak] = filecache_blocks;
    }
    filecache_pakmap[cacheblock_read] = -1;
    least_ucb_start = (cacheblock_read + 1) % filecache_blocks;
  } else {
    cacheblock_read = -1;
  }

  last_pakblock_read = pakblock_read;
  last_cacheblock_read = cacheblock_read;

  // if we wanted to read something, read it
  if(pakblock_read >= 0 && cacheblock_read >= 0) {
  dbg_printf("reading pakblock %d cacheblock %d\n", pakblock_read, cacheblock_read);

    fioLseek(real_pakfd, pakblock_read * filecache_blocksize, SEEK_SET);
    fioSync(FIO_WAIT, NULL);
    fioRead(real_pakfd, filecache + (cacheblock_read * filecache_blocksize), filecache_blocksize);
  }

}

/////////////////////////////////////////////////////////////////////////////
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
) {
  int cacheblock;

  if(pakblock < 0 || pakblock >= total_pakblocks) return 0;
  if(bytes < 0) return 0;
  if(startofs < 0) return 0;
  if(startofs >= filecache_blocksize) return 0;
  if((startofs+bytes) > filecache_blocksize) bytes = filecache_blocksize - startofs;

  for(;;) {

    // see if we can copy from the cache
    cacheblock = where_is_this_pakblock_cached[pakblock];
    if(cacheblock < filecache_blocks) {
      memcpy(dest, filecache + (cacheblock * filecache_blocksize) + startofs, bytes);
      return bytes;
    }

    // it didn't work
    // if we're nonblocking, return failure
    if(!blocking) return 0;

    // otherwise, demand a block
    request_read_pakblock = pakblock;

    filecache_process();
  }

  return bytes;
}

/////////////////////////////////////////////////////////////////////////////
//
// set up where the vfd pointers are
//
void filecache_setvfd(int vfd, int block, int readahead) {

  if(vfd < 0 || vfd >= max_vfds) return;

  vfd_readptr_pakblock[vfd] = block;
  vfd_desired_readahead_blocks[vfd] = readahead;

}

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
) {
  int i;

  real_pakfd = realfd;
  total_pakblocks = pakblocks;
  filecache_blocksize = blocksize;
  filecache_blocks = blocks;
  max_vfds = vfds;

  // allocate everything

  // filecache: needs no initialization
  filecache = tracemalloc("filecache_init", filecache_blocksize * filecache_blocks + 64);
  // align the filecache
  // we can lose this pointer since it'll never be freed anyway
  filecache += 0x40 - (((int)filecache) & 0x3F);

  // pakmap: all values should be -1
  filecache_pakmap = tracemalloc("filecache_init", sizeof(int) * filecache_blocks);
  for(i = 0; i < filecache_blocks; i++) filecache_pakmap[i] = -1;

  // where_is_this_pakblock_cached: all values should be filecache_blocks
  where_is_this_pakblock_cached = tracemalloc("filecache_init", total_pakblocks);
  for(i = 0; i < total_pakblocks; i++) where_is_this_pakblock_cached[i] = filecache_blocks;

  // vfd read pointers: init to -1
  vfd_readptr_pakblock = tracemalloc("filecache_init", sizeof(int) * max_vfds);
  for(i = 0; i < max_vfds; i++) vfd_readptr_pakblock[i] = -1;

  // desired readahead: init to 0
  vfd_desired_readahead_blocks = tracemalloc("filecache_init", sizeof(int) * max_vfds);
  for(i = 0; i < max_vfds; i++) vfd_desired_readahead_blocks[i] = -1;

  // blocks available: needs no init
  vfd_blocks_available = tracemalloc("filecache_init", sizeof(int) * max_vfds);

  filecache_ready = 1;

  // done
}

/////////////////////////////////////////////////////////////////////////////
//
// quick and dirty
//
void filecache_wait_for_prebuffer(int vfd, int nblocks) {

  while(get_vfd_blocks_available(vfd) < nblocks) filecache_process();

}

/////////////////////////////////////////////////////////////////////////////

