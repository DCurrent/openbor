/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

/*
	Code to read files from packfiles.

	============= Format description (a bit cryptical) ================

	dword	4B434150 ("PACK")
	dword	version
	?????	filedata
	{
		dword	structsize
		dword	filestart
		dword	filesize
		bytes	name
	} rep
	dword	headerstart
*/
#include <assert.h>
#ifndef SPK_SUPPORTED

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "borendian.h"
#include "stristr.h"
#include "packfile.h"
#include "filecache.h"
#include "tracemalloc.h"
#include "globals.h"
#include "openbor.h"

#ifdef PS2
#include <sifdev.h>
#endif

#if GP2X || LINUX || DINGOO || SYMBIAN
#define	stricmp	strcasecmp
#endif

#ifndef DC
#pragma pack (1)
#endif


/////////////////////////////////////////////////////////////////////////////
//
// Requirements for Compressed Packfiles.
//
#define	MAXPACKHANDLES	8
#define	PACKMAGIC		0x4B434150
#define	PACKVERSION		0x00000000

/////////////////////////////////////////////////////////////////////////////
//
// This defines are only used for Cached code
// CACHEBLOCKSIZE*CACHEBLOCKS is the size of the ever-present file cache
// cacheblocks must be 255 or less!
//
#define CACHEBLOCKSIZE (32768)
#ifndef DINGOO
#define CACHEBLOCKS    (96)
#else
#define CACHEBLOCKS    (8)
#endif

static int pak_initilized;

/////////////////////////////////////////////////////////////////////////////
//
// This variables are only used for Non-Cached code
//
// Actual file handles.
static int packhandle[MAXPACKHANDLES] = { -1,-1,-1,-1,-1,-1,-1,-1 };

// Own file pointers and sizes
static unsigned int packfilepointer[MAXPACKHANDLES];
static unsigned int packfilesize[MAXPACKHANDLES];

/////////////////////////////////////////////////////////////////////////////
//
// This variables are only used for with Caching code
//
static int pakfd;
static int paksize;
static int pak_vfdexists[MAXPACKHANDLES];
static int pak_vfdstart[MAXPACKHANDLES];
static int pak_vfdsize[MAXPACKHANDLES];
static int pak_vfdpos[MAXPACKHANDLES];
static int pak_vfdreadahead[MAXPACKHANDLES];
static int pak_headerstart;
static int pak_headersize;
static unsigned char *pak_cdheader;
static unsigned char *pak_header;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// Pointers to the Real Functions
//
typedef int (*OpenPackfile)(char*, char*);
typedef int (*ReadPackfile)(int, void*, int);
typedef int (*SeekPackfile)(int, int, int);
typedef int (*ClosePackfile)(int);

int openPackfile(char*, char*);
int readPackfile(int, void*, int);
int seekPackfile(int, int, int);
int closePackfile(int);
int openPackfileCached(char*, char*);
int readPackfileCached(int, void*, int);
int seekPackfileCached(int, int, int);
int closePackfileCached(int);

static OpenPackfile pOpenPackfile;
static ReadPackfile pReadPackfile;
static SeekPackfile pSeekPackfile;
static ClosePackfile pClosePackfile;

/////////////////////////////////////////////////////////////////////////////
//
// Generic but useful functions
//
char tolowerOneChar(char* c)
{
	static const char diff = 'a' - 'A';
	switch(*c) {
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': 
		case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': 
		case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z': 
			return *c + diff;
		case '\\':
			return '/';
		default:
			return *c;
	}
	return '\0'; //should never be reached
}

// we only return 0 on success, and non 0 on failure, to speed it up
int myfilenamecmp(const char *a, size_t asize, const char *b, size_t bsize)
{
	char* ca;
	char* cb;
	
	if (a == b) return 0;
	if(asize != bsize) return 1;
	
	ca = (char*) a;
	cb = (char*) b;
	
	for (;;) {
		if (!*ca) {
			if (*cb) return -1;
			else return 0; // default exit on match
		}
		if (!*cb) return 1;
		if (*ca == *cb) goto cont;
		if (tolowerOneChar(ca) != tolowerOneChar(cb)) return 1;
		cont:
		ca++;
		cb++;		
	}
	return -2; // should never be reached
}

// Convert slashes (UNIX) to backslashes (DOS).
// Return a pointer to buffer with filename converted to DOS format.
static char * slashback(char *sz)
{
    int i=0;
    static char new[256];
    while(sz[i] && i<255)
	{
		new[i] = sz[i];
		if(new[i]=='/') new[i]='\\';
		++i;
    }
    new[i] = 0;
    return new;
}

#ifndef WIN
// Convert backslashes (DOS) to forward slashes (everything else).
// Return a pointer to buffer with filename using forward slash as separator.
static char * slashfwd(char *sz)
{
    int i=0;
    static char new[256];
    while(sz[i] && i<255)
	{
		new[i] = sz[i];
		if(new[i]=='\\') new[i]='/';
		++i;
    }
    new[i] = 0;
    return new;
}
#endif

#ifdef LINUX
char * casesearch(const char *dir, const char *filepath)
{
	DIR *d;
	struct dirent *entry;;
	char filename[256] = {""}, *rest_of_path;
	static char fullpath[256];
	int i=0;
#ifdef VERBOSE
	printf("casesearch: %s, %s\n", dir, filepath);
#endif
	
	if ((d=opendir(dir)) == NULL) return NULL;
	
	// are we searching for a file or a directory?
	rest_of_path = strchr(filepath, '/');
	if (rest_of_path != NULL) // directory
	{
		assert(rest_of_path-filepath > 0);
		strncat(filename, filepath, rest_of_path-filepath);
		rest_of_path++;
	} else strcpy(filename, filepath); // file

	while ((entry=readdir(d)) != NULL)
	{
		if (stricmp(entry->d_name, filename) == 0)
		{
			i = 1;
			break;
		}
	}
	
	if (entry != NULL && entry->d_name != NULL)
		sprintf(fullpath, "%s/%s", dir, entry->d_name);
	
	if (closedir(d)) return NULL;
	if (i == 0) return NULL;	
	
	return rest_of_path == NULL ? fullpath : casesearch(fullpath, rest_of_path);
}

#endif

/////////////////////////////////////////////////////////////////////////////

void packfile_mode(int mode)
{
	if(!mode)
	{
#ifdef DC
		fs_chdir("/cd");
#endif
		pOpenPackfile = openPackfile;
        pReadPackfile = readPackfile;
        pSeekPackfile = seekPackfile;
        pClosePackfile = closePackfile;
		return;
	}
	pOpenPackfile = openPackfileCached;
    pReadPackfile = readPackfileCached;
    pSeekPackfile = seekPackfileCached;
    pClosePackfile = closePackfileCached;
}


/////////////////////////////////////////////////////////////////////////////

int isRawData()
{
	DIR *d;
	if ((d=opendir("data")) == NULL) return 0;
	closedir(d);
	return 1;
}

/////////////////////////////////////////////////////////////////////////////

int openpackfile(char *filename, char *packfilename)
{
#ifdef VERBOSE
	char* pointsto;

	if (pOpenPackfile == openPackfileCached)
		pointsto = "oPackCached";
	else if (pOpenPackfile == openPackfile)
		pointsto = "openPackFile";
	else 
		pointsto = "unknown destination";
	printf ("openpackfile called: f: %s, p: %s, dest: %s\n", filename, packfilename, pointsto);
#endif
	return pOpenPackfile(filename, packfilename);
}

int openPackfile(char *filename, char *packfilename)
{
	int h, handle;
	unsigned int magic, version, headerstart, p;
	pnamestruct pn;
#ifdef LINUX
	char *fspath;
#endif

	for(h=0; h<MAXPACKHANDLES && packhandle[h]>-1; h++); // Find free handle
	if(h>=MAXPACKHANDLES) {
#ifdef VERBOSE
		printf ("no free handles\n");
#endif
		return -1;			// No free handles
	}

#ifdef WIN
	// Convert slashes to backslashes
	filename = slashback(filename);
#else
	// Convert backslashes to slashes
	filename = slashfwd(filename);
#endif

	packfilepointer[h] = 0;

	// Separate file present?
	if((handle=open(filename, O_RDONLY|O_BINARY, 777))!=-1)
	{
		if((packfilesize[h]=lseek(handle,0,SEEK_END))==-1)
		{
			#ifdef VERBOSE
			printf ("err handles 1\n");
			#endif			
			close(handle);
			return -1;
		}
		if(lseek(handle,0,SEEK_SET)==-1)
		{
			#ifdef VERBOSE
			printf ("err handles 2\n");
			#endif						
			close(handle);
			return -1;
		}
		packhandle[h] = handle;
		return h;
	}

#ifdef LINUX
	// Try a case-insensitive search for a separate file.
	fspath = casesearch(".", filename);
	if (fspath != NULL)
	{
		if((handle=open(fspath, O_RDONLY|O_BINARY, 777))!=-1)
		{
			if((packfilesize[h]=lseek(handle,0,SEEK_END))==-1)
			{
				#ifdef VERBOSE
				printf ("err handles 3\n");
				#endif				
				close(handle);
				return -1;
			}
			if(lseek(handle,0,SEEK_SET)==-1)
			{
				#ifdef VERBOSE
				printf ("err handles 4\n");
				#endif							
				close(handle);
				return -1;
			}
			packhandle[h] = handle;
			return h;
		}
	}
#endif

#ifndef WIN
	// Convert slashes to backslashes
	filename = slashback(filename);
#endif

	// Try to open packfile
	if((handle=open(packfilename, O_RDONLY|O_BINARY, 777))==-1) {
		#ifdef VERBOSE
		printf ("perm err\n");
		#endif					
		return -1;
	}
	    

	// Read magic dword ("PACK" identifier)
	if(read(handle,&magic,4)!=4 || magic!=SwapLSB32(PACKMAGIC))
	{
		#ifdef VERBOSE
		printf ("err magic\n");
		#endif					
		close(handle);
		return -1;
	}
	// Read version from packfile
	if(read(handle,&version,4)!=4 || version!=SwapLSB32(PACKVERSION))
	{
		#ifdef VERBOSE
		printf ("err version\n");
		#endif			
		
		close(handle);
		return -1;
	}

	// Seek to position of headerstart indicator
	if(lseek(handle,-4,SEEK_END)==-1)
	{
		#ifdef VERBOSE
		printf ("seek failed\n");
		#endif					
		close(handle);
		return -1;
	}
	// Read headerstart
	if(read(handle,&headerstart,4)!=4)
	{
		#ifdef VERBOSE
		printf ("err header\n");
		#endif					
		close(handle);
		return -1;
	}

	headerstart = SwapLSB32(headerstart);

	// Seek to headerstart
	if(lseek(handle,headerstart,SEEK_SET)==-1)
	{
		#ifdef VERBOSE
		printf ("err headerstart 1\n");
		#endif
		close(handle);
		return -1;
	}

	p = headerstart;

    // Search for filename
	while(read(handle,&pn,sizeof(pn))>12)
	{
		pn.filesize = SwapLSB32(pn.filesize);
		pn.filestart = SwapLSB32(pn.filestart);
		pn.pns_len = SwapLSB32(pn.pns_len);

		if(stricmp(filename,pn.namebuf)==0)
		{
			packhandle[h] = handle;
			packfilesize[h] = pn.filesize;
			lseek(handle,pn.filestart,SEEK_SET);
			return h;
		}
		p += pn.pns_len;
		if(lseek(handle,p,SEEK_SET)==-1)
		{
			#ifdef VERBOSE
			printf ("err seek handles\n");
			#endif						
			close(handle);			
			return -1;
		}
	}
	// Filename not found
	#ifdef VERBOSE
	printf ("err filename not found\n");
	#endif
	close(handle);
	return -1;
}

void update_filecache_vfd(int vfd)
{
    if(pak_vfdexists[vfd]) filecache_setvfd(vfd, pak_vfdstart[vfd], (pak_vfdstart[vfd] + pak_vfdpos[vfd]) / CACHEBLOCKSIZE, (pak_vfdreadahead[vfd] + (CACHEBLOCKSIZE-1)) / CACHEBLOCKSIZE);
    else filecache_setvfd(vfd, -1, -1, 0);
}

int openreadaheadpackfile(char *filename, char *packfilename, int readaheadsize, int prebuffersize)
{
	int hpos;
	int vfd;
	size_t fnl;
	size_t al;
	char* target;   
    
	if(packfilename != packfile) {
		fnl = strlen(packfile);
		al = strlen(packfilename);
		if(myfilenamecmp(packfilename, al, packfile, fnl))
		{
			printf("tried to open from unknown pack file (%s)\n", packfilename);
			return -1;
		}
	}

    // find a free vfd
	for(vfd = 0; vfd < MAXPACKHANDLES; vfd++) if(!pak_vfdexists[vfd]) break;
	if(vfd >= MAXPACKHANDLES) return -1;

	// look for filename in the header
	fnl = strlen(filename);
	hpos = 0;
	for(;;)
	{
		if((hpos + 12) >= pak_headersize) return -1;
		target = (char*)pak_header + hpos + 12;
		al = strlen(target);
	
		if(myfilenamecmp(target, al, filename, fnl))
		{
			hpos += readlsb32(pak_header + hpos);
			continue;
		}
		// found!
		pak_vfdstart[vfd] = readlsb32(pak_header + hpos + 4);
		pak_vfdsize[vfd] = readlsb32(pak_header + hpos + 8);
		break;
	}

	pak_vfdpos[vfd] = 0;
	pak_vfdexists[vfd] = 1;
	pak_vfdreadahead[vfd] = readaheadsize;

	// notify filecache that we have a new vfd
	update_filecache_vfd(vfd);

	// if we want prebuffering, wait for it
	if(prebuffersize > 0) filecache_wait_for_prebuffer(vfd, (prebuffersize + ((CACHEBLOCKSIZE)-1)) / CACHEBLOCKSIZE);
	return vfd;
}

int openPackfileCached(char *filename, char *packfilename)
{
	return openreadaheadpackfile(filename, packfilename, 0, 0);
}

/////////////////////////////////////////////////////////////////////////////

int readpackfile(int handle, void *buf, int len)
{
	return pReadPackfile(handle, buf, len);
}

int readPackfile(int handle, void *buf, int len)
{
    int realhandle;
    if(handle<0 || handle>=MAXPACKHANDLES) return -1;
    if(len<0) return -1;
    if(len==0) return 0;
    realhandle = packhandle[handle];
    if(realhandle == -1) return -1;
    if(len + packfilepointer[handle] > packfilesize[handle])
	len = packfilesize[handle] - packfilepointer[handle];
    if((len = read(realhandle,buf,len))==-1) return -1;
    packfilepointer[handle] += len;
    return len;
}

int pak_isvalidhandle(int handle)
{
    if(handle < 0 || handle >= MAXPACKHANDLES) return 0;
	if(!pak_vfdexists[handle]) return 0;
	return 1;
}

static int pak_rawread(int fd, unsigned char *dest, int len, int blocking)
{
    int end;
    int r;
    int total = 0;
    int pos = pak_vfdstart[fd] + pak_vfdpos[fd];

    if(pos < 0) return 0;
    if(pos >= paksize) return 0;
    if((pos + len) > paksize) { len = paksize - pos; }
    end = pos + len;

    update_filecache_vfd(fd);

    while(pos < end)
    {
        int b = pos / CACHEBLOCKSIZE;
        int startthisblock = pos % CACHEBLOCKSIZE;
        int sizethisblock = CACHEBLOCKSIZE - startthisblock;
        if(sizethisblock > (end-pos)) sizethisblock = (end-pos);
        r = filecache_readpakblock(dest, b, startthisblock, sizethisblock, blocking);
        if(r >= 0)
        {
            total += r;
            pak_vfdpos[fd] += r;
            update_filecache_vfd(fd);
        }
        if(r < sizethisblock) break;

        dest += sizethisblock;
        pos += sizethisblock;
    }
    return total;
}

int readpackfileblocking(int fd, void *buf, int len, int blocking)
{
    int n;
    if(!pak_isvalidhandle(fd)) return -1;
    if(pak_vfdpos[fd] < 0) return 0;
    if(pak_vfdpos[fd] > pak_vfdsize[fd]) pak_vfdpos[fd] = pak_vfdsize[fd];
    if((len + pak_vfdpos[fd]) > pak_vfdsize[fd]) { len = pak_vfdsize[fd] - pak_vfdpos[fd]; }
    if(len < 1) return 0;
    update_filecache_vfd(fd);
    n = pak_rawread(fd, buf, len, blocking);
    if(n < 0) n = 0;
    if(pak_vfdpos[fd] > pak_vfdsize[fd]) pak_vfdpos[fd] = pak_vfdsize[fd];
    update_filecache_vfd(fd);
    return n;
}

int readpackfile_noblock(int handle, void *buf, int len)
{
    return readpackfileblocking(handle, buf, len, 0);
}

int readPackfileCached(int handle, void *buf, int len)
{
    return readpackfileblocking(handle, buf, len, 1);
}

/////////////////////////////////////////////////////////////////////////////

int closepackfile(int handle)
{
#ifdef VERBOSE
	char* pointsto;
	
	if (pClosePackfile == closePackfileCached)
		pointsto = "closePackCached";
	else if (pClosePackfile == closePackfile)
		pointsto = "closePackFile";
	else 
		pointsto = "unknown destination";
	printf ("closepackfile called: h: %d, dest: %s\n", handle, pointsto);
#endif	
	return pClosePackfile(handle);
}

int closePackfile(int handle)
{
#ifdef VERBOSE
	printf ("closePackfile called: h: %d\n", handle);
#endif	
	
	if(handle<0 || handle>=MAXPACKHANDLES) 
	{
#ifdef VERBOSE
		printf("handle too small/big\n");
#endif
		return -1;
	}
	if(packhandle[handle] == -1) {
#ifdef VERBOSE
		printf("packhandle -1\n");
#endif		
		return -1;
	}
	close(packhandle[handle]);
	packhandle[handle] = -1;
	return 0;
}

int closePackfileCached(int handle)
{
    if(!pak_isvalidhandle(handle)) return -1;
	pak_vfdexists[handle] = 0;
	update_filecache_vfd(handle);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////

int seekpackfile(int handle, int offset, int whence)
{
	return pSeekPackfile(handle, offset, whence);
}

int seekPackfile(int handle, int offset, int whence)
{
    int realhandle;
    int desiredoffs;

    if(handle<0 || handle>=MAXPACKHANDLES) return -1;
    realhandle = packhandle[handle];
    if(realhandle == -1) return -1;

    switch(whence)
	{
		case SEEK_SET:
			desiredoffs = offset;
			if(desiredoffs>packfilesize[handle])
				desiredoffs = packfilesize[handle];
			else if(desiredoffs<0)
				desiredoffs = 0;
			break;

		case SEEK_CUR:
			desiredoffs = packfilepointer[handle] + offset;
			if(desiredoffs>packfilesize[handle])
				desiredoffs = packfilesize[handle];
			else if(desiredoffs<0)
				desiredoffs = 0;
			break;

		case SEEK_END:
			desiredoffs = packfilesize[handle] + offset;
			if(desiredoffs>packfilesize[handle])
				desiredoffs = packfilesize[handle];
			else if(desiredoffs<0)
				desiredoffs = 0;
			break;

		default:
			return -1;
    }
    desiredoffs -= packfilepointer[handle];
    if((lseek(realhandle,desiredoffs,SEEK_CUR))==-1) return -1;
    packfilepointer[handle] += desiredoffs;
    return packfilepointer[handle];
}

int seekPackfileCached(int handle, int offset, int whence)
{
    if(!pak_isvalidhandle(handle)) return -1;
	switch(whence)
	{
		case 0: pak_vfdpos[handle]  = offset;                   break; // set
		case 1: pak_vfdpos[handle] += offset;                   break; // cur
		case 2: pak_vfdpos[handle]  = pak_vfdsize[handle] + offset; break; // end
		default: return -1;
	}
	// original code had this check too, so do it here
	if(pak_vfdpos[handle] < 0) pak_vfdpos[handle] = 0;
	if(pak_vfdpos[handle] > pak_vfdsize[handle]) pak_vfdpos[handle] = pak_vfdsize[handle];
	update_filecache_vfd(handle);
	return pak_vfdpos[handle];
}

#ifdef PS2
static void flushfd(int fd)
{
	int busy = 1;
	while(busy) sceIoctl(fd, SCE_FS_EXECUTING, &busy);
}
#endif

/////////////////////////////////////////////////////////////////////////////
//
// returns number of sectors read successfully
//
static int pak_getsectors(void *dest, int lba, int n)
{
#ifdef PS2
	sceLseek(pakfd, lba << 11, SCE_SEEK_SET);
	flushfd(pakfd);
	sceRead(pakfd, dest, n << 11);
	flushfd(pakfd);
#elif DC
	if((lba+n) > ((paksize+0x7FF)/0x800)) {
    	n = ((paksize+0x7FF)/0x800)-lba;
  	}
	if(pakfd >= 0) {
	    lseek(pakfd, lba << 11, SEEK_SET);
	    read(pakfd, dest, n << 11);
	} else {
	    gdrom_readsectors(dest, (-pakfd)+lba, n);
	    while(gdrom_poll());
	}
#else
    int disCcWarns;
    lseek(pakfd, lba << 11, SEEK_SET);
    disCcWarns = read(pakfd, dest, n << 11);
#endif
    return n;
}

#ifdef DC
/////////////////////////////////////////////////////////////////////////////
//
// returns 0 if they match
//
static int fncmp(const char *filename, const char *isofilename, int isolen)
{
	for(; isolen > 0; isolen--)
	{
		char cf = *filename++;
		char ci = *isofilename++;
		if(!cf)
		{
		  // allowed to omit the version on filename
		  if(ci == ';' || ci == 0) return 0;
		  return 1;
		}
		if(cf >= 'A' && cf <= 'Z') cf += 'a' - 'A';
		if(ci >= 'A' && ci <= 'Z') ci += 'a' - 'A';
		if(cf != ci) return 1;
	}
	// allowed to omit the version on isofilename too O_o
	if(*filename == ';' || *filename == 0) return 0;
  	return 1;
}

/////////////////////////////////////////////////////////////////////////////
//
// input: starting lba of the track
// returns starting lba of the file, or 0 on failure
//
int find_iso_file(const char *filename, int lba, int *bytes)
{
	int dirlen;
  	unsigned char sector[4096];
	int secofs;

  	// read the root descriptor
  	gdrom_readsectors(sector, lba+16, 1); while(gdrom_poll());
  	// get the root directory extent and size
  	lba    = 150 + readmsb32(sector+156+6);
  	dirlen =       readmsb32(sector+156+14);

	// at this point, lba is the lba of the root dir
	secofs = 4096;
  	while(dirlen > 0) {
  		if(secofs >= 4096 || ((secofs + sector[secofs]) > 4096)) {
    		memcpy(sector, sector+2048, 2048);
    	  	gdrom_readsectors(sector+2048, lba, 1); while(gdrom_poll());
    	  	lba++;
    	  	secofs -= 2048;
    	}
    	if(!sector[secofs]) break;
		if(!fncmp(filename, sector+secofs+33, sector[secofs+32])) {
			lba = 150 + readmsb32(sector+secofs+6);
      		if(bytes) *bytes = readmsb32(sector+secofs+14);
      		return lba;
    	}
    	secofs += sector[secofs];
    	dirlen -= sector[secofs];
  	}
  	// didn't find the file
  	return 0;
}
#endif

/////////////////////////////////////////////////////////////////////////////

void pak_term()
{
	int i;
	if(!pak_initilized) return;
	if(pak_cdheader != NULL)
	{
		tracefree(pak_cdheader);
		pak_cdheader = NULL;
	}
	filecache_term();
	close(pakfd);
	pakfd           = -1;
	paksize         = -1;
	pak_headerstart = -1;
	pak_headersize  = -1;
	for(i=0; i<MAXPACKHANDLES; i++)
	{
        pak_vfdexists[i]    = -1;
		pak_vfdstart[i]     = -1;
		pak_vfdsize[i]      = -1;
		pak_vfdpos[i]       = -1;
		pak_vfdreadahead[i] = -1;
	}
	pak_initilized = 0;
}

/////////////////////////////////////////////////////////////////////////////

int pak_init()
{
	int i;
	unsigned char *sectors;
	unsigned int magic, version;

	if(pak_initilized)
	{
		printf("pak_init already initialized!");
		return 0;
	}
	
	if(isRawData())
	{
		pak_initilized = 1;
		return 0;
	}
	
	pOpenPackfile = openPackfileCached;
	pReadPackfile = readPackfileCached;
	pSeekPackfile = seekPackfileCached;
	pClosePackfile = closePackfileCached;

#if DC
	if(cd_lba)
	{
		paksize = 0;
		pakfd = find_iso_file(packfile, cd_lba, &paksize);
		if(pakfd <= 0) { printf("unable to find pak file on cd\n"); return 0; }
		pakfd = -pakfd;
  	}
	else
	{
#endif

#ifdef PS2
	pakfd = sceOpen(ps2gethostfilename(packfile), SCE_RDONLY|SCE_NOWAIT, 0);
#else
	pakfd = open(packfile, O_RDONLY|O_BINARY, 777);
#endif

	if(pakfd < 0)
	{
		printf("error opening %s (%d)\n", packfile, pakfd);
		return 0;
	}

#ifdef PS2
	paksize = sceLseek(pakfd, 0, SCE_SEEK_END);
#else
	paksize = lseek(pakfd, 0, SEEK_END);
#endif

#ifdef DC
	}
#endif

	// Is it a valid Packfile
#ifdef PS2
	sceClose(pakfd);
	pakfd = sceOpen(ps2gethostfilename(packfile), SCE_RDONLY, 0);
#else
	close(pakfd);
	pakfd = open(packfile, O_RDONLY|O_BINARY, 777);
#endif

	// Read magic dword ("PACK")
	if(read(pakfd,&magic,4)!=4 || magic!=SwapLSB32(PACKMAGIC))
	{
		close(pakfd);
		return -1;
	}
	// Read version from packfile
	if(read(pakfd,&version,4)!=4 || version!=SwapLSB32(PACKVERSION))
	{
		close(pakfd);
		return -1;
	}

	sectors = tracemalloc("pak_init 1",4096);
	if(!sectors) { printf("sector malloc failed\n"); return 0; }
	{
		int getptrfrom = paksize - 4;
		if(pak_getsectors(sectors, getptrfrom >> 11, 2) < 1)
		{
			printf("unable to read pak header pointer\n");
			return 0;
		}
		pak_headerstart = readlsb32(sectors + (getptrfrom & 0x7FF));
	}
	tracefree(sectors);
	if(pak_headerstart >= paksize || pak_headerstart < 0)
	{
		printf("invalid pak header pointer\n");
		return 0;
	}
	pak_headersize = paksize - pak_headerstart;
	{
		// let's cache it on CD sector boundaries
		int pak_cdheaderstart = pak_headerstart & (~0x7FF);
		int pak_cdheadersize = ((paksize - pak_cdheaderstart) + 0x7FF) & (~0x7FF);
		if(pak_cdheadersize > 524288)
		{
			// Original value was 262144, which has been doubled.
			// I can not find a reason why it was orginally set to
			// this size.  Hence, I have doubled it.  This could
			// pose a problem on optical media, but that is yet to be
			// determined.
			printf("pak header is too large: %d\n",pak_cdheadersize);
			return 0;
		}
		pak_cdheader = tracemalloc("pak_init 2",pak_cdheadersize);
		if(!pak_cdheader) { printf("pak_cdheader malloc failed\n"); return 0; }
		if(pak_getsectors(pak_cdheader, pak_cdheaderstart >> 11, pak_cdheadersize >> 11) != (pak_cdheadersize >> 11))
		{
			printf("unable to read pak header\n");
			return 0;
		}
		// ok, header is now cached
		pak_header = pak_cdheader + (pak_headerstart & 0x7FF);
	}
	// header does not include the last 4-byte stuff
	if(pak_headersize >= 4)
	{
		pak_headersize -= 4;
		// add a trailing null o/~
		pak_header[pak_headersize] = 0;
	}
	//printf("pak cached header (%d bytes)\n", pak_headersize);
	// initialize vfd table
	for(i = 0; i < MAXPACKHANDLES; i++) pak_vfdexists[i] = 0;
	// finally, initialize the filecache
	filecache_init(pakfd, (paksize + 0x7FF) / 0x800, CACHEBLOCKSIZE, CACHEBLOCKS, MAXPACKHANDLES);
	pak_initilized = 1;
	return (CACHEBLOCKSIZE * CACHEBLOCKS + 64);
}

/////////////////////////////////////////////////////////////////////////////

int packfileeof(int handle)
{
    if(!pak_isvalidhandle(handle)) return -1;
	return (pak_vfdpos[handle] >= pak_vfdsize[handle]);
}

/////////////////////////////////////////////////////////////////////////////

int packfile_supported(struct dirent* ds)
{
	if(stricmp(ds->d_name, "menu.pak") != 0)
	{
		if (stristr(ds->d_name, ".pak")) return 1;
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////

void packfile_get_titlename(char In[80], char Out[80])
{
	int i, x=0, y=0;
	for(i=0; i<(int)strlen(In); i++){
		if((In[i] == '/') || (In[i] == '\\')) x = i;
	}
	for(i=0; i<(int)strlen(In); i++){
		if(i > x){
			Out[y] = In[i];
			y++;
		}
	}
}

void packfile_music_read(fileliststruct *filelist, int dListTotal)
{
	pnamestruct pn;
	FILE *fd;
	int len, i;
	unsigned int off;
	char pack[4], *p = NULL;
	for(i = 0; i < dListTotal; i++)
	{
		getBasePath(packfile, filelist[i].filename, 1);
		if(stristr(packfile, ".pak"))
		{
			memset(filelist[i].bgmTracks, 0, 256);
			filelist[i].nTracks = 0;
			fd = fopen(packfile, "rb");
			if(fd == NULL) continue;
			if(!fread(pack, 4, 1, fd)) goto closepak;
			if(fseek(fd, -4, SEEK_END) < 0) goto closepak;
			if(!fread(&off, 4, 1, fd)) goto closepak;
			if(fseek(fd, off, SEEK_SET) < 0) goto closepak;
			while((len = fread(&pn, 1, sizeof(pn), fd)) > 12) {
				p = strrchr(pn.namebuf, '.');
				if((p && !stricmp(p, ".bor")) || (stristr(pn.namebuf, "music"))) {
					if(!stristr(pn.namebuf, ".bor")) goto nextpak;
					if(filelist[i].nTracks < 256)
					{
						packfile_get_titlename(pn.namebuf, filelist[i].bgmFileName[filelist[i].nTracks]);
						filelist[i].bgmTracks[filelist[i].nTracks] = off;
						filelist[i].nTracks++;
					}
				}
nextpak:
				off += pn.pns_len;
				if(fseek(fd, off, SEEK_SET) < 0) goto closepak;
			}
closepak:
			fclose(fd);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////

int packfile_music_play(struct fileliststruct* filelist, FILE *bgmFile, int bgmLoop, int curPos, int scrPos)
{
	pnamestruct pn;
	int len;
	getBasePath(packfile, filelist[curPos+scrPos].filename, 1);
	if (bgmFile)
	{
		fclose(bgmFile);
		bgmFile = NULL;
	}
	bgmFile = fopen(packfile, "rb");
	if (!bgmFile) return 0;
	if (stristr(packfile, ".pak"))
	{
		if(fseek(bgmFile, filelist[curPos+scrPos].bgmTracks[filelist[curPos+scrPos].bgmTrack], SEEK_SET) < 0) return 0;
		if((len = fread(&pn, 1, sizeof(pn), bgmFile)) > 12) sound_open_music(pn.namebuf, packfile, savedata.musicvol, bgmLoop, 0);
	}
	return 1;
}

#endif



