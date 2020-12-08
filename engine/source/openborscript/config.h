/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 - 2017 OpenBOR Team
 */

#ifndef OPENBORSCRIPT_H
#define OPENBORSCRIPT_H

#include "Interpreter.h"
#include "pp_parser.h"

#include "axis.h"
#include "binding.h"
#include "drawmethod.h"
#include "recursive_damage.h"
#include "entity.h"

#define MAX_GLOBAL_VAR 2048
#define MAX_KEY_LEN    24

//osc
#define script_magic ((int)0x73636f)
//vlst
#define varlist_magic ((int)0x74736c76)



typedef enum
{
    //  Array keys for attack entity property.
    //  Damon V. Caskey
    //  2016-02-20

    // Used for varlist[KEY] when getting or setting
    // entity attack property.

    EP_ATTACK_AK_ENTITY,
    EP_ATTACK_AK_PROPERTY,
    EP_ATTACK_AK_ANIMATION,
    EP_ATTACK_AK_FRAME,
    EP_ATTACK_AK_INDEX

} ep_attack_array_key;

//This structure holds a named variable list (list)
// and an indexed list (vars).
typedef struct
{
    int magic;
    List *list;
    ScriptVariant *vars;
} Varlist;

typedef struct Script
{
    int magic;
    Interpreter *pinterpreter;
    char *comment; // debug purpose
    Varlist *varlist;
    int initialized;        //flag
    int interpreterowner;   //flag
} Script;

extern Varlist global_var_list;
extern List theFunctionList;
extern ScriptVariant *indexed_var_list;
extern int max_indexed_vars;
extern int max_entity_vars;
extern int max_script_vars;
extern int max_global_vars;
extern int no_nested_script;
extern int global_var_count;

//these functions can be used by openbor.c
void Varlist_Init(Varlist *varlist, int size);
void Varlist_Clear(Varlist *varlist);
void Varlist_Cleanup(Varlist *varlist);
int Varlist_SetByName(Varlist *varlist, char *theName, ScriptVariant *var);
#define Script_Set_Local_Variant(s, k, v) Varlist_SetByName((s)->varlist, (k), (v))
void Script_Global_Init();
void Script_Global_Clear();
void Script_Init(Script *pscript, char *theName, char *comment, int first);
Script *alloc_script();
void Script_Copy(Script *pdest, Script *psrc, int localclear);
int Script_IsInitialized(Script *pscript);
void Script_Clear(Script *pscript, int localclear);
int Script_AppendText(Script *pscript, char *text, char *path);
int Script_Compile(Script *pscript);
int Script_Execute(Script *pscript);
int Script_Save_Local_Variant(Script *cs, char *namelist[]);
void Script_Load_Local_Variant(Script *cs, int handle);

void Script_LoadSystemFunctions();
void *Script_GetStringMapFunction(void *functionRef);
int Script_MapStringConstants(Instruction *pInstruction);

#ifndef COMPILED_SCRIPT
int Script_Call(Script *pscript, char *method, ScriptVariant *pretvar);
#endif

HRESULT system_isempty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT system_exit(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT system_NULL(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT system_rand(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT system_srand(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT system_getglobalvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT system_setglobalvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT system_getlocalvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT system_setlocalvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT system_clearlocalvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT system_clearglobalvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT system_clearindexedvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT system_free(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT system_typeof(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

HRESULT math_sin(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT math_ssin(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT math_cos(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT math_scos(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT math_sqrt(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT math_pow(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT math_asin(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT math_acos(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT math_atan(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT math_trunc(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT math_round(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

HRESULT openbor_systemvariant(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_setsystemvariant(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_drawstring(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_drawstringtoscreen(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_drawsprite(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_drawspritetoscreen(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_log(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_drawbox(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_drawline(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_drawdot(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_drawboxtoscreen(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_drawlinetoscreen(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_drawdottoscreen(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_drawscreen(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

// Animation properties.
HRESULT openbor_changeplayerproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_changeentityproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_get_animation_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set_animation_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

// Attack properties
HRESULT openbor_get_attack_collection(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_get_attack_instance(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_get_attack_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set_attack_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

// Body collision (bbox) properties
HRESULT openbor_get_body_collision_collection(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_get_body_collision_instance(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_get_body_collision_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set_body_collision_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

// Entity collision (ebox) properties
HRESULT openbor_get_entity_collision_collection(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_get_entity_collision_instance(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_get_entity_collision_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set_entity_collision_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

HRESULT openbor_getplayerproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_getentityproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_clearspawnentry(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_setspawnentry(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_spawn(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_projectile(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_transconst(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_tossentity(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_playmusic(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_fademusic(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_setmusicvolume(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_setmusictempo(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_pausemusic(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_pausesamples(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_pausesample(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_querychannel(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_stopchannel(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_isactivesample(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_sampleid(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_playsample(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_loadsample(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_unloadsample(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_fadeout(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_playerkeys(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_changepalette(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_damageentity(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_getcomputeddamage(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_killentity(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_dograb(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_findtarget(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_checkrange(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_gettextobjproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_changetextobjproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_settextobj(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_cleartextobj(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_getlayerproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_changelayerproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

// 2017-04-25, DC
HRESULT openbor_get_level_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set_level_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_get_set_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set_set_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_get_set_handle(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

// 2017-04-27, DC - bglayer,fglayer, etc.
HRESULT openbor_get_layer_handle(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

HRESULT openbor_getlevelproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_changelevelproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_checkhole(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_checkholeindex(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_checkwall(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_checkwallindex(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_checkplatformbelow(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_checkplatformabove(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_checkplatformbetween(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_checkbasemap(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_checkbasemapindex(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_checkbase(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_generatebasemap(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

HRESULT openbor_openfilestream(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_getfilestreamline(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_getfilestreamargument(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_filestreamnextline(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_getfilestreamposition(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_setfilestreamposition(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

HRESULT openbor_filestreamappend(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_createfilestream(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_closefilestream(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_savefilestream(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

HRESULT openbor_getindexedvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_setindexedvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_getscriptvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_setscriptvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_getentityvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_setentityvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

HRESULT openbor_shutdown(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

HRESULT openbor_jumptobranch(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

HRESULT openbor_changelight(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_changeshadowcolor(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_bindentity(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

HRESULT openbor_array(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_size(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_get(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_delete(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_add(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_reset(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_next(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_previous(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_key(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_value(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_islast(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_isfirst(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

HRESULT openbor_allocscreen(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_clearscreen(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_setdrawmethod(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_updateframe(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_executeanimation(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_performattack(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_setidle(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_getentity(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

HRESULT openbor_loadmodel(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_unload_model(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_loadsprite(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_hallfame(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_menu_options(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_playwebm(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_playgif(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_openanigif(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_decodeanigif(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_getanigifinfo(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);


HRESULT openbor_strinfirst(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_strinlast(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_strleft(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_strlength(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_strwidth(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_strright(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_getmodelproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_changemodelproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_rgbcolor(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

HRESULT openbor_adjustwalkanimation(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_finditem(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_pickup(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_waypoints(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_changedrawmethod(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

HRESULT openbor_drawspriteq(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_clearspriteq(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_getgfxproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_getdrawmethod(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

HRESULT openbor_allocscript(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_loadscript(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_compilescript(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_executescript(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

HRESULT openbor_loadgamefile(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_finishlevel(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_gameover(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_gotomainmenu(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_playgame(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_getrecordingstatus(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_recordinputs(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_getsaveinfo(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

int mapstrings_animationproperty(ScriptVariant **varlist, int paramCount);
int mapstrings_systemvariant(ScriptVariant **varlist, int paramCount);
int mapstrings_entityproperty(ScriptVariant **varlist, int paramCount);
int mapstrings_playerproperty(ScriptVariant **varlist, int paramCount);
int mapstrings_setspawnentry(ScriptVariant **varlist, int paramCount);
int mapstrings_transconst(ScriptVariant **varlist, int paramCount);
int mapstrings_playerkeys(ScriptVariant **varlist, int paramCount);
int mapstrings_textobjproperty(ScriptVariant **varlist, int paramCount);
int mapstrings_layerproperty(ScriptVariant **varlist, int paramCount);
int mapstrings_drawmethodproperty(ScriptVariant **varlist, int paramCount);
int mapstrings_gfxproperty(ScriptVariant **varlist, int paramCount);
int mapstrings_levelproperty(ScriptVariant **varlist, int paramCount);

int mapstrings_attackproperty(ScriptVariant **varlist, int paramCount);


enum systemvariant_enum
{
    _sv_background,
    _sv_blockade,
    _sv_bossescount,
    _sv_branchname,
    _sv_cheats,
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
	_sv_drawmethod_default,
    _sv_effectvol,
    _sv_elapsed_time,
    _sv_ent_max,
    _sv_fps,
    _sv_freeram,
    _sv_game_paused,
    _sv_game_speed,
    _sv_game_time,
    _sv_gfx_x_offset,
    _sv_gfx_y_offset,
    _sv_gfx_y_offset_adj,
    _sv_hresolution,
    _sv_in_cheat_options,
    _sv_in_control_options,
    _sv_in_enginecreditsscreen,
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
	_sv_lasthit_attack,
	_sv_lasthit_attacker,
	_sv_lasthit_target,
	_sv_lasthita,
    _sv_lasthitc,
    _sv_lasthitt,
    _sv_lasthitx,
    _sv_lasthity,
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
    _sv_maxsoundchannels,
    _sv_models_cached,
    _sv_models_loaded,
    _sv_musicvol,
    _sv_nofadeout,
    _sv_nogameover,
    _sv_nohof,
    _sv_nojoin,
    _sv_nopause,
    _sv_nosave,
    _sv_noscreenshot,
    _sv_noshowcomplete,
    _sv_numbasemaps,
    _sv_numbosses,
    _sv_numholes,
    _sv_numlayers,
    _sv_numpalettes,
    _sv_numwalls,
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
    _sv_porting,
    _sv_sample_play_id,
    _sv_scrollmaxx,
    _sv_scrollmaxz,
    _sv_scrollminx,
    _sv_scrollminz,
    _sv_self,
    _sv_sets_count,
    _sv_shadowalpha,
    _sv_shadowcolor,
    _sv_shadowopacity,
    _sv_skiptoset,
    _sv_slowmotion,
    _sv_slowmotion_duration,
    _sv_smartbomber,
    _sv_soundvol,
    _sv_textbox,
    _sv_ticks,
    _sv_totalram,
    _sv_usedram,
    _sv_viewporth,
    _sv_viewportw,
    _sv_viewportx,
    _sv_viewporty,
    _sv_vresolution,
    _sv_vscreen,
    _sv_waiting,
    _sv_xpos,
    _sv_ypos,
    _sv_the_end
};

#endif
