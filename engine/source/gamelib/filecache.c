/////////////////////////////////////////////////////////////////////////////
//
// filecache - code for background file reading/caching
//
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include "utils.h"
#include "packfile.h"
#include "filecache.h"

/////////////////////////////////////////////////////////////////////////////

static int filecache_blocksize;
static int filecache_blocks;

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

typedef struct filecache_vfd
{
    int descriptor;
    int read_block;
    int desired_readahead_blocks;
    int start_block;
    int blocks_available;
} filecache_vfd;

/*
 * Only live virtual descriptors participate in cache scans. The index map
 * converts the public integer descriptor to its compact active-list entry.
 * Both allocations grow on demand and impose no fixed descriptor limit.
 */
static filecache_vfd *active_vfd;
static size_t active_vfd_count;
static size_t active_vfd_capacity;
static int *active_vfd_index;
static size_t active_vfd_index_capacity;

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
static int filecache_reserve_vfd_indices(size_t required_capacity)
{
    size_t old_capacity;
    size_t new_capacity;
    size_t index;
    int *new_index;

    if(required_capacity <= active_vfd_index_capacity)
    {
        return 1;
    }
    if(required_capacity > (size_t)INT_MAX)
    {
        return 0;
    }

    old_capacity = active_vfd_index_capacity;
    new_capacity = old_capacity ? old_capacity : 8U;
    while(new_capacity < required_capacity)
    {
        if(new_capacity > (size_t)INT_MAX / 2U)
        {
            new_capacity = (size_t)INT_MAX;
            break;
        }
        new_capacity *= 2U;
    }
    if(new_capacity > SIZE_MAX / sizeof(*new_index))
    {
        return 0;
    }

    new_index = realloc(active_vfd_index, new_capacity * sizeof(*new_index));
    if(!new_index)
    {
        return 0;
    }
    active_vfd_index = new_index;
    active_vfd_index_capacity = new_capacity;
    for(index = old_capacity; index < new_capacity; index++)
    {
        active_vfd_index[index] = -1;
    }
    return 1;
}

static int filecache_reserve_active_vfds(size_t required_capacity)
{
    size_t new_capacity;
    filecache_vfd *new_vfd;

    if(required_capacity <= active_vfd_capacity)
    {
        return 1;
    }
    if(required_capacity > (size_t)INT_MAX)
    {
        return 0;
    }

    new_capacity = active_vfd_capacity ? active_vfd_capacity : 8U;
    while(new_capacity < required_capacity)
    {
        if(new_capacity > (size_t)INT_MAX / 2U)
        {
            new_capacity = (size_t)INT_MAX;
            break;
        }
        new_capacity *= 2U;
    }
    if(new_capacity > SIZE_MAX / sizeof(*new_vfd))
    {
        return 0;
    }

    new_vfd = realloc(active_vfd, new_capacity * sizeof(*new_vfd));
    if(!new_vfd)
    {
        return 0;
    }
    active_vfd = new_vfd;
    active_vfd_capacity = new_capacity;
    return 1;
}

static filecache_vfd *filecache_get_vfd(int descriptor)
{
    int active_index;

    if(descriptor < 0 || (size_t)descriptor >= active_vfd_index_capacity)
    {
        return NULL;
    }
    active_index = active_vfd_index[descriptor];
    if(active_index < 0 || (size_t)active_index >= active_vfd_count)
    {
        return NULL;
    }
    return &active_vfd[active_index];
}

static void filecache_remove_vfd(int descriptor)
{
    int active_index;
    size_t last_index;

    if(descriptor < 0 || (size_t)descriptor >= active_vfd_index_capacity)
    {
        return;
    }
    active_index = active_vfd_index[descriptor];
    if(active_index < 0 || (size_t)active_index >= active_vfd_count)
    {
        return;
    }

    last_index = active_vfd_count - 1U;
    if((size_t)active_index != last_index)
    {
        active_vfd[active_index] = active_vfd[last_index];
        active_vfd_index[active_vfd[active_index].descriptor] = active_index;
    }
    active_vfd_count--;
    active_vfd_index[descriptor] = -1;
}

int find_least_useful_cacheblock(void)
{
    int i;
    size_t active_index;
    int leastcacheblock = -1;
    for(i = 0; i < filecache_blocks; i++)
    {
        int pakblock = filecache_pakmap[i];
        if(pakblock < 0)
        {
            return i;
        }
        // start and read pointers of any open files are both immune
        for(active_index = 0; active_index < active_vfd_count; active_index++)
        {
            filecache_vfd *vfd = &active_vfd[active_index];

            if(vfd->start_block == pakblock)
            {
                break;
            }
            if(vfd->read_block == pakblock)
            {
                break;
            }
            if(vfd->desired_readahead_blocks > 0)
            {
                if((pakblock >= vfd->read_block) &&
                        (pakblock < (vfd->read_block + vfd->desired_readahead_blocks)))
                {
                    break;
                }
            }
        }
        if(active_index < active_vfd_count)
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
static int get_vfd_blocks_available(const filecache_vfd *vfd)
{
    int i;
    // can't have more blocks than what exists in the cache
    int max = filecache_blocks;
    int ptr;

    if(!vfd)
    {
        return 0;
    }
    ptr = vfd->read_block;
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
    size_t active_index;
    int least_active_index;
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
    least_active_index = -1;
    least_percent_available = 100;
    for(active_index = 0; active_index < active_vfd_count; active_index++)
    {
        filecache_vfd *vfd = &active_vfd[active_index];

        if(vfd->read_block >= 0 && vfd->desired_readahead_blocks > 0)
        {
            percent_available = (100 * vfd->blocks_available) / vfd->desired_readahead_blocks;
            if(percent_available < 25 && percent_available < least_percent_available)
            {
                pakblock = vfd->read_block + vfd->blocks_available;
                if(pakblock >= 0 && pakblock < total_pakblocks)
                {
                    least_percent_available = percent_available;
                    least_active_index = (int)active_index;
                }
            }
        }
    }
    if(least_active_index >= 0)
    {
        filecache_vfd *vfd = &active_vfd[least_active_index];

        if(suggested_min_run)
        {
            *suggested_min_run = vfd->desired_readahead_blocks / 4;
        }
        return vfd->read_block + vfd->blocks_available;
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
    least_active_index = -1;
    least_percent_available = 100;
    for(active_index = 0; active_index < active_vfd_count; active_index++)
    {
        filecache_vfd *vfd = &active_vfd[active_index];

        if(vfd->read_block >= 0 && vfd->desired_readahead_blocks > 0)
        {
            percent_available = (100 * vfd->blocks_available) / vfd->desired_readahead_blocks;
            if(percent_available < least_percent_available)
            {
                pakblock = vfd->read_block + vfd->blocks_available;
                if(pakblock >= 0 && pakblock < total_pakblocks)
                {
                    least_percent_available = percent_available;
                    least_active_index = (int)active_index;
                }
            }
        }
    }
    if(least_active_index >= 0)
    {
        filecache_vfd *vfd = &active_vfd[least_active_index];
        return vfd->read_block + vfd->blocks_available;
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
    size_t active_index;
    int least_useful_cacheblock;
    int cacheblock_read;
    int pakblock_read;

    if(!filecache_ready)
    {
        return;
    }

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
    for(active_index = 0; active_index < active_vfd_count; active_index++)
    {
        active_vfd[active_index].blocks_available = get_vfd_blocks_available(&active_vfd[active_index]);
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
        lseek(real_pakfd, pakblock_read * filecache_blocksize, SEEK_SET);
        read(real_pakfd, (char *) filecache + (cacheblock_read * filecache_blocksize), filecache_blocksize);
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
int filecache_setvfd(int vfd, int start, int block, int readahead)
{
    filecache_vfd *record;

    if(vfd < 0)
    {
        return 0;
    }
    if(start < 0 || block < 0)
    {
        filecache_remove_vfd(vfd);
        return 1;
    }

    record = filecache_get_vfd(vfd);
    if(!record)
    {
        if(!filecache_reserve_vfd_indices((size_t)vfd + 1U) ||
           !filecache_reserve_active_vfds(active_vfd_count + 1U))
        {
            return 0;
        }

        record = &active_vfd[active_vfd_count];
        memset(record, 0, sizeof(*record));
        record->descriptor = vfd;
        active_vfd_index[vfd] = (int)active_vfd_count;
        active_vfd_count++;
    }

    record->start_block = start;
    record->read_block = block;
    record->desired_readahead_blocks = readahead;
    record->blocks_available = 0;
    return 1;
}

/////////////////////////////////////////////////////////////////////////////
//
// Release All Allocations
//
void filecache_term()
{
    filecache_blocksize       = 32768;
    filecache_blocks          = 96;
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
    if(active_vfd != NULL)
    {
        free(active_vfd);
        active_vfd = NULL;
    }
    active_vfd_count = 0;
    active_vfd_capacity = 0;
    if(active_vfd_index != NULL)
    {
        free(active_vfd_index);
        active_vfd_index = NULL;
    }
    active_vfd_index_capacity = 0;
    if(cacheblock_mru != NULL)
    {
        free(cacheblock_mru);
        cacheblock_mru = NULL;
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
    if(filecache_head)
    {
        free(filecache_head);
        filecache_head = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// BLOCKS MUST BE 255 OR LESS
//
void filecache_init(int realfd, int pakcdsectors, int blocksize, unsigned char blocks)
{
    int i;
    real_pakfd = realfd;
    total_pakblocks = ((pakcdsectors * 2048) + (blocksize - 1)) / blocksize;
    filecache_blocksize = blocksize;
    filecache_blocks = blocks;
    filecache_maxcdsectors = pakcdsectors;

    // allocate everything
    filecache_head = malloc(filecache_blocksize * filecache_blocks + 64);
    filecache = filecache_head;

    // align the filecache
    // we can lose this pointer since it'll never be freed anyway and can be reused while running bor
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

    // cache mru: init to 0
    cacheblock_mru = malloc(sizeof(unsigned) * filecache_blocks);
    for(i = 0; i < filecache_blocks; i++)
    {
        cacheblock_mru[i] = 0;
    }

    filecache_ready = 1;
}

/////////////////////////////////////////////////////////////////////////////
//
// quick and dirty
//
void filecache_wait_for_prebuffer(int vfd, int nblocks)
{
    filecache_vfd *record = filecache_get_vfd(vfd);

    if(!record || record->read_block < 0)
    {
        return;
    }
    if((record->read_block + nblocks) > total_pakblocks)
    {
        nblocks = total_pakblocks - record->read_block;
    }
    while(get_vfd_blocks_available(record) < nblocks)
    {
        filecache_process();
    }
}

/////////////////////////////////////////////////////////////////////////////
