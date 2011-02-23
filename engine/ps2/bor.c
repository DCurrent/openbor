/*
	Beats of Rage
	Side-scrolling beat-'em-up
*/


#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <libmc.h>
#include <fileio.h>

#include "ps2port.h"
#include "gsvga.h"

#include "types.h"
#include "video.h"
#include "vga.h"
#include "screen.h"
#include "loadimg.h"
#include "bitmap.h"
#include "sprite.h"
#include "spriteq.h"
#include "font.h"
#include "timer.h"
#include "rand32.h"
#include "soundmix.h"
#include "control.h"
#include "draw.h"
#include "packfile.h"
#include "palette.h"
#include "anigif.h"
#include "texture.h"

#pragma pack (4)

#define		VERSION			0x00010028
#define		COMPATIBLEVERSION	0x00000001


#define		GAME_SPEED		200
#define		THINK_SPEED		2
#define		COUNTER_SPEED		(GAME_SPEED*2)


#define		MAX_SPRITES		2000
#define		MAX_ANIMS		500
#define		MAX_MODELS		100
#define		MAX_ENTS		150
#define		MAX_PANELS		26
#define		ANI_MAX_FRAMES		32
#define		MAX_COLOUR_MAPS		7
#define		MAX_NAME_LEN		20
#define		LEVEL_MAX_SPAWNS	300
#define		LEVEL_MAX_PANELS	1000
#define		LEVEL_MAX_HOLES		20
#define		MAX_LEVELS		50
#define		MAX_DIFFICULTIES	5
#define		MAX_SOUND_CACHE		64


#define		FLAG_ESC		0x00000001
#define		FLAG_START		0x00000002
#define		FLAG_MOVELEFT		0x00000004
#define		FLAG_MOVERIGHT		0x00000008
#define		FLAG_MOVEUP		0x00000010
#define		FLAG_MOVEDOWN		0x00000020
#define		FLAG_ATTACK		0x00000040
#define		FLAG_JUMP		0x00000080
#define		FLAG_SPECIAL		0x00000100
#define		FLAG_SCREENSHOT		0x00000200
#define		FLAG_ANYBUTTON		(FLAG_START|FLAG_SPECIAL|FLAG_ATTACK|FLAG_JUMP)
#define         FLAG_ANYTHING           (FLAG_ANYBUTTON|FLAG_MOVELEFT|FLAG_MOVERIGHT|FLAG_MOVEUP|FLAG_MOVEDOWN)


#define		SDID_MOVEUP		0
#define		SDID_MOVEDOWN		1
#define		SDID_MOVELEFT		2
#define		SDID_MOVERIGHT		3
#define		SDID_SPECIAL		4
#define		SDID_ATTACK		5
#define		SDID_JUMP		6
#define		SDID_START		7
#define		SDID_SCREENSHOT		8


#define		TYPE_NONE		0
#define		TYPE_PLAYER		1
#define		TYPE_ENEMY		2
#define		TYPE_ITEM		4
#define		TYPE_OBSTACLE		8
#define		TYPE_STEAMER		16

#define		SUBTYPE_NONE		0
#define		SUBTYPE_BIKER		1



// Note: the minimum Z coordinate of the player is important
// for several other drawing operations.
#define		PLAYER_MIN_Z		160
#define		PLAYER_MAX_Z		232
#define		FRONTPANEL_Z		(PLAYER_MAX_Z+50)
#define		HOLE_Z			(PLAYER_MIN_Z-1)
#define		NEONPANEL_Z		(PLAYER_MIN_Z-2)
#define		SHADOW_Z		(PLAYER_MIN_Z-3)
#define		SCREENPANEL_Z		(PLAYER_MIN_Z-4)
#define		PANEL_Z			(PLAYER_MIN_Z-5)
#define		PIT_DEPTH		-250


// Distance to make contact
#define		CONTACT_DIST_H		30
#define		CONTACT_DIST_V		12

// Grabbing ents will be placed this far apart.
#define		GRAB_DIST		36
#define		THROW_DAMAGE		21

#define		GRAB_STALL		(GAME_SPEED * 8 / 10)


#define		ATK_NORMAL		0
#define		ATK_BLAST		1
#define		ATK_BURN		2
#define		ATK_FREEZE		3
#define		ATK_SHOCK		4

#define		SCROLL_RIGHT		0
#define		SCROLL_DOWN		1
#define		SCROLL_LEFT		2
#define		SCROLL_UP		3





#define		ANI_IDLE		0
#define		ANI_WALK		1
#define		ANI_JUMP		2
#define		ANI_LAND		3
#define		ANI_PAIN		4
#define		ANI_FALL		5
#define		ANI_RISE		6
#define		ANI_ATTACK1		7
#define		ANI_ATTACK2		8
#define		ANI_ATTACK3		9
#define		ANI_UPPER		10
#define		ANI_SLIDE		11
#define		ANI_JUMPATTACK		12
#define		ANI_JUMPATTACK2		13
#define		ANI_GET			14
#define		ANI_GRAB		15
#define		ANI_GRABATTACK		16
#define		ANI_GRABATTACK2		17
#define		ANI_THROW		18
#define		ANI_SPECIAL		19
#define		ANI_FREESPECIAL		21

#define		MAX_ANIS		22








typedef struct{
	unsigned gamelib_long	version;
	unsigned gamelib_long	compatibleversion;
	int		gamma;
	int		brightness;
	int		usesound;		// Use SB
	int		soundrate;		// SB freq
	int		soundvol;		// SB volume
	int		usemusic;		// Play music
	int		musicvol;		// Music volume
	int		effectvol;		// Sound fx volume
	int		soundbits;		// SB bits
	int		usejoy;

	// Just here for compatibility
	int		unused1;
	int		unused2[2][8];

	int		windowpos;
	int		times_completed;
	int		keys[2][9];
	int		showtitles;
	
	int		offset_X;
	int		offset_Y;
	int		interlace;
	int		videomode;
}s_savedata;



typedef struct{
	int		numframes;
	int 		loop;
	int		throwframe;
	int		shootframe;
	int		jumpframe;
	int		soundframe;
	int		soundtoplay;
	int		sprite[ANI_MAX_FRAMES];
	int		delay[ANI_MAX_FRAMES];
	int		move[ANI_MAX_FRAMES];
	int		vulnerable[ANI_MAX_FRAMES];
	int		bbox_coords[ANI_MAX_FRAMES][4];
	int		attack_coords[ANI_MAX_FRAMES][4];
	int		attack_force[ANI_MAX_FRAMES];
	int		attack_drop[ANI_MAX_FRAMES];
	int		attack_type[ANI_MAX_FRAMES];
	int		range[2];			// Use for attacks
}s_anim;



typedef struct{
	char		name[MAX_NAME_LEN+1];
	int		health;
	int		score;
	int		type;
	int		subtype;
	int		icon;
	int		shadow;
	int		diesound;
	int		secret;
	float		speed;
	char *		colourmap[MAX_COLOUR_MAPS];
	s_anim *	animation[MAX_ANIS];
}s_model;


// Cache list entry for models
typedef struct{
	char		name[MAX_NAME_LEN+1];
	char		path[256];
}s_modelcache;


typedef struct entity{
	int		exists;
	int		health;
	int		oldhealth;
	int		maxhealth;
	int		type;
	int		playerindex;
	char		name[MAX_NAME_LEN+1];
	char		item[MAX_NAME_LEN+1];
	int		if2p;

	float		x;		// X
	float		z;		// Depth
	float		a;		// Altitude
	float		xdir;
	float		zdir;
	float		xto;
	float		zto;

	float		base;		// Default altitude
	float		tossv;		// Effect of gravity
	unsigned gamelib_long	toss_time;	// Used by gravity code

	int		direction;	// 0=left 1=right

	int		boss;
	int		nograb;		// Some enemies cannot be grabbed (bikes)

	unsigned gamelib_long	stalltime;
	unsigned gamelib_long	combotime;	// For multiple-hit combo
	int		combostep;
	unsigned gamelib_long	movetime;	// For special move
	unsigned gamelib_long	lastmove;
	int		movestep;
	unsigned gamelib_long	releasetime;
	

	int		jumping;	// Stuff useful for AI
	int		attacking;
	int		getting;
	int		projectile;
	int		damage_on_landing;

	s_model *	model;

	s_anim *	animation;
	int		animpos;
	int		lastanimpos;	// Used by AI
	unsigned gamelib_long	nextanim;
	int		currentsprite;
	int		animating;	// Set by animation code
	int		autokill;	// Kill on end animation

	void		(*think)();
	unsigned gamelib_long	nextthink;
	void		(*takedamage)(struct entity*,int,int,int);
	int		attack_id;
	int		hit_by_attack_id;
	int		remove_on_attack;

	unsigned gamelib_long	pain_time;
	int		blink;
	int		screen;
	char *		colourmap;

	struct entity * link;		// Used to link 2 entities together.
}entity;



typedef struct{
	s_model *	model;
	int		colourmap;
	unsigned gamelib_long	score;
	int		lives;
	entity *	ent;
	entity *	opponent;
	unsigned gamelib_long	keys;
	unsigned gamelib_long	newkeys;
	unsigned gamelib_long	playkeys;
	int		spawnhealth;
	int		joining;
}s_player;


typedef struct{
	s_sprite * sprite_normal;
	s_sprite * sprite_neon;
	s_sprite * sprite_screen;
}s_panel;


typedef struct{
	int		at;
	int		wait;
	int		groupmin;
	int		groupmax;
	char		name[MAX_NAME_LEN+1];
	char		alias[MAX_NAME_LEN+1];
	char		item[MAX_NAME_LEN+1];
	int		if2p;
	int		health;
	int		x;
	int		z;
	int		a;
	int		colourmap;
	int		boss;
	int		flip;
}s_spawn_entry;


typedef struct{
	char		filename[128];
	int		gonext;
	int		is_scene;
}s_level_entry;


typedef struct{
	int		numspawns;
	s_spawn_entry	spawnpoints[LEVEL_MAX_SPAWNS];
	int		numpanels;
	int		order[LEVEL_MAX_PANELS];
	int		numholes;
	int		holes[LEVEL_MAX_HOLES];
	int		exit_blocked;
	int		exit_hole;
	int		scrolldir;
	int		width;
	int		rocking;
	int		mirror;
	char		bossmusic[256];
}s_level;

typedef struct{
	int		index;
	char		filename[256];
}s_soundcache;



s_savedata savedata;



char *packfile = "bor.pak";



char pal[768];
char * lut_mul = NULL;
char * lut_screen = NULL;
s_screen * vscreen = NULL;
s_screen * background = NULL;
s_screen * rescreen = NULL;
s_level *level = NULL;
s_bitmap *texture = NULL;


int credits;
int levelpos;
float advancex;
float advancey;
unsigned gamelib_long advancetime;

int current_spawn;
int level_waiting;
int groupmin, groupmax;

int quake = 0;
unsigned gamelib_long quaketime;
unsigned gamelib_long go_time;

int level_completed;
int pause;
int endgame;
int showtimeover;

int gfx_y_offset = 0;


unsigned gamelib_long bortime = 0;
int timeleft;


int allow_secret_chars = 0;


s_level_entry *levelorder[MAX_DIFFICULTIES][MAX_LEVELS];
unsigned int num_levels[MAX_DIFFICULTIES];
unsigned int ifcomplete[MAX_DIFFICULTIES];
char set_names[MAX_DIFFICULTIES][MAX_NAME_LEN+1];
unsigned int num_difficulties;

s_panel panels[MAX_PANELS];
unsigned int panels_loaded = 0;
int panel_width = 0;

s_sprite *frontpanels[MAX_PANELS];
unsigned int frontpanels_loaded = 0;

s_sprite *sprites[2][MAX_SPRITES];
unsigned int sprites_loaded = 0;

s_model * model_list[MAX_MODELS];
unsigned int models_loaded = 0;

s_modelcache * model_cache[MAX_MODELS];
unsigned int models_cached = 0;

s_anim * anim_list[MAX_ANIMS];
unsigned int anims_loaded = 0;

entity * ent_list[MAX_ENTS];
entity * self;

s_soundcache soundcache[MAX_SOUND_CACHE];

s_player player[2];
unsigned gamelib_long bothkeys, bothnewkeys;


s_playercontrols playercontrols1;
s_playercontrols playercontrols2;
s_playercontrols * playercontrolpointers[] = {
	&playercontrols1,
	&playercontrols2
};




// Funny neon lights
char neontable[256];
unsigned gamelib_long neon_time = 0;


float lasthitx, lasthitz, lasthita;



// Special sprites
int shadowsprites[6];
int gosprite;
int holesprite;





// Some colours used here and there
int color_black = 0;
int color_red = 0;
int color_orange = 0;
int color_yellow = 0;
int color_white = 0;


// Change to array
int smp_beat;
int smp_indirect;
int smp_get;
int smp_get2;
int smp_fall;
int smp_jump;
int smp_punch;
int smp_1up;
int smp_timeover;
int smp_beep;
int smp_beep2;
int smp_bike;




int cheat_buttons_queue[10] = {
    FLAG_MOVEUP, FLAG_MOVEDOWN, FLAG_ATTACK, FLAG_JUMP,
    FLAG_MOVERIGHT, FLAG_MOVELEFT, FLAG_SPECIAL, FLAG_ATTACK,
    FLAG_JUMP, FLAG_SPECIAL
};

int cheat_mode = 0;

int cheat_cursor = 0;





// Required prototypes
void shutdown(char *, ...);
void debug_printf(char *, ...);




// ------------------------ Save/load -----------------------------

void clearsettings(){
	savedata.version = VERSION;
	savedata.compatibleversion = COMPATIBLEVERSION;
	savedata.gamma = 0;
	savedata.brightness = 0;
	savedata.usesound = 1;
	savedata.soundrate = 11025;
	savedata.soundvol = 14;
	savedata.usemusic = 1;
	savedata.musicvol = 128;
	savedata.effectvol = 48;
	savedata.soundbits = 8;
	savedata.usejoy = 1;

	savedata.keys[0][SDID_MOVEUP]    = CONTROL_DEFAULT1_UP;
	savedata.keys[0][SDID_MOVEDOWN]  = CONTROL_DEFAULT1_DOWN;
	savedata.keys[0][SDID_MOVELEFT]  = CONTROL_DEFAULT1_LEFT;
	savedata.keys[0][SDID_MOVERIGHT] = CONTROL_DEFAULT1_RIGHT;
	savedata.keys[0][SDID_SPECIAL]   = CONTROL_DEFAULT1_SPECIAL;
	savedata.keys[0][SDID_ATTACK]    = CONTROL_DEFAULT1_ATTACK;
	savedata.keys[0][SDID_JUMP]      = CONTROL_DEFAULT1_JUMP;
	savedata.keys[0][SDID_START]     = CONTROL_DEFAULT1_START;
	savedata.keys[0][SDID_SCREENSHOT]= CONTROL_DEFAULT1_SCREENSHOT;

	savedata.keys[1][SDID_MOVEUP]    = CONTROL_DEFAULT2_UP;
	savedata.keys[1][SDID_MOVEDOWN]  = CONTROL_DEFAULT2_DOWN;
	savedata.keys[1][SDID_MOVELEFT]  = CONTROL_DEFAULT2_LEFT;
	savedata.keys[1][SDID_MOVERIGHT] = CONTROL_DEFAULT2_RIGHT;
	savedata.keys[1][SDID_SPECIAL]   = CONTROL_DEFAULT2_SPECIAL;
	savedata.keys[1][SDID_ATTACK]    = CONTROL_DEFAULT2_ATTACK;
	savedata.keys[1][SDID_JUMP]      = CONTROL_DEFAULT2_JUMP;
	savedata.keys[1][SDID_START]     = CONTROL_DEFAULT2_START;
	savedata.keys[1][SDID_SCREENSHOT]= CONTROL_DEFAULT2_SCREENSHOT;

	savedata.windowpos = 0;
	savedata.times_completed = 0;
	savedata.showtitles = 0;
	
	savedata.offset_X = 0;
	savedata.offset_Y = 0;
	
	savedata.interlace = 1;
	savedata.videomode = 1;
}

extern unsigned char icon_icn[];
extern unsigned int size_icon_icn;

void savesettings(){
	int handle;
	int was_blocking;
	mcIcon icon_sys;
	
	static iconIVECTOR bgcolor[4] = {
		{  68,  23, 116,  0 }, // top left
		{ 255, 255, 255,  0 }, // top right
		{ 255, 255, 255,  0 }, // bottom left
		{  68,  23, 116,  0 }, // bottom right
	    };

	static iconFVECTOR lightdir[3] = {
		{ 0.5, 0.5, 0.5, 0.0 },
		{ 0.0,-0.4,-0.1, 0.0 },
		{-0.5,-0.5, 0.5, 0.0 },
	};

	static iconFVECTOR lightcol[3] = {
		{ 0.3, 0.3, 0.3, 0.00 },
		{ 0.4, 0.4, 0.4, 0.00 },
		{ 0.5, 0.5, 0.5, 0.00 },
	};

	static iconFVECTOR ambient = { 0.50, 0.50, 0.50, 0.00 };
	
	if (!(was_blocking = blocking))
		fioSetBlockMode(FIO_WAIT);
	
	handle = fioOpen("mc0:BEATSOFRAGE/icon.sys", O_RDONLY);
	if(handle <= 0) {
		if (fioMkdir("mc0:BEATSOFRAGE") < 0) {
			if (!was_blocking)
				fioSetBlockMode(FIO_NOWAIT);
			return;
		}
		memset(&icon_sys, 0, sizeof(mcIcon));
		strcpy(icon_sys.head, "PS2D");
		strcpy_sjis((short *)&icon_sys.title, "Beats of Rage\nSettings");
		icon_sys.nlOffset = 16;
		icon_sys.trans = 0x60;
    	        memcpy(icon_sys.bgCol, bgcolor, sizeof(bgcolor));
		memcpy(icon_sys.lightDir, lightdir, sizeof(lightdir));
		memcpy(icon_sys.lightCol, lightcol, sizeof(lightcol));
		memcpy(icon_sys.lightAmbient, ambient, sizeof(ambient));
		strcpy(icon_sys.view, "icon.icn"); // these filenames are relative to the directory
		strcpy(icon_sys.copy, "icon.icn"); // in which icon.sys resides.
		strcpy(icon_sys.del, "icon.icn");

		if ((handle = fioOpen("mc0:BEATSOFRAGE/icon.sys", O_WRONLY | O_CREAT)) < 0) {
			if (!was_blocking)
				fioSetBlockMode(FIO_NOWAIT);
			return;
		}
		
		fioWrite(handle, &icon_sys, sizeof(icon_sys));
		fioClose(handle);
		
		if ((handle = fioOpen("mc0:BEATSOFRAGE/icon.icn", O_WRONLY | O_CREAT)) < 0) {
			if (!was_blocking)
				fioSetBlockMode(FIO_NOWAIT);
			return;
		}
		
		fioWrite(handle, icon_icn, size_icon_icn);
		fioClose(handle);
	} else {
		fioClose(handle);
	}
	handle = fioOpen("mc0:BEATSOFRAGE/settings.sav", O_WRONLY | O_CREAT | O_TRUNC);
	if (handle < 0) {
		if (!was_blocking)
			fioSetBlockMode(FIO_NOWAIT);
		return;
	}
	fioWrite(handle, &savedata, sizeof(s_savedata));
	fioClose(handle);
}

void loadsettings(){
	int handle;
	int was_blocking;
	clearsettings();
	if (!(was_blocking = blocking))
		fioSetBlockMode(FIO_WAIT);

	handle = fioOpen("mc0:BEATSOFRAGE/settings.sav", O_RDONLY);
	if (handle < 0) {
		if (!was_blocking)
			fioSetBlockMode(FIO_NOWAIT);
		return;
	}

	fioRead(handle, &savedata, sizeof(s_savedata));
	fioClose(handle);

	if(savedata.compatibleversion != COMPATIBLEVERSION) clearsettings();

	if (!was_blocking)
		fioSetBlockMode(FIO_NOWAIT);
}


// ----------------------- Sound ------------------------------


void music(char *filename, int loop){
//	char t[64];
//	char a[64];

	if(!savedata.usemusic) return;

	sound_async_open_music(filename, packfile, savedata.musicvol, loop);
}

void check_music_opened(void){
        char t[64];
        char a[64];

        if(!savedata.usemusic) return;

        if(!sound_was_music_opened()) return;

        if(savedata.showtitles && sound_query_music(a,t)){
                if(a[0] && t[0]) debug_printf("Playing \"%s\" by %s", t, a);
                else if(a[0]) debug_printf("Playing unknown song by %s", a);
                else if(t[0]) debug_printf("Playing \"%s\" by unknown artist", t);
        }
}

// Load a sound or return index from cache
int loadcache_sound(char *filename){
	int i;

	for(i=0; i<MAX_SOUND_CACHE; i++){
		if(strcmp(filename, soundcache[i].filename)==0) return soundcache[i].index;
	}

	for(i=0; i<MAX_SOUND_CACHE; i++){
		if(!soundcache[i].filename[0]) break;
	}
	if(i==MAX_SOUND_CACHE) return -1;

	soundcache[i].index = sound_load_sample(filename, packfile);
	if(soundcache[i].index < 0) return -1;
	strcpy(soundcache[i].filename, filename);
	return soundcache[i].index;
}



// ----------------------- General ------------------------------




#define		ARG_MAX_LEN		512

char * findarg(char *command, int which){
	int d;
	int argc;
	int inarg;
	int argstart;
	static char arg[ARG_MAX_LEN];


	// Copy the command line, replacing spaces by zeroes,
	// finally returning a pointer to the requested arg.
	d = 0;
	inarg = 0;
	argstart = 0;
	argc = -1;
	while(d<ARG_MAX_LEN-1 && command[d]){
		// Zero out whitespace
		if(command[d]==' ' || command[d]=='\t'){
			arg[d] = 0;
			inarg = 0;
			if(argc == which) return arg + argstart;
		}
		else if(command[d]==0 || command[d]=='\n' || command[d]=='\r' || command[d]=='#'){
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


int inair(entity *e){
	return (diff(e->a, e->base) >= 0.1);
}


float randf(float max){
	float f;
	if(max==0) return 0;
	f = (rand32()%1000);
	f /= (1000/max);
	return f;
}



// ----------------------- Loaders ------------------------------






// Creates a remapping table from two images
int load_colourmap(s_model * model, char *image1, char *image2){
	int i, j, k;
	char *map;
	s_bitmap *bitmap1;
	s_bitmap *bitmap2;

	// Can't use same image twice!
	if(stricmp(image1,image2)==0) return 0;

	// Find an empty slot... ;)
	for(k=0; k<MAX_COLOUR_MAPS && model->colourmap[k]; k++);
	if(k>=MAX_COLOUR_MAPS) return 0;

	map = (char*)tracemalloc("load_colourmap", 256);
	if(map==NULL) return 0;

	bitmap1 = loadbitmap(image1, packfile);
	if(bitmap1==NULL){
		tracefree(map);
		return 0;
	}
	bitmap2 = loadbitmap(image2, packfile);
	if(bitmap2==NULL){
		freebitmap(bitmap1);
		tracefree(map);
		return 0;
	}

	// Create the colour map
	for(i=0;i<256;i++) map[i] = i;
	for(j=0; j<bitmap1->height && j<bitmap2->height; j++){
		for(i=0; i<bitmap1->width && i<bitmap2->width; i++){
			map[(unsigned)(bitmap1->data[j*bitmap1->width+i])] = bitmap2->data[j*bitmap2->width+i];
		}
	}

	freebitmap(bitmap1);
	freebitmap(bitmap2);

	model->colourmap[k] = map;
	return 1;
}



// Load colour 0-127 from data/pal.act
void standard_palette(){
	int handle;
	handle = openpackfile("data/pal.act", packfile);
	readpackfile(handle, pal, 128*3);
	closepackfile(handle);
	pal[0] = pal[1] = pal[2] = 0;
	palette_set_corrected(pal, savedata.gamma,savedata.gamma,savedata.gamma, savedata.brightness,savedata.brightness,savedata.brightness);
}



void unload_background(){
	if(background) freescreen(background);
	background = NULL;
	if(lut_mul) tracefree(lut_mul);
	lut_mul = NULL;
	if(lut_screen) tracefree(lut_screen);
	lut_screen = NULL;
}


void load_background(char *filename, int createtables){
//	s_screen * screen;

	unload_background();
	video_clearscreen();

	background = loadscreen(filename, packfile, pal);
	if(background==NULL) shutdown("Error loading file '%s'", filename);
	pal[0] = pal[1] = pal[2] = 0;
	palette_set_corrected(pal, savedata.gamma,savedata.gamma,savedata.gamma, savedata.brightness,savedata.brightness,savedata.brightness);

	clearscreen(vscreen);
	spriteq_clear();
	font_printf(120,110, 0, "Loading...");
	spriteq_draw(vscreen);
	if(rescreen){
		scalescreen(rescreen, vscreen);
		video_copy_screen(rescreen);
	}
	else video_copy_screen(vscreen);

	color_black = palette_find(pal, 0,0,0);
	color_red = palette_find(pal, 255,0,0);
	color_orange = palette_find(pal, 255,150,0);
	color_yellow = palette_find(pal, 0xF8,0xB8,0x40);
	color_white = palette_find(pal, 255,255,255);

	if(createtables){
		standard_palette();

		lut_mul = palette_table_multiply(pal);
		if(lut_mul==NULL) shutdown("Failed to create colour conversion table! (Out of memory?)");
		lut_screen = palette_table_screen(pal);
		if(lut_screen==NULL) shutdown("Failed to create colour conversion table! (Out of memory?)");
	}
}



void unload_texture(){
	if(texture) freebitmap(texture);
	texture = NULL;
}

void load_texture(char *filename){
	unload_texture();
	texture = loadbitmap(filename, packfile);
	if(texture==NULL) shutdown("Error loading file '%s'", filename);
}







void freepanels(){
	int i;
	for(i=0; i<MAX_PANELS; i++){
		if(panels[i].sprite_normal) tracefree(panels[i].sprite_normal);
		panels[i].sprite_normal = NULL;
		if(panels[i].sprite_neon) tracefree(panels[i].sprite_neon);
		panels[i].sprite_neon = NULL;
		if(panels[i].sprite_screen) tracefree(panels[i].sprite_screen);
		panels[i].sprite_screen = NULL;

		if(frontpanels[i]) tracefree(frontpanels[i]);
		frontpanels[i] = NULL;
	}
	panels_loaded = 0;
	frontpanels_loaded = 0;
	panel_width = 0;
}




s_sprite * loadpanel2(char *filename){
	int size;
	s_bitmap *bitmap;
	s_sprite *sprite;
	int clipl, clipr, clipt, clipb;

	bitmap = loadbitmap(filename, packfile);
	if(!bitmap) return NULL;
	if(bitmap->width > panel_width) panel_width = bitmap->width;
	clipbitmap(bitmap, &clipl, &clipr, &clipt, &clipb);
	size = fakey_encodesprite(bitmap);
	sprite = (s_sprite*)tracemalloc("loadpanel2", size);
	if(!sprite){
		freebitmap(bitmap);
		return NULL;
	}
	encodesprite(-clipl, -clipt, bitmap, sprite);
	freebitmap(bitmap);

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
		i++;
	}
	if(stricmp(filename_screen,"none")!=0 && *filename_screen){
		panels[panels_loaded].sprite_screen = loadpanel2(filename_screen);
		if(panels[panels_loaded].sprite_screen == NULL) return 0;
		i++;
	}
	if(i<1) return 0;	// Nothing was loaded!

	++panels_loaded;
	
	return 1;
}



int loadfrontpanel(char *filename){
//	int x, y;
//	int i;
	int size;
	s_bitmap *bitmap;
	int clipl, clipr, clipt, clipb;


	if(frontpanels_loaded >= MAX_PANELS) return 0;
	bitmap = loadbitmap(filename, packfile);
	if(!bitmap) return 0;

	clipbitmap(bitmap, &clipl, &clipr, &clipt, &clipb);

	size = fakey_encodesprite(bitmap);
	frontpanels[frontpanels_loaded] = (s_sprite*)tracemalloc("loadfrontpanel", size);
	if(!frontpanels[frontpanels_loaded]){
		freebitmap(bitmap);
		return 0;
	}
	encodesprite(-clipl, -clipt, bitmap, frontpanels[frontpanels_loaded]);

	freebitmap(bitmap);
	++frontpanels_loaded;
	
	return 1;
}







void freesprites(){
	int i;
	for(i=0; i<MAX_SPRITES; i++){
		if(sprites[0][i]) tracefree(sprites[0][i]);
		sprites[0][i] = NULL;
		if(sprites[1][i]) tracefree(sprites[1][i]);
		sprites[1][i] = NULL;
	}
	sprites_loaded = 0;
}







// Returns sprite index.
// Does not return on error, as it would shut the program down.
int loadsprite(char *filename, int ofsx, int ofsy){
//	int x, y;
//	int i;
	int size;
	s_bitmap *bitmap;
	int clipl, clipr, clipt, clipb;


	if(sprites_loaded >= MAX_SPRITES) shutdown("Too many sprites (max. %u)", MAX_SPRITES);
	bitmap = loadbitmap(filename, packfile);
	if(!bitmap) shutdown("Unable to load file '%s' (may be out of memory)", filename);

	clipbitmap(bitmap, &clipl, &clipr, &clipt, &clipb);

	size = fakey_encodesprite(bitmap);
	sprites[1][sprites_loaded] = (s_sprite*)tracemalloc("loadsprite", size);
	if(!sprites[1][sprites_loaded]){
		freebitmap(bitmap);
		shutdown("Out of memory!");
	}
	encodesprite(ofsx-clipl, ofsy-clipt, bitmap, sprites[1][sprites_loaded]);

	// Now flip it
	flipbitmap(bitmap);
	size = fakey_encodesprite(bitmap);
	sprites[0][sprites_loaded] = (s_sprite*)tracemalloc("loadsprite", size);
	if(!sprites[0][sprites_loaded]){
		freebitmap(bitmap);
		shutdown("Out of memory!");
	}
	encodesprite((bitmap->width+clipl)-ofsx-1, ofsy-clipt, bitmap, sprites[0][sprites_loaded]);

	freebitmap(bitmap);
	++sprites_loaded;
	
	return sprites_loaded-1;
}




s_model * find_model(char *name){
	int i;
	for(i=0; i<models_loaded; i++){
		if(stricmp(model_list[i]->name, name)==0){
			return model_list[i];
		}
	}
	return NULL;
}



// Use by player select menus
s_model * nextplayermodel(void *current){
	int i;
	int curindex = -1;
	int loops;

	if(current){
		// Find index of current player model
		for(i=0; i<models_loaded; i++){
			if(model_list[i] == current){
				curindex = i;
				break;
			}
		}
	}

	// Find next player model (first one after current index)
	
	for(i=curindex+1, loops=0; loops<models_loaded; i++, loops++){
		if(i >= models_loaded) i = 0;
		if(model_list[i]->type==TYPE_PLAYER && (allow_secret_chars || !model_list[i]->secret)){
			return model_list[i];
		}
	};

	shutdown("Fatal: can't find any player models!");
	return NULL;
}




// Use by player select menus
s_model * prevplayermodel(void *current){
	int i;
	int curindex = -1;
	int loops;

	if(current){
		// Find index of current player model
		for(i=0; i<models_loaded; i++){
			if(model_list[i] == current){
				curindex = i;
				break;
			}
		}
	}

	// Find next player model (first one after current index)
	for(i=curindex-1, loops=0; loops<models_loaded; i--, loops++){
		if(i < 0) i = models_loaded-1;
		if(model_list[i]->type==TYPE_PLAYER && (allow_secret_chars || !model_list[i]->secret)){
			return model_list[i];
		}
	};

	shutdown("Fatal: can't find any player models!");
	return NULL;
}



// Unload all models and animations
void free_models(){
	int i;
	int j;
	for(i=0; i<MAX_MODELS; i++){
		if(model_list[i]){
			for(j=0;j<MAX_COLOUR_MAPS;j++){
				if(model_list[i]->colourmap[j]) tracefree(model_list[i]->colourmap[j]);
			}
			tracefree(model_list[i]);
			model_list[i] = NULL;
		}
	}
	models_loaded = 0;

	for(i=0; i<MAX_ANIMS; i++){
		if(anim_list[i]){
			tracefree(anim_list[i]);
			anim_list[i] = NULL;
		}
	}
	anims_loaded = 0;
}



s_anim * alloc_anim(){
	s_anim * new;

	if(anims_loaded >= MAX_ANIMS) return NULL;
	new = (s_anim *)tracemalloc("alloc_anim", sizeof(s_anim));
	if(new == NULL) return NULL;
	memset(new, 0, sizeof(s_anim));

	anim_list[anims_loaded] = new;
	++anims_loaded;

	return new;
}







// Add another frame to an animation (if possible)
int addframe(s_anim * a, int spriteindex, int delay, int *bbox, int *attack, int attackforce, int attackdrop, int attacktype, int move){
	if(a->numframes >= ANI_MAX_FRAMES) return ANI_MAX_FRAMES;
	a->sprite[a->numframes] = spriteindex;
	a->delay[a->numframes] = delay * GAME_SPEED / 100;
	if((bbox[2]-bbox[0]) && (bbox[3]-bbox[1])){
		a->bbox_coords[a->numframes][0] = bbox[0];
		a->bbox_coords[a->numframes][1] = bbox[1];
		a->bbox_coords[a->numframes][2] = bbox[2];
		a->bbox_coords[a->numframes][3] = bbox[3];
		a->vulnerable[a->numframes] = 1;
	}
	a->attack_coords[a->numframes][0] = attack[0];
	a->attack_coords[a->numframes][1] = attack[1];
	a->attack_coords[a->numframes][2] = attack[2];
	a->attack_coords[a->numframes][3] = attack[3];
	a->attack_force[a->numframes] = attackforce;
	a->attack_drop[a->numframes] = attackdrop;
	a->attack_type[a->numframes] = attacktype;
	a->move[a->numframes] = move;
	++a->numframes;
	return a->numframes;
}




void cache_model(char *name, char *path){
	if(models_cached >= MAX_MODELS) shutdown("Too many models, unable to cache '%s'", name);
	debug_printf("Cacheing '%s'\n", name);

	model_cache[models_cached] = tracemalloc("cache_model", sizeof(s_modelcache));
	if(model_cache[models_cached] == NULL) shutdown("Out of memory cacheing '%s'", name);

	strncpy(model_cache[models_cached]->name, name, MAX_NAME_LEN);
	strncpy(model_cache[models_cached]->path, path, 255);

	++models_cached;
}


void remove_from_cache(char * name){
	int i;
	void *tp;

	for(i=0; i<models_cached; i++){
		if(stricmp(name, model_cache[i]->name)==0){
			tp = model_cache[i];
			model_cache[i] = model_cache[models_cached-1];
			tracefree(tp);
			model_cache[models_cached-1] = NULL;
			models_cached--;
			return;
		}
	}
}


void free_modelcache(){
	int i;
	for(i=0; i<MAX_MODELS; i++){
		if(model_cache[i]) tracefree(model_cache[i]);
		model_cache[i] = NULL;
	}
	models_cached = 0;
}


char *get_cached_model_path(char * name){
	int i;

	for(i=0; i<models_cached; i++){
		if(stricmp(name, model_cache[i]->name)==0){
			return model_cache[i]->path;
		}
	}
	return NULL;
}



void load_cached_model(char * name){

	char *filename;

	int handle;
	char *buf;
	unsigned int size;
	int pos;

	s_model * newchar;
	s_anim * newanim = NULL;
	int index;

	char * command;
	char * value;
	char * value2;

	int curframe = 0;

	int delay = 0;
	int bbox[4] = { 0,0,0,0 };
	int attack[4] = { 0,0,0,0 };
	int attackforce = 0;
	int attackdrop = 0;
	int attacktype = 0;
	int offset[2] = { 0,0 };
	int move = 0;

	int bbox_con[4];
	int attack_con[4];


	if(find_model(name)) return;		// Model already loaded


	filename = get_cached_model_path(name);
	if(filename == NULL){
		shutdown("Fatal: no cache entry for '%s'", name);
	}


	if(models_loaded >= MAX_MODELS){
		shutdown("Cannot load model from '%s' - too many models", filename);
	}
	debug_printf("Loading model %s...\n", filename);

	if((handle=openpackfile(filename,packfile)) < 0) shutdown("Unable to open file '%s'", filename);
	size = seekpackfile(handle,0,SEEK_END);
	seekpackfile(handle,0,SEEK_SET);

	buf = (char*)tracemalloc("load_cached_model", size+1);
	if(buf==NULL){
		closepackfile(handle);
		shutdown("Unable to create buffer for file '%s' (%i bytes)", filename, size);
	}
	if(readpackfile(handle, buf, size) != size){
		tracefree(buf);
		closepackfile(handle);
		shutdown("Read error accessing file '%s'", filename);
	}
	buf[size] = 0;		// Terminate string (important!)
	closepackfile(handle);


	// Alloc space for game model
	newchar = (s_model *)tracemalloc("load_cached_model", sizeof(s_model));
	if(newchar == NULL){
		tracefree(buf);
		shutdown("Out of memory loading model from '%s'", filename);
	}
	model_list[models_loaded] = newchar;
	memset(newchar,0,sizeof(s_model));
	newchar->speed = 1;
	newchar->icon = -1;
	models_loaded++;


	// Now interpret the contents of buf line by line
	pos = 0;
	while(pos<size){
		command = findarg(buf+pos, 0);
		if(command[0]){
			if(stricmp(command, "name")==0){
				value = findarg(buf+pos, 1);
				if(find_model(value)){
					tracefree(buf);
					shutdown("Duplicate model name '%s'", value);
				}
				strncpy(newchar->name, value, MAX_NAME_LEN);
			}
			else if(stricmp(command, "health")==0){
				value = findarg(buf+pos, 1);
				newchar->health = atoi(value);
			}
			else if(stricmp(command, "load")==0){
				value = findarg(buf+pos, 1);
				load_cached_model(value);
			}
			else if(stricmp(command, "score")==0){
				value = findarg(buf+pos, 1);
				newchar->score = atoi(value);
			}
			else if(stricmp(command, "secret")==0){
				value = findarg(buf+pos, 1);
				newchar->secret = atoi(value);
			}
			else if(stricmp(command, "speed")==0){
				value = findarg(buf+pos, 1);
				newchar->speed = atoi(value);
				newchar->speed /= 10;
				if(newchar->speed < 0.5) newchar->speed = 0.5;
				if(newchar->speed > 30) newchar->speed = 30;
			}
			else if(stricmp(command, "shadow")==0){
				value = findarg(buf+pos, 1);
				newchar->shadow = atoi(value);
			}
			else if(stricmp(command, "diesound")==0){
				newchar->diesound = loadcache_sound(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "icon")==0){
				value = findarg(buf+pos, 1);
				if(newchar->icon > -1){
					tracefree(buf);
					shutdown("Error: model '%s' has multiple icons defined", filename);
				}
				newchar->icon = loadsprite(value,0,0);
			}
			else if(stricmp(command, "type")==0){
				value = findarg(buf+pos, 1);
				if(stricmp(value, "none")==0){
					newchar->type = TYPE_NONE;
				}
				else if(stricmp(value, "player")==0){
					newchar->type = TYPE_PLAYER;
				}
				else if(stricmp(value, "enemy")==0){
					newchar->type = TYPE_ENEMY;
				}
				else if(stricmp(value, "item")==0){
					newchar->type = TYPE_ITEM;
				}
				else if(stricmp(value, "obstacle")==0){
					newchar->type = TYPE_OBSTACLE;
				}
				else if(stricmp(value, "steamer")==0){
					newchar->type = TYPE_STEAMER;
				}
				else{
					tracefree(buf);
					shutdown("Model '%s' has invalid type: '%s'", filename, value);
				}
			}
			else if(stricmp(command, "subtype")==0){
				value = findarg(buf+pos, 1);
				if(stricmp(value, "biker")==0){
					newchar->subtype = SUBTYPE_BIKER;
				}
				else{
					tracefree(buf);
					shutdown("Model '%s' has invalid subtype: '%s'", filename, value);
				}
			}
			else if(stricmp(command, "remap")==0){
				value = findarg(buf+pos, 1);
				value2 = findarg(buf+pos, 2);
				if(!load_colourmap(newchar, value, value2)){
					tracefree(buf);
					shutdown("Failed to create colourmap from images\n\t'%s'\nand\n\t'%s'.", value, value2);
				}
			}
			else if(stricmp(command, "anim")==0){
				value = findarg(buf+pos, 1);
				// Create new animation
				newanim = alloc_anim();
				if(newanim==NULL){
					tracefree(buf);
					shutdown("Not enough memory for animations!");
				}
				if(stricmp(value, "idle")==0){
					newchar->animation[ANI_IDLE] = newanim;
				}
				else if(stricmp(value, "walk")==0){
					newchar->animation[ANI_WALK] = newanim;
				}
				else if(stricmp(value, "jump")==0){
					newchar->animation[ANI_JUMP] = newanim;
				}
				else if(stricmp(value, "land")==0){
					newchar->animation[ANI_LAND] = newanim;
				}
				else if(stricmp(value, "pain")==0){
					newchar->animation[ANI_PAIN] = newanim;
				}
				else if(stricmp(value, "fall")==0){
					newchar->animation[ANI_FALL] = newanim;
				}
				else if(stricmp(value, "rise")==0){
					newchar->animation[ANI_RISE] = newanim;
				}
				else if(stricmp(value, "attack1")==0){
					newchar->animation[ANI_ATTACK1] = newanim;
				}
				else if(stricmp(value, "attack2")==0){
					newchar->animation[ANI_ATTACK2] = newanim;
				}
				else if(stricmp(value, "attack3")==0){
					newchar->animation[ANI_ATTACK3] = newanim;
				}
				else if(stricmp(value, "upper")==0){
					newchar->animation[ANI_UPPER] = newanim;
				}
				else if(stricmp(value, "slide")==0){
					newchar->animation[ANI_SLIDE] = newanim;
				}
				else if(stricmp(value, "special")==0){
					newchar->animation[ANI_SPECIAL] = newanim;
				}
				else if(stricmp(value, "freespecial")==0){
					newchar->animation[ANI_FREESPECIAL] = newanim;
				}
				else if(stricmp(value, "jumpattack")==0){
					newchar->animation[ANI_JUMPATTACK] = newanim;
				}
				else if(stricmp(value, "jumpattack2")==0){
					newchar->animation[ANI_JUMPATTACK2] = newanim;
				}
				else if(stricmp(value, "get")==0){
					newchar->animation[ANI_GET] = newanim;
				}
				else if(stricmp(value, "grab")==0){
					newchar->animation[ANI_GRAB] = newanim;
				}
				else if(stricmp(value, "grabattack")==0){
					newchar->animation[ANI_GRABATTACK] = newanim;
				}
				else if(stricmp(value, "grabattack2")==0){
					newchar->animation[ANI_GRABATTACK2] = newanim;
				}
				else if(stricmp(value, "throw")==0){
					newchar->animation[ANI_THROW] = newanim;
				}
				else{
					tracefree(buf);
					shutdown("Invalid animation name '%s'", value);
				}

				// Reset vars
				curframe = 0;
				memset(bbox, 0, 4*sizeof(int));
				memset(attack, 0, 4*sizeof(int));
				memset(offset, 0, 2*sizeof(int));
				attackforce = 0;
				move = 0;
				newanim->range[0] = -10;
				newanim->range[1] = 80;
				newanim->throwframe = -1;
				newanim->shootframe = -1;
				newanim->jumpframe = -1;
				newanim->soundtoplay = -1;
			}
			else if(stricmp(command, "loop")==0){
				if(newanim == NULL){
					tracefree(buf);
					shutdown("Can't set loop: no animation specified!");
				}
				value = findarg(buf+pos, 1);
				newanim->loop = atoi(value);
			}
			else if(stricmp(command, "delay")==0){
				value = findarg(buf+pos, 1);
				delay = atoi(value);
			}
			else if(stricmp(command, "offset")==0){
				offset[0] = atoi(findarg(buf+pos, 1));
				offset[1] = atoi(findarg(buf+pos, 2));
			}
			else if(stricmp(command, "throwframe")==0){
				newanim->throwframe = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "shootframe")==0){
				newanim->shootframe = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "jumpframe")==0){
				newanim->jumpframe = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "sound")==0){
				newanim->soundframe = curframe;
				newanim->soundtoplay = loadcache_sound(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "bbox")==0){
				bbox[0] = atoi(findarg(buf+pos, 1));
				bbox[1] = atoi(findarg(buf+pos, 2));
				bbox[2] = atoi(findarg(buf+pos, 3));
				bbox[3] = atoi(findarg(buf+pos, 4));
			}
			else if(stricmp(command, "attack")==0){
				attack[0] = atoi(findarg(buf+pos, 1));
				attack[1] = atoi(findarg(buf+pos, 2));
				attack[2] = atoi(findarg(buf+pos, 3));
				attack[3] = atoi(findarg(buf+pos, 4));
				attackforce = atoi(findarg(buf+pos, 5));
				attackdrop = atoi(findarg(buf+pos, 6));
				attacktype = ATK_NORMAL;
			}
			else if(stricmp(command, "blast")==0){
				attack[0] = atoi(findarg(buf+pos, 1));
				attack[1] = atoi(findarg(buf+pos, 2));
				attack[2] = atoi(findarg(buf+pos, 3));
				attack[3] = atoi(findarg(buf+pos, 4));
				attackforce = atoi(findarg(buf+pos, 5));
				attackdrop = 1;
				attacktype = ATK_BLAST;
			}
			else if(stricmp(command, "burn")==0){
				attack[0] = atoi(findarg(buf+pos, 1));
				attack[1] = atoi(findarg(buf+pos, 2));
				attack[2] = atoi(findarg(buf+pos, 3));
				attack[3] = atoi(findarg(buf+pos, 4));
				attackforce = atoi(findarg(buf+pos, 5));
				attackdrop = 1;
				attacktype = ATK_BURN;
			}
			else if(stricmp(command, "move")==0){
				move = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "range")==0){
				if(newanim==NULL){
					tracefree(buf);
					shutdown("Cannot set range: no animation!");
				}
				newanim->range[0] = atoi(findarg(buf+pos, 1));
				newanim->range[1] = atoi(findarg(buf+pos, 2));
			}
			else if(stricmp(command, "frame")==0){
				if(newanim==NULL){
					tracefree(buf);
					shutdown("Cannot add frame: animation not specified!");
				}
				value = findarg(buf+pos, 1);
				index = loadsprite(value, offset[0], offset[1]);

				// Adjust coords: add offsets and change size to coords
				bbox_con[0] = bbox[0] - offset[0];
				bbox_con[1] = bbox[1] - offset[1];
				bbox_con[2] = bbox[2] + bbox_con[0];
				bbox_con[3] = bbox[3] + bbox_con[1];
				attack_con[0] = attack[0] - offset[0];
				attack_con[1] = attack[1] - offset[1];
				attack_con[2] = attack[2] + attack_con[0];
				attack_con[3] = attack[3] + attack_con[1];

				curframe = addframe(newanim, index, delay, bbox_con, attack_con, attackforce, attackdrop, attacktype, move);
			}
			else{
				tracefree(buf);
				shutdown("Command '%s' not understood in file '%s'!", command, filename);
			}
		}

		// Go to next line
		while(buf[pos] && buf[pos]!='\n' && buf[pos]!='\r') ++pos;
		while(buf[pos]=='\n' || buf[pos]=='\r') ++pos;
	}
	tracefree(buf);

	// If all models are loaded, the cache becomes obsolete
	// if(models_loaded >= models_cached) free_modelcache();
	remove_from_cache(name);
}




// Load / cache all models
int load_models(){
	char * filename = "data/models.txt";

	int handle;
	char *buf;
	unsigned int size;
	int pos;
	
	char * command;
	char value1[128];
	char value2[128];



	// Read file
	
	if((handle=openpackfile(filename,packfile)) < 0) shutdown("Error loading model list from %s", filename);
	size = seekpackfile(handle,0,SEEK_END);
	seekpackfile(handle,0,SEEK_SET);

	buf = (char*)tracemalloc("load_models", size+1);
	if(buf==NULL){
		closepackfile(handle);
		shutdown("Not enough memory to allocate %i-byte buffer for file %s", size, filename);
	}
	if(readpackfile(handle, buf, size) != size){
		tracefree(buf);
		closepackfile(handle);
		shutdown("Read error accessing file %s", filename);
	}
	buf[size] = 0;		// Terminate string (important!)
	closepackfile(handle);



	// Now interpret the contents of buf line by line
	pos = 0;
	while(pos<size){
		command = findarg(buf+pos, 0);
		if(command[0]){
			if(stricmp(command, "load")==0){
				// Add path to cache list
				strncpy(value1, findarg(buf+pos, 1), 127);
				strncpy(value2, findarg(buf+pos, 2), 127);
				cache_model(value1, value2);

				// Now load the cached model
				load_cached_model(value1);
			}
			else if(stricmp(command, "know")==0){
				// Just add path to cache list
				strncpy(value1, findarg(buf+pos, 1), 127);
				strncpy(value2, findarg(buf+pos, 2), 127);
				cache_model(value1, value2);
			}
			else{
				tracefree(buf);
				shutdown("Command '%s' not understood in file '%s'!", command, filename);
			}
		}

		// Go to next line
		while(buf[pos] && buf[pos]!='\n' && buf[pos]!='\r') ++pos;
		while(buf[pos]=='\n' || buf[pos]=='\r') ++pos;
	}


	tracefree(buf);
	return 1;
}





void unload_levelorder(){
	int i, j;
	for(j=0; j<MAX_DIFFICULTIES; j++){
		for(i=0; i<MAX_LEVELS; i++){
			if(levelorder[j][i]) tracefree(levelorder[j][i]);
			levelorder[j][i] = NULL;
		}
		num_levels[j] = 0;
		strcpy(set_names[j], "");
	}
	num_difficulties = 0;
}



// Add a level to the level order
void add_level(char *filename, int diff){
	if(diff > MAX_DIFFICULTIES) return;
	if(num_levels[diff] >= MAX_LEVELS) shutdown("Too many entries in level order (max. %i)!", MAX_LEVELS);

	levelorder[diff][num_levels[diff]] = (s_level_entry*)tracemalloc("add_level", sizeof(s_level_entry));
	if(levelorder[diff][num_levels[diff]] == NULL) shutdown("Out of memory loading level order!");
	memset(levelorder[diff][num_levels[diff]], 0, sizeof(s_level_entry));
	strncpy(levelorder[diff][num_levels[diff]]->filename, filename, 127);
	num_levels[diff]++;
}



// Add a scene to the level order
void add_scene(char *filename, int diff){
	if(diff > MAX_DIFFICULTIES) return;
	if(num_levels[diff] >= MAX_LEVELS) shutdown("Too many entries in level order (max. %i)!", MAX_LEVELS);

	levelorder[diff][num_levels[diff]] = (s_level_entry*)tracemalloc("add_scene", sizeof(s_level_entry));
	if(levelorder[diff][num_levels[diff]] == NULL) shutdown("Out of memory loading level order!");
	memset(levelorder[diff][num_levels[diff]], 0, sizeof(s_level_entry));
	strncpy(levelorder[diff][num_levels[diff]]->filename, filename, 127);
	levelorder[diff][num_levels[diff]]->is_scene = 1;
	num_levels[diff]++;
}



// Load list of levels
void load_levelorder(){
	char * filename = "data/levels.txt";

	int handle;
	char *buf;
	unsigned int size;
	int pos;
	int current_set;
	
	char * command;
	char value1[128];
//	char value2[128];


	unload_levelorder();


	// Read file
	
	if((handle=openpackfile(filename,packfile)) < 0) shutdown("Error loading level list from %s", filename);
	size = seekpackfile(handle,0,SEEK_END);
	seekpackfile(handle,0,SEEK_SET);

	buf = (char*)tracemalloc("load_levelorder", size+1);
	if(buf==NULL){
		closepackfile(handle);
		shutdown("Not enough memory to allocate %i-byte buffer for file %s", size, filename);
	}
	if(readpackfile(handle, buf, size) != size){
		tracefree(buf);
		closepackfile(handle);
		shutdown("Read error accessing file %s", filename);
	}
	buf[size] = 0;		// Terminate string (important!)
	closepackfile(handle);



	// Now interpret the contents of buf line by line
	pos = 0;
	current_set = -1;
	while(pos<size){
		command = findarg(buf+pos, 0);
		if(command[0]){
			if(stricmp(command, "set")==0){
				if(num_difficulties>=MAX_DIFFICULTIES){
					tracefree(buf);
					shutdown("Too many sets of levels (max %u)!", MAX_DIFFICULTIES);
				}
				++num_difficulties;
				++current_set;
				strncpy(set_names[current_set], findarg(buf+pos, 1), MAX_NAME_LEN);
				ifcomplete[current_set] = 0;
			}
			else if(stricmp(command, "ifcomplete")==0){
				if(current_set<0){
					tracefree(buf);
					shutdown("Error in level order: a set must be specified.");
				}
				ifcomplete[current_set] = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "file")==0){
				if(current_set<0){
					tracefree(buf);
					shutdown("Error in level order: a set must be specified.");
				}
				strncpy(value1, findarg(buf+pos, 1), 127);
				add_level(value1, current_set);
			}
			else if(stricmp(command, "scene")==0){
				if(current_set<0){
					tracefree(buf);
					shutdown("Error in level order: a set must be specified.");
				}
				strncpy(value1, findarg(buf+pos, 1), 127);
				add_scene(value1, current_set);
			}
			else if(stricmp(command, "next")==0){
				if(current_set<0){
					tracefree(buf);
					shutdown("Error in level order: a set must be specified.");
				}
				// Set 'gonext' flag of last loaded level
				if(num_levels[current_set]<1){
					tracefree(buf);
					shutdown("Error in level order (next before file)!");
				}
				levelorder[current_set][num_levels[current_set]-1]->gonext = 1;
			}
			else{
				tracefree(buf);
				shutdown("Command '%s' not understood in level order!", command);
			}
		}

		// Go to next line
		while(buf[pos] && buf[pos]!='\n' && buf[pos]!='\r') ++pos;
		while(buf[pos]=='\n' || buf[pos]=='\r') ++pos;
	}
	tracefree(buf);

	if(current_set<0) shutdown("No levels were loaded!");
}








void unload_level(){
	unload_background();
	unload_texture();
	freepanels();
	if(level) tracefree(level);
	level = NULL;

	levelpos = 0;
	advancex = 0;
	advancey = 0;
	advancetime = 0;
	quake = 0;
	quaketime = 0;
	level_waiting = 0;
	current_spawn = 0;
	groupmin = 100;
	groupmax = 100;
	level_completed = 0;
	showtimeover = 0;
	pause = 0;
	endgame = 0;
	go_time = 0;
	neon_time = 0;
	bortime = 0;
}



void load_level(char *filename){
	int handle;
	char * buf;
	int size;
	int pos;
	char * command;
	char * value;
	s_spawn_entry next;
	int i, j;


	unload_level();


	memset(&next, 0, sizeof(s_spawn_entry));
	timeleft = 100 * COUNTER_SPEED;


	level = (s_level*)tracemalloc("load_level", sizeof(s_level));
	if(level==NULL) shutdown("FATAL: Out of memory!");
	memset(level, 0, sizeof(s_level));


	if((handle=openpackfile(filename,packfile)) < 0) shutdown("Unable to load level file '%s'", filename);
	size = seekpackfile(handle,0,SEEK_END);
	seekpackfile(handle,0,SEEK_SET);

	buf = (char*)tracemalloc("load_level", size+1);
	if(buf==NULL){
		closepackfile(handle);
		shutdown("FATAL: out of memory!");
	}
	if(readpackfile(handle, buf, size) != size){
		tracefree(buf);
		closepackfile(handle);
		shutdown("FATAL: read error accessing file '%s'", filename);
	}
	buf[size] = 0;		// Terminate string (important!)
	closepackfile(handle);


	// Now interpret the contents of buf line by line
	pos = 0;
	while(pos<size){
		command = findarg(buf+pos, 0);
		if(command[0]){
			if(stricmp(command, "background")==0){
				load_background(findarg(buf+pos, 1), 1);
				standard_palette();
			}
			else if(stricmp(command, "water")==0){
				load_texture(findarg(buf+pos, 1));
				i = atoi(findarg(buf+pos, 2));
				if(i<2) i = 2;
				texture_set_wave(i);
			}
			else if(stricmp(command, "direction")==0){
				value = findarg(buf+pos, 1);
				if(stricmp(value, "up")==0) level->scrolldir = SCROLL_UP;
				else if(stricmp(value, "down")==0) level->scrolldir = SCROLL_DOWN;
				else if(stricmp(value, "left")==0) level->scrolldir = SCROLL_LEFT;
				else level->scrolldir = SCROLL_RIGHT;
			}
			else if(stricmp(command, "rock")==0){
				level->rocking = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "mirror")==0){
				level->mirror = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "music")==0){
				music(findarg(buf+pos, 1), 1);
			}
			else if(stricmp(command, "bossmusic")==0){
				strncpy(level->bossmusic, findarg(buf+pos, 1), 255);
			}
			else if(stricmp(command, "frontpanel")==0){
				value = findarg(buf+pos, 1);
				if(!loadfrontpanel(value)){
					tracefree(buf);
					shutdown("Unable to load '%s'!", value);
				}
			}
			else if(stricmp(command, "panel")==0){
				if(!loadpanel(findarg(buf+pos, 1), findarg(buf+pos, 2), findarg(buf+pos, 3))){
					tracefree(buf);
					shutdown("Panel load error in '%s'!", filename);
				}
			}
			else if(stricmp(command, "order")==0){
				// Append to order
				if(panels_loaded<1){
					tracefree(buf);
					shutdown("You must load the panels before entering the level layout!");
				}
				value = findarg(buf+pos, 1);
				i = 0;
				while(value[i] && level->numpanels < LEVEL_MAX_PANELS){
					j = value[i];
					if(j>='A' && j<='Z') j-='A';
					else if(j>='a' && j<='z') j-='a';
					else{
						tracefree(buf);
						shutdown("Illegal character in panel order: '%c' (%02Xh)", j, j);
					}
					if(j >= panels_loaded){
						tracefree(buf);
						shutdown("Illegal panel index: %i (only %i panels loaded)", j, panels_loaded);
					}
					level->order[level->numpanels] = j;
					level->numpanels++;
					i++;
				}
			}
			else if(stricmp(command, "hole")==0){
				if(level->numholes >= LEVEL_MAX_HOLES) shutdown("Too many holes in level (max %i)!", LEVEL_MAX_HOLES);
				level->holes[level->numholes] = atoi(findarg(buf+pos, 1));
				level->numholes++;
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
				memset(&next,0,sizeof(s_spawn_entry));
				// Name of entry to be spawned
				strncpy(next.name, findarg(buf+pos, 1), MAX_NAME_LEN);
				// Load model (if not loaded already)
				load_cached_model(next.name);
			}
			else if(stricmp(command, "boss")==0){
				next.boss = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "flip")==0){
				next.flip = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "health")==0){
				next.health = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "alias")==0){
				// Alias (name displayed) of entry to be spawned
				strncpy(next.alias, findarg(buf+pos, 1), MAX_NAME_LEN);
			}
			else if(stricmp(command, "map")==0){
				// Colourmap for new entry
				next.colourmap = atoi(findarg(buf+pos, 1));
			}
			else if(stricmp(command, "item")==0){
				// Item to be contained by new entry
				next.if2p = 0;
				strncpy(next.item, findarg(buf+pos, 1), MAX_NAME_LEN);
				// Load model (if not loaded already)
				load_cached_model(next.item);
			}
			else if(stricmp(command, "2pitem")==0){
				// Item only for 2p game
				next.if2p = 1;
				strncpy(next.item, findarg(buf+pos, 1), MAX_NAME_LEN);
			}
			else if(stricmp(command, "coords")==0){
				next.x = atoi(findarg(buf+pos, 1));
				next.z = atoi(findarg(buf+pos, 2));
				next.a = atoi(findarg(buf+pos, 3));
			}
			else if(stricmp(command, "at")==0){
				// Place entry on queue
				next.at = atoi(findarg(buf+pos, 1));

				if(level->numspawns >= LEVEL_MAX_SPAWNS) shutdown("Level error: too many entries (max. %i)", LEVEL_MAX_SPAWNS);
				memcpy(&level->spawnpoints[level->numspawns], &next, sizeof(s_spawn_entry));
				level->numspawns++;

				// And clear...
				memset(&next,0,sizeof(s_spawn_entry));
			}
			else{
				tracefree(buf);
				shutdown("Command '%s' not understood!", command);
			}
		}

		// Go to next line
		while(buf[pos] && buf[pos]!='\n' && buf[pos]!='\r') ++pos;
		while(buf[pos]=='\n' || buf[pos]=='\r') ++pos;
	}
	tracefree(buf);

	level->width = level->numpanels * panel_width;
	if(level->scrolldir==SCROLL_LEFT) advancex = level->width-320;

	if(level->numpanels < 1) shutdown("Level error: level has no panels");
}






// ---------------------------------- Status --------------------------------

#define		P2_STATS_DIST		180



void drawlifebar(int x, int y, int h, int maxh){
	int overflow = 0;

	if(maxh<=0) return;
	if(h<0) h = 0;
	if(maxh < h) maxh = h;
	if(h > 100){
		overflow = h % 100;
		h = 100;
	}
	if(maxh > 100) maxh = 100;


//	line(x+h+1,    y+1, x+maxh+1, y+1, color_black, vscreen);
//	line(x+h+1,    y+1, x+h+1,    y+6, color_black, vscreen);
	line(x+1,      y+7, x+maxh+2, y+7, color_black, vscreen);
	line(x+maxh+2, y+1, x+maxh+2, y+7, color_black, vscreen);

	line(x,        y,   x+maxh+1, y,   color_white, vscreen);
	line(x,        y,   x,        y+6, color_white, vscreen);
	line(x,        y+6, x+maxh+1, y+6, color_white, vscreen);
	line(x+maxh+1, y,   x+maxh+1, y+6, color_white, vscreen);

	drawbox_trans(x+h+1, y+1, maxh-h, 5, color_red, vscreen, lut_mul);
	drawbox(x+1, y+1, h, 5, color_yellow, vscreen);
	if(overflow) drawbox(x+1, y+1, overflow, 5, color_orange, vscreen);
}





void spawnplayer(int);
void drop_all_enemies();
void kill_all_enemies();




void predrawstatus(){

	int dt;
	int icon;
	int i;
	int xo;
	static int pauselector = 0;
	char t[256];

	for(i=0, xo=0; i<2; i++, xo+=P2_STATS_DIST){
		if(player[i].ent){
			sprintf(t, "%s - %u", player[i].ent->name, player[i].score);
			font_printf(21+xo, savedata.windowpos+2, 0, t);
			icon = player[i].ent->model->icon;
			if(icon>=0) spriteq_add(2+xo,savedata.windowpos+2,10000, sprites[1][icon], SFX_REMAP, player[i].ent->colourmap);

			font_printf(124+xo,savedata.windowpos+9, 0, "x");
			sprintf(t, "%i", player[i].lives);
			font_printf(131+xo,savedata.windowpos+2, 3, t);

			if(player[i].opponent){
				font_printf(21+xo, savedata.windowpos+19, 0, player[i].opponent->name);
				icon = player[i].opponent->model->icon;
				if(icon>=0) spriteq_add(2+xo,savedata.windowpos+19,10000, sprites[1][icon], SFX_REMAP, player[i].opponent->colourmap);
			}
		}
		else if(player[i].joining && player[i].model){
			font_printf(21+xo, savedata.windowpos+2, 0, player[i].model->name);
			font_printf(21+xo, savedata.windowpos+12, 0, "Select hero");
			icon = player[i].model->icon;
			if(icon>=0) spriteq_add(2+xo,savedata.windowpos+2,10000, sprites[1][icon], 0, NULL);

			if(player[i].playkeys & FLAG_ANYBUTTON){
				player[i].playkeys = player[i].newkeys = 0;
				player[i].colourmap = i;
				player[i].lives = 3;
				player[i].joining = 0;
				player[i].spawnhealth = player[i].model->health;
				spawnplayer(i);
				drop_all_enemies();
				timeleft = 100 * COUNTER_SPEED;
			}
			else if(player[i].playkeys & FLAG_MOVELEFT){
				player[i].model = prevplayermodel(player[i].model);
				player[i].playkeys = 0;
			}
			else if(player[i].playkeys & FLAG_MOVERIGHT){
				player[i].model = nextplayermodel(player[i].model);
				player[i].playkeys = 0;
			}
		}
		else if(credits){
			if((bortime/(GAME_SPEED*2)) & 1) { sprintf(t, "Credit %i", credits); font_printf(21+xo, savedata.windowpos+2, 0, t); }
			else font_printf(21+xo, savedata.windowpos+2, 0, "Press start");
			if(player[i].playkeys & FLAG_START){
				player[i].lives = 0;
				player[i].model = nextplayermodel(NULL);
				player[i].joining = 1;
				player[i].playkeys = 0;
				timeleft = 100 * COUNTER_SPEED;
				if (!cheat_mode)
					--credits;
			}
		}
	}

	// if(timeleft < 0) timeleft = 0;
	dt = timeleft/COUNTER_SPEED;
	if(dt>99) dt = 99;
	if(dt<0) dt = 0;
	sprintf(t, "%02i", dt);
	font_printf(151,savedata.windowpos+6, 3, t);
	if(showtimeover) font_printf(113,110, 3, "TIME OVER");
	if(dt<99) showtimeover = 0;

	if(go_time>bortime){
		dt = (go_time-bortime)%GAME_SPEED;
		if(dt < GAME_SPEED/2){
			if(level->scrolldir==SCROLL_LEFT){
				spriteq_add(40,60,10000, sprites[0][gosprite], 0, NULL);
			}
			else{
				spriteq_add(280,60,10000, sprites[1][gosprite], 0, NULL);
			}
		}
	}


	if(pause){
		font_printf(130,100, 3, "pause");
		font_printf(130,120, (pauselector==0), "Continue");
		font_printf(130,132, (pauselector==1), "End game");

		if(bothnewkeys & (FLAG_MOVEUP|FLAG_MOVEDOWN)) pauselector ^= 1;
		if(bothnewkeys & FLAG_ANYBUTTON){
			if(pauselector){
				player[0].lives = player[1].lives = 0;
				endgame = 1;
			}
			pause = 0;
			sound_pause_music(0);
		}
		if(bothnewkeys & FLAG_ESC){
			pause = 0;
			sound_pause_music(0);
		}
	}
	else if(
		(player[0].ent && (player[0].newkeys & FLAG_START)) ||
		(player[1].ent && (player[1].newkeys & FLAG_START)) ||
		(bothnewkeys & FLAG_ESC)){

		pause = 1;
		// sound_pause_music(1);
		pauselector = 0;
	}
}



void drawstatus(){
	// Health bars
	if(player[0].ent){
		drawlifebar(20, savedata.windowpos+10, player[0].ent->oldhealth, player[0].ent->maxhealth);
		if(player[0].opponent) drawlifebar(20, savedata.windowpos+27, player[0].opponent->oldhealth, player[0].opponent->maxhealth);
	}
	if(player[1].ent){
		drawlifebar(20+P2_STATS_DIST, savedata.windowpos+10, player[1].ent->oldhealth, player[1].ent->maxhealth);
		if(player[1].opponent) drawlifebar(20+P2_STATS_DIST, savedata.windowpos+27, player[1].opponent->oldhealth, player[1].opponent->maxhealth);
	}

	// Time box
	line(149, savedata.windowpos+4, 170, savedata.windowpos+4,   color_black, vscreen);
	line(149, savedata.windowpos+4, 149, savedata.windowpos+24,  color_black, vscreen);
	line(170, savedata.windowpos+4, 170, savedata.windowpos+24,  color_black, vscreen);
	line(149, savedata.windowpos+24, 170, savedata.windowpos+24, color_black, vscreen);
	line(148, savedata.windowpos+3, 169, savedata.windowpos+3,   color_white, vscreen);
	line(148, savedata.windowpos+3, 148, savedata.windowpos+23,  color_white, vscreen);
	line(169, savedata.windowpos+3, 169, savedata.windowpos+23,  color_white, vscreen);
	line(148, savedata.windowpos+23, 169, savedata.windowpos+23, color_white, vscreen);
}




void addscore(int playerindex, unsigned gamelib_long add){
	unsigned gamelib_long s = player[playerindex&1].score;
	unsigned gamelib_long next1up;

	playerindex &= 1;

	next1up = ((s/50000)+1) * 50000;

	s += add;
	if(s>999999999) s=999999999;

	while(s>next1up){
		sound_play_sample(smp_1up, 0, savedata.effectvol,savedata.effectvol, 100);
		player[playerindex].lives++;
		next1up += 50000;
	}

	player[playerindex].score = s;
}




// ---------------------------- Object handling ------------------------------




void free_ents(){
	int i;
	for(i=0; i<MAX_ENTS; i++){
		if(ent_list[i]) tracefree(ent_list[i]);
		ent_list[i] = NULL;
	}
}


int alloc_ents(){
	int i;
	for(i=0; i<MAX_ENTS; i++){
		ent_list[i] = (entity*)tracemalloc("alloc_ents", sizeof(entity));
		if(!ent_list[i]){
			free_ents();
			return 0;
		}
		memset(ent_list[i], 0, sizeof(entity));
	}
	return 1;
}




void ent_set_anim(entity *ent, int aninum){
	s_anim *ani;

	if(ent==NULL) shutdown("FATAL: tried to set animation with invalid address (no such object)");
	if(aninum<0 || aninum>=MAX_ANIS) shutdown("FATAL: tried to set animation with invalid index (%i)", aninum);
	ani = ent->model->animation[aninum];
	if(ani==NULL) shutdown("FATAL: tried to set animation with invalid address (%s, %i)", ent->name, aninum);

	if(ent->animation == ani) return;

	ent->animation = ani;
	ent->animpos = 0;
	ent->lastanimpos = -1;
	ent->currentsprite = ani->sprite[0];
	ent->nextanim = bortime + ani->delay[0];
	if(ent->direction) ent->x += ani->move[0];
	else ent->x -= ani->move[0];
	if(ent->animation->soundtoplay>0 && ent->animation->soundframe==0) sound_play_sample(ent->animation->soundtoplay, 0, savedata.effectvol,savedata.effectvol, 100);
	ent->animating = 1;
}

void ent_reset_anim(entity *ent, int aninum){
	s_anim *ani;

	if(ent==NULL) shutdown("FATAL: tried to set animation with invalid address (no such object)");
	if(aninum<0 || aninum>=MAX_ANIS) shutdown("FATAL: tried to set animation with invalid index (%i)", aninum);
	ani = ent->model->animation[aninum];
	if(ani==NULL) shutdown("FATAL: tried to set animation with invalid address (%s, %i)", ent->name, aninum);

	ent->animation = ani;
	ent->animpos = 0;
	ent->currentsprite = ani->sprite[0];
	ent->nextanim = bortime + ani->delay[0];
	if(ent->direction) ent->x += ani->move[0];
	else ent->x -= ani->move[0];
	if(ent->animation->soundtoplay>0 && ent->animation->soundframe==0) sound_play_sample(ent->animation->soundtoplay, 0, savedata.effectvol,savedata.effectvol, 100);
	ent->animating = 1;
}


// 0 = none, 1+ = alternative
void ent_set_colourmap(entity *ent, unsigned int which){
	if(which>MAX_COLOUR_MAPS) which = 0;
	if(which==0) ent->colourmap = NULL;
	else ent->colourmap = ent->model->colourmap[which-1];
}



void ent_set_model(entity * ent, char * modelname){
	s_model *m;

	if(ent==NULL) shutdown("FATAL: tried to change model of invalid object");

	m = find_model(modelname);
	if(m==NULL) shutdown("Model not found: '%s'", modelname);
	ent->model = m;
	ent_set_anim(ent, ANI_IDLE);
	ent_set_colourmap(ent, 0);
}






entity * spawn(float x, float z, float a, char * name){
	entity *e;
	int i;
	s_model * model;

	model = find_model(name);
	if(model==NULL){
		// Be a bit more tolerant...
		return NULL;
		// shutdown("FATAL: attempt to spawn object with invalid model (%s)!", name);
	}

	for(i=0; i<MAX_ENTS; i++){
		if(!ent_list[i]->exists){
			e = ent_list[i];
			memset(e, 0, sizeof(entity));
			e->exists = 1;
			e->model = model;
			e->health = model->health;
			e->maxhealth = model->health;
			e->x = x;
			e->z = z;
			e->a = a;
			e->nextthink = bortime + 1;
			ent_set_anim(e, ANI_IDLE);
			e->type = model->type;
			strncpy(e->name, e->model->name, MAX_NAME_LEN);
			return e;
		}
	}
	return NULL;
}



// Break the link an entity has with another one
void ent_unlink(entity *e){
	if(e->link){
		e->link->link = NULL;
	}
	e->link = NULL;
}


// Link two entities together
void ents_link(entity *e1, entity *e2){
	ent_unlink(e1);
	ent_unlink(e2);
	e1->link = e2;
	e2->link = e1;
}



void borkill(entity *victim){
	if(!victim) return;
	ent_unlink(victim);
	victim->health = 0;
	victim->exists = 0;
	if(victim==player[0].ent) player[0].ent = NULL;
	if(victim==player[0].opponent) player[0].opponent = NULL;
	if(victim==player[1].ent) player[1].ent = NULL;
	if(victim==player[1].opponent) player[1].opponent = NULL;
}



void kill_all(){
	int i;
	for(i=0; i<MAX_ENTS; i++) borkill(ent_list[i]);
	bortime = 0;
}




int checkhit(entity *attacker, entity *target){
	int * coords1;
	int * coords2;
	int x1, x2, y1, y2;
	float medx, medy;
	int debug_coords[2][4];
	int topleast, bottomleast, leftleast, rightleast;

	if(attacker == target) return 0;
	if(diff(attacker->z,target->z) > CONTACT_DIST_V) return 0;
	if(!target->animation->vulnerable[target->animpos]) return 0;

	coords1 = attacker->animation->attack_coords[attacker->animpos];
	coords2 = target->animation->bbox_coords[target->animpos];
	x1 = attacker->x;
	y1 = attacker->z - attacker->a;
	x2 = target->x;
	y2 = target->z - target->a;


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

	medx = (leftleast + rightleast) / 2;
	medy = (topleast + bottomleast) / 2;

	// Now convert these coords to 3D
	lasthitx = medx;
	lasthitz = target->z + 1;
	lasthita = lasthitz - medy;

	return 1;
}




// Check if two attacks overlap (needed for countermoves)
int checkhithit(entity *attacker, entity *target){
	int * coords1;
	int * coords2;
	int x1, x2, y1, y2;
	float medx, medy;
	int debug_coords[2][4];
	int topleast, bottomleast, leftleast, rightleast;

	if(attacker == target) return 0;
	if(diff(attacker->z,target->z) > CONTACT_DIST_V) return 0;
	if(!target->animation->vulnerable[target->animpos]) return 0;

	coords1 = attacker->animation->attack_coords[attacker->animpos];
	coords2 = target->animation->attack_coords[target->animpos];
	x1 = attacker->x;
	y1 = attacker->z - attacker->a;
	x2 = target->x;
	y2 = target->z - target->a;


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

	medx = (leftleast + rightleast) / 2;
	medy = (topleast + bottomleast) / 2;

	// Now convert these coords to 3D
	lasthitx = medx;
	lasthitz = target->z + 1;
	lasthita = lasthitz - medy;

	return 1;
}



int checkhole(int x, int z){
	float tiltx1, tiltx2;
	int i;

	if(level==NULL) return 0;

	if(level->exit_hole){
		if(x > level->width-(PLAYER_MAX_Z-z)) return 2;
	}

	if(z < 195) return 0;

	tiltx1 = x + (z-195)/4;
	tiltx2 = x - (z-195)*2;

	for(i=0; i<level->numholes; i++){
		if(tiltx1>=level->holes[i]+11 && tiltx2<=level->holes[i]+200) return 1;
	}

	return 0;
}



void do_attack(entity *e){
	int them;
	int i;
	int force;
	entity *temp;
	int didhit = 0;
	int current_attack_id;
	static unsigned int new_attack_id = 1;

	// Can't get hit after this
	if(level_completed) return;

	if(e->type==TYPE_PLAYER || e->projectile) them = TYPE_PLAYER | TYPE_ENEMY | TYPE_OBSTACLE;
	else if(e->type == TYPE_ENEMY) them = TYPE_PLAYER;
	else return;


	// Every attack gets a unique ID to make sure no one
	// gets hit more than once by the same attack
	current_attack_id = e->attack_id;
	if(!current_attack_id){
		++new_attack_id;
		if(new_attack_id==0) new_attack_id = 1;
		e->attack_id = current_attack_id = new_attack_id;
	}


	force = e->animation->attack_force[e->animpos];

	for(i=0; i<MAX_ENTS; i++){
		if(ent_list[i]->exists &&
				(ent_list[i]->type & them) &&
				ent_list[i]->pain_time<bortime &&
				ent_list[i]->takedamage &&
				ent_list[i]->hit_by_attack_id != current_attack_id &&
				checkhit(e, ent_list[i])){

			temp = self;
			self = ent_list[i];
			self->takedamage(e, force, e->animation->attack_drop[e->animpos], e->animation->attack_type[e->animpos]);
			self = temp;

			ent_list[i]->hit_by_attack_id = current_attack_id;


			// Spawn a flash
			temp = spawn(lasthitx,lasthitz,lasthita,"flash");
			if(temp){
				temp->base = lasthita;
				temp->autokill = 1;
			}

			if(e->type==TYPE_PLAYER) e->combotime = bortime + (GAME_SPEED/2);

			didhit = 1;
		}
	}


	// Special case: uppercut.
	// With the uppercut, an enemy can hit a player
	// through their attacks... like a counter.
	if(e->animation == e->model->animation[ANI_UPPER]){
		for(i=0; i<MAX_ENTS; i++){
			if(ent_list[i]->exists &&
					(ent_list[i]->type & them) &&
					ent_list[i]->pain_time<bortime &&
					ent_list[i]->takedamage &&
					checkhithit(e, ent_list[i])){

				temp = self;
				self = ent_list[i];
				self->takedamage(e, force, 1, e->animation->attack_type[e->animpos]);
				self = temp;

				// Spawn a flash
				temp = spawn(lasthitx,lasthitz,lasthita,"flash");
				if(temp){
					temp->base = lasthita;
					temp->autokill = 1;
				}

				if(e->type==TYPE_PLAYER) e->combotime = bortime + (GAME_SPEED/2);

				didhit = 1;
			}
		}
	}

	if(didhit){
		// Play a sound
		if(e->projectile) sound_play_sample(smp_indirect, 0, savedata.effectvol,savedata.effectvol, 100);
		else sound_play_sample(smp_beat, 0, savedata.effectvol,savedata.effectvol, 105 - force);

		if(e->remove_on_attack) borkill(e);
	}
}




// Update all entities that wish to think or animate in this cycle.
// All loops are separated because "self" might die during a pass.
void update_ents(){
	int f;
	int i;
	int holetype;


	if(level){
		// Lost ents
		for(i=0; i<MAX_ENTS; i++){
			if(ent_list[i]->exists){
				self = ent_list[i];
				if(self->x < advancex-1000 || self->a<PIT_DEPTH*2) borkill(self);
			}
		}

		// Gravity
		for(i=0; i<MAX_ENTS; i++){
			if(ent_list[i]->exists){
				self = ent_list[i];

				if((self->tossv>0 || inair(self)) && self->toss_time <= bortime){
					self->a += self->tossv;
					if(self->tossv > -6) self->tossv -= 0.1;
					if(self->a <= self->base){
						self->a = self->base;
					}
					if(self->a < PIT_DEPTH){
						if(!self->takedamage) borkill(self);
						else self->takedamage(self,10000,0,ATK_NORMAL);
						continue;
					}
					self->toss_time = bortime + (GAME_SPEED/100);
				}
				// Check for holes
				if(self->base==0 && !inair(self) && (holetype=checkhole(self->x, self->z))){
					self->base = -1000;
					ent_unlink(self);	// Release grabs
					if(holetype==2){
						// place behind panels
						self->a -= self->z - (PANEL_Z-1);
						self->z = PANEL_Z-1;
					}
				}
			}
		}
	}


	// A.I.
	for(i=0; i<MAX_ENTS; i++){
		if(ent_list[i]->exists){
			self = ent_list[i];
			if(self->nextthink == bortime){
				if(self->think){
					self->nextthink = bortime + THINK_SPEED;
					self->think();
				}
			}
		}
	}


	
	// Animation
	for(i=0; i<MAX_ENTS; i++){
		if(ent_list[i]->exists){
			self = ent_list[i];

			if(self->nextanim == bortime){
				f = self->animpos + self->animating;
				if((unsigned)f >= (unsigned)self->animation->numframes){
					if(f<0) f = self->animation->numframes-1;
					else f = 0;
					if(!self->animation->loop){
						self->animating = 0;
						if(self->autokill){
							borkill(self);
							continue;
						}
					}
				}
				if(self->animating){
					self->nextanim = bortime + (self->animation->delay[f]);
					self->currentsprite = self->animation->sprite[f];
					self->animpos = f;
					if(self->direction) self->x += self->animation->move[f];
					else self->x -= self->animation->move[f];
					if(self->animation->soundtoplay>0 && self->animation->soundframe==f) sound_play_sample(self->animation->soundtoplay, 0, savedata.effectvol,savedata.effectvol, 100);
				}
			}
		}
	}


	// Collission detection (for a part of the entities)
	for(i=(bortime&1); i<MAX_ENTS; i+=2){
		if(ent_list[i]->exists){
			self = ent_list[i];

			// Attack stuff
			f = self->animpos;
			if((self->attacking || self->projectile) && self->animation->attack_force[f]){
				do_attack(self);
			}
			else self->attack_id = 0;
		}
	}


	// Update displayed health
	for(i=0; i<MAX_ENTS; i++){
		if(ent_list[i]->exists){
			self = ent_list[i];

			if(self->oldhealth < self->health) self->oldhealth++;
			else if(self->oldhealth > self->health) self->oldhealth--;
		}
	}
}



void display_ents(){
	unsigned f;
	int i;
	entity *e;
	int use_mirror = (level && level->mirror);

	for(i=0; i<MAX_ENTS; i++){
		if(ent_list[i]->exists && !(ent_list[i]->blink && (bortime%(GAME_SPEED/10))<(GAME_SPEED/20))){
			e = ent_list[i];
			f = e->currentsprite;
			if(f<sprites_loaded){
				if(e->colourmap) spriteq_add(e->x - advancex, e->z-e->a + gfx_y_offset, e->z, sprites[e->direction&1][f], SFX_REMAP, e->colourmap);
				else if(e->screen) spriteq_add(e->x - advancex, e->z-e->a + gfx_y_offset, e->z, sprites[e->direction&1][f], SFX_BLEND, lut_screen);
				else spriteq_add(e->x - advancex, e->z-e->a + gfx_y_offset, e->z, sprites[e->direction&1][f], 0, NULL);

				if(use_mirror){
					if(e->colourmap) spriteq_add(e->x-advancex, (PLAYER_MIN_Z-10)+(PLAYER_MIN_Z - e->z)-e->a+gfx_y_offset, (PANEL_Z - e->z) + PANEL_Z-10, sprites[e->direction&1][f], SFX_REMAP, e->colourmap);
					else spriteq_add(e->x-advancex, (PLAYER_MIN_Z-10)+(PLAYER_MIN_Z - e->z)-e->a+gfx_y_offset, (PANEL_Z - e->z) + (PANEL_Z-10), sprites[e->direction&1][f], 0, NULL);
				}
			}
			if(e->model->shadow && e->a>=0 && !checkhole(e->x, e->z)) spriteq_add(e->x - advancex, e->z + gfx_y_offset, SHADOW_Z, sprites[e->direction&1][shadowsprites[e->model->shadow-1]], SFX_BLEND, lut_mul);
		}
	}
}











void toss(entity *ent, float lift){
	ent->toss_time = bortime + 1;
	ent->tossv = lift;
	ent->a += 0.5;		// Get some altitude (needed for checks)
}




entity * findent(int types){
	int i;
	for(i=0; i<MAX_ENTS; i++){
		if(ent_list[i]->exists && (ent_list[i]->type & types)){
			return ent_list[i];
		}
	}
	return NULL;
}



int count_ents(int types){
	int i;
	int count = 0;
	for(i=0; i<MAX_ENTS; i++){
		if(ent_list[i]->exists && (ent_list[i]->type & types)){
			++count;
		}
	}
	return count;
}






entity * find_ent_here(entity *exclude, float x, float z, int types){
	int i;
	for(i=0; i<MAX_ENTS; i++){
		if(		ent_list[i]->exists
				&& ent_list[i] != exclude
				&& (ent_list[i]->type & types)
				&& diff(ent_list[i]->x,x)<CONTACT_DIST_H
				&& diff(ent_list[i]->z,z)<CONTACT_DIST_V
				&& ent_list[i]->animation->vulnerable[ent_list[i]->animpos]
				){
			return ent_list[i];
		}
	}
	return NULL;
}




void make_quake(int amount){
	if(amount>4) amount = 4;
	if(quake < amount) quake = amount;
}


// ---------------------------------- A.I. ------------------------------------

void suicide(void);

void player_think(void);
void player_jump(void);
void player_land(void);
void player_grab(void);
void player_grabattack(void);
void player_grabbed(void);
void player_throw(void);
void player_pain(void);
void player_fall(void);
void player_rise(void);
void player_blink(void);
void dojump(int speedmul);

void enemy_drop(void);		// Enter field
void enemy_think(void);
void enemy_prepare(void);
void enemy_attack(void);
void enemy_runoff(void);
void enemy_pain(void);
void enemy_rise(void);
void enemy_lie(void);
void enemy_fall(void);
void enemy_grabbed(void);
void enemy_throw(void);
void enemy_grab(void);
void enemy_jumpattack(void);



void suicide(void){
	level_completed |= self->boss;
	borkill(self);
}





// Re-enter playfield
// Used by player_fall and player_takedamage
void player_die(){
	--player[self->playerindex].lives;

	if(player[self->playerindex].lives <= 0){
		borkill(self);
		player[self->playerindex].ent = NULL;
		if(!player[0].ent && !player[1].ent){
			timeleft = 10 * COUNTER_SPEED;
			if(credits<1) timeleft = COUNTER_SPEED/2;
		}
		return;
	}

	ent_unlink(self);
	self->blink = 0;
	if(level->scrolldir == SCROLL_LEFT){
		self->x = advancex + 290;
		self->direction = 0;
	}
	else{
		self->x = advancex + 20;
		self->direction = 1;
	}
	self->z = ((PLAYER_MIN_Z+PLAYER_MAX_Z)/2) - 10;
	self->a = 300;
	self->base = 0;
	self->jumping = 1;
	self->attacking = 0;
	self->projectile = 0;
	self->health = self->maxhealth;
	ent_set_anim(self, ANI_JUMP);
	self->think = player_jump;
	drop_all_enemies();
	timeleft = 100 * COUNTER_SPEED;
}



int player_trymove(float xdir, float zdir){
	entity *other;

	if(self->health<=0 || self->base<0){
		self->x += xdir;
		self->z += zdir;
		return 1;
	}


	// Don't move through obstacles
	if(xdir && (other = find_ent_here(self, self->x + xdir*3, self->z, TYPE_OBSTACLE))){
		if(xdir>0 ? other->x>self->x : other->x<self->x) xdir = 0;
	}
	if(zdir && (other = find_ent_here(self, self->x, self->z + zdir*3, TYPE_OBSTACLE))){
		if(zdir>0 ? other->z>self->z : other->z<self->z) zdir = 0;
	}

/*
	// Player jumping / flying?
	if(inair(self) || self->jumping){
		if(!self->projectile && find_ent_here(self, self->x + xdir, self->z, TYPE_OBSTACLE)){
			return 0;
		}
	}
	else if(other = find_ent_here(self, self->x + xdir, self->z + zdir, (TYPE_ENEMY | TYPE_PLAYER))){
*/
	// Can't grab anyone while (they're) in mid-air...
	// And we can't grab anyone behind us???
	//  && (self->direction ? other->x>self->x : other->x<self->x)

	if(!(inair(self) || self->jumping) && 
			(other = find_ent_here(self, self->x + xdir, self->z + zdir, (TYPE_ENEMY | TYPE_PLAYER))) &&
			!inair(other) && !other->nograb
			){


		// Reposition both on average height and such
		zdir = (self->z + other->z) / 2;
		other->z = zdir;
		self->z = zdir+1;	// Grabber up front

		xdir = (self->x + other->x) / 2;
		if(self->x < other->x){
			self->x = xdir - (GRAB_DIST/2);
			other->x = xdir + (GRAB_DIST/2);
			self->direction = 1;
		}
		else{
			self->x = xdir + (GRAB_DIST/2);
			other->x = xdir - (GRAB_DIST/2);
			self->direction = 0;
		}

		ents_link(self, other);
		player[self->playerindex].opponent = other;
		other->attacking = 0;

		self->combostep = 0;
		self->think = player_grab;

		if(other->type==TYPE_ENEMY){
			other->think = enemy_grabbed;
			other->playerindex = self->playerindex;
			other->stalltime = bortime + GRAB_STALL;
		}
		if(other->type==TYPE_PLAYER){
			other->think = player_grabbed;
			player[other->playerindex].opponent = self;
		}
		ent_set_anim(other, ANI_PAIN);
		ent_set_anim(self, ANI_GRAB);

		return 0;
	}

	self->x += xdir;
	self->z += zdir;

	if(self->z < PLAYER_MIN_Z) self->z = PLAYER_MIN_Z;
	if(self->z > PLAYER_MAX_Z) self->z = PLAYER_MAX_Z;
	if(self->x < advancex+10) self->x = advancex+10;
	if(self->x > advancex+310) self->x = advancex+310;


	// End of level is blocked?
	if(level->exit_blocked){
		if(self->x > level->width-30-(PLAYER_MAX_Z-self->z)) self->x = level->width-30-(PLAYER_MAX_Z-self->z);
	}

	return 1;
}



// Check keys for special move. Used several times, so I func'd it.
int player_check_special(){
	if((player[self->playerindex].playkeys & FLAG_SPECIAL)
		&& self->health>6
		&& self->model->animation[ANI_SPECIAL]){

		player[self->playerindex].playkeys -= FLAG_SPECIAL;
		ent_unlink(self);
		self->health -= 6;
		self->attacking = 1;
		self->combostep = 0;
		self->think = player_think;
		ent_set_anim(self, ANI_SPECIAL);
		return 1;
	}
	return 0;
}




void player_throw(void){
	if(self->animating) return;
	self->think = player_think;
	ent_set_anim(self, ANI_IDLE);
}



void player_pain(void){
	if(player_check_special()) return;

	if(self->animating) return;

	if(self->link){
		self->think = player_grabbed;
		ent_set_anim(self, ANI_PAIN);
	}
	else{
		self->think = player_think;
		ent_set_anim(self, ANI_IDLE);
	}
}


void player_land(void){
	if(self->animating) return;
	self->think = player_think;
	ent_set_anim(self, ANI_IDLE);
}


void player_fall(void){

	// Still falling?
	if(inair(self)){
		if(self->projectile){
			if(self->direction) player_trymove(-2.5, 0);
			else player_trymove(2.5, 0);
		}
		else{
			if(self->direction) player_trymove(-1, 0);
			else player_trymove(1, 0);
		}
		return;
	}

	if(self->projectile){
		self->projectile = 0;

		if((player[self->playerindex].keys & (FLAG_MOVEUP|FLAG_JUMP)) == (FLAG_MOVEUP|FLAG_JUMP)){
			player[self->playerindex].playkeys ^= (FLAG_MOVEUP|FLAG_JUMP);
			if(self->model->animation[ANI_LAND]){
				ent_set_anim(self, ANI_LAND);
				self->think = player_land;
				self->direction = !self->direction;
			}
			else{
				ent_set_anim(self, ANI_IDLE);
				self->think = player_think;
			}
			return;
		}

		self->health -= self->damage_on_landing;
		self->damage_on_landing = 0;
		make_quake(4);
	}

	// Hard landing? Quake and bounce!
	if(self->tossv < -2){
		make_quake(2);
		sound_play_sample(smp_fall, 0, savedata.effectvol,savedata.effectvol, 100);
		toss(self, 1);
		return;
	}

	if(self->health <= 0){
		self->think = player_blink;
		self->blink = 1;
		self->stalltime = bortime + GAME_SPEED * 2;
		return;
	}

	self->think = player_rise;
	ent_set_anim(self, ANI_RISE);
}



void player_blink(void){
	if(bortime >= self->stalltime) player_die();
}


void player_rise(void){
	if(self->animating) return;
	self->pain_time = bortime + (GAME_SPEED/5);
	self->think = player_think;
	ent_set_anim(self, ANI_IDLE);

// TODO: WAS THIS A BUG? it had no ()
	player_think();
}



void player_grabbed(void){
	if(player_check_special()) return;

	// Just check if we're still grabbed...
	if(self->link) return;

	ent_set_anim(self, ANI_IDLE);
	self->think = player_think;
}




void player_grab(void){
//	entity * temp;
	entity * other = self->link;

	if(player_check_special()) return;

	if(other == NULL){
		self->think = player_think;
		return;
	}

	if(self->direction ?
	   (player[self->playerindex].keys & FLAG_MOVELEFT) :
	   (player[self->playerindex].keys & FLAG_MOVERIGHT)){

		if(bortime > self->releasetime){
			// Release
			ent_unlink(self);
			self->think = player_think;
		}
	}
	else self->releasetime = bortime + (GAME_SPEED/2);

	if((player[self->playerindex].playkeys & FLAG_ATTACK) &&
	   (self->direction ?
	   (player[self->playerindex].keys & FLAG_MOVELEFT) :
	   (player[self->playerindex].keys & FLAG_MOVERIGHT))){

		player[self->playerindex].playkeys -= FLAG_ATTACK;
		toss(other, 4);
		other->direction = self->direction;
		other->projectile = 1;
		if(other->type == TYPE_ENEMY){
			other->think = enemy_fall;
			other->damage_on_landing = THROW_DAMAGE;
		}
		if(other->type == TYPE_PLAYER){
			other->think = player_fall;
			other->damage_on_landing = THROW_DAMAGE / 2;
		}
		ent_set_anim(other, ANI_FALL);
		ent_unlink(self);
		ent_set_anim(self, ANI_THROW);
		self->think = player_throw;
	}
	else if((player[self->playerindex].playkeys & FLAG_ATTACK) &&
			self->model->animation[ANI_GRABATTACK]){

		player[self->playerindex].playkeys -= FLAG_ATTACK;
		self->attacking = 1;

		++self->combostep;
		if(self->combostep < 3){
			ent_set_anim(self, ANI_GRABATTACK);
			self->think = player_grabattack;
		}
		else{
			if(self->model->animation[ANI_GRABATTACK2]) ent_set_anim(self, ANI_GRABATTACK2);
			else ent_set_anim(self, ANI_ATTACK3);
			self->think = player_think;
			ent_unlink(self);
			self->combostep = 0;
		}
	}
	else if(player[self->playerindex].playkeys & (FLAG_JUMP|FLAG_ATTACK)){
		player[self->playerindex].playkeys -= player[self->playerindex].playkeys&(FLAG_JUMP|FLAG_ATTACK);
		self->attacking = 1;
		self->think = player_think;
		ent_unlink(self);
		self->combostep = 0;

		// Perform final blow
		if(self->model->animation[ANI_GRABATTACK2]) ent_set_anim(self, ANI_GRABATTACK2);
		else if(self->model->animation[ANI_ATTACK3]) ent_set_anim(self, ANI_ATTACK3);
		else{
			dojump(1);
			return;
		}
	}
}



void player_grabattack(void){
	if(self->animating) return;
	self->attacking = 0;
	if(self->link){
		ent_set_anim(self, ANI_GRAB);
		self->think = player_grab;
	}
	else{
		ent_set_anim(self, ANI_IDLE);
		self->think = player_think;
	}
}


void player_jump(){
	player_trymove(self->xdir, 0);
	if(player[self->playerindex].playkeys & FLAG_ATTACK){
		player[self->playerindex].playkeys -= FLAG_ATTACK;
		self->attacking = 1;
		if((player[self->playerindex].playkeys & FLAG_MOVEDOWN) && self->model->animation[ANI_JUMPATTACK2]) ent_set_anim(self, ANI_JUMPATTACK2);
		else if(self->model->animation[ANI_JUMPATTACK]) ent_set_anim(self, ANI_JUMPATTACK);
	}
	if(!inair(self)){
		self->tossv = 0;
		self->a = self->base;
		self->jumping = 0;
		self->attacking = 0;
		ent_set_anim(self, ANI_IDLE);
		self->think = player_think;
		self->stalltime = 0;
	}
}




void dojump(int speedmul){
	sound_play_sample(smp_jump, 0, savedata.effectvol,savedata.effectvol, 100);

	self->jumping = 1;
	self->attacking = 1;	// This is needed for Mighty
	ent_set_anim(self, ANI_JUMP);

	toss(self, 4);
	if(player[self->playerindex].keys & FLAG_MOVELEFT) self->xdir = -1 * speedmul;
	else if(player[self->playerindex].keys & FLAG_MOVERIGHT) self->xdir = 1 * speedmul;
	else self->xdir = 0;

	self->think = player_jump;
}


void player_think(void){
	int walking = 0;
	entity *other;

	if(endgame) return;

	// Check special!
	if(player[self->playerindex].playkeys & FLAG_MOVELEFT){
		player[self->playerindex].playkeys -= FLAG_MOVELEFT;
		if(bortime < self->movetime && self->lastmove==FLAG_MOVELEFT) ++self->movestep;
		else self->movestep = 0;
		self->lastmove = FLAG_MOVELEFT;
		self->movetime = bortime + (GAME_SPEED/4);
	}
	else if(player[self->playerindex].playkeys & FLAG_MOVERIGHT){
		player[self->playerindex].playkeys -= FLAG_MOVERIGHT;
		if(bortime < self->movetime && self->lastmove==FLAG_MOVERIGHT) ++self->movestep;
		else self->movestep = 0;
		self->lastmove = FLAG_MOVERIGHT;
		self->movetime = bortime + (GAME_SPEED/4);
	}


	// Jumpframe in animation?
	if(self->animpos != self->lastanimpos && self->animpos == self->animation->jumpframe){
		toss(self, 2);
	}
	self->lastanimpos = self->animpos;


	if(self->attacking){
		if(inair(self)){
			if(self->direction) player_trymove(1, 0);
			else player_trymove(-1, 0);
			self->nextthink = bortime + (THINK_SPEED/2);
		}
// TODO: was this a bug?
		if((self->attacking = self->animating)) return;
		ent_set_anim(self, ANI_IDLE);
	}
	if(self->getting){
// TODO: was this a bug?
		if((self->getting = self->animating)) return;
		ent_set_anim(self, ANI_IDLE);
	}


	if(player_check_special()) return;

	if(!(player[self->playerindex].keys & FLAG_ATTACK)){
		if(self->stalltime
		   && self->stalltime+(GAME_SPEED*2) < bortime
		   && self->model->animation[ANI_ATTACK3]){
			self->attacking = 1;
			self->combostep = 0;
			ent_set_anim(self, ANI_ATTACK3);
			sound_play_sample(smp_punch, 0, savedata.effectvol,savedata.effectvol, 100);
			self->stalltime = 0;
			return;
		}
		self->stalltime = 0;
	}
	if(player[self->playerindex].playkeys & FLAG_ATTACK){

		player[self->playerindex].playkeys -= FLAG_ATTACK;

		// Use stalltime to charge end-move
		self->stalltime = bortime;

		// Perform special move, -> -> P
		if(bortime < self->movetime && self->movestep>=1){
			if(!self->model->animation[ANI_FREESPECIAL]){
				// This is for Mighty
				self->combostep = 0;
				dojump(2);
				return;
			}
			player[self->playerindex].playkeys -= FLAG_SPECIAL;
			self->attacking = 1;
			self->combostep = 0;
			ent_set_anim(self, ANI_FREESPECIAL);
			return;
		}


		// Quick turnaround
		if(player[self->playerindex].keys & FLAG_MOVELEFT){
			self->direction = 0;
		}
		else if(player[self->playerindex].keys & FLAG_MOVERIGHT){
			self->direction = 1;
		}

// TODO: was this a bug? probably not.
		if((other = find_ent_here(self, self->x, self->z, TYPE_ITEM))){
			// Pick up item
			player[self->playerindex].opponent = other;
			if(other->model->score){
				addscore(self->playerindex, other->model->score);
				sound_play_sample(smp_get2, 0, savedata.effectvol,savedata.effectvol, 100);
			}
			else if(other->model->health){
				self->health += other->model->health;
				if(self->health > self->model->health) self->health = self->model->health;
				other->health = 0;
				sound_play_sample(smp_get, 0, savedata.effectvol,savedata.effectvol, 100);
			}
			else if(stricmp(other->model->name, "Time")==0){
				timeleft = 100 * COUNTER_SPEED;
				sound_play_sample(smp_get2, 0, savedata.effectvol,savedata.effectvol, 100);
			}
			else{
				// Must be a 1up then.
				player[self->playerindex].lives++;
				sound_play_sample(smp_1up, 0, savedata.effectvol,savedata.effectvol, 100);
			}
			other->think = suicide;
			other->nextthink = bortime + GAME_SPEED * 3;
			other->z = 100000;
			self->getting = 1;
			ent_set_anim(self, ANI_GET);
			return;
		}
		self->attacking = 1;
		if(self->combotime > bortime) self->combostep++;
		else self->combostep = 0;

		if(!self->model->animation[ANI_ATTACK1]){
			// This is for Mighty
			self->combostep = 0;
			dojump(1);
			return;
		}

		if(self->combostep < 2){
			ent_set_anim(self, ANI_ATTACK1);
		}
		else if(self->combostep == 2){
			ent_set_anim(self, ANI_ATTACK2);
		}
		else{
			ent_set_anim(self, ANI_ATTACK3);
			self->combostep = 0;
		}
		sound_play_sample(smp_punch, 0, savedata.effectvol,savedata.effectvol, 100);
		return;
	}

	// Mighty hass no attack animations, he just jumps.
	if(player[self->playerindex].playkeys & FLAG_JUMP){

		player[self->playerindex].playkeys -= FLAG_JUMP;
		dojump(1);
		return;
	}

	if(player[self->playerindex].keys & FLAG_MOVEUP){
		walking = player_trymove(0, -self->model->speed/2);
	}
	else if(player[self->playerindex].keys & FLAG_MOVEDOWN){
		walking = player_trymove(0, self->model->speed/2);
	}

	if(self->link){
		return;		// Grabbed someone
	}

	if(player[self->playerindex].keys & FLAG_MOVELEFT){
		self->direction = 0;
		walking = player_trymove(-self->model->speed, 0);
	}
	else if(player[self->playerindex].keys & FLAG_MOVERIGHT){
		self->direction = 1;
		walking = player_trymove(self->model->speed, 0);
	}


	if(self->link) return;		// Grabbed someone

	if(walking) ent_set_anim(self, ANI_WALK);
	else ent_set_anim(self, ANI_IDLE);
}




void player_takedamage(entity *other, int force, int drop, int type){

	self->attacking = 0;
	self->jumping = 0;

	if(self->a < PIT_DEPTH){
		player_die();
		return;
	}

	if((other!=self->link) || drop) ent_unlink(self);
	if(other != self) player[self->playerindex].opponent = other;

	self->pain_time = bortime + (GAME_SPEED / 5);
	self->direction = (self->x < other->x);
	self->xdir = -self->xdir;

	if(other->type==TYPE_PLAYER) force /= 2;
	self->health -= force;

	if(type == ATK_BLAST){
		self->projectile = 1;
		drop = 1;
	}

	if(drop || self->health<=0 || inair(self)){
		toss(self, 3);
		ent_reset_anim(self, ANI_FALL);
		self->damage_on_landing = 0;
		self->think = player_fall;
		return;
	}

	ent_reset_anim(self, ANI_PAIN);
	self->think = player_pain;
}






////////////////////////////////



// Called when player re-enters the game.
// Drop all enemies EXCEPT for the linked ones.
void drop_all_enemies(){
	int i;
	for(i=0; i<MAX_ENTS; i++){
		if(ent_list[i]->exists &&
			ent_list[i]->health>0 &&
			ent_list[i]->type==TYPE_ENEMY &&
			!ent_list[i]->link &&
			ent_list[i]->model->animation[ANI_FALL]
		){
			ent_list[i]->attacking = 0;
			ent_list[i]->projectile = 0;
			ent_list[i]->think = enemy_fall;
			ent_list[i]->damage_on_landing = 0;
			toss(ent_list[i], 2.5 + randf(1));
			ent_reset_anim(ent_list[i], ANI_FALL);
		}
	}
}



// Called when boss dies
void kill_all_enemies(){
	int i;
	entity * tmpself;

	tmpself = self;
	for(i=0; i<MAX_ENTS; i++){
		if(		ent_list[i]->exists
				&& ent_list[i]->health>0
				&& ent_list[i]->type==TYPE_ENEMY
				&& ent_list[i]->takedamage){
			self = ent_list[i];
			self->takedamage(tmpself, 1000000, 1, ATK_NORMAL);
		}
	}
	self = tmpself;
}



////////////////////////////////



void anything_walk(void){
	if(self->x < advancex - 80 || self->x > advancex + 400){
		borkill(self);
		return;
	}
	self->x += self->xdir;
}




void knife_think(void){
	if(self->x < advancex - 80 || self->x > advancex + 400){
		borkill(self);
		return;
	}
	if(self->direction) self->x += 2;
	else self->x -= 2;
	self->nextthink = bortime + 1;
}


void knife_spawn(float x, float z, int direction){
	entity *e;

	e = spawn(x, z, 70, "Knife");
	if(e==NULL) return;

	e->attacking = 1;
	e->direction = direction;
	e->think = knife_think;
	e->remove_on_attack = 1;
	e->base = 70;
	e->a = 70;
}



void fireball_think(void){
	if(self->x < advancex - 180 || self->x > advancex + 500){
		borkill(self);
		return;
	}
	if(self->direction) self->x += 2;
	else self->x -= 2;
	self->nextthink = bortime + 1;
}


void fireball_spawn(float x, float z, int direction){
	entity *e;

	e = spawn(x, z, 0, "Shot");
	if(e==NULL) return;

	e->attacking = 1;
	e->direction = direction;
	e->think = fireball_think;
	e->remove_on_attack = 1;
}



void star_think(void){
	if(self->x<advancex-80 || self->x>advancex+400 || self->base<0){
		borkill(self);
		return;
	}
	self->x += self->xdir;
	self->base -= 2;
	self->a = self->base;
	self->nextthink = bortime + 1;
}


// Spawn 3 stars
void star_spawn(float x, float z, float a, int direction){
	entity *e;
	float fd = (direction ? 2 : -2);

	e = spawn(x, z, 70, "Star");
	if(e==NULL) return;
	e->attacking = 1;
	e->xdir = fd/2;
	e->think = star_think;
	e->remove_on_attack = 1;
	e->base = a;
	e->a = a;

	e = spawn(x, z, 70, "Star");
	if(e==NULL) return;
	e->attacking = 1;
	e->xdir = fd;
	e->think = star_think;
	e->remove_on_attack = 1;
	e->base = a;
	e->a = a;

	e = spawn(x, z, 70, "Star");
	if(e==NULL) return;
	e->attacking = 1;
	e->xdir = fd * 1.5;
	e->think = star_think;
	e->remove_on_attack = 1;
	e->base = a;
	e->a = a;
}




void steam_think(void){
	if(!self->animating){
		borkill(self);
		return;
	}
	self->base += 1;
	self->a = self->base;
}


void steam_spawn(float x, float z, float a){
	entity *e;

	e = spawn(x, z, a, "Steam");
	if(e==NULL) return;

	e->base = a;
	e->think = steam_think;
	e->screen = 1;
}


void steamer_think(void){
	if(self->x < advancex-80 || self->x > advancex+400){
		borkill(self);
		return;
	}
	steam_spawn(self->x, self->z, self->a);
	self->nextthink = bortime + (GAME_SPEED/10) + (rand32()&31);
}



////////////////////////////////



/*

A brief description of enemy behaviour:

The enemy moves about quite randomly.
If a player is located on the same level (close-to-same Z coordinate),
the enemy will be inclined to move forward and backward only
(still facing the player!).
All of the movement patterns are subject to some randomness.

If, at any time, a player is spotted at a proper location for attack
(e.g. right in front of the enemy), the enemy waits for a half a second
or so, and attacks.

If the player is in mid-air, an uppercut attack should be preferred.

*/


entity * enemy_find_target(){
	float dx1, dz1;
	float dx2, dz2;


	// One or less players present?
	if(!player[0].ent) return player[1].ent;
	if(!player[1].ent) return player[0].ent;


	dx1 = self->x - player[0].ent->x;
	dz1 = self->z - player[0].ent->z;
	dx2 = self->x - player[1].ent->x;
	dz2 = self->z - player[1].ent->z;


	// Is self located between two players (x-wise)?
	if((dx1<0 && dx2>0) || (dx1>0 && dx2<0)){
		// Stay targeted at the one in front of self
		if(self->direction ? dx1<0 : dx1>0) return player[0].ent;
		return player[1].ent;
	}

	// Target the closest opponent
	if(dx1<0) dx1 = -dx1;
	if(dz1<0) dz1 = -dz1;
	if(dx2<0) dx2 = -dx2;
	if(dz2<0) dz2 = -dz2;

	if(dx1+dz1 < dx2+dz2) return player[0].ent;
	return player[1].ent;
}



// Returns 1 if a player was thrown!
int enemy_trymove(float xdir, float zdir){

	entity *other;


	if(!inair(self) && !self->projectile){
		// Walking

		// Out of bounds? Return to level!
		if(self->zdir && self->z < PLAYER_MIN_Z) self->zdir = self->model->speed/2;
		if(self->zdir && self->z > PLAYER_MAX_Z) self->zdir = -self->model->speed/2;

		if(self->xdir || self->zdir){
			// Don't walk into a hole
			if(checkhole(self->x + xdir*5, self->z)) xdir = 0;
			if(checkhole(self->x, self->z + zdir*5)) zdir = 0;

			// Don't walk through obstacles
			if(xdir && (other = find_ent_here(self, self->x + xdir*2, self->z, TYPE_OBSTACLE))){
				if(xdir>0 ? other->x>self->x : other->x<self->x) xdir = 0;
			}
			if(zdir && (other=find_ent_here(self, self->x, self->z + zdir*2, TYPE_OBSTACLE))){
				if(zdir>0 ? other->z>self->z : other->z<self->z) zdir = 0;
			}

			// if(!self->xdir && !self->zdir) ent_set_anim(self, ANI_IDLE);
		}
	}

	self->x += xdir;
	self->z += zdir;

	// Do a throw?
	if(		self->health>0 &&
			self->model->animation[ANI_THROW] &&
			!inair(self) &&
			(other = find_ent_here(self, self->x + xdir, self->z + zdir, TYPE_PLAYER)) &&
			!inair(other) &&
			(self->direction ? other->x>self->x : other->x<self->x) 
			){

		// Reposition both on average height and such
		zdir = (self->z + other->z) / 2;
		other->z = zdir;
		self->z = zdir+1;	// Grabber up front

		xdir = (self->x + other->x) / 2;
		if(self->x < other->x){
			self->x = xdir - (GRAB_DIST/2);
			other->x = xdir + (GRAB_DIST/2);
			self->direction = 1;
		}
		else{
			self->x = xdir + (GRAB_DIST/2);
			other->x = xdir - (GRAB_DIST/2);
			self->direction = 0;
		}

		player[other->playerindex].opponent = self;
		other->attacking = 0;

		self->think = enemy_throw;
		ent_set_anim(self, ANI_THROW);
		ent_unlink(self);

		toss(other, 4);
		other->direction = self->direction;
		other->projectile = 1;
		other->think = player_fall;
		other->damage_on_landing = THROW_DAMAGE;
		ent_set_anim(other, ANI_FALL);
		ent_unlink(other);

		return 1;
	}


	// Do a grab?
	if(		self->health>0 &&
			self->model->animation[ANI_GRAB] &&
			!inair(self) &&
			(other = find_ent_here(self, self->x + xdir, self->z + zdir, TYPE_PLAYER)) &&
			!inair(other) &&
			(self->direction ? other->x>self->x : other->x<self->x)
			){

		// Reposition both on average height and such
		zdir = (self->z + other->z) / 2;
		other->z = zdir;
		self->z = zdir+1;	// Grabber up front

		xdir = (self->x + other->x) / 2;
		if(self->x < other->x){
			self->x = xdir - (GRAB_DIST/2);
			other->x = xdir + (GRAB_DIST/2);
			self->direction = 1;
		}
		else{
			self->x = xdir + (GRAB_DIST/2);
			other->x = xdir - (GRAB_DIST/2);
			self->direction = 0;
		}
		other->direction = !self->direction;

		player[other->playerindex].opponent = self;
		other->attacking = 0;

		self->think = enemy_grab;
		self->attacking = 1;
		ent_set_anim(self, ANI_GRAB);
		ents_link(self, other);

		other->think = player_grabbed;
		player[other->playerindex].opponent = self;
		ent_set_anim(other, ANI_PAIN);

		return 1;
	}

	// End of level is blocked?
	if(level->exit_blocked){
		if(self->x > level->width-30-(PLAYER_MAX_Z-self->z)) self->x = level->width-30-(PLAYER_MAX_Z-self->z);
	}
	return 0;
}



void enemy_think(void){
	entity *target = enemy_find_target();
	int rnum;
	int walk;


	if(!target) return;	// There are no players?


	self->direction = (self->x < target->x);

	if(self->animating && self->animation == self->model->animation[ANI_WALK]){
		if(self->direction ? self->xdir<0 : self->xdir>0){
			// Walking backwards
			self->animating = -1;
		}
		else self->animating = 1;
	}


	// Target in range?
	if(diff(self->z, target->z) < CONTACT_DIST_V){

		// Target jumping? Try uppercut!
		if(target->jumping && self->model->animation[ANI_UPPER] &&
			(self->direction ?
			 self->x<target->x-10 && target->x<self->x+120 :
			 self->x>target->x+10 && target->x>self->x-120 )){

			// Don't waste any time!
			ent_set_anim(self, ANI_UPPER);
			self->think = enemy_attack;
			return;
		}

		// In range for any of the attacks?
		if(self->direction ?
		  (self->model->animation[ANI_ATTACK1] && target->x > self->x+self->model->animation[ANI_ATTACK1]->range[0] && target->x < self->x+self->model->animation[ANI_ATTACK1]->range[1]) ||
		  (self->model->animation[ANI_ATTACK2] && target->x > self->x+self->model->animation[ANI_ATTACK2]->range[0] && target->x < self->x+self->model->animation[ANI_ATTACK2]->range[1]) ||
		  (self->model->animation[ANI_ATTACK3] && target->x > self->x+self->model->animation[ANI_ATTACK3]->range[0] && target->x < self->x+self->model->animation[ANI_ATTACK3]->range[1])
		  :
		  (self->model->animation[ANI_ATTACK1] && target->x < self->x-self->model->animation[ANI_ATTACK1]->range[0] && target->x > self->x-self->model->animation[ANI_ATTACK1]->range[1]) ||
		  (self->model->animation[ANI_ATTACK2] && target->x < self->x-self->model->animation[ANI_ATTACK2]->range[0] && target->x > self->x-self->model->animation[ANI_ATTACK2]->range[1]) ||
		  (self->model->animation[ANI_ATTACK3] && target->x < self->x-self->model->animation[ANI_ATTACK3]->range[0] && target->x > self->x-self->model->animation[ANI_ATTACK3]->range[1])
		  ){

			ent_set_anim(self, ANI_IDLE);
			self->think = enemy_prepare;
			self->stalltime = bortime + (GAME_SPEED/4) + (rand32()%(GAME_SPEED/10));
			return;
		}

		if((self->model->animation[ANI_JUMPATTACK] || self->model->animation[ANI_JUMPATTACK2])
		  && (self->direction ?
		  (target->x > self->x+150 && target->x < self->x+200):
		  (target->x < self->x-150 && target->x > self->x-200))
		  ){

			rnum = 0;
			if(!self->model->animation[ANI_JUMPATTACK]) rnum = 1;
			else if(self->model->animation[ANI_JUMPATTACK2] && (rand32()&1)) rnum = 1;

			if(rnum==0){
				ent_set_anim(self, ANI_JUMPATTACK);
				if(self->direction) self->xdir = 1.3;
				else self->xdir = -1.3;
			}
			else{
				ent_set_anim(self, ANI_JUMPATTACK2);
				self->xdir = 0;
			}
			self->think = enemy_jumpattack;
			self->attacking = 1;
			self->jumping = 1;

			toss(self, 3.5);

			return;
		}

	}

	if(self->stalltime < bortime){
		// Decide next direction... randomly.

		walk = 0;
		self->xdir = self->zdir = 0;

		rnum = rand32();
		if(((rnum & 7) < 3) || diff(self->x, target->x) > 160){
			// Walk forward
			if(self->direction==1) self->xdir = self->model->speed;
			else self->xdir = -self->model->speed;
			walk = 1;
		}
		else if((rnum & 7) > 4){
			// Walk backward
			if(self->direction==1) self->xdir = -self->model->speed/2;
			else self->xdir = self->model->speed/2;
			walk = -1;
		}

		rnum = rand32();
		if((rnum & 7) < 2){
			// Move up
			self->zdir = -self->model->speed/2;
			walk |= 1;
		}
		else if((rnum & 7) > 5){
			// Move down
			self->zdir = self->model->speed/2;
			walk |= 1;
		}

		if(walk){
			ent_set_anim(self, ANI_WALK);
			// if(walk<0) self->animating = -1;
		}
		else ent_set_anim(self, ANI_IDLE);

		self->stalltime = bortime + GAME_SPEED;
	}

	enemy_trymove(self->xdir, self->zdir);
}



void enemy_drop(void){
	if(inair(self)) return;
	self->think = enemy_think;
	if(self->health<=0) borkill(self);
}



void enemy_jumpattack(void){
	if(self->animpos != self->lastanimpos && self->animpos == self->animation->throwframe){
		star_spawn(self->x + (self->direction ? 56 : -56), self->z, self->a+67, self->direction);
	}
	self->lastanimpos = self->animpos;

	self->x += self->xdir;
	if(inair(self)) return;

	self->attacking = 0;
	self->jumping = 0;
	self->xdir = 0;
	ent_set_anim(self, ANI_IDLE);
	self->think = enemy_think;
}



void enemy_throw(void){
	if(self->animating) return;
	self->think = enemy_think;
	ent_set_anim(self, ANI_IDLE);
}


void enemy_grab(void){
	// if(self->link) return;
	if(self->animating) return;
	ent_unlink(self);
	self->think = enemy_think;
	self->attacking = 0;
	ent_set_anim(self, ANI_IDLE);
}


void enemy_grabbed(void){
	// Escape?
	if(bortime >= self->stalltime && self->model->animation[ANI_SPECIAL]){
		ent_unlink(self);
		ent_set_anim(self, ANI_SPECIAL);
		self->attacking = 1;
		self->think = enemy_attack;
		return;
	}

	// Just check if we're still grabbed...
	if(self->link) return;

	ent_set_anim(self, ANI_IDLE);
	self->xdir = self->zdir = 0;
	self->think = enemy_think;
}


void enemy_runoff(){
	entity *target = enemy_find_target();

	if(!target) return;	// There are no players?

	self->direction = (self->x < target->x);
	if(self->direction) self->xdir = -self->model->speed/2;
	else self->xdir = self->model->speed/2;
	enemy_trymove(self->xdir, 0);

	if(bortime > self->stalltime) self->think = enemy_think;
}


void enemy_attack(){
	entity *target;

	self->attacking = 1;
	if(self->animpos != self->lastanimpos && self->animpos == self->animation->throwframe){
		knife_spawn(self->x, self->z, self->direction);
	}
	else if(self->animpos != self->lastanimpos && self->animpos == self->animation->shootframe){
		fireball_spawn(self->x, self->z, self->direction);
	}
	else if(self->animpos != self->lastanimpos && self->animpos == self->animation->jumpframe){
		toss(self, 4);
	}
	self->lastanimpos = self->animpos;
	if(self->animating) return;
	self->attacking = 0;
	self->xdir = 0;
	self->zdir = 0;

	target = enemy_find_target();
	if(target && diff(self->x, target->x)<80 && (rand32()&3)){
		ent_set_anim(self, ANI_WALK);
		self->think = enemy_runoff;
	}
	else{
		ent_set_anim(self, ANI_IDLE);
		self->think = enemy_think;
	}
	self->stalltime = bortime + GAME_SPEED;
}


// Idle a bit before attacking
void enemy_prepare(){
	entity *target = enemy_find_target();
	int pickable[3];

	if(!target){
		// There are no players?
		self->think = enemy_think;
		return;
	}
	self->direction = (self->x < target->x);

	// Wait...
	if(bortime < self->stalltime) return;


	// Pick an attack
	pickable[0] = pickable[1] = pickable[2] = 0;

	if(self->model->animation[ANI_ATTACK1] &&
			(self->direction ? target->x > self->x+self->model->animation[ANI_ATTACK1]->range[0] && target->x < self->x+self->model->animation[ANI_ATTACK1]->range[1]
			: target->x < self->x-self->model->animation[ANI_ATTACK1]->range[0] && target->x > self->x-self->model->animation[ANI_ATTACK1]->range[1])){

		pickable[0] = 1;
	}
	if(self->model->animation[ANI_ATTACK2] &&
			(self->direction ? target->x > self->x+self->model->animation[ANI_ATTACK2]->range[0] && target->x < self->x+self->model->animation[ANI_ATTACK2]->range[1]
			: target->x < self->x-self->model->animation[ANI_ATTACK2]->range[0] && target->x > self->x-self->model->animation[ANI_ATTACK2]->range[1])){

		pickable[1] = 1;
		if((rand32() & 31) < 10) pickable[0] = 0;
	}
	if(self->model->animation[ANI_ATTACK3] &&
			(self->direction ? target->x > self->x+self->model->animation[ANI_ATTACK3]->range[0] && target->x < self->x+self->model->animation[ANI_ATTACK3]->range[1]
			: target->x < self->x-self->model->animation[ANI_ATTACK3]->range[0] && target->x > self->x-self->model->animation[ANI_ATTACK3]->range[1])){

		pickable[2] = 1;
		if((rand32() & 31) < 10) pickable[1] = 0;
	}

	if(pickable[0]){
		ent_set_anim(self, ANI_ATTACK1);
		self->think = enemy_attack;
		return;
	}
	if(pickable[1]){
		ent_set_anim(self, ANI_ATTACK2);
		self->think = enemy_attack;
		return;
	}
	if(pickable[2]){
		ent_set_anim(self, ANI_ATTACK3);
		self->think = enemy_attack;
		return;
	}

	// No attack to perform, return to normal
	self->think = enemy_think;
}



void enemy_fall(void){
	if(self->projectile){
		if(self->direction) enemy_trymove(-2.5, 0);
		else enemy_trymove(2.5, 0);
	}
	else{
		if(self->direction) enemy_trymove(-1.2, 0);
		else enemy_trymove(1.2, 0);
	}

	// Still falling?
	if(inair(self)){
		if(self->a < PIT_DEPTH && self->health<=0) borkill(self);
		return;
	}

	// Landed
	if(self->projectile){
		make_quake(4);
		if(self->takedamage && self->damage_on_landing){
			self->takedamage(self, self->damage_on_landing, 0, ATK_NORMAL);
		}
		self->damage_on_landing = 0;
		self->projectile = 0;
	}

	// Hard landing? Quake and bounce!
	if(self->tossv < -2){
		make_quake(2);
		sound_play_sample(smp_fall, 0, savedata.effectvol,savedata.effectvol, 100);
		toss(self, 1);
		return;
	}

	// Pause a bit...
	self->think = enemy_lie;
	self->stalltime = bortime + GAME_SPEED;
}



void enemy_lie(void){
	// Died?
	if(self->health <= 0){
		self->think = suicide;
		self->nextthink = bortime + GAME_SPEED * 2;
		self->blink = 1;
		return;
	}

	// Stall
	if(bortime < self->stalltime) return;

	// Get up again
	ent_set_anim(self, ANI_RISE);
	self->xdir = self->zdir = 0;
	self->think = enemy_rise;
}



void enemy_rise(void){
	if(self->animating) return;
	ent_set_anim(self, ANI_IDLE);
	self->xdir = self->zdir = 0;
	self->think = enemy_think;
}




void enemy_pain(void){
	if(self->animating) return;
	if(self->link){
		ent_set_anim(self, ANI_PAIN);
		self->think = enemy_grabbed;
	}
	else{
		ent_set_anim(self, ANI_IDLE);
		self->think = enemy_think;
	}
}




void enemy_takedamage(entity *other, int force, int drop, int type){

	if(self->a<=PIT_DEPTH && self->health<=0){
		// Don't scream twice
		borkill(self);
		return;
	}

	self->pain_time = bortime + (GAME_SPEED / 5);

	if(self->x < other->x) self->direction = 1;
	else if(self->x > other->x) self->direction = 0;

	self->attacking = 0;

	if(other != self->link) ent_unlink(self);

	if(type == ATK_BLAST){
		self->projectile = 1;
		drop = 1;
	}

	self->health -= force;
	if(other->type == TYPE_PLAYER){
		player[other->playerindex].opponent = self;
		self->playerindex = other->playerindex;
		addscore(other->playerindex, force*5);
	}

	if(self->health <= 0){
		if(self->model->diesound>=0) sound_play_sample(self->model->diesound, 0, savedata.effectvol,savedata.effectvol, 100);

		if(self->boss){
			kill_all_enemies();
			level_completed = 1;
		}

		// Fell in a hole
		if(self->a < PIT_DEPTH){
			borkill(self);
			return;
		}
	}


	// Don't animate or fall if hurt by self, since
	// it means self fell to the ground already. :)
	if(other==self){
		addscore(other->playerindex, force);
		return;
	}

	if(drop || self->health<=0 || inair(self)){
		ent_unlink(self);
		toss(self, 2.5 + randf(1));
		ent_reset_anim(self, ANI_FALL);
		self->think = enemy_fall;
	}
	else{
		ent_reset_anim(self, ANI_PAIN);
		self->think = enemy_pain;
	}
}





void biker_drive(void){
	self->attacking = 1;
	if(self->direction){
		self->x += self->xdir;
		if(self->x > advancex+520){
			self->z = PLAYER_MIN_Z + randf(PLAYER_MAX_Z-PLAYER_MIN_Z);
			self->direction = 0;
			self->attack_id = 0;
			sound_play_sample(smp_bike, 0, savedata.effectvol,savedata.effectvol, 100);
			self->xdir = 1.7 + randf(0.6);
		}
	}
	else{
		self->x -= self->xdir;
		if(self->x < advancex-200){
			self->z = PLAYER_MIN_Z + randf(PLAYER_MAX_Z-PLAYER_MIN_Z);
			self->direction = 1;
			self->attack_id = 0;
			sound_play_sample(smp_bike, 0, savedata.effectvol,savedata.effectvol, 100);
			self->xdir = 1.7 + randf(0.6);
		}
	}
	self->nextthink = bortime + THINK_SPEED / 2;
}



void bike_crash(void){
	if(self->direction) self->x += 2;
	else self->x -= 2;
	self->nextthink = bortime + THINK_SPEED / 2;

	if(self->x < advancex-100 || self->x > advancex+420) borkill(self);
}



void biker_takedamage(entity *other, int force, int drop, int type){

	entity * driver;


	if(other->type == TYPE_PLAYER){
		player[other->playerindex].opponent = self;
		self->playerindex = other->playerindex;
		addscore(other->playerindex, force);
	}


	if(self->boss){
		kill_all_enemies();
		level_completed = 1;
	}

	// Fell in a hole
	if(self->a < PIT_DEPTH){
		if(self->model->diesound>=0) sound_play_sample(self->model->diesound, 0, savedata.effectvol,savedata.effectvol, 100);
		borkill(self);
		return;
	}

	ent_reset_anim(self, ANI_PAIN);
	self->attacking = 1;
	self->think = bike_crash;
	self->nextthink = bortime + THINK_SPEED;

// TODO: was this a bug? probably not.
	if((driver = spawn(self->x, self->z, 10, "K'"))){
		driver->maxhealth = self->maxhealth;
		driver->health = self->health - (force*3);
		if(driver->health <= 0){
			if(self->model->diesound>=0) sound_play_sample(self->model->diesound, 0, savedata.effectvol,savedata.effectvol, 100);
		}

		toss(driver, 2.5 + randf(1));
		ent_reset_anim(driver, ANI_FALL);
		driver->takedamage = enemy_takedamage;
		driver->think = enemy_fall;
	}

	self->health = 0;
}






void obstacle_fall(void){
	if(inair(self)) self->x += self->xdir;
	else borkill(self);
}



void obstacle_takedamage(entity *other, int force, int unused1, int unused2){
	entity *item;
	if(self->a <= PIT_DEPTH){
		borkill(self);
		return;
	}
	if(other->type==TYPE_PLAYER) player[other->playerindex].opponent = self;
	self->health -= force;
	if(self->health<=0){
		if(self->model->diesound>=0) sound_play_sample(self->model->diesound, 0, savedata.effectvol,savedata.effectvol, 100);
		if(self->item[0] && (!self->if2p || count_ents(TYPE_PLAYER)>1)){
			item = spawn(self->x, self->z, self->a, self->item);
			if(item) item->direction = 1;
		}
		if(other->x < self->x) self->xdir = 1;
		else self->xdir = -1;
		toss(self, 3);
		ent_set_anim(self, ANI_FALL);
		self->blink = 1;

		// I will now become self-aware. Oh glory.
		self->think = obstacle_fall;
		self->nextthink = bortime + 1;
	}
}





entity * smartspawn(s_spawn_entry * props){
	entity *e;
//	int alternative = 0;
	int dodrop;

	if(!props) return NULL;

	e = spawn(props->x + (int)advancex, props->z, props->a, props->name);
	if(e==NULL) return NULL;

	// Alias?
	if(props->alias[0]) strncpy(e->name, props->alias, MAX_NAME_LEN);

	if(props->health!=0){
		e->health = props->health;
		e->maxhealth = props->health;
	}
	ent_set_colourmap(e, props->colourmap);

	if(props->boss && level && level->bossmusic[0]){
		music(level->bossmusic, 1);
	}

	e->direction = props->flip;

	switch(e->type){
		case TYPE_PLAYER:
			e->direction = (props->x < 160);
			e->takedamage = player_takedamage;

			if(bortime){
				// Mid-level spawn
				e->a = 300;
				e->jumping = 1;
				ent_set_anim(e, ANI_JUMP);
				e->think = player_jump;
			}
			else{
				ent_set_anim(e, ANI_IDLE);
				e->think = player_think;
			}
			break;

		case TYPE_ENEMY:
			if(e->model->subtype==SUBTYPE_BIKER){
				e->takedamage = biker_takedamage;
				e->think = biker_drive;
				e->nograb = 1;
				e->attacking = 1;
				if(e->x < 0) e->direction = 0;
				else e->direction = 1;
				e->xdir = 2;
				// sound_play_sample(smp_bike, 0, savedata.effectvol,savedata.effectvol, 100);
				break;
			}
			e->think = enemy_drop;
			e->takedamage = enemy_takedamage;
			e->boss = props->boss;
			dodrop = (level && (level->scrolldir==SCROLL_UP || level->scrolldir==SCROLL_DOWN));
			if((dodrop && e->x > -30 && e->x < 350) || (props->x > 0 && props->x < 320)){
				e->a = 240 + randf(40);
				if(e->health<1){
					// Funny: drop dead right away
					ent_set_anim(e, ANI_FALL);
					e->think = enemy_fall;
				}
			}
			break;

		case TYPE_OBSTACLE:
			strncpy(e->item, props->item, MAX_NAME_LEN);
			e->if2p = props->if2p;
			e->takedamage = obstacle_takedamage;
			ent_set_colourmap(e, props->colourmap);
			break;

		case TYPE_STEAMER:
			e->think = steamer_think;
			e->base = e->a;
			break;

		case TYPE_NONE:
			if(e->model->animation[ANI_WALK]){
				if(props->x < 160) e->xdir = e->model->speed + randf(3);
				else e->xdir = -(e->model->speed + randf(3));
				e->think = anything_walk;
				ent_set_anim(e, ANI_WALK);
			}
			break;
	}
	return e;
}



void spawnplayer(int index){
	s_spawn_entry p;

	index &= 1;

	if(!player[index].model) return;

	memset(&p, 0, sizeof(s_spawn_entry));
//TODO: was bug?
//	strncpy(&p.name, player[index].model->name, MAX_NAME_LEN);
        strncpy(p.name, player[index].model->name, MAX_NAME_LEN);

	p.colourmap = player[index].colourmap;
	if(level->scrolldir==SCROLL_LEFT) p.x = 300 - 30*index;
	else p.x = 20 + 30*index;
	p.z = PLAYER_MIN_Z + 50 - 40*index;
	player[index].ent = smartspawn(&p);
	if(player[index].ent==NULL) shutdown("Fatal: unable to spawn player from '%s'", &p.name);
	player[index].ent->playerindex = index;
	if(player[index].spawnhealth) player[index].ent->health = player[index].spawnhealth + 5;
	if(player[index].ent->health > player[index].ent->model->health) player[index].ent->health = player[index].ent->model->health;
}





void time_over(void){
	if(!player[0].ent && !player[0].ent) endgame = 1;

	if(player[0].ent && player[0].ent->takedamage){
		self = player[0].ent;
		self->takedamage(self, 1000, 1, 0);
	}
	if(player[1].ent && player[1].ent->takedamage){
		self = player[1].ent;
		self->takedamage(self, 1000, 1, 0);
	}
	sound_play_sample(smp_timeover, 0, savedata.effectvol,savedata.effectvol, 100);
	timeleft = 100 * COUNTER_SPEED;
	showtimeover = 1;
}







// ----------------------- Update functions ------------------------------


// Used by screenshot func
#ifndef PS2PORT
int fileexists(char *fnam){
	int handle;
	if((handle=open(fnam, O_RDONLY|O_BINARY))<0) return 0;
	close(handle);
	return 1;
}


void screenshot(){
	static int shotnum = 0;
	static char shotname[20];
	do{
		sprintf(shotname, "shot%04u.pcx", shotnum);
		++shotnum;
	}while(fileexists(shotname) && shotnum<10000);
	if(shotnum<10000) savepcx(shotname, vscreen, pal);
	debug_printf("Saved %s.", shotname);
}

#endif

void update_scroller(){
	float to;

	if(bortime < advancetime) return;
	advancetime = bortime + (GAME_SPEED/100);

	if(level_completed) return;

	if(current_spawn>=level->numspawns && !findent(TYPE_ENEMY)){
		level_completed = 1;
	}
	else if(count_ents(TYPE_ENEMY) < groupmin){
		while(count_ents(TYPE_ENEMY) < groupmax &&
			current_spawn<level->numspawns &&
			levelpos >= level->spawnpoints[current_spawn].at
		){

			if(level->spawnpoints[current_spawn].wait){
				level_waiting = 1;
				go_time = 0;
			}
			else if(level->spawnpoints[current_spawn].groupmin || level->spawnpoints[current_spawn].groupmax){
				groupmin = level->spawnpoints[current_spawn].groupmin;
				groupmax = level->spawnpoints[current_spawn].groupmax;
			}
			else smartspawn(&level->spawnpoints[current_spawn]);
			++current_spawn;
		}
	}

	if(level_waiting){
		// Wait for all enemies to be defeated
		if(findent(TYPE_ENEMY)) return;

		level_waiting = 0;
		timeleft = 100 * COUNTER_SPEED;
		go_time = bortime + 3*GAME_SPEED;
	}

	if(level->scrolldir==SCROLL_RIGHT){
		if(player[0].ent && player[1].ent) to = (player[0].ent->x + player[1].ent->x) / 2;
		else if(player[0].ent) to = player[0].ent->x;
		else if(player[1].ent) to = player[1].ent->x;
		else return;
		to -= 160;

		if(to > advancex){
			if(to > advancex+1) to = advancex+1;
			advancex = to;
		}
		if(advancex > level->width-320){
			advancex = level->width-320;
		}
		if(advancex < 0) advancex = 0;

		levelpos = advancex;
	}
	else if(level->scrolldir==SCROLL_LEFT){
		if(player[0].ent && player[1].ent) to = (player[0].ent->x + player[1].ent->x) / 2;
		else if(player[0].ent) to = player[0].ent->x;
		else if(player[1].ent) to = player[1].ent->x;
		else return;
		to -= 160;

		if(to < advancex){
			if(to < advancex-1) to = advancex-1;
			advancex = to;
		}
		if(advancex > level->width-320){
			advancex = level->width-320;
		}
		if(advancex < 0) advancex = 0;

		levelpos = (level->width-320) - advancex;
	}
	else{
		advancey += 0.5;
		levelpos = advancey;
	}
}





void draw_scrolled_bg(){
	int i;
	int inta;
	int poop;
	int index;
	static int neon_count = 0;
	static int rockpos = 0;
	static int rockoffs[32] = {
		2, 2, 3, 4, 5, 6, 7, 7,
		8, 8, 9, 9, 9, 9, 8, 8,
		7, 7, 6, 5, 4, 3, 2, 2,
		1, 1, 0, 0, 0, 0, 1, 1
	};

	if(advancex < 0) advancex = 0;

	if(background){
		switch(level->scrolldir){
		case SCROLL_DOWN:
			inta = (advancey+0.5)/2;
			inta %= background->height;

			for(i=-inta; i<240; i+=background->height){
				copyscreen_o(vscreen, background, 0, i);
			}
			break;
		case SCROLL_UP:
			inta = (advancey+0.5)/2;
			inta %= background->height;

			for(i=inta-background->height; i<240; i+=background->height){
				copyscreen_o(vscreen, background, 0, i);
			}
			break;
		default:
			inta = (advancex+0.5)/2;
			if(level && level->rocking) inta += (bortime/(GAME_SPEED/30));
			inta %= background->width;
			for(i=-inta; i<320; i+=background->width){
				copyscreen_o(vscreen, background, i, 0);
			}
		}

		// Append bg with texture?
		if(texture && level){
			inta = advancex/2;
			if(level->rocking){
				inta += (bortime/(GAME_SPEED/30));
				texture_plane(vscreen, 0,background->height, vscreen->width,160-background->height, inta*256, 10, texture);
			}
			else texture_wave(vscreen, 0,background->height, vscreen->width, 160-background->height, inta,0, texture, bortime, 5);
		}
	}

	if(level==NULL) return;

	if(level->rocking){
		rockpos = (bortime/(GAME_SPEED/8)) & 31;
		gfx_y_offset = quake - 4 - rockoffs[rockpos];
	}
	else{
		gfx_y_offset = quake - 4;
	}


	// Draw 3 layers: screen, normal and neon
	if(panels_loaded && panel_width){
		if(bortime>=neon_time){
			for(i=0; i<8; i++){
				neontable[128+i] = 128 + ((i+neon_count) & 7);
			}
			neon_time = bortime + (GAME_SPEED/3);
			neon_count += 2;
		}

		if(level->scrolldir==SCROLL_UP || level->scrolldir==SCROLL_DOWN){
			inta = 0;
		}
		else inta = advancex;
		poop = inta / panel_width;
		inta %= panel_width;
		for(i=-inta; i<=320 && poop>=0 && poop<level->numpanels; i+=panel_width){
			index = level->order[poop];
			if(panels[index].sprite_normal) spriteq_add(i,gfx_y_offset, PANEL_Z, panels[index].sprite_normal, 0, NULL);
			if(panels[index].sprite_neon)   spriteq_add(i,gfx_y_offset, NEONPANEL_Z, panels[index].sprite_neon, SFX_REMAP, neontable);
			if(panels[index].sprite_screen) spriteq_add(i,gfx_y_offset, SCREENPANEL_Z, panels[index].sprite_screen, SFX_BLEND, lut_screen);
			poop++;
		}
	}


	for(i=0; i<level->numholes; i++){
		spriteq_add(level->holes[i]-advancex,199+gfx_y_offset, HOLE_Z, sprites[1][holesprite], 0, NULL);
	}

	if(frontpanels_loaded){
		if(level->scrolldir==SCROLL_UP || level->scrolldir==SCROLL_DOWN){
			inta = 0;
		}
		else inta = advancex * 1.4;
		poop = inta / frontpanels[0]->width;
		inta %= frontpanels[0]->width;
		for(i=-inta; i<=320; i+=frontpanels[0]->width){
			poop %= frontpanels_loaded;
			spriteq_add(i,gfx_y_offset, FRONTPANEL_Z, frontpanels[poop], 0, NULL);
			poop++;
		}
	}

	if(quake>0 && bortime>=quaketime){
		--quake;
		quaketime = bortime + (GAME_SPEED/25);
	}
}







static char debug_msg[2048];
unsigned gamelib_long debug_time = 0xFFFFFFFF;


#include <fileio.h>

void debug_printf(char *format, ...){
	va_list arglist;
	
	va_start(arglist, format);
	vsprintf(debug_msg, format, arglist);
	va_end(arglist);

	debug_time = 0xFFFFFFFF;
	
	printf("%s", debug_msg);
	
	fioSync(FIO_WAIT, NULL);
}





void update(int ingame, int usevwait){

	int slowmo = 0;
	unsigned gamelib_long newtime;
	int p;


	display_ents();

	if(ingame){
		draw_scrolled_bg();
		predrawstatus();
		slowmo = level_completed;
	}
	else if(background) copyscreen(vscreen, background);

	spriteq_draw(vscreen);
	if(ingame) drawstatus();

#ifndef PS2PORT
	if(bothnewkeys & FLAG_SCREENSHOT) screenshot();
#endif

#ifdef DEBUG_MODE
	// Debug stuff, should not appear on screenshot
	if(debug_time==0xFFFFFFFF) debug_time = bortime + GAME_SPEED * 5;
	if(bortime<debug_time && debug_msg[0]){
		spriteq_clear();
                font_printf(0,230-16, 0, debug_msg);
		spriteq_draw(vscreen);
	}
	else{
		debug_msg[0] = 0;
		if(levelpos) debug_printf("Position: %i, width: %i, spawn: %i, offsets: %i/%i", levelpos, level->width, current_spawn, quake, gfx_y_offset);
	}
#endif

{ static unsigned stuff = 0;
//  debug_printf("mem: %d bytes / time: %d lines", tracemalloc_total, (timer_gettime() - stuff) & 0xFFFF);

	if(usevwait) vga_vwait();
	if(rescreen){
		scalescreen(rescreen, vscreen);
		video_copy_screen(rescreen);
	}
	else video_copy_screen(vscreen);
  stuff = timer_gettime();
}

	spriteq_clear();

	control_update(playercontrolpointers, 2);


	bothkeys = 0;
	bothnewkeys = 0;

	for(p=0; p<2; p++){
		player[p].keys = playercontrolpointers[p]->keyflags;
		player[p].newkeys = playercontrolpointers[p]->newkeyflags;
		player[p].playkeys |= player[p].newkeys;
		player[p].playkeys &= player[p].keys;

		bothkeys |= player[p].keys;
		bothnewkeys |= player[p].newkeys;
	}


	if(slowmo) newtime = bortime + timer_getinterval(GAME_SPEED / 2);
	else newtime = bortime + timer_getinterval(GAME_SPEED);
	if(pause) newtime = bortime;
	if(newtime > bortime+100) newtime = bortime+100;
	while(bortime < newtime){
		update_ents();
		if(ingame){
			update_scroller();
			if(timeleft>0) --timeleft;
			else time_over();
		}
		++bortime;
	}

	check_music_opened();

	sound_update_music();
}




// ----------------------------------------------------------------------



// Simple palette fade
void fade_out(){
	int i, j;
//	int x, y;
	int b, g;
//	int xd, yd, d;
//	char r=0, s=0;

	timer_getinterval(24);
	for(i=0, j=0; j<64; ){
		while(j<=i){
			b = ((savedata.brightness+256) * (64-j) / 64) - 256;
			g = 256 - ((savedata.gamma+256) * (64-j) / 64);
			vga_vwait();
			palette_set_corrected(pal, g,savedata.gamma,savedata.gamma, b,b,b);
			j++;
		}

		if(rescreen){
			scalescreen(rescreen, vscreen);
			video_copy_screen(rescreen);
		}
		else video_copy_screen(vscreen);

		sound_update_music();
		sound_volume_music(savedata.musicvol*(64-j)/64, savedata.musicvol*(64-j)/64);
		i += timer_getinterval(24);
	}
	sound_close_music();

	clearscreen(vscreen);
	if(rescreen){
		scalescreen(rescreen, vscreen);
		video_copy_screen(rescreen);
	}
	else video_copy_screen(vscreen);
	vga_vwait();
	palette_set_corrected(pal, savedata.gamma,savedata.gamma,savedata.gamma, savedata.brightness,savedata.brightness,savedata.brightness);
}





void apply_controls(){
	int p;

	for(p=0; p<2; p++){
		control_setkey(playercontrolpointers[p], FLAG_ESC, 	  CONTROL_ESC);
		control_setkey(playercontrolpointers[p], FLAG_MOVEUP,     savedata.keys[p][SDID_MOVEUP]);
		control_setkey(playercontrolpointers[p], FLAG_MOVEDOWN,   savedata.keys[p][SDID_MOVEDOWN]);
		control_setkey(playercontrolpointers[p], FLAG_MOVELEFT,   savedata.keys[p][SDID_MOVELEFT]);
		control_setkey(playercontrolpointers[p], FLAG_MOVERIGHT,  savedata.keys[p][SDID_MOVERIGHT]);
		control_setkey(playercontrolpointers[p], FLAG_SPECIAL,    savedata.keys[p][SDID_SPECIAL]);
		control_setkey(playercontrolpointers[p], FLAG_ATTACK,     savedata.keys[p][SDID_ATTACK]);
		control_setkey(playercontrolpointers[p], FLAG_JUMP,       savedata.keys[p][SDID_JUMP]);
		control_setkey(playercontrolpointers[p], FLAG_START,      savedata.keys[p][SDID_START]);
		control_setkey(playercontrolpointers[p], FLAG_SCREENSHOT, savedata.keys[p][SDID_SCREENSHOT]);
	}
}



// ----------------------------------------------------------------------



//#include "gsvga.h"

void shutdown(char *msg, ...){
	static char buf[2048];
	va_list arglist;
	
	va_start(arglist, msg);
	vsprintf(buf, msg, arglist);
	va_end(arglist);
	
	debug_printf("Shutdown() called: %s\n", buf);

	gsvga_shutdown(buf);

//	savesettings();

	video_set_mode(0,0);

	debug_printf("Release level data...\n");
	unload_levelorder();
	unload_level();

	debug_printf("Release graphics data...\n");
	freescreen(vscreen);
	freescreen(rescreen);
	freesprites();
	font_unload(0);
	font_unload(1);
	font_unload(2);

	debug_printf("Release game data...\n");
	free_ents();
	free_models();
	free_modelcache();

	debug_printf("Release timer...\n");
	timer_exit();

	debug_printf("Release input hardware...\n");
	control_exit();

	debug_printf("Release sound system...\n");
	sound_exit();

	debug_printf("Done.\n\n");
	debug_printf(buf);

	gsvga_shutdown(buf);

	ExitDeleteThread();
}




void startup(){
	int i;

	debug_printf("Beats of Rage V%X.%04X, compile date: " __DATE__ "\n\n", VERSION>>16, VERSION&0xFFFF);

	loadsettings();
	
	gsvga_tweak(savedata.offset_X, savedata.offset_Y, savedata.interlace, savedata.videomode);

	debug_printf("Screen allocation...\n");
	if((vscreen = allocscreen(320, 240)) == NULL){
		shutdown("Not enough memory!\n");
	}
	clearscreen(vscreen);

//shutdown("I am a test shutdown\n");

	debug_printf("Loading font...\n");
	if(!font_load(0, "data/sprites/font", packfile)) shutdown("Unable to load font #1!\n");
	if(!font_load(1, "data/sprites/font2", packfile)) shutdown("Unable to load font #2!\n");
	if(!font_load(2, "data/sprites/font3", packfile)) shutdown("Unable to load font #3!\n");
	if(!font_load(3, "data/sprites/font4", packfile)) shutdown("Unable to load font #4!\n");

	debug_printf("Timer init...\n");
	timer_init();

	if(savedata.usesound && sound_init(6)){
		debug_printf("Soundcard initialized.\n");
		if(!sound_start_playback(0,savedata.soundbits,savedata.soundrate)){
			debug_printf("Warning: can't play sound at %u Hz!\n", savedata.soundrate);
		}
//		SB_setvolume(SB_MASTERVOL, 15);
		SB_setvolume(SB_VOICEVOL, savedata.soundvol);

		debug_printf("Loading sounds...\n");
		smp_beat = loadcache_sound("data/sounds/beat1.wav");
		smp_fall = loadcache_sound("data/sounds/fall.wav");
		smp_get =  loadcache_sound("data/sounds/get.wav");
		smp_get2 = loadcache_sound("data/sounds/money.wav");
		smp_jump = loadcache_sound("data/sounds/jump.wav");
		smp_indirect = loadcache_sound("data/sounds/indirect.wav");
		smp_punch = loadcache_sound("data/sounds/punch.wav");
		smp_1up = loadcache_sound("data/sounds/1up.wav");
		smp_timeover = loadcache_sound("data/sounds/timeover.wav");
		smp_beep = loadcache_sound("data/sounds/beep.wav");
		smp_beep2 = loadcache_sound("data/sounds/beep2.wav");
		smp_bike = loadcache_sound("data/sounds/bike.wav");
	}
	else{
		debug_printf("Sound disabled.\n");
	}

	debug_printf("Object engine init...\n");
	if(!alloc_ents()){
		shutdown("Not enough memory for game objects!\n");
	}

	debug_printf("Loading sprites...\n");
	shadowsprites[0] = loadsprite("data/sprites/shadow1",9,3);
	shadowsprites[1] = loadsprite("data/sprites/shadow2",14,5);
	shadowsprites[2] = loadsprite("data/sprites/shadow3",19,6);
	shadowsprites[3] = loadsprite("data/sprites/shadow4",24,8);
	shadowsprites[4] = loadsprite("data/sprites/shadow5",29,9);
	shadowsprites[5] = loadsprite("data/sprites/shadow6",34,11);
	gosprite = loadsprite("data/sprites/arrow",0,0);
	holesprite = loadsprite("data/sprites/hole",0,0);

	debug_printf("Loading models...\n");
	load_models();

	debug_printf("Loading level order...\n");
	load_levelorder();


	debug_printf("Input init...\n");
	control_init(savedata.usejoy);

	apply_controls();

	debug_printf("Video mode...\n");
	if(rescreen){
		if(!video_set_mode(rescreen->width, rescreen->height)){
			shutdown("Unable to set video mode: %ix%i!\n", rescreen->width, rescreen->height);
		}
	}
	else if(!video_set_mode(320, 240)){
		shutdown("Unable to set video mode: 320x240!\n");
	}

	for(i=0; i<256; i++) neontable[i] = i;
}






// ----------------------------------------------------------------------------


// Returns 0 on error, -1 on escape
int playgif(char *filename, int x, int y){
	int code;
	int delay;
	unsigned gamelib_long milliseconds;
	unsigned gamelib_long nextframe;
	unsigned gamelib_long lasttime;
	int done;
	int frame = 0;
	int synctosound = 0;


	// Clear background
	unload_background();
	background = allocscreen(320, 240);
	if(background==NULL) shutdown("Out of memory!");
	clearscreen(background);

	if(!anigif_open(filename, packfile, pal)) return 0;

	bortime = 0;
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
			pal[0] = pal[1] = pal[2] = 0;
			palette_set_corrected(pal, savedata.gamma,savedata.gamma,savedata.gamma, savedata.brightness,savedata.brightness,savedata.brightness);
			update(0,0);
		}
		else update(0,1);
	
		++frame;

		if(synctosound){
			milliseconds += sound_getinterval();
			if(milliseconds==0xFFFFFFFF) synctosound = 0;
		}
		if(!synctosound) milliseconds += (bortime-lasttime) * 1000 / GAME_SPEED;
		lasttime = bortime;

		if(bothnewkeys & (FLAG_ESC | FLAG_ANYBUTTON)) done = 1;
	}
	anigif_close();
	if(bothnewkeys & (FLAG_ESC | FLAG_ANYBUTTON)) return -1;
	return 1;
}



void playscene(char *filename){

	int handle;
	char *buf;
	unsigned int size;
	int pos;
	char * command = NULL;

	char giffile[256];
//	int silence = 0;
	int x=0, y=0;

	int closing = 0;


	// Read file
	if((handle=openpackfile(filename,packfile)) < 0) return;
	size = seekpackfile(handle,0,SEEK_END);
	seekpackfile(handle,0,SEEK_SET);

	buf = (char*)tracemalloc("playscene", size+1);
	if(buf==NULL){
		closepackfile(handle);
		return;
	}
	if(readpackfile(handle, buf, size) != size){
		tracefree(buf);
		closepackfile(handle);
		return;
	}
	buf[size] = 0;		// Terminate string (important!)
	closepackfile(handle);


	// Now interpret the contents of buf line by line
	pos = 0;
	while(buf[pos]){
		command = findarg(buf+pos, 0);
		if(command[0]){
			if(!closing && stricmp(command, "music")==0){
				music(findarg(buf+pos, 1), atoi(findarg(buf+pos, 2)));
			}
			else if(!closing && stricmp(command, "animation")==0){
				strcpy(giffile, findarg(buf+pos, 1));
				x = atoi(findarg(buf+pos, 2));
				y = atoi(findarg(buf+pos, 3));
				if(playgif(giffile, x, y) == -1) closing = 1;
			}
			else if(stricmp(command, "silence")==0){
				sound_close_music();
			}
		}
		// Go to next non-blank line
		while(buf[pos] && buf[pos]!='\n' && buf[pos]!='\r') ++pos;
		while(buf[pos]=='\n' || buf[pos]=='\r') ++pos;
	}
	tracefree(buf);
}




// ----------------------------------------------------------------------------




void gameover(){

	int done = 0;

	unload_background();
	background = allocscreen(320, 240);
	if(background==NULL) shutdown("Out of memory!");
	clearscreen(background);

	music("data/music/gameover.bor", 0);

	bortime = 0;
	while(!done){
		font_printf(113,110, 3, "GAME OVER");
		update(0,0);
		done |= (bortime>GAME_SPEED*8 && !sound_query_music(NULL,NULL));
// TODO: was bug? had no parens around the two flags
		done |= (bothnewkeys & (FLAG_START+FLAG_ESC));
	}

	freescreen(background);
	background = NULL;
}



// Level completed, show bonus stuff
void showcomplete(int num){
	int done = 0;
	int clearbonus[2] = { 10000, 10000 };
	int lifebonus[2];
//	int scores[2];
	int i;
	unsigned gamelib_long nexttime = 0;
	unsigned gamelib_long finishtime = 0;
	int chan = 0;
	char t[256];

	unload_background();

	background = allocscreen(320, 240);
	if(background==NULL) shutdown("Out of memory!");
	clearscreen(background);

	music("data/music/complete.bor", 0);

	lifebonus[0] = player[0].lives * 1000;
	lifebonus[1] = player[1].lives * 1000;

	update(0,0);

	bortime = 0;
	while(!done){
		sprintf(t, "Stage %i complete!", num);
		font_printf(75,60, 3, t);

		font_printf(10,100, 0, "Clear bonus");
		sprintf(t, "%i", clearbonus[0]);
		if(player[0].lives > 0) font_printf(100,100, 0, t);
		sprintf(t, "%i", clearbonus[1]);
		if(player[1].lives > 0) font_printf(200,100, 0, t);

		font_printf(10,120, 0, "Life bonus");
		sprintf(t, "%i", lifebonus[0]);
		if(player[0].lives > 0) font_printf(100,120, 0, t);
		sprintf(t, "%i", lifebonus[1]);
		if(player[1].lives > 0) font_printf(200,120, 0, t);

		font_printf(10,140, 0, "Total score");
		sprintf(t, "%i", player[0].score);
		if(player[0].lives > 0) font_printf(100,140, 0, t);
		sprintf(t, "%i", player[1].score);
		if(player[1].lives > 0) font_printf(200,140, 0, t);

		while(bortime > nexttime){
			if(!finishtime)	finishtime = bortime + 4 * GAME_SPEED;
			for(i=0; i<2; i++){
				if(clearbonus[i] > 0){
					addscore(i, 10);
					clearbonus[i] -= 10;
					finishtime = 0;
				}
				else if(lifebonus[i] > 0){
					addscore(i, 10);
					lifebonus[i] -= 10;
					finishtime = 0;
				}
			}

			if(!finishtime && !(nexttime&15)){
				sound_stop_sample(chan);
				chan = sound_play_sample(smp_beep, 0, savedata.effectvol/2,savedata.effectvol/2, 100);
			}

			nexttime++;
		}

		if(bothnewkeys & (FLAG_ANYBUTTON|FLAG_ESC)) done = 1;
		if(finishtime && bortime>finishtime) done = 1;

		update(0,0);
	}

	// Add remainder of score, incase player skips counter
	for(i=0; i<2; i++){
		addscore(i, clearbonus[i]);
		addscore(i, lifebonus[i]);
	}

	freescreen(background);
	background = NULL;
}




int playlevel(char *filename){

	kill_all();
	load_level(filename);
	bortime = 0;

	if(player[0].lives > 0) spawnplayer(0);
	if(player[1].lives > 0) spawnplayer(1);

	while(!endgame){
		update(1,0);
		if(level_completed) endgame |= (!findent(TYPE_ENEMY));
	}
	fade_out();

	if(player[0].ent) player[0].spawnhealth = player[0].ent->health;
	if(player[1].ent) player[1].spawnhealth = player[1].ent->health;

	sound_close_music();
	kill_all();
	unload_level();

	return (player[0].lives > 0 || player[1].lives > 0);
}




int selectplayer(int p1on, int p2on){

	int i;
	int exit = 0;
	int ready[2] = { 0, 0 };
	int escape = 0;
	entity * example[2] = { NULL, NULL };
	int players_busy = 0;
	int players_ready = 0;
	int immediate[2];


	load_background("data/bgs/select", 1);
	kill_all();

	music("data/music/menu.bor", 1);

	memset(&player[0], 0, sizeof(s_player));
	memset(&player[1], 0, sizeof(s_player));
	credits = cheat_mode ? 99 : 4;

	immediate[0] = p1on;
	immediate[1] = p2on;

	while(!(exit || escape)){
		players_busy = 0;
		players_ready = 0;
		for(i=0; i<2; i++){
			if(!ready[i]){
				if(player[i].lives <= 0 && (player[i].newkeys || immediate[i])){
					if (!cheat_mode)
						--credits;
					player[i].lives = 3;
					example[i] = spawn(83+(i*155),230,0, nextplayermodel(NULL)->name);
					if(example[i]==NULL) shutdown("Failed to create player selection object!");
					example[i]->direction = !i;
					ent_set_colourmap(example[i], i);
					sound_play_sample(smp_beep, 0, savedata.effectvol,savedata.effectvol, 100);
				}
				else if(player[i].newkeys & FLAG_MOVELEFT){
					sound_play_sample(smp_beep, 0, savedata.effectvol,savedata.effectvol, 100);
					ent_set_model(example[i], prevplayermodel(example[i]->model)->name);
					ent_set_colourmap(example[i], i);
				}
				else if(player[i].newkeys & FLAG_MOVERIGHT){
					sound_play_sample(smp_beep, 0, savedata.effectvol,savedata.effectvol, 100);
					ent_set_model(example[i], nextplayermodel(example[i]->model)->name);
					ent_set_colourmap(example[i], i);
				}
				else if(player[i].newkeys & FLAG_ANYBUTTON){
					sound_play_sample(smp_beep2, 0, savedata.effectvol,savedata.effectvol, 100);
					ready[i] = 1;
					player[i].model = example[i]->model;
					player[i].colourmap = i;
				}
			}
			else { char t[256]; sprintf(t, "Player %i ready!", i+1); font_printf(53+(i*152),173,0,t); }

			if(example[i] != NULL) players_busy++;
			if(ready[i]) players_ready++;
		}

		if(players_busy && players_busy==players_ready) exit = 1;

		update(0,0);

		if(bothnewkeys & FLAG_ESC) escape = 1;
	}
	kill_all();
	sound_close_music();

	return (!escape);
}




void playgame(int p1on, int p2on, unsigned which_set){
	int current_level;
	int stage;

	if(which_set>=num_difficulties) return;
	// shutdown("Illegal set chosen: index %i (there are only %i sets)!", which_set, num_difficulties);

	allow_secret_chars = ifcomplete[which_set];

	if(selectplayer(p1on, p2on)){
		current_level = 0;
		stage = 1;
		while(current_level < num_levels[which_set]){
			if(levelorder[which_set][current_level]->is_scene){
				playscene(levelorder[which_set][current_level]->filename);
			}
			else if(!playlevel(levelorder[which_set][current_level]->filename)){
				if(player[0].lives <= 0 && player[1].lives <= 0) gameover();
				break;
			}
			if(levelorder[which_set][current_level]->gonext){
				showcomplete(stage);
				player[0].spawnhealth = 0;
				player[1].spawnhealth = 0;
				++stage;
			}
			++current_level;
		}
		if(current_level >= num_levels[which_set]){
			savedata.times_completed++;
			fade_out();
		}
	}
	sound_close_music();
}





int choose_difficulty(){
	int quit = 0;
	int selector = 0;
	int i;
	char t[256];

	bothnewkeys = 0;

	while(!quit){
		font_printf(120,140, 2, "Difficulty");
		for(i=0; i<num_difficulties; i++){
			if(savedata.times_completed >= ifcomplete[i]) font_printf(120,160+i*10, (selector==i), set_names[i]);
			else{
				sprintf(t, "%s - finish game %i times to unlock", set_names[i], ifcomplete[i]);
				if(ifcomplete[i]>1) font_printf(120,160+i*10, (selector==i), t);
				font_printf(120,160+i*10, (selector==i), "%s - finish game to unlock", set_names[i]);
			}
		}
		font_printf(120,165+i*10, (selector==i), "Back");
		update(0,0);

		if(bothnewkeys & FLAG_ESC) quit = 1;
		if(bothnewkeys & FLAG_MOVEUP){
			--selector;
			sound_play_sample(smp_beep, 0, savedata.effectvol,savedata.effectvol, 100);
		}
		if(bothnewkeys & FLAG_MOVEDOWN){
			++selector;
			sound_play_sample(smp_beep, 0, savedata.effectvol,savedata.effectvol, 100);
		}
		if(selector<0) selector = i;
		if(selector>i) selector = 0;
		if(bothnewkeys & FLAG_ANYBUTTON){
			sound_play_sample(smp_beep2, 0, savedata.effectvol,savedata.effectvol, 100);
			if(selector==i) quit = 1;
			else if(savedata.times_completed >= ifcomplete[selector]) return selector;
		}
	}
	bothnewkeys = 0;
	return -1;
}




// ----------------------------------------------------------------------------


void soundcard_options(){
	int quit = 0;
	int selector = 0;

	savesettings();

	bothnewkeys = 0;

	while(!quit){
		font_printf(60,90, 2, "Sound card options");
		font_printf(60,120, (selector==0), "Frequency: %i", savedata.soundrate);
		font_printf(60,130, (selector==1), "Bits: %i", savedata.soundbits);
		font_printf(60,150, (selector==2), "Apply");
		font_printf(60,160, (selector==3), "Discard");
		font_printf(60,210, (selector==4), "Back");
		update(0,0);

		if(bothnewkeys & FLAG_ESC) quit = 1;
		if(bothnewkeys & FLAG_MOVEUP){
			--selector;
			sound_play_sample(smp_beep, 0, savedata.effectvol,savedata.effectvol, 100);
		}
		if(bothnewkeys & FLAG_MOVEDOWN){
			++selector;
			sound_play_sample(smp_beep, 0, savedata.effectvol,savedata.effectvol, 100);
		}
		if(selector<0) selector = 4;
		selector %= 5;
		if(bothnewkeys & (FLAG_MOVELEFT|FLAG_MOVERIGHT|FLAG_ANYBUTTON)){
			sound_play_sample(smp_beep2, 0, savedata.effectvol,savedata.effectvol, 100);
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
					if(!sound_start_playback(0, savedata.soundbits, savedata.soundrate)){
						savedata.soundbits = 8;
						savedata.soundrate = 11025;
						sound_start_playback(0, savedata.soundbits, savedata.soundrate);
					}
					music("data/music/remix.bor", 1);
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



// Set key or button safely (with switching)
void safe_set(int *arr, int index, int newkey, int oldkey){
	int i;

	for(i=0; i<9; i++){
		if(arr[i]==newkey) arr[i] = oldkey;
	}

	arr[index] = newkey;
}


void keyboard_setup(int player){
	int quit = 0;
	int selector = 0;
	int setting = -1;
	int k, ok = 0;
	char t[256];

	savesettings();

	bothnewkeys = 0;

	while(!quit){
		sprintf(t, "Setup controls for player %i", player+1);
		font_printf(60,90, 2, t);
		font_printf(60,110, (selector==0), "Move up");
		font_printf(160,110, (selector==0), control_getkeyname(savedata.keys[player][SDID_MOVEUP]));
		font_printf(60,120, (selector==1), "Move down");
		font_printf(160,120, (selector==1), control_getkeyname(savedata.keys[player][SDID_MOVEDOWN]));
		font_printf(60,130, (selector==2), "Move left");
		font_printf(160,130, (selector==2), control_getkeyname(savedata.keys[player][SDID_MOVELEFT]));
		font_printf(60,140, (selector==3), "Move right");
		font_printf(160,140, (selector==3), control_getkeyname(savedata.keys[player][SDID_MOVERIGHT]));
		font_printf(60,150, (selector==4), "Special");
		font_printf(160,150, (selector==4), control_getkeyname(savedata.keys[player][SDID_SPECIAL]));
		font_printf(60,160, (selector==5), "Attack");
		font_printf(160,160, (selector==5), control_getkeyname(savedata.keys[player][SDID_ATTACK]));
		font_printf(60,170, (selector==6), "Jump");
		font_printf(160,170, (selector==6), control_getkeyname(savedata.keys[player][SDID_JUMP]));
		font_printf(60,180, (selector==7), "Start");
		font_printf(160,180, (selector==7), control_getkeyname(savedata.keys[player][SDID_START]));
//		font_printf(60,190, (selector==8), "Screenshot");
//		font_printf(160,190, (selector==8), control_getkeyname(savedata.keys[player][SDID_SCREENSHOT]));
		font_printf(60,210, (selector==9), "OK");
		font_printf(60,220, (selector==10), "Cancel");
		update(0,0);


		if(setting > -1){
			if(bothnewkeys & FLAG_ESC){
				savedata.keys[player][setting] = ok;
				sound_play_sample(smp_beep2, 0, savedata.effectvol,savedata.effectvol, 50);
				setting = -1;
			}
			if(setting > -1){
// TODO: was bug?
				if((k = control_scankey())){
					safe_set(savedata.keys[player], setting, k, ok);
					sound_play_sample(smp_beep2, 0, savedata.effectvol,savedata.effectvol, 100);
					setting = -1;
					// Prevent accidental screenshot
					bothnewkeys = 0;
				}
			}
		}
		else{
			if(bothnewkeys & FLAG_ESC) quit = 1;
			if(bothnewkeys & FLAG_MOVEUP){
				--selector; if(selector==8)selector--;
				sound_play_sample(smp_beep, 0, savedata.effectvol,savedata.effectvol, 100);
			}
			if(bothnewkeys & FLAG_MOVEDOWN){
				++selector;if(selector==8)selector++;
				sound_play_sample(smp_beep, 0, savedata.effectvol,savedata.effectvol, 100);
			}
			if(selector<0) selector = 10;
			if(selector>10) selector = 0;

			if(bothnewkeys & FLAG_ANYBUTTON){
				sound_play_sample(smp_beep2, 0, savedata.effectvol,savedata.effectvol, 100);
				if(selector==9) quit = 2;
				else if(selector==10) quit = 1;
				else{
					setting = selector;
					ok = savedata.keys[player][setting];
					savedata.keys[player][setting] = 0;
//					keyboard_getlastkey();
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
}







void input_options(){
	int quit = 0;
	int selector = 1; // 0

	bothnewkeys = 0;

	while(!quit){
		font_printf(60,90, 2, "Options");
/*
		if(savedata.usejoy){
			font_printf(60,120, (selector==0), "Gamepad enabled");
			if(!control_getjoyenabled()){
				font_printf(160,120, (selector==0), "* device is not ready");
				// control_usejoy(1);
			}
		}
		else font_printf(60,120, (selector==0), "Gamepad disabled");
*/
		font_printf(60,120, (selector==1), "Setup controls P1...");
		font_printf(60,130, (selector==2), "Setup controls P2...");
		font_printf(60,210, (selector==3), "Back");
		update(0,0);

		if(bothnewkeys & FLAG_ESC) quit = 1;
		if(bothnewkeys & FLAG_MOVEUP){
			--selector;
			if(selector==0)selector=3;
			sound_play_sample(smp_beep, 0, savedata.effectvol,savedata.effectvol, 100);
		}
		if(bothnewkeys & FLAG_MOVEDOWN){
			++selector;
			if(selector>3)selector=1;
			sound_play_sample(smp_beep, 0, savedata.effectvol,savedata.effectvol, 100);
		}
		if(selector<0) selector = 3;
		if(selector>3) selector = 0;
		if(bothnewkeys & (FLAG_MOVELEFT|FLAG_MOVERIGHT|FLAG_ANYBUTTON)){
			sound_play_sample(smp_beep2, 0, savedata.effectvol,savedata.effectvol, 100);
			switch(selector){
				case 0:
					savedata.usejoy = !savedata.usejoy;
					control_usejoy(savedata.usejoy);
					break;
				case 1:
					keyboard_setup(0);
					break;
				case 2:
					keyboard_setup(1);
					break;
				default:
					quit = (bothnewkeys & FLAG_ANYBUTTON);
			}
		}
	}
	savesettings();
	bothnewkeys = 0;
}



void soundvol_options(){

	int quit = 0;
	int selector = 0;
	int dir;
	char t[256];

	bothnewkeys = 0;

	while(!quit){
		font_printf(60,90, 2, "Options");

		font_printf(60,120, (selector==0), "Sound volume:");
		sprintf(t, "%i", savedata.soundvol);
		font_printf(160,120, (selector==0), t);
		font_printf(60,130, (selector==1), "Sound effects volume:");
		sprintf(t, "%i", savedata.effectvol);
		font_printf(160,130, (selector==1), t);
		font_printf(60,140, (selector==2), "Background music:");
		font_printf(160,140, (selector==2), (savedata.usemusic ? "Enabled" : "Disabled"));
		font_printf(60,150, (selector==3), "Music volume:");
		sprintf(t, "%i", savedata.musicvol);
		font_printf(160,150, (selector==3), t);
		font_printf(60,160, (selector==4), "Show music titles:");
		font_printf(160,160, (selector==4), (savedata.showtitles ? "Yes" : "No"));
		font_printf(60,210, (selector==5), "Back");

		update(0,0);

		if(bothnewkeys & FLAG_ESC) quit = 1;
		if(bothnewkeys & FLAG_MOVEUP){
			--selector;
			sound_play_sample(smp_beep, 0, savedata.effectvol,savedata.effectvol, 100);
		}
		if(bothnewkeys & FLAG_MOVEDOWN){
			++selector;
			sound_play_sample(smp_beep, 0, savedata.effectvol,savedata.effectvol, 100);
		}
		if(selector<0) selector = 5;
		if(selector>5) selector = 0;

		if(bothnewkeys & (FLAG_MOVELEFT|FLAG_MOVERIGHT|FLAG_ANYBUTTON)){
			dir = 0;
			if(bothnewkeys & FLAG_MOVELEFT) dir = -1;
			else if(bothnewkeys & FLAG_MOVERIGHT) dir = 1;
			sound_play_sample(smp_beep2, 0, savedata.effectvol,savedata.effectvol, 100);
			switch(selector){
				case 0:
					savedata.soundvol += dir;
					if(savedata.soundvol < 0) savedata.soundvol = 0;
					if(savedata.soundvol > 15) savedata.soundvol = 15;
					SB_setvolume(SB_VOICEVOL, savedata.soundvol);
					break;
				case 1:
					savedata.effectvol += 8*dir;
					if(savedata.effectvol < 0) savedata.effectvol = 0;
					if(savedata.effectvol > 512) savedata.effectvol = 512;
					break;
				case 2:
					if(!dir) break;
					if(!savedata.usemusic){
						savedata.usemusic = 1;
						music("data/music/remix.bor", 1);
					}
					else{
						savedata.usemusic = 0;
						sound_close_music();
					}
					break;
				case 3:
					savedata.musicvol += 8*dir;
					if(savedata.musicvol < 0) savedata.musicvol = 0;
					if(savedata.musicvol > 512) savedata.musicvol = 512;
					sound_volume_music(savedata.musicvol, savedata.musicvol);
					break;
				case 4:
					savedata.showtitles = !savedata.showtitles;
					break;
				default:
					quit = 1;
			}
		}
	}
	savesettings();
	bothnewkeys = 0;
}

void tv_options(){
	int quit = 0;
	int selector = 0;
	int dir;
	char t[20];

	bothnewkeys = 0;

	while(!quit){
		font_printf(60,90, 2, "TV Options");

		font_printf(60,120, (selector==0), "X Offset:");
		sprintf(t, "%i", savedata.offset_X);
		font_printf(160,120, (selector==0), t);
		font_printf(60,130, (selector==1), "Y Offset:");
		sprintf(t, "%i", savedata.offset_Y);
		font_printf(160,130, (selector==1), t);
		font_printf(60,140, (selector==2), "Interlaced:");
		font_printf(160,140, (selector==2), "%s", savedata.interlace ? "Yes" : "No");
		font_printf(60,150, (selector==3), "Mode:");
		font_printf(160,150, (selector==3), "%s", savedata.videomode ? "Frame" : "Field");
		font_printf(60,210, (selector==4), "Back");

		update(0,0);

		if(bothnewkeys & FLAG_ESC) quit = 1;
		if(bothnewkeys & FLAG_MOVEUP){
			--selector;
			sound_play_sample(smp_beep, 0, savedata.effectvol,savedata.effectvol, 100);
		}
		if(bothnewkeys & FLAG_MOVEDOWN){
			++selector;
			sound_play_sample(smp_beep, 0, savedata.effectvol,savedata.effectvol, 100);
		}
		if(selector<0) selector = 4;
		if(selector>4) selector = 0;

		if(bothnewkeys & (FLAG_MOVELEFT|FLAG_MOVERIGHT|FLAG_ANYBUTTON)){
			dir = 0;
			if(bothnewkeys & FLAG_MOVELEFT) dir = -1;
			else if(bothnewkeys & FLAG_MOVERIGHT) dir = 1;
			sound_play_sample(smp_beep2, 0, savedata.effectvol,savedata.effectvol, 100);
			switch(selector){
				case 0:
					savedata.offset_X += dir;
					break;
				case 1:
					savedata.offset_Y += dir;
					break;
				case 2:
					if (!dir) break;
					savedata.interlace ^= 1;
					break;
				case 3:
					if (!dir) break;
					savedata.videomode ^= 1;
					break;
				default:
					quit = 1;
			}
			gsvga_tweak(savedata.offset_X, savedata.offset_Y, savedata.interlace, savedata.videomode);
		}
	}
	savesettings();
	bothnewkeys = 0;
}



void options(){
	int quit = 0;
	int selector = 0;
	int dir;
	char t[256];

	bothnewkeys = 0;

	while(!quit){
		font_printf(60,90, 2, "Options");
		font_printf(60,120, (selector==0), "Brightness:");
		sprintf(t, "%i", savedata.brightness);
		font_printf(160,120, (selector==0), t);
		font_printf(60,130, (selector==1), "Gamma:");
		sprintf(t, "%i", savedata.gamma);
		font_printf(160,130, (selector==1), t);
		font_printf(60,140, (selector==2), "Setup controls...");
//		font_printf(60,150, (selector==3), "Setup sound card...");
		font_printf(60,150, (selector==4), "Sound options...");
		font_printf(60,160, (selector==5), "TV options...");
		font_printf(60,210, (selector==6), "Back");

		update(0,0);

		if(bothnewkeys & FLAG_ESC) quit = 1;
		if(bothnewkeys & FLAG_MOVEUP){
			--selector;
			if(selector==3)selector--;
			sound_play_sample(smp_beep, 0, savedata.effectvol,savedata.effectvol, 100);
		}
		if(bothnewkeys & FLAG_MOVEDOWN){
			++selector;
			if(selector==3)selector++;
			sound_play_sample(smp_beep, 0, savedata.effectvol,savedata.effectvol, 100);
		}
		if(selector<0) selector = 6;
		if(selector>6) selector = 0;

		if(bothnewkeys & (FLAG_MOVELEFT|FLAG_MOVERIGHT|FLAG_ANYBUTTON)){
			dir = 0;
			if(bothnewkeys & FLAG_MOVELEFT) dir = -1;
			else if(bothnewkeys & FLAG_MOVERIGHT) dir = 1;
			sound_play_sample(smp_beep2, 0, savedata.effectvol,savedata.effectvol, 100);
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
					soundcard_options();
					break;
				case 4:
					soundvol_options();
					break;
				case 5:
					tv_options();
					break;
				default:
					quit = 1;
			}
		}
	}
	savesettings();
	bothnewkeys = 0;
}







// ----------------------------------------------------------------------------


void bor_main(int argc, char **argv){
	int quit = 0;
	int relback = 1;
	int selector = 0;
	int hres, vres;
	unsigned gamelib_long introtime = 0;
	int started = 0;
//	int diff;
	int i;



	// TEST
	for(i=1; i<argc; i++){
		if(stricmp(argv[i], "-res")==0 && argc>=i+3){
			hres = atoi(argv[i+1]);
			vres = atoi(argv[i+2]);
			if((rescreen = allocscreen(hres, vres)) == NULL){
				debug_printf("Not enough memory for %ix%i mode!\n", hres, vres);
				exit(1);
			}
			i+=2;
		}
		else if(stricmp(argv[i], "-pak")==0 && argc>=i+2){
			packfile = argv[i+1];
			i++;
		}
		else{
			debug_printf(
				"Option '%s' not recognized or improperly used.\n"
				"\n"
				"Avaliable options:\n"
				"-pak [filename]        Use alternative PAK file\n"
				"-res [width] [height]  Use alternative resolution\n"
				"\n"
				"Example:\n"
				"bor.exe -pak custom.pak\n"
				"bor.exe -res 640 480\n"
				"\n",
				argv[i]
			);
			exit(1);
		}
	}


#ifndef PS2PORT
	if(!fileexists(packfile)){
		debug_printf(
			"\n\n\n"
			"FATAL: packfile '%s' not found.\n"
			"If you are trying to run this program from a ZIP file,\n"
			"think again and UNZIP it first, smartypants.\n"
			"\n"
			"Now press a key and try again.\n"
			"\n\n\n",
			packfile
		);
		while(kbhit()) getch();
		getch();
		while(kbhit()) getch();
		exit(1);
	}
#endif

	startup();

	load_background("data/bgs/logo", 0);
	while(bortime<GAME_SPEED*6 && !(bothnewkeys&(FLAG_ANYBUTTON|FLAG_ESC))) update(0,0);

	music("data/music/remix.bor", 1);
	playscene("data/scenes/logo.txt");

	while(!quit){
		if(bortime >= introtime){
			playscene("data/scenes/intro.txt");
			update(0,0);
			introtime = bortime + GAME_SPEED * 20;
			relback = 1;
			started = 0;
			cheat_cursor = 0;
		}

		if(bothnewkeys & FLAG_ESC) quit = 1;

		if(!started){
			if((bortime%GAME_SPEED) < (GAME_SPEED/2)) font_printf(129,160, 0, "PRESS START");
			if(bothnewkeys&(FLAG_START)){
				started = 1;
				relback = 1;
			}
			
			if ((cheat_buttons_queue[cheat_cursor] & bothnewkeys) && !cheat_mode) {
				cheat_cursor++;
				if (cheat_cursor == 10) {
					cheat_mode = 1;
					sound_play_sample(smp_1up, 0, savedata.effectvol,savedata.effectvol, 100);
				}
			} else if (bothnewkeys & FLAG_ANYTHING) {
				cheat_cursor = 0;
				if (bothnewkeys & cheat_buttons_queue[0])
				    cheat_cursor = 1;
			}
		}
		else{
			font_printf(120,160, (selector==0), "Start game");
			font_printf(120,170, (selector==1), "Options");
			font_printf(120,180, (selector==2), "How to play");

			if(bothnewkeys) introtime = bortime + GAME_SPEED * 20;

			if(bothnewkeys & FLAG_MOVEUP){
				--selector;if(selector<0)selector=2;
				sound_play_sample(smp_beep, 0, savedata.effectvol,savedata.effectvol, 100);
			}
			if(bothnewkeys & FLAG_MOVEDOWN){
				++selector;if(selector==3)selector=0;
				sound_play_sample(smp_beep, 0, savedata.effectvol,savedata.effectvol, 100);
			}
			if(selector<0) selector = 3;
			selector %= 4;

			if(bothnewkeys&(FLAG_ANYBUTTON)){
				sound_play_sample(smp_beep2, 0, savedata.effectvol,savedata.effectvol, 100);
				switch(selector){
				case 0:
					playgame(player[0].newkeys&(FLAG_ANYBUTTON), player[1].newkeys&(FLAG_ANYBUTTON), choose_difficulty());
					started = 0;
					relback = 1;
					break;
				case 1:
					options();
					break;
				case 2:
					playscene("data/scenes/howto.txt");
					relback = 1;
					break;
				default:
					quit = 1;
				}
				introtime = bortime + GAME_SPEED * 20;
			}
		}
		if(relback){
			if(started) load_background("data/bgs/titleb", 0);
			else load_background("data/bgs/title", 0);
			if(!sound_query_music(NULL,NULL)) music("data/music/remix.bor", 1);
			relback = 0;
		}
		update(0,0);

	}

	shutdown(
		"Beats of Rage V%X.%04X, compile date: " __DATE__ "\n"
		"Presented by Team Senile.\n"
		"Special thanks to SEGA and SNK.\n",
		VERSION>>16,
		VERSION&0xFFFF
	);
}
