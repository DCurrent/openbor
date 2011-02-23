/*
	Copyright 2006 Luigi Auriemma
	Copyright 2009 Bryan Cain

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

	http://www.gnu.org/licenses/gpl.txt
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef WIN32
	#include <direct.h>
	#include <windows.h>
	#include "stristr.h"

	#define MKDIR(x)    mkdir(x)
	#define PATHSLASH   '\\'
#else
	#include <unistd.h>
	
	#define stristr		strcasestr
	#define MKDIR(x)    mkdir(x, 0755)
	#define PATHSLASH   '/'
#endif


#define FNAMEMAX        1024

#define FWRITE(X,Y,Z)   if(fwrite(X, Y, 1, Z) != 1) write_err();



int put_file(FILE *fd, char *fname);
void get_file(FILE *fd, char *fname, u_int off, u_int size);
u_int fdrinum(FILE *fd, int size);
void fdwinum(FILE *fd, u_int num, int size);
int recursive_dir(FILE *fd, char *filedir);
void write_err(void);
void std_err(void);


typedef struct {
	u_int   pns;
	u_int   off;
	u_int   size;
	char    *name;
} pn_t;

struct pnp_t {
	pn_t            pn;
	struct pnp_t    *next;
};



struct  pnp_t   *pnp = NULL;



int main(int argc, char *argv[]) {
	struct  pnp_t   *ps,
					*psfree;
	pn_t    pn;
	FILE    *fd;
	u_int   off;
	int     i,
			len,
			list    = 0,
			build   = 0,
			packver = 0,
			filez   = 0;
	char  pack[4],
			*dir    = NULL,
			*patt   = NULL,
			*fname;

	setbuf(stdout, NULL);

	fputs("\n"
		"BOR PAK extractor/builder v0.1\n"
		"originally by Luigi Auriemma\n"
		"e-mail: aluigi@autistici.org\n"
		"web:    aluigi.org\n"
		"\n"
		"Updated to v0.2 by SX\n"
		"e-mail: sumolx@gmail.com\n"
		"web:    http://lavalit.com:8080/\n"
		"\n"
		"Updated to v0.3 by Plombo\n"
		"web:    http://lavalit.com:8080/\n"
		"\n", stdout);
	
	if(argc < 2) {
		printf("\n"
			"Usage: %s [options] <file.PAK>\n"
			"\n"
			"-d DIR   files folder, default is the current\n"
			"-b       build a PAK from the files folder, default is extraction\n"
			"-l       list files without extracting\n"
			"-p PAT   extract only the files which contain PAT in their name\n"
			"\n", argv[0]);
		exit(1);
	}
	
	argc--;
	for(i = 1; i < argc; i++) {
		if(argv[i][0] != '-') break;
		switch(argv[i][1]) {
			case 'd': dir   = argv[++i];    break;
			case 'b': build = 1;            break;
			case 'l': list  = 1;            break;
			case 'p': patt  = argv[++i];    break;
			default: {
				printf("\nError: wrong command-line argument (%s)\n\n", argv[i]);
				exit(1);
				} break;
		}
	}
	fname = argv[argc];
	
	if(build) {
		if(!dir) {
			printf("\n"
				"Error: please specify the files directory with the -d option and don't specify\n"
				"       the current\n\n");
			exit(1);
		}

		printf("- create file: %s\n", fname);
		printf("- directory: %s\n\n", dir);
		fd = fopen(fname, "rb");
		if(fd) {
			fclose(fd);
			printf("- a file with the same name already exists, overwrite? (y/N)\n  ");
			fflush(stdin);
			if(tolower(fgetc(stdin)) != 'y') exit(1);
		}
		fd = fopen(fname, "wb");
		if(!fd) std_err();

		FWRITE("PACK", 4, fd);
		fdwinum(fd, packver, 32);

		printf(
			"    offset       size   filename\n"
			"--------------------------------\n");

		if(recursive_dir(fd, dir) < 0) {
			printf("\nError: an error occurred during the directory scanning\n");
			goto quit;
		}

		// fflush(fd);
		off = ftell(fd);
		printf("- files info offset: %08x\n", off);

		ps = pnp;
		while(ps) {
			fdwinum(fd, ps->pn.pns,  32);
			fdwinum(fd, ps->pn.off,  32);
			fdwinum(fd, ps->pn.size, 32);
			FWRITE(ps->pn.name, ps->pn.pns - 12, fd);

			psfree = ps;
			ps     = ps->next;
			free(psfree->pn.name);
			free(psfree);
			filez++;
		}
		fdwinum(fd, off,  32);

	} else {

		printf("- open file: %s\n", fname);
		fd = fopen(fname, "rb");
		if(!fd) std_err();

		if(dir) {
			printf("- change directory: %s\n", dir);
			if(chdir(dir) < 0) std_err();
		}

		if(!fread(pack, 4, 1, fd)) goto quit;
		if(memcmp(pack, "PACK", 4)) goto quit;

		packver = fdrinum(fd, 32);

		if(fseek(fd, -4, SEEK_END) < 0) std_err();

		off = fdrinum(fd, 32);
		if(fseek(fd, off, SEEK_SET) < 0) std_err();

		for(;;) {
			pn.pns  = fdrinum(fd, 32);
			pn.off  = fdrinum(fd, 32);
			pn.size = fdrinum(fd, 32);

			len     = pn.pns - 12;
			if(len <= 0) break;
			pn.name = malloc(len + 1);
			if(!pn.name) std_err();

			if(!fread(pn.name, len, 1, fd)) break;

			if(!patt || (patt && stristr(pn.name, patt))) {
				printf("  %s\n", pn.name);
				if(!list) get_file(fd, pn.name, pn.off, pn.size);
				filez++;
			}

			free(pn.name);
			off += pn.pns;
			if(fseek(fd, off, SEEK_SET) < 0) std_err();
		}
	}

quit:
	fclose(fd);
	printf("- finished: %d files\n", filez);
	return(0);
}



int put_file(FILE *fd, char *fname) {
	struct  pnp_t   *ps;
	FILE    *fdi;
	int     len;
	u_int   off,
			size;
	u_char  buff[8192];
	char	*p;

	// printf("  %s\r", fname);
	fdi = fopen(fname, "rb");
	if(!fdi) std_err();

	if(fname[0] == '.') fname += 2;     /* skip .\ */

	for(p = fname; *p; p++) {           // WIN mode
		if(*p == '/') {
			*p = '\\';
		} else {
			*p = toupper(*p);
		}
	}

	// fflush(fd);
	off = ftell(fd);

	for(size = 0; (len = fread(buff, 1, sizeof(buff), fdi)); size += len) {
		FWRITE(buff, len, fd);
	}

	fclose(fdi);

	for(ps = pnp; ps && ps->next; ps = ps->next);

	if(ps) {
		ps->next = malloc(sizeof(struct pnp_t));
		if(!ps->next) std_err();
		ps       = ps->next;
	} else {
		pnp      = malloc(sizeof(struct pnp_t));
		if(!pnp) std_err();
		ps       = pnp;
	}
	ps->next = NULL;

	len = strlen(fname) + 1;
	ps->pn.pns  = 12 + len;
	ps->pn.off  = off;
	ps->pn.size = size;
	ps->pn.name = malloc(len);
	if(!ps->pn.name) std_err();
	memcpy(ps->pn.name, fname, len);

	printf("  %08x %10u   %s\n", off, size, fname);
	return(0);
}



void get_file(FILE *fd, char *fname, u_int off, u_int size) {
	FILE    *fdo;
	int     rlen,
			len;
	u_char  buff[8192];
	char	*p;

	if(fseek(fd, off, SEEK_SET) < 0) std_err();

	for(p = fname; *p; p++) {
		if((*p == '\\') || (*p == '/')) {
			*p = 0;
			MKDIR(fname);
			*p = PATHSLASH;
		} else {
			*p = tolower(*p);
		}
	}

	fdo = fopen(fname, "wb");
	if(!fdo) std_err();

	for(rlen = sizeof(buff); size; size -= len) {
		if(size < rlen) rlen = size;
		len = fread(buff, 1, rlen, fd);
		if(!len) break;
		FWRITE(buff, len, fdo);
	}

	fclose(fdo);
}



u_int fdrinum(FILE *fd, int size) {
	u_int   num;
	int     i;
	u_char  tmp[sizeof(u_int)];

	size >>= 3;
	fread(tmp, size, 1, fd);
	for(num = i = 0; i < size; i++) {
		num |= (tmp[i] << (i << 3));
	}
	return(num);
}



void fdwinum(FILE *fd, u_int num, int size) {
	int     i;
	u_char  tmp[sizeof(u_int)];

	size >>= 3;
	for(i = 0; i < size; i++) {
		tmp[i] = (num >> (i << 3)) & 0xff;
	}
	FWRITE(tmp, size, fd);
}

int recursive_dir(FILE *fd, char *filedir) {
	char  tcDir[FNAMEMAX];
	
	struct  stat    xstat;
	struct  dirent  **namelist;
	int             n,
					i;

	n = scandir(filedir, &namelist, NULL, alphasort);
	if(n < 0) {
		if(stat(filedir, &xstat) < 0) {
			printf("**** %s", filedir);
			std_err();
		}
		if(put_file(fd, filedir) < 0) return(-1);
	} else {
		for(i = 0; i < n; i++) {    // Changed by Plombo
			if(!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;
			
			sprintf(
				tcDir ,
				"%s/%s",
				filedir,
				namelist[i]->d_name);

			if(stat(tcDir, &xstat) < 0) {
				printf("**** %s", tcDir);
				std_err();
			}
			if(S_ISDIR(xstat.st_mode)) {
				if(recursive_dir(fd, tcDir) < 0) goto quit;
			} else {
				if(put_file(fd, tcDir) < 0) goto quit;
			}
			free(namelist[i]);
		}
		free(namelist);
	}

	return(0);
quit:
	for(; i < n; i++) {
		free(namelist[i]);
	}
	free(namelist);
	return(-1);
}



void write_err(void) {
	printf("\nError: write error; probably out of disk space\n\n");
	exit(1);
}



void std_err(void) {
	perror("\nError");
	exit(1);
}
