// Generic control stuff (keyboard+joystick)

#include <stdio.h>		// kbhit, getch
#include <conio.h>		// kbhit, getch
#include "keyboard.h"
#include "joy.h"
#include "control.h"





#define		KEYBOARD_START		1
#define		KEYBOARD_END		255
#define		JOYSTICK_START		256
#define		JOYSTICK_END		265







// Key names
static char *keynames[128] = {
	"...",
	"Escape",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"0",
	"-",
	"=",
	"Backspace",
	"Tab",
	"Q",
	"W",
	"E",
	"R",
	"T",
	"Y",
	"U",
	"I",
	"O",
	"P",
	"[",
	"]",
	"Enter",
	"CTRL",
	"A",
	"S",
	"D",
	"F",
	"G",
	"H",
	"J",
	"K",
	"L",
	";",
	"'",
	"~",
	"Left shift",
	"\\",
	"Z",
	"X",
	"C",
	"V",
	"B",
	"N",
	"M",
	",",
	".",
	"/",
	"Right shift",
	"*",
	"ALT",
	"Space",
	"Capslock",
	"F1",
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7",
	"F8",
	"F9",
	"F10",
	"Num lock",
	"Scroll lock",
	"Home",
	"Uparrow",
	"PgUp",
	"-",
	"Leftarrow",
	"5",
	"Rightarrow",
	"+",
	"End",
	"Downarrow",
	"PgDn",
	"Ins",
	"Del",
	"<key 84>",
	"<key 85>",
	"<key 86>",
	"F11",
	"F12",
	"<key 89>",
	"<key 90>",
	"Logo left",
	"Logo right",
	"Popup",
	"<key 94>",
	"<key 95>",
	"<key 96>",
	"<key 97>",
	"<key 98>",
	"<key 99>",
	"<key 100>",
	"<key 101>",
	"<key 102>",
	"<key 103>",
	"<key 104>",
	"<key 105>",
	"<key 106>",
	"<key 107>",
	"<key 108>",
	"<key 109>",
	"<key 110>",
	"<key 111>",
	"<key 112>",
	"<key 113>",
	"<key 114>",
	"<key 115>",
	"<key 116>",
	"<key 117>",
	"<key 118>",
	"<key 119>",
	"<key 120>",
	"<key 121>",
	"<key 122>",
	"<key 123>",
	"<key 124>",
	"<key 125>",
	"<key 126>",
	"<key 127>"
};

static char *joynames[10] = {
	"Joy Fire 1",
	"Joy Fire 2",
	"Joy Fire 3",
	"Joy Fire 4",
	"Joy Up",
	"Joy Down",
	"Joy Left",
	"Joy Right",
	"Joy Fire 5",
	"Joy Fire 6"
};




static int usejoy;





static int flag_to_index(unsigned long flag){
	int index = 0;
	unsigned long bit = 1;

	while(!((bit<<index)&flag) && index<31) ++index;
	return index;
}










void control_exit(){
	keyboard_exit();
	joy_exit();
	usejoy = 0;
	while(kbhit()) getch();
}



void control_init(int joy_enable){
	control_exit();
	keyboard_init();
	keyboard_enable_extended(1);
	if(joy_enable) usejoy = joy_init();
}



int control_usejoy(int enable){
	usejoy = enable;
	if(usejoy) return joy_init();
	joy_exit();
	return 0;
}


int control_getjoyenabled(){
	return usejoy;
}




void control_setkey(s_playercontrols * pcontrols, unsigned int flag, int key){
	if(!pcontrols) return;
	pcontrols->settings[flag_to_index(flag)] = key;
	pcontrols->keyflags = pcontrols->newkeyflags = 0;
}



// Scan input for newly-pressed keys.
// Return value:
// 0  = no key was pressed
// >0 = key code for pressed key
// <0 = error
int control_scankey(){
	static int ready = 0;
	int k=0, b=0;
	int index;

	k = keyboard_getlastkey();
	if(usejoy) b = joy_getstate();

	if(ready && (k|b)){
		ready = 0;
		if(k) return k;
		if(b & JOY_1) return CONTROL_DEFAULT2_FIRE1;
		if(b & JOY_2) return CONTROL_DEFAULT2_FIRE2;
		if(b & JOY_3) return CONTROL_DEFAULT2_FIRE3;
		if(b & JOY_4) return CONTROL_DEFAULT2_FIRE4;
		if(b & JOY_5) return CONTROL_DEFAULT2_FIRE5;
		if(b & JOY_6) return CONTROL_DEFAULT2_FIRE6;
		if(b & JOY_UP) return CONTROL_DEFAULT2_UP;
		if(b & JOY_DOWN) return CONTROL_DEFAULT2_DOWN;
		if(b & JOY_LEFT) return CONTROL_DEFAULT2_LEFT;
		if(b & JOY_RIGHT) return CONTROL_DEFAULT2_RIGHT;
		return -1;
	}
	ready = (!(k|b));
	return 0;
}





char * control_getkeyname(unsigned int keycode){

	if(keycode >= KEYBOARD_START && keycode <= KEYBOARD_END){
		return keynames[keycode % 128];
	}
	if(keycode >= JOYSTICK_START && keycode <= JOYSTICK_END){
		return joynames[(keycode - JOYSTICK_START) % 10];
	}
	return "...";
}







void control_update(s_playercontrols ** playercontrols, int numplayers){

	unsigned long k;
	unsigned long jf;
	unsigned long i;
	int player;
	int t;
	s_playercontrols * pcontrols;
	int kb_break = keyboard_checkbreak();
	if(usejoy) jf = joy_getstate();
	if(jf == 0xFFFFFFFF) usejoy = 0;


	for(player=0; player<numplayers; player++){

		pcontrols = playercontrols[player];

		k = 0;

		for(i=0; i<32; i++){
			t = pcontrols->settings[i];
			if(t >= KEYBOARD_START && t <= KEYBOARD_END){
				if(keytable[t]) k |= (1<<i);
			}
		}
		pcontrols->kb_break = kb_break;


		if(usejoy){
			for(i=0; i<32; i++){
				t = pcontrols->settings[i];
				switch(t){
					case CONTROL_DEFAULT2_FIRE1:
						if(jf & JOY_1) k |= (1<<i);
						break;
					case CONTROL_DEFAULT2_FIRE2:
						if(jf & JOY_2) k |= (1<<i);
						break;
					case CONTROL_DEFAULT2_FIRE3:
						if(jf & JOY_3) k |= (1<<i);
						break;
					case CONTROL_DEFAULT2_FIRE4:
						if(jf & JOY_4) k |= (1<<i);
						break;
					case CONTROL_DEFAULT2_UP:
						if(jf & JOY_UP) k |= (1<<i);
						break;
					case CONTROL_DEFAULT2_DOWN:
						if(jf & JOY_DOWN) k |= (1<<i);
						break;
					case CONTROL_DEFAULT2_LEFT:
						if(jf & JOY_LEFT) k |= (1<<i);
						break;
					case CONTROL_DEFAULT2_RIGHT:
						if(jf & JOY_RIGHT) k |= (1<<i);
						break;
					case CONTROL_DEFAULT2_FIRE5:
						if(jf & JOY_5) k |= (1<<i);
						break;
					case CONTROL_DEFAULT2_FIRE6:
						if(jf & JOY_6) k |= (1<<i);
						break;
				}
			}
		}

		pcontrols->newkeyflags = k & (~pcontrols->keyflags);
		pcontrols->keyflags = k;
	}
}













