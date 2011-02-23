#ifndef		VGA_H
#define		VGA_H


#define		VGARAM	 	((char*)0xA0000)
#define		TEXTMODE	0x03
#define		MODE_13H	0x13
#define		XPAGE0	 	((char*)0xA0000)
#define		XPAGE1	 	((char*)0xA4B00)
#define		XPAGE2	 	((char*)0xA9600)


// Set video mode
extern void __near vga_setmode(int);
#pragma aux vga_setmode parm[eax] modify[eax ecx];


// Wait for new vertical retrace (interrupts stay ON)
extern void __near vga_vwait(void);
#pragma aux vga_vwait modify[al edx];


// Set VGA-type palette
extern void __near vga_setpalette(char*);
#pragma aux vga_setpalette parm[esi] modify[esi al dx ecx];


// Set VGA border colour index
extern void __near vga_setborderindex(int);
#pragma aux vga_setborderindex parm[ecx] modify[al dx];


// Set mode X (320x240x256 planar VGA)
extern void __near vga_setmodex(void);
#pragma aux vga_setmodex modify[eax ecx edx edi];


// Clear video RAM in mode X
extern void __near vga_clearmodex(void);
#pragma aux vga_clearmodex modify[eax ecx edx edi];


// Activate one of the 3 video pages in mode X.
// Returns a pointer to the active video page (the active video RAM)
// or NULL if the page does not exist.
extern char * __near vga_xpageflip(int);
#pragma aux vga_xpageflip parm[ecx] value[ebx] modify[eax ebx ecx edx];


// Set widescreen mode (320x204x256 linear VGA)
extern void __near vga_setwidemode(void);
#pragma aux vga_setwidemode modify[eax ecx edx];


// Set mode T (288x216x256 VGA)
extern void __near vga_setmodet(void);
#pragma aux vga_setmodet modify[eax ecx edx];


#endif


