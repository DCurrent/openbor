/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef OPENBORSCRIPT_H
#define OPENBORSCRIPT_H

#include "Interpreter.h"
#include "pp_parser.h"

#define MAX_GLOBAL_VAR 2048
#define MAX_KEY_LEN    24

typedef struct
{
	ScriptVariant value;
	char          key[MAX_KEY_LEN];
	struct Script* owner;
}s_variantnode;

//osc
#define script_magic ((int)0x73636f)

typedef struct Script
{
	int magic;
	Interpreter* pinterpreter;
	char* comment; // debug purpose
	ScriptVariant* vars;
	int initialized;        //flag
	int interpreterowner;   //flag
}Script;

extern s_variantnode** global_var_list;
extern List           theFunctionList;
extern ScriptVariant* indexed_var_list;
extern int            max_indexed_vars;
extern int            max_entity_vars;
extern int            max_script_vars;
extern int            max_global_vars;
extern int max_global_var_index ;
extern int global_var_count;

//these functions can be used by openbor.c
void Script_Global_Init();
void Script_Global_Clear();
ScriptVariant* Script_Get_Global_Variant(char* theName);
int Script_Set_Global_Variant(char* theName, ScriptVariant* var);
ScriptVariant* Script_Get_Local_Variant(Script* cs, char* theName);
int Script_Set_Local_Variant(Script* cs, char* theName, ScriptVariant* var);
void Script_Init(Script* pscript, char* theName, char* comment, int first);
Script* alloc_script();
void Script_Copy(Script* pdest, Script* psrc, int localclear);
int Script_IsInitialized(Script* pscript);
void Script_Clear(Script* pscript, int localclear);
int Script_AppendText(Script* pscript, char* text, char* path);
int Script_Compile(Script* pscript);
int Script_Execute(Script* pscript);

void Script_LoadSystemFunctions();

#ifndef COMPILED_SCRIPT
int Script_Call(Script* pscript, char* method, ScriptVariant* pretvar);
#endif

HRESULT system_isempty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT system_NULL(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT system_rand(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT system_maxglobalvarindex(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT system_getglobalvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT system_setglobalvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT system_getlocalvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT system_setlocalvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT system_clearlocalvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT system_clearglobalvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT system_clearindexedvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT system_free(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_systemvariant(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_changesystemvariant(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_drawstring(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_drawstringtoscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_drawsprite(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_drawspritetoscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_log(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_drawbox(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_drawline(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_drawdot(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_drawboxtoscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_drawlinetoscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_drawdottoscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_drawscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_changeplayerproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_changeentityproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_getplayerproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_getentityproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_clearspawnentry(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_setspawnentry(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_spawn(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_projectile(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_transconst(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_tossentity(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_playmusic(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_fademusic(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_setmusicvolume(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_setmusictempo(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_pausemusic(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_playsample(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_loadsample(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_unloadsample(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_fadeout(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_playerkeys(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_changepalette(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_damageentity(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_killentity(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_findtarget(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_checkrange(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_gettextobjproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_changetextobjproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_settextobj(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_cleartextobj(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_getlayerproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_changelayerproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_getlevelproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_changelevelproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_checkhole(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_checkwall(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_checkplatformbelow(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

HRESULT openbor_openfilestream(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_getfilestreamline(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_getfilestreamargument(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_filestreamnextline(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_getfilestreamposition(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_setfilestreamposition(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

HRESULT openbor_filestreamappend(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_createfilestream(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_closefilestream(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_savefilestream(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

HRESULT openbor_getindexedvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_setindexedvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_getscriptvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_setscriptvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_getentityvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_setentityvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

HRESULT openbor_shutdown(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

HRESULT openbor_jumptobranch(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

HRESULT openbor_changelight(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_changeshadowcolor(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_bindentity(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

HRESULT openbor_array(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_size(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_get(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_set(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

HRESULT openbor_allocscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_clearscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_setdrawmethod(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_updateframe(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_performattack(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_setidle(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_getentity(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

HRESULT openbor_loadmodel(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_loadsprite(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_options(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_playgif(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

HRESULT openbor_strinfirst(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_strinlast(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_strleft(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_strlength(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_strwidth(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_strright(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_getmodelproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_changemodelproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_rgbcolor(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_settexture(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_setvertex(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_trianglelist(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

HRESULT openbor_aicheckwarp(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_aichecklie(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_aicheckgrabbed(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_aicheckgrab(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_aicheckescape(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_aicheckbusy(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_aicheckattack(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_aicheckmove(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_aicheckjump(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_aicheckpathblocked(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

HRESULT openbor_adjustwalkanimation(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_finditem(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_pickup(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_waypoints(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_changedrawmethod(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

HRESULT openbor_drawspriteq(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_clearspriteq(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_getgfxproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

HRESULT openbor_allocscript(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_loadscript(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_compilescript(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
HRESULT openbor_executescript(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

void mapstrings_systemvariant(ScriptVariant** varlist, int paramCount);
void mapstrings_getentityproperty(ScriptVariant** varlist, int paramCount);
void mapstrings_changeentityproperty(ScriptVariant** varlist, int paramCount);
void mapstrings_playerproperty(ScriptVariant** varlist, int paramCount);
void mapstrings_setspawnentry(ScriptVariant** varlist, int paramCount);
void mapstrings_transconst(ScriptVariant** varlist, int paramCount);
void mapstrings_playerkeys(ScriptVariant** varlist, int paramCount);
void mapstrings_gettextobjproperty(ScriptVariant** varlist, int paramCount);
void mapstrings_changetextobjproperty(ScriptVariant** varlist, int paramCount);
void mapstrings_layerproperty(ScriptVariant** varlist, int paramCount);
void mapstrings_drawmethodproperty(ScriptVariant** varlist, int paramCount);
void mapstrings_gfxproperty(ScriptVariant** varlist, int paramCount);
void mapstrings_levelproperty(ScriptVariant** varlist, int paramCount);


enum systemvariant_enum
{
_sv_background,
_sv_blockade,
_sv_branchname,
_sv_count_enemies,
_sv_count_entities,
_sv_count_npcs,
_sv_count_players,
_sv_current_branch,
_sv_current_level,
_sv_current_palette,
_sv_current_scene,
_sv_current_set,
_sv_current_stage,
_sv_effectvol,
_sv_elapsed_time,
_sv_ent_max,
_sv_freeram,
_sv_game_paused,
_sv_game_speed,
_sv_gfx_x_offset,
_sv_gfx_y_offset,
_sv_gfx_y_offset_adj,
_sv_hResolution,
_sv_in_cheat_options,
_sv_in_control_options,
_sv_in_gameoverscreen,
_sv_in_halloffamescreen,
_sv_in_level,
_sv_in_load_game,
_sv_in_menuscreen,
_sv_in_new_game,
_sv_in_options,
_sv_in_selectscreen,
_sv_in_showcomplete,
_sv_in_sound_options,
_sv_in_start_game,
_sv_in_system_options,
_sv_in_titlescreen,
_sv_in_video_options,
_sv_lasthita,
_sv_lasthitc,
_sv_lasthitt,
_sv_lasthitx,
_sv_lasthitz,
_sv_levelheight,
_sv_levelpos,
_sv_levelwidth,
_sv_lightx,
_sv_lightz,
_sv_maxanimations,
_sv_maxattacktypes,
_sv_maxentityvars,
_sv_maxglobalvars,
_sv_maxindexedvars,
_sv_maxplayers,
_sv_maxscriptvars,
_sv_models_cached,
_sv_models_loaded,
_sv_musicvol,
_sv_nofadeout,
_sv_nojoin,
_sv_nopause,
_sv_nosave,
_sv_noscreenshot,
_sv_numpalettes,
_sv_pakname,
_sv_pause,
_sv_pixelformat,
_sv_player,
_sv_player1,
_sv_player2,
_sv_player3,
_sv_player4,
_sv_player_max_z,
_sv_player_min_z,
_sv_scrollmaxx,
_sv_scrollmaxz,
_sv_scrollminx,
_sv_scrollminz,
_sv_self,
_sv_shadowalpha,
_sv_shadowcolor,
_sv_skiptoset,
_sv_slowmotion,
_sv_slowmotion_duration,
_sv_smartbomber,
_sv_soundvol,
_sv_textbox,
_sv_ticks,
_sv_totalram,
_sv_usedram,
_sv_usesave,
_sv_vResolution,
_sv_viewporth,
_sv_viewportw,
_sv_viewportx,
_sv_viewporty,
_sv_vscreen,
_sv_waiting,
_sv_xpos,
_sv_ypos,
_sv_the_end,
 };

#endif
