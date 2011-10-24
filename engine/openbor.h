/*
 * OpenBOR - http://www.LavaLit.com
 * -
 ----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
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
#include	"triangle.h"
#include    "ram.h"
#include    "version.h"

#ifdef SDL
#include    "gfx.h"
#endif

/////////////////////////////////////////////////////////////////////////////

#define		exit				borExit
#define		time				borTime
#define		kill				borKill
#define		pause				borPause
#define		shutdown			borShutdown

#define		DEFAULT_SHUTDOWN_MESSAGE \
			"OpenBOR %s, Compile Date: " __DATE__ "\n" \
			"Presented by Senile Team.\n" \
			"This Version is unofficial and based on the Senile source code.\n" \
			"\n" \
			"Special thanks to SEGA and SNK.\n\n", \
			VERSION

#define		COMPATIBLEVERSION	0x00030000
#define		CV_SAVED_GAME		0x00021276
#define		CV_HIGH_SCORE		0x00020048
#define     GAME_SPEED          200
#define		THINK_SPEED			2
#define		COUNTER_SPEED		(GAME_SPEED*2)
#define		MAX_ENTS			150
#define		MAX_PANELS			52
#define		MAX_WEAPONS			10
#define		MAX_COLOUR_MAPS		30
#define		MAX_NAME_LEN		40
#define		LEVEL_MAX_SPAWNS	600
#define		LEVEL_MAX_PANELS	100
#define		LEVEL_MAX_HOLES		40
#define		LEVEL_MAX_WALLS		40
#define     LEVEL_MAX_LAYERS  100
#define     LEVEL_MAX_TEXTOBJS  50
#define     LEVEL_MAX_FILESTREAMS 50
#define     LEVEL_MAX_PALETTES  40                  // altered palettes
#define		MAX_LEVELS			100
#define		MAX_DIFFICULTIES	10
#define		MAX_SPECIALS		8					// Added for customizable freespecials
#define     MAX_SPECIAL_INPUTS  27                  // max freespecial input steps, MAX_SPECIAL_INPUTS-1 is reserved, MAX_SPECIAL_INPUTS-2 is animation index, MAX_SPECIAL_INPUTS-3 is reserved. OX -4 , -5 , -6 , -7 , -8 , -9 , -10 also for cancels
#define		MAX_ATCHAIN			12					// Maximum attack chain length
#define     MAX_IDLES           1                   // Idle animations.
#define     MAX_WALKS           1                   // Walk animations.
#define     MAX_BACKWALKS       1                   // Backwalk animations.
#define     MAX_UPS             1                   // Walk up animations.
#define     MAX_DOWNS           1                   // Walk down animations.
#define		MAX_ATTACKS			4					// Total number of attacks players have
#define     MAX_FOLLOWS         4					// For followup animations
#define     MAX_PLAYERS         4
#define		MAX_ARG_LEN			512
#define		MAX_PAL_SIZE		1024
#define		MAX_CACHED_BACKGROUNDS 9

#define		FLAG_ESC			0x00000001
#define		FLAG_START			0x00000002
#define		FLAG_MOVELEFT		0x00000004
#define		FLAG_MOVERIGHT		0x00000008
#define		FLAG_MOVEUP			0x00000010
#define		FLAG_MOVEDOWN		0x00000020
#define		FLAG_ATTACK			0x00000040
#define		FLAG_JUMP			0x00000080
#define		FLAG_SPECIAL		0x00000100
#define		FLAG_SCREENSHOT		0x00000200
#define		FLAG_ATTACK2		0x00000400
#define		FLAG_ATTACK3		0x00000800
#define		FLAG_ATTACK4		0x00001000
#define		FLAG_ANYBUTTON		(FLAG_START|FLAG_SPECIAL|FLAG_ATTACK|FLAG_ATTACK2|FLAG_ATTACK3|FLAG_ATTACK4|FLAG_JUMP)
#define		FLAG_FORWARD		0
#define		FLAG_BACKWARD		1

#define		SDID_MOVEUP			0
#define		SDID_MOVEDOWN		1
#define		SDID_MOVELEFT		2
#define		SDID_MOVERIGHT		3
#define		SDID_ATTACK 		4
#define		SDID_ATTACK2		5
#define		SDID_ATTACK3		6
#define		SDID_ATTACK4		7
#define		SDID_JUMP			8
#define		SDID_SPECIAL		9
#define		SDID_START			10
#define		SDID_SCREENSHOT		11

#define		TYPE_NONE			0
#define		TYPE_PLAYER			1
#define		TYPE_ENEMY			2
#define		TYPE_ITEM			4
#define		TYPE_OBSTACLE		8
#define		TYPE_STEAMER		16
#define		TYPE_SHOT			32					// 7-1-2005 type to use for player projectiles
#define		TYPE_TRAP			64					// 7-1-2005 lets face it enemies are going to just let you storm in without setting a trap or two!
#define		TYPE_TEXTBOX		128					// New textbox type for displaying messages
#define		TYPE_ENDLEVEL		256					// New endlevel type that ends the level when touched
#define     TYPE_NPC            512                 // A character can be an ally or enemy.
#define     TYPE_PANEL          1024                // Fake panel, scroll with screen using model speed

#define		SUBTYPE_NONE		0
#define		SUBTYPE_BIKER		1
#define		SUBTYPE_NOTGRAB		2					//7-1-2005 new subtype for those ungrabbable enemies
#define		SUBTYPE_ARROW		3					//7-1-2005  subtype for an "enemy" that flies across the screen and dies
#define		SUBTYPE_TOUCH		4					// ltb 1-18-05  new Item subtype for a more platformer feel.
#define		SUBTYPE_WEAPON		5
#define		SUBTYPE_NOSKIP		6					// Text type that can't be skipped
#define		SUBTYPE_FLYDIE		7					// Now obstacles can be hit and fly like on Simpsons/TMNT
#define		SUBTYPE_BOTH		8					// Used with TYPE_ENDLEVEL to force both players to reach the point before ending level
#define		SUBTYPE_PROJECTILE	9					// New weapon projectile type that can be picked up by players/enemies
#define     SUBTYPE_FOLLOW      10                  // Used by NPC character, if set, they will try to follow players
#define     SUBTYPE_CHASE       11                  // Used by enemy always chasing you

//------------reserved for A.I. types-------------------------
//  A.I. move1, affect movement path
#define     AIMOVE1_NORMAL      0                   // Current default style
#define     AIMOVE1_CHASE       0x00000001          // alway move towards target, and can run to them if target is farway
#define     AIMOVE1_CHASEZ      0x00000002          // only try to get close in z direction
#define     AIMOVE1_CHASEX      0x00000004          // only try to get colse in x direction
#define     AIMOVE1_AVOID       0x00000008          // try to avoid target
#define     AIMOVE1_AVOIDZ      0x00000010          // only try to avoid target in z direction
#define     AIMOVE1_AVOIDX      0x00000020          // only try to avoid target in x direction
#define     AIMOVE1_WANDER      0x00000040          // ignore the target's position completely, wander everywhere, long idle time
#define     AIMOVE1_BIKER       0x00000080          // move like a biker
#define     AIMOVE1_ARROW       0x00000100          // fly like an arrow
#define     AIMOVE1_STAR        0x00000200          // fly like a star, subject to ground
#define     AIMOVE1_BOMB        0x00000400          // fly like a bomb, subject to ground/wall etc
#define     AIMOVE1_NOMOVE      0x00000800          // don't move at all
#define     AIMOVE1_BOOMRANG    0x00001000          // boomrang
#define     MASK_AIMOVE1        0x0000FFFF

// A.I move2, affect terrain reflect
#define     AIMOVE2_NORMAL      0                   // Current default style
#define     AIMOVE2_IGNOREHOLES 0x00010000          // don't avoid holes
#define		AIMOVE2_NOTARGETIDLE 0x00020000			// don't move when there's no target
#define     MASK_AIMOVE2        0xFFFF0000

// A.I. attack1, affect attacking style
#define     AIATTACK1_NORMAL      0                   // Current default style
#define     AIATTACK1_LONG        0x00000001          // Long range first
#define     AIATTACK1_MELEE       0x00000002          // Melee attack first
#define     AIATTACK1_NOATTACK    0x00000004          // dont attack at all
#define     MASK_AIATTACK1        0x0000FFFF

//  A.I. attack2, affect Defending style
#define     AIATTACK2_NORMAL      0                   // Current default style, don't dodge at all
#define     AIATTACK2_DODGE       0x00010000          // Use dodge animation to avoid attack
#define     AIATTACK2_DODGEMOVE   0x00020000          // Try to move in z direction if a jump attack is about to hit him
													  // and try to step back if a melee attack is about to hit him
#define     MASK_AIATTACK2        0xFFFF0000


// Note: the minimum Z coordinate of the player is important
// for several other drawing operations.
// movement restirctions are here!
//int			PLAYER_MIN_Z		= 160;				// 2-10-05  adjustable walking area
//int			PLAYER_MAX_Z		= 232;				// 2-10-05  adjustable walking area
//int			BGHEIGHT			= 160;				// 2-10-05  adjustable BGHeight
//int         MAX_WALL_HEIGHT     =  1000;            // max wall height that an entity can be spawned on
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



#define		ATK_NORMAL			0
#define		ATK_NORMAL2			1
#define		ATK_NORMAL3			2
#define		ATK_NORMAL4			3
#define		ATK_BLAST			4
#define		ATK_BURN			5
#define		ATK_FREEZE			6
#define		ATK_SHOCK			7
#define		ATK_STEAL			8					// Steal opponents life
#define		ATK_NORMAL5			9
#define		ATK_NORMAL6			10
#define		ATK_NORMAL7			11
#define		ATK_NORMAL8			12
#define		ATK_NORMAL9			13
#define		ATK_NORMAL10		14
#define		ATK_ITEM		    15

#define     MAX_ATKS            16                // default max attack types
#define     STA_ATKS            6                 // default special attack types: blast burn shock freeze steal
#define     MAX_DOTS            10                // Max active dot effects.

#define		SCROLL_RIGHT		2
#define		SCROLL_DOWN			4
#define		SCROLL_LEFT			8
#define		SCROLL_UP			16
#define		SCROLL_BACK			1
#define		SCROLL_BOTH			(SCROLL_BACK|SCROLL_RIGHT)
#define		SCROLL_RIGHTLEFT	SCROLL_BOTH
#define		SCROLL_LEFTRIGHT    (SCROLL_LEFT|SCROLL_BACK)
#define		SCROLL_INWARD       32
#define     SCROLL_OUTWARD      64
#define		SCROLL_OUTIN		(SCROLL_OUTWARD|SCROLL_BACK)
#define		SCROLL_INOUT		(SCROLL_INWARD|SCROLL_BACK)
#define		SCROLL_UPWARD       128
#define     SCROLL_DOWNWARD     256
// blah, blah,


#define		ANI_IDLE			0
#define		ANI_WALK			1
#define		ANI_JUMP			2
#define		ANI_LAND			3
#define		ANI_PAIN			4
#define		ANI_FALL			5
#define		ANI_RISE			6
#define		ANI_ATTACK1			7
#define		ANI_ATTACK2			8
#define		ANI_ATTACK3			9
#define		ANI_ATTACK4			10					// Very important
#define		ANI_UPPER			11
#define		ANI_BLOCK			12					// New block animation
#define		ANI_JUMPATTACK		13
#define		ANI_JUMPATTACK2		14
#define		ANI_GET				15
#define		ANI_GRAB			16
#define		ANI_GRABATTACK		17
#define		ANI_GRABATTACK2		18
#define		ANI_THROW			19
#define		ANI_SPECIAL			20
#define		ANI_FREESPECIAL		21
#define		ANI_SPAWN			22 					// 26-12-2004 new animation added here ani_spawn
#define		ANI_DIE				23					// 29-12-2004 new animation added here ani_die
#define		ANI_PICK			24					// 7-1-2005 used when players select their character at the select screen
#define		ANI_FREESPECIAL2	25
#define		ANI_JUMPATTACK3		26
#define		ANI_FREESPECIAL3	27
#define		ANI_UP				28					// Mar 2, 2005 - Animation for when going up
#define		ANI_DOWN			29					// Mar 2, 2005 - Animation for when going down
#define		ANI_SHOCK			30					// Animation played when knocked down by shock attack
#define		ANI_BURN			31					// Animation played when knocked down by burn attack
#define		ANI_SHOCKPAIN		32					// Animation played when not knocked down by shock attack
#define		ANI_BURNPAIN		33					// Animation played when not knocked down by shock attack
#define		ANI_GRABBED			34					// Animation played when grabbed
#define		ANI_SPECIAL2		35					// Animation played for when pressing forward special
#define		ANI_RUN				36					// Animation played when a player is running
#define		ANI_RUNATTACK		37					// Animation played when a player is running and presses attack
#define		ANI_RUNJUMPATTACK	38					// Animation played when a player is running and jumps and presses attack
#define		ANI_ATTACKUP		39					// u u animation
#define		ANI_ATTACKDOWN		40					// d d animation
#define		ANI_ATTACKFORWARD	41					// f f animation
#define		ANI_ATTACKBACKWARD	42					// Used for attacking backwards
#define		ANI_FREESPECIAL4	43					// More freespecials added
#define		ANI_FREESPECIAL5	44					// More freespecials added
#define		ANI_FREESPECIAL6	45					// More freespecials added
#define		ANI_FREESPECIAL7	46					// More freespecials added
#define		ANI_FREESPECIAL8	47					// More freespecials added
#define		ANI_RISEATTACK		48					// Attack used for enemies when players are crowding around after knocking them down
#define		ANI_DODGE			49					// Used for up up / down down SOR3 dodge moves for players
#define		ANI_ATTACKBOTH		50					// Used for when a player holds down attack and presses jump
#define		ANI_GRABFORWARD		51					// New grab attack for when a player holds down forward/attack
#define		ANI_GRABFORWARD2	52					// New second grab attack for when a player holds down forward/attack
#define		ANI_JUMPFORWARD		53					// Attack when a player is moving and jumps
#define		ANI_GRABDOWN		54					// Attack when a player has grabbed an opponent and presses down/attack
#define		ANI_GRABDOWN2		55					// Attack when a player has grabbed an opponent and presses down/attack
#define		ANI_GRABUP			56					// Attack when a player has grabbed an opponent and presses up/attack
#define		ANI_GRABUP2			57					// Attack when a player has grabbed an opponent and presses up/attack
#define		ANI_SELECT			58					// Animation that is displayed at the select screen
#define		ANI_DUCK			59					// Animation that is played when pressing down in "platform" type levels
#define		ANI_FAINT			60  				// Faint animations for players/enemys by tails
#define		ANI_CANT			61  				// Can't animation for players(animation when mp is less than mpcost) by tails.
#define		ANI_THROWATTACK		62					// Added for subtype projectile
#define		ANI_CHARGEATTACK	63                  // Plays when player releases attack1 after holding >= chargetime.
#define		ANI_VAULT			64  				// Now you can flip over people like in SOR.
#define		ANI_JUMPCANT		65
#define		ANI_JUMPSPECIAL		66
#define		ANI_BURNDIE			67
#define		ANI_SHOCKDIE		68
#define		ANI_PAIN2			69
#define		ANI_PAIN3			70
#define		ANI_PAIN4			71
#define		ANI_FALL2			72
#define		ANI_FALL3			73
#define		ANI_FALL4			74
#define		ANI_DIE2			75
#define		ANI_DIE3			76
#define		ANI_DIE4			77
#define		ANI_CHARGE			78
#define		ANI_BACKWALK		79
#define		ANI_SLEEP			80
#define		ANI_FOLLOW1			81
#define		ANI_FOLLOW2			82
#define		ANI_FOLLOW3			83
#define		ANI_FOLLOW4			84
#define		ANI_PAIN5			85
#define		ANI_PAIN6			86
#define		ANI_PAIN7			87
#define		ANI_PAIN8			88
#define		ANI_PAIN9			89
#define		ANI_PAIN10			90
#define		ANI_FALL5			91
#define		ANI_FALL6			92
#define		ANI_FALL7			93
#define		ANI_FALL8			94
#define		ANI_FALL9			95
#define		ANI_FALL10			96
#define		ANI_DIE5			97
#define		ANI_DIE6			98
#define		ANI_DIE7			99
#define		ANI_DIE8			100
#define		ANI_DIE9			101
#define		ANI_DIE10			102
#define     ANI_TURN            103   // turn back/flip
#define     ANI_RESPAWN         104    //now spawn works for players
#define     ANI_FORWARDJUMP     105
#define     ANI_RUNJUMP         106
#define     ANI_JUMPLAND        107
#define     ANI_JUMPDELAY       108
#define     ANI_HITWALL         109
#define     ANI_GRABBACKWARD    110
#define     ANI_GRABBACKWARD2   111
#define     ANI_GRABWALK        112
#define     ANI_GRABBEDWALK     113
#define     ANI_GRABWALKUP      114
#define     ANI_GRABBEDWALKUP   115
#define     ANI_GRABWALKDOWN    116
#define     ANI_GRABBEDWALKDOWN 117
#define     ANI_GRABTURN        118
#define     ANI_GRABBEDTURN     119
#define     ANI_GRABBACKWALK    120
#define     ANI_GRABBEDBACKWALK 121
#define     ANI_SLIDE           122    //Down + Jump animation.
#define     ANI_RUNSLIDE        123    //Down + Jump while running.
#define     ANI_BLOCKPAIN       124    //If entity has this, it will play in place of "pain" when it's blokcpain is 1 and incomming attack is blocked.
#define     ANI_DUCKATTACK      125
#define		ANI_RISE2			126
#define		ANI_RISE3			127
#define		ANI_RISE4			128
#define		ANI_RISE5			129
#define		ANI_RISE6			130
#define		ANI_RISE7			131
#define		ANI_RISE8			132
#define		ANI_RISE9			133
#define		ANI_RISE10			134
#define		ANI_RISEB			135
#define		ANI_RISES			136
#define		ANI_BLOCKPAIN2		137
#define		ANI_BLOCKPAIN3		138
#define		ANI_BLOCKPAIN4		139
#define		ANI_BLOCKPAIN5		140
#define		ANI_BLOCKPAIN6		141
#define		ANI_BLOCKPAIN7		142
#define		ANI_BLOCKPAIN8		143
#define		ANI_BLOCKPAIN9		144
#define		ANI_BLOCKPAIN10		145
#define		ANI_BLOCKPAINB		146
#define		ANI_BLOCKPAINS		147
#define     ANI_CHIPDEATH       148
#define     ANI_GUARDBREAK      149
#define		ANI_RISEATTACK2	    150
#define		ANI_RISEATTACK3		151
#define		ANI_RISEATTACK4		152
#define		ANI_RISEATTACK5		153
#define		ANI_RISEATTACK6		154
#define		ANI_RISEATTACK7		155
#define		ANI_RISEATTACK8		156
#define		ANI_RISEATTACK9		157
#define		ANI_RISEATTACK10	158
#define		ANI_RISEATTACKB		159
#define		ANI_RISEATTACKS		160
#define		ANI_WALKOFF			161

#define		MAX_ANIS			162    // max_anis increased for new ANIs

#define     ARG_FLOAT            0
#define     ARG_STRING           1
#define     ARG_INT              2

// perhaps outdated, now use separted flags for entity
#define     SUBJECT_TO_WALL      1
#define     SUBJECT_TO_HOLE      2
#define     SUBJECT_TO_OBSTACLE  4
#define     SUBJECT_TO_BORDER    8
#define     SUBJECT_TO_SCREEN    16
#define     SUBJECT_TO_MINZ      32
#define     SUBJECT_TO_MAXZ      48

#define     MAX(x, y)            ((x>y)?x:y)
#define     MIN(x, y)            ((x<y)?x:y)

//macros for drawing menu text, fits different font size
#define fmw font_monowidths
#ifdef _MSC_VER
#define _strmidx(f,s, ...) ((videomodes.hRes-font_string_width((f), s, __VA_ARGS__))/2)
#else
#define _strmidx(f,s, args...) ((videomodes.hRes-font_string_width((f), s, ##args))/2)
#endif
#define _colx(f,c) ((int)(videomodes.hRes/2+(c)*(font_monowidths[(f)]+1)))
#define _liney(f,l) ((int)(videomodes.vRes/2+(l)*(font_heights[(f)]+1)))
#ifdef _MSC_VER
#define _menutextm(f, l, shift, s, ...) font_printf(_strmidx(f,s, __VA_ARGS__)+(int)((shift)*(font_monowidths[(f)]+1)), _liney(f,l), (f), 0, s, __VA_ARGS__)
#define _menutext(f, c, l, s, ...) font_printf(_colx(f,c), _liney(f,l), (f), 0, s, __VA_ARGS__)
#else
#define _menutextm(f, l, shift, s, args...) font_printf(_strmidx(f,s, ##args)+(int)((shift)*(font_monowidths[(f)]+1)), _liney(f,l), (f), 0, s, ##args)
#define _menutext(f, c, l, s, args...) font_printf(_colx(f,c), _liney(f,l), (f), 0, s, ##args)
#endif

/*
#define     ICO_NORMAL           0
#define     ICO_PAIN             1
#define     ICO_DIE              2
#define     ICO_GET              3
#define     ICO_WEAPON           4*/

// model flags
#define     MODEL_NO_COPY         0x00000001   //dont copy anything from original model
#define     MODEL_NO_WEAPON_COPY  0x00000002   //dont copy weapon list from original model

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
						 if(e->magictime>0)e->magictime++;\
						 if(e->guardtime>0)e->guardtime++;\
						 if(e->toss_time>0)e->toss_time++;\
						 if(e->freezetime>0 && (textbox || smartbomber))e->freezetime++;\
						 if(e->mpchargetime>0)e->mpchargetime++;\
						 if(e->invinctime>0) e->invinctime++;\
						 if(e->sealtime>0) e->sealtime++;
/*                       if(e->dot_time>0) e->dot_time++;\
						 if(e->dot_cnt>0) e->dot_cnt++;
*/

#define freezeall        (smartbomber || textbox)

#define is_projectile(e) (e->modeldata.type == TYPE_SHOT || e->model->subtype == SUBTYPE_ARROW || e->owner)

#define screeny ((level->scrolldir == SCROLL_UP || level->scrolldir == SCROLL_DOWN )? 0:advancey )
#define screenx advancex

#define check_range(self, target, animnum) \
		 ( target && \
		  (self->direction ? \
		  target->x > self->x+self->modeldata.animation[animnum]->range.xmin &&\
		  target->x < self->x+self->modeldata.animation[animnum]->range.xmax\
		:\
		  target->x < self->x-self->modeldata.animation[animnum]->range.xmin &&\
		  target->x > self->x-self->modeldata.animation[animnum]->range.xmax)\
		  && target->z - self->z > self->modeldata.animation[animnum]->range.zmin \
		  && target->z - self->z < self->modeldata.animation[animnum]->range.zmax \
		  && (target->a - self->a) > self->modeldata.animation[animnum]->range.amin \
		  && (target->a - self->a) < self->modeldata.animation[animnum]->range.amax \
		  && (target->base - self->base) > self->modeldata.animation[animnum]->range.bmin \
		  && (target->base - self->base) < self->modeldata.animation[animnum]->range.bmax \
		  )\

#define check_range_both(self, target, animnum) \
		 ( target && \
		  ((target->x > self->x+self->modeldata.animation[animnum]->range.xmin &&\
			target->x < self->x+self->modeldata.animation[animnum]->range.xmax)\
		||\
		   (target->x < self->x-self->modeldata.animation[animnum]->range.xmin &&\
			target->x > self->x-self->modeldata.animation[animnum]->range.xmax))\
		  && target->z - self->z > self->modeldata.animation[animnum]->range.zmin \
		  && target->z - self->z < self->modeldata.animation[animnum]->range.zmax \
		  && (target->a - self->a) > self->modeldata.animation[animnum]->range.amin \
		  && (target->a - self->a) < self->modeldata.animation[animnum]->range.amax \
		  && (target->base - self->base) > self->modeldata.animation[animnum]->range.bmin \
		  && (target->base - self->base) < self->modeldata.animation[animnum]->range.bmax \
		  )\


#define tobounce(e) (e->animation->bounce && diff(0, e->tossv) > 1.5 && \
					 !((autoland == 1 && e->damage_on_landing == -1) ||e->damage_on_landing == -2))

#define getpal ((current_palette&&level)?(level->palettes[current_palette-1]):pal)

#define canbegrabbed(self, other) \
		(other->animation->vulnerable[other->animpos] && \
		 (!self->animation->move || self->animation->move[self->animpos] == 0) && \
		 (!self->animation->movez || self->animation->movez[self->animpos] == 0 ) && \
		 !(other->nograb || other->invincible || other->link || \
		   other->model->animal || inair(other) || \
		  (self->modeldata.type == TYPE_PLAYER && other->modeldata.type == TYPE_PLAYER && savedata.mode)))

#define cangrab(self, other) \
		((other->modeldata.antigrab - self->modeldata.grabforce + \
		  (other->modeldata.paingrab?(other->modeldata.paingrab-other->inpain):0)<=0) &&\
		 canbegrabbed(self, other) && \
		 !inair(self) && \
		 diff(other->a, self->a) <= 0.1)

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
	unsigned int	compatibleversion:32;
	short       	gamma:16;
	short       	brightness:16;
	char			usesound:8;						// Use SB
	unsigned short	soundrate:16;					// SB freq
	short       	soundvol:16;					// SB volume
	char			usemusic:8;						// Play music
	short       	musicvol:16;					// Music volume
	short       	effectvol:16;					// Sound fx volume
	char			soundbits:8;					// SB bits
	char			usejoy:8;
	char			mode:8;							// Mode now saves
	char			windowpos:8;
	int				keys[MAX_PLAYERS][12];
	char			showtitles:8;
	char			videoNTSC:8;
	char			screen[7][2];					// Screen Filtering/Scaling Effects
	char            logo:8;
	char            uselog:8;
	char			debuginfo:8;					// FPS, Memory, etc...
	int				fullscreen:8;					// Window or Full Screen Mode
	char			stretch:8;						// Stretch (1) or preserve aspect ratio (0) in fullscreen mode
#if SDL
	char			usegl[2];						// 1 if OpenGL is preferred over SDL software blitting
	float			glscale;						// Scale factor for OpenGL
	char			glfilter[2];					// Simple or bilinear scaling
#endif
#if PSP
	char			pspcpuspeed:8;					// PSP CPU Speed
	char            overscan[4];                    // Control TV Overscan
	char            usetv:8;						// Initilize TV at bootup
#endif

}s_savedata;


typedef struct
{
	unsigned int	compatibleversion:32;
	char			dName[MAX_NAME_LEN+1];			// Difficulty Name
	unsigned short	level:16;						// Level Number
	unsigned short	stage:16;						// Stage
	unsigned char	pLives[MAX_PLAYERS];			// Player Lives Left
	unsigned char	pCredits[MAX_PLAYERS];			// Player Credits Left
	unsigned int    pScores[MAX_PLAYERS];			// Player Scores
	unsigned char	credits:8;						// Number Of Credits
	unsigned short	times_completed:16;
	unsigned short	which_set:16;
	//-------------------new strict save features-----------------------
	char            flag:8;                                 // 0 useless slot 1 only load level number 2 load player info and level
	char			pName[MAX_PLAYERS][MAX_NAME_LEN+1];     // player names
	int             pSpawnhealth[MAX_PLAYERS];              // hit points left
	int             pSpawnmp[MAX_PLAYERS];                  // magic points left
	char            pWeapnum[MAX_PLAYERS];                  // weapon
	char            pColourmap[MAX_PLAYERS];                // colour map
}s_savelevel;


typedef struct
{
	unsigned int	compatibleversion:32;
	unsigned int    highsc[10];
	char			hscoren[10][MAX_NAME_LEN+1];
}s_savescore;

typedef struct
{
	short           attack_force:16;
	int          	hitsound;					    // Sound effect to be played when attack hits opponent
	int             hitflash;                       // Custom flash for each animation, model id
	int             blockflash;                     // Custom bflash for each animation, model id
	int             blocksound;                     // Custom sound for when an attack is blocked
	char            counterattack:8;
	short 			attack_coords[5];               // stick on the only one victim
	char            no_pain:8;
	char			no_kill:8;						// this attack won't kill target (leave 1 HP)
	char            no_flash:8;						// Flag to determine if an attack spawns a flash or not
	char	        no_block:8;						// Flag to determine if an attack is blockable (default 0 - blockable)
	char            grab:8;
	char            force_direction:8;              // 0 dont care, 1 same direction as attacker, -1 opposite drection as attacker, 2 right, -2 left
	char            blast:8;
	char            freeze:8;
	char            steal:8;
	char            forcemap:8;
	char            seal:8;
	int             freezetime:32;
	int             maptime:32;
	int             sealtime:32;
	int             dot;                            //Dot mode.
	int             dot_index;                      //Dot index.
	int             dot_time;                       //Dot time to expire.
	int             dot_force;                      //Dot amount per tick.
	int             dot_rate;                       //Dot tick delay.
	float           dropv[3];                       // fly height/x/z if the target is knoced down
	char            otg:8;                          // Over The Ground. Gives ground projectiles the ability to hit lying ents.
	short           jugglecost:16;                  // cost for juggling a falling ent
	short           guardcost:16;                   // cost for blocking an attack
	char            attack_drop:8;                  // now be a knock-down factor, how many this attack will knock victim down
	char            attack_type:8;
	char            damage_on_landing:8;            // same as throw damage type
	float           grab_distance;                  // suck target near by
	short           pause_add:16;					// Flag to determine if an attack adds a pause before updating the animation
	int             staydown[3];                    // [0] = Add to rise delay. [1] = Add to rise attack delay.
}s_attack;

typedef struct                                      //2011_04_01, DC: Counterstrike when taking hit.
{
	short int       frameend;                       //Last frame of counter range.
	short int       framestart;                     //First frame of counter range.
	signed char     condition;                      //Counter conditions. 1 = Always. 2 = Hostile attacker. 3 = Hostile attacker from front not using freeze attack.
	signed char     damaged;                        //Receive damage from attack. 0 = No damage. 1 = Normal damage.
}s_counterrange;

typedef struct                                      //2011_04_01, DC: Outdated Dive from jump functionality originally added by tails. Retained for backward compatibility.
{
    float           v;                              //Vertical speed.
    float           x;                              //Horizontal speed.
}s_dive;

typedef struct                                      //2011_04_01, DC: HP and/or MP cost to perform special/freespecials.
{
    short int       cost;                           //Amount of energy cost.
    signed char     disable;                        //Disable flag. See check_energy function.
    signed char     mponly;                         //MPonly type. 0 = MP while available, then HP. 1 = MP only. 2 = HP only.
}s_energycost;

typedef struct                                      //2011_04_01, DC: On frame movement (slide, jump, dive, etc.).
{
    int				ent;							//Index of entity to spawn on liftoff of jump action.
    short int		f;                              //Frame to begin jump action.
	float           v;    		                    //Vertical velocity.
	float           x;                              //Horizontal velcoty.
	float           z;                              //Lateral velocity.
}s_jumpframe;

typedef struct                                      //2011_04_01, DC: Behavior when reaching base after jump or fall.
{
	short int       frame;                          //Frame to assume on landing.
	int             ent;                            //Index of entity to spawn on landing.
}s_landframe;

typedef struct                                      //2011_04_01, DC: Animation looping.
{
    short int       frameend;                       //Frame animation reaches before looping.
    short int       framestart;                     //Frame animation loops back to.
    signed char     mode;                           //0 = No loop, 1 = Loop. Redundant after frame additions, but needed for backward compatibility.
}s_loop;

typedef struct                                      //2011_04_01, DC: Frame based screen shake functionality.
{
    signed char     cnt;                            //Repetition count.
    short int       framestart;                     //Frame to start quake.
    signed char     repeat;                         //Repetitons.
    signed char     v;                              //Vertical distance of screen movement (in pixels).
}s_quakeframe;

typedef struct                                      //2011_04_01, DC: Distance to target verification for AI running, jumping, following parent, and combo chains for all entity types.
{
    short int       xmin;                           //Minimum horizontal range.
    short int       xmax;                           //Maximum horizontal range.
    short int       zmin;                           //Minimum lateral range.
    short int       zmax;                           //Maximum lateral range.
    short int       amin;                           //Minimum vertical range.
    short int       amax;                           //Maximum vertical range.
    short int       bmin;                           //Minumum base range.
    short int       bmax;                           //Maximum base rnage.
}s_range;                                           //2011_04_01, DC: Target range verification for various AI and player functions.

typedef struct
{
	int             model_index;
	short			numframes:16;
	s_loop 		    loop;                           // Animation looping. 2011_03_31, DC: Moved to struct.
	short           height:16;                      // entity's height during animation
	short			tossframe:16;					// Used to determine which frame will toss a bomb/grenade
	short			shootframe:16;
	short			throwframe:16;
	short			throwa:16;						//	Used for setting the "a" at which weapons are spawned
	// various entity model id, knife/star/bomb etc
	int             custknife;
	int             custstar;
	int             custbomb;                       // Used for new projectile bomb
	int             custpshotno;
	int             subentity;                      // Store the sub-entity's name for further use
	char			fastattack:8;					// Flag to determine if the opponent uses their pain time
	s_energycost	energycost;					    // 1-10-05 to adjust the amount of energy used for specials. 2011_03_31, DC: Moved to struct.
	float           chargetime;                     // charge time for an animation
	s_jumpframe     jumpframe;                      // Jumpframe action. 2011_04_01, DC: moved to struct.
	float           bounce;                         // -tossv/bounce = new tossv
	s_dive			dive;						    // dive kick by tails. 2011_04_01, DC: Moved to struct.
	int*	        soundtoplay;                    // each frame can have a sound
	int*			sprite;                         // sprite[set][framenumber]
	short*			delay;
	short*			move;
	short*			movez;
	short*			movea;
	short*			seta;							// Now characters can have a custom "a" value
	short*			vulnerable;
	short			(*bbox_coords)[5];
	int*            shadow;
	unsigned char*	idle;							// Allow free move
	short			(*shadow_coords)[2];            // x, z offset of shadow
	s_drawmethod    **drawmethods;
	char            attackone:8;
	s_attack**      attacks;
	float			(*platform)[8];					// Now entities can have others land on them
	s_range			range;						    // Verify distance to target, jump landings, etc.. 2011_04_01, DC: Moved to struct.
	short			flipframe:16;					// Turns entities around on the desired frame
	short			followanim:16;					// use which FOLLOW anim?
	char			followcond:8;					// conditions under which to use a followup
	s_counterrange	counterrange;				    // Auto counter attack. 2011_04_01, DC: Moved to struct.
	char            cancel:8;                       // Cancel anims with freespecial
	short*			weaponframe;					// Specify with a frame when to switch to a weapon model
	s_quakeframe	quakeframe;					    // Screen shake effect. 2011_04_01, DC; Moved to struct.
    float*          spawnframe;					    // Spawn the subentity as its default type. {frame} {x} {z} {a} {relative?}
    float*          summonframe;					// Summon the subentity as an ally, only one though {frame} {x} {z} {a} {relative?}
	short           unsummonframe:16;               // Un-summon the entity
	s_landframe     landframe;                      // Landing behavior. 2011_04_01, DC: Moved to struct.
	short           dropframe:16;                   // if tossv < 0, this frame will be set
	short           animhits:16;                    // Does the attack need to hit before cancel is allowed?
	short			sync:16;						// sychronize frame to previous animation if they matches
}s_anim;

typedef struct
{
	int     mode;
	float   factor;
	int     cap_min;
	int     cap_max;
	int     range_min;
	int     range_max;
}s_edelay;

struct animlist{
	s_anim *anim;
	struct animlist *next;
};
typedef struct animlist s_anim_list;
s_anim_list *anim_list;

typedef enum {
	horizontalbar = 0,
	verticalbar = 1,
} barorient;

typedef enum {
	valuebar = 0,
	percentagebar = 1,
} bartype;

typedef struct
{
   short        offsetx:16;
   short        offsety:16;
   short        sizex:16;
   short        sizey:16;
   bartype      type:8;
   barorient    orientation:8;
   char         noborder:8;
   char         direction:8;  //0) left to right or botom to top 1) reversed
   int          barlayer:32;
   int          backlayer:32;
   int          borderlayer:32;
   int          shadowlayer:32;
   int          (*colourtable)[11]; //0 default backfill 1-10 foreground colours
}s_barstatus;

typedef enum {
	LSTYPE_NONE = 0,
	LSTYPE_BAR = 1,
	LSTYPE_BACKGROUND = 2,
} loadingScreenType;

typedef struct {
	loadingScreenType set;
	/*set determines how loading screen would be.
	- 0 = no loading screen.
	- 1 = background and status bar.
	- 2 = background only.
	- 3 = status bar only.
	*/
	char tf; //determines used font number for "LOADING" text (last element in command, moved here because of alignment)
	/*
	- 0 = font.gif
	- 1 = font2.gif
	- 2 = font3.gif
	- 3 = font4.gif */
	short bx; //determines x and y coordinates of loading bar top left's location respectively
	short by;
	short bsize; // length of bar in pixels
	short tx; //determines x and y coordinates of "LOADING" text location respectively.
	short ty;
	short refreshMs; // modder defined number of milliseconds in which the screen is updated while loading
} s_loadingbar;

typedef struct {
	Script*         animation_script;               //system generated script
	Script*         update_script;                  //execute when update_ents
	Script*         think_script;                   //execute when entity thinks.
	Script*         takedamage_script;              //execute when taking damage.
	Script*         ondeath_script;                 //execute when killed in game.
	Script*         onkill_script;                  //execute when removed from play.
	Script*         onpain_script;                  //Execute when put in pain animation.
	Script*         onfall_script;                  //execute when falling.
	Script*         onblocks_script;                //execute when blocked by screen.
	Script*         onblockw_script;                //execute when blocked by wall.
	Script*         onblocko_script;                //execute when blocked by obstacle.
	Script*         onblockz_script;                //execute when blocked by Z.
	Script*         onblocka_script;                //execute when "hit head".
	Script*         onmovex_script;                 //execute when moving along X axis.
	Script*         onmovez_script;                 //execute when moving along Z axis.
	Script*         onmovea_script;                 //execute when moving along A axis.
	Script*         didhit_script;                  //execute when attack hits another.
	Script*         onspawn_script;                 //execute when spawned.
	Script*         key_script;                     //execute when entity's player presses a key
	Script*         didblock_script;                //execute when blocking attack.
	Script*         ondoattack_script;              //execute when attack passes do_attack checks.
} s_scripts;

typedef enum  {
	MF_NONE = 0,
	MF_ANIMLIST = 1,
	MF_COLOURMAP = 2,
	MF_PALETTE = 4,
	MF_WEAPONS = 8,
	MF_BRANCH = 16,
	MF_ANIMATION = 32,
	MF_DEF_FACTORS = 64,
	MF_DEF_PAIN = 128,
	MF_DEF_KNOCKDOWN = 256,
	MF_DEF_BLOCKPOWER = 512,
	MF_DEF_BLOCKTRESHOLD = 1024,
	MF_DEF_BLOCKRATIO = 2048,
	MF_DEF_BLOCKTYPE = 4096,
	MF_OFF_FACTORS = 8192,
	MF_SPECIAL = 16384,
	MF_SMARTBOMB = 32768,
	MF_SCRIPTS = 65536,
} ModelFreetype;
#define MF_ALL 0x1FFFF

typedef struct
{
    short int       current;                        //Current guard points remaining.
    short int       maximum;                        //Maximum guard points.
} s_guardpoints;                                    //2011_04_05, DC: Guardpoints feature added by OX.

typedef struct
{
    short int		def;                            //Default icon.
    short int       die;     					    //Health depleted.
    short int		get;    					    //Retrieving item.
	short int       mphigh;                         //MP bar icon; at 66% or more (default if other mp icons not used).
	short int       mplow;                          //MP bar icon; at or between 0% and 32%.
	short int       mpmed;                          //MP bar icon; at or between 33% and 65%.
	short int 		pain;   					    //Taking damage.
	short int       weapon;                         //Weapon model.
	short int       x;                              //X position.
	short int       y;                              //Y position.
} s_icon;                                           //2011_04_05, DC: In game icons added 2005_01_20.

typedef struct
{
    short int       current;                        //Current juggle points accumulated.
    short int       maximum;                        //Maximum juggle points possible.
} s_jugglepoints;                                   //2011_04_05, DC: Jugglepoints feature added by OX.

typedef struct
{
    signed char     frozen;                         //Frozen.
    signed char     hide_end;                       //End range for maps hidden during character selection.
    signed char     hide_start;                     //Start range for maps hidden during character selection.
    signed char     ko;                             //Health depleted.
    signed char     kotype;                         //KO map application. 0 = Immediately. 1 = At last frame of fall/death animation.
} s_maps;                                           //2011_04_07, DC: Pre defined color map selections and behavior.

typedef struct
{
    short int       amax;                           //Maximum vertical range.
    short int       amin;                           //Minimum vertical range.
    short int       xmax;                           //Maximum horizontal range.
    short int       xmin;                           //Minimum horizontal range.
    short int       zmax;                           //Maximum lateral range.
    short int       zmin;                           //Minimum lateral range.
} s_sight;                                          //2011_04_05, DC: Range from self AI can detect other entities.

typedef struct
{
    signed char     detect;                         //Invisbility penetration. If self's detect >= target's hide, self can "see" target.
    signed char     hide;                           //Invisibility to AI.
} s_stealth;                                        //2011_04_05, DC: Invisibility to AI feature added by DC.


// WIP
typedef struct
{
	int inputs[MAX_SPECIAL_INPUTS];
	int cond;
	int anim;
	int cancel;
} s_com;

typedef struct{
	float x;
	float z;
} point2d;

typedef struct
{
	int             index;
	char*           name;
	char*           path;                           // Path, so scripts can dynamically get files, sprites, sounds, etc.
	unsigned int    score:32;
	float           stats[20];                      // Parameters that do nothing on their own.
	int  			health;
	float           scroll;                         // Autoscroll like panel entity.
	unsigned        offscreenkill;                  // for biker, arrow, etc
	//unsigned        offscreenkillz;
	//unsigned        offscreeenkila;
	int 			mp;							    // mp's variable for mpbar by tails
	short			counter:16;						// counter of weapons by tails
	unsigned char	shootnum:8; 					// counter of shots by tails
	unsigned char	reload:8; 						// reload max shots by tails
	char			reactive:8;						// Used for setting the "a" at which weapons are spawned
	char			typeshot:8; 					// see if weapon is a gun or knife by tails
	char			animal:8;  						// see is the weapon is a animal by tails
	char 			nolife:8;	 					// Feb 25, 2005 - Variable flag to show life 0 = no, else yes
	int 			makeinv;	 					// Option to spawn player invincible >0 blink <0 noblink
	int             riseinv;                        // how many seconds will the character become invincible after rise >0 blink, <0 noblink
	char			dofreeze:8;						// Flag to freeze all enemies/players while special is executed
	char			noquake:8;						// Flag to make the screen shake when entity lands 1 = no, else yes
	char			ground:8;						// Flag to determine if enemy projectiles only hit the enemy when hitting the ground
	int 			multiple;						// So you can control how many points are given for hitting opponents
	char			bounce:8;						// Flag to determine if bounce/quake is to be used.
	short			type:16;
	char			subtype:8;
	s_icon          icon;                           //In game icons added 2005_01_20. 2011_04_05, DC: Moved to struct.
	int				parrow[MAX_PLAYERS][3];			// Image to be displayed when player spawns invincible
	int				setlayer;						// Used for forcing enities to be displayed behind
	short			thold:16;						// The entities threshold for block
	s_maps          maps;                           //2011_04_07, DC: Pre defined color map selections and behavior.
	char			alpha:8;						// New alpha variable to determine if the entity uses alpha transparency
	char			toflip:8;						// Flag to determine if flashes flip or not
	char			shadow:8;
	char            gfxshadow:8;                    // use current frame to create a shadow
	char			aironly:8;						// Used to determine if shadows will be shown when jumping only
	char			nomove:8;						// Flag for static enemies
	char			noflip:8;						// Flag to determine if static enemies flip or stay facing the same direction
	char			nodrop:8;						// Flag to determine if enemies can be knocked down
	char			nodieblink:8;					// Flag to determine if blinking while playing die animation
	char			holdblock:8;					// Continue the block animation as long as the player holds the button down
	char            nopassiveblock:8;               // Don't auto block randomly
	char			blockback:8;					// Able to block attacks from behind
	short			blockodds:16;					// Odds that an enemy will block an attack (1 : blockodds)
	s_edelay        edelay;                         // Entity level delay adjustment.
	float			runspeed;						// The speed the character runs at
	float			runjumpheight;					// The height the character jumps when running
	float			runjumpdist;					// The distance the character jumps when running
	int				noatflash:8;					// Flag to determine if attacking characters attack spawns a flash
	int				runupdown:8;					// Flag to determine if a player will continue to run while pressing up or down
	int				runhold:8;						// Flag to determine if a player will continue to run if holding down forward when landing
	int				remove:8;						// Flag to remove a projectile on contact or not
	float			throwheight;					// The height at which an opponent can now be adjusted
	float			throwdist;						// The distance an opponent can now be adjusted
	short           throwframewait:16;              // The frame victim is thrown during ANIM_THROW, added by kbandressen 10/20/06
	int 			(*special)[MAX_SPECIAL_INPUTS]; // Stores freespecials
	short			specials_loaded:16;				// Stores how many specials have been loaded
	short			valid_special:16;				// Used for setting when a valid special has been found
	int         	diesound;
	char			weapnum:8;
	char			secret:8;
	char			weaploss[2];					// Determines possibility of losing weapon.
	char            ownweapons:8;                   // is the weapon list own or share with others
	int             *weapon;						// weapon model list
	int				numweapons;

	// these are model id of various stuff
	int             project;
	int             rider;   		                // 7-1-2005 now every "biker" can have a new driver!
	int             knife;   		                // 7-1-2005 now every enemy can have their own "knife" projectile
	int             pshotno;   		                // 7-1-2005 now every enemy can have their own "knife" projectile
	int             star;   			            // 7-1-2005 now every enemy can have their own "ninja star" projectiles
	int             bomb;   			            // New projectile type for exploding bombs/grenades/dynamite
	int             flash;   			            // Now each entity can have their own flash
	int             bflash;                         // Flash that plays when an attack is blocked
	int             dust[3];   			            // Dust spawn (0 = Fall land, 1 = Jumpland, 2 = Jumpstart.)
	short			height:16;						// Used to set height of player in pixels
	float			speed;
	float			grabdistance;					// 30-12-2004	grabdistance varirable adder per character
	float			pathfindstep;					// UT: how long each step if the entity is trying to find a way
	char			grabflip;						// Flip target or not, bit0: grabber, bit1: opponent
	float           jumpspeed;                      // normal jump foward speed, default to max(1, speed)
	float			jumpheight;						// 28-12-2004	Jump height variable added per character
	char            jumpmovex:8;                    // low byte: 0 default 1 flip in air, 2 move in air, 3 flip and move
	char            jumpmovez:8;                    // 2nd byte: 0 default 1 zjump with flip(not implemented yet) 2 z jump move in air, 3 1+2
	char            grabfinish:8;                   // wait for grab animation to finish before do other actoins
	char 			antigrab:8;						// anti-grab factor
	int             grabforce;                      // grab factor, antigrab - grabforce <= 0 means can grab
	char            facing:8;                       // 0 no effect, 1 alway right, 2 always left, 3, affected by level dir
	char			grabback:8;						// Flag to determine if entities grab images display behind opponenets
	char            grabturn:8;
	char			paingrab:8;						// Can only be grabbed when in pain
	float           grabwalkspeed;
	short			throwdamage:16;					// 1-14-05  adjust throw damage
	unsigned char*  palette;                        // original palette for 32/16bit mode
	unsigned char*	colourmap[MAX_COLOUR_MAPS];
	char			maps_loaded:8;					// Used for player colourmap selecting
	char			unload:8; 						// Unload model after level completed?
	char			falldie:8; 						// Play die animation?
	char            globalmap:8;                    // use global palette for its colour map in 24bit mode
	char			nopain:8;
	char            summonkill:8;                   // kill it's summoned entity when died;  0. dont kill 1. kill summoned only 2. kill all spawned entity
	char            combostyle:8;
	char			blockpain:8;
	char            atchain[MAX_ATCHAIN];
	char		    chainlength:8;
	s_anim**        animation;
	char			credit:8;
	char			escapehits:8;					// Escape spammers!
	char			chargerate:8;					// For the charge animation
	char            guardrate:8;                    // Rate for guardpoints recover.
	char			mprate:8;						// For time-based mp recovery.
	char            mpdroprate:8;                   // Time based MP loss.
	char            mpstable:8;                     // MP stable type.
	short           mpstableval:16;                 // MP Stable target.
	short			aggression:16;					// For enemy A.I.
	short			risetime[2];					// 0 = Rise delay, 1 = Riseattack delay.
	short			sleepwait:16;
	char            riseattacktype:8;
	s_jugglepoints  jugglepoints;                   // Juggle points feature by OX. 2011_04_05, DC: Moved to struct.
	s_guardpoints   guardpoints;                    // Guard points feature by OX. 2011_04_05, DC: Moved to struct.
	char            mpswitch:8;                     // switch between reduce or gain mp for mpstabletype 4
	short           turndelay:16;                   // turn delay
	float           lifespan;                       // lifespan count down
	float           knockdowncount;                 // the knock down count for this entity
	s_stealth       stealth;						// Invisibility to AI feature added by DC. 2011_04_05, DC: Moved to struct.

	//---------------new A.I. switches-----------
	short           hostile:16;                     // specify hostile types
	short           candamage:16;                   // specify types that can be damaged by this entity
	short           projectilehit:16;               // specify types that can be hit by this entity if it is thrown
	unsigned int    aimove:32;                      // move style
	s_sight			sight;						    // Sight range. 2011_04_05, DC: Moved to struct.
	unsigned int    aiattack:32;                    // attack/defend style

	//----------------physical system-------------------
	float           antigravity;                    //antigravity : gravity * (1- antigravity)

	//--------------new property for endlevel item--------
	char*           branch;                         //level branch name
	char            model_flag:8;                   //used to judge some copy method when setting new model to an entity

	float*          defense_factors;                //basic defense factors: damage = damage*defense
	float*          defense_pain;                   //Pain factor (like nopain) for defense type.
	float*          defense_knockdown;              //Knockdowncount (like knockdowncount) for attack type.
	float*          defense_blockpower;             //If > unblockable, this attack type is blocked.
	float*          defense_blockthreshold;         //Strongest attack from this attack type that can be blocked.
	float*          defense_blockratio;             //% of damage still taken from this attack type when blocked.
	float*          defense_blocktype;             //0 = HP, 1=MP, 2=both taken when this attack type is blocked.
	float*          offense_factors;                //basic offense factors: damage = damage*offense
	s_attack*       smartbomb;

	// e.g., boss
	s_barstatus     hpbarstatus;
	short           hpx:16;
	short           hpy:16;
	short           namex:16;
	short           namey:16;

	// movement flags
	char            subject_to_wall:8;
	char            subject_to_platform:8;
	char            subject_to_obstacle:8;
	char            subject_to_hole:8;
	char            subject_to_gravity:8;
	char            subject_to_screen:8;
	char            subject_to_minz:8;
	char            subject_to_maxz:8;
	char            no_adjust_base:8;               // dont change base to 0 automatically
	char            instantitemdeath:8;             // no delay before item suicides
	char isSubclassed;
	ModelFreetype	freetypes;
	s_scripts	scripts;
}s_model;

typedef struct
{
	char			*name;
	char			*path;
	s_model*        model;
	char            loadflag;
	char            selectable;
}s_modelcache;
s_modelcache *model_cache;

typedef struct
{
    int             rise;                           //Time modifier before rise.
    int             riseattack;                     //Time modifier before riseattack.
    int             riseattack_stall;               //Total stalltime before riseattack.
} s_staydown;                                       //2011_04_08, DC: Delay modifiers before rise or riseattack can take place.

typedef struct entity
{
	char            spawntype:8;                    // Type of spawn. 1 = Level spawn. 0 for all else (subject to change).
	char			exists:8;                       // flag to determine if it is a valid entity.
	char			reactive:8;						// Used for setting the "a" at which weapons are spawned
	char			ptype:8;
	char			playerindex:8;
	float           stats[20];                      // Parameters that do nothing on their own.
	int             health;                         // current hp
	int             mp;                             // current mp
	int  			oldhealth;
	int  			oldmp;							//mp's variable for mp for players by tails
	char			name[MAX_NAME_LEN+1];           // this is display name
	s_model         *defaultmodel;                  // this is the default model
	s_model 		*model;                         // current model
	s_model         modeldata;                      // model data copyied here
	short 			item:16;                        // item model id
	char			itemmap:8;						// Now items spawned can have their properties changed
	char			itemtrans:8;                    // alpha effect of item
	char			itemalias[MAX_NAME_LEN+1];		// Now items spawned can have their properties changed
	int  			itemhealth;						// Now items spawned can have their properties changed
	short           itemplayer_count:8;
	short			boss:8;
	char			dying:8;						// Coresponds with which remap is to be used for the dying flash
	unsigned short	per1:16;							// Used to store at what health value the entity begins to flash
	unsigned short	per2:16;							// Used to store at what health value the entity flashes more rapidly
	char			direction:8;					// 0=left 1=right
	char			nograb:8;			            // Some enemies cannot be grabbed (bikes) - now used with cantgrab as well
	char			movestep:8;
	float			x;								// X
	float			z;								// Depth
	float			a;								// Altitude
	float			xdir;
	float			zdir;
	float           destx;							// temporary values for ai functions
	float			destz;
	float			base;							// Default altitude
	float           altbase;                        // Altitude affected by movea
	float			tossv;							// Effect of gravity
	float           jumpz;
	float           jumpx;
	float           jumpv;
	short           jumpid:16;
	unsigned char   combostep[MAX_SPECIAL_INPUTS];  // merge into an array to clear up some code

	// ---------------------- action times -------------------------------
	unsigned int	lastmove;
	unsigned int    lastdir;
	unsigned int    timestamp;
	unsigned int	releasetime;
	unsigned int	toss_time;						// Used by gravity code
	unsigned int	stalltime;
	unsigned int	combotime;						// For multiple-hit combo
	unsigned int	movetime;						// For special move
	unsigned int	freezetime;						// Used to store at what point the a frozen entity becomes unfrozen
	unsigned int	maptime;						// used by forcemap
	unsigned int	sealtime;						// used by seal (stops special moves).
	unsigned int    dot_time[MAX_DOTS]; 			//Dot time to expire.
	int             dot[MAX_DOTS];                  //Dot mode.
	int             dot_atk[MAX_DOTS];              //Dot attack type.
	int             dot_force[MAX_DOTS];            //Dot amount.
	int             dot_rate[MAX_DOTS];             //Dot delay per tick.
	int	            dot_cnt[MAX_DOTS];              //Dot time of next tick.
	struct entity   *dot_owner[MAX_DOTS];           //Dot owner.
	unsigned int 	magictime;
	unsigned int    guardtime;
	unsigned int 	nextanim;
	unsigned int 	nextthink;
	unsigned int 	pain_time;
	unsigned int    mpchargetime;					// For the CHARGE animation
	unsigned int    sleeptime;						// For the SLEEP animation
	unsigned int 	rushtime;						// rush combo timer
	unsigned int    knockdowntime;                  // count knock down hit
	unsigned int 	invinctime;						// Used to set time for invincibility to expire
	unsigned int    turntime;
	s_staydown      staydown;                       //Delay modifiers before rise or riseattack can take place. 2011_04_08, DC: moved to struct.
	// -------------------------end of times ------------------------------
	char            update_mark:8;

	//------------------------- a lot of flags ---------------------------

	char            seal:8;                         //1 = No specials.
	char            dead:8;
	char            jumping:8;						// Stuff useful for AI
	char            idling:8;
	char            drop:8;
	char            attacking:8;
	char            getting:8;
	char            turning:8;
	char            charging:8;
	char            blocking:8;
	char			falling:8;
	char            running:8;						// Flag to determine if a player is running
	char            grabwalking:8;                    // a flag for grabwalk check
	char            inpain:8;                         // playing pain animation
	char          	frozen:8;							// Flag to determine if an entity is frozen
	char			blink:8;
	char			invincible:8;						// Flag used to determine if player is currently invincible
	char			autokill:8;						// Kill on end animation
	char			remove_on_attack:8;
	char			cantfire:8;						// Flag to determine if another shot can be fired that costs energy
	char			tocost:8;							// Flag to determine if special costs life if doesn't hit an enemy
	char            noaicontrol:8;                    // pause A.I. control
	char            projectile:8;
	char            toexplode:8;						// Needed to determine if the projectile is a type that will explode (bombs, dynamite, etc)
	char			animating:8;						// Set by animation code
	char			arrowon:8;						// Flag to display parrow/parrow2 or not
	unsigned char   pathblocked:8;
	point2d * waypoints;
	int numwaypoints;

	//---------------------   end of flags ----------------------------------------------
	short			animpos:16;
	short			lastanimpos:16;					// Used by AI
	short			animnum:16;                     // animation id
	s_anim			*animation;
	float           knockdowncount;
	char			damage_on_landing:8;
	char            damagetype:8;                   // used for set death animation or pain animation
	char            map:8;							// Stores the colourmap for restoring purposes
	void			(*think)();
	void            (*takeaction)();
	int             (*takedamage)(struct entity*,s_attack*);
	int             (*trymove)(float, float);
	short			attack_id:16;
	short			hit_by_attack_id:16;
	unsigned char	*colourmap;
	//struct entity   *thrower;
	struct entity	*link;							// Used to link 2 entities together.
	struct entity	*owner;							// Added for "hitenemy" flag so projectile recognizes its owner
	struct entity	*grabbing;						// Added for "platform level" layering
	struct entity	*weapent;
	struct entity   *parent;                        //Its spawner
	struct entity   *subentity;                     //store the sub entity
	struct entity   *opponent;
	struct entity   *lasthit;
	struct entity   *hithead;                       // when a player jumps and hits head on the bottom of a platform
	struct entity   *bound;                         // ignore trymove method, follow this entity
	struct entity   *landed_on_platform;
	short           bindoffset[4];                  // x, z, a, dir; int is ok
	short           bindanim:16;                    // keep the bound entities same animation id
	char			escapecount:8;					// For escapehits
	unsigned short	rush[2];						// rush combo and max combo
	float           lifespancountdown;              // life span count down

	//------------- these factors will be added by basic factors of model-------------
	float*          defense_factors;                //defense factors: damage = damage*(1-def)
	float*          defense_pain;                   //Pain factor (like nopain) for defense type.
	float*          defense_knockdown;              //Knockdowncount (like knockdowncount) for attack type.
	float*          defense_blockpower;             //If > unblockable, this attack type is blocked.
	float*          defense_blockthreshold;         //Strongest attack from this attack type that can be blocked.
	float*          defense_blockratio;             //% of damage still taken from this attack type when blocked.
	float*          defense_blocktype;              //0 = HP, 1=MP, 2=both taken when this attack type is blocked.
	float*          offense_factors;                //offense factors: damage = damage*(1+def)
	float           antigravity;                    // gravity*(1-antigravity)
	//float			waypoint[3];					//next waypoint, ignore target in this case

	//-------------------A.I. movement factors ----------------------------
	int             sortid;                         // id for sprite queue sort
	ScriptVariant* entvars;
	s_drawmethod    drawmethod;
	s_scripts scripts;
}entity;



typedef struct
{
	char			name[MAX_NAME_LEN+1];
	char			colourmap:8;
	unsigned int    score:32;
	unsigned char	lives:8;
	unsigned char	credits:8;
	entity			*ent;
	u32				keys;
	u32				newkeys;
	u32				playkeys;
	u32				releasekeys;
	int			    spawnhealth;
	int  			spawnmp;
	char			joining:8;
	char			hasplayed:8;
	char			weapnum:8;
}s_player;


typedef struct s_spawn_script_cache_node
{
	char		*filename;
	Script		*cached_spawn_script;
	struct s_spawn_script_cache_node *next;
} s_spawn_script_cache_node;

typedef struct s_spawn_script_list_node
{
	Script		*spawn_script;
	struct s_spawn_script_list_node *next;
} s_spawn_script_list_node;


typedef struct
{
	short			at:16;
	char			wait:8;
	char            nojoin:8;                   // dont allow new hero to join
	char            spawnplayer_count:8;        // spawn this entity according to the amount of players
	char            palette:8;                  //change system palette to ...
	short			groupmin:16;
	short			groupmax:16;
	short           scrollminz:16;              // new scroll limit
	short           scrollmaxz:16;
	short           blockade:16;                //limit how far you can go back
	short           light[2];                   // x, z  light direction, for gfx shadow
	char            shadowcolor:8;              // -1 no shadow
	int             shadowalpha:8;
	char			music[128];
	float           musicfade;
	u32             musicoffset;
	char			*name;                      // must be a name in the model list, so just reference
	int             index;                      // model id
	int             itemindex;                  // item model id
	int             weaponindex;                // the spawned entity with an weapon item, this is the id of the item model
	char			alpha:8;		  		    // Used for alpha effects
	char			boss:8;
	char			flip:8;
	char			itemtrans:8;
	char			itemmap:8;
	char			colourmap:8;
	char			dying:8;					// Used for the dying flash animation
	unsigned short	per1:16;						// Used to store at what health value the entity begins to flash
	unsigned short	per2:16;					    // Used to store at what health value the entity flashes more rapidly
	char			nolife:8;					// So nolife can be overriden for all characters
	char            itemplayer_count:8;         // spawn the item according to the amount of players
	s_model         *itemmodel;
	s_model         *model;
	char			alias[MAX_NAME_LEN+1];
	char			*item;                      // must be a name in the model list, so just reference
	char			itemalias[MAX_NAME_LEN+1];
	int			    itemhealth;
	int			    health[MAX_PLAYERS];
	int			    mp;							// mp's variable for mpbar by tails
	unsigned int	score:32;					// So score can be overridden for enemies/obstacles
	int				multiple;				// So score can be overridden for enemies/obstacles
	// coords
	float			x;
	float			z;
	float			a;
	unsigned char	credit:8;
	short			aggression:16;				// For enemy A.I.
	char            spawntype;                  // Pass 1 when a level spawn.
	char            *weapon;                    // spawn with a weapon, since it should be in the model list, so the model must be loaded, just reference its name
	s_model         *weaponmodel;
	s_spawn_script_list_node*          spawn_script_list_head;
}s_spawn_entry;

typedef enum
{
	normal_level=0,
	cut_scene = 1,
	select_screen = 2,
}le_type;

typedef struct
{
	char            *branchname;            // Use a name so we can find this level in branches
	char			*filename;
	le_type         type;                   // see le_type
	int				z_coords[3];			// Used for setting custom "z"
	int				gonext;                 // 0. dont complete this level and display score,
											// 1. complete level and display score,
											// 2. complete game, show hall of fame
}s_level_entry;


//
typedef enum
{
	bgt_bglayer,
	bgt_fglayer,
	bgt_panel,
	bgt_frontpanel,
	bgt_water,
	bgt_background,
	bgt_generic
	
}bgloldtype;


typedef struct
{
	bgloldtype     oldtype;
	int            order;	//for panel order
	gfx_entry      gfx;
	int            width;
	int            height;
	float          xratio;
	float          zratio;
	int            xoffset;
	int            zoffset;
	int			   xspacing;
	int			   zspacing;
	s_drawmethod   drawmethod;
	float          bgspeedratio;
	int            enabled;
	int            z;
	int            quake;
	int            neon;
}s_layer;


typedef struct
{
	char*			text;
	int				t;		//Time to expire.
	int				x;
	int				y;
	int				font;
	int				z;
}s_textobj;

typedef struct
{
	int pos;
	char *buf;
	size_t size;
}s_filestream;

typedef struct
{
	char* name;
	int numspawns;
	s_spawn_entry spawnpoints[LEVEL_MAX_SPAWNS];
	int numlayers;
	s_layer layers[LEVEL_MAX_LAYERS];
	int numlayersref;
	s_layer layersref[LEVEL_MAX_LAYERS];
	////////////////these below are layer reference
	////////////////use them to ease layer finding for script users
	s_layer* background; // the bglayer that contains the default background
	int numpanels;
	s_layer* panels[LEVEL_MAX_PANELS][3]; //normal neon screen
	int numfrontpanels;
	s_layer* frontpanels[LEVEL_MAX_PANELS];
	int numbglayers;
	s_layer* bglayers[LEVEL_MAX_LAYERS];
	int numfglayers;
	s_layer* fglayers[LEVEL_MAX_LAYERS];
	int numgenericlayers;
	s_layer* genericlayers[LEVEL_MAX_LAYERS];
	int numwaters; // LOL what
	s_layer* waters[LEVEL_MAX_LAYERS];
	////////////////layer reference ends here
	///////////////////////////////////////////////////////////////
	s_textobj textobjs[LEVEL_MAX_TEXTOBJS];
	int numfilestreams;
	int cameraxoffset;
	int camerazoffset;
	int numholes;
	int numwalls;						// Stores number of walls loaded
	float holes[LEVEL_MAX_HOLES][7];
	int holesfound[LEVEL_MAX_HOLES];
	float walls[LEVEL_MAX_WALLS][8];		// Now you can have walls for different walkable areas
	int wallsfound[LEVEL_MAX_WALLS];
	int exit_blocked;
	int exit_hole;
	int scrolldir;
	int width;
	int rocking;
	float bgspeed;						// Used to make autoscrolling backgrounds
	float scrollspeed;					// UT: restore this command  2011/7/8
	int bgdir;							// Used to set which direction the backgrounds scroll for autoscrolling backgrounds
	int mirror;
	int bosses;
	char bossmusic[256];
	unsigned int bossmusic_offset;
	int numpalettes;
	unsigned char palettes[LEVEL_MAX_PALETTES][1024];//dynamic palettes
	unsigned char* blendings[LEVEL_MAX_PALETTES][MAX_BLENDINGS];//blending tables
	int settime;						// Set time limit per level
	int notime;							// Used to specify if the time is displayed 1 = no, else yes
	int noreset;						// If set, clock will not reset when players spawn/die
	int type;							// Used to specify which level type (1 = bonus, else regular)
	int nospecial;						// Used to specify if you can use your special during bonus levels
	int nohurt;							// Used to specify if you can hurt the other player during bonus levels
	int noslow;							// Flag so the level doesn't slow down after a boss is defeated
	int nohit;							// Not able to grab / hit other player on a per level basis
	int spawn[MAX_PLAYERS][4];			// Used to determine the spawn position of players
	int setweap;						// Levels can now specified which weapon will be used by default
	int facing;                         // Force the players to face to ... 0 no effects, 1 right, 2 left, 3 affected by level dir
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
	s_spawn_script_cache_node* spawn_script_cache_head;
	int pos;
	u32 advancetime;
	u32 quaketime;
	int quake;
	int waiting;

}s_level;


typedef struct ArgList {
	size_t count;
	size_t arglen[32];
	char* args[32];
} ArgList;

int     buffer_pakfile(char* filename, char** pbuffer, size_t* psize);
int     getsyspropertybyindex(ScriptVariant* var, int index);
int     changesyspropertybyindex(int index, ScriptVariant* value);
int     load_script(Script* script, char* path);
void    init_scripts();
void    load_scripts();
void    execute_animation_script    (entity* ent);
void    execute_takedamage_script   (entity* ent, entity* other, int force, int drop, int type, int noblock, int guardcost, int jugglecost, int pauseadd);
void    execute_ondeath_script      (entity* ent, entity* other, int force, int drop, int type, int noblock, int guardcost, int jugglecost, int pauseadd);
void    execute_onkill_script       (entity* ent);
void    execute_onpain_script       (entity* ent, int iType, int iReset);
void    execute_onfall_script       (entity* ent, entity* other, int force, int drop, int type, int noblock, int guardcost, int jugglecost, int pauseadd);
void    execute_onblocks_script     (entity* ent);
void    execute_onblockw_script     (entity* ent, int plane, float height);
void    execute_onblocko_script     (entity* ent, entity* other);
void    execute_onblockz_script     (entity* ent);
void    execute_onblocka_script     (entity* ent, entity* other);
void    execute_onmovex_script      (entity* ent);
void    execute_onmovez_script      (entity* ent);
void    execute_onmovea_script      (entity* ent);
void    execute_didblock_script     (entity* ent, entity* other, int force, int drop, int type, int noblock, int guardcost, int jugglecost, int pauseadd);
void    execute_ondoattack_script   (entity* ent, entity* other, int force, int drop, int type, int noblock, int guardcost, int jugglecost, int pauseadd, int iWhich, int iAtkID);
void    execute_updateentity_script (entity* ent);
void    execute_think_script        (entity* ent);
void    execute_didhit_script       (entity* ent, entity* other, int force, int drop, int type, int noblock, int guardcost, int jugglecost, int pauseadd, int blocked);
void    execute_onspawn_script      (entity* ent);
void    clearsettings(void);
void    savesettings(void);
void    saveasdefault(void);
void    loadsettings(void);
void    loadfromdefault(void);
void    clearSavedGame(void);
void    clearHighScore(void);
void    saveGameFile(void);
int     loadGameFile(void);
void    saveHighScoreFile(void);
void    loadHighScoreFile(void);
int music(char *filename, int loop, long offset);
#if DC
void check_music_opened(void);
#endif
char *findarg(char *command, int which);
float diff(float a, float b);
int inair(entity *e);
float randf(float max);
int _makecolour(int r, int g, int b);
int load_colourmap(s_model * model, char *image1, char *image2);
int load_palette(unsigned char* pal, char* filename);
void standard_palette();
void change_system_palette(int palindex);
void unload_background();
void lifebar_colors();
void load_background(char *filename, int createtables);
void unload_texture();
void load_texture(char *filename);
void freepanels();
s_sprite * loadpanel2(char *filename);
int loadpanel(char *filename_normal, char *filename_neon, char *filename_screen);
int loadfrontpanel(char *filename);
void resourceCleanUp(void);
void freesprites();
s_sprite * loadsprite2(char *filename, int* width, int* height);
int loadsprite(char *filename, int ofsx, int ofsy, int bmpformat);
void load_special_sprites();
int load_special_sounds();
s_model * find_model(char *name);
s_model * nextplayermodel(s_model *current);
s_model * prevplayermodel(s_model *current);
void free_anim(s_anim * anim);
void free_models();
s_anim * alloc_anim();
int addframe(s_anim * a, int spriteindex, int framecount, short delay, unsigned char idle,
			 short *bbox, s_attack* attack, short move, short movez,
			 short movea, short seta, float* platform, int frameshadow,
			 short* shadow_coords, int soundtoplay, s_drawmethod* drawmethod);
void cache_model(char *name, char *path, int flag);
void remove_from_cache(char * name);
void free_modelcache();
int get_cached_model_index(char * name);
char *get_cached_model_path(char * name);
s_model* load_cached_model(char * name, char * owner, char unload);
int is_set(s_model * model, int m);
int load_script_setting();
int load_models();
void unload_levelorder();
void add_level(char *filename, int diff);
void add_scene(char *filename, int diff);
void load_levelorder();
void unload_level();
void load_level(char *filename);
void drawlifebar(int x, int y, int h, int maxh);
void drawmpbar(int x, int y, int m, int maxm);
void update_loading(s_loadingbar* s,  int value, int max);
void spawnplayer(int);
void drop_all_enemies();
void kill_all_enemies();
void ent_set_colourmap(entity *ent, unsigned int which);
void predrawstatus();
void drawstatus();
void addscore(int playerindex, int add);
void free_ent(entity* e);
void free_ents();
int alloc_ents();
entity * smartspawn(s_spawn_entry * p);
int adjust_grabposition(entity* ent, entity* other, float dist, int grabin);
int player_trymove(float xdir, float zdir);
void toss(entity *ent, float lift);
void player_think(void);
void subtract_shot(void);
void set_model_ex(entity* ent, char* modelname, int index, s_model* newmodel, int flag);
void dropweapon(int flag);
void biker_drive(void);
void trap_think(void);
void steamer_think(void);
void text_think(void);
void anything_walk(void);
void adjust_walk_animation(entity* other);
void kill(entity*);
int player_takedamage(entity *other, s_attack* attack);
int biker_takedamage(entity *other, s_attack* attack);
int obstacle_takedamage(entity *other, s_attack* attack);
void ent_set_anim(entity*, int, int);
void suicide(void);
void player_blink(void);
void common_prejump();
void common_dot();
void tryjump(float, float, float, int);
void dojump(float, float, float, int);
void biker_drive(void);
void ent_default_init(entity* e);
void ent_spawn_ent(entity* ent);
void ent_summon_ent(entity* ent);
void ent_set_anim(entity *ent, int aninum, int resetable);
void ent_set_colourmap(entity *ent, unsigned int which);
void ent_set_model(entity * ent, char * modelname);
entity * spawn(float x, float z, float a, int direction, char * name, int index, s_model* model);
void ent_unlink(entity *e);
void ents_link(entity *e1, entity *e2);
void kill(entity *victim);
void kill_all();
int checkhit(entity *attacker, entity *target, int counter);
int checkhole(float x, float z);
int testplatform(entity*, float, float, entity*);
int testhole(int, float, float);
int testwall(int, float, float);
int checkwalls(float x, float z, float a1, float a2);
int checkholes(float, float);
int checkwall_below(float x, float z, float a);
int checkwall(float x, float z);
int testmove(entity*, float, float, float, float);
entity * check_platform_below(float x, float z, float a, entity* exclude);
entity * check_platform(float x, float z, entity* exclude);
void do_attack(entity *e);
void update_ents();
entity * find_ent_here(entity *exclude, float x, float z, int types);
void display_ents();
void toss(entity *ent, float lift);
entity * findent(int types);
int count_ents(int types);
entity * find_ent_here(entity *exclude, float x, float z, int types);
int set_idle(entity* ent);
int set_death(entity *iDie, int type, int reset);
int set_fall(entity *iFall, int type, int reset, entity* other, int force, int drop, int noblock, int guardcost, int jugglecost, int pauseadd);
int set_rise(entity *iRise, int type, int reset);
int set_riseattack(entity *iRiseattack, int type, int reset);
int set_blockpain(entity *iBlkpain, int type, int reset);
int set_pain(entity *iPain, int type, int reset);
void set_weapon(entity* ent, int wpnum, int anim_flag);
entity* melee_find_target();
entity* long_find_target();
entity* normal_find_target(int anim, int iDetect);
entity* normal_find_item();
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
void common_vault();
void common_get();
void common_land();
void common_grab(void);
void common_grabattack();
void common_grabbed();
void common_block(void);
int arrow_takedamage(entity *other, s_attack* attack);
int common_takedamage(entity *other, s_attack* attack);
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
entity* common_find_target();
int common_attack(void);
int common_try_jump(void);
int common_try_pick(entity* other);
int common_try_chase(entity* target, int dox, int doz);
int common_try_follow(entity* target, int dox, int doz);
int common_try_avoid(entity* target, int dox, int doz);
int common_try_wandercompletely(int dox, int doz);
int common_try_wander(entity* target, int dox, int doz);
void common_pickupitem(entity* other);
int common_walk_anim(entity* ent);
int common_idle_anim(entity* ent);
int common_up_anim(entity* ent);
int common_down_anim(entity* ent);
int common_backwalk_anim(entity* ent);
int bomb_move(void);
int arrow_move(void);
int common_move(void);
void common_think(void);
void suicide(void);
void prethrow(void);
void player_die();
int player_trymove(float xdir, float zdir);
int check_energy(int which, int ani);
void player_preinput();
int player_check_special();
void runanimal(void);
void player_blink(void);
int check_combo(int m);
int check_costmove(int s, int fs);
void didfind_item(entity *other);
void player_think(void);
void subtract_shot();
void dropweapon(int flag);
void drop_all_enemies();
void kill_all_enemies();
void smart_bomb(entity* e, s_attack* attack);
void anything_walk(void);
entity * knife_spawn(char *name, int index, float x, float z, float a, int direction, int type, int map);
entity * bomb_spawn(char *name, int index, float x, float z, float a, int direction, int map);
void bomb_explode(void);
int star_spawn(float x, float z, float a, int direction);
void steam_think(void);
void trap_think(void);
void steam_spawn(float x, float z, float a);
void steamer_think(void);
void text_think(void);
entity * homing_find_target(int type);
void biker_drive(void);
void bike_crash(void);
void obstacle_fall(void);
void obstacle_fly(void);
entity * smartspawn(s_spawn_entry * props);
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
#ifndef PSP
void display_credits(void);
#endif
void shutdown(int status, char *msg, ...);
#ifndef PSP
void guistartup(void);
#endif
void startup(void);
int playgif(char *filename, int x, int y, int noskip);
void playscene(char *filename);
void gameover();
void hallfame(int addtoscore);
void showcomplete(int num);
int playlevel(char *filename);
int selectplayer(int *players, char* filename);
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
void openborMain(int argc, char** argv);
int getValidInt(char* text, char* file, char* cmd);
float getValidFloat(char* text, char* file, char* cmd);


s_savelevel savelevel[MAX_DIFFICULTIES];
s_savescore savescore;
s_savedata savedata;

#endif
