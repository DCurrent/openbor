// Generic control stuff (keyboard+joystick)

#include "ps2port.h"

#include <libpad.h>

#include "control.h"

#define KEY_PAD_START 1
#define	KEY_PAD_END   33

static const char *padnames[34] = {
	"...",
	"P1 L2",
	"P1 R2",
	"P1 L1",
	"P1 R1",
	"P1 Triangle",
	"P1 O",
	"P1 X",
	"P1 Square",
	"P1 Select",
	"P1 L3",
	"P1 R3",
	"P1 Start",
	"P1 Up",
	"P1 Right",
        "P1 Down",
        "P1 Left",
        "P2 L2",
        "P2 R2",
        "P2 L1",
        "P2 R1",
        "P2 Triangle",
        "P2 O",
        "P2 X",
        "P2 Square",
        "P2 Select",
        "P2 L3",
        "P2 R3",
        "P2 Start",
        "P2 Up",
        "P2 Right",
        "P2 Down",
        "P2 Left",
	"undefined"
};

static unsigned short pad_button_read(int port, int slot) {
  struct padButtonStatus rdata;
  if(padRead(port, slot, &rdata) == 0) return 0; // read failed
  if(rdata.ok == 0) return 0xFFFF ^ rdata.btns;
  return 0;
}

static unsigned getpadbits(void) {
  unsigned pad0bits = pad_button_read(0, 0);
  unsigned pad1bits = pad_button_read(1, 0);
  return pad0bits | (pad1bits << 16);
}

static u128 pad0_dma_buf[16] __attribute__((aligned (64)));
static u128 pad1_dma_buf[16] __attribute__((aligned (64)));

void control_ps2init(void) {
//  int i;
//  char t[256];

  debug_printf("control_ps2init()\n");
  
  if(padInit(0) != 0) {
    debug_printf("padInit failed; halting\n");
    SleepThread();
  }
  if(padPortOpen(0, 0, pad0_dma_buf) == 0) {
    debug_printf("padPortOpen(0) failed; halting\n");
    SleepThread();
  }
  if(padPortOpen(1, 0, pad1_dma_buf) == 0) {
    debug_printf("padPortOpen(1) failed; halting\n");
    SleepThread();
  }

//  for(i = 0;; i++) { debug_printf("padbits = %08X (%d)\n", getpadbits(), i); }
}


static int flag_to_index(unsigned flag){
	int index = 0;
	unsigned bit = 1;

	while(!((bit<<index)&flag) && index<31) ++index;
	return index;
}

void control_exit(){ }

void control_init(int joy_enable){
    debug_printf("control_init()\n");
}


int control_usejoy(int enable){ return 0; }
int control_getjoyenabled(){ return 0; }


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
  static unsigned ready = 0;
  unsigned k=0;

  k = getpadbits();
  if(k) k = 1 + flag_to_index(k);

  if(ready && k) {
    ready = 0;
    return k;
  }
  ready = (!k);
  return 0;
}


char * control_getkeyname(unsigned keycode){

	if(keycode >= KEY_PAD_START && keycode <= KEY_PAD_END){
		return (char*)padnames[keycode];
	}
	return "...";
}


void control_update(s_playercontrols ** playercontrols, int numplayers){

	unsigned gamelib_long k;
	unsigned gamelib_long i;
	int player;
	int t;
	s_playercontrols * pcontrols;
	unsigned padbits = getpadbits();
	for(player=0; player<numplayers; player++){

		pcontrols = playercontrols[player];

		k = 0;

		for(i=0; i<32; i++){
			t = pcontrols->settings[i];
			if(t >= KEY_PAD_START && t <= KEY_PAD_END){
				int shiftby = t - 1;
				if(shiftby >= 0 && shiftby <= 31) {
					if((padbits >> (t - 1)) & 1) k |= (1<<i);
				}
			}
		}
		pcontrols->kb_break = 0; //kb_break;

		pcontrols->newkeyflags = k & (~pcontrols->keyflags);
		pcontrols->keyflags = k;
	}
}

