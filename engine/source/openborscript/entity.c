/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c)  OpenBOR Team
 */

 #include "scriptcommon.h"

typedef struct {
	e_entity_properties property;
	const char* id_string;
	e_property_access_config_flags config_flags;
	size_t              offset;
	VARTYPE             type;
} entity_property_info;

#define PROPERTY_MEMBER_OFFSET(type, member) ((size_t)&(((type *)0)->member))

static const entity_property_info entity_properties[] = {
	{.property = ENTITY_PROPERTY_AI_DISABLE,
	.id_string = "ENTITY_PROPERTY_AI_DISABLE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, noaicontrol),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_AI_TARGET_ENTITY,
	.id_string = "ENTITY_PROPERTY_AI_TARGET_ENTITY",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, custom_target),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_ALTERNATE_IDLE,
	.id_string = "ENTITY_PROPERTY_ALTERNATE_IDLE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, idlemode),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_ALTERNATE_WALK,
	.id_string = "ENTITY_PROPERTY_ALTERNATE_WALK",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, walkmode),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_ANIMATION,
	.id_string = "ENTITY_PROPERTY_ANIMATION",
	.config_flags = PROPERTY_ACCESS_CONFIG_READ,
	.offset = PROPERTY_MEMBER_OFFSET(entity, animation),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_ANIMATION_FRAME,
	.id_string = "ENTITY_PROPERTY_ANIMATION_FRAME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, animpos),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_ANIMATION_ID,
	.id_string = "ENTITY_PROPERTY_ANIMATION_ID",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, animnum),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_ANIMATION_ID_PREVIOUS,
	.id_string = "ENTITY_PROPERTY_ANIMATION_ID_PREVIOUS",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, animnum_previous),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_ANIMATION_STATE,
	.id_string = "ENTITY_PROPERTY_ANIMATION_STATE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, animating),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_ANIMATION_TIME,
	.id_string = "ENTITY_PROPERTY_ANIMATION_TIME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, nextanim),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_ARROW_STATE,
	.id_string = "ENTITY_PROPERTY_ARROW_STATE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, arrowon),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_ATTACK_ID_INCOMING,
	.id_string = "ENTITY_PROPERTY_ATTACK_ID_INCOMING",
	.config_flags = PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER,
	.offset = PROPERTY_MEMBER_OFFSET(entity, attack_id_incoming),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_ATTACK_ID_OUTGOING,
	.id_string = "ENTITY_PROPERTY_ATTACK_ID_OUTGOING",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, attack_id_outgoing),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_ATTACK_STATE,
	.id_string = "ENTITY_PROPERTY_ATTACK_STATE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, attacking),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_AUTOKILL,
	.id_string = "ENTITY_PROPERTY_AUTOKILL",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, autokill),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_BACK_HIT_DIRECTION,
	.id_string = "ENTITY_PROPERTY_BACK_HIT_DIRECTION",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, normaldamageflipdir),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_BIND,
	.id_string = "ENTITY_PROPERTY_BIND",
	.config_flags = PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER,
	.offset = PROPERTY_MEMBER_OFFSET(entity, binding),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_BLAST_STATE,
	.id_string = "ENTITY_PROPERTY_BLAST_STATE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, projectile),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_BLINK,
	.id_string = "ENTITY_PROPERTY_BLINK",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, blink),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_BLOCK_STATE,
	.id_string = "ENTITY_PROPERTY_BLOCK_STATE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, blocking),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_BOSS,
	.id_string = "ENTITY_PROPERTY_BOSS",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, boss),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_CHARGE_STATE,
	.id_string = "ENTITY_PROPERTY_CHARGE_STATE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, charging),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_CHILD,
	.id_string = "ENTITY_PROPERTY_CHILD",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, subentity),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_COLORSET_DEFAULT,
	.id_string = "ENTITY_PROPERTY_COLORSET_DEFAULT",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, map),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_COLORSET_DYING_HEALTH_1,
	.id_string = "ENTITY_PROPERTY_COLORSET_DYING_HEALTH_1",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, per1),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_COLORSET_DYING_HEALTH_2,
	.id_string = "ENTITY_PROPERTY_COLORSET_DYING_HEALTH_2",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, per2),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_COLORSET_DYING_INDEX_1,
	.id_string = "ENTITY_PROPERTY_COLORSET_DYING_INDEX_1",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, dying),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_COLORSET_DYING_INDEX_2,
	.id_string = "ENTITY_PROPERTY_COLORSET_DYING_INDEX_2",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, dying2),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_COLORSET_TABLE,
	.id_string = "ENTITY_PROPERTY_COLORSET_TABLE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, colourmap),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_COLORSET_TIME,
	.id_string = "ENTITY_PROPERTY_COLORSET_TIME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, maptime),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_COMBO_STEP,
	.id_string = "ENTITY_PROPERTY_COMBO_STEP",
	.config_flags = PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER,
	.offset = PROPERTY_MEMBER_OFFSET(entity, combostep),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_COMBO_TIME,
	.id_string = "ENTITY_PROPERTY_COMBO_TIME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, combotime),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_COMMAND_TIME,
	.id_string = "ENTITY_PROPERTY_COMMAND_TIME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, command_time),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_DAMAGE_ON_LANDING,
	.id_string = "ENTITY_PROPERTY_DAMAGE_ON_LANDING",
	.config_flags = PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER,
	.offset = PROPERTY_MEMBER_OFFSET(entity, damage_on_landing),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_DEATH_STATE,
	.id_string = "ENTITY_PROPERTY_DEATH_STATE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, death_state),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_DEFENSE,
	.id_string = "ENTITY_PROPERTY_DEFENSE",
	.config_flags = PROPERTY_ACCESS_CONFIG_READ,
	.offset = PROPERTY_MEMBER_OFFSET(entity, defense),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_DESTINATION_X,
	.id_string = "ENTITY_PROPERTY_DESTINATION_X",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, destx),
	.type = VT_DECIMAL },

	{.property = ENTITY_PROPERTY_DESTINATION_Z,
	.id_string = "ENTITY_PROPERTY_DESTINATION_Z",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, destz),
	.type = VT_DECIMAL },

	{.property = ENTITY_PROPERTY_DIE_ON_LANDING,
	.id_string = "ENTITY_PROPERTY_DIE_ON_LANDING",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, die_on_landing),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_DRAWMETHOD,
	.id_string = "ENTITY_PROPERTY_DRAWMETHOD",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, drawmethod),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_DROP,
	.id_string = "ENTITY_PROPERTY_DROP",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, drop),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_DUCK_STATE,
	.id_string = "ENTITY_PROPERTY_DUCK_STATE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, ducking),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_ENTVAR_COLLECTION,
	.id_string = "ENTITY_PROPERTY_ENTVAR_COLLECTION",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, varlist),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_ESCAPE_COUNT,
	.id_string = "ENTITY_PROPERTY_ESCAPE_COUNT",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, escapecount),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_EXISTS,
	.id_string = "ENTITY_PROPERTY_EXISTS",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, exists),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_EXPLODE,
	.id_string = "ENTITY_PROPERTY_EXPLODE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, toexplode),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_FACTION,
	.id_string = "ENTITY_PROPERTY_FACTION",
	.config_flags = PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER,
	.offset = PROPERTY_MEMBER_OFFSET(entity, faction),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_FALL_STATE,
	.id_string = "ENTITY_PROPERTY_FALL_STATE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, falling),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_FREEZE_STATE,
	.id_string = "ENTITY_PROPERTY_FREEZE_STATE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, frozen),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_FREEZE_TIME,
	.id_string = "ENTITY_PROPERTY_FREEZE_TIME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, freezetime),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_FUNCTION_TAKE_ACTION,
	.id_string = "ENTITY_PROPERTY_FUNCTION_TAKE_ACTION",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, takeaction),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_FUNCTION_TAKE_DAMAGE,
	.id_string = "ENTITY_PROPERTY_FUNCTION_TAKE_DAMAGE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, takedamage),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_FUNCTION_THINK,
	.id_string = "ENTITY_PROPERTY_FUNCTION_THINK",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, think),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_FUNCTION_TRY_MOVE,
	.id_string = "ENTITY_PROPERTY_FUNCTION_TRY_MOVE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, trymove),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_GET_STATE,
	.id_string = "ENTITY_PROPERTY_GET_STATE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, getting),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_GRAB_TARGET,
	.id_string = "ENTITY_PROPERTY_GRAB_TARGET",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, grabbing),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_GRAB_WALK_STATE,
	.id_string = "ENTITY_PROPERTY_GRAB_WALK_STATE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, grabwalking),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_GUARD_POINTS,
	.id_string = "ENTITY_PROPERTY_GUARD_POINTS",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, guardpoints),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_GUARD_TIME,
	.id_string = "ENTITY_PROPERTY_GUARD_TIME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, guardtime),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_HP,
	.id_string = "ENTITY_PROPERTY_HP",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, energy_state.health_current),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_HP_OLD,
	.id_string = "ENTITY_PROPERTY_HP_OLD",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, energy_state.health_old),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_IDLE_STATE,
	.id_string = "ENTITY_PROPERTY_IDLE_STATE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, idling),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_IN_PAIN,
	.id_string = "ENTITY_PROPERTY_IN_PAIN",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, inpain),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_IN_PAIN_BACK,
	.id_string = "ENTITY_PROPERTY_IN_PAIN_BACK",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, inbackpain),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_INVINCIBLE_STATE,
	.id_string = "ENTITY_PROPERTY_INVINCIBLE_STATE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, invincible),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_INVINCIBLE_TIME,
	.id_string = "ENTITY_PROPERTY_INVINCIBLE_TIME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, invinctime),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_ITEM_DATA,
	.id_string = "ENTITY_PROPERTY_ITEM_DATA",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, item_properties),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_JUMP_ANIMATION_ID,
	.id_string = "ENTITY_PROPERTY_JUMP_ANIMATION_ID",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, jump.animation_id),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_JUMP_STATE,
	.id_string = "ENTITY_PROPERTY_JUMP_STATE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, jumping),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_JUMP_VELOCITY_X,
	.id_string = "ENTITY_PROPERTY_JUMP_VELOCITY_X",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, jump.velocity.x),
	.type = VT_DECIMAL },

	{.property = ENTITY_PROPERTY_JUMP_VELOCITY_Y,
	.id_string = "ENTITY_PROPERTY_JUMP_VELOCITY_Y",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, jump.velocity.y),
	.type = VT_DECIMAL },

	{.property = ENTITY_PROPERTY_JUMP_VELOCITY_Z,
	.id_string = "ENTITY_PROPERTY_JUMP_VELOCITY_Z",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, jump.velocity.z),
	.type = VT_DECIMAL },

	{.property = ENTITY_PROPERTY_KNOCKDOWN_COUNT,
	.id_string = "ENTITY_PROPERTY_KNOCKDOWN_COUNT",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, knockdowncount),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_KNOCKDOWN_TIME,
	.id_string = "ENTITY_PROPERTY_KNOCKDOWN_TIME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, knockdowntime),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_LAST_DAMAGE_TYPE,
	.id_string = "ENTITY_PROPERTY_LAST_DAMAGE_TYPE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, last_damage_type),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_LAST_HIT,
	.id_string = "ENTITY_PROPERTY_LAST_HIT",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, lasthit),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_LIFESPAN,
	.id_string = "ENTITY_PROPERTY_LIFESPAN",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, lifespancountdown),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_LINK,
	.id_string = "ENTITY_PROPERTY_LINK",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, link),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_MODEL,
	.id_string = "ENTITY_PROPERTY_MODEL",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, model),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_MODEL_DATA,
	.id_string = "ENTITY_PROPERTY_MODEL_DATA",
	.config_flags = PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER,
	.offset = PROPERTY_MEMBER_OFFSET(entity, modeldata),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_MODEL_DEFAULT,
	.id_string = "ENTITY_PROPERTY_MODEL_DEFAULT",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, defaultmodel),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_MOVE_TIME,
	.id_string = "ENTITY_PROPERTY_MOVE_TIME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, nextmove),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_MOVE_X,
	.id_string = "ENTITY_PROPERTY_MOVE_X",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, movex),
	.type = VT_DECIMAL },

	{.property = ENTITY_PROPERTY_MOVE_Z,
	.id_string = "ENTITY_PROPERTY_MOVE_Z",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, movez),
	.type = VT_DECIMAL },

	{.property = ENTITY_PROPERTY_MP,
	.id_string = "ENTITY_PROPERTY_MP",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, energy_state.mp_current),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_MP_CHARGE_TIME,
	.id_string = "ENTITY_PROPERTY_MP_CHARGE_TIME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, mpchargetime),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_MP_OLD,
	.id_string = "ENTITY_PROPERTY_MP_OLD",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, energy_state.mp_old),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_MP_TIME,
	.id_string = "ENTITY_PROPERTY_MP_TIME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, magictime),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_NAME,
	.id_string = "ENTITY_PROPERTY_NAME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT | PROPERTY_ACCESS_CONFIG_STATIC_LENGTH,
	.offset = PROPERTY_MEMBER_OFFSET(entity, name),
	.type = VT_STR },

	{.property = ENTITY_PROPERTY_NEXT_ATTACK_TIME,
	.id_string = "ENTITY_PROPERTY_NEXT_ATTACK_TIME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, nextattack),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_NEXT_HIT_TIME,
	.id_string = "ENTITY_PROPERTY_NEXT_HIT_TIME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, next_hit_time),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_NOGRAB,
	.id_string = "ENTITY_PROPERTY_NOGRAB",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, nograb),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_NOGRAB_DEFAULT,
	.id_string = "ENTITY_PROPERTY_NOGRAB_DEFAULT",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, nograb_default),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_OBSTRUCTED,
	.id_string = "ENTITY_PROPERTY_OBSTRUCTED",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, hitwall),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_OBSTRUCTION_OVERHEAD,
	.id_string = "ENTITY_PROPERTY_OBSTRUCTION_OVERHEAD",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, hithead),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_OFFENSE,
	.id_string = "ENTITY_PROPERTY_OFFENSE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, offense),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_OPPONENT,
	.id_string = "ENTITY_PROPERTY_OPPONENT",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, opponent),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_OWNER,
	.id_string = "ENTITY_PROPERTY_OWNER",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, owner),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_PARENT,
	.id_string = "ENTITY_PROPERTY_PARENT",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, parent),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_PATH_OBSTRUCTED_WAIT,
	.id_string = "ENTITY_PROPERTY_PATH_OBSTRUCTED_WAIT",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, pathblocked),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_PAUSE_TIME,
	.id_string = "ENTITY_PROPERTY_PAUSE_TIME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, pausetime),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_PLATFORM_LAND,
	.id_string = "ENTITY_PROPERTY_PLATFORM_LAND",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, landed_on_platform),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_PLAYER_INDEX,
	.id_string = "ENTITY_PROPERTY_PLAYER_INDEX",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, playerindex),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_POSITION_BASE,
	.id_string = "ENTITY_PROPERTY_POSITION_BASE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, base),
	.type = VT_DECIMAL },

	{.property = ENTITY_PROPERTY_POSITION_BASE_ALTERNATE,
	.id_string = "ENTITY_PROPERTY_POSITION_BASE_ALTERNATE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, altbase),
	.type = VT_DECIMAL },

	{.property = ENTITY_PROPERTY_POSITION_DIRECTION,
	.id_string = "ENTITY_PROPERTY_POSITION_DIRECTION",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, direction),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_POSITION_X,
	.id_string = "ENTITY_PROPERTY_POSITION_X",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, position.x),
	.type = VT_DECIMAL },

	{.property = ENTITY_PROPERTY_POSITION_Y,
	.id_string = "ENTITY_PROPERTY_POSITION_Y",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, position.y),
	.type = VT_DECIMAL },

	{.property = ENTITY_PROPERTY_POSITION_Z,
	.id_string = "ENTITY_PROPERTY_POSITION_Z",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, position.z),
	.type = VT_DECIMAL },

	{.property = ENTITY_PROPERTY_PROJECTILE_PRIME,
	.id_string = "ENTITY_PROPERTY_PROJECTILE_PRIME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, projectile_prime),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_RECURSIVE_EFFECT_COLLECTION,
    .id_string = "ENTITY_PROPERTY_RECURSIVE_EFFECT_COLLECTION",
    .config_flags = PROPERTY_ACCESS_CONFIG_READ,
    .offset = PROPERTY_MEMBER_OFFSET(entity, recursive_effect_collection),
    .type = VT_PTR },

	{.property = ENTITY_PROPERTY_RELEASE_TIME,
	.id_string = "ENTITY_PROPERTY_RELEASE_TIME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, releasetime),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_RISE_ATTACK_DELAY,
	.id_string = "ENTITY_PROPERTY_RISE_ATTACK_DELAY",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, staydown.riseattack),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_RISE_ATTACK_TIME,
	.id_string = "ENTITY_PROPERTY_RISE_ATTACK_TIME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, staydown.riseattack_stall),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_RISE_DELAY,
	.id_string = "ENTITY_PROPERTY_RISE_DELAY",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, staydown.rise),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_RISE_STATE,
	.id_string = "ENTITY_PROPERTY_RISE_STATE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, rising),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_RUN_STATE,
	.id_string = "ENTITY_PROPERTY_RUN_STATE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, running),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_RUSH,
	.id_string = "ENTITY_PROPERTY_RUSH",
	.config_flags = PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER,
	.offset = PROPERTY_MEMBER_OFFSET(entity, rush),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_SCRIPT,
	.id_string = "ENTITY_PROPERTY_SCRIPT",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, scripts),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_SEAL_ENERGY,
	.id_string = "ENTITY_PROPERTY_SEAL_ENERGY",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, seal),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_SEAL_TIME,
	.id_string = "ENTITY_PROPERTY_SEAL_TIME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, sealtime),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_SHADOW_CONFIG_FLAGS,
	.id_string = "ENTITY_PROPERTY_SHADOW_CONFIG_FLAGS",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, shadow_config_flags),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_SLEEP_TIME,
	.id_string = "ENTITY_PROPERTY_SLEEP_TIME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, sleeptime),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_SORT_ID,
	.id_string = "ENTITY_PROPERTY_SORT_ID",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, sortid),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_SPACE_OTHER,
	.id_string = "ENTITY_PROPERTY_SPACE_OTHER",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, collided_entity),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_SPAWN_TYPE,
	.id_string = "ENTITY_PROPERTY_SPAWN_TYPE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, spawntype),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_SPEED_MULTIPLIER,
	.id_string = "ENTITY_PROPERTY_SPEED_MULTIPLIER",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, speedmul),
	.type = VT_DECIMAL },

	{.property = ENTITY_PROPERTY_STALL_TIME,
	.id_string = "ENTITY_PROPERTY_STALL_TIME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, stalltime),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_THINK_TIME,
	.id_string = "ENTITY_PROPERTY_THINK_TIME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, nextthink),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_TIMESTAMP,
	.id_string = "ENTITY_PROPERTY_TIMESTAMP",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, timestamp),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_TO_COST,
	.id_string = "ENTITY_PROPERTY_TO_COST",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, tocost),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_TOSS_TIME,
	.id_string = "ENTITY_PROPERTY_TOSS_TIME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, toss_time),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_TURN_STATE,
	.id_string = "ENTITY_PROPERTY_TURN_STATE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, turning),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_TURN_TIME,
	.id_string = "ENTITY_PROPERTY_TURN_TIME",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, turntime),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_UPDATE_MARK,
	.id_string = "ENTITY_PROPERTY_UPDATE_MARK",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, update_mark),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_VELOCITY_X,
	.id_string = "ENTITY_PROPERTY_VELOCITY_X",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, velocity.x),
	.type = VT_DECIMAL },

	{.property = ENTITY_PROPERTY_VELOCITY_Y,
	.id_string = "ENTITY_PROPERTY_VELOCITY_Y",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, velocity.y),
	.type = VT_DECIMAL },

	{.property = ENTITY_PROPERTY_VELOCITY_Z,
	.id_string = "ENTITY_PROPERTY_VELOCITY_Z",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, velocity.z),
	.type = VT_DECIMAL },

	{.property = ENTITY_PROPERTY_WALK_STATE,
	.id_string = "ENTITY_PROPERTY_WALK_STATE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, walking),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_WAYPOINT,
	.id_string = "ENTITY_PROPERTY_WAYPOINT",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, waypoints),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_WAYPOINT_COUNT,
	.id_string = "ENTITY_PROPERTY_WAYPOINT_COUNT",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, numwaypoints),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_WEAPON_ITEM,
	.id_string = "ENTITY_PROPERTY_WEAPON_ITEM",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, weapent),
	.type = VT_PTR },

	{.property = ENTITY_PROPERTY_WEAPON_STATE,
	.id_string = "ENTITY_PROPERTY_WEAPON_STATE",
	.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT,
	.offset = PROPERTY_MEMBER_OFFSET(entity, weapon_state),
	.type = VT_INTEGER },

	{.property = ENTITY_PROPERTY_END,
	.id_string = "Entity",
	.config_flags = PROPERTY_ACCESS_CONFIG_NONE,
	.offset = 0,
	.type = VT_EMPTY }
};

#undef PROPERTY_MEMBER_OFFSET

const s_property_access_map entity_get_property_map(const void* acting_object_param, const unsigned int property_index_param)
{
	s_property_access_map property_map = { 0 };
	const entity* acting_object = acting_object_param;

	if (property_index_param < ENTITY_PROPERTY_END) {
		const entity_property_info* info = &entity_properties[property_index_param];
		property_map.config_flags = info->config_flags;
		property_map.id_string = info->id_string;
		property_map.field = (const void*)((const char*)acting_object + info->offset);
		property_map.type = info->type;
	}
	else {
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_NONE;
		property_map.id_string = "Entity";
		property_map.field = NULL;
		property_map.type = VT_EMPTY;
	}

	return property_map;
}

/*
* Caskey, Damon  V.
* 2023-03-03
*
* Return a property. Requires
* an object pointer and property
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
	* for population, then exit.
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
* id, and a new value.
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
* name, and a new value.
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
