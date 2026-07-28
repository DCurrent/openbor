/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) OpenBOR Team
 */

#include "scriptcommon.h"


typedef struct {
    e_faction_properties property;
    e_property_access_config_flags config_flags;
    size_t offset;
    const char* id_string;
    VARTYPE type;
} faction_property_info;

#define PROPERTY_MEMBER_OFFSET(type, member) ((size_t)&(((type*)0)->member))

static const faction_property_info faction_properties[] = {
    {.property = FACTION_PROPERTY_GROUP_DAMAGE_DIRECT,
     .id_string = "FACTION_PROPERTY_GROUP_DAMAGE_DIRECT",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_faction, damage_direct),
     .type = VT_UINTEGER64 },

    {.property = FACTION_PROPERTY_GROUP_DAMAGE_INDIRECT,
     .id_string = "FACTION_PROPERTY_GROUP_DAMAGE_INDIRECT",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_faction, damage_indirect),
     .type = VT_UINTEGER64 },

    {.property = FACTION_PROPERTY_GROUP_HOSTILE,
     .id_string = "FACTION_PROPERTY_GROUP_HOSTILE",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_faction, hostile),
     .type = VT_UINTEGER64 },

    {.property = FACTION_PROPERTY_GROUP_MEMBER,
     .id_string = "FACTION_PROPERTY_GROUP_MEMBER",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_faction, member),
     .type = VT_UINTEGER64 },

    {.property = FACTION_PROPERTY_TYPE_DAMAGE_DIRECT,
     .id_string = "FACTION_PROPERTY_TYPE_DAMAGE_DIRECT",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_faction, type_damage_direct),
     .type = VT_INTEGER },

    {.property = FACTION_PROPERTY_TYPE_DAMAGE_INDIRECT,
     .id_string = "FACTION_PROPERTY_TYPE_DAMAGE_INDIRECT",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_faction, type_damage_indirect),
     .type = VT_INTEGER },

    {.property = FACTION_PROPERTY_TYPE_HOSTILE,
     .id_string = "FACTION_PROPERTY_TYPE_HOSTILE",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_faction, type_hostile),
     .type = VT_INTEGER },

    {.property = FACTION_PROPERTY_END,
     .id_string = "Faction",
     .config_flags = PROPERTY_ACCESS_CONFIG_NONE,
     .offset = 0,
     .type = VT_EMPTY }
};

#undef PROPERTY_MEMBER_OFFSET

const s_property_access_map faction_get_property_map(const void* acting_object_param, const unsigned int property_index_param)
{
    s_property_access_map property_map = { 0 };
    const s_faction* acting_object = acting_object_param;
    const faction_property_info* info = NULL;

    for (size_t i = 0; i < sizeof(faction_properties) / sizeof(faction_properties[0]); ++i) {
        if (faction_properties[i].property == property_index_param) {
            info = &faction_properties[i];
            break;
        }
    }

    if (info) {
        property_map.config_flags = info->config_flags;
        property_map.field = (const void*)((const char*)acting_object + info->offset);
        property_map.id_string = info->id_string;
        property_map.type = info->type;
    }
    else {
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_NONE;
        property_map.field = NULL;
        property_map.id_string = "Faction";
        property_map.type = VT_EMPTY;
    }

    return property_map;
}

/*
* Caskey, Damon V.
* 2023-02-23
*
* Return a faction property. Requires
* a faction pointer and property constant.
*/
HRESULT openbor_get_faction_property(const ScriptVariant* const* varlist, ScriptVariant** const pretvar, const int paramCount)
{
    const char* SELF_NAME = "openbor_get_faction_property(void faction, int property)";
    const unsigned int ARG_OBJECT = 0;
    const unsigned int ARG_PROPERTY = 1;
    const int ARG_MINIMUM = 2;

    /*
    * Clear pass by reference argument used to send
    * property data back to calling script.
    */
    ScriptVariant_Clear(*pretvar);

    /*
    * Should at least be a pointer to the
    * acting object and a property id.
    */
    if (paramCount < ARG_MINIMUM
        || varlist[ARG_OBJECT]->vt != VT_PTR
        || varlist[ARG_OBJECT]->ptrVal == NULL
        || varlist[ARG_PROPERTY]->vt != VT_INTEGER) {
        printf("\n\nScript error: %s. You must provide a valid object pointer and property id.\n\n", SELF_NAME);
        *pretvar = NULL;
        return E_FAIL;
    }

    const s_faction* const acting_object = (const s_faction* const)varlist[ARG_OBJECT]->ptrVal;

    if (acting_object->object_type != OBJECT_TYPE_FACTION) {
        printf("\n\nScript error: %s. Object pointer is not correct type.\n\n", SELF_NAME);
        *pretvar = NULL;
        return E_FAIL;
    }

    const int property_id_param = (const int)varlist[ARG_PROPERTY]->lVal;

    /*
    * If property id is in range, send the
    * property map and return parameter for
    * population.
    */
    if (property_id_param >= 0 && property_id_param < FACTION_PROPERTY_END) {
        const e_faction_properties property_id = (e_faction_properties)property_id_param;
        const s_property_access_map property_map = faction_get_property_map(acting_object, property_id);

        return property_access_get_member(&property_map, *pretvar);
    }

    /*
    * Is this a dump request? If not, then
    * the property id is invalid.
    */
    if (property_id_param == PROPERTY_ACCESS_DUMP) {
        property_access_dump_members(faction_get_property_map, FACTION_PROPERTY_END, acting_object);
        return S_OK;
    }

    printf("\n\nScript error: %s. Unknown property id (%d).\n\n", SELF_NAME, property_id_param);
    *pretvar = NULL;

    return E_FAIL;
}

/*
* Caskey, Damon V.
* 2023-02-23
*
* Mutate a faction property. Requires
* a faction pointer, property constant,
* and new value.
*/
HRESULT openbor_set_faction_property(ScriptVariant** varlist, ScriptVariant** const pretvar, const int paramCount)
{
    const char* SELF_NAME = "openbor_set_faction_property(void faction, int property, <mixed> value)";
    const unsigned int ARG_OBJECT = 0;
    const unsigned int ARG_PROPERTY = 1;
    const unsigned int ARG_VALUE = 2;
    const int ARG_MINIMUM = 3;

    /*
    * Should at least be a pointer to the
    * acting object, a property id, and
    * a new value.
    */
    if (paramCount < ARG_MINIMUM
        || varlist[ARG_OBJECT]->vt != VT_PTR
        || varlist[ARG_OBJECT]->ptrVal == NULL
        || varlist[ARG_PROPERTY]->vt != VT_INTEGER) {
        printf("\n\nScript error: %s. You must provide a valid object pointer, property id, and new value.\n\n", SELF_NAME);
        *pretvar = NULL;
        return E_FAIL;
    }

    s_faction* const acting_object = (s_faction* const)varlist[ARG_OBJECT]->ptrVal;

    if (acting_object->object_type != OBJECT_TYPE_FACTION) {
        printf("\n\nScript error: %s. Object pointer is not correct type.\n\n", SELF_NAME);
        *pretvar = NULL;
        return E_FAIL;
    }

    const int property_id_param = (const int)varlist[ARG_PROPERTY]->lVal;

    if (property_id_param < 0 || property_id_param >= FACTION_PROPERTY_END) {
        printf("\n\nScript error: %s. Unknown property id (%d).\n\n", SELF_NAME, property_id_param);
        *pretvar = NULL;
        return E_FAIL;
    }

    const e_faction_properties property_id = (e_faction_properties)property_id_param;
    const s_property_access_map property_map = faction_get_property_map(acting_object, property_id);

    /*
    * Populate the property value on the
    * acting object and return OK/FAIL.
    */
    return property_access_set_member(acting_object, &property_map, varlist[ARG_VALUE]);
}