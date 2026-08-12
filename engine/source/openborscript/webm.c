/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) OpenBOR Team
 */

#include "scriptcommon.h"

extern s_savedata savedata;

#ifdef WEBM

typedef struct s_webm_property_info {
    e_webm_properties property;
    e_property_access_config_flags config_flags;
    size_t offset;
    const char *id_string;
    VARTYPE type;
} s_webm_property_info;

#define PROPERTY_MEMBER_OFFSET(type, member) ((size_t)&(((type*)0)->member))

static const s_webm_property_info webm_properties[] = {
    { WEBM_PROPERTY_ACTIVE, PROPERTY_ACCESS_CONFIG_READ,
      PROPERTY_MEMBER_OFFSET(s_webm_playback, active),
      "WEBM_PROPERTY_ACTIVE", VT_INTEGER },
    { WEBM_PROPERTY_CHANNEL, PROPERTY_ACCESS_CONFIG_READ,
      PROPERTY_MEMBER_OFFSET(s_webm_playback, index),
      "WEBM_PROPERTY_CHANNEL", VT_INTEGER },
    { WEBM_PROPERTY_DISPLAY_MODE, PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
      PROPERTY_MEMBER_OFFSET(s_webm_playback, display_mode),
      "WEBM_PROPERTY_DISPLAY_MODE", VT_INTEGER },
    { WEBM_PROPERTY_DURATION, PROPERTY_ACCESS_CONFIG_READ,
      PROPERTY_MEMBER_OFFSET(s_webm_playback, duration),
      "WEBM_PROPERTY_DURATION", VT_UINTEGER64 },
    { WEBM_PROPERTY_HEIGHT, PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
      PROPERTY_MEMBER_OFFSET(s_webm_playback, height),
      "WEBM_PROPERTY_HEIGHT", VT_INTEGER },
    { WEBM_PROPERTY_INTERRUPT, PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
      PROPERTY_MEMBER_OFFSET(s_webm_playback, interrupt),
      "WEBM_PROPERTY_INTERRUPT", VT_INTEGER },
    { WEBM_PROPERTY_LOADING, PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
      PROPERTY_MEMBER_OFFSET(s_webm_playback, loading_mode),
      "WEBM_PROPERTY_LOADING", VT_INTEGER },
    { WEBM_PROPERTY_OFFSET_X, PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
      PROPERTY_MEMBER_OFFSET(s_webm_playback, offset_x),
      "WEBM_PROPERTY_OFFSET_X", VT_INTEGER },
    { WEBM_PROPERTY_OFFSET_Y, PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
      PROPERTY_MEMBER_OFFSET(s_webm_playback, offset_y),
      "WEBM_PROPERTY_OFFSET_Y", VT_INTEGER },
    { WEBM_PROPERTY_PAUSED, PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
      PROPERTY_MEMBER_OFFSET(s_webm_playback, paused),
      "WEBM_PROPERTY_PAUSED", VT_INTEGER },
    { WEBM_PROPERTY_POSITION, PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
      PROPERTY_MEMBER_OFFSET(s_webm_playback, position),
      "WEBM_PROPERTY_POSITION", VT_UINTEGER64 },
    { WEBM_PROPERTY_REPEAT, PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
      PROPERTY_MEMBER_OFFSET(s_webm_playback, repeat),
      "WEBM_PROPERTY_REPEAT", VT_INTEGER },
    { WEBM_PROPERTY_SCREEN, PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
      PROPERTY_MEMBER_OFFSET(s_webm_playback, screen),
      "WEBM_PROPERTY_SCREEN", VT_PTR },
    { WEBM_PROPERTY_SOUND_CHANNEL, PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
      PROPERTY_MEMBER_OFFSET(s_webm_playback, sound_channel),
      "WEBM_PROPERTY_SOUND_CHANNEL", VT_INTEGER },
    { WEBM_PROPERTY_SPEED, PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
      PROPERTY_MEMBER_OFFSET(s_webm_playback, speed),
      "WEBM_PROPERTY_SPEED", VT_DECIMAL },
    { WEBM_PROPERTY_WIDTH, PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
      PROPERTY_MEMBER_OFFSET(s_webm_playback, width),
      "WEBM_PROPERTY_WIDTH", VT_INTEGER },
    { WEBM_PROPERTY_END, PROPERTY_ACCESS_CONFIG_NONE, 0,
      "WebM", VT_EMPTY }
};

#undef PROPERTY_MEMBER_OFFSET

/*
* Caskey, Damon V.
* 2026-08-12
*
* Build property metadata for a stable WebM channel object.
*/
static const s_property_access_map webm_get_property_map(
    const void *acting_object_param,
    const unsigned int property_index_param
)
{
    s_property_access_map property_map = { 0 };
    const s_webm_playback *acting_object = acting_object_param;
    const s_webm_property_info *info = NULL;
    size_t property_cursor;

    for(property_cursor = 0;
        property_cursor < sizeof(webm_properties) / sizeof(webm_properties[0]);
        property_cursor++) {
        if(webm_properties[property_cursor].property == property_index_param) {
            info = &webm_properties[property_cursor];
            break;
        }
    }

    if(info) {
        property_map.config_flags = info->config_flags;
        property_map.field = (const void*)((const char*)acting_object + info->offset);
        property_map.id_string = info->id_string;
        property_map.type = info->type;
    } else {
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_NONE;
        property_map.field = NULL;
        property_map.id_string = "WebM";
        property_map.type = VT_EMPTY;
    }
    return property_map;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Start nonblocking playback into a creator-owned 32-bit
* screen and return the acquired channel object.
*/
HRESULT openbor_open_webm(
    ScriptVariant **varlist,
    ScriptVariant **pretvar,
    int paramCount
)
{
    const char *self_name =
        "open_webm(string path, void screen, int sound_channel, int interrupt, int loading)";
    s_webm_playback *playback;
    s_screen *screen;
    LONG sound_channel = SOUND_CHANNEL_MUSIC_DEFAULT;
    LONG interrupt = 1;
    LONG loading_mode = WEBM_LOADING_STREAM;

    ScriptVariant_Clear(*pretvar);
    if(paramCount < 2 ||
       varlist[0]->vt != VT_STR ||
       varlist[1]->vt != VT_PTR ||
       !varlist[1]->ptrVal ||
       (paramCount > 2 && FAILED(ScriptVariant_IntegerValue(varlist[2], &sound_channel))) ||
       (paramCount > 3 && FAILED(ScriptVariant_IntegerValue(varlist[3], &interrupt))) ||
       (paramCount > 4 && FAILED(ScriptVariant_IntegerValue(varlist[4], &loading_mode))) ||
       sound_channel < 0 ||
       (unsigned int)sound_channel >= SOUND_CHANNEL_COUNT_MAX ||
       loading_mode < 0 || loading_mode >= WEBM_LOADING_END) {
        printf("\nScript error: %s. Invalid path, screen, sound channel, interrupt, or loading mode.\n",
            self_name);
        *pretvar = NULL;
        return E_FAIL;
    }

    screen = (s_screen*)varlist[1]->ptrVal;
    playback = webm_playback_open(
        StrCache_Get(varlist[0]->strVal),
        screen,
        (int)sound_channel,
        interrupt != 0,
        (e_webm_loading_mode)loading_mode,
        savedata.musicvol
    );
    if(playback) {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = playback;
        return S_OK;
    }

    printf("\nScript error: %s. Playback could not acquire a channel, screen, or decoder.\n",
        self_name);
    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Return one stable object from the single 64-channel bank.
*/
HRESULT openbor_get_webm_channel_object(
    ScriptVariant **varlist,
    ScriptVariant **pretvar,
    int paramCount
)
{
    LONG channel;
    s_webm_playback *playback;

    ScriptVariant_Clear(*pretvar);
    if(paramCount < 1 ||
       FAILED(ScriptVariant_IntegerValue(varlist[0], &channel)) ||
       channel < 0 ||
       (unsigned int)channel >= WEBM_PLAYBACK_CHANNEL_COUNT ||
       !webm_playback_init()) {
        printf("\nScript error: get_webm_channel_object(int channel). Channel must be from 0 through %u.\n",
            WEBM_PLAYBACK_CHANNEL_COUNT - 1U);
        *pretvar = NULL;
        return E_FAIL;
    }

    playback = webm_playback_get((int)channel);
    if(playback) {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = playback;
    }
    return S_OK;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Resolve a validated playback pointer to its channel index.
*/
HRESULT openbor_get_webm_channel_index(
    ScriptVariant **varlist,
    ScriptVariant **pretvar,
    int paramCount
)
{
    int channel;

    ScriptVariant_Clear(*pretvar);
    if(paramCount < 1 ||
       varlist[0]->vt != VT_PTR ||
       !varlist[0]->ptrVal ||
       (channel = webm_playback_get_index(varlist[0]->ptrVal)) < 0) {
        printf("\nScript error: get_webm_channel_index(void playback). You must provide a valid WebM object.\n");
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)channel;
    return S_OK;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Return the active mask for the single 64-channel video bank.
*/
HRESULT openbor_get_webm_channel_mask(
    ScriptVariant **varlist,
    ScriptVariant **pretvar,
    int paramCount
)
{
    (void)varlist;
    (void)paramCount;

    ScriptVariant_ChangeType(*pretvar, VT_UINTEGER64);
    (*pretvar)->ullVal = webm_playback_get_active_mask();
    return S_OK;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Return a synchronized snapshot property or diagnostic dump.
*/
HRESULT openbor_get_webm_property(
    ScriptVariant **varlist,
    ScriptVariant **pretvar,
    int paramCount
)
{
    s_webm_playback snapshot;
    int property_index;
    s_property_access_map property_map;

    ScriptVariant_Clear(*pretvar);
    if(paramCount < 2 ||
       varlist[0]->vt != VT_PTR ||
       !varlist[0]->ptrVal ||
       varlist[1]->vt != VT_INTEGER ||
       !webm_playback_get_snapshot(varlist[0]->ptrVal, &snapshot)) {
        printf("\nScript error: get_webm_property(void playback, int property). Invalid object or property.\n");
        *pretvar = NULL;
        return E_FAIL;
    }

    property_index = (int)varlist[1]->lVal;
    if(property_index >= 0 && property_index < WEBM_PROPERTY_END) {
        property_map = webm_get_property_map(&snapshot, (unsigned int)property_index);
        return property_access_get_member(&property_map, *pretvar);
    }
    if(property_index == PROPERTY_ACCESS_DUMP) {
        property_access_dump_members(
            webm_get_property_map,
            WEBM_PROPERTY_END,
            &snapshot
        );
        return S_OK;
    }

    printf("\nScript error: get_webm_property(void playback, int property). Unknown property id (%d).\n",
        property_index);
    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Route mutable WebM fields through lifecycle-aware setters
* so seek, audio, clocks, scaling, and screen ownership agree.
*/
HRESULT openbor_set_webm_property(
    ScriptVariant **varlist,
    ScriptVariant **pretvar,
    int paramCount
)
{
    s_webm_playback *playback;
    int property_index;
    LONG integer_value;
    DOUBLE decimal_value;
    uint64_t unsigned_value;
    bool result = false;

    *pretvar = NULL;
    if(paramCount < 3 ||
       varlist[0]->vt != VT_PTR ||
       !varlist[0]->ptrVal ||
       varlist[1]->vt != VT_INTEGER ||
       webm_playback_get_index(varlist[0]->ptrVal) < 0) {
        goto error_object;
    }

    playback = varlist[0]->ptrVal;
    property_index = (int)varlist[1]->lVal;
    switch((e_webm_properties)property_index) {
        case WEBM_PROPERTY_DISPLAY_MODE:
            result = SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &integer_value)) &&
                webm_playback_set_display_mode(playback, (e_webm_display_mode)integer_value);
            break;
        case WEBM_PROPERTY_HEIGHT:
            result = SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &integer_value)) &&
                webm_playback_set_height(playback, (int)integer_value);
            break;
        case WEBM_PROPERTY_INTERRUPT:
            result = SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &integer_value)) &&
                webm_playback_set_interrupt(playback, integer_value != 0);
            break;
        case WEBM_PROPERTY_LOADING:
            result = SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &integer_value)) &&
                webm_playback_set_loading_mode(
                    playback,
                    (e_webm_loading_mode)integer_value,
                    savedata.musicvol
                );
            break;
        case WEBM_PROPERTY_OFFSET_X:
            result = SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &integer_value)) &&
                webm_playback_set_offset_x(playback, (int)integer_value);
            break;
        case WEBM_PROPERTY_OFFSET_Y:
            result = SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &integer_value)) &&
                webm_playback_set_offset_y(playback, (int)integer_value);
            break;
        case WEBM_PROPERTY_PAUSED:
            result = SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &integer_value)) &&
                webm_playback_set_paused(playback, integer_value != 0);
            break;
        case WEBM_PROPERTY_POSITION:
            result = SUCCEEDED(ScriptVariant_Unsigned64Value(varlist[2], &unsigned_value)) &&
                webm_playback_set_position(playback, unsigned_value, savedata.musicvol);
            break;
        case WEBM_PROPERTY_REPEAT:
            result = SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &integer_value)) &&
                webm_playback_set_repeat(playback, integer_value != 0);
            break;
        case WEBM_PROPERTY_SCREEN:
            result = varlist[2]->vt == VT_PTR && varlist[2]->ptrVal &&
                webm_playback_set_screen(playback, varlist[2]->ptrVal);
            break;
        case WEBM_PROPERTY_SOUND_CHANNEL:
            result = SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &integer_value)) &&
                webm_playback_set_sound_channel(
                    playback,
                    (int)integer_value,
                    savedata.musicvol
                );
            break;
        case WEBM_PROPERTY_SPEED:
            result = SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &decimal_value)) &&
                webm_playback_set_speed(playback, decimal_value);
            break;
        case WEBM_PROPERTY_WIDTH:
            result = SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &integer_value)) &&
                webm_playback_set_width(playback, (int)integer_value);
            break;
        case WEBM_PROPERTY_ACTIVE:
        case WEBM_PROPERTY_CHANNEL:
        case WEBM_PROPERTY_DURATION:
        case WEBM_PROPERTY_END:
        default:
            printf("\nScript error: set_webm_property(void playback, int property, <mixed> value). Property id %d is read-only or unknown.\n",
                property_index);
            return E_FAIL;
    }

    if(result) {
        return S_OK;
    }
    printf("\nScript error: set_webm_property(void playback, int property, <mixed> value). Invalid value for property id %d.\n",
        property_index);
    return E_FAIL;

error_object:
    printf("\nScript error: set_webm_property(void playback, int property, <mixed> value). Invalid WebM object or property.\n");
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Explicitly stop and recycle one WebM playback channel.
*/
HRESULT openbor_stop_webm(
    ScriptVariant **varlist,
    ScriptVariant **pretvar,
    int paramCount
)
{
    *pretvar = NULL;
    if(paramCount < 1 ||
       varlist[0]->vt != VT_PTR ||
       !varlist[0]->ptrVal ||
       webm_playback_get_index(varlist[0]->ptrVal) < 0) {
        printf("\nScript error: stop_webm(void playback). You must provide a valid WebM object.\n");
        return E_FAIL;
    }

    webm_playback_stop(varlist[0]->ptrVal);
    return S_OK;
}

#else

/*
* Caskey, Damon V.
* 2026-08-12
*
* Keep WebM script names available on builds that exclude
* the decoder while reporting the unsupported configuration.
*/
static HRESULT openbor_webm_unsupported(ScriptVariant **pretvar)
{
    ScriptVariant_Clear(*pretvar);
    printf("WebM playback is not supported on this platform.\n");
    return S_OK;
}

HRESULT openbor_open_webm(ScriptVariant **v, ScriptVariant **r, int c)
{ (void)v; (void)c; return openbor_webm_unsupported(r); }
HRESULT openbor_get_webm_channel_object(ScriptVariant **v, ScriptVariant **r, int c)
{ (void)v; (void)c; return openbor_webm_unsupported(r); }
HRESULT openbor_get_webm_channel_index(ScriptVariant **v, ScriptVariant **r, int c)
{ (void)v; (void)c; return openbor_webm_unsupported(r); }
HRESULT openbor_get_webm_channel_mask(ScriptVariant **v, ScriptVariant **r, int c)
{ (void)v; (void)c; return openbor_webm_unsupported(r); }
HRESULT openbor_get_webm_property(ScriptVariant **v, ScriptVariant **r, int c)
{ (void)v; (void)c; return openbor_webm_unsupported(r); }
HRESULT openbor_set_webm_property(ScriptVariant **v, ScriptVariant **r, int c)
{ (void)v; (void)c; return openbor_webm_unsupported(r); }
HRESULT openbor_stop_webm(ScriptVariant **v, ScriptVariant **r, int c)
{ (void)v; (void)c; return openbor_webm_unsupported(r); }

#endif
