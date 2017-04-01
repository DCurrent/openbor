/*
http://www.linux-sxs.org/programming/kbhit.html
*/

#ifndef KBHITh
#define KBHITh

void   init_keyboard(void);
void   close_keyboard(void);
int      kbhit(void);
int     readch(void);

#endif
