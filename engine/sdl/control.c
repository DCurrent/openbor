/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2019 OpenBOR Team
 */

// Generic control stuff (keyboard+joystick)

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "globals.h"
#include "control.h"
#include "openbor.h"
#include "List.h"

#define AXIS_THRESHOLD 7000

typedef enum {
    DEVICE_TYPE_NONE,
    DEVICE_TYPE_KEYBOARD,
    DEVICE_TYPE_CONTROLLER, // XInput compatible controller
    DEVICE_TYPE_JOYSTICK,   // controller not compatible with XInput
} DeviceType;

typedef struct {
    DeviceType deviceType;
    char name[CONTROL_DEVICE_NAME_SIZE];
    int mappings[SDID_COUNT];
    SDL_Haptic *haptic;
    union {
        // no extra structure needed for keyboard
        SDL_GameController *controller;
        SDL_Joystick *joystick;
    };
} InputDevice;

static InputDevice devices[MAX_DEVICES];
static bool controlInited = false;
static int keyboardDeviceID = -1;
static bool altEnterPressed = false;

// if non-null, device is being remapped in the input settings menu
static InputDevice *remapDevice = NULL;
static int remapKeycode = -1;

// each list member is an array of SDID_COUNT ints, dynamically allocated
static List savedMappings;
static bool savedMappingsInited = false;

#ifdef ANDROID
#define MAX_POINTERS 30
typedef enum
{
    TOUCH_STATUS_UP,
    TOUCH_STATUS_DOWN
} touch_status;

typedef struct TouchStatus {
    float px[MAX_POINTERS];
    float py[MAX_POINTERS];
    SDL_FingerID pid[MAX_POINTERS];
    touch_status pstatus[MAX_POINTERS];
} TouchStatus;

extern int nativeWidth;
extern int nativeHeight;
static TouchStatus touch_info;

static void control_update_android_touch(TouchStatus *touch_info, int maxp);
#endif

// update the mappings for a device in the save data
static void update_saved_mapping(int deviceID)
{
    InputDevice *device = &devices[deviceID];
    if (device->deviceType == DEVICE_TYPE_NONE) return;

    if (List_FindByName(&savedMappings, device->name))
    {
        memcpy(List_Retrieve(&savedMappings), device->mappings, SDID_COUNT * sizeof(int));
    }
    else
    {
        int *mappings = malloc(SDID_COUNT * sizeof(int));
        memcpy(mappings, device->mappings, SDID_COUNT * sizeof(int));
        List_InsertAfter(&savedMappings, mappings, device->name);
    }
}

// set the mappings for a device to the saved settings
static void load_from_saved_mapping(int deviceID)
{
    InputDevice *device = &devices[deviceID];
    if (device->deviceType == DEVICE_TYPE_NONE) return;

    if (List_FindByName(&savedMappings, device->name))
    {
        memcpy(device->mappings, List_Retrieve(&savedMappings), SDID_COUNT * sizeof(int));
    }
    else
    {
        control_resetmappings(deviceID);
    }
}

static void clear_saved_mappings()
{
    if (!savedMappingsInited)
    {
        List_Init(&savedMappings);
        savedMappingsInited = true;
    }

    int numMappings = List_GetSize(&savedMappings);
	List_Reset(&savedMappings);
	for (int i = 0; i < numMappings; i++)
	{
	    free(List_Retrieve(&savedMappings));
        List_GotoNext(&savedMappings);
    }
    List_Clear(&savedMappings);
}

/* If 2 or more of the same type of controller are plugged in, we need to disambiguate them so that a player assigning
   devices in the options menu can tell them apart. Do this by appending "#2", "#3", etc. to the names. */
static void set_device_name(int deviceID, const char *name)
{
    char fullName[CONTROL_DEVICE_NAME_SIZE];

    for (int i = 0; i < MAX_DEVICES; i++)
    {
        bool nameTaken = false;

        if (i == 0)
        {
            snprintf(fullName, sizeof(fullName), "%s", name);
        }
        else
        {
            snprintf(fullName, sizeof(fullName), "%s #%i", name, i + 1);
        }

        for (int j = 0; j < MAX_DEVICES; j++)
        {
            if (j != deviceID && devices[j].deviceType != DEVICE_TYPE_NONE)
            {
                if (0 == strcmp(devices[j].name, fullName))
                {
                    nameTaken = true;
                    break;
                }
            }
        }

        if (!nameTaken) break;
    }

    snprintf(devices[deviceID].name, sizeof(devices[deviceID].name), "%s", fullName);
}

static void setup_joystick(int deviceID, int sdlDeviceID)
{
    if (SDL_IsGameController(sdlDeviceID))
    {
        devices[deviceID].deviceType = DEVICE_TYPE_CONTROLLER;
        devices[deviceID].controller = SDL_GameControllerOpen(sdlDeviceID);
        const char *name = SDL_GameControllerNameForIndex(sdlDeviceID);
        if (name == NULL)
        {
            name = SDL_JoystickNameForIndex(sdlDeviceID);
            if (name == NULL)
            {
                name = "Unknown Controller";
            }
        }
        //snprintf(devices[deviceID].name, sizeof(devices[deviceID].name), "%s", name);
        set_device_name(deviceID, name);
        load_from_saved_mapping(deviceID);
        printf("%s (device #%i, SDL ID %i) is a game controller.\n", devices[deviceID].name, deviceID, sdlDeviceID);
    }
    else
    {
        devices[deviceID].deviceType = DEVICE_TYPE_JOYSTICK;
        devices[deviceID].joystick = SDL_JoystickOpen(sdlDeviceID);
        const char *name = SDL_JoystickNameForIndex(sdlDeviceID);
        if (name == NULL)
        {
            name = "Unknown Controller";
        }
        //snprintf(devices[deviceID].name, sizeof(devices[deviceID].name), "%s", name);
        set_device_name(deviceID, name);
        load_from_saved_mapping(deviceID);
        printf("%s (device #%i, SDL ID %i) is not a game controller.\n", devices[deviceID].name, deviceID, sdlDeviceID);
    }

    devices[deviceID].haptic = SDL_HapticOpen(sdlDeviceID);
    if (devices[deviceID].haptic != NULL)
    {
        // initialize rumble
        if (SDL_HapticRumbleInit(devices[deviceID].haptic) < 0)
        {
            printf("Warning: Unable to initialize rumble for %s! %s\n", devices[deviceID].name, SDL_GetError());
        }
    }
}

void control_init()
{
    if (controlInited) return;

    if (!savedMappingsInited)
    {
        List_Init(&savedMappings);
        savedMappingsInited = true;
    }

    // initialize all devices to DEVICE_TYPE_NONE
    memset(devices, 0, sizeof(devices));

    int numJoysticks = SDL_NumJoysticks();

    if (numJoysticks >= MAX_DEVICES)
    {
        // subtract 1 so we have room for the keyboard later
        numJoysticks = MAX_DEVICES - 1;
    }

    int joystickCount = 0;
    for (int i = 0; i < numJoysticks; i++)
    {
        // blacklist the Android accelerometer that SDL counts as a "joystick"
        if (0 == stricmp("Android Accelerometer", SDL_JoystickNameForIndex(i)))
        {
            continue;
        }

        setup_joystick(joystickCount, i);
        joystickCount++;
    }

    keyboardDeviceID = joystickCount;
    devices[joystickCount].deviceType = DEVICE_TYPE_KEYBOARD;
#ifdef ANDROID
    snprintf(devices[joystickCount].name, sizeof(devices[joystickCount].name), "%s", "On-Screen Controller");
#else
    snprintf(devices[joystickCount].name, sizeof(devices[joystickCount].name), "%s", "Keyboard");
#endif
    load_from_saved_mapping(joystickCount);

#ifdef ANDROID
    for (int i = 0; i < MAX_POINTERS; i++)
    {
        touch_info.pstatus[i] = TOUCH_STATUS_UP;
    }
#endif

    controlInited = true;
}

void control_exit()
{
    if (!controlInited) return;

    clear_saved_mappings();

    for (int i = 0; i < MAX_DEVICES; i++)
    {
        InputDevice *device = &devices[i];
        if (device->deviceType == DEVICE_TYPE_CONTROLLER)
        {
            SDL_GameControllerClose(device->controller);
            device->controller = NULL;
        }
        else if (device->deviceType == DEVICE_TYPE_JOYSTICK)
        {
            SDL_JoystickClose(device->joystick);
            device->joystick = NULL;
        }
        if (device->haptic)
        {
            SDL_HapticClose(device->haptic);
            device->haptic = NULL;
        }
        device->deviceType = DEVICE_TYPE_NONE;
    }

    keyboardDeviceID = -1;
    remapDevice = NULL;
    remapKeycode = -1;
    controlInited = false;
}

static void set_default_keyboard_mappings(InputDevice *device)
{
    device->mappings[SDID_MOVEUP]     = SDL_SCANCODE_UP;
    device->mappings[SDID_MOVEDOWN]   = SDL_SCANCODE_DOWN;
    device->mappings[SDID_MOVELEFT]   = SDL_SCANCODE_LEFT;
    device->mappings[SDID_MOVERIGHT]  = SDL_SCANCODE_RIGHT;
    device->mappings[SDID_ATTACK]     = SDL_SCANCODE_A;
    device->mappings[SDID_ATTACK2]    = SDL_SCANCODE_S;
    device->mappings[SDID_ATTACK3]    = SDL_SCANCODE_Z;
    device->mappings[SDID_ATTACK4]    = SDL_SCANCODE_X;
    device->mappings[SDID_JUMP]       = SDL_SCANCODE_D;
    device->mappings[SDID_SPECIAL]    = SDL_SCANCODE_F;
    device->mappings[SDID_START]      = SDL_SCANCODE_RETURN;
    device->mappings[SDID_SCREENSHOT] = SDL_SCANCODE_F12;
    device->mappings[SDID_ESC]        = SDL_SCANCODE_ESCAPE;
}

static void set_default_controller_mappings(InputDevice *device)
{
    device->mappings[SDID_MOVEUP]     = SDL_CONTROLLER_BUTTON_DPAD_UP;
    device->mappings[SDID_MOVEDOWN]   = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
    device->mappings[SDID_MOVELEFT]   = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
    device->mappings[SDID_MOVERIGHT]  = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
    device->mappings[SDID_ATTACK]     = SDL_CONTROLLER_BUTTON_A;
    device->mappings[SDID_ATTACK2]    = SDL_CONTROLLER_BUTTON_X;
    device->mappings[SDID_ATTACK3]    = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
    device->mappings[SDID_ATTACK4]    = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
    device->mappings[SDID_JUMP]       = SDL_CONTROLLER_BUTTON_B;
    device->mappings[SDID_SPECIAL]    = SDL_CONTROLLER_BUTTON_Y;
    device->mappings[SDID_START]      = SDL_CONTROLLER_BUTTON_START;
    device->mappings[SDID_SCREENSHOT] = SDL_CONTROLLER_BUTTON_BACK;
    device->mappings[SDID_ESC]        = SDL_CONTROLLER_BUTTON_B;
}

static void set_default_joystick_mappings(InputDevice *device)
{
    int numButtons = SDL_JoystickNumButtons(device->joystick);

    device->mappings[SDID_MOVEUP]     = numButtons;
    device->mappings[SDID_MOVEDOWN]   = numButtons + 1;
    device->mappings[SDID_MOVELEFT]   = numButtons + 2;
    device->mappings[SDID_MOVERIGHT]  = numButtons + 3;
    device->mappings[SDID_ATTACK]     = 0;
    device->mappings[SDID_ATTACK2]    = 3;
    device->mappings[SDID_ATTACK3]    = 4;
    device->mappings[SDID_ATTACK4]    = 5;
    device->mappings[SDID_JUMP]       = 1;
    device->mappings[SDID_SPECIAL]    = 2;
    device->mappings[SDID_START]      = 6;
    device->mappings[SDID_SCREENSHOT] = 7;
    device->mappings[SDID_ESC]        = device->mappings[SDID_SPECIAL];
}

void control_resetmappings(int deviceID)
{
    if (deviceID < 0) return;

    InputDevice *device = &devices[deviceID];
    switch (device->deviceType)
    {
        case DEVICE_TYPE_KEYBOARD:
            set_default_keyboard_mappings(device);
            break;
        case DEVICE_TYPE_CONTROLLER:
            set_default_controller_mappings(device);
            break;
        case DEVICE_TYPE_JOYSTICK:
            set_default_joystick_mappings(device);
            break;
        default:
            memset(device->mappings, 0, sizeof(device->mappings));
            break;
    }
}

static void handle_events()
{
    SDL_Event ev;
    bool altEnterJustReleased = false;
    while (SDL_PollEvent(&ev))
    {
        switch (ev.type)
        {
            case SDL_QUIT:
            {
                borShutdown(0, DEFAULT_SHUTDOWN_MESSAGE);
                break;
            }

            /* There is also a CONTROLLERDEVICEADDED event, but we can ignore it since this one works for
               both kinds of controllers/joysticks. */
            case SDL_JOYDEVICEADDED:
            {
                bool already_in_use = false;

                /* We get this event for devices that were plugged in at application start, which we already
                   initialized in control_init(). So look through the device list to avoid duplicates. */
                if (SDL_IsGameController(ev.jdevice.which))
                {
                    SDL_GameController *controller = SDL_GameControllerOpen(ev.jdevice.which);
                    for (int i = 0; i < MAX_DEVICES; i++)
                    {
                        if (devices[i].deviceType == DEVICE_TYPE_CONTROLLER &&
                            devices[i].controller == controller)
                        {
                            already_in_use = true;
                            //printf("Already in use as device %i\n", i);
                            break;
                        }
                    }
                    SDL_GameControllerClose(controller);
                }
                else
                {
                    SDL_Joystick *joystick = SDL_JoystickOpen(ev.jdevice.which);
                    for (int i = 0; i < MAX_DEVICES; i++)
                    {
                        if (devices[i].deviceType == DEVICE_TYPE_JOYSTICK &&
                            devices[i].joystick == joystick)
                        {
                            already_in_use = true;
                            //printf("Already in use as device %i\n", i);
                            break;
                        }
                    }
                    SDL_JoystickClose(joystick);
                }

                if (already_in_use)
                {
                    break;
                }

                // Okay, it's actually a newly inserted device, not a duplicate. Initialize it.
                printf("Controller or joystick hotplugged: SDL device ID=%i\n", ev.jdevice.which);
                for (int i = 0; i < MAX_DEVICES; i++)
                {
                    if (devices[i].deviceType == DEVICE_TYPE_NONE)
                    {
                        setup_joystick(i, ev.jdevice.which);
                        printf("Hotplugged %s set as device #%i\n",
                               devices[i].deviceType == DEVICE_TYPE_CONTROLLER ? "controller" : "joystick",
                               i);
                        break;
                    }
                }
                break;
            }

            /* There is also a CONTROLLERDEVICEREMOVED event, but we can ignore it since this one works for
               both kinds of controllers/joysticks. */
            case SDL_JOYDEVICEREMOVED:
            {
                for (int i = 0; i < MAX_DEVICES; i++)
                {
                    if (devices[i].deviceType == DEVICE_TYPE_CONTROLLER &&
                        devices[i].controller == SDL_GameControllerFromInstanceID(ev.jdevice.which))
                    {
                        SDL_GameControllerClose(devices[i].controller);
                        devices[i].deviceType = DEVICE_TYPE_NONE;
                        printf("Controller removed: device #%i (SDL instance ID=%i)\n", i, ev.jdevice.which);
                        break;
                    }
                    else if (devices[i].deviceType == DEVICE_TYPE_JOYSTICK &&
                             devices[i].joystick == SDL_JoystickFromInstanceID(ev.jdevice.which))
                    {
                        SDL_JoystickClose(devices[i].joystick);
                        devices[i].deviceType = DEVICE_TYPE_NONE;
                        printf("Joystick removed: device #%i (SDL instance ID=%i)\n", i, ev.jdevice.which);
                    }
                }
                break;
            }

            case SDL_KEYDOWN:
            {
                if (remapDevice && remapDevice->deviceType == DEVICE_TYPE_KEYBOARD)
                {
                    remapKeycode = ev.key.keysym.scancode;
                }

                // Alt+Enter toggles fullscreen
                if (ev.key.keysym.scancode == SDL_SCANCODE_RETURN && (SDL_GetModState() & KMOD_ALT) && !altEnterPressed && !altEnterJustReleased && !ev.key.repeat)
                {
                    video_fullscreen_flip();

                    /* Force the Enter key to unpressed in the SDL keyboard state so it won't be read by the
                       is_key_pressed() function if it's mapped to something in-game. */
                    //((Uint8*)SDL_GetKeyboardState(NULL))[SDL_SCANCODE_RETURN] = 0;
                    altEnterPressed = true;
                }
                break;
            }

            case SDL_KEYUP:
            {
                if (ev.key.keysym.scancode == SDL_SCANCODE_RETURN)
                {
                    altEnterPressed = false;
                    altEnterJustReleased = true;
                }
                break;
            }

            case SDL_CONTROLLERBUTTONDOWN:
            {
                if (remapDevice &&
                    remapDevice->deviceType == DEVICE_TYPE_CONTROLLER &&
                    SDL_GameControllerFromInstanceID(ev.cbutton.which) == remapDevice->controller)
                {
                    remapKeycode = ev.cbutton.button;
                }
                break;
            }
            case SDL_CONTROLLERAXISMOTION:
            {
                if (remapDevice &&
                    remapDevice->deviceType == DEVICE_TYPE_CONTROLLER &&
                    SDL_GameControllerFromInstanceID(ev.caxis.which) == remapDevice->controller)
                {
                    if (ev.caxis.value > AXIS_THRESHOLD)
                    {
                        switch (ev.caxis.axis)
                        {
                            case SDL_CONTROLLER_AXIS_LEFTX:
                            case SDL_CONTROLLER_AXIS_LEFTY:
                            case SDL_CONTROLLER_AXIS_RIGHTX:
                            case SDL_CONTROLLER_AXIS_RIGHTY:
                                remapKeycode = SDL_CONTROLLER_BUTTON_MAX + (ev.caxis.axis * 2) + 1;
                                break;
                            case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
                                remapKeycode = SDL_CONTROLLER_BUTTON_MAX + 8;
                                break;
                            case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
                                remapKeycode = SDL_CONTROLLER_BUTTON_MAX + 9;
                                break;
                        }
                    }
                    else if (ev.caxis.value < -AXIS_THRESHOLD)
                    {
                        // the triggers can't have negative values, so we don't need to handle them here
                        remapKeycode = SDL_CONTROLLER_BUTTON_MAX + (ev.caxis.axis * 2);
                    }
                    break;
                }
                break;
            }
            case SDL_JOYBUTTONDOWN:
            {
                if (remapDevice &&
                    remapDevice->deviceType == DEVICE_TYPE_JOYSTICK &&
                    remapDevice->joystick &&
                    SDL_JoystickFromInstanceID(ev.jbutton.which) == remapDevice->joystick)
                {
                    remapKeycode = ev.jbutton.button;
                }
                break;
            }
            case SDL_JOYHATMOTION:
            {
                if (remapDevice &&
                    remapDevice->deviceType == DEVICE_TYPE_JOYSTICK &&
                    remapDevice->joystick &&
                    SDL_JoystickFromInstanceID(ev.jhat.which) == remapDevice->joystick)
                {
                    // do nothing if the d-pad is pressed diagonally; wait for a cardinal direction
                    unsigned int base = SDL_JoystickNumButtons(remapDevice->joystick) + (4 * ev.jhat.hat);
                    switch (ev.jhat.value)
                    {
                        case SDL_HAT_UP:
                            remapKeycode = base;
                            break;
                        case SDL_HAT_DOWN:
                            remapKeycode = base + 1;
                            break;
                        case SDL_HAT_LEFT:
                            remapKeycode = base + 2;
                            break;
                        case SDL_HAT_RIGHT:
                            remapKeycode = base + 3;
                            break;
                    }
                }
                break;
            }
            case SDL_JOYAXISMOTION:
            {
                if (remapDevice &&
                    remapDevice->deviceType == DEVICE_TYPE_JOYSTICK &&
                    remapDevice->joystick &&
                    SDL_JoystickFromInstanceID(ev.jaxis.which) == remapDevice->joystick)
                {
                    if (ev.jaxis.value < -AXIS_THRESHOLD || ev.jaxis.value > AXIS_THRESHOLD)
                    {
                        remapKeycode = SDL_JoystickNumButtons(remapDevice->joystick) +
                                       4 * SDL_JoystickNumHats(remapDevice->joystick) +
                                       2 * ev.jaxis.axis + ((ev.jaxis.value < -AXIS_THRESHOLD) ? 0 : 1);
                    }
                }
                break;
            }

#ifdef ANDROID
            case SDL_FINGERDOWN:
            {
                for (int i = 0; i < MAX_POINTERS; i++)
                {
                    if (touch_info.pstatus[i] == TOUCH_STATUS_UP)
                    {
                        touch_info.pid[i] = ev.tfinger.fingerId;
                        touch_info.px[i] = ev.tfinger.x * nativeWidth;
                        touch_info.py[i] = ev.tfinger.y * nativeHeight;
                        touch_info.pstatus[i] = TOUCH_STATUS_DOWN;
                        break;
                    }
                }
                control_update_android_touch(&touch_info, MAX_POINTERS);
                break;
            }

            case SDL_FINGERUP:
            {
                for (int i = 0; i < MAX_POINTERS; i++)
                {
                    if (touch_info.pid[i] == ev.tfinger.fingerId)
                    {
                        touch_info.pstatus[i] = TOUCH_STATUS_UP;
                        break;
                    }
                }
                control_update_android_touch(&touch_info, MAX_POINTERS);
                break;
            }

            case SDL_FINGERMOTION:
            {
                for (int i = 0; i < MAX_POINTERS; i++)
                {
                    if (touch_info.pid[i] == ev.tfinger.fingerId)
                    {
                        touch_info.px[i] = ev.tfinger.x * nativeWidth;
                        touch_info.py[i] = ev.tfinger.y * nativeHeight;
                        touch_info.pstatus[i] = TOUCH_STATUS_DOWN;
                        break;
                    }
                }
                control_update_android_touch(&touch_info, MAX_POINTERS);
                break;
            }
#endif
        }
    }
}

// Returns 1 if key is pressed, 0 if not
static unsigned int is_key_pressed(InputDevice *device, int keycode)
{
    if (device->deviceType == DEVICE_TYPE_KEYBOARD)
    {
        // If Enter was pressed as part of an Alt+Enter to toggle fullscreen, don't count it as an in-game button press.
        if (keycode == SDL_SCANCODE_RETURN && altEnterPressed)
        {
            return 0;
        }

        return SDL_GetKeyboardState(NULL)[keycode];
    }
    else if (device->deviceType == DEVICE_TYPE_CONTROLLER)
    {
        SDL_GameController *controller = device->controller;
        if (keycode < SDL_CONTROLLER_BUTTON_MAX)
        {
            return SDL_GameControllerGetButton(controller, keycode);
        }
        else
        {
            int axisCode = keycode - SDL_CONTROLLER_BUTTON_MAX;
            switch (axisCode)
            {
                case 0: return (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) < -AXIS_THRESHOLD);
                case 1: return (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) > AXIS_THRESHOLD);
                case 2: return (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) < -AXIS_THRESHOLD);
                case 3: return (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) > AXIS_THRESHOLD);
                case 4: return (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX) < -AXIS_THRESHOLD);
                case 5: return (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX) > AXIS_THRESHOLD);
                case 6: return (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY) < -AXIS_THRESHOLD);
                case 7: return (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY) > AXIS_THRESHOLD);
                case 8: return (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > AXIS_THRESHOLD);
                case 9: return (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > AXIS_THRESHOLD);
            }
            return 0;
        }
    }
    else if (device->deviceType == DEVICE_TYPE_JOYSTICK)
    {
        SDL_Joystick *joystick = device->joystick;
        if (keycode < SDL_JoystickNumButtons(joystick))
        {
            return SDL_JoystickGetButton(joystick, keycode);
        }

        keycode -= SDL_JoystickNumButtons(joystick);
        if (keycode < 4 * SDL_JoystickNumHats(joystick))
        {
            const int hatMasks[] = {SDL_HAT_UP, SDL_HAT_DOWN, SDL_HAT_LEFT, SDL_HAT_RIGHT};
            return !!(SDL_JoystickGetHat(joystick, keycode / 4) & hatMasks[keycode % 4]);
        }

        keycode -= 4 * SDL_JoystickNumHats(joystick);
        if (keycode < 2 * SDL_JoystickNumAxes(joystick))
        {
            Sint16 axisPosition = SDL_JoystickGetAxis(joystick, keycode / 2);
            if (keycode & 1)
            {
                return (axisPosition > AXIS_THRESHOLD);
            }
            else
            {
                return (axisPosition < -AXIS_THRESHOLD);
            }
        }
    }

    return 0;
}

void control_update_player(s_playercontrols *playerControls)
{
    uint32_t keyflags = 0;
    InputDevice *device = &devices[playerControls->deviceID];

    for (unsigned int i = 0; i < SDID_COUNT; i++)
    {
        keyflags |= (is_key_pressed(device, device->mappings[i]) << i);
    }

    playerControls->newkeyflags = keyflags & (~playerControls->keyflags);
    playerControls->keyflags = keyflags;
}

void control_update(s_playercontrols **playerControls, int numPlayers)
{
    handle_events();

    for (int i = 0; i < numPlayers; i++)
    {
        control_update_player(playerControls[i]);
    }
}

void control_remapdevice(int deviceID)
{
    if (deviceID < 0)
    {
        // done remapping; reset globals to default values
        remapDevice = NULL;
        remapKeycode = -1;
    }
    else
    {
        assert(devices[deviceID].deviceType != DEVICE_TYPE_NONE);
        remapDevice = &devices[deviceID];
        remapKeycode = -1;
    }
}

int control_getremappedkey()
{
    return remapKeycode;
}

int *control_getmappings(int deviceID)
{
    return devices[deviceID].mappings;
}

const char *control_getkeyname(int deviceID, int keycode)
{
    if (deviceID < 0) return "None";

    if (devices[deviceID].deviceType == DEVICE_TYPE_KEYBOARD)
    {
        return SDL_GetKeyName(SDL_GetKeyFromScancode(keycode));
    }
    else if (devices[deviceID].deviceType == DEVICE_TYPE_CONTROLLER)
    {
        // These are more readable than the button names we get from SDL
        const char *buttonNames[] = {
            "A",
            "B",
            "X",
            "Y",
            "Back",
            "Guide",
            "Start",
            "Left Stick Button",
            "Right Stick Button",
            "LB",
            "RB",
            "D-Pad Up",
            "D-Pad Down",
            "D-Pad Left",
            "D-Pad Right",
            "Left Stick Left",
            "Left Stick Right",
            "Left Stick Up",
            "Left Stick Down",
            "Right Stick Left",
            "Right Stick Right",
            "Right Stick Up",
            "Right Stick Down",
            "LT",
            "RT"
        };

        if (keycode < sizeof(buttonNames) / sizeof(buttonNames[0]))
        {
            return buttonNames[keycode];
        }
    }
    else if (devices[deviceID].deviceType == DEVICE_TYPE_JOYSTICK)
    {
        static char buttonName[64];
        SDL_Joystick *joystick = devices[deviceID].joystick;
        if (keycode < SDL_JoystickNumButtons(joystick))
        {
            snprintf(buttonName, sizeof(buttonName), "Button %u", keycode + 1);
            return buttonName;
        }

        keycode -= SDL_JoystickNumButtons(joystick);
        if (keycode < 4 * SDL_JoystickNumHats(joystick))
        {
            // omit the hat number because no real device is going to have more than one d-pad
            const char *directions[] = {"Up", "Down", "Left", "Right"};
            snprintf(buttonName, sizeof(buttonName), "D-Pad %s", directions[keycode % 4]);
            return buttonName;
        }

        keycode -= 4 * SDL_JoystickNumHats(joystick);
        if (keycode < 2 * SDL_JoystickNumAxes(joystick))
        {
            snprintf(buttonName, sizeof(buttonName), "Axis %i %c", (keycode / 2) + 1, (keycode & 1) ? '+' : '-');
            return buttonName;
        }
    }

    return "None";
}

bool control_isvaliddevice(int deviceID)
{
    return deviceID >= 0 && devices[deviceID].deviceType != DEVICE_TYPE_NONE;
}

const char *control_getdevicename(int deviceID)
{
    return devices[deviceID].deviceType == DEVICE_TYPE_NONE ? "None" : devices[deviceID].name;
}

void control_rumble(int deviceID, int ratio, int msec)
{
    if (msec > 0 && devices[deviceID].haptic)
    {
        if (SDL_HapticRumblePlay(devices[deviceID].haptic, ratio, msec) != 0)
        {
            printf("Warning: Unable to play rumble! %s\n", SDL_GetError());
        }
    }
}

#ifdef ANDROID
/*
Get if touchscreen vibration is active
*/
bool is_touchpad_vibration_enabled()
{
    return !!(savedata.is_touchpad_vibration_enabled);
}

/*
Android touch logic, the rest of the code is in android/jni/video.c,
maybe they'll be merged someday.
*/
extern float bx[MAXTOUCHB];
extern float by[MAXTOUCHB];
extern float br[MAXTOUCHB];
extern unsigned touchstates[MAXTOUCHB];
int hide_t = 5000;

static void control_update_android_touch(TouchStatus *touch_info, int maxp)
{
    #define pc(x) devices[keyboardDeviceID].mappings[x]
    int i, j;
    float tx, ty, tr;
    float r[MAXTOUCHB];
    float dirx, diry, circlea, circleb, tan;
    Uint8* keystate = (Uint8*) SDL_GetKeyboardState(NULL);
    SDL_Event event;

    if (keyboardDeviceID < 0) return;

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

        event.type = SDL_KEYDOWN;
        event.key.type = SDL_KEYDOWN;
        event.key.timestamp = SDL_GetTicks();
        event.key.state = SDL_PRESSED;
        event.key.repeat = 0;

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
                    event.key.keysym.scancode = pc(SDID_MOVELEFT);
                    SDL_PushEvent(&event);
                }
                else if(tan<-tanb)
                {
                    touchstates[SDID_MOVEDOWN] = 1;
                    event.key.keysym.scancode = pc(SDID_MOVEDOWN);
                    SDL_PushEvent(&event);
                }
                else if(tan>tanb)
                {
                    touchstates[SDID_MOVEUP] = 1;
                    event.key.keysym.scancode = pc(SDID_MOVEUP);
                    SDL_PushEvent(&event);
                }
                else if(ty<0)
                {
                    touchstates[SDID_MOVEUP] = touchstates[SDID_MOVELEFT] = 1;
                    event.key.keysym.scancode = pc(SDID_MOVEUP);
                    SDL_PushEvent(&event);
                    event.key.keysym.scancode = pc(SDID_MOVELEFT);
                    SDL_PushEvent(&event);
                }
                else
                {
                    touchstates[SDID_MOVELEFT] = touchstates[SDID_MOVEDOWN] = 1;
                    event.key.keysym.scancode = pc(SDID_MOVELEFT);
                    SDL_PushEvent(&event);
                    event.key.keysym.scancode = pc(SDID_MOVEDOWN);
                    SDL_PushEvent(&event);
                }
            }
            else if(tx>0)
            {
                tan = ty/tx;
                if(tan>=-tana && tan<=tana)
                {
                    touchstates[SDID_MOVERIGHT] = 1;
                    event.key.keysym.scancode = pc(SDID_MOVERIGHT);
                    SDL_PushEvent(&event);
                }
                else if(tan<-tanb)
                {
                    touchstates[SDID_MOVEUP] = 1;
                    event.key.keysym.scancode = pc(SDID_MOVEUP);
                    SDL_PushEvent(&event);
                }
                else if(tan>tanb)
                {
                    touchstates[SDID_MOVEDOWN] = 1;
                    event.key.keysym.scancode = pc(SDID_MOVEDOWN);
                    SDL_PushEvent(&event);
                }
                else if(ty<0)
                {
                    touchstates[SDID_MOVEUP] = touchstates[SDID_MOVERIGHT] = 1;
                    event.key.keysym.scancode = pc(SDID_MOVEUP);
                    SDL_PushEvent(&event);
                    event.key.keysym.scancode = pc(SDID_MOVERIGHT);
                    SDL_PushEvent(&event);
                }
                else
                {
                    touchstates[SDID_MOVERIGHT] = touchstates[SDID_MOVEDOWN] = 1;
                    event.key.keysym.scancode = pc(SDID_MOVERIGHT);
                    SDL_PushEvent(&event);
                    event.key.keysym.scancode = pc(SDID_MOVEDOWN);
                    SDL_PushEvent(&event);
                }
            }
            else
            {
                if(ty>0)
                {
                    touchstates[SDID_MOVEDOWN] = 1;
                    event.key.keysym.scancode = pc(SDID_MOVEDOWN);
                    SDL_PushEvent(&event);
                }
                else
                {
                    touchstates[SDID_MOVEUP] = 1;
                    event.key.keysym.scancode = pc(SDID_MOVEUP);
                    SDL_PushEvent(&event);
                }
            }
        }

        //rest of the buttons
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
                event.key.keysym.scancode = pc(j);
                SDL_PushEvent(&event);
            }
        }
    }
    #undef tana
    #undef tanb

    hide_t = timer_gettick() + 5000;

    //map to current user settings
    assert(keyboardDeviceID >= 0);
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
    keystate[pc(SDID_SCREENSHOT)] = touchstates[SDID_SCREENSHOT];
    keystate[pc(SDID_ESC)] = touchstates[SDID_ESC];

    #undef pc
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

#define MAPPINGS_FILE_SENTINEL 0x9cf232d4

bool control_loadmappings(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp)
    {
        return false;
    }

    clear_saved_mappings();

    while (!feof(fp) && !ferror(fp))
    {
        char name[CONTROL_DEVICE_NAME_SIZE];
		int *mapping = malloc(SDID_COUNT * sizeof(int));
		int sentinel;
        if (fread(name, 1, sizeof(name), fp) != sizeof(name) ||
            fread(mapping, sizeof(int), SDID_COUNT, fp) != SDID_COUNT ||
            fread(&sentinel, sizeof(int), 1, fp) != 1)
        {
            free(mapping);
            break;
        }
        else if (sentinel != MAPPINGS_FILE_SENTINEL)
        {
            free(mapping);
            fclose(fp);
            return false;
        }

        name[sizeof(name)-1] = '\0'; // just in case
        printf("Loaded mapping for %s\n", name);
        List_InsertAfter(&savedMappings, mapping, name);
    }

    fclose(fp);

    // update all current device mappings with the newly loaded mappings
    for (int i = 0; i < MAX_DEVICES; i++)
    {
        if (devices[i].deviceType != DEVICE_TYPE_NONE)
        {
            load_from_saved_mapping(i);
        }
    }

    return true;
}

bool control_savemappings(const char *filename)
{
    // update savedMappings with all current device mappings
    for (int i = 0; i < MAX_DEVICES; i++)
    {
        if (devices[i].deviceType != DEVICE_TYPE_NONE)
        {
            update_saved_mapping(i);
        }
    }

    FILE *fp = fopen(filename, "wb");
    if (!fp)
    {
        return false;
    }

    int numMappings = List_GetSize(&savedMappings);
	List_Reset(&savedMappings);
	for (int i = 0; i < numMappings; i++)
	{
		char name[CONTROL_DEVICE_NAME_SIZE];
		snprintf(name, sizeof(name), "%s", List_GetName(&savedMappings));
		int *mapping = List_Retrieve(&savedMappings);
		const int sentinel = MAPPINGS_FILE_SENTINEL;
        if (fwrite(name, 1, sizeof(name), fp) != sizeof(name) ||
            fwrite(mapping, sizeof(int), SDID_COUNT, fp) != SDID_COUNT ||
            fwrite(&sentinel, sizeof(int), 1, fp) != 1)
        {
            fclose(fp);
            return false;
        }

        List_GotoNext(&savedMappings);
    }

    fclose(fp);
    return true;
}

void control_clearmappings()
{
    clear_saved_mappings();

    for (int i = 0; i < MAX_DEVICES; i++)
    {
        if (devices[i].deviceType != DEVICE_TYPE_NONE)
        {
            control_resetmappings(i);
        }
    }
}

