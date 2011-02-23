#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <conio.h>
#include <direct.h>
#include <dirent.h>


typedef struct{
    int index;
    char filename[128];
}s_pakfiles;

char packfile[128] = {"menu.pak"};
s_pakfiles pakfiles[500];

int findmods(void){
	int i = 0;
	int j = 0;
	int k = 0;
	DIR *dp = NULL;
	struct dirent *ds;
	char command[256] = {""};
	char rename[256] = {""};

	dp = opendir(".");
	while((ds = readdir(dp)) != NULL){
		if((strcmp(ds->d_name, ".") != 0) && (strcmp(ds->d_name, "..") != 0)){
			if((stricmp(packfile, ds->d_name) != 0) && 
                (strstr(ds->d_name, ".pak") || strstr(ds->d_name, ".PAK"))){    
				for(j = 0; j < strlen(ds->d_name); j++){
                     if((ds->d_name[j] >= 0x30 && ds->d_name[j] <= 0x39) ||
                        (ds->d_name[j] >= 0x41 && ds->d_name[j] <= 0x5A) ||
                        (ds->d_name[j] >= 0x61 && ds->d_name[j] <= 0x7A) ||
                         ds->d_name[j] == 0x2E){
                          pakfiles[i].filename[j] = ds->d_name[j];
                     }
                     else pakfiles[i].filename[j] = '_';
                }
                strncpy(command, "", 256);
				if(strlen(ds->d_name) > 31){
					strncpy(rename, "", 256);
                    strncpy(rename, ds->d_name, 31);
                    rename[30] = 'k';
                    rename[29] = 'a';
                    rename[28] = 'p';
                    rename[27] = '.';
					sprintf(command, "ren \"%s\" \"%s\"", ds->d_name, rename);
					strncpy(pakfiles[i].filename, rename, 128);
				}
				else sprintf(command, "ren \"%s\" \"%s\"", ds->d_name, pakfiles[i].filename);
                system(command);
				printf("%i.   %s\n", i, pakfiles[i].filename);
				strncat(pakfiles[i].filename, "\n", strlen(pakfiles[i].filename));
				i++;
			}
		}
	}
	closedir(dp);
	return i;
}

int main(void){

	int i = 0;
	int mods = 0;
	FILE* handle = NULL;
	

	printf("PackListCreator v0.4\n\n");

	printf("This app was designed to make it easier for DreamCast\n"
		   "users to update paklist.txt file within menu.pak\n\n");
		   
    printf("Requirements:\n\n");
    
    printf("1.  PACKER.EXE   - ");
    if((handle=fopen("packer.exe", "r")) != NULL){
         fclose(handle);
         printf("Found!\n");
    }
    else{
         printf("Not Found!!!\n");
         goto END;
    }
	
    printf("2.  PAXPLODE.EXE - ");
    if((handle=fopen("packer.exe", "r")) != NULL){
         fclose(handle);
         printf("Found!\n");
    }
    else{
         printf("Not Found!!!\n");
         goto END;
    }
    
    printf("3.  All Mods (Pak Files) To Be Listed.\n");
    printf("4.  ALL Files Must Be In Same Folder!\n\n");

	printf("*** Press Any Key to Start ***\n\n");
	getch();

    printf("Searching for Mods and Renaming.........\n\n");
	if((mods = findmods())){
		printf("\nTotal Mods Found: %d\n\n",mods);
	}
	else{
		printf("No Mods Found! Press Any Key to Exit.\n");
		goto END;
	}

	printf("Searching menu.pak.....   ");
	if((handle=fopen(packfile, "r")) != NULL){
		printf("Found!\n");
		fclose(handle);
	}
	else{
		printf("NOT Found!!!\n");
		goto END;
	}

	printf("Unpacking menu.pak.....\n");
	system("paxplode.exe menu.pak");


	printf("\n*** Press Any Key to Update 'menu.pak' ***\n\n");
	getch();

    printf("Writing paklist.txt....   ");
    if((handle=fopen("menu/paklist.txt", "wt")) != NULL){
        for(i=0; i<mods; i++){
            if(fwrite(pakfiles[i].filename, 1, strlen(pakfiles[i].filename), handle) <= 0){
                 printf("Error Writing To paklist.txt!!!\n\n");
                 fclose(handle);
                 break;
            }
        }
        printf("Success!\n\n");
        fclose(handle);
    }
    else printf("Error Opening paklist.txt!!!\n\n");
                
	system("del menu.pak");
	system("packer.exe menu.pak menu");
	system("del /Q .\\menu\\*.*");
	system("rmdir menu");
	system("move menu.pak menu.pak");

END:
	printf("\nPakListCreator v0.04 by SX\n\n");
	printf("Press Any Key To Exit");
	getch();
	return 0;
}
