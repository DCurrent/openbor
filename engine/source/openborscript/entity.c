/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 - 2018 OpenBOR Team
 */

 #include "scriptcommon.h"

// Use string property argument to find an
// integer property constant and populate
// varlist->lval.
int mapstrings_entity_property(ScriptVariant **varlist, int paramCount)
{
    #define ARG_MINIMUM     2   // Minimum number of arguments allowed in varlist.
    #define ARG_PROPERTY    1   // Varlist element carrying which property is requested.

    char *propname = NULL;  // Placeholder for string property name from varlist.
    int prop;               // Placeholder for integer constant located by string.

    static const char *proplist[] =
    {
		"ai_disable",
		"ai_target_entity",
		"alternate_idle",
		"alternate_walk",
		"animation",
		"animation_frame",
		"animation_id",
		"animation_id_previous",
		"animation_state",
		"animation_time",
		"arrow_state",
		"attack_id_incoming",
		"attack_id_outgoing",
		"attack_state",
		"autokill",
		"back_hit_direction",
		"bind",
		"blast_state",
		"blink",
		"block_state",
		"boss",
		"charge_state",
		"child",
		"colorset_default",
		"colorset_dying_health_1",
		"colorset_dying_health_2",
		"colorset_dying_index_1",
		"colorset_dying_index_2",
		"colorset_table",
		"colorset_time",
		"combo_step",
		"combo_time",
		"command_time",
		"damage_on_landing",
		"dead",
		"deduct_ammo",
		"defense_collection",
		"destination_x",
		"destination_z",
		"die_on_landing",
		"drawmethod",
		"drop",
		"duck_state",
		"entvar_collection",
		"escape_count",
		"exists",
		"explode",
		"fall_state",
		"freeze_state",
		"freeze_time",
		"function_take_action",
		"function_take_damage",
		"function_think",
		"function_try_move",
		"get_state",
		"grab_target",
		"grab_walk_state",
		"guard_time",
		"hp",
		"hp_old",
		"idle_state",
		"in_pain",
		"in_pain_back",
		"invincible_state",
		"invincible_time",
		"item_data",
		"jump_animation_id",
		"jump_state",
		"jump_velocity_x",
		"jump_velocity_y",
		"jump_velocity_z",
		"knockdown_count",
		"knockdown_time",
		"last_damage_type",
		"last_hit",
		"lifespan",
		"link",
		"model",
		"model_data",
		"model_default",
		"move_time",
		"move_x",
		"move_z",
		"mp",
		"mp_charge_time",
		"mp_old",
		"mp_time",
		"name",
		"next_attack_time",
		"next_hit_time",
		"nograb",
		"nograb_default",
		"obstructed",
		"obstruction_overhead",
		"offense_collection",
		"opponent",
		"owner",
		"parent",
		"path_obstructed_wait",
		"pause_time",
		"platform_land",
		"player_index",
		"position_base",
		"position_base_alternate",
		"position_direction",
		"position_x",
		"position_y",
		"position_z",
		"projectile_prime",
		"recursive_damage",
		"release_time",
		"rise_attack_delay",
		"rise_attack_time",
		"rise_delay",
		"rise_state",
		"run_state",
		"rush",
		"script_collection",
		"seal_energy",
		"seal_time",
		"sleep_time",
		"sort_id",
		"space_other",
		"spawn_type",
		"speed_multiplier",
		"stall_time",
		"think_time",
		"timestamp",
		"to_cost",
		"toss_time",
		"turn_state",
		"turn_time",
		"update_mark",
		"velocity_x",
		"velocity_y",
		"velocity_z",
		"walk_state",
		"waypoint_collection",
		"waypoint_count",
		"weapon_item"
    };

    // If the minimum argument count
    // was not passed, then there is
    // nothing to map. Return true - we'll
    // catch the mistake in property access
    // functions.
    if(paramCount < ARG_MINIMUM)
    {
        return 1;
    }

    // See macro - will return 0 on fail.
    MAPSTRINGS(varlist[ARG_PROPERTY], proplist, _ENTITY_END,
               "\n\n Error: '%s' is not a known entity property.\n");


    // If we made it this far everything should be OK.
    return 1;

    #undef ARG_MINIMUM
    #undef ARG_PROPERTY
}


// Caskey, Damon  V.
// 2018-04-02
//
// Return an entity property. Requires
// an entity handle and property name to
// access.
HRESULT openbor_get_entity_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "openbor_get_entity_property(void handle, char property)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_HANDLE      0   // Handle (pointer to property structure).
    #define ARG_PROPERTY    1   // Property to access.

    entity                  *handle     = NULL; // Property handle.
    e_entity_properties    property    = 0;    // Property argument.

    // Clear pass by reference argument used to send
    // property data back to calling script.
    ScriptVariant_Clear(*pretvar);

    // Verify arguments. There should at least
    // be a pointer for the property handle and an integer
    // to determine which property constant is accessed.
    if(paramCount < ARG_MINIMUM
       || varlist[ARG_HANDLE]->vt != VT_PTR
       || varlist[ARG_PROPERTY]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }
    else
    {
        // Populate local vars for readability.
        handle      = (entity *)varlist[ARG_HANDLE]->ptrVal;
        property    = (LONG)varlist[ARG_PROPERTY]->lVal;
    }
	
    switch(property)
    {
		case _ENTITY_AI_DISABLE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->noaicontrol;

			break;

        case _ENTITY_AI_TARGET_ENTITY:

            if(handle->custom_target)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (entity *)handle->custom_target;
            }

            break;

		case _ENTITY_ALTERNATE_IDLE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->idlemode;

			break;

		case _ENTITY_ALTERNATE_WALK:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->walkmode;

			break;

        case _ENTITY_ANIMATION:
            
            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (s_anim *)handle->animation;           

            break;

        case _ENTITY_ANIMATION_FRAME:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->animpos;

            break;

		case _ENTITY_ANIMATION_ID:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->animnum;

			break;

		case _ENTITY_ANIMATION_ID_PREVIOUS:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->animnum_previous;

			break;

		case _ENTITY_ANIMATION_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->animating;

			break;

		case _ENTITY_ANIMATION_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->nextanim;

			break;

        case _ENTITY_ARROW_STATE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->arrowon;

            break;

        case _ENTITY_ATTACK_ID_INCOMING:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->attack_id_incoming;

            break;

        case _ENTITY_ATTACK_ID_OUTGOING:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->attack_id_outgoing;

            break;

		case _ENTITY_ATTACK_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->attacking;

			break;

        case _ENTITY_AUTOKILL:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->autokill;

            break;

		case _ENTITY_BACK_HIT_DIRECTION:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->normaldamageflipdir;

			break;

        case _ENTITY_BIND:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (s_bind *)&handle->binding;

            break;

		case _ENTITY_BLAST_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->projectile;

			break;

        case _ENTITY_BLINK:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->blink;

            break;

        case _ENTITY_BLOCK_STATE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->blocking;

            break;

        case _ENTITY_BOSS:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->boss;

            break;

        case _ENTITY_CHARGE_STATE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->charging;

            break;

		case _ENTITY_CHILD:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (entity *)handle->subentity;

			break;

        case _ENTITY_COLORSET_DEFAULT:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->map;

            break;

        case _ENTITY_COLORSET_DYING_HEALTH_1:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->per1;

            break;

        case _ENTITY_COLORSET_DYING_HEALTH_2:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->per2;

            break;

        case _ENTITY_COLORSET_DYING_INDEX_1:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->dying;

            break;

        case _ENTITY_COLORSET_DYING_INDEX_2:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->dying2;

            break;

        case _ENTITY_COLORSET_TABLE:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)(handle->colourmap);

            break;

        case _ENTITY_COLORSET_TIME:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->maptime;

            break;

        case _ENTITY_COMBO_STEP:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)&handle->combostep;

            break;

        case _ENTITY_COMBO_TIME:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->combotime;

            break;

		case _ENTITY_COMMAND_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->movetime;

			break;

        case _ENTITY_DAMAGE_ON_LANDING:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (s_damage_on_landing *)&handle->damage_on_landing;

            break;

		case _ENTITY_DEAD:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->dead;

			break;

        case _ENTITY_DEDUCT_AMMO:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->deduct_ammo;

            break;

		case _ENTITY_DEFENSE_COLLECTION:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_defense *)handle->defense;

			break;

		case _ENTITY_DESTINATION_X:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->destx;

			break;

		case _ENTITY_DESTINATION_Z:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->destz;

			break;

		case _ENTITY_DIE_ON_LANDING:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->die_on_landing;

			break;

		case _ENTITY_DRAWMETHOD:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_drawmethod *)handle->drawmethod;

			break;

		case _ENTITY_DROP:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->drop;

			break;

		case _ENTITY_DUCK_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->ducking;

			break;

		case _ENTITY_ENTVAR_COLLECTION:

			if (handle->varlist)
			{
				ScriptVariant_ChangeType(*pretvar, VT_PTR);
				(*pretvar)->ptrVal = (Varlist *)handle->varlist;
			}

			break;

		case _ENTITY_ESCAPE_COUNT:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->escapecount;

			break;

		case _ENTITY_EXISTS:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->exists;

			break;

		case _ENTITY_EXPLODE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->toexplode;

			break;

		case _ENTITY_FALL_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->falling;

			break;

		case _ENTITY_FREEZE_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->frozen;

			break;

		case _ENTITY_FREEZE_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->freezetime;

			break;

		case _ENTITY_FUNCTION_TAKE_ACTION:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (VOID *)handle->takeaction;

			break;

		case _ENTITY_FUNCTION_TAKE_DAMAGE:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (VOID *)handle->takedamage;

			break;

		case _ENTITY_FUNCTION_THINK:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (VOID *)handle->think;

			break;

		case _ENTITY_FUNCTION_TRY_MOVE:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (VOID *)handle->trymove;

			break;

		case _ENTITY_GET_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->getting;

			break;

		case _ENTITY_GRAB_TARGET:

			if (handle->custom_target)
			{
				ScriptVariant_ChangeType(*pretvar, VT_PTR);
				(*pretvar)->ptrVal = (entity *)handle->grabbing;
			}

			break;

		case _ENTITY_GRAB_WALK_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->grabwalking;

			break;

		case _ENTITY_GUARD_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->guardtime;

			break;

		case _ENTITY_HP:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->energy_state.health_current;

			break;

		case _ENTITY_HP_OLD:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->energy_state.health_old;

			break;

		case _ENTITY_IDLE_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->idling;

			break;

		case _ENTITY_IN_PAIN:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->inpain;

			break;

		case _ENTITY_IN_PAIN_BACK:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->inbackpain;

			break;

		case _ENTITY_INVINCIBLE_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->invincible;

			break;

		case _ENTITY_INVINCIBLE_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->invinctime;

			break;

		case _ENTITY_ITEM_DATA:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_item_properties *)&handle->item_properties;

			break;

		case _ENTITY_JUMP_ANIMATION_ID:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->jump.animation_id;

			break;

		case _ENTITY_JUMP_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->jumping;

			break;

		case _ENTITY_JUMP_VELOCITY_X:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->jump.velocity.x;

			break;

		case _ENTITY_JUMP_VELOCITY_Y:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->jump.velocity.y;

			break;

		case _ENTITY_JUMP_VELOCITY_Z:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->jump.velocity.z;

			break;

		case _ENTITY_KNOCKDOWN_COUNT:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->knockdowncount;

			break;

		case _ENTITY_KNOCKDOWN_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->knockdowntime;

			break;

		case _ENTITY_LAST_DAMAGE_TYPE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->last_damage_type;

			break;

		case _ENTITY_LAST_HIT:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (entity *)handle->lasthit;

			break;

		case _ENTITY_LIFESPAN:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->lifespancountdown;

			break;

		case _ENTITY_LINK:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (entity *)handle->link;

			break;

		case _ENTITY_MODEL:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_model *)(handle->model);

			break;

		case _ENTITY_MODEL_DATA:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_model *)&handle->modeldata;

			break;

		case _ENTITY_MODEL_DEFAULT:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_model *)(handle->defaultmodel);

			break;

		case _ENTITY_MOVE_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->nextmove;

			break;

		case _ENTITY_MOVE_X:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->movex;

			break;

		case _ENTITY_MOVE_Z:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->movez;

			break;

		case _ENTITY_MP:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->energy_state.mp_current;

			break;

		case _ENTITY_MP_CHARGE_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->mpchargetime;

			break;

		case _ENTITY_MP_OLD:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->energy_state.mp_old;

			break;

		case _ENTITY_MP_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->magictime;

			break;

		case _ENTITY_NAME:

			ScriptVariant_ChangeType(*pretvar, VT_STR);
			(*pretvar)->strVal = StrCache_CreateNewFrom(handle->name);
			break;

		case _ENTITY_NEXT_ATTACK_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->nextattack;

			break;

		case _ENTITY_NEXT_HIT_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->next_hit_time;

			break;

		case _ENTITY_NOGRAB:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->nograb;

			break;

		case _ENTITY_NOGRAB_DEFAULT:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->nograb_default;

			break;

		case _ENTITY_OBSTRUCTED:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->hitwall;

			break;

		case _ENTITY_OBSTRUCTION_OVERHEAD:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (entity *)handle->hithead;

			break;

		case _ENTITY_OFFENSE_COLLECTION:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (float *)handle->offense_factors;

			break;

        case _ENTITY_OPPONENT:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (entity *)handle->opponent;

            break;

        case _ENTITY_OWNER:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (entity *)handle->owner;

            break;

		case _ENTITY_PARENT:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (entity *)handle->parent;

			break;

		case _ENTITY_PATH_OBSTRUCTED_WAIT:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->pathblocked;

			break;

		case _ENTITY_PAUSE_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->pausetime;

			break;

		case _ENTITY_PLATFORM_LAND:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (entity *)handle->landed_on_platform;

			break;

        case _ENTITY_PLAYER_INDEX:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->playerindex;

            break;

		case _ENTITY_POSITION_BASE:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->base;

			break;

        case _ENTITY_POSITION_BASE_ALTERNATE:

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)handle->altbase;

			break;

        case _ENTITY_POSITION_DIRECTION:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->direction;

            break;

		case _ENTITY_POSITION_X:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->position.x;

			break;

		case _ENTITY_POSITION_Y:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->position.y;

			break;

		case _ENTITY_POSITION_Z:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->position.z;

			break;

        case _ENTITY_PROJECTILE_PRIME:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->projectile_prime;

            break;

		case _ENTITY_RECURSIVE_DAMAGE:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_damage_recursive *)handle->recursive_damage;

			break;

		case _ENTITY_RELEASE_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->releasetime;

			break;

		case _ENTITY_RISE_ATTACK_DELAY:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->staydown.riseattack;

			break;

		case _ENTITY_RISE_ATTACK_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->staydown.riseattack_stall;

			break;

		case _ENTITY_RISE_DELAY:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->staydown.rise;

			break;

		case _ENTITY_RISE_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->rising;

			break;

		case _ENTITY_RUN_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->running;

			break;

		case _ENTITY_RUSH:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_rush *)&handle->rush;

			break;

		case _ENTITY_SCRIPT_COLLECTION:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_scripts *)handle->scripts;

			break;

		case _ENTITY_SEAL_ENERGY:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->seal;

			break;

		case _ENTITY_SEAL_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->sealtime;

			break;

		case _ENTITY_SLEEP_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->sleeptime;

			break;

		case _ENTITY_SORT_ID:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->sortid;

			break;

		case _ENTITY_SPACE_OTHER:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (entity *)handle->collided_entity;

			break;

        case _ENTITY_SPAWN_TYPE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->spawntype;

            break;

		case _ENTITY_SPEED_MULTIPLIER:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->speedmul;

			break;

		case _ENTITY_STALL_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->stalltime;

			break;

		case _ENTITY_THINK_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->nextthink;

			break;

		case _ENTITY_TIMESTAMP:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->timestamp;

			break;

		case _ENTITY_TO_COST:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->tocost;

			break;

		case _ENTITY_TOSS_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->toss_time;

			break;

		case _ENTITY_TURN_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->turning;

			break;

		case _ENTITY_TURN_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->turntime;

			break;

		case _ENTITY_UPDATE_MARK:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->update_mark;

			break;

		case _ENTITY_VELOCITY_X:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->velocity.x;

			break;

		case _ENTITY_VELOCITY_Y:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->velocity.y;

			break;

		case _ENTITY_VELOCITY_Z:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->velocity.z;

			break;

		case _ENTITY_WALK_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->walking;

			break;

		case _ENTITY_WAYPOINT_COLLECTION:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_axis_plane_lateral_float *)handle->waypoints;

			break;

		case _ENTITY_WAYPOINT_COUNT:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->numwaypoints;

			break;

		case _ENTITY_WEAPON_ITEM:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (entity *)handle->weapent;

			break;

        default:

            printf("Unsupported property.\n");
            goto error_local;

            break;
    }

    return S_OK;

    error_local:

    printf("You must provide a valid handle and property name: " SELF_NAME "\n");
    *pretvar = NULL;

    return E_FAIL;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_HANDLE
    #undef ARG_INDEX
}

// Caskey, Damon  V.
// 2018-04-03
//
// Mutate an entity property. Requires
// the entity handle, a string property
// name, and new value.
HRESULT openbor_set_entity_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME           "openbor_set_entity_property(void handle, char property, value)"
    #define ARG_MINIMUM         3   // Minimum required arguments.
    #define ARG_HANDLE          0   // Handle (pointer to property structure).
    #define ARG_PROPERTY        1   // Property to access.
    #define ARG_VALUE           2   // New value to apply.

    int                 result      = S_OK; // Success or error?
    entity              *handle     = NULL; // Property handle.
    e_entity_properties property    = 0;    // Property to access.

    // Value carriers to apply on properties after
    // taken from argument.
    LONG    temp_int;
    DOUBLE  temp_float;
	
    // Verify incoming arguments. There should at least
    // be a pointer for the property handle and an integer
    // to determine which property is accessed.
    if(paramCount < ARG_MINIMUM
       || varlist[ARG_HANDLE]->vt != VT_PTR
       || varlist[ARG_PROPERTY]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }

    // Populate local handle and property vars.
    handle      = (entity *)varlist[ARG_HANDLE]->ptrVal;
    property    = (LONG)varlist[ARG_PROPERTY]->lVal;

    // Which property to modify?
    switch(property)
    {

		case _ENTITY_AI_DISABLE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->noaicontrol = temp_int;
			}

			break;

        case _ENTITY_AI_TARGET_ENTITY:

            handle->custom_target = (entity *)varlist[ARG_VALUE]->ptrVal;

            break;

		case _ENTITY_ALTERNATE_IDLE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->idlemode = temp_int;
			}

			break;

		case _ENTITY_ALTERNATE_WALK:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->walkmode = temp_int;
			}

			break;

        case _ENTITY_ANIMATION:

            handle->animation = (s_anim *)varlist[ARG_VALUE]->ptrVal;

            break;

        case _ENTITY_ANIMATION_FRAME:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->animpos = temp_int;
            }

            break;

		case _ENTITY_ANIMATION_ID:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->animnum = temp_int;
			}

			break; 

		case _ENTITY_ANIMATION_ID_PREVIOUS:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->animnum_previous = temp_int;
			}

			break;

		case _ENTITY_ANIMATION_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->animating = temp_int;
			}

			break;

		case _ENTITY_ANIMATION_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->nextanim = temp_int;
			}

			break;

        case _ENTITY_ARROW_STATE:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->arrowon = temp_int;
            }

            break;

        case _ENTITY_ATTACK_ID_INCOMING:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->attack_id_incoming = temp_int;
            }

            break;

        case _ENTITY_ATTACK_ID_OUTGOING:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->attack_id_outgoing = temp_int;
            }

            break;

		case _ENTITY_ATTACK_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->attacking = temp_int;
			}

			break;

        case _ENTITY_AUTOKILL:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->autokill = temp_int;
            }

            break;

		case _ENTITY_BACK_HIT_DIRECTION:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->normaldamageflipdir = temp_int;
			}

			break;

        case _ENTITY_BIND:

            // Read only.

            break;

		case _ENTITY_BLAST_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->projectile = temp_int;
			}

			break;

        case _ENTITY_BLINK:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->blink = temp_int;
            }

            break;

        case _ENTITY_BLOCK_STATE:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->blocking = temp_int;
            }

            break;

        case _ENTITY_BOSS:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->boss = temp_int;
            }

            break;

        case _ENTITY_CHARGE_STATE:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->charging = temp_int;
            }

            break;

		case _ENTITY_CHILD:

			handle->subentity = (entity *)varlist[ARG_VALUE]->ptrVal;

			break;

        case _ENTITY_COLORSET_DEFAULT:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->map = temp_int;
            }

            break;

        case _ENTITY_COLORSET_DYING_HEALTH_1:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->per1 = temp_int;
            }

            break;

        case _ENTITY_COLORSET_DYING_HEALTH_2:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->per2 = temp_int;
            }

            break;

        case _ENTITY_COLORSET_DYING_INDEX_1:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->dying = temp_int;
            }

            break;

        case _ENTITY_COLORSET_DYING_INDEX_2:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->dying2 = temp_int;
            }

            break;

        case _ENTITY_COLORSET_TABLE:

			handle->colourmap = (VOID *)varlist[ARG_VALUE]->ptrVal;

	        break;

        case _ENTITY_COLORSET_TIME:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->maptime = temp_int;
            }

            break;

        case _ENTITY_COMBO_STEP:

            // Read only.

            break;

        case _ENTITY_COMBO_TIME:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->combotime = temp_int;
            }

            break;

		case _ENTITY_COMMAND_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->movetime = temp_int;
			}

			break;

        case _ENTITY_DAMAGE_ON_LANDING:

            // Read only.

            break;

		case _ENTITY_DEAD:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->dead = temp_int;
			}

			break;

        case _ENTITY_DEDUCT_AMMO:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->deduct_ammo = temp_int;
            }

            break;

		case _ENTITY_DEFENSE_COLLECTION:

			handle->defense = (s_defense *)varlist[ARG_VALUE]->ptrVal;

			break;

		case _ENTITY_DESTINATION_X:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->destx = temp_float;
			}

			break;

		case _ENTITY_DESTINATION_Z:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->destz = temp_float;
			}

			break;

		case _ENTITY_DIE_ON_LANDING:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->die_on_landing = temp_int;
			}

			break;

		case _ENTITY_DRAWMETHOD:

			handle->drawmethod = (s_drawmethod *)varlist[ARG_VALUE]->ptrVal;

			break;

		case _ENTITY_DROP:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->drop = temp_int;
			}

			break;

		case _ENTITY_DUCK_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->ducking = temp_int;
			}

			break;

		case _ENTITY_ENTVAR_COLLECTION:

			handle->varlist = (Varlist *)varlist[ARG_VALUE]->ptrVal;

			break;

		case _ENTITY_ESCAPE_COUNT:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->escapecount = temp_int;
			}

			break;

		case _ENTITY_EXISTS:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->exists = temp_int;
			}

			break;

		case _ENTITY_EXPLODE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->toexplode = temp_int;
			}

			break;

		case _ENTITY_FALL_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->falling = temp_int;
			}

			break;

		case _ENTITY_FREEZE_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->frozen = temp_int;
			}

			break;

		case _ENTITY_FREEZE_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->freezetime = temp_int;
			}

			break;

		case _ENTITY_FUNCTION_TAKE_ACTION:

			handle->takeaction = (VOID *)varlist[ARG_VALUE]->ptrVal;

			break;

		case _ENTITY_FUNCTION_TAKE_DAMAGE:

			handle->takedamage = (VOID *)varlist[ARG_VALUE]->ptrVal;

			break;

		case _ENTITY_FUNCTION_THINK:

			handle->think = (VOID *)varlist[ARG_VALUE]->ptrVal;

			break;

		case _ENTITY_FUNCTION_TRY_MOVE:

			handle->trymove = (VOID *)varlist[ARG_VALUE]->ptrVal;

			break;

		case _ENTITY_GET_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->getting = temp_int;
			}

			break;

		case _ENTITY_GRAB_TARGET:

			handle->grabbing = (entity *)varlist[ARG_VALUE]->ptrVal;

			break;

		case _ENTITY_GRAB_WALK_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->grabwalking = temp_int;
			}

			break;

		case _ENTITY_GUARD_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->guardtime = temp_int;
			}

			break;

		case _ENTITY_HP:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->energy_state.health_current = temp_int;
			}

			break;

		case _ENTITY_HP_OLD:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->energy_state.health_old = temp_int;
			}

			break;	

		case _ENTITY_IDLE_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->idling = temp_int;
			}

			break;

		case _ENTITY_IN_PAIN:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->inpain = temp_int;
			}

			break;

		case _ENTITY_IN_PAIN_BACK:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->inbackpain = temp_int;
			}

			break;

		case _ENTITY_INVINCIBLE_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->invincible = temp_int;
			}

			break;

		case _ENTITY_INVINCIBLE_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->invinctime = temp_int;
			}

			break;

		case _ENTITY_ITEM_DATA:

			// Read only.

			break;

		case _ENTITY_JUMP_ANIMATION_ID:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->jump.animation_id = temp_int;
			}

			break;

		case _ENTITY_JUMP_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->jumping = temp_int;
			}

			break;

		case _ENTITY_JUMP_VELOCITY_X:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->jump.velocity.x = temp_float;
			}

			break;

		case _ENTITY_JUMP_VELOCITY_Y:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->jump.velocity.y = temp_float;
			}

			break;

		case _ENTITY_JUMP_VELOCITY_Z:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->jump.velocity.z = temp_float;
			}

			break;

		case _ENTITY_KNOCKDOWN_COUNT:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->knockdowncount = temp_int;
			}

			break;

		case _ENTITY_KNOCKDOWN_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->knockdowntime = temp_int;
			}

			break;

		case _ENTITY_LAST_DAMAGE_TYPE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->last_damage_type = temp_int;
			}

			break;

		case _ENTITY_LAST_HIT:

			handle->lasthit = (entity *)varlist[ARG_VALUE]->ptrVal;

			break;

		case _ENTITY_LIFESPAN:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->lifespancountdown = temp_int;
			}

			break;

		case _ENTITY_LINK:

			handle->link = (entity *)varlist[ARG_VALUE]->ptrVal;

			break;

		case _ENTITY_MODEL:

			handle->model = (s_model *)varlist[ARG_VALUE]->ptrVal;

			break;

		case _ENTITY_MODEL_DATA:

			// Read only.

			break;

		case _ENTITY_MODEL_DEFAULT:

			handle->defaultmodel = (s_model *)varlist[ARG_VALUE]->ptrVal;

			break;

		case _ENTITY_MOVE_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->nextmove = temp_int;
			}

			break;

		case _ENTITY_MOVE_X:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->movex = temp_float;
			}

			break;

		case _ENTITY_MOVE_Z:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->movez = temp_float;
			}

			break;

		case _ENTITY_MP:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->energy_state.mp_current = temp_int;
			}

			break;

		case _ENTITY_MP_OLD:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->energy_state.mp_old = temp_int;
			}

			break;

		case _ENTITY_MP_CHARGE_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->mpchargetime = temp_int;
			}

			break;

		case _ENTITY_MP_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->magictime = temp_int;
			}

			break;

		case _ENTITY_NAME:

			if (varlist[ARG_VALUE]->vt == VT_STR)
			{
				strcpy(handle->name, (char *)StrCache_Get(varlist[ARG_VALUE]->strVal));
			}

			break;

		case _ENTITY_NEXT_ATTACK_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->nextattack = temp_int;
			}

			break;

		case _ENTITY_NEXT_HIT_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->next_hit_time = temp_int;
			}

			break;

		case _ENTITY_NOGRAB:
		
			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->nograb = temp_int;
			}

			break;

		case _ENTITY_NOGRAB_DEFAULT:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->nograb_default = temp_int;
			}

			break;

		case _ENTITY_OBSTRUCTED:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->hitwall = temp_int;
			}

			break;

		case _ENTITY_OBSTRUCTION_OVERHEAD:

			handle->hithead = (entity *)varlist[ARG_VALUE]->ptrVal;

			break;

		case _ENTITY_OFFENSE_COLLECTION:

			handle->offense_factors = (float *)varlist[ARG_VALUE]->ptrVal;

			break;

        case _ENTITY_OPPONENT:

            handle->opponent = (entity *)varlist[ARG_VALUE]->ptrVal;

            break;

        case _ENTITY_OWNER:

            handle->owner = (entity *)varlist[ARG_VALUE]->ptrVal;

            break;

		case _ENTITY_PARENT:

			handle->parent = (entity *)varlist[ARG_VALUE]->ptrVal;

			break;

		case _ENTITY_PATH_OBSTRUCTED_WAIT:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->pathblocked = temp_int;
			}

			break;

		case _ENTITY_PAUSE_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->pausetime = temp_int;
			}

			break;

		case _ENTITY_PLATFORM_LAND:

			handle->parent = (entity *)varlist[ARG_VALUE]->ptrVal;

			break;

        case _ENTITY_PLAYER_INDEX:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->playerindex = temp_int;
            }

            break;

		case _ENTITY_POSITION_BASE:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->base = temp_float;
			}

			break;

        case _ENTITY_POSITION_BASE_ALTERNATE:

            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
            {
                handle->altbase = temp_float;
            }

            break;        

        case _ENTITY_POSITION_DIRECTION:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->direction = temp_int;
            }

            break;

		case _ENTITY_POSITION_X:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->position.x = temp_float;
			}

			break;

		case _ENTITY_POSITION_Y:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->position.y = temp_float;
			}

			break;

		case _ENTITY_POSITION_Z:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->position.z = temp_float;
			}

			break;

        case _ENTITY_PROJECTILE_PRIME:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->projectile_prime = temp_int;
            }

            break;

		case _ENTITY_RECURSIVE_DAMAGE:

			handle->recursive_damage = (s_damage_recursive *)varlist[ARG_VALUE]->ptrVal;

			break;

		case _ENTITY_RELEASE_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->releasetime = temp_int;
			}

			break;

		case _ENTITY_RISE_ATTACK_DELAY:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->staydown.riseattack = temp_int;
			}

			break;

		case _ENTITY_RISE_ATTACK_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->staydown.riseattack_stall = temp_int;
			}

			break;

		case _ENTITY_RISE_DELAY:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->staydown.rise = temp_int;
			}

			break;

		case _ENTITY_RISE_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->rising = temp_int;
			}

			break;

		case _ENTITY_RUN_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->running = temp_int;
			}

			break;

		case _ENTITY_RUSH:

			//  Read only.

			break;

		case _ENTITY_SCRIPT_COLLECTION:

			handle->scripts = (s_scripts *)varlist[ARG_VALUE]->ptrVal;

			break;

		case _ENTITY_SEAL_ENERGY:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->seal = temp_int;
			}

			break;

		case _ENTITY_SEAL_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->sealtime = temp_int;
			}

			break;

		case _ENTITY_SLEEP_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->sleeptime = temp_int;
			}

			break;

		case _ENTITY_SORT_ID:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->sortid = temp_int;
			}

			break;

		case _ENTITY_SPACE_OTHER:

			handle->collided_entity = (entity *)varlist[ARG_VALUE]->ptrVal;

			break;

        case _ENTITY_SPAWN_TYPE:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->spawntype = temp_int;
            }

            break;

		case _ENTITY_SPEED_MULTIPLIER:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->speedmul = temp_float;
			}

			break;

		case _ENTITY_STALL_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->stalltime = temp_int;
			}

			break;

		case _ENTITY_THINK_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->nextthink = temp_int;
			}

			break;
			
		case _ENTITY_TIMESTAMP:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->timestamp = temp_int;
			}

			break;

		case _ENTITY_TO_COST:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->tocost = temp_int;
			}

			break;

		case _ENTITY_TOSS_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->toss_time = temp_int;
			}

			break;

		case _ENTITY_TURN_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->turning = temp_int;
			}

			break;

		case _ENTITY_TURN_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->turntime = temp_int;
			}

			break;

		case _ENTITY_UPDATE_MARK:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->update_mark = temp_int;
			}

			break;

		case _ENTITY_VELOCITY_X:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->velocity.x = temp_float;
			}

			break;

		case _ENTITY_VELOCITY_Y:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->velocity.y = temp_float;
			}

			break;

		case _ENTITY_VELOCITY_Z:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->velocity.z = temp_float;
			}

			break;

		case _ENTITY_WALK_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->walking = temp_int;
			}

			break;

		case _ENTITY_WAYPOINT_COLLECTION:

			// Read only.

			break;

		case _ENTITY_WAYPOINT_COUNT:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->numwaypoints = temp_int;
			}

			break;

		case _ENTITY_WEAPON_ITEM:

			handle->weapent = (entity *)varlist[ARG_VALUE]->ptrVal;

			break;

        default:

            printf("Unsupported property.\n");
            goto error_local;

            break;
    }

    return result;

    // Error trapping.
    error_local:

    printf("You must provide a valid handle, property, and new value: " SELF_NAME "\n");

    result = E_FAIL;
    return result;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_HANDLE
    #undef ARG_PROPERTY
    #undef ARG_VALUE
}
