/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c)  OpenBOR Team
 */

 // Drawmethod Properties
 // 2019-03-28
 // Caskey, Damon V.

#include <limits.h>

#include "scriptcommon.h"

typedef struct s_sound_property_info {
    e_sound_properties property;
    e_property_access_config_flags config_flags;
    size_t offset;
    const char *id_string;
    VARTYPE type;
} s_sound_property_info;

typedef struct s_sound_property_dump_view {
    channelstruct channel;
    uint64_t priority;
} s_sound_property_dump_view;

#define PROPERTY_MEMBER_OFFSET(type, member) ((size_t)&(((type*)0)->member))

static const s_sound_property_info sound_properties[] = {
    {.property = SOUND_PROPERTY_ACTIVE,
     .id_string = "SOUND_PROPERTY_ACTIVE",
     .config_flags = PROPERTY_ACCESS_CONFIG_READ,
     .offset = PROPERTY_MEMBER_OFFSET(channelstruct, active),
     .type = VT_INTEGER },

    {.property = SOUND_PROPERTY_CHANNEL,
     .id_string = "SOUND_PROPERTY_CHANNEL",
     .config_flags = PROPERTY_ACCESS_CONFIG_READ,
     .offset = PROPERTY_MEMBER_OFFSET(channelstruct, index),
     .type = VT_INTEGER },

    {.property = SOUND_PROPERTY_CHANNELS,
     .id_string = "SOUND_PROPERTY_CHANNELS",
     .config_flags = PROPERTY_ACCESS_CONFIG_READ,
     .offset = PROPERTY_MEMBER_OFFSET(channelstruct, channels),
     .type = VT_INTEGER },

    {.property = SOUND_PROPERTY_LOOP_OFFSET,
     .id_string = "SOUND_PROPERTY_LOOP_OFFSET",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(channelstruct, fp_loop_start),
     .type = VT_UINTEGER64 },

    {.property = SOUND_PROPERTY_PAUSED,
     .id_string = "SOUND_PROPERTY_PAUSED",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(channelstruct, paused),
     .type = VT_INTEGER },

    {.property = SOUND_PROPERTY_PERIOD,
     .id_string = "SOUND_PROPERTY_PERIOD",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(channelstruct, fp_period),
     .type = VT_UINTEGER64 },

    {.property = SOUND_PROPERTY_PLAY_ID,
     .id_string = "SOUND_PROPERTY_PLAY_ID",
     .config_flags = PROPERTY_ACCESS_CONFIG_READ,
     .offset = PROPERTY_MEMBER_OFFSET(channelstruct, playid),
     .type = VT_INTEGER },

    {.property = SOUND_PROPERTY_PRIORITY,
     .id_string = "SOUND_PROPERTY_PRIORITY",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(channelstruct, priority),
     .type = VT_INTEGER },

    {.property = SOUND_PROPERTY_SAMPLE,
     .id_string = "SOUND_PROPERTY_SAMPLE",
     .config_flags = PROPERTY_ACCESS_CONFIG_READ,
     .offset = PROPERTY_MEMBER_OFFSET(channelstruct, samplenum),
     .type = VT_INTEGER },

    {.property = SOUND_PROPERTY_SAMPLE_POSITION,
     .id_string = "SOUND_PROPERTY_SAMPLE_POSITION",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(channelstruct, fp_samplepos),
     .type = VT_UINTEGER64 },

    {.property = SOUND_PROPERTY_VOLUME_DIVISOR,
     .id_string = "SOUND_PROPERTY_VOLUME_DIVISOR",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(channelstruct, volume_divisor),
     .type = VT_INTEGER },

    {.property = SOUND_PROPERTY_VOLUME_LEFT,
     .id_string = "SOUND_PROPERTY_VOLUME_LEFT",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(channelstruct, volume[SOUND_SPATIAL_CHANNEL_LEFT]),
     .type = VT_INTEGER },

    {.property = SOUND_PROPERTY_VOLUME_RIGHT,
     .id_string = "SOUND_PROPERTY_VOLUME_RIGHT",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(channelstruct, volume[SOUND_SPATIAL_CHANNEL_RIGHT]),
     .type = VT_INTEGER },

    {.property = SOUND_PROPERTY_GROUP,
     .id_string = "SOUND_PROPERTY_GROUP",
     .config_flags = PROPERTY_ACCESS_CONFIG_READ,
     .offset = PROPERTY_MEMBER_OFFSET(channelstruct, group),
     .type = VT_UINTEGER64 },

    {.property = SOUND_PROPERTY_OWNER_ID,
     .id_string = "SOUND_PROPERTY_OWNER_ID",
     .config_flags = PROPERTY_ACCESS_CONFIG_READ,
     .offset = PROPERTY_MEMBER_OFFSET(channelstruct, owner_id),
     .type = VT_UINTEGER64 },

    {.property = SOUND_PROPERTY_END,
     .id_string = "Sound",
     .config_flags = PROPERTY_ACCESS_CONFIG_NONE,
     .offset = 0,
     .type = VT_EMPTY }
};

#undef PROPERTY_MEMBER_OFFSET

/*
* Caskey, Damon V.
* 2026-08-02
*
* Build property access metadata for a stable
* sound channel object.
*/
static const s_property_access_map sound_get_property_map(
    const void *acting_object_param,
    const unsigned int property_index_param
) {
    s_property_access_map property_map = { 0 };
    const channelstruct *acting_object = acting_object_param;
    const s_sound_property_info *info = NULL;
    size_t property_cursor;

    for(property_cursor = 0;
        property_cursor < sizeof(sound_properties) / sizeof(sound_properties[0]);
        property_cursor++) {
        if(sound_properties[property_cursor].property == property_index_param) {
            info = &sound_properties[property_cursor];
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
        property_map.id_string = "Sound";
        property_map.type = VT_EMPTY;
    }

    return property_map;
}

/*
* Caskey, Damon V.
* 2026-08-06
*
* Adapt normalized sound values to the generic
* property diagnostic. Priority requires dedicated
* unsigned storage because its channel field is 32-bit.
*/
static const s_property_access_map sound_get_dump_property_map(
    const void *acting_object_param,
    const unsigned int property_index_param
) {
    const s_sound_property_dump_view *acting_object = acting_object_param;
    s_property_access_map property_map = sound_get_property_map(
        &acting_object->channel,
        property_index_param
    );

    if(property_index_param == SOUND_PROPERTY_PRIORITY) {
        property_map.field = &acting_object->priority;
        property_map.type = VT_UINTEGER64;
    }

    return property_map;
}

/*
* Caskey, Damon V.
* 2026-08-02
*
* Return a pool-level bank mask selected by its
* SOUND_CHANNEL_BANK_MASK_* constant.
*/
HRESULT openbor_get_sound_channel_bank_mask(
    ScriptVariant** varlist,
    ScriptVariant** pretvar,
    const int paramCount
) {
    const char *self_name = "get_sound_channel_bank_mask(int mask)";
    const int argument_mask = 0;
    const int argument_minimum = 1;
    LONG mask;

    ScriptVariant_Clear(*pretvar);

    if(paramCount < argument_minimum ||
       FAILED(ScriptVariant_IntegerValue(varlist[argument_mask], &mask)) ||
       mask < 0 || mask >= SOUND_CHANNEL_BANK_MASK_END) {
        printf("\nScript error: %s. You must provide a valid bank mask id.\n", self_name);
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_ChangeType(*pretvar, VT_UINTEGER64);
    (*pretvar)->ullVal = sound_get_channel_bank_mask((e_sound_channel_bank_mask)mask);
    return S_OK;
}

/*
* Caskey, Damon V.
* 2026-08-02
*
* Return one 64-channel bank state mask selected by
* its SOUND_CHANNEL_MASK_* constant.
*/
HRESULT openbor_get_sound_channel_mask(
    ScriptVariant** varlist,
    ScriptVariant** pretvar,
    const int paramCount
) {
    const char *self_name = "get_sound_channel_mask(int bank, int mask)";
    const int argument_bank = 0;
    const int argument_mask = 1;
    const int argument_minimum = 2;
    LONG bank;
    LONG mask;

    ScriptVariant_Clear(*pretvar);

    if(paramCount < argument_minimum ||
       FAILED(ScriptVariant_IntegerValue(varlist[argument_bank], &bank)) ||
       FAILED(ScriptVariant_IntegerValue(varlist[argument_mask], &mask)) ||
       bank < 0 || bank >= SOUND_CHANNEL_BANK_COUNT ||
       mask < 0 || mask >= SOUND_CHANNEL_MASK_END) {
        printf("\nScript error: %s. You must provide a valid bank and channel mask id.\n", self_name);
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_ChangeType(*pretvar, VT_UINTEGER64);
    (*pretvar)->ullVal = sound_get_channel_mask(
        (unsigned int)bank,
        (e_sound_channel_mask)mask
    );
    return S_OK;
}

/*
* Caskey, Damon V.
* 2026-08-02
*
* Return the stable sound object at a flattened
* channel index. Unallocated channels return empty.
*/
HRESULT openbor_get_sound_channel_object(
    ScriptVariant** varlist,
    ScriptVariant** pretvar,
    const int paramCount
) {
    const char *self_name = "get_sound_channel_object(int channel)";
    const int argument_channel = 0;
    const int argument_minimum = 1;
    channelstruct *sound_object;
    LONG channel;

    ScriptVariant_Clear(*pretvar);

    if(paramCount < argument_minimum ||
       FAILED(ScriptVariant_IntegerValue(varlist[argument_channel], &channel)) ||
       channel < 0 || channel >= SOUND_CHANNEL_COUNT_MAX) {
        printf("\nScript error: %s. Channel must be from 0 through %u.\n",
            self_name,
            SOUND_CHANNEL_COUNT_MAX - 1U);
        *pretvar = NULL;
        return E_FAIL;
    }

    sound_object = sound_get_channel_object((int)channel);
    if(sound_object) {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = (VOID*)sound_object;
    }

    return S_OK;
}

/*
* Caskey, Damon V.
* 2026-08-02
*
* Resolve a sound object pointer to its flattened
* channel index.
*/
HRESULT openbor_get_sound_channel_index(
    ScriptVariant** varlist,
    ScriptVariant** pretvar,
    const int paramCount
) {
    const char *self_name = "get_sound_channel_index(void sound)";
    const int argument_object = 0;
    const int argument_minimum = 1;
    const channelstruct *sound_object;
    int channel;

    ScriptVariant_Clear(*pretvar);

    if(paramCount < argument_minimum ||
       varlist[argument_object]->vt != VT_PTR ||
       !varlist[argument_object]->ptrVal) {
        goto error_local;
    }

    sound_object = (const channelstruct*)varlist[argument_object]->ptrVal;
    channel = sound_get_channel_index(sound_object);
    if(channel < 0) {
        goto error_local;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)channel;
    return S_OK;

error_local:
    printf("\nScript error: %s. You must provide a valid sound object.\n", self_name);
    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-08-02
*
* Return a sound object property using the shared
* property lookup architecture.
*/
HRESULT openbor_get_sound_property(
    const ScriptVariant* const* varlist,
    ScriptVariant** const pretvar,
    const int paramCount
) {
    const char *self_name = "get_sound_property(void sound, int property)";
    const int argument_object = 0;
    const int argument_property = 1;
    const int argument_minimum = 2;
    const channelstruct *sound_object;
    channelstruct sound_snapshot;
    int property_index;
    s_property_access_map property_map;

    ScriptVariant_Clear(*pretvar);

    if(paramCount < argument_minimum ||
       varlist[argument_object]->vt != VT_PTR ||
       !varlist[argument_object]->ptrVal ||
       varlist[argument_property]->vt != VT_INTEGER) {
        goto error_object;
    }

    sound_object = (const channelstruct*)varlist[argument_object]->ptrVal;
    if(!sound_get_channel_snapshot(sound_object, &sound_snapshot)) {
        goto error_object;
    }

    property_index = (int)varlist[argument_property]->lVal;
    if(property_index >= 0 && property_index < SOUND_PROPERTY_END) {
        if(property_index == SOUND_PROPERTY_LOOP_OFFSET) {
            ScriptVariant_ChangeType(*pretvar, VT_UINTEGER64);
            (*pretvar)->ullVal = SOUND_SAMPLE_FIX_TO_INT(sound_snapshot.fp_loop_start);
            return S_OK;
        }
        if(property_index == SOUND_PROPERTY_PRIORITY) {
            ScriptVariant_ChangeType(*pretvar, VT_UINTEGER64);
            (*pretvar)->ullVal = (uint64_t)sound_snapshot.priority;
            return S_OK;
        }
        if(property_index == SOUND_PROPERTY_SAMPLE_POSITION) {
            ScriptVariant_ChangeType(*pretvar, VT_UINTEGER64);
            (*pretvar)->ullVal = SOUND_SAMPLE_FIX_TO_INT(sound_snapshot.fp_samplepos);
            return S_OK;
        }

        property_map = sound_get_property_map(&sound_snapshot, (unsigned int)property_index);
        return property_access_get_member(&property_map, *pretvar);
    }

    if(property_index == PROPERTY_ACCESS_DUMP) {
        s_sound_property_dump_view dump_view;

        dump_view.channel = sound_snapshot;
        dump_view.channel.fp_loop_start = SOUND_SAMPLE_FIX_TO_INT(
            sound_snapshot.fp_loop_start
        );
        dump_view.channel.fp_samplepos = SOUND_SAMPLE_FIX_TO_INT(
            sound_snapshot.fp_samplepos
        );
        dump_view.priority = (uint64_t)sound_snapshot.priority;

        property_access_dump_members(
            sound_get_dump_property_map,
            SOUND_PROPERTY_END,
            &dump_view
        );
        return S_OK;
    }

    printf("\nScript error: %s. Unknown property id (%d).\n", self_name, property_index);
    *pretvar = NULL;
    return E_FAIL;

error_object:
    printf("\nScript error: %s. You must provide a valid sound object and property id.\n", self_name);
    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-08-02
*
* Mutate a sound object property while routing
* mask-backed and playback-sensitive fields through
* their synchronization helpers.
*/
HRESULT openbor_set_sound_property(
    ScriptVariant** varlist,
    ScriptVariant** const pretvar,
    const int paramCount
) {
    const char *self_name = "set_sound_property(void sound, int property, <mixed> value)";
    const int argument_object = 0;
    const int argument_property = 1;
    const int argument_value = 2;
    const int argument_minimum = 3;
    channelstruct *sound_object;
    int channel;
    int property_index;
    LONG integer_value;
    uint64_t unsigned_value;
    s_property_access_map property_map;

    if(paramCount < argument_minimum ||
       varlist[argument_object]->vt != VT_PTR ||
       !varlist[argument_object]->ptrVal ||
       varlist[argument_property]->vt != VT_INTEGER) {
        goto error_object;
    }

    sound_object = (channelstruct*)varlist[argument_object]->ptrVal;
    channel = sound_get_channel_index(sound_object);
    if(channel < 0) {
        goto error_object;
    }

    property_index = (int)varlist[argument_property]->lVal;
    if(property_index < 0 || property_index >= SOUND_PROPERTY_END) {
        printf("\nScript error: %s. Unknown property id (%d).\n", self_name, property_index);
        *pretvar = NULL;
        return E_FAIL;
    }

    switch((e_sound_properties)property_index) {
        case SOUND_PROPERTY_LOOP_OFFSET:
            if(FAILED(ScriptVariant_Unsigned64Value(varlist[argument_value], &unsigned_value)) ||
               !sound_set_channel_loop_offset(channel, unsigned_value)) {
                goto error_value;
            }
            return S_OK;

        case SOUND_PROPERTY_PAUSED:
            if(FAILED(ScriptVariant_IntegerValue(varlist[argument_value], &integer_value))) {
                goto error_value;
            }
            sound_pause_single_sample(integer_value != 0, channel);
            return S_OK;

        case SOUND_PROPERTY_PERIOD:
            if(FAILED(ScriptVariant_Unsigned64Value(varlist[argument_value], &unsigned_value)) ||
               !sound_set_channel_period(channel, unsigned_value)) {
                goto error_value;
            }
            return S_OK;

        case SOUND_PROPERTY_PRIORITY:
            if(FAILED(ScriptVariant_Unsigned64Value(varlist[argument_value], &unsigned_value)) ||
               unsigned_value > (uint64_t)UINT_MAX ||
               !sound_set_channel_priority(channel, (unsigned int)unsigned_value)) {
                goto error_value;
            }
            return S_OK;

        case SOUND_PROPERTY_SAMPLE_POSITION:
            if(FAILED(ScriptVariant_Unsigned64Value(varlist[argument_value], &unsigned_value)) ||
               !sound_set_channel_position(channel, unsigned_value)) {
                goto error_value;
            }
            return S_OK;

        case SOUND_PROPERTY_VOLUME_DIVISOR:
            if(FAILED(ScriptVariant_IntegerValue(varlist[argument_value], &integer_value)) ||
               !sound_set_channel_volume_divisor(channel, (int)integer_value)) {
                goto error_value;
            }
            return S_OK;

        case SOUND_PROPERTY_VOLUME_LEFT:
            if(FAILED(ScriptVariant_IntegerValue(varlist[argument_value], &integer_value)) ||
               !sound_set_channel_volume(channel, SOUND_SPATIAL_CHANNEL_LEFT, (int)integer_value)) {
                goto error_value;
            }
            return S_OK;

        case SOUND_PROPERTY_VOLUME_RIGHT:
            if(FAILED(ScriptVariant_IntegerValue(varlist[argument_value], &integer_value)) ||
               !sound_set_channel_volume(channel, SOUND_SPATIAL_CHANNEL_RIGHT, (int)integer_value)) {
                goto error_value;
            }
            return S_OK;

        case SOUND_PROPERTY_ACTIVE:
        case SOUND_PROPERTY_CHANNEL:
        case SOUND_PROPERTY_CHANNELS:
        case SOUND_PROPERTY_PLAY_ID:
        case SOUND_PROPERTY_SAMPLE:
        case SOUND_PROPERTY_GROUP:
        case SOUND_PROPERTY_OWNER_ID:
        case SOUND_PROPERTY_END:
        default:
            property_map = sound_get_property_map(sound_object, (unsigned int)property_index);
            return property_access_set_member(sound_object, &property_map, varlist[argument_value]);
    }

error_value:
    printf("\nScript error: %s. Value is invalid for property id (%d).\n", self_name, property_index);
    *pretvar = NULL;
    return E_FAIL;

error_object:
    printf("\nScript error: %s. You must provide a valid sound object, property id, and value.\n", self_name);
    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-08-08
*
* Read and validate the group and owner filters shared by
* sound-group script operations.
*/
static bool sound_group_get_script_filter(
    ScriptVariant** varlist,
    const int paramCount,
    sound_group_mask_t* const group,
    uint64_t* const owner_id
) {
    if(!varlist || !group || !owner_id || paramCount < 2 ||
       FAILED(ScriptVariant_Unsigned64Value(varlist[0], group)) ||
       FAILED(ScriptVariant_Unsigned64Value(varlist[1], owner_id)) ||
       (*group & ~SOUND_GROUP_ALL)) {
        return false;
    }

    return true;
}

/*
* Caskey, Damon V.
* 2026-08-08
*
* Stop active channels sharing any requested group and
* matching an exact entity owner ID. ENTITY_UNIQUE_ID_ALL
* selects matching groups from every owner.
*/
HRESULT openbor_sound_group_stop(
    ScriptVariant** varlist,
    ScriptVariant** pretvar,
    const int paramCount
) {
    const char *self_name =
        "sound_group_stop(uint64 group, uint64 owner_id)";
    sound_group_mask_t group;
    uint64_t owner_id;

    if(!pretvar || !*pretvar ||
       !sound_group_get_script_filter(
           varlist,
           paramCount,
           &group,
           &owner_id
       )) {
        printf("\nScript error: %s. Invalid group or owner ID.\n", self_name);
        if(pretvar) {
            *pretvar = NULL;
        }
        return E_FAIL;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)sound_group_stop(group, owner_id);
    return S_OK;
}

/*
* Caskey, Damon V.
* 2026-08-08
*
* Apply a pause state to matching sound-group channels.
*/
static HRESULT openbor_sound_group_pause_state(
    ScriptVariant** varlist,
    ScriptVariant** pretvar,
    const int paramCount,
    const int toggle,
    const char* const self_name
) {
    sound_group_mask_t group;
    uint64_t owner_id;

    if(!pretvar || !*pretvar ||
       !sound_group_get_script_filter(
           varlist,
           paramCount,
           &group,
           &owner_id
       )) {
        printf("\nScript error: %s. Invalid group or owner ID.\n", self_name);
        if(pretvar) {
            *pretvar = NULL;
        }
        return E_FAIL;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)sound_group_pause(
        toggle,
        group,
        owner_id
    );
    return S_OK;
}

HRESULT openbor_sound_group_pause(
    ScriptVariant** varlist,
    ScriptVariant** pretvar,
    const int paramCount
) {
    return openbor_sound_group_pause_state(
        varlist,
        pretvar,
        paramCount,
        true,
        "sound_group_pause(uint64 group, uint64 owner_id)"
    );
}

HRESULT openbor_sound_group_resume(
    ScriptVariant** varlist,
    ScriptVariant** pretvar,
    const int paramCount
) {
    return openbor_sound_group_pause_state(
        varlist,
        pretvar,
        paramCount,
        false,
        "sound_group_resume(uint64 group, uint64 owner_id)"
    );
}

/*
* Caskey, Damon V.
* 2026-08-08
*
* Seek matching sound-group channels to a PCM frame.
*/
HRESULT openbor_sound_group_offset(
    ScriptVariant** varlist,
    ScriptVariant** pretvar,
    const int paramCount
) {
    const char *self_name =
        "sound_group_offset(uint64 group, uint64 owner_id, uint64 offset)";
    sound_group_mask_t group;
    uint64_t owner_id;
    uint64_t offset;

    if(!pretvar || !*pretvar || paramCount < 3 ||
       !sound_group_get_script_filter(
           varlist,
           paramCount,
           &group,
           &owner_id
       ) ||
       FAILED(ScriptVariant_Unsigned64Value(varlist[2], &offset))) {
        printf(
            "\nScript error: %s. Invalid group, owner ID, or offset.\n",
            self_name
        );
        if(pretvar) {
            *pretvar = NULL;
        }
        return E_FAIL;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)sound_group_set_position(
        group,
        owner_id,
        offset
    );
    return S_OK;
}
