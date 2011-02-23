#ifndef	CONTROL_H
#define	CONTROL_H

// Generic control stuff (keyboard+joystick).

#define		CONTROL_ESC					0x01


#define		CONTROL_DEFAULT1_START		0x1C
#define		CONTROL_DEFAULT1_UP			0xC8
#define		CONTROL_DEFAULT1_DOWN		0xD0
#define		CONTROL_DEFAULT1_LEFT		0xCB
#define		CONTROL_DEFAULT1_RIGHT		0xCD
#define		CONTROL_DEFAULT1_FIRE1		0x1D		// ctrl
#define		CONTROL_DEFAULT1_FIRE2		0x38		// alt
#define		CONTROL_DEFAULT1_FIRE3		0x2A		// lshift
#define		CONTROL_DEFAULT1_FIRE4		0x36		// rshift
#define		CONTROL_DEFAULT1_FIRE5		0x39		// spacebar
#define		CONTROL_DEFAULT1_FIRE6		0x9D		// r-ctrl
#define		CONTROL_DEFAULT1_SCREENSHOT	88	// F12

#define		CONTROL_DEFAULT2_START		60	// F2
#define		CONTROL_DEFAULT2_FIRE1		256
#define		CONTROL_DEFAULT2_FIRE2		257
#define		CONTROL_DEFAULT2_FIRE3		258
#define		CONTROL_DEFAULT2_FIRE4		259
#define		CONTROL_DEFAULT2_UP			260
#define		CONTROL_DEFAULT2_DOWN		261
#define		CONTROL_DEFAULT2_LEFT		262
#define		CONTROL_DEFAULT2_RIGHT		263
#define		CONTROL_DEFAULT2_FIRE5		264
#define		CONTROL_DEFAULT2_FIRE6		265
#define		CONTROL_DEFAULT2_SCREENSHOT	88	// F12

#define		CONTROL_DEFAULT3_START		0x1C
#define		CONTROL_DEFAULT3_UP			0xC8
#define		CONTROL_DEFAULT3_DOWN		0xD0
#define		CONTROL_DEFAULT3_LEFT		0xCB
#define		CONTROL_DEFAULT3_RIGHT		0xCD
#define		CONTROL_DEFAULT3_FIRE1		0x1D		// ctrl
#define		CONTROL_DEFAULT3_FIRE2		0x38		// alt
#define		CONTROL_DEFAULT3_FIRE3		0x2A		// lshift
#define		CONTROL_DEFAULT3_FIRE4		0x36		// rshift
#define		CONTROL_DEFAULT3_FIRE5		0x39		// spacebar
#define		CONTROL_DEFAULT3_FIRE6		0x9D		// r-ctrl
#define		CONTROL_DEFAULT3_SCREENSHOT	88	// F12

#define		CONTROL_DEFAULT4_START		60	// F2
#define		CONTROL_DEFAULT4_FIRE1		256
#define		CONTROL_DEFAULT4_FIRE2		257
#define		CONTROL_DEFAULT4_FIRE3		258
#define		CONTROL_DEFAULT4_FIRE4		259
#define		CONTROL_DEFAULT4_UP			260
#define		CONTROL_DEFAULT4_DOWN		261
#define		CONTROL_DEFAULT4_LEFT		262
#define		CONTROL_DEFAULT4_RIGHT		263
#define		CONTROL_DEFAULT4_FIRE5		264
#define		CONTROL_DEFAULT4_FIRE6		265
#define		CONTROL_DEFAULT4_SCREENSHOT	88	// F12


typedef struct{
	int		settings[32];
	unsigned long	keyflags, newkeyflags;
	int		kb_break;
}s_playercontrols;





void control_exit();
void control_init(int joy_enable);
int control_usejoy(int enable);
int control_getjoyenabled();

void control_setkey(s_playercontrols * pcontrols, unsigned int flag, int key);
int control_scankey();

char * control_getkeyname(unsigned int keycode);
void control_update(s_playercontrols ** playercontrols, int numplayers);


#endif

