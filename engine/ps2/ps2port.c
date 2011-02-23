/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <kernel.h>
#include <loadfile.h>
#include <sifrpc.h>
#include <libcdvd.h>
#include <iopheap.h>
#include <libmc.h>

#include "ps2port.h"

#include "gsvga.h"
#include "ps2pak.h"
#include "timer.h"
#include "control.h"
#include "ps2sdr.h"

#include "borstartup.h"

/////////////////////////////////////////////////////////////////////////////

static char ps2hostdev[256];

int is_cdrom = 0;

//
// NOT REENTRANT
//
const char *ps2gethostfilename(const char *filename) {
  static char t[256];
  char *dest;
  const char *src;
  char slashchar = '/';

  dest = t;
  src = ps2hostdev;
  while(*src) {
    char c = *src++;
    if(c == '/' || c == '\\') slashchar = c;
    *dest++ = c;
  }
  src = filename;
  while(*src) {
    char c = *src++;
    if(c == '/' || c == '\\') c = slashchar;
    if(is_cdrom && c >= 'a' && c <= 'z') c += ('A' - 'a');
    *dest++ = c;
  }
  if(is_cdrom) {
    *dest++ = ';';
    *dest++ = '1';
  }
  *dest = 0;

  return t;
}

/////////////////////////////////////////////////////////////////////////////

unsigned readlsb32(const unsigned char *src) {
  return
    ((((unsigned)(src[0])) & 0xFF) <<  0) |
    ((((unsigned)(src[1])) & 0xFF) <<  8) |
    ((((unsigned)(src[2])) & 0xFF) << 16) |
    ((((unsigned)(src[3])) & 0xFF) << 24);
}

/////////////////////////////////////////////////////////////////////////////

void bor_main(int argc, char **argv);
extern u32 _gp;

/////////////////////////////////////////////////////////////////////////////

void LoadModule(char *path, int argc, char *argv) {
  int ret;
    
  ret = SifLoadModule(path, argc, argv);
  if (ret < 0) {
    debug_printf("Could not load module %s: %d\n", path, ret);
    SleepThread();
  }
}

extern unsigned char sjpcm_irx_embed[];
extern unsigned int size_sjpcm_irx_embed;

enum {
  BOOT_TYPE_CD = 0,
  BOOT_TYPE_DVD,
  BOOT_TYPE_HOST,
  BOOT_TYPE_MC,
  BOOT_TYPE_UNKNOWN = -1
};

int detects_boot(int argc, char ** argv) {
  int i;
  char * p;

  if (argc == 0) { // blehs naplink...
    strcpy(ps2hostdev, "host:");
    return BOOT_TYPE_HOST;
  }
  
  // get the host device name
  strcpy(ps2hostdev, argv[0]);
  
  p = strrchr(ps2hostdev, '/');
  
  if (!p) {
    p = strrchr(ps2hostdev, '\\');
  }
  
  if (!p) {
    p = strrchr(ps2hostdev, ':');
  }
  
  if (!p) {
    return BOOT_TYPE_UNKNOWN;
  }
  
  *(++p) = 0;

  i = p - ps2hostdev;
  
  ps2hostdev[i] = 0;

  // if the exe name starts with 'cd', force cdrom0 as the host device, and
  // set up boot from CD.
  if (strncasecmp("cd", argv[0] + i, 2) == 0) {
    strcpy(ps2hostdev, "cdrom0:\\");
    is_cdrom = 1;
    return BOOT_TYPE_CD;
  }

  // if the exe name starts with 'dvd', force cdrom0 as the host device, and
  // set up boot from DVD.
  if (strncasecmp("dvd", argv[0] + i, 3) == 0) {
    strcpy(ps2hostdev, "cdrom0:\\");
    is_cdrom = 1;
    return BOOT_TYPE_DVD;
  }
  
  if (strncasecmp("host", argv[0] + i, 4) == 0) {
    strcpy(ps2hostdev, "host:");
    return BOOT_TYPE_HOST;
  }
  
  if (strncasecmp(ps2hostdev, "mc", 2) == 0) {
    return BOOT_TYPE_MC;
  }
  
  return BOOT_TYPE_UNKNOWN;
}

int main(int argc, char **argv) {
  int ret;
  int boot_method;
  
  boot_method = detects_boot(argc, argv);

  SifInitRpc(0);
  SifInitIopHeap();

  debug_printf("Startup of BoR - ps2sdk version " PS2VERSION "\n");

  LoadModule("rom0:SIO2MAN", 0, NULL);
  LoadModule("rom0:PADMAN", 0, NULL);
  LoadModule("rom0:MCMAN", 0, NULL);
  LoadModule("rom0:MCSERV", 0, NULL);
  SifExecModuleBuffer(sjpcm_irx_embed, size_sjpcm_irx_embed, 0, NULL, &ret);  
  
  if (mcInit(MC_TYPE_MC) < 0) {
    debug_printf("Failed to initialise memcard server.\n");
    SleepThread();
  }
  
  // perform necessary stuff for booting off a CD
  if (boot_method == BOOT_TYPE_CD) {
    debug_printf("CD init.\n");
    cdInit(CDVD_INIT_INIT);
    cdSetMediaMode(CDVD_MEDIA_MODE_CD);
  }

  // perform necessary stuff for booting off a DVD
  if (boot_method == BOOT_TYPE_DVD) {
    debug_printf("DVD init.\n");
    cdInit(CDVD_INIT_INIT);
    cdSetMediaMode(CDVD_MEDIA_MODE_DVD);
  }

  ChangeThreadPriority(GetThreadId(), 8);

  gsvga_init();

  borstartup_draw();

  timer_init();

  control_ps2init();

  ps2sdr_init();

// determine timer freq
/*
  { unsigned a = timer_gettime();
    for(i = 0; i < 60; i++) gsvga_draw((unsigned char*)main);
    a = timer_gettime() - a;
    debug_printf("elapsed time = %u\n", a);
    for(;;);
  }
*/

//for(i = 0;; i++) { debug_printf("%d\n", i); } // do shit

//  debug_printf("%s\n", argv[0]);

/*
{  unsigned total = 0;
  for(i = 0;; i++) {
    unsigned n = 100000;
    unsigned q = (unsigned)tracemalloc("main", n);
    if(q) {
      total += n;
      debug_printf("allocated %d bytes (%X)\n", total, q);
    } else {
      debug_printf("alloc failed! total bytes %d\n", total);
      for(;;);
    }
  }
}
*/

  // open the pak
  ps2pak_init();

  // run the game
  bor_main(0, 0);

  ExitDeleteThread();
  
  return 0;
}

/////////////////////////////////////////////////////////////////////////////

void _ps2sdk_alloc_init();
void _ps2sdk_alloc_deinit();
void _ps2sdk_stdio_init();
void _ps2sdk_stdio_deinit();
void _ps2sdk_stdlib_init();
void _ps2sdk_stdlib_deinit();

int chdir(const char *path);

void _ps2sdk_libc_init()
{
	_ps2sdk_alloc_init();
	_ps2sdk_stdio_init();
	_ps2sdk_stdlib_init();
}

void _ps2sdk_libc_deinit()
{
	_ps2sdk_stdlib_deinit();
	_ps2sdk_stdio_deinit();
	_ps2sdk_alloc_deinit();
}


void _ps2sdk_args_parse(int argc, char ** argv)
{
	if (argc == 0) // naplink!
	{
		chdir("host:");
	} else {
		char * p, * s = 0;
		// let's find the last slash, or at worst, the :
		for (p = argv[0]; *p; p++) {
			if ((*p == '/') || (*p == '\\') || (*p == ':')) {
				s = p;
			}
		}
		// Nothing?! strange, let's use host.
		if (!s) {
			chdir("host:");
		} else {
			char backup = *(++s);
			*s = 0;
			chdir(argv[0]);
			*s = backup;
		}
	}
}


