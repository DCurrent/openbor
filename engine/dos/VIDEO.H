#ifndef VIDEO_H
#define VIDEO_H


extern s_vram current_videomode;


// Frees all VESA shit when returning to textmode
int video_set_mode(int, int, int);
int video_copy_screen(s_screen*);
void video_clearscreen();


#endif

