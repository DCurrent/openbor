/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
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

typedef struct Script
{
	Interpreter* pinterpreter;
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
ScriptVariant* Script_Get_Local_Variant(char* theName);
int Script_Set_Local_Variant(char* theName, ScriptVariant* var);
void Script_Init(Script* pscript, char* theName, int first);
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

ptrdiff_t system_isempty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t system_NULL(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t system_rand(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t system_maxglobalvarindex(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t system_getglobalvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t system_setglobalvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t system_getlocalvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t system_setlocalvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t system_clearlocalvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t system_clearglobalvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t system_clearindexedvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t system_free(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_systemvariant(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_changesystemvariant(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_drawstring(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_drawstringtoscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_drawsprite(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_drawspritetoscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_log(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_drawbox(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_drawline(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_drawdot(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_drawboxtoscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_drawlinetoscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_drawdottoscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_drawscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_changeplayerproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_changeentityproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_getplayerproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_getentityproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_clearspawnentry(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_setspawnentry(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_spawn(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_projectile(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_transconst(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_tossentity(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_playmusic(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_fademusic(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_setmusicvolume(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_setmusictempo(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_pausemusic(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_playsample(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_loadsample(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_unloadsample(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_fadeout(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_playerkeys(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_changepalette(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_damageentity(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_killentity(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_findtarget(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_checkrange(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_gettextobjproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_changetextobjproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_settextobj(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_cleartextobj(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_getbglayerproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_changebglayerproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_getfglayerproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_changefglayerproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_getlevelproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_changelevelproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_checkhole(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_checkwall(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_checkplatformbelow(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

ptrdiff_t openbor_openfilestream(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_getfilestreamline(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_getfilestreamargument(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_filestreamnextline(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_getfilestreamposition(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_setfilestreamposition(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

ptrdiff_t openbor_filestreamappend(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_createfilestream(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_savefilestream(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

ptrdiff_t openbor_getindexedvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_setindexedvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_getscriptvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_setscriptvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_getentityvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_setentityvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

ptrdiff_t openbor_jumptobranch(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

ptrdiff_t openbor_changelight(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_changeshadowcolor(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_bindentity(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

ptrdiff_t openbor_allocscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_clearscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_setdrawmethod(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_updateframe(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_performattack(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_setidle(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_getentity(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

ptrdiff_t openbor_loadmodel(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_loadsprite(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_playgif(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

ptrdiff_t openbor_strinfirst(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_strinlast(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_strleft(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_strlength(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_strright(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_getmodelproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_changemodelproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);
ptrdiff_t openbor_rgbcolor(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

void mapstrings_systemvariant(ScriptVariant** varlist, int paramCount);
void mapstrings_changesystemvariant(ScriptVariant** varlist, int paramCount);
void mapstrings_getentityproperty(ScriptVariant** varlist, int paramCount);
void mapstrings_changeentityproperty(ScriptVariant** varlist, int paramCount);
void mapstrings_getplayerproperty(ScriptVariant** varlist, int paramCount);
void mapstrings_changeplayerproperty(ScriptVariant** varlist, int paramCount);
void mapstrings_setspawnentry(ScriptVariant** varlist, int paramCount);
void mapstrings_transconst(ScriptVariant** varlist, int paramCount);
void mapstrings_playerkeys(ScriptVariant** varlist, int paramCount);
void mapstrings_gettextobjproperty(ScriptVariant** varlist, int paramCount);
void mapstrings_changetextobjproperty(ScriptVariant** varlist, int paramCount);
void mapstrings_getbglayerproperty(ScriptVariant** varlist, int paramCount);
void mapstrings_changebglayerproperty(ScriptVariant** varlist, int paramCount);
void mapstrings_getfglayerproperty(ScriptVariant** varlist, int paramCount);
void mapstrings_changefglayerproperty(ScriptVariant** varlist, int paramCount);

#endif
