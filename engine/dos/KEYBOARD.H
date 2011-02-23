#ifndef	KEYBOARD_H
#define	KEYBOARD_H

// Keyboard code for games. Does everything... Except for printscreen.
// Last update: 08-21-2000


#define KB_ESC		0x01
#define KB_ENTER	0x1C
#define KB_CTRL		0x1D
#define KB_ALT		0x38
#define KB_LSHIFT	0x2A
#define KB_RSHIFT	0x36
#define KB_SPACE	0x39
#define KB_F1		0x3B
#define KB_F2		0x3C
#define KB_F3		0x3D
#define KB_F4		0x3E
#define KB_F5		0x3F
#define KB_F6		0x40
#define KB_F7		0x41
#define KB_F8		0x42
#define KB_F9		0x43
#define KB_F10		0x44
#define KB_F11		87
#define KB_F12		88
#define KB_UP		0x48
#define KB_DOWN		0x50
#define KB_LEFT		0x4B
#define KB_RIGHT	0x4D
#define KB_PLUS		0x4E
#define KB_MINUS	0x4A
#define KB_DELETE	0x53
#define KB_TAB		0x0F
#define KB_HOME		71
#define KB_END		79
#define KB_PGUP		73
#define KB_PGDN		81
#define KB_PRINTSCREEN	55
#define KB_CAPSLOCK	58
#define KB_BACKSPACE	14
#define KB_TILDE	41
#define KB_INS		82

// Extended keys
#define KB_ENTER2	(0x1C|0x80)
#define KB_LEFT2	(0x4B|0x80)
#define KB_RIGHT2	(0x4D|0x80)
#define KB_UP2		(0x48|0x80)
#define KB_DOWN2	(0x50|0x80)
#define KB_CENTER	0x4C




// Bitfields for keyboard leds
#define		LED_SCROLL	1
#define		LED_NUM		2
#define		LED_CAPS	4


extern volatile char keytable[256];

extern void __near keyboard_init();
#pragma aux keyboard_init modify[eax edx];

extern void __near keyboard_exit();
#pragma aux keyboard_exit modify[eax edx];

extern void __near keyboard_enable_bios(int);
#pragma aux keyboard_enable_bios parm[eax];

extern void __near keyboard_enable_extended(int);
#pragma aux keyboard_enable_extended parm[eax];

extern int __near keyboard_getlastkey();
#pragma aux keyboard_getlastkey value[eax];

extern int __near keyboard_checkbreak();
#pragma aux keyboard_checkbreak value[eax];

extern void __near keyboard_setleds(char);
#pragma aux keyboard_setleds parm[ebx] modify[eax];

extern void __near keyboard_restoreleds();
#pragma aux keyboard_restoreleds modify[eax ebx];

#endif
