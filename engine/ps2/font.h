#ifndef	FONT_H
#define	FONT_H


void font_unload(int which);
int font_load(int which, char *filename, char *packfile);
void font_printf(int x, int y, int which, char *format, ...);
void screen_printf(s_screen * screen, int x, int y, int which, char *format, ...);

#endif
