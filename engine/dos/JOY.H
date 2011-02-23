// Gameport code for single device.
// Last update: 08-21-2000


#ifndef JOY_H
#define JOY_H


#define		JOY_1		0x0010
#define		JOY_2		0x0020
#define		JOY_3		0x0040
#define		JOY_4		0x0080
#define		JOY_UP		0x0100
#define		JOY_DOWN	0x0200
#define		JOY_LEFT	0x0400
#define		JOY_RIGHT	0x0800
#define		JOY_5		0x1000
#define		JOY_6		0x2000



// Initializes joystick/pad code.
// Return value: estimated number of buttons (0, 4 or 6).
extern int __near joy_init(void);

// Resets joystick code.
extern void __near joy_exit(void);

// Read gameport state.
// Return value: bitflags (see definitions above)
extern unsigned int __near joy_getstate(void);


#endif



