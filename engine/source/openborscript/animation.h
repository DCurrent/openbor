/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 - 2019 OpenBOR Team
 */

 // Entity Properties
 // 2018-04-02
 // Caskey, Damon V.

typedef enum
{		
	_ANIMATION_PROP_ATTACK_ONE, 
	_ANIMATION_PROP_BOUNCE_FACTOR,       
	_ANIMATION_PROP_CANCEL,      
	_ANIMATION_PROP_CHARGE_TIME,   
	_ANIMATION_PROP_COUNTER_ACTION_CONDITION,
	_ANIMATION_PROP_COUNTER_ACTION_FRAME_MAX,
	_ANIMATION_PROP_COUNTER_ACTION_FRAME_MIN,
	_ANIMATION_PROP_COUNTER_ACTION_TAKE_DAMAGE,
	_ANIMATION_PROP_DROP_FRAME,
	_ANIMATION_PROP_DROP_MODEL_INDEX,
	_ANIMATION_PROP_ENERGY_COST_AMOUNT,
	_ANIMATION_PROP_ENERGY_COST_DISABLE,
	_ANIMATION_PROP_ENERGY_COST_TYPE,
	_ANIMATION_PROP_FLIP_FRAME,
	_ANIMATION_PROP_FOLLOW_UP_ANIMATION_SELECT,
	_ANIMATION_PROP_FOLLOW_UP_CONDITION,
	_ANIMATION_PROP_FRAME_COUNT,
	_ANIMATION_PROP_HIT_COUNT,
	_ANIMATION_PROP_INDEX,
	_ANIMATION_PROP_JUMP_FRAME,
	_ANIMATION_PROP_JUMP_MODEL_INDEX,
	_ANIMATION_PROP_JUMP_VELOCITY_X,
	_ANIMATION_PROP_JUMP_VELOCITY_Y,
	_ANIMATION_PROP_JUMP_VELOCITY_Z,
	_ANIMATION_PROP_LAND_FRAME,
	_ANIMATION_PROP_LAND_MODEL_INDEX,
	_ANIMATION_PROP_LOOP_FRAME_END,
	_ANIMATION_PROP_LOOP_FRAME_START,
	_ANIMATION_PROP_LOOP_STATE,
	_ANIMATION_PROP_MODEL_INDEX,
	_ANIMATION_PROP_PROJECTILE,
	_ANIMATION_PROP_QUAKE_FRAME_START,
	_ANIMATION_PROP_QUAKE_MOVE_Y,
	_ANIMATION_PROP_QUAKE_REPEAT_COUNT,
	_ANIMATION_PROP_QUAKE_REPEAT_MAX,
	_ANIMATION_PROP_RANGE_BASE_MAX,
	_ANIMATION_PROP_RANGE_BASE_MIN,
	_ANIMATION_PROP_RANGE_X_MAX,
	_ANIMATION_PROP_RANGE_X_MIN,
	_ANIMATION_PROP_RANGE_Y_MAX,
	_ANIMATION_PROP_RANGE_Y_MIN,
	_ANIMATION_PROP_RANGE_Z_MAX,
	_ANIMATION_PROP_RANGE_Z_MIN,
	_ANIMATION_PROP_SIZE_X,
	_ANIMATION_PROP_SIZE_Y,
	_ANIMATION_PROP_SUB_ENTITY_MODEL_INDEX,
	_ANIMATION_PROP_SUB_ENTITY_SPAWN,
	_ANIMATION_PROP_SUB_ENTITY_SUMMON,
	_ANIMATION_PROP_SUB_ENTITY_UNSUMMON,
	_ANIMATION_PROP_SUBJECT_TO_GRAVITY,
	_ANIMATION_PROP_SYNC,				
	_ANIMATION_PROP_WEAPONFRAME,    
	_ANIMATION_PROP_END				// End cap. ALWAYS last!				
} e_animation_properties;

HRESULT openbor_get_animation_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount);
HRESULT openbor_set_animation_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount);

int mapstrings_animation_property(ScriptVariant** varlist, int paramCount);

// Animation frame properties

typedef enum
{
	_ANIMATION_FRAME_PROP_ATTACK,
	_ANIMATION_FRAME_PROP_BODY_COLLISION,
	_ANIMATION_FRAME_PROP_ENTITY_COLLISION,
	_ANIMATION_FRAME_PROP_DELAY,
	_ANIMATION_FRAME_PROP_DRAWMETHODS,
	_ANIMATION_FRAME_PROP_IDLE,
	_ANIMATION_FRAME_PROP_MOVE,	
	_ANIMATION_FRAME_PROP_OFFSET,
	_ANIMATION_FRAME_PROP_PLATFORM,
	_ANIMATION_FRAME_PROP_SHADOW,
	_ANIMATION_FRAME_PROP_SOUNDTOPLAY,	
	_ANIMATION_FRAME_PROP_SPRITE,
	_ANIMATION_FRAME_PROP_VULNERABLE,	
	_ANIMATION_FRAME_PROP_END			// End cap. ALWAYS last!
} e_animation_frame_properties;

HRESULT openbor_get_animation_frame_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount);
HRESULT openbor_set_animation_frame_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount);

int mapstrings_animation_frame_property(ScriptVariant** varlist, int paramCount);

// Sub entity properties.
typedef enum
{
	_SUB_ENTITY_PROP_FRAME,
	_SUB_ENTITY_PROP_PLACEMENT,
	_SUB_ENTITY_PROP_POSITION_X,
	_SUB_ENTITY_PROP_POSITION_Y,
	_SUB_ENTITY_PROP_POSITION_Z,
	_SUB_ENTITY_PROP_END			// End cap. ALWAYS last!
} e_sub_entity_properties;

HRESULT openbor_allocate_sub_entity(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount);
HRESULT openbor_get_sub_entity_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount);
HRESULT openbor_set_sub_entity_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount);

int mapstrings_sub_entity_property(ScriptVariant** varlist, int paramCount);