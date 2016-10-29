/*
 * OpenBOR - http://www.chronocrash.com
 * -
 ----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

/////////////////////////////////////////////////////////////////////////////
//	Beats of Rage                                                          //
//	Side-scrolling beat-'em-up                                             //
/////////////////////////////////////////////////////////////////////////////

#ifndef OPENBOR_H
#define OPENBOR_H


/////////////////////////////////////////////////////////////////////////////

#include	"types.h"
#include	"video.h"
#include	"vga.h"
#include	"screen.h"
#include	"transform.h"
#include	"loadimg.h"
#include	"bitmap.h"
#include	"sprite.h"
#include	"spriteq.h"
#include	"font.h"
#include	"timer.h"
#include	"rand32.h"
#include	"sblaster.h"
#include	"soundmix.h"
#include	"control.h"
#include	"draw.h"
#include	"packfile.h"
#include	"palette.h"
#include	"anigif.h"
#include    "openborscript.h"
#include    "globals.h"
#include    "ram.h"
#include    "version.h"
#include    "savedata.h"

#ifdef SDL
#include    "gfx.h"
#endif

#ifdef WEBM
#include    "yuv.h"
#include    "vidplay.h"
#endif

/////////////////////////////////////////////////////////////////////////////

#define		DEFAULT_SHUTDOWN_MESSAGE \
			"OpenBOR " VERSION ", Compile Date: " __DATE__ "\n" \
			"Presented by the OpenBOR Team.\n" \
			"www.chronocrash.com\n"\
			"OpenBOR is the open source continuation of Beats of Rage by Senile Team.\n" \
			"\n" \
			"Special thanks to SEGA and SNK.\n\n"

#define		COMPATIBLEVERSION	0x00033747
#define		CV_SAVED_GAME		0x00033747
#define		CV_HIGH_SCORE		0x00033747
#define     GAME_SPEED          200
#define		THINK_SPEED			2
#define		COUNTER_SPEED		(GAME_SPEED*2)
#define		MAX_NAME_LEN		47
#define		MAX_ENTS			150
#define		MAX_SPECIALS		8					// Added for customizable freespecials
#define     MAX_SPECIAL_INPUTS  27                  // max freespecial input steps, MAX_SPECIAL_INPUTS-1 is reserved, MAX_SPECIAL_INPUTS-2 is animation index, MAX_SPECIAL_INPUTS-3 is reserved. OX -4 , -5 , -6 , -7 , -8 , -9 , -10 also for cancels
#define		MAX_ATCHAIN			12					// max attack chain length
#define     MAX_IDLES           1                   // Idle animations.
#define     MAX_WALKS           1                   // Walk animations.
#define     MAX_BACKWALKS       1                   // Backwalk animations.
#define     MAX_UPS             1                   // Walk up animations.
#define     MAX_DOWNS           1                   // Walk down animations.
#define		MAX_ATTACKS			4					// Total number of attacks players have
#define     MAX_FOLLOWS         4					// For followup animations
#define		MAX_ARG_LEN			511
#define		MAX_PAL_SIZE		1024
#define		MAX_CACHED_BACKGROUNDS 9
#define     MAX_DOTS            10                  // Max active dot effects.
#define     MAX_ARG_COUNT       64

/*
Note: the min Z coordinate of the player is important
for several other drawing operations.
movement restirctions are here!
*/

#define		FRONTPANEL_Z		(PLAYER_MAX_Z+50)
#define     HUD_Z               (FRONTPANEL_Z+10000)
#define		HOLE_Z				(PLAYER_MIN_Z-46)
#define		NEONPANEL_Z			(PLAYER_MIN_Z-47)
#define		SHADOW_Z			(PLAYER_MIN_Z-48)
#define		SCREENPANEL_Z		(PLAYER_MIN_Z-49)
#define		PANEL_Z				(PLAYER_MIN_Z-50)
#define		MIRROR_Z			(PLAYER_MIN_Z-5)
#define		PIT_DEPTH			-250
#define		P2_STATS_DIST		180
#define		CONTACT_DIST_H		30					// Distance to make contact
#define		CONTACT_DIST_V		12
#define		GRAB_DIST			36					// Grabbing ents will be placed this far apart.
#define		GRAB_STALL			(GAME_SPEED * 8 / 10)


// PLAY/REC INPUT vars
typedef struct InputKeys
{
    u32 prevkeys[MAX_PLAYERS];
    u32 keys[MAX_PLAYERS];
    u32 newkeys[MAX_PLAYERS];
    u32 releasekeys[MAX_PLAYERS];
    u32 playkeys[MAX_PLAYERS];
    u32 time;
    u32 interval;
    u32 synctime;
} RecKeys;

typedef enum
{
    A_REC_STOP,
    A_REC_REC,
    A_REC_PLAY,
    A_REC_FREE,
} a_recstatus;

typedef struct PlayRecStatus {
  char filename[MAX_ARG_LEN + 1];
  char path[MAX_ARG_LEN + 1];
  int status; // 0 = stop / 1 = rec / 2 = play
  int begin;
  u32 starttime;
  u32 endtime;
  u32 synctime; // used to sync rec time with game time
  u32 totsynctime;
  u32 cseed;
  unsigned long seed;
  FILE *handle;
  RecKeys *buffer;
} a_playrecstatus;

extern a_playrecstatus *playrecstatus;



typedef struct
{
    int x;
    int y;
    int font_index;
} s_debug_xy_msg;

typedef enum
{
    /*
    Key def enum.
    Damon V. Caskey
    2013-12-27
    */

    FLAG_ESC			= 0x00000001,
    FLAG_START			= 0x00000002,
    FLAG_MOVELEFT		= 0x00000004,
    FLAG_MOVERIGHT		= 0x00000008,
    FLAG_MOVEUP		    = 0x00000010,
    FLAG_MOVEDOWN		= 0x00000020,
    FLAG_ATTACK		    = 0x00000040,
    FLAG_JUMP			= 0x00000080,
    FLAG_SPECIAL		= 0x00000100,
    FLAG_SCREENSHOT	    = 0x00000200,
    FLAG_ATTACK2		= 0x00000400,
    FLAG_ATTACK3		= 0x00000800,
    FLAG_ATTACK4		= 0x00001000,
    FLAG_ANYBUTTON		= (FLAG_START|FLAG_SPECIAL|FLAG_ATTACK|FLAG_ATTACK2|FLAG_ATTACK3|FLAG_ATTACK4|FLAG_JUMP),
    FLAG_CONTROLKEYS    = (FLAG_SPECIAL|FLAG_ATTACK|FLAG_ATTACK2|FLAG_ATTACK3|FLAG_ATTACK4|FLAG_JUMP|FLAG_MOVEUP|FLAG_MOVEDOWN|FLAG_MOVELEFT|FLAG_MOVERIGHT),
    FLAG_FORWARD		= 0x40000000,
    FLAG_BACKWARD		= 0x80000000
} e_key_def;

typedef enum
{
    /*
    Key id enum.
    Damon V. Caskey
    2013-12-27
    */

    SDID_MOVEUP,
    SDID_MOVEDOWN,
    SDID_MOVELEFT,
    SDID_MOVERIGHT,
    SDID_ATTACK,
    SDID_ATTACK2,
    SDID_ATTACK3,
    SDID_ATTACK4,
    SDID_JUMP,
    SDID_SPECIAL,
    SDID_START,
    SDID_SCREENSHOT,
    SDID_ESC
} e_key_id;

typedef enum
{
    /*
    Entity type enumerator.
    Damon V. Caskey
    2013-12-27
    */

    TYPE_NONE,
    TYPE_PLAYER,
    TYPE_ENEMY,
    TYPE_ITEM      = 4,
    TYPE_OBSTACLE  = 8,
    TYPE_STEAMER	= 16,
    TYPE_SHOT		= 32,			// 7-1-2005 type to use for player projectiles
    TYPE_TRAP		= 64,			// 7-1-2005 lets face it enemies are going to just let you storm in without setting a trap or two!
    TYPE_TEXTBOX   = 128,			// New textbox type for displaying messages
    TYPE_ENDLEVEL  = 256,			// New endlevel type that ends the level when touched
    TYPE_NPC       = 512,          // A character can be an ally or enemy.
    TYPE_PANEL     = 1024,         // Fake panel, scroll with screen using model speed
    TYPE_MAX		= TYPE_PANEL,	// For openbor constant check and type hack (i.e., custom hostile and candamage)
    TYPE_RESERVED  = 0x40000000    // should not use as a type
} e_entity_type;

typedef enum
{
    /*
    Entity subtype enumerator.
    Damon V. Caskey
    2013-12-27
    */

    SUBTYPE_NONE,
    SUBTYPE_BIKER,
    SUBTYPE_NOTGRAB,
    SUBTYPE_ARROW,		//7-1-2005  subtype for an "enemy" that flies across the screen and dies
    SUBTYPE_TOUCH,		// ltb 1-18-05  new Item subtype for a more platformer feel.
    SUBTYPE_WEAPON,
    SUBTYPE_NOSKIP,		// Text type that can't be skipped
    SUBTYPE_FLYDIE,		// Now obstacles can be hit and fly like on Simpsons/TMNT
    SUBTYPE_BOTH,		// Used with TYPE_ENDLEVEL to force both players to reach the point before ending level
    SUBTYPE_PROJECTILE, // New weapon projectile type that can be picked up by players/enemies
    SUBTYPE_FOLLOW,     // Used by NPC character, if set, they will try to follow players
    SUBTYPE_CHASE       // Used by enemy always chasing you
} e_entity_type_sub;

//------------reserved for A.I. types-------------------------
typedef enum
{
    /*
    AI move 1 enum: Affects movement path
    Damon V. Caskey
    2013-12-27
    */

    AIMOVE1_NORMAL,                     // Current default style
    AIMOVE1_CHASE       = 0x00000001,   // alway move towards target, and can run to them if target is farway
    AIMOVE1_CHASEZ      = 0x00000002,   // only try to get close in z direction
    AIMOVE1_CHASEX      = 0x00000004,   // only try to get colse in x direction
    AIMOVE1_AVOID       = 0x00000008,   // try to avoid target
    AIMOVE1_AVOIDZ      = 0x00000010,   // only try to avoid target in z direction
    AIMOVE1_AVOIDX      = 0x00000020,   // only try to avoid target in x direction
    AIMOVE1_WANDER      = 0x00000040,   // ignore the target's position completely, wander everywhere, long idle time
    AIMOVE1_BIKER       = 0x00000080,   // move like a biker
    AIMOVE1_ARROW       = 0x00000100,   // fly like an arrow
    AIMOVE1_STAR        = 0x00000200,   // fly like a star, subject to ground
    AIMOVE1_BOMB        = 0x00000400,   // fly like a bomb, subject to ground/wall etc
    AIMOVE1_NOMOVE      = 0x00000800,   // don't move at all
    AIMOVE1_BOOMRANG    = 0x00001000,   // boomrang
    MASK_AIMOVE1        = 0x0000FFFF
} e_aimove_1;

typedef enum
{
    /*
    A.I move 2 enum: Affect terrain reflect
    Damon V. Caskey
    2013-12-27
    */

    AIMOVE2_NORMAL,                         // Current default style
    AIMOVE2_IGNOREHOLES     = 0x00010000,   // don't avoid holes
    AIMOVE2_NOTARGETIDLE    = 0x00020000,   // don't move when there's no target
    MASK_AIMOVE2            = 0xFFFF0000
} e_aimove_2;

typedef enum
{
    /*
    A.I. attack1 enum: Affect attacking style.
    Damon V. Caskey
    2013-12-27
    */

    AIATTACK1_NORMAL,                   // Current default style
    AIATTACK1_LONG      = 0x00000001,   // Long range first, not used
    AIATTACK1_MELEE     = 0x00000002,   // Melee attack first, not used
    AIATTACK1_NOATTACK  = 0x00000004,   // dont attack at all
    AIATTACK1_ALWAYS    = 0x00000008,   // more aggression than default, useful for traps who don't think
    MASK_AIATTACK1      = 0x0000FFFF
} e_aiattack_1;

typedef enum
{
    /*
    A.I. attack1 enum: Affect Defending style.
    Damon V. Caskey
    2013-12-27
    */

    AIATTACK2_NORMAL,                   // Current default style, don't dodge at all
    AIATTACK2_DODGE     = 0x00010000,   // Use dodge animation to avoid attack
    AIATTACK2_DODGEMOVE = 0x00020000,   // Try to move in z direction if a jump attack is about to hit him and try to step back if a melee attack is about to hit him.
    MASK_AIATTACK2      = 0xFFFF0000
} e_aiattack_2;

typedef enum //Animations
{
    /*
    Animations enum.
    Damon V. Caskey
    2013-12-27
    */

    ANI_IDLE,
    ANI_WALK,
    ANI_JUMP,
    ANI_LAND,
    ANI_PAIN,
    ANI_FALL,
    ANI_RISE,
    ANI_ATTACK,
    ANI_ATTACK1,
    ANI_ATTACK2,
    ANI_ATTACK3,
    ANI_ATTACK4,			// Very important
    ANI_UPPER,
    ANI_BLOCK,				// New block animation
    ANI_JUMPATTACK,
    ANI_JUMPATTACK2,
    ANI_GET,
    ANI_GRAB,
    ANI_GRABATTACK,
    ANI_GRABATTACK2,
    ANI_THROW,
    ANI_SPECIAL,
    ANI_FREESPECIAL,
    ANI_SPAWN, 				// 26-12-2004 new animation added here ani_spawn
    ANI_DIE,				// 29-12-2004 new animation added here ani_die
    ANI_PICK,				// 7-1-2005 used when players select their character at the select screen
    ANI_FREESPECIAL2,
    ANI_JUMPATTACK3,
    ANI_FREESPECIAL3,
    ANI_UP,					// Mar 2, 2005 - Animation for when going up
    ANI_DOWN,				// Mar 2, 2005 - Animation for when going down
    ANI_SHOCK,				// Animation played when knocked down by shock attack
    ANI_BURN,				// Animation played when knocked down by burn attack
    ANI_SHOCKPAIN,			// Animation played when not knocked down by shock attack
    ANI_BURNPAIN,			// Animation played when not knocked down by shock attack
    ANI_GRABBED,			// Animation played when grabbed
    ANI_SPECIAL2,			// Animation played for when pressing forward special
    ANI_RUN,				// Animation played when a player is running
    ANI_RUNATTACK,			// Animation played when a player is running and presses attack
    ANI_RUNJUMPATTACK,		// Animation played when a player is running and jumps and presses attack
    ANI_ATTACKUP,			// u u animation
    ANI_ATTACKDOWN,			// d d animation
    ANI_ATTACKFORWARD,		// f f animation
    ANI_ATTACKBACKWARD,		// Used for attacking backwards
    ANI_FREESPECIAL4,		// More freespecials added
    ANI_FREESPECIAL5,		// More freespecials added
    ANI_FREESPECIAL6,		// More freespecials added
    ANI_FREESPECIAL7,		// More freespecials added
    ANI_FREESPECIAL8,		// More freespecials added
    ANI_RISEATTACK,			// Attack used for enemies when players are crowding around after knocking them down
    ANI_DODGE,				// Used for up up / down down SOR3 dodge moves for players
    ANI_ATTACKBOTH,			// Used for when a player holds down attack and presses jump
    ANI_GRABFORWARD,		// New grab attack for when a player holds down forward/attack
    ANI_GRABFORWARD2,		// New second grab attack for when a player holds down forward/attack
    ANI_JUMPFORWARD,		// Attack when a player is moving and jumps
    ANI_GRABDOWN,			// Attack when a player has grabbed an opponent and presses down/attack
    ANI_GRABDOWN2,			// Attack when a player has grabbed an opponent and presses down/attack
    ANI_GRABUP,				// Attack when a player has grabbed an opponent and presses up/attack
    ANI_GRABUP2,			// Attack when a player has grabbed an opponent and presses up/attack
    ANI_SELECT,				// Animation that is displayed at the select screen
    ANI_DUCK,				// Animation that is played when pressing down in "platform" type levels
    ANI_FAINT,  			// Faint animations for players/enemys by tails
    ANI_CANT,  				// Can't animation for players(animation when mp is less than mpcost) by tails.
    ANI_THROWATTACK,		// Added for subtype projectile
    ANI_CHARGEATTACK,       // Plays when player releases attack1 after holding >= chargetime.
    ANI_JUMPCANT,
    ANI_JUMPSPECIAL,
    ANI_BURNDIE,
    ANI_SHOCKDIE,
    ANI_PAIN2,
    ANI_PAIN3,
    ANI_PAIN4,
    ANI_FALL2,
    ANI_FALL3,
    ANI_FALL4,
    ANI_DIE2,
    ANI_DIE3,
    ANI_DIE4,
    ANI_CHARGE,
    ANI_BACKWALK,
    ANI_SLEEP,
    ANI_FOLLOW1,
    ANI_FOLLOW2,
    ANI_FOLLOW3,
    ANI_FOLLOW4,
    ANI_PAIN5,
    ANI_PAIN6,
    ANI_PAIN7,
    ANI_PAIN8,
    ANI_PAIN9,
    ANI_PAIN10,
    ANI_FALL5,
    ANI_FALL6,
    ANI_FALL7,
    ANI_FALL8,
    ANI_FALL9,
    ANI_FALL10,
    ANI_DIE5,
    ANI_DIE6,
    ANI_DIE7,
    ANI_DIE8,
    ANI_DIE9,
    ANI_DIE10,
    ANI_TURN,               // turn back/flip
    ANI_RESPAWN,            //now spawn works for players
    ANI_FORWARDJUMP,
    ANI_RUNJUMP,
    ANI_JUMPLAND,
    ANI_JUMPDELAY,
    ANI_HITPLATFORM,
    ANI_HITWALL,
    ANI_GRABBACKWARD,
    ANI_GRABBACKWARD2,
    ANI_GRABWALK,
    ANI_GRABBEDWALK,
    ANI_GRABWALKUP,
    ANI_GRABBEDWALKUP,
    ANI_GRABWALKDOWN,
    ANI_GRABBEDWALKDOWN,
    ANI_GRABTURN,
    ANI_GRABBEDTURN,
    ANI_GRABBACKWALK,
    ANI_GRABBEDBACKWALK,
    ANI_SLIDE,              //Down + Jump animation.
    ANI_RUNSLIDE,           //Down + Jump while running.
    ANI_BLOCKPAIN,          //If entity has this, it will play in place of "pain" when it's blokcpain is 1 and incomming attack is blocked.
    ANI_DUCKATTACK,
    ANI_RISE2,
    ANI_RISE3,
    ANI_RISE4,
    ANI_RISE5,
    ANI_RISE6,
    ANI_RISE7,
    ANI_RISE8,
    ANI_RISE9,
    ANI_RISE10,
    ANI_RISEB,
    ANI_RISES,
    ANI_BLOCKPAIN2,
    ANI_BLOCKPAIN3,
    ANI_BLOCKPAIN4,
    ANI_BLOCKPAIN5,
    ANI_BLOCKPAIN6,
    ANI_BLOCKPAIN7,
    ANI_BLOCKPAIN8,
    ANI_BLOCKPAIN9,
    ANI_BLOCKPAIN10,
    ANI_BLOCKPAINB,
    ANI_BLOCKPAINS,
    ANI_CHIPDEATH,
    ANI_GUARDBREAK,
    ANI_RISEATTACK2,
    ANI_RISEATTACK3,
    ANI_RISEATTACK4,
    ANI_RISEATTACK5,
    ANI_RISEATTACK6,
    ANI_RISEATTACK7,
    ANI_RISEATTACK8,
    ANI_RISEATTACK9,
    ANI_RISEATTACK10,
    ANI_RISEATTACKB,
    ANI_RISEATTACKS,
    ANI_WALKOFF,
    ANI_BACKPAIN,
    ANI_BACKPAIN2,
    ANI_BACKPAIN3,
    ANI_BACKPAIN4,
    ANI_BACKPAIN5,
    ANI_BACKPAIN6,
    ANI_BACKPAIN7,
    ANI_BACKPAIN8,
    ANI_BACKPAIN9,
    ANI_BACKPAIN10,
    ANI_BACKFALL,
    ANI_BACKFALL2,
    ANI_BACKFALL3,
    ANI_BACKFALL4,
    ANI_BACKFALL5,
    ANI_BACKFALL6,
    ANI_BACKFALL7,
    ANI_BACKFALL8,
    ANI_BACKFALL9,
    ANI_BACKFALL10,
    ANI_BACKDIE,
    ANI_BACKDIE2,
    ANI_BACKDIE3,
    ANI_BACKDIE4,
    ANI_BACKDIE5,
    ANI_BACKDIE6,
    ANI_BACKDIE7,
    ANI_BACKDIE8,
    ANI_BACKDIE9,
    ANI_BACKDIE10,
    MAX_ANIS                // Maximum # of animations. This must always be last.
} e_animations;

typedef enum
{
    ANI_PROP_ANIMHITS,     // Does the attack need to hit before cancel is allowed?
    ANI_PROP_ANTIGRAV,     // UT: make dive a similar property as antigravity.
    ANI_PROP_ATTACK,
    ANI_PROP_ATTACKONE,    // stick on the only one victim
    ANI_PROP_BBOX,
    ANI_PROP_BOUNCE,       //FLOAT -tossv/bounce = new tossv
    ANI_PROP_CANCEL,       // Cancel anims with freespecial
    ANI_PROP_CHARGETIME,   //FLOAT charge time for an animation
    ANI_PROP_COUNTERRANGE, //SUB Auto counter attack. 2011_04_01, DC: Moved to struct.
    ANI_PROP_DELAY,
    ANI_PROP_DRAWMETHODS,
    ANI_PROP_DROPFRAME,    // SUB if tossv < 0, this frame will be set
    ANI_PROP_DROPV,    // SUB if tossv < 0, this frame will be set
    ANI_PROP_ENERGYCOST,   //SUB. 1-10-05 to adjust the amount of energy used for specials. 2011_03_31, DC: Moved to struct.
    ANI_PROP_FLIPFRAME,    // Turns entities around on the desired frame
    ANI_PROP_FOLLOWUP,     // use which FOLLOW anim?
    ANI_PROP_IDLE,
    ANI_PROP_INDEX,        //unique id
    ANI_PROP_JUMPFRAME,    //SUB
    ANI_PROP_LANDFRAME,    // SUB Landing behavior. 2011_04_01, DC: Moved to struct.
    ANI_PROP_LOOP,         // Animation looping. 2011_03_31, DC: Moved to struct.
    ANI_PROP_MODEL_INDEX,
    ANI_PROP_MOVE,
    ANI_PROP_NUMFRAMES,    //Framecount.
    ANI_PROP_OFFSET,
    ANI_PROP_PLATFORM,
    ANI_PROP_PROJECTILE,
    ANI_PROP_QUAKEFRAME,   // SUB Screen shake effect. 2011_04_01, DC; Moved to struct.
    ANI_PROP_RANGE,        //SUB Verify distance to target, jump landings, etc.. 2011_04_01, DC: Moved to struct.
    ANI_PROP_SHADOW,
    ANI_PROP_SIZE,         // SUB entity's size (height) during animation
    ANI_PROP_SOUNDTOPLAY,
    ANI_PROP_SPAWNFRAME,   // SUB Spawn the subentity as its default type. {frame} {x} {z} {a} {relative?}
    ANI_PROP_SPRITE,
    ANI_PROP_SPRITEA,
    ANI_PROP_SUBENTITY,    // Store the sub-entity's name for further use
    ANI_PROP_SUMMONFRAME,  // SUB Summon the subentity as an ally, only one though {frame} {x} {z} {a} {relative?}
    ANI_PROP_SYNC,         // sychronize frame to previous animation if they matches
    ANI_PROP_UNSUMMONFRAME,// SUB Un-summon the entity
    ANI_PROP_VULNERABLE,
    ANI_PROP_WEAPONFRAME    // SUB Specify with a frame when to switch to a weapon model
} e_animation_properties;

typedef enum
{
    /*
    Argument type enum.
    Damon V. Caskey
    2013-12-27
    */

    ARG_FLOAT,
    ARG_STRING,
    ARG_INT
} e_arg_types;

typedef enum
{
    /*
    Attack type enum
    Damon V. Caskey
    2013-12-27
    */

    ATK_NORMAL,
    ATK_NORMAL1			= ATK_NORMAL,
    ATK_NORMAL2,
    ATK_NORMAL3,
    ATK_NORMAL4,
    ATK_BLAST,
    ATK_BURN,
    ATK_FREEZE,
    ATK_SHOCK,
    ATK_STEAL,
    ATK_NORMAL5,
    ATK_NORMAL6,
    ATK_NORMAL7,
    ATK_NORMAL8,
    ATK_NORMAL9,
    ATK_NORMAL10,
    ATK_ITEM,
    ATK_LAND,
    ATK_PIT,
    ATK_LIFESPAN,
    ATK_TIMEOVER,
    MAX_ATKS,                       //Default max attack types (must be below all attack types in enum to get correct value)
    STA_ATKS        = (MAX_ATKS-10)
} e_attack_types;

// Attack box properties.
// Caskey, Damon V.
// 2016-10-26
typedef enum
{
    ATTACK_PROP_BLOCK_COST,
    ATTACK_PROP_BLOCK_PENETRATE,
    ATTACK_PROP_COUNTER,
    ATTACK_PROP_DAMAGE_FORCE,
    ATTACK_PROP_DAMAGE_LAND_FORCE,
    ATTACK_PROP_DAMAGE_LAND_MODE,
    ATTACK_PROP_DAMAGE_LETHAL_DISABLE,
    ATTACK_PROP_DAMAGE_RECURSIVE_FORCE,
    ATTACK_PROP_DAMAGE_RECURSIVE_INDEX,
    ATTACK_PROP_DAMAGE_RECURSIVE_MODE,
    ATTACK_PROP_DAMAGE_RECURSIVE_TIME_RATE,
    ATTACK_PROP_DAMAGE_RECURSIVE_TIME_EXPIRE,
    ATTACK_PROP_DAMAGE_STEAL,
    ATTACK_PROP_DAMAGE_TYPE,
    ATTACK_PROP_EFFECT_BLOCK_FLASH,
    ATTACK_PROP_EFFECT_BLOCK_SOUND,
    ATTACK_PROP_EFFECT_HIT_FLASH,
    ATTACK_PROP_EFFECT_HIT_FLASH_DISABLE,
    ATTACK_PROP_EFFECT_HIT_SOUND,
    ATTACK_PROP_INDEX,
    ATTACK_PROP_GROUND,
    ATTACK_PROP_MAP_INDEX,
    ATTACK_PROP_MAP_TIME,
    ATTACK_PROP_POSITION_X,
    ATTACK_PROP_POSITION_Y,
    ATTACK_PROP_REACTION_FALL_FORCE,
    ATTACK_PROP_REACTION_FALL_VELOCITY_X,
    ATTACK_PROP_REACTION_FALL_VELOCITY_Y,
    ATTACK_PROP_REACTION_FALL_VELOCITY_Z,
    ATTACK_PROP_REACTION_FREEZE_MODE,
    ATTACK_PROP_REACTION_FREEZE_TIME,
    ATTACK_PROP_REACTION_INVINCIBLE_TIME,
    ATTACK_PROP_REACTION_REPOSITION_DIRECTION,
    ATTACK_PROP_REACTION_REPOSITION_DISTANCE,
    ATTACK_PROP_REACTION_REPOSITION_MODE,
    ATTACK_PROP_REACTION_PAIN_SKIP,
    ATTACK_PROP_REACTION_PAUSE_TIME,
    ATTACK_PROP_SEAL_COST,
    ATTACK_PROP_SEAL_TIME,
    ATTACK_PROP_SIZE_X,
    ATTACK_PROP_SIZE_Y,
    ATTACK_PROP_SIZE_Z_1,
    ATTACK_PROP_SIZE_Z_2,
    ATTACK_PROP_STAYDOWN_RISE,
    ATTACK_PROP_STAYDOWN_RISEATTACK,
    ATTACK_PROP_TAG
} e_attack_properties;

typedef enum
{
    /*
    Status bar direction enum.
    Damon V. Caskey
    2013-12-29
    */

    BARSTATUS_DIR_NORMAL,  //Left to Right or Up to Down.
    BARSTATUS_DIR_INVERT   //Right to Left or Down to Up.
} e_bar_dir;

typedef enum
{
    HORIZONTALBAR,
    VERTICALBAR
} e_barorient;

typedef enum
{
    VALUEBAR,
    PERCENTAGEBAR
} e_bartype;

typedef enum
{
    BGT_BGLAYER,
    BGT_FGLAYER,
    BGT_PANEL,
    BGT_FRONTPANEL,
    BGT_WATER,
    BGT_BACKGROUND,
    BGT_GENERIC
} e_bgloldtype;

typedef enum
{
    /*
    Blocktype enum. Type of resource drained (if any) when attack is blocked.
    Damon V. Caskey
    2013-12-28
    */

    BLOCK_TYPE_HP         = -1,   //HP only.
    BLOCK_TYPE_MP_FIRST   = 1,    //MP until MP is exhuasted, then HP.
    BLOCK_TYPE_BOTH,              //Both MP and HP.
    BLOCK_TYPE_MP_ONLY            //Only MP, even if MP is 0.
} e_blocktype;

typedef enum
{
    /*
    Energy check type enum.
    Damon V. Caskey
    2013-12-29
    */

    COST_CHECK_HP,
    COST_CHECK_MP
} e_cost_check;

typedef enum
{
    /*
    Energycost type enum.
    Damon V. Caskey
    2013-12-29
    */

    COST_TYPE_MP_THEN_HP,
    COST_TYPE_MP_ONLY,
    COST_TYPE_HP_ONLY
} e_cost_type;

typedef enum
{
    /*
    Counter action conditionals.
    2012-12-16
    Damon V. Caskey
    */

    COUNTERACTION_CONDITION_NONE,                  //No counter.
    COUNTERACTION_CONDITION_ALWAYS,                //Always perform coutner action.
    COUNTERACTION_CONDITION_HOSTILE,               //Only if attacker is hostile entity.
    COUNTERACTION_CONDITION_HOSTILE_FRONT_NOFREEZE //Attacker is hostile, strikes from front, and uses non-freeze attack.
} e_counteraction_condition;

typedef enum
{
    /*
    Counteraction damage taking modes.
    2012-12-16
    Damon V. Caskey
    */

    COUNTERACTION_DAMAGE_NONE,  //No damage.
    COUNTERACTION_DAMAGE_NORMAL //Normal damage.
} e_counteraction_damage;

typedef enum
{
    /*
    Direction (facing) enum.
    Damon V. Caskey
    2013-12-16
    */

    DIRECTION_LEFT,
    DIRECTION_RIGHT
} e_direction;

typedef enum
{
    /*
    Direction adjustment enum. Used for binding and changing direction of defender when hit.
    Damon V. Caskey
    2013-12-28
    */

    DIRECTION_ADJUST_NONE,             //Leave as is.
    DIRECTION_ADJUST_SAME,             //Same as attacker/bind/etc.
    DIRECTION_ADJUST_OPPOSITE  = -1,   //Opposite attacker/bind/etc.
    DIRECTION_ADJUST_RIGHT     = 2,    //Always right.
    DIRECTION_ADJUST_LEFT      = -2    //Always left.
} e_direction_adjust;

typedef enum
{
    /*
    Run adjust_grabposition check on dograb or not.
    Damon V. Caskey
    2013-12-30
    */

    DOGRAB_ADJUSTCHECK_TRUE,
    DOGRAB_ADJUSTCHECK_FALSE
} e_dograb_adjustcheck;

typedef enum
{
    /*
    Damage over time mode enum.
    Damon V. Caskey
    2013-12-27
    */

    DOT_MODE_OFF,              //Disable.
    DOT_MODE_HP,               //Drain HP.
    DOT_MODE_HP_MP,            //Drain HP and MP.
    DOT_MODE_MP,               //Drain mp.
    DOT_MODE_NON_LETHAL_HP,    //Drain HP, but do not kill entity.
    DOT_MODE_NON_LETHAL_HP_MP  //Drain HP and MP, but do not kill entity.
} e_dot_mode;

typedef enum
{
    /*
    Edelay factor modes.
    2013-12-16
    Damon V. Caskey
    */
    EDELAY_MODE_ADD,       //Factor is added directly to edelay.
    EDELAY_MODE_MULTIPLY   //Orginal delay value is multiplied by factor.
} e_edelay_mode;

typedef enum
{
    /*
    Facing adjustment enum.
    Damon V. Caskey
    2013-12-29
    */

    FACING_ADJUST_NONE,    //No facing adjustment.
    FACING_ADJUST_RIGHT,   //Always face right.
    FACING_ADJUST_LEFT,    //Always face left.
    FACING_ADJUST_LEVEL    //Face according to level scroll direction.
} e_facing_adjust;

typedef enum
{
    /*
    Follow up conditional enumerator.
    Damon V. Caskey
    2014-01-04
    */

    FOLLOW_CONDITION_DISABLED,                     //No followup (default).
    FOLLOW_CONDITION_ALWAYS,                       //Always perform.
    FOLLOW_CONDITION_HOSTILE,                      //Perform if target is hostile.
    FOLLOW_CONDITION_HOSTILE_NOKILL_NOBLOCK,       //Perform if target is hostile, will not be killed and didn't block.
    FOLLOW_CONDITION_HOSTILE_NOKILL_NOBLOCK_NOGRAB //Perform if target is hostile, will not be killed, didn't block, and cannot be grabbed.
} e_follow_condition;

typedef enum
{
    /*
    Komap application enum. When to apply KO map to entity.
    Damon V. Caskey
    2013-12-28
    */

    KOMAP_TYPE_IMMEDIATELY,    //Apply instantly.
    KOMAP_TYPE_LAST_FALL_FRAME //Apply on last frame of fall.
} e_komap_type;

typedef enum
{
    LE_TYPE_NORMAL,
    LE_TYPE_CUT_SCENE,
    LE_TYPE_SELECT_SCREEN,
    LE_TYPE_SKIP_SELECT
} e_le_type;

typedef enum
{
    LS_TYPE_NONE,        //No loading screen.
    LS_TYPE_BOTH,        //Background and status bar.
    LS_TYPE_BACKGROUND,  //Background only.
    LS_TYPE_BAR,         //Status bar only.
} e_loadingScreenType;

typedef enum
{
    /*
    Model copy flag enum.
    Damon V. Caskey
    2013-12-28
    */

    MODEL_NO_COPY           = 0x00000001,   //dont copy anything from original model
    MODEL_NO_WEAPON_COPY    = 0x00000002,   //dont copy weapon list from original model
    MODEL_NO_SCRIPT_COPY    = 0x00000004    //don't copy scripts
} e_model_copy;

typedef enum
{
    MF_NONE,
    MF_ANIMLIST,
    MF_COLOURMAP,
    MF_PALETTE              = 4,
    MF_WEAPONS              = 8,
    MF_BRANCH               = 16,
    MF_ANIMATION            = 32,
    MF_DEFENSE              = 64,
    MF_OFF_FACTORS          = 128,
    MF_SPECIAL              = 256,
    MF_SMARTBOMB            = 512,
    MF_SCRIPTS              = 1024,
    MF_ALL                  = 0x7ff
} e_ModelFreetype;

typedef enum
{
    /*
    Over thr ground enum. Controls ability to hit downed targets.
    Damon V. Caskey
    2013-12-28
    */

   OTG_NONE,       //Cannot hit grounded targets.
   OTG_BOTH,       //Can hit grounded targets.
   OTG_GROUND_ONLY //Can ONLY hit grounded targets.
} e_otg;

typedef enum
{
    /*
    Scroll enum.
    Damon V. Caskey
    2013-12-28
    */

    SCROLL_RIGHT        = 2,
    SCROLL_DOWN			= 4,
    SCROLL_LEFT			= 8,
    SCROLL_UP			= 16,
    SCROLL_BACK			= 1,
    SCROLL_BOTH			= (SCROLL_BACK|SCROLL_RIGHT),
    SCROLL_RIGHTLEFT    = SCROLL_BOTH,
    SCROLL_LEFTRIGHT    = (SCROLL_LEFT|SCROLL_BACK),
    SCROLL_INWARD       = 32,
    SCROLL_OUTWARD      = 64,
    SCROLL_OUTIN		= (SCROLL_OUTWARD|SCROLL_BACK),
    SCROLL_INOUT		= (SCROLL_INWARD|SCROLL_BACK),
    SCROLL_UPWARD       = 128,
    SCROLL_DOWNWARD     = 256
} e_scroll;

typedef enum
{
    /*
    Slow motion switch enum.
    Damon V. Caskey
    2014-01-21
    */

    SLOW_MOTION_OFF,
    SLOW_MOTION_ON
} e_slow_motion_enable;

typedef enum
{
    /*
    Weapon loss type enum.
    Damon V. Caskey
    2013-12-29
    */

    WEAPLOSS_TYPE_ANY,         //Weapon lost taking any hit.
    WEAPLOSS_TYPE_KNOCKDOWN,   //Weapon lost on knockdown.
    WEAPLOSS_TYPE_DEATH,       //Weapon lost on death.
    WEAPLOSS_TYPE_CHANGE       //weapon is lost only when level ends or character is changed during continue. This depends on the level settings and whether players had weapons on start or not.
} e_weaploss_type;

//macros for drawing menu text, fits different font size
#define _strmidx(f,s, args...) ((videomodes.hRes-font_string_width((f), s, ##args))/2)
#define _colx(f,c) ((int)(videomodes.hRes/2+(c)*(fontmonowidth((f))+1)))
#define _liney(f,l) ((int)(videomodes.vRes/2+(l)*(fontheight((f))+1)))
#define _menutextm(f, l, shift, s, args...) font_printf(_strmidx(f,s, ##args)+(int)((shift)*(fontmonowidth((f))+1)), _liney(f,l), (f), 0, s, ##args)
#define _menutext(f, c, l, s, args...) font_printf(_colx(f,c), _liney(f,l), (f), 0, s, ##args)

//string starts with constant, for animation# series
#define strclen(s) (sizeof(s)-1)
#define starts_with(a, b) (strnicmp(a, b, strclen(b))==0)
#define starts_with_num(a, b) (starts_with(a, b) && (!a[strclen(b)] || (a[strclen(b)] >= '1' && a[strclen(b)] <= '9')))
#define get_tail_number(n, a, b) \
n = atoi(a+strclen(b)); \
if(n<1) n = 1;

#define lut_mul ((level && current_palette)?(level->blendings[current_palette-1][BLEND_MULTIPLY]):(blendings[BLEND_MULTIPLY]))
#define lut_screen ((level && current_palette)?(level->blendings[current_palette-1][BLEND_SCREEN]):(blendings[BLEND_SCREEN]))
#define lut_overlay ((level && current_palette)?(level->blendings[current_palette-1][BLEND_OVERLAY]):(blendings[BLEND_OVERLAY]))
#define lut_hl ((level && current_palette)?(level->blendings[current_palette-1][BLEND_HARDLIGHT]):(blendings[BLEND_HARDLIGHT]))
#define lut_dodge ((level && current_palette)?(level->blendings[current_palette-1][BLEND_DODGE]):(blendings[BLEND_DODGE]))
#define lut_half ((level && current_palette)?(level->blendings[current_palette-1][BLEND_HALF]):(blendings[BLEND_HALF]))
#define lut ((level && current_palette)?(level->blendings[current_palette-1]):(blendings))

#define ABS(x) ((x)>0?(x):(-(x)))

#define set_attacking(e) e->attacking = 1;\
						 e->idling = 0;

#define set_jumping(e)   e->jumping = 1;\
						 e->idling = 0;

#define set_charging(e)  e->charging = 1;\
						 e->idling = 0;

#define set_getting(e)   e->getting = 1;\
						 e->idling = 0;

#define set_blocking(e)  e->blocking = 1;\
						 e->idling = 0;

#define set_turning(e)  e->turning = 1;\
						e->idling = 0;

#define is_frozen(e)     ((textbox && e->modeldata.type != TYPE_TEXTBOX) || \
						 (smartbomber && e != smartbomber && e->modeldata.type != TYPE_TEXTBOX) ||(self->frozen&&self->freezetime>time))

#define expand_time(e)   if(e->stalltime>0) e->stalltime++;\
						 if(e->releasetime>0)e->releasetime++;\
						 if(e->nextanim>0)e->nextanim++;\
						 if(e->nextthink>0)e->nextthink++;\
						 if(e->nextmove>0)e->nextmove++;\
						 if(e->magictime>0)e->magictime++;\
						 if(e->guardtime>0)e->guardtime++;\
						 if(e->toss_time>0)e->toss_time++;\
						 if(e->freezetime>0 && (textbox || smartbomber))e->freezetime++;\
						 if(e->mpchargetime>0)e->mpchargetime++;\
						 if(e->invinctime>0) e->invinctime++;\
						 if(e->turntime>0) e->turntime++;\
						 if(e->sealtime>0) e->sealtime++;
/*                       if(e->dot_time>0) e->dot_time++;\
						 if(e->dot_cnt>0) e->dot_cnt++;
*/

#define freezeall        (smartbomber || textbox)

#define is_projectile(e) (e->modeldata.type == TYPE_SHOT || e->model->subtype == SUBTYPE_ARROW || e->owner)

#define screeny (level?((level->scrolldir == SCROLL_UP || level->scrolldir == SCROLL_DOWN )? 0:advancey ):0)
#define screenx (level?advancex:0)

#define check_range(self, target, animnum) \
		 ( target && \
		  (self->direction == DIRECTION_RIGHT ? \
		  (int)target->position.x >= self->position.x+self->modeldata.animation[animnum]->range.min.x &&\
		  (int)target->position.x <= self->position.x+self->modeldata.animation[animnum]->range.max.x\
		:\
		  (int)target->position.x <= self->position.x-self->modeldata.animation[animnum]->range.min.x &&\
		  (int)target->position.x >= self->position.x-self->modeldata.animation[animnum]->range.max.x)\
		  && (int)(target->position.z - self->position.z) >= self->modeldata.animation[animnum]->range.min.z \
		  && (int)(target->position.z - self->position.z) <= self->modeldata.animation[animnum]->range.max.z \
		  && (int)(target->position.y - self->position.y) >= self->modeldata.animation[animnum]->range.min.y \
		  && (int)(target->position.y - self->position.y) <= self->modeldata.animation[animnum]->range.max.y \
		  && (int)(target->base - self->base) >= self->modeldata.animation[animnum]->range.min.base \
		  && (int)(target->base - self->base) <= self->modeldata.animation[animnum]->range.max.base \
		  )\

#define check_range_both(self, target, animnum) \
		 ( target && \
		  (((int)target->position.x >= self->position.x+self->modeldata.animation[animnum]->range.min.x &&\
			(int)target->position.x <= self->position.x+self->modeldata.animation[animnum]->range.max.x)\
		||\
		   ((int)target->position.x <= self->position.x-self->modeldata.animation[animnum]->range.min.x &&\
			(int)target->position.x >= self->position.x-self->modeldata.animation[animnum]->range.max.x))\
		  && (int)(target->position.z - self->position.z) >= self->modeldata.animation[animnum]->range.min.z \
		  && (int)(target->position.z - self->position.z) <= self->modeldata.animation[animnum]->range.max.z \
		  && (int)(target->position.y - self->position.y) >= self->modeldata.animation[animnum]->range.min.y \
		  && (int)(target->position.y - self->position.y) <= self->modeldata.animation[animnum]->range.max.y \
		  && (int)(target->base - self->base) >= self->modeldata.animation[animnum]->range.min.base \
		  && (int)(target->base - self->base) <= self->modeldata.animation[animnum]->range.max.base \
		  )\


#define tobounce(e) (e->animation->bounce && diff(0, e->velocity.y) > 1.5 && \
					 !((autoland == 1 && e->damage_on_landing == -1) ||e->damage_on_landing == -2))

#define getpal ((current_palette&&level)?(level->palettes[current_palette-1]):pal)

#define canbegrabbed(self, other) \
		(other->animation->vulnerable[other->animpos] && \
		 (!self->animation->move || self->animation->move[self->animpos]->x == 0) && \
		 (!self->animation->move || self->animation->move[self->animpos]->z == 0 ) && \
		 !(other->nograb || other->invincible || other->link || \
		   other->model->animal || inair(other) || \
		  (self->modeldata.type == TYPE_PLAYER && other->modeldata.type == TYPE_PLAYER && savedata.mode)))

#define cangrab(self, other) \
		((other->modeldata.antigrab - self->modeldata.grabforce + \
		  (other->modeldata.paingrab?(other->modeldata.paingrab-other->inpain):0)<=0) &&\
		 canbegrabbed(self, other) && \
		 !inair(self) && \
		 diff(other->position.y, self->position.y) <= 0.1)

#define unfrozen(e) \
		ent_set_colourmap(e, e->map);\
		e->frozen = 0;\
		e->freezetime = 0;

#define validanim(e, a) ((e)->modeldata.animation[a]&&(e)->modeldata.animation[a]->numframes)

//#define     MAX_MOVES             16
//#define     MAX_MOVE_STEPS        16

#pragma pack (4)

typedef struct
{
    /*
    Axis structure for general coordinates and velocity use.
    2013-12-07
    Damon Caskey
    */

    float x;    //Horizontial axis.
    float y;    //Altitude/Vertical axis (Y).
    float z;    //Lateral axis.
} s_axis_f;

typedef struct
{
    /*
    Axis structure for general coordinates and velocity use when only X and Y are needed..
    2014-01-20
    Damon Caskey
    */

    float x;    //Horizontial axis.
    float y;    //Altitude/Vertical axis.
} s_axis_f_2d;

typedef struct
{
    /*
    Axis structure for general coordinates and velocity use when floats are not needed.
    2013-12-09
    Damon Caskey
    */

    int x;      //Horizontal axis.
    int y;      //Altitude/Vertical axis.
    int z;      //Lateral axis.
    int base;   //Base altitude.
} s_axis_i;

typedef struct
{
    /*
    Axis structure for general coordinates when only X and Y are needed.
    2014-01-04
    Damon Caskey
    */

    int x;      //Horizontal axis.
    int y;      //Altitude/Vertical axis.
} s_axis_i_2d;

typedef struct
{
    /*
    Axis structure for general coordinates when only X and Z are needed.
    2014-01-20
    Damon Caskey
    */

    int x;      //Horizontal axis.
    int z;      //Lateral axis.
} s_axis_i_plane;

typedef struct
{
    /*
    Min/max cap for integer measurements.
    2013-12-10
    Damon Caskey
    */

    int max;    //max value.
    int min;    //min value.
} s_metric_i;

typedef struct
{
    /*
    Min/current/max cap for integer measurements.
    2014-01-20
    Damon Caskey
    */

    int current;    //Current.
    int max;    //max value.
    int min;    //min value.
} s_metric_i_current;

typedef struct
{
    /*
    Last hit struct. Populated each time a collision is detected.
    Damon V. Caskey
    2013-12-15
    */

    int type;           //Attack type of last hit.
    s_axis_f position;  //X,Y,Z of last hit.
    int confirm;        //Will engine's default hit handling be used?
} s_lasthit;

typedef struct
{
    unsigned compatibleversion;
    char dName[MAX_NAME_LEN + 1]; // Difficulty Name
    unsigned level; // Level Number
    unsigned stage; // Stage
    unsigned pLives[MAX_PLAYERS]; // Player Lives Left
    unsigned pCredits[MAX_PLAYERS]; // Player Credits Left
    unsigned pScores[MAX_PLAYERS]; // Player Scores
    unsigned credits; // Number Of Credits
    unsigned times_completed;
    unsigned which_set;
    //-------------------new strict save features-----------------------
    int flag; // 0 useless slot 1 only load level number 2 load player info and level
    char pName[MAX_PLAYERS][MAX_NAME_LEN + 1];   // player names
    int pSpawnhealth[MAX_PLAYERS];              // hit points left
    int pSpawnmp[MAX_PLAYERS];                  // magic points left
    int pWeapnum[MAX_PLAYERS];                  // weapon
    int pColourmap[MAX_PLAYERS];                // colour map
    char allowSelectArgs[MAX_ARG_LEN + 1];      // allowselect arguments
} s_savelevel;

typedef struct
{
    unsigned compatibleversion;
    unsigned highsc[10];
    char hscoren[10][MAX_NAME_LEN + 1];
} s_savescore;

typedef struct
{
    /*
    Slow motion struct
    Damon V. Caskey
    2014-01-21
    */

    int duration;
    int counter;
    int toggle;
} s_slow_motion;

typedef struct
{
    /*
    Delay modifiers before rise or riseattack can take place.
    2011-04-08
    Damon V. Caskey
    */

    u32 rise;               //Time modifier before rise.
    u32 riseattack;         //Time modifier before riseattack.
    u32 riseattack_stall;   //Total stalltime before riseattack.
} s_staydown;

typedef struct
{
    /*
    Damage over time struct for attacker and defender.
    2013-12-15
    Damon V. Caskey
    */

    struct s_attack *attack;    //Attack data.
    int index;                  //Index (for multiple dots on single target).
    e_dot_mode mode;            //Dot mode.
    struct entity *owner;       //Entity causing dot effect.
    int rate;                   //Dot tick delay.
    u32 time_expire;            //Time to expire.
    u32 time_next;              //Time of next tick.
} s_dot;

typedef struct
{
	int x;
	int y;
	int width;
	int height;
	int z1;
	int z2;
} s_hitbox;

typedef struct
{
    s_hitbox    coords;
    //s_defense   *defense;
    int         tag;

} s_body;

typedef struct
{
    s_hitbox            attack_coords;
    int                 attack_drop;        // now be a knock-down factor, how many this attack will knock victim down
    int                 attack_force;
    int                 attack_type;        // Reaction animation, death, etc.
    int                 blast;              // Attack box active on hit opponent's fall animation.
    int                 blockflash;         // Custom bflash for each animation, model id
    int                 blocksound;         // Custom sound for when an attack is blocked
    int                 counterattack;      // Treat other attack boxes as body box.
    int                 damage_on_landing;  // same as throw damage type
    e_dot_mode          dot;                // Dot mode.
    int                 dot_force;          // Dot amount per tick.
    int                 dot_index;          // Dot index.
    int                 dot_rate;           // Dot tick delay.
    u32                 dot_time;           // Dot time to expire.
    s_axis_f            dropv;              // Velocity of target if knocked down.
    e_direction_adjust  force_direction;    // Adjust target's direction on hit.
    int                 forcemap;           // Set target's palette on hit.
    int                 freeze;             // Lock target in place and set freeze time.
    u32                 freezetime;         // Time for target to remain frozen.
    int                 grab;               // Not a grab as in grapple - behavior on hit for setting target's position
    float               grab_distance;      // Distance used by "grab".
    int                 guardcost;          // cost for blocking an attack
    int                 hitflash;           // Custom flash for each animation, model id
    int                 hitsound;           // Sound effect to be played when attack hits opponent
    int                 index;              // Possible future support of multiple boxes - it's doubt even if support is added this property will be needed.
    int                 jugglecost;         // cost for juggling a falling ent
    u32                 maptime;            // Time for forcemap to remain in effect.
    int                 no_block;           // If this is greater than defense block power, make the hit
    int                 no_flash;           // Flag to determine if an attack spawns a flash or not
    int                 no_kill;            // this attack won't kill target (leave 1 HP)
    int                 no_pain;            // No animation reaction on hit.
    e_otg               otg;                // Over The Ground. Gives ground projectiles the ability to hit lying ents.
    u32                 pain_time;          // pain invincible time
    int                 pause_add;          // Flag to determine if an attack adds a pause before updating the animation
    int                 seal;               // Disable target's animations with energycost > seal.
    u32                 sealtime;           // Time for seal to remain in effect.
    s_staydown          staydown;           // Modify victum's stayodwn properties.
    int                 steal;              // Add damage to owner's hp.
    int                 tag;                // Non engine use tag for scripting.
} s_attack;

typedef struct
{
    /*
    Counter action when taking hit.
    Damon V. Caskey
    2011-04-01
    */

    e_counteraction_condition condition; //Counter conditions.
    e_counteraction_damage damaged;      //Receive damage from attack.
    s_metric_i frame;   //Frame range.
} s_counterrange;

typedef struct
{
    /*
    HP and/or MP cost to perform special/freespecials.
    Damon V. Caskey
    2011-04-01
    */

    int cost;           //Amount of energy cost.
    int disable;        //Disable flag. See check_energy function.
    e_cost_type mponly; //MPonly type. 0 = MP while available, then HP. 1 = MP only. 2 = HP only.
} s_energycost;

typedef struct
{
    /*
    On frame movement (slide, jump, dive, etc.)
    2011-04-01
    Damon V. Caskey
    */

    int ent;            //Index of entity to spawn.
    int frame;          //Frame to perform action.
    s_axis_f velocity;  //x,a,z velocity.
} s_onframe;

typedef struct
{
    /*
    Animation looping.
    Damon V. Caskey
    2011-04-01
    */

    s_metric_i frame; //max = Frame animation reaches before looping, min = Frame animation loops back to.
    int mode; //0 = No loop, 1 = Loop. Redundant after frame additions, but needed for backward compatibility.
} s_loop;

typedef struct //2011_04_01, DC: Frame based screen shake functionality.
{
    int cnt;        //Repetition count.
    int framestart; //Frame to start quake.
    int repeat;     //Repetitons.
    int v;          //Vertical distance of screen movement (in pixels).
} s_quakeframe;

typedef struct
{
    /*
    Distance to target verification for AI running, jumping, following parent, and combo chains for all entity types.
    2011-04-01
    Damon V. Caskey
    */

    s_axis_i max;   //max ranges.
    s_axis_i min;   //min ranges.
} s_range;

typedef struct
{
    /*
    Model/entity level delay modifier.
    Damon V. Caskey
    (unknown date) revised 2013-12-16.
    */
    s_metric_i cap;
    float factor;
    e_edelay_mode mode;
    s_metric_i range;
} s_edelay;

typedef struct
{
    /*
    Follow up animation struct.
    Damon V. caskey
    2014-01-04
    */

    int animation;                  //Animation to perform.
    e_follow_condition condition;   //Condition in which follow up will be performed.
} s_follow;

typedef struct
{
    /*
    Projectile struct
    Damon V. Caskey
    2014-01-18
    */

    int bomb;           //custbomb;
    int flash;          //custpshotno;
    int knife;          //custknife;
    s_axis_i position;  //Location at which projectiles are spawned
    int star;           //custstar;
    int shootframe;
    int throwframe;
    int tossframe;      // Frame to toss bomb/grenade
} s_projectile;



typedef struct
{
    int             animhits;               // Does the attack need to hit before cancel is allowed?
    int             antigrav;               // UT: make dive a similar property as antigravity
    int             attackone;              // stick on the only one victim
    s_attack        **attacks;
    s_body          **body_collision;
    float           bounce;                 // -tossv/bounce = new tossv
    int             cancel;                 // Cancel anims with freespecial
    float           chargetime;             // charge time for an animation
    s_counterrange  counterrange;           // Auto counter attack. 2011_04_01, DC: Moved to struct.
    int             *delay;
    s_drawmethod    **drawmethods;
    s_onframe       dropframe;              // if tossv < 0, this frame will be set
    s_energycost    energycost;             // 1-10-05 to adjust the amount of energy used for specials. 2011_03_31, DC: Moved to struct.
    int             flipframe;              // Turns entities around on the desired frame
    s_follow        followup;               // Subsequent animation on hit.
    unsigned        *idle;                  // Allow free move
    int             index;                  // unique id
    s_onframe       jumpframe;              // Jumpframe action. 2011_04_01, DC: moved to struct.
    s_onframe       landframe;              // Landing behavior. 2011_04_01, DC: Moved to struct.
    s_loop          loop;                   // Animation looping. 2011_03_31, DC: Moved to struct.
    int             model_index;
    s_axis_i        **move;                 // base = seta, x = move, y = movea, z = movez
    int             numframes;
    int             (*offset)[2];           // original sprite offsets
    float           (*platform)[8];         // Now entities can have others land on them
    s_projectile    projectile;             // Subentity spawn for knives, stars, bombs, hadoken, etc.
    s_quakeframe    quakeframe;             // Screen shake effect. 2011_04_01, DC; Moved to struct.
    s_range         range;                  // Verify distance to target, jump landings, etc.. 2011_04_01, DC: Moved to struct.
    int             *shadow;
    int             (*shadow_coords)[2];    // x, z offset of shadow
    s_axis_i        size;                   // Dimensions (height, width).
    int             *soundtoplay;           // each frame can have a sound
    float           *spawnframe;            // Spawn the subentity as its default type. {frame} {x} {z} {a} {relative?}
    int             *sprite;                // sprite[set][framenumber]
    int             subentity;              // Store the sub-entity's name for further use
    float           *summonframe;           // Summon the subentity as an ally, only one though {frame} {x} {z} {a} {relative?}
    int             sync;                   // sychronize frame to previous animation if they matches
    int             unsummonframe;          // Un-summon the entity
    int             *vulnerable;
    int             *weaponframe;           // Specify with a frame when to switch to a weapon model
} s_anim;

struct animlist
{
    s_anim *anim;
    struct animlist *next;
};
typedef struct animlist s_anim_list;
s_anim_list *anim_list;

typedef struct
{
    s_axis_i_2d offset;
    s_axis_i_2d size;
    e_bartype type;
    e_barorient orientation;
    int noborder;
    e_bar_dir direction;
    int barlayer;
    int backlayer;
    int borderlayer;
    int shadowlayer;
    int (*colourtable)[11]; //0 default backfill 1-10 foreground colours
} s_barstatus;

typedef struct
{
    e_loadingScreenType set;    //Loading bar mode.
    int tf;                     //Font number for "LOADING" text (last element in command, moved here because of alignment)
    s_axis_i_2d bar_position;   //Loading bar position.
    s_axis_i_2d text_position;  //Loading text position.
    int bsize;                  // length of bar in pixels
    int refreshMs;              // modder defined number of milliseconds in which the screen is updated while loading
} s_loadingbar;

typedef struct
{
    Script         *animation_script;               //system generated script
    Script         *update_script;                  //execute when update_ents
    Script         *think_script;                   //execute when entity thinks.
    Script         *takedamage_script;              //execute when taking damage.
    Script         *ondeath_script;                 //execute when killed in game.
    Script         *onkill_script;                  //execute when removed from play.
    Script         *onpain_script;                  //Execute when put in pain animation.
    Script         *onfall_script;                  //execute when falling.
    Script         *inhole_script;                  //execute when yoy're in a hole
    Script         *onblocks_script;                //execute when blocked by screen.
    Script         *onblockw_script;                //execute when blocked by wall.
    Script         *onblockp_script;                //execute when blocked by platform.
    Script         *onblocko_script;                //execute when blocked by obstacle.
    Script         *onblockz_script;                //execute when blocked by Z.
    Script         *onblocka_script;                //execute when "hit head".
    Script         *onmovex_script;                 //execute when moving along X axis.
    Script         *onmovez_script;                 //execute when moving along Z axis.
    Script         *onmovea_script;                 //execute when moving along A axis.
    Script         *didhit_script;                  //execute when attack hits another.
    Script         *onspawn_script;                 //execute when spawned.
    Script         *key_script;                     //execute when entity's player presses a key
    Script         *didblock_script;                //execute when blocking attack.
    Script         *ondoattack_script;              //execute when attack passes do_attack checks.
    Script			*onmodelcopy_script;				//execute when set_model_ex is done
    Script			*ondraw_script;					//when update_ents is called
} s_scripts;

typedef struct
{
    /*
    In game icons added 2005_01_20.
    2011-04-05
    Damon V. Caskey
    */

    int def; //Default icon.
    int die; //Health depleted.
    int get; //Retrieving item.
    int mphigh; //MP bar icon; at 66% or more (default if other mp icons not used).
    int mplow; //MP bar icon; at or between 0% and 32%.
    int mpmed; //MP bar icon; at or between 33% and 65%.
    int pain; //Taking damage.
    int usemap;
    int weapon; //Weapon model.
    s_axis_i_2d position;
} s_icon;

typedef struct
{
    /*
    Pre defined color map selections and behavior.
    Damon V. Caskey
    2011_04_07
    */

    int frozen;             //Frozen.
    int hide_end;           //End range for maps hidden during character selection.
    int hide_start;         //Start range for maps hidden during character selection.
    int ko;                 //Health depleted.
    e_komap_type kotype;   //KO map application.
} s_maps;

typedef struct
{
    /*
    Perception distance (range from self AI can detect other entities).
    Damon V. Caskey
    2013-12-16
    */

    s_axis_i max;   //Maximum.
    s_axis_i min;   //Minimum.
} s_sight;

typedef struct
{
    signed char     detect;                         //Invisbility penetration. If self's detect >= target's hide, self can "see" target.
    signed char     hide;                           //Invisibility to AI.
} s_stealth;                                        //2011_04_05, DC: Invisibility to AI feature added by DC.


// WIP
typedef struct
{
    int input[MAX_SPECIAL_INPUTS];
    int	steps;
    int anim;
    int	cancel;		//should be fine to have 0 if idle is not a valid choice
    s_metric_i frame;
    int hits;
    int valid;		// should not be global unless nosame is set, but anyway...
    //int (*function)(); //reserved
} s_com;

typedef struct
{
    float       blockpower;     // If > unblockable, this attack type is blocked.
    float       blockthreshold; // Strongest attack from this attack type that can be blocked.
    float       blockratio;     // % of damage still taken from this attack type when blocked.
    e_blocktype blocktype;      // Resource drained when attack is blocked.
    float       factor;         // basic defense factors: damage = damage*defense
    float       knockdown;      // Knockdowncount (like knockdowncount) for attack type.
    float       pain;           // Pain factor (like nopain) for defense type.
} s_defense;

//UT: new bit flags for noquake property
#define NO_QUAKE 1  //do not make screen quake
#define NO_QUAKEN 2  //do not quake with screen

typedef struct
{
    /*
    Dust struct. "Dust" effect entity spawned during certain actions.
    Damon V. Caskey
    2013-12-28
    */

    int fall_land;  //Knockdown landing.
    int jump_land;  //Jump landing.
    int jump_start; //Jump lift off.
} s_dust;

typedef struct
{
    int index;
    char *name;
    char *path; // Path, so scripts can dynamically get files, sprites, sounds, etc.
    unsigned score;
    float stats[20]; // Parameters that do nothing on their own.
    int health;
    float scroll; // Autoscroll like panel entity.
    unsigned offscreenkill;                  // for biker, arrow, etc
    int	priority;
    //unsigned offscreenkillz;
    //unsigned offscreeenkila;
    int mp; // mp's variable for mpbar by tails
    int counter; // counter of weapons by tails
    unsigned shootnum; // counter of shots by tails
    unsigned reload; // reload max shots by tails
    int reactive; // Used for setting the "a" at which weapons are spawned
    int typeshot; // see if weapon is a gun or knife by tails
    int animal; // see is the weapon is a animal by tails
    int nolife; // Feb 25, 2005 - Variable flag to show life 0 = no, else yes
    int makeinv; // Option to spawn player invincible >0 blink <0 noblink
    int riseinv; // how many seconds will the character become invincible after rise >0 blink, <0 noblink
    int dofreeze; // Flag to freeze all enemies/players while special is executed
    int noquake; // Flag to make the screen shake when entity lands 1 = no, else yes
    int ground; // Flag to determine if enemy projectiles only hit the enemy when hitting the ground
    int multiple; // So you can control how many points are given for hitting opponents
    int bounce; // Flag to determine if bounce/quake is to be used.
    e_entity_type type;
    e_entity_type_sub subtype;
    s_icon icon; //In game icons added 2005_01_20. 2011_04_05, DC: Moved to struct.
    int parrow[MAX_PLAYERS][3]; // Image to be displayed when player spawns invincible
    int setlayer; // Used for forcing enities to be displayed behind
    int thold; // The entities threshold for block
    s_maps maps; //2011_04_07, DC: Pre defined color map selections and behavior.
    int alpha; // New alpha variable to determine if the entity uses alpha transparency
    int toflip; // Flag to determine if flashes flip or not
    int shadow;
    int gfxshadow; // use current frame to create a shadow
    int aironly; // Used to determine if shadows will be shown when jumping only
    int nomove; // Flag for static enemies
    int noflip; // Flag to determine if static enemies flip or stay facing the same direction
    int nodrop; // Flag to determine if enemies can be knocked down
    int nodieblink; // Flag to determine if blinking while playing die animation
    int holdblock; // Continue the block animation as long as the player holds the button down
    int nopassiveblock; // Don't auto block randomly
    int blockback; // Able to block attacks from behind
    int blockodds; // Odds that an enemy will block an attack (1 : blockodds)
    s_edelay edelay; // Entity level delay adjustment.
    float runspeed; // The speed the character runs at
    float runjumpheight; // The height the character jumps when running
    float runjumpdist; // The distance the character jumps when running
    int noatflash; // Flag to determine if attacking characters attack spawns a flash
    int runupdown; // Flag to determine if a player will continue to run while pressing up or down
    int runhold; // Flag to determine if a player will continue to run if holding down forward when landing
    int remove; // Flag to remove a projectile on contact or not
    float throwheight; // The height at which an opponent can now be adjusted
    float throwdist; // The distance an opponent can now be adjusted
    int throwframewait; // The frame victim is thrown during ANIM_THROW, added by kbandressen 10/20/06
    s_com *special; // Stores freespecials
    int specials_loaded; // Stores how many specials have been loaded
    int diesound;
    int weapnum;
    int secret;
    int clearcount;
    int weaploss[2]; // Determines possibility of losing weapon.
    int ownweapons; // is the weapon list own or share with others
    int *weapon; // weapon model list
    int numweapons;

    // these are model id of various stuff
    int project;
    int rider; // 7-1-2005 now every "biker" can have a new driver!
    int knife; // 7-1-2005 now every enemy can have their own "knife" projectile
    int pshotno; // 7-1-2005 now every enemy can have their own "knife" projectile
    int star; // 7-1-2005 now every enemy can have their own "ninja star" projectiles
    int bomb; // New projectile type for exploding bombs/grenades/dynamite
    int flash; // Now each entity can have their own flash
    int bflash; // Flash that plays when an attack is blocked
    s_dust dust; //Spawn entity during certain actions.
    s_axis_i_2d size; // Used to set height of player in pixels
    float speed;
    float grabdistance; // 30-12-2004	grabdistance varirable adder per character
    float pathfindstep; // UT: how long each step if the entity is trying to find a way
    int grabflip; // Flip target or not, bit0: grabber, bit1: opponent
    float jumpspeed; // normal jump foward speed, default to max(1, speed)
    float jumpheight; // 28-12-2004	Jump height variable added per character
    int jumpmovex; // low byte: 0 default 1 flip in air, 2 move in air, 3 flip and move
    int jumpmovez; // 2nd byte: 0 default 1 zjump with flip(not implemented yet) 2 z jump move in air, 3 1+2
    int grabfinish; // wait for grab animation to finish before do other actoins
    int antigrab; // anti-grab factor
    int grabforce; // grab factor, antigrab - grabforce <= 0 means can grab
    e_facing_adjust facing;
    int grabback; // Flag to determine if entities grab images display behind opponenets
    int grabturn;
    int paingrab; // Can only be grabbed when in pain
    float grabwalkspeed;
    int throwdamage; // 1-14-05  adjust throw damage
    unsigned char  *palette; // original palette for 32/16bit mode
    unsigned char	**colourmap;
    int maps_loaded; // Used for player colourmap selecting
    int unload; // Unload model after level completed?
    int falldie; // Play die animation?
    int globalmap; // use global palette for its colour map in 24bit mode
    int nopain;
    int summonkill; // kill it's summoned entity when died;  0. dont kill 1. kill summoned only 2. kill all spawned entity
    int combostyle;
    int blockpain;
    int atchain[MAX_ATCHAIN];
    int chainlength;
    s_anim **animation;
    int credit;
    int escapehits; // Escape spammers!
    int chargerate; // For the charge animation
    int guardrate; // Rate for guardpoints recover.
    int mprate; // For time-based mp recovery.
    int mpdroprate; // Time based MP loss.
    int mpstable; // MP stable type.
    int mpstableval; // MP Stable target.
    int aggression; // For enemy A.I.
    s_staydown risetime;
    unsigned sleepwait;
    int riseattacktype;
    s_metric_i_current jugglepoints; // Juggle points feature by OX. 2011_04_05, DC: Moved to struct.
    s_metric_i_current guardpoints; // Guard points feature by OX. 2011_04_05, DC: Moved to struct.
    int mpswitch; // switch between reduce or gain mp for mpstabletype 4
    int turndelay; // turn delay
    int lifespan; // lifespan count down
    float knockdowncount; // the knock down count for this entity
    float attackthrottle; // how often the enemy refuse to attack
    float attackthrottletime; // how long does the throttle status last
    s_stealth stealth; // Invisibility to AI feature added by DC. 2011_04_05, DC: Moved to struct.

    //---------------new A.I. switches-----------
    int hostile; // specify hostile types
    int candamage; // specify types that can be damaged by this entity
    int projectilehit; // specify types that can be hit by this entity if it is thrown
    unsigned aimove; // move style
    s_sight sight; // Sight range. 2011_04_05, DC: Moved to struct.
    unsigned aiattack; // attack/defend style

    //----------------physical system-------------------
    float antigravity;                    //antigravity : gravity * (1- antigravity)

    //--------------new property for endlevel item--------
    char *branch; //level branch name
    int model_flag; //used to judge some copy method when setting new model to an entity

    s_defense *defense; //defense related, make a struct to aid copying
    float *offense_factors; //basic offense factors: damage = damage*offense
    s_attack *smartbomb;

    // e.g., boss
    s_barstatus hpbarstatus;
    int hpx;
    int hpy;
    int namex;
    int namey;

    // movement flags
    int subject_to_wall;
    int subject_to_platform;
    int subject_to_obstacle;
    int subject_to_hole;
    int subject_to_gravity;
    int subject_to_screen;
    int subject_to_minz;
    int subject_to_maxz;
    int no_adjust_base; // dont change base to 0 automatically
    int instantitemdeath; // no delay before item suicides
    int	hasPlatforms;
    int isSubclassed;
    int backpain;
    int nohithead; // used to hit or not a platform with head also when you set a height
    e_ModelFreetype freetypes;
    s_scripts *scripts;
} s_model;

typedef struct
{
    char *name;
    char *path;
    s_model *model;
    int loadflag;
    int selectable;
} s_modelcache;
s_modelcache *model_cache;

typedef struct
{
    /*
    Jumping parameters struct.
    2013-12-08
    Damon Caskey
    */

    int id; //Jumping ID.
    s_axis_f velocity; //x,a,z velocity setting.
} s_jump;

typedef struct
{
    /*
    Bind struct for linking of entities.
    2013-12-17
    Damon V. Caskey
    */

    int ani_bind;                   //Animation binding type.
    e_direction_adjust direction;   //Direction force
    struct entity *ent;             //Entity to bind.
    s_axis_i offset;                //x,y,z offset.
    s_axis_i offset_flag;           //on/off binding on x,y,z
} s_bind;

typedef struct
{
    /*
    Rush combo struct.
    Damon V. Caskey
    2013-12-17
    */

    s_metric_i_current count;   //Hits counter.
    u32 time;           //Time to perform combo.
} s_rush;

typedef struct entity
{
    int spawntype; // Type of spawn. 1 = Level spawn. 0 for all else (subject to change).
    int exists; // flag to determine if it is a valid entity.
    int reactive; // Used for setting the "a" at which weapons are spawned
    int ptype;
    int playerindex;
    float stats[20]; // Parameters that do nothing on their own.
    int health; // current hp
    int mp; // current mp
    int oldhealth;
    int oldmp; //mp's variable for mp for players by tails
    char name[MAX_NAME_LEN + 1]; // this is display name
    s_model *defaultmodel; // this is the default model
    s_model *model; // current model
    s_model modeldata; // model data copyied here
    int item; // item model id
    int itemmap; // Now items spawned can have their properties changed
    int itemtrans; // alpha effect of item
    char itemalias[MAX_NAME_LEN + 1]; // Now items spawned can have their properties changed
    int itemhealth; // Now items spawned can have their properties changed
    int itemplayer_count;
    int boss;
    int dying; // Coresponds with which remap is to be used for the dying flash
    unsigned per1; // Used to store at what health value the entity begins to flash
    unsigned per2; // Used to store at what health value the entity flashes more rapidly
    e_direction direction;
    int nograb; // Some enemies cannot be grabbed (bikes) - now used with cantgrab as well
    int nograb_default; // equal to nograb  but this is remain the default value setetd in entity txt file (by White Dragon)
    int movestep;
    s_axis_f position; //x,y,z location.
    s_axis_f velocity; //x,y,z movement speed.
    float destx; // temporary values for ai functions
    float destz;
    float movex;
    float movez;
    float speedmul;
    float base;     // Default altitude
    float altbase; // Altitude affected by movea
    s_jump jump;    //Jumping velocity and id.
    unsigned combostep[MAX_SPECIAL_INPUTS];  // merge into an array to clear up some code

    // ---------------------- action times -------------------------------
    u32	lastmove;
    u32 lastdir;
    u32 timestamp;
    u32 releasetime;
    u32 toss_time; // Used by gravity code
    u32 nextmove;
    u32 stalltime;
    u32 combotime; // For multiple-hit combo
    u32 movetime; // For special move
    u32 freezetime; // Used to store at what point the a frozen entity becomes unfrozen
    u32 maptime; // used by forcemap
    u32 sealtime; // used by seal (stops special moves).
    u32 dot_time[MAX_DOTS]; //Dot time to expire.
    int dot[MAX_DOTS]; //Dot mode.
    int dot_atk[MAX_DOTS]; //Dot attack type.
    int dot_force[MAX_DOTS]; //Dot amount.
    int dot_rate[MAX_DOTS]; //Dot delay per tick.
    int dot_cnt[MAX_DOTS]; //Dot time of next tick.
    struct entity *dot_owner[MAX_DOTS]; //Dot owner.
    u32 magictime;
    u32 guardtime;
    u32 nextanim;
    u32 nextthink;
    u32 nextattack;
    u32 pain_time;
    u32 pausetime; // 2012/4/30 UT: Remove lastanimpos and add this. Otherwise hit pause is always bound to frame and attack box.
    u32 mpchargetime; // For the CHARGE animation
    u32 sleeptime; // For the SLEEP animation
    u32 knockdowntime; // count knock down hit
    u32 invinctime; // Used to set time for invincibility to expire
    u32 turntime;
    s_staydown staydown; //Delay modifiers before rise or riseattack can take place. 2011_04_08, DC: moved to struct.
    // -------------------------end of times ------------------------------
    int update_mark;

    //------------------------- a lot of flags ---------------------------

    int seal; //1 = No specials.
    int dead;
    int jumping; // Stuff useful for AI
    int idling;
    int walking;
    int drop;
    int attacking;
    int getting;
    int turning;
    int charging;
    int blocking;
    int falling;
    int running; // Flag to determine if a player is running
    int grabwalking; // a flag for grabwalk check
    int inpain; // playing pain animation
    int inbackpain; // playing pain/fall/die animation
    int frozen; // Flag to determine if an entity is frozen
    int blink;
    int invincible; // Flag used to determine if player is currently invincible
    int autokill; // Kill on end animation
    int remove_on_attack;
    int cantfire; // Flag to determine if another shot can be fired that costs energy
    int tocost; // Flag to determine if special costs life if doesn't hit an enemy
    int noaicontrol; // pause A.I. control
    int projectile;
    int toexplode; // Needed to determine if the projectile is a type that will explode (bombs, dynamite, etc)
    int animating; // Set by animation code
    int arrowon; // Flag to display parrow/parrow2 or not
    unsigned pathblocked;

    s_axis_f *waypoints;
    int numwaypoints;
    int animpos;
    int animnum; // animation id
    s_anim *animation;
    float knockdowncount;
    int damage_on_landing;
    int damagetype; // used for set death animation or pain animation
    int map; // Stores the colourmap for restoring purposes
    void (*think)();
    void (*takeaction)();
    int (*takedamage)(struct entity *, s_attack *);
    int (*trymove)(float, float);
    int attack_id;
    int hit_by_attack_id;
    int hitwall; // == 1 in the instant that hit the wall/platform/obstacle, else == 0
    unsigned char *colourmap;
    //struct entity   *thrower;
    struct entity *link; // Used to link 2 entities together.
    struct entity *owner; // Added for "hitenemy" flag so projectile recognizes its owner
    struct entity *grabbing; // Added for "platform level" layering
    struct entity *weapent;
    struct entity *parent; //Its spawner
    struct entity *subentity; //store the sub entity
    struct entity *opponent;
    struct entity *lasthit;
    struct entity *hithead; // when a player jumps and hits head on the bottom of a platform
    struct entity *landed_on_platform;
    s_bind binding;
    int escapecount; // For escapehits
    s_rush rush;    //Rush combo display.
    int lifespancountdown; // life span count down

    //------------- copy them from model to avoid global effect -------------
    s_defense *defense;
    float *offense_factors;

    int idlemode;
    int walkmode;

    int sortid; // id for sprite queue sort
    Varlist *varlist;
    s_drawmethod drawmethod;
    s_scripts *scripts;
} entity;


typedef struct
{
    char name[MAX_NAME_LEN + 1];
    int colourmap;
    unsigned score;
    unsigned lives;
    unsigned credits;
    entity *ent;
    u32 keys;
    u32 newkeys;
    u32 playkeys;
    u32 releasekeys;
    u32 combokey[MAX_SPECIAL_INPUTS];
    u32 inputtime[MAX_SPECIAL_INPUTS];
    u32 disablekeys;
    u32 prevkeys; // used for play/rec mode
    int combostep;
    int spawnhealth;
    int spawnmp;
    int joining;
    int hasplayed;
    int weapnum;
} s_player;

typedef struct
{
    int at;
    int wait;
    int nojoin; // dont allow new hero to join
    int spawnplayer_count; // spawn this entity according to the amount of players
    int palette; //change system palette to ...
    int groupmin;
    int groupmax;
    int scrollminz; // new scroll limit
    int scrollmaxz;
    int scrollminx; // new scroll limit
    int scrollmaxx;
    int blockade; //limit how far you can go back
    s_axis_i_2d light; // light direction, for gfx shadow
    int shadowcolor; // -1 no shadow
    int shadowalpha;
    char music[128];
    float musicfade;
    u32 musicoffset;
    char *name; // must be a name in the model list, so just reference
    int index; // model id
    int itemindex; // item model id
    int weaponindex; // the spawned entity with an weapon item, this is the id of the item model
    int alpha; // Used for alpha effects
    int boss;
    int flip;
    int itemtrans;
    int itemmap;
    int colourmap;
    int dying; // Used for the dying flash animation
    unsigned per1; // Used to store at what health value the entity begins to flash
    unsigned per2; // Used to store at what health value the entity flashes more rapidly
    int nolife; // So nolife can be overriden for all characters
    int itemplayer_count; // spawn the item according to the amount of players
    s_model *itemmodel;
    s_model *model;
    char alias[MAX_NAME_LEN + 1];
    char *item; // must be a name in the model list, so just reference
    char itemalias[MAX_NAME_LEN + 1];
    int itemhealth;
    int health[MAX_PLAYERS];
    int mp; // mp's variable for mpbar by tails
    unsigned score; // So score can be overridden for enemies/obstacles
    int multiple; // So score can be overridden for enemies/obstacles
    s_axis_f position;  //x, y, z location.
    unsigned credit;
    int aggression; // For enemy A.I.
    int spawntype; // Pass 1 when a level spawn.
    int entitytype; // if it's a enemy, player etc..
    entity *parent;
    char *weapon; // spawn with a weapon, since it should be in the model list, so the model must be loaded, just reference its name
    s_model *weaponmodel;
    Script spawnscript;
} s_spawn_entry;

typedef struct
{
    char *branchname; // Use a name so we can find this level in branches
    char *filename;
    e_le_type type; // see e_le_type
    int z_coords[3]; // Used for setting custom "z"
    int gonext; // 0. dont complete this level and display score,
    char *skipselect[MAX_PLAYERS]; // skipselect level based //[MAX_NAME_LEN]
    int	noselect;
    // 1. complete level and display score,
    // 2. complete game, show hall of fame
} s_level_entry;

typedef struct
{
    char *name;
    int maxplayers;
    int numlevels;
    s_level_entry *levelorder;
    int ifcomplete;
    int noshowhof;
    int noshowgameover;
    int lives;
    int credits;
    int custfade;
    int musicoverlap; //** shouldn't it be level based?
    int typemp; //** shouldn't it be model based?
    int continuescore;
    //char *skipselect[MAX_PLAYERS]; //** better if level based // depreciated
    int	noselect;
    int saveflag;
    int nosame;
} s_set_entry;

typedef struct
{
    e_bgloldtype oldtype;
    int order;	//for panel order
    gfx_entry gfx;
    s_axis_i_2d size;
    s_axis_f ratio; //Only x and z.
    s_axis_i offset; //Only x and z.
    s_axis_i spacing; //Only x and z.
    s_drawmethod drawmethod;
    float bgspeedratio;
    int enabled;
    int z;
    int quake;
    int neon;
} s_layer;

typedef struct
{
    /*
    Text object (display text on screen) struct
    2013-12-07
    Damon Caskey (Feature originally added by kbanderson)
    */

    int font;           //Font index.
    s_axis_i position;  //x,y,z location on screen.
    u32 time;           //Time to expire.
    char *text;         //Text to display.
} s_textobj;

typedef struct
{
    int pos;
    char *buf;
    size_t size;
} s_filestream;

typedef struct
{
    s_axis_i position;  //only x and z.
    s_axis_i size;      //Only x and z.
    float *map;
} s_basemap;

 typedef struct
 {
    /*
    Hole/Wall structure.
    2013-12-07
    Damon Caskey
    */
    float depth;
    float height;
    float lowerleft;
    float lowerright;
    float upperleft;
    float upperright;
    float x;
    float z;
    int type;
} s_terrain;

typedef struct
{
    char *name;
    int numspawns;
    s_spawn_entry *spawnpoints;
    int numlayers;
    s_layer *layers;
    int numlayersref;
    s_layer *layersref;
    ////////////////these below are layer reference
    ////////////////use them to ease layer finding for script users
    s_layer *background; // the bglayer that contains the default background
    int numpanels;
    s_layer *(*panels)[3]; //normal neon screen
    int numfrontpanels;
    s_layer **frontpanels;
    int numbglayers;
    s_layer **bglayers;
    int numfglayers;
    s_layer **fglayers;
    int numgenericlayers;
    s_layer **genericlayers;
    int numwaters;
    s_layer **waters;
    ////////////////layer reference ends here
    ///////////////////////////////////////////////////////////////
    int numtextobjs;
    s_textobj *textobjs;
    int cameraxoffset;
    int camerazoffset;
    int numholes;
    int numwalls;
    int numbasemaps;
    s_terrain *holes;
    s_terrain *walls;
    s_basemap *basemaps;
    int scrolldir;
    int width;
    int rocking;
    float bgspeed; // Used to make autoscrolling backgrounds
    float vbgspeed;
    float scrollspeed; // UT: restore this command  2011/7/8
    int bgdir; // Used to set which direction the backgrounds scroll for autoscrolling backgrounds
    int mirror;
    int bosses;
    char bossmusic[256];
    unsigned bossmusic_offset;
    int numpalettes;
    unsigned char (*palettes)[1024];//dynamic palettes
    unsigned char *(*blendings)[MAX_BLENDINGS];//blending tables
    int settime; // Set time limit per level
    int notime; // Used to specify if the time is displayed 1 = no, else yes
    int noreset; // If set, clock will not reset when players spawn/die
    int type; // Used to specify which level type (1 = bonus, else regular)
    int nospecial; // Used to specify if you can use your special during bonus levels
    int nohurt; // Used to specify if you can hurt the other player during bonus levels
    int noslow; // Flag so the level doesn't slow down after a boss is defeated
    int nohit; // Not able to grab / hit other player on a per level basis
    int forcefinishlevel; // flag to forca to finish a level
    s_axis_f spawn[MAX_PLAYERS]; // Used to determine the spawn position of players
    int setweap; // Levels can now specified which weapon will be used by default
    e_facing_adjust facing; // Force the players to face to ...
//--------------------gravity system-------------------------
    float maxfallspeed;
    float maxtossspeed;
    float gravity;
//---------------------scripts-------------------------------
    Script update_script;
    Script updated_script;
    Script key_script;
    Script level_script;
    Script endlevel_script;
    int pos;
    u32 advancetime;
    u32 quaketime;
    int quake;
    int waiting;

} s_level;

typedef struct ArgList
{
    size_t count;
    size_t arglen[MAX_ARG_COUNT];
    char *args[MAX_ARG_COUNT];
} ArgList;


#define GET_ARG(z) (arglist.count > z ? arglist.args[z] : "")
#define GET_ARG_LEN(z) (arglist.count > z ? arglist.arglen[z] : 0)
#define GET_ARGP(z) (arglist->count > z ? arglist->args[z] : "")
#define GET_ARGP_LEN(z) (arglist->count > z ? arglist->arglen[z] : 0)
#define GET_INT_ARG(z) getValidInt(GET_ARG(z), filename, command)
#define GET_FLOAT_ARG(z) getValidFloat(GET_ARG(z), filename, command)
#define GET_INT_ARGP(z) getValidInt(GET_ARGP(z), filename, command)
#define GET_FLOAT_ARGP(z) getValidFloat(GET_ARGP(z), filename, command)

#define GET_FRAME_ARG(z) (stricmp(GET_ARG(z), "this")==0?newanim->numframes:GET_INT_ARG(z))

int     buffer_pakfile(char *filename, char **pbuffer, size_t *psize);
size_t  ParseArgs(ArgList *list, char *input, char *output);
int     getsyspropertybyindex(ScriptVariant *var, int index);
int     changesyspropertybyindex(int index, ScriptVariant *value);
int     load_script(Script *script, char *path);
void    init_scripts();
void    load_scripts();
void    execute_animation_script    (entity *ent);
void    execute_takedamage_script   (entity *ent, entity *other, int force, int drop, int type, int noblock, int guardcost, int jugglecost, int pauseadd, int tag);
void    execute_ondeath_script      (entity *ent, entity *other, int force, int drop, int type, int noblock, int guardcost, int jugglecost, int pauseadd, int tag);
void    execute_onkill_script       (entity *ent);
void    execute_onpain_script       (entity *ent, int iType, int iReset);
void    execute_onfall_script       (entity *ent, entity *other, int force, int drop, int type, int noblock, int guardcost, int jugglecost, int pauseadd, int tag);
void    execute_inhole_script       (entity *ent, int plane, float height, int index);
void    execute_onblocks_script     (entity *ent);
void    execute_onblockw_script     (entity *ent, int plane, float height, int index);
void    execute_onblockp_script     (entity *ent, int plane, entity *platform);
void    execute_onblocko_script     (entity *ent, entity *other);
void    execute_onblockz_script     (entity *ent);
void    execute_onblocka_script     (entity *ent, entity *other);
void    execute_onmovex_script      (entity *ent);
void    execute_onmovez_script      (entity *ent);
void    execute_onmovea_script      (entity *ent);
void    execute_didblock_script     (entity *ent, entity *other, int force, int drop, int type, int noblock, int guardcost, int jugglecost, int pauseadd, int tag);
void    execute_ondoattack_script   (entity *ent, entity *other, int force, int drop, int type, int noblock, int guardcost, int jugglecost, int pauseadd, int iWhich, int iAtkID, int tag);
void    execute_updateentity_script (entity *ent);
void    execute_think_script        (entity *ent);
void    execute_didhit_script       (entity *ent, entity *other, int force, int drop, int type, int noblock, int guardcost, int jugglecost, int pauseadd, int blocked, int tag);
void    execute_onspawn_script      (entity *ent);
void    clearsettings(void);
void    savesettings(void);
void    saveasdefault(void);
void    loadsettings(void);
void    loadfromdefault(void);
void    clearSavedGame(void);
void    clearHighScore(void);
int    saveGameFile(void);
int     loadGameFile(void);
int		saveScriptFile(void);
int		loadScriptFile(void);
int    saveHighScoreFile(void);
int    loadHighScoreFile(void);
int translate_SDID(char *value);
int music(char *filename, int loop, long offset);
int readByte(char* command);
char *findarg(char *command, int which);
float diff(float a, float b);
int inair(entity *e);
float randf(float max);
int _makecolour(int r, int g, int b);
int load_colourmap(s_model *model, char *image1, char *image2);
int load_palette(unsigned char *pal, char *filename);
void standard_palette();
void change_system_palette(int palindex);
void unload_background();
void lifebar_colors();
void load_background(char *filename, int createtables);
void unload_texture();
void load_texture(char *filename);
void freepanels();
s_sprite *loadpanel2(char *filename);
int loadpanel(char *filename_normal, char *filename_neon, char *filename_screen);
int loadfrontpanel(char *filename);
void resourceCleanUp(void);
void freesprites();
s_sprite *loadsprite2(char *filename, int *width, int *height);
int loadsprite(char *filename, int ofsx, int ofsy, int bmpformat);
void load_special_sprites();
int load_special_sounds();
s_model *find_model(char *name);
s_model *nextplayermodel(s_model *current);
s_model *prevplayermodel(s_model *current);
void free_anim(s_anim *anim);
void free_models();
s_anim *alloc_anim();
int addframe(s_anim *a, int spriteindex, int framecount, int delay, unsigned idle,
             s_body *bbox, s_attack *attack, s_axis_i *move,
             float *platform, int frameshadow,
             int *shadow_coords, int soundtoplay, s_drawmethod *drawmethod, int *offset);
void cache_model(char *name, char *path, int flag);
void remove_from_cache(char *name);
void free_modelcache();
int get_cached_model_index(char *name);
char *get_cached_model_path(char *name);
s_model *load_cached_model(char *name, char *owner, char unload);
int is_set(s_model *model, int m);
int load_script_setting();
int load_models();
void unload_levelorder();
void load_levelorder();
void unload_level();
void load_level(char *filename);
void drawlifebar(int x, int y, int h, int maxh);
void drawmpbar(int x, int y, int m, int maxm);
void update_loading(s_loadingbar *s,  int value, int max);
void spawnplayer(int);
void drop_all_enemies();
void kill_all_enemies();
unsigned getFPS(void);
unsigned char *model_get_colourmap(s_model *model, unsigned which);
void ent_set_colourmap(entity *ent, unsigned int which);
void predrawstatus();
void drawstatus();
void addscore(int playerindex, int add);
void free_ent(entity *e);
void free_ents();
int alloc_ents();
entity *smartspawn(s_spawn_entry *p);
int adjust_grabposition(entity *ent, entity *other, float dist, int grabin);
int player_trymove(float xdir, float zdir);
void toss(entity *ent, float lift);
void player_think(void);
void subtract_shot(void);
void set_model_ex(entity *ent, char *modelname, int index, s_model *newmodel, int flag);
void dropweapon(int flag);
void biker_drive(void);
void trap_think(void);
void steamer_think(void);
void text_think(void);
void anything_walk(void);
void adjust_walk_animation(entity *other);
void kill(entity *);
int player_takedamage(entity *other, s_attack *attack);
int biker_takedamage(entity *other, s_attack *attack);
int obstacle_takedamage(entity *other, s_attack *attack);
void ent_set_anim(entity *, int, int);
void suicide(void);
void player_blink(void);
void common_prejump();
void common_dot();
void tryjump(float, float, float, int);
void dojump(float, float, float, int);
void biker_drive(void);
void ent_default_init(entity *e);
void ent_spawn_ent(entity *ent);
void ent_summon_ent(entity *ent);
void ent_set_anim(entity *ent, int aninum, int resetable);
void ent_set_colourmap(entity *ent, unsigned int which);
void ent_set_model(entity *ent, char *modelname, int syncAnim);
entity *spawn(float x, float z, float a, int direction, char *name, int index, s_model *model);
void ent_unlink(entity *e);
void ents_link(entity *e1, entity *e2);
void kill(entity *victim);
void kill_all();
int checkhit(entity *attacker, entity *target, int counter);
int checkhole(float x, float z);
int checkhole_index(float x, float z);
int checkhole_in(float x, float z, float a);
int checkholeindex_in(float x, float z, float a);
int checkhole_between(float x, float z, float a1, float a2);
int testplatform(entity *, float, float, entity *);
int testhole(int, float, float);
int testwall(int, float, float);
int checkwalls(float x, float z, float a1, float a2);
int checkholes(float, float);
int checkwall_below(float x, float z, float a);
int checkwall(float x, float z);
int testmove(entity *, float, float, float, float);
entity *check_platform_below(float x, float z, float a, entity *exclude);
entity *check_platform_above(float x, float z, float a, entity *exclude);
entity *check_platform_between(float x, float z, float amin, float amax, entity *exclude);
entity *check_platform(float x, float z, entity *exclude);
void do_attack(entity *e);
void adjust_base(entity *e, entity **pla);
void check_gravity(entity *e);
void update_ents();
entity *find_ent_here(entity *exclude, float x, float z, int types, int (*test)(entity *, entity *));
void display_ents();
void toss(entity *ent, float lift);
entity *findent(int types);
int count_ents(int types);
int set_idle(entity *ent);
int set_death(entity *iDie, int type, int reset);
int set_fall(entity *iFall, int type, int reset, entity *other, int force, int drop, int noblock, int guardcost, int jugglecost, int pauseadd, int tag);
int set_rise(entity *iRise, int type, int reset);
int set_riseattack(entity *iRiseattack, int type, int reset);
int set_blockpain(entity *iBlkpain, int type, int reset);
int set_pain(entity *iPain, int type, int reset);
void set_weapon(entity *ent, int wpnum, int anim_flag);
entity *melee_find_target();
entity *long_find_target();
entity *normal_find_target(int anim, int iDetect);
entity *normal_find_item();
int long_attack();
int melee_attack();
void dothrow();
void doprethrow();
void dograbattack(int);
int check_special();
void normal_prepare();
void common_jump();
void common_spawn(void);
void common_drop(void);
void common_walkoff(void);
void common_jumpattack();
void common_turn();
void common_fall();
void common_lie();
void common_rise();
void common_pain();
void common_get();
void common_land();
void common_grab(void);
void common_grabattack();
void common_grabbed();
void common_block(void);
int arrow_takedamage(entity *other, s_attack *attack);
int common_takedamage(entity *other, s_attack *attack);
int normal_attack();
void common_throw(void);
void common_throw_wait(void);
void common_prethrow(void);
void npc_warp();
int checkpathblocked();
int common_trymove(float xdir, float zdir);
void normal_runoff();
void common_stuck_underneath();
void common_attack_proc();
void normal_attack_finish();
entity *common_find_target();
int common_attack(void);
int common_try_jump(void);
int common_try_pick(entity *other);
int common_try_chase(entity *target, int dox, int doz);
int common_try_follow(entity *target, int dox, int doz);
int common_try_avoid(entity *target, int dox, int doz);
int common_try_wandercompletely(int dox, int doz);
int common_try_wander(entity *target, int dox, int doz);
void common_pickupitem(entity *other);
int common_backwalk_anim(entity *ent);
int bomb_move(void);
int arrow_move(void);
int common_move(void);
void common_think(void);
void suicide(void);
void prethrow(void);
void player_die();
int player_trymove(float xdir, float zdir);
int check_energy(e_cost_check which, int ani);
int player_preinput();
int player_check_special();
void runanimal(void);
void player_blink(void);
int check_combo();
int check_costmove(int s, int fs, int jumphack);
void didfind_item(entity *other);
void player_think(void);
void subtract_shot();
void dropweapon(int flag);
void drop_all_enemies();
void kill_all_enemies();
void smart_bomb(entity *e, s_attack *attack);
void anything_walk(void);
entity *knife_spawn(char *name, int index, float x, float z, float a, int direction, int type, int map);
entity *bomb_spawn(char *name, int index, float x, float z, float a, int direction, int map);
void bomb_explode(void);
int star_spawn(float x, float z, float a, int direction);
void steam_think(void);
void trap_think(void);
void steam_spawn(float x, float z, float a);
void steamer_think(void);
void text_think(void);
entity *homing_find_target(int type);
void biker_drive(void);
void bike_crash(void);
void obstacle_fall(void);
void obstacle_fly(void);
entity *smartspawn(s_spawn_entry *props);
int is_incam(float x, float z, float a, float threshold);
void spawnplayer(int index);
void time_over();
void update_scroller();
void draw_scrolled_bg();
void update(int ingame, int usevwait);
void fade_out(int type, int speed);
void apply_controls();
void plan();
int ai_check_warp();
int ai_check_lie();
int ai_check_grabbed();
int ai_check_grab();
int ai_check_escape();
int ai_check_busy();
void display_credits(void);
void shutdown(int status, char *msg, ...);
#ifdef DC
void guistartup(void);
#endif
void startup(void);
int playgif(char *filename, int x, int y, int noskip);
void playscene(char *filename);
void gameover();
void hallfame(int addtoscore);
void showcomplete(int num);
int playlevel(char *filename);
int selectplayer(int *players, char *filename);
void playgame(int *players,  unsigned which_set, int useSavedGame);
int choose_difficulty();
int load_saved_game();
void term_videomodes();
void init_videomodes(int log);
void safe_set(int *arr, int index, int newkey, int oldkey);
void keyboard_setup(int player);
void input_options();
void inputrefresh();
void soundvol_options();
void config_options();
void cheatoptions();
void system_options();
void video_options();
void options();
void soundcard_options();
void openborMain(int argc, char **argv);
int is_cheat_actived();
int getValidInt(char *text, char *file, char *cmd);
float getValidFloat(char *text, char *file, char *cmd);
int dograb(entity *attacker, entity *target, e_dograb_adjustcheck adjustcheck);
int stopRecordInputs(void);
int recordInputs(void);
int playRecordedInputs(void);
int freeRecordedInputs(void);
a_playrecstatus* init_input_recorder(void);
void free_input_recorder(void);

extern s_savelevel   *savelevel;
extern s_savescore    savescore;

#endif
