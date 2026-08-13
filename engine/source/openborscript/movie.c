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

typedef struct s_movie_property_info {
    e_movie_properties property;
    e_property_access_config_flags config_flags;
    size_t offset;
    const char *id_string;
    VARTYPE type;
} s_movie_property_info;

#define PROPERTY_MEMBER_OFFSET(type, member) ((size_t)&(((type*)0)->member))

static const s_movie_property_info movie_properties[] = {
    { MOVIE_PROPERTY_ACTIVE, PROPERTY_ACCESS_CONFIG_READ,
      PROPERTY_MEMBER_OFFSET(s_movie_playback, active),
      "MOVIE_PROPERTY_ACTIVE", VT_INTEGER },
    { MOVIE_PROPERTY_BLACK_FILTER, PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
      PROPERTY_MEMBER_OFFSET(s_movie_playback, black_filter),
      "MOVIE_PROPERTY_BLACK_FILTER", VT_INTEGER },
    { MOVIE_PROPERTY_CHANNEL, PROPERTY_ACCESS_CONFIG_READ,
      PROPERTY_MEMBER_OFFSET(s_movie_playback, index),
      "MOVIE_PROPERTY_CHANNEL", VT_INTEGER },
    { MOVIE_PROPERTY_DURATION, PROPERTY_ACCESS_CONFIG_READ,
      PROPERTY_MEMBER_OFFSET(s_movie_playback, duration),
      "MOVIE_PROPERTY_DURATION", VT_UINTEGER64 },
    { MOVIE_PROPERTY_HEIGHT, PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
      PROPERTY_MEMBER_OFFSET(s_movie_playback, height),
      "MOVIE_PROPERTY_HEIGHT", VT_INTEGER },
    { MOVIE_PROPERTY_INTERRUPT, PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
      PROPERTY_MEMBER_OFFSET(s_movie_playback, interrupt),
      "MOVIE_PROPERTY_INTERRUPT", VT_INTEGER },
    { MOVIE_PROPERTY_OFFSET_X, PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
      PROPERTY_MEMBER_OFFSET(s_movie_playback, offset_x),
      "MOVIE_PROPERTY_OFFSET_X", VT_INTEGER },
    { MOVIE_PROPERTY_OFFSET_Y, PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
      PROPERTY_MEMBER_OFFSET(s_movie_playback, offset_y),
      "MOVIE_PROPERTY_OFFSET_Y", VT_INTEGER },
    { MOVIE_PROPERTY_PAUSED, PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
      PROPERTY_MEMBER_OFFSET(s_movie_playback, paused),
      "MOVIE_PROPERTY_PAUSED", VT_INTEGER },
    { MOVIE_PROPERTY_POSITION, PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
      PROPERTY_MEMBER_OFFSET(s_movie_playback, position),
      "MOVIE_PROPERTY_POSITION", VT_UINTEGER64 },
    { MOVIE_PROPERTY_REPEAT, PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
      PROPERTY_MEMBER_OFFSET(s_movie_playback, repeat),
      "MOVIE_PROPERTY_REPEAT", VT_INTEGER },
    { MOVIE_PROPERTY_SOUND_CHANNEL, PROPERTY_ACCESS_CONFIG_READ,
      PROPERTY_MEMBER_OFFSET(s_movie_playback, sound_channel),
      "MOVIE_PROPERTY_SOUND_CHANNEL", VT_INTEGER },
    { MOVIE_PROPERTY_SOURCE, PROPERTY_ACCESS_CONFIG_READ,
      PROPERTY_MEMBER_OFFSET(s_movie_playback, source_id),
      "MOVIE_PROPERTY_SOURCE", VT_INTEGER },
    { MOVIE_PROPERTY_SPEED, PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
      PROPERTY_MEMBER_OFFSET(s_movie_playback, speed),
      "MOVIE_PROPERTY_SPEED", VT_DECIMAL },
    { MOVIE_PROPERTY_WIDTH, PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
      PROPERTY_MEMBER_OFFSET(s_movie_playback, width),
      "MOVIE_PROPERTY_WIDTH", VT_INTEGER },
    { MOVIE_PROPERTY_END, PROPERTY_ACCESS_CONFIG_NONE, 0,
      "Movie", VT_EMPTY }
};

#undef PROPERTY_MEMBER_OFFSET

/*
* Caskey, Damon V.
* 2026-08-12
*
* Build metadata for the stable movie playback object API.
*/
static const s_property_access_map movie_get_property_map(
    const void *acting_object_param,
    const unsigned int property_index_param
)
{
    s_property_access_map property_map = { 0 };
    const s_movie_playback *acting_object = acting_object_param;
    const s_movie_property_info *info = NULL;
    size_t property_cursor;

    for(property_cursor = 0;
        property_cursor < sizeof(movie_properties) / sizeof(movie_properties[0]);
        property_cursor++) {
        if(movie_properties[property_cursor].property == property_index_param) {
            info = &movie_properties[property_cursor];
            break;
        }
    }

    if(info) {
        property_map.config_flags = info->config_flags;
        property_map.field =
            (const void*)((const char*)acting_object + info->offset);
        property_map.id_string = info->id_string;
        property_map.type = info->type;
    } else {
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_NONE;
        property_map.id_string = "Movie";
        property_map.type = VT_EMPTY;
    }
    return property_map;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Load one reusable streamed or cached movie source and return
* its stable integer ID.
*/
HRESULT openbor_movie_load(
    ScriptVariant **varlist,
    ScriptVariant **pretvar,
    int paramCount
)
{
    const char *self_name = "movie_load(string path, int loading)";
    LONG loading = MOVIE_LOADING_STREAM;
    int source_id;

    ScriptVariant_Clear(*pretvar);
    if(paramCount < 1 ||
       varlist[0]->vt != VT_STR ||
       (paramCount > 1 &&
        FAILED(ScriptVariant_IntegerValue(varlist[1], &loading))) ||
       loading < 0 || loading >= MOVIE_LOADING_END) {
        printf("\nScript error: %s. Invalid path or loading mode.\n",
            self_name);
        *pretvar = NULL;
        return E_FAIL;
    }

    source_id = movie_source_load(
        StrCache_Get(varlist[0]->strVal),
        (e_movie_loading_mode)loading
    );
    if(source_id < 0) {
        printf("\nScript error: %s. Source could not be loaded.\n",
            self_name);
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)source_id;
    return S_OK;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Release one source ID when no playback still references it.
*/
HRESULT openbor_movie_unload(
    ScriptVariant **varlist,
    ScriptVariant **pretvar,
    int paramCount
)
{
    LONG source_id;

    *pretvar = NULL;
    if(paramCount < 1 ||
       FAILED(ScriptVariant_IntegerValue(varlist[0], &source_id)) ||
       !movie_source_unload((int)source_id)) {
        printf("\nScript error: movie_unload(int source). Source is invalid or still in use.\n");
        return E_FAIL;
    }
    return S_OK;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Play a reusable source on the first free movie channel or
* replace the explicitly selected channel.
*/
HRESULT openbor_movie_play(
    ScriptVariant **varlist,
    ScriptVariant **pretvar,
    int paramCount
)
{
    const char *self_name = "movie_play(int source, int channel)";
    LONG source_id;
    LONG channel = MOVIE_CHANNEL_AUTO;
    s_movie_playback *playback;

    ScriptVariant_Clear(*pretvar);
    if(paramCount < 1 ||
       FAILED(ScriptVariant_IntegerValue(varlist[0], &source_id))) {
        goto error;
    }
    if(paramCount > 1 &&
       varlist[1]->vt != VT_EMPTY &&
       !(varlist[1]->vt == VT_PTR && !varlist[1]->ptrVal) &&
       FAILED(ScriptVariant_IntegerValue(varlist[1], &channel))) {
        goto error;
    }
    if(channel != MOVIE_CHANNEL_AUTO &&
       (channel < 0 || (unsigned int)channel >= MOVIE_CHANNEL_COUNT)) {
        goto error;
    }

    playback = movie_playback_play(
        (int)source_id,
        (int)channel,
        savedata.musicvol,
        false
    );
    if(!playback) {
        printf("\nScript error: %s. Source is invalid or no channel could be acquired.\n",
            self_name);
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_ChangeType(*pretvar, VT_PTR);
    (*pretvar)->ptrVal = playback;
    return S_OK;

error:
    printf("\nScript error: %s. Source or channel is invalid.\n",
        self_name);
    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-08-13
*
* Draw the retained frame from one movie channel to a creator-owned
* 32-bit screen. Call position determines composition order.
*/
HRESULT openbor_movie_draw_to_screen(
    ScriptVariant **varlist,
    ScriptVariant **pretvar,
    int paramCount
)
{
    s_screen *screen;
    LONG channel;

    *pretvar = NULL;
    if(paramCount < 2 ||
       varlist[0]->vt != VT_PTR ||
       !varlist[0]->ptrVal ||
       FAILED(ScriptVariant_IntegerValue(varlist[1], &channel))) {
        goto error;
    }
    screen = varlist[0]->ptrVal;
    if(channel < 0 ||
       (unsigned int)channel >= MOVIE_CHANNEL_COUNT ||
       !movie_playback_draw_to_screen(screen, (int)channel)) {
        goto error;
    }
    return S_OK;

error:
    printf("\nScript error: movie_draw_to_screen(void screen, int channel). Screen or movie channel is invalid.\n");
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Reopen a playback at its current position on a selected
* sound channel without changing its movie channel.
*/
HRESULT openbor_movie_set_sound_channel(
    ScriptVariant **varlist,
    ScriptVariant **pretvar,
    int paramCount
)
{
    LONG channel;

    *pretvar = NULL;
    if(paramCount < 2 ||
       varlist[0]->vt != VT_PTR ||
       !varlist[0]->ptrVal ||
       FAILED(ScriptVariant_IntegerValue(varlist[1], &channel)) ||
       !movie_playback_set_sound_channel(
           varlist[0]->ptrVal,
           (int)channel,
           savedata.musicvol
       )) {
        printf("\nScript error: movie_set_sound_channel(void playback, int channel). Playback or channel is invalid.\n");
        return E_FAIL;
    }
    return S_OK;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Stop and recycle one stable movie playback channel.
*/
HRESULT openbor_movie_stop(
    ScriptVariant **varlist,
    ScriptVariant **pretvar,
    int paramCount
)
{
    *pretvar = NULL;
    if(paramCount < 1 ||
       varlist[0]->vt != VT_PTR ||
       !varlist[0]->ptrVal ||
       movie_playback_get_index(varlist[0]->ptrVal) < 0) {
        printf("\nScript error: movie_stop(void playback). Playback is invalid.\n");
        return E_FAIL;
    }
    movie_playback_stop(varlist[0]->ptrVal);
    return S_OK;
}

HRESULT openbor_movie_get_channel_object(
    ScriptVariant **varlist,
    ScriptVariant **pretvar,
    int paramCount
)
{
    LONG channel;
    s_movie_playback *playback;

    ScriptVariant_Clear(*pretvar);
    if(paramCount < 1 ||
       FAILED(ScriptVariant_IntegerValue(varlist[0], &channel)) ||
       channel < 0 ||
       (unsigned int)channel >= MOVIE_CHANNEL_COUNT ||
       !movie_playback_init()) {
        printf("\nScript error: movie_get_channel_object(int channel). Channel must be from 0 through %u.\n",
            MOVIE_CHANNEL_COUNT - 1U);
        *pretvar = NULL;
        return E_FAIL;
    }

    playback = movie_playback_get((int)channel);
    ScriptVariant_ChangeType(*pretvar, VT_PTR);
    (*pretvar)->ptrVal = playback;
    return S_OK;
}

HRESULT openbor_movie_get_channel_index(
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
       (channel = movie_playback_get_index(varlist[0]->ptrVal)) < 0) {
        printf("\nScript error: movie_get_channel_index(void playback). Playback is invalid.\n");
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)channel;
    return S_OK;
}

HRESULT openbor_movie_get_channel_mask(
    ScriptVariant **varlist,
    ScriptVariant **pretvar,
    int paramCount
)
{
    (void)varlist;
    (void)paramCount;
    ScriptVariant_ChangeType(*pretvar, VT_UINTEGER64);
    (*pretvar)->ullVal = movie_playback_get_active_mask();
    return S_OK;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Read one synchronized movie playback property or dump its
* complete public property map for diagnostics.
*/
HRESULT openbor_movie_get_property(
    ScriptVariant **varlist,
    ScriptVariant **pretvar,
    int paramCount
)
{
    s_movie_playback snapshot;
    int property_index;
    s_property_access_map property_map;

    ScriptVariant_Clear(*pretvar);
    if(paramCount < 2 ||
       varlist[0]->vt != VT_PTR ||
       !varlist[0]->ptrVal ||
       varlist[1]->vt != VT_INTEGER ||
       !movie_playback_get_snapshot(varlist[0]->ptrVal, &snapshot)) {
        goto error;
    }

    property_index = (int)varlist[1]->lVal;
    if(property_index >= 0 && property_index < MOVIE_PROPERTY_END) {
        property_map = movie_get_property_map(
            &snapshot,
            (unsigned int)property_index
        );
        return property_access_get_member(&property_map, *pretvar);
    }
    if(property_index == PROPERTY_ACCESS_DUMP) {
        property_access_dump_members(
            movie_get_property_map,
            MOVIE_PROPERTY_END,
            &snapshot
        );
        return S_OK;
    }

error:
    printf("\nScript error: movie_get_property(void playback, int property). Playback or property is invalid.\n");
    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-08-12
*
* Route writable properties through lifecycle-aware helpers so
* clocks, decoding, scaling, and composition remain synchronized.
*/
HRESULT openbor_movie_set_property(
    ScriptVariant **varlist,
    ScriptVariant **pretvar,
    int paramCount
)
{
    s_movie_playback *playback;
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
       movie_playback_get_index(varlist[0]->ptrVal) < 0) {
        goto error_object;
    }

    playback = varlist[0]->ptrVal;
    property_index = (int)varlist[1]->lVal;
    switch((e_movie_properties)property_index) {
        case MOVIE_PROPERTY_BLACK_FILTER:
            result = SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &integer_value)) &&
                movie_playback_set_black_filter(playback, integer_value != 0);
            break;
        case MOVIE_PROPERTY_HEIGHT:
            result = SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &integer_value)) &&
                movie_playback_set_height(playback, (int)integer_value);
            break;
        case MOVIE_PROPERTY_INTERRUPT:
            result = SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &integer_value)) &&
                movie_playback_set_interrupt(playback, integer_value != 0);
            break;
        case MOVIE_PROPERTY_OFFSET_X:
            result = SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &integer_value)) &&
                movie_playback_set_offset_x(playback, (int)integer_value);
            break;
        case MOVIE_PROPERTY_OFFSET_Y:
            result = SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &integer_value)) &&
                movie_playback_set_offset_y(playback, (int)integer_value);
            break;
        case MOVIE_PROPERTY_PAUSED:
            result = SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &integer_value)) &&
                movie_playback_set_paused(playback, integer_value != 0);
            break;
        case MOVIE_PROPERTY_POSITION:
            result = SUCCEEDED(ScriptVariant_Unsigned64Value(varlist[2], &unsigned_value)) &&
                movie_playback_set_position(playback, unsigned_value, savedata.musicvol);
            break;
        case MOVIE_PROPERTY_REPEAT:
            result = SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &integer_value)) &&
                movie_playback_set_repeat(playback, integer_value != 0);
            break;
        case MOVIE_PROPERTY_SPEED:
            result = SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &decimal_value)) &&
                movie_playback_set_speed(playback, decimal_value);
            break;
        case MOVIE_PROPERTY_WIDTH:
            result = SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &integer_value)) &&
                movie_playback_set_width(playback, (int)integer_value);
            break;
        case MOVIE_PROPERTY_ACTIVE:
        case MOVIE_PROPERTY_CHANNEL:
        case MOVIE_PROPERTY_DURATION:
        case MOVIE_PROPERTY_SOUND_CHANNEL:
        case MOVIE_PROPERTY_SOURCE:
        case MOVIE_PROPERTY_END:
        default:
            printf("\nScript error: movie_set_property(void playback, int property, <mixed> value). Property id %d is read-only or unknown.\n",
                property_index);
            return E_FAIL;
    }

    if(result) {
        return S_OK;
    }
    printf("\nScript error: movie_set_property(void playback, int property, <mixed> value). Value is invalid for property id %d.\n",
        property_index);
    return E_FAIL;

error_object:
    printf("\nScript error: movie_set_property(void playback, int property, <mixed> value). Playback or property is invalid.\n");
    return E_FAIL;
}

#else

/*
* Caskey, Damon V.
* 2026-08-12
*
* Keep the generic movie API visible on decoder-free builds
* while reporting that playback is unavailable.
*/
static HRESULT openbor_movie_unsupported(ScriptVariant **pretvar)
{
    ScriptVariant_Clear(*pretvar);
    printf("Movie playback is not supported on this platform.\n");
    return S_OK;
}

#define MOVIE_UNSUPPORTED(name) \
    HRESULT name(ScriptVariant **v, ScriptVariant **r, int c) \
    { (void)v; (void)c; return openbor_movie_unsupported(r); }

MOVIE_UNSUPPORTED(openbor_movie_load)
MOVIE_UNSUPPORTED(openbor_movie_unload)
MOVIE_UNSUPPORTED(openbor_movie_play)
MOVIE_UNSUPPORTED(openbor_movie_draw_to_screen)
MOVIE_UNSUPPORTED(openbor_movie_set_sound_channel)
MOVIE_UNSUPPORTED(openbor_movie_stop)
MOVIE_UNSUPPORTED(openbor_movie_get_channel_object)
MOVIE_UNSUPPORTED(openbor_movie_get_channel_index)
MOVIE_UNSUPPORTED(openbor_movie_get_channel_mask)
MOVIE_UNSUPPORTED(openbor_movie_get_property)
MOVIE_UNSUPPORTED(openbor_movie_set_property)

#undef MOVIE_UNSUPPORTED

#endif
