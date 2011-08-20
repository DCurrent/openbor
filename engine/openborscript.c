/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

/* This file include all script methods used by openbor engine

 Notice: Make sure to null *pretvar when you about to return E_FAIL,
		 Or the engine might crash.

 Notice: Every new ScriptVariant must be initialized when you first alloc it by
		 ScriptVariant_Init immediately, memset it all to zero should also work by now,
		 unless VT_EMPTY is changed.

		 If you want to reset a ScriptVariant to empty, you must use ScriptVariant_Clear instead.
		 ScriptVariant_Init or memset must be called only ONCE, later you should use ScriptVariant_Clear.

		 Besure to call ScriptVariant_Clear if you want to use free to delete those variants.

		 If you want to copy a ScriptVariant from another, use ScriptVariant_Copy instead of assignment,
		 not because it is faster, but this method is neccessary for string types.

		 If you want to change types of an ScriptVariant, use ScriptVariant_ChangeType, don't change vt directly.

*/

#include "openborscript.h"
#include "openbor.h"
#include "soundmix.h"
#include "globals.h"
#include "ImportCache.h"
#include "models.h"

// Define macro for string mapping
#define MAPSTRINGS(VAR, LIST, MAXINDEX, FAILMSG) \
if(VAR->vt == VT_STR) { \
	propname = (char*)StrCache_Get(VAR->strVal); \
	prop = searchList(LIST, propname, MAXINDEX); \
	if(prop >= 0) { \
		ScriptVariant_ChangeType(VAR, VT_INTEGER); \
		VAR->lVal = prop; \
	} else { printf(FAILMSG, propname); } \
}

extern int			  PLAYER_MIN_Z;
extern int			  PLAYER_MAX_Z;
extern int			  BGHEIGHT;
extern int            MAX_WALL_HEIGHT;
extern int			  SAMPLE_GO;
extern int			  SAMPLE_BEAT;
extern int			  SAMPLE_BLOCK;
extern int			  SAMPLE_INDIRECT;
extern int			  SAMPLE_GET;
extern int			  SAMPLE_GET2;
extern int			  SAMPLE_FALL;
extern int			  SAMPLE_JUMP;
extern int			  SAMPLE_PUNCH;
extern int			  SAMPLE_1UP;
extern int			  SAMPLE_TIMEOVER;
extern int			  SAMPLE_BEEP;
extern int			  SAMPLE_BEEP2;
extern int			  SAMPLE_BIKE;
extern int            current_palette;
extern s_player       player[4];
extern s_savedata     savedata;
extern s_savelevel    savelevel[MAX_DIFFICULTIES];
extern s_savescore    savescore;
extern s_level        *level;
extern entity         *self;
extern int            *animspecials;
extern int            *animattacks;
extern int            *animfollows;
extern int            *animpains;
extern int            *animfalls;
extern int            *animrises;
extern int            *animriseattacks;
extern int            *animdies;
extern int            *animidles;
extern int            *animwalks;
extern int            *animbackwalks;
extern int            *animups;
extern int            *animdowns;
extern int            noshare;
extern int            credits;
extern char           musicname[128];
extern float          musicfade[2];
extern int            musicloop;
extern u32            musicoffset;
extern int            models_cached;

extern unsigned char* blendings[MAX_BLENDINGS];
extern int            current_palette;
s_variantnode** global_var_list = NULL;
Script* pcurrentscript = NULL;//used by local script functions
List theFunctionList;
static List   scriptheap;
static s_spawn_entry spawnentry;
extern s_drawmethod plainmethod;
static s_drawmethod drawmethod;
//fake 3d draw
static gfx_entry	texture;
#define	vert_buf_size	256
static  vert2d		verts[vert_buf_size];


int max_global_var_index = -1;

ScriptVariant* indexed_var_list = NULL;
int            max_global_vars = MAX_GLOBAL_VAR;
int            max_indexed_vars = 0;
int            max_entity_vars = 0;
int            max_script_vars = 0;


//this function should be called before all script methods, for once
void Script_Global_Init()
{
	ptrdiff_t i;
	size_t csize, psize;
	if(max_global_vars>0)
	{
		psize = (sizeof(s_variantnode*) * max_global_vars);
		csize = psize + (sizeof(s_variantnode) * max_global_vars);
		global_var_list = malloc(csize);
		assert(global_var_list != NULL);
		memset(global_var_list, 0, csize);
		for (i=0; i < max_global_vars; i++) {
			global_var_list[i] = (s_variantnode*) (((char*) global_var_list) + psize + (i * sizeof(s_variantnode)));
		}
	}
	/*
	for(i=0; i<max_global_vars; i++)
	{
		global_var_list[i] = malloc(sizeof(s_variantnode));
		assert(global_var_list[i] != NULL);
		memset(global_var_list[i], 0, sizeof(s_variantnode));
	} */
	max_global_var_index = -1;
	memset(&spawnentry, 0, sizeof(s_spawn_entry));//clear up the spawn entry
	drawmethod = plainmethod;

	if(max_indexed_vars>0)
	{
		csize = sizeof(ScriptVariant)*(max_indexed_vars + 1);
		indexed_var_list = (ScriptVariant*)malloc(csize);
		assert(indexed_var_list != NULL);
		memset(indexed_var_list, 0, csize);
	}
	List_Init(&theFunctionList);
	Script_LoadSystemFunctions();
	List_Init(&scriptheap);
	ImportCache_Init();
}

//this function should only be called when the engine is shutting down
void Script_Global_Clear()
{
	int i, size;
	List_Clear(&theFunctionList);
	// dump all un-freed variants
	size = List_GetSize(&scriptheap);
	if(size>0) printf("\nWarning: %d script variants are not freed, dumping...\n", size);
	for(i=0, List_Reset(&scriptheap); i<size; List_GotoNext(&scriptheap), i++)
	{
		printf("%s\n", List_GetName(&scriptheap));
		free(List_Retrieve(&scriptheap));
	}
	List_Clear(&scriptheap);
	// clear the global list
	if(global_var_list)
	{
		for(i=0; i<max_global_vars; i++)
		{
			if(global_var_list[i] != NULL) {
				ScriptVariant_Clear(&(global_var_list[i]->value));
				//free(global_var_list[i]);
			}
			//global_var_list[i] = NULL;
		}
		free(global_var_list);
		global_var_list = NULL;
	}
	if(indexed_var_list)
	{
		for(i=0; i<max_indexed_vars; i++) ScriptVariant_Clear(indexed_var_list+i);
		free(indexed_var_list);
	}
	indexed_var_list = NULL;
	max_global_var_index = -1;
	memset(&spawnentry, 0, sizeof(s_spawn_entry));//clear up the spawn entry
	StrCache_Clear();
	ImportCache_Clear();
}


ScriptVariant* Script_Get_Global_Variant(char* theName)
{
	int i;

	if(!theName || !theName[0]) return NULL;

	for(i=0; i<=max_global_var_index; i++){
		if(!global_var_list[i]->owner &&
		   strcmp(theName, global_var_list[i]->key)==0)
			return &(global_var_list[i]->value);
	}

	return NULL;
}

// local function
int _set_var(char* theName, ScriptVariant* var, Script* owner)
{
	int i;
	s_variantnode* tempnode;
	if(!theName[0] || !theName || (owner && !owner->initialized)) return 0;
	// search the name
	for(i=0; i<=max_global_var_index; i++)
	{
		if(global_var_list[i]->owner == owner &&
		   !strcmp(theName, global_var_list[i]->key))
		{
			if(var->vt != VT_EMPTY)
				ScriptVariant_Copy(&(global_var_list[i]->value), var);
			else // set to null, so remove this value
			{
				/// re-adjust bounds, swap with last node
				if(i!=max_global_var_index)
				{
					tempnode = global_var_list[i];
					global_var_list[i] = global_var_list[max_global_var_index];
					global_var_list[max_global_var_index] = tempnode;
				}
				max_global_var_index--;
			}
			return 1;
		}
	}
	if(var->vt == VT_EMPTY) return 1;
	// all slots are taken
	if(max_global_var_index >= max_global_vars-1)
		return 0;
	// so out of bounds, find another slot
	else
	{
		++max_global_var_index;
		ScriptVariant_Copy(&(global_var_list[max_global_var_index]->value), var);
		global_var_list[max_global_var_index]->owner = owner;
		strcpy(global_var_list[max_global_var_index]->key, theName);
		return 1;
	}
}// end of _set_var

int Script_Set_Global_Variant(char* theName, ScriptVariant* var)
{
	return _set_var(theName, var, NULL);
}

void Script_Local_Clear()
{
	int i;
	s_variantnode* tempnode;
	if(!pcurrentscript) return;
	for(i=0; i<=max_global_var_index; i++)
	{
		if(global_var_list[i]->owner == pcurrentscript)
		{
			if(i!=max_global_var_index)
			{
				tempnode = global_var_list[i];
				global_var_list[i] = global_var_list[max_global_var_index];
				global_var_list[max_global_var_index] = tempnode;
			}
			max_global_var_index--;
		}
	}
	if(pcurrentscript->vars)
		for(i=0; i<max_script_vars; i++) ScriptVariant_Clear(pcurrentscript->vars+i);
}


ScriptVariant* Script_Get_Local_Variant(char* theName)
{
	int i;

	if(!pcurrentscript || !pcurrentscript->initialized ||
	   !theName || !theName[0]) return NULL;

	for(i=0; i<=max_global_var_index; i++)
	{
		if(global_var_list[i]->owner == pcurrentscript &&
		   strcmp(theName, global_var_list[i]->key)==0)
			return &(global_var_list[i]->value);
	}

	return NULL;
}

int Script_Set_Local_Variant(char* theName, ScriptVariant* var)
{
	if(!pcurrentscript) return 0;
	return _set_var(theName, var, pcurrentscript);
}

Script* alloc_script()
{
	int i;
	Script* pscript = (Script*)malloc(sizeof(Script));
	memset(pscript, 0, sizeof(Script));
	if(max_script_vars>0)
	{
		pscript->vars = (ScriptVariant*)malloc(sizeof(ScriptVariant)*max_script_vars);
		for(i=0; i<max_script_vars; i++) ScriptVariant_Init(pscript->vars+i);
	}
	return pscript;
}

void Script_Init(Script* pscript, char* theName, int first)
{
	int i;
	if(first)
	{
		memset(pscript, 0, sizeof(Script));
		if(max_script_vars>0)
		{
			pscript->vars = (ScriptVariant*)malloc(sizeof(ScriptVariant)*max_script_vars);
			for(i=0; i<max_script_vars; i++) ScriptVariant_Init(pscript->vars+i);
		}
	}
	if(!theName || !theName[0])  return; // if no name specified, only alloc the variants
	pcurrentscript = pscript; //used by local script functions
	pscript->pinterpreter = (Interpreter*)malloc(sizeof(Interpreter));
	Interpreter_Init(pscript->pinterpreter, theName, &theFunctionList);
	pscript->interpreterowner = 1; // this is the owner, important
	pscript->initialized = 1;
}

//safe copy method
void Script_Copy(Script* pdest, Script* psrc, int localclear)
{
	if(!psrc->initialized) return;
	if(pdest->initialized) Script_Clear(pdest, localclear);
	pdest->pinterpreter = psrc->pinterpreter;
	pdest->interpreterowner = 0; // dont own it
	pdest->initialized = psrc->initialized; //just copy, it should be 1
}

void Script_Clear(Script* pscript, int localclear)
{
	Script* temp;
	int i;
	ScriptVariant* pvars;
	if(localclear==2 && pscript->vars)
	{
		for(i=0; i<max_script_vars; i++)
		{
			ScriptVariant_Clear(pscript->vars+i);
		}
		free(pscript->vars);
		pscript->vars = NULL;
	}
	if(!pscript->initialized) return;
	temp = pcurrentscript;
	pcurrentscript = pscript; //used by local script functions
	//if it is the owner, free the interpreter
	if(pscript->pinterpreter && pscript->interpreterowner){
		Interpreter_Clear(pscript->pinterpreter);
		free(pscript->pinterpreter);
		pscript->pinterpreter = NULL;
	}
	if(localclear) Script_Local_Clear();
	pvars = pscript->vars; // in game clear(localclear!=2) just keep this value
	memset(pscript, 0, sizeof(Script));
	pscript->vars = pvars; // copy it back
	pcurrentscript = temp;
}

//append part of the script
//Because the script might not be initialized in 1 time.
int Script_AppendText(Script* pscript, char* text, char* path)
{
	int success;

	pcurrentscript = pscript; //used by local script functions
	//printf(text);
	Interpreter_Reset(pscript->pinterpreter);

	success = SUCCEEDED(Interpreter_ParseText(pscript->pinterpreter, text, 1, path));

	return success;
}

//return name of function from pointer to function
const char* Script_GetFunctionName(void* functionRef)
{
	if (functionRef==((void*)system_isempty)) return "isempty";
	else if (functionRef==((void*)system_NULL)) return "NULL";
	else if (functionRef==((void*)system_rand)) return "rand";
	else if (functionRef==((void*)system_maxglobalvarindex)) return "maxglobalvarindex";
	else if (functionRef==((void*)system_getglobalvar)) return "getglobalvar";
	else if (functionRef==((void*)system_setglobalvar)) return "setglobalvar";
	else if (functionRef==((void*)system_getlocalvar)) return "getlocalvar";
	else if (functionRef==((void*)system_setlocalvar)) return "setlocalvar";
	else if (functionRef==((void*)system_clearglobalvar)) return "clearglobalvar";
	else if (functionRef==((void*)system_clearindexedvar)) return "clearindexedvar";
	else if (functionRef==((void*)system_clearlocalvar)) return "clearlocalvar";
	else if (functionRef==((void*)system_free)) return "free";
	else if (functionRef==((void*)openbor_systemvariant)) return "openborvariant";
	else if (functionRef==((void*)openbor_changesystemvariant)) return "changeopenborvariant";
	else if (functionRef==((void*)openbor_drawstring)) return "drawstring";
	else if (functionRef==((void*)openbor_drawstringtoscreen)) return "drawstringtoscreen";
	else if (functionRef==((void*)openbor_log)) return "log";
	else if (functionRef==((void*)openbor_drawbox)) return "drawbox";
	else if (functionRef==((void*)openbor_drawboxtoscreen)) return "drawboxtoscreen";
	else if (functionRef==((void*)openbor_drawline)) return "drawline";
	else if (functionRef==((void*)openbor_drawlinetoscreen)) return "drawlinetoscreen";
	else if (functionRef==((void*)openbor_drawsprite)) return "drawsprite";
	else if (functionRef==((void*)openbor_drawspritetoscreen)) return "drawspritetoscreen";
	else if (functionRef==((void*)openbor_drawdot)) return "drawdot";
	else if (functionRef==((void*)openbor_drawdottoscreen)) return "drawdottoscreen";
	else if (functionRef==((void*)openbor_drawscreen)) return "drawscreen";
	else if (functionRef==((void*)openbor_changeplayerproperty)) return "changeplayerproperty";
	else if (functionRef==((void*)openbor_changeentityproperty)) return "changeentityproperty";
	else if (functionRef==((void*)openbor_getplayerproperty)) return "getplayerproperty";
	else if (functionRef==((void*)openbor_getentityproperty)) return "getentityproperty";
	else if (functionRef==((void*)openbor_tossentity)) return "tossentity";
	else if (functionRef==((void*)openbor_clearspawnentry)) return "clearspawnentry";
	else if (functionRef==((void*)openbor_setspawnentry)) return "setspawnentry";
	else if (functionRef==((void*)openbor_spawn)) return "spawn";
	else if (functionRef==((void*)openbor_projectile)) return "projectile";
	else if (functionRef==((void*)openbor_transconst)) return "openborconstant";
	else if (functionRef==((void*)openbor_playmusic)) return "playmusic";
	else if (functionRef==((void*)openbor_fademusic)) return "fademusic";
	else if (functionRef==((void*)openbor_setmusicvolume)) return "setmusicvolume";
	else if (functionRef==((void*)openbor_setmusictempo)) return "setmusictempo";
	else if (functionRef==((void*)openbor_pausemusic)) return "pausemusic";
	else if (functionRef==((void*)openbor_playsample)) return "playsample";
	else if (functionRef==((void*)openbor_loadsample)) return "loadsample";
	else if (functionRef==((void*)openbor_unloadsample)) return "unloadsample";
	else if (functionRef==((void*)openbor_fadeout)) return "fadeout";
	else if (functionRef==((void*)openbor_playerkeys)) return "playerkeys";
	else if (functionRef==((void*)openbor_changepalette)) return "changepalette";
	else if (functionRef==((void*)openbor_damageentity)) return "damageentity";
	else if (functionRef==((void*)openbor_killentity)) return "killentity";
	else if (functionRef==((void*)openbor_findtarget)) return "findtarget";
	else if (functionRef==((void*)openbor_checkrange)) return "checkrange";
	else if (functionRef==((void*)openbor_gettextobjproperty)) return "gettextobjproperty";
	else if (functionRef==((void*)openbor_changetextobjproperty)) return "changetextobjproperty";
	else if (functionRef==((void*)openbor_settextobj)) return "settextobj";
	else if (functionRef==((void*)openbor_cleartextobj)) return "cleartextobj";
	else if (functionRef==((void*)openbor_getbglayerproperty)) return "getbglayerproperty";
	else if (functionRef==((void*)openbor_changebglayerproperty)) return "changebglayerproperty";
	else if (functionRef==((void*)openbor_getfglayerproperty)) return "getfglayerproperty";
	else if (functionRef==((void*)openbor_changefglayerproperty)) return "changefglayerproperty";
	else if (functionRef==((void*)openbor_getlevelproperty)) return "getlevelproperty";
	else if (functionRef==((void*)openbor_changelevelproperty)) return "changelevelproperty";
	else if (functionRef==((void*)openbor_checkhole)) return "checkhole";
	else if (functionRef==((void*)openbor_checkwall)) return "checkwall";
	else if (functionRef==((void*)openbor_checkplatformbelow)) return "checkplatformbelow";
	else if (functionRef==((void*)openbor_openfilestream)) return "openfilestream";
	else if (functionRef==((void*)openbor_getfilestreamline)) return "getfilestreamline";
	else if (functionRef==((void*)openbor_getfilestreamargument)) return "getfilestreamargument";
	else if (functionRef==((void*)openbor_filestreamnextline)) return "filestreamnextline";
	else if (functionRef==((void*)openbor_getfilestreamposition)) return "getfilestreamposition";
	else if (functionRef==((void*)openbor_setfilestreamposition)) return "setfilestreamposition";
	else if (functionRef==((void*)openbor_filestreamappend)) return "filestreamappend";
	else if (functionRef==((void*)openbor_createfilestream)) return "createfilestream";
	else if (functionRef==((void*)openbor_savefilestream)) return "savefilestream";
	else if (functionRef==((void*)openbor_getindexedvar)) return "getindexedvar";
	else if (functionRef==((void*)openbor_setindexedvar)) return "setindexedvar";
	else if (functionRef==((void*)openbor_getscriptvar)) return "getscriptvar";
	else if (functionRef==((void*)openbor_setscriptvar)) return "setscriptvar";
	else if (functionRef==((void*)openbor_getentityvar)) return "getentityvar";
	else if (functionRef==((void*)openbor_setentityvar)) return "setentityvar";
	else if (functionRef==((void*)openbor_jumptobranch)) return "jumptobranch";
	else if (functionRef==((void*)openbor_changelight)) return "changelight";
	else if (functionRef==((void*)openbor_changeshadowcolor)) return "changeshadowcolor";
	else if (functionRef==((void*)openbor_bindentity)) return "bindentity";
	else if (functionRef==((void*)openbor_allocscreen)) return "allocscreen";
	else if (functionRef==((void*)openbor_clearscreen)) return "clearscreen";
	else if (functionRef==((void*)openbor_setdrawmethod)) return "setdrawmethod";
	else if (functionRef==((void*)openbor_updateframe)) return "updateframe";
	else if (functionRef==((void*)openbor_performattack)) return "performattack";
	else if (functionRef==((void*)openbor_setidle)) return "setidle";
	else if (functionRef==((void*)openbor_getentity)) return "getentity";
	else if (functionRef==((void*)openbor_loadmodel)) return "loadmodel";
	else if (functionRef==((void*)openbor_loadsprite)) return "loadsprite";
	else if (functionRef==((void*)openbor_playgif)) return "playgif";
	else if (functionRef==((void*)openbor_strinfirst)) return "strinfirst";
	else if (functionRef==((void*)openbor_strinlast)) return "strinlast";
	else if (functionRef==((void*)openbor_strleft)) return "strleft";
	else if (functionRef==((void*)openbor_strlength)) return "strlength";
	else if (functionRef==((void*)openbor_strright)) return "strright";
	else if (functionRef==((void*)openbor_getmodelproperty)) return "getmodelproperty";
	else if (functionRef==((void*)openbor_changemodelproperty)) return "changemodelproperty";
	else if (functionRef==((void*)openbor_rgbcolor)) return "rgbcolor";
	else if (functionRef==((void*)openbor_zoom)) return "zoom";
	else if (functionRef==((void*)openbor_settexture)) return "settexture";
	else if (functionRef==((void*)openbor_setvertex)) return "setvertex";
	else if (functionRef==((void*)openbor_trianglelist)) return "trianglelist";
	else return "<unknown function>";
}

//return string mapping function corresponding to a given function
void* Script_GetStringMapFunction(void* functionRef)
{
	if (functionRef==((void*)openbor_systemvariant)) return (void*)mapstrings_systemvariant;
	else if (functionRef==((void*)openbor_changesystemvariant)) return (void*)mapstrings_changesystemvariant;
	else if (functionRef==((void*)openbor_getentityproperty)) return (void*)mapstrings_getentityproperty;
	else if (functionRef==((void*)openbor_changeentityproperty)) return (void*)mapstrings_changeentityproperty;
	else if (functionRef==((void*)openbor_getplayerproperty)) return (void*)mapstrings_getplayerproperty;
	else if (functionRef==((void*)openbor_changeplayerproperty)) return (void*)mapstrings_changeplayerproperty;
	else if (functionRef==((void*)openbor_setspawnentry)) return (void*)mapstrings_setspawnentry;
	else if (functionRef==((void*)openbor_transconst)) return (void*)mapstrings_transconst;
	else if (functionRef==((void*)openbor_playerkeys)) return (void*)mapstrings_playerkeys;
	else if (functionRef==((void*)openbor_gettextobjproperty)) return (void*)mapstrings_gettextobjproperty;
	else if (functionRef==((void*)openbor_changetextobjproperty)) return (void*)mapstrings_changetextobjproperty;
	else if (functionRef==((void*)openbor_getbglayerproperty)) return (void*)mapstrings_getbglayerproperty;
	else if (functionRef==((void*)openbor_changebglayerproperty)) return (void*)mapstrings_changebglayerproperty;
	else if (functionRef==((void*)openbor_getfglayerproperty)) return (void*)mapstrings_getfglayerproperty;
	else if (functionRef==((void*)openbor_changefglayerproperty)) return (void*)mapstrings_changefglayerproperty;
	else return NULL;
}

/* Replace string constants with enum constants at compile time to speed up
   script execution. */
int Script_MapStringConstants(Script* pscript)
{
	Interpreter* pinterpreter = pscript->pinterpreter;
	Instruction* pInstruction, *pInstruction2;
	ScriptVariant** params;
	//ScriptVariant* var;
	void (*pMapstrings)(ScriptVariant**, int);
	int i, j, k, size, paramCount;

	size = List_GetSize(&(pinterpreter->theInstructionList));
	for(i=0; i<size; i++)
	{
		pInstruction = (Instruction*)(pinterpreter->theInstructionList.solidlist[i]);
		if(pInstruction->functionRef)
		{
			params = (ScriptVariant**)pInstruction->theRefList->solidlist;
			paramCount = (int)pInstruction->theRef->lVal;

			// Get the pointer to the correct mapstrings function, if one exists.
			pMapstrings = Script_GetStringMapFunction(pInstruction->functionRef);
			if(pMapstrings)
			{
				// Call the mapstrings function.
				pMapstrings(params, paramCount);

				// Find the instruction containing each constant and update its value.
				for(j=0; j<paramCount; j++)
				{
					for(k=i; k>0; k--)
					{
						pInstruction2 = (Instruction*)(pinterpreter->theInstructionList.solidlist[k]);
						if(pInstruction2->theVal2 == params[j])
						{
							ScriptVariant_Copy(pInstruction2->theVal, pInstruction2->theVal2);
							break;
						}
					}
					if(k<0) return 0;
				}
			}
		}
	}

	return 1;
}

// replaces the entire instruction list with a new instruction list after optimization
int Script_ReplaceInstructionList(Interpreter* pInterpreter, List* newList)
{
	int i, j, newSize = List_GetSize(newList);
	Instruction* pInstruction, *pTarget;
	Instruction** oldList = (Instruction**)pInterpreter->theInstructionList.solidlist;
	List_Solidify(newList);

	char buf[256];

	for(i=0; i<newSize; i++)
	{
		pInstruction = (Instruction*)newList->solidlist[i];
		if(pInstruction->theJumpTargetIndex >= 0)
		{
			pTarget = (Instruction*)oldList[pInstruction->theJumpTargetIndex];
			for(j=0; j<newSize; j++)
			{
				if(newList->solidlist[j] == pTarget)
				{
					pInstruction->theJumpTargetIndex = j;
					break;
				}
			}
			// if the jump target isn't found, it must have been removed in an optimization - whoops!
			if(j == newSize)
			{
				Instruction_ToString(pTarget, buf);
				printf("Error: jump target %i (%s) not found - overzealous optimization!\n", pInstruction->theJumpTargetIndex, buf);
				return 0;
			}
		}
	}

	// replace new list with old list
	List_Clear(&(pInterpreter->theInstructionList));
	memcpy(&(pInterpreter->theInstructionList), newList, sizeof(List));

	return 1;
}

// prints lots of debugging stuff about optimizations that can be made
void Script_LowerConstants(Script* pscript)
{
	Interpreter* pinterpreter = pscript->pinterpreter;
	Instruction* pInstruction, *pInstruction2;
	List* newInstructionList = malloc(sizeof(List));
	int i, j, size;

	List_Init(newInstructionList);

	size = List_GetSize(&(pinterpreter->theInstructionList));

	for(i=0; i<size; i++)
	{
		pInstruction = (Instruction*)(pinterpreter->theInstructionList.solidlist[i]);
		if(pInstruction->OpCode == DATA)
		{
			int numRefs = 0;
			for(j=0; j<size; j++)
			{
				pInstruction2 = (Instruction*)(pinterpreter->theInstructionList.solidlist[j]);
				if(pInstruction2->OpCode == LOAD && pInstruction2->theRef == pInstruction->theVal)
					numRefs++;
			}
			//printf("Variable declared, %i references\n", numRefs, pInstruction->theToken->theSource);
			//printf("DATA (theVal=0x%08X, theRef=0x%08X)\n", pInstruction->theVal, pInstruction->theRef);
			if(numRefs > 0) List_InsertAfter(newInstructionList, pInstruction, NULL);
			else
			{
				printf("Unused variable\n");
				free(pInstruction);
			}
		}
		else List_InsertAfter(newInstructionList, pInstruction, NULL);
#define ISCONST(x) ((x) && ((x->OpCode==CONSTINT)||(x->OpCode==CONSTSTR)||(x->OpCode==CONSTDBL)))
		// Look for constant binary ops
		if(pInstruction->OpCode == ADD)
		{
			Instruction *pSrc1 = NULL, *pSrc2 = NULL;
			char buf[1024], buf2[1024], buf3[1024];
			for(j=0; j<size; j++)
			{
				Instruction* tmp = (Instruction*)(pinterpreter->theInstructionList.solidlist[j]);
				if(tmp->theVal == pInstruction->theRef || tmp->theVal2 == pInstruction->theRef)   pSrc1 = tmp;
				if(tmp->theVal == pInstruction->theRef2 || tmp->theVal2 == pInstruction->theRef2) pSrc2 = tmp;
			}

			if(ISCONST(pSrc1) && ISCONST(pSrc2))
			{
				ScriptVariant* sum = ScriptVariant_Add(pSrc1->theVal2, pSrc2->theVal2);
				ScriptVariant_ToString(pSrc1->theVal2, buf);
		    	ScriptVariant_ToString(pSrc2->theVal2, buf2);
		    	ScriptVariant_ToString(sum, buf3);
		    	//printf("ADD 0x%08X: %s + %s = %s\n", pInstruction, buf, buf2, buf3);
			}
#if 0
			else if(pSrc1 && pSrc2)
			{
		    	Instruction_ToString(pSrc1, buf);
		    	Instruction_ToString(pSrc2, buf2);
		    	printf("ADD 0x%08X: %s + %s\n", pInstruction, buf, buf2);
		    }
		    else printf("ADD 0x%08X: Non-constant addition?\n");
#endif
		}
#undef ISCONST
	}

	// replace old instruction list with optimized one (TODO: enable when this function actually does optimizations)
	//Script_ReplaceInstructionList(pinterpreter, &newInstructionList);
}

// detect unused functions in scripts (to save memory)
int Script_DetectUnusedFunctions(Script* pScript)
{
	Interpreter* pInterpreter = pScript->pinterpreter;
	Instruction* pInstruction, *pInstruction2, **instructionList = (Instruction**)pInterpreter->theInstructionList.solidlist;
	List newInstructionList;
	int i, size = List_GetSize(&(pInterpreter->theInstructionList));

	List_Init(&newInstructionList);

	for(i=0; i<size; i++)
	{
		pInstruction = instructionList[i];
		// detect function declarations (FIXME: should have an opcode for function declarations other than NOOP)
		if(pInstruction->OpCode == NOOP && pInstruction->theToken && strlen(pInstruction->theToken->theSource) > 0)
		{
			int j, numCalls = 0;

			// find all calls to this function
			for(j=0; j<size; j++)
			{
				pInstruction2 = instructionList[j];
				if(pInstruction2->OpCode == CALL && pInstruction2->theJumpTargetIndex == i) numCalls++;
			}

			if(numCalls == 0 && strcmp(pInstruction->theToken->theSource, "main") != 0)
			{
				printf("Unused function %s()\n", pInstruction->theToken->theSource);
				while(instructionList[i]->OpCode != RET) // skip function without adding to new instruction list
				{
					free(instructionList[i++]);
					if(i >= size) {List_Clear(&newInstructionList); return 0;} // this shouldn't happen!
				}
				free(instructionList[i]); // free the final RET instruction too
			}
			else List_InsertAfter(&newInstructionList, pInstruction, NULL);
		}
		else List_InsertAfter(&newInstructionList, pInstruction, NULL);

		List_InsertAfter(&newInstructionList, pInstruction, NULL);

		//if(pInstruction->theToken) {free(pInstruction->theToken); pInstruction->theToken=NULL;} // TODO: move somewhere else
	}

	return Script_ReplaceInstructionList(pInterpreter, &newInstructionList);
}

//should be called only once after parsing text
int Script_Compile(Script* pscript)
{
	int result;
	if(!pscript || !pscript->pinterpreter) return 1;
	//Interpreter_OutputPCode(pscript->pinterpreter, "code");
	result = SUCCEEDED(Interpreter_CompileInstructions(pscript->pinterpreter));
	if(!result) {Script_Clear(pscript, 1);shutdown(1, "Can't compile script!\n");}
	result = Script_MapStringConstants(pscript);
	if(!result) {Script_Clear(pscript, 1);shutdown(1, "Can't compile script!\n");}

	//result = Script_DetectUnusedFunctions(pscript);
	//if(!result) {Script_Clear(pscript, 1);shutdown(1, "Script optimization failed!\n");}
	//Script_LowerConstants(pscript);

	return result;
}

int Script_IsInitialized(Script* pscript)
{
	if(pscript && pscript->initialized) pcurrentscript = pscript; //used by local script functions
	return pscript->initialized;
}

//execute the script
int Script_Execute(Script* pscript)
{
	int result=S_OK;
	Script* temp = pcurrentscript;
	pcurrentscript = pscript; //used by local script functions
	Interpreter_Reset(pscript->pinterpreter);
	result = (int)SUCCEEDED(Interpreter_EvaluateImmediate(pscript->pinterpreter));
	pcurrentscript = temp;
	if(!result) shutdown(1, "There's an exception while executing script '%s'.\n", pscript->pinterpreter->theSymbolTable.name);
	return result;
}

#ifndef COMPILED_SCRIPT
//this method is for debug purpose
int Script_Call(Script* pscript, char* method, ScriptVariant* pretvar)
{
	pcurrentscript = pscript; //used by local script functions
	Interpreter_Reset(pscript->pinterpreter);
	return (int)SUCCEEDED(Interpreter_Call(pscript->pinterpreter, method, pretvar));
}
#endif

//used by Script_Global_Init
void Script_LoadSystemFunctions()
{
	//printf("Loading system script functions....");
	//load system functions if we need
	List_Reset(&theFunctionList);

	List_InsertAfter(&theFunctionList,
					  (void*)system_isempty, "isempty");
	List_InsertAfter(&theFunctionList,
					  (void*)system_NULL, "NULL");
	List_InsertAfter(&theFunctionList,
					  (void*)system_rand, "rand");
	List_InsertAfter(&theFunctionList,
					  (void*)system_maxglobalvarindex, "maxglobalvarindex");
	List_InsertAfter(&theFunctionList,
					  (void*)system_getglobalvar, "getglobalvar");
	List_InsertAfter(&theFunctionList,
					  (void*)system_setglobalvar, "setglobalvar");
	List_InsertAfter(&theFunctionList,
					  (void*)system_getlocalvar, "getlocalvar");
	List_InsertAfter(&theFunctionList,
					  (void*)system_setlocalvar, "setlocalvar");
	List_InsertAfter(&theFunctionList,
					  (void*)system_clearglobalvar, "clearglobalvar");
	List_InsertAfter(&theFunctionList,
					  (void*)system_clearindexedvar, "clearindexedvar");
	List_InsertAfter(&theFunctionList,
					  (void*)system_clearlocalvar, "clearlocalvar");
	List_InsertAfter(&theFunctionList,
					  (void*)system_free, "free");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_systemvariant, "openborvariant");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_changesystemvariant, "changeopenborvariant");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_drawstring, "drawstring");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_drawstringtoscreen, "drawstringtoscreen");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_log, "log");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_drawbox, "drawbox");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_drawboxtoscreen, "drawboxtoscreen");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_drawline, "drawline");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_drawlinetoscreen, "drawlinetoscreen");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_drawsprite, "drawsprite");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_drawspritetoscreen, "drawspritetoscreen");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_drawdot, "drawdot");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_drawdottoscreen, "drawdottoscreen");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_drawscreen, "drawscreen");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_changeplayerproperty, "changeplayerproperty");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_changeentityproperty, "changeentityproperty");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_getplayerproperty, "getplayerproperty");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_getentityproperty, "getentityproperty");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_tossentity, "tossentity");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_clearspawnentry, "clearspawnentry");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_setspawnentry, "setspawnentry");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_spawn, "spawn");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_projectile, "projectile");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_transconst, "openborconstant");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_playmusic, "playmusic");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_fademusic, "fademusic");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_setmusicvolume, "setmusicvolume");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_setmusictempo, "setmusictempo");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_pausemusic, "pausemusic");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_playsample, "playsample");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_loadsample, "loadsample");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_unloadsample, "unloadsample");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_fadeout, "fadeout");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_playerkeys, "playerkeys");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_changepalette, "changepalette");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_damageentity, "damageentity");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_killentity, "killentity");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_findtarget, "findtarget");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_checkrange, "checkrange");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_gettextobjproperty, "gettextobjproperty");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_changetextobjproperty, "changetextobjproperty");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_settextobj, "settextobj");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_cleartextobj, "cleartextobj");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_getbglayerproperty, "getbglayerproperty");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_changebglayerproperty, "changebglayerproperty");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_getfglayerproperty, "getfglayerproperty");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_changefglayerproperty, "changefglayerproperty");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_getlevelproperty, "getlevelproperty");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_changelevelproperty, "changelevelproperty");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_checkhole, "checkhole");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_checkwall, "checkwall");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_checkplatformbelow, "checkplatformbelow");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_openfilestream, "openfilestream");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_getfilestreamline, "getfilestreamline");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_getfilestreamargument, "getfilestreamargument");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_filestreamnextline, "filestreamnextline");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_getfilestreamposition, "getfilestreamposition");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_setfilestreamposition, "setfilestreamposition");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_filestreamappend, "filestreamappend");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_createfilestream, "createfilestream");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_savefilestream, "savefilestream");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_getindexedvar, "getindexedvar");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_setindexedvar, "setindexedvar");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_getscriptvar, "getscriptvar");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_setscriptvar, "setscriptvar");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_getentityvar, "getentityvar");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_setentityvar, "setentityvar");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_jumptobranch, "jumptobranch");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_changelight, "changelight");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_changeshadowcolor, "changeshadowcolor");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_bindentity, "bindentity");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_allocscreen, "allocscreen");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_clearscreen, "clearscreen");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_setdrawmethod, "setdrawmethod");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_updateframe, "updateframe");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_performattack, "performattack");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_setidle, "setidle");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_getentity, "getentity");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_loadmodel, "loadmodel");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_loadsprite, "loadsprite");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_playgif, "playgif");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_strinfirst, "strinfirst");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_strinlast, "strinlast");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_strleft, "strleft");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_strlength, "strlength");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_strright, "strright");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_getmodelproperty, "getmodelproperty");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_changemodelproperty, "changemodelproperty");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_rgbcolor, "rgbcolor");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_zoom, "zoom");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_settexture, "settexture");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_setvertex, "setvertex");
	List_InsertAfter(&theFunctionList,
					  (void*)openbor_trianglelist, "trianglelist");
	//printf("Done!\n");

}

//////////////////////////////////////////////////////////
////////////   system functions
//////////////////////////////////////////////////////////
//isempty(var);
HRESULT system_isempty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	*pretvar = NULL;
	if(paramCount != 1) return E_FAIL;

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	(*pretvar)->lVal = (LONG)((varlist[0])->vt == VT_EMPTY );

	return S_OK;
}
//NULL();
HRESULT system_NULL(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	ScriptVariant_Clear(*pretvar);

	return S_OK;
}
HRESULT system_rand(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	(*pretvar)->lVal = (LONG)rand32();
	return S_OK;
}
//getglobalvar(varname);
HRESULT system_getglobalvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	ScriptVariant * ptmpvar;
	if(paramCount != 1)
	{
		*pretvar = NULL;
		return E_FAIL;
	}
	if(varlist[0]->vt != VT_STR)
	{
		printf("Function getglobalvar must have a string parameter.\n");
		*pretvar = NULL;
		return E_FAIL;
	}
	ptmpvar = Script_Get_Global_Variant(StrCache_Get(varlist[0]->strVal));
	if(ptmpvar) ScriptVariant_Copy(*pretvar, ptmpvar);
	else ScriptVariant_ChangeType(*pretvar, VT_EMPTY);
	return S_OK;
}
//maxglobalvarindex();
HRESULT system_maxglobalvarindex(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	(*pretvar)->lVal = (LONG)max_global_var_index;
	return S_OK;
}
//setglobalvar(varname, value);
HRESULT system_setglobalvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	if(paramCount != 2) {
		*pretvar = NULL;
		return E_FAIL;
	}
	if(varlist[0]->vt != VT_STR)
	{
		printf("Function setglobalvar's first parameter must be a string value.\n");
		*pretvar = NULL;
		return E_FAIL;
	}
	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

	(*pretvar)->lVal = (LONG)Script_Set_Global_Variant(StrCache_Get(varlist[0]->strVal), (varlist[1]));

	return S_OK;
}
//getlocalvar(varname);
HRESULT system_getlocalvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	ScriptVariant *ptmpvar;

	if(paramCount != 1) {
		*pretvar = NULL;
		return E_FAIL;
	}
	if(varlist[0]->vt != VT_STR)
	{
		printf("Function getlocalvar must have a string parameter.\n");
		*pretvar = NULL;
		return E_FAIL;
	}
	ptmpvar = Script_Get_Local_Variant(StrCache_Get(varlist[0]->strVal));
	if(ptmpvar) ScriptVariant_Copy(*pretvar,  ptmpvar);
	else        ScriptVariant_ChangeType(*pretvar, VT_EMPTY);
	return S_OK;
}
//setlocalvar(varname, value);
HRESULT system_setlocalvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	if(paramCount < 2) {
		*pretvar = NULL;
		return E_FAIL;
	}
	if(varlist[0]->vt != VT_STR)
	{
		printf("Function setlocalvar's first parameter must be a string value.\n");
		*pretvar = NULL;
		return E_FAIL;
	}
	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

	(*pretvar)->lVal = (LONG)Script_Set_Local_Variant(StrCache_Get(varlist[0]->strVal), varlist[1]);

	return S_OK;;
}
//clearlocalvar();
HRESULT system_clearlocalvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	*pretvar = NULL;
	Script_Local_Clear();
	return S_OK;
}
//clearglobalvar();
HRESULT system_clearglobalvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	*pretvar = NULL;
	max_global_var_index = -1;
	return S_OK;
}

//clearindexedvar();
HRESULT system_clearindexedvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
	*pretvar = NULL;
	for(i=0; i<max_indexed_vars; i++) ScriptVariant_Clear(indexed_var_list+i);
	return S_OK;
}

//free();
HRESULT system_free(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	*pretvar = NULL;
	if(paramCount<1) return E_FAIL;
	if(List_Includes(&scriptheap, varlist[0]->ptrVal))
	{
		free(List_Retrieve(&scriptheap));
		List_Remove(&scriptheap);
		return S_OK;
	}
	return E_FAIL;
}
//////////////////////////////////////////////////////////
////////////   openbor functions
//////////////////////////////////////////////////////////

// ===== openborvariant =====
void mapstrings_systemvariant(ScriptVariant** varlist, int paramCount)
{
	char* propname;
	int prop;

	// This enum is replicated in getsyspropertybyindex in openbor.c. If you
	// change one, you must change the other as well!!!!
	enum systemvariant_enum
	{
		_sv_branchname,
		_sv_count_enemies,
		_sv_count_entities,
		_sv_count_npcs,
		_sv_count_players,
		_sv_current_level,
		_sv_current_palette,
		_sv_current_scene,
		_sv_current_set,
		_sv_current_stage,
		_sv_effectvol,
		_sv_elapsed_time,
		_sv_ent_max,
		_sv_game_paused,
		_sv_game_speed,
		_sv_gfx_x_offset,
		_sv_gfx_y_offset,
		_sv_gfx_y_offset_adj,
		_sv_hResolution,
		_sv_in_gameoverscreen,
		_sv_in_halloffamescreen,
		_sv_in_level,
		_sv_in_menuscreen,
		_sv_in_selectscreen,
		_sv_in_titlescreen,
		_sv_lasthita,
		_sv_lasthitc,
		_sv_lasthitt,
		_sv_lasthitx,
		_sv_lasthitz,
		_sv_levelheight,
		_sv_levelwidth,
		_sv_lightx,
		_sv_lightz,
		_sv_maxentityvars,
		_sv_maxglobalvars,
		_sv_maxindexedvars,
		_sv_maxplayers,
		_sv_maxscriptvars,
		_sv_models_cached,
		_sv_models_loaded,
		_sv_musicvol,
		_sv_numpalettes,
		_sv_pause,
		_sv_pixelformat,
		_sv_player,
		_sv_player1,
		_sv_player2,
		_sv_player3,
		_sv_player4,
		_sv_player_max_z,
		_sv_player_min_z,
		_sv_shadowalpha,
		_sv_shadowcolor,
		_sv_slowmotion,
		_sv_slowmotion_duration,
		_sv_soundvol,
		_sv_totalram,
		_sv_freeram,
		_sv_usedram,
		_sv_vResolution,
		_sv_xpos,
		_sv_ypos,
		_sv_the_end,
	 };

	// arranged list, for searching
	static const char* proplist[] = {
		"branchname",
		"count_enemies",
		"count_entities",
		"count_npcs",
		"count_players",
		"current_level",
		"current_palette",
		"current_scene",
		"current_set",
		"current_stage",
		"effectvol",
		"elapsed_time",
		"ent_max",
		"game_paused",
		"game_speed",
		"gfx_x_offset",
		"gfx_y_offset",
		"gfx_y_offset_adj",
		"hResolution",
		"in_gameoverscreen",
		"in_halloffamescreen",
		"in_level",
		"in_menuscreen",
		"in_selectscreen",
		"in_titlescreen",
		"lasthita",
		"lasthitc",
		"lasthitt",
		"lasthitx",
		"lasthitz",
		"levelheight",
		"levelwidth",
		"lightx",
		"lightz",
		"maxentityvars",
		"maxglobalvars",
		"maxindexedvars",
		"maxplayers",
		"maxscriptvars",
		"models_cached",
		"models_loaded",
		"musicvol",
		"numpalettes",
		"pause",
		"pixelformat",
		"player",
		"player1",
		"player2",
		"player3",
		"player4",
		"player_max_z",
		"player_min_z",
		"shadowalpha",
		"shadowcolor",
		"slowmotion",
		"slowmotion_duration",
		"soundvol",
		"totalram",
		"freeram",
		"usedram",
		"vResolution",
		"xpos",
		"ypos",
	 };

	MAPSTRINGS(varlist[0], proplist, _sv_the_end,
		"openborvariant: System variable name not found: '%s'\n");
}

//sample function, used for getting a system variant
//openborvariant(varname);
HRESULT openbor_systemvariant(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	//used for getting the index from the enum of properties
	int variantindex = -1;
	//the paramCount used for checking.
	//check it first so the engine wont crash if the list is empty
	if(paramCount != 1)  goto systemvariant_error;
	//call this function's mapstrings function to map string constants to enum values
	mapstrings_systemvariant(varlist, paramCount);
	//the variant name should be here
	//you can check the argument type if you like
	if(varlist[0]->vt == VT_INTEGER)
		variantindex = varlist[0]->lVal;
	else  goto systemvariant_error;
	///////these should be your get method, ///////
	ScriptVariant_Clear(*pretvar);
	if(getsyspropertybyindex(*pretvar, variantindex))
	{
		return S_OK;
	}
	//else if
	//////////////////////////////////////////////
systemvariant_error:
	*pretvar = NULL;
	// we have finshed, so return
	return E_FAIL;
}


// ===== changeopenborvariant =====
void mapstrings_changesystemvariant(ScriptVariant** varlist, int paramCount)
{
	char* propname;
	int prop;

	// This enum is replicated in changesyspropertybyindex in openbor.c. If you
	// change one, you must change the other as well!!!!
	enum changesystemvariant_enum
	{
		_csv_blockade,
		_csv_elapsed_time,
		_csv_gfx_x_offset,
		_csv_gfx_y_offset,
		_csv_gfx_y_offset_adj,
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

	// arranged list, for searching
	static const char* proplist[] = {
		"blockade",
		"elapsed_time",
		"gfx_x_offset",
		"gfx_y_offset",
		"gfx_y_offset_adj",
		"lasthita",
		"lasthitc",
		"lasthitt",
		"lasthitx",
		"lasthitz",
		"levelpos",
		"scrollmaxz",
		"scrollminz",
		"slowmotion",
		"slowmotion_duration",
		"smartbomber",
		"textbox",
		"xpos",
		"ypos",
	 };

	 MAPSTRINGS(varlist[0], proplist, _csv_the_end,
		"changeopenborvariant: System variable name not found: '%s'\n");
}

//used for changing a system variant
//changeopenborvariant(varname, value);
HRESULT openbor_changesystemvariant(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	//used for getting the enum constant corresponding to the desired variable
	int variantindex = 0;
	//reference to the arguments
	ScriptVariant* arg = NULL;
	//the paramCount used for checking.
	//check it first so the engine wont crash if the list is empty
	if(paramCount != 2)   goto changesystemvariant_error;
	// map string constants to enum constants for speed
	mapstrings_changesystemvariant(varlist, paramCount);
	//get the 1st argument
	arg = varlist[0];
	//the variant name should be here
	//you can check the argument type if you like
	if(arg->vt == VT_INTEGER)
		variantindex = arg->lVal;
	else goto changesystemvariant_error;

	if(changesyspropertybyindex(variantindex, varlist[1]))
	{
		return S_OK;
	}
changesystemvariant_error:
	*pretvar = NULL;
	// we have finshed, so return
	return E_FAIL;

}

// use font_printf to draw string
//drawstring(x, y, font, string, z);
HRESULT openbor_drawstring(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
	char buf[256];
	LONG value[4];
	*pretvar = NULL;

	if(paramCount < 4) goto drawstring_error;

	for(i=0; i<3; i++)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i)))
			goto drawstring_error;
	}
	if(paramCount>4)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[4], value+3)))
			goto drawstring_error;
	}
	else value[3] = 0;
	ScriptVariant_ToString(varlist[3], buf);
	font_printf((int)value[0], (int)value[1], (int)value[2], (int)value[3], "%s", buf);
	return S_OK;

drawstring_error:
	printf("First 3 values must be integer values and 4th value a string: drawstring(int x, int y, int font, value)\n");
	return E_FAIL;
}

//use screen_printf
//drawstringtoscreen(screen, x, y, font, string);
HRESULT openbor_drawstringtoscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
	s_screen* scr;
	char buf[256];
	LONG value[3];
	*pretvar = NULL;

	if(paramCount != 5) goto drawstring_error;

	if(varlist[0]->vt!=VT_PTR) goto drawstring_error;
	scr = (s_screen*)varlist[0]->ptrVal;
	if(!scr)  goto drawstring_error;

	for(i=0; i<3; i++)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[i+1], value+i)))
			goto drawstring_error;
	}

	ScriptVariant_ToString(varlist[4], buf);
	screen_printf(scr, (int)value[0], (int)value[1], (int)value[2], "%s", buf);
	return S_OK;

drawstring_error:
	printf("Function needs a valid screen handle, 3 integers and a string value: drawstringtoscreen(screen, int font, value)\n");
	return E_FAIL;
}

// debug purpose
//log(string);
HRESULT openbor_log(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	char buf[256];
	*pretvar = NULL;

	if(paramCount != 1) goto drawstring_error;

	ScriptVariant_ToString(varlist[0], buf);
	printf("%s", buf);
	return S_OK;

drawstring_error:
	printf("Function needs 1 parameter: log(value)\n");
	return E_FAIL;
}

//drawbox(x, y, width, height, z, color, lut);
HRESULT openbor_drawbox(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
	LONG value[6], l;
	*pretvar = NULL;

	if(paramCount < 6) goto drawbox_error;

	for(i=0; i<6; i++)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i)))
			goto drawbox_error;
	}

	if(paramCount > 6)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[6], &l)))
			goto drawbox_error;
	}
	else l = -1;

	if(l >= 0)
	{
		l %= MAX_BLENDINGS+1;
	}
	spriteq_add_box((int)value[0], (int)value[1], (int)value[2], (int)value[3], (int)value[4], (int)value[5], l);

	return S_OK;

drawbox_error:
	printf("Function requires 6 integer values: drawbox(int x, int y, int width, int height, int z, int color, int lut)\n");
	return E_FAIL;
}

//drawboxtoscreen(screen, x, y, width, height, color, lut);
HRESULT openbor_drawboxtoscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
	s_screen* s;
	LONG value[5], l;
	*pretvar = NULL;

	if(paramCount < 6) goto drawbox_error;

	s = (s_screen*)varlist[0]->ptrVal;

	if(!s) goto drawbox_error;

	for(i=1; i<6; i++)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i-1)))
			goto drawbox_error;
	}

	if(paramCount > 6)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[6], &l)))
			goto drawbox_error;
	}
	else l = -1;

	if(l >= 0)
	{
		l %= MAX_BLENDINGS+1;
	}
	switch(s->pixelformat)
	{
	case PIXEL_8:
		drawbox((int)value[0], (int)value[1], (int)value[2], (int)value[3], (int)value[4], s, l);
		break;
	case PIXEL_16:
		drawbox16((int)value[0], (int)value[1], (int)value[2], (int)value[3], (int)value[4], s, l);
		break;
	case PIXEL_32:
		drawbox32((int)value[0], (int)value[1], (int)value[2], (int)value[3], (int)value[4], s, l);
		break;
	}

	return S_OK;

drawbox_error:
	printf("Function requires a screen handle and 5 integer values, 7th integer value is optional: drawboxtoscreen(screen, int x, int y, int width, int height, int color, int lut)\n");
	return E_FAIL;
}

//drawline(x1, y1, x2, y2, z, color, lut);
HRESULT openbor_drawline(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
	LONG value[6], l;
	*pretvar = NULL;

	if(paramCount < 6) goto drawline_error;

	for(i=0; i<6; i++)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i)))
			goto drawline_error;
	}

	if(paramCount > 6)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[6], &l)))
			goto drawline_error;
	}
	else l = -1;

	if(l >=0 )
	{
		l %= MAX_BLENDINGS+1;
	}
	spriteq_add_line((int)value[0], (int)value[1], (int)value[2], (int)value[3], (int)value[4], (int)value[5], l);

	return S_OK;

drawline_error:
	printf("Function requires 6 integer values, 7th integer value is optional: drawline(int x1, int y1, int x2, int y2, int z, int color, int lut)\n");
	return E_FAIL;
}

//drawlinetoscreen(screen, x1, y1, x2, y2, color, lut);
HRESULT openbor_drawlinetoscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
	LONG value[5], l;
	s_screen *s;
	*pretvar = NULL;

	if(paramCount < 6) goto drawline_error;

	s = (s_screen*)varlist[0]->ptrVal;

	if(!s) goto drawline_error;

	for(i=1; i<6; i++)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i-1)))
			goto drawline_error;
	}

	if(paramCount > 6)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[6], &l)))
			goto drawline_error;
	}
	else l = -1;

	if(l >=0 )
	{
		l %= MAX_BLENDINGS+1;
	}
	switch(s->pixelformat)
	{
	case PIXEL_8:
		line((int)value[0], (int)value[1], (int)value[2], (int)value[3], (int)value[4], s, l);
		break;
	case PIXEL_16:
		line16((int)value[0], (int)value[1], (int)value[2], (int)value[3], (int)value[4], s, l);
		break;
	case PIXEL_32:
		line32((int)value[0], (int)value[1], (int)value[2], (int)value[3], (int)value[4], s, l);
		break;
	}

	return S_OK;
drawline_error:
	printf("Function requires a screen handle and 5 integer values, 7th integer value is optional: drawlinetoscreen(screen, int x1, int y1, int x2, int y2, int color, int lut)\n");
	return E_FAIL;
}

//drawsprite(sprite, x, y, z, sortid);
HRESULT openbor_drawsprite(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
	LONG value[4];
	s_sprite* spr;
	*pretvar = NULL;

	if(paramCount < 4) goto drawsprite_error;
	if(varlist[0]->vt!=VT_PTR) goto drawsprite_error;

	spr = varlist[0]->ptrVal;
	if(!spr) goto drawsprite_error;

	value[3] = (LONG)0;
	for(i=1; i<paramCount && i<5; i++)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i-1)))
			goto drawsprite_error;
	}

	spriteq_add_frame((int)value[0], (int)value[1], (int)value[2], spr, &drawmethod, (int)value[3]);

	return S_OK;

drawsprite_error:
	printf("Function requires a valid sprite handle 3 integer values, 5th integer value is optional: drawsprite(sprite, int x, int y, int z, int sortid)\n");
	return E_FAIL;
}

//drawspritetoscreen(sprite, screen, x, y);
HRESULT openbor_drawspritetoscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
	LONG value[2];
	s_sprite* spr;
	s_screen* scr;
	*pretvar = NULL;

	if(paramCount < 4) goto drawsprite_error;
	if(varlist[0]->vt!=VT_PTR) goto drawsprite_error;
	spr = varlist[0]->ptrVal;
	if(!spr) goto drawsprite_error;

	if(varlist[1]->vt!=VT_PTR) goto drawsprite_error;
	scr = varlist[1]->ptrVal;
	if(!scr) goto drawsprite_error;

	for(i=2; i<paramCount && i<4; i++)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i-2)))
			goto drawsprite_error;
	}

	putsprite((int)value[0], (int)value[1], spr, scr, &drawmethod);

	return S_OK;

drawsprite_error:
	printf("Function requires a valid sprite handle, a valid screen handle and 2 integer values: drawspritetoscreen(sprite, screen, int x, int y)\n");
	return E_FAIL;
}

//setvertex(index, x, y, tx, ty)
HRESULT openbor_setvertex(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG value[5], i;
	*pretvar = NULL;

	if(paramCount<5) goto vertex_error;

	for(i=0; i<5; i++)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i)))
			goto vertex_error;
	}

	if(value[0]<0 || value[0]>=vert_buf_size) goto vertex_error2;

	verts[value[0]].x = value[1];
	verts[value[0]].y = value[2];
	verts[value[0]].tx = value[3];
	verts[value[0]].ty = value[4];

	return S_OK;

vertex_error:
	printf("Function requires 5 integer values: setvertex(index, x, y, tx, ty)\n");
	return E_FAIL;

vertex_error2:
	printf("Index out of range in function setvertext: range from 0 to %d, %ld is given.\n", vert_buf_size-1, value[0]);
	return E_FAIL;
}

//settexture(handle, type)
HRESULT openbor_settexture(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG	type;
	*pretvar = NULL;

	if(paramCount<2) goto texture_error;

	if(varlist[0]->vt!=VT_PTR) goto texture_error;

	if(FAILED(ScriptVariant_IntegerValue(varlist[1], &type)))
		goto texture_error;

	switch(type){
	case 0:
		texture.type = gfx_screen;
		texture.screen = (s_screen*)varlist[0]->ptrVal;
		break;
	case 1:
		texture.type = gfx_bitmap;
		texture.bitmap = (s_bitmap*)varlist[0]->ptrVal;
		break;
	case 2:	
		texture.type = gfx_sprite;
		texture.sprite = (s_sprite*)varlist[0]->ptrVal;
		break;
	default:
		goto texture_error2;
		break;
	}

	return S_OK;

texture_error:
	printf("Function requires a valid texture handle and a integer values: settexture(handle, type)\n");
	return E_FAIL;

texture_error2:
	printf("Invalid texture type for function settexture: %ld\n", type);
	return E_FAIL;
}

// TODO: add a new entry type to the sprite queue (may not happen since you can always use a buffer screen)
//trianglelist(screen, start, count);  //screen could be NULL
HRESULT openbor_trianglelist(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	s_screen* scr = NULL;
	extern s_screen* vscreen;
	LONG triangle_start = 0;
	LONG triangle_count = 2;
	*pretvar = NULL;

	if(paramCount<3) goto trianglelist_error;

	if(varlist[0]->vt!=VT_PTR && varlist[0]->vt!=VT_EMPTY) goto trianglelist_error;
	scr = (s_screen*)varlist[0]->ptrVal;

	if(!scr) scr = vscreen;

	if(FAILED(ScriptVariant_IntegerValue(varlist[1], &triangle_start)))
		goto trianglelist_error;

	if(FAILED(ScriptVariant_IntegerValue(varlist[2], &triangle_count)))
		goto trianglelist_error;

	if(triangle_count<=0) return S_OK; // though we does nothing

	 //check for overflow
	if(triangle_start<0) triangle_start = 0;
	else if(triangle_start>vert_buf_size-3) triangle_start = vert_buf_size-3;
	//check for overflow
	if(triangle_count>vert_buf_size-triangle_start-2) triangle_count = vert_buf_size-triangle_start-2;

	//go ahead and draw
	draw_triangle_list(verts + triangle_start, scr, &texture, &drawmethod, triangle_count);

	return S_OK;

trianglelist_error:
	printf("Function requires a valid screen handle(can be NULL) and two integer values: trianglelist(screen, start, count)\n");
	return E_FAIL;
}


//drawdot(x, y, z, color, lut);
HRESULT openbor_drawdot(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
	LONG value[4], l;
	*pretvar = NULL;

	if(paramCount < 4) goto drawdot_error;

	for(i=0; i<4; i++)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i)))
			goto drawdot_error;
	}

	if(paramCount > 4)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[4], &l)))
			goto drawdot_error;
	}
	else l = -1;

	if(l >=0 )
	{
		l %= MAX_BLENDINGS+1;
	}
	spriteq_add_dot((int)value[0], (int)value[1], (int)value[2], (int)value[3], l);

	return S_OK;

drawdot_error:
	printf("Function requires 4 integer values, 5th integer value is optional: drawdot(int x, int y, int z, int color, int lut)\n");
	return E_FAIL;
}

//drawdottoscreen(screen, x, y, color, lut);
HRESULT openbor_drawdottoscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
	LONG value[3], l;
	s_screen* s;
	*pretvar = NULL;

	if(paramCount < 4) goto drawdot_error;

	s = (s_screen*)varlist[0]->ptrVal;

	if(!s) goto drawdot_error;

	for(i=1; i<4; i++)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i-1)))
			goto drawdot_error;
	}

	if(paramCount > 4)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[4], &l)))
			goto drawdot_error;
	}
	else l = -1;

	if(l >=0 )
	{
		l %= MAX_BLENDINGS+1;
	}
	switch(s->pixelformat)
	{
	case PIXEL_8:
		putpixel((int)value[0], (int)value[1], (int)value[2], s, l);
		break;
	case PIXEL_16:
		putpixel16((int)value[0], (int)value[1], (int)value[2], s, l);
		break;
	case PIXEL_32:
		putpixel32((int)value[0], (int)value[1], (int)value[2], s, l);
		break;
	}

	return S_OK;

drawdot_error:
	printf("Function requires a screen handle and 3 integer values, 5th integer value is optional: dottoscreen(screen, int x, int y, int color, int lut)\n");
	return E_FAIL;
}


//drawscreen(screen, x, y, z, lut);
HRESULT openbor_drawscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
	LONG value[3], l;
	s_screen* s;
	extern s_drawmethod plainmethod;
	s_drawmethod screenmethod;
	*pretvar = NULL;

	if(paramCount < 4) goto drawscreen_error;

	s = (s_screen*)varlist[0]->ptrVal;

	if(!s) goto drawscreen_error;

	for(i=1; i<4; i++)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i-1)))
			goto drawscreen_error;
	}

	if(paramCount > 4)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[4], &l)))
			goto drawscreen_error;
	}
	else l = -1;

	if(l >=0 )
	{
		l %= MAX_BLENDINGS+1;
	}
	if(paramCount<=4) screenmethod = drawmethod;
	else
	{
		screenmethod = plainmethod;
		screenmethod.alpha = l;
		screenmethod.transbg = 1;
	}

	spriteq_add_screen((int)value[0], (int)value[1], (int)value[2], s, &screenmethod, 0);

	return S_OK;

drawscreen_error:
	printf("Function requires a screen handle and 3 integer values, 5th integer value is optional: drawscreen(screen, int x, int y, int z, int lut)\n");
	return E_FAIL;
}

//getindexedvar(int index);
HRESULT openbor_getindexedvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG ind;

	if(paramCount < 1 || max_indexed_vars<=0)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	ScriptVariant_Clear(*pretvar);

	if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
	{
		printf("Function requires 1 numberic value: getindexedvar(int index)\n");
		*pretvar = NULL;
		return E_FAIL;
	}

	if(ind<0 || ind>=max_indexed_vars) return S_OK;

	ScriptVariant_Copy(*pretvar, indexed_var_list+ind);

	return S_OK;
}

//setindexedvar(int index, var);
HRESULT openbor_setindexedvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG ind;

	if(paramCount < 2 || max_indexed_vars<=0)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
	{
		printf("Function's 1st argument must be a numberic value: setindexedvar(int index, var)\n");
		*pretvar = NULL;
		return E_FAIL;
	}

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

	if(ind<0 || ind>=max_indexed_vars)
	{
		(*pretvar)->lVal = 0;
		return S_OK;
	}

	ScriptVariant_Copy(indexed_var_list+ind, varlist[1]);
	(*pretvar)->lVal = 1;

	return S_OK;
}

//getscriptvar(int index);
HRESULT openbor_getscriptvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG ind;

	if(paramCount < 1 || max_script_vars<=0 || !pcurrentscript || !pcurrentscript->vars)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
	{
		printf("Function requires 1 numberic value: getscriptvar(int index)\n");
		*pretvar = NULL;
		return E_FAIL;
	}

	ScriptVariant_Clear(*pretvar);

	if(ind<0 || ind>=max_script_vars) return S_OK;

	ScriptVariant_Copy(*pretvar, pcurrentscript->vars+ind);

	return S_OK;
}

//setscriptvar(int index, var);
HRESULT openbor_setscriptvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG ind;

	if(paramCount < 2 || max_script_vars<=0 || !pcurrentscript || !pcurrentscript->vars)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
	{
		printf("Function's 1st argument must be a numberic value: setscriptvar(int index, var)\n");
		*pretvar = NULL;
		return E_FAIL;
	}

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

	if(ind<0 || ind>=max_script_vars)
	{
		(*pretvar)->lVal = 0;
		return S_OK;
	}

	ScriptVariant_Copy(pcurrentscript->vars+ind, varlist[1]);
	(*pretvar)->lVal = 1;

	return S_OK;
}

//getentityvar(entity, int index);
HRESULT openbor_getentityvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	ScriptVariant* arg = NULL;
	LONG ind;
	entity* ent;

	if(paramCount < 2 || max_entity_vars<=0 )
	{
		*pretvar = NULL;
		return E_FAIL;
	}


	arg = varlist[0];

	ScriptVariant_Clear(*pretvar);
	if(arg->vt == VT_EMPTY) ent= NULL;
	else if(arg->vt == VT_PTR) ent = (entity*)arg->ptrVal;
	else
	{
		printf("Function's 1st argument must be a valid entity handle value or empty value: getentityvar(entity, int index)\n");
		*pretvar = NULL;
		return E_FAIL;
	}
	if(!ent || !ent->entvars) return S_OK;

	if(FAILED(ScriptVariant_IntegerValue(varlist[1], &ind)))
	{
		printf("Function's 2nd argument must be a numberic value: getentityvar(entity, int index)\n");
		*pretvar = NULL;
		return E_FAIL;
	}

	if(ind<0 || ind>=max_entity_vars) return S_OK;

	ScriptVariant_Copy(*pretvar, ent->entvars+ind);

	return S_OK;
}

//setentityvar(int index, var);
HRESULT openbor_setentityvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	ScriptVariant* arg = NULL;
	LONG ind;
	entity* ent;

	if(paramCount < 3 || max_entity_vars<=0)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	arg = varlist[0];

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	(*pretvar)->lVal = 0;

	if(arg->vt == VT_EMPTY) ent = NULL;
	else if(arg->vt == VT_PTR)
		ent = (entity*)arg->ptrVal;
	else
	{
		printf("Function's 1st argument must be a valid entity handle value or empty value: setentityvar(entity, int index, var)\n");
		*pretvar = NULL;
		return E_FAIL;
	}
	if(!ent || !ent->entvars) return S_OK;

	if(FAILED(ScriptVariant_IntegerValue(varlist[1], &ind)))
	{
		printf("Function's 2nd argument must be a numberic value: setentityvar(entity, int index, var)\n");
		*pretvar = NULL;
		return E_FAIL;
	}

	if(ind<0 || ind>=max_entity_vars) return S_OK;

	ScriptVariant_Copy(ent->entvars+ind, varlist[2]);
	(*pretvar)->lVal = 1;

	return S_OK;
}

//strinfirst(char string, char search_string);
HRESULT openbor_strinfirst(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	char* tempstr = NULL;

	if(paramCount < 2)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	ScriptVariant_Clear(*pretvar);

	if(varlist[0]->vt!=VT_STR || varlist[1]->vt!=VT_STR)
	{
		printf("\n Error, strinfirst({string}, {search string}): Strinfirst must be passed valid {string} and {search string}. \n");
	}

	tempstr = strstr((char*)StrCache_Get(varlist[0]->strVal), (char*)StrCache_Get(varlist[1]->strVal));

	if (tempstr != NULL)
	{
		ScriptVariant_ChangeType(*pretvar, VT_STR);
		strcpy(StrCache_Get((*pretvar)->strVal), tempstr);
	}
	else
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = -1;
	}
	return S_OK;
}

//strinlast(char string, char search_string);
HRESULT openbor_strinlast(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	char* tempstr = NULL;

	if(paramCount < 2)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	ScriptVariant_Clear(*pretvar);

	if(varlist[0]->vt!=VT_STR || varlist[1]->vt!=VT_STR)
	{
		printf("\n Error, strinlast({string}, {search string}): Strinlast must be passed valid {string} and {search string}. \n");
	}

	tempstr = strrchr((char*)StrCache_Get(varlist[0]->strVal), varlist[1]->strVal);

	if (tempstr != NULL)
	{
		ScriptVariant_ChangeType(*pretvar, VT_STR);
		strcpy(StrCache_Get((*pretvar)->strVal), tempstr);
	}
	else
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = -1;
	}
	return S_OK;
}

//strleft(char string, int i);
HRESULT openbor_strleft(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	char tempstr[66] = {0};

	if(paramCount < 2)
	{
		*pretvar = NULL;
		return E_FAIL;
	}
	if(varlist[0]->vt!=VT_STR || varlist[1]->vt!=VT_INTEGER)
	{
		printf("\n Error, strleft({string}, {characters}): Invalid or missing parameter. Strleft must be passed valid {string} and number of {characters}.\n");
	}

	strncpy(tempstr, (char*)StrCache_Get(varlist[0]->strVal), varlist[1]->lVal);
	ScriptVariant_Clear(*pretvar);

	if (tempstr != NULL)
	{
		ScriptVariant_ChangeType(*pretvar, VT_STR);
		strcpy(StrCache_Get((*pretvar)->strVal),tempstr);
	}
	else
	{
		 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = -1;
	}

	return S_OK;
}

//strlength(char string);
HRESULT openbor_strlength(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	if(paramCount < 1)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	ScriptVariant_Clear(*pretvar);

	if(varlist[0]->vt!=VT_STR)
	{
		printf("Error, strlength({string}): Invalid or missing parameter. Strlength must be passed a valid {string}.\n");
	}

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	(*pretvar)->lVal = strlen((char*)StrCache_Get(varlist[0]->strVal));
	return S_OK;
}

//strright(char string, int i);
HRESULT openbor_strright(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	char* tempstr = NULL;

	if(paramCount < 2)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	if(varlist[0]->vt!=VT_STR || varlist[1]->vt!=VT_INTEGER)
	{
		printf("\n Error, strright({string}, {characters}): Invalid or missing parameter. Strright must be passed valid {string} and number of {characters}.\n");
	}

	ScriptVariant_Clear(*pretvar);
	tempstr = (char*)StrCache_Get(varlist[0]->strVal);

	if (tempstr != NULL || strlen(tempstr)>0)
	{
		ScriptVariant_ChangeType(*pretvar, VT_STR);
		strcpy(StrCache_Get((*pretvar)->strVal), &tempstr[varlist[1]->lVal]);
	}
	else
	{
		 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = -1;
	}

	return S_OK;
}

HRESULT openbor_getmodelproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int iArg;

	if(paramCount < 2)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	if(varlist[0]->vt!=VT_INTEGER || varlist[1]->vt!=VT_INTEGER)
	{
		printf("\n Error, getmodelproperty({model}, {property}): Invalid or missing parameter. Getmodelproperty must be passed valid {model} and {property} indexes.\n");
	}

	iArg = varlist[0]->lVal;

	switch (varlist[1]->lVal)
	{
		case 0:                                                    //Loaded?
		{
			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)model_cache[iArg].loadflag;
			break;
		}
		case 1:
		{
			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (VOID*)model_cache[iArg].model;
		}
		case 2:
		{
			ScriptVariant_ChangeType(*pretvar, VT_STR);
			strcpy(StrCache_Get((*pretvar)->strVal), model_cache[iArg].name);
			break;
		}
		case 3:
		{
			ScriptVariant_ChangeType(*pretvar, VT_STR);
			strcpy(StrCache_Get((*pretvar)->strVal), model_cache[iArg].path);
			break;
		}
		case 4:                                                    //Loaded?
		{
			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)model_cache[iArg].selectable;
			break;
		}
	}

	return S_OK;
}

HRESULT openbor_changemodelproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int iArg;
	LONG ltemp;

	if(paramCount < 2)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	if(varlist[0]->vt!=VT_INTEGER || varlist[1]->vt!=VT_INTEGER)
	{
		printf("\n Error, changemodelproperty({model}, {property}, {value}): Invalid or missing parameter. Changemodelproperty must be passed valid {model}, {property} and {value}.\n");
	}

	iArg = varlist[0]->lVal;

	switch (varlist[1]->lVal)
	{
		case 0:                                                    //Loaded?
		{
			/*
			if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
				model_cache[iArg].loadflag = (int)ltemp;
			else (*pretvar)->lVal = (LONG)0;
			break;
			*/
		}
		case 1:
		{
			/*
			if(varlist[2]->vt != VT_STR)
			{
				printf("You must give a string value for {value}.\n");
				goto changeentityproperty_error;
			}
			strcpy(model_cache[iArg].model, (char*)StrCache_Get(varlist[2]->strVal));
			(*pretvar)->lVal = (LONG)1;
			break;
			*/
		}
		case 2:
		{
			/*
			if(varlist[2]->vt != VT_STR)
			{
				printf("You must give a string value for {value}.\n");
				goto changeentityproperty_error;
			}
			strcpy(model_cache[iArg].name, (char*)StrCache_Get(varlist[2]->strVal));
			(*pretvar)->lVal = (LONG)1;
			break;
			*/
		}
		case 3:
		{
			/*
			if(varlist[2]->vt != VT_STR)
			{
				printf("You must give a string value for {value}.\n");
				goto changeentityproperty_error;
			}
			strcpy(model_cache[iArg].path, (char*)StrCache_Get(varlist[2]->strVal));
			(*pretvar)->lVal = (LONG)1;
			break;
			*/
		}
		case 4:
		{
			if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
				model_cache[iArg].selectable = (int)ltemp;
			else (*pretvar)->lVal = (LONG)0;
			break;
		}
	}

	return S_OK;
}

// ===== getentityproperty =====
enum getentityproperty_enum {
	_gep_a,
	_gep_aggression,
	_gep_aiattack,
	_gep_aiflag,
	_gep_aimove,
	_gep_alpha,
	_gep_animal,
	_gep_animating,
	_gep_animation,
	_gep_animationid,
	_gep_animheight,
	_gep_animhits,
	_gep_animnum,
	_gep_animpos,
	_gep_animvalid,
	_gep_antigravity,
	_gep_attack,
	_gep_attacking,
	_gep_attackid,
	_gep_autokill,
	_gep_base,
	_gep_bbox,
	_gep_blink,
	_gep_blockback,
	_gep_blockodds,
	_gep_blockpain,
	_gep_bounce,
	_gep_chargerate,
	_gep_colourmap,
	_gep_colourtable,
	_gep_damage_on_landing,
	_gep_dead,
	_gep_defaultmodel,
	_gep_defaultname,
	_gep_defense,
	_gep_detect,
	_gep_direction,
	_gep_dot,
	_gep_dropframe,
	_gep_edelay,
	_gep_energycost,
	_gep_escapecount,
	_gep_escapehits,
	_gep_exists,
	_gep_falldie,
	_gep_flash,
	_gep_freezetime,
	_gep_frozen,
	_gep_gfxshadow,
	_gep_grabbing,
	_gep_grabforce,
	_gep_guardpoints,
	_gep_health,
	_gep_height,
	_gep_hitbyid,
	_gep_hmapl,
	_gep_hmapu,
	_gep_icon,
	_gep_invincible,
	_gep_invinctime,
	_gep_jugglepoints,
	_gep_knockdowncount,
	_gep_komap,
	_gep_landframe,
	_gep_lifespancountdown,
	_gep_link,
	_gep_map,
	_gep_mapcount,
	_gep_mapdefault,
	_gep_maps,
	_gep_maxguardpoints,
	_gep_maxhealth,
	_gep_maxjugglepoints,
	_gep_maxmp,
	_gep_model,
	_gep_mp,
	_gep_mpdroprate,
	_gep_mprate,
	_gep_mpstable,
	_gep_mpstableval,
	_gep_name,
	_gep_nextanim,
	_gep_nextthink,
	_gep_no_adjust_base,
	_gep_noaicontrol,
	_gep_nodieblink,
	_gep_nodrop,
	_gep_nograb,
	_gep_nopain,
	_gep_offense,
	_gep_opponent,
	_gep_owner,
	_gep_pain_time,
	_gep_parent,
	_gep_path,
	_gep_playerindex,
	_gep_projectile,
	_gep_range,
	_gep_running,
	_gep_rush_count,
	_gep_rush_tally,
	_gep_rush_time,
	_gep_score,
	_gep_scroll,
	_gep_seal,
	_gep_sealtime,
	_gep_setlayer,
	_gep_spawntype,
	_gep_speed,
	_gep_sprite,
	_gep_spritea,
	_gep_stalltime,
	_gep_stats,
	_gep_staydown,
	_gep_stealth,
	_gep_subentity,
	_gep_subject_to_gravity,
	_gep_subject_to_hole,
	_gep_subject_to_maxz,
	_gep_subject_to_minz,
	_gep_subject_to_obstacle,
	_gep_subject_to_platform,
	_gep_subject_to_screen,
	_gep_subject_to_wall,
	_gep_subtype,
	_gep_thold,
	_gep_throwdamage,
	_gep_throwdist,
	_gep_throwframewait,
	_gep_throwheight,
	_gep_tosstime,
	_gep_tossv,
	_gep_type,
	_gep_weapent,
	_gep_x,
	_gep_xdir,
	_gep_z,
	_gep_zdir,
	_gep_the_end,
};

enum gep_aiflag_enum {
	_gep_aiflag_animating,
	_gep_aiflag_attacking,
	_gep_aiflag_autokill,
	_gep_aiflag_blink,
	_gep_aiflag_blocking,
	_gep_aiflag_charging,
	_gep_aiflag_dead,
	_gep_aiflag_drop,
	_gep_aiflag_falling,
	_gep_aiflag_freezetime,
	_gep_aiflag_frozen,
	_gep_aiflag_getting,
	_gep_aiflag_idling,
	_gep_aiflag_inpain,
	_gep_aiflag_invincible,
	_gep_aiflag_jumpid,
	_gep_aiflag_jumping,
	_gep_aiflag_pain_time,
	_gep_aiflag_projectile,
	_gep_aiflag_running,
	_gep_aiflag_toexplode,
	_gep_aiflag_turning,
	_gep_aiflag_the_end,
};

enum gep_attack_enum {
	_gep_attack_blast,
	_gep_attack_blockflash,
	_gep_attack_blocksound,
	_gep_attack_coords,
	_gep_attack_counterattack,
	_gep_attack_direction,
	_gep_attack_dol,
	_gep_attack_dot,
	_gep_attack_dotforce,
	_gep_attack_dotindex,
	_gep_attack_dotrate,
	_gep_attack_dottime,
	_gep_attack_drop,
	_gep_attack_dropv,
	_gep_attack_force,
	_gep_attack_forcemap,
	_gep_attack_freeze,
	_gep_attack_freezetime,
	_gep_attack_grab,
	_gep_attack_grabdistance,
	_gep_attack_guardcost,
	_gep_attack_hitflash,
	_gep_attack_hitsound,
	_gep_attack_jugglecost,
	_gep_attack_maptime,
	_gep_attack_noblock,
	_gep_attack_noflash,
	_gep_attack_nopain,
	_gep_attack_otg,
	_gep_attack_pause,
	_gep_attack_seal,
	_gep_attack_sealtime,
	_gep_attack_staydown,
	_gep_attack_steal,
	_gep_attack_type,
	_gep_attack_the_end,
};

enum _gep_defense_enum {
    _gep_defense_blockpower,
    _gep_defense_blockratio,
    _gep_defense_blockthreshold,
    _gep_defense_blocktype,
    _gep_defense_factor,
    _gep_defense_knockdown,
	_gep_defense_pain,
	_gep_defense_the_end,
};

enum gep_dot_enum {
	_gep_dot_force,
	_gep_dot_mode,
	_gep_dot_owner,
	_gep_dot_rate,
	_gep_dot_time,
	_gep_dot_type,
	_gep_dot_the_end,
};

enum gep_edelay_enum {
	_gep_edelay_cap_max,
	_gep_edelay_cap_min,
	_gep_edelay_factor,
	_gep_edelay_mode,
	_gep_edelay_range_max,
	_gep_edelay_range_min,
	_gep_edelay_the_end,
};

enum gep_energycost_enum {
    _gep_energycost_cost,
    _gep_energycost_disable,
    _gep_energycost_mponly,
    _gep_energycost_the_end,
};

enum gep_flash_enum {
    _gep_flash_block,
    _gep_flash_def,
    _gep_flash_noattack,
    _gep_flash_the_end,
};

enum gep_icon_enum {
    _gep_icon_def,
    _gep_icon_die,
    _gep_icon_get,
    _gep_icon_mphigh,
    _gep_icon_mplow,
    _gep_icon_mpmed,
    _gep_icon_pain,
    _gep_icon_weapon,
    _gep_icon_x,
    _gep_icon_y,
    _gep_icon_the_end,
};

enum _gep_knockdowncount_enum {
    _gep_knockdowncount_current,
    _gep_knockdowncount_max,
    _gep_knockdowncount_time,
    _gep_knockdowncount_the_end,
};

enum gep_landframe_enum {
    _gep_landframe_ent,
    _gep_landframe_frame,
    _gep_landframe_the_end,
};

enum gep_maps_enum {
    _gep_maps_count,
    _gep_maps_current,
    _gep_maps_default,
    _gep_maps_dying,
    _gep_maps_dying_critical,
    _gep_maps_dying_low,
    _gep_maps_frozen,
    _gep_maps_hide_end,
    _gep_maps_hide_start,
    _gep_maps_ko,
    _gep_maps_kotype,
    _gep_maps_table,
    _gep_maps_time,
    _gep_maps_the_end,
};

enum gep_range_enum {
    _gep_range_amax,
    _gep_range_amin,
    _gep_range_bmax,
    _gep_range_bmin,
    _gep_range_xmax,
    _gep_range_xmin,
    _gep_range_zmax,
    _gep_range_zmin,
    _gep_range_the_end,
};

enum gep_running_enum {
	_gep_running_jumpx,
	_gep_running_jumpy,
	_gep_running_land,
	_gep_running_movez,
	_gep_running_speed,
	_gep_running_the_end,
};

enum gep_spritea_enum {
    _gep_spritea_centerx,
    _gep_spritea_centery,
    _gep_spritea_file,
    _gep_spritea_offsetx,
    _gep_spritea_offsety,
    _gep_spritea_sprite,
    _gep_spritea_the_end,
};

enum gep_staydown_enum {
    _gep_staydown_rise,
    _gep_staydown_riseattack,
    _gep_staydown_riseattack_stall,
    _gep_staydown_the_end,
};

void mapstrings_getentityproperty(ScriptVariant** varlist, int paramCount)
{
	char* propname;
	int prop;

	// arranged list, for searching
	static const char* proplist[] = {
		"a",
		"aggression",
		"aiattack",
		"aiflag",
		"aimove",
		"alpha",
		"animal",
		"animating",
		"animation",
		"animationid",
		"animheight",
		"animhits",
		"animnum",
		"animpos",
		"animvalid",
		"antigravity",
		"attack",
		"attacking",
		"attackid",
		"autokill",
		"base",
		"bbox",
		"blink",
		"blockback",
		"blockodds",
		"blockpain",
		"bounce",
		"chargerate",
		"colourmap",
		"colourtable",
		"damage_on_landing",
		"dead",
		"defaultmodel",
		"defaultname",
		"defense",
		"detect",
		"direction",
		"dot",
		"dropframe",
		"edelay",
		"energycost",
		"escapecount",
		"escapehits",
		"exists",
		"falldie",
		"flash",
		"freezetime",
		"frozen",
		"gfxshadow",
		"grabbing",
		"grabforce",
		"guardpoints",
		"health",
		"height",
		"hitbyid",
		"hmapl",
		"hmapu",
		"icon",
		"invincible",
		"invinctime",
		"jugglepoints",
		"knockdowncount",
		"komap",
		"landframe",
		"lifespancountdown",
		"link",
		"map",
		"mapcount",
		"mapdefault",
		"maps",
		"maxguardpoints",
		"maxhealth",
		"maxjugglepoints",
		"maxmp",
		"model",
		"mp",
		"mpdroprate",
		"mprate",
		"mpstable",
		"mpstableval",
		"name",
		"nextanim",
		"nextthink",
		"no_adjust_base",
		"noaicontrol",
		"nodieblink",
		"nodrop",
		"nograb",
		"nopain",
		"offense",
		"opponent",
		"owner",
		"pain_time",
		"parent",
		"path",
		"playerindex",
		"projectile",
		"range",
		"running",
		"rush_count",
		"rush_tally",
		"rush_time",
		"score",
		"scroll",
		"seal",
		"sealtime",
		"setlayer",
		"spawntype",
		"speed",
		"sprite",
		"spritea",
		"stalltime",
		"stats",
		"staydown",
		"stealth",
		"subentity",
		"subject_to_gravity",
		"subject_to_hole",
		"subject_to_maxz",
		"subject_to_minz",
		"subject_to_obstacle",
		"subject_to_platform",
		"subject_to_screen",
		"subject_to_wall",
		"subtype",
		"thold",
		"throwdamage",
		"throwdist",
		"throwframewait",
		"throwheight",
		"tosstime",
		"tossv",
		"type",
		"weapent",
		"x",
		"xdir",
		"z",
		"zdir",
	};

	static const char* proplist_aiflag[] = {
		"animating",
		"attacking",
		"autokill",
		"blink",
		"blocking",
		"charging",
		"dead",
		"drop",
		"falling",
		"freezetime",
		"frozen",
		"getting",
		"idling",
		"inpain",
		"invincible",
		"jumpid",
		"jumping",
		"pain_time",
		"projectile",
		"running",
		"toexplode",
		"turning",
	};

    static const char* proplist_attack[] = {
		"blast",
		"blockflash",
		"blocksound",
		"coords",
		"counterattack",
		"direction",
		"dol",
		"dot",
		"dotforce",
		"dotindex",
		"dotrate",
		"dottime",
		"drop",
		"dropv",
		"force",
		"forcemap",
		"freeze",
		"freezetime",
		"grab",
		"grabdistance",
		"guardcost",
		"hitflash",
		"hitsound",
		"jugglecost",
		"maptime",
		"noblock",
		"noflash",
		"nopain",
		"otg",
		"pause",
		"seal",
		"sealtime",
		"staydown",
		"steal",
		"type",
	};

	static const char* proplist_defense[] = {
		"blockpower",
		"blockratio",
		"blockthreshold",
		"blocktype",
		"factor",
		"knockdown",
		"pain",
	};

    static const char* proplist_dot[] = {
		"force",
		"mode",
		"owner",
		"rate",
		"time",
		"type",
	};

    static const char* proplist_edelay[] = {
		"cap_max",
		"cap_min",
		"factor",
		"mode",
		"range_max",
		"range_min",
	};

    static const char* proplist_energycost[] = {
		"cost",
		"disable",
		"mponly",
	};

    static const char* proplist_flash[] = {
        "block",
        "default",
        "noattack",
    };

    static const char* proplist_icon[] = {
        "default",
        "die",
        "get",
        "mphigh",
        "mplow",
        "mpmed",
        "pain",
        "weapon",
        "x",
        "y",
    };

    static const char* proplist_knockdowncount[] = {
        "current",
        "max",
        "time",
    };

    static const char* proplist_landframe[] = {
        "ent",
        "frame",
	};

    static const char* proplist_maps[] = {
        "count",
        "current",
        "default",
        "dying",
        "dying_critical",
        "dying_low",
        "frozen",
        "hide_end",
        "hide_start",
        "ko",
        "kotype",
        "table",
        "time",
    };

	static const char* proplist_range[] = {
		"amax",
		"amin",
		"bmax",
		"bmin",
		"xmax",
		"xmin",
		"zmax",
		"zmin",
	};

	static const char* proplist_running[] = {
		"jumpx",
		"jumpy",
		"land",
		"movez",
		"speed",
	};

    static const char* proplist_spritea[] = {
        "centerx",
        "centery",
        "file",
        "offsetx",
        "offsety",
        "sprite",
	};

    static const char* proplist_staydown[] = {
        "rise",
        "riseattack",
        "riseattack_stall",
    };

	if(paramCount < 2) return;

	// map entity properties
	MAPSTRINGS(varlist[1], proplist, _gep_the_end,
		"Property name '%s' is not supported by function getentityproperty.\n");

	if(paramCount < 3) return;

	// map subproperties of aiflag property
	if((varlist[1]->vt == VT_INTEGER) && (varlist[1]->lVal == _gep_aiflag))
	{
		MAPSTRINGS(varlist[2], proplist_aiflag, _gep_aiflag_the_end,
			"'%s' is not a known subproperty of 'aiflag'.\n");
	}

	// map subproperties of Attack
	if((varlist[1]->vt == VT_INTEGER) && (varlist[1]->lVal == _gep_attack))
	{
		MAPSTRINGS(varlist[2], proplist_attack, _gep_attack_the_end,
			"Property name '%s' is not a known subproperty of 'attack'.\n");
	}

	// map subproperties of defense property
	if((varlist[1]->vt == VT_INTEGER) && (varlist[1]->lVal == _gep_defense))
	{
		if(paramCount >= 4)
		{
			MAPSTRINGS(varlist[3], proplist_defense, _gep_defense_the_end,
				"'%s' is not a known subproperty of 'defense'.\n");
		}
	}

    // map subproperties of DOT
	if((varlist[1]->vt == VT_INTEGER) && (varlist[1]->lVal == _gep_dot))
	{
		MAPSTRINGS(varlist[2], proplist_dot, _gep_dot_the_end,
			"Property name '%s' is not a known subproperty of 'dot'.\n");
	}

    // map subproperties of Edelay property
	if((varlist[1]->vt == VT_INTEGER) && (varlist[1]->lVal == _gep_edelay))
	{
		MAPSTRINGS(varlist[2], proplist_edelay, _gep_edelay_the_end,
			"'%s' is not a known subproperty of 'edelay'.\n");
	}

	// map subproperties of Energycost
	if((varlist[1]->vt == VT_INTEGER) && (varlist[1]->lVal == _gep_energycost))
	{
		MAPSTRINGS(varlist[2], proplist_energycost, _gep_energycost_the_end,
			"Property name '%s' is not a known subproperty of 'energycost'.\n");
	}

	// map subproperties of Flash
	if((varlist[1]->vt == VT_INTEGER) && (varlist[1]->lVal == _gep_flash))
	{
		MAPSTRINGS(varlist[2], proplist_flash, _gep_flash_the_end,
			"Property name '%s' is not a known subproperty of 'flash'.\n");
	}

    // map subproperties of Icon
	if((varlist[1]->vt == VT_INTEGER) && (varlist[1]->lVal == _gep_icon))
	{
		MAPSTRINGS(varlist[2], proplist_icon, _gep_icon_the_end,
			"Property name '%s' is not a known subproperty of 'icon'.\n");
	}

	// map subproperties of Knockdowncount
	if((varlist[1]->vt == VT_INTEGER) && (varlist[1]->lVal == _gep_knockdowncount))
	{
		MAPSTRINGS(varlist[2], proplist_knockdowncount, _gep_knockdowncount_the_end,
			"Property name '%s' is not a known subproperty of 'knockdowncount'.\n");
	}

	// map subproperties of Landframe
	if((varlist[1]->vt == VT_INTEGER) && (varlist[1]->lVal == _gep_landframe))
	{
		MAPSTRINGS(varlist[2], proplist_landframe, _gep_landframe_the_end,
			"Property name '%s' is not a known subproperty of 'landframe'.\n");
	}

	// map subproperties of Maps
	if((varlist[1]->vt == VT_INTEGER) && (varlist[1]->lVal == _gep_maps))
	{
		MAPSTRINGS(varlist[2], proplist_maps, _gep_maps_the_end,
			"Property name '%s' is not a known subproperty of 'maps'.\n");
	}

	// map subproperties of Range
	if((varlist[1]->vt == VT_INTEGER) && (varlist[1]->lVal == _gep_range))
	{
		MAPSTRINGS(varlist[2], proplist_range, _gep_range_the_end,
			"Property name '%s' is not a known subproperty of 'range'.\n");
	}

	// map subproperties of Running
	if((varlist[1]->vt == VT_INTEGER) && (varlist[1]->lVal == _gep_running))
	{
		MAPSTRINGS(varlist[2], proplist_running, _gep_running_the_end,
			"Property name '%s' is not a known subproperty of 'running'.\n");
	}

	// map subproperties of Spritea
	if((varlist[1]->vt == VT_INTEGER) && (varlist[1]->lVal == _gep_spritea))
	{
		MAPSTRINGS(varlist[2], proplist_spritea, _gep_spritea_the_end,
			"Property name '%s' is not a known subproperty of 'spritea'.\n");
	}

	// map subproperties of Staydown
	if((varlist[1]->vt == VT_INTEGER) && (varlist[1]->lVal == _gep_staydown))
	{
		MAPSTRINGS(varlist[2], proplist_staydown, _gep_running_the_end,
			"Property name '%s' is not a known subproperty of 'staydown'.\n");
	}
}

//getentityproperty(pentity, propname);
HRESULT openbor_getentityproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	entity* ent			= NULL;
	char* tempstr		= NULL;
	ScriptVariant* arg	= NULL;
	ScriptVariant* arg1	= NULL;
	s_sprite* spr;
	s_attack* attack;
	LONG ltemp;
	int i				= 0;
	int propind ;
	int tempint			= 0;
	short *coords;

	if(paramCount < 2)  {
		*pretvar = NULL;
		return E_FAIL;
	}

	ScriptVariant_Clear(*pretvar);
	mapstrings_getentityproperty(varlist, paramCount);

	arg = varlist[0];
	if(arg->vt != VT_PTR && arg->vt != VT_EMPTY)
	{
		printf("Function getentityproperty must have a valid entity handle.\n");
		*pretvar = NULL;
		return E_FAIL;
	}
	ent = (entity*)arg->ptrVal; //retrieve the entity
	if(!ent) return S_OK;

	arg = varlist[1];
	if(arg->vt!=VT_INTEGER)
	{
		printf("Function getentityproperty must have a string property name.\n");
	}

	propind = arg->lVal;

	switch(propind)
	{
    case _gep_a:
	{
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)ent->a;
		break;
	}
	case _gep_aggression:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.aggression;
		break;
	}
	case _gep_aiattack:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.aiattack;
		break;
	}
	case _gep_aiflag:
	{
		if(paramCount<3) break;
		arg = varlist[2];
		if(arg->vt != VT_INTEGER)
		{
			printf("You must give a string name for aiflag.\n");
			return E_FAIL;
		}
		ltemp = arg->lVal;
		switch(ltemp)
		{
		case _gep_aiflag_dead:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->dead;
			 break;
		}
		case _gep_aiflag_jumpid:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->jumpid;
			 break;
		}
		case _gep_aiflag_jumping:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->jumping;
			 break;
		}
		case _gep_aiflag_idling:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->idling;
			 break;
		}
		case _gep_aiflag_drop:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->drop;
			 break;
		}
		case _gep_aiflag_attacking:
		{
			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)ent->attacking;
			break;
		}
		case _gep_aiflag_getting:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->getting;
			 break;
		}
		case _gep_aiflag_turning:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->turning;
			 break;
		}
		case _gep_aiflag_charging:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->charging;
			 break;
		}
		case _gep_aiflag_blocking:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->blocking;
			 break;
		}
		case _gep_aiflag_falling:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->falling;
			 break;
		}
		case _gep_aiflag_running:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->running;
			 break;
		}
		case _gep_aiflag_inpain:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->inpain;
			 break;
		} 
		case _gep_aiflag_pain_time:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->pain_time;
			 break;
		}
		case _gep_aiflag_projectile:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->projectile;
			 break;
		}
		case _gep_aiflag_frozen:
		{
			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)ent->frozen;
			break;
		}
		case _gep_aiflag_freezetime:
		{
			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)ent->freezetime;
			break;
		}
		case _gep_aiflag_toexplode:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->toexplode;
			 break;
		}
		case _gep_aiflag_animating:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->animating;
			 break;
		}
		case _gep_aiflag_blink:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->blink;
			 break;
		}
		case _gep_aiflag_invincible:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->invincible;
			 break;
		}
		case _gep_aiflag_autokill:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->autokill;
			 break;
		}
		default:
			ScriptVariant_Clear(*pretvar);
			return E_FAIL;
		}
		break;
	}
	case _gep_aimove:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.aimove;
		break;
	}
    case _gep_alpha:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.alpha;
		break;
	}
    case _gep_animal:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.animal;
		break;
	}
	case _gep_animating:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->animating;
		break;
	}
	case _gep_animation:
	{
		ScriptVariant_ChangeType(*pretvar, VT_PTR);
		(*pretvar)->ptrVal = (VOID*)ent->animation;
		break;
	}

	/*
	case _gep_animationid: See animnum.
	*/

	case _gep_animheight:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->animation->height;
		break;
	}
	case _gep_animhits:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->animation->animhits;
		break;
	}
	case _gep_animnum:
	case _gep_animationid:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->animnum;
		break;
	}
	case _gep_animpos:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->animpos;
		break;
	}
	case _gep_animvalid:
	{
		ltemp = 0;
		if(paramCount == 3)
		{
			arg = varlist[2];
			if(FAILED(ScriptVariant_IntegerValue(arg, &ltemp)))
			ltemp = (LONG)0;
		}
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)validanim(ent, ltemp);
		break;
	}
	case _gep_antigravity:
	{
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)ent->antigravity;
		break;
	}
	case _gep_attack:
	{
		if(paramCount<6) break;
		arg = varlist[2];
		if(arg->vt != VT_INTEGER)
		{
			printf("Error, getentityproperty({ent}, 'attack', {sub property}, {index}, {animation}, {frame}): You must give a string name for {sub property}.\n");
			return E_FAIL;
		}
		ltemp = arg->lVal;

		if(varlist[3]->vt != VT_INTEGER
			|| varlist[4]->vt != VT_INTEGER
			|| varlist[5]->vt != VT_INTEGER)
		{
			printf("\n Error, getentityproperty({ent}, 'attack', {sub property}, {index}, {animation}, {frame}): {Animation} or {frame} parameter is missing or invalid. \n");
			return E_FAIL;
		}

		//varlist[3]->lval														//Attack box index (multiple attack boxes).
		i		= varlist[4]->lVal;												//Animation parameter.
		tempint	= varlist[5]->lVal;												//Frame parameter.

		if(!validanim(ent,i) || !ent->modeldata.animation[i]->attacks || !ent->modeldata.animation[i]->attacks[tempint])	//Verify animation and active attack on frame.
		{
			break;
		}

		attack = ent->modeldata.animation[i]->attacks[tempint];					//Get attack struct.

		switch(ltemp)
		{
			case _gep_attack_blockflash:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->blockflash;
				 break;
			case _gep_attack_blocksound:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->blocksound;
				 break;
			case _gep_attack_coords:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->attack_coords[varlist[6]->lVal];
				 break;
			case _gep_attack_counterattack:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->counterattack;
				 break;
			case _gep_attack_direction:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->force_direction;
				 break;
			case _gep_attack_dol:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->damage_on_landing;
				 break;
			case _gep_attack_dot:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->dot;
				 break;
			case _gep_attack_dotforce:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->dot_force;
				 break;
			case _gep_attack_dotindex:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->dot_index;
				 break;
			case _gep_attack_dotrate:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->dot_rate;
				 break;
			case _gep_attack_dottime:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->dot_time;
				 break;
			case _gep_attack_drop:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->attack_drop;
				 break;
			case _gep_attack_dropv:
				 ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
				 (*pretvar)->dblVal = (DOUBLE)attack->dropv[varlist[6]->lVal];
				 break;
			case _gep_attack_force:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->attack_force;
				 break;
			case _gep_attack_forcemap:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->forcemap;
				 break;
			case _gep_attack_freeze:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->freeze;
				 break;
			case _gep_attack_freezetime:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->freezetime;
				 break;
			case _gep_attack_grab:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->grab;
				 break;
			case _gep_attack_grabdistance:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->grab_distance;
				 break;
			case _gep_attack_guardcost:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->guardcost;
				 break;
			case _gep_attack_hitflash:
				ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				(*pretvar)->lVal = (LONG)attack->hitflash;
				break;
			case _gep_attack_hitsound:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->hitsound;
				 break;
			case _gep_attack_jugglecost:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->jugglecost;
				 break;
			case _gep_attack_maptime:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->maptime;
				 break;
			case _gep_attack_noblock:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->no_block;
				 break;
			case _gep_attack_noflash:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->no_flash;
				 break;
			case _gep_attack_nopain:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->no_pain;
				 break;
			case _gep_attack_otg:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->otg;
				 break;
			case _gep_attack_pause:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->pause_add;
				 break;
			case _gep_attack_seal:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->seal;
				 break;
			case _gep_attack_sealtime:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->sealtime;
				 break;
			case _gep_attack_staydown:
				 ScriptVariant_ChangeType(*pretvar, VT_PTR);
				 (*pretvar)->ptrVal = (VOID*)attack->staydown;
				 break;
			case _gep_attack_steal:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->steal;
				 break;
			case _gep_attack_type:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)attack->attack_type;
				 break;
			default:
				ScriptVariant_Clear(*pretvar);
				return E_FAIL;
		}
		break;
	}
	case _gep_attacking:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->attacking;
		break;
	}
	case _gep_attackid:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->attack_id;
		break;
	}
	case _gep_autokill:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->autokill;
		break;
	}
	case _gep_base:
	{
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)ent->base;
		break;
	}
	case _gep_bbox:
	{
		if(paramCount<6) break;

		if(varlist[2]->vt != VT_INTEGER
			|| varlist[3]->vt != VT_INTEGER
			|| varlist[4]->vt != VT_INTEGER
			|| varlist[5]->vt != VT_INTEGER)
		{
			printf("\n Error, getentityproperty({ent}, 'bbox', {index}, {animation}, {frame}, {arg}): {index}, {Animation}, {frame}, or {arg} parameter is missing or invalid. \n");
			return E_FAIL;
		}

		//varlist[3]->lval;														//bbox index (multiple bbox support).
		i		= varlist[3]->lVal;												//Animation parameter.
		tempint	= varlist[4]->lVal;												//Frame parameter.

		if(!ent->modeldata.animation[i]->bbox_coords)
		{
			break;
		}

		coords = ent->modeldata.animation[i]->bbox_coords[tempint];

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)coords[varlist[5]->lVal];
		break;
	}
	case _gep_blink:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->blink;
		break;
	}
	case _gep_blockback:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->modeldata.blockback = (int)ltemp;
		break;
	}
	case _gep_blockodds:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.blockodds;
		break;
	}
	case _gep_blockpain:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->modeldata.blockpain = (int)ltemp;
		break;
	}
	case _gep_bounce:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.bounce;
		break;
	}
	case _gep_chargerate:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.chargerate;
		break;
	}
	case _gep_colourmap:
	{
		ScriptVariant_ChangeType(*pretvar, VT_PTR);
		(*pretvar)->ptrVal = (VOID*)(ent->colourmap);
		break;
	}
	case _gep_colourtable:
	{
		ScriptVariant_ChangeType(*pretvar, VT_PTR);
		(*pretvar)->ptrVal = (VOID*)(ent->modeldata.colourmap[varlist[2]->lVal]);
		break;
	}
	case _gep_damage_on_landing:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->damage_on_landing;
		break;
	}
	case _gep_dead:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->dead;
		break;
	}
	case _gep_defaultmodel:
	case _gep_defaultname:
	{
		ScriptVariant_ChangeType(*pretvar, VT_STR);
		strcpy(StrCache_Get((*pretvar)->strVal), ent->defaultmodel->name);
		break;
	}
	case _gep_defense:
	{
		ltemp = 0;
		if(paramCount == 3)
		{
			arg = varlist[2];
			if(FAILED(ScriptVariant_IntegerValue(arg, &ltemp)))
				ltemp = (LONG)0;
		}

		arg = varlist[3];
		if(arg->vt != VT_INTEGER)
		{
			if(arg->vt != VT_STR)
				printf("You must give a string name for defense property.\n");
			return E_FAIL;
		}
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);

		switch(arg->lVal)
		{
		case _gep_defense_factor:
		{
			(*pretvar)->dblVal = (float)ent->modeldata.defense_factors[(int)ltemp];
			break;
		}
		case _gep_defense_blockpower:
		{
			(*pretvar)->dblVal = (DOUBLE)ent->modeldata.defense_blockpower[(int)ltemp];
			break;
		}
		case _gep_defense_blockratio:
		{
			(*pretvar)->dblVal = (DOUBLE)ent->modeldata.defense_blockratio[(int)ltemp];
			break;
		}
		case _gep_defense_blockthreshold:
		{
			(*pretvar)->dblVal = (DOUBLE)ent->modeldata.defense_blockthreshold[(int)ltemp];
			break;
		}
		case _gep_defense_blocktype:
		{
			(*pretvar)->dblVal = (DOUBLE)ent->modeldata.defense_blocktype[(int)ltemp];
			break;
		}
		case _gep_defense_knockdown:
		{
			(*pretvar)->dblVal = (DOUBLE)ent->modeldata.defense_knockdown[(int)ltemp];
			break;
		}
		case _gep_defense_pain:
		{
			(*pretvar)->dblVal = (DOUBLE)ent->modeldata.defense_pain[(int)ltemp];
			break;
		}
		default:
			return E_FAIL;
		}
		break;
	}
	case _gep_detect:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.stealth.detect;
		break;
	}
	case _gep_direction:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->direction;
		break;
	}
	case _gep_dot:
	{
		if(paramCount<4) break;

		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			i = (int)ltemp;

		arg = varlist[3];
		if(arg->vt != VT_INTEGER)
		{
			if(arg->vt != VT_STR)
				printf("You must provide a string name for dot subproperty.\n\
                        ~'time'\n\
                        ~'mode'\n\
                        ~'force'\n\
                        ~'rate'\n\
                        ~'type'\n\
                        ~'owner'\n");
			return E_FAIL;
		}
		switch(arg->lVal)
		{
		case _gep_dot_time:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->dot_time[i];
			 break;
		}
		case _gep_dot_mode:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->dot[i];
			 break;
		}
		case _gep_dot_force:

		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->dot_force[i];
			 break;
		}
		case _gep_dot_rate:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->dot_rate[i];
			 break;
		}
		case _gep_dot_type:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->dot_atk[i];
			 break;
		}
		case _gep_dot_owner:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_PTR);
			 (*pretvar)->ptrVal = (VOID*)ent->dot_owner[i];
			 break;
		}
		break;
		}
	}
	case _gep_dropframe:
	{
		ltemp = 0;
		if(paramCount == 3)
		{
			arg = varlist[2];
			if(FAILED(ScriptVariant_IntegerValue(arg, &ltemp)))
			ltemp = (LONG)0;
		}
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.animation[ltemp]->dropframe;
		break;
	}
	case _gep_edelay:
	{
		arg = varlist[2];
		if(arg->vt != VT_INTEGER)
		{
			if(arg->vt != VT_STR)
				printf("You must provide a string name for edelay subproperty.\n\
                        ~'cap_max'\n\
                        ~'cap_min'\n\
                        ~'factor'\n\
                        ~'mode'\n\
                        ~'range_max'\n\
                        ~'range_min'\n");
			return E_FAIL;
		}
		ltemp = arg->lVal;
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

		switch(ltemp)
		{
		case _gep_edelay_mode:
		{
			(*pretvar)->lVal = (LONG)ent->modeldata.edelay.mode;
			break;
		}
		case _gep_edelay_factor:
		{
			(*pretvar)->dblVal = (float)ent->modeldata.edelay.factor;
			break;
		}
		case _gep_edelay_cap_min:
		{
			(*pretvar)->lVal = (LONG)ent->modeldata.edelay.cap_min;
			break;
		}
		case _gep_edelay_cap_max:
		{
			(*pretvar)->lVal = (LONG)ent->modeldata.edelay.cap_max;
			break;
		}
		case _gep_edelay_range_min:
		{
			(*pretvar)->lVal = (LONG)ent->modeldata.edelay.range_min;
			break;
		}
		case _gep_edelay_range_max:
		{
			(*pretvar)->lVal = (LONG)ent->modeldata.edelay.range_max;
			break;
		}
		default:
			ScriptVariant_Clear(*pretvar);
			return E_FAIL;
		}
		break;
	}
	case _gep_energycost:
	{
		if(paramCount<4) break;

		if(varlist[2]->vt != VT_INTEGER)
		{
			if(varlist[2]->vt != VT_STR)
				printf("You must provide a string name for edelay energycost.\n\
                        ~'cost'\n\
                        ~'disable'\n\
                        ~'mponly'\n");
			return E_FAIL;
		}
		ltemp	= varlist[2]->lVal;												//Subproperty.
		i		= varlist[3]->lVal;												//Animation.

		if(!validanim(ent,i))													//Verify animation.
		{
			break;
		}

		switch(ltemp)
		{
			case _gep_energycost_cost:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)ent->modeldata.animation[i]->energycost.cost;
				 break;
			case _gep_energycost_disable:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)ent->modeldata.animation[i]->energycost.disable;
				 break;
			case _gep_energycost_mponly:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)ent->modeldata.animation[i]->energycost.mponly;
				 break;
			default:
				ScriptVariant_Clear(*pretvar);
				return E_FAIL;
		}
		break;
	}
	case _gep_escapecount:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->escapecount;
		break;
	}
	case _gep_escapehits:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.escapehits;
		break;
	}
	case _gep_exists:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->exists;
		break;
	}
	case _gep_falldie:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.falldie;
		break;
	}
	case _gep_flash:
	{
		arg = varlist[2];
		if(arg->vt != VT_INTEGER)
		{
			if(arg->vt != VT_STR)
				printf("You must give a string name for flash property.\n");
			return E_FAIL;
		}
		ltemp = arg->lVal;
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

		switch(ltemp)
		{
            case _gep_flash_block:
            {
                i = ent->modeldata.bflash;
                break;
            }
            case _gep_flash_def:
            {
                i = ent->modeldata.flash;
                break;
            }
            case _gep_flash_noattack:
            {
                i = ent->modeldata.noatflash;
                break;
            }
            default:
			{
                ScriptVariant_Clear(*pretvar);
                return E_FAIL;
            }
		}
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)i;
		break;
	}
	case _gep_pain_time:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->pain_time;
		break;
	}
	case _gep_freezetime:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->freezetime;
		break;
	}
	case _gep_frozen:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->frozen;
		break;
	}
	case _gep_gfxshadow:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.gfxshadow;
		break;
	}
	case _gep_grabbing:
	{
		if(ent->grabbing) // always return an empty var if it is NULL
		{
			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (VOID*)ent->grabbing;
		}
		break;
	}
	case _gep_grabforce:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.grabforce;
		break;
	}
	case _gep_guardpoints:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.guardpoints.current;
		break;
	}
	case _gep_health:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->health;
		break;
	}
	case _gep_height:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.height;
		break;
	}
	case _gep_hitbyid:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->hit_by_attack_id;
		break;
	}
	case _gep_hmapl:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.maps.hide_start;
		 break;
	}
	case _gep_hmapu:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.maps.hide_end;
		 break;
	}
	case _gep_icon:
	{
	    arg = varlist[2];
		if(arg->vt != VT_INTEGER)
		{
			if(arg->vt != VT_STR)
				printf("You must provide a string name for icon subproperty:\n\
                        getentityproperty({ent}, 'icon', {subproperty});\n\
                        ~'default'\n\
                        ~'die'\n\
                        ~'get'\n\
                        ~'mphigh'\n\
                        ~'mplow'\n\
                        ~'mpmed'\n\
                        ~'pain'\n\
                        ~'weapon'\n\
                        ~'x'\n\
                        ~'y'\n");
			return E_FAIL;
		}
		ltemp = arg->lVal;
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

		switch(ltemp)
		{
            case _gep_icon_def:
            {
                i = ent->modeldata.icon.def;
                break;
            }
            case _gep_icon_die:
            {
                i = ent->modeldata.icon.die;
                break;
            }
            case _gep_icon_get:
            {
                i = ent->modeldata.icon.get;
                break;
            }
            case _gep_icon_mphigh:
            {
                i = ent->modeldata.icon.mphigh;
                break;
            }
            case _gep_icon_mplow:
            {
                i = ent->modeldata.icon.mplow;
                break;
            }
            case _gep_icon_mpmed:
            {
                i = ent->modeldata.icon.mpmed;
                break;
            }
            case _gep_icon_pain:
            {
                i = ent->modeldata.icon.pain;
                break;
            }
            case _gep_icon_weapon:
            {
                i = ent->modeldata.icon.weapon;
                break;
            }
            case _gep_icon_x:
            {
                i = ent->modeldata.icon.x;
                break;
            }
            case _gep_icon_y:
            {
                i = ent->modeldata.icon.y;
                break;
            }
            default:
            {
                ScriptVariant_Clear(*pretvar);
                return E_FAIL;
            }
		}

		if (i >= 0)
		{
			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			spr = sprite_map[i].sprite;
			spr->centerx = sprite_map[i].centerx;
			spr->centery = sprite_map[i].centery;
			(*pretvar)->ptrVal = (VOID*)(spr);
		}
		else
		{
			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = -1;
		}
		break;
	}
	case _gep_invincible:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->invincible;
		break;
	}
	case _gep_invinctime:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->invinctime;
		break;
	}
	case _gep_jugglepoints:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.jugglepoints.current;
		break;
	}
	case _gep_knockdowncount:
	{
	    /*
	    2011_04_14, DC: Backward compatability; default to current if subproperty not provided.
	    */
	    if(paramCount<3)
	    {
            ltemp = _gep_knockdowncount_current;
	    }
	    else
	    {
            arg = varlist[2];

            if(arg->vt != VT_INTEGER)
            {
                if(arg->vt != VT_STR)
                {
                    printf("You must provide a string name for knockdowncount subproperty:\n\
                            getentityproperty({ent}, 'knockdowncount', {subproperty})\n\
                            ~'current'\n\
                            ~'max'\n\
                            ~'time'\n");
                    return E_FAIL;
                }
            }

            ltemp = arg->lVal;
	    }

		switch(ltemp)
		{
			case _gep_knockdowncount_current:
                ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
                (*pretvar)->dblVal = (DOUBLE)ent->knockdowncount;
			case _gep_knockdowncount_max:
                ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
                (*pretvar)->dblVal = (DOUBLE)ent->modeldata.knockdowncount;
				 break;
            case _gep_knockdowncount_time:
                ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
                (*pretvar)->lVal = (LONG)ent->knockdowncount;
			default:
				ScriptVariant_Clear(*pretvar);
				return E_FAIL;
		}
		break;
	}
	case _gep_komap:
	{
		if(paramCount<2) break;

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.maps.ko;
		break;
	}
	case _gep_landframe:
	{
	    if(paramCount<4) break;

		if(varlist[2]->vt != VT_INTEGER
			|| varlist[3]->vt != VT_INTEGER)
		{
			printf("\n Error, getentityproperty({ent}, 'landframe', {sub property}, {animation}): {Sub property} or {Animation} parameter is missing or invalid. \n");
			return E_FAIL;
		}
		ltemp	= varlist[2]->lVal;												//Subproperty.
		i		= varlist[3]->lVal;												//Animation.

		if(!validanim(ent,i))													//Verify animation.
		{
			break;
		}

		switch(ltemp)
		{
			case _gep_landframe_ent:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)ent->modeldata.animation[i]->landframe.ent;
				 break;
			case _gep_landframe_frame:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)ent->modeldata.animation[i]->landframe.frame;
				 break;
			default:
				ScriptVariant_Clear(*pretvar);
				return E_FAIL;
		}
		break;
	}
	case _gep_lifespancountdown:
	{
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)ent->lifespancountdown;
		break;
	}
	case _gep_link:
	{
		if(ent->link) // always return an empty var if it is NULL
		{
			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (VOID*)ent->link;
		}
		break;
	}
	case _gep_map:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)0;
		for(i=0;i<ent->modeldata.maps_loaded;i++)
		{
			if(ent->colourmap == ent->modeldata.colourmap[i])
			{
				(*pretvar)->lVal = (LONG)(i+1);
				break;
			}
		}
		break;
	}
	case _gep_mapcount:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)(ent->modeldata.maps_loaded+1);
		 break;
	}
	case _gep_mapdefault:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)(ent->map);
		 break;
	}
	case _gep_maps:
	{
	    arg = varlist[2];
		if(arg->vt != VT_INTEGER)
		{
			if(arg->vt != VT_STR)
				printf("You must give a string name for maps property.\n");
			return E_FAIL;
		}
		ltemp = arg->lVal;
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

		switch(ltemp)
		{
		    case _gep_maps_count:
            {
                ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
                (*pretvar)->lVal = (LONG)(ent->modeldata.maps_loaded+1);
                break;
            }

            case _gep_maps_current:
            {
                ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
                (*pretvar)->lVal = (LONG)0;
                for(i=0;i<ent->modeldata.maps_loaded;i++)
                {
                    if(ent->colourmap == ent->modeldata.colourmap[i])
                    {
                        (*pretvar)->lVal = (LONG)(i+1);
                        break;
                    }
                }
                break;
            }
            case _gep_maps_dying:
            {
                ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
                (*pretvar)->lVal = (LONG)(ent->dying);
                break;
            }
            case _gep_maps_dying_critical:
            {
                ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
                (*pretvar)->lVal = (LONG)(ent->per2);
                break;
            }
            case _gep_maps_dying_low:
            {
                ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
                (*pretvar)->lVal = (LONG)(ent->per1);
                break;
            }
            case _gep_maps_frozen:
            {
                ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
                (*pretvar)->lVal = (LONG)(ent->modeldata.maps.frozen);
                break;
            }
            case _gep_maps_hide_end:
            {
                ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
                (*pretvar)->lVal = (LONG)(ent->modeldata.maps.hide_end);
                break;
            }
            case _gep_maps_hide_start:
            {
                ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
                (*pretvar)->lVal = (LONG)(ent->modeldata.maps.hide_start);
                break;
            }
            case _gep_maps_ko:
            {
                ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
                (*pretvar)->lVal = (LONG)(ent->modeldata.maps.ko);
                break;
            }
            case _gep_maps_kotype:
            {
                ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
                (*pretvar)->lVal = (LONG)(ent->modeldata.maps.kotype);
                break;
            }
            case _gep_maps_table:
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID*)(ent->colourmap);
                break;
            }
            case _gep_maps_time:
            {
                ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
                (*pretvar)->lVal = (LONG)(ent->maptime);
                break;
            }
            default:
            {
                ScriptVariant_Clear(*pretvar);
                return E_FAIL;
            }
		}
		break;
	}
	case _gep_maxguardpoints:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.guardpoints.maximum;
		break;
	}
	case _gep_maxhealth:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.health;
		break;
	}
	case _gep_maxjugglepoints:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.jugglepoints.maximum;
		break;
	}
	case _gep_maxmp:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.mp;
		break;
	}
	case _gep_model:
	{
		ScriptVariant_ChangeType(*pretvar, VT_STR);
		strcpy(StrCache_Get((*pretvar)->strVal), ent->model->name);
		break;
	}
	case _gep_mp:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->mp;
		break;
	}
	case _gep_mpdroprate:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.mpdroprate;
		break;
	}
	case _gep_mprate:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.mprate;
		break;
	}
	case _gep_mpstable:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.mpstable;
		break;
	}
	case _gep_mpstableval:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.mpstableval;
		break;
	}
	case _gep_name:
	{
		ScriptVariant_ChangeType(*pretvar, VT_STR);
		strcpy(StrCache_Get((*pretvar)->strVal), ent->name);
		break;
	}
	case _gep_nextanim:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->nextanim;
		break;
	}
	case _gep_nextthink:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->nextthink;
		break;
	}
	case _gep_no_adjust_base:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.no_adjust_base;
		break;
	}
	case _gep_noaicontrol:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->noaicontrol;
		break;
	}
	case _gep_nodieblink:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.nodieblink;
		break;
	}
	case _gep_nodrop:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.nodrop;
		break;
	}
	case _gep_nograb:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->nograb;
		break;
	}
	case _gep_nopain:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.nopain;
		break;
	}
	case _gep_offense:
	{
		ltemp = 0;
		if(paramCount >= 3)
		{
			arg = varlist[2];
			if(FAILED(ScriptVariant_IntegerValue(arg, &ltemp)))
				ltemp = (LONG)0;
		}
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)ent->modeldata.offense_factors[(int)ltemp];
		break;
	}
	case _gep_opponent:
	{
		if(ent->opponent) // always return an empty var if it is NULL
		{
			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (VOID*)ent->opponent;
		}
		break;
	}
	case _gep_owner:
	{
		if(ent->owner) // always return an empty var if it is NULL
		{
			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (VOID*)ent->owner;
		}
		break;
	}
	case _gep_parent:
	{
		if(ent->parent) // always return an empty var if it is NULL
		{
			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (VOID*)ent->parent;
		}
		break;
	}
	case _gep_path:
	{
		ScriptVariant_ChangeType(*pretvar, VT_STR);
		tempstr = ent->modeldata.path;

		strcpy(StrCache_Get((*pretvar)->strVal), tempstr);
		break;
	}
	case _gep_playerindex:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->playerindex;
		//printf("%d %s %d\n", ent->sortid, ent->name, ent->playerindex);
		break;
	}
    case _gep_projectile:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->projectile;
		break;
	}
	case _gep_range:
	{
		if(paramCount<4) break;

		if(varlist[2]->vt != VT_INTEGER
			|| varlist[3]->vt != VT_INTEGER)
		{
			printf("\n Error, getentityproperty({ent}, 'range', {sub property}, {animation}): {Sub property} or {Animation} parameter is missing or invalid. \n");
			return E_FAIL;
		}
		ltemp	= varlist[2]->lVal;												//Subproperty.
		i		= varlist[3]->lVal;												//Animation.

		if(!validanim(ent,i))													//Verify animation.
		{
			break;
		}

		switch(ltemp)
		{
			case _gep_range_amax:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)ent->modeldata.animation[i]->range.amax;
				 break;
			case _gep_range_amin:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)ent->modeldata.animation[i]->range.amin;
				 break;
			case _gep_range_bmax:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)ent->modeldata.animation[i]->range.bmax;
				 break;
			case _gep_range_bmin:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)ent->modeldata.animation[i]->range.bmin;
				 break;
			case _gep_range_xmax:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)ent->modeldata.animation[i]->range.xmax;
				 break;
			case _gep_range_xmin:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)ent->modeldata.animation[i]->range.xmin;
				 break;
			case _gep_range_zmax:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)ent->modeldata.animation[i]->range.zmax;
				 break;
			case _gep_range_zmin:
				 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
				 (*pretvar)->lVal = (LONG)ent->modeldata.animation[i]->range.zmin;
				 break;
			default:
				ScriptVariant_Clear(*pretvar);
				return E_FAIL;
		}
		break;

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)coords[varlist[4]->lVal];
		break;
	}
	case _gep_running:
	{
		if(paramCount<3) break;
		arg = varlist[2];
		if(arg->vt != VT_INTEGER)
		{
			if(arg->vt != VT_STR)
				printf("You must give a string name for running property.\n");
			return E_FAIL;
		}
		ltemp = arg->lVal;
		switch(ltemp)
		{
		case _gep_running_speed:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)ent->modeldata.runspeed;
			break;
		}
		case _gep_running_jumpy:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)ent->modeldata.runjumpheight;
			break;
		}
		case _gep_running_jumpx:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
			(*pretvar)->dblVal = (DOUBLE)ent->modeldata.runjumpdist;
			break;
		}
		case _gep_running_land:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			(*pretvar)->lVal = (LONG)ent->modeldata.runhold;
			break;
		}
		case _gep_running_movez:
		{
			 ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			 (*pretvar)->lVal = (LONG)ent->modeldata.runupdown;
			 break;
		}
		}
		break;
	}
	case _gep_rush_count:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->rush[0];
		break;
	}
	case _gep_rush_tally:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->rush[1];
		break;
	}
	case _gep_rush_time:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->rushtime;
		break;
	}
	case _gep_score:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.score;
		break;
	}
	case _gep_scroll:
	{
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)ent->modeldata.scroll;
		break;
	}
	case _gep_seal:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->seal;
		break;
	}
	case _gep_sealtime:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->sealtime;
		break;
	}
	case _gep_setlayer:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.setlayer;
		break;
	}
	case _gep_spawntype:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->spawntype;
		break;
	}
	case _gep_speed:
	{
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)ent->modeldata.speed;
		break;
	}
	case _gep_sprite:
	{
		ScriptVariant_ChangeType(*pretvar, VT_PTR);
		i = ent->animation->sprite[ent->animpos];
		spr = sprite_map[i].sprite;
		spr->centerx = sprite_map[i].centerx;
		spr->centery = sprite_map[i].centery;
		(*pretvar)->ptrVal = (VOID*)(spr);
		break;
	}
	case _gep_spritea:
	{
		/*
	    2011_04_17, DC: Modder can now specify animation and frame to return sprite from.
	    To retain backward compatibility, sprite from current animation/frame is returned
	    when animation and/or frame parameters are not provided.
	    */

        ltemp   = varlist[2]->lVal;
        arg     = varlist[3];
        arg1    = varlist[4];

        /*
        Request from animation or frame that doesn't exist = shutdown.
        Let's be more user friendly then that; return empty so modder can evaluate
        and take action accordingly.*/
        if(!validanim(ent, arg->lVal) || !(ent->modeldata.animation[arg->lVal]->numframes >= arg1->lVal))
        {
            break;
        }

        i = ent->modeldata.animation[arg->lVal]->sprite[arg1->lVal];

		switch(ltemp)
		{
           case _gep_spritea_centerx:
            {
                ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
                (*pretvar)->lVal = (LONG)sprite_map[i].centerx;
                break;
            }
            case _gep_spritea_centery:
            {
                ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
                (*pretvar)->lVal = (LONG)sprite_map[i].centery;
                break;
            }
            case _gep_spritea_file:
            {
                ScriptVariant_ChangeType(*pretvar, VT_STR);
                strcpy(StrCache_Get((*pretvar)->strVal), sprite_map[i].filename);
                break;
            }
            case _gep_spritea_offsetx:
            {
                ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
                (*pretvar)->lVal = (LONG)sprite_map[i].ofsx;
                break;
            }
            case _gep_spritea_offsety:
            {
                ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
                (*pretvar)->lVal = (LONG)sprite_map[i].ofsy;
                break;
            }
            case _gep_spritea_sprite:
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                spr = sprite_map[i].sprite;
                spr->centerx = sprite_map[i].centery;
                spr->centery = sprite_map[i].centery;
                (*pretvar)->ptrVal = (VOID*)(spr);
                break;
            }
            default:
			{
                ScriptVariant_Clear(*pretvar);
                return E_FAIL;
            }
		}

		break;

	}
	case _gep_stalltime:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->stalltime;
		break;
	}
	case _gep_stats:
	{
		if(paramCount<4) break;
		arg = varlist[2];
		arg1 = varlist[3];

		if(arg->vt != VT_INTEGER || arg1->vt != VT_INTEGER)
		{
			printf("Incorrect parameters: getentityproperty({ent}, 'stats', {type}, {index}) \n {type}: \n 0 = Model. \n 1 = Entity. \n");
			return E_FAIL;
		}

		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);

		switch(arg->lVal)
		{
			default:
				if (ent->modeldata.stats[arg1->lVal])
				{
					(*pretvar)->dblVal = (DOUBLE)ent->modeldata.stats[arg1->lVal];
				}
				break;
			case 1:
				if (ent->stats[arg1->lVal])
				{
					(*pretvar)->dblVal = (DOUBLE)ent->stats[arg1->lVal];
				}
				break;
		}
		break;
	}
	case _gep_staydown:
	{
		arg = varlist[2];
		if(arg->vt != VT_INTEGER)
		{
			if(arg->vt != VT_STR)
				printf("You must provide a string name for staydown property:\n\
                        getentityproperty({ent}, 'staydown', {subproperty})\n\
                        ~'rise'\n\
                        ~'riseattack'\n\
                        ~'riseattack_stall' \n");
			return E_FAIL;
		}
		ltemp = arg->lVal;
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

		switch(ltemp)
		{
            case _gep_staydown_rise:
            {
                i = ent->staydown.rise;
                break;
            }
            case _gep_staydown_riseattack:
            {
                i = ent->staydown.riseattack;
                break;
            }
            case _gep_staydown_riseattack_stall:
            {
                i = ent->staydown.riseattack_stall;
                break;
            }
            default:
			{
                ScriptVariant_Clear(*pretvar);
                return E_FAIL;
            }
		}
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)i;
		break;
	}
	case _gep_stealth:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.stealth.hide;
		break;
	}
	case _gep_subentity:
	{
		if(ent->subentity) // always return an empty var if it is NULL
		{
			ScriptVariant_ChangeType(*pretvar, VT_PTR);
			(*pretvar)->ptrVal = (VOID*)ent->subentity;
		}
		break;
	}
	case _gep_subject_to_gravity:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.subject_to_gravity;
		break;
	}
	case _gep_subject_to_hole:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.subject_to_hole;
		break;
	}
	case _gep_subject_to_maxz:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.subject_to_maxz;
		break;
	}
	case _gep_subject_to_minz:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.subject_to_minz;
		break;
	}
	case _gep_subject_to_obstacle:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.subject_to_obstacle;
		break;
	}
	case _gep_subject_to_platform:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.subject_to_platform;
		break;
	}
	case _gep_subject_to_screen:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.subject_to_screen;
		break;
	}
	case _gep_subject_to_wall:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.subject_to_wall;
		break;
	}
	case _gep_subtype:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.subtype;
		break;
	}
	case _gep_thold:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.thold;
		break;
	}
	case _gep_throwdamage:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.throwdamage;
		break;
	}
	case _gep_throwdist:
	{
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)ent->modeldata.throwdist;
		break;
	}
	case _gep_throwframewait:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.throwframewait;
		break;
	}
	case _gep_throwheight:
	{
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)ent->modeldata.throwheight;
		break;
	}
	case _gep_tosstime:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->toss_time;
		break;
	}
	case _gep_tossv:
	{
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)ent->tossv;
		break;
	}
	case _gep_type:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.type;
		break;
	}
	case _gep_weapent:
	{
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = (VOID*)ent->weapent;
        break;
	}
	case _gep_x:
	{
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)ent->x;
		break;
	}
	case _gep_xdir:
	{
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)ent->xdir;
		break;
	}
	case _gep_z:
	{
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)ent->z;
		break;
	}
	case _gep_zdir:
	{
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)ent->zdir;
		break;
	}
	default:
		//printf("Property name '%s' is not supported by function getentityproperty.\n", propname);
		*pretvar = NULL;
		return E_FAIL;
		break;
	}

	return S_OK;
}

// ===== changeentityproperty =====
enum changeentityproperty_enum {
	_cep_aggression,
    _cep_aiattack,
    _cep_aiflag,
    _cep_aimove,
    _cep_alpha,
    _cep_animation,
    _cep_animhits,
    _cep_animpos,
    _cep_antigrab,
    _cep_antigravity,
    _cep_attacking,
    _cep_attackid,
    _cep_autokill,
    _cep_base,
    _cep_blink,
    _cep_blockback,
    _cep_blockodds,
    _cep_blockpain,
    _cep_bounce,
    _cep_candamage,
    _cep_combostep,
    _cep_colourmap,
    _cep_damage_on_landing,
    _cep_dead,
    _cep_defaultname,
    _cep_defense,
    _cep_detect,
    _cep_direction,
    _cep_dot,
    _cep_edelay,
    _cep_energycost,
    _cep_escapecount,
    _cep_escapehits,
    _cep_falldie,
    _cep_freezetime,
    _cep_frozen,
    _cep_gfxshadow,
    _cep_grabforce,
    _cep_guardpoints,
    _cep_health,
    _cep_hitbyid,
    _cep_hmapl,
    _cep_hmapu,
    _cep_hostile,
    _cep_iconposition,
    _cep_invincible,
    _cep_invinctime,
    _cep_jugglepoints,
    _cep_knockdowncount,
    _cep_komap,
    _cep_lifeposition,
    _cep_lifespancountdown,
    _cep_map,
    _cep_maptime,
    _cep_maxguardpoints,
    _cep_maxhealth,
    _cep_maxjugglepoints,
    _cep_maxmp,
    _cep_model,
    _cep_mp,
    _cep_mpset,
    _cep_name,
    _cep_nameposition,
    _cep_nextanim,
    _cep_nextthink,
    _cep_no_adjust_base,
    _cep_noaicontrol,
    _cep_nodieblink,
    _cep_nodrop,
    _cep_nograb,
    _cep_nopain,
    _cep_offense,
    _cep_opponent,
    _cep_owner,
    _cep_pain_time,
    _cep_parent,
    _cep_position,
    _cep_projectile,
    _cep_projectilehit,
    _cep_running,
    _cep_rush_count,
    _cep_rush_tally,
    _cep_rush_time,
    _cep_score,
    _cep_scroll,
    _cep_seal,
    _cep_sealtime,
    _cep_setlayer,
    _cep_speed,
    _cep_spritea,
    _cep_stalltime,
    _cep_stats,
    _cep_staydown,
    _cep_staydownatk,
    _cep_stealth,
    _cep_subentity,
    _cep_subject_to_gravity,
    _cep_subject_to_hole,
    _cep_subject_to_maxz,
    _cep_subject_to_minz,
    _cep_subject_to_obstacle,
    _cep_subject_to_platform,
    _cep_subject_to_screen,
    _cep_subject_to_wall,
    _cep_takeaction,
    _cep_think,
    _cep_thold,
    _cep_throwdamage,
    _cep_throwdist,
    _cep_throwframewait,
    _cep_throwheight,
    _cep_tosstime,
    _cep_trymove,
    _cep_type,
    _cep_velocity,
    _cep_weapon,
	_cep_the_end,
};

enum cep_aiflag_enum {
	_cep_aiflag_animating,
	_cep_aiflag_attacking,
	_cep_aiflag_autokill,
	_cep_aiflag_blink,
	_cep_aiflag_blocking,
	_cep_aiflag_charging,
	_cep_aiflag_dead,
	_cep_aiflag_drop,
	_cep_aiflag_falling,
	_cep_aiflag_frozen,
	_cep_aiflag_getting,
	_cep_aiflag_idling,
	_cep_aiflag_inpain,
	_cep_aiflag_invincible,
	_cep_aiflag_jumpid,
	_cep_aiflag_jumping,
	_cep_aiflag_projectile,
	_cep_aiflag_running,
	_cep_aiflag_toexplode,
	_cep_aiflag_turning,
	_cep_aiflag_the_end,
};

enum cep_energycost_enum {
    _cep_energycost_cost,
    _cep_energycost_disable,
    _cep_energycost_mponly,
    _cep_energycost_the_end,
};

enum cep_hostile_candamage_enum {
	_cep_hcd_ground,
	_cep_hcd_type_enemy,
	_cep_hcd_type_npc,
	_cep_hcd_type_obstacle,
	_cep_hcd_type_player,
	_cep_hcd_type_shot,
	_cep_hcd_the_end,
};

enum cep_knockdowncount_enum {
    _cep_knockdowncount_current,
    _cep_knockdowncount_max,
    _cep_knockdowncount_time,
    _cep_knockdowncount_the_end,
};

enum cep_spritea_enum {
    _cep_spritea_centerx,
    _cep_spritea_centery,
    _cep_spritea_file,
    _cep_spritea_offsetx,
    _cep_spritea_offsety,
    _cep_spritea_sprite,
    _cep_spritea_the_end,
};

enum cep_staydown_enum {
    _cep_staydown_rise,
    _cep_staydown_riseattack,
    _cep_staydown_riseattack_stall,
    _cep_staydown_the_end,
};

enum cep_takeaction_enum {
	_cep_ta_bomb_explode,
	_cep_ta_common_attack_proc,
	_cep_ta_common_block,
	_cep_ta_common_drop,
	_cep_ta_common_fall,
	_cep_ta_common_get,
	_cep_ta_common_grab,
	_cep_ta_common_grabattack,
	_cep_ta_common_grabbed,
	_cep_ta_common_jump,
	_cep_ta_common_land,
	_cep_ta_common_lie,
	_cep_ta_common_pain,
	_cep_ta_common_prejump,
	_cep_ta_common_rise,
	_cep_ta_common_spawn,
	_cep_ta_common_turn,
	_cep_ta_common_vault,
	_cep_ta_normal_prepare,
	_cep_ta_npc_warp,
	_cep_ta_player_blink,
	_cep_ta_suicide,
	_cep_ta_the_end,
};

enum cep_think_enum { // 2011_03_03, DC: Think types.
    _cep_th_common_think,
    _cep_th_player_think,
    _cep_th_steam_think,
    _cep_th_steamer_think,
    _cep_th_text_think,
    _cep_th_trap_think,
    _cep_th_the_end,
};

void mapstrings_changeentityproperty(ScriptVariant** varlist, int paramCount)
{
	char* propname;
	int prop, i;

	static const char* proplist[] = {
		"aggression",
		"aiattack",
		"aiflag",
		"aimove",
		"alpha",
		"animation",
		"animhits",
		"animpos",
		"antigrab",
		"antigravity",
		"attacking",
		"attackid",
		"autokill",
		"base",
		"blink",
		"blockback",
		"blockodds",
		"blockpain",
		"bounce",
		"candamage",
		"combostep",
		"colourmap",
		"damage_on_landing",
		"dead",
		"defaultname",
		"defense",
		"detect",
		"direction",
		"dot",
		"edelay",
		"energycost",
		"escapehits",
		"escapecount",
		"falldie",
		"freezetime",
		"frozen",
		"gfxshadow",
		"grabforce",
		"guardpoints",
		"health",
		"hitbyid",
		"hmapl",
		"hmapu",
		"hostile",
		"iconposition",
		"invincible",
		"invinctime",
		"jugglepoints",
		"knockdowncount",
		"komap",
		"lifeposition",
		"lifespancountdown",
		"map",
		"maptime",
		"maxguardpoints",
		"maxhealth",
		"maxjugglepoints",
		"maxmp",
		"model",
		"mp",
		"mpset",
		"name",
		"nameposition",
		"nextanim",
		"nextthink",
		"no_adjust_base",
		"noaicontrol",
		"nodieblink",
		"nodrop",
		"nograb",
		"nopain",
		"offense",
		"opponent",
		"owner",
		"pain_time",
		"parent",
		"position",
		"projectile",
		"projectilehit",
		"running",
		"rush_count",
		"rush_tally",
		"rush_time",
		"score",
		"scroll",
		"seal",
		"sealtime",
		"setlayer",
		"speed",
		"spritea",
		"stalltime",
		"stats",
		"staydown",
		"staydownatk",
		"stealth",
		"subentity",
		"subject_to_gravity",
		"subject_to_hole",
		"subject_to_maxz",
		"subject_to_minz",
		"subject_to_obstacle",
		"subject_to_platform",
		"subject_to_screen",
		"subject_to_wall",
		"takeaction",
		"think",
		"thold",
		"throwdamage",
		"throwdist",
		"throwframewait",
		"throwheight",
		"tosstime",
		"trymove",
		"type",
		"velocity",
		"weapon",
	};

    static const char* proplist_aiflag[] = {
		"animating",
		"attacking",
		"autokill",
		"blink",
		"blocking",
		"charging",
		"dead",
		"drop",
		"falling",
		"frozen",
		"getting",
		"idling",
		"inpain",
		"invincible",
		"jumpid",
		"jumping",
		"projectile",
		"running",
		"toexplode",
		"turning",
	};

    static const char* proplist_energycost[] = {
        "cost",
        "disable",
        "mponly",
    };

    static const char* proplist_hostile_candamage[] = {
        "ground",
        "type_enemy",
        "type_npc",
        "type_obstacle",
        "type_player",
        "type_shot",
    };

    static const char* proplist_knockdowncount[] = { //2011_04_14, DC: Knockdowncount colleciton.
	    "current",
	    "max",
	    "time",
	};

    static const char* proplist_spritea[] = {       //2011_05_15, DC: Sprite array.
        "centerx",
        "centery",
        "file",
        "offsetx",
        "offsety",
        "sprite",
	};

    static const char* proplist_staydown[] = { //2011_04_08, DC: Staydown colleciton.
	    "rise",
	    "riseattack",
	    "riseattack_stall",
	};

	static const char* proplist_takeaction[] = {
		"bomb_explode",
		"common_attack_proc",
		"common_block",
		"common_drop",
		"common_fall",
		"common_get",
		"common_grab",
		"common_grabattack",
		"common_grabbed",
		"common_jump",
		"common_land",
		"common_lie",
		"common_pain",
		"common_prejump",
		"common_rise",
		"common_spawn",
		"common_turn",
		"common_vault",
		"normal_prepare",
		"npc_warp",
		"player_blink",
		"suicide",
	};

	static const char* proplist_think[] = { // 2011_03_03, DC: Think types.
		"common_think",
		"player_think",
		"steam_think",
		"steamer_think",
		"text_think",
		"trap_think",
	};

	// property name
	MAPSTRINGS(varlist[1], proplist, _cep_the_end,
		"Property name '%s' is not supported by function changeentityproperty.\n");

    // AI flag name for aiflag
	if((varlist[1]->vt == VT_INTEGER) && (varlist[1]->lVal == _cep_aiflag))
	{
		MAPSTRINGS(varlist[2], proplist_aiflag, _cep_aiflag_the_end,
			"Flag '%s' is not supported by 'aiflag'.\n");
	}

	//2011_03_31, DC: Energycost
	if((varlist[1]->vt == VT_INTEGER) && (varlist[1]->lVal == _cep_energycost))
	{
		MAPSTRINGS(varlist[2], proplist_energycost, _cep_energycost_the_end,
			"Flag '%s' is not supported by 'energycost'.\n");
	}

    // entity type(s) for hostile, candamage, and projectilehit.
	if((varlist[1]->vt == VT_INTEGER) &&
		((varlist[1]->lVal == _cep_hostile) || (varlist[1]->lVal == _cep_candamage) || (varlist[1]->lVal == _cep_projectilehit)))
	{
		for(i=2; i<paramCount; i++)
		{
			MAPSTRINGS(varlist[i], proplist_hostile_candamage, _cep_hcd_the_end,
				"Entity type '%s' is not supported by 'hostile', 'candamage', or 'projectilehit'\n");
		}
	}

	// 2011_04_14, DC: Knockdowncount
	if((varlist[1]->vt == VT_INTEGER) && (varlist[1]->lVal == _cep_knockdowncount))
	{
		MAPSTRINGS(varlist[2], proplist_knockdowncount, _cep_knockdowncount_the_end,
			"Subproperty '%s' is not supported by 'knockdowncount'.\n");
	}

	// 2011_05_15, DC: Sprite array
	if((varlist[1]->vt == VT_INTEGER) && (varlist[1]->lVal == _cep_spritea))
	{
		MAPSTRINGS(varlist[2], proplist_spritea, _cep_spritea_the_end,
			"Property name '%s' is not a known subproperty of 'spritea'.\n");
	}

    // 2011_04_08, DC: Staydown
	if((varlist[1]->vt == VT_INTEGER) && (varlist[1]->lVal == _cep_staydown))
	{
		MAPSTRINGS(varlist[2], proplist_staydown, _cep_staydown_the_end,
			"Subproperty '%s' is not supported by 'staydown'.\n");
	}

	// action for takeaction
	if((varlist[1]->vt == VT_INTEGER) && (varlist[1]->lVal == _cep_takeaction))
	{
		MAPSTRINGS(varlist[2], proplist_takeaction, _cep_ta_the_end,
			"Action '%s' is not supported by 'takeaction'.\n");
	}

    // 2011_03_13, DC: Think sets for think.
	if((varlist[1]->vt == VT_INTEGER) && (varlist[1]->lVal == _cep_think))
	{
		MAPSTRINGS(varlist[2], proplist_think, _cep_th_the_end,
			"Set '%s' is not supported by 'think'.\n");
	}
}

//changeentityproperty(pentity, propname, value1[ ,value2, value3, ...]);
HRESULT openbor_changeentityproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	entity* ent = NULL;
	s_model* tempmodel ;
	char* tempstr = NULL;
	LONG ltemp, ltemp2;
	DOUBLE dbltemp;
	int propind;
	int i = 0;

	static const void* actions[] = { // for takeaction
		bomb_explode,
		common_attack_proc,
		common_block,
		common_drop,
		common_fall,
		common_get,
		common_grab,
		common_grabattack,
		common_grabbed,
		common_jump,
		common_land,
		common_lie,
		common_pain,
		common_prejump,
		common_rise,
		common_spawn,
		common_turn,
		common_vault,
		normal_prepare,
		npc_warp,
		player_blink,
		suicide,
	};

	static const int entitytypes[] = {
        0, // "ground"; not a real entity type
        TYPE_ENEMY,
        TYPE_NPC,
        TYPE_OBSTACLE,
        TYPE_PLAYER,
        TYPE_SHOT,

    };

	static const void* think[] = { // 2011_03_03, DC: Think types.
		common_think,
		player_think,
		steam_think,
		steamer_think,
		text_think,
		trap_think,
	};

	if(paramCount < 3)
	{
		printf("Function changeentityproperty must have have at least 3 parameters.");
		goto changeentityproperty_error;
	}

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	(*pretvar)->lVal = (LONG)0;
	mapstrings_changeentityproperty(varlist, paramCount);

	if(varlist[0]->vt != VT_PTR && varlist[0]->vt != VT_EMPTY)
	{
		printf("Function changeentityproperty must have a valid entity handle.");
		goto changeentityproperty_error;
	}
	ent = (entity*)varlist[0]->ptrVal; //retrieve the entity
	if(!ent)
	{
		(*pretvar)->lVal = (LONG)0;
		return S_OK;
	}

	if(varlist[1]->vt != VT_INTEGER)
	{
		if(varlist[1]->vt != VT_STR)
			printf("Function changeentityproperty must have a string property name.\n");
		goto changeentityproperty_error;
	}

	propind = varlist[1]->lVal;

	switch(propind)
	{
    case _cep_aggression:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->modeldata.aggression = (int)ltemp;
		break;
	}
	case _cep_aiattack:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->modeldata.aiattack = (int)ltemp;
		break;
	}
	case _cep_aiflag:
	{
		if(varlist[2]->vt != VT_INTEGER)
		{
			if(varlist[2]->vt != VT_STR)
				printf("You must give a string value for AI flag name.\n");
			goto changeentityproperty_error;
		}
		if(paramCount<4) break;

		switch(varlist[2]->lVal)
		{
		case _cep_aiflag_dead:
		{
			if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
				ent->dead = (int)ltemp;
			break;
		}
		case _cep_aiflag_jumpid:
		{
			if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
				ent->jumpid = (int)ltemp;
			break;
		}
		case _cep_aiflag_jumping:
		{
			if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
				ent->jumping = (int)ltemp;
			break;
		}
		case _cep_aiflag_idling:
		{
			if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
				ent->idling = (int)ltemp;
			break;
		}
		case _cep_aiflag_drop:
		{
			if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
				ent->drop = (int)ltemp;
			break;
		}
		case _cep_aiflag_attacking:
		{
			if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
				ent->attacking = (int)ltemp;
			break;
		}
		case _cep_aiflag_getting:
		{
			if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
				ent->getting = (int)ltemp;
			break;
		}
		case _cep_aiflag_turning:
		{
			 if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
				 ent->turning = (int)ltemp;
			 break;
		}
		case _cep_aiflag_charging:
		{
			 if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
				 ent->charging = (int)ltemp;
			 break;
		}
		case _cep_aiflag_blocking:
		{
			 if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
				 ent->blocking = (int)ltemp;
			 break;
		}
		case _cep_aiflag_falling:
		{
			 if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
				 ent->falling = (int)ltemp;
			 break;
		}
		case _cep_aiflag_running:
		{
			 if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
				  ent->running = (int)ltemp;
			 break;
		}
		case _cep_aiflag_inpain:
		{
			 if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
				  ent->inpain = (int)ltemp;
			 break;
		}
		case _cep_aiflag_projectile:
		{
			 if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
				  ent->projectile = (int)ltemp;
			 break;
		}
		case _cep_aiflag_frozen:
		{
			 if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
				  ent->frozen = (int)ltemp;
			 break;
		}
		case _cep_aiflag_toexplode:
		{
			 if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
				  ent->toexplode = (int)ltemp;
			 break;
		}
		case _cep_aiflag_animating:
		{
			 if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
				  ent->animating = (int)ltemp;
			 break;
		}
		case _cep_aiflag_blink:
		{
			 if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
				  ent->blink = (int)ltemp;
			 break;
		}
		case _cep_aiflag_invincible:
		{
			 if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
				  ent->invincible = (int)ltemp;
			 break;
		}
		case _cep_aiflag_autokill:
		{
			 if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
				  ent->autokill = (int)ltemp;
			 break;
		}
		default:
			printf("Unknown AI flag.\n");
			goto changeentityproperty_error;
		}
		break;
	}
	case _cep_aimove:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->modeldata.aimove = (int)ltemp;
		break;
	}
	case _cep_alpha:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->modeldata.alpha = (int)ltemp;
		}
		break;
	}
    case _cep_animation:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			(*pretvar)->lVal = (LONG)1;
		if(paramCount >= 4)
		{
			if(FAILED(ScriptVariant_IntegerValue(varlist[3], &ltemp2)))
				(*pretvar)->lVal = (LONG)0;
		}
		else ltemp2 = (LONG)1;
		if((*pretvar)->lVal == (LONG)1)
		{
			ent_set_anim(ent, (int)ltemp, (int)ltemp2);
		}
		break;
	}
	case _cep_animhits:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->animation->animhits = (int)ltemp;
		}
		break;
	}
	case _cep_animpos:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->animpos = (int)ltemp;
		break;
	}
	case _cep_antigrab:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->model->antigrab = (int)ltemp;
		break;
	}
    case _cep_antigravity:
	{
		if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->modeldata.antigravity = (float)dbltemp;
		}
		break;
	}
	case _cep_attacking:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
		   ent->attacking = (int)ltemp;
		   (*pretvar)->lVal = (LONG)1;
		}
		break;
	}
	case _cep_attackid:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->attack_id = (int)ltemp;
		break;
	}
    case _cep_autokill:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			ent->autokill = (int)ltemp;
		   (*pretvar)->lVal = (LONG)1;
		}
		break;
	}
	case _cep_base:
	{
		if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
		{
			ent->base = (float)dbltemp;
			(*pretvar)->lVal = (LONG)1;
		}
		break;
	}
	case _cep_blink:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			ent->blink = (int)ltemp;
			(*pretvar)->lVal = (LONG)1;
		}
		break;
	}
	case _cep_blockback:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->modeldata.blockback = (int)ltemp;
		break;
	}
    case _cep_blockodds:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->modeldata.blockodds = (int)ltemp;
		}
		break;
	}
	case _cep_blockpain:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->modeldata.blockpain = (int)ltemp;
		break;
	}
	case _cep_bounce:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->modeldata.bounce = (int)ltemp;
		break;
	}
	case _cep_candamage:
	{
		ent->modeldata.candamage = 0;

		for(i=2; i<paramCount; i++)
		{
			if(varlist[i]->vt == VT_INTEGER) // known entity type
			{
				ltemp = varlist[i]->lVal;
				if(ltemp==_cep_hcd_ground) // "ground" - not needed?
					ent->modeldata.ground = 1;
				else
					ent->modeldata.candamage |= entitytypes[(int)ltemp];
			}
			else
			{
				printf("You must pass one or more string constants for entity type.\n");
				goto changeentityproperty_error;
			}
		}
		break;
	}
	case _cep_combostep:
	{
		if(paramCount >= 4 &&
		   SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
		}
		if((*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp2)))
				ent->combostep[(int)ltemp]=(int)ltemp2;
			else (*pretvar)->lVal = (LONG)0;
		}
		break;
	}
	case _cep_colourmap:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			self->colourmap = self->modeldata.colourmap[ltemp-1];
		}
		break;
	}
	case _cep_damage_on_landing:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->damage_on_landing = (int)ltemp;
		break;
	}
    case _cep_dead:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->dead = (int)ltemp;
		break;
	}
    case _cep_defaultname:
	{
		if(varlist[2]->vt != VT_STR)
		{
			printf("You must give a string value for entity name.\n");
			goto changeentityproperty_error;
		}
		tempmodel = findmodel((char*)StrCache_Get(varlist[2]->strVal));
		if(!tempmodel)
		{
			printf("Use must give an existing model's name for entity's default model name.\n");
			goto changeentityproperty_error;
		}
		ent->defaultmodel = tempmodel;
		(*pretvar)->lVal = (LONG)1;
		break;
	}
	case _cep_defense:
	{
		if(paramCount >= 4 &&
		   SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)) &&
		   ltemp < (LONG)MAX_ATKS && ltemp >= (LONG)0)
		{
			(*pretvar)->lVal = (LONG)1;
		}
		if((*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
				ent->modeldata.defense_factors[(int)ltemp] = (float)dbltemp;
			else (*pretvar)->lVal = (LONG)0;
		}
		if(paramCount >= 5 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[4], &dbltemp)))
				ent->modeldata.defense_pain[(int)ltemp] = (float)dbltemp;
			else (*pretvar)->lVal = (LONG)0;
		}
		if(paramCount >= 6 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[5], &dbltemp)))
				ent->modeldata.defense_knockdown[(int)ltemp] = (float)dbltemp;
			else (*pretvar)->lVal = (LONG)0;
		}
		if(paramCount >= 7 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[6], &dbltemp)))
				ent->modeldata.defense_blockpower[(int)ltemp] = (float)dbltemp;
			else (*pretvar)->lVal = (LONG)0;
		}
		if(paramCount >= 8 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[7], &dbltemp)))
				ent->modeldata.defense_blockthreshold[(int)ltemp] = (float)dbltemp;
			else (*pretvar)->lVal = (LONG)0;
		}
		if(paramCount >= 9 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[8], &dbltemp)))
				ent->modeldata.defense_blockratio[(int)ltemp] = (float)dbltemp;
			else (*pretvar)->lVal = (LONG)0;
		}
		if(paramCount >= 10 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[9], &dbltemp)))
				ent->modeldata.defense_blocktype[(int)ltemp] = (float)dbltemp;
			else (*pretvar)->lVal = (LONG)0;
		}
		break;
	}
    case _cep_detect:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->modeldata.stealth.detect = (int)ltemp;
		break;
	}
	case _cep_direction:
	{
		if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
		{
			ent->direction = (int)dbltemp;
			(*pretvar)->lVal = (LONG)1;
		}
		break;
	}
	case _cep_dot:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			i = (int)ltemp;
		}
		if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->dot_time[i] = (int)dbltemp;
		}
		if(paramCount >= 5 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[4], &dbltemp)))
				ent->dot[i] = (int)dbltemp;
			else (*pretvar)->lVal = (LONG)0;
		}
		if(paramCount >= 6 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[5], &dbltemp)))
				ent->dot_force[i] = (int)dbltemp;
			else (*pretvar)->lVal = (LONG)0;
		}
		if(paramCount >= 7 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[6], &dbltemp)))
				ent->dot_rate[i] = (int)dbltemp;
			else (*pretvar)->lVal = (LONG)0;
		}
		if(paramCount >= 8 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[7], &dbltemp)))
				ent->dot_atk[i] = (int)dbltemp;
			else (*pretvar)->lVal = (LONG)0;
		}
		if(paramCount >= 9)
		{
			ent->dot_owner[i] = (entity*)varlist[8]->ptrVal;
		}
		break;
	}
	case _cep_edelay:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->modeldata.edelay.mode = (int)ltemp;
		}
		if(paramCount >= 3 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[4], &dbltemp)))
				ent->modeldata.edelay.factor = (float)dbltemp;
		}
		if(paramCount >= 4 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[4], &ltemp)))
				ent->modeldata.edelay.cap_min = (int)ltemp;
			else (*pretvar)->lVal = (LONG)0;
		}
		if(paramCount >= 5 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[5], &ltemp)))
				ent->modeldata.edelay.cap_max = (int)ltemp;
			else (*pretvar)->lVal = (LONG)0;
		}
		if(paramCount >= 6 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[6], &ltemp)))
				ent->modeldata.edelay.range_min = (int)ltemp;
			else (*pretvar)->lVal = (LONG)0;
		}
		if(paramCount >= 7 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[7], &ltemp)))
				ent->modeldata.edelay.range_max = (int)ltemp;
			else (*pretvar)->lVal = (LONG)0;
		}
		break;
	}
	case _cep_energycost:
	{
		if(paramCount != 5)
		{
			printf("\n Error, changeentityproperty({ent}, 'energycost', {subproperty}, {animation}, {value}): Invalid or missing parameter. \n");
			goto changeentityproperty_error;
		}

		(*pretvar)->lVal = (LONG)1;

		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
		{
			i = (int)ltemp;
		}

		if(!validanim(ent,i))
		{
			printf("\n Error, changeentityproperty({ent}, 'energycost', {subproperty}, {animation}, {value}): {animation} parameter invalid. Make sure the animation exists. \n");
			goto changeentityproperty_error;
		}

		if(varlist[2]->vt != VT_INTEGER)
		{
			if(varlist[2]->vt != VT_STR)
				printf("You must give a string value for energycost flag name.\n");
			goto changeentityproperty_error;
		}

		switch(varlist[2]->lVal)
		{
		case _cep_energycost_cost:
		{
			if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[4], &ltemp)))
				ent->modeldata.animation[i]->energycost.cost = (int)ltemp;
			break;
		}
		case _cep_energycost_disable:
		{
			if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[4], &ltemp)))
				ent->modeldata.animation[i]->energycost.disable = (int)ltemp;
			break;
		}
		case _cep_energycost_mponly:
		{
			if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[4], &ltemp)))
				ent->modeldata.animation[i]->energycost.mponly = (int)ltemp;
			break;
		}
		default:
			printf("Unknown Energycost flag.\n");
			goto changeentityproperty_error;
		}
		break;
	}
	case _cep_escapecount:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->escapecount = (int)ltemp;
		}
		break;
	}
	case _cep_escapehits:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->modeldata.escapehits = (int)ltemp;
		}
		break;
	}
    case _cep_falldie:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->modeldata.falldie = (int)ltemp;
		break;
	}
	case _cep_pain_time:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->pain_time = (int)ltemp;
		break;
	}
	case _cep_freezetime:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->freezetime = (int)ltemp;
		break;
	}
	case _cep_frozen:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->frozen = (int)ltemp;
		break;
	}
    case _cep_gfxshadow:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->modeldata.gfxshadow = (int)ltemp;
		break;
	}
    case _cep_grabforce:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->model->grabforce = (int)ltemp;
		break;
	}
	case _cep_guardpoints:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->modeldata.guardpoints.current = (int)ltemp;
		}
		break;
	}
    case _cep_health:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			ent->health = (int)ltemp;
			if(ent->health > ent->modeldata.health) ent->health = ent->modeldata.health;
			else if(ent->health < 0) ent->health = 0;
			(*pretvar)->lVal = (LONG)1;
		}
		break;
	}
    case _cep_hitbyid:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->hit_by_attack_id = (int)ltemp;
		break;
	}
    case _cep_hmapl:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			(*pretvar)->lVal = (LONG)1;
			self->modeldata.maps.hide_start = ltemp;
		break;
	}
	case _cep_hmapu:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			(*pretvar)->lVal = (LONG)1;
			self->modeldata.maps.hide_end = ltemp;
		break;
	}
    case _cep_hostile:
	{

		ent->modeldata.hostile = 0;

		for(i=2; i<paramCount; i++)
		{
			if(varlist[i]->vt == VT_INTEGER) // known entity type
			{
				ltemp = varlist[i]->lVal;
				ent->modeldata.hostile |= entitytypes[(int)ltemp];
			}
			else
			{
				printf("You must pass one or more string constants for entity type.\n");
				goto changeentityproperty_error;
			}
		}

		break;
	}
	case _cep_iconposition:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->modeldata.icon.x = (int)ltemp;
		if(paramCount>3 && SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
			ent->modeldata.icon.y = (int)ltemp;
		break;
	}
	case _cep_invincible:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			ent->invincible = (int)ltemp;
			(*pretvar)->lVal = (LONG)1;
		}
		break;
	}
	case _cep_invinctime:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->invinctime = (int)ltemp;
		break;
	}
	case _cep_jugglepoints:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->modeldata.jugglepoints.current = (int)ltemp;
		}
		break;
	}
	case _cep_knockdowncount:
	{
		if(varlist[2]->vt != VT_INTEGER)
		{
			if(varlist[2]->vt != VT_STR)
				printf("You must provide a string value for Knockdowncount subproperty:\n\
                        changeentityproperty({ent}, 'knockdowncount', {subproperty}, {value})\n\
                        ~'current'\n\
                        ~'max'\n\
                        ~'time'\n");
			goto changeentityproperty_error;
		}

		switch(varlist[2]->lVal)
		{
            case _cep_knockdowncount_current:
            {
                if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
                {
                    (*pretvar)->lVal = (LONG)1;
                    ent->knockdowncount = (float)dbltemp;
                }
            }
            case _cep_knockdowncount_max:
            {
                if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
                {
                    (*pretvar)->lVal = (LONG)1;
                    ent->modeldata.knockdowncount = (float)dbltemp;
                }
            case _cep_knockdowncount_time:
                if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
                {
                    (*pretvar)->lVal = (LONG)1;
                    ent->knockdowntime = (int)ltemp;
                }
            }
            default:
                printf("Unknown knockdowncount subproperty.\n");
                goto changeentityproperty_error;
		}
		break;
	}
    case _cep_komap:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->modeldata.maps.ko = (int)ltemp;
		}
		if(paramCount >= 4 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
				ent->modeldata.maps.kotype = (int)ltemp;
			else (*pretvar)->lVal = (LONG)0;
		}
		break;
	}
    case _cep_lifeposition:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->modeldata.hpx = (int)ltemp;
		if(paramCount>3 && SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
			ent->modeldata.hpy = (int)ltemp;
		break;
	}
    case _cep_lifespancountdown:
	{
		if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
			ent->lifespancountdown = (float)dbltemp;
		break;
	}
    case _cep_map:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			ent_set_colourmap(ent, (int)ltemp);
			(*pretvar)->lVal = (LONG)1;
		}
		break;
	}
    case _cep_maptime:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->maptime = (int)ltemp;
		}
		break;
	}
    case _cep_maxguardpoints:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->modeldata.guardpoints.maximum = (int)ltemp;
		}
		break;
	}
	case _cep_maxhealth:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->modeldata.health = (int)ltemp;
			if(ent->modeldata.health < 0) ent->modeldata.health = 0; //OK, no need to have ot below 0
		}
		break;
	}
	case _cep_maxjugglepoints:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->modeldata.jugglepoints.maximum = (int)ltemp;
		}
		break;
	}
    case _cep_maxmp:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			ent->modeldata.mp = (int)ltemp;
			if(ent->modeldata.mp < 0) ent->modeldata.mp = 0; //OK, no need to have ot below 0
			(*pretvar)->lVal = (LONG)1;
		}
		break;
	}
    case _cep_model:
	{
		if(varlist[2]->vt != VT_STR)
		{
			printf("You must give a string value for model name.\n");
			goto changeentityproperty_error;
		}
		tempstr = (char*)StrCache_Get(varlist[2]->strVal);
		if(paramCount > 3)
		{
			if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
				(*pretvar)->lVal = (LONG)1;
		}
		else
		{
			ltemp = (LONG)0;
			(*pretvar)->lVal = (LONG)1;
		}
		if((*pretvar)->lVal == (LONG)1) set_model_ex(ent, tempstr, -1, NULL, (int)ltemp);
		if(!ent->weapent) ent->weapent = ent;
		break;
	}
    case _cep_mp:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			ent->mp = (int)ltemp;
			if(ent->mp > ent->modeldata.mp) ent->mp = ent->modeldata.mp;
			else if(ent->mp < 0) ent->mp = 0;
			(*pretvar)->lVal = (LONG)1;
		}
		break;
	}
    case _cep_mpset:
	{
		if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->modeldata.mp = (int)dbltemp;
		}
		if(paramCount >= 4 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
				ent->modeldata.mpstable = (int)dbltemp;
		}
		if(paramCount >= 5 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[4], &dbltemp)))
				ent->modeldata.mpstableval = (int)dbltemp;
		}
		if(paramCount >= 6 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[5], &dbltemp)))
				ent->modeldata.mprate = (int)dbltemp;
		}
		if(paramCount >= 7 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[6], &dbltemp)))
				ent->modeldata.mpdroprate = (int)dbltemp;
		}
		if(paramCount >= 8 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[7], &dbltemp)))
				ent->modeldata.chargerate = (int)dbltemp;
		}
		break;
	}
    case _cep_name:
	{
		if(varlist[2]->vt != VT_STR)
		{
			printf("You must give a string value for entity name.\n");
			goto changeentityproperty_error;
		}
		strcpy(ent->name, (char*)StrCache_Get(varlist[2]->strVal));
		(*pretvar)->lVal = (LONG)1;
		break;
	}
    case _cep_nameposition:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->modeldata.namex = (int)ltemp;
		if(paramCount>3 && SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
			ent->modeldata.namey = (int)ltemp;
		break;
	}
    case _cep_nextanim:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->nextanim = (int)ltemp;
		}
		break;
	}
	case _cep_nextthink:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->nextthink = (int)ltemp;
		}
		break;
	}
    case _cep_no_adjust_base:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->modeldata.no_adjust_base = (int)ltemp;
		break;
	}
	case _cep_noaicontrol:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->noaicontrol = (int)ltemp;
		break;
	}
	case _cep_nodieblink:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->modeldata.nodieblink = (int)ltemp;
		break;
	}
    case _cep_nodrop:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->modeldata.nodrop = (int)ltemp;
		break;
	}
	case _cep_nograb:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->nograb = (int)ltemp;
		break;
	}
	case _cep_nopain:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->modeldata.nopain = (int)ltemp;
		break;
	}
    case _cep_offense:
	{
		if(paramCount >= 4 &&
		   SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)) &&
		   ltemp < (LONG)MAX_ATKS && ltemp >= (LONG)0)
		{
			(*pretvar)->lVal = (LONG)1;
		}
		if((*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
				ent->modeldata.offense_factors[(int)ltemp] = (float)dbltemp;
			else (*pretvar)->lVal = (LONG)0;
		}
		break;
	}
    case _cep_opponent:
	{
		ent->opponent = (entity*)varlist[2]->ptrVal;
		break;
	}
    case _cep_owner:
	{
		ent->owner = (entity*)varlist[2]->ptrVal;
		break;
	}
	case _cep_parent:
	{
		ent->parent = (entity*)varlist[2]->ptrVal;
		break;
	}
    case _cep_position:
	{
		if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->x = (float)dbltemp;
		}
		if(paramCount >= 4 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
				ent->z = (float)dbltemp;
			else (*pretvar)->lVal = (LONG)0;
		}
		if(paramCount >= 5 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[4], &dbltemp)))
				ent->a = (float)dbltemp;
			else (*pretvar)->lVal = (LONG)0;
		}
		break;
	}
    case _cep_projectile:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->projectile = (int)ltemp;
		break;
	}
    case _cep_projectilehit:
	{

		ent->modeldata.projectilehit = 0;

		for(i=2; i<paramCount; i++)
		{
			if(varlist[i]->vt == VT_INTEGER) // known entity type
			{
				ltemp = varlist[i]->lVal;
				ent->modeldata.projectilehit |= entitytypes[(int)ltemp];
			}
			else
			{
				printf("You must pass one or more string constants for entity type.\n");
				goto changeentityproperty_error;
			}
		}

		break;
	}
    case _cep_running:
	{
		if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->modeldata.runspeed = (float)dbltemp;
		}
		if(paramCount >= 4 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
				ent->modeldata.runjumpheight = (float)dbltemp;
		}
		if(paramCount >= 5 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[4], &dbltemp)))
				ent->modeldata.runjumpdist = (float)dbltemp;
		}
		if(paramCount >= 6 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[5], &dbltemp)))
				ent->modeldata.runhold = (int)dbltemp;
		}
		if(paramCount >= 7 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[6], &dbltemp)))
				ent->modeldata.runupdown = (int)dbltemp;
		}

		break;
	}
    case _cep_rush_count:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->rush[0] = (int)ltemp;
		}
		break;
	}
	case _cep_rush_tally:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->rush[1] = (int)ltemp;
		}
		break;
	}
	case _cep_rush_time:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->rushtime = (int)ltemp;
		break;
	}
	case _cep_score:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->modeldata.score = (int)ltemp;
		}
		break;
	}
    case _cep_scroll:
	{
		if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->modeldata.scroll = (float)dbltemp;
		}
		break;
	}
    case _cep_seal:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->seal = (int)ltemp;
		break;
	}
	case _cep_sealtime:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->sealtime = (int)ltemp;
		break;
	}
    case _cep_setlayer:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			ent->modeldata.setlayer = (int)ltemp;
			(*pretvar)->lVal = (LONG)1;
		}
		break;
	}
	case _cep_speed:
	{
		if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->modeldata.speed = (float)dbltemp;
		}
		break;
	}
	case _cep_spritea:
	{
	    if(varlist[2]->vt != VT_INTEGER)
		{
			if(varlist[2]->vt != VT_STR)
				printf("You must provide a string value for Sprite Array subproperty:\n\
                        changeentityproperty({ent}, 'spritea', {subproperty}, {animation ID}, {frame}, {value})\n\
                        ~'centerx'\n\
                        ~'centery'\n\
                        ~'file'\n\
                        ~'offsetx'\n\
                        ~'sprite'\n");
			goto changeentityproperty_error;
		}

        ltemp   = varlist[2]->lVal;

        /*
        Failsafe checks. Any attempt to access a sprite property on invalid frame would cause instant shutdown.
        */
        if(!validanim(ent, varlist[3]->lVal) || !(ent->modeldata.animation[varlist[3]->lVal]->numframes >= varlist[4]->lVal) || paramCount<5)
        {
            break;
        }

        i = ent->modeldata.animation[varlist[3]->lVal]->sprite[varlist[4]->lVal];   //Get sprite index.

		switch(ltemp)
		{
            case _cep_spritea_centerx:
            {
                if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[5], &ltemp)))
                {
                    sprite_map[i].centerx = (int)ltemp;
                    (*pretvar)->lVal = (LONG)1;
                }
                break;
            }
            case _cep_spritea_centery:
            {
                if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[5], &ltemp)))
                {
                    sprite_map[i].centery = (int)ltemp;
                    (*pretvar)->lVal = (LONG)1;
                }
                break;
            }
            case _cep_spritea_file:
            {
                if(varlist[5]->vt != VT_STR)
                {
                    printf("You must provide a string value for file name.\n");
                    goto changeentityproperty_error;
                }
                strcpy(sprite_map[i].filename, (char*)StrCache_Get(varlist[5]->strVal));
                (*pretvar)->lVal = (LONG)1;
                break;
            }
            case _cep_spritea_offsetx:
            {
                if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[5], &ltemp)))
                {
                    sprite_map[i].ofsx = (int)ltemp;
                    (*pretvar)->lVal = (LONG)1;
                }
                break;
            }
            case _cep_spritea_offsety:
            {
                if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
                {
                    sprite_map[i].ofsy = (int)ltemp;
                    (*pretvar)->lVal = (LONG)1;
                }
                break;
            }
            case _cep_spritea_sprite:
            {
                sprite_map[i].sprite = (VOID*)varlist[5]->ptrVal;
                (*pretvar)->lVal = (LONG)1;
                break;
            }
            default:
                printf("Unknown Sprite Array subproperty.\n");
                goto changeentityproperty_error;
		}
		break;
	}
    case _cep_stalltime:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->stalltime = (int)ltemp;
		break;
	}
	case _cep_stats:
	{
		if(paramCount<4) break;

		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[4], &dbltemp)))
			{
				switch(varlist[2]->lVal)
				{
					default:
						ent->modeldata.stats[(int)ltemp] = (float)dbltemp;
						break;
					case 1:
						ent->stats[(int)ltemp] = (float)dbltemp;
						break;
				}
			}
		}
		break;
	}
	case _cep_staydown:
	{
		if(varlist[2]->vt != VT_INTEGER)
		{
			if(varlist[2]->vt != VT_STR)
				printf("You must provide a string value for Staydown subproperty:\n\
                        changeentityproperty({ent}, 'staydown', {subproperty}, {value})\n\
                        ~'rise'\n\
                        ~'riseattack'\n\
                        ~'riseattack_stall'\n");
			goto changeentityproperty_error;
		}
		if(paramCount<4) break;

		switch(varlist[2]->lVal)
		{
            case _cep_staydown_rise:
            {
                if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                    ent->staydown.rise = (int)ltemp;
                break;
            }
            case _cep_staydown_riseattack:
            {
                if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                    ent->staydown.riseattack = (int)ltemp;
                break;
            }
            case _cep_staydown_riseattack_stall:
            {
                if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                    ent->staydown.riseattack_stall = (int)ltemp;
                break;
            }
            default:
                printf("Unknown Staydown subproperty.\n");
                goto changeentityproperty_error;
		}
		break;
	}
    case _cep_stealth:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
			ent->modeldata.stealth.hide = (int)ltemp;
		break;
	}
    case _cep_subentity:
	{
		if(ent->subentity) ent->subentity->parent = NULL;
		ent->subentity = (entity*)varlist[2]->ptrVal;
		if(ent->subentity) ent->subentity->parent = ent;
		break;
	}
    case _cep_subject_to_gravity:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			ent->modeldata.subject_to_gravity = (int)ltemp;
			(*pretvar)->lVal = (LONG)1;
		}
		break;
	}
	case _cep_subject_to_hole:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			ent->modeldata.subject_to_hole = (int)ltemp;
			(*pretvar)->lVal = (LONG)1;
		}
		break;
	}
	case _cep_subject_to_maxz:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			ent->modeldata.subject_to_maxz = (int)ltemp;
			(*pretvar)->lVal = (LONG)1;
		}
		break;
	}
	case _cep_subject_to_minz:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			ent->modeldata.subject_to_minz = (int)ltemp;
			(*pretvar)->lVal = (LONG)1;
		}
		break;
	}
	case _cep_subject_to_obstacle:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			ent->modeldata.subject_to_obstacle = (int)ltemp;
			(*pretvar)->lVal = (LONG)1;
		}
		break;
	}
	case _cep_subject_to_platform:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			ent->modeldata.subject_to_platform = (int)ltemp;
			(*pretvar)->lVal = (LONG)1;
		}
		break;
	}
	case _cep_subject_to_screen:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			ent->modeldata.subject_to_screen = (int)ltemp;
			(*pretvar)->lVal = (LONG)1;
		}
		break;
	}
	case _cep_subject_to_wall:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			ent->modeldata.subject_to_wall = (int)ltemp;
			(*pretvar)->lVal = (LONG)1;
		}
		break;
	}
	case _cep_takeaction:
	{
		if(varlist[2]->vt == VT_STR)
		{ // not a known action; if it were it would been mapped by mapstrings
			ent->takeaction = NULL;
			break;
		}
		else if(varlist[2]->vt != VT_INTEGER)
		{
			printf("You must give a string value for action type.\n");
			goto changeentityproperty_error;
		}

		// otherwise, the parameter is a known action
		ltemp = varlist[2]->lVal;
		if((ltemp >= 0) && (ltemp < _cep_ta_the_end))
		{
			ent->takeaction = actions[(int)ltemp];
		}

		break;
	}
	case _cep_think:
	{
		if(varlist[2]->vt == VT_STR)
		{ // not a known action; if it were it would been mapped by mapstrings
			//ent->think = NULL;
			break;
		}
		else if(varlist[2]->vt != VT_INTEGER)
		{
			printf("You must give a string value for think type.\n");
			goto changeentityproperty_error;
		}

		// otherwise, the parameter is a known action
		ltemp = varlist[2]->lVal;
		if((ltemp >= 0) && (ltemp < _cep_th_the_end))
		{
			ent->think = think[(int)ltemp];
		}

		break;
	}
	case _cep_thold:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->modeldata.thold = (int)ltemp;
		}
		break;
	}
    case _cep_throwdamage:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			ent->modeldata.throwdamage = (int)ltemp;
			(*pretvar)->lVal = (LONG)1;
		}
		break;
	}
	case _cep_throwdist:
	{
		if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->modeldata.throwdist = (float)dbltemp;
		}
		break;
	}
	case _cep_throwframewait:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			ent->modeldata.throwframewait = (int)ltemp;
			(*pretvar)->lVal = (LONG)1;
		}
		break;
	}
	case _cep_throwheight:
	{
		if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->modeldata.throwheight = (float)dbltemp;
		}
		break;
	}
    case _cep_tosstime:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->toss_time = (int)ltemp;
		}
		break;
	}
    case _cep_trymove:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			if(ltemp == 1)
				 ent->trymove = common_trymove;
			else if(ltemp == 2)
				 ent->trymove = player_trymove;
			else
				 ent->trymove = NULL;
		}
		break;
	}
	case _cep_type:
	{
		if(varlist[2]->vt != VT_INTEGER)
		{
			printf("You must provide a type constant for type.\n");
			goto changeentityproperty_error;
		}

		ltemp = varlist[2]->lVal;
        ent->modeldata.type = (int)ltemp;

		break;
	}
    case _cep_velocity:
	{
		if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->xdir = (float)dbltemp;
		}
		if(paramCount >= 4 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
				ent->zdir = (float)dbltemp;
			else (*pretvar)->lVal = (LONG)0;
		}
		if(paramCount >= 5 && (*pretvar)->lVal == (LONG)1)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[4], &dbltemp)))
				ent->tossv = (float)dbltemp;
			else (*pretvar)->lVal = (LONG)0;
		}
		break;
	}
	case _cep_weapon:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			if(paramCount > 3)
			{
				if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp2)))
					(*pretvar)->lVal = (LONG)1;
			}
			else
			{
				ltemp2 = (LONG)0;
				(*pretvar)->lVal = (LONG)1;
			}
			set_weapon(ent, (int)ltemp, (int)ltemp2);
			(*pretvar)->lVal = (LONG)1;
		}
		break;
	}
	default:
		//printf("Property name '%s' is not supported by function changeentityproperty.\n", propname);
		goto changeentityproperty_error;
		break;
	}

	return S_OK;
changeentityproperty_error:
	*pretvar = NULL;
	return E_FAIL;
}

//tossentity(entity, height, speedx, speedz)
HRESULT openbor_tossentity(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	entity* ent = NULL;
	DOUBLE height=0, speedx=0, speedz=0;

	if(paramCount < 1)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	(*pretvar)->lVal = (LONG)1;

	ent = (entity*)varlist[0]->ptrVal; //retrieve the entity
	if(!ent)
	{
		(*pretvar)->lVal = (LONG)0;

		return S_OK;
	}

	if(paramCount >= 2)
	{
		if(FAILED(ScriptVariant_DecimalValue(varlist[1], &height)))
		{
			(*pretvar)->lVal = (LONG)0;
			return S_OK;
		}
	}
	if(paramCount >= 3)
	{
		if(FAILED(ScriptVariant_DecimalValue(varlist[2], &speedx)))
		{
			(*pretvar)->lVal = (LONG)0;
			return S_OK;
		}
	}
	if(paramCount >= 4)
	{
		if(FAILED(ScriptVariant_DecimalValue(varlist[3], &speedz)))
		{
			(*pretvar)->lVal = (LONG)0;
			return S_OK;
		}
	}
	ent->xdir = (float)speedx;
	ent->zdir = (float)speedz;
	toss(ent, (float)height);
	return S_OK;
}

// ===== getplayerproperty =====
enum getplayerproperty_enum {
	_gpp_credits,
	_gpp_ent,
	_gpp_entity,
	_gpp_keys,
	_gpp_lives,
	_gpp_name,
	_gpp_playkeys,
	_gpp_score,
	_gpp_weaponum,
	_gpp_the_end,
};

void mapstrings_getplayerproperty(ScriptVariant** varlist, int paramCount)
{
	char* propname;
	int prop;

	static const char* proplist[] = {
		"credits",
		"ent",
		"entity",
		"keys",
		"lives",
		"name",
		"playkeys",
		"score",
		"weaponum"
	};

	if(paramCount < 2) return;

	// property name
	MAPSTRINGS(varlist[1], proplist, _gpp_the_end,
		"Property name '%s' is not supported by function getplayerproperty.\n");
}

//getplayerproperty(index, propname);
HRESULT openbor_getplayerproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG ltemp;
	int index;
	entity* ent = NULL;
	int prop = -1;
	ScriptVariant* arg = NULL;

	if(paramCount < 2)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	mapstrings_getplayerproperty(varlist, paramCount);
	ScriptVariant_Clear(*pretvar);

	arg = varlist[0];
	if(FAILED(ScriptVariant_IntegerValue(arg, &ltemp)))
	{
		index = 0;
	} else index = (int)ltemp;
	if(!(ent = player[index].ent))  //this player is not selected, so just return
	{
		return S_OK; //return S_OK, to tell the engine it is not a FATAL error
	}

	arg = varlist[1];
	if(arg->vt!=VT_INTEGER)
	{
		if(arg->vt!=VT_STR)
			printf("Function call getplayerproperty has invalid propery name parameter, it must be a string value.\n");
		*pretvar = NULL;
		return E_FAIL;
	}
	prop = arg->lVal;

	//change the model
	switch(prop)
	{
	case _gpp_ent:
	case _gpp_entity:
	{
		ScriptVariant_ChangeType(*pretvar, VT_PTR);
		(*pretvar)->ptrVal = (VOID*)ent;
		break;
	}
	case _gpp_name:
	{
		ScriptVariant_ChangeType(*pretvar, VT_STR);
		strncpy(StrCache_Get((*pretvar)->strVal), (char*)player[index].name, MAX_STR_VAR_LEN);
		break;
	}
	case _gpp_score:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)player[index].score;
		break;
	}
	case _gpp_lives:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)player[index].lives;
		break;
	}
	case _gpp_playkeys:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)player[index].playkeys;
		break;
	}
	case _gpp_keys:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)player[index].keys;
		break;
	}
	case _gpp_credits:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		if(noshare) (*pretvar)->lVal = (LONG)player[index].credits;
		else        (*pretvar)->lVal = credits;
		break;
	}
	case _gpp_weaponum:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)player[index].weapnum;
		break;
	}
	//this property is not known
	//default:{
	//  .....
	//}
	}
	return S_OK;
}

// ===== changeplayerproperty =====
enum changeplayerproperty_enum {
	_cpp_credits,
	_cpp_lives,
	_cpp_name,
	_cpp_playkeys,
	_cpp_score,
	_cpp_weapon,
	_cpp_the_end,
};

void mapstrings_changeplayerproperty(ScriptVariant** varlist, int paramCount)
{
	char* propname;
	int prop;

	static const char* proplist[] = {
		"credits",
		"lives",
		"name",
		"playkeys",
		"score",
		"weapon",
	};

	if(paramCount < 3) return;

	// property name
	MAPSTRINGS(varlist[1], proplist, _cpp_the_end,
		"Property name '%s' is not supported by function changeplayerproperty.\n");
}

//changeplayerproperty(index, propname, value[, value2, value3,...]);
HRESULT openbor_changeplayerproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG ltemp, ltemp2;
	int index;
	entity* ent = NULL;
	int prop = -1;
	char* tempstr = NULL;
	ScriptVariant* arg = NULL;

	if(paramCount < 3)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	mapstrings_changeplayerproperty(varlist, paramCount);
	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	(*pretvar)->lVal = (LONG)1;
	arg = varlist[0];
	if(FAILED(ScriptVariant_IntegerValue(arg, &ltemp)))
	{
		index = 0;
	} else index = (int)ltemp;

	if(!(ent = player[index].ent))  //this player is not selected, so just return
	{
		return S_OK; //return S_OK, to tell the engine it is not a FATAL error
	}

	if(varlist[1]->vt != VT_INTEGER)
	{
		if(varlist[1]->vt != VT_STR)
			printf("You must give a string value for player property name.\n");
		return E_FAIL;
	}
	prop = varlist[1]->lVal;

	arg = varlist[2];

	//change the model
	switch(prop)
	{
	case _cpp_weapon:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg,&ltemp))){
			if(paramCount > 3)
			{
				arg = varlist[3];
				if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp2)))
					(*pretvar)->lVal = (LONG)1;
			}
			else
			{
				ltemp2 = (LONG)0;
				(*pretvar)->lVal = (LONG)1;
			}
			set_weapon(player[index].ent, (int)ltemp, (int)ltemp2);
			(*pretvar)->lVal = (LONG)1;
		}
		else (*pretvar)->lVal = (LONG)0;
		break;
	}
	case _cpp_name:
	{
		if(arg->vt != VT_STR)
		{
			//printf();
			return E_FAIL;
		}
		tempstr = (char*)StrCache_Get(arg->strVal);
		strcpy(player[index].name, tempstr);
		(*pretvar)->lVal = (LONG)1;
		break;
	}
	case _cpp_score:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg,&ltemp)))
		{
			if(ltemp < 0) ltemp = 0;
			else if(ltemp > 999999999) ltemp = 999999999;
			player[index].score = (unsigned int)ltemp;
		}
		else (*pretvar)->lVal = (LONG)0;
		break;
	}
	case _cpp_lives:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg,&ltemp)))
			player[index].lives = (int)ltemp;
		else (*pretvar)->lVal = (LONG)0;
		break;
	}
	case _cpp_playkeys:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg,&ltemp)))
			player[index].playkeys = (int)ltemp;
		else (*pretvar)->lVal = (LONG)0;
		break;
	}
	case _cpp_credits:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg,&ltemp)))
		{
		   	if(noshare) player[index].credits = (int)ltemp;
			else        credits = (int)ltemp;
		}
		else (*pretvar)->lVal = (LONG)0;
		break;
	}
	//this property is not known, so return 0
	//default:
	//    (*pretvar)->lVal = (LONG)0;
	}

	return S_OK;
}

//checkhole(x,z), return 1 if there's hole here
HRESULT openbor_checkhole(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	ScriptVariant* arg = NULL;
	DOUBLE x, z;

	if(paramCount < 2)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	(*pretvar)->lVal = (LONG)0;

	arg = varlist[0];
	if(FAILED(ScriptVariant_DecimalValue(arg, &x)))
		return S_OK;

	arg = varlist[1];
	if(FAILED(ScriptVariant_DecimalValue(arg, &z)))
		return S_OK;

	(*pretvar)->lVal = (LONG)(checkhole((float)x, (float)z) && checkwall((float)x, (float)z)<0);
	return S_OK;
}

//checkwall(x,z), return wall height, or 0
HRESULT openbor_checkwall(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	ScriptVariant* arg = NULL;
	DOUBLE x, z;
	int wall;

	if(paramCount < 2)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
	(*pretvar)->dblVal = (DOUBLE)0;

	arg = varlist[0];
	if(FAILED(ScriptVariant_DecimalValue(arg, &x)))
		return S_OK;

	arg = varlist[1];
	if(FAILED(ScriptVariant_DecimalValue(arg, &z)))
		return S_OK;

	if((wall=checkwall_below((float)x, (float)z, 100000))>=0)
	{
		(*pretvar)->dblVal = (DOUBLE)level->walls[wall][7];
	}
	return S_OK;
}

//checkplatformbelow(x,z,a), return the highest platfrom entity below
HRESULT openbor_checkplatformbelow(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	ScriptVariant* arg = NULL;
	DOUBLE x, z, a;

	if(paramCount < 3)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
	(*pretvar)->dblVal = (DOUBLE)0;

	arg = varlist[0];
	if(FAILED(ScriptVariant_DecimalValue(arg, &x)))
		return S_OK;

	arg = varlist[1];
	if(FAILED(ScriptVariant_DecimalValue(arg, &z)))
		return S_OK;

	arg = varlist[2];
	if(FAILED(ScriptVariant_DecimalValue(arg, &a)))
		return S_OK;

	ScriptVariant_ChangeType(*pretvar, VT_PTR);
	(*pretvar)->ptrVal = (VOID*)check_platform_below((float)x,(float)z,(float)a);
	return S_OK;
}

HRESULT openbor_openfilestream(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	char* filename = NULL;
	ScriptVariant* arg = NULL;
	LONG location = 0;

	int disCcWarns;
	FILE *handle = NULL;
	char path[128] = {""};
	long size;

	if(paramCount < 1)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	ScriptVariant_Clear(*pretvar);

	arg = varlist[0];
	if(arg->vt!=VT_STR)
	{
		printf("Filename for openfilestream must be a string.\n");
		*pretvar = NULL;
		return E_FAIL;
	}

	filename = (char*)StrCache_Get(arg->strVal);

	if(paramCount > 1)
	{
		arg = varlist[1];
		if(FAILED(ScriptVariant_IntegerValue(arg, &location)))
			return S_OK;
	}

	if(!level->numfilestreams) level->numfilestreams = 0;
	else if(level->numfilestreams == LEVEL_MAX_FILESTREAMS)
	{
		printf("Maximum file streams exceeded.\n");
		*pretvar = NULL;
		return E_FAIL;
	}

	// Load file from saves directory if specified
	if(location)
	{
		getBasePath(path, "Saves", 0);
		strcat(path, "/");
		strcat(path, filename);
#ifndef DC
		if(!(fileExists(path)))
		{
		    /*
		    2011_03_27, DC: Let's be a little more friendly about missing files; this will let a function evaluate if file exists and decide what to do.

			printf("Openfilestream - file specified does not exist.\n"); //Keep this for possible debug mode in the future.
			*/

			ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = -1;

			return S_OK;
		}
#endif
		handle = fopen(path, "rb");
		if(handle == NULL) return E_FAIL;
		fseek(handle, 0, SEEK_END);
		size = ftell(handle);
		rewind(handle);
		level->filestreams[level->numfilestreams].buf = (char*)malloc(sizeof(char)*size);
		if(level->filestreams[level->numfilestreams].buf == NULL) return E_FAIL;
		disCcWarns = fread(level->filestreams[level->numfilestreams].buf, 1, size, handle);
	}
	else if(buffer_pakfile(filename, &level->filestreams[level->numfilestreams].buf, &level->filestreams[level->numfilestreams].size)!=1)
	{
		  printf("Invalid filename used in openfilestream.\n");
		  *pretvar = NULL;
		  return E_FAIL;
	}

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	(*pretvar)->lVal = (LONG)level->numfilestreams;

	level->filestreams[level->numfilestreams].pos = 0;
	level->numfilestreams++;
	return S_OK;
}

HRESULT openbor_getfilestreamline(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	char line[MAX_STR_VAR_LEN];
	int length;
	ScriptVariant* arg = NULL;
	LONG filestreamindex;

	if(paramCount < 1)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	arg = varlist[0];
	if(FAILED(ScriptVariant_IntegerValue(arg, &filestreamindex)))
		return S_OK;

	ScriptVariant_Clear(*pretvar);
	ScriptVariant_ChangeType(*pretvar, VT_STR);

	length = 0;
	strncpy(line, "it's initialized now", MAX_STR_VAR_LEN);

	while(level->filestreams[filestreamindex].buf[level->filestreams[filestreamindex].pos+length] && level->filestreams[filestreamindex].buf[level->filestreams[filestreamindex].pos+length]!='\n' && level->filestreams[filestreamindex].buf[level->filestreams[filestreamindex].pos+length]!='\r') ++length;
	if(length >= MAX_STR_VAR_LEN)
		strncpy(StrCache_Get((*pretvar)->strVal), (char*)(level->filestreams[filestreamindex].buf+level->filestreams[filestreamindex].pos), MAX_STR_VAR_LEN);
	else
	{
		strncpy(line, (char*)(level->filestreams[filestreamindex].buf+level->filestreams[filestreamindex].pos), length);
		line[length] = '\0';
		strncpy(StrCache_Get((*pretvar)->strVal), line, MAX_STR_VAR_LEN);
	}
	return S_OK;
}

HRESULT openbor_getfilestreamargument(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	ScriptVariant* arg = NULL;
	LONG filestreamindex, argument;
	char* argtype = NULL;

	if(paramCount < 3)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	arg = varlist[0];
	if(FAILED(ScriptVariant_IntegerValue(arg, &filestreamindex)))
		return S_OK;

	arg = varlist[1];
	if(FAILED(ScriptVariant_IntegerValue(arg, &argument)))
		return S_OK;
	ScriptVariant_Clear(*pretvar);

	if(varlist[2]->vt != VT_STR)
	{
		printf("You must give a string value specifying what kind of value you want the argument converted to.\n");
		return E_FAIL;
	}
	argtype = (char*)StrCache_Get(varlist[2]->strVal);

	if(stricmp(argtype, "string")==0)
	{
		ScriptVariant_ChangeType(*pretvar, VT_STR);
		strncpy(StrCache_Get((*pretvar)->strVal), (char*)findarg(level->filestreams[filestreamindex].buf+level->filestreams[filestreamindex].pos, argument), MAX_STR_VAR_LEN);
	}
	else if(stricmp(argtype, "int")==0)
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)atoi(findarg(level->filestreams[filestreamindex].buf+level->filestreams[filestreamindex].pos, argument));
	}
	else if(stricmp(argtype, "float")==0)
	{
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)atof(findarg(level->filestreams[filestreamindex].buf+level->filestreams[filestreamindex].pos, argument));
	}
	else
	{
		printf("Invalid type for argument converted to (getfilestreamargument).\n");
		return E_FAIL;
	}

	return S_OK;
}

HRESULT openbor_filestreamnextline(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	ScriptVariant* arg = NULL;
	LONG filestreamindex;

	if(paramCount < 1)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	arg = varlist[0];
	if(FAILED(ScriptVariant_IntegerValue(arg, &filestreamindex)))
		return S_OK;
	while(level->filestreams[filestreamindex].buf[level->filestreams[filestreamindex].pos] && level->filestreams[filestreamindex].buf[level->filestreams[filestreamindex].pos]!='\n' && level->filestreams[filestreamindex].buf[level->filestreams[filestreamindex].pos]!='\r') ++level->filestreams[filestreamindex].pos;
	while(level->filestreams[filestreamindex].buf[level->filestreams[filestreamindex].pos]=='\n' || level->filestreams[filestreamindex].buf[level->filestreams[filestreamindex].pos]=='\r') ++level->filestreams[filestreamindex].pos;

	return S_OK;
}

HRESULT openbor_getfilestreamposition(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	ScriptVariant* arg = NULL;
	LONG filestreamindex;

	if(paramCount < 1)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	arg = varlist[0];
	if(FAILED(ScriptVariant_IntegerValue(arg, &filestreamindex)))
		return S_OK;

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	(*pretvar)->lVal = (LONG)level->filestreams[filestreamindex].pos;
	return S_OK;
}

HRESULT openbor_setfilestreamposition(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	ScriptVariant* arg = NULL;
	LONG filestreamindex, position;


	if(paramCount < 2)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	arg = varlist[0];
	if(FAILED(ScriptVariant_IntegerValue(arg, &filestreamindex)))
		return S_OK;

	arg = varlist[1];
	if(FAILED(ScriptVariant_IntegerValue(arg, &position)))
		return S_OK;

	level->filestreams[filestreamindex].pos = position;
	return S_OK;
}

HRESULT openbor_filestreamappend(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG filestreamindex;
	ScriptVariant* arg = NULL;
	LONG appendtype;
	LONG ltemp;
	DOUBLE dbltemp;
	char* temp;
	char append[MAX_STR_VAR_LEN];


	if(paramCount < 3)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	ScriptVariant_Clear(*pretvar);

	arg = varlist[0];
	if(FAILED(ScriptVariant_IntegerValue(arg, &filestreamindex)))
		return S_OK;

	arg = varlist[1];
	if(arg->vt==VT_STR)
	{
		strcpy(append, StrCache_Get(arg->strVal));
	}
	else if(arg->vt==VT_INTEGER)
	{
		if(FAILED(ScriptVariant_IntegerValue(arg, &ltemp)))
			return S_OK;
		sprintf(append, "%d", (int)ltemp);
	}
	else if(arg->vt==VT_DECIMAL)
	{
		if(FAILED(ScriptVariant_DecimalValue(arg, &dbltemp)))
			return S_OK;
		sprintf(append, "%f", dbltemp);
	}
	else
	{
		printf("Filename for filestreamappend must be a string.\n");
		*pretvar = NULL;
		return E_FAIL;
	}



	arg = varlist[2];
	if(FAILED(ScriptVariant_IntegerValue(arg, &appendtype)))
		return S_OK;

	temp = (char*)malloc(sizeof(char)*(strlen(level->filestreams[filestreamindex].buf) + strlen(append) + 4));
	strcpy(temp, level->filestreams[filestreamindex].buf);

	if(appendtype == 0)
	{
		strcat(temp, "\r\n");
		strcat(temp, append);
		temp[strlen(level->filestreams[filestreamindex].buf) + strlen(append) + 2] = ' ';
		temp[strlen(level->filestreams[filestreamindex].buf) + strlen(append) + 3] = '\0';
	}
	else if(appendtype == 1)
	{
		strcat(temp, append);
		temp[strlen(level->filestreams[filestreamindex].buf) + strlen(append)] = ' ';
		temp[strlen(level->filestreams[filestreamindex].buf) + strlen(append) + 1] = '\0';
	}
	free(level->filestreams[filestreamindex].buf);
	level->filestreams[filestreamindex].buf = temp;

	return S_OK;
}

HRESULT openbor_createfilestream(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	ScriptVariant_Clear(*pretvar);

	if(!level->numfilestreams) level->numfilestreams = 0;
	else if(level->numfilestreams == LEVEL_MAX_FILESTREAMS)
	{
		printf("Maximum file streams exceeded.\n");
		*pretvar = NULL;
		return E_FAIL;
	}

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	(*pretvar)->lVal = (LONG)level->numfilestreams;

	// Initialize the new filestream
	level->filestreams[level->numfilestreams].pos = 0;
	level->filestreams[level->numfilestreams].buf = (char*)malloc(sizeof(char)*128);
	level->filestreams[level->numfilestreams].buf[0] = '\0';
	level->numfilestreams++;
	return S_OK;
}

HRESULT openbor_savefilestream(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i, x, y;
	LONG filestreamindex;
	ScriptVariant* arg = NULL;
	int disCcWarns;
	FILE *handle = NULL;
	char path[128] = {""};
	char tmpname[128] = {""};
	char mod[256] = {""};

	if(paramCount < 1)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	ScriptVariant_Clear(*pretvar);

	arg = varlist[0];
	if(FAILED(ScriptVariant_IntegerValue(arg, &filestreamindex)))
		return S_OK;

	arg = varlist[1];
	if(arg->vt!=VT_STR)
	{
		printf("Filename for savefilestream must be a string.\n");
		*pretvar = NULL;
		return E_FAIL;
	}

	// Get the saves directory
	getBasePath(path, "Saves/", 0);

	// get the packfile's name by chopping off the rest of the path
	strncpy(mod,packfile,strlen(packfile)-4);
	x=0;
	for(i=0; i<(int)strlen(mod); i++){
		if((mod[i] == '/') || (mod[i] == '\\')) x = i;
	}
	y=0;
	for(i=0; i<(int)strlen(mod); i++){
		// For packfiles without '/'
		if(x == 0){
			tmpname[y] = mod[i];
			y++;
		}
		// For packfiles with '/'
		if(x != 0 && i > x){
			tmpname[y] = mod[i];
			y++;
		}
	}

	// Make ./Saves/PAKNAME if it doesn't exist
	strcat(path,tmpname);
#ifndef DC
	dirExists(path, 1);
#endif

	// Add user's filename to path and write the filestream to it
	strcat(path, "/");
	strcat(path, (char*)StrCache_Get(arg->strVal));
	handle = fopen(path, "wb");
	if(handle==NULL) return E_FAIL;
	disCcWarns = fwrite(level->filestreams[filestreamindex].buf, 1, strlen(level->filestreams[filestreamindex].buf), handle);

	// add blank line so it can be read successfully
	fwrite("\r\n", 1, 2, handle);
	fclose(handle);

	return S_OK;
}

//damageentity(entity, other, force, drop, type)
HRESULT openbor_damageentity(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	entity* ent = NULL;
	entity* other = NULL;
	entity* temp = NULL;
	LONG force, drop, type;
	s_attack attack;
	extern s_attack emptyattack;

	if(paramCount < 1)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	(*pretvar)->lVal = (LONG)0;

	ent = (entity*)(varlist[0])->ptrVal; //retrieve the entity
	if(!ent)  return S_OK;

	other = ent;
	force = (LONG)1;
	drop = (LONG)0;
	type = (LONG)ATK_NORMAL;

	if(paramCount >= 2)
	{
		other = (entity*)(varlist[1])->ptrVal;
		if(!other)
			return S_OK;
	}
	if(paramCount >= 3)
	{
		if(FAILED(ScriptVariant_IntegerValue((varlist[2]), &force)))
			return S_OK;
	}

	if(!ent->takedamage)
	{
		ent->health -= force;
		if(ent->health <= 0) kill(ent);
		(*pretvar)->lVal = (LONG)1;
		return S_OK;
	}

	if(paramCount >= 4)
	{
		if(FAILED(ScriptVariant_IntegerValue((varlist[3]), &drop)))
			return S_OK;
	}
	if(paramCount >= 5)
	{
		if(FAILED(ScriptVariant_IntegerValue((varlist[4]), &type)))
			return S_OK;
	}

	temp = self; self = ent;
	attack = emptyattack;
	attack.attack_force = force;
	attack.attack_drop = drop;
	if(drop) {attack.dropv[0] = (float)3; attack.dropv[1] = (float)1.2; attack.dropv[2] = (float)0;}
	attack.attack_type = type;
	self->takedamage(other, &attack);
	self = temp;
	return S_OK;
}

//killentity(entity)
HRESULT openbor_killentity(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	entity* ent = NULL;

	if(paramCount < 1)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

	ent = (entity*)(varlist[0])->ptrVal; //retrieve the entity
	if(!ent)
	{
		(*pretvar)->lVal = (LONG)0;
		return S_OK;
	}
	kill(ent);
	(*pretvar)->lVal = (LONG)1;
	return S_OK;
}

//findtarget(entity, int animation);
HRESULT openbor_findtarget(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i = 0;
	entity* ent = NULL;
	entity* tempself, *target;
	LONG anim = -1;

    if(paramCount>2) ScriptVariant_IntegerValue(varlist[2], &i);

	if(paramCount < 1)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	ScriptVariant_ChangeType(*pretvar, VT_PTR);

	ent = (entity*)(varlist[0])->ptrVal; //retrieve the entity
	if(!ent)
	{
		ScriptVariant_Clear(*pretvar);
		return S_OK;
	}
	if(paramCount>1 && FAILED(ScriptVariant_IntegerValue(varlist[1], &anim))) return E_FAIL;
	tempself = self;
	self = ent;
	target = normal_find_target((int)anim, i);
	if(!target) ScriptVariant_Clear(*pretvar);
	else (*pretvar)->ptrVal = (VOID*)target;
	self = tempself;
	return S_OK;
}

//checkrange(entity, target, int ani);
HRESULT openbor_checkrange(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	entity* ent = NULL, *target = NULL;
	LONG ani = 0;
	extern int max_animations;

	if(paramCount < 2) goto checkrange_error;

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

	if(varlist[0]->vt !=VT_PTR || varlist[1]->vt!=VT_PTR) goto checkrange_error;

	ent = (entity*)(varlist[0])->ptrVal; //retrieve the entity
	target = (entity*)(varlist[1])->ptrVal; //retrieve the target

	if(!ent || !target) goto checkrange_error;

	if(paramCount >2 && FAILED(ScriptVariant_IntegerValue(varlist[2], &ani))) goto checkrange_error;
	else if(paramCount<=2) ani = ent->animnum;

	if(ani<0 || ani>=max_animations)
	{
		printf("Animation id out of range: %d / %d.\n", (int)ani, max_animations);
		goto checkrange_error;
	}

	(*pretvar)->lVal = check_range(ent, target, ani);

	return S_OK;

checkrange_error:
	printf("Function needs at least 2 valid entity handles, the third parameter is optional: checkrange(entity, target, int animnum)\n");
	*pretvar = NULL;
	return E_FAIL;
}

//clearspawnentry();
HRESULT openbor_clearspawnentry(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	*pretvar = NULL;
	memset(&spawnentry, 0, sizeof(s_spawn_entry));
	spawnentry.index = spawnentry.itemindex = spawnentry.weaponindex = -1;
	return S_OK;
}

// ===== setspawnentry =====
enum setspawnentry_enum
{
	_sse_2phealth,
	_sse_2pitem,
	_sse_3phealth,
	_sse_3pitem,
	_sse_4phealth,
	_sse_4pitem,
	_sse_aggression,
	_sse_alias,
	_sse_alpha,
	_sse_boss,
	_sse_coords,
	_sse_credit,
	_sse_dying,
	_sse_flip,
	_sse_health,
	_sse_item,
	_sse_itemalias,
	_sse_itemhealth,
	_sse_itemmap,
	_sse_map,
	_sse_mp,
	_sse_multiple,
	_sse_name,
	_sse_nolife,
	_sse_weapon,
	_sse_the_end,
};

void mapstrings_setspawnentry(ScriptVariant** varlist, int paramCount)
{
	char* propname;
	int prop;
	static const char* proplist[] =
	{
		"2phealth",
		"2pitem",
		"3phealth",
		"3pitem",
		"4phealth",
		"4pitem",
		"aggression",
		"alias",
		"alpha",
		"boss",
		"coords",
		"credit",
		"dying",
		"flip",
		"health",
		"item",
		"itemalias",
		"itemhealth",
		"itemmap",
		"map",
		"mp",
		"multiple",
		"name",
		"nolife",
		"weapon",
	};

	MAPSTRINGS(varlist[0], proplist, _sse_the_end,
		"Property name '%s' is not supported by setspawnentry.\n");
}

//setspawnentry(propname, value1[, value2, value3, ...]);
HRESULT openbor_setspawnentry(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount)
{
	LONG ltemp;
	s_model* tempmodel;
	DOUBLE dbltemp;
	int temp, prop;
	ScriptVariant* arg = NULL;

	if(paramCount < 2)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	(*pretvar)->lVal = (LONG)1;

	mapstrings_setspawnentry(varlist, paramCount);
	if(varlist[0]->vt != VT_INTEGER)
	{
		if(varlist[0]->vt != VT_STR)
			printf("You must give a string value for spawn entry property name.\n");
		*pretvar = NULL;
		return E_FAIL;
	}

	prop = varlist[0]->lVal;

	arg = varlist[1];

	switch(prop)
	{
	case _sse_name:
		if(arg->vt != VT_STR)
		{
			printf("You must use a string value for spawn entry's name property: function setspawnentry.\n");
			goto setspawnentry_error;
		}
		spawnentry.model = findmodel((char*)StrCache_Get(arg->strVal));
		break;
	case _sse_alias:
		if(arg->vt != VT_STR) goto setspawnentry_error;
		strcpy(spawnentry.alias, (char*)StrCache_Get(arg->strVal));
		break;
	case _sse_item:
		if(arg->vt != VT_STR) goto setspawnentry_error;
		spawnentry.itemmodel = findmodel((char*)StrCache_Get(arg->strVal));
		spawnentry.item = spawnentry.itemmodel->name;
		spawnentry.itemindex = get_cached_model_index(spawnentry.item);
		spawnentry.itemplayer_count = 0;
		break;
	case _sse_2pitem:
		if(arg->vt != VT_STR) goto setspawnentry_error;
		tempmodel = findmodel((char*)StrCache_Get(arg->strVal));
		if(!tempmodel) spawnentry.item = NULL;
		else spawnentry.item = tempmodel->name;
		spawnentry.itemplayer_count = 1;
		break;
	case _sse_3pitem:
		if(arg->vt != VT_STR) goto setspawnentry_error;
		spawnentry.itemmodel = findmodel((char*)StrCache_Get(arg->strVal));
		spawnentry.itemplayer_count = 2;
		break;
	case _sse_4pitem:
		if(arg->vt != VT_STR) goto setspawnentry_error;
		spawnentry.itemmodel = findmodel((char*)StrCache_Get(arg->strVal));
		spawnentry.itemplayer_count = 3;
		break;
	case _sse_health:
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
			spawnentry.health[0] = (int)ltemp;
		else (*pretvar)->lVal = (LONG)0;
		break;
	case _sse_itemhealth:
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
			spawnentry.itemhealth = (int)ltemp;
		else (*pretvar)->lVal = (LONG)0;
		break;
	case _sse_itemalias:
		if(arg->vt != VT_STR) return E_FAIL;
		strcpy(spawnentry.itemalias, (char*)StrCache_Get(arg->strVal));
		break;
	case _sse_2phealth:
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
			spawnentry.health[1] = (int)ltemp;
		else (*pretvar)->lVal = (LONG)0;
		break;
	case _sse_3phealth:
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
			spawnentry.health[2] = (int)ltemp;
		else (*pretvar)->lVal = (LONG)0;
		break;
	case _sse_4phealth:
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
			spawnentry.health[3] = (int)ltemp;
		else (*pretvar)->lVal = (LONG)0;
		break;
	case _sse_coords:
		temp = 1;
		if(SUCCEEDED(ScriptVariant_DecimalValue(arg, &dbltemp))) spawnentry.x = (float)dbltemp;
		else temp = 0;
		if(paramCount >= 3 && temp)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp))) spawnentry.z = (float)dbltemp;
			else temp = 0;
		}
		if(paramCount >= 4 && temp)
		{
			if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp))) spawnentry.a = (float)dbltemp;
			else temp = 0;
		}
		(*pretvar)->lVal = (LONG)temp;
		break;
	case _sse_mp:
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
			spawnentry.mp = (int)ltemp;
		else (*pretvar)->lVal = (LONG)0;
		break;
	case _sse_map:
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
			spawnentry.colourmap = (int)ltemp;
		else (*pretvar)->lVal = (LONG)0;
		break;
	case _sse_itemmap:
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
			spawnentry.itemmap = (int)ltemp;
		else (*pretvar)->lVal = (LONG)0;
		break;
	case _sse_alpha:
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
			spawnentry.alpha = (int)ltemp;
		else (*pretvar)->lVal = (LONG)0;
		break;
	case _sse_multiple:
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
			spawnentry.multiple = (int)ltemp;
		else (*pretvar)->lVal = (LONG)0;
		break;
	case _sse_dying:
		temp = 1;
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
			spawnentry.dying = (int)ltemp;
		else temp = 0;
		if(paramCount >= 3 && temp)
		{
			if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
				spawnentry.per1 = (int)ltemp;
			else temp = 0;
		}
		if(paramCount >= 4 && temp)
		{
			if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
				spawnentry.per2 = (int)ltemp;
			else temp = 0;
		}
		(*pretvar)->lVal = (LONG)temp;
		break;
	case _sse_nolife:
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
			spawnentry.nolife = (int)ltemp;
		else (*pretvar)->lVal = (LONG)0;
		break;
	case _sse_boss:
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
			spawnentry.boss = (int)ltemp;
		else (*pretvar)->lVal = (LONG)0;
		break;
	case _sse_flip:
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
			spawnentry.flip = (int)ltemp;
		else (*pretvar)->lVal = (LONG)0;
		break;
	case _sse_credit:
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
			spawnentry.credit = (int)ltemp;
		else (*pretvar)->lVal = (LONG)0;
		break;
	case _sse_aggression:
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
			spawnentry.aggression = (int)ltemp;
		else (*pretvar)->lVal = (LONG)0;
		break;
	case _sse_weapon:
		if(arg->vt != VT_STR) goto setspawnentry_error;
		spawnentry.weaponmodel = findmodel((char*)StrCache_Get(arg->strVal));
		break;
	default:
		//printf("Property name '%s' is not supported by setspawnentry.\n", propname);
		goto setspawnentry_error;
	}

	return S_OK;
setspawnentry_error:
	*pretvar = NULL;
	return E_FAIL;
}

//spawn();
HRESULT openbor_spawn(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	entity* ent;

	ent = smartspawn(&spawnentry);

	if(ent)
	{
		ScriptVariant_ChangeType(*pretvar, VT_PTR);
		(*pretvar)->ptrVal = (VOID*) ent;
	}
	else     ScriptVariant_Clear(*pretvar);

	return S_OK;
}

//entity * projectile(char *name, float x, float z, float a, int direction, int type, int ptype, int map);
HRESULT openbor_projectile(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
	LONG value[7];
	entity* ent;
	char name[64] = {""};

	if(paramCount != 8) goto projectile_error;
	if(varlist[0]->vt != VT_STR) goto projectile_error;

	ScriptVariant_Clear(*pretvar);

	strncpy(name, StrCache_Get(varlist[0]->strVal), MAX_STR_VAR_LEN);

	for(i=1; i<=7; i++)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[i], &value[i-1])))
		    goto projectile_error;
	}

	switch((int)value[5])
	{
		default:
		case 0:
			ent = knife_spawn(name, -1, (float)value[0], (float)value[1], (float)value[2], (int)value[3], (int)value[4], (int)value[6]);
			break;
		case 1:
			ent = bomb_spawn(name, -1, (float)value[0], (float)value[1], (float)value[2], (int)value[3], (int)value[6]);
			break;
	}

	ScriptVariant_ChangeType(*pretvar, VT_PTR);
	(*pretvar)->ptrVal = (VOID*) ent;

	return S_OK;

projectile_error:
	printf("Function requires 8 values: entity * projectile(char *name, float x, float z, float a, int direction, int type, int ptype, int map)\n");
	return E_FAIL;
}


// ===== openborconstant =====
#define ICMPCONST(x) \
if(stricmp(#x, constname)==0) {\
	ScriptVariant_ChangeType(varlist[0], VT_INTEGER);\
	varlist[0]->lVal = (LONG)x;\
	return;\
}
void mapstrings_transconst(ScriptVariant** varlist, int paramCount)
{
	char* constname = NULL;

	if(paramCount < 1) return;

	if(varlist[0]->vt == VT_STR)
	{
		constname = (char*)StrCache_Get(varlist[0]->strVal);

		ICMPCONST(COMPATIBLEVERSION)
		ICMPCONST(PIXEL_8)
		ICMPCONST(PIXEL_x8)
		ICMPCONST(PIXEL_16)
		ICMPCONST(PIXEL_32)
		ICMPCONST(CV_SAVED_GAME)
		ICMPCONST(CV_HIGH_SCORE)
		ICMPCONST(THINK_SPEED)
		ICMPCONST(COUNTER_SPEED)
		ICMPCONST(MAX_ENTS)
		ICMPCONST(MAX_PANELS)
		ICMPCONST(MAX_WEAPONS)
		ICMPCONST(MAX_COLOUR_MAPS)
		ICMPCONST(MAX_NAME_LEN)
		ICMPCONST(LEVEL_MAX_SPAWNS)
		ICMPCONST(LEVEL_MAX_PANELS)
		ICMPCONST(LEVEL_MAX_HOLES)
		ICMPCONST(LEVEL_MAX_WALLS)
		ICMPCONST(MAX_LEVELS)
		ICMPCONST(MAX_DIFFICULTIES)
		ICMPCONST(MAX_SPECIALS)
		ICMPCONST(MAX_ATCHAIN)
		ICMPCONST(MAX_ATTACKS)
		ICMPCONST(MAX_FOLLOWS)
		ICMPCONST(MAX_PLAYERS)
		ICMPCONST(MAX_ARG_LEN)
		ICMPCONST(FLAG_ESC)
		ICMPCONST(FLAG_START)
		ICMPCONST(FLAG_MOVELEFT)
		ICMPCONST(FLAG_MOVERIGHT)
		ICMPCONST(FLAG_MOVEUP)
		ICMPCONST(FLAG_MOVEDOWN)
		ICMPCONST(FLAG_ATTACK)
		ICMPCONST(FLAG_ATTACK2)
		ICMPCONST(FLAG_ATTACK3)
		ICMPCONST(FLAG_ATTACK4)
		ICMPCONST(FLAG_JUMP)
		ICMPCONST(FLAG_SPECIAL)
		ICMPCONST(FLAG_SCREENSHOT)
		ICMPCONST(FLAG_ANYBUTTON)
		ICMPCONST(FLAG_FORWARD)
		ICMPCONST(FLAG_BACKWARD)
		ICMPCONST(SDID_MOVEUP)
		ICMPCONST(SDID_MOVEDOWN)
		ICMPCONST(SDID_MOVELEFT)
		ICMPCONST(SDID_MOVERIGHT)
		ICMPCONST(SDID_SPECIAL)
		ICMPCONST(SDID_ATTACK)
		ICMPCONST(SDID_ATTACK2)
		ICMPCONST(SDID_ATTACK3)
		ICMPCONST(SDID_ATTACK4)
		ICMPCONST(SDID_JUMP)
		ICMPCONST(SDID_START)
		ICMPCONST(SDID_SCREENSHOT)
		ICMPCONST(TYPE_NONE)
		ICMPCONST(TYPE_PLAYER)
		ICMPCONST(TYPE_ENEMY)
		ICMPCONST(TYPE_ITEM)
		ICMPCONST(TYPE_OBSTACLE)
		ICMPCONST(TYPE_STEAMER)
		ICMPCONST(TYPE_SHOT)
		ICMPCONST(TYPE_TRAP)
		ICMPCONST(TYPE_TEXTBOX)
		ICMPCONST(TYPE_ENDLEVEL)
		ICMPCONST(TYPE_NPC)
		ICMPCONST(TYPE_PANEL)
		ICMPCONST(SUBTYPE_NONE)
		ICMPCONST(SUBTYPE_BIKER)
		ICMPCONST(SUBTYPE_NOTGRAB)
		ICMPCONST(SUBTYPE_ARROW)
		ICMPCONST(SUBTYPE_TOUCH)
		ICMPCONST(SUBTYPE_WEAPON)
		ICMPCONST(SUBTYPE_NOSKIP)
		ICMPCONST(SUBTYPE_FLYDIE)
		ICMPCONST(SUBTYPE_BOTH)
		ICMPCONST(SUBTYPE_PROJECTILE)
		ICMPCONST(SUBTYPE_FOLLOW)
		ICMPCONST(SUBTYPE_CHASE)
		ICMPCONST(AIMOVE1_NORMAL)
		ICMPCONST(AIMOVE1_CHASE)
		ICMPCONST(AIMOVE1_CHASEZ)
		ICMPCONST(AIMOVE1_CHASEX)
		ICMPCONST(AIMOVE1_AVOID)
		ICMPCONST(AIMOVE1_AVOIDZ)
		ICMPCONST(AIMOVE1_AVOIDX)
		ICMPCONST(AIMOVE1_WANDER)
		ICMPCONST(AIMOVE1_NOMOVE)
		ICMPCONST(AIMOVE1_BIKER)
		ICMPCONST(AIMOVE1_STAR)
		ICMPCONST(AIMOVE1_ARROW)
		ICMPCONST(AIMOVE1_BOMB)
		ICMPCONST(AIMOVE2_NORMAL)
		ICMPCONST(AIMOVE2_IGNOREHOLES)
		ICMPCONST(AIATTACK1_NORMAL)
		ICMPCONST(AIATTACK1_LONG)
		ICMPCONST(AIATTACK1_MELEE)
		ICMPCONST(AIATTACK1_NOATTACK)
		ICMPCONST(AIATTACK2_NORMAL)
		ICMPCONST(AIATTACK2_DODGE)
		ICMPCONST(AIATTACK2_DODGEMOVE)
		ICMPCONST(FRONTPANEL_Z)
		ICMPCONST(HOLE_Z)
		ICMPCONST(NEONPANEL_Z)
		ICMPCONST(SHADOW_Z)
		ICMPCONST(SCREENPANEL_Z)
		ICMPCONST(PANEL_Z)
		ICMPCONST(MIRROR_Z)
		ICMPCONST(PIT_DEPTH)
		ICMPCONST(P2_STATS_DIST)
		ICMPCONST(CONTACT_DIST_H)
		ICMPCONST(CONTACT_DIST_V)
		ICMPCONST(GRAB_DIST)
		ICMPCONST(GRAB_STALL)
		ICMPCONST(ATK_NORMAL)
		ICMPCONST(ATK_NORMAL2)
		ICMPCONST(ATK_NORMAL3)
		ICMPCONST(ATK_NORMAL4)
		ICMPCONST(ATK_BLAST)
		ICMPCONST(ATK_BURN)
		ICMPCONST(ATK_FREEZE)
		ICMPCONST(ATK_SHOCK)
		ICMPCONST(ATK_STEAL)
		ICMPCONST(ATK_NORMAL5)
		ICMPCONST(ATK_NORMAL6)
		ICMPCONST(ATK_NORMAL7)
		ICMPCONST(ATK_NORMAL8)
		ICMPCONST(ATK_NORMAL9)
		ICMPCONST(ATK_NORMAL10)
		ICMPCONST(ATK_ITEM)
		ICMPCONST(SCROLL_RIGHT)
		ICMPCONST(SCROLL_DOWN)
		ICMPCONST(SCROLL_LEFT)
		ICMPCONST(SCROLL_UP)
		ICMPCONST(SCROLL_BOTH)
		ICMPCONST(SCROLL_LEFTRIGHT)
		ICMPCONST(SCROLL_RIGHTLEFT)
		ICMPCONST(SCROLL_INWARD)
		ICMPCONST(SCROLL_OUTWARD)
		ICMPCONST(SCROLL_INOUT)
		ICMPCONST(SCROLL_OUTIN)
		ICMPCONST(SCROLL_UPWARD)
		ICMPCONST(SCROLL_DOWNWARD)
		ICMPCONST(ANI_IDLE)
		ICMPCONST(ANI_WALK)
		ICMPCONST(ANI_JUMP)
		ICMPCONST(ANI_LAND)
		ICMPCONST(ANI_PAIN)
		ICMPCONST(ANI_FALL)
		ICMPCONST(ANI_RISE)
		//ICMPCONST(ANI_ATTACK1)// move these below because we have some dynamic animation ids
		//ICMPCONST(ANI_ATTACK2)
		//ICMPCONST(ANI_ATTACK3)
		//ICMPCONST(ANI_ATTACK4)
		ICMPCONST(ANI_UPPER)
		ICMPCONST(ANI_BLOCK)
		ICMPCONST(ANI_JUMPATTACK)
		ICMPCONST(ANI_JUMPATTACK2)
		ICMPCONST(ANI_GET)
		ICMPCONST(ANI_GRAB)
		ICMPCONST(ANI_GRABATTACK)
		ICMPCONST(ANI_GRABATTACK2)
		ICMPCONST(ANI_THROW)
		ICMPCONST(ANI_SPECIAL)
		//ICMPCONST(ANI_FREESPECIAL)// move these below because we have some dynamic animation ids
		ICMPCONST(ANI_SPAWN)
		ICMPCONST(ANI_DIE)
		ICMPCONST(ANI_PICK)
		//ICMPCONST(ANI_FREESPECIAL2)
		ICMPCONST(ANI_JUMPATTACK3)
		//ICMPCONST(ANI_FREESPECIAL3)
		ICMPCONST(ANI_UP)
		ICMPCONST(ANI_DOWN)
		ICMPCONST(ANI_SHOCK)
		ICMPCONST(ANI_BURN)

		ICMPCONST(ANI_SHOCKPAIN)
		ICMPCONST(ANI_BURNPAIN)
		ICMPCONST(ANI_GRABBED)
		ICMPCONST(ANI_SPECIAL2)
		ICMPCONST(ANI_RUN)
		ICMPCONST(ANI_RUNATTACK)
		ICMPCONST(ANI_RUNJUMPATTACK)
		ICMPCONST(ANI_ATTACKUP)
		ICMPCONST(ANI_ATTACKDOWN)
		ICMPCONST(ANI_ATTACKFORWARD)
		ICMPCONST(ANI_ATTACKBACKWARD)
		//ICMPCONST(ANI_FREESPECIAL4)
		//ICMPCONST(ANI_FREESPECIAL5)
		//ICMPCONST(ANI_FREESPECIAL6)
		//ICMPCONST(ANI_FREESPECIAL7)
		//ICMPCONST(ANI_FREESPECIAL8)
		ICMPCONST(ANI_RISEATTACK)
		ICMPCONST(ANI_DODGE)
		ICMPCONST(ANI_ATTACKBOTH)
		ICMPCONST(ANI_GRABFORWARD)
		ICMPCONST(ANI_GRABFORWARD2)
		ICMPCONST(ANI_JUMPFORWARD)
		ICMPCONST(ANI_GRABDOWN)
		ICMPCONST(ANI_GRABDOWN2)
		ICMPCONST(ANI_GRABUP)
		ICMPCONST(ANI_GRABUP2)
		ICMPCONST(ANI_SELECT)
		ICMPCONST(ANI_DUCK)
		ICMPCONST(ANI_FAINT)
		ICMPCONST(ANI_CANT)
		ICMPCONST(ANI_THROWATTACK)
		ICMPCONST(ANI_CHARGEATTACK)
		ICMPCONST(ANI_VAULT)
		ICMPCONST(ANI_JUMPCANT)
		ICMPCONST(ANI_JUMPSPECIAL)
		ICMPCONST(ANI_BURNDIE)
		ICMPCONST(ANI_SHOCKDIE)
		ICMPCONST(ANI_PAIN2)
		ICMPCONST(ANI_PAIN3)
		ICMPCONST(ANI_PAIN4)
		ICMPCONST(ANI_FALL2)
		ICMPCONST(ANI_FALL3)
		ICMPCONST(ANI_FALL4)
		ICMPCONST(ANI_DIE2)
		ICMPCONST(ANI_DIE3)
		ICMPCONST(ANI_DIE4)
		ICMPCONST(ANI_CHARGE)
		ICMPCONST(ANI_BACKWALK)
		ICMPCONST(ANI_SLEEP)
		//ICMPCONST(ANI_FOLLOW1) // move these below because we have some dynamic animation ids
		//ICMPCONST(ANI_FOLLOW2)
		//ICMPCONST(ANI_FOLLOW3)
		//ICMPCONST(ANI_FOLLOW4)
		ICMPCONST(ANI_PAIN5)
		ICMPCONST(ANI_PAIN6)
		ICMPCONST(ANI_PAIN7)
		ICMPCONST(ANI_PAIN8)
		ICMPCONST(ANI_PAIN9)
		ICMPCONST(ANI_PAIN10)
		ICMPCONST(ANI_FALL5)
		ICMPCONST(ANI_FALL6)
		ICMPCONST(ANI_FALL7)
		ICMPCONST(ANI_FALL8)
		ICMPCONST(ANI_FALL9)
		ICMPCONST(ANI_FALL10)
		ICMPCONST(ANI_DIE5)
		ICMPCONST(ANI_DIE6)
		ICMPCONST(ANI_DIE7)
		ICMPCONST(ANI_DIE8)
		ICMPCONST(ANI_DIE9)
		ICMPCONST(ANI_DIE10)
		ICMPCONST(ANI_TURN)
		ICMPCONST(ANI_RESPAWN)
		ICMPCONST(ANI_FORWARDJUMP)
		ICMPCONST(ANI_RUNJUMP)
		ICMPCONST(ANI_JUMPLAND)
		ICMPCONST(ANI_JUMPDELAY)
		ICMPCONST(ANI_HITWALL)
		ICMPCONST(ANI_GRABBACKWARD)
		ICMPCONST(ANI_GRABBACKWARD2)
		ICMPCONST(ANI_GRABWALK)
		ICMPCONST(ANI_GRABBEDWALK)
		ICMPCONST(ANI_GRABWALKUP)
		ICMPCONST(ANI_GRABBEDWALKUP)
		ICMPCONST(ANI_GRABWALKDOWN)
		ICMPCONST(ANI_GRABBEDWALKDOWN)
		ICMPCONST(ANI_GRABTURN)
		ICMPCONST(ANI_GRABBEDTURN)
		ICMPCONST(ANI_GRABBACKWALK)
		ICMPCONST(ANI_GRABBEDBACKWALK)

		ICMPCONST(ANI_SLIDE)
		ICMPCONST(ANI_RUNSLIDE)
		ICMPCONST(ANI_BLOCKPAIN)
		ICMPCONST(ANI_DUCKATTACK)
		ICMPCONST(MAX_ANIS)
		ICMPCONST(PLAYER_MIN_Z)
		ICMPCONST(PLAYER_MAX_Z)
		ICMPCONST(BGHEIGHT)
		ICMPCONST(MAX_WALL_HEIGHT)
		ICMPCONST(SAMPLE_GO);
		ICMPCONST(SAMPLE_BEAT);
		ICMPCONST(SAMPLE_BLOCK);
		ICMPCONST(SAMPLE_INDIRECT);
		ICMPCONST(SAMPLE_GET);
		ICMPCONST(SAMPLE_GET2);
		ICMPCONST(SAMPLE_FALL);
		ICMPCONST(SAMPLE_JUMP);
		ICMPCONST(SAMPLE_PUNCH);
		ICMPCONST(SAMPLE_1UP);
		ICMPCONST(SAMPLE_TIMEOVER);
		ICMPCONST(SAMPLE_BEEP);
		ICMPCONST(SAMPLE_BEEP2);
		ICMPCONST(SAMPLE_BIKE);
		ICMPCONST(ANI_RISE2);
		ICMPCONST(ANI_RISE3);
		ICMPCONST(ANI_RISE4);
		ICMPCONST(ANI_RISE5);
		ICMPCONST(ANI_RISE6);
		ICMPCONST(ANI_RISE7);
		ICMPCONST(ANI_RISE8);
		ICMPCONST(ANI_RISE9);
		ICMPCONST(ANI_RISE10);
		ICMPCONST(ANI_RISEB);
		ICMPCONST(ANI_RISES);
		ICMPCONST(ANI_BLOCKPAIN2);
		ICMPCONST(ANI_BLOCKPAIN3);
		ICMPCONST(ANI_BLOCKPAIN4);
		ICMPCONST(ANI_BLOCKPAIN5);
		ICMPCONST(ANI_BLOCKPAIN6);
		ICMPCONST(ANI_BLOCKPAIN7);
		ICMPCONST(ANI_BLOCKPAIN8);
		ICMPCONST(ANI_BLOCKPAIN9);
		ICMPCONST(ANI_BLOCKPAIN10);
		ICMPCONST(ANI_BLOCKPAINB);
		ICMPCONST(ANI_BLOCKPAINS);
		ICMPCONST(ANI_CHIPDEATH);
		ICMPCONST(ANI_GUARDBREAK);
		ICMPCONST(ANI_RISEATTACK2);
		ICMPCONST(ANI_RISEATTACK3);
		ICMPCONST(ANI_RISEATTACK4);
		ICMPCONST(ANI_RISEATTACK5);
		ICMPCONST(ANI_RISEATTACK6);
		ICMPCONST(ANI_RISEATTACK7);
		ICMPCONST(ANI_RISEATTACK8);
		ICMPCONST(ANI_RISEATTACK9);
		ICMPCONST(ANI_RISEATTACK10);
		ICMPCONST(ANI_RISEATTACKB);
		ICMPCONST(ANI_RISEATTACKS);
		ICMPCONST(ANI_SLIDE);
		ICMPCONST(ANI_RUNSLIDE);
		ICMPCONST(ANI_DUCKATTACK);
	}
}
//openborconstant(constname);
//translate a constant by string, used to retrieve a constant or macro of openbor
HRESULT openbor_transconst(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	char* constname = NULL;
	int temp;

	if(paramCount < 1)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	mapstrings_transconst(varlist, paramCount);
	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

	if(varlist[0]->vt == VT_INTEGER) // return value already determined by mapstrings
	{
		(*pretvar)->lVal = varlist[0]->lVal;
		return S_OK;
	}
	else if(varlist[0]->vt != VT_STR)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	// if we get to this point, it's a dynamic animation id
	constname = StrCache_Get(varlist[0]->strVal);

	// for the extra animation ids
	// for the extra animation ids
	if(strnicmp(constname, "ANI_DOWN", 8)==0 && constname[8] >= '1') // new down walk?
	{
		temp = atoi(constname+8);
		(*pretvar)->lVal = (LONG)(animdowns[temp-1]);
		return S_OK;
	}
	if(strnicmp(constname, "ANI_UP", 8)==0 && constname[8] >= '1') // new up walk?
	{
		temp = atoi(constname+8);
		(*pretvar)->lVal = (LONG)(animups[temp-1]);
		return S_OK;
	}
	if(strnicmp(constname, "ANI_BACKWALK", 8)==0 && constname[8] >= '1') // new backwalk?
	{
		temp = atoi(constname+8);
		(*pretvar)->lVal = (LONG)(animbackwalks[temp-1]);
		return S_OK;
	}
	if(strnicmp(constname, "ANI_WALK", 8)==0 && constname[8] >= '1') // new Walk?
	{
		temp = atoi(constname+8);
		(*pretvar)->lVal = (LONG)(animwalks[temp-1]);
		return S_OK;
	}
	if(strnicmp(constname, "ANI_IDLE", 8)==0 && constname[8] >= '1') // new idle?
	{
		temp = atoi(constname+8);
		(*pretvar)->lVal = (LONG)(animidles[temp-1]);
		return S_OK;
	}
	if(strnicmp(constname, "ANI_FALL", 8)==0 && constname[8] >= '1' && constname[8]<='9') // new fall?
	{
		temp = atoi(constname+8); // so must be greater than 10
		if(temp<MAX_ATKS-STA_ATKS+1) temp = MAX_ATKS-STA_ATKS+1; // just in case
		(*pretvar)->lVal = (LONG)(animfalls[temp+STA_ATKS-1]);
		return S_OK;
	}
	if(strnicmp(constname, "ANI_RISE", 8)==0 && constname[8] >= '1' && constname[8]<='9') // new fall?
	{
		temp = atoi(constname+8);
		if(temp<MAX_ATKS-STA_ATKS+1) temp = MAX_ATKS-STA_ATKS+1; // just in case
		(*pretvar)->lVal = (LONG)(animrises[temp+STA_ATKS-1]);
		return S_OK;
	}
	if(strnicmp(constname, "ANI_RISEATTACK", 14)==0 && constname[14] >= '1' && constname[14]<='9') // new fall?
	{
		temp = atoi(constname+14);
		if(temp<MAX_ATKS-STA_ATKS+1) temp = MAX_ATKS-STA_ATKS+1; // just in case
		(*pretvar)->lVal = (LONG)(animriseattacks[temp+STA_ATKS-1]);
		return S_OK;
	}
	if(strnicmp(constname, "ANI_PAIN", 8)==0 && constname[8] >= '1' && constname[8]<='9') // new fall?
	{
		temp = atoi(constname+8); // so must be greater than 10
		if(temp<MAX_ATKS-STA_ATKS+1) temp = MAX_ATKS-STA_ATKS+1; // just in case
		(*pretvar)->lVal = (LONG)(animpains[temp+STA_ATKS-1]);
		return S_OK;
	}
	if(strnicmp(constname, "ANI_DIE", 7)==0 && constname[7] >= '1' && constname[7]<='9') // new fall?
	{
		temp = atoi(constname+7); // so must be greater than 10
		if(temp<MAX_ATKS-STA_ATKS+1) temp = MAX_ATKS-STA_ATKS+1; // just in case
		(*pretvar)->lVal = (LONG)(animdies[temp+STA_ATKS-1]);
		return S_OK;
	}
	if(strnicmp(constname, "ANI_ATTACK", 10)==0 && constname[10] >= '1' && constname[10]<='9')
	{
		temp = atoi(constname+10);
		(*pretvar)->lVal = (LONG)(animattacks[temp-1]);
		return S_OK;
	}
	if(strnicmp(constname, "ANI_FOLLOW", 10)==0 && constname[10] >= '1' && constname[10]<='9')
	{
		temp = atoi(constname+10);
		(*pretvar)->lVal = (LONG)(animfollows[temp-1]);
		return S_OK;
	}
	if(strnicmp(constname, "ANI_FREESPECIAL", 15)==0 && (!constname[15] ||(constname[15] >= '1' && constname[15]<='9')))
	{
		temp = atoi(constname+15);
		if(temp<1) temp = 1;
		(*pretvar)->lVal = (LONG)(animspecials[temp-1]);
		return S_OK;
	}
	ScriptVariant_Clear(*pretvar);
	return S_OK;
}

//int rgbcolor(int r, int g, int b);
HRESULT openbor_rgbcolor(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG r, g, b;

	if(paramCount != 3) goto rgbcolor_error;
	if(FAILED(ScriptVariant_IntegerValue(varlist[0], &r))) goto rgbcolor_error; // decimal/integer value for red?
	if(FAILED(ScriptVariant_IntegerValue(varlist[1], &g))) goto rgbcolor_error; // decimal/integer value for green?
	if(FAILED(ScriptVariant_IntegerValue(varlist[2], &b))) goto rgbcolor_error; // decimal/integer value for blue?

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	(*pretvar)->lVal = _makecolour(r, g, b);
	return S_OK;

rgbcolor_error:
	*pretvar = NULL;
	return E_FAIL;
}

// ===== playerkeys =====
enum playerkeys_enum
{
	_pk_anybutton,
	_pk_attack,
	_pk_attack2,
	_pk_attack3,
	_pk_attack4,
	_pk_esc,
	_pk_jump,
	_pk_movedown,
	_pk_moveleft,
	_pk_moveright,
	_pk_moveup,
	_pk_screenshot,
	_pk_special,
	_pk_start,
	_pk_the_end,
};

void mapstrings_playerkeys(ScriptVariant** varlist, int paramCount)
{
	char* propname = NULL;
	int i, prop;

	static const char* proplist[] = // for args 2+
	{
		"anybutton",
		"attack",
		"attack2",
		"attack3",
		"attack4",
		"esc",
		"jump",
		"movedown",
		"moveleft",
		"moveright",
		"moveup",
		"screenshot",
		"special",
		"start",
	};

	for(i=2; i<paramCount; i++)
	{
		MAPSTRINGS(varlist[i], proplist, _pk_the_end,
			"Button name '%s' is not supported by playerkeys.");
	}
}

//playerkeys(playerindex, newkey?, key1, key2, ...);
HRESULT openbor_playerkeys(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG ltemp;
	int index, newkey;
	int i;
	u32 keys;
	ScriptVariant* arg = NULL;

	if(paramCount < 3)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	(*pretvar)->lVal = (LONG)1;

	mapstrings_playerkeys(varlist, paramCount);

	if(FAILED(ScriptVariant_IntegerValue((varlist[0]), &ltemp)))
	{
		index = 0;
	} else index = (int)ltemp;

	if(SUCCEEDED(ScriptVariant_IntegerValue((varlist[1]), &ltemp)))
		newkey = (int)ltemp;
	else newkey = 0;

	if(newkey == 1) keys = player[index].newkeys;
	else if(newkey == 2) keys = player[index].releasekeys;
	else keys = player[index].keys;

	for(i=2; i<paramCount; i++)
	{
		arg = varlist[i];
		if(arg->vt == VT_INTEGER)
		{
			switch(arg->lVal)
			{
		    case _pk_jump:
				(*pretvar)->lVal = (LONG)(keys & FLAG_JUMP);
				break;
			case _pk_attack:
				(*pretvar)->lVal = (LONG)(keys & FLAG_ATTACK);
				break;
			case _pk_attack2:
				(*pretvar)->lVal = (LONG)(keys & FLAG_ATTACK2);
				break;
			case _pk_attack3:
				(*pretvar)->lVal = (LONG)(keys & FLAG_ATTACK3);
				break;
			case _pk_attack4:
				(*pretvar)->lVal = (LONG)(keys & FLAG_ATTACK4);
				break;
			case _pk_special:
				(*pretvar)->lVal = (LONG)(keys & FLAG_SPECIAL);
				break;
			case _pk_esc:
				(*pretvar)->lVal = (LONG)(keys & FLAG_ESC);
				break;
			case _pk_start:
				(*pretvar)->lVal = (LONG)(keys & FLAG_START);
				break;
			case _pk_moveleft:
				(*pretvar)->lVal = (LONG)(keys & FLAG_MOVELEFT);
				break;
			case _pk_moveright:
				(*pretvar)->lVal = (LONG)(keys & FLAG_MOVERIGHT);
				break;
			case _pk_moveup:
				(*pretvar)->lVal = (LONG)(keys & FLAG_MOVEUP);
				break;
			case _pk_movedown:
				(*pretvar)->lVal = (LONG)(keys & FLAG_MOVEDOWN);
				break;
			case _pk_screenshot:
				(*pretvar)->lVal = (LONG)(keys & FLAG_SCREENSHOT);
				break;
			case _pk_anybutton:
				(*pretvar)->lVal = (LONG)(keys & FLAG_ANYBUTTON);
				break;
			default:
				(*pretvar)->lVal = (LONG)0;
			}
		}
		else (*pretvar)->lVal = (LONG)0;
		if(!((*pretvar)->lVal)) break;
	}

	return S_OK;
}

//playmusic(name, loop)
HRESULT openbor_playmusic(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int loop = 0;
	LONG offset = 0;
	char* thename = NULL;

	*pretvar = NULL;
	if(paramCount < 1)
	{
		sound_close_music();
		return S_OK;
	}
	if(varlist[0]->vt != VT_STR)
	{
		//printf("");
		return E_FAIL;
	}
	thename = StrCache_Get(varlist[0]->strVal);

	if(paramCount > 1)
	{
		loop = (int)ScriptVariant_IsTrue(varlist[1]);
	}

	if(paramCount > 2)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[2], &offset)))
			return E_FAIL;
	}


	music(thename, loop, offset);
	return S_OK;
}

//fademusic(fade, name, loop, offset)
HRESULT openbor_fademusic(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	DOUBLE value = 0;
	LONG values[2] = {0,0};
	*pretvar = NULL;
	if(paramCount < 1) goto fademusic_error;
	if(FAILED(ScriptVariant_DecimalValue(varlist[0], &value))) goto fademusic_error;
	musicfade[0] = value;
	musicfade[1] = (float)savedata.musicvol;

	if(paramCount == 4)
	{
		strncpy(musicname, StrCache_Get(varlist[1]->strVal), 128);
		if(FAILED(ScriptVariant_IntegerValue(varlist[2], &values[0]))) goto fademusic_error;
		if(FAILED(ScriptVariant_IntegerValue(varlist[3], &values[1]))) goto fademusic_error;
		musicloop = values[0];
		musicoffset = values[1];
	}
	return S_OK;

fademusic_error:
	printf("Function requires 1 value, with an optional 3 for music triggering: fademusic_error(float fade, char name, int loop, unsigned long offset)\n");
	return E_FAIL;
}

//setmusicvolume(left, right)
HRESULT openbor_setmusicvolume(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG channels[2];

	if(paramCount < 1)
	{
		return S_OK;
	}

	if(FAILED(ScriptVariant_IntegerValue(varlist[0], channels)))
			goto setmusicvolume_error;

	if(paramCount > 1)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[1], channels+1)))
			goto setmusicvolume_error;
	}
	else
		channels[1] = channels[0];

	sound_volume_music((int)channels[0], (int)channels[1]);
	return S_OK;

setmusicvolume_error:
	printf("values must be integers: setmusicvolume(int left, (optional)int right)\n");
	return E_FAIL;
}

//setmusicvolume(left, right)
HRESULT openbor_setmusictempo(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG new_tempo;

	if(paramCount < 1)
	{
		return S_OK;
	}

	if(FAILED(ScriptVariant_IntegerValue(varlist[0], &new_tempo)))
		return E_FAIL;

	sound_music_tempo(new_tempo);
	return S_OK;
}

//pausemusic(value)
HRESULT openbor_pausemusic(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int pause = 0;
	if(paramCount < 1)
	{
		return S_OK;
	}

	pause = (int)ScriptVariant_IsTrue(varlist[0]);

	sound_pause_music(pause);
	return S_OK;
}

//playsample(id, priority, lvolume, rvolume, speed, loop)
HRESULT openbor_playsample(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
	LONG value[6];

	if(paramCount != 6) goto playsample_error;

	*pretvar = NULL;
	for(i=0; i<6; i++)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i)))
			goto playsample_error;
	}
	if((int)value[0] < 0)
	{
		printf("Invalid Id for playsample(id=%d, priority=%d, lvolume=%d, rvolume=%d, speed=%d, loop=%d)\n", (int)value[0], (unsigned int)value[1], (int)value[2], (int)value[3], (unsigned int)value[4], (int)value[5]);
		return E_FAIL;
	}
	if((int)value[5]) sound_loop_sample((int)value[0], (unsigned int)value[1], (int)value[2], (int)value[3], (unsigned int)value[4]);
	else sound_play_sample((int)value[0], (unsigned int)value[1], (int)value[2], (int)value[3], (unsigned int)value[4]);
	return S_OK;

playsample_error:

	printf("Function requires 6 integer values: playsample(int id, unsigned int priority, int lvolume, int rvolume, unsigned int speed, int loop)\n");
	return E_FAIL;
}

// int loadsample(filename, log)
HRESULT openbor_loadsample(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int arg = 0;

	if(paramCount < 1)
	{
		goto loadsample_error;
	}
	if(varlist[0]->vt != VT_STR) goto loadsample_error;

	if(paramCount > 1)
	{
		if(varlist[1]->vt == VT_INTEGER)
		{
			arg = varlist[1]->lVal;
		}
		else
		{
			goto loadsample_error;
		}
	}

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	(*pretvar)->lVal = (LONG)sound_load_sample(StrCache_Get(varlist[0]->strVal), packfile, arg);
	return S_OK;

loadsample_error:
	printf("Function requires 1 string value and optional log value: loadsample(string {filename} integer {log})\n");
	*pretvar = NULL;
	return E_FAIL;
}

// void unloadsample(id)
HRESULT openbor_unloadsample(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG id;
	*pretvar = NULL;
	if(paramCount != 1 ) goto unloadsample_error;

	if(FAILED(ScriptVariant_IntegerValue((varlist[0]), &id)))
		goto unloadsample_error;

	sound_unload_sample((int)id);
	return S_OK;

unloadsample_error:
	printf("Function requires 1 integer value: unloadsample(int id)\n");
	return E_FAIL;
}

//fadeout(type, speed);
HRESULT openbor_fadeout(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG type;
	LONG speed;
	*pretvar = NULL;
	if(paramCount < 1 ) goto fade_out_error;

	if(FAILED(ScriptVariant_IntegerValue((varlist[0]), &type)))
		goto fade_out_error;
	if(FAILED(ScriptVariant_IntegerValue((varlist[1]), &speed)))

	fade_out((int)type, (int)speed);
	return S_OK;

fade_out_error:
	printf("Function requires 2 integer values: fade_out(int type, int speed)\n");
	return E_FAIL;
}

//changepalette(index);
HRESULT openbor_changepalette(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG index;

	*pretvar = NULL;

	if(paramCount < 1) goto changepalette_error;

	if(FAILED(ScriptVariant_IntegerValue((varlist[0]), &index)))
		goto changepalette_error;

	change_system_palette((int)index);

	return S_OK;

changepalette_error:
	printf("Function requires 1 integer value: changepalette(int index)\n");
	return E_FAIL;
}

//changelight(x, z);
HRESULT openbor_changelight(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG x, z;
	extern int            light[2];
	ScriptVariant* arg = NULL;

	*pretvar = NULL;
	if(paramCount < 2) goto changelight_error;

	arg = varlist[0];
	if(arg->vt!=VT_EMPTY)
	{
		if(FAILED(ScriptVariant_IntegerValue(arg, &x)))
			goto changelight_error;
		light[0] = (int)x;
	}

	arg = varlist[1];
	if(arg->vt!=VT_EMPTY)
	{
		if(FAILED(ScriptVariant_IntegerValue(arg, &z)))
			goto changelight_error;
		light[1] = (int)z;
	}

	return S_OK;
changelight_error:
	printf("Function requires 2 integer values: changepalette(int x, int z)\n");
	return E_FAIL;
}

//changeshadowcolor(color, alpha);
// color = 0 means no gfxshadow, -1 means don't fill the shadow with colour
// alpha default to 2, <=0 means no alpha effect
HRESULT openbor_changeshadowcolor(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG c, a;
	extern int            shadowcolor;
	extern int            shadowalpha;

	*pretvar = NULL;
	if(paramCount < 1) goto changeshadowcolor_error;

	if(FAILED(ScriptVariant_IntegerValue(varlist[0], &c)))
		goto changeshadowcolor_error;

	shadowcolor = (int)c;

	if(paramCount>1)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[1], &a)))
			goto changeshadowcolor_error;
		shadowalpha = (int)a;
	}

	return S_OK;
changeshadowcolor_error:
	printf("Function requires at least 1 integer value, the 2nd integer parameter is optional: changepalette(int colorindex, int alpha)\n");
	return E_FAIL;
}

// ===== gettextobjproperty(name, value) =====
enum gtop_enum
{
	_gtop_font,
	_gtop_text,
	_gtop_time,
	_gtop_x,
	_gtop_y,
	_gtop_z,
	_gtop_the_end,
};

void mapstrings_gettextobjproperty(ScriptVariant** varlist, int paramCount)
{
	char* propname = NULL;
	int prop;

	static const char* proplist[] = {
		"font",
		"text",
		"time"
		"x",
		"y",
		"z",
	};

	if(paramCount < 2) return;

	MAPSTRINGS(varlist[1], proplist, _gtop_the_end,
		"Property name '%s' is not supported by function gettextobjproperty.\n");
}

HRESULT openbor_gettextobjproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG ind;
	int propind;

	if(paramCount < 2)
		goto gettextobjproperty_error;


	if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
	{
		printf("Function's 1st argument must be a numeric value: gettextproperty(int index, \"property\")\n");
		goto gettextobjproperty_error;
	}

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	mapstrings_gettextobjproperty(varlist, paramCount);

	if(ind<0 || ind >= LEVEL_MAX_TEXTOBJS)
	{
		(*pretvar)->lVal = 0;
		return S_OK;
	}

	if(varlist[1]->vt != VT_INTEGER)
	{
		if(varlist[1]->vt != VT_STR)
			printf("Function gettextobjproperty must have a string property name.\n");
		goto gettextobjproperty_error;
	}

	propind = varlist[1]->lVal;

	switch(propind)
	{
	case _gtop_font:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->textobjs[ind].font;
		break;
	}
	case _gtop_text:
	{
		ScriptVariant_ChangeType(*pretvar, VT_STR);
		strcpy(StrCache_Get((*pretvar)->strVal), level->textobjs[ind].text);
		break;
	}
	case _gtop_time:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->textobjs[ind].t;
		break;
	}
	case _gtop_x:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->textobjs[ind].x;
		break;
	}
	case _gtop_y:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->textobjs[ind].y;
		break;
	}
	case _gtop_z:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->textobjs[ind].z;
		break;
	}
	default:
		//printf("Property name '%s' is not supported by function gettextobjproperty.\n", propname);
		goto gettextobjproperty_error;
		break;
	}

	return S_OK;

gettextobjproperty_error:
	*pretvar = NULL;
	return E_FAIL;
}

// ===== changetextobjproperty(name, value) =====
enum ctop_enum
{
	_ctop_font,
	_ctop_text,
	_ctop_time,
	_ctop_x,
	_ctop_y,
	_ctop_z,
	_ctop_the_end,
};

void mapstrings_changetextobjproperty(ScriptVariant** varlist, int paramCount)
{
	char* propname = NULL;
	int prop;

	static const char* proplist[] = {
		"font",
		"text",
		"time",
		"x",
		"y",
		"z",
	};

	if(paramCount < 2) return;
	MAPSTRINGS(varlist[1], proplist, _ctop_the_end,
		"Property name '%s' is not supported by function changetextobjproperty.\n");
}

HRESULT openbor_changetextobjproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG ind;
	int propind;
	LONG ltemp;

	if(paramCount < 3)
		goto changetextobjproperty_error;


	if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
	{
		printf("Function's 1st argument must be a numeric value: changetextobjproperty(int index, \"property\", value)\n");
		goto changetextobjproperty_error;
	}

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	mapstrings_changetextobjproperty(varlist, paramCount);

	if(ind<0 || ind >= LEVEL_MAX_TEXTOBJS)
	{
		(*pretvar)->lVal = 0;
		return S_OK;
	}

	if(varlist[1]->vt != VT_INTEGER)
	{
		if(varlist[1]->vt != VT_STR)
			printf("Function changetextobjproperty must have a string property name.\n");
		goto changetextobjproperty_error;
	}

	propind = varlist[1]->lVal;

	switch(propind)
	{
	case _ctop_font:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->textobjs[ind].font = (int)ltemp;
		}
		break;
	}
	case _ctop_text:
	{
		if(varlist[2]->vt != VT_STR)
		{
			printf("You must give a string value for textobj text.\n");
			goto changetextobjproperty_error;
		}
		if(!level->textobjs[ind].text)
		{
			level->textobjs[ind].text = (char*)malloc(MAX_STR_VAR_LEN);
		}
		strncpy(level->textobjs[ind].text, (char*)StrCache_Get(varlist[2]->strVal), MAX_STR_VAR_LEN);
		//level->textobjs[ind].text = (char*)StrCache_Get(varlist[2]->strVal);
		(*pretvar)->lVal = (LONG)1;
		break;
	}
	case _ctop_time:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->textobjs[ind].t = (int)ltemp;
		}
		break;
	}
	case _ctop_x:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->textobjs[ind].x = (int)ltemp;
		}
		break;
	}
	case _ctop_y:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->textobjs[ind].y = (int)ltemp;
		}
		break;
	}
	case _ctop_z:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->textobjs[ind].z = (int)ltemp;
		}
		break;
	}
	default:
		//printf("Property name '%s' is not supported by function changetextobjproperty.\n", propname);
		goto changetextobjproperty_error;
		break;
	}

	return S_OK;

changetextobjproperty_error:
	*pretvar = NULL;
	return E_FAIL;
}

// settextobj(int index, int x, int y, int font, int z, char text, int time {optional})
HRESULT openbor_settextobj(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG ind;
	LONG ltemp;

	if(paramCount < 6)
		goto settextobj_error;

	if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
	{
		printf("Function's 1st argument must be a numeric value: settextobj(int index, int x, int y, int font, int z, char text, int time {optional})\n");
		goto settextobj_error;
	}

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

	if(ind<0 || ind >= LEVEL_MAX_TEXTOBJS)
	{
		(*pretvar)->lVal = 0;
		return S_OK;
	}

	if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[1], &ltemp)))
	{
		(*pretvar)->lVal = (LONG)1;
		level->textobjs[ind].x = (int)ltemp;
	}

	if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
	{
		(*pretvar)->lVal = (LONG)1;
		level->textobjs[ind].y = (int)ltemp;
	}

	if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
	{
		(*pretvar)->lVal = (LONG)1;
		level->textobjs[ind].font = (int)ltemp;
	}

	if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[4], &ltemp)))
	{
		(*pretvar)->lVal = (LONG)1;
		level->textobjs[ind].z = (int)ltemp;
	}

	if(varlist[5]->vt != VT_STR)
	{
		printf("You must give a string value for textobj text.\n");
		goto settextobj_error;
	}

	if(paramCount >= 7)
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[6], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->textobjs[ind].t = (int)ltemp;
		}
	}

	if(!level->textobjs[ind].text)
	{
		level->textobjs[ind].text = (char*)malloc(MAX_STR_VAR_LEN);
	}
	strncpy(level->textobjs[ind].text, (char*)StrCache_Get(varlist[5]->strVal), MAX_STR_VAR_LEN);
	(*pretvar)->lVal = (LONG)1;

	return S_OK;

settextobj_error:
	*pretvar = NULL;
	return E_FAIL;
}

HRESULT openbor_cleartextobj(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG ind;

	if(paramCount < 1)
		goto cleartextobj_error;


	if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
	{
		printf("Function's 1st argument must be a numeric value: cleartextobj(int index)\n");
		goto cleartextobj_error;
	}

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

	if(ind<0 || ind >= LEVEL_MAX_TEXTOBJS)
	{
		(*pretvar)->lVal = 0;
		return S_OK;
	}

	level->textobjs[ind].t = 0;
	level->textobjs[ind].x = 0;
	level->textobjs[ind].y = 0;
	level->textobjs[ind].font = 0;
	level->textobjs[ind].z = 0;
	if(level->textobjs[ind].text)
		 free(level->textobjs[ind].text);
	level->textobjs[ind].text = NULL;
	return S_OK;

cleartextobj_error:
	*pretvar = NULL;
	return E_FAIL;
}


// ===== getbglayerproperty ======
enum getbglp_enum
{
	_gbglp_alpha,
	_gbglp_amplitude,
	_gbglp_bgspeedratio,
	_gbglp_enabled,
	_gbglp_transparency,
	_gbglp_watermode,
	_gbglp_wavelength,
	_gbglp_wavespeed,
	_gbglp_xoffset,
	_gbglp_xratio,
	_gbglp_xrepeat,
	_gbglp_xspacing,
	_gbglp_zoffset,
	_gbglp_zratio,
	_gbglp_zrepeat,
	_gbglp_zspacing,
	_gbglp_the_end,
};

void mapstrings_getbglayerproperty(ScriptVariant** varlist, int paramCount)
{
	char *propname = NULL;
	int prop;

	static const char* proplist[] = {
		"alpha",
		"amplitude",
		"bgspeedratio",
		"enabled",
		"transparency",
		"watermode",
		"wavelength",
		"wavespeed",
		"xoffset",
		"xratio",
		"xrepeat",
		"xspacing",
		"zoffset",
		"zratio",
		"zrepeat",
		"zspacing",
	};

	if(paramCount < 2) return;
	MAPSTRINGS(varlist[1], proplist, _gbglp_the_end,
		"Property name '%s' is not supported by function getbglayerproperty.\n");
}

HRESULT openbor_getbglayerproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG ind;
	int propind;

	if(paramCount < 2)
		goto getbglayerproperty_error;

	if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
	{
		printf("Function's 1st argument must be a numeric value: getbglayerproperty(int index, \"property\", value)\n");
		goto getbglayerproperty_error;
	}

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	mapstrings_getbglayerproperty(varlist, paramCount);

	if(ind<0 || ind >= level->numbglayers)
	{
		(*pretvar)->lVal = 0;
		return S_OK;
	}

	if(varlist[1]->vt != VT_INTEGER)
	{
		printf("Function getbglayerproperty must have a string property name.\n");
		goto getbglayerproperty_error;
	}

	propind = varlist[1]->lVal;

	switch(propind)
	{
	case _gbglp_alpha:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->bglayers[ind].alpha;
		break;
	}
	case _gbglp_amplitude:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->bglayers[ind].amplitude;
		break;
	}
	case _gbglp_bgspeedratio:
	{
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)level->bglayers[ind].bgspeedratio;
		break;
	}
	case _gbglp_enabled:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->bglayers[ind].enabled;
		break;
	}
	case _gbglp_transparency:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->bglayers[ind].transparency;
		break;
	}
	case _gbglp_watermode:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->bglayers[ind].watermode;
		break;
	}

	case _gbglp_wavelength:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->bglayers[ind].wavelength;
		break;
	}
	case _gbglp_wavespeed:
	{
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)level->bglayers[ind].wavespeed;
		break;
	}
	case _gbglp_xoffset:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->bglayers[ind].xoffset;
		break;
	}
	case _gbglp_xratio:
	{
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)level->bglayers[ind].xratio;
		break;
	}
	case _gbglp_xrepeat:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->bglayers[ind].xrepeat;
		break;
	}
	case _gbglp_xspacing:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->bglayers[ind].xspacing;
		break;
	}
	case _gbglp_zoffset:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->bglayers[ind].zoffset;
		break;
	}
	case _gbglp_zratio:
	{
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)level->bglayers[ind].zratio;
		break;
	}
	case _gbglp_zrepeat:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->bglayers[ind].zrepeat;
		break;
	}
	case _gbglp_zspacing:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->bglayers[ind].zspacing;
		break;
	}
	default:
		//printf("Property name '%s' is not supported by function getbglayerproperty.\n", propname);
		goto getbglayerproperty_error;
		break;
	}

	return S_OK;

getbglayerproperty_error:
	*pretvar = NULL;
	return E_FAIL;
}

// ===== changebglayerproperty =====
enum cbglp_enum
{
	_cbglp_alpha,
	_cbglp_amplitude,
	_cbglp_bgspeedratio,
	_cbglp_enabled,
	_cbglp_transparency,
	_cbglp_watermode,
	_cbglp_wavelength,
	_cbglp_wavespeed,
	_cbglp_xoffset,
	_cbglp_xratio,
	_cbglp_xrepeat,
	_cbglp_xspacing,
	_cbglp_zoffset,
	_cbglp_zratio,
	_cbglp_zrepeat,
	_cbglp_zspacing,
	_cbglp_the_end,
};

void mapstrings_changebglayerproperty(ScriptVariant** varlist, int paramCount)
{
	char *propname = NULL;
	int prop;

	static const char* proplist[] = {
		"alpha",
		"amplitude",
		"bgspeedratio",
		"enabled",
		"transparency",
		"watermode",
		"wavelength",
		"wavespeed",
		"xoffset",
		"xratio",
		"xrepeat",
		"xspacing",
		"zoffset",
		"zratio",
		"zrepeat",
		"zspacing",
	};

	if(paramCount < 2) return;
	MAPSTRINGS(varlist[1], proplist, _cbglp_the_end,
		"Property name '%s' is not supported by function changebglayerproperty.\n");
}

HRESULT openbor_changebglayerproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG ind;
	int propind;
	LONG ltemp;
	DOUBLE dbltemp;

	if(paramCount < 3)
		goto changebglayerproperty_error;


	if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
	{
		printf("Function's 1st argument must be a numeric value: changebglayerproperty(int index, \"property\", value)\n");
		goto changebglayerproperty_error;
	}

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	mapstrings_changebglayerproperty(varlist, paramCount);

	if(ind<0 || ind >= level->numbglayers)
	{
		(*pretvar)->lVal = 0;
		return S_OK;
	}

	if(varlist[1]->vt != VT_INTEGER)
	{
		printf("Function changebglayerproperty must have a string property name.\n");
		goto changebglayerproperty_error;
	}

	propind = varlist[1]->lVal;

	switch(propind)
	{
	case _cbglp_alpha:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->bglayers[ind].alpha = (int)ltemp;
		}
		break;
	}
	case _cbglp_amplitude:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->bglayers[ind].amplitude = (int)ltemp;
		}
		break;
	}
	case _cbglp_bgspeedratio:
	{
		if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->bglayers[ind].bgspeedratio = (float)dbltemp;
		}
		break;
	}
	case _cbglp_enabled:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->bglayers[ind].enabled = (int)ltemp;
		}
		break;
	}
	case _cbglp_transparency:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->bglayers[ind].transparency = (int)ltemp;
		}
		break;
	}
	case _cbglp_watermode:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->bglayers[ind].watermode = (int)ltemp;
		}
		break;
	}

	case _cbglp_wavelength:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->bglayers[ind].wavelength = (int)ltemp;
		}
		break;
	}
	case _cbglp_wavespeed:
	{
		if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->bglayers[ind].wavespeed = (float)dbltemp;
		}
		break;
	}
	case _cbglp_xoffset:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->bglayers[ind].xoffset = (int)ltemp;
		}
		break;
	}
	case _cbglp_xratio:
	{
		if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->bglayers[ind].xratio = (float)dbltemp;
		}
		break;
	}
	case _cbglp_xrepeat:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->bglayers[ind].xrepeat = (int)ltemp;
		}
		break;
	}
	case _cbglp_xspacing:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->bglayers[ind].xspacing = (int)ltemp;
		}
		break;
	}
	case _cbglp_zoffset:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->bglayers[ind].zoffset = (int)ltemp;
		}
		break;
	}
	case _cbglp_zratio:
	{
		if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->bglayers[ind].zratio = (float)dbltemp;
		}
		break;
	}
	case _cbglp_zrepeat:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->bglayers[ind].zrepeat = (int)ltemp;
		}
		break;
	}
	case _cbglp_zspacing:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->bglayers[ind].zspacing = (int)ltemp;
		}
		break;
	}
	default:
		//printf("Property name '%s' is not supported by function changebglayerproperty.\n", propname);
		goto changebglayerproperty_error;
		break;
	}

	return S_OK;

changebglayerproperty_error:
	*pretvar = NULL;
	return E_FAIL;
}

// ===== getfglayerproperty =====
enum gfglp_enum
{
	_gfglp_alpha,
	_gfglp_amplitude,
	_gfglp_bgspeedratio,
	_gfglp_enabled,
	_gfglp_transparency,
	_gfglp_watermode,
	_gfglp_wavelength,
	_gfglp_wavespeed,
	_gfglp_xoffset,
	_gfglp_xratio,
	_gfglp_xrepeat,
	_gfglp_xspacing,
	_gfglp_zoffset,
	_gfglp_zratio,
	_gfglp_zrepeat,
	_gfglp_zspacing,
	_gfglp_the_end,
};

void mapstrings_getfglayerproperty(ScriptVariant** varlist, int paramCount)
{
	char *propname = NULL;
	int prop;

	static const char* proplist[] = {
		"alpha",
		"amplitude",
		"bgspeedratio",
		"enabled",
		"transparency",
		"watermode",
		"wavelength",
		"wavespeed",
		"xoffset",
		"xratio",
		"xrepeat",
		"xspacing",
		"zoffset",
		"zratio",
		"zrepeat",
		"zspacing",
	};

	if(paramCount < 2) return;
	MAPSTRINGS(varlist[1], proplist, _gfglp_the_end,
		"Property name '%s' is not supported by function getfglayerproperty.\n");
}

HRESULT openbor_getfglayerproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG ind;
	int propind;

	if(paramCount < 2)
		goto getfglayerproperty_error;


	if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
	{
		printf("Function's 1st argument must be a numeric value: getfglayerproperty(int index, \"property\", value)\n");
		goto getfglayerproperty_error;
	}

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	mapstrings_getfglayerproperty(varlist, paramCount);

	if(ind<0 || ind >= level->numfglayers)
	{
		(*pretvar)->lVal = 0;
		return S_OK;
	}

	if(varlist[1]->vt != VT_INTEGER)
	{
		printf("Function getfglayerproperty must have a string property name.\n");
		goto getfglayerproperty_error;
	}

	propind = varlist[1]->lVal;

	switch(propind)
	{
	case _gfglp_alpha:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->fglayers[ind].alpha;
		break;
	}
	case _gfglp_amplitude:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->fglayers[ind].amplitude;
		break;
	}
	case _gfglp_bgspeedratio:
	{
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)level->fglayers[ind].bgspeedratio;
		break;
	}
	case _gfglp_enabled:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->fglayers[ind].enabled;
		break;
	}
	case _gfglp_transparency:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->fglayers[ind].transparency;
		break;
	}
	case _gfglp_watermode:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->fglayers[ind].watermode;
		break;
	}

	case _gfglp_wavelength:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->fglayers[ind].wavelength;
		break;
	}
	case _gfglp_wavespeed:
	{
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)level->fglayers[ind].wavespeed;
		break;
	}
	case _gfglp_xoffset:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->fglayers[ind].xoffset;
		break;
	}
	case _gfglp_xratio:
	{
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)level->fglayers[ind].xratio;
		break;
	}
	case _gfglp_xrepeat:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->fglayers[ind].xrepeat;
		break;
	}
	case _gfglp_xspacing:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->fglayers[ind].xspacing;
		break;
	}
	case _gfglp_zoffset:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->fglayers[ind].zoffset;
		break;
	}
	case _gfglp_zratio:
	{
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)level->fglayers[ind].zratio;
		break;
	}
	case _gfglp_zrepeat:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->fglayers[ind].zrepeat;
		break;
	}
	case _gfglp_zspacing:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->fglayers[ind].zspacing;
		break;
	}
	default:
		//printf("Property name '%s' is not supported by function getfglayerproperty.\n", propname);
		goto getfglayerproperty_error;
		break;
	}

	return S_OK;

getfglayerproperty_error:
	*pretvar = NULL;
	return E_FAIL;
}

// ===== changefglayerproperty =====
typedef enum
{
	_cfglp_alpha,
	_cfglp_amplitude,
	_cfglp_bgspeedratio,
	_cfglp_enabled,
	_cfglp_transparency,
	_cfglp_watermode,
	_cfglp_wavelength,
	_cfglp_wavespeed,
	_cfglp_xoffset,
	_cfglp_xratio,
	_cfglp_xrepeat,
	_cfglp_xspacing,
	_cfglp_zoffset,
	_cfglp_zratio,
	_cfglp_zrepeat,
	_cfglp_zspacing,
	_cfglp_the_end,
} prop_enum;

void mapstrings_changefglayerproperty(ScriptVariant** varlist, int paramCount)
{
	char *propname = NULL;
	int prop;

	static const char* proplist[] = {
		"alpha",
		"amplitude",
		"bgspeedratio",
		"enabled",
		"transparency",
		"watermode",
		"wavelength",
		"wavespeed",
		"xoffset",
		"xratio",
		"xrepeat",
		"xspacing",
		"zoffset",
		"zratio",
		"zrepeat",
		"zspacing",
	};

	if(paramCount < 2) return;
	MAPSTRINGS(varlist[1], proplist, _cfglp_the_end,
		"Property name '%s' is not supported by function changefglayerproperty.\n");
}

HRESULT openbor_changefglayerproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG ind;
	int propind;
	LONG ltemp;
	DOUBLE dbltemp;

	if(paramCount < 3)
		goto changefglayerproperty_error;

	if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
	{
		printf("Function's 1st argument must be a numeric value: changefglayerproperty(int index, \"property\", value)\n");
		goto changefglayerproperty_error;
	}

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	mapstrings_changefglayerproperty(varlist, paramCount);

	if(ind<0 || ind >= level->numfglayers)
	{
		(*pretvar)->lVal = 0;
		return S_OK;
	}

	if(varlist[1]->vt != VT_INTEGER)
	{
		printf("Function changefglayerproperty must have a string property name.\n");
		goto changefglayerproperty_error;
	}

	propind = varlist[1]->lVal;

	switch(propind)
	{
	case _cfglp_alpha:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->fglayers[ind].alpha = (int)ltemp;
		}
		break;
	}
	case _cfglp_amplitude:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->fglayers[ind].amplitude = (int)ltemp;
		}
		break;
	}
	case _cfglp_bgspeedratio:
	{
		if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->fglayers[ind].bgspeedratio = (float)dbltemp;
		}
		break;
	}
	case _cfglp_enabled:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->fglayers[ind].enabled = (int)ltemp;
		}
		break;
	}
	case _cfglp_transparency:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->fglayers[ind].transparency = (int)ltemp;
		}
		break;
	}
	case _cfglp_watermode:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->fglayers[ind].watermode = (int)ltemp;
		}
		break;
	}

	case _cfglp_wavelength:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->fglayers[ind].wavelength = (int)ltemp;
		}
		break;
	}
	case _cfglp_wavespeed:
	{
		if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->fglayers[ind].wavespeed = (float)dbltemp;
		}
		break;
	}
	case _cfglp_xoffset:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->fglayers[ind].xoffset = (int)ltemp;
		}
		break;
	}
	case _cfglp_xratio:
	{
		if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->fglayers[ind].xratio = (float)dbltemp;
		}
		break;
	}
	case _cfglp_xrepeat:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->fglayers[ind].xrepeat = (int)ltemp;
		}
		break;
	}
	case _cfglp_xspacing:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->fglayers[ind].xspacing = (int)ltemp;
		}
		break;
	}
	case _cfglp_zoffset:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->fglayers[ind].zoffset = (int)ltemp;
		}
		break;
	}
	case _cfglp_zratio:
	{
		if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->fglayers[ind].zratio = (float)dbltemp;
		}
		break;
	}
	case _cfglp_zrepeat:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->fglayers[ind].zrepeat = (int)ltemp;
		}
		break;
	}
	case _cfglp_zspacing:
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			level->fglayers[ind].zspacing = (int)ltemp;
		}
		break;
	}
	default:
		//printf("Property name '%s' is not supported by function changefglayerproperty.\n", propname);
		goto changefglayerproperty_error;
		break;
	}

	return S_OK;

changefglayerproperty_error:
	*pretvar = NULL;
	return E_FAIL;
}

HRESULT openbor_getlevelproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	char* propname = NULL;
	int propind;

	static const char* proplist[] = {
		"bgspeed",
		"cameraxoffset",
		"camerazoffset",
		"scrollspeed",
	};

	typedef enum
	{
		_lp_bgspeed,
		_lp_cameraxoffset,
		_lp_camerazoffset,
		_lp_scrollspeed,
		_lp_the_end, // lol
	} prop_enum;



	if(varlist[0]->vt != VT_STR)
	{
		printf("Function getlevelproperty must have a string property name.\n");
		goto getlevelproperty_error;
	}


	propname = (char*)StrCache_Get(varlist[0]->strVal);//see what property it is

	propind = searchList(proplist, propname, _lp_the_end);

	switch(propind)
	{
	case _lp_bgspeed:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->bgspeed;
		break;
	}
	case _lp_cameraxoffset:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->cameraxoffset;
		break;
	}
	case _lp_camerazoffset:
	{
		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)level->camerazoffset;
		break;
	}
	case _lp_scrollspeed:
	{
		ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
		(*pretvar)->dblVal = (DOUBLE)level->scrollspeed;
		break;
	}
	default:
		printf("Property name '%s' is not supported by function getlevelproperty.\n", propname);
		goto getlevelproperty_error;
		break;
	}

	return S_OK;

getlevelproperty_error:
	*pretvar = NULL;
	return E_FAIL;
}

//changelevelproperty(name, value)
HRESULT openbor_changelevelproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	char* propname = NULL;
	LONG ltemp;
	ScriptVariant* arg = NULL;

	if(paramCount < 2)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	if(varlist[0]->vt != VT_STR)
	{
		*pretvar = NULL;
		return E_FAIL;
	}

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	(*pretvar)->lVal = (LONG)1;

	propname = (char*)StrCache_Get(varlist[0]->strVal);

	arg = varlist[1];

	if(strcmp(propname, "rock")==0)
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
			level->rocking = (int)ltemp;
		else (*pretvar)->lVal = (LONG)0;
	}
	else if(strcmp(propname, "bgspeed")==0)
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
			level->bgspeed = (float)ltemp;
		else (*pretvar)->lVal = (LONG)0;
	}
	else if(strcmp(propname, "scrollspeed")==0)
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
			level->scrollspeed = (float)ltemp;
		else (*pretvar)->lVal = (LONG)0;
	}
	else if(strcmp(propname, "cameraxoffset")==0)
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
			level->cameraxoffset = (int)ltemp;
		else (*pretvar)->lVal = (LONG)0;
	}
	else if(strcmp(propname, "camerazoffset")==0)
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
			level->camerazoffset = (int)ltemp;
		else (*pretvar)->lVal = (LONG)0;
	}

	return S_OK;
}

//jumptobranch(name, immediate)
HRESULT openbor_jumptobranch(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG ltemp;
	extern char branch_name[MAX_NAME_LEN+1];
	extern int  endgame;

	*pretvar = NULL;
	if(paramCount < 1) goto jumptobranch_error;
	if(varlist[0]->vt != VT_STR) goto jumptobranch_error;

	strncpy(branch_name, StrCache_Get(varlist[0]->strVal), MIN(MAX_NAME_LEN, MAX_STR_VAR_LEN)); // copy the string value to branch name

	if(paramCount >= 2)
	{
		if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[1], &ltemp)))
		{
			endgame = (int)ltemp;
			// 1 means goto that level immediately, or, wait until the level is complete
		}
		else goto jumptobranch_error;
	}

	return S_OK;
jumptobranch_error:
	printf("Function requires 1 string value, the second argument is optional(int): jumptobranch(name, immediate)\n");
	return E_FAIL;
}

//bindentity(entity, target, x, z, a, direction, bindanim);
//bindentity(entity, NULL()); // unbind
HRESULT openbor_bindentity(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	entity* ent = NULL;
	entity* other = NULL;
	ScriptVariant* arg = NULL;
	void adjust_bind(entity* e);
	LONG x=0, z=0, a=0, dir=0, anim=0;

   *pretvar = NULL;
	if(paramCount < 2)
	{
		return E_FAIL;
	}

	ent = (entity*)(varlist[0])->ptrVal; //retrieve the entity
	if(!ent)  return S_OK;

	other = (entity*)(varlist[1])->ptrVal;
	if(!other)  {ent->bound = NULL; return S_OK;}

	ent->bound = other;

	if(paramCount < 3) goto BIND;
	// x
	arg = varlist[2];
	if(arg->vt != VT_EMPTY)
	{
		if(FAILED(ScriptVariant_IntegerValue(arg, &x)))  return E_FAIL;

		ent->bindoffset[0] = (int)x;
	}
	if(paramCount < 4) goto BIND;
	// z
	arg = varlist[3];
	if(arg->vt != VT_EMPTY)
	{
		if(FAILED(ScriptVariant_IntegerValue(arg, &z)))  return E_FAIL;
		ent->bindoffset[1] = (int)z;
	}
	if(paramCount < 5) goto BIND;
	// a
	arg = varlist[4];
	if(arg->vt != VT_EMPTY)
	{
		if(FAILED(ScriptVariant_IntegerValue(arg, &a)))  return E_FAIL;
		ent->bindoffset[2] = (int)a;
	}
	if(paramCount < 6) goto BIND;
	// direction
	arg = varlist[5];
	if(arg->vt != VT_EMPTY)
	{
		if(FAILED(ScriptVariant_IntegerValue(arg, &dir)))  return E_FAIL;
		ent->bindoffset[3] = (int)dir;
	}
	if(paramCount < 7) goto BIND;
	// animation
	arg = varlist[6];
	if(arg->vt != VT_EMPTY)
	{
		if(FAILED(ScriptVariant_IntegerValue(arg, &anim)))  return E_FAIL;
		ent->bindanim = (int)anim;
	}

BIND:
	adjust_bind(ent);

	return S_OK;
}

//allocscreen(int w, int h);
HRESULT openbor_allocscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG w, h;
	s_screen* screen;

	if(paramCount < 2) goto allocscreen_error;

	if(FAILED(ScriptVariant_IntegerValue(varlist[0], &w)))
		goto allocscreen_error;
	if(FAILED(ScriptVariant_IntegerValue(varlist[1], &h)))
		goto allocscreen_error;


	ScriptVariant_ChangeType(*pretvar, VT_PTR);
	screen = allocscreen((int)w, (int)h, screenformat);
	if(screen) clearscreen(screen);
	(*pretvar)->ptrVal = (VOID*)screen;

	if((*pretvar)->ptrVal==NULL)
	{
		printf("Not enough memory: allocscreen(%d, %d)\n", (int)w, (int)h);
		(*pretvar) = NULL;
		return E_FAIL;
	}
	List_InsertAfter(&scriptheap, (void*)((*pretvar)->ptrVal), "openbor_allocscreen");
	return S_OK;

allocscreen_error:
	printf("Function requires 2 int values: allocscreen(int width, int height)\n");
	(*pretvar) = NULL;
	return E_FAIL;
}

//clearscreen(s_screen* screen)
HRESULT openbor_clearscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	s_screen* screen;

	*pretvar = NULL;
	if(paramCount != 1) goto clearscreen_error;
	if(varlist[0]->vt != VT_PTR) goto clearscreen_error;

	screen = (s_screen*)varlist[0]->ptrVal;

	if(screen == NULL)
	{
		printf("Error: NULL pointer passed to clearscreen(void screen)\n");
		*pretvar = NULL;
		return E_FAIL;
	}
	clearscreen(screen);
	return S_OK;

clearscreen_error:
	printf("Function requires a screen pointer: clearscreen(void screen)\n");
	return E_FAIL;
}

//setdrawmethod(entity, int flag, int scalex, int scaley, int flipx, int flipy, int shiftx, int alpha, int remap, int fillcolor, int rotate, int fliprotate, int transparencybg, void* colourmap);
HRESULT openbor_setdrawmethod(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG value[12];
	entity* e;
	s_drawmethod *pmethod;
	int i;

	*pretvar = NULL;
	if(paramCount<2) goto setdrawmethod_error;

	if(varlist[0]->vt==VT_EMPTY) e = NULL;
	else if(varlist[0]->vt==VT_PTR) e = (entity*)varlist[0]->ptrVal;
	else goto setdrawmethod_error;

	if(e) pmethod = &(e->drawmethod);
	else  pmethod = &(drawmethod);

	memset(value, 0, sizeof(LONG)*12);
	for(i=1; i<paramCount && i<13; i++)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i-1))) goto setdrawmethod_error;
	}

	if(paramCount>=15 && varlist[13]->vt!=VT_PTR && varlist[13]->vt!=VT_EMPTY) goto setdrawmethod_error;

	pmethod->flag = (int)value[0];
	pmethod->scalex = (int)value[1];
	pmethod->scaley = (int)value[2];
	pmethod->flipx = (int)value[3];
	pmethod->flipy = (int)value[4];
	pmethod->shiftx = (int)value[5];
	pmethod->alpha = (int)value[6];
	pmethod->remap = (int)value[7];
	pmethod->fillcolor = (int)value[8];
	pmethod->rotate = ((int)value[9])%360;
	pmethod->fliprotate = (int)value[10];
	pmethod->transbg = (int)value[11];
	if(paramCount>=14) pmethod->table=(unsigned char*)varlist[13]->ptrVal;

	if(pmethod->rotate)
	{
		if(pmethod->rotate<0) pmethod->rotate += 360;
	}
	return S_OK;

setdrawmethod_error:
	printf("Function need a valid entity handle and at least 1 interger parameter, all other parameters should be integers: setdrawmethod(entity, int flag, int scalex, int scaley, int flipx, int flipy, int shiftx, int alpha, int remap, int fillcolor, int rotate, int fliprotate, int transparencybg, void* colourmap)\n");
	return E_FAIL;
}

//updateframe(entity, int frame);
HRESULT openbor_updateframe(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG f;
	entity* e;
	void update_frame(entity* ent, int f);

	*pretvar = NULL;
	if(paramCount<2) goto updateframe_error;

	if(varlist[0]->vt==VT_EMPTY) e = NULL;
	else if(varlist[0]->vt==VT_PTR) e = (entity*)varlist[0]->ptrVal;
	else goto updateframe_error;

	if(!e) goto updateframe_error;

	if(FAILED(ScriptVariant_IntegerValue(varlist[1], &f))) goto updateframe_error;

	update_frame(e, (int)f);

	return S_OK;

updateframe_error:
	printf("Function need a valid entity handle and at an interger parameter: updateframe(entity, int frame)\n");
	return E_FAIL;
}

//performattack(entity, int anim, int resetable);
HRESULT openbor_performattack(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG anim, resetable=0;
	entity* e;

	*pretvar = NULL;
	if(paramCount<1) goto performattack_error;

	if(varlist[0]->vt==VT_EMPTY) e = NULL;
	else if(varlist[0]->vt==VT_PTR) e = (entity*)varlist[0]->ptrVal;
	else goto performattack_error;

	if(!e) goto performattack_error;

	e->takeaction = common_attack_proc;
	e->attacking = 1;
	e->idling = 0;
	e->drop = 0;
	e->falling = 0;
	e->inpain = 0;
	e->blocking = 0;

	if(paramCount==1) return S_OK;

	if(paramCount>1 && FAILED(ScriptVariant_IntegerValue(varlist[1], &anim))) goto performattack_error;
	if(paramCount>2 && FAILED(ScriptVariant_IntegerValue(varlist[2], &resetable)))  goto performattack_error;
	ent_set_anim(e, (int)anim, (int)resetable);

	return S_OK;

performattack_error:
	printf("Function need a valid entity handle, the other 2 integer parameters are optional: performattack(entity, int anim, int resetable)\n");
	return E_FAIL;
}

//setidle(entity, int anim, int resetable, int stalladd);
HRESULT openbor_setidle(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG anim, resetable=0, stalladd;
	entity* e;
	extern unsigned int time;

	*pretvar = NULL;
	if(paramCount<1) goto setidle_error;

	if(varlist[0]->vt==VT_EMPTY) e = NULL;
	else if(varlist[0]->vt==VT_PTR) e = (entity*)varlist[0]->ptrVal;
	else goto setidle_error;

	if(!e) goto setidle_error;

	e->takeaction = NULL;
	e->attacking = 0;
	e->idling = 1;
	e->drop = 0;
	e->falling = 0;
	e->inpain = 0;
	e->blocking = 0;
	e->nograb = 0;

	if(paramCount==1) return S_OK;

	if(paramCount>1 && FAILED(ScriptVariant_IntegerValue(varlist[1], &anim))) goto setidle_error;
	if(paramCount>2 && FAILED(ScriptVariant_IntegerValue(varlist[2], &resetable)))  goto setidle_error;
	if(paramCount>3 && FAILED(ScriptVariant_IntegerValue(varlist[3], &stalladd)))  goto setidle_error;
	ent_set_anim(e, (int)anim, (int)resetable);

	if(stalladd>0) e->stalltime = time+stalladd;

	return S_OK;

setidle_error:
	printf("Function need a valid entity handle, the other 2 integer parameters are optional: setidle(entity, int anim, int resetable, int stalladd)\n");
	return E_FAIL;
}

//getentity(int index_from_list)
HRESULT openbor_getentity(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG ind;
	extern entity* ent_list[MAX_ENTS];

	if(paramCount!=1) goto getentity_error;

	if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind))) goto getentity_error;

	ScriptVariant_Clear(*pretvar);

	if((int)ind<MAX_ENTS && (int)ind>=0)
	{
		ScriptVariant_ChangeType(*pretvar, VT_PTR);
		(*pretvar)->ptrVal = (VOID*)ent_list[(int)ind];
	}
	//else, it should return an empty value
	return S_OK;

getentity_error:
	printf("Function need an integer parameter: getentity(int index_in_list)\n");
	*pretvar = NULL;
	return E_FAIL;
}


//loadmodel(name)
HRESULT openbor_loadmodel(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG unload = 0;
	if(paramCount<1) goto loadmodel_error;
	if(varlist[0]->vt!=VT_STR) goto loadmodel_error;

	 ScriptVariant_ChangeType(*pretvar, VT_PTR);
	 if(paramCount == 2)
		 if(FAILED(ScriptVariant_IntegerValue(varlist[1], &unload))) goto loadmodel_error;

	(*pretvar)->ptrVal = (VOID*)load_cached_model(StrCache_Get(varlist[0]->strVal), "openbor_loadmodel", (char)unload);
	//else, it should return an empty value
	return S_OK;

loadmodel_error:
	printf("Function needs a string and boolean parameters: loadmodel(name, unload)\n");
	ScriptVariant_Clear(*pretvar);
	*pretvar = NULL;
	return E_FAIL;
}

// load a sprite which doesn't belong to the sprite_cache
// loadsprite(path)
HRESULT openbor_loadsprite(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	extern s_sprite * loadsprite2(char *filename, int* width, int* height);
	if(paramCount!=1) goto loadsprite_error;

	if(varlist[0]->vt!=VT_STR) goto loadsprite_error;

	ScriptVariant_ChangeType(*pretvar, VT_PTR);
	if(((*pretvar)->ptrVal = (VOID*)loadsprite2(StrCache_Get(varlist[0]->strVal), NULL, NULL)))
	{
		List_InsertAfter(&scriptheap, (void*)((*pretvar)->ptrVal), "openbor_loadsprite");
	}
	//else, it should return an empty value
	return S_OK;

loadsprite_error:
	printf("Function need a string parameter: loadsprite(path)\n");
	ScriptVariant_Clear(*pretvar);
	*pretvar = NULL;
	return E_FAIL;
}

//playgif(path, int x, int y, int noskip)
HRESULT openbor_playgif(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG temp[3] = {0,0,0}; //x,y,noskip
	int i;
	extern unsigned char pal[1024];
	extern int playgif(char *filename, int x, int y, int noskip);
	if(paramCount<1) goto playgif_error;

	if(varlist[0]->vt!=VT_STR) goto playgif_error;

	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
	for(i=0; i<3 && i<paramCount-1; i++)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[i+1], temp+i))) goto playgif_error;
	}
	(*pretvar)->lVal = (LONG)playgif(StrCache_Get(varlist[0]->strVal), (int)(temp[0]), (int)(temp[1]), (int)(temp[2]));
	palette_set_corrected(pal, savedata.gamma,savedata.gamma,savedata.gamma, savedata.brightness,savedata.brightness,savedata.brightness);
	return S_OK;

playgif_error:
	printf("Function need a string parameter, other parameters are optional: playgif(path, int x, int y, int noskip)\n");
	*pretvar = NULL;
	return E_FAIL;
}


//zoom(int x, int y, int scalex, int scaley, int z)
HRESULT openbor_zoom(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{

	extern int zoom_center_x, zoom_center_y, zoom_scale_x, zoom_scale_y, zoom_z;

	*pretvar = NULL;
	if(paramCount<4) goto zoom_error;

	if(FAILED(ScriptVariant_IntegerValue(varlist[0], &zoom_center_x)))
		goto zoom_error;
	if(FAILED(ScriptVariant_IntegerValue(varlist[1], &zoom_center_y)))
		goto zoom_error;
	if(FAILED(ScriptVariant_IntegerValue(varlist[2], &zoom_scale_x)))
		goto zoom_error;
	if(FAILED(ScriptVariant_IntegerValue(varlist[3], &zoom_scale_y)))
		goto zoom_error;
	if(FAILED(ScriptVariant_IntegerValue(varlist[4], &zoom_z)))
		goto zoom_error;
	
	return S_OK;

zoom_error:
	printf("Function requires 5 int values: zoom(int x, int y, int scalex, int scaley, int z)\n");
	return E_FAIL;
}
