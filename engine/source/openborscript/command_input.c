/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) OpenBOR Team
 */

#include "scriptcommon.h"

/*
* Caskey, Damon V.
* 2026-07-19
*
* Build property access metadata for an individual
* command input-history event.
*/
static const s_property_access_map command_input_event_get_property_map(
    const void* acting_object_param,
    const unsigned int property_index_param
) {
    const s_command_input_event* acting_object = acting_object_param;
    const e_command_input_event_properties property_index =
        property_index_param;
    s_property_access_map property_map;

    switch(property_index) {
        case COMMAND_INPUT_EVENT_PROPERTY_HELD:
            property_map.config_flags =
                PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
            property_map.field = &acting_object->held;
            property_map.id_string =
                "COMMAND_INPUT_EVENT_PROPERTY_HELD";
            property_map.type = VT_UINTEGER64;
            break;

        case COMMAND_INPUT_EVENT_PROPERTY_HOLD:
            property_map.config_flags =
                PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
            property_map.field = &acting_object->hold;
            property_map.id_string =
                "COMMAND_INPUT_EVENT_PROPERTY_HOLD";
            property_map.type = VT_UINTEGER64;
            break;

        case COMMAND_INPUT_EVENT_PROPERTY_PRESS:
            property_map.config_flags =
                PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
            property_map.field = &acting_object->press;
            property_map.id_string =
                "COMMAND_INPUT_EVENT_PROPERTY_PRESS";
            property_map.type = VT_UINTEGER64;
            break;

        case COMMAND_INPUT_EVENT_PROPERTY_PRESS_CHORD:
            property_map.config_flags =
                PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
            property_map.field = &acting_object->press_chord;
            property_map.id_string =
                "COMMAND_INPUT_EVENT_PROPERTY_PRESS_CHORD";
            property_map.type = VT_UINTEGER64;
            break;

        case COMMAND_INPUT_EVENT_PROPERTY_RELEASE:
            property_map.config_flags =
                PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
            property_map.field = &acting_object->release;
            property_map.id_string =
                "COMMAND_INPUT_EVENT_PROPERTY_RELEASE";
            property_map.type = VT_UINTEGER64;
            break;

        case COMMAND_INPUT_EVENT_PROPERTY_TICKS:
            property_map.config_flags =
                PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
            property_map.field = &acting_object->ticks;
            property_map.id_string =
                "COMMAND_INPUT_EVENT_PROPERTY_TICKS";
            property_map.type = VT_UINTEGER64;
            break;

        case COMMAND_INPUT_EVENT_PROPERTY_TIME:
            property_map.config_flags =
                PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
            property_map.field = &acting_object->time;
            property_map.id_string =
                "COMMAND_INPUT_EVENT_PROPERTY_TIME";
            property_map.type = VT_UINTEGER64;
            break;

        case COMMAND_INPUT_EVENT_PROPERTY_END:
        default:
            property_map.config_flags = PROPERTY_ACCESS_CONFIG_NONE;
            property_map.field = NULL;
            property_map.id_string = "Command Input Event";
            property_map.type = VT_EMPTY;
            break;
    }

    return property_map;
}

/*
* Caskey, Damon V.
* 2026-07-19
*
* Build property access metadata for an individual
* configurable command input step.
*/
static const s_property_access_map command_input_step_get_property_map(
    const void* acting_object_param,
    const unsigned int property_index_param
) {
    const s_command_input_step* acting_object = acting_object_param;
    const e_command_input_step_properties property_index =
        property_index_param;
    s_property_access_map property_map;

    switch(property_index) {
        case COMMAND_INPUT_STEP_PROPERTY_CHORD_TIME:
            property_map.config_flags =
                PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
            property_map.field = &acting_object->chord_time;
            property_map.id_string =
                "COMMAND_INPUT_STEP_PROPERTY_CHORD_TIME";
            property_map.type = VT_UINTEGER64;
            break;

        case COMMAND_INPUT_STEP_PROPERTY_HOLD:
            property_map.config_flags =
                PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
            property_map.field = &acting_object->hold;
            property_map.id_string =
                "COMMAND_INPUT_STEP_PROPERTY_HOLD";
            property_map.type = VT_UINTEGER64;
            break;

        case COMMAND_INPUT_STEP_PROPERTY_HOLD_TIME:
            property_map.config_flags =
                PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
            property_map.field = &acting_object->hold_time;
            property_map.id_string =
                "COMMAND_INPUT_STEP_PROPERTY_HOLD_TIME";
            property_map.type = VT_UINTEGER64;
            break;

        case COMMAND_INPUT_STEP_PROPERTY_HOLD_TIME_MAXIMUM:
            property_map.config_flags =
                PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
            property_map.field = &acting_object->hold_time_maximum;
            property_map.id_string =
                "COMMAND_INPUT_STEP_PROPERTY_HOLD_TIME_MAXIMUM";
            property_map.type = VT_UINTEGER64;
            break;

        case COMMAND_INPUT_STEP_PROPERTY_HOLD_TRIGGER:
            property_map.config_flags =
                PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
            property_map.field = &acting_object->hold_trigger;
            property_map.id_string =
                "COMMAND_INPUT_STEP_PROPERTY_HOLD_TRIGGER";
            property_map.type = VT_UINTEGER64;
            break;

        case COMMAND_INPUT_STEP_PROPERTY_PRESS:
            property_map.config_flags =
                PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
            property_map.field = &acting_object->press;
            property_map.id_string =
                "COMMAND_INPUT_STEP_PROPERTY_PRESS";
            property_map.type = VT_UINTEGER64;
            break;

        case COMMAND_INPUT_STEP_PROPERTY_RELEASE:
            property_map.config_flags =
                PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
            property_map.field = &acting_object->release;
            property_map.id_string =
                "COMMAND_INPUT_STEP_PROPERTY_RELEASE";
            property_map.type = VT_UINTEGER64;
            break;

        case COMMAND_INPUT_STEP_PROPERTY_END:
        default:
            property_map.config_flags = PROPERTY_ACCESS_CONFIG_NONE;
            property_map.field = NULL;
            property_map.id_string = "Command Input Step";
            property_map.type = VT_EMPTY;
            break;
    }

    return property_map;
}

/*
* Caskey, Damon V.
* 2026-07-19
*
* Return an indexed command input event from an
* s_command_input_event[MAX_SPECIAL_INPUTS] collection.
*
* get_command_input_event_object(void collection, int index)
*/
HRESULT openbor_get_command_input_event_object(
    ScriptVariant** varlist,
    ScriptVariant** pretvar,
    const int paramCount
) {
    const char* self_name =
        "get_command_input_event_object(void collection, int index)";
    const int argument_collection = 0;
    const int argument_index = 1;
    const int argument_minimum = 2;

    s_command_input_event* collection;
    LONG index;

    ScriptVariant_Clear(*pretvar);

    if(paramCount < argument_minimum
        || varlist[argument_index]->vt != VT_INTEGER
        || (varlist[argument_collection]->vt != VT_PTR
            && varlist[argument_collection]->vt != VT_EMPTY)) {
        goto error_local;
    }

    /*
    * An empty collection is a valid result from an
    * upstream property accessor. Propagate empty rather
    * than treating the absent owner as an index error.
    */
    if(varlist[argument_collection]->vt == VT_EMPTY
        || !varlist[argument_collection]->ptrVal) {
        return S_OK;
    }

    index = varlist[argument_index]->lVal;

    if(index < 0 || index >= MAX_SPECIAL_INPUTS) {
        goto error_local;
    }

    collection = (s_command_input_event*)
        varlist[argument_collection]->ptrVal;

    ScriptVariant_ChangeType(*pretvar, VT_PTR);
    (*pretvar)->ptrVal = (VOID*)&collection[index];

    return S_OK;

error_local:
    printf(
        "\nScript error: %s. You must provide a valid command "
        "input event collection and index from 0 through %d.\n",
        self_name,
        MAX_SPECIAL_INPUTS - 1
    );
    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-07-19
*
* Return a command input-event property.
*/
HRESULT openbor_get_command_input_event_property(
    ScriptVariant** varlist,
    ScriptVariant** pretvar,
    const int paramCount
) {
    const char* self_name =
        "get_command_input_event_property(void event, int property)";
    const int argument_object = 0;
    const int argument_property = 1;
    const int argument_minimum = 2;

    const s_command_input_event* acting_object;
    int property_index;
    s_property_access_map property_map;

    ScriptVariant_Clear(*pretvar);

    if(paramCount < argument_minimum
        || varlist[argument_object]->vt != VT_PTR
        || !varlist[argument_object]->ptrVal
        || varlist[argument_property]->vt != VT_INTEGER) {
        goto error_local;
    }

    acting_object = (const s_command_input_event*)
        varlist[argument_object]->ptrVal;
    property_index = (int)varlist[argument_property]->lVal;

    if(property_index >= 0
        && property_index < COMMAND_INPUT_EVENT_PROPERTY_END) {
        property_map = command_input_event_get_property_map(
            acting_object,
            (unsigned int)property_index
        );
        return property_access_get_member(
            &property_map,
            *pretvar
        );
    }

    if(property_index == PROPERTY_ACCESS_DUMP) {
        property_access_dump_members(
            command_input_event_get_property_map,
            COMMAND_INPUT_EVENT_PROPERTY_END,
            acting_object
        );
        return S_OK;
    }

error_local:
    printf(
        "\nScript error: %s. You must provide a valid command "
        "input event pointer and property id.\n",
        self_name
    );
    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-07-19
*
* Mutate a command input-event property.
*/
HRESULT openbor_set_command_input_event_property(
    ScriptVariant** varlist,
    ScriptVariant** pretvar,
    const int paramCount
) {
    const char* self_name =
        "set_command_input_event_property(void event, int property, value)";
    const int argument_object = 0;
    const int argument_property = 1;
    const int argument_value = 2;
    const int argument_minimum = 3;

    s_command_input_event* acting_object;
    int property_index;
    s_property_access_map property_map;

    if(paramCount < argument_minimum
        || varlist[argument_object]->vt != VT_PTR
        || !varlist[argument_object]->ptrVal
        || varlist[argument_property]->vt != VT_INTEGER) {
        goto error_local;
    }

    acting_object = (s_command_input_event*)
        varlist[argument_object]->ptrVal;
    property_index = (int)varlist[argument_property]->lVal;

    if(property_index < 0
        || property_index >= COMMAND_INPUT_EVENT_PROPERTY_END) {
        goto error_local;
    }

    property_map = command_input_event_get_property_map(
        acting_object,
        (unsigned int)property_index
    );

    return property_access_set_member(
        acting_object,
        &property_map,
        varlist[argument_value]
    );

error_local:
    printf(
        "\nScript error: %s. You must provide a valid command "
        "input event pointer, property id, and value.\n",
        self_name
    );
    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-07-19
*
* Return an indexed command input step from an
* s_command_input_step[MAX_SPECIAL_INPUTS] collection.
*
* get_command_input_step_object(void collection, int index)
*/
HRESULT openbor_get_command_input_step_object(
    ScriptVariant** varlist,
    ScriptVariant** pretvar,
    const int paramCount
) {
    const char* self_name =
        "get_command_input_step_object(void collection, int index)";
    const int argument_collection = 0;
    const int argument_index = 1;
    const int argument_minimum = 2;

    s_command_input_step* collection;
    LONG index;

    ScriptVariant_Clear(*pretvar);

    if(paramCount < argument_minimum
        || varlist[argument_index]->vt != VT_INTEGER
        || (varlist[argument_collection]->vt != VT_PTR
            && varlist[argument_collection]->vt != VT_EMPTY)) {
        goto error_local;
    }

    /*
    * An empty collection is a valid result from an
    * upstream property accessor. Propagate empty rather
    * than treating the absent owner as an index error.
    */
    if(varlist[argument_collection]->vt == VT_EMPTY
        || !varlist[argument_collection]->ptrVal) {
        return S_OK;
    }

    index = varlist[argument_index]->lVal;

    if(index < 0 || index >= MAX_SPECIAL_INPUTS) {
        goto error_local;
    }

    collection = (s_command_input_step*)
        varlist[argument_collection]->ptrVal;

    ScriptVariant_ChangeType(*pretvar, VT_PTR);
    (*pretvar)->ptrVal = (VOID*)&collection[index];

    return S_OK;

error_local:
    printf(
        "\nScript error: %s. You must provide a valid command "
        "input step collection and index from 0 through %d.\n",
        self_name,
        MAX_SPECIAL_INPUTS - 1
    );
    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-07-19
*
* Return a command input-step property.
*/
HRESULT openbor_get_command_input_step_property(
    ScriptVariant** varlist,
    ScriptVariant** pretvar,
    const int paramCount
) {
    const char* self_name =
        "get_command_input_step_property(void step, int property)";
    const int argument_object = 0;
    const int argument_property = 1;
    const int argument_minimum = 2;

    const s_command_input_step* acting_object;
    int property_index;
    s_property_access_map property_map;

    ScriptVariant_Clear(*pretvar);

    if(paramCount < argument_minimum
        || varlist[argument_object]->vt != VT_PTR
        || !varlist[argument_object]->ptrVal
        || varlist[argument_property]->vt != VT_INTEGER) {
        goto error_local;
    }

    acting_object = (const s_command_input_step*)
        varlist[argument_object]->ptrVal;
    property_index = (int)varlist[argument_property]->lVal;

    if(property_index >= 0
        && property_index < COMMAND_INPUT_STEP_PROPERTY_END) {
        property_map = command_input_step_get_property_map(
            acting_object,
            (unsigned int)property_index
        );
        return property_access_get_member(
            &property_map,
            *pretvar
        );
    }

    if(property_index == PROPERTY_ACCESS_DUMP) {
        property_access_dump_members(
            command_input_step_get_property_map,
            COMMAND_INPUT_STEP_PROPERTY_END,
            acting_object
        );
        return S_OK;
    }

error_local:
    printf(
        "\nScript error: %s. You must provide a valid command "
        "input step pointer and property id.\n",
        self_name
    );
    *pretvar = NULL;
    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2026-07-19
*
* Mutate a command input-step property.
*/
HRESULT openbor_set_command_input_step_property(
    ScriptVariant** varlist,
    ScriptVariant** pretvar,
    const int paramCount
) {
    const char* self_name =
        "set_command_input_step_property(void step, int property, value)";
    const int argument_object = 0;
    const int argument_property = 1;
    const int argument_value = 2;
    const int argument_minimum = 3;

    s_command_input_step* acting_object;
    int property_index;
    s_property_access_map property_map;

    if(paramCount < argument_minimum
        || varlist[argument_object]->vt != VT_PTR
        || !varlist[argument_object]->ptrVal
        || varlist[argument_property]->vt != VT_INTEGER) {
        goto error_local;
    }

    acting_object = (s_command_input_step*)
        varlist[argument_object]->ptrVal;
    property_index = (int)varlist[argument_property]->lVal;

    if(property_index < 0
        || property_index >= COMMAND_INPUT_STEP_PROPERTY_END) {
        goto error_local;
    }

    property_map = command_input_step_get_property_map(
        acting_object,
        (unsigned int)property_index
    );

    return property_access_set_member(
        acting_object,
        &property_map,
        varlist[argument_value]
    );

error_local:
    printf(
        "\nScript error: %s. You must provide a valid command "
        "input step pointer, property id, and value.\n",
        self_name
    );
    *pretvar = NULL;
    return E_FAIL;
}
