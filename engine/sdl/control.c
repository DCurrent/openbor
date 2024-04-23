/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

// Generic control stuff (keyboard+joystick)

#include "video.h"
#include "globals.h"
#include "control.h"
#include "stristr.h"
#include "sblaster.h"
#include "joysticks.h"
#include "openbor.h"

#define T_AXIS 7000

#ifdef ANDROID
#include "jniutils.h"
#endif

SDL_Joystick *joystick[JOY_LIST_TOTAL];         // SDL struct for joysticks
SDL_Haptic *joystick_haptic[JOY_LIST_TOTAL];   // SDL haptic for joysticks
static int usejoy;						        // To be or Not to be used?
static int numjoy;						        // Number of Joy(s) found
static int lastkey;						        // Last keyboard key Pressed
static int lastjoy;                             // Last joystick button/axis/hat input

int sdl_game_started  = 0;

extern int default_keys[MAX_BTN_NUM];
extern s_playercontrols default_control;

#ifdef ANDROID
extern int nativeWidth;
extern int nativeHeight;
static TouchStatus touch_info;
#endif

/*
Here is where we aquiring all joystick events
and map them to BOR's layout.  Currently support
up to 4 controllers.
*/
void getPads(Uint8* keystate, Uint8* keystate_def)
{
	int i, j, x, axis;
	SDL_Event ev;
	while(SDL_PollEvent(&ev))
	{
		switch(ev.type)
		{
			case SDL_KEYDOWN:
				lastkey = ev.key.keysym.scancode;
				if((keystate[SDL_SCANCODE_LALT] || keystate[SDL_SCANCODE_RALT]) && (lastkey == SDL_SCANCODE_RETURN))
				{
					video_fullscreen_flip();
					keystate[SDL_SCANCODE_RETURN] = 0;
				}
				if(lastkey != SDL_SCANCODE_F10) break;
#ifdef ANDROID
			case SDL_FINGERDOWN:
			{
				for(i=0; i<MAX_POINTERS; i++)
				{
					if(touch_info.pstatus[i] == TOUCH_STATUS_UP)
					{
						touch_info.pid[i] = ev.tfinger.fingerId;
						touch_info.px[i] = ev.tfinger.x*nativeWidth;
						touch_info.py[i] = ev.tfinger.y*nativeHeight;
						touch_info.pstatus[i] = TOUCH_STATUS_DOWN;

            // migration for White Dragon's vibration logic from SDLActivity.java
            if (is_touchpad_vibration_enabled() &&
                is_touch_area(touch_info.px[i], touch_info.py[i]))
            {
              jniutils_vibrate_device();
            }

						break;
					}
				}
				control_update_android_touch(&touch_info, MAX_POINTERS, keystate, keystate_def);
			}
				break;
			case SDL_FINGERUP:
			{
				for(i=0; i<MAX_POINTERS; i++)
				{
					if(touch_info.pid[i] == ev.tfinger.fingerId)
					{
						touch_info.pstatus[i] = TOUCH_STATUS_UP;
						break;
					}
				}
				control_update_android_touch(&touch_info, MAX_POINTERS, keystate, keystate_def);
			}
				break;
			case SDL_FINGERMOTION:
			{
				for(i=0; i<MAX_POINTERS; i++)
				{
					if(touch_info.pid[i] == ev.tfinger.fingerId)
					{
						touch_info.px[i] = ev.tfinger.x*nativeWidth;
						touch_info.py[i] = ev.tfinger.y*nativeHeight;
						touch_info.pstatus[i] = TOUCH_STATUS_DOWN;
						break;
					}
				}
				control_update_android_touch(&touch_info, MAX_POINTERS, keystate, keystate_def);
			}
				break;
#endif
			case SDL_QUIT:
				borShutdown(0, DEFAULT_SHUTDOWN_MESSAGE);
				break;

			case SDL_JOYBUTTONUP:
				for(i=0; i<JOY_LIST_TOTAL; i++)
				{
					if(ev.jbutton.which == i)
					{
						if(joysticks[i].Type == JOY_TYPE_GAMEPARK)
						{
							if(ev.jbutton.button == 0 || ev.jbutton.button == 7 || ev.jbutton.button == 1) joysticks[i].Hats &= ~(JoystickBits[1]);
							if(ev.jbutton.button == 6 || ev.jbutton.button == 5 || ev.jbutton.button == 7) joysticks[i].Hats &= ~(JoystickBits[2]);
							if(ev.jbutton.button == 4 || ev.jbutton.button == 3 || ev.jbutton.button == 5) joysticks[i].Hats &= ~(JoystickBits[3]);
							if(ev.jbutton.button == 2 || ev.jbutton.button == 1 || ev.jbutton.button == 3) joysticks[i].Hats &= ~(JoystickBits[4]);
							if(ev.jbutton.button >= 8 && ev.jbutton.button <= 18) joysticks[i].Buttons &= ~(JoystickBits[ev.jbutton.button - 3]);
						}
						/*else
                        {
                            // add key flag from event
                            #ifdef ANDROID
                            joysticks[i].Buttons &= 0x00 << ev.jbutton.button;
                            #endif
                        }*/
					}
				}
				break;

			case SDL_JOYBUTTONDOWN:
				for(i=0; i<JOY_LIST_TOTAL; i++)
				{
					if (SDL_JoystickInstanceID(joystick[i]) == ev.jbutton.which)
					{
						//printf("Button down: controller %i, button %i\n", i, ev.jbutton.button);
						lastjoy = 1 + i * JOY_MAX_INPUTS + ev.jbutton.button;

						// add key flag from event
						/*#ifdef ANDROID
						joysticks[i].Buttons |= 0x01 << ev.jbutton.button;
						#endif*/
					}
				}
				break;

			case SDL_JOYHATMOTION:
				for(i=0; i<JOY_LIST_TOTAL; i++)
				{
					if (SDL_JoystickInstanceID(joystick[i]) == ev.jhat.which)
					{
						int hatfirst = 1 + i * JOY_MAX_INPUTS + joysticks[i].NumButtons + 2*joysticks[i].NumAxes + 4*ev.jhat.hat;
						x = (joysticks[i].Hats >> (4*ev.jhat.hat)) & 0x0F; // hat's previous state
						if(ev.jhat.value & SDL_HAT_UP       && !(x & SDL_HAT_UP))       lastjoy = hatfirst;
						if(ev.jhat.value & SDL_HAT_RIGHT    && !(x & SDL_HAT_RIGHT))	lastjoy = hatfirst + 1;
						if(ev.jhat.value & SDL_HAT_DOWN     && !(x & SDL_HAT_DOWN))	    lastjoy = hatfirst + 2;
						if(ev.jhat.value & SDL_HAT_LEFT     && !(x & SDL_HAT_LEFT))	    lastjoy = hatfirst + 3;
						//if(lastjoy) fprintf(stderr, "SDL_JOYHATMOTION - Joystick %i Hat %i (Index %i)\n", i, ev.jhat.hat, lastjoy);

						// add key flag from event (0x01 0x02 0x04 0x08)
						#ifdef ANDROID
						if(ev.jhat.value & SDL_HAT_UP)      joysticks[i].Hats |= SDL_HAT_UP         << (ev.jhat.hat*4);
						if(ev.jhat.value & SDL_HAT_RIGHT)   joysticks[i].Hats |= SDL_HAT_RIGHT      << (ev.jhat.hat*4);
						if(ev.jhat.value & SDL_HAT_DOWN)    joysticks[i].Hats |= SDL_HAT_DOWN       << (ev.jhat.hat*4);
						if(ev.jhat.value & SDL_HAT_LEFT)    joysticks[i].Hats |= SDL_HAT_LEFT       << (ev.jhat.hat*4);
						#endif
					}
				}
				break;

			case SDL_JOYAXISMOTION:
				for(i=0; i<JOY_LIST_TOTAL; i++)
				{
					if (SDL_JoystickInstanceID(joystick[i]) == ev.jaxis.which)
					{
						int axisfirst = 1 + i * JOY_MAX_INPUTS + joysticks[i].NumButtons + 2*ev.jaxis.axis;
						x = (joysticks[i].Axes >> (2*ev.jaxis.axis)) & 0x03; // previous state of axis
						if(ev.jaxis.value <  -1*T_AXIS && !(x & 0x01))		lastjoy = axisfirst;
						if(ev.jaxis.value >     T_AXIS && !(x & 0x02))		lastjoy = axisfirst + 1;
						//if(lastjoy) fprintf(stderr, "SDL_JOYAXISMOTION - Joystick %i Axis %i = Position %i (Index %i)\n", i, ev.jaxis.axis, ev.jaxis.value, lastjoy);

						// add key flag from event
						#ifdef ANDROID
                        if(ev.jaxis.value < -1*T_AXIS)  { joysticks[i].Axes |= 0x01 << (ev.jaxis.axis*2); }
                        if(ev.jaxis.value >    T_AXIS)  { joysticks[i].Axes |= 0x02 << (ev.jaxis.axis*2); }
                        #endif
					}
				}
				break;

            // PLUG AND PLAY
            case SDL_JOYDEVICEADDED:
                if (ev.jdevice.which < JOY_LIST_TOTAL)
                {
                    int i = ev.jdevice.which;
                    char buffer[MAX_BUFFER_LEN];
                    char joy_name[MAX_BUFFER_LEN];
                    open_joystick(i);
                    //get_time_string(buffer, MAX_BUFFER_LEN, (time_t)ev.jdevice.timestamp, TIMESTAMP_PATTERN);
                    get_now_string(buffer, MAX_BUFFER_LEN, TIMESTAMP_PATTERN);
                    numjoy = SDL_NumJoysticks();
                    strcpy(joy_name,get_joystick_name(joysticks[i].Name));
                    printf("Joystick: \"%s\" connected at port: %d at %s\n",joy_name,i,buffer);
                }
                break;

            case SDL_JOYDEVICEREMOVED:
                if (ev.jdevice.which < JOY_LIST_TOTAL)
                {
                    int i = ev.jdevice.which;
                    if(joystick[i])
                    {
                        char buffer[MAX_BUFFER_LEN];
                        char joy_name[MAX_BUFFER_LEN];
                        get_now_string(buffer, MAX_BUFFER_LEN, TIMESTAMP_PATTERN);
                        //close_joystick(i); //Kratus (20-04-21) disable the entire code to maintain joystick IDs
                        numjoy = SDL_NumJoysticks();
                        strcpy(joy_name,get_joystick_name(joysticks[i].Name));
                        printf("Joystick: \"%s\" disconnected from port: %d at %s\n",joy_name,i,buffer);
                    }
                }
                break;

            default:
                break;
		}

	}

	if(joysticks[0].Type != JOY_TYPE_GAMEPARK)
	{
		// new PC joystick code - forget about SDL joystick events, just do a state check
		SDL_JoystickUpdate();
		for(i = 0; i < JOY_LIST_TOTAL; i++)
		{
			// reset state
			joysticks[i].Axes = joysticks[i].Hats = joysticks[i].Buttons = 0;
			if (joystick[i] == NULL) continue;

			// check buttons
			for(j = 0; j < joysticks[i].NumButtons; j++)
            {
                joysticks[i].Buttons |= SDL_JoystickGetButton(joystick[i], j) << j;
            }

			// check axes
			for(j = 0; j < joysticks[i].NumAxes; j++)
			{
				axis = SDL_JoystickGetAxis(joystick[i], j);
				if(axis < -1*T_AXIS)  { joysticks[i].Axes |= 0x01 << (j*2); }
				if(axis >    T_AXIS)  { joysticks[i].Axes |= 0x02 << (j*2); }
			}

			// check hats
			for(j = 0; j < joysticks[i].NumHats; j++)
            {
                //joysticks[i].Hats |= SDL_JoystickGetHat(joystick[i], j) << (j*4);

                Uint8 hat_value = SDL_JoystickGetHat(joystick[i], j);
                if(hat_value & SDL_HAT_UP)      joysticks[i].Hats |= SDL_HAT_UP     << (j*4);
                if(hat_value & SDL_HAT_RIGHT)   joysticks[i].Hats |= SDL_HAT_RIGHT  << (j*4);
                if(hat_value & SDL_HAT_DOWN)    joysticks[i].Hats |= SDL_HAT_DOWN   << (j*4);
                if(hat_value & SDL_HAT_LEFT)    joysticks[i].Hats |= SDL_HAT_LEFT   << (j*4);
            }

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
static int flag_to_index(u64 flag)
{
	int index = 0;
	u64 bit = 1;
	while(!((bit<<index)&flag) && index<JOY_MAX_INPUTS-1) ++index;
	return index;
}

char* get_joystick_name(const char* name)
{
    char lname[strlen(name) + 1];

    if (strlen(name) <= 0) return JOY_UNKNOWN_NAME;
    strcpy(lname,name);
    for(int i = 0; lname[i]; i++)
    {
        lname[i] = tolower(lname[i]);
    }
    if ( strstr(lname, "null") == NULL ) return JOY_UNKNOWN_NAME;
    return ( (char*)name );
}

/*
Search for usb joysticks. Set
types, defaults and keynames.
*/
void joystick_scan(int scan)
{
	//Kratus (20-04-21) new joystick scan, avoid accelerometer detection as buttons in Android
	int i;
	int numjoyNoAcc = 0;

	if(!scan) return;

	numjoy = SDL_NumJoysticks();
	numjoyNoAcc = numjoy;

	if (scan != 2)
    {
        for(i = 0; i < numjoy; i++)
        {
            char real_joy_name[MAX_BUFFER_LEN];

            strcpy(real_joy_name,SDL_JoystickNameForIndex(i));
            if (strcmp(real_joy_name, "Android Accelerometer") == 0)
            {
                --numjoyNoAcc;
            }
        }
        if(numjoyNoAcc <= 0)
        {
            printf("No Joystick(s) Found!\n");
            return;
        }
        else
        {
            printf("\n%d joystick(s) found!\n", numjoyNoAcc);
        }
    }

	if (numjoyNoAcc > JOY_LIST_TOTAL) numjoy = JOY_LIST_TOTAL; // avoid overflow bug

	for(i = 0; i < numjoy; i++)
	{
	    int joy_idx = i;
        char real_joy_name[MAX_BUFFER_LEN];

        strcpy(real_joy_name,SDL_JoystickNameForIndex(i));

        if (strcmp(real_joy_name, "Android Accelerometer") == 0)
        {
            continue;
        }

        open_joystick(joy_idx);

        if(scan != 2)
        {
            int is_rumble_support = (joystick_haptic[i] != NULL && SDL_HapticRumbleSupported(joystick_haptic[i])) ? 1 : 0;
            char* rumble_support = (is_rumble_support) ? "yes" : "no";

            // print JOY_MAX_INPUTS (64) spaces for alignment
            if(numjoy == 1)
            {
                printf("%s (%s) - %d axes, %d buttons, %d hat(s), rumble support: %s\n",
                        get_joystick_name(joysticks[joy_idx].Name), SDL_JoystickName(i), joysticks[joy_idx].NumAxes, joysticks[joy_idx].NumButtons, joysticks[joy_idx].NumHats, rumble_support);
            }
            else if(numjoy > 1)
            {
                if(joy_idx) printf("\n");
                printf("%d. %s (%s) - %d axes, %d buttons, %d hat(s), rumble support: %s\n", i + 1,
                        get_joystick_name(joysticks[joy_idx].Name), SDL_JoystickName(i), joysticks[joy_idx].NumAxes, joysticks[joy_idx].NumButtons, joysticks[joy_idx].NumHats, rumble_support);
            }
        }
	}
}

/*
Open a single joystick
*/
void open_joystick(int i)
{
    int j;

    if ( ( joystick[i] = SDL_JoystickOpen(i) ) == NULL )
    {
       printf("\nWarning: Unable to initialize joystick in port: %d! SDL Error: %s\n", i, SDL_GetError());
       return;
    }
    joysticks[i].NumHats = SDL_JoystickNumHats(joystick[i]);
    joysticks[i].NumAxes = SDL_JoystickNumAxes(joystick[i]);
    joysticks[i].NumButtons = SDL_JoystickNumButtons(joystick[i]);

    strcpy(joysticks[i].Name, SDL_JoystickName(i));

    joystick_haptic[i] = SDL_HapticOpenFromJoystick(joystick[i]);
    if (joystick_haptic[i] != NULL)
    {
        //Get initialize rumble
        if( SDL_HapticRumbleInit( joystick_haptic[i] ) < 0 )
        {
            printf("\nWarning: Unable to initialize rumble for joystick: %s in port: %d! SDL Error: %s\n", joysticks[i].Name, i, SDL_GetError());
        }
    }

    for(j = 1; j < JOY_MAX_INPUTS + 1; j++)
    {
        strcpy(joysticks[i].KeyName[j], PC_GetJoystickKeyName(i, j));
    }

    return;
}

void reset_joystick_map(int i)
{
	memset(joysticks[i].Name,0,sizeof(joysticks[i].Name));
	memset(joysticks[i].KeyName,0,sizeof(joysticks[i].KeyName));
	joysticks[i].Type = 0;
	joysticks[i].NumHats = 0;
	joysticks[i].NumAxes = 0;
	joysticks[i].NumButtons = 0;
	joysticks[i].Hats = 0;
	joysticks[i].Axes = 0;
	joysticks[i].Buttons = 0;
	joysticks[i].Data = 0;
    set_default_joystick_keynames(i);
}

/*
Reset All data back to Zero and
destroy all SDL Joystick data.
*/
void control_exit()
{
	int i;
	for(i = 0; i < numjoy; i++)
    {
		close_joystick(i);
	}
	usejoy = 0;
}

/*
Reset single joystick
*/
void close_joystick(int i)
{
	if(joystick[i] != NULL) SDL_JoystickClose(joystick[i]);
	if(joystick_haptic[i] != NULL) SDL_HapticClose(joystick_haptic[i]);
	joystick[i] = NULL;
	joystick_haptic[i] = NULL;
	reset_joystick_map(i);
}

/*
Create default values for joysticks if enabled.
Then scan for joysticks and update their data.
*/
void control_init(int joy_enable)
{
	int i;

	usejoy = joy_enable;

	//memset(joysticks, 0, sizeof(s_joysticks) * JOY_LIST_TOTAL);
	for(i = 0; i < JOY_LIST_TOTAL; i++)
	{
        joystick[i] = NULL;
        joystick_haptic[i] = NULL;
		reset_joystick_map(i);
	}
	joystick_scan(usejoy);

#ifdef ANDROID
	for(i = 0; i < MAX_POINTERS; i++)
    {
        touch_info.pstatus[i] = TOUCH_STATUS_UP;
    }
#endif
}

void set_default_joystick_keynames(int i)
{
	int j;
	for(j = 0; j < JOY_MAX_INPUTS + 1; j++)
	{
		// Kratus (20-04-21) rename all keys when disconnected
		// Kratus (10-2021) Added constants and automatic translation for some Joystick status
		if(j) strcpy(joysticks[i].KeyName[j], JOY_DISCONNECTED_NAME);
	}
}

char *control_getkeyname(unsigned int keycode)
{
	int i;
	for(i = 0; i < JOY_LIST_TOTAL; i++)
	{
		if((keycode >= (JOY_LIST_FIRST + 1 + (i * JOY_MAX_INPUTS))) && (keycode <= JOY_LIST_FIRST + JOY_MAX_INPUTS + (i * JOY_MAX_INPUTS)))
			return (char*)joysticks[i].KeyName[keycode - (JOY_LIST_FIRST + (i * JOY_MAX_INPUTS))];
	}

	if(keycode > SDLK_FIRST && keycode < SDLK_LAST)
		return JOY_GetKeyName(keycode);
	else
	if(keycode == CONTROL_NONE) // Kratus (20-04-21) value used to clear all keys
		return JOY_NONE_NAME; // Kratus (10-2021) Added constants and automatic translation for some Joystick status
	else
		return "...";
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

#if ANDROID
/*
Get if touchscreen vibration is active
*/
int is_touchpad_vibration_enabled()
{
	return savedata.is_touchpad_vibration_enabled;
}
#endif

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

#if ANDROID
/*
Android touch logic, the rest of the code is in android/jni/video.c,
maybe they'll be merged someday.
*/
extern float bx[MAXTOUCHB];
extern float by[MAXTOUCHB];
extern float br[MAXTOUCHB];
extern unsigned touchstates[MAXTOUCHB];
int hide_t = 5000;
void control_update_android_touch(TouchStatus *touch_info, int maxp, Uint8* keystate, Uint8* keystate_def)
{
	int i, j;
	float tx, ty, tr;
	float r[MAXTOUCHB];
	float dirx, diry, circlea, circleb, tan;

	memset(touchstates, 0, sizeof(touchstates));

	for(j=0; j<MAXTOUCHB; j++)
	{
		r[j] = br[j]*br[j]*(1.5*1.5);
	}
	dirx = (bx[SDID_MOVERIGHT]+bx[SDID_MOVELEFT])/2.0;
	diry = (by[SDID_MOVEUP]+by[SDID_MOVEDOWN])/2.0;
	circlea = bx[SDID_MOVERIGHT]-dirx-br[SDID_MOVEUP];
	circleb = bx[SDID_MOVERIGHT]-dirx+br[SDID_MOVEUP]*1.5;
	circlea *= circlea;
	circleb *= circleb;
	#define tana 0.577350f
	#define tanb 1.732051f
	for (i=0; i<maxp; i++)
	{
		if(touch_info->pstatus[i] == TOUCH_STATUS_UP) continue;
		tx = touch_info->px[i]-dirx;
		ty = touch_info->py[i]-diry;
		tr = tx*tx + ty*ty;
		//direction button logic is different, check a ring instead of individual buttons
		if(tr>circlea && tr<=circleb)
		{
			if(tx<0)
			{
				tan = ty/tx;
				if(tan>=-tana && tan<=tana)
                {
					touchstates[SDID_MOVELEFT] = 1;
				}
				else if(tan<-tanb)
                {
					touchstates[SDID_MOVEDOWN] = 1;
				}
				else if(tan>tanb)
				{
					touchstates[SDID_MOVEUP] = 1;
				}
				else if(ty<0)
				{
					touchstates[SDID_MOVEUP] = touchstates[SDID_MOVELEFT] = 1;
				}
				else
				{
					touchstates[SDID_MOVELEFT] = touchstates[SDID_MOVEDOWN] = 1;
                }
			}
			else if(tx>0)
			{
				tan = ty/tx;
				if(tan>=-tana && tan<=tana)
                {
					touchstates[SDID_MOVERIGHT] = 1;
				}
				else if(tan<-tanb)
				{
					touchstates[SDID_MOVEUP] = 1;
				}
				else if(tan>tanb)
                {
					touchstates[SDID_MOVEDOWN] = 1;
				}
				else if(ty<0)
                {
					touchstates[SDID_MOVEUP] = touchstates[SDID_MOVERIGHT] = 1;
				}
				else
                {
					touchstates[SDID_MOVERIGHT] = touchstates[SDID_MOVEDOWN] = 1;
                }
			}
			else
			{
				if(ty>0)
				{
                    touchstates[SDID_MOVEDOWN] = 1;
				}
				else
				{
				    touchstates[SDID_MOVEUP] = 1;
                }
			}
		}
		//rest buttons
		for(j=0; j<MAXTOUCHB; j++)
		{
			if(j==SDID_MOVERIGHT || j==SDID_MOVEUP ||
				j==SDID_MOVELEFT || j==SDID_MOVEDOWN)
				continue;
			tx = touch_info->px[i]-bx[j];
			ty = touch_info->py[i]-by[j];
			tr = tx*tx + ty*ty;
			if(tr<=r[j])
            {
                touchstates[j] = 1;
            }
		}
	}
	#undef tana
	#undef tanb

	hide_t = timer_gettick() + 5000;

	//map to current user settings
	extern s_savedata savedata;
	#define pc(x) savedata.keys[0][x]
	keystate[pc(SDID_MOVEUP)] = touchstates[SDID_MOVEUP];
	keystate[pc(SDID_MOVEDOWN)] = touchstates[SDID_MOVEDOWN];
	keystate[pc(SDID_MOVELEFT)] = touchstates[SDID_MOVELEFT];
	keystate[pc(SDID_MOVERIGHT)] = touchstates[SDID_MOVERIGHT];
	keystate[pc(SDID_ATTACK)] = touchstates[SDID_ATTACK];
	keystate[pc(SDID_ATTACK2)] = touchstates[SDID_ATTACK2];
	keystate[pc(SDID_ATTACK3)] = touchstates[SDID_ATTACK3];
	keystate[pc(SDID_ATTACK4)] = touchstates[SDID_ATTACK4];
	keystate[pc(SDID_JUMP)] = touchstates[SDID_JUMP];
	keystate[pc(SDID_SPECIAL)] = touchstates[SDID_SPECIAL];
	keystate[pc(SDID_START)] = touchstates[SDID_START];
	keystate[pc(SDID_SCREENSHOT)] = touchstates[SDID_SCREENSHOT];
	#undef pc

	//use default value for touch key mapping
    keystate_def[default_keys[SDID_MOVEUP]]    = touchstates[SDID_MOVEUP];
    keystate_def[default_keys[SDID_MOVEDOWN]]  = touchstates[SDID_MOVEDOWN];
    keystate_def[default_keys[SDID_MOVELEFT]]  = touchstates[SDID_MOVELEFT];
    keystate_def[default_keys[SDID_MOVERIGHT]] = touchstates[SDID_MOVERIGHT];
    keystate_def[default_keys[SDID_ATTACK]]    = touchstates[SDID_ATTACK];
    keystate_def[default_keys[SDID_ATTACK2]]   = touchstates[SDID_ATTACK2];
    keystate_def[default_keys[SDID_ATTACK3]]   = touchstates[SDID_ATTACK3];
    keystate_def[default_keys[SDID_ATTACK4]]   = touchstates[SDID_ATTACK4];
    keystate_def[default_keys[SDID_JUMP]]      = touchstates[SDID_JUMP];
    keystate_def[default_keys[SDID_SPECIAL]]   = touchstates[SDID_SPECIAL];
    keystate_def[default_keys[SDID_START]]     = touchstates[SDID_START];
    keystate_def[default_keys[SDID_SCREENSHOT]] = touchstates[SDID_SCREENSHOT];

    keystate[CONTROL_ESC] = keystate_def[CONTROL_ESC] = touchstates[SDID_ESC];

    return;
}

int is_touch_area(float x, float y)
{
	int j;
	float tx, ty, tr;
	float r[MAXTOUCHB];
	float dirx, diry, circlea, circleb, tan;

	for(j=0; j<MAXTOUCHB; j++)
	{
		r[j] = br[j]*br[j]*(1.5*1.5);
	}
	dirx = (bx[SDID_MOVERIGHT]+bx[SDID_MOVELEFT])/2.0;
	diry = (by[SDID_MOVEUP]+by[SDID_MOVEDOWN])/2.0;
	circlea = bx[SDID_MOVERIGHT]-dirx-br[SDID_MOVEUP];
	circleb = bx[SDID_MOVERIGHT]-dirx+br[SDID_MOVEUP]*1.5;
	circlea *= circlea;
	circleb *= circleb;
	#define tana 0.577350f
	#define tanb 1.732051f
    tx = x-dirx;
    ty = y-diry;
    tr = tx*tx + ty*ty;
    //direction button logic is different, check a ring instead of individual buttons
    if(tr>circlea && tr<=circleb)
    {
        if(tx<0)
        {
            tan = ty/tx;
            if(tan>=-tana && tan<=tana)
            {
                return 1;
            }
            else if(tan<-tanb)
            {
                return 1;
            }
            else if(tan>tanb)
            {
                return 1;
            }
            else if(ty<0)
            {
                return 1;
            }
            else
            {
                return 1;
            }
        }
        else if(tx>0)
        {
            tan = ty/tx;
            if(tan>=-tana && tan<=tana)
            {
                return 1;
            }
            else if(tan<-tanb)
            {
                return 1;
            }
            else if(tan>tanb)
            {
                return 1;
            }
            else if(ty<0)
            {
                return 1;
            }
            else
            {
                return 1;
            }
        }
        else
        {
            if(ty>0)
            {
                return 1;
            }
            else
            {
                return 1;
            }
        }
    }
    //rest buttons
    for(j=0; j<MAXTOUCHB; j++)
    {
        if(j==SDID_MOVERIGHT || j==SDID_MOVEUP ||
            j==SDID_MOVELEFT || j==SDID_MOVEDOWN)
            continue;
        tx = x-bx[j];
        ty = y-by[j];
        tr = tx*tx + ty*ty;
        if(tr<=r[j])
        {
            return 1;
        }
    }
	#undef tana
	#undef tanb

	return 0;
}
#endif

int keyboard_getlastkey()
{
		int i, ret = lastkey;
		lastkey = 0;
		for(i = 0; i < JOY_LIST_TOTAL; i++) joysticks[i].Buttons = 0;
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

void control_update(s_playercontrols ** playercontrols, int numplayers)
{
	u64 k;
	unsigned i;
	int player;
	int t;
	s_playercontrols * pcontrols;
	Uint8* keystate = (Uint8*)SDL_GetKeyState(NULL); // Here retrieve keyboard state
	Uint8* keystate_def = (Uint8*)SDL_GetKeyState(NULL); // Here retrieve keyboard state for default

	getPads(keystate,keystate_def);

	for(player = 0; player < numplayers; player++){

		pcontrols = playercontrols[player];

		k = 0;

		for(i = 0; i < JOY_MAX_INPUTS; i++)
		{
			t = pcontrols->settings[i];
			if(t >= SDLK_FIRST && t < SDLK_LAST){
                if(keystate[t]) k |= (1<<i);
			}
		}

        //White Dragon: Set input from default keys overriding previous keys
        //Default keys are available just if no configured keys are pressed!
        if (player <= 0 && !k)
        {
            for(i = 0; i < JOY_MAX_INPUTS; i++)
            {
                t = default_control.settings[i];
                if(t >= SDLK_FIRST && t < SDLK_LAST){
                    if(keystate_def[t]) k |= (1<<i);
                }
            }
        }

		if(usejoy)
		{
			for(i = 0; i < JOY_MAX_INPUTS; i++)
			{
				t = pcontrols->settings[i];
				if(t >= JOY_LIST_FIRST && t <= JOY_LIST_LAST)
				{
					int portnum = (t-JOY_LIST_FIRST-1) / JOY_MAX_INPUTS;
					int shiftby = (t-JOY_LIST_FIRST-1) % JOY_MAX_INPUTS;
					if(portnum >= 0 && portnum <= JOY_LIST_TOTAL-1)
					{
						if((joysticks[portnum].Data >> shiftby) & 1) k |= (1<<i);
					}
				}
			}
		}
		pcontrols->kb_break = 0;
		pcontrols->newkeyflags = k & (~pcontrols->keyflags);
		pcontrols->keyflags = k;

		//if (player <= 0) debug_printf("hats: %d, axes: %d, data: %d",joysticks[0].Hats,joysticks[0].Axes,joysticks[0].Data);
	}
}

void control_rumble(int port, int ratio, int msec)
{
    #if SDL
    if (joystick[port] != NULL && joystick_haptic[port] != NULL) {
        if(SDL_HapticRumblePlay(joystick_haptic[port], ratio, msec) != 0)
        {
            //printf( "Warning: Unable to play rumble! %s\n", SDL_GetError() );
        }
    }
    #endif
}

