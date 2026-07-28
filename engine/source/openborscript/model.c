/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 OpenBOR Team
 */

#include "scriptcommon.h"


typedef struct {
    e_model_properties property;
    e_property_access_config_flags config_flags;
    size_t offset;
    const char* id_string;
    VARTYPE type;
} model_property_info;

#define PROPERTY_MEMBER_OFFSET(type, member) ((size_t)&(((type *)0)->member))

static const model_property_info model_properties[] = {
    {.property = MODEL_PROPERTY_ACTION_FREEZE,
     .id_string = "MODEL_PROPERTY_ACTION_FREEZE",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, dofreeze),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_AIR_CONTROL,
     .id_string = "MODEL_PROPERTY_AIR_CONTROL",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, air_control),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_ANTI_GRAVITY,
     .id_string = "MODEL_PROPERTY_ANTI_GRAVITY",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, antigravity),
     .type = VT_DECIMAL },

    {.property = MODEL_PROPERTY_BLEND_MODE,
     .id_string = "MODEL_PROPERTY_BLEND_MODE",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, alpha),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_BLOCK_CONFIG_FLAGS,
     .id_string = "MODEL_PROPERTY_BLOCK_CONFIG_FLAGS",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, block_config_flags),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_BLOCK_ODDS,
     .id_string = "MODEL_PROPERTY_BLOCK_ODDS",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, blockodds),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_BLOCK_PAIN,
     .id_string = "MODEL_PROPERTY_BLOCK_PAIN",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, blockpain),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_BLOCK_THRESHOLD,
     .id_string = "MODEL_PROPERTY_BLOCK_THRESHOLD",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, thold),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_BOUNCE,
     .id_string = "MODEL_PROPERTY_BOUNCE",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, bounce),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_CHILD_FOLLOW,
     .id_string = "MODEL_PROPERTY_CHILD_FOLLOW",
     .config_flags = PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, child_follow),
     .type = VT_PTR },

    {.property = MODEL_PROPERTY_COLORSET,
     .id_string = "MODEL_PROPERTY_COLORSET",
     .config_flags = PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, colorsets),
     .type = VT_PTR },

    {.property = MODEL_PROPERTY_DEATH_CONFIG_FLAGS,
     .id_string = "MODEL_PROPERTY_DEATH_CONFIG_FLAGS",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, death_config_flags),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_ENHANCED_DELAY_CAP_MAX,
     .id_string = "MODEL_PROPERTY_ENHANCED_DELAY_CAP_MAX",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, edelay.cap.max),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_ENHANCED_DELAY_CAP_MIN,
     .id_string = "MODEL_PROPERTY_ENHANCED_DELAY_CAP_MIN",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, edelay.cap.min),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_ENHANCED_DELAY_MODIFIER,
     .id_string = "MODEL_PROPERTY_ENHANCED_DELAY_MODIFIER",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, edelay.modifier),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_ENHANCED_DELAY_MULTIPLIER,
     .id_string = "MODEL_PROPERTY_ENHANCED_DELAY_MULTIPLIER",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, edelay.factor),
     .type = VT_DECIMAL },

    {.property = MODEL_PROPERTY_ENHANCED_DELAY_RANGE_MAX,
     .id_string = "MODEL_PROPERTY_ENHANCED_DELAY_RANGE_MAX",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, edelay.range.max),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_ENHANCED_DELAY_RANGE_MIN,
     .id_string = "MODEL_PROPERTY_ENHANCED_DELAY_RANGE_MIN",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, edelay.range.min),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_FACTION,
     .id_string = "MODEL_PROPERTY_FACTION",
     .config_flags = PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, faction),
     .type = VT_PTR },

    {.property = MODEL_PROPERTY_FLIP,
     .id_string = "MODEL_PROPERTY_FLIP",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, toflip),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_GROUND,
     .id_string = "MODEL_PROPERTY_GROUND",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, ground),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_GUARD_POINTS,
     .id_string = "MODEL_PROPERTY_GUARD_POINTS",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, guardpoints),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_GUARD_RATE,
     .id_string = "MODEL_PROPERTY_GUARD_RATE",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, guardrate),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_HP,
     .id_string = "MODEL_PROPERTY_HP",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, health),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_HUD_DISABLE,
     .id_string = "MODEL_PROPERTY_HUD_DISABLE",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, nolife),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_HUD_POPUP,
     .id_string = "MODEL_PROPERTY_HUD_POPUP",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, hud_popup),
     .type = VT_PTR },

    {.property = MODEL_PROPERTY_ICON,
     .id_string = "MODEL_PROPERTY_ICON",
     .config_flags = PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, icon),
     .type = VT_PTR },

    {.property = MODEL_PROPERTY_INDEX,
     .id_string = "MODEL_PROPERTY_INDEX",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, index),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_LAYER,
     .id_string = "MODEL_PROPERTY_LAYER",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, setlayer),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_MAKE_INVINCIBLE,
     .id_string = "MODEL_PROPERTY_MAKE_INVINCIBLE",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, makeinv),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_MOVE_CONFIG_FLAGS,
     .id_string = "MODEL_PROPERTY_MOVE_CONFIG_FLAGS",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, move_config_flags),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_MP,
     .id_string = "MODEL_PROPERTY_MP",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, mp),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_MULTIPLE,
     .id_string = "MODEL_PROPERTY_MULTIPLE",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, multiple),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_NAME,
     .id_string = "MODEL_PROPERTY_NAME",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, name),
     .type = VT_STR },

    {.property = MODEL_PROPERTY_TEST_FIXED,
     .id_string = "MODEL_PROPERTY_TEST_FIXED",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT | PROPERTY_ACCESS_CONFIG_STATIC_LENGTH,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, test_fixed),
     .type = VT_STR },

    {.property = MODEL_PROPERTY_TEST_POINTER,
     .id_string = "MODEL_PROPERTY_TEST_POINTER",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, test_pointer),
     .type = VT_STR },

    {.property = MODEL_PROPERTY_OFF_SCREEN_KILL,
     .id_string = "MODEL_PROPERTY_OFF_SCREEN_KILL",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, offscreenkill),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_OFF_SCREEN_NO_ATTACK,
     .id_string = "MODEL_PROPERTY_OFF_SCREEN_NO_ATTACK",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, offscreen_noatk_factor),
     .type = VT_DECIMAL },

    {.property = MODEL_PROPERTY_PAIN_CONFIG_FLAGS,
     .id_string = "MODEL_PROPERTY_PAIN_CONFIG_FLAGS",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, pain_config_flags),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_PATH,
     .id_string = "MODEL_PROPERTY_PATH",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, path),
     .type = VT_STR },

    {.property = MODEL_PROPERTY_PRIORITY,
     .id_string = "MODEL_PROPERTY_PRIORITY",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, priority),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_QUAKE_CONFIG,
     .id_string = "MODEL_PROPERTY_QUAKE_CONFIG",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, quake_config),
     .type = VT_INTEGER },

    { .property = MODEL_PROPERTY_REMOVE_CONFIG,
     .id_string = "MODEL_PROPERTY_REMOVE_CONFIG",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, remove_config),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_RISE_INVINCIBLE,
     .id_string = "MODEL_PROPERTY_RISE_INVINCIBLE",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, riseinv),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_RUN_CONFIG_FLAGS,
     .id_string = "MODEL_PROPERTY_RUN_CONFIG_FLAGS",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, run_config_flags),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_RUN_JUMP_HEIGHT,
     .id_string = "MODEL_PROPERTY_RUN_JUMP_HEIGHT",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, runjumpheight),
     .type = VT_DECIMAL },

    {.property = MODEL_PROPERTY_RUN_JUMP_LENGTH,
     .id_string = "MODEL_PROPERTY_RUN_JUMP_LENGTH",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, runjumpdist),
     .type = VT_DECIMAL },

    {.property = MODEL_PROPERTY_RUN_SPEED,
     .id_string = "MODEL_PROPERTY_RUN_SPEED",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, runspeed),
     .type = VT_DECIMAL },

    {.property = MODEL_PROPERTY_SCORE,
     .id_string = "MODEL_PROPERTY_SCORE",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, score),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_SCROLL,
     .id_string = "MODEL_PROPERTY_SCROLL",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, scroll),
     .type = VT_DECIMAL },

    {.property = MODEL_PROPERTY_SHADOW_CONFIG_FLAGS,
     .id_string = "MODEL_PROPERTY_SHADOW_CONFIG_FLAGS",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, shadow_config_flags),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_SHADOW_INDEX,
     .id_string = "MODEL_PROPERTY_SHADOW_INDEX",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, shadow),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_SPAWN_HUD,
     .id_string = "MODEL_PROPERTY_SPAWN_HUD",
     .config_flags = PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, player_arrow),
     .type = VT_PTR },

    {.property = MODEL_PROPERTY_SUBTYPE,
     .id_string = "MODEL_PROPERTY_SUBTYPE",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, subtype),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_TYPE,
     .id_string = "MODEL_PROPERTY_TYPE",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, type),
     .type = VT_INTEGER },

    {.property = MODEL_PROPERTY_WEAPON,
     .id_string = "MODEL_PROPERTY_WEAPON",
     .config_flags = PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, weapon_properties),
     .type = VT_PTR },

    {.property = MODEL_PROPERTY_TEST_FIXED,
     .id_string = "MODEL_PROPERTY_TEST_FIXED",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT | PROPERTY_ACCESS_CONFIG_STATIC_LENGTH,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, test_fixed),
     .type = VT_STR },

    {.property = MODEL_PROPERTY_TEST_POINTER,
     .id_string = "MODEL_PROPERTY_TEST_POINTER",
     .config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
     .offset = PROPERTY_MEMBER_OFFSET(s_model, test_pointer),
     .type = VT_STR },

    {.property = MODEL_PROPERTY_END,
     .id_string = "Model",
     .config_flags = PROPERTY_ACCESS_CONFIG_NONE,
     .offset = 0,
     .type = VT_EMPTY }
};

#undef PROPERTY_MEMBER_OFFSET

const s_property_access_map model_get_property_map(const void* acting_object_param, const unsigned int property_index_param)
{
    s_property_access_map property_map = { 0 };
    const s_model* acting_object = acting_object_param;
    const model_property_info* info = NULL;

    for (size_t i = 0; i < sizeof(model_properties) / sizeof(model_properties[0]); ++i) {
        if (model_properties[i].property == property_index_param) {
            info = &model_properties[i];
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
        property_map.id_string = "Model";
        property_map.type = VT_EMPTY;
    }

    return property_map;
}

/*
* Caskey, Damon  V.
* 2023-03-03
*
* Return a property. Requires
* a object pointer and property 
* constant to access.
*/ 
HRESULT openbor_get_model_property(const ScriptVariant* const* varlist, ScriptVariant** const pretvar, const int paramCount)
{
    const char* SELF_NAME = "openbor_get_model_property(void model, int property)";
    const unsigned int ARG_OBJECT    = 0; // Handle (pointer to property structure).
    const unsigned int ARG_PROPERTY  = 1; // Property to access.
    
    /*
    * Clear pass by reference argument used to send
    * property data back to calling script.
    */
    ScriptVariant_Clear(*pretvar);

    /*
    * Should at least be a pointer to the 
    * acting object and a  property id.
    */
    if(varlist[ARG_OBJECT]->vt != VT_PTR
       || varlist[ARG_PROPERTY]->vt != VT_INTEGER){
        printf("\n\n Script error: %s. You must provide a valid object pointer and property id.\n\n", SELF_NAME);
        return E_FAIL;
    }
    
    /*
    * Now let's make sure the object type is
    * correct (ex. entity vs. model) so we
    * can shut down gracefully if there's
    * a mismatch.
    */

    const s_model* const acting_object = (const s_model* const)varlist[ARG_OBJECT]->ptrVal;

    if (acting_object->object_type != OBJECT_TYPE_MODEL) {
        printf("\n\nScript error: %s. Object pointer is not correct type.\n\n", SELF_NAME);
        *pretvar = NULL;
        return E_FAIL;
    }

    const int property_id_param = (const int)varlist[ARG_PROPERTY]->lVal;
    const e_model_properties property_id = (e_model_properties)(property_id_param);
    const s_property_access_map property_map = model_get_property_map(acting_object, property_id);

    /*
    * If property id is in range, we send
    * the property map and return parameter
    * for population, then ext.
    */

    if (property_id_param >= 0 && property_id_param < MODEL_PROPERTY_END) {
        property_access_get_member(&property_map, *pretvar);
        return S_OK;
    }
    
    /*
    * Is this a dump request? If not, then
    * the property id is invalid.
    */

    if (property_id_param == PROPERTY_ACCESS_DUMP) {
        property_access_dump_members(model_get_property_map, MODEL_PROPERTY_END, acting_object);
    }
    else {
        printf("\n\nScript error: %s. Unknown property id (%d). \n\n", SELF_NAME, property_id_param);
        return E_FAIL;
    }

    return S_OK;
}


/*
* Caskey, Damon  V.
* 2018-04-03
*
* Mutate a property. Requires
* the object pointer, a property
* id, and new value.
*/ 
HRESULT openbor_set_model_property(ScriptVariant** varlist, ScriptVariant** const pretvar, const int paramCount)
{
    const char* SELF_NAME = "openbor_set_model_property(void model, int property, <mixed> value)";
    const unsigned int ARG_OBJECT = 0;
    const unsigned int ARG_PROPERTY = 1;
    const unsigned int ARG_VALUE = 2;
    const unsigned int ARG_MINIMUM = 3;
    
    /*
    * Should at least be a pointer to the
    * acting object, a property id, and 
    * a new value.
    */

    if (varlist[ARG_OBJECT]->vt != VT_PTR
        || varlist[ARG_PROPERTY]->vt != VT_INTEGER
        || paramCount < ARG_MINIMUM) {
        printf("\n\n Script error: %s. You must provide a valid object pointer, property id, and new value.\n\n", SELF_NAME);
        *pretvar = NULL;
        return E_FAIL;
    }

    /*
    * Now let's make sure the object type is
    * correct (ex. entity vs. model) so we
    * can shut down gracefully if there's
    * a mismatch.
    */

    const s_model* const acting_object = (const s_model* const)varlist[ARG_OBJECT]->ptrVal;  

    if (acting_object->object_type != OBJECT_TYPE_MODEL) {
        printf("\n\nScript error: %s. Object pointer is not correct type.\n\n", SELF_NAME);
        *pretvar = NULL;
        return E_FAIL;
    }
    
    const int property_id_param = (const int)varlist[ARG_PROPERTY]->lVal;
    const e_model_properties property_id = (e_model_properties)(property_id_param);

    if (property_id_param < 0 && property_id_param >= MODEL_PROPERTY_END) {
        printf("\n\nScript error: %s. Unknown property id (%d). \n\n", SELF_NAME, property_id_param);
        return E_FAIL;
    }

    /*
    * Get map of property. This is a struct
    * that contains the property variable
    * type, reference to the acting object's
    * appropriate data member, text name, 
    * read only, etc.
    */    
    
    const s_property_access_map property_map = model_get_property_map(acting_object, property_id);

    /*
    * Populate the property value on
    * acting object and return OK/FAIL.
    */

    return property_access_set_member(acting_object, &property_map, varlist[ARG_VALUE]);
}





