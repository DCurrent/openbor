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

#include "animation.h"
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

// Sub entity properties.

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

int mapstrings_animation_property(ScriptVariant **varlist, int paramCount);
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
    SYSTEM_VARIANT_BACKGROUND,
    SYSTEM_VARIANT_BLOCKADE,
    SYSTEM_VARIANT_BOSSESCOUNT,
    SYSTEM_VARIANT_BRANCHNAME,
    SYSTEM_VARIANT_CHEATS,
    SYSTEM_VARIANT_COUNT_ENEMIES,
    SYSTEM_VARIANT_COUNT_ENTITIES,
    SYSTEM_VARIANT_COUNT_NPCS,
    SYSTEM_VARIANT_COUNT_PLAYERS,
    SYSTEM_VARIANT_CURRENT_BRANCH,
    SYSTEM_VARIANT_CURRENT_LEVEL,
    SYSTEM_VARIANT_CURRENT_PALETTE,
    SYSTEM_VARIANT_CURRENT_SCENE,
    SYSTEM_VARIANT_CURRENT_SET,
    SYSTEM_VARIANT_CURRENT_STAGE,
    SYSTEM_VARIANT_DRAWMETHOD_COMMON,
	SYSTEM_VARIANT_DRAWMETHOD_DEFAULT,    
    SYSTEM_VARIANT_EFFECTVOL,
    SYSTEM_VARIANT_ELAPSED_TIME,
    SYSTEM_VARIANT_ENT_MAX,
    SYSTEM_VARIANT_FPS,
    SYSTEM_VARIANT_FREERAM,
    SYSTEM_VARIANT_GAME_PAUSED,
    SYSTEM_VARIANT_GAME_SPEED,
    SYSTEM_VARIANT_GAME_TIME,
    SYSTEM_VARIANT_GFX_X_OFFSET,
    SYSTEM_VARIANT_GFX_Y_OFFSET,
    SYSTEM_VARIANT_GFX_Y_OFFSET_ADJ,
    SYSTEM_VARIANT_HRESOLUTION,
    SYSTEM_VARIANT_IN_CHEAT_OPTIONS,
    SYSTEM_VARIANT_IN_CONTROL_OPTIONS,
    SYSTEM_VARIANT_IN_ENGINECREDITSSCREEN,
    SYSTEM_VARIANT_IN_GAMEOVERSCREEN,
    SYSTEM_VARIANT_IN_HALLOFFAMESCREEN,
    SYSTEM_VARIANT_IN_LEVEL,
    SYSTEM_VARIANT_IN_LOAD_GAME,
    SYSTEM_VARIANT_IN_MENUSCREEN,
    SYSTEM_VARIANT_IN_NEW_GAME,
    SYSTEM_VARIANT_IN_OPTIONS,
    SYSTEM_VARIANT_IN_SELECTSCREEN,
    SYSTEM_VARIANT_IN_SHOWCOMPLETE,
    SYSTEM_VARIANT_IN_SOUND_OPTIONS,
    SYSTEM_VARIANT_IN_START_GAME,
    SYSTEM_VARIANT_IN_SYSTEM_OPTIONS,
    SYSTEM_VARIANT_IN_TITLESCREEN,
    SYSTEM_VARIANT_IN_VIDEO_OPTIONS,
	SYSTEM_VARIANT_LASTHIT_ATTACK,
	SYSTEM_VARIANT_LASTHIT_ATTACKER,
	SYSTEM_VARIANT_LASTHIT_TARGET,
	SYSTEM_VARIANT_LASTHITA,
    SYSTEM_VARIANT_LASTHITC,
    SYSTEM_VARIANT_LASTHITT,
    SYSTEM_VARIANT_LASTHITX,
    SYSTEM_VARIANT_LASTHITY,
    SYSTEM_VARIANT_LASTHITZ,
    SYSTEM_VARIANT_LEVELHEIGHT,
    SYSTEM_VARIANT_LEVELPOS,
    SYSTEM_VARIANT_LEVELWIDTH,
    SYSTEM_VARIANT_LIGHTX,
    SYSTEM_VARIANT_LIGHTZ,
    SYSTEM_VARIANT_MAXANIMATIONS,
    SYSTEM_VARIANT_MAXATTACKTYPES,
    SYSTEM_VARIANT_MAXENTITYVARS,
    SYSTEM_VARIANT_MAXGLOBALVARS,
    SYSTEM_VARIANT_MAXINDEXEDVARS,
    SYSTEM_VARIANT_MAXPLAYERS,
    SYSTEM_VARIANT_MAXSCRIPTVARS,
    SYSTEM_VARIANT_MAXSOUNDCHANNELS,
    SYSTEM_VARIANT_MODELS_CACHED,
    SYSTEM_VARIANT_MODELS_LOADED,
    SYSTEM_VARIANT_MUSICVOL,
    SYSTEM_VARIANT_NOFADEOUT,
    SYSTEM_VARIANT_NOGAMEOVER,
    SYSTEM_VARIANT_NOHOF,
    SYSTEM_VARIANT_NOJOIN,
    SYSTEM_VARIANT_NOPAUSE,
    SYSTEM_VARIANT_NOSAVE,
    SYSTEM_VARIANT_NOSCREENSHOT,
    SYSTEM_VARIANT_NOSHOWCOMPLETE,
    SYSTEM_VARIANT_NUMBASEMAPS,
    SYSTEM_VARIANT_NUMBOSSES,
    SYSTEM_VARIANT_NUMHOLES,
    SYSTEM_VARIANT_NUMLAYERS,
    SYSTEM_VARIANT_NUMPALETTES,
    SYSTEM_VARIANT_NUMWALLS,
    SYSTEM_VARIANT_PAKNAME,
    SYSTEM_VARIANT_PAUSE,
    SYSTEM_VARIANT_PIXELFORMAT,
    SYSTEM_VARIANT_PLAYER,
    SYSTEM_VARIANT_PLAYER1,
    SYSTEM_VARIANT_PLAYER2,
    SYSTEM_VARIANT_PLAYER3,
    SYSTEM_VARIANT_PLAYER4,
    SYSTEM_VARIANT_PLAYER_MAX_Z,
    SYSTEM_VARIANT_PLAYER_MIN_Z,
    SYSTEM_VARIANT_PORTING,
    SYSTEM_VARIANT_SAMPLE_PLAY_ID,
    SYSTEM_VARIANT_SCROLLMAXX,
    SYSTEM_VARIANT_SCROLLMAXZ,
    SYSTEM_VARIANT_SCROLLMINX,
    SYSTEM_VARIANT_SCROLLMINZ,
    SYSTEM_VARIANT_SELF,
    SYSTEM_VARIANT_SETS_COUNT,
    SYSTEM_VARIANT_SHADOWALPHA,
    SYSTEM_VARIANT_SHADOWCOLOR,
    SYSTEM_VARIANT_SHADOWOPACITY,
    SYSTEM_VARIANT_SKIPTOSET,
    SYSTEM_VARIANT_SLOWMOTION,
    SYSTEM_VARIANT_SLOWMOTION_DURATION,
    SYSTEM_VARIANT_SMARTBOMBER,
    SYSTEM_VARIANT_SOUNDVOL,
    SYSTEM_VARIANT_TEXTBOX,
    SYSTEM_VARIANT_TICKS,
    SYSTEM_VARIANT_TOTALRAM,
    SYSTEM_VARIANT_USEDRAM,
    SYSTEM_VARIANT_VIEWPORTH,
    SYSTEM_VARIANT_VIEWPORTW,
    SYSTEM_VARIANT_VIEWPORTX,
    SYSTEM_VARIANT_VIEWPORTY,
    SYSTEM_VARIANT_VRESOLUTION,
    SYSTEM_VARIANT_VSCREEN,
    SYSTEM_VARIANT_WAITING,
    SYSTEM_VARIANT_XPOS,
    SYSTEM_VARIANT_YPOS,
    SYSTEM_VARIANT_THE_END
};

#endif
