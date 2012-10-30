/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

// Generic control stuff (keyboard+joystick)
#include "video.h"
#include "globals.h"
#include "control.h"
#include "stristr.h"
#include "sblaster.h"
#include "joysticks.h"
#include "openbor.h"


SDL_Joystick *joystick[JOY_LIST_TOTAL]; // SDL struct for joysticks
static int usejoy;						// To be or Not to be used?
static int numjoy;						// Number of Joy(s) found
static int lastkey;						// Last keyboard key Pressed
static int lastjoy;                     // Last joystick button/axis/hat input

/*
Here is where we aquiring all joystick events
and map them to BOR's layout.  Currently support
up to 4 controllers.
*/
void getPads(Uint8* keystate)
{
	int i, j, x, axis;
	SDL_Event ev;
	while(SDL_PollEvent(&ev))
	{
		switch(ev.type)
		{
			case SDL_KEYDOWN:
				lastkey = ev.key.keysym.sym;
				//if (lastkey==SDLK_F11) video_fullscreen_flip();
				if((keystate[SDLK_LALT] || keystate[SDLK_RALT]) && (lastkey == SDLK_RETURN))
				{
					video_fullscreen_flip();
					keystate[SDLK_RETURN] = 0;
				}
				if(lastkey != SDLK_F10) break;

			case SDL_QUIT:
				shutdown(0, DEFAULT_SHUTDOWN_MESSAGE);
				break;

			case SDL_JOYBUTTONUP:
				for(i=0; i<JOY_LIST_TOTAL; i++)
				{
					if(ev.jbutton.which == i)
					{
						if(joysticks[i].Type == JOY_TYPE_SONY)
						{
							if(ev.jbutton.button <= 3 && (joysticks[i].Buttons & JoystickBits[ev.jbutton.button + 5]))
								joysticks[i].Buttons &= ~(JoystickBits[ev.jbutton.button + 5]);
							else if(ev.jbutton.button >= 4 && ev.jbutton.button <= 7)
								joysticks[i].Hats &= ~(JoystickBits[ev.jbutton.button - 3]);
							else if(ev.jbutton.button >= 8 && ev.jbutton.button <= 16 && (joysticks[i].Buttons & JoystickBits[ev.jbutton.button + 1]))
								joysticks[i].Buttons &= ~(JoystickBits[ev.jbutton.button + 1]);
						}
						else if(joysticks[i].Type == JOY_TYPE_GAMEPARK)
						{
							if(ev.jbutton.button == 0 || ev.jbutton.button == 7 || ev.jbutton.button == 1) joysticks[i].Hats &= ~(JoystickBits[1]);
							if(ev.jbutton.button == 6 || ev.jbutton.button == 5 || ev.jbutton.button == 7) joysticks[i].Hats &= ~(JoystickBits[2]);
							if(ev.jbutton.button == 4 || ev.jbutton.button == 3 || ev.jbutton.button == 5) joysticks[i].Hats &= ~(JoystickBits[3]);
							if(ev.jbutton.button == 2 || ev.jbutton.button == 1 || ev.jbutton.button == 3) joysticks[i].Hats &= ~(JoystickBits[4]);
							if(ev.jbutton.button >= 8 && ev.jbutton.button <= 18) joysticks[i].Buttons &= ~(JoystickBits[ev.jbutton.button - 3]);
						}
					}
				}
				break;

			case SDL_JOYBUTTONDOWN:
				// FIXME: restore PSP/GP2X controls
				for(i=0; i<JOY_LIST_TOTAL; i++)
				{
					if(ev.jbutton.which == i)
					{
						lastjoy = 1 + i * JOY_MAX_INPUTS + ev.jbutton.button;
					}
				}
				break;

			case SDL_JOYHATMOTION:
				for(i=0; i<JOY_LIST_TOTAL; i++)
				{
					if(ev.jhat.which == i)
					{
						int hatfirst = 1 + i * JOY_MAX_INPUTS + joysticks[i].NumButtons + 2*joysticks[i].NumAxes + 4*ev.jhat.hat;
						x = (joysticks[i].Hats >> (4*ev.jhat.hat)) & 15; // hat's previous state
						if(ev.jhat.value & SDL_HAT_UP && !(x & SDL_HAT_UP))			lastjoy = hatfirst;
						if(ev.jhat.value & SDL_HAT_RIGHT && !(x & SDL_HAT_RIGHT))	lastjoy = hatfirst + 1;
						if(ev.jhat.value & SDL_HAT_DOWN && !(x & SDL_HAT_DOWN))		lastjoy = hatfirst + 2;
						if(ev.jhat.value & SDL_HAT_LEFT && !(x & SDL_HAT_LEFT))		lastjoy = hatfirst + 3;
						//if(lastjoy) fprintf(stderr, "SDL_JOYHATMOTION - Joystick %i Hat %i (Index %i)\n", i, ev.jhat.hat, lastjoy);
					}
				}
				break;

			case SDL_JOYAXISMOTION:
				for(i=0; i<JOY_LIST_TOTAL; i++)
				{
					if(ev.jaxis.which == i && joysticks[i].Type != JOY_TYPE_SONY)
					{
						int axisfirst = 1 + i * JOY_MAX_INPUTS + joysticks[i].NumButtons + 2*ev.jaxis.axis;
						x = (joysticks[i].Axes >> (2*ev.jaxis.axis)) & 3; // previous state of axis
						if(ev.jaxis.value < -7000 && !(x & JoystickBits[0]))		lastjoy = axisfirst;
						if(ev.jaxis.value > +7000 && !(x & JoystickBits[1]))		lastjoy = axisfirst + 1;
						//if(lastjoy) fprintf(stderr, "SDL_JOYAXISMOTION - Joystick %i Axis %i = Position %i (Index %i)\n", i, ev.jaxis.axis, ev.jaxis.value, lastjoy);
					}
				}
				break;
		}

	}

	if((joysticks[0].Type != JOY_TYPE_SONY) && (joysticks[0].Type != JOY_TYPE_GAMEPARK))
	{
		// new PC joystick code - forget about SDL joystick events, just do a state check
		SDL_JoystickUpdate();
		for(i=0; i<JOY_LIST_TOTAL; i++)
		{
			// reset state
			joysticks[i].Axes = joysticks[i].Hats = joysticks[i].Buttons = 0;

			// check buttons
			for(j=0; j<joysticks[i].NumButtons; j++)
				joysticks[i].Buttons |= SDL_JoystickGetButton(joystick[i], j) << j;

			// check axes
			for(j=0; j<joysticks[i].NumAxes; j++)
			{
				axis = SDL_JoystickGetAxis(joystick[i], j);
				if(axis < -7000)  { joysticks[i].Axes |= 1 << (j*2); }
				if(axis > +7000)  { joysticks[i].Axes |= 2 << (j*2); }
			}

			// check hats
			for(j=0; j<joysticks[i].NumHats; j++)
				joysticks[i].Hats |= SDL_JoystickGetHat(joystick[i], j) << (j*4);

			// combine axis, hat, and button state into a single value
			joysticks[i].Data = joysticks[i].Buttons;
			joysticks[i].Data |= joysticks[i].Axes << joysticks[i].NumButtons;
			joysticks[i].Data |= joysticks[i].Hats << (joysticks[i].NumButtons + 2*joysticks[i].NumAxes);
		}
	}
}


/*
Convert binary masked data to indexes
*/
static int flag_to_index(u32 flag)
{
	int index = 0;
	u32 bit = 1;
	while(!((bit<<index)&flag) && index<31) ++index;
	return index;
}


/*
Search for usb joysticks. Set
types, defaults and keynames.
*/
void joystick_scan(int scan)
{
	int i, j, k;
	if(!scan) return;
	numjoy = SDL_NumJoysticks();
	if(!numjoy && scan != 2)
	{
		printf("No Joystick(s) Found!\n");
		return;
	}
	for(i=0, k=0; i<numjoy; i++, k+=JOY_MAX_INPUTS)
	{
		joystick[i] = SDL_JoystickOpen(i);
		joysticks[i].NumHats = SDL_JoystickNumHats(joystick[i]);
		joysticks[i].NumAxes = SDL_JoystickNumAxes(joystick[i]);
		joysticks[i].NumButtons = SDL_JoystickNumButtons(joystick[i]);
		joysticks[i].Name = SDL_JoystickName(i);
#if PSP
		joysticks[i].Type = JOY_TYPE_SONY;
		for(j=0; j<JOY_MAX_INPUTS+1; j++)
		{
			if(j) joysticks[i].KeyName[j] = SonyKeyName[j + k];
			else joysticks[i].KeyName[j] = SonyKeyName[j];
		}
#elif XBOX
		joysticks[i].Type = JOY_TYPE_MICROSOFT;
		for(j=0; j<JOY_MAX_INPUTS+1; j++)
		{
			if(j) joysticks[i].KeyName[j] = MicrosoftKeyName[j + k];
			else joysticks[i].KeyName[j] = MicrosoftKeyName[j];
		}
#elif GP2X
		joysticks[i].Type = JOY_TYPE_GAMEPARK;
		for(j=0; j<JOY_MAX_INPUTS+1; j++)
		{
			if(j) joysticks[i].KeyName[j] = GameparkKeyName[j + k];
			else joysticks[i].KeyName[j] = GameparkKeyName[j];
		}
#else
		//SDL_JoystickEventState(SDL_IGNORE); // disable joystick events
		for(j=1; j<JOY_MAX_INPUTS+1; j++)
		{
			joysticks[i].KeyName[j] = PC_GetJoystickKeyName(i, j);
		}
#endif
		if(scan != 2)
		{
			if(numjoy == 1) printf("%s - %d axes, %d buttons, %d hat(s)\n",
									joysticks[i].Name, joysticks[i].NumAxes, joysticks[i].NumButtons, joysticks[i].NumHats);
			else if(numjoy > 1)
			{
				if(i) printf("                                "); // print 32 spaces for alignment
				printf("%d. %s - %d axes, %d buttons, %d hat(s)\n", i + 1,
						joysticks[i].Name, joysticks[i].NumAxes, joysticks[i].NumButtons, joysticks[i].NumHats);
			}
		}
	}
}


/*
Reset All data back to Zero and
destroy all SDL Joystick data.
*/
void control_exit()
{
	int i;
	usejoy = 0;
	for(i=0; i<numjoy; i++) SDL_JoystickClose(joystick[i]);
	memset(joysticks, 0, sizeof(s_joysticks) * JOY_LIST_TOTAL);
}


/*
Create default values for joysticks if enabled.
Then scan for joysticks and update their data.
*/
void control_init(int joy_enable)
{
	int i, j, k;
#ifdef GP2X
	usejoy = joy_enable ? joy_enable : 1;
#else
	usejoy = joy_enable;
#endif
	memset(joysticks, 0, sizeof(s_joysticks) * JOY_LIST_TOTAL);
	for(i=0, k=0; i<JOY_LIST_TOTAL; i++, k+=JOY_MAX_INPUTS)
	{
		for(j=0; j<JOY_MAX_INPUTS+1; j++)
		{
			if(j) joysticks[i].KeyName[j] = JoystickKeyName[j + k];
			else joysticks[i].KeyName[j] = JoystickKeyName[j];
		}
	}
	joystick_scan(usejoy);
}


/*
Set global variable, which is used for
enabling and disabling all joysticks.
*/
int control_usejoy(int enable)
{
	usejoy = enable;
	return 0;
}


/*
Only used in openbor.c to get current status
of joystick usage.
*/
int control_getjoyenabled()
{
	return usejoy;
}


void control_setkey(s_playercontrols * pcontrols, unsigned int flag, int key)
{
	if(!pcontrols) return;
	pcontrols->settings[flag_to_index(flag)] = key;
	pcontrols->keyflags = pcontrols->newkeyflags = 0;
}


int keyboard_getlastkey()
{
		int i, ret = lastkey;
		lastkey = 0;
		for(i=0; i<JOY_LIST_TOTAL; i++) joysticks[i].Buttons = 0;
		return ret;
}


// Scan input for newly-pressed keys.
// Return value:
// 0  = no key was pressed
// >0 = key code for pressed key
// <0 = error
int control_scankey()
{
	static unsigned ready = 0;
	unsigned k = 0, j = 0;

	k = keyboard_getlastkey();
	j = lastjoy;
	lastjoy = 0;

#if 0
		 if(joysticks[0].Data) j = 1 + 0 * JOY_MAX_INPUTS + flag_to_index(joysticks[0].Data);
	else if(joysticks[1].Data) j = 1 + 1 * JOY_MAX_INPUTS + flag_to_index(joysticks[1].Data);
	else if(joysticks[2].Data) j = 1 + 2 * JOY_MAX_INPUTS + flag_to_index(joysticks[2].Data);
	else if(joysticks[3].Data) j = 1 + 3 * JOY_MAX_INPUTS + flag_to_index(joysticks[3].Data);
#endif

	if(ready && (k || j))
	{
		ready = 0;
		if(k) return k;
		if(j) return JOY_LIST_FIRST+j;
		else return -1;
	}
	ready = (!k || !j);
	return 0;
}


char *control_getkeyname(unsigned int keycode)
{
	int i;
	for(i=0; i<JOY_LIST_TOTAL; i++)
	{
		if((keycode >= (JOY_LIST_FIRST + 1 + (i * JOY_MAX_INPUTS))) && (keycode <= JOY_LIST_FIRST + JOY_MAX_INPUTS + (i * JOY_MAX_INPUTS)))
			return (char*)joysticks[i].KeyName[keycode - (JOY_LIST_FIRST + (i * JOY_MAX_INPUTS))];
	}
	if(keycode > SDLK_FIRST && keycode < SDLK_LAST)
		return JOY_GetKeyName(keycode);
	else
		return "...";
}


void control_update(s_playercontrols ** playercontrols, int numplayers)
{
	unsigned k;
	unsigned i;
	int player;
	int t;
	s_playercontrols * pcontrols;
	Uint8* keystate;

	keystate = SDL_GetKeyState(NULL);

	getPads(keystate);
	for(player=0; player<numplayers; player++){

		pcontrols = playercontrols[player];

		k = 0;

		for(i=0;i<32;i++)
		{
			t = pcontrols->settings[i];
			if(t >= SDLK_FIRST && t < SDLK_LAST){
				if(keystate[t]) k |= (1<<i);
			}
		}

		if(usejoy)
		{
			for(i=0; i<32; i++)
			{
				t = pcontrols->settings[i];
				if(t >= JOY_LIST_FIRST && t <= JOY_LIST_LAST)
				{
					int portnum = (t-JOY_LIST_FIRST-1) / JOY_MAX_INPUTS;
					int shiftby = (t-JOY_LIST_FIRST-1) % JOY_MAX_INPUTS;
					if(portnum >= 0 && portnum <= 3)
					{
						if((joysticks[portnum].Data >> shiftby) & 1) k |= (1<<i);
					}
				}
			}
		}
		pcontrols->kb_break = 0;
		pcontrols->newkeyflags = k & (~pcontrols->keyflags);
		pcontrols->keyflags = k;
	}
}

void control_rumble(int port, int msec)
{
}

