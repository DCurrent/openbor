/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c)  OpenBOR Team
 */

 #include "scriptcommon.h"

const s_property_access_map entity_get_property_map(const void* acting_object_param, const unsigned int property_index_param)
{	
	s_property_access_map property_map;
	const entity* acting_object = acting_object_param;
	const e_entity_properties property_index = property_index_param;
	
	switch (property_index)
	{

	case ENTITY_PROPERTY_AI_DISABLE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->noaicontrol;
		property_map.id_string = "ENTITY_PROPERTY_AI_DISABLE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_AI_TARGET_ENTITY:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->custom_target;
		property_map.id_string = "ENTITY_PROPERTY_AI_TARGET_ENTITY";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_ALTERNATE_IDLE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->idlemode;
		property_map.id_string = "ENTITY_PROPERTY_ALTERNATE_IDLE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_ALTERNATE_WALK:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->walkmode;
		property_map.id_string = "ENTITY_PROPERTY_ALTERNATE_WALK";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_ANIMATION:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_READ;
		property_map.field = &acting_object->animation;
		property_map.id_string = "ENTITY_PROPERTY_ANIMATION";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_ANIMATION_FRAME:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->animpos;
		property_map.id_string = "ENTITY_PROPERTY_ANIMATION_FRAME";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_ANIMATION_ID:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->animnum;
		property_map.id_string = "ENTITY_PROPERTY_ANIMATION_ID";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_ANIMATION_ID_PREVIOUS:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->animnum_previous;
		property_map.id_string = "ENTITY_PROPERTY_ANIMATION_ID_PREVIOUS";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_ANIMATION_STATE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->animating;
		property_map.id_string = "ENTITY_PROPERTY_ANIMATION_STATE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_ANIMATION_TIME:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->nextanim;
		property_map.id_string = "ENTITY_PROPERTY_ANIMATION_TIME";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_ARROW_STATE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->arrowon;
		property_map.id_string = "ENTITY_PROPERTY_ARROW_STATE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_ATTACK_ID_INCOMING:

		property_map.config_flags = (PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER);
		property_map.field = &acting_object->attack_id_incoming;
		property_map.id_string = "ENTITY_PROPERTY_ATTACK_ID_INCOMING";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_ATTACK_ID_OUTGOING:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->attack_id_outgoing;
		property_map.id_string = "ENTITY_PROPERTY_ATTACK_ID_OUTGOING";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_ATTACK_STATE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->attacking;
		property_map.id_string = "ENTITY_PROPERTY_ATTACK_STATE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_AUTOKILL:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->autokill;
		property_map.id_string = "ENTITY_PROPERTY_AUTOKILL";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_BACK_HIT_DIRECTION:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->normaldamageflipdir;
		property_map.id_string = "ENTITY_PROPERTY_BACK_HIT_DIRECTION";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_BIND:

		property_map.config_flags = (PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER);
		property_map.field = &acting_object->binding;
		property_map.id_string = "ENTITY_PROPERTY_BIND";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_BLAST_STATE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->projectile;
		property_map.id_string = "ENTITY_PROPERTY_BLAST_STATE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_BLINK:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->blink;
		property_map.id_string = "ENTITY_PROPERTY_BLINK";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_BLOCK_STATE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->blocking;
		property_map.id_string = "ENTITY_PROPERTY_BLOCK_STATE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_BOSS:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->boss;
		property_map.id_string = "ENTITY_PROPERTY_BOSS";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_CHARGE_STATE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->charging;
		property_map.id_string = "ENTITY_PROPERTY_CHARGE_STATE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_CHILD:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->subentity;
		property_map.id_string = "ENTITY_PROPERTY_CHILD";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_COLORSET_DEFAULT:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->map;
		property_map.id_string = "ENTITY_PROPERTY_COLORSET_DEFAULT";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_COLORSET_DYING_HEALTH_1:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->per1;
		property_map.id_string = "ENTITY_PROPERTY_COLORSET_DYING_HEALTH_1";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_COLORSET_DYING_HEALTH_2:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->per2;
		property_map.id_string = "ENTITY_PROPERTY_COLORSET_DYING_HEALTH_2";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_COLORSET_DYING_INDEX_1:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->dying;
		property_map.id_string = "ENTITY_PROPERTY_COLORSET_DYING_INDEX_1";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_COLORSET_DYING_INDEX_2:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->dying2;
		property_map.id_string = "ENTITY_PROPERTY_COLORSET_DYING_INDEX_2";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_COLORSET_TABLE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->colourmap;
		property_map.id_string = "ENTITY_PROPERTY_COLORSET_TABLE";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_COLORSET_TIME:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->maptime;
		property_map.id_string = "ENTITY_PROPERTY_COLORSET_TIME";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_COMBO_STEP:

		property_map.config_flags = (PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER);
		property_map.field = &acting_object->combostep;
		property_map.id_string = "ENTITY_PROPERTY_COMBO_STEP";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_COMBO_TIME:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->combotime;
		property_map.id_string = "ENTITY_PROPERTY_COMBO_TIME";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_COMMAND_TIME:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->movetime;
		property_map.id_string = "ENTITY_PROPERTY_COMMAND_TIME";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_DAMAGE_ON_LANDING:

		property_map.config_flags = (PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER);
		property_map.field = &acting_object->damage_on_landing;
		property_map.id_string = "ENTITY_PROPERTY_DAMAGE_ON_LANDING";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_DEATH_STATE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->death_state;
		property_map.id_string = "ENTITY_PROPERTY_DEATH_STATE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_DEFENSE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_READ;
		property_map.field = &acting_object->defense;
		property_map.id_string = "ENTITY_PROPERTY_DEFENSE";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_DESTINATION_X:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->destx;
		property_map.id_string = "ENTITY_PROPERTY_DESTINATION_X";
		property_map.type = VT_DECIMAL;
		break;

	case ENTITY_PROPERTY_DESTINATION_Z:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->destz;
		property_map.id_string = "ENTITY_PROPERTY_DESTINATION_Z";
		property_map.type = VT_DECIMAL;
		break;

	case ENTITY_PROPERTY_DIE_ON_LANDING:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->die_on_landing;
		property_map.id_string = "ENTITY_PROPERTY_DIE_ON_LANDING";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_DRAWMETHOD:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->drawmethod;
		property_map.id_string = "ENTITY_PROPERTY_DRAWMETHOD";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_DROP:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->drop;
		property_map.id_string = "ENTITY_PROPERTY_DROP";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_DUCK_STATE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->ducking;
		property_map.id_string = "ENTITY_PROPERTY_DUCK_STATE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_ENTVAR_COLLECTION:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->varlist;
		property_map.id_string = "ENTITY_PROPERTY_ENTVAR_COLLECTION";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_ESCAPE_COUNT:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->escapecount;
		property_map.id_string = "ENTITY_PROPERTY_ESCAPE_COUNT";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_EXISTS:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->exists;
		property_map.id_string = "ENTITY_PROPERTY_EXISTS";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_EXPLODE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->toexplode;
		property_map.id_string = "ENTITY_PROPERTY_EXPLODE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_FACTION:

		property_map.config_flags = (PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER);
		property_map.field = &acting_object->faction;
		property_map.id_string = "ENTITY_PROPERTY_FACTION";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_FALL_STATE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->falling;
		property_map.id_string = "ENTITY_PROPERTY_FALL_STATE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_FREEZE_STATE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->frozen;
		property_map.id_string = "ENTITY_PROPERTY_FREEZE_STATE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_FREEZE_TIME:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->freezetime;
		property_map.id_string = "ENTITY_PROPERTY_FREEZE_TIME";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_FUNCTION_TAKE_ACTION:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->takeaction;
		property_map.id_string = "ENTITY_PROPERTY_FUNCTION_TAKE_ACTION";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_FUNCTION_TAKE_DAMAGE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->takedamage;
		property_map.id_string = "ENTITY_PROPERTY_FUNCTION_TAKE_DAMAGE";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_FUNCTION_THINK:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->think;
		property_map.id_string = "ENTITY_PROPERTY_FUNCTION_THINK";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_FUNCTION_TRY_MOVE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->trymove;
		property_map.id_string = "ENTITY_PROPERTY_FUNCTION_TRY_MOVE";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_GET_STATE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->getting;
		property_map.id_string = "ENTITY_PROPERTY_GET_STATE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_GRAB_TARGET:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->grabbing;
		property_map.id_string = "ENTITY_PROPERTY_GRAB_TARGET";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_GRAB_WALK_STATE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->grabwalking;
		property_map.id_string = "ENTITY_PROPERTY_GRAB_WALK_STATE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_GUARD_POINTS:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->guardpoints;
		property_map.id_string = "ENTITY_PROPERTY_GUARD_POINTS";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_GUARD_TIME:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->guardtime;
		property_map.id_string = "ENTITY_PROPERTY_GUARD_TIME";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_HP:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->energy_state.health_current;
		property_map.id_string = "ENTITY_PROPERTY_HP";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_HP_OLD:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->energy_state.health_old;
		property_map.id_string = "ENTITY_PROPERTY_HP_OLD";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_IDLE_STATE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->idling;
		property_map.id_string = "ENTITY_PROPERTY_IDLE_STATE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_IN_PAIN:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->inpain;
		property_map.id_string = "ENTITY_PROPERTY_IN_PAIN";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_IN_PAIN_BACK:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->inbackpain;
		property_map.id_string = "ENTITY_PROPERTY_IN_PAIN_BACK";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_INVINCIBLE_STATE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->invincible;
		property_map.id_string = "ENTITY_PROPERTY_INVINCIBLE_STATE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_INVINCIBLE_TIME:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->invinctime;
		property_map.id_string = "ENTITY_PROPERTY_INVINCIBLE_TIME";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_ITEM_DATA:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->item_properties;
		property_map.id_string = "ENTITY_PROPERTY_ITEM_DATA";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_JUMP_ANIMATION_ID:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->jump.animation_id;
		property_map.id_string = "ENTITY_PROPERTY_JUMP_ANIMATION_ID";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_JUMP_STATE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->jumping;
		property_map.id_string = "ENTITY_PROPERTY_JUMP_STATE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_JUMP_VELOCITY_X:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->jump.velocity.x;
		property_map.id_string = "ENTITY_PROPERTY_JUMP_VELOCITY_X";
		property_map.type = VT_DECIMAL;
		break;

	case ENTITY_PROPERTY_JUMP_VELOCITY_Y:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->jump.velocity.y;
		property_map.id_string = "ENTITY_PROPERTY_JUMP_VELOCITY_Y";
		property_map.type = VT_DECIMAL;
		break;

	case ENTITY_PROPERTY_JUMP_VELOCITY_Z:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->jump.velocity.z;
		property_map.id_string = "ENTITY_PROPERTY_JUMP_VELOCITY_Z";
		property_map.type = VT_DECIMAL;
		break;

	case ENTITY_PROPERTY_KNOCKDOWN_COUNT:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->knockdowncount;
		property_map.id_string = "ENTITY_PROPERTY_KNOCKDOWN_COUNT";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_KNOCKDOWN_TIME:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->knockdowntime;
		property_map.id_string = "ENTITY_PROPERTY_KNOCKDOWN_TIME";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_LAST_DAMAGE_TYPE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->last_damage_type;
		property_map.id_string = "ENTITY_PROPERTY_LAST_DAMAGE_TYPE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_LAST_HIT:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->lasthit;
		property_map.id_string = "ENTITY_PROPERTY_LAST_HIT";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_LIFESPAN:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->lifespancountdown;
		property_map.id_string = "ENTITY_PROPERTY_LIFESPAN";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_LINK:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->link;
		property_map.id_string = "ENTITY_PROPERTY_LINK";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_MODEL:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->model;
		property_map.id_string = "ENTITY_PROPERTY_MODEL";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_MODEL_DATA:

		property_map.config_flags = (PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER);
		property_map.field = &acting_object->modeldata;
		property_map.id_string = "ENTITY_PROPERTY_MODEL_DATA";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_MODEL_DEFAULT:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->defaultmodel;
		property_map.id_string = "ENTITY_PROPERTY_MODEL_DATA";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_MOVE_TIME:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->nextmove;
		property_map.id_string = "ENTITY_PROPERTY_MOVE_TIME";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_MOVE_X:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->movex;
		property_map.id_string = "ENTITY_PROPERTY_MOVE_X";
		property_map.type = VT_DECIMAL;
		break;

	case ENTITY_PROPERTY_MOVE_Z:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->movez;
		property_map.id_string = "ENTITY_PROPERTY_MOVE_Z";
		property_map.type = VT_DECIMAL;
		break;

	case ENTITY_PROPERTY_MP:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->energy_state.mp_current;
		property_map.id_string = "ENTITY_PROPERTY_MP";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_MP_CHARGE_TIME:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->mpchargetime;
		property_map.id_string = "ENTITY_PROPERTY_MP_CHARGE_TIME";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_MP_OLD:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->energy_state.mp_old;
		property_map.id_string = "ENTITY_PROPERTY_MP_OLD";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_MP_TIME:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->magictime;
		property_map.id_string = "ENTITY_PROPERTY_MP_TIME";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_NAME:
		property_map.config_flags = (PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT | PROPERTY_ACCESS_CONFIG_STATIC_LENGTH);
		property_map.field = &acting_object->name;
		property_map.id_string = "ENTITY_PROPERTY_NAME";
		property_map.type = VT_STR;
		break;

	case ENTITY_PROPERTY_NEXT_ATTACK_TIME:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->nextattack;
		property_map.id_string = "ENTITY_PROPERTY_NEXT_ATTACK_TIME";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_NEXT_HIT_TIME:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->next_hit_time;
		property_map.id_string = "ENTITY_PROPERTY_NEXT_HIT_TIME";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_NOGRAB:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->nograb;
		property_map.id_string = "ENTITY_PROPERTY_NOGRAB";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_NOGRAB_DEFAULT:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->nograb_default;
		property_map.id_string = "ENTITY_PROPERTY_NOGRAB_DEFAULT";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_OBSTRUCTED:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->hitwall;
		property_map.id_string = "ENTITY_PROPERTY_OBSTRUCTED";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_OBSTRUCTION_OVERHEAD:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->hithead;
		property_map.id_string = "ENTITY_PROPERTY_OBSTRUCTION_OVERHEAD";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_OFFENSE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->offense;
		property_map.id_string = "ENTITY_PROPERTY_OFFENSE";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_OPPONENT:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->opponent;
		property_map.id_string = "ENTITY_PROPERTY_OPPONENT";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_OWNER:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->owner;
		property_map.id_string = "ENTITY_PROPERTY_OWNER";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_PARENT:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->parent;
		property_map.id_string = "ENTITY_PROPERTY_PARENT";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_PATH_OBSTRUCTED_WAIT:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->pathblocked;
		property_map.id_string = "ENTITY_PROPERTY_PATH_OBSTRUCTED_WAIT";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_PAUSE_TIME:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->pausetime;
		property_map.id_string = "ENTITY_PROPERTY_PAUSE_TIME";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_PLATFORM_LAND:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->landed_on_platform;
		property_map.id_string = "ENTITY_PROPERTY_PLATFORM_LAND";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_PLAYER_INDEX:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->playerindex;
		property_map.id_string = "ENTITY_PROPERTY_PLAYER_INDEX";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_POSITION_BASE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->base;
		property_map.id_string = "ENTITY_PROPERTY_POSITION_BASE";
		property_map.type = VT_DECIMAL;
		break;

	case ENTITY_PROPERTY_POSITION_BASE_ALTERNATE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->altbase;
		property_map.id_string = "ENTITY_PROPERTY_POSITION_BASE_ALTERNATE";
		property_map.type = VT_DECIMAL;
		break;

	case ENTITY_PROPERTY_POSITION_DIRECTION:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->direction;
		property_map.id_string = "ENTITY_PROPERTY_POSITION_DIRECTION";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_POSITION_X:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->position.x;
		property_map.id_string = "ENTITY_PROPERTY_POSITION_X";
		property_map.type = VT_DECIMAL;
		break;

	case ENTITY_PROPERTY_POSITION_Y:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->position.y;
		property_map.id_string = "ENTITY_PROPERTY_POSITION_Y";
		property_map.type = VT_DECIMAL;
		break;

	case ENTITY_PROPERTY_POSITION_Z:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->position.z;
		property_map.id_string = "ENTITY_PROPERTY_POSITION_Z";
		property_map.type = VT_DECIMAL;
		break;

	case ENTITY_PROPERTY_PROJECTILE_PRIME:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->projectile_prime;
		property_map.id_string = "ENTITY_PROPERTY_PROJECTILE_PRIME";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_RECURSIVE_DAMAGE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->recursive_damage;
		property_map.id_string = "ENTITY_PROPERTY_RECURSIVE_DAMAGE";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_RELEASE_TIME:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->releasetime;
		property_map.id_string = "ENTITY_PROPERTY_RELEASE_TIME";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_RISE_ATTACK_DELAY:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->staydown.riseattack;
		property_map.id_string = "ENTITY_PROPERTY_RISE_ATTACK_DELAY";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_RISE_ATTACK_TIME:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->staydown.riseattack_stall;
		property_map.id_string = "ENTITY_PROPERTY_RISE_ATTACK_TIME";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_RISE_DELAY:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->staydown.rise;
		property_map.id_string = "ENTITY_PROPERTY_RISE_DELAY";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_RISE_STATE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->rising;
		property_map.id_string = "ENTITY_PROPERTY_RISE_STATE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_RUN_STATE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->running;
		property_map.id_string = "ENTITY_PROPERTY_RUN_STATE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_RUSH:

		property_map.config_flags = (PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER);
		property_map.field = &acting_object->rush;
		property_map.id_string = "ENTITY_PROPERTY_RUSH";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_SCRIPT:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->scripts;
		property_map.id_string = "ENTITY_PROPERTY_SCRIPT";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_SEAL_ENERGY:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->seal;
		property_map.id_string = "ENTITY_PROPERTY_SEAL_ENERGY";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_SEAL_TIME:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->sealtime;
		property_map.id_string = "ENTITY_PROPERTY_SEAL_TIME";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_SHADOW_CONFIG_FLAGS:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->shadow_config_flags;
		property_map.id_string = "ENTITY_PROPERTY_SHADOW_CONFIG_FLAGS";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_SLEEP_TIME:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->sleeptime;
		property_map.id_string = "ENTITY_PROPERTY_SLEEP_TIME";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_SORT_ID:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->sortid;
		property_map.id_string = "ENTITY_PROPERTY_SORT_ID";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_SPACE_OTHER:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->collided_entity;
		property_map.id_string = "ENTITY_PROPERTY_SPACE_OTHER";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_SPAWN_TYPE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->spawntype;
		property_map.id_string = "ENTITY_PROPERTY_SPAWN_TYPE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_SPEED_MULTIPLIER:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->speedmul;
		property_map.id_string = "ENTITY_PROPERTY_SPEED_MULTIPLIER";
		property_map.type = VT_DECIMAL;
		break;

	case ENTITY_PROPERTY_STALL_TIME:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->stalltime;
		property_map.id_string = "ENTITY_PROPERTY_STALL_TIME";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_THINK_TIME:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->nextthink;
		property_map.id_string = "ENTITY_PROPERTY_THINK_TIME";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_TIMESTAMP:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->timestamp;
		property_map.id_string = "ENTITY_PROPERTY_TIMESTAMP";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_TO_COST:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->tocost;
		property_map.id_string = "ENTITY_PROPERTY_TO_COST";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_TOSS_TIME:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->toss_time;
		property_map.id_string = "ENTITY_PROPERTY_TOSS_TIME";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_TURN_STATE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->turning;
		property_map.id_string = "ENTITY_PROPERTY_TURN_STATE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_TURN_TIME:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->turntime;
		property_map.id_string = "ENTITY_PROPERTY_TURN_TIME";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_UPDATE_MARK:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->update_mark;
		property_map.id_string = "ENTITY_PROPERTY_UPDATE_MARK";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_VELOCITY_X:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->velocity.x;
		property_map.id_string = "ENTITY_PROPERTY_VELOCITY_X";
		property_map.type = VT_DECIMAL;
		break;

	case ENTITY_PROPERTY_VELOCITY_Y:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->velocity.y;
		property_map.id_string = "ENTITY_PROPERTY_VELOCITY_Y";
		property_map.type = VT_DECIMAL;
		break;

	case ENTITY_PROPERTY_VELOCITY_Z:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->velocity.z;
		property_map.id_string = "ENTITY_PROPERTY_VELOCITY_Z";
		property_map.type = VT_DECIMAL;
		break;

	case ENTITY_PROPERTY_WALK_STATE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->walking;
		property_map.id_string = "ENTITY_PROPERTY_WALK_STATE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_WAYPOINT:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->waypoints;
		property_map.id_string = "ENTITY_PROPERTY_WAYPOINT";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_WAYPOINT_COUNT:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->numwaypoints;
		property_map.id_string = "ENTITY_PROPERTY_WAYPOINT_COUNT";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_WEAPON_ITEM:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->weapent;
		property_map.id_string = "ENTITY_PROPERTY_WEAPON_ITEM";
		property_map.type = VT_PTR;
		break;

	case ENTITY_PROPERTY_WEAPON_STATE:

		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->weapon_state;
		property_map.id_string = "ENTITY_PROPERTY_WEAPON_STATE";
		property_map.type = VT_INTEGER;
		break;

	case ENTITY_PROPERTY_END:
	default:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_NONE;
		property_map.field = NULL;
		property_map.id_string = "Entity";
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
HRESULT openbor_get_entity_property(const ScriptVariant* const* varlist, ScriptVariant** const pretvar, const int paramCount)
{
	const char* SELF_NAME = "openbor_get_entity_property(void entity, int property)";
	const unsigned int ARG_OBJECT = 0;
	const unsigned int ARG_PROPERTY = 1;

	/*
	* Clear pass by reference argument used to send
	* property data back to calling script.
	*/
	ScriptVariant_Clear(*pretvar);

	/*
	* Should at least be a pointer to the
	* acting object and a property id.
	*/
	if (varlist[ARG_OBJECT]->vt != VT_PTR
		|| varlist[ARG_PROPERTY]->vt != VT_INTEGER) {
		printf("\n\n Script error: %s. You must provide a valid object pointer and property id.\n\n", SELF_NAME);
		return E_FAIL;
	}

	/*
	* Now let's make sure the object type is
	* correct (ex. entity vs. model) so we
	* can shut down gracefully if there's
	* a mismatch.
	*/

	const entity* const acting_object = (const entity* const)varlist[ARG_OBJECT]->ptrVal;

	if (acting_object->object_type != OBJECT_TYPE_ENTITY) {
		printf("\n\nScript error: %s. Object pointer is not correct type.\n\n", SELF_NAME);
		*pretvar = NULL;
		return E_FAIL;
	}

	const int property_id_param = (const int)varlist[ARG_PROPERTY]->lVal;

	const e_entity_properties property_id = (e_entity_properties)(property_id_param);
	const s_property_access_map property_map = entity_get_property_map(acting_object, property_id);

	/*
	* If property id is in range, we send
	* the property map and return parameter
	* for population, then ext.
	*/

	if (property_id_param >= 0 && property_id_param < ENTITY_PROPERTY_END) {
		property_access_get_member(&property_map, *pretvar);
		return S_OK;
	}

	/*
	* Is this a dump request? If not, then
	* the property id is invalid.
	*/

	if (property_id_param == PROPERTY_ACCESS_DUMP) {
		property_access_dump_members(entity_get_property_map, ENTITY_PROPERTY_END, acting_object);
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
HRESULT openbor_set_entity_property(ScriptVariant** varlist, ScriptVariant** const pretvar, const int paramCount)
{
	const char* SELF_NAME = "openbor_set_entity_property(void entity, int property, <mixed> value)";
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

	const entity* const acting_object = (const entity* const)varlist[ARG_OBJECT]->ptrVal;

	if (acting_object->object_type != OBJECT_TYPE_ENTITY) {
		printf("\n\nScript error: %s. Object pointer is not correct type.\n\n", SELF_NAME);
		*pretvar = NULL;
		return E_FAIL;
	}

	const int property_id_param = (const int)varlist[ARG_PROPERTY]->lVal;
	const e_entity_properties property_id = (e_entity_properties)(property_id_param);

	if (property_id_param < 0 && property_id_param >= ENTITY_PROPERTY_END) {
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

	const s_property_access_map property_map = entity_get_property_map(acting_object, property_id);

	/*
	* Populate the property value on
	* acting object and return OK/FAIL.
	*/

	return property_access_set_member(acting_object, &property_map, varlist[ARG_VALUE]);

	return S_OK;
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
