/////////////////////////////////////////////////////////////////////////////
//
// filecache - code for background file reading/caching
//
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "packfile.h"
#include "filecache.h"

#ifdef PSP
#include <pspsuspend.h>
#endif

#ifndef XBOX
#include <unistd.h>
#endif

#ifdef PS2
#include <eekernel.h>
#include <sifdev.h>
#endif

/////////////////////////////////////////////////////////////////////////////

static int filecache_blocksize;
static int filecache_blocks;
static int max_vfds;

//static int default_minimum_run_bytes = 98304;
static int default_minimum_run_bytes = 131072;
//static int default_minimum_run_bytes = 262144;

/////////////////////////////////////////////////////////////////////////////

// fd for pak file
// lba for pak file (if negative)
static int real_pakfd;

// total number of blocks in the pak
static int total_pakblocks;

// BLOCKSIZE * BLOCKS; be sure to 64-byte-align
static unsigned char *filecache;
static unsigned char *filecache_head = NULL;

// which pakblock is cached in each cacheblock?
// -1 means invalid
static int *filecache_pakmap;

// where is this pakblock cached?
// a value of FILECACHE_BLOCKS (255) means not cached.
// one byte for every block in the entire pak.
static unsigned char *where_is_this_pakblock_cached;

/////////////////////////////////////////////////////////////////////////////

// one per cacheblock
static unsigned *cacheblock_mru;

static unsigned cacheblock_lastused = 0;
static unsigned cacheblock_mru_counter = 0;

static void cacheblock_mark_used(unsigned n)
{
    if(n >= filecache_blocks)
    {
        return;
    }
    if(n == cacheblock_lastused)
    {
        return;
    }
    if(cacheblock_mru_counter == 0xFFFFFFFF)
    {
        unsigned i;
        for(i = 0; i < filecache_blocks; i++)
        {
            cacheblock_mru[i] >>= 1;
        }
        cacheblock_mru_counter >>= 1;
    }
    cacheblock_mru[n] = ++cacheblock_mru_counter;
    cacheblock_lastused = n;
}

/////////////////////////////////////////////////////////////////////////////

// current read pointer (round down)
// -1 if not open
static int *vfd_readptr_pakblock;
// desired readahead in blocks
static int *vfd_desired_readahead_blocks;
// starting block of each open vfd
// these blocks are immune to being replaced in the cache
static int *vfd_startptr_pakblock;

// THIS IS TEMPORARY; DO NOT RELY ON THIS VALUE
static int *vfd_blocks_available;

// requested read block
static int request_read_pakblock = -1;

// avoid going off the end of the track for gdroms
static int filecache_maxcdsectors;

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
// find which cacheblock is the least useful
// this means: least recently used, and is not immune
//
int find_least_useful_cacheblock(void)
{
    int i, vfd;
    int leastcacheblock = -1;
    for(i = 0; i < filecache_blocks; i++)
    {
        int pakblock = filecache_pakmap[i];
        if(pakblock < 0)
        {
            return i;
        }
        // start and read pointers of any open files are both immune
        for(vfd = 0; vfd < max_vfds; vfd++)
        {
            if(vfd_readptr_pakblock[vfd] < 0)
            {
                continue;
            }
            if(vfd_startptr_pakblock[vfd] == pakblock)
            {
                break;
            }
            if(vfd_readptr_pakblock[vfd] == pakblock)
            {
                break;
            }
            if(vfd_desired_readahead_blocks[vfd] > 0)
            {
                if((pakblock >= vfd_readptr_pakblock[vfd]) &&
                        (pakblock < (vfd_readptr_pakblock[vfd] + vfd_desired_readahead_blocks[vfd])))
                {
                    break;
                }
            }
        }
        if(vfd < max_vfds)
        {
            continue;
        }
        if((leastcacheblock < 0) ||
                (cacheblock_mru[i] < cacheblock_mru[leastcacheblock]))
        {
            leastcacheblock = i;
        }
    }
    if(leastcacheblock < 0)
    {
        leastcacheblock = 0;
    }
    return leastcacheblock;
}

/////////////////////////////////////////////////////////////////////////////
//
// get the number of blocks available for this vfd
// (short loop)
//
static int get_vfd_blocks_available(int vfd)
{
    int i;
    // can't have more blocks than what exists in the cache
    int max = filecache_blocks;
    int ptr = vfd_readptr_pakblock[vfd];
    if(ptr < 0)
    {
        return 0;    // no blocks available for a vfd that doesn't exist
    }
    max += ptr;
    if(max > total_pakblocks)
    {
        max = total_pakblocks;
    }
    for(i = ptr; i < max; i++) if(where_is_this_pakblock_cached[i] >= filecache_blocks)
        {
            break;
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
//
int which_pakblock_to_read(int *suggested_min_run)
{
    int vfd;
    int least_avail;
    int percent_available;
    int least_percent_available;
    int pakblock;

    if(suggested_min_run)
    {
        *suggested_min_run = default_minimum_run_bytes / filecache_blocksize;
    }

    // top priority: emergency stream reads
    //   any vfds with desired readahead > 0, and less than 1/4 blocks available
    //   the vfd with the least available blocks is serviced first
    least_avail = -1;
    least_percent_available = 100;
    for(vfd = 0; vfd < max_vfds; vfd++)
    {
        if(vfd_readptr_pakblock[vfd] >= 0 && vfd_desired_readahead_blocks[vfd] > 0)
        {
            percent_available = (100 * vfd_blocks_available[vfd]) / vfd_desired_readahead_blocks[vfd];
            if(percent_available < 25 && percent_available < least_percent_available)
            {
                pakblock = vfd_readptr_pakblock[vfd] + vfd_blocks_available[vfd];
                if(pakblock >= 0 && pakblock < total_pakblocks)
                {
                    least_percent_available = percent_available;
                    least_avail = vfd;
                }
            }
        }
    }
    if(least_avail >= 0)
    {
        if(suggested_min_run)
        {
            *suggested_min_run = vfd_desired_readahead_blocks[vfd] / 4;
        }
        return vfd_readptr_pakblock[least_avail] + vfd_blocks_available[least_avail];
    }

    // normal priority: blocks needed for read calls
    //   any vfd blocked on a read call (must have 0 blocks available) is serviced
    //   ideally this is a first-come first-serve queue, but they can probably just
    //   be serviced in any order
    if(request_read_pakblock >= 0)
    {
        return request_read_pakblock;
    }

    // low priority: stream reads
    //   any vfds with desired readahead > 0, and less than that many bytes available
    //   the vfd with the least available blocks is serviced first
    least_avail = -1;
    least_percent_available = 100;
    for(vfd = 0; vfd < max_vfds; vfd++)
    {
        if(vfd_readptr_pakblock[vfd] >= 0 && vfd_desired_readahead_blocks[vfd] > 0)
        {
            percent_available = (100 * vfd_blocks_available[vfd]) / vfd_desired_readahead_blocks[vfd];
            if(percent_available < least_percent_available)
            {
                pakblock = vfd_readptr_pakblock[vfd] + vfd_blocks_available[vfd];
                if(pakblock >= 0 && pakblock < total_pakblocks)
                {
                    least_percent_available = percent_available;
                    least_avail = vfd;
                }
            }
        }
    }
    if(least_avail >= 0)
    {
        return vfd_readptr_pakblock[least_avail] + vfd_blocks_available[least_avail];
    }

    // nothing needed to read
    return -1;
}

/////////////////////////////////////////////////////////////////////////////

static int last_cacheblock_read = -1;
static int last_pakblock_read = -1;

static int filecache_ready = 0;

static int pakblock_run_ptr = 0;
static int pakblock_run_len = 0;
static int pakblock_run_min = 1;

void filecache_process(void)
{
    int vfd;
    int least_useful_cacheblock;
    int cacheblock_read;
    int pakblock_read;

#ifdef PS2
    int busy;
#endif

    if(!filecache_ready)
    {
        return;
    }

#ifdef PS2
    busy = 1;
    sceIoctl(real_pakfd, SCE_FS_EXECUTING, &busy);
    if(busy)
    {
        return;
    }
#elif DC
    // busy?
    if(real_pakfd < 0)
    {
        if(gdrom_poll())
        {
            return;
        }
    }
#endif

    cacheblock_read = last_cacheblock_read;
    pakblock_read = last_pakblock_read;

    // if we just updated the cache, reflect the new changes
    if(cacheblock_read >= 0 && pakblock_read >= 0)
    {
        filecache_pakmap[cacheblock_read] = pakblock_read;
        where_is_this_pakblock_cached[pakblock_read] = cacheblock_read;
        cacheblock_mark_used(cacheblock_read);
        cacheblock_read = -1;
        pakblock_read = -1;
    }

    // make sure request_read_pakblock isn't out of range
    if(request_read_pakblock >= total_pakblocks)
    {
        request_read_pakblock = 0;
    }

    // if the requested read block is available, signal so
    if(request_read_pakblock >= 0 && where_is_this_pakblock_cached[request_read_pakblock] < filecache_blocks)
    {
        request_read_pakblock = -1;
    }

    // get the least useful cacheblock
    least_useful_cacheblock = find_least_useful_cacheblock();

    // get how many blocks are available to each vfd
    for(vfd = 0; vfd < max_vfds; vfd++)
    {
        vfd_blocks_available[vfd] = get_vfd_blocks_available(vfd);
    }

    //
    // now decide what pakblock to read next
    //
    if(pakblock_run_len >= pakblock_run_min)
    {
        pakblock_run_len = 0;
    }
    if((pakblock_run_len > 0) && ((pakblock_run_ptr + 1) < total_pakblocks))
    {
        pakblock_run_len++;
        pakblock_read = ++pakblock_run_ptr;
    }
    else
    {
        int mymin = default_minimum_run_bytes / filecache_blocksize;
        pakblock_run_min = mymin;
        pakblock_run_len = 1;
        pakblock_read = which_pakblock_to_read(&pakblock_run_min);
        pakblock_run_ptr = pakblock_read;
        if(pakblock_run_min < mymin)
        {
            pakblock_run_min = mymin;
        }
        if(pakblock_read < 0)
        {
            pakblock_run_len = 0;
        }
    }

    //
    // nullify pakblock_read if it's out of bounds or already cached
    //
    // if pakblock_read is out of range, nullify it
    if(pakblock_read >= 0)
    {
        if(pakblock_read >= total_pakblocks)
        {
            pakblock_read = -1;
        }
    }
    // if the pakblock is already cached, don't read it!
    if(pakblock_read >= 0 && where_is_this_pakblock_cached[pakblock_read] < filecache_blocks)
    {
        pakblock_read = -1;
    }

    // if we're reading a pakblock, read it into the least useful cacheblock
    // and invalidate that part of the cache
    if(pakblock_read >= 0)
    {
        int oldpak;
        cacheblock_read = least_useful_cacheblock;
        oldpak = filecache_pakmap[cacheblock_read];
        if(oldpak >= 0 && oldpak < total_pakblocks)
        {
            where_is_this_pakblock_cached[oldpak] = filecache_blocks;
        }
        filecache_pakmap[cacheblock_read] = -1;
    }
    else
    {
        cacheblock_read = -1;
    }

    last_pakblock_read = pakblock_read;
    last_cacheblock_read = cacheblock_read;

    // if we wanted to read something, read it
    if(pakblock_read >= 0 && cacheblock_read >= 0)
    {
#ifdef DC
        if(real_pakfd < 0)
        {
            void *dest = filecache + (cacheblock_read * filecache_blocksize);
            int lba = (pakblock_read * filecache_blocksize) / 2048;
            int n = filecache_blocksize / 2048;
            // definitely do not go out of bounds here
            if((lba + n) > filecache_maxcdsectors)
            {
                n = filecache_maxcdsectors - lba;
            }
            // gdrom reads are non-blocking
            gdrom_readsectors(dest, (-real_pakfd) + lba, n);
        }
        else
#endif
        {
#ifdef PS2
            sceLseek(real_pakfd, pakblock_read * filecache_blocksize, SCE_SEEK_SET);
            busy = 1;
            while(busy)
            {
                sceIoctl(real_pakfd, SCE_FS_EXECUTING, &busy);
            }
            sceRead(real_pakfd, filecache + (cacheblock_read * filecache_blocksize), filecache_blocksize);
#else
            lseek(real_pakfd, pakblock_read * filecache_blocksize, SEEK_SET);
            read(real_pakfd, (char *) filecache + (cacheblock_read * filecache_blocksize), filecache_blocksize);
#endif
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// attempt to read a block
// returns the number of bytes read or 0 on error
//
int filecache_readpakblock(unsigned char *dest, int pakblock, int startofs, int bytes, int blocking)
{
    int cacheblock;
    if(pakblock < 0 || pakblock >= total_pakblocks)
    {
        return 0;
    }
    if(bytes < 0)
    {
        return 0;
    }
    if(startofs < 0)
    {
        return 0;
    }
    if(startofs >= filecache_blocksize)
    {
        return 0;
    }
    if((startofs + bytes) > filecache_blocksize)
    {
        bytes = filecache_blocksize - startofs;
    }

    for(;;)
    {
        // see if we can copy from the cache
        cacheblock = where_is_this_pakblock_cached[pakblock];
        if(cacheblock < filecache_blocks)
        {
            cacheblock_mark_used(cacheblock);
            memcpy(dest, filecache + (cacheblock * filecache_blocksize) + startofs, bytes);
            return bytes;
        }

        // it didn't work
        // if we're nonblocking, return failure
        if(!blocking)
        {
            return 0;
        }

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
void filecache_setvfd(int vfd, int start, int block, int readahead)
{
    if(vfd < 0 || vfd >= max_vfds)
    {
        return;
    }
    vfd_startptr_pakblock[vfd] = start;
    vfd_readptr_pakblock[vfd] = block;
    vfd_desired_readahead_blocks[vfd] = readahead;
}

/////////////////////////////////////////////////////////////////////////////
//
// Release All Allocations
//
void filecache_term()
{
    filecache_blocksize       = 32768;
    filecache_blocks          = 96;
    max_vfds                  = 8;
    default_minimum_run_bytes = 131072;
    real_pakfd                = 0;
    total_pakblocks           = 0;
    cacheblock_lastused       = 0;
    cacheblock_mru_counter    = 0;
    request_read_pakblock     = -1;
    filecache_maxcdsectors    = 0;
    last_cacheblock_read      = -1;
    last_pakblock_read        = -1;
    filecache_ready           = 0;
    pakblock_run_ptr          = 0;
    pakblock_run_len          = 0;
    pakblock_run_min          = 1;
    if(vfd_blocks_available != NULL)
    {
        free(vfd_blocks_available);
        vfd_blocks_available = NULL;
    }
    if(cacheblock_mru != NULL)
    {
        free(cacheblock_mru);
        cacheblock_mru = NULL;
    }
    if(vfd_desired_readahead_blocks != NULL)
    {
        free(vfd_desired_readahead_blocks);
        vfd_desired_readahead_blocks = NULL;
    }
    if(vfd_startptr_pakblock != NULL)
    {
        free(vfd_startptr_pakblock);
        vfd_startptr_pakblock = NULL;
    }
    if(vfd_readptr_pakblock != NULL)
    {
        free(vfd_readptr_pakblock);
        vfd_readptr_pakblock = NULL;
    }
    if(where_is_this_pakblock_cached != NULL)
    {
        free(where_is_this_pakblock_cached);
        where_is_this_pakblock_cached = NULL;
    }
    if(filecache_pakmap != NULL)
    {
        free(filecache_pakmap);
        filecache_pakmap = NULL;
    }
#ifdef PSP
    // Release 4 MBytes of reserved space by PSP.
    if(sceKernelVolatileMemUnlock(0))
    {
        printf("Error allocation filecache!\n");
        exit(0);
    }
#elif DC
    filecache_head = NULL;
#else
    if(filecache_head)
    {
        free(filecache_head);
        filecache_head = NULL;
    }
#endif
}

/////////////////////////////////////////////////////////////////////////////
//
// BLOCKS MUST BE 255 OR LESS
//
void filecache_init(int realfd, int pakcdsectors, int blocksize, unsigned char blocks, int vfds)
{
    int i;

#ifdef PSP
    int size;
#endif

    real_pakfd = realfd;
    total_pakblocks = ((pakcdsectors * 2048) + (blocksize - 1)) / blocksize;
    filecache_blocksize = blocksize;
    filecache_blocks = blocks;
    max_vfds = vfds;
    filecache_maxcdsectors = pakcdsectors;

    // allocate everything
#ifdef PSP
    // This will give us the extra 4 MBytes of reserved space by PSP.
    if(sceKernelVolatileMemLock(0, (void *)&filecache_head, &size))
    {
        printf("Error allocation filecache!\n");
        exit(0);
    }
#elif DC
    filecache_head = (void *)(0xA5500000 - 64);
#else
    filecache_head = malloc(filecache_blocksize * filecache_blocks + 64);
#endif

    filecache = filecache_head;

    // align the filecache
    // we can lose this pointer since it'll never be freed anyway and can be reused while running bor
    // When we exit with sceKernalExitGame All resources are Freed up prior to returning to PSP OS.
    filecache += 0x40 - (((size_t)filecache) & 0x3F);

    // pakmap: all values should be -1
    filecache_pakmap = malloc(sizeof(int) * filecache_blocks);
    for(i = 0; i < filecache_blocks; i++)
    {
        filecache_pakmap[i] = -1;
    }

    // where_is_this_pakblock_cached: all values should be filecache_blocks
    where_is_this_pakblock_cached = malloc(total_pakblocks);
    for(i = 0; i < total_pakblocks; i++)
    {
        where_is_this_pakblock_cached[i] = filecache_blocks;
    }

    // vfd read pointers: init to -1
    vfd_readptr_pakblock = malloc(sizeof(int) * max_vfds);
    for(i = 0; i < max_vfds; i++)
    {
        vfd_readptr_pakblock[i] = -1;
    }

    // vfd starting pointers: init to -1
    vfd_startptr_pakblock = malloc(sizeof(int) * max_vfds);
    for(i = 0; i < max_vfds; i++)
    {
        vfd_startptr_pakblock[i] = -1;
    }

    // desired readahead: init to 0
    vfd_desired_readahead_blocks = malloc(sizeof(int) * max_vfds);
    for(i = 0; i < max_vfds; i++)
    {
        vfd_desired_readahead_blocks[i] = -1;
    }

    // cache mru: init to 0
    cacheblock_mru = malloc(sizeof(unsigned) * filecache_blocks);
    for(i = 0; i < filecache_blocks; i++)
    {
        cacheblock_mru[i] = 0;
    }

    // blocks available: needs no init
    vfd_blocks_available = malloc(sizeof(int) * max_vfds);

    filecache_ready = 1;
}

/////////////////////////////////////////////////////////////////////////////
//
// quick and dirty
//
void filecache_wait_for_prebuffer(int vfd, int nblocks)
{
    if(vfd_readptr_pakblock[vfd] < 0)
    {
        return;
    }
    if((vfd_readptr_pakblock[vfd] + nblocks) > total_pakblocks)
    {
        nblocks = total_pakblocks - vfd_readptr_pakblock[vfd];
    }
    while(get_vfd_blocks_available(vfd) < nblocks)
    {
        filecache_process();
    }
}

/////////////////////////////////////////////////////////////////////////////
