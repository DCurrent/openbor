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
	_ANIMATION_PROP_ANIMHITS,     // Does the attack need to hit before cancel is allowed?
	_ANIMATION_PROP_ANTIGRAV,     // UT: make dive a similar property as antigravity.
	_ANIMATION_PROP_ATTACK,
	_ANIMATION_PROP_COLLISIONONE,    // stick on the only one victim
	_ANIMATION_PROP_BODY_COLLISION,
	_ANIMATION_PROP_ENTITY_COLLISION,
	_ANIMATION_PROP_BOUNCE,       //FLOAT -tossv/bounce = new tossv
	_ANIMATION_PROP_CANCEL,       // Cancel anims with freespecial
	_ANIMATION_PROP_CHARGETIME,   //INT charge time for an animation
	_ANIMATION_PROP_COUNTERRANGE, //SUB Auto counter attack. 2011_04_01, DC: Moved to struct.
	_ANIMATION_PROP_DELAY,
	_ANIMATION_PROP_DRAWMETHODS,
	_ANIMATION_PROP_DROPFRAME,    // SUB if tossv < 0, this frame will be set
	_ANIMATION_PROP_DROPV,    // SUB if tossv < 0, this frame will be set
	_ANIMATION_PROP_ENERGYCOST,   //SUB. 1-10-05 to adjust the amount of energy used for specials. 2011_03_31, DC: Moved to struct.
	_ANIMATION_PROP_FLIPFRAME,    // Turns entities around on the desired frame
	_ANIMATION_PROP_FOLLOWUP,     // use which FOLLOW anim?
	_ANIMATION_PROP_IDLE,
	_ANIMATION_PROP_IGNOREATTACKID,
	_ANIMATION_PROP_INDEX,        //unique id
	_ANIMATION_PROP_JUMPFRAME,    //SUB
	_ANIMATION_PROP_LANDFRAME,    // SUB Landing behavior. 2011_04_01, DC: Moved to struct.
	_ANIMATION_PROP_LOOP,         // Animation looping. 2011_03_31, DC: Moved to struct.
	_ANIMATION_PROP_MODEL_INDEX,
	_ANIMATION_PROP_MOVE,
	_ANIMATION_PROP_NUMFRAMES,    //Framecount.
	_ANIMATION_PROP_OFFSET,
	_ANIMATION_PROP_PLATFORM,
	_ANIMATION_PROP_PROJECTILE,
	_ANIMATION_PROP_QUAKEFRAME,   // SUB Screen shake effect. 2011_04_01, DC; Moved to struct.
	_ANIMATION_PROP_RANGE,        //SUB Verify distance to target, jump landings, etc.. 2011_04_01, DC: Moved to struct.
	_ANIMATION_PROP_SHADOW,
	_ANIMATION_PROP_SIZE,         // SUB entity's size (height) during animation
	_ANIMATION_PROP_SOUNDTOPLAY,
	_ANIMATION_PROP_SPAWNFRAME,   // SUB Spawn the subentity as its default type. {frame} {x} {z} {a} {relative?}
	_ANIMATION_PROP_SPRITE,
	_ANIMATION_PROP_SPRITEA,
	_ANIMATION_PROP_SUBENTITY,    // Store the sub-entity's name for further use
	_ANIMATION_PROP_SUMMONFRAME,  // SUB Summon the subentity as an ally, only one though {frame} {x} {z} {a} {relative?}
	_ANIMATION_PROP_SYNC,         // sychronize frame to previous animation if they matches
	_ANIMATION_PROP_UNSUMMONFRAME,// SUB Un-summon the entity
	_ANIMATION_PROP_VULNERABLE,
	_ANIMATION_PROP_WEAPONFRAME,    // SUB Specify with a frame when to switch to a weapon model
	_ANIMATION_PROP_END				// End cap. ALWAYS last!
} e_animation_properties;

HRESULT openbor_get_animation_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount);
HRESULT openbor_set_animation_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount);

int mapstrings_animation_property(ScriptVariant** varlist, int paramCount);