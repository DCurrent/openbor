#ifndef	CONTROL_H
#define	CONTROL_H

// Generic control stuff (keyboard+joystick).


#define	CONTROL_ESC		33

#define	CONTROL_DEFAULT1_UP	 13 
#define	CONTROL_DEFAULT1_DOWN	 15
#define	CONTROL_DEFAULT1_LEFT	 16
#define	CONTROL_DEFAULT1_RIGHT	 14
#define	CONTROL_DEFAULT1_SPECIAL 5
#define CONTROL_DEFAULT1_ATTACK  8
#define CONTROL_DEFAULT1_JUMP    7
#define CONTROL_DEFAULT1_START   12
#define CONTROL_DEFAULT1_SCREENSHOT 33

#define CONTROL_DEFAULT2_UP      (13+16)
#define CONTROL_DEFAULT2_DOWN    (15+16)
#define CONTROL_DEFAULT2_LEFT    (16+16)
#define CONTROL_DEFAULT2_RIGHT   (14+16)
#define CONTROL_DEFAULT2_SPECIAL (5+16)
#define CONTROL_DEFAULT2_ATTACK  (8+16)
#define CONTROL_DEFAULT2_JUMP    (7+16)
#define CONTROL_DEFAULT2_START   (12+16)
#define CONTROL_DEFAULT2_SCREENSHOT 33


typedef struct{
	int		settings[32];
	unsigned gamelib_long	keyflags, newkeyflags;
	int		kb_break;
}s_playercontrols;

void control_ps2init(void);

void control_exit();
void control_init(int joy_enable);
int control_usejoy(int enable);
int control_getjoyenabled();

void control_setkey(s_playercontrols * pcontrols, unsigned int flag, int key);
int control_scankey();

char * control_getkeyname(unsigned int keycode);
void control_update(s_playercontrols ** playercontrols, int numplayers);


#endif

