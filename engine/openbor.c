/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

/////////////////////////////////////////////////////////////////////////////
//	Beats of Rage                                                           //
//	Side-scrolling beat-'em-up                                              //
/////////////////////////////////////////////////////////////////////////////

#include    "openbor.h"

/////////////////////////////////////////////////////////////////////////////
//  Global Variables                                                        //
/////////////////////////////////////////////////////////////////////////////

s_level_entry*      levelorder[MAX_DIFFICULTIES][MAX_LEVELS];
s_level*            level               = NULL;
s_screen*           vscreen             = NULL;
s_screen*           background          = NULL;
s_screen*           bgbuffer            = NULL;
char                bgbuffer_updated    = 0;
s_bitmap*           texture             = NULL;
s_videomodes        videomodes;
s_spawn_script_cache_node* spawn_script_cache_head = NULL;
int sprite_map_max_items = 0;
int model_map_max_items = 0;
int cache_map_max_items = 0;


//see types.h
const s_drawmethod plainmethod = {
    NULL, // table
    NULL, //fp
    0,    // fillcolor
    1,    //flag
    -1,   // alpha
    -1,   // remap
    0,    //flipx
    0,    //flipy
    0,    //transbg
    0,    //fliprotate
    0,    //rotate
    256,  //scalex
    256,  //scaley
    0,    //shiftx
    0,    //centerx  //currently used only by gfxshadow, do not touch it
    0     //centery
};


// unknockdown attack
const s_attack emptyattack = {
    0,  // force
   -1,  // sound
   -1,  // flash
   -1,  // blockflash
   -1,  // blocksound
    0,  // counter
   {0,
    0,
    0,
    0,
    0}, // coods
    0,  // nopain/noreflect
    0,  // noflash
    0,  // noblock
    0,  // grab
    0,  // dir
    0,  // blast
    0,  // freeze
    0,  // steal
    0,  // map
    0,  // seal
    0,  // freezetime
    0,  // maptime
    0,  // sealtime
    0,  // dot
    0,  // dot index
    0,  // dot_time
    0,  // dot_force
    0,  // dot_rate
   {0,
    0,
    0}, // dropv
	0,  // otg
	0,  // jugglecost
	0,  // Guardcost
    0,  // drop
    0,  // type
    0,  // damage on landing
    0,  // grabdist
    0,  // pause
};

char                *custScenes = NULL;
char                *custBkgrds = NULL;
char                *custLevels = NULL;
char                *custModels = NULL;
char                rush_names[2][MAX_NAME_LEN];
char                branch_name[MAX_NAME_LEN+1];    // Used for branches
char                set_names[MAX_DIFFICULTIES][MAX_NAME_LEN+1];
unsigned char       pal[MAX_PAL_SIZE] = {""};
int                 blendfx[MAX_BLENDINGS] = {0,1,0,0,0,0};
char                blendfx_is_set = 0;
int                 fontmonospace[8] = {0,0,0,0,0,0,0,0};

// move all blending effects here
unsigned char*      blendings[MAX_BLENDINGS] = {NULL, NULL, NULL, NULL, NULL, NULL} ;
// function pointers to create the tables
palette_table_function blending_table_functions[MAX_BLENDINGS] = {palette_table_screen, palette_table_multiply, palette_table_overlay,
                                         palette_table_hardlight, palette_table_dodge, palette_table_half};


char                comment_mark[4] = {"#"};

int                 current_set = 0;
int                 current_level = 0;
int                 current_stage = 1;

float               bgtravelled;
int                 traveltime;
int                 texttime;
float               advancex;
float               advancey;
float               scrolldx;                       // advancex changed previous loop
float               scrolldy;                       // advancey .....................
float               scrollminz;                     // Limit level z-scroll
float               scrollmaxz;
float               blockade;                       // Limit x scroll back
float               lasthitx;						//Last hit X location.
float               lasthitz;						//Last hit Z location.
float               lasthita;						//Last hit A location.
int                 lasthitt;                       //Last hit type.
int                 lasthitc;                       //Last hit confirm (i.e. if engine hit code will be used).

// used by gfx shadow
int                 light[2] = {0, 0};
int                 shadowcolor = 0;
int                 shadowalpha = BLEND_MULTIPLY+1;

u64 totalram = 0;
u64 usedram = 0;
u64 freeram = 0;
u32 interval = 0;
extern unsigned long seed;

int                 SAMPLE_GO			= -1;
int                 SAMPLE_BEAT			= -1;
int                 SAMPLE_BLOCK		= -1;
int                 SAMPLE_INDIRECT		= -1;
int                 SAMPLE_GET			= -1;
int                 SAMPLE_GET2			= -1;
int                 SAMPLE_FALL			= -1;
int                 SAMPLE_JUMP			= -1;
int                 SAMPLE_PUNCH		= -1;
int                 SAMPLE_1UP			= -1;
int                 SAMPLE_TIMEOVER		= -1;
int                 SAMPLE_BEEP			= -1;
int                 SAMPLE_BEEP2		= -1;
int                 SAMPLE_BIKE			= -1;

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
int*                animdowns           = NULL;
int*                animups             = NULL;
int*                animbackwalks       = NULL;
int*                animwalks           = NULL;
int*                animidles           = NULL;
int*                animpains           = NULL;
int*                animdies            = NULL;
int*                animfalls           = NULL;
int*                animrises           = NULL;
int*                animriseattacks     = NULL;
int*                animblkpains        = NULL;
int*                animattacks         = NULL;
int*                animfollows         = NULL;
int*                animspecials        = NULL;

// system default values
int                 downs[MAX_DOWNS]        = {ANI_DOWN};
int                 ups[MAX_UPS]            = {ANI_UP};
int                 backwalks[MAX_BACKWALKS]= {ANI_BACKWALK};
int                 walks[MAX_WALKS]        = {ANI_WALK};
int                 idles[MAX_IDLES]        = {ANI_IDLE};

int                 falls[MAX_ATKS] = {
                        ANI_FALL,  ANI_FALL2, ANI_FALL3, ANI_FALL4,
                        ANI_FALL,  ANI_BURN,  ANI_FALL,  ANI_SHOCK,
                        ANI_FALL,  ANI_FALL5, ANI_FALL6, ANI_FALL7,
                        ANI_FALL8, ANI_FALL9, ANI_FALL10
                    };

int                 rises[MAX_ATKS] = {
                        ANI_RISE,  ANI_RISE2, ANI_RISE3, ANI_RISE4,
                        ANI_RISE,  ANI_RISEB,  ANI_RISE,  ANI_RISES,
                        ANI_RISE,  ANI_RISE5, ANI_RISE6, ANI_RISE7,
                        ANI_RISE8, ANI_RISE9, ANI_RISE10
                    };

int                 riseattacks[MAX_ATKS] = {
                        ANI_RISEATTACK,  ANI_RISEATTACK2, ANI_RISEATTACK3, ANI_RISEATTACK4,
                        ANI_RISEATTACK,  ANI_RISEATTACKB,  ANI_RISEATTACK,  ANI_RISEATTACKS,
                        ANI_RISEATTACK,  ANI_RISEATTACK5, ANI_RISEATTACK6, ANI_RISEATTACK7,
                        ANI_RISEATTACK8, ANI_RISEATTACK9, ANI_RISEATTACK10
                    };

int                 pains[MAX_ATKS] = {
                        ANI_PAIN,  ANI_PAIN2,    ANI_PAIN3, ANI_PAIN4,
                        ANI_PAIN,  ANI_BURNPAIN, ANI_PAIN,  ANI_SHOCKPAIN,
                        ANI_PAIN,  ANI_PAIN5,    ANI_PAIN6, ANI_PAIN7,
                        ANI_PAIN8, ANI_PAIN9,    ANI_PAIN10
                    };

int                 deaths[MAX_ATKS] = {
                        ANI_DIE,   ANI_DIE2,     ANI_DIE3,  ANI_DIE4,
                        ANI_DIE,   ANI_BURNDIE,  ANI_DIE,   ANI_SHOCKDIE,
                        ANI_DIE,   ANI_DIE5,     ANI_DIE6,  ANI_DIE7,
                        ANI_DIE8,  ANI_DIE9,     ANI_DIE10
                    };

int                 blkpains[MAX_ATKS] = {
                        ANI_BLOCKPAIN,  ANI_BLOCKPAIN2,    ANI_BLOCKPAIN3, ANI_BLOCKPAIN4,
                        ANI_BLOCKPAIN,  ANI_BLOCKPAINB, ANI_BLOCKPAIN,  ANI_BLOCKPAINS,
                        ANI_BLOCKPAIN,  ANI_BLOCKPAIN5,    ANI_BLOCKPAIN6, ANI_BLOCKPAIN7,
                        ANI_BLOCKPAIN8, ANI_BLOCKPAIN9,    ANI_BLOCKPAIN10
                    };

int                 normal_attacks[MAX_ATTACKS] = {
                        ANI_ATTACK1, ANI_ATTACK2, ANI_ATTACK3, ANI_ATTACK4
                    };

int                 grab_attacks[5][2] = {
                        {ANI_GRABATTACK, ANI_GRABATTACK2},
                        {ANI_GRABFORWARD, ANI_GRABFORWARD2},
                        {ANI_GRABUP, ANI_GRABUP2},
                        {ANI_GRABDOWN, ANI_GRABDOWN2},
                        {ANI_GRABBACKWARD, ANI_GRABBACKWARD2}
                    };

int                 freespecials[MAX_SPECIALS] = {
                        ANI_FREESPECIAL,   ANI_FREESPECIAL2,  ANI_FREESPECIAL3,
						ANI_FREESPECIAL4,  ANI_FREESPECIAL5,  ANI_FREESPECIAL6,
						ANI_FREESPECIAL7,  ANI_FREESPECIAL8
                    };

int                 follows[MAX_FOLLOWS] = {
                        ANI_FOLLOW1, ANI_FOLLOW2, ANI_FOLLOW3, ANI_FOLLOW4
                    };

//movie log stuffs
#ifndef DC
#define MOVIEBUF_LEN 2048
int movielog = 0;
int movieplay = 0;
int moviebufptr = 0;
int movielen = 0;
int movieloglen = 0;
FILE* moviefile = NULL;
u32 (*moviebuffer)[5][2] = NULL; //keyflags, newkeyflags;
#endif

// background cache to speed up in-game menus
#if WII
s_screen*           bg_cache[MAX_CACHED_BACKGROUNDS] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
unsigned char		bg_palette_cache[MAX_CACHED_BACKGROUNDS][MAX_PAL_SIZE];
#endif

int                 maxplayers[MAX_DIFFICULTIES] = {2,2,2,2,2,2,2,2,2,2};
int                 ctrlmaxplayers[MAX_DIFFICULTIES] = {0,0,0,0,0,0,0,0,0,0};
unsigned int        num_levels[MAX_DIFFICULTIES];
unsigned int        ifcomplete[MAX_DIFFICULTIES];
unsigned int        num_difficulties;
unsigned int        noshowhof[MAX_DIFFICULTIES];
unsigned int        difflives[MAX_DIFFICULTIES];				// What too easy?  change the # of lives players get
unsigned int        custfade[MAX_DIFFICULTIES];
unsigned int        diffcreds[MAX_DIFFICULTIES];				// What still to easy - lets see how they do without continues!
unsigned int        diffoverlap[MAX_DIFFICULTIES];				// Music overlap
unsigned int        typemp[MAX_DIFFICULTIES];
unsigned int        continuescore[MAX_DIFFICULTIES];             //what to do with score if continue is used.
char*               (*skipselect)[MAX_DIFFICULTIES][MAX_PLAYERS] = NULL;              // skips select screen and automatically gives players models specified
int                 cansave_flag[MAX_DIFFICULTIES];             // 0, no save, 1 save level position 2 save all: lives/credits/hp/mp/also player

int                 cameratype          = 0;
u32                 advancetime         = 0;
u32                 quaketime           = 0;
u32                 go_time             = 0;
u32                 time                = 0;
u32                 newtime             = 0;
unsigned char       slowmotion[3]       = {0,2,0};              // [0] = enable/disable; [1] = duration; [2] = counter;
int                 disablelog          = 0;
int                 currentspawnplayer  = 0;
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
int                 levelpos            = 0;
int                 current_spawn       = 0;
int                 level_waiting       = 0;
int                 level_completed     = 0;
int                 nojoin              = 0;					// dont allow new hero to join in, use "Please Wait" instead of "Select Hero"
int                 groupmin            = 0;
int					groupmax            = 0;
int                 selectScreen        = 0;					// Flag to determine if at select screen (used for setting animations)
int                 quake               = 0;
int                 tospeedup           = 0;          			// If set will speed the level back up after a boss hits the ground
int                 reached[4]          = {0,0,0,0};			// Used with TYPE_ENDLEVEL to determine which players have reached the point //4player
int                 noslowfx			= 0;           			// Flag to determine if sound speed when hitting opponent slows or not
int                 equalairpause 		= 0;         			// If set to 1, there will be no extra pausetime for players who hit multiple enemies in midair
int                 hiscorebg			= 0;					// If set to 1, will look for a background image to display at the highscore screen
int                 completebg			= 0;           			// If set to 1, will look for a background image to display at the showcomplete screen
int                 loadingbg[2][7]		= {{-1,0,0,0,0,0,0},{-1,0,0,0,0,0,0}};  // If set to 1, will look for a background image to display at the loading screen
int					loadingmusic        = 0;
int                 unlockbg            = 0;         			// If set to 1, will look for a different background image after defeating the game
int                 pause               = 0;
int                 nopause             = 0;                    // OX. If set to 1 , pausing the game will be disabled.
int                 noscreenshot        = 0;                    // OX. If set to 1 , taking screenshots is disabled.
int                 endgame             = 0;
int                 forcecheatsoff      = 0;
int                 cheats              = 0;
int                 livescheat          = 0;
int                 keyscriptrate       = 0;
int                 creditscheat        = 0;
int                 healthcheat         = 0;
int                 showtimeover        = 0;
int                 sameplayer          = 0;            		// 7-1-2005  flag to determine if players can use the same character
int                 PLAYER_LIVES        = 3;					// 7-1-2005  default setting for Lives
int                 CONTINUES           = 5;					// 7-1-2005  default setting for continues
int                 colourselect		= 0;					// 6-2-2005 Colour select is optional
int                 autoland			= 0;					// Default set to no autoland and landing is valid with u j combo
int                 ajspecial			= 0;					// Flag to determine if holding down attack and pressing jump executes special
int                 nolost				= 0;					// variable to control if drop weapon when grab a enemy by tails
int                 nocost				= 0;					// If set, special will not cost life unless an enemy is hit
int                 mpstrict			= 0;					// If current system will check all animation's energy cost when set new animations
int                 magic_type			= 0;					// use for restore mp by time by tails
entity*             textbox				= NULL;
entity*             smartbomber			= NULL;
int                 plife[4][2]         = {{0,0},{0,0},{0,0},{0,0}};// Used for customizable player lifebar
int                 plifeX[4][3]        = {{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1}};// Used for customizable player lifebar 'x'
int                 plifeN[4][3]        = {{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1}};// Used for customizable player lifebar number of lives
int                 picon[4][2]         = {{0,0},{0,0},{0,0},{0,0}};// Used for customizable player icon
int                 piconw[4][2]        = {{0,0},{0,0},{0,0},{0,0}};// Used for customizable player weapon icons
int                 mpicon[4][2]        = {{0,0},{0,0},{0,0},{0,0}};// Used for customizable magicbar player icon
int                 pnameJ[4][7]        = {{0,0,0,0,0,0,-1},{0,0,0,0,0,0,-1},{0,0,0,0,0,0,-1},{0,0,0,0,0,0,-1}};// Used for customizable player name, Select Hero, (Credits, Press Start, Game Over) when joining
int                 pscore[4][7]        = {{0,0,0,0,0,0,-1},{0,0,0,0,0,0,-1},{0,0,0,0,0,0,-1},{0,0,0,0,0,0,-1}};// Used for customizable player name, dash, score
int                 pshoot[4][3]        = {{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1}};// Used for customizable player shootnum
int                 prush[4][8]         = {{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0}};// Used for customizable player combo/rush system
int                 psmenu[4][4]        = {{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};// Used for customizable player placement in select menu
int                 mpcolourtable[11]   = {0,0,0,0,0,0,0,0,0,0,0};
int                 hpcolourtable[11]   = {0,0,0,0,0,0,0,0,0,0,0};
int                 ldcolourtable[11]   = {0,0,0,0,0,0,0,0,0,0,0};
char                musicname[128]      = {""};
float               musicfade[2]        = {0,0};
int                 musicloop           = 0;
u32                 musicoffset         = 0;

s_barstatus         loadingbarstatus =
{
    0,                          //int          offsetx:16;
    0,                          //int          offsety:16;
    0,                          //int          sizex:16;
    10,                          //int          sizey:16;
    percentagebar,                   //bartype      type:8;
    horizontalbar,              //barorient    orientation:8;
    0,                          //int          noborder:8;
    0,                          //int          direction:8;
    0,                          //int          barlayer;
    0,                          //int          backlayer;
    0,                          //int          borderlayer;
    0,                          //int          shadowlayer;
    &ldcolourtable
};
s_barstatus         lbarstatus =                                // Used for customizable lifebar size
{
    0,                          //int          offsetx:16;
    0,                          //int          offsety:16;
    0,                          //int          sizex:16;
    0,                          //int          sizey:16;
    valuebar,                   //bartype      type:8;
    horizontalbar,              //barorient    orientation:8;
    0,                          //int          noborder:8;
    0,                          //int          direction:8;
    0,                          //int          barlayer;
    0,                          //int          backlayer;
    0,                          //int          borderlayer;
    0,                          //int          shadowlayer;
    &hpcolourtable
};
s_barstatus         olbarstatus =                               // Used for customizable opponent lifebar size
{
    0,                          //int          offsetx:16;
    0,                          //int          offsety:16;
    0,                          //int          sizex:16;
    0,                          //int          sizey:16;
    valuebar,                   //bartype      type:8;
    horizontalbar,              //barorient    orientation:8;
    0,                          //int          noborder:8;
    0,                          //int          direction:8;
    0,                          //int          barlayer;
    0,                          //int          backlayer;
    0,                          //int          borderlayer;
    0,                          //int          shadowlayer;
    &hpcolourtable
};
int                 timeloc[6]			= {0,0,0,0,0,-1};		// Used for customizable timeclock location/size
int                 timeicon			= -1;
short               timeicon_offsets[2] = {0,0};
char                timeicon_path[128]  = {""};
int                 bgicon   			= -1;
short               bgicon_offsets[3]	= {0,0,0};
char                bgicon_path[128]    = {""};
int                 olicon    			= -1;
short               olicon_offsets[3]	= {0,0,0};
char                olicon_path[128]    = {""};
int                 elife[4][2]         = {{0,0},{0,0},{0,0},{0,0}};// Used for customizable enemy lifebar
int                 ename[4][3]         = {{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1}};// Used for customizable enemy name
int                 eicon[4][2]         = {{0,0},{0,0},{0,0},{0,0}};// Used for customizable enemy icon
int                 scomplete[6]		= {0,0,0,0,0,0};		// Used for customizable Stage # Complete
int                 cbonus[10]          = {0,0,0,0,0,0,0,0,0,0};// Used for customizable clear bonus
int                 lbonus[10]          = {0,0,0,0,0,0,0,0,0,0};// Used for customizable life bonus
int                 rbonus[10]          = {0,0,0,0,0,0,0,0,0,0};// Used for customizable rush bonus
int                 tscore[10]          = {0,0,0,0,0,0,0,0,0,0};// Used for customizable total score
int                 scbonuses[4]        = {10000, 1000, 100, 0};//Stage complete bonus multipliers
int                 showrushbonus       = 0;
int                 noshare				= 0;					// Used for when you want to keep p1 & p2 credits separate
int                 nodropen			= 0;					// Drop or not when spawning is now a modder option
int                 gfx_y_offset		= 0;
int                 timeleft			= 0;
int                 oldtime             = 0;                    // One second back from time left.
int                 holez				= 0;					// Used for setting spawn points
int                 allow_secret_chars	= 0;
unsigned int        lifescore			= 50000;				// Number of points needed to earn a 1-up
unsigned int        credscore			= 0;					// Number of points needed to earn a credit
int                 mpblock				= 0;					// Take chip damage from health or MP first?
int                 blockratio			= 0;					// Take half-damage while blocking?
int                 nochipdeath			= 0;					// Prevents entities from dying due to chip damage (damage while blocking)
int                 noaircancel         = 0;					// Now, you can make jumping attacks uncancellable!
int                 nomaxrushreset[5]   = {0,0,0,0,0};
s_barstatus         mpbarstatus =                               // Used for customizable lifebar size
{
    0,                          //int          offsetx:16;
    0,                          //int          offsety:16;
    0,                          //int          sizex:16;
    0,                          //int          sizey:16;
    valuebar,                   //bartype      type:8;
    horizontalbar,              //barorient    orientation:8;
    0,                          //int          noborder:8;
    0,                          //int          direction:8;
    0,                          //int          barlayer;
    0,                          //int          backlayer;
    0,                          //int          borderlayer;
    0,                          //int          shadowlayer;
    &mpcolourtable
};
int			        mpbartext[4]		= {-1,0,0,0};			// Array for adjusting MP status text (font, Xpos, Ypos, Display type).
int			        lbartext[4]			= {-1,0,0,0};			// Array for adjusting HP status text (font, Xpos, Ypos, Display type).
int                 pmp[4][2]			= {{0,0},{0,0},{0,0},{0,0}};// Used for customizable player mpbar
int                 spdirection[4]		= {1,0,1,0};			// Used for Select Player Direction for select player screen
int                 bonus				= 0;					// Used for unlocking Bonus difficulties
int                 versusdamage		= 2;					// Used for setting mode. (ability to hit other players)
int                 same[MAX_DIFFICULTIES];						// ltb 1-13-05   sameplayer
int                 z_coords[3]			= {0,0,0};				// Used for setting customizable walkable area
int                 rush[6]				= {0,2,3,3,3,3};
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
int                 shadowsprites[6]	= {-1,-1,-1,-1,-1,-1};
int                 gosprite			= -1;
int                 golsprite			= -1;
int                 holesprite			= -1;
int                 videoMode			= 0;
int                 scoreformat			= 0;					// If set fill score values with 6 Zeros

// Funny neon lights
unsigned char       neontable[MAX_PAL_SIZE];
unsigned int        neon_time			= 0;

s_panel             panels[MAX_PANELS];
unsigned int        panels_loaded		= 0;
int                 panel_width			= 0;
int                 panel_height		= 0;

s_sprite*           frontpanels[MAX_PANELS];
unsigned int        frontpanels_loaded	= 0;

unsigned int        sprites_loaded		= 0;
unsigned int        anims_loaded		= 0;

unsigned int        models_loaded		= 0;
unsigned int        models_cached		= 0;

entity*             ent_list[MAX_ENTS];
entity*             self;
int                 ent_count			= 0;					// log count of entites
int                 ent_max				= 0;

s_player            player[4];
u32                 bothkeys, bothnewkeys;

s_playercontrols    playercontrols1;
s_playercontrols    playercontrols2;
s_playercontrols    playercontrols3;
s_playercontrols    playercontrols4;
s_playercontrols*   playercontrolpointers[] = {&playercontrols1, &playercontrols2, &playercontrols3, &playercontrols4};


//global script
Script level_script;    //execute when level start
Script endlevel_script; //execute when level finished
Script update_script;   //execute when ingame update
Script updated_script;  //execute when ingame update finished
Script key_script_all;  //keyscript for all players
Script timetick_script; //time tick script.

//player script
Script score_script[4];     //execute when add score, 4 players
Script key_script[4];       //key listeners, lol
Script join_script[4];      //player join scripts
Script respawn_script[4];   //player respawn scripts
Script pdie_script[4];      //player death scripts

extern Script* pcurrentscript;//used by local script functions
//-------------------------methods-------------------------------

// returns: 1 - succeeded 0 - failed
int buffer_pakfile(char* filename, char** pbuffer, size_t* psize)
{
    int handle;
    *psize = 0;
    *pbuffer = NULL;
    // Read file
    if((handle=openpackfile(filename,packfile)) < 0) return 0;
    *psize = seekpackfile(handle,0,SEEK_END);
    seekpackfile(handle,0,SEEK_SET);

    *pbuffer = (char*)tracemalloc("buffer_packfile",*psize+1);
    if(*pbuffer == NULL){
        *psize = 0;
        closepackfile(handle);
        shutdown(1, "Can't create buffer for packfile '%s'", filename);
        return 0;
    }
    if(readpackfile(handle, *pbuffer, *psize) != *psize){
        if(*pbuffer != NULL){
            tracefree(*pbuffer);
            *pbuffer = NULL;
            *psize = 0;
        }
        closepackfile(handle);
        shutdown(1, "Can't read from packfile '%s'", filename);
        return 0;
    }
    (*pbuffer)[*psize] = 0;        // Terminate string (important!)
    closepackfile(handle);
    return 1;
}

//this method is used by script engine, we move it here
// it will get a system property, put it in the ScriptVariant
// if failed return 0, otherwise return 1
int getsyspropertybyindex(ScriptVariant* var, int index)
{
	enum nameenum
    {
        _e_branchname,
        _e_count_enemies,
        _e_count_entities,
        _e_count_npcs,
        _e_count_players,
        _e_current_level,
        _e_current_palette,
        _e_current_set,
        _e_current_stage,
        _e_elapsed_time,
        _e_ent_max,
        _e_game_paused,
        _e_game_speed,
		_e_gfx_y_offset,
        _e_hResolution,
        _e_in_level,
        _e_in_selectscreen,
        _e_lasthita,
		_e_lasthitc,
        _e_lasthitt,
        _e_lasthitx,
        _e_lasthitz,
        _e_levelheight,
        _e_levelwidth,
        _e_lightx,
        _e_lightz,
        _e_maxentityvars,
        _e_maxglobalvars,
        _e_maxindexedvars,
        _e_maxplayers,
        _e_maxscriptvars,
        _e_models_cached,
        _e_models_loaded,
        _e_numpalettes,
        _e_pixelformat,
        _e_player,
        _e_player1,
        _e_player2,
        _e_player3,
        _e_player4,
        _e_player_max_z,
        _e_player_min_z,
        _e_shadowalpha,
        _e_shadowcolor,
        _e_slowmotion,
        _e_slowmotion_duration,
        _e_totalram,
		_e_freeram,
        _e_usedram,
        _e_vResolution,
        _e_xpos,
        _e_ypos,
        _e_the_end,
    };

    if(!var) return 0;

    switch(index)
    {
    case _e_count_enemies:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)count_ents(TYPE_ENEMY);
        break;
    case _e_count_players:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)count_ents(TYPE_PLAYER);
        break;
    case _e_count_npcs:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)count_ents(TYPE_NPC);
        break;
    case _e_count_entities:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)ent_count;
        break;
    case _e_ent_max:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)ent_max;
        break;
    case _e_in_level:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)(level == NULL);
        break;
    case _e_elapsed_time:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)time;
        break;
    case _e_game_speed:
        if(!level) return 0;
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)GAME_SPEED;
        break;
    case _e_gfx_y_offset:
        if(!level) return 0;
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)gfx_y_offset;
        break;
    case _e_in_selectscreen:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)(selectScreen);
        break;
    case _e_lasthita:
        ScriptVariant_ChangeType(var, VT_DECIMAL);
        var->dblVal = (DOUBLE)(lasthita);
        break;
	case _e_lasthitc:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)(lasthitc);
        break;
    case _e_lasthitt:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)(lasthitt);
        break;
    case _e_lasthitx:
        ScriptVariant_ChangeType(var, VT_DECIMAL);
        var->dblVal = (DOUBLE)(lasthitx);
        break;
    case _e_lasthitz:
        ScriptVariant_ChangeType(var, VT_DECIMAL);
        var->dblVal = (DOUBLE)(lasthitz);
        break;
    case _e_xpos:
        if(!level) return 0;
        ScriptVariant_ChangeType(var, VT_DECIMAL);
        var->dblVal = (DOUBLE)advancex;
        break;
    case _e_ypos:
        if(!level) return 0;
        ScriptVariant_ChangeType(var, VT_DECIMAL);
        var->dblVal = (DOUBLE)advancey;
        break;
    case _e_hResolution:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)videomodes.hRes;
        break;
    case _e_vResolution:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)videomodes.vRes;
        break;
    case _e_current_set:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)(current_set);
        break;
    case _e_current_level:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)(current_level);
        break;
    case _e_current_palette:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)(current_palette);
        break;
    case _e_current_stage:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)(current_stage);
        break;
    case _e_levelwidth:
        if(!level) return 0;
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)(level->width);
        break;
    case _e_levelheight:
        if(!level) return 0;
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)(panel_height);
        break;
    case _e_branchname:
        ScriptVariant_ChangeType(var, VT_STR);
        strcpy(StrCache_Get(var->strVal), branch_name);
        break;
    case _e_maxentityvars:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)max_entity_vars;
        break;
    case _e_maxglobalvars:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)max_global_vars;
        break;
    case _e_maxindexedvars:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)max_indexed_vars;
        break;
    case _e_maxplayers:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)maxplayers[current_set];
        break;
    case _e_maxscriptvars:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)max_script_vars;
        break;
    case _e_models_cached:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)models_cached;
        break;
    case _e_models_loaded:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)models_loaded;
        break;
    case _e_numpalettes:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)(level->numpalettes);
        break;
    case _e_pixelformat:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)pixelformat;
        break;
    case _e_player:
    case _e_player1:
        ScriptVariant_ChangeType(var, VT_PTR);
        var->ptrVal = (VOID*)player;
        break;
    case _e_player2:
        ScriptVariant_ChangeType(var, VT_PTR);
        var->ptrVal = (VOID*)(player+1);
        break;
    case _e_player3:
        ScriptVariant_ChangeType(var, VT_PTR);
        var->ptrVal = (VOID*)(player+2);
        break;
    case _e_player4:
        ScriptVariant_ChangeType(var, VT_PTR);
        var->ptrVal = (VOID*)(player+3);
        break;
    case _e_player_max_z:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)(PLAYER_MAX_Z);
        break;
    case _e_player_min_z:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)(PLAYER_MIN_Z);
        break;
    case _e_lightx:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)(light[0]);
        break;
    case _e_lightz:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)(light[1]);
        break;
    case _e_shadowalpha:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)shadowalpha;
        break;
    case _e_shadowcolor:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)shadowcolor;
        break;
    case _e_slowmotion:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)slowmotion[0];
        break;
    case _e_slowmotion_duration:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)slowmotion[1];
        break;
    case _e_game_paused:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (LONG)pause;
        break;
    case _e_totalram:
         ScriptVariant_ChangeType(var, VT_INTEGER);
         var->lVal = (LONG)getSystemRam(KBYTES);
        break;
	case _e_freeram:
		ScriptVariant_ChangeType(var, VT_INTEGER);
		var->lVal = (LONG)getFreeRam(KBYTES);
		break;
    case _e_usedram:
         ScriptVariant_ChangeType(var, VT_INTEGER);
         var->lVal = (LONG)getUsedRam(KBYTES);
        break;
    default:
		// We use indices now, but players/modders don't need to be exposed
		// to that implementation detail, so we write "name" and not "index".
        printf("Unknown system property name.\n");
        return 0;
    }
    return 1;
}

// change a system variant, used by script
int changesyspropertybyindex(int index, ScriptVariant* value)
{
    //char* tempstr = NULL;
    LONG ltemp;
    //DOUBLE dbltemp;

	// This enum is replicated in mapstrings_changesystemvariant in
	// openborscript.c. If you change one, you must change the other as well!!!!
	enum changesystemvariant_enum
    {
		_csv_blockade,
		_csv_elapsed_time,
		_csv_lasthita,
		_csv_lasthitc,
		_csv_lasthitt,
		_csv_lasthitx,
		_csv_lasthitz,
		_csv_levelpos,
		_csv_scrollmaxz,
		_csv_scrollminz,
		_csv_slowmotion,
		_csv_slowmotion_duration,
		_csv_smartbomber,
		_csv_textbox,
		_csv_xpos,
		_csv_ypos,
		_csv_the_end,
     };

	switch(index)
	{
    case _csv_elapsed_time:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
            time = (int)ltemp;
		break;
    case _csv_levelpos:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
            levelpos = (int)ltemp;
		break;
    case _csv_xpos:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
            advancex = (float)ltemp;
		break;
    case _csv_ypos:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
            advancey = (float)ltemp;
		break;
    case _csv_scrollminz:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
            scrollminz = (float)ltemp;
		break;
    case _csv_scrollmaxz:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
            scrollmaxz = (float)ltemp;
		break;
    case _csv_blockade:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
            blockade = (float)ltemp;
		break;
    case _csv_slowmotion:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
            slowmotion[0] = (unsigned char)ltemp;
		break;
    case _csv_slowmotion_duration:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
            slowmotion[1] = (unsigned char)ltemp;
		break;
    case _csv_lasthitx:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
            lasthitx = (float)ltemp;
		break;
    case _csv_lasthita:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
            lasthita = (float)ltemp;
		break;
	 case _csv_lasthitc:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
            lasthitc = (int)ltemp;
        break;
    case _csv_lasthitz:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
            lasthitz = (float)ltemp;
		break;
    case _csv_lasthitt:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
            lasthitt = (int)ltemp;
        break;
    case _csv_smartbomber:
        smartbomber = (entity*)value;
        break;
    case _csv_textbox:
        textbox = (entity*)value;
		break;
	default:
		return 0;
    }

    return 1;
}


int load_script(Script* script, char* file)
{
	size_t size = 0;
	int failed = 0;
	char* buf = NULL;

	if(buffer_pakfile(file, &buf, &size)!=1) return 0;

	failed = !Script_AppendText(script, buf, file);

	if(buf != NULL)
	{
		tracefree(buf);
		buf = NULL;
	}

	// text loaded but parsing failed, shutdown
	if(failed) shutdown(1, "Failed to parse script file: '%s'!\n", file);
	return !failed;
}

// this method is used by load_scripts, don't call it
void init_scripts()
{
	int i;
	Script_Global_Init();
	Script_Init(&update_script,     "update",   1);
	Script_Init(&level_script,      "level",    1);
	Script_Init(&endlevel_script,   "endlevel", 1);
	Script_Init(&key_script_all,    "keyall",   1);
	Script_Init(&timetick_script,   "timetick", 1);
	for(i=0; i<4; i++) Script_Init(&score_script[i],    "score",    1);
	for(i=0; i<4; i++) Script_Init(&key_script[i],      "key",      1);
	for(i=0; i<4; i++) Script_Init(&join_script[i],     "join",     1);
	for(i=0; i<4; i++) Script_Init(&respawn_script[i],  "respawn",  1);
	for(i=0; i<4; i++) Script_Init(&pdie_script[i],     "die",      1);
}

// This method is called once when the engine starts, do not use it multiple times
// It should be calld after load_script_setting
void load_scripts()
{
	int i;
	init_scripts();
	//Script_Clear's second parameter set to 2, because the script fails to load,
	//and will never have another chance to be loaded, so just clear the variable list in it
	if(!load_script(&update_script,     "data/scripts/update.c"))   Script_Clear(&update_script,        2);
	if(!load_script(&updated_script,    "data/scripts/updated.c"))  Script_Clear(&updated_script,       2);
	if(!load_script(&level_script,      "data/scripts/level.c"))    Script_Clear(&level_script,         2);
	if(!load_script(&endlevel_script,   "data/scripts/endlevel.c")) Script_Clear(&endlevel_script,      2);
	if(!load_script(&key_script_all,    "data/scripts/keyall.c"))   Script_Clear(&key_script_all,       2);
	if(!load_script(&timetick_script,   "data/scripts/timetick.c")) Script_Clear(&timetick_script,      2);
	if(!load_script(&score_script[0],   "data/scripts/score1.c"))   Script_Clear(&score_script[0],      2);
	if(!load_script(&score_script[1],   "data/scripts/score2.c"))   Script_Clear(&score_script[1],      2);
	if(!load_script(&score_script[2],   "data/scripts/score3.c"))   Script_Clear(&score_script[2],      2);
	if(!load_script(&score_script[3],   "data/scripts/score4.c"))   Script_Clear(&score_script[3],      2);
	if(!load_script(&key_script[0],     "data/scripts/key1.c"))     Script_Clear(&key_script[0],        2);
	if(!load_script(&key_script[1],     "data/scripts/key2.c"))     Script_Clear(&key_script[1],        2);
	if(!load_script(&key_script[2],     "data/scripts/key3.c"))     Script_Clear(&key_script[2],        2);
	if(!load_script(&key_script[3],     "data/scripts/key4.c"))     Script_Clear(&key_script[3],        2);
	if(!load_script(&join_script[0],    "data/scripts/join1.c"))    Script_Clear(&join_script[0],       2);
	if(!load_script(&join_script[1],    "data/scripts/join2.c"))    Script_Clear(&join_script[1],       2);
	if(!load_script(&join_script[2],    "data/scripts/join3.c"))    Script_Clear(&join_script[2],       2);
	if(!load_script(&join_script[3],    "data/scripts/join4.c"))    Script_Clear(&join_script[3],       2);
	if(!load_script(&respawn_script[0], "data/scripts/respawn1.c")) Script_Clear(&respawn_script[0],    2);
	if(!load_script(&respawn_script[1], "data/scripts/respawn2.c")) Script_Clear(&respawn_script[1],    2);
	if(!load_script(&respawn_script[2], "data/scripts/respawn3.c")) Script_Clear(&respawn_script[2],    2);
	if(!load_script(&respawn_script[3], "data/scripts/respawn4.c")) Script_Clear(&respawn_script[3],    2);
	if(!load_script(&pdie_script[0],    "data/scripts/die1.c"))     Script_Clear(&pdie_script[0],       2);
	if(!load_script(&pdie_script[1],    "data/scripts/die2.c"))     Script_Clear(&pdie_script[1],       2);
	if(!load_script(&pdie_script[2],    "data/scripts/die3.c"))     Script_Clear(&pdie_script[2],       2);
	if(!load_script(&pdie_script[3],    "data/scripts/die4.c"))     Script_Clear(&pdie_script[3],       2);
	Script_Compile(&update_script);
	Script_Compile(&updated_script);
	Script_Compile(&level_script);
	Script_Compile(&endlevel_script);
	Script_Compile(&key_script_all);
	Script_Compile(&timetick_script);
	for(i=0; i<4; i++) Script_Compile(&score_script[i]);
	for(i=0; i<4; i++) Script_Compile(&key_script[i]);
	for(i=0; i<4; i++) Script_Compile(&join_script[i]);
	for(i=0; i<4; i++) Script_Compile(&respawn_script[i]);
	for(i=0; i<4; i++) Script_Compile(&pdie_script[i]);
}

// This method is called once when the engine is shutting down, do not use it multiple times
void clear_scripts()
{
	int i;
	//Script_Clear's second parameter set to 2, because the script fails to load,
	//and will never have another chance to be loaded, so just clear the variable list in it
	Script_Clear(&update_script,    2);
	Script_Clear(&updated_script,   2);
	Script_Clear(&level_script,     2);
	Script_Clear(&endlevel_script,  2);
	Script_Clear(&key_script_all,   2);
	Script_Clear(&timetick_script,  2);
	for(i=0; i<4; i++)
		Script_Clear(&score_script[i],      2);
	for(i=0; i<4; i++)
		Script_Clear(&key_script[i],        2);
	for(i=0; i<4; i++)
		Script_Clear(&join_script[i],       2);
	for(i=0; i<4; i++)
		Script_Clear(&respawn_script[i],    2);
	for(i=0; i<4; i++)
		Script_Clear(&pdie_script[i],       2);
	Script_Global_Clear();
}

void execute_animation_script(entity* ent)
{
	ScriptVariant tempvar;
	Script* ptempscript = pcurrentscript;
	if(Script_IsInitialized(ent->animation_script))
	{
		ScriptVariant_Init(&tempvar);
		ScriptVariant_ChangeType(&tempvar, VT_PTR);
		tempvar.ptrVal = (VOID*)ent;
		Script_Set_Local_Variant("self",    &tempvar);
		ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
		tempvar.lVal = (LONG)ent->animnum;
		Script_Set_Local_Variant("animnum", &tempvar);
		ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
		tempvar.lVal = (LONG)ent->animpos;
		Script_Set_Local_Variant("frame",   &tempvar);
		Script_Execute(ent->animation_script);
		//clear to save variant space
		ScriptVariant_Clear(&tempvar);
		Script_Set_Local_Variant("self",    &tempvar);
		Script_Set_Local_Variant("animnum", &tempvar);
		Script_Set_Local_Variant("frame",   &tempvar);
	}
	pcurrentscript = ptempscript;
}

void execute_takedamage_script(entity* ent, entity* other, int force, int drop, int type, int noblock, int guardcost, int jugglecost, int pauseadd)
{
    ScriptVariant tempvar;
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(ent->takedamage_script))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID*)ent;
        Script_Set_Local_Variant("self",        &tempvar);
        tempvar.ptrVal = (VOID*)other;
        Script_Set_Local_Variant("attacker",    &tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        tempvar.lVal = (LONG)force;
        Script_Set_Local_Variant("damage",      &tempvar);
        tempvar.lVal = (LONG)drop;
        Script_Set_Local_Variant("drop",        &tempvar);
        tempvar.lVal = (LONG)type;
        Script_Set_Local_Variant("attacktype",  &tempvar);
        tempvar.lVal = (LONG)noblock;
        Script_Set_Local_Variant("noblock",     &tempvar);
        tempvar.lVal = (LONG)guardcost;
        Script_Set_Local_Variant("guardcost",   &tempvar);
        tempvar.lVal = (LONG)jugglecost;
        Script_Set_Local_Variant("jugglecost",  &tempvar);
        tempvar.lVal = (LONG)pauseadd;
        Script_Set_Local_Variant("pauseadd",    &tempvar);
        Script_Execute(ent->takedamage_script);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("self",        &tempvar);
        Script_Set_Local_Variant("attacker",    &tempvar);
        Script_Set_Local_Variant("damage",      &tempvar);
        Script_Set_Local_Variant("drop",        &tempvar);
        Script_Set_Local_Variant("attacktype",  &tempvar);
        Script_Set_Local_Variant("noblock",     &tempvar);
        Script_Set_Local_Variant("guardcost",   &tempvar);
        Script_Set_Local_Variant("jugglecost",  &tempvar);
        Script_Set_Local_Variant("pauseadd",    &tempvar);
    }
    pcurrentscript = ptempscript;
}

void execute_onpain_script(entity* ent, int iType, int iReset)
{
    ScriptVariant tempvar;
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(ent->onpain_script))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID*)ent;
        Script_Set_Local_Variant("self",        &tempvar);
        tempvar.lVal = (LONG)iType;
        Script_Set_Local_Variant("attacktype",   &tempvar);
        tempvar.lVal = (LONG)iReset;
        Script_Set_Local_Variant("reset",       &tempvar);
        Script_Execute(ent->onpain_script);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("self",        &tempvar);
        Script_Set_Local_Variant("type",        &tempvar);
        Script_Set_Local_Variant("reset",       &tempvar);
    }
    pcurrentscript = ptempscript;
}

void execute_onfall_script(entity* ent, entity* other, int force, int drop, int type, int noblock, int guardcost, int jugglecost, int pauseadd)
{
    ScriptVariant tempvar;
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(ent->onfall_script))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID*)ent;
        Script_Set_Local_Variant("self",        &tempvar);
        tempvar.ptrVal = (VOID*)other;
        Script_Set_Local_Variant("attacker",    &tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        tempvar.lVal = (LONG)force;
        Script_Set_Local_Variant("damage",      &tempvar);
        tempvar.lVal = (LONG)drop;
        Script_Set_Local_Variant("drop",        &tempvar);
        tempvar.lVal = (LONG)type;
        Script_Set_Local_Variant("attacktype",  &tempvar);
        tempvar.lVal = (LONG)noblock;
        Script_Set_Local_Variant("noblock",     &tempvar);
        tempvar.lVal = (LONG)guardcost;
        Script_Set_Local_Variant("guardcost",   &tempvar);
        tempvar.lVal = (LONG)jugglecost;
        Script_Set_Local_Variant("jugglecost",  &tempvar);
        tempvar.lVal = (LONG)pauseadd;
        Script_Set_Local_Variant("pauseadd",    &tempvar);
        Script_Execute(ent->onfall_script);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("self",        &tempvar);
        Script_Set_Local_Variant("attacker",    &tempvar);
        Script_Set_Local_Variant("damage",      &tempvar);
        Script_Set_Local_Variant("drop",        &tempvar);
        Script_Set_Local_Variant("attacktype",  &tempvar);
        Script_Set_Local_Variant("noblock",     &tempvar);
        Script_Set_Local_Variant("guardcost",   &tempvar);
        Script_Set_Local_Variant("jugglecost",  &tempvar);
        Script_Set_Local_Variant("pauseadd",    &tempvar);
    }
    pcurrentscript = ptempscript;
}

void execute_onblocks_script(entity* ent)
{
    ScriptVariant tempvar;
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(ent->onblocks_script))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID*)ent;
        Script_Set_Local_Variant("self", &tempvar);
        Script_Execute(ent->onblocks_script);

        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("self", &tempvar);
    }
    pcurrentscript = ptempscript;
}

void execute_onblockw_script(entity* ent, int plane, float height)
{
    ScriptVariant tempvar;
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(ent->onblockw_script))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID*)ent;
        Script_Set_Local_Variant("self", &tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        tempvar.lVal = (LONG)plane;
        Script_Set_Local_Variant("plane",      &tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_DECIMAL);
        tempvar.dblVal = (DOUBLE)height;
        Script_Set_Local_Variant("height",      &tempvar);
        Script_Execute(ent->onblockw_script);

        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("self", &tempvar);
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("plane", &tempvar);
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("height", &tempvar);
    }
    pcurrentscript = ptempscript;
}

void execute_onblocko_script(entity* ent, entity* other)
{
    ScriptVariant tempvar;
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(ent->onblocko_script))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID*)ent;
        Script_Set_Local_Variant("self",        &tempvar);
        tempvar.ptrVal = (VOID*)other;
        Script_Set_Local_Variant("obstacle",    &tempvar);
        Script_Execute(ent->onblocko_script);

        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("self",        &tempvar);
        Script_Set_Local_Variant("obstacle",    &tempvar);
    }
    pcurrentscript = ptempscript;
}

void execute_onblockz_script(entity* ent)
{
    ScriptVariant tempvar;
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(ent->onblockz_script))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID*)ent;
        Script_Set_Local_Variant("self", &tempvar);
        Script_Execute(ent->onblockz_script);

        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("self", &tempvar);
    }
    pcurrentscript = ptempscript;
}

void execute_onblocka_script(entity* ent, entity* other)
{
    ScriptVariant tempvar;
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(ent->onblocka_script))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID*)ent;
        Script_Set_Local_Variant("self",        &tempvar);
        tempvar.ptrVal = (VOID*)other;
        Script_Set_Local_Variant("obstacle",    &tempvar);
        Script_Execute(ent->onblocka_script);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("self",        &tempvar);
        Script_Set_Local_Variant("obstacle",    &tempvar);
    }
    pcurrentscript = ptempscript;
}

void execute_onmovex_script(entity* ent)
{
    ScriptVariant tempvar;
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(ent->onmovex_script))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID*)ent;
        Script_Set_Local_Variant("self", &tempvar);
        Script_Execute(ent->onmovex_script);

        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("self", &tempvar);
    }
    pcurrentscript = ptempscript;
}

void execute_onmovez_script(entity* ent)
{
    ScriptVariant tempvar;
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(ent->onmovez_script))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID*)ent;
        Script_Set_Local_Variant("self", &tempvar);
        Script_Execute(ent->onmovez_script);

        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("self", &tempvar);
    }
    pcurrentscript = ptempscript;
}

void execute_onmovea_script(entity* ent)
{
    ScriptVariant tempvar;
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(ent->onmovea_script))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID*)ent;
        Script_Set_Local_Variant("self", &tempvar);
        Script_Execute(ent->onmovea_script);

        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("self", &tempvar);
    }
    pcurrentscript = ptempscript;
}

void execute_ondeath_script(entity* ent, entity* other, int force, int drop, int type, int noblock, int guardcost, int jugglecost, int pauseadd)
{
    ScriptVariant tempvar;
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(ent->ondeath_script))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID*)ent;
        Script_Set_Local_Variant("self",        &tempvar);
        tempvar.ptrVal = (VOID*)other;
        Script_Set_Local_Variant("attacker",    &tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        tempvar.lVal = (LONG)force;
        Script_Set_Local_Variant("damage",      &tempvar);
        tempvar.lVal = (LONG)drop;
        Script_Set_Local_Variant("drop",        &tempvar);
        tempvar.lVal = (LONG)type;
        Script_Set_Local_Variant("attacktype",  &tempvar);
        tempvar.lVal = (LONG)noblock;
        Script_Set_Local_Variant("noblock",     &tempvar);
        tempvar.lVal = (LONG)guardcost;
        Script_Set_Local_Variant("guardcost",   &tempvar);
        tempvar.lVal = (LONG)jugglecost;
        Script_Set_Local_Variant("jugglecost",  &tempvar);
        tempvar.lVal = (LONG)pauseadd;
        Script_Set_Local_Variant("pauseadd",    &tempvar);
        Script_Execute(ent->ondeath_script);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("self",        &tempvar);
        Script_Set_Local_Variant("attacker",    &tempvar);
        Script_Set_Local_Variant("damage",      &tempvar);
        Script_Set_Local_Variant("drop",        &tempvar);
        Script_Set_Local_Variant("attacktype",  &tempvar);
        Script_Set_Local_Variant("noblock",     &tempvar);
        Script_Set_Local_Variant("guardcost",   &tempvar);
        Script_Set_Local_Variant("jugglecost",  &tempvar);
        Script_Set_Local_Variant("pauseadd",    &tempvar);
    }
    pcurrentscript = ptempscript;
}

void execute_onkill_script(entity* ent)
{
    ScriptVariant tempvar;
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(ent->onkill_script))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID*)ent;
        Script_Set_Local_Variant("self", &tempvar);
        Script_Execute(ent->onkill_script);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("self", &tempvar);
    }
    pcurrentscript = ptempscript;
}

void execute_didblock_script(entity* ent, entity* other, int force, int drop, int type, int noblock, int guardcost, int jugglecost, int pauseadd)
{
    ScriptVariant tempvar;
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(ent->didblock_script))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID*)ent;
        Script_Set_Local_Variant("self",        &tempvar);
        tempvar.ptrVal = (VOID*)other;
        Script_Set_Local_Variant("attacker",    &tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        tempvar.lVal = (LONG)force;
        Script_Set_Local_Variant("damage",      &tempvar);
        tempvar.lVal = (LONG)drop;
        Script_Set_Local_Variant("drop",        &tempvar);
        tempvar.lVal = (LONG)type;
        Script_Set_Local_Variant("attacktype",  &tempvar);
        tempvar.lVal = (LONG)noblock;
        Script_Set_Local_Variant("noblock",     &tempvar);
        tempvar.lVal = (LONG)guardcost;
        Script_Set_Local_Variant("guardcost",   &tempvar);
        tempvar.lVal = (LONG)jugglecost;
        Script_Set_Local_Variant("jugglecost",  &tempvar);
        tempvar.lVal = (LONG)pauseadd;
        Script_Set_Local_Variant("pauseadd",    &tempvar);
        Script_Execute(ent->didblock_script);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("self",        &tempvar);
        Script_Set_Local_Variant("attacker",    &tempvar);
        Script_Set_Local_Variant("damage",      &tempvar);
        Script_Set_Local_Variant("drop",        &tempvar);
        Script_Set_Local_Variant("attacktype",  &tempvar);
        Script_Set_Local_Variant("noblock",     &tempvar);
        Script_Set_Local_Variant("guardcost",   &tempvar);
        Script_Set_Local_Variant("jugglecost",  &tempvar);
        Script_Set_Local_Variant("pauseadd",    &tempvar);
    }
    pcurrentscript = ptempscript;
}

void execute_ondoattack_script(entity* ent, entity* other, int force, int drop, int type, int noblock, int guardcost, int jugglecost, int pauseadd, int iWhich, int iAtkID)
{
    ScriptVariant tempvar;
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(ent->ondoattack_script))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID*)ent;
        Script_Set_Local_Variant("self",        &tempvar);
        tempvar.ptrVal = (VOID*)other;
        Script_Set_Local_Variant("other",    &tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        tempvar.lVal = (LONG)force;
        Script_Set_Local_Variant("damage",      &tempvar);
        tempvar.lVal = (LONG)drop;
        Script_Set_Local_Variant("drop",        &tempvar);
        tempvar.lVal = (LONG)type;
        Script_Set_Local_Variant("attacktype",  &tempvar);
        tempvar.lVal = (LONG)noblock;
        Script_Set_Local_Variant("noblock",     &tempvar);
        tempvar.lVal = (LONG)guardcost;
        Script_Set_Local_Variant("guardcost",   &tempvar);
        tempvar.lVal = (LONG)jugglecost;
        Script_Set_Local_Variant("jugglecost",  &tempvar);
        tempvar.lVal = (LONG)pauseadd;
        Script_Set_Local_Variant("pauseadd",    &tempvar);
		tempvar.lVal = (LONG)iWhich;
        Script_Set_Local_Variant("which",    &tempvar);
		tempvar.lVal = (LONG)iAtkID;
        Script_Set_Local_Variant("attackid",    &tempvar);
        Script_Execute(ent->ondoattack_script);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("self",        &tempvar);
        Script_Set_Local_Variant("other",		&tempvar);
        Script_Set_Local_Variant("damage",      &tempvar);
        Script_Set_Local_Variant("drop",        &tempvar);
        Script_Set_Local_Variant("attacktype",  &tempvar);
        Script_Set_Local_Variant("noblock",     &tempvar);
        Script_Set_Local_Variant("guardcost",   &tempvar);
        Script_Set_Local_Variant("jugglecost",  &tempvar);
        Script_Set_Local_Variant("pauseadd",    &tempvar);
		Script_Set_Local_Variant("which",		&tempvar);
		Script_Set_Local_Variant("attackid",	&tempvar);
    }
    pcurrentscript = ptempscript;
}

void execute_updateentity_script(entity* ent)
{
    ScriptVariant tempvar;
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(ent->update_script))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID*)ent;
        Script_Set_Local_Variant("self", &tempvar);
        Script_Execute(ent->update_script);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("self", &tempvar);
    }
    pcurrentscript = ptempscript;
}

void execute_think_script(entity* ent)
{
    ScriptVariant tempvar;
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(ent->think_script))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID*)ent;
        Script_Set_Local_Variant("self", &tempvar);
        Script_Execute(ent->think_script);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("self", &tempvar);
    }
    pcurrentscript = ptempscript;
}

void execute_didhit_script(entity* ent, entity* other, int force, int drop, int type, int noblock, int guardcost, int jugglecost, int pauseadd, int blocked)
{
    ScriptVariant tempvar;
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(ent->didhit_script))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID*)ent;
        Script_Set_Local_Variant("self",        &tempvar);
        tempvar.ptrVal = (VOID*)other;
        Script_Set_Local_Variant("damagetaker", &tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        tempvar.lVal = (LONG)force;
        Script_Set_Local_Variant("damage",      &tempvar);
        tempvar.lVal = (LONG)drop;
        Script_Set_Local_Variant("drop",        &tempvar);
        tempvar.lVal = (LONG)type;
        Script_Set_Local_Variant("attacktype",  &tempvar);
        tempvar.lVal = (LONG)noblock;
        Script_Set_Local_Variant("noblock",     &tempvar);
        tempvar.lVal = (LONG)guardcost;
        Script_Set_Local_Variant("guardcost",   &tempvar);
        tempvar.lVal = (LONG)jugglecost;
        Script_Set_Local_Variant("jugglecost",  &tempvar);
        tempvar.lVal = (LONG)pauseadd;
        Script_Set_Local_Variant("pauseadd",    &tempvar);
        tempvar.lVal = (LONG)blocked;
        Script_Set_Local_Variant("blocked",     &tempvar);
        Script_Execute(ent->didhit_script);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("self",        &tempvar);
        Script_Set_Local_Variant("damagetaker", &tempvar);
        Script_Set_Local_Variant("damage",      &tempvar);
        Script_Set_Local_Variant("drop",        &tempvar);
        Script_Set_Local_Variant("attacktype",  &tempvar);
        Script_Set_Local_Variant("noblock",     &tempvar);
        Script_Set_Local_Variant("guardcost",   &tempvar);
        Script_Set_Local_Variant("jugglecost",  &tempvar);
        Script_Set_Local_Variant("pauseadd",    &tempvar);
        Script_Set_Local_Variant("blocked",     &tempvar);
    }
    pcurrentscript = ptempscript;
}

void execute_onspawn_script(entity* ent)
{
    ScriptVariant tempvar;
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(ent->onspawn_script))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID*)ent;
        Script_Set_Local_Variant("self", &tempvar);
        Script_Execute(ent->onspawn_script);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("self", &tempvar);
    }
    pcurrentscript = ptempscript;
}

void execute_entity_key_script(entity* ent)
{
    ScriptVariant tempvar;
    Script* ptempscript ;
    if(!ent) return;
    ptempscript = pcurrentscript;
    if(Script_IsInitialized(ent->key_script))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_PTR);
        tempvar.ptrVal = (VOID*)ent;
        Script_Set_Local_Variant("self",    &tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        tempvar.lVal = (LONG)ent->playerindex;
        Script_Set_Local_Variant("player",  &tempvar);
        Script_Execute(ent->key_script);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("self",    &tempvar);
        Script_Set_Local_Variant("player",  &tempvar);
    }
    pcurrentscript = ptempscript;
}

void execute_spawn_script(s_spawn_entry* p, entity* e)
{
	s_spawn_script_list_node* tempnode = p->spawn_script_list_head;
    ScriptVariant tempvar;
    Script* ptempscript = pcurrentscript;
	while(tempnode)
    {
		pcurrentscript = tempnode->spawn_script;
        if(e)
        {
            ScriptVariant_Init(&tempvar);
            ScriptVariant_ChangeType(&tempvar, VT_PTR);
            tempvar.ptrVal = (VOID*)e;
            Script_Set_Local_Variant("self", &tempvar);
        }
        Script_Execute(tempnode->spawn_script);
        if(e)
        {
            ScriptVariant_Clear(&tempvar);
            Script_Set_Local_Variant("self", &tempvar);
        }
		tempnode = tempnode->next;
    }
    pcurrentscript = ptempscript;
}

void execute_level_key_script(int player)
{
    ScriptVariant tempvar;
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(&(level->key_script)))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        tempvar.lVal = (LONG)player;
        Script_Set_Local_Variant("player", &tempvar);
        Script_Execute(&(level->key_script));
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("player", &tempvar);
    }
    pcurrentscript = ptempscript;
}

void execute_key_script_all(int player)
{
    ScriptVariant tempvar;
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(&key_script_all))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        tempvar.lVal = (LONG)player;
        Script_Set_Local_Variant("player", &tempvar);
        Script_Execute(&key_script_all);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("player", &tempvar);
    }
    pcurrentscript = ptempscript;
}

void execute_timetick_script(int time, int gotime)
{
    ScriptVariant tempvar;
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(&timetick_script))
    {
        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        tempvar.lVal = (LONG)time;
        Script_Set_Local_Variant("time",    &tempvar);
        tempvar.lVal = (LONG)gotime;
        Script_Set_Local_Variant("gotime", &tempvar);
        Script_Execute(&timetick_script);
        //clear to save variant space
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant("time",    &tempvar);
        Script_Set_Local_Variant("gotime",  &tempvar);
    }
    pcurrentscript = ptempscript;
}

void execute_key_script(int index)
{
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(&key_script[index]))
    {
        Script_Execute(&key_script[index]);
    }
    pcurrentscript = ptempscript;
}

void execute_join_script(int index)
{
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(&join_script[index]))
    {
        Script_Execute(&join_script[index]);
    }
    pcurrentscript = ptempscript;
}

void execute_respawn_script(int index)
{
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(&respawn_script[index]))
    {
        Script_Execute(&respawn_script[index]);
    }
    pcurrentscript = ptempscript;
}

void execute_pdie_script(int index)
{
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(&pdie_script[index]))
    {
        Script_Execute(&pdie_script[index]);
    }
    pcurrentscript = ptempscript;
}

// ------------------------ Save/load -----------------------------

void clearsettings()
{
    savedata.compatibleversion = COMPATIBLEVERSION;
    savedata.gamma = 0;
    savedata.brightness = 0;
    savedata.usesound = 1;
    savedata.soundrate = 44100;
    savedata.soundbits = 16;
    savedata.soundvol = 15;
    savedata.usemusic = 1;
    savedata.musicvol = 100;
    savedata.effectvol = 120;
    savedata.usejoy = 1;
    savedata.mode = 0;
    savedata.showtitles = 0;
    savedata.windowpos = 0;
    savedata.logo = 0;
    savedata.uselog = 1;
    savedata.debuginfo = 0;
    savedata.fullscreen = 0;
    savedata.stretch = 0;

#ifdef SDL
    savedata.usegl = 0;
    savedata.glscale = 1.0;
    savedata.glfilter = 0;
#endif

	savedata.screen[0][0] = 0; savedata.screen[0][1] = 0;
    savedata.screen[1][0] = 0; savedata.screen[1][1] = 0;
    savedata.screen[2][0] = 0; savedata.screen[2][1] = 0;
    savedata.screen[3][0] = 0; savedata.screen[3][1] = 0;
    savedata.screen[4][0] = 0; savedata.screen[4][1] = 0;
    savedata.screen[5][0] = 0; savedata.screen[5][1] = 0;
    savedata.screen[6][0] = 0; savedata.screen[6][1] = 0;

#ifdef PSP
    savedata.screen[1][0] = 3;
    savedata.pspcpuspeed = 2;
	savedata.overscan[0] = 0;
	savedata.overscan[1] = 0;
	savedata.overscan[2] = 0;
	savedata.overscan[3] = 0;
#endif

    savedata.keys[0][SDID_MOVEUP]    = CONTROL_DEFAULT1_UP;
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
    savedata.keys[0][SDID_SCREENSHOT]= CONTROL_DEFAULT1_SCREENSHOT;

    savedata.keys[1][SDID_MOVEUP]    = CONTROL_DEFAULT2_UP;
    savedata.keys[1][SDID_MOVEDOWN]  = CONTROL_DEFAULT2_DOWN;
    savedata.keys[1][SDID_MOVELEFT]  = CONTROL_DEFAULT2_LEFT;
    savedata.keys[1][SDID_MOVERIGHT] = CONTROL_DEFAULT2_RIGHT;
    savedata.keys[1][SDID_ATTACK]    = CONTROL_DEFAULT2_FIRE1;
    savedata.keys[1][SDID_ATTACK2]   = CONTROL_DEFAULT2_FIRE2;
	savedata.keys[1][SDID_ATTACK3]   = CONTROL_DEFAULT2_FIRE3;
	savedata.keys[1][SDID_ATTACK4]   = CONTROL_DEFAULT2_FIRE4;
    savedata.keys[1][SDID_JUMP]      = CONTROL_DEFAULT2_FIRE5;
	savedata.keys[1][SDID_SPECIAL]   = CONTROL_DEFAULT2_FIRE6;
    savedata.keys[1][SDID_START]     = CONTROL_DEFAULT2_START;
    savedata.keys[1][SDID_SCREENSHOT]= CONTROL_DEFAULT2_SCREENSHOT;

    savedata.keys[2][SDID_MOVEUP]    = CONTROL_DEFAULT3_UP;
    savedata.keys[2][SDID_MOVEDOWN]  = CONTROL_DEFAULT3_DOWN;
    savedata.keys[2][SDID_MOVELEFT]  = CONTROL_DEFAULT3_LEFT;
    savedata.keys[2][SDID_MOVERIGHT] = CONTROL_DEFAULT3_RIGHT;
    savedata.keys[2][SDID_ATTACK]    = CONTROL_DEFAULT3_FIRE1;
	savedata.keys[2][SDID_ATTACK2]   = CONTROL_DEFAULT3_FIRE2;
	savedata.keys[2][SDID_ATTACK3]   = CONTROL_DEFAULT3_FIRE3;
	savedata.keys[2][SDID_ATTACK4]   = CONTROL_DEFAULT3_FIRE4;
    savedata.keys[2][SDID_JUMP]      = CONTROL_DEFAULT3_FIRE5;
	savedata.keys[2][SDID_SPECIAL]   = CONTROL_DEFAULT3_FIRE6;
    savedata.keys[2][SDID_START]     = CONTROL_DEFAULT3_START;
    savedata.keys[2][SDID_SCREENSHOT]= CONTROL_DEFAULT3_SCREENSHOT;

    savedata.keys[3][SDID_MOVEUP]    = CONTROL_DEFAULT4_UP;
    savedata.keys[3][SDID_MOVEDOWN]  = CONTROL_DEFAULT4_DOWN;
    savedata.keys[3][SDID_MOVELEFT]  = CONTROL_DEFAULT4_LEFT;
    savedata.keys[3][SDID_MOVERIGHT] = CONTROL_DEFAULT4_RIGHT;
    savedata.keys[3][SDID_ATTACK]    = CONTROL_DEFAULT4_FIRE1;
	savedata.keys[3][SDID_ATTACK2]   = CONTROL_DEFAULT4_FIRE2;
	savedata.keys[3][SDID_ATTACK3]   = CONTROL_DEFAULT4_FIRE3;
	savedata.keys[3][SDID_ATTACK4]   = CONTROL_DEFAULT4_FIRE4;
    savedata.keys[3][SDID_JUMP]      = CONTROL_DEFAULT4_FIRE5;
	savedata.keys[3][SDID_SPECIAL]   = CONTROL_DEFAULT4_FIRE6;
    savedata.keys[3][SDID_START]     = CONTROL_DEFAULT4_START;
    savedata.keys[3][SDID_SCREENSHOT]= CONTROL_DEFAULT4_SCREENSHOT;
}


void savesettings(){
#ifndef DC
    int disCcWarns;
    FILE *handle = NULL;
    char path[128] = {""};
	char tmpname[128] = {""};
    getBasePath(path, "Saves", 0);
	getPakName(tmpname, 4);
    strcat(path, tmpname);
    handle = fopen(path, "wb");
    if(handle==NULL) return;
    disCcWarns = fwrite(&savedata, 1, sizeof(s_savedata), handle);
    fclose(handle);
#endif
}

void saveasdefault(){
#ifndef DC
	int disCcWarns;
	FILE *handle = NULL;
	char path[128] = {""};
	getBasePath(path, "Saves", 0);
	strncat(path, "default.cfg", 128);
	handle = fopen(path, "wb");
	if(handle==NULL) return;
	disCcWarns = fwrite(&savedata, 1, sizeof(s_savedata), handle);
	fclose(handle);
#endif
}


void loadsettings(){
#ifndef DC
    int disCcWarns;
    FILE *handle = NULL;
    char path[128] = {""};
	char tmpname[128] = {""};
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
    if(handle == NULL) return;
    disCcWarns = fread(&savedata, 1, sizeof(s_savedata), handle);
    fclose(handle);
    if(savedata.compatibleversion != COMPATIBLEVERSION) clearsettings();
#else
    clearsettings();
#endif
}

void loadfromdefault(){
#ifndef DC
	int disCcWarns;
	FILE *handle = NULL;
	char path[128] = {""};
	getBasePath(path, "Saves", 0);
	strncat(path, "default.cfg", 128);
	clearsettings();
	handle = fopen(path, "rb");
	if(handle == NULL) return;
	disCcWarns = fread(&savedata, 1, sizeof(s_savedata), handle);
	fclose(handle);
	if(savedata.compatibleversion != COMPATIBLEVERSION) clearsettings();
#else
	clearsettings();
#endif
}




void clearSavedGame(){
    int i;

    for(i=0; i<MAX_DIFFICULTIES; i++){
        memset(savelevel+i, 0, sizeof(s_savelevel));
        savelevel[i].compatibleversion = CV_SAVED_GAME;
    }
}



void clearHighScore(){
    savescore.compatibleversion = CV_HIGH_SCORE;
    savescore.highsc[0] = 0;    // Resets all the highscores to 0
    savescore.highsc[1] = 0;
    savescore.highsc[2] = 0;
    savescore.highsc[3] = 0;
    savescore.highsc[4] = 0;
    savescore.highsc[5] = 0;
    savescore.highsc[6] = 0;
    savescore.highsc[7] = 0;
    savescore.highsc[8] = 0;
    savescore.highsc[9] = 0;
    strcpy(savescore.hscoren[0], "None");    // Resets all the highscore names to "None"
    strcpy(savescore.hscoren[1], "None");
    strcpy(savescore.hscoren[2], "None");
    strcpy(savescore.hscoren[3], "None");
    strcpy(savescore.hscoren[4], "None");
    strcpy(savescore.hscoren[5], "None");
    strcpy(savescore.hscoren[6], "None");
    strcpy(savescore.hscoren[7], "None");
    strcpy(savescore.hscoren[8], "None");
    strcpy(savescore.hscoren[9], "None");
}



void saveGameFile(){
#ifndef DC
    int disCcWarns;
    FILE *handle = NULL;
    char path[256] = {""};
    char tmpname[256] = {""};
    getBasePath(path, "Saves", 0);
    getPakName(tmpname, 0);
    strcat(path, tmpname);
    //if(!savelevel[saveslot].level) return;
    handle = fopen(path, "wb");
    if(handle == NULL) return;
    disCcWarns = fwrite(&savelevel, sizeof(s_savelevel), MAX_DIFFICULTIES, handle);
    fclose(handle);
#endif
}


int loadGameFile(){
#ifndef DC
    int disCcWarns;
    FILE *handle = NULL;
    int i;
    char path[256] = {""};
    char tmpname[256] = {""};
    getBasePath(path, "Saves", 0);
    getPakName(tmpname, 0);
    strcat(path,tmpname);
    handle = fopen(path, "rb");
    if(handle == NULL) return 0;
    disCcWarns = fread(&savelevel, sizeof(s_savelevel), MAX_DIFFICULTIES, handle);
    fclose(handle);
    for(i=0; i<MAX_DIFFICULTIES; i++) if(savelevel[i].compatibleversion != CV_SAVED_GAME) clearSavedGame();
    return 1;
#else
    clearSavedGame();
    return 0;
#endif
}


void saveHighScoreFile(){
#ifndef DC
    int disCcWarns;
    FILE *handle = NULL;
    char path[256] = {""};
    char tmpname[256] = {""};
    getBasePath(path, "Saves", 0);
    getPakName(tmpname, 1);
    strcat(path, tmpname);
    handle = fopen(path, "wb");
    if(handle == NULL) return;
    disCcWarns = fwrite(&savescore, 1, sizeof(s_savescore), handle);
    fclose(handle);
#endif
}


void loadHighScoreFile(){
#ifndef DC
    int disCcWarns;
    FILE *handle = NULL;
    char path[256] = {""};
    char tmpname[256] = {""};
    getBasePath(path, "Saves", 0);
    getPakName(tmpname, 1);
    strcat(path,tmpname);
    clearHighScore();
    handle = fopen(path, "rb");
    if(handle == NULL) return;
    disCcWarns = fread(&savescore, 1, sizeof(s_savescore), handle);
    fclose(handle);
    if(savescore.compatibleversion != CV_HIGH_SCORE) clearHighScore();
#else
    clearHighScore();
#endif
}


void saveScriptFile()
{
#ifndef DC
    int disCcWarns;
    FILE *handle = NULL;
    int i, l, c;
    char path[256] = {""};
    char tmpname[256] = {""};
    //named list
    //if(max_global_vars<=0) return ;
    getBasePath(path, "Saves", 0);
    getPakName(tmpname, 2);//.scr
    strcat(path, tmpname);
    l = strlen(path); //s00, s01, s02 etc
    path[l-2] = '0'+(current_set/10);
    path[l-1] = '0'+(current_set%10);
    handle = fopen(path, "wb");
    if(handle == NULL) return;
    //global variables count
    for(i=0,c=0; i<=max_global_var_index; i++)
    {
        if(!global_var_list[i]->owner) c++;
    }
    disCcWarns = fwrite(&c, sizeof(c), 1, handle);
    for(i=0; i<=max_global_var_index; i++)
    {
        if(!global_var_list[i]->owner)
            disCcWarns = fwrite(global_var_list[i], sizeof(s_variantnode), 1, handle);
    }
    // indexed list
    if(max_indexed_vars<=0) goto CLOSEF;
    disCcWarns = fwrite(indexed_var_list+i, sizeof(ScriptVariant), max_indexed_vars, handle);
    CLOSEF:
    fclose(handle);
#endif
}


void loadScriptFile(){
#ifndef DC
    int disCcWarns;

    size_t size;
    ptrdiff_t l, c;

    FILE *handle = NULL;
    char path[256] = {""};
    char tmpname[256] = {""};
    //named list
    //if(max_global_vars<=0) return ;
    getBasePath(path, "Saves", 0);
    getPakName(tmpname, 2);//.scr
    strcat(path,tmpname);
    l = strlen(path); //s00, s01, s02 etc
    path[l-2] = '0'+(current_set/10);
    path[l-1] = '0'+(current_set%10);
    handle = fopen(path, "rb");
    if(handle == NULL) return;
    fseek(handle, 0, SEEK_END);
    size = ftell(handle);
    fseek(handle, 0, SEEK_SET);
    if(size<sizeof(c)) {fclose(handle); return;}
    disCcWarns = fread(&c, sizeof(c), 1, handle);
    max_global_var_index = c;
    if(max_global_var_index >= max_global_vars)
        max_global_var_index = max_global_vars -1;
    for(size=0;size<=max_global_var_index; size++)
    {
        disCcWarns = fread(global_var_list[size], sizeof(s_variantnode), 1, handle);
    }
    //indexed list
    if(max_indexed_vars<=0) {fclose(handle);return; }
    size -= ftell(handle);
    if(size> sizeof(ScriptVariant)*max_indexed_vars) size = sizeof(ScriptVariant)*max_indexed_vars;
    disCcWarns = fread(indexed_var_list, size, 1, handle);
    fclose(handle);
#endif
}

// ----------------------- Sound ------------------------------

void music(char *filename, int loop, long offset)
{
    char t[64];
    char a[64];
    if(!savedata.usemusic) return;
    if(!sound_open_music(filename, packfile, savedata.musicvol, loop, offset))
        printf("\nCan't play music file '%s'\n", filename);
    if(savedata.showtitles && sound_query_music(a,t))
    {
        if(a[0] && t[0]) debug_printf("Playing \"%s\" by %s", t, a);
        else if(a[0]) debug_printf("Playing unknown song by %s", a);
        else if(t[0]) debug_printf("Playing \"%s\" by unknown artist", t);
        else debug_printf("");
    }
}

void check_music(){
	if(musicfade[1] > 0)
	{
		musicfade[1] -= musicfade[0];
		sound_volume_music((int)musicfade[1], (int)musicfade[1]);
	}
	else if(musicname[0])
	{
		sound_volume_music(savedata.musicvol, savedata.musicvol);
		music(musicname, musicloop, musicoffset);
		musicname[0] = 0;
	}
}

// ----------------------- General ------------------------------

char *findarg(char *command, int which){
    int d;
    int argc;
    int inarg;
    int argstart;
    static char arg[MAX_ARG_LEN];


    // Copy the command line, replacing spaces by zeroes,
    // finally returning a pointer to the requested arg.
    d = 0;
    inarg = 0;
    argstart = 0;
    argc = -1;

    while(d<MAX_ARG_LEN-1 && command[d]){
        // Zero out whitespace
        if(command[d]==' ' || command[d]=='\t'){
            arg[d] = 0;
            inarg = 0;
            if(argc == which) return arg + argstart;
        }
        else if(command[d]==0 || command[d]=='\n' || command[d]=='\r' ||
            strncmp(command+d, comment_mark, strlen(comment_mark))==0){
                // End of line
                arg[d] = 0;
                if(argc == which) return arg + argstart;
                return arg + d;
            }
        else{
            if(!inarg){
                // if(argc==-1 && command[d]=='#') return arg;
                inarg = 1;
                argstart = d;
                argc++;
            }
            arg[d] = command[d];
        }
        ++d;
    }

    return arg;
}




float diff(float a, float b){
    if(a<b) return b-a;
    return a-b;
}



int inair(entity *e)
{
    return (diff(e->a, e->base) >= 0.1);
}



float randf(float max){
    float f;
    if(max==0) return 0;
    f = (float)(rand32()%1000);
    f /= (1000/max);
    return f;
}



// ----------------------- Loaders ------------------------------


// Creates a remapping table from two images
int load_colourmap(s_model * model, char *image1, char *image2)
{
    int i, j, k;
	unsigned char *map = NULL;
    s_bitmap *bitmap1 = NULL;
    s_bitmap *bitmap2 = NULL;

    // Can't use same image twice!
    if(stricmp(image1,image2)==0) return 0;

    // Find an empty slot... ;)
    for(k=0; k<MAX_COLOUR_MAPS && model->colourmap[k]; k++);
    if(k>=MAX_COLOUR_MAPS) return -1;

	if((map = tracemalloc("load_colormap",MAX_PAL_SIZE/4)) == NULL)
	{
		return -2;
	}
    if((bitmap1 = loadbitmap(image1, packfile, PIXEL_8)) == NULL)
	{
		tracefree(map);
		map = NULL;
		return -3;
	}
    if((bitmap2 = loadbitmap(image2, packfile, PIXEL_8)) == NULL)
	{
		freebitmap(bitmap1);
		tracefree(map);
		map = NULL;
		return -4;
	}

    // Create the colour map
    for(i=0;i<MAX_PAL_SIZE/4;i++) map[i] = i;
    for(j=0; j<bitmap1->height && j<bitmap2->height; j++)
	{
        for(i=0; i<bitmap1->width && i<bitmap2->width; i++)
		{
            map[(unsigned)(bitmap1->data[j*bitmap1->width+i])] = bitmap2->data[j*bitmap2->width+i];
        }
    }

    freebitmap(bitmap1);
    freebitmap(bitmap2);

    model->colourmap[k] = map;
    model->maps_loaded = k + 1;
    return 1;
}

//PIXEL_x8
// This function is used to enable remap command in 24bit mode
// So old mods can still run under 16/24/32bit color system
// This function should be called when all colourmaps are loaded, e.g.,
// at the end of load_cached_model
// map flag is used to determine whether a colourmap is a real colourmap
int convert_map_to_palette(s_model* model, unsigned char mapflag[])
{
    int i, c;
	unsigned char *newmap, *oldmap;
    unsigned char *p1, *p2;
    unsigned pb = pixelbytes[(int)screenformat];
    if(model->palette==NULL) return 0;
    for(c=0; c<model->maps_loaded; c++)
    {
        if(mapflag[c]==0) continue;
		if((newmap = tracemalloc("convert_map_to_palette", PAL_BYTES)) == NULL)
		{
			shutdown(1, "Error convert_map_to_palette for model: %s\n", model->name);
		}
        // Create new colour map
        memcpy(newmap, model->palette, PAL_BYTES);
        oldmap = model->colourmap[c];
        for(i=0; i<MAX_PAL_SIZE/4; i++)
		{
            if(oldmap[i]==i) continue;
            p1 = newmap + i*pb;
            p2 = model->palette + oldmap[i]*pb;
            memcpy(p1, p2, pb);
        }
        model->colourmap[c] = newmap;
		tracefree(oldmap); oldmap = NULL;
    }
    return 1;
}

static int _load_palette16(unsigned char* palette, char* filename)
{
    int handle, i;
    unsigned char tp[3];
    handle = openpackfile(filename, packfile);
    if(handle <0) return 0;
    memset(palette, 0, MAX_PAL_SIZE/2);
    for(i=0; i<MAX_PAL_SIZE/4; i++)
    {
        if(readpackfile(handle, tp, 3) != 3)
        {
            closepackfile(handle);
            return 0;
        }
        ((unsigned short*)palette)[i] = colour16(tp[0], tp[1], tp[2]);
    }
    closepackfile(handle);
    *(unsigned short*)palette = 0;

    return 1;
}


static int _load_palette32(unsigned char* palette, char* filename)
{
    int handle, i;
    unsigned* dp;
    unsigned char tpal[3];
    handle = openpackfile(filename, packfile);
    if(handle <0) return 0;
    memset(palette, 0, MAX_PAL_SIZE);
    dp = (unsigned*)palette;
    for(i=0; i<MAX_PAL_SIZE/4; i++)
    {
        if(readpackfile(handle, tpal, 3) != 3)
        {
            closepackfile(handle);
            return 0;
        }
        dp[i] = colour32(tpal[0],tpal[1],tpal[2]);

    }
    closepackfile(handle);
    dp[0] = 0;

    return 1;
}

//load a 256 colors' palette
int load_palette(unsigned char* palette, char* filename)
{
    int handle;
    if(screenformat==PIXEL_32)
        return _load_palette32(palette, filename);
    else if(screenformat==PIXEL_16)
        return _load_palette16(palette, filename);

    handle = openpackfile(filename, packfile);
    if(handle <0) return 0;
    if(readpackfile(handle, palette, 768) != 768){
        closepackfile(handle);
        return 0;
    }
    closepackfile(handle);
    palette[0] = palette[1] = palette[2] = 0;

    return 1;
}

// create blending tables for the palette
int create_blending_tables(unsigned char* palette, unsigned char* tables[], int usemap[])
{
    int i;
    if(pixelformat!=PIXEL_8) return 1;
    if(!palette || !tables) return 0;

    memset(tables, 0, MAX_BLENDINGS*sizeof(unsigned char*));
    for(i=0; i<MAX_BLENDINGS; i++)
    {
        if(!usemap || usemap[i])
        {
            tables[i] = (blending_table_functions[i])(palette);
            if(!tables[i]) return 0;
        }
    }

    return 1;
}


//change system palette by index
void change_system_palette(int palindex)
{
    if(palindex < 0) palindex = 0;
    //if(current_palette == palindex ) return;


    if(!level || palindex == 0 || palindex > level->numpalettes)
    {
        current_palette = 0;
        if(screenformat==PIXEL_8)
        {
            palette_set_corrected(pal, savedata.gamma,savedata.gamma,savedata.gamma, savedata.brightness,savedata.brightness,savedata.brightness);
            set_blendtables(blendings); // set global blending tables
        }
    }
    else if(level)
    {
        current_palette = palindex;
        if(screenformat==PIXEL_8)
        {
            palette_set_corrected(level->palettes[palindex-1], savedata.gamma,savedata.gamma,savedata.gamma, savedata.brightness,savedata.brightness,savedata.brightness);
            set_blendtables(level->blendings[palindex-1]);
        }
    }
}

// Load colour 0-127 from data/pal.act
void standard_palette(int immediate){
	unsigned char* pp[MAX_PAL_SIZE] = {0};
    if(load_palette((unsigned char*)pp, "data/pal.act"))
    {
        memcpy(pal, pp, (PAL_BYTES)/2);
    }
    if(immediate)
    {
       change_system_palette(0);
    }
}


void unload_background(){
    int i;
	if (background)	clearscreen(background);
    for(i=0; i<MAX_BLENDINGS; i++)
    {
        if(blendings[i]) tracefree(blendings[i]);
        blendings[i] = NULL;
    }
}


int _makecolour(int r, int g, int b)
{
    switch(screenformat)
    {
    case PIXEL_8:
        return palette_find(pal, r, g, b);
    case PIXEL_16:
         return colour16(r,g,b);
    case PIXEL_32:
        return colour32(r,g,b);
    }
    return 0;
}

// parses a color string in the format "R_G_B" or as a raw integer
int parsecolor(const char* string)
{
	int r, g, b;
	if(strchr(string, '_') != strrchr(string, '_'))
	{ // 2 underscores; color is in "R_G_B" format
		r = atoi(string);
		g = atoi(strchr(string, '_')+1);
		b = atoi(strrchr(string, '_')+1);
		return _makecolour(r,g,b);
	} else return atoi(string); // raw integer
}

// ltb 1-17-05   new function for lifebar colors
void lifebar_colors()
{
    char * filename = "data/lifebar.txt";
    char *buf;
    size_t size;
    int pos;

    char * command;

    if(buffer_pakfile(filename, &buf, &size)!=1)
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
        shadowalpha = BLEND_MULTIPLY+1;
        return;
    }

    pos = 0;
    colorbars=1;
    while(pos<size){
        command = findarg(buf+pos, 0);
        if(command[0])
        {
            if(stricmp(command, "blackbox")==0)
                color_black = _makecolour(atoi(findarg(buf+pos, 1)), atoi(findarg(buf+pos, 2)), atoi(findarg(buf+pos, 3)));
            else if(stricmp(command, "whitebox")==0)
                color_white = _makecolour(atoi(findarg(buf+pos, 1)), atoi(findarg(buf+pos, 2)), atoi(findarg(buf+pos, 3)));
            else if(stricmp(command, "color300")==0)
                color_orange = _makecolour(atoi(findarg(buf+pos, 1)), atoi(findarg(buf+pos, 2)), atoi(findarg(buf+pos, 3)));
            else if(stricmp(command, "color25")==0)
                color_red = _makecolour(atoi(findarg(buf+pos, 1)), atoi(findarg(buf+pos, 2)), atoi(findarg(buf+pos, 3)));
            else if(stricmp(command, "color50")==0)
                color_yellow = _makecolour(atoi(findarg(buf+pos, 1)), atoi(findarg(buf+pos, 2)), atoi(findarg(buf+pos, 3)));
            else if(stricmp(command, "color100")==0)
                color_green = _makecolour(atoi(findarg(buf+pos, 1)), atoi(findarg(buf+pos, 2)), atoi(findarg(buf+pos, 3)));
            else if(stricmp(command, "color200")==0)
                color_blue = _makecolour(atoi(findarg(buf+pos, 1)), atoi(findarg(buf+pos, 2)), atoi(findarg(buf+pos, 3)));
            else if(stricmp(command, "color400")==0)
                color_pink = _makecolour(atoi(findarg(buf+pos, 1)), atoi(findarg(buf+pos, 2)), atoi(findarg(buf+pos, 3)));
            else if(stricmp(command, "color500")==0)
                color_purple = _makecolour(atoi(findarg(buf+pos, 1)), atoi(findarg(buf+pos, 2)), atoi(findarg(buf+pos, 3)));
            //magic bars color declarations by tails
            else if(stricmp(command, "colormagic")==0)
                color_magic = _makecolour(atoi(findarg(buf+pos, 1)), atoi(findarg(buf+pos, 2)), atoi(findarg(buf+pos, 3)));
            else if(stricmp(command, "colormagic2")==0)
                color_magic2 = _makecolour(atoi(findarg(buf+pos, 1)), atoi(findarg(buf+pos, 2)), atoi(findarg(buf+pos, 3)));
            //end of magic bars color declarations by tails
            else if(stricmp(command, "shadowcolor")==0)
                shadowcolor = _makecolour(atoi(findarg(buf+pos, 1)), atoi(findarg(buf+pos, 2)), atoi(findarg(buf+pos, 3)));
            else if(stricmp(command, "shadowalpha")==0) //gfxshadow alpha
                shadowalpha = atoi(findarg(buf+pos, 1));
            else printf("Warning: Unknown command in lifebar.txt: '%s'.\n", command);
        }

        // Go to next line
        while(buf[pos] && buf[pos]!='\n' && buf[pos]!='\r') ++pos;
        while(buf[pos]=='\n' || buf[pos]=='\r') ++pos;
    }
    if(buf != NULL){
        tracefree(buf);
        buf = NULL;
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

	memcpy(ldcolourtable, hpcolourtable, 11*sizeof(int));
}

void load_background(char *filename, int createtables)
{
    //if(pixelformat!=PIXEL_8) createtables = 0;
    unload_background();

    if(pixelformat==PIXEL_8)
	{
		if(!loadscreen(filename, packfile, pal, PIXEL_8, &background))
		{
			shutdown(1, "Error loading background (PIXEL_8) file '%s'", filename);
		}
	}
    else if(pixelformat==PIXEL_x8)
    {
		if(!loadscreen(filename, packfile, NULL, PIXEL_x8, &background))
		{
			shutdown(1, "Error loading background (PIXEL_x8) file '%s'", filename);
		}
        memcpy(pal, background->palette, PAL_BYTES);
    }
	else
	{
		shutdown(1, "Error loading background, Unknown Pixel Format!\n");
	}

	if(createtables)
	{
		standard_palette(0);
		if(!create_blending_tables(pal, blendings, blendfx))
			shutdown(1, "Failed to create colour conversion tables! (Out of memory?)");
	}

	lifebar_colors();
	if(!color_black)  color_black = _makecolour(0,0,0);           // black boxes 500-600HP
	if(!color_red)    color_red = _makecolour(255,0,0);           // 1% - 25% Full Health
	if(!color_orange) color_orange = _makecolour(255,150,0);      // 200-300HP
	if(!color_yellow) color_yellow = _makecolour(0xF8,0xB8,0x40); // 26%-50% Full health
	if(!color_white)  color_white = _makecolour(255,255,255);     // white boxes 600+ HP
	if(!color_blue)   color_blue = _makecolour(0,0,255);          // 100-200 HP
	if(!color_green)  color_green = _makecolour(0,255,0);         // 51% - 100% full health
	if(!color_pink)   color_pink = _makecolour(255,0,255);        // 300-400HP
	if(!color_purple) color_purple = _makecolour(128,48,208);     // transbox 400-500HP
	if(!color_magic)  color_magic = _makecolour(98,180,255);      // 1st magic bar color by tails
	if(!color_magic2) color_magic2 = _makecolour(24,48,143);      // 2sec magic bar color by tails
	if(!shadowcolor)  shadowcolor =  _makecolour(64,64,64);
	init_colourtable();

    video_clearscreen();
    pal[0] = pal[1] = pal[2] = 0;
    //palette_set_corrected(pal, savedata.gamma,savedata.gamma,savedata.gamma, savedata.brightness,savedata.brightness,savedata.brightness);
    change_system_palette(0);
}

void load_cached_background(char *filename, int createtables)
{
#if !WII
	load_background(filename, createtables);
#else
	int index = -1;
	unload_background();

	if(strcmp(filename, "data/bgs/logo")==0)
		index = 0;
	else if(strcmp(filename, "data/bgs/title")==0)
		index = 1;
	else if(strcmp(filename, "data/bgs/titleb")==0)
		index = 2;
	else if(strcmp(filename, "data/bgs/loading")==0)
		index = 3;
	else if(strcmp(filename, "data/bgs/loading2")==0)
		index = 4;
	else if(strcmp(filename, "data/bgs/hiscore")==0)
		index = 5;
	else if(strcmp(filename, "data/bgs/complete")==0)
		index = 6;
	else if(strcmp(filename, "data/bgs/unlockbg")==0)
		index = 7;
	else if(strcmp(filename, "data/bgs/select")==0)
		index = 8;

	if((index==-1) || (bg_cache[index]==NULL))
		shutdown(1, "Error: can't load cached background '%s'", filename);

	if(background) freescreen(&background);
	background = allocscreen(videomodes.hRes, videomodes.vRes, pixelformat);
	copyscreen(background, bg_cache[index]);

	if(pixelformat==PIXEL_8)
		memcpy(pal, bg_palette_cache[index], PAL_BYTES);
	else if(pixelformat==PIXEL_x8)
	{
		memcpy(background->palette, bg_cache[index]->palette, PAL_BYTES);
		memcpy(pal, background->palette, PAL_BYTES);
	}


	if(createtables)
	{
		standard_palette(0);
		if(!create_blending_tables(pal, blendings, blendfx))
			shutdown(1, "Failed to create colour conversion tables! (Out of memory?)");
	}

	video_clearscreen();
	pal[0] = pal[1] = pal[2] = 0;
	//palette_set_corrected(pal, savedata.gamma,savedata.gamma,savedata.gamma, savedata.brightness,savedata.brightness,savedata.brightness);
	change_system_palette(0);
#endif
}

#if WII
void cache_background(char *filename)
{
	s_screen *bg = allocscreen(videomodes.hRes, videomodes.vRes, pixelformat);
	int index = -1;

	if(pixelformat==PIXEL_8)
	{
		if(!loadscreen(filename, packfile, pal, pixelformat, &bg))
		{
			freescreen(&bg);
			bg = NULL;
		}
	}
    else if(pixelformat==PIXEL_x8)
    {
		if(!loadscreen(filename, packfile, NULL, pixelformat, &bg))
		{
			freescreen(&bg);
			bg = NULL;
		}
    }
	else
	{
		shutdown(1, "Error caching background, Unknown Pixel Format!\n");
	}

	if(strcmp(filename, "data/bgs/logo")==0)
		index = 0;
	else if(strcmp(filename, "data/bgs/title")==0)
		index = 1;
	else if(strcmp(filename, "data/bgs/titleb")==0)
		index = 2;
	else if(strcmp(filename, "data/bgs/loading")==0)
		index = 3;
	else if(strcmp(filename, "data/bgs/loading2")==0)
		index = 4;
	else if(strcmp(filename, "data/bgs/hiscore")==0)
		index = 5;
	else if(strcmp(filename, "data/bgs/complete")==0)
		index = 6;
	else if(strcmp(filename, "data/bgs/unlockbg")==0)
		index = 7;
	else if(strcmp(filename, "data/bgs/select")==0)
		index = 8;
	else shutdown(1, "Error: unknown cached background '%s'", filename);

	bg_cache[index] = bg;

	if(pixelformat==PIXEL_8)
		memcpy(bg_palette_cache[index], pal, PAL_BYTES);

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

void load_bglayer(char *filename, int index)
{
    if(!level) return;

    // use screen for water effect for now, maybe get it to work with sprites at some point
    if(level->bglayers[index].watermode)
    {
        if(loadscreen(filename, packfile, NULL, pixelformat, &level->bglayers[index].screen))
        {
            level->bglayers[index].height = level->bglayers[index].screen->height;
            level->bglayers[index].width = level->bglayers[index].screen->width;
            level->bglayers[index].type = bg_screen;
        }
    }
    else if(level->bglayers[index].alpha>0 || level->bglayers[index].transparency)
    {
    // assume sprites are faster than screen when transparency or alpha are specified
        level->bglayers[index].sprite = loadsprite2(filename, &(level->bglayers[index].width),&(level->bglayers[index].height));
        level->bglayers[index].type = bg_sprite;
    }
    else
    {
    // otherwise, a screen should be fine, especially in 8bit mode, it is super fast,
    //            or, at least it is not slower than a sprite
        if(loadscreen(filename, packfile, NULL, pixelformat, &level->bglayers[index].screen))
        {
            level->bglayers[index].height = level->bglayers[index].screen->height;
            level->bglayers[index].width = level->bglayers[index].screen->width;
            level->bglayers[index].type = bg_screen;
        }
    }

    if(level->bglayers[index].handle ==NULL) shutdown(1, "Error loading file '%s'", filename);

}

void load_fglayer(char *filename, int index)
{
    if(!level) return;

    if(level->fglayers[index].alpha>0 || level->fglayers[index].transparency)
    {
    // assume sprites are faster than screen when transparency or alpha are specified
        level->fglayers[index].sprite = loadsprite2(filename, &(level->fglayers[index].width),&(level->fglayers[index].height));
        level->fglayers[index].type = fg_sprite;
    }
    else
    {
    // otherwise, a screen should be fine, especially in 8bit mode, it is super fast,
    //            or, at least it is not slower than a sprite
        if(loadscreen(filename, packfile, NULL, pixelformat, &level->fglayers[index].screen))
        {
            level->fglayers[index].height = level->fglayers[index].screen->height;
            level->fglayers[index].width = level->fglayers[index].screen->width;
            level->fglayers[index].type = fg_screen;
        }
    }

    if(level->fglayers[index].handle ==NULL) shutdown(1, "Error loading file '%s'", filename);

}

void unload_texture(){
    if(texture) freebitmap(texture);
    texture = NULL;
}



void load_texture(char *filename){
    unload_texture();
    texture = loadbitmap(filename, packfile, pixelformat);
    if(texture==NULL) shutdown(1, "Error loading file '%s'", filename);
}



void freepanels(){
    int i;
    for(i=0; i<MAX_PANELS; i++){
        if(panels[i].sprite_normal != NULL){
            tracefree(panels[i].sprite_normal);
            panels[i].sprite_normal = NULL;
        }
        if(panels[i].sprite_neon != NULL){
            tracefree(panels[i].sprite_neon);
            panels[i].sprite_neon = NULL;
        }
        if(panels[i].sprite_screen != NULL){
            tracefree(panels[i].sprite_screen);
            panels[i].sprite_screen = NULL;
        }
        if(frontpanels[i] != NULL){
            tracefree(frontpanels[i]);
            frontpanels[i] = NULL;
        }
    }
    panels_loaded = 0;
    frontpanels_loaded = 0;
    panel_width = 0;
    panel_height = 0;
}

s_sprite * loadsprite2(char *filename, int* width, int* height)
{
    size_t size;
    s_bitmap *bitmap = NULL;
    s_sprite *sprite = NULL;
    int clipl, clipr, clipt, clipb;

    bitmap = loadbitmap(filename, packfile, pixelformat);
    if(!bitmap) return NULL;
    if(width) *width = bitmap->width;
    if(height) *height = bitmap->height;
    clipbitmap(bitmap, &clipl, &clipr, &clipt, &clipb);
    size = fakey_encodesprite(bitmap);
    sprite = (s_sprite*)tracemalloc("loadsprite2",size);
    if(!sprite){
        freebitmap(bitmap);
        return NULL;
    }
    encodesprite(-clipl, -clipt, bitmap, sprite);
    freebitmap(bitmap);

    return sprite;
}



s_sprite * loadpanel2(char *filename){
    s_sprite *sprite;
    int w, h;

    if(NULL==(sprite=loadsprite2(filename, &w, &h)))
        return NULL;

    if(w > panel_width) panel_width = w;
    if(h > panel_height) panel_height = h;

    return sprite;
}



int loadpanel(char *filename_normal, char *filename_neon, char *filename_screen){

    int i = 0;

    if(panels_loaded >= MAX_PANELS) return 0;

    if(stricmp(filename_normal,"none")!=0 && *filename_normal){
        panels[panels_loaded].sprite_normal = loadpanel2(filename_normal);
        if(panels[panels_loaded].sprite_normal == NULL) return 0;
        i++;
    }
    if(stricmp(filename_neon,"none")!=0 && *filename_neon){
        panels[panels_loaded].sprite_neon = loadpanel2(filename_neon);
        if(panels[panels_loaded].sprite_neon == NULL) return 0;
        if(panels[panels_loaded].sprite_neon->palette) // under 24bit mode, copy the palette
            memcpy(neontable, panels[panels_loaded].sprite_neon->palette, PAL_BYTES);
        i++;
    }
    if(stricmp(filename_screen,"none")!=0 && *filename_screen){
        panels[panels_loaded].sprite_screen = loadpanel2(filename_screen);
        if(panels[panels_loaded].sprite_screen == NULL) return 0;
        else if(blendfx_is_set==0) blendfx[BLEND_SCREEN] = 1;
        i++;
    }
    if(i<1) return 0;    // Nothing was loaded!

    ++panels_loaded;

    return 1;
}



int loadfrontpanel(char *filename){

    size_t size;
    s_bitmap *bitmap = NULL;
    int clipl, clipr, clipt, clipb;


    if(frontpanels_loaded >= MAX_PANELS) return 0;
    bitmap = loadbitmap(filename, packfile, pixelformat);
    if(!bitmap) return 0;

    clipbitmap(bitmap, &clipl, &clipr, &clipt, &clipb);

    size = fakey_encodesprite(bitmap);
    frontpanels[frontpanels_loaded] = (s_sprite*)tracemalloc("loadfrontpanel",size);
    if(!frontpanels[frontpanels_loaded]){
        freebitmap(bitmap);
        return 0;
    }
    encodesprite(-clipl, -clipt, bitmap, frontpanels[frontpanels_loaded]);

    freebitmap(bitmap);
    ++frontpanels_loaded;

    return 1;
}

// Added to conserve memory
void resourceCleanUp(){
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
	unsigned short i;
	s_sprite_list *head;
	for(i=0; i<=sprites_loaded; i++)
	{
		if(sprite_list != NULL)
		{
			tracefree(sprite_list->sprite);
			sprite_list->sprite = NULL;
			tracefree(sprite_list->filename);
			sprite_list->filename = NULL;
			head = sprite_list->next;
			tracefree(sprite_list);
			sprite_list = head;
		}
	}
	if(sprite_map != NULL)
	{
		tracefree(sprite_map);
		sprite_map = NULL;
	}
	sprites_loaded = 0;
}

// allocate enough members for sprite_map
void add_sprite_map(size_t size)
{
	s_sprite_map *copy;
	if(sprite_map == NULL)
	{
		sprite_map = tracemalloc("sprite_map", sizeof(s_sprite_map));
	}
	else
	{
		// We Create Copy then append new map to list.
		copy = tracemalloc("copy_sprite_map", (size + 1) * sizeof(s_sprite_map));
		if(copy == NULL) shutdown(1, "Out Of Memory!  Failed to create a copy of sprite_map\n");
		memcpy(copy, sprite_map, (size + 1) * sizeof(s_sprite_map));
		tracefree(sprite_map);
		sprite_map = tracemalloc("add_sprite_map",(size + 1) *  (sizeof(s_sprite_map) + sizeof(s_sprite_map)));
		if(sprite_map == NULL) shutdown(1, "Out Of Memory!  Failed to create a new sprite_map\n");
		memcpy(sprite_map, copy, (size + 1) * sizeof(s_sprite_map));
		tracefree(copy);
		copy = NULL;
	}
}

// Returns sprite index.
// Does not return on error, as it would shut the program down.
// UT:
// bmpformat - In 24bit mode, a sprite can have a 24bit palette(e.g., panel),
//             so add this paramter to let sprite encoding function know.
//             Actually the sprite pixel encoding method is the same, but a
//             24bit palettte sprite should have a palette allocated at the end of
//             pixel data, and the information is carried by the bitmap paramter.
int loadsprite(char *filename, int ofsx, int ofsy, int bmpformat)
{
	ptrdiff_t i, size, len;
	s_bitmap *bitmap = NULL;
	int clipl, clipr, clipt, clipb;
	s_sprite_list *curr = NULL, *head = NULL;

	for(i=0; i<sprites_loaded; i++) {
		if(sprite_map != NULL) {
			if(stricmp(sprite_map[i].filename, filename) == 0) {
				if(sprite_map[i].ofsx == ofsx && sprite_map[i].ofsy == ofsy) return i;
				else {
					bitmap = loadbitmap(filename, packfile, bmpformat);
					if(bitmap == NULL) shutdown(1, "Unable to load file '%s'\n", filename);
					clipbitmap(bitmap, &clipl, &clipr, &clipt, &clipb);
					add_sprite_map(sprites_loaded);
					sprite_map[sprites_loaded].filename = sprite_map[i].filename;
					sprite_map[sprites_loaded].sprite = sprite_map[i].sprite;
					sprite_map[sprites_loaded].ofsx = ofsx;
					sprite_map[sprites_loaded].ofsy = ofsy;
					sprite_map[sprites_loaded].centerx = ofsx-clipl;
					sprite_map[sprites_loaded].centery = ofsy-clipt;
					freebitmap(bitmap);
					++sprites_loaded;
					return sprites_loaded-1;
				}
			}
		}
	}

	bitmap = loadbitmap(filename, packfile, bmpformat);
	if(bitmap == NULL) shutdown(1, "Unable to load file '%s'\n", filename);

	clipbitmap(bitmap, &clipl, &clipr, &clipt, &clipb);

	len = strlen(filename);
	size = fakey_encodesprite(bitmap);
	curr = tracemalloc("sprite_list", sizeof(s_sprite_list));
	curr->sprite = tracemalloc("loadsprite 1", size);
	curr->filename = tracemalloc("sm_fn", len + 1);
	if(curr == NULL || curr->sprite == NULL || curr->filename == NULL){
		freebitmap(bitmap);
		shutdown(1, "loadsprite() Out of memory!\n");
	}
	memcpy(curr->filename, filename,len);
	curr->filename[len] = 0;
	encodesprite(ofsx-clipl, ofsy-clipt, bitmap, curr->sprite);
	if(sprite_list == NULL){
		sprite_list = curr;
		sprite_list->next = NULL;
	}
	else{
		head = sprite_list;
		sprite_list = curr;
		sprite_list->next = head;
	}
	add_sprite_map(sprites_loaded);
	sprite_map[sprites_loaded].filename = sprite_list->filename;
	sprite_map[sprites_loaded].sprite = sprite_list->sprite;
	sprite_map[sprites_loaded].ofsx = ofsx;
	sprite_map[sprites_loaded].ofsy = ofsy;
	sprite_map[sprites_loaded].centerx = ofsx-clipl;
	sprite_map[sprites_loaded].centery = ofsy-clipt;
	freebitmap(bitmap);
	++sprites_loaded;
	return sprites_loaded-1;
}

void load_special_sprites()
{
	memset(shadowsprites, -1, sizeof(shadowsprites[0])*6);
	golsprite = gosprite = -1;
	if(testpackfile("data/sprites/shadow1.gif", packfile) >=0) shadowsprites[0] = loadsprite("data/sprites/shadow1",9,3,pixelformat);
	if(testpackfile("data/sprites/shadow2.gif", packfile) >=0) shadowsprites[1] = loadsprite("data/sprites/shadow2",14,5,pixelformat);
	if(testpackfile("data/sprites/shadow3.gif", packfile) >=0) shadowsprites[2] = loadsprite("data/sprites/shadow3",19,6,pixelformat);
	if(testpackfile("data/sprites/shadow4.gif", packfile) >=0) shadowsprites[3] = loadsprite("data/sprites/shadow4",24,8,pixelformat);
	if(testpackfile("data/sprites/shadow5.gif", packfile) >=0) shadowsprites[4] = loadsprite("data/sprites/shadow5",29,9,pixelformat);
	if(testpackfile("data/sprites/shadow6.gif", packfile) >=0) shadowsprites[5] = loadsprite("data/sprites/shadow6",34,11,pixelformat);
	if(testpackfile("data/sprites/arrow.gif", packfile) >=0) gosprite  = loadsprite("data/sprites/arrow",35,23,pixelformat);
	if(testpackfile("data/sprites/arrowl.gif", packfile) >=0) golsprite = loadsprite("data/sprites/arrowl",35,23,pixelformat);
	if(timeicon_path[0]) timeicon = loadsprite(timeicon_path,0,0,pixelformat);
	if(bgicon_path[0]) bgicon = loadsprite(bgicon_path,0,0,pixelformat);
	if(olicon_path[0]) olicon = loadsprite(olicon_path,0,0,pixelformat);
}

void unload_all_fonts()
{
	int i;
	for(i=0; i<8; i++)
	{
		font_unload(i);
	}
}

void load_all_fonts()
{
	if(!font_load(0, "data/sprites/font", packfile, fontmonospace[0])) shutdown(1, "Unable to load font #1!\n");
	if(!font_load(1, "data/sprites/font2", packfile, fontmonospace[1])) shutdown(1, "Unable to load font #2!\n");
	if(!font_load(2, "data/sprites/font3", packfile, fontmonospace[2])) shutdown(1, "Unable to load font #3!\n");
	if(!font_load(3, "data/sprites/font4", packfile, fontmonospace[3])) shutdown(1, "Unable to load font #4!\n");
	if(testpackfile("data/sprites/font5.gif", packfile) >=0) font_load(4, "data/sprites/font5", packfile, fontmonospace[4]);
	if(testpackfile("data/sprites/font6.gif", packfile) >=0) font_load(5, "data/sprites/font6", packfile, fontmonospace[5]);
	if(testpackfile("data/sprites/font7.gif", packfile) >=0) font_load(6, "data/sprites/font7", packfile, fontmonospace[6]);
	if(testpackfile("data/sprites/font8.gif", packfile) >=0) font_load(7, "data/sprites/font8", packfile, fontmonospace[7]);
}

void load_menu_txt()
{
    char * filename = "data/menu.txt";
    int pos;
    char *buf, *command;
    size_t size;

    // Read file
    if(buffer_pakfile(filename, &buf, &size)!=1)
    {
		return;
    }

    // Now interpret the contents of buf line by line
    pos = 0;
    while(pos<size){
        command = findarg(buf+pos, 0);
        if(command[0]){
            if(stricmp(command, "disablekey")==0){
                // here to keep from crashing
            }
            else if(stricmp(command, "renamekey")==0){
                // here to keep from crashing
            }
            else if(stricmp(command, "fontmonospace")==0)
            {
                fontmonospace[0] = atoi(findarg(buf+pos, 1));
                fontmonospace[1] = atoi(findarg(buf+pos, 2));
                fontmonospace[2] = atoi(findarg(buf+pos, 3));
                fontmonospace[3] = atoi(findarg(buf+pos, 4));
                fontmonospace[4] = atoi(findarg(buf+pos, 5));
                fontmonospace[5] = atoi(findarg(buf+pos, 6));
                fontmonospace[6] = atoi(findarg(buf+pos, 7));
                fontmonospace[7] = atoi(findarg(buf+pos, 8));
            }
            else{
                if(buf != NULL){
                    tracefree(buf);
                    buf = NULL;
                }
                shutdown(1, "Command '%s' not understood in file '%s'!", command, filename);
            }
        }

        // Go to next line
        while(buf[pos] && buf[pos]!='\n' && buf[pos]!='\r') ++pos;
        while(buf[pos]=='\n' || buf[pos]=='\r') ++pos;
    }

    if(buf != NULL){
        tracefree(buf);
        buf = NULL;
    }
}

int load_special_sounds()
{
	sound_unload_all_samples();
    SAMPLE_GO		= sound_load_sample("data/sounds/go.wav",		packfile,	0);
    SAMPLE_BEAT		= sound_load_sample("data/sounds/beat1.wav",	packfile,	0);
    SAMPLE_BLOCK	= sound_load_sample("data/sounds/block.wav",	packfile,	0);
    SAMPLE_FALL		= sound_load_sample("data/sounds/fall.wav",		packfile,	0);
    SAMPLE_GET		= sound_load_sample("data/sounds/get.wav",		packfile,	0);
    SAMPLE_GET2		= sound_load_sample("data/sounds/money.wav",	packfile,	0);
    SAMPLE_JUMP		= sound_load_sample("data/sounds/jump.wav",		packfile,	0);
    SAMPLE_INDIRECT	= sound_load_sample("data/sounds/indirect.wav",	packfile,	0);
    SAMPLE_PUNCH	= sound_load_sample("data/sounds/punch.wav",	packfile,	0);
    SAMPLE_1UP		= sound_load_sample("data/sounds/1up.wav",		packfile,	0);
    SAMPLE_TIMEOVER = sound_load_sample("data/sounds/timeover.wav", packfile,	0);
    SAMPLE_BEEP		= sound_load_sample("data/sounds/beep.wav",		packfile,	0);
    SAMPLE_BEEP2	= sound_load_sample("data/sounds/beep2.wav",	packfile,	0);
    SAMPLE_BIKE		= sound_load_sample("data/sounds/bike.wav",		packfile,	0);
	if(SAMPLE_GO < 0 || SAMPLE_BEAT < 0 || SAMPLE_BLOCK < 0 ||
	   SAMPLE_FALL < 0 || SAMPLE_GET < 0 || SAMPLE_GET2 < 0 ||
	   SAMPLE_JUMP < 0 || SAMPLE_INDIRECT < 0 || SAMPLE_PUNCH < 0 ||
       SAMPLE_1UP < 0 || SAMPLE_TIMEOVER < 0 || SAMPLE_BEEP < 0 ||
	   SAMPLE_BEEP2 < 0 || SAMPLE_BIKE < 0) return 0;
	return 1;
}

// UT:
// now the model list should be arranged first, if you find something wrong that means the model is not loaded through load_cached_model
s_model * find_model(char *name)
{
    int result, first, last, mid;

    if(models_loaded==0) return NULL;

    first = 0;
    last = models_loaded-1;
    while(first<last)
    {
        mid = (first+last)>>1;
        result = stricmp(model_map[mid].model->name, name);
        if(result==0) return model_map[mid].model; // find here
        if(result>0) last = mid-1;
        else first = mid+1;
    }
    return (stricmp(model_map[first].model->name, name)==0)?model_map[first].model:NULL;
}

// Use by player select menus
s_model * nextplayermodel(s_model *current){
    int i;
    int curindex = -1;
    int loops;
    if(current){
        // Find index of current player model
        for(i=0; i<models_cached; i++){
            if(model_cache[i].model == current){
                curindex = i;
                break;
            }
        }
    }
    // Find next player model (first one after current index)
    for(i=curindex+1, loops=0; loops<models_cached; i++, loops++){
		if(i >= models_cached) i = 0;
		if(model_cache[i].model && model_cache[i].model->type==TYPE_PLAYER &&
		  (allow_secret_chars || !model_cache[i].model->secret) && model_cache[i].selectable){
			return model_cache[i].model;
		}
    }
    shutdown(1, "Fatal: can't find any player models!");
    return NULL;
}

// Use by player select menus
s_model * prevplayermodel(s_model *current){
    int i;
    int curindex = -1;
    int loops;
    if(current){
        // Find index of current player model
        for(i=0; i<models_cached; i++){
            if(model_cache[i].model == current){
                curindex = i;
                break;
            }
        }
    }
    // Find next player model (first one after current index)
	for(i=curindex-1, loops=0; loops<models_cached; i--, loops++){
		if(i < 0) i = models_cached-1;
		if(model_cache[i].model && model_cache[i].model->type==TYPE_PLAYER &&
		  (allow_secret_chars || !model_cache[i].model->secret) && model_cache[i].selectable){
			return model_cache[i].model;
		}
	}
    shutdown(1, "Fatal: can't find any player models!");
    return NULL;
}

// Reset All Player Models to on/off for Select Screen.
static void reset_playable_list(char which)
{
	int i;
	for(i=0;i<models_cached;i++)
	{
		if(model_cache[i].model && model_cache[i].model->type == TYPE_PLAYER)
		{
			model_cache[i].selectable = which;
		}
	}
}

// Specify which Player Models are allowable for selecting
static void load_playable_list(char* buf)
{
	int i, index;
	char* value;
	s_model *playermodels = NULL;
	reset_playable_list(0);
	for(i=1;(value=findarg(buf, i))[0];i++)
	{
		playermodels = find_model(value);
		if(playermodels == NULL) shutdown(1, "Player model '%s' is not loaded.\n", value);
		index = get_cached_model_index(playermodels->name);
		if(index == -1) shutdown(1, "Player model '%s' is not cached.\n", value);
		model_cache[index].selectable = 1;
	}
}

void alloc_frames(s_anim * anim, int fcount)
{
    anim->sprite = tracemalloc("anim->sprite", fcount * sizeof(anim->sprite));
	anim->delay = tracemalloc("anim->delay", fcount * sizeof(anim->delay));
	anim->vulnerable = tracemalloc("anim->vulnerable", fcount * sizeof(anim->vulnerable));
	memset(anim->sprite, 0, fcount*sizeof(anim->sprite));
	memset(anim->delay, 0, fcount*sizeof(anim->delay));
	memset(anim->vulnerable, 0, fcount*sizeof(anim->vulnerable));
}

void free_frames(s_anim * anim)
{
    int i;
	if(anim->idle)			{tracefree(anim->idle);			 anim->idle = NULL;}
	if(anim->seta)          {tracefree(anim->seta);          anim->seta = NULL;}
	if(anim->move)          {tracefree(anim->move);          anim->move = NULL;}
	if(anim->movez)         {tracefree(anim->movez);         anim->movez = NULL;}
    if(anim->movea)         {tracefree(anim->movea);         anim->movea = NULL;}
	if(anim->delay)         {tracefree(anim->delay);         anim->delay = NULL;}
	if(anim->sprite)        {tracefree(anim->sprite);        anim->sprite = NULL;}
	if(anim->platform)      {tracefree(anim->platform);      anim->platform = NULL;}
	if(anim->vulnerable)    {tracefree(anim->vulnerable);    anim->vulnerable = NULL;}
    if(anim->bbox_coords)   {tracefree(anim->bbox_coords);   anim->bbox_coords = NULL;}
    if(anim->shadow)        {tracefree(anim->shadow);        anim->shadow = NULL;}
    if(anim->shadow_coords) {tracefree(anim->shadow_coords); anim->shadow_coords = NULL;}
	if(anim->soundtoplay)   {tracefree(anim->soundtoplay);   anim->soundtoplay = NULL;}
	if(anim->attacks)
    {
        for(i=0; i<anim->numframes; i++)
        {
            if(anim->attacks[i])
            {
                tracefree(anim->attacks[i]);
                anim->attacks[i] = NULL;
            }
        }
        tracefree(anim->attacks);
        anim->attacks = NULL;
    }
	if(anim->drawmethods)
    {
        for(i=0; i<anim->numframes; i++)
        {
            if(anim->drawmethods[i])
            {
                tracefree(anim->drawmethods[i]);
                anim->drawmethods[i] = NULL;
            }
        }
        tracefree(anim->drawmethods);
        anim->drawmethods = NULL;
    }
}

s_anim_list *anim_list_delete(s_anim_list *list, int index)
{
	if(list == NULL) return NULL;
	if(list->anim->model_index == index)
	{
		s_anim_list *next;
		next = list->next;
        free_anim(list->anim);
		tracefree(list);
		return next;
	}
	list->next = anim_list_delete(list->next, index);
	return list;
}

void free_anim(s_anim * anim)
{
	if(!anim) return;
	free_frames(anim);
	if(anim->weaponframe)
	{
        tracefree(anim->weaponframe);
        anim->weaponframe = NULL;
    }
	if(anim->spawnframe)
	{
        tracefree(anim->spawnframe);
        anim->spawnframe = NULL;
    }
	if(anim->summonframe)
	{
        tracefree(anim->summonframe);
        anim->summonframe = NULL;
    }
    if(anim){
        tracefree(anim);
        anim = NULL;
    }
}

s_model_list *model_list_delete(s_model_list *list, s_model *value)
{
	if(list == NULL) return NULL;
	if(list->model == value)
	{
		s_model_list *next;
		next = list->next;
		tracefree(list);
		return next;
	}
	list->next = model_list_delete(list->next, value);
	return list;
}

void model_map_sort()
{
	int i, j;
	s_model_map temp;
	if(models_loaded<2) return;
	for(j=models_loaded-1; j>0; j--)
	{
		for(i=0; i<j; i++)
		{
			if(stricmp(model_map[i].model->name, model_map[i+1].model->name)>0)
			{
				temp = model_map[i];
				model_map[i] = model_map[i+1];
				model_map[i+1] = temp;
			}
		}
	}
}

s_model_map *model_map_delete(s_model_map *map, size_t size)
{
	s_model_map *copy;
	if(map == NULL) return NULL;
	copy = tracemalloc("copy_model_map", size * sizeof(s_model_map));
	if(copy == NULL) shutdown(1, "Out Of Memory!  Failed to create a copy of model_map\n");
	memcpy(copy, map, size * sizeof(s_model_map));
	tracefree(map);
	return copy;
}

void model_map_move(int offset)
{
	s_model_map mid, end;
	mid = model_map[offset];
	end = model_map[models_loaded-1];
	model_map[offset] = end;
	model_map[models_loaded-1] = mid;
	model_map = model_map_delete(model_map, --models_loaded);
}

// Unload single model from memory
int free_model(s_model* model, int mapid)
{
	int i;
	if(model == NULL) return 0;
	printf("Unloaded '%s'\n", model->name);
	model_cache[model->index].model = NULL;
	model_map_move(mapid);
	model_list = model_list_delete(model_list, model);
	for(i=0; i<max_animations; i++) anim_list = anim_list_delete(anim_list, model->index);
	for(i=0; i<MAX_COLOUR_MAPS; i++)
	{
		if(model->colourmap[i] != NULL)
		{
			tracefree(model->colourmap[i]);
			model->colourmap[i] = NULL;
		}
	}
	if(model->palette)                     {tracefree(model->palette);                model->palette                = NULL;}
	if(model->weapon && model->ownweapons) {tracefree(model->weapon);                 model->weapon                 = NULL;}
	if(model->branch)                      {tracefree(model->branch);                 model->branch                 = NULL;}
	if(model->animation)                   {tracefree(model->animation);              model->animation              = NULL;}
	if(model->defense_factors)             {tracefree(model->defense_factors);        model->defense_factors        = NULL;}
	if(model->defense_pain)                {tracefree(model->defense_pain);           model->defense_pain           = NULL;}
	if(model->defense_knockdown)           {tracefree(model->defense_knockdown);      model->defense_knockdown      = NULL;}
	if(model->defense_blockpower)          {tracefree(model->defense_blockpower);     model->defense_blockpower     = NULL;}
	if(model->defense_blockthreshold)      {tracefree(model->defense_blockthreshold); model->defense_blockthreshold = NULL;}
	if(model->defense_blockratio)          {tracefree(model->defense_blockratio);     model->defense_blockratio     = NULL;}
	if(model->defense_blocktype)           {tracefree(model->defense_blocktype);      model->defense_blocktype      = NULL;}
	if(model->offense_factors)             {tracefree(model->offense_factors);        model->offense_factors        = NULL;}
	if(model->special)                     {tracefree(model->special);                model->special                = NULL;}
	if(model->smartbomb)                   {tracefree(model->smartbomb);              model->smartbomb              = NULL;}
	Script_Clear(model->animation_script,   2);
	Script_Clear(model->update_script,      2);
	Script_Clear(model->think_script,       2);
	Script_Clear(model->takedamage_script,  2);
	Script_Clear(model->onfall_script,      2);
	Script_Clear(model->onpain_script,      2);
	Script_Clear(model->onblocks_script,    2);
	Script_Clear(model->onblockw_script,    2);
	Script_Clear(model->onblocko_script,    2);
	Script_Clear(model->onblockz_script,    2);
	Script_Clear(model->onblocka_script,    2);
	Script_Clear(model->onmovex_script,     2);
	Script_Clear(model->onmovez_script,     2);
	Script_Clear(model->onmovea_script,     2);
	Script_Clear(model->ondeath_script,     2);
	Script_Clear(model->onkill_script,      2);
	Script_Clear(model->didblock_script,    2);
	Script_Clear(model->ondoattack_script,  2);
	Script_Clear(model->didhit_script,      2);
	Script_Clear(model->onspawn_script,     2);
	Script_Clear(model->key_script,         2);
	tracefree(model->animation_script);
	tracefree(model->update_script);
	tracefree(model->think_script);
	tracefree(model->takedamage_script);
	tracefree(model->onfall_script);
	tracefree(model->onpain_script);
	tracefree(model->onblocks_script);
	tracefree(model->onblockw_script);
	tracefree(model->onblocko_script);
	tracefree(model->onblockz_script);
	tracefree(model->onblocka_script);
	tracefree(model->onmovex_script);
	tracefree(model->onmovez_script);
	tracefree(model->onmovea_script);
	tracefree(model->ondeath_script);
	tracefree(model->onkill_script);
	tracefree(model->didblock_script);
	tracefree(model->ondoattack_script);
	tracefree(model->didhit_script);
	tracefree(model->onspawn_script);
	tracefree(model->key_script);
	tracefree(model);
	model = NULL;
	if(models_loaded == 0 && model_map != NULL)
	{
		tracefree(model_map);
		model_map = NULL;
	}
	model_map_sort();
	return models_loaded;
}

// Unload all models and animations memory
void free_models()
{
	if(!model_map) return;
	while(free_model(model_map[models_loaded-1].model, models_loaded-1));

	// free animation ids
	if(animdowns)       {tracefree(animdowns);       animdowns          = NULL;}
	if(animups)         {tracefree(animups);         animups            = NULL;}
	if(animbackwalks)   {tracefree(animbackwalks);   animbackwalks      = NULL;}
	if(animwalks)       {tracefree(animwalks);       animwalks          = NULL;}
	if(animidles)       {tracefree(animidles);       animidles          = NULL;}
	if(animspecials)    {tracefree(animspecials);    animspecials       = NULL;}
	if(animattacks)     {tracefree(animattacks);     animattacks        = NULL;}
	if(animfollows)     {tracefree(animfollows);     animfollows        = NULL;}
	if(animpains)       {tracefree(animpains);       animpains          = NULL;}
	if(animfalls)       {tracefree(animfalls);       animfalls          = NULL;}
	if(animrises)       {tracefree(animrises);       animrises          = NULL;}
	if(animriseattacks) {tracefree(animriseattacks); animriseattacks    = NULL;}
	if(animblkpains)    {tracefree(animblkpains);    animblkpains       = NULL;}
	if(animdies)        {tracefree(animdies);        animdies           = NULL;}
}


s_anim * alloc_anim()
{
	s_anim_list *curr = NULL, *head = NULL;
	curr = tracemalloc("anim_list", sizeof(s_anim_list));
	curr->anim = tracemalloc("curr->anim", sizeof(s_anim));
    if(curr == NULL || curr->anim == NULL) return NULL;
    memset(curr->anim, 0, sizeof(s_anim));
	if(anim_list == NULL){
		anim_list = curr;
		anim_list->next = NULL;
	}
	else{
		head = anim_list;
		anim_list = curr;
		anim_list->next = head;
	}
    ++anims_loaded;
    return anim_list->anim;
}


int addframe(s_anim * a, int spriteindex, int framecount, short delay, unsigned char idle,
             short *bbox, s_attack* attack, short move, short movez,
             short movea, short seta, float* platform, int frameshadow, short* shadow_coords, int soundtoplay, s_drawmethod* drawmethod)
{
    if(framecount>0) alloc_frames(a, framecount);
    else framecount = -framecount; // for alloc method, use a negative value

    a->sprite[a->numframes] = spriteindex;
    a->delay[a->numframes] = delay * GAME_SPEED / 100;

	if((bbox[2]-bbox[0]) && (bbox[3]-bbox[1]))
    {
        if(!a->bbox_coords)
        {
            a->bbox_coords = tracemalloc("a->bbox_coords", framecount * sizeof(*a->bbox_coords));
            memset(a->bbox_coords, 0, framecount * sizeof(*a->bbox_coords));
        }
        memcpy(a->bbox_coords[a->numframes], bbox, sizeof(*a->bbox_coords));
        a->vulnerable[a->numframes] = 1;
    }
    if((attack->attack_coords[2]-attack->attack_coords[0]) &&
       (attack->attack_coords[3]-attack->attack_coords[1]))
    {
        if(!a->attacks)
        {
            a->attacks = tracemalloc("a->attacks", framecount * sizeof(s_attack*));
            memset(a->attacks, 0, framecount * sizeof(s_attack*));
        }
        a->attacks[a->numframes] = tracemalloc("addframe#attack", sizeof(s_attack));
        memcpy(a->attacks[a->numframes], attack, sizeof(s_attack));
    }
    if(drawmethod->flag)
    {
        if(!a->drawmethods)
        {
            a->drawmethods = tracemalloc("a->drawmethods", framecount * sizeof(s_drawmethod*));
            memset(a->drawmethods, 0, framecount * sizeof(s_drawmethod*));
        }
        a->drawmethods[a->numframes] = tracemalloc("addframe#drawmethod", sizeof(s_drawmethod));
        memcpy(a->drawmethods[a->numframes], drawmethod, sizeof(s_drawmethod));
    }
	if(idle && !a->idle)
	{
		a->idle = tracemalloc("a->idle", framecount*sizeof(*a->idle));
		memset(a->idle, 0, framecount*sizeof(*a->idle));
	}
	if(a->idle) a->idle[a->numframes] = idle;
    if(move && !a->move)
    {
        a->move = tracemalloc("a->move", framecount * sizeof(*a->move));
        memset(a->move, 0, framecount * sizeof(*a->move));
    }
    if(a->move) a->move[a->numframes] = move;
    if(movez && !a->movez)
    {
        a->movez = tracemalloc("a->movez", framecount * sizeof(*a->movez));
        memset(a->movez, 0, framecount * sizeof(*a->movez));
    }
    if(a->movez) a->movez[a->numframes] = movez;						           // Move command for the "z" axis
    if(movea && !a->movea)
    {
        a->movea = tracemalloc("a->movea", framecount * sizeof(*a->movea));
        memset(a->movea, 0, framecount * sizeof(*a->movea));
    }
    if(a->movea) a->movea[a->numframes] = movea;						           // Move command for moving along the "a" axis
    if(seta>=0 && !a->seta)
    {
        a->seta = tracemalloc("a->seta", framecount * sizeof(*a->seta));
        memset(a->seta, -1, framecount * sizeof(*a->seta)); //default to -1
    }
    if(a->seta) a->seta[a->numframes] = seta;						               // Sets the "a" for the character on a frame/frame basis
    if(frameshadow >= 0 && !a->shadow)
    {
        a->shadow = tracemalloc("a->shadow", framecount * sizeof(*a->shadow));
        memset(a->shadow, -1, framecount * sizeof(*a->shadow)); //default to -1
    }
    if(a->shadow) a->shadow[a->numframes] = frameshadow;                          // shadow index for each frame
    if(shadow_coords[0] || shadow_coords[1])
    {
        if(!a->shadow_coords)
        {
            a->shadow_coords=tracemalloc("a->shadow_coords", framecount * sizeof(*a->shadow_coords));
            memset(a->shadow_coords, 0, framecount * sizeof(*a->shadow_coords));
        }
        memcpy(a->shadow_coords[a->numframes], shadow_coords, sizeof(*a->shadow_coords));
    }
    if(platform[7]) //height
    {
        if(!a->platform)
        {
            a->platform = tracemalloc("a->platform", framecount * sizeof(*a->platform));
            memset(a->platform, 0, framecount * sizeof(*a->platform));
        }
        memcpy(a->platform[a->numframes], platform, sizeof(*a->platform));// Used so entity can be landed on
    }
    if(soundtoplay >= 0)
    {
        if(!a->soundtoplay)
        {
            a->soundtoplay = tracemalloc("a->soundtoplay", framecount * sizeof(*a->soundtoplay));
            memset(a->soundtoplay, -1, framecount * sizeof(*a->soundtoplay)); // default to -1
        }
        a->soundtoplay[a->numframes] = soundtoplay;
    }

    ++a->numframes;
    return a->numframes;
}

void _peek_model_name(int index)
{
	size_t size = 0;
	ptrdiff_t pos = 0, len;
	char *buf = NULL;
	char *command, *value;

	if(buffer_pakfile(model_cache[index].path, &buf, &size)!=1) return;

	while(pos<size)
	{
		command = findarg(buf+pos, 0);
		if(command[0]){
			if(stricmp(command, "name")==0)
			{
				value = findarg(buf+pos, 1);
				tracefree(model_cache[index].name);
				model_cache[index].name = NULL;
				len = strlen(value);
				model_cache[index].name = tracemalloc("peek_name", len + 1);
				strcpy(model_cache[index].name, value);
				model_cache[index].name[len] = 0;
				break;
			}
		}
		while(buf[pos] && buf[pos]!='\n' && buf[pos]!='\r') ++pos;
		while(buf[pos]=='\n' || buf[pos]=='\r') ++pos;
	}

	if(buf != NULL)
	{
		tracefree(buf);
		buf = NULL;
	}
}

void add_cache_map(size_t size)
{
	s_modelcache *copy;
	if(model_cache == NULL)
	{
		model_cache = tracemalloc("sprite_map", sizeof(s_modelcache));
	}
	else
	{
		// We Create Copy then append new map to list.
		copy = tracemalloc("copy_cache_map", (size + 1) * sizeof(s_modelcache));
		if(copy == NULL) shutdown(1, "Out Of Memory!  Failed to create a copy of cache_map\n");
		memcpy(copy, model_cache, (size + 1) * sizeof(s_modelcache));
		tracefree(model_cache);
		model_cache = tracemalloc("add_cache_map", (size + 1) * (sizeof(s_modelcache) + sizeof(s_modelcache)));
		if(model_cache == NULL) shutdown(1, "Out Of Memory!  Failed to create a new cache_map\n");
		memcpy(model_cache, copy, (size + 1) * sizeof(s_modelcache));
		tracefree(copy);
		copy = NULL;
	}
}

void cache_model(char *name, char *path, int flag)
{
	int len;
	printf("Cacheing '%s'\n", name);
	add_cache_map(models_cached);
	memset(&model_cache[models_cached], 0, sizeof(s_modelcache));
	len = strlen(name);
	model_cache[models_cached].name = tracemalloc("model->name", len + 1);
	strcpy(model_cache[models_cached].name, name);
	model_cache[models_cached].name[len] = 0;
	len = strlen(path);
	model_cache[models_cached].path = tracemalloc("model->path", len + 1);
	strcpy(model_cache[models_cached].path, path);
	model_cache[models_cached].path[len] = 0;
	model_cache[models_cached].loadflag = flag;
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
			tracefree(model_cache[models_cached].name);
			model_cache[models_cached].name = NULL;
			tracefree(model_cache[models_cached].path);
			model_cache[models_cached].path = NULL;
		}
		tracefree(model_cache);
		model_cache = NULL;
	}
}


int get_cached_model_index(char * name)
{
	int i;
	for(i=0; i<models_cached; i++)
	{
		if(stricmp(name, model_cache[i].name)==0) return i;
	}
	return -1;
}

char *get_cached_model_path(char * name)
{
	int i;
	for(i=0; i<models_cached; i++)
	{
		if(stricmp(name, model_cache[i].name)==0) return model_cache[i].path;
	}
	return NULL;
}

s_model_map *model_map_add(s_model_map *map, size_t size)
{
	s_model_map *copy;
	if(map == NULL)
	{
		map = tracemalloc("model_map", sizeof(s_model_map));
		if(map == NULL) shutdown(1, "Out Of Memory!  Failed to create a initial model_map\n");
	}
	else
	{
		// We Create Copy then append new map to list.
		copy = tracemalloc("copy_model_map", (size + 1) * sizeof(s_model_map));
		if(copy == NULL) shutdown(1, "Out Of Memory!  Failed to create a copy of model_map\n");
		memcpy(copy, map, (size + 1) * sizeof(s_model_map));
		tracefree(map);
		map = tracemalloc("model_map_add", (size + 1) * (sizeof(s_model_map) + sizeof(s_model_map)));
		if(map == NULL) shutdown(1, "Out Of Memory!  Failed to create a new model_map\n");
		memcpy(map, copy, (size + 1) * sizeof(s_model_map));
		tracefree(copy);
		copy = NULL;
	}
	return map;
}

static void _readbarstatus(char*, s_barstatus*);
s_model* load_cached_model(char * name, char * owner, char unload)
{
	s_model_list *curr = NULL,
	*head = NULL;

	s_model *newchar = NULL,
	*tempmodel = NULL;

	s_anim *newanim = NULL;

	char *filename = NULL,
	*buf = NULL,
	*scriptbuf = NULL,
	*command = NULL,
	*value = NULL,
	*value2 = NULL,
	*value3 = NULL;

	char cur_name[MAX_NAME_LEN+1] = {""},
		load_name[MAX_NAME_LEN+1],
		cur_owner[256] = {""},
		namebuf[256] = {""};

	float tempFloat;

	int ani_id = -1,
		script_id = -1,
		i = 0,
		j = 0,
		tempInt = 0,
		framecount = 0,
		frameset = 0,
		peek = 0,
		cacheindex = 0,
		curframe = 0,
		delay = 0,
		weap = 0,
		last = 0,
		errorVal = 0,
		shadow_set = 0,
		idle = 0,
		move = 0,
		movez = 0,
		movea = 0,
		seta = -1,			// Used for setting custom "a". Set to -1 to distinguish between disabled and setting "a" to 0
		frameshadow = -1,	// -1 will use default shadow for this entity, otherwise will use this value
		soundtoplay = -1,
		aimoveset = 0,
		maskindex = -1;

	size_t size = 0,
		len = 0;
	ptrdiff_t pos = 0,
		index = 0;

	short bbox[5] = { 0,0,0,0,0 },
		bbox_con[5] = { 0,0,0,0,0 },
		abox[5] = { 0,0,0,0,0 },
		offset[2] = { 0,0 },
		shadow_xz[2] = {0,0},
		shadow_coords[2] = {0,0};

	float platform[8] = { 0,0,0,0,0,0,0,0 },
		platform_con[8] = { 0,0,0,0,0,0,0,0 };

	s_attack attack,
	*pattack = NULL;

	s_drawmethod drawmethod;

	unsigned char mapflag[MAX_COLOUR_MAPS]; // in 24bit mode, we need to know whether a colourmap is a common map or a palette

	static const char* pre_text =  // this is the skeleton of frame function
		"void main()\n"
		"{\n"
		"    int frame = getlocalvar(\"frame\");\n"
		"    int animnum = getlocalvar(\"animnum\");\n"
		"\n}\n";

	static const char* sur_text = // end of function text
		"\n}\n";

	static const char* ifid_text = // if expression to check animation id
		"    if(animnum==%d)\n"
		"    {\n"
		"        return;\n"
		"    }\n";

	static const char* endifid_text = // end of if
		"        return;\n"
		"    }\n";

	static const char* if_text = // this is the if expression of frame function
		"        if(frame==%d)\n"
		"        {\n";

	static const char* endif_text = // end of if
		"\n"
		"        }\n" ;

	static const char* comma_text = // arguments separator
		", ";

	static const char* call_text = //begin of function call
		"            %s(";

	static const char* endcall_text = //end of function call
		");\n";

	//printf("%s\n", name);
	strcpy(cur_owner, owner);
	owner = cur_owner;
	strncpy(cur_name, name, MAX_NAME_LEN);
	name = cur_name; // copy the name, cus the name might be a static variable pointer

	// Model already loaded but we might want to unload after level is completed.
	if((tempmodel=find_model(name))!=NULL) {tempmodel->unload = unload; return tempmodel;}

	cacheindex = get_cached_model_index(name);
	if(cacheindex < 0) shutdown(1, "Fatal: No cache entry for '%s' within '%s'\n\n", name, owner);

	filename = model_cache[cacheindex].path;

	if(buffer_pakfile(filename, &buf, &size)!=1) shutdown(1, "Unable to open file '%s'\n\n", filename);

	scriptbuf = (char*)tracemalloc("load_cached_model #scriptbuf", size*2+1);

	if(scriptbuf==NULL){
		shutdown(1, "Unable to create script buffer for file '%s' (%i bytes)", filename, size*2);
	}
	scriptbuf[0] = 0;

	// Alloc space for game model
	curr = tracemalloc("model_list", sizeof(s_model_list));
	curr->model = tracemalloc("curr->model", sizeof(s_model));
	if(curr == NULL || curr->model == NULL) shutdown(1, "Out of memory loading model from '%s'", filename);
	memset(curr->model, 0, sizeof(s_model));
	if(model_list == NULL){
		model_list = curr;
		model_list->next = NULL;
	}
	else{
		head = model_list;
		model_list = curr;
		model_list->next = head;
	}
	newchar = model_list->model;
	model_map = model_map_add(model_map, models_loaded);
	model_map[models_loaded++].model = newchar;
	memset(newchar,0,sizeof(s_model));
	newchar->name = model_cache[cacheindex].name; // well give it a name for sort method
	newchar->index = cacheindex;
	model_map_sort(); // sort it might improve searching speed...

	newchar->defense_factors        = (float*)tracemalloc("newchar->defense_factors",           sizeof(float)*max_attack_types);
	newchar->defense_pain           = (float*)tracemalloc("newchar->defense_pain",              sizeof(float)*max_attack_types);
	newchar->defense_knockdown      = (float*)tracemalloc("newchar->defense_knockdown",         sizeof(float)*max_attack_types);
	newchar->defense_blockpower     = (float*)tracemalloc("newchar->defense_blockpower",        sizeof(float)*max_attack_types);
	newchar->defense_blockthreshold = (float*)tracemalloc("newchar->defense_blockthreshold",    sizeof(float)*max_attack_types);
	newchar->defense_blockratio     = (float*)tracemalloc("newchar->defense_blockratio",        sizeof(float)*max_attack_types);
	newchar->defense_blocktype      = (float*)tracemalloc("newchar->defense_blocktype",         sizeof(float)*max_attack_types);
	memset(newchar->defense_factors,        0,  sizeof(float)*max_attack_types);
	memset(newchar->defense_pain,           0,  sizeof(float)*max_attack_types);
	memset(newchar->defense_knockdown,      0,  sizeof(float)*max_attack_types);
	memset(newchar->defense_blockpower,     0,  sizeof(float)*max_attack_types);
	memset(newchar->defense_blockthreshold, 0,  sizeof(float)*max_attack_types);
	memset(newchar->defense_blockratio,     0,  sizeof(float)*max_attack_types);
	memset(newchar->defense_blocktype,      0,  sizeof(float)*max_attack_types);

	newchar->offense_factors = (float*)tracemalloc("newchar->offense_factors", sizeof(float)*max_attack_types);
	memset(newchar->offense_factors, 0,sizeof(float)*max_attack_types);

	newchar->special = tracemalloc("newchar->special", sizeof(*newchar->special)*max_freespecials);
	memset(newchar->special, 0, sizeof(*newchar->special)*max_freespecials);
	newchar->animation_script   = alloc_script();
	newchar->update_script      = alloc_script();
	newchar->think_script       = alloc_script();
	newchar->didhit_script      = alloc_script();
	newchar->onspawn_script     = alloc_script();
	newchar->takedamage_script  = alloc_script();
	newchar->onfall_script      = alloc_script();
	newchar->onpain_script      = alloc_script();
	newchar->onblocks_script    = alloc_script();
	newchar->onblockw_script    = alloc_script();
	newchar->onblocko_script    = alloc_script();
	newchar->onblockz_script    = alloc_script();
	newchar->onblocka_script    = alloc_script();
	newchar->onmovex_script     = alloc_script();
	newchar->onmovez_script     = alloc_script();
	newchar->onmovea_script     = alloc_script();
	newchar->ondeath_script     = alloc_script();
	newchar->onkill_script      = alloc_script();
	newchar->didblock_script    = alloc_script();
	newchar->ondoattack_script  = alloc_script();
	newchar->key_script         = alloc_script();
	newchar->unload             = unload;
	newchar->jumpspeed          = -1;
	newchar->jumpheight         = 4;		        // 28-12-2004   Set default jump height to 4, if not specified
	newchar->runjumpheight      = 4;		        // Default jump height if a player is running
	newchar->runjumpdist        = 1;		        // Default jump distane if a player is running
	newchar->grabdistance       = 36;		        //  30-12-2004 Default grabdistance is same as originally set
	newchar->throwdamage        = 21;		        // default throw damage
	newchar->icon               = -1;
	newchar->icondie            = -1;
	newchar->iconpain           = -1;
	newchar->iconget            = -1;
	newchar->iconw              = -1;			    // No weapon icon set yet
	newchar->diesound           = -1;
	newchar->nolife             = 0;			    // default show life = 1 (yes)
	newchar->remove             = 1;			    // Flag set to weapons are removed upon hitting an opponent
	newchar->throwdist          = 2.5;
	newchar->counter            = 3;			    // Default 3 times to drop a weapon / projectile
	newchar->aimove             = -1;
	newchar->aiattack           = -1;
	newchar->throwframewait     = -1;               // makes sure throw animations run normally unless throwfram is specified, added by kbandressen 10/20/06
	newchar->path               = filename;         // Record path, so script can get it without looping the whole model collection.

	for(i=0; i<3; i++) newchar->iconmp[i] = -1;    // No magicbar icon set yet

	// Default Attack1 in chain must be referenced if not used.
	for(i=0; i<MAX_ATCHAIN; i++) newchar->atchain[i] = 1;
	newchar->chainlength = 1;

	if(magic_type == 1) newchar->mprate = 1;
	else newchar->mprate                = 2;
	newchar->chargerate = newchar->guardrate = 2;
	newchar->risetime[0]                = -1;
	newchar->sleepwait                  = 1000;
	newchar->jugglepoints[0] = newchar->jugglepoints[1] = 0;
	newchar->guardpoints[0] = newchar->guardpoints[1] = 0;
	newchar->mpswitch                   = -1;       // switch between reduce mp or gain mp for mpstabletype 4
	newchar->weaploss[0]                = -1;
	newchar->weaploss[1]                = -1;
	newchar->lifespan                   = (float)0xFFFFFFFF;
	newchar->summonkill                 = 1;
	newchar->candamage                  = -1;
	newchar->hostile                    = -1;
	newchar->projectilehit              = -1;
	newchar->subject_to_wall            = -1;
	newchar->subject_to_platform        = -1;
	newchar->subject_to_obstacle        = -1;
	newchar->subject_to_hole            = -1;
	newchar->subject_to_gravity         = -1;
	newchar->subject_to_screen          = -1;
	newchar->subject_to_minz            = -1;
	newchar->subject_to_maxz            = -1;
	newchar->no_adjust_base             = -1;
	newchar->pshotno                    = -1;
	newchar->project                    = -1;
	newchar->dust[0]                    = -1;
	newchar->dust[1]                    = -1;
	newchar->dust[2]                    = -1;
	newchar->bomb                       = -1;
	newchar->star                       = -1;
	newchar->knife                      = -1;

	newchar->animation = (s_anim**)tracemalloc("newchar->animation", sizeof(s_anim*)*max_animations);
	memset(newchar->animation, 0, sizeof(s_anim*)*max_animations);

	attack = emptyattack;      // empty attack
	drawmethod = plainmethod;  // better than memset it to 0
	memset(mapflag, 0, MAX_COLOUR_MAPS);

    // default string value, only by reference
	newchar->rider = get_cached_model_index("K'");
	newchar->flash = newchar->bflash = get_cached_model_index("flash");

    //Default offense/defense values.
	for(i=0;i<max_attack_types;i++)
	{
		newchar->offense_factors[i]     = 1;
		newchar->defense_factors[i]     = 1;
		newchar->defense_knockdown[i]   = 1;
	}

	for(i=0; i<3; i++)
	{
		newchar->sight[i*2] = -9999;
		newchar->sight[i*2+1] = 9999;
	}

	newchar->offense_factors[ATK_BLAST]     = 1;
	newchar->defense_factors[ATK_BLAST]     = 1;
	newchar->defense_knockdown[ATK_BLAST]   = 1;
	newchar->offense_factors[ATK_BURN]      = 1;
	newchar->defense_factors[ATK_BURN]      = 1;
	newchar->defense_knockdown[ATK_BURN]    = 1;
	newchar->offense_factors[ATK_FREEZE]    = 1;
	newchar->defense_factors[ATK_FREEZE]    = 1;
	newchar->defense_knockdown[ATK_FREEZE]  = 1;
	newchar->offense_factors[ATK_SHOCK]     = 1;
	newchar->defense_factors[ATK_SHOCK]     = 1;
	newchar->defense_knockdown[ATK_SHOCK]   = 1;
	newchar->offense_factors[ATK_STEAL]     = 1;
	newchar->defense_factors[ATK_STEAL]     = 1;
	newchar->defense_knockdown[ATK_STEAL]   = 1;

    // Now interpret the contents of buf line by line
	while(pos<size)
	{
		command = findarg(buf+pos, 0);
		if(command[0]){
			if(stricmp(command, "name")==0){
				value = findarg(buf+pos, 1);
				if((tempmodel=find_model(value)) && tempmodel!=newchar) shutdown(1, "Duplicate model name '%s'", value);
				tempInt = get_cached_model_index(value);
				model_cache[tempInt].model = newchar;
				newchar->name = model_cache[tempInt].name;
				if(stricmp(newchar->name, "steam")==0)
				{
					newchar->alpha = 1;
				}
			}
			else if(stricmp(command, "type")==0){    // Moved here to be able to access the character type later on
				value = findarg(buf+pos, 1);
				if(stricmp(value, "none")==0){
					newchar->type = TYPE_NONE;
				}
				else if(stricmp(value, "player")==0){
					newchar->type = TYPE_PLAYER;
					newchar->nopassiveblock = 1;
					for(i=0; i<MAX_ATCHAIN; i++)
					{
						if(i < 2 || i > 3) newchar->atchain[i] = 1;
						else newchar->atchain[i] = i;
					}
					newchar->chainlength            = 4;
					newchar->bounce                 = 1;
					newchar->subject_to_wall        = 1;
					newchar->subject_to_platform    = 1;
					newchar->subject_to_obstacle    = 1;
					newchar->subject_to_hole        = 1;
					newchar->subject_to_gravity     = 1;
					newchar->subject_to_screen      = 1;
					newchar->subject_to_minz        = 1;
					newchar->subject_to_maxz        = 1;
					newchar->no_adjust_base         = 0;
				}
				else if(stricmp(value, "enemy")==0){
					newchar->type                   = TYPE_ENEMY;
					newchar->bounce                 = 1;
					newchar->subject_to_wall        = 1;
					newchar->subject_to_platform    = 1;
					newchar->subject_to_hole        = 1;
					newchar->subject_to_obstacle    = 1;
					newchar->subject_to_gravity     = 1;
					newchar->subject_to_minz        = 1;
					newchar->subject_to_maxz        = 1;
					newchar->no_adjust_base         = 0;
				}
				else if(stricmp(value, "item")==0){
					newchar->type                   = TYPE_ITEM;
					newchar->subject_to_wall        = 1;
					newchar->subject_to_platform    = 1;
					newchar->subject_to_hole        = 1;
					newchar->subject_to_obstacle    = 1;
					newchar->subject_to_gravity     = 1;
					newchar->subject_to_minz        = 1;
					newchar->subject_to_maxz        = 1;
					newchar->no_adjust_base         = 0;
				}
				else if(stricmp(value, "obstacle")==0){
					newchar->type                   = TYPE_OBSTACLE;
					newchar->subject_to_wall        = 1;
					newchar->subject_to_platform    = 1;
					newchar->subject_to_hole        = 1;
					newchar->subject_to_gravity     = 1;
					newchar->subject_to_minz        = 1;
					newchar->subject_to_maxz        = 1;
					newchar->no_adjust_base         = 0;
				}
				else if(stricmp(value, "steamer")==0){
					newchar->type = TYPE_STEAMER;
				}
				// my new types   7-1-2005
				else if(stricmp(value, "pshot")==0){
					newchar->type = TYPE_SHOT;
					if(newchar->aimove==-1) newchar->aimove = 0;
					newchar->aimove |= AIMOVE1_ARROW;
					if(!newchar->offscreenkill) newchar->offscreenkill = 200;
					newchar->subject_to_hole                = 0;
					newchar->subject_to_gravity             = 1;
					newchar->subject_to_wall                = 0;
					newchar->subject_to_platform            = 0;
					newchar->subject_to_screen              = 0;
					newchar->subject_to_minz                = 1;
					newchar->subject_to_maxz                = 1;
					newchar->subject_to_platform            = 0;
					newchar->no_adjust_base                 = 1;
				}
				else if(stricmp(value, "trap")==0){
					newchar->type                   = TYPE_TRAP;
					newchar->subject_to_wall        = 1;
					newchar->subject_to_platform    = 1;
					newchar->subject_to_hole        = 1;
					newchar->subject_to_gravity     = 1;
					newchar->subject_to_minz        = 1;
					newchar->subject_to_maxz        = 1;
					newchar->no_adjust_base         = 0;
				}
				else if(stricmp(value, "text")==0){    // Used for displaying text/images and freezing the screen
					newchar->type                   = TYPE_TEXTBOX;
					newchar->subject_to_gravity     = 0;
					newchar->subject_to_minz        = 1;
					newchar->subject_to_maxz        = 1;
				}
				else if(stricmp(value, "endlevel")==0){    // Used for ending the level when the players reach a certain point
					newchar->type                   = TYPE_ENDLEVEL;
					newchar->subject_to_wall        = 1;
					newchar->subject_to_platform    = 1;
					newchar->subject_to_hole        = 1;
					newchar->subject_to_obstacle    = 1;
					newchar->subject_to_gravity     = 1;
				}
				else if(stricmp(value, "npc")==0){    // NPC type
					newchar->type                   = TYPE_NPC;
					newchar->bounce                 = 1;
					newchar->subject_to_wall        = 1;
					newchar->subject_to_platform    = 1;
					newchar->subject_to_hole        = 1;
					newchar->subject_to_obstacle    = 1;
					newchar->subject_to_gravity     = 1;
					newchar->subject_to_minz        = 1;
					newchar->subject_to_maxz        = 1;
					newchar->no_adjust_base         = 0;
				}
				else if(stricmp(value, "panel")==0){    // NPC type
					newchar->type                   = TYPE_PANEL;
					newchar->antigravity            = 1.0; //float type
					newchar->subject_to_gravity     = 1;
					newchar->no_adjust_base         = 1;
				}
				else shutdown(1, "Model '%s' has invalid type: '%s'", filename, value);
			}
			else if(stricmp(command, "subtype")==0){
				value = findarg(buf+pos, 1);
				if(stricmp(value, "biker")==0){
				newchar->subtype                                        = SUBTYPE_BIKER;
				if(newchar->aimove==-1) newchar->aimove                 = 0;
				newchar->aimove |= AIMOVE1_BIKER;
				for(i=0; i<MAX_ATKS; i++) newchar->defense_factors[i]   = 2;
				if(!newchar->offscreenkill) newchar->offscreenkill = 300;
				newchar->subject_to_hole                                = 1;
				newchar->subject_to_gravity                             = 1;
				newchar->subject_to_wall                                = 0;
				newchar->subject_to_platform                            = 0;
				newchar->subject_to_screen                              = 0;
				newchar->subject_to_minz                                = 1;
				newchar->subject_to_maxz                                = 1;
				newchar->subject_to_platform                            = 0;
				newchar->no_adjust_base                                 = 0;
			}
			else if(stricmp(value, "arrow")==0){  // 7-1-2005 Arrow type
				newchar->subtype = SUBTYPE_ARROW;   // 7-1-2005 Arrow type
				if(newchar->aimove==-1) newchar->aimove = 0;
				newchar->aimove |= AIMOVE1_ARROW;
				if(!newchar->offscreenkill) newchar->offscreenkill = 200;
				newchar->subject_to_hole        = 0;
				newchar->subject_to_gravity     = 1;
				newchar->subject_to_wall        = 0;
				newchar->subject_to_platform    = 0;
				newchar->subject_to_screen      = 0;
				newchar->subject_to_minz        = 1;
				newchar->subject_to_maxz        = 1;
				newchar->subject_to_platform    = 0;
				newchar->no_adjust_base         = 1;
			}
			else if(stricmp(value, "notgrab")==0){  // 7-1-2005 notgrab type
				newchar->subtype = SUBTYPE_NOTGRAB;   // 7-1-2005 notgrab type
			}
			//    ltb 1-18-05  Item Subtype
			else if(stricmp(value, "touch")==0){  // 7-1-2005 notgrab type
				newchar->subtype = SUBTYPE_TOUCH;   // 7-1-2005 notgrab type
			}
			else if(stricmp(value, "weapon")==0){  // 7-1-2005 notgrab type
				newchar->subtype = SUBTYPE_WEAPON;   // 7-1-2005 notgrab type
			}
			else if(stricmp(value, "noskip")==0){    // Text animation cannot be skipped if subtype noskip
				newchar->subtype = SUBTYPE_NOSKIP;
			}
			else if(stricmp(value, "flydie")==0){    // Obstacle will fly across the screen when hit if subtype flydie
				newchar->subtype = SUBTYPE_FLYDIE;
			}
			else if(stricmp(value, "both")==0){
				newchar->subtype = SUBTYPE_BOTH;
			}
			else if(stricmp(value, "project")==0){
				newchar->subtype = SUBTYPE_PROJECTILE;
			}
			else if(stricmp(value, "follow")==0){
				newchar->subtype = SUBTYPE_FOLLOW;
			}
			else if(stricmp(value, "chase")==0){
				newchar->subtype = SUBTYPE_CHASE;
			}
			//    end new subtype
			else shutdown(1, "Model '%s' has invalid subtype: '%s'", filename, value);
		}
		else if(stricmp(command, "stats")==0){
			value = findarg(buf+pos, 1);
			newchar->stats[atoi(value)] = atof(findarg(buf+pos, 2));
		}
		else if(stricmp(command, "health")==0){
			value = findarg(buf+pos, 1);
			newchar->health = atoi(value);
		}
		else if(stricmp(command, "scroll")==0){
			value = findarg(buf+pos, 1);
			newchar->scroll = atof(value);
		}
		//Left for backward compatability. See mpset.
		else if(stricmp(command, "mp")==0){// mp values to put max mp for player by tails
			value = findarg(buf+pos, 1);
			newchar->mp = atoi(value);
		}
		else if(stricmp(command, "nolife")==0){    // Feb 25, 2005 - Flag to display enemy life or not
			newchar->nolife = atoi(findarg(buf+pos, 1));
		}
		else if(stricmp(command, "makeinv")==0){    // Mar 12, 2005 - If a value is supplied, corresponds to amount of time the player spawns invincible
			newchar->makeinv = atoi(findarg(buf+pos, 1)) * GAME_SPEED;
			if(atoi(findarg(buf+pos, 2))) newchar->makeinv = -newchar->makeinv;
		}
		else if(stricmp(command, "riseinv")==0){
			newchar->riseinv = atoi(findarg(buf+pos, 1)) * GAME_SPEED;
			if(atoi(findarg(buf+pos, 2))) newchar->riseinv = -newchar->riseinv;
		}
		else if(stricmp(command, "load")==0){
			strncpy(load_name, findarg(buf+pos, 1), MAX_NAME_LEN);
			load_cached_model(load_name, name, atoi(findarg(buf+pos, 2)));
		}
		else if(stricmp(command, "score")==0){
			newchar->score = atoi(findarg(buf+pos, 1));
			newchar->multiple = atoi(findarg(buf+pos, 2));			// New var multiple for force/scoring
		}
		else if(stricmp(command, "smartbomb")==0){ //smartbomb now use a normal attack box
			if(!newchar->smartbomb)
			{
				newchar->smartbomb = tracemalloc("newchar->smartbomb", sizeof(s_attack));
				*(newchar->smartbomb) = emptyattack;
			}
			else shutdown(1, "Model '%s' has multiple smartbomb commands defined.", filename);
			newchar->smartbomb->attack_force = atoi(findarg(buf+pos, 1));			// Special force
			newchar->smartbomb->attack_type = atoi(findarg(buf+pos, 2));			// Special attack type
			newchar->smartbomb->attack_drop = 1; //by default
			newchar->smartbomb->dropv[0] = 3;
			if(newchar->smartbomb->attack_type==ATK_BLAST)
			{
				newchar->smartbomb->blast = 1;
				newchar->smartbomb->dropv[1] = 2.5;
			}
			else
			{
				newchar->smartbomb->dropv[1] = (float)1.2;
			}
			if(newchar->smartbomb->attack_type==ATK_FREEZE)
			{
				newchar->smartbomb->freeze = 1;
				newchar->smartbomb->forcemap = -1;
				newchar->smartbomb->attack_drop = 0;
			}
			else if(newchar->smartbomb->attack_type==ATK_STEAL)
			{
				newchar->smartbomb->steal = 1;
			}
			if(newchar->type == TYPE_ITEM)
			{
				newchar->dofreeze = 0;								// Items don't animate
				newchar->smartbomb->freezetime = atoi(findarg(buf+pos, 3)) * GAME_SPEED;
			}
			else
			{
				newchar->dofreeze = atoi(findarg(buf+pos, 3));		// Are all animations frozen during special
				newchar->smartbomb->freezetime = atoi(findarg(buf+pos, 4)) * GAME_SPEED;
			}
		}
		else if(stricmp(command, "bounce")==0){						// Flag to determine if bounce/quake is to be used.
			newchar->bounce = atoi(findarg(buf+pos, 1));
		}
		else if(stricmp(command, "noquake")==0){					// Mar 12, 2005 - Flag to determine if entity shakes screen
			newchar->noquake = atoi(findarg(buf+pos, 1));
		}
		else if(stricmp(command, "blockback")==0){					// Flag to determine if attacks can be blocked from behind
			newchar->blockback = atoi(findarg(buf+pos, 1));
		}
		else if(stricmp(command, "hitenemy")==0){					// Flag to determine if an enemy projectile will hit enemies
			value = findarg(buf+pos, 1);
			if(atoi(value) == 1)
			newchar->candamage = newchar->hostile = TYPE_PLAYER | TYPE_ENEMY;
			else if(atoi(value) == 2)
			newchar->candamage = newchar->hostile = TYPE_PLAYER;
			newchar->ground = atoi(findarg(buf+pos, 2));    // Added to determine if enemies are damaged with mid air projectiles or ground only
		}
		else if(stricmp(command, "hostile")==0){
			i = 1;
			newchar->hostile = 0;
			value = findarg(buf+pos, i);
			while(value && value[0])
			{
				if(stricmp(value, "enemy")==0){
					newchar->hostile |= TYPE_ENEMY;
				} else if(stricmp(value, "player")==0){
					newchar->hostile |= TYPE_PLAYER;
				} else if(stricmp(value, "obstacle")==0){
					newchar->hostile |= TYPE_OBSTACLE;
				} else if(stricmp(value, "shot")==0){
					newchar->hostile |= TYPE_SHOT;
				} else if(stricmp(value, "npc")==0){
					newchar->hostile |= TYPE_NPC;
				}
				i++;
				value = findarg(buf+pos, i);
			}
		}
		else if(stricmp(command, "candamage")==0){
			i = 1;
			newchar->candamage = 0;
			value = findarg(buf+pos, i);
			while(value && value[0])
			{
				if(stricmp(value, "enemy")==0){
					newchar->candamage |= TYPE_ENEMY;
				} else if(stricmp(value, "player")==0){
					newchar->candamage |= TYPE_PLAYER;
				} else if(stricmp(value, "obstacle")==0){
					newchar->candamage |= TYPE_OBSTACLE;
				} else if(stricmp(value, "shot")==0){
					newchar->candamage |= TYPE_SHOT;
				} else if(stricmp(value, "npc")==0){
					newchar->candamage |= TYPE_NPC;
				} else if(stricmp(value, "ground")==0){ // not really needed, though
					newchar->ground = 1;
				} //else {
				// wont shut down
				//}
				i++;
				value = findarg(buf+pos, i);
			}
		}
		else if(stricmp(command, "projectilehit")==0){
			i = 1;
			newchar->projectilehit = 0;
			value = findarg(buf+pos, i);
			while(value && value[0])
			{
				if(stricmp(value, "enemy")==0){
					newchar->projectilehit |= TYPE_ENEMY;
				} else if(stricmp(value, "player")==0){
					newchar->projectilehit |= TYPE_PLAYER;
				} else if(stricmp(value, "obstacle")==0){
					newchar->projectilehit |= TYPE_OBSTACLE;
				} else if(stricmp(value, "shot")==0){
					newchar->projectilehit |= TYPE_SHOT;
				} else if(stricmp(value, "npc")==0){
					newchar->projectilehit |= TYPE_NPC;
				} //else {
					// wont shut down
					//}
				i++;
				value = findarg(buf+pos, i);
			}
		}
		else if(stricmp(command, "aimove")==0){
			if(!aimoveset)
			{
				newchar->aimove = 0;
				aimoveset = 1;
			}
			value = findarg(buf+pos, 1);
			//main A.I. move switches
			if(value && value[0])
			{
				if(stricmp(value, "normal")==0){
					newchar->aimove |= AIMOVE1_NORMAL;
				}
				else if(stricmp(value, "chase")==0){
					newchar->aimove |= AIMOVE1_CHASE;
				}
				else if(stricmp(value, "chasex")==0){
					newchar->aimove |= AIMOVE1_CHASEX;
				}
				else if(stricmp(value, "chasez")==0){
					newchar->aimove |= AIMOVE1_CHASEZ;
				}
				else if(stricmp(value, "avoid")==0){
					newchar->aimove |= AIMOVE1_AVOID;
				}
				else if(stricmp(value, "avoidx")==0){
					newchar->aimove |= AIMOVE1_AVOIDX;
				}
				else if(stricmp(value, "avoidz")==0){
					newchar->aimove |= AIMOVE1_AVOIDZ;
				}
				else if(stricmp(value, "wander")==0){
					newchar->aimove |= AIMOVE1_WANDER;
				}
				else if(stricmp(value, "biker")==0){
					newchar->aimove |= AIMOVE1_BIKER;
				}
				else if(stricmp(value, "arrow")==0){
					newchar->aimove |= AIMOVE1_ARROW;
					if(!newchar->offscreenkill) newchar->offscreenkill = 200;
				}
				else if(stricmp(value, "star")==0){
					newchar->aimove |= AIMOVE1_STAR;
				}
				else if(stricmp(value, "bomb")==0){
					newchar->aimove |= AIMOVE1_BOMB;
				}
				else if(stricmp(value, "nomove")==0){
					newchar->aimove |= AIMOVE1_NOMOVE;
				}
				else shutdown(1, "Model '%s' has invalid A.I. move switch: '%s'", filename, value);
			}
			value = findarg(buf+pos, 2);
			//sub A.I. move switches
			if(value && value[0])
			{
				if(stricmp(value, "normal")==0){
					newchar->aimove |= AIMOVE2_NORMAL;
				}
				else if(stricmp(value, "ignoreholes")==0){
					newchar->aimove |= AIMOVE2_IGNOREHOLES;
				}
				else shutdown(1, "Model '%s' has invalid A.I. move switch: '%s'", filename, value);
			}
		}
		else if(stricmp(command, "aiattack")==0){
			if(newchar->aiattack==-1) newchar->aiattack = 0;
			//do nothing for now, until ai attack is implemented
		}
		else if(stricmp(command, "subject_to_wall")==0)
		{
			newchar->subject_to_wall = (0!=atoi(findarg(buf+pos, 1)));
		}
		else if(stricmp(command, "subject_to_hole")==0)
		{
			newchar->subject_to_hole = (0!=atoi(findarg(buf+pos, 1)));
		}
		else if(stricmp(command, "subject_to_platform")==0)
		{
			newchar->subject_to_platform = (0!=atoi(findarg(buf+pos, 1)));
		}
		else if(stricmp(command, "subject_to_obstacle")==0)
		{
			newchar->subject_to_obstacle = (0!=atoi(findarg(buf+pos, 1)));
		}
		else if(stricmp(command, "subject_to_gravity")==0)
		{
			newchar->subject_to_gravity = (0!=atoi(findarg(buf+pos, 1)));
		}
		else if(stricmp(command, "subject_to_screen")==0)
		{
			newchar->subject_to_screen = (0!=atoi(findarg(buf+pos, 1)));
		}
		else if(stricmp(command, "subject_to_minz")==0)
		{
			newchar->subject_to_minz = (0!=atoi(findarg(buf+pos, 1)));
		}
		else if(stricmp(command, "subject_to_maxz")==0)
		{
			newchar->subject_to_maxz = (0!=atoi(findarg(buf+pos, 1)));
		}
		else if(stricmp(command, "no_adjust_base")==0)
		{
			newchar->no_adjust_base = (0!=atoi(findarg(buf+pos, 1)));
		}
		else if(stricmp(command, "instantitemdeath")==0)
		{
			newchar->instantitemdeath = atoi(findarg(buf+pos, 1));
		}
		else if(stricmp(command, "secret")==0){
			newchar->secret = atoi(findarg(buf+pos, 1));
		}
		else if(stricmp(command, "modelflag")==0){ // model copy flag
			newchar->model_flag = atoi(findarg(buf+pos, 1));
		}
		// weapons
		else if(stricmp(command, "weaploss")==0){
			newchar->weaploss[0] = atoi(findarg(buf+pos, 1));
			newchar->weaploss[1] = atoi(findarg(buf+pos, 2));
		}
		else if(stricmp(command, "weapnum")==0){
			newchar->weapnum = atoi(findarg(buf+pos, 1));
		}
		else if(stricmp(command, "project")==0){  // New projectile subtype
			value = findarg(buf+pos, 1);
			if(stricmp(value, "none")==0) newchar->project = -1;
			else newchar->project = get_cached_model_index(value);
		}
		else if(stricmp(command, "weapons")==0){
			if(!newchar->weapon)
			{
				newchar->weapon = tracemalloc("newchar->weapon", sizeof(*newchar->weapon));
				memset(newchar->weapon, 0xFF, sizeof(*newchar->weapon));
				newchar->ownweapons = 1;
			}
			for(weap = 0; weap<MAX_WEAPONS; weap++){
				value = findarg(buf+pos, weap+1);
				if(value[0]){
					if(stricmp(value, "none")!=0){
						(*newchar->weapon)[weap] = get_cached_model_index(value);
					} else { // make empty weapon slots  2007-2-16
						(*newchar->weapon)[weap] = -1;
					}
					last = weap;
				}
				else (*newchar->weapon)[weap] = (*newchar->weapon)[last];
			}
		}
		//here weapons things like shoot rest type of weapon ect..by tails
		else if(stricmp(command, "shootnum")==0){
			newchar->shootnum = atoi(findarg(buf+pos, 1));
		}
		else if(stricmp(command, "reload")==0){
			newchar->reload = atoi(findarg(buf+pos, 1));
		}
		else if(stricmp(command, "typeshot")==0){
			newchar->typeshot = atoi(findarg(buf+pos, 1));
		}
		else if(stricmp(command, "counter")==0){
			newchar->counter = atoi(findarg(buf+pos, 1));
		}
		else if(stricmp(command, "animal")==0){
			newchar->animal = atoi(findarg(buf+pos, 1));
		}
		// end weapons
		else if(stricmp(command, "rider")==0){
			value = findarg(buf+pos, 1);
			if(stricmp(value, "none")==0) newchar->rider = -1;
			else newchar->rider = get_cached_model_index(value);
		}
		else if(stricmp(command, "knife")==0 ||
			stricmp(command, "fireb")==0 ||
			stricmp(command, "playshot")==0 ||
			stricmp(command, "playshotw")==0) {
			value = findarg(buf+pos, 1);
			if(stricmp(value, "none")==0) newchar->knife = -1;
			else  newchar->knife = get_cached_model_index(value);
		}
		else if(stricmp(command, "playshotno")==0){
			value = findarg(buf+pos, 1);
			if(stricmp(value, "none")==0) newchar->pshotno = -1;
			else newchar->pshotno = get_cached_model_index(value);
		}
		else if(stricmp(command, "star")==0){
			value = findarg(buf+pos, 1);
			if(stricmp(value, "none")==0) newchar->star = -1;
			else newchar->star = get_cached_model_index(value);
		}
		else if(stricmp(command, "bomb")==0 ||
			stricmp(command, "playbomb")==0){
			value = findarg(buf+pos, 1);
			if(stricmp(value, "none")==0) newchar->bomb = -1;
			else newchar->bomb = get_cached_model_index(value);
		}
		else if(stricmp(command, "flash")==0){    // Now all characters can have their own flash - even projectiles (useful for blood)
			value = findarg(buf+pos, 1);
			if(stricmp(value, "none")==0) newchar->flash = -1;
			else newchar->flash = get_cached_model_index(value);
		}
		else if(stricmp(command, "bflash")==0){    // Flash that is spawned if an attack is blocked
			value = findarg(buf+pos, 1);
			if(stricmp(value, "none")==0) newchar->bflash = -1;
			else newchar->bflash = get_cached_model_index(value);
		}
		else if(stricmp(command, "dust")==0){    // Spawned when hitting the ground to "kick up dust"
			value = findarg(buf+pos, 1);
			if(stricmp(value, "none")==0) newchar->dust[0] = -1;
			else newchar->dust[0] = get_cached_model_index(value);
			value = findarg(buf+pos, 2);
			if(stricmp(value, "none")==0) newchar->dust[1] = -1;
			else newchar->dust[1] = get_cached_model_index(value);
			value = findarg(buf+pos, 3);
			if(stricmp(value, "none")==0) newchar->dust[2] = -1;
			else newchar->dust[2] = get_cached_model_index(value);
		}
		else if(stricmp(command, "branch")==0){    // for endlevel item's level branch
			value = findarg(buf+pos, 1);
			if(!newchar->branch)
			{
				newchar->branch = tracemalloc("newchar->branch", MAX_NAME_LEN+1);
				newchar->branch[0] = 0;
			}
			strncpy(newchar->branch, value, MAX_NAME_LEN);
		}
		else if(stricmp(command, "cantgrab")==0 ||
			stricmp(command, "notgrab")==0){
			tempInt = atoi(findarg(buf+pos, 1));
			if(tempInt == 2) newchar->grabforce = -999999;
			else             newchar->antigrab = 1;
		}
		else if(stricmp(command, "antigrab")==0) // a can grab b: a->antigrab - b->grabforce <=0
		{
			newchar->antigrab = atoi(findarg(buf+pos, 1));
		}
		else if(stricmp(command, "grabforce")==0)
		{
			newchar->grabforce = atoi(findarg(buf+pos, 1));
		}
		else if(stricmp(command, "grabback")==0){
			newchar->grabback = atoi(findarg(buf+pos, 1));
		}
		else if(stricmp(command, "offscreenkill")==0){
			newchar->offscreenkill = atoi(findarg(buf+pos, 1));
		}
		else if(stricmp(command, "falldie")==0 ||
			stricmp(command, "death")==0){
			newchar->falldie = atoi(findarg(buf+pos, 1));
		}
		else if(stricmp(command, "speed")==0){
			value = findarg(buf+pos, 1);
			newchar->speed = atof(value);
			newchar->speed /= 10;
			if(newchar->speed < 0.5) newchar->speed = 0.5;
			if(newchar->speed > 30) newchar->speed = 30;
		}
		else if(stricmp(command, "speedf")==0){ // float speed
			value = findarg(buf+pos, 1);
			newchar->speed = atof(value);
		}
		else if(stricmp(command, "jumpspeed")==0){
			value = findarg(buf+pos, 1);
			newchar->jumpspeed = atof(value);
			newchar->jumpspeed /= 10;
		}
		else if(stricmp(command, "jumpspeedf")==0){
			value = findarg(buf+pos, 1);
			newchar->jumpspeed = atof(value);
		}
		else if(stricmp(command, "antigravity")==0){
			value = findarg(buf+pos, 1);
			newchar->antigravity = atof(value);
			newchar->antigravity /= 100;
		}
		else if(stricmp(command, "stealth")==0){
			newchar->stealth[0] = atoi(findarg(buf+pos, 1));
			newchar->stealth[1] = atoi(findarg(buf+pos, 2));
		}
		else if(stricmp(command, "jugglepoints")==0){
			value = findarg(buf+pos, 1);
			newchar->jugglepoints[0] = atoi(value);
			newchar->jugglepoints[1] = atoi(value);
		}
		else if(stricmp(command, "riseattacktype")==0){
			value = findarg(buf+pos, 1);
			newchar->riseattacktype = atoi(value);
		}
		else if(stricmp(command, "guardpoints")==0){
			value = findarg(buf+pos, 1);
			newchar->guardpoints[0] = atoi(value);
			newchar->guardpoints[1] = atoi(value);
		}

#define tempdef(x, y, z, p, k, b, t, r, e) \
x(stricmp(value, #y)==0)\
{\
    newchar->z[ATK_##y] = atof(findarg(buf+pos, 2));\
    /*newchar->z[ATK_##y] /= 100;*/\
    newchar->p[ATK_##y] = atof(findarg(buf+pos, 3));\
    newchar->k[ATK_##y] = atof(findarg(buf+pos, 4));\
    /*newchar->k[ATK_##y] /= 100;*/\
    newchar->b[ATK_##y] = atof(findarg(buf+pos, 5));\
    newchar->t[ATK_##y] = atof(findarg(buf+pos, 6));\
    newchar->r[ATK_##y] = atof(findarg(buf+pos, 7));\
    /*newchar->r[ATK_##y] /= 100;*/\
    newchar->e[ATK_##y] = atof(findarg(buf+pos, 8));\
}
		else if(stricmp(command, "defense")==0){
			value = findarg(buf+pos, 1);
			tempdef(if, NORMAL, defense_factors, defense_pain, defense_knockdown, defense_blockpower, defense_blockthreshold, defense_blockratio, defense_blocktype)
			tempdef(else if, NORMAL2,   defense_factors, defense_pain, defense_knockdown, defense_blockpower, defense_blockthreshold, defense_blockratio, defense_blocktype)
			tempdef(else if, NORMAL3,   defense_factors, defense_pain, defense_knockdown, defense_blockpower, defense_blockthreshold, defense_blockratio, defense_blocktype)
			tempdef(else if, NORMAL4,   defense_factors, defense_pain, defense_knockdown, defense_blockpower, defense_blockthreshold, defense_blockratio, defense_blocktype)
			tempdef(else if, NORMAL5,   defense_factors, defense_pain, defense_knockdown, defense_blockpower, defense_blockthreshold, defense_blockratio, defense_blocktype)
			tempdef(else if, NORMAL6,   defense_factors, defense_pain, defense_knockdown, defense_blockpower, defense_blockthreshold, defense_blockratio, defense_blocktype)
			tempdef(else if, NORMAL7,   defense_factors, defense_pain, defense_knockdown, defense_blockpower, defense_blockthreshold, defense_blockratio, defense_blocktype)
			tempdef(else if, NORMAL8,   defense_factors, defense_pain, defense_knockdown, defense_blockpower, defense_blockthreshold, defense_blockratio, defense_blocktype)
			tempdef(else if, NORMAL9,   defense_factors, defense_pain, defense_knockdown, defense_blockpower, defense_blockthreshold, defense_blockratio, defense_blocktype)
			tempdef(else if, NORMAL10,  defense_factors, defense_pain, defense_knockdown, defense_blockpower, defense_blockthreshold, defense_blockratio, defense_blocktype)
			tempdef(else if, BLAST,     defense_factors, defense_pain, defense_knockdown, defense_blockpower, defense_blockthreshold, defense_blockratio, defense_blocktype)
			tempdef(else if, STEAL,     defense_factors, defense_pain, defense_knockdown, defense_blockpower, defense_blockthreshold, defense_blockratio, defense_blocktype)
			tempdef(else if, BURN,      defense_factors, defense_pain, defense_knockdown, defense_blockpower, defense_blockthreshold, defense_blockratio, defense_blocktype)
			tempdef(else if, SHOCK,     defense_factors, defense_pain, defense_knockdown, defense_blockpower, defense_blockthreshold, defense_blockratio, defense_blocktype)
			tempdef(else if, FREEZE,    defense_factors, defense_pain, defense_knockdown, defense_blockpower, defense_blockthreshold, defense_blockratio, defense_blocktype)
			else if(strnicmp(value, "normal", 6)==0)
			{
				tempInt = atoi(value+6);
				if(tempInt<11) tempInt = 11;
				newchar->defense_factors[tempInt+STA_ATKS-1]        = atof(findarg(buf+pos, 2));
				newchar->defense_pain[tempInt+STA_ATKS-1]           = atof(findarg(buf+pos, 3));
				newchar->defense_knockdown[tempInt+STA_ATKS-1]      = atof(findarg(buf+pos, 4));
				newchar->defense_blockpower[tempInt+STA_ATKS-1]     = atof(findarg(buf+pos, 5));
				newchar->defense_blockthreshold[tempInt+STA_ATKS-1] = atof(findarg(buf+pos, 6));
				newchar->defense_blockratio[tempInt+STA_ATKS-1]     = atof(findarg(buf+pos, 7));
				newchar->defense_blocktype[tempInt+STA_ATKS-1]      = atof(findarg(buf+pos, 8));
			}
			else if(stricmp(value, "ALL")==0)
			{
				for(i=0;i<max_attack_types;i++)
				{
					newchar->defense_factors[i]         = atof(findarg(buf+pos, 2));
					newchar->defense_pain[i]            = atof(findarg(buf+pos, 3));
					newchar->defense_knockdown[i]       = atof(findarg(buf+pos, 4));
					newchar->defense_blockpower[i]      = atof(findarg(buf+pos, 5));
					newchar->defense_blockthreshold[i]  = atof(findarg(buf+pos, 6));
					newchar->defense_blockratio[i]      = atof(findarg(buf+pos, 7));
					newchar->defense_blocktype[i]       = atof(findarg(buf+pos, 8));
				}
			}
		}
#undef tempdef
#define tempoff(x, y, z) \
x(stricmp(value, #y)==0)\
{\
    newchar->z[ATK_##y] = atof(findarg(buf+pos, 2));\
    /*newchar->z[ATK_##y] /= 100;*/\
}
            else if(stricmp(command, "offense")==0){
                value = findarg(buf+pos, 1);
                tempoff(if,         NORMAL,     offense_factors)
                tempoff(else if,    NORMAL2,    offense_factors)
                tempoff(else if,    NORMAL3,    offense_factors)
                tempoff(else if,    NORMAL4,    offense_factors)
                tempoff(else if,    NORMAL5,    offense_factors)
                tempoff(else if,    NORMAL6,    offense_factors)
                tempoff(else if,    NORMAL7,    offense_factors)
                tempoff(else if,    NORMAL8,    offense_factors)
                tempoff(else if,    NORMAL9,    offense_factors)
                tempoff(else if,    NORMAL10,   offense_factors)
                tempoff(else if,    BLAST,      offense_factors)
                tempoff(else if,    STEAL,      offense_factors)
                tempoff(else if,    BURN,       offense_factors)
                tempoff(else if,    SHOCK,      offense_factors)
                tempoff(else if,    FREEZE,     offense_factors)
                else if(strnicmp(value, "normal", 6)==0)
                {
                    tempInt = atoi(value+6);
                    if(tempInt<11) tempInt = 11;
                    newchar->offense_factors[tempInt+STA_ATKS-1] = atof(findarg(buf+pos, 2));
                }
                else if(stricmp(value, "ALL")==0)
                {
                    tempFloat = atof(findarg(buf+pos, 2));
                    for(i=0;i<max_attack_types;i++)
                    {
                        newchar->offense_factors[i] = tempFloat;
                    }
                }
            }
#undef tempoff
            else if(stricmp(command, "height")==0){
                newchar->height = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "jumpheight")==0){        // 28-12-2004 if string for jump height found
                newchar->jumpheight = atof(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "jumpmove")==0){    // see openbor.h for details
                newchar->jumpmovex = atoi(findarg(buf+pos, 1));
                newchar->jumpmovez = atoi(findarg(buf+pos, 2));
            }
            else if(stricmp(command, "knockdowncount")==0){
                newchar->knockdowncount = atof(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "grabdistance")==0){        // 30-12-2004 if string for grabdistance found
                newchar->grabdistance = atof(findarg(buf+pos, 1));                    // 30-12-2004 and store for character
            }
            else if(stricmp(command, "grabfinish")==0){ // wait until grab animation is finished
                newchar->grabfinish = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "throwdamage")==0){
                newchar->throwdamage = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "shadow")==0){
                newchar->shadow = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "gfxshadow")==0){
                newchar->gfxshadow = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "aironly")==0){    // Shadows display in air only?
                newchar->aironly = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "fmap")==0){    // Map that corresponds with the remap when a character is frozen
                newchar->fmap = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "komap")==0){   // Remap when character is KO'd.
                newchar->komap[0] = atoi(findarg(buf+pos, 1));  //Remap.
                newchar->komap[1] = atoi(findarg(buf+pos, 2));  //Type: 0 start of fall/death, 1 last frame.
            }
            else if(stricmp(command, "hmap")==0){    // Maps range unavailable to player in select screen.
                newchar->hmap1 = atoi(findarg(buf+pos, 1)); //First unavailable map.
                newchar->hmap2 = atoi(findarg(buf+pos, 2)); //Last unavailable map.
            }
            else if(stricmp(command, "setlayer")==0){
                newchar->setlayer = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "toflip")==0){    // Flag to determine if flashes images will be flipped or not
                newchar->toflip = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "nodieblink")==0){    // Added to determine if dying animation blinks or not
                newchar->nodieblink = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "noatflash")==0){    // Flag to determine if an opponents attack spawns their flash or not
                newchar->noatflash = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "nomove")==0){    // If set, will be static (speed must be set to 0 or left blank)
                newchar->nomove = atoi(findarg(buf+pos, 1));
                newchar->noflip = atoi(findarg(buf+pos, 2));    // If set, static will not flip directions
                if(newchar->nomove) newchar->nodrop = 1;
            }
            else if(stricmp(command, "nodrop")==0){
                newchar->nodrop = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "thold")==0){    // Threshold for enemies/players block
                newchar->thold = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "running")==0){    // The speed at which the player runs
                newchar->runspeed = atof(findarg(buf+pos, 1));
                newchar->runspeed /= 10;
                newchar->runjumpheight = atof(findarg(buf+pos, 2));    // The height at which a player jumps when running
                newchar->runjumpdist = atof(findarg(buf+pos, 3));    // The distance a player jumps when running
                newchar->runupdown = atoi(findarg(buf+pos, 4));
                newchar->runhold = atoi(findarg(buf+pos, 5));
            }
            else if(stricmp(command, "blockodds")==0){    // Odds that an attack will hit an enemy (1 : blockodds)
                newchar->blockodds = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "holdblock")==0){
                newchar->holdblock = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "blockpain")==0){
                newchar->blockpain = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "nopassiveblock")==0){
                newchar->nopassiveblock = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "edelay")==0){
                newchar->edelay.mode        = atoi(findarg(buf+pos, 1));
                newchar->edelay.factor      = atof(findarg(buf+pos, 2));
                newchar->edelay.cap_min     = atoi(findarg(buf+pos, 3));
                newchar->edelay.cap_max     = atoi(findarg(buf+pos, 4));
                newchar->edelay.range_min   = atoi(findarg(buf+pos, 5));
                newchar->edelay.range_max   = atoi(findarg(buf+pos, 6));
            }
            else if(stricmp(command, "paingrab")==0){
                newchar->paingrab = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "throw")==0){
                newchar->throwdist = atof(findarg(buf+pos, 1));
                newchar->throwheight = atof(findarg(buf+pos, 2));
            }
            else if(stricmp(command, "grabwalk")==0){
                newchar->grabwalkspeed = atof(findarg(buf+pos, 1));
                newchar->grabwalkspeed /= 10;

                if(newchar->grabwalkspeed < 0.5) newchar->grabwalkspeed = 0.5;
            }
            else if(stricmp(command, "grabturn")==0){
                newchar->grabturn = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "throwframewait")==0){
                newchar->throwframewait = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "diesound")==0){
                newchar->diesound = sound_load_sample(findarg(buf+pos, 1), packfile, 0);
            }
            else if(stricmp(command, "icon")==0){
                value = findarg(buf+pos, 1);

                if(newchar->icon > -1) shutdown(1, "Error: model '%s' has multiple icons defined", filename);

                newchar->icon = loadsprite(value,0,0,pixelformat); //use same palette as the owner
                newchar->iconpain = newchar->icon;
                newchar->icondie = newchar->icon;
                newchar->iconget = newchar->icon;
            }
            else if(stricmp(command, "iconpain")==0){    // 20-1-2005 New icons here
                value = findarg(buf+pos, 1);
                newchar->iconpain = loadsprite(value,0,0,pixelformat);
            }
            else if(stricmp(command, "icondie")==0){
                value = findarg(buf+pos, 1);
                newchar->icondie = loadsprite(value,0,0,pixelformat);
            }
            else if(stricmp(command, "iconget")==0){
                value = findarg(buf+pos, 1);
                newchar->iconget = loadsprite(value,0,0,pixelformat);
            }                                // 20-1-2005 New icons finish here
            else if(stricmp(command, "iconw")==0){
                value = findarg(buf+pos, 1);
                newchar->iconw = loadsprite(value,0,0,pixelformat);
            }
            else if(stricmp(command, "iconmphigh")==0){
                value = findarg(buf+pos, 1);
                newchar->iconmp[0] = loadsprite(value,0,0,pixelformat);
            }
            else if(stricmp(command, "iconmphalf")==0){
                value = findarg(buf+pos, 1);
                newchar->iconmp[1] = loadsprite(value,0,0,pixelformat);
            }
            else if(stricmp(command, "iconmplow")==0){
                value = findarg(buf+pos, 1);
                newchar->iconmp[2] = loadsprite(value,0,0,pixelformat);
            }
            else if(stricmp(command, "parrow")==0){    // Image that is displayed when player 1 spawns invincible
                value = findarg(buf+pos, 1);
                newchar->parrow[0][0] = loadsprite(value,0,0,pixelformat);
                newchar->parrow[0][1] = atoi(findarg(buf+pos, 2));
                newchar->parrow[0][2] = atoi(findarg(buf+pos, 3));
            }
            else if(stricmp(command, "parrow2")==0){    // Image that is displayed when player 2 spawns invincible
                value = findarg(buf+pos, 1);
                newchar->parrow[1][0] = loadsprite(value,0,0,pixelformat);
                newchar->parrow[1][1] = atoi(findarg(buf+pos, 2));
                newchar->parrow[1][2] = atoi(findarg(buf+pos, 3));
            }
            else if(stricmp(command, "parrow3")==0){    // Image that is displayed when player 3 spawns invincible
                value = findarg(buf+pos, 1);
                newchar->parrow[2][0] = loadsprite(value,0,0,pixelformat);
                newchar->parrow[2][1] = atoi(findarg(buf+pos, 2));
                newchar->parrow[2][2] = atoi(findarg(buf+pos, 3));
            }
            else if(stricmp(command, "parrow4")==0){    // Image that is displayed when player 4 spawns invincible
                value = findarg(buf+pos, 1);
                newchar->parrow[3][0] = loadsprite(value,0,0,pixelformat);
                newchar->parrow[3][1] = atoi(findarg(buf+pos, 2));
                newchar->parrow[3][2] = atoi(findarg(buf+pos, 3));
            }
            else if(stricmp(command, "atchain")==0)
            {
                newchar->chainlength = 0;
                for(i = 0; i < MAX_ATCHAIN; i++)
                {
                    newchar->atchain[i] = atoi(findarg(buf+pos, i + 1));
                    if(newchar->atchain[i] < 0) newchar->atchain[i] = 0;
                    if(newchar->atchain[i] > max_attacks) newchar->atchain[i] = max_attacks;
                    if(newchar->atchain[i]) newchar->chainlength = i+1;
                }
            }
            else if(stricmp(command, "combostyle")==0)
            {
                newchar->combostyle = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "credit")==0){
                newchar->credit = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "nopain")==0){
                newchar->nopain = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "escapehits")==0){      // How many times an enemy can be hit before retaliating
                newchar->escapehits = atoi(findarg(buf+pos, 1));
            }
            //Left for backward compatability. See Mpset.
            else if(stricmp(command, "chargerate")==0){      // How much mp does this character gain while recharging?
                newchar->chargerate = atoi(findarg(buf+pos, 1));
            }
            //Left for backward compatability. See Mpset.
            else if(stricmp(command, "mprate")==0){
                newchar->mprate = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "mpset")==0){    // Mp bar wax/wane.
                newchar->mp             = atoi(findarg(buf+pos, 1)); //Max MP.
                newchar->mpstable       = atoi(findarg(buf+pos, 2)); //MP stable setting.
                newchar->mpstableval    = atoi(findarg(buf+pos, 3)); //MP stable value (% Mp bar will try and maintain).
                newchar->mprate         = atoi(findarg(buf+pos, 4)); //Rate MP value rises over time.
                newchar->mpdroprate     = atoi(findarg(buf+pos, 5)); //Rate MP value drops over time.
                newchar->chargerate     = atoi(findarg(buf+pos, 6)); //MP Chargerate.
            }
            else if(stricmp(command, "sleepwait")==0){
                newchar->sleepwait = atoi(findarg(buf+pos, 1));
            }
			else if(stricmp(command, "guardrate")==0){
                newchar->guardrate = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "aggression")==0){
                newchar->aggression = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "risetime")==0){
                newchar->risetime[0] = atoi(findarg(buf+pos, 1));
				newchar->risetime[1] = atoi(findarg(buf+pos, 2));
            }
            else if(stricmp(command, "facing")==0){
                newchar->facing = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "turndelay")==0){
                newchar->turndelay = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "lifespan")==0){
                newchar->lifespan = atof(findarg(buf+pos, 1))*GAME_SPEED;
            }
            else if(stricmp(command, "summonkill")==0){
                newchar->summonkill = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "lifeposition")==0){
                if((value=findarg(buf+pos, 1))[0]) newchar->hpx = atoi(value);
                if((value=findarg(buf+pos, 2))[0]) newchar->hpy = atoi(value);
            }
            else if(stricmp(command, "lifebarstatus")==0)
            {
				_readbarstatus(buf+pos, &(newchar->hpbarstatus));
				newchar->hpbarstatus.colourtable = &hpcolourtable;
            }
            else if(stricmp(command, "iconposition")==0){
                if((value=findarg(buf+pos, 1))[0]) newchar->iconx = atoi(value);
                if((value=findarg(buf+pos, 2))[0]) newchar->icony = atoi(value);
            }
            else if(stricmp(command, "nameposition")==0){
                if((value=findarg(buf+pos, 1))[0]) newchar->namex = atoi(value);
                if((value=findarg(buf+pos, 2))[0]) newchar->namey = atoi(value);
            }

            // Section for custom freespecials starts here

            else if(stricmp(command, "com")==0){
                int i;
                int t;
                for(i = 0, t = 1; i < MAX_SPECIAL_INPUTS-3; i++, t++)
                {
                    value = findarg(buf+pos, t);
                    if(!value[0]) break;
                    if(stricmp(value, "u")==0){
                        newchar->special[newchar->specials_loaded][i] = FLAG_MOVEUP;
                    }
                    else if(stricmp(value, "d")==0){
                        newchar->special[newchar->specials_loaded][i] = FLAG_MOVEDOWN;
                    }
                    else if(stricmp(value, "f")==0){
                        newchar->special[newchar->specials_loaded][i] = FLAG_FORWARD;
                    }
                    else if(stricmp(value, "b")==0){
                        newchar->special[newchar->specials_loaded][i] = FLAG_BACKWARD;
                    }
                    else if(stricmp(value, "a")==0){
                        newchar->special[newchar->specials_loaded][i] = FLAG_ATTACK;
                    }
					else if(stricmp(value, "a2")==0){
                        newchar->special[newchar->specials_loaded][i] = FLAG_ATTACK2;
                    }
					else if(stricmp(value, "a3")==0){
                        newchar->special[newchar->specials_loaded][i] = FLAG_ATTACK3;
                    }
					else if(stricmp(value, "a4")==0){
                        newchar->special[newchar->specials_loaded][i] = FLAG_ATTACK4;
                    }
                    else if(stricmp(value, "j")==0){
                        newchar->special[newchar->specials_loaded][i] = FLAG_JUMP;
                    }
                    else if(stricmp(value, "s")==0){
                        newchar->special[newchar->specials_loaded][i] = FLAG_SPECIAL;
                    }
                    else if(stricmp(value, "k")==0){
                        newchar->special[newchar->specials_loaded][i] = FLAG_SPECIAL;
                    }
					else if(strnicmp(value, "freespecial", 11)==0 && (!value[11] || (value[11] >= '1' && value[11] <= '9'))){
						tempInt = atoi(value+11);
                        if(tempInt<1) tempInt = 1;
						newchar->special[newchar->specials_loaded][MAX_SPECIAL_INPUTS-2] = animspecials[tempInt-1];
                    }
                    else shutdown(1, "Invalid freespecial command '%s'", value);
                }
                newchar->special[newchar->specials_loaded][MAX_SPECIAL_INPUTS-3]=i-1; // max steps
                newchar->specials_loaded++;
				if(newchar->specials_loaded > max_freespecials) shutdown(1, "Too many Freespecials and/or Cancels. Please increase Maxfreespecials", value); // OX. This is to catch freespecials that use same animation.
            }

            // End section for custom freespecials

            // This command should not be used under 24bit mode, but for old mods, just give it a default palette
            else if(stricmp(command, "remap")==0){
                value = findarg(buf+pos, 1);
                value2 = findarg(buf+pos, 2);
                errorVal = load_colourmap(newchar, value, value2);
                if(pixelformat==PIXEL_x8 && newchar->palette==NULL)
                {
                    newchar->palette = tracemalloc("newchar#remap#palette", PAL_BYTES);
                    memcpy(newchar->palette, pal, PAL_BYTES);
                }
                mapflag[newchar->maps_loaded-1] = 1;
                if(!errorVal){
                    switch(errorVal){
                        case 0:
                            shutdown(1, "Failed to create colourmap from images\n\t'%s'\nand\n\t'%s'. Image Used Twice!", value, value2);
                            break;
                        case -1:
                            shutdown(1, "Failed to create colourmap from images\n\t'%s'\nand\n\t'%s'. MAX_COLOUR_MAPS full!", value, value2);
                            break;
                        case -2:
                            shutdown(1, "Failed to create colourmap from images\n\t'%s'\nand\n\t'%s'. Failed to tracemalloc(256)!", value, value2);
                            break;
                        case -3:
                            shutdown(1, "Failed to create colourmap from images\n\t'%s'\nand\n\t'%s'. Failed to create bitmap1", value, value2);
                            break;
                        case -4:
                            shutdown(1, "Failed to create colourmap from images\n\t'%s'\nand\n\t'%s'. Failed to create bitmap2", value, value2);
                            break;
                    }
                }
            }
            // main palette for the entity under 24bit mode
            else if(stricmp(command, "palette")==0)
            {
                if(pixelformat!=PIXEL_x8) printf("Warning: command '%s' is not available under 8bit mode\n", command);
                else if(newchar->palette==NULL)
                {
                    value = findarg(buf+pos, 1);
                    newchar->palette = tracemalloc("newchar->palette", PAL_BYTES);
                    if(loadimagepalette(value, packfile, newchar->palette)==0)
                        shutdown(1, "Failed to load palette '%s' for '%s'", value, filename);
                }
            }
            // remap for the entity under 24bit mode, this method can replace remap command
            else if(stricmp(command, "alternatepal")==0)
            {
                if(pixelformat!=PIXEL_x8) printf("Warning: command '%s' is not available under 8bit mode\n", command);
                else if(newchar->maps_loaded<MAX_COLOUR_MAPS)
                {
                    value = findarg(buf+pos, 1);
                    newchar->colourmap[(int)newchar->maps_loaded] = tracemalloc("newchar#alternatepal", PAL_BYTES);
                    if(loadimagepalette(value, packfile, newchar->colourmap[(int)newchar->maps_loaded])==0)
                        shutdown(1, "Failed to load palette '%s' for '%s'", value, filename);
                    newchar->maps_loaded++;
                }
            }
            // use global palette under 24bit mode, so some entity/panel/bg can still use palette feature, that saves some memory
            else if(stricmp(command, "globalmap")==0)
            {
                if(pixelformat!=PIXEL_x8) printf("Warning: command '%s' is not available under 8bit mode\n", command);
                else newchar->globalmap = atoi(findarg(buf+pos, 1));
            }
            //new alpha effects by tails
            else if(stricmp(command, "alpha")==0){
                newchar->alpha = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "remove")==0){
                newchar->remove = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "script")==0 ){//load the update script
                Script_Init(newchar->update_script, "updateentityscript", 0);
                if(load_script(newchar->update_script, findarg(buf+pos, 1)))
                    Script_Compile(newchar->update_script);
                else shutdown(1, "Unable to load entity update script '%s' in file '%s'.\n", findarg(buf+pos, 1), filename);
            }
			else if(stricmp(command, "thinkscript")==0){//load the think script
                Script_Init(newchar->think_script, "thinkscript", 0);
                if(load_script(newchar->think_script, findarg(buf+pos, 1)))
                    Script_Compile(newchar->think_script);
                else  shutdown(1, "Unable to load think script '%s' in file '%s'.\n", findarg(buf+pos, 1), filename);
            }
            else if(stricmp(command, "takedamagescript")==0){//load the takedamage script
                Script_Init(newchar->takedamage_script, "takedamagescript", 0);
                if(load_script(newchar->takedamage_script, findarg(buf+pos, 1)))
                    Script_Compile(newchar->takedamage_script);
                else shutdown(1, "Unable to load takedamage script '%s' in file '%s'.\n", findarg(buf+pos, 1), filename);
            }
            else if(stricmp(command, "onfallscript")==0){//load the onfall script
                Script_Init(newchar->onfall_script, "onfallscript", 0);
                if(load_script(newchar->onfall_script , findarg(buf+pos, 1)))
                    Script_Compile(newchar->onfall_script);
                else shutdown(1, "Unable to load onfall script '%s' in file '%s'.\n", findarg(buf+pos, 1), filename);
            }
            else if(stricmp(command, "onpainscript")==0){//load the onpain script
                Script_Init(newchar->onpain_script, "onpainscript", 0);
                if(load_script(newchar->onpain_script , findarg(buf+pos, 1)))
                    Script_Compile(newchar->onpain_script);
                else shutdown(1, "Unable to load onpain script '%s' in file '%s'.\n", findarg(buf+pos, 1), filename);
            }
            else if(stricmp(command, "onblocksscript")==0){//load the onblocks script
                Script_Init(newchar->onblocks_script, "onblocksscript", 0);
                if(load_script(newchar->onblocks_script , findarg(buf+pos, 1)))
                    Script_Compile(newchar->onblocks_script);
                else shutdown(1, "Unable to load onblocks script '%s' in file '%s'.\n", findarg(buf+pos, 1), filename);
            }
            else if(stricmp(command, "onblockwscript")==0){//load the onblockw script
                Script_Init(newchar->onblockw_script, "onblockwscript", 0);
                if(load_script(newchar->onblockw_script , findarg(buf+pos, 1)))
                    Script_Compile(newchar->onblockw_script);
                else shutdown(1, "Unable to load onblockw script '%s' in file '%s'.\n", findarg(buf+pos, 1), filename);
            }
            else if(stricmp(command, "onblockoscript")==0){//load the onblocko script
                Script_Init(newchar->onblocko_script, "onblockoscript", 0);
                if(load_script(newchar->onblocko_script , findarg(buf+pos, 1)))
                    Script_Compile(newchar->onblocko_script);
                else shutdown(1, "Unable to load onblocko script '%s' in file '%s'.\n", findarg(buf+pos, 1), filename);
            }
            else if(stricmp(command, "onblockzscript")==0){//load the onblockz script
                Script_Init(newchar->onblockz_script, "onblockzscript", 0);
                if(load_script(newchar->onblockz_script , findarg(buf+pos, 1)))
                    Script_Compile(newchar->onblockz_script);
                else shutdown(1, "Unable to load onblockz script '%s' in file '%s'.\n", findarg(buf+pos, 1), filename);
            }
            else if(stricmp(command, "onblockascript")==0){//load the onblocka script
                Script_Init(newchar->onblocka_script, "onblockascript", 0);
                if(load_script(newchar->onblocka_script , findarg(buf+pos, 1)))
                    Script_Compile(newchar->onblocka_script);
                else shutdown(1, "Unable to load onblocka script '%s' in file '%s'.\n", findarg(buf+pos, 1), filename);
            }
            else if(stricmp(command, "onmovexscript")==0){//load the onmovex script
                Script_Init(newchar->onmovex_script, "onmovexscript", 0);
                if(load_script(newchar->onmovex_script , findarg(buf+pos, 1)))
                    Script_Compile(newchar->onmovex_script);
                else shutdown(1, "Unable to load onmovex script '%s' in file '%s'.\n", findarg(buf+pos, 1), filename);
            }
            else if(stricmp(command, "onmovezscript")==0){//load the onmovez script
                Script_Init(newchar->onmovez_script, "onmovezscript", 0);
                if(load_script(newchar->onmovez_script , findarg(buf+pos, 1)))
                    Script_Compile(newchar->onmovez_script);
                else shutdown(1, "Unable to load onmovez script '%s' in file '%s'.\n", findarg(buf+pos, 1), filename);
            }
            else if(stricmp(command, "onmoveascript")==0){//load the onmovea script
                Script_Init(newchar->onmovea_script, "onmoveascript", 0);
                if(load_script(newchar->onmovea_script , findarg(buf+pos, 1)))
                    Script_Compile(newchar->onmovea_script);
                else shutdown(1, "Unable to load onmovea script '%s' in file '%s'.\n", findarg(buf+pos, 1), filename);
            }
            else if(stricmp(command, "ondeathscript")==0){//load the death script
                Script_Init(newchar->ondeath_script, "ondeathscript", 0);
                if(load_script(newchar->ondeath_script, findarg(buf+pos, 1)))
                    Script_Compile(newchar->ondeath_script);
                else shutdown(1, "Unable to load ondeath script '%s' in file '%s'.\n", findarg(buf+pos, 1), filename);
            }
            else if(stricmp(command, "onkillscript")==0){//load the kill script
                Script_Init(newchar->onkill_script, "onkillscript", 0);
                if(load_script(newchar->onkill_script, findarg(buf+pos, 1)))
                    Script_Compile(newchar->onkill_script);
                else shutdown(1, "Unable to load onkill script '%s' in file '%s'.\n", findarg(buf+pos, 1), filename);
            }
            else if(stricmp(command, "didblockscript")==0){//load the didblock script
                Script_Init(newchar->didblock_script, "didblockscript", 0);
                if(load_script(newchar->didblock_script, findarg(buf+pos, 1)))
                    Script_Compile(newchar->didblock_script);
                else shutdown(1, "Unable to load didblock script '%s' in file '%s'.\n", findarg(buf+pos, 1), filename);
            }
			else if(stricmp(command, "ondoattackscript")==0){//load the checkhit script
                Script_Init(newchar->ondoattack_script, "ondoattackscript", 0);
                if(load_script(newchar->ondoattack_script, findarg(buf+pos, 1)))
                    Script_Compile(newchar->ondoattack_script);
                else shutdown(1, "Unable to load ondoattack script '%s' in file '%s'.\n", findarg(buf+pos, 1), filename);
            }
            else if(stricmp(command, "didhitscript")==0){//load the didhit script
                Script_Init(newchar->didhit_script, "didhitscript", 0);
                if(load_script(newchar->didhit_script, findarg(buf+pos, 1)))
                    Script_Compile(newchar->didhit_script);
                else  shutdown(1, "Unable to load didhit script '%s' in file '%s'.\n", findarg(buf+pos, 1), filename);
            }
            else if(stricmp(command, "onspawnscript")==0){//load the onspawn script
                Script_Init(newchar->onspawn_script, "onspawnscript", 0);
                if(load_script(newchar->onspawn_script, findarg(buf+pos, 1)))
                    Script_Compile(newchar->onspawn_script);
                else  shutdown(1, "Unable to load onspawn script '%s' in file '%s'.\n", findarg(buf+pos, 1), filename);
            }
            else if(stricmp(command, "animationscript")==0){//load the animation script
                Script_Init(newchar->animation_script, "animationscript", 0);
                if(!load_script(newchar->animation_script, findarg(buf+pos, 1)))
                    shutdown(1, "Unable to load animation script '%s' in file '%s'.\n", findarg(buf+pos, 1), filename);
                //dont compile, until at end of this function
            }
            else if(stricmp(command, "keyscript")==0){//load the key script
                Script_Init(newchar->key_script, "entitykeyscript", 0);
                if(load_script(newchar->key_script, findarg(buf+pos, 1)))
                    Script_Compile(newchar->key_script);
                else  shutdown(1, "Unable to load key script '%s' in file '%s'.\n", findarg(buf+pos, 1), filename);
            }
            else if(stricmp(command, "anim")==0){
                value = findarg(buf+pos, 1);
                frameset = 0;
                framecount = 0;
                // Create new animation
                newanim = alloc_anim();
                if(newanim==NULL){
                    if(buf != NULL){
                        tracefree(buf);
                        buf = NULL;
                    }
                    if(scriptbuf){
                        tracefree(scriptbuf);
                        scriptbuf = NULL;
                    }
                    shutdown(1, "Not enough memory for animations!");
                }
                newanim->model_index = newchar->index;
                // Reset vars
                curframe = 0;
                memset(bbox, 0, sizeof(bbox));
                memset(abox, 0, sizeof(abox));
                memset(offset, 0, sizeof(offset));
                memset(shadow_coords, 0, sizeof(shadow_coords));
                memset(shadow_xz, 0, sizeof(shadow_xz));
                memset(platform, 0, sizeof(platform));
                shadow_set = 0;
				attack = emptyattack;
				attack.hitsound = -1;
				attack.hitflash = -1;
				attack.blockflash = -1;
				attack.blocksound = -1;
				drawmethod = plainmethod;
				idle = 0;
                move = 0;
                movez = 0;
                movea = 0;
                seta = -1;
                frameshadow = -1;
                soundtoplay = -1;

                if(!newanim->range[0]) newanim->range[0] = -10;
				newanim->range[1] = (int)newchar->jumpheight*20;    //30-12-2004 default range affected by jump height
				newanim->range[2] = (int)-newchar->grabdistance/3;  //zmin
				newanim->range[3] = (int)newchar->grabdistance/3;   //zmax
				newanim->range[4] = -1000;                          //amin
				newanim->range[5] = 1000;                           //amax
                newanim->range[6] = -1000;                          //Base min.
                newanim->range[7] = 1000;                           //Base max.

                newanim->jumpv = 0;    // Default disabled
                newanim->fastattack = 0;
                newanim->energycost[1] = 0;							//MP only.
				newanim->energycost[2] = 0;							//Disable flag.
                newanim->chargetime = 2;			// Default for backwards compatibility
				newanim->shootframe = -1;
                newanim->throwframe = -1;
                newanim->tossframe = -1;			// this get 1 of weapons numshots shots in the animation that you want(normaly the last)by tails
                newanim->jumpframe = -1;
                newanim->flipframe = -1;
                newanim->attackone = -1;
				newanim->dive[0] = newanim->dive[1] = 0;
                newanim->followanim = 0;			// Default disabled
                newanim->followcond = 0;
                newanim->counterframe[0] = -1;		//Start frame.
				newanim->counterframe[1] = -1;		//End frame.
				newanim->counterframe[2] = 0;		//Counter cond.
				newanim->counterframe[3] = 0;		//Counter damage.
                newanim->unsummonframe = -1;
                newanim->landframe[0] = -1;
                newanim->dropframe = -1;
				newanim->cancel = 0;  // OX. For cancelling anims into a freespecial. 0 by default , 3 when enabled. IMPORTANT!! Must stay as it is!
				newanim->animhits = 0; //OX counts hits on a per anim basis for cancels.
                newanim->subentity = newanim->custbomb = newanim->custknife = newanim->custstar = newanim->custpshotno = -1;
				memset(newanim->quakeframe, 0, sizeof(newanim->quakeframe));

                if(strnicmp(value, "idle", 4)==0 &&
					(!value[4] || (value[4] >= '1' && value[4]<='9'))){
                    tempInt = atoi(value+4);
                    if(tempInt<1) tempInt = 1;
                    ani_id = animidles[tempInt-1];
                }
                else if(stricmp(value, "waiting")==0){
                    ani_id = ANI_SELECT;
                }
                else if(strnicmp(value, "walk", 4)==0 &&
					(!value[4] || (value[4] >= '1' && value[4]<='9'))){
                    tempInt = atoi(value+4);
                    if(tempInt<1) tempInt = 1;
                    ani_id = animwalks[tempInt-1];
                }
                else if(stricmp(value, "sleep")==0){
                    ani_id = ANI_SLEEP;
                }
                else if(stricmp(value, "run")==0){
                    ani_id = ANI_RUN;
                }
                else if(strnicmp(value, "up", 2)==0 &&
					(!value[2] || (value[2] >= '1' && value[2]<='9'))){
                    tempInt = atoi(value+2);
                    if(tempInt<1) tempInt = 1;
                    ani_id = animups[tempInt-1];
                }
                else if(strnicmp(value, "down", 4)==0 &&
					(!value[4] || (value[4] >= '1' && value[4]<='9'))){
                    tempInt = atoi(value+4);
                    if(tempInt<1) tempInt = 1;
                    ani_id = animdowns[tempInt-1];
                }
                else if(strnicmp(value, "backwalk", 8)==0 &&
					(!value[8] || (value[8] >= '1' && value[8]<='9'))){
                    tempInt = atoi(value+8);
                    if(tempInt<1) tempInt = 1;
                    ani_id = animbackwalks[tempInt-1];
                }
                else if(stricmp(value, "jump")==0){
                    ani_id = ANI_JUMP;
                    newanim->range[0] = 50;    // Used for enemies that jump on walls
                    newanim->range[1] = 60;    // Used for enemies that jump on walls
                }
                else if(stricmp(value, "duck")==0){
                    ani_id = ANI_DUCK;
                }
                else if(stricmp(value, "land")==0){
                    ani_id = ANI_LAND;
                }
                else if(stricmp(value, "pain")==0){
                    ani_id = ANI_PAIN;
                }
                else if(strnicmp(value, "pain", 4)==0 && value[4]>='1' && value[4]<='9'){
					tempInt = atoi(value+4);
	                if(tempInt==1){
						ani_id = ANI_PAIN;
					}
					else if(tempInt==2){
						ani_id = ANI_PAIN2;
					}
					else if(tempInt==3){
						ani_id = ANI_PAIN3;
					}
					else if(tempInt==4){
						ani_id = ANI_PAIN4;
					}
					else if(tempInt==5){
						ani_id = ANI_PAIN5;
					}
					else if(tempInt==6){
						ani_id = ANI_PAIN6;
					}
					else if(tempInt==7){
						ani_id = ANI_PAIN7;
					}
					else if(tempInt==8){
						ani_id = ANI_PAIN8;
					}
					else if(tempInt==9){
						ani_id = ANI_PAIN9;
					}
					else if(tempInt==10){
						ani_id = ANI_PAIN10;
					}
					else{
                        if(tempInt<MAX_ATKS-STA_ATKS+1) tempInt = MAX_ATKS-STA_ATKS+1;
	                    ani_id = animpains[tempInt+STA_ATKS-1];
					}
                }
                else if(stricmp(value, "spain")==0){    // If shock attacks don't knock opponent down, play this
                    ani_id = ANI_SHOCKPAIN;
                }
                else if(stricmp(value, "bpain")==0){    // If burn attacks don't knock opponent down, play this
                    ani_id = ANI_BURNPAIN;
                }
                else if(stricmp(value, "fall")==0){
                    ani_id = ANI_FALL;   // If no new animation, load fall animation into both "respawn" & "fall"
                    newanim->bounce = 4;
                }
                else if(strnicmp(value, "fall", 4)==0 && value[4]>='1' && value[4]<='9'){
                    tempInt = atoi(value+4);
					if(tempInt==1){
						ani_id = ANI_FALL;
					}
					else if(tempInt==2){
						ani_id = ANI_FALL2;
					}
					else if(tempInt==3){
						ani_id = ANI_FALL3;
					}
					else if(tempInt==4){
						ani_id = ANI_FALL4;
					}
					else if(tempInt==5){
						ani_id = ANI_FALL5;
					}
					else if(tempInt==6){
						ani_id = ANI_FALL6;
					}
					else if(tempInt==7){
						ani_id = ANI_FALL7;
					}
					else if(tempInt==8){
						ani_id = ANI_FALL8;
					}
					else if(tempInt==9){
						ani_id = ANI_FALL9;
					}
					else if(tempInt==10){
						ani_id = ANI_FALL10;
					}
					else{
						if(tempInt<MAX_ATKS-STA_ATKS+1) tempInt = MAX_ATKS-STA_ATKS+1;
						ani_id = animfalls[tempInt+STA_ATKS-1];
					}
                    newanim->bounce = 4;
                }
                else if(stricmp(value, "shock")==0){    // If shock attacks do knock opponent down, play this
                    ani_id = ANI_SHOCK;
                    newanim->bounce = 4;
                }
                else if(stricmp(value, "burn")==0){    // If burn attacks do knock opponent down, play this
                    ani_id = ANI_BURN;
                    newanim->bounce = 4;
                }
                else if(stricmp(value, "death")==0){      //  If new animation is present, overwrite new_anim with it.
                    ani_id = ANI_DIE;
                }
                else if(strnicmp(value, "death", 5)==0 && value[5]>='1' && value[5]<='9'){
                    tempInt = atoi(value+5);
					if(tempInt==1){
						ani_id = ANI_DIE;
					}
					else if(tempInt==2){
						ani_id = ANI_DIE2;
					}
					else if(tempInt==3){
						ani_id = ANI_DIE3;
					}
					else if(tempInt==4){
						ani_id = ANI_DIE4;
					}
					else if(tempInt==5){
						ani_id = ANI_DIE5;
					}
					else if(tempInt==6){
						ani_id = ANI_DIE6;
					}
					else if(tempInt==7){
						ani_id = ANI_DIE7;
					}
					else if(tempInt==8){
						ani_id = ANI_DIE8;
					}
					else if(tempInt==9){
						ani_id = ANI_DIE9;
					}
					else if(tempInt==10){
						ani_id = ANI_DIE10;
					}
					else{
                        if(tempInt<MAX_ATKS-STA_ATKS+1) tempInt = MAX_ATKS-STA_ATKS+1;
						ani_id = animdies[tempInt+STA_ATKS-1];
					}
                }
                else if(stricmp(value, "sdie")==0){
                    ani_id = ANI_SHOCKDIE;
                }
                else if(stricmp(value, "bdie")==0){
                    ani_id = ANI_BURNDIE;
                }
				else if(stricmp(value, "chipdeath")==0){
                    ani_id = ANI_CHIPDEATH;
                }
				else if(stricmp(value, "guardbreak")==0){
                    ani_id = ANI_GUARDBREAK;
                }
				else if(stricmp(value, "riseb")==0){
                    ani_id = ANI_RISEB;
                }


				else if(stricmp(value, "rises")==0){
                    ani_id = ANI_RISES;
                }
                else if(stricmp(value, "rise")==0){
                    ani_id = ANI_RISE;   // If no new animation, load fall animation into both "respawn" & "fall"
                }
                else if(strnicmp(value, "rise", 4)==0 && value[4]>='1' && value[4]<='9'){
                    tempInt = atoi(value+4);
					if(tempInt==1){
						ani_id = ANI_RISE;
					}
					else if(tempInt==2){
						ani_id = ANI_RISE2;
					}
					else if(tempInt==3){
						ani_id = ANI_RISE3;
					}
					else if(tempInt==4){
						ani_id = ANI_RISE4;
					}
					else if(tempInt==5){
						ani_id = ANI_RISE5;
					}
					else if(tempInt==6){
						ani_id = ANI_RISE6;
					}
					else if(tempInt==7){
						ani_id = ANI_RISE7;
					}
					else if(tempInt==8){
						ani_id = ANI_RISE8;
					}
					else if(tempInt==9){
						ani_id = ANI_RISE9;
					}
					else if(tempInt==10){
						ani_id = ANI_RISE10;
					}
					else{
						if(tempInt<MAX_ATKS-STA_ATKS+1) tempInt = MAX_ATKS-STA_ATKS+1;
						ani_id = animrises[tempInt+STA_ATKS-1];
					}
                }
				else if(stricmp(value, "riseattackb")==0){
                    ani_id = ANI_RISEATTACKB;
                }
				else if(stricmp(value, "riseattacks")==0){
                    ani_id = ANI_RISEATTACKS;
                }
                else if(stricmp(value, "riseattack")==0){
                    ani_id = ANI_RISEATTACK;   // If no new animation, load fall animation into both "respawn" & "fall"
                }
                else if(strnicmp(value, "riseattack", 10)==0 && value[10]>='1' && value[10]<='9'){
                    tempInt = atoi(value+10);
					if(tempInt==1){
						ani_id = ANI_RISEATTACK;
					}
					else if(tempInt==2){
						ani_id = ANI_RISEATTACK2;
					}
					else if(tempInt==3){
						ani_id = ANI_RISEATTACK3;
					}
					else if(tempInt==4){
						ani_id = ANI_RISEATTACK4;
					}
					else if(tempInt==6){
						ani_id = ANI_RISEATTACK5;
					}
					else if(tempInt==6){
						ani_id = ANI_RISEATTACK6;
					}
					else if(tempInt==7){
						ani_id = ANI_RISEATTACK7;
					}
					else if(tempInt==8){
						ani_id = ANI_RISEATTACK8;
					}
					else if(tempInt==9){
						ani_id = ANI_RISEATTACK9;
					}
					else if(tempInt==10){
						ani_id = ANI_RISEATTACK10;
					}
					else{
						if(tempInt<MAX_ATKS-STA_ATKS+1) tempInt = MAX_ATKS-STA_ATKS+1;
						ani_id = animriseattacks[tempInt+STA_ATKS-1];
					}
                }
                else if(stricmp(value, "select")==0){
                    ani_id = ANI_PICK;
                }
                else if(strnicmp(value, "attack", 6)==0 &&
                        (!value[6] || (value[6] >= '1' && value[6]<='9')))
                {
                    tempInt = atoi(value+6);
                    if(tempInt<1) tempInt = 1;
                    ani_id = animattacks[tempInt-1];
                }
                else if(stricmp(value, "throwattack")==0){
                    ani_id = ANI_THROWATTACK;
                }
                else if(stricmp(value, "upper")==0){
                    ani_id = ANI_UPPER;
                    attack.counterattack = 100; //default to 100
                    newanim->range[0] = -10;
                    newanim->range[1] = 120;
                }
                else if(stricmp(value, "cant")==0){
                    ani_id = ANI_CANT;
                }
                else if(stricmp(value, "jumpcant")==0){
                    ani_id = ANI_JUMPCANT;
                }
                else if(stricmp(value, "charge")==0){
                    ani_id = ANI_CHARGE;
                }
                else if(stricmp(value, "faint")==0){
                    ani_id = ANI_FAINT;
                }
                else if(stricmp(value, "dodge")==0){
                    ani_id = ANI_DODGE;
                }
                else if(stricmp(value, "special")==0 || stricmp(value, "special1")==0){
                    ani_id = ANI_SPECIAL;
                    newanim->energycost[0] = 6;
                }
                else if(stricmp(value, "special2")==0){
                    ani_id = ANI_SPECIAL2;
                }
                else if(stricmp(value, "special3")==0 || stricmp(value, "jumpspecial")==0){
                    ani_id = ANI_JUMPSPECIAL;
                }
				else if(strnicmp(value, "freespecial", 11)==0 && (!value[11] || (value[11] >= '1' && value[11] <= '9'))){
					tempInt = atoi(value+11);
                    if(tempInt<1) tempInt = 1;
                    ani_id = animspecials[tempInt-1];
                    switch(tempInt) // old default values
                    {
                    case 1:
                        if(!is_set(newchar, ANI_FREESPECIAL))
                        {
                            newchar->special[newchar->specials_loaded][0] = FLAG_FORWARD;
                            newchar->special[newchar->specials_loaded][1] = FLAG_FORWARD;
                            newchar->special[newchar->specials_loaded][2] = FLAG_ATTACK;
                            newchar->special[newchar->specials_loaded][MAX_SPECIAL_INPUTS-2] = ANI_FREESPECIAL;
                            newchar->special[newchar->specials_loaded][MAX_SPECIAL_INPUTS-3] = 3;
                            newchar->specials_loaded++;
                        }
                        break;
                    case 2:
                        if(!is_set(newchar, ANI_FREESPECIAL2))
                        {
                            newchar->special[newchar->specials_loaded][0] = FLAG_MOVEDOWN;
                            newchar->special[newchar->specials_loaded][1] = FLAG_MOVEDOWN;
                            newchar->special[newchar->specials_loaded][2] = FLAG_ATTACK;
                            newchar->special[newchar->specials_loaded][MAX_SPECIAL_INPUTS-2] = ANI_FREESPECIAL2;
                            newchar->special[newchar->specials_loaded][MAX_SPECIAL_INPUTS-3] = 3;
                            newchar->specials_loaded++;
                        }
                        break;
                    case 3:
                        if(!is_set(newchar, ANI_FREESPECIAL3))
                        {
                            newchar->special[newchar->specials_loaded][0] = FLAG_MOVEUP;
                            newchar->special[newchar->specials_loaded][1] = FLAG_MOVEUP;
                            newchar->special[newchar->specials_loaded][2] = FLAG_ATTACK;
                            newchar->special[newchar->specials_loaded][MAX_SPECIAL_INPUTS-2] = ANI_FREESPECIAL3;
                            newchar->special[newchar->specials_loaded][MAX_SPECIAL_INPUTS-3] = 3;
                            newchar->specials_loaded++;
                        }
                        break;
                    }
                }
                else if(stricmp(value, "jumpattack")==0){
                    ani_id = ANI_JUMPATTACK;
                    if(newchar->jumpheight==4)
                    {
                        newanim->range[0] = 150;
                        newanim->range[1] = 200;
                    }
                }
                else if(stricmp(value, "jumpattack2")==0){
                    ani_id = ANI_JUMPATTACK2;
                }
                else if(stricmp(value, "jumpattack3")==0){
                    ani_id = ANI_JUMPATTACK3;
                }
                else if(stricmp(value, "jumpforward")==0){
                    ani_id = ANI_JUMPFORWARD;
                }
                else if(stricmp(value, "runjumpattack")==0){
                    ani_id = ANI_RUNJUMPATTACK;
                }
                else if(stricmp(value, "runattack")==0){
                    ani_id = ANI_RUNATTACK;    // New attack for when a player is running
                }
                else if(stricmp(value, "attackup")==0){
                    ani_id = ANI_ATTACKUP;    // New attack for when a player presses u u
                }
                else if(stricmp(value, "attackdown")==0){
                    ani_id = ANI_ATTACKDOWN;    // New attack for when a player presses d d
                }
                else if(stricmp(value, "attackforward")==0){
                    ani_id = ANI_ATTACKFORWARD;    // New attack for when a player presses f f
                }
                else if(stricmp(value, "attackbackward")==0){
                    ani_id = ANI_ATTACKBACKWARD;    // New attack for when a player presses b a
                }
                else if(stricmp(value, "attackboth")==0){    // Attack that is executed by holding down j and pressing a
                    ani_id = ANI_ATTACKBOTH;
                }
                else if(stricmp(value, "get")==0){
                    ani_id = ANI_GET;
                }
                else if(stricmp(value, "grab")==0){
                    ani_id = ANI_GRAB;
                }
                else if(stricmp(value, "grabwalk")==0){
                    ani_id = ANI_GRABWALK;
                }
                else if(stricmp(value, "grabwalkup")==0){
                    ani_id = ANI_GRABWALKUP;
                }
                else if(stricmp(value, "grabwalkdown")==0){
                    ani_id = ANI_GRABWALKDOWN;
                }
                else if(stricmp(value, "grabbackwalk")==0){
                    ani_id = ANI_GRABBACKWALK;
                }
                else if(stricmp(value, "grabturn")==0){
                    ani_id = ANI_GRABTURN;
                }
                else if(stricmp(value, "grabbed")==0){    // New grabbed animation for when grabbed
                    ani_id = ANI_GRABBED;
                }
                else if(stricmp(value, "grabbedwalk")==0){    // New animation for when grabbed and forced to walk
                    ani_id = ANI_GRABBEDWALK;
                }
                else if(stricmp(value, "grabbedwalkup")==0){
                    ani_id = ANI_GRABWALKUP;
                }
                else if(stricmp(value, "grabbedwalkdown")==0){
                    ani_id = ANI_GRABWALKDOWN;
                }
                else if(stricmp(value, "grabbedbackwalk")==0){
                    ani_id = ANI_GRABBEDBACKWALK;
                }
                else if(stricmp(value, "grabbedturn")==0){
                    ani_id = ANI_GRABBEDTURN;
                }
                else if(stricmp(value, "grabattack")==0){
                    ani_id = ANI_GRABATTACK;
                    newanim->attackone = 1; // default to 1, attack one one opponent
                }
                else if(stricmp(value, "grabattack2")==0){
                    ani_id = ANI_GRABATTACK2;
                    newanim->attackone = 1;
                }
                else if(stricmp(value, "grabforward")==0){    // New grab attack for when pressing forward attack
                    ani_id = ANI_GRABFORWARD;
                    newanim->attackone = 1;
                }
                else if(stricmp(value, "grabforward2")==0){    // New grab attack for when pressing forward attack
                    ani_id = ANI_GRABFORWARD2;
                    newanim->attackone = 1;
                }
                else if(stricmp(value, "grabbackward")==0){    // New grab attack for when pressing backward attack
                    ani_id = ANI_GRABBACKWARD;
                    newanim->attackone = 1;
                }
                else if(stricmp(value, "grabbackward2")==0){    // New grab attack for when pressing backward attack
                    ani_id = ANI_GRABBACKWARD2;
                    newanim->attackone = 1;
                }
                else if(stricmp(value, "grabup")==0){    // New grab attack for when pressing up attack
                    ani_id = ANI_GRABUP;
                    newanim->attackone = 1;
                }
                else if(stricmp(value, "grabup2")==0){    // New grab attack for when pressing up attack
                    ani_id = ANI_GRABUP2;
                    newanim->attackone = 1;
                }
                else if(stricmp(value, "grabdown")==0){    // New grab attack for when pressing down attack
                    ani_id = ANI_GRABDOWN;
                    newanim->attackone = 1;
                }
                else if(stricmp(value, "grabdown2")==0){    // New grab attack for when pressing down attack
                    ani_id = ANI_GRABDOWN2;
                    newanim->attackone = 1;
                }
                else if(stricmp(value, "spawn")==0){      //  spawn/respawn works separately now
                    ani_id = ANI_SPAWN;
                }
                else if(stricmp(value, "respawn")==0){      //  spawn/respawn works separately now
                    ani_id = ANI_RESPAWN;
                }
                else if(stricmp(value, "throw")==0){
                    ani_id = ANI_THROW;
                }
                else if(stricmp(value, "block")==0){    // Now enemies can block attacks on occasion
                    ani_id = ANI_BLOCK;
                    newanim->range[0] = 1;
                    newanim->range[1] = 100;
                }
                else if(strnicmp(value, "follow", 6)==0 && (!value[6] || (value[6]>='1' && value[6]<='9'))){
                    tempInt = atoi(value+6);
                    if(tempInt<1) tempInt = 1;
                    ani_id = animfollows[tempInt-1];
                }
                else if(stricmp(value, "chargeattack")==0){
                    ani_id = ANI_CHARGEATTACK;
                }
                else if(stricmp(value, "vault")==0){
                    ani_id = ANI_VAULT;
                }
                else if(stricmp(value, "turn")==0){
                    ani_id = ANI_TURN;
                }
                else if(stricmp(value, "forwardjump")==0){
                    ani_id = ANI_FORWARDJUMP;
                }
                else if(stricmp(value, "runjump")==0){
                    ani_id = ANI_RUNJUMP;
                }
                else if(stricmp(value, "jumpland")==0){
                    ani_id = ANI_JUMPLAND;
                }
                else if(stricmp(value, "jumpdelay")==0){
                    ani_id = ANI_JUMPDELAY;
                }
                else if(stricmp(value, "hitwall")==0){
                    ani_id = ANI_HITWALL;
                }
                else if(stricmp(value, "slide")==0){
                    ani_id = ANI_SLIDE;
                }
                else if(stricmp(value, "runslide")==0){
                    ani_id = ANI_RUNSLIDE;
                }
				else if(stricmp(value, "blockpainb")==0){
                    ani_id = ANI_BLOCKPAINB;
                }
				else if(stricmp(value, "blockpains")==0){
                    ani_id = ANI_BLOCKPAINS;
                }
                else if(stricmp(value, "blockpain")==0){
                    ani_id = ANI_BLOCKPAIN;   // If no new animation, load fall animation into both "respawn" & "fall"
                }
                else if(strnicmp(value, "blockpain", 9)==0 && value[9]>='1' && value[9]<='9'){
                    tempInt = atoi(value+9);
					if(tempInt==1){
						ani_id = ANI_BLOCKPAIN;
					}
					else if(tempInt==2){
						ani_id = ANI_BLOCKPAIN2;
					}
					else if(tempInt==3){
						ani_id = ANI_BLOCKPAIN3;
					}
					else if(tempInt==4){
						ani_id = ANI_BLOCKPAIN4;
					}
					else if(tempInt==5){
						ani_id = ANI_BLOCKPAIN5;
					}
					else if(tempInt==6){
						ani_id = ANI_BLOCKPAIN6;
					}
					else if(tempInt==7){
						ani_id = ANI_BLOCKPAIN7;
					}
					else if(tempInt==8){
						ani_id = ANI_BLOCKPAIN8;
					}
					else if(tempInt==9){
						ani_id = ANI_BLOCKPAIN9;
					}
					else if(tempInt==10){
						ani_id = ANI_BLOCKPAIN10;
					}
					else{
						if(tempInt<MAX_ATKS-STA_ATKS+1) tempInt = MAX_ATKS-STA_ATKS+1;
						ani_id = animblkpains[tempInt+STA_ATKS-1];
					}
                }
                else if(stricmp(value, "duckattack")==0){
                    ani_id = ANI_DUCKATTACK;
                }
                else shutdown(1, "Invalid animation name '%s'", value);

                newchar->animation[ani_id] = newanim;

            }
            else if(stricmp(command, "loop")==0){
                if(newanim == NULL) shutdown(1, "Can't set loop: no animation specified!");
                newanim->loop[0] = atoi(findarg(buf+pos, 1)); //0 = Off, 1 = on.
                newanim->loop[1] = atoi(findarg(buf+pos, 2)); //Loop to frame.
                newanim->loop[2] = atoi(findarg(buf+pos, 3)); //Loop end frame.
            }
            else if(stricmp(command, "animheight")==0){
                newanim->height = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "delay")==0){
                delay = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "offset")==0){
                offset[0] = atoi(findarg(buf+pos, 1));
                offset[1] = atoi(findarg(buf+pos, 2));
            }
            else if(stricmp(command, "shadowcoords")==0){
                shadow_xz[0] = atoi(findarg(buf+pos, 1));
                shadow_xz[1] = atoi(findarg(buf+pos, 2));
                shadow_set=1;
            }
            // 1-10-05  adjust the energycost of specials
            else if(stricmp(command, "energycost")==0 || stricmp(command, "mpcost")==0){
                newanim->energycost[0] = atoi(findarg(buf+pos, 1));
                newanim->energycost[1] = atoi(findarg(buf+pos, 2));
				newanim->energycost[2] = atoi(findarg(buf+pos, 3));
            }
            else if(stricmp(command, "mponly")==0){
                newanim->energycost[1] = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "chargetime")==0){
                newanim->chargetime = atof(findarg(buf+pos, 1));
            }
            //dive kicks for players like tmnt by tails
            else if(stricmp(command, "dive")==0){
                newanim->dive[0] = atof(findarg(buf+pos, 1));
                newanim->dive[1] = atof(findarg(buf+pos, 2));
            }
            else if(stricmp(command, "dive1")==0){
                newanim->dive[0] = atof(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "dive2")==0){
                newanim->dive[1] = atof(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "attackone")==0){
                newanim->attackone = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "counterattack")==0){
                attack.counterattack = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "throwframe")==0 ||
					stricmp(command, "pshotframe")==0 ||
					stricmp(command, "pshotframew")==0 ||
				    stricmp(command, "pshotframeno")==0){
                    newanim->throwframe = atoi(findarg(buf+pos, 1));
                    newanim->throwa = atoi(findarg(buf+pos, 2));
					if(!newanim->throwa) newanim->throwa = 70;
					else if(newanim->throwa == -1) newanim->throwa = 0;
            }
			else if(stricmp(command, "shootframe")==0){
                    newanim->shootframe = atoi(findarg(buf+pos, 1));
					newanim->throwa = atoi(findarg(buf+pos, 2));
                    if(newanim->throwa == -1) newanim->throwa = 0;
            }
            else if(stricmp(command, "tossframe")==0 ||
				    stricmp(command, "pbombframe")==0){
                newanim->tossframe = atoi(findarg(buf+pos, 1));
                newanim->throwa = atoi(findarg(buf+pos, 2));
                if(newanim->throwa < 0) newanim->throwa = -1;
            }
            else if(stricmp(command, "custknife")==0 ||
				    stricmp(command, "custpshot")==0 ||
					stricmp(command, "custpshotw")==0){
                newanim->custknife= get_cached_model_index(findarg(buf+pos, 1));
            }
			else if(stricmp(command, "custpshotno")==0){
                newanim->custpshotno= get_cached_model_index(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "custbomb")==0 ||
				    stricmp(command, "custpbomb")==0){
                newanim->custbomb= get_cached_model_index(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "custstar")==0){
                newanim->custstar= get_cached_model_index(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "jumpframe")==0){
                newanim->jumpframe = atoi(findarg(buf+pos, 1));
                newanim->jumpv = atof(findarg(buf+pos, 2));    // Added so movement can be customized for jumpframes
                value = findarg(buf+pos, 3);
                if(value[0])
                {
                    newanim->jumpx = atof(findarg(buf+pos, 3));
                    newanim->jumpz = atof(findarg(buf+pos, 4));
                }
                else // k, only for backward compatibility :((((((((((((((((
                {
                    if(newanim->jumpv <= 0)
                    {
                        if(newchar->type == TYPE_PLAYER)
                        {
                            newanim->jumpv = newchar->jumpheight / 2;
                            newanim->jumpz = 0;
                            newanim->jumpx = 2;
                        }
                        else
                        {
                            newanim->jumpv = newchar->jumpheight;
                            newanim->jumpz = newanim->jumpx = 0;
                        }
                    }
                    else
                    {
                        if(newchar->type != TYPE_ENEMY && newchar->type != TYPE_NPC)
                            newanim->jumpz = newanim->jumpx = 0;
                        else
                        {
                            newanim->jumpz = 0;
                            newanim->jumpx = (float)1.3;
                        }
                    }
                }

				value = findarg(buf+pos, 5);
				if(value[0]) newanim->jumpd = get_cached_model_index(value);
                else newanim->jumpd = -1;

            }
            else if(stricmp(command, "bouncefactor")==0)
            {
                newanim->bounce = atof(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "landframe")==0)
            {
                newanim->landframe[0] = atoi(findarg(buf+pos, 1));
				value = findarg(buf+pos, 2);
				if(value[0]) newanim->landframe[1] = get_cached_model_index(value);
                else newanim->landframe[1] = -1;
            }
            else if(stricmp(command, "dropframe")==0)
            {
                newanim->dropframe = atoi(findarg(buf+pos, 1));
            }
			else if(stricmp(command, "cancel")==0) // OX. Cancel command.
            {
                int i;                                                                   // OX. Modified copy/paste of COM settings code
                int t;
				newanim->cancel = 3;
				for(i = 0, t = 4; i < MAX_SPECIAL_INPUTS-6; i++, t++)
                {
                    value = findarg(buf+pos, t);
                    if(!value[0]) break;
                    if(stricmp(value, "u")==0){
                        newchar->special[newchar->specials_loaded][i] = FLAG_MOVEUP;
                    }
                    else if(stricmp(value, "d")==0){
                        newchar->special[newchar->specials_loaded][i] = FLAG_MOVEDOWN;
                    }
                    else if(stricmp(value, "f")==0){
                        newchar->special[newchar->specials_loaded][i] = FLAG_FORWARD;
                    }
                    else if(stricmp(value, "b")==0){
                        newchar->special[newchar->specials_loaded][i] = FLAG_BACKWARD;
                    }
                    else if(stricmp(value, "a")==0){
                        newchar->special[newchar->specials_loaded][i] = FLAG_ATTACK;
                    }
					else if(stricmp(value, "a2")==0){
                        newchar->special[newchar->specials_loaded][i] = FLAG_ATTACK2;
                    }
					else if(stricmp(value, "a3")==0){
                        newchar->special[newchar->specials_loaded][i] = FLAG_ATTACK3;
                    }
					else if(stricmp(value, "a4")==0){
                        newchar->special[newchar->specials_loaded][i] = FLAG_ATTACK4;
                    }
                    else if(stricmp(value, "j")==0){
                        newchar->special[newchar->specials_loaded][i] = FLAG_JUMP;
                    }
                    else if(stricmp(value, "s")==0){
                        newchar->special[newchar->specials_loaded][i] = FLAG_SPECIAL;
                    }
                    else if(stricmp(value, "k")==0){
                        newchar->special[newchar->specials_loaded][i] = FLAG_SPECIAL;
                    }
					else if(strnicmp(value, "freespecial", 11)==0 && (!value[11] || (value[11] >= '1' && value[11] <= '9'))){
						tempInt = atoi(value+11);
                        if(tempInt<1) tempInt = 1;
						newchar->special[newchar->specials_loaded][MAX_SPECIAL_INPUTS-5] = animspecials[tempInt-1];
						newchar->special[newchar->specials_loaded][MAX_SPECIAL_INPUTS-7] = atoi(findarg(buf+pos, 1)); // stores start frame
				        newchar->special[newchar->specials_loaded][MAX_SPECIAL_INPUTS-8] = atoi(findarg(buf+pos, 2)); // stores end frame
						newchar->special[newchar->specials_loaded][MAX_SPECIAL_INPUTS-9] = ani_id;                    // stores current anim
						newchar->special[newchar->specials_loaded][MAX_SPECIAL_INPUTS-10] = atoi(findarg(buf+pos, 3));// stores hits
                    }
                    else shutdown(1, "Invalid cancel command '%s'", value); // OX. Changed this line so that errors do not confuse modders.
                }
                newchar->special[newchar->specials_loaded][MAX_SPECIAL_INPUTS-6]=i-1; // max steps
                newchar->specials_loaded++;
				if(newchar->specials_loaded > max_freespecials) shutdown(1, "Too many Freespecials and/or Cancels. Please increase Maxfreespecials", value); // OX. This is to catch freespecials that use same animation.
            }
            else if(stricmp(command, "sound")==0){
                soundtoplay = sound_load_sample(findarg(buf+pos, 1), packfile, 0);
            }
            else if(stricmp(command, "hitfx")==0){
                attack.hitsound = sound_load_sample(findarg(buf+pos, 1), packfile, 0);
            }
            else if(stricmp(command, "hitflash")==0){
                value = findarg(buf+pos, 1);
                if(stricmp(value, "none")==0) attack.hitflash = -1;
                else attack.hitflash = get_cached_model_index(value);
            }
			else if(stricmp(command, "blockflash")==0){
                value = findarg(buf+pos, 1);
                if(stricmp(value, "none")==0) attack.blockflash = -1;
                else attack.blockflash = get_cached_model_index(value);
            }
			else if(stricmp(command, "blockfx")==0){
                attack.blocksound = sound_load_sample(findarg(buf+pos, 1), packfile, 0);
            }
            else if(stricmp(command, "fastattack")==0){
                newanim->fastattack = atoi(findarg(buf+pos, 1));

            }
            else if(stricmp(command, "bbox")==0){
                bbox[0] = atoi(findarg(buf+pos, 1));
                bbox[1] = atoi(findarg(buf+pos, 2));
                bbox[2] = atoi(findarg(buf+pos, 3));
                bbox[3] = atoi(findarg(buf+pos, 4));
                bbox[4] = atoi(findarg(buf+pos, 5));
            }
            else if(stricmp(command, "bboxz")==0){
                bbox[4] = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "platform")==0){
                for(i=0;findarg(buf+pos,i+1)[0]; i++);
                if(i<8)
                {
                    for(i=0;i<6; i++) platform[i+2] = atof(findarg(buf+pos, i+1));
                    platform[0] = 99999;
                }
                else for(i=0; i<8; i++) platform[i] = atof(findarg(buf+pos, i+1));
            }
            else if(stricmp(command, "drawmethod")==0) // special effects
            {
                drawmethod.scalex = atoi(findarg(buf+pos, 1));
                drawmethod.scaley = atoi(findarg(buf+pos, 2));
                drawmethod.flipx = atoi(findarg(buf+pos, 3));
                drawmethod.flipy = atoi(findarg(buf+pos, 4));
                drawmethod.shiftx = atoi(findarg(buf+pos, 5));
                drawmethod.alpha = atoi(findarg(buf+pos, 6));
                if(!blendfx_is_set)
                {
                    if(drawmethod.alpha>0 && drawmethod.alpha<=MAX_BLENDINGS)
                    {
                        blendfx[drawmethod.alpha-1] = 1;
                    }
                }
                drawmethod.remap = atoi(findarg(buf+pos, 7));
                drawmethod.fillcolor = parsecolor(findarg(buf+pos, 8));
                drawmethod.rotate = atoi(findarg(buf+pos, 9));
                drawmethod.fliprotate = atoi(findarg(buf+pos, 10))%360;
                if(drawmethod.scalex<0) {drawmethod.scalex = -drawmethod.scalex;drawmethod.flipx = !drawmethod.flipx;}
                if(drawmethod.scaley<0) {drawmethod.scaley = - drawmethod.scaley;drawmethod.flipy = !drawmethod.flipy;}
                if(drawmethod.rotate)
                {
                    if(drawmethod.rotate<0) drawmethod.rotate += 360;
                }
                drawmethod.flag = 1;
            }
            else if(stricmp(command, "nodrawmethod")==0)//disable special effects
            {
                drawmethod.flag = 0;
            }
            else if(
                (strnicmp(command, "attack", 6)==0 &&
                 (!command[6]||(command[6]>='1' && command[6]<='9'))) ||
                stricmp(command, "shock")==0 || stricmp(command, "burn")==0 ||
                stricmp(command, "steal")==0 || stricmp(command, "freeze")==0 || stricmp(command, "itembox")==0){
                    abox[0] = atoi(findarg(buf+pos, 1));
                    abox[1] = atoi(findarg(buf+pos, 2));
                    abox[2] = atoi(findarg(buf+pos, 3));
                    abox[3] = atoi(findarg(buf+pos, 4));
                    attack.dropv[0] = 3;
                    attack.dropv[1] = (float)1.2;
                    attack.dropv[2] = 0;
                    attack.attack_force = atoi(findarg(buf+pos, 5));

					if(stricmp(command, "steal")==0) attack.steal = 1;

                    if(stricmp(command, "freeze")==0)
                    {
                        attack.freeze = 1;
                        attack.freezetime = atoi(findarg(buf+pos, 6)) * GAME_SPEED;
                        attack.forcemap = -1;
                        attack.attack_drop = 0;
                    }
                    else attack.attack_drop = atoi(findarg(buf+pos, 6));

                    attack.no_block = atoi(findarg(buf+pos, 7));
                    attack.no_flash = atoi(findarg(buf+pos, 8));
                    attack.pause_add = atoi(findarg(buf+pos, 9));
                    attack.attack_coords[4] = atoi(findarg(buf+pos, 10)); // depth or z

                    if(stricmp(command, "attack")==0 || stricmp(command, "attack1")==0) attack.attack_type = ATK_NORMAL;
                    else if(stricmp(command, "attack2")     ==0)    attack.attack_type  = ATK_NORMAL2;
                    else if(stricmp(command, "attack3")     ==0)    attack.attack_type  = ATK_NORMAL3;
                    else if(stricmp(command, "attack4")     ==0)    attack.attack_type  = ATK_NORMAL4;
                    else if(stricmp(command, "attack5")     ==0)    attack.attack_type  = ATK_NORMAL5;
                    else if(stricmp(command, "attack6")     ==0)    attack.attack_type  = ATK_NORMAL6;
                    else if(stricmp(command, "attack7")     ==0)    attack.attack_type  = ATK_NORMAL7;
                    else if(stricmp(command, "attack8")     ==0)    attack.attack_type  = ATK_NORMAL8;
                    else if(stricmp(command, "attack9")     ==0)    attack.attack_type  = ATK_NORMAL9;
                    else if(stricmp(command, "attack10")    ==0)    attack.attack_type  = ATK_NORMAL10;
                    else if(stricmp(command, "shock")       ==0)    attack.attack_type  = ATK_SHOCK;
                    else if(stricmp(command, "burn")        ==0)    attack.attack_type  = ATK_BURN;
                    else if(stricmp(command, "steal")       ==0)    attack.attack_type  = ATK_STEAL;
                    else if(stricmp(command, "freeze")      ==0)    attack.attack_type  = ATK_FREEZE;
                    else if(stricmp(command, "itembox")     ==0)    attack.attack_type  = ATK_ITEM;
                    else if(strnicmp(command, "attack", 6)  ==0)    // new attack types?
                    {
                        tempInt = atoi(command+6);
                        if(tempInt<MAX_ATKS-STA_ATKS+1) tempInt = MAX_ATKS-STA_ATKS+1;
                        attack.attack_type = tempInt+STA_ATKS-1;
                    }
                }
            else if(stricmp(command, "attackz")==0 || stricmp(command, "hitz")==0){
                attack.attack_coords[4] = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "blast")==0){
                abox[0] = atoi(findarg(buf+pos, 1));
                abox[1] = atoi(findarg(buf+pos, 2));
                abox[2] = atoi(findarg(buf+pos, 3));
                abox[3] = atoi(findarg(buf+pos, 4));
                attack.dropv[0] = 3;
                attack.dropv[1] = 2.5;
                attack.dropv[2] = 0;
                attack.attack_force = atoi(findarg(buf+pos, 5));
                attack.no_block = atoi(findarg(buf+pos, 6));
                attack.no_flash = atoi(findarg(buf+pos, 7));
                attack.pause_add = atoi(findarg(buf+pos, 8));
                attack.attack_drop = 1;
                attack.attack_type = ATK_BLAST;
                attack.attack_coords[4] = atoi(findarg(buf+pos, 9)); // depth or z
                attack.blast = 1;
            }
            else if(stricmp(command, "dropv")==0) // drop velocity add if the target is knocked down
            {
                pattack = (!newanim && newchar->smartbomb)?newchar->smartbomb:&attack;
                pattack->dropv[0] = atof(findarg(buf+pos, 1)); // height add
                pattack->dropv[1] = atof(findarg(buf+pos, 2)); // xdir add
                pattack->dropv[2] = atof(findarg(buf+pos, 3)); // zdir add
            }
			else if(stricmp(command, "otg")==0) // Over The Ground hit.
            {
                attack.otg = atoi(findarg(buf+pos, 1));
            }
			else if(stricmp(command, "jugglecost")==0) // if cost >= opponents jugglepoints , we can juggle
            {
                attack.jugglecost = atoi(findarg(buf+pos, 1));
            }
			else if(stricmp(command, "guardcost")==0) // if cost >= opponents guardpoints , opponent will play guardcrush anim
            {
                attack.guardcost = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "stun")==0) // DC, 12/19/2007. Like Freeze, but no auto remap.
            {
                pattack = (!newanim && newchar->smartbomb)?newchar->smartbomb:&attack;
                pattack->freeze = 1;
                pattack->freezetime = atoi(findarg(buf+pos, 1)) * GAME_SPEED;
                pattack->attack_drop = 0;
            }
            else if(stricmp(command, "grabin")==0) // fake grab distanse efffect, not link
            {
                pattack = (!newanim && newchar->smartbomb)?newchar->smartbomb:&attack;
                pattack->grab =  atoi(findarg(buf+pos, 1));
                pattack->grab_distance = atof(findarg(buf+pos, 2));
            }
            else if(stricmp(command, "noreflect")==0) // only cost target's hp, don't knock down or cause pain, unless the target is killed
            {
                pattack = (!newanim && newchar->smartbomb)?newchar->smartbomb:&attack;
                pattack->no_pain = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "forcedirection")==0) // the attack direction
            {
                pattack = (!newanim && newchar->smartbomb)?newchar->smartbomb:&attack;
                pattack->force_direction = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "damageonlanding")==0) // fake throw damage on landing
            {
                pattack = (!newanim && newchar->smartbomb)?newchar->smartbomb:&attack;
                pattack->damage_on_landing = atoi(findarg(buf+pos, 1));
                pattack->blast = atoi(findarg(buf+pos, 2));
            }
            else if(stricmp(command, "seal")==0) // Disable special moves for specified time.
            {
                pattack = (!newanim && newchar->smartbomb)?newchar->smartbomb:&attack;
                pattack->sealtime = atoi(findarg(buf+pos, 1)) * GAME_SPEED;
                pattack->seal = atoi(findarg(buf+pos, 2));
            }
            else if(stricmp(command, "staydown")==0) // Disable special moves for specified time.
            {
                pattack = (!newanim && newchar->smartbomb)?newchar->smartbomb:&attack;
                pattack->staydown[0]    = atoi(findarg(buf+pos, 1)); //Risetime modifier.
                pattack->staydown[1]    = atoi(findarg(buf+pos, 2)); //Riseattack time addition and toggle.
            }
            else if(stricmp(command, "dot")==0) // Cause damage over time effect.
            {
                attack.dot_index  = atoi(findarg(buf+pos, 1));  //Index.
                attack.dot_time   = atoi(findarg(buf+pos, 2));  //Time to expiration.
                attack.dot        = atoi(findarg(buf+pos, 3));  //Mode, see common_dot.
                attack.dot_force  = atoi(findarg(buf+pos, 4));  //Amount per tick.
                attack.dot_rate   = atoi(findarg(buf+pos, 5));  //Tick delay.
            }
            else if(stricmp(command, "forcemap")==0) // force color map change for specified time
            {
                pattack = (!newanim && newchar->smartbomb)?newchar->smartbomb:&attack;
                pattack->forcemap = atoi(findarg(buf+pos, 1));
                pattack->maptime = atoi(findarg(buf+pos, 2)) * GAME_SPEED;
            }
			else if(stricmp(command, "idle")==0){
				idle = atoi(findarg(buf+pos, 1));
			}
            else if(stricmp(command, "move")==0){
                move = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "movez")==0){
                movez = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "movea")==0){
                movea = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "seta")==0){
                seta = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "fshadow")==0){
                frameshadow = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "range")==0){
                if(newanim==NULL) shutdown(1, "Cannot set range: no animation!");
                newanim->range[0] = atoi(findarg(buf+pos, 1));
                newanim->range[1] = atoi(findarg(buf+pos, 2));
            }
            else if(stricmp(command, "rangez")==0){
                if(newanim==NULL) shutdown(1, "Cannot set rangez: no animation!");
                newanim->range[2] = atoi(findarg(buf+pos, 1));
                newanim->range[3] = atoi(findarg(buf+pos, 2));
            }
            else if(stricmp(command, "rangea")==0){
                if(newanim==NULL) shutdown(1, "Cannot set rangea: no animation!");
                newanim->range[4] = atoi(findarg(buf+pos, 1));
                newanim->range[5] = atoi(findarg(buf+pos, 2));
            }
            else if(stricmp(command, "rangeb")==0){
                if(newanim==NULL) shutdown(1, "Cannot set rangeb: no animation!");
                newanim->range[6] = atoi(findarg(buf+pos, 1));
                newanim->range[7] = atoi(findarg(buf+pos, 2));
            }
            else if(stricmp(command, "frame")==0){
                if(newanim==NULL) shutdown(1, "Cannot add frame: animation not specified!");
                peek = 0;
                if(frameset && framecount>=0) framecount = -framecount;
                while(!frameset){
                    value3 = findarg(buf+pos+peek, 0);
                    if(stricmp(value3, "frame")==0) framecount++;
                    if((stricmp(value3, "anim")==0) || (pos+peek >= size)) frameset = 1;
                    // Go to next line
                    while(buf[pos+peek] && buf[pos+peek]!='\n' && buf[pos+peek]!='\r') ++peek;
                    while(buf[pos+peek]=='\n' || buf[pos+peek]=='\r') ++peek;
                }
                value = findarg(buf+pos, 1);
                //printf("frame count: %d\n",framecount);
                //printf("Load sprite '%s'...\n", value);
                index = loadsprite(value, offset[0], offset[1],PIXEL_8);//don't use palette for the sprite since it will one palette from the entity's remap list in 24bit mode
                if(pixelformat==PIXEL_x8)
                {
                    // for old mod just give it a default palette
                    if(newchar->palette==NULL)
                    {
                        newchar->palette = tracemalloc("newchar#frame#palette", PAL_BYTES);
                        if(loadimagepalette(value, packfile, newchar->palette)==0)
                            shutdown(1, "Failed to load palette from '%s' in '%s'.", value, filename);
                    }
                    if(index>=0)
                    {
						sprite_map[index].sprite->palette = newchar->palette;
						sprite_map[index].sprite->pixelformat = pixelformat;
                    }
                }
                if((index>=0) && (maskindex>=0))
				{
					sprite_map[index].sprite->mask = sprite_map[maskindex].sprite;
					maskindex = -1;
				}
                // Adjust coords: add offsets and change size to coords
                bbox_con[0] = bbox[0] - offset[0];
                bbox_con[1] = bbox[1] - offset[1];
                bbox_con[2] = bbox[2] + bbox_con[0];
                bbox_con[3] = bbox[3] + bbox_con[1];
                bbox_con[4] = bbox[4];
                attack.attack_coords[0] = abox[0] - offset[0];
                attack.attack_coords[1] = abox[1] - offset[1];
                attack.attack_coords[2] = abox[2] + attack.attack_coords[0];
                attack.attack_coords[3] = abox[3] + attack.attack_coords[1];
                //attack.attack_coords[4] = abox[4];
                if(platform[0]==99999) // old style
                {
                    platform_con[0] = 0;
                    platform_con[1] = 3;
                    platform_con[2] = platform[2] - offset[0];
                    platform_con[3] = platform[3] - offset[0];
                    platform_con[4] = platform[4] - offset[0];
                    platform_con[5] = platform[5] - offset[0];
                    platform_con[6] = platform[6]+3;
                }
                else // wall style
                {
                    platform_con[0] = platform[0] - offset[0];
                    platform_con[1] = platform[1] - offset[1];
                    platform_con[2] = platform[2];
                    platform_con[3] = platform[3];
                    platform_con[4] = platform[4];
                    platform_con[5] = platform[5];
                    platform_con[6] = platform[6];
                }
                platform_con[6] = platform[6];
                platform_con[7] = platform[7];
                if(shadow_set)
                {
                    shadow_coords[0] = shadow_xz[0] - offset[0];
                    shadow_coords[1] = shadow_xz[1] - offset[1];
                }
                else
                {
                    shadow_coords[0] = shadow_coords[1] = 0;
                }

                curframe = addframe(newanim, index, framecount, delay, (unsigned char)idle,
                                    bbox_con, &attack, move, movez,
                                    movea, seta, platform_con, frameshadow, shadow_coords, soundtoplay, &drawmethod);

				memset(bbox_con, 0, sizeof(bbox_con));
                soundtoplay = -1;
            }
			else if(stricmp(command, "alphamask")==0){
                if(newanim==NULL) shutdown(1, "Cannot add alpha mask: animation not specified!");
				if(maskindex>=0) shutdown(1, "Cannot add alpha mask: a mask has already been specified for this frame!");
                value = findarg(buf+pos, 1);
                //printf("frame count: %d\n",framecount);
                //printf("Load sprite '%s'...\n", value);
                index = loadsprite(value, offset[0], offset[1],PIXEL_8);//don't use palette for the mask
				maskindex = index;
            }
            else if(stricmp(command, "flipframe")==0){
                newanim->flipframe = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "followanim")==0){
                newanim->followanim = atoi(findarg(buf+pos, 1));
                if(newanim->followanim > max_follows) newanim->followanim = max_follows;
                if(newanim->followanim < 0) newanim->followanim = 0;
            }
            else if(stricmp(command, "followcond")==0){
                newanim->followcond = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "counterframe")==0){
				newanim->counterframe[0]	= atoi(findarg(buf+pos, 1));
				newanim->counterframe[1]	= atoi(findarg(buf+pos, 1));
				newanim->counterframe[2]	= atoi(findarg(buf+pos, 2));
				newanim->counterframe[3]	= atoi(findarg(buf+pos, 3));
            }
			else if(stricmp(command, "counterrange")==0){
				newanim->counterframe[0]	= atoi(findarg(buf+pos, 1));
				newanim->counterframe[1]	= atoi(findarg(buf+pos, 2));
				newanim->counterframe[2]	= atoi(findarg(buf+pos, 3));
				newanim->counterframe[3]	= atoi(findarg(buf+pos, 4));
            }
            else if(stricmp(command, "weaponframe")==0){
				newanim->weaponframe    = tracemalloc("weaponframe", 2 * sizeof(newanim->weaponframe));
				memset(newanim->weaponframe, 0, 2 * sizeof(newanim->weaponframe));
                newanim->weaponframe[0] = atoi(findarg(buf+pos, 1));
                newanim->weaponframe[1] = atoi(findarg(buf+pos, 2));
            }
            else if(stricmp(command, "quakeframe")==0){
                newanim->quakeframe[0] = atoi(findarg(buf+pos, 1));
                newanim->quakeframe[1] = atoi(findarg(buf+pos, 2));
                newanim->quakeframe[2] = atoi(findarg(buf+pos, 3));
				newanim->quakeframe[3] = 0;
            }
            else if(stricmp(command, "subentity")==0 ||
				    stricmp(command, "custentity")==0){
                value = findarg(buf+pos, 1);
                if(value[0]) newanim->subentity = get_cached_model_index(value);
            }
            else if(stricmp(command, "spawnframe")==0){
				newanim->spawnframe    = tracemalloc("spawnframe", 5 * sizeof(newanim->spawnframe));
				memset(newanim->spawnframe, 0, 5 * sizeof(newanim->spawnframe));
                newanim->spawnframe[0] = atof(findarg(buf+pos, 1));
                newanim->spawnframe[1] = atof(findarg(buf+pos, 2));
                newanim->spawnframe[2] = atof(findarg(buf+pos, 3));
                newanim->spawnframe[3] = atof(findarg(buf+pos, 4));
                newanim->spawnframe[4] = atof(findarg(buf+pos, 5));
            }
            else if(stricmp(command, "summonframe")==0){
				newanim->summonframe    = tracemalloc("summonframe", 5 * sizeof(newanim->summonframe));
				memset(newanim->summonframe, 0, 5 * sizeof(newanim->summonframe));
                newanim->summonframe[0] = atof(findarg(buf+pos, 1));
                newanim->summonframe[1] = atof(findarg(buf+pos, 2));
                newanim->summonframe[2] = atof(findarg(buf+pos, 3));
                newanim->summonframe[3] = atof(findarg(buf+pos, 4));
                newanim->summonframe[4] = atof(findarg(buf+pos, 5));
            }
            else if(stricmp(command, "unsummonframe")==0){
                newanim->unsummonframe = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "@script") == 0){
                if(ani_id < 0)  shutdown(1, "command '@script' must follow an animation! file: '%s'", filename);
                if(!scriptbuf[0]){ // if empty, paste the main function text here
                    strcat(scriptbuf, pre_text);
                }
                scriptbuf[strlen(scriptbuf) - strlen(sur_text)] = 0; // cut last chars
                if(script_id != ani_id){ // if expression 1
                    sprintf(namebuf, ifid_text, ani_id);
                    strcat(scriptbuf, namebuf);
                    script_id = ani_id;
                }
                scriptbuf[strlen(scriptbuf) - strlen(endifid_text)] = 0; // cut last chars
                while(strncmp(buf+pos, "@script", 7)){
                    pos++;
                }
                pos += 7;
                while(strncmp(buf+pos, "@end_script", 11)){
                    len = strlen(scriptbuf);
                    scriptbuf[len] = *(buf+pos);
                    scriptbuf[len+1] = 0;
                    pos++;
                }
                pos += 11;
                strcat(scriptbuf, endifid_text); // put back last  chars
                strcat(scriptbuf, sur_text); // put back last  chars
            }
            else if(stricmp(command, "@cmd")==0){ //translate @cmd into script function call
                if(ani_id < 0) shutdown(1, "command '@cmd' must follow an animation! file: '%s'", filename);
                if(!scriptbuf[0]){ // if empty, paste the main function text here
                    strcat(scriptbuf, pre_text);
                }
                scriptbuf[strlen(scriptbuf) - strlen(sur_text)] = 0; // cut last chars
                if(script_id != ani_id){ // if expression 1
                    sprintf(namebuf, ifid_text, ani_id);
                    strcat(scriptbuf, namebuf);
                    script_id = ani_id;
                }
                j = 1;
                value = findarg(buf+pos, j);
                scriptbuf[strlen(scriptbuf) - strlen(endifid_text)] = 0; // cut last chars
                if(value && value[0]){
                    sprintf(namebuf, if_text, curframe);//only execute in current frame
                    strcat(scriptbuf, namebuf);
                    sprintf(namebuf, call_text, value);
                    strcat(scriptbuf, namebuf);
                    do{ //argument and comma
                        j++;
                        value = findarg(buf+pos, j);
                        if(value && value[0]) {
                            if(j!=2) strcat(scriptbuf, comma_text);
                            strcat(scriptbuf, value);
                        }
                    }while(value && value[0]);
                }
                strcat(scriptbuf, endcall_text);
                strcat(scriptbuf, endif_text); //end of if
                strcat(scriptbuf, endifid_text); // put back last  chars
                strcat(scriptbuf, sur_text); // put back last  chars
            }
            else shutdown(1, "Command '%s' not understood in file '%s'!", command, filename);
        }

        // Go to next line
        while(buf[pos] && buf[pos]!='\n' && buf[pos]!='\r') ++pos;
        while(buf[pos]=='\n' || buf[pos]=='\r') ++pos;
    }
    tempInt = 1;
    if(scriptbuf[0]) {
        //printf("\n%s\n", scriptbuf);
        if(!Script_IsInitialized(newchar->animation_script))
            Script_Init(newchar->animation_script, newchar->name, 0);
        tempInt = Script_AppendText(newchar->animation_script, scriptbuf, filename);
        //Interpreter_OutputPCode(newchar->animation_script.pinterpreter, "code");
        writeToScriptLog("\n####animationscript function main#####\n# ");
        writeToScriptLog(filename);
        writeToScriptLog("\n########################################\n");
        writeToScriptLog(scriptbuf);
    }
    Script_Compile(newchar->animation_script);

    if(buf != NULL){
        tracefree(buf);
        buf = NULL;
    }
    if(scriptbuf){
        tracefree(scriptbuf);
        scriptbuf = NULL;
    }

    if(!tempInt)// parse script failed
    {
        shutdown(1, "Error parsing function main of animation script in file '%s'!", filename);
    }

    // We need a little more work to initialize the new A.I. types if they are not loaded from file
    if(newchar->aiattack==-1) newchar->aiattack = 0;
    if(newchar->aimove==-1) newchar->aimove = 0;
    //if(!newchar->offscreenkill) newchar->offscreenkill = 1000;

    if(newchar->risetime[0]==-1)
    {
        if(newchar->type==TYPE_PLAYER)
        {
            if(newchar->animation[ANI_RISEATTACK]) newchar->risetime[0] = GAME_SPEED/2;
            else newchar->risetime[0]=GAME_SPEED;
        }
        else if(newchar->type==TYPE_ENEMY || newchar->type==TYPE_NPC)
        {
            newchar->risetime[0] = 0;
        }
    }

    if(newchar->hostile<0) {// not been initialized, so initialize it
        switch (newchar->type){
             case TYPE_ENEMY:
                 newchar->hostile = TYPE_PLAYER ;
                 break;
             case TYPE_PLAYER: // dont really needed, since you don't need A.I. control for players
                 newchar->hostile = TYPE_PLAYER | TYPE_ENEMY | TYPE_OBSTACLE;
                 break;
             case TYPE_TRAP:
                 newchar->hostile  = TYPE_ENEMY | TYPE_PLAYER;
             case TYPE_OBSTACLE:
                 newchar->hostile = 0;
                 break;
             case TYPE_SHOT:  // only target enemies
                 newchar->hostile = TYPE_ENEMY ;
                 break;
             case TYPE_NPC: // default npc behivior
                 newchar->hostile = TYPE_ENEMY ;
                 break;
        }
    }

    if(newchar->candamage<0) {// not been initialized, so initialize it
        switch (newchar->type){
             case TYPE_ENEMY:
                 newchar->candamage = TYPE_PLAYER | TYPE_SHOT;
                 if(newchar->subtype == SUBTYPE_ARROW) newchar->candamage |= TYPE_OBSTACLE;
                 break;
             case TYPE_PLAYER:
                 newchar->candamage = TYPE_PLAYER | TYPE_ENEMY | TYPE_OBSTACLE;
                 break;
             case TYPE_TRAP:
                 newchar->candamage  = TYPE_ENEMY | TYPE_PLAYER | TYPE_OBSTACLE;
             case TYPE_OBSTACLE:
                 newchar->candamage = TYPE_PLAYER | TYPE_ENEMY | TYPE_OBSTACLE;
                 break;
             case TYPE_SHOT:
                 newchar->candamage = TYPE_ENEMY | TYPE_PLAYER | TYPE_OBSTACLE;
                 break;
             case TYPE_NPC:
                 newchar->candamage = TYPE_ENEMY | TYPE_OBSTACLE;
                 break;
             case TYPE_ITEM:
                 newchar->candamage = TYPE_PLAYER;
                 break;
        }
    }

    if(newchar->projectilehit<0) {// not been initialized, so initialize it
        switch (newchar->type){
             case TYPE_ENEMY:
                 newchar->projectilehit = TYPE_ENEMY | TYPE_PLAYER | TYPE_OBSTACLE;
                 break;
             case TYPE_PLAYER:
                 newchar->projectilehit = TYPE_ENEMY | TYPE_PLAYER | TYPE_OBSTACLE;
                 break;
             case TYPE_TRAP: // hmm, don't really needed
                 newchar->projectilehit  = TYPE_ENEMY | TYPE_PLAYER | TYPE_OBSTACLE;
             case TYPE_OBSTACLE: // hmm, don't really needed
                 newchar->projectilehit = TYPE_ENEMY | TYPE_PLAYER | TYPE_OBSTACLE;
                 break;
             case TYPE_SHOT: // hmm, don't really needed
                 newchar->projectilehit = TYPE_ENEMY | TYPE_PLAYER | TYPE_OBSTACLE;
                 break;
             case TYPE_NPC:
                 newchar->projectilehit = TYPE_ENEMY | TYPE_PLAYER | TYPE_OBSTACLE;
                 break;
        }
    }

    if(newchar->jumpspeed < 0) newchar->jumpspeed = MAX(newchar->speed, 1);

    if(blendfx_is_set==0)
    {
        if(newchar->alpha)
        {
            blendfx[newchar->alpha-1] = 1;
        }
        if(newchar->gfxshadow || newchar->shadow)
        {
            blendfx[BLEND_MULTIPLY] = 1;
        }
    }

    // we need to convert 8bit colourmap into 24bit palette
    if(pixelformat==PIXEL_x8)
    {
        convert_map_to_palette(newchar, mapflag);
    }

	printf("Loading '%s'\n", newchar->name);
    return newchar;
}



int is_set(s_model * model, int m){    // New function to determine if a freespecial has been set
    int i;

    for(i = 0; i < model->specials_loaded; i++){
        if(model->special[i][MAX_SPECIAL_INPUTS-2] == m){
            return 1;
        }
    }

    return 0;
}

int load_script_setting()
{
    char * filename = "data/script.txt";
    char *buf, *command;
    ptrdiff_t pos = 0;
    size_t size = 0;

    if(buffer_pakfile(filename, &buf, &size)!=1) return 0;

    while(pos<size)
    {
        command = findarg(buf+pos, 0);
        if(command[0])
        {
            if(stricmp(command, "maxscriptvars")==0) // each script can have a variable list that can be accessed by index
            {
                max_script_vars = atoi(findarg(buf+pos, 1)) ;
                if(max_script_vars<0) max_script_vars = 0;
            }
            else if(stricmp(command, "maxentityvars")==0) // each entity can have a variable list that can be accessed by index
            {
                max_entity_vars = atoi(findarg(buf+pos, 1)) ;
                if(max_entity_vars<0) max_entity_vars = 0;
            }
            else if(stricmp(command, "maxindexedvars")==0) // a global variable list that can be accessed by index
            {
                max_indexed_vars = atoi(findarg(buf+pos, 1));
                if(max_indexed_vars<0) max_indexed_vars = 0;
            }
            else if(stricmp(command, "maxglobalvars")==0) // for global_var_list, default to 2048
            {
                max_global_vars = atoi(findarg(buf+pos, 1));
                if(max_global_vars<0) max_global_vars = 0;
            }
			else if(stricmp(command, "keyscriptrate")==0) // Rate that keyscripts fire when holding a key.
            {
                keyscriptrate = atoi(findarg(buf+pos, 1));
            }
        }
        // Go to next line
        while(buf[pos] && buf[pos]!='\n' && buf[pos]!='\r') ++pos;
        while(buf[pos]=='\n' || buf[pos]=='\r') ++pos;
    }

    if(buf != NULL)
    {
        tracefree(buf);
        buf = NULL;
    }
    return 1;
}

// Load / cache all models
int load_models()
{
	char filename[128] = "data/models.txt";
	int i;
	char *buf;
	size_t size;
	ptrdiff_t pos;
	char * command;
	char value1[128];
	char value2[128];
	char tmpBuff[128] = {""};
	int maxanim = MAX_ANIS; // temporary counter

	free_modelcache();

	if(loadingbg[0][0] > 0)
	{
		// New alternative background path for PSP
		if(custBkgrds != NULL)
		{
			strcpy(tmpBuff,custBkgrds);
			strncat(tmpBuff,"loading", 7);
			load_background(tmpBuff, 0);
		}
		else load_background("data/bgs/loading", 0);
	}
	else if(loadingbg[0][0] < 0)
	{
		clearscreen(vscreen);
		spriteq_clear();
	}
	if(loadingbg[0][0])
	{
		standard_palette(1);
		lifebar_colors();
		init_colourtable();
	}

	// reload default values
	max_idles        = MAX_IDLES;
	max_walks        = MAX_WALKS;
	max_ups          = MAX_UPS;
	max_downs        = MAX_DOWNS;
	max_backwalks    = MAX_BACKWALKS;
	max_attack_types = MAX_ATKS;
	max_freespecials = MAX_SPECIALS;
	max_follows      = MAX_FOLLOWS;
	max_attacks      = MAX_ATTACKS;
	max_animations   = MAX_ANIS;

	// free old values
	if(animspecials){tracefree(animspecials); animspecials = NULL;}
	if(animattacks){tracefree(animattacks);  animattacks = NULL;}
	if(animfollows){tracefree(animfollows);  animfollows = NULL;}
	if(animpains){tracefree(animpains);    animpains = NULL;}
	if(animfalls){tracefree(animfalls);    animfalls = NULL;}
	if(animrises){tracefree(animrises);    animrises = NULL;}
	if(animriseattacks){tracefree(animriseattacks);    animriseattacks = NULL;}
	if(animblkpains) {tracefree(animblkpains); animblkpains = NULL;}
	if(animdies){tracefree(animdies);     animdies = NULL;}
	if(animwalks){tracefree(animwalks);     animwalks = NULL;}
	if(animbackwalks){tracefree(animbackwalks);     animbackwalks = NULL;}
	if(animidles){tracefree(animidles);     animidles = NULL;}
	if(animups){tracefree(animups);     animups = NULL;}
	if(animdowns){tracefree(animdowns);     animdowns = NULL;}

	if(custModels != NULL)
	{
		strcpy(filename, "data/");
		strcat(filename, custModels);
	}

	// Read file
	if(buffer_pakfile(filename, &buf, &size)!=1) shutdown(1, "Error loading model list from %s", filename);

	pos = 0;
	while(pos<size) // peek global settings
	{
		command = findarg(buf+pos, 0);
		if(command[0])
		{
			if(stricmp(command, "maxidles")==0) // max idle stances
			{
				max_idles = atoi(findarg(buf+pos, 1));
				if(max_idles < MAX_IDLES) max_idles = MAX_IDLES;
			}
			if(stricmp(command, "maxwalks")==0) // max walks
			{
				max_walks = atoi(findarg(buf+pos, 1));
				if(max_walks < MAX_WALKS) max_walks = MAX_WALKS;
			}
			if(stricmp(command, "maxbackwalks")==0) // max backward walks
			{
				max_backwalks = atoi(findarg(buf+pos, 1));
				if(max_backwalks < MAX_BACKWALKS) max_backwalks = MAX_BACKWALKS;
			}
			if(stricmp(command, "maxups")==0) // max up walks
			{
				max_ups = atoi(findarg(buf+pos, 1));
				if(max_ups < MAX_UPS) max_ups = MAX_UPS;
			}
			if(stricmp(command, "maxdowns")==0) // max down walks
			{
				max_downs = atoi(findarg(buf+pos, 1));
				if(max_downs < MAX_DOWNS) max_downs = MAX_DOWNS;
			}
			if(stricmp(command, "maxattacktypes")==0) // max attacktype/pain/fall/die
			{
				max_attack_types = atoi(findarg(buf+pos, 1)) + STA_ATKS;
				if(max_attack_types < MAX_ATKS) max_attack_types = MAX_ATKS;
			}
			else if(stricmp(command, "maxfollows")==0) // max follow-ups
			{
				max_follows = atoi(findarg(buf+pos, 1));
				if(max_follows<MAX_FOLLOWS) max_follows = MAX_FOLLOWS;
			}
			else if(stricmp(command, "maxfreespecials")==0)// max freespecials
			{
				max_freespecials = atoi(findarg(buf+pos, 1));
				if(max_freespecials<MAX_SPECIALS) max_freespecials = MAX_SPECIALS;
			}
			else if(stricmp(command, "maxattacks")==0) // max attacks
			{
				max_attacks = atoi(findarg(buf+pos, 1));
				if(max_attacks<MAX_ATTACKS) max_attacks = MAX_ATTACKS;
			}
		}
		// Go to next line
		while(buf[pos] && buf[pos]!='\n' && buf[pos]!='\r') ++pos;
		while(buf[pos]=='\n' || buf[pos]=='\r') ++pos;
	}
	// calculate max animations
	max_animations += (max_attack_types - MAX_ATKS) * 6 +// multply by 5, for fall/die/pain/rise/blockpain/riseattack
			(max_follows - MAX_FOLLOWS) +
			(max_freespecials - MAX_SPECIALS) +
			(max_attacks - MAX_ATTACKS) +
			(max_idles - MAX_IDLES)+
			(max_walks - MAX_WALKS)+
			(max_ups - MAX_UPS)+
			(max_downs - MAX_DOWNS)+
			(max_backwalks - MAX_BACKWALKS);

    // alloc indexed animation ids
	animdowns       = (int*)tracemalloc("animdowns",        sizeof(int)*max_downs);
	animups         = (int*)tracemalloc("animups",          sizeof(int)*max_ups);
	animbackwalks   = (int*)tracemalloc("animbackwalks",    sizeof(int)*max_backwalks);
	animwalks       = (int*)tracemalloc("animwalks",        sizeof(int)*max_walks);
	animidles       = (int*)tracemalloc("animidles",        sizeof(int)*max_idles);
	animpains       = (int*)tracemalloc("animpains",        sizeof(int)*max_attack_types);
	animdies        = (int*)tracemalloc("animdies",         sizeof(int)*max_attack_types);
	animfalls       = (int*)tracemalloc("animfalls",        sizeof(int)*max_attack_types);
	animrises       = (int*)tracemalloc("animrises",        sizeof(int)*max_attack_types);
	animriseattacks = (int*)tracemalloc("animriseattacks",  sizeof(int)*max_attack_types);
	animblkpains    = (int*)tracemalloc("animblkpains",     sizeof(int)*max_attack_types);
	animattacks     = (int*)tracemalloc("animattacks",      sizeof(int)*max_attacks);
	animfollows     = (int*)tracemalloc("animfollows",      sizeof(int)*max_follows);
	animspecials    = (int*)tracemalloc("animspecials",     sizeof(int)*max_freespecials);

	// copy default values and new animation ids
	memcpy(animdowns, downs, sizeof(int)*MAX_DOWNS);
	for(i=MAX_DOWNS; i<max_downs; i++) animdowns[i] = maxanim++;
	memcpy(animups, ups, sizeof(int)*MAX_UPS);
	for(i=MAX_UPS; i<max_ups; i++) animups[i] = maxanim++;
	memcpy(animbackwalks, backwalks, sizeof(int)*MAX_BACKWALKS);
	for(i=MAX_BACKWALKS; i<max_backwalks; i++) animbackwalks[i] = maxanim++;
	memcpy(animwalks, walks, sizeof(int)*MAX_WALKS);
	for(i=MAX_WALKS; i<max_walks; i++) animwalks[i] = maxanim++;
	memcpy(animidles, idles, sizeof(int)*MAX_IDLES);
	for(i=MAX_IDLES; i<max_idles; i++) animidles[i] = maxanim++;
	memcpy(animspecials, freespecials,   sizeof(int)*MAX_SPECIALS);
	for(i=MAX_SPECIALS; i<max_freespecials; i++) animspecials[i] = maxanim++;
	memcpy(animattacks,  normal_attacks, sizeof(int)*MAX_ATTACKS);
	for(i=MAX_ATTACKS; i<max_attacks; i++) animattacks[i] = maxanim++;
	memcpy(animfollows,  follows,        sizeof(int)*MAX_FOLLOWS);
	for(i=MAX_FOLLOWS; i<max_follows; i++) animfollows[i] = maxanim++;
	memcpy(animpains,    pains,          sizeof(int)*MAX_ATKS);
	for(i=MAX_ATKS; i<max_attack_types; i++) animpains[i] = maxanim++;
	memcpy(animfalls,    falls,          sizeof(int)*MAX_ATKS);
	for(i=MAX_ATKS; i<max_attack_types; i++) animfalls[i] = maxanim++;
	memcpy(animrises,    rises,          sizeof(int)*MAX_ATKS);
	for(i=MAX_ATKS; i<max_attack_types; i++) animrises[i] = maxanim++;
	memcpy(animriseattacks,    riseattacks,          sizeof(int)*MAX_ATKS);
	for(i=MAX_ATKS; i<max_attack_types; i++) animriseattacks[i] = maxanim++;
	memcpy(animblkpains,    blkpains,    sizeof(int)*MAX_ATKS);
	for(i=MAX_ATKS; i<max_attack_types; i++) animblkpains[i] = maxanim++;
	memcpy(animdies,     deaths,         sizeof(int)*MAX_ATKS);
	for(i=MAX_ATKS; i<max_attack_types; i++) animdies[i] = maxanim++;

	// Now interpret the contents of buf line by line
	pos = 0;
	while(pos<size)
	{
		command = findarg(buf+pos, 0);
		if(command[0]) {
			if(stricmp(command, "music")==0){
				strncpy(value1, findarg(buf+pos, 1), 127);
				music(value1, 1, atol(findarg(buf+pos, 2)));
			}
			else if(stricmp(command, "load")==0){
				// Add path to cache list
				strncpy(value1, findarg(buf+pos, 1), 127);
				strncpy(value2, findarg(buf+pos, 2), 127);
				cache_model(value1, value2, 1);
			}
			else if(stricmp(command, "colourselect")==0){   // 6-2-2005 if string for colourselect found
				colourselect =  atoi(findarg(buf+pos, 1));          //  6-2-2005
			}
			else if(stricmp(command, "spdirection")==0){                // Select Player Direction for select player screen
				spdirection[0] =  atoi(findarg(buf+pos, 1));
				spdirection[1] =  atoi(findarg(buf+pos, 2));
				spdirection[2] =  atoi(findarg(buf+pos, 3));
				spdirection[3] =  atoi(findarg(buf+pos, 4));
			}
			else if(stricmp(command, "autoland")==0){    // New flag to determine if a player auto lands when thrown by another player (2 completely disables the ability to land)
				autoland = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "nolost")==0){    // this is use for dont lost your weapon if you grab a enemy flag it to 1 to no drop by tails
				nolost = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "ajspecial")==0){    // Flag to determine if a + j executes special
				ajspecial = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "nocost")==0){    // Nocost set in models.txt
				nocost = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "nocheats")==0){ //disable cheat option in menu
				forcecheatsoff =  atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "nodropen")==0){
				nodropen = 1;
			}
			else if(stricmp(command, "know")==0){
				// Just add path to cache list
				strncpy(value1, findarg(buf+pos, 1), 127);
				strncpy(value2, findarg(buf+pos, 2), 127);
				cache_model(value1, value2, 0);
			}
			else if(stricmp(command, "noaircancel")==0){
				noaircancel = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "nomaxrushreset")==0){
				nomaxrushreset[4] = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "mpblock")==0){               // Take from MP first?
				mpblock = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "blockratio")==0){            // Nullify or reduce damage?
				blockratio = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "nochipdeath")==0){
				nochipdeath = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "lifescore")==0){
				lifescore =  atoi(findarg(buf+pos, 1));
			}                    // Number of points needed to earn a 1-up
			else if(stricmp(command, "credscore")==0){
				credscore =  atoi(findarg(buf+pos, 1));
			}                    // Number of points needed to earn a credit
			else if(stricmp(command, "versusdamage")==0){
				versusdamage =  atoi(findarg(buf+pos, 1));
				if(versusdamage == 0 || versusdamage == 1) savedata.mode = versusdamage^1;
			}
			else if(!(stricmp(command, "maxdowns")==0 ||
				stricmp(command, "maxups")==0 ||
				stricmp(command, "maxbackwalks")==0 ||
				stricmp(command, "maxwalks")==0 ||
				stricmp(command, "maxidles")==0 ||
				stricmp(command, "maxattacktypes")==0 ||
				stricmp(command, "maxfollows")==0 ||
				stricmp(command, "maxfreespecials")==0 ||
				stricmp(command, "maxattacks")==0))
			{
				shutdown(1, "Command '%s' not understood in file '%s'!", command, filename);
			}
		}

		// Go to next line
		while(buf[pos] && buf[pos]!='\n' && buf[pos]!='\r') ++pos;
		while(buf[pos]=='\n' || buf[pos]=='\r') ++pos;
		if(loadingbg[0][0]) update_loading(loadingbg[0][1]+videomodes.hShift, loadingbg[0][2]+videomodes.vShift, loadingbg[0][3], loadingbg[0][4]+videomodes.hShift, loadingbg[0][5]+videomodes.vShift, pos/2, size, loadingbg[0][6]);
	}

	if(buf != NULL)
	{
		tracefree(buf);
		buf = NULL;
	}

    // Defer load_cached_model, so you can define models after their nested model.
	printf("\n");
	for(i=0; i<models_cached; i++)
	{
		//printf("Checking '%s' '%s'\n", model_cache[i].name, model_cache[i].path);
		if(loadingbg[0][0]) update_loading(loadingbg[0][1]+videomodes.hShift, loadingbg[0][2]+videomodes.vShift, loadingbg[0][3], loadingbg[0][4]+videomodes.hShift, loadingbg[0][5]+videomodes.vShift, size/2 + i*size/2/models_cached, size, loadingbg[0][6]);
		if(model_cache[i].loadflag)
		{
		load_cached_model(model_cache[i].name, "models.txt", 0);
		}
	}
	printf("\nLoading models...............\tDone!\n");
	return 1;
}




void unload_levelorder(){
    int i, j;
    for(j=0; j<MAX_DIFFICULTIES; j++){
        for(i=0; i<MAX_LEVELS; i++){
            if(levelorder[j][i] != NULL){
                tracefree(levelorder[j][i]->branchname);
                levelorder[j][i]->branchname = NULL;
                tracefree(levelorder[j][i]->filename);
                levelorder[j][i]->filename = NULL;
                tracefree(levelorder[j][i]);
                levelorder[j][i] = NULL;
            }
        }
        num_levels[j] = 0;
        strcpy(set_names[j], "");
    }
    num_difficulties = 0;

    if(skipselect)
    {
        for(i=0; i<MAX_DIFFICULTIES; i++)
        {
            for(j=0; j<MAX_PLAYERS; j++)
            {
                if((*skipselect)[i][j])
                {
                    tracefree((*skipselect)[i][j]);
                    (*skipselect)[i][j] = NULL;
                }
            }
        }
        tracefree(skipselect);
        skipselect = NULL;
    }
}



// Add a level to the level order
void add_level(char *filename, int diff){
    int len, Zs[3] = {0,0,0};

    if(z_coords[0] > 0) Zs[0] = z_coords[0];
    else Zs[0] = PLAYER_MIN_Z;

    if(z_coords[1] > 0) Zs[1] = z_coords[1];
    else Zs[1] = PLAYER_MAX_Z;

    if(z_coords[2] > 0) Zs[2] = z_coords[2];
    else Zs[2] = PLAYER_MIN_Z;

    if(diff > MAX_DIFFICULTIES) return;
    if(num_levels[diff] >= MAX_LEVELS) shutdown(1, "Too many entries in level order (max. %i)!", MAX_LEVELS);

    levelorder[diff][num_levels[diff]] = tracemalloc("add_level", sizeof(s_level_entry));
    memset(levelorder[diff][num_levels[diff]], 0, sizeof(s_level_entry));

    len = strlen(branch_name);
    levelorder[diff][num_levels[diff]]->branchname = tracemalloc("add_level(bn)", len + 1);
    strcpy(levelorder[diff][num_levels[diff]]->branchname, branch_name);
    levelorder[diff][num_levels[diff]]->branchname[len] = 0;

    len = strlen(filename);
    levelorder[diff][num_levels[diff]]->filename = tracemalloc("add_level(fn)", len + 1);
    strcpy(levelorder[diff][num_levels[diff]]->filename, filename);
    levelorder[diff][num_levels[diff]]->filename[len] = 0;

    levelorder[diff][num_levels[diff]]->z_coords[0] = Zs[0];
    levelorder[diff][num_levels[diff]]->z_coords[1] = Zs[1];
    levelorder[diff][num_levels[diff]]->z_coords[2] = Zs[2];
    num_levels[diff]++;
}



// Add a scene to the level order
void add_scene(char *filename, int diff){
    int len;
    if(diff > MAX_DIFFICULTIES) return;
    if(num_levels[diff] >= MAX_LEVELS) shutdown(1, "Too many entries in level order (max. %i)!", MAX_LEVELS);

    levelorder[diff][num_levels[diff]] = (s_level_entry*)tracemalloc("add_scene",sizeof(s_level_entry));
    memset(levelorder[diff][num_levels[diff]], 0, sizeof(s_level_entry));

    len = strlen(branch_name);
    levelorder[diff][num_levels[diff]]->branchname = tracemalloc("add_scene(bn)", len + 1);
    strcpy(levelorder[diff][num_levels[diff]]->branchname, branch_name);
    levelorder[diff][num_levels[diff]]->branchname[len] = 0;

    len = strlen(filename);
    levelorder[diff][num_levels[diff]]->filename = tracemalloc("add_scene(fn)", len + 1);
    strcpy(levelorder[diff][num_levels[diff]]->filename, filename);
    levelorder[diff][num_levels[diff]]->filename[len] = 0;

    levelorder[diff][num_levels[diff]]->type = cut_scene;
    num_levels[diff]++;
}

// Add a select screen file to the level order
void add_select(char *filename, int diff){
    int len;
    if(diff > MAX_DIFFICULTIES) return;
    if(num_levels[diff] >= MAX_LEVELS) shutdown(1, "Too many entries in level order (max. %i)!", MAX_LEVELS);

    levelorder[diff][num_levels[diff]] = (s_level_entry*)tracemalloc("add_select",sizeof(s_level_entry));
    memset(levelorder[diff][num_levels[diff]], 0, sizeof(s_level_entry));

    len = strlen(branch_name);
    levelorder[diff][num_levels[diff]]->branchname = tracemalloc("add_select(bn)", len + 1);
    strcpy(levelorder[diff][num_levels[diff]]->branchname, branch_name);
    levelorder[diff][num_levels[diff]]->branchname[len] = 0;

    len = strlen(filename);
    levelorder[diff][num_levels[diff]]->filename = tracemalloc("add_select(fn)", len + 1);
    strcpy(levelorder[diff][num_levels[diff]]->filename, filename);
    levelorder[diff][num_levels[diff]]->filename[len] = 0;

    levelorder[diff][num_levels[diff]]->type = select_screen;
    num_levels[diff]++;
}

static void _readbarstatus(char* buf, s_barstatus* pstatus)
{
    char* value;
    if((value=findarg(buf, 1))[0]) pstatus->sizex       = atoi(value);
    else return;
    if((value=findarg(buf, 2))[0]) pstatus->sizey       = atoi(value);
    else return;
    if((value=findarg(buf, 3))[0]) pstatus->noborder    = atoi(value);
    else return;
    if((value=findarg(buf, 4))[0]) pstatus->type        = atoi(value);
    else return;
    if((value=findarg(buf, 5))[0]) pstatus->orientation = atoi(value);
    else return;
    if((value=findarg(buf, 6))[0]) pstatus->borderlayer = atoi(value);
    else return;
    if((value=findarg(buf, 7))[0]) pstatus->shadowlayer = atoi(value);
    else return;
    if((value=findarg(buf, 8))[0]) pstatus->barlayer    = atoi(value);
    else return;
    if((value=findarg(buf, 9))[0]) pstatus->backlayer   = atoi(value);
    else return;
}


// Load list of levels
void load_levelorder()
{
    char filename[128] = "";
    int i;
    char *buf;
    size_t size;
    int pos;
    int current_set;
    char * command;
    char* arg;
	char value[128]   = {""};
    int plifeUsed[2]  = {0,0};
    int elifeUsed[2]  = {0,0};
    int piconUsed[2]  = {0,0};
    int piconwUsed[2] = {0,0};
    int eiconUsed[4]  = {0,0,0,0};
    int pmpUsed[4]    = {0,0,0,0};
    int plifeXused[4] = {0,0,0,0};        // 4-7-2006 New custimizable variable for players 'x'
    int plifeNused[4] = {0,0,0,0};        // 4-7-2006 New custimizable variable for players 'lives'
    int enameused[4]  = {0,0,0,0};        // 4-7-2006 New custimizable variable for enemy names
    int pnameJused[4] = {0,0,0,0};        // 1-8-2006 New custimizable variable for players name Select Hero
    int pscoreUsed[4] = {0,0,0,0};        // 1-8-2006 New custimizable variable for players name Select Hero

    unload_levelorder();

    if(custLevels != NULL)
	{
        strcpy(filename,"data/");
        strcat(filename,custLevels);
    }
    else strcpy(filename,"data/levels.txt");

    // Read file

    if(buffer_pakfile(filename, &buf, &size)!=1) shutdown(1, "Error loading level list from %s", filename);

    // Now interpret the contents of buf line by line
    pos = 0;
    current_set = -1;

    // Custom lifebar/timebox/icon positioning and size
    picon[0][0] = piconw[0][0] = picon[2][0] = piconw[2][0] = eicon[0][0] = eicon[2][0] = 2;
    picon[1][0] = piconw[1][0] = picon[3][0] = piconw[3][0] = eicon[1][0] = eicon[3][0] = 2 + P2_STATS_DIST;
    picon[0][1] = piconw[0][1] = picon[1][1] = piconw[1][1] = 2;
    picon[2][1] = piconw[2][1] = picon[3][1] = piconw[3][1] = 202;
    plife[0][0] = pmp[0][0] = plife[2][0] = pmp[2][0] = elife[0][0] = elife[2][0] = 20;
    plife[1][0] = pmp[1][0] = plife[3][0] = pmp[3][0] = elife[1][0] = elife[3][0] = 20 + P2_STATS_DIST;
    plife[0][1] = plife[1][1] = 10;
    plife[2][1] = plife[3][1] = 210;
    pmp[0][1] = pmp[1][1] = 18;
    pmp[2][1] = pmp[3][1] = 218;

    memset(psmenu, 0, sizeof(int)*4*4);

    eicon[0][1] = eicon[1][1] = 19;
    eicon[2][1] = eicon[3][1] = 220;
    elife[0][1] = elife[1][1] = 27;
    elife[2][1] = elife[3][1] = 227;

    timeloc[0] = 149;
    timeloc[1] = 4;
    timeloc[2] = 21;
    timeloc[3] = 20;
    timeloc[4] = 0;

    lbarstatus.sizex  = mpbarstatus.sizex = 100;
    lbarstatus.sizey  = 5;
    mpbarstatus.sizey = 3;
    lbarstatus.noborder = mpbarstatus.noborder = 0;

    // Show Complete Default Values
    scomplete[0] = 75;
    scomplete[1] = 60;
    scomplete[2] = 0;
    scomplete[3] = 0;
    scomplete[4] = 0;
    scomplete[5] = 0;

    // Show Complete Y Values
    cbonus[0] = lbonus[0] = rbonus[0] = tscore[0] = 10;
    cbonus[1] = cbonus[3] = cbonus[5] = cbonus[7] = cbonus[9] = 100;
    lbonus[1] = lbonus[3] = lbonus[5] = lbonus[7] = lbonus[9] = 120;
    rbonus[1] = rbonus[3] = rbonus[5] = rbonus[7] = rbonus[9] = 140;
    tscore[1] = tscore[3] = tscore[5] = tscore[7] = tscore[9] = 160;

    // Show Complete X Values
    cbonus[2] = lbonus[2] = rbonus[2] = tscore[2] = 100;
    cbonus[4] = lbonus[4] = rbonus[4] = tscore[4] = 155;
    cbonus[6] = lbonus[6] = rbonus[6] = tscore[6] = 210;
    cbonus[8] = lbonus[8] = rbonus[8] = tscore[8] = 265;


    while(pos<size){
        command = findarg(buf+pos, 0);
        if(command[0]){
			if(stricmp(command, "blendfx")==0){
				for(i=0; i<MAX_BLENDINGS; i++)
				{
					if(atoi(findarg(buf+pos, i+1))) blendfx[i] = 1;
					else blendfx[i] = 0;
				}
				blendfx_is_set = 1;
            }
            else if(stricmp(command, "set")==0){
                if(num_difficulties>=MAX_DIFFICULTIES)
                    shutdown(1, "Too many sets of levels (max %u)!", MAX_DIFFICULTIES);

                ++num_difficulties;
                ++current_set;
                strncpy(set_names[current_set], findarg(buf+pos, 1), MAX_NAME_LEN);
                ifcomplete[current_set] = 0;
                cansave_flag[current_set] = 1; // default to 1, so the level can be saved
                branch_name[0] = 0;
            }
            else if(stricmp(command, "ifcomplete")==0){
                if(current_set<0)
                    shutdown(1, "Error in level order: a set must be specified.");

                ifcomplete[current_set] = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "skipselect")==0){
                if(current_set<0)
                    shutdown(1, "Error in level order: a set must be specified.");

                if(!skipselect)
                {
                    skipselect = tracemalloc("skipselect", sizeof(*skipselect));
                    memset(skipselect, 0, sizeof(*skipselect));
                }

                for(i=0; i<4;i++)
                {
                    if((arg=findarg(buf+pos, i+1))[0])
                    {
                        if(!(*skipselect)[current_set][i])
                            (*skipselect)[current_set][i] = tracemalloc("skipselect#2", MAX_NAME_LEN+1);
                        strncpy((*skipselect)[current_set][i], arg, MAX_NAME_LEN);
                    }
                }
            }
            else if(stricmp(command, "file")==0){
                if(current_set<0)
                    shutdown(1, "Error in level order: a set must be specified.");

                strncpy(value, findarg(buf+pos, 1), 127);
                add_level(value, current_set);
            }
            else if(stricmp(command, "scene")==0){
                if(current_set<0)
                    shutdown(1, "Error in level order: a set must be specified.");

                strncpy(value, findarg(buf+pos, 1), 127);
                add_scene(value, current_set);
            }
            else if(stricmp(command, "select")==0){
                if(current_set<0)
                    shutdown(1, "Error in level order: a set must be specified.");

                strncpy(value, findarg(buf+pos, 1), 127);
                add_select(value, current_set);
            }
            else if(stricmp(command, "next")==0){
                if(current_set<0)
                    shutdown(1, "Error in level order: a set must be specified.");

                // Set 'gonext' flag of last loaded level
                if(num_levels[current_set]<1)
                    shutdown(1, "Error in level order (next before file)!");

                levelorder[current_set][num_levels[current_set]-1]->gonext = 1;
            }
            else if(stricmp(command, "end")==0){
                if(current_set<0)
                    shutdown(1, "Error in level order: a set must be specified.");

                // Set endgame flag of last loaded level
                if(num_levels[current_set]<1)
                    shutdown(1, "Error in level order (next before file)!");

                levelorder[current_set][num_levels[current_set]-1]->gonext = 2;
            }
            // 7-1-2005  credits/lives/singleplayer start here
            // used to read the new # of lives/credits from the levels.txt
            else if(stricmp(command, "lives")==0){
                if(current_set<0)
                    shutdown(1, "Error in level order: a set must be specified.");

                difflives[current_set] = atoi(findarg(buf+pos, 1));
            }

			else if(stricmp(command, "disablehof")==0){
                if(current_set<0)
                    shutdown(1, "Error in level order: a set must be specified.");

                noshowhof[current_set] = atoi(findarg(buf+pos, 1));
            }
            // 07-12-31
            // 0 this set can't be saved
            // 1 save level only
            // 2 save player info and level, can't choose player in select menu
            else if(stricmp(command, "cansave")==0){
                if(current_set<0)
                    shutdown(1, "Error in level order: a set must be specified.");

                cansave_flag[current_set] = atoi(findarg(buf+pos, 1));
            }
            //    2-10-05  adjust the walkable coordinates
            else if(stricmp(command, "z")==0){
                if(current_set<0)
                    shutdown(1, "Error in level order: a set must be specified.");

                z_coords[0] = atoi(findarg(buf+pos, 1));
                z_coords[1] = atoi(findarg(buf+pos, 2));
                z_coords[2] = atoi(findarg(buf+pos, 3));
            }
            //    2007-2-22 level branch name
            else if(stricmp(command, "branch")==0){
                if(current_set<0)
                    shutdown(1, "Error in level order: a set must be specified.");

                strncpy(branch_name, findarg(buf+pos, 1), MAX_NAME_LEN);
            }
            else if(stricmp(command, "p1life")==0){
                if((arg=findarg(buf+pos, 1))[0]) plife[0][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) plife[0][1] = atoi(arg);
            }
            else if(stricmp(command, "p2life")==0){
                if((arg=findarg(buf+pos, 1))[0]) plife[1][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) plife[1][1] = atoi(arg);
            }
            else if(stricmp(command, "p3life")==0){
                if((arg=findarg(buf+pos, 1))[0]) plife[2][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) plife[2][1] = atoi(arg);
                plifeUsed[0] = 1;
            }
            else if(stricmp(command, "p4life")==0){
                if((arg=findarg(buf+pos, 1))[0]) plife[3][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) plife[3][1] = atoi(arg);
                plifeUsed[1] = 1;
            }
            else if(stricmp(command, "p1mp")==0){
                if((arg=findarg(buf+pos, 1))[0]) pmp[0][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) pmp[0][1] = atoi(arg);
                pmpUsed[0] = 1;
            }
            else if(stricmp(command, "p2mp")==0){
                if((arg=findarg(buf+pos, 1))[0]) pmp[1][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) pmp[1][1] = atoi(arg);
                pmpUsed[1] = 1;
            }
            else if(stricmp(command, "p3mp")==0){
                if((arg=findarg(buf+pos, 1))[0]) pmp[2][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) pmp[2][1] = atoi(arg);
                pmpUsed[2] = 1;
            }
            else if(stricmp(command, "p4mp")==0){
                if((arg=findarg(buf+pos, 1))[0]) pmp[3][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) pmp[3][1] = atoi(arg);
                pmpUsed[3] = 1;
            }
            else if(stricmp(command, "p1lifex")==0){
				for(i=0; i<3; i++)
					if((arg=findarg(buf+pos, i+1))[0]) plifeX[0][i] = atoi(arg);
                plifeXused[0] = 1;
            }
            else if(stricmp(command, "p2lifex")==0){
                for(i=0; i<3; i++)
					if((arg=findarg(buf+pos, i+1))[0]) plifeX[1][i] = atoi(arg);
                plifeXused[1] = 1;
            }
            else if(stricmp(command, "p3lifex")==0){
                for(i=0; i<3; i++)
					if((arg=findarg(buf+pos, i+1))[0]) plifeX[2][i] = atoi(arg);
                plifeXused[2] = 1;
            }
            else if(stricmp(command, "p4lifex")==0){
                for(i=0; i<3; i++)
					if((arg=findarg(buf+pos, i+1))[0]) plifeX[3][i] = atoi(arg);
                plifeXused[3] = 1;
            }
            else if(stricmp(command, "p1lifen")==0){
				for(i=0; i<3; i++)
					if((arg=findarg(buf+pos, i+1))[0]) plifeN[0][i] = atoi(arg);
                plifeNused[0] = 1;
            }
            else if(stricmp(command, "p2lifen")==0){
                for(i=0; i<3; i++)
					if((arg=findarg(buf+pos, i+1))[0]) plifeN[1][i] = atoi(arg);
                plifeNused[1] = 1;
            }
            else if(stricmp(command, "p3lifen")==0){
                for(i=0; i<3; i++)
					if((arg=findarg(buf+pos, i+1))[0]) plifeN[2][i] = atoi(arg);
                plifeNused[2] = 1;
            }
            else if(stricmp(command, "p4lifen")==0){
                for(i=0; i<3; i++)
					if((arg=findarg(buf+pos, i+1))[0]) plifeN[3][i] = atoi(arg);
                plifeNused[3] = 1;
            }
            else if(stricmp(command, "e1life")==0){
                if((arg=findarg(buf+pos, 1))[0]) elife[0][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) elife[0][1] = atoi(arg);
            }
            else if(stricmp(command, "e2life")==0){
                if((arg=findarg(buf+pos, 1))[0]) elife[1][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) elife[1][1] = atoi(arg);
            }
            else if(stricmp(command, "e3life")==0){
                if((arg=findarg(buf+pos, 1))[0]) elife[2][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) elife[2][1] = atoi(arg);
                elifeUsed[0] = 1;
            }
            else if(stricmp(command, "e4life")==0){
                if((arg=findarg(buf+pos, 1))[0]) elife[3][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) elife[3][1] = atoi(arg);
                elifeUsed[1] = 1;
            }
            else if(stricmp(command, "p1icon")==0){
                if((arg=findarg(buf+pos, 1))[0]) picon[0][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) picon[0][1] = atoi(arg);
            }
            else if(stricmp(command, "p2icon")==0){
                if((arg=findarg(buf+pos, 1))[0]) picon[1][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) picon[1][1] = atoi(arg);
            }
            else if(stricmp(command, "p3icon")==0){
                if((arg=findarg(buf+pos, 1))[0]) picon[2][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) picon[2][1] = atoi(arg);
                piconUsed[0] = 1;
            }
            else if(stricmp(command, "p4icon")==0){
                if((arg=findarg(buf+pos, 1))[0]) picon[3][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) picon[3][1] = atoi(arg);
                piconUsed[1] = 1;
            }
            else if(stricmp(command, "p1iconw")==0){
                if((arg=findarg(buf+pos, 1))[0]) piconw[0][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) piconw[0][1] = atoi(arg);
            }
            else if(stricmp(command, "p2iconw")==0){
                if((arg=findarg(buf+pos, 1))[0]) piconw[1][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) piconw[1][1] = atoi(arg);
            }
            else if(stricmp(command, "p3iconw")==0){
                if((arg=findarg(buf+pos, 1))[0]) piconw[2][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) piconw[2][1] = atoi(arg);
                piconwUsed[0] = 1;
            }
            else if(stricmp(command, "p4iconw")==0){
                if((arg=findarg(buf+pos, 1))[0]) piconw[3][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) piconw[3][1] = atoi(arg);
                piconwUsed[1] = 1;
            }
            else if(stricmp(command, "mp1icon")==0){
                if((arg=findarg(buf+pos, 1))[0]) mpicon[0][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) mpicon[0][1] = atoi(arg);
            }
            else if(stricmp(command, "mp2icon")==0){
                if((arg=findarg(buf+pos, 1))[0]) mpicon[1][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) mpicon[1][1] = atoi(arg);
            }
            else if(stricmp(command, "mp3icon")==0){
                if((arg=findarg(buf+pos, 1))[0]) mpicon[2][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) mpicon[2][1] = atoi(arg);
            }
            else if(stricmp(command, "mp4icon")==0){
                if((arg=findarg(buf+pos, 1))[0]) mpicon[3][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) mpicon[3][1] = atoi(arg);
            }
            else if(stricmp(command, "p1namej")==0){
				for(i=0; i<7; i++)
					if((arg=findarg(buf+pos, i+1))[0]) pnameJ[0][i] = atoi(arg);
                pnameJused[0] = 1;
            }
            else if(stricmp(command, "p2namej")==0){
				for(i=0; i<7; i++)
					if((arg=findarg(buf+pos, i+1))[0]) pnameJ[1][i] = atoi(arg);
                pnameJused[1] = 1;
            }
            else if(stricmp(command, "p3namej")==0){
				for(i=0; i<7; i++)
					if((arg=findarg(buf+pos, i+1))[0]) pnameJ[2][i] = atoi(arg);
                pnameJused[2] = 1;
            }
            else if(stricmp(command, "p4namej")==0){
				for(i=0; i<7; i++)
					if((arg=findarg(buf+pos, i+1))[0]) pnameJ[3][i] = atoi(arg);
                pnameJused[3] = 1;
            }
            else if(stricmp(command, "p1score")==0){
				for(i=0; i<7; i++)
					if((arg=findarg(buf+pos, i+1))[0]) pscore[0][i] = atoi(arg);
                pscoreUsed[0] = 1;
            }
            else if(stricmp(command, "p2score")==0){
				for(i=0; i<7; i++)
					if((arg=findarg(buf+pos, i+1))[0]) pscore[1][i] = atoi(arg);
                pscoreUsed[1] = 1;
            }
            else if(stricmp(command, "p3score")==0){
				for(i=0; i<7; i++)
					if((arg=findarg(buf+pos, i+1))[0]) pscore[2][i] = atoi(arg);
                pscoreUsed[2] = 1;
            }
            else if(stricmp(command, "p4score")==0){
				for(i=0; i<7; i++)
					if((arg=findarg(buf+pos, i+1))[0]) pscore[3][i] = atoi(arg);
                pscoreUsed[3] = 1;
            }
            else if(stricmp(command, "p1shoot")==0){
				for(i=0; i<3; i++)
					if((arg=findarg(buf+pos, i+1))[0]) pshoot[0][i] = atoi(arg);
            }
            else if(stricmp(command, "p2shoot")==0){
                for(i=0; i<3; i++)
					if((arg=findarg(buf+pos, i+1))[0]) pshoot[1][i] = atoi(arg);
            }
            else if(stricmp(command, "p3shoot")==0){
                for(i=0; i<3; i++)
					if((arg=findarg(buf+pos, i+1))[0]) pshoot[2][i] = atoi(arg);
            }
            else if(stricmp(command, "p4shoot")==0){
                for(i=0; i<3; i++)
					if((arg=findarg(buf+pos, i+1))[0]) pshoot[3][i] = atoi(arg);
            }
            else if(stricmp(command, "p1rush")==0){
				for(i=0; i<8; i++)
					if((arg=findarg(buf+pos, i+1))[0]) prush[0][i] = atoi(arg);
            }
            else if(stricmp(command, "p2rush")==0){
				for(i=0; i<8; i++)
					if((arg=findarg(buf+pos, i+1))[0]) prush[1][i] = atoi(arg);
            }
            else if(stricmp(command, "p3rush")==0){
				for(i=0; i<8; i++)
					if((arg=findarg(buf+pos, i+1))[0]) prush[2][i] = atoi(arg);
            }
            else if(stricmp(command, "p4rush")==0){
				for(i=0; i<8; i++)
					if((arg=findarg(buf+pos, i+1))[0]) prush[3][i] = atoi(arg);
            }
            else if(stricmp(command, "e1icon")==0){
                if((arg=findarg(buf+pos, 1))[0]) eicon[0][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) eicon[0][1] = atoi(arg);
            }
            else if(stricmp(command, "e2icon")==0){
                if((arg=findarg(buf+pos, 1))[0]) eicon[1][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) eicon[1][1] = atoi(arg);
            }
            else if(stricmp(command, "e3icon")==0){
                if((arg=findarg(buf+pos, 1))[0]) eicon[2][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) eicon[2][1] = atoi(arg);
                eiconUsed[0] = 1;
            }
            else if(stricmp(command, "e4icon")==0){
                if((arg=findarg(buf+pos, 1))[0]) eicon[3][0] = atoi(arg);
                if((arg=findarg(buf+pos, 2))[0]) eicon[3][1] = atoi(arg);
                eiconUsed[1] = 1;
            }
            else if(stricmp(command, "e1name")==0){
                for(i=0; i<3; i++)
					if((arg=findarg(buf+pos, i+1))[0]) ename[0][i] = atoi(arg);
                enameused[0] = 1;
            }
            else if(stricmp(command, "e2name")==0){
                for(i=0; i<3; i++)
					if((arg=findarg(buf+pos, i+1))[0]) ename[1][i] = atoi(arg);
                enameused[1] = 1;
            }
            else if(stricmp(command, "e3name")==0){
                for(i=0; i<3; i++)
					if((arg=findarg(buf+pos, i+1))[0]) ename[2][i] = atoi(arg);
                enameused[2] = 1;
            }
            else if(stricmp(command, "e4name")==0){
                for(i=0; i<3; i++)
					if((arg=findarg(buf+pos, i+1))[0]) ename[3][i] = atoi(arg);
                enameused[3] = 1;
            }
            else if(stricmp(command, "p1smenu")==0){
				for(i=0; i<4; i++)
					if((arg=findarg(buf+pos, i+1))[0]) psmenu[0][i] = atoi(arg);
            }
            else if(stricmp(command, "p2smenu")==0){
				for(i=0; i<4; i++)
					if((arg=findarg(buf+pos, i+1))[0]) psmenu[1][i] = atoi(arg);
            }
            else if(stricmp(command, "p3smenu")==0){
				for(i=0; i<4; i++)
					if((arg=findarg(buf+pos, i+1))[0]) psmenu[2][i] = atoi(arg);
            }
            else if(stricmp(command, "p4smenu")==0){
				for(i=0; i<4; i++)
					if((arg=findarg(buf+pos, i+1))[0]) psmenu[3][i] = atoi(arg);
            }
            else if(stricmp(command, "timeicon")==0){
   				strncpy(timeicon_path, findarg(buf+pos, 1), 127);
                timeicon = loadsprite(timeicon_path,0,0,pixelformat);
                if((arg=findarg(buf+pos, 2))[0]) timeicon_offsets[0] = atoi(arg);
                if((arg=findarg(buf+pos, 3))[0]) timeicon_offsets[1] = atoi(arg);
            }
            else if(stricmp(command, "bgicon")==0){
				strncpy(bgicon_path, findarg(buf+pos, 1), 127);
                bgicon = loadsprite(bgicon_path,0,0,pixelformat);
                if((arg=findarg(buf+pos, 2))[0]) bgicon_offsets[0] = atoi(arg);
                if((arg=findarg(buf+pos, 3))[0]) bgicon_offsets[1] = atoi(arg);
				if((arg=findarg(buf+pos, 4))[0]) bgicon_offsets[2] = atoi(arg);
				else bgicon_offsets[2] = HUD_Z / 2;
            }
			else if(stricmp(command, "olicon")==0){
				strncpy(olicon_path, findarg(buf+pos, 1), 127);
                olicon = loadsprite(olicon_path,0,0,pixelformat);
                if((arg=findarg(buf+pos, 2))[0]) olicon_offsets[0] = atoi(arg);
                if((arg=findarg(buf+pos, 3))[0]) olicon_offsets[1] = atoi(arg);
				if((arg=findarg(buf+pos, 4))[0]) olicon_offsets[2] = atoi(arg);
                else olicon_offsets[2] = HUD_Z * 3;
            }
            else if(stricmp(command, "timeloc")==0){
				for(i=0; i<6; i++)
					if((arg=findarg(buf+pos, i+1))[0]) timeloc[i] = atoi(arg);
            }
            else if(stricmp(command, "lbarsize")==0){
				_readbarstatus(buf+pos, &lbarstatus);
			}
			else if(stricmp(command, "olbarsize")==0){
				_readbarstatus(buf+pos, &olbarstatus);
			}
            else if(stricmp(command, "mpbarsize")==0){
				_readbarstatus(buf+pos, &mpbarstatus);
			}
			else if(stricmp(command, "lbartext")==0){
				for(i=0; i<4; i++)
					if((arg=findarg(buf+pos, i+1))[0]) lbartext[i] = atoi(arg);
			}
			else if(stricmp(command, "mpbartext")==0){
				for(i=0; i<4; i++)
					if((arg=findarg(buf+pos, i+1))[0]) mpbartext[i] = atoi(arg);
			}
            else if(stricmp(command, "showcomplete")==0){
				for(i=0; i<6; i++)
					if((arg=findarg(buf+pos, i+1))[0]) scomplete[i] = atoi(arg);
            }
            else if(stricmp(command, "clearbonus")==0){
				for(i=0; i<10; i++)
					if((arg=findarg(buf+pos, i+1))[0]) cbonus[i] = atoi(arg);
            }
			else if(stricmp(command, "rushbonus")==0){
				for(i=0; i<10; i++)
					if((arg=findarg(buf+pos, i+1))[0]) rbonus[i] = atoi(arg);
            }
            else if(stricmp(command, "lifebonus")==0){
				for(i=0; i<10; i++)
					if((arg=findarg(buf+pos, i+1))[0]) lbonus[i] = atoi(arg);
            }
			else if(stricmp(command, "scbonuses")==0){
				for(i=0; i<4; i++)
					if((arg=findarg(buf+pos, i+1))[0]) scbonuses[i] = atoi(arg);
            }
            else if(stricmp(command, "totalscore")==0){
				for(i=0; i<10; i++)
					if((arg=findarg(buf+pos, i+1))[0]) tscore[i] = atoi(arg);
            }
            else if(stricmp(command, "musicoverlap")==0){
                if(current_set<0)
                    shutdown(1, "Error in level order: a set must be specified.");

                diffoverlap[current_set] = atoi(findarg(buf+pos, 1));
            }
			else if(stricmp(command, "showrushbonus")==0){
                showrushbonus = 1;
            }
            else if(stricmp(command, "noslowfx")==0){
                noslowfx = 1;
            }
            else if(stricmp(command, "equalairpause")==0){
                equalairpause = 1;
            }
            else if(stricmp(command, "hiscorebg")==0){
                hiscorebg = 1;
            }
            else if(stricmp(command, "completebg")==0){
                completebg = 1;
            }
			else if(stricmp(command, "loadingbg")==0){
				for(i=0; i<7; i++) loadingbg[0][i] = atoi(findarg(buf+pos, i+1));
            }
			else if(stricmp(command, "loadingbg2")==0){
				for(i=0; i<7; i++) loadingbg[1][i] = atoi(findarg(buf+pos, i+1));
	        }
			else if(stricmp(command, "loadingmusic")==0){
                loadingmusic = atoi(findarg(buf+pos, 1));
			}
            else if(stricmp(command, "unlockbg")==0){
                unlockbg = 1;
            }
            else if(stricmp(command, "noshare")==0){
                noshare = 1;
            }

            //8-2-2005 custom fade
            else if(stricmp(command, "custfade")==0){
                if(current_set<0)
                    shutdown(1, "Error in level order: a set must be specified.");

                custfade[current_set] = atoi(findarg(buf+pos, 1));
            }
            //8-2-2005 custom fade end
			//continuescore
			else if(stricmp(command, "continuescore")==0){
                if(current_set<0)
                    shutdown(1, "Error in level order: a set must be specified.");

                continuescore[current_set] = atoi(findarg(buf+pos, 1));
            }

            else if(stricmp(command, "credits")==0){
                if(current_set<0)
                    shutdown(1, "Error in level order: a set must be specified.");

                diffcreds[current_set] = atoi(findarg(buf+pos, 1));
            }
            //typemp for change for mp restored by time (0) to by enemys (1) or no restore (2) by tails
            else if(stricmp(command, "typemp")==0){
                if(current_set<0)
                    shutdown(1, "Error in level order: a set must be specified.");

                typemp[current_set] = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "single")==0){
                if(current_set<0)
                {
                    for(i=0; i<MAX_DIFFICULTIES; i++)
                        maxplayers[i] = ctrlmaxplayers[i] = 1;
                }
                else
                {
                    maxplayers[current_set] = ctrlmaxplayers[current_set] = 1;
                }
            }  // 7-1-2005  credits/lives/singleplayer end here
            else if(stricmp(command, "maxplayers")==0){
                if(current_set<0)
                {
                    maxplayers[0] = atoi(findarg(buf+pos, 1));
                    for(i=0; i<MAX_DIFFICULTIES; i++)
                        maxplayers[i] = ctrlmaxplayers[i] = maxplayers[0];
                }
                else
                {
                    maxplayers[current_set] = ctrlmaxplayers[current_set] = atoi(findarg(buf+pos, 1));
                }
            }
            else if(stricmp(command, "nosame")==0){
                if(current_set<0)
                    shutdown(1, "Error in level order: a set must be specified.");

                same[current_set] = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "rush")==0){
                rush[0] = atoi(findarg(buf+pos, 1));
                rush[1] = atoi(findarg(buf+pos, 2));
                strncpy(rush_names[0], findarg(buf+pos, 3), MAX_NAME_LEN);
                rush[2] = atoi(findarg(buf+pos, 4));
                rush[3] = atoi(findarg(buf+pos, 5));
                strncpy(rush_names[1], findarg(buf+pos, 6), MAX_NAME_LEN);
                rush[4] = atoi(findarg(buf+pos, 7));
                rush[5] = atoi(findarg(buf+pos, 8));
            }
            else if(stricmp(command, "maxwallheight")==0){
                MAX_WALL_HEIGHT = atoi(findarg(buf+pos, 1));
                if(MAX_WALL_HEIGHT < 0) MAX_WALL_HEIGHT = 1000;
            }
			else if(stricmp(command, "scoreformat")==0){
                scoreformat = atoi(findarg(buf+pos, 1));
            }
            else {
                if(buf != NULL){
                    tracefree(buf);
                    buf = NULL;
                }
                shutdown(1, "Command '%s' not understood in level order!", command);
            }
        }

        // Go to next line
        while(buf[pos] && buf[pos]!='\n' && buf[pos]!='\r') ++pos;
        while(buf[pos]=='\n' || buf[pos]=='\r') ++pos;
    }
    if(buf != NULL){
        tracefree(buf);
        buf = NULL;
    }

    // Variables without defaults will be auto populated.
    if(olbarstatus.sizex==0) {olbarstatus = lbarstatus;}

    if(!plifeUsed[0]){ plife[2][0] = plife[0][0]; plife[2][1] = plife[2][1] + (plife[0][1] - 10); }
    if(!plifeUsed[1]){ plife[3][0] = plife[1][0]; plife[3][1] = plife[3][1] + (plife[1][1] - 10); }

    if(!elifeUsed[0]){ elife[2][0] = elife[0][0]; elife[2][1] = elife[2][1] + (elife[0][1] - 27); }
    if(!elifeUsed[1]){ elife[3][0] = elife[1][0]; elife[3][1] = elife[3][1] + (elife[1][1] - 27); }

    if(!piconUsed[0]){ picon[2][0] = picon[0][0]; picon[2][1] = picon[2][1] + (picon[0][1] - 2); }
    if(!piconUsed[1]){ picon[3][0] = picon[1][0]; picon[3][1] = picon[3][1] + (picon[1][1] - 2); }

    if(!piconwUsed[0]){ piconw[2][0] = piconw[0][0]; piconw[2][1] = piconw[2][1] + (piconw[0][1] - 2); }
    if(!piconwUsed[1]){ piconw[3][0] = piconw[1][0]; piconw[3][1] = piconw[3][1] + (piconw[1][1] - 2); }

    if(!eiconUsed[0]){ eicon[2][0] = eicon[0][0]; eicon[2][1] = eicon[2][1] + (eicon[0][1] - 19); }
    if(!eiconUsed[1]){ eicon[3][0] = eicon[1][0]; eicon[3][1] = eicon[3][1] + (eicon[1][1] - 19); }

    if(!pmpUsed[0]){ pmp[0][0] = plife[0][0]; pmp[0][1] = plife[0][1] + 8; }
    if(!pmpUsed[1]){ pmp[1][0] = plife[1][0]; pmp[1][1] = plife[1][1] + 8; }
    if(!pmpUsed[2]){ pmp[2][0] = pmp[0][0]; pmp[2][1] = pmp[2][1] + (pmp[0][1] - 18); }
    if(!pmpUsed[3]){ pmp[3][0] = pmp[1][0]; pmp[3][1] = pmp[1][1] + (pmp[1][1] - 18); }

    if(!plifeXused[0]){ plifeX[0][0] = plife[0][0] + lbarstatus.sizex + 4; plifeX[0][1] = picon[0][1] + 7; }
    if(!plifeXused[1]){ plifeX[1][0] = plife[1][0] + lbarstatus.sizex + 4; plifeX[1][1] = picon[1][1] + 7; }
    if(!plifeXused[2]){ plifeX[2][0] = plife[2][0] + lbarstatus.sizex + 4; plifeX[2][1] = picon[2][1] + 7; }
    if(!plifeXused[3]){ plifeX[3][0] = plife[3][0] + lbarstatus.sizex + 4; plifeX[3][1] = picon[3][1] + 7; }
	for(i=0; i<4; i++) if(plifeX[i][2] == -1) plifeX[i][2] = 0;

    if(!plifeNused[0]){ plifeN[0][0] = plife[0][0] + lbarstatus.sizex + 11; plifeN[0][1] = picon[0][1]; }
    if(!plifeNused[1]){ plifeN[1][0] = plife[1][0] + lbarstatus.sizex + 11; plifeN[1][1] = picon[1][1]; }
    if(!plifeNused[2]){ plifeN[2][0] = plifeN[0][0]; plifeN[2][1] = picon[2][1]; }
    if(!plifeNused[3]){ plifeN[3][0] = plifeN[1][0]; plifeN[3][1] = picon[3][1]; }
	for(i=0; i<4; i++) if(plifeN[i][2] == -1) plifeN[i][2] = 3;

    if(!pnameJused[0]){ pnameJ[0][2] = pnameJ[0][4] = pnameJ[0][0] = plife[0][0] + 1; pnameJ[0][5] = pnameJ[0][1] = picon[0][1]; pnameJ[0][3] = 10 + pnameJ[0][5]; }
    if(!pnameJused[1]){ pnameJ[1][2] = pnameJ[1][4] = pnameJ[1][0] = plife[1][0] + 1; pnameJ[1][5] = pnameJ[1][1] = picon[1][1]; pnameJ[1][3] = 10 + pnameJ[1][5]; }
    if(!pnameJused[2]){ pnameJ[2][2] = pnameJ[2][4] = pnameJ[2][0] = plife[2][0] + 1; pnameJ[2][5] = pnameJ[2][1] = picon[2][1]; pnameJ[2][3] = 10 + pnameJ[2][5]; }
    if(!pnameJused[3]){ pnameJ[3][2] = pnameJ[3][4] = pnameJ[3][0] = plife[3][0] + 1; pnameJ[3][5] = pnameJ[3][1] = picon[3][1]; pnameJ[3][3] = 10 + pnameJ[3][5]; }
	for(i=0; i<4; i++) if(pnameJ[i][6] == -1) pnameJ[i][6] = 0;

    if(!pscoreUsed[0]){ pscore[0][0] = plife[0][0] + 1; pscore[0][1] = picon[0][1]; }
    if(!pscoreUsed[1]){ pscore[1][0] = plife[1][0] + 1; pscore[1][1] = picon[1][1]; }
    if(!pscoreUsed[2]){ pscore[2][0] = plife[2][0] + 1; pscore[2][1] = picon[2][1]; }
    if(!pscoreUsed[3]){ pscore[3][0] = plife[3][0] + 1; pscore[3][1] = picon[3][1]; }
	for(i=0; i<4; i++) if(pscore[i][6] == -1) pscore[i][6] = 0;

    if(!enameused[0]){ ename[0][0] = elife[0][0] + 1; ename[0][1] = eicon[0][1]; }
    if(!enameused[1]){ ename[1][0] = elife[1][0] + 1; ename[1][1] = eicon[1][1]; }
    if(!enameused[2]){ ename[2][0] = ename[0][0]; ename[2][1] = eicon[2][1]; }
    if(!enameused[3]){ ename[3][0] = ename[1][0]; ename[3][1] = eicon[3][1]; }
	for(i=0; i<4; i++) if(ename[i][2] == -1) ename[i][2] = 0;

	for(i=0; i<2; i++)
	{
		if(loadingbg[i][0] < 0 && !loadingbg[i][3])
		{
			loadingbg[i][1]	= - 1 - videomodes.hShift;
			loadingbg[i][2]	= videomodes.vRes - 12;
			loadingbg[i][3]	= videomodes.hRes + 1;
			loadingbg[i][4]	= videomodes.hRes / 2 - (14 * (int)videomodes.hScale) - videomodes.hShift;
			loadingbg[i][5]	= videomodes.vRes / 2 - videomodes.vShift;
		}
	}

    branch_name[0] = 0; //clear up branch name, so we can use it in game

	for(i=0; i<4; i++) if(pshoot[i][2] == -1) pshoot[i][2] = 2;
	if(timeloc[5] == -1) timeloc[5] = 3;
    if(current_set<0) shutdown(1, "No levels were loaded!");
}


void free_level(s_level* lv)
{
    int i, j;
	s_spawn_script_list_node* templistnode;
	s_spawn_script_list_node* templistnode2;
	s_spawn_script_cache_node* tempnode;
	s_spawn_script_cache_node* tempnode2;
    if(!lv) return;
    //offload blending tables
    for(i=0; i<LEVEL_MAX_PALETTES; i++)
    {
        for(j=0; j<MAX_BLENDINGS; j++)
        {
            if(lv->blendings[i][j]) tracefree(lv->blendings[i][j]);
            lv->blendings[i][j] = NULL;
        }
    }
    //offload bglayers
    for(i=1; i<lv->numbglayers; i++)
    {
        if(lv->bglayers[i].handle)
        {
            tracefree(lv->bglayers[i].handle);
            lv->bglayers[i].handle = NULL;
        }
    }
        //offload fglayers
    for(i=0; i<lv->numfglayers; i++)
    {
        if(lv->fglayers[i].handle)
        {
            tracefree(lv->fglayers[i].handle);
            lv->fglayers[i].handle = NULL;
        }
    }

    //offload textobjs
    for(i=0; i<LEVEL_MAX_TEXTOBJS; i++)
    {
        if(lv->textobjs[i].text)
        {
            tracefree(lv->textobjs[i].text);
            lv->textobjs[i].text = NULL;
        }
    }

    for(i=0; i<LEVEL_MAX_FILESTREAMS; i++)
    {
        if(lv->filestreams[i].buf)
        {
            tracefree(lv->filestreams[i].buf);
            lv->filestreams[i].buf = NULL;
        }
    }

    //offload scripts
    Script_Clear(&(lv->update_script), 2);
    Script_Clear(&(lv->updated_script), 2);
    Script_Clear(&(lv->key_script), 2);
    Script_Clear(&(lv->level_script), 2);
    Script_Clear(&(lv->endlevel_script), 2);

    for(i=0; i<LEVEL_MAX_SPAWNS; i++)
    {
		if(lv->spawnpoints[i].spawn_script_list_head)
		{
			templistnode = lv->spawnpoints[i].spawn_script_list_head;
			lv->spawnpoints[i].spawn_script_list_head = NULL;
			while(templistnode)
			{
				templistnode2 = templistnode->next;
				templistnode->next = NULL;
				templistnode->spawn_script = NULL;
				tracefree(templistnode);
				templistnode = templistnode2;
		    }
		}
    }

	tempnode = spawn_script_cache_head;
	spawn_script_cache_head = NULL;
	while(tempnode)
	{
		tempnode2 = tempnode->next;
		Script_Clear(tempnode->cached_spawn_script, 2);
		tracefree(tempnode->cached_spawn_script);
		tempnode->cached_spawn_script = NULL;
		tracefree(tempnode->filename);
		tempnode->filename = NULL;
		tempnode->next = NULL;
		tracefree(tempnode);
		tempnode = tempnode2;
    }

	tracefree(lv);
    lv = NULL;
}


void unload_level(){
	int i, crlf = 0;
	char name[128] = {""};
	unload_background();
	unload_texture();
	freepanels();
	freescreen(&bgbuffer);

	if(level != NULL){
		strcpy(name, level->name);
		printf("Level Unloading: '%s'\n", name);
		getRamStatus(BYTES);
		tracefree(level->name);
		level->name = NULL;
		free_level(level);
		level = NULL;
		for(i=models_loaded; i; i--)
		{
			if(model_map[i-1].model->unload)
			{
				free_model(model_map[i-1].model, i-1);
				crlf = 1;
			}
		}
		if(crlf) printf("\n");
		printf("Level Unloaded:  '%s'\n", name);
		getRamStatus(BYTES);
	}

	levelpos = 0;
	advancex = 0;
	advancey = 0;
	advancetime = 0;
	quake = 0;
	quaketime = 0;
	level_waiting = 0;
	nojoin = 0;
	current_spawn = 0;
	groupmin = 100;
	groupmax = 100;
	scrollminz = 0;
	scrollmaxz = 0;
	blockade = 0;
	level_completed = 0;
	tospeedup = 0;    // Reset so it sets to normal speed for the next level
	reached[0] = reached[1] = reached[2] = reached[3] = 0;    // TYPE_ENDLEVEL values reset after level completed //4player
	showtimeover = 0;
	pause = 0;
	endgame = 0;
	go_time = 0;
	neon_time = 0;
	time = 0;
	cameratype = 0;
	light[0] = 128;
	light[1] = 64;
	gfx_y_offset = 0;    // Added so select screen graphics display correctly
}

void load_level(char *filename){
	char *buf;
	size_t size, len;
	ptrdiff_t pos, oldpos;
	char *command;
	char *value, *value2;
	char string[128] = {""};
	s_spawn_entry next;
	s_model *tempmodel, *cached_model;
	s_spawn_script_cache_node* tempnode;
	s_spawn_script_cache_node* tempnode2;
	s_spawn_script_list_node* templistnode;
	int i, j, crlf = 0;
	int usemap[MAX_BLENDINGS];
	char bgPath[128] = {""};
	int  bgPosi[6] = {0,0,0,0,0};
	char musicPath[128] = {""};
	u32 musicOffset = 0;

	unload_level();

	printf("Level Loading:   '%s'\n", filename);





	getRamStatus(BYTES);

	if(loadingbg[1][0] > 0)
	{
		if(custBkgrds != NULL)
		{
		strcpy(string, custBkgrds);
		strcat(string, "loading2");
		load_background(string, 0);
		}
		else load_cached_background("data/bgs/loading2", 0);
	}
	else if(loadingbg[1][0] < 0)
	{
		clearscreen(vscreen);
		spriteq_clear();
	}
	if(loadingbg[1][0])
	{
	    standard_palette(1);
	    lifebar_colors();
	    init_colourtable();
	}

	memset(&next, 0, sizeof(s_spawn_entry));

	level = tracemalloc("level",sizeof(s_level));
	if(level == NULL) shutdown(1, "load_level() #1 FATAL: Out of memory!\n");
	memset(level, 0, sizeof(s_level));
	len = strlen(filename);
	level->name = tracemalloc("level->name", len + 1);
	if(level->name == NULL) shutdown(1, "load_level() #1 FATAL: Out of memory!\n");
	strcpy(level->name, filename);
	level->name[len] = 0;

	if(buffer_pakfile(filename, &buf, &size)!=1) shutdown(1, "Unable to load level file '%s'", filename);

	level->settime = 100;    // Feb 25, 2005 - Default time limit set to 100
	level->nospecial = 0;    // Default set to specials can be used during bonus levels
	level->nohurt = 0;    // Default set to players can hurt each other during bonus levels
	level->nohit = 0;    // Default able to hit the other player
	level->spawn[0][2] = level->spawn[1][2] = level->spawn[2][2] = level->spawn[3][2] = 300;    // Set the default spawn a to 300
	level->setweap = 0;
	level->maxtossspeed = 100;
	level->maxfallspeed = -6;
	level->gravity = (float)-0.1;
	level->scrolldir = SCROLL_RIGHT;
	level->cameraxoffset = 0;
	level->camerazoffset = 0;
	level->bosses = 0;
	blendfx[BLEND_MULTIPLY] = 1;
	bgtravelled = 0;
	traveltime = 0;
	texttime = 0;
	nopause = 0;
	noscreenshot = 0;

	reset_playable_list(1);

    // Now interpret the contents of buf line by line
	pos = 0;
	while(pos<size){
		command = findarg(buf+pos, 0);
		if(command[0]){
			if(stricmp(command, "loadingbg")==0){
				load_background(findarg(buf+pos, 1), 0);
				for(i=0; i<6; i++) bgPosi[i] = atoi(findarg(buf+pos, i+2));
				standard_palette(1);
				lifebar_colors();
				init_colourtable();
			}
			else if(stricmp(command, "musicfade")==0){
				memset(&next,0,sizeof(s_spawn_entry));
				next.musicfade = atof(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "music")==0){
				value = findarg(buf+pos, 1);
				strncpy(string, value, 128);
				musicOffset = atol(findarg(buf+pos, 2));
				if(loadingmusic)
				{
					music(string, 1, musicOffset);
					musicPath[0] = 0;
				}
				else
				{
					oldpos = pos;
					// Go to next line
					while(buf[pos] && buf[pos]!='\n' && buf[pos]!='\r') ++pos;
					while(buf[pos]=='\n' || buf[pos]=='\r') ++pos;
					if(pos<size && (command = findarg(buf+pos, 0)) &&
					command[0] && stricmp(command, "at")==0)
					{
						if(next.musicfade == 0) memset(&next,0,sizeof(s_spawn_entry));
						strncpy(next.music, string, 128);
						next.musicoffset = musicOffset;
					}
					else
					{
						strncpy(musicPath, string, 128);
					}
					pos = oldpos;
				}
			}
			else if(stricmp(command, "allowselect")==0)
			{
				load_playable_list(buf+pos);
			}
			else if(stricmp(command, "load")==0){
				strncpy(string, findarg(buf+pos, 1), 128);
				load_cached_model(string, filename, atoi(findarg(buf+pos, 2)));
			}
			else if(stricmp(command, "background")==0){
				value = findarg(buf+pos, 1);
				strncpy(bgPath, value, strlen(value)+1);
				level->bglayers[0].type = bg_screen;
				level->bglayers[0].bgspeedratio = 1;

				level->bglayers[0].xratio = atof(findarg(buf+pos, 2)); // x ratio
				level->bglayers[0].zratio = atof(findarg(buf+pos, 3)); // z ratio
				level->bglayers[0].xoffset = atoi(findarg(buf+pos, 4)); // x start
				level->bglayers[0].zoffset = atoi(findarg(buf+pos, 5)); // z start
				level->bglayers[0].xspacing = atoi(findarg(buf+pos, 6)); // x spacing
				level->bglayers[0].zspacing = atoi(findarg(buf+pos, 7)); // z spacing
				level->bglayers[0].xrepeat = atoi(findarg(buf+pos, 8)); // x repeat
				level->bglayers[0].zrepeat = atoi(findarg(buf+pos, 9)); // z repeat
				// unused
				level->bglayers[0].transparency = atoi(findarg(buf+pos, 10)); // transparency
				level->bglayers[0].alpha = atoi(findarg(buf+pos, 11)); // alpha
				level->bglayers[0].watermode = atoi(findarg(buf+pos, 12)); // amplitude
				level->bglayers[0].amplitude = atoi(findarg(buf+pos, 13)); // amplitude
				level->bglayers[0].wavelength = atoi(findarg(buf+pos, 14)); // wavelength
				level->bglayers[0].wavespeed = atof(findarg(buf+pos, 15)); // waterspeed
				level->bglayers[0].enabled = 1; // enabled

				if(findarg(buf+pos, 2)[0]==0) level->bglayers[0].xratio = 0.5;
				if(findarg(buf+pos, 3)[0]==0) level->bglayers[0].zratio = 0.5;

				if(findarg(buf+pos, 8)[0]==0) level->bglayers[0].xrepeat = 5000;
				if(findarg(buf+pos, 9)[0]==0) level->bglayers[0].zrepeat = 5000;

				if(level->numbglayers==0) level->numbglayers = 1;
			}
			else if(stricmp(command, "bglayer") == 0){
				if(level->numbglayers >= LEVEL_MAX_BGLAYERS) shutdown(1, "Too many bg layers in level (max %i)!", LEVEL_MAX_BGLAYERS);
				if(level->numbglayers==0) level->numbglayers = 1; // reserve for background

				level->bglayers[level->numbglayers].xratio = atof(findarg(buf+pos, 2)); // x ratio
				level->bglayers[level->numbglayers].zratio = atof(findarg(buf+pos, 3)); // z ratio
				level->bglayers[level->numbglayers].xoffset = atoi(findarg(buf+pos, 4)); // x start
				level->bglayers[level->numbglayers].zoffset = atoi(findarg(buf+pos, 5)); // z start
				level->bglayers[level->numbglayers].xspacing = atoi(findarg(buf+pos, 6)); // x spacing
				level->bglayers[level->numbglayers].zspacing = atoi(findarg(buf+pos, 7)); // z spacing
				level->bglayers[level->numbglayers].xrepeat = atoi(findarg(buf+pos, 8)); // x repeat
				level->bglayers[level->numbglayers].zrepeat = atoi(findarg(buf+pos, 9)); // z repeat
				level->bglayers[level->numbglayers].transparency = atoi(findarg(buf+pos, 10)); // transparency
				level->bglayers[level->numbglayers].alpha = atoi(findarg(buf+pos, 11)); // alpha
				level->bglayers[level->numbglayers].watermode = atoi(findarg(buf+pos, 12)); // amplitude
				level->bglayers[level->numbglayers].amplitude = atoi(findarg(buf+pos, 13)); // amplitude
				level->bglayers[level->numbglayers].wavelength = atoi(findarg(buf+pos, 14)); // wavelength
				level->bglayers[level->numbglayers].wavespeed = atof(findarg(buf+pos, 15)); // waterspeed
				level->bglayers[level->numbglayers].bgspeedratio = atof(findarg(buf+pos, 16)); // moving
				level->bglayers[level->numbglayers].enabled = 1; // enabled

				if(findarg(buf+pos, 2)[0]==0) level->bglayers[level->numbglayers].xratio = 0.5;
				if(findarg(buf+pos, 3)[0]==0) level->bglayers[level->numbglayers].zratio = 0.5;

				if(findarg(buf+pos, 8)[0]==0) level->bglayers[level->numbglayers].xrepeat = 5000; // close enough to infinite, lol
				if(findarg(buf+pos, 9)[0]==0) level->bglayers[level->numbglayers].zrepeat = 5000;

				if(blendfx_is_set==0 && level->bglayers[level->numbglayers].alpha) blendfx[level->bglayers[level->numbglayers].alpha-1] = 1;

				load_bglayer(findarg(buf+pos, 1), level->numbglayers);
				level->numbglayers++;
			}
			else if(stricmp(command, "fglayer") == 0){
				if(level->numfglayers >= LEVEL_MAX_FGLAYERS) shutdown(1, "Too many bg layers in level (max %i)!", LEVEL_MAX_FGLAYERS);

				level->fglayers[level->numfglayers].z = atoi(findarg(buf+pos, 2)); // z
				level->fglayers[level->numfglayers].xratio = atof(findarg(buf+pos, 3)); // x ratio
				level->fglayers[level->numfglayers].zratio = atof(findarg(buf+pos, 4)); // z ratio
				level->fglayers[level->numfglayers].xoffset = atoi(findarg(buf+pos, 5)); // x start
				level->fglayers[level->numfglayers].zoffset = atoi(findarg(buf+pos, 6)); // z start
				level->fglayers[level->numfglayers].xspacing = atoi(findarg(buf+pos, 7)); // x spacing
				level->fglayers[level->numfglayers].zspacing = atoi(findarg(buf+pos, 8)); // z spacing
				level->fglayers[level->numfglayers].xrepeat = atoi(findarg(buf+pos, 9)); // x repeat

				level->fglayers[level->numfglayers].zrepeat = atoi(findarg(buf+pos, 10)); // z repeat
				level->fglayers[level->numfglayers].transparency = atoi(findarg(buf+pos, 11)); // transparency
				level->fglayers[level->numfglayers].alpha = atoi(findarg(buf+pos, 12)); // alpha
				level->fglayers[level->numfglayers].watermode = atoi(findarg(buf+pos, 13)); // amplitude
				level->fglayers[level->numfglayers].amplitude = atoi(findarg(buf+pos, 14)); // amplitude
				level->fglayers[level->numfglayers].wavelength = atoi(findarg(buf+pos, 15)); // wavelength
				level->fglayers[level->numfglayers].wavespeed = atof(findarg(buf+pos, 16)); // waterspeed
				level->fglayers[level->numfglayers].bgspeedratio = atof(findarg(buf+pos, 17)); // moving
				level->fglayers[level->numfglayers].enabled = 1;

				if(findarg(buf+pos, 2)[0]==0) level->fglayers[level->numfglayers].xratio = 1.5;
				if(findarg(buf+pos, 3)[0]==0) level->fglayers[level->numfglayers].zratio = 1.5;

				if(findarg(buf+pos, 8)[0]==0) level->fglayers[level->numfglayers].xrepeat = 5000; // close enough to infinite, lol
				if(findarg(buf+pos, 9)[0]==0) level->fglayers[level->numfglayers].zrepeat = 5000;

				if(blendfx_is_set==0 && level->fglayers[level->numfglayers].alpha) blendfx[level->fglayers[level->numfglayers].alpha-1] = 1;

				load_fglayer(findarg(buf+pos, 1), level->numfglayers);
				level->numfglayers++;
			}
			else if(stricmp(command, "water")==0){
				load_texture(findarg(buf+pos, 1));
				i = atoi(findarg(buf+pos, 2));
				if(i<1) i = 1;
				texture_set_wave((float)i);
			}
			else if(stricmp(command, "direction")==0){
				value = findarg(buf+pos, 1);
				if(stricmp(value, "up")==0) level->scrolldir = SCROLL_UP;
				else if(stricmp(value, "down")==0) level->scrolldir = SCROLL_DOWN;
				else if(stricmp(value, "left")==0) level->scrolldir = SCROLL_LEFT;
				else if(stricmp(value, "both")==0 || stricmp(value, "rightleft")==0) level->scrolldir = SCROLL_BOTH;
				else if(stricmp(value, "leftright")==0) level->scrolldir = SCROLL_LEFTRIGHT;
				else if(stricmp(value, "right")==0) level->scrolldir = SCROLL_RIGHT;
				else if(stricmp(value, "in")==0) level->scrolldir = SCROLL_INWARD;
				else if(stricmp(value, "out")==0) level->scrolldir = SCROLL_OUTWARD;
				else if(stricmp(value, "inout")==0) level->scrolldir = SCROLL_INOUT;
				else if(stricmp(value, "outin")==0) level->scrolldir = SCROLL_OUTIN;
			}
			else if(stricmp(command, "facing")==0){
				level->facing = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "rock")==0){
				level->rocking = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "bgspeed")==0){
				level->bgspeed = atof(findarg(buf+pos, 1));
				if(atoi(findarg(buf+pos, 2)))level->bgspeed*=-1;
			}
			else if(stricmp(command, "mirror")==0){
				level->mirror = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "bossmusic")==0){
				strncpy(level->bossmusic, findarg(buf+pos, 1), 255);
				level->bossmusic_offset = atol(findarg(buf+pos, 2));
			}
			else if(stricmp(command, "nopause")==0){
				nopause = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "noscreenshot")==0){
				noscreenshot = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "settime")==0){    // If settime is found, overwrite the default 100 for time limit
				if(atoi(findarg(buf+pos, 1)) > 100 || atoi(findarg(buf+pos, 1)) < 0) level->settime = 100;
				else level->settime = atoi(findarg(buf+pos, 1)); // Feb 25, 2005 - Time limit loaded from individual .txt file
			}
			else if(stricmp(command, "setweap")==0){    // Specify a weapon for each level
				level->setweap = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "notime")==0){    // Flag to if the time should be displayed 1 = no, else yes
				level->notime = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "noreset")==0){    // Flag to if the time should be reset when players respawn 1 = no, else yes
				level->noreset = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "noslow")==0){    // If set, level will not slow down when bosses are defeated
				level->noslow = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "type")==0){
				level->type = atoi(findarg(buf+pos, 1));    // Level type - 1 = bonus, else regular

				if(atoi(findarg(buf+pos, 2))) level->nospecial = atoi(findarg(buf+pos, 2));    // Can use specials during bonus levels (default 0 - yes)
				if(atoi(findarg(buf+pos, 3))) level->nohurt = atoi(findarg(buf+pos, 3));    // Can hurt other players during bonus levels (default 0 - yes)
			}
			else if(stricmp(command, "nohit")==0){
				level->nohit = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "gravity")==0){
				level->gravity = atof(findarg(buf+pos, 1));
				level->gravity /= 100;
			}
			else if(stricmp(command, "maxfallspeed")==0){
				level->maxfallspeed = atof(findarg(buf+pos, 1));
				level->maxfallspeed /= 10;
			}
			else if(stricmp(command, "maxtossspeed")==0){
				level->maxtossspeed = atof(findarg(buf+pos, 1));
				level->maxtossspeed /= 10;
			}
			else if(stricmp(command, "cameratype")==0){
				cameratype = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "cameraoffset")==0){
				level->cameraxoffset = atoi(findarg(buf+pos, 1));
				level->camerazoffset = atoi(findarg(buf+pos, 2));
			}
			else if(stricmp(command, "spawn1")==0){
				level->spawn[0][0] = atoi(findarg(buf+pos, 1));
				level->spawn[0][1] = atoi(findarg(buf+pos, 2));
				level->spawn[0][2] = atoi(findarg(buf+pos, 3));

				if(level->spawn[0][1] > 232 || level->spawn[0][1] < 0) level->spawn[0][1] = 232;

				if(level->spawn[0][2] < 0) level->spawn[0][2] = 300;
			}
			else if(stricmp(command, "spawn2")==0){
				level->spawn[1][0] = atoi(findarg(buf+pos, 1));
				level->spawn[1][1] = atoi(findarg(buf+pos, 2));
				level->spawn[1][2] = atoi(findarg(buf+pos, 3));

				if(level->spawn[1][1] > 232 || level->spawn[1][1] < 0) level->spawn[1][1] = 232;
				if(level->spawn[1][2] < 0) level->spawn[1][2] = 300;
			}
			else if(stricmp(command, "spawn3")==0){
				level->spawn[2][0] = atoi(findarg(buf+pos, 1));
				level->spawn[2][1] = atoi(findarg(buf+pos, 2));
				level->spawn[2][2] = atoi(findarg(buf+pos, 3));

				if(level->spawn[2][1] > 232 || level->spawn[2][1] < 0) level->spawn[2][1] = 232;
				if(level->spawn[2][2] < 0) level->spawn[2][2] = 300;
			}
			else if(stricmp(command, "spawn4")==0){
				level->spawn[3][0] = atoi(findarg(buf+pos, 1));
				level->spawn[3][1] = atoi(findarg(buf+pos, 2));
				level->spawn[3][2] = atoi(findarg(buf+pos, 3));

				if(level->spawn[3][1] > 232 || level->spawn[3][1] < 0) level->spawn[3][1] = 232;
				if(level->spawn[3][2] < 0) level->spawn[3][2] = 300;
			}
			else if(stricmp(command, "frontpanel")==0){
				value = findarg(buf+pos, 1);

				if(!loadfrontpanel(value)) shutdown(1, "Unable to load '%s'!", value);
			}
			else if(stricmp(command, "panel")==0){
				if(!loadpanel(findarg(buf+pos, 1), findarg(buf+pos, 2), findarg(buf+pos, 3)))  {
					//shutdown(1, "Panel load error in '%s'!", filename);
					printf("loadpanel :%s :%s :%s failed\n", findarg(buf+pos, 1), findarg(buf+pos, 2), findarg(buf+pos, 3));
					assert(0);
				}

			}
			else if(stricmp(command, "stagenumber")==0){
				current_stage = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "order")==0){
				// Append to order
				if(panels_loaded<1) shutdown(1, "You must load the panels before entering the level layout!");

				value = findarg(buf+pos, 1);
				i = 0;
				while(value[i] && level->numpanels < LEVEL_MAX_PANELS){
					j = value[i];

					if(j>='A' && j<='Z') j-='A';
					else if(j>='a' && j<='z') j-='a';
					else shutdown(1, "Illegal character in panel order: '%c' (%02Xh)", j, j);

					if(j >= panels_loaded) shutdown(1, "Illegal panel index: %i (only %i panels loaded)", j, panels_loaded);

					level->order[level->numpanels] = j;
					level->numpanels++;
					i++;
				}
			}
			else if(stricmp(command, "hole")==0){
				value = findarg(buf+pos, 1);    // ltb    1-18-05  adjustable hole sprites

				if(holesprite < 0)
				{
					if(testpackfile(value, packfile) >= 0) holesprite = loadsprite(value,0,0,pixelformat);// ltb 1-18-05  load new hole sprite
					else holesprite = loadsprite("data/sprites/hole",0,0,pixelformat);    // ltb 1-18-05  no new sprite load the default
				}

				if(level->numholes >= LEVEL_MAX_HOLES) shutdown(1, "Too many holes in level (max %i)!", LEVEL_MAX_HOLES);
				level->holes[level->numholes][0] = atof(findarg(buf+pos, 1));
				level->holes[level->numholes][1] = atof(findarg(buf+pos,2));
				level->holes[level->numholes][2] = atof(findarg(buf+pos,3));
				level->holes[level->numholes][3] = atof(findarg(buf+pos,4));
				level->holes[level->numholes][4] = atof(findarg(buf+pos,5));
				level->holes[level->numholes][5] = atof(findarg(buf+pos,6));
				level->holes[level->numholes][6] = atof(findarg(buf+pos,7));

				if(!level->holes[level->numholes][1]) level->holes[level->numholes][1] = 240;
				if(!level->holes[level->numholes][2]) level->holes[level->numholes][2] = 12;
				if(!level->holes[level->numholes][3]) level->holes[level->numholes][3] = 1;
				if(!level->holes[level->numholes][4]) level->holes[level->numholes][4] = 200;
				if(!level->holes[level->numholes][5]) level->holes[level->numholes][5] = 287;
				if(!level->holes[level->numholes][6]) level->holes[level->numholes][6] = 45;
				level->numholes++;
			}
				else if(stricmp(command, "wall")==0){
					if(level->numwalls >= LEVEL_MAX_WALLS) shutdown(1, "Too many walls in level (max %i)!", LEVEL_MAX_WALLS);
					level->walls[level->numwalls][0] = atof(findarg(buf+pos, 1));
					level->walls[level->numwalls][1] = atof(findarg(buf+pos,2));
					level->walls[level->numwalls][2] = atof(findarg(buf+pos,3));
					level->walls[level->numwalls][3] = atof(findarg(buf+pos,4));
					level->walls[level->numwalls][4] = atof(findarg(buf+pos,5));
					level->walls[level->numwalls][5] = atof(findarg(buf+pos,6));
					level->walls[level->numwalls][6] = atof(findarg(buf+pos,7));
					level->walls[level->numwalls][7] = atof(findarg(buf+pos,8));
					level->numwalls++;
				}
				else if(stricmp(command, "palette")==0){
					if(level->numpalettes >= LEVEL_MAX_PALETTES) shutdown(1, "Too many palettes in level (max %i)!", LEVEL_MAX_PALETTES);
					for(i=0; i<MAX_BLENDINGS; i++)
					usemap[i] = atoi(findarg(buf+pos, i+2));
					if(!load_palette(level->palettes[level->numpalettes], findarg(buf+pos,1)) ||
					!create_blending_tables(level->palettes[level->numpalettes], level->blendings[level->numpalettes], usemap))
					{
						shutdown(1, "Failed to create colour conversion tables for level! (Out of memory?)");
					}
					level->numpalettes++;
				}
				else if(stricmp(command, "updatescript")==0)
				{
					value = findarg(buf+pos, 1);
					if(!Script_IsInitialized(&(level->update_script)))
						Script_Init(&(level->update_script), "levelupdatescript", 1);
					else shutdown(1, "Multiple level update script: '%s'!", value);
					if(load_script(&(level->update_script), value))
						Script_Compile(&(level->update_script));
					else shutdown(1, "Failed loading level update script: '%s'!", value);
				}
				else if(stricmp(command, "updatedscript")==0)
				{
					value = findarg(buf+pos, 1);
					if(!Script_IsInitialized(&(level->updated_script)))
						Script_Init(&(level->updated_script), "levelupdatedscript", 1);
					else shutdown(1, "Multiple level updated script: '%s'!", value);
					if(load_script(&(level->updated_script), value))
						Script_Compile(&(level->updated_script));
					else shutdown(1, "Failed loading level updated script: '%s'!", value);
				}
            else if(stricmp(command, "keyscript")==0)
            {
                value = findarg(buf+pos, 1);
                if(!Script_IsInitialized(&(level->key_script)))
                    Script_Init(&(level->key_script), "levelkeyscript", 1);
                else shutdown(1, "Multiple level key script: '%s'!", value);
                if(load_script(&(level->key_script), value))
                    Script_Compile(&(level->key_script));
                else shutdown(1, "Failed loading level key script: '%s'!", value);
            }
            else if(stricmp(command, "levelscript")==0)
            {
                value = findarg(buf+pos, 1);
                if(!Script_IsInitialized(&(level->level_script)))
                    Script_Init(&(level->level_script), command, 1);
                else shutdown(1, "Multiple level script: '%s'!", value);
                if(load_script(&(level->level_script), value))
                    Script_Compile(&(level->level_script));
                else shutdown(1, "Failed loading level script: '%s'!", value);
            }
            else if(stricmp(command, "endlevelscript")==0)
            {
                value = findarg(buf+pos, 1);
                if(!Script_IsInitialized(&(level->endlevel_script)))
                    Script_Init(&(level->endlevel_script), command, 1);
                else shutdown(1, "Multiple end-level script: '%s'!", value);
                if(load_script(&(level->endlevel_script), value))
                    Script_Compile(&(level->endlevel_script));
                else shutdown(1, "Failed loading end-level script: '%s'!", value);
            }
            else if(stricmp(command, "blocked")==0){
                level->exit_blocked = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "endhole")==0){
                level->exit_hole = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "wait")==0){
                // Clear spawn thing, set wait state instead
                memset(&next,0,sizeof(s_spawn_entry));
                next.wait = 1;
            }
            else if(stricmp(command, "nojoin")==0){
                // Clear spawn thing, set nojoin state instead
                memset(&next,0,sizeof(s_spawn_entry));
                next.nojoin = 1;
            }
            else if(stricmp(command, "canjoin")==0){
                // Clear spawn thing, set nojoin state instead
                memset(&next,0,sizeof(s_spawn_entry));
                next.nojoin = -1;
            }
            else if(stricmp(command, "shadowcolor")==0){
                memset(&next,0,sizeof(s_spawn_entry));
				next.shadowcolor = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "shadowalpha")==0){
                memset(&next,0,sizeof(s_spawn_entry));
				next.shadowalpha = atoi(findarg(buf+pos, 1));
				if(blendfx_is_set==0 && next.shadowalpha>0) blendfx[next.shadowalpha-1] = 1;
            }
            else if(stricmp(command, "light")==0){
                memset(&next,0,sizeof(s_spawn_entry));
			    next.light[0] = atoi(findarg(buf+pos, 1));
				next.light[1] = atoi(findarg(buf+pos, 2));
				if(next.light[1] == 0) next.light[1] = 64;
            }
            else if(stricmp(command, "scrollz")==0 || stricmp(command, "scrollx")==0){
                // now z scroll can be limited by this
                // if the level is vertical, use scrollx, only different in name ..., but makes more sense
                memset(&next,0,sizeof(s_spawn_entry));
                next.scrollminz = atoi(findarg(buf+pos, 1));
                next.scrollmaxz = atoi(findarg(buf+pos, 2));
                if(next.scrollminz <= 0) next.scrollminz = 4;
                if(next.scrollmaxz <= 0) next.scrollmaxz = 4;
            }
            else if(stricmp(command, "blockade")==0){
                // now x scroll can be limited by this
                memset(&next,0,sizeof(s_spawn_entry));
                next.blockade = atoi(findarg(buf+pos, 1));
                if(next.blockade==0) next.blockade = -1;
            }
            else if(stricmp(command, "setpalette")==0){
                // change system palette
                memset(&next,0,sizeof(s_spawn_entry));
                next.palette = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "group")==0){
                // Clear spawn thing, set group instead
                memset(&next,0,sizeof(s_spawn_entry));
                next.groupmin = atoi(findarg(buf+pos, 1));
                next.groupmax = atoi(findarg(buf+pos, 2));
                if(next.groupmax < 1) next.groupmax = 1;
                if(next.groupmin < 1) next.groupmin = 100;
            }
            else if(stricmp(command, "spawn")==0){
                // Back to defaults
                next.spawnplayer_count = 0;
                memset(&next,0,sizeof(s_spawn_entry));
                next.index = next.itemindex = next.weaponindex = -1;
                // Name of entry to be spawned
                // Load model (if not loaded already)
				strncpy(string, findarg(buf+pos, 1), 128);
				cached_model = find_model(string);
				if(cached_model) tempmodel = cached_model;
				else tempmodel = load_cached_model(string, filename, 0);
                if(tempmodel)
                {
                    next.name = tempmodel->name;
                    next.index = get_cached_model_index(next.name);
					crlf = 1;
                }
            }
            else if(stricmp(command, "2pspawn")==0){
                // Entity only for 2p game
                next.spawnplayer_count = 1;
            }
            else if(stricmp(command, "3pspawn")==0){
                // Entity only for 3p game
                next.spawnplayer_count = 2;
            }
            else if(stricmp(command, "4pspawn")==0){
                // Entity only for 4p game
                next.spawnplayer_count = 3;
            }
            else if(stricmp(command, "boss")==0){
                next.boss = atoi(findarg(buf+pos, 1));
				level->bosses += next.boss ? 1 : 0;
            }
            else if(stricmp(command, "flip")==0){
                next.flip = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "health")==0){
                next.health[0] = next.health[1] = next.health[2] = next.health[3] = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "2phealth")==0){    // Health the spawned entity will have if 2 people are playing
                next.health[1] = next.health[2] = next.health[3] = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "3phealth")==0){    // Health the spawned entity will have if 2 people are playing
                next.health[2] = next.health[3] = atoi(findarg(buf+pos, 1));  //4player
            }
            else if(stricmp(command, "4phealth")==0){    // Health the spawned entity will have if 2 people are playing
                next.health[3] = atoi(findarg(buf+pos, 1));  //4player
            }
            else if(stricmp(command, "mp")==0){        // mp values to put max mp for player by tails
                next.mp = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "score")==0){    // So score can be overriden in the levels .txt file
                next.score = atoi(findarg(buf+pos, 1));
                if(next.score < 0) next.score = 0;    // So negative values cannot be added
                next.multiple = atoi(findarg(buf+pos, 2));
                if(next.multiple < 0) next.multiple = 0;    // So negative values cannot be added
            }
            else if(stricmp(command, "nolife")==0){    // Flag to determine if entity life is shown when hit
                next.nolife = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "alias")==0){
                // Alias (name displayed) of entry to be spawned
                strncpy(next.alias, findarg(buf+pos, 1), MAX_NAME_LEN);
            }
            else if(stricmp(command, "map")==0){
                // Colourmap for new entry
                next.colourmap = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "alpha")==0){
                // Item to be contained by new entry
                next.alpha = atoi(findarg(buf+pos, 1));
                if(blendfx_is_set==0 && next.alpha) blendfx[next.alpha-1] = 1;
            }
            else if(stricmp(command, "dying")==0){    // Used to store which remake corresponds with the dying flash
                next.dying = atoi(findarg(buf+pos, 1));
                next.per1 = atoi(findarg(buf+pos, 2));
                next.per2 = atoi(findarg(buf+pos, 3));
            }
            else if(stricmp(command, "item")==0){
                // Item to be contained by new entry
                next.itemplayer_count = 0;
                // Load model (if not loaded already)
				strncpy(string, findarg(buf+pos, 1), 128);
				cached_model = find_model(string);
				if(cached_model) tempmodel = cached_model;
				else tempmodel = load_cached_model(string, filename, 0);
                if(tempmodel)
                {
                    next.item = tempmodel->name;
                    next.itemindex = get_cached_model_index(next.item);
                }
            }
            else if(stricmp(command, "2pitem")==0){
                // Item only for 2p game
                next.itemplayer_count = 1;
                // Load model (if not loaded already)  // 2007-2-12, inserted by UTunnels
				strncpy(string, findarg(buf+pos, 1), 128);
				cached_model = find_model(string);
				if(cached_model) tempmodel = cached_model;
				else tempmodel = load_cached_model(string, filename, 0);
                if(tempmodel)
                {
                    next.item = tempmodel->name;
                    next.itemindex = get_cached_model_index(next.item);
                }
            }
            else if(stricmp(command, "3pitem")==0){
                // Item only for 2p game
                next.itemplayer_count = 2;
                // Load model (if not loaded already)  // 2007-2-12, inserted by UTunnels
				strncpy(string, findarg(buf+pos, 1), 128);
				cached_model = find_model(string);
				if(cached_model) tempmodel = cached_model;
				else tempmodel = load_cached_model(string, filename, 0);
                if(tempmodel)
                {
                    next.item = tempmodel->name;
                    next.itemindex = get_cached_model_index(next.item);
                }
            }
            else if(stricmp(command, "4pitem")==0){
                // Item only for 2p game
                next.itemplayer_count = 3;
                // Load model (if not loaded already) // 2007-2-12, inserted by UTunnels
				strncpy(string, findarg(buf+pos, 1), 128);
				cached_model = find_model(string);
				if(cached_model) tempmodel = cached_model;
				else tempmodel = load_cached_model(string, filename, 0);
                if(tempmodel)
                {
                    next.item = tempmodel->name;
                    next.itemindex = get_cached_model_index(next.item);
                }
            }
            else if(stricmp(command, "itemmap")==0){
                next.itemmap = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "itemhealth")==0){
                next.itemhealth = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "itemalias")==0){
                strncpy(next.itemalias, findarg(buf+pos, 1), MAX_NAME_LEN);
            }
            else if(stricmp(command, "weapon")==0){  //spawn with a weapon 2007-2-12 by UTunnels
                // Load model (if not loaded already)
				strncpy(string, findarg(buf+pos, 1), 128);
				cached_model = find_model(string);
				if(cached_model) tempmodel = cached_model;
				else tempmodel = load_cached_model(string, filename, 0);
                if(tempmodel)
                {
                    next.weapon = tempmodel->name;
                    next.weaponindex = get_cached_model_index(next.weapon);
                }
            }
            else if(stricmp(command, "aggression")==0){ // Aggression can be set per spawn.
                next.aggression = next.aggression + atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "credit")==0){
                next.credit = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "itemtrans")==0 || stricmp(command, "itemalpha")==0){
                next.itemtrans = atoi(findarg(buf+pos, 1));
            }
            else if(stricmp(command, "coords")==0){
                next.x = atof(findarg(buf+pos, 1));
                next.z = atof(findarg(buf+pos, 2));
                next.a = atof(findarg(buf+pos, 3));
            }
            else if(stricmp(command, "spawnscript")==0) // spawn entry script
            {
                value = findarg(buf+pos, 1);
				tempnode = spawn_script_cache_head;
				if(!next.spawn_script_list_head) next.spawn_script_list_head = NULL;
				templistnode = next.spawn_script_list_head;
				if(templistnode)
				{
					while(templistnode->next)
					{
						templistnode = templistnode->next;
					}
					templistnode->next = tracemalloc("spawn_script_list_node", sizeof(s_spawn_script_list_node));
					templistnode = templistnode->next;
				}
				else
				{
					next.spawn_script_list_head = tracemalloc("spawn_script_list_node", sizeof(s_spawn_script_list_node));
					templistnode = next.spawn_script_list_head;
				}
				templistnode->spawn_script = NULL;
				templistnode->next = NULL;
				if(tempnode)
				{
					while(1)
					{
						if(stricmp(value, tempnode->filename)==0)
						{
							templistnode->spawn_script = tempnode->cached_spawn_script;
							break;
						}
						else
						{
							if(tempnode->next)
								tempnode = tempnode->next;
							else
								break;
						}
					}
				}
				if(!templistnode->spawn_script)
				{
					templistnode->spawn_script = alloc_script();
					if(!Script_IsInitialized(templistnode->spawn_script))
					{
						Script_Init(templistnode->spawn_script, command, 0);
					}
					else shutdown(1, "Multiple spawn entry script: '%s'!", value);
					if(load_script(templistnode->spawn_script, value))
					{
	                    Script_Compile(templistnode->spawn_script);
						if(tempnode)
						{
                            value2 = findarg(buf+pos, 1);
                            len = strlen(value2);
							tempnode2 = tracemalloc("spawn_script_node", sizeof(s_spawn_script_cache_node));
							tempnode2->cached_spawn_script = templistnode->spawn_script;
							tempnode2->filename = tracemalloc("spawn_script_node_filename", len + 1);
							strcpy(tempnode2->filename, value2);
                            tempnode2->filename[len] = 0;
							tempnode2->next = NULL;
							tempnode->next = tempnode2;
						}
						else
						{
                            value2 = findarg(buf+pos, 1);
                            len = strlen(value2);
							spawn_script_cache_head = tracemalloc("spawn_script_node", sizeof(s_spawn_script_cache_node));
							spawn_script_cache_head->cached_spawn_script = templistnode->spawn_script;
							spawn_script_cache_head->filename = tracemalloc("spawn_script_node_filename", len + 1);
							spawn_script_cache_head->next = NULL;
							strcpy(spawn_script_cache_head->filename, value2);
                            spawn_script_cache_head->filename[len] = 0;
						}
					}
					else shutdown(1, "Failed loading spawn entry script: '%s'!", value);
				}
			}
			else if(stricmp(command, "at")==0){
                // Place entry on queue
                next.at = atoi(findarg(buf+pos, 1));

                if(level->numspawns >= LEVEL_MAX_SPAWNS) shutdown(1, "Level error: too many entries (max. %i)", LEVEL_MAX_SPAWNS);
                memcpy(&level->spawnpoints[level->numspawns], &next, sizeof(s_spawn_entry));
                level->numspawns++;

                // And clear...
                memset(&next,0,sizeof(s_spawn_entry));
            }
            else shutdown(1, "Command '%s' not understood!", command);
        }

		// Go to next line
        while(buf[pos] && buf[pos]!='\n' && buf[pos]!='\r') ++pos;
        while(buf[pos]=='\n' || buf[pos]=='\r') ++pos;
		if(bgPosi[2])
			update_loading(bgPosi[0]+videomodes.hShift, bgPosi[1]+videomodes.vShift, bgPosi[2], bgPosi[3]+videomodes.hShift, bgPosi[4]+videomodes.vShift, pos, size, bgPosi[5]);
		else if(loadingbg[1][0])
			update_loading(loadingbg[1][1]+videomodes.hShift, loadingbg[1][2]+videomodes.vShift, loadingbg[1][3], loadingbg[1][4]+videomodes.hShift, loadingbg[1][5]+videomodes.vShift, pos, size, loadingbg[1][6]);
    }
    if(buf != NULL){
        tracefree(buf);
        buf = NULL;
    }

    if(level->numpanels < 1) shutdown(1, "Level error: level has no panels");

	if(bgPath[0])
	{
		clearscreen(vscreen);
		spriteq_clear();
		load_background(bgPath, 1);
        level->bglayers[0].screen=background;
        level->bglayers[0].width=background->width;
        //if(!level->bglayers[0].xoffset && level->bglayers[0].xrepeat<5000)level->bglayers[0].xoffset=-background->width;
        level->bglayers[0].height=background->height;
	}
	else
	{
		if(level->numbglayers>1) level->bglayers[0] = level->bglayers[--level->numbglayers];
        else                     level->numbglayers = 0;

        if(background) unload_background();
    }

    if(pixelformat==PIXEL_x8)
    {
        if(level->numbglayers>0) bgbuffer = allocscreen(videomodes.hRes, videomodes.vRes, screenformat);
    }
    bgbuffer_updated = 0;
    if(musicPath[0]) music(musicPath, 1, musicOffset);

    timeleft = level->settime * COUNTER_SPEED;    // Feb 24, 2005 - This line moved here to set custom time
    level->width = level->numpanels * panel_width;

    if(level->scrolldir&SCROLL_LEFT) advancex = (float)(level->width-videomodes.hRes);
    else if(level->scrolldir&SCROLL_INWARD) advancey = (float)(panel_height-videomodes.vRes);

	if(crlf) printf("\n");
    printf("Level Loaded:    '%s'\n", level->name);
	totalram = getSystemRam(BYTES); freeram = getFreeRam(BYTES); usedram = getUsedRam(BYTES);
	printf("Total Ram: %"PRIu64" Bytes\n Free Ram: %"PRIu64" Bytes\n Used Ram: %"PRIu64" Bytes\n\n", totalram, freeram, usedram);
}





/////////////////////////////////////////////////////////////////////////////
//  Status                                                                  //
/////////////////////////////////////////////////////////////////////////////
void bar(int x, int y, int value, int maxvalue, s_barstatus* pstatus)
{
    int max = 100, len, alphabg=0, bgindex, colourindex;
    int forex, forey, forew,foreh, bkw, bkh;

    x += pstatus->offsetx;
    y += pstatus->offsety;

    if(pstatus->orientation==horizontalbar)    max = pstatus->sizex;
    else if(pstatus->orientation==verticalbar) max = pstatus->sizey;
    else return;

    if (value > maxvalue) value = maxvalue;

    if(pstatus->type==valuebar)
    {
        if(max>maxvalue) max = maxvalue;
        if(colorbars)
        {
            if(value<=max/4) {bgindex = 0; colourindex = 1;}
            else if(value<=max/2) {bgindex=0; colourindex = 2;}
            else if(value<=max) {bgindex=0; colourindex = 3;}
            else {colourindex = value/(max+1) + 3; bgindex = colourindex-1;}
            if(colourindex>10) colourindex = bgindex = 10;
        }
        else
        {
            colourindex = 2;
            bgindex = value>max? 5:1;
        }

        len = value%max;
        if(!len && value) len = max;
        alphabg = value>max?0:(BLEND_MULTIPLY+1);
    }
    else if(pstatus->type==percentagebar)
    {
        colourindex = colorbars?(value*5/maxvalue+1):2;
        bgindex = colorbars?8:1;
        len = value * max / maxvalue;
        if(!len && value) len = 1;
        alphabg = BLEND_MULTIPLY+1;
    }
    else return;

    if(pstatus->orientation==horizontalbar)
    {
        forex = pstatus->direction?(x + max - len):x;
        forey = y;
        forew = len; bkw = max;
        bkh = foreh = pstatus->sizey;
    }
    else if(pstatus->orientation==verticalbar)
    {
        forex = x;
        forey = pstatus->direction?y:(y + max - len);
        bkw = forew = pstatus->sizex;
        foreh = len; bkh = max;
    }
    else return;

    if(!pstatus->colourtable) pstatus->colourtable = &hpcolourtable;

    spriteq_add_box(x+1, y+1, bkw, bkh, HUD_Z+1+pstatus->backlayer, (*pstatus->colourtable)[bgindex], alphabg);
    spriteq_add_box(forex+1, forey+1, forew, foreh, HUD_Z+2+pstatus->barlayer, (*pstatus->colourtable)[colourindex], 0);

    if(pstatus->noborder==0)
    {
    	spriteq_add_line(x, y, x+bkw+1, y, HUD_Z+3+pstatus->borderlayer, color_white, 0); //Top border.
		spriteq_add_line(x, y+bkh+1, x+bkw+1, y+bkh+1, HUD_Z+3+pstatus->borderlayer, color_white, 0); //Bottom border.
		spriteq_add_line(x, y+1, x, y+bkh, HUD_Z+3+pstatus->borderlayer, color_white, 0); //Left border.
		spriteq_add_line(x+bkw+1, y+1, x+bkw+1, y+bkh, HUD_Z+3+pstatus->borderlayer, color_white, 0); //Right border.
		spriteq_add_line(x, y+bkh+2, x+bkw+1, y+bkh+2, HUD_Z+pstatus->borderlayer, color_black, 0); //Bottom shadow.
		spriteq_add_line(x+bkw+2, y+1, x+bkw+2, y+bkh+2, HUD_Z+pstatus->borderlayer, color_black, 0); //Right shadow.
    }
}


void pausemenu()
{
	int pauselector = 0;
    int quit = 0;

    pause = 2;
    bothnewkeys = 0;
    while(!quit)
    {
		_menutextm(3, -2, 0, "Pause");
        _menutextm((pauselector==0), -1, 0, "Continue");
        _menutextm((pauselector==1), 0, 0, "End Game");

        update(1,0);

        if(bothnewkeys & (FLAG_MOVEUP|FLAG_MOVEDOWN)){
            pauselector ^= 1;
            sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        if(bothnewkeys & FLAG_START){
            if(pauselector){
                player[0].lives = player[1].lives = player[2].lives = player[3].lives = 0; //4player
                endgame = 1;
            }
            quit = 1;
            sound_pause_music(0);
            sound_play_sample(SAMPLE_BEEP2, 0, savedata.effectvol,savedata.effectvol, 100);
            pauselector = 0;
        }
        if(bothnewkeys & FLAG_ESC){
            quit = 1;
            sound_pause_music(0);
            sound_play_sample(SAMPLE_BEEP2, 0, savedata.effectvol,savedata.effectvol, 100);
            pauselector = 0;
        }
        if(bothnewkeys & FLAG_SCREENSHOT){
            pause = 1;
            sound_pause_music(1);
            sound_play_sample(SAMPLE_BEEP2, 0, savedata.effectvol,savedata.effectvol, 100);
            options();
        }
    }
    pause = 0;
    bothnewkeys = 0;
    spriteq_unlock();
}

unsigned getFPS(void)
{
	static unsigned lasttick=0,framerate = 0;
    unsigned curtick = timer_gettick();
    if(lasttick > curtick) lasttick = curtick;
    framerate = (framerate + (curtick-lasttick))/2;
    lasttick = curtick;
	if(!framerate) return 0;
#ifdef PSP
	return ((10000000/framerate)+9)/10;
#else
    return ((10000000/framerate)+9)/10000;
#endif
}

void predrawstatus(){

    int dt;
    int tperror=0;
    int icon = 0;
    int i,x;
    s_model * model = NULL;
    s_drawmethod drawmethod = plainmethod;

	if(bgicon >= 0) spriteq_add_sprite(videomodes.hShift+bgicon_offsets[0],savedata.windowpos+bgicon_offsets[1], bgicon_offsets[2], bgicon, NULL, 0);
	if(olicon >= 0) spriteq_add_sprite(videomodes.hShift+olicon_offsets[0],savedata.windowpos+olicon_offsets[1], olicon_offsets[2], olicon, NULL, 0);

    for(i=0; i<maxplayers[current_set]; i++)
    {
        if(player[i].ent)
        {
            if(!pscore[i][2] && !pscore[i][3] && !pscore[i][4] && !pscore[i][5]) font_printf(videomodes.shiftpos[i]+pscore[i][0], savedata.windowpos+pscore[i][1], pscore[i][6], 0, (scoreformat ? "%s - %09lu" : "%s - %lu"), player[i].ent->name, player[i].score);
            else
            {
                font_printf(videomodes.shiftpos[i]+pscore[i][0], savedata.windowpos+pscore[i][1], pscore[i][6], 0, "%s", player[i].ent->name);
                font_printf(videomodes.shiftpos[i]+pscore[i][2], savedata.windowpos+pscore[i][3], pscore[i][6], 0, "-");
                font_printf(videomodes.shiftpos[i]+pscore[i][4], savedata.windowpos+pscore[i][5], pscore[i][6], 0, (scoreformat ? "%09lu" : "%lu"), player[i].score);
            }

            if(player[i].ent->health <= 0) icon = player[i].ent->modeldata.icondie;
            else if(player[i].ent->inpain) icon = player[i].ent->modeldata.iconpain;
            else if(player[i].ent->getting) icon = player[i].ent->modeldata.iconget;
            else icon = player[i].ent->modeldata.icon;

            if(icon>=0)
            {
                drawmethod.table = player[i].ent->colourmap;
                spriteq_add_sprite(videomodes.shiftpos[i]+picon[i][0],savedata.windowpos+picon[i][1],10000, icon, &drawmethod, 0);
            }

            if(player[i].ent->weapent)
            {
                if(player[i].ent->weapent->modeldata.iconw >= 0)
                {
                    drawmethod.table = player[i].ent->weapent->colourmap;
                    spriteq_add_sprite(videomodes.shiftpos[i]+piconw[i][0],savedata.windowpos+piconw[i][1],10000, player[i].ent->weapent->modeldata.iconw, &drawmethod, 0);
                }

                if(player[i].ent->weapent->modeldata.typeshot && player[i].ent->weapent->modeldata.shootnum)
                    font_printf(videomodes.shiftpos[i]+pshoot[i][0], savedata.windowpos+pshoot[i][1], pshoot[i][2], 0, "%u", player[i].ent->weapent->modeldata.shootnum);
            }

            if(player[i].ent->modeldata.mp)
            {
                if(player[i].ent->modeldata.iconmp[0] > 0 && (player[i].ent->oldmp >= (player[i].ent->modeldata.mp * .66))){
                    drawmethod.table = player[i].ent->colourmap;
                    spriteq_add_sprite(videomodes.shiftpos[i]+mpicon[i][0],savedata.windowpos+mpicon[i][1],10000, player[i].ent->modeldata.iconmp[0], &drawmethod, 0);
                }
                else if(player[i].ent->modeldata.iconmp[1] > 0 && (player[i].ent->oldmp >= (player[i].ent->modeldata.mp * .33) && player[i].ent->oldmp < (player[i].ent->modeldata.mp * .66))){
                    drawmethod.table = player[i].ent->colourmap;
                    spriteq_add_sprite(videomodes.shiftpos[i]+mpicon[i][0],savedata.windowpos+mpicon[i][1],10000, player[i].ent->modeldata.iconmp[1], &drawmethod, 0);
                }
                else if(player[i].ent->modeldata.iconmp[2] > 0 && (player[i].ent->oldmp >= 0 && player[i].ent->oldmp < (player[i].ent->modeldata.mp * .33))){
                    drawmethod.table = player[i].ent->colourmap;
                    spriteq_add_sprite(videomodes.shiftpos[i]+mpicon[i][0],savedata.windowpos+mpicon[i][1],10000, player[i].ent->modeldata.iconmp[2], &drawmethod, 0);
                }
                else if(player[i].ent->modeldata.iconmp[0] > 0 && player[i].ent->modeldata.iconmp[1] == -1 && player[i].ent->modeldata.iconmp[2] == -1){
                    drawmethod.table = player[i].ent->colourmap;
                    spriteq_add_sprite(videomodes.shiftpos[i]+mpicon[i][0],savedata.windowpos+mpicon[i][1],10000, player[i].ent->modeldata.iconmp[0], &drawmethod, 0);
                }
            }

            font_printf(videomodes.shiftpos[i]+plifeX[i][0],savedata.windowpos+plifeX[i][1], plifeX[i][2], 0, "x");
            font_printf(videomodes.shiftpos[i]+plifeN[i][0],savedata.windowpos+plifeN[i][1], plifeN[i][2], 0, "%i", player[i].lives);

            if(rush[0] && player[i].ent->rush[0] > 1 && time <= player[i].ent->rushtime)
            {
                font_printf(videomodes.shiftpos[i]+prush[i][0],prush[i][1], rush[2], 0, "%s", rush_names[0]);
                font_printf(videomodes.shiftpos[i]+prush[i][2],prush[i][3], rush[3], 0, "%i", player[i].ent->rush[0]);

                if(rush[0] != 2){
                    font_printf(videomodes.shiftpos[i]+prush[i][4],prush[i][5], rush[4], 0, "%s", rush_names[1]);
                    font_printf(videomodes.shiftpos[i]+prush[i][6],prush[i][7], rush[5], 0, "%i", player[i].ent->rush[1]);
                }
            }

            if(rush[0] == 2)
            {
                font_printf(videomodes.shiftpos[i]+prush[i][4],prush[i][5], rush[4], 0, "%s", rush_names[1]);
                font_printf(videomodes.shiftpos[i]+prush[i][6],prush[i][7], rush[5], 0, "%i", player[i].ent->rush[1]);
            }

            if(player[i].ent->opponent && !player[i].ent->opponent->modeldata.nolife)
            {    // Displays life unless overridden by flag
                font_printf(videomodes.shiftpos[i]+ename[i][0], savedata.windowpos+ename[i][1], ename[i][2], 0, player[i].ent->opponent->name);
                if(player[i].ent->opponent->health <= 0) icon = player[i].ent->opponent->modeldata.icondie;
                else if(player[i].ent->opponent->inpain) icon = player[i].ent->opponent->modeldata.iconpain;
                else if(player[i].ent->opponent->getting) icon = player[i].ent->opponent->modeldata.iconget;
                else icon = player[i].ent->opponent->modeldata.icon;

                if(icon>=0)
                {
                    drawmethod.table = player[i].ent->opponent->colourmap;
                    spriteq_add_sprite(videomodes.shiftpos[i]+eicon[i][0],savedata.windowpos+eicon[i][1],10000, icon, &drawmethod, 0);    // Feb 26, 2005 - Changed to opponent->map so icons don't pallete swap with die animation
                }
            }
        }
        else if(player[i].joining && player[i].name[0])
        {
            model = find_model(player[i].name);
            font_printf(videomodes.shiftpos[i]+pnameJ[i][0], savedata.windowpos+pnameJ[i][1], pnameJ[i][6], 0, player[i].name);
            if(nojoin) font_printf(videomodes.shiftpos[i]+pnameJ[i][2], savedata.windowpos+pnameJ[i][3], pnameJ[i][6], 0, "Please Wait");
            else font_printf(videomodes.shiftpos[i]+pnameJ[i][2], savedata.windowpos+pnameJ[i][3], pnameJ[i][6], 0, "Select Hero");
            icon = model->icon;

            if(icon>=0 && !player[i].colourmap){
                spriteq_add_sprite(videomodes.shiftpos[i]+picon[i][0],picon[i][1],10000, icon, NULL, 0);
            }
            else if(icon>=0)
            {
                drawmethod.table = model->colourmap[player[i].colourmap - 1];
                spriteq_add_sprite(videomodes.shiftpos[i]+picon[i][0],picon[i][1], 10000, icon, &drawmethod, 0);
            }

            if((player[i].playkeys & FLAG_ANYBUTTON || (skipselect&&(*skipselect)[current_set][i])) && !freezeall && !nojoin)    // Can't join while animations are frozen
            {
                // reports error if players try to use the same character and sameplay mode is off
                if(sameplayer){
                    for(x=0; x<maxplayers[current_set]; x++){
                        if((strncmp(player[i].name,player[x].name,strlen(player[i].name)) == 0) && (i != x)){
                            tperror = i+1;
                            break;
                        }
                    }
                }

                if (!tperror){    // Fixed so players can be selected if other player is no longer va //4player                        player[i].playkeys = player[i].newkeys = 0;
                    player[i].lives = PLAYER_LIVES;            // to address new lives settings
                    player[i].joining = 0;
                    player[i].hasplayed = 1;
                    player[i].spawnhealth = model->health;
                    player[i].spawnmp = model->mp;
                    spawnplayer(i);
                    execute_join_script(i);

                    player[i].playkeys = player[i].newkeys = player[i].releasekeys = 0;

                    if(!nodropen) drop_all_enemies();   //27-12-2004  If drop enemies is on, drop all enemies

                    if(!level->noreset) timeleft = level->settime * COUNTER_SPEED;    // Feb 24, 2005 - This line moved here to set custom time
                }

            }
            else if(player[i].playkeys & FLAG_MOVELEFT)
            {
                player[i].colourmap = i;
                model = prevplayermodel(model);
                strcpy(player[i].name, model->name);

                while(   // Keep looping until a non-hmap is found
                    ((model->hmap1) && (model->hmap2) &&
                    player[i].colourmap >= model->hmap1 &&
                    player[i].colourmap <= model->hmap2)
                    )
                    {
                        player[i].colourmap++;
                        if(player[i].colourmap > model->maps_loaded) player[i].colourmap = 0;
                    }

                player[i].playkeys = 0;
            }
            else if(player[i].playkeys & FLAG_MOVERIGHT)
            {
                player[i].colourmap = i;
                model = nextplayermodel(model);
                strcpy(player[i].name, model->name);

                while(   // Keep looping until a non-hmap is found
                    ((model->hmap1) && (model->hmap2) &&
                    player[i].colourmap >= model->hmap1 &&
                    player[i].colourmap <= model->hmap2)
                    )
                    {
                        player[i].colourmap++;
                        if(player[i].colourmap > model->maps_loaded) player[i].colourmap = 0;
                    }

                player[i].playkeys = 0;

            }
            // don't like a characters color try a new one!
            else if(player[i].playkeys & FLAG_MOVEUP && colourselect)
            {

                do{
                    player[i].colourmap++;

                    if(player[i].colourmap > model->maps_loaded) player[i].colourmap = 0;
                }
                while(    // Keep looping until a non-fmap is found
                    (model->fmap &&
                    player[i].colourmap - 1 == model->fmap - 1) ||
                    ((model->hmap1) && (model->hmap2) &&
                    player[i].colourmap - 1 >= model->hmap1 - 1 &&
                    player[i].colourmap - 1 <= model->hmap2 - 1)
                    );

                    player[i].playkeys = 0;
            }
            else if(player[i].playkeys & FLAG_MOVEDOWN && colourselect)
            {

                do{
                    player[i].colourmap--;

                    if(player[i].colourmap < 0) player[i].colourmap = model->maps_loaded;
                }
                while(    // Keep looping until a non-fmap is found
                    (model->fmap &&
                    player[i].colourmap - 1 == model->fmap - 1) ||
                    ((model->hmap1) && (model->hmap2) &&
                    player[i].colourmap - 1 >= model->hmap1 - 1 &&
                    player[i].colourmap - 1 <= model->hmap2 - 1)
                    );

                    player[i].playkeys = 0;
            }
        }
        else if(player[i].credits || credits || (!player[i].hasplayed && noshare))
        {
            if(player[i].credits && (time/(GAME_SPEED*2)) & 1) font_printf(videomodes.shiftpos[i]+pnameJ[i][4], savedata.windowpos+pnameJ[i][5], pnameJ[i][6], 0, "Credit %i", player[i].credits);
            else if(credits && (time/(GAME_SPEED*2)) & 1) font_printf(videomodes.shiftpos[i]+pnameJ[i][4], savedata.windowpos+pnameJ[i][5], pnameJ[i][6], 0, "Credit %i", credits);
            else if(nojoin) font_printf(videomodes.shiftpos[i]+pnameJ[i][4], savedata.windowpos+pnameJ[i][5], pnameJ[i][6], 0, "Please Wait");
            else font_printf(videomodes.shiftpos[i]+pnameJ[i][4], savedata.windowpos+pnameJ[i][5], pnameJ[i][6], 0, "Press Start");

            if(player[i].playkeys & FLAG_START)
            {
                player[i].lives = 0;
                model = (skipselect&&(*skipselect)[current_set][i])?find_model((*skipselect)[current_set][i]):nextplayermodel(NULL);
                strncpy(player[i].name, model->name, MAX_NAME_LEN);
                player[i].colourmap = i;
                 // Keep looping until a non-hmap is found
                while(model->hmap1 && model->hmap2 && player[i].colourmap >= model->hmap1 && player[i].colourmap <= model->hmap2)
                {
                    player[i].colourmap++;
                    if(player[i].colourmap > model->maps_loaded) player[i].colourmap = 0;
                }

                player[i].joining = 1;
                player[i].playkeys = player[i].newkeys = player[i].releasekeys = 0;

                if(!level->noreset) timeleft = level->settime * COUNTER_SPEED;    // Feb 24, 2005 - This line moved here to set custom time

                if(!player[i].hasplayed && noshare) player[i].credits = CONTINUES;

                if(!creditscheat)
                {
                    if(noshare) --player[i].credits;
                    else --credits;
					if(continuescore[current_set] == 1)player[i].score = 0;
					if(continuescore[current_set] == 2)player[i].score = player[i].score+1;
                }
            }
        }
        else
        {
            font_printf(videomodes.shiftpos[i]+pnameJ[i][4], savedata.windowpos+pnameJ[i][5], pnameJ[i][6], 0, "GAME OVER");
        }
    }// end of for

    if(savedata.debuginfo)
    {
		spriteq_add_box(0, videomodes.dOffset-12, videomodes.hRes, videomodes.dOffset+12, 0x0FFFFFFE, 0, 0);
        font_printf(2,                   videomodes.dOffset-10, 0, 0, "FPS: %02d", getFPS());
		font_printf(videomodes.hRes / 2, videomodes.dOffset-10, 0, 0, "Free Ram: %s KBytes", commaprint(freeram/1000));
		font_printf(2,                   videomodes.dOffset,    0, 0, "Total Ram: %s KBytes", commaprint(totalram/1000));
		font_printf(videomodes.hRes / 2, videomodes.dOffset,    0, 0, "Used Ram: %s KBytes", commaprint(usedram/1000));
    }

    dt = timeleft/COUNTER_SPEED;
    if(dt>=99)
    {
        dt      = 99;

        oldtime = 99;
    }
    if(dt<=0)
    {
        dt      = 0;
        oldtime = 99;
    }

    if(dt < oldtime || oldtime == 0)
    {
        execute_timetick_script(dt, go_time);
        oldtime = dt;
    }

    if(timeicon >= 0) spriteq_add_sprite(videomodes.hShift+timeicon_offsets[0], savedata.windowpos+timeicon_offsets[1],10000, timeicon, NULL, 0);
    if(!level->notime) font_printf(videomodes.hShift+timeloc[0]+2, savedata.windowpos+timeloc[1]+2, timeloc[5], 0, "%02i", dt);
    if(showtimeover) font_printf(videomodes.hShift+113, videomodes.vShift+savedata.windowpos+110, timeloc[5], 0, "TIME OVER");

    if(dt<99) showtimeover = 0;

    if(go_time>time)
    {
        dt = (go_time-time)%GAME_SPEED;

        if(dt < GAME_SPEED/2){
            if(level->scrolldir&SCROLL_LEFT){ //TODO: upward and downward go

                if(golsprite >= 0) spriteq_add_sprite(40,60+videomodes.vShift,10000, golsprite, NULL, 0); // new sprite for left direction
                else
                {
                    drawmethod.table = 0;
                    drawmethod.flipx = 1;
                    spriteq_add_sprite(40,60+videomodes.vShift,10000, gosprite, &drawmethod, 0);
                }

                if(gosound == 0 ){

                    if(SAMPLE_GO >= 0) sound_play_sample(SAMPLE_GO, 0, savedata.effectvol,savedata.effectvol, 100);// 26-12-2004 Play go sample as arrow flashes

                    gosound = 1;                // 26-12-2004 Sets sample as already played - stops sample repeating too much
                }
            }
            else if(level->scrolldir&SCROLL_RIGHT){
                spriteq_add_sprite(videomodes.hRes-40,60+videomodes.vShift,10000, gosprite, NULL, 0);

                if(gosound == 0 ){

                    if(SAMPLE_GO >= 0) sound_play_sample(SAMPLE_GO, 0, savedata.effectvol,savedata.effectvol, 100); // 26-12-2004 Play go sample as arrow flashes

                    gosound = 1;  // 26-12-2004 Sets sample as already played - stops sample repeating too much
                }
            }
        }
        else gosound = 0;    //26-12-2004 Resets go sample after loop so it can be played again next time
    }

}

// draw boss status on screen
void drawenemystatus(entity* ent)
{
    s_drawmethod drawmethod;
    int icon;

    if(ent->modeldata.namex>-1000 && ent->modeldata.namey>-1000) font_printf(ent->modeldata.namex, ent->modeldata.namey, 0, 0, "%s", ent->name);

    if(ent->modeldata.iconx>-1000 &&  ent->modeldata.icony>-1000)
    {
        if(ent->health <= 0) icon = ent->modeldata.icondie;
        else if(ent->inpain) icon = ent->modeldata.iconpain;
        else if(ent->getting) icon = ent->modeldata.iconget;
        else icon = ent->modeldata.icon;

        if(icon>=0)
        {
            drawmethod = plainmethod;
            drawmethod.table = ent->colourmap;
            spriteq_add_sprite(ent->modeldata.iconx, ent->modeldata.icony, HUD_Z, icon, &drawmethod, 0);
        }
    }

    if(ent->modeldata.health && ent->modeldata.hpx>-1000 && ent->modeldata.hpy>-1000)
        bar(ent->modeldata.hpx, ent->modeldata.hpy, ent->oldhealth, ent->modeldata.health, &(ent->modeldata.hpbarstatus));
}


void drawstatus(){
    int i;

    for(i=0; i < MAX_PLAYERS; i++)
    {
        if(player[i].ent)
        {
            // Health bars
            bar(videomodes.shiftpos[i]+plife[i][0],savedata.windowpos+plife[i][1], player[i].ent->oldhealth, player[i].ent->modeldata.health, &lbarstatus);
            if(player[i].ent->opponent && !player[i].ent->opponent->modeldata.nolife && player[i].ent->opponent->modeldata.health)
                bar(videomodes.shiftpos[i]+elife[i][0], savedata.windowpos+elife[i][1], player[i].ent->opponent->oldhealth, player[i].ent->opponent->modeldata.health,&olbarstatus); // Tied in with the nolife flag
            // Draw mpbar
            if(player[i].ent->modeldata.mp)
            {
                bar(videomodes.shiftpos[i]+pmp[i][0], savedata.windowpos+pmp[i][1], player[i].ent->oldmp, player[i].ent->modeldata.mp, &mpbarstatus);
            }
        }
    }

    // Time box
    if(!level->notime && !timeloc[4])    // Only draw if notime is set to 0 or not specified
    {
        spriteq_add_line(videomodes.hShift+timeloc[0],                savedata.windowpos+timeloc[1],                videomodes.hShift+timeloc[0]+timeloc[2],     savedata.windowpos+timeloc[1],                HUD_Z, color_black, 0);
        spriteq_add_line(videomodes.hShift+timeloc[0],                savedata.windowpos+timeloc[1],                videomodes.hShift+timeloc[0],                savedata.windowpos+timeloc[1]+timeloc[3],     HUD_Z,  color_black, 0);
        spriteq_add_line(videomodes.hShift+timeloc[0]+timeloc[2],     savedata.windowpos+timeloc[1],                videomodes.hShift+timeloc[0]+timeloc[2],     savedata.windowpos+timeloc[1]+timeloc[3],     HUD_Z,  color_black, 0);
        spriteq_add_line(videomodes.hShift+timeloc[0],                savedata.windowpos+timeloc[1]+timeloc[3],     videomodes.hShift+timeloc[0]+timeloc[2],     savedata.windowpos+timeloc[1]+timeloc[3],     HUD_Z, color_black, 0);
        spriteq_add_line(videomodes.hShift+timeloc[0] - 1,            savedata.windowpos+timeloc[1] - 1,            videomodes.hShift+timeloc[0]+timeloc[2] - 1, savedata.windowpos+timeloc[1] - 1,            HUD_Z+1,   color_white, 0);
        spriteq_add_line(videomodes.hShift+timeloc[0] - 1,            savedata.windowpos+timeloc[1] - 1,            videomodes.hShift+timeloc[0] - 1,            savedata.windowpos+timeloc[1]+timeloc[3] - 1, HUD_Z+1,  color_white, 0);
        spriteq_add_line(videomodes.hShift+timeloc[0]+timeloc[2] - 1, savedata.windowpos+timeloc[1] - 1,            videomodes.hShift+timeloc[0]+timeloc[2] - 1, savedata.windowpos+timeloc[1]+timeloc[3] - 1, HUD_Z+1,  color_white, 0);
        spriteq_add_line(videomodes.hShift+timeloc[0] - 1,            savedata.windowpos+timeloc[1]+timeloc[3] - 1, videomodes.hShift+timeloc[0]+timeloc[2] - 1, savedata.windowpos+timeloc[1]+timeloc[3] - 1, HUD_Z+1, color_white, 0);
    }
}

void update_loading(int pos_x, int pos_y, int size_x, int text_x, int text_y, int value, int max, int font)
{
	font_printf(text_x, text_y, font, 0, "Loading...");
    loadingbarstatus.sizex = size_x;
    bar(pos_x, pos_y, value, max, &loadingbarstatus);
	if(background) putscreen(vscreen, background, 0, 0, NULL);
	else           clearscreen(vscreen);
    spriteq_draw(vscreen, 0);
	video_copy_screen(vscreen);
    spriteq_clear();
	sound_update_music();
	inputrefresh();
}

void addscore(int playerindex, int add){
    unsigned int s = player[playerindex&3].score;
    unsigned int next1up;
    ScriptVariant var; // used for execute script
    Script* ptempscript = pcurrentscript;

    if(playerindex < 0) return;//dont score if <0, e.g., npc damage enemy, enemy damage enemy

    playerindex &= 3;

    next1up = ((s/lifescore)+1) * lifescore;

    s += add;
    if(s>999999999) s=999999999;

    while(s>next1up){

        if(SAMPLE_1UP >= 0) sound_play_sample(SAMPLE_1UP, 0, savedata.effectvol,savedata.effectvol, 100);

        player[playerindex].lives++;
        next1up += lifescore;
    }

    player[playerindex].score = s;

    //execute a script then
    if(Script_IsInitialized(score_script + playerindex))
    {
        ScriptVariant_Clear(&var);
        ScriptVariant_ChangeType(&var, VT_INTEGER);
        var.lVal = (LONG)add;
        Script_Set_Local_Variant("score", &var);
        Script_Execute(score_script+playerindex);
        ScriptVariant_Clear(&var);
        Script_Set_Local_Variant("score", &var);
    }
    pcurrentscript=ptempscript;
}




// ---------------------------- Object handling ------------------------------

void free_ent(entity* e)
{
    int i;
	if(!e) return;
    Script_Clear(e->update_script,      2);
	Script_Clear(e->think_script,       2);
    Script_Clear(e->takedamage_script,  2);
    Script_Clear(e->ondeath_script,     2);
    Script_Clear(e->onkill_script,      2);
    Script_Clear(e->onfall_script,      2);
    Script_Clear(e->onpain_script,      2);
    Script_Clear(e->onblocks_script,    2);
    Script_Clear(e->onblockw_script,    2);
    Script_Clear(e->onblocko_script,    2);
    Script_Clear(e->onblockz_script,    2);
    Script_Clear(e->onblocka_script,    2);
    Script_Clear(e->onmovex_script,     2);
    Script_Clear(e->onmovez_script,     2);
    Script_Clear(e->onmovea_script,     2);
    Script_Clear(e->didblock_script,    2);
	Script_Clear(e->ondoattack_script,   2);
    Script_Clear(e->didhit_script,      2);
    Script_Clear(e->onspawn_script,     2);
    Script_Clear(e->animation_script,   2);
    Script_Clear(e->key_script,         2);
    if(e->update_script){           tracefree(e->update_script);            e->update_script            = NULL; }
    if(e->think_script){            tracefree(e->think_script);             e->think_script             = NULL; }
    if(e->takedamage_script){       tracefree(e->takedamage_script);        e->takedamage_script        = NULL; }
    if(e->onfall_script){           tracefree(e->onfall_script);            e->onfall_script            = NULL; }
    if(e->onpain_script){           tracefree(e->onpain_script);            e->onpain_script            = NULL; }
    if(e->onblocks_script){         tracefree(e->onblocks_script);          e->onblocks_script          = NULL; }
    if(e->onblockw_script){         tracefree(e->onblockw_script);          e->onblockw_script          = NULL; }
    if(e->onblocko_script){         tracefree(e->onblocko_script);          e->onblocko_script          = NULL; }
    if(e->onblockz_script){         tracefree(e->onblockz_script);          e->onblockz_script          = NULL; }
    if(e->onblocka_script){         tracefree(e->onblocka_script);          e->onblocka_script          = NULL; }
    if(e->onmovex_script){          tracefree(e->onmovex_script);           e->onmovex_script           = NULL; }
    if(e->onmovez_script){          tracefree(e->onmovez_script);           e->onmovez_script           = NULL; }
    if(e->onmovea_script){          tracefree(e->onmovea_script);           e->onmovea_script           = NULL; }
    if(e->ondeath_script){          tracefree(e->ondeath_script);           e->ondeath_script           = NULL; }
    if(e->onkill_script){           tracefree(e->onkill_script);            e->onkill_script            = NULL; }
    if(e->didblock_script){         tracefree(e->didblock_script);          e->didblock_script          = NULL; }
	if(e->ondoattack_script){       tracefree(e->ondoattack_script);        e->ondoattack_script        = NULL; }
    if(e->didhit_script){           tracefree(e->didhit_script);            e->didhit_script            = NULL; }
    if(e->onspawn_script){          tracefree(e->onspawn_script);           e->onspawn_script           = NULL; }
    if(e->animation_script){        tracefree(e->animation_script);         e->animation_script         = NULL; }
    if(e->key_script){              tracefree(e->key_script);               e->key_script               = NULL; }
    if(e->defense_factors){         tracefree(e->defense_factors);          e->defense_factors          = NULL; }
    if(e->defense_pain){            tracefree(e->defense_pain);             e->defense_pain             = NULL; }
    if(e->defense_knockdown){       tracefree(e->defense_knockdown);        e->defense_knockdown        = NULL; }
    if(e->defense_blockpower){      tracefree(e->defense_blockpower);       e->defense_blockpower       = NULL; }
    if(e->defense_blockthreshold){  tracefree(e->defense_blockthreshold);   e->defense_blockthreshold   = NULL; }
    if(e->defense_blockratio){      tracefree(e->defense_blockratio);       e->defense_blockratio       = NULL; }
    if(e->defense_blocktype){       tracefree(e->defense_blocktype);        e->defense_blocktype        = NULL; }
    if(e->offense_factors){         tracefree(e->offense_factors);          e->offense_factors          = NULL; }
    if(e->entvars)
    {
        // Although free_ent will be only called once when the engine is shutting down,
        // just clear those in case we forget something
        for(i=0; i<max_entity_vars; i++)
        {
            ScriptVariant_Clear(e->entvars+i);
        }
        tracefree(e->entvars); e->entvars = NULL;
    }
    tracefree(e);
    e = NULL;
}

void free_ents()
{
    int i;
    for(i=0; i<MAX_ENTS; i++) free_ent(ent_list[i]);
}

entity* alloc_ent()
{
    entity* ent = (entity*)tracemalloc("alloc_ent",sizeof(entity));
    if(!ent) return NULL;
    memset(ent, 0, sizeof(entity));
    ent->defense_factors        = (float*)tracemalloc("ent->defense_factors",           sizeof(float)*max_attack_types);
    memset(ent->defense_factors,        0, sizeof(float)*max_attack_types);
    ent->defense_pain           = (float*)tracemalloc("ent->defense_pain",              sizeof(float)*max_attack_types);
    memset(ent->defense_pain,           0, sizeof(float)*max_attack_types);
    ent->defense_knockdown      = (float*)tracemalloc("ent->defense_knockdown",         sizeof(float)*max_attack_types);
    memset(ent->defense_knockdown,      0, sizeof(float)*max_attack_types);
    ent->defense_blockpower     = (float*)tracemalloc("ent->defense_blockpower",        sizeof(float)*max_attack_types);
    memset(ent->defense_blockpower,     0, sizeof(float)*max_attack_types);
    ent->defense_blockthreshold = (float*)tracemalloc("ent->defense_blockthreshold",    sizeof(float)*max_attack_types);
    memset(ent->defense_blockthreshold, 0, sizeof(float)*max_attack_types);
    ent->defense_blockratio     = (float*)tracemalloc("ent->defense_blockratio",        sizeof(float)*max_attack_types);
    memset(ent->defense_blockratio,     0, sizeof(float)*max_attack_types);
    ent->defense_blocktype      = (float*)tracemalloc("ent->defense_blocktype",         sizeof(float)*max_attack_types);
    memset(ent->defense_blocktype,      0, sizeof(float)*max_attack_types);
    ent->offense_factors        = (float*)tracemalloc("ent->offense_factors",           sizeof(float)*max_attack_types);
    memset(ent->offense_factors,        0, sizeof(float)*max_attack_types);
    if(max_entity_vars>0)
    {
        ent->entvars = (ScriptVariant*)tracemalloc("ent->entvars", sizeof(ScriptVariant)*max_entity_vars);
        // memset should be OK by know, because VT_EMPTY is zero by value, or else we should use ScriptVariant_Init
        memset(ent->entvars, 0, sizeof(ScriptVariant)*max_entity_vars);
    }
    ent->update_script      = alloc_script();
    ent->think_script       = alloc_script();
    ent->takedamage_script  = alloc_script();
    ent->onfall_script      = alloc_script();
    ent->onpain_script      = alloc_script();
    ent->onblocks_script    = alloc_script();
    ent->onblockw_script    = alloc_script();
    ent->onblocko_script    = alloc_script();
    ent->onblockz_script    = alloc_script();
    ent->onblocka_script    = alloc_script();
    ent->onmovex_script     = alloc_script();
    ent->onmovez_script     = alloc_script();
    ent->onmovea_script     = alloc_script();
    ent->ondeath_script     = alloc_script();
    ent->onkill_script      = alloc_script();
    ent->didblock_script    = alloc_script();
	ent->ondoattack_script  = alloc_script();
    ent->didhit_script      = alloc_script();
    ent->onspawn_script     = alloc_script();
    ent->animation_script   = alloc_script();
    ent->key_script         = alloc_script();
    return ent;
}


int alloc_ents()
{
    int i;
    for(i=0; i<MAX_ENTS; i++)
	{
        ent_list[i] = alloc_ent();
        if(!ent_list[i])
		{
            free_ents();
            return 0;
        }
        ent_list[i]->sortid = i*100;
    }
    ent_count = ent_max = 0;
    return 1;
}

// this method initialize an entity's A.I. behaviors
void ent_default_init(entity* e)
{
    int dodrop;
    int wall;
    entity *other;

	if(!e) return;

    if((!selectScreen && !time) || e->modeldata.type != TYPE_PLAYER )
    {
        if( validanim(e,ANI_SPAWN)) ent_set_anim(e, ANI_SPAWN, 0); // use new playerselect spawn anim
        //else set_idle(e);
    }
    else if(!selectScreen && time && e->modeldata.type == TYPE_PLAYER) // mid-level respawn
    {
        if( validanim(e,ANI_RESPAWN)) ent_set_anim(e, ANI_RESPAWN, 0);
        else if( validanim(e,ANI_SPAWN)) ent_set_anim(e, ANI_SPAWN, 0);
        //else set_idle(e);
    }
    else if(selectScreen && validanim(e,ANI_SELECT)) ent_set_anim(e, ANI_SELECT, 0);
    //else set_idle(e);

    if(!level)
    {
       if(!e->animation) set_idle(e);
       return;
    }

    switch(e->modeldata.type){
    case TYPE_ENDLEVEL:
    case TYPE_ITEM:
        e->nograb = 1;
        break;

    case TYPE_PLAYER:
        //e->direction = (level->scrolldir != SCROLL_LEFT);
        e->takedamage = player_takedamage;
        e->think = player_think;
        e->trymove = player_trymove;

        if(validanim(e,ANI_SPAWN) || validanim(e,ANI_RESPAWN))
        {
            e->takeaction = common_spawn;
        }
        else if(!e->animation)
        {
            if(time && level->spawn[(int)e->playerindex][2] > e->a)

            {
                e->a = (float)level->spawn[(int)e->playerindex][2];
                if(validanim(e, ANI_JUMP))
                    ent_set_anim(e, ANI_JUMP, 0);
                e->takeaction = common_drop;
            }
        }
        if(time && e->modeldata.makeinv)
        {
			// Spawn invincible code
            e->invincible = 1;
            e->blink = (e->modeldata.makeinv > 0);
            e->invinctime = time + ABS(e->modeldata.makeinv);
            e->arrowon = 1;    // Display the image above the player
        }

        break;

    case TYPE_NPC: // use NPC(or A.I. player) instread of an enemy subtype or trap subtype, for further A.I. use
        if(e->modeldata.multiple ==0) e->modeldata.multiple = -1;

    case TYPE_ENEMY:
        e->think = common_think;
        if(e->modeldata.subtype == SUBTYPE_BIKER)
        {
            e->nograb = 1;
            e->attacking = 1;
            //e->direction = (e->x<0);
            if(e->modeldata.speed) e->xdir = (e->direction)?(e->modeldata.speed):(-e->modeldata.speed);
            else e->xdir = (e->direction)?(1.7 + randf((float)0.6)):(-(1.7 + randf((float)0.6)));
            e->takedamage = biker_takedamage;
            break;
        }
        // define new subtypes
        else if(e->modeldata.subtype == SUBTYPE_ARROW)
        {
            e->health = 1;
            if(!e->modeldata.speed && !e->modeldata.nomove) e->modeldata.speed = 2;    // Set default speed to 2 for arrows
            else if(e->modeldata.nomove) e->modeldata.speed = 0;
            if(e->ptype) e->base = 0;
            else e->base = e->a;
            e->nograb = 1;
            e->attacking = 1;
            e->takedamage = arrow_takedamage;
            break;
        }
        else
        {
            e->trymove = common_trymove;
            // Must just be a regular enemy, set defaults accordingly
            if(!e->modeldata.speed && !e->modeldata.nomove) e->modeldata.speed = 1;
            else if(e->modeldata.nomove) e->modeldata.speed = 0;
            if(e->modeldata.multiple==0) e->modeldata.multiple = 5;
            e->takedamage = common_takedamage;//enemy_takedamage;
        }

        if(e->modeldata.subtype == SUBTYPE_NOTGRAB) e->nograb = 1;

        if(validanim(e,ANI_SPAWN) /*|| validanim(e,ANI_RESPAWN)*/)
        {
            e->takeaction = common_spawn;
        }
        else
        {
            dodrop = (e->modeldata.subtype != SUBTYPE_ARROW && level && (level->scrolldir==SCROLL_UP || level->scrolldir==SCROLL_DOWN));

            if(dodrop || (e->x > advancex-30 && e->x < advancex + videomodes.hRes+30 && e->a == 0))
            {
                e->a += videomodes.vRes + randf(40);
                e->takeaction = common_drop;//enemy_drop;
                if(validanim(e, ANI_JUMP)) ent_set_anim(e, ANI_JUMP, 0);
            }
        }
        break;
        // define trap type
    case TYPE_TRAP:
        e->think = trap_think;
        e->takedamage =  common_takedamage;//enemy_takedamage;
        break;
        //

    case TYPE_OBSTACLE:
        e->nograb = 1;
        if(e->health<=0) e->dead = 1; // so it won't get hit
        e->takedamage = obstacle_takedamage;//obstacle_takedamage;
        break;

    case TYPE_STEAMER:
        e->nograb = 1;
        e->think = steamer_think;
        e->base = e->a;
        break;

    case TYPE_TEXTBOX:    // New type for displaying text purposes
        e->nograb = 1;
        e->think = text_think;
        break;

    case TYPE_SHOT:
        e->health = 1;
        e->nograb = 1;
        e->think = common_think;
        e->takedamage = arrow_takedamage;
        e->attacking = 1;
        if(!e->model->speed && !e->modeldata.nomove) e->modeldata.speed = 2;    // Set default speed to 2 for arrows
        else if(e->modeldata.nomove) e->modeldata.speed = 0;
        if(e->ptype) e->base = 0;
		else e->base = e->a;
        break;

    case TYPE_NONE:
        e->nograb = 1;
        if(e->modeldata.subject_to_gravity<0) e->modeldata.subject_to_gravity = 1;
        //e->base=e->a; //complained?
        if(e->modeldata.no_adjust_base<0) e->modeldata.no_adjust_base= 1;

        if(validanim(e,ANI_WALK))
        {
            if(e->direction) e->xdir = e->modeldata.speed;
            else e->xdir = -(e->modeldata.speed);
            e->think = anything_walk;

            common_walk_anim(e);
            //ent_set_anim(e, ANI_WALK, 0);
        }

        break;
    case TYPE_PANEL:
        e->nograb = 1;
        break;
    }
    if(!e->animation){
        set_idle(e);
    }

    if(e->modeldata.multiple < 0) e->modeldata.multiple = 0;

    if(e->modeldata.subject_to_platform>0 && (other=check_platform_below(e->x, e->z, e->a+1)) && other != e)    e->base += other->a + other->animation->platform[other->animpos][7];
    else if(e->modeldata.subject_to_wall>0 && (wall=checkwall_below(e->x, e->z, 9999999)) >= 0) e->base += level->walls[wall][7];
}

void ent_spawn_ent(entity* ent)
{
    entity* s_ent = NULL;
    float* spawnframe = ent->animation->spawnframe;
	// spawn point relative to current entity
    if(spawnframe[4] == 0)
        s_ent = spawn(ent->x + ((ent->direction)?spawnframe[1]:-spawnframe[1]),ent->z + spawnframe[2], ent->a + spawnframe[3], ent->direction, NULL, ent->animation->subentity, NULL);
    //relative to screen position
	else if(spawnframe[4] == 1)
	{
        if(level && !(level->scrolldir&SCROLL_UP) && !(level->scrolldir&SCROLL_DOWN)) s_ent = spawn(advancex+spawnframe[1], advancey+spawnframe[2], spawnframe[3], 0, NULL, ent->animation->subentity, NULL);
        else s_ent = spawn(advancex+spawnframe[1], spawnframe[2], spawnframe[3], 0, NULL, ent->animation->subentity, NULL);
    }
    //absolute position in level
	else s_ent = spawn(spawnframe[1], spawnframe[2], spawnframe[3]+0.001, 0, NULL, ent->animation->subentity, NULL);

    if(s_ent)
	{
        //ent_default_init(s_ent);
        if(s_ent->modeldata.type & TYPE_SHOT) s_ent->playerindex = ent->playerindex;
        if(s_ent->modeldata.subtype == SUBTYPE_ARROW) s_ent->owner = ent;
        s_ent->parent = ent;  //maybe used by A.I.
        execute_onspawn_script(s_ent);
    }
}

void ent_summon_ent(entity* ent){
    entity* s_ent = NULL;
    float* spawnframe = ent->animation->summonframe;
    // spawn point relative to current entity
	if(spawnframe[4] == 0)
        s_ent = spawn(ent->x + ((ent->direction)?spawnframe[1]:-spawnframe[1]),ent->z + spawnframe[2],  ent->a + spawnframe[3], ent->direction, NULL, ent->animation->subentity, NULL);
    //relative to screen position
	else if(spawnframe[4] == 1)
	{
        if(level && !(level->scrolldir&SCROLL_UP) && !(level->scrolldir&SCROLL_DOWN)) s_ent = spawn(advancex+spawnframe[1], advancey+spawnframe[2], spawnframe[3], 0, NULL, ent->animation->subentity, NULL);
        else s_ent = spawn(advancex+spawnframe[1], spawnframe[2], spawnframe[3], 0, NULL, ent->animation->subentity, NULL);
    }
    //absolute position in level
	else s_ent = spawn(spawnframe[1], spawnframe[2], spawnframe[3], 0, NULL, ent->animation->subentity, NULL);

    if(s_ent)
	{
        if(!spawnframe[4]) s_ent->direction = ent->direction;
        //ent_default_init(s_ent);
        if(s_ent->modeldata.type & TYPE_SHOT) s_ent->playerindex = ent->playerindex;
        if(s_ent->modeldata.subtype == SUBTYPE_ARROW) s_ent->owner = ent;
		//maybe used by A.I.
        s_ent->parent = ent;
        ent->subentity = s_ent;
        execute_onspawn_script(s_ent);
    }
}

// move here to prevent some duplicated code in ent_sent_anim and update_ents
void update_frame(entity* ent, int f)
{
    entity* tempself;
	entity* dust;
    s_attack attack;
    float move, movez, movea;
    int iDelay, iED_Mode, iED_Capmin, iED_CapMax, iED_RangeMin, iED_RangeMax;
    float fED_Factor;
    //important!
    tempself = self;
    self = ent;

    self->animpos = f;
    //self->currentsprite = self->animation->sprite[f];

    if(self->animating){
        iDelay          = self->animation->delay[f];
        iED_Mode        = self->modeldata.edelay.mode;
        fED_Factor      = self->modeldata.edelay.factor;
        iED_Capmin      = self->modeldata.edelay.cap_min;
        iED_CapMax      = self->modeldata.edelay.cap_max;
        iED_RangeMin    = self->modeldata.edelay.range_min;
        iED_RangeMax    = self->modeldata.edelay.range_max;

        if (iDelay >= iED_RangeMin && iDelay <= iED_RangeMax) //Regular delay within ignore ranges?
        {
            switch(iED_Mode)
            {
                case 1:
                    iDelay = (int)(iDelay * fED_Factor);
                    break;
                default:
                    iDelay += (int)fED_Factor;
                    break;
            }

            if (iED_Capmin && iDelay < iED_Capmin){ iDelay = iED_Capmin; }
            if (iED_CapMax && iDelay > iED_CapMax){ iDelay = iED_CapMax; }
        }

        self->nextanim = time + iDelay;
        execute_animation_script(self);
    }

    if(level && (self->animation->move || self->animation->movez))
    {
        move = (float)(self->animation->move?self->animation->move[f]:0);
        movez = (float)(self->animation->movez?self->animation->movez[f]:0);
        if(self->direction==0) move = -move;
        if(movez || move)
        {
            if(self->trymove)
            {
                self->trymove(move, movez);
            }
            else
            {
                self->x += move;
                self->z += movez;
            }
        }
    }

	if(self->animation->seta && self->animation->seta[0] >= 0 && self->base <= 0) ent->base = (float)ent->animation->seta[0];
	else if(!self->animation->seta || self->animation->seta[0] < 0)
	{
	   movea = (float)(self->animation->movea?self->animation->movea[f]:0);
       self->base += movea;
       if(movea!=0) self->altbase += movea;
       else self->altbase = 0;
    }

    if(self->animation->flipframe == f) self->direction = !self->direction;

    if(self->animation->weaponframe && self->animation->weaponframe[0] == f)
    {
		dropweapon(2);
		set_weapon(self, self->animation->weaponframe[1], 0);
		self->idling = 1;
    }

	if(self->animation->quakeframe[0]+self->animation->quakeframe[3] == f)
    {
		if(self->animation->quakeframe[3]%2 || self->animation->quakeframe[2] > 0) quake = self->animation->quakeframe[2];
		else quake = self->animation->quakeframe[2] * -1;
		if((self->animation->quakeframe[1]-self->animation->quakeframe[3]) > 1) self->animation->quakeframe[3]++;
		else self->animation->quakeframe[3] = 0;
    }

    //spawn / summon /unsummon features
    if(self->animation->spawnframe && self->animation->spawnframe[0] == f && self->animation->subentity) ent_spawn_ent(self);

    if(self->animation->summonframe && self->animation->summonframe[0] == f && self->animation->subentity)
    {
		//subentity is dead
		if(!self->subentity || self->subentity->dead) ent_summon_ent(self);
    }

    if(self->animation->unsummonframe == f)
    {
        if(self->subentity)
        {
            self = self->subentity;
            attack = emptyattack;
            attack.dropv[0] = (float)3; attack.dropv[1] = (float)1.2; attack.dropv[2] = (float)0;
            attack.attack_force = self->health;
            attack.attack_type = max_attack_types;
            if(self->takedamage) self->takedamage(self, &attack);
            else kill(self);
            self = ent; // lol ...
            self->subentity = NULL;
        }
    }

    if(self->animation->soundtoplay && self->animation->soundtoplay[f] >= 0) sound_play_sample(self->animation->soundtoplay[f], 0, savedata.effectvol,savedata.effectvol, 100);

    if(self->animation->jumpframe == f)
    {
        // Set custom jumpheight for jumpframes
		/*if(self->animation->jumpv > 0)*/ toss(self, self->animation->jumpv);
        self->xdir = self->direction?self->animation->jumpx:-self->animation->jumpx;
        self->zdir = self->animation->jumpz;

		if(self->animation->jumpd>=0)
        {
            dust = spawn(self->x, self->z, self->a, self->direction, NULL, self->animation->jumpd, NULL);
            dust->base = self->a;
            dust->autokill = 1;
            execute_onspawn_script(dust);
        }
    }

    if(self->animation->throwframe == f)
    {
		// For backward compatible thing
		// throw stars in the air, hmm, strange
		// custstar custknife in animation should be checked first
		// then if the entiti is jumping, check star first, if failed, try knife instead
		// well, try knife at last, if still failed, try star, or just let if shutdown?
#define __trystar star_spawn(self->x + (self->direction ? 56 : -56), self->z, self->a+67, self->direction)
#define __tryknife knife_spawn(NULL, -1, self->x, self->z, self->a + self->animation->throwa, self->direction, 0, 0)
		if(self->animation->custknife>=0 || self->animation->custpshotno>=0) __tryknife;
        else if(self->animation->custstar>=0) __trystar;
        else if(self->jumping) { if(!__trystar) __tryknife;}
        else if(!__tryknife) __trystar;
        self->reactive=1;
    }

	if(self->animation->shootframe == f)
    {
        knife_spawn(NULL, -1, self->x, self->z, self->a, self->direction, 1, 0);
        self->reactive=1;
    }

    if(self->animation->tossframe == f)
    {
        bomb_spawn(NULL, -1, self->x, self->z, self->a + self->animation->throwa, self->direction, 0);
        self->reactive=1;
    }
    //important!
    self = tempself;
}


void ent_set_anim(entity *ent, int aninum, int resetable)
{
    s_anim *ani = NULL;

    if(ent==NULL) shutdown(1, "FATAL: tried to set animation with invalid address (no such object)");
    if(aninum<0 || aninum>=max_animations) shutdown(1, "FATAL: tried to set animation with invalid index (%s, %i)", ent->name, aninum);

    if(!validanim(ent, aninum)) shutdown(1, "FATAL: tried to set animation with invalid address (%s, %i)", ent->name, aninum);

    ani = ent->modeldata.animation[aninum];

    if(!resetable && ent->animation == ani) return;

    if(ani->numframes == 0) return;

    if(aninum!=ANI_SLEEP) ent->sleeptime = time + ent->modeldata.sleepwait;
    ent->animation = ani;
    ent->animnum = aninum;    // Stored for nocost usage
	ent->animation->animhits = 0;

    if(!resetable) ent->lastanimpos = -1;
    ent->animating = 1;
    ent->lasthit = ent->grabbing;
    ent->altbase = 0;

    update_frame(ent, 0);
}



// 0 = none, 1+ = alternative
void ent_set_colourmap(entity *ent, unsigned int which)
{
    if(which>MAX_COLOUR_MAPS) which = 0;
    if(which==0) ent->colourmap = NULL;
    else ent->colourmap = ent->modeldata.colourmap[which-1];
    ent->map = which;
}

// used by ent_set_model
void ent_copy_uninit(entity* ent, s_model* oldmodel)
{
    if(ent->modeldata.multiple<0)               ent->modeldata.multiple             = oldmodel->multiple;
    if(ent->modeldata.subject_to_wall<0)        ent->modeldata.subject_to_wall      = oldmodel->subject_to_wall;
    if(ent->modeldata.subject_to_platform<0)    ent->modeldata.subject_to_platform  = oldmodel->subject_to_platform;
    if(ent->modeldata.subject_to_obstacle<0)    ent->modeldata.subject_to_obstacle  = oldmodel->subject_to_obstacle;
    if(ent->modeldata.subject_to_hole<0)        ent->modeldata.subject_to_hole      = oldmodel->subject_to_hole;
    if(ent->modeldata.subject_to_gravity<0)     ent->modeldata.subject_to_gravity   = oldmodel->subject_to_gravity;
    if(ent->modeldata.subject_to_screen<0)      ent->modeldata.subject_to_screen    = oldmodel->subject_to_screen;
    if(ent->modeldata.subject_to_minz<0)        ent->modeldata.subject_to_minz      = oldmodel->subject_to_minz;
    if(ent->modeldata.subject_to_maxz<0)        ent->modeldata.subject_to_maxz      = oldmodel->subject_to_maxz;
    if(ent->modeldata.no_adjust_base<0)         ent->modeldata.no_adjust_base       = oldmodel->no_adjust_base;
    if(ent->modeldata.aimove<0)                 ent->modeldata.aimove               = oldmodel->aimove;
    if(ent->modeldata.aiattack<0)               ent->modeldata.aiattack             = oldmodel->aiattack;
    if(ent->modeldata.hostile<0)                ent->modeldata.hostile              = oldmodel->hostile;
    if(ent->modeldata.candamage<0)              ent->modeldata.candamage            = oldmodel->candamage;
    if(ent->modeldata.projectilehit<0)          ent->modeldata.projectilehit        = oldmodel->projectilehit;
    if(!ent->modeldata.health)                  ent->modeldata.health               = oldmodel->health;
    if(!ent->modeldata.mp)                      ent->modeldata.mp                   = oldmodel->mp;
    if(ent->modeldata.risetime[0]==-1)          ent->modeldata.risetime[0]          = oldmodel->risetime[0];

    if(ent->health>ent->modeldata.health) ent->health = ent->modeldata.health;
    if(ent->mp>ent->modeldata.mp) ent->mp = ent->modeldata.mp;
}


void ent_set_model(entity * ent, char * modelname)
{
    s_model *m = NULL;
    s_model oldmodel;
    if(ent==NULL) shutdown(1, "FATAL: tried to change model of invalid object");
    m = find_model(modelname);
    if(m==NULL) shutdown(1, "Model not found: '%s'", modelname);
    oldmodel = ent->modeldata;
    ent->model = m;
    ent->modeldata = *m;
    ent_copy_uninit(ent, &oldmodel);
    ent_set_colourmap(ent, ent->map);
    if((!selectScreen && !time) || ent->modeldata.type != TYPE_PLAYER)
	{
		// use new playerselect spawn anim
        if( validanim(ent,ANI_SPAWN)) ent_set_anim(ent, ANI_SPAWN, 0);
        else ent_set_anim(ent, ANI_IDLE, 0);
	}
    else if(!selectScreen && time && ent->modeldata.type == TYPE_PLAYER)
	{
		// mid-level respawn
        if( validanim(ent, ANI_RESPAWN)) ent_set_anim(ent, ANI_RESPAWN, 0);
        else if( validanim(ent, ANI_SPAWN)) ent_set_anim(ent, ANI_SPAWN, 0);
        else  ent_set_anim(ent, ANI_IDLE, 0);
	}
    else if(selectScreen && validanim(ent, ANI_SELECT)) ent_set_anim(ent, ANI_SELECT, 0);
    else ent_set_anim(ent, ANI_IDLE, 0);
}


entity * spawn(float x, float z, float a, int direction, char * name, int index, s_model* model)
{
    entity *e = NULL;
    int i, id;
    float *dfs, *dfsp, *dfsk, *dfsbp, *dfsbt, *dfsbr, *dfsbe, *ofs;
    ScriptVariant* vars;
    Script* pas, *pus, *pts, *pks, *pds, *pan, *pfs, *bls, *blw, *blo, *blz, *bla, *mox, *moz, *moa, *pis, *pkl, *phs, *osp, *pbs, *ocs;

    if(!model)
    {
        if(index>=0) model = model_cache[index].model;
        else if(name) model = find_model(name);
    }

	// Be a bit more tolerant...
    if(model==NULL)
    {
		if(index>=0)
			printf("FATAL: attempt to spawn object with invalid model cache id (%d)!\n", index);
		else if(name)
			printf("FATAL: attempt to spawn object with invalid model name (%s)!\n", name);
        return NULL;
    }

    for(i=0; i<MAX_ENTS; i++)
    {
        if(!ent_list[i]->exists)
        {
            e = ent_list[i];
            // save these values, or they will loss when memset called
            id      = e->sortid;
			dfs     = e->defense_factors;
			dfsp    = e->defense_pain;
			dfsk    = e->defense_knockdown;
			dfsbp   = e->defense_blockpower;
			dfsbt   = e->defense_blockthreshold;
			dfsbr   = e->defense_blockratio;
            dfsbe   = e->defense_blocktype;
			ofs     = e->offense_factors;
			vars    = e->entvars;
            memset(dfs,     0, sizeof(float)*max_attack_types);
            memset(dfsp,    0, sizeof(float)*max_attack_types);
            memset(dfsk,    0, sizeof(float)*max_attack_types);
            memset(dfsbp,   0, sizeof(float)*max_attack_types);
            memset(dfsbt,   0, sizeof(float)*max_attack_types);
            memset(dfsbr,   0, sizeof(float)*max_attack_types);
            memset(dfsbe,   0, sizeof(float)*max_attack_types);
            memset(ofs,     0, sizeof(float)*max_attack_types);
            // clear up
            Script_Clear(e->animation_script,   1);
            Script_Clear(e->update_script,      1);
            Script_Clear(e->think_script,       1);
            Script_Clear(e->takedamage_script,  1);
            Script_Clear(e->onfall_script,      1);
            Script_Clear(e->onpain_script,      1);
            Script_Clear(e->onblocks_script,    1);
            Script_Clear(e->onblockw_script,    1);
            Script_Clear(e->onblocko_script,    1);
            Script_Clear(e->onblockz_script,    1);
            Script_Clear(e->onblocka_script,    1);
            Script_Clear(e->onmovex_script,     1);
            Script_Clear(e->onmovez_script,     1);
            Script_Clear(e->onmovea_script,     1);
            Script_Clear(e->ondeath_script,     1);
            Script_Clear(e->onkill_script,      1);
            Script_Clear(e->didblock_script,    1);
			Script_Clear(e->ondoattack_script,  1);
            Script_Clear(e->didhit_script,      1);
            Script_Clear(e->onspawn_script,     1);
            Script_Clear(e->key_script,         1);
            pas = e->animation_script;
            pus = e->update_script;
			pts = e->think_script;
            pds = e->takedamage_script;
            pfs = e->onfall_script;
            pan = e->onpain_script;
            bls = e->onblocks_script;
            blw = e->onblockw_script;
            blo = e->onblocko_script;
            blz = e->onblockz_script;
            bla = e->onblocka_script;
            mox = e->onmovex_script;
            moz = e->onmovez_script;
            moa = e->onmovea_script;
            pis = e->ondeath_script;
            pkl = e->onkill_script;
            pbs = e->didblock_script;
			ocs = e->ondoattack_script;
            phs = e->didhit_script;
            osp = e->onspawn_script;
            pks = e->key_script;
            memset(e, 0, sizeof(entity));

            // add to list and count current entities
            e->exists = 1;
            ent_count++;

            e->modeldata = *model; // copy the entir model data here
            e->model = model;
            e->defaultmodel = model;

            e->animation_script     = pas;
            e->update_script        = pus;
			e->think_script         = pts;
            e->takedamage_script    = pds;
            e->onfall_script        = pfs;
            e->onpain_script        = pan;
            e->onblocks_script      = bls;
            e->onblockw_script      = blw;
            e->onblocko_script      = blo;
            e->onblockz_script      = blz;
            e->onblocka_script      = bla;
            e->onmovex_script       = mox;
            e->onmovez_script       = moz;
            e->onmovea_script       = moa;
            e->ondeath_script       = pis;
            e->onkill_script        = pkl;
            e->didblock_script      = pbs;
			e->ondoattack_script    = ocs;
            e->didhit_script        = phs;
            e->onspawn_script       = osp;
            e->key_script           = pks;
            // copy from model a fresh script
            Script_Copy(e->animation_script,    model->animation_script,    1);
            Script_Copy(e->update_script,       model->update_script,       1);
            Script_Copy(e->think_script,        model->think_script,        1);
            Script_Copy(e->takedamage_script,   model->takedamage_script,   1);
            Script_Copy(e->onfall_script,       model->onfall_script,       1);
            Script_Copy(e->onpain_script,       model->onpain_script,       1);
            Script_Copy(e->onblocks_script,     model->onblocks_script,     1);
            Script_Copy(e->onblockw_script,     model->onblockw_script,     1);
            Script_Copy(e->onblocko_script,     model->onblocko_script,     1);
            Script_Copy(e->onblockz_script,     model->onblockz_script,     1);
            Script_Copy(e->onblocka_script,     model->onblocka_script,     1);
            Script_Copy(e->onmovex_script,      model->onmovex_script,      1);
            Script_Copy(e->onmovez_script,      model->onmovez_script,      1);
            Script_Copy(e->onmovea_script,      model->onmovea_script,      1);
            Script_Copy(e->ondeath_script,      model->ondeath_script,      1);
            Script_Copy(e->onkill_script,       model->onkill_script,       1);
            Script_Copy(e->didblock_script,     model->didblock_script,     1);
			Script_Copy(e->ondoattack_script,   model->ondoattack_script,   1);
            Script_Copy(e->didhit_script,       model->didhit_script,       1);
            Script_Copy(e->onspawn_script,      model->onspawn_script,      1);
            Script_Copy(e->key_script,          model->key_script,          1);
            if(ent_count>ent_max) ent_max=ent_count;
            e->timestamp = time; // log time so update function will ignore it if it is new

            e->health = e->modeldata.health;
            e->mp = e->modeldata.mp;
            e->knockdowncount = e->modeldata.knockdowncount;
            e->x = x;
            e->z = z;
            e->a = a;
            e->direction = direction;
            e->nextthink = time + 1;
            e->lifespancountdown = model->lifespan; // new life span countdown
            if((e->modeldata.type & (TYPE_PLAYER|TYPE_SHOT)) && level && (level->nohit || savedata.mode)) e->modeldata.hostile &= ~TYPE_PLAYER;
            if(e->modeldata.type==TYPE_PLAYER) e->playerindex = currentspawnplayer;

            if(e->modeldata.type == TYPE_TEXTBOX) textbox = e;

            strncpy(e->name, e->modeldata.name, MAX_NAME_LEN);
            // copy back the value
            e->sortid = id;
            e->defense_factors = dfs;
            e->defense_pain = dfsp;
			e->defense_knockdown = dfsk;
			e->defense_blockpower = dfsbp;
			e->defense_blockthreshold = dfsbt;
			e->defense_blockratio = dfsbr;
			e->defense_blocktype = dfsbe;
            e->offense_factors = ofs;
			e->entvars = vars;

            ent_default_init(e);
            return e;
        }
    }

    return NULL;
}



// Break the link an entity has with another one
void ent_unlink(entity *e){
    if(e->link){
        e->link->link = NULL;
        e->link->grabbing = NULL;
    }
    e->link = NULL;
    e->grabbing = NULL;
}



// Link two entities together
void ents_link(entity *e1, entity *e2)
{
    ent_unlink(e1);
    ent_unlink(e2);
    e1->grabbing = e2;    // Added for platform layering
    e1->link = e2;
    e2->link = e1;
}



void kill(entity *victim)
{
    int i = 0;
    s_attack attack;
    entity* tempent = self;

    execute_onkill_script(victim);

    if(!victim || !victim->exists) return;

    if(victim->modeldata.type == TYPE_SHOT && player[(int)victim->playerindex].ent) player[(int)victim->playerindex].ent->cantfire = 0;

    ent_unlink(victim);
    victim->weapent = NULL;
    victim->health = 0;
    victim->exists = 0;
    ent_count--;

    Script_Clear(victim->animation_script,  1);
    Script_Clear(victim->update_script,     1);
    Script_Clear(victim->think_script,      1);
    Script_Clear(victim->takedamage_script, 1);
    Script_Clear(victim->onfall_script,     1);
    Script_Clear(victim->onpain_script,     1);
    Script_Clear(victim->onblocks_script,   1);
    Script_Clear(victim->onblockw_script,   1);
    Script_Clear(victim->onblocko_script,   1);
    Script_Clear(victim->onblockz_script,   1);
    Script_Clear(victim->onblocka_script,   1);
    Script_Clear(victim->onmovex_script,    1);
    Script_Clear(victim->onmovez_script,    1);
    Script_Clear(victim->onmovea_script,    1);
    Script_Clear(victim->ondeath_script,    1);
    Script_Clear(victim->onkill_script,     1);
    Script_Clear(victim->didblock_script,   1);
	Script_Clear(victim->ondoattack_script, 1);
    Script_Clear(victim->didhit_script,     1);
    Script_Clear(victim->onspawn_script,    1);
    Script_Clear(victim->key_script,        1);

    if(victim->parent && victim->parent->subentity == victim) victim->parent->subentity = NULL;
    victim->parent = NULL;
    if(victim->modeldata.summonkill)
    {
        attack = emptyattack;
        attack.attack_type = max_attack_types;
        attack.dropv[0] = (float)3; attack.dropv[1] = (float)1.2; attack.dropv[2] = (float)0;
    }
    // kill minions
    if(victim->modeldata.summonkill == 1 && victim->subentity)
    { // kill only summoned one
        victim->subentity->parent = NULL;
        self = victim->subentity;
        attack.attack_force = self->health;
        if(self->takedamage && !level_completed) self->takedamage(self, &attack);
        else kill(self);
    }
    victim->subentity = NULL;

    if(victim==player[0].ent) player[0].ent = NULL;
    else if(victim==player[1].ent) player[1].ent = NULL;
    else if(victim==player[2].ent) player[2].ent = NULL;
    else if(victim==player[3].ent) player[3].ent = NULL;

    if(victim == smartbomber) smartbomber = NULL;
    if(victim == textbox)  textbox = NULL;

    for(i=0; i<ent_max; i++)
    {
        if(ent_list[i]->exists)
        {// kill all minions
            self = ent_list[i];
            if(self->parent == victim)
            {
                self->parent = NULL;
                if(victim->modeldata.summonkill == 2)
                {
                    attack.attack_force = self->health;
                    if(self->takedamage && !level_completed) self->takedamage(self, &attack);
                    else kill(self);
                }
            }
            if(self->owner == victim)
            {
                self->owner = victim->owner;
            }
            if(self->opponent == victim) self->opponent = NULL;
            if(self->bound == victim) self->bound = NULL;
            if(self->landed_on_platform == victim) self->landed_on_platform = NULL;
            if(self->hithead == victim) self->hithead = NULL;
            if(!textbox && self->modeldata.type == TYPE_TEXTBOX)
                textbox = self;
        }
    }
    self = tempent;
}


void kill_all()
{
    int i;
    entity *e = NULL;
    for(i=0; i<ent_max; i++)
    {
        e = ent_list[i];

        if (e && e->exists) execute_onkill_script(e);
        e->exists = 0; // well, no need to use kill function
    }
    textbox = smartbomber = NULL;
    ent_max = ent_count = 0;
    time = 0;
}


int checkhit(entity *attacker, entity *target, int counter)
{
    short *coords1;
    short *coords2;
    int x1, x2, y1, y2;
    float medx, medy;
    int debug_coords[2][4];
    int topleast, bottomleast, leftleast, rightleast;
    float zdist = 0;

    if(attacker == target || !target->animation->bbox_coords ||
       !attacker->animation->attacks || !target->animation->vulnerable[target->animpos] ||
       ((attacker->modeldata.type == TYPE_PLAYER && target->modeldata.type == TYPE_PLAYER) && savedata.mode)) return 0;


    coords1 = attacker->animation->attacks[attacker->animpos]->attack_coords;

    if(!counter) coords2 = target->animation->bbox_coords[target->animpos];
	else if((target->animation->attacks && target->animation->attacks[target->animpos]) && target->animation->attacks[target->animpos]->counterattack <= attacker->animation->attacks[attacker->animpos]->counterattack) coords2 = target->animation->attacks[target->animpos]->attack_coords;
    else return 0;

    if(coords1[4]) zdist += coords1[4];
    else           zdist += attacker->modeldata.grabdistance/3;
    if(coords2[4]) zdist += coords2[4];

    if(diff(attacker->z,target->z) > zdist) return 0;

    x1 = (int)(attacker->x);
    y1 = (int)(attacker->z - attacker->a);
    x2 = (int)(target->x);
    y2 = (int)(target->z - target->a);


    if(attacker->direction==0){
        debug_coords[0][0] = x1-coords1[2];
        debug_coords[0][1] = y1+coords1[1];
        debug_coords[0][2] = x1-coords1[0];
        debug_coords[0][3] = y1+coords1[3];
    }
    else{
        debug_coords[0][0] = x1+coords1[0];
        debug_coords[0][1] = y1+coords1[1];
        debug_coords[0][2] = x1+coords1[2];
        debug_coords[0][3] = y1+coords1[3];
    }
    if(target->direction==0){
        debug_coords[1][0] = x2-coords2[2];
        debug_coords[1][1] = y2+coords2[1];
        debug_coords[1][2] = x2-coords2[0];
        debug_coords[1][3] = y2+coords2[3];
    }
    else{
        debug_coords[1][0] = x2+coords2[0];
        debug_coords[1][1] = y2+coords2[1];
        debug_coords[1][2] = x2+coords2[2];
        debug_coords[1][3] = y2+coords2[3];
    }

    if(debug_coords[0][0] > debug_coords[1][2]) return 0;
    if(debug_coords[1][0] > debug_coords[0][2]) return 0;
    if(debug_coords[0][1] > debug_coords[1][3]) return 0;
    if(debug_coords[1][1] > debug_coords[0][3]) return 0;


    // Find center of attack area
    leftleast = debug_coords[0][0];
    if(leftleast < debug_coords[1][0]) leftleast = debug_coords[1][0];
    topleast = debug_coords[0][1];
    if(topleast < debug_coords[1][1]) topleast = debug_coords[1][1];
    rightleast = debug_coords[0][2];
    if(rightleast > debug_coords[1][2]) rightleast = debug_coords[1][2];
    bottomleast = debug_coords[0][3];
    if(bottomleast > debug_coords[1][3]) bottomleast = debug_coords[1][3];

    medx = (float)(leftleast + rightleast) / 2;
    medy = (float)(topleast + bottomleast) / 2;

    // Now convert these coords to 3D
    lasthitx = medx;

    if(attacker->z > target->z) lasthitz = attacker->z + 1;    // Changed so flashes always spawn in front
    else lasthitz = target->z + 1;

    lasthita = lasthitz - medy;
    lasthitt = attacker->animation->attacks[attacker->animpos]->attack_type;
    lasthitc = 1;
	return 1;
}



/*
Calculates the coef relative to the bottom left point. This is done by figuring out how far the entity is from
the bottom of the platform and multiplying the result by the difference of the bottom left point and the top
left point divided by depth of the platform. The same is done for the right side, and checks to see if they are
within the bottom/top and the left/right area.
*/
int testhole(int hole, float x, float z)
{
    float coef1, coef2;
    if(z < level->holes[hole][1] && z > level->holes[hole][1] - level->holes[hole][6])
    {
        coef1 = (level->holes[hole][1] - z) * ((level->holes[hole][2] - level->holes[hole][3]) / level->holes[hole][6]);
        coef2 = (level->holes[hole][1] - z) * ((level->holes[hole][4] - level->holes[hole][5]) / level->holes[hole][6]);
        if(x > level->holes[hole][0] + level->holes[hole][3] + coef1 && x < level->holes[hole][0] + level->holes[hole][5] + coef2)
            return 1;
    }
    return 0;
}

/// find all holes here and return the count
int checkholes(float x, float z)
{
    int i, c;

    for(i=0, c=0; i<level->numholes; i++)
        c += (level->holesfound[i] =  testhole(i, x, z));

    return c;
}

// find the 1st hole here
int checkhole(float x, float z)
{
    int i;

    if(level==NULL) return 0;

    if(level->exit_hole){
        if(x > level->width-(PLAYER_MAX_Z-z)) return 2;
    }

    for(i=0; i<level->numholes; i++)
    {
        if(testhole(i, x, z))
        {
            holez = i;
            return 1;
        }
    }
    return 0;
}

/*
Calculates the coef relative to the bottom left point. This is done by figuring out how far the entity is from
the bottom of the platform and multiplying the result by the difference of the bottom left point and the top
left point divided by depth of the platform. The same is done for the right side, and checks to see if they are
within the bottom/top and the left/right area.
*/
int testwall(int wall, float x, float z)
{
    float coef1, coef2;
//    if(wall >= level->numwalls || wall < 0) return 0;
    if(z < level->walls[wall][1] && z > level->walls[wall][1] - level->walls[wall][6])
    {
        coef1 = (level->walls[wall][1] - z) * ((level->walls[wall][2] - level->walls[wall][3]) / level->walls[wall][6]);
        coef2 = (level->walls[wall][1] - z) * ((level->walls[wall][4] - level->walls[wall][5]) / level->walls[wall][6]);
        if(x > level->walls[wall][0] + level->walls[wall][3] + coef1 && x < level->walls[wall][0] + level->walls[wall][5] + coef2) return 1;
    }

    return 0;
}

// find all walls here within altitude1 and 2, return the count
int checkwalls(float x, float z, float a1, float a2)
{
    int i, c;

    for(i=0, c=0; i<level->numwalls; i++)
        c += (level->wallsfound[i] =  (testwall(i, x, z) && level->walls[i][7] >= a1 && level->walls[i][7] <= a2));

    return c;
}

// get a highest wall below this altitude
int checkwall_below(float x, float z, float a)
{
    float maxa;
    int i, ind;

    if(level==NULL) return -1;

    maxa = 0;
    ind = -1;
    for(i=0; i<level->numwalls; i++)
    {
        if(testwall(i, x, z) && level->walls[i][7] < a+0.1 && level->walls[i][7] > maxa)
        {
            maxa = level->walls[i][7];
            ind = i;
        }
    }

    return ind;
}

// return the 1st wall found here
int checkwall(float x, float z)
{
    int i;
    if(level==NULL) return -1;

    for(i=0; i<level->numwalls; i++)
        if(testwall(i, x, z)) return i;

    return -1;
}

/*
Calculates the coef relative to the bottom left point. This is done by figuring out how far the entity is from
the bottom of the platform and multiplying the result by the difference of the bottom left point and the top
left point divided by depth of the platform. The same is done for the right side, and checks to see if they are
within the bottom/top and the left/right area.
*/
int testplatform(entity* plat, float x, float z)
{
    float coef1, coef2;
    float offz, offx;
    if(!plat->animation || !plat->animation->platform || !plat->animation->platform[plat->animpos][7]) return 0;
    offz = plat->z+plat->animation->platform[plat->animpos][1];
    offx = plat->x+plat->animation->platform[plat->animpos][0];
    if(z <= offz && z > offz - plat->animation->platform[plat->animpos][6])
    {
        coef1 = (offz - z) * ((plat->animation->platform[plat->animpos][2] -
            plat->animation->platform[plat->animpos][3]) / plat->animation->platform[plat->animpos][6]);
        coef2 = (offz - z) * ((plat->animation->platform[plat->animpos][4] -
            plat->animation->platform[plat->animpos][5]) / plat->animation->platform[plat->animpos][6]);

        if(x > offx + plat->animation->platform[plat->animpos][3] + coef1 &&
           x < offx + plat->animation->platform[plat->animpos][5] + coef2) return 1;
    }
    return 0;
}


//find the first platform between these 2 altitudes
entity * check_platform_between(float x, float z, float amin, float amax)
{
    entity *plat = NULL;
    int i;

    if(level==NULL) return NULL;

    for(i=0; i<ent_max; i++)
    {
        if(ent_list[i]->exists && testplatform(ent_list[i], x, z) )
        {
            plat = ent_list[i];
            if(plat->a <= amax && plat->a + plat->animation->platform[plat->animpos][7] > amin)
            {
                return plat;
            }
        }
    }
    return NULL;
}

//find a lowest platform above this altitude
entity * check_platform_above(float x, float z, float a)
{
    float mina;
    entity *plat = NULL;
    int i, ind;

    if(level==NULL) return NULL;

    mina = 9999999;
    ind = -1;
    for(i=0; i<ent_max; i++)
    {
        if(ent_list[i]->exists && testplatform(ent_list[i], x, z) )
        {
            plat = ent_list[i];
            if(plat->a >= a && plat->a < mina)
            {
                mina = plat->a;
                ind = i;
            }
        }
    }
    return (ind>=0)?ent_list[ind]:NULL;
}

//find a highest platform below this altitude
entity * check_platform_below(float x, float z, float a)
{
    float maxa;
    entity *plat = NULL;
    int i, ind;

    if(level==NULL) return NULL;

    maxa = 0;
    ind = -1;
    for(i=0; i<ent_max; i++)
    {
        if(ent_list[i]->exists && testplatform(ent_list[i], x, z) )
        {
            plat = ent_list[i];
            if(plat->a + plat->animation->platform[plat->animpos][7] <= a &&
               plat->a + plat->animation->platform[plat->animpos][7] > maxa)
            {
                maxa = plat->a + plat->animation->platform[plat->animpos][7];
                ind = i;
            }
        }
    }
    return (ind>=0)?ent_list[ind]:NULL;
}

// find the 1st platform entity here
entity * check_platform(float x, float z)
{
    int i;
    if(level==NULL) return NULL;

    for(i=0; i<ent_max; i++)
    {
        if(ent_list[i]->exists && testplatform(ent_list[i], x, z))
        {
            return ent_list[i];
        }
    }
    return NULL;
}

// find real opponent
void set_opponent(entity* ent, entity* other)
{
    entity* realself, *realother;

    if(!ent) return;

    realself = ent;
    while(realself->owner) realself = realself->owner;

    realother = other;
    while(realother && realother->owner) realother = realother->owner;

    realself->opponent = ent->opponent = realother;
    if(realother) realother->opponent = other->opponent = realself;

}


void do_attack(entity *e)
{
    int them;
    int i;
    int force;
    entity *temp            = NULL;
    entity *flash           = NULL;    // Used so new flashes can be used
    entity *def             = NULL;
    entity *topowner        = NULL;
    entity *otherowner      = NULL;
    int didhit              = 0;
    int didblock            = 0;    // So a different sound effect can be played when an attack is blocked
    int current_attack_id;
    int current_follow_id   = 0;
    int followed = 0;
    s_anim* current_anim;
    s_attack* attack = e->animation->attacks[e->animpos];
    static unsigned int new_attack_id = 1;
	int fdefense_blockthreshold = (int)self->modeldata.defense_blockthreshold[(short)attack->attack_type]; //Maximum damage that can be blocked for attack type.

    // Can't get hit after this
    if(level_completed || !attack) return;

    topowner = e; // trace the top owner, for projectile combo checking :)
    while(topowner->owner) topowner = topowner->owner;

    if(e->projectile>0) them = e->modeldata.projectilehit;
    else them = e->modeldata.candamage;

    // Every attack gets a unique ID to make sure no one
    // gets hit more than once by the same attack
    current_attack_id = e->attack_id;

    if(!current_attack_id)
    {
        ++new_attack_id;
        if(new_attack_id==0) new_attack_id = 1;
        e->attack_id = current_attack_id = new_attack_id;
    }

    force = attack->attack_force;
    current_anim = e->animation;

    for(i=0; i<ent_max && !followed; i++)
    {

		// if #0
        if( ent_list[i]->exists &&
            !ent_list[i]->dead && // dont hit the dead
            (ent_list[i]->invincible != 1 || attack->attack_type == ATK_ITEM) && // so invincible people can get items
            !(current_anim->attackone>0 && e->lasthit && ent_list[i]!=e->lasthit) &&
            (ent_list[i]->modeldata.type & them) &&
            (ent_list[i]->pain_time<time || e->animation->fastattack) &&
            ent_list[i]->takedamage &&
            ent_list[i]->hit_by_attack_id != current_attack_id &&
			((ent_list[i]->takeaction != common_lie && attack->otg < 2) || (attack->otg >= 1 && ent_list[i]->takeaction == common_lie)) && //over the ground hit
			((ent_list[i]->falling == 0 && attack->jugglecost >= 0) || (ent_list[i]->falling == 1 && attack->jugglecost <= ent_list[i]->modeldata.jugglepoints[0])) && // juggle system
            (checkhit(e, ent_list[i], 0) || // normal check bbox
             (attack->counterattack && checkhit(e, ent_list[i], 1)))  )// check counter, e.g. upper
        {
    		temp = self;
            self = ent_list[i];

			execute_ondoattack_script(self, e, force, attack->attack_drop, attack->attack_type, attack->no_block, attack->guardcost, attack->jugglecost, attack->pause_add, 0, current_attack_id);	//Execute on defender.
			execute_ondoattack_script(e, self, force, attack->attack_drop, attack->attack_type, attack->no_block, attack->guardcost, attack->jugglecost, attack->pause_add, 1, current_attack_id);	//Execute on attacker.

			if(!lasthitc){	return;	}	//12312010, DC: Allows modder to cancel engine's attack handling. Useful for parry systems, alternate blocking, or other scripted hit events.

            didhit = 1;

            otherowner = self; // trace top owner for opponent
            while(otherowner->owner) otherowner = otherowner->owner;

            //if #01, if they are fired by the same owner, or the owner itself
            if(topowner == otherowner) didhit = 0;

            //if #02 , ground missle checking, and bullets wont hit each other
            if( (e->owner && self->owner) ||
                (e->modeldata.ground && inair(e))  )
            {
                didhit = 0;
            }//end of if #02

            //if #05,   blocking code section
            if(didhit)
            {
                if(attack->attack_type == ATK_ITEM){
                     didfind_item(e);
					 execute_didhit_script(e, self, force, attack->attack_drop, self->modeldata.subtype, attack->no_block, attack->guardcost, attack->jugglecost, attack->pause_add, 1);
                     return;
                }
                //if #051
                if(self->toexplode == 1) self->toexplode = 2;    // Used so the bomb type entity explodes when hit
                //if #052
                if(e->toexplode == 1) e->toexplode = 2;    // Used so the bomb type entity explodes when hitting

				if(inair(self)) self->modeldata.jugglepoints[0] = self->modeldata.jugglepoints[0] - attack->jugglecost; //reduce available juggle points.

                //if #053
                if( !self->modeldata.nopassiveblock && // cant block by itself
                    validanim(self,ANI_BLOCK) && // of course, move it here to avoid some useless checking
					((self->modeldata.guardpoints[1] == 0) || (self->modeldata.guardpoints[1] > 0 && self->modeldata.guardpoints[0] > 0)) &&
                    !(self->link ||
                    inair(self) ||
                    self->frozen ||
                    (self->direction == e->direction && self->modeldata.blockback < 1) ||                       // Can't block an attack that is from behind unless blockback flag is enabled
                    (!self->idling && self->attacking>=0)) &&                                                   // Can't block if busy, attack <0 means the character is preparing to attack, he can block during this time
                    attack->no_block <= self->modeldata.defense_blockpower[(short)attack->attack_type] &&       // If unblockable, will automatically hit
                    (rand32()&self->modeldata.blockodds) == 1 &&                                                // Randomly blocks depending on blockodds (1 : blockodds ratio)
                    (!self->modeldata.thold || (self->modeldata.thold > 0 && self->modeldata.thold > force))&&
                    (!fdefense_blockthreshold ||                                                                //Specific attack type threshold.
                    (fdefense_blockthreshold > force)))
                {   //execute the didhit script
                    execute_didhit_script(e, self, force, attack->attack_drop, attack->attack_type, attack->no_block, attack->guardcost, attack->jugglecost, attack->pause_add, 1);
                    set_blocking(self);
                    self->xdir = self->zdir = 0;
                    ent_set_anim(self, ANI_BLOCK, 0);
                    self->takeaction = common_block;
					execute_didblock_script(self, e, force, attack->attack_drop, attack->attack_type, attack->no_block, attack->guardcost, attack->jugglecost, attack->pause_add);
                    if(self->modeldata.guardpoints[1] > 0) self->modeldata.guardpoints[0] = self->modeldata.guardpoints[0] - attack->guardcost;
					++e->animation->animhits;
                    didblock = 1;    // Used for when playing the block.wav sound
                    // Spawn a flash
                    //if #0531
                    if(!attack->no_flash)
                    {
						if(!self->modeldata.noatflash)
						{
							if(attack->blockflash>=0) flash = spawn(lasthitx, lasthitz, lasthita, 0, NULL, attack->blockflash, NULL); // custom bflash
							else flash = spawn(lasthitx,lasthitz,lasthita, 0, NULL, ent_list[i]->modeldata.bflash, NULL);    // New block flash that can be smaller
						}
						else flash = spawn(lasthitx,lasthitz,lasthita, 0, NULL, self->modeldata.bflash, NULL);
                        //ent_default_init(flash); // initiliaze this because there're no default values now

                        if(flash) execute_onspawn_script(flash);
                    }
                    //end of if #0531
                }
                else if(self->modeldata.nopassiveblock && // can block by itself
                    self->blocking &&  // of course he must be blocking
					((self->modeldata.guardpoints[1] == 0) || (self->modeldata.guardpoints[1] > 0 && self->modeldata.guardpoints[0] > 0)) &&
                    !((self->direction == e->direction && self->modeldata.blockback < 1)|| self->frozen) &&    // Can't block if facing the wrong direction (unless blockback flag is enabled) or frozen in the block animation or opponent is a projectile
                    attack->no_block <= self->modeldata.defense_blockpower[(short)attack->attack_type] &&    // Make sure you are actually blocking and that the attack is blockable
                    (!self->modeldata.thold ||
                    (self->modeldata.thold > 0 &&
                    self->modeldata.thold > force))&&
                    (!self->modeldata.defense_blockthreshold[(short)attack->attack_type] ||                   //Specific attack type threshold.
                    (self->modeldata.defense_blockthreshold[(short)attack->attack_type] > force)))
                {    // Only block if the attack is less than the players threshold
                    //execute the didhit script
                    execute_didhit_script(e, self, force, attack->attack_drop, attack->attack_type, attack->no_block, attack->guardcost, attack->jugglecost, attack->pause_add, 1);
                    if(self->modeldata.guardpoints[1] > 0) self->modeldata.guardpoints[0] = self->modeldata.guardpoints[0] - attack->guardcost;
					++e->animation->animhits;
                    didblock = 1;    // Used for when playing the block.wav sound

                    if(self->modeldata.blockpain && self->modeldata.blockpain <= force && self->animation == self->modeldata.animation[ANI_BLOCK]) //Blockpain 1 and in block animation?
                    {
						set_blockpain(self, attack->attack_type, 0);
                    }
                    execute_didblock_script(self, e, force, attack->attack_drop, attack->attack_type, attack->no_block, attack->guardcost, attack->jugglecost, attack->pause_add);

                    // Spawn a flash
                    if(!attack->no_flash)
                    {
						if(!self->modeldata.noatflash)
						{
							if(attack->blockflash>=0) flash = spawn(lasthitx, lasthitz, lasthita, 0, NULL, attack->blockflash, NULL); // custom bflash
							else flash = spawn(lasthitx,lasthitz,lasthita, 0, NULL, ent_list[i]->modeldata.bflash, NULL);    // New block flash that can be smaller
						}
						else flash = spawn(lasthitx,lasthitz,lasthita, 0, NULL, self->modeldata.bflash, NULL);
                        //ent_default_init(flash); // initiliaze this because there're no default values now
                        if(flash) execute_onspawn_script(flash);
                    }
                }
                else if((self->animpos >= self->animation->counterframe[0] && self->animpos <= self->animation->counterframe[1])  &&	//Within counter range?
                    !self->frozen)// &&																								//Not frozen?
                    //(self->animation->counterframe[2] <= 1 && e->modeldata.type & them)) //&&												//Friend/foe?
                    //(self->animation->counterframe[2] <= 3 && !attack->no_block) &&														//Counter attack self couldn't block?
                    //self->animation->counterframe[2] <= 2 ||
					//self->animation->counterframe[2] <= 2 || !(self->direction == e->direction)) //&&										//Direction check.
                    //(self->animation->counterframe[2] <= 3 || !attack->freeze))															//Freeze attacks?

					//&& (!self->animation->counterframe[3] || self->health > force))													// Does damage matter?
                {
                    if(self->animation->counterframe[3]) self->health -= force;					// Take damage?
                    current_follow_id = animfollows[self->animation->followanim - 1];
                    if(validanim(self,current_follow_id))
                    {
                        if(self->modeldata.animation[current_follow_id]->attackone==-1)
                            self->modeldata.animation[current_follow_id]->attackone = self->animation->attackone;
                        ent_set_anim(self, current_follow_id, 0);
                        self->hit_by_attack_id = current_attack_id;
                    }

                    if(!attack->no_flash)
                    {
						if(!self->modeldata.noatflash)
						{
							if(attack->blockflash>=0) flash = spawn(lasthitx, lasthitz, lasthita, 0, NULL, attack->blockflash, NULL); // custom bflash
							else flash = spawn(lasthitx,lasthitz,lasthita, 0, NULL, ent_list[i]->modeldata.bflash, NULL);    // New block flash that can be smaller
						}
						else flash = spawn(lasthitx,lasthitz,lasthita, 0, NULL, self->modeldata.bflash, NULL);
                        //ent_default_init(flash); // initiliaze this because there're no default values now
                        if(flash) execute_onspawn_script(flash);
                    }
                }
                else if(self->takedamage(e, attack))
                {    // Didn't block so go ahead and take the damage
					//printf("*%d*", current_attack_id);
                    execute_didhit_script(e, self, force, attack->attack_drop, attack->attack_type, attack->no_block, attack->guardcost, attack->jugglecost, attack->pause_add, 0);
					++e->animation->animhits;

                    e->lasthit = self;

                    // Spawn a flash
                    if(!attack->no_flash)
                    {
                        if(!self->modeldata.noatflash)
                        {
                            if(attack->hitflash>=0) flash = spawn(lasthitx, lasthitz, lasthita, 0, NULL, attack->hitflash, NULL);
                            else flash = spawn(lasthitx, lasthitz, lasthita, 0, NULL, e->modeldata.flash, NULL);
                        }
                        else flash = spawn(lasthitx,lasthitz,lasthita, 0, NULL, self->modeldata.flash, NULL);
                        if(flash) execute_onspawn_script(flash);
                    }
                    topowner->combotime = time + (GAME_SPEED/2); // well, add to its owner's combo

                    if(e->animpos != e->lastanimpos || (inair(e) && !equalairpause))          // if equalairpause is set, inair(e) is nolonger a condition for extra pausetime
                    {    // Adds pause to the current animation
                        e->toss_time += attack->pause_add;      // So jump height pauses in midair
                        e->nextanim += attack->pause_add;       //Pause animation for a bit
                        e->nextthink += attack->pause_add;      // So anything that auto moves will pause
                    }

                    e->lastanimpos = e->animpos;

                    self->toss_time += attack->pause_add;       // So jump height pauses in midair
                    self->nextanim += attack->pause_add;        //Pause animation for a bit
                    self->nextthink += attack->pause_add;       // So anything that auto moves will pause

                }
                else
                {
                    didhit = 0;
                    continue;
                }
                // end of if #053

                // if #054
                if(flash && !attack->no_flash)
                {
                    if(flash->modeldata.toflip) flash->direction = (e->x > self->x);    // Now the flash will flip depending on which side the attacker is on

                    flash->base = lasthita;
                    flash->autokill = 1;
                }//end of if #054

                // 2007 3 24, hmm, def should be like this
                if(didblock && !def)  def = self;
                //if #055
                if((e->animation->followanim) &&                                        // follow up?
                    (e->animation->counterframe[0] == -1) &&                                // This isn't suppossed to be a counter, right?
                    ((e->animation->followcond < 2) || (self->modeldata.type & them)) &&    // Does type matter?
                    ((e->animation->followcond < 3) || ((self->health > 0) &&
                    !didblock)) &&                   // check if health or blocking matters
                    ((e->animation->followcond < 4) || cangrab(e,self))  )// check if nograb matters
                {
                    current_follow_id = animfollows[e->animation->followanim-1];
                    if(validanim(e,current_follow_id))
                    {
                        if(e->modeldata.animation[current_follow_id]->attackone==-1)
                            e->modeldata.animation[current_follow_id]->attackone = e->animation->attackone;
                        ent_set_anim(e, current_follow_id, 1);          // Then go to it!
                    }
                    followed = 1; // quit loop, animation is changed
                }//end of if #055

                self->hit_by_attack_id = current_attack_id;
                if(self==def) self->blocking = didblock; // yeah, if get hit, stop blocking
            }//end of if #05
            self = temp;
        }//end of if #0

    }//end of for


    // if ###
    if(didhit)
    {
        // well, dont check player or not - UTunnels. TODO: take care of that healthcheat
        if(e==topowner && current_anim->energycost[0] > 0 && nocost && !healthcheat) e->tocost = 1;    // Set flag so life is subtracted when animation is finished
        else if(e!=topowner && current_anim->energycost[0] > 0 && nocost && !healthcheat && !e->tocost) // if it is not top, then must be a shot
        {
            if(current_anim->energycost[1] != 2 && topowner->mp > 0)
            {
                topowner->mp -= current_anim->energycost[0];
                if(topowner->mp < 0) topowner->mp = 0;
            }
            else
            {
                topowner->health -= current_anim->energycost[0];
                if(topowner->health <= 0) topowner->health = 1;
            }

            topowner->cantfire = 0;    // Life subtracted, so go ahead and allow firing
            e->tocost = 1;    // Little backwards, but set to 1 so cost doesn't get subtracted multiple times
        }
        // New blocking checks
        //04/27/2008 Damon Caskey: Added checks for defense property specfic blockratio and type. Could probably use some cleaning.
        if(didblock)
        {
            if(blockratio || def->modeldata.defense_blockratio[(short)attack->attack_type]) // Is damage reduced?
            {
                if (def->modeldata.defense_blockratio[(short)attack->attack_type]){                      //Typed blockratio?
                    force = (int)(force * def->modeldata.defense_blockratio[(short)attack->attack_type]);
                }else{                                                                            //No typed. Use static block ratio.
                    force = force / 4;
                }

                if(mpblock && !def->modeldata.defense_blocktype[(short)attack->attack_type]){                                                                                 // Drain MP bar first?
                    def->mp -= force;
                    if(def->mp < 0)
                    {
                        force = -def->mp;
                        def->mp = 0;
                    }
                    else force = 0;                                                               // Damage removed from MP!
                }else if(def->modeldata.defense_blocktype[(short)attack->attack_type]==1){                //Damage from MP only for this attack type.
                    def->mp -= force;
                    if(def->mp < 0){
                        force = -def->mp;
                        def->mp = 0;
                    }
                    else force = 0;                                                               // Damage removed from MP!
                }else if(def->modeldata.defense_blocktype[(short)attack->attack_type]==2){              //Damage from both HP and MP at once.
                    def->mp -= force;
                }else if(def->modeldata.defense_blocktype[(short)attack->attack_type]==-1){             //Health only?
                    //Do nothing. This is so modders can overidde energycost[1] 1 with health only.
                }

                if(force < def->health)                    // If an attack won't deal damage, this line won't do anything anyway.
                    def->health -= force;
                else if(nochipdeath)                       // No chip deaths?
                    def->health = 1;
                else
                {
                    temp = self;
                    self = def;
                    self->takedamage(e, attack);           // Must be a fatal attack, then!
                    self = temp;
                }
            }
        }

        if(!didblock)
        {
            topowner->rushtime = time + (GAME_SPEED*rush[1]);
            topowner->rush[0]++;
            if(topowner->rush[0] > topowner->rush[1] && topowner->rush[0] > 1) topowner->rush[1] = topowner->rush[0];
        }

        if(didblock)
		{
			if(attack->blocksound >= 0) sound_play_sample(attack->blocksound, 0, savedata.effectvol,savedata.effectvol, 100); // New custom block sound effect
			else if(SAMPLE_BLOCK >= 0) sound_play_sample(SAMPLE_BLOCK, 0, savedata.effectvol,savedata.effectvol, 100);    // Default block sound effect
		}
        else if(e->projectile > 0 && SAMPLE_INDIRECT >= 0) sound_play_sample(SAMPLE_INDIRECT, 0, savedata.effectvol,savedata.effectvol, 100);
        else
        {
            if(noslowfx)
            {
                if(attack->hitsound >= 0) sound_play_sample(attack->hitsound, 0, savedata.effectvol,savedata.effectvol, 100);
                else if(SAMPLE_BEAT >= 0) sound_play_sample(SAMPLE_BEAT, 0, savedata.effectvol,savedata.effectvol, 100);
            }
            else
            {
                if(attack->hitsound >= 0) sound_play_sample(attack->hitsound, 0, savedata.effectvol,savedata.effectvol, 105 - force);
                else if(SAMPLE_BEAT >= 0) sound_play_sample(SAMPLE_BEAT, 0, savedata.effectvol,savedata.effectvol, 105 - force);
            }
        }

        if(e->remove_on_attack) kill(e);
    }//end of if ###
}


void check_gravity()
{
	int heightvar;
    entity* other, *dust;
    s_attack attack;

    if(!is_frozen(self) )// Incase an entity is in the air, don't update animations
    {
        if((self->falling || self->tossv || self->a!=self->base) && self->toss_time <= time && !self->animation->dive[0] && !self->animation->dive[1])
        {
            if(self->modeldata.subject_to_platform>0 && self->tossv>0)
                other = check_platform_above(self->x, self->z, self->a+self->tossv);
            else other = NULL;

            if(self->animation->height) heightvar = self->animation->height;
            else heightvar = self->modeldata.height;

            if( other && other->a<=self->a+heightvar)
            {
                if(self->hithead == NULL) // bang! Hit the ceiling.
                {
                    self->tossv = 0;
                    self->hithead = other;
                    execute_onblocka_script(self, other);
                }
            }
            else self->hithead = NULL;
            // gravity, antigravity factors
            self->a += self->tossv;
            if(self->modeldata.subject_to_gravity>0)
                self->tossv += level->gravity * (1.0-self->modeldata.antigravity-self->antigravity);

            if(self->tossv < level->maxfallspeed)
            {
                self->tossv = level->maxfallspeed;
            }
            else if(self->tossv > level->maxtossspeed)
            {
                self->tossv = level->maxtossspeed;
            }
            if(self->animation->dropframe>=0 && self->tossv<=0 && self->animpos<self->animation->dropframe) // begin dropping
            {
                update_frame(self, self->animation->dropframe);
            }
            if (self->tossv) execute_onmovea_script(self); //Move A event.

            // UTunnels: tossv <= 0 means land, while >0 means still rising, so
            // you wont be stopped if you are passing the edge of a wall
            if( (self->a<=self->base || !inair(self)) && self->tossv <= 0)
            {
                self->a = self->base;
                self->falling = 0;
                //self->projectile = 0;
                // cust dust entity
                if(self->modeldata.dust[0]>=0 && self->tossv < -1 && self->drop)
                {
                    dust = spawn(self->x, self->z, self->a, self->direction, NULL, self->modeldata.dust[0], NULL);
                    dust->base = self->a;
                    dust->autokill = 1;
                    execute_onspawn_script(dust);
                }
                // bounce/quake
                if(tobounce(self) && self->modeldata.bounce)
                {
					int i;
					self->xdir /= self->animation->bounce;
					self->zdir /= self->animation->bounce;
					toss(self, (-self->tossv)/self->animation->bounce);
					if(!self->modeldata.noquake) quake = 4;    // Don't shake if specified
					if(SAMPLE_FALL >= 0) sound_play_sample(SAMPLE_FALL, 0, savedata.effectvol,savedata.effectvol, 100);
					if(self->modeldata.type == TYPE_PLAYER) control_rumble(self->playerindex, 100*(int)self->tossv/2);
					for(i=0; i<MAX_PLAYERS; i++) control_rumble(i, 75*(int)self->tossv/2);
                }
                else if((!self->animation->seta || self->animation->seta[self->animpos]<0) &&
					(!self->animation->movea || self->animation->movea[self->animpos]<=0))
						self->xdir = self->zdir = self->tossv= 0;
                else self->tossv = 0;

                if(self->animation->landframe[0]>=0 && self->animpos < self->animation->landframe[0])
                {
                    update_frame(self, self->animation->landframe[0]);
					if(self->animation->landframe[1]>=0)
					{
						dust = spawn(self->x, self->z, self->a, self->direction, NULL, self->animation->landframe[1], NULL);
						dust->base = self->a;
						dust->autokill = 1;
						execute_onspawn_script(dust);
					}
                }

                // takedamage if thrown or basted
                if(self->damage_on_landing > 0 && !self->dead)
                {
                    if(self->takedamage)

                    {
                        attack              = emptyattack;
                        attack.attack_force = self->damage_on_landing;
                        attack.attack_type  = ATK_NORMAL;
                        self->takedamage(self, &attack);
                    }
                    else
                    {
                        self->health -= self->damage_on_landing;
                        if(self->health <=0 ) kill(self);
                        self->damage_on_landing = 0;
                    }
                }
                // in case landing, set hithead to NULL
                self->hithead = NULL;
            }// end of if - land checking
            self->toss_time = time + (GAME_SPEED/100);
        }// end of if  - in-air checking

    }//end of if
}

void check_lost()
{
    s_attack attack;
    int osk = self->modeldata.offscreenkill?self->modeldata.offscreenkill:1000;

    if((self->z!=100000 && (advancex - self->x>osk || self->x - advancex - videomodes.hRes>osk ||
		(level->scrolldir!=SCROLL_UP && level->scrolldir!=SCROLL_DOWN && (advancey - self->z>osk || self->z - advancey - videomodes.vRes>osk)) ||
		((level->scrolldir==SCROLL_UP || level->scrolldir==SCROLL_DOWN) && (self->z<-osk || self->z>videomodes.vRes + osk))		) )
        || self->a < 2*PIT_DEPTH) //self->z<100000, so weapon item won't be killed
    {
        if(self->modeldata.type==TYPE_PLAYER) player_die();
        else kill(self);
        return;
    }
    // fall in to a pit
    if(self->a < PIT_DEPTH || self->lifespancountdown<0)
    {
        if(!self->takedamage) kill(self);
        else
        {
            attack              = emptyattack;
            attack.dropv[0]     = (float)3; attack.dropv[1] = (float)1.2; attack.dropv[2] = (float)0;
            attack.attack_force = self->health;
            attack.attack_type  = max_attack_types;
            self->takedamage(self, &attack);
        }
        return;
    }//else
    // Doom count down
    if(!is_frozen(self) && self->lifespancountdown != (float)0xFFFFFFFF) self->lifespancountdown--;
}

// grab walk check
void check_link_move(float xdir, float zdir)
{
    float x, z, gx, gz;
    int tryresult;
    entity* tempself = self;
    gx = self->grabbing->x; gz = self->grabbing->z;
    x = self->x; z = self->z;
    self = self->grabbing;
    tryresult = self->trymove(xdir, zdir);
    self = tempself;
    if(tryresult!=1) // changed
    {
        xdir = self->grabbing->x - gx;
        zdir = self->grabbing->z - gz;
    }
    tryresult = self->trymove(xdir, zdir);
    if(tryresult != 1)
    {
        self->grabbing->x = self->x - x + gx;
        self->grabbing->z = self->z - z + gz;
    }
}

void check_ai()
{
    entity* plat;
    // check moving platform
    if((plat=self->landed_on_platform) &&
        (plat->xdir || plat->zdir) &&
        (plat->nextthink <= time || (plat->update_mark & 2)) &&// plat is updated before self or will be updated this loop
        testplatform(plat,self->x,self->z) &&
        self->a <= plat->a + plat->animation->platform[plat->animpos][7] + 0.5 ) // on the platform?
    {
        // passive move with the platform
        if(self->trymove )
        {
            // grab walk check
            if(self->grabbing && self->grabwalking && self->grabbing->trymove)
            {
                check_link_move(plat->xdir, plat->zdir);
            }
            else self->trymove(plat->xdir, plat->zdir);
        }
        else
        {
            self->x += plat->xdir;
            self->z += plat->zdir;
        }
    }

    if(self->nextthink <= time && !endgame)
    {
        self->update_mark |= 2; //mark it
        // take actions
        if(self->takeaction) self->takeaction();

        // A.I. think
        if(self->think)
        {
            if(self->nextthink<=time) self->nextthink = time + THINK_SPEED;
			// use noaicontrol flag to turn of A.I. think
            if(!self->noaicontrol) self->think();
        }

		// Execute think script
		execute_think_script(self);

        // A.I. move
        if (self->xdir || self->zdir)
        {
            if(self->trymove)
            {
                // grab walk check
                if(self->grabbing && self->grabwalking && self->grabbing->trymove)
                {
                    check_link_move(self->xdir, self->zdir);
                }
                else if(self->trymove(self->xdir, self->zdir)!=1 && self->idling)
                {
                    self->pathblocked++; // for those who walk against wall or borders
                }
            }
            else
            {
                self->x += self->xdir;
                self->z += self->zdir;
            }
        }
        // Used so all entities can have a spawn animation, and then just changes to the idle animation when done
        // move here to so players wont get stuck
        if((self->animation == self->modeldata.animation[ANI_SPAWN] || self->animation == self->modeldata.animation[ANI_RESPAWN]) && !self->animating /*&& (!inair(self)||!self->modeldata.subject_to_gravity)*/) set_idle(self);
    }
}


void update_animation()
{
    int f, wall, hole;
    float move, movez, seta;
    entity *other = NULL;

    if(level)
    {
        if(self->modeldata.facing == 1 || level->facing == 1) self->direction = 1;
        else if(self->modeldata.facing == 2 || level->facing == 2) self->direction = 0;
        else if((self->modeldata.facing == 3 || level->facing == 3) && (level->scrolldir & SCROLL_RIGHT)) self->direction = 1;
		else if((self->modeldata.facing == 3 || level->facing == 3) && (level->scrolldir & SCROLL_LEFT)) self->direction = 0;
        if(self->modeldata.type == TYPE_PANEL)
        {
            self->x += scrolldx * ((float)(self->modeldata.speed));
            if(level->scrolldir==SCROLL_UP)
            {
                self->a += scrolldy * ((float)(self->modeldata.speed));
            }
            else if(level->scrolldir==SCROLL_DOWN)
            {
                self->a -= scrolldy * ((float)(self->modeldata.speed));
            }
            else
            {
                self->a -= scrolldy * ((float)(self->modeldata.speed));
            }
        }
        if(self->modeldata.scroll)
        {
            self->x += scrolldx * ((float)(self->modeldata.scroll));
            if(level->scrolldir==SCROLL_UP)
            {
                self->a += scrolldy * ((float)(self->modeldata.scroll));
            }
            else if(level->scrolldir==SCROLL_DOWN)
            {
                self->a -= scrolldy * ((float)(self->modeldata.scroll));
            }
            else
            {
                self->a -= scrolldy * ((float)(self->modeldata.scroll));
            }
        }
    }

    if(self->invincible && time >= self->invinctime)    // Invincible time has run out, turn off
    {
        self->invincible    = 0;
        self->blink         = 0;
        self->invinctime    = 0;
        self->arrowon       = 0;
    }

    if(self->dying)    // Code for doing dying flash
    {
        if((self->health <= self->per1 && self->health > self->per2 && (time %(GAME_SPEED / 10)) < (GAME_SPEED / 40)) ||
            (self->health <= self->per2))
        {
            if(self->colourmap == self->modeldata.colourmap[self->dying - 1] || self->health <= 0 )
            {
                ent_set_colourmap(self, self->map);
            }
            else
            {
                self->colourmap = self->modeldata.colourmap[self->dying - 1];
            }
        }
    }

    if(self->freezetime && time >= self->freezetime)
    {
        unfrozen(self);
    }

    if(self->maptime && time >= self->maptime)
    {
        ent_set_colourmap(self, self->map);
    }

    if(self->sealtime && time >= self->sealtime) //Remove seal, special moves are available again.
    {
        self->seal = 0;
    }

    if(self->nextanim == time ||
        (self->modeldata.type == TYPE_TEXTBOX && self->modeldata.subtype != SUBTYPE_NOSKIP &&
         (bothnewkeys&(FLAG_JUMP|FLAG_ATTACK|FLAG_ATTACK2|FLAG_ATTACK3|FLAG_ATTACK4|FLAG_SPECIAL))))// Textbox will autoupdate if a valid player presses an action button
    {    // Now you can display text and cycle through with any jump/attack/special unless SUBTYPE_NOSKIP

        f = self->animpos + self->animating;

        //Specified loop break frame.
        if(self->animation->loop[0] && self->animation->loop[2])
        {
            if (f == self->animation->loop[2])
            {
                if(f<0) f = self->animation->numframes-1;
                else f = 0;

                if (self->animation->loop[1])
                {
                    f = self->animation->loop[1];
                }
            }
            else if((unsigned)f >= (unsigned)self->animation->numframes)
            {
                self->animating = 0;

                if(self->autokill)
                {
                    kill(self);
                    return;
                }
            }
        }
        else if((unsigned)f >= (unsigned)self->animation->numframes)
        {
            if(f<0) f = self->animation->numframes-1;
            else f = 0;

            if(!self->animation->loop[0])
            {
                self->animating = 0;

                if(self->autokill)
                {
                    kill(self);
                    return;
                }
            }
            else
            {
                if (self->animation->loop[1])
                {
                    f = self->animation->loop[1];
                }
            }
        }

        if(self->animating)
        {
            //self->nextanim = time + (self->animation->delay[f]);
            self->update_mark |= 1; // frame updated, mark it
            // just switch frame to f, if frozen, expand_time will deal with it well
            update_frame(self, f);
        }
    }

	if(self->modeldata.subject_to_platform>0 /*&& self->projectile==0*/)
	{
        other = self->landed_on_platform;
        if(other && testplatform(other, self->x, self->z) && self->a < other->a + other->animation->platform[other->animpos][7])
        {
            self->a = self->base = other->a + other->animation->platform[other->animpos][7]+0.5;
        }
        else other = check_platform_below(self->x, self->z, self->a+1);
	}
    else other = NULL;
    self->landed_on_platform = other;
    // adjust base
    if(self->modeldata.no_adjust_base<=0)
    {
        seta = (float)((self->animation->seta)?(self->animation->seta[self->animpos]):(-1));

        // Checks to see if entity is over a wall and or obstacle, and adjusts the base accordingly
        //wall = checkwall_below(self->x, self->z);
        //find a wall below us
        if(self->modeldata.subject_to_wall>0)
            wall = checkwall_below(self->x, self->z, self->a);
        else wall = -1;

        if(self->modeldata.subject_to_hole>0)
        {
            hole = (wall<0&&!other)?checkhole(self->x, self->z):0;

            if(seta<0 && hole)
            {
                self->base=-1000;
                ent_unlink(self);
            }
            else if(!hole && self->base == -1000)
            {
                 if(self->a>=0) self->base = 0;
                 else
                 {
                     self->xdir = self->zdir = 0; // hit the hole border
                 }
            }
        }

        if(self->base != -1000 || wall>=0)
        {
            if(other != NULL && other != self )
            {
                self->base = (seta + self->altbase >=0 ) * (seta+self->altbase) + (other->a + other->animation->platform[other->animpos][7]);
            }
            else if(wall >= 0)
            {
    			//self->modeldata.subject_to_wall &&//we move this up to avoid some checking time
                self->base = (seta + self->altbase >=0 ) * (seta+self->altbase) + (self->a >= level->walls[wall][7]) * level->walls[wall][7];
            }
            else if(seta >= 0) self->base = (seta + self->altbase >=0 ) * (seta+self->altbase);
            else if(self->animation != self->modeldata.animation[ANI_VAULT] && (!self->animation->movea || self->animation->movea[self->animpos] == 0))
            {
    			// Don't want to adjust the base if vaulting
    			// No obstacle/wall or seta, so just set to 0
                self->base = 0;
            }
        }
    }

    // Code for when entities move (useful for moving platforms, etc)
    if(other && other != self )
    {
        // a bit complex, other->nextanim == time means other is behind self and not been updated,
        // update_mark & 1 means other is updated in this loop and before self
        if((other->nextanim == time || (other->update_mark & 1)) && self->a <= other->a + other->animation->platform[other->animpos][7]+0.5)
        {
            if(other->update_mark & 1) f = other->animpos;
            else f = other->animpos + other->animating;
            if(f >= other->animation->numframes)
            {
                if(f<0) f = other->animation->numframes-1;
                else f = 0;
            }
            //printf("%d %d %d\n", other->nextanim, time, other->update_mark);
            move = (float)(other->animation->move?other->animation->move[f]:0);
            movez = (float)(other->animation->movez?other->animation->movez[f]:0);
            if(other->direction==0) move = -move;
            if(move||movez)
            {
                if(self->trymove)
                {
                    self->trymove(move, movez);
                }
                else
                {
                    self->z += movez;
                    self->x += move;
                }
            }
        }
    }
}

void check_attack()
{
    // a normal fall
    if(self->falling && !self->projectile)
    {
        self->attack_id = 0;
        return;
    }
    // on ground
    if(self->drop && !self->falling)
    {
        self->attack_id= 0;
        return;
    }

    // Can't hit an opponent if you are frozen
    if(!is_frozen(self) && self->animation->attacks &&
        self->animation->attacks[self->animpos])
    {
        do_attack(self);
        return;
    }
    self->attack_id = 0;
}


void update_health()
{
	//12/30/2008: Guardrate by OX. Guardpoints increase over time.
	if(self->modeldata.guardpoints[1] > 0 && time >= self->guardtime) // If this is > 0 then guardpoints are set..
	{
		if(self->blocking)
		{
			self->modeldata.guardpoints[0] += (self->modeldata.guardrate/2);
			if(self->modeldata.guardpoints[0] > self->modeldata.guardpoints[1]) self->modeldata.guardpoints[0] = self->modeldata.guardpoints[1];
		}
		else
		{
			self->modeldata.guardpoints[0] += self->modeldata.guardrate;
			if(self->modeldata.guardpoints[0] > self->modeldata.guardpoints[1]) self->modeldata.guardpoints[0] = self->modeldata.guardpoints[1];
		}
		self->guardtime = time + GAME_SPEED;    //Reset guardtime.
	}

    common_dot();   //Damage over time.

    // this is for restoring mp by time by tails
    // Cleaning and addition of mpstable by DC, 08172008.
	// stabletype 4 added by OX 12272008
    if(magic_type == 0 && !self->charging)
    {
        if(time >= self->magictime)
        {

            // 1 Only recover MP > mpstableval.
            // 2 No recover. Drop MP if MP < mpstableval.
            // 3 Both: recover if MP if MP < mpstableval and drop if MP > mpstableval.
            // 0 Default. Recover MP at all times.


            if (self->modeldata.mpstable == 1){
                if (self->mp < self->modeldata.mpstableval) self->mp += self->modeldata.mprate;
            }else if(self->modeldata.mpstable == 2){
                if (self->mp > self->modeldata.mpstableval) self->mp -= self->modeldata.mpdroprate;
            }else if (self->modeldata.mpstable == 3){
                if (self->mp < self->modeldata.mpstableval)
                {

                    self->mp += self->modeldata.mprate;
                }
                else if (self->mp > self->modeldata.mpstableval)
                {
                    self->mp -= self->modeldata.mpdroprate;
                }
            }

			// OX. Stabletype 4. Gain mp until it reaches max. Then it drops down to mpstableval.
			else if (self->modeldata.mpstable == 4)
			{
				if(self->mp <= self->modeldata.mpstableval) self->modeldata.mpswitch = 0;
				else if(self->mp == self->modeldata.mp) self->modeldata.mpswitch = 1;

				if(self->modeldata.mpswitch == 1)
				{
					self->mp -= self->modeldata.mpdroprate;
				}
				else if(self->modeldata.mpswitch == 0)
				{
					self->mp += self->modeldata.mprate;
				}
			}
            else
            {
                self->mp += self->modeldata.mprate;
            }

            self->magictime = time + GAME_SPEED;    //Reset magictime.
        }
    }
    if(self->charging && time >= self->mpchargetime)
    {
        self->mp += self->modeldata.chargerate;
        self->mpchargetime = time + (GAME_SPEED / 4);
    }
    if(self->mp > self->modeldata.mp) self->mp = self->modeldata.mp; // Don't want to add more than the max

    if(self->oldhealth < self->health) self->oldhealth++;
    else if(self->oldhealth > self->health) self->oldhealth--;

    if(self->oldmp < self->mp) self->oldmp++;
    else if(self->oldmp > self->mp) self->oldmp--;
}

void common_dot()
{
    //common_dot
    //Damon V. Caskey
    //06172009
    //Mitigates damage over time (dot). Moved here from update_health().

    int         iFForce;    //Final force; total damage after defense and offense factors are applied.
    int         iType;      //Attack type.
    int         iIndex;     //Dot index.
    int         iDot;       //Dot mode.
    int         iDot_time;  //Dot expire time.
    int         iDot_cnt;   //Dot next tick time.
    int         iDot_rate;  //Dot tick rate.
    int         iForce;     //Unmodified force.
    float       fOffense;   //Owner's offense.
    float       fDefense;   //Self defense.
    entity*     eOpp;       //Owner of dot effect.
    s_attack    attack;     //Attack struct.

    for(iIndex=0; iIndex<=MAX_DOTS; iIndex++)                                                   //Loop through all DOT indexes.
    {
        iDot_time   =   self->dot_time[iIndex];                                                 //Get expire time.
        iDot_cnt    =   self->dot_cnt[iIndex];                                                  //Get next tick time.
        iDot_rate   =   self->dot_rate[iIndex];                                                 //Get tick rate.

        if(iDot_time)                                                                           //Dot time present?
        {
            if(time > iDot_time)                                                                //Dot effect expired? Then clear variants.
            {
                self->dot[iIndex]       = 0;
                self->dot_atk[iIndex]   = 0;
                self->dot_cnt[iIndex]   = 0;
                self->dot_rate[iIndex]  = 0;
                self->dot_time[iIndex]  = 0;
                self->dot_force[iIndex] = 0;
            }
            else if(time >= iDot_cnt && self->health>=0)                                        //Time for a dot tick and alive?
            {
                self->dot_cnt[iIndex] = time + (iDot_rate * GAME_SPEED / 100);                  //Reset next tick time.

                iDot    =   self->dot[iIndex];                                                  //Get dot mode.
                iForce  =   self->dot_force[iIndex];                                            //Get dot force.

                if(iDot==1 || iDot==3 || iDot==4 || iDot==5)                                    //HP?
                {
                    eOpp        = self->dot_owner[iIndex];                                      //Get dot effect owner.
                    iType       = self->dot_atk[iIndex];                                        //Get attack type.
                    iFForce     = iForce;                                                       //Initialize final force.
                    fOffense    = eOpp->modeldata.offense_factors[iType];                       //Get owner's offense.
                    fDefense    = self->modeldata.defense_factors[iType];                       //Get Self defense.

                    if (fOffense){  iFForce = (int)(iForce  * fOffense);    }                   //Apply offense factors.
                    if (fDefense){  iFForce = (int)(iFForce * fDefense);    }                   //Apply defense factors.

                    if(iFForce >= self->health && (iDot==4 || iDot==5))                         //Total force lethal?
                    {
                        attack              = emptyattack;                                      //Clear struct.
                        attack.attack_type  = iType;                                            //Set type.
                        attack.attack_force = iForce;                                           //Set force. Use unmodified force here; takedamage applys damage mitigation.
                        attack.dropv[0]     = (float)3;                                         //Apply drop Y.
                        attack.dropv[1]     = (float)1.2;                                       //Apply drop X
                        attack.dropv[2]     = (float)0;                                         //Apply drop Z

                        if(self->takedamage)                                                    //Defender uses takedamage()?
                        {
                            self->takedamage(eOpp, &attack);                                    //Apply attack to kill defender.
                        }
                        else
                        {
                            kill(self);                                                         //Kill defender instantly.
                        }
                    }
                    else                                                                        //Total force less then health or using non lethal setting.
                    {
                        if (self->health > iFForce)                                             //Final force less then health?
                        {
                            self->health -= iFForce;                                            //Reduce health directly. Using takedamage() breaks grabs and spams defender's status in HUD.
                        }
                        else
                        {
                            self->health = 1;                                                   //Set minimum health.
                        }
                        execute_takedamage_script(self, eOpp, iForce, 0, iType, 0, 0, 0, 0);    //Execute the takedamage script.
                    }
                }

                if(iDot==2 || iDot==3 || iDot==5)                                               //MP?
                {
                    self->mp -= iForce;                                                         //Subtract force from MP.
                    if(self->mp<0) self->mp = 0;                                                //Stablize MP at 0.
                }
            }
        }
    }
}

void adjust_bind(entity* e)
{
    if(e->bindanim)
    {
        if(e->animnum!=e->bound->animnum)
        {
            if(!validanim(e,e->bound->animnum))
            {
                if(e->bindanim&4)
                {
                   kill(e);
                }
                e->bound=NULL;
                return;
            }
            ent_set_anim(e, e->bound->animnum, 1);
        }
        if(e->animpos!=e->bound->animpos && e->bindanim&2)
        {
            update_frame(e, e->bound->animpos);
        }
    }
    e->z = e->bound->z +e->bindoffset[1];
    e->a = e->bound->a + e->bindoffset[2];
    switch(e->bindoffset[3])
    {
    case 0:
        if(e->bound->direction) e->x = e->bound->x + e->bindoffset[0];
        else e->x = e->bound->x - e->bindoffset[0];
        break;
    case 1:
        e->direction = e->bound->direction;
        if(e->bound->direction) e->x = e->bound->x + e->bindoffset[0];
        else e->x = e->bound->x - e->bindoffset[0];
        break;
    case -1:
        e->direction = !e->bound->direction;
        if(e->bound->direction) e->x = e->bound->x + e->bindoffset[0];
        else e->x = e->bound->x - e->bindoffset[0];
        break;
    case 2:
        e->direction = 1;
        e->x = e->bound->x + e->bindoffset[0];
        break;
    case -2:
        e->direction = 0;
        e->x = e->bound->x + e->bindoffset[0];
        break;
    default:
        e->x = e->bound->x + e->bindoffset[0];
        break;
    // the default is no change :), just give a value of 12345 or so
    }
}

// arrenge the list reduce its length
void arrange_ents()
{
    int i, ind=-1;
    entity* temp;
    if(ent_count == 0) return;
    if(ent_max == ent_count)
    {
        for(i=0; i<ent_max; i++)
        {
            ent_list[i]->update_mark = 0;
            if(ent_list[i]->exists && ent_list[i]->bound)
            {
                adjust_bind(ent_list[i]);
            }
        }
    }
    else
    {
        for(i=0; i<ent_max; i++)
        {
            if(!ent_list[i]->exists && ind<0)
                ind = i;
            else if(ent_list[i]->exists && ind>=0)
            {
                temp = ent_list[i];
                ent_list[i] = ent_list[ind];
                ent_list[ind] = temp;
                ind++;
            }
            ent_list[i]->update_mark = 0;
            if(ent_list[i]->exists && ent_list[i]->bound)
            {
                adjust_bind(ent_list[i]);
            }
        }
        ent_max = ent_count;
    }
}

// Update all entities that wish to think or animate in this cycle.
// All loops are separated because "self" might die during a pass.
void update_ents()
{
    int i;
    for(i=0; i<ent_max; i++)
    {
        if(ent_list[i]->exists && time != ent_list[i]->timestamp)// dont update fresh entity
        {
            self = ent_list[i];
            self->update_mark = 0;
            if(level) check_lost();// check lost caused by level scrolling or lifespan
            if(!self->exists) continue;
            // expand time incase being frozen
            if(is_frozen(self)){expand_time(self);}
            else
            {
                execute_updateentity_script(self);// execute a script
                if(!self->exists) continue;
                check_ai();// check ai
                if(!self->exists) continue;
                check_gravity();// check gravity
                if(!self->exists) continue;
                update_animation(); // if not frozen, update animation
                if(!self->exists) continue;
                check_attack();// Collission detection
                if(!self->exists) continue;
                update_health();// Update displayed health
            }
        }
    }//end of for
    arrange_ents();
}


void display_ents()
{
    unsigned f;
    int i, z, wall = 0, wall2;
    entity *e = NULL;
    entity *other = NULL;
    int qx, qy, sy,sz, alty;
    float temp1, temp2;
    int useshadow = 0;
    int can_mirror = 0;
    s_drawmethod* drawmethod = NULL;
    s_drawmethod commonmethod;
    s_drawmethod shadowmethod;
    int use_mirror = (level && level->mirror);

    for(i=0; i<MAX_ENTS; i++)
    {
        if(ent_list[i] && ent_list[i]->exists)
        {
            e = ent_list[i];
            if(e->modeldata.hpbarstatus.sizex)
            {
                drawenemystatus(e);

            }
            if(freezeall || !(e->blink && (time%(GAME_SPEED/10))<(GAME_SPEED/20)))
            {    // If special is being executed, display all entities regardless
                f = e->animation->sprite[e->animpos];

                other = check_platform(e->x, e->z);
                wall = checkwall(e->x, e->z);

                if(f<sprites_loaded)
                {
                    // var "z" takes into account whether it has a setlayer set, whether there are other entities on
                    // the same "z", in which case there is a layer offset, whether the entity is on an obstacle, and
                    // whether the entity is grabbing someone and has grabback set

                    z = (int)e->z;    // Set the layer offset

                    if(e->grabbing && e->modeldata.grabback)
                        z = (int)e->link->z - 1;    // Grab animation displayed behind
                    else if(!e->modeldata.grabback && e->grabbing)
                        z = (int)e->link->z + 1;

                    if(e->bound && e->bound->grabbing)
                    {
                        if(e->bound->modeldata.grabback) z--;
                        else                             z++;
                    }

                    if(other && other != e && e->a >= other->a + other->animation->platform[other->animpos][7])
                    {
                        if(
                            e->link &&
                            ((e->modeldata.grabback &&
                            !e->grabbing) ||
                            (e->link->modeldata.grabback &&
                            e->link->grabbing) ||
                            e->grabbing)
                            )
                            z = (int)(other->z + 2);    // Make sure entities get displayed in front of obstacle and grabbee

                        else z = (int)(other->z + 1);    // Entity should always display in front of the obstacle

                    }

                    if(e->owner) z = (int)(e->z/* + e->layer*/ + 1);    // Always in front

                    if(e->modeldata.setlayer) z = HOLE_Z + e->modeldata.setlayer;    // Setlayer takes precedence

                    if(checkhole(e->x, e->z)==2) z = PANEL_Z-1;        // place behind panels

                    drawmethod = e->animation->drawmethods?e->animation->drawmethods[e->animpos]:NULL;
                    if(e->drawmethod.flag) drawmethod = &(e->drawmethod);
                    if(!drawmethod)
                        commonmethod = plainmethod;
                    else
                        commonmethod = *drawmethod;
                    drawmethod = &commonmethod;

                    if(drawmethod->remap>=1 && drawmethod->remap<=e->modeldata.maps_loaded)
                    {
                        drawmethod->table = e->modeldata.colourmap[drawmethod->remap-1];
                    }

                    if(e->colourmap)
                    {
                        if(drawmethod->remap<0) drawmethod->table = e->colourmap;
                    }
                    if(e->modeldata.alpha >=1 && e->modeldata.alpha <= MAX_BLENDINGS)
                    {
                        if(drawmethod->alpha<0)
                        {
                            drawmethod->alpha = e->modeldata.alpha;
                        }
                    }
                    if(!drawmethod->table) drawmethod->table = e->modeldata.palette;
                    if(e->modeldata.globalmap)
                    {
                        if(level&&current_palette)
                            drawmethod->table = level->palettes[current_palette-1];
                        else drawmethod->table = pal;
                    }

                    if(!e->direction)
                    {
                        drawmethod->flipx = !drawmethod->flipx;
                        if(drawmethod->fliprotate && drawmethod->rotate) drawmethod->rotate = 360-drawmethod->rotate;
                    }

                    if(!use_mirror || z > MIRROR_Z) // don't display if behind the mirror
                    {
                        spriteq_add_sprite((int)(e->x - advancex), (int)(e->z-e->a + gfx_y_offset), z, f, drawmethod, e->bound?e->bound->sortid-1:e->sortid);
                    }

                    can_mirror = (use_mirror && self->z>MIRROR_Z);
                    if(can_mirror)
                    {
                        spriteq_add_sprite((int)(e->x-advancex), (int)((2*MIRROR_Z - e->z)-e->a+gfx_y_offset), 2*PANEL_Z - z , f, drawmethod, MAX_ENTS - (e->bound?e->bound->sortid-1:e->sortid));
                    }
                }//end of if(f<sprites_loaded)

                if(e->modeldata.gfxshadow==1 && f<sprites_loaded)//gfx shadow
                {
                    useshadow = (e->animation->shadow?e->animation->shadow[e->animpos]:1) && shadowcolor && light[1];
                    //printf("\n %d, %d, %d\n", shadowcolor, light[0], light[1]);
                    if(useshadow && e->a>=0 && (!e->modeldata.aironly || (e->modeldata.aironly && inair(e))))
                    {
                        wall = checkwall_below(e->x, e->z, e->a);
                        if(wall<0)
                        {
                            alty = (int)e->a;
                            temp1 = -1*e->a*light[0]/256; // xshift
                            temp2 = (float)(-alty*light[1]/256);                   // zshift
                            qx = (int)(e->x - advancex/* + temp1*/);
                            qy = (int)(e->z + gfx_y_offset/* +  temp2*/);
                        }
                        else
                        {
                            alty = (int)(e->a-level->walls[wall][7]);
                            temp1 = -1*(e->a-level->walls[wall][7])*light[0]/256; // xshift
                            temp2 = (float)(-alty*light[1]/256);                   // zshift
                            qx = (int)(e->x - advancex/* + temp1*/);
                            qy = (int)(e->z + gfx_y_offset /*+  temp2*/ - level->walls[wall][7]);
                        }

                        wall2=checkwall_below(e->x + temp1, e->z + temp2, e->a); // check if the shadow drop into a hole or fall on another wall
                        if(!(checkhole(e->x + temp1, e->z + temp2) && wall2<0) )//&& !(wall>=0 && level->walls[wall][7]>e->a))
                        {
                            if(wall>=0 && wall2 >= 0)
                            {
                               alty += (int)(level->walls[wall][7] - level->walls[wall2][7]);
                               /*qx += -1*(level->walls[wall][7]-level->walls[wall2][7])*light[0]/256;
                               qy += (level->walls[wall][7]-level->walls[wall2][7]) - (level->walls[wall][7]-level->walls[wall2][7])*light[1]/256;*/
                            }
                            else if(wall>=0)
                            {
                               alty += (int)(level->walls[wall][7]);
                               /*qx += -1*level->walls[wall][7]*light[0]/256;
                               qy += level->walls[wall][7] - level->walls[wall][7]*light[1]/256;*/
                            }
                            else if(wall2>=0)
                            {
                               alty -= (int)(level->walls[wall2][7]);
                               /*qx -= -1*level->walls[wall2][7]*light[0]/256;
                               qy -= level->walls[wall2][7] - level->walls[wall2][7]*light[1]/256;*/
                            }
                            sy = (2*MIRROR_Z - qy) + 2*gfx_y_offset;
                            z = SHADOW_Z;
                            sz = PANEL_Z-HUD_Z;
                            if(e->animation->shadow_coords)
                            {
                                if(e->direction) qx += e->animation->shadow_coords[e->animpos][0];
                                else qx -= e->animation->shadow_coords[e->animpos][0];
                                qy += e->animation->shadow_coords[e->animpos][1];
                                sy -= e->animation->shadow_coords[e->animpos][1];
                            }
                            shadowmethod = plainmethod;
                            shadowmethod.fillcolor = (shadowcolor>0?shadowcolor:0);
                            shadowmethod.alpha = shadowalpha;
                            shadowmethod.scalex = drawmethod->scalex;
                            shadowmethod.flipx = drawmethod->flipx;
                            shadowmethod.scaley = light[1]*drawmethod->scaley/256;
                            shadowmethod.flipy = drawmethod->flipy;
                            shadowmethod.centery += alty;
                            if(shadowmethod.flipy) shadowmethod.centery = -shadowmethod.centery;
                            if(shadowmethod.scaley<0)
                            {
                                shadowmethod.scaley = -shadowmethod.scaley;
                                shadowmethod.flipy = !shadowmethod.flipy;
                            }
                            shadowmethod.rotate = drawmethod->rotate;
                            shadowmethod.shiftx = drawmethod->shiftx + light[0];

                            spriteq_add_sprite(qx, qy, z, f, &shadowmethod, 0);
                            if(use_mirror)
                            {
                                shadowmethod.flipy = !shadowmethod.flipy;
                                shadowmethod.centery = -shadowmethod.centery;
                                spriteq_add_sprite(qx, sy, sz, f, &shadowmethod, 0);
                            }
                        }
                    }//end of gfxshadow
                }
                else //plan shadow
                {
                    useshadow = e->animation->shadow?e->animation->shadow[e->animpos]:e->modeldata.shadow;
                    if(useshadow<0) {useshadow=e->modeldata.shadow;}
                    if(useshadow && e->a>=0 && !(checkhole(e->x, e->z) && checkwall_below(e->x, e->z, e->a)<0) && (!e->modeldata.aironly || (e->modeldata.aironly && inair(e))))
                    {
                        if(other && other != e && e->a >= other->a + other->animation->platform[other->animpos][7])
                        {
                            qx = (int)(e->x - advancex);
                            qy = (int)(e->z - other->a - other->animation->platform[other->animpos][7] + gfx_y_offset);
                            sy = (int)((2*MIRROR_Z - e->z) - other->a - other->animation->platform[other->animpos][7] + gfx_y_offset);
                            z = (int)(other->z + 1);
                            sz = 2*PANEL_Z - z;
                        }
                        else if(level && wall >= 0)// && e->a >= level->walls[wall][7])
                        {
                            qx = (int)(e->x - advancex);
                            qy = (int)(e->z - level->walls[wall][7] + gfx_y_offset);
                            sy = (int)((2*MIRROR_Z - e->z)  - level->walls[wall][7] + gfx_y_offset);
                            z = SHADOW_Z;
                            sz = PANEL_Z-HUD_Z;
                        }
                        else
                        {
                            qx = (int)(e->x - advancex);
                            qy = (int)(e->z + gfx_y_offset);
                            sy = (int)((2*MIRROR_Z - e->z) + gfx_y_offset);
                            z = SHADOW_Z;
                            sz = PANEL_Z-HUD_Z;
                        }
                        if(e->animation->shadow_coords)
                        {   if(e->direction) qx += e->animation->shadow_coords[e->animpos][0];
                            else qx -= e->animation->shadow_coords[e->animpos][0];
                            qy += e->animation->shadow_coords[e->animpos][1];
                            sy -= e->animation->shadow_coords[e->animpos][1];
                        }
                        shadowmethod=plainmethod;
                        shadowmethod.alpha = BLEND_MULTIPLY+1;
                        shadowmethod.flipx = !e->direction;
                        spriteq_add_sprite(qx, qy, z, shadowsprites[useshadow-1], &shadowmethod, 0);
                        if(use_mirror)
                        spriteq_add_sprite(qx, sy, sz, shadowsprites[useshadow-1], &shadowmethod, 0);
                    }//end of plan shadow
                }
            }// end of blink checking

            if(e->arrowon)    // Display the players image while invincible to indicate player number
            {
                if(e->modeldata.parrow[(int)e->playerindex][0] && e->invincible == 1)
                    spriteq_add_sprite((int)(e->x - advancex + e->modeldata.parrow[(int)e->playerindex][1]), (int)(e->z-e->a+gfx_y_offset + e->modeldata.parrow[(int)e->playerindex][2]), (int)e->z, e->modeldata.parrow[(int)e->playerindex][0], NULL, e->sortid*2);
            }
        }// end of if(ent_list[i]->exists)
    }// end of for
}



void toss(entity *ent, float lift)
{
	if(!lift) return; //zero?
    ent->toss_time = time + 1;
    ent->tossv = lift;
    ent->a += 0.5;        // Get some altitude (needed for checks)
}



entity * findent(int types)
{
    int i;
    for(i=0; i<MAX_ENTS; i++)
    { // 2007-12-18, remove all nodieblink checking, because dead corpse with nodieblink 3 will be changed to TYPE_NONE
      // so if it is "dead" and TYPE_NONE, it must be a corpse
        if(ent_list[i]->exists && (ent_list[i]->modeldata.type & types) && !(ent_list[i]->dead && ent_list[i]->modeldata.type==TYPE_NONE))
        {
                return ent_list[i];
        }
    }
    return NULL;
}



int count_ents(int types)
{
    int i;
    int count = 0;
    for(i=0; i<MAX_ENTS; i++)
    { // 2007-12-18, remove all nodieblink checking, because dead corpse with nodieblink 3 will be changed to TYPE_NONE
      // so if it is "dead" and TYPE_NONE, it must be a corpse
        count += (ent_list[i]->exists && (ent_list[i]->modeldata.type & types) && !(ent_list[i]->dead && ent_list[i]->modeldata.type==TYPE_NONE));
    }
    return count;
}



entity * find_ent_here(entity *exclude, float x, float z, int types)
{
    int i;
    for(i=0; i<MAX_ENTS; i++)
    {
        if( ent_list[i]->exists
            && ent_list[i] != exclude
            && (ent_list[i]->modeldata.type & types)
            && diff(ent_list[i]->x,x)<(self->modeldata.grabdistance*0.83333)
            && diff(ent_list[i]->z,z)<(self->modeldata.grabdistance/3)
            && ent_list[i]->animation->vulnerable[ent_list[i]->animpos]  )
        {
            return ent_list[i];
        }
    }
    return NULL;
}

int set_idle(entity* ent)
{
    //int ani = ANI_IDLE;
    //if(validanim(ent,ANI_FAINT) && ent->health <= ent->modeldata.health / 4) ani = ANI_FAINT;
    //if(validanim(ent,ani)) ent_set_anim(ent, ani, 0);
    if (common_idle_anim(ent))
    {
    }
    else return 0;
    ent->idling = 1;
    ent->attacking = 0;
    ent->inpain = 0;
	ent->jumping = 0;
    ent->blocking = 0;
    return 1;
}

int set_death(entity *iDie, int type, int reset)
{
    //iDie->xdir = iDie->zdir = iDie->tossv = 0; // stop the target
	if(iDie->blocking && validanim(iDie, ANI_CHIPDEATH)){
		ent_set_anim(iDie,ANI_CHIPDEATH,reset);
		iDie->idling = 0;
		iDie->getting = 0;
		iDie->jumping = 0;
		iDie->charging = 0;
		iDie->attacking = 0;
		iDie->blocking = 0;
		return 1;
	}
    if(type < 0 || type >= max_attack_types || !validanim(iDie,animdies[type])) type = 0;
    if(validanim(iDie,animdies[type])) ent_set_anim(iDie, animdies[type], reset);
    else return 0;

    iDie->idling = 0;
    iDie->getting = 0;
    iDie->jumping = 0;
    iDie->charging = 0;
    iDie->attacking = 0;
    iDie->blocking = 0;
    if(iDie->frozen) unfrozen(iDie);
    return 1;
}


int set_fall(entity *iFall, int type, int reset, entity* other, int force, int drop, int noblock, int guardcost, int jugglecost, int pauseadd)
{
    if(type < 0 || type >= max_attack_types || !validanim(iFall,animfalls[type])) type = 0;
    if(validanim(iFall,animfalls[type])) ent_set_anim(iFall, animfalls[type], reset);
    else return 0;
    iFall->drop = 1;
    iFall->inpain = 0;
    iFall->idling = 0;
	iFall->falling = 1;
	iFall->jumping = 0;
	iFall->getting = 0;
    iFall->charging = 0;
	iFall->attacking = 0;
    iFall->blocking = 0;
	iFall->nograb = 1;
    if(iFall->frozen) unfrozen(iFall);
    execute_onfall_script(iFall, other, force, drop, type, noblock, guardcost, jugglecost, pauseadd);

    return 1;
}

int set_rise(entity *iRise, int type, int reset)
{
	if(type < 0 || type >= max_attack_types || !validanim(iRise,animrises[type])) type = 0;
	if(validanim(iRise,animrises[type])) ent_set_anim(iRise, animrises[type], reset);
	else return 0;
	iRise->takeaction = common_rise;
    // Get up again
    iRise->drop = 0;
    iRise->falling = 0;
    iRise->projectile = 0;
	iRise->nograb = 0;
    iRise->xdir = self->zdir = self->tossv = 0;
	iRise->modeldata.jugglepoints[0] = iRise->modeldata.jugglepoints[1]; //reset jugglepoints
	return 1;
}

int set_riseattack(entity *iRiseattack, int type, int reset)
{
	if(!validanim(iRiseattack,animriseattacks[type]) && iRiseattack->modeldata.riseattacktype == 1) type = 0;
	if(iRiseattack->modeldata.riseattacktype == 0 || type < 0 || type >= max_attack_types) type = 0;
	if(validanim(iRiseattack,animriseattacks[type])) ent_set_anim(iRiseattack, animriseattacks[type], reset);
	else return 0;
	self->staydown[2] = 0;			//Reset riseattack delay.
    set_attacking(iRiseattack);
    iRiseattack->drop = 0;
	iRiseattack->nograb = 0;
    ent_set_anim(iRiseattack, animriseattacks[type], 0);
    iRiseattack->takeaction = common_attack_proc;
	iRiseattack->modeldata.jugglepoints[0] = iRiseattack->modeldata.jugglepoints[1]; //reset jugglepoints
	return 1;
}

int set_blockpain(entity *iBlkpain, int type, int reset)
{
	if(!validanim(iBlkpain,ANI_BLOCKPAIN)){
		iBlkpain->takeaction = common_pain;
		return 1;
	}
	if(type < 0 || type >= max_attack_types || !validanim(iBlkpain,animblkpains[type])) type = 0;
	if(validanim(iBlkpain,animblkpains[type])) ent_set_anim(iBlkpain, animblkpains[type], reset);
	else return 0;
	iBlkpain->takeaction = common_block;
    set_attacking(iBlkpain);
	return 1;
}

int set_pain(entity *iPain, int type, int reset)
{
    int pain = 0;

    iPain->xdir = iPain->zdir = iPain->tossv = 0; // stop the target
	if(iPain->modeldata.guardpoints[1] > 0 && iPain->modeldata.guardpoints[0] <= 0) pain = ANI_GUARDBREAK;
    else if(type == -1 || type >= max_attack_types) pain = ANI_GRABBED;
    else pain = animpains[type];
    if(validanim(iPain,pain))              ent_set_anim(iPain, pain, reset);
    else if(validanim(iPain,animpains[0])) ent_set_anim(iPain, animpains[0], reset);
    else if(validanim(iPain,ANI_IDLE))     ent_set_anim(iPain, ANI_IDLE, reset);
    else return 0;

    if(pain == ANI_GRABBED) iPain->inpain = 0;
    else iPain->inpain = 1;

    iPain->idling = 0;
    iPain->falling = 0;
    iPain->projectile = 0;
    iPain->drop = 0;
    iPain->attacking = 0;
    iPain->getting = 0;
    iPain->charging = 0;
    iPain->jumping = 0;
    iPain->blocking = 0;
	if(iPain->modeldata.guardpoints[1] > 0 && iPain->modeldata.guardpoints[0] <= 0) iPain->modeldata.guardpoints[0] = iPain->modeldata.guardpoints[1];
    if(iPain->frozen) unfrozen(iPain);

    execute_onpain_script(iPain, type, reset);
    return 1;
}


//change model, anim_flag 1: reset animation 0: use original animation
void set_model_ex(entity* ent, char* modelname, int index, s_model* newmodel, int anim_flag)
{
    s_model* model = NULL;
    s_model oldmodel;
    int   animnum, animpos;
    int   i;
    int   type = ent->modeldata.type;

    model = ent->model;
    if(!newmodel)
    {
        if(index>=0) newmodel = model_cache[index].model;
        else newmodel = find_model(modelname);
    }
    if(!newmodel) shutdown(1, "Can't set model for entity '%s', model not found.\n", ent->name);
    if(newmodel==model) return;

    animnum = ent->animnum;
    animpos = ent->animpos;

    if(!(newmodel->model_flag & MODEL_NO_COPY))
    {
        if(!newmodel->speed) newmodel->speed = model->speed;
        if(!newmodel->runspeed)
        {
            newmodel->runspeed = model->runspeed;
            newmodel->runjumpheight = model->runjumpheight;
            newmodel->runjumpdist = model->runjumpdist;
            newmodel->runupdown = model->runupdown;
            newmodel->runhold = model->runhold;
        }
        if(newmodel->icon     < 0) newmodel->icon       = model->icon;
        if(newmodel->iconpain < 0) newmodel->iconpain   = model->iconpain;
        if(newmodel->iconget  < 0) newmodel->iconget    = model->iconget;
        if(newmodel->icondie  < 0) newmodel->icondie    = model->icondie;
        if(newmodel->shadow   < 0) newmodel->shadow     = model->shadow;
        if(newmodel->knife    < 0) newmodel->knife      = model->knife;
        if(newmodel->pshotno  < 0) newmodel->pshotno    = model->pshotno;
        if(newmodel->bomb     < 0) newmodel->bomb       = model->bomb;
        if(newmodel->star     < 0) newmodel->star       = model->star;
        if(newmodel->flash    < 0) newmodel->flash      = model->flash;
        if(newmodel->bflash   < 0) newmodel->bflash     = model->bflash;
        if(newmodel->dust[0]  < 0) newmodel->dust[0]    = model->dust[0];
        if(newmodel->dust[1]  < 0) newmodel->dust[1]    = model->dust[1];
		if(newmodel->diesound < 0) newmodel->diesound   = model->diesound;

        for(i=0; i<max_animations; i++)
        {
            if(!newmodel->animation[i] && model->animation[i] && model->animation[i]->numframes>0)
                newmodel->animation[i] = model->animation[i];
        }
        // copy the weapon list if model flag is not set to use its own weapon list
        if(!(newmodel->model_flag & MODEL_NO_WEAPON_COPY))
        {
            newmodel->weapnum = model->weapnum;
            if(!newmodel->weapon) newmodel->weapon = model->weapon;
        }
    }

    if(anim_flag)
    {
        ent->attacking = 0;
        ent_set_model(ent, newmodel->name);
    }
    else
    {
        oldmodel = ent->modeldata;
        ent->model = newmodel;
        ent->modeldata = *newmodel;
        ent->animation = newmodel->animation[ent->animnum];
        ent_copy_uninit(ent, &oldmodel);
    }


    ent->modeldata.type = type;

    Script_Copy(ent->animation_script,  newmodel->animation_script,     0);
    Script_Copy(ent->update_script,     newmodel->update_script,        0);
    Script_Copy(ent->think_script,      newmodel->think_script,         0);
    Script_Copy(ent->didhit_script,     newmodel->didhit_script,        0);
    Script_Copy(ent->onspawn_script,    newmodel->onspawn_script,       0);
    Script_Copy(ent->takedamage_script, newmodel->takedamage_script,    0);
    Script_Copy(ent->onpain_script,     newmodel->onpain_script,        0);
    Script_Copy(ent->onfall_script,     newmodel->onfall_script,        0);
    Script_Copy(ent->onblocks_script,   newmodel->onblocks_script,      0);
    Script_Copy(ent->onblockw_script,   newmodel->onblockw_script,      0);
    Script_Copy(ent->onblocko_script,   newmodel->onblocko_script,      0);
    Script_Copy(ent->onblockz_script,   newmodel->onblockz_script,      0);
    Script_Copy(ent->onblocka_script,   newmodel->onblocka_script,      0);
    Script_Copy(ent->onmovex_script,    newmodel->onmovex_script,       0);
    Script_Copy(ent->onmovez_script,    newmodel->onmovez_script,       0);
    Script_Copy(ent->onmovea_script,    newmodel->onmovea_script,       0);
    Script_Copy(ent->ondeath_script,    newmodel->ondeath_script,       0);
    Script_Copy(ent->onkill_script,     newmodel->onkill_script,        0);
    Script_Copy(ent->didblock_script,   newmodel->didblock_script,      0);
	Script_Copy(ent->ondoattack_script, newmodel->ondoattack_script,    0);
    Script_Copy(ent->key_script,        newmodel->key_script,           0);

    ent_set_colourmap(ent, ent->map);
}

void set_weapon(entity* ent, int wpnum, int anim_flag) // anim_flag added for scripted midair weapon changing
{
    if(!ent) return;
//printf("setweapon: %d \n", wpnum);
    if(ent->modeldata.weapon && wpnum > 0 && wpnum <= MAX_WEAPONS && (*ent->modeldata.weapon)[wpnum-1])
        set_model_ex(ent, NULL, (*ent->modeldata.weapon)[wpnum-1], NULL, !anim_flag);
    else set_model_ex(ent, NULL, -1, ent->defaultmodel, 1);

    if(ent->modeldata.type == TYPE_PLAYER) // save current weapon for player's weaploss 3
    {
        if(ent->modeldata.weaploss[0] >= 3) player[(int)ent->playerindex].weapnum = wpnum;
        else player[(int)ent->playerindex].weapnum = level->setweap;
    }
}

//////////////////////////////////////////////////////////////////////////
//                  common A.I. code for enemies & NPCs
//////////////////////////////////////////////////////////////////////////


entity* melee_find_target()
{
    return NULL;
}

entity* long_find_target()
{
    return NULL;
}

entity* normal_find_target(int anim)
{
    int i , min, max;
    int index = -1;
    min = 0;
    max = 9999;
    //find the 'nearest' one
    for(i=0; i<ent_max; i++)
    {
        if( ent_list[i]->exists && ent_list[i] != self //cant target self
            && (ent_list[i]->modeldata.type & self->modeldata.hostile)
            && (anim<0||(anim>=0 && check_range(self, ent_list[i], anim)))
            && !ent_list[i]->dead //must be alive
            && diff(ent_list[i]->x,self->x)+ diff(ent_list[i]->z,self->z) >= min
            && diff(ent_list[i]->x,self->x)+ diff(ent_list[i]->z,self->z) <= max
			&& ent_list[i]->modeldata.stealth[0] <= self->modeldata.stealth[1] //Stealth factor less then perception factor (allows invisibility).
              )
        {
            if(index <0 || (index>=0 && (!ent_list[index]->animation->vulnerable[ent_list[index]->animpos] || ent_list[index]->invincible == 1)) ||
                ( (self->x < ent_list[i]->x) == (self->direction) && // don't turn to the one on the back
                diff(ent_list[i]->x, self->x)+diff(ent_list[i]->z, self->z) < diff(ent_list[index]->x, self->x)+diff(ent_list[index]->z, self->z)))
                index = i;
        }
    }
    if( index >=0) {return ent_list[index];}
    return NULL;
}

//Used by default A.I. pattern
// A.I. characters try to find a pickable item
entity * normal_find_item(){

    int i;
    int index = -1;
    //find the 'nearest' one
    for(i=0; i<ent_max; i++){
        if( ent_list[i]->exists
            && (ent_list[i]->modeldata.type & TYPE_ITEM)
            && diff(ent_list[i]->x,self->x)+ diff(ent_list[i]->z,self->z)< 300
            && ent_list[i]->animation->vulnerable[ent_list[i]->animpos]
            && (validanim(self,ANI_GET) || ent_list[i]->modeldata.subtype == SUBTYPE_TOUCH)
                && ((ent_list[i]->modeldata.subtype == SUBTYPE_WEAPON && !self->weapent && self->modeldata.weapon && (*self->modeldata.weapon)[ent_list[i]->modeldata.weapnum-1]>=0)
                ||(ent_list[i]->modeldata.subtype == SUBTYPE_PROJECTILE && !self->weapent)
                ||(ent_list[i]->health && self->health < self->modeldata.health && ent_list[i]->modeldata.subtype != SUBTYPE_PROJECTILE && ent_list[i]->modeldata.subtype != SUBTYPE_WEAPON)
                )
                ){
                    if(index <0 || diff(ent_list[i]->x, self->x)+diff(ent_list[i]->z, self->z) < diff(ent_list[index]->x, self->x)+diff(ent_list[index]->z, self->z))
                        index = i;
                }
    }
    if( index >=0) return ent_list[index];
    return NULL;
}

int long_attack()
{
    return 0;
}

int melee_attack()
{
    return 0;
}

// chose next attack in atchain, if succeeded, return 1, otherwise return 0.
int perform_atchain()
{
    int pickanim = 0;
    if(self->combotime > time)
        self->combostep[0]++;
    else self->combostep[0] = 1;

    if(self->modeldata.atchain[self->combostep[0]-1]==0) // 0 means the chain ends
    {
        self->combostep[0] = 0;
        return 0;
    }

    if(validanim(self,animattacks[self->modeldata.atchain[self->combostep[0]-1]-1]) )
    {
        if(((self->combostep[0]==1||self->modeldata.combostyle!=1) && self->modeldata.type==TYPE_PLAYER) ||  // player should use attack 1st step without checking range
           (self->modeldata.combostyle!=1 && normal_find_target(animattacks[self->modeldata.atchain[0]-1])) || // normal chain just checks the first attack in chain(guess no one like it)
           (self->modeldata.combostyle==1 && normal_find_target(animattacks[self->modeldata.atchain[self->combostep[0]-1]-1]))) // combostyle 1 checks all anyway
        {
            pickanim = 1;
        }
        else if(self->modeldata.combostyle==1 && self->combostep[0]!=1) // ranged combo? search for a valid attack
        {
            while(++self->combostep[0]<=self->modeldata.chainlength)
            {
                if(self->modeldata.atchain[self->combostep[0]-1] &&
                   validanim(self,animattacks[self->modeldata.atchain[self->combostep[0]-1]-1]) &&
                   (self->combostep[0]==self->modeldata.chainlength ||
                    normal_find_target(animattacks[self->modeldata.atchain[self->combostep[0]-1]-1])))
                {
                    pickanim = 1;
                    break;
                }
            }
        }
    }
    else self->combostep[0] = 0;
    if(pickanim)
    {
        ent_set_anim(self, animattacks[self->modeldata.atchain[self->combostep[0]-1]-1], 1);
        set_attacking(self);
        self->takeaction = common_attack_proc;
    }
    if(!pickanim || self->combostep[0] > self->modeldata.chainlength) self->combostep[0] = 0;
    return pickanim;
}

void normal_prepare()
{
    int i, lastpick = -1;
    int predir = self->direction;
    entity* target = normal_find_target(-1);

    self->xdir = self->zdir = 0; //stop

    if(!target)
    {
        self->idling = 1;
        self->takeaction = NULL;
        return;
    }

    //check if target is behind, so we can perform a turn back animation
    if(!self->modeldata.noflip) self->direction = (self->x < target->x);
    if(predir != self->direction && validanim(self,ANI_TURN))

    {
        self->direction = predir;
        set_turning(self);
        ent_set_anim(self, ANI_TURN, 0);
        self->takeaction = common_turn;
        return;
    }

    // Wait...
    if(time < self->stalltime) return;
    // let go the projectile, well
    if( self->weapent && self->weapent->modeldata.subtype == SUBTYPE_PROJECTILE &&
        validanim(self,ANI_THROWATTACK) &&
        (target = normal_find_target(ANI_THROWATTACK)))
    {
        set_attacking(self);
        ent_set_anim(self, ANI_THROWATTACK, 0);
        self->takeaction = common_attack_proc;
        return ;
    }

    // move freespecial check here
    if((rand32()&7) < 2)
    {
        for(i=0; i<max_freespecials; i++)
        {
            if(validanim(self,animspecials[i]) &&
               (check_energy(1, animspecials[i]) ||
                check_energy(0, animspecials[i])) &&
               (target=normal_find_target(animspecials[i])) &&
               (rand32()%max_freespecials)<3 &&
               check_costmove(animspecials[i], 1) )
            {
                return;
            }
        }
    }

    if(self->modeldata.chainlength > 1) // have a chain?
    {
        if(perform_atchain()) return;
    }
    else if(target) // dont have a chain so just select an attack randomly
    {
        // Pick an attack
        for(i=0; i<max_attacks; i++)
        {
            if( validanim(self,animattacks[i]) &&
                (target = normal_find_target(animattacks[i])))
            {
                lastpick = animattacks[i];
                if((rand32() & 31) > 10) break;
            }
        }
        if(lastpick >= 0)
        {
            set_attacking(self);
            ent_set_anim(self, lastpick, 0);
            self->takeaction = common_attack_proc;
            return ;
        }
    }

    // No attack to perform, return to A.I. root
    self->idling = 1;
    self->takeaction = NULL;
}

void common_jumpland()
{
    if(self->animating) return;
    set_idle(self);
    self->takeaction = NULL;
}

//A.I characters play the jump animation
void common_jump()
{
    entity* dust;

    if(inair(self))
    {
        if(self->animation->dive[0] || self->animation->dive[1])
        {
            self->tossv = 0;    // Void tossv so "a" can be adjusted manually
            self->toss_time = 0;

            if(self->direction) self->xdir = self->animation->dive[0];
            else self->xdir = -self->animation->dive[0];

            self->a -= self->animation->dive[1];

            if(self->a <= self->base) self->a = self->base;    // Don't want to go below ground
        }
        return;
    }

    if(self->tossv<=0) // wait if it is still go up
    {
        self->tossv = 0;
        self->a = self->base;

        self->jumping = 0;
        self->attacking = 0;

        if(!self->modeldata.runhold) self->running = 0;

        self->zdir = self->xdir = 0;

        if(validanim(self,ANI_JUMPLAND) && self->animation->landframe[0] == -1) // check if jumpland animation exists and not using landframe
        {
            ent_set_anim(self, ANI_JUMPLAND, 0);
			if(self->modeldata.dust[1]>=0)
			{
				dust = spawn(self->x, self->z, self->a, self->direction, NULL, self->modeldata.dust[1], NULL);
				dust->base = self->a;
				dust->autokill = 1;
				execute_onspawn_script(dust);
			}
            self->takeaction = common_jumpland;
        }
        else
        {
			if(self->modeldata.dust[1]>=0 && self->animation->landframe[0] == -1)
			{
				dust = spawn(self->x, self->z, self->a, self->direction, NULL, self->modeldata.dust[1], NULL);
				dust->base = self->a;
				dust->autokill = 1;
				execute_onspawn_script(dust);
			}
			if(self->animation->landframe[0] >= 0 && self->animating) return;

            set_idle(self);
            self->takeaction = NULL; // back to A.I. root
        }
    }
}

//A.I. characters spawn
void common_spawn()
{
    self->idling = 0;
    if(self->animating) return;
    set_idle(self);
    self->takeaction = NULL; // come to life
}

//A.I. characters drop from the sky
void common_drop()
{
    if(inair(self)) return;
    self->idling = 1;
    self->takeaction = NULL;
    if(self->health<=0) kill(self);
}

// play turn animation and then flip
void common_turn()
{
    if(!self->animating)
    {
        self->xdir = self->zdir = 0;
        self->direction = !self->direction;
        set_idle(self);
        self->takeaction = NULL;
    }
}

// switch to land animation, land safely
void doland()
{
    self->xdir = self->zdir = 0;
    self->drop = 0;
    self->projectile = 0;
	self->damage_on_landing = 0;
    if(validanim(self,ANI_LAND))
    {
        self->direction = !self->direction;
        ent_set_anim(self, ANI_LAND, 0);
        self->takeaction = common_land;
    }
    else
    {
        set_idle(self);
        self->takeaction = NULL;
    }
}

void common_fall()
{
    // Still falling?
    if(self->falling ||  inair(self) || self->tossv)
    {
        return;
    }


    //self->xdir = self->zdir;

    // Landed
    if(self->projectile > 0)
    {
        if(self->projectile == 2)
        {   // damage_on_landing==-2 means a player has pressed up+jump and has a land animation
            if((autoland == 1 && self->damage_on_landing == -1) ||self->damage_on_landing == -2)
            {
				// Added autoland option for landing
                doland();
                return;
            }
        }
        //self->projectile = 0;
        self->falling = 0;
    }

    // Drop Weapon due to Enemy Falling.
    if(self->modeldata.weaploss[0] == 1) dropweapon(1);

    if(self->boss && level_completed) tospeedup = 1;

    // Pause a bit...
    self->takeaction	= common_lie;
    self->stalltime		= self->staydown[0] + (time + GAME_SPEED - self->modeldata.risetime[0]);	//Set rise delay.
	self->staydown[2]	= self->staydown[1] + (time - self->modeldata.risetime[1]);					//Set rise attack delay.
	self->staydown[0] = 0; //Reset staydown.
	self->staydown[1] = 0; //Reset staydown atk.
}

void common_try_riseattack()
{
    entity * target;
    if(!validanim(self,ANI_RISEATTACK)) return;

    target = normal_find_target(ANI_RISEATTACK);
    if(!target)
    {
        self->direction = !self->direction;
        target = normal_find_target(ANI_RISEATTACK);
        self->direction = !self->direction;
    }

    if(target)
    {
        self->direction = (target->x > self->x);    // Stands up and swings in the right direction depending on chosen target
        set_riseattack(self, self->damagetype, 0);
    }
}

void common_lie()
{
    // Died?
    if(self->health <= 0)
    {
        // Drop Weapon due to death.
        if(self->modeldata.weaploss[0] <= 2) dropweapon(1);
        if(self->modeldata.falldie == 2) set_death(self, self->damagetype, 0);
        if(!self->modeldata.nodieblink || (self->modeldata.nodieblink == 1 && !self->animating))
        {    // Now have the option to blink or not
            self->takeaction = (self->modeldata.type == TYPE_PLAYER)?player_blink:suicide;
            self->blink = 1;
            self->stalltime = self->nextthink = time + GAME_SPEED * 2;
        }
        else if(self->modeldata.nodieblink == 2  && !self->animating)
        {
            self->takeaction = (self->modeldata.type == TYPE_PLAYER)?player_die:suicide;

        }
        else if(self->modeldata.nodieblink == 3  && !self->animating)
        {
            if(self->modeldata.type == TYPE_PLAYER)
            {
                self->takeaction = player_die;

            }
            else
            {
                self->modeldata.type = TYPE_NONE;
                self->noaicontrol = 1;
            }
        }

        if (self->modeldata.komap[0])                                                           //Have a KO map?
        {
            if (self->modeldata.komap[1])                                                       //Wait for fall/death animation to finish?
            {
                if (!self->animating)
                {
                    self->colourmap = self->modeldata.colourmap[self->modeldata.komap[0]-1];    //If finished animating, apply map.
                }
            }
            else                                                                                //Don't bother waiting.
            {
                self->colourmap = self->modeldata.colourmap[self->modeldata.komap[0]-1];        //Apply map.
            }
        }

        return;
    }

    if(time < self->stalltime || self->a!=self->base || self->tossv) return;

    //self->takeaction = common_rise;
    // Get up again
    //self->drop = 0;
    //self->falling = 0;
    //self->projectile = 0;
    //self->xdir = self->zdir = self->tossv = 0;

    set_rise(self, self->damagetype, 0);
}

// rise proc
void common_rise()
{
    if(self->animating) return;
	self->staydown[2] = 0;	//Reset riseattack delay.
    set_idle(self);
    if(self->modeldata.riseinv)
    {
        self->blink = self->modeldata.riseinv>0;
        self->invinctime = time + ABS(self->modeldata.riseinv);
        self->invincible = 1;
    }
    self->takeaction = NULL;
}

// pain proc
void common_pain()
{
    //self->xdir = self->zdir = 0; // complained

    if(self->animating || inair(self)) return;

    self->inpain = 0;
    if(self->link){
//        set_pain(self, -1, 0);
        self->takeaction = common_grabbed;
    }
    else if(self->blocking)
    {
        ent_set_anim(self, ANI_BLOCK, 1);
        self->takeaction = common_block;
    }
    else
    {
        set_idle(self);
        self->takeaction = NULL;
    }
}

void doprethrow()
{
    entity * other = self->link;
	self->xdir = self->zdir = self->tossv = other->xdir = other->zdir = other->tossv = 0;
    ent_set_anim(self, ANI_THROW, 0);
    other->takeaction = common_prethrow;
    self->takeaction = common_throw_wait;
}

// 1 grabattack 2 grabforward 3 grabup 4 grabdown 5 grabbackward
// other means grab finisher at once
void dograbattack(int which)
{
    entity * other = self->link;
    other->xdir = other->zdir = self->xdir = self->zdir = 0;
    if(which<5 && which>=0)
    {
        ++self->combostep[which];
        if(self->combostep[which] < 3)
            ent_set_anim(self, grab_attacks[which][0], 0);
        else
        {
            if(validanim(self,grab_attacks[which][1])) ent_set_anim(self, grab_attacks[which][1], 0);
            else ent_set_anim(self, ANI_ATTACK3, 0);
            memset(self->combostep, 0, sizeof(int)*5);
        }
    }
    else
    {
        if(validanim(self,grab_attacks[0][1])) ent_set_anim(self, grab_attacks[0][1], 0);
        else if(validanim(self,ANI_ATTACK3)) ent_set_anim(self, ANI_ATTACK3, 0);
        else return ;
        memset(self->combostep, 0, sizeof(int)*5);
    }
    self->attacking = 1;
    self->takeaction = common_grabattack;
}

void dovault()
{
    int heightvar;
	entity * other = self->link;
    self->link->xdir = self->link->zdir = self->xdir = self->zdir = 0;

    self->attacking = 1;
    self->x = other->x;

    if(other->animation->height) heightvar = other->animation->height;
    else heightvar = other->modeldata.height;

    self->base = other->base + heightvar;
    ent_set_anim(self, ANI_VAULT, 0);
    self->takeaction = common_vault;
}

void common_grab_check()
{
    int rnum, which;
    entity * other = self->link;

    if(other == NULL || (self->modeldata.grabfinish && self->animating && !self->grabwalking)) return;

    if(self->base != other->base)
    {       // Change this from ->a to ->base
        ent_unlink(self);
        set_idle(self);
        self->takeaction = NULL;
        return;
    }

    if(!nolost && self->modeldata.weaploss[0] <= 0) dropweapon(1);

    self->attacking = 0; //for checking

    rnum = rand32()&31;

    if(time > self->releasetime)
    {
        if(rnum < 12)
        {
            // Release
            ent_unlink(self);
            set_idle(self);
            self->takeaction = NULL;
            return;
        }
        else self->releasetime = time + (GAME_SPEED/2);
    }

    if(validanim(self,ANI_THROW) && rnum < 7)
    {
        if(self->modeldata.throwframewait >= 0)
            doprethrow();
        else
            dothrow();
        return;
    }
    //grab finisher
    if(rnum < 4)
    {
         dograbattack(-1);
         return;
    }
    which = rnum%5;
    // grab attacks
    if(rnum > 12 && validanim(self,grab_attacks[which][0]))
    {
        dograbattack(which);
        return;
    }
    // Vaulting.
    if(rnum < 8 && validanim(self,ANI_VAULT))
    {
        dovault();
        return;
    }
}

//grabbing someone
void common_grab()
{
    // if(self->link) return;
    if(self->link || (self->modeldata.grabfinish && self->animating && !self->grabwalking)) return;

    memset(self->combostep, 0, sizeof(int)*5);
    set_idle(self);
    self->takeaction = NULL;
    self->attacking = 0;
}

// being grabbed
void common_grabbed()
{
    // Just check if we're still grabbed...
    if(self->link) return;

    set_idle(self);
    self->stalltime=0;
    self->takeaction = NULL;
}

// picking up something
void common_get()
{
    if(self->animating) return;

    set_idle(self);
    self->getting = 0;
    self->takeaction = NULL;
}

// A.I. characters do the block
void common_block()
{
    if(self->animating) return;

    set_idle(self);
    self->blocking = 0;
    self->takeaction = NULL;
}


void common_charge()
{
    if(self->animating) return;

    set_idle(self);
    self->charging = 0;
    self->takeaction = NULL;
}


// common code for entities hold an item
entity* drop_item(entity* e)
{
    s_spawn_entry p;
    entity* item;
    memset(&p, 0, sizeof(s_spawn_entry));

    p.index = e->item;
    p.itemindex = p.weaponindex = -1;
    strcpy(p.alias, e->itemalias);
    p.a = e->a+0.01; // for check, or an enemy "item" will drop from the sky
    p.health[0] = e->itemhealth;
    p.alpha = e->itemtrans;
    p.colourmap = e->itemmap;
    p.flip = e->direction;

    item = smartspawn(&p);

    if(item)
    {
        item->x = e->x;
        item->z = e->z;
        if(item->x < advancex) item->x = advancex + 10;
        else if(item->x > advancex + videomodes.hRes) item->x = advancex + videomodes.hRes - 10;
        if(!(level->scrolldir &(SCROLL_UP|SCROLL_DOWN)))
        {
            if(item->z-item->a < advancey) item->z = advancey + 10;
            else if(item->z-item->a > advancey + videomodes.vRes) item->z = advancey + videomodes.vRes - 10;
        }
        if(e->boss && item->modeldata.type==TYPE_ENEMY) item->boss = 1;
    }
    return item;
}

//drop the driver, just spawn, dont takedamage
// damage will adjust by the biker
entity* drop_driver(entity* e)
{
    int i;
    s_spawn_entry p;
    entity* driver;
    memset(&p, 0, sizeof(s_spawn_entry));

    if(e->modeldata.rider>=0) p.index = e->modeldata.rider;
    else         return NULL; // should not happen, just in case
    /*p.x = e->x - advancex; p.z = e->z; */p.a = e->a + 10;
    p.itemindex = e->item;
    p.weaponindex = -1;
    strcpy(p.itemalias, e->itemalias);
    strcpy(p.alias, e->name);
    p.itemmap = e->itemmap;
    p.itemtrans = e->itemtrans;
    p.itemhealth = e->itemhealth;
    p.itemplayer_count = e->itemplayer_count;
    //p.colourmap = e->map;
    for(i=0; i<MAX_PLAYERS; i++) p.health[i] = e->modeldata.health;
    p.boss = e->boss;

    driver = smartspawn(&p);
    if(driver)
    {
        driver->x = e->x;
        driver->z = e->z;
    }
    return driver;
}


void checkdeath()
{
    entity* item;
    if(self->health>0) return;
    self->dead = 1;
    //be careful, since the opponent can be other types
    if(self->opponent && self->opponent->modeldata.type == TYPE_PLAYER)
    {
        addscore(self->opponent->playerindex, self->modeldata.score);    // Add score to the player
    }
    self->nograb = 1;
    self->idling = 0;

    if(self->modeldata.diesound >= 0) sound_play_sample(self->modeldata.diesound, 0, savedata.effectvol,savedata.effectvol, 100);

    // drop item
    if(self->item && count_ents(TYPE_PLAYER) > self->itemplayer_count){ //4player
        item = drop_item(self);
    }

	if(self->boss){
        self->boss = 0;
		--level->bosses;
		if(!level->bosses && self->modeldata.type == TYPE_ENEMY){
			kill_all_enemies();
			level_completed = 1;
		}
	}
}

void checkdamageflip(entity* other, s_attack* attack)
{
    if(other == NULL || other==self || (!self->drop && (attack->no_pain || self->modeldata.nopain || (self->modeldata.defense_pain[(short)attack->attack_type] && attack->attack_force < self->modeldata.defense_pain[(short)attack->attack_type])))) return;

    if(!self->frozen && !self->modeldata.noflip)// && !inair(self))
    {
		if(attack->force_direction == 0)
		{
			if(self->x < other->x) self->direction = 1;
            else if(self->x > other->x) self->direction = 0;
		}
		else if(attack->force_direction == 1)
		{
            self->direction = other->direction;
		}
		else if(attack->force_direction == -1)
		{
            self->direction = !other->direction;
		}
		else if(attack->force_direction == 2)
		{
            self->direction = 1;
		}
		else if(attack->force_direction == -2)
		{
            self->direction = 0;
        }
    }
}

void checkdamageeffects(s_attack* attack)
{
#define _freeze         attack->freeze
#define _maptime        attack->maptime
#define _freezetime     attack->freezetime
#define _remap          attack->forcemap
#define _blast          attack->blast
#define _steal          attack->steal
#define _seal           attack->seal
#define _sealtime       attack->sealtime
#define _dot            attack->dot
#define _dot_index      attack->dot_index
#define _dot_time       attack->dot_time
#define _dot_force      attack->dot_force
#define _dot_rate       attack->dot_rate
#define _staydown0      attack->staydown[0]
#define _staydown1		attack->staydown[1]

    entity* opp = self->opponent;

    if(_steal && opp && opp!=self)
    {
        if(self->health >= attack->attack_force) opp->health += attack->attack_force;
        else opp->health += self->health;
        if(opp->health > opp->modeldata.health)
            opp->health = opp->modeldata.health;
    }
    if(_freeze && !self->frozen && !self->owner && !self->modeldata.nomove)
    {    // New freeze attack - If not frozen, freeze entity unless it's a projectile
        self->frozen = 1;
        if(self->freezetime == 0) self->freezetime = time + _freezetime;
        if(_remap == -1 && self->modeldata.fmap != -1) self->colourmap = self->modeldata.colourmap[self->modeldata.fmap-1];    //12/14/2007 Damon Caskey: If opponents fmap = -1 or only stun, then don't change the color map.
        self->drop = 0;
    }
    else if(self->frozen)
    {
        unfrozen(self);
        self->drop = 1;
    }

    if(_remap>0 && !_freeze)
    {
        self->maptime = time + _maptime;
        self->colourmap = self->modeldata.colourmap[_remap-1];
    }

    if(_seal)                                                                       //Sealed: Disable special moves.
    {
        self->sealtime  = time + _sealtime;                                         //Set time to apply seal. No specials for you!
        self->seal      = _seal;                                                    //Set seal. Any animation with energycost > seal is disabled.
    }

    if(_dot)                                                                        //dot: Damage over time effect.
    {
        self->dot_owner[_dot_index] = self->opponent;                               //dot owner.
        self->dot[_dot_index]       = _dot;                                         //Mode: 1. HP (non lethal), 2. MP, 3. HP (non lethal) & MP, 4. HP, 5. HP & MP.
        self->dot_time[_dot_index]  = time + (_dot_time * GAME_SPEED / 100);        //Gametime dot will expire.
        self->dot_force[_dot_index] = _dot_force;                                   //How much to dot each tick.
        self->dot_rate[_dot_index]  = _dot_rate;                                    //Delay between dot ticks.
        self->dot_atk[_dot_index]   = attack->attack_type;                          //dot attack type.
    }


    if(self->modeldata.nodrop) self->drop = 0;                                      // Static enemies/nodrop enemies cannot be knocked down

    if(inair(self) && !self->frozen && self->modeldata.nodrop<2) self->drop = 1;

    if(attack->no_pain) self->drop = 0;

    self->projectile = _blast;

    if(self->drop)
    {
        self->staydown[0]	= _staydown0;                                            //Staydown: Add to risetime until next rise.
        self->staydown[1]   = _staydown1;
    }

#undef _freeze
#undef _maptime
#undef _freezetime
#undef _remap
#undef _blast
#undef _steal
#undef _seal
#undef _sealtime
#undef _dot
#undef _dot_index
#undef _dot_time
#undef _dot_force
#undef _dot_rate
#undef _staydown0
#undef _staydown1
}

void checkdamagedrop(s_attack* attack)
{
	int attackdrop = attack->attack_drop;
	float fdefense_knockdown = self->modeldata.defense_knockdown[(short)attack->attack_type];
    if(self->modeldata.animal) self->drop = 1;
	if(self->modeldata.guardpoints[1] > 0 && self->modeldata.guardpoints[0] <= 0) attackdrop = 0; //guardbreak does not knock down.
    if(self->drop || attack->no_pain) return; // just in case, if we already fall, dont check fall again
    // reset count if knockdowntime expired.
    if(self->knockdowntime && self->knockdowntime<time)
        self->knockdowncount = self->modeldata.knockdowncount;

    self->knockdowncount -= (attackdrop * fdefense_knockdown);
    self->knockdowntime = time + GAME_SPEED;
    self->drop = (self->knockdowncount<0); // knockdowncount < 0 means knocked down
}

void checkmpadd()
{
    entity* other = self->opponent;
    if(other == NULL || other == self) return;

    if(magic_type == 1 )
    {
        if(other->modeldata.mprate) other->mp += other->modeldata.mprate;
        else other->mp++;

        if(other->mp > other->modeldata.mp) other->mp = other->modeldata.mp;
    }
}

void checkhitscore(entity* other, s_attack* attack)
{
    entity* opp = self->opponent;
    if(!opp) return;
    if(opp && opp!=self && opp->modeldata.type == TYPE_PLAYER)
    {    // Added obstacle so explosions can hurt enemies
        addscore(opp->playerindex, attack->attack_force*self->modeldata.multiple);    // New multiple variable
		control_rumble(opp->playerindex, attack->attack_force*2);
    }
    // Don't animate or fall if hurt by self, since
    // it means self fell to the ground already. :)
	// Add throw score to the player
    else if(other==self && self->damage_on_landing > 0) addscore(opp->playerindex, attack->attack_force);
}

void checkdamage(entity* other, s_attack* attack)
{
    int force = attack->attack_force;
    int type = attack->attack_type;
	if(self->modeldata.guardpoints[1] > 0 && self->modeldata.guardpoints[0] <= 0) force = 0; //guardbreak does not deal damage.
    if(!(self->damage_on_landing && self == other) && !other->projectile && type >= 0 && type<max_attack_types)
    {
        force = (int)(force * other->modeldata.offense_factors[type]);
        force = (int)(force * self->modeldata.defense_factors[type]);
    }

    self->health -= force; //Apply damage.

    if (self->health > self->modeldata.health) self->health = self->modeldata.health; //Cap negative damage to max health.

    execute_takedamage_script(self, other, force, attack->attack_drop, type, attack->no_block, attack->guardcost, attack->jugglecost, attack->pause_add);                       //Execute the take damage script.

    if (self->health <= 0)                                      //Health at 0?
    {
        if(!(self->a < PIT_DEPTH || self->lifespancountdown<0)) //Not a pit death or countdown?
        {
            if (self->invincible == 2)                          //Invincible type 2?
            {
                self->health = 1;                               //Stop at 1hp.
            }
            else if(self->invincible == 3)                      //Invincible type 3?
            {
                self->health = self->modeldata.health;          //Reset to max health.
            }
        }
        execute_ondeath_script(self, other, force, attack->attack_drop, type, attack->no_block, attack->guardcost, attack->jugglecost, attack->pause_add);   //Execute ondeath script.
    }
}

int checkgrab(entity *other, s_attack* attack)
{
    //if(attack->no_pain) return  0; //no effect, let modders to deside, don't bother check it here
    if(self!=other && attack->grab && cangrab(other, self))
    {
        if(adjust_grabposition(other, self, attack->grab_distance, attack->grab))
        {
            ents_link(other, self);
            self->a = other->a;
        }
        else return 0;
    }
    return 1;
}

int arrow_takedamage(entity *other, s_attack* attack)
{
    self->modeldata.no_adjust_base = 0;
    self->modeldata.subject_to_wall = self->modeldata.subject_to_platform = self->modeldata.subject_to_hole = self->modeldata.subject_to_gravity = 1;
    if( common_takedamage(other, attack) && self->dead)
    {
              return 1;
    }
    return 0;
}

int common_takedamage(entity *other, s_attack* attack)
{
    if(self->dead) return 0;
    if(self->toexplode == 2) return 0;
    // fake 'grab', if failed, return as the attack hit nothing
    if(!checkgrab(other, attack)) return 0; // try to grab but failed, so return 0 means attack missed

    // set pain_time so it wont get hit too often
    self->pain_time = time + (GAME_SPEED / 5);
    // set oppoent
    if(self!=other) set_opponent(self, other);
    // adjust type
    if(attack->attack_type >= 0 && attack->attack_type<max_attack_types) self->damagetype = attack->attack_type;
    else self->damagetype = ATK_NORMAL;
    // pre-check drop
    checkdamagedrop(attack);
    // Drop Weapon due to being hit.
    if(self->modeldata.weaploss[0] <= 0) dropweapon(1);
    // check effects, e.g., frozen, blast, steal
    if(!(self->modeldata.guardpoints[1] > 0 && self->modeldata.guardpoints[0] <= 0)) checkdamageeffects(attack);
    // check flip direction
    checkdamageflip(other, attack);
    // mprate can also control the MP recovered per hit.
    checkmpadd();
    //damage score
    checkhitscore(other, attack);
    // check damage, cost hp.
    checkdamage(other, attack);
    // is it dead now?
    checkdeath();

    if(self->modeldata.type == TYPE_PLAYER) control_rumble(self->playerindex, attack->attack_force * 3);
    if(self->a<=PIT_DEPTH && self->dead)
    {
        if(self->modeldata.type == TYPE_PLAYER) player_die();
        else kill(self);
        return 1;
    }
    // fall to the ground so dont fall again
    if(self->damage_on_landing)
    {
        self->damage_on_landing = 0;
        return 1;
    }
    // unlink due to being hit
    if((self->opponent && self->opponent->grabbing != self)||
       self->dead || self->frozen || self->drop)
    {
        ent_unlink(self);
    }
    // Enemies can now use SPECIAL2 to escape cheap attack strings!
    if(self->modeldata.escapehits)
    {
        if(self->drop) self->escapecount = 0;
        else self->escapecount++;
    }
    // New pain, fall, and death animations. Also, the nopain flag.
    if(self->drop || self->health <= 0)
    {
        // Drop Weapon due to death.
        if(self->modeldata.weaploss[0] <= 2) dropweapon(1);
        if(self->health <= 0 && self->modeldata.falldie == 1)
        {
            self->xdir = self->zdir = self->tossv = 0;
            set_death(self, self->damagetype, 0);
        }
        else
        {
            self->xdir = attack->dropv[1];
            self->zdir = attack->dropv[2];
            if(self->direction) self->xdir = -self->xdir;
            toss(self, attack->dropv[0]);
            self->damage_on_landing = attack->damage_on_landing;
            self->knockdowncount = self->modeldata.knockdowncount; // reset the knockdowncount
            self->knockdowntime = 0;

            // Now if no fall/die animations exist, entity simply disapears
            //set_fall(entity *iFall, int type, int reset, entity* other, int force, int drop)
            if(!set_fall(self, self->damagetype, 1, other, attack->attack_force, attack->attack_drop, attack->no_block, attack->guardcost, attack->jugglecost, attack->pause_add))
            {
                if(self->modeldata.type == TYPE_PLAYER) player_die();
                else kill(self);
                return 1;
            }
        }
        self->takeaction = common_fall;
        if(self->modeldata.type == TYPE_PLAYER) control_rumble(self->playerindex, attack->attack_force * 3);
    }
    else if(attack->grab && !attack->no_pain)
    {
        set_pain(self, self->damagetype, 0);
        other->stalltime = time + GRAB_STALL;
        self->releasetime = time + (GAME_SPEED/2);
        self->takeaction = common_pain;
        other->takeaction = common_grabattack;
    }
    // Don't change to pain animation if frozen
    else if(!self->frozen && !self->modeldata.nopain && !attack->no_pain && !(self->modeldata.defense_pain[(short)attack->attack_type] && attack->attack_force < self->modeldata.defense_pain[(short)attack->attack_type]))
    {
        set_pain(self, self->damagetype, 1);
        self->takeaction = common_pain;
    }
	return 1;
}

// A.I. try upper cut
int common_try_upper(entity* target)
{
    if(!validanim(self,ANI_UPPER)) return 0;


    if(!target) target = normal_find_target(ANI_UPPER);

    // Target jumping? Try uppercut!
    if(target && target->jumping )
    {
        set_attacking(self);
        self->zdir = self->xdir = 0;
        // Don't waste any time!
        ent_set_anim(self, ANI_UPPER, 0);
        self->takeaction = common_attack_proc;
        return 1;
    }
    return 0;
}

int common_try_runattack(entity* target)
{
    if(!self->running || !validanim(self,ANI_RUNATTACK)) return 0;


    if(!target) target = normal_find_target(ANI_RUNATTACK);

    if(target)
    {
        self->zdir = self->xdir = 0;
        set_attacking(self);
        ent_set_anim(self, ANI_RUNATTACK, 0);
        self->takeaction = common_attack_proc;
        return 1;
    }
    return 0;
}

int common_try_block(entity* target)
{
    if(self->modeldata.nopassiveblock==0 ||
       (rand32()&self->modeldata.blockodds) != 1 ||
       !validanim(self,ANI_BLOCK))
       return 0;

    if(!target) target = normal_find_target(ANI_BLOCK);

    // no passive block, so block by himself :)
    if(target && target->attacking)
    {
        set_blocking(self);
        self->zdir = self->xdir = 0;
        ent_set_anim(self, ANI_BLOCK, 0);
        self->takeaction = common_block;
        return 1;
    }
    return 0;
}
/*
int common_try_freespecial(entity* target)
{
    int i, s=max_freespecials;

    for(i=0; i<s; i++)
    {
        if(validanim(self,animspecials[i]) &&
           (check_energy(1, animspecials[i]) ||
            check_energy(0, animspecials[i])) &&
           (target || (target=normal_find_target(animspecials[i]))) &&
           (rand32()%s)<3 &&
           check_costmove(animspecials[i], 1)  )
        {
            return 1;
        }
    }

    return 0;
}*/

int common_try_normalattack(entity* target)
{
    int i, found = 0;

    for(i=0; !found && i<max_freespecials; i++)
    {
        if(validanim(self,animspecials[i]) &&
           (check_energy(1, animspecials[i]) ||
            check_energy(0, animspecials[i])) &&
           (target || (target=normal_find_target(animspecials[i]))) &&
           (rand32()%max_freespecials)<3)
        {
            found = 1;
        }
    }

    for(i=0; !found && i<max_attacks; i++) // TODO: recheck range for attacks chains
    {
        if(!validanim(self,animattacks[i])) continue;

        if(target || (target=normal_find_target(animattacks[i])))
        {
            found = 1;
        }
    }

    if(!found && validanim(self,ANI_THROWATTACK) &&
        self->weapent && self->weapent->modeldata.subtype == SUBTYPE_PROJECTILE &&
        (target || (target=normal_find_target( ANI_THROWATTACK))) )
    {
        found = 1;
    }

    if(found)
    {
        self->zdir = self->xdir = 0;
        set_idle(self);
        self->idling = 0; // not really idle, in fact it is thinking
        self->attacking = -1; // pre-attack, for AI-block check
        self->takeaction = normal_prepare;
        if(self->combostep[0] && self->combotime>time) self->stalltime = time+1;
        else self->stalltime = time + (GAME_SPEED/4) + (rand32()%(GAME_SPEED/10) - self->modeldata.aggression);
        return 1;
    }

    return 0;
}

int common_try_jumpattack(entity* target)
{
    entity* dust;
    int rnum ;

    if((validanim(self,ANI_JUMPATTACK) || validanim(self,ANI_JUMPATTACK2)))
    {
        if(!validanim(self,ANI_JUMPATTACK)) rnum = 1;
        else if(validanim(self,ANI_JUMPATTACK2) && (rand32()&1)) rnum = 1;
        else rnum = 0;

        if(rnum==0 &&
            // do a jumpattack
            (target || (target = normal_find_target(ANI_JUMPATTACK))) )
        {
            ent_set_anim(self, ANI_JUMPATTACK, 0);
            if(self->direction) self->xdir = (float)1.3;
            else self->xdir = (float)-1.3;
            self->zdir = 0;
        }
        else if(rnum==1 &&
            // do a jumpattack2
            (target || (target = normal_find_target(ANI_JUMPATTACK2))) )
        {
            ent_set_anim(self, ANI_JUMPATTACK2, 0);
            self->xdir = self->zdir = 0;
        } else {
            rnum = -1;
        }

        if(rnum >= 0)
        {

            set_attacking(self);
            self->jumping = 1;
            toss(self, self->modeldata.jumpheight);
            self->takeaction = common_jump;

            if(self->modeldata.dust[2]>=0)
            {
                dust = spawn(self->x, self->z, self->a, self->direction, NULL, self->modeldata.dust[2], NULL);
                dust->base = self->a;
                dust->autokill = 1;
                execute_onspawn_script(dust);
            }

            return 1;
        }
    }
    return 0;
}

// Normal attack style
// Used by root A.I., what to do if a target is found.
// return 0 if no action is token
// return 1 if an action is token
int normal_attack()
{
    //int rnum;

    //rnum = rand32()&7;
    if(common_try_upper(NULL) ||
       common_try_block(NULL) ||
       common_try_runattack(NULL) ||
       //(rnum < 2 && common_try_freespecial(NULL)) ||
       common_try_normalattack(NULL) ||
       common_try_jumpattack(NULL) )
    {
        self->running = 0;
        return 1;
    }
    return 0;// nothing to do? so go to next think step
}

// A.I. characters do a throw
void common_throw()
{
    if(self->animating) return; // just play the throw animation

    set_idle(self);

    // we have done the throw, return to A.I. root
    self->takeaction = NULL;
}

// toss the grabbed one
void dothrow()
{
    entity* other;
	self->xdir = self->zdir = 0;
    other = self->link;
    if(other == NULL) //change back to idle, or we will get stuck here
    {
        set_idle(self);
        self->takeaction = NULL;// A.I. root again
        return;
    }

    if(other->modeldata.throwheight) toss(other, other->modeldata.throwheight);
    else toss(other, other->modeldata.jumpheight);

    other->direction = self->direction;
    other->projectile = 2;
    other->xdir = (other->direction)?(-other->modeldata.throwdist):(other->modeldata.throwdist);

    if(autoland == 1 && validanim(other,ANI_LAND)) other->damage_on_landing = -1;
    else other->damage_on_landing = self->modeldata.throwdamage;

    set_fall(other, ATK_NORMAL, 0, self, 0, 0, 0, 0, 0, 0);
    ent_set_anim(self, ANI_THROW, 0);
    ent_unlink(other);

    other->takeaction = common_fall;
    self->takeaction = common_throw;
}


// Waiting until throw frame reached
void common_throw_wait()
{
    if(!self->link)
    {
        set_idle(self);
        self->takeaction = NULL;// A.I. root again
        return;
    }

    self->releasetime += THINK_SPEED; //extend release time

    if(self->animpos != self->modeldata.throwframewait) return;

    dothrow();
}


void common_prethrow()
{
    self->running = 0;    // Quits running if grabbed by opponent

    // Just check if we're still grabbed...
    if(self->link) return;

    set_idle(self);

    self->takeaction = NULL;// A.I. root again
}

// warp to its parent entity, just like skeletons in Diablo 2
void npc_warp()
{
    if(!self->parent) return;
    self->z = self->parent->z;
    self->x = self->parent->x;
    self->a = self->parent->a;
    self->xdir = self->zdir = 0;
    self->base = self->parent->base;
    self->tossv = 0;

    if(validanim(self,ANI_RESPAWN)) {
        ent_set_anim(self, ANI_RESPAWN, 0);
        self->takeaction = common_spawn;
    } else if(validanim(self,ANI_SPAWN)) {
        ent_set_anim(self, ANI_SPAWN, 0);
        self->takeaction = common_spawn;
    }
}

int adjust_grabposition(entity* ent, entity* other, float dist, int grabin)
{
    float x1, z1, x2, z2, a, x;
    int wall1, wall2;
    a = ent->a;
    if(grabin==1)
    {
        x1 = ent->x;
        z1 = z2 = ent->z;
        x2 = ent->x + ((other->x>ent->x)?dist:-dist);
    }
    else
    {
        x = (ent->x + other->x)/2;
        x1 = x + ((ent->x>=other->x)?(dist/2):(-dist/2));
        x2 = x + ((other->x>ent->x)?(dist/2):(-dist/2));
        z1 = z2 = (ent->z + other->z)/2;
    }

    if(ent->modeldata.subject_to_screen>0 && (x1<advancex || x1>advancex+videomodes.hRes)) return 0;
    else if(other->modeldata.subject_to_screen>0 && (x2<advancex || x2>advancex+videomodes.hRes)) return 0;

    wall1 = checkwall_below(x1, z1, 9999999);
    wall2 = checkwall_below(x2, z2, 9999999);
    if(wall1<0 && wall2>=0) return 0;
    else if(wall1>=0 && wall2<0) return 0;
    else if(wall1>=0 && level->walls[wall1][7] >a) return 0;
    else if(wall2>=0 && level->walls[wall2][7] >a) return 0;
    else if(wall1>=0 && wall2>=0 && level->walls[wall1][7] != level->walls[wall2][7]) return 0;

    if(wall1<0 && checkhole(x1, z1)) return 0;
    if(wall2<0 && checkhole(x2, z2)) return 0;

    ent->x = x1; ent->z = z1;
    other->x = x2; other->z = z2;
    //other->a = ent->a;
    //other->base = ent->base;
    return 1;
}


int common_trymove(float xdir, float zdir)
{
    entity *other = NULL;
    int wall, heightvar;
    float x, z, oxdir, ozdir;

    if(!xdir && !zdir) return 0;

    oxdir = xdir; ozdir = zdir;
    /*
    // entity is grabbed by other
    if(self->link && self->link->grabbing==self && self->link->grabwalking)
    {
        return 1; // just return so we don't have to check twice
    }*/

    x = self->x + xdir;  z = self->z + zdir;
    // -----------bounds checking---------------
    // Subjec to Z and out of bounds? Return to level!
    if (self->modeldata.subject_to_minz>0)
    {
        if(zdir && z < PLAYER_MIN_Z)
        {
            zdir = PLAYER_MIN_Z - self->z;
            execute_onblockz_script(self);
        }
    }

    if (self->modeldata.subject_to_maxz>0)
    {
        if(zdir && z > PLAYER_MAX_Z)
        {
            zdir = PLAYER_MAX_Z - self->z;
            execute_onblockz_script(self);
        }
    }

    // End of level is blocked?
    if(level->exit_blocked)
    {
        if(x > level->width-30-(PLAYER_MAX_Z-z))
        {
            xdir = level->width-30-(PLAYER_MAX_Z-z) - self->x;
        }
    }
    // screen checking
    if(self->modeldata.subject_to_screen>0)
    {
        if(x < advancex+10)
        {
            xdir = advancex+10 - self->x;
            execute_onblocks_script(self);  //Screen block event.
        }
        else if(x > advancex+(videomodes.hRes-10))
        {
            xdir = advancex+(videomodes.hRes-10) - self->x;
            execute_onblocks_script(self);  //Screen block event.
        }
    }

    if(!xdir && !zdir) return 0;
    x = self->x + xdir;  z = self->z + zdir;

    //-----------end of bounds checking-----------

    //-------------hole checking ---------------------
    // Don't walk into a hole or walk off platforms into holes
    if( self->modeldata.type!=TYPE_PLAYER && self->idling &&
        (!self->animation->seta||self->animation->seta[self->animpos]<0) &&
        self->modeldata.subject_to_hole>0 && !inair(self) &&
        !(self->modeldata.aimove&AIMOVE2_IGNOREHOLES))
    {
        if(xdir && checkhole(x, self->z) && checkwall(x, self->z)<0 && (((other = check_platform(x, self->z)) && self->base < other->a + other->animation->platform[other->animpos][7]) || other == NULL))
        {
            xdir = 0;
        }
        if(zdir && checkhole(self->x, z) && checkwall(self->x, z)<0 && (((other = check_platform(self->x, z)) &&  self->base < other->a + other->animation->platform[other->animpos][7]) || other == NULL))
        {
            zdir = 0;
        }
    }

    if(!xdir && !zdir) return 0;
    x = self->x + xdir;  z = self->z + zdir;
    //-----------end of hole checking---------------

    //--------------obstacle checking ------------------
    if(self->modeldata.subject_to_obstacle>0 /*&& !inair(self)*/)
    {
        if((other = find_ent_here(self, x, self->z, (TYPE_OBSTACLE | TYPE_TRAP))) &&
           (xdir>0 ? other->x > self->x: other->x < self->x) &&
           (!other->animation->platform||!other->animation->platform[other->animpos][7]))
            {
                xdir    = 0;
                execute_onblocko_script(self, other);
            }
        if((other = find_ent_here(self, self->x, z, (TYPE_OBSTACLE | TYPE_TRAP))) &&
           (zdir>0 ? other->z > self->z: other->z < self->z) &&
           (!other->animation->platform||!other->animation->platform[other->animpos][7]))
            {
                zdir    = 0;
                execute_onblocko_script(self, other);
            }
    }

    if(!xdir && !zdir) return 0;
    x = self->x + xdir*2;  z = self->z + zdir*2;

    //-----------end of obstacle checking--------------

    // ---------------- platform checking----------------

    if(self->animation->height) heightvar = self->animation->height;
    else heightvar = self->modeldata.height;

    // Check for obstacles with platform code and adjust base accordingly
    if(self->modeldata.subject_to_platform>0 && (other = check_platform_between(x, z, self->a, self->a+heightvar)) )
    {
        if(xdir>0 ? other->x>self->x : other->x<self->x) {xdir = 0; }
        if(zdir>0 ? other->z>self->z : other->z<self->z) {zdir = 0; }
    }

    if(!xdir && !zdir) return 0;
    x = self->x + xdir;  z = self->z + zdir;

    //-----------end of platform checking------------------

    // ------------------ wall checking ---------------------
    if(self->modeldata.subject_to_wall){
        if((wall = checkwall(x, self->z))>=0 && level->walls[wall][7]>self->a)
        {
            if(xdir > 0.5){ xdir = 0.5; }
            else if(xdir < -0.5){ xdir = -0.5; }
            if((wall = checkwall(self->x + xdir, self->z))>=0 && level->walls[wall][7]>self->a){
                xdir = 0;
                execute_onblockw_script(self,1,(double)level->walls[wall][7]);
            }
        }
        if((wall = checkwall(self->x, z))>=0 && level->walls[wall][7]>self->a)
        {
            if(zdir > 0.5){ zdir = 0.5; }
            else if(zdir < -0.5){ zdir = -0.5; }
            if((wall = checkwall(self->x, self->z + zdir))>=0 && level->walls[wall][7]>self->a){
                zdir = 0;
                execute_onblockw_script(self,2,(double)level->walls[wall][7]);
            }
        }
    }


    if(!xdir && !zdir) return 0;
    x = self->x + xdir;  z = self->z + zdir;
    //----------------end of wall checking--------------

    //------------------ grab/throw checking------------------
    if( (validanim(self,ANI_THROW) ||
         validanim(self,ANI_GRAB)) && self->idling &&
        (other = find_ent_here(self,x, z, self->modeldata.hostile))&&
        cangrab(self, other) &&
        adjust_grabposition(self, other, self->modeldata.grabdistance, 0))
    {
        self->direction = (self->x < other->x);

        set_opponent(other, self);
        ents_link(self, other);
        other->attacking = 0;
        self->idling = 0;
        self->running = 0;

        self->xdir = self->zdir =
        other->xdir = other->zdir = 0;
        if(validanim(self,ANI_GRAB))
        {
            other->direction = !self->direction;
            ent_set_anim(self, ANI_GRAB, 0);
            set_pain(other, -1, 0); //set grabbed animation
            self->attacking = 0;
            memset(self->combostep, 0, 5*sizeof(int));
            other->takeaction = common_grabbed;
            self->takeaction = common_grab;
            other->stalltime = time + GRAB_STALL;
            self->releasetime = time + (GAME_SPEED/2);
        }
        // use original throw code if throwframewait not present, kbandressen 10/20/06
        else if(self->modeldata.throwframewait == -1)
            dothrow();
        // otherwise enemy_throw_wait will be used, kbandressen 10/20/06
        else
        {
            other->direction = !self->direction;
            ent_set_anim(self, ANI_THROW, 0);
            set_pain(other, -1, 0); // set grabbed animation

            other->takeaction = common_prethrow;
            self->takeaction = common_throw_wait;
        }
        return 0;
    }
    // ---------------  end of grab/throw checking ------------------------

    // do move and return
    self->x += xdir;
    self->z += zdir;

    if(xdir)    execute_onmovex_script(self);   //X move event.
    if(zdir)    execute_onmovez_script(self);   //Z move event.
    return 2-(xdir==oxdir && zdir ==ozdir);     // return 2 for some checks
}

// enemies run off after attack
void common_runoff()
{
    entity *target = normal_find_target(-1);

    if(target == NULL) return;    // There are no players?
    if(!self->modeldata.noflip) self->direction = (self->x < target->x);
    if(self->direction) self->xdir = -self->modeldata.speed/2;
    else self->xdir = self->modeldata.speed/2;

    self->zdir = 0;

    adjust_walk_animation(target);

    if(time > self->stalltime) self->takeaction = NULL; // OK, back to A.I. root
}


void common_stuck_underneath()
{
    if(player[(int)self->playerindex].keys & FLAG_MOVELEFT)
    {
        player[(int)self->playerindex].playkeys -= FLAG_MOVELEFT;
        self->direction = 0;
    }
    else if(player[(int)self->playerindex].keys & FLAG_MOVERIGHT)
    {
        player[(int)self->playerindex].playkeys -= FLAG_MOVERIGHT;
        self->direction = 1;
    }
    if(player[(int)self->playerindex].keys & FLAG_ATTACK && validanim(self,ANI_DUCKATTACK))
    {
        player[(int)self->playerindex].playkeys -= FLAG_ATTACK;
        set_attacking(self);
        self->xdir = self->zdir = 0;
        self->combostep[0] = 0;
        self->running = 0;
        ent_set_anim(self, ANI_DUCKATTACK, 0);
        self->takeaction = common_attack_proc;
        return;
    }
    if((player[(int)self->playerindex].keys & FLAG_MOVEDOWN) && (player[(int)self->playerindex].keys & FLAG_JUMP) && validanim(self,ANI_SLIDE))
    {
        player[(int)self->playerindex].playkeys -= FLAG_MOVEDOWN;
        player[(int)self->playerindex].playkeys -= FLAG_JUMP;
        set_attacking(self);
        self->xdir = self->zdir = 0;
        self->combostep[0] = 0;
        self->running = 0;
        ent_set_anim(self, ANI_SLIDE, 0);
        self->takeaction = common_attack_proc;
        return;
    }
    if(!check_platform_between(self->x, self->z, self->a, self->a+self->modeldata.height) )
    {
        set_idle(self);
        self->takeaction = NULL;
        return;
    }
}


// finish attacking, do something
void common_attack_finish()
{
    entity *target;

    self->xdir = self->zdir = 0;

    if(self->modeldata.type == TYPE_PLAYER)
    {
        set_idle(self);
        self->takeaction = NULL;
        return;
    }

    target = normal_find_target(-1);

    if(target && !self->modeldata.nomove && diff(self->x, target->x)<80 && (rand32()&3))
    {
        common_walk_anim(self);
        //ent_set_anim(self, ANI_WALK, 0);
        self->idling = 1;
        self->takeaction = common_runoff;
    }
    else
    {
        set_idle(self);
        self->takeaction = NULL;
    }

    self->stalltime = time + GAME_SPEED - self->modeldata.aggression;
}


//while playing attack animation
void common_attack_proc()
{

    if(self->animating || diff(self->a, self->base)>=4) return;

    if(self->tocost)
    {    // Enemy was hit with a special so go ahead and subtract life
        if(check_energy(1, self->animnum))
        {
            self->mp -= self->animation->energycost[0];
        }
        else self->health -= self->animation->energycost[0];
        self->tocost = 0;    // Life is subtracted, so go ahead and reset the flag
    }

    if(self == smartbomber)
    {    // Player is done with the special animation, so unfreeze and execute a smart bomb
        smart_bomb(self, self->modeldata.smartbomb);
        smartbomber = NULL;
    }
    if(self->reactive == 1)
    {
        subtract_shot();
        self->reactive=0;
    }
    self->attacking = 0;
    // end of attack proc
    common_attack_finish();
}


// dispatch A.I. attack
int common_attack()
{
    int aiattack ;

    if(self->modeldata.aiattack==-1) return 0;

    aiattack = self->modeldata.aiattack & MASK_AIATTACK1;

    switch(aiattack)
    {
    case AIATTACK1_LONG:
    case AIATTACK1_MELEE:
    case AIATTACK1_NOATTACK:
        return 0;
    default:                    // this is the only available attack style by now
        return inair(self)?0:normal_attack();
    }
}

//maybe used many times, so make a function
// A.I. characters will check if there's a wall infront, and jump onto it if possible
// return 1 if jump
int common_try_jump()
{
    float xdir, zdir;
    int wall, j=0;
    float rmin, rmax;

	if(validanim(self,ANI_JUMP) || validanim(self,ANI_FORWARDJUMP) || validanim(self, ANI_RUNJUMP))		//Can jump?
	{
		//Check to see if there is a wall within jumping distance and within a jumping height
		xdir = 0; wall = -1;
		rmin = (float)self->modeldata.animation[ANI_JUMP]->range[0];
		rmax = (float)self->modeldata.animation[ANI_JUMP]->range[1];
		if(self->direction) xdir = self->x + rmin;
		else xdir = self->x - rmin;
		//check z jump
		if(self->modeldata.jumpmovez) zdir = self->z + self->zdir;
		else zdir = self->z;

		if( (wall = checkwall_below(xdir, zdir, 999999)) >= 0 &&
			level->walls[wall][7] <= self->a + rmax &&
			!inair(self) && self->a < level->walls[wall][7]  )
		{
			j=1;
		}
		else if(checkhole(self->x + (self->direction?2:-2), zdir) &&
				checkwall(self->x + (self->direction?2:-2), zdir)<0 &&
				!checkhole(self->x + (self->direction?rmax:-rmax), zdir))
		{
			j=1;
		}
	}

	/*
	Damon V. Caskey
	03292010
	AI can will check its RUNJUMP range if JUMP can't reach. Code is pretty redundant,
	can probably be moved to a function later.
	*/
	if(!j && validanim(self, ANI_RUNJUMP))														//Jump check failed and can run jump?
	{
		//Check for wall in range of RUNJUMP.
		xdir = 0; wall = -1;
		rmin = (float)self->modeldata.animation[ANI_RUNJUMP]->range[0];
		rmax = (float)self->modeldata.animation[ANI_RUNJUMP]->range[1];
		if(self->direction) xdir = self->x + rmin;
		else xdir = self->x - rmin;
		//check z jump
		if(self->modeldata.jumpmovez) zdir = self->z + self->zdir;
		else zdir = self->z;

		if( (wall = checkwall_below(xdir, zdir, 999999)) >= 0 &&
			level->walls[wall][7] <= self->a + rmax &&
			!inair(self) && self->a < level->walls[wall][7]  )
		{
			j=2;																				//Set to perform runjump.
		}
		//Check for pit in range of RUNJUMP.
		else if(checkhole(self->x + (self->direction?2:-2), zdir) &&
				checkwall(self->x + (self->direction?2:-2), zdir)<0 &&
				!checkhole(self->x + (self->direction?rmax:-rmax), zdir))
		{
			j=2;																				//Set to perform runjump.
		}
	}

    if(j)
    {
        if(self->running || j==2)
        {
            if(validanim(self,ANI_RUNJUMP))														//Running or only within range of RUNJUMP?
                tryjump(self->modeldata.runjumpheight, self->modeldata.jumpspeed*self->modeldata.runjumpdist, (self->modeldata.jumpmovez)?self->zdir:0, ANI_RUNJUMP);
            else if(validanim(self,ANI_FORWARDJUMP))
                tryjump(self->modeldata.runjumpheight, self->modeldata.jumpspeed*self->modeldata.runjumpdist, (self->modeldata.jumpmovez)?self->zdir:0, ANI_FORWARDJUMP);
            else
                tryjump(self->modeldata.runjumpheight, self->modeldata.jumpspeed*self->modeldata.runjumpdist, (self->modeldata.jumpmovez)?self->zdir:0, ANI_JUMP);
        }
        else
        {
            if(validanim(self,ANI_FORWARDJUMP))
                tryjump(self->modeldata.jumpheight, self->modeldata.jumpspeed, (self->modeldata.jumpmovez)?self->zdir:0, ANI_FORWARDJUMP);
            else
                tryjump(self->modeldata.jumpheight, self->modeldata.jumpspeed, (self->modeldata.jumpmovez)?self->zdir:0, ANI_JUMP);
        }

		return 1;
    }
    return 0;
}

void adjust_walk_animation(entity* other)
{
    if(self->running)
    {
        ent_set_anim(self, ANI_RUN, 0);
        return;
    }

    //reset the walk animation
    if( ((!other && self->zdir<0)||(other && self->z>other->z)) && validanim(self,ANI_UP))
        common_up_anim(self); //ent_set_anim(self, ANI_UP, 0);
    else if(((!other && self->zdir>0)||(other && other->z > self->z)) && validanim(self,ANI_DOWN))
        common_down_anim(self); //ent_set_anim(self, ANI_DOWN, 0);
    else if((self->direction ? self->xdir<0 : self->xdir>0) && validanim(self,ANI_BACKWALK))
        common_backwalk_anim(self); //ent_set_anim(self, ANI_BACKWALK, 0);
    else common_walk_anim(self); //ent_set_anim(self, ANI_WALK, 0);

    if((self->direction ? self->xdir<0 : self->xdir>0) && self->animnum!=ANI_BACKWALK)
        self->animating = -1;
    else self->animating = 1;
}

//may be used many times, so make a function
// try to move towards the item
int common_try_pick(entity* other)
{
    // if there's an item to pick up, move towards it.
    float maxspeed = self->modeldata.speed*1.5;
    float dx = diff(self->x, other->x);
    float dz = diff(self->z, other->z);

    if(other == NULL || self->modeldata.nomove) return 0;

    if(!dz && !dx) self->xdir = self->zdir = 0;
    else {
        self->xdir = maxspeed * dx / (dx+dz);
        self->zdir = maxspeed * dz / (dx+dz);
    }
    if(self->x > other->x) self->xdir = -self->xdir;
    if(self->z > other->z) self->zdir = -self->zdir;

    self->running = 0;

    adjust_walk_animation(other);

    return 1;
}

void checkpathblocked()
{
    if(self->stalltime>=time)
    {
        if(self->pathblocked>10)
        {
            self->xdir = self->zdir = 0;
            set_idle(self);
            self->pathblocked = 0;
            self->stalltime = time + GAME_SPEED/4;
        }
    }
}

//may be used many times, so make a function
// try to move towards the target
int common_try_chase(entity* target)
{
    // start chasing the target
    float maxspeed;
    float dx, dz;
    int aitype;
    int rnum;

    if(target == NULL || self->modeldata.nomove) return 0;

    dx = diff(self->x, target->x);
    dz = diff(self->z, target->z);

    aitype = self->modeldata.aimove & rand32();
    if(!aitype) aitype = self->modeldata.aimove;
    if(self->modeldata.subtype==SUBTYPE_CHASE) aitype |= AIMOVE1_CHASE;

    if((aitype & AIMOVE1_CHASEX) && dx < 20) aitype -= AIMOVE1_CHASEX;
    if((aitype & AIMOVE1_CHASEZ) && dz < 10) aitype -= AIMOVE1_CHASEZ;
    if((aitype & AIMOVE1_CHASE) && dz + dx < 20 ) aitype -= AIMOVE1_CHASE;

    if(!(aitype & (AIMOVE1_CHASEX|AIMOVE1_CHASEZ|AIMOVE1_CHASE))) return 0; // none available, exit

    rnum = rand32();
    // if target is far away, run instead of walk
    if( (self->direction?self->x<target->x:self->x>target->x) && (
        ((aitype & AIMOVE1_CHASEX) && (dx > 200 || (rnum&15)<3) && validanim(self,ANI_RUN))||
        ((dx+dz > 200 || (rnum&15)<3) && !(aitype & AIMOVE1_CHASEZ) && dx > 2 * dz && validanim(self,ANI_RUN))) )
         {maxspeed = self->modeldata.runspeed;self->running = 1;}
    else {maxspeed = self->modeldata.speed;self->running = 0;}

    if(!dz && !dx) self->xdir = self->zdir = 0;
    else if(aitype & AIMOVE1_CHASEX) { // wander in z direction, chase in x direction
        self->xdir = maxspeed ;
        if(self->modeldata.runupdown || !self->running)
        {
            if(diff(target->z, self->z) > videomodes.vRes/3)
                self->zdir = (target->z>self->z)?self->modeldata.speed/2:-self->modeldata.speed/2;
            else self->zdir = randf(1) - randf(1);
        }
    }
    else if(aitype & AIMOVE1_CHASEZ){// wander in x direction, chase in z direction
        if(diff(target->x, self->x) > videomodes.hRes/2.5)
            self->xdir = (target->x>self->x)?self->modeldata.speed:-self->modeldata.speed;
        else self->xdir = randf(1) - randf(1);
        self->zdir = maxspeed / 2;
    }
    else { // chase in x and z direction
        if(self->modeldata.runupdown || !self->running)
        {
            self->xdir = maxspeed * dx / (dx+dz);
            self->zdir = maxspeed * dz / (dx+dz);
        }
        else
        {
            self->xdir = maxspeed;
            self->zdir = 0;
        }
    }
    if(self->x > target->x && !(aitype & AIMOVE1_CHASEZ)) self->xdir = -self->xdir;
    if(self->z > target->z && !(aitype & AIMOVE1_CHASEX)) self->zdir = -self->zdir;

    adjust_walk_animation(target);

    return 1;
}

//may be used many times, so make a function
// minion follow his owner
int common_try_follow()
{
    // start chasing the target
    entity* target = NULL;
    float distance = 0;
    float maxspeed, dx, dz;

    target = self->parent;
    if(target == NULL || self->modeldata.nomove) return 0;

    dx = diff(self->x, target->x);
    dz = diff(self->z, target->z);
    distance = (float)((validanim(self,ANI_IDLE))? self->modeldata.animation[ANI_IDLE]->range[0]: 100);

    if(dz + dx < distance ) return 0;

    // if target is far away, run instead of walk
    if(dx+dz > 200 && dx > 2 * dz && validanim(self,ANI_RUN)){maxspeed = self->modeldata.runspeed;self->running = 1;}
    else {maxspeed = self->modeldata.speed;self->running = 0;}

    if(!dz && !dx) self->xdir = self->zdir = 0;
    else {
        if(self->modeldata.runupdown || !self->running)
        {
            self->xdir = maxspeed * dx / (dx+dz);
            self->zdir = maxspeed * dz / (dx+dz);
        }
        else
        {
            self->xdir = maxspeed;
            self->zdir = 0;
        }
    }

    if(self->x > target->x ) self->xdir = -self->xdir;
    if(self->z > target->z ) self->zdir = -self->zdir;

    adjust_walk_animation(target);

    return 1;
}

// try to avoid the target
// used by 'avoid avoidz avoidx
int common_try_avoid(entity* target)
{
    float maxspeed = 0;
    float dx, dz;

    int aitype = 0;

    if(target == NULL || self->modeldata.nomove) return 0;

    dx = diff(self->x, target->x);
    dz = diff(self->z, target->z);

    aitype = self->modeldata.aimove & rand32();
    if(!aitype) aitype = self->modeldata.aimove ;

    if((aitype & AIMOVE1_AVOIDX) && dx > 100) aitype -= AIMOVE1_AVOIDX;
    if((aitype & AIMOVE1_AVOIDZ) && dz > 50) aitype -= AIMOVE1_AVOIDZ;
    if((aitype & AIMOVE1_AVOID) && dz + dx > 150) aitype -= AIMOVE1_AVOID;

    if(!(aitype &(AIMOVE1_AVOIDX|AIMOVE1_AVOIDZ|AIMOVE1_AVOIDX))) return 0; // none available, exit

    maxspeed = self->modeldata.speed;

    if(self->x < advancex - 10) self->xdir = maxspeed;
    else if(self->x > advancex + videomodes.hRes + 10) self->xdir = -maxspeed;
    else self->xdir = (self->x < target->x)? (-maxspeed):maxspeed;

    if((level->scrolldir != SCROLL_UP && level->scrolldir != SCROLL_DOWN && self->z < advancey - 5) ||
        ((level->scrolldir == SCROLL_UP || level->scrolldir == SCROLL_DOWN ) && self->z < -5) ){
            self->zdir = maxspeed/2;
        }
    else if((level->scrolldir != SCROLL_UP && level->scrolldir != SCROLL_DOWN && self->z > advancey + videomodes.vRes + 5)||
        ((level->scrolldir == SCROLL_UP || level->scrolldir == SCROLL_DOWN ) && self->z > videomodes.vRes + 5) ){
            self->zdir = -maxspeed/2;
        }
    else self->zdir = (self->z < target->z)? (-maxspeed/2):(maxspeed/2);

    adjust_walk_animation(target);

    return 1;
}

//  wander completely
int common_try_wandercompletely()
{
    int walk = 0;
    int rnum = 0;

    if(self->modeldata.nomove) return 0;

    walk = 0;
    rnum = rand32();
    self->xdir = self->zdir = 0;
    if((rnum & 15) < 4){
        // Move up
        self->zdir = -self->modeldata.speed/2;
    }
    else if((rnum & 15) > 11){
        // Move down
        self->zdir = self->modeldata.speed/2;
    }

    rnum = rand32();
    if((rnum & 15) < 4 ){
        // Walk forward
        if(self->direction==1) self->xdir = self->modeldata.speed;
        else self->xdir = -self->modeldata.speed;
    }
    else if((rnum & 15) > 11 ){
        if(self->modeldata.noflip){
            // Walk backward
            if(self->direction==1) self->xdir = -self->modeldata.speed;

            else self->xdir = self->modeldata.speed;
        } else if(!validanim(self,ANI_TURN))  {
            // flip and Walk forward
            self->direction = !self->direction;
            if(self->direction==1) self->xdir = self->modeldata.speed;
            else self->xdir = -self->modeldata.speed;
        } else self->direction = !self->direction;
    }

    if(self->x < advancex - 10) {
        self->xdir = self->modeldata.speed;
    }
    else if(self->x > advancex + videomodes.hRes + 10) {
        self->xdir = -self->modeldata.speed;
    }

    if(((self->xdir > 0 && !self->direction) ||
        (self->xdir < 0 && self->direction)) && !self->modeldata.noflip)
        self->direction = !self->direction;

    if((level->scrolldir != SCROLL_UP && level->scrolldir != SCROLL_DOWN && self->z < advancey - 5) ||
        ((level->scrolldir == SCROLL_UP || level->scrolldir == SCROLL_DOWN ) && self->z < -5) ){
            self->zdir = self->modeldata.speed/2;
        }
    else if((level->scrolldir != SCROLL_UP && level->scrolldir != SCROLL_DOWN && self->z > advancey + videomodes.vRes + 5)||
        ((level->scrolldir == SCROLL_UP || level->scrolldir == SCROLL_DOWN ) && self->z > videomodes.vRes + 5) ){
            self->zdir = -self->modeldata.speed/2;
        }

        if(self->xdir || self->zdir){
            if((self->xdir > 0 && !self->direction) ||
                (self->xdir < 0 && self->direction))
                walk = -1;
            else walk = 1;
        }

        if(walk ){
            adjust_walk_animation(NULL);

            return 1;
        }
        return 0;
}

// just wander around, face to the target
int common_try_wander(entity* target)
{
    int walk = 0;
    int rnum = rand32()&7;

    if(target == NULL || self->modeldata.nomove) return 0;
    // Decide next direction... randomly.

    self->xdir = self->zdir = 0;

    if(diff(self->x, target->x)>videomodes.hRes/2)
    {
        self->xdir = (self->x>target->x)?-self->modeldata.speed:self->modeldata.speed;;
        walk = 1;
    }
    else if(rnum < 3){
        self->xdir = self->modeldata.speed;
        walk = 1;
    }
    else if(rnum > 4){
        self->xdir = -self->modeldata.speed;
        walk = 1;
    }

    rnum = rand32()&7;
    if(rnum < 2){
        // Move up
        self->zdir = -self->modeldata.speed/2;
        walk |= 1;
    }
    else if(rnum > 5){
        // Move down
        self->zdir = self->modeldata.speed/2;
        walk |= 1;
    }

    if(walk ){
        adjust_walk_animation(target);
    }
    else
    {
        self->xdir = self->zdir = 0;
        set_idle(self);
    }

    return 1;
}

//A.I chracter pickup an item
void common_pickupitem(entity* other){
    int pickup = 0;
    //weapons
    if(self->weapent == NULL &&
        other->modeldata.subtype == SUBTYPE_WEAPON
        && validanim(self,ANI_GET))
    {
        dropweapon(0);  //don't bother dropping the previous one though, scine it won't pickup another
        self->weapent = other;
        set_weapon(self, other->modeldata.weapnum, 0);
        ent_set_anim(self, ANI_GET, 0);
        if(self->modeldata.animal)  // UTunnels: well, ride, not get. :)
        {
            self->direction = other->direction;
            self->x = other->x;
            self->z = other->z;
        }
        set_getting(self);
        self->takeaction = common_get;
        self->xdir=self->zdir=0;//stop moving
        pickup = 1;
    }
    // projectiles
    else if(self->weapent == NULL &&
        other->modeldata.subtype == SUBTYPE_PROJECTILE
        && validanim(self,ANI_GET))
    {
        dropweapon(0);
        self->weapent = other;
        ent_set_anim(self, ANI_GET, 0);
        set_getting(self);
        self->takeaction = common_get;
        self->xdir=self->zdir=0;//stop moving
        pickup = 1;
    }
    // other items
    else if(other->modeldata.subtype != SUBTYPE_WEAPON &&
        other->modeldata.subtype != SUBTYPE_PROJECTILE     )
    {
        if(validanim(self,ANI_GET) && other->modeldata.subtype!=SUBTYPE_TOUCH)
        {
            ent_set_anim(self, ANI_GET, 0);
            set_getting(self);
            self->takeaction = common_get;
            self->xdir=self->zdir=0;//stop moving
        }
        if(other->health){
            self->health += other->health;
            if(self->health > self->modeldata.health) self->health = self->modeldata.health;
            other->health = 0;
            //if(SAMPLE_GET >= 0) sound_play_sample(SAMPLE_GET, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        // else if, TODO: other effects
        // kill that item
        other->takeaction = suicide;
        other->nextthink = time + GAME_SPEED * 3;
        pickup = 1;
    }
    // hide it
    if(pickup)
    {
        other->z = 100000;
        execute_didhit_script(other, self, 0, 0, other->modeldata.subtype, 0, 0, 0, 0, 0); //Execute didhit script as if item "hit" collecter to allow easy item scripting.
    }
}

//walk/run/pickup/jump etc
//Used by normal A.I. pattern
int normal_move()
{
    entity* other = NULL; //item
    entity* target = NULL;//hostile target
    entity* owner = NULL;
    float seta;
    int predir ;

    predir = self->direction;

    target = normal_find_target(-1); // confirm the target again
    other = normal_find_item();    // find an item
    owner = self->parent;

    if(!self->modeldata.noflip && !self->running){
        if(other)   //try to pick up an item, if possible
            self->direction = (self->x < other->x);
        else if(target)
            self->direction = (self->x < target->x);
        else if(owner)
            self->direction = (self->x < owner->x);
    }

    //turn back if we have a turn animation
    if(self->direction != predir && validanim(self,ANI_TURN)){
        self->direction = !self->direction;
        ent_set_anim(self, ANI_TURN, 0);
        self->xdir = self->zdir = 0;
        self->takeaction = common_turn;
        return 1;
    }

    if(common_try_jump()) return 1;  //need to jump? so quit

    checkpathblocked();

    // judge next move if stalltime is expired
    if(self->stalltime < time ){
        if(other){
            // try walking to the item
            common_try_pick(other);
        }
        else if(target && (self->modeldata.subtype == SUBTYPE_CHASE ||
            (self->modeldata.type == TYPE_NPC && self->parent))){
                // try chase a target
                common_try_chase(target);
            }
        else if(target){
            // just wander around
            common_try_wander(target);
        } else if(!common_try_follow()){
            // no target or item, just relex and idle
            self->xdir = self->zdir = 0;
            set_idle(self);
        }
        //end of if

        self->stalltime = time + GAME_SPEED - self->modeldata.aggression;
    }

    //pick up the item if possible
    if( (other && other == find_ent_here(self, self->x, self->z, TYPE_ITEM)) && other->animation->vulnerable[other->animpos])//won't pickup an item that is not previous one
    {
        seta = (float)(self->animation->seta?self->animation->seta[self->animpos]:-1);
        if(diff(self->a - (seta>= 0) * seta , other->a)<0.1)
        common_pickupitem(other);
    }

    return 1;
}

//walk/run/pickup/jump etc
//Used by avoid A.I. pattern
int avoid_move()
{

    entity* other = NULL; //item
    entity* target = NULL;//hostile target
    entity* owner = NULL;
    float seta;
    int predir ;

    predir = self->direction;
    target = normal_find_target(-1); // confirm the target again
    other = normal_find_item();    // find an item
    owner = self->parent;

    if(!self->modeldata.noflip && !self->running){
        if(target)
            self->direction = (self->x < target->x);
        else if(other)   //try to pick up an item, if possible
            self->direction = (self->x < other->x);
        else if(owner)
            self->direction = (self->x < owner->x);
    }

    //turn back if we have a turn animation
    if(self->direction != predir && validanim(self,ANI_TURN)){
        self->direction = !self->direction;
        ent_set_anim(self, ANI_TURN, 0);
        self->xdir = self->zdir = 0;
        self->takeaction = common_turn;
        return 1;
    }

    if(common_try_jump()) return 1;  //need to jump? so quit

    checkpathblocked();

    // judge next move if stalltime is expired
    if(self->stalltime < time ){
        if(target != NULL && (other == NULL ||
            (other && diff(self->x, other->x) + diff(self->z, other->z) >
            diff(self->x, target->x)+ diff(self->z, target->z)))){
                // avoid the target
                if(!common_try_avoid(target))
                    common_try_wander(target);

            }
        else if(other){
            // try walking to the item
            common_try_pick(other);
        } else if(!common_try_follow()){
            // no target or item, just relex and idle
            self->xdir = self->zdir = 0;
            set_idle(self);
        }
        //end of if

        self->stalltime = time + GAME_SPEED - self->modeldata.aggression;
    }

    //pick up the item if possible
    if( (other && other == find_ent_here(self, self->x, self->z, TYPE_ITEM)) && other->animation->vulnerable[other->animpos])//won't pickup an item that is not previous one
    {
        seta = (float)(self->animation->seta?self->animation->seta[self->animpos]:-1);
        if(diff(self->a - (seta>= 0) * seta , other->a)<0.1)
        common_pickupitem(other);
    }

    return 1;
}

//walk/run/pickup/jump etc
//Used by chase A.I. pattern
int chase_move()
{

    entity* other = NULL; //item
    entity* target = NULL;//hostile target
    entity* owner = NULL;
    float seta;
    int predir ;

    predir = self->direction;
    target = normal_find_target(-1); // confirm the target again
    other = normal_find_item();    // find an item
    owner = self->parent;

    if(!self->modeldata.noflip && !self->running){
        if(target)
            self->direction = (self->x < target->x);
        else if(other)   //try to pick up an item, if possible
            self->direction = (self->x < other->x);
        else if(owner)
            self->direction = (self->x < owner->x);
    }

    //turn back if we have a turn animation
    if(self->direction != predir && validanim(self,ANI_TURN)){
        self->direction = !self->direction;
        ent_set_anim(self, ANI_TURN, 0);
        self->xdir = self->zdir = 0;
        self->takeaction = common_turn;
        return 1;
    }

    if(common_try_jump()) return 1;  //need to jump? so quit

    checkpathblocked();

    // judge next move if stalltime is expired
    if(self->stalltime < time ){
        if(target != NULL && (other == NULL ||
            (other && diff(self->x, other->x) + diff(self->z, other->z) >
            diff(self->x, target->x)+ diff(self->z, target->z)))){
                // chase the target
                if(!common_try_chase(target))
                    common_try_wander(target);

            }
        else if(other){
            // try walking to the item
            common_try_pick(other);
        } else if(!common_try_follow()){
            // no target or item, just relex and idle
            self->xdir = self->zdir = 0;
            set_idle(self);
        }
        //end of if

        self->stalltime = time + GAME_SPEED - self->modeldata.aggression;
    }


    //pick up the item if possible
    if( (other && other == find_ent_here(self, self->x, self->z, TYPE_ITEM)) && other->animation->vulnerable[other->animpos])//won't pickup an item that is not previous one
    {
        seta = (float)(self->animation->seta?self->animation->seta[self->animpos]:-1);
        if(diff(self->a - (seta>= 0) * seta , other->a)<0.1)
        common_pickupitem(other);
    }

    return 1;
}

//Used by wander A.I. pattern
// these guys ignore target's position, wandering around
int wander_move()
{

    entity* other = NULL; //item
    //entity* target = NULL;//hostile target
    float seta;
    int predir;

    predir = self->direction;
    //target = normal_find_target(); // confirm the target again
    other = normal_find_item();    // find an item

    if(!self->modeldata.noflip && other && !self->running){
        //try to pick up an item, if possible
        // just ignore target, but will still chase item
        self->direction = (self->x < other->x);
    }

    //turn back if we have a turn animation
    if(self->direction != predir && validanim(self,ANI_TURN)){
        self->direction = !self->direction;
        ent_set_anim(self, ANI_TURN, 0);
        self->xdir = self->zdir = 0;
        self->takeaction = common_turn;
        return 1;
    }

    if(common_try_jump()) return 1;  //need to jump? so quit

    checkpathblocked();

    // judge next move if stalltime is expired
    if(self->stalltime < time ){
        if(other){
            // try walking to the item
            common_try_pick(other);
        }
        // let's wander around
        else if(!common_try_wandercompletely()){
            // no target or item, just relex and idle
            self->xdir = self->zdir = 0;
            set_idle(self);
        }
        //turn back if we have a turn animation
        if(self->direction != predir && validanim(self,ANI_TURN)){
            self->direction = !self->direction;
            ent_set_anim(self, ANI_TURN, 0);
            self->xdir = self->zdir = 0;
            self->takeaction = common_turn;
            return 1;
        }
        self->stalltime = time + GAME_SPEED - self->modeldata.aggression;
    }

    //pick up the item if possible
    if( (other && other == find_ent_here(self, self->x, self->z, TYPE_ITEM)) && other->animation->vulnerable[other->animpos])//won't pickup an item that is not previous one
    {
        seta = (float)(self->animation->seta?self->animation->seta[self->animpos]:-1);
        if(diff(self->a - (seta>= 0) * seta , other->a)<0.1)
        common_pickupitem(other);
    }

    return 1;
}

// for old bikers
int biker_move()
{

    if((self->direction)?(self->x > advancex+(videomodes.hRes+200)):(self->x < advancex-200))
    {
        self->direction = !self->direction;
        self->attack_id = 0;
        self->z = (float)(PLAYER_MIN_Z + randf((float)(PLAYER_MAX_Z-PLAYER_MIN_Z)));
        if(SAMPLE_BIKE >= 0) sound_play_sample(SAMPLE_BIKE, 0, savedata.effectvol,savedata.effectvol, 100);
        if(self->modeldata.speed) self->xdir = (self->direction)?(self->modeldata.speed):(-self->modeldata.speed);
        else self->xdir = (self->direction)?((float)1.7 + randf((float)0.6)):(-((float)1.7 + randf((float)0.6)));
    }

    self->nextthink = time + THINK_SPEED / 2;
    return 1;
}

// for common arrow types
int arrow_move(){
    int wall;
    float dx;
    float dz;
    float maxspeed;
    entity* target = NULL;

    /*
    int osk = self->modeldata.offscreenkill?self->modeldata.offscreenkill:200; //TODO: temporary code here, needs refine

    if( (!self->direction && self->x < advancex - osk) || (self->direction && self->x > advancex + (videomodes.hRes+osk)) ||
        (level && level->scrolldir != SCROLL_UP && level->scrolldir != SCROLL_DOWN &&
         (self->z < advancey - 200 || self->z > advancey + (videomodes.vRes+200))) ||
        (level && (level->scrolldir == SCROLL_UP || level->scrolldir == SCROLL_DOWN) &&
         (self->z < -osk || self->z > videomodes.vRes + osk)) )
    {
        kill(self);
		return 0;
    }*/

    // new subtype chase
    if(self->modeldata.subtype == SUBTYPE_CHASE)
	{
        target = homing_find_target(self->modeldata.hostile);

        if(target)
		{
            if(!self->modeldata.noflip) self->direction = (target->x > self->x);
            // start chasing the target
            dx = diff(self->x, target->x);
            dz = diff(self->z, target->z);
            maxspeed = self->modeldata.speed * 1.5;

            if(!dz && !dx) self->xdir = self->zdir = 0;
            else
			{
                self->xdir = maxspeed * dx / (dx+dz);
                self->zdir = maxspeed * dz / (dx+dz);
            }
            if(self->direction!=1) self->xdir = -self->xdir;
            if(self->z > target->z) self->zdir = -self->zdir;
        }
		else
		{
            if(!self->xdir && !self->zdir)
			{
                if(self->direction == 0) self->xdir = -self->modeldata.speed;
                else if(self->direction == 1) self->xdir = self->modeldata.speed;
            }
        }
        if(!self->modeldata.nomove)
        {
            if(target && self->z > target->z && validanim(self,ANI_UP)) common_up_anim(self); //ent_set_anim(self, ANI_UP, 0);
            else if(target && target->z > self->z && validanim(self,ANI_DOWN)) common_down_anim(self); //ent_set_anim(self, ANI_DOWN, 0);
            else if(validanim(self,ANI_WALK)) common_walk_anim(self); //ent_set_anim(self, ANI_WALK, 0);
            else
            {
                ent_set_anim(self, ANI_IDLE, 0);
            }
        }
    }
	else
	{
		// Now projectiles can have custom speeds
        if(self->direction == 0) self->xdir = -self->modeldata.speed;
        else if(self->direction == 1) self->xdir = self->modeldata.speed;
    }

    if(level)
    {
        if((level->exit_blocked && self->x > level->width-30-(PLAYER_MAX_Z-self->z)) ||
            (self->modeldata.subject_to_wall && (wall = checkwall(self->x, self->z)) >= 0 && self->a < level->walls[wall][7]))
        {
			// Added so projectiles bounce off blocked exits
            if(validanim(self,ANI_FALL))
            {
                self->attacking = 0;
                self->health -= 100000;
                self->projectile = 0;
				if(self->direction == 0) self->xdir = (float)-1.2;
				else if(self->direction == 1) self->xdir = (float)1.2;
                self->takeaction = common_fall;
                self->damage_on_landing = 0;
                toss(self, 2.5 + randf(1));
                self->modeldata.no_adjust_base = 0;
                self->modeldata.subject_to_wall = self->modeldata.subject_to_platform = self->modeldata.subject_to_hole = self->modeldata.subject_to_gravity = 1;
                set_fall(self, ATK_NORMAL, 0, self, 100000, 0, 0, 0, 0, 0);
            }
        }
    }
	if(self->ptype)
	{
		self->autokill = 1;
		self->nextthink = time + 1;
	}
	else self->nextthink = time + THINK_SPEED / 2;
	return 1;
}

// for common bomb types
int bomb_move()
{
    /*if(!(self->x > level->width - 30 - (PLAYER_MAX_Z-self->z)))
    {*/
        if(self->direction == 0) self->xdir = -self->modeldata.speed;
        else if(self->direction == 1) self->xdir = self->modeldata.speed;
    /*}*/

    self->nextthink = time + THINK_SPEED / 2;

    if(inair(self) && self->toexplode == 1) return 1;

    self->tossv = 0;    // Stop moving up/down
    self->modeldata.no_adjust_base = 1;    // Stop moving up/down
    self->base = self->a;
    self->xdir = self->zdir = 0;

    if(self->modeldata.diesound >= 0) sound_play_sample(self->modeldata.diesound, 0, savedata.effectvol,savedata.effectvol, 100);

    if(self->toexplode == 2 && validanim(self,ANI_ATTACK2))
	{
		ent_set_anim(self, ANI_ATTACK2, 0);    // If bomb never reaces the ground, play this
	}
	else
	{
		if (validanim(self,ANI_ATTACK1)) ent_set_anim(self, ANI_ATTACK1, 0);
	}
    // hit something, just make it an explosion animation.
    self->modeldata.subject_to_wall = 0;
    self->modeldata.subject_to_platform = 1;
    self->modeldata.subject_to_hole = 0;
    self->takeaction = bomb_explode;
	return 1;
}

int star_move(){
    int wall;

    if(self->x<advancex-80 || self->x>advancex+(videomodes.hRes+80) || (self->base<=0 && !self->modeldata.falldie)){
        kill(self);
        return 0;
    }

    self->base -= 2;
    self->a = self->base;

    if(validanim(self,ANI_FALL))    // Added so projectiles bounce off blocked exits
    {
        if((level->exit_blocked && self->x > level->width-30-(PLAYER_MAX_Z-self->z)) ||
            ((wall = checkwall(self->x, self->z)) >= 0 && self->a < level->walls[wall][7]))
        {
            self->attacking = 0;
            self->health -= 100000;
            self->projectile = 0;
            self->xdir = (self->direction)?(-1.2):1.2;
            self->takeaction = common_fall;
            self->damage_on_landing = 0;
            toss(self, 2.5 + randf(1));
            set_fall(self, ATK_NORMAL, 0, self, 100000, 0, 0, 0, 0, 0);
        }
    }

	if(self->landed_on_platform || self->base <= 0)
	{
        self->health -= 100000;
		if(self->modeldata.nodieblink == 2) self->animating = 0;
        self->takeaction = common_lie;
	}

    self->nextthink = time + THINK_SPEED / 2;
    return 1;
}


//dispatch move patterns
int common_move()
{
    int aimove;
    int air = inair(self);
	if(self->modeldata.aimove==-1) return 0; // invalid value

    // get move pattern
    aimove = self->modeldata.aimove & MASK_AIMOVE1 & rand32();
    if(!aimove) aimove = self->modeldata.aimove & MASK_AIMOVE1;
//if(stricmp(self->name, "os")==0) printf("%d\n", aimove);
    if(!aimove)
    {// normal move style, for common enemy/npc
        return air?0:normal_move();
    }
    else if(aimove&(AIMOVE1_AVOID|AIMOVE1_AVOIDX|AIMOVE1_AVOIDZ))
    {// try to avoid target
        return air?0:avoid_move();
    }
    else if(aimove&(AIMOVE1_CHASE|AIMOVE1_CHASEX|AIMOVE1_CHASEZ))
    {// try to chase target
        return air?0:chase_move();
    }
    else if(aimove&AIMOVE1_WANDER)
    {// ignore target
        return air?0:wander_move();
    }
    else if(aimove&AIMOVE1_BIKER)
    {// for biker subtype
        return biker_move();
    }
    else if(aimove&AIMOVE1_ARROW)
    {// for common straight-flying arrow
		return arrow_move();
    }
    else if(aimove&AIMOVE1_STAR)
    {// for a star, disappear when hit ground
        return star_move();
    }
    else if(aimove&AIMOVE1_BOMB)
    {// for a bomb, travel in a arc
        return bomb_move();
    }
    else if(aimove&AIMOVE1_NOMOVE)
    {// no move, just return
        return 0;
    }

    return 0;
}

// A.I root
void common_think()
{
    if(self->dead) return;

    // too far away , do a warp
    if(self->modeldata.subtype == SUBTYPE_FOLLOW && self->parent &&
        (diff(self->z, self->parent->z) > self->modeldata.animation[ANI_IDLE]->range[1] ||
        diff(self->x, self->parent->x) > self->modeldata.animation[ANI_IDLE]->range[1]) )
    {
        self->takeaction = npc_warp;
        return;
    }

    // rise? try rise attack
	if(self->drop && self->a==self->base && !self->tossv && validanim(self,ANI_RISEATTACK) && ((rand32()%(self->stalltime-time+1)) < 3) && (self->health >0 && time > self->staydown[2]))
    {
        common_try_riseattack();
        return;
    }

    // Escape?
    if(self->link && !self->grabbing && !self->inpain && self->takeaction!=common_prethrow &&
       time >= self->stalltime && validanim(self,ANI_SPECIAL))
    {
        check_special();
        return;
    }

    if(self->grabbing && !self->attacking)
    {
        common_grab_check();
        return;
    }

    // Reset their escapecount if they aren't being spammed anymore.
    if(self->modeldata.escapehits && !self->inpain) self->escapecount = 0;

    // Enemies can now escape non-knockdown spammage (What a weird phrase)!
    if((self->escapecount > self->modeldata.escapehits) && !inair(self) && validanim(self,ANI_SPECIAL2))
    {
        // Counter the player!
        check_costmove(ANI_SPECIAL2, 0);
        return;
    }

    if(self->link) return;

    // idle, so try to attack or judge next move
    // dont move if fall into a hole or off a wall
	if(self->idling /*|| (self->animation->idle && self->animation->idle[self->animpos])*/)
    {
       if(common_attack()) return;
       common_move();
    }
}

//////////////////////////////////////////////////////////////////////////

void suicide()
{
    if(time < self->stalltime) return;
    level_completed |= self->boss;
    kill(self);
}



// Re-enter playfield
// Used by player_fall and player_takedamage
void player_die()
{
    int playerindex = self->playerindex;
    if(!livescheat) --player[playerindex].lives;

    execute_pdie_script(playerindex);

	if(nomaxrushreset[4] >= 1) nomaxrushreset[playerindex] = player[playerindex].ent->rush[1];
    player[playerindex].ent = NULL;
    player[playerindex].spawnhealth = self->modeldata.health;
    player[playerindex].spawnmp = self->modeldata.mp;



    if(self->modeldata.nodieblink != 3) kill(self);
    else
    {
        self->think = NULL;
        self->takeaction = NULL;
        self->modeldata.type = TYPE_NONE;
    }

    if(player[playerindex].lives <= 0)
    {
        if(!player[0].ent && !player[1].ent && !player[2].ent && !player[3].ent)
        {
            timeleft = 10 * COUNTER_SPEED;
            if((!noshare && credits < 1) || (noshare && player[0].credits < 1 && player[1].credits < 1 && player[2].credits <1 && player[3].credits <1)) timeleft = COUNTER_SPEED/2;
        }
        if(self->modeldata.weaploss[0]<=3) player[playerindex].weapnum = level->setweap;
		if(nomaxrushreset[4] != 2) nomaxrushreset[playerindex] = 0;
        return;
    }
    else
    {
        spawnplayer(playerindex);
        execute_respawn_script(playerindex);
    	if(!nodropen)
        {
    		control_rumble(playerindex, 125);
    		drop_all_enemies();
    	}
    }

    if(!level->noreset) timeleft = level->settime * COUNTER_SPEED;    // Feb 24, 2005 - This line moved here to set custom time

}



int player_trymove(float xdir, float zdir)
{
    return common_trymove(xdir, zdir);
}

int check_energy(int which, int ani)
{
	int iCost[3];																//0 = Energycost[0] (amount of HP or MP needed), 1 = Cost type (MP, HP, both), 2 = Disable flag.
	int iType;																	//Entity type.

	if(self->modeldata.animation[ani])											//Does animation exist?
	{
		iCost[2]	= self->modeldata.animation[ani]->energycost[2];			//Get disable flag.
		iType		= self->modeldata.type;										//Get entity type.

		/* DC 05082010: It is now possible to individualy disable specials. In
		many cases (weapons in particular) this can	help cut down the need for
		superflous models when differing abilities are desired for players,

		enemies, or npcs. */
		if(!(iCost[2]==iType													//Disabled by type?
			|| (iCost[2]==-1)													//Disabled for all?
			|| (iCost[2]==-2 && (iType == TYPE_ENEMY || iType == TYPE_NPC))		//Disabled for all AI?
			|| (iCost[2]==-3 && (iType == TYPE_PLAYER || iType == TYPE_NPC))	//Disabled for players & NPCs?
			|| (iCost[2]==-4 && (iType == TYPE_PLAYER || iType == TYPE_ENEMY))))//Disabled for all AI?
		{
			iCost[0] = self->modeldata.animation[ani]->energycost[0];			//Get energy cost amount
			iCost[1] = self->modeldata.animation[ani]->energycost[1];			//Get energy cost type.

			if(!self->seal || self->seal >= iCost[0])							//No seal or seal is less/same as energy cost?
			{
				if(validanim(self,ani) &&										//Validate the animation one more time.
						((which &&												//Check magic validity
						(iCost[1] != 2) &&										//2 = From health bar only, 0 from both
						(self->mp >= iCost[0])) ||
						(!which &&												//Checks health validity
						(iCost[1] != 1) &&										//1 = From magic bar only, 0 from both
						(self->health > iCost[0]))))
				{
					return 1;
				}
				else
				{
					//DC 01232009
					//Tried putting the CANT animation here to keep code compacted, but won't work. I'll come back to this.
					//if (validanim(self,ani)){
					//    ent_set_anim(self, ANI_CANT, 0);
					//    self->takeaction = common_attack_proc;
					//    player[(int)self->playerindex].playkeys = 0;
					//}
					return 0;
				}
			}
		}
	}
	return 0;
}


int check_special()
{
    if((!level->nospecial || level->nospecial == 3) &&
       !self->cantfire &&
       (check_energy(0, ANI_SPECIAL) ||
        check_energy(1, ANI_SPECIAL)))
    {
        set_attacking(self);
        memset(self->combostep, 0, sizeof(int)*5);
        ent_unlink(self);

        if(self->modeldata.smartbomb && !self->modeldata.dofreeze)
        {
            smart_bomb(self, self->modeldata.smartbomb); // do smartbomb immediately if it doesn't freeze screen
        }

        self->running = 0;    // If special is executed while running, ceases to run
        self->xdir = self->zdir = 0;
        ent_set_anim(self, ANI_SPECIAL, 0);
        self->takeaction = common_attack_proc;

        if(self->modeldata.dofreeze) smartbomber = self;    // Freezes the animations of all enemies/players while special is executed

        if(!nocost && !healthcheat)
        {
            if(check_energy(1, ANI_SPECIAL)) self->mp -= self->modeldata.animation[ANI_SPECIAL]->energycost[0];
            else self->health -= self->modeldata.animation[ANI_SPECIAL]->energycost[0];
        }

        return 1;
    }
    return 0;
}


// Check keys for special move. Used several times, so I func'd it.
// 1-10-05 changed self->health>6 to self->health > self->modeldata.animation[ANI_SPECIAL]->energycost[0]
int player_check_special()
{
    u32 thekey = 0;

    if((!ajspecial || (ajspecial && !validanim(self,ANI_BLOCK))) &&
        (player[(int)self->playerindex].playkeys & FLAG_SPECIAL))
    {
        thekey = FLAG_SPECIAL;
    }
    else if(ajspecial && ((player[(int)self->playerindex].playkeys & FLAG_JUMP) &&
        (player[(int)self->playerindex].keys & FLAG_ATTACK)))
    {
        thekey = FLAG_JUMP;
    }
    else return 0;

    if(check_special())
    {
        self->stalltime = 0;
        player[(int)self->playerindex].playkeys -= thekey;
        return 1;
    }else{
        return 0;
    }
}


void common_land()
{
    self->xdir = self->zdir = 0;
    if(self->animating) return;

    set_idle(self);
    self->takeaction = NULL;
}


//animal run when you lost it 3 times by tails
void runanimal()
{
    common_walk_anim(self);
    //ent_set_anim(self, ANI_WALK, 0);

    if(self->x < advancex - 80 || self->x > advancex + (videomodes.hRes+80)){
        kill(self);
        return;
    }

    if(self->direction) self->x += self->modeldata.speed;
    else self->x -= self->modeldata.speed;
}


void player_blink()
{
    self->blink = 1;
    if(time >= self->stalltime) player_die();
}


void common_grabattack()
{
    if(self->animating) return;

    self->attacking = 0;

    if(!(self->combostep[0] || self->combostep[1] ||
         self->combostep[2] || self->combostep[3] ||
         self->combostep[4]))
    {
        ent_unlink(self);
    }

    if(self->link)
    {
        self->attacking = 0;
        ent_set_anim(self, ANI_GRAB, 0);
        set_pain(self->link, -1, 0);
        update_frame(self, self->animation->numframes-1);
        update_frame(self->link, self->link->animation->numframes-1);
        self->takeaction = common_grab;
        self->link->takeaction = common_grabbed;
    }
    else
    {
        memset(self->combostep, 0, sizeof(int)*5);
        set_idle(self);
        self->takeaction = NULL;
    }
}

// The vault.
void common_vault()
{
    if(!self->link)
    {
        set_idle(self);
        self->takeaction = NULL;
        return;
    }
    if(!self->animating)
    {
        self->attacking = 0;
        self->direction = !self->direction;
        self->a = self->base = self->link->base;

        if(self->direction) self->x = self->link->x - self->modeldata.grabdistance;
        else self->x = self->link->x + self->modeldata.grabdistance;

        ent_set_anim(self, ANI_GRAB, 0);
        set_pain(self->link, -1, 0);
        update_frame(self, self->animation->numframes-1);
        update_frame(self->link, self->link->animation->numframes-1);
        self->takeaction = common_grab;
        self->link->takeaction = common_grabbed;
        return;
    }
}


void common_prejump()
{
    if(self->animating) return;
    dojump(self->jumpv, self->jumpx, self->jumpz, self->jumpid);
}

void tryjump(float jumpv, float jumpx, float jumpz, int jumpid)
{
    self->jumpv = jumpv;              self->jumpx = jumpx;
    self->jumpz = jumpz;              self->jumpid = jumpid;
    if(validanim(self,ANI_JUMPDELAY))
    {
        ent_set_anim(self, ANI_JUMPDELAY, 0);
        self->xdir = self->zdir = 0;

        self->idling = 0;
        self->takeaction = common_prejump;
    }
    else
    {
        dojump(jumpv, jumpx, jumpz, jumpid);
    }
}


void dojump(float jumpv, float jumpx, float jumpz, int jumpid)
{
    entity* dust;

    if(SAMPLE_JUMP >= 0) sound_play_sample(SAMPLE_JUMP, 0, savedata.effectvol,savedata.effectvol, 100);

    //Spawn jumpstart dust.
    if(self->modeldata.dust[2]>=0)
    {
        dust = spawn(self->x, self->z, self->a, self->direction, NULL, self->modeldata.dust[2], NULL);
        dust->base = self->a;
        dust->autokill = 1;
        execute_onspawn_script(dust);
    }

    set_jumping(self);
    ent_set_anim(self, jumpid, 0);

    toss(self, jumpv);

    if(self->direction == 0)
        self->xdir = -jumpx;
    else self->xdir = jumpx;

    self->zdir = jumpz;

    self->takeaction = common_jump;
}

// make a function so enemies can use
int check_costmove(int s, int fs)
{
    if(((fs == 1 && level->nospecial < 2) || (fs == 0 && level->nospecial == 0) || (fs == 0 && level->nospecial == 3)) &&
       (check_energy(0, s) ||
        check_energy(1, s))  )
    {
        if(!nocost && !healthcheat)
        {
            if(check_energy(1, s)) self->mp -= self->modeldata.animation[s]->energycost[0];
            else self->health -= self->modeldata.animation[s]->energycost[0];
        }

        self->xdir = self->zdir = 0;
        set_attacking(self);
        self->inpain = 0;
        memset(self->combostep, 0, sizeof(int)*5);
        ent_unlink(self);
        self->movestep = 0;
        ent_set_anim(self, s, 0);
        self->takeaction = common_attack_proc;
        return 1;
    }
    return 0;
}


// Function to check custom combos. If movestep is 0, means ready to check to see if the second step in the combo is
// valid. If so, returns 1, setting each valid combo in the list so far to 1, otherwise 0. If movestep is > 0, means
// ready to check the action button step of the combo. Loops through the "valid combo" list and sees if the action
// button step is valid, returning 1 if true, otherwise 0.
int check_combo(int m){    // New function to check combos to make sure they are valid
    int i;
    int found = 0;    // Default not found unless overridden by finding a valid combo
	int value = self->animation->cancel; // OX. If cancel is enabled , we will be checking MAX_SPECIAL_INPUTS-4, -5, -6 instead.

    // check one-step freespecial
    for(i = 0; i < self->modeldata.specials_loaded; i++)
    {
        if(self->modeldata.special[i][MAX_SPECIAL_INPUTS-3]==1 && self->modeldata.special[i][0]==m && value == 0)
        {
            if(check_costmove(self->modeldata.special[i][MAX_SPECIAL_INPUTS-(2+value)], 1))
            {
                self->modeldata.valid_special = i;
                return 1;    // Valid combo found, go ahead and return
            }
            else return 0;    // Found, but cost more health than the player had
        }
		else if(self->modeldata.special[i][MAX_SPECIAL_INPUTS-6]==1 && self->modeldata.special[i][0]==m &&
			self->modeldata.special[i][MAX_SPECIAL_INPUTS-10] <= self->animation->animhits &&
				self->modeldata.special[i][MAX_SPECIAL_INPUTS-7] <= self->animpos &&
				self->modeldata.special[i][MAX_SPECIAL_INPUTS-8] >= self->animpos &&
				self->modeldata.special[i][MAX_SPECIAL_INPUTS-9] == self->animnum)
		{
			if(check_costmove(self->modeldata.special[i][MAX_SPECIAL_INPUTS-5], 1))
            {
                self->modeldata.valid_special = i;
                return 1;    // Valid combo found, go ahead and return
            }
            else return 0;    // Found, but cost more health than the player had
        }
    }// end of for

    if(time > self->movetime) return 0;    // Too much time passed so return 0

    if(m == FLAG_FORWARD || m==FLAG_BACKWARD || m==FLAG_MOVEUP || m==FLAG_MOVEDOWN){   // direction keys
        for(i = 0; i < self->modeldata.specials_loaded; i++)
        {
            if( self->modeldata.special[i][MAX_SPECIAL_INPUTS-(3+value)]>self->movestep &&
                self->modeldata.special[i][(int)self->movestep] == self->lastmove &&
                self->modeldata.special[i][(int)self->movestep+1] == m )
            {

                self->modeldata.special[i][MAX_SPECIAL_INPUTS-(1+value)] = 1;    // Marks all valid directional combos with a 1
                found = 1;    // There is at least 1 valid combo, so return found
            }
            else self->modeldata.special[i][MAX_SPECIAL_INPUTS-(1+value)] = 0;    // Marks all invalid directional combos with a 0
        }//end of for

        return found;    // Returns 1 if found, otherwise returns 0
    }
    else// action buttons
    {
        for(i = 0; i < self->modeldata.specials_loaded; i++)
        {
            if(self->modeldata.special[i][MAX_SPECIAL_INPUTS-1] && self->modeldata.special[i][self->movestep+1] == m && value == 0)
            {    // Checks only valid directional combos to see if the action button matches
                if(check_costmove(self->modeldata.special[i][MAX_SPECIAL_INPUTS-2], 1))
                {
                    self->modeldata.valid_special = i;    // Says which one is valid and returns that it was found
                    return 1;    // Valid combo found, go ahead and return
                }
                else return 0;    // Found, but cost more health than the player had
            }
			else if(self->modeldata.special[i][MAX_SPECIAL_INPUTS-4] && self->modeldata.special[i][self->movestep+1] == m  &&
				self->modeldata.special[i][MAX_SPECIAL_INPUTS-10] <= self->animation->animhits &&
				self->modeldata.special[i][MAX_SPECIAL_INPUTS-7] <= self->animpos &&
				self->modeldata.special[i][MAX_SPECIAL_INPUTS-8] >= self->animpos &&
				self->modeldata.special[i][MAX_SPECIAL_INPUTS-9] == self->animnum)
            {    // Checks only valid directional combos to see if the action button matches
                if(check_costmove(self->modeldata.special[i][MAX_SPECIAL_INPUTS-5], 1))
                {
                    self->modeldata.valid_special = i;    // Says which one is valid and returns that it was found
                    return 1;    // Valid combo found, go ahead and return
                }
                else return 0;    // Found, but cost more health than the player had
            }
        }// end of for

        return 0;    // No valid combos found, return 0
    }
}


// Function that causes the player to continue to move up or down until the animation has finished playing
void common_dodge()    // New function so players can dodge with up up or down down
{
    if(self->animating)    // Continues to move as long as the player is animating
    {
        if(self->zdir<0) self->zdir = -self->modeldata.speed * 1.75;
        else self->zdir = self->modeldata.speed * 1.75;
        self->xdir = 0;
    }
    else    // Once done animating, returns to thinking
    {
        self->xdir = self->zdir = 0;
        set_idle(self);
        self->takeaction = NULL;
    }
}



// Function created to combine the action taken if either picking up an item, or running into an item that is a
// SUBTYPE_TOUCH, executing the appropriate action based on which type of item is picked up
void didfind_item(entity *other)
{    // Function that takes care of items when picked up
    set_opponent(self, other);

    //for reload weapons that are guns(no knife) we use this items reload for ours shot at max and shootnum in items for get a amount of shoots by tails
    if(other->modeldata.reload)
    {
        if(self->weapent && self->weapent->modeldata.typeshot)
        {
            self->weapent->modeldata.shootnum += other->modeldata.reload;
            if(self->weapent->modeldata.shootnum > self->weapent->modeldata.shootnum) self->weapent->modeldata.shootnum = self->weapent->modeldata.shootnum;
            if(SAMPLE_GET >= 0) sound_play_sample(SAMPLE_GET, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        else
        {
            addscore(self->playerindex, other->modeldata.score);
            if(SAMPLE_GET2 >= 0) sound_play_sample(SAMPLE_GET2, 0, savedata.effectvol,savedata.effectvol, 100);
        }
    }
    //end of weapons items section
    else if(other->modeldata.score)
    {
        addscore(self->playerindex, other->modeldata.score);

        if(SAMPLE_GET2 >= 0) sound_play_sample(SAMPLE_GET2, 0, savedata.effectvol,savedata.effectvol, 100);
    }
    else if(other->health)
    {
        self->health += other->health;

        if(self->health > self->modeldata.health) self->health = self->modeldata.health;

        other->health = 0;

        if(SAMPLE_GET >= 0) sound_play_sample(SAMPLE_GET, 0, savedata.effectvol,savedata.effectvol, 100);
    }
    else if(other->modeldata.mp)
    {
        self->mp += other->modeldata.mp;

        if(self->mp > self->modeldata.mp) self->mp = self->modeldata.mp;

        other->mp = 0;
        sound_play_sample(SAMPLE_GET, 0, savedata.effectvol,savedata.effectvol, 100);
    }
    else if(stricmp(other->modeldata.name, "Time")==0)
    {
        timeleft = level->settime * COUNTER_SPEED;    // Feb 24, 2005 - This line moved here to set custom time

        if(SAMPLE_GET2 >= 0) sound_play_sample(SAMPLE_GET2, 0, savedata.effectvol,savedata.effectvol, 100);
    }
    else if(other->modeldata.makeinv)
    {    // Mar 2, 2005 - New item makes player invincible
        self->invincible = 1;
        self->invinctime = time + ABS(other->modeldata.makeinv);
        self->blink = (other->modeldata.makeinv>0);

        if(SAMPLE_GET2 >= 0) sound_play_sample(SAMPLE_GET2, 0, savedata.effectvol,savedata.effectvol, 100);
    }
    else if(other->modeldata.smartbomb)
    {    // Damages everything on the screen
        smart_bomb(self, other->modeldata.smartbomb);

        if(SAMPLE_GET2 >= 0) sound_play_sample(SAMPLE_GET2, 0, savedata.effectvol,savedata.effectvol, 100);
    }
    else if(other->modeldata.subtype == SUBTYPE_WEAPON)
    {
        dropweapon(0);
        self->weapent = other;
        set_weapon(self, other->modeldata.weapnum, 0);

        if(self->modeldata.animal)  // UTunnels: well, ride, not get. :)
        {
            self->direction = other->direction;
            self->x = other->x;
            self->z = other->z;
        }

        if(!other->modeldata.typeshot && self->modeldata.typeshot) other->modeldata.typeshot = 1;

        if(SAMPLE_GET >= 0) sound_play_sample(SAMPLE_GET, 0, savedata.effectvol,savedata.effectvol, 100);
    }
    else if(other->modeldata.subtype == SUBTYPE_PROJECTILE)
    {
        dropweapon(0);
        self->weapent = other;

        if(SAMPLE_GET >= 0) sound_play_sample(SAMPLE_GET, 0, savedata.effectvol,savedata.effectvol, 100);
    }
    else if(other->modeldata.credit)
    {
        if(!noshare) credits++;
        else player[(int)self->playerindex].credits++;

        if(SAMPLE_1UP >= 0) sound_play_sample(SAMPLE_1UP, 0, savedata.effectvol,savedata.effectvol, 100);
    }
    else
    {
        // Must be a 1up then.
        player[(int)self->playerindex].lives++;

        if(SAMPLE_1UP >= 0) sound_play_sample(SAMPLE_1UP, 0, savedata.effectvol,savedata.effectvol, 100);
    }

    if(other->modeldata.subtype != SUBTYPE_WEAPON && other->modeldata.subtype != SUBTYPE_PROJECTILE)
    {
        other->takeaction = suicide;
        if(!other->modeldata.instantitemdeath)
             other->nextthink = time + GAME_SPEED * 3;
    }
    other->z = 100000;
}

void player_fall_check()
{
    if(autoland != 2 && (player[(int)self->playerindex].keys & (FLAG_MOVEUP|FLAG_JUMP)) == (FLAG_MOVEUP|FLAG_JUMP))
    {
        player[(int)self->playerindex].playkeys ^= (FLAG_MOVEUP|FLAG_JUMP);
        self->damage_on_landing=-2; // mark it, so we will play land animation when hit the ground
    }
}

void player_grab_check()
{
    entity * other = self->link;

    if(other == NULL || (self->modeldata.grabfinish && self->animating && !self->grabwalking)) return;

    if(self->base != other->base)
    {       // Change this from ->a to ->base
        ent_unlink(self);
        set_idle(self);
        self->takeaction = NULL;
        return;
    }
	  // OX cancel checking.
	if(self->animation->cancel==3)
    {
		if((player[(int)self->playerindex].playkeys & FLAG_ATTACK) && check_combo(FLAG_ATTACK))
        {
            player[(int)self->playerindex].playkeys -= FLAG_ATTACK;
			ent_unlink(self);
			self->attacking = 1;
			self->takeaction = common_attack_proc;
            return;
        }
        if((player[(int)self->playerindex].playkeys & FLAG_ATTACK2) && check_combo(FLAG_ATTACK2))
        {
            player[(int)self->playerindex].playkeys -= FLAG_ATTACK2;
			ent_unlink(self);
			self->attacking = 1;
			self->takeaction = common_attack_proc;
            return;
        }
        if((player[(int)self->playerindex].playkeys & FLAG_ATTACK3) && check_combo(FLAG_ATTACK3))
        {
            player[(int)self->playerindex].playkeys -= FLAG_ATTACK3;
			ent_unlink(self);
			self->attacking = 1;
			self->takeaction = common_attack_proc;
            return;
        }
        if((player[(int)self->playerindex].playkeys & FLAG_ATTACK4) && check_combo(FLAG_ATTACK4))
        {
            player[(int)self->playerindex].playkeys -= FLAG_ATTACK4;
			ent_unlink(self);
			self->attacking = 1;
			self->takeaction = common_attack_proc;
            return;
        }
		if((player[(int)self->playerindex].playkeys & FLAG_JUMP) && check_combo(FLAG_JUMP))
        {
            player[(int)self->playerindex].playkeys -= FLAG_JUMP;
			ent_unlink(self);
			self->attacking = 1;
			self->takeaction = common_attack_proc;
            return;
        }
		if((player[(int)self->playerindex].playkeys & FLAG_SPECIAL) && check_combo(FLAG_SPECIAL))
        {
            player[(int)self->playerindex].playkeys -= FLAG_SPECIAL;
			ent_unlink(self);
			self->attacking = 1;
			self->takeaction = common_attack_proc;
            return;
        }
    }
	// End cancel checking

	if(player_check_special()) return;

    if(!nolost && self->modeldata.weaploss[0] <= 0) dropweapon(1);

    // grabturn code
    if(self->animation == self->modeldata.animation[ANI_GRABTURN])
    {
        // still turning? don't bother with anything else
        if(self->animating) return;

        // done turning? switch directions and return to grab animation
        else
        {
            if(self->direction)
            {
                self->direction = 0;
                other->direction = 1;
            }
            else
            {
                self->direction = 1;
                other->direction = 0;
            }
            ent_set_anim(self, ANI_GRAB, 0);
            set_pain(other, -1, 0);
            update_frame(self, self->animation->numframes-1);
            update_frame(other, other->animation->numframes-1);
            other->x = self->x + (((self->direction * 2) - 1) * self->modeldata.grabdistance);
        }
    }

    self->attacking = 0; //for checking
    self->grabwalking = 0;
    if(self->direction ?
        (player[(int)self->playerindex].keys & FLAG_MOVELEFT) :
        (player[(int)self->playerindex].keys & FLAG_MOVERIGHT))
    {
        // initiating grabturn
        if(self->modeldata.grabturn)
        {
                // start animation if it exists...
                if(validanim(self,ANI_GRABTURN))
                {
                    ent_set_anim(self, ANI_GRABTURN, 0);
                    if(validanim(other,ANI_GRABBEDTURN)) ent_set_anim(other, ANI_GRABBEDTURN, 0);
                    else if(validanim(other,ANI_GRABBED)) ent_set_anim(other, ANI_GRABBED, 0);
                    else ent_set_anim(other, ANI_PAIN, 0);
                    other->xdir = other->zdir = self->xdir = self->zdir = 0;
                    other->x = self->x;
                    return;
                }

                // otherwise, just turn around
                else
                {
                    if(self->direction)
                    {
                        self->direction = 0;
                        other->direction = 1;
                    }
                    else
                    {
                        self->direction = 1;
                        other->direction = 0;
                    }
                    ent_set_anim(self, ANI_GRAB, 0);
                    set_pain(other, -1, 0);
                    update_frame(self, self->animation->numframes-1);
                    update_frame(other, other->animation->numframes-1);
                    other->x = self->x + (((self->direction * 2) - 1) * self->modeldata.grabdistance);
                }
        }
        else if(!validanim(self,ANI_GRABWALK) && time > self->releasetime)
        {
            // Release
            ent_unlink(self);
            set_idle(self);
            self->takeaction = NULL;
            return;
        }
    }
    else self->releasetime = time + (GAME_SPEED/2);

    if((player[(int)self->playerindex].playkeys & FLAG_ATTACK) &&
        (self->direction ?
         (player[(int)self->playerindex].keys & FLAG_MOVELEFT) :
         (player[(int)self->playerindex].keys & FLAG_MOVERIGHT)))
    {
        player[(int)self->playerindex].playkeys -= FLAG_ATTACK;
        if(validanim(self,ANI_GRABBACKWARD))
             dograbattack(4);
        else if(validanim(self,ANI_THROW))
        {
             if(self->modeldata.throwframewait >= 0)
                  doprethrow();
             else
                  dothrow();
        }
        else
             dograbattack(0);
    }
    // grab forward
    else if((player[(int)self->playerindex].playkeys & FLAG_ATTACK) &&
        validanim(self,ANI_GRABFORWARD) &&
        (!self->direction ?
         (player[(int)self->playerindex].keys & FLAG_MOVELEFT) :
         (player[(int)self->playerindex].keys & FLAG_MOVERIGHT)))
    {
        player[(int)self->playerindex].playkeys -= FLAG_ATTACK;
        dograbattack(1);
    }
    // grab up
    else if((player[(int)self->playerindex].playkeys & FLAG_ATTACK) &&
        validanim(self, ANI_GRABUP) && (player[(int)self->playerindex].keys & FLAG_MOVEUP))
    {
        player[(int)self->playerindex].playkeys -= FLAG_ATTACK;
        dograbattack(2);
    }
    // grab down
    else if((player[(int)self->playerindex].playkeys & FLAG_ATTACK) &&
        validanim(self,ANI_GRABDOWN) && (player[(int)self->playerindex].keys & FLAG_MOVEDOWN))
    {
        player[(int)self->playerindex].playkeys -= FLAG_ATTACK;
        dograbattack(3);
    }
    // normal grab attack
    else if((player[(int)self->playerindex].playkeys & FLAG_ATTACK) && validanim(self,ANI_GRABATTACK))
    {
        player[(int)self->playerindex].playkeys -= FLAG_ATTACK;
        dograbattack(0);
    }
    // Vaulting.
    else if((player[(int)self->playerindex].playkeys & FLAG_JUMP) && validanim(self,ANI_VAULT))
    {
        player[(int)self->playerindex].playkeys -= FLAG_JUMP;
        dovault();
    }
    // grab attack finisher
    else if(player[(int)self->playerindex].playkeys & (FLAG_JUMP|FLAG_ATTACK))
    {
        player[(int)self->playerindex].playkeys -= player[(int)self->playerindex].playkeys&(FLAG_JUMP|FLAG_ATTACK);

        // Perform final blow
        if(validanim(self,ANI_GRABATTACK2) || validanim(self,ANI_ATTACK3))
            dograbattack(-1);
        else
        {
            self->attacking = 1;
            memset(self->combostep, 0, sizeof(int)*5);
            self->takeaction = common_grabattack;
            tryjump(self->modeldata.jumpheight, self->modeldata.jumpspeed, 0, ANI_JUMP);
        }
    }

    // grab walk code
    else if(validanim(self,ANI_GRABWALK)  // check if grabwalk animation exists

         // if entity is still animating anything besides a grabwalk variant, don't let them move
         && (!self->animating || self->animation == self->modeldata.animation[ANI_GRABWALK]
         || self->animation == self->modeldata.animation[ANI_GRABWALKUP]
         || self->animation == self->modeldata.animation[ANI_GRABWALKDOWN]
         || self->animation == self->modeldata.animation[ANI_GRABBACKWALK])){

         // z axis movement
        if(PLAYER_MIN_Z != PLAYER_MAX_Z)
        {
            if(player[(int)self->playerindex].keys & FLAG_MOVEUP)
            {
                if(self->modeldata.grabwalkspeed) self->zdir = -self->modeldata.grabwalkspeed/2;
                else self->zdir = -self->modeldata.speed/2;
            }
            else if(player[(int)self->playerindex].keys & FLAG_MOVEDOWN)
            {
                if(self->modeldata.grabwalkspeed) self->zdir = self->modeldata.grabwalkspeed/2;
                else self->zdir = self->modeldata.speed/2;
            }
            else if(!(player[(int)self->playerindex].keys & (FLAG_MOVEUP|FLAG_MOVEDOWN)))
                self->zdir = 0;
        }

        // x axis movement
        if(player[(int)self->playerindex].keys & FLAG_MOVELEFT)
        {
            if(self->modeldata.grabwalkspeed) self->xdir = -self->modeldata.grabwalkspeed;
            else self->xdir = -self->modeldata.speed;
        }

        else if(player[(int)self->playerindex].keys & FLAG_MOVERIGHT)
        {
            if(self->modeldata.grabwalkspeed) self->xdir = self->modeldata.grabwalkspeed;
            else self->xdir = self->modeldata.speed;
        }
        else if(!((player[(int)self->playerindex].keys & FLAG_MOVELEFT) || (player[(int)self->playerindex].keys & FLAG_MOVERIGHT)) )
            self->xdir = 0;

        // setting the animations based on the velocity set above
        if(self->xdir || self->zdir){
            if(((self->xdir > 0 && !self->direction) || (self->xdir < 0 && self->direction)) && validanim(self,ANI_GRABBACKWALK))
                ent_set_anim(self, ANI_GRABBACKWALK, 0);
            else if(self->zdir < 0 && validanim(self,ANI_GRABWALKUP)) ent_set_anim(self, ANI_GRABWALKUP, 0);
            else if(self->zdir > 0 && validanim(self,ANI_GRABWALKDOWN)) ent_set_anim(self, ANI_GRABWALKDOWN, 0);
            else ent_set_anim(self, ANI_GRABWALK, 0);
            if(self->animation == self->modeldata.animation[ANI_GRABWALKUP] && validanim(other,ANI_GRABBEDWALKUP))
                ent_set_anim(other, ANI_GRABBEDWALKUP, 0);
            else if(self->animation == self->modeldata.animation[ANI_GRABWALKDOWN] && validanim(other,ANI_GRABBEDWALKDOWN))
                ent_set_anim(other, ANI_GRABBEDWALKDOWN, 0);
            else if(self->animation == self->modeldata.animation[ANI_GRABBACKWALK] && validanim(other,ANI_GRABBEDBACKWALK))
                ent_set_anim(other, ANI_GRABBEDBACKWALK, 0);
            else if(validanim(other,ANI_GRABBEDWALK)) ent_set_anim(other, ANI_GRABBEDWALK, 0);
            else if (validanim(other,ANI_GRABBED)) ent_set_anim(other, ANI_GRABBED, 0);
            else ent_set_anim(other, ANI_PAIN, 0);
        }
        else{
            ent_set_anim(self, ANI_GRAB, 0);
            if (validanim(other,ANI_GRABBED)) ent_set_anim(other, ANI_GRABBED, 0);
            else ent_set_anim(other, ANI_PAIN, 0);
        }
        // use check_link_move to set velocity, don't change it here
        other->zdir = other->xdir = 0;
        self->grabwalking = 1;
    }
		 player_preinput();

    if(self->attacking)  self->releasetime = time + (GAME_SPEED/2); // reset releasetime when do attacks
}


void player_jump_check()
{
    int candospecial = 0;
    if(!noaircancel || !self->animating || self->animnum == self->jumpid)
    {
        //air special, copied and changed from Fugue's code
        if((!level->nospecial || level->nospecial == 3) && player[(int)self->playerindex].playkeys & FLAG_SPECIAL){

            if(validanim(self,ANI_JUMPSPECIAL))
            {
                if(check_energy(1, ANI_JUMPSPECIAL))
                {
                    if(!healthcheat) self->mp -= self->modeldata.animation[ANI_JUMPSPECIAL]->energycost[0];
                    candospecial = 1;
                }
                else if(check_energy(0, ANI_JUMPSPECIAL))
                {
                    if(!healthcheat) self->health -= self->modeldata.animation[ANI_JUMPSPECIAL]->energycost[0];
                    candospecial = 1;
                }
                else if(validanim(self,ANI_JUMPCANT))
                {
					player[(int)self->playerindex].playkeys -= FLAG_SPECIAL;
                    ent_set_anim(self, ANI_JUMPCANT, 0);
                    self->tossv = 0;
                }

                if(candospecial)
                {
					player[(int)self->playerindex].playkeys -= FLAG_SPECIAL;
                    ent_set_anim(self, ANI_JUMPSPECIAL, 0);
                    self->attacking = 1;
                    self->xdir = self->zdir = 0;                         // Kill movement when the special starts
                    self->tossv = 0;
                }
            }
        }//end of jumpspecial

        //jumpattacks, up down forward normal....we don't check energy cost
        else if(player[(int)self->playerindex].playkeys & FLAG_ATTACK){
            player[(int)self->playerindex].playkeys -= FLAG_ATTACK;
            self->attacking = 1;
			//OX cancel checking of attack button for compatibility
			if(self->animation->cancel==3 && check_combo(FLAG_ATTACK))
			{
				//player[(int)self->playerindex].playkeys -= FLAG_ATTACK;
				self->jumping = 1;
				self->takeaction = common_jump;
				return;
			}
			// End cancel check.

            else if((player[(int)self->playerindex].keys & FLAG_MOVEDOWN) && validanim(self,ANI_JUMPATTACK2)) ent_set_anim(self, ANI_JUMPATTACK2, 0);
            else if((player[(int)self->playerindex].keys & FLAG_MOVEUP) && validanim(self,ANI_JUMPATTACK3)) ent_set_anim(self, ANI_JUMPATTACK3, 0);
            else if(self->running && validanim(self,ANI_RUNJUMPATTACK)) ent_set_anim(self, ANI_RUNJUMPATTACK, 0);    // Added so an extra strong jump attack can be executed
            else if(self->xdir != 0 && validanim(self,ANI_JUMPFORWARD)) ent_set_anim(self, ANI_JUMPFORWARD, 0);    // If moving and set, do this attack
            else if(validanim(self,ANI_JUMPATTACK)) ent_set_anim(self, ANI_JUMPATTACK, 0);
        }//end of jumpattack
    }
    if(self->modeldata.jumpmovex&1) //flip?
    {
       if((player[(int)self->playerindex].keys & FLAG_MOVELEFT)) self->direction = 0;
       else if((player[(int)self->playerindex].keys & FLAG_MOVERIGHT)) self->direction = 1;
    }
    if(self->modeldata.jumpmovex&2) //move?
    {
       if(((player[(int)self->playerindex].keys & FLAG_MOVELEFT)&&self->xdir>0) ||
          ((player[(int)self->playerindex].keys & FLAG_MOVERIGHT)&&self->xdir<0))	self->xdir = -self->xdir;
    }
	if(self->modeldata.jumpmovex&4) //Move x if vertical jump?
	{
		if(((player[(int)self->playerindex].keys & FLAG_MOVELEFT)&&self->xdir>0) ||
          ((player[(int)self->playerindex].keys & FLAG_MOVERIGHT)&&self->xdir<0))	self->xdir = -self->xdir;

		if((player[(int)self->playerindex].keys & FLAG_MOVELEFT) && (!self->xdir))
		{
			self->xdir -= self->modeldata.speed;
		}
		else if((player[(int)self->playerindex].keys & FLAG_MOVERIGHT) && (!self->xdir))
		{
			self->xdir = self->modeldata.speed;
		}
	}
    if(self->modeldata.jumpmovez&2) //z move?
    {
       if(((player[(int)self->playerindex].keys & FLAG_MOVEUP)&&self->zdir>0) ||
          ((player[(int)self->playerindex].keys & FLAG_MOVEDOWN)&&self->zdir<0)) self->zdir = -self->zdir;
    }
	if(self->modeldata.jumpmovez&4) //Move z if vertical jump?
	{
		if((player[(int)self->playerindex].keys & FLAG_MOVELEFT)) self->direction = 0;
       else if((player[(int)self->playerindex].keys & FLAG_MOVERIGHT)) self->direction = 1;

		if(((player[(int)self->playerindex].keys & FLAG_MOVEUP)&&self->zdir>0) ||
          ((player[(int)self->playerindex].keys & FLAG_MOVEDOWN)&&self->zdir<0)) self->zdir = -self->zdir;

		if((player[(int)self->playerindex].keys & FLAG_MOVEUP) && (!self->zdir))
		{
			self->zdir -= (0.5 * self->modeldata.speed);
		}
		else if((player[(int)self->playerindex].keys & FLAG_MOVEDOWN) && (!self->zdir))
		{
			self->zdir = (0.5 * self->modeldata.speed);
		}
	}

	//OX Rest of cancel checking

	if(self->animation->cancel==3)
		{
			if((player[(int)self->playerindex].playkeys & FLAG_JUMP) && check_combo(FLAG_JUMP))
			{
				player[(int)self->playerindex].playkeys -= FLAG_JUMP;
				self->jumping = 1;
				self->takeaction = common_jump;
				return;
			}
			if((player[(int)self->playerindex].playkeys & FLAG_SPECIAL) && check_combo(FLAG_SPECIAL))
			{
				player[(int)self->playerindex].playkeys -= FLAG_SPECIAL;
				self->jumping = 1;
				self->takeaction = common_jump;
				return;
			}
			if((player[(int)self->playerindex].playkeys & FLAG_ATTACK2) && check_combo(FLAG_ATTACK2))
			{
				player[(int)self->playerindex].playkeys -= FLAG_ATTACK2;
				//set_jumping(self);
				self->jumping = 1;
				self->takeaction = common_jump;
				return;
			}
			if((player[(int)self->playerindex].playkeys & FLAG_ATTACK3) && check_combo(FLAG_ATTACK3))
			{
				player[(int)self->playerindex].playkeys -= FLAG_ATTACK3;
				self->jumping = 1;
				self->takeaction = common_jump;
				return;
			}
			if((player[(int)self->playerindex].playkeys & FLAG_ATTACK4) && check_combo(FLAG_ATTACK4))
			{
				player[(int)self->playerindex].playkeys -= FLAG_ATTACK4;
				self->jumping = 1;
				self->takeaction = common_jump;
				return;
			}
		}

	// End cancel checking.

    player_preinput();
}

void player_pain_check()
{
    if(player_check_special())  self->inpain = 0;
    player_preinput();
}

// check riseattack input up+attack
void player_lie_check()
{
    if(validanim(self,ANI_RISEATTACK) &&
       (player[(int)self->playerindex].playkeys & FLAG_ATTACK) &&
       (player[(int)self->playerindex].keys & FLAG_MOVEUP) &&
	   (self->health > 0 && time > self->staydown[2]))
    {
        player[(int)self->playerindex].playkeys -= FLAG_ATTACK;
        if((player[(int)self->playerindex].keys & FLAG_MOVELEFT))
        {
            self->direction = 0;
        }
        if((player[(int)self->playerindex].keys & FLAG_MOVERIGHT))
        {
            self->direction = 1;
        }
        self->stalltime = 0;
        set_riseattack(self, self->damagetype, 0);
    }
    player_preinput();
}

void player_charge_check()
{
    if(!((player[(int)self->playerindex].keys&FLAG_JUMP)&&
         (player[(int)self->playerindex].keys&FLAG_SPECIAL)))
    {
        self->charging = 0;
        set_idle(self);
        self->takeaction = NULL;
    }
}

void player_preinput()
{
	float altdiff ; // Used to check that
    int notinair;   // entity is not in the air

    if(player[(int)self->playerindex].playkeys & FLAG_MOVEUP)
    {
        player[(int)self->playerindex].playkeys -= FLAG_MOVEUP;
        self->lastdir = 0;

        if(check_combo(FLAG_MOVEUP)) ++self->movestep;    // Check the combo and increase movestep if valid
        else self->movestep = 0;

        self->lastmove = FLAG_MOVEUP;
        self->movetime = time + (GAME_SPEED/4);
        return ;
    }
    if(player[(int)self->playerindex].playkeys & FLAG_MOVEDOWN)
    {
        player[(int)self->playerindex].playkeys -= FLAG_MOVEDOWN;
        self->lastdir = 0;

        if(check_combo(FLAG_MOVEDOWN)) ++self->movestep;    // Check the combo and increase movestep if valid
        else self->movestep = 0;

        self->lastmove = FLAG_MOVEDOWN;
        self->movetime = time + (GAME_SPEED/4);
        return ;
    }
    if((player[(int)self->playerindex].playkeys & FLAG_MOVELEFT))
    {
        player[(int)self->playerindex].playkeys -= FLAG_MOVELEFT;
        self->lastdir = FLAG_MOVELEFT;

        if(!self->direction && check_combo(FLAG_FORWARD))  ++self->movestep;    // Check direction to distinguish forward/backward movements
        else if(self->direction && check_combo(FLAG_BACKWARD))  ++self->movestep;    // Check direction to distinguish forward/backward movements
        else self->movestep = 0;

        if(self->direction) self->lastmove = FLAG_BACKWARD;
        else self->lastmove = FLAG_FORWARD;

        self->movetime = time + (GAME_SPEED/4);
        return ;
    }
    if((player[(int)self->playerindex].playkeys & FLAG_MOVERIGHT))
    {
        player[(int)self->playerindex].playkeys -= FLAG_MOVERIGHT;
        self->lastdir = FLAG_MOVERIGHT;

        if(!self->direction && check_combo(FLAG_BACKWARD))  ++self->movestep;    // Check direction to distinguish forward/backward movements
        else if(self->direction && check_combo(FLAG_FORWARD))  ++self->movestep;    // Check direction to distinguish forward/backward movements
        else self->movestep = 0;

        if(!self->direction) self->lastmove = FLAG_BACKWARD;
        else self->lastmove = FLAG_FORWARD;

        self->movetime = time + (GAME_SPEED/4);
        return ;
    }

	// OX Cancel checking.

	altdiff = diff(self->a, self->base);
    notinair = (self->landed_on_platform?altdiff<5:altdiff<2);

	if(self->attacking && self->animation->cancel==3 && notinair)
		{
			if((player[(int)self->playerindex].playkeys & FLAG_ATTACK) && check_combo(FLAG_ATTACK))
			{
				player[(int)self->playerindex].playkeys -= FLAG_ATTACK;
				self->attacking = 1;
			    self->takeaction = common_attack_proc;
				return;
			}
			if((player[(int)self->playerindex].playkeys & FLAG_JUMP) && check_combo(FLAG_JUMP))
			{
				player[(int)self->playerindex].playkeys -= FLAG_JUMP;
				self->attacking = 1;
			    self->takeaction = common_attack_proc;
				return;
			}
			if((player[(int)self->playerindex].playkeys & FLAG_SPECIAL) && check_combo(FLAG_SPECIAL))
			{
				player[(int)self->playerindex].playkeys -= FLAG_SPECIAL;
				self->attacking = 1;
			    self->takeaction = common_attack_proc;
				return;
			}
			if((player[(int)self->playerindex].playkeys & FLAG_ATTACK2) && check_combo(FLAG_ATTACK2))
			{
				player[(int)self->playerindex].playkeys -= FLAG_ATTACK2;
				self->attacking = 1;
			    self->takeaction = common_attack_proc;
				return;
			}
			if((player[(int)self->playerindex].playkeys & FLAG_ATTACK3) && check_combo(FLAG_ATTACK3))
			{
				player[(int)self->playerindex].playkeys -= FLAG_ATTACK3;
				self->attacking = 1;
			    self->takeaction = common_attack_proc;
				return;
			}
			if((player[(int)self->playerindex].playkeys & FLAG_ATTACK4) && check_combo(FLAG_ATTACK4))
			{
				player[(int)self->playerindex].playkeys -= FLAG_ATTACK4;
				self->attacking = 1;
			    self->takeaction = common_attack_proc;
				return;
			}
		}

	// End cancel checking.
 }

void player_think()
{
	int action = 0;		// 1=walking, 2=up, 3=down, 4=running
    int bkwalk = 0;   //backwalk
    entity *other = NULL;
    float altdiff ;
    float seta ;
    int notinair;

    if(player[(int)self->playerindex].ent != self || self->dead) return;

    seta = (float)(self->animation->seta?self->animation->seta[self->animpos]:-1);
    // check endlevel item
    if((other = find_ent_here(self, self->x, self->z, TYPE_ENDLEVEL)) && self->a -
        (seta >= 0? seta:0) == other->a)
    {
        if(!reached[0] && !reached[1] && !reached[2] && !reached[3]) addscore(self->playerindex, other->modeldata.score);
        reached[(int)self->playerindex] = 1;

        if (!other->modeldata.subtype ||(other->modeldata.subtype == SUBTYPE_BOTH &&
            (reached[0]+reached[1]+reached[2]+reached[3]) >= (count_ents(TYPE_PLAYER))))
        {
            level_completed = 1;

            if(other->modeldata.branch) strncpy( branch_name, other->modeldata.branch, MAX_NAME_LEN); //now, you can branch to another level
            return;
        }
    }

    if(time > player[(int)self->playerindex].ent->rushtime)
    {
        player[(int)self->playerindex].ent->rush[0] = 0;
        player[(int)self->playerindex].ent->rushtime = 0;
    }

    if(self->charging)
    {
        player_charge_check();
        return;
    }

    if(self->inpain || (self->link && !self->grabbing))
    {
        player_pain_check();
        return;
    }

    // falling? check for landing
    if(self->projectile == 2)
    {
        player_fall_check();
        return;
    }

    // grab section, dont move if still animating
    if(self->grabbing && !self->attacking && self->takeaction!=common_throw_wait)
    {
        player_grab_check();
        return;
    }

    // jump section
    if(self->jumping)
    {
        player_jump_check();
        return;
    }

    if(self->drop && self->a==self->base && !self->tossv)
    {
        player_lie_check();
        return;
    }

    // cant do anything if busy
	if(!self->idling && !(self->animation->idle && self->animation->idle[self->animpos]))
    {
        player_preinput();
        return;
    }

    // Check if entity is under a platform
    if(self->modeldata.subject_to_platform>0 && validanim(self,ANI_DUCK) && check_platform_between(self->x/*+self->direction*2-1*/, self->z, self->a, self->a+self->modeldata.height) && (check_platform_between(self->x/*+self->direction*2-1*/, self->z, self->a, self->a+self->animation->height) || !self->animation->height) )
    {
        ent_set_anim(self, ANI_DUCK, 1);
        self->takeaction = common_stuck_underneath;
        return;
    }

    altdiff = diff(self->a, self->base);
    notinair = (self->landed_on_platform?altdiff<5:altdiff<2);

    // Changed the way combos are checked so combos can be customized
    if(player[(int)self->playerindex].playkeys & FLAG_MOVEUP)
    {
        player[(int)self->playerindex].playkeys -= FLAG_MOVEUP;
        self->lastdir = 0;
        if(time < self->movetime && self->lastmove==FLAG_MOVEUP && validanim(self,ANI_ATTACKUP) && notinair)
        {    // New u u combo attack
            set_attacking(self);
            self->combostep[0] = 0;
            self->xdir = self->zdir = 0;
            ent_set_anim(self, ANI_ATTACKUP, 0);
            self->takeaction = common_attack_proc;
            return;
        }
        else if(time < self->movetime && self->lastmove==FLAG_MOVEUP && validanim(self,ANI_DODGE) && notinair)
        {    // New dodge move like on SOR3
            self->combostep[0] = 0;
            self->idling = 0;
            self->zdir = (float)-0.1;
            ent_set_anim(self, ANI_DODGE, 0);
            self->takeaction = common_dodge;
            return;
        }
        else if(check_combo(FLAG_MOVEUP)) ++self->movestep;    // Check the combo and increase movestep if valid
        else self->movestep = 0;

        self->lastmove = FLAG_MOVEUP;
        self->movetime = time + (GAME_SPEED/4);
    }

    if(player[(int)self->playerindex].playkeys & FLAG_MOVEDOWN)
    {
        player[(int)self->playerindex].playkeys -= FLAG_MOVEDOWN;
        self->lastdir = 0;
        if(self->lastmove==FLAG_MOVEDOWN && validanim(self,ANI_ATTACKDOWN) && time < self->movetime && notinair)
        {    // New d d combo attack
            set_attacking(self);
            self->xdir = self->zdir = 0;
            self->combostep[0] = 0;
            ent_set_anim(self, ANI_ATTACKDOWN, 0);
            self->takeaction = common_attack_proc;
            return;
        }
        else if(time < self->movetime && self->lastmove == FLAG_MOVEDOWN && validanim(self,ANI_DODGE) && notinair)
        {    // New dodge move like on SOR3
            self->combostep[0] = 0;
            self->idling = 0;
            self->zdir = (float)0.1;          //used for checking
            ent_set_anim(self, ANI_DODGE, 0);
            self->takeaction = common_dodge;
            return;
        }
        else if(check_combo(FLAG_MOVEDOWN))  ++self->movestep;    // Check the combo and increase movestep if valid
        else self->movestep = 0;

        self->lastmove = FLAG_MOVEDOWN;
        self->movetime = time + (GAME_SPEED/4);
    }

    // Left/right movement for combos is more complicated because forward/backward is relative to the direction the player is facing
    // Checks on current direction have to be made before and after executing the combo to make sure they get the right one
    if((player[(int)self->playerindex].playkeys & FLAG_MOVELEFT))
    {
        player[(int)self->playerindex].playkeys -= FLAG_MOVELEFT;
        if(validanim(self,ANI_RUN) && !self->direction && time < self->movetime && self->lastdir==FLAG_MOVELEFT)
            self->running = 1;    // Player begins to run

        else if(validanim(self,ANI_ATTACKFORWARD) && !self->direction && time < self->movetime && self->lastdir==FLAG_MOVELEFT)
        {
            set_attacking(self);
            self->xdir = self->zdir = 0;
            self->combostep[0] = 0;
            ent_set_anim(self, ANI_ATTACKFORWARD, 0);
            self->takeaction = common_attack_proc;
            return;
        }
        else if(!self->direction && check_combo(FLAG_FORWARD))  ++self->movestep;    // Check direction to distinguish forward/backward movements
        else if(self->direction && check_combo(FLAG_BACKWARD))  ++self->movestep;    // Check direction to distinguish forward/backward movements
        else self->movestep = 0;

        if(self->direction) self->lastmove = FLAG_BACKWARD;
        else self->lastmove = FLAG_FORWARD;
        self->lastdir = FLAG_MOVELEFT;

        self->movetime = time + (GAME_SPEED/4);
    }

    if((player[(int)self->playerindex].playkeys & FLAG_MOVERIGHT))
    {
        player[(int)self->playerindex].playkeys -= FLAG_MOVERIGHT;
        if(validanim(self,ANI_RUN) && self->direction && time < self->movetime &&  self->lastdir==FLAG_MOVERIGHT)
            self->running = 1;    // Player begins to run

        else if(validanim(self,ANI_ATTACKFORWARD) &&self->direction && time < self->movetime &&  self->lastdir==FLAG_MOVERIGHT)
        {
            set_attacking(self);
            self->xdir = self->zdir = 0;
            self->combostep[0] = 0;
            ent_set_anim(self, ANI_ATTACKFORWARD, 0);
            self->takeaction = common_attack_proc;
            return;
        }
        else if(!self->direction && check_combo(FLAG_BACKWARD))  ++self->movestep;    // Check direction to distinguish forward/backward movements
        else if(self->direction && check_combo(FLAG_FORWARD))  ++self->movestep;    // Check direction to distinguish forward/backward movements
        else self->movestep = 0;

        if(!self->direction) self->lastmove = FLAG_BACKWARD;
        else self->lastmove = FLAG_FORWARD;
        self->lastdir = FLAG_MOVERIGHT;

        self->movetime = time + (GAME_SPEED/4);
    }

    if(!ajspecial && (player[(int)self->playerindex].playkeys & FLAG_JUMP) && validanim(self,ANI_ATTACKBOTH))
    {
        if((player[(int)self->playerindex].keys & FLAG_ATTACK) && notinair)
        {
            player[(int)self->playerindex].playkeys -= FLAG_JUMP;
            set_attacking(self);
            self->xdir = self->zdir = 0;
            self->combostep[0] = 0;
            self->movestep = 0;
            self->stalltime = 0;    // If attack is pressed, holding down attack to execute attack3 is no longer valid
            ent_set_anim(self, ANI_ATTACKBOTH, 0);
            self->takeaction = common_attack_proc;
            return;
        }
    }

    if((player[(int)self->playerindex].playkeys & FLAG_JUMP) &&  validanim(self,ANI_CHARGE))
    {
        if((player[(int)self->playerindex].playkeys & FLAG_SPECIAL) && notinair)
        {
            self->combostep[0] = 0;
            self->movestep = 0;
            self->xdir = self->zdir = 0;
            self->stalltime = 0;
            set_charging(self);
            ent_set_anim(self, ANI_CHARGE, 0);
            self->takeaction = common_charge;
            return;
        }
    }

    if(player[(int)self->playerindex].playkeys & FLAG_SPECIAL )    //    The special button can now be used for freespecials
    {
        if( validanim(self,ANI_SPECIAL2) && notinair &&
            (!self->direction ?
            (player[(int)self->playerindex].keys & FLAG_MOVELEFT) :
             (player[(int)self->playerindex].keys & FLAG_MOVERIGHT))  )
        {
            if(check_costmove(ANI_SPECIAL2, 0))
            {
                player[(int)self->playerindex].playkeys -= FLAG_SPECIAL;
                return;
            }
        }

        if(check_combo(FLAG_SPECIAL))
        {
            player[(int)self->playerindex].playkeys -= FLAG_SPECIAL;
            return;
        }

        if(validanim(self,ANI_BLOCK) && !self->modeldata.holdblock && notinair)    // New block code for players
        {
            player[(int)self->playerindex].playkeys -= FLAG_SPECIAL;
            self->xdir = self->zdir = 0;
            set_blocking(self);
            self->combostep[0] = 0;
            self->movestep = 0;
            ent_set_anim(self, ANI_BLOCK, 0);
            self->takeaction = common_block;
            return;
        }
    }

    if(notinair && player_check_special()) return;    // So you don't perform specials falling off the edge

    if((player[(int)self->playerindex].releasekeys & FLAG_ATTACK))
    {
        if(self->stalltime  && notinair &&
	      ((validanim(self,ANI_CHARGEATTACK) && self->stalltime+(GAME_SPEED*self->modeldata.animation[ANI_CHARGEATTACK]->chargetime) < time) ||
		   (!validanim(self,ANI_CHARGEATTACK) && self->stalltime+(GAME_SPEED*self->modeldata.animation[animattacks[self->modeldata.atchain[self->modeldata.chainlength-1]-1]]->chargetime) < time)))
        {
            set_attacking(self);
            self->xdir = self->zdir = 0;
            self->combostep[0] = 0;

			if(validanim(self,ANI_CHARGEATTACK)) ent_set_anim(self, ANI_CHARGEATTACK, 0);
			else ent_set_anim(self, animattacks[self->modeldata.atchain[self->modeldata.chainlength-1]-1], 0);

			if(SAMPLE_PUNCH >= 0) sound_play_sample(SAMPLE_PUNCH, 0, savedata.effectvol,savedata.effectvol, 100);

			self->stalltime = 0;
            self->takeaction = common_attack_proc;
            return;
        }
        self->stalltime = 0;
    }

    if((player[(int)self->playerindex].playkeys & FLAG_ATTACK)  && notinair)
    {
        player[(int)self->playerindex].playkeys -= FLAG_ATTACK;
        self->stalltime = 0;    // Disable the attack3 stalltime

        if(player[(int)self->playerindex].keys & FLAG_MOVEDOWN && validanim(self, ANI_DUCKATTACK) && PLAYER_MIN_Z == PLAYER_MAX_Z)
        {
            set_attacking(self);
            self->xdir = self->zdir = 0;
            self->combostep[0] = 0;
            ent_set_anim(self, ANI_DUCKATTACK, 0);
            self->takeaction = common_attack_proc;
            return;
        }

        if(self->running && validanim(self,ANI_RUNATTACK))    // New run attack code section
        {
            player[(int)self->playerindex].playkeys -= FLAG_SPECIAL;
            set_attacking(self);
            self->xdir = self->zdir = 0;
            self->combostep[0] = 0;
            self->running = 0;
            ent_set_anim(self, ANI_RUNATTACK, 0);
            self->takeaction = common_attack_proc;
            return;
        }

        // Perform special move, Now checks custom combos
        if(check_combo(FLAG_ATTACK))
        {
            //player[(int)self->playerindex].playkeys -= FLAG_SPECIAL;
            return;
        }

        if(validanim(self,ANI_ATTACKBACKWARD) && time < self->movetime - (GAME_SPEED/10) &&
            !self->movestep && self->lastmove == FLAG_BACKWARD)    // New back attacks
        {
            set_attacking(self);
            self->xdir = self->zdir = 0;
            if(self->direction && (player[(int)self->playerindex].keys & FLAG_MOVERIGHT)) self->direction = 0;    // Since the back part of the combo will flip the player, need to flip them back around
            else if(!self->direction && (player[(int)self->playerindex].keys & FLAG_MOVELEFT)) self->direction = 1;    // Since the back part of the combo will flip the player, need to flip them back around

            self->combostep[0] = 0;
            ent_set_anim(self, ANI_ATTACKBACKWARD, 0);
            self->takeaction = common_attack_proc;
            return;
        }

        if( (other = find_ent_here(self, self->x, self->z, TYPE_ITEM)) &&
            other->modeldata.subtype != SUBTYPE_TOUCH && !other->blink &&
            diff(self->a - (seta >= 0) * seta , other->a)<0.1  )
        {
            if( validanim(self,ANI_GET) && // so we wont get stuck
                 //dont pickup a weapon that is not in weapon list
                !((other->modeldata.subtype==SUBTYPE_WEAPON && self->modeldata.weapon && (*self->modeldata.weapon)[other->modeldata.weapnum-1]<0) ||
                //if on an real animal, can't pick up weapons
                (self->modeldata.animal==2 && other->modeldata.subtype == SUBTYPE_WEAPON)))
            {
                didfind_item(other);
                self->xdir = self->zdir = 0;
                set_getting(self);
                ent_set_anim(self, ANI_GET, 0);
                self->takeaction = common_get;
                execute_didhit_script(other, self, 0, 0, other->modeldata.subtype, 0, 0, 0, 0, 0); //Execute didhit script as if item "hit" collecter to allow easy item scripting.
                return;
            }
        }

        // Use stalltime to charge end-move
        self->stalltime = time;
        self->xdir = self->zdir = 0;

        if(!validanim(self,ANI_ATTACK1) && validanim(self,ANI_JUMP))
        {
            // This is for Mighty
            self->combostep[0] = 0;
            tryjump(self->modeldata.jumpheight, self->modeldata.jumpspeed, 0, ANI_JUMP);
            return;
        }
        if( self->weapent &&
            self->weapent->modeldata.subtype == SUBTYPE_PROJECTILE &&
            validanim(self,ANI_THROWATTACK)  )
        {
            set_attacking(self);
            ent_set_anim(self, ANI_THROWATTACK, 0);
            self->takeaction = common_attack_proc;
        }
        else if(perform_atchain())
        {
            if(SAMPLE_PUNCH >= 0 && self->attacking) sound_play_sample(SAMPLE_PUNCH, 0, savedata.effectvol,savedata.effectvol, 100);
        }

        return;
    }
    // 7-1-2005 spawn projectile end

    // Mighty hass no attack animations, he just jumps.
    if(player[(int)self->playerindex].playkeys & FLAG_JUMP  && notinair)
    {    // Added !inair(self) so players can't jump when falling into holes
        player[(int)self->playerindex].playkeys -= FLAG_JUMP;

        if(self->running)
        {
            //Slide
            if((player[(int)self->playerindex].keys & FLAG_MOVEDOWN) && validanim(self,ANI_RUNSLIDE))
            {
                set_attacking(self);
                self->xdir = self->zdir = 0;
                self->combostep[0] = 0;
                self->running = 0;
                ent_set_anim(self, ANI_RUNSLIDE, 0);
                self->takeaction = common_attack_proc;
                return;
            }

            if(validanim(self,ANI_RUNJUMP))
                tryjump(self->modeldata.runjumpheight, self->modeldata.jumpspeed*self->modeldata.runjumpdist, (self->modeldata.jumpmovez)?self->zdir:0, ANI_RUNJUMP);
            else if(validanim(self,ANI_FORWARDJUMP))
                tryjump(self->modeldata.runjumpheight, self->modeldata.jumpspeed*self->modeldata.runjumpdist, (self->modeldata.jumpmovez)?self->zdir:0, ANI_FORWARDJUMP);
            else if(validanim(self,ANI_JUMP))
                tryjump(self->modeldata.runjumpheight, self->modeldata.jumpspeed*self->modeldata.runjumpdist, (self->modeldata.jumpmovez)?self->zdir:0, ANI_JUMP);
        }
        else
        {
            if(check_combo(FLAG_JUMP))
            {    // Jump can now be used with freespecials
                //player[(int)self->playerindex].playkeys -= FLAG_SPECIAL;
                return;
            }
            else
            {
                //Slide
                if((player[(int)self->playerindex].keys & FLAG_MOVEDOWN) && validanim(self,ANI_SLIDE))
                {
                    set_attacking(self);
                    self->xdir = self->zdir = 0;
                    self->combostep[0] = 0;
                    self->running = 0;
                    ent_set_anim(self, ANI_SLIDE, 0);
                    self->takeaction = common_attack_proc;
                    return;
                }

                if(!(player[(int)self->playerindex].keys & (FLAG_MOVELEFT|FLAG_MOVERIGHT)) && validanim(self,ANI_JUMP))
                {
                    tryjump(self->modeldata.jumpheight, 0, (self->modeldata.jumpmovez)?self->zdir:0, ANI_JUMP);
                    return;
                }
                else if((player[(int)self->playerindex].keys & FLAG_MOVELEFT)) self->direction = 0;
                else if((player[(int)self->playerindex].keys & FLAG_MOVERIGHT)) self->direction = 1;

                if(validanim(self,ANI_FORWARDJUMP))
                    tryjump(self->modeldata.jumpheight, self->modeldata.jumpspeed, (self->modeldata.jumpmovez)?self->zdir:0, ANI_FORWARDJUMP);
                else if(validanim(self,ANI_JUMP)) tryjump(self->modeldata.jumpheight, self->modeldata.jumpspeed, (self->modeldata.jumpmovez)?self->zdir:0, ANI_JUMP);
            }
        }
        return;
    }

    if(validanim(self,ANI_BLOCK) && self->modeldata.holdblock &&
       player[(int)self->playerindex].keys & FLAG_SPECIAL && notinair)
    {
        player[(int)self->playerindex].playkeys -= FLAG_SPECIAL;
        if(!self->blocking )
        {
            self->blocking = 1;
            self->xdir = self->zdir = 0;
            ent_set_anim(self, ANI_BLOCK, 0);
        }
        return;
    }
    else
    {
        self->blocking = 0;
    }

    // check attack2 - attack4 freespecial
    if(notinair)
    {
        if((player[(int)self->playerindex].playkeys & FLAG_ATTACK2) && check_combo(FLAG_ATTACK2))
        {
            player[(int)self->playerindex].playkeys -= FLAG_ATTACK2;
            return;
        }
        if((player[(int)self->playerindex].playkeys & FLAG_ATTACK3) && check_combo(FLAG_ATTACK3))
        {
            player[(int)self->playerindex].playkeys -= FLAG_ATTACK3;
            return;
        }
        if((player[(int)self->playerindex].playkeys & FLAG_ATTACK4) && check_combo(FLAG_ATTACK4))
        {
            player[(int)self->playerindex].playkeys -= FLAG_ATTACK4;
            return;
        }
    }

    if(PLAYER_MIN_Z != PLAYER_MAX_Z)
    {    // More of a platform feel
        if(player[(int)self->playerindex].keys & FLAG_MOVEUP)
        {
            if(!self->modeldata.runupdown ) self->running = 0;    // Quits running if player presses up (or the up animation exists

            if(validanim(self,ANI_UP) && !self->running)
            {
                action = 2;
                self->zdir = -self->modeldata.speed/2;    // Used for up animation
            }
            else if(self->running)
            {
                action = 4;
                 self->zdir = -self->modeldata.runspeed/2;    // Moves up at a faster rate running
            }
            else
            {
                action = 1;
                self->zdir = -self->modeldata.speed/2;
            }
        }
        else if(player[(int)self->playerindex].keys & FLAG_MOVEDOWN)
        {
            if(!self->modeldata.runupdown ) self->running = 0;    // Quits running if player presses down (or the down animation exists

            if(validanim(self,ANI_DOWN) && !self->running )
            {
                action = 3;
                self->zdir = self->modeldata.speed/2;    // Used for down animation
            }
            else if(self->running)
            {
                action = 4;
                self->zdir = self->modeldata.runspeed/2;    // Moves down at a faster rate running
            }
            else
            {
                action = 1;
                self->zdir = self->modeldata.speed/2;
            }
        }
        else if(!(player[(int)self->playerindex].keys & (FLAG_MOVEUP|FLAG_MOVEDOWN)))
            self->zdir = 0;
    }
    else if(validanim(self,ANI_DUCK) && player[(int)self->playerindex].keys & FLAG_MOVEDOWN  && notinair)
    {
        ent_set_anim(self, ANI_DUCK, 0);
        self->xdir = self->zdir = 0;
        return;
    }

    if(player[(int)self->playerindex].keys & FLAG_MOVELEFT)
    {
        if(self->direction)
        {
            self->running = 0;    // Quits running if player changes direction
            if(self->modeldata.turndelay && !self->turntime)
                self->turntime = time + self->modeldata.turndelay;
            else if(self->turntime && time >= self->turntime)
            {
                self->turntime = 0;
                if(validanim(self,ANI_TURN))
                {
                    set_turning(self);
                    ent_set_anim(self, ANI_TURN, 0);
                    self->takeaction = common_turn;
                    return;
                }
                self->direction = 0;
            }
            else if(!self->modeldata.turndelay && validanim(self,ANI_TURN))
            {
                set_turning(self);
                ent_set_anim(self, ANI_TURN, 0);
                self->takeaction = common_turn;
                return;
            }
            else if(!self->turntime) self->direction = 0;
        }
        else  self->turntime = 0;

        if(self->running)
        {
            action = 4;
            self->xdir = -self->modeldata.runspeed;    // If running, player moves at a faster rate
        }
        else if(action!=2 && action != 3)
        {
            action = 1;
            self->xdir = -self->modeldata.speed;
        }
        else
        {
            self->xdir = -self->modeldata.speed;
        }
    }
    else if(player[(int)self->playerindex].keys & FLAG_MOVERIGHT)
    {
        if(!self->direction)
        {
            self->running = 0;    // Quits running if player changes direction
            if(self->modeldata.turndelay && !self->turntime)
                self->turntime = time + self->modeldata.turndelay;
            else if(self->turntime && time >= self->turntime)
            {
                self->turntime = 0;
                if(validanim(self,ANI_TURN))
                {
                    set_turning(self);
                    ent_set_anim(self, ANI_TURN, 0);
                    self->takeaction = common_turn;
                    return;
                }
                self->direction = 1;
            }
            else if(!self->modeldata.turndelay && validanim(self,ANI_TURN))
            {
                set_turning(self);
                ent_set_anim(self, ANI_TURN, 0);
                self->takeaction = common_turn;
                return;
            }
            else if(!self->turntime) self->direction = 1;
        }
        else  self->turntime = 0;

        if(self->running)
        {
            action = 4;
            self->xdir = self->modeldata.runspeed;    // If running, player moves at a faster rate
        }
        else if(action!=2 && action != 3)
        {
            action = 1;
            self->xdir = self->modeldata.speed;
        }
        else
        {
            self->xdir = self->modeldata.speed;
        }
    }
    else if(!((player[(int)self->playerindex].keys & FLAG_MOVELEFT) ||
        (player[(int)self->playerindex].keys & FLAG_MOVERIGHT)) )
    {
        self->running = 0;    // Player let go of left/right and so quits running
        self->xdir = 0;
        self->turntime = 0;
    }

    //    ltb 1-18-05  new Item get code to address new subtype

    if((other = find_ent_here(self, self->x, self->z, TYPE_ITEM)) && other->modeldata.subtype == SUBTYPE_TOUCH && !other->blink &&
		diff(self->a - (seta >= 0) * seta , other->a)<0.1 && action)
    {
        didfind_item(other);    // Added function to clean code up a bit
    }

	switch(action)
    {
		case 1:
        // back walk feature
			if(level && validanim(self,ANI_BACKWALK))
			{
            if(self->modeldata.facing == 1 || level->facing == 1)  bkwalk = !self->direction;
            else if(self->modeldata.facing == 2 || level->facing == 2) bkwalk = self->direction;
            else if((self->modeldata.facing == 3 || level->facing == 3) && (level->scrolldir & SCROLL_LEFT) && !self->direction ) bkwalk = 1;
            else if((self->modeldata.facing == 3 || level->facing == 3) && (level->scrolldir & SCROLL_RIGHT) && self->direction) bkwalk = 1;
            else if(self->turntime && self->modeldata.turndelay) bkwalk = 1;
            if(bkwalk) common_backwalk_anim(self); //ent_set_anim(self, ANI_BACKWALK, 0);
            else common_walk_anim(self); //ent_set_anim(self, ANI_WALK, 0);    // If neither up nor down exist, set to walk
        }
        else common_walk_anim(self); //ent_set_anim(self, ANI_WALK, 0);    // If neither up nor down exist, set to walk
			break;
		case 2:
            common_up_anim(self); //ent_set_anim(self, ANI_UP, 0);    // Set to up animation if exists
			break;
		case 3:
		    common_down_anim(self); //ent_set_anim(self, ANI_DOWN, 0);    // Set to down animation if exists
			break;
		case 4:
            ent_set_anim(self, ANI_RUN, 0);    // Set to run animation if exists
			break;
		default:
			if(self->idling)
			{
				common_idle_anim(self);
				return;
			}
			break;
    }
	if(action)
	{
		self->takeaction = NULL;
		self->idling = 1;
	}
}

int common_idle_anim(entity* ent)
{
    /*
    common_idle_anim
    Damon Vaughn Caskey
    11012009
    Determine and set appropriate idle animation based on condition and range.
    Returns 1 if any animation is set.
    */

    int i;                                                                              //Loop counter.
    int iAni;                                                                           //Animation.

    if (ent->model->subtype != SUBTYPE_BIKER) // biker fix by Plombo
    	ent->xdir = ent->zdir = 0;                                                      //Stop movement.

    if(validanim(ent,ANI_FAINT) && ent->health <= ent->modeldata.health / 4)            //ANI_FAINT and health at/below 25%?
    {
        ent_set_anim(ent, ANI_FAINT, 0);                                                //Set ANI_FAINT.
        return 1;                                                                       //Return 1 and exit.
    }
    else if(validanim(ent,ANI_SLEEP) && (time >= ent->sleeptime) && ent->animating)     //ANI_SLEEP, sleeptime up and currently animating?
    {
        ent_set_anim(ent, ANI_SLEEP, 0);                                                //Set sleep anim.
        return 1;                                                                       //Return 1 and exit.
    }
	else
    {
        for (i=0; i<max_idles; i++)                                                     //Loop through all idle animations.
        {
            iAni = animidles[i];                                                        //Get current animation.

            if (validanim(ent, iAni) && iAni != ANI_IDLE)                               //Valid and not ANI_IDLE?
            {
                if (normal_find_target(iAni))                                           //Opponent in range of current animation?
                {
                    ent_set_anim(ent, iAni, 0);                                         //Set animation.
                    return 1;                                                           //Return 1 and exit.
                }
            }
        }

        if (validanim(ent, ANI_IDLE))
        {
            ent_set_anim(ent, ANI_IDLE, 0);                                             //No alternates were set. Set ANI_IDLE.
            return 1;                                                                   //Return 1 and exit.
        }
    }

    return 0;
}

int common_walk_anim(entity* ent)
{
    /*
    common_walk_anim
    Damon Vaughn Caskey
    11032009
    Determine and set appropriate walk animation based on condition and range.
    Returns 1 if any animation is set.
    */

    int i;                                                                          //Loop counter.
    int iAni;                                                                       //Animation.

    for (i=0; i<max_walks; i++)                                                     //Loop through all relevant animations.
    {
        iAni = animwalks[i];                                                        //Get current animation.

        if (validanim(ent, iAni) && iAni != ANI_WALK)                               //Valid and not Default animation??
        {
            if (normal_find_target(iAni))                                           //Opponent in range of current animation?
            {
                ent_set_anim(ent, iAni, 0);                                         //Set animation.
                return 1;                                                           //Return 1 and exit.
            }
        }
    }

    if (validanim(ent, ANI_WALK))
    {
        ent_set_anim(ent, ANI_WALK, 0);                                             //No alternates were set. Set default..
        return 1;                                                                   //Return 1 and exit.
    }

    return 0;
}

int common_backwalk_anim(entity* ent)
{
    /*
    common_backwalk_anim
    Damon Vaughn Caskey
    11032009
    Determine and set appropriate backwalk animation based on condition and range.
    Returns 1 if any animation is set.
    */

    int i;                                                                          //Loop counter.
    int iAni;                                                                       //Animation.

    for (i=0; i<max_backwalks; i++)                                                 //Loop through all relevant animations.
    {
        iAni = animbackwalks[i];                                                    //Get current animation.

        if (validanim(ent, iAni) && iAni != ANI_BACKWALK)                           //Valid and not Default animation??
        {
            if (normal_find_target(iAni))                                           //Opponent in range of current animation?
            {
                ent_set_anim(ent, iAni, 0);                                         //Set animation.
                return 1;                                                           //Return 1 and exit.
            }
        }
    }

    if (validanim(ent, ANI_BACKWALK))
    {
        ent_set_anim(ent, ANI_BACKWALK, 0);                                         //No alternates were set. Set default..
        return 1;                                                                   //Return 1 and exit.
    }

    return 0;
}

int common_up_anim(entity* ent)
{
    /*
    common_up_anim
    Damon Vaughn Caskey
    11032009
    Determine and set appropriate up animation based on condition and range.
    Returns 1 if any animation is set.
    */

    int i;                                                                    //Loop counter.
    int iAni;                                                                 //Animation.

    for (i=0; i<max_ups; i++)                                                 //Loop through all relevant animations.
    {
        iAni = animups[i];                                                    //Get current animation.

        if (validanim(ent, iAni) && iAni != ANI_UP)                           //Valid and not Default animation??
        {
            if (normal_find_target(iAni))                                     //Opponent in range of current animation?
            {
                ent_set_anim(ent, iAni, 0);                                   //Set animation.
                return 1;                                                     //Return 1 and exit.
            }
        }
    }

    if (validanim(ent, ANI_UP))
    {
        ent_set_anim(ent, ANI_UP, 0);                                         //No alternates were set. Set default..
        return 1;                                                             //Return 1 and exit.
    }

    return 0;
}

int common_down_anim(entity* ent)
{
    /*
    common_up_anim
    Damon Vaughn Caskey
    11032009
    Determine and set appropriate up animation based on condition and range.
    Returns 1 if any animation is set.
    */

    int i;                                                                    //Loop counter.
    int iAni;                                                                 //Animation.

    for (i=0; i<max_downs; i++)                                               //Loop through all relevant animations.
    {
        iAni = animdowns[i];                                                  //Get current animation.

        if (validanim(ent, iAni) && iAni != ANI_DOWN)                         //Valid and not Default animation??
        {
            if (normal_find_target(iAni))                                     //Opponent in range of current animation?
            {
                ent_set_anim(ent, iAni, 0);                                   //Set animation.
                return 1;                                                     //Return 1 and exit.
            }
        }
    }

    if (validanim(ent, ANI_DOWN))
    {
        ent_set_anim(ent, ANI_DOWN, 0);                                       //No alternates were set. Set default..
        return 1;                                                             //Return 1 and exit.
    }

    return 0;
}

//ammo count goes down
void subtract_shot()
{
    if(self->weapent && self->weapent->modeldata.shootnum)
    {
        self->weapent->modeldata.shootnum--;
        if(!self->weapent->modeldata.shootnum)
        {
            self->weapent->modeldata.counter = 0;
            dropweapon(0);
        }
    }

}


void dropweapon(int flag)
{
    int wall;
    entity* other = NULL;

    if(self->weapent)
    {
        if(self->weapent->modeldata.typeshot || (!self->weapent->modeldata.typeshot && self->weapent->modeldata.shootnum))
        {
            self->weapent->direction = self->direction;//same direction as players, 2007 -2 - 11   by UTunnels
            if(flag < 2) self->weapent->modeldata.counter -= flag;
            self->weapent->z = self->z;
            self->weapent->x = self->x;
            self->weapent->a = self->a;

            other = check_platform(self->weapent->x, self->weapent->z);
            wall = checkwall(self->weapent->x, self->weapent->z);

            if(other && other != self->weapent)   self->weapent->base += other->a + other->animation->platform[other->animpos][7];
            else if(wall >= 0) self->weapent->base += level->walls[wall][7];

            if(validanim(self->weapent,ANI_RESPAWN)) ent_set_anim(self->weapent, ANI_RESPAWN, 0);
            else if(validanim(self->weapent,ANI_SPAWN)) ent_set_anim(self->weapent, ANI_SPAWN, 0);
            else ent_set_anim(self->weapent, ANI_IDLE, 0);

            if(!self->weapent->modeldata.counter)
            {
                if(!self->modeldata.animal)
                {
                    self->weapent->blink = 1;
                    self->weapent->takeaction = common_lie;
                }
                else{
                    self->weapent->modeldata.type = TYPE_NONE;
                    self->weapent->think = runanimal;
                }
                self->weapent->nextthink = time + 1;
            }
        }
        self->weapent = NULL;
    }
    if(flag < 2)
    {
        if(self->modeldata.type == TYPE_PLAYER)
        {
            if(player[(int)self->playerindex].weapnum)
                set_weapon(self, player[(int)self->playerindex].weapnum, 0);
            else set_weapon(self, level->setweap, 0);
        }
        else set_weapon(self, 0, 0);
    }

    if(self->modeldata.weaploss[1]>0)
    {
        set_weapon(self, self->modeldata.weaploss[1], 0);
    }
}


int player_takedamage(entity *other, s_attack* attack)
{
    s_attack atk;
    //printf("damaged by: '%s' %d\n", other->name, attack->attack_force);
    if(healthcheat)
    {
        memcpy(&atk, attack, sizeof(s_attack));
        atk.attack_force = 0;
        return common_takedamage(other, &atk);
    }
    return common_takedamage(other, attack);
}


////////////////////////////////

// Called when player re-enters the game.
// Drop all enemies EXCEPT for the linked/frozen ones.
void drop_all_enemies()
{
    int i;
    entity* weapself = self;
    for(i=0; i<ent_max; i++)
    {
        if(ent_list[i]->exists &&
            ent_list[i]->health>0 &&
            ent_list[i]->modeldata.type==TYPE_ENEMY &&
            !ent_list[i]->owner &&    // Don't want to knock down a projectile
            !ent_list[i]->frozen &&    // Don't want to unfreeze a frozen enemy
            !ent_list[i]->modeldata.nomove &&
            !ent_list[i]->modeldata.nodrop &&
            validanim(ent_list[i],ANI_FALL) )
        {
            ent_list[i]->attacking = 0;
            ent_list[i]->projectile = 0;
            ent_list[i]->takeaction = common_fall;//enemy_fall;
            ent_list[i]->damage_on_landing = 0;
            self = ent_list[i];
            ent_unlink(self);
            ent_list[i]->xdir = (self->direction)?(-1.2):1.2;
            dropweapon(1);
            toss(ent_list[i], 2.5 + randf(1));
            set_fall(ent_list[i], ATK_NORMAL, 1, self, 0, 0, 0, 0, 0, 0);
            ent_list[i]->knockdowncount = ent_list[i]->modeldata.knockdowncount;

            ent_list[i]->knockdowntime = 0;
        }
    }
    self = weapself;
}



// Called when boss dies
void kill_all_enemies()
{
    int i;
    s_attack attack;
    entity * tmpself = NULL;

    attack = emptyattack;
    attack.attack_type = max_attack_types;
    attack.dropv[0] = (float)3; attack.dropv[1] = (float)1.2; attack.dropv[2] = (float)0;

    tmpself = self;
    for(i=0; i<ent_max; i++)
    {
        if(  ent_list[i]->exists
            && ent_list[i]->health>0
            && ent_list[i]->modeldata.type==TYPE_ENEMY
            && ent_list[i]->takedamage)
        {
            self = ent_list[i];
            attack.attack_force = self->health;
            self->takedamage(tmpself, &attack);
            self->dead = 1;
        }
    }
    self = tmpself;
}



void smart_bomb(entity* e, s_attack* attack)    // New method for smartbombs
{
    int i, hostile, hit=0;
    entity * tmpself = NULL;

    hostile = e->modeldata.hostile;
    if(e->modeldata.type == TYPE_PLAYER)
        hostile &= ~(TYPE_PLAYER);

    tmpself = self;
    for(i=0; i<ent_max; i++)
    {
        if( ent_list[i]->exists
            && ent_list[i] != e
            && ent_list[i]->health>0
            && (ent_list[i]->modeldata.type&(e->modeldata.hostile)))
        {
            self = ent_list[i];
            hit = 1; // for nocost, if the bomb doesn't hit, it won't cost energy
            if(self->takedamage)
            {
                //attack.attack_drop = self->modeldata.knockdowncount+1;
                self->takedamage(e, attack);
            }
            else
            {
                self->health -= attack->attack_force;
                if(self->health<=0) kill(self);
            }
        }
    }
    if(nocost && hit && smartbomber) // don't use e, because this can be an item-bomb
    {
        self = smartbomber;
        if(check_energy(1, ANI_SPECIAL))
        {
            self->mp -= self->modeldata.animation[ANI_SPECIAL]->energycost[0];
        }
        else self->health -= self->modeldata.animation[ANI_SPECIAL]->energycost[0];
    }
    self = tmpself;

}


////////////////////////////////

void anything_walk()
{
    if(self->x < advancex - 80 || self->x > advancex + (videomodes.hRes+80)){
        kill(self);
        return;
    }
    //self->x += self->xdir;
}

entity * knife_spawn(char *name, int index, float x, float z, float a, int direction, int type, int map)
{
    entity *e = NULL;

	if(self->weapent && self->weapent->modeldata.project>=0) {e = spawn(x, z, a, direction, NULL, self->weapent->modeldata.project, NULL); if(!e) return NULL; e->ptype = 0; e->a = a;}
	else if(self->animation->custknife>=0)                   {e = spawn(x, z, a, direction, NULL, self->animation->custknife, NULL);       if(!e) return NULL; e->ptype = 0; e->a = a;}
	else if(self->animation->custpshotno>=0)                 {e = spawn(x, z, 0, direction, NULL, self->animation->custpshotno, NULL);     if(!e) return NULL; e->ptype = 1; e->a = 0;}
	else if(self->modeldata.knife>=0)                        {e = spawn(x, z, a, direction, NULL, self->modeldata.knife, NULL);            if(!e) return NULL; e->ptype = 0; e->a = a;}
	else if(self->modeldata.pshotno>=0)                      {e = spawn(x, z, 0, direction, NULL, self->modeldata.pshotno, NULL);          if(!e) return NULL; e->ptype = 1; e->a = 0;}
	else if(index>=0 ||name)                                 {e = spawn(x, z, a, direction, name, index, NULL);                            if(!e) return NULL; e->ptype = 0; e->a = a;}
	else if(type)                                            {e = spawn(x, z, a, direction, "Shot", -1, NULL);                             if(!e) return NULL; e->ptype = 0; e->a = a;}
	else/* if(!type) */                                      {e = spawn(x, z, a, direction, "Knife", -1, NULL);                            if(!e) return NULL; e->ptype = 0; e->a = a;}

    if(e==NULL) return NULL;
    else if(self->modeldata.type == TYPE_PLAYER) e->modeldata.type = TYPE_SHOT;
    else e->modeldata.type = self->modeldata.type;

    if(self->animation->energycost[0] > 0 && nocost) self->cantfire = 1;    // Can't fire if still exists on screen

    if(!e->model->speed && !e->modeldata.nomove) e->modeldata.speed = 2;
    else if(e->modeldata.nomove) e->modeldata.speed = 0;

    e->owner = self;                                                     // Added so projectiles don't hit the owner
    e->nograb = 1;                                                       // Prevents trying to grab a projectile
    e->attacking = 1;
    //e->direction = direction;
    e->think = common_think;
    e->nextthink = time+1;
    e->trymove = NULL;
    e->takedamage = arrow_takedamage;
    e->takeaction = NULL;
    e->modeldata.aimove = AIMOVE1_ARROW;
    if(!e->modeldata.offscreenkill) e->modeldata.offscreenkill = 200; //default value
    e->modeldata.aiattack = AIATTACK1_NOATTACK;
    e->remove_on_attack = e->modeldata.remove;
    e->autokill = e->modeldata.nomove;

	ent_set_colourmap(e, map);

	if(e->ptype) e->base = 0;
	else e->base = a;

    if(e->modeldata.hostile < 0)   e->modeldata.hostile = self->modeldata.hostile;
    if(e->modeldata.candamage < 0) e->modeldata.candamage = self->modeldata.candamage;

    e->modeldata.subject_to_wall = e->modeldata.subject_to_platform = e->modeldata.subject_to_hole = e->modeldata.subject_to_gravity = 1;
    e->modeldata.no_adjust_base  = 1;
	return e;
}



void bomb_explode()
{
    if(self->animating) return;
    kill(self);
}


entity * bomb_spawn(char *name, int index, float x, float z, float a, int direction, int map)
{
    entity *e = NULL;

    if(self->weapent && self->weapent->modeldata.subtype == SUBTYPE_PROJECTILE && self->weapent->modeldata.project>=0) e = spawn(x, z, a, direction, NULL, self->weapent->modeldata.project, NULL);
    else if(self->animation->custbomb>=0) e = spawn(x, z, a, direction, NULL, self->animation->custbomb, NULL);
    else if(self->modeldata.bomb>=0) e = spawn(x, z, a, direction, NULL, self->modeldata.bomb, NULL);
	else e = spawn(x, z, a, direction, name, index, NULL);

    if(e==NULL) return NULL;

    e->a = a;

    if(self->animation->energycost[0] > 0 && nocost) self->cantfire = 1;    // Can't fire if still exists on screen

    if(!e->model->speed && !e->modeldata.nomove) e->modeldata.speed = 2;
    else if(e->modeldata.nomove) e->modeldata.speed = 0;

    e->attacking = 1;
    e->owner = self;                                                     // Added so projectiles don't hit the owner
    e->nograb = 1;                                                       // Prevents trying to grab a projectile
    e->toexplode = 1;                                                    // Set to distinguish exploding projectiles and also so stops falling when hitting an opponent
	ent_set_colourmap(e, map);
    //e->direction = direction;
    toss(e, e->modeldata.jumpheight);
    e->think = common_think;
    e->nextthink = time+1;
    e->trymove = NULL;
    e->takeaction = NULL;
    e->modeldata.aimove = AIMOVE1_BOMB;
    e->modeldata.aiattack = AIATTACK1_NOATTACK;                                    // Well, bomb's attack animation is passive, dont use any A.I. code.
    e->takedamage = common_takedamage;
    e->remove_on_attack = 0;
    e->autokill = e->modeldata.nomove;


    // Ok, some old mods use type none, will have troubles.
    // so we give them some default hostile types.
    if(e->modeldata.hostile <0) e->modeldata.hostile = self->modeldata.hostile;
    if(e->modeldata.candamage <0) e->modeldata.candamage = self->modeldata.candamage;
    e->modeldata.no_adjust_base = 0;
    e->modeldata.subject_to_wall = e->modeldata.subject_to_platform = e->modeldata.subject_to_hole = e->modeldata.subject_to_gravity = 1;
	return e;
}


// Spawn 3 stars
int star_spawn(float x, float z, float a, int direction){ // added entity to know which star to load
    entity *e = NULL;
    int i, index = -1;
    char* starname = NULL;
    float fd = (float)((direction ? 2 : -2));

    //merge enemy/player together, use the same rules
    if(self->weapent && self->weapent->modeldata.subtype==SUBTYPE_PROJECTILE && self->weapent->modeldata.project>=0) index = self->weapent->modeldata.project;
    else if(self->animation->custstar>=0) index = self->animation->custstar; //use any star
    else if(self->modeldata.star>=0) index = self->modeldata.star;
    else starname = "Star"; // this is default star

    for(i = 0; i<3; i++){
        e = spawn(x, z, a, direction, starname, index, NULL);
        if(e == NULL) return 0;

        self->attacking = 0;

        e->takedamage = arrow_takedamage;//enemy_takedamage;    // Players can now hit projectiles
        e->owner = self;    // Added so enemy projectiles don't hit the owner
        e->attacking = 1;
        e->nograb = 1;    // Prevents trying to grab a projectile
        e->xdir = fd/2 * (float)i;
        e->think = common_think;
        e->nextthink = time+1;
        e->trymove = NULL;
        e->takeaction = NULL;
        e->modeldata.aimove = AIMOVE1_STAR;
        e->modeldata.aiattack = AIATTACK1_NOATTACK;
        e->remove_on_attack = e->modeldata.remove;
        e->base = a;
        e->a = a;
        //e->direction = direction;

        if(e->modeldata.hostile <0) e->modeldata.hostile = self->modeldata.hostile;
        if(e->modeldata.candamage <0) e->modeldata.candamage = self->modeldata.candamage;

        e->modeldata.subject_to_wall = e->modeldata.subject_to_platform =
        e->modeldata.subject_to_hole = e->modeldata.subject_to_gravity = 1;
        e->modeldata.no_adjust_base = 1;
    }
    return 1;
}



void steam_think()
{
    if(!self->animating)
    {
        kill(self);
        return;
    }

    self->base += 1;
    self->a = self->base;
}



// for the "trap" type   7-1-2005  trap start
void trap_think()
{
    if(self->x < advancex-80 || self->x > advancex+(videomodes.hRes+80)){
        //        kill(self);   // 6-2-2005 removed temporarily
        return;
    }

    self->attacking = 1;
    self->nextthink = time + 1;
}
//    7-1-2005  trap end




void steam_spawn(float x, float z, float a){
    entity *e = NULL;

    e = spawn(x, z, a, 0, "Steam", -1, NULL);

    if(e==NULL) return;

    e->base = a;
    e->modeldata.no_adjust_base = 1;
    e->think = steam_think;
}



void steamer_think()
{
    if(self->x < advancex-80 || self->x > advancex+(videomodes.hRes+80)){
        kill(self);
        return;
    }

    steam_spawn(self->x, self->z, self->a);
    self->nextthink = time + (GAME_SPEED/10) + (rand32()&31);
}



void text_think(){    // New function so text can be displayed
     // wait to suicide
    if(!self->animating) kill(self);
}

////////////////////////////////

//homing arrow find its target
// type : target type
entity * homing_find_target(int type){
    int i, min, max;
    int index = -1;
    //use the walk animation's range
    if(validanim(self,ANI_WALK)){
        min = self->modeldata.animation[ANI_WALK]->range[0];
        max = self->modeldata.animation[ANI_WALK]->range[1];
    } else {
        min = 0;
        max = 999;
    }
    //find the 'nearest' one
    for(i=0; i<ent_max; i++){
        if( ent_list[i]->exists && ent_list[i] != self //cant target self
            && (ent_list[i]->modeldata.type & type)
            && diff(ent_list[i]->x,self->x)+ diff(ent_list[i]->z,self->z) >= min
            && diff(ent_list[i]->x,self->x)+ diff(ent_list[i]->z,self->z) <= max
            && ent_list[i]->animation->vulnerable[ent_list[i]->animpos]  )
        {
            if(index <0 || diff(ent_list[i]->x, self->x)+diff(ent_list[i]->z, self->z) < diff(ent_list[index]->x, self->x)+diff(ent_list[index]->z, self->z))
                index = i;
        }
    }
    if( index >=0) return ent_list[index];
    return NULL;
}


void bike_crash(){
	int i;
    if(self->direction) self->xdir = 2;
    else self->xdir = -2;
    self->nextthink = time + THINK_SPEED / 2;
    for(i=0; i<maxplayers[current_set]; i++) control_rumble(i, 100);
    //if(self->x < advancex-100 || self->x > advancex+(videomodes.hRes+100)) kill(self);
}



int biker_takedamage(entity *other, s_attack* attack)
{
    entity *driver = NULL;
    entity* tempself = NULL;
    if(self->dead) return 0;
    // Fell in a hole
    if(self->a < PIT_DEPTH)
	{
        kill(self);
        return 0;
    }
    if(other!=self)set_opponent(other, self);

    if(attack->no_pain) // don't drop driver until it is dead, because the attack has no pain effect
    {
        checkdamage(other, attack);
        if(self->health>0) return 1; // not dead yet
    }

    set_pain(self,  self->damagetype, 1);
    self->attacking = 1;
    if(!self->modeldata.offscreenkill) self->modeldata.offscreenkill = 100;
    self->think = bike_crash;
    self->nextthink = time + THINK_SPEED;
    // well, this is the real entity, the driver who take the damage
    if((driver = drop_driver(self)))
    {
        driver->a = self->a;
        tempself = self;
        self = driver;
        self->drop = 1;
		self->direction = tempself->direction;
        if(self->takedamage) self->takedamage(self, attack);
        else self->health -= attack->attack_force;
        self = tempself;

    }
    self->health = 0;
    checkdeath();
    return 1;
}



void obstacle_fall()
{
    if(inair(self)) return;

    self->xdir = self->zdir = 0;
    if((!self->animating && validanim(self,ANI_DIE)) || !validanim(self,ANI_DIE)) kill(self);    // Fixed so ANI_DIE can be used
}



void obstacle_fly()    // Now obstacles can fly when hit like on Simpsons/TMNT
{
    //self->x += self->xdir * 4;    // Equivelant of speed 40
    if(self->x > advancex+(videomodes.hRes + 200) || self->x < advancex-200) kill(self);

    self->nextthink = time + 2;
}



int obstacle_takedamage(entity *other, s_attack* attack)
{
    if(self->a <= PIT_DEPTH)
    {
        kill(self);
        return 0;
    }

    self->pain_time = time + (GAME_SPEED / 5);
    set_opponent(other, self);
    if(self->opponent && self->opponent->modeldata.type==TYPE_PLAYER)
    {
        control_rumble(self->opponent->playerindex, 75);
    }
    checkdamage(other, attack);
    self->playerindex = other->playerindex;    // Added so points go to the correct player
    addscore(other->playerindex, attack->attack_force*self->modeldata.multiple);    // Points can now be given for hitting an obstacle

    if(self->health<=0){

        checkdeath();

        if(other->x < self->x) self->xdir = 1;
        else self->xdir = -1;

        self->attacking = 1;    // So obstacles can explode and hurt players/enemies

        if(self->modeldata.subtype == SUBTYPE_FLYDIE){    // Now obstacles can fly like on Simpsons/TMNT
            self->xdir *= 4;
            self->think = obstacle_fly;
            ent_set_anim(self, ANI_FALL, 0);
        }
        else{
            self->think = obstacle_fall;

            if(validanim(self,ANI_DIE)) ent_set_anim(self, ANI_DIE, 0);    //  LTB 1-13-05  Die before toss
            else{
                toss(self, self->modeldata.jumpheight/1.333);
                ent_set_anim(self, ANI_FALL, 0);
            }

            if(!self->modeldata.nodieblink) self->blink = 1;
        }
    }

    self->nextthink = time + 1;
    return 1;
}


entity * smartspawn(s_spawn_entry * props){   // 7-1-2005 Entire section replaced with lord balls code
    entity *e = NULL;
    entity* wp = NULL;
    int playercount;

    if(props == NULL) return NULL;

    // Now you can make it so enemies/obstacles/etc only spawn if there are 2 players
    if(props->spawnplayer_count >= (playercount = count_ents(TYPE_PLAYER))){
        if(props->boss) --level->bosses;
        return NULL;
    }

    if((level->scrolldir&SCROLL_INWARD) || (level->scrolldir&SCROLL_OUTWARD))
         e = spawn(props->x, props->z + advancey, props->a, props->flip, props->name, props->index, props->model);
    else e = spawn(props->x + advancex, props->z, props->a, props->flip, props->name, props->index, props->model);


    if(e == NULL) return NULL;

    //printf("%s, (%f, %f, %f) - (%f, %f, %f)", props->name, props->x, props->z, props->a, e->x, e->z, e->a);

    // Alias?
    if(props->alias[0]) strncpy(e->name, props->alias, MAX_NAME_LEN);
    if(props->item) e->item = props->itemindex;
    if(props->itemalias[0]) strncpy(e->itemalias, props->itemalias, MAX_NAME_LEN);
    if(props->itemmap) e->itemmap = props->itemmap;
    if(props->itemhealth) e->itemhealth = props->itemhealth;
    e->itemplayer_count = props->itemplayer_count;

    if(props->health[playercount-1] != 0){
        e->health = e->modeldata.health = props->health[playercount-1];
    }

    if(props->mp != 0){
        e->mp = e->modeldata.mp = props->mp;
    }

    if(props->score != 0) e->modeldata.score = props->score;    // Overwrite score if exists in the level's. txt file
    if(props->multiple != 0)e->modeldata.multiple = props->multiple;    // Overwrite multiple if exists in the level's .txt file

    if(!e->map && props->colourmap)
    {
        ent_set_colourmap(e, props->colourmap);
    }

    if(props->aggression) e->modeldata.aggression = props->aggression;  // Aggression can be changed with spawn points now
    if(props->itemtrans) e->itemtrans = props->itemtrans;
    if(props->alpha) e->modeldata.alpha = props->alpha;

    // Feb 26, 2005 - Store the original map to be able to restore with dying flash
    if(props->dying){
        e->dying = props->dying;    // Feb 26, 2005 - Used to define which colourmap is used for the dying flash
        e->per1 = props->per1;    // Mar 21, 2005 - Used to store custom percentages
        e->per2 = props->per2;    // Mar 21, 2005 - Used to store custom percentages
    }

    if(props->nolife) e->modeldata.nolife = props->nolife;    // Overwrite whether live is visible or not
    e->boss = props->boss;

    if(props->boss && level && level->bossmusic[0])
	{
		music(level->bossmusic, 1, level->bossmusic_offset);
	}

    // give the entity a weapon item
    if(props->weapon)
    {
        wp = spawn(e->x, 100000, 0, 0, props->weapon, props->weaponindex, props->weaponmodel);
        //ent_default_init(wp);
        set_weapon(e, wp->modeldata.weapnum, 0);
        e->weapent = wp;
    }

    //ent_default_init(e);
    execute_onspawn_script(e);
    execute_spawn_script(props, e);
    return e;
}   // 7-1-2005 replaced section ends here



void spawnplayer(int index)
{
    s_spawn_entry p;
    //s_model * model = NULL;
    int wall;
    int xc, zc, find = 0;
    index &= 3;

//    model = find_model(player[index].name);
//    if(model == NULL) return;

    memset(&p, 0, sizeof(s_spawn_entry));
    p.name= player[index].name;
    p.index = -1;
    p.itemindex = -1;
    p.weaponindex = -1;
    p.colourmap = player[index].colourmap;
    p.spawnplayer_count = -1;

    if(level->scrolldir&SCROLL_LEFT){
        if(level->spawn[index][0]) p.x = (float)(videomodes.hRes - level->spawn[index][0]);
        else p.x = (float)((videomodes.hRes-20) - 30*index);
    }
    else{
        if(level->spawn[index][0]) p.x = (float)(level->spawn[index][0]);
        else p.x = (float)(20 + 30*index);
        p.flip = 1;
    }
    if(level->spawn[index][1]){
        if(level->scrolldir&(SCROLL_INWARD|SCROLL_OUTWARD))
            p.z = (float)(level->spawn[index][1]);
        else p.z = (float)(PLAYER_MIN_Z + level->spawn[index][1]);
    }
    else if(PLAYER_MAX_Z - PLAYER_MIN_Z > 5) p.z = (float)(PLAYER_MIN_Z + 5);
    else p.z = (float)PLAYER_MIN_Z;
    //////////////////checking holes/ walls///////////////////////////////////
    for(xc = 0; xc < videomodes.hRes / 4; xc++){
        if(p.x > videomodes.hRes) p.x -= videomodes.hRes;
        if(p.x < 0) p.x += videomodes.hRes;
        if(PLAYER_MIN_Z==PLAYER_MAX_Z)
        {
            wall = checkwall(advancex + p.x, p.z);
            if(wall >= 0 && level->walls[wall][7] < MAX_WALL_HEIGHT) break; //found
            if(checkhole(advancex + p.x, p.z) || (wall>=0 && level->walls[wall][7] >= MAX_WALL_HEIGHT)) find = 0;
            else break; // found
        }
        else for(zc=0;zc < (PLAYER_MAX_Z - PLAYER_MIN_Z) / 3; zc++, p.z += 3){
            if(p.z > PLAYER_MAX_Z) p.z -= PLAYER_MAX_Z - PLAYER_MIN_Z;
            if(p.z < PLAYER_MIN_Z) p.z += PLAYER_MAX_Z - PLAYER_MIN_Z;
            wall = checkwall(advancex + p.x, p.z);
            if(wall >= 0 && level->walls[wall][7] < MAX_WALL_HEIGHT) {find = 1; break;}
            else if(wall >=0 && level->walls[wall][7] >= MAX_WALL_HEIGHT) continue;
            if(checkhole(advancex + p.x, p.z)) continue;
            find = 1;
            break;
        }
        if(find) break;
        p.x += (level->scrolldir&SCROLL_LEFT)?-4:4;
    }
    ///////////////////////////////////////////////////////////////////////
    currentspawnplayer = index;
    player[index].ent = smartspawn(&p);

    if(player[index].ent==NULL) shutdown(1, "Fatal: unable to spawn player from '%s'", &p.name);

    player[index].ent->playerindex = index;
	if(nomaxrushreset[4] >= 1) player[index].ent->rush[1] = nomaxrushreset[index];
	else player[index].ent->rush[1] = 0;

    if(player[index].spawnhealth) player[index].ent->health = player[index].spawnhealth + 5;
    if(player[index].ent->health > player[index].ent->modeldata.health) player[index].ent->health = player[index].ent->modeldata.health;

    //mp little recorver after a level by tails
    if(player[index].spawnmp) player[index].ent->mp = player[index].spawnmp + 2;
    if(player[index].ent->mp > player[index].ent->modeldata.mp) player[index].ent->mp = player[index].ent->modeldata.mp;

    if(player[index].weapnum) set_weapon(player[index].ent, player[index].weapnum, 0);
    else set_weapon(player[index].ent, level->setweap, 0);
}





void time_over()
{
    int i;
    s_attack attack;

    attack = emptyattack;
    attack.attack_type = max_attack_types;
    attack.dropv[0] = (float)3; attack.dropv[1] = (float)1.2; attack.dropv[2] = (float)0;
    if(level->type == 1) level_completed = 1;        //    Feb 25, 2005 - Used for bonus levels so a life isn't taken away if time expires.level->type == 1 means bonus level, else regular
    else if(!level_completed)
    {
        endgame = 1;
        for(i=0; i<4; i++)
        {
            if(player[i].ent)
            {
                endgame = 0;
                self = player[i].ent;
                attack.attack_force = self->health;
                self->takedamage(self, &attack);
            }
        }

        if(SAMPLE_TIMEOVER >= 0) sound_play_sample(SAMPLE_TIMEOVER, 0, savedata.effectvol,savedata.effectvol, 100);

        timeleft = level->settime * COUNTER_SPEED;    // Feb 24, 2005 - This line moved here to set custom time
        if(!endgame)showtimeover = 1;
    }
}


// ----------------------- Update functions ------------------------------

void update_scroller(){
    int to=0, i;
    int numplay=0; //4player
    float tx = advancex, ty = advancey;
    static int scrolladd = 0;

    scrolldx = scrolldy = 0;

    if(time < advancetime || freezeall) return;    // Added freezeall so backgrounds/scrolling don't update if animations are frozen

    //advancetime = time + (GAME_SPEED/100);    // Changed so scrolling speeds up for faster players
    advancetime = time -
        ((player[0].ent && (player[0].ent->modeldata.speed >= 12 || player[0].ent->modeldata.runspeed >= 12)) ||
         (player[1].ent && (player[1].ent->modeldata.speed >= 12 || player[1].ent->modeldata.runspeed >= 12)) ||
         (player[2].ent && (player[2].ent->modeldata.speed >= 12 || player[2].ent->modeldata.runspeed >= 12)) ||
         (player[3].ent && (player[3].ent->modeldata.speed >= 12 || player[3].ent->modeldata.runspeed >= 12)) );    // Changed so if your player is faster the backgrounds scroll faster

    if(level_completed) return;

    if(current_spawn>=level->numspawns && !findent(TYPE_ENEMY) &&
    ((player[0].ent && !player[0].ent->dead) || (player[1].ent && !player[1].ent->dead) || (player[2].ent && !player[2].ent->dead) || (player[3].ent && !player[3].ent->dead))
    ){
        if(!findent(TYPE_ENDLEVEL) && ((!findent(TYPE_ITEM) && !findent(TYPE_OBSTACLE) && level->type) || level->type != 1)){    // Feb 25, 2005 - Added so obstacles
            level_completed = 1;                                                // can be used for bonus levels
        }
    }
    else if(count_ents(TYPE_ENEMY) < groupmin){
        while(count_ents(TYPE_ENEMY) < groupmax &&
            current_spawn<level->numspawns &&
            levelpos >= level->spawnpoints[current_spawn].at
            ){
                if(level->spawnpoints[current_spawn].musicfade){
                    musicfade[0] = (float)level->spawnpoints[current_spawn].musicfade;
                    musicfade[1] = (float)savedata.musicvol;
                }
                if(level->spawnpoints[current_spawn].music[0]){
                    strncpy(musicname, level->spawnpoints[current_spawn].music, 128);
                    musicoffset = level->spawnpoints[current_spawn].musicoffset;
					musicloop = 1;
                }
                if(level->spawnpoints[current_spawn].wait){
                    level_waiting = 1;
                    go_time = 0;
                }
                else if(level->spawnpoints[current_spawn].groupmin || level->spawnpoints[current_spawn].groupmax){
                    groupmin = level->spawnpoints[current_spawn].groupmin;
                    groupmax = level->spawnpoints[current_spawn].groupmax;
                }
                else if(level->spawnpoints[current_spawn].nojoin!=0){
                    nojoin = (level->spawnpoints[current_spawn].nojoin==1);
                }
                else if(level->spawnpoints[current_spawn].scrollminz ||
                    level->spawnpoints[current_spawn].scrollmaxz){
                        scrollminz = (float)level->spawnpoints[current_spawn].scrollminz;
                        scrollmaxz = (float)level->spawnpoints[current_spawn].scrollmaxz;
                        if(!time) advancey = scrollminz; // reset y if spawn at very beginning
                    }
                else if(level->spawnpoints[current_spawn].blockade){
                    // assume level spawn entry will not roll back, so just change it to 0 here
                    if(level->spawnpoints[current_spawn].blockade<0) level->spawnpoints[current_spawn].blockade = 0;
                    blockade = (float)level->spawnpoints[current_spawn].blockade;
                }
                else if(level->spawnpoints[current_spawn].palette!=0){
                    // assume level spawn entry will not roll back, so just change it to 0 here
                    if(level->spawnpoints[current_spawn].palette<0) level->spawnpoints[current_spawn].palette = 0;
                    change_system_palette(level->spawnpoints[current_spawn].palette);
                }
                else if(level->spawnpoints[current_spawn].light[1]){ // change light direction for gfxshadow
                    light[0] = level->spawnpoints[current_spawn].light[0];
                    light[1] = level->spawnpoints[current_spawn].light[1];
                }
                else if(level->spawnpoints[current_spawn].shadowcolor){ // change color for gfxshadow
                    shadowcolor = level->spawnpoints[current_spawn].shadowcolor;
                    if(shadowcolor==-1) shadowcolor = 0;
                    else if(shadowcolor==-2) shadowcolor = -1;
                }
                else if(level->spawnpoints[current_spawn].shadowalpha){ // change color for gfxshadow
                    shadowalpha = level->spawnpoints[current_spawn].shadowalpha;
                    if(shadowalpha==-1) shadowalpha = 0;
                }
                else smartspawn(&level->spawnpoints[current_spawn]);
                ++current_spawn;
            }
    }

    for(i=0; i<maxplayers[current_set]; i++)
    {
        if(player[i].ent) numplay++;
    }

    if(level_waiting){
        // Wait for all enemies to be defeated
        if(!findent(TYPE_ENEMY))
        {
            level_waiting = 0;
            if(level->noreset<=1) timeleft = level->settime * COUNTER_SPEED;    // Feb 24, 2005 - This line moved here to set custom time
            go_time = time + 3*GAME_SPEED;
        }
    }
    if(numplay == 0) return;
    if(!level_waiting)
    {
        if(level->scrolldir&SCROLL_RIGHT){

            for(i=0; i<maxplayers[current_set]; i++)
            {
                if(player[i].ent)
                {
                    to += (int)player[i].ent->x;
                }
            }

            to /= numplay;
            to -= (videomodes.hRes/2);

			to += level->cameraxoffset;

            if((level->scrolldir&SCROLL_BACK) && to < blockade) to = (int)blockade;

            if(to > advancex){
                if(to > advancex+1) to = (int)(advancex+1);
                advancex = (float)to;
            }

            if(level->scrolldir&SCROLL_BACK){    // Can't go back to the beginning

                if(to < advancex && to > blockade){
                    if(to < advancex-1) to = (int)(advancex-1);
                    advancex = (float)to;
                }
            }

            if(advancex > level->width-videomodes.hRes) advancex = (float)level->width-videomodes.hRes;
            if(advancex < 0) advancex = 0;

            if(level->width - levelpos > videomodes.hRes) levelpos = (int)advancex;
            else levelpos++;
        }
        else if(level->scrolldir&SCROLL_LEFT){

            for(i=0; i<maxplayers[current_set]; i++)
            {
                if(player[i].ent)
                {
                    to += (int)player[i].ent->x;
                }
            }

            to /= numplay;
            to -= (videomodes.hRes/2);

            if(to < advancex){
                if(to < advancex-1) to = (int)(advancex-1);
                advancex = (float)to;
            }
            if(level->scrolldir&SCROLL_BACK){    // Can't go back to the beginning

                if(to > advancex){
                    if(to > advancex+1) to = (int)(advancex+1);
                    advancex = (float)to;
                }
            }
            if(advancex > level->width-videomodes.hRes) advancex = (float)level->width-videomodes.hRes;
            if((level->scrolldir&SCROLL_BACK) && level->width- videomodes.hRes - advancex < blockade) advancex = level->width- videomodes.hRes - blockade;
            if(advancex < 0) advancex = 0;

            if(level->width - levelpos > videomodes.hRes) levelpos = (int)((level->width-videomodes.hRes) - advancex);
            else levelpos++;
        }
        else if(level->scrolldir&SCROLL_OUTWARD){ // z scroll only

            for(i=0; i<maxplayers[current_set]; i++)
            {
                if(player[i].ent)
                {
                    to += (int)player[i].ent->z;
                }
            }

            to /= numplay;
            to -= (videomodes.vRes/2);

            if(to > advancey){
                if(to > advancey+1) to = (int)(advancey+1);
                advancey = (float)to;
            }

            if(level->scrolldir&SCROLL_BACK){    // Can't go back to the beginning

                if(to < advancey){
                    if(to < advancey-1) to = (int)(advancey-1);
                    advancey = (float)to;
                }
            }

            if(advancey > panel_height-videomodes.vRes) advancey = (float)panel_height-videomodes.vRes;
            if((level->scrolldir&SCROLL_BACK) && advancey < blockade) advancey = blockade;
            if(advancey < 4) advancey = 4;

            if(panel_height - levelpos > videomodes.vRes) levelpos = (int)advancey;
            else levelpos++;
        }
        else if(level->scrolldir&SCROLL_INWARD){
            for(i=0; i<maxplayers[current_set]; i++)
            {
                if(player[i].ent)
                {
                    to += (int)player[i].ent->z;
                }
            }

            to /= numplay;
            to -= (videomodes.vRes/2);

            if(to < advancey){
                if(to < advancey-1) to = (int)advancey-1;
                advancey = (float)to;
            }
            if(level->scrolldir&SCROLL_BACK){    // Can't go back to the beginning

                if(to > advancey){
                    if(to > advancey+1) to = (int)advancey+1;
                    advancey = (float)to;
                }
            }
            if(advancey > panel_height-videomodes.vRes) advancey = (float)panel_height-videomodes.vRes;
            if((level->scrolldir&SCROLL_BACK) && panel_height- videomodes.vRes - advancey < blockade) advancey = panel_height- videomodes.vRes - blockade;
            if(advancey < 4) advancey = 4;

            if(panel_height - levelpos > videomodes.vRes) levelpos = (int)((panel_height-videomodes.vRes) - advancey);
            else levelpos++;
        }
        //up down, elevator stage
        else if(level->scrolldir&(SCROLL_UP|SCROLL_DOWN))
        {
            //advancey += 0.5;
            if(scrolladd==1)
            {
                scrolladd=0;
                advancey++;
            }
            else
            {
                scrolladd++;
            }
            levelpos = (int)advancey;
        }
    }//if(!level_waiting)

    // z auto-scroll, 2007 - 2 - 10 by UTunnels
    if((level->scrolldir&SCROLL_LEFT) || (level->scrolldir&SCROLL_RIGHT)) // added scroll type both; weird things can happen, but only if the modder is lazy in using blockades, lol
    {
        for(i=0, to=0; i<maxplayers[current_set]; i++)
        {
            if(player[i].ent)
            {
                to += (int)(player[i].ent->z - (cameratype==1?player[i].ent->a:0));
            }
        }

        to /= numplay;
        to -= (videomodes.vRes/2);

		to += level->camerazoffset;

        // new scroll limit
        if(scrollmaxz && to > scrollmaxz) to = (int)scrollmaxz;
        if(scrollminz && to < scrollminz) to = (int)scrollminz;

        if(to > advancey){
            if(to > advancey+1) to = (int)advancey+1;
            advancey = (float)to;
        }

        if(to < advancey){
            if(to < advancey-1) to = (int)advancey-1;
            advancey = (float)to;
        }

        if(advancey > panel_height - 16 -videomodes.vRes) advancey = (float)(panel_height - 16 -videomodes.vRes);
        if(advancey < 4) advancey = 4;
    }
    // now x auto scroll
    else if((level->scrolldir&SCROLL_INWARD) || (level->scrolldir&SCROLL_OUTWARD))
    {
        for(i=0, to=0; i<maxplayers[current_set]; i++)
        {
            if(player[i].ent)
            {
                to += (int)player[i].ent->x;
            }
        }

        to /= numplay;
        to -= (videomodes.hRes/2);

        // new scroll limit
        if(scrollmaxz && to > scrollmaxz) to = (int)scrollmaxz;
        if(scrollminz && to < scrollminz) to = (int)scrollminz;

        if(to > advancex){
            if(to > advancex+1) to = (int)advancex+1;
            advancex = (float)to;
        }

        if(to < advancex){
            if(to < advancex-1) to = (int)advancex-1;
            advancex = (float)to;
        }

        if(advancex > level->width - videomodes.hRes) advancex = (float)(level->width - videomodes.hRes);
        if(advancex < 0) advancex = 0;
    }
    //end of z auto-scroll
    // global value for type_panel
    scrolldx = advancex - tx;
    scrolldy = advancey - ty;
}


void applybglayers(s_screen* pbgscreen)
{
    int index, x, z, i, j, k, l, timevar;
    s_bglayer* bglayer;
    int width, height;
	s_drawmethod screenmethod;

    if(!textbox)
         bgtravelled += (((time-traveltime)*level->bgspeed/30*4) + ((level->rocking)?((time-traveltime)/(GAME_SPEED/30)):0)); // no like in real life, maybe
    else
         texttime += time-traveltime;

    timevar = time - texttime;

    for(index = 0; index < level->numbglayers; index++)
    {
        bglayer = level->bglayers+index;


        if(!bglayer->xrepeat || !bglayer->zrepeat || !bglayer->enabled) continue;

        width = bglayer->width + bglayer->xspacing;
        height = bglayer->height + bglayer->zspacing;

        //if(level->bgdir==0) // count from left
        //{
            x = (int)(bglayer->xoffset + (advancex)*(bglayer->xratio) - advancex - (int)(bgtravelled * (1-bglayer->xratio) * bglayer->bgspeedratio)%width);
        //}
        //else //count from right, complex
        //{
        //    x = videomodes.hRes1 - (level->width-videomodes.hRes1-advancex)*(bglayer->xratio) - bglayer->xoffset - width*bglayer->xrepeat + bglayer->xspacing + (int)(bgtravelled * (1-bglayer->xratio) * bglayer->bgspeedratio)%width;
        //}

        if(level->scrolldir&SCROLL_UP)
        {
            z = (int)(4 + videomodes.vRes + (advancey+4)*bglayer->zratio - bglayer->zoffset - height*bglayer->zrepeat + height + bglayer->zspacing);
        }
        else
        {
            z = (int)(4 + bglayer->zoffset + (advancey-4)* bglayer->zratio - advancey);
        }

        if(x<0)
        {
            i = (-x)/width;
            x %= width;
        }
        else i=0;
        if(z<0)
        {
            j = (-z)/height;
            z %= height;
        }
        else j=0;

        screenmethod=plainmethod;
        screenmethod.table = (pixelformat==PIXEL_x8)?(current_palette>0?(level->palettes[current_palette-1]):NULL):NULL;
        screenmethod.alpha = bglayer->alpha;
        screenmethod.transbg = bglayer->transparency;
        for(; j<bglayer->zrepeat && z<videomodes.vRes; z+=height, j++)
        {
            for(k=i, l=x; k<bglayer->xrepeat && l<videomodes.hRes + bglayer->amplitude*2; l+=width, k++)
            {
                if(bglayer->type==bg_screen)
                {
                    if(bglayer->watermode && bglayer->amplitude)
                         putscreen_water(pbgscreen, bglayer->screen, l, z, bglayer->amplitude, (float)bglayer->wavelength, (int)(timevar*bglayer->wavespeed), bglayer->watermode, &screenmethod);
                    else
                         putscreen(pbgscreen, bglayer->screen, l, z, &screenmethod);
                }
                else if(bglayer->type==bg_sprite)
                    putsprite(l, z, bglayer->sprite, pbgscreen, &screenmethod);

            }
        }
    }

    traveltime = time;
}

void applyfglayers(s_screen* pbgscreen)
{
    int index, x, z, i, j, k, l;
    s_fglayer* fglayer;
    int width, height;
	s_drawmethod screenmethod;

    for(index = 0; index < level->numfglayers; index++)
    {
        fglayer = level->fglayers+index;


        if(!fglayer->xrepeat || !fglayer->zrepeat || !fglayer->enabled) continue;

        width = fglayer->width + fglayer->xspacing;
        height = fglayer->height + fglayer->zspacing;

        //if(level->bgdir==0) // count from left
        //{
            x = (int)(fglayer->xoffset + (advancex)*(fglayer->xratio) - advancex - (int)(bgtravelled * (1-fglayer->xratio) * fglayer->bgspeedratio)%width);
        //}
        //else //count from right, complex
        //{
        //    x = videomodes.hRes1 - (level->width-videomodes.hRes1-advancex)*(fglayer->xratio) - fglayer->xoffset - width*fglayer->xrepeat + fglayer->xspacing + (int)(bgtravelled * (1-fglayer->xratio) * fglayer->bgspeedratio)%width;
        //}

        if(level->scrolldir&SCROLL_UP)
        {
            z = (int)(4 + videomodes.vRes + (advancey+4)*fglayer->zratio - fglayer->zoffset - height*fglayer->zrepeat + height + fglayer->zspacing);
        }
        else
        {
            z = (int)(4 + fglayer->zoffset + (advancey-4)* fglayer->zratio - advancey);
        }

        if(x<0)
        {
            i = (-x)/width;
            x %= width;
        }
        else i=0;
        if(z<0)
        {
            j = (-z)/height;
            z %= height;
        }
        else j=0;


        screenmethod=plainmethod;
        screenmethod.table = (pixelformat==PIXEL_x8)?(current_palette>0?(level->palettes[current_palette-1]):NULL):NULL;
        screenmethod.alpha = fglayer->alpha;
        screenmethod.transbg = fglayer->transparency;
        for(; j<fglayer->zrepeat && z<videomodes.vRes; z+=height, j++)
        {
            for(k=i, l=x; k<fglayer->xrepeat && l<videomodes.hRes + fglayer->amplitude*2; l+=width, k++)
            {
                    spriteq_add_frame(l, z, FRONTPANEL_Z + fglayer->z, fglayer->sprite, &screenmethod, 0);

            }
        }
    }
}




void draw_scrolled_bg(){
    int i = 0;
    int inta ;
    int poop = 0;
    int index = 0;
    int fix_y = 0;
    unsigned char neonp[32];//3*8
    static float oldadvx=0, oldadvy=0;
    static int   oldpal = 0;
    static int neon_count = 0;
    static int rockpos = 0;
    static int rockoffssine[32] = {
        2, 2, 3, 4, 5, 6, 7, 7,
            8, 8, 9, 9, 9, 9, 8, 8,
            7, 7, 6, 5, 4, 3, 2, 2,
            1, 1, 0, 0, 0, 0, 1, 1
    };   // normal rock
    static int rockoffsshake[32] = {
        2, 2, 2, 2, 2, 2, 2, 2,
            2, 2, 0, 4, 2, 0, 4, 2,
            2, 2, 2, 2, 2, 2, 2, 2,
            2, 2, 0, 4, 2, 0, 4, 2,
    };   // slow, constant jarring rock, like on a train
    static int rockoffsrumble[32] = {
        2, 2, 3, 3, 2, 2, 3, 3,
            2, 2, 3, 3, 2, 3, 2, 3,
            2, 2, 3, 3, 2, 2, 3, 3,
            2, 2, 3, 3, 2, 3, 2, 3,
    };   // fast, constant rumbling, like in/on a van or trailer
    s_screen*  pbgscreen;
    s_drawmethod screenmethod=plainmethod, *pscreenmethod=&screenmethod;
    int pb = pixelbytes[(int)screenformat];

    if(pixelformat==PIXEL_x8)
    {
        screenmethod.table = current_palette? level->palettes[current_palette-1]: NULL;
    }

    if(bgbuffer)
    {
        if(((level->rocking || level->bgspeed>0 || texture)&& !pause) ||
           oldadvx!=advancex || oldadvy != advancey || current_palette!=oldpal)
            bgbuffer_updated = 0;
        oldadvx = advancex;
        oldadvy = advancey;
        oldpal = current_palette;
    }
    else bgbuffer_updated = 0;
    //font_printf(2, 100, 1, 0, "%d", bgbuffer_updated);

    if(bgbuffer)  pbgscreen = bgbuffer_updated?vscreen:bgbuffer;
    else          pbgscreen = vscreen;

	if(!bgbuffer_updated && level->numbglayers>0) applybglayers(pbgscreen);
	applyfglayers(pbgscreen);

	// Append bg with texture?
	if(texture){
		inta = (int)(advancex/2);
		if(level->rocking){
			inta += (time/(GAME_SPEED/30));
			apply_texture_plane(pbgscreen, 0,background->height, vscreen->width,BGHEIGHT-background->height, inta*256, 10, texture, pscreenmethod);
		}
		else apply_texture_wave(pbgscreen, 0,background->height, vscreen->width, BGHEIGHT-background->height, inta,0, texture, time, 5, pscreenmethod);
	}

    pscreenmethod->alpha = 0;
    pscreenmethod->transbg = 0;

    if(bgbuffer)
    {
        putscreen(vscreen, bgbuffer, 0, 0, NULL);
    }

    bgbuffer_updated = 1;

    if(level->rocking){
        rockpos = (time/(GAME_SPEED/8)) & 31;
        if(level->rocking == 1)         gfx_y_offset = quake - 4 - rockoffssine[rockpos];
        else if(level->rocking == 2) gfx_y_offset = quake - 4 - rockoffsshake[rockpos];
        else if(level->rocking == 3) gfx_y_offset = quake - 4 - rockoffsrumble[rockpos];
    }
    else if(time){
        if(quake >= 0) gfx_y_offset = quake - 4;
        else           gfx_y_offset = quake + 4;
    }

    if(level->scrolldir!=SCROLL_UP && level->scrolldir!=SCROLL_DOWN) gfx_y_offset -= (int)(advancey - 4);

    // Draw 3 layers: screen, normal and neon
    if(panels_loaded && panel_width){
        if(time>=neon_time && !freezeall){    // Added freezeall so neon lights don't update if animations are frozen
            if(pixelformat==PIXEL_8) // under 8bit mode just cycle the palette from 128 to 135
            {
                for(i=0; i<8; i++)  neontable[128+i] = 128 + ((i+neon_count) & 7);
            }
            else if(pixelformat==PIXEL_x8) // copy palette under 24bit mode
            {
                if(pscreenmethod->table)
                {
                    memcpy(neonp, pscreenmethod->table+128*pb, 8*pb);
                    memcpy(pscreenmethod->table+128*pb, neonp+2*pb, 6*pb);
                    memcpy(pscreenmethod->table+(128+6)*pb, neonp, 2*pb);
                }
                else
                {
                    memcpy(neonp, neontable+128*pb, 8*pb);
                    memcpy(neontable+128*pb, neonp+2*pb, 6*pb);
                    memcpy(neontable+(128+6)*pb, neonp, 2*pb);
                }
            }
            neon_time = time + (GAME_SPEED/3);
            neon_count += 2;
        }

        if(level->scrolldir==SCROLL_UP || level->scrolldir==SCROLL_DOWN) inta = 0;
        else inta = (int)advancex;

        poop = inta / panel_width;
        inta %= panel_width;
        for(i=-inta; i<=videomodes.hRes && poop>=0 && poop<level->numpanels; i+=panel_width){
            index = level->order[poop];
            pscreenmethod->table = (pixelformat==PIXEL_x8 && current_palette)?level->palettes[current_palette-1]:NULL;
            if(panels[index].sprite_normal)
            {
                pscreenmethod->alpha = 0;
                spriteq_add_frame(i,gfx_y_offset, PANEL_Z, panels[index].sprite_normal, pscreenmethod, 0);
            }
            if(panels[index].sprite_neon)
            {
                if(pixelformat!=PIXEL_x8 || current_palette<=0)
                    pscreenmethod->table = neontable;
                spriteq_add_frame(i,gfx_y_offset, NEONPANEL_Z, panels[index].sprite_neon, pscreenmethod, 0);
            }
            if(panels[index].sprite_screen)
            {
                pscreenmethod->alpha = BLEND_SCREEN+1;
                spriteq_add_frame(i,gfx_y_offset, SCREENPANEL_Z, panels[index].sprite_screen, pscreenmethod, 0);
            }
            poop++;
        }
    }

    pscreenmethod->alpha = 0;

    for(i=0; i<level->numholes; i++) spriteq_add_sprite((int)(level->holes[i][0]-advancex),(int)(level->holes[i][1] - level->holes[i][6] + 4 + gfx_y_offset), HOLE_Z, holesprite, pscreenmethod, 0);

    if(frontpanels_loaded){

        if(level->scrolldir==SCROLL_UP || level->scrolldir==SCROLL_DOWN) inta = 0;
        else{
            inta = (int)(advancex * 1.4);
            fix_y = (int)(advancey - 4);
        }
        poop = inta / frontpanels[0]->width;
        inta %= frontpanels[0]->width;
        for(i=-inta; i<=videomodes.hRes; i+=frontpanels[0]->width){
            poop %= frontpanels_loaded;
            spriteq_add_frame(i,gfx_y_offset + fix_y, FRONTPANEL_Z, frontpanels[poop], pscreenmethod, 0);
            poop++;
        }
    }

    if(quake!=0 && time>=quaketime){
        quake /= 2;
        quaketime = time + (GAME_SPEED/25);
    }
}

#ifndef DC
void movie_openfile(int save)
{
    char path[256] = {""};
    char tmpname[256] = {""};
    moviebuffer = tracemalloc("movie_openfile", sizeof(*moviebuffer)*MOVIEBUF_LEN);
    if(!moviebuffer) return ;
    memset(moviebuffer, 0, sizeof(*moviebuffer)*MOVIEBUF_LEN);
    getBasePath(path, "Saves", 0);
    getPakName(tmpname, 3);
    strcat(path, tmpname);
    if(save) moviefile = fopen(path, "wb");
    else moviefile = fopen(path, "rb");
    if(moviefile == NULL) return;
    if(save)
    {
        movielog = 1;
        movieplay = 0;
        movieloglen = 0;
        moviebufptr = 0;
    }
    else
    {
        fseek(moviefile, 0, SEEK_END);
        movielen = ftell(moviefile);
        movielen /= sizeof(*moviebuffer);
        fseek(moviefile, 0, SEEK_SET);
        movielog = 0;
        movieplay = 1;
        movieloglen = 0;
        moviebufptr = MOVIEBUF_LEN;
    }
}

void movie_flushbuf()
{
    int disCcWarns;
    if(!moviefile || !moviebuffer) return;
    disCcWarns = fwrite(moviebuffer, sizeof(*moviebuffer), MOVIEBUF_LEN, moviefile);
    memset(moviebuffer, 0, sizeof(*moviebuffer)*MOVIEBUF_LEN);
    moviebufptr = 0;
}

void movie_closefile()
{
    if(moviefile) fclose(moviefile);
    if(moviebuffer) tracefree(moviebuffer);
    moviebuffer = NULL;
    moviefile = NULL;
    moviebufptr = 0;
    movielog = 0;
    movieplay = 0;
}

void movie_update(s_playercontrols ** pctrls)
{
    int p;
    int disCcWarns;
    if(!moviefile || !moviebuffer) return;
    if(moviebufptr==MOVIEBUF_LEN)
    {
        moviebufptr = 0;
        if(movieloglen<=movielen)
        {
            disCcWarns = fread(moviebuffer, sizeof(*moviebuffer), MOVIEBUF_LEN, moviefile);
            movieloglen += MOVIEBUF_LEN;
        }
        else
        {
            movie_closefile();
            return;
        }
    }
    for(p=0; p<4; p++)
	{
        playercontrolpointers[p]->keyflags = moviebuffer[moviebufptr][p][0];
        playercontrolpointers[p]->newkeyflags = moviebuffer[moviebufptr][p][1];
    }
    seed = moviebuffer[moviebufptr][4][0];
    interval = moviebuffer[moviebufptr][4][1];
    moviebufptr++;
}

void movie_save(s_playercontrols ** pctrls)
{
    int p;
    if(!moviefile || !moviebuffer) return;
    if(moviebufptr==MOVIEBUF_LEN)
    {
        movie_flushbuf();
        movieloglen += MOVIEBUF_LEN;
    }
    for(p=0; p<4; p++)
	{
        moviebuffer[moviebufptr][p][0] = playercontrolpointers[p]->keyflags;
        moviebuffer[moviebufptr][p][1] = playercontrolpointers[p]->newkeyflags;
    }
    moviebuffer[moviebufptr][4][0] = seed;
    moviebuffer[moviebufptr][4][1] = interval;
    moviebufptr++;
}

#endif


void inputrefresh()
{
    int p;

#ifndef DC
    int moviestop = 0;
    if(movieplay)
    {
        control_update(playercontrolpointers, maxplayers[current_set]);
        for(p=0; p<maxplayers[current_set]; p++)
        {
            if(playercontrolpointers[p]->newkeyflags & FLAG_ESC)
            {
                moviestop = 1;
                break;
            }
        }
        if(!moviestop)
        {
            movie_update(playercontrolpointers);
            font_printf(2, 2, 1, 0, "Playing movie, frames: %d/%d", movieloglen + moviebufptr-MOVIEBUF_LEN, movielen);
        }
        else
        {
            movie_closefile();
        }
    }
    else
    {
#endif
         control_update(playercontrolpointers, maxplayers[current_set]);
         interval = timer_getinterval(GAME_SPEED); // so interval can be logged into movie
         if(interval > GAME_SPEED) interval = GAME_SPEED/GAME_SPEED;
         if(interval > GAME_SPEED/4) interval = GAME_SPEED/4;

#ifndef DC
    }

    if(movielog && !pause)
    {
        movie_save(playercontrolpointers);
        font_printf(2, 2, 1, 0, "Recording movie, frames: %d", movieloglen + moviebufptr);
    }
#endif
    bothkeys = 0;
    bothnewkeys = 0;

    for(p=0; p<maxplayers[current_set]; p++)
	{
        player[p].releasekeys = (playercontrolpointers[p]->keyflags|player[p].keys) - playercontrolpointers[p]->keyflags;
        player[p].keys = playercontrolpointers[p]->keyflags;
        player[p].newkeys = playercontrolpointers[p]->newkeyflags;
        player[p].playkeys |= player[p].newkeys;
        player[p].playkeys &= player[p].keys;

        bothkeys |= player[p].keys;
        bothnewkeys |= player[p].newkeys;
#ifndef DC
        if(movielog && (bothnewkeys & FLAG_ESC) && !pause)
        {
            movie_flushbuf();
            movie_closefile();
        }
#endif
    }

}

void execute_keyscripts()
{
    int p;
    for(p=0; p<maxplayers[current_set]; p++)
    {
        if(!pause && (level||selectScreen) && (player[p].newkeys || (keyscriptrate && player[p].keys) || player[p].releasekeys)){
            if(level){
                execute_level_key_script(p);
                execute_entity_key_script(player[p].ent);
            }
            execute_key_script(p);
            execute_key_script_all(p);
        }
    }
}

void execute_updatescripts()
{
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(&update_script))
    {
        Script_Execute(&(update_script));
    }
    pcurrentscript = ptempscript;
    if(level && Script_IsInitialized(&(level->update_script)))
    {
        Script_Execute(&(level->update_script));
    }
    pcurrentscript = ptempscript;
}

void execute_updatedscripts()
{
    Script* ptempscript = pcurrentscript;
    if(Script_IsInitialized(&updated_script))
    {
        Script_Execute(&(updated_script));
    }
    pcurrentscript = ptempscript;
    if(level && Script_IsInitialized(&(level->updated_script)))
    {
        Script_Execute(&(level->updated_script));
    }
    pcurrentscript = ptempscript;
}

void draw_textobjs()
{
    int i;
    s_textobj* textobj;
    for(i = 0;i < LEVEL_MAX_TEXTOBJS;i++)
    {
         textobj = level->textobjs + i;

		if(textobj->t && textobj->t <= time)		//If a time was set and passed, remove the text object.
		{
			level->textobjs[i].t	= 0;
			level->textobjs[i].x	= 0;
			level->textobjs[i].y	= 0;
			level->textobjs[i].font = 0;
			level->textobjs[i].z	= 0;
			if(level->textobjs[i].text)
			{
				tracefree(level->textobjs[i].text);
				level->textobjs[i].text = NULL;
			}
		}
		else
		{
			if(textobj->text)
              font_printf(textobj->x, textobj->y, textobj->font, textobj->z, textobj->text);
		}
    }
}

void update(int ingame, int usevwait)
{
    newtime = 0;
    inputrefresh();

    if(!pause)
    {
        if(ingame == 1) execute_updatescripts();
        if(ingame == 1 || selectScreen) execute_keyscripts();

        if((level_completed && !level->noslow && !tospeedup) || slowmotion[0])
    	{
            if(slowmotion[1] == slowmotion[2]) newtime = time + interval;
        }
        else newtime = time + interval;

        slowmotion[2]++;
        if(slowmotion[2] == (slowmotion[1] + 1))
        {
            slowmotion[2] = 0;
            if(slowmotion[0] > 1) slowmotion[1] = slowmotion[0];
        }
        if(newtime > time + 100) newtime = time + 100;

        while(time < newtime)
    	{
            if(ingame == 1)
    		{
                update_scroller();
                if(!freezeall)
    			{
                    if(level->settime > 0 || (!player[0].ent && !player[1].ent && !player[2].ent && !player[3].ent))
    				{
    					if(timeleft>0) --timeleft;
                        else if((level->settime > 0 && !player[0].joining && !player[1].joining && !player[2].joining && !player[3].joining) ||
                                (((!noshare && credits < 1) || (noshare && player[0].credits < 1 && player[1].credits < 1 && player[2].credits <1 && player[3].credits <1))
                                 && !player[0].joining && !player[1].joining && !player[2].joining && !player[3].joining )
                                )
                        {
                            time_over();
                        }
                    }
                }
            }
            if(ingame || selectScreen) update_ents();
            ++time;
        }

    }

    if(ingame==1 &&
#ifndef DC
		!movieplay &&
#endif
		!pause && !nopause &&
       ((player[0].ent && (player[0].newkeys & FLAG_START)) ||
        (player[1].ent && (player[1].newkeys & FLAG_START)) ||
        (player[2].ent && (player[2].newkeys & FLAG_START)) ||
        (player[3].ent && (player[3].newkeys & FLAG_START)))
    )
    {
        sound_play_sample(SAMPLE_BEEP2, 0, savedata.effectvol,savedata.effectvol, 100);
        sound_pause_music(1);
        spriteq_lock();
        pausemenu();
    }


    // gfx section
    if(ingame == 1)
    {
        draw_scrolled_bg();
        predrawstatus();
        drawstatus();
        execute_updatedscripts();
        draw_textobjs();
    }
    else
    {

        clearscreen(vscreen);
        if(background) putscreen(vscreen, background, 0, 0, NULL);
    }
    if(ingame==1 || selectScreen) display_ents();

    spriteq_draw(vscreen, (ingame==0)); // notice, always draw sprites at the very end of other methods

    if(pause!=2 && !noscreenshot && (bothnewkeys&FLAG_SCREENSHOT)) screenshot(vscreen, getpal, 1);

    // Debug stuff, should not appear on screenshot
    if(debug_time==0xFFFFFFFF) debug_time = time + GAME_SPEED * 5;
    if(time<debug_time && debug_msg[0])
	{
        spriteq_clear();
        font_printf(0,230, 0, 0, debug_msg);
        spriteq_draw(vscreen, (ingame==0));
    }
    else
	{
        debug_msg[0] = 0;
#ifdef DEBUG_MODE
        if(levelpos) debug_printf("Position: %i, width: %i, spawn: %i, offsets: %i/%i", levelpos, level->width, current_spawn, quake, gfx_y_offset);
#endif
    }

    if(usevwait) vga_vwait();
    video_copy_screen(vscreen);

    spriteq_clear();

    check_music();
    sound_update_music();
}




// ----------------------------------------------------------------------
/* Plombo 9/4/2010: New function that can use brightness/gamma correction
 * independent from the global palette on platforms where it's available.
 * Hardware accelerated brightness/gamma correction is available on Wii and
 * OpenGL platforms using TEV and GLSL, respectively. Returns 1 on success, 0 on
 * error. */
int set_color_correction(int gm, int br)
{
#if WII
	vga_set_color_correction(gm, br);
	return 1;
#elif SDL
	if(opengl)
	{
		vga_set_color_correction(gm, br);
		return 1;
	}
	else if(screenformat == PIXEL_8)
	{
		palette_set_corrected(pal, savedata.gamma,savedata.gamma,savedata.gamma, savedata.brightness,savedata.brightness,savedata.brightness);
		return 1;
	}
	else return 0;
#else
	if(screenformat == PIXEL_8)
	{
		palette_set_corrected(pal, savedata.gamma,savedata.gamma,savedata.gamma, savedata.brightness,savedata.brightness,savedata.brightness);
		return 1;
	}
	else return 0;
#endif
}

// copied from palette.c, seems it works well
static void _fade_screen(s_screen* screen, int gr, int gg, int gb, int br, int bg, int bb)
{
	int i, len = screen->width*screen->height;
	int pb = pixelbytes[(int)screenformat];
	unsigned c, r, g, b;


	if(gr<-255) gr = -255;
	else if(gr>255) gr = 255;
	if(gg<-255) gg = -255;
	else if(gg>255) gg = 255;
	if(gb<-255) gb = -255;
	else if(gb>255) gb = 255;

	if(br<-255) br = -255;
	else if(br>255) br = 255;
	if(bg<-255) bg = -255;
	else if(bg>255) bg = 255;
	if(bb<-255) bb = -255;
	else if(bb>255) bb = 255;

    if(pb==2) for(i=0; i<len; i++)
    {
        c = ((unsigned short*)screen->data)[i];
        b = (c>>11)*0xFF/0x1F;
        g = ((c&0x7E0)>>5)*0xFF/0x3F;
        r = (c&0x1F)*0xFF/0x1F;
        ((unsigned short*)screen->data)[i] = colour16(gbcorrect(r, gr, br), gbcorrect(g, gg, bg), gbcorrect(b, gb, bb));
    }
    else for(i=0; i<len; i++){
		screen->data[i*pb] =   gbcorrect(screen->data[i*pb],   gr, br);
		screen->data[i*pb+1] = gbcorrect(screen->data[i*pb+1], gg, bg);
		screen->data[i*pb+2] = gbcorrect(screen->data[i*pb+2], gb, bb);
	}
}

// Simple palette fade / vscreen fade
void fade_out(int type, int speed)
{
    int i, j = 0;
    int b, g = 0;
    u32 interval = 0;
    unsigned char* thepal = NULL;
	int current = speed ? speed : fade;

    if(pixelformat==PIXEL_8)
    {
        if(current_palette && level) thepal = level->palettes[current_palette - 1];
        else thepal = pal;
    }

    for(i=0, j=0; j<64; )
	{
		while(j<=i)
		{
			if(!type || type == 1)
			{
				b = ((savedata.brightness+256) * (64-j) / 64) - 256;
				g = 256 - ((savedata.gamma+256) * (64-j) / 64);
				vga_vwait();
				if(!set_color_correction(g, b))
					_fade_screen(vscreen, g,savedata.gamma,savedata.gamma, b,b,b);
			}
			j++;
			if(!type || type == 1)
			{
				video_copy_screen(vscreen);
			}
		}
		if(!type || type == 2)
		{
			sound_update_music();
	        if(!musicoverlap) sound_volume_music(savedata.musicvol*(64-j)/64, savedata.musicvol*(64-j)/64);
		}
        interval = timer_getinterval(current);
        if(interval > current) interval = current/60;
        if(interval > current/4) interval = current/4;
        i += interval;
    }

    if(!type || type == 2)
	{
		if(!musicoverlap) sound_close_music();
	}

	if(!type || type == 1)
    {
		clearscreen(vscreen);
		video_copy_screen(vscreen);
	    vga_vwait();
		//the black screen, so we return to normal palette
		set_color_correction(savedata.gamma, savedata.brightness);
	}
}



void apply_controls(){
    int p;

    for(p=0; p<4; p++){
        control_setkey(playercontrolpointers[p], FLAG_ESC,        CONTROL_ESC);
        control_setkey(playercontrolpointers[p], FLAG_MOVEUP,     savedata.keys[p][SDID_MOVEUP]);
        control_setkey(playercontrolpointers[p], FLAG_MOVEDOWN,   savedata.keys[p][SDID_MOVEDOWN]);
        control_setkey(playercontrolpointers[p], FLAG_MOVELEFT,   savedata.keys[p][SDID_MOVELEFT]);
        control_setkey(playercontrolpointers[p], FLAG_MOVERIGHT,  savedata.keys[p][SDID_MOVERIGHT]);
        control_setkey(playercontrolpointers[p], FLAG_ATTACK,     savedata.keys[p][SDID_ATTACK]);
		control_setkey(playercontrolpointers[p], FLAG_ATTACK2,    savedata.keys[p][SDID_ATTACK2]);
		control_setkey(playercontrolpointers[p], FLAG_ATTACK3,    savedata.keys[p][SDID_ATTACK3]);
		control_setkey(playercontrolpointers[p], FLAG_ATTACK4,    savedata.keys[p][SDID_ATTACK4]);
        control_setkey(playercontrolpointers[p], FLAG_JUMP,       savedata.keys[p][SDID_JUMP]);
        control_setkey(playercontrolpointers[p], FLAG_SPECIAL,    savedata.keys[p][SDID_SPECIAL]);
        control_setkey(playercontrolpointers[p], FLAG_START,      savedata.keys[p][SDID_START]);
        control_setkey(playercontrolpointers[p], FLAG_SCREENSHOT, savedata.keys[p][SDID_SCREENSHOT]);
    }
}



// ----------------------------------------------------------------------

void display_credits()
{
    u32 finishtime = time + 10 * GAME_SPEED;
    int done = 0;

    if(savedata.logo != 1) return;
    fade_out(0, 0);
    unload_background();

    bothnewkeys = 0;

    while(!done)
	{
        font_printf(videomodes.hShift + 140, videomodes.vShift + 3,   2, 0, "Credits");
        font_printf(videomodes.hShift + 125, videomodes.vShift + 25,  1, 0, "Beats Of Rage");
        font_printf(videomodes.hShift + 133, videomodes.vShift + 35,  0, 0, "Senile Team");

        font_printf(videomodes.hShift + 138, videomodes.vShift + 55,  1, 0, "OpenBOR");
        font_printf(videomodes.hShift + 150, videomodes.vShift + 65,  0, 0, "SX");
        font_printf(videomodes.hShift + 70,  videomodes.vShift + 75,  0, 0, "CGRemakes");
        font_printf(videomodes.hShift + 205, videomodes.vShift + 75,  0, 0, "Fugue");
        font_printf(videomodes.hShift + 70,  videomodes.vShift + 85,  0, 0, "uTunnels");
        font_printf(videomodes.hShift + 205, videomodes.vShift + 85,  0, 0, "Kirby");
        font_printf(videomodes.hShift + 70,  videomodes.vShift + 95,  0, 0, "LordBall");
        font_printf(videomodes.hShift + 205, videomodes.vShift + 95,  0, 0, "Tails");
		font_printf(videomodes.hShift + 70,  videomodes.vShift + 105, 0, 0, "KBAndressen");
		font_printf(videomodes.hShift + 205, videomodes.vShift + 105, 0, 0, "Damon Caskey");
		font_printf(videomodes.hShift + 70,  videomodes.vShift + 115, 0, 0, "Plombo");
		font_printf(videomodes.hShift + 205, videomodes.vShift + 115, 0, 0, "Orochi_X");

        font_printf(videomodes.hShift + 138, videomodes.vShift + 125, 1, 0, "Consoles");
        font_printf(videomodes.hShift + 70,  videomodes.vShift + 135, 0, 0, "PSP, PS3, Linux, OSX");
        font_printf(videomodes.hShift + 205, videomodes.vShift + 135, 0, 0, "SX");
        font_printf(videomodes.hShift + 70,  videomodes.vShift + 145, 0, 0, "Dingoo");
        font_printf(videomodes.hShift + 205, videomodes.vShift + 145, 0, 0, "Shin-NiL");
        font_printf(videomodes.hShift + 70,  videomodes.vShift + 155, 0, 0, "Windows");
        font_printf(videomodes.hShift + 205, videomodes.vShift + 155, 0, 0, "SX & Nazo");
		font_printf(videomodes.hShift + 70,  videomodes.vShift + 165, 0, 0, "GamePark");
        font_printf(videomodes.hShift + 205, videomodes.vShift + 165, 0, 0, "SX & Lemon");
		font_printf(videomodes.hShift + 70,  videomodes.vShift + 175, 0, 0, "DreamCast");
        font_printf(videomodes.hShift + 205, videomodes.vShift + 175, 0, 0, "SX & Neill Corlett");
        font_printf(videomodes.hShift + 70,  videomodes.vShift + 185, 0, 0, "MS XBoX");
        font_printf(videomodes.hShift + 205, videomodes.vShift + 185, 0, 0, "SX & XPort");
		font_printf(videomodes.hShift + 70,  videomodes.vShift + 195, 0, 0, "Wii");
		font_printf(videomodes.hShift + 205, videomodes.vShift + 195, 0, 0, "SX & Plombo");

        font_printf(videomodes.hShift + 133, videomodes.vShift + 215, 1, 0, "Menu Design");
        font_printf(videomodes.hShift + 70,  videomodes.vShift + 225, 0, 0, "SX");
		font_printf(videomodes.hShift + 205, videomodes.vShift + 225, 0, 0, "Fightn Words");

        update(2,0);

        done |= (time > finishtime);
        done |= (bothnewkeys & (FLAG_START+FLAG_ESC));
    }
    fade = 75;
    fade_out(0, 0);
}


void shutdown(int status, char *msg, ...)
{
    char buf[1024] = "";
    va_list arglist;
#if WII
	int i;
#endif

    va_start(arglist, msg);
    vsprintf(buf, msg, arglist);
    va_end(arglist);

	if(!disablelog)
	{
		switch(status)
		{
		case 0:
			printf("\n************ Shutting Down ************\n\n");
			break;
		default:
			printf("\n********** An Error Occurred **********"
				"\n*            Shutting Down            *\n\n");
			break;
		}
	}

	getRamStatus(BYTES);
    savesettings();

	if(status != 2) display_credits();
    term_videomodes();

	if(!disablelog) printf("Release level data");
    unload_levelorder();
	if(!disablelog) printf("...........");
    unload_level();
	if(!disablelog) printf("\tDone!\n");

	if(!disablelog) printf("Release graphics data");
    if(!disablelog) printf("..");
	freescreen(&vscreen);
	if(!disablelog) printf("..");
	freescreen(&background);
    if(!disablelog) printf("..");
#if WII
	for(i=0; i<MAX_CACHED_BACKGROUNDS; i++) freescreen(&bg_cache[i]);
	if(!disablelog) printf("..");
#endif
    freesprites();
	if(!disablelog) printf("..");
    unload_all_fonts();
	if(!disablelog) printf("\tDone!\n");


    if(!disablelog) printf("Release game data............\n\n");
    free_ents();
    free_models();
    free_modelcache();
    clear_scripts();
	if(!disablelog) printf("\nRelease game data............\tDone!\n");

    if(!disablelog) printf("Release timer................");
    borTimerExit();
	if(!disablelog) printf("\tDone!\n");

    if(!disablelog) printf("Release input hardware.......");
    control_exit();
	if(!disablelog) printf("\tDone!\n");

    if(!disablelog) printf("Release sound system.........");
    sound_exit();
	if(!disablelog) printf("\tDone!\n");


#if PSP || DC || GP2X || XBOX || MEMTEST || WII || DINGOO || SYMBIAN
    if(!disablelog) printf("Release FileCaching System...");
    pak_term();
	if(!disablelog) printf("\tDone!\n");
#endif

    if(!disablelog) printf("\n**************** Done *****************\n\n");
    if(!disablelog) printf("%s", buf);

	exit(status);
}


#if XBOX || DC
void guistartup(){
    int i;

    if(!font_load(0, "menu/font1", packfile, 0)) shutdown(1, "Unable to load font #1!\n");
    if(!font_load(1, "menu/font2", packfile, 0)) shutdown(1, "Unable to load font #2!\n");
    if(!font_load(2, "menu/font3", packfile, 0)) shutdown(1, "Unable to load font #3!\n");


    borTimerInit();

    control_init(2);
    apply_controls();

	init_videomodes(0);
	if(!video_set_mode(videomodes)) shutdown(1, "Unable to set video mode: %d x %d!\n", videomodes.hRes, videomodes.vRes);

    for(i=0; i<256; i++) neontable[i] = i;
}
#endif

void startup(){
	int i;


#if PSP || DC || GP2X || XBOX || MEMTEST || WII || DINGOO || SYMBIAN

	size_t size;
	printf("FileCaching System Init......\t");
	if(!(size = pak_init())) shutdown(1, "FileCaching System failed to Initialize!\n");
	printf("%d Bytes\n", size);
#endif

#if PSP
	if(savedata.pspcpuspeed<0) savedata.pspcpuspeed = 2;
	if(savedata.pspcpuspeed>2) savedata.pspcpuspeed = 0;
	switch(savedata.pspcpuspeed){
		case 0:
			scePowerSetClockFrequency(222, 222, 111);
			break;
		case 1:
			scePowerSetClockFrequency(266, 266, 133);
			break;
		case 2:
			scePowerSetClockFrequency(333, 333, 166);
			break;
	}
#endif

	loadHighScoreFile();
	clearSavedGame();

	init_videomodes(1);
	if(!video_set_mode(videomodes)) shutdown(1, "Unable to set video mode: %d x %d!\n", videomodes.hRes, videomodes.vRes);

	printf("Loading menu.txt.............\t");
	load_menu_txt();
	printf("Done!\n");

	printf("Loading fonts................\t");
	load_all_fonts();
	printf("Done!\n");

	printf("Timer init...................\t");
	borTimerInit();
	printf("Done!\n");

	printf("Initialize Sound..............\t");
	if(savedata.usesound && sound_init(12)){
		if(load_special_sounds()) printf("Done!\n");
		else printf("\n");
		if(!sound_start_playback(savedata.soundbits,savedata.soundrate)) printf("Warning: can't play sound at %u Hz!\n", savedata.soundrate);
		SB_setvolume(SB_MASTERVOL, 15);
		SB_setvolume(SB_VOICEVOL, savedata.soundvol);
	}
	else shutdown(1, "Unable to Initialize Sound.\n");

	printf("Loading sprites..............\t");
	load_special_sprites();
	printf("Done!\n");

	printf("Loading level order..........\t");
	load_levelorder();
	printf("Done!\n");

	printf("Loading script settings......\t");
	load_script_setting();
	printf("Done!\n");

	printf("Loading scripts..............\t");
	load_scripts();
	printf("Done!\n");

	printf("Loading models...............\n\n");
	load_models();

	printf("Object engine init...........\t");
	if(!alloc_ents()) shutdown(1, "Not enough memory for game objects!\n");
	printf("Done!\n");

	printf("Input init...................\t");
	control_init(savedata.usejoy);
	apply_controls();
	printf("Done!\n");

#if WII
	printf("Caching backgrounds..........\t");
	cache_all_backgrounds();
	printf("Done!\n");
#endif

	printf("\n\n");

	for(i=0; i<MAX_PAL_SIZE/4; i++) neontable[i] = i;
	if(savedata.logo++ > 10) savedata.logo = 0;
	savesettings();
}







// ----------------------------------------------------------------------------

// Returns 0 on error, -1 on escape
int playgif(char *filename, int x, int y, int noskip){
	unsigned char gifpal[768] = {0};
    int code;
    int delay;
    u32 milliseconds;
    u32 nextframe;
    u32 lasttime;
    u32 temptime, tempnewtime; // temporary patch for ingame gif play
    int done;
    int frame = 0;
    int synctosound = 0;

	s_screen* tempbg = background;
	background = allocscreen(videomodes.hRes, videomodes.vRes, pixelformat);
	if(background==NULL) shutdown(1, "Out of memory! Function Call 'ShowComplete'");
	clearscreen(background);
	standard_palette(1);

	if(!anigif_open(filename, packfile, background->palette?background->palette:gifpal))
	{
		freescreen(&background); background = tempbg;
		return 0;
	}

    temptime = time;
    tempnewtime = newtime;
    time = 0;
    lasttime = 0;
    milliseconds = 0;
    nextframe = 0;
    delay = 100;
    code = ANIGIF_DECODE_RETRY;
    done = 0;
    synctosound = (sound_getinterval() != 0xFFFFFFFF);

    while(!done){
        if(milliseconds >= nextframe){
            if(code != ANIGIF_DECODE_END){
                while((code = anigif_decode(background, &delay, x, y)) == ANIGIF_DECODE_RETRY);
                // if(code == ANIGIF_DECODE_FRAME){
                // Set time for next frame
                nextframe += delay * 10;
                // }
            }
            else done = 1;
        }
        if(code == ANIGIF_DECODE_END) break;

        if(frame==0){
            vga_vwait();
            if(!background->palette)
            {
                palette_set_corrected(gifpal, savedata.gamma,savedata.gamma,savedata.gamma, savedata.brightness,savedata.brightness,savedata.brightness);
            }
            update(0,0);
        }
        else update(0,1);

        ++frame;

        if(synctosound){
            milliseconds += sound_getinterval();
            if(milliseconds==0xFFFFFFFF) synctosound = 0;
        }
        if(!synctosound) milliseconds += (time-lasttime) * 1000 / GAME_SPEED;
        lasttime = time;

        if(!noskip && (bothnewkeys & (FLAG_ESC | FLAG_ANYBUTTON))) done = 1;
    }
    anigif_close();

	time = temptime;
    newtime = tempnewtime;

	freescreen(&background); background = tempbg;
    if(bothnewkeys & (FLAG_ESC | FLAG_ANYBUTTON)) return -1;
    return 1;
}





void playscene(char *filename)
{
    char *buf;
    size_t size;
    int pos;
    char * command = NULL;
    char giffile[256];
    int x=0, y=0, skipone=0, noskip=0;
    int closing = 0;

    // Read file
    if(buffer_pakfile(filename, &buf, &size)!=1) return;

    // Now interpret the contents of buf line by line
    pos = 0;
    while(buf[pos]){
        command = findarg(buf+pos, 0);
        if(command[0]){
            if(!closing && stricmp(command, "music")==0){
                music(findarg(buf+pos, 1), atoi(findarg(buf+pos, 2)), atol(findarg(buf+pos, 3)));
            }
            else if(!closing && stricmp(command, "animation")==0){
                strcpy(giffile, findarg(buf+pos, 1));
                x = atoi(findarg(buf+pos, 2));
                y = atoi(findarg(buf+pos, 3));
                skipone = atoi(findarg(buf+pos, 4));
                noskip = atoi(findarg(buf+pos, 5));
                if(playgif(giffile, x, y, noskip) == -1 && !skipone) closing = 1;
            }
            else if(stricmp(command, "silence")==0){
                sound_close_music();
            }
        }
        // Go to next non-blank line
        while(buf[pos] && buf[pos]!='\n' && buf[pos]!='\r') ++pos;
        while(buf[pos]=='\n' || buf[pos]=='\r') ++pos;
    }
    if(buf != NULL){
        tracefree(buf);
        buf = NULL;
    }
}




// ----------------------------------------------------------------------------




void gameover(){
    int done = 0;
    char tmpBuff[128] = {""};

    music("data/music/gameover", 0, 0);

    time = 0;
    while(!done)
	{
        if(custScenes != NULL)
		{
			strcpy(tmpBuff,custScenes);
            strncat(tmpBuff,"gameover.txt", 12);
            if(testpackfile(tmpBuff, packfile) >=0) playscene(tmpBuff);
            else font_printf(_strmidx(3, "GAME OVER"),110+videomodes.vShift, 3, 0, "GAME OVER");
        }
        else
		{
		    if(testpackfile("data/scenes/gameover.txt", packfile) >=0) playscene("data/scenes/gameover.txt");
            else font_printf(_strmidx(3, "GAME OVER"),110+videomodes.vShift, 3, 0, "GAME OVER");
		}
        done |= (time>GAME_SPEED*8 && !sound_query_music(NULL,NULL));
        done |= (bothnewkeys & (FLAG_ESC|FLAG_ANYBUTTON));
        update(0,0);
    }
}




void hallfame(int addtoscore)
{
    int done = 0;
    int topten[10] = { 0,0,0,0,0,0,0,0,0,0 };
    u32 score;
    char name[MAX_NAME_LEN+1];
    int i, p, y;
    char tmpBuff[128] = {""};
    s_model *model = NULL;
    int col1 = -8;
    int col2 = 6;

    if(hiscorebg)
	{
        // New alternative background path for PSP
        if(custBkgrds != NULL)
		{
            strcpy(tmpBuff,custBkgrds);
            strncat(tmpBuff,"hiscore", 7);
            load_background(tmpBuff, 0);
        }
        else load_cached_background("data/bgs/hiscore", 0);
    }

    if(addtoscore)
	{
        for(p = 0; p < maxplayers[current_set]; p++)
		{
            model = find_model(player[p].name);
            if(player[p].score > savescore.highsc[9])
			{
                savescore.highsc[9] = player[p].score;
                strcpy(savescore.hscoren[9], model->name);
                topten[9] = 1;

                for(i = 8; i >= 0 && player[p].score > savescore.highsc[i]; i--)
				{
                    score = savescore.highsc[i];
                    strcpy(name, savescore.hscoren[i]);
                    savescore.highsc[i] = player[p].score;
                    strcpy(savescore.hscoren[i], model->name);
                    topten[i] = 1;
                    savescore.highsc[i + 1] = score;
                    strcpy(savescore.hscoren[i + 1], name);
                    topten[i + 1] = 0;
                }
            }
        }
    }

    time = 0;

    while(!done)
	{
        y = 60;
        if(!hiscorebg) font_printf(_strmidx(3, "Hall Of Fame"), y-font_heights[3]-10+videomodes.vShift, 3, 0, "Hall Of Fame");

        for(i = 0; i < 10; i++)
		{
            font_printf(_colx(topten[i], col1), y+videomodes.vShift, topten[i], 0, "%2i.  %s", i+1, savescore.hscoren[i]);
			font_printf(_colx(topten[i], col2), y+videomodes.vShift, topten[i], 0, (scoreformat ? "%09lu" : "%u"), savescore.highsc[i]);
            y += font_heights[topten[i]] + 6;
        }

        update(0,0);
        done |= (time>GAME_SPEED*8);
        done |= (bothnewkeys & (FLAG_START+FLAG_ESC));
    }
    unload_background();
}




// Level completed, show bonus stuff
void showcomplete(int num)
{
    int done = 0;
    int i, j, k;
	u32 clearbonus[4] = { 10000, 10000, 10000, 10000 };
    u32 lifebonus[4] = { 10000, 10000, 10000, 10000 };
	u32 rushbonus[4] = { 10000, 10000, 10000, 10000 };
    u32 nexttime = 0;
    u32 finishtime = 0;
    int chan = 0;
    char tmpBuff[128] = {""};

    if(completebg)
	{
        // New alternative background path for PSP
        if(custBkgrds != NULL)
		{
            strcpy(tmpBuff,custBkgrds);
            strncat(tmpBuff,"complete", 8);
            load_background(tmpBuff, 0);
        }
        else load_cached_background("data/bgs/complete", 0);
    }

    music("data/music/complete", 0, 0);

    for(i=0; i<maxplayers[current_set]; i++)
	{
		if(rush[0] >= 1 && showrushbonus == 1)
		{
			rushbonus[i] = nomaxrushreset[i] * scbonuses[2];
		}
		if(scbonuses[3] == 1) clearbonus[i] = num * scbonuses[0];
		else clearbonus[i] = scbonuses[0];
		lifebonus[i] = player[i].lives * scbonuses[1];
	}

    update(0,0);

    time = 0;
    while(!done)
	{
        if(!scomplete[5]) font_printf(videomodes.hShift+scomplete[0],videomodes.vShift+scomplete[1], 3, 0, "Stage %i Complete!", num);
        else
		{
            font_printf(videomodes.hShift+scomplete[0],videomodes.vShift+scomplete[1], 3, 0, "Stage");
            font_printf(videomodes.hShift+scomplete[2],videomodes.vShift+scomplete[3], 3, 0, "%i",num);
            font_printf(videomodes.hShift+scomplete[4],videomodes.vShift+scomplete[5], 3, 0, "Complete");
        }

        font_printf(videomodes.hShift+cbonus[0],videomodes.vShift+cbonus[1], 0, 0, "Clear Bonus");
		for(i=0, j=2, k=3; i < maxplayers[current_set]; i++, j=j+2, k=k+2) if(player[i].lives > 0) font_printf(videomodes.hShift+cbonus[j],videomodes.vShift+cbonus[k], 0, 0, (scoreformat ? "%09lu" : "%lu"), clearbonus[i]);
		font_printf(videomodes.hShift+lbonus[0],videomodes.vShift+lbonus[1], 0, 0, "Life bonus");
		for(i=0, j=2, k=3; i < maxplayers[current_set]; i++, j=j+2, k=k+2) if(player[i].lives > 0) font_printf(videomodes.hShift+lbonus[j],videomodes.vShift+lbonus[k], 0, 0, (scoreformat ? "%09lu" : "%lu"), lifebonus[i]);
		if(rush[0] >= 1 && showrushbonus == 1)
		{
			font_printf(videomodes.hShift+rbonus[0],videomodes.vShift+rbonus[1], 0, 0, "Rush Bonus");
		for(i=0, j=2, k=3; i < maxplayers[current_set]; i++, j=j+2, k=k+2) if(player[i].lives > 0) font_printf(videomodes.hShift+rbonus[j],videomodes.vShift+rbonus[k], 0, 0, (scoreformat ? "%09lu" : "%lu"), rushbonus[i]);
		}
        font_printf(videomodes.hShift+tscore[0],videomodes.vShift+tscore[1], 0, 0, "Total Score");
		for(i=0, j=2, k=3; i < maxplayers[current_set]; i++, j=j+2, k=k+2) if(player[i].lives > 0) font_printf(videomodes.hShift+tscore[j],videomodes.vShift+tscore[k], 0, 0, (scoreformat ? "%09lu" : "%lu"), player[i].score);

        while(time > nexttime)
		{
            if(!finishtime)    finishtime = time + 4 * GAME_SPEED;

            for(i=0; i<maxplayers[current_set]; i++)
			{
                if(player[i].lives > 0)
				{
                    if(clearbonus[i] > 0)
					{
                        addscore(i, 10);
                        clearbonus[i] -= 10;
                        finishtime = 0;
                    }
                    else if(lifebonus[i] > 0)
					{
                        addscore(i, 10);
                        lifebonus[i] -= 10;
                        finishtime = 0;
                    }
					else if(rush[0] >= 1 && showrushbonus == 1 && (rushbonus[i] > 0))
					{
                        addscore(i, 10);
                        rushbonus[i] -= 10;
                        finishtime = 0;
                    }
                }
            }

            if(!finishtime && !(nexttime&15))
			{
                sound_stop_sample(chan);
                chan = sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol/2,savedata.effectvol/2, 100);
            }
            nexttime++;
        }

        if(bothnewkeys & (FLAG_ANYBUTTON|FLAG_ESC)) done = 1;
        if(finishtime && time>finishtime) done = 1;

        update(0,0);
    }

    // Add remainder of score, incase player skips counter
    for(i=0; i<maxplayers[current_set]; i++)
	{
        if(player[i].lives > 0)
		{
			if(rush[0] >= 1 && showrushbonus == 1)
			{
				addscore(i, rushbonus[i]);
			}
            addscore(i, clearbonus[i]);
            addscore(i, lifebonus[i]);
        }
    }
    unload_background();
}

void savelevelinfo()
{
    int i;
    savelevel[current_set].flag = cansave_flag[current_set];
    // don't check flag here save all info, for simple logic
    for(i=0; i<maxplayers[current_set]; i++)
    {
        savelevel[current_set].pLives[i] = player[i].lives;
        savelevel[current_set].pCredits[i] = player[i].credits;
        savelevel[current_set].pScores[i] = player[i].score;
        savelevel[current_set].pSpawnhealth[i] = player[i].spawnhealth;
        savelevel[current_set].pSpawnmp[i] = player[i].spawnmp;
        savelevel[current_set].pWeapnum[i] = player[i].weapnum;
        savelevel[current_set].pColourmap[i] = player[i].colourmap;
        strncpy(savelevel[current_set].pName[i], player[i].name, MAX_NAME_LEN);
    }
    savelevel[current_set].credits = credits;
    savelevel[current_set].level = current_level;
    savelevel[current_set].stage = current_stage;
    savelevel[current_set].which_set = current_set;
    strncpy(savelevel[current_set].dName, set_names[current_set], MAX_NAME_LEN);
}



int playlevel(char *filename)
{
    int i;
    Script* ptempscript = pcurrentscript;

    kill_all();

    savelevelinfo();
    saveGameFile();
    saveHighScoreFile();
    saveScriptFile();

    load_level(filename);
    time = 0;

    // Fixes the start level executing last button bug
    for(i=0; i<maxplayers[current_set]; i++)
    {
        if(player[i].lives > 0)
        {
            player[i].newkeys = player[i].playkeys = 0;
            player[i].weapnum = level->setweap;
            spawnplayer(i);
			player[i].ent->rush[1] = 0;
        }
    }

    //execute a script when level started
    if(Script_IsInitialized(&level_script)) Script_Execute(&level_script);
    if(Script_IsInitialized(&(level->level_script))) Script_Execute(&(level->level_script));

    while(!endgame)
    {
        update(1,0);
        if(level_completed) endgame |= (!findent(TYPE_ENEMY) || level->type || findent(TYPE_ENDLEVEL));    // Ends when all enemies die or a bonus level
    }
    //execute a script when level finished
    if(Script_IsInitialized(&endlevel_script)) Script_Execute(&endlevel_script);
    if(Script_IsInitialized(&(level->endlevel_script))) Script_Execute(&(level->endlevel_script));
    fade_out(0, 0);

    for(i=0; i<maxplayers[current_set]; i++)
    {
        if(player[i].ent)
        {
			nomaxrushreset[i] = player[i].ent->rush[1];
            player[i].spawnhealth = player[i].ent->health;
            player[i].spawnmp = player[i].ent->mp;
        }
    }

    if(!musicoverlap) sound_close_music();

    kill_all();
    unload_level();

    pcurrentscript = ptempscript;

    return (player[0].lives > 0 || player[1].lives > 0 || player[2].lives > 0|| player[3].lives > 0); //4player
}


int selectplayer(int *players, char* filename)
{
    entity *example[4] = {NULL,NULL,NULL,NULL};
    int i,x;
    int cmap[MAX_PLAYERS] = {0,1,2,3};
    int tperror = 0;
    int exit = 0;
    int ready[MAX_PLAYERS] = {0,0,0,0};
    int escape = 0;
    int players_busy = 0;
    int players_ready = 0;
    int immediate[MAX_PLAYERS]= {0,0,0,0};
    char string[128] = {""};
    char* buf, *command;
    size_t size = 0;
    ptrdiff_t pos = 0;

    selectScreen = 1;
	kill_all();
	reset_playable_list(1);

    if(loadGameFile())
	{
        bonus = 0;
        for(i=0; i<MAX_DIFFICULTIES; i++) if(savelevel[i].times_completed > 0) bonus += savelevel[i].times_completed;
    }

   	if(filename && filename[0])
	{
        if(buffer_pakfile(filename, &buf, &size)!=1) shutdown(1, "Failed to load player select file '%s'", filename);
        while(pos < size)
        {
            command = findarg(buf+pos, 0);
            if(command[0])
            {
                if(stricmp(command, "music")==0)
                {
                    music(findarg(buf+pos, 1), atoi(findarg(buf+pos, 2)), atol(findarg(buf+pos, 3)));
                }
                else if(stricmp(command, "allowselect")==0)
                {
                    load_playable_list(buf+pos);
                }
                else if(stricmp(command, "background")==0)
                {
                    load_background(findarg(buf+pos, 1), 1);
                }
                else if(stricmp(command, "load")==0){
                    strncpy(string, findarg(buf+pos, 1), 128);
                    load_cached_model(string, filename, atoi(findarg(buf+pos, 2)));
                }
                else shutdown(1, "Command '%s' is not understood in file '%s'", command, filename);
            }
            while(buf[pos] && buf[pos]!='\n' && buf[pos]!='\r') ++pos;
            while(buf[pos]=='\n' || buf[pos]=='\r') ++pos;
        }
        if(buf != NULL)
        {
            tracefree(buf);
            buf = NULL;
        }
        for(i=0; i<maxplayers[current_set]; i++)
        {
            if(players[i])
            {
                if(!psmenu[i][0] && !psmenu[i][1])
				{
                    if(maxplayers[current_set] > 2) example[i] = spawn((float)((111-(maxplayers[current_set]*14))+((i*(320-(166/maxplayers[current_set]))/maxplayers[current_set])+videomodes.hShift)),(float)(230+videomodes.vShift),0 ,spdirection[i] , NULL, -1, nextplayermodel(NULL));
                    else example[i] = spawn((float)(83+(videomodes.hShift/2)+(i*(155+videomodes.hShift))),(float)(230+videomodes.vShift),0 ,spdirection[i] , NULL, -1, nextplayermodel(NULL));
                }
                else example[i] = spawn((float)psmenu[i][0], (float)psmenu[i][1], 0, spdirection[i], NULL, -1, nextplayermodel(NULL));
            }
        }
    }
    else // without select.txt
    {
        if(skipselect&&(*skipselect)[current_set][0])
        {
            for(i=0; i<MAX_PLAYERS; i++)
            {
    			memset(&player[i], 0, sizeof(s_player));
                if(!players[i]) continue;

                if((*skipselect)[current_set][i]) // just in case or it will be an array overflow issue
                    strncpy(player[i].name, (*skipselect)[current_set][i], MAX_NAME_LEN);
                //else continue;
    			if(!noshare) credits = CONTINUES;
    			else
    			{
    				player[i].credits = CONTINUES;
    				player[i].hasplayed = 1;
    			}
        		if(!creditscheat)
    			{
        			if(noshare) --player[i].credits;
        			else --credits;
    			}
    			player[i].lives = PLAYER_LIVES;
    		}
            selectScreen = 0;
            return 1;
    	}

        if(unlockbg && bonus)
    	{
            // New alternative background path for PSP
            if(custBkgrds != NULL)
    		{
                strcpy(string, custBkgrds);
                strncat(string, "unlockbg", 8);
                load_background(string, 1);
            }
            else load_cached_background("data/bgs/unlockbg", 1);
        }
        else
    	{
            // New alternative background path for PSP
    		if(custBkgrds != NULL)
    		{
                strncpy(string, custBkgrds, 128);
                strncat(string, "select", 6);
                load_background(string, 1);
            }
            else load_cached_background("data/bgs/select", 1);
        }
        music("data/music/menu", 1, 0);
        if(!noshare) credits = CONTINUES;
        for(i=0; i<MAX_PLAYERS; i++)
        {
        	memset(&player[i], 0, sizeof(s_player));
        	immediate[i] = players[i];
        }
    }


    while(!(exit || escape))
	{
        players_busy = 0;
        players_ready = 0;
        for(i=0; i<maxplayers[current_set]; i++)
		{
            // you can't have that character!
            if((tperror == i+1) && !ready[i]) font_printf(75+videomodes.hShift,123+videomodes.vShift,0, 0,"Player %d Choose a Different Character!", i+1);
            if(!ready[i])
            {
                if(player[i].lives <= 0 && (noshare || credits>0) && ((player[i].newkeys & FLAG_ANYBUTTON) || immediate[i]))
				{
                    if(noshare)
                    {
                        player[i].credits = CONTINUES;
                        player[i].hasplayed = 1;
                    }

                    if(!creditscheat)
					{
                        if(noshare) --player[i].credits;
                        else --credits;
                    }

                    player[i].lives = PLAYER_LIVES;

                    if(!psmenu[i][0] && !psmenu[i][1])
					{
                        if(maxplayers[current_set] > 2) example[i] = spawn((float)((111-(maxplayers[current_set]*14))+((i*(320-(166/maxplayers[current_set]))/maxplayers[current_set])+videomodes.hShift)),(float)(230+videomodes.vShift),0 ,spdirection[i] , NULL, -1, nextplayermodel(NULL));
                        else example[i] = spawn((float)(83+(videomodes.hShift/2)+(i*(155+videomodes.hShift))),(float)(230+videomodes.vShift),0 ,spdirection[i] , NULL, -1, nextplayermodel(NULL));
                    }
                    else example[i] = spawn((float)psmenu[i][0], (float)psmenu[i][1], 0, spdirection[i], NULL, -1, nextplayermodel(NULL));

                    if(example[i]==NULL) shutdown(1, "Failed to create player selection object!");

					// Select Player Direction for select player screen
                    // example[i]->direction = spdirection[i]; // moved to spawn method

					// Make player 2 different colour automatically
					player[i].colourmap = i;

                    while((example[i]->modeldata.hmap1) && (example[i]->modeldata.hmap2) &&
                        cmap[i] >= example[i]->modeldata.hmap1 &&
                        cmap[i] <= example[i]->modeldata.hmap2
                        )
                    {
                        cmap[i]++;
                        if(cmap[i] > example[i]->modeldata.maps_loaded) cmap[i] = 0;
                    }

                    player[i].playkeys = 0;
                    ent_set_colourmap(example[i], cmap[i]);

                    if(SAMPLE_BEEP >= 0) sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
                }
                else if(player[i].newkeys & FLAG_MOVELEFT && example[i])
				{
                    if(SAMPLE_BEEP >= 0) sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
                    ent_set_model(example[i], prevplayermodel(example[i]->model)->name);
                    cmap[i] = i;

    				while((example[i]->modeldata.hmap1) && (example[i]->modeldata.hmap2) &&
                        cmap[i] >= example[i]->modeldata.hmap1 &&
                        cmap[i] <= example[i]->modeldata.hmap2
                        )
                    {
                        cmap[i]++;
                        if(cmap[i] > example[i]->modeldata.maps_loaded) cmap[i] = 0;
                    }

                    ent_set_colourmap(example[i], cmap[i]);
					tperror = 0;
                }
				else if(player[i].newkeys & FLAG_MOVERIGHT && example[i])
				{
                    if(SAMPLE_BEEP >= 0) sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
                    ent_set_model(example[i], nextplayermodel(example[i]->model)->name);
                    cmap[i] = i;

                    while((example[i]->modeldata.hmap1) && (example[i]->modeldata.hmap2) &&
                        cmap[i] >= example[i]->modeldata.hmap1 && cmap[i] <= example[i]->modeldata.hmap2
                        )
                    {
                        cmap[i]++;
                        if(cmap[i] > example[i]->modeldata.maps_loaded) cmap[i] = 0;
                    }

                    ent_set_colourmap(example[i], cmap[i]);
                    tperror = 0;
                }
                // oooh pretty colors! - selectable color scheme for player characters
                else if(player[i].newkeys & FLAG_MOVEUP && colourselect && example[i])
				{
                    do
					{
                        cmap[i]++;
                        if(cmap[i] > example[i]->modeldata.maps_loaded) cmap[i] = 0;
                    }
                    while(
                        (example[i]->modeldata.fmap &&
                        cmap[i] - 1 == example[i]->modeldata.fmap - 1) ||
                        ((example[i]->modeldata.hmap1) && (example[i]->modeldata.hmap2) &&
                        cmap[i] - 1 >= example[i]->modeldata.hmap1 - 1 &&
                        cmap[i] - 1 <= example[i]->modeldata.hmap2 - 1)
                        );

                        ent_set_colourmap(example[i], cmap[i]);
                }
                else if(player[i].newkeys & FLAG_MOVEDOWN && colourselect && example[i])
				{
                    do
					{
                        cmap[i]--;
                        if(cmap[i] < 0) cmap[i] = example[i]->modeldata.maps_loaded;
                    }
                    while(
                        (example[i]->modeldata.fmap &&
                        cmap[i] - 1 == example[i]->modeldata.fmap - 1) ||
                        ((example[i]->modeldata.hmap1) && (example[i]->modeldata.hmap2) &&
                        cmap[i] - 1 >= example[i]->modeldata.hmap1 - 1 &&
                        cmap[i] - 1 <= example[i]->modeldata.hmap2 - 1)
                        );

                        ent_set_colourmap(example[i], cmap[i]);
                }
                else if((player[i].newkeys & FLAG_ANYBUTTON) && example[i])
				{

                    if(SAMPLE_BEEP2 >= 0) sound_play_sample(SAMPLE_BEEP2, 0, savedata.effectvol,savedata.effectvol, 100);
                    strcpy(player[i].name, example[i]->modeldata.name);
                    player[i].colourmap = cmap[i];

                    // reports error if players try to use the same character and sameplay mode is off
                    if(sameplayer)
					{
                        for(x=0; x<maxplayers[current_set]; x++)
						{
                            if((strncmp(player[i].name,player[x].name,strlen(player[i].name)) == 0) && (i != x))
							{
                                tperror = i+1;
                                break;
                            }
                        }
                    }

                    if(!tperror)
					{
                        time=0;
                        // yay you picked me!
                        if(validanim(example[i],ANI_PICK)) ent_set_anim(example[i], ANI_PICK, 0);
                        while(!ready[i] && example[i] != NULL)
						{
                            update(0,0);
                            if((!validanim(example[i],ANI_PICK) || example[i]->modeldata.animation[ANI_PICK]->loop[0]) && time>GAME_SPEED*2) ready[i] = 1;
							else if(!example[i]->animating) ready[i] = 1;
							if(ready[i]) time=0;
                        }
                    }
                }
            }
            else
			{
                if(!psmenu[i][2] && !psmenu[i][3])
				{
                    if(maxplayers[current_set] > 2) font_printf((95-(maxplayers[current_set]*14))+((i*(320-(166/maxplayers[current_set]))/maxplayers[current_set])+videomodes.hShift), 225+videomodes.vShift, 0, 0, "Ready!");
                    else font_printf(67+(videomodes.hShift/2)+(i*(155+videomodes.hShift)), 225+videomodes.vShift, 0, 0, "Ready!");
                }
                else font_printf(psmenu[i][2], psmenu[i][3], 0, 0, "Ready!");
            }

            if(example[i] != NULL) players_busy++;
            if(ready[i]) players_ready++;
        }

        if(players_busy && players_busy==players_ready && time>GAME_SPEED) exit = 1;

        update(0,0);

        if(bothnewkeys & FLAG_ESC) escape = 1;
    }

	// No longer at the select screen
    selectScreen = 0;
    kill_all();
    sound_close_music();

    return (!escape);
}

void playgame(int *players,  unsigned which_set, int useSavedGame)
{
    int i;
    current_level = 0;
    current_stage = 1;
    current_set = which_set;

    if(which_set>=num_difficulties) return;
    // shutdown(1, "Illegal set chosen: index %i (there are only %i sets)!", which_set, num_difficulties);

    allow_secret_chars = ifcomplete[which_set];
    PLAYER_LIVES = difflives[which_set];
    musicoverlap=diffoverlap[which_set];
    fade=custfade[which_set];
    CONTINUES = diffcreds[which_set];
    magic_type = typemp[which_set];
    if(PLAYER_LIVES == 0) PLAYER_LIVES = 3;
    if(CONTINUES == 0) CONTINUES = 5;
    if(fade == 0) fade = 24;
    sameplayer = same[which_set];

    memset(player, 0, sizeof(s_player)*4);

    if(useSavedGame)
    {
        loadScriptFile();
        current_level = savelevel[current_set].level;
        current_stage = savelevel[current_set].stage;
        if(savelevel[current_set].flag == 2) // don't check 1 or 0 becuase if we use saved game the flag must be >0
        {
            for(i=0; i<maxplayers[current_set]; i++)
            {
                player[i].lives = savelevel[current_set].pLives[i];
                player[i].score = savelevel[current_set].pScores[i];
                player[i].colourmap = savelevel[current_set].pColourmap[i];
                player[i].weapnum = savelevel[current_set].pWeapnum[i];
                player[i].spawnhealth = savelevel[current_set].pSpawnhealth[i];
                player[i].spawnmp = savelevel[current_set].pSpawnmp[i];
                strncpy(player[i].name, savelevel[current_set].pName[i], MAX_NAME_LEN);
            }
            credits = savelevel[current_set].credits;
        }
    }
    else
    {
        max_global_var_index = 0;
    }

    if((useSavedGame && savelevel[current_set].flag == 2) || selectplayer(players, NULL)) // if save flag is 2 don't select player
    {
        while(current_level < num_levels[which_set])
        {
            if(branch_name[0])  // branch checking
            {
                //current_stage = 1; //jump, jump... perhaps we don't need to reset it, modders should take care of it.
                for(i=0; i<num_levels[which_set]; i++)
                {
                    if(stricmp(levelorder[which_set][i]->branchname, branch_name)==0)
                    {
                        current_level = i;
                        branch_name[0] = 0;// clear up so we won't stuck here
                        break;
                    }
                    //if(levelorder[which_set][i]->gonext==1) ++current_stage; OX. Commented this line out. Seems to be cause of inacurate stage # complete message.
                }
            }
            PLAYER_MIN_Z = levelorder[which_set][current_level]->z_coords[0];
            PLAYER_MAX_Z = levelorder[which_set][current_level]->z_coords[1];
            BGHEIGHT = levelorder[which_set][current_level]->z_coords[2];

            if(levelorder[which_set][current_level]->type==cut_scene)
                playscene(levelorder[which_set][current_level]->filename);
            else if(levelorder[which_set][current_level]->type==select_screen)
            {
                for(i=0; i<MAX_PLAYERS ; i++) players[i] = (player[i].lives>0);
                if(selectplayer(players, levelorder[which_set][current_level]->filename)==0)
                {
                    break;
                }
            }
            else if(!playlevel(levelorder[which_set][current_level]->filename))
            {
                if(player[0].lives <= 0 && player[1].lives <= 0 && player[2].lives <= 0 && player[3].lives <= 0){
                    gameover();
                    if(!noshowhof[which_set]) hallfame(1);
                    for(i=0; i<maxplayers[current_set]; i++)
                    {
                        player[i].hasplayed = 0;
                        player[i].weapnum = 0;
                    }
                }
                break;
            }
            if(levelorder[which_set][current_level]->gonext==1)
            {
                showcomplete(current_stage);
                for(i=0; i<maxplayers[current_set]; i++)
                {
                    player[i].spawnhealth = 0;
                    player[i].spawnmp = 0;
                }
                ++current_stage;
                savelevel[current_set].stage = current_stage;
            }
            current_level++;
            savelevel[current_set].level = current_level;
            //2007-2-24, gonext = 2, end game
            if(levelorder[which_set][current_level-1]->gonext == 2)
            {
                current_level = num_levels[which_set];
            }
        }

        if(current_level >= num_levels[which_set])
        {
            bonus += savelevel[current_set].times_completed++;
            saveGameFile();
            saveHighScoreFile();
            fade_out(0, 0);
            hallfame(1);
        }
    }
    // clear global script variant list
    max_global_var_index = -1;
    for(i=0; i<max_indexed_vars; i++) ScriptVariant_Clear(indexed_var_list+i);
    sound_close_music();
}

int choose_difficulty()
{
	int quit = 0;
    int selector = 0;
	int maxdisplay = 5;
	int i, j;
	//float slider = 0;
    int barx, bary, barw, barh;

    barx = videomodes.hRes/5; bary = _liney(0,2)-2; barw = videomodes.hRes*3/5; barh = 5*(font_heights[0]+1)+4;
	bothnewkeys = 0;

    if(loadGameFile())
	{
        bonus = 0;
        for(i=0; i<MAX_DIFFICULTIES; i++) if(savelevel[i].times_completed > 0) bonus += savelevel[i].times_completed;
    }

    while(!quit)
	{
		if(num_difficulties > 1)
		{
			_menutextm(2, 0, 0, "Game Mode");
			for(j=0,i=num_difficulties <= maxdisplay?0:(selector>=maxdisplay?maxdisplay:0); i<num_difficulties; j++,i++)
			{
				if(j < maxdisplay)
				{
					if(bonus >= ifcomplete[i]) _menutextm((selector==i), 2+j, 0, "%s", set_names[i]);
					else
					{
						if(ifcomplete[i]>1) _menutextm((selector==i), 2+j, 0, "%s - Finish Game %i Times To UnLock", set_names[i], ifcomplete[i]);
						else _menutextm((selector==i), 2+j, 0, "%s - Finish Game To UnLock", set_names[i]);
					}
				}
				else break;
			}
			_menutextm((selector==i), 8, 0, "Back");

			//draw the scroll bar
			if(num_difficulties>maxdisplay)
			{
                spriteq_add_box(barx,  bary,        barw,     barh,   0, color_black, 0); //outerbox
				spriteq_add_line(barx, bary,  barx+8, bary, 1, color_white, 0);
				spriteq_add_line(barx, bary, barx, bary + barh, 1, color_white, 0);
				spriteq_add_line(barx + 8, bary, barx+8, bary+barh,  1, color_white, 0);
				spriteq_add_line(barx, bary+barh, barx+8, bary+barh,  1, color_white, 0);
				spriteq_add_box(barx+1,  bary + selector*(barh-3)/num_difficulties, 7,             3,            2, color_white, 0); //slider
			}
		}

        update(0,0);

		if(num_difficulties==1){ // OX. Mods with only one set will auto load that difficulty.
			if(selector==num_difficulties) quit = 1;
            else if(bonus >= ifcomplete[selector]){
                saveslot = selector;
                strncpy(savelevel[saveslot].dName, set_names[saveslot], MAX_NAME_LEN+1);
                return saveslot;
		}
		}

        if(bothnewkeys & FLAG_ESC) quit = 1;
        if(bothnewkeys & FLAG_MOVEUP)
		{
            --selector;
            if(SAMPLE_BEEP >= 0) sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        if(bothnewkeys & FLAG_MOVEDOWN)
		{
            ++selector;
            if(SAMPLE_BEEP >= 0) sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        if(selector<0) selector = num_difficulties;
        if(selector>num_difficulties) selector = 0;
		//if(selector<num_difficulties) slider = selector * 4.5;

        if(bothnewkeys & FLAG_ANYBUTTON){

            if(SAMPLE_BEEP2 >= 0) sound_play_sample(SAMPLE_BEEP2, 0, savedata.effectvol,savedata.effectvol, 100);

            if(selector==num_difficulties) quit = 1;
            else if(bonus >= ifcomplete[selector]){
                saveslot = selector;
                strncpy(savelevel[saveslot].dName, set_names[saveslot], MAX_NAME_LEN+1);
                return saveslot;
            }
        }
    }
    bothnewkeys = 0;
    return -1;
}

int load_saved_game()
{
    int quit = 0;
    int selector = 0;
    int savedStatus = 0;
    char name[256] = {""};
    int col1 = -8, col2=6;

    bothnewkeys = 0;

    if((savedStatus = loadGameFile())) getPakName(name,0);
    for(saveslot=0; saveslot<MAX_DIFFICULTIES; saveslot++) if(savelevel[saveslot].flag && savelevel[saveslot].level) break;

    while(!quit)
	{
        if(saveslot>=MAX_DIFFICULTIES) // not found
        {
			_menutextm(2, 0, 0, "Load Game");
            _menutext(0, col1, 2, "Saved File:");
            _menutext(0, col2, 2, "Not Found!");
			_menutextm(1, 4, 0, "Back");

			selector = 2;
        }
        else
        {
			_menutextm(2, -5, 0, "Load Game");
            _menutext(0, col1, -3, "Saved File:");
            if(savedStatus) _menutext(0, col2, -3, "%s", name);
            else _menutext(0, col2, -3, "Not Found!");

            if(savedStatus){
                _menutext((selector==0), col1, -2, "Mode:");
                _menutext((selector==0), col2, -2, "%s", savelevel[saveslot].dName);
                _menutext(0, col1, -1, "Stage:");
                _menutext(0, col2, -1, "%d", savelevel[saveslot].stage);
                _menutext(0, col1, 0, "Level:");
                _menutext(0, col2, 0, "%d", savelevel[saveslot].level);
                _menutext(0, col1, 1, "Credits:");
                _menutext(0, col2, 1, "%d", savelevel[saveslot].credits);
                _menutext(0, col1, 2, "Player 1 Lives:");
                _menutext(0, col2, 2, "%d", savelevel[saveslot].pLives[0]);
                _menutext(0, col1, 3, "Player 2 Lives:");
                _menutext(0, col2, 3, "%d", savelevel[saveslot].pLives[1]);
                _menutext(0, col1, 4, "Player 3 Lives:");
                _menutext(0, col2, 4, "%d", savelevel[saveslot].pLives[2]);
                _menutext(0, col1, 5, "Player 4 Lives:");
                _menutext(0, col2, 5, "%d", savelevel[saveslot].pLives[3]);
                _menutextm((selector==1), 7, 0, "Start Game");
            }
			_menutextm((selector==2), 8, 0, "Back");
        }
        update(0,0);

        if(bothnewkeys & FLAG_ESC) quit = 1;
        if(selector == 0 && (bothnewkeys & FLAG_MOVELEFT)){
            while(1){
                --saveslot;
                if(saveslot<0) saveslot = MAX_DIFFICULTIES - 1;
                if(savelevel[saveslot].flag && savelevel[saveslot].level) break;
            }
            sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        if(selector == 0 && (bothnewkeys & FLAG_MOVERIGHT)){
            while(1){
                ++saveslot;
                if(saveslot>MAX_DIFFICULTIES - 1) saveslot = 0;
                if(savelevel[saveslot].flag && savelevel[saveslot].level) break;
            }
            sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        if(bothnewkeys & FLAG_MOVEUP){
            --selector;
            sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        if(bothnewkeys & FLAG_MOVEDOWN){
            ++selector;
            sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        if(savedStatus){
            if(selector<0) selector = 2;
            if(selector>2) selector = 0;
        }
        else selector = 2;

        if(selector > 0 && (bothnewkeys & FLAG_ANYBUTTON)){
            sound_play_sample(SAMPLE_BEEP2, 0, savedata.effectvol,savedata.effectvol, 100);
            switch(selector){
                case 1:
                    return saveslot;
                    break;
                case 2:
                    quit = 1;
                    break;
            }
        }
    }
    bothnewkeys = 0;
    return -1;
}

int choose_mode(int *players)
{
	int quit = 0;
	int relback = 0;
    int selector = 0;
	int status = 0;

	bothnewkeys = 0;

    while(!quit)
	{
        _menutextm(2, 0, 0, "Choose Mode");
		_menutextm((selector==0), 2, 0, "New Game");
		_menutextm((selector==1), 3, 0, "Load Game");
        _menutextm((selector==2), 5, 0, "Back");

        update(0,0);

        if(bothnewkeys & FLAG_ESC) quit = 1;
        if(bothnewkeys & FLAG_MOVEUP)
		{
            --selector;
            if(SAMPLE_BEEP >= 0) sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        if(bothnewkeys & FLAG_MOVEDOWN)
		{
            ++selector;
            if(SAMPLE_BEEP >= 0) sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        if(selector<0) selector = 2;
        if(selector>2) selector = 0;

        if(bothnewkeys & FLAG_ANYBUTTON)
		{
            if(SAMPLE_BEEP2 >= 0) sound_play_sample(SAMPLE_BEEP2, 0, savedata.effectvol,savedata.effectvol, 100);
			switch(selector)
			{
				case 0:
					status = choose_difficulty();
					if(status != -1)
					{
						playgame(players, status, 0);
						relback = 1;
						quit = 1;
					}
                    break;
                case 1:
					status = load_saved_game();
					if(status != -1)
					{
						playgame(players, status, 1);
						relback = 1;
						quit = 1;
					}
                    break;
				default:
					quit = 1;
					break;
			}
        }
    }
    bothnewkeys = 0;
	return relback;
}

void term_videomodes()
{
    videomodes.hRes = 0;
    videomodes.vRes = 0;
	video_set_mode(videomodes);
    if(custScenes != NULL) tracefree(custScenes); custScenes = NULL;
    if(custBkgrds != NULL) tracefree(custBkgrds); custBkgrds = NULL;
    if(custLevels != NULL) tracefree(custLevels); custLevels = NULL;
    if(custModels != NULL) tracefree(custModels); custModels = NULL;
}

// Load Video Mode from file
void init_videomodes(int log)

{
	char *filename = "data/video.txt";
	int bits = 8, tmp;
	ptrdiff_t pos, len;
	size_t size;
	char *buf = NULL;
	char *command = NULL;
	char *value = NULL;

	if(log) printf("Initializing video............\n");

    // Use an alternative video.txt if there is one.  Some of these are long filenames; create your PAKs with borpak and you'll be fine.
#define tryfile(X) if((tmp=openpackfile(X,packfile))!=-1) { closepackfile(tmp); filename=X; goto readfile; }
#if WIN || LINUX
    tryfile("data/videopc.txt");
#elif WII
    tryfile("data/videowii.txt");
    if(CONF_GetAspectRatio() == CONF_ASPECT_16_9) { tryfile("data/video169.txt") }
    else tryfile("data/video43.txt");
#elif PSP
    tryfile("data/videopsp.txt");
    tryfile("data/video169.txt");
#elif DC
    tryfile("data/videodc.txt");
    tryfile("data/video43.txt");
#elif WIZ
    tryfile("data/videowiz.txt");
    tryfile("data/video169.txt");
#elif GP2X
    tryfile("data/videogp2x.txt");
    tryfile("data/video43.txt");
#elif DINGOO
    tryfile("data/videodingoo.txt");
    tryfile("data/video43.txt");
#elif SYMBIAN
    tryfile("data/videosymbian.txt");
#endif
#undef tryfile

readfile:
    // Read file
	if(buffer_pakfile(filename, &buf, &size)!=1)
	{
		videoMode = 0;
		printf("'%s' not found.\n", filename);
		goto VIDEOMODES;
	}

	printf("Reading video settings from '%s'.\n", filename);

    // Now interpret the contents of buf line by line
	pos = 0;
	while(pos<size){
        command = findarg(buf+pos, 0);
        if(command[0]){
			if(stricmp(command, "video")==0){
				videoMode = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "scenes")==0){
                len = strlen(findarg(buf+pos, 1));
                custScenes = tracemalloc("custScenes", len + 1);
				strcpy(custScenes, findarg(buf+pos, 1));
                custScenes[len] = 0;
			}
			else if(stricmp(command, "backgrounds")==0){
                len = strlen(findarg(buf+pos, 1));
                custBkgrds = tracemalloc("custBkgrds", len + 1);
				strcpy(custBkgrds, findarg(buf+pos, 1));
                custBkgrds[len] = 0;
			}
			else if(stricmp(command, "levels")==0){
                len = strlen(findarg(buf+pos, 1));
                custLevels = tracemalloc("custLevels", len + 1);
				strcpy(custLevels, findarg(buf+pos, 1));
                custLevels[len] = 0;
			}
			else if(stricmp(command, "models")==0){
                len = strlen(findarg(buf+pos, 1));
                custModels = tracemalloc("custModels", len + 1);
				strcpy(custModels, findarg(buf+pos, 1));
                custModels[len] = 0;
			}
            else if(stricmp(command, "colourdepth")==0){
                pixelformat=PIXEL_x8;
                value = findarg(buf+pos, 1);
                if(stricmp(value, "8bit")==0)
                {
                    screenformat=PIXEL_8;
                    pixelformat=PIXEL_8;
                }
                else if(stricmp(value, "16bit")==0) { screenformat=PIXEL_16; bits=16; }
                else if(stricmp(value, "32bit")==0) { screenformat=PIXEL_32; bits=32; }
                else if(value[0]==0) screenformat=PIXEL_32;
                else shutdown(1, "Screen colour depth can only be either 8bit, 16bit or 32bit.");
            }
            else if(stricmp(command, "forcemode")==0) {}
			else printf("Command '%s' not understood in file '%s'!", command, filename);
        }
        // Go to next line
        while(buf[pos] && buf[pos]!='\n' && buf[pos]!='\r') ++pos;
        while(buf[pos]=='\n' || buf[pos]=='\r') ++pos;
    }

	if(buf != NULL){
		tracefree(buf);
		buf = NULL;
	}

#if DINGOO || GP2X
	videoMode = 0;
#endif

#if SYMBIAN
    if(videoMode != 0 && videoMode != 2)
    {
    	videoMode = 0;
    }
#endif

VIDEOMODES:
    switch (videoMode)
    {
		// 320x240 - All Platforms
		case 0:
			videomodes.mode    = savedata.screen[videoMode][0];
			videomodes.filter  = savedata.screen[videoMode][1];
			videomodes.hRes    = 320;
			videomodes.vRes    = 240;
			videomodes.hScale  = 1;
			videomodes.vScale  = 1;
			videomodes.hShift  = 0;
			videomodes.vShift  = 0;
			videomodes.dOffset = 231;
			PLAYER_MIN_Z       = 160;
			PLAYER_MAX_Z       = 232;
			BGHEIGHT           = 160;
	        break;

		// 480x272 - All Platforms
		case 1:
			videomodes.mode    = savedata.screen[videoMode][0];
			videomodes.filter  = savedata.screen[videoMode][1];
	        videomodes.hRes    = 480;
			videomodes.vRes    = 272;
			videomodes.hScale  = (float)1.5;
			videomodes.vScale  = (float)1.13;
			videomodes.hShift  = 80;
			videomodes.vShift  = 20;
			videomodes.dOffset = 263;
			PLAYER_MIN_Z       = 182;
			PLAYER_MAX_Z       = 264;
			BGHEIGHT           = 182;
			break;

		// 640x480 - PC, Dreamcast, Wii
		case 2:
			videomodes.mode    = savedata.screen[videoMode][0];
			videomodes.filter  = savedata.screen[videoMode][1];
	        videomodes.hRes    = 640;
			videomodes.vRes    = 480;
			videomodes.hScale  = 2;
			videomodes.vScale  = 2;
			videomodes.hShift  = 160;
			videomodes.vShift  = 35;
			videomodes.dOffset = 464;
			PLAYER_MIN_Z       = 321;
			PLAYER_MAX_Z       = 465;
			BGHEIGHT           = 321;
			break;

		// 720x480 - PC, Wii
		case 3:
			videomodes.mode    = savedata.screen[videoMode][0];
			videomodes.filter  = savedata.screen[videoMode][1];
	        videomodes.hRes    = 720;
			videomodes.vRes    = 480;
			videomodes.hScale  = 2.25;
			videomodes.vScale  = 2;
			videomodes.hShift  = 200;
			videomodes.vShift  = 35;
			videomodes.dOffset = 464;
			PLAYER_MIN_Z       = 321;
			PLAYER_MAX_Z       = 465;
			BGHEIGHT           = 321;
			break;

		// 800x480 - PC, Wii, Pandora
		case 4:
			videomodes.mode    = savedata.screen[videoMode][0];
			videomodes.filter  = savedata.screen[videoMode][1];
			videomodes.hRes    = 800;
			videomodes.vRes    = 480;
			videomodes.hScale  = 2.5;
			videomodes.vScale  = 2;
			videomodes.hShift  = 240;
			videomodes.vShift  = 35;
			videomodes.dOffset = 464;
			PLAYER_MIN_Z       = 321;
			PLAYER_MAX_Z       = 465;
			BGHEIGHT           = 321;
			break;

		// 800x600 - PC, Dreamcast, Wii
		case 5:
			videomodes.mode    = savedata.screen[videoMode][0];
			videomodes.filter  = savedata.screen[videoMode][1];
			videomodes.hRes    = 800;
			videomodes.vRes    = 600;
			videomodes.hScale  = 2.5;
			videomodes.vScale  = 2.5;
			videomodes.hShift  = 240;
			videomodes.vShift  = 44;
			videomodes.dOffset = 580;
			PLAYER_MIN_Z       = 401;
			PLAYER_MAX_Z       = 582;
			BGHEIGHT           = 401;
			break;

		// 960x540 - PC, Wii
		case 6:
			videomodes.mode    = savedata.screen[videoMode][0];
			videomodes.filter  = savedata.screen[videoMode][1];
	        videomodes.hRes    = 960;
			videomodes.vRes    = 540;
			videomodes.hScale  = 3;
			videomodes.vScale  = 2.25;
			videomodes.hShift  = 320;
			videomodes.vShift  = 40;
			videomodes.dOffset = 522;
			PLAYER_MIN_Z       = 362;
			PLAYER_MAX_Z       = 524;
			BGHEIGHT           = 362;
			break;

		default:
			shutdown(1, "Invalid video mode: %d in 'data/video.txt', supported modes:\n"
				        "0 - 320x240\n"
						"1 - 480x272\n"
						"2 - 640x480\n"
						"3 - 720x480\n"
						"4 - 800x480\n"
						"5 - 800x600\n"
						"6 - 960x540\n\n", videoMode);
			break;
    }

#if SDL || WII
    video_stretch(savedata.stretch);
#endif

    if((vscreen = allocscreen(videomodes.hRes, videomodes.vRes, screenformat)) == NULL) shutdown(1, "Not enough memory!\n");
	videomodes.pixel = pixelbytes[(int)vscreen->pixelformat];
	video_set_mode(videomodes);
	clearscreen(vscreen);

    if(log) printf("Initialized video.............\t%dx%d (Mode: %d, Depth: %d Bit)\n\n",videomodes.hRes, videomodes.vRes, videoMode, bits);
}



// ----------------------------------------------------------------------------


// Set key or button safely (with switching)
void safe_set(int *arr, int index, int newkey, int oldkey){
    int i;
    for(i=0; i<12; i++){
        if(arr[i]==newkey) arr[i] = oldkey;
    }
    arr[index] = newkey;
}


void keyboard_setup(int player){
	int quit = 0,
		selector = 0,
		setting = -1,
		i, k, ok = 0,
		disabledkey[12] = {0,0,0,0,0,0,0,0,0,0,0,0},
		col1 =-8, col2 = 6;
	ptrdiff_t pos, voffset;
	size_t size;
	char *buf, *command, *filename = "data/menu.txt",
	     buttonnames[12][16];

	printf("Loading control settings.......\t");

	strncpy(buttonnames[0], "Move Up", 16);
	strncpy(buttonnames[1], "Move Down", 16);
	strncpy(buttonnames[2], "Move Left", 16);
	strncpy(buttonnames[3], "Move Right", 16);
	strncpy(buttonnames[4], "Attack 1", 16);
	strncpy(buttonnames[5], "Attack 2", 16);
	strncpy(buttonnames[6], "Attack 3", 16);
	strncpy(buttonnames[7], "Attack 4", 16);
	strncpy(buttonnames[8], "Jump", 16);
	strncpy(buttonnames[9], "Special", 16);
	strncpy(buttonnames[10], "Start", 16);
	strncpy(buttonnames[11], "Screenshot", 16);

	savesettings();
	bothnewkeys = 0;

    // Read file
	if(buffer_pakfile(filename, &buf, &size)){
		// Now interpret the contents of buf line by line
		pos = 0;
		while(pos<size){
			command = findarg(buf+pos, 0);
			if(command[0]){
				if(stricmp(command, "disablekey")==0){

					if(stricmp(findarg(buf+pos, 1), "moveup")==0)
						disabledkey[0] = 1;
					else if(stricmp(findarg(buf+pos, 1), "movedown")==0)
						disabledkey[1] = 1;
					else if(stricmp(findarg(buf+pos, 1), "moveleft")==0)
						disabledkey[2] = 1;
					else if(stricmp(findarg(buf+pos, 1), "moveright")==0)
						disabledkey[3] = 1;
					else if(stricmp(findarg(buf+pos, 1), "attack")==0)
						disabledkey[4] = 1;
					else if(stricmp(findarg(buf+pos, 1), "attack2") == 0)
						disabledkey[5] = 1;
					else if(stricmp(findarg(buf+pos, 1), "attack3") == 0)
						disabledkey[6] = 1;
					else if(stricmp(findarg(buf+pos, 1), "attack4") == 0)
						disabledkey[7] = 1;
					else if(stricmp(findarg(buf+pos, 1), "jump") == 0)
						disabledkey[8] = 1;
					else if(stricmp(findarg(buf+pos, 1), "special") == 0)
						disabledkey[9] = 1;
					else if(stricmp(findarg(buf+pos, 1), "start") == 0)
						disabledkey[10] = 1;
					else if(stricmp(findarg(buf+pos, 1), "screenshot") == 0)
						disabledkey[11] = 1;
				}
				else if(stricmp(command, "renamekey")==0){
					if(stricmp(findarg(buf+pos, 1), "moveup") == 0)
						strncpy(buttonnames[0], findarg(buf+pos, 2), 16);
					else if(stricmp(findarg(buf+pos, 1), "movedown") == 0)
						strncpy(buttonnames[1], findarg(buf+pos, 2), 16);
					else if(stricmp(findarg(buf+pos, 1), "moveleft") == 0)
						strncpy(buttonnames[2], findarg(buf+pos, 2), 16);
					else if(stricmp(findarg(buf+pos, 1), "moveright") == 0)
						strncpy(buttonnames[3], findarg(buf+pos, 2), 16);
					else if(stricmp(findarg(buf+pos, 1), "attack") == 0)
						strncpy(buttonnames[4], findarg(buf+pos, 2), 16);
					else if(stricmp(findarg(buf+pos, 1), "attack2") == 0)
						strncpy(buttonnames[5], findarg(buf+pos, 2), 16);
					else if(stricmp(findarg(buf+pos, 1), "attack3") == 0)
						strncpy(buttonnames[6], findarg(buf+pos, 2), 16);
					else if(stricmp(findarg(buf+pos, 1), "attack4") == 0)
						strncpy(buttonnames[7], findarg(buf+pos, 2), 16);
					else if(stricmp(findarg(buf+pos, 1), "jump") == 0)
						strncpy(buttonnames[8], findarg(buf+pos, 2), 16);
					else if(stricmp(findarg(buf+pos, 1), "special") == 0)
						strncpy(buttonnames[9], findarg(buf+pos, 2), 16);
					else if(stricmp(findarg(buf+pos, 1), "start") == 0)
						strncpy(buttonnames[10], findarg(buf+pos, 2), 16);
					else if(stricmp(findarg(buf+pos, 1), "screenshot") == 0)
						strncpy(buttonnames[11], findarg(buf+pos, 2), 16);
				}
				else if(stricmp(command, "fontmonospace")==0){
					 // here to keep from crashing
				}
				else{
					if(buf != NULL){
						tracefree(buf);
						buf = NULL;
					}
					shutdown(1, "Command '%s' not understood in file '%s'!", command, filename);
				}
			}
			// Go to next line
			while(buf[pos] && buf[pos]!='\n' && buf[pos]!='\r') ++pos;
			while(buf[pos]=='\n' || buf[pos]=='\r') ++pos;
		}
		if(buf != NULL){
			tracefree(buf);
			buf = NULL;
		}
	}

    while(disabledkey[selector]) if(++selector>11) break;

    while(!quit){
        voffset = -6;
        _menutextm(2, -8, 0, "Player %i", player+1);
        for(i = 0; i < 12; i++){
              if(!disabledkey[i]){
                    _menutext((selector==i), col1, voffset, "%s", buttonnames[i]);
                    _menutext((selector==i), col2, voffset, "%s", control_getkeyname(savedata.keys[player][i]));
                    voffset++;
              }
        }
        _menutextm((selector==12), 7, 0, "OK");
        _menutextm((selector==13), 8, 0, "Cancel");
        update(0,0);

        if(setting > -1){
            if(bothnewkeys & FLAG_ESC){
                savedata.keys[player][setting] = ok;
                sound_play_sample(SAMPLE_BEEP2, 0, savedata.effectvol,savedata.effectvol, 50);
                setting = -1;
            }
            if(setting > -1){
                k = control_scankey();
                if(k){
                    safe_set(savedata.keys[player], setting, k, ok);
                    sound_play_sample(SAMPLE_BEEP2, 0, savedata.effectvol,savedata.effectvol, 100);
                    setting = -1;
                    // Prevent accidental screenshot
                    bothnewkeys = 0;
                }
            }
        }
        else{
            if(bothnewkeys & FLAG_ESC) quit = 1;
            if(bothnewkeys & FLAG_MOVEUP){
                do{ if(--selector<0) break; }while(disabledkey[selector]);
                sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
            }
            if(bothnewkeys & FLAG_MOVEDOWN){
                do{ if(++selector>11) break; }while(disabledkey[selector]);
                sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
            }
            if(selector<0) selector = 13;
            if(selector>13){
                 selector = 0;
                 while(disabledkey[selector]) if(++selector>11) break;
            }
            if(bothnewkeys & FLAG_ANYBUTTON){
                sound_play_sample(SAMPLE_BEEP2, 0, savedata.effectvol,savedata.effectvol, 100);
                if(selector==12) quit = 2;
                else if(selector==13) quit = 1;
                else{
                    setting = selector;
                    ok = savedata.keys[player][setting];
                    savedata.keys[player][setting] = 0;
#if DOS || SDL || PSP || WII
                    keyboard_getlastkey();
#endif
                }
            }
        }
    }

    if(quit==2){
        apply_controls();
        savesettings();
    }
    else loadsettings();


    update(0,0);
    bothnewkeys = 0;
	printf("Done!\n");
}


#ifndef DC
void movie_options(){
    int quit = 0;
    int selector = 1; // 0

    bothnewkeys = 0;

    while(!quit){
        _menutextm(2, 1, 0, "Movie Mode");
        _menutextm((selector==0), 2, 0, "Save Movie");
        _menutextm((selector==1), 3, 0, "Load Movie");
        _menutextm((selector==2), 5, 0, "Back");
        update(0,0);

        if(bothnewkeys & FLAG_ESC) quit = 1;
        if(bothnewkeys & FLAG_MOVEUP){
            --selector;
            if(SAMPLE_BEEP >= 0) sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        if(bothnewkeys & FLAG_MOVEDOWN){
            ++selector;
            if(SAMPLE_BEEP >= 0) sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        if(selector<0) selector = 2;
        if(selector>2) selector = 0;
        if(bothnewkeys & FLAG_ANYBUTTON){

            if(SAMPLE_BEEP2 >= 0) sound_play_sample(SAMPLE_BEEP2, 0, savedata.effectvol,savedata.effectvol, 100);

            switch(selector){
                case 0:
                    movie_closefile(); //close first
                    movie_openfile(1); // save movie
                    quit = 1;
                    break;
                case 1:
                    selector = 0;
                    movie_closefile(); //close first
                    movie_openfile(0); // play movie
                    quit = 1;
                    break;
                default:
                    quit = (bothnewkeys & FLAG_ANYBUTTON);
            }
        }
    }
    savesettings();
    bothnewkeys = 0;
}
#endif



void input_options(){
    int quit = 0;
    int selector = 1; // 0

    bothnewkeys = 0;

    while(!quit){
        _menutextm(2, -5, 0, "Control Options");

#if PSP
        if(savedata.usejoy) _menutext((selector==0), -4, -2, "Analog Pad Enabled");
        else _menutext((selector==0), -4, -2, "Analog Pad Disabled");
#elif WII
        if(savedata.usejoy) _menutext((selector==0), -4, -2, "Nunchuk Analog Enabled");
        else _menutext((selector==0), -4, -2, "Nunchuk Analog Disabled");
#else
        if(savedata.usejoy){
            _menutext((selector==0),  -4, -2, "GamePads Enabled");
            if(!control_getjoyenabled()){
                _menutext((selector==0), 7, -2, " - Device Not Ready");
            }
        }
        else _menutext((selector==0),  -4, -2, "GamePads Disabled");
#endif

        _menutext((selector==1), -4, -1, "Setup Player 1...");
        _menutext((selector==2), -4, 0, "Setup Player 2...");
        _menutext((selector==3), -4, 1, "Setup Player 3...");
        _menutext((selector==4), -4, 2, "Setup Player 4...");
        _menutextm((selector==5), 7, 0, "Back");
        update(0,0);

        if(bothnewkeys & FLAG_ESC) quit = 1;
        if(bothnewkeys & FLAG_MOVEUP){
            --selector;
            if(SAMPLE_BEEP >= 0) sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        if(bothnewkeys & FLAG_MOVEDOWN){
            ++selector;
            if(SAMPLE_BEEP >= 0) sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        if(selector<0) selector = 5;
        if(selector>5) selector = 0;
        if(bothnewkeys & (FLAG_MOVELEFT|FLAG_MOVERIGHT|FLAG_ANYBUTTON)){

            if(SAMPLE_BEEP2 >= 0) sound_play_sample(SAMPLE_BEEP2, 0, savedata.effectvol,savedata.effectvol, 100);

            switch(selector){
                case 0:
                    control_usejoy((savedata.usejoy ^= 1));
                    break;
                case 1:
                    keyboard_setup(0);
                    break;
                case 2:
                    keyboard_setup(1);
                    break;
                case 3:
                    keyboard_setup(2);
                    break;
                case 4:
                    keyboard_setup(3);
                    break;
                default:
                    quit = (bothnewkeys & FLAG_ANYBUTTON);
            }
        }
    }
    savesettings();
    bothnewkeys = 0;
}



void sound_options(){

    int quit = 0;
    int selector = 0;
    int dir;
    int col1 = -8;
    int col2 = 6;

    bothnewkeys = 0;

    while(!quit){
        _menutextm(2, -5, 0, "Sound Options");
        _menutext((selector==0), col1, -2, "Sound Volume:");
        _menutext((selector==0), col2, -2, "%i", savedata.soundvol);
        _menutext((selector==1), col1, -1, "SFX Volume:");
        _menutext((selector==1), col2, -1, "%i", savedata.effectvol);
        _menutext((selector==2), col1, 0, "Music Volume:");
        _menutext((selector==2), col2, 0, "%i", savedata.musicvol);
		_menutext((selector==3), col1, 1, "BGM:");
        _menutext((selector==3), col2, 1, "%s", (savedata.usemusic ? "Enabled" : "Disabled"));
        _menutext((selector==4), col1, 2, "Show Titles:");
		_menutext((selector==4), col2, 2, "%s", (savedata.showtitles ? "Yes" : "No"));
		_menutext((selector==5), col1, 3, "Advanced Options...");
        _menutextm((selector==6), 7, 0, "Back");

        update(0,0);

        if(bothnewkeys & FLAG_ESC) quit = 1;
        if(bothnewkeys & FLAG_MOVEUP){
            --selector;

            if(SAMPLE_BEEP >= 0) sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        if(bothnewkeys & FLAG_MOVEDOWN){
            ++selector;

            if(SAMPLE_BEEP >= 0) sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        if(selector<0) selector = 6;
        if(selector>6) selector = 0;

        if(bothnewkeys & (FLAG_MOVELEFT|FLAG_MOVERIGHT|FLAG_ANYBUTTON)){
            dir = 0;

            if(bothnewkeys & FLAG_MOVELEFT) dir = -1;
            else if(bothnewkeys & FLAG_MOVERIGHT) dir = 1;

            if(SAMPLE_BEEP2 >= 0) sound_play_sample(SAMPLE_BEEP2, 0, savedata.effectvol,savedata.effectvol, 100);

            switch(selector){
                case 0:
                    savedata.soundvol += dir;
                    if(savedata.soundvol < 0) savedata.soundvol = 0;
                    if(savedata.soundvol > 15) savedata.soundvol = 15;
                    SB_setvolume(SB_VOICEVOL, savedata.soundvol);
                    break;
                case 1:
                    savedata.effectvol += 4*dir;
                    if(savedata.effectvol < 0) savedata.effectvol = 0;
                    if(savedata.effectvol > 512) savedata.effectvol = 512;
                    break;
                case 3:
                    if(!dir) break;
                    if(!savedata.usemusic){
                        savedata.usemusic = 1;
                        music("data/music/remix", 1, 0);
                    }
                    else{
                        savedata.usemusic = 0;
                        sound_close_music();
                    }
                    break;
                case 2:
                    savedata.musicvol += 4*dir;
                    if(savedata.musicvol < 0) savedata.musicvol = 0;
                    if(savedata.musicvol > 512) savedata.musicvol = 512;
                    sound_volume_music(savedata.musicvol, savedata.musicvol);
                    break;
                case 4:
                    savedata.showtitles = !savedata.showtitles;
                    break;
				case 5:
                    soundcard_options();
                    break;
                default:
                    quit = 1;
            }
        }
    }
    savesettings();
    bothnewkeys = 0;
}

void config_settings(){    //  OX. Load from / save to default.cfg. Restore OpenBoR "factory" settings.
    int quit = 0;
    int selector = 0;
	int dir = 0;
    int saved = 0;
	int loaded = 0;
	int restored = 0;

    bothnewkeys = 0;

    while(!quit){
        _menutextm(2, -5, 0, "Configuration Settings");

		if(saved == 1) _menutextm((selector==0), -2, 0, "Save Settings To Default.cfg%s", "  Done!");
        else _menutextm((selector==0), -2, 0, "Save Settings To Default.cfg%s","");

		if(loaded == 1) _menutextm((selector==1), -1, 0, "Load Settings From Default.cfg%s", "  Done!");
        else  _menutextm((selector==1), -1, 0, "Load Settings From Default.cfg%s", "");

		if(restored == 1) _menutextm((selector==2), 0, 0, "Restore OpenBoR Defaults%s", "  Done!");
        else _menutextm((selector==2), 0, 0, "Restore OpenBoR Defaults%s", "");

        _menutextm((selector==3), 1, 0, "Back");

        update(0,0);

        if(bothnewkeys & FLAG_ESC) quit = 1;
        if(bothnewkeys & FLAG_MOVEUP){
            --selector;

            if(SAMPLE_BEEP >= 0) sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        if(bothnewkeys & FLAG_MOVEDOWN){
            ++selector;

            if(SAMPLE_BEEP >= 0) sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }

		if(selector<0) selector = 3;
        if(selector>3) selector = 0;

        if(bothnewkeys & (FLAG_MOVELEFT|FLAG_MOVERIGHT|FLAG_ANYBUTTON)){
            dir = 0;

            if(bothnewkeys & FLAG_MOVELEFT) dir = -1;
            else if(bothnewkeys & FLAG_MOVERIGHT) dir = 1;

            if(SAMPLE_BEEP2 >= 0) sound_play_sample(SAMPLE_BEEP2, 0, savedata.effectvol,savedata.effectvol, 100);

            switch(selector){
                case 0:
                    saveasdefault();
					saved = 1;
                    break;

				case 1:
                    loadfromdefault();
					//shutdown(2, "\nSettings Loaded From Default.cfg. Restart Required.\n\n");
					init_videomodes(0);
                    SB_setvolume(SB_VOICEVOL, savedata.soundvol);
					sound_volume_music(savedata.musicvol, savedata.musicvol);
					loaded = 1;
                    break;
				case 2:
                    clearsettings();
					//shutdown(2, "\nSettings Loaded From Default.cfg. Restart Required.\n\n");
					init_videomodes(0);
                    SB_setvolume(SB_VOICEVOL, savedata.soundvol);
					sound_volume_music(savedata.musicvol, savedata.musicvol);
					restored = 1;
                    break;
                default:
                    quit = 1;
			}
		}
	}
	savesettings();
    bothnewkeys = 0;
}


void cheatoptions(){    //  LTB 1-13-05 took out sameplayer option
    int quit = 0;
    int selector = 0;
    int dir;
    int col1 = -8;
    int col2 = 6;

    bothnewkeys = 0;

    while(!quit){
        _menutextm(2, -5, 0, "Cheat Options");
        _menutext((selector==0), col1, -3, "Brightness:");
        _menutext((selector==0), col2, -3, "%i", savedata.brightness);
        _menutext((selector==1), col1, -2, "Gamma:");
        _menutext((selector==1), col2, -2, "%i", savedata.gamma);
        _menutext((selector==2), col1, -1, "Control Options...");
        _menutext((selector==3), col1, 0, "Sound Options...");
        _menutext((selector==4), col1, 1, "System Options...");

        if(livescheat)         _menutext((selector==5), col1, 2, "Infinite Lives On");
        else if(!livescheat)   _menutext((selector==5), col1, 2, "Infinite Lives Off");
        if(creditscheat)       _menutext((selector==6), col1, 3, "Infinite Credits On"); // Enemies fall down when you respawn
        else if(!creditscheat) _menutext((selector==6), col1, 3, "Infinite Credits Off");//Enemies don't fall down when you respawn
        if(healthcheat)        _menutext((selector==7), col1, 4, "Infinite Health On"); // Enemies fall down when you respawn
        else if(!healthcheat)  _menutext((selector==7), col1, 4, "Infinite Health Off");//Enemies don't fall down when you respawn

        _menutextm((selector==8), 7, 0, "Back");

        update(0,0);

        if(bothnewkeys & FLAG_ESC) quit = 1;
        if(bothnewkeys & FLAG_MOVEUP){
            --selector;

            if(SAMPLE_BEEP >= 0) sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        if(bothnewkeys & FLAG_MOVEDOWN){
            ++selector;

            if(SAMPLE_BEEP >= 0) sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }

        if(selector<0) selector = 8;  // 7-1-2005 //13-1-2005 changed to 11
        if(selector>8) selector = 0;//    7-1-2005 6 changed to 10 //13-1-2005 changed to 11

        if(bothnewkeys & (FLAG_MOVELEFT|FLAG_MOVERIGHT|FLAG_ANYBUTTON)){
            dir = 0;

            if(bothnewkeys & FLAG_MOVELEFT) dir = -1;
            else if(bothnewkeys & FLAG_MOVERIGHT) dir = 1;

            if(SAMPLE_BEEP2 >= 0) sound_play_sample(SAMPLE_BEEP2, 0, savedata.effectvol,savedata.effectvol, 100);

            switch(selector){
                case 0:
                    savedata.brightness += 8*dir;
                    if(savedata.brightness < -256) savedata.brightness = -256;
                    if(savedata.brightness > 256) savedata.brightness = 256;
                    vga_vwait();
                    palette_set_corrected(pal, savedata.gamma,savedata.gamma,savedata.gamma, savedata.brightness,savedata.brightness,savedata.brightness);
                    break;
                case 1:
                    savedata.gamma += 8*dir;
                    if(savedata.gamma < -256) savedata.gamma = -256;
                    if(savedata.gamma > 256) savedata.gamma = 256;
                    vga_vwait();
                    palette_set_corrected(pal, savedata.gamma,savedata.gamma,savedata.gamma, savedata.brightness,savedata.brightness,savedata.brightness);
                    break;
                case 2:
                    input_options();
                    break;
                case 3:
                    sound_options();
                    break;
                case 4:
                    system_options();
                    break;
                case 5:
                    if(!livescheat) livescheat = 1;
                    else if(livescheat) livescheat = 0;
                    break;
                case 6:
                    if(!creditscheat) creditscheat = 1;
                    else if(creditscheat) creditscheat = 0;
                    break;
                case 7:
                    if(!healthcheat) healthcheat = 1;
                    else if(healthcheat) healthcheat = 0;
                    break;

                default:
                    quit = 1;
            }
        }
    }
    savesettings();
    bothnewkeys = 0;
}


void system_options(){

    int quit = 0;
    int selector = 0;
    int dir = 0;
    int ret = 6;
    int col1 = -8;
    int col2 = 5;

#if PSP
    int batteryPercentage = 0;
    int batteryLifeTime = 0;
    int externalPower = 0;
#endif

    bothnewkeys = 0;

    while(!quit){
        _menutextm(2, -5, 0, "System Options");

        _menutext(0, col1, -2, "Total RAM:");
        _menutext(0, col2, -2, "%s KBytes", commaprint(getSystemRam(KBYTES)));

        _menutext(0, col1, -1, "Used RAM:");
        _menutext(0, col2, -1, "%s KBytes", commaprint(getUsedRam(KBYTES)));

        _menutext((selector==0), col1, 0, "Debug Info:");
        _menutext((selector==0), col2, 0, (savedata.debuginfo ? "Enabled" : "Disabled"));

        _menutext((selector==1), col1, 1, "File Logging:");
        _menutext((selector==1), col2, 1, (savedata.uselog ? "Enabled" : "Disabled"));

        _menutext((selector==2), col1, 2, "Players: ");
        if(!ctrlmaxplayers[current_set]) _menutext((selector==2), col2, 2, "%i", maxplayers[current_set]);
        else _menutext((selector==2), col2, 2, "%i by Mod", maxplayers[current_set]);

        _menutext((selector==3), col1, 3, "Versus Damage:", 0);
        if(versusdamage == 0) _menutext((selector==3), col2, 3, "Disabled by Mod");
        else if(versusdamage == 1) _menutext((selector==3), col2, 3, "Enabled by Mod");
        else
		{
            if(savedata.mode) _menutext((selector==3), col2, 3, "Disabled");//Mode 1 - Players CAN'T attack each other
            else _menutext((selector==3), col2, 3, "Enabled");//Mode 2 - Players CAN attack each other
        }

        _menutext((selector==4), col1, 4, "Cheats:");
        _menutext((selector==4), col2, 4, forcecheatsoff?"Disabled by Mod":(cheats?"On":"Off"));

#ifndef DC

		_menutext((selector==5), col1, 5, "Config Settings");

#endif

#if PSP
        externalPower = scePowerIsPowerOnline();
        _menutext((selector==6), col1, 6, "CPU Speed:");
        _menutext((selector==6), col2, 6, "%d MHz", scePowerGetCpuClockFrequency());
        if(!externalPower){
            batteryPercentage = scePowerGetBatteryLifePercent();
            batteryLifeTime = scePowerGetBatteryLifeTime();
            _menutext(0, col1, 7, "Battery:");
            if(batteryPercentage < 0 || batteryLifeTime < 0) _menutext(0, col2, 8, "Calculating...");
            else _menutext(0, col2, 7, "%d%% - %02d:%02d", batteryPercentage, batteryLifeTime/60,batteryLifeTime-(batteryLifeTime/60*60));
        }
        else{
            _menutext(0, col1, 7, "Charging:");
            _menutext(0, col2, 7, "%d%% AC Power", scePowerGetBatteryLifePercent());
        }
        ret = 7;
#endif

        _menutextm((selector==ret), 8, 0, "Back");

        update(0,0);

        if(bothnewkeys & FLAG_ESC) quit = 1;
        if(bothnewkeys & FLAG_MOVEUP){
            --selector;
            sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        if(bothnewkeys & FLAG_MOVEDOWN){
            ++selector;
            sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }

        if(selector < 0) selector = ret;
        if(selector > ret) selector = 0;

        if(bothnewkeys & (FLAG_MOVELEFT|FLAG_MOVERIGHT|FLAG_ANYBUTTON)){
            dir = 0;
            if(bothnewkeys & FLAG_MOVELEFT) dir = -1;
            else if(bothnewkeys & FLAG_MOVERIGHT) dir = 1;
            sound_play_sample(SAMPLE_BEEP2, 0, savedata.effectvol,savedata.effectvol, 100);

            switch(selector){
                case 0:
                    savedata.debuginfo = !savedata.debuginfo;
                    break;

                case 1:
                    savedata.uselog =  !savedata.uselog;
                    break;

                case 2:
                    if(!ctrlmaxplayers[current_set]){
                        if(maxplayers[current_set] == 2) maxplayers[current_set] = 4;
                        else maxplayers[current_set] = 2;
                    }
                    break;

                case 3:
                    if(versusdamage > 1)
					{
                        if(savedata.mode) savedata.mode = 0;
                        else              savedata.mode = 1;
                    }
                    break;

                case 4:
                    cheats = !cheats;
                    break;

#ifndef DC

				case 5:
					config_settings();
					break;

#endif

#ifdef PSP
                case 6:
                    savedata.pspcpuspeed += dir;
                    if(savedata.pspcpuspeed<0) savedata.pspcpuspeed = 2;
                    if(savedata.pspcpuspeed>2) savedata.pspcpuspeed = 0;
                    switch(savedata.pspcpuspeed){
                case 0:
                    scePowerSetClockFrequency(222, 222, 111);
                    break;
                case 1:
                    scePowerSetClockFrequency(266, 266, 133);
                    break;
                case 2:
                    scePowerSetClockFrequency(333, 333, 166);
                    break;
                    }
                    break;
#endif

                default:
                    quit = 1;
                    break;
            }
        }
    }
    savesettings();
    bothnewkeys = 0;
}


void video_options(){
    int quit = 0;
    int selector = 0;
    int dir;
    int col1 = -8, col2 = 6;

    bothnewkeys = 0;

    while(!quit){
        _menutextm(2, -5, 0, "Video Options");
        _menutext((selector==0), col1, -3, "Brightness:");
        _menutext((selector==0), col2, -3, "%i", savedata.brightness);
        _menutext((selector==1), col1, -2, "Gamma:");
        _menutext((selector==1), col2, -2, "%i", savedata.gamma);
        _menutext((selector==2), col1, -1, "Window Offset:");
        _menutext((selector==2), col2, -1, "%i", savedata.windowpos);

#if DOS || DC || GP2X || DINGOO
        _menutextm((selector==3), 7, 0, "Back");
        if(selector<0) selector = 3;
        if(selector>3) selector = 0;
#endif

#if XBOX
        _menutext((selector==3), col1, 0, "Screen Size:");
        _menutext((selector==3), col2, 0, "Guide R/L Thumbsticks");
        _menutext((selector==3), col1, 1, "GFX Filters:");
        _menutext((selector==3), col2, 1, "Press R/L Thumbsticks");
        _menutextm((selector==4), 7, 0, "Back");
        if(selector<0) selector = 4;
        if(selector>4) selector = 0;
#endif

#if WII
        _menutext((selector==3), col1, 0, "Display Mode:");
        _menutext((selector==3), col2, 0, savedata.fullscreen ? "Stretch to Screen" : "Preserve Aspect Ratio");
        _menutextm((selector==4), 7, 0, "Back");
        if(selector<0) selector = 4;
        if(selector>4) selector = 0;
#endif

#if SDL
#ifndef GP2X
        _menutext((selector==3), col1, 0, "Display Mode:");
        _menutext((selector==3), col2, 0, "%s", savedata.fullscreen ? "Full" : "Window");

        _menutext((selector==4), col1, 1, "Video Backend:");
        _menutext((selector==4), col2, 1, savedata.fullscreen ? "Automatic (%s)" : "%s", opengl ? "OpenGL" : "SDL");

        if(opengl)
        {
            _menutext((selector==5), col1, 2, "Screen:");
            if(savedata.fullscreen) _menutext((selector==5), col2, 2, "Automatic");
            else _menutext((selector==5), col2, 2, "%4.2fx - %ix%i", savedata.glscale, (int)(videomodes.hRes*savedata.glscale), (int)(videomodes.vRes*savedata.glscale));

            _menutext((selector==6), col1, 3, "Filters:");
            if(savedata.fullscreen) _menutext((selector==6), col2, 3, "Automatic (Bilinear)");
            else _menutext((selector==6), col2, 3, "%s", savedata.glscale!=1.0 ? (savedata.glfilter ? "Simple" : "Bilinear") : "Disabled");
        }
        else
        {
            _menutext((selector==5), col1, 2, "Screen:");
            if(savedata.screen[videoMode][0]) _menutext((selector==3), col2, 2, "%ix - %ix%i", savedata.screen[videoMode][0], videomodes.hRes*savedata.screen[videoMode][0], videomodes.vRes*savedata.screen[videoMode][0]);
            else _menutext((selector==5), col2, 2, "Disabled");

            _menutext((selector==6), col1, 3, "Filters:");
            _menutext((selector==6), col2, 3, "%s", savedata.screen[videoMode][0]==2 ? GfxBlitterNames[(int)savedata.screen[videoMode][1]] : "Disabled");
        }

        if(savedata.fullscreen)
        {
        	_menutext((selector==7), col1, 4, "Fullscreen Type:");
        	_menutext((selector==7), col2, 4, "%s", savedata.stretch ? "Stretch to Screen" : "Preserve Aspect Ratio");
        } else if(selector==7) selector = (bothnewkeys & FLAG_MOVEUP) ? 6 : 8;

        _menutextm((selector==8), 7, 0, "Back");
        if(selector<0) selector = 8;
        if(selector>8) selector = 0;
#endif
#endif

#if PSP
		_menutext((selector==3), col1, 0, "Screen:");
		_menutext((selector==3), col2, 0, "%s", displayFormat[(int)videomodes.mode].name);
		_menutext((selector==4), col1, 1, "Filters:");
		_menutext((selector==4), col2, 1, "%s", filterName[(int)videomodes.filter]);
		_menutext((selector==5), col1, 2, "Display:");
		_menutext((selector==5), col2, 2, "%s", displayName[displayMode]);
		_menutext((selector>=6 && selector<=9), col1, 3, "Overscan:");
		_menutext((selector>=6 && selector<=9), col2+1.5, 3, ".");
		_menutext((selector>=6 && selector<=9), col2+3.5, 3, ".");
		_menutext((selector>=6 && selector<=9), col2+5.5, 3, ".");
		_menutext((selector==6), col2, 3, "%02d", savedata.overscan[0]);
		_menutext((selector==7), col2+2, 3, "%02d", savedata.overscan[1]);
		_menutext((selector==8), col2+4, 3, "%02d", savedata.overscan[2]);
		_menutext((selector==9), col2+6, 3, "%02d", savedata.overscan[3]);
		_menutextm((selector==10), 7, 0, "Back");
		if(selector<0) selector = 10;
		if(selector>10) selector = 0;
#endif

        update(0,0);

        if(bothnewkeys & FLAG_ESC) quit = 1;
        if(bothnewkeys & FLAG_MOVEUP){
            --selector;
            if(SAMPLE_BEEP >= 0) sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        if(bothnewkeys & FLAG_MOVEDOWN){
            ++selector;
            if(SAMPLE_BEEP >= 0) sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        if(bothnewkeys & (FLAG_MOVELEFT|FLAG_MOVERIGHT|FLAG_ANYBUTTON)){
            dir = 0;

            if(bothnewkeys & FLAG_MOVELEFT) dir = -1;
            else if(bothnewkeys & FLAG_MOVERIGHT) dir = 1;

            if(SAMPLE_BEEP2 >= 0) sound_play_sample(SAMPLE_BEEP2, 0, savedata.effectvol,savedata.effectvol, 100);

            switch(selector){
                case 0:
                    savedata.brightness += 8*dir;
                    if(savedata.brightness < -256) savedata.brightness = -256;
                    if(savedata.brightness > 256) savedata.brightness = 256;
                    vga_vwait();
                    set_color_correction(savedata.gamma, savedata.brightness);
                    break;
                case 1:
                    savedata.gamma += 8*dir;
                    if(savedata.gamma < -256) savedata.gamma = -256;
                    if(savedata.gamma > 256) savedata.gamma = 256;
                    vga_vwait();
                    set_color_correction(savedata.gamma, savedata.brightness);
                    break;
                case 2:
                    savedata.windowpos += dir;
                    if(savedata.windowpos < -2) savedata.windowpos = -2;
                    if(savedata.windowpos > 20) savedata.windowpos = 20;
                    break;
#if SDL || PSP || XBOX || WII
                case 3:
#if XBOX
                    update(0,0);
                    xbox_resize();
#endif

#if WII
                    //video_fullscreen_flip();
                    video_stretch((savedata.stretch ^= 1));
                    break;
#endif

#if PSP
                    if(videoMode == 0)
					{   // 320x240
						videomodes.mode += dir;
						if(videomodes.mode > PSP_DISPLAY_FORMATS - 1) videomodes.mode = 0;
						if(videomodes.mode < 0) videomodes.mode = PSP_DISPLAY_FORMATS - 1;
						savedata.screen[videoMode][0] = videomodes.mode;
						video_set_mode(videomodes);
					}
                    break;

				case 4:
					if(videoMode == 0)
					{   // 320x240
						videomodes.filter += dir;
						if(videomodes.filter > PSP_DISPLAY_FILTERS - 1) videomodes.filter = 0;
						if(videomodes.filter < 0) videomodes.filter = PSP_DISPLAY_FILTERS - 1;
						savedata.screen[videoMode][1] = videomodes.filter;
						video_set_mode(videomodes);
					}
					break;

				case 5:
					displayMode += dir;
					if(displayMode > PSP_DISPLAY_MODES - 1) displayMode = 0;
					if(displayMode < 0) displayMode = PSP_DISPLAY_MODES - 1;
					if(displayMode)
						setGraphicsTVOverScan(savedata.overscan[0], savedata.overscan[1], savedata.overscan[2], savedata.overscan[3]);
					else
						setGraphicsTVOverScan(0, 0, 0, 0);
					savedata.usetv = displayMode;
					disableGraphics();
					initGraphics(savedata.usetv, videomodes.pixel);
					video_set_mode(videomodes);
					break;
				case 6:
				case 7:
				case 8:
				case 9:
                    savedata.overscan[selector-8] += dir;
                    if(savedata.overscan[selector-8] > 99) savedata.overscan[selector-8] = 0;
                    if(savedata.overscan[selector-8] < 0) savedata.overscan[selector-8] = 99;
                    if(displayMode)
					{
						setGraphicsTVOverScan(savedata.overscan[0], savedata.overscan[1], savedata.overscan[2], savedata.overscan[3]);
						video_set_mode(videomodes);
					}
                    break;
#endif
#endif


#if SDL
#ifndef GP2X
                    video_fullscreen_flip();
                    break;
                case 4:
                	if(savedata.fullscreen) break;
                    savedata.usegl ^= 1;
                    video_set_mode(videomodes);
                    set_color_correction(savedata.gamma, savedata.brightness);
                    break;
                case 5:
                    if(opengl)
                    {
                    	if(savedata.fullscreen) break;
                        savedata.glscale += dir * 0.25;
                        if(savedata.glscale < 0.25) savedata.glscale = 0.25;
                        if(savedata.glscale > 4.00) savedata.glscale = 4.00;
                        video_set_mode(videomodes);
                    }
                    else
                    {
	    				videomodes.mode += dir * 2;
#ifdef WII
					    // Wii with SDL is limited to 640x480
                        if(videomodes.mode > 2) videomodes.mode = 0;
					    if(videomodes.mode < 0) videomodes.mode = 2;
#else
                        if(videomodes.mode > 4) videomodes.mode = 0;
					    if(videomodes.mode < 0) videomodes.mode = 4;
#endif
					    savedata.screen[videoMode][0] = videomodes.mode;
                        video_set_mode(videomodes);
                        change_system_palette(current_palette);
                    }
                    break;
                case 6:
                    if(opengl)
                    {
                    	if(savedata.fullscreen || (savedata.glscale == 1.0)) break;
                        savedata.glfilter += dir;
                        if(savedata.glfilter < 0) savedata.glfilter = 1;
                        if(savedata.glfilter > 1) savedata.glfilter = 0;
                    }
                    else
                    {
                        if(videomodes.mode!=2) break;
                        videomodes.filter += dir;
                        if(videomodes.filter > BLITTER_MAX - 1) videomodes.filter = 0;
					    if(videomodes.filter < 0) videomodes.filter = BLITTER_MAX - 1;
                        savedata.screen[videoMode][1] = videomodes.filter;
                    }
                    break;
                case 7:
                	video_stretch((savedata.stretch ^= 1));
                	break;
#endif
#endif
                default:
                    quit = 1;
            }
        }
    }
    savesettings();
    bothnewkeys = 0;
}


void options(){
    int quit = 0;
    int selector = 0;
    int dir;

    bothnewkeys = 0;

    while(!quit){
        _menutextm(2, 0, 0, "Options");
        _menutextm((selector==0), 2, 0, "Video Options...");
        _menutextm((selector==1), 3, 0, "Sound Options...");
        _menutextm((selector==2), 4, 0, "Control Options...");
        _menutextm((selector==3), 5, 0, "System Options...");
        _menutextm((selector==4), 7, 0, "Back");

        if(selector<0) selector = 4;
        if(selector>4) selector = 0;

        update(0,0);

        if(bothnewkeys & FLAG_ESC) quit = 1;
        if(bothnewkeys & FLAG_MOVEUP){
            --selector;

            if(SAMPLE_BEEP >= 0) sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        if(bothnewkeys & FLAG_MOVEDOWN){
            ++selector;

            if(SAMPLE_BEEP >= 0) sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        if(bothnewkeys & (FLAG_MOVELEFT|FLAG_MOVERIGHT|FLAG_ANYBUTTON)){
            dir = 0;

            if(bothnewkeys & FLAG_MOVELEFT) dir = -1;
            else if(bothnewkeys & FLAG_MOVERIGHT) dir = 1;

            if(SAMPLE_BEEP2 >= 0) sound_play_sample(SAMPLE_BEEP2, 0, savedata.effectvol,savedata.effectvol, 100);

            switch(selector){
                case 0:
                    video_options();
                    break;
                case 1:
                    sound_options();
                    break;
                case 2:
                    input_options();
                    break;
                case 3:
                    system_options();
                    break;
                default:
                    quit = 1;
            }
        }
    }
    savesettings();
    if(pause==1) pause=2;
    bothnewkeys = 0;
}

void soundcard_options(){
    int quit = 0;
    int selector = 0;
    int col1=-8, col2=6;

    savesettings();

    bothnewkeys = 0;

    while(!quit){
        _menutextm(2, -5, 0, "Advanced Sound Options");
        _menutext((selector==0), col1, -2, "Frequency:");
        _menutext((selector==0), col2, -2, "%i", savedata.soundrate);
        _menutext((selector==1), col1, -1, "Bits:");
        _menutext((selector==1), col2, -1, "%i", savedata.soundbits);
        _menutextm((selector==2), 1, 0, "Apply");
        _menutextm((selector==3), 2, 0, "Discard");
        _menutextm((selector==4), 7, 0, "Back");
        update(0,0);

        if(bothnewkeys & FLAG_ESC) quit = 1;
        if(bothnewkeys & FLAG_MOVEUP){
            --selector;
            sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        if(bothnewkeys & FLAG_MOVEDOWN){
            ++selector;
            sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
        }
        if(selector<0) selector = 4;
        selector %= 5;
        if(bothnewkeys & (FLAG_MOVELEFT|FLAG_MOVERIGHT|FLAG_ANYBUTTON)){
            sound_play_sample(SAMPLE_BEEP2, 0, savedata.effectvol,savedata.effectvol, 100);
            switch(selector){
                case 0:
                    if(bothnewkeys & FLAG_MOVELEFT) savedata.soundrate >>= 1;
                    if(bothnewkeys & FLAG_MOVERIGHT) savedata.soundrate <<= 1;
                    if(savedata.soundrate < 11025) savedata.soundrate = 44100;
                    if(savedata.soundrate > 44100) savedata.soundrate = 11025;
                    break;
                case 1:
                    savedata.soundbits = (savedata.soundbits ^ (8+16));
                    if(savedata.soundbits!=8 && savedata.soundbits!=16) savedata.soundbits = 8;
                    break;
                case 2:
                    if(!(bothnewkeys & FLAG_ANYBUTTON)) break;
                    // Apply new hardware settings
                    sound_stop_playback();
                    if(!sound_start_playback(savedata.soundbits, savedata.soundrate)){
                        savedata.soundbits = 8;
                        savedata.soundrate = 11025;
                        sound_start_playback(savedata.soundbits, savedata.soundrate);
                    }
                    music("data/music/remix", 1, 0);
                    savesettings();
                    break;
                case 3:
                    if(bothnewkeys & FLAG_ANYBUTTON) loadsettings();
                    break;
                default:
                    quit = (bothnewkeys & FLAG_ANYBUTTON);
            }
        }
    }
    loadsettings();
    bothnewkeys = 0;
}

#ifdef XBOX
void display_logfile()
{
	int i, j, k;
	stringptr *logfile = NULL;
	char textpad[128] = {""};
	int currentline = 0;
	int filesize = 0;
	int done = 0;

	logfile = readFromLogFile(OPENBOR_LOG);
	if(logfile != NULL)
	{
		unload_background();
		load_background("menu/logview", 0);
		while(!done)
		{
			font_printf(5,3, 1, 0, "Log Viewer");
			font_printf(259,3, 1, 0, "Quit : Escape");
			filesize = logfile->size;
			if(bothkeys & FLAG_MOVEUP)
			{
				currentline -= 5;
				if(currentline < 0) currentline = 0;
			}
			if(bothkeys & FLAG_MOVEDOWN)
			{
				currentline += 5;
				if(currentline > filesize) currentline = filesize;
			}
			k = 0;
			j = 2;
			for(i=currentline; i<filesize; i++)
			{
				if(logfile->ptr[i] >= 0x20 && logfile->ptr[i] <= 0x7e)
				{
					textpad[k] = logfile->ptr[i];
					k++;
				}
				else if(logfile->ptr[i] == 0x09)
				{
					textpad[k+0] = ' '; textpad[k+1] = ' ';
					textpad[k+2] = ' '; textpad[k+3] = ' ';
					k+=4;
				}
				else
				{
					font_printf(5, j*10, 0, 0, "%s", textpad);
					j++;
					k=0;
					strncpy(textpad, "", 128);
				}
			}
			if(bothkeys & FLAG_ESC) done = 1;
			update(0,0);
		}
		free_string(logfile);
		logfile = NULL;
		unload_background();
		load_background("menu/logo", 0);
	}
}
#endif

// ----------------------------------------------------------------------------


void openborMain()
{
	sprite_map = NULL;
	int quit = 0;
	int relback = 1;
	int selector = 0;
	u32 introtime = 0;
	int started = 0;
	char tmpBuff[128] = {""};
	int players[MAX_PLAYERS];
	int i;

#if XBOX
	int done = 0;
	char pakname[128] = {""};
	char listing[32] = {""};
	int paks = 0;
	int list = 0;
	int lOffset=0;
	u32 menutime = 0;
#endif

	printf("OpenBoR %s, Compile Date: " __DATE__ "\n\n", VERSION);

#if XBOX
	loadsettings();
	paks = findmods();
	if(paks==1) getBasePath(packfile, paklist[0].filename, 1);
	else
	{
		strcpy(packfile, "d:\\Paks\\menu.pak");
		guistartup();
		load_background("menu/logo", 0);
		while(!done)
		{
			if(paks < 1) font_printf(10,24, 1, 0, "No Mods In Paks Folder!");
			if(bothnewkeys & FLAG_ESC)
			{
				disablelog = 1;
				shutdown(1, "");
			}

			for(list=0; list<paks; list++)
			{
				strncpy(pakname, paklist[list+lOffset].filename,128-strlen(paklist[list+lOffset].filename));
				if(list<18)
				{
					strncpy(listing, "", 32);
					strncpy(listing, pakname, 31);
					font_printf(10,24+(11*list), selector==list, 0, "%s", listing);
				}
			}
			if(bothkeys & FLAG_MOVEUP && time >= menutime)
			{
				--selector;
				menutime = time + GAME_SPEED/8;
			}
			if(bothkeys & FLAG_MOVEDOWN && time >= menutime)
			{
				++selector;
				menutime = time + GAME_SPEED/8;
			}
#ifndef DC
			if(bothkeys & FLAG_SPECIAL) display_logfile();
#endif
			if(selector>paks-1) selector=paks-1;
			if(selector>17)
			{
				if((selector+lOffset)<paks) lOffset++;
				selector=17;
			}
			if(selector<0)
			{
				selector=0;
				if(lOffset>0) lOffset--;
			}
			if((bothnewkeys&(FLAG_START)) && paks > 1)
			{
				strncpy(pakname, paklist[selector+lOffset].filename,128-strlen(paklist[selector+lOffset].filename));
				getBasePath(packfile, pakname, 1);
				done = 1;
			}

			font_printf(5,3, 2, 0, "OpenBoR %s", VERSION);
			font_printf(265,3, 2, 0, __DATE__);
			font_printf(197,155, 2, 0, "www.LavaLit.com");
			font_printf(190,165, 2, 0, "www.SenileTeam.com");
			font_printf(5,229, 2, 0, "Start : Load");
			font_printf(259,229, 2, 0, "Quit : Escape");

			if(done) font_printf(215,175, 2, 0, "Loading...");
			update(0,0);
		}

		if(paks != 1)
		{
			// unload whats been allocated.
			selector = 0;
			unload_background();
			freescreen(vscreen);
			unload_all_fonts();
			borTimerExit();
			control_exit();
		}
	}
#endif

	// Load necessary components.
	printf("Game Selected: %s\n\n", packfile);
	loadsettings();
	startup();

	// New alternative background path for PSP
	if(custBkgrds != NULL)
	{
		strcpy(tmpBuff,custBkgrds);
		strncat(tmpBuff,"logo", 4);
		load_background(tmpBuff, 0);
	}
	else {
		printf("use cached bg\n");
		load_cached_background("data/bgs/logo", 0);
	}	

	while(time<GAME_SPEED*6 && !(bothnewkeys&(FLAG_ANYBUTTON|FLAG_ESC))) update(0,0);

	music("data/music/remix", 1, 0);

    // New alternative scene path for PSP
	if(custScenes != NULL)
	{
		strncpy(tmpBuff,custScenes, 128);
		strncat(tmpBuff,"logo.txt", 8);
		playscene(tmpBuff);
	}
	else playscene("data/scenes/logo.txt");
	clearscreen(background);

	while(!quit)
	{
		if(time >= introtime)
		{
			// New alternative scene path for PSP
			if(custScenes != NULL)
			{
				strncpy(tmpBuff,custScenes, 128);
				strncat(tmpBuff,"intro.txt", 9);
				playscene(tmpBuff);
			}
			else playscene("data/scenes/intro.txt");
			update(0,0);
			introtime = time + GAME_SPEED * 20;
			relback = 1;
			started = 0;
		}

		if(bothnewkeys & FLAG_ESC) quit = 1;

		if(!started)
		{
			if((time%GAME_SPEED) < (GAME_SPEED/2)) _menutextm(0, 0, 0, "PRESS START");
			if(bothnewkeys&(FLAG_ANYBUTTON))
			{
				started = 1;
				relback = 1;
			}
		}
		else
		{
			_menutextm((selector==0), 2, 0, "Start Game");
#ifndef DC
			_menutextm((selector==1), 3, 0, "Movie Mode");
#else
			_menutextm((selector==1), 3, 0, "Movie Options(Disabled)");
#endif
			_menutextm((selector==2), 4, 0, "Options");
			_menutextm((selector==3), 5, 0, "How To Play");
			_menutextm((selector==4), 6, 0, "Hall Of Fame");
			_menutextm((selector==5), 7, 0, "Quit");
			if(selector<0) selector = 5;
			if(selector>5) selector = 0;

			if(bothnewkeys) introtime = time + GAME_SPEED * 20;

			if(bothnewkeys & FLAG_MOVEUP)
			{
				--selector;
				if(SAMPLE_BEEP >= 0) sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
			}
			if(bothnewkeys & FLAG_MOVEDOWN)
			{
				++selector;
				if(SAMPLE_BEEP >= 0) sound_play_sample(SAMPLE_BEEP, 0, savedata.effectvol,savedata.effectvol, 100);
			}
			if(bothnewkeys&(FLAG_ANYBUTTON))
			{
				if(SAMPLE_BEEP2 >= 0) sound_play_sample(SAMPLE_BEEP2, 0, savedata.effectvol,savedata.effectvol, 100);
				switch(selector)
				{
				case 0:
					for(i=0; i<MAX_PLAYERS; i++) players[i] = player[i].newkeys & (FLAG_ANYBUTTON);
					relback = choose_mode(players);
					if(relback) started = 0;
					break;
				case 1:
#ifndef DC
					movie_options();
#endif
					break;
				case 2:
					if(!cheats) options();
					else
					{
						if(!forcecheatsoff) cheatoptions();
						else options();
					}
					break;
				case 3:
					if(custScenes != NULL)
					{
						strncpy(tmpBuff,custScenes, 128);
						strncat(tmpBuff,"howto.txt", 9);
						playscene(tmpBuff);
					}
					else playscene("data/scenes/howto.txt");
					relback = 1;
					break;
				case 4:
					hallfame(0);
					relback = 1;
					break;
				default:
					quit = 1;
					break;
				}
				introtime = time + GAME_SPEED * 20;
			}
		}
		if(relback)
		{
			if(started)
			{
				if(custBkgrds != NULL)
				{
					strncpy(tmpBuff,custBkgrds, 128);
					strncat(tmpBuff,"titleb", 6);
					load_background(tmpBuff, 0);
				}
				else load_cached_background("data/bgs/titleb", 0);
			}
			else
			{
				if(custBkgrds != NULL)
				{
					strncpy(tmpBuff,custBkgrds, 128);
					strncat(tmpBuff,"title", 5);
					load_background(tmpBuff, 0);
				}
				else load_cached_background("data/bgs/title", 0);
			}

			if(!sound_query_music(NULL,NULL)) music("data/music/remix", 1, 0);
			relback = 0;
		}
		update(0,0);
	}

	shutdown(0,
		"OpenBoR %s, Compile Date: " __DATE__ "\n"
		"Presented by Team Senile.\n"
		"This Version is unofficial and based on the Senile Source Code.\n"
		"\n"
		"Special thanks to SEGA and SNK.\n\n",
		VERSION
	);
}

