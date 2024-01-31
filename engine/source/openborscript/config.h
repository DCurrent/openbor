/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef OPENBORSCRIPT_H
#define OPENBORSCRIPT_H

#include "Interpreter.h"
#include "pp_parser.h"

#include "animation.h"
#include "audio.h"
#include "axis.h"
#include "binding.h"
#include "drawmethod.h"
#include "colorset.h"
#include "entity.h"
#include "faction.h"
#include "flash.h"
#include "global_config.h"
#include "icon.h"
#include "model.h"
#include "recursive_damage.h"
#include "spawn_hud.h"
#include "status_dial.h"


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
HRESULT system_string_to_float(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount);
HRESULT system_string_to_int(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount);

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
HRESULT openbor_isarray(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount);

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
    SYSTEM_PROPERTY_BACKGROUND,
    SYSTEM_PROPERTY_BACKGROUND_HEIGHT,
    SYSTEM_PROPERTY_BLOCKADE,
    SYSTEM_PROPERTY_BOSSESCOUNT,
    SYSTEM_PROPERTY_BRANCHNAME,
    SYSTEM_PROPERTY_COUNT_ENEMIES,
    SYSTEM_PROPERTY_COUNT_ENTITIES,
    SYSTEM_PROPERTY_COUNT_NPCS,
    SYSTEM_PROPERTY_COUNT_PLAYERS,
    SYSTEM_PROPERTY_CURRENT_BRANCH,
    SYSTEM_PROPERTY_CURRENT_LEVEL,
    SYSTEM_PROPERTY_CURRENT_PALETTE,
    SYSTEM_PROPERTY_CURRENT_SCENE,
    SYSTEM_PROPERTY_CURRENT_SET,
    SYSTEM_PROPERTY_CURRENT_STAGE,
    SYSTEM_PROPERTY_DRAWMETHOD_COMMON,
	SYSTEM_PROPERTY_DRAWMETHOD_DEFAULT,
    SYSTEM_PROPERTY_EFFECTVOL,
    SYSTEM_PROPERTY_ELAPSED_TIME,
    SYSTEM_PROPERTY_ENT_MAX,
    SYSTEM_PROPERTY_FPS,
    SYSTEM_PROPERTY_FREERAM,
    SYSTEM_PROPERTY_FRONT_PANEL_Z,
    SYSTEM_PROPERTY_GAME_PAUSED,
    SYSTEM_PROPERTY_GAME_SPEED,
    SYSTEM_PROPERTY_GAME_TIME,
    SYSTEM_PROPERTY_GFX_X_OFFSET,
    SYSTEM_PROPERTY_GFX_Y_OFFSET,
    SYSTEM_PROPERTY_GFX_Y_OFFSET_ADJ,
    SYSTEM_PROPERTY_GLOBAL_CONFIG,
    SYSTEM_PROPERTY_GLOBAL_SAMPLE_BEAT,
    SYSTEM_PROPERTY_GLOBAL_SAMPLE_BEEP,
    SYSTEM_PROPERTY_GLOBAL_SAMPLE_BEEP_2,
    SYSTEM_PROPERTY_GLOBAL_SAMPLE_BIKE,
    SYSTEM_PROPERTY_GLOBAL_SAMPLE_BLOCK,
    SYSTEM_PROPERTY_GLOBAL_SAMPLE_FALL,
    SYSTEM_PROPERTY_GLOBAL_SAMPLE_GET,
    SYSTEM_PROPERTY_GLOBAL_SAMPLE_GET_2,
    SYSTEM_PROPERTY_GLOBAL_SAMPLE_GO,
    SYSTEM_PROPERTY_GLOBAL_SAMPLE_INDIRECT,
    SYSTEM_PROPERTY_GLOBAL_SAMPLE_JUMP,
    SYSTEM_PROPERTY_GLOBAL_SAMPLE_ONE_UP,
    SYSTEM_PROPERTY_GLOBAL_SAMPLE_PAUSE,
    SYSTEM_PROPERTY_GLOBAL_SAMPLE_PUNCH,
    SYSTEM_PROPERTY_GLOBAL_SAMPLE_TIME_OVER,
    SYSTEM_PROPERTY_HOLE_Z,
    SYSTEM_PROPERTY_HRESOLUTION,
    SYSTEM_PROPERTY_HUD_COMMON_OPPONENT,
    SYSTEM_PROPERTY_HUD_COMMON_MAIN,
    SYSTEM_PROPERTY_HUD_COMMON_MP,
    SYSTEM_PROPERTY_HUD_LOAD,
    SYSTEM_PROPERTY_HUD_Z,
    SYSTEM_PROPERTY_IN_BUTTON_CONFIG,
    SYSTEM_PROPERTY_IN_CHEAT_OPTIONS,
    SYSTEM_PROPERTY_IN_CONTROL_OPTIONS,
    SYSTEM_PROPERTY_IN_ENGINECREDITSSCREEN,
    SYSTEM_PROPERTY_IN_GAMEOVERSCREEN,
    SYSTEM_PROPERTY_IN_HALLOFFAMESCREEN,
    SYSTEM_PROPERTY_IN_LEVEL,
    SYSTEM_PROPERTY_IN_LOAD_GAME,
    SYSTEM_PROPERTY_IN_MENUSCREEN,
    SYSTEM_PROPERTY_IN_NEW_GAME,
    SYSTEM_PROPERTY_IN_OPTIONS,
    SYSTEM_PROPERTY_IN_SELECTSCREEN,
    SYSTEM_PROPERTY_IN_SHOWCOMPLETE,
    SYSTEM_PROPERTY_IN_SOUND_OPTIONS,
    SYSTEM_PROPERTY_IN_START_GAME,
    SYSTEM_PROPERTY_IN_SYSTEM_OPTIONS,
    SYSTEM_PROPERTY_IN_TITLESCREEN,
    SYSTEM_PROPERTY_IN_VIDEO_OPTIONS,
	SYSTEM_PROPERTY_LASTHIT_ATTACK,
	SYSTEM_PROPERTY_LASTHIT_ATTACKER,
	SYSTEM_PROPERTY_LASTHIT_TARGET,
	SYSTEM_PROPERTY_LASTHITA,
    SYSTEM_PROPERTY_LASTHITC,
    SYSTEM_PROPERTY_LASTHITT,
    SYSTEM_PROPERTY_LASTHITX,
    SYSTEM_PROPERTY_LASTHITY,
    SYSTEM_PROPERTY_LASTHITZ,
    SYSTEM_PROPERTY_LEVELHEIGHT,
    SYSTEM_PROPERTY_LEVELPOS,
    SYSTEM_PROPERTY_LEVELWIDTH,
    SYSTEM_PROPERTY_LIGHTX,
    SYSTEM_PROPERTY_LIGHTZ,
    SYSTEM_PROPERTY_MAX_WALL_HEIGHT,
    SYSTEM_PROPERTY_MAXANIMATIONS,
    SYSTEM_PROPERTY_MAXATTACKTYPES,
    SYSTEM_PROPERTY_MAXENTITYVARS,
    SYSTEM_PROPERTY_MAXGLOBALVARS,
    SYSTEM_PROPERTY_MAXINDEXEDVARS,
    SYSTEM_PROPERTY_MAXPLAYERS,
    SYSTEM_PROPERTY_MAXSCRIPTVARS,
    SYSTEM_PROPERTY_MAXSOUNDCHANNELS,
    SYSTEM_PROPERTY_MIRROR_Z,
    SYSTEM_PROPERTY_MODELS_CACHED,
    SYSTEM_PROPERTY_MODELS_LOADED,
    SYSTEM_PROPERTY_MUSIC_CHANNEL,
    SYSTEM_PROPERTY_MUSICVOL,
    SYSTEM_PROPERTY_NEON_PANEL_Z,
    SYSTEM_PROPERTY_NOAIRCANCEL,
    SYSTEM_PROPERTY_NOFADEOUT,
    SYSTEM_PROPERTY_NOGAMEOVER,
    SYSTEM_PROPERTY_NOHOF,
    SYSTEM_PROPERTY_NOJOIN,
    SYSTEM_PROPERTY_NOPAUSE,
    SYSTEM_PROPERTY_NOSAVE,
    SYSTEM_PROPERTY_NOSCREENSHOT,
    SYSTEM_PROPERTY_NOSHOWCOMPLETE,
    SYSTEM_PROPERTY_NUMBASEMAPS,
    SYSTEM_PROPERTY_NUMBOSSES,
    SYSTEM_PROPERTY_NUMHOLES,
    SYSTEM_PROPERTY_NUMLAYERS,
    SYSTEM_PROPERTY_NUMPALETTES,
    SYSTEM_PROPERTY_NUMWALLS,
    SYSTEM_PROPERTY_PAKNAME,
    SYSTEM_PROPERTY_PANEL_Z,
    SYSTEM_PROPERTY_PAUSE,
    SYSTEM_PROPERTY_PIXELFORMAT,
    SYSTEM_PROPERTY_PLAYER,
    SYSTEM_PROPERTY_PLAYER1,
    SYSTEM_PROPERTY_PLAYER2,
    SYSTEM_PROPERTY_PLAYER3,
    SYSTEM_PROPERTY_PLAYER4,
    SYSTEM_PROPERTY_PLAYER_MAX_Z,
    SYSTEM_PROPERTY_PLAYER_MIN_Z,
    SYSTEM_PROPERTY_PORTING,
    SYSTEM_PROPERTY_SAMPLE_PLAY_ID,
    SYSTEM_PROPERTY_SCREEN_PANEL_Z,
    SYSTEM_PROPERTY_SCREEN_STATUS,
    SYSTEM_PROPERTY_SCROLLMAXX,
    SYSTEM_PROPERTY_SCROLLMAXZ,
    SYSTEM_PROPERTY_SCROLLMINX,
    SYSTEM_PROPERTY_SCROLLMINZ,
    SYSTEM_PROPERTY_SELF,
    SYSTEM_PROPERTY_SETS_COUNT,
    SYSTEM_PROPERTY_SHADOW_Z,
    SYSTEM_PROPERTY_SHADOWALPHA,
    SYSTEM_PROPERTY_SHADOWCOLOR,
    SYSTEM_PROPERTY_SHADOWOPACITY,
    SYSTEM_PROPERTY_SHOWGO,
    SYSTEM_PROPERTY_SKIPTOSET,
    SYSTEM_PROPERTY_SLOWMOTION,
    SYSTEM_PROPERTY_SLOWMOTION_DURATION,
    SYSTEM_PROPERTY_SMARTBOMBER,
    SYSTEM_PROPERTY_SOUNDVOL,
    SYSTEM_PROPERTY_TEXTBOX,
    SYSTEM_PROPERTY_TICKS,
    SYSTEM_PROPERTY_TOTALRAM,
    SYSTEM_PROPERTY_USEDRAM,
    SYSTEM_PROPERTY_VIEWPORTH,
    SYSTEM_PROPERTY_VIEWPORTW,
    SYSTEM_PROPERTY_VIEWPORTX,
    SYSTEM_PROPERTY_VIEWPORTY,
    SYSTEM_PROPERTY_VRESOLUTION,
    SYSTEM_PROPERTY_VSCREEN,
    SYSTEM_PROPERTY_WAITING,
    SYSTEM_PROPERTY_XPOS,
    SYSTEM_PROPERTY_YPOS,
    SYSTEM_PROPERTY_THE_END
};

#endif
