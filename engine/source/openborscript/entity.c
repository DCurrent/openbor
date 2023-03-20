/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 - 2018 OpenBOR Team
 */

 #include "scriptcommon.h"

// Caskey, Damon  V.
// 2018-04-02
//
// Return an entity property. Requires
// an entity handle and property name to
// access.
HRESULT openbor_get_entity_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "openbor_get_entity_property(void entity, int property)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_OBJECT      0   // Handle (pointer to property structure).
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
       || varlist[ARG_OBJECT]->vt != VT_PTR
       || varlist[ARG_PROPERTY]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }
    else
    {
        // Populate local vars for readability.
        handle      = (entity *)varlist[ARG_OBJECT]->ptrVal;
        property    = (LONG)varlist[ARG_PROPERTY]->lVal;
    }
	
    switch(property)
    {
		
		case ENTITY_PROPERTY_AI_DISABLE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->noaicontrol;

			break;

        case ENTITY_PROPERTY_AI_TARGET_ENTITY:

            if(handle->custom_target)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (entity *)handle->custom_target;
            }

            break;

		case ENTITY_PROPERTY_ALTERNATE_IDLE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->idlemode;

			break;

		case ENTITY_PROPERTY_ALTERNATE_WALK:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->walkmode;

			break;

        case ENTITY_PROPERTY_ANIMATION:
            
            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (s_anim *)handle->animation;           

            break;

        case ENTITY_PROPERTY_ANIMATION_FRAME:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->animpos;

            break;

		case ENTITY_PROPERTY_ANIMATION_ID:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->animnum;

			break;

		case ENTITY_PROPERTY_ANIMATION_ID_PREVIOUS:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->animnum_previous;

			break;

		case ENTITY_PROPERTY_ANIMATION_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->animating;

			break;

		case ENTITY_PROPERTY_ANIMATION_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->nextanim;

			break;

        case ENTITY_PROPERTY_ARROW_STATE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->arrowon;

            break;

        case ENTITY_PROPERTY_ATTACK_ID_INCOMING:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (void*)handle->attack_id_incoming;

            break;

        case ENTITY_PROPERTY_ATTACK_ID_OUTGOING:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->attack_id_outgoing;

            break;

		case ENTITY_PROPERTY_ATTACK_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->attacking;

			break;

        case ENTITY_PROPERTY_AUTOKILL:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->autokill;

            break;

		case ENTITY_PROPERTY_BACK_HIT_DIRECTION:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->normaldamageflipdir;

			break;

        case ENTITY_PROPERTY_BIND:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (s_bind *)&handle->binding;

            break;

		case ENTITY_PROPERTY_BLAST_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->projectile;

			break;

        case ENTITY_PROPERTY_BLINK:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->blink;

            break;

        case ENTITY_PROPERTY_BLOCK_STATE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->blocking;

            break;

        case ENTITY_PROPERTY_BOSS:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->boss;

            break;

        case ENTITY_PROPERTY_CHARGE_STATE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->charging;

            break;

		case ENTITY_PROPERTY_CHILD:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (entity *)handle->subentity;

			break;

        case ENTITY_PROPERTY_COLORSET_DEFAULT:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->map;

            break;

        case ENTITY_PROPERTY_COLORSET_DYING_HEALTH_1:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->per1;

            break;

        case ENTITY_PROPERTY_COLORSET_DYING_HEALTH_2:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->per2;

            break;

        case ENTITY_PROPERTY_COLORSET_DYING_INDEX_1:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->dying;

            break;

        case ENTITY_PROPERTY_COLORSET_DYING_INDEX_2:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->dying2;

            break;

        case ENTITY_PROPERTY_COLORSET_TABLE:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)(handle->colourmap);

            break;

        case ENTITY_PROPERTY_COLORSET_TIME:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->maptime;

            break;

        case ENTITY_PROPERTY_COMBO_STEP:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)&handle->combostep;

            break;

        case ENTITY_PROPERTY_COMBO_TIME:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->combotime;

            break;

		case ENTITY_PROPERTY_COMMAND_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->movetime;

			break;

        case ENTITY_PROPERTY_DAMAGE_ON_LANDING:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (s_damage_on_landing *)&handle->damage_on_landing;

            break;

		case ENTITY_PROPERTY_DEAD:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->dead;

			break;
			
		case ENTITY_PROPERTY_DEFENSE_COLLECTION:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_defense *)handle->defense;

			break;

		case ENTITY_PROPERTY_DESTINATION_X:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->destx;

			break;

		case ENTITY_PROPERTY_DESTINATION_Z:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->destz;

			break;

		case ENTITY_PROPERTY_DIE_ON_LANDING:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->die_on_landing;

			break;

		case ENTITY_PROPERTY_DRAWMETHOD:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_drawmethod *)handle->drawmethod;

			break;

		case ENTITY_PROPERTY_DROP:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->drop;

			break;

		case ENTITY_PROPERTY_DUCK_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->ducking;

			break;

		case ENTITY_PROPERTY_ENTVAR_COLLECTION:

			if (handle->varlist)
			{
				ScriptVariant_ChangeType(*pretvar, VT_PTR);
				(*pretvar)->ptrVal = (Varlist *)handle->varlist;
			}

			break;

		case ENTITY_PROPERTY_ESCAPE_COUNT:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->escapecount;

			break;

		case ENTITY_PROPERTY_EXISTS:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->exists;

			break;

		case ENTITY_PROPERTY_EXPLODE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->toexplode;

			break;

		case ENTITY_PROPERTY_FACTION:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_faction *)&handle->faction;

			break;

		case ENTITY_PROPERTY_FALL_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->falling;

			break;

		case ENTITY_PROPERTY_FREEZE_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->frozen;

			break;

		case ENTITY_PROPERTY_FREEZE_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->freezetime;

			break;

		case ENTITY_PROPERTY_FUNCTION_TAKE_ACTION:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (VOID *)handle->takeaction;

			break;

		case ENTITY_PROPERTY_FUNCTION_TAKE_DAMAGE:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (VOID *)handle->takedamage;

			break;

		case ENTITY_PROPERTY_FUNCTION_THINK:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (VOID *)handle->think;

			break;

		case ENTITY_PROPERTY_FUNCTION_TRY_MOVE:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (VOID *)handle->trymove;

			break;

		case ENTITY_PROPERTY_GET_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->getting;

			break;

		case ENTITY_PROPERTY_GRAB_TARGET:

			if (handle->custom_target)
			{
				ScriptVariant_ChangeType(*pretvar, VT_PTR);
				(*pretvar)->ptrVal = (entity *)handle->grabbing;
			}

			break;

		case ENTITY_PROPERTY_GRAB_WALK_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->grabwalking;

			break;

		case ENTITY_PROPERTY_GUARD_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->guardtime;

			break;

		case ENTITY_PROPERTY_HP:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->energy_state.health_current;

			break;

		case ENTITY_PROPERTY_HP_OLD:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->energy_state.health_old;

			break;

		case ENTITY_PROPERTY_IDLE_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->idling;

			break;

		case ENTITY_PROPERTY_IN_PAIN:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->inpain;

			break;

		case ENTITY_PROPERTY_IN_PAIN_BACK:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->inbackpain;

			break;

		case ENTITY_PROPERTY_INVINCIBLE_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->invincible;

			break;

		case ENTITY_PROPERTY_INVINCIBLE_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->invinctime;

			break;

		case ENTITY_PROPERTY_ITEM_DATA:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_item_properties *)&handle->item_properties;

			break;

		case ENTITY_PROPERTY_JUMP_ANIMATION_ID:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->jump.animation_id;

			break;

		case ENTITY_PROPERTY_JUMP_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->jumping;

			break;

		case ENTITY_PROPERTY_JUMP_VELOCITY_X:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->jump.velocity.x;

			break;

		case ENTITY_PROPERTY_JUMP_VELOCITY_Y:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->jump.velocity.y;

			break;

		case ENTITY_PROPERTY_JUMP_VELOCITY_Z:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->jump.velocity.z;

			break;

		case ENTITY_PROPERTY_KNOCKDOWN_COUNT:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->knockdowncount;

			break;

		case ENTITY_PROPERTY_KNOCKDOWN_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->knockdowntime;

			break;

		case ENTITY_PROPERTY_LAST_DAMAGE_TYPE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->last_damage_type;

			break;

		case ENTITY_PROPERTY_LAST_HIT:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (entity *)handle->lasthit;

			break;

		case ENTITY_PROPERTY_LIFESPAN:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->lifespancountdown;

			break;

		case ENTITY_PROPERTY_LINK:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (entity *)handle->link;

			break;

		case ENTITY_PROPERTY_MODEL:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_model *)(handle->model);

			break;

		case ENTITY_PROPERTY_MODEL_DATA:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_model *)&handle->modeldata;

			break;

		case ENTITY_PROPERTY_MODEL_DEFAULT:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_model *)(handle->defaultmodel);

			break;

		case ENTITY_PROPERTY_MOVE_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->nextmove;

			break;

		case ENTITY_PROPERTY_MOVE_X:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->movex;

			break;

		case ENTITY_PROPERTY_MOVE_Z:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->movez;

			break;

		case ENTITY_PROPERTY_MP:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->energy_state.mp_current;

			break;

		case ENTITY_PROPERTY_MP_CHARGE_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->mpchargetime;

			break;

		case ENTITY_PROPERTY_MP_OLD:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->energy_state.mp_old;

			break;

		case ENTITY_PROPERTY_MP_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->magictime;

			break;

		case ENTITY_PROPERTY_NAME:

			ScriptVariant_ChangeType(*pretvar, VT_STR);
			(*pretvar)->strVal = StrCache_CreateNewFrom(handle->name);
			break;

		case ENTITY_PROPERTY_NEXT_ATTACK_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->nextattack;

			break;

		case ENTITY_PROPERTY_NEXT_HIT_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->next_hit_time;

			break;

		case ENTITY_PROPERTY_NOGRAB:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->nograb;

			break;

		case ENTITY_PROPERTY_NOGRAB_DEFAULT:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->nograb_default;

			break;

		case ENTITY_PROPERTY_OBSTRUCTED:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->hitwall;

			break;

		case ENTITY_PROPERTY_OBSTRUCTION_OVERHEAD:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (entity *)handle->hithead;

			break;

		case ENTITY_PROPERTY_OFFENSE_COLLECTION:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_offense *)handle->offense;

			break;

        case ENTITY_PROPERTY_OPPONENT:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (entity *)handle->opponent;

            break;

        case ENTITY_PROPERTY_OWNER:

            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (entity *)handle->owner;

            break;

		case ENTITY_PROPERTY_PARENT:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (entity *)handle->parent;

			break;

		case ENTITY_PROPERTY_PATH_OBSTRUCTED_WAIT:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->pathblocked;

			break;

		case ENTITY_PROPERTY_PAUSE_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->pausetime;

			break;

		case ENTITY_PROPERTY_PLATFORM_LAND:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (entity *)handle->landed_on_platform;

			break;

        case ENTITY_PROPERTY_PLAYER_INDEX:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->playerindex;

            break;

		case ENTITY_PROPERTY_POSITION_BASE:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->base;

			break;

        case ENTITY_PROPERTY_POSITION_BASE_ALTERNATE:

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)handle->altbase;

			break;

        case ENTITY_PROPERTY_POSITION_DIRECTION:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->direction;

            break;

		case ENTITY_PROPERTY_POSITION_X:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->position.x;

			break;

		case ENTITY_PROPERTY_POSITION_Y:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->position.y;

			break;

		case ENTITY_PROPERTY_POSITION_Z:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->position.z;

			break;

        case ENTITY_PROPERTY_PROJECTILE_PRIME:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->projectile_prime;

            break;

		case ENTITY_PROPERTY_RECURSIVE_DAMAGE:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_damage_recursive *)handle->recursive_damage;

			break;

		case ENTITY_PROPERTY_RELEASE_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->releasetime;

			break;

		case ENTITY_PROPERTY_RISE_ATTACK_DELAY:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->staydown.riseattack;

			break;

		case ENTITY_PROPERTY_RISE_ATTACK_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->staydown.riseattack_stall;

			break;

		case ENTITY_PROPERTY_RISE_DELAY:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->staydown.rise;

			break;

		case ENTITY_PROPERTY_RISE_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->rising;

			break;

		case ENTITY_PROPERTY_RUN_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->running;

			break;

		case ENTITY_PROPERTY_RUSH:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_rush *)&handle->rush;

			break;

		case ENTITY_PROPERTY_SCRIPT_COLLECTION:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_scripts *)handle->scripts;

			break;

		case ENTITY_PROPERTY_SEAL_ENERGY:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->seal;

			break;

		case ENTITY_PROPERTY_SEAL_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->sealtime;

			break;

		case ENTITY_PROPERTY_SHADOW_CONFIG_FLAGS:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (e_shadow_config_flags)handle->shadow_config_flags;

			break;

		case ENTITY_PROPERTY_SLEEP_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->sleeptime;

			break;

		case ENTITY_PROPERTY_SORT_ID:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->sortid;

			break;

		case ENTITY_PROPERTY_SPACE_OTHER:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (entity *)handle->collided_entity;

			break;

        case ENTITY_PROPERTY_SPAWN_TYPE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->spawntype;

            break;

		case ENTITY_PROPERTY_SPEED_MULTIPLIER:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->speedmul;

			break;

		case ENTITY_PROPERTY_STALL_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->stalltime;

			break;

		case ENTITY_PROPERTY_THINK_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->nextthink;

			break;

		case ENTITY_PROPERTY_TIMESTAMP:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->timestamp;

			break;

		case ENTITY_PROPERTY_TO_COST:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->tocost;

			break;

		case ENTITY_PROPERTY_TOSS_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->toss_time;

			break;

		case ENTITY_PROPERTY_TURN_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->turning;

			break;

		case ENTITY_PROPERTY_TURN_TIME:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->turntime;

			break;

		case ENTITY_PROPERTY_UPDATE_MARK:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->update_mark;

			break;

		case ENTITY_PROPERTY_VELOCITY_X:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->velocity.x;

			break;

		case ENTITY_PROPERTY_VELOCITY_Y:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->velocity.y;

			break;

		case ENTITY_PROPERTY_VELOCITY_Z:

			ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)handle->velocity.z;

			break;

		case ENTITY_PROPERTY_WALK_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->walking;

			break;

		case ENTITY_PROPERTY_WAYPOINT_COLLECTION:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (s_axis_plane_lateral_float *)handle->waypoints;

			break;

		case ENTITY_PROPERTY_WAYPOINT_COUNT:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->numwaypoints;

			break;

		case ENTITY_PROPERTY_WEAPON_ITEM:

			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (entity *)handle->weapent;

			break;

		case ENTITY_PROPERTY_WEAPON_STATE:

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)handle->weapon_state;

			break;

        default:

            printf("Unknwon property.\n");
            goto error_local;

            break;
    }

    return S_OK;

    error_local:

    printf("\nYou must provide a valid pointer and property constant: " SELF_NAME "\n");
    *pretvar = NULL;

    return E_FAIL;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_OBJECT
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
    #define SELF_NAME           "openbor_set_entity_property(void entity, int property, value)"
    #define ARG_MINIMUM         3   // Minimum required arguments.
    #define ARG_OBJECT          0   // Handle (pointer to property structure).
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
       || varlist[ARG_OBJECT]->vt != VT_PTR
       || varlist[ARG_PROPERTY]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }

    // Populate local handle and property vars.
    handle      = (entity *)varlist[ARG_OBJECT]->ptrVal;
    property    = (LONG)varlist[ARG_PROPERTY]->lVal;

    // Which property to modify?
    switch(property)
    {

		case ENTITY_PROPERTY_AI_DISABLE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->noaicontrol = temp_int;
			}

			break;

        case ENTITY_PROPERTY_AI_TARGET_ENTITY:

            handle->custom_target = (entity *)varlist[ARG_VALUE]->ptrVal;

            break;

		case ENTITY_PROPERTY_ALTERNATE_IDLE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->idlemode = temp_int;
			}

			break;

		case ENTITY_PROPERTY_ALTERNATE_WALK:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->walkmode = temp_int;
			}

			break;

        case ENTITY_PROPERTY_ANIMATION:

            handle->animation = (s_anim *)varlist[ARG_VALUE]->ptrVal;

            break;

        case ENTITY_PROPERTY_ANIMATION_FRAME:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->animpos = temp_int;
            }

            break;

		case ENTITY_PROPERTY_ANIMATION_ID:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->animnum = temp_int;
			}

			break; 

		case ENTITY_PROPERTY_ANIMATION_ID_PREVIOUS:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->animnum_previous = temp_int;
			}

			break;

		case ENTITY_PROPERTY_ANIMATION_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->animating = temp_int;
			}

			break;

		case ENTITY_PROPERTY_ANIMATION_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->nextanim = temp_int;
			}

			break;

        case ENTITY_PROPERTY_ARROW_STATE:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->arrowon = temp_int;
            }

            break;

        case ENTITY_PROPERTY_ATTACK_ID_INCOMING:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
				/*
				* 2021- 09-04. Property is now an array, so
				* so this is read only. Creator can change IDs
				* by getting pointer and modifying elements.
				*/ 
            }

            break;

        case ENTITY_PROPERTY_ATTACK_ID_OUTGOING:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->attack_id_outgoing = temp_int;
            }

            break;

		case ENTITY_PROPERTY_ATTACK_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->attacking = temp_int;
			}

			break;

        case ENTITY_PROPERTY_AUTOKILL:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->autokill = temp_int;
            }

            break;

		case ENTITY_PROPERTY_BACK_HIT_DIRECTION:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->normaldamageflipdir = temp_int;
			}

			break;

        case ENTITY_PROPERTY_BIND:

            // Read only.

            break;

		case ENTITY_PROPERTY_BLAST_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->projectile = temp_int;
			}

			break;

        case ENTITY_PROPERTY_BLINK:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->blink = temp_int;
            }

            break;

        case ENTITY_PROPERTY_BLOCK_STATE:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->blocking = temp_int;
            }

            break;

        case ENTITY_PROPERTY_BOSS:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->boss = temp_int;
            }

            break;

        case ENTITY_PROPERTY_CHARGE_STATE:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->charging = temp_int;
            }

            break;

		case ENTITY_PROPERTY_CHILD:

			handle->subentity = (entity *)varlist[ARG_VALUE]->ptrVal;

			break;

        case ENTITY_PROPERTY_COLORSET_DEFAULT:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->map = temp_int;
            }

            break;

        case ENTITY_PROPERTY_COLORSET_DYING_HEALTH_1:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->per1 = temp_int;
            }

            break;

        case ENTITY_PROPERTY_COLORSET_DYING_HEALTH_2:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->per2 = temp_int;
            }

            break;

        case ENTITY_PROPERTY_COLORSET_DYING_INDEX_1:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->dying = temp_int;
            }

            break;

        case ENTITY_PROPERTY_COLORSET_DYING_INDEX_2:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->dying2 = temp_int;
            }

            break;

        case ENTITY_PROPERTY_COLORSET_TABLE:

			handle->colourmap = (VOID *)varlist[ARG_VALUE]->ptrVal;

	        break;

        case ENTITY_PROPERTY_COLORSET_TIME:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->maptime = temp_int;
            }

            break;

        case ENTITY_PROPERTY_COMBO_STEP:

            // Read only.

            break;

        case ENTITY_PROPERTY_COMBO_TIME:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->combotime = temp_int;
            }

            break;

		case ENTITY_PROPERTY_COMMAND_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->movetime = temp_int;
			}

			break;

        case ENTITY_PROPERTY_DAMAGE_ON_LANDING:

            // Read only.

            break;

		case ENTITY_PROPERTY_DEAD:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->dead = temp_int;
			}

			break;

		case ENTITY_PROPERTY_DEFENSE_COLLECTION:

			handle->defense = (s_defense *)varlist[ARG_VALUE]->ptrVal;

			break;

		case ENTITY_PROPERTY_DESTINATION_X:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->destx = temp_float;
			}

			break;

		case ENTITY_PROPERTY_DESTINATION_Z:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->destz = temp_float;
			}

			break;

		case ENTITY_PROPERTY_DIE_ON_LANDING:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->die_on_landing = temp_int;
			}

			break;

		case ENTITY_PROPERTY_DRAWMETHOD:

			handle->drawmethod = (s_drawmethod *)varlist[ARG_VALUE]->ptrVal;

			break;

		case ENTITY_PROPERTY_DROP:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->drop = temp_int;
			}

			break;

		case ENTITY_PROPERTY_DUCK_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->ducking = temp_int;
			}

			break;

		case ENTITY_PROPERTY_ENTVAR_COLLECTION:

			handle->varlist = (Varlist *)varlist[ARG_VALUE]->ptrVal;

			break;

		case ENTITY_PROPERTY_ESCAPE_COUNT:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->escapecount = temp_int;
			}

			break;

		case ENTITY_PROPERTY_EXISTS:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->exists = temp_int;
			}

			break;

		case ENTITY_PROPERTY_EXPLODE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->toexplode = temp_int;
			}

			break;

		case ENTITY_PROPERTY_FACTION:
			
			// Read only. 

			break;

		case ENTITY_PROPERTY_FALL_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->falling = temp_int;
			}

			break;

		case ENTITY_PROPERTY_FREEZE_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->frozen = temp_int;
			}

			break;

		case ENTITY_PROPERTY_FREEZE_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->freezetime = temp_int;
			}

			break;

		case ENTITY_PROPERTY_FUNCTION_TAKE_ACTION:

			handle->takeaction = (VOID *)varlist[ARG_VALUE]->ptrVal;

			break;

		case ENTITY_PROPERTY_FUNCTION_TAKE_DAMAGE:

			handle->takedamage = (VOID *)varlist[ARG_VALUE]->ptrVal;

			break;

		case ENTITY_PROPERTY_FUNCTION_THINK:

			handle->think = (VOID *)varlist[ARG_VALUE]->ptrVal;

			break;

		case ENTITY_PROPERTY_FUNCTION_TRY_MOVE:

			handle->trymove = (VOID *)varlist[ARG_VALUE]->ptrVal;

			break;

		case ENTITY_PROPERTY_GET_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->getting = temp_int;
			}

			break;

		case ENTITY_PROPERTY_GRAB_TARGET:

			handle->grabbing = (entity *)varlist[ARG_VALUE]->ptrVal;

			break;

		case ENTITY_PROPERTY_GRAB_WALK_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->grabwalking = temp_int;
			}

			break;

		case ENTITY_PROPERTY_GUARD_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->guardtime = temp_int;
			}

			break;

		case ENTITY_PROPERTY_HP:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->energy_state.health_current = temp_int;
			}

			break;

		case ENTITY_PROPERTY_HP_OLD:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->energy_state.health_old = temp_int;
			}

			break;	

		case ENTITY_PROPERTY_IDLE_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->idling = temp_int;
			}

			break;

		case ENTITY_PROPERTY_IN_PAIN:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->inpain = temp_int;
			}

			break;

		case ENTITY_PROPERTY_IN_PAIN_BACK:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->inbackpain = temp_int;
			}

			break;

		case ENTITY_PROPERTY_INVINCIBLE_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->invincible = temp_int;
			}

			break;

		case ENTITY_PROPERTY_INVINCIBLE_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->invinctime = temp_int;
			}

			break;

		case ENTITY_PROPERTY_ITEM_DATA:

			// Read only.

			break;

		case ENTITY_PROPERTY_JUMP_ANIMATION_ID:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->jump.animation_id = temp_int;
			}

			break;

		case ENTITY_PROPERTY_JUMP_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->jumping = temp_int;
			}

			break;

		case ENTITY_PROPERTY_JUMP_VELOCITY_X:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->jump.velocity.x = temp_float;
			}

			break;

		case ENTITY_PROPERTY_JUMP_VELOCITY_Y:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->jump.velocity.y = temp_float;
			}

			break;

		case ENTITY_PROPERTY_JUMP_VELOCITY_Z:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->jump.velocity.z = temp_float;
			}

			break;

		case ENTITY_PROPERTY_KNOCKDOWN_COUNT:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->knockdowncount = temp_int;
			}

			break;

		case ENTITY_PROPERTY_KNOCKDOWN_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->knockdowntime = temp_int;
			}

			break;

		case ENTITY_PROPERTY_LAST_DAMAGE_TYPE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->last_damage_type = temp_int;
			}

			break;

		case ENTITY_PROPERTY_LAST_HIT:

			handle->lasthit = (entity *)varlist[ARG_VALUE]->ptrVal;

			break;

		case ENTITY_PROPERTY_LIFESPAN:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->lifespancountdown = temp_int;
			}

			break;

		case ENTITY_PROPERTY_LINK:

			handle->link = (entity *)varlist[ARG_VALUE]->ptrVal;

			break;

		case ENTITY_PROPERTY_MODEL:

			handle->model = (s_model *)varlist[ARG_VALUE]->ptrVal;

			break;

		case ENTITY_PROPERTY_MODEL_DATA:

			// Read only.

			break;

		case ENTITY_PROPERTY_MODEL_DEFAULT:

			handle->defaultmodel = (s_model *)varlist[ARG_VALUE]->ptrVal;

			break;

		case ENTITY_PROPERTY_MOVE_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->nextmove = temp_int;
			}

			break;

		case ENTITY_PROPERTY_MOVE_X:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->movex = temp_float;
			}

			break;

		case ENTITY_PROPERTY_MOVE_Z:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->movez = temp_float;
			}

			break;

		case ENTITY_PROPERTY_MP:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->energy_state.mp_current = temp_int;
			}

			break;

		case ENTITY_PROPERTY_MP_OLD:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->energy_state.mp_old = temp_int;
			}

			break;

		case ENTITY_PROPERTY_MP_CHARGE_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->mpchargetime = temp_int;
			}

			break;

		case ENTITY_PROPERTY_MP_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->magictime = temp_int;
			}

			break;

		case ENTITY_PROPERTY_NAME:

			if (varlist[ARG_VALUE]->vt == VT_STR)
			{
				strcpy(handle->name, (char *)StrCache_Get(varlist[ARG_VALUE]->strVal));
			}

			break;

		case ENTITY_PROPERTY_NEXT_ATTACK_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->nextattack = temp_int;
			}

			break;

		case ENTITY_PROPERTY_NEXT_HIT_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->next_hit_time = temp_int;
			}

			break;

		case ENTITY_PROPERTY_NOGRAB:
		
			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->nograb = temp_int;
			}

			break;

		case ENTITY_PROPERTY_NOGRAB_DEFAULT:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->nograb_default = temp_int;
			}

			break;

		case ENTITY_PROPERTY_OBSTRUCTED:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->hitwall = temp_int;
			}

			break;

		case ENTITY_PROPERTY_OBSTRUCTION_OVERHEAD:

			handle->hithead = (entity *)varlist[ARG_VALUE]->ptrVal;

			break;

		case ENTITY_PROPERTY_OFFENSE_COLLECTION:

			handle->offense = (s_offense *)varlist[ARG_VALUE]->ptrVal;

			break;

        case ENTITY_PROPERTY_OPPONENT:

            handle->opponent = (entity *)varlist[ARG_VALUE]->ptrVal;

            break;

        case ENTITY_PROPERTY_OWNER:

            handle->owner = (entity *)varlist[ARG_VALUE]->ptrVal;

            break;

		case ENTITY_PROPERTY_PARENT:

			handle->parent = (entity *)varlist[ARG_VALUE]->ptrVal;

			break;

		case ENTITY_PROPERTY_PATH_OBSTRUCTED_WAIT:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->pathblocked = temp_int;
			}

			break;

		case ENTITY_PROPERTY_PAUSE_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->pausetime = temp_int;
			}

			break;

		case ENTITY_PROPERTY_PLATFORM_LAND:

			handle->parent = (entity *)varlist[ARG_VALUE]->ptrVal;

			break;

        case ENTITY_PROPERTY_PLAYER_INDEX:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->playerindex = temp_int;
            }

            break;

		case ENTITY_PROPERTY_POSITION_BASE:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->base = temp_float;
			}

			break;

        case ENTITY_PROPERTY_POSITION_BASE_ALTERNATE:

            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
            {
                handle->altbase = temp_float;
            }

            break;        

        case ENTITY_PROPERTY_POSITION_DIRECTION:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->direction = temp_int;
            }

            break;

		case ENTITY_PROPERTY_POSITION_X:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->position.x = temp_float;
			}

			break;

		case ENTITY_PROPERTY_POSITION_Y:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->position.y = temp_float;
			}

			break;

		case ENTITY_PROPERTY_POSITION_Z:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->position.z = temp_float;
			}

			break;

        case ENTITY_PROPERTY_PROJECTILE_PRIME:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->projectile_prime = temp_int;
            }

            break;

		case ENTITY_PROPERTY_RECURSIVE_DAMAGE:

			handle->recursive_damage = (s_damage_recursive *)varlist[ARG_VALUE]->ptrVal;

			break;

		case ENTITY_PROPERTY_RELEASE_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->releasetime = temp_int;
			}

			break;

		case ENTITY_PROPERTY_RISE_ATTACK_DELAY:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->staydown.riseattack = temp_int;
			}

			break;

		case ENTITY_PROPERTY_RISE_ATTACK_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->staydown.riseattack_stall = temp_int;
			}

			break;

		case ENTITY_PROPERTY_RISE_DELAY:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->staydown.rise = temp_int;
			}

			break;

		case ENTITY_PROPERTY_RISE_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->rising = temp_int;
			}

			break;

		case ENTITY_PROPERTY_RUN_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->running = temp_int;
			}

			break;

		case ENTITY_PROPERTY_RUSH:

			//  Read only.

			break;

		case ENTITY_PROPERTY_SCRIPT_COLLECTION:

			handle->scripts = (s_scripts *)varlist[ARG_VALUE]->ptrVal;

			break;

		case ENTITY_PROPERTY_SEAL_ENERGY:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->seal = temp_int;
			}

			break;	

		case ENTITY_PROPERTY_SEAL_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->sealtime = temp_int;
			}

			break;

		case ENTITY_PROPERTY_SHADOW_CONFIG_FLAGS:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->shadow_config_flags = temp_int;
			}

			break;

		case ENTITY_PROPERTY_SLEEP_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->sleeptime = temp_int;
			}

			break;

		case ENTITY_PROPERTY_SORT_ID:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->sortid = temp_int;
			}

			break;

		case ENTITY_PROPERTY_SPACE_OTHER:

			handle->collided_entity = (entity *)varlist[ARG_VALUE]->ptrVal;

			break;

        case ENTITY_PROPERTY_SPAWN_TYPE:

            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
            {
                handle->spawntype = temp_int;
            }

            break;

		case ENTITY_PROPERTY_SPEED_MULTIPLIER:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->speedmul = temp_float;
			}

			break;

		case ENTITY_PROPERTY_STALL_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->stalltime = temp_int;
			}

			break;

		case ENTITY_PROPERTY_THINK_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->nextthink = temp_int;
			}

			break;
			
		case ENTITY_PROPERTY_TIMESTAMP:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->timestamp = temp_int;
			}

			break;

		case ENTITY_PROPERTY_TO_COST:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->tocost = temp_int;
			}

			break;

		case ENTITY_PROPERTY_TOSS_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->toss_time = temp_int;
			}

			break;

		case ENTITY_PROPERTY_TURN_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->turning = temp_int;
			}

			break;

		case ENTITY_PROPERTY_TURN_TIME:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->turntime = temp_int;
			}

			break;

		case ENTITY_PROPERTY_UPDATE_MARK:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->update_mark = temp_int;
			}

			break;

		case ENTITY_PROPERTY_VELOCITY_X:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->velocity.x = temp_float;
			}

			break;

		case ENTITY_PROPERTY_VELOCITY_Y:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->velocity.y = temp_float;
			}

			break;

		case ENTITY_PROPERTY_VELOCITY_Z:

			if (SUCCEEDED(ScriptVariant_DecimalValue(varlist[ARG_VALUE], &temp_float)))
			{
				handle->velocity.z = temp_float;
			}

			break;

		case ENTITY_PROPERTY_WALK_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->walking = temp_int;
			}

			break;

		case ENTITY_PROPERTY_WAYPOINT_COLLECTION:

			// Read only.

			break;

		case ENTITY_PROPERTY_WAYPOINT_COUNT:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->numwaypoints = temp_int;
			}

			break;

		case ENTITY_PROPERTY_WEAPON_ITEM:

			handle->weapent = (entity *)varlist[ARG_VALUE]->ptrVal;

			break;

		case ENTITY_PROPERTY_WEAPON_STATE:

			if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
			{
				handle->weapon_state = temp_int;
			}

			break;

        default:

            printf("Unsupported property.\n");
            goto error_local;

            break;
    }

    return result;

    // Error trapping.
    error_local:

    printf("\nYou must provide a valid pointer, property constant, and new value: " SELF_NAME "\n");

    result = E_FAIL;
    return result;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_OBJECT
    #undef ARG_PROPERTY
    #undef ARG_VALUE
}

/*
* Caskey, Damon V.
* 2021-08-04
*
* Return an attack ID array element value.
*/
HRESULT openbor_get_attack_id_value(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount)
{
#define SELF_NAME       "get_attack_id_value(void handle, int element)"
#define ARG_MINIMUM     2   // Minimum required arguments.
#define ARG_OBJECT      0   // Handle (pointer to property structure).
#define ARG_ELEMENT		1   // Array element to access.

	LONG* handle = NULL; // Property handle.
	int  element = 0;    // Property argument.

	/*
	* Clear pass by reference argument used to send
	* property data back to calling script.
	*/
	ScriptVariant_Clear(*pretvar);

	/*
	* Verify arguments.There should at least
	* be a pointer for the property handle and an integer
	* to determine which element of the array is accessed.
	*/
	if (paramCount < ARG_MINIMUM
		|| varlist[ARG_OBJECT]->vt != VT_PTR
		|| varlist[ARG_ELEMENT]->vt != VT_INTEGER)
	{
		*pretvar = NULL;
		goto error_local;
	}
	else
	{
		// Populate local vars for readability.
		handle = (LONG*)varlist[ARG_OBJECT]->ptrVal;
		element = (LONG)varlist[ARG_ELEMENT]->lVal;
	}

	/* Don't allow an out of bounds element. */
	if (element < 0 || element > MAX_ATTACK_IDS)
	{
		element = 0;
	}

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	(*pretvar)->lVal = (LONG)handle[element];

	return S_OK;

error_local:

	printf("\nYou must provide a valid handle and element: " SELF_NAME "\n");
	*pretvar = NULL;

	return E_FAIL;

#undef SELF_NAME
#undef ARG_MINIMUM
#undef ARG_ELEMENT
}

/*
* Caskey, Damon  V.
* 2018-04-03
*
* Mutate an entity property. Requires
* the entity handle, a string property
* name, and new value.
*/
HRESULT openbor_set_attack_id_value(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount)
{
#define SELF_NAME           "set_attack_id_value(void handle, char property, value)"
#define ARG_MINIMUM         3   // Minimum required arguments.
#define ARG_OBJECT          0   // Handle (pointer to property structure).
#define ARG_ELEMENT        1	// Element to access.
#define ARG_VALUE           2   // New value to apply.

	int     result = S_OK; // Success or error?
	LONG*	handle = NULL; // Property handle.
	int		element = 0;    // Array element to access.

	/*
	* Value carriers to apply on properties after
	* taken from argument.
	*/
	LONG    temp_int;

	/*
	* Verify incoming arguments. There should at least
	* be a pointer for the property handle and an integer
	* to determine which property is accessed.
	*/
	if (paramCount < ARG_MINIMUM
		|| varlist[ARG_OBJECT]->vt != VT_PTR
		|| varlist[ARG_ELEMENT]->vt != VT_INTEGER)
	{
		*pretvar = NULL;
		goto error_local;
	}

	// Populate local handle and property vars.
	handle = (LONG*)varlist[ARG_OBJECT]->ptrVal;
	element = (LONG)varlist[ARG_ELEMENT]->lVal;

	/* Don't allow an out of bounds element. */
	if (element < 0 || element > MAX_ATTACK_IDS)
	{
		element = 0;
	}

	if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
	{
		handle[element] = temp_int;
	}

	return result;

	/* Error trapping. */
error_local:

	printf("\nYou must provide a valid handle, element, and new value: " SELF_NAME "\n");

	result = E_FAIL;
	return result;

#undef SELF_NAME
#undef ARG_MINIMUM
#undef ARG_OBJECT
#undef ARG_ELEMENT
#undef ARG_VALUE
}
