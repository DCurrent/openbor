/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 OpenBOR Team
 */

#include "scriptcommon.h"


const s_property_access_map model_get_property_map(const void* acting_object_param, const unsigned int property_index_param)
{
    s_property_access_map property_map;
    const s_model* acting_object = acting_object_param;
    const e_model_properties property_index = property_index_param;

    switch (property_index)
    {
    case MODEL_PROPERTY_ACTION_FREEZE:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->dofreeze;
        property_map.id_string = "MODEL_PROPERTY_ACTION_FREEZE";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_AIR_CONTROL:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->air_control;
        property_map.id_string = "MODEL_PROPERTY_AIR_CONTROL";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_ANTI_GRAVITY:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->antigravity;
        property_map.id_string = "MODEL_PROPERTY_ANTI_GRAVITY";
        property_map.type = VT_DECIMAL;
        break;

    case MODEL_PROPERTY_BLEND_MODE:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->alpha;
        property_map.id_string = "MODEL_PROPERTY_BLEND_MODE";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_BLOCK_CONFIG_FLAGS:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->block_config_flags;
        property_map.id_string = "MODEL_PROPERTY_BLOCK_CONFIG_FLAGS";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_BLOCK_ODDS:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->blockodds;
        property_map.id_string = "MODEL_PROPERTY_BLOCK_ODDS";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_BLOCK_PAIN:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->blockpain;
        property_map.id_string = "MODEL_PROPERTY_BLOCK_PAIN";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_BLOCK_THRESHOLD:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->thold;
        property_map.id_string = "MODEL_PROPERTY_BLOCK_THRESHOLD";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_BOUNCE:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->bounce;
        property_map.id_string = "MODEL_PROPERTY_BOUNCE";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_CHILD_FOLLOW:
        property_map.config_flags = (PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER);
        property_map.field = &acting_object->child_follow;
        property_map.id_string = "MODEL_PROPERTY_CHILD_FOLLOW";
        property_map.type = VT_PTR;
        break;

    case MODEL_PROPERTY_COLORSET:
        property_map.config_flags = (PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER);
        property_map.field = &acting_object->colorsets;
        property_map.id_string = "MODEL_PROPERTY_COLORSET";
        property_map.type = VT_PTR;
        break;

    case MODEL_PROPERTY_DEATH_CONFIG_FLAGS:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->death_config_flags;
        property_map.id_string = "MODEL_PROPERTY_DEATH_CONFIG_FLAGS";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_ENHANCED_DELAY_CAP_MAX:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->edelay.cap.max;
        property_map.id_string = "MODEL_PROPERTY_ENHANCED_DELAY_CAP_MAX";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_ENHANCED_DELAY_CAP_MIN:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->edelay.cap.min;
        property_map.id_string = "MODEL_PROPERTY_ENHANCED_DELAY_CAP_MIN";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_ENHANCED_DELAY_MODIFIER:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->edelay.modifier;
        property_map.id_string = "MODEL_PROPERTY_ENHANCED_DELAY_MODIFIER";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_ENHANCED_DELAY_MULTIPLIER:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->edelay.factor;
        property_map.id_string = "MODEL_PROPERTY_ENHANCED_DELAY_MULTIPLIER";
        property_map.type = VT_DECIMAL;
        break;

    case MODEL_PROPERTY_ENHANCED_DELAY_RANGE_MAX:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->edelay.range.max;
        property_map.id_string = "MODEL_PROPERTY_ENHANCED_DELAY_RANGE_MAX";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_ENHANCED_DELAY_RANGE_MIN:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->edelay.range.min;
        property_map.id_string = "MODEL_PROPERTY_ENHANCED_DELAY_RANGE_MIN";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_FACTION:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->faction;
        property_map.id_string = "MODEL_PROPERTY_FACTION";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_FLIP:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->toflip;
        property_map.id_string = "MODEL_PROPERTY_FLIP";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_GROUND:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->ground;
        property_map.id_string = "MODEL_PROPERTY_GROUND";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_GUARD_POINTS:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->guardpoints;
        property_map.id_string = "MODEL_PROPERTY_GUARD_POINTS";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_GUARD_RATE:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->guardrate;
        property_map.id_string = "MODEL_PROPERTY_GUARD_RATE";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_HP:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->health;
        property_map.id_string = "MODEL_PROPERTY_HP";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_HUD_DISABLE:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->nolife;
        property_map.id_string = "MODEL_PROPERTY_HUD_DISABLE";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_HUD_POPUP:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->hud_popup;
        property_map.id_string = "MODEL_PROPERTY_HUD_POPUP";
        property_map.type = VT_PTR;
        break;

    case MODEL_PROPERTY_ICON:
        property_map.config_flags = (PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER);
        property_map.field = &acting_object->icon;
        property_map.id_string = "MODEL_PROPERTY_ICON";
        property_map.type = VT_PTR;
        break;

    case MODEL_PROPERTY_INDEX:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->index;
        property_map.id_string = "MODEL_PROPERTY_INDEX";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_LAYER:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->setlayer;
        property_map.id_string = "MODEL_PROPERTY_LAYER";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_MAKE_INVINCIBLE:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->makeinv;
        property_map.id_string = "MODEL_PROPERTY_MAKE_INVINCIBLE";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_MOVE_CONFIG_FLAGS:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->move_config_flags;
        property_map.id_string = "MODEL_PROPERTY_MOVE_CONFIG_FLAGS";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_MP:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->mp;
        property_map.id_string = "MODEL_PROPERTY_MP";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_MULTIPLE:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->multiple;
        property_map.id_string = "MODEL_PROPERTY_MULTIPLE";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_NAME:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->name;
        property_map.id_string = "MODEL_PROPERTY_NAME";
        property_map.type = VT_STR;
        break;

    case MODEL_PROPERTY_TEST_FIXED:
        property_map.config_flags = (PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT | PROPERTY_ACCESS_CONFIG_STATIC_LENGTH);
        property_map.field = &acting_object->test_fixed;
        property_map.id_string = "MODEL_PROPERTY_TEST_FIXED";
        property_map.type = VT_STR;
        break;

    case MODEL_PROPERTY_TEST_POINTER:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->test_pointer;
        property_map.id_string = "MODEL_PROPERTY_TEST_POINTER";
        property_map.type = VT_STR;
        break;

    case MODEL_PROPERTY_OFF_SCREEN_KILL:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->offscreenkill;
        property_map.id_string = "MODEL_PROPERTY_OFF_SCREEN_KILL";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_OFF_SCREEN_NO_ATTACK:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->offscreen_noatk_factor;
        property_map.id_string = "MODEL_PROPERTY_OFF_SCREEN_NO_ATTACK";
        property_map.type = VT_DECIMAL;
        break;

    case MODEL_PROPERTY_PAIN_CONFIG_FLAGS:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->pain_config_flags;
        property_map.id_string = "MODEL_PROPERTY_PAIN_CONFIG_FLAGS";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_PATH:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->path;
        property_map.id_string = "MODEL_PROPERTY_PATH";
        property_map.type = VT_STR;
        break;

    case MODEL_PROPERTY_PRIORITY:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->priority;
        property_map.id_string = "MODEL_PROPERTY_PRIORITY";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_QUAKE_CONFIG:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->quake_config;
        property_map.id_string = "MODEL_PROPERTY_QUAKE_CONFIG";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_RISE_INVINCIBLE:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->riseinv;
        property_map.id_string = "MODEL_PROPERTY_RISE_INVINCIBLE";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_RUN_CONFIG_FLAGS:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->run_config_flags;
        property_map.id_string = "MODEL_PROPERTY_RUN_CONFIG_FLAGS";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_RUN_JUMP_HEIGHT:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->runjumpheight;
        property_map.id_string = "MODEL_PROPERTY_RUN_JUMP_HEIGHT";
        property_map.type = VT_DECIMAL;
        break;

    case MODEL_PROPERTY_RUN_JUMP_LENGTH:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->runjumpdist;
        property_map.id_string = "MODEL_PROPERTY_RUN_JUMP_LENGTH";
        property_map.type = VT_DECIMAL;
        break;

    case MODEL_PROPERTY_RUN_SPEED:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->runspeed;
        property_map.id_string = "MODEL_PROPERTY_RUN_SPEED";
        property_map.type = VT_DECIMAL;
        break;

    case MODEL_PROPERTY_SCORE:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->score;
        property_map.id_string = "MODEL_PROPERTY_SCORE";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_SCROLL:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->scroll;
        property_map.id_string = "MODEL_PROPERTY_SCROLL";
        property_map.type = VT_DECIMAL;
        break;

    case MODEL_PROPERTY_SHADOW_CONFIG_FLAGS:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->shadow_config_flags;
        property_map.id_string = "MODEL_PROPERTY_SHADOW_CONFIG_FLAGS";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_SHADOW_INDEX:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->shadow;
        property_map.id_string = "MODEL_PROPERTY_SHADOW_INDEX";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_SPAWN_HUD:
        property_map.config_flags = (PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER);
        property_map.field = &acting_object->player_arrow;
        property_map.id_string = "MODEL_PROPERTY_SPAWN_HUD";
        property_map.type = VT_PTR;
        break;

    case MODEL_PROPERTY_SUBTYPE:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->subtype;
        property_map.id_string = "MODEL_PROPERTY_SUBTYPE";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_TYPE:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
        property_map.field = &acting_object->type;
        property_map.id_string = "MODEL_PROPERTY_TYPE";
        property_map.type = VT_INTEGER;
        break;

    case MODEL_PROPERTY_WEAPON:
        property_map.config_flags = (PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER);
        property_map.field = &acting_object->weapon_properties;
        property_map.id_string = "MODEL_PROPERTY_WEAPON";
        property_map.type = VT_PTR;
        break;

    case MODEL_PROPERTY_END:
    default:
        property_map.config_flags = PROPERTY_ACCESS_CONFIG_NONE;
        property_map.field = NULL;
        property_map.id_string = "Model";
        property_map.type = VT_EMPTY;
        break;

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

