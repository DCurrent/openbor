/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * 
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) OpenBOR Team
 */

/////////////////////////////////////////////////////////////////////////////
//	Beats of Rage                                                           //
//	Side-scrolling beat-'em-up                                              //
/////////////////////////////////////////////////////////////////////////////

#include "openbor.h"
#include "commands.h"
#include "models.h"
#include "translation.h"
#include "soundmix.h"
#include "source/bitmask.h" // Inline bitmask utility functions.
#include <inttypes.h>
#include <stdbool.h>
#include <ctype.h>
#include <limits.h>

#define NaN 0xAAAAAAAA

char E_OUT_OF_MEMORY[] = "Error: Could not allocate sufficient memory.\n";
static int DEFAULT_OFFSCREEN_KILL = 3000;


s_sprite_list *sprite_list;
s_sprite_map *sprite_map;

s_savelevel *savelevel;
static char **savelevel_allowselect_args;
static size_t savelevel_count;
s_savescore savescore;
s_savedata savedata;

static void clear_saved_allowselect_arguments(void);
static const char* get_saved_allowselect_arguments(size_t index);
static void set_saved_allowselect_arguments(size_t index, const char* source);

/////////////////////////////////////////////////////////////////////////////
//  Global Variables                                                        //
/////////////////////////////////////////////////////////////////////////////

a_playrecstatus *playrecstatus = NULL;
s_anim_list *anim_list = NULL;
s_modelcache *model_cache = NULL;

s_set_entry *levelsets = NULL;
int        num_difficulties;

int no_cmd_compatible = 0;

int		skiptoset = -1;
//when there are more entities than this, those with lower priority will be erased
int spawnoverride = 999999;
int maxentities = 999999;

int	global_model = MODEL_INDEX_NONE;
#define global_model_scripts ((global_model>=0 && model_cache[global_model].model)?model_cache[global_model].model->scripts:NULL)

s_level            *level               = NULL;
s_filestream *filestreams = NULL;
int numfilestreams = 0;
s_screen           *vscreen             = NULL;
s_screen           *background          = NULL;
s_videomodes        videomodes;
int sprite_map_max_items = 0;
int cache_map_max_items = 0;

int startup_done = 0; // startup is only called when a game is loaded. so when exitting from the menu we need a way to figure out which resources to free.
List *modelcmdlist = NULL;
List *modelstxtcmdlist = NULL;
List *levelcmdlist = NULL;
List *levelordercmdlist = NULL;

/*
* Temporary attack choices assembled by AI routines.
* Capacity follows the configured animation table instead
* of assuming the compile-time MAX_ANIS default is enough.
*/
static int* ai_attack_choices = NULL;
static size_t ai_attack_choice_capacity = 0;

//see types.h
const s_drawmethod plainmethod =
{
    .object_type = OBJECT_TYPE_DRAWMETHOD,
    .table      = NULL,
    .fillcolor  = 0,
    .config     = DRAWMETHOD_CONFIG_ENABLED,
    .alpha      = BLEND_MODE_MODEL,
    .remap      = -1,
    .rotate     = 0,
    .scalex     = 256,
    .scaley     = 256,
    .shiftx     = 0,
    .centerx    = 0,
    .centery    = 0,
    .xrepeat    = 1,
    .yrepeat    = 1,
    .xspan      = 0,
    .yspan      = 0,
    .channelr   = 255,
    .channelg   = 255,
    .channelb   = 255,
    .tintmode   = 0,
    .tintcolor  = 0,
    .clipx      = 0,
    .clipy      = 0,
    .clipw      = 0,
    .cliph      = 0,
    .water = {{.beginsize = 0.0}, {.endsize = 0.0}, 0, {.wavespeed = 0}, 0}
};

// Caskey, Damon V.
// 2019-12-13
// Need default values for projectile animation 
// settings, and projectiles in general.
const s_projectile projectile_default_config = {
	
	.bomb = MODEL_INDEX_NONE,
	.color_set_adjust = COLORSET_ADJUST_NONE,
	.direction_adjust = DIRECTION_ADJUST_SAME,
	.flash = MODEL_INDEX_NONE,
	.knife = MODEL_INDEX_NONE,
	.offense = PROJECTILE_OFFENSE_SELF,
	.placement = PROJECTILE_PLACEMENT_PARENT,

    /*
    * X position defaults are different for stars
    * vs. other projectiles and we want players to 
    * have 0 as an option, so we start with a silly
    * value here. If creator doesn't change it, then
    * we'll apply real defaults in the projectile 
    * spawn functions.
    */

	.position = {.x = PROJECTILE_LEGACY_COMPATABILITY_POSITION_X, 
					.y = PROJECTILE_DEFAULT_POSITION_Y,
					.z = PROJECTILE_DEFAULT_POSITION_Z},
	.shootframe = FRAME_NONE,
	.throwframe = FRAME_NONE,
	.tossframe = FRAME_NONE,
	.star		= MODEL_INDEX_NONE,
	.star_velocity = {0.f, 
						1.f, 
						2.f},
	.velocity = {.x = PROJECTILE_DEFAULT_SPEED_X,
					.y = PROJECTILE_DEFAULT_SPEED_Y,
					.z = PROJECTILE_DEFAULT_SPEED_Z }
};

const s_defense default_defense =
{
    .block_damage_adjust    = 0,
    .block_damage_max       = MAX_INT,
    .block_damage_min       = MIN_INT,
    .blockpower             = 0,
    .blockthreshold         = 0,
    .blockratio             = DEFENSE_BLOCKRATIO_COMPATABILITY_DEFAULT,
    .blocktype              = BLOCK_TYPE_GLOBAL,
    .death_config_flags     = DEATH_CONFIG_MACRO_DEFAULT,
    .damage_adjust          = 0,
    .damage_max             = MAX_INT,
    .damage_min             = MIN_INT,
    .factor                 = 1.f,
    .knockdown              = 1.f,
    .pain                   = 0
};

const s_offense default_offense =
{
    .damage_adjust  = 0,
    .damage_max     = MAX_INT,
    .damage_min     = MIN_INT,
    .factor         = 1.f    
};

const s_hitbox empty_collision_coords = {   .x      = 0,
                                            .y      = 0,
                                            .width  = 0,
                                            .height = 0,
                                            .z_background     = 0,
                                            .z_foreground     = 0};

const s_body empty_body = { .defense = NULL,
                            .flash = {
                                .object_type = OBJECT_TYPE_FLASH,
                                .layer_adjust = 0,
                                .layer_source = 0,
                                .model_block = MODEL_INDEX_NONE,
                                .model_hit = MODEL_INDEX_NONE,
                                .z_source = 0
                            }
                                
};

/*
* 2026-06-28 - In progress.
*/
const s_space empty_space = {
    .push = { .x = 0.0f, .y = 0.0f, .z = 0.0f }
};

// unknockdown attack
const s_attack emptyattack =
{
    .attack_drop        = 0,
    .attack_force       = 0,
    .attack_type        = ATK_NORMAL,
    .blast              = 0,
    .blocksound         = SAMPLE_ID_NONE,
    .counterattack      = 0,
    .damage_on_landing.attack_force =  0,
    .damage_on_landing.attack_type = ATK_NONE,
    .dropv              = { .x = 0,
                            .y = 0,
                            .z = 0},
    .flash = {
            .object_type = OBJECT_TYPE_FLASH,
            .layer_adjust = 0,
            .layer_source = 0,
            .model_block = MODEL_INDEX_NONE,
            .model_hit = MODEL_INDEX_NONE,
            .z_source = 0
        },
    .force_direction    = DIRECTION_ADJUST_NONE,
    .forcemap           = MAP_TYPE_NONE,
    .freeze             = 0,
    .freezetime         = 0,
    .grab               = 0,
    .grab_distance      = 0,
    .guardcost          = 0,
    .hitsound           = SAMPLE_ID_NONE,
    .jugglecost         = 0,
    .maptime            = 0,
    .no_block           = 0,
    .no_flash           = 0,
    .no_kill            = 0,
    .no_pain            = 0,
    .otg                = OTG_NONE,
    .next_hit_time      = 0,
    .pause_add          = 0,
    .recursive          = NULL,
    .seal               = 0,
    .sealtime           = 0,
    .staydown           = { .rise               = 0,
                            .riseattack         = 0,
                            .riseattack_stall   = 0},
    .steal              = 0,
    .meta_data          = NULL,
    .meta_tag           = 0
};

// Default values for knockdown velocity.
s_axis_principal_float default_model_dropv =
{
    .x = 1.2f,
    .y = 3.f,
    .z = 0.f
};

//default values
float default_level_maxtossspeed = 100.0f;
float default_level_maxfallspeed = -6.0f;
float default_level_gravity = -0.1f;

float default_model_jumpheight = 4.0f;
float default_model_jumpspeed = -1;
float default_model_grabdistance = 36.0f;

// AI attack debug stuff for development purpose,
// Don't open them to modders yet
float move_noatk_factor = 3.0f;
float group_noatk_factor = 0.01f;
float agg_noatk_factor = 0.0f;
float min_noatk_chance = 0.0f;
float max_noatk_chance = 0.6f;
float offscreen_noatk_factor = 0.5f;
float noatk_duration = 0.75f;

char                *custScenes = NULL;
char                *custBkgrds = NULL;
char                *custLevels = NULL;
char                *custModels = NULL;
char                rush_names[2][MAX_NAME_LEN];
char				skipselect[MAX_PLAYERS][MAX_NAME_LEN];
char                branch_name[MAX_NAME_LEN + 1];  // Used for branches
char                *allowselect_args = NULL; // stored allowselect players
int					useSave = 0;
int					useSet = -1;
unsigned char       pal[MAX_PAL_SIZE] = {""};
int                 blendfx[MAX_BLENDINGS] = {0, 1, 0, 0, 0, 0};
char                blendfx_is_set = 0;
int                 fontmonospace[MAX_FONTS] = {0, 0, 0, 0, 0, 0, 0, 0};
int                 fontmbs[MAX_FONTS] = {0, 0, 0, 0, 0, 0, 0, 0};

// function pointers to create the blending tables
blend_table_function blending_table_functions32[MAX_BLENDINGS] = {create_screen32_tbl, create_multiply32_tbl, create_overlay32_tbl, create_hardlight32_tbl, create_dodge32_tbl, create_half32_tbl};

int                 current_set = 0;
int                 current_level = 0;
int                 current_stage = 1;

int					timevar;
float               bgtravelled;
float               vbgtravelled;
int                 traveltime;
int                 texttime;
int					timetoshow;
int                 is_total_timeover = 0;
float               advancex;
float               advancey;

float               scrolldx;                       // advancex changed previous loop
float               scrolldy;                       // advancey .....................
float               scrollminz;                     // Limit level z-scroll
float               scrollmaxz;
float               blockade;                    // Limit x scroll back
float				scrollminx;
float				scrollmaxx;

s_lasthit           lasthit;  //Last collision variables. 2013-12-15, moved to struct.

uint64_t			combodelay = GAME_SPEED_DEFAULT / 2;		// avoid annoying 112112... infinite combo

//Use for gfx_shadow
s_axis_plane_vertical_int light = {   .x = 128,
                        .y = 64};

int                 shadowcolor = 0;
int                 shadowalpha = BLEND_MULTIPLY + 1;
int                 shadowopacity = 255;

u64 totalram = 0;
u64 usedram = 0;
u64 freeram = 0;
u32 interval = 0;
//extern u64 seed;

/*
* Hard coded sound sample IDs.
*/
s_global_sample global_sample_list = {
    .beat = SAMPLE_ID_NONE,
    .beep = SAMPLE_ID_NONE,
    .beep_2 = SAMPLE_ID_NONE,
    .bike = SAMPLE_ID_NONE,
    .block = SAMPLE_ID_NONE,
    .fall = SAMPLE_ID_NONE,       
    .get = SAMPLE_ID_NONE,
    .get_2 = SAMPLE_ID_NONE,
    .go = SAMPLE_ID_NONE, 
    .indirect = SAMPLE_ID_NONE,
    .jump = SAMPLE_ID_NONE,
    .one_up = SAMPLE_ID_NONE,
    .pause = SAMPLE_ID_NONE,    
    .punch = SAMPLE_ID_NONE,    
    .time_over = SAMPLE_ID_NONE    
};

// 2016-11-01
// Caskey, Damon V.
//
// Collision indexes. Only using globals while
// building multiple collision box support.
// Once we get this working, variables should
// be moved into a structure. Globals BAD!
int                 max_collisons       = MAX_COLLISIONS;
int                 *collisions         = NULL;


int                 max_downs           = MAX_DOWNS;
int                 max_ups             = MAX_UPS;
int                 max_backwalks       = MAX_BACKWALKS;
int                 max_walks           = MAX_WALKS;
int                 max_idles           = MAX_IDLES;
int                 max_attack_types    = MAX_ATKS;
int                 max_freespecials    = MAX_SPECIALS;
int                 max_follows         = MAX_FOLLOWS;
int                 max_attacks         = MAX_ATTACKS;
int                 max_animations      = MAX_ANIS;

// -------dynamic animation indexes-------
animation_id_t  *animdowns           = NULL;
animation_id_t  *animups             = NULL;
animation_id_t  *animbackwalks       = NULL;
animation_id_t  *animwalks           = NULL;
animation_id_t  *animidles           = NULL;
animation_id_t  *animpains           = NULL;
animation_id_t  *animbackpains       = NULL;
animation_id_t  *animdies            = NULL;
animation_id_t  *animbackdies        = NULL;
animation_id_t  *animfalls           = NULL;
animation_id_t  *animbackfalls       = NULL;
animation_id_t  *animrises           = NULL;
animation_id_t  *animbackrises       = NULL;
animation_id_t  *animriseattacks     = NULL;
animation_id_t  *animbackriseattacks = NULL;
animation_id_t  *animblkpains        = NULL;
animation_id_t  *animbackblkpains    = NULL;
animation_id_t  *animattacks         = NULL;
animation_id_t  *animfollows         = NULL;
animation_id_t  *animspecials        = NULL;

// system default values
animation_id_t      downs[MAX_DOWNS]         = {ANI_DOWN};
animation_id_t      ups[MAX_UPS]             = {ANI_UP};
animation_id_t      backwalks[MAX_BACKWALKS] = {ANI_BACKWALK};
animation_id_t      walks[MAX_WALKS]         = {ANI_WALK};
animation_id_t      idles[MAX_IDLES]         = {ANI_IDLE};

animation_id_t      falls[MAX_ATKS] =
{
    ANI_FALL,       // ATK_NORMAL
    ANI_FALL2,      // ATK_NORMAL2
    ANI_FALL3,      // ATK_NORMAL3
    ANI_FALL4,      // ATK_NORMAL4
    ANI_FALL,       // ATK_BLAST
    ANI_BURN,       // ATK_BURN
    ANI_FALL,       // ATK_FREEZE 
    ANI_SHOCK,      // ATK_SHOCK
    ANI_FALL,       // ATK_STEAL 
    ANI_FALL5,      // ATK_NORMAL5
    ANI_FALL6,      // ATK_NORMAL6
    ANI_FALL7,      // ATK_NORMAL7
    ANI_FALL8,      // ATK_NORMAL8
    ANI_FALL9,      // ATK_NORMAL9
    ANI_FALL10,     // ATK_NORMAL10
    ANI_FALL,       // ATK_BOSS DEATH
    ANI_FALL,       // ATK_ITEM
    ANI_FALL,       // ATK_LAND
    ANI_FALL,       // ATK_LIFESPAN
    ANI_FALLLOSE,   // ATK_LOSE
    ANI_FALL,       // ATK_PIT
    ANI_FALL,       // ATK_SUB_ENTITY_PARENT_KILL
    ANI_FALL,       // ATK_SUB_ENTITY_UNSUMMON
    ANI_FALL        // ATK_TIMEOVER
};

animation_id_t      backfalls[MAX_ATKS] =
{
    ANI_BACKFALL,  ANI_BACKFALL2, ANI_BACKFALL3,  ANI_BACKFALL4,
    ANI_BACKFALL,  ANI_BACKBURN,  ANI_BACKFALL,   ANI_BACKSHOCK,
    ANI_BACKFALL,  ANI_BACKFALL5, ANI_BACKFALL6,  ANI_BACKFALL7,
    ANI_BACKFALL8, ANI_BACKFALL9, ANI_BACKFALL10, ANI_BACKFALL,
    ANI_BACKFALL,  ANI_BACKFALL,  ANI_BACKFALL,   ANI_FALLLOSE,
    ANI_BACKFALL,  ANI_BACKFALL,  ANI_BACKFALL,   ANI_BACKFALL
};

animation_id_t      rises[MAX_ATKS] =
{
    ANI_RISE,  ANI_RISE2, ANI_RISE3,  ANI_RISE4,
    ANI_RISE,  ANI_RISEB,  ANI_RISE,  ANI_RISES,
    ANI_RISE,  ANI_RISE5, ANI_RISE6,  ANI_RISE7,
    ANI_RISE8, ANI_RISE9, ANI_RISE10, ANI_RISE,
    ANI_RISE,  ANI_RISE,  ANI_RISE,   ANI_RISE,
    ANI_RISE,  ANI_RISE,  ANI_RISE,   ANI_RISE
};

animation_id_t      backrises[MAX_ATKS] =
{
    ANI_BACKRISE,  ANI_BACKRISE2, ANI_BACKRISE3,  ANI_BACKRISE4,
    ANI_BACKRISE,  ANI_BACKRISEB, ANI_BACKRISE,   ANI_BACKRISES,
    ANI_BACKRISE,  ANI_BACKRISE5, ANI_BACKRISE6,  ANI_BACKRISE7,
    ANI_BACKRISE8, ANI_BACKRISE9, ANI_BACKRISE10, ANI_BACKRISE,
    ANI_BACKRISE,  ANI_BACKRISE,  ANI_BACKRISE,   ANI_BACKRISE,
    ANI_BACKRISE,  ANI_BACKRISE,  ANI_BACKRISE,   ANI_BACKRISE
};

animation_id_t      riseattacks[MAX_ATKS] =
{
    ANI_RISEATTACK,  ANI_RISEATTACK2, ANI_RISEATTACK3,  ANI_RISEATTACK4,
    ANI_RISEATTACK,  ANI_RISEATTACKB, ANI_RISEATTACK,   ANI_RISEATTACKS,
    ANI_RISEATTACK,  ANI_RISEATTACK5, ANI_RISEATTACK6,  ANI_RISEATTACK7,
    ANI_RISEATTACK8, ANI_RISEATTACK9, ANI_RISEATTACK10, ANI_RISEATTACK,
    ANI_RISEATTACK,  ANI_RISEATTACK,  ANI_RISEATTACK,   ANI_RISEATTACK,
    ANI_RISEATTACK,  ANI_RISEATTACK,  ANI_RISEATTACK,   ANI_RISEATTACK
};

animation_id_t      backriseattacks[MAX_ATKS] =
{
    ANI_BACKRISEATTACK,  ANI_BACKRISEATTACK2, ANI_BACKRISEATTACK3,  ANI_BACKRISEATTACK4,
    ANI_BACKRISEATTACK,  ANI_BACKRISEATTACKB, ANI_BACKRISEATTACK,   ANI_BACKRISEATTACKS,
    ANI_BACKRISEATTACK,  ANI_BACKRISEATTACK5, ANI_BACKRISEATTACK6,  ANI_BACKRISEATTACK7,
    ANI_BACKRISEATTACK8, ANI_BACKRISEATTACK9, ANI_BACKRISEATTACK10, ANI_BACKRISEATTACK,
    ANI_BACKRISEATTACK,  ANI_BACKRISEATTACK,  ANI_BACKRISEATTACK,   ANI_BACKRISEATTACK,
    ANI_BACKRISEATTACK,  ANI_BACKRISEATTACK,  ANI_BACKRISEATTACK,   ANI_BACKRISEATTACK
};

animation_id_t      pains[MAX_ATKS] =
{
    ANI_PAIN,  ANI_PAIN2,    ANI_PAIN3,  ANI_PAIN4,
    ANI_PAIN,  ANI_BURNPAIN, ANI_PAIN,   ANI_SHOCKPAIN,
    ANI_PAIN,  ANI_PAIN5,    ANI_PAIN6,  ANI_PAIN7,
    ANI_PAIN8, ANI_PAIN9,    ANI_PAIN10, ANI_PAIN,
    ANI_PAIN,  ANI_PAIN,     ANI_PAIN,   ANI_PAIN,
    ANI_PAIN,  ANI_PAIN,     ANI_PAIN,   ANI_PAIN
};

animation_id_t      backpains[MAX_ATKS] =
{
    ANI_BACKPAIN,  ANI_BACKPAIN2,    ANI_BACKPAIN3,  ANI_BACKPAIN4,
    ANI_BACKPAIN,  ANI_BACKBURNPAIN, ANI_BACKPAIN,   ANI_BACKSHOCKPAIN,
    ANI_BACKPAIN,  ANI_BACKPAIN5,    ANI_BACKPAIN6,  ANI_BACKPAIN7,
    ANI_BACKPAIN8, ANI_BACKPAIN9,    ANI_BACKPAIN10, ANI_BACKPAIN,
    ANI_BACKPAIN,  ANI_BACKPAIN,     ANI_BACKPAIN,   ANI_BACKPAIN,
    ANI_BACKPAIN,  ANI_BACKPAIN,     ANI_BACKPAIN,   ANI_BACKPAIN
};

animation_id_t      deaths[MAX_ATKS] =
{
    ANI_DIE,   ANI_DIE2,     ANI_DIE3,  ANI_DIE4,
    ANI_DIE,   ANI_BURNDIE,  ANI_DIE,   ANI_SHOCKDIE,
    ANI_DIE,   ANI_DIE5,     ANI_DIE6,  ANI_DIE7,
    ANI_DIE8,  ANI_DIE9,     ANI_DIE10, ANI_DIE,
    ANI_DIE,   ANI_DIE,      ANI_DIE,   ANI_LOSE,
    ANI_DIE,   ANI_DIE,      ANI_DIE,   ANI_DIE
};

animation_id_t      backdeaths[MAX_ATKS] =
{
    ANI_BACKDIE,   ANI_BACKDIE2,     ANI_BACKDIE3,  ANI_BACKDIE4,
    ANI_BACKDIE,   ANI_BACKBURNDIE,  ANI_BACKDIE,   ANI_BACKSHOCKDIE,
    ANI_BACKDIE,   ANI_BACKDIE5,     ANI_BACKDIE6,  ANI_BACKDIE7,
    ANI_BACKDIE8,  ANI_BACKDIE9,     ANI_BACKDIE10, ANI_BACKDIE,
    ANI_BACKDIE,   ANI_BACKDIE,      ANI_BACKDIE,   ANI_LOSE,
    ANI_BACKDIE,   ANI_BACKDIE,      ANI_BACKDIE,   ANI_BACKDIE
};

animation_id_t      blkpains[MAX_ATKS] =
{
    ANI_BLOCKPAIN,  ANI_BLOCKPAIN2, ANI_BLOCKPAIN3,  ANI_BLOCKPAIN4,
    ANI_BLOCKPAIN,  ANI_BLOCKPAINB, ANI_BLOCKPAIN,   ANI_BLOCKPAINS,
    ANI_BLOCKPAIN,  ANI_BLOCKPAIN5, ANI_BLOCKPAIN6,  ANI_BLOCKPAIN7,
    ANI_BLOCKPAIN8, ANI_BLOCKPAIN9, ANI_BLOCKPAIN10, ANI_BLOCKPAIN,
    ANI_BLOCKPAIN,  ANI_BLOCKPAIN,  ANI_BLOCKPAIN,   ANI_BLOCKPAIN,
    ANI_BLOCKPAIN,  ANI_BLOCKPAIN,  ANI_BLOCKPAIN,   ANI_BLOCKPAIN
};

animation_id_t      backblkpains[MAX_ATKS] =
{
    ANI_BACKBLOCKPAIN,  ANI_BACKBLOCKPAIN2, ANI_BACKBLOCKPAIN3,  ANI_BACKBLOCKPAIN4,
    ANI_BACKBLOCKPAIN,  ANI_BACKBLOCKPAINB, ANI_BACKBLOCKPAIN,   ANI_BACKBLOCKPAINS,
    ANI_BACKBLOCKPAIN,  ANI_BACKBLOCKPAIN5, ANI_BACKBLOCKPAIN6,  ANI_BACKBLOCKPAIN7,
    ANI_BACKBLOCKPAIN8, ANI_BACKBLOCKPAIN9, ANI_BACKBLOCKPAIN10, ANI_BACKBLOCKPAIN,
    ANI_BACKBLOCKPAIN,  ANI_BACKBLOCKPAIN,  ANI_BACKBLOCKPAIN,   ANI_BACKBLOCKPAIN,
    ANI_BACKBLOCKPAIN,  ANI_BACKBLOCKPAIN,  ANI_BACKBLOCKPAIN,   ANI_BACKBLOCKPAIN
};

animation_id_t      normal_attacks[MAX_ATTACKS] =
{
    ANI_ATTACK1, ANI_ATTACK2, ANI_ATTACK3, ANI_ATTACK4
};

int                 grab_attacks[GRAB_ACTION_SELECT_MAX][2] =
{
    [GRAB_ACTION_SELECT_ATTACK] = {ANI_GRABATTACK, ANI_GRABATTACK2},
	[GRAB_ACTION_SELECT_BACKWARD] = {ANI_GRABBACKWARD, ANI_GRABBACKWARD2},
	[GRAB_ACTION_SELECT_FORWARD] = {ANI_GRABFORWARD, ANI_GRABFORWARD2},
    [GRAB_ACTION_SELECT_DOWN] = {ANI_GRABDOWN, ANI_GRABDOWN2},
	[GRAB_ACTION_SELECT_UP] = {ANI_GRABUP, ANI_GRABUP2}
};

animation_id_t      freespecials[MAX_SPECIALS] =
{
    ANI_FREESPECIAL,   ANI_FREESPECIAL2,  ANI_FREESPECIAL3,
    ANI_FREESPECIAL4,  ANI_FREESPECIAL5,  ANI_FREESPECIAL6,
    ANI_FREESPECIAL7,  ANI_FREESPECIAL8
};

animation_id_t      follows[MAX_FOLLOWS] =
{
    ANI_FOLLOW1, ANI_FOLLOW2, ANI_FOLLOW3, ANI_FOLLOW4
};

// background cache to speed up in-game menus
#ifdef CACHE_BACKGROUNDS
s_screen           *bg_cache[MAX_CACHED_BACKGROUNDS] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
unsigned char		bg_palette_cache[MAX_CACHED_BACKGROUNDS][MAX_PAL_SIZE];
#endif

s_debug_xy_msg      debug_xy_msg;
int                 cameratype          = 0;
int					defaultmaxplayers	= 2;

uint64_t            go_time             = 0;
uint64_t            _time               = 0;
uint64_t            newtime             = 0;
s_slow_motion       slowmotion          = { .toggle     = SLOW_MOTION_OFF,
                                            .counter    = 0,
                                            .duration   = 2};
int                 disablelog          = 0;
int                 currentspawnplayer  = 0;
int					ent_list_size		= 0;
int                 PLAYER_MIN_Z        = 160;
int                 PLAYER_MAX_Z        = 232;
int                 BGHEIGHT            = 160;
int                 MAX_WALL_HEIGHT     = 1000;					// Max wall height that an entity can be spawned on
int                 saveslot            = 0;
int                 current_palette     = 0;
int                 fade                = 24;
int                 credits             = 0;
int                 gosound             = 0;					// Used to prevent go sound playing too frequently,
int                 musicoverlap        = 0;
int                 colorbars           = 0;
int                 current_spawn       = 0;
int                 level_completed     = 0;
int                 level_completed_defeating_boss     = 0;
int                 nojoin              = 0;					// dont allow new hero to join in, use "Please Wait" instead of "Select Hero"
int                 groupmin            = 0;
int					groupmax            = 0;
e_screen_status     screen_status       = IN_SCREEN_NONE;       // Caskey, Damon V. (2022-04-21) - Current screen status. Replaces the previous 16+ "inscreen" flag variables.
char				*currentScene		= NULL;
int                 tospeedup           = 0;          			// If set will speed the level back up after a boss hits the ground
bool                reached[MAX_PLAYERS]          = {false, false, false, false};			// Used with TYPE_ENDLEVEL to determine which players have reached the point //4player
int                 noslowfx			= 0;           			// Flag to determine if sound speed when hitting opponent slows or not
int                 equalairpause 		= 0;         			// If set to 1, there will be no extra pausetime for players who hit multiple enemies in midair
int                 hiscorebg			= 0;					// If set to 1, will look for a background image to display at the highscore screen
int                 completebg			= 0;           			// If set to 1, will look for a background image to display at the showcomplete screen
s_loadingbar        loadingbg[2] = {{0, 0, {0, 0}, {0, 0}, 0, 0}, {0, 0, {0, 0}, {0, 0}, 0, 0}}; // If set to 1, will look for a background image to display at the loading screen
int					loadingmusic        = 0;
int                 unlockbg            = 0;         			// If set to 1, will look for a different background image after defeating the game
int                 _pause              = 0;
int                 goto_mainmenu_flag  = 0;
int                 escape_flag         = 0;                    // Kratus (20-04-21) Added the new "escape" flag in the select screen, has the same effect as the esc key but now accessible by the "gotomainmenu" function
int					nofadeout			= 0;
int					nosave				= 0;
int                 nopause             = 0;                    // OX. If set to 1 , pausing the game will be disabled.
int                 noscreenshot        = 0;                    // OX. If set to 1 , taking screenshots is disabled.
int                 endgame             = 0;
int                 nodebugoptions      = 0;

int                 keyscriptrate       = 0;
int                 showtimeover        = 0;
int                 sameplayer          = 0;            		// 7-1-2005  flag to determine if players can use the same character
int                 PLAYER_LIVES        = 3;					// 7-1-2005  default setting for Lives
int                 CONTINUES           = 5;					// 7-1-2005  default setting for continues
int                 colourselect		= 0;					// 6-2-2005 Colour select is optional
int                 autoland			= 0;					// Default set to no autoland and landing is valid with u j combo
int                 nolost				= 0;					// variable to control if drop weapon when grab a enemy by tails
int                 nocost				= 0;					// If set, special will not cost life unless an enemy is hit
int                 mpstrict			= 0;					// If current system will check all animation's energy cost when set new animations
int                 magic_type			= 0;					// use for restore mp by time by tails
entity             *textbox				= NULL;
entity             *smartbomber			= NULL;
entity				*stalker			= NULL;					// an enemy (usually) tries to go behind the player
entity				*firstplayer		= NULL;
int					stalking			= 0;
int					nextplan			= 0;
int                 plife[MAX_PLAYERS][2]         = {{0, 0}, {0, 0}, {0, 0}, {0, 0}}; // Used for customizable player lifebar
int                 plifeX[MAX_PLAYERS][3]        = {{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}}; // Used for customizable player lifebar 'x'
int                 plifeN[MAX_PLAYERS][3]        = {{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}}; // Used for customizable player lifebar number of lives
int                 picon[MAX_PLAYERS][2]         = {{0, 0}, {0, 0}, {0, 0}, {0, 0}}; // Used for customizable player icon
int                 piconw[MAX_PLAYERS][2]        = {{0, 0}, {0, 0}, {0, 0}, {0, 0}}; // Used for customizable player weapon icons
int                 mpicon[MAX_PLAYERS][2]        = {{0, 0}, {0, 0}, {0, 0}, {0, 0}}; // Used for customizable magicbar player icon
int                 pnameJ[MAX_PLAYERS][7]        = {{0, 0, 0, 0, 0, 0, -1}, {0, 0, 0, 0, 0, 0, -1}, {0, 0, 0, 0, 0, 0, -1}, {0, 0, 0, 0, 0, 0, -1}}; // Used for customizable player name, Select Hero, (Credits, Press Start, Game Over) when joining
int                 pscore[MAX_PLAYERS][7]        = {{0, 0, 0, 0, 0, 0, -1}, {0, 0, 0, 0, 0, 0, -1}, {0, 0, 0, 0, 0, 0, -1}, {0, 0, 0, 0, 0, 0, -1}}; // Used for customizable player name, dash, score
int                 pshoot[MAX_PLAYERS][3]        = {{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}}; // Used for customizable player shootnum
int                 prush[MAX_PLAYERS][8]         = {{0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0}}; // Used for customizable player combo/rush system
int                 psmenu[MAX_PLAYERS][4]        = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}; // Used for customizable player placement in select menu
int                 mpcolourtable[11]   = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int                 hpcolourtable[11]   = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int                 ldcolourtable[11]   = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
char                musicname[MAX_BUFFER_LEN]      = {""};
char                currentmusic[MAX_BUFFER_LEN]    = {""};
float               musicfade[2]        = {0, 0};
int                 musicloop           = 0;
u32                 musicoffset         = 0;
int					alwaysupdate		= 0; //execute update/updated scripts whenever it has a chance

s_global_config global_config =
{
    .object_type = OBJECT_TYPE_GLOBAL_CONFIG,
    .ajspecial = AJSPECIAL_KEY_SPECIAL,
    .block_type = BLOCK_TYPE_GLOBAL,
    .cheats = CHEAT_OPTIONS_ALL_MENU,
    .flash = {
        .object_type = OBJECT_TYPE_FLASH,
        .layer_adjust = 1,
        .layer_source = 255,
        .z_source = 0},
    .delay_unit = DELAY_UNIT_CENTISECOND,
    .showgo = 0,
    .game_speed = GAME_SPEED_DEFAULT,
    .counter_speed = COUNTER_SPEED_DEFAULT,
    .grab_stall = GRAB_STALL_DEFAULT,
    .command_time = COMMAND_TIME_DEFAULT
};

s_barstatus loadingbarstatus =
{
    .size           = { .x = 0,
                        .y = 10},
    .graph_position    = { .x = 0,
                        .y = 0},
    .name_position = {  .x = 0,
                        .y = 0},
    .config_flags   = STATUS_CONFIG_GRAPH_RATIO,
    .barlayer       = 0,
    .backlayer      = 0,
    .borderlayer    = 0,
    .shadowlayer    = 0,
    .colourtable    = &ldcolourtable
};

s_barstatus lbarstatus, olbarstatus =
{
    .size           = { .x = 0,
                        .y = 0},
    .graph_position    = { .x = 0,
                        .y = 0},
    .name_position  = { .x = 0,
                        .y = 0},
    .config_flags   = STATUS_CONFIG_DEFAULT,
    .barlayer       = 0,
    .backlayer      = 0,
    .borderlayer    = 0,
    .shadowlayer    = 0,
    .colourtable    = &hpcolourtable
};

s_barstatus mpbarstatus =
{
    .size           = { .x = 0,
                        .y = 0},
    .graph_position    = { .x = 0,
                        .y = 0},
    .name_position  = { .x = 0,
                        .y = 0},
    .config_flags   = STATUS_CONFIG_DEFAULT,
    .barlayer       = 0,
    .backlayer      = 0,
    .borderlayer    = 0,
    .shadowlayer    = 0,
    .colourtable    = &mpcolourtable
};

int                 timeloc[6]			= {0, 0, 0, 0, 0, -1};		// Used for customizable timeclock location/size
int                 timeicon			= -1;
int                 timeicon_offsets[2] = {0, 0};
char                timeicon_path[MAX_BUFFER_LEN]  = {""};
int                 bgicon   			= -1;
int                 bgicon_offsets[3]	= {0, 0, 0};
char                bgicon_path[MAX_BUFFER_LEN]    = {""};
int                 olicon    			= -1;
int                 olicon_offsets[3]	= {0, 0, 0};
char                olicon_path[MAX_BUFFER_LEN]    = {""};
int                 elife[4][2]         = {{0, 0}, {0, 0}, {0, 0}, {0, 0}}; // Used for customizable enemy lifebar
int                 ename[4][3]         = {{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}}; // Used for customizable enemy name
int                 eicon[4][2]         = {{0, 0}, {0, 0}, {0, 0}, {0, 0}}; // Used for customizable enemy icon
int                 scomplete[6]		= {0, 0, 0, 0, 0, 0};		// Used for customizable Stage # Complete
int                 cbonus[10]          = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // Used for customizable clear bonus
int                 lbonus[10]          = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // Used for customizable life bonus
int                 rbonus[10]          = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // Used for customizable rush bonus
int                 tscore[10]          = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // Used for customizable total score
int                 scbonuses[4]        = {10000, 1000, 100, 0};//Stage complete bonus multipliers
int                 showrushbonus       = 0;
int                 noshare				= 0;					// Used for when you want to keep p1 & p2 credits separate
int                 nodropen			= 0;					// Drop or not when spawning is now a modder option
int					nodropspawn			= 0;					// don't spawn from the sky if the modder doesn't set it
int                 gfx_x_offset		= 0;                    //2011_04_03, DC: Enable X offset adjustment by modders.
int                 gfx_y_offset		= 0;
int                 gfx_y_offset_adj    = 0;                    //2011_04_03, DC: Enable Y offset adjustment by modders.

// 2011/10/22 UT: temporary solution for custom viewport
int					viewportx			= 0;
int					viewporty			= 0;
int					viewportw			= 0;
int					viewporth			= 0;


uint64_t            timeleft			= 0;                    // Time left in active level in game logic counts.
int                 oldtime             = 0;                    // One second back from time left.
int                 holez				= 0;					// Used for setting spawn points
int                 allow_secret_chars	= 0;
uint64_t        lifescore			= 50000;				// Number of points needed to earn a 1-up
uint64_t        credscore			= 0;					// Number of points needed to earn a credit
int                 nochipdeath			= 0;					// Prevents entities from dying due to chip damage (damage while blocking)
int                 noaircancel			= 0;					// Now, you can make jumping attacks uncancellable!
int                 nomaxrushreset[5]	= {0, 0, 0, 0, 0};
int			        mpbartext[4]		= { -1, 0, 0, 0};			// Array for adjusting MP status text (font, Xpos, Ypos, Display type).
int			        lbartext[4]			= { -1, 0, 0, 0};			// Array for adjusting HP status text (font, Xpos, Ypos, Display type).
int                 pmp[4][2]			= {{0, 0}, {0, 0}, {0, 0}, {0, 0}}; // Used for customizable player mpbar
int                 spdirection[4]		= {DIRECTION_RIGHT, DIRECTION_LEFT, DIRECTION_RIGHT, DIRECTION_LEFT}; // Used for Select Player Direction for select player screen
int                 bonus				= 0;					// Used for unlocking Bonus difficulties
int                 versusdamage		= 2;					// Used for setting mode. (ability to hit other players)
int                 z_coords[3]			= {0, 0, 0};				// Used for setting customizable walkable area
int                 rush[6]				= {0, 2, 3, 3, 3, 3};
int                 pauseoffset[7]  	= {0, 1, 0, 0, 3, 0, 0};		// Used for customizable pause menu location (font0, font1, xpos, ypos, font_pause, xpos_pause, ypos_pause)
int                 color_black			= 0;
int                 color_red			= 0;
int                 color_orange		= 0;
int                 color_yellow		= 0;
int                 color_white			= 0;
int                 color_blue			= 0;
int                 color_green			= 0;
int                 color_pink			= 0;
int                 color_purple		= 0;
int                 color_magic			= 0;
int                 color_magic2		= 0;
int                 lifebarfgalpha      = 0;
int                 lifebarbgalpha      = 2;
int                 shadowsprites[6]	= { -1, -1, -1, -1, -1, -1};
int                 gosprite			= -1;
int                 golsprite			= -1;
//int                 holesprite			= -1;
int                 videoMode			= 0;
int                 scoreformat			= 0;					// If set fill score values with 6 Zeros

// Funny neon lights
unsigned char       neontable[MAX_PAL_SIZE];
uint64_t        neon_time			= 0;

int                 panel_width			= 0;
int                 panel_height		= 0;
int                 frontpanels_loaded	= 0;

uint64_t        sprites_loaded		= 0;
uint64_t        anims_loaded		= 0;

uint64_t        models_loaded		= 0;
uint64_t        models_cached		= 0;

entity            **ent_list;
entity            **ent_stack; //temporary list, reference only
int					ent_stack_size = 0;
entity             *self;
int                 ent_count			= 0;					// log count of entites
int                 ent_max				= 0;
static uint64_t     entity_unique_id_counter = ENTITY_UNIQUE_ID_NONE;

s_player            player[MAX_PLAYERS];
key_mask_t  bothkeys;
key_mask_t  bothnewkeys;

s_playercontrols    playercontrols1;
s_playercontrols    playercontrols2;
s_playercontrols    playercontrols3;
s_playercontrols    playercontrols4;
s_playercontrols   *playercontrolpointers[] = {&playercontrols1, &playercontrols2, &playercontrols3, &playercontrols4};
s_playercontrols    default_control;
int default_keys[MAX_BTN_NUM];

//global script
Script level_script;		//execute when level start
Script endlevel_script;		//execute when level finished
Script update_script;		//execute when ingame update
Script updated_script;		//execute when ingame update finished
Script update_logic_script;   //execute before each logical tick
Script updated_logic_script;  //execute after each logical tick
Script model_load_script;     //execute after any model finishes loading
Script model_unload_script;   //execute before any model is unloaded
Script loading_script;		// in loading screen
Script input_script_all;  //keyscript for all players
Script key_script_all;		//keyscript for all players
Script score_script_all;    //score listener for all players
Script join_script_all;     //join listener for all players
Script respawn_script_all;  //respawn listener for all players
Script pdie_script_all;     //death listener for all players
Script timetick_script;		//time tick script.

//player script
Script score_script[MAX_PLAYERS];     //execute when add score, 4 players
Script key_script[MAX_PLAYERS];       //key listeners
Script join_script[MAX_PLAYERS];      //player join scripts
Script respawn_script[MAX_PLAYERS];   //player respawn scripts
Script pdie_script[MAX_PLAYERS];      //player death scripts

extern Script *pcurrentscript;//used by local script functions
//-------------------------methods-------------------------------

void setDrawMethod(s_anim *a, ptrdiff_t index, s_drawmethod *m)
{
    assert(index >= 0);
    assert(a != NULL);
    assert(m != NULL);
    assert(index < a->numframes);
    a->drawmethods[index] = m;
}

s_drawmethod *getDrawMethod(s_anim *a, ptrdiff_t index)
{
    assert(index >= 0);
    assert(a != NULL);
    assert(index < a->numframes);
    return a->drawmethods[index];
}

int isLoadingScreenTypeBg(e_loadingScreenType what)
{
    return (what & LS_TYPE_BACKGROUND) == LS_TYPE_BACKGROUND;
}

int isLoadingScreenTypeBar(e_loadingScreenType what)
{
    return (what & LS_TYPE_BAR) == LS_TYPE_BAR;
}

char *fill_s_loadingbar(s_loadingbar *s, e_loadingScreenType set, int bx, int by, int bsize, int tx, int ty, int tf, int ms)
{
    switch (set)
    {
        case LS_TYPE_BOTH:
            s->set = (LS_TYPE_BACKGROUND | LS_TYPE_BAR);
            break;
        case LS_TYPE_BACKGROUND:
            s->set = LS_TYPE_BACKGROUND;
            break;
        case LS_TYPE_BAR:
            s->set = LS_TYPE_BAR;
            break;
        case LS_TYPE_NONE:
            s->set = LS_TYPE_NONE;
            break;
        default:
            s->set = LS_TYPE_NONE;
            printf("invalid loadingbg type %d!\n", set);
    }
    s->tf = tf;
    s->bar_position.x = bx;
    s->bar_position.y = by;
    s->bsize = bsize;
    s->text_position.x = tx;
    s->text_position.y = ty;
    s->refreshMs = (ms ? ms : 100);
    return NULL;
}

static int buffer_file(const char *filename, char **pbuffer, size_t *psize)
{
    FILE *handle;
    *psize = 0;
    *pbuffer = NULL;
    // Read file
#ifdef VERBOSE
    printf("file requested: %s.\n", filename);
#endif

    if(!(handle = fopen(filename, "rb")) )
    {
#ifdef VERBOSE
        printf("couldnt get handle!\n");
#endif
        return 0;
    }
    fseek(handle, 0, SEEK_END);
    *psize = ftell(handle);
    fseek(handle, 0, SEEK_SET);

    *pbuffer = (char *)malloc(*psize + 1);
    if(*pbuffer == NULL)
    {
        *psize = 0;
        fclose(handle);
        borShutdown(1, "Can't create buffer for file '%s'", filename);
        return 0;
    }
    if(fread(*pbuffer, 1, *psize, handle) != *psize)
    {
        if(*pbuffer != NULL)
        {
            free(*pbuffer);
            *pbuffer = NULL;
            *psize = 0;
        }
        fclose(handle);
        borShutdown(1, "Can't read from file '%s'", filename);
        return 0;
    }
    (*pbuffer)[*psize] = 0;        // Terminate string (important!)
    fclose(handle);
    return 1;
}


// returns: 1 - succeeded 0 - failed
int buffer_pakfile(const char *filename, char **pbuffer, size_t *psize)
{
    int handle;
    *psize = 0;
    *pbuffer = NULL;

    if(buffer_file(filename, pbuffer, psize) == 1)
    {
        return 1;
    }

    // Read file
#ifdef VERBOSE
    printf("pakfile requested: %s.\n", filename); //ASDF
#endif

    if((handle = openpackfile(filename, packfile)) < 0)
    {
#ifdef VERBOSE
        printf("couldnt get handle!\n");
#endif
        return 0;
    }
    *psize = seekpackfile(handle, 0, SEEK_END);
    seekpackfile(handle, 0, SEEK_SET);

    *pbuffer = (char *)malloc(*psize + 1);
    if(*pbuffer == NULL)
    {
        *psize = 0;
        closepackfile(handle);
        borShutdown(1, "Can't create buffer for packfile '%s'", filename);
        return 0;
    }
    if(readpackfile(handle, *pbuffer, *psize) != *psize)
    {
        if(*pbuffer != NULL)
        {
            free(*pbuffer);
            *pbuffer = NULL;
            *psize = 0;
        }
        closepackfile(handle);
        borShutdown(1, "Can't read from packfile '%s'", filename);
        return 0;
    }
    (*pbuffer)[*psize] = 0;        // Terminate string (important!)
    closepackfile(handle);
    return 1;
}

int buffer_append(char **buffer, const char *str, size_t n, size_t *bufferlen, size_t *len)
{
    size_t appendlen = 0;

    while(appendlen < n && str[appendlen])
    {
        appendlen++;
    }

    if(appendlen + *len + 1 > *bufferlen)
    {
        //printf("*Debug* reallocating buffer...\n");
        *buffer = realloc(*buffer, *bufferlen = appendlen + *len + 1024);
        if(*buffer == NULL)
        {
            borShutdown(1, "Unable to resize buffer.\n");
        }
    }
    memcpy(*buffer + *len, str, appendlen);
    *len = *len + appendlen;
    (*buffer)[*len] = 0;
    return *len;
}

/*
- Caskey, Damon V.
- 2026-08-12
-
- Verify and atomically remove two adjoining suffixes
  from a generated text buffer. Leave the buffer intact
  if its tail does not match the complete sequence.
*/
static bool buffer_remove_suffix_pair(
    char* buffer,
    size_t* length,
    const char* first,
    size_t first_length,
    const char* second,
    size_t second_length
) {
    size_t pair_length;

    assert(length);
    assert(first);
    assert(second);

    if(!buffer || first_length > SIZE_MAX - second_length) {
        return false;
    }

    pair_length = first_length + second_length;

    if(*length < pair_length
        || memcmp(
            buffer + *length - second_length,
            second,
            second_length
        )
        || memcmp(
            buffer + *length - pair_length,
            first,
            first_length
        )) {
        return false;
    }

    *length -= pair_length;
    buffer[*length] = '\0';

    return true;
}

int handle_txt_include(char *command, ArgList *arglist, char **fn, char *namebuf, char **buf, ptrdiff_t *pos, size_t *len)
{
    char *incfile, *filename = *fn, *buf2, *endstr = "\r\n@end";
    size_t size, t;
    if(stricmp(command, "@include") == 0)
    {
        incfile = GET_ARGP(1);
        buffer_pakfile(incfile, &buf2, &size) ;
        if(buf2)
        {
            *buf = realloc(*buf, *len + size + strlen(incfile) + strlen(filename) + 100); //leave enough memory for jump command
            if(*buf == NULL)
            {
                borShutdown(1, "Unable to resize buffer. (handle_txt_include)\n");
                free(buf2);
                return 0;
            }
            sprintf((*buf) + *len - 1, "%s\r\n@filename %s\r\n", endstr, incfile);
            strcat((*buf) + *len, buf2);
            t = strlen(*buf);
            sprintf((*buf) + t, "\r\n@filename %s\r\n@jump %d\r\n", filename, (int)(*pos));
            (*buf)[*pos] = '#';
            *pos = *len + strlen(endstr); //continue from the new file position
            *len = strlen(*buf);
            free(buf2);
            //printf(*buf);
            return 1;
        }
        borShutdown(1, "Can't find file '%s' to include.\n", incfile);
    }
    else if(stricmp(command, "@jump") == 0)
    {
        *pos = GET_INT_ARGP(1);
        return 2;
    }
    else if(stricmp(command, "@end") == 0)
    {
        *pos = *len;
        return 3;
    }
    else if(stricmp(command, "@filename") == 0)
    {
        strcpy(namebuf, GET_ARGP(1));
        *fn = namebuf;
        return 4;
    }
    return 0;
}

int load_script(Script *script, char *file)
{
    size_t size = 0;
    int failed = 0;
    char *buf = NULL;

    if(buffer_pakfile(file, &buf, &size) != 1)
    {
        return 0;
    }

    failed = !Script_AppendText(script, buf, file);

    if(buf != NULL)
    {
        free(buf);
        buf = NULL;
    }

    // text loaded but parsing failed, shutdown
    if(failed)
    {
        borShutdown(1, "Failed to parse script file: '%s'!\n", file);
    }
    return !failed;
}

// this method is used by load_scripts, don't call it
void init_scripts()
{
    int i;
    Script_Global_Init();
    Script_Init(&update_script,     "update",  NULL,  1);
    Script_Init(&updated_script,    "updated",  NULL, 1);
    Script_Init(&update_logic_script,  "updatelogic",  NULL, 1);
    Script_Init(&updated_logic_script, "updatedlogic", NULL, 1);
    Script_Init(&model_load_script,    "modelload",    NULL, 1);
    Script_Init(&model_unload_script,  "modelunload",  NULL, 1);
    Script_Init(&level_script,      "level",    NULL,  1);
    Script_Init(&endlevel_script,   "endlevel",  NULL, 1);
	Script_Init(&input_script_all, "inputall", NULL, 1);
    Script_Init(&key_script_all,    "keyall",   NULL,  1);
    Script_Init(&score_script_all,  "scoreall", NULL,  1);
    Script_Init(&join_script_all,   "joinall",  NULL,  1);
    Script_Init(&respawn_script_all, "respawnall", NULL, 1);
    Script_Init(&pdie_script_all,   "dieall",   NULL,  1);
    Script_Init(&timetick_script,   "timetick",  NULL, 1);
    Script_Init(&loading_script,    "loading",   NULL, 1);
    for(i = 0; i < MAX_PLAYERS; i++)
    {
        Script_Init(&score_script[i],    "score",    NULL,  1);
    }
    for(i = 0; i < MAX_PLAYERS; i++)
    {
        Script_Init(&key_script[i],      "key",      NULL,  1);
    }
    for(i = 0; i < MAX_PLAYERS; i++)
    {
        Script_Init(&join_script[i],     "join",      NULL, 1);
    }
    for(i = 0; i < MAX_PLAYERS; i++)
    {
        Script_Init(&respawn_script[i],  "respawn",   NULL, 1);
    }
    for(i = 0; i < MAX_PLAYERS; i++)
    {
        Script_Init(&pdie_script[i],     "die",       NULL, 1);
    }
}

// This method is called once when the engine starts, do not use it multiple times
// It should be calld after load_script_setting
void load_scripts()
{
    int i;
    init_scripts();
    //Script_Clear's second parameter set to 2, because the script fails to load,
    //and will never have another chance to be loaded, so just clear the variable list in it
    if(!load_script(&update_script,     "data/scripts/update.c"))
    {
        Script_Clear(&update_script,        2);
    }
    if(!load_script(&updated_script,    "data/scripts/updated.c"))
    {
        Script_Clear(&updated_script,       2);
    }
    if(!load_script(&update_logic_script, "data/scripts/updatelogic.c"))
    {
        Script_Clear(&update_logic_script,  2);
    }
    if(!load_script(&updated_logic_script, "data/scripts/updatedlogic.c"))
    {
        Script_Clear(&updated_logic_script, 2);
    }
    if(!load_script(&model_load_script, "data/scripts/modelload.c"))
    {
        Script_Clear(&model_load_script, 2);
    }
    if(!load_script(&model_unload_script, "data/scripts/modelunload.c"))
    {
        Script_Clear(&model_unload_script, 2);
    }
    if(!load_script(&level_script,      "data/scripts/level.c"))
    {
        Script_Clear(&level_script,         2);
    }
    if(!load_script(&endlevel_script,   "data/scripts/endlevel.c"))
    {
        Script_Clear(&endlevel_script,      2);
    }
	if (!load_script(&input_script_all, "data/scripts/inputall.c"))
	{
		Script_Clear(&input_script_all, 2);
	}
    if(!load_script(&key_script_all,    "data/scripts/keyall.c"))
    {
        Script_Clear(&key_script_all,       2);
    }
    if(!load_script(&score_script_all,  "data/scripts/scoreall.c"))
    {
        Script_Clear(&score_script_all,     2);
    }
    if(!load_script(&join_script_all,   "data/scripts/joinall.c"))
    {
        Script_Clear(&join_script_all,      2);
    }
    if(!load_script(&respawn_script_all, "data/scripts/respawnall.c"))
    {
        Script_Clear(&respawn_script_all,   2);
    }
    if(!load_script(&pdie_script_all,   "data/scripts/dieall.c"))
    {
        Script_Clear(&pdie_script_all,      2);
    }
    if(!load_script(&timetick_script,   "data/scripts/timetick.c"))
    {
        Script_Clear(&timetick_script,      2);
    }
    if(!load_script(&loading_script,    "data/scripts/loading.c"))
    {
        Script_Clear(&loading_script,       2);
    }
    if(!load_script(&score_script[0],   "data/scripts/score1.c"))
    {
        Script_Clear(&score_script[0],      2);
    }
    if(!load_script(&score_script[1],   "data/scripts/score2.c"))
    {
        Script_Clear(&score_script[1],      2);
    }
    if(!load_script(&score_script[2],   "data/scripts/score3.c"))
    {
        Script_Clear(&score_script[2],      2);
    }
    if(!load_script(&score_script[3],   "data/scripts/score4.c"))
    {
        Script_Clear(&score_script[3],      2);
    }
    if(!load_script(&key_script[0],     "data/scripts/key1.c"))
    {
        Script_Clear(&key_script[0],        2);
    }
    if(!load_script(&key_script[1],     "data/scripts/key2.c"))
    {
        Script_Clear(&key_script[1],        2);
    }
    if(!load_script(&key_script[2],     "data/scripts/key3.c"))
    {
        Script_Clear(&key_script[2],        2);
    }
    if(!load_script(&key_script[3],     "data/scripts/key4.c"))
    {
        Script_Clear(&key_script[3],        2);
    }
    if(!load_script(&join_script[0],    "data/scripts/join1.c"))
    {
        Script_Clear(&join_script[0],       2);
    }
    if(!load_script(&join_script[1],    "data/scripts/join2.c"))
    {
        Script_Clear(&join_script[1],       2);
    }
    if(!load_script(&join_script[2],    "data/scripts/join3.c"))
    {
        Script_Clear(&join_script[2],       2);
    }
    if(!load_script(&join_script[3],    "data/scripts/join4.c"))
    {
        Script_Clear(&join_script[3],       2);
    }
    if(!load_script(&respawn_script[0], "data/scripts/respawn1.c"))
    {
        Script_Clear(&respawn_script[0],    2);
    }
    if(!load_script(&respawn_script[1], "data/scripts/respawn2.c"))
    {
        Script_Clear(&respawn_script[1],    2);
    }
    if(!load_script(&respawn_script[2], "data/scripts/respawn3.c"))
    {
        Script_Clear(&respawn_script[2],    2);
    }
    if(!load_script(&respawn_script[3], "data/scripts/respawn4.c"))
    {
        Script_Clear(&respawn_script[3],    2);
    }
    if(!load_script(&pdie_script[0],    "data/scripts/die1.c"))
    {
        Script_Clear(&pdie_script[0],       2);
    }
    if(!load_script(&pdie_script[1],    "data/scripts/die2.c"))
    {
        Script_Clear(&pdie_script[1],       2);
    }
    if(!load_script(&pdie_script[2],    "data/scripts/die3.c"))
    {
        Script_Clear(&pdie_script[2],       2);
    }
    if(!load_script(&pdie_script[3],    "data/scripts/die4.c"))
    {
        Script_Clear(&pdie_script[3],       2);
    }
    Script_Compile(&update_script);
    Script_Compile(&updated_script);
    Script_Compile(&update_logic_script);
    Script_Compile(&updated_logic_script);
    Script_Compile(&model_load_script);
    Script_Compile(&model_unload_script);
    Script_Compile(&level_script);
    Script_Compile(&endlevel_script);
	Script_Compile(&input_script_all);
    Script_Compile(&key_script_all);
    Script_Compile(&score_script_all);
    Script_Compile(&join_script_all);
    Script_Compile(&respawn_script_all);
    Script_Compile(&pdie_script_all);
    Script_Compile(&timetick_script);
    Script_Compile(&loading_script);
    for(i = 0; i < MAX_PLAYERS; i++)
    {
        Script_Compile(&score_script[i]);
    }
    for(i = 0; i < MAX_PLAYERS; i++)
    {
        Script_Compile(&key_script[i]);
    }
    for(i = 0; i < MAX_PLAYERS; i++)
    {
        Script_Compile(&join_script[i]);
    }
    for(i = 0; i < MAX_PLAYERS; i++)
    {
        Script_Compile(&respawn_script[i]);
    }
    for(i = 0; i < MAX_PLAYERS; i++)
    {
        Script_Compile(&pdie_script[i]);
    }
}

void unfrozen(entity *e)
{
    ent_set_colourmap(e, e->map);
    e->frozen = 0;
    e->freezetime = 0;
}

int is_frozen(entity *e)
{
    return ((textbox && e->modeldata.type != TYPE_TEXTBOX) ||
						 (smartbomber && e != smartbomber && e->modeldata.type != TYPE_TEXTBOX) || (self->frozen && self->freezetime > _time));
}

// This method is called once when the engine is shutting down, do not use it multiple times
void clear_scripts()
{
    int i;
    //Script_Clear's second parameter set to 2, because the script fails to load,
    //and will never have another chance to be loaded, so just clear the variable list in it
    Script_Clear(&update_script,    2);
    Script_Clear(&updated_script,   2);
    Script_Clear(&update_logic_script,  2);
    Script_Clear(&updated_logic_script, 2);
    Script_Clear(&model_load_script,    2);
    Script_Clear(&model_unload_script,  2);
    Script_Clear(&level_script,     2);
    Script_Clear(&endlevel_script,  2);
	Script_Clear(&input_script_all, 2);
    Script_Clear(&key_script_all,   2);
    Script_Clear(&score_script_all, 2);
    Script_Clear(&join_script_all,  2);
    Script_Clear(&respawn_script_all, 2);
    Script_Clear(&pdie_script_all,  2);
    Script_Clear(&timetick_script,  2);
    Script_Clear(&loading_script,   2);
    for(i = 0; i < MAX_PLAYERS; i++)
    {
        Script_Clear(&score_script[i],      2);
    }
    for(i = 0; i < MAX_PLAYERS; i++)
    {
        Script_Clear(&key_script[i],        2);
    }
    for(i = 0; i < MAX_PLAYERS; i++)
    {
        Script_Clear(&join_script[i],       2);
    }
    for(i = 0; i < MAX_PLAYERS; i++)
    {
        Script_Clear(&respawn_script[i],    2);
    }
    for(i = 0; i < MAX_PLAYERS; i++)
    {
        Script_Clear(&pdie_script[i],       2);
    }
    Script_Global_Clear();
}

#define scripts_membercount (sizeof(s_scripts) / sizeof(Script*))

void alloc_all_scripts(s_scripts **s)
{
    size_t i;

    if(!(*s))
    {
        *s = (s_scripts *)malloc(sizeof(s_scripts));
        for (i = 0; i < scripts_membercount; i++)
        {
            (((Script **) (*s))[i]) = alloc_script();
        }
    }
}

void clear_all_scripts(s_scripts *s, int method)
{
    size_t i;
    Script **ps = (Script **) s;

    for (i = 0; i < scripts_membercount; i++)
    {
        Script_Clear(ps[i],   method);
    }
}

void free_all_scripts(s_scripts **s)
{
    size_t i;
    Script **ps = (Script **) (*s);

    for (i = 0; i < scripts_membercount; i++)
    {
        if (ps[i])
        {
            free(ps[i]);
            ps[i] = NULL;
        }
    }
    free(*s);
    *s = NULL;
}

void copy_all_scripts(s_scripts *src, s_scripts *dest, int method)
{
    size_t i;
    Script **ps = (Script **) src;
    Script **pd = (Script **) dest;

    for (i = 0; i < scripts_membercount; i++)
    {
        Script_Copy(pd[i], ps[i], method);
    }
}

/*
* Caskey, Damon V.
* 2026-07-31
*
* Execute model-owned animation bytecode with 
* an entity's local variables.
*
* This function fixes an issue where oncreate()
* and ondestroy() were called every time an 
* animation script was executed.
*
* Script_Copy() is a lifecycle operation: it 
* runs ondestroy() for the current script and 
* oncreate() for the incoming script. Animation 
* scripts only need to borrow the model interpreter 
* while processing a frame, so temporarily select 
* the source interpreter without recreating the 
* entity script.
*/
static void execute_animation_script_source(Script *context, Script *source) {

    Interpreter *saved_interpreter = context->pinterpreter;
    char *saved_comment = context->comment;

    context->pinterpreter = source->pinterpreter;
    context->comment = source->comment;

    Script_Execute(context);

    context->pinterpreter = saved_interpreter;
    context->comment = saved_comment;
}

void execute_animation_script(entity *ent) {
    ScriptVariant tempvar;

    char *namelist[] = {"self", "animnum", "frame", "animhandle", ""};
    int handle = 0;
    
    Script *cs = ent->scripts->animation_script;
    Script *model_script = ent->model->scripts->animation_script;
    Script *defaultmodel_script = ent->defaultmodel->scripts->animation_script;
    
    const bool model_script_initialized = Script_IsInitialized(model_script);
    const bool defaultmodel_script_initialized = Script_IsInitialized(defaultmodel_script);
    
    if(model_script_initialized || defaultmodel_script_initialized) {

        if(cs->pinterpreter && cs->pinterpreter->bReset) {
            handle = Script_Save_Local_Variant(cs, namelist);
        }
        
        ScriptVariant_Init(&tempvar);
        
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID *)ent;
        Script_Set_Local_Variant(cs, "self",    &tempvar);
        
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        tempvar.lVal = (LONG)ent->animnum;
        
        Script_Set_Local_Variant(cs, "animnum", &tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        
        tempvar.lVal = (LONG)ent->animpos;
        Script_Set_Local_Variant(cs, "frame",   &tempvar);
        
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        tempvar.lVal = (LONG)ent->animation->index;
        Script_Set_Local_Variant(cs, "animhandle",   &tempvar);
        
        if(model_script_initialized) {
            execute_animation_script_source(cs, model_script);
        }

        
        if(ent->model != ent->defaultmodel && defaultmodel_script_initialized) {
            execute_animation_script_source(cs, defaultmodel_script);
        }

        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self",    &tempvar);
        Script_Set_Local_Variant(cs, "animnum", &tempvar);
        Script_Set_Local_Variant(cs, "frame",   &tempvar);
        Script_Set_Local_Variant(cs, "animhandle", &tempvar);
        
        if(handle) {
            Script_Load_Local_Variant(cs, handle);
        }
    }
}

void execute_takedamage_script(entity *ent, entity *other, s_attack *attack)
{
    ScriptVariant tempvar;
    Script *cs = ent->scripts->takedamage_script;
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);

        tempvar.ptrVal = (VOID *)ent;
        Script_Set_Local_Variant(cs, "self",        &tempvar);

        tempvar.ptrVal = (VOID *)other;
        Script_Set_Local_Variant(cs, "attacker",    &tempvar);

        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);

        tempvar.lVal = (LONG)attack->attack_force;
        Script_Set_Local_Variant(cs, "damage",      &tempvar);

        tempvar.lVal = (LONG)attack->attack_drop;
        Script_Set_Local_Variant(cs, "drop",        &tempvar);

        tempvar.lVal = (LONG)attack->attack_type;
        Script_Set_Local_Variant(cs, "attacktype",  &tempvar);

        tempvar.lVal = (LONG)attack->no_block;
        Script_Set_Local_Variant(cs, "noblock",     &tempvar);

        tempvar.lVal = (LONG)attack->guardcost;
        Script_Set_Local_Variant(cs, "guardcost",   &tempvar);

        tempvar.lVal = (LONG)attack->jugglecost;
        Script_Set_Local_Variant(cs, "jugglecost",  &tempvar);

        tempvar.lVal = (LONG)attack->pause_add;
        Script_Set_Local_Variant(cs, "pauseadd",    &tempvar);

        tempvar.lVal = (LONG)attack->meta_tag;
        Script_Set_Local_Variant(cs, "tag",    &tempvar);


        Script_Execute(cs);

        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self",        &tempvar);
        Script_Set_Local_Variant(cs, "attacker",    &tempvar);
        Script_Set_Local_Variant(cs, "damage",      &tempvar);
        Script_Set_Local_Variant(cs, "drop",        &tempvar);
        Script_Set_Local_Variant(cs, "attacktype",  &tempvar);
        Script_Set_Local_Variant(cs, "noblock",     &tempvar);
        Script_Set_Local_Variant(cs, "guardcost",   &tempvar);
        Script_Set_Local_Variant(cs, "jugglecost",  &tempvar);
        Script_Set_Local_Variant(cs, "pauseadd",    &tempvar);
        Script_Set_Local_Variant(cs, "tag",    &tempvar);
    }
}

// Caskey, Damon V.
// 2018-08-30
//
// Run on the bind target when updating a bind.
void execute_on_bind_update_other_to_self(entity *ent, entity *other, s_bind *bind)
{
    ScriptVariant tempvar;
    Script *cs = ent->scripts->on_bind_update_other_to_self_script;

    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);

        tempvar.ptrVal = (entity *)ent;
        Script_Set_Local_Variant(cs, "self", &tempvar);

        tempvar.ptrVal = (entity *)other;
        Script_Set_Local_Variant(cs, "other", &tempvar);

        tempvar.ptrVal = (s_bind *)bind;
        Script_Set_Local_Variant(cs, "bind", &tempvar);

        Script_Execute(cs);

        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self",	&tempvar);
        Script_Set_Local_Variant(cs, "other",	&tempvar);
        Script_Set_Local_Variant(cs, "bind",	&tempvar);
    }
}

// Caskey, Damon V.
// 2018-08-30
//
// Run on bound entity when updating bind.
void execute_on_bind_update_self_to_other(entity *ent, entity *other, s_bind *bind)
{
    ScriptVariant tempvar;
    Script *cs = ent->scripts->on_bind_update_self_to_other_script;

    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);

        tempvar.ptrVal = (entity *)ent;
        Script_Set_Local_Variant(cs, "self", &tempvar);

        tempvar.ptrVal = (entity *)other;
        Script_Set_Local_Variant(cs, "other", &tempvar);

        tempvar.ptrVal = (s_bind *)bind;
        Script_Set_Local_Variant(cs, "bind", &tempvar);

        Script_Execute(cs);

        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self",	&tempvar);
        Script_Set_Local_Variant(cs, "other",	&tempvar);
        Script_Set_Local_Variant(cs, "bind",	&tempvar);
    }
}

void execute_onpain_script(entity *ent, int iType, int iReset)
{
    ScriptVariant tempvar;
    Script *cs = ent->scripts->onpain_script;
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID *)ent;
        Script_Set_Local_Variant(cs, "self",        &tempvar);
        tempvar.lVal = (LONG)iType;
        Script_Set_Local_Variant(cs, "attacktype",   &tempvar);
        tempvar.lVal = (LONG)iReset;
        Script_Set_Local_Variant(cs, "reset",       &tempvar);
        Script_Execute(cs);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self",        &tempvar);
        Script_Set_Local_Variant(cs, "type",        &tempvar);
        Script_Set_Local_Variant(cs, "reset",       &tempvar);
    }
}

void execute_onfall_script(entity *ent, entity *other, s_attack *attack)
{
    ScriptVariant tempvar;
    Script *cs = ent->scripts->onfall_script;
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID *)ent;
        Script_Set_Local_Variant(cs, "self",        &tempvar);

        tempvar.ptrVal = (VOID *)other;
        Script_Set_Local_Variant(cs, "attacker",    &tempvar);

        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);

        tempvar.lVal = (LONG)attack->attack_force;
        Script_Set_Local_Variant(cs, "damage",      &tempvar);

        tempvar.lVal = (LONG)attack->attack_drop;
        Script_Set_Local_Variant(cs, "drop",        &tempvar);

        tempvar.lVal = (LONG)attack->attack_type;
        Script_Set_Local_Variant(cs, "attacktype",  &tempvar);

        tempvar.lVal = (LONG)attack->no_block;
        Script_Set_Local_Variant(cs, "noblock",     &tempvar);

        tempvar.lVal = (LONG)attack->guardcost;
        Script_Set_Local_Variant(cs, "guardcost",   &tempvar);

        tempvar.lVal = (LONG)attack->jugglecost;
        Script_Set_Local_Variant(cs, "jugglecost",  &tempvar);

        tempvar.lVal = (LONG)attack->pause_add;
        Script_Set_Local_Variant(cs, "pauseadd",    &tempvar);

        tempvar.lVal = (LONG)attack->meta_tag;
        Script_Set_Local_Variant(cs, "tag",    &tempvar);

        Script_Execute(cs);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self",        &tempvar);
        Script_Set_Local_Variant(cs, "attacker",    &tempvar);
        Script_Set_Local_Variant(cs, "damage",      &tempvar);
        Script_Set_Local_Variant(cs, "drop",        &tempvar);
        Script_Set_Local_Variant(cs, "attacktype",  &tempvar);
        Script_Set_Local_Variant(cs, "noblock",     &tempvar);
        Script_Set_Local_Variant(cs, "guardcost",   &tempvar);
        Script_Set_Local_Variant(cs, "jugglecost",  &tempvar);
        Script_Set_Local_Variant(cs, "pauseadd",    &tempvar);
        Script_Set_Local_Variant(cs, "tag",         &tempvar);
    }
}

void execute_onblocks_script(entity *ent)
{
    ScriptVariant tempvar;
    Script *cs = ent->scripts->onblocks_script;
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID *)ent;
        Script_Set_Local_Variant(cs, "self", &tempvar);
        Script_Execute(cs);

        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self", &tempvar);
    }
}

void execute_onblockw_script(entity *ent, s_terrain *wall, int index, e_plane plane)
{
    ScriptVariant tempvar;
    Script *cs = ent->scripts->onblockw_script;
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID *)ent;
        Script_Set_Local_Variant(cs, "self", &tempvar);

        ScriptVariant_ChangeType(&tempvar, VT_DECIMAL);
        tempvar.dblVal = (DOUBLE)wall->height;
        Script_Set_Local_Variant(cs, "height", &tempvar);

        tempvar.dblVal = (DOUBLE)wall->depth;
        Script_Set_Local_Variant(cs, "depth", &tempvar);

        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);

        tempvar.lVal = (LONG)wall->type;
        Script_Set_Local_Variant(cs, "type", &tempvar);

        tempvar.lVal = (LONG)index;
        Script_Set_Local_Variant(cs, "index", &tempvar);

        tempvar.lVal = (LONG)plane;
        Script_Set_Local_Variant(cs, "plane", &tempvar);

        Script_Execute(cs);

        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self", &tempvar);
        Script_Set_Local_Variant(cs, "plane", &tempvar);
        Script_Set_Local_Variant(cs, "height", &tempvar);
        Script_Set_Local_Variant(cs, "index", &tempvar);
        Script_Set_Local_Variant(cs, "depth", &tempvar);
        Script_Set_Local_Variant(cs, "type", &tempvar);
    }
}

void execute_inhole_script(entity *ent, s_terrain *hole, int index)
{
    ScriptVariant tempvar;
    Script *cs = ent->scripts->inhole_script;
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID *)ent;
        Script_Set_Local_Variant(cs, "self", &tempvar);

        ScriptVariant_ChangeType(&tempvar, VT_DECIMAL);
        tempvar.dblVal = (DOUBLE)hole->height;
        Script_Set_Local_Variant(cs, "height", &tempvar);

        tempvar.dblVal = (DOUBLE)hole->depth;
        Script_Set_Local_Variant(cs, "depth", &tempvar);

        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);

        tempvar.lVal = (LONG)hole->type;
        Script_Set_Local_Variant(cs, "type", &tempvar);

        tempvar.lVal = (LONG)index;
        Script_Set_Local_Variant(cs, "index", &tempvar);

        Script_Execute(cs);

        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self", &tempvar);
        Script_Set_Local_Variant(cs, "height", &tempvar);
        Script_Set_Local_Variant(cs, "index", &tempvar);
        Script_Set_Local_Variant(cs, "depth", &tempvar);
        Script_Set_Local_Variant(cs, "type", &tempvar);
    }
}

void execute_onblockp_script(entity *ent, int plane, entity *platform)
{
    ScriptVariant tempvar;
    Script *cs = ent->scripts->onblockp_script;
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID *)ent;
        Script_Set_Local_Variant(cs, "self", &tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        tempvar.lVal = (LONG)plane;
        Script_Set_Local_Variant(cs, "plane",      &tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID *)platform;
        Script_Set_Local_Variant(cs, "platform",      &tempvar);
        Script_Execute(cs);

        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self", &tempvar);
        Script_Set_Local_Variant(cs, "plane", &tempvar);
        Script_Set_Local_Variant(cs, "platform", &tempvar);
    }
}

void execute_onblocko_script(entity *ent, int plane, entity *other)
{
    ScriptVariant tempvar;
    Script *cs = ent->scripts->onblocko_script;
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID *)ent;
        Script_Set_Local_Variant(cs, "self",        &tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        tempvar.lVal = (LONG)plane;
        Script_Set_Local_Variant(cs, "plane",      &tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID *)other;
        Script_Set_Local_Variant(cs, "obstacle",    &tempvar);
        Script_Execute(cs);

        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self",        &tempvar);
        Script_Set_Local_Variant(cs, "plane", &tempvar);
        Script_Set_Local_Variant(cs, "obstacle",    &tempvar);
    }
}

void execute_onblockz_script(entity *ent)
{
    ScriptVariant tempvar;
    Script *cs = ent->scripts->onblockz_script;
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID *)ent;
        Script_Set_Local_Variant(cs, "self", &tempvar);
        Script_Execute(cs);

        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self", &tempvar);
    }
}

void execute_onblocky_script(entity *ent, entity *other)
{
    ScriptVariant tempvar;
    Script *cs = ent->scripts->onblocky_script;
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID *)ent;
        Script_Set_Local_Variant(cs, "self",        &tempvar);
        tempvar.ptrVal = (VOID *)other;
        Script_Set_Local_Variant(cs, "obstacle",    &tempvar);
        Script_Execute(cs);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self",        &tempvar);
        Script_Set_Local_Variant(cs, "obstacle",    &tempvar);
    }
}

void execute_onmovex_script(entity *ent)
{
    ScriptVariant tempvar;
    Script *cs = ent->scripts->onmovex_script;
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID *)ent;
        Script_Set_Local_Variant(cs, "self", &tempvar);
        Script_Execute(cs);

        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self", &tempvar);
    }
}

void execute_onmovez_script(entity *ent)
{
    ScriptVariant tempvar;
    Script *cs = ent->scripts->onmovez_script;
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID *)ent;
        Script_Set_Local_Variant(cs, "self", &tempvar);
        Script_Execute(cs);

        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self", &tempvar);
    }
}

void execute_onmovea_script(entity *ent)
{
    ScriptVariant tempvar;
    Script *cs = ent->scripts->onmovea_script;
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID *)ent;
        Script_Set_Local_Variant(cs, "self", &tempvar);
        Script_Execute(cs);

        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self", &tempvar);
    }
}

void execute_ondeath_script(entity *ent, entity *other, s_attack *attack)
{
    ScriptVariant tempvar;
    Script *cs = ent->scripts->ondeath_script;
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);

        tempvar.ptrVal = (VOID *)ent;
        Script_Set_Local_Variant(cs, "self",        &tempvar);

        tempvar.ptrVal = (VOID *)other;
        Script_Set_Local_Variant(cs, "attacker",    &tempvar);

        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);

        tempvar.lVal = (LONG)attack->attack_force;
        Script_Set_Local_Variant(cs, "damage",      &tempvar);

        tempvar.lVal = (LONG)attack->attack_drop;
        Script_Set_Local_Variant(cs, "drop",        &tempvar);

        tempvar.lVal = (LONG)attack->attack_type;
        Script_Set_Local_Variant(cs, "attacktype",  &tempvar);

        tempvar.lVal = (LONG)attack->no_block;
        Script_Set_Local_Variant(cs, "noblock",     &tempvar);

        tempvar.lVal = (LONG)attack->guardcost;
        Script_Set_Local_Variant(cs, "guardcost",   &tempvar);

        tempvar.lVal = (LONG)attack->jugglecost;
        Script_Set_Local_Variant(cs, "jugglecost",  &tempvar);

        tempvar.lVal = (LONG)attack->pause_add;
        Script_Set_Local_Variant(cs, "pauseadd",    &tempvar);

        tempvar.lVal = (LONG)attack->meta_tag;
        Script_Set_Local_Variant(cs, "tag",    &tempvar);

        Script_Execute(cs);
        //clear to save variant space

        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self",        &tempvar);
        Script_Set_Local_Variant(cs, "attacker",    &tempvar);
        Script_Set_Local_Variant(cs, "damage",      &tempvar);
        Script_Set_Local_Variant(cs, "drop",        &tempvar);
        Script_Set_Local_Variant(cs, "attacktype",  &tempvar);
        Script_Set_Local_Variant(cs, "noblock",     &tempvar);
        Script_Set_Local_Variant(cs, "guardcost",   &tempvar);
        Script_Set_Local_Variant(cs, "jugglecost",  &tempvar);
        Script_Set_Local_Variant(cs, "pauseadd",    &tempvar);
        Script_Set_Local_Variant(cs, "tag",         &tempvar);
    }
}

void execute_onkill_script(entity *ent, e_kill_entity_trigger trigger)
{
    ScriptVariant tempvar;
    Script *cs = ent->scripts->onkill_script;
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        
        ScriptVariant_ChangeType(&tempvar, VT_PTR);

        tempvar.ptrVal = (VOID *)ent;
        Script_Set_Local_Variant(cs, "self", &tempvar);

        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);

        tempvar.lVal = (e_kill_entity_trigger)trigger;
        Script_Set_Local_Variant(cs, "trigger", &tempvar);

        Script_Execute(cs);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self", &tempvar);
        Script_Set_Local_Variant(cs, "trigger", &tempvar);
    }
}

void execute_didblock_script(entity *ent, entity *other, s_attack *attack)
{
    ScriptVariant tempvar;
    Script *cs = ent->scripts->didblock_script;
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);

        tempvar.ptrVal = (VOID *)ent;
        Script_Set_Local_Variant(cs, "self",        &tempvar);

        tempvar.ptrVal = (VOID *)other;
        Script_Set_Local_Variant(cs, "attacker",    &tempvar);

        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);

        tempvar.lVal = (LONG)attack->attack_force;
        Script_Set_Local_Variant(cs, "damage",      &tempvar);

        tempvar.lVal = (LONG)attack->attack_drop;
        Script_Set_Local_Variant(cs, "drop",        &tempvar);

        tempvar.lVal = (LONG)attack->attack_type;
        Script_Set_Local_Variant(cs, "attacktype",  &tempvar);

        tempvar.lVal = (LONG)attack->no_block;
        Script_Set_Local_Variant(cs, "noblock",     &tempvar);

        tempvar.lVal = (LONG)attack->guardcost;
        Script_Set_Local_Variant(cs, "guardcost",   &tempvar);

        tempvar.lVal = (LONG)attack->jugglecost;
        Script_Set_Local_Variant(cs, "jugglecost",  &tempvar);

        tempvar.lVal = (LONG)attack->pause_add;
        Script_Set_Local_Variant(cs, "pauseadd",    &tempvar);

        tempvar.lVal = (LONG)attack->meta_tag;
        Script_Set_Local_Variant(cs, "tag",    &tempvar);

        Script_Execute(cs);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self",        &tempvar);
        Script_Set_Local_Variant(cs, "attacker",    &tempvar);
        Script_Set_Local_Variant(cs, "damage",      &tempvar);
        Script_Set_Local_Variant(cs, "drop",        &tempvar);
        Script_Set_Local_Variant(cs, "attacktype",  &tempvar);
        Script_Set_Local_Variant(cs, "noblock",     &tempvar);
        Script_Set_Local_Variant(cs, "guardcost",   &tempvar);
        Script_Set_Local_Variant(cs, "jugglecost",  &tempvar);
        Script_Set_Local_Variant(cs, "pauseadd",    &tempvar);
        Script_Set_Local_Variant(cs, "tag",         &tempvar);
    }
}

void execute_ondoattack_script(entity *ent, entity *other, s_attack *attack, e_exchange which, int attack_id)
{
    ScriptVariant tempvar;
    Script *cs = ent->scripts->ondoattack_script;
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID *)ent;
        Script_Set_Local_Variant(cs, "self",        &tempvar);

        tempvar.ptrVal = (VOID *)other;
        Script_Set_Local_Variant(cs, "other",    &tempvar);

        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);

        tempvar.lVal = (LONG)attack->attack_force;
        Script_Set_Local_Variant(cs, "damage",      &tempvar);

        tempvar.lVal = (LONG)attack->attack_drop;
        Script_Set_Local_Variant(cs, "drop",        &tempvar);

        tempvar.lVal = (LONG)attack->attack_type;
        Script_Set_Local_Variant(cs, "attacktype",  &tempvar);

        tempvar.lVal = (LONG)attack->no_block;
        Script_Set_Local_Variant(cs, "noblock",     &tempvar);

        tempvar.lVal = (LONG)attack->guardcost;
        Script_Set_Local_Variant(cs, "guardcost",   &tempvar);

        tempvar.lVal = (LONG)attack->jugglecost;
        Script_Set_Local_Variant(cs, "jugglecost",  &tempvar);

        tempvar.lVal = (LONG)attack->pause_add;
        Script_Set_Local_Variant(cs, "pauseadd",    &tempvar);

        tempvar.lVal = (LONG)attack->meta_tag;
        Script_Set_Local_Variant(cs, "tag",    &tempvar);

        tempvar.lVal = (LONG)which;
        Script_Set_Local_Variant(cs, "which",    &tempvar);

        tempvar.lVal = (LONG)attack_id;
        Script_Set_Local_Variant(cs, "attack_id",    &tempvar);

        Script_Execute(cs);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self",        &tempvar);
        Script_Set_Local_Variant(cs, "other",		&tempvar);
        Script_Set_Local_Variant(cs, "damage",      &tempvar);
        Script_Set_Local_Variant(cs, "drop",        &tempvar);
        Script_Set_Local_Variant(cs, "attacktype",  &tempvar);
        Script_Set_Local_Variant(cs, "noblock",     &tempvar);
        Script_Set_Local_Variant(cs, "guardcost",   &tempvar);
        Script_Set_Local_Variant(cs, "jugglecost",  &tempvar);
        Script_Set_Local_Variant(cs, "pauseadd",    &tempvar);
        Script_Set_Local_Variant(cs, "which",		&tempvar);
        Script_Set_Local_Variant(cs, "attack_id",	&tempvar);
        Script_Set_Local_Variant(cs, "tag",	        &tempvar);
    }
}

void execute_updateentity_script(entity *ent)
{
    ScriptVariant tempvar;
    Script *cs = ent->scripts->update_script;
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID *)ent;
        Script_Set_Local_Variant(cs, "self", &tempvar);
        Script_Execute(cs);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self", &tempvar);
    }
}

void execute_think_script(entity *ent)
{
    ScriptVariant tempvar;
    Script *cs = ent->scripts->think_script;
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID *)ent;
        Script_Set_Local_Variant(cs, "self", &tempvar);
        Script_Execute(cs);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self", &tempvar);
    }
}

static void _execute_didhit_script(Script *cs, entity *ent, entity *other, s_attack *attack, int blocked)
{
    ScriptVariant tempvar;
    ScriptVariant_Init(&tempvar);

    ScriptVariant_ChangeType(&tempvar, VT_PTR);

    tempvar.ptrVal = (VOID *)ent;
    Script_Set_Local_Variant(cs, "self",        &tempvar);

    tempvar.ptrVal = (VOID *)other;
    Script_Set_Local_Variant(cs, "damagetaker", &tempvar);

    ScriptVariant_ChangeType(&tempvar, VT_INTEGER);

    tempvar.lVal = (LONG)attack->attack_force;
    Script_Set_Local_Variant(cs, "damage",      &tempvar);

    tempvar.lVal = (LONG)attack->attack_drop;
    Script_Set_Local_Variant(cs, "drop",        &tempvar);

    tempvar.lVal = (LONG)attack->attack_type;
    Script_Set_Local_Variant(cs, "attacktype",  &tempvar);

    tempvar.lVal = (LONG)attack->no_block;
    Script_Set_Local_Variant(cs, "noblock",     &tempvar);

    tempvar.lVal = (LONG)attack->guardcost;
    Script_Set_Local_Variant(cs, "guardcost",   &tempvar);

    tempvar.lVal = (LONG)attack->jugglecost;
    Script_Set_Local_Variant(cs, "jugglecost",  &tempvar);

    tempvar.lVal = (LONG)attack->pause_add;
    Script_Set_Local_Variant(cs, "pauseadd",    &tempvar);

    tempvar.lVal = (LONG)attack->meta_tag;
    Script_Set_Local_Variant(cs, "tag",    &tempvar);

    tempvar.lVal = (LONG)blocked;
    Script_Set_Local_Variant(cs, "blocked",    &tempvar);


    Script_Execute(cs);
    //clear to save variant space
    ScriptVariant_Clear(&tempvar);
    Script_Set_Local_Variant(cs, "self",        &tempvar);
    Script_Set_Local_Variant(cs, "damagetaker", &tempvar);
    Script_Set_Local_Variant(cs, "damage",      &tempvar);
    Script_Set_Local_Variant(cs, "drop",        &tempvar);
    Script_Set_Local_Variant(cs, "attacktype",  &tempvar);
    Script_Set_Local_Variant(cs, "noblock",     &tempvar);
    Script_Set_Local_Variant(cs, "guardcost",   &tempvar);
    Script_Set_Local_Variant(cs, "jugglecost",  &tempvar);
    Script_Set_Local_Variant(cs, "pauseadd",    &tempvar);
    Script_Set_Local_Variant(cs, "blocked",     &tempvar);
    Script_Set_Local_Variant(cs, "tag",         &tempvar);
}

void execute_didhit_script(entity *ent, entity *other, s_attack *attack, int blocked)
{
    Script *cs;
    s_scripts *gs = global_model_scripts;
    if(gs && (cs = gs->didhit_script) && Script_IsInitialized(cs))
    {
        _execute_didhit_script(cs, ent, other, attack, blocked);
    }
    if(Script_IsInitialized(cs = ent->scripts->didhit_script))
    {
        _execute_didhit_script(cs, ent, other, attack, blocked);
    }
}

static void _execute_onspawn_script(Script *cs, entity *ent)
{
    ScriptVariant tempvar;
    ScriptVariant_Init(&tempvar);
    ScriptVariant_ChangeType(&tempvar, VT_PTR);
    tempvar.ptrVal = (VOID *)ent;
    Script_Set_Local_Variant(cs, "self", &tempvar);
    Script_Execute(cs);
    //clear to save variant space
    ScriptVariant_Clear(&tempvar);
    Script_Set_Local_Variant(cs, "self", &tempvar);
}

void execute_onspawn_script(entity *ent)
{
    Script *cs;
    s_scripts *gs = global_model_scripts;
    if(gs && (cs = gs->onspawn_script) && Script_IsInitialized(cs))
    {
        _execute_onspawn_script(cs, ent);
    }
    if(Script_IsInitialized(cs = ent->scripts->onspawn_script))
    {
        _execute_onspawn_script(cs, ent);
    }
}

void execute_onmodelcopy_script(entity *ent, entity *old)
{
    ScriptVariant tempvar;
    Script *cs = ent->scripts->onmodelcopy_script;
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID *)ent;
        Script_Set_Local_Variant(cs, "self", &tempvar);
        tempvar.ptrVal = (VOID *)old;
        Script_Set_Local_Variant(cs, "old", &tempvar);
        Script_Execute(cs);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self", &tempvar);
        Script_Set_Local_Variant(cs, "old", &tempvar);
    }
}

void execute_ondraw_script(entity *ent)
{
    ScriptVariant tempvar;
    Script *cs = ent->scripts->ondraw_script;
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID *)ent;
        Script_Set_Local_Variant(cs, "self", &tempvar);
        Script_Execute(cs);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self", &tempvar);
    }
}

void execute_entity_key_script(entity *ent)
{
    ScriptVariant tempvar;
    Script *cs ;
    if(!ent)
    {
        return;
    }
    cs = ent->scripts->key_script;

    /*
    * Caskey, Damon V.
    * 2026-08-23
    *
    * Temporary diagnostic for tracing the SORX aerial-recovery
    * regression. Report the complete native gate state only when
    * a falling player holds the recovery chord.
    */
    if(ent->playerindex >= 0
        && ent->playerindex < MAX_PLAYERS
        && ent->drop
        && (player[ent->playerindex].keys & (FLAG_MOVEUP | FLAG_JUMP))
            == (FLAG_MOVEUP | FLAG_JUMP))
    {
        printf(
            "AERIAL_RECOVERY_GATE player=%" PRId64
            " animation=%" PRIu64
            " fall=%" PRIu64
            " frame=%" PRIu64
            " seal=%" PRId64
            " dead=%d"
            " guard=%" PRId64
            " maxguard=%d"
            " projectile=%d"
            " linked=%d\n",
            ent->playerindex,
            (uint64_t)ent->animnum,
            (uint64_t)ANI_FALL,
            ent->animpos,
            ent->seal,
            (ent->death_state & DEATH_STATE_DEAD) != 0,
            ent->guardpoints,
            ent->modeldata.guardpoints,
            ent->projectile,
            ent->link != NULL
        );
    }

    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID *)ent;
        Script_Set_Local_Variant(cs, "self",    &tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        tempvar.lVal = (LONG)ent->playerindex;
        Script_Set_Local_Variant(cs, "player",  &tempvar);
        Script_Execute(cs);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "self",    &tempvar);
        Script_Set_Local_Variant(cs, "player",  &tempvar);
    }
}

void execute_spawn_script(s_spawn_entry *p, entity *e)
{
    ScriptVariant tempvar;
    Script *cs;
    cs = &p->spawnscript;
    if(Script_IsInitialized(cs))
    {
        if(e)
        {
            ScriptVariant_Init(&tempvar);
            ScriptVariant_ChangeType(&tempvar, VT_PTR);
            tempvar.ptrVal = (VOID *)e;
            Script_Set_Local_Variant(cs, "self", &tempvar);
            ScriptVariant_ChangeType(&tempvar, VT_DECIMAL);
            tempvar.dblVal = (DOUBLE)p->position.x;
            Script_Set_Local_Variant(cs, "spawnx", &tempvar);
            tempvar.dblVal = (DOUBLE)p->position.z;
            Script_Set_Local_Variant(cs, "spawnz", &tempvar);
            tempvar.dblVal = (DOUBLE)p->position.y;
            Script_Set_Local_Variant(cs, "spawny", &tempvar);
            Script_Set_Local_Variant(cs, "spawna", &tempvar);
            ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
            tempvar.lVal = (LONG)p->at;
            Script_Set_Local_Variant(cs, "spawnat", &tempvar);
        }
        Script_Execute(cs);
        if(e)
        {
            ScriptVariant_Clear(&tempvar);
            Script_Set_Local_Variant(cs, "self", &tempvar);
            Script_Set_Local_Variant(cs, "spawnx", &tempvar);
            Script_Set_Local_Variant(cs, "spawnz", &tempvar);
            Script_Set_Local_Variant(cs, "spawny", &tempvar);
            Script_Set_Local_Variant(cs, "spawna", &tempvar);
            Script_Set_Local_Variant(cs, "spawnat", &tempvar);
        }
    }
}

void execute_level_key_script(int player)
{
    ScriptVariant tempvar;
    Script *cs = &(level->key_script);
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        tempvar.lVal = (LONG)player;
        Script_Set_Local_Variant(cs, "player", &tempvar);
        Script_Execute(cs);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "player", &tempvar);
    }
}

void execute_input_script_all(int player)
{
	ScriptVariant tempvar;
	Script *cs = &input_script_all;
	if (Script_IsInitialized(cs))
	{
		ScriptVariant_Init(&tempvar);

		//ScriptVariant_ChangeType(&tempvar, VT_PTR);

		//tempvar.ptrVal = (VOID *)player_object;
		//Script_Set_Local_Variant(cs, "player_object", &tempvar);

		ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
		tempvar.lVal = (LONG)player;
		
		Script_Set_Local_Variant(cs, "player", &tempvar);
		
		Script_Execute(cs);
		
		//clear to save variant space
		ScriptVariant_Clear(&tempvar);
		Script_Set_Local_Variant(cs, "player", &tempvar);
		//Script_Set_Local_Variant(cs, "player_object", &tempvar);
	}
}


void execute_key_script_all(int player)
{
    ScriptVariant tempvar;
    Script *cs = &key_script_all;
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        tempvar.lVal = (LONG)player;
        Script_Set_Local_Variant(cs, "player", &tempvar);
        Script_Execute(cs);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "player", &tempvar);
    }
}

/*
* Caskey, Damon V.
* 2026-08-20
*
* Execute the shared score listener with the zero-based
* playerindex and signed 64-bit score adjustment.
*/
void execute_score_script_all(int playerindex, int64_t score)
{
    ScriptVariant tempvar;
    Script *cs = &score_script_all;

    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        tempvar.lVal = (LONG)playerindex;
        Script_Set_Local_Variant(cs, "playerindex", &tempvar);

        ScriptVariant_ChangeType(&tempvar, VT_INTEGER64);
        tempvar.llVal = score;
        Script_Set_Local_Variant(cs, "score", &tempvar);

        Script_Execute(cs);

        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "playerindex", &tempvar);
        Script_Set_Local_Variant(cs, "score", &tempvar);
    }
}

void execute_timetick_script(int time, int gotime)
{
    ScriptVariant tempvar;
    Script *cs = &timetick_script;
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        tempvar.lVal = (LONG)time;
        Script_Set_Local_Variant(cs, "time",    &tempvar);
        tempvar.lVal = (LONG)gotime;
        Script_Set_Local_Variant(cs, "gotime", &tempvar);
        Script_Execute(cs);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "time",    &tempvar);
        Script_Set_Local_Variant(cs, "gotime",  &tempvar);
    }
}

void execute_loading_script(int value, int max)
{
    ScriptVariant tempvar;
    Script *cs = &loading_script;
    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        tempvar.lVal = (LONG)value;
        Script_Set_Local_Variant(cs, "value",    &tempvar);
        tempvar.lVal = (LONG)max;
        Script_Set_Local_Variant(cs, "max", &tempvar);
        Script_Execute(cs);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "value",    &tempvar);
        Script_Set_Local_Variant(cs, "max",  &tempvar);
    }
}

void execute_key_script(int index)
{
    if(Script_IsInitialized(&key_script[index]))
    {
        Script_Execute(&key_script[index]);
    }
}

/*
* Caskey, Damon V.
* 2026-08-20
*
* Execute a shared player event script with the zero-based
* playerindex that triggered the event.
*/
static void execute_player_script_all(Script *cs, int playerindex)
{
    ScriptVariant tempvar;

    if(Script_IsInitialized(cs))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        tempvar.lVal = (LONG)playerindex;
        Script_Set_Local_Variant(cs, "playerindex", &tempvar);
        Script_Execute(cs);
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(cs, "playerindex", &tempvar);
    }
}

void execute_join_script(int index)
{
    if(Script_IsInitialized(&join_script[index]))
    {
        Script_Execute(&join_script[index]);
    }

    execute_player_script_all(&join_script_all, index);
}

void execute_respawn_script(int index)
{
    if(Script_IsInitialized(&respawn_script[index]))
    {
        Script_Execute(&respawn_script[index]);
    }

    execute_player_script_all(&respawn_script_all, index);
}

void execute_pdie_script(int index)
{
    if(Script_IsInitialized(&pdie_script[index]))
    {
        Script_Execute(&pdie_script[index]);
    }

    execute_player_script_all(&pdie_script_all, index);
}

// ------------------------ Save/load -----------------------------

void clearbuttons(int player)
{
    savedata.joyrumble[player] = 0;

    if (player == 0)
    {
        savedata.keys[0][SDID_MOVEUP]    = CONTROL_DEFAULT1_UP; //Kratus (22-04-21) Maintain the key config only for player 1 because other modules will not work with CONTROL_NONE
        savedata.keys[0][SDID_MOVEDOWN]  = CONTROL_DEFAULT1_DOWN;
        savedata.keys[0][SDID_MOVELEFT]  = CONTROL_DEFAULT1_LEFT;
        savedata.keys[0][SDID_MOVERIGHT] = CONTROL_DEFAULT1_RIGHT;
        savedata.keys[0][SDID_ATTACK]    = CONTROL_DEFAULT1_FIRE1;
        savedata.keys[0][SDID_ATTACK2]   = CONTROL_DEFAULT1_FIRE2;
        savedata.keys[0][SDID_ATTACK3]   = CONTROL_DEFAULT1_FIRE3;
        savedata.keys[0][SDID_ATTACK4]   = CONTROL_DEFAULT1_FIRE4;
        savedata.keys[0][SDID_JUMP]      = CONTROL_DEFAULT1_FIRE5;
        savedata.keys[0][SDID_SPECIAL]   = CONTROL_DEFAULT1_FIRE6;
        savedata.keys[0][SDID_START]     = CONTROL_DEFAULT1_START;
        savedata.keys[0][SDID_SCREENSHOT] = CONTROL_DEFAULT1_SCREENSHOT;
        #ifdef SDL
            //savedata.keys[0][SDID_ESC]       = CONTROL_DEFAULT1_ESC;
        #endif

        /* *************** SET DEFAULT KEYS *************** */
        // White Dragon: These are default keys: for Android is the touchpad and for Win/Linux etc. is the keyboard
        default_keys[SDID_MOVEUP]    = CONTROL_DEFAULT1_UP;
        default_keys[SDID_MOVEDOWN]  = CONTROL_DEFAULT1_DOWN;
        default_keys[SDID_MOVELEFT]  = CONTROL_DEFAULT1_LEFT;
        default_keys[SDID_MOVERIGHT] = CONTROL_DEFAULT1_RIGHT;
        default_keys[SDID_ATTACK]    = CONTROL_DEFAULT1_FIRE1;
        default_keys[SDID_ATTACK2]   = CONTROL_DEFAULT1_FIRE2;
        default_keys[SDID_ATTACK3]   = CONTROL_DEFAULT1_FIRE3;
        default_keys[SDID_ATTACK4]   = CONTROL_DEFAULT1_FIRE4;
        default_keys[SDID_JUMP]      = CONTROL_DEFAULT1_FIRE5;
        default_keys[SDID_SPECIAL]   = CONTROL_DEFAULT1_FIRE6;
        default_keys[SDID_START]     = CONTROL_DEFAULT1_START;
        default_keys[SDID_SCREENSHOT] = CONTROL_DEFAULT1_SCREENSHOT;

        control_setkey(&default_control, FLAG_ESC,        CONTROL_ESC);
        control_setkey(&default_control, FLAG_MOVEUP,     default_keys[SDID_MOVEUP]);
        control_setkey(&default_control, FLAG_MOVEDOWN,   default_keys[SDID_MOVEDOWN]);
        control_setkey(&default_control, FLAG_MOVELEFT,   default_keys[SDID_MOVELEFT]);
        control_setkey(&default_control, FLAG_MOVERIGHT,  default_keys[SDID_MOVERIGHT]);
        control_setkey(&default_control, FLAG_ATTACK,     default_keys[SDID_ATTACK]);
        control_setkey(&default_control, FLAG_ATTACK2,    default_keys[SDID_ATTACK2]);
        control_setkey(&default_control, FLAG_ATTACK3,    default_keys[SDID_ATTACK3]);
        control_setkey(&default_control, FLAG_ATTACK4,    default_keys[SDID_ATTACK4]);
        control_setkey(&default_control, FLAG_JUMP,       default_keys[SDID_JUMP]);
        control_setkey(&default_control, FLAG_SPECIAL,    default_keys[SDID_SPECIAL]);
        control_setkey(&default_control, FLAG_START,      default_keys[SDID_START]);
        control_setkey(&default_control, FLAG_SCREENSHOT, default_keys[SDID_SCREENSHOT]);
    }
    else if (player == 1)
    {
        savedata.keys[1][SDID_MOVEUP]    = CONTROL_NONE; //Kratus (20-04-21) Used to clear all keys
        savedata.keys[1][SDID_MOVEDOWN]  = CONTROL_NONE;
        savedata.keys[1][SDID_MOVELEFT]  = CONTROL_NONE;
        savedata.keys[1][SDID_MOVERIGHT] = CONTROL_NONE;
        savedata.keys[1][SDID_ATTACK]    = CONTROL_NONE;
        savedata.keys[1][SDID_ATTACK2]   = CONTROL_NONE;
        savedata.keys[1][SDID_ATTACK3]   = CONTROL_NONE;
        savedata.keys[1][SDID_ATTACK4]   = CONTROL_NONE;
        savedata.keys[1][SDID_JUMP]      = CONTROL_NONE;
        savedata.keys[1][SDID_SPECIAL]   = CONTROL_NONE;
        savedata.keys[1][SDID_START]     = CONTROL_NONE;
        savedata.keys[1][SDID_SCREENSHOT] = CONTROL_NONE;
        #ifdef SDL
            //savedata.keys[1][SDID_ESC]       = CONTROL_DEFAULT2_ESC;
        #endif
    }
    else if (player == 2)
    {
        savedata.keys[2][SDID_MOVEUP]    = CONTROL_NONE; //Kratus (20-04-21) Used to clear all keys
        savedata.keys[2][SDID_MOVEDOWN]  = CONTROL_NONE;
        savedata.keys[2][SDID_MOVELEFT]  = CONTROL_NONE;
        savedata.keys[2][SDID_MOVERIGHT] = CONTROL_NONE;
        savedata.keys[2][SDID_ATTACK]    = CONTROL_NONE;
        savedata.keys[2][SDID_ATTACK2]   = CONTROL_NONE;
        savedata.keys[2][SDID_ATTACK3]   = CONTROL_NONE;
        savedata.keys[2][SDID_ATTACK4]   = CONTROL_NONE;
        savedata.keys[2][SDID_JUMP]      = CONTROL_NONE;
        savedata.keys[2][SDID_SPECIAL]   = CONTROL_NONE;
        savedata.keys[2][SDID_START]     = CONTROL_NONE;
        savedata.keys[2][SDID_SCREENSHOT] = CONTROL_NONE;
        #ifdef SDL
            //savedata.keys[2][SDID_ESC]       = CONTROL_DEFAULT3_ESC;
        #endif
    }
    else if (player == 3)
    {
        savedata.keys[3][SDID_MOVEUP]    = CONTROL_NONE; //Kratus (20-04-21) Used to clear all keys
        savedata.keys[3][SDID_MOVEDOWN]  = CONTROL_NONE;
        savedata.keys[3][SDID_MOVELEFT]  = CONTROL_NONE;
        savedata.keys[3][SDID_MOVERIGHT] = CONTROL_NONE;
        savedata.keys[3][SDID_ATTACK]    = CONTROL_NONE;
        savedata.keys[3][SDID_ATTACK2]   = CONTROL_NONE;
        savedata.keys[3][SDID_ATTACK3]   = CONTROL_NONE;
        savedata.keys[3][SDID_ATTACK4]   = CONTROL_NONE;
        savedata.keys[3][SDID_JUMP]      = CONTROL_NONE;
        savedata.keys[3][SDID_SPECIAL]   = CONTROL_NONE;
        savedata.keys[3][SDID_START]     = CONTROL_NONE;
        savedata.keys[3][SDID_SCREENSHOT] = CONTROL_NONE;
        #ifdef SDL
            //savedata.keys[3][SDID_ESC]       = CONTROL_DEFAULT4_ESC;
        #endif
    }
}

void clearsettings()
{
    int i = 0;

    savedata.compatibleversion = COMPATIBLEVERSION;
    savedata.gamma = 0;
    savedata.brightness = 0;
    global_config.cheats = CHEAT_OPTIONS_ALL_MENU;
    savedata.soundvol = 100; //Kratus (02-2023) Changed the default master volume
    savedata.usemusic = 1;
    savedata.musicvol = 100; //Kratus (02-2023) Changed the default music volume
    savedata.effectvol = 100; //Kratus (02-2023) Changed the default effect volume
    savedata.usejoy = 1;
    savedata.mode = 0;
    savedata.showtitles = 0;
    savedata.windowpos = 0;
    savedata.logo = 1;
    savedata.uselog = 1;
    savedata.debuginfo = 0;
    savedata.fullscreen = 0;
    savedata.fpslimit = 1; // default to vsync
    savedata.stretch = 0;
    savedata.swfilter = 0;

    #ifdef SDL
    savedata.usegl = 1;
    savedata.hwfilter = 1;
        #ifdef ANDROID
        savedata.hwscale = 0.0;
        #else
        savedata.hwscale = 1.0;
        #endif
    #endif

    #ifdef ANDROID
    savedata.is_touchpad_vibration_enabled = 0;
    #endif

    for (i = 0; i < MAX_PLAYERS; i++)
    {
        clearbuttons(i);
    }
}


void savesettings()
{
    FILE *handle = NULL;
    char path[MAX_BUFFER_LEN] = {""};
    char tmpname[MAX_BUFFER_LEN] = {""};
    getBasePath(path, "Saves", 0);
    getPakName(tmpname, 4);
    strcat(path, tmpname);
    handle = fopen(path, "wb");
    if(handle == NULL)
    {
        return;
    }
    fwrite(&savedata, 1, sizeof(savedata), handle);
    fclose(handle);
}

void saveasdefault()
{
    FILE *handle = NULL;
    char path[MAX_BUFFER_LEN] = {""};
    getBasePath(path, "Saves", 0);
    strcat(path, "default.cfg");
    handle = fopen(path, "wb");
    if(handle == NULL)
    {
        return;
    }
    fwrite(&savedata, 1, sizeof(savedata), handle);
    fclose(handle);
}


void loadsettings()
{
    FILE *handle = NULL;
    char path[MAX_BUFFER_LEN] = {""};
    char tmpname[MAX_BUFFER_LEN] = {""};
    getBasePath(path, "Saves", 0);
    getPakName(tmpname, 4);
    strcat(path, tmpname);
    if(!(fileExists(path)))
    {
        loadfromdefault();
        return;
    }
    clearsettings();
    handle = fopen(path, "rb");
    if(handle == NULL)
    {
        return;
    }
    fread(&savedata, 1, sizeof(savedata), handle);
    fclose(handle);
    if(savedata.compatibleversion != COMPATIBLEVERSION)
    {
        clearsettings();
    }
}

void loadfromdefault()
{
    FILE *handle = NULL;
    char path[MAX_BUFFER_LEN] = {""};
    getBasePath(path, "Saves", 0);
    strcat(path, "default.cfg");
    clearsettings();
    handle = fopen(path, "rb");
    if(handle == NULL)
    {
        return;
    }
    fread(&savedata, 1, sizeof(savedata), handle);
    fclose(handle);
    if(savedata.compatibleversion != COMPATIBLEVERSION)
    {
        clearsettings();
    }
}


/*
- Caskey, Damon V.
- 2026-08-11
-
- Append complete allowselect state to the fixed-record save
  file using length-prefixed values without imposing a list-size
  limit.
*/
#define SAVE_ALLOWSELECT_EXTENSION_MAGIC UINT64_C(0x5443454C45534C41)
#define SAVE_ALLOWSELECT_EXTENSION_VERSION UINT32_C(1)

static bool write_saved_allowselect_extension(FILE* handle)
{
    const uint64_t magic = SAVE_ALLOWSELECT_EXTENSION_MAGIC;
    const uint32_t version = SAVE_ALLOWSELECT_EXTENSION_VERSION;
    const uint64_t entry_count = (uint64_t)savelevel_count;
    size_t i;

    if(fwrite(&magic, sizeof(magic), 1, handle) != 1
        || fwrite(&version, sizeof(version), 1, handle) != 1
        || fwrite(&entry_count, sizeof(entry_count), 1, handle) != 1)
    {
        return false;
    }

    for(i = 0; i < savelevel_count; i++)
    {
        const char* value = get_saved_allowselect_arguments(i);
        const uint64_t length = value ? (uint64_t)strlen(value) : 0;

        if(fwrite(&length, sizeof(length), 1, handle) != 1
            || (length
                && fwrite(value, 1, (size_t)length, handle)
                    != (size_t)length))
        {
            return false;
        }
    }

    return true;
}

static bool get_file_remaining_size(FILE* handle, uint64_t* remaining)
{
    long current_position;
    long end_position;

    current_position = ftell(handle);

    if(current_position < 0 || fseek(handle, 0, SEEK_END) != 0)
    {
        return false;
    }

    end_position = ftell(handle);

    if(end_position < current_position
        || fseek(handle, current_position, SEEK_SET) != 0)
    {
        return false;
    }

    *remaining = (uint64_t)(end_position - current_position);

    return true;
}

static bool read_saved_allowselect_extension(FILE* handle)
{
    uint64_t magic;
    uint32_t version;
    uint64_t entry_count;
    char** loaded_values;
    size_t i;

    if(fread(&magic, sizeof(magic), 1, handle) != 1
        || magic != SAVE_ALLOWSELECT_EXTENSION_MAGIC)
    {
        return false;
    }

    if(fread(&version, sizeof(version), 1, handle) != 1
        || fread(&entry_count, sizeof(entry_count), 1, handle) != 1)
    {
        return false;
    }

    if(version != SAVE_ALLOWSELECT_EXTENSION_VERSION
        || entry_count != (uint64_t)savelevel_count)
    {
        return false;
    }

    loaded_values = calloc(savelevel_count, sizeof(*loaded_values));

    for(i = 0; i < savelevel_count; i++)
    {
        uint64_t length;
        uint64_t remaining;

        if(fread(&length, sizeof(length), 1, handle) != 1
            || length > (uint64_t)(SIZE_MAX - 1)
            || !get_file_remaining_size(handle, &remaining)
            || length > remaining)
        {
            goto error;
        }

        if(length)
        {
            loaded_values[i] = malloc((size_t)length + 1);

            if(fread(loaded_values[i], 1, (size_t)length, handle)
                != (size_t)length)
            {
                goto error;
            }

            loaded_values[i][(size_t)length] = '\0';
        }
    }

    for(i = 0; i < savelevel_count; i++)
    {
        set_saved_allowselect_arguments(i, loaded_values[i]);
        free(loaded_values[i]);
    }

    free(loaded_values);
    return true;

error:
    for(i = 0; i < savelevel_count; i++)
    {
        free(loaded_values[i]);
    }

    free(loaded_values);
    return false;
}



void clearSavedGame()
{
    clear_saved_allowselect_arguments();

    if(savelevel)
    {
        memset(savelevel, 0, sizeof(*savelevel) * savelevel_count);
    }
}



void clearHighScore()
{
    int i;
    savescore.compatibleversion = CV_HIGH_SCORE;
    for(i = 0; i < 10; i++)
    {
        savescore.highsc[i] = 0;    // Resets all the highscores to 0
        strcpy(savescore.hscoren[i], "None");    // Resets all the highscore names to "None"
    }
}



int saveGameFile()
{
    size_t i;
    FILE *handle = NULL;
    char path[MAX_BUFFER_LEN] = {""};
    char tmpname[MAX_BUFFER_LEN] = {""};

    getBasePath(path, "Saves", 0);
    getPakName(tmpname, 0);
    strcat(path, tmpname);
    //if(!savelevel[saveslot].level) return;
    handle = fopen(path, "wb");

    if(handle == NULL)
    {
        return 0;
    }

    if(!savelevel || savelevel_count != (size_t)num_difficulties)
    {
        fclose(handle);
        return 0;
    }

    for(i = 0; i < savelevel_count; i++)
    {
        savelevel[i].compatibleversion = CV_SAVED_GAME;
    }

    if(fwrite(savelevel, sizeof(*savelevel), savelevel_count, handle)
            != savelevel_count
        || !write_saved_allowselect_extension(handle))
    {
        fclose(handle);
        return 0;
    }

    fclose(handle);

    return 1;
}


int loadGameFile()
{
    int result = 1;
    size_t i;
    FILE *handle = NULL;
    char path[MAX_BUFFER_LEN] = {""};
    char tmpname[MAX_BUFFER_LEN] = {""};
    //size_t filesize = 0;

    getBasePath(path, "Saves", 0);
    getPakName(tmpname, 0);
    strcat(path, tmpname);
    handle = fopen(path, "rb");

    if(handle == NULL)
    {
        return 0;
    }

    if(!savelevel || savelevel_count != (size_t)num_difficulties)
    {
        fclose(handle);
        return 0;
    }

    clearSavedGame();

    //fseek(handle, 0L, SEEK_END);
    //filesize = ftell(handle);
    //fseek(handle, 0L, SEEK_SET); // or rewind(handle);
    //(filesize != sizeof(*savelevel)*num_difficulties)

    if(fread(savelevel, sizeof(*savelevel), savelevel_count, handle)
            != savelevel_count
        || (savelevel_count
            && savelevel[0].compatibleversion != CV_SAVED_GAME)
        || !read_saved_allowselect_extension(handle))
    {
        clearSavedGame();
        result = 0;
    }
    else
    {
        bonus = 0;
        for(i = 0; i < savelevel_count; i++) if(savelevel[i].times_completed > 0)
            {
                bonus += savelevel[i].times_completed;
            }
    }

    fclose(handle);

    return result;
}


int saveHighScoreFile()
{
    FILE *handle = NULL;
    char path[MAX_BUFFER_LEN] = {""};
    char tmpname[MAX_BUFFER_LEN] = {""};
    getBasePath(path, "Saves", 0);
    getPakName(tmpname, 1);
    strcat(path, tmpname);
    handle = fopen(path, "wb");
    if(handle == NULL)
    {
        return 0;
    }
    fwrite(&savescore, 1, sizeof(savescore), handle);
    fclose(handle);
    return 1;
}


int loadHighScoreFile()
{
    FILE *handle = NULL;
    char path[MAX_BUFFER_LEN] = {""};
    char tmpname[MAX_BUFFER_LEN] = {""};
    getBasePath(path, "Saves", 0);
    getPakName(tmpname, 1);
    strcat(path, tmpname);
    clearHighScore();
    handle = fopen(path, "rb");
    if(handle == NULL)
    {
        return 0;
    }
    fread(&savescore, 1, sizeof(savescore), handle);
    fclose(handle);
    if(savescore.compatibleversion != CV_HIGH_SCORE)
    {
        clearHighScore();
        return 0;
    }
    return 1;
}


static void vardump(ScriptVariant *var, char buffer[])
{
    char *tmpstr;
    int l, t, c;
    buffer[0] = 0;
    switch(var->vt)
    {

    case VT_STR:
        strcpy(buffer, "\"");
        tmpstr = StrCache_Get(var->strVal);
        l = strlen(tmpstr);
        for(c = 0; c < l; c++)
        {
            if(tmpstr[c] == '\n')
            {
                strcat(buffer, "\\n");
            }
            else if(tmpstr[c] == '\r')
            {
                strcat(buffer, "\\r");
            }
            else if(tmpstr[c] == '\\')
            {
                strcat(buffer, "\\\\");
            }
            else
            {
                t = strlen(buffer);
                buffer[t] = tmpstr[c];
                buffer[t + 1] = 0;
            }
        }
        strcat(buffer, "\"");
        break;
    case VT_DECIMAL:
        sprintf(buffer, "%lf", (double)var->dblVal);
        break;
    case VT_INTEGER:
        sprintf(buffer, "%ld", (long)var->lVal);
        break;
    default:
        strcpy(buffer, "NULL()");
        break;
    }
}

int saveScriptFile()
{
#define _writestr(v) fwrite(v, strlen(v), 1, handle);
#define _writetmp  _writestr(tmpvalue)
#define _writeconst(s) strcpy(tmpvalue,s);_writetmp
    FILE *handle = NULL;
    int i, l, size;
    ScriptVariant *var;
    char path[MAX_BUFFER_LEN] = {""};
    char tmpvalue[MAX_BUFFER_LEN] = {""};
    getBasePath(path, "Saves", 0);
    getPakName(tmpvalue, 2);//.scr
    strcat(path, tmpvalue);
    l = strlen(path); //s00, s01, s02 etc
    path[l - 2] = '0' + (current_set / 10);
    path[l - 1] = '0' + (current_set % 10);
    handle = fopen(path, "wb");
    if(handle == NULL)
    {
        return 0;
    }

    _writeconst("void main() {\n");
    size = List_GetSize(global_var_list.list);
    for(i = 0, List_Reset(global_var_list.list); i < size; List_GotoNext(global_var_list.list), i++)
    {
        var = (ScriptVariant *)List_Retrieve(global_var_list.list);
        if( var->vt != VT_EMPTY && var->vt != VT_PTR)
        {
            _writeconst("\tsetglobalvar(\"")
            _writestr(List_GetName(global_var_list.list))
            _writeconst("\",")
            vardump(var, tmpvalue);
            _writetmp
            _writeconst(");\n")
        }
    }
    // indexed list
    for(i = 1; i <= global_var_list.vars->lVal; i++)
    {
        if(global_var_list.vars[i].vt != VT_PTR && global_var_list.vars[i].vt != VT_EMPTY)
        {
            _writeconst("\tsetglobalvar(")
            sprintf(tmpvalue, "%d", i - 1);
            _writetmp
            _writeconst(",")
            vardump(global_var_list.vars + i, tmpvalue);
            _writetmp
            _writeconst(");\n")
        }
    }
    //allow select
    for(i = 0; i < models_cached; i++)
    {
        if(model_cache[i].selectable)
        {
            _writeconst("\tchangemodelproperty(\"")
            _writestr(model_cache[i].name)
            _writeconst("\",4,1);\n")
        }
        /*
        if(model_cache[i].model) {
        	_writeconst("\tloadmodel(\"")
        	_writestr(model_cache[i].name)
        	sprintf(tmpvalue, "\",%d,%d);\n", model_cache[i].model->unload, model_cache[i].selectable);
        	_writetmp
        }*/
    }
    _writeconst("}\n");

    fclose(handle);
    return 1;
#undef _writestr
#undef _writetmp
#undef _writeconst
}


int loadScriptFile()
{
    Script script;
    int result = 0;
    char *buf = NULL;
    ptrdiff_t l;
    size_t len;
    FILE *handle = NULL;

    char path[MAX_BUFFER_LEN] = {""};
    char tmpname[MAX_BUFFER_LEN] = {""};
    getBasePath(path, "Saves", 0);
    getPakName(tmpname, 2);//.scr
    strcat(path, tmpname);
    l = strlen(path); //s00, s01, s02 etc
    path[l - 2] = '0' + (current_set / 10);
    path[l - 1] = '0' + (current_set % 10);

    handle = fopen(path, "rb");
    if(handle == NULL)
    {
        return 0;
    }

    fseek(handle, 0, SEEK_END);
    len = ftell(handle);
    fseek(handle, 0, SEEK_SET);
    buf = malloc(len + 1);

    if(!buf)
    {
        return 0;
    }

    fread(buf, 1, len, handle);
    buf[len - 1] = 0;

    Script_Init(&script, "loadScriptFile",  NULL, 1);

    result = (Script_AppendText(&script, buf, path) &&
              Script_Compile(&script) &&
              Script_Execute(&script) );

    Script_Clear(&script, 2);
    free(buf);
    return result;
}

// ----------------------- Sound ------------------------------

int music(char *filename, int loop, long offset)
{
    char t[64];
    char a[64];
    int res = 1;

    if(!savedata.usemusic)
    {
        return 0;
    }
    if(!sound_open_music(filename, packfile, savedata.musicvol, loop, offset))
    {
        printf("\nCan't play music file '%s'\n", filename);
        res = 0;
    }
    if(savedata.showtitles && sound_query_music(a, t))
    {
        debug_xy_msg.font_index = 0;
        //debug_xy_msg.x = videomodes.hRes/2 - videomodes.hShift - (fontmonowidth(debug_xy_msg.font_index)*16);
        //debug_xy_msg.y = videomodes.vRes - videomodes.vShift - fontheight(debug_xy_msg.font_index);
        debug_xy_msg.x = fontmonowidth(debug_xy_msg.font_index);
        debug_xy_msg.y = videomodes.vRes - fontheight(debug_xy_msg.font_index)*2;

        if(a[0] && t[0])
        {
            debug_printf("%s \"%s\" %s %s", Tr("Playing"), t, Tr("by"), a);
        }
        else if(a[0])
        {
            debug_printf("%s %s", Tr("Playing unknown song by"), a);
        }
        else if(t[0])
        {
            debug_printf("%s \"%s\" %s", Tr("Playing"), t, Tr("by unknown artist"));
        }
        else
        {
            debug_printf("%s", Tr("Playing unknown song by unknown artist"));
        }
    }
    strncpy(currentmusic, filename, sizeof(currentmusic) - 1);
    return res;
}

void check_music()
{
    if(musicfade[1] > 0)
    {
        musicfade[1] -= musicfade[0];
        sound_volume_music((int)musicfade[1], (int)musicfade[1]);
    }
    else if(musicname[0])
    {
        music(musicname, musicloop, musicoffset);
        sound_volume_music(savedata.musicvol, savedata.musicvol);
        musicname[0] = 0;
    }
}

// ----------------------- General ------------------------------
// atof and atoi return a valid number, if only the first char is one.
// so we only check that.
int isNumeric(const char *text)
{
    const char *p = text;
    assert(p);
    if(!*p)
    {
        return 0;
    }
    switch(*p)
    {
    case '-':
    case '+':
        p++;
        break;
    default:
        break;
    }
    switch (*p)
    {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return 1;
    default:
        return 0;
    }
    return 1;
}


int64_t getValidInt(const char *text, const char *file, const char *cmd) {
    const char *WARN_NUMBER_EXPECTED = "WARNING: %s tries to load a non-numeric value at %s, where a number is expected!\nerroneus string: %s\n";
    if(!text || !*text)
    {
        return 0;
    }

    if(isNumeric(text)) {
        return atoll(text);
    
    } else {
        printf(WARN_NUMBER_EXPECTED, file, cmd, text);
        return 0;
    }

}

float getValidFloat(const char *text, const char *file, const char *cmd)
{
    const char *WARN_NUMBER_EXPECTED = "WARNING: %s tries to load a non-numeric value at %s, where a number is expected!\nerroneus string: %s\n";
    if(!text || !*text)
    {
        return 0.0f;
    }
    if(isNumeric(text))
    {
        if(text[strlen(text) - 1] == '%')
        {
            return atof(text) / 100.0f;
        }
        return atof(text);
    }
    else
    {
        printf(WARN_NUMBER_EXPECTED, file, cmd, text);
        return 0.0f;
    }
}

/*
* Non-owning view of a token in a command line.
*
* Text is not null terminated. Length identifies
* the complete token.
*/
typedef struct s_command_token
{
    const char* text;
    size_t length;
    size_t value_length;
} s_command_token;

/*
* Sequential reader for tokens in a command line.
*
* Cursor points directly into the original file
* buffer. No token collection or copy is required.
*/
typedef struct s_command_token_reader
{
    const char* cursor;
    char unterminated_quote;
} s_command_token_reader;

/*
- Caskey, Damon V.
- 2026-08-11
-
- Identify command quote delimiters and update quote state.
  Double quotes may group text anywhere within an argument.
  Single quotes may begin grouping only at the argument boundary,
  allowing apostrophes in ordinary words to remain literal.
*/
static bool command_token_update_quote_state(
    const char* token_start,
    const char* cursor,
    bool* inside_double_quotes,
    bool* inside_single_quotes
) {
    bool escaped;

    assert(token_start);
    assert(cursor);
    assert(inside_double_quotes);
    assert(inside_single_quotes);

    escaped =
        cursor > token_start
        && cursor[-1] == '\\';

    if(*cursor == '"' && !escaped && !*inside_single_quotes) {
        *inside_double_quotes = !*inside_double_quotes;
        return true;
    }

    if(*cursor == '\'' && !escaped && !*inside_double_quotes) {
        if(*inside_single_quotes || cursor == token_start) {
            *inside_single_quotes = !*inside_single_quotes;
            return true;
        }
    }

    return false;
}

/*
* Read the next token from a command line.
*
* Tokens end at whitespace, a line ending, a comment
* marker, or the null terminator. Quoted text may contain
* whitespace, line endings, and comment markers. Double
* quotes may open anywhere in an argument. Single quotes
* may open only at its beginning so ordinary apostrophes
* remain literal. Matching delimiters are omitted from the
* logical value, while the opposite quote type is literal.
*
* Return true when a token is available. Return false
* when the command line has no remaining tokens or the
* current token contains an unterminated quote. The reader
* records the invalid delimiter for callers that distinguish
* malformed input from an ordinary end.
*/
static bool command_token_reader_next(s_command_token_reader* reader, s_command_token* token) {
    const char* cursor;
    const char* token_start;

    size_t value_length = 0;

    bool inside_double_quotes = false;
    bool inside_single_quotes = false;

    assert(reader);
    assert(token);

    cursor = reader->cursor;

    /*
    * Skip whitespace before the next token.
    */
    while(*cursor == ' ' || *cursor == '\t') {
        cursor++;
    }

    /*
    * Stop at the end of the command line or the
    * beginning of a comment.
    */
    if(*cursor == '\0'
        || *cursor == '\r'
        || *cursor == '\n'
        || *cursor == '#') {
        reader->cursor = cursor;
        token->text = NULL;
        token->length = 0;
        token->value_length = 0;

        return false;
    }

    token_start = cursor;

    while(*cursor) {
        if(command_token_update_quote_state(
                token_start,
                cursor,
                &inside_double_quotes,
                &inside_single_quotes
            )) {
            cursor++;
            continue;
        }

        if(!inside_double_quotes && !inside_single_quotes) {
            if(*cursor == ' '
                || *cursor == '\t'
                || *cursor == '\r'
                || *cursor == '\n'
                || *cursor == '#') {
                break;
            }
        }

        value_length++;
        cursor++;
    }

    if(inside_double_quotes || inside_single_quotes) {
        reader->cursor = cursor;
        reader->unterminated_quote = inside_double_quotes ? '"' : '\'';
        token->text = NULL;
        token->length = 0;
        token->value_length = 0;

        return false;
    }

    token->text = token_start;
    token->length = (size_t)(cursor - token_start);
    token->value_length = value_length;
    reader->cursor = cursor;

    return true;
}

/*
- Caskey, Damon V.
- 2026-08-11
-
- Copy a command token while discarding quote delimiters.
  The destination receives the logical argument text and
  is always null terminated on success.
*/
static bool command_token_copy_value(
    const s_command_token* token,
    char* destination,
    const size_t capacity
) {
    const char* cursor;
    const char* source_end;

    size_t destination_index = 0;

    bool inside_double_quotes = false;
    bool inside_single_quotes = false;

    assert(token);
    assert(destination);

    if(!token->text) {
        return false;
    }

    source_end = token->text + token->length;

    if(capacity <= token->value_length) {
        return false;
    }

    if(token->length == token->value_length) {
        memcpy(destination, token->text, token->length);
        destination[token->length] = '\0';
        return true;
    }

    for(cursor = token->text; cursor < source_end; cursor++) {
        if(command_token_update_quote_state(
                token->text,
                cursor,
                &inside_double_quotes,
                &inside_single_quotes
            )) {
            continue;
        }

        destination[destination_index++] = *cursor;
    }

    assert(!inside_double_quotes);
    assert(!inside_single_quotes);
    assert(destination_index == token->value_length);

    destination[destination_index] = '\0';

    return true;
}

/*
- Caskey, Damon V.
- 2026-08-11
-
- Read one requested command argument sequentially from the
  source line. Return its non-owning source view and decoded
  length so the caller can allocate only the required storage.
  Distinguish malformed quoting from a missing argument.
*/
e_command_argument_read_result command_argument_read(
    const char* command_line,
    size_t argument_index,
    s_command_argument_view* argument
) {
    s_command_token_reader reader = {
        .cursor = command_line
    };

    s_command_token token;

    assert(command_line);
    assert(argument);

    *argument = (s_command_argument_view){0};

    while(argument_index) {
        if(!command_token_reader_next(&reader, &token)) {
            return reader.unterminated_quote
                ? COMMAND_ARGUMENT_READ_INVALID
                : COMMAND_ARGUMENT_READ_END;
        }

        argument_index--;
    }

    if(!command_token_reader_next(&reader, &token)) {
        return reader.unterminated_quote
            ? COMMAND_ARGUMENT_READ_INVALID
            : COMMAND_ARGUMENT_READ_END;
    }

    argument->source = token.text;
    argument->source_length = token.length;
    argument->length = token.value_length;

    return COMMAND_ARGUMENT_READ_SUCCESS;
}

/*
- Caskey, Damon V.
- 2026-08-11
-
- Copy a previously read command argument into caller-owned
  storage while discarding its opening and closing quote
  delimiters. Preserve literal apostrophes and opposite quotes.
*/
bool command_argument_copy(
    const s_command_argument_view* argument,
    char* destination,
    const size_t capacity
) {
    s_command_token token;

    assert(argument);
    assert(destination);

    token = (s_command_token){
        .text = argument->source,
        .length = argument->source_length,
        .value_length = argument->length
    };

    return command_token_copy_value(&token, destination, capacity);
}

/*
- Caskey, Damon V.
- 2026-08-11
-
- Reserve fixed storage for one sequentially read command
  argument. Keep this independent from the legacy command-line,
  script file-stream, path, and persistent save-field limit.
*/
#define MAX_COMMAND_ARGUMENT_LEN 512

/*
- Caskey, Damon V.
- 2026-08-11
-
- Maintain sequential command argument reading state. The
  underlying token reader walks the source line directly,
  while one fixed scratch buffer holds only the current item.
*/
typedef struct s_command_argument_reader
{
    s_command_token_reader token_reader;
    char value[MAX_COMMAND_ARGUMENT_LEN];
} s_command_argument_reader;

/*
- Caskey, Damon V.
- 2026-08-11
-
- Initialize a command argument reader at the requested
  argument index. Skip preceding items directly in the
  source line without collecting them into a buffer.
*/
static bool command_argument_reader_initialize(
    s_command_argument_reader* reader,
    const char* command_line,
    size_t argument_index
) {
    s_command_token skipped_token;

    assert(reader);
    assert(command_line);

    *reader = (s_command_argument_reader){
        .token_reader = {
            .cursor = command_line
        }
    };

    while(argument_index) {
        if(!command_token_reader_next(
                &reader->token_reader,
                &skipped_token
            )) {
            if(reader->token_reader.unterminated_quote) {
                borShutdown(
                    1,
                    "Command argument has an unterminated %c quote.\n",
                    reader->token_reader.unterminated_quote
                );
            }

            return false;
        }

        argument_index--;
    }

    return true;
}

/*
- Caskey, Damon V.
- 2026-08-11
-
- Read the next command argument into the reader's reusable
  fixed buffer. Enforce the dedicated per-item length limit
  without imposing whole-line or argument-count limits.
*/
static bool command_argument_reader_next(
    s_command_argument_reader* reader,
    const char** value
) {
    s_command_token token;

    assert(reader);
    assert(value);

    if(!command_token_reader_next(&reader->token_reader, &token)) {
        if(reader->token_reader.unterminated_quote) {
            borShutdown(
                1,
                "Command argument has an unterminated %c quote.\n",
                reader->token_reader.unterminated_quote
            );
        }

        *value = NULL;
        return false;
    }

    if(token.value_length >= sizeof(reader->value)) {
        borShutdown(
            1,
            "Command argument exceeds the maximum length of %zu characters.\n",
            sizeof(reader->value) - 1
        );
        *value = NULL;
        return false;
    }

    if(!command_token_copy_value(
            &token,
            reader->value,
            sizeof(reader->value)
        )) {
        *value = NULL;
        return false;
    }

    *value = reader->value;

    return true;
}

static void clear_saved_allowselect_arguments(void)
{
    size_t i;

    if(!savelevel_allowselect_args) {
        return;
    }

    for(i = 0; i < savelevel_count; i++) {
        free(savelevel_allowselect_args[i]);
        savelevel_allowselect_args[i] = NULL;
    }
}

static const char* get_saved_allowselect_arguments(size_t index)
{
    if(index >= savelevel_count || !savelevel_allowselect_args) {
        return NULL;
    }

    return savelevel_allowselect_args[index];
}

static void set_saved_allowselect_arguments(
    size_t index,
    const char* source
) {
    char* owned_source = NULL;

    if(index >= savelevel_count || !savelevel_allowselect_args) {
        return;
    }

    if(source && source[0]) {
        const size_t length = strlen(source);

        if(length == SIZE_MAX) {
            borShutdown(1, "Allowselect state exceeds addressable memory.\n");
            return;
        }

        owned_source = malloc(length + 1);
        memcpy(owned_source, source, length + 1);
    }

    free(savelevel_allowselect_args[index]);
    savelevel_allowselect_args[index] = owned_source;
}

/*
* Compare a command token with a null-terminated
* string without regard to letter case.
*/
static bool command_token_equals(const s_command_token* token, const char* expected) {
    size_t index;
    const size_t expected_length = strlen(expected);

    assert(token);
    assert(expected);

    if(token->length != expected_length) {
        return false;
    }

    for(index = 0; index < token->length; index++) {
        const unsigned char token_character =
            (unsigned char)token->text[index];

        const unsigned char expected_character =
            (unsigned char)expected[index];

        if(tolower(token_character)
            != tolower(expected_character)) {
            return false;
        }
    }

    return true;
}

/*
* Caskey, Damon V.
* 2026-07-16
*
* Convert a complete command token to a signed
* 64-bit integer.
*
* Return true when the token contains only an
* optional sign followed by decimal digits and
* the result fits within the int64_t range.
*
* Return false when the token is empty, malformed,
* or outside the int64_t range.
*/
static bool command_token_get_int64(const s_command_token* token, int64_t* result) {
    size_t index = 0;

    bool negative = false;

    uint64_t magnitude = 0;
    uint64_t magnitude_limit;

    const uint64_t negative_limit = (uint64_t)INT64_MAX + UINT64_C(1);

    assert(token);
    assert(result);

    if(!token->text || token->length == 0) {
        return false;
    }

    /*
    * Read an optional leading sign.
    */
    if(token->text[index] == '+'
        || token->text[index] == '-') {
        negative = token->text[index] == '-';
        index++;

        /*
        * A sign by itself is not an integer.
        */
        if(index >= token->length) {
            return false;
        }
    }

    /*
    * INT64_MIN has a magnitude one greater than
    * INT64_MAX, so negative values receive the
    * larger magnitude limit.
    */
    magnitude_limit = negative
        ? negative_limit
        : (uint64_t)INT64_MAX;

    for(; index < token->length; index++) {
        const unsigned char character =
            (unsigned char)token->text[index];

        uint64_t digit;

        if(character < '0' || character > '9') {
            return false;
        }

        digit = (uint64_t)(character - '0');

        /*
        * Test before multiplying so malformed input
        * cannot overflow the unsigned accumulator.
        */
        if(magnitude > (magnitude_limit - digit) / UINT64_C(10)) {
            return false;
        }

        magnitude =
            magnitude * UINT64_C(10) + digit;
    }

    if(negative) {
        /*
        * INT64_MIN cannot be produced by negating
        * INT64_MAX + 1 as a signed value, so handle
        * that exact magnitude directly.
        */
        if(magnitude == negative_limit) {
            *result = INT64_MIN;
        } else {
            *result = -(int64_t)magnitude;
        }
    } else {
        *result = (int64_t)magnitude;
    }

    return true;
}

/*
* Caskey, Damon V.
* 2026-07-17
*
* Convert a complete command token to an unsigned
* 64-bit integer.
*
* Hold durations accept decimal digits only. Testing
* before multiplication prevents the accumulator from
* overflowing when a malformed value is too large.
*/
static bool command_token_get_uint64(const s_command_token* token, uint64_t* result) {
    size_t index;

    uint64_t value = 0;

    assert(token);
    assert(result);

    if(!token->text || token->length == 0) {
        return false;
    }

    for(index = 0; index < token->length; index++) {
        const unsigned char character =
            (unsigned char)token->text[index];

        uint64_t digit;

        if(character < '0' || character > '9') {
            return false;
        }

        digit = (uint64_t)(character - '0');

        if(value > (UINT64_MAX - digit) / UINT64_C(10)) {
            return false;
        }

        value = value * UINT64_C(10) + digit;
    }

    *result = value;

    return true;
}

/*
* Caskey, Damon V.
* 2026-08-20
*
* Convert a null-terminated command argument to a
* signed 64-bit integer using the bounded token parser.
*/
static bool command_argument_get_int64(const char* argument, int64_t* result) {
    s_command_token token;

    assert(argument);
    assert(result);

    token.text = argument;
    token.length = strlen(argument);

    return command_token_get_int64(&token, result);
}

/*
* Caskey, Damon V.
* 2026-08-06
*
* Convert a delay unit name to its internal constant. Model
* frame delays may select the models.txt global mode, while
* the models.txt setting itself must resolve to a concrete
* unit.
*/
static const char* delay_unit_from_text(
    const char* unit_text,
    const bool allow_global,
    e_delay_unit* unit
) {
    assert(unit);

    if(!unit_text || !unit_text[0]) {
        return allow_global
            ? "Delay unit requires a value."
            : "Delay unit requires a value.";
    }

    if(stricmp(unit_text, "global") == 0) {
        if(!allow_global) {
            return "Global delay unit must be 'centisecond', 'millisecond', 'second', 'minute', or 'direct'.";
        }

        *unit = DELAY_UNIT_GLOBAL;
    } else if(stricmp(unit_text, "centisecond") == 0) {
        *unit = DELAY_UNIT_CENTISECOND;
    } else if(stricmp(unit_text, "millisecond") == 0) {
        *unit = DELAY_UNIT_MILLISECOND;
    } else if(stricmp(unit_text, "second") == 0) {
        *unit = DELAY_UNIT_SECOND;
    } else if(stricmp(unit_text, "minute") == 0) {
        *unit = DELAY_UNIT_MINUTE;
    } else if(stricmp(unit_text, "direct") == 0) {
        *unit = DELAY_UNIT_DIRECT;
    } else {
        return allow_global
            ? "Delay unit must be 'global', 'centisecond', 'millisecond', 'second', 'minute', or 'direct'."
            : "Global delay unit must be 'centisecond', 'millisecond', 'second', 'minute', or 'direct'.";
    }

    return NULL;
}

/*
* Caskey, Damon V.
* 2026-08-06
*
* Parse an animation frame delay and its optional input
* unit. Negative numeric values and the symbolic infinite
* forms normalize to DELAY_INFINITE. Finite positive values
* must fit the reserved 32-bit delay range; the bit-63 flag
* value is accepted as the explicit infinite representation.
*/
static const char* command_token_get_delay(
    const char* value_text,
    const char* unit_text,
    uint64_t* result,
    e_delay_unit* unit
) {
    size_t index;

    s_command_token numeric_token;
    s_command_token value_token;

    uint64_t parsed_value;

    bool negative = false;

    assert(result);
    assert(unit);

    if(!value_text || !value_text[0]) {
        return "Delay requires a value.";
    }

    value_token.text = value_text;
    value_token.length = strlen(value_text);

    if(command_token_equals(&value_token, "\xE2\x88\x9E")
        || command_token_equals(&value_token, "infinite")) {
        *result = DELAY_INFINITE;
    } else {
        numeric_token = value_token;

        if(numeric_token.text[0] == '+'
            || numeric_token.text[0] == '-') {
            negative = numeric_token.text[0] == '-';
            numeric_token.text++;
            numeric_token.length--;
        }

        if(numeric_token.length == 0) {
            return "Delay must be an integer, '\xE2\x88\x9E', or 'infinite'.";
        }

        if(negative) {
            /*
            * Magnitude is irrelevant once a delay is negative,
            * but every remaining character must still be numeric.
            */
            for(index = 0; index < numeric_token.length; index++) {
                if(numeric_token.text[index] < '0'
                    || numeric_token.text[index] > '9') {
                    return "Delay must be an integer, '\xE2\x88\x9E', or 'infinite'.";
                }
            }

            *result = DELAY_INFINITE;
        } else {
            if(!command_token_get_uint64(&numeric_token, &parsed_value)) {
                return "Delay must fit the unsigned 64-bit integer range.";
            }

            if(parsed_value == DELAY_INFINITE) {
                *result = DELAY_INFINITE;
            } else if(parsed_value > DELAY_FINITE_MAX) {
                return "Finite delay must not exceed the 32-bit delay range.";
            } else {
                *result = parsed_value;
            }
        }
    }

    if(!unit_text || !unit_text[0]) {
        *unit = DELAY_UNIT_GLOBAL;

        return NULL;
    }

    return delay_unit_from_text(unit_text, true, unit);
}

/*
* Read the next command token and convert it to an
* integer.
*
* Return true when the next token is a complete,
* valid integer. Return false when the token is
* missing or is not an integer.
*/
static bool command_token_reader_next_int64(s_command_token_reader* reader, int64_t* result) {
    s_command_token token;

    assert(reader);
    assert(result);

    if(!command_token_reader_next(reader, &token)) {
        return false;
    }

    return command_token_get_int64(&token, result);
}

/*
* Caskey, Damon V.
* 2026-07-18
*
* Parse and validate a numbered freespecial animation
* token without atoi(), atoll(), or signed overflow.
*
* Recognized distinguishes unrelated animation or input
* names from malformed freespecial names. The unnumbered
* legacy spelling "freespecial" selects freespecial1.
*
* Return NULL when the token is unrelated or valid.
* Return error_buffer when a recognized freespecial name
* is malformed or exceeds the configured animation table.
*/
static const char* command_token_get_freespecial_number(
    const s_command_token* token,
    bool* recognized,
    int* result,
    char* error_buffer,
    const size_t error_buffer_size
) {
    static const char prefix[] = "freespecial";

    const size_t prefix_length = sizeof(prefix) - 1;

    s_command_token prefix_token;
    s_command_token suffix_token;

    uint64_t parsed_number;

    assert(token);
    assert(recognized);
    assert(result);
    assert(error_buffer);
    assert(error_buffer_size);

    *recognized = false;
    *result = 0;

    if(token->length < prefix_length) {
        return NULL;
    }

    prefix_token.text = token->text;
    prefix_token.length = prefix_length;

    if(!command_token_equals(&prefix_token, prefix)) {
        return NULL;
    }

    *recognized = true;

    /*
    * Preserve legacy behavior where an omitted suffix
    * means freespecial1.
    */
    if(token->length == prefix_length) {
        *result = 1;
        return NULL;
    }

    suffix_token.text = token->text + prefix_length;
    suffix_token.length = token->length - prefix_length;

    if(suffix_token.text[0] < '1'
        || suffix_token.text[0] > '9') {
        snprintf(
            error_buffer,
            error_buffer_size,
            "Freespecial animation number must be at least 1."
        );

        return error_buffer;
    }

    if(!command_token_get_uint64(&suffix_token, &parsed_number)) {
        snprintf(
            error_buffer,
            error_buffer_size,
            "Freespecial animation number is malformed or exceeds "
            "the unsigned 64-bit range."
        );

        return error_buffer;
    }

    if(parsed_number > (uint64_t)max_freespecials) {
        snprintf(
            error_buffer,
            error_buffer_size,
            "Freespecial animation %" PRIu64 " exceeds maxfreespecials %d.\n"
            "Increase maxfreespecials in data/models.txt.",
            parsed_number,
            max_freespecials
        );

        return error_buffer;
    }

    *result = (int)parsed_number;

    return NULL;
}

/*
* Convert a command-sequence token into its input flag.
*
* Return true when the token names a recognized input.
* Return false when it is not an input token.
*/
static bool command_token_get_input_flag(const s_command_token* token, e_key_def* result) {
    static const struct {
        const char* name;
        e_key_def flag;
    } input_map[] = {
        {"l",  FLAG_MOVELEFT},
        {"r",  FLAG_MOVERIGHT},
        {"u",  FLAG_MOVEUP},
        {"d",  FLAG_MOVEDOWN},
        {"f",  FLAG_FORWARD},
        {"b",  FLAG_BACKWARD},
        {"a",  FLAG_ATTACK},
        {"a1", FLAG_ATTACK},
        {"a2", FLAG_ATTACK2},
        {"a3", FLAG_ATTACK3},
        {"a4", FLAG_ATTACK4},
        {"j",  FLAG_JUMP},
        {"st", FLAG_START},
        {"sc", FLAG_SCREENSHOT},
        {"s",  FLAG_SPECIAL},
        {"k",  FLAG_SPECIAL}
    };

    size_t index;

    assert(token);
    assert(result);

    for(index = 0;
        index < sizeof(input_map) / sizeof(input_map[0]);
        index++) {
        if(command_token_equals(token, input_map[index].name)) {
            *result = input_map[index].flag;
            return true;
        }
    }

    return false;
}

/*
* Caskey, Damon V.
* 2026-07-17
*
* Parse one configurable special-command input token.
*
* Supported forms:
*
* key             Positive-edge press.
* ~key            Negative-edge release.
* key[min]        Held-state requirement with an
*                 inclusive minimum duration.
* key[min][max]   Held-state requirement with inclusive
*                 minimum and maximum durations. A zero
*                 maximum means no upper bound.
* *key[min]       Automatic positive edge generated once
*                 when the held duration reaches min.
* key*[min]       Compatible alternate spelling.
*
* Return NULL on success or a static error message when
* the complete token is malformed.
*/
static const char* command_token_get_input_requirement(
    const s_command_token* token,
    s_command_input_step* result
) {
    s_command_token input_name;
    s_command_token hold_time_minimum_token;
    s_command_token hold_time_maximum_token;

    e_key_def input_flag;

    size_t hold_minimum_open_index = 0;
    size_t hold_minimum_close_index = 0;
    size_t hold_maximum_open_index = 0;
    size_t input_name_start = 0;
    size_t input_name_length;
    size_t index;

    bool automatic_hold = false;

    uint64_t hold_time_minimum;
    uint64_t hold_time_maximum = 0;

    assert(token);
    assert(result);

    memset(result, 0, sizeof(*result));

    if(!token->text || token->length == 0) {
        return "Empty input token in special command";
    }

    /*
    * A leading tilde marks a negative-edge input.
    * Release and hold modifiers remain separate tokens,
    * as in a[50] + ~a.
    */
    if(token->text[0] == '~') {
        if(token->length == 1) {
            return "Release input is missing a key name";
        }

        input_name.text = token->text + 1;
        input_name.length = token->length - 1;

        for(index = 0; index < input_name.length; index++) {
            if(input_name.text[index] == '['
                || input_name.text[index] == ']'
                || input_name.text[index] == '*'
                || input_name.text[index] == '~') {
                return "Release input cannot contain a hold modifier";
            }
        }

        if(!command_token_get_input_flag(&input_name, &input_flag)) {
            return "Invalid release input token in special command";
        }

        result->release = (key_mask_t)input_flag;

        return NULL;
    }

    /*
    * A closing bracket makes the token a held
    * requirement. The first bracket is the minimum. One
    * optional second bracket is the inclusive maximum.
    */
    if(token->text[token->length - 1] == ']') {
        for(index = 0; index < token->length; index++) {
            if(token->text[index] == '[') {
                hold_minimum_open_index = index;
                break;
            }
        }

        if(!hold_minimum_open_index) {
            return "Held input is missing a key name or opening bracket";
        }

        for(index = hold_minimum_open_index + 1;
            index < token->length;
            index++) {

            if(token->text[index] == ']') {
                hold_minimum_close_index = index;
                break;
            }
        }

        if(!hold_minimum_close_index) {
            return "Held input minimum time is missing a closing bracket";
        }

        if(hold_minimum_close_index < token->length - 1) {
            hold_maximum_open_index =
                hold_minimum_close_index + 1;

            if(token->text[hold_maximum_open_index] != '[') {
                return "Held input maximum time must immediately follow the minimum";
            }
        }

        input_name_length = hold_minimum_open_index;

        /*
        * Accept the automatic modifier before the key,
        * as in *a[600]. Keep the earlier a*[600] form as
        * a compatible alias so existing definitions do
        * not need conversion.
        */
        if(token->text[0] == '*') {
            automatic_hold = true;
            input_name_start = 1;
            input_name_length--;

            if(!input_name_length) {
                return "Automatic held input is missing a key name";
            }
        }

        if(token->text[hold_minimum_open_index - 1] == '*') {
            if(automatic_hold) {
                return "Automatic held input has more than one trigger modifier";
            }

            automatic_hold = true;
            input_name_length--;

            if(!input_name_length) {
                return "Automatic held input is missing a key name";
            }
        }

        input_name.text = token->text + input_name_start;
        input_name.length = input_name_length;

        hold_time_minimum_token.text =
            token->text + hold_minimum_open_index + 1;

        hold_time_minimum_token.length =
            hold_minimum_close_index
            - hold_minimum_open_index - 1;

        if(hold_maximum_open_index) {
            hold_time_maximum_token.text =
                token->text + hold_maximum_open_index + 1;

            hold_time_maximum_token.length =
                token->length - hold_maximum_open_index - 2;
        }

        if(!command_token_get_input_flag(&input_name, &input_flag)) {
            return "Invalid held input token in special command";
        }

        if(!command_token_get_uint64(
            &hold_time_minimum_token,
            &hold_time_minimum
        )) {
            return "Held input minimum time must be an unsigned integer";
        }

        if(hold_maximum_open_index
            && !command_token_get_uint64(
                &hold_time_maximum_token,
                &hold_time_maximum
            )) {
            return "Held input maximum time must be an unsigned integer";
        }

        if(hold_time_maximum
            && hold_time_maximum < hold_time_minimum) {
            return "Held input maximum time must be zero or at least the minimum";
        }

        if(automatic_hold) {
            result->hold_trigger = (key_mask_t)input_flag;

        } else {
            result->hold = (key_mask_t)input_flag;
        }

        result->hold_time = hold_time_minimum;
        result->hold_time_maximum = hold_time_maximum;

        return NULL;
    }

    /*
    * Brackets, tildes, and stars are only valid in the
    * complete decorated forms handled above.
    */
    for(index = 0; index < token->length; index++) {
        if(token->text[index] == '['
            || token->text[index] == ']'
            || token->text[index] == '*'
            || token->text[index] == '~') {
            return "Malformed input modifier in special command";
        }
    }

    input_name = *token;

    if(!command_token_get_input_flag(&input_name, &input_flag)) {
        return "Invalid input token in special command";
    }

    result->press = (key_mask_t)input_flag;

    return NULL;
}

/*
* Caskey, Damon V.
* 2026-07-18
*
* Parse the optional grace modifier for a plain press
* chord. The complete token form is +[time].
*
* Recognized distinguishes a non-modifier token from a
* malformed modifier. Return NULL when the token is not
* a modifier or when parsing succeeds. Return a static
* error message when a modifier begins with +[ but does
* not contain one unsigned tick value.
*/
static const char* command_token_get_chord_time_modifier(
    const s_command_token* token,
    bool* recognized,
    uint64_t* result
) {
    s_command_token chord_time_token;

    assert(token);
    assert(recognized);
    assert(result);

    *recognized = false;
    *result = 0;

    if(token->length < 2
        || token->text[0] != '+'
        || token->text[1] != '[') {
        return NULL;
    }

    *recognized = true;

    if(token->length < 4
        || token->text[token->length - 1] != ']') {
        return "Chord grace time must use +[time]";
    }

    chord_time_token.text = token->text + 2;
    chord_time_token.length = token->length - 3;

    if(!command_token_get_uint64(&chord_time_token, result)) {
        return "Chord grace time must be an unsigned integer";
    }

    return NULL;
}

/*
* Caskey, Damon V.
* 2026-07-18
*
* Parse the optional grace modifier for a complete
* command sequence. The complete token form is
* :[time].
*
* Recognized distinguishes a non-modifier token from a
* malformed modifier. Return NULL when the token is not
* a modifier or when parsing succeeds. Return a static
* error message when a modifier begins with :[ but does
* not contain one unsigned logical tick value.
*/
static const char* command_token_get_sequence_grace_time_modifier(const s_command_token* token, bool* recognized, uint64_t* result) {
    
    s_command_token sequence_grace_time_token;

    assert(token);
    assert(recognized);
    assert(result);

    *recognized = false;
    *result = 0;

    if(token->length < 2
        || token->text[0] != ':'
        || token->text[1] != '[') {
        return NULL;
    }

    *recognized = true;

    if(token->length < 4
        || token->text[token->length - 1] != ']') {
        return "Sequence grace time must use :[time]";
    }

    sequence_grace_time_token.text = token->text + 2;
    sequence_grace_time_token.length = token->length - 3;

    if(!command_token_get_uint64(&sequence_grace_time_token, result)) {
        return "Sequence grace time must be an unsigned integer";
    }

    return NULL;
}

/*
* Parse the input-sequence portion of a special command.
*
* Sequence grammar:
*
* input [-> input ...] freespecial#
* input + input [+[time]] [-> input ...] freespecial#
*
* The arrow token is an optional visual separator.
* The plus token combines the following input with the
* preceding sequence step. The +[time] token overrides
* same-tick sensitivity for the preceding press chord.
* The :[time] token overrides the global grace time
* between every step in this command sequence. It may
* appear once in any token position outside an unfinished
* plus expression.
*
* Return NULL on success. Return a static error message
* when the sequence is invalid.
*/
static const char* special_command_parse_sequence(
    s_command_token_reader* reader,
    s_com* special,
    int* freespecial_number,
    char* error_buffer,
    const size_t error_buffer_size
) {
    s_command_token token;

    s_command_input_step input_requirement;
    s_command_input_step* destination_step;

    size_t step_count = 0;
    size_t step_index;

    bool combine_with_previous = false;

    uint64_t chord_time_defined_steps = 0;

    assert(reader);
    assert(special);
    assert(freespecial_number);
    assert(error_buffer);
    assert(error_buffer_size);

    *freespecial_number = 0;
    special->numkeys = 0;
    special->steps = 0;
    special->sequence_grace_time = 0;
    special->sequence_grace_time_override = false;

    while(command_token_reader_next(reader, &token)) {
        bool freespecial_recognized;

        int numbered_animation;

        const char* freespecial_error =
            command_token_get_freespecial_number(
                &token,
                &freespecial_recognized,
                &numbered_animation,
                error_buffer,
                error_buffer_size
            );

        if(freespecial_error) {
            return freespecial_error;
        }

        /*
        * Destination animation terminates the sequence.
        */
        if(freespecial_recognized) {
            
            /*
            * A plus token must be followed by another
            * input, not the destination animation.
            */
            if(combine_with_previous) {
                return "Invalid '+' placement in special command";
            }

            /*
            * Destination animation must be the final
            * non-comment token.
            */
            if(command_token_reader_next(reader, &token)) {
                return "Unexpected token after special command animation";
            }

            if(step_count == 0) {
                return "Special command requires at least one input step";
            }

            if(special->sequence_grace_time_override
                && step_count < 2) {
                return "Sequence grace time requires at least two complete input steps";
            }

            /*
            * Passive held requirements qualify a press,
            * release, or future automatic hold event. They
            * cannot create a sequence step by themselves.
            */
            for(step_index = 0;
                step_index < step_count;
                step_index++) {

                const s_command_input_step* input_step =
                    &special->input[step_index];

                if(!input_step->press
                    && !input_step->release
                    && !input_step->hold_trigger) {
                    return "Held input step requires a press, release, or automatic trigger";
                }
            }

            special->steps = (int)step_count;
            *freespecial_number = numbered_animation;

            return NULL;
        }

        /*
        * Optional command-wide grace between separate
        * input steps. A zero value is a valid explicit
        * override, so the boolean records its presence.
        * Its token position does not affect its scope.
        */
        {
            bool sequence_grace_time_recognized;

            uint64_t sequence_grace_time;

            const char* sequence_grace_time_error =
                command_token_get_sequence_grace_time_modifier(
                    &token,
                    &sequence_grace_time_recognized,
                    &sequence_grace_time
                );

            if(sequence_grace_time_error) {
                return sequence_grace_time_error;
            }

            if(sequence_grace_time_recognized) {
                if(combine_with_previous) {
                    return "Sequence grace time cannot interrupt a '+' expression";
                }

                if(special->sequence_grace_time_override) {
                    return "Sequence grace time is already defined for this command";
                }

                special->sequence_grace_time =
                    sequence_grace_time;

                special->sequence_grace_time_override = true;

                continue;
            }
        }

        /*
        * Optional separator between sequence steps.
        *
        * Preserve the old parser's acceptance of repeated
        * arrows after at least one input step.
        */
        if(command_token_equals(&token, "->")) {
            if(combine_with_previous
                || (step_count == 0
                    && !special->sequence_grace_time_override)) {
                return "Invalid '->' placement in special command";
            }

            continue;
        }

        /*
        * Optional grace for the multi-key press chord in
        * the current sequence step.
        */
        {
            bool chord_time_recognized;

            uint64_t chord_time;

            const char* chord_time_error =
                command_token_get_chord_time_modifier(
                    &token,
                    &chord_time_recognized,
                    &chord_time
                );

            if(chord_time_error) {
                return chord_time_error;
            }

            if(chord_time_recognized) {
                uint64_t step_flag;

                if(step_count == 0 || combine_with_previous) {
                    return "Chord grace time must follow a complete press chord";
                }

                destination_step =
                    &special->input[step_count - 1];

                if(!(destination_step->press
                    & (destination_step->press - 1))) {
                    return "Chord grace time requires a multi-key press chord";
                }

                step_flag =
                    UINT64_C(1) << (step_count - 1);

                if(chord_time_defined_steps & step_flag) {
                    return "Chord grace time is already defined for this step";
                }

                destination_step->chord_time = chord_time;
                chord_time_defined_steps |= step_flag;

                continue;
            }
        }

        /*
        * Combine the next input with the most recently
        * stored sequence step.
        */
        if(command_token_equals(&token, "+")) {
            if(step_count == 0 || combine_with_previous) {
                return "Invalid '+' placement in special command";
            }

            combine_with_previous = true;
            continue;
        }

        {
            const char* input_error =
                command_token_get_input_requirement(
                    &token,
                    &input_requirement
                );

            if(input_error) {
                return input_error;
            }
        }

        if(combine_with_previous) {
            destination_step =
                &special->input[step_count - 1];

            combine_with_previous = false;

        } else {
            if(step_count >= MAX_SPECIAL_INPUTS) {
                return "Special command exceeds the maximum of 64 sequence steps.";
            }

            destination_step = &special->input[step_count];
            step_count++;
        }

        /*
        * One command step stores one held-time range.
        * Multiple held inputs may share that range, but a
        * step cannot silently collapse different bounds.
        */
        if(input_requirement.hold
            || input_requirement.hold_trigger) {

            if((destination_step->hold
                    || destination_step->hold_trigger)
                && (destination_step->hold_time
                        != input_requirement.hold_time
                    || destination_step->hold_time_maximum
                        != input_requirement.hold_time_maximum)) {
                return "Held inputs combined in one step must use the same time range";
            }

            destination_step->hold_time =
                input_requirement.hold_time;

            destination_step->hold_time_maximum =
                input_requirement.hold_time_maximum;
        }

        destination_step->press |= input_requirement.press;
        destination_step->hold |= input_requirement.hold;
        destination_step->hold_trigger |=
            input_requirement.hold_trigger;

        destination_step->release |= input_requirement.release;

        /*
        * Preserve existing ranking behavior. numkeys
        * counts input tokens, including inputs combined
        * into the same sequence step.
        */
        special->numkeys++;
    }

    if(combine_with_previous) {
        return "Special command ends with an incomplete '+' expression";
    }

    return "Special command is missing a freespecial animation";
}

size_t ParseArgs(ArgList *list, char *input, char *output)
{
    assert(list);
    assert(input);
    assert(output);

    memset(output,0,MAX_ARG_LEN);

    size_t pos = 0;
    size_t wordstart = 0;
    size_t item = 0;
    // flags
    int done_flag = 0;
    int space_flag = 0; // can find more spaces
    int double_apex_flag = 0;
    int single_apex_flag = 0;

    while(pos < MAX_ARG_LEN - 1 && item < MAX_ARG_COUNT)
    {
        switch(input[pos])
        {
            // read strings
            case '"':
                if ( (pos > 0 && input[pos-1] != '\\') || pos <= 0 )
                {
                    if (space_flag && !double_apex_flag)
                    {
                        double_apex_flag = 1;
                        space_flag = 0;
                        wordstart = pos;
                        output[pos] = input[pos];
                        break;
                    }
                    else if (double_apex_flag)
                    {
                        double_apex_flag = 0;
                        output[pos] = input[pos];
                        // continue to get inputs
                        break;
                    }
                    else
                    {
                        if(space_flag)
                        {
                            wordstart = pos;
                        }
                        output[pos] = input[pos];
                        space_flag = 0;
                        break;
                    }
                }
                else
                {
                    if(space_flag)
                    {
                        wordstart = pos;
                    }
                    output[pos] = input[pos];
                    space_flag = 0;
                    break;
                }
            case '\'':
                if ( (pos > 0 && input[pos-1] != '\\') || pos <= 0 )
                {
                    if (space_flag && !single_apex_flag)
                    {
                        single_apex_flag = 1;
                        space_flag = 0;
                        wordstart = pos;
                        output[pos] = input[pos];
                        break;
                    }
                    else if (single_apex_flag)
                    {
                        single_apex_flag = 0;
                        output[pos] = input[pos];
                        // continue to get inputs
                        break;
                    }
                    else
                    {
                        if(space_flag)
                        {
                            wordstart = pos;
                        }
                        output[pos] = input[pos];
                        space_flag = 0;
                        break;
                    }
                }
                else
                {
                    if(space_flag)
                    {
                        wordstart = pos;
                    }
                    output[pos] = input[pos];
                    space_flag = 0;
                    break;
                }

            // complete item
            case '\r':
            case '\n':
            case '#':
                if (double_apex_flag || single_apex_flag)
                {
                    output[pos] = input[pos];
                    break;
                }
            case '\0':
                done_flag = 1;

            // skip spaces
            case ' ':
            case '\t':
                if (!double_apex_flag && !single_apex_flag)
                {
                    output[pos] = '\0';
                    if(!space_flag && wordstart != pos)
                    {
                        list->args[item] = output + wordstart;
                        list->arglen[item] = pos - wordstart;
                        item++;
                    }
                    space_flag = 1;
                    break;
                }

            // read character
            default:
                if(space_flag && !double_apex_flag && !single_apex_flag)
                {
                    wordstart = pos;
                }
                output[pos] = input[pos];
                space_flag = 0;
        }

        if(done_flag)
        {
            break;
        }
        pos++;
    }
    list->count = item;

    // TEST
    /*printf("found: ");
    int i;
    for (i = 0; i < list->count; i++) {
        printf("|%s|:%d",list->args[i],list->arglen[i]);
        if (i < list->count - 1) printf(" ");
    }
    printf("\n");*/

    return item;
}

int readByte(char *buf)
{
    int num = 0;

    num = (uint64_t)buf[0]&0xFF;

    return num;
}

float diff(float a, float b)
{
    if(a < b)
    {
        return b - a;
    }
    return a - b;
}



int inair(entity *e)
{
    return (diff(e->position.y, e->base) >= 0.1);
}

int inair_range(entity *e)
{
    return (diff(e->position.y, e->base) > T_WALKOFF);
}


// ----------------------- Loaders ------------------------------


// Creates a remapping table from two images
int load_colourmap(s_model *model, char *image1, char *image2)
{
    int i, j, k;
    unsigned char *map = NULL;
    s_bitmap *bitmap1 = NULL;
    s_bitmap *bitmap2 = NULL;

    // Can't use same image twice!
    if(stricmp(image1, image2) == 0)
    {
        return 0;
    }

    __realloc(model->colourmap, model->maps_loaded);
    k = model->maps_loaded++;

    if((map = malloc(MAX_PAL_SIZE / 4)) == NULL)
    {
        return -2;
    }
    if((bitmap1 = loadbitmap(image1, packfile, PIXEL_8)) == NULL)
    {
        free(map);
        map = NULL;
        return -3;
    }
    if((bitmap2 = loadbitmap(image2, packfile, PIXEL_8)) == NULL)
    {
        freebitmap(bitmap1);
        free(map);
        map = NULL;
        return -4;
    }

    // Create the colour map
    for(i = 0; i < MAX_PAL_SIZE / 4; i++)
    {
        map[i] = i;
    }
    for(j = 0; j < bitmap1->height && j < bitmap2->height; j++)
    {
        for(i = 0; i < bitmap1->width && i < bitmap2->width; i++)
        {
            map[(unsigned)(bitmap1->data[j * bitmap1->width + i])] = bitmap2->data[j * bitmap2->width + i];
        }
    }

    freebitmap(bitmap1);
    freebitmap(bitmap2);

    model->colourmap[k] = map;
    return 1;
}

//PIXEL_x8
// This function is used to enable remap command in 24bit mode
// So old mods can still run under 16/24/32bit color system
// This function should be called when all colourmaps are loaded, e.g.,
// at the end of load_cached_model
// map flag is used to determine whether a colourmap is a real colourmap
int convert_map_to_palette(s_model *model, unsigned mapflag[])
{
    int i, c;
    unsigned char *newmap, *oldmap;
    unsigned char *p1, *p2;
    unsigned pb = pixelbytes[(int)PIXEL_32];
    if(model->palette == NULL)
    {
        return 0;
    }
    for(c = 0; c < model->maps_loaded; c++)
    {
        if(mapflag[c] == 0)
        {
            continue;
        }
        if((newmap = malloc(PAL_BYTES)) == NULL)
        {
            borShutdown(1, "Error convert_map_to_palette for model: %s\n", model->name);
        }
        // Create new colour map
        memcpy(newmap, model->palette, PAL_BYTES);
        oldmap = model->colourmap[c];
        for(i = 0; i < MAX_PAL_SIZE / 4; i++)
        {
            if(oldmap[i] == i)
            {
                continue;
            }
            p1 = newmap + i * pb;
            p2 = model->palette + oldmap[i] * pb;
            memcpy(p1, p2, pb);
        }
        model->colourmap[c] = newmap;
        free(oldmap);
        oldmap = NULL;
    }
    return 1;
}

//load a 256 colors' palette
int load_palette(unsigned char *palette, char *filename)
{
    char *fileext;
    int file_id, i;
    uint64_t *acting_palette;
    unsigned char rgb_temp[COLOR_COMPONENT_RGB];

    //printf("\n\nfileext: %s", filename);

    // Determine whether the author is using an .act or image file, and
    // verify the file content is valid to load a color table from.
    fileext = strrchr(filename, '.');
    if(fileext != NULL && stricmp(fileext, ".act") == 0)
    {        

        file_id = openpackfile(filename, packfile);
        if(file_id < 0)
        {
            return 0;
        }

        /* 
        * Reset the palette. 
        */
        memset(palette, 0, MAX_PAL_SIZE);


        acting_palette = (uint64_t*)palette;

        
        for(i = 0; i < MAX_PAL_SIZE / 4; i++)
        {
            //printf("\n\t i: %d", i);


            if(readpackfile(file_id, rgb_temp, COLOR_COMPONENT_RGB) != COLOR_COMPONENT_RGB)
            {
                closepackfile(file_id);
                return 0;
            }

            //printf("\n\t pal[COLOR_COMPONENT_RED]: %d, pal[RGB_GREEN]: %d, pal[RGB_BLUE]: %d", rgb_temp[COLOR_COMPONENT_RED], rgb_temp[COLOR_COMPONENT_GREEN], rgb_temp[COLOR_COMPONENT_BLUE]);
            //printf("\n\t colour32: %d", colour32(rgb_temp[COLOR_COMPONENT_RED], rgb_temp[COLOR_COMPONENT_GREEN], rgb_temp[COLOR_COMPONENT_BLUE]));
            
            acting_palette[i] = colour32(rgb_temp[COLOR_COMPONENT_RED], rgb_temp[COLOR_COMPONENT_GREEN], rgb_temp[COLOR_COMPONENT_BLUE]);
            
            //printf("\n\t acting_palette[%d]: %d", i, acting_palette[i]);
        }                

        closepackfile(file_id);
        
        acting_palette[0] = 0;

        return 1;
    }
    else
    {
        return loadimagepalette(filename, packfile, palette);
    }
}

void create_blend_tables_x8(unsigned char *tables[])
{
    int i;
    for(i = 0; i < MAX_BLENDINGS; i++)
    {
        tables[i] = blending_table_functions32[i] ? (blending_table_functions32[i])() : NULL;
    }

}


//change system palette by index
void change_system_palette(int palindex)
{
    if(palindex < 0)
    {
        palindex = 0;
    }
    //if(current_palette == palindex ) return;


    if(!level || palindex == 0 || palindex > level->numpalettes)
    {
        current_palette = 0;
    }
    else if(level)
    {
        current_palette = palindex;
    }
}

// Load colour 0-127 from data/pal.act
void standard_palette(int immediate)
{
    unsigned char pp[MAX_PAL_SIZE] = {0};
    char *filename = "data/pal.act";

    if(load_palette(pp, filename))
    {
        memcpy(pal, pp, (PAL_BYTES) / 2);
    }
    if(immediate)
    {
        change_system_palette(0);
    }
}

void unload_background()
{
    if (background)
    {
        clearscreen(background);
    }
}


int _makecolour(int r, int g, int b)
{
    return colour32(r, g, b);
}

// parses a color string in the format "R_G_B" or as a raw integer
int parsecolor(const char *string)
{
    int r, g, b;
    if(strchr(string, '_') != strrchr(string, '_'))
    {
        // 2 underscores; color is in "R_G_B" format
        r = atoi(string);
        g = atoi(strchr(string, '_') + 1);
        b = atoi(strrchr(string, '_') + 1);
        return _makecolour(r, g, b);
    }
    else
    {
        return atoi(string);    // raw integer
    }
}

// ltb 1-17-05   new function for lifebar colors
/*
    Kratus (03-2023) Added an alternative location for the lifebar file, now it's possible to use in the external "saves" folder
    Now the modder can load exported lifebar files by using "filestream" script functions
    Useful for creating custom lifebar colors according to certain script functions without unpacking the game
    The default engine lifebar location will be maintained for backward compatibility
    Don't forget that the external file will bypass the internal file!!
*/
void lifebar_colors()
{
    char *filename;
    char *buffer;
    size_t size;
    int position;
    ArgList arglist;
    char argbuf[MAX_ARG_LEN + 1] = "";

    char *command;

    /*
    * Try getting buffer from saves. If that fails
    * we check the data folder. If that also fails
    * then creator didn't provide values. Use the
    * defaults and exit.
    * 
    * The variable "filename" seems superflous, but
    * macros below expect it.
    */
    
    filename = "saves/lifebar.txt";

    if(buffer_pakfile(filename, &buffer, &size) != 1)
    {
        filename = "data/lifebar.txt";

        if (buffer_pakfile(filename, &buffer, &size) != 1)
        {
            color_black = 0;
            color_red = 0;
            color_orange = 0;
            color_yellow = 0;
            color_white = 0;
            color_blue = 0;
            color_green = 0;
            color_pink = 0;
            color_purple = 0;
            color_magic = 0;
            color_magic2 = 0;
            shadowcolor = 0;
            shadowalpha = BLEND_MULTIPLY + 1;
            shadowopacity = 255;
            return;
        }
    }    

    /*
    * 2024-01-04 DC, not sure what this global 
    * assignment does does yet. Leaving as-is for now.
    */
    colorbars = 1;
    

    // Lookup table for color commands
    struct ColorCommand {
        const char* name;
        int* color;
    };

    const struct ColorCommand colorCommands[] = {
        {"blackbox", &color_black},
        {"color25", &color_red},
        {"color50", &color_yellow},
        {"color100", &color_green},
        {"color200", &color_blue},
        {"color300", &color_orange},
        {"color400", &color_pink},
        {"color500", &color_purple},
        {"colormagic", &color_magic},
        {"colormagic2", &color_magic2},
        {"shadowcolor", &shadowcolor},
        {"whitebox", &color_white},
        {NULL, NULL} // Null terminator for the lookup table
    };    
    
    position = 0;
    while (position < size) {
        if (ParseArgs(&arglist, buffer + position, argbuf)) {
            command = GET_ARG(0);

            if (command && command[0]) {
                
                // Search for the command in the lookup table
                const struct ColorCommand* entry = colorCommands;

                while (entry->name != NULL) {
                    if (stricmp(command, entry->name) == 0) {
                        // Found a matching command in the lookup table
                        *(entry->color) = _makecolour(GET_INT_ARG(1), GET_INT_ARG(2), GET_INT_ARG(3));
                        break;
                    }
                    entry++;
                }

                // Check if the command was not found in the lookup table
                if (entry->name == NULL) {
                    printf("Warning: Unknown command in lifebar.txt: '%s'.\n", command);
                }
            }
        }

        // Go to next line
        position += getNewLineStart(buffer + position);
    }   

    if(buffer != NULL)
    {
        free(buffer);
        buffer = NULL;
    }
}
// ltb 1-17-05 end new lifebar colors

void init_colourtable()
{
    mpcolourtable[0]  = color_magic2;
    mpcolourtable[1]  = color_magic;
    mpcolourtable[2]  = color_magic;
    mpcolourtable[3]  = color_magic;
    mpcolourtable[4]  = color_magic2;
    mpcolourtable[5]  = color_magic;
    mpcolourtable[6]  = color_magic2;
    mpcolourtable[7]  = color_magic;
    mpcolourtable[8]  = color_magic2;
    mpcolourtable[9]  = color_magic;
    mpcolourtable[10] = color_magic2;

    hpcolourtable[0]  = color_purple;
    hpcolourtable[1]  = color_red;
    hpcolourtable[2]  = color_yellow;
    hpcolourtable[3]  = color_green;
    hpcolourtable[4]  = color_blue;
    hpcolourtable[5]  = color_orange;
    hpcolourtable[6]  = color_pink;
    hpcolourtable[7]  = color_purple;
    hpcolourtable[8]  = color_black;
    hpcolourtable[9]  = color_white;
    hpcolourtable[10] = color_white;

    memcpy(ldcolourtable, hpcolourtable, 11 * sizeof(*hpcolourtable));
}

void load_background(char *filename)
{
    // Clean up any previous background.
    unload_background();

    // Attempt to load 8bit color depth background. If it fails,
    // then attempt to load 24bit color depth background. If THAT
    // fails, something is wrong and we better shut down to avoid
    // a crash.
    if(!loadscreen(filename, packfile, NULL, PIXEL_x8, &background))
    {
        if (loadscreen32(filename, packfile, &background))
        {
            printf("Loaded 32-bit background '%s'\n", filename);
        }
        else
        {
            borShutdown(1, "Error loading background (PIXEL_x8/PIXEL_32) file '%s'", filename);
        }
    }

    // If background is 8bit color depth, use its color
    // table to populate the global and global neon palettes.
    if (background->pixelformat == PIXEL_x8)
    {
        memcpy(pal, background->palette, PAL_BYTES);
        memcpy(neontable, pal, PAL_BYTES);
    }

    lifebar_colors();
    if(!color_black)
    {
        color_black = _makecolour(0, 0, 0);    // black boxes 500-600HP
    }
    if(!color_red)
    {
        color_red = _makecolour(255, 0, 0);    // 1% - 25% Full Health
    }
    if(!color_orange)
    {
        color_orange = _makecolour(255, 150, 0);    // 200-300HP
    }
    if(!color_yellow)
    {
        color_yellow = _makecolour(0xF8, 0xB8, 0x40);    // 26%-50% Full health
    }
    if(!color_white)
    {
        color_white = _makecolour(255, 255, 255);    // white boxes 600+ HP
    }
    if(!color_blue)
    {
        color_blue = _makecolour(0, 0, 255);    // 100-200 HP
    }
    if(!color_green)
    {
        color_green = _makecolour(0, 255, 0);    // 51% - 100% full health
    }
    if(!color_pink)
    {
        color_pink = _makecolour(255, 0, 255);    // 300-400HP
    }
    if(!color_purple)
    {
        color_purple = _makecolour(128, 48, 208);    // transbox 400-500HP
    }
    if(!color_magic)
    {
        color_magic = _makecolour(98, 180, 255);    // 1st magic bar color by tails
    }
    if(!color_magic2)
    {
        color_magic2 = _makecolour(24, 48, 143);    // 2sec magic bar color by tails
    }
    if(!shadowcolor)
    {
        shadowcolor =  _makecolour(64, 64, 64);
    }
    init_colourtable();

    video_clearscreen();
    pal[0] = pal[1] = pal[2] = 0;
    //palette_set_corrected(pal, savedata.gamma,savedata.gamma,savedata.gamma, savedata.brightness,savedata.brightness,savedata.brightness);
    change_system_palette(0);
}

void load_cached_background(char *filename)
{
#ifndef CACHE_BACKGROUNDS
    load_background(filename);
#else
    int index = -1;
    unload_background();

    if(strcmp(filename, "data/bgs/logo") == 0)
    {
        index = 0;
    }
    else if(strcmp(filename, "data/bgs/title") == 0)
    {
        index = 1;
    }
    else if(strcmp(filename, "data/bgs/titleb") == 0)
    {
        index = 2;
    }
    else if(strcmp(filename, "data/bgs/loading") == 0)
    {
        index = 3;
    }
    else if(strcmp(filename, "data/bgs/loading2") == 0)
    {
        index = 4;
    }
    else if(strcmp(filename, "data/bgs/hiscore") == 0)
    {
        index = 5;
    }
    else if(strcmp(filename, "data/bgs/complete") == 0)
    {
        index = 6;
    }
    else if(strcmp(filename, "data/bgs/unlockbg") == 0)
    {
        index = 7;
    }
    else if(strcmp(filename, "data/bgs/select") == 0)
    {
        index = 8;
    }

    if((index == -1) || (bg_cache[index] == NULL))
    {
        borShutdown(1, "Error: can't load cached background '%s'", filename);
    }

    if(background)
    {
        freescreen(&background);
    }
    background = allocscreen(videomodes.hRes, videomodes.vRes, bg_cache[index]->pixelformat);
    copyscreen(background, bg_cache[index]);

    if(background->pixelformat == PIXEL_8)
    {
        memcpy(pal, bg_palette_cache[index], PAL_BYTES);
    }
    else if(background->pixelformat == PIXEL_x8)
    {
        memcpy(background->palette, bg_cache[index]->palette, PAL_BYTES);
        memcpy(pal, background->palette, PAL_BYTES);
    }

    video_clearscreen();
    pal[0] = pal[1] = pal[2] = 0;
    //palette_set_corrected(pal, savedata.gamma,savedata.gamma,savedata.gamma, savedata.brightness,savedata.brightness,savedata.brightness);
    change_system_palette(0);
    printf("use cached bg\n");
#endif
}

#ifdef CACHE_BACKGROUNDS
void cache_background(char *filename)
{
    s_screen *bg = allocscreen(videomodes.hRes, videomodes.vRes, pixelformat);
    int index = -1;

    if(pixelformat == PIXEL_8)
    {
        if(!loadscreen(filename, packfile, pal, pixelformat, &bg))
        {
            freescreen(&bg);
            bg = NULL;
        }
    }
    else if(pixelformat == PIXEL_x8)
    {
        if(!loadscreen(filename, packfile, NULL, pixelformat, &bg))
        {
            if(!loadscreen32(filename, packfile, &bg))
            {
                freescreen(&bg);
                bg = NULL;
            }
        }
    }
    else
    {
        borShutdown(1, "Error caching background, Unknown Pixel Format!\n");
    }

    if(strcmp(filename, "data/bgs/logo") == 0)
    {
        index = 0;
    }
    else if(strcmp(filename, "data/bgs/title") == 0)
    {
        index = 1;
    }
    else if(strcmp(filename, "data/bgs/titleb") == 0)
    {
        index = 2;
    }
    else if(strcmp(filename, "data/bgs/loading") == 0)
    {
        index = 3;
    }
    else if(strcmp(filename, "data/bgs/loading2") == 0)
    {
        index = 4;
    }
    else if(strcmp(filename, "data/bgs/hiscore") == 0)
    {
        index = 5;
    }
    else if(strcmp(filename, "data/bgs/complete") == 0)
    {
        index = 6;
    }
    else if(strcmp(filename, "data/bgs/unlockbg") == 0)
    {
        index = 7;
    }
    else if(strcmp(filename, "data/bgs/select") == 0)
    {
        index = 8;
    }
    else
    {
        borShutdown(1, "Error: unknown cached background '%s'", filename);
    }

    bg_cache[index] = bg;

    if(pixelformat == PIXEL_8)
    {
        memcpy(bg_palette_cache[index], pal, PAL_BYTES);
    }

    change_system_palette(0);
}

void cache_all_backgrounds()
{
    cache_background("data/bgs/logo");
    cache_background("data/bgs/title");
    cache_background("data/bgs/titleb");
    cache_background("data/bgs/loading2");
    cache_background("data/bgs/hiscore");
    cache_background("data/bgs/complete");
    cache_background("data/bgs/unlockbg");
    cache_background("data/bgs/select");
}
#endif

void load_layer(char *filename, char *maskfilename, int index)
{
    if(!level)
    {
        return;
    }

    if(filename && level->layers[index].gfx.handle == NULL)
    {
        if(*maskfilename || ((level->layers[index].drawmethod.alpha > 0 || level->layers[index].drawmethod.config & DRAWMETHOD_CONFIG_BACKGROUND_TRANSPARENCY) && !level->layers[index].drawmethod.water.watermode))
        {
            // assume sprites are faster than screen when transparency or alpha are specified
            level->layers[index].gfx.sprite = loadsprite2(filename, &(level->layers[index].size.x), &(level->layers[index].size.y));
            if (*maskfilename)
            {
                level->layers[index].gfx.sprite->mask = loadsprite2(maskfilename, &(level->layers[index].size.x), &(level->layers[index].size.y));
                *maskfilename = 0; // clear mask filename so mask is only used for this one sprite
            }
        }
        else
        {
            // use screen for water effect for now, it should be faster than sprite
            // otherwise, a screen should be fine, especially in 8bit mode, it is super fast,
            //            or, at least it is not slower than a sprite
            if(loadscreen(filename, packfile, NULL, pixelformat, &level->layers[index].gfx.screen))
            {
                level->layers[index].size.y = level->layers[index].gfx.screen->height;
                level->layers[index].size.x = level->layers[index].gfx.screen->width;
            }
        }
    }

    if(filename && level->layers[index].gfx.handle == NULL)
    {
        borShutdown(1, "Error loading file '%s'", filename);
    }
    else
    {
        if(level->layers[index].drawmethod.xrepeat < 0)
        {
            level->layers[index].offset.x -= level->layers[index].size.x * 20000;
            level->layers[index].drawmethod.xrepeat = 40000;
        }
        if(level->layers[index].drawmethod.yrepeat < 0)
        {
            level->layers[index].offset.z -= level->layers[index].size.y * 20000;
            level->layers[index].drawmethod.yrepeat = 40000;
        }
        //printf("bglayer width=%d height=%d xoffset=%d zoffset=%d xrepeat=%d zrepeat%d\n", level->layers[index].size.x, level->layers[index].size.y, level->layers[index].offset.x, level->layers[index].offset.z, level->layers[index].xrepeat, level->layers[index].zrepeat);
    }

}


s_sprite *loadsprite2(char *filename, int *width, int *height)
{
    size_t size;
    s_bitmap *bitmap = NULL;
    s_sprite *sprite = NULL;
    int clip_left;
    int clip_right;
    int clip_top;
    int clip_bottom;

    // Load raw bitmap (image) file from pack. If this
    // fails, then we return NULL.
    bitmap = loadbitmap(filename, packfile, pixelformat);

    if(!bitmap)
    {
        return NULL;
    }

    // Apply width and height adjustments, if any.
    if(width)
    {
        *width = bitmap->width;
    }

    if(height)
    {
        *height = bitmap->height;
    }

    // Trim empty pixels from the bitmap to save memory.
    // We will pass the arguments by reference - they will
    // be modified by the clipping function to tell us
    // exactly how much trim work on each axis was done.
    clipbitmap(bitmap, &clip_left, &clip_right, &clip_top, &clip_bottom);

    // Get size of trimmed bitmap and allocate memory for
    // use as a sprite. If this fails, then free the memory
    // bitmap occupies, and return NULL.
    size = fakey_encodesprite(bitmap);
    sprite = (s_sprite *)malloc(size);

    if(!sprite)
    {
        freebitmap(bitmap);
        return NULL;
    }

    // Transpose bitmap to a sprite, using the memory
    // we allocated for it above. The trim arguments
    // from our trimming function will be used as an
    // offset. We'll also store/ the bitmap's trimmed
    // dimensions for later use.
    encodesprite(-clip_left, -clip_top, bitmap, sprite);
    sprite->offsetx = clip_left;
    sprite->offsety = clip_top;
    sprite->srcwidth = bitmap->clipped_width;
    sprite->srcheight = bitmap->clipped_height;

    // Delete the raw bitmap, we don't need it
    // any more.
    freebitmap(bitmap);

    // Return encoded sprite.
    return sprite;
}


// Added to conserve memory
void resourceCleanUp()
{
    freesprites();
    free_models();
    free_modelcache();
    load_special_sounds();
    load_script_setting();
    load_special_sprites();
    load_levelorder();
    load_models();
}

void freesprites()
{
    unsigned i;
    s_sprite_list *head;
    for(i = 0; i <= sprites_loaded; i++)
    {
        if(sprite_list != NULL)
        {
            free(sprite_list->sprite);
            sprite_list->sprite = NULL;
            free(sprite_list->filename);
            sprite_list->filename = NULL;
            head = sprite_list->next;
            free(sprite_list);
            sprite_list = head;
        }
    }
    if(sprite_map != NULL)
    {
        free(sprite_map);
        sprite_map = NULL;
    }
    sprites_loaded = 0;
}

// allocate enough members for sprite_map
void prepare_sprite_map(size_t size)
{
    if(sprite_map == NULL || size + 1 > sprite_map_max_items )
    {
#ifdef VERBOSE
        printf("%s %p\n", "prepare_sprite_map was", sprite_map);
#endif
        sprite_map_max_items = (((size + 1) >> 8) + 1) << 8;
        sprite_map = realloc(sprite_map, sizeof(*sprite_map) * sprite_map_max_items);
        if(sprite_map == NULL)
        {
            borShutdown(1, "Out Of Memory!  Failed to create a new sprite_map\n");
        }
    }
}

void cachesound(int index, int load)
{
    if(index < 0)
    {
        return;
    }
    if(load)
    {
        sound_reload_sample(index);
    }
    else
    {
        sound_unload_sample(index);
    }
}

// Cachesprite
// Unknown original date & author
// Rewrite by Caskey, Damon V.
// 2018-03-19
//
// Add or remove a sprite to the sprite list
// by index.
//
// index: Target index in the sprite list.
// load: Load 1, or unload 0 the target sprite index.
void cachesprite(int index, int load) {

    s_sprite *sprite;           // Sprite placeholder.
    s_sprite_list *map_node;    // Sprite map node placeholder.

    // Valid sprite list?
    if(sprite_map) {

        // Index argument valid?
        if(index >= 0) {

            // Index argument should be more than
            // the number of sprites loaded.
            if(index < sprites_loaded) {

                // Get the sprite list node from sprite maps
                // using our target index.
                map_node = sprite_map[index].node;

                // If load is true, then we want to load
                // a sprite and assign it the target index.
                // Otherwise, we want to free a sprite with
                // target index.
                if(load) {

                    // Make sure there is not already
                    // a sprite with our target index.
                    sprite = map_node->sprite;

                    if(!sprite) {

                        // Load the sprite file, then assign its
                        // new pointer to the sprite map using our
                        // index for the sprite map position.
                        sprite = loadsprite2(map_node->filename, NULL, NULL);
                        map_node->sprite = sprite;
                    }

                } else if(!load) {

                    // Does the target sprite exist?
                    sprite = map_node->sprite;

                    if(sprite) {

                        // Free the target sprite's resources, then remove
                        // its pointer from sprite map.
                        free(sprite);
                        map_node->sprite = NULL;

                        //printf("uncached sprite: %s\n", map_node->filename);
                    }
                }
            }
        }
    }
}

/*
* Caskey, Damon V.
* Original date and author unknown, reworked 2026-06-01.
*
* Loads a sprite from disk or the active pack file, encodes
* it into the internal sprite format, and returns the sprite
* index. If the same source image and offset were previously
* loaded, the existing sprite index is returned.
*
* Hard fails on load or allocation failure and sends error
* to log.
*/
int loadsprite(char *filename, int offset_x, int offset_y, int bmpformat) {

    ptrdiff_t i;
    ptrdiff_t size;
    ptrdiff_t len;
    s_bitmap *bitmap = NULL;
    int clip_left; 
    int clip_right; 
    int clip_top; 
    int clip_bottom;
    s_sprite_list *curr = NULL;
    s_sprite_list *head = NULL;
    s_sprite_list *toshare = NULL;

    /*
    * Look for an already loaded copy of this sprite.
    * Sprites with the same source image can share pixel data,
    * but may still need separate sprite map entries if they use
    * different offsets.
    */
    for(i = 0; i < sprites_loaded; i++) {
        if(sprite_map && sprite_map[i].node) {
            if(stricmp(sprite_map[i].node->filename, filename) == 0) {

                /*
                * Some shared sprite nodes may exist without encoded
                * sprite data loaded yet. Load it on demand before
                * checking offset values.
                */
                if(!sprite_map[i].node->sprite) {
                    sprite_map[i].node->sprite = loadsprite2(filename, NULL, NULL);
                }

                /*
                * The sprite map stores center coordinates relative to
                * the clipped sprite offset. If the requested offset
                * matches an existing entry, return that sprite index.
                */
                if(sprite_map[i].centerx + sprite_map[i].node->sprite->offsetx == offset_x &&
                        sprite_map[i].centery + sprite_map[i].node->sprite->offsety == offset_y) {
                    return i;
                } else {

                    /*
                    * Same image, different offset. Keep the existing
                    * sprite node so the new map entry can share the
                    * encoded sprite data instead of loading another copy.
                    */
                    toshare = sprite_map[i].node;
                }
            }
        }
    }

    /*
    * If the image was already loaded but the offset did not match,
    * create a new sprite map entry that points to the existing sprite
    * node. This avoids duplicate bitmap loads and duplicate encoded
    * sprite allocations.
    */
    if(toshare && toshare->sprite){
        prepare_sprite_map(sprites_loaded + 1);
        sprite_map[sprites_loaded].node = toshare;
        sprite_map[sprites_loaded].centerx = offset_x - toshare->sprite->offsetx;
        sprite_map[sprites_loaded].centery = offset_y - toshare->sprite->offsety;
        ++sprites_loaded;
        return sprites_loaded - 1;
    }

    /*
    * No usable cached sprite was found, so load the source bitmap from
    * disk or the active pack file. loadbitmap() returns NULL on any
    * image open, allocation, or decode failure.
    */
    bitmap = loadbitmap(filename, packfile, bmpformat);
    if(bitmap == NULL) {
        borShutdown(1, "Unable to load image file '%s'\nAcceptable formats: \n\t - 8bit non-interlaced .png \n\t - 24-bit .png (background only) \n\t - Animated .gif (cutscenes only) \n\n", filename);
    }

    /*
    * Trim empty transparent borders from the bitmap before encoding.
    * The clip values are saved as sprite offsets so the rendered image
    * still aligns to the original requested position.
    */
    clipbitmap(bitmap, &clip_left, &clip_right, &clip_top, &clip_bottom);

    /*
    * Calculate storage requirements for the encoded sprite data.
    * fakey_encodesprite() returns the number of bytes needed by
    * encodesprite() for this bitmap and format.
    */
    len = strlen(filename);
    size = fakey_encodesprite(bitmap);
    
    /*
    * Allocate memory for the new sprite list node, and 
    * the sprite and filename members of the new node. 
    * If any of these allocations fail, free any memory 
    * we allocated for the bitmap, and shut down to avoid 
    * a crash.
    */
    curr = malloc(sizeof(*curr));
    if(curr == NULL) {
        freebitmap(bitmap);
        borShutdown(1, "loadsprite() Out of memory!\n");
    }

    curr->sprite = malloc(size);
    curr->filename = malloc(len + 1);

    if(curr->sprite == NULL || curr->filename == NULL) {
        free(curr->sprite);
        free(curr->filename);
        free(curr);
        freebitmap(bitmap);
        borShutdown(1, "loadsprite() Out of memory!\n");
    }

    /*
    * Store the original filename for future cache lookups, then encode
    * the clipped bitmap into the engine's internal sprite format.
    */
    memcpy(curr->filename, filename, len);
    curr->filename[len] = 0;
    encodesprite(offset_x - clip_left, offset_y - clip_top, bitmap, curr->sprite);
    
    /*
    * Insert the new sprite node at the head of the global sprite list.
    * The sprite map below will point at this node by reference.
    */
    if(sprite_list == NULL) {
        sprite_list = curr;
        sprite_list->next = NULL;
    } else {
        head = sprite_list;
        sprite_list = curr;
        sprite_list->next = head;
    }

    /*
    * Add the new sprite to the sprite map. The map stores the adjusted
    * center position, while the sprite itself stores the clip offset and
    * original clipped source dimensions for rendering and collision use.
    */
    prepare_sprite_map(sprites_loaded + 1);
    sprite_map[sprites_loaded].node = sprite_list;
    sprite_map[sprites_loaded].centerx = offset_x - clip_left;
    sprite_map[sprites_loaded].centery = offset_y - clip_top;
    sprite_list->sprite->offsetx = clip_left;
    sprite_list->sprite->offsety = clip_top;
    sprite_list->sprite->srcwidth = bitmap->clipped_width;
    sprite_list->sprite->srcheight = bitmap->clipped_height;

    /*
    * The encoded sprite now owns the data needed by the engine, so the
    * temporary bitmap can be released before returning the new index.
    */
    freebitmap(bitmap);
    ++sprites_loaded;
    return sprites_loaded - 1;
}

/*
* Caskey, Damon V.
* Original date and author unknown, reworked 2026-07-06.
*
* Loads hard coded special sprites and 
* configured icons. Does nothing if the
* sprites do not exist in the packfile. 
*
* Reworked to remove .gif support and 
* replace if chain with a more maintainable 
* lookup table for hard coded paths.
*/
void load_special_sprites() {

    /*
    * Lookup table structure for hard coded special
    * sprites.
    */

    typedef struct {
        char* path;
        int x_offset;
        int y_offset;
        int* sprite;
    } s_special_sprite_load;
    
    const s_special_sprite_load special_sprite_loads[] = {
        { "data/sprites/shadow1.png", 9,  3,  &shadowsprites[0] },
        { "data/sprites/shadow2.png", 14, 5,  &shadowsprites[1] },
        { "data/sprites/shadow3.png", 19, 6,  &shadowsprites[2] },
        { "data/sprites/shadow4.png", 24, 8,  &shadowsprites[3] },
        { "data/sprites/shadow5.png", 29, 9,  &shadowsprites[4] },
        { "data/sprites/shadow6.png", 34, 11, &shadowsprites[5] },
        { "data/sprites/arrow.png",   35, 23, &gosprite },
        { "data/sprites/arrowl.png",  35, 23, &golsprite }
    };

    size_t sprite_index;
    const s_special_sprite_load* sprite_load;
    const size_t num_special_sprites = sizeof(special_sprite_loads) / sizeof(special_sprite_loads[0]);

    /* 
    * Scan the lookup table, test each path and
    * load the corresponding sprite if the file 
    * exists.
    * 
    * Non existent files get -1 assigned to their 
    * sprite index to indicate they are not loaded.
    */
    for(sprite_index = 0; sprite_index < num_special_sprites; sprite_index++) {

        /* 
        * Dereference the current lookup table entry.
        */
        sprite_load = &special_sprite_loads[sprite_index];

        /*
        * Initialize the sprite index to -1 to indicate
        * that the sprite is not loaded yet.
        */
        *sprite_load->sprite = -1;

        /*
        * Check if the file exists in the packfile. If it 
        * does, load the sprite and update the sprite index.
        */
        if(testpackfile(sprite_load->path, packfile) >= 0) {
            *sprite_load->sprite = loadsprite(sprite_load->path,
                                              sprite_load->x_offset,
                                              sprite_load->y_offset,
                                              pixelformat);
        }
    }

    /*
    * Load configured icons.
    */

    if(timeicon_path[0]) {
        timeicon = loadsprite(timeicon_path, 0, 0, pixelformat);
    }

    if(bgicon_path[0]) {
        bgicon = loadsprite(bgicon_path, 0, 0, pixelformat);
    }

    if(olicon_path[0]) {
        olicon = loadsprite(olicon_path, 0, 0, pixelformat);
    }
}

void unload_all_fonts()
{
    int i;
    for(i = 0; i < MAX_FONTS; i++)
    {
        font_unload(i);
    }
}

void load_all_fonts()
{
    char path[MAX_BUFFER_LEN];
    int i;

    for(i = 0; i < MAX_FONTS; i++)
    {
        if(i == 0)
        {
            strcpy(path, "data/sprites/font");
        }
        else
        {
            sprintf(path, "%s%d", "data/sprites/font", i + 1);
        }
        if(font_load(i, path, packfile, fontmonospace[i] | fontmbs[i]))
        {
            // Plombo 3/1/2013: allow fonts to have alpha masks
            if(i == 0)
            {
                strcpy(path, "data/sprites/fontmask");
            }
            else
            {
                sprintf(path, "%s%d", "data/sprites/fontmask", i + 1);
            }
            if(font_loadmask(i, path, packfile, fontmonospace[i] | fontmbs[i]))
            {
                printf("%d(m) ", i + 1);
            }
            else
            {
                printf("%d ", i + 1);
            }
        }
    }
}

int translate_SDID(char *value)
{
    if(stricmp(value, "moveup") == 0)
    {
        return SDID_MOVEUP;
    }
    else if(stricmp(value, "movedown") == 0)
    {
        return SDID_MOVEDOWN;
    }
    else if(stricmp(value, "moveleft") == 0)
    {
        return SDID_MOVELEFT;
    }
    else if(stricmp(value, "moveright") == 0)
    {
        return SDID_MOVERIGHT;
    }
    else if(stricmp(value, "attack") == 0)
    {
        return SDID_ATTACK;
    }
    else if(stricmp(value, "attack2") == 0)
    {
        return SDID_ATTACK2;
    }
    else if(stricmp(value, "attack3") == 0)
    {
        return SDID_ATTACK3;
    }
    else if(stricmp(value, "attack4") == 0)
    {
        return SDID_ATTACK4;
    }
    else if(stricmp(value, "jump") == 0)
    {
        return SDID_JUMP;
    }
    else if(stricmp(value, "special") == 0)
    {
        return SDID_SPECIAL;
    }
    else if(stricmp(value, "start") == 0)
    {
        return SDID_START;
    }
    else if(stricmp(value, "screenshot") == 0)
    {
        return SDID_SCREENSHOT;
    }
    else if(stricmp(value, "esc") == 0)
    {
        return SDID_ESC;
    }

    return -1;
}

void load_menu_txt()
{
    char *filename = "translation/menu.txt";
    int pos, i;
    char *buf, *command;
    size_t size;
    ArgList arglist;
    char argbuf[MAX_ARG_LEN + 1] = "";

    /*
        Kratus (10-2021) Added an alternative location for the translation file, now it's possible to use in an external folder
        Now the modder can load exported translation files by using "filestream" script functions
        Useful for creating custom translations without unpack the game
        The default engine translation location will be maintained for backward compatibility

        Kratus (11-2021) Inverted the path priority, now the external file will override the internal file
        Useful to maintain the english translation intact inside the pak file if no other language file is found in the external path
        Otherwise you will need to rollback the english file every time another language is used and then removed
        This operation is needed only if the english translation file uses some custom menu texts for english language too
    */
    if(buffer_pakfile(filename, &buf, &size) != 1)
    {
        goto default_file;
    }
    else
    {
        goto proceed;
    }

default_file:

    if(buffer_pakfile("data/menu.txt", &buf, &size) != 1)
    {
        return;
    }
    else
    {
        goto proceed;
    }

proceed:

    // Now interpret the contents of buf line by line
    pos = 0;
    while(pos < size)
    {
        if(ParseArgs(&arglist, buf + pos, argbuf))
        {
            command = GET_ARG(0);
            if(command && command[0])
            {
                if(stricmp(command, "fontmonospace") == 0)
                {
                    for(i = 0; i < MAX_FONTS; i++)
                    {
                        fontmonospace[i] = GET_INT_ARG((i + 1)) ? FONT_MONO : 0;
                    }
                }
                else if(stricmp(command, "fontmbs") == 0)
                {
                    for(i = 0; i < MAX_FONTS; i++)
                    {
                        fontmbs[i] = GET_INT_ARG((i + 1)) ? FONT_MBS : 0;
                    }
                }
            }
        }

        // Go to next line
        pos += getNewLineStart(buf + pos);
    }

    if(buf != NULL)
    {
        free(buf);
        buf = NULL;
    }
}

int load_special_sounds()
{
    sound_unload_all_samples();
    global_sample_list.go = sound_load_sample("data/sounds/go.wav",		packfile,	0, 0);
    global_sample_list.beat = sound_load_sample("data/sounds/beat1.wav",	packfile,	0, 0);
    global_sample_list.block = sound_load_sample("data/sounds/block.wav",	packfile,	0, 0);
    global_sample_list.fall = sound_load_sample("data/sounds/fall.wav",		packfile,	0, 0);
    global_sample_list.get = sound_load_sample("data/sounds/get.wav",		packfile,	0, 0);
    global_sample_list.get_2 = sound_load_sample("data/sounds/money.wav",	packfile,	0, 0);
    global_sample_list.jump = sound_load_sample("data/sounds/jump.wav",		packfile,	0, 0);
    global_sample_list.indirect = sound_load_sample("data/sounds/indirect.wav",	packfile,	0, 0);
    global_sample_list.punch = sound_load_sample("data/sounds/punch.wav",	packfile,	0, 0);
    global_sample_list.one_up = sound_load_sample("data/sounds/1up.wav",		packfile,	0, 0);
    global_sample_list.time_over = sound_load_sample("data/sounds/timeover.wav", packfile,	0, 0);
    global_sample_list.beep = sound_load_sample("data/sounds/beep.wav",		packfile,	0, 0);
    global_sample_list.beep_2 = sound_load_sample("data/sounds/beep2.wav",	packfile,	0, 0);
    global_sample_list.pause = sound_load_sample("data/sounds/pause.wav",	packfile,	0, 0);
    global_sample_list.bike = sound_load_sample("data/sounds/bike.wav",		packfile,	0, 0);

    if (global_sample_list.pause < 0 ) global_sample_list.pause = global_sample_list.beep_2;
    if(global_sample_list.go < 0 || global_sample_list.beat < 0 || global_sample_list.block < 0 ||
        global_sample_list.fall < 0 || global_sample_list.get < 0 || global_sample_list.get_2 < 0 ||
        global_sample_list.jump < 0 || global_sample_list.indirect < 0 || global_sample_list.punch < 0 ||
        global_sample_list.one_up < 0 || global_sample_list.time_over < 0 || global_sample_list.beep < 0 ||
        global_sample_list.beep_2 < 0 || global_sample_list.pause < 0 || global_sample_list.bike < 0)
    {
        return 0;
    }
    return 1;
}

// Caskey, Damon V.
// 2019-01-02
//
// Return true if map_index matches a special purpose
// map or falls within author defined hidden map range, 
// unless any of the above are same as default map (0).
int is_map_hidden(s_model *model, int map_index)
{
	// Have frozen map and it isn't same as default?
	// If we do and it matches, return true.
	if (model->colorsets.frozen > 0)
	{
		if (map_index == model->colorsets.frozen)
		{
			return 1;
		}
	}

	// Check KO map. Same logic as frozen.
	if (model->colorsets.ko > 0)
	{
		if (map_index == model->colorsets.ko)
		{
			return 1;
		}
	}

	// Hidden map range. Both should be
	// something other than default. If 
	// they are and map index is in range
	// we return true.
	if (model->colorsets.hide_start > 0
		&& model->colorsets.hide_end > 0)
	{
		if (map_index >= model->colorsets.hide_start
			&& map_index <= model->colorsets.hide_end)
		{
			return 1;
		}
	}

	// If we got this far, there's no match. 
	return 0;
}

// Return model's next selectable map index in line.
int nextcolourmap(s_model *model, int map_index)
{
	// Increment to next color set, or return to 0 
	// if we go past number of available sets. 
	// Continue until we find an index that
	// isn't hidden.
    do
    {
		map_index++;

        if(map_index > model->maps_loaded)
        {
			map_index = 0;
        }
    }
    while(is_map_hidden(model, map_index));

    return map_index;
}

// Increment to next map in player's (player_index) model
// while avoiding the map another player with same 
// is using.
int nextcolourmapn(s_model *model, int map_index, int player_index)
{
	// Increment to next index.
	map_index = nextcolourmap(model, map_index);

    s_set_entry *set = levelsets + current_set;

	// If color selection is allowed but identical map is 
	// not (nosame 2), then let's make sure anohter player 
	// with same model isn't already using this map.
	// If they are we'll find the next map available.
    if (colourselect && (set->nosame & 2))
    {
		int i = 0;
		int j = 0;
        int maps_count = model->maps_loaded + 1;
        int used_colors_map[maps_count];
        int used_color_count = 0;

        // Reset local used map array elements to 0.
		for (i = 0; i < maps_count; i++)
		{
			used_colors_map[i] = 0;
		}

        // Deduct hidden maps from map count.
		if (model->colorsets.frozen > 0)
		{
			--maps_count;
		}

		if (model->colorsets.ko > 0)
		{
			--maps_count;
		}

		if (model->colorsets.hide_start > 0)
		{
			maps_count -= model->colorsets.hide_end - model->colorsets.hide_start + 1;
		}

        // This logic attempts to populate used_colors_map array with
		// every color in use by other players who picking same
		// character. If there aren't enough unused map indexes to
		// go around (i.e. three players select a character that only
		// has two maps), then we return initial map selection.

        for(i = 0; i < MAX_PLAYERS; i++)
        {
			// Compare every player index to player_index argument. If
			// it's a different index but that index's model matches
			// player_index's model, then it's another player choosing 
			// (or about to choose) the same character.

            if (player_index != i 
				&& 
				stricmp(player[player_index].name, player[i].name) == 0)
            {
				// Use the map index as an array element index, and mark it true.
				// Now we now this map index is in use.
                used_colors_map[player[i].colourmap] = 1;
                
				// Increment number of used map indexes.
				++used_color_count;
                
				// If all the map indexes are used, we'll just
				// have to settle for one we already picked.
				if (used_color_count >= maps_count)
				{
					return map_index;
				}
            }
        }

		// Now that we have a list of used maps, let's employ it to
		// find the first free map.
		//
        // Loop to number of maps for the model. If our used_colors_map
		// array element matching the map index doesn't have a true
		// value, we can return the index.

        for(i = map_index, j = 0; j < maps_count; j++)
        {
            if (!used_colors_map[i])
            {
				return i;
            }

            i = nextcolourmap(model, i);
        }
    }

	// If we got here, then we couldn't find a free map index,
	// so just return initial selection.
    return map_index;
}

// Return model's previous selectable map index in line.
int prevcolourmap(s_model *model, int map_index)
{
	// Decrement to previous color set, or return 
	// to last set if we go below 0. Continue until
	// we find an index that isn't hidden.
    do
    {
		map_index--;
        if(map_index < 0)
        {
			map_index = model->maps_loaded;
        }
    }
    while(is_map_hidden(model, map_index));

    return map_index;
}

// Decrement to previous map in player's (player_index) model
// while avoiding the map another player with same 
// is using.
int prevcolourmapn(s_model *model, int map_index, int player_index)
{
	// Decrement to previous index.
	map_index = prevcolourmap(model, map_index);

	s_set_entry *set = levelsets + current_set;

	// If color selection is allowed but identical map is 
	// not (nosame 2), then let's make sure anohter player 
	// with same model isn't already using this map.
	// If they are we'll find the next map available.
	if (colourselect && (set->nosame & 2))
	{
		int i = 0;
		int j = 0;
		int maps_count = model->maps_loaded + 1;
		int used_colors_map[maps_count];
		int used_color_count = 0;

		// Reset local used map array elements to 0.
		for (i = 0; i < maps_count; i++)
		{
			used_colors_map[i] = 0;
		}

		// Deduct hidden maps from map count.
		if (model->colorsets.frozen > 0)
		{
			--maps_count;
		}

		if (model->colorsets.ko > 0)
		{
			--maps_count;
		}

		if (model->colorsets.hide_start > 0)
		{
			maps_count -= model->colorsets.hide_end - model->colorsets.hide_start + 1;
		}

		// This logic attempts to populate used_colors_map array with
		// every color in use by other players who picking same
		// character. If there aren't enough unused map indexes to
		// go around (i.e. three players select a character that only
		// has two maps), then we return initial map selection.

		for (i = 0; i < MAX_PLAYERS; i++)
		{
			// Compare every player index to player_index argument. If
			// it's a different index but that index's model matches
			// player_index's model, then it's another player choosing 
			// (or about to choose) the same character.

			if (player_index != i
				&&
				stricmp(player[player_index].name, player[i].name) == 0)
			{
				// Use the map index as an array element index, and mark it true.
				// Now we now this map index is in use.
				used_colors_map[player[i].colourmap] = 1;

				// Increment number of used map indexes.
				++used_color_count;

				// If all the map indexes are used, we'll just
				// have to settle for one we already picked.
				if (used_color_count >= maps_count)
				{
					return map_index;
				}
			}
		}

		// Now that we have a list of used maps, let's employ it to
		// find the first free map.
		//
		// Loop to number of maps for the model. If our used_colors_map
		// array element matching the map index doesn't have a true
		// value, we can return the index.

		for (i = map_index, j = 0; j < maps_count; j++)
		{
			if (!used_colors_map[i])
			{
				return i;
			}

			i = prevcolourmap(model, i);
		}
	}

	// If we got here, then we couldn't find a free map index,
	// so just return initial selection.
	return map_index;
}

// Caskey, Damon V.
// 2019-01-02
//
// Return true if a model cache element is selectable by player.
int is_model_cache_index_selectable(int cache_index)
{
	// Must have selectable flag.
	if (!model_cache[cache_index].selectable)
	{
		return 0;
	}

	// Element must contain a valid model.
	if (!model_cache[cache_index].model)
	{
		return 0;
	}
	
	// Element's model must be selectable.
	if (!is_model_selectable(model_cache[cache_index].model))
	{
		return 0;
	}

	// All checks passed. Return true.
	return 1;
}

// Caskey, Damon V.
// 2019-01-02
//
// Return true if a model is selectable by player.
int is_model_selectable(s_model *model)
{
	// Must be a player type.
	if (model->type != TYPE_PLAYER)
	{
		return 0;
	}

	// If model is marked secret, then secret
	// characters must be allowed.
	if (model->secret)
	{
		if (!allow_secret_chars)
		{
			return 0;
		}
	}

	// 2019-01-02 DC: Not sure what this is. 
	// TO DO - Document clearcount vs. bonus.
	if (model->clearcount > bonus)
	{
		return 0;
	}

	// Got this far, we can return true.
	return 1;
}

// Caskey, Damon V.
// 2019-01-03
//
// Return current number of player selectable models.
int find_selectable_model_count()
{
	int result;
	int i;

	result = 0;

	// Loop over model cache and increment
	// count each time we find a selectable
	// model.
	for (i = 0; i < models_cached; i++)
	{
		if (is_model_cache_index_selectable(i))
		{
			++result;
		}
	}

	return result;
}

// Use by player select menus
s_model *nextplayermodel(s_model *current)
{
    int i;
    int curindex = -1;
    int loops;
    
	// Do we have a model?
	if(current)
    {
        // Find index of current player model
        for(i = 0; i < models_cached; i++)
        {
            if(model_cache[i].model == current)
            {
                curindex = i;
                break;
            }
        }
    }

    // Find next player model (first one after current index)
    for(i = curindex + 1, loops = 0; loops < models_cached; i++, loops++)
    {
		// Return to 0 if we've gone past the last model.
        if(i >= models_cached)
        {
            i = 0;
        }

		// If valid and selectable, return the model.
        if(is_model_cache_index_selectable(i))
        {
			//printf("next %s\n", model_cache[i].model->name);
			return model_cache[i].model;            
        }
    }
    borShutdown(1, "Fatal: can't find any player models!");
    return NULL;
}

s_model *nextplayermodeln(s_model *current, int player_index)
{
    int i;
    s_set_entry *set = levelsets + current_set;
    s_model *model = nextplayermodel(current);

    if(set->nosame & 1)
    {
		int used_player_count = 0;
		int player_count = 0;

		// Get number of selectable models.
		player_count = find_selectable_model_count();

        // count all used player
        for(i = 0; model && i < MAX_PLAYERS; i++)
        {
            if(i != player_index 
				&& stricmp(player[player_index].name, player[i].name) == 0)
            {
                ++used_player_count;
                // all busy players? return the next natural
				if (used_player_count >= player_count)
				{
					return model;
				}
            }
        }

        // search the first free player
        for(i = 0; model && i < MAX_PLAYERS; i++)
        {
            if(i != player_index && stricmp(model->name, player[i].name) == 0)
            {
                i = -1;
                model = nextplayermodel(model);
            }
        }
    }

    return model;
}

// Use by player select menus
s_model *prevplayermodel(s_model *current)
{
    int i;
    int curindex = -1;
    int loops;
    if(current)
    {
        // Find index of current player model
        for(i = 0; i < models_cached; i++)
        {
            if(model_cache[i].model == current)
            {
                curindex = i;
                break;
            }
        }
    }
    // Find next player model (first one after current index)
    for(i = curindex - 1, loops = 0; loops < models_cached; i--, loops++)
    {
        if(i < 0)
        {
            i = models_cached - 1;
        }

		// If valid and selectable, return the model.
        if(is_model_cache_index_selectable(i))
        {
            //printf("prev %s\n", model_cache[i].model->name);
            return model_cache[i].model;
        }
    }
    borShutdown(1, "Fatal: can't find any player models!");
    return NULL;
}

s_model *prevplayermodeln(s_model *current, int player_index)
{
    int i;
    s_set_entry *set = levelsets + current_set;
    s_model *model = prevplayermodel(current);

    if(set->nosame & 1)
    {
		int used_player_count = 0; 
		int player_count = 0;

		// Get number of selectable models.
		player_count = find_selectable_model_count();

        // count all used player
        for(i = 0; model && i < MAX_PLAYERS; i++)
        {
            if(i != player_index && stricmp(player[player_index].name, player[i].name) == 0)
            {
                ++used_player_count;
                // all busy players? return the prev natural
                if (used_player_count >= player_count) return model;
            }
        }

        // search the first free player
        for(i = 0; model && i < MAX_PLAYERS; i++)
        {
            if(i != player_index && stricmp(model->name, player[i].name) == 0)
            {
                i = -1;
                model = prevplayermodel(model);
            }
        }
    }

    return model;
}

// Reset All Player Models to on/off for Select Screen.
static void reset_playable_list(char which)
{
    int i;
    for(i = 0; i < models_cached; i++)
    {
        if(!which || (model_cache[i].model && model_cache[i].model->type == TYPE_PLAYER))
        {
            model_cache[i].selectable = which;
        }
    }
}

/*
- Caskey, Damon V.
- 2026-08-11
-
- Grow a normalized command line as sequential arguments arrive.
  Capacity follows actual content, so total command size is bound
  only by addressable memory while individual arguments retain
  their dedicated validation limit.
*/
static void append_command_argument(
    char** command_line,
    size_t* length,
    size_t* capacity,
    const char* value
) {
    const size_t value_length = strlen(value);
    const size_t separator_length = *length ? 1 : 0;
    size_t required_capacity;
    size_t expanded_capacity;

    if(*length > SIZE_MAX - separator_length - 1
        || value_length
            > SIZE_MAX - *length - separator_length - 1) {
        borShutdown(1, "Command line exceeds addressable memory.\n");
        return;
    }

    required_capacity =
        *length + separator_length + value_length + 1;

    if(required_capacity > *capacity) {
        expanded_capacity = *capacity ? *capacity : 64;

        while(expanded_capacity < required_capacity) {
            if(expanded_capacity > SIZE_MAX / 2) {
                expanded_capacity = required_capacity;
                break;
            }

            expanded_capacity *= 2;
        }

        *command_line = realloc(*command_line, expanded_capacity);
        *capacity = expanded_capacity;
    }

    if(separator_length) {
        (*command_line)[(*length)++] = ' ';
    }

    memcpy(*command_line + *length, value, value_length);
    *length += value_length;
    (*command_line)[*length] = '\0';
}

/*
- Caskey, Damon V.
- 2026-08-11
-
- Read selectable player model names directly from the source
  line one item at a time. Apply and retain the complete runtime
  list without a whole-line or persistent save-field ceiling.
*/
static void load_playable_list(const char* command_line)
{
    const char* value;
    s_command_argument_reader reader;
    s_model* playermodel;
    char* stored_arguments = NULL;
    size_t stored_length = 0;
    size_t stored_capacity = 0;
    int index;

    if(!command_line
        || !command_argument_reader_initialize(
            &reader,
            command_line,
            0
        )
        || !command_argument_reader_next(&reader, &value)
        || stricmp(value, "allowselect") != 0) {
        return;
    }

    reset_playable_list(0);
    append_command_argument(
        &stored_arguments,
        &stored_length,
        &stored_capacity,
        "allowselect"
    );

    while(command_argument_reader_next(&reader, &value)) {
        playermodel = findmodel((char*)value);

        if(!playermodel) {
            free(stored_arguments);
            borShutdown(1, "Player model '%s' is not loaded.\n", value);
            return;
        }

        index = get_cached_model_index(playermodel->name);

        if(index == -1) {
            free(stored_arguments);
            borShutdown(1, "Player model '%s' is not cached.\n", value);
            return;
        }

        model_cache[index].selectable = 1;
        append_command_argument(
            &stored_arguments,
            &stored_length,
            &stored_capacity,
            value
        );
    }

    free(allowselect_args);
    allowselect_args = stored_arguments;
}

/*
* Caskey, Damon V.
* 2026-07-18 - Original author and date unknown, reworked 2026-07-06.
*
* Expand the model's configurable special-command table
* by one zero-initialized entry.
*
* Preserve an owned table until realloc succeeds. When a
* subclass still shares its parent's table, allocate and
* copy the inherited entries before appending so changes
* cannot modify or reallocate the parent table.
*/
static bool alloc_specials(s_model* newchar)
{
    s_com* expanded_specials;
    size_t expanded_count;
    bool owns_special_table;

    if(!newchar
        || newchar->specials_loaded < 0
        || newchar->specials_loaded == INT_MAX) {
        return false;
    }

    owns_special_table =
        (newchar->freetypes & MF_SPECIAL) != 0;

    /*
    * A subclass initially shares its parent's command
    * table. A positive inherited count without a table
    * is invalid and cannot be copied safely.
    */
    if(newchar->specials_loaded > 0
        && !newchar->special) {
        return false;
    }

    /*
    * Calculate the new size of the table, which is
    * the current number of loaded specials plus one
    * for the new entry. Check for overflow before
    * attempting to realloc the table.
    */
    expanded_count = (size_t)newchar->specials_loaded + 1;

    if(expanded_count > SIZE_MAX / sizeof(*expanded_specials)) {
        return false;
    }

    /*
    * Models that own their table may expand it in place.
    * A subclass that still shares an inherited table must
    * allocate and copy first, leaving the parent untouched.
    */

    if(owns_special_table) {
        expanded_specials = realloc(
            newchar->special,
            sizeof(*expanded_specials) * expanded_count
        );
    } else {
        expanded_specials = malloc(
            sizeof(*expanded_specials) * expanded_count
        );

        if(expanded_specials
            && newchar->specials_loaded > 0) {
            memcpy(
                expanded_specials,
                newchar->special,
                sizeof(*expanded_specials)
                    * (size_t)newchar->specials_loaded
            );
        }
    }

    if(!expanded_specials) {
        return false;
    }

    newchar->special = expanded_specials;
    newchar->freetypes |= MF_SPECIAL;

    memset(
        &newchar->special[newchar->specials_loaded],
        0,
        sizeof(*newchar->special)
    );

    return true;
}

void alloc_frames(s_anim *anim, int fcount)
{
    anim->sprite = malloc(fcount * sizeof(*anim->sprite));
    anim->delay = malloc(fcount * sizeof(*anim->delay));
    anim->vulnerable = malloc(fcount * sizeof(*anim->vulnerable));
    memset(anim->sprite, 0, fcount * sizeof(*anim->sprite));
    memset(anim->delay, 0, fcount * sizeof(*anim->delay));
    memset(anim->vulnerable, 0, fcount * sizeof(*anim->vulnerable));
}

void free_frames(s_anim *anim) {
    int i;

    if(anim->offset) {
        for(i = 0; i < anim->numframes; i++) {
            if(anim->offset[i]) {
                free(anim->offset[i]);
                anim->offset[i] = NULL;
            }
        }
        free(anim->offset);
        anim->offset = NULL;
    }

    if(anim->idle) {
        free(anim->idle);
        anim->idle = NULL;
    }

    if(anim->move) {
        for(i = 0; i < anim->numframes; i++) {
            if(anim->move[i]) {
                free(anim->move[i]);
                anim->move[i] = NULL;
            }
        }
        free(anim->move);
        anim->move = NULL;
    }

    if(anim->delay) {
        free(anim->delay);
        anim->delay = NULL;
    }

    if(anim->sprite) {
        free(anim->sprite);
        anim->sprite = NULL;
    }

    if(anim->platform) {
        free(anim->platform);
        anim->platform = NULL;
    }

    if(anim->vulnerable) {
        free(anim->vulnerable);
        anim->vulnerable = NULL;
    }

    if (anim->collision_attack) {
        
        for (i = 0; i < anim->numframes; i++) {
            collision_collection_free(anim->collision_attack[i]);
            anim->collision_attack[i] = NULL;
        }

        free(anim->collision_attack);
        anim->collision_attack = NULL;
    }

    if (anim->collision_body) {

        for (i = 0; i < anim->numframes; i++) {
            collision_collection_free(anim->collision_body[i]);
            anim->collision_body[i] = NULL;
        }

        free(anim->collision_body);
        anim->collision_body = NULL;
    }

    if (anim->collision_space) {
        
        for (i = 0; i < anim->numframes; i++) {
            collision_collection_free(anim->collision_space[i]);
            anim->collision_space[i] = NULL;
        }

        free(anim->collision_space);
        anim->collision_space = NULL;
    }

    if (anim->child_spawn) {
        child_spawn_free_list(*anim->child_spawn);
        anim->child_spawn = NULL;
    }

    if(anim->shadow) {
        free(anim->shadow);
        anim->shadow = NULL;
    }

    if(anim->shadow_coords) {
        free(anim->shadow_coords);
        anim->shadow_coords = NULL;
    }

    if(anim->sound) {
        for(i = 0; i < anim->numframes; i++) {
            frame_sound_collection_free(anim->sound[i]);
            anim->sound[i] = NULL;
        }

        free(anim->sound);
        anim->sound = NULL;
    }
    
    if(anim->drawmethods) {
        for(i = 0; i < anim->numframes; i++) {
            
            if(anim->drawmethods[i]) {
                free(anim->drawmethods[i]);
                anim->drawmethods[i] = NULL;
            }
        }
        free(anim->drawmethods);
        anim->drawmethods = NULL;
    }
}

void anim_list_delete(int index)
{
    s_anim_list head;
    head.next = anim_list;
    s_anim_list *list = &head;
    while(list && list->next)
    {
        if(list->next->anim->model_index == index)
        {
            s_anim_list *next = list->next->next;
            free_anim(list->next->anim);
            if(list->next == anim_list)
            {
                anim_list = next;
            }
            free(list->next);
            --anims_loaded;
            list->next = next;
        }
        else
        {
            list = list->next;
        }
    }
}

void free_anim(s_anim *anim)
{
    if(!anim)
    {
        return;
    }
    free_frames(anim);
    
	if (anim->projectile)
	{
		free(anim->projectile);
		anim->projectile = NULL;
	}
	if (anim->sub_entity_spawn)
	{
		free(anim->sub_entity_spawn);
		anim->sub_entity_spawn = NULL;
	}
	if (anim->sub_entity_summon)
	{
		free(anim->sub_entity_summon);
		anim->sub_entity_summon = NULL;
	}
	if (anim->weaponframe)
	{
		free(anim->weaponframe);
		anim->weaponframe = NULL;
	}
   
    free(anim);
}

int hasFreetype(s_model *m, e_ModelFreetype t)
{
    assert(m);
    return (m->freetypes & t) == t;
}

void addFreeType(s_model *m, e_ModelFreetype t)
{
    assert(m);
    m->freetypes |= t;
}

/*
* Caskey, Damon V.
* 2026-07-04
*
* Forward declaration for collision active mask scan.
*/
static int collision_get_lowest_active_index(uint64_t active_status);

/*
* Caskey, Damon V.
* 2020-03-30
*
* Load/unload sound IDs assigned to attack collisions.
*/
void cache_attack_hit_sounds(s_collision_collection* collection, int load) {
    s_collision_instance* collision = NULL;
    uint64_t active_status;
    int collision_index;

    if (!collection || !collection->active_status) {
        return;
    }

    active_status = collection->active_status;

    while (active_status) {
        collision_index = collision_get_lowest_active_index(active_status);
        active_status &= active_status - 1;

        collision = collection->slots[collision_index];

        if (!collision || !collision->attack) {
            continue;
        }

        cachesound(collision->attack->hitsound, load);
        cachesound(collision->attack->blocksound, load);
    }
}

void cache_model_sprites(s_model *m, int ld)
{
    int i, f;
    s_anim *anim;
    cachesprite(m->icon.def, ld);
    cachesprite(m->icon.die, ld);
    cachesprite(m->icon.get, ld);
    cachesprite(m->icon.mpmax, ld);
    cachesprite(m->icon.mphigh, ld);
    cachesprite(m->icon.mplow, ld);
    cachesprite(m->icon.mpmed, ld);
    cachesprite(m->icon.mpnone, ld);
    cachesprite(m->icon.pain, ld);
    cachesprite(m->icon.weapon, ld);
    cachesound(m->diesound, ld);
    for(i = 0; i < MAX_PLAYERS; i++)
    {
        cachesprite(m->player_arrow[i].sprite, ld);
    }

    //if(hasFreetype(model, MF_ANIMLIST)){
    for(i = 0; i < max_animations; i++)
    {
        anim = m->animation[i];
        if(anim)
        {
            for(f = 0; f < anim->numframes; f++)
            {
                cachesprite(anim->sprite[f], ld);
                if(anim->sound)
                {
                    frame_sound_cache_collection(anim->sound[f], ld);
                }
                
                // Hit sounds.
                if(anim->collision_attack && anim->collision_attack[f])
                {
                    cache_attack_hit_sounds(anim->collision_attack[f], ld);
                }
            }
        }
    }
}

/*
* Caskey, Damon V.
* 2026-08-21
*
* Execute a model lifecycle script with the model template,
* stable cache index, and model name exposed as local values.
*/
static void execute_model_lifecycle_script(Script *cs, s_model *model)
{
    ScriptVariant tempvar;

    if(!Script_IsInitialized(cs))
    {
        return;
    }

    ScriptVariant_Init(&tempvar);

    ScriptVariant_ChangeType(&tempvar, VT_PTR);
    tempvar.ptrVal = (VOID *)model;
    Script_Set_Local_Variant(cs, "model", &tempvar);

    ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
    tempvar.lVal = (LONG)model->index;
    Script_Set_Local_Variant(cs, "modelindex", &tempvar);

    ScriptVariant_ChangeType(&tempvar, VT_STR);
    tempvar.strVal = StrCache_CreateNewFrom(model_cache[model->index].name);
    Script_Set_Local_Variant(cs, "modelname", &tempvar);

    Script_Execute(cs);

    ScriptVariant_Clear(&tempvar);
    Script_Set_Local_Variant(cs, "model", &tempvar);
    Script_Set_Local_Variant(cs, "modelindex", &tempvar);
    Script_Set_Local_Variant(cs, "modelname", &tempvar);
}

/*
* Caskey, Damon V.
* 2026-08-21
*
* Execute model-owned setup before notifying the global
* model-load observer of a completed model parse.
*/
static void execute_model_load_scripts(s_model *model)
{
    s_modelcache *cache = &model_cache[model->index];

    execute_model_lifecycle_script(cache->load_script, model);
    execute_model_lifecycle_script(&model_load_script, model);
}

/*
* Caskey, Damon V.
* 2026-08-21
*
* Execute model-owned teardown before notifying the global
* model-unload observer and beginning native destruction.
*/
static void execute_model_unload_scripts(s_model *model)
{
    s_modelcache *cache = &model_cache[model->index];

    execute_model_lifecycle_script(cache->unload_script, model);
    execute_model_lifecycle_script(&model_unload_script, model);
}


// Unload single model from memory
int free_model(s_model *model)
{
    int i;
    s_modelcache *cache;

    if(!model)
    {
        return 0;
    }

    cache = &model_cache[model->index];

    /*
    * Ignore recursive removal from lifecycle callbacks and
    * attempts to remove a model that has not finished loading.
    */
    if(cache->lifecycle != MODEL_LIFECYCLE_LOADED)
    {
        return 0;
    }

    cache->lifecycle = MODEL_LIFECYCLE_UNLOAD_EVENT;
    execute_model_unload_scripts(model);
    cache->lifecycle = MODEL_LIFECYCLE_UNLOADING;

    printf("Unload '%s' ", model->name);

    if(hasFreetype(model, MF_ANIMLIST))
    {
        anim_list_delete(model->index);
    }

    printf(".");

    if(hasFreetype(model, MF_COLOURMAP))
    {
        for(i = 0; i < model->maps_loaded; i++)
        {
            if(model->colourmap[i] != NULL)
            {
                free(model->colourmap[i]);
                model->colourmap[i] = NULL;
            }
        }
        if(model->colourmap)
        {
            free(model->colourmap);
        }
        model->colourmap = NULL;
        model->maps_loaded = 0;
    }

    printf(".");

    if(hasFreetype(model, MF_PALETTE) && model->palette)
    {
        free(model->palette);
        model->palette = NULL;
    }
    printf(".");
    if(hasFreetype(model, MF_WEAPONS) && model->weapon_properties.weapon_list && model->weapon_properties.weapon_state & WEAPON_STATE_HAS_LIST)
    {
        free(model->weapon_properties.weapon_list);
        model->weapon_properties.weapon_list = NULL;
    }
    printf(".");
    if(hasFreetype(model, MF_BRANCH) && model->branch)
    {
        free(model->branch);
        model->branch = NULL;
    }
    printf(".");
    if(hasFreetype(model, MF_ANIMATION) && model->animation)
    {
        free(model->animation);
        model->animation = NULL;
    }
    printf(".");
    if(hasFreetype(model, MF_DEFENSE) && model->defense)
    {
        defense_free_object(model->defense);
        model->defense = NULL;
    }
    printf(".");
    if(hasFreetype(model, MF_OFF_FACTORS) && model->offense)
    {
        offense_free_object(model->offense);
        model->offense = NULL;
    }
    printf(".");
    if(hasFreetype(model, MF_SPECIAL) && model->special)
    {
        free(model->special);
        model->special = NULL;
    }
    printf(".");
    if(hasFreetype(model, MF_SMARTBOMB) && model->smartbomb)
    {
        free(model->smartbomb);
        model->smartbomb = NULL;
    }
    printf(".");

    if (hasFreetype(model, MF_CHILD_FOLLOW) && model->child_follow)
    {
        free(model->child_follow);
        model->child_follow = NULL;
    }
    printf(".");

    if(hasFreetype(model, MF_SCRIPTS))
    {
        clear_all_scripts(model->scripts, 2);
        free_all_scripts(&model->scripts);
    }
    printf(".");

    cache->model = NULL;
    deleteModel(model->name);
    printf(".");

    Script_Clear(cache->load_script, 1);
    Script_Clear(cache->unload_script, 1);
    cache->lifecycle = MODEL_LIFECYCLE_UNLOADED;

    printf("Done.\n");

    return models_loaded--;
}

// Unload all models and animations memory
void free_models()
{
    s_model *temp;

    while((temp = getFirstModel()))
    {
        free_model(temp);
    }

    // free animation ids
    if(animdowns)
    {
        free(animdowns);
        animdowns          = NULL;
    }
    if(animups)
    {
        free(animups);
        animups            = NULL;
    }
    if(animbackwalks)
    {
        free(animbackwalks);
        animbackwalks      = NULL;
    }
    if(animwalks)
    {
        free(animwalks);
        animwalks          = NULL;
    }
    if(animidles)
    {
        free(animidles);
        animidles          = NULL;
    }
    if(animspecials)
    {
        free(animspecials);
        animspecials       = NULL;
    }
    if(animattacks)
    {
        free(animattacks);
        animattacks        = NULL;
    }
    if(animfollows)
    {
        free(animfollows);
        animfollows        = NULL;
    }
    if(animpains)
    {
        free(animpains);
        animpains          = NULL;
    }
    if(animbackpains)
    {
        free(animbackpains);
        animbackpains      = NULL;
    }
    if(animfalls)
    {
        free(animfalls);
        animfalls          = NULL;
    }
    if(animbackfalls)
    {
        free(animbackfalls);
        animbackfalls      = NULL;
    }
    if(animrises)
    {
        free(animrises);
        animrises          = NULL;
    }
    if(animbackrises)
    {
        free(animbackrises);
        animbackrises          = NULL;
    }
    if(animriseattacks)
    {
        free(animriseattacks);
        animriseattacks    = NULL;
    }
    if(animbackriseattacks)
    {
        free(animbackriseattacks);
        animbackriseattacks    = NULL;
    }
    if(animblkpains)
    {
        free(animblkpains);
        animblkpains       = NULL;
    }
    if(animbackblkpains)
    {
        free(animbackblkpains);
        animbackblkpains       = NULL;
    }
    if(animdies)
    {
        free(animdies);
        animdies           = NULL;
    }
    if(animbackdies)
    {
        free(animbackdies);
        animbackdies        = NULL;
    }
    if(ai_attack_choices)
    {
        free(ai_attack_choices);
        ai_attack_choices = NULL;
        ai_attack_choice_capacity = 0;
    }
}


s_anim *alloc_anim()
{
    static int animindex = 0;
    s_anim_list *curr = NULL, *head = NULL;
    curr = malloc(sizeof(*curr));
    curr->anim = malloc(sizeof(*curr->anim));
    if(curr == NULL || curr->anim == NULL)
    {
        return NULL;
    }
    memset(curr->anim, 0, sizeof(*curr->anim));
    curr->anim->index = animindex++;
    if(anim_list == NULL)
    {
        anim_list = curr;
        anim_list->next = NULL;
    }
    else
    {
        head = anim_list;
        anim_list = curr;
        anim_list->next = head;
    }
    ++anims_loaded;
    return anim_list->anim;
}

// Caskey, Damon V.
// 2020-04-09
// 
// Remove a meta data list from memory.
//
// TODO (2020-04-09): List not yet implemented. 
void meta_data_free_list(s_meta_data* head)
{
    free(head);
}

/*
* Caskey, Damon V.
* 2022-06-22
* 
* Accept string and return function reference for 
* damage taking behavior.
*/
entity_takedamage_function takedamage_get_reference_from_argument(char* value)
{
    entity_takedamage_function result = NULL;

    if (stricmp(value, "none") == 0)
    {
        result = NULL;
    }
    else if (stricmp(value, "arrow") == 0)
    {
        result = &arrow_takedamage;
    }
    else if (stricmp(value, "biker") == 0)
    {
        result = &biker_takedamage;
    }
    else if (stricmp(value, "common") == 0)
    {
        result = &common_takedamage;
    }
    else if (stricmp(value, "obstacle") == 0)
    {
        result = &obstacle_takedamage;
    }
    else if (stricmp(value, "player") == 0)
    {
        result = &player_takedamage;
    }

    return result;
}

/* **** Child Follow **** */
s_child_follow* child_follow_getsert_property(s_child_follow** acting_object)
{
    if (!(*acting_object))
    {
        *acting_object = child_follow_allocate_object();
    }

    return *acting_object;
}


s_child_follow* child_follow_allocate_object()
{
    s_child_follow* result;
    const size_t alloc_size = sizeof(*result);

    /* Allocate memory and get pointer. */
    result = calloc(1, alloc_size);

    *result = (s_child_follow){
        .direction_adjust_config = DIRECTION_ADJUST_TOWARD,
        .direction_adjust_range = {
            .base = {.max = MAX_INT, .min = MIN_INT },
            .x = {.max = MAX_INT, .min = MIN_INT },
            .y = {.max = MAX_INT, .min = MIN_INT },
            .z = {.max = MAX_INT, .min = MIN_INT }
        },
        .recall_animation = ANI_RESPAWN,
        .recall_range = {

            /*
            * X and Z  min / max defaults are overwritten
            * by Idle range X max for legacy compatability.
            */

            .base = {.max = MAX_INT, .min = MIN_INT },
            .x = {.max = MAX_INT, .min = MIN_INT },
            .y = {.max = MAX_INT, .min = MIN_INT },
            .z = {.max = MAX_INT, .min = MIN_INT }
        },
        .follow_range = {

            /*
            * X and Z  min / max defaults are overwritten
            * by Idle range X min for legacy compatability.
            */

            .base = {.max = MAX_INT, .min = MIN_INT },
            .x = {.max = MAX_INT, .min = MIN_INT },
            .y = {.max = MAX_INT, .min = MIN_INT },
            .z = {.max = MAX_INT, .min = MIN_INT }
        },
        .follow_run_range = {
            .base = {.max = MAX_INT, .min = MIN_INT },
            .x = {.max = MAX_INT, .min = MIN_INT },
            .y = {.max = MAX_INT, .min = MIN_INT },
            .z = {.max = MAX_INT, .min = MIN_INT }
        }
    };

    return result;
}

/* **** Child Spawn **** */

/*
* Caskey, Damon V.
* 2023-02-01
*
* Read a text argument for child color
* and get constant or color index.
*/
int child_spawn_get_color_from_argument(char* filename, char* command, char* value)
{
    e_model_copy result = MODEL_COPY_FLAG_NONE;

    if (stricmp(value, "parent_index") == 0)
    {
        result = COLORSET_INDEX_PARENT_INDEX;
    }
    else if (stricmp(value, "parent_table") == 0)
    {
        result = COLORSET_INDEX_PARENT_TABLE;
    }    
    else
    {
        result = getValidInt(value, filename, command);
    }

    return result;
}


/*
* Caskey, Damon V.
* 2022-05-27
*
* Read a text argument for child spawn config
* flag and output appropriate constant.
*/
e_child_spawn_config child_spawn_get_config_bit_from_argument(
    const char* value
)
{
    e_child_spawn_config result = CHILD_SPAWN_CONFIG_NONE;

    if (stricmp(value, "none") == 0)
    {
        result = CHILD_SPAWN_CONFIG_NONE;
    }
    else if (stricmp(value, "autokill_animation") == 0)
    {
        result = CHILD_SPAWN_CONFIG_AUTOKILL_ANIMATION;
    }
    else if (stricmp(value, "autokill_hit") == 0)
    {
        result = CHILD_SPAWN_CONFIG_AUTOKILL_HIT;
    }
    else if (stricmp(value, "behavior_bomb") == 0)
    {
        result = CHILD_SPAWN_CONFIG_BEHAVIOR_BOMB;
    }
    else if (stricmp(value, "behavior_shot") == 0)
    {
        result = CHILD_SPAWN_CONFIG_BEHAVIOR_SHOT;
    }
    else if (stricmp(value, "explode") == 0)
    {
        result = CHILD_SPAWN_CONFIG_EXPLODE;
    }
    else if (stricmp(value, "faction_damage_parameter") == 0)
    {
        result = CHILD_SPAWN_CONFIG_FACTION_DAMAGE_PARAMETER;
    }
    else if (stricmp(value, "faction_damage_parent") == 0)
    {
        result = CHILD_SPAWN_CONFIG_FACTION_DAMAGE_PARENT;
    }
    else if (stricmp(value, "faction_hostile_parameter") == 0)
    {
        result = CHILD_SPAWN_CONFIG_FACTION_HOSTILE_PARAMETER;
    }
    else if (stricmp(value, "faction_hostile_parent") == 0)
    {
        result = CHILD_SPAWN_CONFIG_FACTION_HOSTILE_PARENT;
    }
    else if (stricmp(value, "faction_indirect_parameter") == 0)
    {
        result = CHILD_SPAWN_CONFIG_FACTION_INDIRECT_PARAMETER;
    }
    else if (stricmp(value, "faction_indirect_parent") == 0)
    {
        result = CHILD_SPAWN_CONFIG_FACTION_INDIRECT_PARENT;
    }
    else if (stricmp(value, "faction_member_parameter") == 0)
    {
        result = CHILD_SPAWN_CONFIG_FACTION_MEMBER_PARAMETER;
    }
    else if (stricmp(value, "faction_member_parent") == 0)
    {
        result = CHILD_SPAWN_CONFIG_FACTION_MEMBER_PARENT;
    }
    else if (stricmp(value, "gravity_off") == 0)
    {
        result = CHILD_SPAWN_CONFIG_GRAVITY_OFF;
    }
    else if (stricmp(value, "launch_throw") == 0)
    {
        result = CHILD_SPAWN_CONFIG_LAUNCH_THROW;
    }
    else if (stricmp(value, "launch_toss") == 0)
    {
        result = CHILD_SPAWN_CONFIG_LAUNCH_TOSS;
    }
    else if (stricmp(value, "offense_parent") == 0)
    {
        result = CHILD_SPAWN_CONFIG_OFFENSE_PARENT;
    }
    else if (stricmp(value, "position_level") == 0)
    {
        result = CHILD_SPAWN_CONFIG_POSITION_LEVEL;
    }
    else if (stricmp(value, "takedamage_parameter") == 0)
    {
        result = CHILD_SPAWN_CONFIG_TAKEDAMAGE_PARAMETER;
    }
    else if (stricmp(value, "relationship_child") == 0)
    {
        result = CHILD_SPAWN_CONFIG_RELATIONSHIP_CHILD;
    }
    else if (stricmp(value, "relationship_owner") == 0)
    {
        result = CHILD_SPAWN_CONFIG_RELATIONSHIP_OWNER;
    }
    else if (stricmp(value, "relationship_parent") == 0)
    {
        result = CHILD_SPAWN_CONFIG_RELATIONSHIP_PARENT;
    }
    else
    {
        printf("\n\n Unknown child_spawn_config argument (%s). Ignoring.\n", value);
    }

    return result;
}

/*
* Caskey, Damon V.
* 2022-05-27
*
* Read text argument lists, updates bit flags
* and outputs integer. Accepts existing
* argument as a default.
*/
e_child_spawn_config child_spawn_get_config_argument(
    const char* command_line,
    e_child_spawn_config config_current
)
{
    const char* value;
    s_command_argument_reader reader;
    e_child_spawn_config result = config_current;

    command_argument_reader_initialize(&reader, command_line, 1);

    while(command_argument_reader_next(&reader, &value)) {
        result |= child_spawn_get_config_bit_from_argument(value);
    }

    return result;
}



/*
* Caskey, Damon V.
* 2022-05-26
*
* Allocate a blank child spawn object
* and return its pointer. Does not
* allocate sub-objects.
*/
s_child_spawn* child_spawn_allocate_object()
{
    s_child_spawn* result;
    const size_t alloc_size = sizeof(*result);

    /* Allocate memory and get pointer. */
    result = calloc(1, alloc_size);

    result->aimove = AIMOVE_SPECIAL_DEFAULT;

    result->next = NULL;
    return result;
}

/*
* Caskey, Damon V.
* 2022-05-26
*
* Allocate new child spawn node and append it to
* end of child_spawn linked list. If no lists exists
* yet, the new node becomes head of a new list.
*
* First step in adding another child_spawn instance.
*
* Returns pointer to new node.
*/
s_child_spawn* child_spawn_append_node(s_child_spawn* head)
{
    /* Allocate node. */
    s_child_spawn* new_node = child_spawn_allocate_object();
    s_child_spawn* last = head;

    /*
    * New node is going to be the last node in
    * list, so set its next as NULL.
    */
    new_node->next = NULL;

    /*
    * If there wasn't already a list, the
    * new node is our head. We are done and
    * can return the new node pointer.
    */

    if (head == NULL)
    {
        head = new_node;

        return new_node;
    }

    /*
    * If we got here, there was already a
    * list in place. Iterate to its last
    * node.
    */

    while (last->next != NULL)
    {
        last = last->next;
    }

    /*
    * Populate existing last node's next
    * with new node pointer. The new node
    * is now the last node in list.
    */

    last->next = new_node;

    return new_node;
}

/*
* Caskey, Damon V
* 2022-05-26
*
* Allocate new child spawn list with same 
* values as source. Returns pointer to head 
* of new list.
*/
s_child_spawn* child_spawn_clone_list(s_child_spawn* source_head)
{
    s_child_spawn* source_cursor = NULL;
    s_child_spawn* clone_head = NULL;
    s_child_spawn* clone_node = NULL;

    /* Head is null? Get out now. */
    if (source_head == NULL)
    {
        return source_cursor;
    }

    source_cursor = source_head;

    while (source_cursor != NULL)
    {
        clone_node = child_spawn_append_node(clone_head);
        
        /*
        * Populate head if NULL so we
        * have one for the next cycle.
        */
        if (clone_head == NULL)
        {
            clone_head = clone_node;
        }

        /* 
        * Copy the values. We start with
        * a memcopy to get most properties
        * and then manually update the
        * rest as needed.
        */

        *clone_node = *source_cursor;
                
        /* 
        * Clear clone's next value.
        */
        clone_node->next = NULL;
        
        
        source_cursor = source_cursor->next;
    }

    return clone_head;
}

/*
* Caskey, Damon V
* 2020-03-10
*
* Send all child spawn list data to log for debugging.
*/
void child_spawn_dump_list(s_child_spawn* head)
{
    printf("\n\n -- Child Spawn List (head: %p) Dump --", head);

    s_child_spawn* cursor;
    int count = 0;

    cursor = head;

    while (cursor != NULL)
    {
        count++;

        printf("\n\n\t Node: %p", cursor);
              
        child_spawn_dump_object(cursor);

        cursor = cursor->next;
    }

    printf("\n\n %d nodes.", count);
    printf("\n\n -- Child Spawn List (head: %p) dump complete! -- \n", head);
}

/*
* Caskey, Damon V
* 2020-03-10
*
* Send child spawn object data to log for debugging.
*/
void child_spawn_dump_object(s_child_spawn* object)
{
    const int space_label = 20;

    printf("\n\n -- Child Spawn object (%p) Dump --", object);   

    printf("\n\t\t %-*s %d", space_label, "->aimove:", object->aimove);
    printf("\n\t\t %-*s %d", space_label, "->autokill:", object->autokill);
    printf("\n\t\t %-*s %p", space_label, "->bind:", object->bind);

    if (object->bind)
    {
        //bind_dump_object(cursor->bind);
    }
    
    printf("\n\t\t %-*s %d", space_label, "->candamage:", object->candamage);
    printf("\n\t\t %-*s %d", space_label, "->color:", object->color);
    printf("\n\t\t %-*s %d", space_label, "->config:", object->config);
    printf("\n\t\t %-*s %d", space_label, "->direction_adjust:", object->direction_adjust);
    printf("\n\t\t %-*s %d", space_label, "->hostile:", object->hostile);
    printf("\n\t\t %-*s %d", space_label, "->index:", object->index);
    printf("\n\t\t %-*s %d", space_label, "->model_index:", object->model_index);
    printf("\n\t\t %-*s %d", space_label, "->move_config_flags:", object->move_config_flags);
    printf("\n\t\t %-*s %p", space_label, "->next:", object->next);
    printf("\n\t\t %-*s %d, %d, %d", space_label, "->position:", object->position.x, object->position.y, object->position.z);
    printf("\n\t\t %-*s %d", space_label, "->projectilehit:", object->projectilehit);
    printf("\n\t\t %-*s %p", space_label, "->takedamage:", object->takedamage);
    printf("\n\t\t %-*s %f, %f, %f", space_label, "->velocity:", object->velocity.x, object->velocity.y, object->velocity.z);
    

    printf("\n\n -- Child Spawn object (%p) dump complete! -- \n", object);
}

/*
* Caskey, Damon V.
* 2022-05-26
*
* Find a child spawn node by index and return 
* pointer, or NULL if no match found.
*/
s_child_spawn* child_spawn_find_node_index(s_child_spawn* head, int index)
{
    s_child_spawn* current = NULL;

    /*
    * Starting from head node, iterate through
    * all nodes and compare their index
    * property to index argument.
    * 
    * If we found a match, return the node
    * pointer. 
    */

    current = head;

    while (current != NULL)
    {        
        if (current->index == index)
        {
            return current;
        }
        
        current = current->next;
    }

    /*
    * If we got here, find failed.
    * Just return NULL.
    */
    return NULL;
}

/*
* Caskey, Damon V.
* 2022-05-26
*
* Clear a child spawn linked list from memory.
*/
void child_spawn_free_list(s_child_spawn* head)
{
    s_child_spawn* cursor = NULL;
    s_child_spawn* next = NULL;

    /*
    * Starting from head node, iterate through
    * all nodes and free them.
    */
    cursor = head;

    while (cursor != NULL)
    {
        /*
        * We still need the next member after we
        * delete object, so we'll store it in a 
        * temp var.
        */

        next = cursor->next;

        /* Free the current object. */
        child_spawn_free_node(cursor);

        cursor = next;
    }
}

/*
* Caskey, Damon V.
* 2022-05-26
*
* Clear a single child spawn object from memory.
* Note this does NOT remove node from list.
* Be careful not to create a dangling pointer!
*/
void child_spawn_free_node(s_child_spawn* target)
{
    /* Free sub objects. */

    if (target->bind)
    {
        //bind_free_object(target->bind);
        //target->attack = NULL;
    }        

    /* Free the structure. */
    free(target);
}

/*
* 2020-02-23
* Caskey, Damon V
*
* Get pointer to object for modification. Used when
* loading a model and reading in attack properties.
*/
s_child_spawn* child_spawn_upsert_property(s_child_spawn** head, int index)
{
    // printf("\n\t child_spawn_upsert_bind_property(%p, %d)", *head, index);

    s_child_spawn* temp_object_current;

    /*
    * 1. First we need to know index.
                *  -- temp_collision_index

                * 2. Look for index and get pointer (found or allocated).

                * Get the node we want to work on by searching
                * for a matched index. In most cases, this will
                * just be the head node.
    */

    temp_object_current = child_spawn_upsert_index(*head, index);

    /*
    * If head is NULL, this must be the first allocated
    * collision for current frame. Populate head with
    * current so we have a head for the next pass.
    */

    if (*head == NULL)
    {
        *head = temp_object_current;
    } 

    /* Return pointer to the attack structure. */
    return temp_object_current;
}

/*
* Caskey, Damon V.
* 2022-05-27
*
* Find a child spawn node by index, or append a new node
* with target index if no match is found. Returns pointer
* to found or appended node.
*/
s_child_spawn* child_spawn_upsert_index(s_child_spawn* head, int index)
{
    s_child_spawn* result = NULL;

    /* Run index search. */
    result = child_spawn_find_node_index(head, index);

    /*
    * If we couldn't find an index match, lets add
    * a node and apply the index we wanted.
    */
    if (!result)
    {
        result = child_spawn_append_node(head);
        result->index = index;
    }

    return result;
}

/*
* Caskey, Damon V.
* 2022-05-27
*
* Allocate and apply child spawn settings to target frame.
*/
void child_spawn_initialize_frame_property(s_addframe_data* data, ptrdiff_t frame)
{
    s_child_spawn* temp_object;
    size_t memory_size;

    if (!data->child_spawn)
    {
        return;
    }
    
    /*
    * If object is not allocated yet, we need to allocate
    * an array of object pointers (one element for each
    * animation frame). If the frame has an object, its
    * object property is populated with pointer to head
    * of a linked list of objects.
    */
    if (!data->animation->child_spawn)
    {
        memory_size = data->framecount * sizeof(*data->animation->child_spawn);

        data->animation->child_spawn = malloc(memory_size);
        memset(data->animation->child_spawn, 0, memory_size);
    }

    //printf("\n\n child_spawn_initialize_frame_property");

    child_spawn_dump_list(data->child_spawn);

    /*
    * Clone source list and populate frame's object
    * property with the pointer to clone list head.
    */
    temp_object = child_spawn_clone_list(data->child_spawn);
        
    /* Frame object property is head of object list. */
    data->animation->child_spawn[frame] = temp_object;

    
    //printf("\n\t data->animation->child_spawn[%d]: %p \n", frame, data->animation->child_spawn[frame]);
}

/*
* Caskey, Damon V.
* 2022-05-29
* 
* Accept head of a lisst of child spawns.
* Iterate through list and apply properties 
* to spawn child entities.
*/
void child_spawn_execute_list(s_child_spawn* head, entity* parent)
{
    s_child_spawn* cursor = NULL;
    s_child_spawn* next = NULL;

    /*
    * Starting from head node, iterate through
    * all nodes and free them.
    */
    cursor = head;

    while (cursor != NULL)
    {
        /*
        * We still need the next member after we
        * delete object, so we'll store it in a
        * temp var.
        */

        next = cursor->next;

        /* Free the current object. */
        child_spawn_execute_object(cursor, parent);

        cursor = next;
    }
}

/*
* Caskey, Damon V.
* 2022-05-29
* 
* Accept pointer to node in list of child
* spawns. Apply properties to spawn
* a child entity. Returns pointer to
* spawned entity.
*/
entity* child_spawn_execute_object(s_child_spawn* object, entity* parent)
{
    printf("\n\n child_spawn_execute_object(object: %p, parent: %p)", object, parent);
    
    int i = 0;
    entity* child_entity = NULL;
    s_axis_principal_float position;
    e_direction direction = DIRECTION_RIGHT;
    s_model* child_model = NULL;

    if (object->model_index == MODEL_INDEX_NONE || !parent)
    {
        return NULL;
    }  
    
    child_model = model_cache[object->model_index].model;

    printf("\n\t object->model_index: %d", object->model_index);
    printf("\n\t child_model: %p", child_model);
    
    
    position.x = parent->position.x;
    position.y = parent->position.y + object->position.y;
    position.z = parent->position.z + object->position.z;

    printf("\n\t Child: x: %f, y: %f, z: %f", position.x, position.y, position.z);
    
    /*
    * Spawn entity using model pointer. If the spawn 
    * fails then we exit immediately.
    */

    child_entity = spawn(position.x, position.z, position.y, direction, NULL, MODEL_INDEX_NONE, child_model);

    printf("\n\t child_entity: %p", child_entity);

    if (!child_entity)
    {
        return NULL;
    }

    /*
    * Let's set up the spawn position. Reverse X when
    * parent faces left.
    *
    * Apply default X position if creator did not give
    * us a value.
    */
    direction = direction_get_adjustment_result(child_entity, parent, object->direction_adjust);

    printf("\n\t direction: %d", direction);
    printf("\n\t Parent: x: %f, y: %f, z: %f", parent->position.x, parent->position.y, parent->position.z);

    if (direction == DIRECTION_RIGHT && object->config & ~(CHILD_SPAWN_CONFIG_POSITION_LEVEL | CHILD_SPAWN_CONFIG_POSITION_SCREEN))
    {
        position.x = parent->position.x + object->position.x;
    }
    else
    {
        position.x = parent->position.x - object->position.x;
    }

    child_entity->direction = direction;
    child_entity->position.x = position.x;
    

    child_entity->spawntype = SPAWN_TYPE_CHILD;

    /*
    * Populate relationship properties as
    * requested.
    */

    if (object->config & CHILD_SPAWN_CONFIG_RELATIONSHIP_CHILD)
    {
        parent->subentity = parent;
    }

    if (object->config & CHILD_SPAWN_CONFIG_RELATIONSHIP_OWNER)
    {
        child_entity->owner = parent;
    }

    if (object->config & CHILD_SPAWN_CONFIG_RELATIONSHIP_PARENT)
    {
        child_entity->parent = parent;
    }

    /*
    * Handle initial velocity. We will start with
    * an initial velocity. Then may copy velocity
    * to child's speed settings, or apply a toss 
    * effect (to throw in an arc assuming gravity).
    */

    child_entity->velocity = object->velocity;
        
    if (object->config & CHILD_SPAWN_CONFIG_LAUNCH_THROW)
    {
        /* For throw effect, we copy velocity to child's model speed. */
        child_entity->modeldata.speed = object->velocity;
    }

    if (object->config & CHILD_SPAWN_CONFIG_LAUNCH_TOSS && object->velocity.y)
    {
        /* To toss, we use toss function with Y velocity. */
        toss(child_entity, object->velocity.y);
    }

    /*
    * Set up basic behavior packages.
    */

    if (object->config & CHILD_SPAWN_CONFIG_BEHAVIOR_SHOT)
    {
        child_entity->modeldata.aimove = AIMOVE1_ARROW;
        child_entity->attacking = ATTACKING_ACTIVE;
        //->takedamage = arrow_takedamage;
        child_entity->modeldata.aiattack = AIATTACK1_NOATTACK;
        child_entity->nograb = 1;                
    }

    if (object->config & CHILD_SPAWN_CONFIG_BEHAVIOR_BOMB)
    {
        child_entity->modeldata.aimove = AIMOVE1_BOMB;
        child_entity->attacking = ATTACKING_ACTIVE;
        //child_entity->takedamage = common_takedamage;
        child_entity->modeldata.aiattack = AIATTACK1_NOATTACK;
        child_entity->nograb = 1;
        child_entity->toexplode |= (EXPLODE_PREPARE_TOUCH | EXPLODE_PREPARE_GROUND);               
    }

    //if (object->config & CHILD_SPAWN_CONFIG_TAKEDAMAGE_PARAMETER)
    //{
    //    child_entity->takedamage = object->takedamage;
    //}

    /*
    * If requested, apply AI Flags.
    */
    if (!(object->aimove & AIMOVE_SPECIAL_DEFAULT))
    {
        child_entity->modeldata.aimove = object->aimove;
    }

    printf("\n\t child_entity->modeldata.aimove: %d", child_entity->modeldata.aimove);

    /*
    * Copy offense values from parent offense settings
    * to projectile enity if requested.
    */ 
    if (object->config & CHILD_SPAWN_CONFIG_OFFENSE_PARENT)
    {
        /*
        * Parent might not have offense. In that 
        * case, if the child also has no offense
        * we can just do nothing and not waste
        * any memory. If the child does have an
        * offense, we'll need to overwrite it
        * with default so it matches parent.
        */
        
        if (!parent->offense)
        {
            if (child_entity->offense)
            {
                memcpy(child_entity->offense, &default_offense, sizeof(*child_entity->offense) * max_attack_types);
            }
        }
        else
        {
            /*
            * Make sure child has memory allocated.
            */

            if (!child_entity->offense)
            {
                child_entity->offense = offense_allocate_object();
            }
                        
            memcpy(child_entity->offense, parent->offense, sizeof(*child_entity->offense) * max_attack_types);
            
        }
    }
    
    /* Apply color adjustment. */
    if (object->color == COLORSET_INDEX_PARENT_INDEX)
    {        
        for (i = 0; i < parent->modeldata.maps_loaded; i++)
        {
            if (parent->colourmap == parent->modeldata.colourmap[i])
            {
                ent_set_colourmap(child_entity, i);
                break;
            }
        }
    }
    else if (object->color == COLORSET_INDEX_PARENT_TABLE)
    {
        child_entity->colourmap = parent->colourmap;
    }
    else
    {
        ent_set_colourmap(child_entity, object->color);
    }

    /* Populate common behavior flags. */
    child_entity->think = common_think;
    child_entity->nextthink = _time + 1;
    child_entity->trymove = NULL;    
    child_entity->takeaction = NULL;
    child_entity->speedmul = 2;

    /* Populate autokill. */
    if (object->config & CHILD_SPAWN_CONFIG_AUTOKILL_ANIMATION)
    {
        child_entity->autokill |= AUTOKILL_ANIMATION_COMPLETE;
    }

    if (object->config & CHILD_SPAWN_CONFIG_AUTOKILL_HIT)
    {
        child_entity->autokill |= AUTOKILL_ATTACK_HIT;
    }

    /* 
    * Handle type based damage and hostilty. Copy from
    * parent or parameter on request.
    * 
    * If player damage turned off, remove player type.
    */

    if (object->config & CHILD_SPAWN_CONFIG_FACTION_DAMAGE_PARAMETER)
    {
        child_entity->faction.type_damage_direct = object->candamage;
    }

    if (object->config & CHILD_SPAWN_CONFIG_FACTION_DAMAGE_PARENT)
    {
        child_entity->faction.type_damage_direct = parent->faction.type_damage_direct;
    }

    if (object->config & CHILD_SPAWN_CONFIG_FACTION_HOSTILE_PARAMETER)
    {
        child_entity->faction.type_hostile = object->hostile;
    }

    if (object->config & CHILD_SPAWN_CONFIG_FACTION_HOSTILE_PARENT)
    {
        child_entity->faction.type_hostile = parent->faction.type_hostile;
    }

    if (object->config & CHILD_SPAWN_CONFIG_FACTION_INDIRECT_PARAMETER)
    {
        child_entity->faction.type_damage_indirect = object->projectilehit;
    }

    if (object->config & CHILD_SPAWN_CONFIG_FACTION_INDIRECT_PARENT)
    {
        child_entity->faction.type_damage_indirect = parent->faction.type_damage_indirect;
    }
    
    if ((parent->modeldata.type & TYPE_PLAYER) && ((level && level->nohit == DAMAGE_FROM_PLAYER_OFF) || savedata.mode))
    {
        child_entity->faction.type_hostile &= ~TYPE_PLAYER;
        child_entity->faction.type_damage_direct &= ~TYPE_PLAYER;
    }

    printf("\n\t child_entity->faction.type_hostile: %d", child_entity->faction.type_hostile);
    printf("\n\t child_entity->faction.type_damage_direct: %d", child_entity->faction.type_damage_direct);
    printf("\n\t child_entity->faction.type_damage_indirect: %d", child_entity->faction.type_damage_indirect);

    /* 
    * Apply any move constraints. 
    */

    if (object->config & CHILD_SPAWN_CONFIG_MOVE_CONFIG_PARAMETER)
    {
        child_entity->modeldata.move_config_flags = object->move_config_flags;
    }

    if (object->config & CHILD_SPAWN_CONFIG_MOVE_CONFIG_PARENT)
    {
        child_entity->modeldata.move_config_flags = parent->modeldata.move_config_flags;
    }

    printf("\n\t child_entity->modeldata.move_config_flags: %d", child_entity->modeldata.move_config_flags);

    /*
    * Execute event scripts.
    */

    // execute_on_pre_child_spawn_script(parent, child_entity, object);
    execute_onspawn_script(child_entity);
    // execute_on_post_child_spawn_script(parent, child_entity, object);

    printf("\n\t return: %p", child_entity);

    return child_entity;
}

/* **** Frame Sound Support Functions */

/*
* Caskey, Damon V.
* 2026-08-07
*
* Validate an author-facing frame sound slot index.
*/
static bool frame_sound_validate_slot_index(const int index) {
    return index >= 0 && index < MAX_FRAME_SOUNDS_PER_FRAME;
}

/*
* Caskey, Damon V.
* 2026-08-08
*
* Parse and validate an author-facing mixer channel.
*/
static bool frame_sound_parse_channel(
    const char* const text,
    int* const result
) {
    s_command_token token;
    uint64_t channel;

    if (!text || !result) {
        return false;
    }

    token.text = text;
    token.length = strlen(text);

    if (!command_token_get_uint64(&token, &channel)
        || channel >= SOUND_CHANNEL_COUNT_MAX) {
        return false;
    }

    *result = (int)channel;
    return true;
}

/*
* Caskey, Damon V.
* 2026-08-07
*
* Return the active mask bit for a frame sound slot.
*/
static uint64_t frame_sound_get_slot_mask(const int index) {
    return UINT64_C(1) << (uint64_t)index;
}

/*
* Caskey, Damon V.
* 2026-08-07
*
* Return an inclusive mask for a validated pair of frame
* sound slot indexes without ever shifting by 64 bits.
*/
static uint64_t frame_sound_get_slot_range_mask(const int min, const int max) {
    const uint64_t lower_mask = UINT64_MAX << (uint64_t)min;
    const uint64_t upper_mask = max == MAX_FRAME_SOUNDS_PER_FRAME - 1
        ? UINT64_MAX
        : (UINT64_C(1) << ((uint64_t)max + 1U)) - 1U;

    return lower_mask & upper_mask;
}

/*
* Caskey, Damon V.
* 2026-08-07
*
* Return the lowest active slot in a non-zero mask.
*/
static int frame_sound_get_lowest_active_index(uint64_t active_status) {
    int sound_index = 0;

    while (!(active_status & UINT64_C(1))) {
        active_status >>= 1;
        sound_index++;
    }

    return sound_index;
}

/*
* Caskey, Damon V.
* 2026-08-08
*
* Translate one author-facing sound group name.
*/
static bool sound_group_get_flag_from_string(
    const char* const value,
    sound_group_mask_t* const result
) {
    static const struct {
        const char* text_name;
        sound_group_mask_t flag;
    } flag_lookup_table[] = {
        { "none", SOUND_GROUP_NONE },
        { "all", SOUND_GROUP_ALL },
        { "all0", SOUND_GROUP_ALL_0 },
        { "all1", SOUND_GROUP_ALL_1 },
        { "a", SOUND_GROUP_A },
        { "b", SOUND_GROUP_B },
        { "c", SOUND_GROUP_C },
        { "d", SOUND_GROUP_D },
        { "e", SOUND_GROUP_E },
        { "f", SOUND_GROUP_F },
        { "g", SOUND_GROUP_G },
        { "h", SOUND_GROUP_H },
        { "i", SOUND_GROUP_I },
        { "j", SOUND_GROUP_J },
        { "k", SOUND_GROUP_K },
        { "l", SOUND_GROUP_L },
        { "m", SOUND_GROUP_M },
        { "n", SOUND_GROUP_N },
        { "o", SOUND_GROUP_O },
        { "p", SOUND_GROUP_P },
        { "q", SOUND_GROUP_Q },
        { "r", SOUND_GROUP_R },
        { "s", SOUND_GROUP_S },
        { "t", SOUND_GROUP_T },
        { "u", SOUND_GROUP_U },
        { "v", SOUND_GROUP_V },
        { "w", SOUND_GROUP_W },
        { "x", SOUND_GROUP_X },
        { "y", SOUND_GROUP_Y },
        { "z", SOUND_GROUP_Z },
        { "a1", SOUND_GROUP_A1 },
        { "b1", SOUND_GROUP_B1 },
        { "c1", SOUND_GROUP_C1 },
        { "d1", SOUND_GROUP_D1 },
        { "e1", SOUND_GROUP_E1 },
        { "f1", SOUND_GROUP_F1 },
        { "g1", SOUND_GROUP_G1 },
        { "h1", SOUND_GROUP_H1 },
        { "i1", SOUND_GROUP_I1 },
        { "j1", SOUND_GROUP_J1 },
        { "k1", SOUND_GROUP_K1 },
        { "l1", SOUND_GROUP_L1 },
        { "m1", SOUND_GROUP_M1 },
        { "n1", SOUND_GROUP_N1 },
        { "o1", SOUND_GROUP_O1 },
        { "p1", SOUND_GROUP_P1 },
        { "q1", SOUND_GROUP_Q1 },
        { "r1", SOUND_GROUP_R1 },
        { "s1", SOUND_GROUP_S1 },
        { "t1", SOUND_GROUP_T1 },
        { "u1", SOUND_GROUP_U1 },
        { "v1", SOUND_GROUP_V1 },
        { "w1", SOUND_GROUP_W1 },
        { "x1", SOUND_GROUP_X1 },
        { "y1", SOUND_GROUP_Y1 },
        { "z1", SOUND_GROUP_Z1 }
    };
    size_t flag_index;

    if (!value || !value[0] || !result) {
        return false;
    }

    for (flag_index = 0;
        flag_index < sizeof(flag_lookup_table) / sizeof(*flag_lookup_table);
        flag_index++) {
        if (stricmp(value, flag_lookup_table[flag_index].text_name) == 0) {
            *result = flag_lookup_table[flag_index].flag;
            return true;
        }
    }

    return false;
}

/*
* Caskey, Damon V.
* 2026-08-08
*
* Combine every sound group argument into one independent
* mask. Return the first invalid token for diagnostics.
*/
static bool sound_group_get_flags_from_arglist_range(
    const ArgList* const arglist,
    const size_t argument_first,
    const size_t argument_end,
    sound_group_mask_t* const result,
    const char** const invalid_value
) {
    sound_group_mask_t flag;
    const char* value;
    size_t argument_index;

    if (!arglist || !result || argument_first >= argument_end
        || argument_end > arglist->count) {
        if (invalid_value) {
            *invalid_value = "";
        }
        return false;
    }

    *result = SOUND_GROUP_NONE;

    for (argument_index = argument_first;
        argument_index < argument_end;
        argument_index++) {
        value = GET_ARGP(argument_index);

        if (!sound_group_get_flag_from_string(value, &flag)) {
            if (invalid_value) {
                *invalid_value = value;
            }
            return false;
        }

        *result |= flag;
    }

    return true;
}

static bool sound_group_get_flags_from_arglist(
    const ArgList* const arglist,
    sound_group_mask_t* const result,
    const char** const invalid_value
) {
    return sound_group_get_flags_from_arglist_range(
        arglist,
        1,
        arglist ? arglist->count : 0,
        result,
        invalid_value
    );
}

/*
* Caskey, Damon V.
* 2026-08-07
*
* Allocate one frame sound instance with playback disabled.
*/
s_frame_sound* frame_sound_allocate(void) {
    s_frame_sound* result = malloc(sizeof(*result));

    if (!result) {
        borShutdown(1, E_OUT_OF_MEMORY);
    }

    memset(result, 0, sizeof(*result));
    result->channel = -1;
    result->sample = SAMPLE_ID_NONE;
    result->chance = SOUND_PLAY_CHANCE_MAX;
    result->group = SOUND_GROUP_DEFAULT;

    return result;
}

/*
* Caskey, Damon V.
* 2026-08-07
*
* Free one frame sound instance.
*/
void frame_sound_free(s_frame_sound* const sound) {
    if (!sound) {
        return;
    }

    free(sound->source);
    sound->source = NULL;
    free(sound);
}

/*
* Caskey, Damon V.
* 2026-08-08
*
* Replace the temporary source path for an indexed frame
* sound. Sample loading is deferred until the frame command
* so loading mode and source may be supplied in any order.
*/
static void frame_sound_set_source(s_frame_sound* const sound, const char* const source) {
    char* source_copy = NULL;

    if (!sound) {
        return;
    }

    if (source) {
        source_copy = strdup(source);
    }

    free(sound->source);
    sound->source = source_copy;
    sound->sample = SAMPLE_ID_NONE;
}

/*
* Caskey, Damon V.
* 2026-08-07
*
* Allocate an empty indexed frame sound collection.
*/
s_frame_sound_collection* frame_sound_collection_allocate(void) {
    s_frame_sound_collection* result = malloc(sizeof(*result));

    if (!result) {
        borShutdown(1, E_OUT_OF_MEMORY);
    }

    memset(result, 0, sizeof(*result));
    result->active_status = FRAME_SOUND_ACTIVE_NONE;

    return result;
}

/*
* Caskey, Damon V.
* 2026-08-07
*
* Free a frame sound collection and its instances.
*/
void frame_sound_collection_free(s_frame_sound_collection* const collection) {
    int sound_index;

    if (!collection) {
        return;
    }

    for (sound_index = 0; sound_index < MAX_FRAME_SOUNDS_PER_FRAME; sound_index++) {
        frame_sound_free(collection->slots[sound_index]);
        collection->slots[sound_index] = NULL;
    }

    collection->active_status = FRAME_SOUND_ACTIVE_NONE;
    collection->random_status = FRAME_SOUND_ACTIVE_NONE;
    free(collection->action);
    collection->action = NULL;
    collection->action_count = 0;
    collection->action_capacity = 0;
    free(collection);
}

/*
* Caskey, Damon V.
* 2026-08-08
*
* Append a frame-level sound operation. Actions retain
* declaration order and execute before new sounds are
* submitted for the frame.
*/
static s_frame_sound_action* frame_sound_action_append(
    s_frame_sound_collection** const collection,
    const e_frame_sound_action type
) {
    s_frame_sound_action* action;
    s_frame_sound_action* resized_actions;
    size_t new_capacity;

    if (!collection) {
        return NULL;
    }

    if (!*collection) {
        *collection = frame_sound_collection_allocate();
    }

    if ((*collection)->action_count
        == (*collection)->action_capacity) {
        new_capacity = (*collection)->action_capacity
            ? (*collection)->action_capacity * 2U
            : 4U;

        if (new_capacity < (*collection)->action_capacity
            || new_capacity > SIZE_MAX / sizeof(*resized_actions)) {
            borShutdown(1, E_OUT_OF_MEMORY);
        }

        resized_actions = realloc(
            (*collection)->action,
            new_capacity * sizeof(*resized_actions)
        );

        if (!resized_actions) {
            borShutdown(1, E_OUT_OF_MEMORY);
        }

        (*collection)->action = resized_actions;
        (*collection)->action_capacity = new_capacity;
    }

    action = &(*collection)->action[
        (*collection)->action_count++
    ];
    memset(action, 0, sizeof(*action));
    action->type = type;

    return action;
}

/*
* Caskey, Damon V.
* 2026-08-07
*
* Find an allocated sound instance by author-facing slot index.
* Allocation may precede the sound command so properties can
* be supplied in any order.
*/
s_frame_sound* frame_sound_find_slot_index(s_frame_sound_collection* const collection, const int sound_index) {
    if (!collection || !frame_sound_validate_slot_index(sound_index)) {
        return NULL;
    }

    return collection->slots[sound_index];
}

/*
* Caskey, Damon V.
* 2026-08-07
*
* Find or allocate an indexed sound instance. Collection
* allocation is deferred until the first sound property is
* supplied.
*/
s_frame_sound* frame_sound_upsert_index(s_frame_sound_collection** const collection, const int sound_index) {
    s_frame_sound* sound;

    if (!collection || !frame_sound_validate_slot_index(sound_index)) {
        return NULL;
    }

    if (!*collection) {
        *collection = frame_sound_collection_allocate();
    }

    sound = frame_sound_find_slot_index(*collection, sound_index);

    if (!sound) {
        frame_sound_free((*collection)->slots[sound_index]);
        sound = frame_sound_allocate();
        (*collection)->slots[sound_index] = sound;
    }

    return sound;
}

/*
* Caskey, Damon V.
* 2026-08-07
*
* Clone explicitly configured sound slots into a frame-owned
* collection. SAMPLE_ID_NONE remains active so it can serve
* as a deliberate blank in a random selection.
*/
s_frame_sound_collection* frame_sound_collection_clone(const s_frame_sound_collection* const source) {
    s_frame_sound_collection* result;
    s_frame_sound* sound_clone;
    const s_frame_sound* source_sound;
    uint64_t active_status;
    size_t action_memory_size;
    int sound_index;

    if (!source || (!source->active_status
        && !source->action_count)) {
        return NULL;
    }

    result = frame_sound_collection_allocate();
    result->random_status = source->random_status;

    if (source->action_count) {
        action_memory_size = source->action_count
            * sizeof(*result->action);
        result->action = malloc(action_memory_size);

        if (!result->action) {
            frame_sound_collection_free(result);
            borShutdown(1, E_OUT_OF_MEMORY);
        }

        memcpy(
            result->action,
            source->action,
            action_memory_size
        );
        result->action_count = source->action_count;
        result->action_capacity = source->action_count;
    }

    active_status = source->active_status;

    while (active_status) {
        sound_index = frame_sound_get_lowest_active_index(active_status);
        active_status &= active_status - 1;
        source_sound = source->slots[sound_index];

        if (!source_sound) {
            continue;
        }

        sound_clone = frame_sound_allocate();
        sound_clone->delay = source_sound->delay;
        sound_clone->loop_offset = source_sound->loop_offset;
        sound_clone->start_offset = source_sound->start_offset;
        sound_clone->group = source_sound->group;
        sound_clone->channel = source_sound->channel;
        sound_clone->sample = source_sound->sample;
        sound_clone->chance = source_sound->chance;
        sound_clone->priority = source_sound->priority;
        sound_clone->loop = source_sound->loop;
        sound_clone->start_offset_supplied =
            source_sound->start_offset_supplied;
        sound_clone->stream = source_sound->stream;
        result->slots[sound_index] = sound_clone;
        result->active_status |= frame_sound_get_slot_mask(sound_index);
    }

    return result;
}

/*
* Caskey, Damon V.
* 2026-08-08
*
* Resolve explicit frame sound sources after every indexed
* property has been parsed. Explicit silent entries retain
* SAMPLE_ID_NONE. Failed sources are removed from the frame.
*/
static void frame_sound_load_collection(
    s_frame_sound_collection* const collection,
    char* const packfilename
) {
    s_frame_sound* sound;
    uint64_t active_status;
    int sound_index;

    if (!collection || !collection->active_status) {
        return;
    }

    active_status = collection->active_status;

    while (active_status) {
        sound_index = frame_sound_get_lowest_active_index(active_status);
        active_status &= active_status - 1U;
        sound = collection->slots[sound_index];

        if (!sound) {
            collection->active_status &=
                ~frame_sound_get_slot_mask(sound_index);
            continue;
        }

        if (!sound->source) {
            sound->sample = SAMPLE_ID_NONE;
            continue;
        }

        sound->sample = sound_load_sample(
            sound->source,
            packfilename,
            true,
            sound->stream
        );

        if (sound->sample < 0) {
            collection->active_status &=
                ~frame_sound_get_slot_mask(sound_index);
        }
    }
}

/*
* Caskey, Damon V.
* 2026-08-07
*
* Clone parser scratch sounds into an animation frame.
*/
void frame_sound_initialize_frame_property(s_addframe_data* const data, const ptrdiff_t frame) {
    s_frame_sound_collection* sound_clone;
    size_t memory_size;

    sound_clone = frame_sound_collection_clone(data->sound);

    if (!sound_clone) {
        return;
    }

    if (!data->animation->sound) {
        memory_size = data->framecount * sizeof(*data->animation->sound);
        data->animation->sound = malloc(memory_size);

        if (!data->animation->sound) {
            frame_sound_collection_free(sound_clone);
            borShutdown(1, E_OUT_OF_MEMORY);
        }

        memset(data->animation->sound, 0, memory_size);
    }

    data->animation->sound[frame] = sound_clone;
}

/*
* Caskey, Damon V.
* 2026-08-07
*
* Load or unload every sample referenced by a frame.
*/
void frame_sound_cache_collection(const s_frame_sound_collection* const collection, const int load) {
    const s_frame_sound* sound;
    uint64_t active_status;
    int sound_index;

    if (!collection || !collection->active_status) {
        return;
    }

    active_status = collection->active_status;

    while (active_status) {
        sound_index = frame_sound_get_lowest_active_index(active_status);
        active_status &= active_status - 1;
        sound = collection->slots[sound_index];

        if (sound && sound->sample >= 0) {
            cachesound(sound->sample, load);
        }
    }
}

/*
* Caskey, Damon V.
* 2026-08-07
*
* Select one configured sound bit uniformly from a non-zero
* candidate mask. Rejection avoids modulo bias while keeping
* random selection on the entity update thread.
*/
static uint64_t frame_sound_select_random_status(uint64_t candidate_status) {
    uint64_t candidate_scan;
    uint64_t random_limit;
    uint64_t random_value;
    unsigned int candidate_count = 0;
    unsigned int candidate_offset;

    candidate_scan = candidate_status;
    while (candidate_scan) {
        candidate_scan &= candidate_scan - 1U;
        candidate_count++;
    }

    if (candidate_count <= 1U) {
        return candidate_status;
    }

    random_limit = (UINT64_C(1) << 32)
        - ((UINT64_C(1) << 32) % candidate_count);

    do {
        random_value = rand32();
    } while (random_value >= random_limit);

    candidate_offset = (unsigned int)(random_value % candidate_count);
    while (candidate_offset) {
        candidate_status &= candidate_status - 1U;
        candidate_offset--;
    }

    return candidate_status & (~candidate_status + 1U);
}

/*
* Caskey, Damon V.
* 2026-08-08
*
* Apply frame-level sound operations in declaration order.
* These run before the frame submits new sounds, allowing
* earlier playback to be manipulated before new submission.
*/
static void frame_sound_execute_actions(
    const s_frame_sound_collection* const collection,
    const uint64_t owner_id
) {
    const s_frame_sound_action* action;
    size_t action_index;

    if (!collection) {
        return;
    }

    for (action_index = 0;
        action_index < collection->action_count;
        action_index++) {
        action = &collection->action[action_index];

        switch (action->type) {
            case FRAME_SOUND_CHANNEL_ACTION_STOP:
                sound_stop_sample(action->channel);
                break;
            case FRAME_SOUND_CHANNEL_ACTION_PAUSE:
                sound_pause_single_sample(true, action->channel);
                break;
            case FRAME_SOUND_CHANNEL_ACTION_RESUME:
                sound_pause_single_sample(false, action->channel);
                break;
            case FRAME_SOUND_CHANNEL_ACTION_OFFSET:
                sound_set_channel_position(
                    action->channel,
                    action->offset
                );
                break;
            case FRAME_SOUND_GROUP_ACTION_STOP:
                sound_group_stop(action->group, owner_id);
                break;
            case FRAME_SOUND_GROUP_ACTION_PAUSE:
                sound_group_pause(true, action->group, owner_id);
                break;
            case FRAME_SOUND_GROUP_ACTION_RESUME:
                sound_group_pause(false, action->group, owner_id);
                break;
            case FRAME_SOUND_GROUP_ACTION_OFFSET:
                sound_group_set_position(
                    action->group,
                    owner_id,
                    action->offset
                );
                break;
            default:
                break;
        }
    }
}

/*
* Caskey, Damon V.
* 2026-08-07
*
* Submit active frame sounds in ascending slot order. When a
* random range is enabled, submit one configured entry from
* that range and every configured entry outside it. The
* acquired mixer channel owns delay, chance, looping, and
* offsets from this point forward.
*/
void frame_sound_execute_collection(
    const s_frame_sound_collection* const collection,
    const uint64_t owner_id
) {
    const s_frame_sound* sound;
    s_sound_play_options options;
    uint64_t active_status;
    int sound_index;

    if (!collection) {
        return;
    }

    frame_sound_execute_actions(collection, owner_id);

    if (!collection->active_status) {
        return;
    }

    active_status = collection->active_status;

    if (collection->random_status) {
        const uint64_t random_candidates =
            active_status & collection->random_status;

        active_status &= ~collection->random_status;
        active_status |= frame_sound_select_random_status(random_candidates);
    }

    while (active_status) {
        sound_index = frame_sound_get_lowest_active_index(active_status);
        active_status &= active_status - 1;
        sound = collection->slots[sound_index];

        if (!sound || sound->sample < 0) {
            continue;
        }

        memset(&options, 0, sizeof(options));
        options.delay = sound->delay;
        options.loop_offset = sound->loop_offset;
        options.owner_id = owner_id;
        options.start_offset = sound->start_offset;
        options.group = sound->group;
        options.channel = sound->channel >= 0
            ? (unsigned int)sound->channel
            : 0;
        options.delay_rate = global_config.game_speed > 0
            ? (unsigned int)global_config.game_speed
            : GAME_SPEED_DEFAULT;
        options.chance = sound->chance;
        options.channel_supplied = sound->channel >= 0;
        options.loop = sound->loop;
        options.start_offset_supplied =
            sound->start_offset_supplied;

        sound_play_sample_with_options(
            sound->sample,
            sound->priority,
            savedata.effectvol,
            savedata.effectvol,
            100,
            &options
        );
    }
}

/* **** Collision Support Functions */

/*
* Caskey, Damon V.
* 2026-06-27
*
* Validate that a collision index is within allowed
* range.
*/
static bool collision_validate_slot_index(const int index) {
    if (index < 0 || index >= MAX_COLLISION_BOXES_PER_FRAME) {
        return false;
    }
    return true;
}

/*
* Caskey, Damon V.
* 2026-06-27
*
* Get the bitmask for a collision slot index.
*
* Ex: Index 0 returns 0x0000000000000001
*     Index 1 returns 0x0000000000000002
*/
static uint64_t collision_get_slot_mask(const int index) {
    return ((uint64_t)1 << (uint64_t)index);
}

/*
* Caskey, Damon V.
* 2026-06-27
*
* Activate a collision slot bit in active masl
* by index.
*/
static void collision_activate_slot(uint64_t* const active_status, const int index) {
    *active_status |= collision_get_slot_mask(index);
}

/*
* Caskey, Damon V.
* 2026-06-27
*
* Deactivate a collision slot bit in active masl
* by index.
*
* 2026-07-03 - Not in use, left for future reference.
*/
//static void collision_deactivate_slot(uint64_t* const active_status, const int index) {
//    *active_status &= ~collision_get_slot_mask(index);
//}

/*
* Caskey, Damon V.
* 2026-06-27
*
* 
* Get the lowest active collision slot index from an
* active status mask. The caller must ensure active_status
* is not zero.
*/
static int collision_get_lowest_active_index(uint64_t active_status) {
    int collision_index = 0;

    while (!(active_status & 1)) {
        active_status >>= 1;
        collision_index++;
    }

    return collision_index;
}

/*
* Caskey, Damon V.
* 2026-06-27
*
* Check if a collision object has coordinates set.
* Returns TRUE if coordinates are set, FALSE otherwise.
*/
static int collision_check_has_coords(const s_hitbox* const coords) {
    if (!coords) {
        return FALSE;
    }

    return (coords->x || coords->y || coords->height || coords->width);
}

/*
* Caskey, Damon V.
* 2026-06-27
*
* Allocate the collision collection - container
* to house collision objects for a single frame. 
* Returns pointer to new collection.
*/
s_collision_collection* collision_collection_allocate(void) {
    s_collision_collection* result = malloc(sizeof(*result));

    if (!result) {
        borShutdown(1, E_OUT_OF_MEMORY);
    }

    memset(result, 0, sizeof(*result));
    result->active_status = COLLISION_ACTIVE_STATUS_NONE;

    return result;
}

/*
* Caskey, Damon V.
* 2026-06-27 (rework from 2021)
*
* Apply final frame coordinate adjustments 
* to a collision collection.
*/
static void collision_prepare_coordinates_for_frame(s_collision_collection* const collection, s_model* const model, const s_addframe_data* const add_frame_data, const bool apply_attack_z_default) {

    s_collision_instance* collision;
    s_hitbox* coords;
    uint64_t active_status;
    int collision_index;

    if (!collection || !collection->active_status) {
        return;
    }

    /*
    * Get the active status mask from the collection.
    */
    active_status = collection->active_status;

    while (active_status) {
    
        /*
        * Get the lowest active collision slot index from 
        * the active status mask.
        */
        collision_index = collision_get_lowest_active_index(active_status);

        /*
        * Clear the lowest active bit so the next loop
        * finds the next active collision slot.
        */
        active_status &= active_status - 1;

        collision = collection->slots[collision_index];

        if (!collision) {
            continue;
        }

        coords = &collision->coords;

        coords->x = coords->x - add_frame_data->offset->x;
        coords->y = coords->y - add_frame_data->offset->y;
        coords->width = coords->width + coords->x;
        coords->height = coords->height + coords->y;

        /*
        * Preserve legacy attack Z-depth fallback.
        */
        if (apply_attack_z_default && !coords->z_background && !coords->z_foreground) {
            coords->z_background = coords->z_foreground = (int)(model->grabdistance / 3 + 1);
        }
    }
}

/*
* Caskey, Damon V.
* 2026-06-27
*
* Allocate a collision instance - container
* to house collision data for a single instance.
* Returns pointer to new instance.
*
* The basic instance will need additional data 
* populated after allocation depending on use 
* (attack, body, space, etc.).
*/
s_collision_instance* collision_instance_allocate(const e_collision_config config) {
    s_collision_instance* result = malloc(sizeof(*result));

    if (!result) {
        borShutdown(1, E_OUT_OF_MEMORY);
    }

    memset(result, 0, sizeof(*result));

    result->config = config;
    result->coords = empty_collision_coords;

    return result;
}

/*
* Caskey, Damon V.
* 2026-06-30
*
* Allocate a new collision instance and copy values
* from an existing collision instance.
*
* Important:
* Do not memcpy the whole collision instance. The
* collision instance owns optional child objects
* like attack, body, space, and meta_data. A raw
* memcpy would copy those pointers directly and
* cause shared ownership, stale references, and
* double-free problems during cleanup.
*/
s_collision_instance* collision_instance_clone(const s_collision_instance* const source) {
    s_collision_instance* result = NULL;

    /*
    * No source means there is nothing to clone.
    */
    if (!source) {
        return NULL;
    }

    /*
    * Allocate the new collision instance using the
    * same config flags as the source. This also
    * initializes resident/default values.
    */
    result = collision_instance_allocate(source->config);

    /*
    * Copy resident value members.
    *
    * Coordinates are inlined in the collision instance,
    * so a direct structure copy is safe here.
    */
    result->coords = source->coords;
    result->meta_tag = source->meta_tag;

    /*
    * Clone owned optional property objects.
    *
    * These helpers are responsible for returning new
    * allocations with the same values as their source
    * object. Null source pointers stay null.
    */
    if (source->attack) {
        result->attack = attack_clone_object(source->attack);
    }

    if (source->body) {
        result->body = body_clone_object(source->body);
    }

    if (source->space) {
        result->space = space_clone_object(source->space);
    }

    /*
    * Meta data cloning is not implemented yet.
    *
    * Leave this null rather than shallow-copying the
    * source pointer. The collision instance free path
    * owns and frees meta_data, so a shallow copy would
    * create shared ownership and eventual double-free.
    */
    result->meta_data = NULL;

    return result;
}

/*
* Caskey, Damon V.
* 2026-06-28
*
* Free a collision collection.
*
* This frees every collision instance in 
* the collection, clears the active slot mask, 
* and frees the collection itself.
*/
void collision_collection_free(s_collision_collection* const collection) {
    int collision_index;

    if (!collection) {
        return;
    }

    /*
    * Free every collision instance in the collection.
    * 
    * Since this is a clean up operation and shouldn't
    * run in the hot path, we'll do a normal loop
    * instead of using the active status mask 
    * to find active slots. Just to ensure we 
    * get everythign cleaned up.
    */
    for (collision_index = 0; collision_index < MAX_COLLISION_BOXES_PER_FRAME; collision_index++) {
        collision_instance_free(collection->slots[collision_index]);
        collection->slots[collision_index] = NULL;
    }

    collection->active_status = COLLISION_ACTIVE_STATUS_NONE;

    free(collection);
}

/*
* Caskey, Damon V.
* 2026-06-30
*
* Allocate a new collision collection and clone each
* active collision instance from the source collection.
*
* The result preserves source slot indexes. This matters
* because each collision index is author-facing data, not
* merely an internal packed array position.
*
* If check_coords is enabled, source instances with empty
* coordinates are skipped. This preserves legacy behavior
* where all-zero coordinates mean "no collision here" and
* should not become an active frame collision.
*/
s_collision_collection* collision_collection_clone(const s_collision_collection* const source, const int check_coords) {

    s_collision_collection* result = NULL;
    s_collision_instance* collision_clone = NULL;
    const s_collision_instance* source_collision = NULL;
    uint64_t active_status;
    int collision_index;

    /*
    * No source collection, or no active source slots,
    * means there is nothing to clone.
    */
    if (!source || !source->active_status) {
        return NULL;
    }

    /*
    * Work from a local copy of the active mask.
    *
    * The source collection must not be modified by clone.
    * Each loop consumes one active bit from this local copy.
    */
    active_status = source->active_status;

    while (active_status) {

        /*
        * Get the lowest active slot index from the local
        * active mask copy.
        */
        collision_index = collision_get_lowest_active_index(active_status);

        /*
        * Clear the lowest active bit so the next loop
        * moves to the next active source slot.
        */
        active_status &= active_status - 1;

        /*
        * Active mask and pointer slots should agree, but
        * guard against a damaged or partially populated
        * source collection. A missing instance is simply
        * not cloned into the result.
        */
        source_collision = source->slots[collision_index];

        if (!source_collision) {
            continue;
        }

        /*
        * Optional coordinate filter.
        *
        * This is used when committing parser/carry-forward
        * data to an animation frame. Empty coordinates mean
        * the collision slot is inactive for the finalized
        * frame, so do not allocate a downstream clear.
        */
        if (check_coords && !collision_check_has_coords(&source_collision->coords)) {
            continue;
        }

        /*
        * Lazily allocate the destination collection only
        * after we know at least one source instance should
        * actually survive cloning.
        */
        if (!result) {
            result = collision_collection_allocate();
        }

        /*
        * Clone the collision instance. The instance clone
        * handles its owned child objects, so the collection
        * only needs to install the returned pointer.
        */
        collision_clone = collision_instance_clone(source_collision);

        /*
        * This should not normally fail because a valid source
        * instance was provided and allocation failure should
        * shut down through the allocator. Still, keep this
        * guard so clone failure cannot leave a partial result
        * in circulation.
        */
        if (!collision_clone) {
            collision_collection_free(result);
            return NULL;
        }

        /*
        * Preserve the original slot index, then mark that
        * same slot active in the destination mask.
        */
        result->slots[collision_index] = collision_clone;
        collision_activate_slot(&result->active_status, collision_index);
    }

    /*
    * If every active source slot was filtered out, result
    * remains NULL. That is intentional and avoids allocating
    * collections that only represent "nothing here".
    */
    return result;
}

/*
* Caskey, Damon V.
* 2026-07-01
*
* Find an active collision instance by slot index.
*
* This is a read-style lookup. It does not allocate,
* clone, activate, or deactivate anything. It only
* returns an existing collision instance when the
* collection exists, the requested index is valid,
* and the requested slot is marked active.
*/
s_collision_instance* collision_find_slot_index(s_collision_collection* const collection, const int collision_index) {
    uint64_t active_bit;

    /*
    * No collection means there is nowhere to search.
    */
    if (!collection) {
        return NULL;
    }

    /*
    * Guard against invalid author-facing collision indexes.
    *
    * This also protects collision_get_slot_mask() from
    * shifting outside the 64-bit slot range.
    */
    if (!collision_validate_slot_index(collision_index)) {
        return NULL;
    }

    /*
    * No active slots means the requested slot cannot
    * currently contain a usable collision instance.
    */
    if (!collection->active_status) {
        return NULL;
    }

    /*
    * Convert the requested collision index into the
    * matching active-status bit.
    */
    active_bit = collision_get_slot_mask(collision_index);

    /*
    * Active status is the source of truth for whether
    * a slot participates in collision processing.
    *
    * A non-null pointer in an inactive slot should not
    * be returned here.
    */
    if (!(collection->active_status & active_bit)) {
        return NULL;
    }

    /*
    * The slot is marked active. Return the stored
    * collision instance pointer. In a healthy collection,
    * this should be non-null.
    */
    return collection->slots[collision_index];
}

/*
* Caskey, Damon V.
* 2026-07-01
*
* Find or create a collision instance at a specific
* collection slot index.
*
* This is the generic upsert used by parser-facing
* helpers for attack, body, and space boxes.
*
* Behavior:
* - Allocates the collection if it does not exist.
* - Reuses an active collision instance if present.
* - Repairs an active slot with a missing instance.
* - Marks the slot active.
* - Applies the requested config flag to the instance.
*
* The function does not allocate attack/body/space
* property objects directly. That work belongs to
* the parser-facing property helpers.
*/
s_collision_instance* collision_upsert_index(s_collision_collection** const collection, const int collision_index, const e_collision_config config) {
    s_collision_instance* collision = NULL;

    /*
    * The caller must provide the address of a collection
    * pointer so this function can lazily allocate it.
    */
    if (!collection) {
        return NULL;
    }

    /*
    * Guard against invalid author-facing collision indexes.
    *
    * This also protects the active mask helpers from
    * shifting outside the 64-bit slot range.
    */
    if (!collision_validate_slot_index(collision_index)) {
        return NULL;
    }

    /*
    * A config-less collision instance is not useful.
    * Refuse to create one here.
    */
    if (config == COLLISION_CONFIG_NONE) {
        return NULL;
    }

    /*
    * Allocate the collection on demand. This avoids
    * burning memory for frames or parser scratch data
    * that never actually receive collision boxes.
    */
    if (!*collection) {
        *collection = collision_collection_allocate();
    }

    /*
    * Try to find an already-active collision instance
    * at the requested slot.
    */
    collision = collision_find_slot_index(*collection, collision_index);

    /*
    * If the slot was not active, or if the active bit
    * somehow pointed to a missing instance, allocate
    * a new collision instance for this slot.
    */
    if (!collision) {

        /*
        * If a stale inactive pointer exists here, clear it
        * before replacing the slot. Inactive slots should
        * not own live collision data.
        */
        if ((*collection)->slots[collision_index]) {
            collision_instance_free((*collection)->slots[collision_index]);
            (*collection)->slots[collision_index] = NULL;
        }

        collision = collision_instance_allocate(config);
        (*collection)->slots[collision_index] = collision;
    }
    else {
        /*
        * Existing collision instances may accumulate config
        * flags when later parser commands add another property
        * family to the same slot.
        */
        collision->config |= config;
    }

    /*
    * Mark the slot active. The active mask is the source
    * of truth for iteration and hot-path collision scans.
    */
    collision_activate_slot(&(*collection)->active_status, collision_index);

    return collision;
}

/*
* Caskey, Damon V.
* 2026-07-01
*
* Find or create a collision instance at a specific
* collection slot index, then return its coordinate
* property.
*
* Coordinates are embedded directly in the collision
* instance. There is no separate coordinate allocation
* to perform here.
*
* This function intentionally does not clear or overwrite
* existing coordinate values. Parser-facing code may be
* updating an existing carried-forward collision slot, so
* the caller is responsible for assigning whichever fields
* the parsed command intends to change.
*/
s_hitbox* collision_upsert_coordinates_property(s_collision_collection** const collection, const int collision_index, const e_collision_config config) {
    s_collision_instance* collision = NULL;

    /*
    * Upsert the parent collision instance first.
    *
    * This handles:
    * - Collection allocation.
    * - Slot validation.
    * - Collision instance allocation.
    * - Active mask update.
    * - Config flag accumulation.
    */
    collision = collision_upsert_index(collection, collision_index, config);

    /*
    * If the collision instance could not be created or
    * found, there is no coordinate property to return.
    */
    if (!collision) {
        return NULL;
    }

    /*
    * Coordinates live inline on the collision instance.
    * Return their address so parser-facing code can write
    * directly into the active slot.
    */
    return &collision->coords;
}

/*
* Caskey, Damon V.
* 2026-07-02
*
* Find or create an attack property at a specific
* collision collection slot.
*/
s_attack* collision_attack_upsert_property(s_collision_collection** const collection, const int collision_index) {
    s_collision_instance* collision = NULL;

    collision = collision_upsert_index(collection, collision_index, COLLISION_CONFIG_ATTACK);

    if (!collision) {
        return NULL;
    }

    if (!collision->attack) {
        collision->attack = attack_allocate_object();
    }

    return collision->attack;
}

/*
* Caskey, Damon V.
* 2026-07-02
*
* Find or create a recursive attack property at a
* specific collision collection slot.
*/
s_recursive_effect* collision_attack_upsert_recursive_property(s_collision_collection** const collection, const int collision_index) {
    s_attack* attack = NULL;

    attack = collision_attack_upsert_property(collection, collision_index);

    if (!attack) {
        return NULL;
    }

    if (!attack->recursive) {
        attack->recursive = recursive_effect_allocate_object();
    }

    return attack->recursive;
}

/*
* Caskey, Damon V.
* 2026-07-02
*
* Find or create attack coordinates at a specific
* collision collection slot.
*/
s_hitbox* collision_attack_upsert_coordinates_property(s_collision_collection** const collection, const int collision_index) {
    return collision_upsert_coordinates_property(collection, collision_index, COLLISION_CONFIG_ATTACK);
}

/*
* Caskey, Damon V.
* 2026-07-02
*
* Find or create a body property at a specific
* collision collection slot.
*/
s_body* collision_body_upsert_property(s_collision_collection** const collection, const int collision_index) {
    s_collision_instance* collision = NULL;

    collision = collision_upsert_index(collection, collision_index, COLLISION_CONFIG_BODY);

    if (!collision) {
        return NULL;
    }

    if (!collision->body) {
        collision->body = body_allocate_object();
    }

    return collision->body;
}

/*
* Caskey, Damon V.
* 2026-07-02
*
* Find or create body coordinates at a specific
* collision collection slot.
*/
s_hitbox* collision_body_upsert_coordinates_property(s_collision_collection** const collection, const int collision_index) {
    return collision_upsert_coordinates_property(collection, collision_index, COLLISION_CONFIG_BODY);
}

/*
* Caskey, Damon V.
* 2026-07-02
*
* Find or create a space property at a specific
* collision collection slot.
*/
s_space* collision_space_upsert_property(s_collision_collection** const collection, const int collision_index) {
    s_collision_instance* collision = NULL;

    collision = collision_upsert_index(collection, collision_index, COLLISION_CONFIG_SPACE);

    if (!collision) {
        return NULL;
    }

    if (!collision->space) {
        collision->space = space_allocate_object();
    }

    return collision->space;
}

/*
* Caskey, Damon V.
* 2026-07-02
*
* Find or create space coordinates at a specific
* collision collection slot.
*/
s_hitbox* collision_space_upsert_coordinates_property(s_collision_collection** const collection, const int collision_index) {
    return collision_upsert_coordinates_property(collection, collision_index, COLLISION_CONFIG_SPACE);
}


/*
* Free a collision instance and any optional property
* objects owned by it.
*/
void collision_instance_free(s_collision_instance* const collision) {
    
    if (!collision) {
        return;
    }

    if (collision->attack) {
        attack_free_object(collision->attack);
        collision->attack = NULL;
    }

    if (collision->body) {
        body_free_object(collision->body);
        collision->body = NULL;
    }

    if (collision->space) {
        space_free_object(collision->space);
        collision->space = NULL;
    }

    if (collision->meta_data) {
        meta_data_free_list(collision->meta_data);
        collision->meta_data = NULL;
    }

    free(collision);
}

/*
* Caskey, Damon V.
* 2020-03-07
*
* Allocate and apply collision settings to target frame.
*/
void collision_attack_initialize_frame_property(s_addframe_data* data, ptrdiff_t frame) {
    
    s_collision_collection* collision_clone = NULL;
    size_t memory_size;

    if (!data->collision_attack) {
        return;
    }

    /*
    * If the animation does not have any collision
    * attacks allocated yet, we need to allocate the
    * frame pointer table for the animation. This
    * will prepare us an array of empty pointers to
    * s_collision_collection, one for each animation
    * frame.  
    */
    if (!data->animation->collision_attack) {
        memory_size = data->framecount * sizeof(*data->animation->collision_attack);

        data->animation->collision_attack = malloc(memory_size);

        if (!data->animation->collision_attack) {
            borShutdown(1, E_OUT_OF_MEMORY);
        }

        memset(data->animation->collision_attack, 0, memory_size);
    }

    /*
    * Allocates a copy of the temporary source collision
    * list - this clone is the "real" collision list. The
    * temp is discarded by parent function after the frame 
    * is added to the animation.
    */
    collision_clone = collision_collection_clone(data->collision_attack, TRUE);
    
    /* Finalize collision coordinates for the clone. */
    collision_prepare_coordinates_for_frame(collision_clone, data->model, data, TRUE);

    /* Populate the animation frame with the pointer to the clone. */
    data->animation->collision_attack[frame] = collision_clone;
}

/*
* Caskey, Damon V.
* 2026-06-27 (rework from 2021)
*
* Accept animation, frame, and a block value.
* Find the first active attack collision instance
* whose no_block value is less than or equal to
* the block argument.
*/
s_collision_instance* collision_attack_find_no_block_on_frame(s_anim* animation, const int frame, const int block) {
    s_collision_collection* collection = NULL;
    s_collision_instance* collision = NULL;
    uint64_t active_status;
    int collision_index;

    if (!animation || !animation->collision_attack) {
        return NULL;
    }

    /*
    * Protect the frame table lookup.
    */
    if (frame < 0 || frame >= animation->numframes) {
        return NULL;
    }

    collection = animation->collision_attack[frame];

    if (!collection || !collection->active_status) {
        return NULL;
    }

    active_status = collection->active_status;

    while (active_status) {
        collision_index = collision_get_lowest_active_index(active_status);
        active_status &= active_status - 1;

        collision = collection->slots[collision_index];

        if (collision && collision->attack && collision->attack->no_block <= block) {
            return collision;
        }
    }

    return NULL;
}

/*
* Caskey, Damon V.
* 2026-07-03
*
* Send collision collection data to log for debugging.
*/
void collision_collection_dump(const s_collision_collection* const collection) {
    const s_collision_instance* collision = NULL;
    uint64_t active_status;
    int collision_index;

    printf("\n\n -- Collision collection (%p) dump --", collection);

    if (!collection) {
        printf("\n\n -- Collision collection dump complete... -- \n");
        return;
    }

    printf("\n\t ->active_status: %" PRIu64, collection->active_status);

    /*
    * Only dump active slots. This mirrors the runtime
    * active-mask scan pattern and avoids noise from
    * unused slot storage.
    */
    active_status = collection->active_status;

    while (active_status) {
        collision_index = collision_get_lowest_active_index(active_status);
        active_status &= active_status - 1;

        collision = collection->slots[collision_index];

        printf("\n\t ->slot[%d]: %p", collision_index, collision);

        if (!collision) {
            continue;
        }

        printf("\n\t\t ->config: %d", collision->config);
        printf("\n\t\t ->coords.x: %d", collision->coords.x);
        printf("\n\t\t ->coords.y: %d", collision->coords.y);
        printf("\n\t\t ->coords.width: %d", collision->coords.width);
        printf("\n\t\t ->coords.height: %d", collision->coords.height);
        printf("\n\t\t ->coords.z_background: %d", collision->coords.z_background);
        printf("\n\t\t ->coords.z_foreground: %d", collision->coords.z_foreground);
        printf("\n\t\t ->attack: %p", collision->attack);
        printf("\n\t\t ->body: %p", collision->body);
        printf("\n\t\t ->space: %p", collision->space);
        printf("\n\t\t ->meta_data: %p", collision->meta_data);
        printf("\n\t\t ->meta_tag: %" PRId64, collision->meta_tag);
    }

    printf("\n\n -- Collision collection (%p) dump complete... -- \n", collection);
}

/* 
* Caskey, Damon V.
* 2020-02-11
* 
* Allocate an attack property structure and return pointer.
*/
s_attack* attack_allocate_object(void) {
    s_attack* result = NULL;

    result = malloc(sizeof(*result));

    if (!result) {
        borShutdown(1, E_OUT_OF_MEMORY);
    }

    memcpy(result, &emptyattack, sizeof(*result));

    result->hitsound = global_sample_list.beat;

    result->dropv.x = default_model_dropv.x;
    result->dropv.y = default_model_dropv.y;
    result->dropv.z = default_model_dropv.z;

    return result;
}

/* 
* Caskey, Damon V.
* 2020-03-09
*
* Allocate new attack object with same values (but not same 
* pointers) as received attack object. Returns pointer to
* new object.
*/
s_attack* attack_clone_object(s_attack* source) {
    s_attack* result = NULL;

    if (!source)
    {
        return result;
    }

    result = attack_allocate_object();

    /* Make a local copy of sub object pointers. */
    s_recursive_effect *source_recursive = source->recursive;

    /* 
    * Attack has a ton of members. Rather than do everything 
    * piecemeal, we'll memcopy to get all the basic values, 
    * and then overwrite members individually as needed.
    */

    memcpy(result, source, sizeof(*result));

    /* 
    * Clone sub objects. Same principal as parent. We want 
    * new pointers allocated with the same data as the source 
    * pointers.
    */

    /* -- Clone recursive effect. */
    result->recursive = NULL;

    if (source_recursive) {
        result->recursive = recursive_effect_allocate_object();
        memcpy(result->recursive, source_recursive, sizeof(*result->recursive));
    }

    return result;
}

/*
* Caskey, Damon V
* 2020-03-12
*
* Send all attack data to log for debugging.
*/
void attack_dump_object(s_attack* attack)
{
    printf("\n\n -- Attack (%p) dump --", attack);

    if (attack) {
        printf("\n\t ->attack_drop: %d", attack->attack_drop);
        printf("\n\t ->attack_force: %d", attack->attack_force);
        printf("\n\t ->attack_type: %d", attack->attack_type);
        printf("\n\t ->blocksound: %d", attack->blocksound);
        printf("\n\t ->counterattack: %d", attack->counterattack);
        printf("\n\t ->damage_on_landing.attack_force: %d", attack->damage_on_landing.attack_force);
        printf("\n\t ->damage_on_landing.attack_type: %d", attack->damage_on_landing.attack_type);
        printf("\n\t ->dropv.x: %f", attack->dropv.x);
        printf("\n\t ->dropv.y: %f", attack->dropv.y);
        printf("\n\t ->dropv.z: %f", attack->dropv.z);
        printf("\n\t ->flash.layer_adjust: %d", attack->flash.layer_adjust);
        printf("\n\t ->flash.layer_source: %d", attack->flash.layer_source);
        printf("\n\t ->flash.model_block: %d", attack->flash.model_block);
        printf("\n\t ->flash.model_hit: %d", attack->flash.model_hit);
        printf("\n\t ->flash.z_source: %d", attack->flash.z_source);
        printf("\n\t ->forcemap: %d", attack->forcemap);
        printf("\n\t ->force_direction: %d", attack->force_direction);
        printf("\n\t ->freeze: %d", attack->freeze);
        printf("\n\t ->freezetime: %d", attack->freezetime);
        printf("\n\t ->grab: %d", attack->grab);
        printf("\n\t ->grab_distance: %d", attack->grab_distance);
        printf("\n\t ->guardcost: %d", attack->guardcost);
        printf("\n\t ->hitsound: %d", attack->hitsound);
        printf("\n\t ->jugglecost: %d", attack->jugglecost);
        printf("\n\t ->maptime: %d", attack->maptime);
        printf("\n\t ->next_hit_time: %d", attack->next_hit_time);
        printf("\n\t ->no_block: %d", attack->no_block);
        printf("\n\t ->otg: %d", attack->otg);
        printf("\n\t ->pause_add: %d", attack->pause_add);
        printf("\n\t ->recursive: %p", attack->recursive);

        if (attack->recursive) {
            recursive_effect_dump_object(attack->recursive);
        }

        printf("\n\t ->seal: %d", attack->seal);
        printf("\n\t ->sealtime: %d", attack->sealtime);
        printf("\n\t ->staydown.rise: %d", attack->staydown.rise);
        printf("\n\t ->staydown.riseattack: %d", attack->staydown.riseattack);
        printf("\n\t ->staydown.riseattack_stall: %d", attack->staydown.riseattack_stall);
    }

    printf("\n\n -- Attack (%p) dump complete... -- \n", attack);
}

/*
* Caskey, Damon V.
* 2020-03-10
* 
* Free attack properties from memory.
*/
void attack_free_object(s_attack* target) {
    if (!target) {
        return;
    }

    if (target->recursive) {
        free(target->recursive);
        target->recursive = NULL;
    }

    free(target);
}

/*
* Caskey, Damon V.
* 2021-08-08
*
* Allocate a body property structure and return pointer.
*/
s_body* body_allocate_object(void) {
    s_body* result = NULL;

    result = malloc(sizeof(*result));

    if (!result) {
        borShutdown(1, E_OUT_OF_MEMORY);
    }

    memcpy(result, &empty_body, sizeof(*result));

    return result;
}

/*
* Caskey, Damon V.
* 2021-08-08
*
* Allocate new body object with same values, but
* without sharing owned nested pointers.
*/
s_body* body_clone_object(s_body* source) {
    s_body* result = NULL;

    if (!source) {
        return NULL;
    }

    result = body_allocate_object();

    /*
    * Copy resident values first.
    */
    memcpy(result, source, sizeof(*result));

    /*
    * Do not share the source defense pointer.
    *
    * Body free owns and releases body->defense, so
    * a shallow copy would create shared ownership and
    * eventual use-after-free or double-free trouble.
    */
    result->defense = NULL;

    if (source->defense) {
        result->defense = defense_allocate_object();

        memcpy(
            result->defense,
            source->defense,
            sizeof(*result->defense) * (max_attack_types + 1));
    }

    return result;
}

/*
* Caskey, Damon V
* 2020-03-12
*
* Send all body data to log for debugging.
*/
void body_dump_object(s_body* body) {
    printf("\n\n -- Body (%p) dump --", body);

    if (body) {        
        printf("\n\t ->body_defense: %p", body->defense);
        printf("\n\t ->flash.layer_adjust: %d", body->flash.layer_adjust);
        printf("\n\t ->flash.layer_source: %d", body->flash.layer_source);
        printf("\n\t ->flash.z_source: %d", body->flash.z_source);
    }

    printf("\n\n -- Body (%p) dump complete... -- \n", body);
}

/*
* Caskey, Damon V.
* 2021-08-21
*
* Free body properties from memory.
*/
void body_free_object(s_body* target) {
    if (!target) {
        return;
    }

    if (target->defense) {
        defense_free_object(target->defense);
        target->defense = NULL;
    }

    free(target);
}

/*
* Caskey, Damon V.
* 2026-07-02
*
* Allocate a space property object and apply
* default values.
*/
s_space* space_allocate_object(void) {
    s_space* result = NULL;

    result = malloc(sizeof(*result));

    if (!result) {
        borShutdown(1, E_OUT_OF_MEMORY);
    }

    memcpy(result, &empty_space, sizeof(*result));

    return result;
}

/*
* Caskey, Damon V.
* 2026-07-02
*
* Allocate a new space property object with the
* same values as an existing space object.
*/
s_space* space_clone_object(s_space* source) {
    s_space* result = NULL;

    if (!source) {
        return NULL;
    }

    result = space_allocate_object();

    /*
    * Space currently owns no nested allocations,
    * so a structure copy is safe.
    */
    memcpy(result, source, sizeof(*result));

    return result;
}

/*
* Caskey, Damon V.
* 2026-07-02
*
* Send space property values to log for debugging.
*/
void space_dump_object(s_space* space) {
    printf("\n\n -- Space (%p) dump --", space);

    if (space) {
        printf("\n\t ->push.x: %f", space->push.x);
        printf("\n\t ->push.y: %f", space->push.y);
        printf("\n\t ->push.z: %f", space->push.z);
    }

    printf("\n\n -- Space (%p) dump complete... -- \n", space);
}

/*
* Caskey, Damon V.
* 2026-07-02
*
* Free a space property object.
*/
void space_free_object(s_space* target) {
    if (!target) {
        return;
    }

    free(target);
}

/*
* Caskey, Damon V.
* 2020-03-07
*
* Allocate and apply collision settings to target frame.
*/
void collision_body_initialize_frame_property(s_addframe_data* data, ptrdiff_t frame) {
    
    s_collision_collection* collision_clone = NULL;
    size_t memory_size;

    if (!data->collision_body) {
        return;
    }

    /*
    * If the animation does not have any collision
    * body allocated yet, we need to allocate the
    * frame pointer table for the animation. This
    * will prepare us an array of empty pointers to
    * s_collision_collection, one for each animation
    * frame.  
    */
    if (!data->animation->collision_body) {
        memory_size = data->framecount * sizeof(*data->animation->collision_body);

        data->animation->collision_body = malloc(memory_size);

        if (!data->animation->collision_body) {
            borShutdown(1, E_OUT_OF_MEMORY);
        }

        memset(data->animation->collision_body, 0, memory_size);
    }

    /*
    * Allocates a copy of the temporary source collision
    * list - this clone is the "real" collision list. The
    * temp is discarded by parent function after the frame 
    * is added to the animation.
    */
    collision_clone = collision_collection_clone(data->collision_body, TRUE);
    
    /* Finalize collision coordinates for the clone. */
    collision_prepare_coordinates_for_frame(collision_clone, data->model, data, FALSE);

    /* Populate the animation frame with the pointer to the clone. */
    data->animation->collision_body[frame] = collision_clone;

    /*
    * Preserve legacy vulnerability behavior. A frame is vulnerable
    * when it contains at least one active body collision box.
    */
    data->animation->vulnerable[frame] =
        collision_clone
        && collision_clone->active_status != COLLISION_ACTIVE_STATUS_NONE;
}

/*
* Caskey, Damon V.
* 2020-03-07
*
* Allocate and apply collision settings to target frame.
*/
void collision_space_initialize_frame_property(s_addframe_data* data, ptrdiff_t frame) {
    
    s_collision_collection* collision_clone = NULL;
    size_t memory_size;

    if (!data->collision_space) {
        return;
    }

    /*
    * If the animation does not have any collision
    * space allocated yet, we need to allocate the
    * frame pointer table for the animation. This
    * will prepare us an array of empty pointers to
    * s_collision_collection, one for each animation
    * frame.  
    */
    if (!data->animation->collision_space) {
        memory_size = data->framecount * sizeof(*data->animation->collision_space);

        data->animation->collision_space = malloc(memory_size);

        if (!data->animation->collision_space) {
            borShutdown(1, E_OUT_OF_MEMORY);
        }

        memset(data->animation->collision_space, 0, memory_size);
    }

    /*
    * Allocates a copy of the temporary source collision
    * list - this clone is the "real" collision list. The
    * temp is discarded by parent function after the frame 
    * is added to the animation.
    */
    collision_clone = collision_collection_clone(data->collision_space, TRUE);
    
    /* Finalize collision coordinates for the clone. */
    collision_prepare_coordinates_for_frame(collision_clone, data->model, data, FALSE);

    /* Populate the animation frame with the pointer to the clone. */
    data->animation->collision_space[frame] = collision_clone;
}

/*
* 2020-03-10
* Caskey, Damon V
*
* allocate a recursive effect object and return
* its pointer.
*/
s_recursive_effect* recursive_effect_allocate_object(void) {
    s_recursive_effect* result;
    size_t memory_size;

    memory_size = sizeof(*result);
    result = malloc(memory_size);

    if (!result) {
        borShutdown(1, (char *)E_OUT_OF_MEMORY);
        return NULL;
    }

    /*
    * 0 The property values, then set
    * specific default values.
    */
    memset(result, 0, memory_size);

    result->type = ATK_NONE;

    return result;
}

/*
* Caskey, Damon V.
* 2026-07-10
*
* Allocate the recursive effect collection used by
* an entity. The collection remains allocated until
* the entity dies.
*/
static s_recursive_effect* recursive_effect_allocate_collection(void) {
    s_recursive_effect* result;
    uint64_t recursive_index;

    result = calloc(MAX_RECURSIVE_EFFECTS, sizeof(*result));

    if (!result) {
        borShutdown(1, (char*)E_OUT_OF_MEMORY);
        return NULL;
    }

    /*
    * calloc initializes type to 0, but ATK_NONE is -1.
    * Initialize each inactive slot to the proper default.
    */
    for (recursive_index = 0;
        recursive_index < MAX_RECURSIVE_EFFECTS;
        recursive_index++) {

        result[recursive_index].type = ATK_NONE;
    }

    return result;
}

/*
* Caskey, Damon V.
* 2026-07-10
*
* Release all recursive effect state owned by
* an entity.
*/
static void recursive_effect_free_collection(entity* target) {
    if (!target) {
        return;
    }

    free(target->recursive_effect_collection);

    target->recursive_effect_collection = NULL;
    target->recursive_effect_active = 0;
}

/*
* Caskey, Damon V.
* 2019-01-15
*
* If attack has any recursive effects, apply
* them to entity accordingly.
*/
void recursive_effect_check_apply(entity* ent, entity* other, s_attack* attack) {
    s_recursive_effect* recursive_effect;
    uint64_t active_flag;
    uint64_t index;

    /*
    * No target, attack, or recursive effect means
    * there is nothing to apply.
    */
    if (!ent || !attack || !attack->recursive) {
        return;
    }

    index = attack->recursive->index;

    /*
    * Protect the collection lookup and bit shift.
    */
    if (index >= MAX_RECURSIVE_EFFECTS) {
        return;
    }

    active_flag = bitmask64_from_index(index);

    if (!active_flag) {
        return;
    }

    /*
    * Allocate the entity collection only when the
    * first recursive effect is applied.
    */
    if (!ent->recursive_effect_collection) {
        ent->recursive_effect_collection = recursive_effect_allocate_collection();

        if (!ent->recursive_effect_collection) {
            return;
        }
    }

    /*
    * Get and reset the indexed resident slot.
    */
    recursive_effect = &ent->recursive_effect_collection[index];

    memset(recursive_effect, 0, sizeof(*recursive_effect));
    recursive_effect->type = ATK_NONE;

    /*
    * Populate the resident effect.
    */
    const uint64_t time_multiplier = 1; // global_config.game_speed / 100;

    recursive_effect->meta_tag = attack->recursive->meta_tag;
    recursive_effect->mode = attack->recursive->mode;
    recursive_effect->index = index;
    recursive_effect->rate = attack->recursive->rate;
    recursive_effect->force = attack->recursive->force;
    recursive_effect->owner = other;
    recursive_effect->time =
        _time + (attack->recursive->time * time_multiplier);
    recursive_effect->tick =
        _time + (recursive_effect->rate * time_multiplier);

    /*
    * ATK_NONE means inherit the original attack type.
    */
    if (attack->recursive->type == ATK_NONE) {
        recursive_effect->type = attack->attack_type;
    } else {
        recursive_effect->type = attack->recursive->type;
    }

    /*
    * Mark the slot active only after its contents
    * have been fully initialized.
    */
    ent->recursive_effect_active |= active_flag;
}

/*
* Caskey, Damon V
* 2020-03-12
*
* Send all recursive effect data to log for debugging.
*/
void recursive_effect_dump_object(s_recursive_effect* recursive) {
    printf("\n\n -- Recursive (%p) dump --", recursive);

    if (recursive) {
        printf("\n\t ->force: %d", recursive->force);
        printf("\n\t ->index: %u", recursive->index);
        //printf("\n\t ->meta_data: %p", recursive->meta_data);
        printf("\n\t ->meta_tag: %" PRId64, (int64_t)recursive->meta_tag);
        printf("\n\t ->mode: %d", recursive->mode);
        printf("\n\t ->owner: %p", recursive->owner);
        printf("\n\t ->rate: %" PRIu32, (uint32_t)recursive->rate);
        printf("\n\t ->tick: %" PRIu32, (uint32_t)recursive->tick);
        printf("\n\t ->time: %" PRIu32, (uint32_t)recursive->time);
        printf("\n\t ->type: %d", recursive->type);
    }

    printf("\n\n -- Recursive (%p) dump complete. -- \n", recursive);
}

/*
* Caskey, Damon V.
* 2020-03-13
*
* Wrapper for deleting a recursive object's data.
*/
void recursive_effect_free_object(s_recursive_effect* target) {
    free(target);
}

/*
* Caskey, Damon V.
* 2026-06-01
*
* Get recursive effect mode flag from text argument.
*/
static inline e_damage_recursive_logic recursive_effect_get_mode_flag_from_argument(const char* value) {

    static const struct {
        const char* name;
        e_damage_recursive_logic flag;
    } modes[] = {
        { "none",       DAMAGE_RECURSIVE_MODE_NONE },
        { "hp",         DAMAGE_RECURSIVE_MODE_HP },
        { "mp",         DAMAGE_RECURSIVE_MODE_MP },
        { "non-lethal", DAMAGE_RECURSIVE_MODE_NON_LETHAL }
    };

    const size_t num_modes = sizeof(modes) / sizeof(modes[0]);
    size_t i;

    /*
    * Safety check. If no value provided, just return 0 (no flags).
    */
    if (!value) {
        return DAMAGE_RECURSIVE_MODE_NONE;
    }

    /*
    * Iterate through all possible modes and compare with input value.
    * If a match is found, return the corresponding flag. O(n), but
    * we're only doing this on load.
    */
    for (i = 0; i < num_modes; i++) {
        if (stricmp(value, modes[i].name) == 0) {
            return modes[i].flag;
        }
    }

    /*
    * If no match is found, return 0 (no flags).
    */
    return DAMAGE_RECURSIVE_MODE_NONE;
}

/*
- Caskey, Damon V.
- 2026-08-11
-
- Read recursive damage mode arguments directly from the
  source line and combine their corresponding behavior flags.
*/
e_damage_recursive_logic recursive_effect_get_mode_setup_from_command_line(
    const char* command_line
)
{
    const char* value;
    s_command_argument_reader reader;
    e_damage_recursive_logic result = 0;

    /*
    * Read all arguments left to right. We send each arg
    * to function that interprets the value to get appropriate
    * bit to toggle.
    */

    command_argument_reader_initialize(&reader, command_line, 1);

    while(command_argument_reader_next(&reader, &value)) {
        result |= recursive_effect_get_mode_flag_from_argument(value);
    }

    return result;
}

/*
* Caskey, Damon V.
* 2021-08-24
*
* Interpret integer input and return mode 
* value with appropriate bit flags toggled. 
* an integer argument. This is for legacy
* support of the very poorly conceived mode
* flag originally coded by yours truly.
*/
e_damage_recursive_logic recursive_effect_get_mode_setup_from_legacy_argument(e_damage_recursive_cmd_read value) {
    e_damage_recursive_logic result = DAMAGE_RECURSIVE_MODE_NONE;

    switch (value)
    {
    case DAMAGE_RECURSIVE_CMD_READ_NONLETHAL_HP:
        result |= DAMAGE_RECURSIVE_MODE_HP;
        result |= DAMAGE_RECURSIVE_MODE_NON_LETHAL;
        break;
    case DAMAGE_RECURSIVE_CMD_READ_MP:
        result |= DAMAGE_RECURSIVE_MODE_MP;
        break;
    case DAMAGE_RECURSIVE_CMD_READ_MP_NONLETHAL_HP:
        result |= DAMAGE_RECURSIVE_MODE_HP;
        result |= DAMAGE_RECURSIVE_MODE_MP;
        result |= DAMAGE_RECURSIVE_MODE_NON_LETHAL;
        break;
    case DAMAGE_RECURSIVE_CMD_READ_HP:
        result |= DAMAGE_RECURSIVE_MODE_HP;
        break;
    case DAMAGE_RECURSIVE_CMD_READ_HP_MP:
        result |= DAMAGE_RECURSIVE_MODE_HP;
        result |= DAMAGE_RECURSIVE_MODE_MP;
        break;
    }

    return result;
}

/*
* Caskey, Damon V.
* 2009-06-17
* --2018-01-02 retooled from former common_dot.
* --2019-01-16 Replace recursion array with linked list.
* --2026-06-01 Restore resident array and add bitmask scan.
*
* Apply recursive effect (damage over time (dot)).
*/
void recursive_entity_effect_update(entity* acting_entity) {
    s_attack attack;
    const s_defense* defense_object = NULL;
    s_recursive_effect* recursive_effect_collection;
    s_recursive_effect* cursor = NULL;
    s_recursive_effect snapshot;
    uint64_t scan_mask;
    int calculated_force;

    if (!acting_entity) {
        return;
    }

    /*
    * No collection means this entity has never
    * received a recursive effect.
    */
    recursive_effect_collection = acting_entity->recursive_effect_collection;

    if (!recursive_effect_collection) {
        /*
        * Keep mask and collection state synchronized
        * if outside code ever corrupts the invariant.
        */
        acting_entity->recursive_effect_active = 0;
        return;
    }

    scan_mask = acting_entity->recursive_effect_active;

    while (scan_mask) {
        uint64_t index;
        uint64_t active_flag;

        index = bitmask64_get_lowest_index(scan_mask);
        active_flag = bitmask64_from_index(index);

        if (!active_flag) {
            break;
        }

        scan_mask &= ~active_flag;

        if (index >= MAX_RECURSIVE_EFFECTS) {
            acting_entity->recursive_effect_active &= ~active_flag;
            continue;
        }

        cursor = &recursive_effect_collection[index];

        /*
        * If time has expired, clear the slot and exit this
        * loop iteration.
        */
        if (_time > cursor->time) {
            acting_entity->recursive_effect_active &= ~active_flag;
            memset(cursor, 0, sizeof(*cursor));
            cursor->type = ATK_NONE;
            continue;
        }

        /*
        * If it is not yet time for a tick, exit this iteration
        * of loop.
        */
        if (_time < cursor->tick) {
            continue;
        }

        /*
        * If target is not alive, exit this iteration of loop.
        */
        if (acting_entity->energy_state.health_current <= 0) {
            continue;
        }

        /*
        * Snapshot the recursive effect before we do anything
        * that could invoke scripts, takedamage(), or other side
        * effects that might clear or rewrite the resident slot.
        */
        snapshot = *cursor;

        /*
        * Reset next tick time on the resident slot. Use the
        * snapshot rate so this tick is stable even if scripts
        * later modify the slot.
        */
        cursor->tick = _time + (snapshot.rate * global_config.game_speed / 100);

        /*
        * Does this recursive effect affect MP?
        */
        if (snapshot.mode & DAMAGE_RECURSIVE_MODE_MP) {

            /*
            * Recursive MP Damage Logic:
            *
            * Subtract recursive force from MP. If MP would
            * end with negative value, set 0.
            */
            acting_entity->energy_state.mp_current -= snapshot.force;

            if (acting_entity->energy_state.mp_current < 0) {
                acting_entity->energy_state.mp_current = 0;
            }
        }

        /*
        * Does this recursive effect affect HP?
        */
        if (snapshot.mode & DAMAGE_RECURSIVE_MODE_HP) {

            /*
            * Recursive HP Damage Logic:
            *
            * Normally it is preferable to apply takedamage(),
            * any time we want to damage a target, but because
            * it breaks grabs and would spam the HUD,
            * takedamage() is not tenable for every tick
            * of a recursive effect effect. However, we DO want
            * the owner to get credit, grabs to be broken, HUD
            * to react, etc., if the target is KO'd.
            *
            * To handle both needs, we will first factor offense
            * and defense manually to get a calculated force. If
            * the calculated force is sufficient to KO target, and
            * this recursive tick is allowed to KO, we will go ahead
            * and apply takedamage() using the original recursive
            * force. takedamage() automatically calculates offense
            * and defense. This way the engine will treat KO tick as
            * if it were a direct hit with all appropriate reactions
            * and credit. Otherwise, we'll just subtract the calculated
            * force directly from target's HP for a silent damage effect.
            */

            /*
            * Populate local attack structure with recursive
            * damage values and apply any damage mitigation.
            */
            attack = emptyattack;
            attack.attack_type = snapshot.type;
            attack.attack_force = snapshot.force;
            attack.dropv = default_model_dropv;
            attack.meta_tag = snapshot.meta_tag;

            defense_object = defense_find_current_object(acting_entity, NULL, attack.attack_type);
            calculated_force = calculate_force_damage(acting_entity, snapshot.owner, &attack, defense_object, FALSE);

            /*
            * Force is sufficient to KO target. Do we have 
            * permission to KO with this recursive effect?
            */
            if (calculated_force >= acting_entity->energy_state.health_current) {

                /*
                * We can KO with this recursive effect. 
                */

                if (!(snapshot.mode & DAMAGE_RECURSIVE_MODE_NON_LETHAL)) {

                    /*
                    * Do we have a takedamage structure? If so
                    * we can use takedamage() for the finishing 
                    * damage. Otherwise we are a none type or some 
                    * other exceptional entity without a takedamage
                    * function assigned. Just kill ourself.
                    */

                    if (acting_entity->takedamage) {
                        /* Pass raw force so takedamage applies normal calculation once. */
                        attack.attack_force = snapshot.force;
                        acting_entity->takedamage(acting_entity, snapshot.owner, &attack, 0, defense_object);
                    } else {
                        kill_entity(acting_entity, KILL_ENTITY_TRIGGER_RECURSIVE_EFFECT);
                    }

                } else {

                    /*
                    * Recursive effect is not allowed to KO.
                    * Just set target's HP to minimum value.
                    */

                    attack.attack_force = calculated_force;
                    acting_entity->energy_state.health_current = 1;
                    execute_takedamage_script(acting_entity, snapshot.owner, &attack);
                }

            } else {

                /*
                * Calculated damage is insufficient to KO.
                * Subtract directly from target's HP.
                */

                attack.attack_force = calculated_force;
                acting_entity->energy_state.health_current -= calculated_force;
                execute_takedamage_script(acting_entity, snapshot.owner, &attack);
            }
        }
        
        /*
        * Damage processing may have killed the entity and
        * released or replaced its recursive effect collection.
        * Never continue scanning the old allocation.
        */
        if (!acting_entity->exists ||
            acting_entity->recursive_effect_collection != recursive_effect_collection) {

            return;
        }
    }
}

/*
* Caskey, Damon V.
* 2026-08-06
*
* Convert subdivisions of a second to logical clock ticks without
* overflowing the finite 32-bit delay representation.
* DELAY_INFINITE remains a sentinel instead of entering
* the conversion, and finite overflow clamps to
* DELAY_FINITE_MAX instead of creating the sentinel.
*/
static uint64_t delay_subsecond_units_to_ticks(
    const uint64_t value,
    const uint64_t units_per_second
) {
    const uint64_t whole_seconds =
        value / units_per_second;

    const uint64_t remaining_units =
        value % units_per_second;

    const uint64_t tick_whole_units =
        global_config.game_speed / units_per_second;

    const uint64_t tick_remainder =
        global_config.game_speed % units_per_second;

    uint64_t result;
    uint64_t remainder_ticks;

    if(value & DELAY_FLAG_INFINITE) {
        return value;
    }

    if(whole_seconds
        && global_config.game_speed
            > DELAY_FINITE_MAX / whole_seconds) {
        return DELAY_FINITE_MAX;
    }

    result = whole_seconds * global_config.game_speed;

    if(remaining_units
        && tick_whole_units
            > (DELAY_FINITE_MAX - result)
                / remaining_units) {
        return DELAY_FINITE_MAX;
    }

    result += remaining_units
        * tick_whole_units;

    /*
    * Supported subdivisions are at most 1,000 units per
    * second, so this product cannot overflow.
    */
    remainder_ticks = remaining_units
        * tick_remainder
        / units_per_second;

    if(remainder_ticks > DELAY_FINITE_MAX - result) {
        return DELAY_FINITE_MAX;
    }

    return result + remainder_ticks;
}

static uint64_t delay_centiseconds_to_ticks(const uint64_t centiseconds) {
    return delay_subsecond_units_to_ticks(
        centiseconds,
        UINT64_C(100)
    );
}

static uint64_t delay_milliseconds_to_ticks(const uint64_t milliseconds) {
    return delay_subsecond_units_to_ticks(
        milliseconds,
        UINT64_C(1000)
    );
}

/*
* Caskey, Damon V.
* 2026-08-06
*
* Convert whole time units to logical clock ticks. The
* supplied seconds-per-unit multiplier supports seconds
* and minutes while keeping every intermediate within the
* finite delay range.
*/
static uint64_t delay_whole_units_to_ticks(
    const uint64_t value,
    const uint64_t seconds_per_unit
) {
    uint64_t seconds;

    if(value & DELAY_FLAG_INFINITE) {
        return value;
    }

    if(!value || !seconds_per_unit || !global_config.game_speed) {
        return 0;
    }

    if(seconds_per_unit > DELAY_FINITE_MAX / value) {
        return DELAY_FINITE_MAX;
    }

    seconds = value * seconds_per_unit;

    if(global_config.game_speed > DELAY_FINITE_MAX / seconds) {
        return DELAY_FINITE_MAX;
    }

    return seconds * global_config.game_speed;
}

/*
* Convert an animation delay from its declared input unit
* to logical clock ticks.
*/
static uint64_t delay_to_ticks(
    const uint64_t delay,
    const e_delay_unit mode
) {
    const e_delay_unit resolved_mode =
        mode == DELAY_UNIT_GLOBAL
            ? global_config.delay_unit
            : mode;

    switch(resolved_mode) {
        case DELAY_UNIT_GLOBAL:
            /* Defensive fallback for invalid internal configuration. */
        case DELAY_UNIT_CENTISECOND:
            return delay_centiseconds_to_ticks(delay);
        case DELAY_UNIT_MILLISECOND:
            return delay_milliseconds_to_ticks(delay);
        case DELAY_UNIT_SECOND:
            return delay_whole_units_to_ticks(delay, UINT64_C(1));
        case DELAY_UNIT_MINUTE:
            return delay_whole_units_to_ticks(delay, UINT64_C(60));
        case DELAY_UNIT_DIRECT:
        default:
            return delay;
    }
}



/*
* Caskey, Damon V. (original author unknown, 
* reworked to the point it's essentially a 
* new funciton)
* 2020-03-04 (Previous reworks 2016).
*
* Allocate a frame to animation and add frame
* properties as needed. In OpenBOR, frames are 
* a bottom up structure: There is no single frame 
* structure with sub properties. Instead, each 
* "frame" property is an arrayed animation 
* property array with number of elements matched 
* to number of desired frames.
*/
int addframe(s_addframe_data* data) {

    ptrdiff_t currentframe;

    if(data->framecount > 0) {
        alloc_frames(data->animation, data->framecount);
    } else {
        data->framecount = -data->framecount;    // for alloc method, use a negative value
    }

    currentframe = data->animation->numframes;
    ++data->animation->numframes;

    data->animation->sprite[currentframe] = data->spriteindex;
    data->animation->delay[currentframe] = delay_to_ticks(
        data->delay,
        data->delay_mode
    );

    /* Allocate collision. */
    collision_attack_initialize_frame_property(data, currentframe);
    collision_body_initialize_frame_property(data, currentframe);
    collision_space_initialize_frame_property(data, currentframe);
    
    /* Child spawns. */
    child_spawn_initialize_frame_property(data, currentframe);

    /* Frame sounds. */
    frame_sound_initialize_frame_property(data, currentframe);

    // Drawmethod (graphic settings)
    if(data->drawmethod->config & DRAWMETHOD_CONFIG_ENABLED) {
        if(!data->animation->drawmethods) {
            data->animation->drawmethods = malloc(data->framecount * sizeof(*data->animation->drawmethods));
            memset(data->animation->drawmethods, 0, data->framecount * sizeof(*data->animation->drawmethods));
        }
        setDrawMethod(data->animation, currentframe, malloc(sizeof(**data->animation->drawmethods)));
        //data->animation->drawmethods[currenframe] = malloc(sizeof(s_drawmethod));
        memcpy(getDrawMethod(data->animation, currentframe), data->drawmethod, sizeof(**data->animation->drawmethods));
        //memcpy(data->animation->drawmethods[currentframe], data->drawmethod, sizeof(s_drawmethod));
    }

    // Idle flag.
    if(data->idle && !data->animation->idle) {
        data->animation->idle = malloc(data->framecount * sizeof(*data->animation->idle));
        memset(data->animation->idle, 0, data->framecount * sizeof(*data->animation->idle));
    }

    if(data->animation->idle) {
        data->animation->idle[currentframe] = data->idle;
    }

    // Movement
    if(data->move) {
        if(!data->animation->move) {
            data->animation->move = malloc(data->framecount * sizeof(*data->animation->move));
            memset(data->animation->move, 0, data->framecount * sizeof(*data->animation->move));
        }
        data->animation->move[currentframe] = malloc(sizeof(**data->animation->move));
        memcpy(data->animation->move[currentframe], data->move, sizeof(**data->animation->move));
    }

    // Shadow effects.
    if(data->frameshadow >= 0 && !data->animation->shadow) {
        data->animation->shadow = malloc(data->framecount * sizeof(*data->animation->shadow));
        memset(data->animation->shadow, FRAME_SHADOW_NONE, data->framecount * sizeof(*data->animation->shadow));
    }

    if(data->animation->shadow) {
        data->animation->shadow[currentframe] = data->frameshadow;    // shadow index for each frame
    }

    if(data->shadow_coords[0] || data->shadow_coords[1]) {
        if(!data->animation->shadow_coords) {
            data->animation->shadow_coords = malloc(data->framecount * sizeof(*data->animation->shadow_coords));
            memset(data->animation->shadow_coords, 0, data->framecount * sizeof(*data->animation->shadow_coords));
        }
        memcpy(data->animation->shadow_coords[currentframe], data->shadow_coords, sizeof(*data->animation->shadow_coords));
    }    

    // Offset
    if(data->offset->x || data->offset->y) {
        if(!data->animation->offset) {
            data->animation->offset = malloc(data->framecount * sizeof(*data->animation->offset));
            memset(data->animation->offset, 0, data->framecount * sizeof(*data->animation->offset));
        }
        data->animation->offset[currentframe] = malloc(sizeof(**data->animation->offset));
        memcpy(data->animation->offset[currentframe], data->offset, sizeof(**data->animation->offset));
    }

    // Platform
    if(data->platform[PLATFORM_HEIGHT]) { //height
        if(!data->animation->platform) {
            data->animation->platform = malloc(data->framecount * sizeof(*data->animation->platform));
            memset(data->animation->platform, 0, data->framecount * sizeof(*data->animation->platform));
        }
        memcpy(data->animation->platform[currentframe], data->platform, sizeof(*data->animation->platform));// Used so entity can be landed on
    }

    return data->animation->numframes;
}


// ok this func only seems to overwrite the name which was assigned from models.txt with the one
// in the models own text file.
// it does so in the cache.
void _peek_model_name(int index)
{
    size_t size = 0;
    ptrdiff_t pos = 0, len;
    char *buf = NULL;
    char *command, *value;
    ArgList arglist;
    char argbuf[MAX_ARG_LEN + 1] = "";
    modelCommands cmd;

    if(buffer_pakfile(model_cache[index].path, &buf, &size) != 1)
    {
        return;
    }

    while(pos < size)
    {
        ParseArgs(&arglist, buf + pos, argbuf);
        command = GET_ARG(0);

        if(command && command[0])
        {
            cmd = getModelCommand(modelcmdlist, command);
            if(cmd == CMD_MODEL_NAME)
            {
                value = GET_ARG(1);
                free(model_cache[index].name);
                model_cache[index].name = NULL;
                len = strlen(value);
                model_cache[index].name = malloc(len + 1);
                strcpy(model_cache[index].name, value);
                model_cache[index].name[len] = 0;
                break;
            }
        }
        pos += getNewLineStart(buf + pos);
    }

    if(buf != NULL)
    {
        free(buf);
        buf = NULL;
    }
}

void prepare_cache_map(size_t size)
{
    if(model_cache == NULL || size + 1 > cache_map_max_items )
    {
#ifdef VERBOSE
        printf("%s %p\n", "prepare_cache_map was", model_cache);
#endif
        do
        {
            cache_map_max_items += 128;
        }
        while (size + 1 > cache_map_max_items);

        model_cache = realloc(model_cache, sizeof(*model_cache) * cache_map_max_items);
        if(model_cache == NULL)
        {
            borShutdown(1, "Out Of Memory!  Failed to create a new cache_map\n");
        }
    }
}

void cache_model(char *name, char *path, int flag)
{
    int len;
    printf("Caching '%s' from %s\n", name, path);
    prepare_cache_map(models_cached + 1);
    memset(&model_cache[models_cached], 0, sizeof(model_cache[models_cached]));

    len = strlen(name);
    model_cache[models_cached].name = malloc(len + 1);
    strcpy(model_cache[models_cached].name, name);
    model_cache[models_cached].name[len] = 0;

    len = strlen(path);
    model_cache[models_cached].path = malloc(len + 1);
    strcpy(model_cache[models_cached].path, path);
    model_cache[models_cached].path[len] = 0;

    model_cache[models_cached].loadflag = flag;
    model_cache[models_cached].load_script = alloc_script();
    model_cache[models_cached].unload_script = alloc_script();

    _peek_model_name(models_cached);
    ++models_cached;
}


void free_modelcache()
{
    if(model_cache != NULL)
    {
        while(models_cached)
        {
            --models_cached;
            Script_Clear(model_cache[models_cached].load_script, 2);
            free(model_cache[models_cached].load_script);
            model_cache[models_cached].load_script = NULL;
            Script_Clear(model_cache[models_cached].unload_script, 2);
            free(model_cache[models_cached].unload_script);
            model_cache[models_cached].unload_script = NULL;
            free(model_cache[models_cached].name);
            model_cache[models_cached].name = NULL;
            free(model_cache[models_cached].path);
            model_cache[models_cached].path = NULL;
        }
        free(model_cache);
        model_cache = NULL;
    }
}


int get_cached_model_index(const char *name)
{
    int i;
    for(i = 0; i < models_cached; i++)
    {
        if(stricmp(name, model_cache[i].name) == 0)
        {
            return i;
        }
    }
    return MODEL_INDEX_NONE;
}

char *get_cached_model_path(char *name)
{
    int i;
    for(i = 0; i < models_cached; i++)
    {
        if(stricmp(name, model_cache[i].name) == 0)
        {
            return model_cache[i].path;
        }
    }
    return NULL;
}

static void _readbarstatus(char *, s_barstatus *);

static int translate_attack_type(char* command, char* filename)
{
    int atk_id = -1, tempInt;

    modelCommands cmd = getModelCommand(modelcmdlist, command);

    switch(cmd)
    {
    case CMD_MODEL_COLLISION:
    case CMD_MODEL_COLLISION1:
        atk_id = ATK_NORMAL;
        break;
    case CMD_MODEL_COLLISION2:
        atk_id   = ATK_NORMAL2;
        break;
    case CMD_MODEL_COLLISION3:
        atk_id  = ATK_NORMAL3;
        break;
    case CMD_MODEL_COLLISION4:
        atk_id  = ATK_NORMAL4;
        break;
    case CMD_MODEL_COLLISION5:
        atk_id  = ATK_NORMAL5;
        break;
    case CMD_MODEL_COLLISION6:
        atk_id  = ATK_NORMAL6;
        break;
    case CMD_MODEL_COLLISION7:
        atk_id  = ATK_NORMAL7;
        break;
    case CMD_MODEL_COLLISION8:
        atk_id  = ATK_NORMAL8;
        break;
    case CMD_MODEL_COLLISION9:
        atk_id  = ATK_NORMAL9;
        break;
    case CMD_MODEL_COLLISION10:
        atk_id  = ATK_NORMAL10;
        break;
    case CMD_MODEL_SHOCK:
        atk_id  = ATK_SHOCK;
        break;
    case CMD_MODEL_BURN:
        atk_id  = ATK_BURN;
        break;
    case CMD_MODEL_STEAL:
        atk_id  = ATK_STEAL;
        break;
    case CMD_MODEL_FREEZE:
        atk_id  = ATK_FREEZE;
        break;
    case CMD_MODEL_ITEMBOX:
        atk_id  = ATK_ITEM;
        break;
    case CMD_MODEL_LOSE:
        atk_id  = ATK_LOSE;
        break;
    case CMD_MODEL_COLLISION_ETC:
        
        tempInt = get_attack_type_from_string(command, filename);
        
        //tmpInt = atoi(command + 6); // White Dragon: 6 is "ATTACK" string length
		
		if(tempInt < MAX_ATKS - STA_ATKS + 1)
        {
            tempInt = MAX_ATKS - STA_ATKS + 1;
        }
        atk_id = tempInt + STA_ATKS - 1;        

        break;
    default:
        break;
    }

    return atk_id;
}

//move here to ease animation name to id logic
static int translate_ani_id(const char *value, s_model *newchar, s_anim *newanim)
{
    int ani_id = -1, tempInt;
    //those are dummy values to simplify code
    static s_model mdl;
    static s_anim ani;
    if(!newchar)
    {
        newchar = &mdl;
    }
    if(!newanim)
    {
        newanim = &ani;
    }
    
    if(starts_with_num(value, "idle"))
    {
        get_tail_number(tempInt, value, "idle");
        ani_id = animidles[tempInt - 1];
    }
    else if(stricmp(value, "waiting") == 0)
    {
        ani_id = ANI_SELECT;
    }
	else if (stricmp(value, "selectin") == 0)
	{		
		ani_id = ANI_SELECTIN;
	}
	else if (stricmp(value, "selectout") == 0)
	{
		ani_id = ANI_SELECTOUT;
	}
    else if(starts_with_num(value, "walk"))
    {
        get_tail_number(tempInt, value, "walk");
        ani_id = animwalks[tempInt - 1];
        newanim->sync = ANI_WALK;

    }
    else if(stricmp(value, "sleep") == 0)
    {
        ani_id = ANI_SLEEP;
    }
    else if(stricmp(value, "run") == 0)
    {
        ani_id = ANI_RUN;
    }
    else if(stricmp(value, "backrun") == 0)
    {
        ani_id = ANI_BACKRUN;
    }
    else if(starts_with_num(value, "up"))
    {
        get_tail_number(tempInt, value, "up");
        ani_id = animups[tempInt - 1];
        newanim->sync = ANI_WALK;
    }
    else if(starts_with_num(value, "down"))
    {
        get_tail_number(tempInt, value, "down");
        ani_id = animdowns[tempInt - 1];
        newanim->sync = ANI_WALK;
    }
    else if(starts_with_num(value, "backwalk"))
    {
        get_tail_number(tempInt, value, "backwalk");
        ani_id = animbackwalks[tempInt - 1];
        newanim->sync = ANI_WALK;
    }
    else if(stricmp(value, "jump") == 0)
    {
        ani_id = ANI_JUMP;
        newanim->range.x.min = 50;
        newanim->range.x.max = 60;
    }
    else if(stricmp(value, "duck") == 0)
    {
        ani_id = ANI_DUCK;
    }
    else if(stricmp(value, "land") == 0)
    {
        ani_id = ANI_LAND;
    }
    else if(starts_with_num(value, "pain"))
    {
        get_tail_number(tempInt, value, "pain");
        if(tempInt == 1)
        {
            ani_id = ANI_PAIN;
        }
        else if(tempInt == 2)
        {
            ani_id = ANI_PAIN2;
        }
        else if(tempInt == 3)
        {
            ani_id = ANI_PAIN3;
        }
        else if(tempInt == 4)
        {
            ani_id = ANI_PAIN4;
        }
        else if(tempInt == 5)
        {
            ani_id = ANI_PAIN5;
        }
        else if(tempInt == 6)
        {
            ani_id = ANI_PAIN6;
        }
        else if(tempInt == 7)
        {
            ani_id = ANI_PAIN7;
        }
        else if(tempInt == 8)
        {
            ani_id = ANI_PAIN8;
        }
        else if(tempInt == 9)
        {
            ani_id = ANI_PAIN9;
        }
        else if(tempInt == 10)
        {
            ani_id = ANI_PAIN10;
        }
        else
        {
            if(tempInt < MAX_ATKS - STA_ATKS + 1)
            {
                tempInt = MAX_ATKS - STA_ATKS + 1;
            }
            ani_id = animpains[tempInt + STA_ATKS - 1];
        }
    }
    else if(starts_with_num(value, "backpain"))
    {
        get_tail_number(tempInt, value, "backpain");
        if(tempInt == 1)
        {
            ani_id = ANI_BACKPAIN;
        }
        else if(tempInt == 2)
        {
            ani_id = ANI_BACKPAIN2;
        }
        else if(tempInt == 3)
        {
            ani_id = ANI_BACKPAIN3;
        }
        else if(tempInt == 4)
        {
            ani_id = ANI_BACKPAIN4;
        }
        else if(tempInt == 5)
        {
            ani_id = ANI_BACKPAIN5;
        }
        else if(tempInt == 6)
        {
            ani_id = ANI_BACKPAIN6;
        }
        else if(tempInt == 7)
        {
            ani_id = ANI_BACKPAIN7;
        }
        else if(tempInt == 8)
        {
            ani_id = ANI_BACKPAIN8;
        }
        else if(tempInt == 9)
        {
            ani_id = ANI_BACKPAIN9;
        }
        else if(tempInt == 10)
        {
            ani_id = ANI_BACKPAIN10;
        }
        else
        {
            if(tempInt < MAX_ATKS - STA_ATKS + 1)
            {
                tempInt = MAX_ATKS - STA_ATKS + 1;
            }
            ani_id = animbackpains[tempInt + STA_ATKS - 1];
        }
    }
    else if(stricmp(value, "spain") == 0)   // If shock attacks don't knock opponent down, play this
    {
        ani_id = ANI_SHOCKPAIN;
    }
    else if(stricmp(value, "bpain") == 0)   // If burn attacks don't knock opponent down, play this
    {
        ani_id = ANI_BURNPAIN;
    }
    else if(stricmp(value, "backspain") == 0)   // If shock attacks don't knock opponent down, play this
    {
        ani_id = ANI_BACKSHOCKPAIN;
    }
    else if(stricmp(value, "backbpain") == 0)   // If burn attacks don't knock opponent down, play this
    {
        ani_id = ANI_BACKBURNPAIN;
    }
    else if(starts_with_num(value, "fall"))
    {
        get_tail_number(tempInt, value, "fall");
        if(tempInt == 1)
        {
            ani_id = ANI_FALL;
        }
        else if(tempInt == 2)
        {
            ani_id = ANI_FALL2;
        }
        else if(tempInt == 3)
        {
            ani_id = ANI_FALL3;
        }
        else if(tempInt == 4)
        {
            ani_id = ANI_FALL4;
        }
        else if(tempInt == 5)
        {
            ani_id = ANI_FALL5;
        }
        else if(tempInt == 6)
        {
            ani_id = ANI_FALL6;
        }
        else if(tempInt == 7)
        {
            ani_id = ANI_FALL7;
        }
        else if(tempInt == 8)
        {
            ani_id = ANI_FALL8;
        }
        else if(tempInt == 9)
        {
            ani_id = ANI_FALL9;
        }
        else if(tempInt == 10)
        {
            ani_id = ANI_FALL10;
        }
        else
        {
            if(tempInt < MAX_ATKS - STA_ATKS + 1)
            {
                tempInt = MAX_ATKS - STA_ATKS + 1;
            }
            ani_id = animfalls[tempInt + STA_ATKS - 1];
        }
        newanim->bounce_factor = ANIMATION_BOUNCE_FACTOR_DEFAULT;
    }
    else if(starts_with_num(value, "backfall"))
    {
        get_tail_number(tempInt, value, "backfall");
        if(tempInt == 1)
        {
            ani_id = ANI_BACKFALL;
        }
        else if(tempInt == 2)
        {
            ani_id = ANI_BACKFALL2;
        }
        else if(tempInt == 3)
        {
            ani_id = ANI_BACKFALL3;
        }
        else if(tempInt == 4)
        {
            ani_id = ANI_BACKFALL4;
        }
        else if(tempInt == 5)
        {
            ani_id = ANI_BACKFALL5;
        }
        else if(tempInt == 6)
        {
            ani_id = ANI_BACKFALL6;
        }
        else if(tempInt == 7)
        {
            ani_id = ANI_BACKFALL7;
        }
        else if(tempInt == 8)
        {
            ani_id = ANI_BACKFALL8;
        }
        else if(tempInt == 9)
        {
            ani_id = ANI_BACKFALL9;
        }
        else if(tempInt == 10)
        {
            ani_id = ANI_BACKFALL10;
        }
        else
        {
            if(tempInt < MAX_ATKS - STA_ATKS + 1)
            {
                tempInt = MAX_ATKS - STA_ATKS + 1;
            }
            ani_id = animbackfalls[tempInt + STA_ATKS - 1];
        }
        newanim->bounce_factor = ANIMATION_BOUNCE_FACTOR_DEFAULT;
    }
    else if(stricmp(value, "shock") == 0)   // If shock attacks do knock opponent down, play this
    {
        ani_id = ANI_SHOCK;
        newanim->bounce_factor = ANIMATION_BOUNCE_FACTOR_DEFAULT;
    }
    else if(stricmp(value, "backshock") == 0)   // If shock attacks do knock opponent down, play this
    {
        ani_id = ANI_BACKSHOCK;
        newanim->bounce_factor = ANIMATION_BOUNCE_FACTOR_DEFAULT;
    }
    else if(stricmp(value, "burn") == 0)   // If burn attacks do knock opponent down, play this
    {
        ani_id = ANI_BURN;
        newanim->bounce_factor = ANIMATION_BOUNCE_FACTOR_DEFAULT;
    }
    else if(stricmp(value, "backburn") == 0)   // If burn attacks do knock opponent down, play this
    {
        ani_id = ANI_BACKBURN;
        newanim->bounce_factor = ANIMATION_BOUNCE_FACTOR_DEFAULT;
    }
    else if(starts_with_num(value, "death"))
    {
        get_tail_number(tempInt, value, "death");
        if(tempInt == 1)
        {
            ani_id = ANI_DIE;
        }
        else if(tempInt == 2)
        {
            ani_id = ANI_DIE2;
        }
        else if(tempInt == 3)
        {
            ani_id = ANI_DIE3;
        }
        else if(tempInt == 4)
        {
            ani_id = ANI_DIE4;
        }
        else if(tempInt == 5)
        {
            ani_id = ANI_DIE5;
        }
        else if(tempInt == 6)
        {
            ani_id = ANI_DIE6;
        }
        else if(tempInt == 7)
        {
            ani_id = ANI_DIE7;
        }
        else if(tempInt == 8)
        {
            ani_id = ANI_DIE8;
        }
        else if(tempInt == 9)
        {
            ani_id = ANI_DIE9;
        }
        else if(tempInt == 10)
        {
            ani_id = ANI_DIE10;
        }
        else
        {
            if(tempInt < MAX_ATKS - STA_ATKS + 1)
            {
                tempInt = MAX_ATKS - STA_ATKS + 1;
            }
            ani_id = animdies[tempInt + STA_ATKS - 1];
        }
    }
    else if(starts_with_num(value, "backdeath"))
    {
        get_tail_number(tempInt, value, "backdeath");
        if(tempInt == 1)
        {
            ani_id = ANI_BACKDIE;
        }
        else if(tempInt == 2)
        {
            ani_id = ANI_BACKDIE2;
        }
        else if(tempInt == 3)
        {
            ani_id = ANI_BACKDIE3;
        }
        else if(tempInt == 4)
        {
            ani_id = ANI_BACKDIE4;
        }
        else if(tempInt == 5)
        {
            ani_id = ANI_BACKDIE5;
        }
        else if(tempInt == 6)
        {
            ani_id = ANI_BACKDIE6;
        }
        else if(tempInt == 7)
        {
            ani_id = ANI_BACKDIE7;
        }
        else if(tempInt == 8)
        {
            ani_id = ANI_BACKDIE8;
        }
        else if(tempInt == 9)
        {
            ani_id = ANI_BACKDIE9;
        }
        else if(tempInt == 10)
        {
            ani_id = ANI_BACKDIE10;
        }
        else
        {
            if(tempInt < MAX_ATKS - STA_ATKS + 1)
            {
                tempInt = MAX_ATKS - STA_ATKS + 1;
            }
            ani_id = animbackdies[tempInt + STA_ATKS - 1];
        }
    }
    else if(stricmp(value, "sdie") == 0)
    {
        ani_id = ANI_SHOCKDIE;
    }
    else if(stricmp(value, "bdie") == 0)
    {
        ani_id = ANI_BURNDIE;
    }
    else if(stricmp(value, "backsdie") == 0)
    {
        ani_id = ANI_BACKSHOCKDIE;
    }
    else if(stricmp(value, "backbdie") == 0)
    {
        ani_id = ANI_BACKBURNDIE;
    }
    else if(stricmp(value, "chipdeath") == 0)
    {
        ani_id = ANI_CHIPDEATH;
    }
    else if(stricmp(value, "guardbreak") == 0)
    {
        ani_id = ANI_GUARDBREAK;
    }
    else if(stricmp(value, "riseb") == 0)
    {
        ani_id = ANI_RISEB;
    }
    else if(stricmp(value, "backriseb") == 0)
    {
        ani_id = ANI_BACKRISEB;
    }
    else if(stricmp(value, "rises") == 0)
    {
        ani_id = ANI_RISES;
    }
    else if(stricmp(value, "backrises") == 0)
    {
        ani_id = ANI_BACKRISES;
    }
    else if(starts_with_num(value, "rise"))
    {
        get_tail_number(tempInt, value, "rise");
        if(tempInt == 1)
        {
            ani_id = ANI_RISE;
        }
        else if(tempInt == 2)
        {
            ani_id = ANI_RISE2;
        }
        else if(tempInt == 3)
        {
            ani_id = ANI_RISE3;
        }
        else if(tempInt == 4)
        {
            ani_id = ANI_RISE4;
        }
        else if(tempInt == 5)
        {
            ani_id = ANI_RISE5;
        }
        else if(tempInt == 6)
        {
            ani_id = ANI_RISE6;
        }
        else if(tempInt == 7)
        {
            ani_id = ANI_RISE7;
        }
        else if(tempInt == 8)
        {
            ani_id = ANI_RISE8;
        }
        else if(tempInt == 9)
        {
            ani_id = ANI_RISE9;
        }
        else if(tempInt == 10)
        {
            ani_id = ANI_RISE10;
        }
        else
        {
            if(tempInt < MAX_ATKS - STA_ATKS + 1)
            {
                tempInt = MAX_ATKS - STA_ATKS + 1;
            }
            ani_id = animrises[tempInt + STA_ATKS - 1];
        }
    }
    else if(starts_with_num(value, "backrise"))
    {
        get_tail_number(tempInt, value, "backrise");
        if(tempInt == 1)
        {
            ani_id = ANI_BACKRISE;
        }
        else if(tempInt == 2)
        {
            ani_id = ANI_BACKRISE2;
        }
        else if(tempInt == 3)
        {
            ani_id = ANI_BACKRISE3;
        }
        else if(tempInt == 4)
        {
            ani_id = ANI_BACKRISE4;
        }
        else if(tempInt == 5)
        {
            ani_id = ANI_BACKRISE5;
        }
        else if(tempInt == 6)
        {
            ani_id = ANI_BACKRISE6;
        }
        else if(tempInt == 7)
        {
            ani_id = ANI_BACKRISE7;
        }
        else if(tempInt == 8)
        {
            ani_id = ANI_BACKRISE8;
        }
        else if(tempInt == 9)
        {
            ani_id = ANI_BACKRISE9;
        }
        else if(tempInt == 10)
        {
            ani_id = ANI_BACKRISE10;
        }
        else
        {
            if(tempInt < MAX_ATKS - STA_ATKS + 1)
            {
                tempInt = MAX_ATKS - STA_ATKS + 1;
            }
            ani_id = animbackrises[tempInt + STA_ATKS - 1];
        }
    }
    else if(stricmp(value, "riseattackb") == 0)
    {
        ani_id = ANI_RISEATTACKB;
    }
    else if(stricmp(value, "backriseattackb") == 0)
    {
        ani_id = ANI_BACKRISEATTACKB;
    }
    else if(stricmp(value, "riseattacks") == 0)
    {
        ani_id = ANI_RISEATTACKS;
    }
    else if(stricmp(value, "backriseattacks") == 0)
    {
        ani_id = ANI_BACKRISEATTACKS;
    }
    else if(starts_with_num(value, "riseattack"))
    {
        get_tail_number(tempInt, value, "riseattack");
        if(tempInt == 1)
        {
            ani_id = ANI_RISEATTACK;
        }
        else if(tempInt == 2)
        {
            ani_id = ANI_RISEATTACK2;
        }
        else if(tempInt == 3)
        {
            ani_id = ANI_RISEATTACK3;
        }
        else if(tempInt == 4)
        {
            ani_id = ANI_RISEATTACK4;
        }
        else if(tempInt == 6)
        {
            ani_id = ANI_RISEATTACK5;
        }
        else if(tempInt == 6)
        {
            ani_id = ANI_RISEATTACK6;
        }
        else if(tempInt == 7)
        {
            ani_id = ANI_RISEATTACK7;
        }
        else if(tempInt == 8)
        {
            ani_id = ANI_RISEATTACK8;
        }
        else if(tempInt == 9)
        {
            ani_id = ANI_RISEATTACK9;
        }
        else if(tempInt == 10)
        {
            ani_id = ANI_RISEATTACK10;
        }
        else
        {
            if(tempInt < MAX_ATKS - STA_ATKS + 1)
            {
                tempInt = MAX_ATKS - STA_ATKS + 1;
            }
            ani_id = animriseattacks[tempInt + STA_ATKS - 1];
        }
    }
    else if(starts_with_num(value, "backriseattack"))
    {
        get_tail_number(tempInt, value, "backriseattack");
        if(tempInt == 1)
        {
            ani_id = ANI_BACKRISEATTACK;
        }
        else if(tempInt == 2)
        {
            ani_id = ANI_BACKRISEATTACK2;
        }
        else if(tempInt == 3)
        {
            ani_id = ANI_BACKRISEATTACK3;
        }
        else if(tempInt == 4)
        {
            ani_id = ANI_BACKRISEATTACK4;
        }
        else if(tempInt == 6)
        {
            ani_id = ANI_BACKRISEATTACK5;
        }
        else if(tempInt == 6)
        {
            ani_id = ANI_BACKRISEATTACK6;
        }
        else if(tempInt == 7)
        {
            ani_id = ANI_BACKRISEATTACK7;
        }
        else if(tempInt == 8)
        {
            ani_id = ANI_BACKRISEATTACK8;
        }
        else if(tempInt == 9)
        {
            ani_id = ANI_BACKRISEATTACK9;
        }
        else if(tempInt == 10)
        {
            ani_id = ANI_BACKRISEATTACK10;
        }
        else
        {
            if(tempInt < MAX_ATKS - STA_ATKS + 1)
            {
                tempInt = MAX_ATKS - STA_ATKS + 1;
            }
            ani_id = animbackriseattacks[tempInt + STA_ATKS - 1];
        }
    }
    else if(stricmp(value, "select") == 0)
    {
        ani_id = ANI_PICK;
    }
    else if(starts_with_num(value, "attack"))
    {
        get_tail_number(tempInt, value, "attack");
        ani_id = animattacks[tempInt - 1];
    }
    else if(stricmp(value, "throwattack") == 0)
    {
        ani_id = ANI_THROWATTACK;
    }
    else if(stricmp(value, "upper") == 0)
    {
        ani_id = ANI_UPPER;
        newanim->range.x.min = -10;
        newanim->range.x.max = 120;
    }
    else if(stricmp(value, "cant") == 0)
    {
        ani_id = ANI_CANT;
    }
    else if(stricmp(value, "jumpcant") == 0)
    {
        ani_id = ANI_JUMPCANT;
    }
    else if(stricmp(value, "charge") == 0)
    {
        ani_id = ANI_CHARGE;
    }
    else if(stricmp(value, "faint") == 0)
    {
        ani_id = ANI_FAINT;
    }
    else if(stricmp(value, "dodge") == 0)
    {
        ani_id = ANI_DODGE;
    }
    else if(stricmp(value, "special") == 0 || stricmp(value, "special1") == 0)
    {
        ani_id = ANI_SPECIAL;
        newanim->energy_cost.cost = ENERGY_COST_DEFAULT_COST;
    }
    else if(stricmp(value, "special2") == 0)
    {
        ani_id = ANI_SPECIAL2;
    }
    else if(stricmp(value, "special3") == 0 || stricmp(value, "jumpspecial") == 0)
    {
        ani_id = ANI_JUMPSPECIAL;
    }
    else if(stricmp(value, "jumpattack") == 0)
    {
        ani_id = ANI_JUMPATTACK;
        if(newchar->jumpheight == 4)
        {
            newanim->range.x.min = 150;
            newanim->range.x.max = 200;
        }
    }
    else if(stricmp(value, "jumpattack2") == 0)
    {
        ani_id = ANI_JUMPATTACK2;
    }
    else if(stricmp(value, "jumpattack3") == 0)
    {
        ani_id = ANI_JUMPATTACK3;
    }
    else if(stricmp(value, "jumpforward") == 0)
    {
        ani_id = ANI_JUMPFORWARD;
    }
    else if(stricmp(value, "runjumpattack") == 0)
    {
        ani_id = ANI_RUNJUMPATTACK;
    }
    else if(stricmp(value, "runattack") == 0)
    {
        ani_id = ANI_RUNATTACK;    // New attack for when a player is running
    }
    else if(stricmp(value, "attackup") == 0)
    {
        ani_id = ANI_ATTACKUP;    // New attack for when a player presses u u
    }
    else if(stricmp(value, "attackdown") == 0)
    {
        ani_id = ANI_ATTACKDOWN;    // New attack for when a player presses d d
    }
    else if(stricmp(value, "attackforward") == 0)
    {
        ani_id = ANI_ATTACKFORWARD;    // New attack for when a player presses f f
    }
    else if(stricmp(value, "attackbackward") == 0)
    {
        ani_id = ANI_ATTACKBACKWARD;    // New attack for when a player presses b a
    }
    else if(stricmp(value, "attackboth") == 0)   // Attack that is executed by holding down j and pressing a
    {
        ani_id = ANI_ATTACKBOTH;
    }
    else if(stricmp(value, "get") == 0)
    {
        ani_id = ANI_GET;
    }
    else if(stricmp(value, "grab") == 0)
    {
        ani_id = ANI_GRAB;
    }
    else if(stricmp(value, "backgrab") == 0) // Kratus (10-2021) Added the new backgrab animation
    {
        ani_id = ANI_BACKGRAB;
    }
    else if(stricmp(value, "vault") == 0) // Kratus (10-2021) Added the new vault animation
    {
        ani_id = ANI_VAULT;
    }
    else if(stricmp(value, "vault2") == 0) // Kratus (10-2021) Added the new vault2 animation
    {
        ani_id = ANI_VAULT2;
    }
    else if(stricmp(value, "grabwalk") == 0)
    {
        ani_id = ANI_GRABWALK;
        newanim->sync = ANI_GRABWALK;
    }
    else if(stricmp(value, "grabwalkup") == 0)
    {
        ani_id = ANI_GRABWALKUP;
        newanim->sync = ANI_GRABWALK;
    }
    else if(stricmp(value, "grabwalkdown") == 0)
    {
        ani_id = ANI_GRABWALKDOWN;
        newanim->sync = ANI_GRABWALK;
    }
    else if(stricmp(value, "grabbackwalk") == 0)
    {
        ani_id = ANI_GRABBACKWALK;
        newanim->sync = ANI_GRABWALK;
    }
    else if(stricmp(value, "grabturn") == 0)
    {
        ani_id = ANI_GRABTURN;
    }
    else if(stricmp(value, "grabbed") == 0)   // New grabbed animation for when grabbed
    {
        ani_id = ANI_GRABBED;
    }
    else if(stricmp(value, "grabbedwalk") == 0)   // New animation for when grabbed and forced to walk
    {
        ani_id = ANI_GRABBEDWALK;
        newanim->sync = ANI_GRABBEDWALK;
    }
    else if(stricmp(value, "grabbedwalkup") == 0)
    {
        ani_id = ANI_GRABWALKUP;
        newanim->sync = ANI_GRABBEDWALK;
    }
    else if(stricmp(value, "grabbedwalkdown") == 0)
    {
        ani_id = ANI_GRABWALKDOWN;
        newanim->sync = ANI_GRABBEDWALK;
    }
    else if(stricmp(value, "grabbedbackwalk") == 0)
    {
        ani_id = ANI_GRABBEDBACKWALK;
        newanim->sync = ANI_GRABBEDWALK;
    }
    else if(stricmp(value, "grabbedturn") == 0)
    {
        ani_id = ANI_GRABBEDTURN;
    }
    else if(stricmp(value, "grabattack") == 0)
    {
        ani_id = ANI_GRABATTACK;
        newanim->attack_one = 1; // default to 1, attack one one opponent
    }
    else if(stricmp(value, "grabattack2") == 0)
    {
        ani_id = ANI_GRABATTACK2;
        newanim->attack_one = 1;
    }
    else if(stricmp(value, "grabforward") == 0)   // New grab attack for when pressing forward attack
    {
        ani_id = ANI_GRABFORWARD;
        newanim->attack_one = 1;
    }
    else if(stricmp(value, "grabforward2") == 0)   // New grab attack for when pressing forward attack
    {
        ani_id = ANI_GRABFORWARD2;
        newanim->attack_one = 1;
    }
    else if(stricmp(value, "grabbackward") == 0)   // New grab attack for when pressing backward attack
    {
        ani_id = ANI_GRABBACKWARD;
        newanim->attack_one = 1;
    }
    else if(stricmp(value, "grabbackward2") == 0)   // New grab attack for when pressing backward attack
    {
        ani_id = ANI_GRABBACKWARD2;
        newanim->attack_one = 1;
    }
    else if(stricmp(value, "grabup") == 0)   // New grab attack for when pressing up attack
    {
        ani_id = ANI_GRABUP;
        newanim->attack_one = 1;
    }
    else if(stricmp(value, "grabup2") == 0)   // New grab attack for when pressing up attack
    {
        ani_id = ANI_GRABUP2;
        newanim->attack_one = 1;
    }
    else if(stricmp(value, "grabdown") == 0)   // New grab attack for when pressing down attack
    {
        ani_id = ANI_GRABDOWN;
        newanim->attack_one = 1;
    }
    else if(stricmp(value, "grabdown2") == 0)   // New grab attack for when pressing down attack
    {
        ani_id = ANI_GRABDOWN2;
        newanim->attack_one = 1;
    }
    else if(stricmp(value, "spawn") == 0)     //  spawn/respawn works separately now
    {
        ani_id = ANI_SPAWN;
    }
    else if(stricmp(value, "respawn") == 0)     //  spawn/respawn works separately now
    {
        ani_id = ANI_RESPAWN;
    }
    else if(stricmp(value, "throw") == 0)
    {
        ani_id = ANI_THROW;
    }
    else if(stricmp(value, "block") == 0)   // Now enemies can block attacks on occasion
    {
        ani_id = ANI_BLOCK;
        newanim->range.x.min = 1;
        newanim->range.x.max = 100;
    }
	else if (stricmp(value, "blockrelease") == 0) 
	{
		ani_id = ANI_BLOCKRELEASE;
	}
	else if (stricmp(value, "blockstart") == 0)
	{
		ani_id = ANI_BLOCKSTART;
	}
    else if(starts_with_num(value, "follow"))
    {
        get_tail_number(tempInt, value, "follow");
        ani_id = animfollows[tempInt - 1];
    }
    else if(stricmp(value, "chargeattack") == 0)
    {
        ani_id = ANI_CHARGEATTACK;
    }
    else if(stricmp(value, "turn") == 0)
    {
        ani_id = ANI_TURN;
    }
    else if(stricmp(value, "forwardjump") == 0)
    {
        ani_id = ANI_FORWARDJUMP;
    }
    else if(stricmp(value, "runjump") == 0)
    {
        ani_id = ANI_RUNJUMP;
    }
    else if(stricmp(value, "jumpland") == 0)
    {
        ani_id = ANI_JUMPLAND;
    }
    else if(stricmp(value, "jumpdelay") == 0)
    {
        ani_id = ANI_JUMPDELAY;
    }
    else if(stricmp(value, "hitobstacle") == 0)
    {
        ani_id = ANI_HITOBSTACLE;
    }
    else if(stricmp(value, "hitplatform") == 0)
    {
        ani_id = ANI_HITPLATFORM;
    }
    else if(stricmp(value, "hitwall") == 0)
    {
        ani_id = ANI_HITWALL;
    }
    else if(stricmp(value, "slide") == 0)
    {
        ani_id = ANI_SLIDE;
    }
    else if(stricmp(value, "runslide") == 0)
    {
        ani_id = ANI_RUNSLIDE;
    }
    else if(stricmp(value, "blockpainb") == 0)
    {
        ani_id = ANI_BLOCKPAINB;
    }
    else if(stricmp(value, "blockpains") == 0)
    {
        ani_id = ANI_BLOCKPAINS;
    }
    else if(starts_with_num(value, "blockpain"))
    {
        get_tail_number(tempInt, value, "blockpain");
        if(tempInt == 1)
        {
            ani_id = ANI_BLOCKPAIN;
        }
        else if(tempInt == 2)
        {
            ani_id = ANI_BLOCKPAIN2;
        }
        else if(tempInt == 3)
        {
            ani_id = ANI_BLOCKPAIN3;
        }
        else if(tempInt == 4)
        {
            ani_id = ANI_BLOCKPAIN4;
        }
        else if(tempInt == 5)
        {
            ani_id = ANI_BLOCKPAIN5;
        }
        else if(tempInt == 6)
        {
            ani_id = ANI_BLOCKPAIN6;
        }
        else if(tempInt == 7)
        {
            ani_id = ANI_BLOCKPAIN7;
        }
        else if(tempInt == 8)
        {
            ani_id = ANI_BLOCKPAIN8;
        }
        else if(tempInt == 9)
        {
            ani_id = ANI_BLOCKPAIN9;
        }
        else if(tempInt == 10)
        {
            ani_id = ANI_BLOCKPAIN10;
        }
        else
        {
            if(tempInt < MAX_ATKS - STA_ATKS + 1)
            {
                tempInt = MAX_ATKS - STA_ATKS + 1;
            }
            ani_id = animblkpains[tempInt + STA_ATKS - 1];
        }
    }
    else if(stricmp(value, "backblockpainb") == 0)
    {
        ani_id = ANI_BACKBLOCKPAINB;
    }
    else if(stricmp(value, "backblockpains") == 0)
    {
        ani_id = ANI_BACKBLOCKPAINS;
    }
    else if(starts_with_num(value, "backblockpain"))
    {
        get_tail_number(tempInt, value, "backblockpain");
        if(tempInt == 1)
        {
            ani_id = ANI_BACKBLOCKPAIN;
        }
        else if(tempInt == 2)
        {
            ani_id = ANI_BACKBLOCKPAIN2;
        }
        else if(tempInt == 3)
        {
            ani_id = ANI_BACKBLOCKPAIN3;
        }
        else if(tempInt == 4)
        {
            ani_id = ANI_BACKBLOCKPAIN4;
        }
        else if(tempInt == 5)
        {
            ani_id = ANI_BACKBLOCKPAIN5;
        }
        else if(tempInt == 6)
        {
            ani_id = ANI_BACKBLOCKPAIN6;
        }
        else if(tempInt == 7)
        {
            ani_id = ANI_BACKBLOCKPAIN7;
        }
        else if(tempInt == 8)
        {
            ani_id = ANI_BACKBLOCKPAIN8;
        }
        else if(tempInt == 9)
        {
            ani_id = ANI_BACKBLOCKPAIN9;
        }
        else if(tempInt == 10)
        {
            ani_id = ANI_BACKBLOCKPAIN10;
        }
        else
        {
            if(tempInt < MAX_ATKS - STA_ATKS + 1)
            {
                tempInt = MAX_ATKS - STA_ATKS + 1;
            }
            ani_id = animbackblkpains[tempInt + STA_ATKS - 1];
        }
    }
    else if(stricmp(value, "duckattack") == 0)
    {
        ani_id = ANI_DUCKATTACK;
    }
    else if(stricmp(value, "walkoff") == 0)
    {
        ani_id = ANI_WALKOFF;
    }
    else if(stricmp(value, "edge") == 0)
    {
        ani_id = ANI_EDGE;
    }
    else if(stricmp(value, "backedge") == 0)
    {
        ani_id = ANI_BACKEDGE;
    }
    else if(stricmp(value, "ducking") == 0)
    {
        ani_id = ANI_DUCKING;
    }
    else if(stricmp(value, "duckrise") == 0)
    {
        ani_id = ANI_DUCKRISE;
    }
    else if(stricmp(value, "victory") == 0)
    {
        ani_id = ANI_VICTORY;
    }
    else if(stricmp(value, "lose") == 0)
    {
        ani_id = ANI_LOSE;
    }

    return ani_id;

}

void lcmHandleCommandName(ArgList *arglist, s_model *newchar, int cacheindex)
{
    char *value = GET_ARGP(1);
    s_model *tempmodel;
    if((tempmodel = findmodel(value)) && tempmodel != newchar)
    {
        borShutdown(1, "Duplicate model name '%s'", value);
    }
    /*if((tempmodel=find_model(value))) {
    	return tempmodel;
    }*/
    model_cache[cacheindex].model = newchar;
    newchar->name = model_cache[cacheindex].name;
    if(stricmp(newchar->name, "steam") == 0)
    {
        newchar->alpha = BLEND_MODE_ALPHA;
    }
}

void lcmHandleCommandType(ArgList *arglist, s_model *newchar, char *filename)
{
    char *value = GET_ARGP(1);
    int i;
    if(stricmp(value, "none") == 0)
    {
        newchar->type = TYPE_NONE;
        newchar->move_config_flags        |= (MOVE_CONFIG_NO_ADJUST_BASE | MOVE_CONFIG_SUBJECT_TO_GRAVITY);
        newchar->move_config_flags        &= ~(MOVE_CONFIG_SUBJECT_TO_BASEMAP | MOVE_CONFIG_SUBJECT_TO_HOLE | MOVE_CONFIG_SUBJECT_TO_MAX_Z | MOVE_CONFIG_SUBJECT_TO_MIN_Z | MOVE_CONFIG_SUBJECT_TO_OBSTACLE | MOVE_CONFIG_SUBJECT_TO_PLATFORM | MOVE_CONFIG_SUBJECT_TO_SCREEN | MOVE_CONFIG_SUBJECT_TO_WALL);
    }
    else if(stricmp(value, "player") == 0)
    {
        newchar->type = TYPE_PLAYER;
        newchar->block_config_flags |= BLOCK_CONFIG_ACTIVE;

        for(i = 0; i < MAX_ATCHAIN; i++)
        {
            if(i < 2 || i > 3)
            {
                newchar->atchain[i] = 1;
            }
            else
            {
                newchar->atchain[i] = i;
            }
        }

        newchar->chainlength            = 4;
        newchar->bounce                 = 1;
        newchar->move_config_flags        |= (MOVE_CONFIG_SUBJECT_TO_BASEMAP | MOVE_CONFIG_SUBJECT_TO_GRAVITY | MOVE_CONFIG_SUBJECT_TO_HOLE | MOVE_CONFIG_SUBJECT_TO_MAX_Z | MOVE_CONFIG_SUBJECT_TO_MIN_Z | MOVE_CONFIG_SUBJECT_TO_OBSTACLE | MOVE_CONFIG_SUBJECT_TO_PLATFORM | MOVE_CONFIG_SUBJECT_TO_SCREEN | MOVE_CONFIG_SUBJECT_TO_WALL);
        newchar->move_config_flags         &= ~MOVE_CONFIG_NO_ADJUST_BASE;
    }
    else if(stricmp(value, "enemy") == 0)
    {
        newchar->type                   = TYPE_ENEMY;
        newchar->bounce                 = 1;
        newchar->move_config_flags        |= (MOVE_CONFIG_SUBJECT_TO_BASEMAP | MOVE_CONFIG_SUBJECT_TO_GRAVITY | MOVE_CONFIG_SUBJECT_TO_HOLE | MOVE_CONFIG_SUBJECT_TO_MAX_Z | MOVE_CONFIG_SUBJECT_TO_MIN_Z | MOVE_CONFIG_SUBJECT_TO_OBSTACLE | MOVE_CONFIG_SUBJECT_TO_PLATFORM | MOVE_CONFIG_SUBJECT_TO_WALL);
        newchar->move_config_flags &= ~MOVE_CONFIG_NO_ADJUST_BASE;
    }
    else if(stricmp(value, "item") == 0)
    {
        newchar->type                   = TYPE_ITEM;
        newchar->move_config_flags        |= (MOVE_CONFIG_SUBJECT_TO_BASEMAP | MOVE_CONFIG_SUBJECT_TO_GRAVITY | MOVE_CONFIG_SUBJECT_TO_HOLE | MOVE_CONFIG_SUBJECT_TO_MAX_Z | MOVE_CONFIG_SUBJECT_TO_MIN_Z | MOVE_CONFIG_SUBJECT_TO_OBSTACLE | MOVE_CONFIG_SUBJECT_TO_PLATFORM | MOVE_CONFIG_SUBJECT_TO_WALL);
        newchar->move_config_flags &= ~MOVE_CONFIG_NO_ADJUST_BASE;

    }
    else if(stricmp(value, "obstacle") == 0)
    {
        newchar->type                   = TYPE_OBSTACLE;
        if(newchar->aimove == AIMOVE1_NONE)
        {
            newchar->aimove = AIMOVE1_NORMAL;
        }
        newchar->aimove |= AIMOVE1_NOMOVE;
        if(newchar->aimove == AIMOVE1_NONE)
        {
            newchar->aiattack = 0;
        }
        newchar->aimove |= AIATTACK1_NOATTACK;
        newchar->move_config_flags |= (MOVE_CONFIG_SUBJECT_TO_BASEMAP | MOVE_CONFIG_SUBJECT_TO_GRAVITY | MOVE_CONFIG_SUBJECT_TO_HOLE | MOVE_CONFIG_SUBJECT_TO_MAX_Z | MOVE_CONFIG_SUBJECT_TO_MIN_Z | MOVE_CONFIG_SUBJECT_TO_PLATFORM | MOVE_CONFIG_SUBJECT_TO_WALL);
        newchar->move_config_flags &= ~MOVE_CONFIG_NO_ADJUST_BASE;
    }
    else if(stricmp(value, "steamer") == 0)
    {
        newchar->offscreenkill = 80;
        newchar->type = TYPE_STEAMER;
    }
	else if(stricmp(value, "projectile") == 0)
	{
		newchar->type |= TYPE_PROJECTILE;

        if (newchar->aimove == AIMOVE1_NONE)
        {
            newchar->aimove = AIMOVE1_NORMAL;
        }

        //newchar->aimove |= AIMOVE1_NORMAL;
        		
		if (!newchar->offscreenkill)
		{			
			newchar->offscreenkill = (int)(videomodes.hRes * 0.5);
		}

		// Note when using as a projectile, these may
		// be modified. See knife_spawn and bomb_spawn.

		newchar->move_config_flags |= (MOVE_CONFIG_SUBJECT_TO_BASEMAP | MOVE_CONFIG_PROJECTILE_WALL_BOUNCE | MOVE_CONFIG_PROJECTILE_BASE_DIE | MOVE_CONFIG_SUBJECT_TO_GRAVITY | MOVE_CONFIG_SUBJECT_TO_HOLE | MOVE_CONFIG_SUBJECT_TO_MAX_Z | MOVE_CONFIG_SUBJECT_TO_MIN_Z | MOVE_CONFIG_SUBJECT_TO_PLATFORM | MOVE_CONFIG_SUBJECT_TO_WALL);
		newchar->move_config_flags &= ~(MOVE_CONFIG_SUBJECT_TO_SCREEN);
	}
    // my new types   7-1-2005
    else if(stricmp(value, "pshot") == 0)
    {
        newchar->type = TYPE_SHOT;
        if(newchar->aimove == AIMOVE1_NONE)
        {
            newchar->aimove = AIMOVE1_NORMAL;
        }
        newchar->aimove |= AIMOVE1_ARROW;
        if(!newchar->offscreenkill)
        {
            newchar->offscreenkill = 200;
        }

		// Note when using as a projectile, most of these
		// are modified. See knife_spawn and bomb_spawn.
        newchar->move_config_flags |= (MOVE_CONFIG_NO_ADJUST_BASE | MOVE_CONFIG_SUBJECT_TO_GRAVITY | MOVE_CONFIG_SUBJECT_TO_MAX_Z | MOVE_CONFIG_SUBJECT_TO_MIN_Z | MOVE_CONFIG_PROJECTILE_WALL_BOUNCE);
        newchar->move_config_flags &= ~(MOVE_CONFIG_SUBJECT_TO_BASEMAP | MOVE_CONFIG_SUBJECT_TO_HOLE | MOVE_CONFIG_SUBJECT_TO_PLATFORM | MOVE_CONFIG_SUBJECT_TO_SCREEN | MOVE_CONFIG_SUBJECT_TO_WALL);
    }
    else if(stricmp(value, "trap") == 0)
    {
        newchar->type                   = TYPE_TRAP;
        newchar->move_config_flags |= (MOVE_CONFIG_SUBJECT_TO_BASEMAP | MOVE_CONFIG_SUBJECT_TO_GRAVITY | MOVE_CONFIG_SUBJECT_TO_HOLE | MOVE_CONFIG_SUBJECT_TO_MAX_Z | MOVE_CONFIG_SUBJECT_TO_MIN_Z | MOVE_CONFIG_SUBJECT_TO_PLATFORM | MOVE_CONFIG_SUBJECT_TO_WALL);
        newchar->move_config_flags &= ~MOVE_CONFIG_NO_ADJUST_BASE;
    }
    else if(stricmp(value, "text") == 0)   // Used for displaying text/images and freezing the screen
    {
        newchar->type                   = TYPE_TEXTBOX;
        newchar->move_config_flags        &= ~MOVE_CONFIG_SUBJECT_TO_GRAVITY;
        newchar->move_config_flags        |= (MOVE_CONFIG_SUBJECT_TO_MAX_Z | MOVE_CONFIG_SUBJECT_TO_MIN_Z);
    }
    else if(stricmp(value, "endlevel") == 0)   // Used for ending the level when the players reach a certain point
    {
        newchar->type               = TYPE_ENDLEVEL;
        newchar->move_config_flags    |= (MOVE_CONFIG_SUBJECT_TO_BASEMAP | MOVE_CONFIG_SUBJECT_TO_GRAVITY | MOVE_CONFIG_SUBJECT_TO_HOLE | MOVE_CONFIG_SUBJECT_TO_OBSTACLE | MOVE_CONFIG_SUBJECT_TO_PLATFORM | MOVE_CONFIG_SUBJECT_TO_WALL);
    }
    else if(stricmp(value, "npc") == 0)   // NPC type
    {
        newchar->type                   = TYPE_NPC;
        newchar->bounce                 = 1;
        newchar->move_config_flags |= (MOVE_CONFIG_SUBJECT_TO_BASEMAP | MOVE_CONFIG_SUBJECT_TO_GRAVITY | MOVE_CONFIG_SUBJECT_TO_HOLE | MOVE_CONFIG_SUBJECT_TO_MAX_Z | MOVE_CONFIG_SUBJECT_TO_MIN_Z | MOVE_CONFIG_SUBJECT_TO_OBSTACLE | MOVE_CONFIG_SUBJECT_TO_PLATFORM | MOVE_CONFIG_SUBJECT_TO_WALL);
        newchar->move_config_flags &= ~MOVE_CONFIG_NO_AЧnчзПКЧ¬ўh­µзHB‚€YЉЩ[‹OЭ\ЭЫWЭ\™Щ]OH•S\Щ[‹OЭ\ЭЫWЭ\™Щ]O™^\ЭИ
H\™Щ]H›Ь›X[Щљ[™Э\™Щ]
LK
NВ€[ЩH\™Щ]HЩ[‹OЭ\ЭЫWЭ\™Щ]В‚€YЉ\™Щ]	‰€JXЭ[Ы—ШЪXЪЧШШ[—Щ[XYЩJЩ[‹\™Щ][™\™XЭ
JJB€В€™]\›€В€B‚€Y€

Щ[‹O™XЪЪ[™И	€PТЧРPХU‘JH	‰€Щ[‹O[љ[[ќ[HOHS’WСPТКB€В€Y€
]\™Щ][Z\Љ\™Щ]
HJ\™Щ]O™XЪЪ[™И	€PТЧРPХU‘JH€
XЪXЪЧЬ[™ЩWЭ\™Щ]Ш[
Щ[‹\™Щ]S’WСPТЛ
H	‰‚€Y™ЉЩ[‹OњЬЪ][Ы‹ћ\™Щ]OњЬЪ][Ы‹ћ
H€Ь[™Щ^	‰€Y™ЉЩ[‹OњЬЪ][Ы‹ћ‹\™Щ]OњЬЪ][Ы‹ћЉH€Ь[™Щ^ЉH
B€В€Щ[‹Oќ™[ШЪ]KћHЩ[‹Oќ™[ШЪ]Kћ€HВ€ћYXЪЬљ\ЩJЩ[ЉNВ€™]\›€NВ€B€B‚€YЉ]\™Щ]J\™Щ]O™XЪЪ[™И	€PТЧРPХU‘JJB€В€™]\›€В€B€[ЩB€В€Y€
Щ[‹O™XЪЪ[™И	€PТЧРPХU‘JB€В€™]\›€NВ€B€[ЩB€В€[ќ[™ЩWЩ›YИHЪXЪЧЬ[™ЩWЭ\™Щ]Ш[
Щ[‹\™Щ]S’WСPТЛ
NВ€Y€
\[™ЩWЩ›YКB€В€Y€
Y™ЉЩ[‹OњЬЪ][Ы‹ћ\™Щ]OњЬЪ][Ы‹ћ
HHЬ[™Щ^	‰‚€Y™ЉЩ[‹OњЬЪ][Ы‹ћ‹\™Щ]OњЬЪ][Ы‹ћЉHHЬ[™Щ^ЉH[™ЩWЩ›YИHNВ€B€Y€
\[™ЩWЩ›YКH™]\›€В‚€YЉЩ[‹O™XЪЪ[™ИOHPТЧУ“У‘JB€В€ћYXЪКЩ[ЉNВ€B€™]\›€NВ€B€B‚€™]\›€ВџB‚‚‹ЛИK’H›ЫЭќ›ЪYЫЫ[[Ы—Э[љК
BћВ‚€YЉЩ[‹O™X]ЬЭ]H	€PUФХUWСPQ
B€В€™]\›ЋВ€H‚€ЛЪYЉЪXЪЬ[›™Y

JH™]\›ЋВ‚€ЛИЫИ\€]Ш^HИHШ\њ€YЉZWШЪXЪЧЬ™XШ[

JB€В€™]\›ЋВ€B‚€ЛИљ\ЩOИћHљ\ЩH]XЪВ€YЉZWШЪXЪЧЫYJ
JB€В€™]\›ЋВ€B‚€ЛИ\ШШ\OВ€YЉZWШЪXЪЧЩЬX™Y

JB€В€™]\›ЋВ€B‚€ЛЩЬXљ[™ИЫЫY][™В€YЉZWШЪXЪЧЩЬXЉ
JB€В€™]\›ЋВ€B‚€ЛИ[™[ZY\ИШ[€›ЭИ\ШШ\H›Ы‹ZЫ›ШЪЩЭЫ€Ь[[XYЩH
Ъ]HЩZ\™\ЩJHB€YЉZWШЪXЪЧЩ\ШШ\J
JB€В€™]\›ЋВ€B‚€ЛИќ\ЮHљYЪ›ЭПВ€YЉZWШЪXЪЧШќ\ЮJ
JB€В€™]\›ЋВ€B‚€ЛИYKЫИћHИ]XЪИЬ€ќYЩH™^[Э™B€ЛИЫќ[Э™HY€[[ќИHЫHЬ€Щ™€HШ[€YЉЫЫ[[Ы—Ш]XЪК
JB€В€™]\›ЋВ€B‚€ЛИ\™Щ]\ИXЪЪ[™ПИћHИXЪЪ[™Л‹‚€YЉZWШЪXЪЧЩXЪЪ[™К
JB€В€™]\›ЋВ€B‚€ЫЫ[[Ы—Ы[Э™J
NВџB‚‹ЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛВ‚ќ›ЪYЭZXЪYJ
BћВ€€YЉЭ[YHЩ[‹OњЭ[[YJB€В€™]\›ЋВ€B€]™[ШЫЫ\]YHЩ[‹O›ЬЬОВ€]™[ШЫЫ\]YЩY™X][™ЧШ›ЬЬИHЩ[‹O›ЬЬОВ‚€Ъ[Щ[ќ]JЩ[‹ТSСS•UWХ’QССT—ФХRPТQJNВџB‚‚‚‹ЛИ™KY[ќ\€^YљY[‹ЛИ\ЩYћH^Y\—Щ[[™^Y\—ЭZЩY[XYЩBќ›ЪY^Y\—ЩYWЩ[ќ]J[ќ]J€XЭ[™ЧЩ[ќ]JBћВ€[ќ^Y\љ[™^HXЭ[™ЧЩ[ќ]KOњ^Y\љ[™^В€[ќHHВ‚€YЉJЫШ[ШЫЫ™љYЛЪX]И	€ТPUУФSУ”ЧУU‘TЧРPХU‘JJB€В€K\^Y\–Ь^Y\љ[™^K›]™\ОВ€B‚€YЉљ\њЭ^Y\€OHXЭ[™ЧЩ[ќ]JB€В€љ\њЭ^Y\€H•SВ€B‚€^XЭ]WЬYWЬШЬљ\
^Y\љ[™^
NВ‚€YЉ›ЫX^ќ\Ъ™\Щ]НHЏHJB€В€›ЫX^ќ\Ъ™\Щ]Ь^Y\љ[™^HH^Y\–Ь^Y\љ[™^K™[ќOњќ\Ъ›X^В€B€^Y\–Ь^Y\љ[™^K™[ќH•SВ€^Y\–Ь^Y\љ[™^KњЬ]ЫљX[HXЭ[™ЧЩ[ќ]KO›[Щ[]KљX[В€^Y\–Ь^Y\љ[™^KњЬ]Ы›\HXЭ[™ЧЩ[ќ]KO›[Щ[]K›\В‚€К€€
€[™HH›ЩK€Y€[ћHЫЬњЩH›YЬВ€
€\™HЩ]ЩHX]™H[ќ]HЫ€ШЬ™Y[€€
€[™XZЩH][™\ќ€Э\ќЪ\ЩHЩHШ[‚€
€ќ\Эќ[€Ъ[ќ[Э[Ы€И™[[Э™K‚€
€€
€‘SSХ‘WРУФ”СWК€›YЬИ™Y™\€ИЭИ€
€H]™H[ќ]H\И™[[Э™Yњ›ЫHH€
€Ш[YHЪ[HX]љ[™ИHЫЬњЩH™Z[™‚€
‹В‚€Э]XИЫЫњЭWЩX]ШЫЫ™љYЧЩ›YЬИX]™WШЫЬњЩHHPUРУУ‘’QЧФ‘SSХ‘WРУФ”СWРRT€PUРУУ‘’QЧФ‘SSХ‘WРУФ”СWСФ“ХS‘В‚€Y€
XЭ[™ЧЩ[ќ]KO›[Щ[]K™X]ШЫЫ™љYЧЩ›YЬИ	€X]™WШЫЬњЩJHВ€XЭ[™ЧЩ[ќ]KOќ[љИH•SВ€XЭ[™ЧЩ[ќ]KOќZЩXXЭ[Ы€H•SВ€XЭ[™ЧЩ[ќ]KO™X]ЬЭ]HHPUФХUWРУФ”СNВ€H[ЩHВ€Ъ[Щ[ќ]JXЭ[™ЧЩ[ќ]KТSСS•UWХ’QССT—ФVQT—СPU
NВ€B‚€YЉ^Y\–Ь^Y\љ[™^K›]™\ИH
B€В€[ќ[ЬЪЫИHВ‚‚BKЛИЫЭ[ќќ[X™\€Щ€УЙЩ
XY
H^Y\њЛћHЫЬ[™И^Y\‚‚BKЛИ[™^\И[™[Ь™[Y[ќ[™ИЪ[€^Y\€Щ\И›Э]™B‚BKЛИ[€[ќ]K‚€›ЬЉHHИHPVФVQT”ОИJККB€В‚BBZY€
\^Y\–ЪWK™[ќ
B‚BB^В‚BBBJКШ[ЬЪЫОВ‚BB_B€B€‚BKЛИY€[^Y\њИ\™HУЙЩ[€УИЫЭ[ќHK‚‚BKЛИЭ\ќЪ\ЩKЩ]]И‚‚BX[ЬЪЫИH
[ЬЪЫИЏHPVФVQT”КHИH€В‚‚BKЛИ[^Y\њИУЙЩВ€YЉ[ЬЪЫКB€В‚BBZ[ќ[ЬЫ›Ъ›Ъ[€HВ‚BBZ[ќ[ЬЫ›ШЬ™Y]ИHВ‚‚BBKЛИ[^Y\њИ“Х›Ъ[љ[™ПВ‚BBKЛИШ[YHЩЪXИ\И[^Y\€УЛ€€›ЬЉHHИHPVФVQT”ОИJККB€В‚BBBZY€
\^Y\–ЪWKљ›Ъ[љ[™КB‚BBB^В‚BBBBJКШ[ЬЫ›Ъ›Ъ[ЋВ‚BBB_B€B‚€[ЬЫ›Ъ›Ъ[€H
[ЬЫ›Ъ›Ъ[€ЏHPVФVQT”КHИH€В‚‚BBKЛИ[^Y\њИЭ]Щ€Ь™Y]ПВ‚BBKЛИШ[YHЩЪXИ\И[^Y\€УЛ‚€›ЬЉHHИHPVФVQT”ОИJККB€В‚BBBZY€
^Y\–ЪWKЬ™Y]ИJB‚BBB^В‚BBBBJКШ[ЬЫ›ШЬ™Y]ОВ‚BBB_B€BBBB‚€[ЬЫ›ШЬ™Y]ИH
[ЬЫ›ШЬ™Y]ИЏHPVФVQT”КHИH€В‚‚BBKЛИЩ]H[Y\€ИHLЩXЫЫ™ЫЭ[ќЭЫ‹‚€[Y[YќHL
€ЫШ[ШЫЫ™љYЛЫЭ[ќ\—ЬЬYYВ‚‚BBKЛИ›ИЫ™H›Ъ[љ[™И[ЏВ€YЉ[ЬЫ›Ъ›Ъ[ЉB€В‚BBBKЛИY€H^Y\€Ш[‰ЭЫЫќ[ќYK]	ЬИЩ]H[YHЭ™\‚‚BBBKЛИИ[™[[ЬЭ[њЭ[ќHЫИ^HЫЫ‰Э]™HИШZ]‚‚‚BBBKЛИY€›ЬЪ\™H\И[X›YЬ™Y]Ъ\™\И\™H›Э[ЭЩY€™\љYћH[‚BBBKЛИ^Y\€[™]љYX[Ь™Y]Э\Y\И\™H[\K€Э\ќЪ\ЩHЬ™Y]‚BBBKЛИЪ\™\И\™H[ЭЩYЫИ™\љYћHЫЫЩ€Ь™Y]И\И[\K‚‚BBBZY€
›ЬЪ\™JB‚BBB^В‚BBBBZY€
[ЬЫ›ШЬ™Y]КB‚BBBB^В‚BBBBB][Y[YќHЫШ[ШЫЫ™љYЛЫЭ[ќ\—ЬЬYYИЋВ‚BBBB_BBBBBB‚BBB_B‚BBBY[ЩB‚BBB^В‚BBBBZY€
Ь™Y]ИJB‚BBBB^В‚BBBBB][Y[YќHЫШ[ШЫЫ™љYЛЫЭ[ќ\—ЬЬYYИЋВ‚BBBB_B‚BBB_B€B€B‚€YЉXЭ[™ЧЩ[ќ]KO›[Щ[]KќЩX\Ы—Ь›Ь\ќY\Л›ЬЬЧШЫЫ™][Ы€	€СPTУ—УФФЧРУУ‘USУ—ФХQСJB€В€^Y\–Ь^Y\љ[™^KќЩX\ќ[HH]™[OњЩ]ЩX\В€B‚€YЉ›ЫX^ќ\Ъ™\Щ]НHOHЉB€В€›ЫX^ќ\Ъ™\Щ]Ь^Y\љ[™^HHВ€B€‚B\™]\›ЋВ€B€[ЩB€В€Ь]Ыњ^Y\Љ^Y\љ[™^
NВ€^XЭ]WЬ™\Ь]Ы—ЬШЬљ\
^Y\љ[™^
NВ€YЉ[›Щ›Ь[ЉB€В€Y€
Ш]™Y]Kљ›Ю\ќ[X›VЬ^Y\љ[™^JHЫЫќ›ЫЬќ[X›J^Y\љ[™^KLЌJNВ€›ЬШ[Щ[™[ZY\К
NВ€B€B‚€YЉ[]™[O››Ь™\Щ]
B€В€[Y[YќH]™[OњЩ][YH
€ЫШ[ШЫЫ™љYЛЫЭ[ќ\—ЬЬYYИЛИ™X€ЌЊHH\И[™H[Э™Y\™HИЩ]Э\ЭЫH[YB€B‚џB‚ќ›ЪY^Y\—ЩYJ›ЪY
BћВ€^Y\—ЩYWЩ[ќ]JЩ[ЉNВџB‚‚‚љ[ќ^Y\—Эћ[[Э™J›Ш]\‹›Ш]™\ЉBћВ€™]\›€ЫЫ[[Ы—Эћ[[Э™J\‹™\ЉNВџB‚љ[ќЪXЪЧЩ[™\™ЮJWШЫЬЭШЪXЪИЪXЪ[ќ[љJBћВ€[ќ™\Э[HђSСNВ€WЩ[ќ]WЭ\H\NИЛС[ќ]H\K‚€ЧЩ[™\™ЮWШЫЬЭ[™\™ЮWШЫЬЭВ‚H€ЛИЩ][љ[X][Ы‰ЬИ[™\™ЮWШЫЬЭ]K‚€[™\™ЮWШЫЬЭЫЬЭHЩ[‹O›[Щ[]K[љ[X][Ы–Ш[љWKO™[™\™ЮWШЫЬЭЫЬЭВ‚Y[™\™ЮWШЫЬЭ™\ШX›HHЩ[‹O›[Щ[]K[љ[X][Ы–Ш[љWKO™[™\™ЮWШЫЬЭ™\ШX›NВ‚Y[™\™ЮWШЫЬЭ›\Ы›HHЩ[‹O›[Щ[]K[љ[X][Ы–Ш[љWKO™[™\™ЮWШЫЬЭ›\Ы›NВ‚€ЛИЩ][ќ]H\K‚€\BHHЩ[‹O›[Щ[]Kќ\NВ‚€ЛИY€ЩIЬ™Hљ[™[™ЬXЪX[\ИЭ™\њљY[‹[‚€ЛИ™]\›€[ЩK‚€YЉ\H	€
TWСS‘SVHTWУ”КJB€В€YЉЪXЪЧШљ[™ЫЭ™\њљYJЩ[‹’S‘РУУ‘’QЧУХ‘T”’QWФФPТPSРRJJB€В€™]\›€ђSСNВ€B€B€[ЩHYЉ\H	€TWФVQTЉB€В€YЉЪXЪЧШљ[™ЫЭ™\њљYJЩ[‹’S‘РУУ‘’QЧУХ‘T”’QWФФPТPSФVQTЉJB€В€™]\›€ђSСNВ€B€B‚€YЉ[Y[љ[JЩ[‹[љJJB€В€ЛИШ\ЪЩ^K[[Ы€‹‚€ЛИЊLLKL€ЛВ€ЛИ]\И›ЭИЬЬЪX›HИ[™]љYX[H\ШX›HЬXЪX[Л€[‚€ЛИX[ћHШ\Щ\И
ЩX\ЫњИ[€\ќXЭ[\ЉH\ИШ[‚Z[Э]ЭЫ€H™YY›Ь‚€ЛИЭ\\™›[Э\И[Щ[ИЪ[€Y™™\љ[™ИXљ[]Y\И\™H\Ъ\™Y›Ь€^Y\њЛ€ЛИ[™[ZY\ЛЬ€њЬЛ‚€ЛИЬ]\И
LLЊЊJHљ^YH™]Ињ›ЪЩ[€ЫЩH›Ь€™\ШX›H€›YИЪXЪЛXЪИИH™]љ[Э\ИЫЩB€YЉJ[™\™ЮWШЫЬЭ™\ШX›HOH\BBBBBBBBBBBBBKЛИ\ШX›YћH\OВ€
[™\™ЮWШЫЬЭ™\ШX›HOHLJBBBBBBBBBBBHЛИ\ШX›Y›Ь€[В€
[™\™ЮWШЫЬЭ™\ШX›HOHL€	‰€
\H	€
TWСS‘SVHTWУ”КJJBBHЛИ\ШX›Y›Ь€[ROВ€
[™\™ЮWШЫЬЭ™\ШX›HOHLИ	‰€
\H	€
TWФVQT€TWУ”КJJBHЛИ\ШX›Y›Ь€^Y\њИ	€”ЬПВ€
[™\™ЮWШЫЬЭ™\ШX›HOHM	‰€
\H	€
TWФVQT€TWСS‘SVJJJJJHЛИ\ШX›Y›Ь€[ROВ€В€ЛИ›ИЩX[Ь€ЩX[\И\ЬЛЬШ[YH\И[™\™ЮHЫЬЭВ€Y€
\Щ[‹OњЩX[Щ[‹OњЩX[ЏH[™\™ЮWШЫЬЭЫЬЭ
B€В€YЉ
ЪXЪOHS‘T‘ЦWХTWУT	‰€
[™\™ЮWШЫЬЭ›\Ы›HOHУФХХTWТУУ“JH	‰€
Щ[‹O™[™\™ЮWЬЭ]K›\ШЭ\њ™[ќЏH[™\™ЮWШЫЬЭЫЬЭ
JH‚BBBB_
ЪXЪOHS‘T‘ЦWХTWТ	‰€
[™\™ЮWШЫЬЭ›\Ы›HOHУФХХTWУTУУ“JH	‰€
Щ[‹O™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќ€[™\™ЮWШЫЬЭЫЬЭ
JJB€В€™\Э[H•QNВ€B€[ЩB€В€ЛСИЊKLKLЊВ€ЛХљYY][™ИHРS•[љ[X][Ы€\™HИЩY\ЫЩHЫЫ\XЭYќ]ЫЫ‰ЭЫЬљЛ€IЫЫЫYHXЪИИ\Л‚€ЛЪY€
[Y[љ[JЩ[‹[љJJ^В€ЛИ[ќЬЩ]Ш[љ[JЩ[‹S’WРРS•
NВ€ЛИЩ[‹OќZЩXXЭ[Ы€HЫЫ[[Ы—Ш]XЪЧЬ›ШОВ€ЛИ^Y\–ЬЩ[‹Oњ^Y\љ[™^Kњ^ZЩ^\ИHВ€ЛЯB€B€B€B€B‚€™]\›€™\Э[ВџB‚‹ЛИШ\ЪЩ^H[[Ы€‹‚‹ЛИЊNLKLL‹ЛВ‹ЛИ™\XЩ\И[њ™XYX›HЪXЪЧЬ[™ЩJ
HXXЬ›Л€ќ[њИ[™]љYX[‹ЛИЪXЪИ[™ЩHќ[Э[ЫњИ›Ь€XXЪ^\И[™™]\›њИќYB‹ЛИY€\™Щ]\ИЪ][€[™ЩHЩ€S‚›ЫЫЪXЪЧЬ[™ЩWЭ\™Щ]Ш[
ЫЫњЭ[ќ]H
™[ќЫЫњЭ[ќ]H
ќ\™Щ]ЫЫњЭ[љ[X][Ы—ЪYЭ[љ[X][Ы—ЪY[ќЌЭ[™ЩWЫZ[‹[ќЌЭ[™ЩWЫX^
BћВ€ЛИ]\Э]™HH[Y\™Щ][ќ]K‚€YЉ]\™Щ]
B€В€™]\›€[ЩNВ€B‚€ЛИЩ]Ъ[ќ\€И[љ[X][Ы‹‚€ЫЫњЭЧШ[љ[H
[љ[X][Ы€H[ќO›[Щ[]K[љ[X][Ы–Ш[љ[X][Ы—ЪYNВ‚€Y€
XЪXЪЧЬ[™ЩWЭ\™Щ]Ю
[ќ\™Щ][љ[X][Ы‹[™ЩWЫZ[‹[™ЩWЫX^
JB€В€™]\›€[ЩNВ€B‚€Y€
XЪXЪЧЬ[™ЩWЭ\™Щ]ЮJ[ќ\™Щ][љ[X][Ы‹[™ЩWЫZ[‹[™ЩWЫX^
JB€В€™]\›€[ЩNВ€B‚€Y€
XЪXЪЧЬ[™ЩWЭ\™Щ]ЮЉ[ќ\™Щ][љ[X][Ы‹[™ЩWЫZ[‹[™ЩWЫX^
JB€В€™]\›€[ЩNВ€B‚€Y€
XЪXЪЧЬ[™ЩWЭ\™Щ]Ш\ЩJ[ќ\™Щ][љ[X][Ы‹[™ЩWЫZ[‹[™ЩWЫX^
JB€В€™]\›€[ЩNВ€B‚€™]\›€ќYNВџB‚‹ЛИШ\ЪЩ^K[[Ы€‹‚‹ЛИЊNLKLL‚‹ЛВ‹ЛИ™]\›€ќYHY€\™Щ]\ИЪ][€\ЩH[™ЩB‹ЛИЩ€[ќ]IЬИ[љ[X][Ы‹‚›ЫЫЪXЪЧЬ[™ЩWЭ\™Щ]Ш\ЩJЫЫњЭ[ќ]H
XЭ[™ЧЩ[ќ]KЫЫњЭ[ќ]H
ќ\™Щ]ЫЫњЭЧШ[љ[H
[љ[X][Ы‹[ќЌЭ[™ЩWЫZ[‹[ќЌЭ[™ЩWЫX^
BћВ€[ќ[ќШ\ЩNВ€[ќ\™Щ]Ш\ЩNВ‚€ЧЫY]љXЧЬ[™ЩH[™ЩNВ‚€ЛИ]\Э]™HH\™Щ]‚€YЉXXЭ[™ЧЩ[ќ]H]\™Щ]
B€В€™]\›€[ЩNВ€B‚€[™ЩK›X^H[™ЩWЫX^В€[™ЩK›Z[€H[™ЩWЫZ[ЋВ‚€Y€
[љ[X][ЫЉB€В€[™ЩK›X^
ПH[љ[X][Ы‹Oњ[™ЩK\ЩK›X^В€[™ЩK›Z[€
ПH[љ[X][Ы‹Oњ[™ЩK\ЩK›Z[ЋВ€B‚€ЛИЩ]ЬЪ][ЫњИШ\Э\И[ќYЩ\њЛ‚€[ќШ\ЩHH
[ќ
XXЭ[™ЧЩ[ќ]KO\ЩNВ€\™Щ]Ш\ЩHH
[ќ
]\™Щ]O\ЩNВ‚€ЛИЭXќXЭ[ќ]H\ЩHЬЪ][Ы€њ›ЫH\™Щ]ЬЪ][Ы‹‚€\™Щ]Ш\ЩHOH[ќШ\ЩNВ‚€ЛИ™]\›€ќYHY€љ[[\™Щ]ШШ][Ы€\В€ЛИЪ][€[™ЩHZ[€[™X^‚€™]\›€
\™Щ]Ш\ЩHЏH[™ЩK›Z[‚€	‰€\™Щ]Ш\ЩHH[™ЩK›X^
HИќYH€[ЩNВџB‚‹ЛИШ\ЪЩ^K[[Ы€‹‚‹ЛИЊNLKLL‹ЛВ‹ЛИ™]\›€ќYHY€\™Щ]\ИЪ][€[™ЩB‹ЛИЩ€[ќ]IЬИ[љ[X][Ы‹‚›ЫЫЪXЪЧЬ[™ЩWЭ\™Щ]Ю
ЫЫњЭ[ќ]H
XЭ[™ЧЩ[ќ]KЫЫњЭ[ќ]H
ќ\™Щ]ЫЫњЭЧШ[љ[H
[љ[X][Ы‹[ќЌЭ[™ЩWЫZ[‹[ќЌЭ[™ЩWЫX^
BћВ€ЛИ]\Э]™H[ќ]Y\Л‚€YЉXXЭ[™ЧЩ[ќ]H]\™Щ]
B€В€™]\›€[ЩNВ€B‚€ЛИЩ]ЬЪ][ЫњИШ\Э\И[ќYЩ\њЛ‚€ЫЫњЭ[ќ[ќЮH
[ќ
XXЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћВ€ЫЫњЭ[ќ\™Щ]ЮH
[ќ
]\™Щ]OњЬЪ][Ы‹ћВ‚€ЧЫY]љXЧЬ[™ЩH[™ЩNВ‚€[™ЩK›X^H[™ЩWЫX^В€[™ЩK›Z[€H[™ЩWЫZ[ЋВ‚€Y€
[љ[X][ЫЉB€В€[™ЩK›X^
ПH[љ[X][Ы‹Oњ[™ЩKћ›X^В€[™ЩK›Z[€
ПH[љ[X][Ы‹Oњ[™ЩKћ›Z[ЋВ€B‚€ЛИ™]\›€ќYHY€љ[[\™Щ]ШШ][Ы€\В€ЛИЪ][€[™ЩHZ[€[™X^€[™ЩHЫЫ\\љ\ЫЫ‚€ЛИ\И™]™\њЩYЪ[€[ќ]HXЩ\ИYќ‚€YЉXЭ[™ЧЩ[ќ]KO™\™XЭ[Ы€OHT‘PХSУ—Ф’QТ
B€В€ЛИY[™ЩHИ[ќ]HЬЪ][Ы‚€ЛИ›Ь€љ[[[™ЩHЫЫЬ™[]\Л‚€[™ЩK›Z[€H[ќЮ
И[™ЩK›Z[ЋВ€[™ЩK›X^H[ќЮ
И[™ЩK›X^В‚€™]\›€
\™Щ]ЮЏH[™ЩK›Z[‚€	‰€\™Щ]ЮH[™ЩK›X^
HИќYH€[ЩNВ€B€[ЩB€В€ЛИЭXќXЭ[™ЩHњ›ЫH[ќ]H€ЛИЬЪ][Ы€›Ь€љ[[[™ЩHЫЫЬ™[]\Л‚€[™ЩK›Z[€H[ќЮH[™ЩK›Z[ЋВ€[™ЩK›X^H[ќЮH[™ЩK›X^В‚€™]\›€
\™Щ]ЮH[™ЩK›Z[‚€	‰€\™Щ]ЮЏH[™ЩK›X^
HИќYH€[ЩNВ€BџB‚‹ЛИШ\ЪЩ^K[[Ы€‹‚‹ЛИЊNLKLL‹ЛВ‹ЛИ™]\›€ќYHY€\™Щ]\ИЪ][€H[™ЩB‹ЛИЩ€[ќ]IЬИ[љ[X][Ы‹‚›ЫЫЪXЪЧЬ[™ЩWЭ\™Щ]ЮJЫЫњЭ[ќ]H
XЭ[™ЧЩ[ќ]KЫЫњЭ[ќ]H
ќ\™Щ]ЫЫњЭЧШ[љ[H
[љ[X][Ы‹[ќЌЭ[™ЩWЫZ[‹[ќЌЭ[™ЩWЫX^
BћВ€ЛИ]\Э]™HH\™Щ]‚€YЉXXЭ[™ЧЩ[ќ]H]\™Щ]
B€В€™]\›€[ЩNВ€B‚€ЧЫY]љXЧЬ[™ЩH[™ЩNВ‚€[™ЩK›X^H[™ЩWЫX^В€[™ЩK›Z[€H[™ЩWЫZ[ЋВ‚€Y€
[љ[X][ЫЉB€В€[™ЩK›X^
ПH[љ[X][Ы‹Oњ[™ЩKћK›X^В€[™ЩK›Z[€
ПH[љ[X][Ы‹Oњ[™ЩKћK›Z[ЋВ€B‚€ЛИЩ]ЬЪ][ЫњИШ\Э\И[ќYЩ\њЛ‚€ЫЫњЭ[ќ[ќЮHH
[ќ
XXЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћNВ€ЫЫњЭ[ќ\™Щ]ЮHH
[ќ
]\™Щ]OњЬЪ][Ы‹ћHH[ќЮNВ‚€ЛИ™]\›€ќYHY€љ[[\™Щ]ШШ][Ы€\В€ЛИЪ][€[™ЩHZ[€[™X^‚€™]\›€
\™Щ]ЮHЏH[™ЩK›Z[‚€	‰€\™Щ]ЮHH[™ЩK›X^
HИќYH€[ЩNВџB‚‹ЛИШ\ЪЩ^K[[Ы€‹‚‹ЛИЊNLKLL‚‹ЛВ‹ЛИ™]\›€ќYHY€\™Щ]\ИЪ][€€[™ЩB‹ЛИЩ€[ќ]IЬИ[љ[X][Ы‹‚›ЫЫЪXЪЧЬ[™ЩWЭ\™Щ]ЮЉЫЫњЭ[ќ]H
€XЭ[™ЧЩ[ќ]KЫЫњЭ[ќ]H
ќ\™Щ]ЫЫњЭЧШ[љ[H
[љ[X][Ы‹[ќЌЭ[™ЩWЫZ[‹[ќЌЭ[™ЩWЫX^
BћВ€ЛИ]\Э]™HH\™Щ]‚€YЉXXЭ[™ЧЩ[ќ]H]\™Щ]
B€В€™]\›€[ЩNВ€B‚€ЧЫY]љXЧЬ[™ЩH[™ЩNВ‚€[™ЩK›Z[€H[™ЩWЫZ[ЋВ€[™ЩK›X^H[™ЩWЫX^В‚€Y€
[љ[X][ЫЉB€В€[™ЩK›X^
ПH[љ[X][Ы‹Oњ[™ЩKћ‹›X^В€[™ЩK›Z[€
ПH[љ[X][Ы‹Oњ[™ЩKћ‹›Z[ЋВ€B‚€ЛИЩ]ЬЪ][ЫњИШ\Э\И[ќYЩ\њЛ‚€ЫЫњЭ[ќ[ќЮ€H
[ќ
XXЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћЋВ€ЫЫњЭ[ќ\™Щ]Ю€H
[ќ
]\™Щ]OњЬЪ][Ы‹ћ€H[ќЮЋВ‚€ЛИ™]\›€ќYHY€љ[[\™Щ]ШШ][Ы€\В€ЛИЪ][€[™ЩHZ[€[™X^‚€™]\›€
\™Щ]Ю€ЏH[™ЩK›Z[‚€	‰€\™Щ]Ю€H[™ЩK›X^
HИќYH€[ЩNВџB‚љ[ќЪXЪЧЬЬXЪX[

BћВ€[ќ]H
™NВ€YЉ
[]™[O››ЬЬXЪX[]™[O››ЬЬXЪX[OHКH	‰€[Y[љ[JЩ[‹S’WФФPТPS
H	‰‚€
ЪXЪЧЩ[™\™ЮJS‘T‘ЦWХTWТS’WФФPТPS
HЪXЪЧЩ[™\™ЮJS‘T‘ЦWХTWУTS’WФФPТPS
JB€
B€В€Щ[‹OќZЩXXЭ[Ы€HЫЫ[[Ы—Ш]XЪЧЬ›ШОВ€Щ]Ш]XЪЪ[™КЩ[ЉNВ€Y[\Щ]
Щ[‹OЫЫX›ЬЭ\Ъ^™[ЩЉ
њЩ[‹OЫЫX›ЬЭ\
H
€JNВ‚€HHЩ[‹O›[љОВ€YЉJB€В€KOќZЩXXЭ[Ы€H•SВ€[ќЭ[›[љКЩ[ЉNВ€Щ]ЪYJJNВ€B‚€YЉЩ[‹O›[Щ[]KњЫX\ќ›ЫX€	‰€\Щ[‹O›[Щ[]K™Щњ™Y^™JB€В€ЫX\ќШ›ЫXЉЩ[‹Щ[‹O›[Щ[]KњЫX\ќ›ЫXЉNИЛИИЫX\ќ›ЫX€[[YYX][HY€]Щ\Ы‰Эњ™Y^™HШЬ™Y[‚€B‚€Щ[‹Oњќ[›љ[™ИH•S—ФХUWУ“У‘NИЛИY€ЬXЪX[\И^XЭ]YЪ[Hќ[›љ[™ЛЩX\Щ\ИИќ[‚€Щ[‹Oќ™[ШЪ]KћHЩ[‹Oќ™[ШЪ]Kћ€HВ€[ќЬЩ]Ш[љ[JЩ[‹S’WФФPТPS
NВ‚€YЉЩ[‹O›[Щ[]K™Щњ™Y^™JB€В€ЫX\ќ›ЫX™\€HЩ[ЋИЛИњ™Y^™\ИH[љ[X][ЫњИЩ€[[™[ZY\ЛЬ^Y\њИЪ[HЬXЪX[\И^XЭ]Y€B‚€ЛИЬ]\И
LLЊЊJH›ЭИHљ[™љ[љ]HX[ЪX]€Y™™XЭИ^Y\њИЫ›K›Э[™[ZY\ИЬ€њВ€ЛИ[™›ЭИHљ[™љ[љ]HX[ЪX]€Ъ[Ы›HЫЬљИЪ[€HЫЬЭ\ИPSЪ[›ЭЫЬљИЪ[€HЫЬЭ\ИT[ћ[[Ь™B€YЉЩ[‹O›[Щ[]Kќ\H	€TWФVQTЉB€В€YЉ[›ШЫЬЭ
B€В€YЉJЫШ[ШЫЫ™љYЛЪX]И	€ТPUУФSУ”ЧТPSРPХU‘JJB€В€YЉЩ[‹O›[Щ[]K[љ[X][Ы–РS’WФФPТPSKO™[™\™ЮWШЫЬЭЫЬЭ
B€В€YЉЪXЪЧЩ[™\™ЮJS‘T‘ЦWХTWУTS’WФФPТPS
JB€В€Щ[‹O™[™\™ЮWЬЭ]K›\ШЭ\њ™[ќOHЩ[‹O›[Щ[]K[љ[X][Ы–РS’WФФPТPSKO™[™\™ЮWШЫЬЭЫЬЭВ€B€[ЩB€В€Щ[‹O™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќOHЩ[‹O›[Щ[]K[љ[X][Ы–РS’WФФPТPSKO™[™\™ЮWШЫЬЭЫЬЭВ€B€B€B€[ЩB€В€YЉЩ[‹O›[Щ[]K[љ[X][Ы–РS’WФФPТPSKO™[™\™ЮWШЫЬЭЫЬЭ
B€В€YЉЪXЪЧЩ[™\™ЮJS‘T‘ЦWХTWУTS’WФФPТPS
JB€В€Щ[‹O™[™\™ЮWЬЭ]K›\ШЭ\њ™[ќOHЩ[‹O›[Щ[]K[љ[X][Ы–РS’WФФPТPSKO™[™\™ЮWШЫЬЭЫЬЭВ€B€B€B€B€B€[ЩB€В€YЉ[›ШЫЬЭ
B€В€YЉЩ[‹O›[Щ[]K[љ[X][Ы–РS’WФФPТPSKO™[™\™ЮWШЫЬЭЫЬЭ
B€В€YЉЪXЪЧЩ[™\™ЮJS‘T‘ЦWХTWУTS’WФФPТPS
JB€В€Щ[‹O™[™\™ЮWЬЭ]K›\ШЭ\њ™[ќOHЩ[‹O›[Щ[]K[љ[X][Ы–РS’WФФPТPSKO™[™\™ЮWШЫЬЭЫЬЭВ€B€[ЩB€В€Щ[‹O™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќOHЩ[‹O›[Щ[]K[љ[X][Ы–РS’WФФPТPSKO™[™\™ЮWШЫЬЭЫЬЭВ€B€B€B€B€™]\›€NВ€B€™]\›€ВџB‚‹ЛИЪXЪИЩ^\И›Ь€ЬXЪX[[Э™K€\ЩYЩ]™\[[Y\ЛЫИHќ[ЙЩ]‚‹ЛИЬ]\И
LLЊЊJHYY™]И›YЬИИ\ЩHЪ][›Э\€UPТИИЩ^\И\И[€™]И[\›]]™Bљ[ќ^Y\—ШЪXЪЧЬЬXЪX[

BћВ€Щ^WЫX\ЪЧЭZЩ^HHВ€Щ^WЫX\ЪЧЭ^Y\—ЪЩ^\ИH^Y\–ЬЩ[‹Oњ^Y\љ[™^Kњ^ZЩ^\ОВ‚€ЭЪ]Ъ
ЫШ[ШЫЫ™љYЛZњЬXЪX[
B€В€Ш\ЩHR”ФPТPSТСVWФФPТPS‚€€Y€
^Y\—ЪЩ^\И	€“QЧФФPТPS
B€В€ZЩ^HH“QЧФФPТPSВ€H‚€њ™XZОВ‚€Ш\ЩHR”ФPТPSТСVWСХP“N‚‚€Y€
][Y[љ[JЩ[‹S’WР“РТКH	‰€^Y\—ЪЩ^\И	€“QЧФФPТPS
B€В€ZЩ^HH“QЧФФPТPSВ€B€[ЩHY€
^Y\—ЪЩ^\И	€“QЧТ•ST	‰€^Y\—ЪЩ^\И	€“QЧРUPТКB€В€ZЩ^HH“QЧТ•ST“QЧРUPТОВ€B€њ™XZОВ‚€Ш\ЩHR”ФPТPSТСVWРUPТМЋ‚‚€Y€
^Y\—ЪЩ^\И	€“QЧРUPТМЉB€В€ZЩ^HH“QЧРUPТМЋВ€B€њ™XZОВ‚€Ш\ЩHR”ФPТPSТСVWРUPТМО‚‚€Y€
^Y\—ЪЩ^\И	€“QЧРUPТМКB€В€ZЩ^HH“QЧРUPТМОВ€B€њ™XZОВ‚€Ш\ЩHR”ФPТPSТСVWРUPТН‚‚€Y€
^Y\—ЪЩ^\И	€“QЧРUPТН
B€В€ZЩ^HH“QЧРUPТНВ€B€њ™XZОВ€€Y][‚€ZЩ^HHВ€B‚€Y€
]ZЩ^JB€В€™]\›€В€B‚€YЉЪXЪЧЬЬXЪX[

JB€В€Щ[‹OњЭ[[YHHВ€^Y\–ЬЩ[‹Oњ^Y\љ[™^Kњ^ZЩ^\И	ЏHќZЩ^NВ€™]\›€NВ€B€[ЩB€В€™]\›€В€BџB‚‚ќ›ЪYЫЫ[[Ы—Ы[™

BћВ€Щ[‹Oќ™[ШЪ]KћHЩ[‹Oќ™[ШЪ]Kћ€HВ€YЉЩ[‹O[љ[X][™КB€В€™]\›ЋВ€B‚€Щ[‹OќZЩXXЭ[Ы€H•SВ€Щ]ЪYJЩ[ЉNВџB‚‚‹ЛШ[љ[X[ќ[€Ъ[€[ЭHЬЭ]И[Y\ИћHZ[Вќ›ЪYќ[[љ[X[

BћВ€ЫЫ[[Ы—ЭШ[ЧШ[љ[JЩ[ЉNВ€ЛЩ[ќЬЩ]Ш[љ[JЩ[‹S’WХРSЛ
NВ‚€YЉЩ[‹OњЬЪ][Ы‹ћY[Щ^HЩ[‹OњЬЪ][Ы‹ћ€Y[Щ^
И
љY[Ы[Щ\Лљ™\И
И
JB€В€Ъ[Щ[ќ]JЩ[‹ТSСS•UWХ’QССT—РS’SPSФ•S—УХUУС—Р“ХS‘КNВ€™]\›ЋВ€B‚€YЉЩ[‹O™\™XЭ[Ы€OHT‘PХSУ—Ф’QТ
B€В€Щ[‹OњЬЪ][Ы‹ћ
ПHЩ[‹O›[Щ[]KњЬYYћВ€B€[ЩB€В€Щ[‹OњЬЪ][Ы‹ћOHЩ[‹O›[Щ[]KњЬYYћВ€BџB‚‚ќ›ЪY^Y\—Ш›[љК
BћВ€Щ[‹O›[љИHNВ€YЉЭ[YHЏHЩ[‹OњЭ[[YJB€В€^Y\—ЩYJ
NВ€BџB‚‚ќ›ЪYЫЫ[[Ы—ЩЬX]XЪК
BћВ€YЉЩ[‹O[љ[X][™КB€В€™]\›ЋВ€B‚€Щ[‹O]XЪЪ[™ИHUPТТS‘ЧУ“У‘NВ‚€YЉJЩ[‹OЫЫX›ЬЭ\МHЩ[‹OЫЫX›ЬЭ\МWH€Щ[‹OЫЫX›ЬЭ\М—HЩ[‹OЫЫX›ЬЭ\МЧH€Щ[‹OЫЫX›ЬЭ\НJJB€В€[ќЭ[›[љКЩ[ЉNВ€B‚€YЉЩ[‹O›[љКB€В€Щ[‹OќZЩXXЭ[Ы€HЫЫ[[Ы—ЩЬXЋВ€Щ[‹O›[љЛOќZЩXXЭ[Ы€HЫЫ[[Ы—ЩЬX™YВ€Щ[‹O]XЪЪ[™ИHUPТТS‘ЧУ“У‘NВ€[ќЬЩ]Ш[љ[JЩ[‹S’WСФђP‹
NВ€Щ]ЬZ[ЉЩ[‹O›[љЛLK
NВ€\]WЩњ[YJЩ[‹Щ[‹O[љ[X][Ы‹O›ќ[Yњ[Y\ИHJNВ€\]WЩњ[YJЩ[‹O›[љЛЩ[‹O›[љЛO[љ[X][Ы‹O›ќ[Yњ[Y\ИHJNВ€B€[ЩB€В€Щ[‹OќZЩXXЭ[Ы€H•SВ€Y[\Щ]
Щ[‹OЫЫX›ЬЭ\Ъ^™[ЩЉ
њЩ[‹OЫЫX›ЬЭ\
H
€JNВ€Щ]ЪYJЩ[ЉNВ€BџB‚‹ЛИќ[Э[Ы€]Ш]\Щ\ИH^Y\€ИЫЫќ[ќYHИ[Э™H\Ь€ЭЫ€[ќ[H[љ[X][Ы€\Иљ[љ\ЪY^Z[™Вќ›ЪYЫЫ[[Ы—ЩЩЩJ
HЛИ™]Иќ[Э[Ы€ЫИ^Y\њИШ[€ЩЩHЪ]\\Ь€ЭЫ€ЭЫ‚ћВ€YЉЩ[‹O[љ[X][™КHЛИЫЫќ[ќY\ИИ[Э™H\ИЫ™И\ИH^Y\€\И[љ[X][™В€В€™]\›ЋВ€B€[ЩHЛИЫЩHЫ™H[љ[X][™Л™]\›њИИ[љЪ[™В€В€Щ[‹OќZЩXXЭ[Ы€H•SВ€Щ[‹Oќ™[ШЪ]KћHЩ[‹Oќ™[ШЪ]Kћ€HВ€Щ]ЪYJЩ[ЉNВ€BџB‚‚ќ›ЪYЫЫ[[Ы—Ь™YXЪК
BћВ€YЉЩ[‹O[љ[X][™КB€В€™]\›ЋВ€B€ЩXЪКЩ[ЉNВџB‚‚ќ›ЪYЫЫ[[Ы—ЪYJ
BћВ€YЉЩ[‹O[љ[X][™КB€В€™]\›ЋВ€B€Щ[‹OќZЩXXЭ[Ы€H•SВ€Щ[‹O]XЪЪ[™ИHUPТТS‘ЧУ“У‘NВ€Щ[‹OљY[™ИHQS‘ЧФ‘TT‘QВ€ЫЫ[[Ы—ЪYWШ[љ[JЩ[ЉNВџB‚‚ќ›ЪYћYXЪК[ќ]H
™[ќ
BћВ€[ќOњќ[›љ[™ИH•S—ФХUWУ“У‘NВ€YЉ[Y[љ[J[ќS’WСPТТS‘КJB€В€[ќOќZЩXXЭ[Ы€HЫЫ[[Ы—Ь™YXЪОВ€[ќOќ™[ШЪ]KћH[ќOќ™[ШЪ]Kћ€HВ€[ќO™XЪЪ[™ИHPТЧФ‘TT‘QВ€[ќOљY[™ИHQS‘ЧУ“У‘NВ€[ќЬЩ]Ш[љ[J[ќS’WСPТТS‘Л
NВ€B€[ЩB€В€ЩXЪК[ќ
NВ€BџB‚‚ќ›ЪYћYXЪЬљ\ЩJ[ќ]H
™[ќ
BћВ€[ќOњќ[›љ[™ИH•S—ФХUWУ“У‘NВ€YЉ[Y[љ[J[ќS’WСPТФ’TСJJB€В€[ќOќZЩXXЭ[Ы€HЫЫ[[Ы—ЪYNВ€[ќOќ™[ШЪ]KћH[ќOќ™[ШЪ]Kћ€HВ€[ќO™XЪЪ[™ИHPТЧФ’TСNВ€[ќOљY[™ИHQS‘ЧУ“У‘NВ€[ќЬЩ]Ш[љ[J[ќS’WСPТФ’TСK
NВ€B€[ЩB€В€[ќOќZЩXXЭ[Ы€H•SВ€[ќOљY[™ИHQS‘ЧФ‘TT‘QВ€ЫЫ[[Ы—ЪYWШ[љ[JЩ[ЉNВ€BџB‚‚ќ›ЪYЩXЪК[ќ]H
™[ќ
BћВ€[ќOќZЩXXЭ[Ы€H•SВ€[ќOќ™[ШЪ]KћH[ќOќ™[ШЪ]Kћ€HВ€[ќO™XЪЪ[™ИHPТЧРPХU‘NВ€[ќOљY[™ИHQS‘ЧФ‘TT‘QВ€[ќЬЩ]Ш[љ[J[ќS’WСPТЛ
NВџB‚‚ќ›ЪYЫЫ[[Ы—Ь™Zќ[\

BћВ€YЉЩ[‹O[љ[X][™КB€В€™]\›ЋВ€B€Ъќ[\
Щ[‹Oљќ[\ќ™[ШЪ]KћKЩ[‹Oљќ[\ќ™[ШЪ]KћЩ[‹Oљќ[\ќ™[ШЪ]Kћ‹Щ[‹Oљќ[\[љ[X][Ы—ЪY
NВџB‚‚ќ›ЪYћZќ[\
›Ш]ќ[\‹›Ш]ќ[\›Ш]ќ[\‹[љ[X][Ы—ЪYЭ[љ[X][Ы—ЪY
BћВ€Щ[‹Oљќ[\ќ™[ШЪ]KћHHќ[\ЋВ€Щ[‹Oљќ[\ќ™[ШЪ]KћHќ[\В€Щ[‹Oљќ[\ќ™[ШЪ]Kћ€Hќ[\ЋВ€Щ[‹Oљќ[\[љ[X][Ы—ЪYH[љ[X][Ы—ЪYВ‚€Щ[‹O™XЪЪ[™ИHPТЧУ“У‘NВ€YЉ[Y[љ[JЩ[‹S’WТ•STSVJJB€В€Щ[‹OќZЩXXЭ[Ы€HЫЫ[[Ы—Ь™Zќ[\В€Щ[‹Oќ™[ШЪ]KћHЩ[‹Oќ™[ШЪ]Kћ€HВ‚€Щ[‹OљY[™ИHQS‘ЧУ“У‘NВ€[ќЬЩ]Ш[љ[JЩ[‹S’WТ•STSVK
NВ€B€[ЩB€В€Ъќ[\
ќ[\‹ќ[\ќ[\‹[љ[X][Ы—ЪY
NВ€BџB‚‚ќ›ЪYЪќ[\
›Ш]ќ[\‹›Ш]ќ[\›Ш]ќ[\‹[љ[X][Ы—ЪYЭ[љ[X][Ы—ЪY
BћВ€[ќ]H
™\ЭВ‚€Щ[‹OќZЩXXЭ[Ы€HЫЫ[[Ы—Ъќ[\В‚€YЉЫШ[ЬШ[\WЫ\Эљќ[\ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Эљќ[\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B‚€ЛФЬ]Ы€ќ[\Э\ќ\Э‚€YЉЩ[‹O›[Щ[]K™\Эљќ[\ЬЭ\ќЏH
B€В€\ЭHЬ]ЫЉЩ[‹OњЬЪ][Ы‹ћЩ[‹OњЬЪ][Ы‹ћ‹Щ[‹OњЬЪ][Ы‹ћKЩ[‹O™\™XЭ[Ы‹•SЩ[‹O›[Щ[]K™\Эљќ[\ЬЭ\ќ•S
NВ€YЉ\Э
B€В€\ЭOњЬ]Ыќ\HHФUУ—ХTWСTХТ•STВ€\ЭO\ЩHHЩ[‹OњЬЪ][Ы‹ћNВ€\ЭO]]ЪЪ[HUUТТSРS’SPUSУ—РУУTUNВ€^XЭ]WЫЫњЬ]Ы—ЬШЬљ\
\Э
NВ€B€B‚€Щ]Ъќ[\[™КЩ[ЉNВ‚€ЬЬКЩ[‹ќ[\ЉNВ‚€YЉЩ[‹O™\™XЭ[Ы€OHT‘PХSУ—УQ•
B€В€Щ[‹Oќ™[ШЪ]KћHZќ[\В€B€[ЩB€В€Щ[‹Oќ™[ШЪ]KћHќ[\В€B‚€Щ[‹Oќ™[ШЪ]Kћ€Hќ[\ЋВ€[ќЬЩ]Ш[љ[JЩ[‹[љ[X][Ы—ЪY
NВџB‚‹ЛИќ[Э[Ы€Ь™X]YИЫЫXљ[™HHXЭ[Ы€ZЩ[€Y€Z]\€XЪЪ[™И\[€][KЬ€ќ[›љ[™И[ќИ[€][H]\ИB‹ЛИХP•TWХХPТ^XЭ][™ИH\›ЬљX]HXЭ[Ы€\ЩYЫ€ЪXЪ\HЩ€][H\ИXЪЩY\ќ›ЪYYљ[™Ъ][J[ќ]H
›Э\ЉBћВ€ЛИќ[Э[Ы€]ZЩ\ИШ\™HЩ€][\ИЪ[€XЪЩY\€Щ]ЫЬЫ™[ќ
Щ[‹Э\ЉNВ‚€К‚€
€YШXЮH[Z]Y\ЩH™\[љ\Ъ‚€
€€
€Y€][H\ИH\ЩWШY[YH[™ЫЫXЭ[™В€
€[ќ]H\ИH[Z]Y\ЩHЩX\Ы‹ЩHY€
€\ЩWШYИ\Щ\И™[XZ[љ[™Л‚€
‹В‚€YЉЭ\‹O›[Щ[]KќЩX\Ы—Ь›Ь\ќY\Лќ\ЩWШY
B€В€YЉЩ[‹OќЩX\[ќ	‰€Щ[‹OќЩX\[ќO›[Щ[]KќЩX\Ы—Ь›Ь\ќY\ЛќЩX\Ы—ЬЭ]H	€СPTУ—ФХUWУSRUQХTСJB€В€Щ[‹OќЩX\[ќO›[Щ[]KќЩX\Ы—Ь›Ь\ќY\Лќ\ЩWШЫЭ[ќ
ПHЭ\‹O›[Щ[]KќЩX\Ы—Ь›Ь\ќY\Лќ\ЩWШYВ‚€YЉЫШ[ЬШ[\WЫ\Э™Щ]ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Щ]Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€[ЩB€В€YШЫЬ™JЩ[‹Oњ^Y\љ[™^Э\‹O›[Щ[]KњШЫЬ™JNВ€YЉЫШ[ЬШ[\WЫ\Э™Щ]М€ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Щ]М‹Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€B‚€ЛЩ[™Щ€ЩX\ЫњИ][\ИЩXЭ[Ы‚€[ЩHYЉЭ\‹O›[Щ[]KњШЫЬ™JB€В€YШЫЬ™JЩ[‹Oњ^Y\љ[™^Э\‹O›[Щ[]KњШЫЬ™JNВ€YЉЫШ[ЬШ[\WЫ\Э™Щ]М€ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Щ]М‹Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€[ЩHYЉЭ\‹O™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќ
B€В€Щ[‹O™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќ
ПHЭ\‹O™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќВ‚€YЉЩ[‹O™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќ€Щ[‹O›[Щ[]KљX[
B€В€Щ[‹O™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќHЩ[‹O›[Щ[]KљX[В€B‚€Э\‹O™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќHВ‚€YЉЫШ[ЬШ[\WЫ\Э™Щ]ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Щ]Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€[ЩHYЉЭ\‹O›[Щ[]K›\
B€В€Щ[‹O™[™\™ЮWЬЭ]K›\ШЭ\њ™[ќ
ПHЭ\‹O›[Щ[]K›\В‚€YЉЩ[‹O™[™\™ЮWЬЭ]K›\ШЭ\њ™[ќ€Щ[‹O›[Щ[]K›\
B€В€Щ[‹O™[™\™ЮWЬЭ]K›\ШЭ\њ™[ќHЩ[‹O›[Щ[]K›\В€B‚€Э\‹O™[™\™ЮWЬЭ]K›\ШЭ\њ™[ќHВ€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Щ]Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€[ЩHYЉЭљXЫ\
Э\‹O›[Щ[]K›[YK•[YHЉHOH
B€В€[Y[YќH]™[OњЩ][YH
€ЫШ[ШЫЫ™љYЛЫЭ[ќ\—ЬЬYYИЛИ™X€ЌЊHH\И[™H[Э™Y\™HИЩ]Э\ЭЫH[YB‚€YЉЫШ[ЬШ[\WЫ\Э™Щ]М€ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Щ]М‹Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€[ЩHYЉЭ\‹O›[Щ[]K›XZЩZ[ќЉB€В€ЛИX\€‹ЊHH™]И][HXZЩ\И^Y\€[ќљ[ЪX›B€Щ[‹Oљ[ќљ[ЪX›HHS•’SђТP“WТS•S‘ТP“NВ€Щ[‹Oљ[ќљ[Э[YHHЭ[YH
ИP”КЭ\‹O›[Щ[]K›XZЩZ[ќЉNВ€Щ[‹O›[љИH
Э\‹O›[Щ[]K›XZЩZ[ќ€€
NВ‚€YЉЫШ[ЬШ[\WЫ\Э™Щ]М€ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Щ]М‹Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€[ЩHYЉЭ\‹O›[Щ[]KњЫX\ќ›ЫXЉB€В€ЛИ[XYЩ\И]™\ћ][™ИЫ€HШЬ™Y[‚€ЫX\ќШ›ЫXЉЩ[‹Э\‹O›[Щ[]KњЫX\ќ›ЫXЉNВ‚€YЉЫШ[ЬШ[\WЫ\Э™Щ]М€ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Щ]М‹Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€[ЩHYЉЭ\‹O›[Щ[]KњЭXќ\HOHХP•TWХСPTУЉB€В€›ЬЩX\ЫЉЩ[‹
NВ€Щ[‹OќЩX\[ќHЭ\ЋВ€Щ]ЭЩX\ЫЉЩ[‹Э\‹O›[Щ[]KќЩX\Ы—Ь›Ь\ќY\ЛќЩX\Ы—Ъ[™^
NВ‚€К€[Э™HИЩX\Ы€ШШ][Ы€Y€]	ЬИ[€[љ[X[‹€
‹В€YЉЩ[‹O›[Щ[]KќЩX\Ы—Ь›Ь\ќY\ЛќЩX\Ы—ЬЭ]H	€СPTУ—ФХUWРS’SPS
B€В€Щ[‹O™\™XЭ[Ы€HЭ\‹O™\™XЭ[ЫЋВ€Щ[‹OњЬЪ][Ы‹ћHЭ\‹OњЬЪ][Ы‹ћВ€Щ[‹OњЬЪ][Ы‹ћ€HЭ\‹OњЬЪ][Ы‹ћЋВ€B‚€YЉJЭ\‹O›[Щ[]KќЩX\Ы—Ь›Ь\ќY\ЛќЩX\Ы—ЬЭ]H	€СPTУ—ФХUWУSRUQХTСJH	‰€Щ[‹O›[Щ[]KќЩX\Ы—Ь›Ь\ќY\ЛќЩX\Ы—ЬЭ]H	€СPTУ—ФХUWУSRUQХTСJB€В€Э\‹O›[Щ[]KќЩX\Ы—Ь›Ь\ќY\ЛќЩX\Ы—ЬЭ]HHСPTУ—ФХUWУSRUQХTСNВ€B‚€YЉЫШ[ЬШ[\WЫ\Э™Щ]ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Щ]Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€[ЩHYЉЭ\‹O›[Щ[]KњЭXќ\HOHХP•TWФ“Т‘PХSJB€В€›ЬЩX\ЫЉЩ[‹
NВ€Щ[‹OќЩX\[ќHЭ\ЋВ‚€YЉЫШ[ЬШ[\WЫ\Э™Щ]ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Щ]Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€[ЩHYЉЭ\‹O›[Щ[]KЬ™Y]
B€В€YЉ[›ЬЪ\™JB€В€Ь™Y]ККОВ€B€[ЩB€В€^Y\–ЬЩ[‹Oњ^Y\љ[™^KЬ™Y]ККОВ€B‚€YЉЫШ[ЬШ[\WЫ\Э›Ы™WЭ\ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э›Ы™WЭ\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€[ЩB€В€ЛИ]\Э™HH]\[‹‚€^Y\–ЬЩ[‹Oњ^Y\љ[™^K›]™\ККОВ‚€YЉЫШ[ЬШ[\WЫ\Э›Ы™WЭ\ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э›Ы™WЭ\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B‚€YЉЭ\‹O›[Щ[]KњЭXќ\HOHХP•TWХСPTУ€	‰€Э\‹O›[Щ[]KњЭXќ\HOHХP•TWФ“Т‘PХSJB€В€Э\‹OќZЩXXЭ[Ы€HЭZXЪYNВ€YЉ[Э\‹O›[Щ[]Kљ[њЭ[ќ][YX]
B€В€Э\‹O›™^[љИHЭ[YH
ИЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYY
€ОВ€B€B€[ЩB€В€YЉJЭ\‹O›™^[љ[H	€SVWС“QЧТS‘’S’UJJHВ€Э\‹O›™^[љ[HH[љ[X][Ы—Э[Y\Э[\ШYШ›Э[™Y
€Э[YK€[^WЫ][\WШ›Э[™Y
€ЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYY€RS•ЌРКNNNNNJB€
B€
NВ€B€Э\‹O›™^[љИHЭ[YH
ИЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYY
€NNNNNNВ€B€Э\‹OњЬЪ][Ы‹ћ€HUSWТQWФФТUSУ—ЦЋВџB‚ќ›ЪY^Y\—Щ[ШЪXЪК
BћВ€YЉ]]Ы[™OH€	‰€
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€
“QЧУSХ‘UT“QЧТ•ST
JHOH
“QЧУSХ‘UT“QЧТ•ST
JB€В‚BKЛИX\љИ]ЫИЩHЪ[^H[™[љ[X][Ы€Ъ[€]HЬ›Э[™‚€Щ[‹O™[XYЩWЫЫ—Ы[™[™Л]XЪЧЩ›ЬЩHHUPТЧС“ФђСWУS‘РУУSPS‘И€BџB‚ќ›ЪY^Y\—ЩЬX—ШЪXЪК
BћВ€[ќ]H
›Э\€HЩ[‹O›[љОВ‚€YЉЭ\€OH•S
Щ[‹O›[Щ[]K™ЬX™љ[љ\Ъ	‰€Щ[‹O[љ[X][™И	‰€\Щ[‹O™ЬXќШ[Ъ[™КJB€В€™]\›ЋВ€B‚€YЉЩ[‹O\ЩHOHЭ\‹O\ЩJB€В€ЛИЪ[™ЩH\Ињ›ЫHOњЬЪ][Ы‹ћHИO\ЩB€Щ[‹OќZЩXXЭ[Ы€H•SВ€[ќЭ[›[љКЩ[ЉNВ€Щ]ЪYJЩ[ЉNВ€™]\›ЋВ€B‚€YЉ^Y\—ШЪXЪЧЬЬXЪX[

JB€В€™]\›ЋВ€B‚€YЉ[›ЫЬЭ	‰€Щ[‹O›[Щ[]KќЩX\Ы—Ь›Ь\ќY\Л›ЬЬЧШЫЫ™][Ы€	€СPTУ—УФФЧРУУ‘USУ—СФђPђ’S‘КB€В€›ЬЩX\ЫЉЩ[‹JNВ€B‚€ЛИЬXќ\›€ЫЩB€YЉЩ[‹O[љ[X][Ы€OHЩ[‹O›[Щ[]K[љ[X][Ы–РS’WСФђP•T“—JB€В€ЛИЭ[\›љ[™ПИЫ‰Э›Э\€Ъ][ћ][™И[ЩB€YЉЩ[‹O[љ[X][™КB€В€™]\›ЋВ€B‚€ЛИЫ™H\›љ[™ПИЭЪ]Ъ\™XЭ[ЫњИ[™™]\›€ИЬX€[љ[X][Ы‚€[ЩB€В€YЉЩ[‹O™\™XЭ[Ы€OHT‘PХSУ—Ф’QТ
B€В€Щ[‹O™\™XЭ[Ы€HT‘PХSУ—УQ•В€Э\‹O™\™XЭ[Ы€HT‘PХSУ—Ф’QТВ€B€[ЩB€В€Щ[‹O™\™XЭ[Ы€HT‘PХSУ—Ф’QТВ€Э\‹O™\™XЭ[Ы€HT‘PХSУ—УQ•В€B€Э\‹OњЬЪ][Ы‹ћHЩ[‹OњЬЪ][Ы‹ћ
И


Щ[‹O™\™XЭ[Ы€
€ЉHHJH
€Щ[‹O›[Щ[]K™ЬX™\Э[ЩJNВ€[ќЬЩ]Ш[љ[JЩ[‹S’WСФђP‹
NВ€Щ]ЬZ[ЉЭ\‹LK
NВ€\]WЩњ[YJЩ[‹Щ[‹O[љ[X][Ы‹O›ќ[Yњ[Y\ИHJNВ€\]WЩњ[YJЭ\‹Э\‹O[љ[X][Ы‹O›ќ[Yњ[Y\ИHJNВ€B€B‚€Щ[‹O]XЪЪ[™ИHUPТТS‘ЧУ“У‘NИЛЩ›Ь€ЪXЪЪ[™В€Щ[‹O™ЬXќШ[Ъ[™ИHВ‚‚KЛИ[Э™HЩ^HЬЬЪ]HњЛ€XЬ™][ЫЏВ€YЉЩ[‹O™\™XЭ[Ы€OHT‘PХSУ—Ф’QТИ
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘SQ•
H€
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘T’QТ
JB€В€ЛИ[љ]X][™ИЬXќ\›‚€YЉЩ[‹O›[Щ[]K™ЬXќ\›ЉB€В€ЛИЭ\ќ[љ[X][Ы€Y€]^\ЭЛ‹‹‚€YЉ[Y[љ[JЩ[‹S’WСФђP•T“ЉJB€В€[ќЬЩ]Ш[љ[JЩ[‹S’WСФђP•T“‹
NВ€YЉ[Y[љ[JЭ\‹S’WСФђPђ‘QT“ЉJB€В€[ќЬЩ]Ш[љ[JЭ\‹S’WСФђPђ‘QT“‹
NВ€B€[ЩHYЉ[Y[љ[JЭ\‹S’WСФђPђ‘Q
JB€В€[ќЬЩ]Ш[љ[JЭ\‹S’WСФђPђ‘Q
NВ€B€[ЩB€В€Y€


[Э\‹O™\™XЭ[Ы€	‰€Щ[‹OњЬЪ][Ы‹ћ€Э\‹OњЬЪ][Ы‹ћ
H
Э\‹O™\™XЭ[Ы€	‰€Щ[‹OњЬЪ][Ы‹ћЭ\‹OњЬЪ][Ы‹ћ
JH	‰€[Y[љ[JЭ\‹S’WРђPТФRSЉH
H[ќЬЩ]Ш[љ[JЭ\‹S’WРђPТФRS‹
NВ€[ЩH[ќЬЩ]Ш[љ[JЭ\‹S’WФRS‹
NВ€B€Э\‹Oќ™[ШЪ]KћHЭ\‹Oќ™[ШЪ]Kћ€HЩ[‹Oќ™[ШЪ]KћHЩ[‹Oќ™[ШЪ]Kћ€HВ€Э\‹OњЬЪ][Ы‹ћHЩ[‹OњЬЪ][Ы‹ћВ€™]\›ЋВ€B‚€ЛИЭ\ќЪ\ЩKќ\Э\›€\›Э[™€[ЩB€В€YЉЩ[‹O™\™XЭ[Ы€OHT‘PХSУ—Ф’QТ
B€В€Щ[‹O™\™XЭ[Ы€HT‘PХSУ—УQ•В€Э\‹O™\™XЭ[Ы€HT‘PХSУ—Ф’QТВ€B€[ЩB€В€Щ[‹O™\™XЭ[Ы€HT‘PХSУ—Ф’QТВ€Э\‹O™\™XЭ[Ы€HT‘PХSУ—УQ•В€B€[ќЬЩ]Ш[љ[JЩ[‹S’WСФђP‹
NВ€Щ]ЬZ[ЉЭ\‹LK
NВ€\]WЩњ[YJЩ[‹Щ[‹O[љ[X][Ы‹O›ќ[Yњ[Y\ИHJNВ€\]WЩњ[YJЭ\‹Э\‹O[љ[X][Ы‹O›ќ[Yњ[Y\ИHJNВ€Э\‹OњЬЪ][Ы‹ћHЩ[‹OњЬЪ][Ы‹ћ
И


Щ[‹O™\™XЭ[Ы€
€ЉHHJH
€Щ[‹O›[Щ[]K™ЬX™\Э[ЩJNВ€B€B€[ЩHYЉ][Y[љ[JЩ[‹S’WСФђP•РSКH	‰€Э[YH€Щ[‹Oњ™[X\Щ][YJB€В€ЛИ™[X\ЩB€Щ[‹OќZЩXXЭ[Ы€H•SВ€[ќЭ[›[љКЩ[ЉNВ€Щ]ЪYJЩ[ЉNВ€™]\›ЋВ€B€B€[ЩB€В€Щ[‹Oњ™[X\Щ][YHHЭ[YH
И
ЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYYИЉNВ€B‚€YЉ
^Y\–ЬЩ[‹Oњ^Y\љ[™^Kњ^ZЩ^\И	€“QЧРUPТКH	‰‚€
Щ[‹O™\™XЭ[Ы€В€
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘SQ•
H‚€
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘T’QТ
JJB€В€^Y\–ЬЩ[‹Oњ^Y\љ[™^Kњ^ZЩ^\ИOH“QЧРUPТОВ€YЉ[Y[љ[JЩ[‹S’WСФђPђђPТХРT‘
JB€В€ЩЬX]XЪКФђP—РPХSУ—ФСSPХРђPТХРT‘
NВ€B€[ЩHYЉ[Y[љ[JЩ[‹S’WХ“ХКJB€В€YЉЩ[‹O›[Щ[]Kќ›ЭЩњ[Y]ШZ]ЏH
B€В€Ь™]›ЭК
NВ€B€[ЩB€В€Э›ЭК
NВ€B€B€[ЩB€В€ЩЬX]XЪКФђP—РPХSУ—ФСSPХРUPТКNВ€B€B€ЛИЬX€›ЬќШ\™€[ЩHYЉ
^Y\–ЬЩ[‹Oњ^Y\љ[™^Kњ^ZЩ^\И	€“QЧРUPТКH	‰‚€[Y[љ[JЩ[‹S’WСФђP‘“Ф•РT‘
H	‰‚€
\Щ[‹O™\™XЭ[Ы€В€
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘SQ•
H‚€
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘T’QТ
JJB€В€^Y\–ЬЩ[‹Oњ^Y\љ[™^Kњ^ZЩ^\И	ЏH‘“QЧРUPТОВ€ЩЬX]XЪКФђP—РPХSУ—ФСSPХС“Ф•РT‘
NВ€B€ЛИЬX€\€[ЩHYЉ
^Y\–ЬЩ[‹Oњ^Y\љ[™^Kњ^ZЩ^\И	€“QЧРUPТКH	‰‚€[Y[љ[JЩ[‹S’WСФђP•T
H	‰€
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘UT
JB€В€^Y\–ЬЩ[‹Oњ^Y\љ[™^Kњ^ZЩ^\И	ЏH‘“QЧРUPТОВ€ЩЬX]XЪКФђP—РPХSУ—ФСSPХХT
NВ€B€ЛИЬX€ЭЫ‚€[ЩHYЉ
^Y\–ЬЩ[‹Oњ^Y\љ[™^Kњ^ZЩ^\И	€“QЧРUPТКH	‰‚€[Y[љ[JЩ[‹S’WСФђP‘ХУЉH	‰€
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘QХУЉJB€В€^Y\–ЬЩ[‹Oњ^Y\љ[™^Kњ^ZЩ^\И	ЏH‘“QЧРUPТОВ€ЩЬX]XЪКФђP—РPХSУ—ФСSPХСХУЉNВ€B€ЛИ›Ь›X[ЬX€]XЪВ€[ЩHYЉ
^Y\–ЬЩ[‹Oњ^Y\љ[™^Kњ^ZЩ^\И	€“QЧРUPТКH	‰€[Y[љ[JЩ[‹S’WСФђPђUPТКJB€В€^Y\–ЬЩ[‹Oњ^Y\љ[™^Kњ^ZЩ^\И	ЏH‘“QЧРUPТОВ€ЩЬX]XЪКФђP—РPХSУ—ФСSPХРUPТКNВ€B‚KЛИЬ]\И
LLЊЊJHYYH][[љ[X][Ы‚€[ЩHYЉ^Y\–ЬЩ[‹Oњ^Y\љ[™^Kњ^ZЩ^\И	€“QЧТ•ST	‰€[Y[љ[JЩ[‹S’WХђUS
JB€В€ЛИЬ]\И
ЛLЊЊЉHљ^YHЊЛ]][И€ЫЬќYЛ›ЭИЪ[^XЭ]HЫЩH\И[ќ[™Y€ЛИЬ]\И
KLЊЊКHљ^Y[€\ЬЭYHЪ\™HЬX]XЪМ€[љ[X][Ы€ќ[YљY\И][Y€›Э\™HXЫ\™Y€ЛИЬ]\И
KLЊЊКHYYЫЫYH]]™H™Z]љ[Э\њВ€^Y\–ЬЩ[‹Oњ^Y\љ[™^Kњ^ZЩ^\И	ЏH‘“QЧТ•STВ€Щ[‹O]XЪЪ[™ИHUPТТS‘ЧРPХU‘NВ€Щ[‹OќZЩXXЭ[Ы€HЫЫ[[Ы—ЩЬX]XЪОВ€Y[\Щ]
Щ[‹OЫЫX›ЬЭ\Ъ^™[ЩЉ
њЩ[‹OЫЫX›ЬЭ\
H
€JNВ€[ќЬЩ]Ш[љ[JЩ[‹S’WХђUS
NВ€B€ЛИЬX€]XЪИљ[љ\Ъ\‚€[ЩHYЉ^Y\–ЬЩ[‹Oњ^Y\љ[™^Kњ^ZЩ^\И	€
“QЧРUPТИ“QЧТ•ST
JB€В€^Y\–ЬЩ[‹Oњ^Y\љ[™^Kњ^ZЩ^\И	ЏHЉ“QЧРUPТИ“QЧТ•ST
NВ‚€ЛИ\™›Ь›Hљ[[›ЭВ€YЉ[Y[љ[JЩ[‹S’WСФђPђUPТМЉH[Y[љ[JЩ[‹S’WРUPТМКJB€В€ЩЬX]XЪКФђP—РPХSУ—ФСSPХС’S’TТ
NВ€B€[ЩB€В€Щ[‹O]XЪЪ[™ИHUPТТS‘ЧРPХU‘NВ€Y[\Щ]
Щ[‹OЫЫX›ЬЭ\Ъ^™[ЩЉ
њЩ[‹OЫЫX›ЬЭ\
H
€JNВ€Щ[‹OќZЩXXЭ[Ы€HЫЫ[[Ы—ЩЬX]XЪОВ€ћZќ[\
Щ[‹O›[Щ[]Kљќ[\ZYЪЩ[‹O›[Щ[]Kљќ[\ЬYYS’WТ•ST
NВ€B€B‚€ЛИЬX€Ш[ИЫЩB€[ЩHYЉ[Y[љ[JЩ[‹S’WСФђP•РSКHЛИЪXЪИY€ЬXќШ[И[љ[X][Ы€^\ЭВ‚€ЛИY€[ќ]H\ИЭ[[љ[X][™И[ћ][™И™\ЪY\ИHЬXќШ[И\љX[ќЫ‰Э][H[Э™B€	‰€
\Щ[‹O[љ[X][™ИЩ[‹O[љ[X][Ы€OHЩ[‹O›[Щ[]K[љ[X][Ы–РS’WСФђP•РSЧB€Щ[‹O[љ[X][Ы€OHЩ[‹O›[Щ[]K[љ[X][Ы–РS’WСФђP•РSХTB€Щ[‹O[љ[X][Ы€OHЩ[‹O›[Щ[]K[љ[X][Ы–РS’WСФђP•РSСХУ—B€Щ[‹O[љ[X][Ы€OHЩ[‹O›[Щ[]K[љ[X][Ы–РS’WСФђPђђPТХРSЧJJB€В‚€ЛИ€^\И[Э™[Y[ќ€YЉVQT—УRS—Ц€OHVQT—УPVЦЉB€В€YЉ^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘UT
B€В€YЉЩ[‹O›[Щ[]K™ЬXќШ[ЬЬYY
B€В€Щ[‹Oќ™[ШЪ]Kћ€H\Щ[‹O›[Щ[]K™ЬXќШ[ЬЬYYИЋВ€B€[ЩB€В€Щ[‹Oќ™[ШЪ]Kћ€H\Щ[‹O›[Щ[]KњЬYYћИЋВ€B€B€[ЩHYЉ^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘QХУЉB€В€YЉЩ[‹O›[Щ[]K™ЬXќШ[ЬЬYY
B€В€Щ[‹Oќ™[ШЪ]Kћ€HЩ[‹O›[Щ[]K™ЬXќШ[ЬЬYYИЋВ€B€[ЩB€В€Щ[‹Oќ™[ШЪ]Kћ€HЩ[‹O›[Щ[]KњЬYYћИЋВ€B€B€[ЩHYЉJ^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€
“QЧУSХ‘UT“QЧУSХ‘QХУЉJJB€В€Щ[‹Oќ™[ШЪ]Kћ€HВ€B€B‚€ЛИ^\И[Э™[Y[ќ€YЉ^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘SQ•
B€В€YЉЩ[‹O›[Щ[]K™ЬXќШ[ЬЬYY
B€В€Щ[‹Oќ™[ШЪ]KћH\Щ[‹O›[Щ[]K™ЬXќШ[ЬЬYYВ€B€[ЩB€В€Щ[‹Oќ™[ШЪ]KћH\Щ[‹O›[Щ[]KњЬYYћВ€B€B‚€[ЩHYЉ^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘T’QТ
B€В€YЉЩ[‹O›[Щ[]K™ЬXќШ[ЬЬYY
B€В€Щ[‹Oќ™[ШЪ]KћHЩ[‹O›[Щ[]K™ЬXќШ[ЬЬYYВ€B€[ЩB€В€Щ[‹Oќ™[ШЪ]KћHЩ[‹O›[Щ[]KњЬYYћВ€B€B€[ЩHYЉJ
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘SQ•
H
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘T’QТ
JH
B€В€Щ[‹Oќ™[ШЪ]KћHВ€B‚€ЛИЩ][™ИH[љ[X][ЫњИ\ЩYЫ€H™[ШЪ]HЩ]X›Э™B€YЉЩ[‹Oќ™[ШЪ]KћЩ[‹Oќ™[ШЪ]KћЉB€В€YЉ

Щ[‹Oќ™[ШЪ]Kћ€	‰€Щ[‹O™\™XЭ[Ы€OHT‘PХSУ—УQ•
H
Щ[‹Oќ™[ШЪ]Kћ	‰€Щ[‹O™\™XЭ[Ы€OHT‘PХSУ—Ф’QТ
JH	‰€[Y[љ[JЩ[‹S’WСФђPђђPТХРSКJB€В€[ќЬЩ]Ш[љ[JЩ[‹S’WСФђPђђPТХРSЛ
NВ€B€[ЩHYЉЩ[‹Oќ™[ШЪ]Kћ€	‰€[Y[љ[JЩ[‹S’WСФђP•РSХT
JB€В€[ќЬЩ]Ш[љ[JЩ[‹S’WСФђP•РSХT
NВ€B€[ЩHYЉЩ[‹Oќ™[ШЪ]Kћ€€	‰€[Y[љ[JЩ[‹S’WСФђP•РSСХУЉJB€В€[ќЬЩ]Ш[љ[JЩ[‹S’WСФђP•РSСХУ‹
NВ€B€[ЩB€В€[ќЬЩ]Ш[љ[JЩ[‹S’WСФђP•РSЛ
NВ€B€YЉЩ[‹O[љ[X][Ы€OHЩ[‹O›[Щ[]K[љ[X][Ы–РS’WСФђP•РSХTH	‰€[Y[љ[JЭ\‹S’WСФђPђ‘QРSХT
JB€В€[ќЬЩ]Ш[љ[JЭ\‹S’WСФђPђ‘QРSХT
NВ€B€[ЩHYЉЩ[‹O[љ[X][Ы€OHЩ[‹O›[Щ[]K[љ[X][Ы–РS’WСФђP•РSСХУ—H	‰€[Y[љ[JЭ\‹S’WСФђPђ‘QРSСХУЉJB€В€[ќЬЩ]Ш[љ[JЭ\‹S’WСФђPђ‘QРSСХУ‹
NВ€B€[ЩHYЉЩ[‹O[љ[X][Ы€OHЩ[‹O›[Щ[]K[љ[X][Ы–РS’WСФђPђђPТХРSЧH	‰€[Y[љ[JЭ\‹S’WСФђPђ‘QђPТХРSКJB€В€[ќЬЩ]Ш[љ[JЭ\‹S’WСФђPђ‘QђPТХРSЛ
NВ€B€[ЩHYЉ[Y[љ[JЭ\‹S’WСФђPђ‘QРSКJB€В€[ќЬЩ]Ш[љ[JЭ\‹S’WСФђPђ‘QРSЛ
NВ€B€[ЩHY€
[Y[љ[JЭ\‹S’WСФђPђ‘Q
JB€В€[ќЬЩ]Ш[љ[JЭ\‹S’WСФђPђ‘Q
NВ€B€[ЩB€В€[ќЬЩ]Ш[љ[JЭ\‹S’WФRS‹
NВ€B€B€[ЩB€В€[ќЬЩ]Ш[љ[JЩ[‹S’WСФђP‹
NВ€Y€
[Y[љ[JЭ\‹S’WСФђPђ‘Q
JB€В€[ќЬЩ]Ш[љ[JЭ\‹S’WСФђPђ‘Q
NВ€B€[ЩB€В€[ќЬЩ]Ш[љ[JЭ\‹S’WФRS‹
NВ€B€B€ЛИ\ЩHЪXЪЧЫ[љЧЫ[Э™HИЩ]™[ШЪ]KЫ‰ЭЪ[™ЩH]\™B€Э\‹Oќ™[ШЪ]Kћ€HЭ\‹Oќ™[ШЪ]KћHВ€Щ[‹O™ЬXќШ[Ъ[™ИHNВ€B‚€YЉЩ[‹O]XЪЪ[™ИOHUPТТS‘ЧУ“У‘JB€В€Щ[‹Oњ™[X\Щ][YHHЭ[YH
И
ЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYYИЉNИЛИ™\Щ]™[X\Щ][YHЪ[€ИЫЫ\Ъ[Ы‚€BџB‚ќ›ЪY^Y\—ЭШ[ЫЩ™—ШЪXЪК
BћВ€К‚€
€\›‹€€
‹В€YЉЩ[‹O›[Щ[]KZ\—ШЫЫќ›Ы	€RT—РУУ•“УХРSУС‘—ХT“ЉB€В€YЉ
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘SQ•
JB€В€Щ[‹O™\™XЭ[Ы€HT‘PХSУ—УQ•В€B€[ЩHYЉ
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘T’QТ
JB€В€Щ[‹O™\™XЭ[Ы€HT‘PХSУ—Ф’QТВ€B€B‚€К‚€
€ЭЬ€Y€›ЭЫ[™ИHYќЬ‚€
€љYЪЩ^KЪ[Ьљ^›Ыќ[[ЫY[ќ[K‚€
‹В€Y€
Щ[‹O›[Щ[]KZ\—ШЫЫќ›Ы	€RT—РУУ•“УХРSУС‘—ЦФХФ
B€В€Y€
J^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€
“QЧУSХ‘SQ•“QЧУSХ‘T’QТ
JJB€В€Y€
Щ[‹Oќ™[ШЪ]KћOH
B€В€Щ[‹Oќ™[ШЪ]Kћ
ЏHRT—РУУ•“УФХФСђPХФЋВ€B€B€B‚€К‚€
€€ЭЬ€Y€›ЭЫ[™И[€\Ь‚€
€ЭЫ€Щ^KЪ[]\[[ЫY[ќ[K‚€
‹В€Y€
Щ[‹O›[Щ[]KZ\—ШЫЫќ›Ы	€RT—РУУ•“УХРSУС‘—Ц—ФХФ
B€В€Y€
J^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€
“QЧУSХ‘UT“QЧУSХ‘QХУЉJJB€В€Y€
Щ[‹Oќ™[ШЪ]Kћ€OH
B€В€Щ[‹Oќ™[ШЪ]Kћ€
ЏHRT—РУУ•“УФХФСђPХФЋВ€B€B€B‚€К€Ьљ^›Ыќ[[Э™HЫЫќ›Ы
Y€[™XYH[Эљ[™КK€
‹В€YЉЩ[‹O›[Щ[]KZ\—ШЫЫќ›Ы	€RT—РУУ•“УХРSУС‘—ЦРQ•TХ
HЛЫ[Э™OВ€В€YЉ

^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘SQ•
H	‰€Щ[‹Oќ™[ШЪ]Kћ€
H€

^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘T’QТ
H	‰€Щ[‹Oќ™[ШЪ]Kћ
JB€В€Щ[‹Oќ™[ШЪ]KћH\Щ[‹Oќ™[ШЪ]KћВ€B€B‚€К€Ьљ^›Ыќ[[Э™HЫЫќ›Ы
[ћHШ[ЫЩ™ЉK€
‹В€YЉЩ[‹O›[Щ[]KZ\—ШЫЫќ›Ы	€RT—РУУ•“УХРSУС‘—ЦУSХ‘JB€В€YЉ

^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘SQ•
H	‰€Щ[‹Oќ™[ШЪ]Kћ€
H€

^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘T’QТ
H	‰€Щ[‹Oќ™[ШЪ]Kћ
JB€В€Щ[‹Oќ™[ШЪ]KћH\Щ[‹Oќ™[ШЪ]KћВ€B‚€YЉ
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘SQ•
H	‰€
\Щ[‹Oќ™[ШЪ]Kћ
JB€В€Щ[‹Oќ™[ШЪ]KћOHЩ[‹O›[Щ[]KњЬYYћВ€B€[ЩHYЉ
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘T’QТ
H	‰€
\Щ[‹Oќ™[ШЪ]Kћ
JB€В€Щ[‹Oќ™[ШЪ]KћHЩ[‹O›[Щ[]KњЬYYћВ€B€B‚€К€€[Э™HЫЫќ›Ы
Y€[™XYH[Эљ[™КK€
‹В€YЉЩ[‹O›[Щ[]KZ\—ШЫЫќ›Ы	€RT—РУУ•“УХРSУС‘—Ц—РQ•TХ
B€В€YЉ

^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘UT
H	‰€Щ[‹Oќ™[ШЪ]Kћ€€
H€

^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘QХУЉH	‰€Щ[‹Oќ™[ШЪ]Kћ€
JB€В€Щ[‹Oќ™[ШЪ]Kћ€H\Щ[‹Oќ™[ШЪ]KћЋВ€B€B‚€К€€[Э™HЫЫќ›Ы
[ћHШ[ЫЩ™ЉK€
‹В€YЉЩ[‹O›[Щ[]KZ\—ШЫЫќ›Ы	€RT—РУУ•“УХРSУС‘—Ц—УSХ‘JB€И€YЉ
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘UT
H	‰€
\Щ[‹Oќ™[ШЪ]KћЉJB€В€Щ[‹Oќ™[ШЪ]Kћ€OH
ЌH
€Щ[‹O›[Щ[]KњЬYYћ
NВ€B€[ЩHYЉ
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘QХУЉH	‰€
\Щ[‹Oќ™[ШЪ]KћЉJB€В€Щ[‹Oќ™[ШЪ]Kћ€H
ЌH
€Щ[‹O›[Щ[]KњЬYYћ
NВ€B€B‚€™]\›ЋВџB‚ќ›ЪY^Y\—Ъќ[\ШЪXЪК
BћВ€[ќШ[™ЬЬXЪX[HВ‚€ЛИЬ]\И
LLЊЊJHљ^YH›ШZ\Ш[Щ[ќ[Э[Ы‚€ЛИ›ЭИH›YИЊ€€ЫЬљЬИ\И[ќ[™Y[™ЫЫ\][H\ШX›\ИHШ[Щ[][Ы€™]ЩY[€[ќ[\[™И]XЪЬВ€ЛИ[€H™]љ[Э\ИЫЩK›Э›YЬИH[™€]™HHШ[YHY™™XЭ[™[ЭИHШ[Щ[][Ы€Yќ\€\Эќ[\]XЪИ\Иљ[љ\ЪY€YЉ
[›ШZ\Ш[Щ[
H
›ШZ\Ш[Щ[	ЊH	‰€\Щ[‹O[љ[X][™КH
›ШZ\Ш[Щ[	ЊИ	‰€
Щ[‹O[љ[[ќ[HOHЩ[‹Oљќ[\[љ[X][Ы—ЪY
JJB€В€ЛШZ\€ЬXЪX[ЫЬYY[™Ъ[™ЩYњ›ЫHќYЭYIЬИЫЩB€YЉ
[]™[O››ЬЬXЪX[]™[O››ЬЬXЪX[OHКH	‰€^Y\–ЬЩ[‹Oњ^Y\љ[™^Kњ^ZЩ^\И	€“QЧФФPТPS
B€В€YЉ[Y[љ[JЩ[‹S’WТ•STФPТPS
JB€В€ЛИЬ]\И
LLЊЊJH›Ь€ШY™KYY[€™]ИЭ\ИЪXЪИY€H[ќ]H\И›Э[€Hљќ[\ЬXЪX[€[љ[X][Ы‚€ЛИљ^\ИHќYИ]ЫЫњЭ[ќHЫЫњЭ[Y\ИX[Ь€\Y€HЬXЪX[ќ]Ы€\ИЫЫњЭ[ќH™\ЬЩY]™[€Y‚€ЛИHЭ\њ™[ќљќ[\ЬXЪX[€[љ[X][Ы€ЮXЫH\И›Эљ[љ\ЪYY]€YЉЩ[‹O[љ[[ќ[HOHS’WТ•STФPТPS
B€В€YЉЩ[‹O›[Щ[]K[љ[X][Ы–РS’WТ•STФPТPSKO™[™\™ЮWШЫЬЭЫЬЭ	‰€ЪXЪЧЩ[™\™ЮJS‘T‘ЦWХTWУTS’WТ•STФPТPS
JB€В€ЛИЬ]\И
LLЊЊJH›ЭИHљ[™љ[љ]HX[ЪX]€Ъ[Ы›HЫЬљИЪ[€HЫЬЭ\ИPSЪ[›ЭЫЬљИЪ[€HЫЬЭ\ИT[ћ[[Ь™B€Щ[‹O™[™\™ЮWЬЭ]K›\ШЭ\њ™[ќOHЩ[‹O›[Щ[]K[љ[X][Ы–РS’WТ•STФPТPSKO™[™\™ЮWШЫЬЭЫЬЭВ€Ш[™ЬЬXЪX[HNВ€B€[ЩHYЉЩ[‹O›[Щ[]K[љ[X][Ы–РS’WТ•STФPТPSKO™[™\™ЮWШЫЬЭЫЬЭ	‰€ЪXЪЧЩ[™\™ЮJS‘T‘ЦWХTWТS’WТ•STФPТPS
JB€В€YЉJЫШ[ШЫЫ™љYЛЪX]И	€ТPUУФSУ”ЧТPSРPХU‘JJB€В€Щ[‹O™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќOHЩ[‹O›[Щ[]K[љ[X][Ы–РS’WТ•STФPТPSKO™[™\™ЮWШЫЬЭЫЬЭВ€B€Ш[™ЬЬXЪX[HNВ€B€[ЩHYЉ[Y[љ[JЩ[‹S’WТ•STРS•
JB€В€[ќЬЩ]Ш[љ[JЩ[‹S’WТ•STРS•
NВ€Щ[‹Oќ™[ШЪ]KћHHВ€B€B‚€YЉШ[™ЬЬXЪX[
B€В€^Y\–ЬЩ[‹Oњ^Y\љ[™^Kњ^ZЩ^\И	ЏH‘“QЧФФPТPSВ€Щ[‹O]XЪЪ[™ИHUPТТS‘ЧРPХU‘NВ‚€ЛИЬ]\И
LLЊЊJHYHЬ[Ы€ИЪ[Ь€›ЭH^€[Э™[Y[ќ€ЛИЬ]\И
LЊЊЉHZ[›Ь€љ^Ы€Hќ[\ЬXЪX[ЫЩB€YЉJЩ[‹O›[Щ[]Kљќ[\ЬXЪX[	€JJB€В€Щ[‹Oќ™[ШЪ]KћHЩ[‹Oќ™[ШЪ]Kћ€HИЛИЪ[[Э™[Y[ќЪ[€HЬXЪX[Э\ќВ€Щ[‹Oќ™[ШЪ]KћHHВ€B‚€[ќЬЩ]Ш[љ[JЩ[‹S’WТ•STФPТPS
NВ€B€B€KЛЩ[™Щ€ќ[\ЬXЪX[‚€ЛЪќ[\]XЪЬЛ\ЭЫ€›ЬќШ\™›Ь›X[‹‹‹ќЩHЫ‰ЭЪXЪИ[™\™ЮHЫЬЭ€[ЩHYЉ^Y\–ЬЩ[‹Oњ^Y\љ[™^Kњ^ZЩ^\И	€“QЧРUPТКB€В€^Y\–ЬЩ[‹Oњ^Y\љ[™^Kњ^ZЩ^\И	ЏH‘“QЧРUPТОВ€Щ[‹O]XЪЪ[™ИHUPТТS‘ЧРPХU‘NВ‚€YЉ
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘QХУЉH	‰€[Y[љ[JЩ[‹S’WТ•STUPТМЉJB€В€[ќЬЩ]Ш[љ[JЩ[‹S’WТ•STUPТМ‹
NВ€B€[ЩHYЉ
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘UT
H	‰€[Y[љ[JЩ[‹S’WТ•STUPТМКJB€В€[ќЬЩ]Ш[љ[JЩ[‹S’WТ•STUPТМЛ
NВ€B€[ЩHYЉЩ[‹Oњќ[›љ[™И	‰€[Y[љ[JЩ[‹S’WФ•S’•STUPТКJB€В€[ќЬЩ]Ш[љ[JЩ[‹S’WФ•S’•STUPТЛ
NИЛИYYЫИ[€^HЭ›Ы™Иќ[\]XЪИШ[€™H^XЭ]Y€B€[ЩHYЉЩ[‹Oќ™[ШЪ]KћOH	‰€[Y[љ[JЩ[‹S’WТ•ST“Ф•РT‘
JB€В€[ќЬЩ]Ш[љ[JЩ[‹S’WТ•ST“Ф•РT‘
NИЛИY€[Эљ[™И[™Щ]И\И]XЪВ€B€[ЩHYЉ[Y[љ[JЩ[‹S’WТ•STUPТКJB€В€[ќЬЩ]Ш[љ[JЩ[‹S’WТ•STUPТЛ
NВ€B€KЛЩ[™Щ€ќ[\]XЪВ€B‚€К€€
€ќ[\ZYЪЫЫќ›Ы€ЭЬљ\Ъ[™ИY€ќ[\€
€Щ^H\И[XЭ]™K‚€
‹В€Y€
Щ[‹O›[Щ[]KZ\—ШЫЫќ›Ы	€RT—РУУ•“УТ•STЦWФХФ
B€В€Y€
J^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧТ•ST
JB€В€Y€
Щ[‹Oќ™[ШЪ]KћH€
B€В€Щ[‹Oќ™[ШЪ]KћH
ЏHRT—РУУ•“УФХФСђPХФЋВ€B€B€B‚€К€€
€ќ[\ЭЬ€Y€›ЭЫ[™ИHYќЬ‚€
€љYЪЩ^KЪ[Ьљ^›Ыќ[[ЫY[ќ[K‚€
‹В€Y€
Щ[‹O›[Щ[]KZ\—ШЫЫќ›Ы	€RT—РУУ•“УТ•STЦФХФ
B€В€Y€
J^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€
“QЧУSХ‘SQ•“QЧУSХ‘T’QТ
JJB€В€Y€
Щ[‹Oќ™[ШЪ]KћOH
B€В€Щ[‹Oќ™[ШЪ]Kћ
ЏHRT—РУУ•“УФХФСђPХФЋВ€B€B€B‚€К‚€
€ќ[\€ЭЬ€Y€›ЭЫ[™И[€\Ь‚€
€ЭЫ€Щ^KЪ[]\[[ЫY[ќ[K‚€
‹В€Y€
Щ[‹O›[Щ[]KZ\—ШЫЫќ›Ы	€RT—РУУ•“УТ•STЦ—ФХФ
B€В€Y€
J^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€
“QЧУSХ‘UT“QЧУSХ‘QХУЉJJB€В€Y€
Щ[‹Oќ™[ШЪ]Kћ€OH
B€В€Щ[‹Oќ™[ШЪ]Kћ€
ЏHRT—РУУ•“УФХФСђPХФЋВ€B€B€B‚€К€ќ[\\›€ЫЫќ›Ы€
‹В€YЉЩ[‹O›[Щ[]KZ\—ШЫЫќ›Ы	€RT—РУУ•“УТ•STХT“ЉB€В€YЉ
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘SQ•
JB€В€Щ[‹O™\™XЭ[Ы€HT‘PХSУ—УQ•В€B€[ЩHYЉ
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘T’QТ
JB€В€Щ[‹O™\™XЭ[Ы€HT‘PХSУ—Ф’QТВ€B€B‚€К€ќ[\Ьљ^›Ыќ[[Э™HЫЫќ›Ы
Y€[™XYH[Эљ[™КK€
‹В€YЉЩ[‹O›[Щ[]KZ\—ШЫЫќ›Ы	€RT—РУУ•“УТ•STЦРQ•TХ
B€В€YЉ

^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘SQ•
H	‰€Щ[‹Oќ™[ШЪ]Kћ€
H€

^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘T’QТ
H	‰€Щ[‹Oќ™[ШЪ]Kћ
JB€В€Щ[‹Oќ™[ШЪ]KћH\Щ[‹Oќ™[ШЪ]KћВ€B€B€€К€ќ[\Ьљ^›Ыќ[[Э™HЫЫќ›Ы
[ћHќ[\
K€
‹В€YЉЩ[‹O›[Щ[]KZ\—ШЫЫќ›Ы	€RT—РУУ•“УТ•STЦУSХ‘JB€И€YЉ

^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘SQ•
H	‰€Щ[‹Oќ™[ШЪ]Kћ€
H€

^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘T’QТ
H	‰€Щ[‹Oќ™[ШЪ]Kћ
JB€В€Щ[‹Oќ™[ШЪ]KћH\Щ[‹Oќ™[ШЪ]KћВ€B‚€YЉ
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘SQ•
H	‰€
\Щ[‹Oќ™[ШЪ]Kћ
JB€В€Щ[‹Oќ™[ШЪ]KћOHЩ[‹O›[Щ[]KњЬYYћВ€B€[ЩHYЉ
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘T’QТ
H	‰€
\Щ[‹Oќ™[ШЪ]Kћ
JB€В€Щ[‹Oќ™[ШЪ]KћHЩ[‹O›[Щ[]KњЬYYћВ€B€B€€К€ќ[\€[Э™HЫЫќ›Ы
Y€[™XYH[Эљ[™КK€
‹В€YЉЩ[‹O›[Щ[]KZ\—ШЫЫќ›Ы	€RT—РУУ•“УТ•STЦ—РQ•TХ
B€В€YЉ

^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘UT
H	‰€Щ[‹Oќ™[ШЪ]Kћ€€
H€

^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘QХУЉH	‰€Щ[‹Oќ™[ШЪ]Kћ€
JB€В€Щ[‹Oќ™[ШЪ]Kћ€H\Щ[‹Oќ™[ШЪ]KћЋВ€B€B‚€К€ќ[\€[Э™HЫЫќ›Ы
[ћHќ[\
K€
‹В€YЉЩ[‹O›[Щ[]KZ\—ШЫЫќ›Ы	€RT—РУУ•“УТ•STЦ—УSХ‘JB€В€YЉ
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘SQ•
JB€В€Щ[‹O™\™XЭ[Ы€HT‘PХSУ—УQ•В€B€[ЩHYЉ
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘T’QТ
JB€В€Щ[‹O™\™XЭ[Ы€HT‘PХSУ—Ф’QТВ€B‚€YЉ

^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘UT
H	‰€Щ[‹Oќ™[ШЪ]Kћ€€
H€

^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘QХУЉH	‰€Щ[‹Oќ™[ШЪ]Kћ€
JB€В€Щ[‹Oќ™[ШЪ]Kћ€H\Щ[‹Oќ™[ШЪ]KћЋВ€B‚€YЉ
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘UT
H	‰€
\Щ[‹Oќ™[ШЪ]KћЉJB€В€Щ[‹Oќ™[ШЪ]Kћ€OH
ЌH
€Щ[‹O›[Щ[]KњЬYYћ
NВ€B€[ЩHYЉ
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘QХУЉH	‰€
\Щ[‹Oќ™[ШЪ]KћЉJB€В€Щ[‹Oќ™[ШЪ]Kћ€H
ЌH
€Щ[‹O›[Щ[]KњЬYYћ
NВ€B€B‚€™]\›ЋВџB‚ќ›ЪY^Y\—ЬZ[—ШЪXЪК
BћВ€YЉ^Y\—ШЪXЪЧЬЬXЪX[

JB€В€Щ[‹Oљ[њZ[€HS—ФRS—У“У‘NВ€Щ[‹Oњљ\Ъ[™ИH’TТS‘ЧУ“У‘NВ€Щ[‹O™XЪЪ[™ИHPТЧУ“У‘NВ€Щ[‹Oљ[XЪЬZ[€HВ€BџB‚‹ЛИЪXЪИљ\ЩX]XЪИ[њ]\
Ш]XЪВќ›ЪY^Y\—ЫYWШЪXЪК
BћВ€YЉ[Y[љ[JЩ[‹S’WФ’TСPUPТКH	‰‚€
^Y\–ЬЩ[‹Oњ^Y\љ[™^Kњ^ZЩ^\И	€“QЧРUPТКH	‰‚€
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘UT
H	‰‚€
Щ[‹O™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќ€	‰€Э[YH€Щ[‹OњЭ^YЭЫ‹њљ\ЩX]XЪЧЬЭ[
JB€В€^Y\–ЬЩ[‹Oњ^Y\љ[™^Kњ^ZЩ^\И	ЏH‘“QЧРUPТОВ€YЉ
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘SQ•
JB€В€Щ[‹O™\™XЭ[Ы€HT‘PХSУ—УQ•В€B€YЉ
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧУSХ‘T’QТ
JB€В€Щ[‹O™\™XЭ[Ы€HT‘PХSУ—Ф’QТВ€B€Щ[‹OњЭ[[YHHВ€Щ]Ьљ\ЩX]XЪКЩ[‹Щ[‹O›\ЭЩ[XYЩWЭ\K
NВ€BџB‚ќ›ЪY^Y\—ШЪ\™ЩWШЪXЪК
BћВ€YЉJ
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧТ•ST
H	‰‚€
^Y\–ЬЩ[‹Oњ^Y\љ[™^KљЩ^\И	€“QЧФФPТPS
JJB€В€Щ[‹OќZЩXXЭ[Ы€H•SВ€Щ[‹OЪ\™Ъ[™ИHВ€Щ]ЪYJЩ[ЉNВ€BџB‚‹ЛИXZЩHHќ[Э[Ы€ЫИ[™[ZY\ИШ[€\ЩB‹ЛИU€ќ[\XЪИ\ИH[\Ь\ћHљ^›Ь€ќ[\Ш[Щ[љ[ќЪXЪЧШЫЬЭ[Э™J[ќЛ[ќњЛ[ќќ[\XЪКBћВ€YЉ

њИOHH	‰€]™[O››ЬЬXЪX[ЉH
њИOH	‰€]™[O››ЬЬXЪX[OH
H
њИOH	‰€]™[O››ЬЬXЪX[OHКJH	‰‚€
ЪXЪЧЩ[™\™ЮJS‘T‘ЦWХTWТКH€ЪXЪЧЩ[™\™ЮJS‘T‘ЦWХTWУTКJH
B€В€YЉZќ[\XЪКB€В€Щ[‹OќZЩXXЭ[Ы€HЫЫ[[Ы—Ш]XЪЧЬ›ШОВ€B€ЛИЬ]\И
LLЊЊJH›ЭИHљ[™љ[љ]HX[ЪX]€Y™™XЭИ^Y\њИЫ›K›Э[™[ZY\ИЬ€њВ€ЛИ[™›ЭИHљ[™љ[љ]HX[ЪX]€Ъ[Ы›HЫЬљИЪ[€HЫЬЭ\ИPSЪ[›ЭЫЬљИЪ[€HЫЬЭ\ИT[ћ[[Ь™B€YЉЩ[‹O›[Щ[]Kќ\H	€TWФVQTЉB€В€YЉ[›ШЫЬЭ
B€В€YЉJЫШ[ШЫЫ™љYЛЪX]И	€ТPUУФSУ”ЧТPSРPХU‘JJB€В€YЉЩ[‹O›[Щ[]K[љ[X][Ы–ЬЧKO™[™\™ЮWШЫЬЭЫЬЭ
B€В€YЉЪXЪЧЩ[™\™ЮJS‘T‘ЦWХTWУTКJB€В€Щ[‹O™[™\™ЮWЬЭ]K›\ШЭ\њ™[ќOHЩ[‹O›[Щ[]K[љ[X][Ы–ЬЧKO™[™\™ЮWШЫЬЭЫЬЭВ€B€[ЩB€В€Щ[‹O™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќOHЩ[‹O›[Щ[]K[љ[X][Ы–ЬЧKO™[™\™ЮWШЫЬЭЫЬЭВ€B€B€B€[ЩB€В€YЉЩ[‹O›[Щ[]K[љ[X][Ы–ЬЧKO™[™\™ЮWШЫЬЭЫЬЭ
B€В€YЉЪXЪЧЩ[™\™ЮJS‘T‘ЦWХTWУTКJB€В€Щ[‹O™[™\™ЮWЬЭ]K›\ШЭ\њ™[ќOHЩ[‹O›[Щ[]K[љ[X][Ы–ЬЧKO™[™\™ЮWШЫЬЭЫЬЭВ€B€B€B€B€B€[ЩB€В€YЉ[›ШЫЬЭ
B€В€YЉЩ[‹O›[Щ[]K[љ[X][Ы–ЬЧKO™[™\™ЮWШЫЬЭЫЬЭ
B€В€YЉЪXЪЧЩ[™\™ЮJS‘T‘ЦWХTWУTКJB€В€Щ[‹O™[™\™ЮWЬЭ]K›\ШЭ\њ™[ќOHЩ[‹O›[Щ[]K[љ[X][Ы–ЬЧKO™[™\™ЮWШЫЬЭЫЬЭВ€B€[ЩB€В€Щ[‹O™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќOHЩ[‹O›[Щ[]K[љ[X][Ы–ЬЧKO™[™\™ЮWШЫЬЭЫЬЭВ€B€B€B€B‚‚B\Щ[‹Oњќ[›љ[™ИH•S—ФХUWУ“У‘NВ€Щ[‹Oќ™[ШЪ]KћHЩ[‹Oќ™[ШЪ]Kћ€HВ€Щ]Ш]XЪЪ[™КЩ[ЉNВ€Щ[‹Oљ[њZ[€HS—ФRS—У“У‘NВ€Щ[‹Oњљ\Ъ[™ИH’TТS‘ЧУ“У‘NВ€Щ[‹Oљ[XЪЬZ[€HВ€Y[\Щ]
Щ[‹OЫЫX›ЬЭ\Ъ^™[ЩЉ
њЩ[‹OЫЫX›ЬЭ\
H
€JNВ€[ќЭ[›[љКЩ[ЉNВ€[ќЬЩ]Ш[љ[JЩ[‹Л
NВ€™]\›€NВ€B€™]\›€ВџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊЌ‹LЛLMВЉ‚Љ€™XЫЬ™HЭ\ќ[™И[YH›Ь€]™\ћH™]ЫH™\ЬЩYЉ€ЫЫ[X[™[њ]›YЛ‚Љ‚Љ€\ЪXШ[[™\™XЭ[Ы‹\™[]]™H›YЬИЪ\™HHШ[YBЉ€ЌXљ][Y\ЬXЩK€\Ъ[™ИH›YЙЬИљ][™^›ЭљY\ВЉ€Ы[Z[™ИЪ]Э]H\[[ЫЫ[X[™Z\ЭЬћHљ[™Л‚Љ‹ВњЭ]XИ›ЪYЫЫ[X[™Ъ[њ]ЪЫЬЭ\ќЭ\]J€ЧЬ^Y\Љ€XЭ[™ЧЬ^Y\‹€ЫЫњЭЩ^WЫX\ЪЧЭ™\ЬЩYЩ›YЬЛ€ЫЫњЭZ[ќЌЭ]™[ќЭ[YBЉHВ€Щ^WЫX\ЪЧЭШШ[—ЫX\ЪИH™\ЬЩYЩ›YЬОВ‚€Ъ[JШШ[—ЫX\ЪКHВ€ЫЫњЭZ[ќЌЭ[њ]Ъ[™^B€љ]X\ЪНЌЩЩ]ЫЭЩ\ЭЪ[™^
ШШ[—ЫX\ЪКNВ‚€ЫЫњЭЩ^WЫX\ЪЧЭ[њ]Щ›YИB€љ]X\ЪНЌЩњ›ЫWЪ[™^
[њ]Ъ[™^
NВ‚€XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]ЪЫЬЭ\ќЭ[YVВ€[њ]Ъ[™^€HH]™[ќЭ[YNВ‚€XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]ЪЫЬЭ\ќЭ[YB€[њ]Щ›YОВ‚€К‚€
€H™]И™\ЬИ™YЪ[њИH™]ИЫЫќ[ќ[Э\ИЫ€ЫX\‚€
€H]]ЫX]XЛ]љYЩЩ\€™XЫЬ™ЫИ[YH™\›ИШ[‚€
€љYЩЩ\€YШZ[€›Ь€H™]ИЫ‚€
‹В€XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]ЪЫЭљYЩЩ\—Э[Y	ЏB€љ[њ]Щ›YОВ‚€ШШ[—ЫX\ЪИ	ЏHљ[њ]Щ›YОВ€BџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊЌ‹LЛLMВЉ‚Љ€ЫЫXЭ]]ЫX]XИ[Z[њ]YЩ\ИЪXЪ™XXЪZ\‚Љ€ЫЫ™љYЭ\™Y™\ЪЫЫ€\ИЩЪXШ[XЪЛ‚Љ‚Љ€™\ЪЫИЫЫYHњ›ЫHHXЭ[™И[ќ]IЬИЫЫ™љYЭ\X›BЉ€ЬXЪX[ЫЫ[X[™Л€H\‹Z[њ][Y\Э[\™]™[ќИBЉ€ЩXЫЫ™[њ]™Yњ™\Ъ\љ[™ИHШ[YHXЪИњ›ЫH[Z][™ВЉ€HШ[YHYЩHYШZ[‹€Y™™\™[ќ™\ЪЫИ›Ь€HШ[YBЉ€[њ]™[XZ[€[™\[™[ќ™XШ]\ЩHXXЪ\И™XXЪYЫ€BЉ€Y™™\™[ќЩЪXШ[XЪИ\љ[™ИЫ™HЫЫќ[ќ[Э\ИЫ‚Љ‹ВњЭ]XИЩ^WЫX\ЪЧЭЫЫ[X[™Ъ[њ]ЪЫЭљYЩЩ\—ШЫЫXЭ
€ЧЬ^Y\Љ€XЭ[™ЧЬ^Y\‹€ЫЫњЭЩ^WЫX\ЪЧЭ[Щ›YЬЛ€ЫЫњЭZ[ќЌЭ]™[ќЭ[YBЉHВ€ЫЫњЭ[ќ]J€XЭ[™ЧЩ[ќ]NВ‚€Щ^WЫX\ЪЧЭЫЫXЭYЩ›YЬИHВ‚€[ќЬXЪX[Ъ[™^В‚€YЉXXЭ[™ЧЬ^Y\€XXЭ[™ЧЬ^Y\‹O™[ќ
HВ€™]\›€В€B‚€XЭ[™ЧЩ[ќ]HHXЭ[™ЧЬ^Y\‹O™[ќВ‚€›ЬЉЬXЪX[Ъ[™^HВ€ЬXЪX[Ъ[™^XЭ[™ЧЩ[ќ]KO›[Щ[]KњЬXЪX[ЧЫШYYВ€ЬXЪX[Ъ[™^
ККHВ‚€ЫЫњЭЧШЫЫJ€ЬXЪX[ШЫЫ[X[™B€	XЭ[™ЧЩ[ќ]KO›[Щ[]KњЬXЪX[ЬЬXЪX[Ъ[™^NВ‚€[ќЭ\Ъ[™^В‚€YЉЬXЪX[ШЫЫ[X[™OњЭ\ИH€ЬXЪX[ШЫЫ[X[™OњЭ\И€PVФФPТPSТS”UКHВ€ЫЫќ[ќYNВ€B‚€›ЬЉЭ\Ъ[™^HВ€Э\Ъ[™^ЬXЪX[ШЫЫ[X[™OњЭ\ОВ€Э\Ъ[™^
ККHВ‚€ЫЫњЭЧШЫЫ[X[™Ъ[њ]ЬЭ\
€[њ]ЬЭ\B€	њЬXЪX[ШЫЫ[X[™Oљ[њ]ЬЭ\Ъ[™^NВ‚€Щ^WЫX\ЪЧЭШШ[—ЫX\ЪИB€[њ]ЬЭ\OљЫЭљYЩЩ\€	€[Щ›YЬОВ‚€Ъ[JШШ[—ЫX\ЪКHВ€ЫЫњЭZ[ќЌЭ[њ]Ъ[™^B€љ]X\ЪНЌЩЩ]ЫЭЩ\ЭЪ[™^
ШШ[—ЫX\ЪКNВ‚€ЫЫњЭЩ^WЫX\ЪЧЭ[њ]Щ›YИB€љ]X\ЪНЌЩњ›ЫWЪ[™^
[њ]Ъ[™^
NВ‚€ЫЫњЭZ[ќЌЭЫЬЭ\ќЭ[YHB€XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]ЪЫЬЭ\ќЭ[YVВ€[њ]Ъ[™^€NВ‚€ЫЫњЭ›ЫЫ[™XYWЩ[Z]YЭ\ЧЭXЪИB€
XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]ЪЫЭљYЩЩ\—Э[Y€	€[њ]Щ›YКB€	‰€XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]ЪЫЭљYЩЩ\—Э[YVВ€[њ]Ъ[™^€HOH]™[ќЭ[YNВ‚€YЉ
XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]ЪЫЬЭ\ќЭ[Y€	€[њ]Щ›YКB€	‰€ЫЬЭ\ќЭ[YHH]™[ќЭ[YB€	‰€]™[ќЭ[YHHЫЬЭ\ќЭ[YB€OH[њ]ЬЭ\OљЫЭ[YB€	‰€X[™XYWЩ[Z]YЭ\ЧЭXЪКHВ‚€ЫЫXЭYЩ›YЬИH[њ]Щ›YОВ€B‚€ШШ[—ЫX\ЪИ	ЏHљ[њ]Щ›YОВ€B€B€B‚€К‚€
€™XЫЬ™[Z]Y›YЬИЫ›HYќ\€ШШ[›љ[™И]™\ћB€
€ЫЫ[X[™€ЫЫ[X[™ИЪ\љ[™ИHШ[YH[њ][™€
€™\ЪЫ[X™\][HЪ\™HЫ™H\ЭЬћHYЩK‚€
‹В€В€Щ^WЫX\ЪЧЭШШ[—ЫX\ЪИHЫЫXЭYЩ›YЬОВ‚€Ъ[JШШ[—ЫX\ЪКHВ€ЫЫњЭZ[ќЌЭ[њ]Ъ[™^B€љ]X\ЪНЌЩЩ]ЫЭЩ\ЭЪ[™^
ШШ[—ЫX\ЪКNВ‚€ЫЫњЭЩ^WЫX\ЪЧЭ[њ]Щ›YИB€љ]X\ЪНЌЩњ›ЫWЪ[™^
[њ]Ъ[™^
NВ‚€XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]ЪЫЭљYЩЩ\—Э[YVВ€[њ]Ъ[™^€HH]™[ќЭ[YNВ‚€XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]ЪЫЭљYЩЩ\—Э[YB€[њ]Щ›YОВ‚€ШШ[—ЫX\ЪИ	ЏHљ[њ]Щ›YОВ€B€B‚€™]\›€ЫЫXЭYЩ›YЬОВџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊЌ‹LЛLMВЉ‚Љ€\[™Ы™H]™[ќИH^Y\‰ЬИЫЫ[X[™\ЭЬћK‚Љ‹ВњЭ]XИ›ЪYЫЫ[X[™Ъ[њ]Ъ\ЭЬћWЬ\Ъ
ЧЬ^Y\Љ€XЭ[™ЧЬ^Y\‹ЫЫњЭЧШЫЫ[X[™Ъ[њ]Щ]™[ќ
€[њ]Щ]™[ќ
HВ€€XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]Ъ\ЭЬћVШXЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]Ъ[™^HH
љ[њ]Щ]™[ќВ‚€XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]Ъ[™^H
XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]Ъ[™^
ИJH	€ФPТPSТS”UТS‘VУPTТОВ‚€YЉXЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]ШЫЭ[ќPVФФPТPSТS”UКHВ€XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]ШЫЭ[ќ
КОВ€BџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊЌ‹LЛLMВЉ‚Љ€ЫЫњЭ[YHH™]Щ\Э™\ЬИYЩHњ›ЫHH^Y\‰ЬИЫЫ[X[™Љ€\ЭЬћK‚Љ‚Љ€Щ]™\[\™XЫЩY[Э™[Y[ќЫЫ[X[™ИЫЫњЭ[YHZ\‚Љ€љYЩЩ\љ[™И™\ЬЛ€HЫЫXљ[™Y]™[ќX^H[ЫИЫЫќZ[€BЉ€™[X\ЩHЬ€]]ЫX]XИЫYЩKЫИЫ›H™[[Э™HBЉ€ЫЫ\]H[ќћHЪ[€›ИЭ\€YЩH™[XZ[њЛ‚Љ‹ВњЭ]XИ›ЪYЫЫ[X[™Ъ[њ]Ъ\ЭЬћWШЫЫњЭ[YWЫ]\ЭЬ™\ЬК€ЧЬ^Y\Љ€XЭ[™ЧЬ^Y\‚ЉHВ€ЧШЫЫ[X[™Ъ[њ]Щ]™[ќ
€[њ]Щ]™[ќВ‚€Z[ќЌЭ[њ]Ъ[™^В‚€YЉXXЭ[™ЧЬ^Y\€XXЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]ШЫЭ[ќ
HВ€™]\›ЋВ€B‚€[њ]Ъ[™^B€
XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]Ъ[™^HJB€	€ФPТPSТS”UТS‘VУPTТОВ‚€[њ]Щ]™[ќB€	XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]Ъ\ЭЬћVВ€[њ]Ъ[™^€NВ‚€[њ]Щ]™[ќOњ™\ЬИHВ€[њ]Щ]™[ќOњ™\ЬЧШЪЬ™HВ‚€YЉ[њ]Щ]™[ќOљЫ[њ]Щ]™[ќOњ™[X\ЩJHВ€™]\›ЋВ€B‚€XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]Ъ[™^H[њ]Ъ[™^В‚€Y[\Щ]
[њ]Щ]™[ќЪ^™[ЩЉ
љ[њ]Щ]™[ќ
JNВ‚€XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]ШЫЭ[ќKNВџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊЌ‹LЛLMВЉ‚Љ€ЫЫњЭ[YHЩ[XЭY]]ЫX]XИ[Z[њ]›YЬИњ›ЫHBЉ€]™[ќЪXЪљYЩЩ\™YHЫЫ™љYЭ\X›HЫЫ[X[™‚Љ‚Љ€Э\€›YЬИ™[XZ[€]Z[X›HЪ[€HШ[YH\ЭЬћBЉ€[ќћH[ЫИЫЫќZ[њИH™\ЬЛ™[X\ЩKЬ€[›Э\€[Љ€YЩK€[\H›Ы‹[]\Э[ќљY\ИЭ^H[€XЩHЫИљ[™ВЉ€Ь™\€[™ЭЬ™Y]™[ќYЩ\ИИ›Э™YYЫЫ\XЭ[Ы‹‚Љ‹ВњЭ]XИ›ЪYЫЫ[X[™Ъ[њ]Ъ\ЭЬћWШЫЫњЭ[YWЪЫЭљYЩЩ\Љ€ЧЬ^Y\Љ€XЭ[™ЧЬ^Y\‹€ЫЫњЭZ[ќЌЭ]™[ќШYЩK€ЫЫњЭЩ^WЫX\ЪЧЭљYЩЩ\—Щ›YЬВЉHВ€ЧШЫЫ[X[™Ъ[њ]Щ]™[ќ
€[њ]Щ]™[ќВ‚€Z[ќЌЭ[њ]Ъ[™^В‚€YЉXXЭ[™ЧЬ^Y\‚€]љYЩЩ\—Щ›YЬВ€]™[ќШYЩHЏHXЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]ШЫЭ[ќ
HВ€™]\›ЋВ€B‚€[њ]Ъ[™^B€
XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]Ъ[™^H]™[ќШYЩHHJB€	€ФPТPSТS”UТS‘VУPTТОВ‚€[њ]Щ]™[ќB€	XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]Ъ\ЭЬћVЪ[њ]Ъ[™^NВ‚€[њ]Щ]™[ќOљЫ	ЏHќљYЩЩ\—Щ›YЬОВ‚€YЉ[њ]Щ]™[ќOњ™\ЬВ€[њ]Щ]™[ќOљЫ€[њ]Щ]™[ќOњ™[X\ЩB€]™[ќШYЩJHВ€™]\›ЋВ€B‚€XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]Ъ[™^H[њ]Ъ[™^В‚€Y[\Щ]
[њ]Щ]™[ќЪ^™[ЩЉ
љ[њ]Щ]™[ќ
JNВ‚€XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]ШЫЭ[ќKNВџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊЌ‹LЛLMВЉ‚Љ€™XYH™XЩY[™И[Y]™[ќњ›ЫHH^Y\‰ЬВЉ€ЫЫ[X[™\ЭЬћK‚Љ‹ВњЭ]XИЫЫњЭЧШЫЫ[X[™Ъ[њ]Щ]™[ќ
€ЫЫ[X[™Ъ[њ]Ъ\ЭЬћWЬ™]љ[Э\К€ЫЫњЭЧЬ^Y\Љ€XЭ[™ЧЬ^Y\‹€Z[ќЌЭ
€\ЭЬћWЪ[™^€Z[ќЌЭ
€™[XZ[љ[™ЧЩ]™[ќВЉHВ€YЉJњ™[XZ[љ[™ЧЩ]™[ќКHВ€™]\›€•SВ€B‚€
љ\ЭЬћWЪ[™^B€

љ\ЭЬћWЪ[™^HJH	€ФPТPSТS”UТS‘VУPTТОВ‚€

њ™[XZ[љ[™ЧЩ]™[ќКKKNВ‚€™]\›€	XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]Ъ\ЭЬћVВ€
љ\ЭЬћWЪ[™^€NВџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊЌ‹LЛLMВЉ‚Љ€™XYH™XЩY[™И™\ЬИ]™[ќњ›ЫHH^Y\‰ЬВЉ€ЫЫ[X[™\ЭЬћK‚Љ‚Љ€›Ы‹\™\ЬИ[ќљY\И\™HЪЪ\YЫИ™[X\ЩH[™[YYЉ€Ы]™[ќИШ[€™HYYЪ]Э]Ъ[™Ъ[™ИYШXЮBЉ€\™XЫЩYЫЫ[X[™™Z]љ[Ь‹‚Љ‹ВњЭ]XИЫЫњЭЧШЫЫ[X[™Ъ[њ]Щ]™[ќ
€ЫЫ[X[™Ъ[њ]Ъ\ЭЬћWЬ™]љ[Э\ЧЬ™\ЬК€ЫЫњЭЧЬ^Y\Љ€XЭ[™ЧЬ^Y\‹€Z[ќЌЭ
€\ЭЬћWЪ[™^€Z[ќЌЭ
€™[XZ[љ[™ЧЩ]™[ќВЉHВ€ЫЫњЭЧШЫЫ[X[™Ъ[њ]Щ]™[ќ
€[њ]Щ]™[ќВ‚€Ъ[J
[њ]Щ]™[ќHЫЫ[X[™Ъ[њ]Ъ\ЭЬћWЬ™]љ[Э\К€XЭ[™ЧЬ^Y\‹€\ЭЬћWЪ[™^€™[XZ[љ[™ЧЩ]™[ќВ€
JJHВ‚€YЉ[њ]Щ]™[ќOњ™\ЬКHВ€™]\›€[њ]Щ]™[ќВ€B€B‚€™]\›€•SВџB‚‹К‚Љ€ЪXЪИЪ]\€H^Y\‰ЬИ[ЬЭ™XЩ[ќ™\ЬВЉ€]™[ќИШ]\ЩћHH\™XЫЩYЫЫ[X[™Щ\]Y[ЩK‚Љ‚Љ€XXЪЩ\]Y[ЩH[ќћH\ИHљ]X\ЪИЩ€™\]Z\™YЉ€[њ]Л€H™XЫЬ™Y™\ЬИX^HЫЫќZ[€Y][Ы[Љ€›YЬИ[™Э[X]Ъ‚Љ‚Љ€™[X\ЩH[™[YYЫ]™[ќИ\™H[X™\][BЉ€YЫ›Ь™YЫИ^\Э[™И\™XЫЩYЫЫ[X[™И™]Z[‚Љ€Z\€ЬЪ]]™KYYЩH™Z]љ[Ь‹‚Љ‹ВњЭ]XИ›ЫЫX]ЪШЫЫX›КЫЫњЭЩ^WЫX\ЪЧЭЩ\]Y[ЩVЧKЫЫњЭЧЬ^Y\Љ€XЭ[™ЧЬ^Y\‹ЫЫњЭZ[ќЌЭ[™Э
HВ€ЫЫњЭЧШЫЫ[X[™Ъ[њ]Щ]™[ќ
€[њ]Щ]™[ќВ‚€Z[ќЌЭ\ЭЬћWЪ[™^В€Z[ќЌЭ™]Щ\—Щ]™[ќЭ[YNВ€Z[ќЌЭ™[XZ[љ[™ЧЩ]™[ќОВ€Z[ќЌЭЩ\]Y[ЩWЪ[™^В‚€YЉ\Щ\]Y[ЩB€XXЭ[™ЧЬ^Y\‚€[[™Э€[™Э€PVФФPТPSТS”UКHВ€™]\›€[ЩNВ€B‚€\ЭЬћWЪ[™^HXЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]Ъ[™^В€™]Щ\—Щ]™[ќЭ[YHHЭ[YNВ€™[XZ[љ[™ЧЩ]™[ќИHXЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]ШЫЭ[ќВ‚€›ЬЉЩ\]Y[ЩWЪ[™^H[™ЭВ€Щ\]Y[ЩWЪ[™^€В€Щ\]Y[ЩWЪ[™^KJHВ‚€ЫЫњЭЩ^WЫX\ЪЧЭ™\]Z\™YЬ™\ЬИB€Щ\]Y[ЩVЬЩ\]Y[ЩWЪ[™^HWNВ‚€[њ]Щ]™[ќHЫЫ[X[™Ъ[њ]Ъ\ЭЬћWЬ™]љ[Э\ЧЬ™\ЬК€XЭ[™ЧЬ^Y\‹€	љ\ЭЬћWЪ[™^€	њ™[XZ[љ[™ЧЩ]™[ќВ€
NВ‚€YЉZ[њ]Щ]™[ќ€
[њ]Щ]™[ќOњ™\ЬИ	€™\]Z\™YЬ™\ЬКB€OH™\]Z\™YЬ™\ЬВ€™]Щ\—Щ]™[ќЭ[YHH[њ]Щ]™[ќOќ[YB€€ЫШ[ШЫЫ™љYЛЫЫ[X[™Э[YJHВ€™]\›€[ЩNВ€B‚€™]Щ\—Щ]™[ќЭ[YHH[њ]Щ]™[ќOќ[YNВ€B‚€™]\›€ќYNВџB‚‹К‚Љ€™]\›€HЬXЩH[YH\ЩYћHЫ™HЫЫ™љYЭ\X›HЫЫ[X[™Љ€Щ\]Y[ЩK€ЫЫ[X[™ИЪ]Э][€^XЪ]Э™\њљYH[љ\љ]Љ€H[™Ъ[™K]ЪYHЩ][™И]X]Ъ[YK‚Љ‹ВњЭ]XИZ[ќЌЭЫЫ[X[™ЬЩ\]Y[ЩWЩЬXЩWЭ[YWЩЩ]
€ЫЫњЭЧШЫЫJ€ЬXЪX[ШЫЫ[X[™ЉHВ€\ЬЩ\ќ
ЬXЪX[ШЫЫ[X[™
NВ‚€™]\›€ЬXЪX[ШЫЫ[X[™OњЩ\]Y[ЩWЩЬXЩWЭ[YWЫЭ™\њљYB€ИЬXЪX[ШЫЫ[X[™OњЩ\]Y[ЩWЩЬXЩWЭ[YB€€ЫШ[ШЫЫ™љYЛЫЫ[X[™Э[YNВџB‚‹К‚Љ€™]\›€HЫ™Щ\ЭЬXЩH[YH™YYYћHHXЭ[™ВЉ€[ќ]IЬИЫЫ™љYЭ\X›H[™\™XЫЩYЫЫ[X[™Л‚Љ‚Љ€[њ]\ЭЬћH\ИЪ\™YћH]™\ћHЫЫ[X[™€™]Z[љ[™И]Љ€›Ь€HЫ™Щ\ЭЫЫ™љYЭ\™YЬXЩH™]™[ќИHЫЫ[X[™Љ€Ъ]HЫ™Щ\€ШШ[Э™\њљYHњ›ЫHЬЪ[™И]ИX\›Y\‚Љ€Э\ИИHЫШ[\ЭЬћH[Y[Э]‚Љ‹ВњЭ]XИZ[ќЌЭЫЫ[X[™Ъ[њ]Ъ\ЭЬћWЩЬXЩWЭ[YWЩЩ]
€ЫЫњЭ[ќ]J€XЭ[™ЧЩ[ќ]BЉHВ€Z[ќЌЭ\ЭЬћWЩЬXЩWЭ[YHHЫШ[ШЫЫ™љYЛЫЫ[X[™Э[YNВ‚€[ќЬXЪX[Ъ[™^В‚€YЉXXЭ[™ЧЩ[ќ]JHВ€™]\›€\ЭЬћWЩЬXЩWЭ[YNВ€B‚€›ЬЉЬXЪX[Ъ[™^HВ€ЬXЪX[Ъ[™^XЭ[™ЧЩ[ќ]KO›[Щ[]KњЬXЪX[ЧЫШYYВ€ЬXЪX[Ъ[™^
ККHВ‚€ЫЫњЭЧШЫЫJ€ЬXЪX[ШЫЫ[X[™B€	XЭ[™ЧЩ[ќ]KO›[Щ[]KњЬXЪX[ЬЬXЪX[Ъ[™^NВ‚€ЫЫњЭZ[ќЌЭЩ\]Y[ЩWЩЬXЩWЭ[YHB€ЫЫ[X[™ЬЩ\]Y[ЩWЩЬXЩWЭ[YWЩЩ]
ЬXЪX[ШЫЫ[X[™
NВ‚€YЉЩ\]Y[ЩWЩЬXЩWЭ[YH€\ЭЬћWЩЬXЩWЭ[YJHВ€\ЭЬћWЩЬXЩWЭ[YHHЩ\]Y[ЩWЩЬXЩWЭ[YNВ€B€B‚€™]\›€\ЭЬћWЩЬXЩWЭ[YNВџB‚‹К‚Љ€™]\›€HXњЫЫ]HЩЪXШ[XЪИЪ[€Ъ\™YЫЫ[X[™Љ€\ЭЬћH^\™\Л€Ш]\][Ы€™\Щ\ќ™\И^™[Y[H\™ЩBЉ€Ь™X]Ь‹YYљ[™YЬXЩH[Y\ИЪ]Э]Ь\[™ИBЉ€^\][Ы€[YH\›Э[™ИH™YЪ[›љ[™ИЩ€HЫШЪЛ‚Љ‹ВњЭ]XИZ[ќЌЭЫЫ[X[™Ъ[њ]Ъ\ЭЬћWЩ^\][Ы—Э[YWЩЩ]
€ЫЫњЭ[ќ]J€XЭ[™ЧЩ[ќ]K€ЫЫњЭZ[ќЌЭ]™[ќЭ[YBЉHВ€ЫЫњЭZ[ќЌЭ\ЭЬћWЩЬXЩWЭ[YHB€ЫЫ[X[™Ъ[њ]Ъ\ЭЬћWЩЬXЩWЭ[YWЩЩ]
XЭ[™ЧЩ[ќ]JNВ‚€YЉ\ЭЬћWЩЬXЩWЭ[YH€RS•ЌУPVH]™[ќЭ[YJHВ€™]\›€RS•ЌУPVВ€B‚€™]\›€]™[ќЭ[YH
И\ЭЬћWЩЬXЩWЭ[YNВџB‚‹К‚Љ€™]\›€Ъ]\€[€[њ]]™[ќШ]\ЩљY\ИHЬЪ]]™BЉ€YЩH™\]Z\™[Y[ќ›Ь€Ы™HЫЫ™љYЭ\X›HЫЫ[X[™Э\‚Љ‚Љ€Ъ[™ЫKZЩ^HЭ\И™]Z[€^XЭYШXЮH™Z]љ[Ь€[™\ЩBЉ€Ы›HH\ЪXШ[™\ЬИYЩK€][KZЩ^HЭ\И\ЩHBЉ€ЪЬ™Ы\ЪЭ[™Z\€ЫЫ™љYЭ\™YЪЬ™Э[YK€™\›ВЉ€™\]Z\™\И]™\ћHY[X™\€И™YЪ[€Ы€HШ[YHЩЪXШ[XЪЛ‚Љ€]X\ЭЫ™H™\]Z\™YЩ^H]\Э™H\ЪXШ[H™\ЬЩY[‚Љ€HљYЩЩ\љ[™И]™[ќ™]™[ќ[™И[€[њ™[]Y\™Щ^BЉ€њ›ЫH™]љYЩЩ\љ[™ИHЪЬ™ЪXЪY\™[H™[XZ[њИ[‚Љ‹ВњЭ]XИ›ЫЫЫЫ[X[™Ъ[њ]Щ]™[ќЫX]Ъ\ЧЬ™\ЬК€ЫЫњЭЧШЫЫ[X[™Ъ[њ]ЬЭ\
€[њ]ЬЭ\€ЫЫњЭЧШЫЫ[X[™Ъ[њ]Щ]™[ќ
€[њ]Щ]™[ќ€ЫЫњЭЧЬ^Y\Љ€XЭ[™ЧЬ^Y\‚ЉHВ€ЫЫњЭЩ^WЫX\ЪЧЭ™\]Z\™YЬ™\ЬИH[њ]ЬЭ\Oњ™\ЬОВ‚€Щ^WЫX\ЪЧЭШШ[—ЫX\ЪОВ‚€YЉ\™\]Z\™YЬ™\ЬКHВ€™]\›€ќYNВ€B‚€К‚€
€HX\ЪИЪ]Ы™HXЭ]™Hљ]\И›ИЩXЫЫ™љ]Yќ\‚€
€™[[Эљ[™И]ИЭЩ\Эљ]‚€
‹В€YЉJ™\]Z\™YЬ™\ЬИ	€
™\]Z\™YЬ™\ЬИHJJJHВ€™]\›€
[њ]Щ]™[ќOњ™\ЬИ	€™\]Z\™YЬ™\ЬКB€OH™\]Z\™YЬ™\ЬОВ€B‚€YЉJ[њ]Щ]™[ќOњ™\ЬИ	€™\]Z\™YЬ™\ЬКB€
[њ]Щ]™[ќOњ™\ЬЧШЪЬ™	€™\]Z\™YЬ™\ЬКB€OH™\]Z\™YЬ™\ЬКHВ€™]\›€[ЩNВ€B‚€ШШ[—ЫX\ЪИH™\]Z\™YЬ™\ЬОВ‚€Ъ[JШШ[—ЫX\ЪКHВ€ЫЫњЭZ[ќЌЭ[њ]Ъ[™^B€љ]X\ЪНЌЩЩ]ЫЭЩ\ЭЪ[™^
ШШ[—ЫX\ЪКNВ‚€ЫЫњЭЩ^WЫX\ЪЧЭ[њ]Щ›YИB€љ]X\ЪНЌЩњ›ЫWЪ[™^
[њ]Ъ[™^
NВ‚€ЫЫњЭZ[ќЌЭ™\ЬЧЭ[YHB€XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]ЪЫЬЭ\ќЭ[YVВ€[њ]Ъ[™^€NВ‚€YЉJXЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]ЪЫЬЭ\ќЭ[Y€	€[њ]Щ›YКB€™\ЬЧЭ[YH€[њ]Щ]™[ќOќ[YB€[њ]Щ]™[ќOќ[YHH™\ЬЧЭ[YB€€[њ]ЬЭ\OЪЬ™Э[YJHВ€™]\›€[ЩNВ€B‚€ШШ[—ЫX\ЪИ	ЏHљ[њ]Щ›YОВ€B‚€™]\›€ќYNВџB‚‹К‚Љ€™]\›€Ъ]\€[€[њ]]™[ќШ]\ЩљY\И]™\ћH\ЬЪ]™BЉ€[™]]ЫX]XИ[™\]Z\™[Y[ќ›Ь€Ы™HЫЫ[X[™Э\‚Љ‹ВњЭ]XИ›ЫЫЫЫ[X[™Ъ[њ]Щ]™[ќЫX]Ъ\ЧЪЫ
€ЫЫњЭЧШЫЫ[X[™Ъ[њ]ЬЭ\
€[њ]ЬЭ\€ЫЫњЭЧШЫЫ[X[™Ъ[њ]Щ]™[ќ
€[њ]Щ]™[ќ€ЫЫњЭЧЬ^Y\Љ€XЭ[™ЧЬ^Y\‚ЉHВ€ЫЫњЭЩ^WЫX\ЪЧЭ™\]Z\™YЪЫЩ›YЬИB€[њ]ЬЭ\OљЫ[њ]ЬЭ\OљЫЭљYЩЩ\ЋВ‚€Щ^WЫX\ЪЧЭШШ[—ЫX\ЪОВ‚€YЉ\™\]Z\™YЪЫЩ›YЬКHВ€™]\›€ќYNВ€B‚€YЉ
[њ]Щ]™[ќOљ[	€™\]Z\™YЪЫЩ›YЬКB€OH™\]Z\™YЪЫЩ›YЬКHВ€™]\›€[ЩNВ€B‚€ШШ[—ЫX\ЪИH™\]Z\™YЪЫЩ›YЬОВ‚€Ъ[JШШ[—ЫX\ЪКHВ€ЫЫњЭZ[ќЌЭ[њ]Ъ[™^B€љ]X\ЪНЌЩЩ]ЫЭЩ\ЭЪ[™^
ШШ[—ЫX\ЪКNВ‚€ЫЫњЭЩ^WЫX\ЪЧЭ[њ]Щ›YИB€љ]X\ЪНЌЩњ›ЫWЪ[™^
[њ]Ъ[™^
NВ‚€ЫЫњЭZ[ќЌЭЫЬЭ\ќЭ[YHB€XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]ЪЫЬЭ\ќЭ[YVВ€[њ]Ъ[™^€NВ‚€YЉJXЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]ЪЫЬЭ\ќЭ[Y€	€[њ]Щ›YКB€ЫЬЭ\ќЭ[YH€[њ]Щ]™[ќOќ[YJHВ€™]\›€[ЩNВ€B‚€В€ЫЫњЭZ[ќЌЭ[Э[YHB€[њ]Щ]™[ќOќ[YHHЫЬЭ\ќЭ[YNВ‚€К‚€
€\ЬЪ]™HЫИXШЩ\H[Ы\Ъ]™HZ[љ[][B€
€›ЭYЪHЬ[Ы[[Ы\Ъ]™HX^[][K€B€
€™\›ИX^[][HX]™\ИH\\€›Э[™Ь[‹‚€
€]]ЫX]XИЫИX]ЪЫ›HZ\€^XЭ€
€Z[љ[][H™\ЪЫYЩK™]™[ќ[™ИH]\‚€
€™\ЪЫ›Ь€HШ[YHЩ^Hњ›ЫH™]љYЩЩ\љ[™В€
€[€X\›Y\€]]ЫX]XИЫЫ[X[™‚€
‹В€YЉ

[њ]ЬЭ\OљЫ	€[њ]Щ›YКB€	‰€
[Э[YH[њ]ЬЭ\OљЫЭ[YB€
[њ]ЬЭ\OљЫЭ[YWЫX^[][B€	‰€[Э[YB€€[њ]ЬЭ\OљЫЭ[YWЫX^[][JJJB€

[њ]ЬЭ\OљЫЭљYЩЩ\€	€[њ]Щ›YКB€	‰€[Э[YHOH[њ]ЬЭ\OљЫЭ[YJJHВ€™]\›€[ЩNВ€B€B‚€ШШ[—ЫX\ЪИ	ЏHљ[њ]Щ›YОВ€B‚€™]\›€ќYNВџB‚‹К‚Љ€™XYH™XЩY[™И]™[ќЪЬЩHYЩH\H\И™[][ќЉ€ИHЫЫ™љYЭ\X›HЫЫ[X[™Э\‚Љ‚Љ€™[X\ЩK[Ы›H]™[ќИИ›Э\Э\€™\ЬЛ[Ы›HЭ\ЛЉ€[™™\ЬЛ[Ы›H]™[ќИИ›Э\Э\€™[X\ЩK[Ы›BЉ€Э\Л€[€]™[ќЩ€H™\]Z\™Y\HЪ]HЬ›Ы™ИЩ^BЉ€\И™]\›™YЫИ]ЫЬњ™XЭH[ќ\њќ\ИHЩ\]Y[ЩK‚Љ‹ВњЭ]XИЫЫњЭЧШЫЫ[X[™Ъ[њ]Щ]™[ќ
€ЫЫ[X[™Ъ[њ]Ъ\ЭЬћWЬ™]љ[Э\ЧЩ›Ь—ЬЭ\
€ЫЫњЭЧЬ^Y\Љ€XЭ[™ЧЬ^Y\‹€ЫЫњЭЧШЫЫ[X[™Ъ[њ]ЬЭ\
€[њ]ЬЭ\€Z[ќЌЭ
€\ЭЬћWЪ[™^€Z[ќЌЭ
€™[XZ[љ[™ЧЩ]™[ќВЉHВ€ЫЫњЭЧШЫЫ[X[™Ъ[њ]Щ]™[ќ
€[њ]Щ]™[ќВ‚€Ъ[J
[њ]Щ]™[ќHЫЫ[X[™Ъ[њ]Ъ\ЭЬћWЬ™]љ[Э\К€XЭ[™ЧЬ^Y\‹€\ЭЬћWЪ[™^€™[XZ[љ[™ЧЩ]™[ќВ€
JJHВ‚€YЉ
[њ]ЬЭ\Oњ™\ЬИ	‰€[њ]Щ]™[ќOњ™\ЬКB€
[њ]ЬЭ\Oњ™[X\ЩH	‰€[њ]Щ]™[ќOњ™[X\ЩJB€
[њ]ЬЭ\OљЫЭљYЩЩ\€	‰€[њ]Щ]™[ќOљЫ
JHВ€™]\›€[њ]Щ]™[ќВ€B€B‚€™]\›€•SВџB‚‹К‚Љ€ЪXЪИЪ]\€H^Y\‰ЬИ[ЬЭ™XЩ[ќ[њ]]™[ќВЉ€Ш]\ЩћHHЫЫ™љYЭ\X›HЬXЪX[ЫЫ[X[™‚Љ‚Љ€™\ЬЛ™[X\ЩK[™]]ЫX]XИЫX\ЪЬИ\™HYЩBЉ€™\]Z\™[Y[ќЛ€\ЬЪ]™HЫ\ИHЭ]H[™Z[љ[][BЉ€\][Ы€™\]Z\™[Y[ќ][X]Y]HљYЩЩ\љ[™И]™[ќ‚Љ‹ВњЭ]XИ›ЫЫX]ЪЬЬXЪX[ШЫЫ[X[™
€ЫЫњЭЧШЫЫ[X[™Ъ[њ]ЬЭ\Щ\]Y[ЩVЧK€ЫЫњЭЧЬ^Y\Љ€XЭ[™ЧЬ^Y\‹€ЫЫњЭZ[ќЌЭ[™Э€ЫЫњЭZ[ќЌЭЩ\]Y[ЩWЩЬXЩWЭ[YK€Z[ќЌЭ
€X]ЪЭ[YK€Z[ќЌЭ
€X]ЪШYЩBЉHВ€ЫЫњЭЧШЫЫ[X[™Ъ[њ]Щ]™[ќ
€[њ]Щ]™[ќВ‚€Z[ќЌЭ\ЭЬћWЪ[™^В€Z[ќЌЭ™]Щ\—Щ]™[ќЭ[YNВ€Z[ќЌЭ™[XZ[љ[™ЧЩ]™[ќОВ€Z[ќЌЭЩ\]Y[ЩWЪ[™^В‚€YЉ\Щ\]Y[ЩB€XXЭ[™ЧЬ^Y\‚€[X]ЪЭ[YB€[X]ЪШYЩB€[[™Э€[™Э€PVФФPТPSТS”UКHВ€™]\›€[ЩNВ€B‚€
›X]ЪЭ[YHHВ€
›X]ЪШYЩHHRS•ЌУPVВ‚€\ЭЬћWЪ[™^HXЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]Ъ[™^В€™]Щ\—Щ]™[ќЭ[YHHЭ[YNВ€™[XZ[љ[™ЧЩ]™[ќИHXЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]ШЫЭ[ќВ‚€›ЬЉЩ\]Y[ЩWЪ[™^H[™ЭВ€Щ\]Y[ЩWЪ[™^€В€Щ\]Y[ЩWЪ[™^KJHВ‚€ЫЫњЭЧШЫЫ[X[™Ъ[њ]ЬЭ\
€[њ]ЬЭ\B€	њЩ\]Y[ЩVЬЩ\]Y[ЩWЪ[™^HWNВ‚€[њ]Щ]™[ќHЫЫ[X[™Ъ[њ]Ъ\ЭЬћWЬ™]љ[Э\ЧЩ›Ь—ЬЭ\
€XЭ[™ЧЬ^Y\‹€[њ]ЬЭ\€	љ\ЭЬћWЪ[™^€	њ™[XZ[љ[™ЧЩ]™[ќВ€
NВ‚€YЉZ[њ]Щ]™[ќ€XЫЫ[X[™Ъ[њ]Щ]™[ќЫX]Ъ\ЧЬ™\ЬК€[њ]ЬЭ\€[њ]Щ]™[ќ€XЭ[™ЧЬ^Y\‚€
B€
[њ]Щ]™[ќOњ™[X\ЩH	€[њ]ЬЭ\Oњ™[X\ЩJB€OH[њ]ЬЭ\Oњ™[X\ЩB€
[њ]Щ]™[ќOљЫ	€[њ]ЬЭ\OљЫЭљYЩЩ\ЉB€OH[њ]ЬЭ\OљЫЭљYЩЩ\‚€™]Щ\—Щ]™[ќЭ[YHH[њ]Щ]™[ќOќ[YB€€Щ\]Y[ЩWЩЬXЩWЭ[YB€XЫЫ[X[™Ъ[њ]Щ]™[ќЫX]Ъ\ЧЪЫ
€[њ]ЬЭ\€[њ]Щ]™[ќ€XЭ[™ЧЬ^Y\‚€
JHВ€™]\›€[ЩNВ€B‚€YЉЩ\]Y[ЩWЪ[™^OH[™Э
HВ€
›X]ЪЭ[YHH[њ]Щ]™[ќOќ[YNВ€
›X]ЪШYЩHB€XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]ШЫЭ[ќ€H™[XZ[љ[™ЧЩ]™[ќИHNВ€B‚€™]Щ\—Щ]™[ќЭ[YHH[њ]Щ]™[ќOќ[YNВ€B‚€™]\›€ќYNВџB‚›ЫЫЪXЪЧШЫЫX›КЧЬ^Y\Љ€XЭ[™ЧЬ^Y\ЉHВ‚€[њЪYЫ™Y[ќNВ€[ќX^Э\HLNВ€[ќX^Щ^\ИHLNВ€[ќ[YHLNВ‚€Z[ќЌЭZ[—ЫX]ЪШYЩHHRS•ЌУPVВ€Z[ќЌЭX^ЫX]ЪЭ[YHHВ€Щ^WЫX\ЪЧЭЩ[XЭYЪЫЭљYЩЩ\—Щ›YЬИHВ‚€›ЫЫX]ЪЩ›Э[™H[ЩNВ‚€ЧШЫЫH
ЫЫNВ‚€›ЬЉHHИHЩ[‹O›[Щ[]KњЬXЪX[ЧЫШYYИJККHВ‚€ЫЫHHЩ[‹O›[Щ[]KњЬXЪX[
ИNВ‚€YЉЩ[‹O[љ[X][Ы‹OШ[Щ[OHS’SPUSУ—РРSђСSСTРP“Q	‰‚€
Щ[‹O[љ[[ќ[HOHЫЫKOШ[Щ[€ЫЫKO™њ[YK›Z[€€Щ[‹O[љ[\ЬИ€ЫЫKO™њ[YK›X^Щ[‹O[љ[\ЬИ€Щ[‹O[љ[X][Ы‹Oљ]ШЫЭ[ќЫЫKOљ]КJHВ€€ЫЫќ[ќYNВ€€H[ЩHYЉЩ[‹O[љ[X][Ы‹OШ[Щ[OHS’SPUSУ—РРSђСSСTРP“Q€	‰€
ЫЫKOШ[Щ[\^Y\—ШXШЩ\ЧЪYWЪ[њ]
Щ[ЉHY™ЉЩ[‹OњЬЪ][Ы‹ћKЩ[‹O\ЩJH€JJHВ€€ЫЫќ[ќYNВ€B‚€В€Z[ќЌЭЫЫ[X[™ЫX]ЪШYЩNВ€Z[ќЌЭЫЫ[X[™ЫX]ЪЭ[YNВ‚€ЫЫњЭZ[ќЌЭЩ\]Y[ЩWЩЬXЩWЭ[YHB€ЫЫ[X[™ЬЩ\]Y[ЩWЩЬXЩWЭ[YWЩЩ]
ЫЫJNВ‚€YЉ[Y[љ[JЩ[‹ЫЫKO[љ[JB€	‰€
ЪXЪЧЩ[™\™ЮJS‘T‘ЦWХTWУTЫЫKO[љ[JHЪXЪЧЩ[™\™ЮJS‘T‘ЦWХTWТЫЫKO[љ[JJB€	‰€X]ЪЬЬXЪX[ШЫЫ[X[™
€ЫЫKOљ[њ]€XЭ[™ЧЬ^Y\‹€ЫЫKOњЭ\Л€Щ\]Y[ЩWЩЬXЩWЭ[YK€	ЫЫ[X[™ЫX]ЪЭ[YK€	ЫЫ[X[™ЫX]ЪШYЩB€
JHВ‚€К‚€
€™Y™\€HЫЫ[X[™љYЩЩ\™YћHH™]Щ\Э€
€[њ]]™[ќ€љ[™ИYЩH™\ЫЫ™\И]™[ќИ™XЫЬ™Y€
€\љ[™ИHШ[YHЩЪXШ[XЪЛ€ЫЫ[X[™ИЪ\љ[™В€
€]]™[ќ™]Z[€H^\Э[™ИЫ™Щ\Э\Щ\]Y[ЩK€
€[€Ь™X]\ЭXЪЬ™\Ъ^™H[љЪ[™Л‚€
‹В€ЫЫњЭ›ЫЫ™]\—ЫX]ЪB€[X]ЪЩ›Э[™€ЫЫ[X[™ЫX]ЪШYЩHZ[—ЫX]ЪШYЩB€
ЫЫ[X[™ЫX]ЪШYЩHOHZ[—ЫX]ЪШYЩB€	‰€
ЫЫ[X[™ЫX]ЪЭ[YH€X^ЫX]ЪЭ[YB€
ЫЫ[X[™ЫX]ЪЭ[YHOHX^ЫX]ЪЭ[YB€	‰€
ЫЫKOњЭ\И€X^Э\€
ЫЫKOњЭ\ИOHX^Э\€	‰€ЫЫKO›ќ[ZЩ^\И€X^Щ^\КJJJJNВ‚€YЉX™]\—ЫX]Ъ
HВ€ЫЫќ[ќYNВ€B‚€[YHЫЫKO[љ[NВ€Z[—ЫX]ЪШYЩHHЫЫ[X[™ЫX]ЪШYЩNВ€X^ЫX]ЪЭ[YHHЫЫ[X[™ЫX]ЪЭ[YNВ€X^Э\HЫЫKOњЭ\ОВ€X^Щ^\ИHЫЫKO›ќ[ZЩ^\ОВ€Щ[XЭYЪЫЭљYЩЩ\—Щ›YЬИB€ЫЫKOљ[њ]ШЫЫKOњЭ\ИHWKљЫЭљYЩЩ\ЋВ€X]ЪЩ›Э[™HќYNВ€B€B‚€B‚€YЉ[YЏH	‰€ЪXЪЧШЫЬЭ[Э™J[YKЩ[‹Oљќ[\[™КJHВ€К‚€
€]]ЫX]XИ[YЩ\И\™HЫ™K\ЪЭ[њ]]™[ќЛ‚€
€ЫЫњЭ[YHHЩ[XЭYљ[[\Э\›YЬИЫ›HYќ\‚€
€H[Э™HЭ\ќИЭXШЩ\ЬЩќ[K€™\ЬЛ™[X\ЩK€
€[™[њ™[]Y]]ЫX]XИ›YЬИ™[XZ[€]Z[X›K‚€
‹В€ЫЫ[X[™Ъ[њ]Ъ\ЭЬћWШЫЫњЭ[YWЪЫЭљYЩЩ\Љ€XЭ[™ЧЬ^Y\‹€Z[—ЫX]ЪШYЩK€Щ[XЭYЪЫЭљYЩЩ\—Щ›YЬВ€
NВ‚€™]\›€ќYNВ€B‚€™]\›€[ЩNВџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊЌ‹LЛLЌ€HЬљYЪ[[HЬљ][€ћHќYЭYH
[љЫ›ЭЫ€]JK‚Љ‚Љ€™]ЫЬљИ][X]\ИЪ]\€H^Y\€Љ€\ИH[™[™ИЫЫ[X[™Щ\]Y[ЩHИ^XЭ]K‚Љ‹В›ЫЫ^Y\—Ь™Z[њ]

HВ€€ЧЬ^Y\€
XЭ[™ЧЬ^Y\€H^Y\€
ИЩ[‹Oњ^Y\љ[™^В‚€YЉЪXЪЧШЫЫX›КXЭ[™ЧЬ^Y\ЉJHВ€XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	ЏH‘“QЧРУУ•“УСVTОВ‚€™]\›€ќYNВ€B‚€™]\›€[ЩNВџB‚‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊЊЛLL‹LЋHЉ€Љ€ЭЬќ[›љ[™ИY€^Y\‚Љ€›ИЫ™Щ\€[њ]И\™XЭ[Ы‚Љ€]Э\ќYHќ[‹‚Љ‹Вќ›ЪYќ[—ЭћWЬќ[њЭЬШЪXЪК[ќ]J€XЭ[™ЧЩ[ќ]KЫЫњЭWФќ[–\™XЭ[Ы€[Э™^ЫЫњЭWФќ[–‘\™XЭ[Ы€[Э™^‹ЫЫњЭWФќ[–\™XЭ[Ы€ќ[›љ[™ЧЮЫЫњЭWФќ[–‘\™XЭ[Ы€ќ[›љ[™ЧЮ‹ЫЫњЭ[ќќ[ђЫЫ™љYС›YЬЛЫЫњЭ[ќ\ЪЫЫ[X[™›YЛЫЫњЭ[ќ\Ъљ^Y›YЛЫЫњЭ[ќ[X›Y›YЛЫЫњЭ[ќЭЬЭ]Q›YКHВ€Y€
Jќ[ђЫЫ™љYС›YЬИ	€[X›Y›YКH€
\Ъљ^Y›YИ	‰€XXЭ[™ЧЩ[ќ]KO[љ[X][™КB€

\ЪЫЫ[X[™›YИ	‰€XXЭ[™ЧЩ[ќ]KO[љ[X][™КB€
Y™Љ[Э™^ќ[›љ[™ЧЮ
H	‰€
[Э™^€OH•S—СT—Ц—У“У‘HXЭ[™ЧЩ[ќ]KOњќ[›љ[™И	€•S—ФХUWФХT•Ц
JB€
Y™Љ[Э™^‹ќ[›љ[™ЧЮЉH	‰€
[Э™^OH•S—СT—ЦУ“У‘HXЭ[™ЧЩ[ќ]KOњќ[›љ[™И	€•S—ФХUWФХT•ЦЉJJJHВ€XЭ[™ЧЩ[ќ]KOњќ[›љ[™ИHЭЬЭ]Q›YОВ€BџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊЊЛLL‹LЋBЉ€Љ€ЪXЪИ^Y\€[њ][™ќ[€ЭЬЉ€ќ[Э[Ы€Y€™YYY‚Љ‹Вќ›ЪYќ[—ЭћWЬќ[њЭЬЬ^Y\Љ[ќ]J€XЭ[™ЧЩ[ќ]KЫЫњЭЧЬ^Y\Љ€XЭ[™ЧЬ^Y\ЉBћИ€ЫЫњЭWЬќ[—ЬЭ]Hќ[—ЬЭ]HHXЭ[™ЧЩ[ќ]KOњќ[›љ[™ОВ‚€Y€
ќ[—ЬЭ]HOH•S—ФХUWУ“У‘JB€В€™]\›ЋВ€B‚€К‚€
€Щ]^Y\€\™XЭ[Ы[[њ]€
€њ›ЫHЩ^HЭ]\Л‚€
‹В‚€ЫЫњЭWФќ[–‘\™XЭ[Ы€[Э™^€H
XЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧУSХ‘UT
HИ•S—СT—Ц—ХT€
XЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧУSХ‘QХУЉHИ•S—СT—Ц—СХУ€€•S—СT—Ц—У“У‘NВ€ЫЫњЭWФќ[–\™XЭ[Ы€[Э™^H
XЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧУSХ‘SQ•
HИ•S—СT—ЦУQ•€
XЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧУSХ‘T’QТ
HИ•S—СT—ЦФ’QТ€•S—СT—ЦУ“У‘NВ‚€К‚€
€Щ]Hќ[›љ[™И\™XЭ[Ы€њ›ЫB€
€™[ШЪ]K‚€
‹В‚€ЫЫњЭWФќ[–‘\™XЭ[Ы€ќ[›љ[™ЧЮ€Hќ[—ЬЭ]HИ

XЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€
HИ•S—СT—Ц—ХT€
XЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€€
HИ•S—СT—Ц—СХУ€€•S—СT—Ц—У“У‘JH€•S—СT—Ц—У“У‘NВ€ЫЫњЭWФќ[–\™XЭ[Ы€ќ[›љ[™ЧЮHќ[—ЬЭ]HИ

XЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ
HИ•S—СT—ЦУQ•€
XЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€
HИ•S—СT—ЦФ’QТ€•S—СT—ЦУ“У‘JH€•S—СT—ЦУ“У‘NВ‚€К€ќ\Э›Ь€™XYXљ[]H™[ЭЛ€
‹В€ЫЫњЭWЬќ[—ШЫЫ™љYЧЩ›YЬИXЭ[™ЧЬќ[—ШЫЫ™љYЧЩ›YЬИHXЭ[™ЧЩ[ќ]KO›[Щ[]Kњќ[—ШЫЫ™љYЧЩ›YЬОВ‚€К‚€
€ЭЬќ[›љ[™ПВ€
‚€
€K€^\И\™XЭ[Ы‹‚€
€‹€X›HИќ[€[€\™XЭ[ЫЏВ€
€Л€љ^Y\ЪИљ^Y\Ъ\ИЭЬ€
€Ъ[€H[љ[X][Ы€ЫЫ\]\Л€^B€
€YЫ›Ь™H™[X\Ъ[™ИHЩ^H[™И›Э€
€ЭЬЫ€H\њ[™XЭ[\€^\ИЫЫ[X[™€
€]™[€Y€ЩHШ[‰Э[Э™HЫ€HЭ\‚€
€^\Л€›ЭHHЭ\€^\И\ИYЫ›Ь™Y€
€ћHЩЪXИЭЫњЭ™X[K‚€
‚€
€€ЭЬЫ€›ЫЭЪ[™О‚€
€H›И\™XЭ[Ы€ЫЫ[X[™‚€
€H\™XЭ[Ы€ЫЫ[X[™ЬЬЩ\ИЭ\њ™[ќ\™XЭ[Ы‹‚€
€H\њ[™XЭ[\€^\ИЫЫ[X[™Ъ[€ЩHЫ‰Э]™HH›YИ[X›Y‚€
‚€
€™\X]ЪXЪИ›Ь€ЬЬЪ]H\™XЭ[Ы‚€
€[™›Ь€€^\Л‚€
‹В‚€ЭЪ]Ъ
ќ[›љ[™ЧЮ
HВ€Ш\ЩH•S—СT—ЦУQ•‚€ќ[—ЭћWЬќ[њЭЬШЪXЪКXЭ[™ЧЩ[ќ]K[Э™^[Э™^‹ќ[›љ[™ЧЮќ[›љ[™ЧЮ‹XЭ[™ЧЬќ[—ШЫЫ™љYЧЩ›YЬЛ•S—РУУ‘’QЧЦУQ•СTТРУУSPS‘•S—РУУ‘’QЧЦУQ•СTТС’VQ•S—РУУ‘’QЧЦУQ•СSђP“Q•S—ФХUWУ“У‘JNВ€њ™XZОВ‚€Ш\ЩH•S—СT—ЦФ’QТ‚€ќ[—ЭћWЬќ[њЭЬШЪXЪКXЭ[™ЧЩ[ќ]K[Э™^[Э™^‹ќ[›љ[™ЧЮќ[›љ[™ЧЮ‹XЭ[™ЧЬќ[—ШЫЫ™љYЧЩ›YЬЛ•S—РУУ‘’QЧЦФ’QТСTТРУУSPS‘•S—РУУ‘’QЧЦФ’QТСTТС’VQ•S—РУУ‘’QЧЦФ’QТСSђP“Q•S—ФХUWУ“У‘JNВ€њ™XZОВ‚€Ш\ЩH•S—СT—ЦУ“У‘N‚€њ™XZОВ€B‚€ЭЪ]Ъ
ќ[›љ[™ЧЮЉHВ€Ш\ЩH•S—СT—Ц—ХT‚€ќ[—ЭћWЬќ[њЭЬШЪXЪКXЭ[™ЧЩ[ќ]K[Э™^[Э™^‹ќ[›љ[™ЧЮќ[›љ[™ЧЮ‹XЭ[™ЧЬќ[—ШЫЫ™љYЧЩ›YЬЛ•S—РУУ‘’QЧЦ—ХTСTТРУУSPS‘•S—РУУ‘’QЧЦ—ХTСTТС’VQ•S—РУУ‘’QЧЦ—ХTСSђP“Q•S—ФХUWУ“У‘JNВ€њ™XZОВ‚€Ш\ЩH•S—СT—Ц—СХУЋ‚€ќ[—ЭћWЬќ[њЭЬШЪXЪКXЭ[™ЧЩ[ќ]K[Э™^[Э™^‹ќ[›љ[™ЧЮќ[›љ[™ЧЮ‹XЭ[™ЧЬќ[—ШЫЫ™љYЧЩ›YЬЛ•S—РУУ‘’QЧЦ—СХУ—СTТРУУSPS‘•S—РУУ‘’QЧЦ—СХУ—СTТС’VQ•S—РУУ‘’QЧЦ—СХУ—СSђP“Q•S—ФХUWУ“У‘JNВ€њ™XZОВ‚€Ш\ЩH•S—СT—Ц—У“У‘N‚€њ™XZОВ€BџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊЌ‹LЛLM‚Љ‚Љ€™]\›€ќYHЪ[€H[ќ]HX^HЉ€›ШЩ\ЬИЬ™[\ћHYK\Э]H^Y\€Љ€[њ]‚Љ‹В›ЫЫ^Y\—ШXШЩ\ЧЪYWЪ[њ]
ЫЫњЭ[ќ]H
XЭ[™ЧЩ[ќ]JHВ€€ЫЫњЭЧШ[љ[H
[љ[X][Ы€HXЭ[™ЧЩ[ќ]KO[љ[X][ЫЋВ‚€YЉXЭ[™ЧЩ[ќ]KOљY[™КHВ€™]\›€ќYNВ€B‚€YЉX[љ[X][Ы‚€X[љ[X][Ы‹OљYB€[љ[X][Ы‹O›ќ[Yњ[Y\ИH€XЭ[™ЧЩ[ќ]KO[љ[\ЬВ€ЏH
Z[ќЌЭ
X[љ[X][Ы‹O›ќ[Yњ[Y\КHВ€™]\›€[ЩNВ€B‚€™]\›€[љ[X][Ы‹OљYVШXЭ[™ЧЩ[ќ]KO[љ[\ЬЧHИќYH€[ЩNВџB‚ќ›ЪY^Y\—Э[љК
BћВ€\YY€[ќ[HWЫШШ[ШXЭ[Ы—Щ›YЬВ€В€PХSУ—У“У‘K€PХSУ—ХРSЛ€PХSУ—ХT€PХSУ—СХУ‹€PХSУ—Ф•S‚€HWЫШШ[ШXЭ[Ы—Щ›YЬОВ‚€WЫШШ[ШXЭ[Ы—Щ›YЬИXЭ[Ы€HВ€›ЫЫXЪЧЭШ[ИH[ЩNИЛШXЪЭШ[В€€[ќ]H
›Э\€H•SВ€›Ш][љ]X[Ъќ[\Э™[ШЪ]WЮ€HЊВ‚€[ќ]J€XЭ[™ЧЩ[ќ]HHЩ[ЋВ‚€ЫЫњЭЩ^WЫX\ЪЧЭЩ\]Y[ЩWЫYќЫYќЧHHС“QЧУSХ‘SQ•“QЧУSХ‘SQ•NВ€ЫЫњЭЩ^WЫX\ЪЧЭЩ\]Y[ЩWЬљYЪЬљYЪЧHHС“QЧУSХ‘T’QТ“QЧУSХ‘T’QТNВ€ЫЫњЭЩ^WЫX\ЪЧЭЩ\]Y[ЩWЭ\Э\ЧHHС“QЧУSХ‘UT“QЧУSХ‘UTNВ€ЫЫњЭЩ^WЫX\ЪЧЭЩ\]Y[ЩWЩЭЫ—ЩЭЫ–ЧHHС“QЧУSХ‘QХУ‹“QЧУSХ‘QХУџNИ€€[ќHHXЭ[™ЧЩ[ќ]KOњ^Y\љ[™^В€ЧЬ^Y\€
XЭ[™ЧЬ^Y\€H^Y\€
ИNВ‚€YЉXЭ[™ЧЬ^Y\‹O™[ќOHXЭ[™ЧЩ[ќ]HXЭ[™ЧЩ[ќ]KO™X]ЬЭ]H	€PUФХUWСPQ
B€В€™]\›ЋВ€B‚€К‚€
€\™HЩHЭXЪ[™ИH[™]™[[ќ]OВ€
‹В€YЉ
Э\€Hљ[™Щ[ќЪ\™JXЭ[™ЧЩ[ќ]KXЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћXЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћ‹TWСS‘U‘S•S
JH€	‰€Y™ЉXЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћKЭ\‹OњЬЪ][Ы‹ћJHHЊJHВ‚€›ЫЫ›ЧЬ^Y\—Ь™XXЪYH[ЩNВ€Z[ќЌЭЭ[WЫ›ЭЬ™XXЪYHВ€Z[ќЌЭЭ[WЬ™XXЪYHВ€Z[ќЌЭNВ€€›Ь€
HHИHPVФVQT”ОИJККHВ€Y€
\™XXЪYЪWJ^В€Э[WЫ›ЭЬ™XXЪY
КОВ€B€B€›ЧЬ^Y\—Ь™XXЪYH
Э[WЫ›ЭЬ™XXЪYЏHPVФVQT”КHИќYH€[ЩNВ‚€YЉ›ЧЬ^Y\—Ь™XXЪY
HВ€YШЫЬ™JKЭ\‹O›[Щ[]KњШЫЬ™JNВ€B€™XXЪYЬWHHќYNВ‚€›Ь€
HHИHPVФVQT”ОИJККHВ€Э[WЬ™XXЪY
ПH™XXЪYЪWNВ€B‚€Y€
[Э\‹O›[Щ[]KњЭXќ\H
Э\‹O›[Щ[]KњЭXќ\HOHХP•TWР“Х	‰€Э[WЬ™XXЪYЏH
ЫЭ[ќЩ[ќКTWФVQTЉJJJHВ€]™[ШЫЫ\]YHNВ‚€YЉЭ\‹O›[Щ[]Kњ[Ъ
HВ€Э›ЬJњ[ЪЫ[YKЭ\‹O›[Щ[]Kњ[ЪPVУђSQWУSЉNИЛЫ›ЭЛ[ЭHШ[€њ[ЪИ[›Э\€]™[€B€™]\›ЋВ€B€B‚€К‚€
€™\Щ]ЫЫX›ИЫЭ[ќY€[YH\И^\™Y‚€
‹В€YЉЭ[YH€XЭ[™ЧЩ[ќ]KOњќ\Ъќ[YJHВ€XЭ[™ЧЩ[ќ]KOњќ\ЪЫЭ[ќHВ€XЭ[™ЧЩ[ќ]KOњќ\Ъќ[YHHВ€B‚€YЉ^Y\—Ь™Z[њ]

JHВ€ЫЭИ[™[љШЪXЪОВ€B‚€YЉXЭ[™ЧЩ[ќ]KOЪ\™Ъ[™КHВ€^Y\—ШЪ\™ЩWШЪXЪК
NВ€ЫЭИ[™[љШЪXЪОВ€B‚€YЉXЭ[™ЧЩ[ќ]KOљ[њZ[€	€’S—ФRS—У“У‘H
XЭ[™ЧЩ[ќ]KO›[љИ	‰€XXЭ[™ЧЩ[ќ]KO™ЬXљ[™КJHВ€^Y\—ЬZ[—ШЪXЪК
NВ€ЫЭИ[™[љШЪXЪОВ€B‚€ЛИ[[™ПИЪXЪИ›Ь€[™[™В€YЉXЭ[™ЧЩ[ќ]KOњ›Ъ™XЭ[H	€“TХХФФКHВ€^Y\—Щ[ШЪXЪК
NВ€ЫЭИ[™[љШЪXЪОВ€B‚€ЛИЬX€ЩXЭ[Ы‹Ыќ[Э™HY€Э[[љ[X][™В€YЉXЭ[™ЧЩ[ќ]KO™ЬXљ[™И	‰€XЭ[™ЧЩ[ќ]KO]XЪЪ[™ИOHUPТТS‘ЧУ“У‘H	‰€XЭ[™ЧЩ[ќ]KOќZЩXXЭ[Ы€OHЫЫ[[Ы—Э›ЭЧЭШZ]
HВ€^Y\—ЩЬX—ШЪXЪК
NВ€ЫЭИ[™[љШЪXЪОВ€B‚€ЛИќ[\ЩXЭ[Ы‚€YЉXЭ[™ЧЩ[ќ]KOљќ[\[™КHВ€^Y\—Ъќ[\ШЪXЪК
NВ€ЫЭИ[™[љШЪXЪОВ€B‚€YЉXЭ[™ЧЩ[ќ]KO[љ[[ќ[HOHS’WХРSУС‘ЉHВ€^Y\—ЭШ[ЫЩ™—ШЪXЪК
NВ€ЫЭИ[™[љШЪXЪОВ€B‚€YЉXЭ[™ЧЩ[ќ]KO™›Ь	‰€XЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћHOHXЭ[™ЧЩ[ќ]KO\ЩH	‰€XXЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћJHВ€^Y\—ЫYWШЪXЪК
NВ€ЫЭИ[™[љШЪXЪОВ€B‚€К‚€
€ЪXЪИY€^Y\€\И[€HЭ]H][ЭЬВ€
€Ь™[\ћHYK\Э]H[њ]›ШЩ\ЬЪ[™Л€Y€›Э€
€ЪЪ\H™\ЭЩ€H[њ]ЪXЪЬЛ‚€
‹В€YЉ\^Y\—ШXШЩ\ЧЪYWЪ[њ]
XЭ[™ЧЩ[ќ]JJHВ€ЫЭИ[™[љШЪXЪОВ€B‚€ЛИЪXЪИY€[ќ]H\И[™\€H]›Ь›B€КљYЉXЭ[™ЧЩ[ќ]KO›[Щ[]K›[Э™WШЫЫ™љYЧЩ›YЬИ	€SХ‘WРУУ‘’QЧФХP’‘PХХЧФU“Ф“H	‰€
ZYЪ\€HXЭ[™ЧЩ[ќ]KO[љ[X][Ы‹OњЪ^™KћHИXЭ[™ЧЩ[ќ]KO[љ[X][Ы‹OњЪ^™KћH€XЭ[™ЧЩ[ќ]KO›[Щ[]KњЪ^™KћJH	‰‚€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WСPТКH	‰€ЪXЪЧЬ]›Ь›WШ™]ЩY[ЉXЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћXЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћ‹XЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћKXЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћH
ИZYЪ\‹XЭ[™ЧЩ[ќ]JJB€В€XЭ[™ЧЩ[ќ]KOљY[™ИHQS‘ЧУ“У‘NВ€XЭ[™ЧЩ[ќ]KO™XЪЪ[™ИHPТЧРPХU‘NВ€XЭ[™ЧЩ[ќ]KOќZЩXXЭ[Ы€HЫЫ[[Ы—ЬЭXЪЧЭ[™\›™X]В€[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WСPТЛ
NВ€ЫЭИ[™[љШЪXЪОВ€J‹В‚€ЫЫњЭ›Ш][Y™€HY™ЉXЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћKXЭ[™ЧЩ[ќ]KO\ЩJNВ€ЫЫњЭ›ЫЫ›Э[Z\€H
XЭ[™ЧЩ[ќ]KO›[™YЫЫ—Ь]›Ь›HИ[Y™€H€[Y™€ЉNВ‚€YЉXЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	€“QЧУSХ‘UT
HВ‚€ЫЫњЭ›ЫЫЫЫ[X[™ЫX]ЪH
›Э[Z\€	‰€X]ЪШЫЫX›КЩ\]Y[ЩWЭ\Э\XЭ[™ЧЬ^Y\‹ЉJNВ‚€YЉЫЫ[X[™ЫX]Ъ	‰€
XЭ[™ЧЩ[ќ]KO›[Щ[]Kњќ[—ШЫЫ™љYЧЩ›YЬИ	€
•S—РУУ‘’QЧЦ—ХTСSђP“Q•S—РУУ‘’QЧЦ—ХTТS’UPS
JHOH
•S—РУУ‘’QЧЦ—ХTСSђP“Q•S—РУУ‘’QЧЦ—ХTТS’UPS
H	‰€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WФ•SЉJHВ€XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	ЏH‘“QЧУSХ‘UTВ€ЫЫ[X[™Ъ[њ]Ъ\ЭЬћWШЫЫњЭ[YWЫ]\ЭЬ™\ЬКXЭ[™ЧЬ^Y\ЉNВ€XЭ[™ЧЩ[ќ]KOњќ[›љ[™ИH•S—ФХUWФХT•ЦИЛИ^Y\€™YЪ[њИИќ[‚€€H[ЩHYЉЫЫ[X[™ЫX]Ъ	‰€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WРUPТХT
JHВ‚€ЛИ™]ИHHЫЫX›И]XЪВ€XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	ЏH‘“QЧУSХ‘UTВ€XЭ[™ЧЩ[ќ]KOќZЩXXЭ[Ы€HЫЫ[[Ы—Ш]XЪЧЬ›ШОВ€Щ]Ш]XЪЪ[™КXЭ[™ЧЩ[ќ]JNВ€XЭ[™ЧЩ[ќ]KOЫЫX›ЬЭ\МHHВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHXЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HВ€[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WРUPТХT
NВ€ЫЫ[X[™Ъ[њ]Ъ\ЭЬћWШЫЫњЭ[YWЫ]\ЭЬ™\ЬКXЭ[™ЧЬ^Y\ЉNИЛИ\ИЫЬљШ\›Э[™X[ИY][њ™Y\ЬXЪX[‚€ЫЭИ[™[љШЪXЪОВ€€H[ЩHYЉЫЫ[X[™ЫX]Ъ	‰€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WСССJJHВ€ЛИ™]ИЩЩH[Э™HZЩHЫ€УФЊВ€XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	ЏH‘“QЧУSХ‘UTВ€XЭ[™ЧЩ[ќ]KOќZЩXXЭ[Ы€HЫЫ[[Ы—ЩЩЩNВ€XЭ[™ЧЩ[ќ]KOЫЫX›ЬЭ\МHHВ€XЭ[™ЧЩ[ќ]KOљY[™ИHQS‘ЧУ“У‘NВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HXXЭ[™ЧЩ[ќ]KO›[Щ[]KњЬYYћ
€KЌНNВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHЛЛИТИ[ЭHШ[€\ЩHќ[\њ[YHИ[ЩYћH\И[ћ]Ш^B€[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WСССK
NВ€ЫЫ[X[™Ъ[њ]Ъ\ЭЬћWШЫЫњЭ[YWЫ]\ЭЬ™\ЬКXЭ[™ЧЬ^Y\ЉNВ€ЫЭИ[™[љШЪXЪОВ€B€B‚€YЉXЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	€“QЧУSХ‘QХУЉHВ€ЫЫњЭ›ЫЫЫЫ[X[™ЫX]ЪH
›Э[Z\€	‰€X]ЪШЫЫX›КЩ\]Y[ЩWЩЭЫ—ЩЭЫ‹XЭ[™ЧЬ^Y\‹ЉJNВ‚€YЉЫЫ[X[™ЫX]Ъ	‰€
XЭ[™ЧЩ[ќ]KO›[Щ[]Kњќ[—ШЫЫ™љYЧЩ›YЬИ	€
•S—РУУ‘’QЧЦ—СХУ—СSђP“Q•S—РУУ‘’QЧЦ—СХУ—ТS’UPS
JHOH
•S—РУУ‘’QЧЦ—СХУ—СSђP“Q•S—РУУ‘’QЧЦ—СХУ—ТS’UPS
H	‰€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WФ•SЉJHВ€XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	ЏH‘“QЧУSХ‘QХУЋВ€ЫЫ[X[™Ъ[њ]Ъ\ЭЬћWШЫЫњЭ[YWЫ]\ЭЬ™\ЬКXЭ[™ЧЬ^Y\ЉNВ€XЭ[™ЧЩ[ќ]KOњќ[›љ[™ИH•S—ФХUWФХT•ЦЋИЛИ^Y\€™YЪ[њИИќ[‚€€H[ЩHYЉЫЫ[X[™ЫX]Ъ	‰€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WРUPТСХУЉJHВ€ЛИ™]ИЫЫX›И]XЪВ€XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	ЏH‘“QЧУSХ‘QХУЋВ€XЭ[™ЧЩ[ќ]KOќZЩXXЭ[Ы€HЫЫ[[Ы—Ш]XЪЧЬ›ШОВ€Щ]Ш]XЪЪ[™КXЭ[™ЧЩ[ќ]JNВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHXЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HВ€XЭ[™ЧЩ[ќ]KOЫЫX›ЬЭ\МHHВ€[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WРUPТСХУ‹
NВ€ЫЫ[X[™Ъ[њ]Ъ\ЭЬћWШЫЫњЭ[YWЫ]\ЭЬ™\ЬКXЭ[™ЧЬ^Y\ЉNВ€ЫЭИ[™[љШЪXЪОВ€€H[ЩHYЉЫЫ[X[™ЫX]Ъ	‰€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WСССJJHВ€ЛИ™]ИЩЩH[Э™HZЩHЫ€УФЊВ€XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	ЏH‘“QЧУSХ‘QХУЋВ€XЭ[™ЧЩ[ќ]KOќZЩXXЭ[Ы€HЫЫ[[Ы—ЩЩЩNВ€XЭ[™ЧЩ[ќ]KOЫЫX›ЬЭ\МHHВ€XЭ[™ЧЩ[ќ]KOљY[™ИHQS‘ЧУ“У‘NВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HXЭ[™ЧЩ[ќ]KO›[Щ[]KњЬYYћ
€KЌНNВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHВ€[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WСССK
NВ€ЫЫ[X[™Ъ[њ]Ъ\ЭЬћWШЫЫњЭ[YWЫ]\ЭЬ™\ЬКXЭ[™ЧЬ^Y\ЉNВ€ЫЭИ[™[љШЪXЪОВ€B€B‚€YЉ
XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	€
“QЧУSХ‘SQ•“QЧУSХ‘T’QТ
JJHВ€€ЫЫњЭ›ЫЫЫЫ[X[™ЫX]ЪЫYќH
›Э[Z\€	‰€
XЭ[™ЧЩ[ќ]KO™\™XЭ[Ы€OHT‘PХSУ—УQ•	‰€X]ЪШЫЫX›КЩ\]Y[ЩWЫYќЫYќXЭ[™ЧЬ^Y\‹ЉJJNВ€ЫЫњЭ›ЫЫЫЫ[X[™ЫX]ЪЬљYЪH
›Э[Z\€	‰€
XЭ[™ЧЩ[ќ]KO™\™XЭ[Ы€OHT‘PХSУ—Ф’QТ	‰€X]ЪШЫЫX›КЩ\]Y[ЩWЬљYЪЬљYЪXЭ[™ЧЬ^Y\‹ЉJJNВ€ЫЫњЭ›ЫЫЫЫ[X[™ЫX]ЪЩ›ЬќШ\™HЫЫ[X[™ЫX]ЪЫYќЫЫ[X[™ЫX]ЪЬљYЪВ€ЫЫњЭ›ЫЫЫЫ[X[™ЫX]ЪШXЪИH›Э[Z\‚€	‰€XЭ[™ЧЩ[ќ]KO›[Щ[]K™XЪ[™В€	‰€

XЭ[™ЧЩ[ќ]KO™\™XЭ[Ы€OHT‘PХSУ—Ф’QТ	‰€X]ЪШЫЫX›КЩ\]Y[ЩWЫYќЫYќXЭ[™ЧЬ^Y\‹ЉJB€
XЭ[™ЧЩ[ќ]KO™\™XЭ[Ы€OHT‘PХSУ—УQ•	‰€X]ЪШЫЫX›КЩ\]Y[ЩWЬљYЪЬљYЪXЭ[™ЧЬ^Y\‹ЉJJNИ€€Y€
ЫЫ[X[™ЫX]ЪЫYќ	‰€
XЭ[™ЧЩ[ќ]KO›[Щ[]Kњќ[—ШЫЫ™љYЧЩ›YЬИ	€
•S—РУУ‘’QЧЦУQ•СSђP“Q•S—РУУ‘’QЧЦУQ•ТS’UPS
JHOH
•S—РУУ‘’QЧЦУQ•СSђP“Q•S—РУУ‘’QЧЦУQ•ТS’UPS
H	‰€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WФ•SЉJHВ‚€XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	ЏHЉ“QЧУSХ‘SQ•“QЧУSХ‘T’QТ
NИЛИ\ЭX[HYќ
ИљYЪ\И›ЭXШЩ\X›KЫИ]\ИТИИќ[›Э€ЫЫ[X[™Ъ[њ]Ъ\ЭЬћWШЫЫњЭ[YWЫ]\ЭЬ™\ЬКXЭ[™ЧЬ^Y\ЉNВ€XЭ[™ЧЩ[ќ]KOњќ[›љ[™ИH•S—ФХUWФХT•ЦИЛИ^Y\€™YЪ[њИИќ[‚€€H[ЩHYЉЫЫ[X[™ЫX]ЪЬљYЪ	‰€
XЭ[™ЧЩ[ќ]KO›[Щ[]Kњќ[—ШЫЫ™љYЧЩ›YЬИ	€
•S—РУУ‘’QЧЦФ’QТСSђP“Q•S—РУУ‘’QЧЦФ’QТТS’UPS
JHOH
•S—РУУ‘’QЧЦФ’QТСSђP“Q•S—РУУ‘’QЧЦФ’QТТS’UPS
H	‰€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WФ•SЉJHВ€€XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	ЏHЉ“QЧУSХ‘SQ•“QЧУSХ‘T’QТ
NИЛИ\ЭX[HYќ
ИљYЪ\И›ЭXШЩ\X›KЫИ]\ИТИИќ[›Э€ЫЫ[X[™Ъ[њ]Ъ\ЭЬћWШЫЫњЭ[YWЫ]\ЭЬ™\ЬКXЭ[™ЧЬ^Y\ЉNВ€XЭ[™ЧЩ[ќ]KOњќ[›љ[™ИH•S—ФХUWФХT•ЦИЛИ^Y\€™YЪ[њИИќ[‚€€H[ЩHYЉЫЫ[X[™ЫX]ЪШXЪИ	‰€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WРђPТФ•SЉJHВ€XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	ЏHЉ“QЧУSХ‘SQ•“QЧУSХ‘T’QТ
NИЛИ\ЭX[HYќ
ИљYЪ\И›ЭXШЩ\X›KЫИ]\ИТИИќ[›Э€ЫЫ[X[™Ъ[њ]Ъ\ЭЬћWШЫЫњЭ[YWЫ]\ЭЬ™\ЬКXЭ[™ЧЬ^Y\ЉNВ€XЭ[™ЧЩ[ќ]KOњќ[›љ[™ИH•S—ФХUWФХT•ЦИЛИ^Y\€™YЪ[њИИќ[‚€€H[ЩHYЉЫЫ[X[™ЫX]ЪЩ›ЬќШ\™	‰€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WРUPТС“Ф•РT‘
JHВ€XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	ЏHЉ“QЧУSХ‘SQ•“QЧУSХ‘T’QТ
NВ€XЭ[™ЧЩ[ќ]KOќZЩXXЭ[Ы€HЫЫ[[Ы—Ш]XЪЧЬ›ШОВ€Щ]Ш]XЪЪ[™КXЭ[™ЧЩ[ќ]JNВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHXЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HВ€XЭ[™ЧЩ[ќ]KOЫЫX›ЬЭ\МHHВ€[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WРUPТС“Ф•РT‘
NВ€ЫЫ[X[™Ъ[њ]Ъ\ЭЬћWШЫЫњЭ[YWЫ]\ЭЬ™\ЬКXЭ[™ЧЬ^Y\ЉNВ€ЫЭИ[™[љШЪXЪОВ€B€B‚€К‚€
€]XЪИ›Э€]XЪИ
ИЩ^HX\Y€
€ћHZњЬXЪX[‚€
‹В‚€YЉ
ЫШ[ШЫЫ™љYЛZњЬXЪX[OHR”ФPТPSТСVWФФPТPS	‰€
XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	€“QЧТ•ST
H	‰€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WРUPТР“Х
J_€
ЫШ[ШЫЫ™љYЛZњЬXЪX[OHR”ФPТPSТСVWРUPТМ€	‰€
XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	€“QЧТ•ST
H	‰€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WРUPТР“Х
J_€
ЫШ[ШЫЫ™љYЛZњЬXЪX[OHR”ФPТPSТСVWРUPТМИ	‰€
XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	€“QЧТ•ST
H	‰€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WРUPТР“Х
J_€
ЫШ[ШЫЫ™љYЛZњЬXЪX[OHR”ФPТPSТСVWРUPТН	‰€
XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	€“QЧТ•ST
H	‰€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WРUPТР“Х
JJB€В€YЉ
XЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧРUPТКH	‰€›Э[Z\ЉB€В€XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	ЏH‘“QЧТ•STВ€XЭ[™ЧЩ[ќ]KOќZЩXXЭ[Ы€HЫЫ[[Ы—Ш]XЪЧЬ›ШОВ€Щ]Ш]XЪЪ[™КXЭ[™ЧЩ[ќ]JNВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHXЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HВ€XЭ[™ЧЩ[ќ]KOЫЫX›ЬЭ\МHHВ€XЭ[™ЧЩ[ќ]KOњЭ[[YHHИЛИY€]XЪИ\И™\ЬЩYЫ[™ИЭЫ€]XЪИИ^XЭ]H]XЪМИ\И›ИЫ™Щ\€[Y€[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WРUPТР“Х
NВ€ЫЭИ[™[љШЪXЪОВ€B€B‚€К‚€
€ЫЫ[X[™TЪ\™ЩK‚€
‹В‚€YЉ
XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	€“QЧТ•ST
H	‰€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WРТT‘СJJB€В€YЉ
XЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧФФPТPS
H	‰€›Э[Z\ЉB€В€XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	ЏH‘“QЧТ•STВ€XЭ[™ЧЩ[ќ]KOќZЩXXЭ[Ы€HЫЫ[[Ы—ШЪ\™ЩNВ€XЭ[™ЧЩ[ќ]KOЫЫX›ЬЭ\МHHВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHXЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HВ€XЭ[™ЧЩ[ќ]KOњЭ[[YHHВ€Щ]ШЪ\™Ъ[™КXЭ[™ЧЩ[ќ]JNВ€[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WРТT‘СK
NВ€ЫЭИ[™[љШЪXЪОВ€B€B‚€К‚€
€[™HЬXЪX[Щ^K€ЪXЪИ›Ь€\ќЩ€H€
€њ™Y\ЬXЪX[ЫЫ[X[™[™[€›ШЪЪ[™Л‚€
‹В‚€YЉXЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	€“QЧФФPТPS
HЛИHЬXЪX[ќ]Ы€Ш[€›ЭИ™H\ЩY›Ь€њ™Y\ЬXЪX[В€В€YЉ[Y[љ[JXЭ[™ЧЩ[ќ]KS’WФФPТPSЉH	‰€›Э[Z\€	‰‚€
XЭ[™ЧЩ[ќ]KO™\™XЭ[Ы€OHT‘PХSУ—УQ•В€
XЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧУSХ‘SQ•
H‚€
XЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧУSХ‘T’QТ
JH
B€В€YЉЪXЪЧШЫЬЭ[Э™JS’WФФPТPS‹
JB€В€XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	ЏH‘“QЧФФPТPSВ€ЫЭИ[™[љШЪXЪОВ€B€B‚‚BKЛИ›ШЪЪ[™Л‚€YЉ[Y[љ[JXЭ[™ЧЩ[ќ]KS’WР“РТКH	‰€›Э[Z\€	‰€JXЭ[™ЧЩ[ќ]KO›[Щ[]K›ШЪЧШЫЫ™љYЧЩ›YЬИ	€“РТЧРУУ‘’QЧСTРP“Q
JB€В€XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	ЏH‘“QЧФФPТPSВ‚‚BBKЛИЩ]\›YЬЛXЭ[Ы‹[™›ШЪИ[љ[X][ЫњЛ‚‚BBYЧШXЭ]™WШ›ШЪКXЭ[™ЧЩ[ќ]JNВ‚‚BBYЫЭИ[™[љШЪXЪОВ€B€B‚€К‚€
€њ™XZЫЭ]ЬXЪX[‚€
‹В‚€YЉ›Э[Z\€	‰€^Y\—ШЪXЪЧЬЬXЪX[

JB€В€ЫЭИ[™[љШЪXЪОИЛИЫИ[ЭHЫ‰Э\™›Ь›HЬXЪX[И[[™ИЩ™€HYЩB€B‚€К‚€
€Ъ\™ЩH]XЪИ
Ы]XЪИ	€™[X\ЩJK‚€
‹В‚€YЉ
XЭ[™ЧЬ^Y\‹Oњ™[X\ЩZЩ^\И	€“QЧРUPТКJB€В€YЉXЭ[™ЧЩ[ќ]KOњЭ[[YH	‰€›Э[Z\€	‰‚€

[Y[љ[JXЭ[™ЧЩ[ќ]KS’WРТT‘СPUPТКH	‰€XЭ[™ЧЩ[ќ]KOњЭ[[YH
И
ЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYY
€XЭ[™ЧЩ[ќ]KO›[Щ[]K[љ[X][Ы–РS’WРТT‘СPUPТЧKOЪ\™ЩWЭ[YJHЭ[YJH€
][Y[љ[JXЭ[™ЧЩ[ќ]KS’WРТT‘СPUPТКH	‰€[Y[љ[JXЭ[™ЧЩ[ќ]K[љ[X]XЪЬЦШXЭ[™ЧЩ[ќ]KO›[Щ[]K]ЪZ[–ШXЭ[™ЧЩ[ќ]KO›[Щ[]KЪZ[›[™ЭHWHHWJB€	‰€XЭ[™ЧЩ[ќ]KO›[Щ[]KЪZ[›[™Э€	‰€XЭ[™ЧЩ[ќ]KOњЭ[[YH
И
ЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYY
€XЭ[™ЧЩ[ќ]KO›[Щ[]K[љ[X][Ы–Ш[љ[X]XЪЬЦШXЭ[™ЧЩ[ќ]KO›[Щ[]K]ЪZ[–ШXЭ[™ЧЩ[ќ]KO›[Щ[]KЪZ[›[™ЭHWHHWWKOЪ\™ЩWЭ[YJHЭ[YJJJB€В€XЭ[™ЧЩ[ќ]KOќZЩXXЭ[Ы€HЫЫ[[Ы—Ш]XЪЧЬ›ШОВ€Щ]Ш]XЪЪ[™КXЭ[™ЧЩ[ќ]JNВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHXЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HВ‚€XЭ[™ЧЩ[ќ]KOњЭ[[YHHВ€XЭ[™ЧЩ[ќ]KOЫЫX›ЬЭ\МHHВ‚€YЉЫШ[ЬШ[\WЫ\Эњ[ЪЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Эњ[ЪШ]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B‚€YЉ[Y[љ[JXЭ[™ЧЩ[ќ]KS’WРТT‘СPUPТКJB€В€[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WРТT‘СPUPТЛ
NВ€B€[ЩHYЉ[Y[љ[JXЭ[™ЧЩ[ќ]K[љ[X]XЪЬЦШXЭ[™ЧЩ[ќ]KO›[Щ[]K]ЪZ[–ШXЭ[™ЧЩ[ќ]KO›[Щ[]KЪZ[›[™ЭHWHHWJJB€В€[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]K[љ[X]XЪЬЦШXЭ[™ЧЩ[ќ]KO›[Щ[]K]ЪZ[–ШXЭ[™ЧЩ[ќ]KO›[Щ[]KЪZ[›[™ЭHWHHWK
NВ€B€ЫЭИ[™[љШЪXЪОВ€B€XЭ[™ЧЩ[ќ]KOњЭ[[YHHВ€B‚€К‚€
€]XЪИќ]Ы‹€[™H\ЪXИ]XЪИ€
€ќ]Ы€XЭ[ЫњИ
]XЪЛЩ]]ЛЉK‚€
‹В‚€YЉ
XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	€“QЧРUPТКH	‰€›Э[Z\ЉB€В€XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	ЏH‘“QЧРUPТОВ€XЭ[™ЧЩ[ќ]KOњЭ[[YHHИЛИ\ШX›HH]XЪМИЭ[[YB‚€YЉ
XЭ[™ЧЩ[ќ]KO™XЪЪ[™И	€PТЧРPХU‘JH	‰€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WСPТРUPТКH	‰€VQT—УRS—Ц€OHVQT—УPVЦЉHЛШXЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧУSХ‘QХУ‚€В€XЭ[™ЧЩ[ќ]KOќZЩXXЭ[Ы€HЫЫ[[Ы—Ш]XЪЧЬ›ШОВ€Щ]Ш]XЪЪ[™КXЭ[™ЧЩ[ќ]JNВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHXЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HВ€XЭ[™ЧЩ[ќ]KOЫЫX›ЬЭ\МHHВ€[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WСPТРUPТЛ
NВ€ЫЭИ[™[љШЪXЪОВ€B‚€YЉXЭ[™ЧЩ[ќ]KOњќ[›љ[™И	‰€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WФ•SђUPТКJHЛИ™]Иќ[€]XЪИЫЩHЩXЭ[Ы‚€В€XЭ[™ЧЩ[ќ]KOќZЩXXЭ[Ы€HЫЫ[[Ы—Ш]XЪЧЬ›ШОВ€Щ]Ш]XЪЪ[™КXЭ[™ЧЩ[ќ]JNВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHXЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HВ€XЭ[™ЧЩ[ќ]KOЫЫX›ЬЭ\МHHВ€XЭ[™ЧЩ[ќ]KOњќ[›љ[™ИH•S—ФХUWУ“У‘NВ€[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WФ•SђUPТЛ
NВ€ЫЭИ[™[љШЪXЪОВ€B‚€К‚€
€XЪИ]XЪЛ€Y€^Y\‰ЬИЫЫ[X[™ќY™™\€X]Ъ\И€
€HXЪИ]XЪИЩ\]Y[ЩKЩIЫ][\ИИ€
€HXЪИ]XЪЛ‚€
‹В€ЫЫњЭЩ^WЫX\ЪЧЭЩ\]Y[ЩWШXЪЧШ]XЪЦЧHHС“QЧРђPТХРT‘“QЧРUPТЯNВ‚€YЉ[Y[љ[JXЭ[™ЧЩ[ќ]KS’WРUPТРђPТХРT‘
H	‰€X]ЪШЫЫX›КЩ\]Y[ЩWШXЪЧШ]XЪЛXЭ[™ЧЬ^Y\‹ЉJHВ€ЫЫњЭЧШЫЫ[X[™Ъ[њ]Щ]™[ќ
€]XЪЧЪ[њ]Щ]™[ќВ€ЫЫњЭЧШЫЫ[X[™Ъ[њ]Щ]™[ќ
€XЪЭШ\™Ъ[њ]Щ]™[ќВ‚€Z[ќЌЭ\ЭЬћWЪ[™^B€XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]Ъ[™^В‚€Z[ќЌЭ™[XZ[љ[™ЧЩ]™[ќИB€XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]ШЫЭ[ќВ‚€К‚€
€Щ]H\ЭЫИ™\ЬИ]™[ќЛ€™XY[™И›ЭYЪ€
€HЫЫ\]Xљ[]H[\€™]™[ќИќ]\™H™[X\ЩB€
€[™[YYЫ]™[ќИњ›ЫH\Э\љ[™ИXЪИ]XЪЛ‚€
‹В€]XЪЧЪ[њ]Щ]™[ќHЫЫ[X[™Ъ[њ]Ъ\ЭЬћWЬ™]љ[Э\ЧЬ™\ЬК€XЭ[™ЧЬ^Y\‹€	љ\ЭЬћWЪ[™^€	њ™[XZ[љ[™ЧЩ]™[ќВ€
NВ‚€XЪЭШ\™Ъ[њ]Щ]™[ќHЫЫ[X[™Ъ[њ]Ъ\ЭЬћWЬ™]љ[Э\ЧЬ™\ЬК€XЭ[™ЧЬ^Y\‹€	љ\ЭЬћWЪ[™^€	њ™[XZ[љ[™ЧЩ]™[ќВ€
NВ‚€К‚€
€YXЪИ]XЪИ[њ]ИЫЫYHЪ][€B€
€[YHЪ[™ЭПИY€ЫЛЩIЫИHXЪИ]XЪЛ‚€
‹В€YЉ]XЪЧЪ[њ]Щ]™[ќ€	‰€XЪЭШ\™Ъ[њ]Щ]™[ќ€	‰€]XЪЧЪ[њ]Щ]™[ќOќ[YB€HXЪЭШ\™Ъ[њ]Щ]™[ќOќ[YB€ЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYYИL
HВ‚€XЭ[™ЧЩ[ќ]KOќZЩXXЭ[Ы€HЫЫ[[Ы—Ш]XЪЧЬ›ШОВ€Щ]Ш]XЪЪ[™КXЭ[™ЧЩ[ќ]JNВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HВ‚€YЉXЭ[™ЧЩ[ќ]KO™\™XЭ[Ы€OHT‘PХSУ—УQ•€	‰€
XЪЭШ\™Ъ[њ]Щ]™[ќOњ™\ЬИ	€“QЧУSХ‘SQ•
JHВ€€XЭ[™ЧЩ[ќ]KO™\™XЭ[Ы€HT‘PХSУ—Ф’QТВ€€H[ЩHYЉXЭ[™ЧЩ[ќ]KO™\™XЭ[Ы€OHT‘PХSУ—Ф’QТ€	‰€
XЪЭШ\™Ъ[њ]Щ]™[ќOњ™\ЬИ	€“QЧУSХ‘T’QТ
JHВ€XЭ[™ЧЩ[ќ]KO™\™XЭ[Ы€HT‘PХSУ—УQ•В€B‚€XЭ[™ЧЩ[ќ]KOЫЫX›ЬЭ\МHHВ€[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WРUPТРђPТХРT‘
NВ€ЫЭИ[™[љШЪXЪОВ€B€B‚€К‚€
€Щ]][K€Y€^Y\€\ИЭ[™[™ИЫ€[€][B€
€[™HЩ][љ[X][Ы€\И[Y[‚€
€ЩIЫXЪИ]\[™ќ[€HЩ][љ[X][Ы‹‚€
‹В€YЉ[Y[љ[JXЭ[™ЧЩ[ќ]KS’WССU
H€	‰€
Э\€Hљ[™Щ[ќЪ\™JXЭ[™ЧЩ[ќ]KXЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћXЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћ‹TWТUSK^Y\—Э\ЭЬXЪШX›JJJHВ‚€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHXЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HВ€Щ]ЩЩ][™КXЭ[™ЧЩ[ќ]JNВ€XЭ[™ЧЩ[ќ]KOќZЩXXЭ[Ы€HЫЫ[[Ы—ЩЩ]В€[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WССU
NВ‚€К‚€
€][H]XЪЬИ€ЫЫXЭЬ€ИXZЩH]€
€X\ЮHИШЬљ\XЭ[ЫњИЫ€][HXЪИ\€€
‹В€ЧЪ][WЬШЬљ\
XЭ[™ЧЩ[ќ]KЭ\ЉNВ‚€Yљ[™Ъ][JЭ\ЉNВ€ЫЭИ[™[љШЪXЪОВ€B‚€ЛИ\ЩHЭ[[YHИЪ\™ЩH[™[[Э™B€XЭ[™ЧЩ[ќ]KOњЭ[[YHHЭ[YNВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHXЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HВ‚€YЉXЭ[™ЧЩ[ќ]KOќЩX\[ќ	‰‚€XЭ[™ЧЩ[ќ]KOќЩX\[ќO›[Щ[]KњЭXќ\HOHХP•TWФ“Т‘PХSH	‰‚€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WХ“ХРUPТКH
B€В€XЭ[™ЧЩ[ќ]KOќZЩXXЭ[Ы€HЫЫ[[Ы—Ш]XЪЧЬ›ШОВ€Щ]Ш]XЪЪ[™КXЭ[™ЧЩ[ќ]JNВ€[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WХ“ХРUPТЛ
NВ€ЫЭИ[™[љШЪXЪОВ€B€[ЩHYЉ\™›Ь›WШ]ЪZ[Љ
JB€В€YЉЫШ[ЬШ[\WЫ\Эњ[ЪЏH	‰€XЭ[™ЧЩ[ќ]KO]XЪЪ[™ИOHUPТТS‘ЧУ“У‘JB€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Эњ[ЪШ]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€ЫЭИ[™[љШЪXЪОВ€B‚€B€€К‚€
€ќ[\[™ИЫЫќЬ›‚€
‹В‚€YЉXЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	€“QЧТ•ST	‰€›Э[Z\ЉB€В€ЛИYYZ[Z\ЉXЭ[™ЧЩ[ќ]JHЫИ^Y\њИШ[‰Эќ[\Ъ[€[[™И[ќИЫ\В€XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	ЏH‘“QЧТ•STВ‚€К‚€
€€^\ИЫЫќ›Ы‚€
‚€
€Y€^Y\€Ш[€[љ]X[^™HH€ќ[\€
€[™\Ь€ЭЫ€Щ^\И\™HXЭ]™HЩIЫ€
€Щ]HЬќ[Ы€Щ€Z\€ќ[\ЬYY\В€
€H€™[ШЪ]K€ЩIЫ\ЩH\И[YH\В€
€H€\[Y]\€›Ь€[ЭЫњЭ™X[B€
€ќ[\ќ[Э[Ы€Ш[Л‚€
‹В‚€Y€
XЭ[™ЧЩ[ќ]KO›[Щ[]KZ\—ШЫЫќ›Ы	€RT—РУУ•“УТ•STЦ—ТS’UPS
B€В€Y€
XЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧУSХ‘UT
B€В€[љ]X[Ъќ[\Э™[ШЪ]WЮ€HXXЭ[™ЧЩ[ќ]KO›[Щ[]Kљќ[\ЬYY
€ЌNВ€B€[ЩHY€
XЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧУSХ‘QХУЉB€В€[љ]X[Ъќ[\Э™[ШЪ]WЮ€HXЭ[™ЧЩ[ќ]KO›[Щ[]Kљќ[\ЬYY
€ЌNВ€B€B€[ЩB€В€[љ]X[Ъќ[\Э™[ШЪ]WЮ€HЊВ€B‚€YЉXЭ[™ЧЩ[ќ]KOњќ[›љ[™КB€В€ЛФЫYB€YЉ
XЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧУSХ‘QХУЉH	‰€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WФ•S”УQJJB€В€XЭ[™ЧЩ[ќ]KOќZЩXXЭ[Ы€HЫЫ[[Ы—Ш]XЪЧЬ›ШОВ€Щ]Ш]XЪЪ[™КXЭ[™ЧЩ[ќ]JNВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHXЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HВ€XЭ[™ЧЩ[ќ]KOЫЫX›ЬЭ\МHHВ€XЭ[™ЧЩ[ќ]KOњќ[›љ[™ИH•S—ФХUWУ“У‘NВ€[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WФ•S”УQK
NВ€ЫЭИ[™[љШЪXЪОВ€H‚€К€ќ[\[™И[ЭЩYИ
‹В€Y€
JXЭ[™ЧЩ[ќ]KO›[Щ[]KZ\—ШЫЫќ›Ы	€RT—РУУ•“УТ•STСTРP“JJB€В€Y€
[Y[љ[JXЭ[™ЧЩ[ќ]KS’WФ•S’•ST
JB€В€ћZќ[\
XЭ[™ЧЩ[ќ]KO›[Щ[]Kњќ[љќ[\ZYЪXЭ[™ЧЩ[ќ]KO›[Щ[]Kљќ[\ЬYY
€XЭ[™ЧЩ[ќ]KO›[Щ[]Kњќ[љќ[\\Э[љ]X[Ъќ[\Э™[ШЪ]WЮ‹S’WФ•S’•ST
NВ€B€[ЩHY€
[Y[љ[JXЭ[™ЧЩ[ќ]KS’WС“Ф•РT‘•ST
JB€В€ћZќ[\
XЭ[™ЧЩ[ќ]KO›[Щ[]Kњќ[љќ[\ZYЪXЭ[™ЧЩ[ќ]KO›[Щ[]Kљќ[\ЬYY
€XЭ[™ЧЩ[ќ]KO›[Щ[]Kњќ[љќ[\\Э[љ]X[Ъќ[\Э™[ШЪ]WЮ‹S’WС“Ф•РT‘•ST
NВ€B€[ЩHY€
[Y[љ[JXЭ[™ЧЩ[ќ]KS’WТ•ST
JB€В€ћZќ[\
XЭ[™ЧЩ[ќ]KO›[Щ[]Kњќ[љќ[\ZYЪXЭ[™ЧЩ[ќ]KO›[Щ[]Kљќ[\ЬYY
€XЭ[™ЧЩ[ќ]KO›[Щ[]Kњќ[љќ[\\Э[љ]X[Ъќ[\Э™[ШЪ]WЮ‹S’WТ•ST
NВ€B€B€B€[ЩB€В€ЛФЫYB€YЉ
XЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧУSХ‘QХУЉH	‰€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WФУQJJB€В€XЭ[™ЧЩ[ќ]KOќZЩXXЭ[Ы€HЫЫ[[Ы—Ш]XЪЧЬ›ШОВ€Щ]Ш]XЪЪ[™КXЭ[™ЧЩ[ќ]JNВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHXЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HВ€XЭ[™ЧЩ[ќ]KOЫЫX›ЬЭ\МHHВ€XЭ[™ЧЩ[ќ]KOњќ[›љ[™ИH•S—ФХUWУ“У‘NВ€[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WФУQK
NВ€ЫЭИ[™[љШЪXЪОВ€B‚€К€ќ[\[™И[ЭЩYИ
‹В€Y€
JXЭ[™ЧЩ[ќ]KO›[Щ[]KZ\—ШЫЫќ›Ы	€RT—РУУ•“УТ•STСTРP“JJB€В‚€К‚€
€[™HYќЬљYЪ\™XЭ[Ы€ЫЫ[X[™‚€
‚€
€Y€YќЬ€љYЪЩ^H\ИXЭ]™K€
€[€ЭЪ]ЪXЪ[™ИXШЫЬ™[™ЫH[™€
€ЫЫќ[ќYHЫ€И[Эљ[™Иќ[\ЩЪXЛ‚€
‚€
€Э\ќЪ\ЩH\™›Ь›HHќ[\Ъ]›В€
€Ьљ^›Ыќ[™[ШЪ]K‚€
‹В‚€Y€
JXЭ[™ЧЬ^Y\‹OљЩ^\И	€
“QЧУSХ‘SQ•“QЧУSХ‘T’QТ
JH	‰€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WТ•ST
JB€В€ћZќ[\
XЭ[™ЧЩ[ќ]KO›[Щ[]Kљќ[\ZYЪ[љ]X[Ъќ[\Э™[ШЪ]WЮ‹S’WТ•ST
NВ€ЫЭИ[™[љШЪXЪОВ€B€[ЩHY€

XЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧУSХ‘SQ•
JB€В€XЭ[™ЧЩ[ќ]KO™\™XЭ[Ы€HT‘PХSУ—УQ•В€B€[ЩHY€

XЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧУSХ‘T’QТ
JB€В€XЭ[™ЧЩ[ќ]KO™\™XЭ[Ы€HT‘PХSУ—Ф’QТВ€B‚€К‚€
€Ьљ^›Ыќ[[Эљ[™Иќ[\‚€
‹В‚€Y€
[Y[љ[JXЭ[™ЧЩ[ќ]KS’WС“Ф•РT‘•ST
JB€В€ћZќ[\
XЭ[™ЧЩ[ќ]KO›[Щ[]Kљќ[\ZYЪXЭ[™ЧЩ[ќ]KO›[Щ[]Kљќ[\ЬYY[љ]X[Ъќ[\Э™[ШЪ]WЮ‹S’WС“Ф•РT‘•ST
NВ€B€[ЩHY€
[Y[љ[JXЭ[™ЧЩ[ќ]KS’WТ•ST
JB€В€ћZќ[\
XЭ[™ЧЩ[ќ]KO›[Щ[]Kљќ[\ZYЪXЭ[™ЧЩ[ќ]KO›[Щ[]Kљќ[\ЬYY[љ]X[Ъќ[\Э™[ШЪ]WЮ‹S’WТ•ST
NВ€B€B€B€™]\›ЋВ€B‚€К‚€
€ќ[€ЭЬ‚€
‹В€ќ[—ЭћWЬќ[њЭЬЬ^Y\ЉXЭ[™ЧЩ[ќ]KXЭ[™ЧЬ^Y\ЉNВ‚€К‚€
‹В‚€YЉVQT—УRS—Ц€OHVQT—УPVЦ€	‰€XЭ[™ЧЩ[ќ]KO™XЪЪ[™ИOHPТЧУ“У‘JB€В€ЛИ[Ь™HЩ€H]›Ь›H™Y[€YЉXЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧУSХ‘UT
B€И€YЉ[Y[љ[JXЭ[™ЧЩ[ќ]KS’WХT
H	‰€XЭ[™ЧЩ[ќ]KOњќ[›љ[™ИOH•S—ФХUWУ“У‘JB€В€XЭ[Ы€HPХSУ—ХTВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HXXЭ[™ЧЩ[ќ]KO›[Щ[]KњЬYYћИЋИЛИ\ЩY›Ь€\[љ[X][Ы‚€B€[ЩHYЉXЭ[™ЧЩ[ќ]KOњќ[›љ[™И	€•S—ФХUWФХT•Ц
B€В€XЭ[Ы€HPХSУ—Ф•SЋВ‚€Y€
XЭ[™ЧЩ[ќ]KO›[Щ[]Kњќ[—ШЫЫ™љYЧЩ›YЬИ	€•S—РУУ‘’QЧЦФ’QТСTТС’VQ€	‰€JXЭ[™ЧЩ[ќ]KO›[Щ[]Kњќ[—ШЫЫ™љYЧЩ›YЬИ	€•S—РУУ‘’QЧЦ—ХTСSђP“Q
JHВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HЊВ€B€[ЩB€В€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HXXЭ[™ЧЩ[ќ]KO›[Щ[]Kњќ[њЬYYИЋИЛИ[Э™\И\]H\Э\€]Hќ[›љ[™В€B€B€[ЩB€В€XЭ[Ы€HPХSУ—ХРSОВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HXXЭ[™ЧЩ[ќ]KO›[Щ[]KњЬYYћИЋВ€B€B€[ЩHYЉXЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧУSХ‘QХУЉB€И€YЉ[Y[љ[JXЭ[™ЧЩ[ќ]KS’WСХУЉH	‰€XЭ[™ЧЩ[ќ]KOњќ[›љ[™ИOH•S—ФХUWУ“У‘JB€В€XЭ[Ы€HPХSУ—СХУЋВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HXЭ[™ЧЩ[ќ]KO›[Щ[]KњЬYYћИЋИЛИ\ЩY›Ь€ЭЫ€[љ[X][Ы‚€B€[ЩHYЉXЭ[™ЧЩ[ќ]KOњќ[›љ[™И	€•S—ФХUWФХT•Ц
B€В€XЭ[Ы€HPХSУ—Ф•SЋВ‚€Y€
XЭ[™ЧЩ[ќ]KO›[Щ[]Kњќ[—ШЫЫ™љYЧЩ›YЬИ	€•S—РУУ‘’QЧЦФ’QТСTТС’VQ€	‰€JXЭ[™ЧЩ[ќ]KO›[Щ[]Kњќ[—ШЫЫ™љYЧЩ›YЬИ	€•S—РУУ‘’QЧЦ—СХУ—СSђP“Q
JHВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HЊВ€B€[ЩB€В€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HXЭ[™ЧЩ[ќ]KO›[Щ[]Kњќ[њЬYYИЋИЛИ[Э™\И\]H\Э\€]Hќ[›љ[™В€B€B€[ЩB€В€XЭ[Ы€HPХSУ—ХРSОВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HXЭ[™ЧЩ[ќ]KO›[Щ[]KњЬYYћИЋВ€B€B€[ЩHYЉJXЭ[™ЧЬ^Y\‹OљЩ^\И	€
“QЧУSХ‘UT“QЧУSХ‘QХУЉJJB€В€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HВ€B€B€[ЩHYЉXЭ[™ЧЩ[ќ]KO™XЪЪ[™ИOHPТЧУ“У‘H	‰€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WСPТКH	‰€XЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧУSХ‘QХУ€	‰€›Э[Z\ЉB€В€ћYXЪКXЭ[™ЧЩ[ќ]JNВ€ЫЭИ[™[љШЪXЪОВ€B‚€YЉXЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧУSХ‘SQ•	‰€XЭ[™ЧЩ[ќ]KO™XЪЪ[™ИOHPТЧУ“У‘JB€В€YЉXЭ[™ЧЩ[ќ]KO™\™XЭ[Ы€OHT‘PХSУ—Ф’QТ
B€В€ЛШXЭ[™ЧЩ[ќ]KOњќ[›љ[™ИH•S—ФХUWУ“У‘NИЛИ]Z]Иќ[›љ[™ИY€^Y\€Ъ[™Щ\И\™XЭ[Ы‚€YЉXЭ[™ЧЩ[ќ]KO›[Щ[]Kќ\›™[^H	‰€XXЭ[™ЧЩ[ќ]KOќ\›ќ[YJB€В€XЭ[™ЧЩ[ќ]KOќ\›ќ[YHHЭ[YH
ИXЭ[™ЧЩ[ќ]KO›[Щ[]Kќ\›™[^NВ€B€[ЩHYЉXЭ[™ЧЩ[ќ]KOќ\›ќ[YH	‰€Э[YHЏHXЭ[™ЧЩ[ќ]KOќ\›ќ[YJB€В€XЭ[™ЧЩ[ќ]KOќ\›ќ[YHHВ€YЉ[Y[љ[JXЭ[™ЧЩ[ќ]KS’WХT“ЉJB€В€XЭ[™ЧЩ[ќ]KOќZЩXXЭ[Ы€HЫЫ[[Ы—Э\›ЋВ€Щ]Э\›љ[™КXЭ[™ЧЩ[ќ]JNВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHXЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HВ€[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WХT“‹
NВ€ЫЭИ[™[љШЪXЪОВ€B€XЭ[™ЧЩ[ќ]KO™\™XЭ[Ы€HT‘PХSУ—УQ•В€B€[ЩHYЉXXЭ[™ЧЩ[ќ]KO›[Щ[]Kќ\›™[^H	‰€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WХT“ЉJB€В€XЭ[™ЧЩ[ќ]KOќZЩXXЭ[Ы€HЫЫ[[Ы—Э\›ЋВ€Щ]Э\›љ[™КXЭ[™ЧЩ[ќ]JNВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHXЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HВ€[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WХT“‹
NВ€ЫЭИ[™[љШЪXЪОВ€B€[ЩHYЉXXЭ[™ЧЩ[ќ]KOќ\›ќ[YJB€В€XЭ[™ЧЩ[ќ]KO™\™XЭ[Ы€HT‘PХSУ—УQ•В€B€B€[ЩB€В€XЭ[™ЧЩ[ќ]KOќ\›ќ[YHHВ€B‚€YЉXЭ[™ЧЩ[ќ]KOњќ[›љ[™КB€В€XЭ[Ы€HPХSУ—Ф•SЋВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHXXЭ[™ЧЩ[ќ]KO›[Щ[]Kњќ[њЬYYИЛИY€ќ[›љ[™Л^Y\€[Э™\И]H\Э\€]B€B€[ЩHYЉXЭ[Ы€OHPХSУ—ХT	‰€XЭ[Ы€OHPХSУ—СХУЉB€В€XЭ[Ы€HPХSУ—ХРSОВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHXXЭ[™ЧЩ[ќ]KO›[Щ[]KњЬYYћВ€B€[ЩB€В€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHXXЭ[™ЧЩ[ќ]KO›[Щ[]KњЬYYћВ€B€B€[ЩHYЉXЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧУSХ‘T’QТ	‰€XЭ[™ЧЩ[ќ]KO™XЪЪ[™ИOHPТЧУ“У‘JB€В€YЉXЭ[™ЧЩ[ќ]KO™\™XЭ[Ы€OHT‘PХSУ—УQ•
B€В€ЛШXЭ[™ЧЩ[ќ]KOњќ[›љ[™ИH•S—ФХUWУ“У‘NИЛИ]Z]Иќ[›љ[™ИY€^Y\€Ъ[™Щ\И\™XЭ[Ы‚€YЉXЭ[™ЧЩ[ќ]KO›[Щ[]Kќ\›™[^H	‰€XXЭ[™ЧЩ[ќ]KOќ\›ќ[YJB€В€XЭ[™ЧЩ[ќ]KOќ\›ќ[YHHЭ[YH
ИXЭ[™ЧЩ[ќ]KO›[Щ[]Kќ\›™[^NВ€B€[ЩHYЉXЭ[™ЧЩ[ќ]KOќ\›ќ[YH	‰€Э[YHЏHXЭ[™ЧЩ[ќ]KOќ\›ќ[YJB€В€XЭ[™ЧЩ[ќ]KOќ\›ќ[YHHВ€YЉ[Y[љ[JXЭ[™ЧЩ[ќ]KS’WХT“ЉJB€В€XЭ[™ЧЩ[ќ]KOќZЩXXЭ[Ы€HЫЫ[[Ы—Э\›ЋВ€Щ]Э\›љ[™КXЭ[™ЧЩ[ќ]JNВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHXЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HВ€[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WХT“‹
NВ€ЫЭИ[™[љШЪXЪОВ€B€XЭ[™ЧЩ[ќ]KO™\™XЭ[Ы€HT‘PХSУ—Ф’QТВ€B€[ЩHYЉXXЭ[™ЧЩ[ќ]KO›[Щ[]Kќ\›™[^H	‰€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WХT“ЉJB€В€XЭ[™ЧЩ[ќ]KOќZЩXXЭ[Ы€HЫЫ[[Ы—Э\›ЋВ€Щ]Э\›љ[™КXЭ[™ЧЩ[ќ]JNВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHXЭ[™ЧЩ[ќ]KOќ™[ШЪ]Kћ€HВ€[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WХT“‹
NВ€ЫЭИ[™[љШЪXЪОВ€B€[ЩHYЉXXЭ[™ЧЩ[ќ]KOќ\›ќ[YJB€В€XЭ[™ЧЩ[ќ]KO™\™XЭ[Ы€HT‘PХSУ—Ф’QТВ€B€B€[ЩB€В€XЭ[™ЧЩ[ќ]KOќ\›ќ[YHHВ€B‚€YЉXЭ[™ЧЩ[ќ]KOњќ[›љ[™КB€В€XЭ[Ы€HPХSУ—Ф•SЋВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHXЭ[™ЧЩ[ќ]KO›[Щ[]Kњќ[њЬYYИЛИY€ќ[›љ[™Л^Y\€[Э™\И]H\Э\€]B€B€[ЩHYЉXЭ[Ы€OHPХSУ—ХT	‰€XЭ[Ы€OHPХSУ—СХУЉB€В€XЭ[Ы€HPХSУ—ХРSОВ€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHXЭ[™ЧЩ[ќ]KO›[Щ[]KњЬYYћВ€B€[ЩB€В€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHXЭ[™ЧЩ[ќ]KO›[Щ[]KњЬYYћВ€B€B€[ЩHYЉJ
XЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧУSХ‘SQ•
H€
XЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧУSХ‘T’QТ
JH
B€В€ЛШXЭ[™ЧЩ[ќ]KOњќ[›љ[™ИH•S—ФХUWУ“У‘NИЛИ^Y\€]ЫИЩ€YќЬљYЪ[™ЫИ]Z]Иќ[›љ[™В€XЭ[™ЧЩ[ќ]KOќ™[ШЪ]KћHВ€XЭ[™ЧЩ[ќ]KOќ\›ќ[YHHВ€B‚€YЉ
Э\€Hљ[™Щ[ќЪ\™JXЭ[™ЧЩ[ќ]KXЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћXЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћ‹TWТUSK^Y\—Э\ЭЭЭXЪ
JH
B€В€ЧЪ][WЬШЬљ\
XЭ[™ЧЩ[ќ]KЭ\ЉNВ€Yљ[™Ъ][JЭ\ЉNИЛИYYќ[Э[Ы€ИЫX[€ЫЩH\Hљ]€B‚€Y€
XЭ[™ЧЩ[ќ]KO™XЪЪ[™И	€PТЧРPХU‘JB€В€Y€
JXЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧУSХ‘QХУЉJB€В€ћYXЪЬљ\ЩJXЭ[™ЧЩ[ќ]JNВ€ЫЭИ[™[љШЪXЪОВ€B€YЉXЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧУSХ‘SQ•
B€В€YЉXЭ[™ЧЩ[ќ]KO™\™XЭ[Ы€OHT‘PХSУ—Ф’QТ
B€В€XЭ[™ЧЩ[ќ]KO™\™XЭ[Ы€HT‘PХSУ—УQ•В€B€B€[ЩHYЉXЭ[™ЧЬ^Y\‹OљЩ^\И	€“QЧУSХ‘T’QТ
B€В€YЉXЭ[™ЧЩ[ќ]KO™\™XЭ[Ы€OHT‘PХSУ—УQ•
B€В€XЭ[™ЧЩ[ќ]KO™\™XЭ[Ы€HT‘PХSУ—Ф’QТВ€B€B€B€Y€
XЭ[™ЧЩ[ќ]KO™XЪЪ[™КB€В€ЫЭИ[™[љШЪXЪОВ€B‚‚€ЛХЪ]HYЫЫЋ€™\\™H›Ь€Y[™И[љ[X][ЫњЛ‹‹‚€YЉXЭ[ЫЉB€В€XЭ[™ЧЩ[ќ]KOќZЩXXЭ[Ы€H•SВ€XЭ[™ЧЩ[ќ]KOљY[™ИHQS‘ЧФ‘TT‘QВ€B‚€ЭЪ]Ъ
XЭ[ЫЉB€В€Ш\ЩHPХSУ—ХРSО‚€ЛИXЪИШ[И™X]\™B€YЉ]™[	‰€[Y[љ[JXЭ[™ЧЩ[ќ]KS’WРђPТХРSКJHВ‚€YЉXЭ[™ЧЩ[ќ]KO›[Щ[]K™XЪ[™ИOHђPТS‘ЧРQ•TХФ’QТ]™[O™XЪ[™ИOHђPТS‘ЧРQ•TХФ’QТ
HВ‚€XЪЧЭШ[ИHXXЭ[™ЧЩ[ќ]KO™\™XЭ[ЫЋВ‚€H[ЩHYЉXЭ[™ЧЩ[ќ]KO›[Щ[]K™XЪ[™ИOHђPТS‘ЧРQ•TХУQ•]™[O™XЪ[™ИOHђPТS‘ЧРQ•TХУQ•
HВ‚€XЪЧЭШ[ИHXЭ[™ЧЩ[ќ]KO™\™XЭ[ЫЋВ‚€H[ЩHYЉ
XЭ[™ЧЩ[ќ]KO›[Щ[]K™XЪ[™ИOHђPТS‘ЧРQ•TХУU‘S]™[O™XЪ[™ИOHђPТS‘ЧРQ•TХУU‘S
H	‰€
]™[OњШЬ›Ы\€	€РФ“УУQ•
H	‰€XЭ[™ЧЩ[ќ]KO™\™XЭ[Ы€OHT‘PХSУ—УQ•
HВ‚€XЪЧЭШ[ИHќYNВ‚€H[ЩHYЉ
XЭ[™ЧЩ[ќ]KO›[Щ[]K™XЪ[™ИOHђPТS‘ЧРQ•TХУU‘S]™[O™XЪ[™ИOHђPТS‘ЧРQ•TХУU‘S
H	‰€
]™[OњШЬ›Ы\€	€РФ“УФ’QТ
H	‰€XЭ[™ЧЩ[ќ]KO™\™XЭ[Ы€OHT‘PХSУ—Ф’QТ
HВ‚€XЪЧЭШ[ИHќYNВ‚€H[ЩHYЉXЭ[™ЧЩ[ќ]KOќ\›ќ[YH	‰€XЭ[™ЧЩ[ќ]KO›[Щ[]Kќ\›™[^JHВ‚€XЪЧЭШ[ИHќYNВ€B‚€YЉXЪЧЭШ[КHВ€ЫЫ[[Ы—ШXЪЭШ[ЧШ[љ[JXЭ[™ЧЩ[ќ]JNИ€H[ЩHВ€ЫЫ[[Ы—ЭШ[ЧШ[љ[JXЭ[™ЧЩ[ќ]JNВ€B€€H[ЩHВ€ЫЫ[[Ы—ЭШ[ЧШ[љ[JXЭ[™ЧЩ[ќ]JNИ€B€њ™XZОВ€Ш\ЩHPХSУ—ХT‚€ЫЫ[[Ы—Э\Ш[љ[JXЭ[™ЧЩ[ќ]JNИЛЩ[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WХT
NИЛИЩ]И\[љ[X][Ы€Y€^\ЭВ€њ™XZОВ€Ш\ЩHPХSУ—СХУЋ‚€ЫЫ[[Ы—ЩЭЫ—Ш[љ[JXЭ[™ЧЩ[ќ]JNИЛЩ[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WСХУ‹
NИЛИЩ]ИЭЫ€[љ[X][Ы€Y€^\ЭВ€њ™XZОВ€Ш\ЩHPХSУ—Ф•SЋ‚€Y€
[Y[љ[JXЭ[™ЧЩ[ќ]KS’WРђPТФ•SЉJB€В€Y€
\ЧЪ[—ШXЪЬќ[ЉXЭ[™ЧЩ[ќ]JJH[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WРђPТФ•S‹
NВ€[ЩH[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WФ•S‹
NВ€B€[ЩH[ќЬЩ]Ш[љ[JXЭ[™ЧЩ[ќ]KS’WФ•S‹
NИЛИЩ]Иќ[€[љ[X][Ы€Y€^\ЭВ‚€њ™XZОВ€Y][‚‚€YЉXЭ[™ЧЩ[ќ]KOљY[™КB€В€ЫЫ[[Ы—ЪYWШ[љ[JXЭ[™ЧЩ[ќ]JNВ€B€њ™XZОВ€B‚‚™[™[љШЪXЪО‚€ЛЪ[њЩ\ќЪXЪИ\™B€™]\›ЋВџB‚љ[ќ\ЧЪ[—ШXЪЬќ[Љ[ќ]J€Щ[ЉBћВ€Y€


Щ[‹O›[Щ[]K™XЪ[™ИOHђPТS‘ЧРQ•TХФ’QТ]™[O™XЪ[™ИOHђPТS‘ЧРQ•TХФ’QТ
H	‰€Щ[‹Oќ™[ШЪ]Kћ
H€

Щ[‹O›[Щ[]K™XЪ[™ИOHђPТS‘ЧРQ•TХУQ•]™[O™XЪ[™ИOHђPТS‘ЧРQ•TХУQ•
H	‰€Щ[‹Oќ™[ШЪ]Kћ€
H€


Щ[‹O›[Щ[]K™XЪ[™ИOHђPТS‘ЧРQ•TХУU‘S]™[O™XЪ[™ИOHђPТS‘ЧРQ•TХУU‘S
H	‰€
]™[OњШЬ›Ы\€	€РФ“УФ’QТ
JH	‰€Щ[‹Oќ™[ШЪ]Kћ€
H€


Щ[‹O›[Щ[]K™XЪ[™ИOHђPТS‘ЧРQ•TХУU‘S]™[O™XЪ[™ИOHђPТS‘ЧРQ•TХУU‘S
H	‰€
]™[OњШЬ›Ы\€	€РФ“УУQ•
JH	‰€Щ[‹Oќ™[ШЪ]Kћ€
B€
H™]\›€NВ€[ЩH™]\›€ВџB‚‹ЛШ[[[ИЫЭ[ќЫЩ\ИЭЫ‚ќ›ЪYЭXќXЭЬЪЭ

BћВ€YЉЩ[‹OќЩX\[ќ	‰€Щ[‹OќЩX\[ќO›[Щ[]KќЩX\Ы—Ь›Ь\ќY\Лќ\ЩWШЫЭ[ќ
B€В€Щ[‹OќЩX\[ќO›[Щ[]KќЩX\Ы—Ь›Ь\ќY\Лќ\ЩWШЫЭ[ќKNВ‚€К‚€
€Э]Щ€\Щ\ПИ›ЬHЩX\Ы‹‚€
‹В‚€YЉ\Щ[‹OќЩX\[ќO›[Щ[]KќЩX\Ы—Ь›Ь\ќY\Лќ\ЩWШЫЭ[ќ
B€В€Щ[‹OќЩX\[ќO›[Щ[]KќЩX\Ы—Ь›Ь\ќY\Л›ЬЬЧШЫЭ[ќHВ€›ЬЩX\ЫЉЩ[‹
NВ€B€BџB‚‚ќ›ЪY›ЬЩX\ЫЉ[ќ]J€XЭ[™ЧЩ[ќ]K[ќ›YКBћВ€[ќШ[HВ€[ќ]H
›Э\€H•SВ€[ќ]J€ЩX\Ы—Щ[ќ]HH•SВ€ЧЭЩX\ЫЉ€ЩX\Ы—Ь›Ь\ќY\ИH•SВ‚‚KЛИY€ЩH[™XYH]™HHЩX\Ы‹ЩIЫ™YYИ\ШШ\™]‚€YЉXЭ[™ЧЩ[ќ]KOќЩX\[ќ
B€В€К‚€
€[\Ъ[ќ\њИИЩ[‰ЬИЩX\Ы€[ќ]H[™H€
€ЩX\Ы€[ќ]IЬИ[Щ[]HЩX\Ы€›Ь\ќY\И€
€[ќИШШ[\љXX›\Л€\И\Иќ\Э›Ь€XZ\Щ\€€
€™XY[™ИЭЫњЭ™X[K‚€
‹В‚€ЩX\Ы—Щ[ќ]HHXЭ[™ЧЩ[ќ]KOќЩX\[ќВ€ЩX\Ы—Ь›Ь\ќY\ИH	XЭ[™ЧЩ[ќ]KOќЩX\[ќO›[Щ[]KќЩX\Ы—Ь›Ь\ќY\ОВ‚‚BKЛИЊNKLKLЋHH›ЭЭ\™HX›Э]\ИЩЪXЛ€]\X\њИ]Ы›H\HЪЭ‚BKЛИЩX\ЫњИЬ€ЩX\ЫњИЪ]ЪЭ[[[И\™H›ЬY€[ћ][™И[ЩH\ИЪ[\H\ШШ\™Y‚‚BKЛИ™YYИ][X]H[ЩX\Ы€ЩЪXИИЩ]HЫЬљЩ›ЭЛ‚€YЉЩX\Ы—Ь›Ь\ќY\ЛOќЩX\Ы—ЬЭ]H	€СPTУ—ФХUWУSRUQХTСH
JЩX\Ы—Ь›Ь\ќY\ЛOќЩX\Ы—ЬЭ]H	€СPTУ—ФХUWУSRUQХTСJH	‰€ЩX\Ы—Ь›Ь\ќY\ЛOќ\ЩWШЫЭ[ќ
JB€И‚BBKЛИY€H›YИ\И€Ь€™[ЭЛЩHЭXќXЭH›YЙЬВ‚BBKЛИ[YHњ›ЫHЩX\Ы€ЫЭ[ќ\‹‚‚BBZYЉ›YИЉB€В€ЩX\Ы—Ь›Ь\ќY\ЛO›ЬЬЧШЫЭ[ќOH›YОВ€B€‚BBKЛИЩIЬ™HЫЪ[™ИИ\ЩHЭ\€ЭЫ€ЬЪ][Ы€›Ь€HЩX\Ы‹‚€ЩX\Ы—Щ[ќ]KO™\™XЭ[Ы€HXЭ[™ЧЩ[ќ]KO™\™XЭ[ЫЋВ€ЩX\Ы—Щ[ќ]KOњЬЪ][Ы‹ћ€HXЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћЋВ€ЩX\Ы—Щ[ќ]KOњЬЪ][Ы‹ћHXЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћВ€ЩX\Ы—Щ[ќ]KOњЬЪ][Ы‹ћHHXЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћNВ‚‚BBKЛИЩ][ћHШ[И[™]›Ь›\Л‚€Э\€HЪXЪЧЬ]›Ь›JЩX\Ы—Щ[ќ]KOњЬЪ][Ы‹ћЩX\Ы—Щ[ќ]KOњЬЪ][Ы‹ћ‹XЭ[™ЧЩ[ќ]JNВ€Ш[HЪXЪЭШ[Ъ[™^
ЩX\Ы—Щ[ќ]KOњЬЪ][Ы‹ћЩX\Ы—Щ[ќ]KOњЬЪ][Ы‹ћЉNВ‚‚BBKЛИXЩHЫќИШ[Ь€]›Ь›K‚€YЉЭ\€	‰€Э\€OHЩX\Ы—Щ[ќ]JB€В€ЩX\Ы—Щ[ќ]KO\ЩH
ПHЭ\‹OњЬЪ][Ы‹ћH
ИЭ\‹O[љ[X][Ы‹Oњ]›Ь›VЫЭ\‹O[љ[\ЬЧVФU“Ф“WТRQТNВ€B€[ЩHYЉШ[ЏH
B€В€ЩX\Ы—Щ[ќ]KO\ЩH
ПH]™[OќШ[ЦЭШ[KљZYЪВ€B‚‚BBKЛИ\ЩHHЩX\Ы‰ЬИ‘TФUУ€Ь€ФUУ€[љ[X][ЫњИY€]Z[X›KЭ\ќЪ\ЩB‚BBKЛИЫИљYЪИYK‚€YЉ[Y[љ[JЩX\Ы—Щ[ќ]KS’WФ‘TФUУЉJB€В€[ќЬЩ]Ш[љ[JЩX\Ы—Щ[ќ]KS’WФ‘TФUУ‹JNВ€B€[ЩHYЉ[Y[љ[JЩX\Ы—Щ[ќ]KS’WФФUУЉJB€В€[ќЬЩ]Ш[љ[JЩX\Ы—Щ[ќ]KS’WФФUУ‹JNВ€B€[ЩB€В€YЉ[Y[љ[JЩX\Ы—Щ[ќ]KS’WТQJJH[ќЬЩ]Ш[љ[JЩX\Ы—Щ[ќ]KS’WТQKJNВ€B‚‚BBKЛИY€HЩX\Ы‰ЬИЫЭ[ќ\€\И\]Y[€ЩX\Ы€\ИЬЭ›Ь€ЫЫЩ‚‚BBKЛИY€]\И[€[љ[X[‹[€ЩH\HH[љ[X[ќ[›љ[™И]Ш^HЩЪXЛ‚‚BBKЛИЭ\ќЪ\ЩHHЩX\Ы€›[љЬИЭ]‚€YЉ]ЩX\Ы—Ь›Ь\ќY\ЛO›ЬЬЧШЫЭ[ќ
B€В€YЉJXЭ[™ЧЩ[ќ]KO›[Щ[]KќЩX\Ы—Ь›Ь\ќY\ЛќЩX\Ы—ЬЭ]H	€СPTУ—ФХUWРS’SPS
JB€В€ЩX\Ы—Щ[ќ]KO›[љИHNВ€ЩX\Ы—Щ[ќ]KOќZЩXXЭ[Ы€HЫЫ[[Ы—ЫYNВ€B€[ЩB€В€ЩX\Ы—Щ[ќ]KO›[Щ[]Kќ\HHTWУ“У‘NВ€ЩX\Ы—Щ[ќ]KOќ[љИHќ[[љ[X[В€B€B€ЩX\Ы—Щ[ќ]KO›™^[љИHЭ[YH
ИNВ€B‚‚BKЛИЫX\€Э\€XЪЪ[™И\љXX›H]ЩY\ИHЩX\Ы€[ќ]HЪ[ќ\‹‚€XЭ[™ЧЩ[ќ]KOќЩX\[ќH•SВ€B‚‚KЛИ›YИ€YX[њИЩIЬ™H›ШX›HЩ][™ИHЩX\Ы€\™XЭH
^€Щ]ЩX\Ы€ЫЫ[X[™
K€‚KЛИ[€]Ш\ЩHЩHЫ‰ЭЫЬњћHX›Э]HЩX\Ы€[ќ]K€ќ\ЭЭЪ]ЪЭ\њЩ[™\ИЭ™\‚‚KЛИИHЩX\Ы€[Щ[‚€YЉ›YИЉB€В€YЉXЭ[™ЧЩ[ќ]KO›[Щ[]Kќ\H	€TWФVQTЉB€В€YЉ^Y\–ШXЭ[™ЧЩ[ќ]KOњ^Y\љ[™^KќЩX\ќ[JB€В€Щ]ЭЩX\ЫЉXЭ[™ЧЩ[ќ]K^Y\–ШXЭ[™ЧЩ[ќ]KOњ^Y\љ[™^KќЩX\ќ[K
NВ€B€[ЩB€В€Щ]ЭЩX\ЫЉXЭ[™ЧЩ[ќ]K]™[OњЩ]ЩX\
NВ€B€B€[ЩB€В€Щ]ЭЩX\ЫЉXЭ[™ЧЩ[ќ]K
NВ€B€B‚‚KЛИ[Щ[Э™\њљYK€Y€\И\ИЬ[]YЩH\ЩH]И[YB‚KЛИИШШ]HH[Щ[ћH[™^[™™]™\ќИ][њЭXY‚KЛИЩ€HY][[Щ[Ъ[€HЩX\Ы€\ИЬЭ‚€YЉXЭ[™ЧЩ[ќ]KO›[Щ[]KќЩX\Ы—Ь›Ь\ќY\Л›ЬЬЧЪ[™^OHSСSТS‘VУ“У‘JB€В€Щ]ЭЩX\ЫЉXЭ[™ЧЩ[ќ]KXЭ[™ЧЩ[ќ]KO›[Щ[]KќЩX\Ы—Ь›Ь\ќY\Л›ЬЬЧЪ[™^
NВ€BџB‚‚љ[ќ^Y\—ЭZЩY[XYЩJ[ќ]J€\™Щ]Щ[ќ]K[ќ]J€]XЪЪ[™ЧЩ[ќ]KЧШ]XЪК€]XЪЧЫШљ™XЭ[ќ[Щ›YЛЫЫњЭЧЩY™[њЩJ€Y™[њЩWЫШљ™XЭ
BћВ€ЧШ]XЪИ]ИH
]XЪЧЫШљ™XЭВ€ЛЬљ[ќЉ™[XYЩYћN€	Й\ЙИ	Y€‹Э\‹O›[YK]XЪЛO]XЪЧЩ›ЬЩJNВ‚‚KЛИЬ]\И
LLЊЊJH›ЭИHљ[™љ[љ]HX[ЪX]€Ъ[ЪXЪИH[XYЩHЫЭ\ЩK]Ъ[]›ЪYЫЫYHњЬXЪX[€[XYЩHЫЭ\Щ\В‚‚KЛИ[XYЩHЫЫY\Ињ›ЫHH›Ь›X[ЫЭ\ЩOВ‚XЫЫњЭ›ЫЫ›Ь›X[Щ[XYЩHH
Z\ЧШ]XЪЧЭ\WЬЬXЪX[
]Л]XЪЧЭ\JJHИќYH€[ЩNВ‚€YЉ
ЫШ[ШЫЫ™љYЛЪX]И	€ТPUУФSУ”ЧТPSРPХU‘H	‰€›Ь›X[Щ[XYЩJB€
]™[O››Ъ\ќOHSPQСWС”“УWСS‘SVWУС‘€	‰€
]XЪЪ[™ЧЩ[ќ]KO›[Щ[]Kќ\H	€TWСS‘SVJJJB€В€]Л]XЪЧЩ›ЬЩHHВ€B€€™]\›€ЫЫ[[Ы—ЭZЩY[XYЩJ\™Щ]Щ[ќ]K]XЪЪ[™ЧЩ[ќ]K	]Л[Щ›YЛY™[њЩWЫШљ™XЭ
NВџB‚‹ЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛВ‚‹ЛИШ[YЪ[€^Y\€™KY[ќ\њИHШ[YK‚‹ЛИ›Ь[[™[ZY\ИVСT›Ь€H[љЩYЩњ›Ю™[€Ы™\Л‚ќ›ЪY›ЬШ[Щ[™[ZY\К
BћВ€[ќNВ€[ќ]H
ќЩX\Щ[€HЩ[ЋВ€ЧШ]XЪИ]XЪИH[\X]XЪОВ‚€›ЬЉHHИH[ќЫX^ИJККB€В€YЉ[ќЫ\ЭЪWKO™^\ЭИ	‰‚€[ќЫ\ЭЪWKO™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќ€	‰‚€
[ќЫ\ЭЪWKO›[Щ[]Kќ\H	€TWСS‘SVJH	‰‚€Y[ќЫ\ЭЪWKO›ЭЫ™\€	‰€ЛИЫ‰ЭШ[ќИЫ›ШЪИЭЫ€H›Ъ™XЭ[B€Y[ќЫ\ЭЪWKO™њ›Ю™[€	‰€ЛИЫ‰ЭШ[ќИ[™њ™Y^™HHњ›Ю™[€[™[^B€J[ќЫ\ЭЪWKO›[Щ[]K›[Э™WШЫЫ™љYЧЩ›YЬИ	€SХ‘WРУУ‘’QЧУ“ЧУSХ‘JH	‰‚€J[ќЫ\ЭЪWKO›[Щ[]KњZ[—ШЫЫ™љYЧЩ›YЬИ	€RS—РУУ‘’QЧСђSСTРP“JH	‰‚€[Y[љ[J[ќЫ\ЭЪWKS’WСђS
H
B€В€[ќЫ\ЭЪWKO]XЪЪ[™ИHUPТТS‘ЧУ“У‘NВ€[ќЫ\ЭЪWKOњ›Ъ™XЭ[HH“TХУ“У‘NВ€[ќЫ\ЭЪWKOќZЩXXЭ[Ы€HЫЫ[[Ы—Щ[ЛЛЩ[™[^WЩ[В€[ќЫ\ЭЪWKO™[XYЩWЫЫ—Ы[™[™Л]XЪЧЩ›ЬЩHHВ€[ќЫ\ЭЪWKO™[XYЩWЫЫ—Ы[™[™Л]XЪЧЭ\HHUЧУ“У‘NВ€Щ[€H[ќЫ\ЭЪWNВ€[ќЭ[›[љКЩ[ЉNВ€[ќЫ\ЭЪWKOќ™[ШЪ]KћH
Щ[‹O™\™XЭ[Ы€OHT‘PХSУ—Ф’QТ
HИ
LKЊЉH€KЊЋВ€€YЉ[ќЫ\ЭЪWKO›[Щ[]KќЩX\Ы—Ь›Ь\ќY\Л›ЬЬЧШЫЫ™][Ы€	€СPTУ—УФФЧРУУ‘USУ—ФХQСJB€В€›ЬЩX\ЫЉЩ[‹JNВ€B‚€ЬЬК[ќЫ\ЭЪWK‹ЌH
И[™ЉJJNВ€[ќЫ\ЭЪWKOљЫ›ШЪЩЭЫЫЭ[ќH[ќЫ\ЭЪWKO›[Щ[]KљЫ›ШЪЩЭЫЫЭ[ќВ‚€[ќЫ\ЭЪWKOљЫ›ШЪЩЭЫќ[YHHВ‚€Щ]Щ[
[ќЫ\ЭЪWKЩ[‹	]XЪЛJNВ€B€B€Щ[€HЩX\Щ[ЋВџB‚‚‚‹К€Ш[YЪ[€›ЬЬИY\Л€
‹Вќ›ЪYЪ[Ш[Щ[™[ZY\К
BћВ€[ќNВ€ЧШ]XЪИ]XЪИH[\X]XЪОВ€[ќ]H
ќ\Щ[€H•SВ‚‚X]XЪЛ]XЪЧЭ\HHUЧР“ФФЧСPUВ‚X]XЪЛ™›Ь€HY][Ы[Щ[Щ›ЬЋВ‚€К€€
€ЭЫњЭ™X[H[XYЩHќ[Э[ЫњИ\ЩH€
€Щ[€ЫШ[ЫИЩHЬ[]H]Ъ]€
€[ќ]HЭ\њЫЬ€[€XXЪ]\][Ы€Щ€€
€ЫЬ™[ЭЛ€ЩY\Э\њ™[ќ[YH\™H€
€ЫИЩHШ[€™\ЭЬ™HЪ[€ЫЬ\Иљ[љ\ЪY€€
‹В€\Щ[€HЩ[ЋВ‚€К€€
€]™\ћH[Y[™[^H\HЪ]HZЩY[XYЩB€
€ќ[Э[Ы€ZЩ\И]ИЭ\њ™[ќX[[€[XYЩK‚€
‹В€›ЬЉHHИH[ќЫX^ИJККB€В€YЉ[ќЫ\ЭЪWKO™^\ЭВ€	‰€[ќЫ\ЭЪWKO™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќ€€	‰€
[ќЫ\ЭЪWKO›[Щ[]Kќ\H	€TWСS‘SVJB€	‰€[ќЫ\ЭЪWKOќZЩY[XYЩJB€В€Щ[€H[ќЫ\ЭЪWNВ‚€]XЪЛ]XЪЧЩ›ЬЩHHЩ[‹O™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќВ€Щ[‹OќZЩY[XYЩJЩ[‹Щ[‹	]XЪЛЩ[‹O™Y™[њЩJNВ€B€B‚€Щ[€H\Щ[ЋИџB‚‚‚ќ›ЪYЫX\ќШ›ЫXЉ[ќ]H
™KЧШ]XЪИ
]XЪКHИЛИ™]ИY]Щ›Ь€ЫX\ќ›ЫXњВ‚€[ќK]HВ€[ќ]H
ќ\Щ[€H•SВ‚€\Щ[€HЩ[ЋВ€›ЬЉHHИH[ќЫX^ИJККHВ‚€YЉ[ќЫ\ЭЪWKO™^\ЭВ€	‰€[ќЫ\ЭЪWHOHB€	‰€[ќЫ\ЭЪWKO™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќ€€	‰€XЭ[Ы—ШЪXЪЧЪ\ЧЪЬЭ[JK[ќЫ\ЭЪWJJHВ€€Щ[€H[ќЫ\ЭЪWNВ€]HNИЛИ›Ь€›ШЫЬЭY€H›ЫX€Щ\Ы‰Э]]ЫЫ‰ЭЫЬЭ[™\™ЮB‚€\Э]]XЪИH]XЪОВ€\Э]]XЪЩ\€HNВ€\Э]ЫЫ\Ъ[Ы—Ш]XЪИH•SВ€\Э]ЫЫ™љ\›HHNВ€\Э]™]XЭШ›ЩHH•SВ€\Э]™]XЭШЫЫ\Ъ[Ы—Ш]XЪИH•SВ€\Э]™]XЭШЫЫ\Ъ[Ы—Ш›ЩHH•SВ€\Э]њЬЪ][Ы‹ћHЩ[‹OњЬЪ][Ы‹ћВ€\Э]њЬЪ][Ы‹ћHHЩ[‹OњЬЪ][Ы‹ћNВ€\Э]њЬЪ][Ы‹ћ€HЩ[‹OњЬЪ][Ы‹ћЋВ€\Э]ќ\™Щ]HЩ[ЋВ‚€YЉЩ[‹OќZЩY[XYЩJHВ‚€ЫЫњЭЧЩY™[њЩJ€Y™[њЩWЫШљ™XЭHY™[њЩWЩљ[™ШЭ\њ™[ќЫШљ™XЭ
Щ[‹•S]XЪЛO]XЪЧЭ\JNВ€€ЛШ]XЪЛ]XЪЧЩ›ЬHЩ[‹O›[Щ[]KљЫ›ШЪЩЭЫЫЭ[ќ
МNВ€Щ[‹OќZЩY[XYЩJЩ[‹K]XЪЛY™[њЩWЫШљ™XЭ
NВ€€H[ЩHВ€Щ[‹O™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќOH]XЪЛO]XЪЧЩ›ЬЩNВ€€YЉЩ[‹O™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќH
HВ€Ъ[Щ[ќ]JЩ[‹ТSСS•UWХ’QССT—ФУPT•“УPЉNВ€B€B‚€Ь]Ы—Ш]XЪЧЩ›\Ъ
Щ[‹]XЪЛ]XЪЛO™›\Ъ›[Щ[Ъ]Щ[‹O›[Щ[]K™›\Ъ›[Щ[Ш›ШЪКNВ€€B€B‚€YЉ›ШЫЬЭ	‰€]	‰€ЫX\ќ›ЫX™\ЉHИЛИЫ‰Э\ЩHK™XШ]\ЩH\ИШ[€™H[€][KX›ЫX‚€Щ[€HЫX\ќ›ЫX™\ЋВ€€YЉЪXЪЧЩ[™\™ЮJS‘T‘ЦWХTWУTS’WФФPТPS
JHВ€Щ[‹O™[™\™ЮWЬЭ]K›\ШЭ\њ™[ќOHЩ[‹O›[Щ[]K[љ[X][Ы–РS’WФФPТPSKO™[™\™ЮWШЫЬЭЫЬЭВ€H[ЩHВ€Щ[‹O™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќOHЩ[‹O›[Щ[]K[љ[X][Ы–РS’WФФPТPSKO™[™\™ЮWШЫЬЭЫЬЭВ€B€€B‚€Щ[€H\Щ[ЋВ‚џB‚‚‹ЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛВ‚ќ›ЪY[ћ][™ЧЭШ[К
BћВ€YЉЩ[‹OњЬЪ][Ы‹ћY[Щ^HЩ[‹OњЬЪ][Ы‹ћ€Y[Щ^
И
љY[Ы[Щ\Лљ™\И
И
JB€В€Ъ[Щ[ќ]JЩ[‹ТSСS•UWХ’QССT—ХРSЧУХUУС—Р“ХS‘КNВ€™]\›ЋВ€B€ЛЬЩ[‹OњЬЪ][Ы‹ћ
ПHЩ[‹Oќ™[ШЪ]KћВџB‚‹ЛИШ\ЪЩ^K[[Ы€‹‚‹ЛИЊNKLL‹LN‹ЛВ‹ЛИ\HЫЫЬ€Щ]Yќ\ЭY[ќИ[ќ]KЬЬЪX›B‹ЛИ\ЩYЫ€H\™[ќЫЭЫ™\€\[™[™ИЫ€ЫЫЬ€Щ]‹ЛИYќ\ЭY[ќЩ][™Л‚ќ›ЪY\WШЫЫЬ—ЬЩ]ШYќ\Э
[ќ]J€[ќ[ќ]J€\™[ќWШЫЫЬ—ШYќ\ЭYќ\ЭY[ќ
BћВ‚Z[ќHHИЛИЫЬЭ\њЫЬ‹‚‚‚KЛИ\HЫЫЬ€Щ][™Л‚‚\ЭЪ]Ъ
Yќ\ЭY[ќ
B‚^В‚YY][‚‚B‚BKЛИ\ЩHYќ\ЭY[ќ[YH\ИЫЫЬ€Щ][™^‚‚BY[ќЬЩ]ШЫЫЭ\›X\
[ќYќ\ЭY[ќ
NВ‚BXњ™XZОВ‚B‚XШ\ЩHУУФ”СUРQ•TХУ“У‘N‚‚BB‚BKЛИИ›Э[™Л‚BB‚BXњ™XZОВ‚B‚XШ\ЩHУУФ”СUРQ•TХФT‘S•ТS‘V‚‚‚BKЛИШШ]H\™[ќ	ЬИЭ\њ™[ќЫЫЬ€Щ][™^€[‚‚BKЛИЩ]Э\€ЫЫЬ€Щ]ћH][™^‚‚‚BY›Ь€
HHИH\™[ќO›[Щ[]K›X\ЧЫШYYИJККB‚B^В‚BBZY€
\™[ќOЫЫЭ\›X\OH\™[ќO›[Щ[]KЫЫЭ\›X\ЪWJB‚BB^В‚BBBY[ќЬЩ]ШЫЫЭ\›X\
[ќJNВ‚BBBXњ™XZОВ‚BB_B‚B_B‚BXњ™XZОВ‚‚XШ\ЩHУУФ”СUРQ•TХФT‘S•ХP“N‚‚BB‚BKЛИ\ЩH\™[ќ	ЬИЫЫЬ€X›K‚‚‚BY[ќOЫЫЭ\›X\H\™[ќOЫЫЭ\›X\В‚BXњ™XZОВ‚‚_BџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ‚Љ€ЊNKLL‹LЊ‚Љ€ЫЬHHXЭ[Ы€Щ][™ЬИњ›ЫHHЉ€ЫЭ\ЩH[ќ]K‚Љ‹Иќ›ЪYXЭ[Ы—ШЫЬWШ[
[ќ]J€\Э[ќ]J€ЫЭ\ЩJBћВ€XЭ[Ы—ШЫЬWЩ]J	™\ЭO›[Щ[]K™XЭ[Ы‹	њЫЭ\ЩKO™XЭ[ЫЉNВ€XЭ[Ы—ШЫЬWЩ]J	™\ЭO™XЭ[Ы‹	њЫЭ\ЩKO™XЭ[ЫЉNВџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊЊЛL‹LЋЉ€Љ€ЫЬHXЭ[Ы€›Ь\ќY\ИY€Љ€ЫЫ™][ЫњИ\ЬЛ‚Љ‹Вќ›ЪYXЭ[Ы—ШЫЬWЩ]JЧЩXЭ[ЫЉ€\ЭЧЩXЭ[ЫЉ€ЫЭ\ЩJHВ‚€К‚€
€H\Э[][Ы€\И[Ш^\ИHXЭ[Ы‚€
€Шљ™XЭ™YШ\™\ЬИЩ€ЪXЪXЭ[Ы‚€
€›Ь\ќY\И]X[YћH›Ь€ЫЬZ[™Л‚€
‹В€\ЭO›Шљ™XЭЭ\HHР’‘PХХTWСђPХSУЋВ‚€К‚€
€XЭ[Ы€Ь›Э\Л€Ы›HЫЬHY€ЫЭ\ЩHЬ€€
€\Э[][Ы€\ИH[YH[™Щ\И›Э€
€]™HH›ИЫЬH›YЛ‚€
€€
€IЩ]\€™HX›HИЫЬH[\H[Y\И€
€\ИЩ[ќ]]™\]Z\™\И[њЩ\ќ[™И€
€Y][[Y\И[€Щ]™\[Y™™\›ќ€
€ШШ][ЫњИ\›Э[™HЫЩK€™]\€В€
€ЩY\]\™H[™XZ[ќZ[X›K€€
‹В‚€Y€
ЫЭ\ЩKO™[XYЩWЩ\™XЭOHђPХSУ—СФ“ХTУ“У‘H€	‰€JЫЭ\ЩKO™[XYЩWЩ\™XЭ	€ђPХSУ—СФ“ХTУ“ЧРУФJH€	‰€J\ЭO™[XYЩWЩ\™XЭ	€ђPХSУ—СФ“ХTУ“ЧРУФJJHВ€\ЭO™[XYЩWЩ\™XЭHЫЭ\ЩKO™[XYЩWЩ\™XЭИ€B‚€Y€
ЫЭ\ЩKO™[XYЩWЪ[™\™XЭOHђPХSУ—СФ“ХTУ“У‘H€	‰€JЫЭ\ЩKO™[XYЩWЪ[™\™XЭ	€ђPХSУ—СФ“ХTУ“ЧРУФJH€	‰€J\ЭO™[XYЩWЪ[™\™XЭ	€ђPХSУ—СФ“ХTУ“ЧРУФJJHВ€\ЭO™[XYЩWЪ[™\™XЭHЫЭ\ЩKO™[XYЩWЪ[™\™XЭИ€B€€Y€
ЫЭ\ЩKOљЬЭ[HOHђPХSУ—СФ“ХTУ“У‘H€	‰€JЫЭ\ЩKOљЬЭ[H	€ђPХSУ—СФ“ХTУ“ЧРУФJH€	‰€J\ЭOљЬЭ[H	€ђPХSУ—СФ“ХTУ“ЧРУФJJHВ€\ЭOљЬЭ[HHЫЭ\ЩKOљЬЭ[NИ€B€€Y€
ЫЭ\ЩKO›Y[X™\€OHђPХSУ—СФ“ХTУ“У‘H€	‰€JЫЭ\ЩKO›Y[X™\€	€ђPХSУ—СФ“ХTУ“ЧРУФJH€	‰€J\ЭO›Y[X™\€	€ђPХSУ—СФ“ХTУ“ЧРУФJJHВ€\ЭO›Y[X™\€HЫЭ\ЩKO›Y[X™\ЋИ€B‚€К‚€
€\\Л€Ш[YHќ[H\ИXЭ[Ы€Ь›Э\Л‚€
‹В‚€Y€
ЫЭ\ЩKOќ\WЩ[XYЩWЩ\™XЭOHTWХS‘PУT‘Q€	‰€JЫЭ\ЩKOќ\WЩ[XYЩWЩ\™XЭ	€TWУ“ЧРУФJH€	‰€J\ЭOќ\WЩ[XYЩWЩ\™XЭ	€TWУ“ЧРУФJJHВ€€\ЭOќ\WЩ[XYЩWЩ\™XЭHЫЭ\ЩKOќ\WЩ[XYЩWЩ\™XЭИ€B‚€Y€
ЫЭ\ЩKOќ\WЩ[XYЩWЪ[™\™XЭOHTWХS‘PУT‘Q€	‰€JЫЭ\ЩKOќ\WЩ[XYЩWЪ[™\™XЭ	€TWУ“ЧРУФJH€	‰€J\ЭOќ\WЩ[XYЩWЪ[™\™XЭ	€TWУ“ЧРУФJJHВ€\ЭOќ\WЩ[XYЩWЪ[™\™XЭHЫЭ\ЩKOќ\WЩ[XYЩWЪ[™\™XЭИ€B‚€Y€
ЫЭ\ЩKOќ\WЪЬЭ[HOHTWХS‘PУT‘Q€	‰€JЫЭ\ЩKOќ\WЪЬЭ[H	€TWУ“ЧРУФJH€	‰€J\ЭOќ\WЪЬЭ[H	€TWУ“ЧРУФJJHВ€\ЭOќ\WЪЬЭ[HHЫЭ\ЩKOќ\WЪЬЭ[NИ€HџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊЊ‹LKLЌЉ‚Љ€™XYH^\™Э[Y[ќ›Ь€[Щ[ЫЬH›YВЉ€[™Э]]\›ЬљX]HЫЫњЭ[ќ‚Љ‹В™XЭ[Ы—ЩЬ›Э\ЫX\ЪЧЭXЭ[Ы—ЩЩ]Щ›YЧЩњ›ЫWЬЭљ[™КЫЫњЭЪ\Љ€[YJHВ‚€ЫЫњЭЭќXЭВ€ЫЫњЭЪ\Љ€^Ы[YNВ€XЭ[Ы—ЩЬ›Э\ЫX\ЪЧЭ›YОВ‚€H›YЧЫЫЪЭ\ЭX›VЧHHВ€И››Ы™H‹ђPХSУ—СФ“ХTУ“У‘HK€И[‹ђPХSУ—СФ“ХTРSУ“Ф“PSK€И[‹ђPХSУ—СФ“ХTРSУ“Ф“PSМK€И[H‹ђPХSУ—СФ“ХTРSУ“Ф“PSМHK€И›™]][‹ђPХSУ—СФ“ХTУ‘UUђSK€И››ЧШЫЬH‹ђPХSУ—СФ“ХTУ“ЧРУФHK€Ињ^Y\—Э™\њЩ\И‹ђPХSУ—СФ“ХTФVQT—Х‘T”СTИK€Иќ\WЩ^Ы\Ъ]™H‹ђPХSУ—СФ“ХTХTWСVУTТU‘HK€Иќ\WЪ[Ы\Ъ]™H‹ђPХSУ—СФ“ХTХTWТSђУTТU‘HK€ИH‹ђPХSУ—СФ“ХTРHK€И€‹ђPХSУ—СФ“ХTР€K€ИИ‹ђPХSУ—СФ“ХTРИK€И™‹ђPХSУ—СФ“ХTСK€И™H‹ђPХSУ—СФ“ХTСHK€И™€‹ђPХSУ—СФ“ХTС€K€И™И‹ђPХSУ—СФ“ХTСИK€Иљ‹ђPХSУ—СФ“ХTТK€ИљH‹ђPХSУ—СФ“ХTТHK€Иљ€‹ђPХSУ—СФ“ХTТ€K€ИљИ‹ђPХSУ—СФ“ХTТИK€И›‹ђPХSУ—СФ“ХTУK€И›H‹ђPХSУ—СФ“ХTУHK€И›€‹ђPХSУ—СФ“ХTУ€K€И›И‹ђPХSУ—СФ“ХTУИK€Ињ‹ђPХSУ—СФ“ХTФK€ИњH‹ђPХSУ—СФ“ХTФHK€Ињ€‹ђPХSУ—СФ“ХTФ€K€ИњИ‹ђPХSУ—СФ“ХTФИK€Иќ‹ђPХSУ—СФ“ХTХK€ИќH‹ђPХSУ—СФ“ХTХHK€Иќ€‹ђPХSУ—СФ“ХTХ€K€ИќИ‹ђPХSУ—СФ“ХTХИK€Ић‹ђPХSУ—СФ“ХTЦK€ИћH‹ђPХSУ—СФ“ХTЦHK€Ић€‹ђPХSУ—СФ“ХTЦ€K€ИLH‹ђPХSУ—СФ“ХTРLHK€ИЊH‹ђPХSУ—СФ“ХTРЊHK€ИМH‹ђPХSУ—СФ“ХTРМHK€И™H‹ђPХSУ—СФ“ХTСHK€И™LH‹ђPХSУ—СФ“ХTСLHK€И™ЊH‹ђPХSУ—СФ“ХTСЊHK€И™МH‹ђPХSУ—СФ“ХTСМHK€ИљH‹ђPХSУ—СФ“ХTТHK€ИљLH‹ђPХSУ—СФ“ХTТLHK€ИљЊH‹ђPХSУ—СФ“ХTТЊHK€ИљМH‹ђPХSУ—СФ“ХTТМHK€И›H‹ђPХSУ—СФ“ХTУHK€И›LH‹ђPХSУ—СФ“ХTУLHK€И›ЊH‹ђPХSУ—СФ“ХTУЊHK€И›МH‹ђPХSУ—СФ“ХTУМHK€ИњH‹ђPХSУ—СФ“ХTФHK€ИњLH‹ђPХSУ—СФ“ХTФLHK€ИњЊH‹ђPХSУ—СФ“ХTФЊHK€ИњМH‹ђPХSУ—СФ“ХTФМHK€ИќH‹ђPХSУ—СФ“ХTХHK€ИќLH‹ђPХSУ—СФ“ХTХLHK€ИќЊH‹ђPХSУ—СФ“ХTХЊHK€ИќМH‹ђPХSУ—СФ“ХTХМHK€ИћH‹ђPХSУ—СФ“ХTЦHK€ИћLH‹ђPХSУ—СФ“ХTЦLHK€ИћЊH‹ђPХSУ—СФ“ХTЦЊHB€NВ‚€ЫЫњЭЪ^™WЭ\ЭШЫЭ[ќHЪ^™[ЩЉ›YЧЫЫЪЭ\ЭX›JHИЪ^™[ЩЉ
™›YЧЫЫЪЭ\ЭX›JNВ‚€›Ь€
Ъ^™WЭHHИH\ЭШЫЭ[ќИJККHВ€Y€
ЭљXЫ\
[YK›YЧЫЫЪЭ\ЭX›VЪWKќ^Ы[YJHOH
HВ€™]\›€›YЧЫЫЪЭ\ЭX›VЪWK™›YОВ€B€B‚€К‚€
€ЫЭ[‰Эљ[™HX]Ъ[€HЫЪЭ\€
€X›K€Щ[™[\ќИЩИ[™™]\›‚€
€›Ы™H›YЛ‚€
‹В‚€љ[ќЉ——€[љЫ›ЭЫ€XЭ[Ы€
	\КK€€‹[YJNВ€™]\›€ђPХSУ—СФ“ХTУ“У‘NВџB‚‹К‚‹HШ\ЪЩ^K[[Ы€‹‚‹HЊЌ‹LLLB‹B‹H™XYXЭ[Ы€\™Э[Y[ќИ\™XЭHњ›ЫHHЫЭ\ЩH[™H[™€ЫЫXљ[™HZ\€ЫЬњ™\ЬЫ™[™ИЬ›Э\›YЬЛ‚Љ‹В™XЭ[Ы—ЩЬ›Э\ЫX\ЪЧЭXЭ[Ы—ЩЩ]Щ›YЬЧЩњ›ЫWШЫЫ[X[™Ы[™J€ЫЫњЭЪ\Љ€ЫЫ[X[™Ы[™BЉBћВ€ЫЫњЭЪ\Љ€[YNВ€ЧШЫЫ[X[™Ш\™Э[Y[ќЬ™XY\€™XY\ЋВ€XЭ[Ы—ЩЬ›Э\ЫX\ЪЧЭ™\Э[HђPХSУ—СФ“ХTУ“У‘NВ‚€ЫЫ[X[™Ш\™Э[Y[ќЬ™XY\—Ъ[љ]X[^™J	њ™XY\‹ЫЫ[X[™Ы[™KJNВ‚€Ъ[JЫЫ[X[™Ш\™Э[Y[ќЬ™XY\—Ы™^
	њ™XY\‹	ќ[YJJHВ€™\Э[HXЭ[Ы—ЩЩ]Щ›YЧЩњ›ЫWЬЭљ[™К[YJNВ€B‚€™]\›€™\Э[ВџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊЊЛL‹LЌ‚Љ‚Љ€™]\›€ќYHY€XЭ[™И[ќ]HШ[€Љ€]\™Щ][ќ]HЪ]]XЪЬЛ‚Љ‹В›ЫЫXЭ[Ы—ШЪXЪЧШШ[—Щ[XYЩJ[ќ]J€XЭ[™ЧЩ[ќ]K[ќ]J€\™Щ]Щ[ќ]KЫЫњЭ›ЫЫ[™\™XЭ
HВ‚€WЩ[ќ]WЭ\HXЭ[™ЧЭ\NВ€WЩ[ќ]WЭ\H\™Щ]Э\NВ€XЭ[Ы—ЩЬ›Э\ЫX\ЪЧЭXЭ[™ЧЩXЭ[ЫЋВ€XЭ[Ы—ЩЬ›Э\ЫX\ЪЧЭXЭ[™ЧЩXЭ[Ы—Щљ[\™YВ€XЭ[Ы—ЩЬ›Э\ЫX\ЪЧЭ\™Щ]ЩXЭ[ЫЋВ‚€Y€
XXЭ[™ЧЩ[ќ]H]\™Щ]Щ[ќ]JHВ€™]\›€[ЩNВ€B‚€К‚€
€ЩHЪ[\ЩHY™™\™[ќXЭ[™ИXЭ[Ы€›Ь\ќB€
€[™\HY€H[™\™XЭ›YИ\ИЩ]‚€
‹В‚€Y€
[™\™XЭ
HВ€XЭ[™ЧЩXЭ[Ы€HXЭ[™ЧЩ[ќ]KO™XЭ[Ы‹™[XYЩWЪ[™\™XЭВ€XЭ[™ЧЭ\HHXЭ[™ЧЩ[ќ]KO™XЭ[Ы‹ќ\WЩ[XYЩWЪ[™\™XЭВ€€H[ЩHВ€XЭ[™ЧЩXЭ[Ы€HXЭ[™ЧЩ[ќ]KO™XЭ[Ы‹™[XYЩWЩ\™XЭВ€XЭ[™ЧЭ\HHXЭ[™ЧЩ[ќ]KO™XЭ[Ы‹ќ\WЩ[XYЩWЩ\™XЭВ€B‚€XЭ[™ЧЩXЭ[Ы—Щљ[\™YHXЭ[™ЧЩXЭ[ЫЋВ‚€К‚€
€™[[Э™HЬXЪX[\\Л‚€
‹В‚€XЭ[™ЧЩXЭ[Ы—Щљ[\™Y	ЏH‘ђPХSУ—СФ“ХTУ“ЧРТPТОВ€XЭ[™ЧЭ\H	ЏH•TWУ“ЧРТPТОВ‚€К‚€
€ЪXЪИ^Y\€[ќ\XЭ[Ы‹‚€
‹В‚€Y€
XЭ[Ы—ШЪXЪЧЬ^Y\—Э™\њЩ\КXЭ[™ЧЩ[ќ]K\™Щ]Щ[ќ]KXЭ[™ЧЩXЭ[ЫЉJHВ€™]\›€[ЩNВ€B‚€\™Щ]Э\HH\™Щ]Щ[ќ]KO›[Щ[]Kќ\NВ‚€К‚€
€Y€Ы™HЩ€HXЭ[™ИXЭ[ЫњИ\ИB€
€\H^Ы\Ъ]™HЬ›Э\[€ЩHЫ›B€
€ЪXЪИXЭ[™И\\ИњЛ€\™Щ]\B€
€[™YЫ›Ь™HЭ\€XЭ[ЫњЛ‚€
‹В‚€Y€
XЭ[™ЧЩXЭ[Ы€	€ђPХSУ—СФ“ХTХTWСVУTТU‘JHВ€€Y€
XЭ[™ЧЭ\H	€\™Щ]Э\JHВ€™]\›€ќYNВ€H[ЩHВ€™]\›€[ЩNВ€B€B‚€К‚€
€›ЭИЩHЫЫ\\™HЭ\€XЭ[™ИXЭ[ЫЉКHИB€
€XЭ[ЫЉКH\™Щ]\ИHY[X™\€Щ€›Ь€[ћHX]Ъ‚€
‹В‚€\™Щ]ЩXЭ[Ы€H\™Щ]Щ[ќ]KO™XЭ[Ы‹›Y[X™\ЋВ‚€Y€
XЭ[™ЧЩXЭ[Ы—Щљ[\™Y	€\™Щ]ЩXЭ[ЫЉHВ€К‚€
€Y€Ы™HЩ€HXЭ[™ИXЭ[ЫњИ\В€
€HYH[Ы\Ъ[™ИЬ›Э\[€ЩB€
€[ЫИЪXЪИXЭ[™И\\ИњИB€
€\™Щ]	ЬИ\K‚€
‹В‚€Y€
XЭ[™ЧЩXЭ[Ы€	€ђPХSУ—СФ“ХTХTWТSђУTТU‘JHВ€€Y€
XЭ[™ЧЭ\H	€\™Щ]Э\JHВ€™]\›€ќYNВ€€H[ЩHВ€™]\›€[ЩNВ€B€B‚€™]\›€ќYNВ€B‚€™]\›€[ЩNВџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊЊЛL‹LЌ‚Љ€Љ€™]\›€ќYHY€XЭ[™И[ќ]H\ВЉ€ЬЭ[HЭШ\™\™Щ][ќ]K‚Љ‹Вљ[ќXЭ[Ы—ШЪXЪЧЪ\ЧЪЬЭ[J[ќ]J€XЭ[™ЧЩ[ќ]K[ќ]J€\™Щ]Щ[ќ]JBћВ€ЛЬљ[ќЉ——€XЭ[Ы—ШЪXЪЧЪ\ЧЪЬЭ[J	\	\
H‹XЭ[™ЧЩ[ќ]K\™Щ]Щ[ќ]JNВ‚€WЩ[ќ]WЭ\HXЭ[™ЧЭ\NВ€WЩ[ќ]WЭ\H\™Щ]Э\NВ€XЭ[Ы—ЩЬ›Э\ЫX\ЪЧЭXЭ[™ЧЩXЭ[ЫЋВ€XЭ[Ы—ЩЬ›Э\ЫX\ЪЧЭљ[\™YЩXЭ[ЫЋВ€XЭ[Ы—ЩЬ›Э\ЫX\ЪЧЭ\™Щ]ЩXЭ[ЫЋВ‚€Y€
XXЭ[™ЧЩ[ќ]H]\™Щ]Щ[ќ]JB€В€™]\›€В€B‚€ЛЬљ[ќЉ——XЭ[™ЧЩ[ќ]KO›[YN€	\Л\™Щ]Щ[ќ]KO›[YN€	\И‹XЭ[™ЧЩ[ќ]KO›[YK\™Щ]Щ[ќ]KO›[YJNВ‚€XЭ[™ЧЩXЭ[Ы€HXЭ[™ЧЩ[ќ]KO™XЭ[Ы‹љЬЭ[NВ€љ[\™YЩXЭ[Ы€HXЭ[™ЧЩXЭ[ЫЋВ€XЭ[™ЧЭ\HHXЭ[™ЧЩ[ќ]KO™XЭ[Ы‹ќ\WЪЬЭ[NВ€\™Щ]Э\HH\™Щ]Щ[ќ]KO›[Щ[]Kќ\NВ‚€ЛЬљ[ќЉ——XЭ[™ЧЩXЭ[ЫЋ€	Y‹XЭ[™ЧЩXЭ[ЫЉNВ€ЛЬљ[ќЉ——XЭ[™ЧЭ\N€	Y‹XЭ[™ЧЭ\JNВ€ЛЬљ[ќЉ——\™Щ]Э\N€	Y‹\™Щ]Э\JNВ‚€К‚€
€™[[Э™HЬXЪX[\\Л‚€
‹В‚€љ[\™YЩXЭ[Ы€	ЏH‘ђPХSУ—СФ“ХTУ“ЧРТPТОВ€XЭ[™ЧЭ\H	ЏH•TWУ“ЧРТPТОВ‚€ЛЬљ[ќЉ——XЭ[™ЧЩXЭ[ЫЋ€	Y‹XЭ[™ЧЩXЭ[ЫЉNВ€ЛЬљ[ќЉ——XЭ[™ЧЭ\N€	Y‹XЭ[™ЧЭ\JNВ‚€К‚€
€ЪXЪИ^Y\€[ќ\XЭ[Ы‹€€
‹В‚€Y€
XЭ[Ы—ШЪXЪЧЬ^Y\—Э™\њЩ\КXЭ[™ЧЩ[ќ]K\™Щ]Щ[ќ]KXЭ[™ЧЩXЭ[ЫЉJB€В€ЛЬљ[ќЉ——^Y\€њЛ€™]\›€ЉNВ‚€™]\›€В€B‚€К‚€
€Y€Ы™HЩ€HXЭ[™ИXЭ[ЫњИ\ИB€
€\H^Ы\Ъ]™HЬ›Э\[€ЩHЫ›B€
€ЪXЪИXЭ[™И\\ИњЛ€\™Щ]\B€
€[™YЫ›Ь™HЭ\€XЭ[ЫњЛ‚€
‹В‚€Y€
XЭ[™ЧЩXЭ[Ы€	€ђPХSУ—СФ“ХTХTWСVУTТU‘JB€В€ЛЬљ[ќЉ——XЭ[™ЧЩXЭ[Ы€	€ђPХSУ—СФ“ХTХTWСVУTТU‘N€	Y‹XЭ[™ЧЩXЭ[Ы€	€ђPХSУ—СФ“ХTХTWСVУTТU‘JNВ‚€Y€
XЭ[™ЧЭ\H	€\™Щ]Э\JB€В€ЛЬљ[ќЉ——XЭ[™ЧЭ\H	€\™Щ]Э\N€	Y
Y\КH‹XЭ[™ЧЭ\H	€\™Щ]Э\JNВ‚€™]\›€NВ€B€[ЩB€В€љ[ќЉ——XЭ[™ЧЭ\H	€\™Щ]Э\N€	Y
›КH‹XЭ[™ЧЭ\H	€\™Щ]Э\JNВ‚€™]\›€В€B€B‚€К‚€
€›ЭИЩHЫЫ\\™HЭ\€XЭ[™ИXЭ[ЫЉКHИB€
€XЭ[ЫЉКH\™Щ]\ИHY[X™\€Щ€›Ь€[ћHX]Ъ‚€
‹В‚€\™Щ]ЩXЭ[Ы€H\™Щ]Щ[ќ]KO™XЭ[Ы‹›Y[X™\ЋВ‚€Y€
љ[\™YЩXЭ[Ы€	€\™Щ]ЩXЭ[ЫЉB€В€К‚€
€Y€Ы™HЩ€HXЭ[™ИXЭ[ЫњИ\В€
€HYH[Ы\Ъ[™ИЬ›Э\[€ЩB€
€[ЫИЪXЪИXЭ[™И\\ИњИB€
€\™Щ]	ЬИ\K‚€
‹В‚€Y€
XЭ[™ЧЩXЭ[Ы€	€ђPХSУ—СФ“ХTХTWТSђУTТU‘JB€В€ЛЬљ[ќЉ——XЭ[™ЧЩXЭ[Ы€	€ђPХSУ—СФ“ХTХTWТSђУTТU‘N€	Y‹XЭ[™ЧЩXЭ[Ы€	€ђPХSУ—СФ“ХTХTWТSђУTТU‘JNВ‚€Y€
XЭ[™ЧЭ\H	€\™Щ]Э\JB€В€ЛЬљ[ќЉ——XЭ[™ЧЭ\H	€\™Щ]Э\N€	Y
Y\КH‹XЭ[™ЧЭ\H	€\™Щ]Э\JNВ‚€™]\›€NВ€B€[ЩB€В€ЛЬљ[ќЉ——XЭ[™ЧЭ\H	€\™Щ]Э\N€	Y
›КH‹XЭ[™ЧЭ\H	€\™Щ]Э\JNВ‚€™]\›€В€B€B‚€™]\›€NВ€B‚€ЛЬљ[ќЉ——›ИX]Ъ€ЉNВ‚€™]\›€ВџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊЊЛL‹LЋЉ€Љ€™]\њИHY€^Y\њИ\™H›Э[ЭЩYЉ€њЛ€[ќ\XЭ[Ы€Ы€HЪ]™[€XЭ[Ы€Љ€›Ь\ќK€^€ЬЭ[K\™XЭ[XYЩKЉ€[™\™XЭ[XYЩK‚Љ‹Вљ[ќXЭ[Ы—ШЪXЪЧЬ^Y\—Э™\њЩ\К[ќ]J€XЭ[™ЧЩ[ќ]K[ќ]J€\™Щ]Щ[ќ]KXЭ[Ы—ЩЬ›Э\ЫX\ЪЧЭXЭ[Ы—Ь›Ь\ќJBћВ€Y€
XXЭ[™ЧЩ[ќ]H]\™Щ]Щ[ќ]JB€В€™]\›€В€B‚€К‚€
€›Э[ќ]\И]\Э™H^Y\њЛ‚€
‹В‚€Y€
JXЭ[™ЧЩ[ќ]KO›[Щ[]Kќ\H	€TWФVQTЉJB€В€™]\›€В€B‚€Y€
J\™Щ]Щ[ќ]KO›[Щ[]Kќ\H	€TWФVQTЉJB€В€™]\›€В€B‚€К‚€
€[€XЭ[Ы€Ь›Э\ИЭ™\њљYHњљY[™B€
€љ\™HЩ][™ЬПВ€
‹В‚€Y€
XЭ[Ы—Ь›Ь\ќH	€ђPХSУ—СФ“ХTФVQT—Х‘T”СTКB€В€™]\›€В€B‚€К‚€
€Y€”Л€[ЩH\›™YЩ™€[€Ь[ЫњВ€
€Ь€]™[›Ъ]\И[X›Y[‚€
€ЩH\™H™њљY[™H€ЭШ\™\™Щ]‚€
‹В‚€Y€
Ш]™Y]K›[ЩJB€В€™]\›€NИ€B‚€Y€
]™[	‰€]™[O››Ъ]OHSPQСWС”“УWФVQT—УС‘ЉB€В€™]\›€NВ€B‚€™]\›€ВџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊNKLL‹LN
™YXЭЬЉBЉ‚Љ€ЬљYЪ[[]]Ь€[љЫ›ЭЫ€
Z[ПКK€™YXЭЬ™YИ™[[Э™HH]™\‹YЬ›ЭЪ[™И\[Y]\€\ЭЉ€[™ЫЫњЫЫY]H›Ъ™XЭ[HЬ]Ы€ЩЪXЛ€Ь]ЫњИ[€[ќ]H[™љ\™\И]\ИH›Ъ™XЭ[K€Љ€[Щ[\ЩY›Ь€Ь]Ы€\И]\›Z[™YћHHY\\ЪHЩ€YШXЮH\[Y]\њИ
ЩYH]Z[YЉ€ЫЫ[Y[ќИ[€ќ[Э[ЫЉK‚Љ‚Љ€™]\›њИЪ[ќ\€Щ€Ь]Ы™Y›Ъ™XЭ[KЬ€•SЫ€Z[‚Љ‹В™[ќ]H
љЫљY™WЬЬ]ЫЉ[ќ]H
њ\™[ќЧЬ›Ъ™XЭ[H
њ›Ъ™XЭ[JBћВ€[ќ]H
њ›Ъ™XЭ[WЩ[ќ]HH•SВ‚‚\ЧШ^\ЧЬљ[Ъ\[Щ›Ш]ЬЪ][ЫЋВ‚YWЩ\™XЭ[Ы€\™XЭ[Ы€HT‘PХSУ—Ф’QТВ‚YWЬ›Ъ™XЭ[WЬљ[YH›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWУ“У‘NВ‚B‚KК€Y€\™IЬИ›И›Ъ™XЭ[HЬ€\™[ќЩ][™Л^]›ЭЛ€
‹В‚ZY€
\›Ъ™XЭ[H\\™[ќ
B‚^В‚B\™]\›€•SВ‚_B‚€ЬЪ][Ы‹ћH\™[ќOњЬЪ][Ы‹ћВ‚\ЬЪ][Ы‹ћHH\™[ќOњЬЪ][Ы‹ћH
И›Ъ™XЭ[KOњЬЪ][Ы‹ћNВ‚\ЬЪ][Ы‹ћ€H\™[ќOњЬЪ][Ы‹ћ€
И›Ъ™XЭ[KOњЬЪ][Ы‹ћЋВ‚€К‚‚J€›ЭИЩH™YYИЬ]Ы€H›Ъ™XЭ[H[ќ]K€\™H\™HX[ћH\^\™YШXЮH‚J€Y][ЫњИИЪYќ›ЭYЪЫИЩH™YYИљ[Ьљ]^™HЪXЪ[Щ[ИЬ]Ы‹€‚J€[€Щ[™\[ЩHЫЬљИXЪИњ›ЫH[ЬЭЬ[ќ[\€И[ЬЭЫШ[‚‚J‚‚J€њ›ЫHYЪ\ЭИЭЩ\Эљ[Ьљ]N‚‚J€‚J€K€›Ъ™XЭ[HЫљY™H›Ь\ќK‚‚J€‹€›Ъ™XЭ[H›\Ъ›Ь\ќK‚‚J€Л€\Ъ[™ИЩX\Ы€Ъ][Щ[›Ъ™XЭ›Ь\ќK‚‚J€€[Щ[ЫљY™H›Ь\ќK‚‚J€K€[Щ[ЪЭ›И›Ь\ќK‚‚J€‹€ЫШ[\™ЫЩH[Щ[[YK’ЫљY™H‹‚‚J€Л€ЫШ[\™ЫЩH[Щ[[YK”ЪЭ‹‚‚J‹В€Y€
›Ъ™XЭ[KOљЫљY™HЏH
B‚^В€›Ъ™XЭ[WЩ[ќ]HHЬ]ЫЉЬЪ][Ы‹ћЬЪ][Ы‹ћ‹ЬЪ][Ы‹ћK\™XЭ[Ы‹•S›Ъ™XЭ[KOљЫљY™K•S
NВ‚BB‚B\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWРђTСWЦNВ‚B\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWУUSђТУSХ’S‘ОВ‚B\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWФУХTђСWФ“Т—ТУ’Q‘NВ‚_B‚Y[ЩHY€
›Ъ™XЭ[KO™›\ЪЏH
B‚^В€›Ъ™XЭ[WЩ[ќ]HHЬ]ЫЉЬЪ][Ы‹ћЬЪ][Ы‹ћ‹ЬЪ][Ы‹ћK\™XЭ[Ы‹•S›Ъ™XЭ[KO™›\Ъ•S
NВ‚BB‚B\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWРђTСWС“УФЋВ‚B\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWУUSђТФХUSУђT–NВ‚B\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWФУХTђСWФ“Т—С“TТВ‚_B‚Y[ЩHY€
\™[ќOќЩX\[ќ	‰€\™[ќOќЩX\[ќO›[Щ[]Kњ›Ъ™XЭЏH
B‚^В€›Ъ™XЭ[WЩ[ќ]HHЬ]ЫЉЬЪ][Ы‹ћЬЪ][Ы‹ћ‹ЬЪ][Ы‹ћK\™XЭ[Ы‹•S\™[ќOќЩX\[ќO›[Щ[]Kњ›Ъ™XЭ•S
NВ‚BB‚B\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWРђTСWЦNВ‚B\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWУUSђТУSХ’S‘ОВ‚B\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWФУХTђСWХСPTУ—Ф“Т‘PХSNВ‚_B‚Y[ЩHY€
\™[ќO›[Щ[]KљЫљY™HЏH
B‚^В€›Ъ™XЭ[WЩ[ќ]HHЬ]ЫЉЬЪ][Ы‹ћЬЪ][Ы‹ћ‹ЬЪ][Ы‹ћK\™XЭ[Ы‹•S\™[ќO›[Щ[]KљЫљY™K•S
NВ‚BB‚B\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWРђTСWЦNВ‚B\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWУUSђТУSХ’S‘ОВ‚B\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWФУХTђСWУSСSТУ’Q‘NВ‚_B‚Y[ЩHY€
\™[ќO›[Щ[]KњЪЭ›ИЏH
B‚^В€›Ъ™XЭ[WЩ[ќ]HHЬ]ЫЉЬЪ][Ы‹ћЬЪ][Ы‹ћ‹ЬЪ][Ы‹ћK\™XЭ[Ы‹•S\™[ќO›[Щ[]KњЪЭ›Л•S
NВ‚BB‚B\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWРђTСWС“УФЋВ‚B\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWУUSђТФХUSУђT–NВ‚B\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWФУХTђСWУSСSФТХ“ОВ‚_B‚Y[ЩB‚^В€К‚‚BJ€›И[Щ[[™^\ИЩ]ЫИ]	ЬИ[XЪИВ‚BJ€HYШXЮH\™ЫЩH[Щ[[Y\Л‚€
‚‚BJ€ћH\™ЫЩHљЫљY™H€љ\њЭ€Y€]Z[ЛЩIЫћB‚BJ€њЪЭ€™^‚€
‹В€›Ъ™XЭ[WЩ[ќ]HHЬ]ЫЉЬЪ][Ы‹ћЬЪ][Ы‹ћ‹ЬЪ][Ы‹ћK\™XЭ[Ы‹’ЫљY™H‹SСSТS‘VУ“У‘K•S
NВ‚BB‚BZY€
›Ъ™XЭ[WЩ[ќ]JB‚B^В€К‚‚BBJ€\™ЫЩHЫљY™HЬ]Ы€ЭXШЩ\ЬЩќ[€X\љИ\ИYШXЮHЫљY™B‚BBJ€[™ЫЫќ[ќYK‚€
‹И‚BB\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWФУХTђСWСУРђSТУ’Q‘NВ‚B_B‚BY[ЩH‚B^В‚BBKК€ћHњЪЭ‹€
‹В€›Ъ™XЭ[WЩ[ќ]HHЬ]ЫЉЬЪ][Ы‹ћЬЪ][Ы‹ћ‹ЬЪ][Ы‹ћK\™XЭ[Ы‹”ЪЭ‹SСSТS‘VУ“У‘K•S
NВ‚‚BB\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWФУХTђСWСУРђSФТХВ‚B_B‚‚B\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWРђTСWЦNВ‚B\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWУUSђТУSХ’S‘ОВ‚_B‚‚KК€Y€ЩH™]™\€ЭXШЩ\ЬЩќ[HЬ]Ы™YH›Ъ™XЭ[H[ќ]K^]€
‹В€YЉ\›Ъ™XЭ[WЩ[ќ]JB€В€™]\›€•SВ€B‚€К‚€
€Щ]™\Э[Щ€\™XЭ[Ы€Yќ\ЭY[ќ€ЩH™YY\И™Y›Ь™HЩHШ[€[™B€
€ЬЪ][Ыљ[™ИЫ€^\Л‚€
‹В€\™XЭ[Ы€H\™XЭ[Ы—ЩЩ]ШYќ\ЭY[ќЬ™\Э[
›Ъ™XЭ[WЩ[ќ]K\™[ќ›Ъ™XЭ[KO™\™XЭ[Ы—ШYќ\Э
NВ‚€К‚€
€]	ЬИЩ]\HЬ]Ы€ЬЪ][Ы‹€™]™\њЩHЪ[‚€
€\™[ќXЩ\ИYќ‚€
‚€
€\HY][ЬЪ][Ы€Y€Ь™X]Ь€Y›ЭЪ]™B€
€\ИH[YK‚€
‹В‚€Y€
›Ъ™XЭ[KOњЬЪ][Ы‹ћOH“Т‘PХSWУQРPЦWРУУTUP’SUWФФТUSУ—Ц
B€В€›Ъ™XЭ[KOњЬЪ][Ы‹ћH“Т‘PХSWСQђUSФФТUSУ—ЦВ€B‚€Y€
\™XЭ[Ы€OHT‘PХSУ—Ф’QТ
B€В€ЬЪ][Ы‹ћH\™[ќOњЬЪ][Ы‹ћ
И›Ъ™XЭ[KOњЬЪ][Ы‹ћВ€B€[ЩB€В€ЬЪ][Ы‹ћH\™[ќOњЬЪ][Ы‹ћH›Ъ™XЭ[KOњЬЪ][Ы‹ћВ€B‚€›Ъ™XЭ[WЩ[ќ]KO™\™XЭ[Ы€H\™XЭ[ЫЋВ€›Ъ™XЭ[WЩ[ќ]KOњЬЪ][Ы‹ћHЬЪ][Ы‹ћВ€€К€€
€^Y\€›Ъ™XЭ[\И\™H[Ш^\И\HњЪЭ‹[›\ЬИ€
€\Ъ[™ИHЭ\њ™[ќ“Т‘PХSH\K‚€
‹В€Y€
J›Ъ™XЭ[WЩ[ќ]KO›[Щ[]Kќ\H	€TWФ“Т‘PХSJJB€В€Y€
\™[ќO›[Щ[]Kќ\H	€TWФVQTЉB€В€›Ъ™XЭ[WЩ[ќ]KO›[Щ[]Kќ\HHTWФТХВ€B€[ЩB€В€›Ъ™XЭ[WЩ[ќ]KO›[Щ[]Kќ\HH\™[ќO›[Щ[]Kќ\NВ€B€B‚‚KК€\H›Ъ™XЭ[Hљ[YH›YЬЛ€
‹В€›Ъ™XЭ[WЩ[ќ]KOњ›Ъ™XЭ[WЬљ[YHH›Ъ™XЭ[WЬљ[YNВ‚€К‚‚J€ЫЬHЩ™™[њЩH[Y\Ињ›ЫH\™[ќЩ™™[њЩHЩ][™ЬИ‚J€И›Ъ™XЭ[H[љ]HY€™\]Y\ЭY‚‚J‹В€Y€
›Ъ™XЭ[KO›Щ™™[њЩHOH“Т‘PХSWУС‘‘S”СWФT‘S•
B‚^В‚B[Y[XЬJ›Ъ™XЭ[WЩ[ќ]KO›Щ™™[њЩK\™[ќO›Щ™™[њЩKЪ^™[ЩЉ
њ›Ъ™XЭ[WЩ[ќ]KO›Щ™™[њЩJH
€X^Ш]XЪЧЭ\\КNВ‚_B‚‚KК€\HЫЫЬ€Yќ\ЭY[ќ€
‹В‚X\WШЫЫЬ—ЬЩ]ШYќ\Э
›Ъ™XЭ[WЩ[ќ]K\™[ќ›Ъ™XЭ[KOЫЫЬ—ЬЩ]ШYќ\Э
NВ‚BB‚€К‚‚J€Y€›И[Э™K[€[ЬYY\И€Э\ќЪ\ЩHЪXЪИ›Ь€\ЩHЩ‚‚J€›Ъ™XЭ[H™[ШЪ]K€Y€^Y\€Э\YY[ћH[YHЭ\€‚J€[€SСSФФQQУ“У‘KЩH\ЩH^Y\‰ЬИ[YK€Y€›Э[‚J€XЪИИY][[Y\Л€\И\ИHљ]Э™\ЫЫ\XШ]Yќ]‚J€[ЭЬИ^Y\њИИЭ\HH™[ШЪ]H[YHЫ€[ћH^\Л‚‚J‹В€Y€
›Ъ™XЭ[WЩ[ќ]KO›[Щ[]K›[Э™WШЫЫ™љYЧЩ›YЬИ	€SХ‘WРУУ‘’QЧУ“ЧУSХ‘JB‚^В€›Ъ™XЭ[WЩ[ќ]KO›[Щ[]KњЬYYћHВ€›Ъ™XЭ[WЩ[ќ]KO›[Щ[]KњЬYYћHHВ€›Ъ™XЭ[WЩ[ќ]KO›[Щ[]KњЬYYћ€HВ‚_B‚Y[ЩB‚^ВB‚BKК€ЫЬHЬYY[Y\Ињ›ЫH[љ[X][Ы€›Ъ™XЭ[HЩ][™ЬИИ[Щ[€
‹В€›Ъ™XЭ[KOќ™[ШЪ]HH›Ъ™XЭ[WЩ[ќ]KO›[Щ[]KњЬYYВ‚_B‚‚KК€Щ]\™Z]љ[Ь€›YЬЛ€
‹В€›Ъ™XЭ[WЩ[ќ]KOњЬ]Ыќ\HHФUУ—ХTWФ“Т‘PХSWУ“Ф“PSВ€›Ъ™XЭ[WЩ[ќ]KO›ЭЫ™\€H\™[ќВ€›Ъ™XЭ[WЩ[ќ]KO››ЩЬX€HNВ€›Ъ™XЭ[WЩ[ќ]KO]XЪЪ[™ИHUPТТS‘ЧРPХU‘NВ€›Ъ™XЭ[WЩ[ќ]KOќ[љИHЫЫ[[Ы—Э[љОВ€›Ъ™XЭ[WЩ[ќ]KO›™^[љИHЭ[YH
ИNВ€›Ъ™XЭ[WЩ[ќ]KOќћ[[Э™HH•SВ€›Ъ™XЭ[WЩ[ќ]KOќZЩY[XYЩHH\њ›ЭЧЭZЩY[XYЩNВ€›Ъ™XЭ[WЩ[ќ]KOќZЩXXЭ[Ы€H•SВ€›Ъ™XЭ[WЩ[ќ]KO›[Щ[]KZ[[Э™HHRSSХ‘LWРT”“ХОВ€›Ъ™XЭ[WЩ[ќ]KOњЬYY][HЋВ€›Ъ™XЭ[WЩ[ќ]KO›[Щ[]KZX]XЪИHRPUPТМWУ“РUPТОВ€€YЉ\›Ъ™XЭ[WЩ[ќ]KO›[Щ[]K›Щ™њШЬ™Y[љЪ[
B€В€›Ъ™XЭ[WЩ[ќ]KO›[Щ[]K›Щ™њШЬ™Y[љЪ[HЊИЛЩY][[YB€BB€‚KК€Ъ[Щ[€Ъ[€ЩH]€
‹В‚ZY€
›Ъ™XЭ[WЩ[ќ]KO›[Щ[]Kњ™[[Э™WШЫЫ™љYИ	€‘SSХ‘WРУУ‘’QЧТU
B‚^В€›Ъ™XЭ[WЩ[ќ]KO]]ЪЪ[HUUТТSРUPТЧТUВ‚_B‚B€К€Ъ[Щ[€Ъ[€ЩHљ[љ\Ъ[љ[X][Ы‹€
‹В‚ZY€
›Ъ™XЭ[WЩ[ќ]KO›[Щ[]K›[Э™WШЫЫ™љYЧЩ›YЬИ	€SХ‘WРУУ‘’QЧУ“ЧУSХ‘JB‚^В€›Ъ™XЭ[WЩ[ќ]KO]]ЪЪ[HUUТТSРS’SPUSУ—РУУTUNВ‚_B‚B‚KК€\И\ИH›ЫЬ€Ь€›Z[™И›Ъ™XЭ[HИЩ]\ЩHXШЫЬ™[™ЫK€
‹В€YЉ›Ъ™XЭ[WЩ[ќ]KOњ›Ъ™XЭ[WЬљ[YH	€“Т‘PХSWФ’SQWРђTСWС“УФЉB€В€›Ъ™XЭ[WЩ[ќ]KO\ЩHHВ€B€[ЩB€В€›Ъ™XЭ[WЩ[ќ]KO\ЩHHЬЪ][Ы‹ћNВ€B‚€К‚‚J€ЫЬHXЭ[Ы€]Hњ›ЫH\™[ќ‚‚J‹В‚€XЭ[Ы—ШЫЬWШ[
›Ъ™XЭ[WЩ[ќ]K\™[ќ
NВ‚€К‚‚J€Y€^Y\€[XYЩH\›™YЩ™‹™[[Э™H^Y\€\Hњ›ЫB‚J€ЬЭ[H
ЫИЫZ[™И›Ъ™XЭ[\ИX]™H^Y\њИ[Ы™JH[™‚J€њ›ЫHШ[™[XYЩK‚€
‹В€YЉ
\™[ќO›[Щ[]Kќ\H	€TWФVQTЉH	‰€

]™[	‰€]™[O››Ъ]OHSPQСWС”“УWФVQT—УС‘ЉHШ]™Y]K›[ЩJJB€В€›Ъ™XЭ[WЩ[ќ]KO™XЭ[Ы‹ќ\WЪЬЭ[H	ЏH•TWФVQTЋВ€›Ъ™XЭ[WЩ[ќ]KO™XЭ[Ы‹ќ\WЩ[XYЩWЩ\™XЭ	ЏH•TWФVQTЋВ€B‚‚KК‚€
€Щ]\њZ[€[™[Э™[Y[ќ™Z]љ[Ь€›YЬЛ€YШXЮH€
€™Z]љ[Ь€ЫЭ[‰Э[ЭИ[Э™[Y[ќ[Ы™ИH^\В€
€Ъ]Э][€\Лќ]ЩHШ[‰Эљ^\ИЪ]Э]€
€њ™XZЪ[™ИЫЫ\]Xљ[]KЫИЩIЫ™YYHY™™\™[ќ€
€Щ]\\[™[™ИЫ€Ъ]ќ[Э[Ы€Ь™X]Ь€\ЩY‚€
‹В€€›Ъ™XЭ[WЩ[ќ]KO›[Щ[]K›[Э™WШЫЫ™љYЧЩ›YЬИH
SХ‘WРУУ‘’QЧФ“Т‘PХSWРђTСWСQHSХ‘WРУУ‘’QЧФХP’‘PХХЧТУHSХ‘WРУУ‘’QЧФХP’‘PХХЧФU“Ф“HSХ‘WРУУ‘’QЧФХP’‘PХХЧХРSSХ‘WРУУ‘’QЧФ“Т‘PХSWХРSР“ХSђСJNВ€›Ъ™XЭ[WЩ[ќ]KO›[Щ[]K›[Э™WШЫЫ™љYЧЩ›YЬИ	ЏH“SХ‘WРУУ‘’QЧУ“ЧРQ•TХРђTСNВ‚€Y€
›Ъ™XЭ[WЩ[ќ]KOњ›Ъ™XЭ[WЬљ[YH	€“Т‘PХSWФ’SQWТS’UPSV‘WУQРPЦWФ“Т‘PХSWС•SђХSУЉB€В€›Ъ™XЭ[WЩ[ќ]KO›[Щ[]K›[Э™WШЫЫ™љYЧЩ›YЬИHSХ‘WРУУ‘’QЧФХP’‘PХХЧСФђU’UNВ€B€[ЩB€В€›Ъ™XЭ[WЩ[ќ]KO›[Щ[]K›[Э™WШЫЫ™љYЧЩ›YЬИ	ЏH“SХ‘WРУУ‘’QЧФХP’‘PХХЧСФђU’UNВ€B€‚KК€^XЭ]HH›Ъ™XЭ[IЬИЫ€Ь]Ы€]™[ќ€
‹В‚Y^XЭ]WЫЫњЬ]Ы—ЬШЬљ\
›Ъ™XЭ[WЩ[ќ]JNВ‚B‚\™]\›€›Ъ™XЭ[WЩ[ќ]NВџB‚ќ›ЪY›ЫX—Щ^ЩJ
BћВ€YЉЩ[‹O[љ[X][™КB€В€™]\›ЋВ€B€Ъ[Щ[ќ]JЩ[‹ТSСS•UWХ’QССT—Р“УP—СVСWРS’SPUSУ—РУУTUJNВџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊNKLL‹LЊ€
™YXЭЬЉBЉ‚Љ€ЬљYЪ[[]]Ь€[љЫ›ЭЫ€
Z[ПКK€™YXЭЬ™YИ™[[Э™HH]™\‹YЬ›ЭЪ[™И\[Y]\€\ЭЉ€[™ЫЫњЫЫY]H›Ъ™XЭ[HЬ]Ы€ЩЪXЛ€Ь]ЫњИ[€[ќ]H[™љ\™\И]\ИH›ЫX€›Ъ™XЭ[K€Љ€[Щ[\ЩY›Ь€Ь]Ы€\И]\›Z[™YћHHY\\ЪHЩ€YШXЮH\[Y]\њИ
ЩYH]Z[YЉ€ЫЫ[Y[ќИ[€ќ[Э[ЫЉK‚Љ‚Љ€™]\›њИЪ[ќ\€Щ€Ь]Ы™Y›Ъ™XЭ[KЬ€•SЫ€Z[‚Љ‹В™[ќ]H
›ЫX—ЬЬ]ЫЉ[ќ]H
њ\™[ќЧЬ›Ъ™XЭ[H
њ›Ъ™XЭ[JBћВ‚Y[ќ]J€[ќH•SВ‚\ЧШ^\ЧЬљ[Ъ\[Щ›Ш]ЬЪ][ЫЋВ‚YWЩ\™XЭ[Ы€\™XЭ[Ы€HT‘PХSУ—Ф’QТВ‚YWЬ›Ъ™XЭ[WЬљ[YH›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWУ“У‘NВ‚‚KЛИY€\™IЬИ›И›Ъ™XЭ[HЬ€\™[ќЩ][™Л^]›ЭЛ‚‚ZY€
\›Ъ™XЭ[H\\™[ќ
B‚^В‚B\™]\›€•SВ‚_B€B€ЬЪ][Ы‹ћH\™[ќOњЬЪ][Ы‹ћВ‚\ЬЪ][Ы‹ћHH\™[ќOњЬЪ][Ы‹ћH
И›Ъ™XЭ[KOњЬЪ][Ы‹ћNВ‚\ЬЪ][Ы‹ћ€H\™[ќOњЬЪ][Ы‹ћ€
И›Ъ™XЭ[KOњЬЪ][Ы‹ћЋВ‚€К‚‚J€›ЭИЩH™YYИЬ]Ы€H›Ъ™XЭ[H[ќ]K€\™H\™HX[ћH\^\™YШXЮH‚J€Y][ЫњИИЪYќ›ЭYЪЫИЩH™YYИљ[Ьљ]^™HЪXЪ[Щ[ИЬ]Ы‹€‚J€[€Щ[™\[ЩHЫЬљИXЪИњ›ЫH[ЬЭЬ[ќ[\€И[ЬЭЫШ[‚‚J‚‚J€њ›ЫHYЪ\ЭИЭЩ\Эљ[Ьљ]N‚‚J€‚J€K€›Ъ™XЭ[H›ЫX€›Ь\ќK‚‚J€‹€\Ъ[™ИЩX\Ы€Ъ][Щ[›ЫX€›Ь\ќK‚‚J€Л€[Щ[›ЫX€›Ь\ќK‚‚J‹В‚€YЉ›Ъ™XЭ[KO›ЫX€ЏH
B€В€[ќHЬ]ЫЉЬЪ][Ы‹ћЬЪ][Ы‹ћ‹ЬЪ][Ы‹ћK\™XЭ[Ы‹•S›Ъ™XЭ[KO›ЫX‹•S
NВ‚‚B\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWФУХTђСWФ“Т—Р“УPЋВ‚_B‚Y[ЩHY€
Щ[‹OќЩX\[ќ	‰€Щ[‹OќЩX\[ќO›[Щ[]KњЭXќ\HOHХP•TWФ“Т‘PХSH	‰€Щ[‹OќЩX\[ќO›[Щ[]Kњ›Ъ™XЭЏH
B‚^В‚BY[ќHЬ]ЫЉЬЪ][Ы‹ћЬЪ][Ы‹ћ‹ЬЪ][Ы‹ћK\™XЭ[Ы‹•SЩ[‹OќЩX\[ќO›[Щ[]Kњ›Ъ™XЭ•S
NВ‚BB‚B\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWФУХTђСWХСPTУ—Ф“Т‘PХSNВ‚_B€[ЩHYЉЩ[‹O›[Щ[]K›ЫX€ЏH
B€В€[ќHЬ]ЫЉЬЪ][Ы‹ћЬЪ][Ы‹ћ‹ЬЪ][Ы‹ћK\™XЭ[Ы‹•SЩ[‹O›[Щ[]K›ЫX‹•S
NВ‚‚B\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWФУХTђСWУSСSР“УPЋВ‚_B‚B‚\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWРђTСWС“УФЋВ‚\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWУUSђТУSХ’S‘ОВ‚B‚KЛИY€ЩH™]™\€ЭXШЩ\ЬЩќ[HЬ]Ы™YH›Ъ™XЭ[H[ќ]K^]‚€YЉY[ќ
B€В€™]\›€•SВ€B‚€К‚€
€Щ]™\Э[Щ€\™XЭ[Ы€Yќ\ЭY[ќ€ЩH™YY\И™Y›Ь™HЩHШ[€[™B€
€ЬЪ][Ыљ[™ИЫ€^\Л‚€
‹В€\™XЭ[Ы€H\™XЭ[Ы—ЩЩ]ШYќ\ЭY[ќЬ™\Э[
[ќ\™[ќ›Ъ™XЭ[KO™\™XЭ[Ы—ШYќ\Э
NВ‚€К‚€
€]	ЬИЩ]\HЬ]Ы€ЬЪ][Ы‹€™]™\њЩHЪ[‚€
€\™[ќXЩ\ИYќ‚€
‚€
€\HY][ЬЪ][Ы€Y€Ь™X]Ь€Y›ЭЪ]™B€
€\ИH[YK‚€
‹В‚€Y€
›Ъ™XЭ[KOњЬЪ][Ы‹ћOH“Т‘PХSWУQРPЦWРУУTUP’SUWФФТUSУ—Ц
B€В€›Ъ™XЭ[KOњЬЪ][Ы‹ћH“Т‘PХSWСQђUSФФТUSУ—ЦВ€B‚€Y€
\™XЭ[Ы€OHT‘PХSУ—Ф’QТ
B€В€ЬЪ][Ы‹ћH\™[ќOњЬЪ][Ы‹ћ
И›Ъ™XЭ[KOњЬЪ][Ы‹ћВ€B€[ЩB€В€ЬЪ][Ы‹ћH\™[ќOњЬЪ][Ы‹ћH›Ъ™XЭ[KOњЬЪ][Ы‹ћВ€B‚€[ќO™\™XЭ[Ы€H\™XЭ[ЫЋВ€[ќOњЬЪ][Ы‹ћHЬЪ][Ы‹ћВ‚‚‚Y[ќOњ›Ъ™XЭ[WЬљ[YHH›Ъ™XЭ[WЬљ[YNВ‚€К‚€
€Y€›И[Э™K[€[ЬYY\И€Э\ќЪ\ЩHЪXЪИ›Ь€\ЩHЩ‚‚J€›Ъ™XЭ[H™[ШЪ]K€Y€^Y\€Э\YY[ћH[YHЭ\€‚J€[€SСSФФQQУ“У‘KЩH\ЩH^Y\‰ЬИ[YK€Y€›Э[‚J€XЪИИY][[Y\Л€\И\ИHљ]Э™\ЫЫ\XШ]Yќ]‚J€[ЭЬИ^Y\њИИЭ\HH™[ШЪ]H[YHЫ€[ћH^\Л‚€
‹В‚‚ZY€
[ќO›[Щ[]K›[Э™WШЫЫ™љYЧЩ›YЬИ	€SХ‘WРУУ‘’QЧУ“ЧУSХ‘JB‚^В‚BY[ќO›[Щ[]KњЬYYћHВ‚BY[ќO›[Щ[]KњЬYYћHHВ‚BY[ќO›[Щ[]KњЬYYћ€HВ‚_B‚Y[ЩB‚^ВBB‚BKЛИЫЬHЬYY[Y\Ињ›ЫH[љ[X][Ы€›Ъ™XЭ[HЩ][™ЬИИ[Щ[‚‚B\›Ъ™XЭ[KOќ™[ШЪ]KћH[ќO›[Щ[]KњЬYYћВ‚_B‚€К‚‚J€ЬЬИH›ЫX€[ќ]H[€[€\Л‚‚J‚‚J€ЩHШ[ќИ[™HYШXЮH™Z]љ[Ь€
\ЩH›Ъ™XЭ[Hќ[\ZYЪ‚J€›Ь€HЬЬИ™[ШЪ]JK[ЭИ]]Ь€И\HH[YH›Ь€H‚J€™[ШЪ]K[™\ЩHHШ[YH™[ШЪ]HЭќXЭ\™HY[X™\њИ›Ь€›ЫX€‚J€[™ЫљY™H›Ъ™XЭ[\Л€H\Э\ќ\И[€^HЪ[[™ЩH™XШ]\ЩH‚J€HY][™[ШЪ]H™YYИ›Ь€ЫљY™H[™›ЫX€\™H›ЭЫЫ\]X›K€‚J€И[™H[Щ€\И\ИЩHЪ[Щ]H™[ШЪ]KћHY[X™\€И‚J€SСSФФQQУ“У‘HЬXЩљXШ[HЪ[€H›ЫX€›Ъ™XЭ[H\И™\]Y\ЭY‚J€ћHH›ЭЩњ[YHЫЫ[X[™Ь€›ЫX€€\Hњ›ЫHYШXЮHШЬљ\ќ[Э[Ы€‚J€›Ъ™XЭ[J
K€Y€H]]Ь€Щ\И›Э[ЩYћH\И[YKЩHЫ›ЭИ‚J€И[XЪИИYШXЮH™Z]љ[Ь€[™\ЩHH›Ъ™XЭ[H[ќ]IЬИ‚J€[Щ[ќ[\ZYЪ€‚J‚‚J€Y€H[YH\И[ћ][™ИЭ\€[€SСSФФQQУ“У‘K[€ЩHЫ›ЭИ‚J€H]]Ь€™\]Y\ЭYHЬXЪYљXИ™[ШЪ]H
[ЫY[™И
H[™Ъ[‚J€\ЩH]]Ь€[YH[њЭXYЩ€[Щ[ќ[\ZYЪ‚‚J‹В‚‚ZY€
›Ъ™XЭ[KOќ™[ШЪ]KћHOHSСSФФQQУ“У‘JB‚^В‚B]ЬЬК[ќ[ќO›[Щ[]Kљќ[\ZYЪ
NВ‚_B‚Y[ЩB‚^В‚B]ЬЬК[ќ›Ъ™XЭ[KOќ™[ШЪ]KћJNВ‚_B‚B‚KК€\HЫЫЬ€Yќ\ЭY[ќ€
‹В‚X\WШЫЫЬ—ЬЩ]ШYќ\Э
[ќ\™[ќ›Ъ™XЭ[KOЫЫЬ—ЬЩ]ШYќ\Э
NВ‚€[ќOњЬ]Ыќ\HHФUУ—ХTWФ“Т‘PХSWР“УPЋВ€[ќO]XЪЪ[™ИHUPТТS‘ЧРPХU‘NВ€[ќO›ЭЫ™\€H\™[ќИ€[ќO››ЩЬX€HNИ€[ќOќЩ^ЩHH
VСWФ‘TT‘WХХPТVСWФ‘TT‘WСФ“ХS‘
NИ€€[ќOќ[љИHЫЫ[[Ы—Э[љОВ€[ќO›™^[љИHЭ[YH
ИNВ€[ќOќћ[[Э™HH•SВ€[ќOќZЩXXЭ[Ы€H•SВ€[ќO›[Щ[]KZ[[Э™HHRSSХ‘LWР“УPЋВ€[ќO›[Щ[]KZX]XЪИHRPUPТМWУ“РUPТОВ€[ќOќZЩY[XYЩHHЫЫ[[Ы—ЭZЩY[XYЩNВ‚Y[ќO]]ЪЪ[	ЏHђUUТТSРUPТЧТUИ‚‚ZY€
[ќO›[Щ[]K›[Э™WШЫЫ™љYЧЩ›YЬИ	€SХ‘WРУУ‘’QЧУ“ЧУSХ‘JB‚^В‚BY[ќO]]ЪЪ[HUUТТSРS’SPUSУ—РУУTUNВ‚_B‚B€[ќOњЬYY][HЋВ‚‚KК‚€
€ЫЬHXЭ[Ы€]K‚€
‹В‚‚YXЭ[Ы—ШЫЬWШ[
[ќ\™[ќ
NВ€‚Y[ќO›[Щ[]K›[Э™WШЫЫ™љYЧЩ›YЬИ	ЏH“SХ‘WРУУ‘’QЧУ“ЧРQ•TХРђTСNВ‚Y[ќO›[Щ[]K›[Э™WШЫЫ™љYЧЩ›YЬИH
SХ‘WРУУ‘’QЧФХP’‘PХХЧРђTСSPTSХ‘WРУУ‘’QЧФ“Т‘PХSWРђTСWСQHSХ‘WРУУ‘’QЧФ“Т‘PХSWХРSР“ХSђСHSХ‘WРУУ‘’QЧФХP’‘PХХЧСФђU’UHSХ‘WРУУ‘’QЧФХP’‘PХХЧТУHSХ‘WРУУ‘’QЧФХP’‘PХХЧФU“Ф“HSХ‘WРУУ‘’QЧФХP’‘PХХЧХРS
NВ‚B‚KЛИ^XЭ]HH›Ъ™XЭ[IЬИЫ€Ь]Ы€]™[ќ‚‚Y^XЭ]WЫЫњЬ]Ы—ЬШЬљ\
[ќ
NВ‚B‚\™]\›€[ќВџB‚‹ЛИЬ]Ы€ИЭ\њВ‹ЛВ‹ЛИШ\ЪЩ^K[[Ы€‹‚‹ЛИЊNKLL‹LMВ‹ЛВ‹ЛИЬ]Ы€™YHњЭ\€€›Ъ™XЭ[\Л€YX[ќ›Ь€ZZљH[™[ZY\И[€‹ЛИЬљYЪ[[™X]ИЩ€YЩKЪИЫЭ[ќ[\[™›ЭИ™YHЭ\€‹ЛИЪ\љZЩ[€XYЫЫ[HЭЫќШ\™]^Y\њЛ€ЬљYЪ[[]]Ь€‹ЛИ›Щ[ќ][ЩYљYYЩ]™\[[Y\ИћH[љЫ›ЭЫ€\ќY\Л€™YXЭЬ™Y‹ЛИћHИЊNKLL‹LMИИЫЬљИЪ]Э\њ™[ќ›Ъ™XЭ[HЮ\Э[K€‹ЛИШќљ[Э\ЫKЩH™YY\И›Ь€XЪЭШ\™ЫЫ\]Xљ[]HЪ]YШXЮH‹ЛИ[Щ[\Лќ]Э\ќЪ\ЩHШЬљ\\ИH™\ЭЪЪXЩHИ[™H‹ЛИ][\H›Ъ™XЭ[\ИЬ€[ћHЭ\€ЫЬќЩ€ЬXЪX[^™Y‹ЛИ›Ъ™XЭ[HЬ]ЫњЛ‚‹ЛВ‹ЛИ™]\›€•QHY€Э\њИЬ]Ы™YђSСHЫ€Z[‚љ[ќЭ\—ЬЬ]ЫЉ[ќ]H
њ\™[ќЧЬ›Ъ™XЭ[H
њ›Ъ™XЭ[JBћВ€ЫЫњЭ[ќPVФХT”ИHОВ‚€[ќ]H
™[ќH•SВ‚Z[ќHHВ‚Z[ќ[™^HSСSТS‘VУ“У‘NВ€[ќљ\њЭЬЫЬќYHВ‚‚\ЧШ^\ЧЬљ[Ъ\[Щ›Ш]ЬЪ][ЫЋВ‚YWЩ\™XЭ[Ы€\™XЭ[Ы€HT‘PХSУ—Ф’QТВ‚YWЬ›Ъ™XЭ[WЬљ[YH›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWУ“У‘NВ‚‚KЛИY€\™IЬИ›И›Ъ™XЭ[HЬ€\™[ќЩ][™Л^]›ЭЛ‚‚ZY€
\›Ъ™XЭ[H\\™[ќ
B‚^В‚B\™]\›€ђSСNВ‚_B‚B€ЬЪ][Ы‹ћH\™[ќOњЬЪ][Ы‹ћВ‚\ЬЪ][Ы‹ћHH\™[ќOњЬЪ][Ы‹ћH
И›Ъ™XЭ[KOњЬЪ][Ы‹ћNВ‚\ЬЪ][Ы‹ћ€H\™[ќOњЬЪ][Ы‹ћ€
И›Ъ™XЭ[KOњЬЪ][Ы‹ћЋВ‚€ЛИШ[YHЫЫЩ\\ИЫљY™HЬ]Ы‹€ЫЪИ›Ь€[Щ[ИЬ]Ы‹‚‚KЛИK€[љ[X][Ы€›Ъ™XЭ[K‚‚KЛИ‹€ЩX\Ы€[Щ[›Ъ™XЭ[K‚‚KЛИЛ€\ЩH[Щ[›Ъ™XЭ[K‚‚KЛИ€YШXЮHY][‚€YЉ›Ъ™XЭ[KOњЭ\€ЏH
B€В€[™^H›Ъ™XЭ[KOњЭ\ЋВ‚‚B\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWФУХTђСWФ“Т—ФХTЋВ€B‚Y[ЩHY€
\™[ќOќЩX\[ќ	‰€\™[ќOќЩX\[ќO›[Щ[]KњЭXќ\HOHХP•TWФ“Т‘PХSH	‰€\™[ќOќЩX\[ќO›[Щ[]Kњ›Ъ™XЭЏH
B‚^В‚BZ[™^H\™[ќOќЩX\[ќO›[Щ[]Kњ›Ъ™XЭВ‚‚B\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWФУХTђСWХСPTУ—Ф“Т‘PХSNВ‚_B€[ЩHYЉ\™[ќO›[Щ[]KњЭ\€ЏH
B€В€[™^H\™[ќO›[Щ[]KњЭ\ЋВ‚‚B\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWФУХTђСWУSСSФХTЋВ€B€[ЩB€В€[™^HЩ]ШШXЪYЫ[Щ[Ъ[™^
”Э\€ЉNИ‚‚B\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWФУХTђСWСУРђSФХTЋВ€B‚‚\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWРђTСWЦNВ‚\›Ъ™XЭ[WЬљ[YHH“Т‘PХSWФ’SQWУUSђТУSХ’S‘ОВ‚‚KЛИЫЬИX^Э\€ЫЭ[ќ‚€›ЬЉHHИHPVФХT”ОИJККB€В‚BKЛИЬ]Ы€HЭ\€[ќ]K€Y€ЩHZ[^][™™]\›€[ЩK‚€[ќHЬ]ЫЉЬЪ][Ы‹ћЬЪ][Ы‹ћ‹ЬЪ][Ы‹ћK\™XЭ[Ы‹•S[™^•S
NВ€YЉ[ќOH•S
B€В€™]\›€В€B‚€ЛИЩ]™\Э[Щ€\™XЭ[Ы€Yќ\ЭY[ќ€ЩH™YY\И™Y›Ь™HЩHШ[€[™B€ЛИЬЪ][Ыљ[™ИЫ€^\Л‚€\™XЭ[Ы€H\™XЭ[Ы—ЩЩ]ШYќ\ЭY[ќЬ™\Э[
[ќ\™[ќ›Ъ™XЭ[KO™\™XЭ[Ы—ШYќ\Э
NВ‚€К‚€
€\HY][ЬЪ][Ы€Y‚€
€Ь™X]Ь€Y›ЭЪ]™H\ИH[YK‚€
‹В‚€Y€
›Ъ™XЭ[KOњЬЪ][Ы‹ћOH“Т‘PХSWУQРPЦWРУУTUP’SUWФФТUSУ—Ц
B€В€›Ъ™XЭ[KOњЬЪ][Ы‹ћH“Т‘PХSWСQђUSФХT—ФФТUSУ—ЦВ€B‚€ЛИ™]™\њЩHЪ[€\™[ќXЩ\ИYќ‚€Y€
\™XЭ[Ы€OHT‘PХSУ—Ф’QТ
B€В€ЬЪ][Ы‹ћH\™[ќOњЬЪ][Ы‹ћ
И›Ъ™XЭ[KOњЬЪ][Ы‹ћВ€B€[ЩB€В€ЬЪ][Ы‹ћH\™[ќOњЬЪ][Ы‹ћH›Ъ™XЭ[KOњЬЪ][Ы‹ћВ€B‚€[ќOњЬЪ][Ы‹ћHЬЪ][Ы‹ћВ€[ќO™\™XЭ[Ы€H\™XЭ[ЫЋВ‚‚BKЛИЊNKLL‹LMИИH›ЭЭ\™HЪHЩHЩ]]XЪЪ[™ИЩ™‹ќ]‚BKЛИX]љ[™И]\™H›Ь€YШXЮH™Z]љ[Ь‹‚€\™[ќO]XЪЪ[™ИHUPТТS‘ЧУ“У‘NВ‚‚BKЛИљ\њЭЭ\€Ь]Ы™YЫЬќYЩ\ќ™\И\ИH\ЩH›Ь€ЫЬќ[™Л€‚BKЛИ[€H™^Э\‰ЬИЫЬќ\ИH\ЩHHЫЬ[™^€XXЪ‚BKЛИЭXњЩ\]Y[ќЭ\€\X\њИЫ™HЭ\ќ\ќ\€™Z[™[‚‚BKЛИЫЬќ[™ИЬ™\‹€‚BKЛВ‚BKЛИ^€\ЩH
љ\њЭЭ\ЉHHЊЊHHHNKЊH€HN‚‚BZY€
HH
B‚B^В‚BBYљ\њЭЬЫЬќYH[ќOњЫЬќYВ‚B_B‚€[ќOњЫЬќYHљ\њЭЬЫЬќYHNВ‚€[ќOќZЩY[XYЩHH\њ›ЭЧЭZЩY[XYЩNВ€[ќO›ЭЫ™\€H\™[ќВ€[ќO]XЪЪ[™ИHUPТТS‘ЧРPХU‘NВ€[ќO››ЩЬX€HNВ€‚BKЛИЩ]HЭ\€™[ШЪ]HЩ][™Ињ›ЫH[љ[X][Ы‹‚‚BY[ќOќ™[ШЪ]KћH›Ъ™XЭ[KOњЭ\—Э™[ШЪ]VЪWNВ‚‚BKЛИ™]™\њЩH™[ШЪ]HY€\™XЭ[Ы€\ИYќ‚‚BZY€
\™XЭ[Ы€OHT‘PХSУ—УQ•
B‚B^В‚BBY[ќOќ™[ШЪ]KћHY[ќOќ™[ШЪ]KћВ‚B_B‚BB‚BY[ќOќ[љИHЫЫ[[Ы—Э[љОВ€[ќO›™^[љИHЭ[YH
ИNВ€[ќOќћ[[Э™HH•SВ€[ќOќZЩXXЭ[Ы€H•SВ€[ќO›[Щ[]KZ[[Э™HHRSSХ‘LWФХTЋВ€[ќO›[Щ[]KZX]XЪИHRPUPТМWУ“РUPТОВ€‚BKЛИ™[[Э™HЭ\€Ы€ЫЫќXЭ‚‚BZY€
[ќO›[Щ[]Kњ™[[Э™WШЫЫ™љYИ	€‘SSХ‘WРУУ‘’QЧТU
B‚B^В‚BBY[ќO]]ЪЪ[HUUТТSРUPТЧТUВ‚B_B‚BBBB‚BY[ќOњЬЪ][Ы‹ћHHЬЪ][Ы‹ћNВ‚BY[ќO\ЩHHЬЪ][Ы‹ћNВ€[ќOњЬYY][HЋВ€€К‚€
€ЫЬHXЭ[Ы€]K‚€
‹В€€XЭ[Ы—ШЫЬWШ[
[ќ\™[ќ
NВ‚‚BKЛИ\ЪXИ\њљX[€›Ь\ќHЩ]\‚‚BY[ќO›[Щ[]K›[Э™WШЫЫ™љYЧЩ›YЬИH
SХ‘WРУУ‘’QЧФХP’‘PХХЧРђTСSPTSХ‘WРУУ‘’QЧФХP’‘PХХЧСФђU’UHSХ‘WРУУ‘’QЧФХP’‘PХХЧТУHSХ‘WРУУ‘’QЧФХP’‘PХХЧФU“Ф“HSХ‘WРУУ‘’QЧФХP’‘PХХЧХРS
NВ‚BY[ќO›[Щ[]K›[Э™WШЫЫ™љYЧЩ›YЬИ	ЏH“SХ‘WРУУ‘’QЧУ“ЧРQ•TХРђTСNВ‚€[ќOњЬ]Ыќ\HHФUУ—ХTWФ“Т‘PХSWФХTЋВ‚BY[ќOњ›Ъ™XЭ[WЬљ[YHH›Ъ™XЭ[WЬљ[YNВ‚‚BKЛИ^XЭ]HH›Ъ™XЭ[IЬИЫ€Ь]Ы€]™[ќ‚‚BY^XЭ]WЫЫњЬ]Ы—ЬШЬљ\
[ќ
NВ€B€™]\›€•QNВџB‚‚‚ќ›ЪYЭX[WЭ[љК
BћВ€YЉ\Щ[‹O[љ[X][™КB€В€Ъ[Щ[ќ]JЩ[‹ТSСS•UWХ’QССT—ФХPSWРS’SPUSУ—РУУTUJNВ€™]\›ЋВ€B‚€Щ[‹O\ЩH
ПHNВ€Щ[‹OњЬЪ][Ы‹ћHHЩ[‹O\ЩNВџB‚‚‚‹ЛИ›Ь€Hќ\€\HЛLKLЊH\Э\ќќ›ЪY\Э[љК
BћВ€Щ[‹O]XЪЪ[™ИHUPТТS‘ЧРPХU‘NВџB‹ЛИЛLKLЊH\[™‚‚‚‚ќ›ЪYЭX[WЬЬ]ЫЉ›Ш]›Ш]‹›Ш]JBћВ€[ќ]H
™HH•SВ‚€HHЬ]ЫЉ‹K”ЭX[H‹LK•S
NВ‚€YЉHOH•S
B€В€™]\›ЋВ€B‚€KOњЬ]Ыќ\HHФUУ—ХTWФХPSNВ€KO\ЩHHNВ€KO›[Щ[]K›[Э™WШЫЫ™љYЧЩ›YЬИHSХ‘WРУУ‘’QЧУ“ЧРQ•TХРђTСNВ€KOќ[љИHЭX[WЭ[љОВ‚‚KЛИ^XЭ]HHЭX[\ЙЬИЫ€Ь]Ы€]™[ќ‚‚Y^XЭ]WЫЫњЬ]Ы—ЬШЬљ\
JNВџB‚‚‚ќ›ЪYЭX[Y\—Э[љК
BћВ€ЭX[WЬЬ]ЫЉЩ[‹OњЬЪ][Ы‹ћЩ[‹OњЬЪ][Ы‹ћ‹Щ[‹OњЬЪ][Ы‹ћJNВ€Щ[‹O›™^[љИHЭ[YH
И
ЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYYИL
H
И
[™МЉ
H	€МJNВџB‚‚‚ќ›ЪY^Э[љК
HЛИ™]Иќ[Э[Ы€ЫИ^Ш[€™H\Ь^YYћВ€ЛИШZ]ИЭZXЪYB€YЉ\Щ[‹O[љ[X][™КB€В€Ъ[Щ[ќ]JЩ[‹ТSСS•UWХ’QССT—ХVРS’SPUSУ—РУУTUJNВ€BџB‚‹ЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛВ‚‹ЛЪЫZ[™И\њ›ЭИљ[™]И\™Щ]‹ЛИ\H€\™Щ]\B™[ќ]H
љЫZ[™ЧЩљ[™Э\™Щ]
[ќ]J€XЭ[™ЧЩ[ќ]JBћВ€[ќNВ€[ќZ[ЋВ€[ќX^В€€[ќ]J€\™Щ]Щ[ќ]HH•SВ€[ќ]J€Э\њЫЬ—Щ[ќ]NВ‚€ЛЭ\ЩHHШ[И[љ[X][Ы‰ЬИ[™ЩB€YЉ[Y[љ[JXЭ[™ЧЩ[ќ]KS’WХРSКJB€В€Z[€HXЭ[™ЧЩ[ќ]KO›[Щ[]K[љ[X][Ы–РS’WХРSЧKOњ[™ЩKћ›Z[ЋВ€X^HXЭ[™ЧЩ[ќ]KO›[Щ[]K[љ[X][Ы–РS’WХРSЧKOњ[™ЩKћ›X^В€B€[ЩB€В€Z[€HВ€X^HNNNВ€B‚€ЛЩљ[™H	Ы™X\™\Э	ИЫ™B€›ЬЉHHИH[ќЫX^ИJККB€В€Э\њЫЬ—Щ[ќ]HH[ќЫ\ЭЪWNВ‚€YЉЭ\њЫЬ—Щ[ќ]KO™^\ЭИ	‰€Э\њЫЬ—Щ[ќ]HOHXЭ[™ЧЩ[ќ]HЛИШ[ќ\™Щ]Щ[‚€	‰€Э\њЫЬ—Щ[ќ]HOHXЭ[™ЧЩ[ќ]KO›ЭЫ™\€ЛИЫ‰ЭЫИYќ\€ЭЫ™\‹‚€	‰€XЭ[Ы—ШЪXЪЧЪ\ЧЪЬЭ[JXЭ[™ЧЩ[ќ]KЭ\њЫЬ—Щ[ќ]JB€	‰€Y™ЉЭ\њЫЬ—Щ[ќ]KOњЬЪ][Ы‹ћXЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћ
H
ИY™ЉЭ\њЫЬ—Щ[ќ]KOњЬЪ][Ы‹ћ‹XЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћЉHЏHZ[‚€	‰€Y™ЉЭ\њЫЬ—Щ[ќ]KOњЬЪ][Ы‹ћXЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћ
H
ИY™ЉЭ\њЫЬ—Щ[ќ]KOњЬЪ][Ы‹ћ‹XЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћЉHHX^€	‰€Э\њЫЬ—Щ[ќ]KO[љ[X][Ы‹Oќќ[™\X›VШЭ\њЫЬ—Щ[ќ]KO[љ[\ЬЧH
B€В€YЉ]\™Щ]Щ[ќ]HY™ЉЭ\њЫЬ—Щ[ќ]KOњЬЪ][Ы‹ћXЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћ
H
ИY™ЉЭ\њЫЬ—Щ[ќ]KOњЬЪ][Ы‹ћ‹XЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћЉHY™Љ\™Щ]Щ[ќ]KOњЬЪ][Ы‹ћXЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћ
H
ИY™Љ\™Щ]Щ[ќ]KOњЬЪ][Ы‹ћ‹XЭ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћЉJB€В€\™Щ]Щ[ќ]HHЭ\њЫЬ—Щ[ќ]NВ€B€B€B€€™]\›€\™Щ]Щ[ќ]NВџB‚‚ќ›ЪYљZЩWШЬ\Ъ

BћВ€[ќNВ€YЉЩ[‹O™\™XЭ[Ы€OHT‘PХSУ—Ф’QТ
B€В€Щ[‹Oќ™[ШЪ]KћHЋВ€B€[ЩB€В€Щ[‹Oќ™[ШЪ]KћHLЋВ€B€›ЬЉHHИH]™[Щ]ЦШЭ\њ™[ќЬЩ]K›X^^Y\њОИJККB€В€Y€
Ш]™Y]Kљ›Ю\ќ[X›VЪWJHЫЫќ›ЫЬќ[X›JKKL
NВ€B€ЛЪYЉЩ[‹OњЬЪ][Ы‹ћY[Щ^LLЩ[‹OњЬЪ][Ы‹ћ€Y[Щ^
КљY[Ы[Щ\Лљ™\КМL
JHЪ[Щ[ќ]JЩ[ЉNВџB‚‚‚љ[ќљZЩ\—ЭZЩY[XYЩJ[ќ]J€\™Щ]Щ[ќ]K[ќ]J€]XЪЪ[™ЧЩ[ќ]KЧШ]XЪК€]XЪЧЫШљ™XЭ[ќ[Щ›YЛЫЫњЭЧЩY™[њЩJ€Y™[њЩWЫШљ™XЭ
BћВ€[ќ]H
™љ]™\€H•SВ‚€YЉ\™Щ]Щ[ќ]KO™X]ЬЭ]H	€PUФХUWСPQ
B€В€™]\›€В€B€ЛИ™[[€HЫB€YЉ\™Щ]Щ[ќ]KOњЬЪ][Ы‹ћHUСT
B€В€Ъ[Щ[ќ]J\™Щ]Щ[ќ]KТSСS•UWХ’QССT—ХRСWСSPQСWР’RСT—ФU
NВ€™]\›€В€B€YЉ]XЪЪ[™ЧЩ[ќ]HOH\™Щ]Щ[ќ]JB€В€Щ]ЫЬЫ™[ќ
]XЪЪ[™ЧЩ[ќ]K\™Щ]Щ[ќ]JNВ€B‚€YЉ]XЪЧЫШљ™XЭO››ЧЬZ[ЉHЛИЫ‰Э›Ьљ]™\€[ќ[]\ИXY™XШ]\ЩHH]XЪИ\И›ИZ[€Y™™XЭ€В€ЪXЪЩ[XYЩJ\™Щ]Щ[ќ]K]XЪЪ[™ЧЩ[ќ]K]XЪЧЫШљ™XЭY™[њЩWЫШљ™XЭ
NВ€YЉ\™Щ]Щ[ќ]KO™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќ€
B€В€™]\›€NИЛИ›ЭXYY]€B€B‚€ЪXЪЧШXЪЬZ[Љ]XЪЪ[™ЧЩ[ќ]K\™Щ]Щ[ќ]JNВ€Щ]ЬZ[Љ\™Щ]Щ[ќ]K\™Щ]Щ[ќ]KO›\ЭЩ[XYЩWЭ\KJNВ€\™Щ]Щ[ќ]KO]XЪЪ[™ИHUPТТS‘ЧРPХU‘NВ€YЉ]\™Щ]Щ[ќ]KO›[Щ[]K›Щ™њШЬ™Y[љЪ[
B€В€\™Щ]Щ[ќ]KO›[Щ[]K›Щ™њШЬ™Y[љЪ[HLВ€B€\™Щ]Щ[ќ]KOќ[љИHљZЩWШЬ\ЪВ€ЛИЩ[\И\ИH™X[[ќ]KHљ]™\€ЪИZЩHH[XYЩB€YЉ
љ]™\€H›ЬЩљ]™\Љ\™Щ]Щ[ќ]JJJB€В€љ]™\‹OњЬЪ][Ы‹ћHH\™Щ]Щ[ќ]KOњЬЪ][Ы‹ћNВ€љ]™\‹O™›ЬHNВ€љ]™\‹O™\™XЭ[Ы€H\™Щ]Щ[ќ]KO™\™XЭ[ЫЋВ€YЉљ]™\‹OќZЩY[XYЩJB€В€љ]™\‹OќZЩY[XYЩJљ]™\‹]XЪЪ[™ЧЩ[ќ]K]XЪЧЫШљ™XЭ[Щ›YЛY™[њЩWЫШљ™XЭ
NВ€B€[ЩB€В€љ]™\‹O™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќOH]XЪЧЫШљ™XЭO]XЪЧЩ›ЬЩNВ€B€B€\™Щ]Щ[ќ]KO™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќHВ€ЪXЪЩX]
\™Щ]Щ[ќ]JNВ€™]\›€NВџB‚‚‚ќ›ЪYШњЭXЫWЩ[

BћВ€YЉ[Z\ЉЩ[ЉJB€В€™]\›ЋВ€B‚€Щ[‹Oќ™[ШЪ]KћHЩ[‹Oќ™[ШЪ]Kћ€HВ€YЉ
\Щ[‹O[љ[X][™И	‰€[Y[љ[JЩ[‹S’WСQJJH][Y[љ[JЩ[‹S’WСQJJB€В€Ъ[Щ[ќ]JЩ[‹ТSСS•UWХ’QССT—УР”ХPУWСђSУ“ЧСPUРS’SPUSУЉNИЛИљ^YЫИS’WСQHШ[€™H\ЩY€BџB‚‚‚ќ›ЪYШњЭXЫWЩ›J
HЛИ›ЭИШњЭXЫ\ИШ[€›HЪ[€]ZЩHЫ€Ъ[\ЫЫњЛХS•ћВ€ЛЬЩ[‹OњЬЪ][Ы‹ћ
ПHЩ[‹Oќ™[ШЪ]Kћ
€ИЛИ\]Z]™[[ќЩ€ЬYY€YЉЩ[‹OњЬЪ][Ы‹ћ€Y[Щ^
И
љY[Ы[Щ\Лљ™\И
ИЊ
HЩ[‹OњЬЪ][Ы‹ћY[Щ^HЊ
B€В€Ъ[Щ[ќ]JЩ[‹ТSСS•UWХ’QССT—УР”ХPУWС“WУХUУС—Р“ХS‘КNВ€BџB‚‚‚љ[ќШњЭXЫWЭZЩY[XYЩJ[ќ]J€\™Щ]Щ[ќ]K[ќ]J€]XЪЪ[™ЧЩ[ќ]KЧШ]XЪК€]XЪЧЫШљ™XЭ[ќ[Щ›YЛЫЫњЭЧЩY™[њЩJ€Y™[њЩWЫШљ™XЭ
BћВ€YЉ\™Щ]Щ[ќ]KOњЬЪ][Ы‹ћHHUСT
B€В€Ъ[Щ[ќ]J\™Щ]Щ[ќ]KТSСS•UWХ’QССT—ХRСWСSPQСWУР”ХPУWФU
NВ€™]\›€В€B‚€Щ]ЫЬЫ™[ќ
]XЪЪ[™ЧЩ[ќ]K\™Щ]Щ[ќ]JNВ€YЉ\™Щ]Щ[ќ]KO›ЬЫ™[ќ	‰€
\™Щ]Щ[ќ]KO›ЬЫ™[ќO›[Щ[]Kќ\H	€TWФVQTЉJB€В€Y€
Ш]™Y]Kљ›Ю\ќ[X›VЭ\™Щ]Щ[ќ]KO›ЬЫ™[ќOњ^Y\љ[™^JHЫЫќ›ЫЬќ[X›J\™Щ]Щ[ќ]KO›ЬЫ™[ќOњ^Y\љ[™^KНJNВ€B€€К€Ш[Э[]H[™\H[XYЩK€
‹В€ЪXЪЩ[XYЩJ\™Щ]Щ[ќ]K]XЪЪ[™ЧЩ[ќ]K]XЪЧЫШљ™XЭY™[њЩWЫШљ™XЭ
NВ‚€\™Щ]Щ[ќ]KOњ^Y\љ[™^H]XЪЪ[™ЧЩ[ќ]KOњ^Y\љ[™^ИЛИYYЫИЪ[ќИЫИИHЫЬњ™XЭ^Y\‚€YШЫЬ™J]XЪЪ[™ЧЩ[ќ]KOњ^Y\љ[™^]XЪЧЫШљ™XЭO]XЪЧЩ›ЬЩH
€\™Щ]Щ[ќ]KO›[Щ[]K›][\JNИЛИЪ[ќИШ[€›ЭИ™HЪ]™[€›Ь€][™И[€ШњЭXЫB‚€YЉ\™Щ]Щ[ќ]KO™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќH
B€В‚€ЪXЪЩX]
\™Щ]Щ[ќ]JNВ‚€YЉ]XЪЪ[™ЧЩ[ќ]KOњЬЪ][Ы‹ћ\™Щ]Щ[ќ]KOњЬЪ][Ы‹ћ
B€В€\™Щ]Щ[ќ]KOќ™[ШЪ]KћHNВ€B€[ЩB€В€\™Щ]Щ[ќ]KOќ™[ШЪ]KћHLNВ€B‚€\™Щ]Щ[ќ]KO]XЪЪ[™ИHUPТТS‘ЧРPХU‘NИЛИЫИШњЭXЫ\ИШ[€^ЩH[™\ќ^Y\њЛЩ[™[ZY\В‚€YЉ\™Щ]Щ[ќ]KO›[Щ[]KњЭXќ\HOHХP•TWС“QQJHЛИ›ЭИШњЭXЫ\ИШ[€›HZЩHЫ€Ъ[\ЫЫњЛХS•€В€\™Щ]Щ[ќ]KOќ™[ШЪ]Kћ
ЏHВ€\™Щ]Щ[ќ]KOќ[љИHШњЭXЫWЩ›NВ€[ќЬЩ]Ш[љ[J\™Щ]Щ[ќ]KS’WСђS
NВ€B€[ЩB€В€\™Щ]Щ[ќ]KOќ[љИHШњЭXЫWЩ[В‚€YЉ[Y[љ[J\™Щ]Щ[ќ]KS’WСQJJB€В€[ќЬЩ]Ш[љ[J\™Щ]Щ[ќ]KS’WСQK
NИЛИ€KLLЛLHYH™Y›Ь™HЬЬВ€B€[ЩB€В€ЬЬК\™Щ]Щ[ќ]K\™Щ]Щ[ќ]KO›[Щ[]Kљќ[\ZYЪИKЊММКNВ€[ќЬЩ]Ш[љ[J\™Щ]Щ[ќ]KS’WСђS
NВ€B‚€YЉ\™Щ]Щ[ќ]KO›[Щ[]K™X]ШЫЫ™љYЧЩ›YЬИ	€PUРУУ‘’QЧУPPФ“ЧР“S’КB€В€\™Щ]Щ[ќ]KO›[љИHNВ€B€B€B‚€\™Щ]Щ[ќ]KO›™^[љИHЭ[YH
ИNВ€™]\›€NВџB‚‹ЛИШ\ЪЩ^K[[Ы€‹‚‹ЛИЊNLLЌВ‹ЛВ‹ЛИ[ШШ]HY[[ЬћH[™Щ][ќ]H›Ь\ќY\И]Ъ[™H[њЩ™\™Y‹ЛИИH›ЬY][K‚ќ›ЪY[љ]X[^™WЪ][WШШ\њћJ[ќ]H
™[ќЧЬЬ]Ы—Щ[ќћH
њЬ]Ы—Щ[ќћJBћВ€ЛИ]	ЬИЬЬЪX›HИШ[\Ињ›ЫHШЬљ\ЫИY‚€ЛИ\™H\И[™XYHY[[ЬћH›Ь€[€][H[ШШ]Y€ЛИ\™KЫX\€]Э]ИXZЩHЭ\™HЩHЫ‰Э[™\€ЛИЪ][ћHY[[ЬћHXZЬЛ‚€YЉ[ќOљ][WЬ›Ь\ќY\КB€В€њ™YJ[ќOљ][WЬ›Ь\ќY\КNВ€[ќOљ][WЬ›Ь\ќY\ИH•SВ€B‚€ЛИ[ШШ]HY[[ЬћH›Ь€H][K‚€[ќOљ][WЬ›Ь\ќY\ИHX[ШКЪ^™[ЩЉ
™[ќOљ][WЬ›Ь\ќY\КJNВ€Y[\Щ]
[ќOљ][WЬ›Ь\ќY\ЛЪ^™[ЩЉ
™[ќOљ][WЬ›Ь\ќY\КJNВ‚€YЉЬ]Ы—Щ[ќћJB€В€[ќOљ][WЬ›Ь\ќY\ЛOљ[™^HЬ]Ы—Щ[ќћKOљ][WЬ›Ь\ќY\Лљ[™^В‚€YЉЬ]Ы—Щ[ќћKOљ][WЬ›Ь\ќY\Л[X\ЦМJB€В€ЭЬJ[ќOљ][WЬ›Ь\ќY\ЛO[X\ЛЬ]Ы—Щ[ќћKOљ][WЬ›Ь\ќY\Л[X\КNВ€B‚€YЉЬ]Ы—Щ[ќћKOљ][WЬ›Ь\ќY\ЛЫЫЬњЩ]
B€В€[ќOљ][WЬ›Ь\ќY\ЛOЫЫЬњЩ]HЬ]Ы—Щ[ќћKOљ][WЬ›Ь\ќY\ЛЫЫЬњЩ]В€B‚€YЉЬ]Ы—Щ[ќћKOљ][WЬ›Ь\ќY\ЛљX[
B€В€[ќOљ][WЬ›Ь\ќY\ЛOљX[HЬ]Ы—Щ[ќћKOљ][WЬ›Ь\ќY\ЛљX[В€B€[ќOљ][WЬ›Ь\ќY\ЛOњ^Y\—ШЫЭ[ќHЬ]Ы—Щ[ќћKOљ][WЬ›Ь\ќY\Лњ^Y\—ШЫЭ[ќВ€BџB‚™[ќ]H
њЫX\ќЬ]ЫЉЧЬЬ]Ы—Щ[ќћH
њ›ЬКHЛИЛLKLЊH[ќ\™HЩXЭ[Ы€™\XЩYЪ]Ь™[ИЫЩBћВ€[ќ]H
™HH•SВ€[ќ]H
ќЬH•SВ€[ќ^Y\ЫЭ[ќВ‚€YЉ›ЬИOH•SК€€
]™[OH•S	‰‚€JШЬ™Y[—ЬЭ]\И	€
S—ФРФ‘QS—ФСSPХS—ФРФ‘QS—ХUHS—ФРФ‘QS—ТSУС—СђSQHS—ФРФ‘QS—СРSQWУХ‘T€S—ФРФ‘QS—ФТХЧРУУTUHS—ФРФ‘QS—СS‘ТS‘WРФ‘QUS—ФРФ‘QS—УQS•HS—ФРФ‘QS—СРSQWФХT•УQS•HS—ФРФ‘QS—У‘UЧСРSQWУQS•HS—ФРФ‘QS—УРQСРSQWУQS•HS—ФРФ‘QS—УФSУ”ЧУQS•HS—ФРФ‘QS—РУУ•“УУФSУ”ЧУQS•HS—ФРФ‘QS—ФУХS‘УФSУ”ЧУQS•HS—ФРФ‘QS—Х’QSЧУФSУ”ЧУQS•HS—ФРФ‘QS—ФЦTХSWУФSУ”ЧУQS•JJH	‰‚€Э\њ™[ќШЩ[™JJ‹В€
B€В€™]\›€•SВ€B‚€К€Ь]Ы€][H\ЩYЫ€ќ[X™\€Щ€XЭ]™H^Y\њЛ€
‹В€YЉ›ЬЛOњЬ]Ыњ^Y\—ШЫЭ[ќЏH
^Y\ЫЭ[ќHPV
KЫЭ[ќЩ[ќКTWФVQTЉJJJB€В€YЉ›ЬЛO›ЬЬИ	‰€]™[OH•S
B€В€K[]™[O›ЬЬЩ\ШЫЭ[ќВ€B€™]\›€•SВ€B‚€YЉ]™[OH•S	‰€

]™[OњШЬ›Ы\€	€РФ“УТS•РT‘
H
]™[OњШЬ›Ы\€	€РФ“УУХUРT‘
JH
B€В€HHЬ]ЫЉ›ЬЛOњЬЪ][Ы‹ћ›ЬЛOњЬЪ][Ы‹ћ€
ИY[Щ^K›ЬЛOњЬЪ][Ы‹ћK›ЬЛO™›\›ЬЛO›[YK›ЬЛOљ[™^›ЬЛO›[Щ[
NВ€B€[ЩB€В€HHЬ]ЫЉ›ЬЛOњЬЪ][Ы‹ћ
ИY[Щ^›ЬЛOњЬЪ][Ы‹ћ‹›ЬЛOњЬЪ][Ы‹ћK›ЬЛO™›\›ЬЛO›[YK›ЬЛOљ[™^›ЬЛO›[Щ[
NВ€B‚€YЉHOH•S
B€В€™]\›€•SВ€B‚€ЛЬљ[ќЉ‰\Л
	Y‹	Y‹	YЉHH
	Y‹	Y‹	YЉH‹›ЬЛO›[YK›ЬЛOњЬЪ][Ы‹ћ›ЬЛOњЬЪ][Ы‹ћ‹›ЬЛOњЬЪ][Ы‹ћKKOњЬЪ][Ы‹ћKOњЬЪ][Ы‹ћ‹KOњЬЪ][Ы‹ћJNВ‚€ЛИ[X\ПВ€YЉ›ЬЛO[X\ЦМJB€В€Y[\Щ]
KO›[YKЪ^™[ЩЉKO›[YJJNВ€ЭЬJKO›[YK›ЬЛO[X\КNВ€B‚€ЛИY€ЩH]™H][H›Ь\ќY\И[€Ь]Ы€[ќћK[€™\\™HHЩ]Щ‚€ЛИ›Ь\ќY\ИИ\ЬИЫ€ИH][HЪ[€]\И›ЬY‚€YЉ›ЬЛOљ][JB€В€[љ]X[^™WЪ][WШШ\њћJK›ЬКNВ€B€€XЭ[Ы—ШЫЬWЩ]J	™KO›[Щ[]K™XЭ[Ы‹	њ›ЬЛO™XЭ[ЫЉNВ€XЭ[Ы—ШЫЬWЩ]J	™KO™XЭ[Ы‹	њ›ЬЛO™XЭ[ЫЉNВ‚€YЉ›ЬЛOњЬ]Ыќ\JB€В€KOњЬ]Ыќ\HH›ЬЛOњЬ]Ыќ\NИЛМЊLWМЧМЊЛ\ЬИЬ]Ыќ\K‚€B‚€YЉ›ЬЛOљX[Ь^Y\ЫЭ[ќHWHOH
B€В€KO™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќHKO›[Щ[]KљX[H›ЬЛOљX[Ь^Y\ЫЭ[ќHWNВ€B‚€YЉ›ЬЛO›\OH
B€В€KO™[™\™ЮWЬЭ]K›\ШЭ\њ™[ќHKO›[Щ[]K›\H›ЬЛO›\В€B‚€YЉ›ЬЛOњШЫЬ™HOH
B€В€KO›[Щ[]KњШЫЬ™HH›ЬЛOњШЫЬ™NИЛИЭ™\ќЬљ]HШЫЬ™HY€^\ЭИ[€H]™[	ЬЛ€љ[B€B€YЉ›ЬЛO›][\HOH
B€В€KO›[Щ[]K›][\HH›ЬЛO›][\NИЛИЭ™\ќЬљ]H][\HY€^\ЭИ[€H]™[	ЬИќљ[B€B‚€YЉYKO›X\	‰€›ЬЛOЫЫЭ\›X\
B€В€[ќЬЩ]ШЫЫЭ\›X\
K›ЬЛOЫЫЭ\›X\
NВ€B‚€YЉ›ЬЛOYЩЬ™\ЬЪ[ЫЉB€В€KO›[Щ[]KYЩЬ™\ЬЪ[Ы€H›ЬЛOYЩЬ™\ЬЪ[ЫЋИЛИYЩЬ™\ЬЪ[Ы€Ш[€™HЪ[™ЩYЪ]Ь]Ы€Ъ[ќИ›ЭВ€B€YЉ›ЬЛOљ][WЬ›Ь\ќY\Л[JB€В€KOљ][WЬ›Ь\ќY\ЛO[HH›ЬЛOљ][WЬ›Ь\ќY\Л[NВ€B€YЉ›ЬЛO[JB€В€KO›[Щ[]K[HH›ЬЛO[NВ€B‚€ЛИ™X€Ќ‹ЊHHЭЬ™HHЬљYЪ[[X\И™HX›HИ™\ЭЬ™HЪ]Z[™И›\Ъ€YЉ›ЬЛO™Z[™КB€В€KO™Z[™ИH›ЬЛO™Z[™ОИЛИ™X€Ќ‹ЊHH\ЩYИYљ[™HЪXЪЫЫЭ\›X\\И\ЩY›Ь€HZ[™И›\Ъ€KOњ\ЊHH›ЬЛOњ\ЊNИЛИX\€ЊKЊHH\ЩYИЭЬ™HЭ\ЭЫH\Щ[ќYЩ\В€KOњ\Њ€H›ЬЛOњ\ЊЋИЛИX\€ЊKЊHH\ЩYИЭЬ™HЭ\ЭЫH\Щ[ќYЩ\В€KO™Z[™М€H›ЬЛO™Z[™МЋИЛИXИMKЊM€H\ЩYИYљ[™HЪXЪЫЫЭ\›X\\И\ЩY›Ь€HZ[™И›\Ъ›Ь€\Њ€ћHЪ]HYЫЫ‚€B‚€YЉ›ЬЛO››ЫY™JB€В€KO›[Щ[]K››ЫY™HH›ЬЛO››ЫY™NИЛИЭ™\ќЬљ]HЪ]\€]™H\Иљ\ЪX›HЬ€›Э€B€KO›ЬЬИH›ЬЛO›ЬЬОВ‚€YЉ›ЬЛO›ЬЬИ	‰€]™[OH•S	‰€]™[O›ЬЬЫ]\ЪXЦМJB€В€]\ЪXК]™[O›ЬЬЫ]\ЪXЛK]™[O›ЬЬЫ]\ЪXЧЫЩ™њЩ]
NВ€B‚€ЛИЪ]™HH[ќ]HHЩX\Ы€][B€YЉ›ЬЛOќЩX\ЫЉB€В€ЬHЬ]ЫЉKOњЬЪ][Ы‹ћUSWТQWФФТUSУ—Ц‹›ЬЛOќЩX\Ы‹›ЬЛOќЩX\Ыљ[™^›ЬЛOќЩX\Ы›[Щ[
NВ€YЉЬ
B€В€ЛЩ[ќЩY][Ъ[љ]
Ь
NВ€Щ]ЭЩX\ЫЉKЬO›[Щ[]KќЩX\Ы—Ь›Ь\ќY\ЛќЩX\Ы—Ъ[™^
NВ€KOќЩX\[ќHЬВ‚€KOќЩX\[ќOњЬ]Ыќ\HHФUУ—ХTWХСPTУЋВ€B€B‚€ЛИЩ][ќ]H\N€^Y\‹[™[^KњЛ‹‹‚€YЉ›ЬЛO™[ќ]]\JB€В€KO›[Щ[]Kќ\HH›ЬЛO™[ќ]]\NВ€B‚€ЛИЩ]H\™[ќ€YЉ›ЬЛOњ\™[ќ
HЛЛOќ\›\ЭOќ\њЛOќќOH•СSTB€В€KOњ\™[ќH
[ќ]H
Љ\›ЬЛOњ\™[ќВ€B‚€ЛЩ[ќЩY][Ъ[љ]
JNВ€^XЭ]WЫЫњЬ]Ы—ЬШЬљ\
JNВ€^XЭ]WЬЬ]Ы—ЬШЬљ\
›ЬЛJNВ€™]\›€NВџHЛИЛLKLЊH™\XЩYЩXЭ[Ы€[™И\™B‚љ[ќ\ЧЪ[Ш[J›Ш]›Ш]‹›Ш]K›Ш]™\ЪЫ
BћВ€Y€
]™[
B€В€Y€
ЏHY[Щ^
Э™\ЪЫ	‰€HY[Щ^
ЭљY[Ы[Щ\Лљ™\Л]™\ЪЫ	‰€‹XHЏHY[Щ^JЭ™\ЪЫ	‰€‹XHHY[Щ^JЭљY[Ы[Щ\Лќ”™\ЛM
HВ€Y€
ЏHШЬ›ЫZ[ћ	‰€HШЬ›ЫX^
ЭљY[Ы[Щ\Лљ™\И	‰€€ЏHVQT—УRS—Ц€	‰€€HVQT—УPVЦ€
HВ€™]\›€NВ€B€B€B‚€™]\›€ВџB‚ќ›ЪYЬ]Ыњ^Y\Љ[ќ[™^
BћВ€ЧЬЬ]Ы—Щ[ќћHВ€ЛЬЧЫ[Щ[
€[Щ[H•SВ€[ќШ[В€[ќЛЛљ[™HВ€[™^	ЏHОВ‚‹ЛИ[Щ[Hљ[™Ы[Щ[
^Y\–Ъ[™^K›[YJNВ‹ЛИYЉ[Щ[OH•S
H™]\›ЋВ‚€Y[\Щ]
	њЪ^™[ЩЉ
JNВ€›[YHH^Y\–Ъ[™^K›[YNВ€љ[™^HLNВ€љ][WЬ›Ь\ќY\Лљ[™^HLNВ€ќЩX\Ыљ[™^HLNВ€ЫЫЭ\›X\H^Y\–Ъ[™^KЫЫЭ\›X\В€њЬ]Ыњ^Y\—ШЫЭ[ќHLNВ€њЬ]Ыќ\HHФUУ—ХTWФVQT—УPRSЋВ‚€YЉ]™[OњШЬ›Ы\€	€РФ“УУQ•
B€В€YЉ]™[OњЬ]Ы€	‰€]™[OњЬ]Ы–Ъ[™^Kћ
B€В€њЬЪ][Ы‹ћH
›Ш]
JљY[Ы[Щ\Лљ™\ИH]™[OњЬ]Ы–Ъ[™^Kћ
NВ€B€[ЩB€В€њЬЪ][Ы‹ћH
›Ш]
J
љY[Ы[Щ\Лљ™\ИHЊ
HHМ
€[™^
NВ€B€B€[ЩB€В€YЉ]™[OњЬ]Ы€	‰€]™[OњЬ]Ы–Ъ[™^Kћ
B€В€њЬЪ][Ы‹ћH
›Ш]
J]™[OњЬ]Ы–Ъ[™^Kћ
NВ€B€[ЩB€В€њЬЪ][Ы‹ћH
›Ш]
JЊ
ИМ
€[™^
NВ€B€™›\HNВ€B‚€YЉ]™[OњЬ]Ы€	‰€]™[OњЬ]Ы–Ъ[™^KћЉB€В€YЉ]™[OњШЬ›Ы\€	€
РФ“УТS•РT‘РФ“УУХUРT‘
JB€В€њЬЪ][Ы‹ћ€H
›Ш]
J]™[OњЬ]Ы–Ъ[™^KћЉNВ€B€[ЩB€В€њЬЪ][Ы‹ћ€H
›Ш]
JVQT—УRS—Ц€
И]™[OњЬ]Ы–Ъ[™^KћЉNВ€B€B€[ЩHYЉVQT—УPVЦ€HVQT—УRS—Ц€€JB€В€њЬЪ][Ы‹ћ€H
›Ш]
JVQT—УRS—Ц€
ИJNВ€B€[ЩB€В€њЬЪ][Ы‹ћ€H
›Ш]
TVQT—УRS—ЦЋВ€B‚€YЉњЬЪ][Ы‹ћ€VQT—УRS—ЦЉB€В€њЬЪ][Ы‹ћ€HVQT—УRS—ЦЋВ€B€[ЩHYЉњЬЪ][Ы‹ћ€€VQT—УPVЦЉB€В€њЬЪ][Ы‹ћ€HVQT—УPVЦЋВ€B‚€ЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛШЪXЪЪ[™ИЫ\ЛИШ[ЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛВ€›ЬЉИHИИљY[Ы[Щ\Лљ™\ИИИКККB€В€YЉњЬЪ][Ы‹ћЏHљY[Ы[Щ\Лљ™\КB€В€њЬЪ][Ы‹ћOHљY[Ы[Щ\Лљ™\ОВ€B€YЉњЬЪ][Ы‹ћ
B€В€њЬЪ][Ы‹ћ
ПHљY[Ы[Щ\Лљ™\ОВ€B€YЉVQT—УRS—Ц€OHVQT—УPVЦЉB€В€Ш[HЪXЪЭШ[Ъ[™^
Y[Щ^
ИњЬЪ][Ы‹ћњЬЪ][Ы‹ћЉNВ€YЉШ[ЏH	‰€]™[OќШ[ЦЭШ[KљZYЪPVХРSТRQТ
B€В€њ™XZОИЛЩ›Э[™€B€YЉЪXЪЪЫWЪ[ЉY[Щ^
ИњЬЪ][Ы‹ћњЬЪ][Ы‹ћ‹њЬЪ][Ы‹ћJH
Ш[ЏH	‰€]™[OќШ[ЦЭШ[KљZYЪЏHPVХРSТRQТ
JB€В€љ[™HВ€B€[ЩB€В€њ™XZОИЛИ›Э[™€B€B€[ЩH›ЬЉИHИИ
VQT—УPVЦ€HVQT—УRS—ЦЉHИОИККЛњЬЪ][Ы‹ћ€
ПHКB€В€YЉњЬЪ][Ы‹ћ€€VQT—УPVЦЉB€В€њЬЪ][Ы‹ћ€OHVQT—УPVЦ€HVQT—УRS—ЦЋВ€B€YЉњЬЪ][Ы‹ћ€VQT—УRS—ЦЉB€В€њЬЪ][Ы‹ћ€
ПHVQT—УPVЦ€HVQT—УRS—ЦЋВ€B€Ш[HЪXЪЭШ[Ъ[™^
Y[Щ^
ИњЬЪ][Ы‹ћњЬЪ][Ы‹ћЉNВ€YЉШ[ЏH	‰€]™[OќШ[ЦЭШ[KљZYЪPVХРSТRQТ
B€В€љ[™HNВ€њ™XZОВ€B€[ЩHYЉШ[ЏH	‰€]™[OќШ[ЦЭШ[KљZYЪЏHPVХРSТRQТ
B€В€ЫЫќ[ќYNВ€B€YЉЪXЪЪЫWЪ[ЉY[Щ^
ИњЬЪ][Ы‹ћњЬЪ][Ы‹ћ‹њЬЪ][Ы‹ћJJB€В€ЫЫќ[ќYNВ€B€љ[™HNВ€њ™XZОВ€B€YЉљ[™
B€В€њ™XZОВ€B€њЬЪ][Ы‹ћ
ПH
]™[OњШЬ›Ы\€	€РФ“УУQ•
HИM€В€B€ЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛЛВ€Э\њ™[ќЬ]Ыњ^Y\€H[™^В€^Y\–Ъ[™^K™[ќHЫX\ќЬ]ЫЉ	њ
NВ‚€YЉ^Y\–Ъ[™^K™[ќOH•S
B€В€›Ь”Ъ]ЭЫЉK‘][€[X›HИЬ]Ы€^Y\€њ›ЫH	Й\ЙЧ€‹›[YJNВ€B‚€^Y\–Ъ[™^K™[ќOњ^Y\љ[™^H[™^В‚€YЉ›ЫX^ќ\Ъ™\Щ]НHЏHJB€В€^Y\–Ъ[™^K™[ќOњќ\Ъ›X^H›ЫX^ќ\Ъ™\Щ]Ъ[™^NВ€B€[ЩB€В€^Y\–Ъ[™^K™[ќOњќ\Ъ›X^HВ€B‚€Y[\Щ]
€^Y\–Ъ[™^KЫЫ[X[™Ъ[њ]Ъ\ЭЬћK€€Ъ^™[ЩЉ^Y\–Ъ[™^KЫЫ[X[™Ъ[њ]Ъ\ЭЬћJB€
NВ‚€Y[\Щ]
€^Y\–Ъ[™^KЫЫ[X[™Ъ[њ]ЪЫЬЭ\ќЭ[YK€€Ъ^™[ЩЉ^Y\–Ъ[™^KЫЫ[X[™Ъ[њ]ЪЫЬЭ\ќЭ[YJB€
NВ‚€Y[\Щ]
€^Y\–Ъ[™^KЫЫ[X[™Ъ[њ]ЪЫЭљYЩЩ\—Э[YK€€Ъ^™[ЩЉ^Y\–Ъ[™^KЫЫ[X[™Ъ[њ]ЪЫЭљYЩЩ\—Э[YJB€
NВ‚€^Y\–Ъ[™^KЫЫ[X[™Ъ[њ]ЪЫЬЭ\ќЭ[YHВ€^Y\–Ъ[™^KЫЫ[X[™Ъ[њ]ЪЫЭљYЩЩ\—Э[YHВ€^Y\–Ъ[™^KЫЫ[X[™Ъ[њ]ШЫЭ[ќHВ€^Y\–Ъ[™^KЫЫ[X[™Ъ[њ]Ъ[™^HВ‚€YЉ^Y\–Ъ[™^KњЬ]ЫљX[
B€В€^Y\–Ъ[™^K™[ќO™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќH^Y\–Ъ[™^KњЬ]ЫљX[
ИNВ€B€YЉ^Y\–Ъ[™^K™[ќO™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќ€^Y\–Ъ[™^K™[ќO›[Щ[]KљX[
B€В€^Y\–Ъ[™^K™[ќO™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќH^Y\–Ъ[™^K™[ќO›[Щ[]KљX[В€B‚€ЛЫ\]H™XЫЬќ™\€Yќ\€H]™[ћHZ[В€YЉ^Y\–Ъ[™^KњЬ]Ы›\
B€В€^Y\–Ъ[™^K™[ќO™[™\™ЮWЬЭ]K›\ШЭ\њ™[ќH^Y\–Ъ[™^KњЬ]Ы›\
ИЋВ€B€YЉ^Y\–Ъ[™^K™[ќO™[™\™ЮWЬЭ]K›\ШЭ\њ™[ќ€^Y\–Ъ[™^K™[ќO›[Щ[]K›\
B€В€^Y\–Ъ[™^K™[ќO™[™\™ЮWЬЭ]K›\ШЭ\њ™[ќH^Y\–Ъ[™^K™[ќO›[Щ[]K›\В€B‚€YЉ^Y\–Ъ[™^KќЩX\ќ[JB€В€Щ]ЭЩX\ЫЉ^Y\–Ъ[™^K™[ќ^Y\–Ъ[™^KќЩX\ќ[K
NВ€B€[ЩB€В€Щ]ЭЩX\ЫЉ^Y\–Ъ[™^K™[ќ]™[OњЩ]ЩX\
NВ€BџB‚љ[ќ›ЧЬ^Y\—Ш[]™WЭЧЪ›Ъ[Љ
BћВ€[ќ›ЧШ[]™WЬ^Y\њИHВ€[ќNВ€›ЬЉHHИHPVФVQT”ОИJККB€В€YЉ

\^Y\–ЪWK™[ќ^Y\–ЪWK›]™\ИH
^Y\–ЪWK›]™\ИHH	‰€^Y\–ЪWK™[ќO™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќH
JH	‰‚€

›ЬЪ\™H	‰€^Y\–ЪWKЬ™Y]ИH
H
[›ЬЪ\™H	‰€Ь™Y]ИH
JJB€
B€В€
КЫ›ЧШ[]™WЬ^Y\њОВ€B€B€›ЧШ[]™WЬ^Y\њИH
›ЧШ[]™WЬ^Y\њИЏHPVФVQT”КHИH€В‚€™]\›€›ЧШ[]™WЬ^Y\њОВџB‚ќ›ЪYЪ[Ш[Ь^Y\њЧШћWЭ[Y[Э™\Љ
BћВ€[ќNВ€ЧШ]XЪИ]XЪЧЭ[Y[Э™\€H[\X]XЪОВ€ЧШ]XЪИ]XЪЧЫЬЩHH[\X]XЪОВ€ЫЫњЭЧЩY™[њЩJ€Y™[њЩWЫШљ™XЭH•SВ‚€]XЪЧЭ[Y[Э™\‹]XЪЧЭ\HHUЧХSQSХ‘TЋВ€]XЪЧЭ[Y[Э™\‹™›Ь‹ћHHY][Ы[Щ[Щ›Ь‹ћNВ€]XЪЧЭ[Y[Э™\‹™›Ь‹ћHY][Ы[Щ[Щ›Ь‹ћВ€]XЪЧЭ[Y[Э™\‹™›Ь‹ћ€HY][Ы[Щ[Щ›Ь‹ћЋВ‚€]XЪЧЫЬЩK]XЪЧЭ\HHUЧУФСNВ€‚€[™Ш[YHHNВ€›ЬЉHHИHPVФVQT”ОИJККB€В€[ќ]J€\HЩ[ЋВ€Щ[€H^Y\–ЪWK™[ќВ€YЉЩ[€	‰€][Y[љ[JЩ[‹S’WУФСJJB€В€[™Ш[YHHВ€]XЪЧЭ[Y[Э™\‹]XЪЧЩ›ЬЩHHЩ[‹O™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќВ‚€Y™[њЩWЫШљ™XЭHY™[њЩWЩљ[™ШЭ\њ™[ќЫШљ™XЭ
Щ[‹•S]XЪЧЭ[Y[Э™\‹]XЪЧЭ\JNВ€€Щ[‹OќZЩY[XYЩJЩ[‹Щ[‹	]XЪЧЭ[Y[Э™\‹Y™[њЩWЫШљ™XЭ
NВ€B€[ЩHYЉЩ[ЉB€В€[™Ш[YHHВ‚€]XЪЧЫЬЩK]XЪЧЩ›ЬЩHHЩ[‹O™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќВ€Y€
[Z\ЉЩ[ЉH	‰€[Y[љ[JЩ[‹S’WСђSФСJJB€В€]XЪЧЫЬЩK™›Ь‹ћHHY][Ы[Щ[Щ›Ь‹ћNВ€]XЪЧЫЬЩK™›Ь‹ћHY][Ы[Щ[Щ›Ь‹ћВ€]XЪЧЫЬЩK™›Ь‹ћ€HY][Ы[Щ[Щ›Ь‹ћЋВ€Щ[‹O›[Щ[]K™X]ШЫЫ™љYЧЩ›YЬИHPUРУУ‘’QЧУPPФ“ЧСPUСђSУS‘В€B€[ЩB€В€Щ[‹O›[Щ[]K™X]ШЫЫ™љYЧЩ›YЬИHPUРУУ‘’QЧУPPФ“ЧСPUВ€B‚€Y™[њЩWЫШљ™XЭHY™[њЩWЩљ[™ШЭ\њ™[ќЫШљ™XЭ
Щ[‹•S]XЪЧЫЬЩK]XЪЧЭ\JNВ€€Щ[‹OќZЩY[XYЩJЩ[‹Щ[‹	]XЪЧЫЬЩKY™[њЩWЫШљ™XЭ
NВ€B€Щ[€H\В€BџB‚ќ›ЪY[YWЫЭ™\Љ
BћВ€YЉ]™[Oќ\HOHJB€В€]™[ШЫЫ\]YHNИЛИ™X€ЌKЊHH\ЩY›Ь€›Ыќ\И]™[ИЫИHY™H\Ы‰ЭZЩ[€]Ш^HY€[YH^\™\Л›]™[Oќ\HOHHYX[њИ›Ыќ\И]™[[ЩH™YЭ[\‚€B€[ЩHYЉ[]™[ШЫЫ\]Y
B€В€Ъ[Ш[Ь^Y\њЧШћWЭ[Y[Э™\Љ
NВ‚€Y€
Z\ЧЭЭ[Э[Y[Э™\ЉB€В€YЉЫШ[ЬШ[\WЫ\Эќ[YWЫЭ™\€ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Эќ[YWЫЭ™\‹Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B‚€[Y[YќH]™[OњЩ][YH
€ЫШ[ШЫЫ™љYЛЫЭ[ќ\—ЬЬYYИЛИ™X€ЌЊHH\И[™H[Э™Y\™HИЩ]Э\ЭЫH[YB€YЉY[™Ш[YJB€В€ЪЭЭ[Y[Э™\€HNВ€B€B€Y€
Z\ЧЭЭ[Э[Y[Э™\€	‰€›ЧЬ^Y\—Ш[]™WЭЧЪ›Ъ[Љ
JB€В€\ЧЭЭ[Э[Y[Э™\€HNВ€›Ъ›Ъ[€HNВ€B€BџB‚‚‹ЛИKKKKKKKKKKKKKKKKKKKKKKH\]Hќ[Э[ЫњИKKKKKKKKKKKKKKKKKKKKKKKKKKKKKB‚ќ›ЪY\]WЬШЬ›Ы\Љ
BћВ€›Ш]ИHВ€[ќKYШZ[њЭ[™HВ€[ќќ[\^HHИЛН^Y\‚€›Ш]HY[Щ^HHY[Щ^NВ€›Ш]›HHNNNNKHHNNNNNK›HHNNNNKHHNNNNNNИЛЬ^Y\€›Э[™\ћH›Ю€Э]XИ[ќШЬ›ЫYHВ€ШЬ›ЫHШЬ›ЫHHВ€[ќШ[]™HHВ‚€YЉЭ[YH]™[OY[Щ][YHњ™Y^™X[
B€В€™]\›ЋИЛИYYњ™Y^™X[ЫИXЪЩЬ›Э[™ЛЬШЬ›Ы[™ИЫ‰Э\]HY€[љ[X][ЫњИ\™Hњ›Ю™[‚€B‚€К‚€KЛЫ]™[OY[Щ][YHHЭ[YH
И
ЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYYМL
NИЛИЪ[™ЩYЫИШЬ›Ы[™ИЬYYИ\›Ь€\Э\€^Y\њВ€[]™[OY[Щ][YHHЭ[YHB€BJ
^Y\–МK™[ќ	‰€
^Y\–МK™[ќO›[Щ[]KњЬYYћЏHL€^Y\–МK™[ќO›[Щ[]Kњќ[њЬYYЏHLЉJH€BH
^Y\–МWK™[ќ	‰€
^Y\–МWK™[ќO›[Щ[]KњЬYYћЏHL€^Y\–МWK™[ќO›[Щ[]Kњќ[њЬYYЏHLЉJH€BH
^Y\–М—K™[ќ	‰€
^Y\–М—K™[ќO›[Щ[]KњЬYYћЏHL€^Y\–М—K™[ќO›[Щ[]Kњќ[њЬYYЏHLЉJH€BH
^Y\–МЧK™[ќ	‰€
^Y\–МЧK™[ќO›[Щ[]KњЬYYћЏHL€^Y\–МЧK™[ќO›[Щ[]Kњќ[њЬYYЏHLЉJH
NИЛИЪ[™ЩYЫИY€[Э\€^Y\€\И\Э\€HXЪЩЬ›Э[™ИШЬ›Ы\Э\Љ‹В‚€]™[OY[Щ][YHHЭ[YNВ‚€YЉ]™[ШЫЫ\]Y
B€В€™]\›ЋВ€B‚€›ЬЉHHИHPVФVQT”ОИJККB€В€Y€
^Y\–ЪWK™[ќ	‰€J^Y\–ЪWK™[ќO™X]ЬЭ]H	€PUФХUWСPQ
JB€В€Ш[]™HHNВ€њ™XZОВ€B€B‚€ЛХЪ]HYЫЫЋ€›И[Ь™H[™[ZY\ИB€YЉЭ\њ™[ќЬЬ]Ы€ЏH]™[O›ќ[\Ь]ЫњИ	‰€Yљ[™[ќ
TWСS‘SVJH	‰€Ш[]™JB€В€YЉYљ[™[ќ
TWСS‘U‘S
H	‰€

Yљ[™[ќ
TWТUSHTWУР”ХPУJH	‰€]™[Oќ\HOHJH]™[Oќ\HOH
JHЛИ™X€ЌKЊHHYYЫИШњЭXЫ\В€В€]™[ШЫЫ\]YHNИЛИШ[€™H\ЩY›Ь€›Ыќ\И]™[В€B€B€[ЩHYЉЫЭ[ќЩ[ќКTWСS‘SVJHЬ›Э\Z[ЉB€В€Ъ[JЫЭ[ќЩ[ќКTWСS‘SVJHЬ›Э\X^	‰‚€Э\њ™[ќЬЬ]Ы€]™[O›ќ[\Ь]ЫњИ	‰‚€]™[OњЬИЏH]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—K]€
B€В€YЉ]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—K›]\ЪXЩYJB€В€]\ЪXЩYVМHH
›Ш]
[]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—K›]\ЪXЩYNВ€]\ЪXЩYVМWHH
›Ш]
\Ш]™Y]K›]\ЪXЭ›ЫВ€B€[ЩHYЉ]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—K›]\ЪXЦМJB€В€Э›ЬJ]\ЪXЫ[YK]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—K›]\ЪXЛPVР•Q‘‘T—УSЉNВ€]\ЪXЫЩ™њЩ]H]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—K›]\ЪXЫЩ™њЩ]В€]\ЪXЫЫЬHNВ€B€[ЩHYЉ]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—KќШZ]
B€В€]™[OќШZ][™ИHNВ€ЫЧЭ[YHHВ€B€[ЩHYЉ]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—K™Ь›Э\Z[€]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—K™Ь›Э\X^
B€В€Ь›Э\Z[€H]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—K™Ь›Э\Z[ЋВ€Ь›Э\X^H]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—K™Ь›Э\X^В€B€[ЩHYЉ]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—K››Ъ›Ъ[€OH
B€В€›Ъ›Ъ[€H
]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—K››Ъ›Ъ[€OHJNВ€B€[ЩHYЉ]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—KњШЬ›ЫZ[ћ€	€
B€В€ШЬ›ЫZ[ћ€H
›Ш]
J]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—KњШЬ›ЫZ[ћ€	€Щ™™™™™™ЉNВ€ШЬ›ЫX^€H
›Ш]
[]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—KњШЬ›ЫX^ЋВ€YЉWЭ[YJB€В€Y[Щ^HHШЬ›ЫZ[ћЋИЛИ™\Щ]HY€Ь]Ы€]™\ћH™YЪ[›љ[™В€B€B€[ЩHYЉ]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—KњШЬ›ЫZ[ћ	€
B€В€ШЬ›ЫZ[ћH
›Ш]
J]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—KњШЬ›ЫZ[ћ	€Щ™™™™™™ЉNВ€ШЬ›ЫX^H
›Ш]
[]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—KњШЬ›ЫX^В€B€[ЩHYЉ]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—K›ШЪШYJB€В€ЛИ\ЬЭ[YH]™[Ь]Ы€[ќћHЪ[›Э›ЫXЪЛЫИќ\ЭЪ[™ЩH]И\™B€YЉ]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—K›ШЪШYH
B€В€]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—K›ШЪШYHHВ€B€›ШЪШYHH
›Ш]
[]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—K›ШЪШYNВ€B€[ЩHYЉ]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—Kњ[]HOH
B€В€ЛИ\ЬЭ[YH]™[Ь]Ы€[ќћHЪ[›Э›ЫXЪЛЫИќ\ЭЪ[™ЩH]И\™B€YЉ]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—Kњ[]H
B€В€]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—Kњ[]HHВ€B€Ъ[™ЩWЬЮ\Э[WЬ[]J]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—Kњ[]JNВ€B€[ЩHYЉ]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—K›YЪћJHЛИЪ[™ЩHYЪ\™XЭ[Ы€›Ь€ЩћЪYЭВ€В€YЪћH]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—K›YЪћВ€YЪћHH]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—K›YЪћNВ€B€[ЩHYЉ]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—KњЪYЭШЫЫЬЉHЛИЪ[™ЩHЫЫЬ€›Ь€ЩћЪYЭВ€В€ЪYЭШЫЫЬ€H]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—KњЪYЭШЫЫЬЋВ€YЉЪYЭШЫЫЬ€OHLJB€В€ЪYЭШЫЫЬ€HВ€B€[ЩHYЉЪYЭШЫЫЬ€OHLЉB€В€ЪYЭШЫЫЬ€HLNВ€B€B€[ЩHYЉ]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—KњЪYЭШ[JHЛИЪ[™ЩHЫЫЬ€›Ь€ЩћЪYЭВ€В€ЪYЭШ[HH]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—KњЪYЭШ[NВ€YЉЪYЭШ[HOH“S‘УSСWУSСS
B€В€ЪYЭШ[HH“S‘УSСWУ“У‘NВ€B€B€[ЩHYЉ]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—KњЪYЭЫЬXЪ]JHЛИЪ[™ЩHЫЫЬ€›Ь€ЩћЪYЭВ€В€ЪYЭЫЬXЪ]HH]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—KњЪYЭЫЬXЪ]NВ€YЉЪYЭЫЬXЪ]HOHLJB€В€ЪYЭЫЬXЪ]HHВ€B€YЉЪYЭЫЬXЪ]HOHLЉB€В€ЪYЭЫЬXЪ]HHLNВ€B€B€[ЩB€В€ЫX\ќЬ]ЫЉ	›]™[OњЬ]ЫњЪ[ќЦШЭ\њ™[ќЬЬ]Ы—JNВ€B€
КШЭ\њ™[ќЬЬ]ЫЋВ€B€B‚€›ЬЉHHИH]™[Щ]ЦШЭ\њ™[ќЬЩ]K›X^^Y\њОИJККB€В€YЉ^Y\–ЪWK™[ќ
B€В€YЉ^Y\–ЪWK™[ќOњЬЪ][Ы‹ћ€›JB€В€›HH^Y\–ЪWK™[ќOњЬЪ][Ы‹ћВ€B€YЉ^Y\–ЪWK™[ќOњЬЪ][Ы‹ћJB€В€HH^Y\–ЪWK™[ќOњЬЪ][Ы‹ћВ€B€YЉ^Y\–ЪWK™[ќOњЬЪ][Ы‹ћ€€›JB€В€›HH^Y\–ЪWK™[ќOњЬЪ][Ы‹ћЋВ€B€YЉ^Y\–ЪWK™[ќOњЬЪ][Ы‹ћ€JB€В€HH^Y\–ЪWK™[ќOњЬЪ][Ы‹ћЋВ€B€ќ[\^JКОВ€B€B‚€YЉ]™[OќШZ][™КB€В€ЛИШZ]›Ь€[[™[ZY\ИИ™HY™X]Y€YЉYљ[™[ќ
TWСS‘SVJJB€В€]™[OќШZ][™ИHВ€YЉ]™[O››Ь™\Щ]HJB€В€[Y[YќH]™[OњЩ][YH
€ЫШ[ШЫЫ™љYЛЫЭ[ќ\—ЬЬYYИЛИ™X€ЌЊHH\И[™H[Э™Y\™HИЩ]Э\ЭЫH[YB€B€ЫЧЭ[YHHЭ[YH
ИИ
€ЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYYВ€B€B‚€YЉќ[\^HOH
B€В€™]\›ЋВ€B‚‚‚€YЉ[]™[OќШZ][™ИЫШ[ШЫЫ™љYЛЪX]И	€ТPUУФSУ”ЧТSTPРP“WРPХU‘JB€В€YЉ]™[OњШЬ›Ы\€	€РФ“УФ’QТ
B€В‚€YШZ[њЭ[™H
]™[OќЪYHљY[Ы[Щ\Лљ™\КNВ‚€YЉ›HHH€љY[Ы[Щ\Лљ™\КB€В€ИHY[Щ^В€B€[ЩB€В€ИH
H
И›JHИ€HљY[Ы[Щ\Лљ™\ИИ€
И]™[OШ[Y\^Щ™њЩ]В€B‚€YЉИШЬ›ЫZ[ћ
B€В€ИHШЬ›ЫZ[ћВ€B€[ЩHYЉИ€ШЬ›ЫX^
B€В€ИHШЬ›ЫX^В€B‚€YЉ
]™[OњШЬ›Ы\€	€РФ“УРђPТКH	‰€И›ШЪШYJB€В€ИH›ШЪШYNВ€B‚€YЉИ€Y[Щ^
B€В€YЉИ€Y[Щ^
И]™[OњШЬ›ЫЬYY
B€В€ИHY[Щ^
И]™[OњШЬ›ЫЬYYВ€B€Y[Щ^HОВ€B€[ЩHYЉ
]™[OњШЬ›Ы\€	€РФ“УРђPТКH	‰€ИY[Щ^
B€В€YЉИY[Щ^H]™[OњШЬ›ЫЬYY
B€В€ИHY[Щ^H]™[OњШЬ›ЫЬYYВ€B€Y[Щ^HОВ€B‚€YЉY[Щ^
B€В€Y[Щ^HВ€B€YЉY[Щ^ЏH]™[OќЪYHљY[Ы[Щ\Лљ™\КB€В€Y[Щ^H
›Ш]
[]™[OќЪYHљY[Ы[Щ\Лљ™\ОВ€YШZ[њЭ[™HNВ€B‚€YЉYШZ[њЭ[™
B€В€]™[OњЬККОВ€B€[ЩB€В€]™[OњЬИH
[ќ
XY[Щ^В€B‚‚€B€[ЩHYЉ]™[OњШЬ›Ы\€	€РФ“УУQ•
B€В‚€YШZ[њЭ[™H
]™[OќЪYHљY[Ы[Щ\Лљ™\КNВ‚€YЉ›HHH€љY[Ы[Щ\Лљ™\КB€В€ИHY[Щ^В€B€[ЩB€В€ИH
H
И›JHИ€HљY[Ы[Щ\Лљ™\ИИ€
И]™[OШ[Y\^Щ™њЩ]В€B‚€YЉИШЬ›ЫZ[ћ
B€В€ИHШЬ›ЫZ[ћВ€B€[ЩHYЉИ€ШЬ›ЫX^
B€В€ИHШЬ›ЫX^В€B‚€YЉ
]™[OњШЬ›Ы\€	€РФ“УРђPТКH	‰€]™[OќЪYHљY[Ы[Щ\Лљ™\ИHИ›ШЪШYJB€В€ИH]™[OќЪYHљY[Ы[Щ\Лљ™\ИH›ШЪШYNВ€B‚€YЉИY[Щ^
B€В€YЉИY[Щ^H]™[OњШЬ›ЫЬYY
B€В€ИHY[Щ^H]™[OњШЬ›ЫЬYYВ€B€Y[Щ^HОВ€B€[ЩHYЉ
]™[OњШЬ›Ы\€	€РФ“УРђPТКH	‰€И€Y[Щ^
B€В€YЉИ€Y[Щ^
И]™[OњШЬ›ЫЬYY
B€В€ИHY[Щ^
И]™[OњШЬ›ЫЬYYВ€B€Y[Щ^HОВ€B‚€YЉY[Щ^€]™[OќЪYHљY[Ы[Щ\Лљ™\КB€В€Y[Щ^H
›Ш]
[]™[OќЪYHљY[Ы[Щ\Лљ™\ОВ€B€YЉY[Щ^H
B€В€Y[Щ^HВ€YШZ[њЭ[™HNВ€B‚€YЉYШZ[њЭ[™
B€В€]™[OњЬККОВ€B€[ЩB€В€]™[OњЬИH
[ќ
J
]™[OќЪYHљY[Ы[Щ\Лљ™\КHHY[Щ^
NВ€B€B€[ЩHYЉ]™[OњШЬ›Ы\€	€РФ“УУХUРT‘
HЛИ€ШЬ›ЫЫ›B€В‚€YЉ›HHH€љY[Ы[Щ\Лќ”™\КB€В€ИHY[Щ^NВ€B€[ЩB€В€ИH
›H
ИJHИ€HљY[Ы[Щ\Лќ”™\ИИ€
И]™[OШ[Y\^›Щ™њЩ]В€B‚€YЉИШЬ›ЫZ[ћЉB€В€ИHШЬ›ЫZ[ћЋВ€B€[ЩHYЉИ€ШЬ›ЫX^ЉB€В€ИHШЬ›ЫX^ЋВ€B‚€YЉ
]™[OњШЬ›Ы\€	€РФ“УРђPТКH	‰€И›ШЪШYJB€В€ИH›ШЪШYNВ€B‚€YЉИ€Y[Щ^JB€В€YЉИ€Y[Щ^H
И]™[OњШЬ›ЫЬYY
B€В€ИHY[Щ^H
И]™[OњШЬ›ЫЬYYВ€B€Y[Щ^HHОВ€B€[ЩHYЉ
]™[OњШЬ›Ы\€	€РФ“УРђPТКH	‰€ИY[Щ^JB€В€YЉИY[Щ^HH]™[OњШЬ›ЫЬYY
B€В€ИHY[Щ^HH]™[OњШЬ›ЫЬYYВ€B€Y[Щ^HHОВ€B‚€YЉY[Щ^H€[™[ЪZYЪHљY[Ы[Щ\Лќ”™\КB€В€Y[Щ^HH
›Ш]
\[™[ЪZYЪHљY[Ы[Щ\Лќ”™\ОВ€YШZ[њЭ[™HNВ€B€YЉY[Щ^H
B€В€Y[Щ^HHВ€B‚€YЉYШZ[њЭ[™
B€В€]™[OњЬККОВ€B€[ЩB€В€]™[OњЬИH
[ќ
XY[Щ^NВ€B€B€[ЩHYЉ]™[OњШЬ›Ы\€	€РФ“УТS•РT‘
B€В€YЉ›HHH€љY[Ы[Щ\Лќ”™\КB€В€ИHY[Щ^NВ€B€[ЩB€В€ИH
›H
ИJHИ€HљY[Ы[Щ\Лќ”™\ИИ€
И]™[OШ[Y\^›Щ™њЩ]В€B‚€YЉИШЬ›ЫZ[ћЉB€В€ИHШЬ›ЫZ[ћЋВ€B€[ЩHYЉИ€ШЬ›ЫX^ЉB€В€ИHШЬ›ЫX^ЋВ€B‚€YЉ
]™[OњШЬ›Ы\€	€РФ“УРђPТКH	‰€[™[ЪZYЪHљY[Ы[Щ\Лќ”™\ИHИ›ШЪШYJB€В€ИH[™[ЪZYЪHљY[Ы[Щ\Лќ”™\ИH›ШЪШYNВ€B‚€YЉИY[Щ^JB€В€YЉИY[Щ^HH]™[OњШЬ›ЫЬYY
B€В€ИHY[Щ^HH]™[OњШЬ›ЫЬYYВ€B€Y[Щ^HHОВ€B€[ЩHYЉ
]™[OњШЬ›Ы\€	€РФ“УРђPТКH	‰€И€Y[Щ^JB€В€YЉИ€Y[Щ^H
И]™[OњШЬ›ЫЬYY
B€В€ИHY[Щ^H
И]™[OњШЬ›ЫЬYYВ€B€Y[Щ^HHОВ€B‚€YЉY[Щ^H€[™[ЪZYЪHљY[Ы[Щ\Лќ”™\КB€В€Y[Щ^HH
›Ш]
\[™[ЪZYЪHљY[Ы[Щ\Лќ”™\ОВ€B€YЉY[Щ^HH
B€В€Y[Щ^HHВ€YШZ[њЭ[™HNВ€B‚€YЉYШZ[њЭ[™
B€В€]™[OњЬККОВ€B€[ЩB€В€]™[OњЬИH
[ќ
J
[™[ЪZYЪHљY[Ы[Щ\Лќ”™\КHHY[Щ^JNВ€B€B€ЛЭ\ЭЫ‹[]]Ь€ЭYЩB€[ЩHYЉ]™[OњШЬ›Ы\€	€
РФ“УХTРФ“УСХУЉJB€В€ЛШY[Щ^H
ПHЌNВ€YЉШЬ›ЫYOHJB€В€ШЬ›ЫYHВ€Y[Щ^JКОВ€B€[ЩB€В€ШЬ›ЫY
КОВ€B€]™[OњЬИH
[ќ
XY[Щ^NВ€B€KЛЪYЉ[]™[OќШZ][™КB‚€ЛИ€]]Л\ШЬ›Ы€YЉ
]™[OњШЬ›Ы\€	€РФ“УУQ•
H
]™[OњШЬ›Ы\€	€РФ“УФ’QТ
JHЛИYYШЬ›Ы\H›ЭИЩZ\™[™ЬИШ[€\[‹ќ]Ы›HY€H[Щ\€\И^ћH[€\Ъ[™И›ШЪШY\ЛЫ€В‚€YЉШ[Y\]\HOHJB€В€›HHNNNNNВ€HHNNNNNNИЛЬ™XШ[Э[]B€›ЬЉHHИH]™[Щ]ЦШЭ\њ™[ќЬЩ]K›X^^Y\њОИJККB€В€YЉ^Y\–ЪWK™[ќ
B€В€YЉ^Y\–ЪWK™[ќOњЬЪ][Ы‹ћ€H^Y\–ЪWK™[ќOњЬЪ][Ы‹ћH€›JB€В€›HH^Y\–ЪWK™[ќOњЬЪ][Ы‹ћ€H^Y\–ЪWK™[ќOњЬЪ][Ы‹ћNВ€B€YЉ^Y\–ЪWK™[ќOњЬЪ][Ы‹ћ€H^Y\–ЪWK™[ќOњЬЪ][Ы‹ћHJB€В€HH^Y\–ЪWK™[ќOњЬЪ][Ы‹ћ€H^Y\–ЪWK™[ќOњЬЪ][Ы‹ћNВ€B€B€B€B€YЉ›HHH€љY[Ы[Щ\Лќ”™\КB€В€ИHY[Щ^NВ€B€[ЩB€В€ИH
›H
ИJHИ€HљY[Ы[Щ\Лќ”™\ИИ€
И]™[OШ[Y\^›Щ™њЩ]В€B‚€ЛИ™]ИШЬ›Ы[Z]€YЉИ€ШЬ›ЫX^ЉB€В€ИHШЬ›ЫX^ЋВ€B€[ЩHYЉИШЬ›ЫZ[ћЉB€В€ИHШЬ›ЫZ[ћЋВ€B‚€YЉИOHY[Щ^JB€В€YЉИ€Y[Щ^H
И]™[OњШЬ›ЫЬYY
B€В€ИHY[Щ^H
И]™[OњШЬ›ЫЬYYВ€B€[ЩHYЉИY[Щ^HH]™[OњШЬ›ЫЬYY
B€В€ИHY[Щ^HH]™[OњШЬ›ЫЬYYВ€B€Y[Щ^HH
›Ш]
]ОВ€B‚€YЉY[Щ^H€[™[ЪZYЪH
]™[Oњ›ШЪЪ[™ИИM€€LЉHHљY[Ы[Щ\Лќ”™\КB€В€Y[Щ^HH
›Ш]
J[™[ЪZYЪH
]™[Oњ›ШЪЪ[™ИИM€€LЉHHљY[Ы[Щ\Лќ”™\КNВ€B€YЉY[Щ^H
B€В€Y[Щ^HHВ€B€B€ЛИ›ЭИ]]ИШЬ›Ы€[ЩHYЉ
]™[OњШЬ›Ы\€	€РФ“УТS•РT‘
H
]™[OњШЬ›Ы\€	€РФ“УУХUРT‘
JB€В€YЉ›HHH€љY[Ы[Щ\Лљ™\КB€В€ИHY[Щ^В€B€[ЩB€В€ИH
H
И›JHИ€HљY[Ы[Щ\Лљ™\ИИ€
И]™[OШ[Y\^Щ™њЩ]В€B‚€ЛИ™]ИШЬ›Ы[Z]€YЉИ€ШЬ›ЫX^
B€В€ИHШЬ›ЫX^В€B€[ЩHYЉИШЬ›ЫZ[ћ
B€В€ИHШЬ›ЫZ[ћВ€B‚€YЉИOHY[Щ^
B€В€YЉИ€Y[Щ^
И]™[OњШЬ›ЫЬYY
B€В€ИHY[Щ^
И]™[OњШЬ›ЫЬYYВ€B€[ЩHYЉИY[Щ^H]™[OњШЬ›ЫЬYY
B€В€ИHY[Щ^H]™[OњШЬ›ЫЬYYВ€B€Y[Щ^H
›Ш]
]ОВ€B‚€YЉY[Щ^€]™[OќЪYHљY[Ы[Щ\Лљ™\КB€В€Y[Щ^H
›Ш]
J]™[OќЪYHљY[Ы[Щ\Лљ™\КNВ€B€YЉY[Щ^
B€В€Y[Щ^HВ€B€B€ЛЩ[™Щ€€]]Л\ШЬ›Ы€ЛИЫШ[[YH›Ь€\WЬ[™[€ШЬ›ЫHY[Щ^HВ€ШЬ›ЫHHY[Щ^HHNВџB‚‚ќ›ЪY\]WЬШЬ›ЫYШ™К
BћВ€›Ш]›ШЪЭ]™[В€[њЪYЫ™YЪ\€™[ЫњММ—NЛЛМКЋ€Э]XИ[ќ›ШЪЬЬИHВ€Э]XИ[ќ›ШЪЫЩ™њЬЪ[™VММ—HB€В€‹‹ЛK‹ЛЛ€KKKK€ЛЛ‹KЛ‹‹€KKKB€NИЛИ›Ь›X[›ШЪВ€Э]XИ[ќ›ШЪЫЩ™њЬЪZЩVММ—HB€В€‹‹‹‹‹‹‹‹€‹‹‹‹€‹‹‹‹‹‹‹‹€‹‹‹‹€NИЛИЫЭЛЫЫњЭ[ќ\њљ[™И›ШЪЛZЩHЫ€HZ[‚€Э]XИ[ќ›ШЪЫЩ™њЬќ[X›VММ—HB€В€‹‹ЛЛ‹‹ЛЛ€‹‹ЛЛ‹Л‹Л€‹‹ЛЛ‹‹ЛЛ€‹‹ЛЛ‹Л‹Л€NИЛИ\ЭЫЫњЭ[ќќ[X›[™ЛZЩH[‹ЫЫ€H[€Ь€Z[\‚€[ќ€H^[ћ]\ЦК[ќ
TVSММ—NВ‚€ЛИ[YHИ\]H™[Ы€[™ШЬ™Y[€[›YИ[ЩOВ€YЉЭ[YHЏH™[Ы—Э[YH	‰€Yњ™Y^™X[
B€В€Y[XЬJ™[Ыњ™[ЫќX›H
ИLЋ
€‹
€ЉNВ€Y[XЬJ™[ЫќX›H
ИLЋ
€‹™[Ыњ
И€
€‹€
€ЉNВ€Y[XЬJ™[ЫќX›H
И
LЋ
ИЉJњ‹™[Ыњ€
€ЉNВ‚€™[Ы—Э[YHHЭ[YH
И
ЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYYИКNВ€B‚€YЉYњ™Y^™X[
B€В€›ШЪЭ]™[H
]™[Oњ›ШЪЪ[™КHИ

Э[YHH]™[[YJHИ

›Ш]
YЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYYИМ
JH€ИЛИ›ИZЩH[€™X[Y™KX^X™B€YЉ]™[O™ЬЬYY
B€В€›ШЪЭ]™[H\›ШЪЭ]™[В€B€™Э]™[Y
ПH
Э[YHH]™[[YJH
€]™[O™ЬЬYYИМ
€
И›ШЪЭ]™[В€™Э]™[Y
ПH
Э[YHH]™[[YJH
€]™[Oќ™ЬЬYYИМ
€В€B€[ЩB€В€^[YH
ПHЭ[YHH]™[[YNВ€B‚€[Y]\€HЭ[YHH^[YNВ‚€YЉ]™[Oњ›ШЪЪ[™КB€В€›ШЪЬЬИH
[Y]\€И
ЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYYИ
JH	€МNВ€YЉ]™[Oњ›ШЪЪ[™ИOHJB€В€ЩћЮWЫЩ™њЩ]H]™[Oњ]XZЩHHH›ШЪЫЩ™њЬЪ[™VЬ›ШЪЬЬЧNВ€B€[ЩHYЉ]™[Oњ›ШЪЪ[™ИOHЉB€В€ЩћЮWЫЩ™њЩ]H]™[Oњ]XZЩHHH›ШЪЫЩ™њЬЪZЩVЬ›ШЪЬЬЧNВ€B€[ЩHYЉ]™[Oњ›ШЪЪ[™ИOHКB€В€ЩћЮWЫЩ™њЩ]H]™[Oњ]XZЩHHH›ШЪЫЩ™њЬќ[X›VЬ›ШЪЬЬЧNВ€B€B€[ЩB€В€YЉ]™[Oњ]XZЩHЏH
B€В€ЩћЮWЫЩ™њЩ]H]™[Oњ]XZЩHHВ€B€[ЩB€В€ЩћЮWЫЩ™њЩ]H]™[Oњ]XZЩH
ИВ€B€B‚€ЛЪYЉ]™[OњШЬ›Ы\€OTРФ“УХT	‰€]™[OњШЬ›Ы\€OTРФ“УСХУЉHЩћЮWЫЩ™њЩ]OHY[Щ^NВ€ЩћЮWЫЩ™њЩ]
ПHЩћЮWЫЩ™њЩ]ШYЋИЛМЊLWММЛО€\H[Щ\€Yќ\ЭY[ќ‚‚€]™[[YHHЭ[YNВ‚€YЉЭ[YHЏH]™[Oњ]XZЩ][YJB€В€]™[Oњ]XZЩHПHЋВ€]™[Oњ]XZЩ][YHHЭ[YH
И
ЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYYИЌJNВ€BџB‚ќ›ЪY]ЧЬШЬ›ЫYШ™К
BћВ€[ќ[™^H‹HH‹NВ€ЧЫ^Y\€
›^Y\ЋВ€[ќЪYZYЪВ‚€[ќњњKњЛњВ‚€YЉљY]ЬЬќИ€
B€В€њHљY]ЬЬќВ€њHHљY]ЬЬќNВ€њИHљY]ЬЬќОВ€њHљY]ЬЬќВ€B€[ЩB€В€њHњHHВ€њИHљY[Ы[Щ\Лљ™\ОВ€њHљY[Ы[Щ\Лќ”™\ОВ€B‚€ЛЪYЉ]™[
Hљ[ќЉ‰Y	Y	Y	Y€‹њњKњЛњ
NВ‚€ЧЩ]ЫY]ЩШЬ™Y[›Y]ЩHZ[›Y]ЩВ‚€КњЧЩ]ЫY]Щ
њШЬ™Y[›Y]ЩH	њШЬ™Y[›Y]ЩВ€›ЬЉHHИH]™[O›ќ[ZЫ\ОИJККB€В€Ьљ]\WШYЬЬљ]J
[ќ
J]™[OљЫ\ЦЪWKћHШЬ™Y[ћ
ИЩћЮЫЩ™њЩ]
K
[ќ
J]™[OљЫ\ЦЪWKћ€H]™[OљЫ\ЦЪWK™\HШЬ™Y[ћH
ИЩћЮWЫЩ™њЩ]
KУWЦ‹Ы\Ьљ]KШЬ™Y[›Y]Щ
NВ€J‹В‚€›ЬЉ[™^HИ[™^]™[O›ќ[[^Y\њЬ™YЋИ[™^
ККB€В€^Y\€H]™[O›^Y\њЬ™Y€
И[™^В‚€ШЬ™Y[›Y]ЩH^Y\‹O™]ЫY]ЩВ‚€ЛЬљ[ќЉ›^Y\€	Y[™N‰]KЋ‰Y€‹[™^^Y\‹O™Щћљ[™K^Y\‹OњЬЪ][Ы‹ћЉNВ‚‚BKЛИ^Y\€]\Э™H[X›Y[™]™H]X\ЭЫ™H[њЭXЩKЬ€ЩHЫ‰Э]И]‚€YЉ\ШЬ™Y[›Y]Щћ™\X]\ШЬ™Y[›Y]Щћ\™\X][^Y\‹O™[X›Y
B€В€ЫЫќ[ќYNВ€B‚€ЪYHШЬ™Y[›Y]ЩћЬ[€H^Y\‹OњЪ^™Kћ
И^Y\‹OњЬXЪ[™ЛћВ€ZYЪHШЬ™Y[›Y]Щћ\Ь[€H^Y\‹OњЪ^™KћH
И^Y\‹OњЬXЪ[™ЛћЋВ‚€H
[ќ
J^Y\‹O›Щ™њЩ]ћH
Y[Щ^
И™Э]™[Y
€^Y\‹O™ЬЬYY][КH
€
KЊH^Y\‹Oњ][Лћ
H
HВ‚€ЛЬљ[ќЉ›^Y\ћ][И	Y€	Y	Y—€‹^Y\‹Oћ][Л^Y\‹O™ЬЬYY][КNВ‚€YЉ
]™[OњШЬ›Ы\€	€РФ“УХT
JB€В€ЛЮ€H
[ќ
J^Y\‹O›Щ™њЩ]ћ€
ИY[Щ^H
€
KЊH^Y\‹Oњ][ЛћЉH
HВ€€H
[ќ
J^Y\‹O›Щ™њЩ]ћ€H
Y[Щ^H
И™Э]™[Y
€^Y\‹O™ЬЬYY][КH
€
KЊH^Y\‹Oњ][ЛћЉH
HВ€B€[ЩB€В€ЛЮ€H
[ќ
J^Y\‹O›Щ™њЩ]ћ€HY[Щ^H
€
KЊH^Y\‹Oњ][ЛћЉH
HВ€€H
[ќ
J^Y\‹O›Щ™њЩ]ћ€H
Y[Щ^H
И™Э]™[Y
€^Y\‹O™ЬЬYY][КH
€
KЊH^Y\‹Oњ][ЛћЉH
HВ€B‚€YЉ^Y\‹Oњ]XZЩJB€В€
ПHЩћЮЫЩ™њЩ]В€€
ПHЩћЮWЫЩ™њЩ]В€ЛЬљ[ќЉ‰YH	Y	Y€‹[™^ЩћЮWЫЩ™њЩ]ЉNВ€B‚€OHњВ€€OHњNВ‚‚€YЉ
B€В€HH
^
HИЪYВ€	OHЪYВ€B€[ЩB€В€HHВ€B‚€YЉH€	‰€ШЬ™Y[›Y]ЩќШ]\‹ќШ]\›[ЩHOHРUT—УSСWФТPT€	‰€ШЬ™Y[›Y]ЩќШ]\‹[\]YJB€В€KKNВ€OHЪYВ€B‚€YЉ€
B€В€€H
^ЉHИZYЪВ€€	OHZYЪВ€B€[ЩB€В€€HВ€B€‚BZYЉ^Y\‹O›™[ЫЉB€В€ШЬ™Y[›Y]ЩќX›HH™[ЫќX›NВ€B€[ЩB€В€YЉЭ\њ™[ќЬ[]H€
B€В€ШЬ™Y[›Y]ЩќX›HH]™[Oњ[]\ЦШЭ\њ™[ќЬ[]HHWNВ€B€[ЩB€В€ШЬ™Y[›Y]ЩќX›HH•SВ€B€B‚€ШЬ™Y[›Y]ЩќШ]\‹ќШ]™][YHH
[ќ
J[Y]\€
€ШЬ™Y[›Y]ЩќШ]\‹ќШ]™\ЬYY
NВ€ШЬ™Y[›Y]Щћ™\X]HШЬ™Y[›Y]Щћ\™\X]HВ€›ЬЉHHЋИ€^Y\‹O™]ЫY]Щћ\™\X]	‰€HњИH
ПHZYЪЉКЛШЬ™Y[›Y]Щћ\™\X]
ККNВ€›ЬЉHИH^Y\‹O™]ЫY]Щћ™\X]	‰€њИ
И
ШЬ™Y[›Y]ЩќШ]\‹ќШ]\›[ЩHOHРUT—УSСWФТPT€И€ШЬ™Y[›Y]ЩќШ]\‹[\]YH
€ЉNИ
ПHЪYJКЛШЬ™Y[›Y]Щћ™\X]
ККNВ‚€YЉ^Y\‹O™ЩћњШЬ™Y[‹O›XYЪXИOHШЬ™Y[—ЫXYЪXКB€В€Ьљ]\WШYЬШЬ™Y[Љ
Ињ€
ИњK^Y\‹Oћ‹^Y\‹O™ЩћњШЬ™Y[‹	њШЬ™Y[›Y]Щ[™^
NВ€B€[ЩHYЉ^Y\‹O™ЩћњЬљ]KO›XYЪXИOHЬљ]WЫXYЪXКB€В€Ьљ]\WШYЩњ[YJ
Ињ€
ИњK^Y\‹Oћ‹^Y\‹O™ЩћњЬљ]K	њШЬ™Y[›Y]Щ[™^
NВ€B‚€ЛЬљ[ќЉЉЉЉЉЉЉ‰Y	Y	Y	Y	Y
ЉЉЉЉ—€‹
ЭњЉЭњK^Y\‹O‹ћ‹ШЬ™Y[›Y]Щћ™\X]ШЬ™Y[›Y]Щћ\™\X]
NВ€B‚‚џB‚ќLМ€Щ][ќ\ќ[

BћВ€[ќ\ќ[H[Y\—ЩЩ][ќ\ќ[
ЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYY
NИЛИЫИ[ќ\ќ[Ш[€™HЩЩЩY[ќИ[ЭљYB€YЉ[ќ\ќ[€ЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYY
B€В€[ќ\ќ[HNВ€B€YЉ[ќ\ќ[€ЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYYИ
B€В€[ќ\ќ[HЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYYИВ€B€™]\›€[ќ\ќ[ВџB‚‹ЛИШ\ЪЩ^K[[Ы€‹‚‹ЛИЊNKLLЊ‚‹ЛИ‹ЛИќ[€[њ]ШЬљ\Л€Ъ[Z[\€ИЩ^\Лќ]‹ЛИ^XЭ]H™Y›Ь™H›ШЩ\ЬЪ[™И[ћHЫЫ[X[™ќ[Э[ЫњЛ‚ќ›ЪY^XЭ]WЪ[њ]ЬШЬљ\К[ќ^Y\—Ъ[™^
BћВ‚\ЧЬ^Y\€
њ^Y\—ЫШљ€H•SВ‚BB‚\^Y\—ЫШљ€H^Y\€
И^Y\—Ъ[™^В‚‚ZY€
\^Y\—ЫШљЉB‚^В‚B\™]\›ЋВ‚_B‚B‚ZY€
^Y\—ЫШљ‹O›™]ЪЩ^\И
Щ^\ШЬљ\]H	‰€^Y\‹OљЩ^\КH^Y\‹Oњ™[X\ЩZЩ^\КB‚^В‚BKЛИЊNKLLЊ€Ы‰Э^\ЭY]‚BKЛЪY€
]™[
B‚BKЛЮВ‚‚BBKЛЩ^XЭ]WЫ]™[ЪЩ^WЬШЬљ\
^Y\—Ъ[™^^Y\ЉNВ‚BBKЛЩ^XЭ]WЩ[ќ]WЪЩ^WЬШЬљ\
^Y\‹™[ќ
NВ‚BKЛЯB‚BKЛЩ^XЭ]WЪЩ^WЬШЬљ\
^Y\—Ъ[™^^Y\ЉNВ‚‚BY^XЭ]WЪ[њ]ЬШЬљ\Ш[
^Y\—Ъ[™^
NВ‚_BџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊЌ‹LЛLMВЉ‚Љ€Y\™XЭ[Ы‹\™[]]™HЫЫ[X[™›YЬИИH\ЪXШ[Љ€[њ]X\ЪЛ‚Љ‚Љ€Yќ[™љYЪ™[XZ[€[€H™\Э[ЫИ\™XЫЩYЉ€ЫЫ[X[™ИШ[€ЫЫќ[ќYHИ[њЬXЭH\ЪXШ[[њ]‚Љ‹ВњЭ]XИЩ^WЫX\ЪЧЭЫЫ[X[™Ъ[њ]Ь™\ЫЫ™WЩ\™XЭ[ЫЉ€ЫЫњЭЩ^WЫX\ЪЧЭ[њ]Щ›YЬЛ€ЫЫњЭ[ќ]J€XЭ[™ЧЩ[ќ]BЉHВ€Щ^WЫX\ЪЧЭ™\ЫЫ™YЩ›YЬИH[њ]Щ›YЬОВ‚€YЉXXЭ[™ЧЩ[ќ]JHВ€™]\›€™\ЫЫ™YЩ›YЬОВ€B‚€YЉ[њ]Щ›YЬИ	€“QЧУSХ‘SQ•
HВ€™\ЫЫ™YЩ›YЬИHXЭ[™ЧЩ[ќ]KO™\™XЭ[Ы‚€И“QЧРђPТХРT‘€€“QЧС“Ф•РT‘В‚€H[ЩHYЉ[њ]Щ›YЬИ	€“QЧУSХ‘T’QТ
HВ€™\ЫЫ™YЩ›YЬИHXЭ[™ЧЩ[ќ]KO™\™XЭ[Ы‚€И“QЧС“Ф•РT‘€€“QЧРђPТХРT‘В€B‚€™]\›€™\ЫЫ™YЩ›YЬОВџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊЌ‹LЛLMИHЬ™Ъ[[ћH][›™[Л[љЫ›ЭИ]K‚Љ‚Љ€\]HH[њ]Э]H›Ь€XXЪ^Y\‹€\И[ЫY\ВЉ€\ЪXШ[Щ^\ЛЫЫ[X[™[њ]\ЭЬћK[™]]ЫX]XВЉ€[YЩ\Л€™YXЭЬ™YИЭ\Ьќ\™XЭ[Ы‹\™[]]™HЉ€ЫЫ[X[™›YЬИ[™^[њЪX›HЫЫ[X[™Ю\Э[K‚Љ‹Вќ›ЪY[њ]™Yњ™\Ъ
[ќ^\™XЫ[ЩJHВ‚€[ќВ€ЧЬ^Y\€
XЭ[™ЧЬ^Y\ЋВ‚€ЫЫќ›ЫЭ\]J^Y\ЫЫќ›ЫЪ[ќ\њЛPVФVQT”КNВ‚€›ЭЩ^\ИHВ€›Э™]ЪЩ^\ИHВ‚€›ЬЉHИPVФVQT”ОИ
ККB€В€XЭ[™ЧЬ^Y\€H^Y\€
ИВ‚€Y€
^\™XЫ[ЩHOHWФ‘PЧФVJHВ‚€XЭ[™ЧЬ^Y\‹Oњ™[X\ЩZЩ^\ИH
^Y\ЫЫќ›ЫЪ[ќ\њЦЬKOљЩ^Y›YЬИXЭ[™ЧЬ^Y\‹OљЩ^\КHH^Y\ЫЫќ›ЫЪ[ќ\њЦЬKOљЩ^Y›YЬОВ€XЭ[™ЧЬ^Y\‹Oњ™[X\ЩZЩ^\И	ЏHXЭ[™ЧЬ^Y\‹O™\ШX›ZЩ^\ОВ€XЭ[™ЧЬ^Y\‹OљЩ^\ИH^Y\ЫЫќ›ЫЪ[ќ\њЦЬKOљЩ^Y›YЬИ	€XЭ[™ЧЬ^Y\‹O™\ШX›ZЩ^\ОВ€XЭ[™ЧЬ^Y\‹O›™]ЪЩ^\ИH^Y\ЫЫќ›ЫЪ[ќ\њЦЬKO›™]ЪЩ^Y›YЬИ	€XЭ[™ЧЬ^Y\‹O™\ШX›ZЩ^\ОВ€XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\ИHXЭ[™ЧЬ^Y\‹O›™]ЪЩ^\ОВ€XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	ЏHXЭ[™ЧЬ^Y\‹OљЩ^\ОВ€XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	ЏHXЭ[™ЧЬ^Y\‹O™\ШX›ZЩ^\ОВ€€H[ЩHВ€ЛИ[€^H[ЩN€Y™\ЬЩYЩ^\ИИ™XИЩ^\В€XЭ[™ЧЬ^Y\‹Oњ™[X\ЩZЩ^\ИH
^Y\ЫЫќ›ЫЪ[ќ\њЦЬKOљЩ^Y›YЬИXЭ[™ЧЬ^Y\‹Oњ™]љЩ^\КHH^Y\ЫЫќ›ЫЪ[ќ\њЦЬKOљЩ^Y›YЬОВ€XЭ[™ЧЬ^Y\‹Oњ™[X\ЩZЩ^\И	ЏHXЭ[™ЧЬ^Y\‹O™\ШX›ZЩ^\ОВ€XЭ[™ЧЬ^Y\‹OљЩ^\ИH^Y\ЫЫќ›ЫЪ[ќ\њЦЬKOљЩ^Y›YЬИ	€XЭ[™ЧЬ^Y\‹O™\ШX›ZЩ^\ОВ€XЭ[™ЧЬ^Y\‹O›™]ЪЩ^\ИH^Y\ЫЫќ›ЫЪ[ќ\њЦЬKO›™]ЪЩ^Y›YЬИ	€XЭ[™ЧЬ^Y\‹O™\ШX›ZЩ^\ОВ€XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\ИHXЭ[™ЧЬ^Y\‹O›™]ЪЩ^\ОВ€XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	ЏHXЭ[™ЧЬ^Y\‹OљЩ^\ОВ€XЭ[™ЧЬ^Y\‹Oњ^ZЩ^\И	ЏHXЭ[™ЧЬ^Y\‹O™\ШX›ZЩ^\ОВ€B‚BBBB‚BY^XЭ]WЪ[њ]ЬШЬљ\К
NВBB‚€К‚€
€™\Щ]ЫЫ[X[™Щ\]Y[ЩHY€H\ЭЫЫ[X[™Ш\В€
€ЫИЫ™ИYЫЛ‚€
‹В€YЉXЭ[™ЧЬ^Y\‹O™[ќ	‰€XЭ[™ЧЬ^Y\‹O™[ќOЫЫ[X[™Э[YHЭ[YJHВ€Y[\Щ]
€XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]Ъ\ЭЬћK€€Ъ^™[ЩЉXЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]Ъ\ЭЬћJB€
NВ‚€XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]ШЫЭ[ќHВ€XЭ[™ЧЬ^Y\‹OЫЫ[X[™Ъ[њ]Ъ[™^HВ€B‚€К‚€
€ќZ[Ы™H]™[ќ]™\ћH™Yњ™\ЪЫИ]]ЫX]XИ[€
€™\ЪЫИШ[€Ь™X]H[€YЩHЪ]Э]H\ЪXШ[€
€ќ]Ы€Ъ[™ЩK€ЭЬ™H]Ы›HЪ[€]X\ЭЫ™B€
€™\ЬЛ™[X\ЩKЬ€]]ЫX]XИ[YЩH^\ЭЛ‚€
‹В€В€ЧШЫЫ[X[™Ъ[њ]Щ]™[ќ[њ]Щ]™[ќHМNВ‚€[њ]Щ]™[ќњ™\ЬИHЫЫ[X[™Ъ[њ]Ь™\ЫЫ™WЩ\™XЭ[ЫЉ€XЭ[™ЧЬ^Y\‹O›™]ЪЩ^\Л€XЭ[™ЧЬ^Y\‹O™[ќ€
NВ‚€[њ]Щ]™[ќњ™[X\ЩHHЫЫ[X[™Ъ[њ]Ь™\ЫЫ™WЩ\™XЭ[ЫЉ€XЭ[™ЧЬ^Y\‹Oњ™[X\ЩZЩ^\Л€XЭ[™ЧЬ^Y\‹O™[ќ€
NВ‚€К‚€
€™[X\ЩYЩ^\ИЩ\™H[[[YYX][H™Y›Ь™H\В€
€YЩK€[ЫYH[H[€H]™[ќЫ\ЪЭЫИB€
€ЫЫ[X[™ЭXЪ\ИVНLH
ИHШ[€\Э›ЭXЭЛ‚€
‹В€[њ]Щ]™[ќљ[HЫЫ[X[™Ъ[њ]Ь™\ЫЫ™WЩ\™XЭ[ЫЉ€XЭ[™ЧЬ^Y\‹OљЩ^\В€XЭ[™ЧЬ^Y\‹Oњ™[X\ЩZЩ^\Л€XЭ[™ЧЬ^Y\‹O™[ќ€
NВ‚€[њ]Щ]™[ќќ[YHHЭ[YNВ‚€ЫЫ[X[™Ъ[њ]ЪЫЬЭ\ќЭ\]J€XЭ[™ЧЬ^Y\‹€[њ]Щ]™[ќњ™\ЬЛ€[њ]Щ]™[ќќ[YB€
NВ‚€YЉ[њ]Щ]™[ќњ™\ЬКHВ€К‚€
€Ш\\™H[њ]ИЭ[[]\ИЬЪ]]™B€
€YЩK€HЫЫ™љYЭ\X›HЫЫ[X[™X]Ъ\‚€
€\Y\ИXXЪЭ\	ЬИЪЬ™Э[YHИZ\‚€
€™XЫЬ™Y™\ЬИ[Y\Л€\™XЫЩYЫЫ[X[™В€
€ЫЫќ[ќYHИ[њЬXЭ[њ]Щ]™[ќњ™\ЬИЫ›K‚€
‹В€[њ]Щ]™[ќњ™\ЬЧШЪЬ™B€ЫЫ[X[™Ъ[њ]Ь™\ЫЫ™WЩ\™XЭ[ЫЉ€XЭ[™ЧЬ^Y\‹OљЩ^\Л€XЭ[™ЧЬ^Y\‹O™[ќ€
NВ€B‚€[њ]Щ]™[ќљЫB€ЫЫ[X[™Ъ[њ]ЪЫЭљYЩЩ\—ШЫЫXЭ
€XЭ[™ЧЬ^Y\‹€[њ]Щ]™[ќљ[€[њ]Щ]™[ќќ[YB€
NВ‚€YЉ[њ]Щ]™[ќњ™\ЬВ€[њ]Щ]™[ќљЫ€[њ]Щ]™[ќњ™[X\ЩJHВ‚€[њ]Щ]™[ќќXЪЬИH[Y\—ЩЩ]XЪК
NВ€ЫЫ[X[™Ъ[њ]Ъ\ЭЬћWЬ\Ъ
XЭ[™ЧЬ^Y\‹	љ[њ]Щ]™[ќ
NВ‚€YЉXЭ[™ЧЬ^Y\‹O™[ќ
HВ€XЭ[™ЧЬ^Y\‹O™[ќOЫЫ[X[™Э[YHB€ЫЫ[X[™Ъ[њ]Ъ\ЭЬћWЩ^\][Ы—Э[YWЩЩ]
€XЭ[™ЧЬ^Y\‹O™[ќ€[њ]Щ]™[ќќ[YB€
NВ€B€B‚€B‚€›ЭЩ^\ИHXЭ[™ЧЬ^Y\‹OљЩ^\ОВ€›Э™]ЪЩ^\ИHXЭ[™ЧЬ^Y\‹O›™]ЪЩ^\ОВ€€B‚џB‚ќ›ЪY^XЭ]WЪЩ^\ШЬљ\К
BћВ€[ќВ€›ЬЉHИ]™[Щ]ЦШЭ\њ™[ќЬЩ]K›X^^Y\њОИ
ККB€В€YЉWЬ]\ЩH	‰€
]™[ЪXЪЧЪ[—ЬШЬ™Y[Љ
JH	‰€
^Y\–ЬK›™]ЪЩ^\И
Щ^\ШЬљ\]H	‰€^Y\–ЬKљЩ^\КH^Y\–ЬKњ™[X\ЩZЩ^\КJB€В€YЉ]™[
B€В€^XЭ]WЫ]™[ЪЩ^WЬШЬљ\

NВ€^XЭ]WЩ[ќ]WЪЩ^WЬШЬљ\
^Y\–ЬK™[ќ
NВ€B€^XЭ]WЪЩ^WЬШЬљ\

NВ€^XЭ]WЪЩ^WЬШЬљ\Ш[

NВ€B€BџB‚ќ›ЪY^XЭ]WЭ\]\ШЬљ\К
BћВ€YЉШЬљ\Т\Т[љ]X[^™Y
	ќ\]WЬШЬљ\
JB€В€ШЬљ\С^XЭ]J	Љ\]WЬШЬљ\
JNВ€B€YЉ]™[	‰€ШЬљ\Т\Т[љ]X[^™Y
	Љ]™[Oќ\]WЬШЬљ\
JJB€В€ШЬљ\С^XЭ]J	Љ]™[Oќ\]WЬШЬљ\
JNВ€BџB‚ќ›ЪY^XЭ]WЭ\]YШЬљ\К
BћВ€YЉШЬљ\Т\Т[љ]X[^™Y
	ќ\]YЬШЬљ\
JB€В€ШЬљ\С^XЭ]J	Љ\]YЬШЬљ\
JNВ€B€YЉ]™[	‰€ШЬљ\Т\Т[љ]X[^™Y
	Љ]™[Oќ\]YЬШЬљ\
JJB€В€ШЬљ\С^XЭ]J	Љ]™[Oќ\]YЬШЬљ\
JNВ€BџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊЌ‹LLЊBЉ‚Љ€^XЭ]HЫШ[[™]™[ШЬљ\И[[YYX][H™Y›Ь™H[‚Љ€[™Ъ[™HЩЪXШ[ЫШЪИXЪИ\И›ШЩ\ЬЩY‚Љ‹Вќ›ЪY^XЭ]WЭ\][ЩЪXЬШЬљ\К
BћВ€YЉШЬљ\Т\Т[љ]X[^™Y
	ќ\]WЫЩЪXЧЬШЬљ\
JB€В€ШЬљ\С^XЭ]J	ќ\]WЫЩЪXЧЬШЬљ\
NВ€B€YЉ]™[	‰€ШЬљ\Т\Т[љ]X[^™Y
	›]™[Oќ\]WЫЩЪXЧЬШЬљ\
JB€В€ШЬљ\С^XЭ]J	›]™[Oќ\]WЫЩЪXЧЬШЬљ\
NВ€BџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊЌ‹LLЊBЉ‚Љ€^XЭ]HЫШ[[™]™[ШЬљ\ИYќ\€[€[™Ъ[™HЩЪXШ[Љ€ЫШЪИXЪИ\И›ШЩ\ЬЩY™Y›Ь™H]ИЫШЪИ[YHY[Щ\Л‚Љ‹Вќ›ЪY^XЭ]WЭ\]YЩЪXЬШЬљ\К
BћВ€YЉШЬљ\Т\Т[љ]X[^™Y
	ќ\]YЫЩЪXЧЬШЬљ\
JB€В€ШЬљ\С^XЭ]J	ќ\]YЫЩЪXЧЬШЬљ\
NВ€B€YЉ]™[	‰€ШЬљ\Т\Т[љ]X[^™Y
	›]™[Oќ\]YЫЩЪXЧЬШЬљ\
JB€В€ШЬљ\С^XЭ]J	›]™[Oќ\]YЫЩЪXЧЬШЬљ\
NВ€BџB‚ќ›ЪY]ЧЭ^ШљњК
BћВ€[ќNВ€ЧЭ^Шљ€
ќ^ШљЋВ€YЉ[]™[
B€В€™]\›ЋВ€B€›ЬЉHHИH]™[O›ќ[]^ШљњИИJККB€В€^Шљ€H]™[Oќ^ШљњИ
ИNВ‚€YЉ^Шљ‹Oќ[YH	‰€^Шљ‹Oќ[YHHЭ[YJBBKЛТY€H[YHШ\ИЩ][™\ЬЩY™[[Э™HH^Шљ™XЭ‚€В€]™[Oќ^ШљњЦЪWKќ[YBOHВ€]™[Oќ^ШљњЦЪWKњЬЪ][Ы‹ћHВ€]™[Oќ^ШљњЦЪWKњЬЪ][Ы‹ћHHВ€]™[Oќ^ШљњЦЪWK™›ЫќHВ€]™[Oќ^ШљњЦЪWKњЬЪ][Ы‹ћ€HВ€YЉ]™[Oќ^ШљњЦЪWKќ^
B€В€њ™YJ]™[Oќ^ШљњЦЪWKќ^
NВ€]™[Oќ^ШљњЦЪWKќ^H•SВ€B€B€[ЩB€В€YЉ^Шљ‹Oќ^
B€В€›ЫќЬљ[ќЫ[™Э
€^Шљ‹OњЬЪ][Ы‹ћ€^Шљ‹OњЬЪ][Ы‹ћK€^Шљ‹O™›Ыќ€^Шљ‹OњЬЪ][Ы‹ћ‹€^Шљ‹Oќ^€Э›[Љ^Шљ‹Oќ^
B€
NВ€B€B€BџB‚љ[ќ™XЫЬ™[њ]К
BћВ€[ќHВ€™XТЩ^\И™XЪЩ^NВ€Z[ќЌЭЪ[™ЭИHMЋВ€LМ€X^Ь™XЧЭ[YHHЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYY
ЌЊ
ЊLИЛИ›ЭXЭ[Ы‚‚€YЉ^\™XЬЭ]\ЛOњЭ]\ИOHWФ‘PЧФ‘PКH™]\›€В€Y€
\^\™XЬЭ]\ЛO™YЪ[€
B€В€^\™XЬЭ]\ЛOњЭ\ќ[YHHЭ[YNВ€^\™XЬЭ]\ЛOњЮ[Э[YHHВ€Y€
^\™XЬЭ]\ЛOќY™™\ЉB€В€њ™YJ^\™XЬЭ]\ЛOќY™™\ЉNВ€^\™XЬЭ]\ЛOќY™™\€H•SВ€B€^\™XЬЭ]\ЛOќY™™\€H
™XТЩ^\КЉXШ[ШКЪ[™ЭКЬ^\™XЬЭ]\ЛOњЮ[Э[YJњЪ^™[ЩЉ™XТЩ^\КKЪ^™[ЩЉ™XТЩ^\КJNВ€Y€
^\™XЬЭ]\ЛOќY™™\€OH•S
B€В€љ[ќЉ‘\њ›Ь€И[ШШ]HќY™™\€[€™XЫЬ™[њ]И[ЩK—€ЉNВ€™]\›€В€B€^\™XЬЭ]\ЛO™YЪ[€HNВ€^\™XЬЭ]\ЛOњЩYYHЩ]ЩYY

NВ€^\™XЬЭ]\ЛOЬЩYYHЭ[YNВ€Ь[™
Э[YJNВ€^\™XЬЭ]\ЛOќXЪЬИH[Y\—ЩЩ]XЪК
NВ€B€[ЩB€В€Y€
^\™XЬЭ]\ЛOњЮ[Э[YI]Ъ[™ЭИЏHЪ[™ЭЛL€
HЛИ\Э\И•Sћ]\В€В€^\™XЬЭ]\ЛOќY™™\€H
™XТЩ^\КЉ\™X[ШК^\™XЬЭ]\ЛOќY™™\‹Ъ^™[ЩЉ™XТЩ^\КJЉ
^\™XЬЭ]\ЛOњЮ[Э[YKЭЪ[™ЭКМJJќЪ[™ЭКЭЪ[™ЭКJNВ€Y€
^\™XЬЭ]\ЛOќY™™\€OH•S
B€В€љ[ќЉ‘\њ›Ь€И[ШШ]HќY™™\€[€™XЫЬ™[њ]И[ЩK—€ЉNВ€™]\›€В€H[ЩHY[\Щ]
^\™XЬЭ]\ЛOќY™™\ЉК^\™XЬЭ]\ЛOњЮ[Э[YJМJK
^\™XЬЭ]\ЛOњЮ[Э[YKЭЪ[™ЭКМJJќЪ[™ЭКЭЪ[™ЭЛJ^\™XЬЭ]\ЛOњЮ[Э[YJМJKLJNИЛИL€™XШ]\ЩHLH\ИИИЪ^™KLB€B€B‚€ЛИ›ЭИ™XИB€YЉ^\™XЬЭ]\ЛOќY™™\€	‰€^\™XЬЭ]\ЛO™YЪ[ЉB€В€›Ь€
HИPVФVQT”ОИ
КИ
B€В€™XЪЩ^KљЩ^\ЦЬHH^Y\–ЬKљЩ^\ОВ€™XЪЩ^K›™]ЪЩ^\ЦЬHH^Y\–ЬK›™]ЪЩ^\ОВ€™XЪЩ^Kњ™[X\ЩZЩ^\ЦЬHH^Y\–ЬKњ™[X\ЩZЩ^\ОВ€™XЪЩ^Kњ^ZЩ^\ЦЬHH^Y\–ЬKњ^ZЩ^\ОВ€B€™XЪЩ^Kќ[YHHЭ[YNВ€™XЪЩ^Kљ[ќ\ќ[H[ќ\ќ[В€™XЪЩ^KњЮ[Э[YHH^\™XЬЭ]\ЛOњЮ[Э[YNВ€ЛЬ™XЪЩ^KњЩYYHЩ]ЩYY

NВ€Y[XЬJ	њ^\™XЬЭ]\ЛOќY™™\–Ь^\™XЬЭ]\ЛOњЮ[Э[YWK	њ™XЪЩ^KЪ^™[ЩЉ™XЪЩ^JH
NВ€B‚€Y€
Э[YHЏHX^Ь™XЧЭ[YH
HЭЬ™XЫЬ™[њ]К
NИЛИШY™B€YЉ^\™XЬЭ]\ЛOњЭ]\ИOHWФ‘PЧФ‘PКH
КЬ^\™XЬЭ]\ЛOњЮ[Э[YNВ‚€ЛЩXќYЧЬљ[ќЉќ[YN€	YЮ[О€	Y‹
LМЉ][YK
LМЉ\^\™XЬЭ]\ЛOњЮ[Э[YJNВ€ЛЩXќYЧЬљ[ќЉљЩ^\О€	Y‹^Y\–МKњ™[X\ЩZЩ^\Й‘“QЧРUPТКNВ‚€™]\›€NВџB‚љ[ќ^T™XЫЬ™Y[њ]К
BћВ€[ќHВ€™XТЩ^\И™XЪЩ^NВ€Ъ\€]УPVРT‘ЧУS€
ИWNВ€Ъ^™WЭљ[\Ъ^™HHВ€Ъ\€XY\–Н—NВ‚€YЉ^\™XЬЭ]\ЛOњЭ]\ИOHWФ‘PЧФVJH™]\›€В€Y€
\^\™XЬЭ]\ЛO™YЪ[€
HЛЭ[YHЏH^\™XЬЭ]\ЛOњЭ\ќ[YH	‰‚€В€Y€
Э›[Љ^\™XЬЭ]\ЛOњ]
HH
HЩ]\ЩT]
]”Ш]™\И‹
NВ€[ЩHЭЬJ]^\™XЬЭ]\ЛOњ]
NВ€Y€
]ЬЭ›[Љ]
KLWHOH	ЛЙИ
HЭЬJ]‹ИЉNВ‚€Y€
^\™XЬЭ]\ЛOќY™™\ЉB€В€њ™YJ^\™XЬЭ]\ЛOќY™™\ЉNВ€^\™XЬЭ]\ЛOќY™™\€H•SВ€B€YЉ^\™XЬЭ]\ЛOљ[™JB€В€ЫЬЩJ^\™XЬЭ]\ЛOљ[™JNВ€B‚€^\™XЬЭ]\ЛOљ[™HH›Ь[ЉЭШ]
]^\™XЬЭ]\ЛO™љ[[[YJKњЉИЉNВ€YЉ^\™XЬЭ]\ЛOљ[™JB€В€њЩYZК^\™XЬЭ]\ЛOљ[™KСQRЧСS‘
NВ€љ[\Ъ^™HHќ[
^\™XЬЭ]\ЛOљ[™JNВ€њЩYZК^\™XЬЭ]\ЛOљ[™KСQRЧФСU
NИЛИЬ€™]Ъ[™
^\™XЬЭ]\ЛOљ[™JB‚€Y€
љ[\Ъ^™HH
B€В€љ[ќЉ‘[\H™XЫЬ™Y[њ]Иљ[K—€ЉNВ€™]\›€В€B€^\™XЬЭ]\ЛOќY™™\€H
™XТЩ^\КЉXШ[ШКKљ[\Ъ^™JМJNВ€YЉ\^\™XЬЭ]\ЛOќY™™\ЉB€В€љ[ќЉ‘\њ›Ь€И[ШШ]HќY™™\€›Ь€™XЫЬ™Y[њ]Л—€ЉNВ€™]\›€В€B€H[ЩB€В€љ[ќЉ‘\њ›Ь€ИЬ[€™XЫЬ™Y[њ]Иљ[K—€ЉNВ€™]\›€В€B€њ™XY
	љXY\‹‹K^\™XЬЭ]\ЛOљ[™JNВ€њ™XY
	њ^\™XЬЭ]\ЛOњЭ\ќ[YKЪ^™[ЩЉLМЉKK^\™XЬЭ]\ЛOљ[™JNВ€њ™XY
	њ^\™XЬЭ]\ЛO™[™[YKЪ^™[ЩЉLМЉKK^\™XЬЭ]\ЛOљ[™JNВ€њ™XY
	њ^\™XЬЭ]\ЛOќЭЮ[Э[YKЪ^™[ЩЉLМЉKK^\™XЬЭ]\ЛOљ[™JNВ€њ™XY
	њ^\™XЬЭ]\ЛOЬЩYYЪ^™[ЩЉLМЉKK^\™XЬЭ]\ЛOљ[™JNВ€њ™XY
	њ^\™XЬЭ]\ЛOњЩYYЪ^™[ЩЉZ[ќЌЭ
KK^\™XЬЭ]\ЛOљ[™JNВ€њ™XY
	њ^\™XЬЭ]\ЛOќXЪЬЛЪ^™[ЩЉ[њЪYЫ™Y
KK^\™XЬЭ]\ЛOљ[™JNВ€њ™XY
^\™XЬЭ]\ЛOќY™™\‹Ъ^™[ЩЉ™XТЩ^\КJЉ^\™XЬЭ]\ЛO™[™[YJМJKK^\™XЬЭ]\ЛOљ[™JNВ‚€ЛИЮ[И]Э\ќ[YB€Э[YHH^\™XЬЭ]\ЛOњЭ\ќ[YNВ€^\™XЬЭ]\ЛOњЮ[Э[YHHВ€^\™XЬЭ]\ЛO™YЪ[€HNВ€Ь[™
^\™XЬЭ]\ЛOЬЩYY
NВ€Ь[™МЉ^\™XЬЭ]\ЛOњЩYY
NВ€ЛЬЩ]ЭXЪЬК[Y\—ЩЩ]XЪК
K\^\™XЬЭ]\ЛOќXЪЬКNВ€B‚€ЛИ›ЭИ^HB€YЉ^\™XЬЭ]\ЛOќY™™\€	‰€^\™XЬЭ]\ЛO™YЪ[€	‰€^\™XЬЭ]\ЛOњЮ[Э[YH^\™XЬЭ]\ЛOќЭЮ[Э[YJB€В€Y[XЬJ	њ™XЪЩ^K	њ^\™XЬЭ]\ЛOќY™™\–Ь^\™XЬЭ]\ЛOњЮ[Э[YWKЪ^™[ЩЉ™XЪЩ^JH
NВ‚€YЉЭ[YHOH™XЪЩ^Kќ[YH
B€В€LМ€™^Ю[Э[YHH™XЪЩ^KњЮ[Э[YNВ‚€ЛЭ[YHH
LМЉ\™XЪЩ^Kќ[YNВ€љ[ќЉ”^H™XЫЬ™Y[њ]О€Э]Щ€Ю[ИH[YN€	Y™XХ[YN€	Y€‹[YK™XЪЩ^Kќ[YJNВ€КљY€
[ќ\ќ[OH™XЪЩ^Kљ[ќ\ќ[
B€В€ЛЪ[ќ\ќ[H
LМЉ\™XЪЩ^Kљ[ќ\ќ[В€љ[ќЉ”^H™XЫЬ™Y[њ]О€Э]Щ€Ю[ИH[ќ\ќ[€	Y™XТ[ќ\ќ[€	Y€‹[ќ\ќ[™XЪЩ^Kљ[ќ\ќ[
NВ€J‹В‚€Ъ[JЭ[YH€™XЪЩ^Kќ[YH	‰€™^Ю[Э[YH€
HВ€Y[XЬJ	њ™XЪЩ^K	њ^\™XЬЭ]\ЛOќY™™\–ЛK[™^Ю[Э[YWKЪ^™[ЩЉ™XЪЩ^JH
NВ€B€Э[YHH™XЪЩ^Kќ[YNВ‚€Ъ[JЭ[YH™XЪЩ^Kќ[YH	‰€™^Ю[Э[YH^\™XЬЭ]\ЛOќЭЮ[Э[YH
HВ€Y[XЬJ	њ™XЪЩ^K	њ^\™XЬЭ]\ЛOќY™™\–ККЫ™^Ю[Э[YWKЪ^™[ЩЉ™XЪЩ^JH
NВ€B€Э[YHH™XЪЩ^Kќ[YNВ€B‚€Y€
Э[YHOH™XЪЩ^Kќ[YH
B€В€Y€
^\™XЬЭ]\ЛOњЮ[Э[YHOH™XЪЩ^KњЮ[Э[YH
B€В€LМ€™^Ю[Э[YHH™XЪЩ^KњЮ[Э[YNВ‚€љ[ќЉ”^H™XЫЬ™Y[њ]О€Э]Щ€Ю[ИHЮ[Х[YN€	Y™XФЮ[Х[YN€	Y€‹^\™XЬЭ]\ЛOњЮ[Э[YK™XЪЩ^KњЮ[Э[YJNВ‚€Ъ[J^\™XЬЭ]\ЛOњЮ[Э[YH€™XЪЩ^KњЮ[Э[YH	‰€™^Ю[Э[YH€
HВ€Y[XЬJ	њ™XЪЩ^K	њ^\™XЬЭ]\ЛOќY™™\–ЛK[™^Ю[Э[YWKЪ^™[ЩЉ™XЪЩ^JH
NВ€B€^\™XЬЭ]\ЛOњЮ[Э[YHH™XЪЩ^KњЮ[Э[YNВ‚€Ъ[J^\™XЬЭ]\ЛOњЮ[Э[YH™XЪЩ^KњЮ[Э[YH	‰€™^Ю[Э[YH^\™XЬЭ]\ЛOќЭЮ[Э[YH
HВ€Y[XЬJ	њ™XЪЩ^K	њ^\™XЬЭ]\ЛOќY™™\–ККЫ™^Ю[Э[YWKЪ^™[ЩЉ™XЪЩ^JH
NВ€B€^\™XЬЭ]\ЛOњЮ[Э[YHH™XЪЩ^KњЮ[Э[YNВ€B‚€Y€
[ќ\ќ[OH™XЪЩ^Kљ[ќ\ќ[
H[ќ\ќ[H™XЪЩ^Kљ[ќ\ќ[В€ЛЬЬ[™МЉ™XЪЩ^KњЩYY
NВ€›Ь€
HИPVФVQT”ОИ
КИ
B€В€^Y\–ЬKљЩ^\ИH™XЪЩ^KљЩ^\ЦЬNВ€^Y\–ЬK›™]ЪЩ^\ИH™XЪЩ^K›™]ЪЩ^\ЦЬNВ€^Y\–ЬKњ™[X\ЩZЩ^\ИH™XЪЩ^Kњ™[X\ЩZЩ^\ЦЬNВ€^Y\–ЬKњ^ZЩ^\ИH™XЪЩ^Kњ^ZЩ^\ЦЬNВ€B€ЛЪ[њ]™Yњ™\Ъ
^\™XЬЭ]\ЛOњЭ]\КNВ€B€B‚€ЛЩXќYЧЬљ[ќЉњЮ[Э[N€	YЭЮ[О€	YЭ]\О‰Y‹^\™XЬЭ]\ЛOњЮ[Э[YK^\™XЬЭ]\ЛOќЭЮ[Э[YK^\™XЬЭ]\ЛOњЭ]\КNВ€YЉ^\™XЬЭ]\ЛOњЭ]\ИOHWФ‘PЧФVJH
КЬ^\™XЬЭ]\ЛOњЮ[Э[YNВ€Y€
^\™XЬЭ]\ЛOњЮ[Э[YHЏH^\™XЬЭ]\ЛOќЭЮ[Э[YHЭ[YHЏH^\™XЬЭ]\ЛO™[™[YH
HЭЬ™XЫЬ™[њ]К
NВ‚€ЛЩXќYЧЬљ[ќЉќ[YN€	YЮ[О€	Y‹
LМЉ][YK
LМЉ\^\™XЬЭ]\ЛOњЮ[Э[YJNВ€ЛЩXќYЧЬљ[ќЉљЩ^\О€	Y‹^Y\–МKњ™[X\ЩZЩ^\Й‘“QЧРUPТКNВ‚€™]\›€NВџB‚љ[ќЭЬ™XЫЬ™[њ]К
BћВ€YЉ^\™XЬЭ]\КB€В€ЭЪ]Ъ
^\™XЬЭ]\ЛOњЭ]\КB€В€Ш\ЩHWФ‘PЧФ‘PО‚€В€Ъ\€]УPVРT‘ЧУS€
ИWNВ€Ъ\€XY\–Н—HH’S”LЋВ‚€Y€
Э›[Љ^\™XЬЭ]\ЛOњ]
HH
HЩ]\ЩT]
]”Ш]™\И‹
NВ€[ЩHЭЬJ]^\™XЬЭ]\ЛOњ]
NВ€Y€
]ЬЭ›[Љ]
KLWHOH	ЛЙИ
HЭЬJ]‹ИЉNВ‚€Y€
^\™XЬЭ]\ЛOќY™™\ЉB€В€^\™XЬЭ]\ЛOљ[™HH›Ь[ЉЭШ]
]^\™XЬЭ]\ЛO™љ[[[YJKќШЉИЉNВ€YЉ^\™XЬЭ]\ЛOљ[™JB€В€^\™XЬЭ]\ЛO™[™[YHHЭ[YNВ€Y€
^\™XЬЭ]\ЛOњЮ[Э[YHЉH^\™XЬЭ]\ЛOњЮ[Э[YHHЋВ€[ЩH^\™XЬЭ]\ЛOњЮ[Э[YHOHЋВ‚€ќЬљ]JXY\‹‹K^\™XЬЭ]\ЛOљ[™JNВ€ќЬљ]J	њ^\™XЬЭ]\ЛOњЭ\ќ[YKЪ^™[ЩЉLМЉKK^\™XЬЭ]\ЛOљ[™JNВ€ќЬљ]J	њ^\™XЬЭ]\ЛO™[™[YKЪ^™[ЩЉLМЉKK^\™XЬЭ]\ЛOљ[™JNВ€ќЬљ]J	њ^\™XЬЭ]\ЛOњЮ[Э[YKЪ^™[ЩЉLМЉKK^\™XЬЭ]\ЛOљ[™JNВ€ќЬљ]J	њ^\™XЬЭ]\ЛOЬЩYYЪ^™[ЩЉLМЉKK^\™XЬЭ]\ЛOљ[™JNВ€ќЬљ]J	њ^\™XЬЭ]\ЛOњЩYYЪ^™[ЩЉZ[ќЌЭ
KK^\™XЬЭ]\ЛOљ[™JNВ€ќЬљ]J	њ^\™XЬЭ]\ЛOќXЪЬЛЪ^™[ЩЉ[њЪYЫ™Y
KK^\™XЬЭ]\ЛOљ[™JNВ€ќЬљ]J^\™XЬЭ]\ЛOќY™™\‹Ъ^™[ЩЉ™XТЩ^\КJЉ^\™XЬЭ]\ЛOњЮ[Э[YJМJKK^\™XЬЭ]\ЛOљ[™JNВ€™›\Ъ
^\™XЬЭ]\ЛOљ[™JNИЛИШY™B€ЫЬЩJ^\™XЬЭ]\ЛOљ[™JNВ€H[ЩH™]\›€В‚€њ™YJ^\™XЬЭ]\ЛOќY™™\ЉNВ€^\™XЬЭ]\ЛOќY™™\€H•SВ€H[ЩH™]\›€В€њ™XZОВ€B€Ш\ЩHWФ‘PЧФVN‚€В€YЉ^\™XЬЭ]\ЛOљ[™JB€В€Y€
^\™XЬЭ]\ЛOљ[™JHЫЬЩJ^\™XЬЭ]\ЛOљ[™JNВ€[ЩH™]\›€В€B€њ™XZОВ€B€Ш\ЩHWФ‘PЧФХФ‚€В€YЉ^\™XЬЭ]\ЛOљ[™JB€В€Y€
^\™XЬЭ]\ЛOљ[™JHЫЬЩJ^\™XЬЭ]\ЛOљ[™JNВ€[ЩH™]\›€В€B€њ™XZОВ€B€B‚€^\™XЬЭ]\ЛOњЭ]\ИHWФ‘PЧФХФВ€^\™XЬЭ]\ЛO™YЪ[€HВ€^\™XЬЭ]\ЛOњЮ[Э[YHHВ€њ™YT™XЫЬ™Y[њ]К
NВ€H™]\›€В‚€™]\›€NВџB‚љ[ќњ™YT™XЫЬ™Y[њ]К
BћВ€^\™XЬЭ]\ЛOњЭ]\ИHWФ‘PЧФХФВ€YЉ^\™XЬЭ]\ЛOљ[™JHЫЬЩJ^\™XЬЭ]\ЛOљ[™JNВ€YЉ^\™XЬЭ]\ЛOќY™™\ЉB€В€њ™YJ^\™XЬЭ]\ЛOќY™™\ЉNВ€^\™XЬЭ]\ЛOќY™™\€H•SВ€™]\›€NВ€B‚€™]\›€ВџB‚WЬ^\™XЬЭ]\К€[љ]Ъ[њ]Ь™XЫЬ™\Љ
BћВ€^\™XЬЭ]\ИH
WЬ^\™XЬЭ]\КЉXШ[ШКKЪ^™[ЩЉ
њ^\™XЬЭ]\КJNВ‚€™]\›€^\™XЬЭ]\ОВџB‚ќ›ЪYњ™YWЪ[њ]Ь™XЫЬ™\Љ
BћВ€YЉ^\™XЬЭ]\КB€В€YЉ^\™XЬЭ]\ЛOќY™™\ЉB€В€YЉ^\™XЬЭ]\ЛOљ[™JHЫЬЩJ^\™XЬЭ]\ЛOљ[™JNВ€њ™YJ^\™XЬЭ]\ЛOќY™™\ЉNВ€^\™XЬЭ]\ЛOќY™™\€H•SВ€B€њ™YJ^\™XЬЭ]\КNВ€^\™XЬЭ]\ИH•SВ€BџB‚ќ›ЪY\]J[ќ[™Ш[YK[ќ\Щ]ќШZ]
BћВ€[ќHHВ€[ќЪЩ^\ИHВ‚€ЪY€С€Y€
Ш]™Y]K™њЫ[Z]OHJHЛИњЮ[И[X›Y€В€ЛИИ™YXЩH[њ]][ЮKШZ][ќ[H\Э\И
\КHЩ€HЭ\њ™[ќ€ЛИњ[YHИ™XY[њ]ИЬ€И[ћ][™И[ЩK€ЩHШ[€Щ]]Ш^HЪ]\И™XШ]\ЩB€ЛИHФ\ИЩ€[[Щ\›€ЫЫ\]\њИH]™[€Ы™\И[™ЭЛY[™Э]]YЬИB€ЛИ\™HЫЫ\]HЭ™\љЪ[›Ь€Ь[ђ“Ф‰ЬИ™YYЛ‚€НЌ\™Щ]Э[YHH[Y\—Э]XЪЬК
H
ИLЭљY[ЧШЭ\њ™[ќЬ™Yњ™\ЪЬ]J
HHВ€MЌЭ\њ™[ќЭ[YHH[Y\—Э]XЪЬК
NВ€Ъ[H
Э\њ™[ќЭ[YH\™Щ]Э[YJB€В€\ЫY\
\™Щ]Э[YHHЭ\њ™[ќЭ[YJNВ€Э\њ™[ќЭ[YHH[Y\—Э]XЪЬК
NВ€B€B€Щ[™Y‚‚€Щ][ќ\ќ[

NВ€YЉ^\™XЬЭ]\ЛOњЭ]\ИOHWФ‘PЧФVH	‰€WЬ]\ЩH	‰€]™[
HY€
\^T™XЫЬ™Y[њ]К
H
HЭЬ™XЫЬ™[њ]К
NВ€[њ]™Yњ™\Ъ
^\™XЬЭ]\ЛOњЭ]\КNВ€ЪY™Y€СP“B€[ЭљYWЬ^XXЪЧЭ\]J€
›Э™]ЪЩ^\И	€
“QЧСTРИ“QЧРS–P•UУЉJHOH€
NВ€Щ[™Y‚€YЉ^\™XЬЭ]\ЛOњЭ]\ИOHWФ‘PЧФ‘PИ	‰€WЬ]\ЩH	‰€]™[
HY€
\™XЫЬ™[њ]К
H
HЭЬ™XЫЬ™[њ]К
NВ‚€Y€

WЬ]\ЩH	‰€[™Ш[YHOHJH[Ш^\Э\]JB€В€^XЭ]WЭ\]\ШЬљ\К
NВ€B‚€™]Э[YHHВ€YЉWЬ]\ЩJB€В€YЉ[™Ш[YHOHHЪXЪЧЪ[—ЬШЬ™Y[Љ
JB€В€^XЭ]WЪЩ^\ШЬљ\К
NВ€B‚€YЉ
]™[ШЫЫ\]Y	‰€]™[O›ЬЬЧЬЫЭИOH“ФФЧФУХЧУУ€	‰€]ЬЬYY\
HЫЭЫ[Э[Ы‹ќЩЩЫH€УХЧУSХSУ—УС‘ЉB€В€YЉЫЭЫ[Э[Ы‹™\][Ы€OHЫЭЫ[Э[Ы‹ЫЭ[ќ\ЉB€В€™]Э[YHHЭ[YH
И[ќ\ќ[В€B€B€[ЩB€В€™]Э[YHHЭ[YH
И[ќ\ќ[В€B‚€ЫЭЫ[Э[Ы‹ЫЭ[ќ\ЉКОВ€YЉЫЭЫ[Э[Ы‹ЫЭ[ќ\€OH
ЫЭЫ[Э[Ы‹™\][Ы€
ИJJB€В€ЫЭЫ[Э[Ы‹ЫЭ[ќ\€HВ€YЉЫЭЫ[Э[Ы‹ќЩЩЫH€УХЧУSХSУ—УУЉB€В€ЫЭЫ[Э[Ы‹™\][Ы€HЫЭЫ[Э[Ы‹ќЩЩЫNВ€B€B€YЉ™]Э[YH€Э[YH
ИL
B€В€™]Э[YHHЭ[YH
ИLВ€B‚€Ъ[JЭ[YH™]Э[YJB€В€YЉ[™Ш[YHOHH[Ш^\Э\]JB€В€^XЭ]WЭ\][ЩЪXЬШЬљ\К
NВ€B‚€YЉ[™Ш[YHOHJB€В€\]WЬШЬ›Ы\Љ
NВ€YЉYњ™Y^™X[
B€В€[ќ[ЬШ[]™HHВ‚€›ЬЉHHИHPVФVQT”ОИJККB€В€Y€
\^Y\–ЪWK™[ќ
H
КШ[ЬШ[]™NВ€B€[ЬШ[]™HH
[ЬШ[]™HЏHPVФVQT”КHИH€В‚€YЉ]™[OњЩ][YH€
]™[Oќ\HOH€	‰€[ЬШ[]™JJB€ЛЪYЉ]™[OњЩ][YH€
]™[Oќ\HOH€	‰€\^Y\–МK™[ќ	‰€\^Y\–МWK™[ќ	‰€\^Y\–М—K™[ќ	‰€\^Y\–МЧK™[ќ
JB€В€[ќ[ЬЫ›Ъ›Ъ[€H[ЬЫ›ШЬ™Y]ИHВ‚€›ЬЉHHИHPVФVQT”ОИJККB€В€Y€
\^Y\–ЪWKљ›Ъ[љ[™КH
КШ[ЬЫ›Ъ›Ъ[ЋВ€B€[ЬЫ›Ъ›Ъ[€H
[ЬЫ›Ъ›Ъ[€ЏHPVФVQT”КHИH€В‚€›ЬЉHHИHPVФVQT”ОИJККB€В€Y€
^Y\–ЪWKЬ™Y]ИJH
КШ[ЬЫ›ШЬ™Y]ОВ€B€[ЬЫ›ШЬ™Y]ИH
[ЬЫ›ШЬ™Y]ИЏHPVФVQT”КHИH€В‚€YЉ[Y[Yќ€
B€В€K][Y[YќВ€B€[ЩHYЉ
]™[OњЩ][YH€	‰€[ЬЫ›Ъ›Ъ[ЉH€


[›ЬЪ\™H	‰€Ь™Y]ИJH
›ЬЪ\™H	‰€[ЬЫ›ШЬ™Y]КJB€	‰€[ЬЫ›Ъ›Ъ[€
B€
B€В€[YWЫЭ™\Љ
NВ€B€B€B€\]WЬШЬ›ЫYШ™К
NВ€YЉ]™[Oќ\HOHЉB€В€\]\Э]\К
NВ€B€B€YЉ[™Ш[YHOHHЪXЪЧЪ[—ЬШЬ™Y[Љ
JB€В€\]WЩ[ќК
NВ€B‚€YЉ[™Ш[YHOHH[Ш^\Э\]JB€В€^XЭ]WЭ\]YЩЪXЬШЬљ\К
NВ€B‚€
КЧЭ[YNВ€B‚€B‚€КЉЉЉЉЉЉЉЉЉЉЉ€Щћ]Y]YZ[™И
ЉЉЉЉЉЉЉЉЉЉЉ‹В‚€ЫX\њШЬ™Y[ЉњШЬ™Y[ЉNВ‚€YЉ[™Ш[YHOHH	‰€WЬ]\ЩJB€В€]ЧЬШЬ›ЫYШ™К
NВ€YЉ]™[Oќ\HOHЉB€В€™Y]ЬЭ]\К
NВ€B€YЉ]™[Oќ\HOHЉB€В€]ЬЭ]\К
NВ€B€]ЧЭ^ШљњК
NВ€B‚€YЉZ[™Ш[YJB€В€YЉXЪЩЬ›Э[™
B€В€Ьљ]\WШYЬШЬ™Y[ЉRS—ТS•XЪЩЬ›Э[™•S
NВ€B€B‚€ЛИ[ќ]HЬљ]\И]Y]YZ[™В€YЉ[™Ш[YHOHHЪXЪЧЪ[—ЬШЬ™Y[Љ
JB€YЉWЬ]\ЩJB€В€\Ь^WЩ[ќК
NВ€B‚€КЉЉЉЉЉЉЉЉЉЉЉ€\]YШЬљ\
ЉЉЉЉЉЉЉЉЉЉЉ‹В€YЉ[™Ш[YHOHH[Ш^\Э\]JB€В€^XЭ]WЭ\]YШЬљ\К
NВ€B‚€›ЬЉHHИHPVФVQT”ОИJККB€В€Y€
^Y\–ЪWK™[ќ	‰€
^Y\–ЪWK›™]ЪЩ^\И	€“QЧФХT•
JB€В€ЪЩ^\ИHNВ€њ™XZОВ€B€B‚€ЛИЊLKМLМЊ€U€[Э™H]\ЩHY[ќHЩЪXИ\™B€КљYЉ[™Ш[YHOHH	‰€WЬ]\ЩH	‰€[›Ь]\ЩH	‰‚€

^Y\–МK™[ќ	‰€
^Y\–МK›™]ЪЩ^\И	€“QЧФХT•
JH€
^Y\–МWK™[ќ	‰€
^Y\–МWK›™]ЪЩ^\И	€“QЧФХT•
JH€
^Y\–М—K™[ќ	‰€
^Y\–М—K›™]ЪЩ^\И	€“QЧФХT•
JH€
^Y\–МЧK™[ќ	‰€
^Y\–МЧK›™]ЪЩ^\И	€“QЧФХT•
JJB€
J‹В€YЉ[™Ш[YHOHH	‰€WЬ]\ЩH	‰€[›Ь]\ЩH	‰€ЪЩ^\КB€В€Y€
JЫЭЧЫXZ[›Y[ќWЩ›YЙЊJH
B€В€ЫЭ[™Ь]\ЩWЫ]\ЪXКJNВ€ЫЭ[™Ь]\ЩWЬШ[\JJNВ€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Эњ]\ЩKШ]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€]\Щ[Y[ќJ
NВ€™]\›ЋВ€B€B€YЉ[™Ш[YHOHH	‰€
ЫЭЧЫXZ[›Y[ќWЩ›YЙЊJH
B€В€XЪЭЧЫXZ[›Y[ќJ
NВ€™]\›ЋВ€B‚€КЉЉЉЉЉЉЉЉЉ€\]HШЬ™Y[€
ЉЉЉЉЉЉЉЉЉЉЉЉЉ‹В‚€Ьљ]\WЩ]КњШЬ™Y[‹RS—ТS•PVТS•
NИЛИ›ЭXЩK[Ш^\И]ИЬљ]\И]H™\ћH[™Щ€Э\€Y]ЩВ‚€YЉЬ]\ЩHOH€	‰€[›ЬШЬ™Y[њЪЭ	‰€
›Э™]ЪЩ^\И	€“QЧФРФ‘QS”ТХ
JB€В€ШЬ™Y[њЪЭ
њШЬ™Y[‹Щ][JNВ€B‚€ЛИXќYИЭY™‹ЪЭ[›Э\X\€Ы€ШЬ™Y[њЪЭ€YЉXќYЧЭ[YHOH‘‘‘‘‘‘‘ЉB€В€XќYЧЭ[YHHЭ[YH
ИЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYY
€NВ€B€YЉЭ[YHXќYЧЭ[YH	‰€XќYЧЫ\ЩЦМJB€В€YЉXќYЧЮWЫ\ЩЛћЏH	‰€XќYЧЮWЫ\ЩЛћHЏH
B€В€Y€
XќYЧЮWЫ\ЩЛ™›ЫќЪ[™^
HXќYЧЮWЫ\ЩЛ™›ЫќЪ[™^HВ€ШЬ™Y[—Ьљ[ќЉњШЬ™Y[‹XќYЧЮWЫ\ЩЛћXќYЧЮWЫ\ЩЛћKXќYЧЮWЫ\ЩЛ™›ЫќЪ[™^XќYЧЫ\ЩКNВ€H[ЩHШЬ™Y[—Ьљ[ќЉњШЬ™Y[‹љY[Ы[Щ\Лќ”™\ЛY›ЫќZYЪ

KXќYЧЫ\ЩКNВ€B€[ЩB€В€XќYЧЫ\ЩЦМHHВ€XќYЧЮWЫ\ЩЛћHLNВ€XќYЧЮWЫ\ЩЛћHHLNВ€ЪY™Y€P•QЧУSСB€YЉ]™[OњЬКB€В€XќYЧЬљ[ќЉ”ЬЪ][ЫЋ€	ZKЪY€	ZKЬ]ЫЋ€	ZKЩ™њЩ]О€	ZKЙZH‹]™[OњЬЛ]™[OќЪYЭ\њ™[ќЬЬ]Ы‹]™[Oњ]XZЩKЩћЮWЫЩ™њЩ]
NВ€B€Щ[™Y‚€B‚€YЉ\Щ]ќШZ]
B€В€™ШWЭќШZ]

NВ€B€љY[ЧШЫЬWЬШЬ™Y[ЉњШЬ™Y[ЉNВ€Ьљ]\WШЫX\Љ
NВ‚€ЪXЪЧЫ]\ЪXК
NВ€ЫЭ[™Э\]WЫ]\ЪXК
NВџB‚‚‚‚‹ЛИKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKB‹К€ЫX›ИKНМЊL€™]Иќ[Э[Ы€]Ш[€\ЩHњљYЪ™\ЬЛЩШ[[XHЫЬњ™XЭ[Ы‚€
€[™\[™[ќњ›ЫHHЫШ[[]HЫ€]›Ь›\ИЪ\™H]	ЬИ]Z[X›K‚€
€\™Ш\™HXШЩ[\]YњљYЪ™\ЬЛЩШ[[XHЫЬњ™XЭ[Ы€\И]Z[X›HЫ€Ь[‘У€
€]›Ь›\И\Ъ[™ИУУ€™]\›њИHЫ€ЭXШЩ\ЬЛЫ€\њ›Ь‹‚€
‹Вљ[ќЩ]ШЫЫЬ—ШЫЬњ™XЭ[ЫЉ[ќЫK[ќњЉBћВ€љY[ЧЬЩ]ШЫЫЬ—ШЫЬњ™XЭ[ЫЉЫKњЉNВ€™]\›€NВџB‚‹ЛИЪ[\H[]HYHИњШЬ™Y[€YBќ›ЪYYWЫЭ]
[ќ\K[ќЬYY
BћВ€[ќK€HВ€[ќ‹ИHВ€LМ€[ќ\ќ[HВ€[ќЭ\њ™[ќHЬYYИЬYY€YNВ€ЧЬШЬ™Y[€
™ќY™™\€H•SВ€ЧЩ]ЫY]ЩHHZ[›Y]ЩВ€K[HH“S‘УSСWРU‘TђQСNВ‚€›ЬЉHH€HИ€ЌИ
B€В€Ъ[J€HH	‰€€Ќ
B€В€YЉ]\H\HOHJB€В€€H

Ш]™Y]KњљYЪ™\ЬИ
ИЌMЉH
€
ЌHЉHИЌ
HHЌMЋВ€ИHЌM€H

Ш]™Y]K™Ш[[XH
ИЌMЉH
€
ЌHЉHИЌ
NВ€™ШWЭќШZ]

NВ€YЉ\Щ]ШЫЫЬ—ШЫЬњ™XЭ[ЫЉЛЉJB€В€YЉYќY™™\ЉB€В€ќY™™\€H[ШЬШЬ™Y[ЉњШЬ™Y[‹OќЪYњШЬ™Y[‹OљZYЪњШЬ™Y[‹Oњ^[›Ь›X]
NВ€ЫЬ\ШЬ™Y[ЉќY™™\‹њШЬ™Y[ЉNВ€B€ЛМЌMH
И[H“S‘УSСWРU‘TђQСH\ИXЭX[H[€›[™ЫИ\ЩHЌM[њЭXY€KЪ[›™[€HKЪ[›™[ИHKЪ[›™[€HЌM
€
ЌHЉHИЌВ€ЫX\њШЬ™Y[ЉњШЬ™Y[ЉNВ€]ШЬ™Y[ЉњШЬ™Y[‹ќY™™\‹	™JNВ€B€B€ЉКОВ€YЉ]\H\HOHJB€В€љY[ЧШЫЬWЬШЬ™Y[ЉњШЬ™Y[ЉNВ€B€B€YЉ]\H\HOHЉB€В€ЫЭ[™Э\]WЫ]\ЪXК
NВ€YЉ[]\ЪXЫЭ™\›\
B€В€ЫЭ[™Э›Ы[YWЫ]\ЪXКШ]™Y]K›]\ЪXЭ›Ы
€
ЌHЉHИЌШ]™Y]K›]\ЪXЭ›Ы
€
ЌHЉHИЌ
NВ€B€B€[ќ\ќ[H[Y\—ЩЩ][ќ\ќ[
Э\њ™[ќ
NВ€YЉ[ќ\ќ[€Э\њ™[ќ
B€В€[ќ\ќ[HЭ\њ™[ќИЊВ€B€YЉ[ќ\ќ[€Э\њ™[ќИ
B€В€[ќ\ќ[HЭ\њ™[ќИВ€B€H
ПH[ќ\ќ[В€B‚€YЉ]\H\HOHЉB€В€YЉ[]\ЪXЫЭ™\›\
B€В€ЫЭ[™ШЫЬЩWЫ]\ЪXК
NВ€B€B‚€YЉ]\H\HOHJB€В€ЫX\њШЬ™Y[ЉњШЬ™Y[ЉNВ€љY[ЧШЫЬWЬШЬ™Y[ЉњШЬ™Y[ЉNВ€™ШWЭќШZ]

NВ€ЛЭH›XЪИШЬ™Y[‹ЫИЩH™]\›€И›Ь›X[[]B€Щ]ШЫЫЬ—ШЫЬњ™XЭ[ЫЉШ]™Y]K™Ш[[XKШ]™Y]KњљYЪ™\ЬКNВ€B‚€YЉќY™™\ЉB€В€њ™Y\ШЬ™Y[Љ	™ќY™™\ЉNВ€BџB‚‚‚ќ›ЪY\WШЫЫќ›ЫК
BћВ€[ќВ‚€›ЬЉHИPVФVQT”ОИ
ККB€В€ЫЫќ›ЫЬЩ]Щ^J^Y\ЫЫќ›ЫЪ[ќ\њЦЬK“QЧСTРЛУУ•“УСTРКNВ€ЫЫќ›ЫЬЩ]Щ^J^Y\ЫЫќ›ЫЪ[ќ\њЦЬK“QЧУSХ‘UTШ]™Y]KљЩ^\ЦЬVФСQУSХ‘UTJNВ€ЫЫќ›ЫЬЩ]Щ^J^Y\ЫЫќ›ЫЪ[ќ\њЦЬK“QЧУSХ‘QХУ‹Ш]™Y]KљЩ^\ЦЬVФСQУSХ‘QХУ—JNВ€ЫЫќ›ЫЬЩ]Щ^J^Y\ЫЫќ›ЫЪ[ќ\њЦЬK“QЧУSХ‘SQ•Ш]™Y]KљЩ^\ЦЬVФСQУSХ‘SQ•JNВ€ЫЫќ›ЫЬЩ]Щ^J^Y\ЫЫќ›ЫЪ[ќ\њЦЬK“QЧУSХ‘T’QТШ]™Y]KљЩ^\ЦЬVФСQУSХ‘T’QТJNВ€ЫЫќ›ЫЬЩ]Щ^J^Y\ЫЫќ›ЫЪ[ќ\њЦЬK“QЧРUPТЛШ]™Y]KљЩ^\ЦЬVФСQРUPТЧJNВ€ЫЫќ›ЫЬЩ]Щ^J^Y\ЫЫќ›ЫЪ[ќ\њЦЬK“QЧРUPТМ‹Ш]™Y]KљЩ^\ЦЬVФСQРUPТМ—JNВ€ЫЫќ›ЫЬЩ]Щ^J^Y\ЫЫќ›ЫЪ[ќ\њЦЬK“QЧРUPТМЛШ]™Y]KљЩ^\ЦЬVФСQРUPТМЧJNВ€ЫЫќ›ЫЬЩ]Щ^J^Y\ЫЫќ›ЫЪ[ќ\њЦЬK“QЧРUPТНШ]™Y]KљЩ^\ЦЬVФСQРUPТНJNВ€ЫЫќ›ЫЬЩ]Щ^J^Y\ЫЫќ›ЫЪ[ќ\њЦЬK“QЧТ•STШ]™Y]KљЩ^\ЦЬVФСQТ•STJNВ€ЫЫќ›ЫЬЩ]Щ^J^Y\ЫЫќ›ЫЪ[ќ\њЦЬK“QЧФФPТPSШ]™Y]KљЩ^\ЦЬVФСQФФPТPSJNВ€ЫЫќ›ЫЬЩ]Щ^J^Y\ЫЫќ›ЫЪ[ќ\њЦЬK“QЧФХT•Ш]™Y]KљЩ^\ЦЬVФСQФХT•JNВ€ЫЫќ›ЫЬЩ]Щ^J^Y\ЫЫќ›ЫЪ[ќ\њЦЬK“QЧФРФ‘QS”ТХШ]™Y]KљЩ^\ЦЬVФСQФРФ‘QS”ТХJNВ€BџB‚‚‚‹ЛИKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKB‚ќ›ЪY\Ь^WШЬ™Y]К
BћВ€LМ€љ[љ\Ъ[YHHЭ[YH
ИL
€ЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYYВ€[ќЫ™HHВ€[ќИHљY[Ы[Щ\Лќ”ЪYќИ€
ИОВ€[ќ€H
љY[Ы[Щ\Лќ”™\ИHљY[Ы[Щ\Лќ”ЪYќ
HИЌВ€[ќHHВ€[ќHљY[Ы[Щ\Лљ™\ИИЋВ€[ќЫЫHHH›Ыќ[Ы›ЭЪY

H
€MЋВ€[ќЫЫ€H
И›Ыќ[Ы›ЭЪY

H
€В‚€YЉШ]™Y]K›ЩЫИOHJB€В€™]\›ЋВ€B€YWЫЭ]

NВ‚€[›ШYЫ]™[

NВ‚€›Э™]ЪЩ^\ИHВ‚€Ъ[JYЫ™JB€В€HHЋВ‚€›ЫќЬљ[ќЉЬЭ›ZY
‹ђЬ™Y]ИЉKЛ‹ђЬ™Y]ИЉNВ‚€›ЫќЬљ[ќЉЬЭ›ZY
K“Ь[ђ“Ф€ЉKИ
И€
€KK“Ь[ђ“Ф€ЉNИ
КЫNВ‚€›ЫќЬљ[ќЉЫЫKИ
И€
€KђШ\ЪЩ^K[[Ы€‹€ЉNВ€›ЫќЬљ[ќЉЫЫ‹И
И€
€K”›Ъ™XЭXYЉNИ
КЫNВ‚€›ЫќЬљ[ќЉЫЫKИ
И€
€K“\ЫX[ZНЋHЉNВ€›ЫќЬљ[ќЉЫЫ‹И
И€
€K‘]™[Ь\€ЉNИ
КЫNВ‚€›ЫќЬљ[ќЉЫЫKИ
И€
€K’Ь]\ИЉNВ€›ЫќЬљ[ќЉЫЫ‹И
И€
€K‘]™[Ь\€ЉNИ
КЫNВ‚€›ЫќЬљ[ќЉЬЭ›ZY
K‘›Ь›Y\€ЭY™€ЉKИ
И€
€KK‘›Ь›Y\€ЭY™€ЉNИ
КЫNВ‚€›ЫќЬљ[ќЉЫЫKИ
И€
€K‘љYЪ€ЫЬ™ИЉNВ€›ЫќЬљ[ќЉЫЫ‹И
И€
€K‘ќYЭYHЉNИ
КЫNВ‚€›ЫќЬљ[ќЉЫЫKИ
И€
€K’Рђ[™™\ЬЩ[€ЉNВ€›ЫќЬљ[ќЉЫЫ‹И
И€
€K’Ъ\ћHЉNИ
КЫNВ‚€›ЫќЬљ[ќЉЫЫKИ
И€
€K“Ь™[ЉNВ€›ЫќЬљ[ќЉЫЫ‹И
И€
€K“Ь›ШЪWЦЉNИ
КЫNВ‚€›ЫќЬљ[ќЉЫЫKИ
И€
€K”ЦЉNВ€›ЫќЬљ[ќЉЫЫ‹И
И€
€K•Z[ИЉNИ
КЫNВ‚€›ЫќЬљ[ќЉЫЫKИ
И€
€KќU[›™[ИЉNВ€›ЫќЬљ[ќЉЫЫ‹И
И€
€K•Ъ]HYЫЫ€ЉNИ
КЫNВ‚€›ЫќЬљ[ќЉЫЫKИ
И€
€K”ЫX›ИЉNИ
КЫNВ‚€›ЫќЬљ[ќЉЬЭ›ZY
K”ЬќИЉKИ
И€
€KK”ЬќИЉNИ
КЫNВ‚€›ЫќЬљ[ќЉЫЫKИ
И€
€K“[ќ^УФЦЉNВ€›ЫќЬљ[ќЉЫЫ‹И
И€
€K”ЦЉNИ
КЫNВ‚€›ЫќЬљ[ќЉЫЫKИ
И€
€Kђ[™›ЪYЉNВ€›ЫќЬљ[ќЉЫЫ‹И
И€
€KђФћ‘YKЫX›ЛЉNИ
КЫNВ€›ЫќЬљ[ќЉЫЫ‹И
И€
€KќU[›™[Л\ЫX[ZНЋHЉNИ
КЫNВ€›ЫќЬљ[ќЉЫЫ‹И
И€
€K•Ъ]HYЫЫ€ЉNИ
КЫNВ‚€\]J‹
NВ‚€Ы™HH
Э[YH€љ[љ\Ъ[YJNВ€Ы™HH
›Э™]ЪЩ^\И	€
“QЧФХT•
И“QЧСTРКJNВ€B€YHHНNВ€YWЫЭ]

NВџB‚‚ќ›ЪY›Ь”Ъ]ЭЫЉ[ќЭ]\ЛЫЫњЭЪ\€
›\ЩЛ‹‹ЉBћВ€WЫ\Э\™Ы\ЭВ€WЫ\ЭЭ]]Ш\™Э[Y[ќОВ€[ќNВ‚€Э]XИ[ќЪ][™ЩЭЫ€HВ‚€YЉЪ][™ЩЭЫЉB€В€™]\›ЋВ€B‚€Ъ][™ЩЭЫ€HNВ€WЬЭ\ќ
\™Ы\Э\ЩКNВ‚€ЛЬљ[ќЉњШ]™Y]K›ЩЫИ	Y€‹Ш]™Y]K›ЩЫКNВ‚€YЉY\ШX›[ЩКB€В€ЭЪ]Ъ
Э]\КB€В€Ш\ЩH‚€љ[ќЉ—ЉЉЉЉЉЉЉЉЉЉЉЉ€Ъ][™ИЭЫ€
ЉЉЉЉЉЉЉЉЉЉЉ——€ЉNВ€њ™XZОВ€Y][‚€љ[ќЉ—ЉЉЉЉЉЉЉЉЉЉ€[€\њ›Ь€ШШЭ\њ™Y
ЉЉЉЉЉЉЉЉЉ€‚€—Љ€Ъ][™ИЭЫ€
——€ЉNВ€њ™XZОВ€B€B‚€YЉY\ШX›[ЩКB€В€WШЫЬJЭ]]Ш\™Э[Y[ќЛ\™Ы\Э
NВ€Ьљ]UУЩСљ[UЉ\ЩЛЭ]]Ш\™Э[Y[ќКNВ€WЩ[™
Э]]Ш\™Э[Y[ќКNВ€B‚‚€Щ][TЭ]\К–UTКNВ€Ш]™\Щ][™ЬК
NВ‚€К€[ќћHЪ[ќ›Ь€H[™Ъ[™HЬ™Y]ИШЬ™Y[‹€
‹В€ЛЬШЬ™Y[—ЬЭ]\ИHS—ФРФ‘QS—СS‘ТS‘WРФ‘QUВBB‚€ЛИYЉЭ]\ИOHЉB€ЛИВ€ЛИ\Ь^WШЬ™Y]К
NВ€ЛИB‚€YЉЭ\ќ\ЩЫ™JB€И€ШЬ™Y[—ЬЭ]\И	ЏH’S—ФРФ‘QS—СS‘ТS‘WРФ‘QUВ€\›WЭљY[Ы[Щ\К
NВ€B‚€YЉY\ШX›[ЩКB€В€љ[ќЉ”™[X\ЩH]™[]HЉNВ€B€Y€
Э\ќ\ЩЫ™JB€В€[›ШYЫ]™[Ь™\Љ
NВ€B€YЉY\ШX›[ЩКB€В€љ[ќЉ‹‹‹‹‹‹‹‹‹‹‹—€ЉNВ€B€YЉЭ\ќ\ЩЫ™JB€В€[›ШYЫ]™[

NВ€B€YЉY\ШX›[ЩКB€В€љ[ќЉ‘Ы™HW—€ЉNВ€B‚€YЉY\ШX›[ЩКB€В€љ[ќЉ”™[X\ЩHЬ\XЬИ]HЉNВ€B€YЉY\ШX›[ЩКB€В€љ[ќЉ‹‹€ЉNВ€B€YЉЭ\ќ\ЩЫ™JB€В€њ™Y\ШЬ™Y[Љ	ќњШЬ™Y[ЉNИЛИ[ШШ]YћH[љ]ЭљY[Ы[Щ\В€B€YЉЭ\ќ\ЩЫ™H	‰€^[›Ь›X]OHVSЮ
H›ЬЉHHИHPVР“S‘S‘ФОИJККB€В€њ™YJ›[™X›\ЦЪWJNВ€B€YЉY\ШX›[ЩКB€В€љ[ќЉ‹‹€ЉNВ€B€YЉЭ\ќ\ЩЫ™JB€В€њ™Y\ШЬ™Y[Љ	XЪЩЬ›Э[™
NВ€B€YЉY\ШX›[ЩКB€В€љ[ќЉ‹‹€ЉNВ€B€ЪY™Y€РPТWРђPТСФ“ХS‘В€YЉЭ\ќ\ЩЫ™JH›ЬЉHHИHPVРРPТQРђPТСФ“ХS‘ОИJККB€В€њ™Y\ШЬ™Y[Љ	™ЧШШXЪVЪWJNВ€B€YЉY\ШX›[ЩКB€В€љ[ќЉ‹‹€ЉNВ€B€Щ[™Y‚€YЉЭ\ќ\ЩЫ™JB€В€њ™Y\Ьљ]\К
NВ€B€YЉY\ШX›[ЩКB€В€љ[ќЉ‹‹€ЉNВ€B€YЉЭ\ќ\ЩЫ™JB€В€[›ШYШ[Щ›ЫќК
NВ€B€YЉY\ШX›[ЩКB€В€љ[ќЉ—Ы™HW€ЉNВ€B‚‚€YЉY\ШX›[ЩКB€В€љ[ќЉ”™[X\ЩHШ[YH]K‹‹‹‹‹‹‹‹‹‹‹——€ЉNВ€B‚€YЉЭ\ќ\ЩЫ™JB€В€њ™YWЩ[ќК
NВ€B€YЉЭ\ќ\ЩЫ™JB€В€њ™YWЫ[Щ[К
NВ€B€YЉЭ\ќ\ЩЫ™JB€В€њ™YWЫ[Щ[ШXЪJ
NВ€B€YЉЭ\ќ\ЩЫ™JB€В€ЫX\—ЬШЬљ\К
NВ€B€YЉY\ШX›[ЩКB€В€љ[ќЉ—”™[X\ЩHШ[YH]K‹‹‹‹‹‹‹‹‹‹‹—Ы™HW€ЉNВ€B‚€YЉY\ШX›[ЩКB€В€љ[ќЉ”™[X\ЩH[Y\‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹€ЉNВ€B€YЉЭ\ќ\ЩЫ™JB€В€›Ь•[Y\‘^]

NВ€B€YЉY\ШX›[ЩКB€В€љ[ќЉ—Ы™HW€ЉNВ€B‚€YЉY\ШX›[ЩКB€В€љ[ќЉ”™[X\ЩH[њ]\™Ш\™K‹‹‹‹‹‹€ЉNВ€B€YЉЭ\ќ\ЩЫ™JB€В€ЫЫќ›ЫЩ^]

NВ€B€YЉY\ШX›[ЩКB€В€љ[ќЉ—Ы™HW€ЉNВ€B‚€YЉY\ШX›[ЩКB€В€љ[ќЉ”™[X\ЩHЫЭ[™Ю\Э[K‹‹‹‹‹‹‹‹€ЉNВ€B€YЉЭ\ќ\ЩЫ™JB€В€ЫЭ[™Щ^]

NВ€B€YЉY\ШX›[ЩКB€В€љ[ќЉ—Ы™HW€ЉNВ€B‚€YЉY\ШX›[ЩКB€В€љ[ќЉ”™[X\ЩHљ[PШXЪ[™ИЮ\Э[K‹‹€ЉNВ€B€YЉЭ\ќ\ЩЫ™JB€В€ZЧЭ\›J
NВ€B€YЉY\ШX›[ЩКB€В€љ[ќЉ—Ы™HW€ЉNВ€B‚€YЉ[Щ[ЫY\Э
B€В€њ™YPЫЫ[X[™\Э
[Щ[ЫY\Э
NИЛИ[Э™Y\™H™XШ]\ЩH\Э\И›Э[љ]X[^™YY€Ъ]ЭЫ€\И[љ]X]Yњ›ЫH[њЪYHHY[ќB€B€YЉ[Щ[ЭЫY\Э
B€В€њ™YPЫЫ[X[™\Э
[Щ[ЭЫY\Э
NВ€B€YЉ]™[ЫY\Э
B€В€њ™YPЫЫ[X[™\Э
]™[ЫY\Э
NВ€B€YЉ]™[Ь™\ЫY\Э
B€В€њ™YPЫЫ[X[™\Э
]™[Ь™\ЫY\Э
NВ€B‚€њ™YS[Щ[\Э

NВ€ЫX\—ЬШ]™YШ[ЭЬЩ[XЭШ\™Э[Y[ќК
NВ€њ™YJШ]™[]™[Ш[ЭЬЩ[XЭШ\™ЬКNВ€Ш]™[]™[Ш[ЭЬЩ[XЭШ\™ЬИH•SВ€YЉШ]™[]™[
B€В€њ™YJШ]™[]™[
NВ€Ш]™[]™[H•SВ€B€Ш]™[]™[ШЫЭ[ќHВ€њ™YJ[ЭЬЩ[XЭШ\™ЬКNВ€[ЭЬЩ[XЭШ\™ЬИH•SВ€њ™YYљ[[[YXШXЪJ
NВ€Ш—Э\›][њК
NВ‚€ЛИњ™YH[њ]™XЫЬ™\‚€њ™YWЪ[њ]Ь™XЫЬ™\Љ
NВ‚€YЉY\ШX›[ЩКB€В€љ[ќЉ—ЉЉЉЉЉЉЉЉЉЉЉЉЉЉЉЉ€Ы™H
ЉЉЉЉЉЉЉЉЉЉЉЉЉЉЉЉ——€ЉNВ€B‚€YЉY\ШX›[ЩКB€В€WШЫЬJЭ]]Ш\™Э[Y[ќЛ\™Ы\Э
NВ€Ьљ]UУЩСљ[UЉ\ЩЛЭ]]Ш\™Э[Y[ќКNВ€WЩ[™
Э]]Ш\™Э[Y[ќКNВ€B‚€WЩ[™
\™Ы\Э
NВ€Ъ][™ЩЭЫ€HВ€›Ь‘^]
Э]\КNВџB‚‚ќ›ЪYЭZ\Э\ќ\

BћВ€[ќNВ‚€YЉY›ЫќЫШY
›Y[ќKЩ›ЫќH‹XЪЩљ[K
JB€В€›Ь”Ъ]ЭЫЉK•[X›HИШY›ЫќМHW€ЉNВ€B€YЉY›ЫќЫШY
K›Y[ќKЩ›Ыќ€‹XЪЩљ[K
JB€В€›Ь”Ъ]ЭЫЉK•[X›HИШY›ЫќМ€W€ЉNВ€B€YЉY›ЫќЫШY
‹›Y[ќKЩ›ЫќИ‹XЪЩљ[K
JB€В€›Ь”Ъ]ЭЫЉK•[X›HИШY›ЫќМИW€ЉNВ€B‚‚€›Ь•[Y\’[љ]

NВ‚€ЫЫќ›ЫЪ[љ]
ЉNВ€\WШЫЫќ›ЫК
NВ‚€[љ]ЭљY[Ы[Щ\К
NВ€YЉ]љY[ЧЬЩ]Ы[ЩJљY[Ы[Щ\КJB€В€›Ь”Ъ]ЭЫЉK•[X›HИЩ]љY[И[ЩN€	Y	YW€‹љY[Ы[Щ\Лљ™\ЛљY[Ы[Щ\Лќ”™\КNВ€B‚€›ЬЉHHИHЌMЋИJККB€В€™[ЫќX›VЪWHHNВ€BџB‚‚ќ›ЪYЭ\ќ\

BћВ€[ќNВ‚€љ[ќЉ‘љ[PШXЪ[™ИЮ\Э[H[љ]‹‹‹‹‹‹—ЉNВ€YЉZЧЪ[љ]

JB€В€љ[ќЉ‘[X›Y€ЉNВ€B€[ЩB€В€љ[ќЉ‘\ШX›Y€ЉNВ€B‚€ШYYЪШЫЬ™Qљ[J
NВ€ЫX\”Ш]™YШ[YJ
NВ‚€[љ]ЭљY[Ы[Щ\КJNВ€YЉ]љY[ЧЬЩ]Ы[ЩJљY[Ы[Щ\КJB€В€›Ь”Ъ]ЭЫЉK•[X›HИЩ]љY[И[ЩN€	Y	YW€‹љY[Ы[Щ\Лљ™\ЛљY[Ы[Щ\Лќ”™\КNВ€B‚€љ[ќЉ•[Y\€[љ]‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹—ЉNВ€›Ь•[Y\’[љ]

NВ€љ[ќЉ‘Ы™HW€ЉNВ‚€љ[ќЉ’[љ]X[^™HЫЭ[™‹‹‹‹‹‹‹‹‹‹‹‹—ЉNВ€YЉЫЭ[™Ъ[љ]

JB€В€YЉШYЬЬXЪX[ЬЫЭ[™К
JB€В€љ[ќЉ‘Ы™HW€ЉNВ€B€[ЩB€В€љ[ќЉ—€ЉNВ€B€YЉ\ЫЭ[™ЬЭ\ќЬ^XXЪК
JB€В€љ[ќЉ•Ш\›љ[™О€Ш[‰Э^HЫЭ[™W€ЉNВ€B€Р—ЬЩ]›Ы[YJР—УPTХT•“УШ]™Y]KњЫЭ[™›Ы
NВ€Р—ЬЩ]›Ы[YJР—Х“ТPСU“УШ]™Y]KњЫЭ[™›Ы
NВ€B€[ЩB€В€›Ь”Ъ]ЭЫЉK•[X›HИ[љ]X[^™HЫЭ[™—€ЉNВ€B‚€ЛИ[љ]€[њ]™XЫЬ™\‚€[љ]Ъ[њ]Ь™XЫЬ™\Љ
NВ‚€љ[ќЉ“ШY[™И›ЫќЛ‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹—ЉNВ€ШYШ[Щ›ЫќК
NВ€љ[ќЉ‘Ы™HW€ЉNВ‚€љ[ќЉ“ШY[™ИЬљ]\Л‹‹‹‹‹‹‹‹‹‹‹‹‹—ЉNВ€ШYЬЬXЪX[ЬЬљ]\К
NВ€љ[ќЉ‘Ы™HW€ЉNВ‚€љ[ќЉ“ШY[™И]™[Ь™\‹‹‹‹‹‹‹‹‹‹—ЉNВ€ШYЫ]™[Ь™\Љ
NВ€љ[ќЉ‘Ы™HW€ЉNВ‚€љ[ќЉ“ШY[™И[Щ[ЫЫњЭ[ќЛ‹‹‹‹‹—ЉNВ€ШYЫ[Щ[ШЫЫњЭ[ќК
NВ€љ[ќЉ‘Ы™HW€ЉNВ‚€љ[ќЉ“ШY[™ИШЬљ\Щ][™ЬЛ‹‹‹‹‹—ЉNВ€ШYЬШЬљ\ЬЩ][™К
NВ€љ[ќЉ‘Ы™HW€ЉNВ‚€љ[ќЉ“ШY[™ИШЬљ\Л‹‹‹‹‹‹‹‹‹‹‹‹‹—ЉNВ€ШYЬШЬљ\К
NВ€љ[ќЉ‘Ы™HW€ЉNВ‚€љ[ќЉ“ШY[™И[Щ[Л‹‹‹‹‹‹‹‹‹‹‹‹‹‹——€ЉNВ€ШYЫ[Щ[К
NВ‚€љ[ќЉ“Шљ™XЭ[™Ъ[™H[љ]‹‹‹‹‹‹‹‹‹‹—ЉNВ€YЉX[ШЧЩ[ќК
JB€В€›Ь”Ъ]ЭЫЉK“›Э[›ЭYЪY[[ЬћH›Ь€Ш[YHШљ™XЭИW€ЉNВ€B€љ[ќЉ‘Ы™HW€ЉNВ‚€љ[ќЉ“ШY[™ИY[ќKќ‹‹‹‹‹‹‹‹‹‹‹‹—ЉNВ€ШYЫY[ќWЭ

NВ€љ[ќЉ‘Ы™HW€ЉNВ‚€К‚€Ь]\И
KLЊЌ
H[Э™YH[њЫ][Ы€[™Y[ќHќ[Э[ЫњИИH[™Щ€H[™Ъ[™HњЭ\ќ\€ќ[Э[Ы‹€ќ]™Y›Ь™HHЫЫќ›Ы[љ]€ќ[Э[Ы€
™]™\ќYH›Ыќќ[Э[Ы€ИШY™Y›Ь™HШЬљ\КB€
‹В€љ[ќЉ“ШY[™И[њЫ][Ы‹‹‹‹‹‹‹‹‹‹—ЉNВ€Ш—Ъ[љ][њК
NВ€љ[ќЉ‘Ы™HW€ЉNВ‚€љ[ќЉ’[њ][љ]‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹—ЉNВ€ЫЫќ›ЫЪ[љ]
Ш]™Y]Kќ\ЩZ›ЮJNВ€\WШЫЫќ›ЫК
NВ€љ[ќЉ‘Ы™HW€ЉNВ‚€ЪY™Y€РPТWРђPТСФ“ХS‘В€љ[ќЉђШXЪ[™ИXЪЩЬ›Э[™Л‹‹‹‹‹‹‹‹‹—ЉNВ€ШXЪWШ[ШXЪЩЬ›Э[™К
NВ€љ[ќЉ‘Ы™HW€ЉNВ€Щ[™Y‚‚€љ[ќЉђЬ™X]H›[™[™ИX›\Л‹‹‹‹‹‹—ЉNВ€YЉ^[›Ь›X]OHVSЮ
B€В€Ь™X]WШ›[™ЭX›\ЧЮ
›[™X›\КNВ€B‚€›ЬЉHHИHPVФSФТV‘HИИJККB€В€™[ЫќX›VЪWHHNВ€B€љ[ќЉ‘Ы™HW€ЉNВ‚€YЉШ]™Y]K›ЩЫККИ€L
B€В€Ш]™Y]K›ЩЫИHВ€B‚€љ[ќЉ”Ш]™HЩ][™ЬИЫИ\‹‹‹‹‹‹‹‹‹—ЉNВ€Ш]™\Щ][™ЬК
NВ€љ[ќЉ‘Ы™HW€ЉNВ‚€Э\ќ\ЩЫ™HHNВ‚€љ[ќЉ——€ЉNВ‚џB‚‚‚‹ЛИKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKB‚‚‹ЛИ™]\›њИЫ€\њ›Ь‹LHЫ€\ШШ\Bљ[ќ^YЪYЉЪ\€
™љ[[[YK[ќ[ќK[ќ›ЬЪЪ\
BћВ€[љYЪY—Ъ[™›И
љ[™›ИHШ[ШКKЪ^™[ЩЉ
љ[™›КJNВ€ЧЬШЬ™Y[€
XЪШќY™™\€H•SВ€ЧЬШЬ™Y[€
ќ[\™ИHXЪЩЬ›Э[™В€[ќ™\Э[HNВ€LМ€Ю[ЭЬЫЭ[™В€LМ€\Э[YNВ€LМ€Z[\ЩXЫЫ™ОВ€LМ€[\[YK[\™]Э[YNИЛИ[\Ь\ћH]Ъ›Ь€[™Ш[YHЪY€^B‚€Ю[ЭЬЫЭ[™H
ЫЭ[™ЩЩ][ќ\ќ[

HOH‘‘‘‘‘‘‘ЉNВ€[\[YHHЭ[YNВ€[\™]Э[YHH™]Э[YNВ€Э[YHHВ€Z[\ЩXЫЫ™ИHВ€\Э[YHHВ€XЪЩЬ›Э[™H•SВ‚€YЉJ™\Э[H[љYЪY—ЫЬ[Љљ[[[YKXЪЩљ[K[™›КJJB€В€ЫЭИ^YЪY—Щ[™В€B‚€Ъ[JZ[™›ЛO™Ы™JB€В€YЉZ[\ЩXЫЫ™ИЏH[™›ЛOљ[™›ЦМK›™^њ[YJB€В€[љYЪY—ЩXЫЩWЩњ[YJ[™›КNВ€B€YЉJXЪШќY™™\€H[љYЪY—ЩЩ]ќY™™\Љ[™›КJJB€В€њ™XZОВ€B€Ьљ]\WШYЬШЬ™Y[ЉKXЪШќY™™\‹•S
NВ€YЉ[™›ЛO™њ[YHOH
B€В€™ШWЭќШZ]

NВ€\]J
NВ€B€[ЩB€В€\]JJNВ€B‚€YЉЮ[ЭЬЫЭ[™
B€В€Z[\ЩXЫЫ™И
ПHЫЭ[™ЩЩ][ќ\ќ[

NВ€B€[ЩB€В€Z[\ЩXЫЫ™И
ПH
Э[YHH\Э[YJH
€LИЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYYВ€B‚€\Э[YHHЭ[YNВ‚€YЉ[›ЬЪЪ\	‰€
›Э™]ЪЩ^\И	€
“QЧСTРИ“QЧРS–P•UУЉJJB€В€™\Э[HLNВ€њ™XZОВ€B€B‚‚њ^YЪY—Щ[™‚€[љYЪY—ШЫЬЩJ[™›КNВ€њ™YJ[™›КNВ€Э[YHH[\[YNВ€™]Э[YHH[\™]Э[YNВ€XЪЩЬ›Э[™H[\™ОВ€ЛЬЭ[™\™Ь[]JJNИЛТЬ]\И
KLЊЌ
H\ШX›YИљ^HњИ›Ь\ЬЭYB‚€YЉOH™\Э[
B€В€љ[ќЉ—•Ш\›љ[™Л[€\њ›Ь€ШШЭ\њ™YЪ[H^Z[™И[љ[X]YЪY€љ[H	Й\ЙЛ—€‹љ[[[YJNВ€B€™]\›€™\Э[В‚џB‚‚€ЪY™Y€СP“B‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊЌ‹LLL‚Љ‚Љ€™\Щ\ќ™H›ШЪЪ[™И^]ЩX›J
H™Z]љ[Ь€\ИHЬ\\€\›Э[™Љ€H™]\ШX›H[ЭљYHT\Л€™]\›€HЫ€ЫЫ\][Ы‹Ы€\њ›Ь‹Љ€Ь€LHЪ[€Ь™X]Ь‹Y[X›Y[њ][ќ\њќ\И^XXЪЛ‚Љ‹Вљ[ќ^]ЩX›JЫЫњЭЪ\€
њ][ќ›ЬЪЪ\
BћВ€[ќ™][HNВ€[ќЫЭ\ЩWЪYHLNВ€ЧЫ[ЭљYWЬ^XXЪИ
њ^XXЪИH•SВ€ЧЬШЬ™Y[€
њШЬ™Y[њЪЭЩњ[YHH•SВ‚€[ЭљYWЬ^XXЪЧЬЭЬШ[

NВ€ЫЭ\ЩWЪYH[ЭљYWЬЫЭ\ЩWЫШY
]SХ’QWУРQS‘ЧФХ‘PSJNВ€YЉЫЭ\ЩWЪY
HВ€™]\›€В€B€^XXЪИH[ЭљYWЬ^XXЪЧЬ^J€ЫЭ\ЩWЪY€SХ’QWРТS“‘SРUUЛ€Ш]™Y]K›]\ЪXЭ›Ы€ќYB€
NВ€YЉ\^XXЪКHВ€™][HВ€ЫЭИ]Z]В€B€YЉ[[ЭљYWЬ^XXЪЧЬЩ]Ъ[ќ\њќ\
^XXЪЛ[›ЬЪЪ\
JHВ€™][HВ€ЫЭИ]Z]В€B‚€К€YШXЮH^XXЪИ\Щ\ИHЩX“H\Ь^HЪ^™H[™\™Ш\™HUU€Э]]€
‹В€[ЭљYWЬ^XXЪЧЬЩ]ЭЪY
^XXЪЛSХ’QWФТV‘WУђUU‘JNВ€[ЭљYWЬ^XXЪЧЬЩ]ЪZYЪ
^XXЪЛSХ’QWФТV‘WУђUU‘JNВ‚€Ъ[J^XXЪЛOXЭ]™JHВ€[ќ[ќ\њќ\Ь™\]Y\ЭYВ€[ќ™\Щ[ќЩњ[YNВ‚€[њ]™Yњ™\Ъ
^\™XЬЭ]\ЛOњЭ]\КNВ€[ќ\њќ\Ь™\]Y\ЭYB€
›Э™]ЪЩ^\И	€
“QЧСTРИ“QЧРS–P•UУЉJHOHВ€YЉ[ќ\њќ\Ь™\]Y\ЭY	‰€^XXЪЛOљ[ќ\њќ\
HВ€™][HLNВ€B€[ЭљYWЬ^XXЪЧЭ\]J[ќ\њќ\Ь™\]Y\ЭY
NВ€YЉ\^XXЪЛOXЭ]™JHВ€њ™XZОВ€B€™\Щ[ќЩњ[YHH^XXЪЛO™њ[YWЩ\ќNВ€YЉ™\Щ[ќЩњ[YJHВ€YЉ[[ЭљYWЬ^XXЪЧЩ]ЧЭЧЮ]]Љ^XXЪЛOљ[™^
JHВ€™][HВ€њ™XZОВ€B€B€YЉ^XXЪЛOЭ\њ™[ќЩњ[YH	‰‚€[›ЬШЬ™Y[њЪЭ	‰‚€
›Э™]ЪЩ^\И	€“QЧФРФ‘QS”ТХ
JHВ€YЉ\ШЬ™Y[њЪЭЩњ[YH€ШЬ™Y[њЪЭЩњ[YKOќЪYOH^XXЪЛOЭ\њ™[ќЩњ[YKOќЪY€ШЬ™Y[њЪЭЩњ[YKOљZYЪOH^XXЪЛOЭ\њ™[ќЩњ[YKOљZYЪ
HВ€YЉШЬ™Y[њЪЭЩњ[YJHВ€њ™Y\ШЬ™Y[Љ	њШЬ™Y[њЪЭЩњ[YJNВ€B€ШЬ™Y[њЪЭЩњ[YHH[ШЬШЬ™Y[Љ€^XXЪЛOЭ\њ™[ќЩњ[YKOќЪY€^XXЪЛOЭ\њ™[ќЩњ[YKOљZYЪ€VSММ‚€
NВ€B€YЉШЬ™Y[њЪЭЩњ[YJHВ€]]—ЭЧЬ™ШЉ^XXЪЛOЭ\њ™[ќЩњ[YKШЬ™Y[њЪЭЩњ[YJNВ€ШЬ™Y[њЪЭ
ШЬ™Y[њЪЭЩњ[YK•S
NВ€B€B€\ЫY\
L
NВ€B‚њ]Z]‚€YЉШЬ™Y[њЪЭЩњ[YJHВ€њ™Y\ШЬ™Y[Љ	њШЬ™Y[њЪЭЩњ[YJNВ€B€YЉ^XXЪИ	‰€^XXЪЛO™Z[Y	‰€™][€
HВ€™][HВ€B€YЉ^XXЪИ	‰€^XXЪЛOXЭ]™JHВ€[ЭљYWЬ^XXЪЧЬЭЬ
^XXЪКNВ€B€Ъ[JЫЭ\ЩWЪYЏH	‰€[[ЭљYWЬЫЭ\ЩWЭ[›ШY
ЫЭ\ЩWЪY
JHВ€[ЭљYWЬ^XXЪЧЭ\]J
NВ€\ЫY\
L
NВ€B€™]\›€™][ВџB€Щ[™Y‚‚‚‚ќ›ЪY^\ШЩ[™JЪ\€
™љ[[[YJBћВ€Ъ\€
ќYЋВ€Ъ^™WЭЪ^™NВ€[ќЬОВ€Ъ\€
ЫЫ[X[™H•SВ€Ъ\€љY[Щљ[VУPVР•Q‘‘T—УS—NВ€[ќHHHЪЪ\Ы™HH›ЬЪЪ\HNВ€[ќЫЬЪ[™ИHЭ]\ОВ‚€\™У\Э\™Ы\ЭВ€Ъ\€\™ШќY–УPVРT‘ЧУS€
ИWHH€ЋВ‚€ЛИ™XYљ[B€YЉќY™™\—ЬZЩљ[Jљ[[[YK	ќY‹	њЪ^™JHOHJB€В€™]\›ЋВ€B‚€Э\њ™[ќШЩ[™HHљ[[[YNВ‚€ЛИ›ЭИ[ќ\њ™]HЫЫќ[ќИЩ€ќY€[™HћH[™B€ЬИHВ€Ъ[JќY–ЬЬЧJB€В€\њЩP\™ЬК	\™Ы\ЭќY€
ИЬЛ\™ШќYЉNВ€ЫЫ[X[™HСUРT‘К
NВ€YЉЫЫ[X[™МJB€В€YЉXЫЬЪ[™И	‰€ЭљXЫ\
ЫЫ[X[™›]\ЪXИЉHOH
B€В€]\ЪXКСUРT‘КJKСUТS•РT‘КЉK]Ы
СUРT‘ККJJNВ€B€[ЩHYЉXЫЬЪ[™И	‰€ЭљXЫ\
ЫЫ[X[™[љ[X][Ы€ЉHOH
B€В€ЭЬJљY[Щљ[KСUРT‘КJJNВ€HСUТS•РT‘КЉNВ€HHСUТS•РT‘ККNВ€ЪЪ\Ы™HHСUТS•РT‘К
NВ€›ЬЪЪ\HСUТS•РT‘КJNВ€Э]\ИH^YЪYЉљY[Щљ[KK›ЬЪЪ\
NВ€YЉЭ]\ИOHLH	‰€\ЪЪ\Ы™JB€В€ЫЬЪ[™ИHNВ€B€B€[ЩHYЉXЫЬЪ[™И	‰€ЭљXЫ\
ЫЫ[X[™ќљY[ИЉHOH
B€В€ЪY™Y€СP“B€ЭЬJљY[Щљ[KСUРT‘КJJNВ€ЪЪ\Ы™HHСUТS•РT‘КЉNВ€›ЬЪЪ\HСUТS•РT‘ККNВ€Э]\ИH^]ЩX›JљY[Щљ[K›ЬЪЪ\
NВ€YЉЭ]\ИOHLH	‰€\ЪЪ\Ы™JB€В€ЫЬЪ[™ИHNВ€B€[ЩHYЉЭ]\ИOH
B€В€љ[ќЉђ[€\њ›Ь€ШШЭ\њ™YЪ[€ћZ[™ИИ^HHљY[И	\Ч€‹љY[Щљ[JNВ€B€Щ[ЩB€љ[ќЉ”ЪЪ\[™ИљY[И	\ОИЩX“H^XXЪИ›ЭЭ\ЬќYЫ€\И]›Ь›W€ЉNВ€Щ[™Y‚€B€[ЩHYЉЭљXЫ\
ЫЫ[X[™њЪ[[ЩHЉHOH
B€В€ЫЭ[™ШЫЬЩWЫ]\ЪXК
NВ€B€B€ЛИЫИИ™^›Ы‹X›[љИ[™B€ЬИ
ПHЩ]™]У[™TЭ\ќ
ќY€
ИЬКNВ€B€YЉќY€OH•S
B€В€њ™YJќYЉNВ€ќY€H•SВ€B€Э\њ™[ќШЩ[™HH•SВ€›ЬЉHHИHPVФVQT”ОИJККB€В€^Y\–ЪWK™\ШX›ZЩ^\ИH^Y\–ЪWK›™]ЪЩ^\ИH^Y\–ЪWKњ^ZЩ^\ИHВ€BџB‚‚‚‚‹ЛИKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKB‚‚‚‚ќ›ЪYШ[Y[Э™\Љ
BћВ€[ќЫ™HHВ€Ъ\€\ќY™–УPVР•Q‘‘T—УS—HHИ€џNВ‚€]\ЪXК™]KЫ]\ЪXЛЩШ[Y[Э™\€‹
NВ‚€Э[YHHВ€ШЬ™Y[—ЬЭ]\ИHS—ФРФ‘QS—СРSQWУХ‘TЋВ‚€YЉЭ\ЭШЩ[™\ИOH•S
B€В€ЭЬJ\ќY™‹Э\ЭШЩ[™\КNВ€ЭШ]
\ќY™‹™Ш[Y[Э™\‹ќЉNВ€YЉ\ЭXЪЩљ[J\ќY™‹XЪЩљ[JHЏH
B€В€^\ШЩ[™J\ќY™ЉNВ€Ы™HHNВ€B€B€[ЩB€В€YЉ\ЭXЪЩљ[J™]KЬШЩ[™\ЛЩШ[Y[Э™\‹ќ‹XЪЩљ[JHЏH
B€В€^\ШЩ[™J™]KЬШЩ[™\ЛЩШ[Y[Э™\‹ќЉNВ€Ы™HHNВ€B‚€B‚€Ъ[JYЫ™JB€В€›ЫќЬљ[ќЉЬЭ›ZY
ЛЉ‘РSQHХ‘T€ЉJKLL
ИљY[Ы[Щ\Лќ”ЪYќЛЉ‘РSQHХ‘T€ЉJNВ€Ы™HH
Э[YH€ЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYY
€	‰€\ЫЭ[™Ь]Y\ћWЫ]\ЪXК•S•S
JNВ€Ы™HH
›Э™]ЪЩ^\И	€
“QЧСTРИ“QЧРS–P•UУЉJNВ€\]J
NВ€B‚€ШЬ™Y[—ЬЭ]\И	ЏH’S—ФРФ‘QS—СРSQWУХ‘TЋВџB‚‚‚‚ќ›ЪY[[YJ[ќYЬШЫЬ™JBћВ€[ќЫ™HHВ€[ќЬ[–МLHHИNВ€Z[ќЌЭШЫЬ™NВ€Ъ\€[YVУPVУђSQWУS€
ИWNВ€[ќKNВ€Ъ\€\ќY™–УPVР•Q‘‘T—УS—HHИ€џNВ€[ќЫЫHHNВ€[ќЫЫ€HЋВ‚€ШЬ™Y[—ЬЭ]\ИHS—ФРФ‘QS—ТSУС—СђSQNВ‚€YЉ\ШЫЬ™X™КB€В€ЛИ™]И[\›]]™HXЪЩЬ›Э[™]‚€YЉЭ\ЭљЩЬ™ИOH•S
B€В€ЭЬJ\ќY™‹Э\ЭљЩЬ™КNВ€ЭШ]
\ќY™‹љ\ШЫЬ™HЉNВ€ШYШXЪЩЬ›Э[™
\ќY™ЉNВ€B€[ЩB€В€ШYШШXЪYШXЪЩЬ›Э[™
™]KШ™ЬЛЪ\ШЫЬ™HЉNВ€B€B‚€YЉYЬШЫЬ™JB€В€›ЬЉHИ]™[Щ]ЦШЭ\њ™[ќЬЩ]K›X^^Y\њОИ
ККB€В€YЉ^Y\–ЬKњШЫЬ™H€Ш]™\ШЫЬ™KљYЪШЦОWJB€В€Ш]™\ШЫЬ™KљYЪШЦОWHH^Y\–ЬKњШЫЬ™NВ€ЭЬJШ]™\ШЫЬ™KљШЫЬ™[–ОWK^Y\–ЬK›[YJNВ€Ь[–ОWHHNВ‚€›ЬЉHHИHЏH	‰€^Y\–ЬKњШЫЬ™H€Ш]™\ШЫЬ™KљYЪШЦЪWNИKKJB€В€ШЫЬ™HHШ]™\ШЫЬ™KљYЪШЦЪWNВ€ЭЬJ[YKШ]™\ШЫЬ™KљШЫЬ™[–ЪWJNВ€Ш]™\ШЫЬ™KљYЪШЦЪWHH^Y\–ЬKњШЫЬ™NВ€ЭЬJШ]™\ШЫЬ™KљШЫЬ™[–ЪWK^Y\–ЬK›[YJNВ€Ь[–ЪWHHNВ€Ш]™\ШЫЬ™KљYЪШЦЪH
ИWHHШЫЬ™NВ€ЭЬJШ]™\ШЫЬ™KљШЫЬ™[–ЪH
ИWK[YJNВ€Ь[–ЪH
ИWHHВ€B€B€B€Ш]™RYЪШЫЬ™Qљ[J
NВ€B‚€Э[YHHВ‚€Ъ[JYЫ™JB€В€HHMЋВ€YЉZ\ШЫЬ™X™КB€В€›ЫќЬљ[ќЉЬЭ›ZY
ЛЉ’[Щ€[YHЉJKHH›ЫќZYЪ
КHHL
ИљY[Ы[Щ\Лќ”ЪYќЛЉ’[Щ€[YHЉJNВ€B‚€›ЬЉHHИHLИJККB€В€›ЫќЬљ[ќЉШЫЫ
Ь[–ЪWKЫЫJKH
ИљY[Ы[Щ\Лќ”ЪYќЬ[–ЪWK‰LљK€	\И‹H
ИKШ]™\ШЫЬ™KљШЫЬ™[–ЪWJNВ€›ЫќЬљ[ќЉШЫЫ
Ь[–ЪWKЫЫЉKH
ИљY[Ы[Щ\Лќ”ЪYќЬ[–ЪWK
ШЫЬ™Y›Ь›X]И‰LH€’]MЌ€‰H€’]MЌ
KШ]™\ШЫЬ™KљYЪШЦЪWJNВ€H
ПH
љY[Ы[Щ\Лќ”™\ИHљY[Ы[Щ\Лќ”ЪYќHM€HМЉHИLИЛЩ›ЫќЪZYЪЦЭЬ[–ЪWWH
ИЋВ€B‚€ЛИЬ]\И
KLЊЊКHYYH‘“QЧРS–P•UУ€€И^]H[Щ€[YHШЬ™Y[‚€\]J
NВ€Ы™HH
Э[YH€ЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYY
€
NВ€Ы™HH
›Э™]ЪЩ^\И	€
“QЧФХT•“QЧРS–P•UУ€“QЧСTРКJNВ€B€[›ШYШXЪЩЬ›Э[™

NВ€ШЬ™Y[—ЬЭ]\И	ЏH’S—ФРФ‘QS—ТSУС—СђSQNВџB‚‚‚‚‹ЛИ]™[ЫЫ\]YЪЭИ›Ыќ\ИЭY™‚ќ›ЪYЪЭШЫЫ\]J[ќќ[JBћВ€[ќЫ™HHВ€[ќK‹ОВ€LМ€ЫX\›Ыќ\ЦНHHИLLLLNВ€LМ€Y™X›Ыќ\ЦНHHИLLLLNВ€LМ€ќ\Ъ›Ыќ\ЦНHHИLLLLNВ€LМ€™^[YHHВ€LМ€љ[љ\Ъ[YHHВ€[ќЪ[€HВ€Ъ\€\ќY™–УPVР•Q‘‘T—УS—HHИ€џNВ‚€ШЬ™Y[—ЬЭ]\ИHS—ФРФ‘QS—ФТХЧРУУTUNВ‚€YЉЫЫ\]X™КB€В€ЛИ™]И[\›]]™HXЪЩЬ›Э[™]‚€YЉЭ\ЭљЩЬ™ИOH•S
B€В€ЭЬJ\ќY™‹Э\ЭљЩЬ™КNВ€ЭШ]
\ќY™‹ЫЫ\]HЉNВ€ШYШXЪЩЬ›Э[™
\ќY™ЉNВ€B€[ЩB€В€ШYШШXЪYШXЪЩЬ›Э[™
™]KШ™ЬЛШЫЫ\]HЉNВ€B€B‚€]\ЪXК™]KЫ]\ЪXЛШЫЫ\]H‹
NВ‚€›ЬЉHHИH]™[Щ]ЦШЭ\њ™[ќЬЩ]K›X^^Y\њОИJККB€В€YЉќ\ЪМHЏHH	‰€ЪЭЬќ\Ъ›Ыќ\ИOHJB€В€ќ\Ъ›Ыќ\ЦЪWHH›ЫX^ќ\Ъ™\Щ]ЪWH
€ШШ›Ыќ\Щ\ЦМ—NВ€B€YЉШШ›Ыќ\Щ\ЦМЧHOHJB€В€ЫX\›Ыќ\ЦЪWHHќ[H
€ШШ›Ыќ\Щ\ЦМNВ€B€[ЩB€В€ЫX\›Ыќ\ЦЪWHHШШ›Ыќ\Щ\ЦМNВ€B€Y™X›Ыќ\ЦЪWHH^Y\–ЪWK›]™\И
€ШШ›Ыќ\Щ\ЦМWNВ€B‚€\]J
NВ‚€Э[YHHВ€Ъ[JYЫ™JB€В€YЉ\ШЫЫ\]VНWJB€В€›ЫќЬљ[ќЉљY[Ы[Щ\ЛљЪYќ
ИШЫЫ\]VМKљY[Ы[Щ\Лќ”ЪYќ
ИШЫЫ\]VМWKЛЉ”ЭYЩH	ZHЫЫ\]HHЉKќ[JNВ€B€[ЩB€В€›ЫќЬљ[ќЉљY[Ы[Щ\ЛљЪYќ
ИШЫЫ\]VМKљY[Ы[Щ\Лќ”ЪYќ
ИШЫЫ\]VМWKЛЉ”ЭYЩHЉJNВ€›ЫќЬљ[ќЉљY[Ы[Щ\ЛљЪYќ
ИШЫЫ\]VМ—KљY[Ы[Щ\Лќ”ЪYќ
ИШЫЫ\]VМЧKЛ‰ZH‹ќ[JNВ€›ЫќЬљ[ќЉљY[Ы[Щ\ЛљЪYќ
ИШЫЫ\]VНKљY[Ы[Щ\Лќ”ЪYќ
ИШЫЫ\]VНWKЛЉђЫЫ\]HЉJNВ€B‚€›ЫќЬљ[ќЉљY[Ы[Щ\ЛљЪYќ
ИШ›Ыќ\ЦМKљY[Ы[Щ\Лќ”ЪYќ
ИШ›Ыќ\ЦМWKЉђЫX\€›Ыќ\ИЉJNВ€›ЬЉHH€H‹ИHОИH]™[Щ]ЦШЭ\њ™[ќЬЩ]K›X^^Y\њОИJКЛ€H€
И‹ИHИ
ИЉHYЉ^Y\–ЪWK›]™\И€
B€В€›ЫќЬљ[ќЉљY[Ы[Щ\ЛљЪYќ
ИШ›Ыќ\ЦЪ—KљY[Ы[Щ\Лќ”ЪYќ
ИШ›Ыќ\ЦЪЧK
ШЫЬ™Y›Ь›X]И‰L[H€€‰[HЉKЫX\›Ыќ\ЦЪWJNВ€B€›ЫќЬљ[ќЉљY[Ы[Щ\ЛљЪYќ
И›Ыќ\ЦМKљY[Ы[Щ\Лќ”ЪYќ
И›Ыќ\ЦМWKЉ“Y™H›Ыќ\ИЉJNВ€›ЬЉHH€H‹ИHОИH]™[Щ]ЦШЭ\њ™[ќЬЩ]K›X^^Y\њОИJКЛ€H€
И‹ИHИ
ИЉHYЉ^Y\–ЪWK›]™\И€
B€В€›ЫќЬљ[ќЉљY[Ы[Щ\ЛљЪYќ
И›Ыќ\ЦЪ—KљY[Ы[Щ\Лќ”ЪYќ
И›Ыќ\ЦЪЧK
ШЫЬ™Y›Ь›X]И‰L[H€€‰[HЉKY™X›Ыќ\ЦЪWJNВ€B€YЉќ\ЪМHЏHH	‰€ЪЭЬќ\Ъ›Ыќ\ИOHJB€В€›ЫќЬљ[ќЉљY[Ы[Щ\ЛљЪYќ
И›Ыќ\ЦМKљY[Ы[Щ\Лќ”ЪYќ
И›Ыќ\ЦМWKЉ”ќ\Ъ›Ыќ\ИЉJNВ€›ЬЉHH€H‹ИHОИH]™[Щ]ЦШЭ\њ™[ќЬЩ]K›X^^Y\њОИJКЛ€H€
И‹ИHИ
ИЉHYЉ^Y\–ЪWK›]™\И€
B€В€›ЫќЬљ[ќЉљY[Ы[Щ\ЛљЪYќ
И›Ыќ\ЦЪ—KљY[Ы[Щ\Лќ”ЪYќ
И›Ыќ\ЦЪЧK
ШЫЬ™Y›Ь›X]И‰L[H€€‰[HЉKќ\Ъ›Ыќ\ЦЪWJNВ€B€B€›ЫќЬљ[ќЉљY[Ы[Щ\ЛљЪYќ
ИШЫЬ™VМKљY[Ы[Щ\Лќ”ЪYќ
ИШЫЬ™VМWKЉ•Э[ШЫЬ™HЉJNВ€›ЬЉHH€H‹ИHОИH]™[Щ]ЦШЭ\њ™[ќЬЩ]K›X^^Y\њОИJКЛ€H€
И‹ИHИ
ИЉHYЉ^Y\–ЪWK›]™\И€
B€В€›ЫќЬљ[ќЉљY[Ы[Щ\ЛљЪYќ
ИШЫЬ™VЪ—KљY[Ы[Щ\Лќ”ЪYќ
ИШЫЬ™VЪЧK
ШЫЬ™Y›Ь›X]И‰LH€’]MЌ€‰H€’]MЌ
K^Y\–ЪWKњШЫЬ™JNВ€B‚€Ъ[JЭ[YH€™^[YJB€В€YЉYљ[љ\Ъ[YJB€В€љ[љ\Ъ[YHHЭ[YH
И
€ЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYYВ€B‚€›ЬЉHHИH]™[Щ]ЦШЭ\њ™[ќЬЩ]K›X^^Y\њОИJККB€В€YЉ^Y\–ЪWK›]™\И€
B€В€YЉЫX\›Ыќ\ЦЪWH€
B€В€YШЫЬ™JKL
NВ€ЫX\›Ыќ\ЦЪWHOHLВ€љ[љ\Ъ[YHHВ€B€[ЩHYЉY™X›Ыќ\ЦЪWH€
B€В€YШЫЬ™JKL
NВ€Y™X›Ыќ\ЦЪWHOHLВ€љ[љ\Ъ[YHHВ€B€[ЩHYЉќ\ЪМHЏHH	‰€ЪЭЬќ\Ъ›Ыќ\ИOHH	‰€
ќ\Ъ›Ыќ\ЦЪWH€
JB€В€YШЫЬ™JKL
NВ€ќ\Ъ›Ыќ\ЦЪWHOHLВ€љ[љ\Ъ[YHHВ€B€B€B‚€YЉYљ[љ\Ъ[YH	‰€J™^[YH	€MJJB€В€ЫЭ[™ЬЭЬЬШ[\JЪ[ЉNВ€Ъ[€HЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫИ‹Ш]™Y]K™Y™™XЭ›ЫИ‹L
NВ€B€™^[YJКОВ€B‚€YЉ›Э™]ЪЩ^\И	€
“QЧРS–P•UУ€“QЧСTРКJB€В€Ы™HHNВ€B€YЉљ[љ\Ъ[YH	‰€Э[YH€љ[љ\Ъ[YJB€В€Ы™HHNВ€B‚€\]J
NВ€B‚€ЛИY™[XZ[™\€Щ€ШЫЬ™K[Ш\ЩH^Y\€ЪЪ\ИЫЭ[ќ\‚€›ЬЉHHИH]™[Щ]ЦШЭ\њ™[ќЬЩ]K›X^^Y\њОИJККB€В€YЉ^Y\–ЪWK›]™\И€
B€В€YЉќ\ЪМHЏHH	‰€ЪЭЬќ\Ъ›Ыќ\ИOHJB€В€YШЫЬ™JKќ\Ъ›Ыќ\ЦЪWJNВ€B€YШЫЬ™JKЫX\›Ыќ\ЦЪWJNВ€YШЫЬ™JKY™X›Ыќ\ЦЪWJNВ€B€B€[›ШYШXЪЩЬ›Э[™

NВ‚€ШЬ™Y[—ЬЭ]\И	ЏH’S—ФРФ‘QS—ФТХЧРУУTUNВџB‚ќ›ЪYШ]™[]™[[™›К
BћВ€[ќNВ€ЧЬЩ]Щ[ќћH
њЩ]H]™[Щ]И
ИЭ\њ™[ќЬЩ]В€ЧЬШ]™[]™[
њШ]™HHШ]™[]™[
ИЭ\њ™[ќЬЩ]В‚€Ш]™KO™›YИHЩ]OњШ]™Y›YОВ€ЛИЫ‰ЭЪXЪИ›YИ\™HШ]™H[[™›Л›Ь€Ъ[\HЩЪXВ€›ЬЉHHИHЩ]O›X^^Y\њОИJККB€В€Ш]™KOњ]™\ЦЪWHH^Y\–ЪWK›]™\ОВ€Ш]™KOњЬ™Y]ЦЪWHH^Y\–ЪWKЬ™Y]ОВ€Ш]™KOњШЫЬ™\ЦЪWHH^Y\–ЪWKњШЫЬ™NВ€Ш]™KOњЬ]ЫљX[ЪWHH^Y\–ЪWKњЬ]ЫљX[В€Ш]™KOњЬ]Ы›\ЪWHH^Y\–ЪWKњЬ]Ы›\В€Ш]™KOњЩX\ќ[VЪWHH^Y\–ЪWKќЩX\ќ[NВ€Ш]™KOњЫЫЭ\›X\ЪWHH^Y\–ЪWKЫЫЭ\›X\В€Э›ЬJШ]™KOњ[YVЪWK^Y\–ЪWK›[YKPVУђSQWУSЉNВ€B€Ш]™KOЬ™Y]ИHЬ™Y]ОВ€Ш]™KO›]™[HЭ\њ™[ќЫ]™[В€Ш]™KOњЭYЩHHЭ\њ™[ќЬЭYЩNВ€Ш]™KOќЪXЪЬЩ]HЭ\њ™[ќЬЩ]В€Э›ЬJШ]™KO™[YKЩ]O›[YKPVУђSQWУS€HJNВ€Щ]ЬШ]™YШ[ЭЬЩ[XЭШ\™Э[Y[ќКЭ\њ™[ќЬЩ][ЭЬЩ[XЭШ\™ЬКNВџB‚ќ›ЪYћ]љXЭЬћ\ЬЩJ[ќ]H
™[ќ
BћВ€YЉ[ќ	‰‚€[ќOљ[њZ[€	€’S—ФRS—У“У‘H	‰‚€Y[ќO™[[™И	‰‚€J[ќO™X]ЬЭ]H	€PUФХUWСPQ
H	‰‚€Y[ќOњљ\Ъ[™И	‰‚€
[ќOљY[™И	€QS‘ЧРPХU‘JH	‰‚€[ќOњЬЪ][Ы‹ћHH[ќO\ЩH
B€В€[ќOќZЩXXЭ[Ы€H•SВ€[ќOќ™[ШЪ]KћH[ќOќ™[ШЪ]Kћ€HВ€[ќOљY[™ИHQS‘ЧУ“У‘NВ€[ќЬЩ]Ш[љ[J[ќS’WХ’PХФ–K
NВ€BџB‚њЭ]XИ›ЪYЪXЪЧЭљXЭЬћWЬЬЩJ
BћВ€Y€

[™Ш[YH	€JH	‰€]™[ШЫЫ\]YЩY™X][™ЧШ›ЬЬИ
B€В€[ќNВ€›ЬЉHHИHPVФVQT”ОИJККB€В€[ќ]J€\HЩ[ЋВ€Щ[€H^Y\–ЪWK™[ќВ€YЉЩ[€	‰€[Y[љ[JЩ[‹S’WХ’PХФ–JH	‰€Щ[‹O[љ[[ќ[HOHS’WХ’PХФ–JB€В€ћ]љXЭЬћ\ЬЩJЩ[ЉNВ€[™Ш[YHHВ€B€[ЩHY€
Щ[€	‰€Щ[‹O[љ[[ќ[HOHS’WХ’PХФ–H	‰€Щ[‹O[љ[X][™КB€В€[™Ш[YHHВ€B‚€Щ[€H\В€B€BџB‚љ[ќ^[]™[
Ъ\€
™љ[[[YJBћВ€[ќK\KШ[]™HHВ‚€Ъ[Ш[

NВ‚€Ш]™[]™[[™›К
NИЛИќ\Э[€Ш\ЩHЩHЬЩH[HYќ\€]™[\Ињ™YY‚€ШYЫ]™[
љ[[[YJNВ‚€YЉ[›ЬШ]™JB€В€Ш]™QШ[YQљ[J
NВ€Ш]™RYЪШЫЬ™Qљ[J
NВ€Ш]™TШЬљ\љ[J
NВ€B€›ЬШ]™HHВ‚€Э[YHHВ€™^[€HВ€Э[Щ\€H•SВ€љ\њЭ^Y\€H•SВ€\HH]™[Oќ\NВ‚€ЛИљ^\ИHЭ\ќ]™[^XЭ][™И\Эќ]Ы€ќYВ€›ЬЉHHИH]™[Щ]ЦШЭ\њ™[ќЬЩ]K›X^^Y\њОИJККB€В€YЉ^Y\–ЪWK›]™\И€
B€В€^Y\–ЪWK™\ШX›ZЩ^\ИH^Y\–ЪWK›™]ЪЩ^\ИH^Y\–ЪWKњ^ZЩ^\ИHВ€^Y\–ЪWKќЩX\ќ[HH]™[OњЩ]ЩX\В€^Y\–ЪWKљ›Ъ[љ[™ИHВ€^Y\–ЪWKљ\Ь^YYHNВ€Ь]Ыњ^Y\ЉJNВ€^Y\–ЪWK™[ќOњќ\Ъ›X^HВ€B€B‚€ЛЩ^XЭ]HHШЬљ\Ъ[€]™[Э\ќY€YЉШЬљ\Т\Т[љ]X[^™Y
	›]™[ЬШЬљ\
JB€В€ШЬљ\С^XЭ]J	›]™[ЬШЬљ\
NВ€B€YЉШЬљ\Т\Т[љ]X[^™Y
	Љ]™[O›]™[ЬШЬљ\
JJB€В€ШЬљ\С^XЭ]J	Љ]™[O›]™[ЬШЬљ\
JNВ€B‚€Ъ[JY[™Ш[YJB€В€\]JK
NВ‚€Y€
]™[O™›ЬЩWЩљ[љ\Ъ]™[
B€В€]™[ШЫЫ\]YHNВ€[™Ш[YHHNВ€]™[O™›ЬЩWЩљ[љ\Ъ]™[HВ€B€Y€
]™[O™›ЬЩWЩШ[Y[Э™\ЉB€В€ЛЩ[ќ]J€[\HЩ[ЋВ€›ЬЉHHИHPVФVQT”ОИJККHЛЫ]™[Щ]ЦШЭ\њ™[ќЬЩ]K›X^^Y\њВ€В€^Y\–ЪWK›]™\ИHВ€YЉ›ЬЪ\™JB€В€^Y\–ЪWKЬ™Y]ИHВ€B€[ЩB€В€Ь™Y]ИHВ€B€Y€
^Y\–ЪWK™[ќ
B‚BBB^В‚BBBBZЪ[Щ[ќ]J^Y\–ЪWK™[ќТSСS•UWХ’QССT—УU‘SСРSQWУХ‘TЉNВ‚BBBB\^Y\–ЪWK™[ќH•SВ‚BBB_B€ЛЬЩ[€H^Y\–ЪWK™[ќВ€ЛЬ^Y\—ЩYJ
NВ€B€ЛЬЩ[€H[\В€ЛЪЪ[Ш[

NВ€[™Ш[YHHNВ€]™[O™›ЬЩWЩШ[Y[Э™\€HВ€B€YЉ]™[ШЫЫ\]Y
B€В€[™Ш[YHH
Yљ[™[ќ
TWСS‘SVJH]™[Oќ\Hљ[™[ќ
TWСS‘U‘S
JNИЛИ[™ИЪ[€[[™[ZY\ИYHЬ€H›Ыќ\И]™[€ЪXЪЧЭљXЭЬћWЬЬЩJ
NВ€B€B€ЛЩ^XЭ]HHШЬљ\Ъ[€]™[љ[љ\ЪY€YЉШЬљ\Т\Т[љ]X[^™Y
	™[™]™[ЬШЬљ\
JB€В€ШЬљ\С^XЭ]J	™[™]™[ЬШЬљ\
NВ€B€YЉШЬљ\Т\Т[љ]X[^™Y
	Љ]™[O™[™]™[ЬШЬљ\
JJB€В€ШЬљ\С^XЭ]J	Љ]™[O™[™]™[ЬШЬљ\
JNВ€B€YЉ[›ЩY[Э]
B€В€YWЫЭ]

NВ€B‚€›ЬЉHHИH]™[Щ]ЦШЭ\њ™[ќЬЩ]K›X^^Y\њОИJККB€В€YЉ^Y\–ЪWK™[ќ
B€В€›ЫX^ќ\Ъ™\Щ]ЪWHH^Y\–ЪWK™[ќOњќ\Ъ›X^В€^Y\–ЪWKњЬ]ЫљX[H^Y\–ЪWK™[ќO™[™\™ЮWЬЭ]KљX[ШЭ\њ™[ќВ€^Y\–ЪWKњЬ]Ы›\H^Y\–ЪWK™[ќO™[™\™ЮWЬЭ]K›\ШЭ\њ™[ќВ€B€ЛИ™\Щ]€^Y\–ЪWKќЩX\ќ[HHВ€B‚€YЉ[]\ЪXЫЭ™\›\
B€В€ЫЭ[™ШЫЬЩWЫ]\ЪXК
NВ€B€ЫЭ[™ЬЭЬ[ЬШ[\J[ЩJNВ‚€[›ШYЫ]™[

NВ‚€ЛИ\™H[ћH^Y\њИ[]™OВ‚Y›ЬЉHHИHPVФVQT”ОИJККB€В€Y€
^Y\–ЪWK›]™\И€
B€В€Ш[]™HHNВ€њ™XZОВ€B€B‚€™]\›€

\HOH€	‰€[™Ш[YHOHЉHШ[]™JNВџB‚‹ЛИШ\ЪЩ^K[[Ы€€
™]ЫЫРH[љЫ›ЭЫЉB‹ЛИЊNKLKLВ‹ЛВ‹ЛИ›Ь€Щ[XЭШЬ™Y[‹€Ь]Ы€Ш[\H[ќ]H›Ь€^Y\—Ъ[™^‚њЭ]XИ[ќ]H
њЬ]Ы™^[\J[ќ^Y\—Ъ[™^
BћВ€Ъ\Љ€ФUУ—УSСSУђSQHH•SВ€ЫЫњЭ[ќФUУ—УSСSТS‘VHSСSТS‘VУ“У‘NВ‚€[ќ]BJ™^[\NВ‚\ЧЫ[Щ[
›[Щ[В‚\ЧЬЩ]Щ[ќћH
њЩ]В‚‚Y›Ш]ЬЧЮВ‚Y›Ш]ЬЧЮNВ‚Y›Ш]ЬЧЮЋВ‚Z[ќ\™XЭ[ЫЋВ‚BB‚\Щ]H]™[Щ]И
ИЭ\њ™[ќЬЩ]В‚‚KЛИЩ]Ь]Ы€]љXќ]\И[™Ь]Ы€[ќ]K‚‚\ЬЧЮH
›Ш]
\ЫY[ќVЬ^Y\—Ъ[™^VМNВ‚\ЬЧЮHHВ‚\ЬЧЮ€H
›Ш]
\ЫY[ќVЬ^Y\—Ъ[™^VМWNВ‚Y\™XЭ[Ы€HЬ\™XЭ[Ы–Ь^Y\—Ъ[™^NВ‚‚KЛИ™^Щ[XЭX›H[Щ[[€ЮXЫK€ЩIЫ\ЩH\В‚KЛИИXЪYHЪ]ЩHЬ]Ы‹‚‚[[Щ[H™^^Y\›[Щ[Љ•S^Y\—Ъ[™^
NВ‚‚Y^[\HHЬ]ЫЉЬЧЮЬЧЮ‹ЬЧЮK\™XЭ[Ы‹ФUУ—УSСSУђSQKФUУ—УSСSТS‘V[Щ[
NВ€‚KЛИЫЬH[Щ[	ЬИ[YHИ^Y\€›Ь\ќK‚‚\ЭЬJ^Y\–Ь^Y\—Ъ[™^K›[YK[Щ[O›[YJNВ‚‚KЛИY€ЫЫЬ€Щ[XЭ[Ы€\И[ЭЩY[™ЩHШ[ќ‚KЛИ^Y\њИЪ]Ш[YH[Щ[ИИ\ЩHY™™\™[ќ‚KЛИX\Л[€\ЩH™^X\[€ЮXЫK€Э\ќЪ\ЩB‚KЛИќ\ЭЫИЪ]Y][X\‚‚ZY€
ЫЫЭ\њЩ[XЭ	‰€
Щ]O››ЬШ[YH	€ЉJB‚^В‚B\^Y\–Ь^Y\—Ъ[™^KЫЫЭ\›X\H™^ЫЫЭ\›X\Љ[Щ[LK^Y\—Ъ[™^
NВ‚_B‚Y[ЩB‚^В‚B\^Y\–Ь^Y\—Ъ[™^KЫЫЭ\›X\HВ‚_B‚‚KЛИ\HX\ИЬ]Ы™Y[ќ]K‚€[ќЬЩ]ШЫЫЭ\›X\
^[\K^Y\–Ь^Y\—Ъ[™^KЫЫЭ\›X\
NВ€‚KЛИЫИH[ќ]HЫ›ЭЬИЭИ]Ш[YHИ™K‚‚Y^[\KOњЬ]Ыќ\HHФUУ—ХTWФVQT—ФСSPХВ€‚\™]\›€^[\NВџB‚‹ЛИШYШ]™YЩ[XЭШЬ™Y[‚њЭ]XИ›ЪYШYЬЩ[XЭЬШЬ™Y[—Ъ[™›КЧЬШ]™[]™[
њШ]™JBћВ€[ќHHВ€\™У\Э\™Ы\ЭВ€Ъ\€\™ШќY–УPVРT‘ЧУS€
ИWHH€ЋВ€Ъ\€
™љ[[[YHH€ЋИЛИ›Э\ЩYќ\Эќ\ЩY[ќИСUТS•РT‘К
B€Ъ\€
ЫЫ[X[™H€ЋИЛИ›Э\ЩYќ\Эќ\ЩY[ќИСUТS•РT‘К
B‚€ЛИШY[Щ[ЫШYВ€›ЬЉHHИHШ]™KOњЩ[XЭШYЫЭ[ќИJККB€В€ЧЫ[Щ[
ќ[\[Щ[В€ЫЫ[X[™HСUРT‘К
NВ‚€YЉXЫЫ[X[™XЫЫ[X[™МJHЫЫќ[ќYNВ€\њЩP\™ЬК	\™Ы\ЭШ]™KOњЩ[XЭШYЪWK\™ШќYЉNВ‚€[\[Щ[Hљ[™[Щ[
СUРT‘КJJNВ€Y€
[\[Щ[
B€В€\]WЫ[Щ[ЫШY›YК[\[Щ[СUТS•РT‘КЉJNВ€B€B‚€\њЩP\™ЬК	\™Ы\ЭШ]™KOњЩ[XЭ]\ЪXЛ\™ШќYЉNВ€ЫЫ[X[™HСUРT‘К
NВ€YЉЫЫ[X[™	‰€ЫЫ[X[™МJH]\ЪXКСUРT‘КJKСUТS•РT‘КЉK]Ы
СUРT‘ККJJNВ‚€\њЩP\™ЬК	\™Ы\ЭШ]™KOњЩ[XЭXЪЩЬ›Э[™\™ШќYЉNВ€ЫЫ[X[™HСUРT‘К
NВ€YЉЫЫ[X[™	‰€ЫЫ[X[™МJHШYШXЪЩЬ›Э[™
СUРT‘КJJNВ‚€™]\›ЋВџB‚љ[ќЩ[XЭ^Y\Љ[ќ
њ^Y\њЛЪ\€
™љ[[[YK[ќ\ЩTШ]™YШ[YJBћВ‚\ЧЫ[Щ[
ќ[\[Щ[В‚\ЧЫ[Щ[
›[Щ[Ы™]ЦУPVФVQT”ЧHHИ•SNВ‚Z[ќNВ‚Z[ќ^]HВ‚Z[ќ\ШШ\HHВ‚Z[ќY][Щ[XЭHВ‚][њЪYЫ™Y^][^HHВ‚Z[ќ^Y\њЧШќ\ЮHHВ‚Z[ќ^Y\њЧЬ™XYHHВ‚XЪ\€Эљ[™ЦУPVР•Q‘‘T—УS—HHИ€€NВ‚XЪ\€
ќY‹
ЫЫ[X[™В‚\Ъ^™WЭЪ^™HHВ‚\™Y™—ЭЬИHВ‚P\™У\Э\™Ы\ЭВ‚XЪ\€\™ШќY–УPVРT‘ЧУS€
ИWHH€ЋВ‚\ЧЬЩ]Щ[ќћH
њЩ]H]™[Щ]И
ИЭ\њ™[ќЬЩ]В‚\ЧЬШ]™[]™[
њШ]™HHШ]™[]™[
ИЭ\њ™[ќЬЩ]В‚Z[ќШYШЫЭ[ќHВ‚Z[ќШ]™YЬЩ[XЭЬШЬ™Y[€HВ‚Z[ќ\ЧЩљ\њЭЬЩ[XЭHNВ‚‚\Ш]™[]™[[™›К
NВ‚€ШЬ™Y[—ЬЭ]\ИHS—ФРФ‘QS—ФСSPХВ‚€Ъ[Ш[

NВ‚‚KЛИ[љ]X[^™H^Y\€Ъ^™Y\њ^\Л‚‚Y[ќ]H
™^[\VЬЩ]O›X^^Y\њЧNВ‚Z[ќ™XYVЬЩ]O›X^^Y\њЧNВ‚‚KЛИ[љ]X[^™H‚Y›Ь€
HHИHЩ]O›X^^Y\њОИJККB‚^В‚BY^[\VЪWHH•SВ‚B\™XYVЪWHHВ‚_B‚‚KЛИ[ЭИЩ[XЭИ	ШIИ\ИHљ\њЭЪ\€Щ€[ЭЬЩ[XЭ‚KЛИY€\™IЬИ	ШIИ[€\™H\И[ЭЬЩ[XЭ‚‚ZY€
X[ЭЬЩ[XЭШ\™ЬВ‚B_
[ЭЬЩ[XЭШ\™ЬЦМHOH	ШIВ‚BBI‰€[ЭЬЩ[XЭШ\™ЬЦМHOH	РIКJB‚^В‚B\™\Щ]Ь^XX›WЫ\Э
JNВ‚_B‚‚KЛИ™\Щ]Y[[ЬћH›Ь€^Y\€\њ^K‚‚[Y[\Щ]
^Y\‹Ъ^™[ЩЉ
њ^Y\ЉH
€PVФVQT”КNВ‚‚KЛИШYШ[YHЩ[XЭY[™HШ]™HШ[YH]Z[X›OВ‚ZY€
\ЩTШ]™YШ[YH	‰€Ш]™JB‚^В‚BZY€
Ш]™KOњЩ[XЭ›YКB‚B^В‚BB[ШYЬЩ[XЭЬШЬ™Y[—Ъ[™›КШ]™JNВ‚BB[ШYЬ^XX›WЫ\Э
‚BBBYЩ]ЬШ]™YШ[ЭЬЩ[XЭШ\™Э[Y[ќКЭ\њ™[ќЬЩ]
B‚BBJNВ‚BB\Ш]™YЬЩ[XЭЬШЬ™Y[€HNВ‚B_B‚_B‚‚KЛИX\љИљ\Ь^YY€›Ь€[^Y\њЛ‚‚Y›Ь€
HHИHЩ]O›X^^Y\њОИJККB‚^В‚B\^Y\–ЪWKљ\Ь^YYH^Y\њЦЪWNВ‚_B‚‚Y›Ь€
HHИHЩ]O›X^^Y\њОИJККB‚^В‚BZY€
Ш]™[]™[ШЭ\њ™[ќЬЩ]Kњ]™\ЦЪWH€
B‚B^В‚BBZ\ЧЩљ\њЭЬЩ[XЭHВ‚BBXњ™XZОВ‚B_B‚_B‚‚KЛИШY[™\HЩ[XЭ[Ы€^љ[K‚‚ZY€
љ[[[YH	‰€љ[[[YVМJB‚^В‚BZY€
ќY™™\—ЬZЩљ[Jљ[[[YK	ќY‹	њЪ^™JHOHJB‚B^В‚BBX›Ь”Ъ]ЭЫЉK‘Z[YИШY^Y\€Щ[XЭљ[H	Й\ЙИ‹љ[[[YJNВ‚B_B‚B]Ъ[H
ЬИЪ^™JB‚B^В‚BBT\њЩP\™ЬК	\™Ы\ЭќY€
ИЬЛ\™ШќYЉNВ‚BBXЫЫ[X[™HСUРT‘К
NВ‚BBZY€
ЫЫ[X[™	‰€ЫЫ[X[™МJB‚BB^В‚BBBZY€
ЭљXЫ\
ЫЫ[X[™›]\ЪXИЉHOH
B‚BBB^В‚BBBB[]\ЪXКСUРT‘КJKСUТS•РT‘КЉK]Ы
СUРT‘ККJJNВ‚BBBBKЛИРU‘B‚BBBB[][\ЭШ]Ь
Ш]™KOњЩ[XЭ]\ЪXЛЫЫ[X[™СUРT‘КJKСUРT‘КЉKСUРT‘ККK•S
NВ‚BBB_B‚BBBY[ЩHY€
ЭљXЫ\
ЫЫ[X[™[ЭЬЩ[XЭЉHOH
B‚BBB^В‚BBBB[ШYЬ^XX›WЫ\Э
ќY€
ИЬКNВ‚BBBB\Щ]ЬШ]™YШ[ЭЬЩ[XЭШ\™Э[Y[ќК‚BBBBBXЭ\њ™[ќЬЩ]‚BBBBBX[ЭЬЩ[XЭШ\™ЬВ‚BBBBJNВ‚BBB_B‚BBBY[ЩHY€
ЭљXЫ\
ЫЫ[X[™XЪЩЬ›Э[™ЉHOH
B‚BBB^В‚BBBB[ШYШXЪЩЬ›Э[™
СUРT‘КJJNВ‚BBBBKЛИРU‘B‚BBBB[][\ЭШ]Ь
Ш]™KOњЩ[XЭXЪЩЬ›Э[™ЫЫ[X[™СUРT‘КJK•S
NВ‚BBB_B‚BBBY[ЩHY€
ЭљXЫ\
ЫЫ[X[™›ШYЉHOH
B‚BBB^В‚BBBB][\[Щ[Hљ[™[Щ[
СUРT‘КJJNВ‚BBBBZY€
][\[Щ[
B‚BBBB^В‚BBBBB[ШYШШXЪYЫ[Щ[
СUРT‘КJKљ[[[YKСUТS•РT‘КЉJNВ‚BBBB_B‚BBBBY[ЩB‚BBBB^В‚BBBBB]\]WЫ[Щ[ЫШY›YК[\[Щ[СUТS•РT‘КЉJNВ‚BBBB_B‚BBBBKЛИРU‘B‚BBBBZY€
ШYШЫЭ[ќPVФСSPХУРQКB‚BBBB^В‚BBBBB[][\ЭШ]Ь
Ш]™KOњЩ[XЭШYЫШYШЫЭ[ќKЫЫ[X[™СUРT‘КJKСUРT‘КЉK•S
NВ‚BBBBB[ШYШЫЭ[ќ
КОВ‚BBBB_B‚BBB_B‚BBBY[ЩHY€
ЫЫ[X[™	‰€ЫЫ[X[™МJB‚BBB^В‚BBBB\љ[ќЉђЫЫ[X[™	Й\ЙИ\И›Э[™\њЭЫЩ[€љ[H	Й\ЙЧ€‹ЫЫ[X[™љ[[[YJNВ‚BBB_B‚BB_B‚‚BB\ЬИ
ПHЩ]™]У[™TЭ\ќ
ќY€
ИЬКNВ‚B_B‚B\Ш]™KOњЩ[XЭШYЫЭ[ќHШYШЫЭ[ќИЛИРU‘Hќ[X™\€Щ€РQЫЫ[X[™‚B\Ш]™KOњЩ[XЭ›YИHNВ‚‚BZY€
ќY€OH•S
B‚B^В‚BBYњ™YJќYЉNВ‚BBXќY€H•SВ‚B_B‚_B‚Y[ЩHЛИЪ]Э]Щ[XЭќ‚^В‚BZY€
\ЧЩљ\њЭЬЩ[XЭ
\ЪЪ\Щ[XЭМVМH	‰€\Щ]O››ЬЩ[XЭ
JHЛИ›ИЩ[XЭ\ИЪЪ\Щ[XЭЪ]Э][Y\В‚B^В‚BBYY][Щ[XЭHNИЛИ›Ь›X[Щ[XЭЬ€ЪЪ\Щ[XЭЫ›ЬЩ[XЭИHOH›Ь›X[Щ[XЭ‚B_B‚‚BZY€
[›ЬЪ\™JB‚B^В‚BBXЬ™Y]ИHУУ•S•QTОВ‚B_B‚BY[ЩH›Ь€
HHИHЩ]O›X^^Y\њОИJККB‚B^В‚BBZY€
^Y\њЦЪWJB‚BB^В‚BBB\^Y\–ЪWKЬ™Y]ИHУУ•S•QTОВ‚BB_B‚B_B‚‚BZY€
ЪЪ\Щ[XЭМVМHЩ]O››ЬЩ[XЭ
B‚B^В‚BBY›Ь€
HHИHЩ]O›X^^Y\њОИJККB‚BB^В‚BBBZY€
\^Y\њЦЪWJB‚BBB^В‚BBBBXЫЫќ[ќYNВ‚BBB_B‚BBB\Э›ЬJ^Y\–ЪWK›[YKЪЪ\Щ[XЭЪWKPVУђSQWУSЉNВ‚‚BBBZY€
Y][Щ[XЭ
B‚BBB^В‚BBBB\^Y\–ЪWK›]™\ИHVQT—УU‘TОВ‚BBBBZY€
JЫШ[ШЫЫ™љYЛЪX]И	€ТPUУФSУ”ЧРФ‘QUЧРPХU‘JJB‚BBBB^В‚BBBBBZY€
›ЬЪ\™JB‚BBBBB^В‚BBBBBBKK\^Y\–ЪWKЬ™Y]ОВ‚BBBBB_B‚BBBBBY[ЩB‚BBBBB^В‚BBBBBBKKXЬ™Y]ОВ‚BBBBB_B‚BBBB_B‚BBB_B‚BBBY[ЩB‚BBB^В‚BBBB\^Y\–ЪWK›]™\ИHШ]™[]™[ШЭ\њ™[ќЬЩ]Kњ]™\ЦЪWNВ‚BBBB\^Y\–ЪWKњШЫЬ™HHШ]™[]™[ШЭ\њ™[ќЬЩ]KњШЫЬ™\ЦЪWNВ‚BBBBZY€
›ЬЪ\™JH^Y\–ЪWKЬ™Y]ИHШ]™[]™[ШЭ\њ™[ќЬЩ]KњЬ™Y]ЦЪWNВ‚BBBBY[ЩHЬ™Y]ИHШ]™[]™[ШЭ\њ™[ќЬЩ]KЬ™Y]ОВ‚BBB_B‚BB_B‚‚BB\ШЬ™Y[—ЬЭ]\И	ЏH’S—ФРФ‘QS—ФСSPХВ‚‚BB\™]\›€NВ‚B_B‚‚BZY€
\Ш]™YЬЩ[XЭЬШЬ™Y[ЉB‚B^В‚BBZY€
[›ШЪШ™И	‰€›Ыќ\КB‚BB^В‚BBBKЛИ™]И[\›]]™HXЪЩЬ›Э[™]‚‚BBBZY€
Э\ЭљЩЬ™ИOH•S
B‚BBB^В‚BBBB\ЭЬJЭљ[™ЛЭ\ЭљЩЬ™КNВ‚BBBB\ЭШ]
Эљ[™Лќ[›ШЪШ™ИЉNВ‚BBBB[ШYШXЪЩЬ›Э[™
Эљ[™КNВ‚BBB_B‚BBBY[ЩB‚BBB^В‚BBBB[ШYШШXЪYШXЪЩЬ›Э[™
™]KШ™ЬЛЭ[›ШЪШ™ИЉNВ‚BBB_B‚BB_B‚BBY[ЩB‚BB^В‚BBBKЛИ™]И[\›]]™HXЪЩЬ›Э[™]‚‚BBBZY€
Э\ЭљЩЬ™ИOH•S
B‚BBB^В‚BBBB\ЭЬJЭљ[™ЛЭ\ЭљЩЬ™КNВ‚BBBB\ЭШ]
Эљ[™ЛњЩ[XЭЉNВ‚BBBB[ШYШXЪЩЬ›Э[™
Эљ[™КNВ‚BBB_B‚BBBY[ЩB‚BBB^В‚BBBB[ШYШШXЪYШXЪЩЬ›Э[™
™]KШ™ЬЛЬЩ[XЭЉNВ‚BBB_B‚BB_B‚BBZY€
[]\ЪXК™]KЫ]\ЪXЛЫY[ќH‹K
JB‚BB^В‚BBB[]\ЪXК™]KЫ]\ЪXЛЬ™[Z^‹K
NВ‚BB_B‚B_B‚_B‚‚Y›Ь€
HHИHЩ]O›X^^Y\њОИJККB‚^В‚BZY€
^Y\њЦЪWJB‚B^В‚BBY^[\VЪWHHЬ]Ы™^[\JJNВ‚BB\^Y\–ЪWKњ^ZЩ^\ИHВ‚BBZY€
Y][Щ[XЭ
B‚BB^В‚BBB\^Y\–ЪWK›]™\ИHVQT—УU‘TОВ‚BBBZY€
JЫШ[ШЫЫ™љYЛЪX]И	€ТPUУФSУ”ЧРФ‘QUЧРPХU‘JJB‚BBB^В‚BBBBZY€
›ЬЪ\™JB‚BBBB^В‚BBBBBKK\^Y\–ЪWKЬ™Y]ОВ‚BBBB_B‚BBBBY[ЩB‚BBBB^В‚BBBBBKKXЬ™Y]ОВ‚BBBB_B‚BBB_B‚BB_B‚BBY[ЩB‚BB^В‚BBB\^Y\–ЪWK›]™\ИHШ]™[]™[ШЭ\њ™[ќЬЩ]Kњ]™\ЦЪWNВ‚BBB\^Y\–ЪWKњШЫЬ™HHШ]™[]™[ШЭ\њ™[ќЬЩ]KњШЫЬ™\ЦЪWNВ‚BBBZY€
›ЬЪ\™JH^Y\–ЪWKЬ™Y]ИHШ]™[]™[ШЭ\њ™[ќЬЩ]KњЬ™Y]ЦЪWNВ‚BBBY[ЩHЬ™Y]ИHШ]™[]™[ШЭ\њ™[ќЬЩ]KЬ™Y]ОВ‚BB_B‚B_B‚_B‚‚WЭ[YHHВ‚‚KЛИЭ^H[€Щ[XЭ[Ы€[ќ[\ШШ\HЬ€^]‚‚KЛИ‚KЛИ^]H[^Y\њИ™XYH
Щ[XЭY
H[™^][^H^\™Y‚‚KЛИ\ШШ\HH\ШШ\HЩ^H™\ЬЩY‚‚]Ъ[H
J^]\ШШ\JJB‚^В‚B\^Y\њЧШќ\ЮHHВ‚B\^Y\њЧЬ™XYHHВ‚‚BKЛИЫЬ›ЭYЪ^Y\њЛ‚‚BY›Ь€
HHИHЩ]O›X^^Y\њОИJККB‚B^В‚BBKЛИЭ\њ™[ќ^Y\€[™^›ЭY]Щ[XЭYВ‚BBZY€
\™XYVЪWJB‚BB^В‚BBBB‚BBBKЛИ\И\ИЪ\™HЩH™\Щ[ќ^Y\€Щ[XЭ[ЫњЛ€HЩЪXИ\ИЫ™В‚BBBKЛИ[™H]HY\ЬЮKЫИќXЪЫH\H\ЪXШ[KЩHШ[ќИЬ]Ы‚‚BBBKЛИ[€^[\H[ќ]HИЩ]Э\ќY[™]^[\H[ќ]B‚BBBKЛИ\ИЪ]^Y\€ЩY\ИЫ€HЩ[XЭ[Ы€ШЬ™Y[‹€[€ЩHЭЪ]Ъ‚BBBKЛИ]И[Щ[ШЫЫЬ‹Ш[љ[X][Ы€\ЩYЫ€HЪ]X][Ы€[™^Y\‚‚BBBKЛИ[њ]‚‚BBBKЛВ‚BBBKЛИK€Y€[€^[\H[ќ]H^\ЭИ[™\И^Z[™ИH[њЪ][Ы€‚BBBKЛИ[€ЩK‹‹‚‚BBBKЛВ‚BBBKЛИJHИ›Э[™ИY€H[љ[X][Ы€\Ы‰Эљ[љ\ЪY‚‚BBBKЛВ‚BBBKЛИЉHY€]TИљ[љ\ЪY‹‹‚‚BBBKЛВ‚BBBKЛИKHK€Y€H[љ[X][Ы€\ИS’WФСSPХS‹[€^HS’WФСSPХ‚‚BBBKЛИ‚BBBKЛИKH‹€Y€[љX[]Ы€\ИS’WФСSPХХU[€ЩHЭЪ]ЪИ™]И‚BBBKЛИ[Щ[
Y€]Z[X›JK‚‚BBBKЛВ‚BBBKЛИ‹€Y€^Y\€\Ы‰Э^YYY]\ИЫЫYHЬ™Y]ИЬ€‚BBBKЛИШ[€]Ињ›ЫHЬ™Y]ЫЫ[™™\ЬЩY[ћHXЭ[Ы€ќ]Ы‹‚BBBKЛИ[€ЩIЫX[Ъ]Z\€Ь™Y]ЫЫ[™Ь]Ы€B‚BBBKЛИљ\њЭ^[\H
Щ[XЭX›H[Щ[™]љY]КK€]љ[™И[€‚BBBKЛИ^[\HЬ]Ы™Y[ЫИ[И\ИH^Y\€\ИЫЫ\]Y‚BBBKЛИ\ИЭ\[™ЫИ]	ЬИТИИќ[€XЭ[ЫњИњ›ЫH[ћHЩ€‚BBBKЛИHЭ\њЛ‚‚BBBKЛВ‚BBBKЛИЛ€Y€H^Y\€™\ЬЩYYќЬ€љYЪ[њЭXY[™\™IЬВ‚BBBKЛИ[€^[\HЬ]Ы™Y[€ЩHљ[™H™]љ[Э\ЛЫ™^‚BBBKЛИЪ\XЭ\€[€[™K[™™XЫЬ™]ИH\љXX›K€[€ЩB‚BBBKЛИЩYHY€^[\H\ИS’WФСSPХS‹€Y€]Щ\Ы‰ЭЩHЭЪ]Ъ‚BBBKЛИИ™]И[Щ[€Y€]Щ\Л^HS’WФСSPХ‚‚BBBKЛВ‚BBBKЛИ€Y€H^Y\€™\ЬЩ\И\Ь€ЭЫ‹ЩH]™H[€^[\H‚BBBKЛИЬ]Ы™Y[™ЫЫЭ\њЩ[XЭ\И[X›Y[€ЮXЫHИB‚BBBKЛИ[Щ[	ЬИ™]љ[Э\ЛЫ™^ЫЫЬ€Щ]ЪЪXЩK‚‚BBBKЛВ‚BBBKЛИK€Y€H^Y\€™\ЬЩ\И[ћHXЭ[Ы€ќ]Ы€[™ЩH]™B‚BBBKЛИ[€^[\HЬ]Ы™Y[€ЩHX\љИH^Y\‰ЬИ™XYH[^H‚BBBKЛИ›YЛ[™Э[[YK€ЩYHH\™[ќЩЪXИ›ШЪИ›Ь€‚BBBKЛИЩ[XЭ[Ы€[^H	€^]]Z[Л€\И\ИH^Y\‚‚BBBKЛИXZЪ[™ИZ\€Щ[XЭ[Ы€ЪЪXЩK‚‚‚BBBKЛИ^[\H^\ЭИ[™Щ[XЭ[њЪ][Ы€[љ[X][ЫЏВ‚BBBZY€
^[\VЪWH‚BBBBI‰€
^[\VЪWKO[љ[[ќ[HOHS’WФСSPХS€^[\VЪWKO[љ[[ќ[HOHS’WФСSPХХU
JB‚BBB^В‚BBBBKЛИY€Э[[љ[X][™И[€И›Э[™Л€]H[њЪ][Ы€љ[љ\Ъ‚‚BBBBZY€
^[\VЪWKO[љ[X][™КB‚BBBB^В‚BBBB_B‚BBBBY[ЩB‚BBBB^В‚BBBBBKЛИ[њЪ][Ы€ИЩ[XЭ[љ[X][Ы‹‚‚BBBBBZY€
^[\VЪWKO[љ[[ќ[HOHS’WФСSPХSЉB‚BBBBB^В‚BBBBBBY[ќЬЩ]Ш[љ[J^[\VЪWKS’WФСSPХ
NВ‚BBBBB_B‚‚BBBBBKЛИ[њЪ][Ы€њ›ЫHЩ[XЭ
^Y\€Щ[XЭY[›Э\€[Щ[[™B‚BBBBBKЛИЩ[XЭЭ][њЪ][Ы€\И›ЭИљ[љ\ЪY
K€™\X]Щ€YќЬљYЪЩ^H‚BBBBBKЛИЩЪXИ™[ЭИ[™›ШX›H™YYИЫЫњЫЫY][Ы‹‚‚BBBBBZY€
^[\VЪWKO[љ[[ќ[HOHS’WФСSPХХU	‰€[Щ[Ы™]ЦЪWJB‚BBBBB^В‚BBBBBBKЛИ\H™]И[Щ[‚‚BBBBBBY[ќЬЩ]Ы[Щ[
^[\VЪWK[Щ[Ы™]ЦЪWKO›[YK
NВ‚BBBBBB[[Щ[Ы™]ЦЪWHH•SВ‚‚BBBBBBKЛИЫЬH^[\H[Щ[[YHИ^Y\€[YH\љXX›K‚‚BBBBBB\ЭЬJ^Y\–ЪWK›[YK^[\VЪWKO›[Щ[O›[YJNВ‚‚BBBBBBKЛИY€ЫЫЬњЩ[XЭ\И[X›Y[™›ЬШ[YH€\И[X›YЪЪ\В‚BBBBBBKЛИЭ\ќ]™^]ZX[›HЫЫЬ€ЮXЫK€Э\ќЪ\ЩHќ\ЭЭ\ќ‚BBBBBBKЛИЪ]Y][ЫЫЬ€Щ]

K‚‚BBBBBBZY€
ЫЫЭ\њЩ[XЭ	‰€
Щ]O››ЬШ[YH	€ЉJB‚BBBBBB^В‚BBBBBBB\^Y\–ЪWKЫЫЭ\›X\H™^ЫЫЭ\›X\Љ^[\VЪWKO›[Щ[LKJNВ‚BBBBBB_B‚BBBBBBY[ЩB‚BBBBBB^В‚BBBBBBB\^Y\–ЪWKЫЫЭ\›X\HВ‚BBBBBB_B‚‚BBBBBBKЛИ\HЫЫЬ€Щ]‚‚BBBBBBY[ќЬЩ]ШЫЫЭ\›X\
^[\VЪWK^Y\–ЪWKЫЫЭ\›X\
NВ‚BBBBB_B‚BBBB_B‚BBB_B‚BBBY[ЩHY€
\^Y\–ЪWKљ\Ь^YY‚BBBBI‰€
›ЬЪ\™HЬ™Y]И€
B‚BBBBI‰€
^Y\–ЪWK›™]ЪЩ^\И	€“QЧРS–P•UУЉJB‚BBB^В‚‚BBBBKЛИ›ЭИ\И^Y\€\И^YY‚‚BBBB\^Y\њЦЪWHH^Y\–ЪWKљ\Ь^YYHNВ‚BBBBKЛЬљ[ќЉ‰Y	Y	Y€‹K^Y\–ЪWK›]™\Л[[YYX]VЪWJNВ‚‚BBBBKЛИ›ЬЪ\™HYX[њИXXЪ^Y\€\ИZ\€ЭЫ€Ь™Y]ЫЫ‚‚BBBBZY€
›ЬЪ\™JB‚BBBB^В‚BBBBB\^Y\–ЪWKЬ™Y]ИHУУ•S•QTОВ‚BBBB_B‚‚BBBBKЛИЬ™Y]ИЪX]H[™љ[љ]HЬ™Y]Л€Y€]	ЬИ›Э[X›Y‚BBBBKЛИ[€YXЭHЬ™Y]‚‚BBBBZY€
JЫШ[ШЫЫ™љYЛЪX]И	€ТPUУФSУ”ЧРФ‘QUЧРPХU‘JJB‚BBBB^В‚BBBBBZY€
›ЬЪ\™JB‚BBBBB^В‚BBBBBBKK\^Y\–ЪWKЬ™Y]ОВ‚BBBBB_B‚BBBBBY[ЩB‚BBBBB^В‚BBBBBBKKXЬ™Y]ОВ‚BBBBB_B‚BBBB_B‚‚BBBBKЛИЪ]™H^Y\€Y][ќ[X™\€Щ€]™\ЛЬ]Ы‚‚BBBBKЛИ^[\H[Щ[[™Ш[Щ[HЩ^H›YЛ‚‚BBBB\^Y\–ЪWK›]™\ИHVQT—УU‘TОВ‚BBBBY^[\VЪWHHЬ]Ы™^[\JJNВ‚BBBB\^Y\–ЪWKњ^ZЩ^\ИHВ‚‚BBBBKЛИ^HЫЭ[™Y™™XЭ‚‚BBBBZY€
ЫШ[ЬШ[\WЫ\Э™Y\ЏH
B‚BBBB^В‚BBBBB\ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ‚BBBB_B‚BBB_B‚BBBY[ЩHY€

^Y\–ЪWK›™]ЪЩ^\И	€“QЧРS–P•UУЉH	‰€^[\VЪWJHЛТЬ]\И
KLKLЊJH[Э™YH[ћXќ]Ы€€ЫЩHИ™Y›Ь™HЩ€H›YќЬљYЪ€ЫЩHИљ^HќYИ]XZЩ\И›ИЪ\XЭ\€ЪЬЩ[€Ъ[€›Э\™H™\ЬЩYЩЩ]\‚‚BBB^В‚BBBBZY€
ЫШ[ЬШ[\WЫ\Э™Y\М€ЏH
B‚BBBB^В‚BBBBB\ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\М‹Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ‚BBBB_B‚BBBBKЛИX^H[ЭHXЪЩYYHB‚BBBBZY€
[Y[љ[J^[\VЪWKS’WФPТКJB‚BBBB^В‚BBBBBY[ќЬЩ]Ш[љ[J^[\VЪWKS’WФPТЛ
NВ‚BBBB_B‚BBBBY^[\VЪWKOњЭ[[YHHЭ[YH
ИЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYY
€ЋВ‚BBBB\™XYVЪWHHNВ‚BBB_B‚BBBY[ЩHY€
^Y\–ЪWK›™]ЪЩ^\И	€
“QЧУSХ‘SQ•“QЧУSХ‘T’QТ
H	‰€^[\VЪWJB‚BBB^В‚BBBBKЛИЪ]™H^Y\€H™YYXЪИЫЭ[™‚‚BBBBZY€
ЫШ[ЬШ[\WЫ\Э™Y\ЏH
B‚BBBB^В‚BBBBB\ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ‚BBBB_B‚‚BBBBKЛИЩ][Щ[[€\ЩHљYЪ›ЭЛ‚‚BBBBKЛИ]	ЬИЩ]H™]И[Щ[€YќЩ^HH™]љ[Э\И[Щ[‚BBBBKЛИ[€ЮXЫK€љYЪЩ^HH™^‚‚BBBBZY€

^Y\–ЪWK›™]ЪЩ^\И	€“QЧУSХ‘SQ•
JB‚BBBB^В‚BBBBB[[Щ[Ы™]ЦЪWHH™]њ^Y\›[Щ[Љ^[\VЪWKO›[Щ[JNВ‚BBBB_B‚BBBBY[ЩB‚BBBB^В‚BBBBB[[Щ[Ы™]ЦЪWHH™^^Y\›[Щ[Љ^[\VЪWKO›[Щ[JNВ‚BBBB_B‚‚BBBBKЛИИЩH]™HHЩ[XЭЭ][њЪ][ЫЏИY€ЫИ^H]\™K€‚BBBBKЛИЭ\ќЪ\ЩHЭЪ]ЪИ™]И[Щ[€‚BBBBZY€
[Y[љ[J^[\VЪWKS’WФСSPХХU
JB‚BBBB^ВBBBBBB‚BBBBBY[ќЬЩ]Ш[љ[J^[\VЪWKS’WФСSPХХU
NВBBBB‚BBBB_B‚BBBBY[ЩB‚BBBB^В‚BBBBBKЛИ\H™]И[Щ[‚‚BBBBBY[ќЬЩ]Ы[Щ[
^[\VЪWK[Щ[Ы™]ЦЪWKO›[YK
NВ‚BBBBB[[Щ[Ы™]ЦЪWHH•SВ‚‚BBBBBKЛИЫЬH^[\H[Щ[[YHИ^Y\€[YH\љXX›K‚‚BBBBB\ЭЬJ^Y\–ЪWK›[YK^[\VЪWKO›[Щ[O›[YJNВ‚‚BBBBBKЛИY€ЫЫЬњЩ[XЭ\И[X›Y[™›ЬШ[YH€\И[X›YЪЪ\В‚BBBBBKЛИЭ\ќ]™^]ZX[›HЫЫЬ€ЮXЫK€Э\ќЪ\ЩHќ\ЭЭ\ќ‚BBBBBKЛИЪ]Y][ЫЫЬ€Щ]

K‚‚BBBBBZY€
ЫЫЭ\њЩ[XЭ	‰€
Щ]O››ЬШ[YH	€ЉJB‚BBBBB^В‚BBBBBB\^Y\–ЪWKЫЫЭ\›X\H™^ЫЫЭ\›X\Љ^[\VЪWKO›[Щ[LKJNВ‚BBBBB_B‚BBBBBY[ЩB‚BBBBB^В‚BBBBBB\^Y\–ЪWKЫЫЭ\›X\HВ‚BBBBB_B‚‚BBBBBKЛИ\HЫЫЬ€Щ]‚‚BBBBBY[ќЬЩ]ШЫЫЭ\›X\
^[\VЪWK^Y\–ЪWKЫЫЭ\›X\
NВ‚BBBB_BBBBBB‚BBB_B‚BBBY[ЩHY€
^Y\–ЪWK›™]ЪЩ^\И	€
“QЧУSХ‘UT“QЧУSХ‘QХУЉH	‰€ЫЫЭ\њЩ[XЭ	‰€^[\VЪWJB‚BBB^В‚BBBB\^Y\–ЪWKЫЫЭ\›X\H

^Y\–ЪWK›™]ЪЩ^\И	€“QЧУSХ‘UT
HИ™^ЫЫЭ\›X\€€™]ЫЫЭ\›X\ЉJ^[\VЪWKO›[Щ[^Y\–ЪWKЫЫЭ\›X\JNВ‚BBBBY[ќЬЩ]ШЫЫЭ\›X\
^[\VЪWK^Y\–ЪWKЫЫЭ\›X\
NВ‚BBB_B‚BB_B‚BBY[ЩHY€
™XYVЪWHOHJB‚BB^В‚BBBZY€


][Y[љ[J^[\VЪWKS’WФPТКH^[\VЪWKO›[Щ[]K[љ[X][Ы–РS’WФPТЧKO›ЫЬ›[ЩJH	‰€Э[YH€^[\VЪWKOњЭ[[YJHY^[\VЪWKO[љ[X][™КB‚BBB^В‚BBBB\™XYVЪWHHЋВ‚BBBBY^][^HHЭ[YH
ИЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYYВ‚BBB_B‚BB_B‚BBY[ЩHY€
™XYVЪWHOHЉB‚BB^В‚BBBY›ЫќЬљ[ќЉЫY[ќVЪWVМ—KЫY[ќVЪWVМЧKЉ”™XYHHЉJNВ‚BB_B‚‚BBZY€
^[\VЪWHOH•S
B‚BB^В‚BBB\^Y\њЧШќ\ЮJКОВ‚BB_B‚BBZY€
™XYVЪWHOHЉB‚BB^В‚BBB\^Y\њЧЬ™XYJКОВ‚BB_B‚B_B‚‚BZY€
^Y\њЧШќ\ЮH	‰€^Y\њЧШќ\ЮHOH^Y\њЧЬ™XYH	‰€^][^H	‰€Э[YH€^][^JB‚B^В‚BBY^]HNВ‚B_B‚B]\]J
NВ‚‚BZY€
›Э™]ЪЩ^\И	€“QЧСTРИ\ШШ\WЩ›YИOHLJHЛТЬ]\И
ЊLLЊJHYYH™]И™\ШШ\H€›YИ[€HЩ[XЭШЬ™Y[€ћH\Ъ[™ИH™ЫЭЫXZ[›Y[ќH€ќ[Э[Ы€[™H›YИЊLH‚‚B^В‚BBY\ШШ\HHNВ€\ШШ\WЩ›YИHВ‚B_B‚_B‚‚KЛИ›ИЫ™Щ\€]HЩ[XЭШЬ™Y[‚‚ZЪ[Ш[

NВ‚\ЫЭ[™ШЫЬЩWЫ]\ЪXК
NВ‚\ШЬ™Y[—ЬЭ]\И	ЏH’S—ФРФ‘QS—ФСSPХВ‚‚\™]\›€
Y\ШШ\JNВџB‚ќ›ЪY^YШ[YJ[ќ
њ^Y\њЛ[њЪYЫ™YЪXЪЬЩ][ќ\ЩTШ]™YШ[YJBћВ€[ќNВ€Э\њ™[ќЫ]™[HВ€Э\њ™[ќЬЭYЩHHNВ€Э\њ™[ќЬЩ]HЪXЪЬЩ]В€ЧЬЩ]Щ[ќћH
њЩ]H]™[Щ]И
ИЭ\њ™[ќЬЩ]В€ЧЬШ]™[]™[
њШ]™HHШ]™[]™[
ИЭ\њ™[ќЬЩ]В€ЧЫ]™[Щ[ќћH
›NВ‚€\ЩTШ]™HHВ€\ЩTЩ]HLNВ‚€YЉЪXЪЬЩ]ЏHќ[WЩY™љXЭ[Y\КB€В€™]\›ЋВ€B€ЛИ›Ь”Ъ]ЭЫЉK’[YШ[Щ]ЪЬЩ[Ћ€[™^	ZH
\™H\™HЫ›H	ZHЩ]КHH‹ЪXЪЬЩ]ќ[WЩY™љXЭ[Y\КNВ‚€[ЭЧЬЩXЬ™]ШЪ\њИHЩ]OљYЫЫ\]NВ€VQT—УU‘TИHЩ]O›]™\ОВ€]\ЪXЫЭ™\›\HЩ]O›]\ЪXЫЭ™\›\В€YHHЩ]OЭ\ЭYNВ€УУ•S•QTИHЩ]OЬ™Y]ОВ€XYЪXЧЭ\HHЩ]Oќ\[\В€YЉVQT—УU‘TИOH
B€В€VQT—УU‘TИHОИЛИY][€B€YЉУУ•S•QTИOH
B€В€УУ•S•QTИHNИЛИY][€B€YЉYHOH
B€В€YHHЌИЛИY][€B€Ш[Y\^Y\€HЩ]O››ЬШ[YNВ‚€YЉ\ЩTШ]™YШ[YHOHH	‰€Ш]™KO™›YКB€В€Y[\Щ]
^Y\‹Ъ^™[ЩЉ
њ^Y\ЉH
€
NВ€YЉ[ШYШЬљ\љ[J
JB€В€љ[ќЉ•Ш\›љ[™ЛZ[YИШYШЬљ\Ш]™HW€ЉNВ€B€Э\њ™[ќЫ]™[HШ]™KO›]™[В€Э\њ™[ќЬЭYЩHHШ]™KOњЭYЩNВ€YЉШ]™KO™›YИOHЉHЛИЫ‰ЭЪXЪИHЬ€™XЭX\ЩHY€ЩH\ЩHШ]™YШ[YHH›YИ]\Э™HЊ€В€›ЬЉHHИHЩ]O›X^^Y\њОИJККB€В€^Y\–ЪWK›]™\ИHШ]™KOњ]™\ЦЪWNВ€^Y\–ЪWKЬ™Y]ИHШ]™KOњЬ™Y]ЦЪWNВ€^Y\–ЪWKњШЫЬ™HHШ]™KOњШЫЬ™\ЦЪWNВ€^Y\–ЪWKЫЫЭ\›X\HШ]™KOњЫЫЭ\›X\ЪWNВ€^Y\–ЪWKќЩX\ќ[HHШ]™KOњЩX\ќ[VЪWNВ€^Y\–ЪWKњЬ]ЫљX[HШ]™KOњЬ]ЫљX[ЪWNВ€^Y\–ЪWKњЬ]Ы›\HШ]™KOњЬ]Ы›\ЪWNВ€Э›ЬJ^Y\–ЪWK›[YKШ]™KOњ[YVЪWKPVУђSQWУSЉNВ€B€Ь™Y]ИHШ]™KOЬ™Y]ОВ€B€ШYЬ^XX›WЫ\Э
€Щ]ЬШ]™YШ[ЭЬЩ[XЭШ\™Э[Y[ќКЭ\њ™[ќЬЩ]
B€
NВ€ЛЬ™\Щ]Ь^XX›WЫ\Э
JNИЛИY\И™XШ]\ЩH\™IЬИ›ИЩ[XЭШЬ™Y[‹[\Ь\ћHЫЫ][Ы‚€B‚€›ЬШ]™HHNВ‚€ЛИљ^Ш]™H[€[™^\Э[ќ]™[€Y€
Э\њ™[ќЫ]™[ЏHЩ]O›ќ[[]™[И
B€В€Э\њ™[ќЫ]™[HЩ]O›ќ[[]™[ЛLNВ€B‚€HHЩ]O›]™[Ь™\€
ИЭ\њ™[ќЫ]™[В€Щ]O››ЬЩ[XЭHKO››ЬЩ[XЭВ€Y€
Ш]™KOњЩ[XЭЪЪ\Щ[XЭМJHHHШYЬЪЪ\Щ[XЭ
Ш]™KOњЩ[XЭЪЪ\Щ[XЭЩ]
NИЛИРQЪЪ\Щ[XЭ€›ЬЉHHИHPVФVQT”ОИJККB€В€YЉKOњЪЪ\Щ[XЭЪWJB€В€ЭЬJЪЪ\Щ[XЭЪWKKOњЪЪ\Щ[XЭЪWJNВ€B€[ЩB€В€ЪЪ\Щ[XЭЪWVМHHВ€B€B‚€YЉ
\ЩTШ]™YШ[YHOHH	‰€Ш]™KO™›YИOHЉH\ЩTШ]™YШ[YHOH€Щ[XЭ^Y\Љ^Y\њЛ•S\ЩTШ]™YШ[YJJHЛИY€Ш]™H›YИ\И€Ы‰ЭЩ[XЭ^Y\‚€В€Ъ[JЭ\њ™[ќЫ]™[Щ]O›ќ[[]™[КB€В€YЉњ[ЪЫ[YVМJHЛИњ[ЪЪXЪЪ[™В€В€ЛШЭ\њ™[ќЬЭYЩHHNИЛЪќ[\ќ[\‹‹€\љ\ИЩHЫ‰Э™YYИ™\Щ]][Щ\њИЪЭ[ZЩHШ\™HЩ€]‚€›ЬЉHHИHЩ]O›ќ[[]™[ОИJККB€В€YЉЩ]O›]™[Ь™\–ЪWKњ[Ъ[YH	‰€ЭљXЫ\
Щ]O›]™[Ь™\–ЪWKњ[Ъ[YKњ[ЪЫ[YJHOH
B€В€Э\њ™[ќЫ]™[HNВ€њ™XZОВ€B€ЛЪYЉ]™[Ь™\–ЭЪXЪЬЩ]VЪWKO™ЫЫ™^OLJH
КШЭ\њ™[ќЬЭYЩNИЦ€ЫЫ[Y[ќY\И[™HЭ]€ЩY[\ИИ™HШ]\ЩHЩ€[XЭ\]HЭYЩHИЫЫ\]HY\ЬШYЩK‚€B€њ[ЪЫ[YVМHHЛЛИЫX\€\ЫИЩHЫЫ‰ЭЭXЪИ\™B€B€HHЩ]O›]™[Ь™\€
ИЭ\њ™[ќЫ]™[В€VQT—УRS—Ц€HKOћ—ШЫЫЬ™ЦМNВ€VQT—УPVЦ€HKOћ—ШЫЫЬ™ЦМWNВ€‘ТRQТHKOћ—ШЫЫЬ™ЦМ—NВ‚€YЉKOќ\HOHWХTWРХUФРСS‘JB€В€^\ШЩ[™JKO™љ[[[YJNВ€B€[ЩHYЉKOќ\HOHWХTWФСSPХФРФ‘QSЉB€В€Y[\Щ]
Ш]™KOњЩ[XЭЪЪ\Щ[XЭЪ^™[ЩЉШ]™KOњЩ[XЭЪЪ\Щ[XЭ
JNИЛИ‘TСUЪЪ\Щ[XЭ€Щ]O››ЬЩ[XЭHВ€›ЬЉHHИHЩ]O›X^^Y\њОИJККHЛИ™\Щ]ЪЪ\Щ[XЭ€В€YЉKOњЪЪ\Щ[XЭЪWJB€В€ЪЪ\Щ[XЭЪWVМHHВ€B€B€›ЬЉHHИHЩ]O›X^^Y\њИИJККB€В€^Y\њЦЪWHH
^Y\–ЪWK›]™\И€
NВ€B€YЉЩ[XЭ^Y\Љ^Y\њЛKO™љ[[[YK\ЩTШ]™YШ[YJHOH
B€В€њ™XZОВ€B€B€[ЩHYЉKOќ\HOHWХTWФТТTФСSPХ
B€В€Щ]O››ЬЩ[XЭHKO››ЬЩ[XЭВ€›ЬЉHHИHPVФVQT”ОИJККB€В€YЉKOњЪЪ\Щ[XЭЪWJB€В€ЭЬJЪЪ\Щ[XЭЪWKKOњЪЪ\Щ[XЭЪWJNВ€B€[ЩB€В€ЪЪ\Щ[XЭЪWVМHHВ€B€B€Ш]™WЬЪЪ\Щ[XЭ
Ш]™KOњЩ[XЭЪЪ\Щ[XЭЪЪ\Щ[XЭ
NИЛИИРU‘H’SB€Щ[XЭ^Y\Љ^Y\њЛ•S\ЩTШ]™YШ[YJNИЛИ™K\Щ[XЭH^Y\‚€B€[ЩHYЉ\^[]™[
KO™љ[[[YJJB€В€[ќ[ЬЫ]™\ЧЮ™\›ИHВ€›ЬЉHHИHPVФVQT”ОИJККB€В€Y€
^Y\–ЪWK›]™\ИH
H
КШ[ЬЫ]™\ЧЮ™\›ОВ€B€[ЬЫ]™\ЧЮ™\›ИH
[ЬЫ]™\ЧЮ™\›ИЏHPVФVQT”КHИH€В‚€ЛЪYЉ
^Y\–МK›]™\ИH	‰€^Y\–МWK›]™\ИH	‰€^Y\–М—K›]™\ИH	‰€^Y\–МЧK›]™\ИH
H
B€YЉ[ЬЫ]™\ЧЮ™\›КB€В€YЉ
\Щ]O››ЬЪЭЩШ[Y[Э™\€	‰€JЫЭЧЫXZ[›Y[ќWЩ›YЙЊЉJH
B€В€Ш[Y[Э™\Љ
NВ€B€YЉ\Щ]O››ЬЪЭЪЩ€	‰€JЫЭЧЫXZ[›Y[ќWЩ›YЙЌ
JB€В€[[YJJNВ€B€›ЬЉHHИHЩ]O›X^^Y\њОИJККB€В€^Y\–ЪWKљ\Ь^YYHВ€^Y\–ЪWKќЩX\ќ[HHВ€B€B€њ™XZОВ€B€YЉKO™ЫЫ™^OHJB€В€Y€
\Щ]O››ЬЪЭШЫЫ\]JHЪЭШЫЫ\]JЭ\њ™[ќЬЭYЩJNВ€›ЬЉHHИHЩ]O›X^^Y\њОИJККB€В€^Y\–ЪWKњЬ]ЫљX[HВ€^Y\–ЪWKњЬ]Ы›\HВ€B€
КШЭ\њ™[ќЬЭYЩNВ€Ш]™KOњЭYЩHHЭ\њ™[ќЬЭYЩNВ€B€Э\њ™[ќЫ]™[
КОВ€HHЩ]O›]™[Ь™\€
ИЭ\њ™[ќЫ]™[В€Ш]™KO›]™[HЭ\њ™[ќЫ]™[В€ЛМЊЛL‹LЌЫЫ™^H‹[™Ш[YB€YЉ
HHJKO™ЫЫ™^OHЉB€В€Э\њ™[ќЫ]™[HЩ]O›ќ[[]™[ОВ€B€YЉ\ЩTШ]™JB€В€ЫЭИ[™Ш[YNИЛЬ]ZXЪИ^]Ъ]Э]Ш]љ[™Л›Ь€ШЬљ\ШYШ[YHЩЪXВ€B€KЛЭЪ[B‚€YЉЭ\њ™[ќЫ]™[ЏHЩ]O›ќ[[]™[КB€В€К‚€Ь]\И
KLЊЌ
H™\Щ]]™[ЬЭYЩH[Y\ИЪ[€HШ[YH\Иљ[љ\ЪY™Y›Ь™HШ]љ[™И]‚€]Ъ[]]ЫX]XШ[H\\ЩHHЭ\њ™[ќШ[YH›ЩЬ™\ЬИќ]Ы›HYќ\€H[™[™Л‚€ЭЩ]™\‹]Ъ[XZ[ќZ[€Hќ[Y\ЧШЫЫ\]Y€[ќXЭ‚€Ъ]Э]\Иљ^H^Y\€Ш[€[Ь™X\ЩHHќ[Y\ЧШЫЫ\]Y€[™љ[љ][HћB€ќ\Эљ[љ\Ъ[™ИHШ[YHЫЩH[™ШY[™ИH[™[™И\™XЭHЭИX[ћH[Y\ИHШ[ќ‚€
‹В€›Ыќ\И
ПHШ]™KOќ[Y\ЧШЫЫ\]Y
КОВ€Э\њ™[ќЫ]™[HВ€Э\њ™[ќЬЭYЩHHNВ€Ш]™KO›]™[HЭ\њ™[ќЫ]™[В€Ш]™KOњЭYЩHHЭ\њ™[ќЬЭYЩNВ€Ш]™QШ[YQљ[J
NВ€YЉ[›ЩY[Э]
B€В€YWЫЭ]

NВ€B€YЉ\Щ]O››ЬЪЭЪЩЉB€В€[[YJJNВ€B€B€B‚™[™Ш[YN‚€ЛИЫX\€ЫШ[ШЬљ\\љX[ќ\Э€њ[ЪЫ[YVМHHВ€ЫЭ[™ШЫЬЩWЫ]\ЪXК
NВџB‚љ[ќY[ќWЩY™љXЭ[J
BћВ€[ќ]Z]HВ€[ќЩ[XЭЬ€HВ€[ќX^\Ь^HHNВ€[ќK‹В€ЛЩ›Ш]ЫY\€HВ€[ќ\ћ\ћK\ќЛ\љВ€ЧЩ]ЫY]Щ]ЫY]ЩHZ[›Y]ЩВ€]ЫY]Щ[HH“S‘УSСWРSNВ‚€\ћHљY[Ы[Щ\Лљ™\ИИNВ€\ћHHЫ[™^J
HHЋВ€\ќИHљY[Ы[Щ\Лљ™\И
€ИИNВ€\љHH
€
›ЫќZYЪ

H
ИJH
ИВ€ШЬ™Y[—ЬЭ]\ИHS—ФРФ‘QS—У‘UЧСРSQWУQS•NВ€›Э™]ЪЩ^\ИHВ‚€ШYШ[YQљ[J
NВ‚€Ъ[J\]Z]
B€В€YЉќ[WЩY™љXЭ[Y\И€JB€В€ЫY[ќ]^J‹L‹Љ‘Ш[YH[ЩHЉJNВ€H
Щ[XЭЬ€H
Щ[XЭЬ€OHќ[WЩY™љXЭ[Y\КJHИX^\Ь^H
€X^\Ь^NВ€›ЬЉ€HHHИHX^\Ь^H
И	‰€Hќ[WЩY™љXЭ[Y\ОИЉКЛJККB€В€YЉ€X^\Ь^JB€В€ЛИЬ]\И
LLЊЊJHYY[њЫ][Ы€™X]\™HИHњЩ]€[YB€ЛИ]Ъ[Щ]HњЩ]€[YH[€H›]™[Лќ€љ[H[™[њЫ]H]€ЛИ[ЭH™YYИYH™]И›\ЩЪYЫ\ЩЬЭ€€[њЭ[ЩH[€[Э\€ќ[њЫ][Ы‹ќ€љ[H[™]€ЛИHШ[YH[Y\И\И\ЩYћH[]™[Щ]В€YЉ›Ыќ\ИЏH]™[Щ]ЦЪWKљYЫЫ\]JB€В€ЫY[ќ]^J
Щ[XЭЬ€OHJK‹‰\И‹Љ]™[Щ]ЦЪWK›[YJJNВ€B€[ЩB€В€YЉ]™[Щ]ЦЪWKљYЫЫ\]H€JB€В€ЫY[ќ]^J
Щ[XЭЬ€OHJK‹Љ‰\ИHљ[љ\ЪШ[YH	ZH[Y\ИИ[“ШЪИЉKЉ]™[Щ]ЦЪWK›[YJK]™[Щ]ЦЪWKљYЫЫ\]JNВ€B€[ЩB€В€ЫY[ќ]^J
Щ[XЭЬ€OHJK‹Љ‰\ИHљ[љ\ЪШ[YHИ[“ШЪИЉKЉ]™[Щ]ЦЪWK›[YJJNВ€B€B€B€[ЩB€В€њ™XZОВ€B€B€ЫY[ќ]^J
Щ[XЭЬ€OHJK‹ЉђXЪИЉJNВ‚€ЛЩ]ИHШЬ›Ы\‚€YЉќ[WЩY™љXЭ[Y\И€X^\Ь^JB€В€Ьљ]\WШYШ›Ю
\ћ\ћK\ќЛ\љЫЫЬ—Ш›XЪЛ	™]ЫY]Щ
NИЛЫЭ]\›Ю€Ьљ]\WШYЫ[™J\ћ\ћK\ћ
И\ћKKЫЫЬ—ЭЪ]K•S
NВ€Ьљ]\WШYЫ[™J\ћ\ћK\ћ\ћH
И\љKЫЫЬ—ЭЪ]K•S
NВ€Ьљ]\WШYЫ[™J\ћ
И\ћK\ћ
И\ћH
И\љKЫЫЬ—ЭЪ]K•S
NВ€Ьљ]\WШYЫ[™J\ћ\ћH
И\љ\ћ
И\ћH
И\љKЫЫЬ—ЭЪ]K•S
NВ€Ьљ]\WШYШ›Ю
\ћ
ИK\ћH
ИЩ[XЭЬ€
€
\љHКHИќ[WЩY™љXЭ[Y\ЛЛЛ‹ЫЫЬ—ЭЪ]K•S
NИЛЬЫY\‚€B€B‚€\]J
NВ‚€YЉќ[WЩY™љXЭ[Y\ИOHJHЛИЦ€[ЩИЪ]Ы›HЫ™HЩ]Ъ[]]ИШY]Y™љXЭ[K‚€В€YЉЩ[XЭЬ€OHќ[WЩY™љXЭ[Y\КB€В€]Z]HNВ€B€[ЩHYЉ›Ыќ\ИЏH]™[Щ]ЦЬЩ[XЭЬ—KљYЫЫ\]JB€В€Ш]™\ЫЭHЩ[XЭЬЋВ€Э›ЬJШ]™[]™[ЬШ]™\ЫЭK™[YK]™[Щ]ЦЬШ]™\ЫЭK›[YKPVУђSQWУS€HJNВ€ШЬ™Y[—ЬЭ]\И	ЏH’S—ФРФ‘QS—У‘UЧСРSQWУQS•NВ€™]\›€Ш]™\ЫЭВ€B€B‚€YЉ›Э™]ЪЩ^\И	€“QЧСTРКB€В€]Z]HNВ€B€YЉ›Э™]ЪЩ^\И	€“QЧУSХ‘UT
B€В€K\Щ[XЭЬЋВ€YЉЫШ[ЬШ[\WЫ\Э™Y\ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€YЉ›Э™]ЪЩ^\И	€“QЧУSХ‘QХУЉB€В€
КЬЩ[XЭЬЋВ€YЉЫШ[ЬШ[\WЫ\Э™Y\ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€YЉЩ[XЭЬ€
B€В€Щ[XЭЬ€Hќ[WЩY™љXЭ[Y\ОВ€B€YЉЩ[XЭЬ€€ќ[WЩY™љXЭ[Y\КB€В€Щ[XЭЬ€HВ€B€ЛЪYЉЩ[XЭЬЏќ[WЩY™љXЭ[Y\КHЫY\€HЩ[XЭЬ€
€ЌNВ‚€YЉ›Э™]ЪЩ^\И	€“QЧРS–P•UУЉB€В‚€YЉЫШ[ЬШ[\WЫ\Э™Y\М€ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\М‹Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B‚€YЉЩ[XЭЬ€OHќ[WЩY™љXЭ[Y\КB€В€]Z]HNВ€B€[ЩHYЉ›Ыќ\ИЏH]™[Щ]ЦЬЩ[XЭЬ—KљYЫЫ\]JB€В€Ш]™\ЫЭHЩ[XЭЬЋВ€Э›ЬJШ]™[]™[ЬШ]™\ЫЭK™[YK]™[Щ]ЦЬШ]™\ЫЭK›[YKPVУђSQWУS€HJNВ€ШЬ™Y[—ЬЭ]\И	ЏH’S—ФРФ‘QS—У‘UЧСРSQWУQS•NВ€™]\›€Ш]™\ЫЭВ€B€B€B€›Э™]ЪЩ^\ИHВ€ШЬ™Y[—ЬЭ]\И	ЏH’S—ФРФ‘QS—У‘UЧСРSQWУQS•NВ€™]\›€LNВџB‚љ[ќШYЬШ]™YЩШ[YJ
BћВ€[ќ]Z]HВ€[ќЩ[XЭЬ€HВ€[ќШ]™YЭ]\ИHВ€Ъ\€[YVУPVР•Q‘‘T—УS—HHИ€џNВ€[ќЫЫHHNЫЫ€HЋВ‚€ШЬ™Y[—ЬЭ]\ИHS—ФРФ‘QS—УРQСРSQWУQS•NВ€›Э™]ЪЩ^\ИHВ‚€YЉ
Ш]™YЭ]\ИHШYШ[YQљ[J
JJB€В€Щ]ZУ[YJ[YK
NВ€B€›ЬЉШ]™\ЫЭHИШ]™\ЫЭќ[WЩY™љXЭ[Y\ОИШ]™\ЫЭ
ККHYЉШ]™[]™[ЬШ]™\ЫЭK™›YИ	‰€Ш]™[]™[ЬШ]™\ЫЭK›]™[
B€В€њ™XZОВ€B‚€Ъ[J\]Z]
B€В€YЉШ]™\ЫЭЏHќ[WЩY™љXЭ[Y\КHЛИ›Э›Э[™€В€ЫY[ќ]^J‹MЉ“ШYШ[YHЉJNВ€ЫY[ќ]^
ЫЫKL‹Љ”Ш]™Yљ[N€ЉJNВ€ЫY[ќ]^
ЫЫ‹L‹Љ“›Э›Э[™HЉJNВ€ЫY[ќ]^JK‹ЉђXЪИЉJNВ‚€К‚€Ь]\И
KLЊЌ
HZ[›Ь€љ^[€HЩ[XЭЬ€ќ[X™\€Y€›ИШ]™H\И›Э[™‚€\™IЬИ›ИЩ[XЭЬ€Њ€€[€\ИY[ќKЫ›H
ШY
HЬ€H
XЪКK‚€
‹В€Щ[XЭЬ€HNВ€B€[ЩB€В€ЫY[ќ]^J‹MЉ“ШYШ[YHЉJNВ€ЫY[ќ]^
ЫЫKL‹Љ”Ш]™Yљ[N€ЉJNВ€YЉШ]™YЭ]\КB€В€ЫY[ќ]^
ЫЫ‹L‹‰\И‹[YJNВ€B€[ЩB€В€ЫY[ќ]^
ЫЫ‹L‹Љ“›Э›Э[™HЉJNВ€B‚€YЉШ]™YЭ]\КB€В€ЫY[ќ]^

Щ[XЭЬ€OH
KЫЫKLKЉ“[ЩN€ЉJNВ€ЫY[ќ]^

Щ[XЭЬ€OH
KЫЫ‹LK‰\И‹Ш]™[]™[ЬШ]™\ЫЭK™[YJNВ€ЫY[ќ]^
ЫЫKЉ”ЭYЩN€ЉJNВ€ЫY[ќ]^
ЫЫ‹‰Y‹Ш]™[]™[ЬШ]™\ЫЭKњЭYЩJNВ€ЫY[ќ]^
ЫЫKKЉ“]™[€ЉJNВ€ЫY[ќ]^
ЫЫ‹K‰Y‹Ш]™[]™[ЬШ]™\ЫЭK›]™[
NВ€ЫY[ќ]^
ЫЫK‹ЉђЬ™Y]О€ЉJNВ‚€YЉ›ЬЪ\™J^В€ЫY[ќ]^
ЫЫ‹‹‰YЙYЙYЙY‹€Ш]™[]™[ЬШ]™\ЫЭKњЬ™Y]ЦМK€Ш]™[]™[ЬШ]™\ЫЭKњЬ™Y]ЦМWKШ]™[]™[ЬШ]™\ЫЭKњЬ™Y]ЦМ—K€Ш]™[]™[ЬШ]™\ЫЭKњЬ™Y]ЦМЧJNВ€H[ЩHВ€ЫY[ќ]^
ЫЫ‹‹‰Y‹Ш]™[]™[ЬШ]™\ЫЭKЬ™Y]КNВ€B‚€ЫY[ќ]^
ЫЫKЛЉ”^Y\€]™\О€ЉJNВ€ЫY[ќ]^
ЫЫ‹Л‰YЙYЙYЙY‹€Ш]™[]™[ЬШ]™\ЫЭKњ]™\ЦМK€Ш]™[]™[ЬШ]™\ЫЭKњ]™\ЦМWKШ]™[]™[ЬШ]™\ЫЭKњ]™\ЦМ—K€Ш]™[]™[ЬШ]™\ЫЭKњ]™\ЦМЧJNВ€B€ЫY[ќ]^J
Щ[XЭЬ€OHJK‹ЉђXЪИЉJNВ€B€\]J
NВ‚€YЉ›Э™]ЪЩ^\И	€“QЧСTРКB€В€]Z]HNВ€B€YЉЩ[XЭЬ€OH	‰€
›Э™]ЪЩ^\И	€“QЧУSХ‘SQ•
JB€В€Ъ[JJB€В€K\Ш]™\ЫЭВ€YЉШ]™\ЫЭ
B€В€Ш]™\ЫЭHќ[WЩY™љXЭ[Y\ИHNВ€B€YЉШ]™[]™[ЬШ]™\ЫЭK™›YИ	‰€Ш]™[]™[ЬШ]™\ЫЭK›]™[
B€В€њ™XZОВ€B€B€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€YЉЩ[XЭЬ€OH	‰€
›Э™]ЪЩ^\И	€“QЧУSХ‘T’QТ
JB€В€Ъ[JJB€В€
КЬШ]™\ЫЭВ€YЉШ]™\ЫЭ€ќ[WЩY™љXЭ[Y\ИHJB€В€Ш]™\ЫЭHВ€B€YЉШ]™[]™[ЬШ]™\ЫЭK™›YИ	‰€Ш]™[]™[ЬШ]™\ЫЭK›]™[
B€В€њ™XZОВ€B€B€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€YЉ›Э™]ЪЩ^\И	€“QЧУSХ‘UT
B€В€K\Щ[XЭЬЋВ€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€YЉ›Э™]ЪЩ^\И	€“QЧУSХ‘QХУЉB€В€
КЬЩ[XЭЬЋВ€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€YЉШ]™YЭ]\КB€В€YЉЩ[XЭЬ€
B€В€Щ[XЭЬ€HNВ€B€YЉЩ[XЭЬ€€JB€В€Щ[XЭЬ€HВ€B€B€[ЩB€В€Щ[XЭЬ€HNВ€B‚€YЉ
›Э™]ЪЩ^\И	€“QЧРS–P•UУЉJB€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\М‹Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€ЭЪ]Ъ
Щ[XЭЬЉB€В€Ш\ЩH‚€™]\›€Ш]™\ЫЭВ€њ™XZОВ€Ш\ЩHN‚€]Z]HNВ€њ™XZОВ€B€B€B€›Э™]ЪЩ^\ИHВ€ШЬ™Y[—ЬЭ]\И	ЏH’S—ФРФ‘QS—УРQСРSQWУQS•NВ€™]\›€LNВџB‚љ[ќЪЫЬЩWЫ[ЩJ[ќ
њ^Y\њКBћВ€[ќ]Z]HВ€[ќ™[XЪИHВ€[ќЩ[XЭЬ€HВ€[ќЭ]\ИHВ‚€ШЬ™Y[—ЬЭ]\ИHS—ФРФ‘QS—СРSQWФХT•УQS•NВ€›Э™]ЪЩ^\ИHВ‚€Ъ[J\]Z]
B€В€ЫY[ќ]^J‹KЉђЪЫЬЩH[ЩHЉJNВ€ЫY[ќ]^J
Щ[XЭЬ€OH
KЛЉ“™]ИШ[YHЉJNВ€ЫY[ќ]^J
Щ[XЭЬ€OHJKЉ“ШYШ[YHЉJNВ€ЫY[ќ]^J
Щ[XЭЬ€OHЉK‹ЉђXЪИЉJNВ‚€\]J
NВ‚€YЉ›Э™]ЪЩ^\И	€“QЧСTРКB€В€]Z]HNВ€B€YЉ›Э™]ЪЩ^\И	€“QЧУSХ‘UT
B€В€K\Щ[XЭЬЋВ€YЉЫШ[ЬШ[\WЫ\Э™Y\ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€YЉ›Э™]ЪЩ^\И	€“QЧУSХ‘QХУЉB€В€
КЬЩ[XЭЬЋВ€YЉЫШ[ЬШ[\WЫ\Э™Y\ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€YЉЩ[XЭЬ€
B€В€Щ[XЭЬ€HЋВ€B€YЉЩ[XЭЬ€€ЉB€В€Щ[XЭЬ€HВ€B‚€YЉ›Э™]ЪЩ^\И	€“QЧРS–P•UУЉB€В€YЉЫШ[ЬШ[\WЫ\Э™Y\М€ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\М‹Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€ЭЪ]Ъ
Щ[XЭЬЉB€В€Ш\ЩH‚€Э]\ИHY[ќWЩY™љXЭ[J
NВ€YЉЭ]\ИOHLJB€В€^YШ[YJ^Y\њЛЭ]\Л
NВ€™[XЪИHNВ€]Z]HNВ€B€њ™XZОВ€Ш\ЩHN‚€Э]\ИHШYЬШ]™YЩШ[YJ
NВ€YЉЭ]\ИOHLJB€В€^YШ[YJ^Y\њЛЭ]\ЛJNВ€™[XЪИHNВ€]Z]HNВ€B€њ™XZОВ€Y][‚€]Z]HNВ€њ™XZОВ€B€B€B€›Э™]ЪЩ^\ИHВ€ШЬ™Y[—ЬЭ]\И	ЏH’S—ФРФ‘QS—СРSQWФХT•УQS•NВ€™]\›€™[XЪОВџB‚ќ›ЪY\›WЭљY[Ы[Щ\К
BћВ€љY[Ы[Щ\Лљ™\ИHВ€љY[Ы[Щ\Лќ”™\ИHВ€љY[ЧЬЩ]Ы[ЩJљY[Ы[Щ\КNВ€YЉЭ\ЭШЩ[™\ИOH•S
B€В€њ™YJЭ\ЭШЩ[™\КNВ€B€Э\ЭШЩ[™\ИH•SВ€YЉЭ\ЭљЩЬ™ИOH•S
B€В€њ™YJЭ\ЭљЩЬ™КNВ€B€Э\ЭљЩЬ™ИH•SВ€YЉЭ\Э]™[ИOH•S
B€В€њ™YJЭ\Э]™[КNВ€B€Э\Э]™[ИH•SВ€YЉЭ\Э[Щ[ИOH•S
B€В€њ™YJЭ\Э[Щ[КNВ€B€Э\Э[Щ[ИH•SВџB‚‹ЛИШYљY[И[ЩHњ›ЫHљ[Bќ›ЪY[љ]ЭљY[Ы[Щ\К[ќЩКBћВ€Ъ\€
™љ[[[YHH™]KЭљY[ЛќЋВ€[ќ\В€™Y™—ЭЬЛ[ЋВ€Ъ^™WЭЪ^™NВ€Ъ\€
ќY€H•SВ€Ъ\€
ЫЫ[X[™H•SВ€Ъ\€
ќ[YHH•S
ќ[YLЋВ€\™У\Э\™Ы\ЭВ€Ъ\€\™ШќY–УPVРT‘ЧУS€
ИWHH€ЋВ‚€ЛВ‚€YЉЩКB€В€љ[ќЉ’[љ]X[^љ[™ИљY[Л‹‹‹‹‹‹‹‹‹‹‹—€ЉNВ€B‚€ЛИ\ЩH[€[\›]]™HљY[ЛќY€\™H\ИЫ™K€ЫЫYHЩ€\ЩH\™HЫ™Иљ[[[Y\ОИЬ™X]H[Э\€RЬИЪ]›ЬњZИ[™[ЭIЫ™Hљ[™K‚€ЩYљ[™HћYљ[J
HYЉ
\[Ь[њXЪЩљ[JXЪЩљ[JJHOKLJHИЫЬЩ\XЪЩљ[J\
NИљ[[[YOVИЫЭИ™XYљ[NИB€ЪY€ТS€S•V€ћYљ[J™]KЭљY[ЬЛќЉNВ€Щ[™Y‚€Э[™Y€ћYљ[B‚њ™XYљ[N‚€ЛИ™XYљ[B€YЉќY™™\—ЬZЩљ[Jљ[[[YK	ќY‹	њЪ^™JHOHJB€В€љY[У[ЩHHВ€љ[ќЉ‰Й\ЙИ›Э›Э[™—€‹љ[[[YJNВ€ЫЭИ’QSУSСTОВ€B‚€љ[ќЉ”™XY[™ИљY[ИЩ][™ЬИњ›ЫH	Й\ЙЛ—€‹љ[[[YJNВ‚€ЛИ›ЭИ[ќ\њ™]HЫЫќ[ќИЩ€ќY€[™HћH[™B€ЬИHВ€Ъ[JЬИЪ^™JB€В€\њЩP\™ЬК	\™Ы\ЭќY€
ИЬЛ\™ШќYЉNВ€ЫЫ[X[™HСUРT‘К
NВ‚€YЉЫЫ[X[™	‰€ЫЫ[X[™МJB€В€YЉЭљXЫ\
ЫЫ[X[™ќљY[ИЉHOH
B€В€[YHHСUРT‘КJNВ€YЉ
[YL€HЭЪЉ[YK	Ю	КJJB€В€љY[Ы[Щ\Лљ™\ИH]ЪJ[YJNВ€љY[Ы[Щ\Лќ”™\ИH]ЪJ[YL€
ИJNВ€љY[У[ЩHHЌMNВ€B€[ЩB€В€љY[У[ЩHHСUТS•РT‘КJNВ€B€B€[ЩHYЉЭљXЫ\
ЫЫ[X[™њШЩ[™\ИЉHOH
B€В€[€HЭ›[ЉСUРT‘КJJNВ€Э\ЭШЩ[™\ИHX[ШК[€
ИJNВ€ЭЬJЭ\ЭШЩ[™\ЛСUРT‘КJJNВ€Э\ЭШЩ[™\ЦЫ[—HHВ€B€[ЩHYЉЭљXЫ\
ЫЫ[X[™XЪЩЬ›Э[™ИЉHOH
B€В€[€HЭ›[ЉСUРT‘КJJNВ€Э\ЭљЩЬ™ИHX[ШК[€
ИJNВ€ЭЬJЭ\ЭљЩЬ™ЛСUРT‘КJJNВ€Э\ЭљЩЬ™ЦЫ[—HHВ€B€[ЩHYЉЭљXЫ\
ЫЫ[X[™›]™[ИЉHOH
B€В€[€HЭ›[ЉСUРT‘КJJNВ€Э\Э]™[ИHX[ШК[€
ИJNВ€ЭЬJЭ\Э]™[ЛСUРT‘КJJNВ€Э\Э]™[ЦЫ[—HHВ€B€[ЩHYЉЭљXЫ\
ЫЫ[X[™›[Щ[ИЉHOH
B€В€[€HЭ›[ЉСUРT‘КJJNВ€Э\Э[Щ[ИHX[ШК[€
ИJNВ€ЭЬJЭ\Э[Щ[ЛСUРT‘КJJNВ€Э\Э[Щ[ЦЫ[—HHВ€B€[ЩHYЉЭљXЫ\
ЫЫ[X[™ЫЫЭ\™\ЉHOH
B€В€љ[ќЉ—ђЫЫЬ™\\И›ЭЭ\ЬќY[ћ[[Ь™K€[[Щ[\И\™H\Ь^YYЪ]HМљ]ЫЫЬ€ШЬ™Y[‹——€ЉNВ€B€[ЩHYЉЭљXЫ\
ЫЫ[X[™™›ЬЩ[[ЩHЉHOH
HЯB€[ЩHYЉЫЫ[X[™	‰€ЫЫ[X[™МJB€В€љ[ќЉђЫЫ[X[™	Й\ЙИ›Э[™\њЭЫЩ[€љ[H	Й\ЙИW€‹ЫЫ[X[™љ[[[YJNВ€B€B€ЛИЫИИ™^[™B€ЬИ
ПHЩ]™]У[™TЭ\ќ
ќY€
ИЬКNВ€B‚€YЉќY€OH•S
B€В€њ™YJќYЉNВ€ќY€H•SВ€B‚•’QSУSСTО‚€љY[Ы[Щ\Л›[ЩHHљY[У[ЩNВ€љY[Ы[Щ\Л™љ[\€HШ]™Y]KњЭЩљ[\ЋВ€ЭЪ]Ъ
љY[У[ЩJB€В€ЛИМЊЌHU‘РB€Ш\ЩH‚€љY[Ы[Щ\Лљ™\ИHМЊВ€љY[Ы[Щ\Лќ”™\ИHЌВ€љY[Ы[Щ\ЛљШШ[HHNВ€љY[Ы[Щ\Лќ”ШШ[HHNВ€љY[Ы[Щ\ЛљЪYќHВ€љY[Ы[Щ\Лќ”ЪYќHВ€љY[Ы[Щ\Л™Щ™њЩ]HЊМNВ€VQT—УRS—Ц€HMЊВ€VQT—УPVЦ€HЊМЋВ€‘ТRQТHMЊВ€њ™XZОВ‚€ЛИЌМ€HФU‘РB€Ш\ЩHN‚€љY[Ы[Щ\Лљ™\ИHВ€љY[Ы[Щ\Лќ”™\ИHЌМЋВ€љY[Ы[Щ\ЛљШШ[HH
›Ш]
LKЌNВ€љY[Ы[Щ\Лќ”ШШ[HH
›Ш]
LKЊLОВ€љY[Ы[Щ\ЛљЪYќHВ€љY[Ы[Щ\Лќ”ЪYќHЊВ€љY[Ы[Щ\Л™Щ™њЩ]HЌЊОВ€VQT—УRS—Ц€HNЋВ€VQT—УPVЦ€HЌЌВ€‘ТRQТHNЋВ€њ™XZОВ‚€ЛИЌH‘РB€Ш\ЩHЋ‚€љY[Ы[Щ\Лљ™\ИHЌВ€љY[Ы[Щ\Лќ”™\ИHВ€љY[Ы[Щ\ЛљШШ[HHЋВ€љY[Ы[Щ\Лќ”ШШ[HHЋВ€љY[Ы[Щ\ЛљЪYќHMЊВ€љY[Ы[Щ\Лќ”ЪYќHНNВ€љY[Ы[Щ\Л™Щ™њЩ]HЌВ€VQT—УRS—Ц€HМЊNВ€VQT—УPVЦ€HЌNВ€‘ТRQТHМЊNВ€њ™XZОВ‚€ЛИМЊH‘€Ш\ЩHО‚€љY[Ы[Щ\Лљ™\ИHМЊВ€љY[Ы[Щ\Лќ”™\ИHВ€љY[Ы[Щ\ЛљШШ[HH‹ЊЌNВ€љY[Ы[Щ\Лќ”ШШ[HHЋВ€љY[Ы[Щ\ЛљЪYќHЊВ€љY[Ы[Щ\Лќ”ЪYќHНNВ€љY[Ы[Щ\Л™Щ™њЩ]HЌВ€VQT—УRS—Ц€HМЊNВ€VQT—УPVЦ€HЌNВ€‘ТRQТHМЊNВ€њ™XZОВ‚€ЛИHХ‘РB€Ш\ЩH‚€љY[Ы[Щ\Лљ™\ИHВ€љY[Ы[Щ\Лќ”™\ИHВ€љY[Ы[Щ\ЛљШШ[HH‹ЌNВ€љY[Ы[Щ\Лќ”ШШ[HHЋВ€љY[Ы[Щ\ЛљЪYќHЌВ€љY[Ы[Щ\Лќ”ЪYќHНNВ€љY[Ы[Щ\Л™Щ™њЩ]HЌВ€VQT—УRS—Ц€HМЊNВ€VQT—УPVЦ€HЌNВ€‘ТRQТHМЊNВ€њ™XZОВ‚€ЛИЊHХ‘РB€Ш\ЩHN‚€љY[Ы[Щ\Лљ™\ИHВ€љY[Ы[Щ\Лќ”™\ИHЊВ€љY[Ы[Щ\ЛљШШ[HH‹ЌNВ€љY[Ы[Щ\Лќ”ШШ[HH‹ЌNВ€љY[Ы[Щ\ЛљЪYќHЌВ€љY[Ы[Щ\Лќ”ЪYќHВ€љY[Ы[Щ\Л™Щ™њЩ]HNВ€VQT—УRS—Ц€HNВ€VQT—УPVЦ€HNЋВ€‘ТRQТHNВ€њ™XZОВ‚€ЛИMЊMHR€Ш\ЩHЋ‚€љY[Ы[Щ\Лљ™\ИHMЊВ€љY[Ы[Щ\Лќ”™\ИHMВ€љY[Ы[Щ\ЛљШШ[HHОВ€љY[Ы[Щ\Лќ”ШШ[HH‹ЊЌNВ€љY[Ы[Щ\ЛљЪYќHМЊВ€љY[Ы[Щ\Лќ”ЪYќHВ€љY[Ы[Щ\Л™Щ™њЩ]HLЊЋВ€VQT—УRS—Ц€HНЊЋВ€VQT—УPVЦ€HLЌВ€‘ТRQТHНЊЋВ€њ™XZОВ‚€ЛИЭ\ЭЫHљY[И[ЩB€Ш\ЩHЌMN‚€љY[Ы[Щ\Л™Щ™њЩ]HљY[Ы[Щ\Лќ”™\И
€ЋMЊЌNВ€љ[ќЉ—•\Ъ[™ИXќYИљY[И[ЩN€	Y	Y€‹љY[Ы[Щ\Лљ™\ЛљY[Ы[Щ\Лќ”™\КNВ€њ™XZОВ€Y][‚€›Ь”Ъ]ЭЫЉK’[ќ[YљY[И[ЩN€	Y[€	Щ]KЭљY[Лќ	ЛЭ\ЬќY[Щ\О—€‚€ЊHМЊЌ€‚€ЊHHЌМ—€‚€Њ€HЌ€‚€ЊИHМЊ€‚€ЌH€‚€ЌHHЊ€‚€Ќ€HMЊM—€‹љY[У[ЩJNВ€њ™XZОВ€B‚€љY[ЧЬЭ™]Ъ
Ш]™Y]KњЭ™]Ъ
NВ‚€YЉ
њШЬ™Y[€H[ШЬШЬ™Y[ЉљY[Ы[Щ\Лљ™\ЛљY[Ы[Щ\Лќ”™\ЛVSММЉJHOH•S
B€В€›Ь”Ъ]ЭЫЉK“›Э[›ЭYЪY[[ЬћHW€ЉNВ€B€љY[Ы[Щ\Лњ^[H^[ћ]\ЦК[ќ
]њШЬ™Y[‹Oњ^[›Ь›X]NВ€ЛЭљY[ЧЬЩ]Ы[ЩJљY[Ы[Щ\КNВ€ЫX\њШЬ™Y[ЉњШЬ™Y[ЉNВ‚€YЉЩКB€В€љ[ќЉ’[љ]X[^™YљY[Л‹‹‹‹‹‹‹‹‹‹‹—	Y	Y
[ЩN€	Y
W—€‹љY[Ы[Щ\Лљ™\ЛљY[Ы[Щ\Лќ”™\ЛљY[У[ЩJNВ€BџB‚‚‚‹ЛИKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKB‚‚‹ЛИЩ]Щ^HЬ€ќ]Ы€ШY™[H
Ъ]ЭЪ]Ъ[™КBќ›ЪYШY™WЬЩ]
[ќ
\њ‹[ќ[™^[ќ™]ЪЩ^K[ќЫЩ^JBћВ€[ќNВ€›ЬЉHHИHLЋИJККB€В€YЉ\њ–ЪWHOH™]ЪЩ^JB€В€\њ–ЪWHHЫЩ^NВ€B€B€\њ–Ъ[™^HH™]ЪЩ^NВџB‚ќ›ЪYЩ^X›Ш\™ЬЩ]\
[ќ^Y\ЉBћВ€[ќ]Z]HИ€[ќЩYHВ€[ќЩ[XЭЬ€HВ€[ќЩ][™ИHLNВ€[ќHHВ€[ќИHВ€[ќЪИHВ€[ќ\ШX›YЩ^VУPVР•—У•SWHHИHNВ€[ќЫЫHHNВ€[ќЫЫ€HЋВ‚€™Y™—Э›Щ™њЩ]И€™Y™—ЭЬОВ‚€Ъ\Љ€ќYЋВ€Ъ\Љ€ЫЫ[X[™В€Ъ\Љ€љ[[[YHHќ[њЫ][Ы‹ЫY[ќKќЋВ€€Ъ\€ќ]Ы›[Y\ЦУPVР•—У•SWVММ—NВ€€Ъ^™WЭЪ^™NВ€\™У\Э\™Ы\ЭВ€€Ъ\€\™ШќY–УPVРT‘ЧУS€
ИWHH€ЋВ€[ќФSУ”ЧУ•SHHPVР•—У•SH
ИОВ‚€ШЬ™Y[—ЬЭ]\ИHS—ФРФ‘QS—Р•UУ—РУУ‘’QЧУQS•NВ‚€љ[ќЉ“ШY[™ИЫЫќ›ЫЩ][™ЬЛ‹‹‹‹‹‹—ЉNВ‚€ЭЬJќ]Ы›[Y\ЦФСQУSХ‘UTK“[Э™H\ЉNВ€ЭЬJќ]Ы›[Y\ЦФСQУSХ‘QХУ—K“[Э™HЭЫ€ЉNВ€ЭЬJќ]Ы›[Y\ЦФСQУSХ‘SQ•K“[Э™HYќЉNВ€ЭЬJќ]Ы›[Y\ЦФСQУSХ‘T’QТK“[Э™HљYЪЉNВ€ЭЬJќ]Ы›[Y\ЦФСQРUPТЧKђ]XЪИHЉNВ€ЭЬJќ]Ы›[Y\ЦФСQРUPТМ—Kђ]XЪИ€ЉNВ€ЭЬJќ]Ы›[Y\ЦФСQРUPТМЧKђ]XЪИИЉNВ€ЭЬJќ]Ы›[Y\ЦФСQРUPТНKђ]XЪИЉNВ€ЭЬJќ]Ы›[Y\ЦФСQТ•STK’ќ[\ЉNВ€ЭЬJќ]Ы›[Y\ЦФСQФФPТPSK”ЬXЪX[ЉNВ€ЭЬJќ]Ы›[Y\ЦФСQФХT•K”Э\ќЉNВ€ЭЬJќ]Ы›[Y\ЦФСQФРФ‘QS”ТХK”ШЬ™Y[њЪЭЉNВ€ЭЬJќ]Ы›[Y\ЦФСQСTРЧK‘^]ЉNВ‚€Ш]™\Щ][™ЬК
NВ€›Э™]ЪЩ^\ИHВ‚€К‚€Ь]\И
LLЊЊJHYY[€[\›]]™HШШ][Ы€›Ь€H[њЫ][Ы€љ[K›ЭИ]	ЬИЬЬЪX›HИ\ЩH[€[€^\›[›Ы\‚€›ЭИH[Щ\€Ш[€ШY^ЬќY[њЫ][Ы€љ[\ИћH\Ъ[™И™љ[\Э™X[H€ШЬљ\ќ[Э[ЫњВ€\ЩYќ[›Ь€Ь™X][™ИЭ\ЭЫH[њЫ][ЫњИЪ]Э][њXЪИHШ[YB€HY][[™Ъ[™H[њЫ][Ы€ШШ][Ы€Ъ[™HXZ[ќZ[™Y›Ь€XЪЭШ\™ЫЫ\]Xљ[]B‚€Ь]\И
LKLЊЊJH[ќ™\ќYH]љ[Ьљ]K›ЭИH^\›[љ[HЪ[Э™\њљYHH[ќ\›[љ[B€\ЩYќ[ИXZ[ќZ[€H[™Ы\Ъ[њЫ][Ы€[ќXЭ[њЪYHHZИљ[HY€›ИЭ\€[™ЭXYЩHљ[H\И›Э[™[€H^\›[]€Э\ќЪ\ЩH[ЭHЪ[™YYИ›ЫXЪИH[™Ы\Ъљ[H]™\ћH[YH[›Э\€[™ЭXYЩH\И\ЩY[™[€™[[Э™Y€\ИЬ\][Ы€\И™YYYЫ›HY€H[™Ы\Ъ[њЫ][Ы€љ[H\Щ\ИЫЫYHЭ\ЭЫHY[ќH^И›Ь€[™Ы\Ъ[™ЭXYЩHЫВ€
‹В€YЉќY™™\—ЬZЩљ[Jљ[[[YK	ќY‹	њЪ^™JHOHJB€В€ЫЭИY][Щљ[NВ€B€[ЩB€В€ЫЭИ›ШЩYYВ€B‚™Y][Щљ[N‚‚€YЉќY™™\—ЬZЩљ[J™]KЫY[ќKќ‹	ќY‹	њЪ^™JHOHJB€В€ЫЭИљ[љ\ЪВ€B€[ЩB€В€ЫЭИ›ШЩYYВ€B‚њ›ШЩYY‚‚€ЛИ™XYљ[B€ЛИ›ЭИ[ќ\њ™]HЫЫќ[ќИЩ€ќY€[™HћH[™B€ЬИHВ€Ъ[JЬИЪ^™JB€В€\њЩP\™ЬК	\™Ы\ЭќY€
ИЬЛ\™ШќYЉNВ€ЫЫ[X[™HСUРT‘К
NВ€YЉЫЫ[X[™МJB€В€YЉЭљXЫ\
ЫЫ[X[™™\ШX›ZЩ^HЉHOH
B€В€ЩYH[њЫ]WФСQ
СUРT‘КJJNВ€YЉЩYЏH
B€В€\ШX›YЩ^VЬЩYHHNВ€B€B€[ЩHYЉЭљXЫ\
ЫЫ[X[™њ™[[YZЩ^HЉHOH
B€В€ЩYH[њЫ]WФСQ
СUРT‘КJJNВ€YЉЩYЏH
B€В€ЭЬJќ]Ы›[Y\ЦЬЩYKСUРT‘КЉJNВ€B€B‚€B€ЛИЫИИ™^[™B€ЬИ
ПHЩ]™]У[™TЭ\ќ
ќY€
ИЬКNВ€B€YЉќY€OH•S
B€В€њ™YJќYЉNВ€ќY€H•SВ€B‚™љ[љ\Ъ‚‚€Ъ[J\ШX›YЩ^VЬЩ[XЭЬ—JHYЉ
КЬЩ[XЭЬ€€PVР•—У•SHHJHњ™XZОВ‚€Ъ[J\]Z]
B€В€›Щ™њЩ]HMЋВ€ЫY[ќ]^J‹NЉ”^Y\€	ZHЉK^Y\€
ИJNВ€›ЬЉHHИHPVР•—У•SNИJККB€В€YЉY\ШX›YЩ^VЪWJB€В€ЫY[ќ]^

Щ[XЭЬ€OHJKЫЫK›Щ™њЩ]‰\И‹ќ]Ы›[Y\ЦЪWJNВ€ЫY[ќ]^

Щ[XЭЬ€OHJKЫЫ‹›Щ™њЩ]‰\И‹ЫЫќ›ЫЩЩ]Щ^[[YJШ]™Y]KљЩ^\ЦЬ^Y\—VЪWJJNВ€›Щ™њЩ]
КОВ€B€B‚€
КЭ›Щ™њЩ]В€YЉШ]™Y]Kљ›Ю\ќ[X›VЬ^Y\—JB€В€ЫY[ќ]^

Щ[XЭЬ€OHФSУ”ЧУ•SHHКKЫЫK›Щ™њЩ]
КЛЉ”ќ[X›H[X›YЉJNВ€B€[ЩB€В€ЫY[ќ]^

Щ[XЭЬ€OHФSУ”ЧУ•SHHКKЫЫK›Щ™њЩ]
КЛЉ”ќ[X›H\ШX›YЉJNВ€B‚€ЫY[ќ]^J
Щ[XЭЬ€OHФSУ”ЧУ•SHHЉK
КЭ›Щ™њЩ]Љ“ТИЉJNВ€ЫY[ќ]^J
Щ[XЭЬ€OHФSУ”ЧУ•SHHJK
КЭ›Щ™њЩ]ЉђШ[Щ[ЉJNВ€ЫY[ќ]^J
Щ[XЭЬ€OHФSУ”ЧУ•SJK
КЭ›Щ™њЩ]Љ‘Y][ЉJNВ€\]J
]™[OH•S
K
NВ‚€YЉЩ][™И€LJB€В€YЉ›Э™]ЪЩ^\И	€“QЧСTРКB€В€Ш]™Y]KљЩ^\ЦЬ^Y\—VЬЩ][™ЧHHЪОВ€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\М‹Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€Щ][™ИHLNВ€B€YЉЩ][™И€LJB€В€ИHЫЫќ›ЫЬШШ[љЩ^J
NВ€YЉКB€В€ШY™WЬЩ]
Ш]™Y]KљЩ^\ЦЬ^Y\—KЩ][™ЛЛЪКNВ€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\М‹Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€Щ][™ИHLNВ€ЛИ™]™[ќXШЪY[ќ[ШЬ™Y[њЪЭ€›Э™]ЪЩ^\ИHВ€B€B€B€[ЩB€В€YЉ›Э™]ЪЩ^\И	€“QЧСTРКB€В€]Z]HNВ€B€YЉ›Э™]ЪЩ^\И	€“QЧУSХ‘UT
B€В€В€В€YЉK\Щ[XЭЬ€
B€В€њ™XZОВ€B€B€Ъ[JЩ[XЭЬ€PVР•—У•SH	‰€\ШX›YЩ^VЬЩ[XЭЬ—JNВ€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€YЉ›Э™]ЪЩ^\И	€“QЧУSХ‘QХУЉB€В€В€В€YЉ
КЬЩ[XЭЬ€€PVР•—У•SHHJHњ™XZОВ€B€Ъ[J\ШX›YЩ^VЬЩ[XЭЬ—JNВ€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€YЉЩ[XЭЬ€
B€В€Щ[XЭЬ€HФSУ”ЧУ•SNВ€B€YЉЩ[XЭЬ€€ФSУ”ЧУ•SJB€В€Щ[XЭЬ€HВ€Ъ[J\ШX›YЩ^VЬЩ[XЭЬ—JHYЉ
КЬЩ[XЭЬ€€PVР•—У•SHHJHњ™XZОВ€B‚€YЉ›Э™]ЪЩ^\И	€
“QЧУSХ‘SQ•“QЧУSХ‘T’QТ“QЧРS–P•UУЉJB€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\М‹Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ‚€Y€
Щ[XЭЬ€OHФSУ”ЧУ•SHHИ	‰‚€›Э™]ЪЩ^\И	€
“QЧУSХ‘SQ•“QЧУSХ‘T’QТ
JHЫЫќ[ќYNВ‚€YЉЩ[XЭЬ€OHФSУ”ЧУ•SHHКB€В€Ш]™Y]Kљ›Ю\ќ[X›VЬ^Y\—HЏHNВ€B€[ЩHYЉЩ[XЭЬ€OHФSУ”ЧУ•SHHЉHЛИТВ€В€]Z]HЋВ€B€[ЩHYЉЩ[XЭЬ€OHФSУ”ЧУ•SHHJHЛИШ[Щ[€В€]Z]HNВ€B€[ЩHYЉЩ[XЭЬ€OHФSУ”ЧУ•SJHЛИY][€В€ЫX\ќ]ЫњК^Y\ЉNВ€B€[ЩB€В€Щ][™ИHЩ[XЭЬЋВ€ЪИHШ]™Y]KљЩ^\ЦЬ^Y\—VЬЩ][™ЧNВ€Ш]™Y]KљЩ^\ЦЬ^Y\—VЬЩ][™ЧHHВ€Щ^X›Ш\™ЩЩ]\ЭЩ^J
NВ€B€B€B€B‚€YЉ]Z]OHЉB€В€\WШЫЫќ›ЫК
NВ€Ш]™\Щ][™ЬК
NВ€B€[ЩB€В€ШYЩ][™ЬК
NВ€B‚€ШЬ™Y[—ЬЭ]\И	ЏH’S—ФРФ‘QS—Р•UУ—РУУ‘’QЧУQS•NВ‚€\]J
NВ€›Э™]ЪЩ^\ИHВ€љ[ќЉ‘Ы™HW€ЉNВџB‚ќ›ЪYY[ќWЫЬ[ЫњЧЪ[њ]

BћВ€[ќ]Z]HВ€[ќЩ[XЭЬ€HNИЛИ€[ќЬЬИHMЋВ€ЪY€S‘“ТQ€[ќФSУ”ЧУ•SHHЋВ€Щ[ЩB€[ќФSУ”ЧУ•SHHNВ€Щ[™Y‚‚€ШЬ™Y[—ЬЭ]\ИHS—ФРФ‘QS—РУУ•“УУФSУ”ЧУQS•NВ€›Э™]ЪЩ^\ИHВ‚€ЛИЬ]\И
LLЊЊJHYYHЩXЫЫ™[њЭ[ЩHЩ€HЫЫќ›ЫЪ[љ]€ќ[Э[Ы€Ъ[H[€HЫЫќ›ЫЬ[ЫњВ€ЛИ\ЩYќ[И™Yњ™\ЪЫЫYH^[њЫ][Ы€Y€H[™ЭXYЩH\ИЪ[™ЩY›Ы‹]KY›H€[™™KY]XЭ[XЭ]™HЫЫќ›ЫВ€ЫЫќ›ЫЪ[љ]
Ш]™Y]Kќ\ЩZ›ЮJNВ‚€Ъ[J\]Z]
B€В€ЫY[ќ]^J‹ЬЬЛLKЉђЫЫќ›ЫЬ[ЫњИЉJNВ€YЉШ]™Y]Kќ\ЩZ›ЮJB€В€ЫY[ќ]^

Щ[XЭЬ€OH
KЬЬЛL‹Љ‘Ш[YTYИ[X›YЉJNВ€YЉXЫЫќ›ЫЩЩ]›ЮY[X›Y

JB€В€ЫY[ќ]^

Щ[XЭЬ€OH
KЬЬКМLKL‹Љ€H]љXЩH›Э™XYHЉJNВ€B€B€[ЩB€В€ЫY[ќ]^

Щ[XЭЬ€OH
KЬЬЛL‹Љ‘Ш[YTYИ\ШX›YЉJNВ€B€ЫY[ќ]^

Щ[XЭЬ€OHJKЬЬЛLKЉ”Щ]\^Y\€K‹‹€ЉJNВ€ЫY[ќ]^

Щ[XЭЬ€OHЉKЬЬЛЉ”Щ]\^Y\€‹‹‹€ЉJNВ€ЫY[ќ]^

Щ[XЭЬ€OHКKЬЬЛKЉ”Щ]\^Y\€Л‹‹€ЉJNВ€ЫY[ќ]^

Щ[XЭЬ€OH
KЬЬЛ‹Љ”Щ]\^Y\€‹‹€ЉJNВ€ЪY€S‘“ТQ€YЉШ]™Y]Kљ\ЧЭЭXЪYЭљXњ][Ы—Щ[X›Y
B€В€ЫY[ќ]^J
Щ[XЭЬ€OHJKЉ•ЭXЪYљXњ][Ы€[X›YЉJNВ€B€[ЩB€В€ЫY[ќ]^J
Щ[XЭЬ€OHJKЉ•ЭXЪYљXњ][Ы€\ШX›YЉJNВ€B€ЫY[ќ]^J
Щ[XЭЬ€OHЉK‹ЉђXЪИЉJNВ€Щ[ЩB€ЫY[ќ]^J
Щ[XЭЬ€OHJKKЉђXЪИЉJNВ€Щ[™Y‚‚€\]J
]™[OH•S
K
NВ‚€YЉ›Э™]ЪЩ^\И	€“QЧСTРКB€В€]Z]HNВ€B€YЉ›Э™]ЪЩ^\И	€“QЧУSХ‘UT
B€В€K\Щ[XЭЬЋВ€YЉЫШ[ЬШ[\WЫ\Э™Y\ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€YЉ›Э™]ЪЩ^\И	€“QЧУSХ‘QХУЉB€В€
КЬЩ[XЭЬЋВ€YЉЫШ[ЬШ[\WЫ\Э™Y\ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€YЉЩ[XЭЬ€
B€В€Щ[XЭЬ€HФSУ”ЧУ•SNВ€B€YЉЩ[XЭЬ€€ФSУ”ЧУ•SJB€В€Щ[XЭЬ€HВ€B€YЉ›Э™]ЪЩ^\И	€
“QЧУSХ‘SQ•“QЧУSХ‘T’QТ“QЧРS–P•UУЉJB€В‚€YЉЫШ[ЬШ[\WЫ\Э™Y\М€ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\М‹Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B‚€ЭЪ]Ъ
Щ[XЭЬЉB€В€Ш\ЩH‚€ЫЫќ›ЫЭ\ЩZ›ЮJ
Ш]™Y]Kќ\ЩZ›ЮHЏHJJNВ€њ™XZОВ€Ш\ЩHN‚€Щ^X›Ш\™ЬЩ]\

NВ€њ™XZОВ€Ш\ЩHЋ‚€Щ^X›Ш\™ЬЩ]\
JNВ€њ™XZОВ€Ш\ЩHО‚€Щ^X›Ш\™ЬЩ]\
ЉNВ€њ™XZОВ€Ш\ЩH‚€Щ^X›Ш\™ЬЩ]\
КNВ€њ™XZОВ€ЪY€S‘“ТQ€Ш\ЩHN‚€Ш]™Y]Kљ\ЧЭЭXЪYЭљXњ][Ы—Щ[X›YЏHNВ€њ™XZОВ€Щ[™Y‚€Y][‚€]Z]H
›Э™]ЪЩ^\И	€“QЧРS–P•UУЉNВ€B€B€B€Ш]™\Щ][™ЬК
NВ€›Э™]ЪЩ^\ИHВ€ШЬ™Y[—ЬЭ]\И	ЏH’S—ФРФ‘QS—РУУ•“УУФSУ”ЧУQS•NВџB‚‚‹К‚€Ь]\И
‹LЊЊКH™XYќ\ЭYHX\Э\€ИY™™XЭ›ЫИ]\ЪXЭ›Ы[™ЩH[™[Ь™[Y[ќ€›Э[™[Y\ИZЩHLћHY][XZЩH[Ь™HЩ[њЩH[€ЫЫYHZЩHММLЊ€ЫЛH[њЩ™\њ™YH\™ЫЩY›Ы[YHYќ\ЭY[ќИHњЫЭ[™Z^€ЫЩH[њЭXYЩ€Ъ[™Ъ[™ИHY][›Ы[Y\В€X\Э\€›ЭИЫЩ\Ињ›ЫHИL[™Y][ИL€Y™™XЭИ
Ш[\\КH›ЭИЫЩ\Ињ›ЫHИL[™Y][ИL€]\ЪXИ›ЭИЫЩ\Ињ›ЫHИЊ[™Y][ИLЉ‹Вќ›ЪYY[ќWЫЬ[ЫњЧЬЫЭ[™

BћВ€[ќ]Z]HВ€[ќЩ[XЭЬ€HВ€[ќ\ЋВ€[ќЫЫHHNВ€[ќЫЫ€HЋВ‚€ШЬ™Y[—ЬЭ]\ИHS—ФРФ‘QS—ФУХS‘УФSУ”ЧУQS•NВ€›Э™]ЪЩ^\ИHВ‚€Ъ[J\]Z]
B€В€ЫY[ќ]^J‹MKЉ”ЫЭ[™Ь[ЫњИЉJNВ€ЫY[ќ]^

Щ[XЭЬ€OH
KЫЫKL‹Љ“X\Э\€›Ы[YN€ЉJNИЛИЬ]\И
‹LЊЊКH™[[YYњ›ЫHЫЭ[™ИX\Э\€›Ы[YB€ЫY[ќ]^

Щ[XЭЬ€OH
KЫЫ‹L‹‰ZH‹Ш]™Y]KњЫЭ[™›Ы
NВ€ЫY[ќ]^

Щ[XЭЬ€OHJKЫЫKLKЉ‘Y™™XЭИ›Ы[YN€ЉJNИЛИЬ]\И
‹LЊЊКH™[[YYњ›ЫHС–ИY™™XЭИ›Ы[YB€ЫY[ќ]^

Щ[XЭЬ€OHJKЫЫ‹LK‰ZH‹Ш]™Y]K™Y™™XЭ›Ы
NВ€ЫY[ќ]^

Щ[XЭЬ€OHЉKЫЫKЉ“]\ЪXИ›Ы[YN€ЉJNВ€ЫY[ќ]^

Щ[XЭЬ€OHЉKЫЫ‹‰ZH‹Ш]™Y]K›]\ЪXЭ›Ы
NВ€ЫY[ќ]^

Щ[XЭЬ€OHКKЫЫKKЉђ‘УN€ЉJNВ€ЫY[ќ]^

Щ[XЭЬ€OHКKЫЫ‹K
Ш]™Y]Kќ\Щ[]\ЪXИИЉ‘[X›YЉH€Љ‘\ШX›YЉJJNВ€ЫY[ќ]^

Щ[XЭЬ€OH
KЫЫK‹Љ”ЪЭИ]\О€ЉJNВ€ЫY[ќ]^

Щ[XЭЬ€OH
KЫЫ‹‹
Ш]™Y]KњЪЭЭ]\ИИЉ–Y\ИЉH€Љ“›ИЉJJNВ€ЫY[ќ]^J
Щ[XЭЬ€OHJKKЉђXЪИЉJNВ‚€\]J
]™[OH•S
K
NВ‚€YЉ›Э™]ЪЩ^\И	€“QЧСTРКB€В€]Z]HNВ€B€YЉ›Э™]ЪЩ^\И	€“QЧУSХ‘UT
B€В€K\Щ[XЭЬЋВ‚€YЉЫШ[ЬШ[\WЫ\Э™Y\ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€YЉ›Э™]ЪЩ^\И	€“QЧУSХ‘QХУЉB€В€
КЬЩ[XЭЬЋВ‚€YЉЫШ[ЬШ[\WЫ\Э™Y\ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€YЉЩ[XЭЬ€
B€В€Щ[XЭЬ€HNВ€B€YЉЩ[XЭЬ€€JB€В€Щ[XЭЬ€HВ€B‚€YЉ›Э™]ЪЩ^\И	€
“QЧУSХ‘SQ•“QЧУSХ‘T’QТ“QЧРS–P•UУЉJB€В€\€HВ‚€YЉ›Э™]ЪЩ^\И	€“QЧУSХ‘SQ•
B€В€\€HLNВ€B€[ЩHYЉ›Э™]ЪЩ^\И	€“QЧУSХ‘T’QТ
B€В€\€HNВ€B‚€YЉЫШ[ЬШ[\WЫ\Э™Y\М€ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\М‹Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B‚€ЛИЬ]\И
‹LЊЊКH™]ЫЬљЩYHЫ[™[ќ\ЩYќ[њЫЭ[™›Ы€ИXЭ\ИHX\Э\€›Ы[YH›Ь€›ЭЫЭ[™Ы]\ЪXВ€ЭЪ]Ъ
Щ[XЭЬЉB€В€Ш\ЩH‚€Ш]™Y]KњЫЭ[™›Ы
ПHH
€\ЋВ€YЉШ]™Y]KњЫЭ[™›Ы
B€В€Ш]™Y]KњЫЭ[™›ЫHВ€B€YЉШ]™Y]KњЫЭ[™›Ы€L
B€В€Ш]™Y]KњЫЭ[™›ЫHLВ€B€Р—ЬЩ]›Ы[YJР—Х“ТPСU“УШ]™Y]KњЫЭ[™›Ы
NВ‚€ЛИЬ]\И
‹LЊЊКHЮ[Ъ›Ыљ^™HЫЭ[™Ы]\ЪXИ›Ы[Y\ИИX\Э\‚€Ш]™Y]K™Y™™XЭ›ЫHШ]™Y]KњЫЭ[™›ЫВ€Ш]™Y]K›]\ЪXЭ›ЫHШ]™Y]KњЫЭ[™›ЫВ€њ™XZОВ€Ш\ЩHN‚€Ш]™Y]K™Y™™XЭ›Ы
ПHH
€\ЋВ€YЉШ]™Y]K™Y™™XЭ›Ы
B€В€Ш]™Y]K™Y™™XЭ›ЫHВ€B€YЉШ]™Y]K™Y™™XЭ›Ы€L
B€В€Ш]™Y]K™Y™™XЭ›ЫHLВ€B€њ™XZОВ€Ш\ЩHО‚€YЉY\ЉB€В€њ™XZОВ€B€YЉ\Ш]™Y]Kќ\Щ[]\ЪXКB€В€Ш]™Y]Kќ\Щ[]\ЪXИHNВ€]\ЪXК™]KЫ]\ЪXЛЬ™[Z^‹K
NВ€B€[ЩB€В€Ш]™Y]Kќ\Щ[]\ЪXИHВ€ЫЭ[™ШЫЬЩWЫ]\ЪXК
NВ€B€њ™XZОВ€Ш\ЩHЋ‚€Ш]™Y]K›]\ЪXЭ›Ы
ПHH
€\ЋВ€YЉШ]™Y]K›]\ЪXЭ›Ы
B€В€Ш]™Y]K›]\ЪXЭ›ЫHВ€B€YЉШ]™Y]K›]\ЪXЭ›Ы€Њ
B€В€Ш]™Y]K›]\ЪXЭ›ЫHЊВ€B€ЫЭ[™Э›Ы[YWЫ]\ЪXКШ]™Y]K›]\ЪXЭ›ЫШ]™Y]K›]\ЪXЭ›Ы
NВ€њ™XZОВ€Ш\ЩH‚€Ш]™Y]KњЪЭЭ]\ИH\Ш]™Y]KњЪЭЭ]\ОВ€њ™XZОВ€Y][‚€]Z]HNВ€B€B€B€Ш]™\Щ][™ЬК
NВ€›Э™]ЪЩ^\ИHВ€ШЬ™Y[—ЬЭ]\И	ЏH’S—ФРФ‘QS—ФУХS‘УФSУ”ЧУQS•NВџB‚ќ›ЪYY[ќWЫЬ[ЫњЧШЫЫ™љYК
HЛИЦ€ШYњ›ЫHИШ]™HИY][Щ™Л€™\ЭЬ™HЬ[ђ›Ф€™XЭЬћH€Щ][™ЬЛ‚ћВ€[ќ]Z]HВ€[ќЩ[XЭЬ€HВ€[ќШ]™YHВ€[ќШYYHВ€[ќ™\ЭЬ™YHВ‚€›Э™]ЪЩ^\ИHВ‚€ЛИЬ]\И
LKLЊЊJH™[[Э™YЬXЩ\ИЫ€H€Ы™HH€^Иљ^[њЫ][Ы‚€Ъ[J\]Z]
B€В€ЫY[ќ]^J‹MKЉђЫЫ™љYЭ\][Ы€Щ][™ЬИЉJNВ‚€YЉШ]™YOHJB€В€ЫY[ќ]^J
Щ[XЭЬ€OH
KLЛЉ”Ш]™HЩ][™ЬИИY][Щ™Й\ИЉKЉ€Ы™HHЉJNВ€B€[ЩB€В€ЫY[ќ]^J
Щ[XЭЬ€OH
KLЛЉ”Ш]™HЩ][™ЬИИY][Щ™Й\ИЉK€ЉNВ€B‚€YЉШYYOHJB€В€ЫY[ќ]^J
Щ[XЭЬ€OHJKL‹Љ“ШYЩ][™ЬИњ›ЫHY][Щ™Й\ИЉKЉ€Ы™HHЉJNВ€B€[ЩB€В€ЫY[ќ]^J
Щ[XЭЬ€OHJKL‹Љ“ШYЩ][™ЬИњ›ЫHY][Щ™Й\ИЉK€ЉNВ€B‚€YЉ™\ЭЬ™YOHJB€В€ЫY[ќ]^J
Щ[XЭЬ€OHЉKLKЉ”™\ЭЬ™HЬ[ђ›Ф€Y][Й\ИЉKЉ€Ы™HHЉJNВ€B€[ЩB€В€ЫY[ќ]^J
Щ[XЭЬ€OHЉKLKЉ”™\ЭЬ™HЬ[ђ›Ф€Y][Й\ИЉK€ЉNВ€B‚€ЫY[ќ]^J
Щ[XЭЬ€OHКK‹ЉђXЪИЉJNВ‚€\]J
]™[OH•S
K
NВ‚€YЉ›Э™]ЪЩ^\И	€“QЧСTРКB€В€]Z]HNВ€B€YЉ›Э™]ЪЩ^\И	€“QЧУSХ‘UT
B€В€K\Щ[XЭЬЋВ‚€YЉЫШ[ЬШ[\WЫ\Э™Y\ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€YЉ›Э™]ЪЩ^\И	€“QЧУSХ‘QХУЉB€В€
КЬЩ[XЭЬЋВ‚€YЉЫШ[ЬШ[\WЫ\Э™Y\ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B‚€YЉЩ[XЭЬ€
B€В€Щ[XЭЬ€HОВ€B€YЉЩ[XЭЬ€€КB€В€Щ[XЭЬ€HВ€B‚€YЉ›Э™]ЪЩ^\И	€
“QЧУSХ‘SQ•“QЧУSХ‘T’QТ“QЧРS–P•UУЉJB€В‚€YЉЫШ[ЬШ[\WЫ\Э™Y\М€ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\М‹Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B‚€ЭЪ]Ъ
Щ[XЭЬЉB€В€Ш\ЩH‚€Ш]™X\ЩY][

NВ€Ш]™YHNВ€њ™XZОВ‚€Ш\ЩHN‚€ШYњ›ЫYY][

NВ€ЛШ›Ь”Ъ]ЭЫЉ‹—”Щ][™ЬИШYYњ›ЫHY][Щ™Л€™\Э\ќ™\]Z\™Y——€ЉNВ€[љ]ЭљY[Ы[Щ\К
NВ€YЉ]љY[ЧЬЩ]Ы[ЩJљY[Ы[Щ\КJB€В€›Ь”Ъ]ЭЫЉK•[X›HИЩ]љY[И[ЩN€	Y	YW€‹љY[Ы[Щ\Лљ™\ЛљY[Ы[Щ\Лќ”™\КNВ€B€Р—ЬЩ]›Ы[YJР—Х“ТPСU“УШ]™Y]KњЫЭ[™›Ы
NВ€ЫЭ[™Э›Ы[YWЫ]\ЪXКШ]™Y]K›]\ЪXЭ›ЫШ]™Y]K›]\ЪXЭ›Ы
NВ€ШYYHNВ€њ™XZОВ€Ш\ЩHЋ‚€ЫX\њЩ][™ЬК
NВ€ЛШ›Ь”Ъ]ЭЫЉ‹—”Щ][™ЬИШYYњ›ЫHY][Щ™Л€™\Э\ќ™\]Z\™Y——€ЉNВ€[љ]ЭљY[Ы[Щ\К
NВ€YЉ]љY[ЧЬЩ]Ы[ЩJљY[Ы[Щ\КJB€В€›Ь”Ъ]ЭЫЉK•[X›HИЩ]љY[И[ЩN€	Y	YW€‹љY[Ы[Щ\Лљ™\ЛљY[Ы[Щ\Лќ”™\КNВ€B€Р—ЬЩ]›Ы[YJР—Х“ТPСU“УШ]™Y]KњЫЭ[™›Ы
NВ€ЫЭ[™Э›Ы[YWЫ]\ЪXКШ]™Y]K›]\ЪXЭ›ЫШ]™Y]K›]\ЪXЭ›Ы
NВ€™\ЭЬ™YHNВ€њ™XZОВ€Y][‚€]Z]HNВ€B€B€B€Ш]™\Щ][™ЬК
NВ€›Э™]ЪЩ^\ИHВџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊЌ‹LЛL‚Љ‚Љ€™]\›€Y[ќH\Ь^H^›Ь€HЫЫ\Ъ[Ы€XќYВЉ€\Ь^HZ\‹‚Љ‹ВњЭ]XИЪ\Љ€ЫЫ\Ъ[Ы—ЩXќYЧЭ\WЬЭљ[™КWЩXќYЧЩ\Ь^HXќYЧЩљY[WЩXќYЧЩ\Ь^H›]Шљ]WЩXќYЧЩ\Ь^H›Ъ™XЭYШљ]
HВ‚€Y€

XќYЧЩљY[	€›]Шљ]
H	‰€
XќYЧЩљY[	€›Ъ™XЭYШљ]
JHВ€™]\›€Љ‘›]
И›Ъ™XЭYЉNВ€H[ЩHY€
XќYЧЩљY[	€›]Шљ]
HВ€™]\›€Љ‘›]ЉNВ€H[ЩHY€
XќYЧЩљY[	€›Ъ™XЭYШљ]
HВ€™]\›€Љ”›Ъ™XЭYЉNВ€H[ЩHВ€™]\›€Љ‘\ШX›YЉNВ€BџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊЌ‹LЛL‚Љ‚Љ€ЮXЫHЫЫ\Ъ[Ы€XќYИ\Ь^H[ЩH›Ь€Ы™HЫЫ\Ъ[Ы€\K‚Љ‚Љ€XXЪЫЫ\Ъ[Ы€\H\Щ\ИЫИXќYИљ]О‚Љ‚Љ€›]Шљ]H]ИH›Ь›X[‘™XЭ[™ЫHЭ™\›^K‚Љ€›Ъ™XЭYШљ]H]ИH›Ъ™XЭYСЭX™HЭ™\›^K‚Љ‚Љ€HЫИљ]ИЫЫXљ[™H[ќИ›Э\€ЬЬЪX›HЭ]\О‚Љ‚Љ€H›Ы™BЉ€HH›]Љ€€H›Ъ™XЭYЉ€ИH›]
И›Ъ™XЭYЉ‚Љ€\™XЭ[Ы€ЫЫќ›ЫИЮXЫHЬ™\Ћ‚Љ‚Љ€ССУWСT‘PХSУ—РђPТХРT‘HЮXЫHXЪЭШ\™‚Љ€ССУWСT‘PХSУ—С“Ф•РT‘HЮXЫH›ЬќШ\™‚Љ‚Љ€™]\›€[YH\ИH\]YXќYИљ]љY[‚Љ‹ВњЭ]XИWЩXќYЧЩ\Ь^HЫЫ\Ъ[Ы—ЩXќYЧЭ\WШЮXЫJЫЫњЭWЩXќYЧЩ\Ь^HXќYЧЩљY[ЫЫњЭWЩXќYЧЩ\Ь^H›]Шљ]ЫЫњЭWЩXќYЧЩ\Ь^H›Ъ™XЭYШљ]ЫЫњЭWЫY[ќWЭЩЩЫWЩ\™XЭ[Ы€ЩЩЫWЩ\™XЭ[ЫЉHВ‚€К‚€
€ШШ[Ы›K€XЭX[Э]H\ИЫЫќ›ЫYћB€
€HШ]™Y]K™XќYЪ[™›Иљ]љY[€\Иќ\Э€
€Ъ]™\И\ИHЪ[\H[™X\€[YHИЫЬљИЪ]€
€›Ь€ЮXЫ[™ИЫИЩHЙЭ™YYHX\ЬЪ]™HY‹Щ[ЩB€
€ЪZ[‹‚€
‹В€ЩYљ[™HУУTТSУ—СP•QЧСTФVWУ“У‘H€ЩYљ[™HУУTТSУ—СP•QЧСTФVWС“UB€ЩYљ[™HУУTТSУ—СP•QЧСTФVWФ“Т‘PХQ‚€ЩYљ[™HУУTТSУ—СP•QЧСTФVWР“ХИ‚€К‚€
€[ќШЬ]ЪYЛ€\ЩH\™H[ќ[H[Y\Лќ]ЫЫ\[\‚€
€Ъ[›ЭИHљ]Y€ЩHћHИ\ЩH[H[€Ь\][ЫњВ€
€]Ш]\ЩH[\XЪ][ќYЩ\€›Ы[Э[Ы‹‚€
‹В€[ќЭ]NВ€[ќXќYЧЩ›YЬОВ‚€К‚€
€ЫЬљИ[€[€[ќШЬ]Ъ[YH™XШ]\ЩH[ќ[H[Y\И\™B€
€Э[љ]љY[Л[™љ]Ъ\ЩHЬ\]ЬњИ›Ы[ЭH[B€
€И[ќ[ћ]Ш^K‚€
‹В€XќYЧЩ›YЬИHXќYЧЩљY[В‚€К‚€
€ЫЫќ™\ќЭ\њ™[ќљ]љY[Э]H[ќИHЫЫ\XЭ€
€\Ь^H[ЩH[YK‚€
‚€
€љ]H›]\Ь^H[X›Y‚€
€љ]HH›Ъ™XЭY\Ь^H[X›Y‚€
‹В€Э]HHУУTТSУ—СP•QЧСTФVWУ“У‘NВ‚€Y€
XќYЧЩ›YЬИ	€›]Шљ]
HВ€Э]HHУУTТSУ—СP•QЧСTФVWС“UВ€B‚€Y€
XќYЧЩ›YЬИ	€›Ъ™XЭYШљ]
HВ€Э]HHУУTТSУ—СP•QЧСTФVWФ“Т‘PХQВ€B‚€К‚€
€Э\›ЭYЪH›Э\€\Ь^H[Щ\Л‚€
‚€
€XЪЭШ\™‚€
€›Ы™HH›]H›Ъ™XЭYH›ЭH›Ы™B€
‚€
€›ЬќШ\™‚€
€›Ы™HO€›]O€›Ъ™XЭYO€›ЭO€›Ы™B€
‹В€ЭЪ]Ъ
ЩЩЫWЩ\™XЭ[ЫЉHВ€Ш\ЩHССУWСT‘PХSУ—РђPТХРT‘‚€Э]KKNВ‚€Y€
Э]HУУTТSУ—СP•QЧСTФVWУ“У‘JHВ€Э]HHУУTТSУ—СP•QЧСTФVWР“ХВ€B‚€њ™XZОВ‚€Ш\ЩHССУWСT‘PХSУ—С“Ф•РT‘‚€Y][‚€Э]JКОВ‚€Y€
Э]H€УУTТSУ—СP•QЧСTФVWР“Х
HВ€Э]HHУУTТSУ—СP•QЧСTФVWУ“У‘NВ€B‚€њ™XZОВ€B‚€К‚€
€ЫX\€Ы›HHЫИљ]ИX[YЩYћH\ИЫЫ\Ъ[Ы‚€
€XќYИЬ[Ы‹€X]™H[Э\€XќYИ›YЬИ[ќЭXЪY‚€
‹В€XќYЧЩ›YЬИ	ЏHЉ›]Шљ]›Ъ™XЭYШљ]
NВ‚€К‚€
€^[™ЫЫ\XЭЭ]HXЪИ[ќИHXќYИљ]љY[‚€
‹В€Y€
Э]H	€УУTТSУ—СP•QЧСTФVWС“U
HВ€XќYЧЩ›YЬИH›]Шљ]В€B‚€Y€
Э]H	€УУTТSУ—СP•QЧСTФVWФ“Т‘PХQ
HВ€XќYЧЩ›YЬИH›Ъ™XЭYШљ]В€B‚€™]\›€
WЩXќYЧЩ\Ь^JYXќYЧЩ›YЬОВ‚€Э[™Y€УУTТSУ—СP•QЧСTФVWУ“У‘B€Э[™Y€УУTТSУ—СP•QЧСTФVWС“U€Э[™Y€УУTТSУ—СP•QЧСTФVWФ“Т‘PХQ€Э[™Y€УУTТSУ—СP•QЧСTФVWР“ХџB‚ќ›ЪYY[ќWЫЬ[ЫњЧЩXќYК
HВ€ЩYљ[™HQS•WФФЧЦHM€ЩYљ[™HQS•WТUSTЧУPT‘ТS—ЦH‚€ЩYљ[™HУУSS—МWФФЧЦLLB€ЩYљ[™HУУSS—М—ФФЧЦУУSS—МWФФЧЦ
ИM€ЩYљ[™HQS•WТUSWС’T”ХТS‘V‚€ЛИЩ[XЭ[ЫњИ[ќ[Y\]Ь‹€[€ЛИЩ[XЭ[Ы€][\ИЪЭ[™HXЩY€ЛИ\™Hљ\њЭ‚€\YY€[ќ[HВ‚€ЛИљ\њЭ][HШ[€™B€ЛИЪ]]™\€ЩHZЩK€ЛИќ]]]\Э™HЩ]€ЛИИHQS•WТUSWС’T”ХТS‘V€ЛИЫЫњЭ[ќ‚€USWФT‘“Ф“PSђСHHQS•WТUSWС’T”ХТS‘V‚€ЛИ][\И™]ЩY[€Hљ\њЭ€ЛИ[™\ЭШ[€ЫИ[€[ћB€ЛИЬ™\‹‚€USWФФТUSУ‹€USWРУУРUPТЛ€USWРУУР“СK€USWРУУФФPСK€USWРУУФђS‘СK‚€ЛИ\И\ИHђXЪИ‚€ЛИЩ[XЭ[Ы€[™ЪЭ[€ЛИ[Ш^\И™H\Э‚€USWСVU€HWЬЩ[XЭ[ЫњОВ‚€[ќЬЧЮNВ€[ќ]Z]HВ€WЬЩ[XЭ[ЫњИЩ[XЭЬ€HВ€›Э™]ЪЩ^\ИHВ€€WЫY[ќWЭЩЩЫWЩ\™XЭ[Ы€ЩЩЫWЩ\™XЭ[Ы€HССУWСT‘PХSУ—С“Ф•РT‘В‚€Ъ[J\]Z]
HВ‚€ЛИ\Ь^HY[ќH]K‚€ЫY[ќ]^J‹QS•WФФЧЦKЉ‘XќYИЩ][™ЬИЉJNВ‚€ЛИY[ќH][\Л‚€ЛИHЬЪ][Ы€\ИЫЫќ›ЫYћHH[Ь™[Y[ќY[ќYЩ\‹‚€ЛИ[ќYЩ\€\ИH\ЩH[YHЩ€HY[ќH]HB€ЛИ\ИЭ]XИЩ™њЩ]€™[ЭИXXЪ][K\ИH[YB€ЛИ[Ь™[Y[ќИћHЫ™K€ЩHШ[€[€YЬ€™[[Э™B€ЛИY[ќH][\И\™H[™HЫ€ШЬ™Y[€ЬЪ][Ы€Ъ[€ЛИZЩHШ\™HЩ€]Щ[€Ъ]Э]\И™YY[™ИИY\ЬВ€ЛИЪ]Hќ[ЪЩ€\™ЫЫњЭ[ќЛ‚‚€ЛИ™\Щ]Y[ќH][HЬЪ][Ы€K‚€ЬЧЮHHQS•WФФЧЦH
ИQS•WТUSTЧУPT‘ТS—ЦNВ‚€ЫY[ќ]^

Щ[XЭЬ€OHUSWФT‘“Ф“PSђСJKУУSS—МWФФЧЦЬЧЮKЉ”\™›Ь›X[ЩN€ЉJNВ€ЫY[ќ]^

Щ[XЭЬ€OHUSWФT‘“Ф“PSђСJKУУSS—М—ФФЧЦЬЧЮK
Ш]™Y]K™XќYЪ[™›И	€P•QЧСTФVWФT‘“Ф“PSђСHИЉ‘[X›YЉH€Љ‘\ШX›YЉJJNВ€ЬЧЮJКОВ‚€ЫY[ќ]^

Щ[XЭЬ€OHUSWФФТUSУЉKУУSS—МWФФЧЦЬЧЮKЉђ\ЪXИ›Ь\ќY\О€ЉJNВ€ЫY[ќ]^

Щ[XЭЬ€OHUSWФФТUSУЉKУУSS—М—ФФЧЦЬЧЮK
Ш]™Y]K™XќYЪ[™›И	€P•QЧСTФVWФ“ФT•QTИИЉ‘[X›YЉH€Љ‘\ШX›YЉJJNВ€ЬЧЮJКОИ‚€ЫY[ќ]^

Щ[XЭЬ€OHUSWРУУРUPТКKУУSS—МWФФЧЦЬЧЮKЉђЫЫ\Ъ[Ы€]XЪО€ЉJNВ€ЫY[ќ]^

Щ[XЭЬ€OHUSWРУУРUPТКKУУSS—М—ФФЧЦЬЧЮKЫЫ\Ъ[Ы—ЩXќYЧЭ\WЬЭљ[™КШ]™Y]K™XќYЪ[™›ЛP•QЧСTФVWРУУTТSУ—РUPТЧМ‘P•QЧСTФVWРУУTТSУ—РUPТЧМС
JNВ€ЬЧЮJКОВ‚€ЫY[ќ]^

Щ[XЭЬ€OHUSWРУУР“СJKУУSS—МWФФЧЦЬЧЮKЉђЫЫ\Ъ[Ы€›ЩN€ЉJNВ€ЫY[ќ]^

Щ[XЭЬ€OHUSWРУУР“СJKУУSS—М—ФФЧЦЬЧЮKЫЫ\Ъ[Ы—ЩXќYЧЭ\WЬЭљ[™КШ]™Y]K™XќYЪ[™›ЛP•QЧСTФVWРУУTТSУ—Р“СWМ‘P•QЧСTФVWРУУTТSУ—Р“СWМС
JNВ€ЬЧЮJКОВ‚€ЫY[ќ]^

Щ[XЭЬ€OHUSWРУУФФPСJKУУSS—МWФФЧЦЬЧЮKЉђЫЫ\Ъ[Ы€ЬXЩN€ЉJNВ€ЫY[ќ]^

Щ[XЭЬ€OHUSWРУУФФPСJKУУSS—М—ФФЧЦЬЧЮKЫЫ\Ъ[Ы—ЩXќYЧЭ\WЬЭљ[™КШ]™Y]K™XќYЪ[™›ЛP•QЧСTФVWРУУTТSУ—ФФPСWМ‘P•QЧСTФVWРУУTТSУ—ФФPСWМС
JNВ€ЬЧЮJКОВ‚€ЫY[ќ]^

Щ[XЭЬ€OHUSWРУУФђS‘СJKУУSS—МWФФЧЦЬЧЮKЉ”[™ЩN€ЉJNВ€ЫY[ќ]^

Щ[XЭЬ€OHUSWРУУФђS‘СJKУУSS—М—ФФЧЦЬЧЮK
Ш]™Y]K™XќYЪ[™›И	€P•QЧСTФVWФђS‘СHИЉ‘[X›YЉH€Љ‘\ШX›YЉJJNВ€ЬЧЮJКОВ‚€ЛИ\Ь^H^]]B€ЫY[ќ]^J
Щ[XЭЬ€OHUSWСVU
KЬЧЮH
И
QS•WТUSTЧУPT‘ТS—ЦJKЉђXЪИЉJNВ‚€ЛИќ[€[€[™Ъ[™H\]K‚€\]J
]™[OH•S
K
NВ‚€ЛИY€\Щ\€™\ЬЩ\И\ЩЭЫ€Ь€\ШЛ]	ЬИXЭXШЫЬ™[™ЫK‚€YЉ›Э™]ЪЩ^\И	€
“QЧУSХ‘UT“QЧУSХ‘QХУ€“QЧСTРКJHВ‚€ЛИY€\Щ\€™\ЬЩ\И\ШШ\K[€Щ]]Z]€ЛИ›YИ[[YYX][K€[ЩHЪ\ЩK[Ь™[Y[ќ€ЛИЬ€XЬ™[Y[ќЩ[XЭЬ€\И™YYY‚€YЉ›Э™]ЪЩ^\И	€“QЧСTРКHВ€]Z]HNВ€€H[ЩHYЉ›Э™]ЪЩ^\И	€“QЧУSХ‘UT
HВ€ЛИ^H™Y\Y€]Z[X›K‚€ЛИЬ]\И
LЊЊЉH[Э™YH‘QTЫЭ[™ИЫЬљИЫ›HЪ]TСХУ€Щ^\В€YЉЫШ[ЬШ[\WЫ\Э™Y\ЏH
HВ€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B‚€ЛИY€ЩH\™H]HЬ][KЫЬ€ЛИИ\Э€Э\ќЪ\ЩK[Э™HЫ™H\‚€YЉЩ[XЭЬ€HQS•WТUSWС’T”ХТS‘V
HВ€Щ[XЭЬ€HUSWСVUВ€H[ЩHВ€K\Щ[XЭЬЋВ€B‚€H[ЩHYЉ›Э™]ЪЩ^\И	€“QЧУSХ‘QХУЉHВ‚€ЛИ^H™Y\Y€]Z[X›K‚€ЛИЬ]\И
LЊЊЉH[Э™YH‘QTЫЭ[™ИЫЬљИЫ›HЪ]TСХУ€Щ^\В€YЉЫШ[ЬШ[\WЫ\Э™Y\ЏH
HВ€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B‚€ЛИY€ЩH\™H]H\Э][B€ЛИ
ЪXЪЪЭ[™HXЪИЉK[‚€ЛИЫЬXЪИИљ\њЭ€Э\ќЪ\ЩB€ЛИ[Э™HЫ™HЭЫ‹‚€YЉЩ[XЭЬ€ЏHUSWСVU
HВ€Щ[XЭЬ€HQS•WТUSWС’T”ХТS‘VВ€€H[ЩHВ€
КЬЩ[XЭЬЋВ€B€B€B‚‚€ЛИЩЩЫHЩ[XЭ[Ы€[YHЫ€YќЬљYЪЬ‚€ЛИљYЩЩ\€ќ]Ы€™\ЬЛ‚€YЉ›Э™]ЪЩ^\И	€
“QЧУSХ‘SQ•“QЧУSХ‘T’QТ“QЧРS–P•UУЉJHВ€€YЉЫШ[ЬШ[\WЫ\Э™Y\М€ЏH
HВ€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\М‹Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B‚€ЩЩЫWЩ\™XЭ[Ы€H
›Э™]ЪЩ^\И	€“QЧУSХ‘SQ•
HИССУWСT‘PХSУ—РђPТХРT‘€ССУWСT‘PХSУ—С“Ф•РT‘В€€ЛИ\И\ИЪ\™HY[ќH][\И\™H^XЭ]Y‚€ЭЪ]Ъ
Щ[XЭЬЉHВ€Ш\ЩHUSWФT‘“Ф“PSђСN‚€Ш]™Y]K™XќYЪ[™›ИЏHP•QЧСTФVWФT‘“Ф“PSђСNВ€њ™XZОВ€Ш\ЩHUSWФФТUSУЋ‚€Ш]™Y]K™XќYЪ[™›ИЏHP•QЧСTФVWФ“ФT•QTОВ€њ™XZОИ€Ш\ЩHUSWРУУРUPТО‚€Ш]™Y]K™XќYЪ[™›ИHЫЫ\Ъ[Ы—ЩXќYЧЭ\WШЮXЫJШ]™Y]K™XќYЪ[™›ЛP•QЧСTФVWРУУTТSУ—РUPТЧМ‘P•QЧСTФVWРУУTТSУ—РUPТЧМСЩЩЫWЩ\™XЭ[ЫЉNВ€њ™XZОВ€Ш\ЩHUSWРУУР“СN‚€Ш]™Y]K™XќYЪ[™›ИHЫЫ\Ъ[Ы—ЩXќYЧЭ\WШЮXЫJШ]™Y]K™XќYЪ[™›ЛP•QЧСTФVWРУУTТSУ—Р“СWМ‘P•QЧСTФVWРУУTТSУ—Р“СWМСЩЩЫWЩ\™XЭ[ЫЉNВ€њ™XZОВ€Ш\ЩHUSWРУУФФPСN‚€Ш]™Y]K™XќYЪ[™›ИHЫЫ\Ъ[Ы—ЩXќYЧЭ\WШЮXЫJШ]™Y]K™XќYЪ[™›ЛP•QЧСTФVWРУУTТSУ—ФФPСWМ‘P•QЧСTФVWРУУTТSУ—ФФPСWМСЩЩЫWЩ\™XЭ[ЫЉNВ€њ™XZОВ€Ш\ЩHUSWРУУФђS‘СN‚€Ш]™Y]K™XќYЪ[™›ИЏHP•QЧСTФVWФђS‘СNВ€њ™XZОВ€Ш\ЩHUSWСVU‚€]Z]HNВ€B€B€B‚€Ш]™\Щ][™ЬК
NВ€›Э™]ЪЩ^\ИHВ‚€Э[™Y€QS•WФФЧЦB€Э[™Y€QS•WТUSTЧУPT‘ТS—ЦB€Э[™Y€УУSS—МWФФЧЦ€Э[™Y€УУSS—М—ФФЧЦ€Э[™Y€QS•WТUSWС’T”ХТS‘VџB‚‹К‚Љ€Ш\ЪЩ^K[[Ы€‹‚Љ€ЊЊ‹LKLBЉ€Љ€\Ь^HЪX]Ь[ЫњИY[ќH[™Љ€\H^Y\€Щ[XЭ[ЫњЛ‚Љ‹Вќ›ЪYY[ќWЫЬ[ЫњЧШЪX]К
BћВ€ЩYљ[™HQS•WФФЧЦHM€ЩYљ[™HQS•WТUSTЧУPT‘ТS—ЦH‚€ЩYљ[™HУУSS—МWФФЧЦLLB€ЩYљ[™HУУSS—М—ФФЧЦУУSS—МWФФЧЦ
ИM‚€\YY€ЭќXЭЧЫЬ[Ы—Ы\Э€В€WШЪX]ЫЬ[ЫњИXЭ]™NВ€WШЪX]ЫЬ[ЫњИY[ќNВ€Ъ\€X™[МЊNВ€HЧЫЬ[Ы—Ы\ЭВ‚€[ќЬ[Ы—Ы\ЭШЫЭ[ќHОВ€ЧЫЬ[Ы—Ы\ЭЬ[Ы—Ы\ЭЧHHВ€ЛXЭ]™HHТPUУФSУ”ЧТSTPРP“WРPХU‘K›Y[ќHHТPUУФSУ”ЧТSTPРP“WУQS•K›X™[H’[\XШX›HX\Ъ€€K€ЛXЭ]™HHТPUУФSУ”ЧРФ‘QUЧРPХU‘K›Y[ќHHТPUУФSУ”ЧРФ‘QUЧУQS•K›X™[H’[™љ[љ]HЬ™Y]О€€K€ЛXЭ]™HHТPUУФSУ”ЧСS‘T‘ЦWРPХU‘K›Y[ќHHТPUУФSУ”ЧСS‘T‘ЦWУQS•K›X™[H’[™љ[љ]H[™\™ЮN€€K€ЛXЭ]™HHТPUУФSУ”ЧТPSРPХU‘K›Y[ќHHТPUУФSУ”ЧТPSУQS•K›X™[H’[™љ[љ]HX[€€K€ЛXЭ]™HHТPUУФSУ”ЧУU‘TЧРPХU‘K›Y[ќHHТPUУФSУ”ЧУU‘TЧУQS•K›X™[H’[™љ[љ]H]™\О€€K€ЛXЭ]™HHТPUУФSУ”ЧУUSRUРPХU‘K›Y[ќHHТPUУФSУ”ЧУUSRUУQS•K›X™[H“][Z]Ы]Ъ€€K€ЛXЭ]™HHТPUУФSУ”ЧХСРPХU‘K›Y[ќHHТPUУФSУ”ЧХСУQS•K›X™[H•ЭXЪЩ€X]€€B€NВ€€[ќЬ[Ы—Ы\ЭШЭ\њЫЬ€HВ€[ќЬ[Ы—Ш]Z[X›WШЫЭ[ќHВ€[ќЬ[Ы—Ш]Z[X›WЩљ\њЭHВ€[ќЬЧЮHHВ€[ќ]Z]HВ€[ќЩ[XЭЬ€HВ€€›Э™]ЪЩ^\ИHВ‚€ШЬ™Y[—ЬЭ]\ИHS—ФРФ‘QS—РТPUУФSУ”ЧУQS•NИ‚€ЛЬљ[ќЉ——€Y[ќWЫЬ[ЫњЧШЪX]К
HЉNВ‚€Ъ[H
\]Z]
B€В€К€\Ь^HY[ќH]K€
‹В€ЫY[ќ]^J‹QS•WФФЧЦKЉђЪX]Ь[ЫњИЉJNВ€€К‚€
€™\Щ]Y[ќH][HЬЪ][Ы€H[™€
€[YY[ќH][HЫЭ[ќ‚€
‹В€Ь[Ы—Ш]Z[X›WШЫЭ[ќHВ€Ь[Ы—Ш]Z[X›WЩљ\њЭHВ€ЬЧЮHHQS•WФФЧЦH
ИQS•WТUSTЧУPT‘ТS—ЦNВ‚€К‚€
€]\]H›ЭYЪHЬ[Ы€\Э€
€\њ^H[™\Ь^H]Z[X›H][\Л‚€
€€
€]XXЪ[YЬ[Ы‹[Ь™[Y[ќ€
€Ь[Ы€ЫЭ[ќ[™HЬЪ][Ы‹‚€
‹В‚€›Ь€
Ь[Ы—Ы\ЭШЭ\њЫЬ€HИЬ[Ы—Ы\ЭШЭ\њЫЬ€Ь[Ы—Ы\ЭШЫЭ[ќИЬ[Ы—Ы\ЭШЭ\њЫЬЉККB€И€Y€
ЫШ[ШЫЫ™љYЛЪX]И	€Ь[Ы—Ы\ЭЫЬ[Ы—Ы\ЭШЭ\њЫЬ—K›Y[ќJB€И€К€€
€Y€Ь[Ы€]Z[X›HЫЭ[ќ\ИЭ[\В€
€\ИHљ\њЭ]Z[X›HЬ[Ы‹€™XЫЬ™B€
€[™^›Ь€\ЩHЭЫњЭ™X[K‚€
‹В‚€Y€
[Ь[Ы—Ш]Z[X›WШЫЭ[ќ
B€В€Ь[Ы—Ш]Z[X›WЩљ\њЭHЬ[Ы—Ы\ЭШЭ\њЫЬЋВ€B€€К€љ[ќHY[ќH^[™Ы‹ЫЩ™€Э]\Л€
‹В€ЫY[ќ]^

Щ[XЭЬ€OHЬ[Ы—Ы\ЭШЭ\њЫЬЉKУУSS—МWФФЧЦЬЧЮKЉЬ[Ы—Ы\ЭЫЬ[Ы—Ы\ЭШЭ\њЫЬ—K›X™[
JNВ€ЫY[ќ]^

Щ[XЭЬ€OHЬ[Ы—Ы\ЭШЭ\њЫЬЉKУУSS—М—ФФЧЦЬЧЮK
ЫШ[ШЫЫ™љYЛЪX]И	€Ь[Ы—Ы\ЭЫЬ[Ы—Ы\ЭШЭ\њЫЬ—KXЭ]™HИЉ“Ы€ЉH€Љ“Щ™€ЉJJNВ€€Ь[Ы—Ш]Z[X›WШЫЭ[ќ
КОВ€ЬЧЮJКОВ€H€B‚€К€›И][\И]ZX[›K€[\ќ^Y\‹€
‹В€Y€
[Ь[Ы—Ш]Z[X›WШЫЭ[ќ
B€В€ЫY[ќ]^JЬЧЮKЉ“›ИЪX]И]Z[X›K€ЉJNВ€H‚€К‚€
€YH^]Ь[Ы‹€ЩHШ[€\ЩHH\њ^HЪ^™H€
€›Ь€HЩ[XЭЬ€[™^YHИ[™^[™Л‚€
‹В€€ЬЧЮJКОИ€ЫY[ќ]^J
Щ[XЭЬ€OHЬ[Ы—Ы\ЭШЫЭ[ќ
KЬЧЮKЉђXЪИЉJNВ‚€К€ќ[€[€[™Ъ[™H\]K€
‹В€\]J
]™[OH•S
K
NВ€€К€€
€[™H\Щ\€[њ][™Ь[]HЩ[XЭЬ€В€
€\›ЬљX]H[YK€€
€€
€љ\њЭЩHЩ]Щ[XЭЬ€Иљ\њЭ][H[€Ш\ЩH€
€Hљ\њЭ]Z[X›IЬИ[™^\И›Э‚€
€€
€™^[™Hќ]ЫњЛ€TРИ]Z]ИHY[ќH€
€[њЭ[ќK€\СЭЫ€Щ[XЭ\™Щ]][K‚€
€YќФљYЪРXЭ[Ы€ќ]ЫњИЩЩЫH[YK‚€
‹В‚€Y€
\Щ[XЭЬ€	‰€Ь[Ы—Ш]Z[X›WЩљ\њЭ
B€В€Щ[XЭЬ€HЬ[Ы—Ш]Z[X›WЩљ\њЭВ€B‚€Y€
›Э™]ЪЩ^\И	€“QЧСTРКB€В€]Z]HNВ€B‚€Y€
›Э™]ЪЩ^\И	€“QЧУSХ‘UT
B€В€К€^H™Y\Y€]Z[X›K€
‹В€Y€
ЫШ[ЬШ[\WЫ\Э™Y\ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B‚€К€Ь\\›Э[™Y€™YYY€
‹В€Y€
Щ[XЭЬ€HЬ[Ы—Ш]Z[X›WЩљ\њЭ
B€В€Щ[XЭЬ€HЬ[Ы—Ы\ЭШЫЭ[ќВ€B€[ЩB€В€Щ[XЭЬ‹KNВ€B‚€К€ЪЪ\\ШX›Y][\Л€
‹В€Ъ[H
JЫШ[ШЫЫ™љYЛЪX]И	€Ь[Ы—Ы\ЭЬЩ[XЭЬ—K›Y[ќJH	‰€Щ[XЭЬ€€Ь[Ы—Ш]Z[X›WЩљ\њЭ
B€И€Щ[XЭЬ‹KNВ€H€B€€Y€
›Э™]ЪЩ^\И	€“QЧУSХ‘QХУЉB€В€К€^H™Y\Y€]Z[X›K€
‹В€Y€
ЫШ[ЬШ[\WЫ\Э™Y\ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B‚€К€Ь\\›Э[™Y€™YYY€
‹В€Y€
Щ[XЭЬ€ЏHЬ[Ы—Ы\ЭШЫЭ[ќ
B€В€Щ[XЭЬ€HЬ[Ы—Ш]Z[X›WЩљ\њЭВ€B€[ЩB€В€Щ[XЭЬЉКОВ€B‚€К€ЪЪ\\ШX›Y][\Л€
‹В€Ъ[H
JЫШ[ШЫЫ™љYЛЪX]И	€Ь[Ы—Ы\ЭЬЩ[XЭЬ—K›Y[ќJH	‰€Щ[XЭЬ€Ь[Ы—Ы\ЭШЫЭ[ќ
B€В€Щ[XЭЬЉКОВ€B€H‚€К€€
€ЩЩЫHЩ[XЭ[Ы€[YHЫ€YќЬљYЪЬ‚€
€љYЩЩ\€ќ]Ы€™\ЬЛ‚€
‹В€Y€
›Э™]ЪЩ^\И	€
“QЧУSХ‘SQ•“QЧУSХ‘T’QТ“QЧРS–P•UУЉJB€В€Y€
ЫШ[ЬШ[\WЫ\Э™Y\М€ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\М‹Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B‚€К‚€
€ZЩHXЭ[Ы€\ЩYЫ€Щ[XЭЬ‹‚€
€€
€H\ЭЬ[Ы€\ИH^]][KЫИY€]	ЬВ€
€Э\€Щ[XЭЬ€[YHЩH^]\ИY[ќK€€
‚€
€Э\ќЪ\ЩH\ЩHЩ[XЭЬ€\И\њ^HЩ^HИЩ]H€
€љ]ЩHШ[ќИЩЩЫH[€Э\€\™Щ][YK‚€
‹В‚€Y€
Щ[XЭЬ€Ь[Ы—Ы\ЭШЫЭ[ќ
B€В€ЫШ[ШЫЫ™љYЛЪX]ИЏHЬ[Ы—Ы\ЭЬЩ[XЭЬ—KXЭ]™NВ€B€[ЩB€В€]Z]HNВ€H€B€B€€ЛЬШ]™\Щ][™ЬК
NВ€€›Э™]ЪЩ^\ИHВ€ШЬ™Y[—ЬЭ]\И	ЏH’S—ФРФ‘QS—РТPUУФSУ”ЧУQS•NВ‚€Э[™Y€QS•WФФЧЦB€Э[™Y€QS•WТUSTЧУPT‘ТS—ЦB€Э[™Y€УУSS—МWФФЧЦ€Э[™Y€УУSS—М—ФФЧЦџB‚ќ›ЪYY[ќWЫЬ[ЫњЧЬЮ\Э[J
BћВ€ЩYљ[™HЦTЧУФЦWФФИM‚€[ќ[HВ€ЦTЧУФУСЛ€ЦTЧУФХ”СSPQСK€ЦTЧУФРТPUЛ€ЦTЧУФСP•QЛ€ЦTЧУФРУУ‘’QЛ€ЦTЧУФРђPТВ€NВ‚€[ќ]Z]HВ€[ќЩ[XЭЬ€HВ€[ќЫЫHHLLNВ€[ќЫЫ€HЫЫJМMВ€[ќ^ЫX™[ИHВ€[ќ‘UHЦTЧУФРђPТОВ€[ќ[™HHВ‚€ШЬ™Y[—ЬЭ]\ИHS—ФРФ‘QS—ФЦTХSWУФSУ”ЧУQS•NВ€›Э™]ЪЩ^\ИHВ€Y€
›ЩXќYЫЬ[ЫњКH^ЫX™[ИHNВ€‘UOH^ЫX™[ОВ€€Ъ[J\]Z]
B€В€[™HHВ‚€ЫY[ќ]^J‹ЦTЧУФЦWФФЛL‹Љ”Ю\Э[HЬ[ЫњИЉJNВ‚€ЫY[ќ]^
ЫЫKЦTЧУФЦWФФИ
И[™KЉ•Э[ђSN€ЉJNВ€ЫY[ќ]^
ЫЫ‹ЦTЧУФЦWФФИ
И[™KЉ‰\ИР€ЉKЫЫ[X\љ[ќ
Щ]Ю\Э[T[JР–UTКJJNВ€[™JКОВ€€ЫY[ќ]^
ЫЫKЦTЧУФЦWФФИ
И[™KЉ•\ЩYђSN€ЉJNВ€ЫY[ќ]^
ЫЫ‹ЦTЧУФЦWФФИ
И[™KЉ‰\ИР€ЉKЫЫ[X\љ[ќ
Щ]\ЩY[JР–UTКJJNВ€[™JКОВ‚€ЫY[ќ]^
ЫЫKЦTЧУФЦWФФИ
И[™KЉ“X^^Y\њО€ЉJNВ€ЫY[ќ]^
ЫЫ‹ЦTЧУФЦWФФИ
И[™KЉ‰ZHЉK]™[Щ]ЦШЭ\њ™[ќЬЩ]K›X^^Y\њКNВ€[™JКОВ‚€ЫY[ќ]^

Щ[XЭЬ€OHЦTЧУФУСКKЫЫKЦTЧУФЦWФФИ
И[™KЉ‘љ[HЩЩЪ[™О€ЉJNВ€ЫY[ќ]^

Щ[XЭЬ€OHЦTЧУФУСКKЫЫ‹ЦTЧУФЦWФФИ
И[™K
Ш]™Y]Kќ\Щ[ЩИИЉ‘[X›YЉH€Љ‘\ШX›YЉJJNВ€[™JКОВ‚€ЫY[ќ]^

Щ[XЭЬ€OHЦTЧУФХ”СSPQСJKЫЫKЦTЧУФЦWФФИ
И[™KЉ•™\њЭ\И[XYЩN€ЉK
NВ€YЉ™\њЭ\Щ[XYЩHOH
B€В€ЫY[ќ]^

Щ[XЭЬ€OHЦTЧУФХ”СSPQСJKЫЫ‹ЦTЧУФЦWФФИ
И[™KЉ‘\ШX›YћH[Щ[HЉJNВ€B€[ЩHYЉ™\њЭ\Щ[XYЩHOHJB€В€ЫY[ќ]^

Щ[XЭЬ€OHЦTЧУФХ”СSPQСJKЫЫ‹ЦTЧУФЦWФФИ
И[™KЉ‘[X›YћH[Щ[HЉJNВ€B€[ЩB€В€YЉШ]™Y]K›[ЩJB€В€ЫY[ќ]^

Щ[XЭЬ€OHЦTЧУФХ”СSPQСJKЫЫ‹ЦTЧУФЦWФФИ
И[™KЉ‘\ШX›YЉJNИЛУ[ЩHHH^Y\њИРS‰Х]XЪИXXЪЭ\‚€B€[ЩB€В€ЫY[ќ]^

Щ[XЭЬ€OHЦTЧУФХ”СSPQСJKЫЫ‹ЦTЧУФЦWФФИ
И[™KЉ‘[X›YЉJNИЛУ[ЩH€H^Y\њИРS€]XЪИXXЪЭ\‚€B€B€[™JКОВ€€Y€
ЫШ[ШЫЫ™љYЛЪX]И	€ТPUУФSУ”ЧУPTХT—УQS•JB€В€ЫY[ќ]^

Щ[XЭЬ€OHЦTЧУФРТPUКKЫЫKЦTЧУФЦWФФИ
И[™KЉђЪX]Ь[ЫњЛ‹‹€ЉJNВ€[™JКОВ€B‚€Y€
[›ЩXќYЫЬ[ЫњКB€В€ЫY[ќ]^

Щ[XЭЬ€OHЦTЧУФСP•QКKЫЫKЦTЧУФЦWФФИ
И[™KЉ‘XќYИЩ][™ЬЛ‹‹€ЉJNВ€[™JКОВ€B‚€ЫY[ќ]^

Щ[XЭЬ€OHЦTЧУФРУУ‘’QКKЫЫKЦTЧУФЦWФФИ
И[™KЉђЫЫ™љYИЩ][™ЬЛ‹‹€ЉJNВ‚€К€^H[™\И›Ь€ЬXЪ[™Л€
‹В€[™H
ПHЋВ‚€ЫY[ќ]^J
Щ[XЭЬ€OH‘U
KЦTЧУФЦWФФИ
И[™KЉђXЪИЉJNВ‚€\]J
]™[OH•S
K
NВ‚€YЉ›Э™]ЪЩ^\И	€“QЧСTРКB€В€]Z]HNВ€B‚€YЉ›Э™]ЪЩ^\И	€“QЧУSХ‘UT
B€В€K\Щ[XЭЬЋВ‚€К€ЪЪ\\ШX›Y][\Л€
‹В‚€Y€
Щ[XЭЬ€OHЦTЧУФРТPUИ	‰€JЫШ[ШЫЫ™љYЛЪX]И	€ТPUУФSУ”ЧУPTХT—УQS•JJB€В€K\Щ[XЭЬЋВ€B‚€Y€
Щ[XЭЬ€OHЦTЧУФСP•QИ	‰€›ЩXќYЫЬ[ЫњКB€В€K\Щ[XЭЬЋВ€B‚€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B‚€YЉ›Э™]ЪЩ^\И	€“QЧУSХ‘QХУЉB€В€
КЬЩ[XЭЬЋВ‚€К€ЪЪ\\ШX›Y][\Л€
‹В‚€Y€
Щ[XЭЬ€OHЦTЧУФРТPUИ	‰€JЫШ[ШЫЫ™љYЛЪX]И	€ТPUУФSУ”ЧУPTХT—УQS•JJB€В€
КЬЩ[XЭЬЋВ€B‚€Y€
Щ[XЭЬ€OHЦTЧУФСP•QИ	‰€›ЩXќYЫЬ[ЫњКB€В€
КЬЩ[XЭЬЋВ€H‚€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B‚€YЉЩ[XЭЬ€
B€В€Щ[XЭЬ€H‘UВ€B€YЉЩ[XЭЬ€€‘U
B€В€Щ[XЭЬ€HВ€B‚€YЉ›Э™]ЪЩ^\И	€
“QЧУSХ‘SQ•“QЧУSХ‘T’QТ“QЧРS–P•UУЉJB€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\М‹Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ‚€Y€
Щ[XЭЬ€OH‘U
H]Z]HNВ€[ЩHY€
Щ[XЭЬ€OHЦTЧУФУСКHШ]™Y]Kќ\Щ[ЩИH\Ш]™Y]Kќ\Щ[ЩОВ€[ЩHY€
Щ[XЭЬ€OHЦTЧУФХ”СSPQСJB€В€Y€
™\њЭ\Щ[XYЩH€JB€В€Y€
Ш]™Y]K›[ЩJB€В€Ш]™Y]K›[ЩHHВ€B€[ЩB€В€Ш]™Y]K›[ЩHHNВ€B€B€B€[ЩHY€
Щ[XЭЬ€OHЦTЧУФРТPUКB€В€Y[ќWЫЬ[ЫњЧШЪX]К
NВ€B€[ЩHY€
Щ[XЭЬ€OHЦTЧУФСP•QИ	‰€[›ЩXќYЫЬ[ЫњКB€В€Y[ќWЫЬ[ЫњЧЩXќYК
NВ€B€[ЩHY€
Щ[XЭЬ€OHЦTЧУФРТPUКB€В€Y[ќWЫЬ[ЫњЧШЪX]К
NВ€B€[ЩHY€
Щ[XЭЬ€OHЦTЧУФСP•QИ	‰€[›ЩXќYЫЬ[ЫњКHY[ќWЫЬ[ЫњЧЩXќYК
NВ€[ЩHY€
Щ[XЭЬЏOTЦTЧУФРУУ‘’QКHY[ќWЫЬ[ЫњЧШЫЫ™љYК
NВ€[ЩH]Z]HNВ€B€B€Ш]™\Щ][™ЬК
NВ€›Э™]ЪЩ^\ИHВ€ШЬ™Y[—ЬЭ]\И	ЏH’S—ФРФ‘QS—ФЦTХSWУФSУ”ЧУQS•NВ‚€Э[™Y€ЦTЧУФЦWФФВџB‚‚ќ›ЪYY[ќWЫЬ[ЫњЧЭљY[К
BћВ€[ќ]Z]HВ€[ќЩ[XЭЬ€HВ€[ќ\ЋВ€[ќЫЫHHLMKЫЫ€HNВ‚€ШЬ™Y[—ЬЭ]\ИHS—ФРФ‘QS—Х’QSЧУФSУ”ЧУQS•NВ€›Э™]ЪЩ^\ИHВ‚€Ъ[J\]Z]
B€В€ЫY[ќ]^J‹MKЉ•љY[ИЬ[ЫњИЉJNВ€ЫY[ќ]^

Щ[XЭЬ€OH
KЫЫKLЛЉђњљYЪ™\ЬО€ЉJNВ€ЫY[ќ]^

Щ[XЭЬ€OH
KЫЫ‹LЛ‰ZH‹Ш]™Y]KњљYЪ™\ЬКNВ€ЫY[ќ]^

Щ[XЭЬ€OHJKЫЫKL‹Љ‘Ш[[XN€ЉJNВ€ЫY[ќ]^

Щ[XЭЬ€OHJKЫЫ‹L‹‰ZH‹Ш]™Y]K™Ш[[XJNВ€ЫY[ќ]^

Щ[XЭЬ€OHЉKЫЫKLKЉ•Ъ[™ЭИЩ™њЩ]€ЉJNВ€ЫY[ќ]^

Щ[XЭЬ€OHЉKЫЫ‹LK‰ZH‹Ш]™Y]KќЪ[™ЭЬЬКNВ‚€ЪY€С€ЫY[ќ]^

Щ[XЭЬ€OHКKЫЫKЉ‘\Ь^H[ЩN€ЉJNВ€ЫY[ќ]^

Щ[XЭЬ€OHКKЫЫ‹Ш]™Y]K™ќ[ШЬ™Y[€ИЉ‘ќ[ЉH€Љ•Ъ[™ЭИЉJNВ‚€ЫY[ќ]^

Щ[XЭЬ€OH
KЫЫKKЉ•љY[ИXЪЩ[™€ЉJNВ€ЫY[ќ]^

Щ[XЭЬ€OH
KЫЫ‹K
Ь[™ЫИЉ“Ь[‘УЉH€Љ”СЉJJNВ‚€ЫY[ќ]^

Щ[XЭЬ€OHJKЫЫK‹Љ”ШШ[N€ЉJNВ€ЪY™Y€S‘“ТQ€YЉШ]™Y]KљЬШШ[HOH
B€Щ[ЩB‚BZYЉШ]™Y]K™ќ[ШЬ™Y[ЉB€Щ[™Y‚€В€ЫY[ќ]^

Щ[XЭЬ€OHJKЫЫ‹‹Љђ]]ЫX]XИЉJNВ€B€[ЩB€В€ЫY[ќ]^

Щ[XЭЬ€OHJKЫЫ‹‹‰MЊ™ћH	Z^	ZH‹Ш]™Y]KљЬШШ[K
[ќ
JљY[Ы[Щ\Лљ™\И
€Ш]™Y]KљЬШШ[JK
[ќ
JљY[Ы[Щ\Лќ”™\И
€Ш]™Y]KљЬШШ[JJNВ€B‚€ЫY[ќ]^

Щ[XЭЬ€OHЉKЫЫKЛЉ’\™Ш\™Hљ[\Ћ€ЉJNВ€В€Ъ\€
™љ[\“[YNВ€Y€
Ш]™Y]KљЬШШ[HOHKЊ	‰€\Ш]™Y]K™ќ[ШЬ™Y[ЉB€љ[\“[YHH‘\ШX›YЋВ€[ЩHY€
Ь[™Ы
B€љ[\“[YHH’YЪ]X[]HЋВ€[ЩHY€
Ш]™Y]KљЩљ[\ЉB€љ[\“[YHH”Ъ[\HЋВ€[ЩB€љ[\“[YHHђљ[[™X\€ЋВ€ЫY[ќ]^

Щ[XЭЬ€OHЉKЫЫ‹ЛЉљ[\“[YJJNВ€B‚€ЫY[ќ]^

Щ[XЭЬ€OHКKЫЫKЉ”ЫЩќШ\™Hљ[\Ћ€ЉJNВ€ЫY[ќ]^

Щ[XЭЬ€OHКKЫЫ‹

Ш]™Y]KљЬШШ[HЏH‹ЊШ]™Y]K™ќ[ШЬ™Y[ЉHИЉЩћ›]\“[Y\ЦЬШ]™Y]KњЭЩљ[\—JH€Љ‘\ШX›YЉJJNВ‚€Ъ\€њЫ[Z]Э^ММ—NВ€ЭЪ]Ъ
Ш]™Y]K™њЫ[Z]
B€В€Ш\ЩH‚€Ыњљ[ќЉњЫ[Z]Э^Ъ^™[ЩЉњЫ[Z]Э^
K‰\И‹Љ‘\ШX›YЉJNВ€њ™XZОВ€Ш\ЩHN‚€Ыњљ[ќЉњЫ[Z]Э^Ъ^™[ЩЉњЫ[Z]Э^
K‰ZH
”Ю[КH‹љY[ЧШЭ\њ™[ќЬ™Yњ™\ЪЬ]J
JNВ€њ™XZОВ€Ш\ЩHЋ‚€Ыњљ[ќЉњЫ[Z]Э^Ъ^™[ЩЉњЫ[Z]Э^
K‰ZH‹Њ
NВ€њ™XZОВ€Ш\ЩHО‚€Ыњљ[ќЉњЫ[Z]Э^Ъ^™[ЩЉњЫ[Z]Э^
K‰ZH‹L
NВ€њ™XZОВ€Y][‚€Ыњљ[ќЉњЫ[Z]Э^Ъ^™[ЩЉњЫ[Z]Э^
K‰\И‹Љ•[љЫ›ЭЫ€ЉJNВ€њ™XZОВ€B‚€ЫY[ќ]^

Щ[XЭЬ€OH
KЫЫKKЉ‘”И[Z]€ЉJNВ€ЫY[ќ]^

Щ[XЭЬ€OH
KЫЫ‹KњЫ[Z]Э^
NВ‚€YЉШ]™Y]K™ќ[ШЬ™Y[ЉB€В€ЫY[ќ]^

Щ[XЭЬ€OHJKЫЫK‹Љ‘ќ[ШЬ™Y[€\N€ЉJNВ€ЫY[ќ]^

Щ[XЭЬ€OHJKЫЫ‹‹
Ш]™Y]KњЭ™]ЪИЉ”Э™]ЪИШЬ™Y[€ЉH€Љ”™\Щ\ќ™H\ЬXЭ][ИЉJJNВ€B€[ЩHYЉЩ[XЭЬ€OHJB€В€Щ[XЭЬ€H
›Э™]ЪЩ^\И	€“QЧУSХ‘UT
HИ€LВ€B‚€ЫY[ќ]^J
Щ[XЭЬ€OHL
KKЉђXЪИЉJNВ€YЉЩ[XЭЬ€
B€В€Щ[XЭЬ€HLВ€B€YЉЩ[XЭЬ€€L
B€В€Щ[XЭЬ€HВ€B€Щ[™Y‚‚€\]J
]™[OH•S
K
NВ‚€YЉ›Э™]ЪЩ^\И	€“QЧСTРКB€В€]Z]HNВ€B€YЉ›Э™]ЪЩ^\И	€“QЧУSХ‘UT
B€В€K\Щ[XЭЬЋВ€YЉЫШ[ЬШ[\WЫ\Э™Y\ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€YЉ›Э™]ЪЩ^\И	€“QЧУSХ‘QХУЉB€В€
КЬЩ[XЭЬЋВ€YЉЫШ[ЬШ[\WЫ\Э™Y\ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€YЉ›Э™]ЪЩ^\И	€
“QЧУSХ‘SQ•“QЧУSХ‘T’QТ“QЧРS–P•UУЉJB€В€\€HВ‚€YЉ›Э™]ЪЩ^\И	€“QЧУSХ‘SQ•
B€В€\€HLNВ€B€[ЩHYЉ›Э™]ЪЩ^\И	€“QЧУSХ‘T’QТ
B€В€\€HNВ€B‚€YЉЫШ[ЬШ[\WЫ\Э™Y\М€ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\М‹Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B‚€ЭЪ]Ъ
Щ[XЭЬЉB€В€Ш\ЩH‚€Ш]™Y]KњљYЪ™\ЬИ
ПH
€\ЋВ€YЉШ]™Y]KњљYЪ™\ЬИLЌMЉB€В€Ш]™Y]KњљYЪ™\ЬИHLЌMЋВ€B€YЉШ]™Y]KњљYЪ™\ЬИ€ЌMЉB€В€Ш]™Y]KњљYЪ™\ЬИHЌMЋВ€B€™ШWЭќШZ]

NВ€Щ]ШЫЫЬ—ШЫЬњ™XЭ[ЫЉШ]™Y]K™Ш[[XKШ]™Y]KњљYЪ™\ЬКNВ€њ™XZОВ€Ш\ЩHN‚€Ш]™Y]K™Ш[[XH
ПH
€\ЋВ€YЉШ]™Y]K™Ш[[XHLЌMЉB€В€Ш]™Y]K™Ш[[XHHLЌMЋВ€B€YЉШ]™Y]K™Ш[[XH€ЌMЉB€В€Ш]™Y]K™Ш[[XHHЌMЋВ€B€™ШWЭќШZ]

NВ€Щ]ШЫЫЬ—ШЫЬњ™XЭ[ЫЉШ]™Y]K™Ш[[XKШ]™Y]KњљYЪ™\ЬКNВ€њ™XZОВ€Ш\ЩHЋ‚€Ш]™Y]KќЪ[™ЭЬЬИ
ПH\ЋВ€YЉШ]™Y]KќЪ[™ЭЬЬИLЉB€В€Ш]™Y]KќЪ[™ЭЬЬИHLЋВ€B€YЉШ]™Y]KќЪ[™ЭЬЬИ€Њ
B€В€Ш]™Y]KќЪ[™ЭЬЬИHЊВ€B€њ™XZОВ€ЪY€С€Ш\ЩHО‚€љY[ЧЩќ[ШЬ™Y[—Щ›\

NВ€њ™XZОВ€Ш\ЩH‚€Ш]™Y]Kќ\ЩYЫH\Ш]™Y]Kќ\ЩYЫВ€љY[ЧЬЩ]Ы[ЩJљY[Ы[Щ\КNВ€Щ]ШЫЫЬ—ШЫЬњ™XЭ[ЫЉШ]™Y]K™Ш[[XKШ]™Y]KњљYЪ™\ЬКNВ€њ™XZОВ€Ш\ЩHN‚€ЪY›™Y€S‘“ТQ€YЉШ]™Y]K™ќ[ШЬ™Y[ЉB€В€њ™XZОВ€B€Ш]™Y]KљЬШШ[H
ПH\€
€ЊЌNВ€YЉШ]™Y]KљЬШШ[HЊЌJB€В€Ш]™Y]KљЬШШ[HHЊЌNВ€B€YЉШ]™Y]KљЬШШ[H€Њ
B€В€Ш]™Y]KљЬШШ[HHЊВ€B€љY[ЧЬЩ]Ы[ЩJљY[Ы[Щ\КNВ€Щ[ЩB€Ш]™Y]KљЬШШ[H
ПH\€
€ЊЌNВ€YЉШ]™Y]KљЬШШ[HЊ
B€В€Ш]™Y]KљЬШШ[HHЊВ€B€YЉШ]™Y]KљЬШШ[H€Њ
B€В€Ш]™Y]KљЬШШ[HHЊВ€B€Щ[™Y‚€Ш\ЩHЋ‚€ЪY›™Y€S‘“ТQ€Y€
Ь[™Ы
\Ш]™Y]K™ќ[ШЬ™Y[€	‰€Ш]™Y]KљЬШШ[HOHKЊ
JB€В€њ™XZОВ€B€Щ[™Y‚€Ш]™Y]KљЩљ[\€
ПH\ЋВ€YЉШ]™Y]KљЩљ[\€
B€В€Ш]™Y]KљЩљ[\€HNВ€B€YЉШ]™Y]KљЩљ[\€€JB€В€Ш]™Y]KљЩљ[\€HВ€B€љY[ЧЬЩ]Ы[ЩJљY[Ы[Щ\КNВ‚BBBXњ™XZОВ€Ш\ЩHО‚€YЉ\Ш]™Y]K™ќ[ШЬ™Y[€	‰€Ш]™Y]KљЬШШ[H‹Њ
B€В€њ™XZОВ€B€љY[Ы[Щ\Л™љ[\€
ПH\ЋВ€YЉљY[Ы[Щ\Л™љ[\€€“UT—УPVHJB€В€љY[Ы[Щ\Л™љ[\€HВ€B€YЉљY[Ы[Щ\Л™љ[\€
B€В€љY[Ы[Щ\Л™љ[\€H“UT—УPVHNВ€B€Ш]™Y]KњЭЩљ[\€HљY[Ы[Щ\Л™љ[\ЋВ€Y[\Щ]
[PќY™™\‹LЌMЊ
NВ‚BBB]љY[ЧЬЩ]Ы[ЩJљY[Ы[Щ\КNВ€њ™XZОВ€Ш\ЩH‚€Ш]™Y]K™њЫ[Z]H
Ш]™Y]K™њЫ[Z]
И\ЉH	HВ€Y€
Ш]™Y]K™њЫ[Z]
HШ]™Y]K™њЫ[Z]
ПHВ€љY[ЧЬЩ]Ы[ЩJљY[Ы[Щ\КNВ€њ™XZОВ€Ш\ЩHN‚€љY[ЧЬЭ™]Ъ

Ш]™Y]KњЭ™]ЪЏHJJNВ€њ™XZОВ€Щ[™Y‚€Y][‚€]Z]HNВ€B€B€B€Ш]™\Щ][™ЬК
NВ€›Э™]ЪЩ^\ИHВ€ШЬ™Y[—ЬЭ]\И	ЏH’S—ФРФ‘QS—Х’QSЧУФSУ”ЧУQS•NВџB‚‚ќ›ЪYY[ќWЫЬ[ЫњК
BћВ€ЩYљ[™HФЦWФФИLB€ЩYљ[™HФЦФФИMВ€€\YY€[ќ[HВ€’QSЧУФSУ‹€УХS‘УФSУ‹€УУ•“УУФSУ‹€ЦTХSWУФSУ‹‚€S‘УФSУ‚€HWЬЩ[XЭЬЋВ‚€[ќ]Z]HВ€[ќWЫЩ™њЩ]HФЦWФФОВ€[ќђPТЧУФSУ€HS‘УФSУЋВ€WЬЩ[XЭЬ€Щ[XЭЬ€H’QSЧУФSУЋВ‚€ШЬ™Y[—ЬЭ]\ИHS—ФРФ‘QS—УФSУ”ЧУQS•NВ€›Э™]ЪЩ^\ИHИ€€Ъ[J\]Z]
B€И€ЫY[ќ]^J
Щ[XЭЬ€OH’QSЧУФSУЉKWЫЩ™њЩ]
Х’QSЧУФSУ‹Љ•љY[ИЬ[ЫњЛ‹‹€ЉJNВ€ЫY[ќ]^J
Щ[XЭЬ€OHУХS‘УФSУЉKWЫЩ™њЩ]
ФУХS‘УФSУ‹Љ”ЫЭ[™Ь[ЫњЛ‹‹€ЉJNВ€ЫY[ќ]^J
Щ[XЭЬ€OHУУ•“УУФSУЉKWЫЩ™њЩ]
РУУ•“УУФSУ‹ЉђЫЫќ›ЫЬ[ЫњЛ‹‹€ЉJNВ€ЫY[ќ]^J
Щ[XЭЬ€OHЦTХSWУФSУЉKWЫЩ™њЩ]
ФЦTХSWУФSУ‹Љ”Ю\Э[HЬ[ЫњЛ‹‹€ЉJNВ€€ЫY[ќ]^J
Щ[XЭЬ€OHђPТЧУФSУЉKWЫЩ™њЩ]
РђPТЧУФSУЉМ‹ЉђXЪИЉJNВ‚€\]J
]™[OH•S
K
NВ‚€YЉ›Э™]ЪЩ^\И	€“QЧСTРКB€В€]Z]HNВ€B€YЉ›Э™]ЪЩ^\И	€“QЧУSХ‘UT
B€В€YЉЩ[XЭЬ€H’QSЧУФSУЉB€В€Щ[XЭЬ€HђPТЧУФSУЋВ€B€[ЩHK\Щ[XЭЬЋВ‚€YЉЫШ[ЬШ[\WЫ\Э™Y\ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€YЉ›Э™]ЪЩ^\И	€“QЧУSХ‘QХУЉB€В€
КЬЩ[XЭЬЋВ€YЉЩ[XЭЬ€€ђPТЧУФSУЉB€В€Щ[XЭЬ€H’QSЧУФSУЋВ€B‚€YЉЫШ[ЬШ[\WЫ\Э™Y\ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€YЉ›Э™]ЪЩ^\И	€
“QЧУSХ‘SQ•“QЧУSХ‘T’QТ“QЧРS–P•UУЉJB€В‚€YЉЫШ[ЬШ[\WЫ\Э™Y\М€ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\М‹Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B‚€YЉЩ[XЭЬЏOPђPТЧУФSУЉH]Z]HNВ€[ЩHYЉЩ[XЭЬЏOU’QSЧУФSУЉHY[ќWЫЬ[ЫњЧЭљY[К
NВ€[ЩHYЉЩ[XЭЬЏOTУХS‘УФSУЉHY[ќWЫЬ[ЫњЧЬЫЭ[™

NВ€[ЩHYЉЩ[XЭЬЏOPУУ•“УУФSУЉHY[ќWЫЬ[ЫњЧЪ[њ]

NВ€[ЩHYЉЩ[XЭЬЏOTЦTХSWУФSУЉB€В€Y[ќWЫЬ[ЫњЧЬЮ\Э[J
NИ€B€€[ЩH]Z]HNВ€B€B€Ш]™\Щ][™ЬК
NВ€YЉЬ]\ЩHOHJB€В€Ь]\ЩHHЋВ€B€›Э™]ЪЩ^\ИHВ€ШЬ™Y[—ЬЭ]\И	ЏH’S—ФРФ‘QS—УФSУ”ЧУQS•NВ‚€Э[™Y€ХРТPUВ€Э[™Y€ФЦWФФВ€Э[™Y€ФЦФФВ€Э[™Y€ТPUФUTСWФФЦBџB‚‹ЛИKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKB‚‚ќ›ЪYЬ[›Ь“XZ[Љ[ќ\™ШЛЪ\€
Љ\™ЭЉBћВ€Ьљ]WЫX\H•SВ€[ќ]Z]HВ€[ќ™[XЪИHNВ€[ќЩ[XЭЬ€HВ€LМ€[ќ›Э[YHHВ€[ќЭ\ќYHВ€Ъ\€\ќY™–УPVР•Q‘‘T—УS—HHИ€џNВ€[ќ^Y\њЦУPVФVQT”ЧHHМNВ€[ќNВ€[ќ\™ЫВ‚€љ[ќЉ“Ь[ђ›Ф€	\ЛЫЫ\[H]N€€ЧСUWЧИ——€‹‘T”ТSУЉNВ‚€YЉ\™ШИ€JB€В€\™ЫHЭ›[Љ\™Э–МWJNВ€YЉ\™Ы€M	‰€[Y[XЫ\
\™Э–МWK›Щ™њШЬ™Y[љЪ[H‹M
JB€В€QђUSУС‘”РФ‘QS—ТТSHЩ][Y[ќ

Ъ\€
ЉX\™Э–МWH
ИM€‹€ЉNВ€B€YЉ\™Ы€M	‰€[Y[XЫ\
\™Э–МWKњЪЭЩљ[\Э\ЩYH‹M
JB€В€љ[ќљ[U\ШYЩTЭ]\ЭXЬИHЩ][Y[ќ

Ъ\€
ЉX\™Э–МWH
ИM€‹€ЉNВ€B€B‚‚€[Щ[ЫY\ЭHЬ™X]S[Щ[ЫЫ[X[™\Э

NВ€[Щ[ЭЫY\ЭHЬ™X]S[Щ[ЭЫЫ[X[™\Э

NВ€]™[ЫY\ЭHЬ™X]S]™[ЫЫ[X[™\Э

NВ€]™[Ь™\ЫY\ЭHЬ™X]S]™[Ь™\ђЫЫ[X[™\Э

NВ€Ь™X]S[Щ[\Э

NВ‚€ЛИШY™XЩ\ЬШ\ћHЫЫ\Ы™[ќЛ‚€љ[ќЉ‘Ш[YHЩ[XЭY€	\Ч—€‹XЪЩљ[JNВ€ШYЩ][™ЬК
NВ€Э\ќ\

NВ‚X›Э™]ЪЩ^\ИHВ‚€YЉЪЪ\ЬЩ]
B€В‚€ЛИ™]И[\›]]™HXЪЩЬ›Э[™]‚€YЉЭ\ЭљЩЬ™ИOH•S
B€В€ЭЬJ\ќY™‹Э\ЭљЩЬ™КNВ€ЭШ]
\ќY™‹›ЩЫИЉNВ€ШYШXЪЩЬ›Э[™
\ќY™ЉNВ€B€[ЩB€В€ШYШШXЪYШXЪЩЬ›Э[™
™]KШ™ЬЛЫЩЫИЉNВ€B‚€Ъ[JЭ[YHЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYY
€€	‰€J›Э™]ЪЩ^\И	€
“QЧРS–P•UУ€“QЧСTРКJJB€В€\]J
NВ€B‚€]\ЪXК™]KЫ]\ЪXЛЬ™[Z^‹K
NВ‚€ЛИ™]И[\›]]™HШЩ[™H]‚€YЉЭ\ЭШЩ[™\ИOH•S
B€В€ЭЬJ\ќY™‹Э\ЭШЩ[™\КNВ€ЭШ]
\ќY™‹›ЩЫЛќЉNВ€^\ШЩ[™J\ќY™ЉNВ€B€[ЩB€В€^\ШЩ[™J™]KЬШЩ[™\ЛЫЩЫЛќЉNВ€B€B€ЫX\њШЬ™Y[ЉXЪЩЬ›Э[™
NВ‚€Ъ[J\]Z]
B€В€YЉЪЪ\ЬЩ]	‰€JЫЭЧЫXZ[›Y[ќWЩ›YЙЋ
JB€В€YЉЭ[YHЏH[ќ›Э[YJB€В€ЛИ™]И[\›]]™HШЩ[™H]‚€YЉЭ\ЭШЩ[™\ИOH•S
B€В€ЭЬJ\ќY™‹Э\ЭШЩ[™\КNВ€ЭШ]
\ќY™‹љ[ќ›ЛќЉNВ€^\ШЩ[™J\ќY™ЉNВ€B€[ЩB€В€^\ШЩ[™J™]KЬШЩ[™\ЛЪ[ќ›ЛќЉNВ€B€\]J
NВ€[ќ›Э[YHHЭ[YH
ИЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYY
€ЊВ€™[XЪИHNВ€Э\ќYHВ€B‚€YЉ›Э™]ЪЩ^\И	€“QЧСTРКB€В€]Z]HNВ€B€B€[ЩB€В€Э\ќYHNВ€™[XЪИHВ€B‚€Y€
ЫЭЧЫXZ[›Y[ќWЩ›YИOH
HЫЭЧЫXZ[›Y[ќWЩ›YИHВ€YЉ\Э\ќY
B€В€YЉ
Э[YH	HЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYY
H
ЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYYИЉJB€В€ЫY[ќ]^JЉ”‘TФИХT•ЉJNВ€B€YЉ›Э™]ЪЩ^\И	€
“QЧРS–P•UУЉJB€В€Э\ќYHNВ€™[XЪИHNВ€B€B€[ЩHYЉЪЪ\ЬЩ]ЏH
B€В€ШYШ[YQљ[J
NВ€^YШ[YJ^Y\њЛ\ЩTЩ]ЏHИ\ЩTЩ]€ЪЪ\ЬЩ]\ЩTШ]™JNВ€B€[ЩB€В€ЫY[ќ]^J
Щ[XЭЬ€OH
K‹Љ”Э\ќШ[YHЉJNВ€ЫY[ќ]^J
Щ[XЭЬ€OHJKЛЉ“Ь[ЫњИЉJNВ€ЫY[ќ]^J
Щ[XЭЬ€OHЉKЉ’ЭИИ^HЉJNВ€ЫY[ќ]^J
Щ[XЭЬ€OHКKKЉ’[Щ€[YHЉJNВ€ЫY[ќ]^J
Щ[XЭЬ€OH
K‹Љ”]Z]ЉJNВ€YЉЩ[XЭЬ€
B€В€Щ[XЭЬ€HВ€B€YЉЩ[XЭЬ€€
B€В€Щ[XЭЬ€HВ€B‚€YЉ›Э™]ЪЩ^\КB€В€[ќ›Э[YHHЭ[YH
ИЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYY
€ЊВ€B‚€YЉ›Э™]ЪЩ^\И	€“QЧУSХ‘UT
B€В€K\Щ[XЭЬЋВ€YЉЫШ[ЬШ[\WЫ\Э™Y\ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€YЉ›Э™]ЪЩ^\И	€“QЧУSХ‘QХУЉB€В€
КЬЩ[XЭЬЋВ€YЉЫШ[ЬШ[\WЫ\Э™Y\ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€B€YЉ›Э™]ЪЩ^\И	€
“QЧРS–P•UУЉJB€В€YЉЫШ[ЬШ[\WЫ\Э™Y\М€ЏH
B€В€ЫЭ[™Ь^WЬШ[\JЫШ[ЬШ[\WЫ\Э™Y\М‹Ш]™Y]K™Y™™XЭ›ЫШ]™Y]K™Y™™XЭ›ЫL
NВ€B€ЭЪ]Ъ
Щ[XЭЬЉB€В€Ш\ЩH‚€›ЬЉHHИHPVФVQT”ОИJККB€В€^Y\њЦЪWHH^Y\–ЪWK›™]ЪЩ^\И	€
“QЧРS–P•UУЉNВ€B€™[XЪИHЪЫЬЩWЫ[ЩJ^Y\њКNВ€YЉ™[XЪКB€В€Э\ќYHВ€B€њ™XZОВ€Ш\ЩHN‚€Y[ќWЫЬ[ЫњК
NВ€њ™XZОВ€Ш\ЩHЋ‚€В€[ќ™]љ[Э\УЫЬH]\ЪXЫЫЬВ€Ъ\€™]љ[Э\У]\ЪXЦЬЪ^™[ЩЉЭ\њ™[ќ]\ЪXКWNВ‚€Y[\Щ]
™]љ[Э\У]\ЪXЛЪ^™[ЩЉ™]љ[Э\У]\ЪXКJNВ€ЭЬJ™]љ[Э\У]\ЪXЛЭ\њ™[ќ]\ЪXКNВ‚€YЉЭ\ЭШЩ[™\ИOH•S
B€В€ЭЬJ\ќY™‹Э\ЭШЩ[™\КNВ€ЭШ]
\ќY™‹љЭЭЛќЉNВ€^\ШЩ[™J\ќY™ЉNВ€B€[ЩB€В€^\ШЩ[™J™]KЬШЩ[™\ЛЪЭЭЛќЉNВ€B€YЉЭљXЫ\
™]љ[Э\У]\ЪXЛЭ\њ™[ќ]\ЪXКHOH
B€В€]\ЪXК™]љ[Э\У]\ЪXЛ™]љ[Э\УЫЬ
NВ€B€™[XЪИHNВ€њ™XZОВ€B€Ш\ЩHО‚€[[YJ
NВ€™[XЪИHNВ€њ™XZОВ€Y][‚€]Z]HNВ€њ™XZОВ€B€[ќ›Э[YHHЭ[YH
ИЫШ[ШЫЫ™љYЛ™Ш[YWЬЬYY
€ЊВ€B€B€YЉ™[XЪКB€В€YЉЭ\ќY
B€В€К€€Ь]\И
LLЊЊJHYY[€Y][Ы[[њЭ[ЩHЩ€H[њЫ][Ы€ќ[Э[Ы€]Y[ќHШЬ™Y[‚€\ЩYИ™Yњ™\Ъ[^Ъ]Э]ЫЬЩH[™™[Ь[€H[™Ъ[™B‚€Ь]\И
‹LЊЊКHYY[€Y][Ы[[њЭ[ЩHЩ€HљY[ЧЫ[ЩHќ[Э[Ы€]Y[ќHШЬ™Y[‚€\ЩYИ™Yњ™\ЪHЭXЪШЬ™Y[€›Ь\ќY\ИЫ€[™›ЪYЪ]Э]™\Э\ќ[™В€
‹В€љY[ЧЬЩ]Ы[ЩJљY[Ы[Щ\КNВ€Ш—Ъ[љ][њК
NВ€€ШЬ™Y[—ЬЭ]\ИHS—ФРФ‘QS—УQS•NВ€ШЬ™Y[—ЬЭ]\И	ЏH’S—ФРФ‘QS—ХUNВ‚€YЉЭ\ЭљЩЬ™ИOH•S
B€В€ЭЬJ\ќY™‹Э\ЭљЩЬ™КNВ€ЭШ]
\ќY™‹ќ]X€ЉNВ€ШYШXЪЩЬ›Э[™
\ќY™ЉNВ€B€[ЩB€В€ШYШШXЪYШXЪЩЬ›Э[™
™]KШ™ЬЛЭ]X€ЉNВ€B€B€[ЩB€В€ШЬ™Y[—ЬЭ]\И	ЏH’S—ФРФ‘QS—УQS•NВ€ШЬ™Y[—ЬЭ]\ИHS—ФРФ‘QS—ХUNВ‚€YЉЭ\ЭљЩЬ™ИOH•S
B€В€ЭЬJ\ќY™‹Э\ЭљЩЬ™КNВ€ЭШ]
\ќY™‹ќ]HЉNВ€ШYШXЪЩЬ›Э[™
\ќY™ЉNВ€B€[ЩB€В€ШYШШXЪYШXЪЩЬ›Э[™
™]KШ™ЬЛЭ]HЉNВ€B€B‚€YЉ\ЫЭ[™Ь]Y\ћWЫ]\ЪXК•S•S
JB€В€]\ЪXК™]KЫ]\ЪXЛЬ™[Z^‹K
NВ€B€™[XЪИHВ€B€\]J
NВ€B€›Ь”Ъ]ЭЫЉQђUSФТUХУ—УQTФРQСJNВџB‚€Э[™Y€СUРT‘В€Э[™Y€СUРT‘ЧУS‚€Э[™Y€СUРT‘Ф€Э[™Y€СUРT‘ФУS‚€Э[™Y€СUТS•РT‘В€Э[™Y€СUС“РUРT‘В€Э[™Y€СUТS•РT‘Ф€Э[™Y€СUС“РUРT‘Ф