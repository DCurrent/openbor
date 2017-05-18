/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
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

#ifndef SCRIPT_COMMON
#define SCRIPT_COMMON 1

#include "config.h"
#include "../../openbor.h"
#include "../gamelib/soundmix.h"
#include "../globals.h"
#include "ImportCache.h"
#include "../gamelib/models.h"

#define _is_not_a_known_subproperty_of_  "'%s' is not a known subproperty of '%s'.\n"
#define _is_not_supported_by_ "'%s' is not supported by '%s'.\n"

// Define macro for string mapping
#define MAPSTRINGS(VAR, LIST, MAXINDEX, FAILMSG, args...) \
if(VAR->vt == VT_STR) { \
	propname = (char*)StrCache_Get(VAR->strVal); \
	prop = searchList(LIST, propname, MAXINDEX); \
	if(prop >= 0) { \
		ScriptVariant_ChangeType(VAR, VT_INTEGER); \
		VAR->lVal = prop; \
	} else { printf(FAILMSG, propname, ##args); return 0;} \
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
extern s_level        *level;
extern s_filestream   *filestreams;
extern int			  numfilestreams;
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
extern int            *animbackpains;
extern int            *animbackfalls;
extern int            *animbackdies;


extern int            noshare;
extern int            credits;
extern char           musicname[128];
extern float          musicfade[2];
extern int            musicloop;
extern u32            musicoffset;
extern int            models_cached;
extern int endgame;
extern int useSave;
extern int useSet;


extern s_sprite_list *sprite_list;
extern s_sprite_map *sprite_map;

extern unsigned char *blendings[MAX_BLENDINGS];
extern int            current_palette;
extern s_collision_attack emptyattack;
Varlist global_var_list;
Script *pcurrentscript = NULL; //used by local script functions
List theFunctionList;
static List   scriptheap;
static s_spawn_entry spawnentry;
static s_drawmethod drawmethod;

int            max_indexed_vars = 0;
int            max_entity_vars = 0;
int            max_script_vars = 0;
int			   no_nested_script = 0;

static void clear_named_var_list(List *list, int level)
{
    ScriptVariant *var;
    int i, size;
    size = List_GetSize(list);
    for(i = 0, List_Reset(list); i < size; i++)
    {
        var = (ScriptVariant *)List_Retrieve(list);
        ScriptVariant_Clear(var);
        free(var);
        List_Remove(list);
    }
    if(level)
    {
        List_Clear(list);
    }
}

void Varlist_Init(Varlist *varlist, int size)
{
    int i;

    varlist->magic = varlist_magic;
    varlist->list = calloc(1, sizeof(*varlist->list));
    List_Init(varlist->list);
    varlist->vars = calloc(size + 1, sizeof(*varlist->vars));
    for(i = 0; i <= size; i++)
    {
        ScriptVariant_Init(varlist->vars + i);
        ScriptVariant_ChangeType(varlist->vars, VT_INTEGER);
        varlist->vars->lVal = (LONG)size;
    }
}

void Varlist_Clear(Varlist *varlist)
{
    int i;
    clear_named_var_list(varlist->list, 1);
    free(varlist->list);
    varlist->list = NULL;
    // the first one must be an integer variable, so it's safe to leave it alone
    for(i = 1; i <= varlist->vars->lVal; i++)
    {
        ScriptVariant_Clear(varlist->vars + i);
    }
    free(varlist->vars);
    varlist->vars = NULL;
    varlist->magic = 0;
}

void Varlist_Cleanup(Varlist *varlist)
{
    int i;
    clear_named_var_list(varlist->list, 0);
    for(i = 1; i <= varlist->vars->lVal; i++)
    {
        ScriptVariant_Clear(varlist->vars + i);
    }
}

ScriptVariant *Varlist_GetByName(Varlist *varlist, char *theName)
{
    if(!theName || !theName[0])
    {
        return NULL;
    }

    if(List_FindByName(varlist->list, theName))
    {
        return (ScriptVariant *)List_Retrieve(varlist->list);
    }

    return NULL;
}

int Varlist_SetByName(Varlist *varlist, char *theName, ScriptVariant *var)
{
    ScriptVariant *v;
    if(!theName || !theName[0])
    {
        return 0;
    }
    if(List_FindByName(varlist->list, theName))
    {
        ScriptVariant_Copy((ScriptVariant *)List_Retrieve(varlist->list), var);
    }
    else
    {
        v = calloc(1, sizeof(*v));
        ScriptVariant_Copy(v, var);
        List_InsertAfter(varlist->list, v, theName);
    }
    return 1;
}

ScriptVariant *Varlist_GetByIndex(Varlist *varlist, int index)
{
    if(index < 0 || index >= varlist->vars->lVal)
    {
        return NULL;
    }
    return varlist->vars + index + 1;
}

int Varlist_SetByIndex(Varlist *varlist, int index, ScriptVariant *var)
{
    if(index < 0)
    {
        return 0;
    }
    else if(index >= varlist->vars->lVal)
    {
        __reallocto(varlist->vars, varlist->vars->lVal + 1, index + 2);
        varlist->vars->lVal = index + 1;
    }
    ScriptVariant_Copy(varlist->vars + index + 1, var);
    return 1;
}

// By White Dragon
int Varlist_AddByIndex(Varlist *array, int index, ScriptVariant *var)
{
    if(index < 0 || index >= array->vars->lVal+1)
    {
        return 0;
    }
    else
    {
        int i = 0;
        int size = array->vars->lVal;

        __reallocto(array->vars, size+1, size+2);
        size = ++array->vars->lVal;

        for ( i = size-1; i > index; i-- )
        {
            ScriptVariant_Copy(array->vars+1+i, array->vars+1+i-1); // first value of array is his size!
        }
        ScriptVariant_Copy(array->vars+1+index, var);

        //printf("aaa: %s\n", (char*)StrCache_Get(elem->strVal) );
    }

    return 1;
}

// By White Dragon
int Varlist_DeleteByIndex(Varlist *array, int index)
{
    if(index < 0 || index >= array->vars->lVal)
    {
        return 0;
    }
    else
    {
        int i = 0;
        int size = array->vars->lVal;
        ScriptVariant *elem;

        for ( i = index; i < size-1; i++ )
        {
            ScriptVariant_Copy(array->vars+1+i, array->vars+1+i+1); // first value of array is his size!
        }
        --array->vars->lVal;

        // set last element to NULL
        elem = array->vars+1+size-1;
        ScriptVariant_ChangeType(elem, VT_EMPTY);
        elem->ptrVal = NULL;

        //realloc mem
        array->vars = realloc((array->vars), sizeof(*(array->vars))*(array->vars->lVal+1));

        //printf("aaa: %s\n", (char*)StrCache_Get(elem->strVal) );
    }

    return 1;
}

// By White Dragon
int Varlist_DeleteByName(Varlist *array, char *theName)
{
    if(!theName || !theName[0])
    {
        return 0;
    }
    if(List_FindByName(array->list, theName))
    {
        Node* node;
        Node* prev_node;
        Node* next_node;

        node = array->list->current;
        prev_node = node->prev;
        next_node = node->next;

        if ( prev_node ) prev_node->next = next_node;
        if ( next_node ) next_node->prev = prev_node;

        if ( array->list->last == node ) array->list->last = prev_node;
        if ( array->list->first == node ) array->list->first = next_node;
        if ( array->list->first == array->list->last && array->list->first == node ) {
            array->list->last = NULL;
            array->list->first = NULL;
        }

        --array->list->size;

        free(node);
    } else return 0;

    return 1;
}

//this function should be called before all script methods, for once
void Script_Global_Init()
{
    memset(&spawnentry, 0, sizeof(spawnentry)); //clear up the spawn entry
    drawmethod = plainmethod;

    Varlist_Init(&global_var_list, max_indexed_vars);

    List_Init(&theFunctionList);
    Script_LoadSystemFunctions();
    List_Init(&scriptheap);
    ImportCache_Init(&theFunctionList);
}

void _freeheapnode(void *ptr)
{
    if(((Script *)ptr)->magic == script_magic)
    {
        Script_Clear((Script *)ptr, 2);
    }
    else if(((anigif_info *)ptr)->magic == anigif_magic)
    {
        anigif_close((anigif_info *)ptr);
    }
    else if(((Varlist *)ptr)->magic == varlist_magic)
    {
        Varlist_Clear((Varlist *)ptr);
    }
    else if(((s_sprite *)ptr)->magic == sprite_magic)
    {
        if(((s_sprite *)ptr)->mask)
        {
            free(((s_sprite *)ptr)->mask);
        }
    }
    free(ptr);
}

//this function should only be called when the engine is shutting down
void Script_Global_Clear()
{
    int i, size;
    List_Clear(&theFunctionList);
    // dump all un-freed variants
    size = List_GetSize(&scriptheap);
    if(size > 0)
    {
        printf("\nWarning: %d script variants are not freed, dumping...\n", size);
    }
    for(i = 0, List_Reset(&scriptheap); i < size; List_GotoNext(&scriptheap), i++)
    {
        printf("%s\n", List_GetName(&scriptheap));
        _freeheapnode(List_Retrieve(&scriptheap));
    }
    List_Clear(&scriptheap);
    // clear the global list
    Varlist_Clear(&global_var_list);

    memset(&spawnentry, 0, sizeof(spawnentry));//clear up the spawn entry
    for(i = 0; i < numfilestreams; i++)
    {
        if(filestreams[i].buf)
        {
            free(filestreams[i].buf);
            filestreams[i].buf = NULL;
        }
    }
    if(filestreams)
    {
        free(filestreams);
    }
    filestreams = NULL;
    numfilestreams = 0;
    ImportCache_Clear();
    StrCache_Clear();
}

int Script_Save_Local_Variant(Script *cs, char *namelist[])
{
    return 0;
}

void Script_Load_Local_Variant(Script *cs, int handle)
{

}

Script *alloc_script()
{
    Script *pscript = calloc(1, sizeof(*pscript));
    pscript->magic = script_magic;
    pscript->varlist = calloc(1, sizeof(*pscript->varlist));
    Varlist_Init(pscript->varlist, max_script_vars);
    return pscript;
}

void Script_Init(Script *pscript, char *theName, char *comment, int first)
{
    if(first)
    {
        memset(pscript, 0, sizeof(*pscript));
        pscript->magic = script_magic;
        pscript->varlist = calloc(1, sizeof(*pscript->varlist));
        Varlist_Init(pscript->varlist, max_script_vars);
    }
    if(!theName || !theName[0])
    {
        return;    // if no name specified, only alloc the variants
    }

    pscript->pinterpreter = malloc(sizeof(*pscript->pinterpreter));
    Interpreter_Init(pscript->pinterpreter, theName, &theFunctionList);
    pscript->interpreterowner = 1; // this is the owner, important
    pscript->initialized = 1;
    if(comment)
    {
        pscript->comment = malloc(sizeof(*pscript->comment) * (strlen(comment) + 1));
        strcpy(pscript->comment, comment);
    }
}

static void execute_init_method(Script *pdest, int iscopy, int localclear)
{
    Script *temp;
    ScriptVariant tempvar;
    //Execute init method
    if(pdest->initialized && pdest->pinterpreter->pInitEntry)
    {
        temp = pcurrentscript;
        pcurrentscript = pdest;

        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        tempvar.lVal = (LONG)iscopy;
        Script_Set_Local_Variant(pdest, "iscopy", &tempvar);
        tempvar.lVal = (LONG)localclear;
        Script_Set_Local_Variant(pdest, "localclear", &tempvar);
        Interpreter_Reset(pdest->pinterpreter);
        pdest->pinterpreter->pCurrentInstruction = pdest->pinterpreter->pInitEntry;
        if(FAILED( Interpreter_EvaluateCall(pdest->pinterpreter)))
        {
            shutdown(1, "Fatal: failed to execute 'init' in script %s %s", pdest->pinterpreter->theSymbolTable.name, pdest->comment ? pdest->comment : "");
        }
        pdest->pinterpreter->bReset = FALSE; // not needed, perhaps
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(pdest, "iscopy", &tempvar);
        Script_Set_Local_Variant(pdest, "localclear", &tempvar);
        pcurrentscript = temp;
    }
}

//safe copy method
void Script_Copy(Script *pdest, Script *psrc, int localclear)
{
    if(!psrc->initialized)
    {
        return;
    }
    if(pdest->initialized)
    {
        Script_Clear(pdest, localclear);
    }
    pdest->pinterpreter = psrc->pinterpreter;
    pdest->comment = psrc->comment;
    pdest->interpreterowner = 0; // dont own it
    pdest->initialized = psrc->initialized; //just copy, it should be 1
    execute_init_method(pdest, 1, localclear);
}

void Script_Clear(Script *pscript, int localclear)
{
    Script *temp;
    Varlist *pvars;

    ScriptVariant tempvar;
    //Execute clear method
    if(pscript->initialized && pscript->pinterpreter->pClearEntry)
    {
        temp = pcurrentscript;
        pcurrentscript = pscript;

        ScriptVariant_Init(&tempvar);
        ScriptVariant_ChangeType(&tempvar, VT_INTEGER);
        tempvar.lVal = (LONG)localclear;
        Script_Set_Local_Variant(pscript, "localclear", &tempvar);
        Interpreter_Reset(pscript->pinterpreter);
        pscript->pinterpreter->pCurrentInstruction = pscript->pinterpreter->pClearEntry;
        if(FAILED( Interpreter_EvaluateCall(pscript->pinterpreter)))
        {
            shutdown(1, "Fatal: failed to execute 'clear' in script %s %s", pscript->pinterpreter->theSymbolTable.name, pscript->comment ? pscript->comment : "");
        }
        pscript->pinterpreter->bReset = FALSE; // not needed, perhaps
        ScriptVariant_Clear(&tempvar);
        Script_Set_Local_Variant(pscript, "localclear", &tempvar);
        pcurrentscript = temp;
    }

    if(localclear && pscript->varlist)
    {
        if(localclear == 2)
        {
            Varlist_Clear(pscript->varlist);
            free(pscript->varlist);
            pscript->varlist = NULL;
        }
        else
        {
            Varlist_Cleanup(pscript->varlist);
        }
    }
    if(!pscript->initialized)
    {
        return;
    }

    //if it is the owner, free the interpreter
    if(pscript->pinterpreter && pscript->interpreterowner)
    {
        Interpreter_Clear(pscript->pinterpreter);
        free(pscript->pinterpreter);
        pscript->pinterpreter = NULL;
        if(pscript->comment)
        {
            free(pscript->comment);
        }
        pscript->comment = NULL;
    }
    pvars = pscript->varlist; // in game clear(localclear!=2) just keep this value
    memset(pscript, 0, sizeof(*pscript));
    pscript->varlist = pvars; // copy it back
}

//append part of the script
//Because the script might not be initialized in 1 time.
int Script_AppendText(Script *pscript, char *text, char *path)
{
    int success;

    //printf(text);
    Interpreter_Reset(pscript->pinterpreter);

    success = SUCCEEDED(Interpreter_ParseText(pscript->pinterpreter, text, 1, path));

    return success;
}

//return name of function from pointer to function
const char *Script_GetFunctionName(void *functionRef)
{
    if (functionRef == ((void *)system_isempty))
    {
        return "isempty";
    }
    else if (functionRef == ((void *)system_exit))
    {
        return "exit";
    }
    else if (functionRef == ((void *)system_NULL))
    {
        return "NULL";
    }
    else if (functionRef == ((void *)system_rand))
    {
        return "rand";
    }
    else if (functionRef == ((void *)system_srand))
    {
        return "srand";
    }
    else if (functionRef == ((void *)system_getglobalvar))
    {
        return "getglobalvar";
    }
    else if (functionRef == ((void *)system_setglobalvar))
    {
        return "setglobalvar";
    }
    else if (functionRef == ((void *)system_getlocalvar))
    {
        return "getlocalvar";
    }
    else if (functionRef == ((void *)system_setlocalvar))
    {
        return "setlocalvar";
    }
    else if (functionRef == ((void *)system_clearglobalvar))
    {
        return "clearglobalvar";
    }
    else if (functionRef == ((void *)system_clearlocalvar))
    {
        return "clearlocalvar";
    }
    else if (functionRef == ((void *)system_free))
    {
        return "free";
    }
    else if (functionRef == ((void *)system_typeof))
    {
        return "typeof";
    }
    else if (functionRef == ((void *)math_sin))
    {
        return "sin";
    }
    else if (functionRef == ((void *)math_ssin))
    {
        return "ssin";
    }
    else if (functionRef == ((void *)math_cos))
    {
        return "cos";
    }
    else if (functionRef == ((void *)math_scos))
    {
        return "scos";
    }
    else if (functionRef == ((void *)math_sqrt))
    {
        return "sqrt";
    }
    else if (functionRef == ((void *)math_pow))
    {
        return "pow";
    }
    else if (functionRef == ((void *)math_asin))
    {
        return "asin";
    }
    else if (functionRef == ((void *)math_acos))
    {
        return "acos";
    }
    else if (functionRef == ((void *)math_atan))
    {
        return "atan";
    }
    else if (functionRef == ((void *)math_trunc))
    {
        return "trunc";
    }
    else if (functionRef == ((void *)math_round))
    {
        return "round";
    }
    else if (functionRef == ((void *)openbor_systemvariant))
    {
        return "openborvariant";
    }
    else if (functionRef == ((void *)openbor_setsystemvariant))
    {
        return "changeopenborvariant";
    }
    else if (functionRef == ((void *)openbor_setsystemvariant))
    {
        return "setopenborvariant";
    }
    else if (functionRef == ((void *)openbor_drawstring))
    {
        return "drawstring";
    }
    else if (functionRef == ((void *)openbor_drawstringtoscreen))
    {
        return "drawstringtoscreen";
    }
    else if (functionRef == ((void *)openbor_log))
    {
        return "log";
    }
    else if (functionRef == ((void *)openbor_drawbox))
    {
        return "drawbox";
    }
    else if (functionRef == ((void *)openbor_drawboxtoscreen))
    {
        return "drawboxtoscreen";
    }
    else if (functionRef == ((void *)openbor_drawline))
    {
        return "drawline";
    }
    else if (functionRef == ((void *)openbor_drawlinetoscreen))
    {
        return "drawlinetoscreen";
    }
    else if (functionRef == ((void *)openbor_drawsprite))
    {
        return "drawsprite";
    }
    else if (functionRef == ((void *)openbor_drawspritetoscreen))
    {
        return "drawspritetoscreen";
    }
    else if (functionRef == ((void *)openbor_drawdot))
    {
        return "drawdot";
    }
    else if (functionRef == ((void *)openbor_drawdottoscreen))
    {
        return "drawdottoscreen";
    }
    else if (functionRef == ((void *)openbor_drawscreen))
    {
        return "drawscreen";
    }
    else if (functionRef == ((void *)openbor_changeplayerproperty))
    {
        return "changeplayerproperty";
    }
    else if (functionRef == ((void *)openbor_getplayerproperty))
    {
        return "getplayerproperty";
    }
    else if (functionRef == ((void *)openbor_changeentityproperty))
    {
        return "changeentityproperty";
    }
    else if (functionRef == ((void *)openbor_getentityproperty))
    {
        return "getentityproperty";
    }
    else if (functionRef == ((void *)openbor_get_animation_property))
    {
        return "get_animation_property";
    }
    else if (functionRef == ((void *)openbor_set_animation_property))
    {
        return "set_animation_property";
    }
    else if (functionRef == ((void *)openbor_get_attack_collection))
    {
        return "get_attack_collection";
    }
    else if (functionRef == ((void *)openbor_get_attack_instance))
    {
        return "get_attack_instance";
    }
    else if (functionRef == ((void *)openbor_get_attack_property))
    {
        return "get_attack_property";
    }
    else if (functionRef == ((void *)openbor_set_attack_property))
    {
        return "set_attack_property";
    }

    // Body collision (bbox)
    else if (functionRef == ((void *)openbor_get_body_collision_collection))
    {
        return "get_body_collision_collection";
    }
    else if (functionRef == ((void *)openbor_get_body_collision_instance))
    {
        return "get_body_collision_instance";
    }
    else if (functionRef == ((void *)openbor_get_body_collision_property))
    {
        return "get_body_collision_property";
    }
    else if (functionRef == ((void *)openbor_set_body_collision_property))
    {
        return "set_body_collision_property";
    }

    else if (functionRef == ((void *)openbor_tossentity))
    {
        return "tossentity";
    }
    else if (functionRef == ((void *)openbor_clearspawnentry))
    {
        return "clearspawnentry";
    }
    else if (functionRef == ((void *)openbor_setspawnentry))
    {
        return "setspawnentry";
    }
    else if (functionRef == ((void *)openbor_spawn))
    {
        return "spawn";
    }
    else if (functionRef == ((void *)openbor_projectile))
    {
        return "projectile";
    }
    else if (functionRef == ((void *)openbor_transconst))
    {
        return "openborconstant";
    }
    else if (functionRef == ((void *)openbor_playmusic))
    {
        return "playmusic";
    }
    else if (functionRef == ((void *)openbor_fademusic))
    {
        return "fademusic";
    }
    else if (functionRef == ((void *)openbor_setmusicvolume))
    {
        return "setmusicvolume";
    }
    else if (functionRef == ((void *)openbor_setmusictempo))
    {
        return "setmusictempo";
    }
    else if (functionRef == ((void *)openbor_pausemusic))
    {
        return "pausemusic";
    }
    else if (functionRef == ((void *)openbor_pausesamples))
    {
        return "pausesamples";
    }
    else if (functionRef == ((void *)openbor_pausesample))
    {
        return "pausesample";
    }
    else if (functionRef == ((void *)openbor_querychannel))
    {
        return "querychannel";
    }
    else if (functionRef == ((void *)openbor_stopchannel))
    {
        return "stopchannel";
    }
    else if (functionRef == ((void *)openbor_isactivesample))
    {
        return "isactivesample";
    }
    else if (functionRef == ((void *)openbor_sampleid))
    {
        return "sampleid";
    }
    else if (functionRef == ((void *)openbor_playsample))
    {
        return "playsample";
    }
    else if (functionRef == ((void *)openbor_loadsample))
    {
        return "loadsample";
    }
    else if (functionRef == ((void *)openbor_unloadsample))
    {
        return "unloadsample";
    }
    else if (functionRef == ((void *)openbor_fadeout))
    {
        return "fadeout";
    }
    else if (functionRef == ((void *)openbor_playerkeys))
    {
        return "playerkeys";
    }
    else if (functionRef == ((void *)openbor_changepalette))
    {
        return "changepalette";
    }
    else if (functionRef == ((void *)openbor_damageentity))
    {
        return "damageentity";
    }
    else if (functionRef == ((void *)openbor_killentity))
    {
        return "killentity";
    }
    else if (functionRef == ((void *)openbor_dograb))
    {
        return "dograb";
    }
    else if (functionRef == ((void *)openbor_findtarget))
    {
        return "findtarget";
    }
    else if (functionRef == ((void *)openbor_checkrange))
    {
        return "checkrange";
    }
    else if (functionRef == ((void *)openbor_gettextobjproperty))
    {
        return "gettextobjproperty";
    }
    else if (functionRef == ((void *)openbor_changetextobjproperty))
    {
        return "changetextobjproperty";
    }
    else if (functionRef == ((void *)openbor_settextobj))
    {
        return "settextobj";
    }
    else if (functionRef == ((void *)openbor_cleartextobj))
    {
        return "cleartextobj";
    }
    else if (functionRef == ((void *)openbor_getlayerproperty))
    {
        return "getlayerproperty";
    }
    else if (functionRef == ((void *)openbor_changelayerproperty))
    {
        return "changelayerproperty";
    }
    else if (functionRef == ((void *)openbor_get_level_property))
    {
        return "get_level_property";
    }
    else if (functionRef == ((void *)openbor_set_level_property))
    {
        return "set_level_property";
    }
    else if (functionRef == ((void *)openbor_get_set_property))
    {
        return "get_set_property";
    }
    else if (functionRef == ((void *)openbor_set_set_property))
    {
        return "set_set_property";
    }
    else if (functionRef == ((void *)openbor_get_set_handle))
    {
        return "get_set_handle";
    }
    else if (functionRef == ((void *)openbor_get_layer_handle))
    {
        return "get_layer_handle";
    }
    else if (functionRef == ((void *)openbor_changelevelproperty))
    {
        return "changelevelproperty";
    }
    else if (functionRef == ((void *)openbor_checkhole))
    {
        return "checkhole";
    }
    else if (functionRef == ((void *)openbor_checkholeindex))
    {
        return "checkholeindex";
    }
    else if (functionRef == ((void *)openbor_checkwall))
    {
        return "checkwall";
    }
    else if (functionRef == ((void *)openbor_checkwallindex))
    {
        return "checkwallindex";
    }
    else if (functionRef == ((void *)openbor_checkplatformbelow))
    {
        return "checkplatformbelow";
    }
    else if (functionRef == ((void *)openbor_checkplatformabove))
    {
        return "checkplatformabove";
    }
    else if (functionRef == ((void *)openbor_checkplatformbetween))
    {
        return "checkplatformbetween";
    }
    else if (functionRef == ((void *)openbor_checkbasemap))
    {
        return "checkbasemap";
    }
    else if (functionRef == ((void *)openbor_checkbasemapindex))
    {
        return "checkbasemapindex";
    }
    else if (functionRef == ((void *)openbor_generatebasemap))
    {
        return "generatebasemap";
    }
    else if (functionRef == ((void *)openbor_openfilestream))
    {
        return "openfilestream";
    }
    else if (functionRef == ((void *)openbor_getfilestreamline))
    {
        return "getfilestreamline";
    }
    else if (functionRef == ((void *)openbor_getfilestreamargument))
    {
        return "getfilestreamargument";
    }
    else if (functionRef == ((void *)openbor_filestreamnextline))
    {
        return "filestreamnextline";
    }
    else if (functionRef == ((void *)openbor_getfilestreamposition))
    {
        return "getfilestreamposition";
    }
    else if (functionRef == ((void *)openbor_setfilestreamposition))
    {
        return "setfilestreamposition";
    }
    else if (functionRef == ((void *)openbor_filestreamappend))
    {
        return "filestreamappend";
    }
    else if (functionRef == ((void *)openbor_createfilestream))
    {
        return "createfilestream";
    }
    else if (functionRef == ((void *)openbor_closefilestream))
    {
        return "closefilestream";
    }
    else if (functionRef == ((void *)openbor_savefilestream))
    {
        return "savefilestream";
    }
    else if (functionRef == ((void *)openbor_getindexedvar))
    {
        return "getindexedvar";
    }
    else if (functionRef == ((void *)openbor_setindexedvar))
    {
        return "setindexedvar";
    }
    else if (functionRef == ((void *)openbor_getscriptvar))
    {
        return "getscriptvar";
    }
    else if (functionRef == ((void *)openbor_setscriptvar))
    {
        return "setscriptvar";
    }
    else if (functionRef == ((void *)openbor_getentityvar))
    {
        return "getentityvar";
    }
    else if (functionRef == ((void *)openbor_setentityvar))
    {
        return "setentityvar";
    }
    else if (functionRef == ((void *)openbor_shutdown))
    {
        return "shutdown";
    }
    else if (functionRef == ((void *)openbor_jumptobranch))
    {
        return "jumptobranch";
    }
    else if (functionRef == ((void *)openbor_changelight))
    {
        return "changelight";
    }
    else if (functionRef == ((void *)openbor_changeshadowcolor))
    {
        return "changeshadowcolor";
    }
    else if (functionRef == ((void *)openbor_bindentity))
    {
        return "bindentity";
    }
    else if (functionRef == ((void *)openbor_array))
    {
        return "array";
    }
    else if (functionRef == ((void *)openbor_size))
    {
        return "size";
    }
    else if (functionRef == ((void *)openbor_get))
    {
        return "get";
    }
    else if (functionRef == ((void *)openbor_set))
    {
        return "set";
    }
    else if (functionRef == ((void *)openbor_delete))
    {
        return "delete";
    }
    else if (functionRef == ((void *)openbor_add))
    {
        return "add";
    }
    else if (functionRef == ((void *)openbor_reset))
    {
        return "reset";
    }
    else if (functionRef == ((void *)openbor_next))
    {
        return "next";
    }
    else if (functionRef == ((void *)openbor_previous))
    {
        return "previous";
    }
    else if (functionRef == ((void *)openbor_key))
    {
        return "key";
    }
    else if (functionRef == ((void *)openbor_value))
    {
        return "value";
    }
    else if (functionRef == ((void *)openbor_islast))
    {
        return "islast";
    }
    else if (functionRef == ((void *)openbor_isfirst))
    {
        return "isfirst";
    }
    else if (functionRef == ((void *)openbor_allocscreen))
    {
        return "allocscreen";
    }
    else if (functionRef == ((void *)openbor_clearscreen))
    {
        return "clearscreen";
    }
    else if (functionRef == ((void *)openbor_setdrawmethod))
    {
        return "setdrawmethod";
    }
    else if (functionRef == ((void *)openbor_changedrawmethod))
    {
        return "changedrawmethod";
    }
    else if (functionRef == ((void *)openbor_getdrawmethod))
    {
        return "getdrawmethod";
    }
    else if (functionRef == ((void *)openbor_updateframe))
    {
        return "updateframe";
    }
    else if (functionRef == ((void *)openbor_performattack))
    {
        return "performattack";
    }
    else if (functionRef == ((void *)openbor_executeanimation))
    {
        return "executeanimation";
    }
    else if (functionRef == ((void *)openbor_setidle))
    {
        return "setidle";
    }
    else if (functionRef == ((void *)openbor_getentity))
    {
        return "getentity";
    }
    else if (functionRef == ((void *)openbor_hallfame))
    {
        return "hallfame";
    }
    else if (functionRef == ((void *)openbor_loadmodel))
    {
        return "loadmodel";
    }
    else if (functionRef == ((void *)openbor_loadsprite))
    {
        return "loadsprite";
    }
    else if (functionRef == ((void *)openbor_menu_options))
    {
        return "options";
    }
    else if (functionRef == ((void *)openbor_playwebm))
    {
        return "playwebm";
    }
    else if (functionRef == ((void *)openbor_playgif))
    {
        return "playgif";
    }
    else if (functionRef == ((void *)openbor_openanigif))
    {
        return "openanigif";
    }
    else if (functionRef == ((void *)openbor_decodeanigif))
    {
        return "decodeanigif";
    }
    else if (functionRef == ((void *)openbor_getanigifinfo))
    {
        return "getanigifinfo";
    }
    else if (functionRef == ((void *)openbor_strinfirst))
    {
        return "strinfirst";
    }
    else if (functionRef == ((void *)openbor_strinlast))
    {
        return "strinlast";
    }
    else if (functionRef == ((void *)openbor_strleft))
    {
        return "strleft";
    }
    else if (functionRef == ((void *)openbor_strlength))
    {
        return "strlength";
    }
    else if (functionRef == ((void *)openbor_strwidth))
    {
        return "strwidth";
    }
    else if (functionRef == ((void *)openbor_strright))
    {
        return "strright";
    }
    else if (functionRef == ((void *)openbor_getmodelproperty))
    {
        return "getmodelproperty";
    }
    else if (functionRef == ((void *)openbor_changemodelproperty))
    {
        return "changemodelproperty";
    }
    else if (functionRef == ((void *)openbor_rgbcolor))
    {
        return "rgbcolor";
    }
    else if (functionRef == ((void *)openbor_adjustwalkanimation))
    {
        return "adjustwalkanimation";
    }
    else if (functionRef == ((void *)openbor_finditem))
    {
        return "finditem";
    }
    else if (functionRef == ((void *)openbor_pickup))
    {
        return "pickup";
    }
    else if (functionRef == ((void *)openbor_waypoints))
    {
        return "waypoints";
    }
    else if (functionRef == ((void *)openbor_drawspriteq))
    {
        return "drawspriteq";
    }
    else if (functionRef == ((void *)openbor_clearspriteq))
    {
        return "clearspriteq";
    }
    else if (functionRef == ((void *)openbor_getgfxproperty))
    {
        return "getgfxproperty";
    }
    else if (functionRef == ((void *)openbor_allocscript))
    {
        return "allocscript";
    }
    else if (functionRef == ((void *)openbor_loadscript))
    {
        return "loadscript";
    }
    else if (functionRef == ((void *)openbor_compilescript))
    {
        return "compilescript";
    }
    else if (functionRef == ((void *)openbor_executescript))
    {
        return "executescript";
    }
    else if (functionRef == ((void *)openbor_loadgamefile))
    {
        return "loadgamefile";
    }
    else if (functionRef == ((void *)openbor_finishlevel))
    {
        return "finishlevel";
    }
    else if (functionRef == ((void *)openbor_gotomainmenu))
    {
        return "gotomainmenu";
    }
    else if (functionRef == ((void *)openbor_playgame))
    {
        return "playgame";
    }
    else if (functionRef == ((void *)openbor_getrecordingstatus))
    {
        return "getrecordingstatus";
    }
    else if (functionRef == ((void *)openbor_recordinputs))
    {
        return "recordinputs";
    }
    else if (functionRef == ((void *)openbor_getsaveinfo))
    {
        return "getsaveinfo";
    }
    else
    {
        return "<unknown function>";
    }
}

//return string mapping function corresponding to a given function
void *Script_GetStringMapFunction(void *functionRef)
{
    if (functionRef == ((void *)openbor_systemvariant))
    {
        return (void *)mapstrings_systemvariant;
    }
    else if (functionRef == ((void *)openbor_setsystemvariant))
    {
        return (void *)mapstrings_systemvariant;
    }
    else if (functionRef == ((void *)openbor_getentityproperty))
    {
        return (void *)mapstrings_entityproperty;
    }
    else if (functionRef == ((void *)openbor_changeentityproperty))
    {
        return (void *)mapstrings_entityproperty;
    }
    else if (functionRef == ((void *)openbor_getplayerproperty))
    {
        return (void *)mapstrings_playerproperty;
    }
    else if (functionRef == ((void *)openbor_changeplayerproperty))
    {
        return (void *)mapstrings_playerproperty;
    }
    else if (functionRef == ((void *)openbor_setspawnentry))
    {
        return (void *)mapstrings_setspawnentry;
    }
    else if (functionRef == ((void *)openbor_transconst))
    {
        return (void *)mapstrings_transconst;
    }
    else if (functionRef == ((void *)openbor_playerkeys))
    {
        return (void *)mapstrings_playerkeys;
    }
    else if (functionRef == ((void *)openbor_gettextobjproperty))
    {
        return (void *)mapstrings_textobjproperty;
    }
    else if (functionRef == ((void *)openbor_changetextobjproperty))
    {
        return (void *)mapstrings_textobjproperty;
    }
    else if (functionRef == ((void *)openbor_getlayerproperty))
    {
        return (void *)mapstrings_layerproperty;
    }
    else if (functionRef == ((void *)openbor_changelayerproperty))
    {
        return (void *)mapstrings_layerproperty;
    }
    else if (functionRef == ((void *)openbor_changedrawmethod))
    {
        return (void *)mapstrings_drawmethodproperty;
    }
    else if (functionRef == ((void *)openbor_getgfxproperty))
    {
        return (void *)mapstrings_gfxproperty;
    }
    else if (functionRef == ((void *)openbor_getlevelproperty))
    {
        return (void *)mapstrings_levelproperty;
    }
    else if (functionRef == ((void *)openbor_changelevelproperty))
    {
        return (void *)mapstrings_levelproperty;
    }
    else
    {
        return NULL;
    }
}

/* Replace string constants with enum constants at compile time to speed up
   script execution. */
int Script_MapStringConstants(Instruction *pInstruction)
{
    ScriptVariant **params;
    int paramCount;
    int (*pMapstrings)(ScriptVariant **, int);

    if(pInstruction->functionRef)
    {
        params = (ScriptVariant **)pInstruction->theRefList->solidlist;
        paramCount = (int)pInstruction->theRef->lVal;
        assert(paramCount <= 32);
        // Get the pointer to the correct mapstrings function, if one exists.
        pMapstrings = Script_GetStringMapFunction(pInstruction->functionRef);
        if(pMapstrings)
        {
            // Call the mapstrings function.
            if(!pMapstrings(params, paramCount))
            {
                return 0;
            }
        }
    }

    return 1;
}

//should be called only once after parsing text
int Script_Compile(Script *pscript)
{
    int result;
    if(!pscript || !pscript->pinterpreter)
    {
        return 1;
    }
    //Interpreter_OutputPCode(pscript->pinterpreter, "code");
    result = SUCCEEDED(Interpreter_CompileInstructions(pscript->pinterpreter));
    if(!result)
    {
        shutdown(1, "Can't compile script '%s' %s\n", pscript->pinterpreter->theSymbolTable.name, pscript->comment ? pscript->comment : "");
    }

    pscript->pinterpreter->bReset = FALSE;
    execute_init_method(pscript, 0, 1);
    return result;
}

int Script_IsInitialized(Script *pscript)
{
    //if(pscript && pscript->initialized) pcurrentscript = pscript; //used by local script functions
    return pscript->initialized;
}

//execute the script
int Script_Execute(Script *pscript)
{
    int result, nested;
    extern int no_cmd_compatible;
    Script *temp = pcurrentscript;
    Interpreter tinter, *pinter;
    pcurrentscript = pscript; //used by local script functions
    nested = pscript->pinterpreter->bReset;
    if(no_nested_script && nested)
    {
        result = 1;
    }
    else
    {
        pinter = pscript->pinterpreter;
        if(nested && no_cmd_compatible)
        {
            tinter = *pinter;
        }
        Interpreter_Reset(pinter);
        result = (int)SUCCEEDED(Interpreter_EvaluateImmediate(pinter));
        if(nested && no_cmd_compatible)
        {
            *pinter = tinter;
        }
        else if(nested)
        {
            pinter->bReset = FALSE;
        }
    }
    pcurrentscript = temp;
    if(!result)
    {
        shutdown(1, "There's an exception while executing script '%s' %s", pscript->pinterpreter->theSymbolTable.name, pscript->comment ? pscript->comment : "");
    }
    return result;
}

#ifndef COMPILED_SCRIPT
//this method is for debug purpose
int Script_Call(Script *pscript, char *method, ScriptVariant *pretvar)
{
    int result;
    Script *temp = pcurrentscript;
    pcurrentscript = pscript; //used by local script functions
    Interpreter_Reset(pscript->pinterpreter);
    result = (int)SUCCEEDED(Interpreter_Call(pscript->pinterpreter, method, pretvar));
    pcurrentscript = temp;
    return result;
}
#endif

//used by Script_Global_Init
void Script_LoadSystemFunctions()
{
    //printf("Loading system script functions....");
    //load system functions if we need
    List_Reset(&theFunctionList);

    List_InsertAfter(&theFunctionList,
                     (void *)system_isempty, "isempty");
    List_InsertAfter(&theFunctionList,
                     (void *)system_exit, "exit");
    List_InsertAfter(&theFunctionList,
                     (void *)system_NULL, "NULL");
    List_InsertAfter(&theFunctionList,
                     (void *)system_rand, "rand");
    List_InsertAfter(&theFunctionList,
                     (void *)system_srand, "srand");
    List_InsertAfter(&theFunctionList,
                     (void *)system_getglobalvar, "getglobalvar");
    List_InsertAfter(&theFunctionList,
                     (void *)system_setglobalvar, "setglobalvar");
    List_InsertAfter(&theFunctionList,
                     (void *)system_getlocalvar, "getlocalvar");
    List_InsertAfter(&theFunctionList,
                     (void *)system_setlocalvar, "setlocalvar");
    List_InsertAfter(&theFunctionList,
                     (void *)system_clearglobalvar, "clearglobalvar");
    List_InsertAfter(&theFunctionList,
                     (void *)system_clearlocalvar, "clearlocalvar");
    List_InsertAfter(&theFunctionList,
                     (void *)system_free, "free");
    List_InsertAfter(&theFunctionList,
                     (void *)system_typeof, "typeof");
    List_InsertAfter(&theFunctionList,
                     (void *)math_sin, "sin");
    List_InsertAfter(&theFunctionList,
                     (void *)math_ssin, "ssin");
    List_InsertAfter(&theFunctionList,
                     (void *)math_cos, "cos");
    List_InsertAfter(&theFunctionList,
                     (void *)math_scos, "scos");
    List_InsertAfter(&theFunctionList,
                     (void *)math_sqrt, "sqrt");
    List_InsertAfter(&theFunctionList,
                     (void *)math_pow, "pow");
    List_InsertAfter(&theFunctionList,
                     (void *)math_asin, "asin");
    List_InsertAfter(&theFunctionList,
                     (void *)math_acos, "acos");
    List_InsertAfter(&theFunctionList,
                     (void *)math_atan, "atan");
    List_InsertAfter(&theFunctionList,
                     (void *)math_trunc, "trunc");
    List_InsertAfter(&theFunctionList,
                     (void *)math_round, "round");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_systemvariant, "openborvariant");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_setsystemvariant, "changeopenborvariant");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_setsystemvariant, "setopenborvariant");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_drawstring, "drawstring");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_drawstringtoscreen, "drawstringtoscreen");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_log, "log");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_drawbox, "drawbox");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_drawboxtoscreen, "drawboxtoscreen");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_drawline, "drawline");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_drawlinetoscreen, "drawlinetoscreen");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_drawsprite, "drawsprite");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_drawspritetoscreen, "drawspritetoscreen");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_drawdot, "drawdot");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_drawdottoscreen, "drawdottoscreen");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_drawscreen, "drawscreen");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_changeplayerproperty, "changeplayerproperty");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_changeentityproperty, "changeentityproperty");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_getplayerproperty, "getplayerproperty");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_get_animation_property, "get_animation_property");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_set_animation_property, "set_animation_property");

    // Attack properties
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_get_attack_collection, "get_attack_collection");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_get_attack_instance, "get_attack_instance");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_get_attack_property, "get_attack_property");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_set_attack_property, "set_attack_property");

    // Body collision (bbox) properties.
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_get_body_collision_collection, "get_body_collision_collection");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_get_body_collision_instance, "get_body_collision_instance");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_get_body_collision_property, "get_body_collision_property");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_set_body_collision_property, "set_body_collision_property");

    List_InsertAfter(&theFunctionList,
                     (void *)openbor_getentityproperty, "getentityproperty");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_tossentity, "tossentity");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_clearspawnentry, "clearspawnentry");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_setspawnentry, "setspawnentry");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_spawn, "spawn");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_projectile, "projectile");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_transconst, "openborconstant");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_playmusic, "playmusic");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_fademusic, "fademusic");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_setmusicvolume, "setmusicvolume");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_setmusictempo, "setmusictempo");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_pausemusic, "pausemusic");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_pausesamples, "pausesamples");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_pausesample, "pausesample");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_querychannel, "querychannel");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_stopchannel, "stopchannel");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_isactivesample, "isactivesample");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_sampleid, "sampleid");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_playsample, "playsample");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_loadsample, "loadsample");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_unloadsample, "unloadsample");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_fadeout, "fadeout");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_playerkeys, "playerkeys");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_changepalette, "changepalette");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_damageentity, "damageentity");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_killentity, "killentity");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_dograb, "dograb");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_findtarget, "findtarget");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_checkrange, "checkrange");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_gettextobjproperty, "gettextobjproperty");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_changetextobjproperty, "changetextobjproperty");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_settextobj, "settextobj");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_cleartextobj, "cleartextobj");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_getlayerproperty, "getlayerproperty");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_changelayerproperty, "changelayerproperty");

    // 2017-04-25, DC
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_get_level_property, "get_level_property");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_set_level_property, "set_level_property");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_get_set_property, "get_set_property");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_set_set_property, "set_set_property");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_get_set_handle, "get_set_handle");

    // 2017-04-27, DC, Layers
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_get_layer_handle, "get_layer_handle");

    List_InsertAfter(&theFunctionList,
                     (void *)openbor_getlevelproperty, "getlevelproperty");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_changelevelproperty, "changelevelproperty");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_checkhole, "checkhole");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_checkholeindex, "checkholeindex");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_checkwall, "checkwall");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_checkholeindex, "checkwallindex");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_checkplatformbelow, "checkplatformbelow");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_checkplatformabove, "checkplatformabove");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_checkplatformbetween, "checkplatformbetween");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_checkbasemap, "checkbasemap");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_checkbasemap, "checkbasemapindex");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_generatebasemap, "generatebasemap");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_openfilestream, "openfilestream");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_getfilestreamline, "getfilestreamline");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_getfilestreamargument, "getfilestreamargument");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_filestreamnextline, "filestreamnextline");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_getfilestreamposition, "getfilestreamposition");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_setfilestreamposition, "setfilestreamposition");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_filestreamappend, "filestreamappend");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_createfilestream, "createfilestream");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_closefilestream, "closefilestream");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_savefilestream, "savefilestream");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_getindexedvar, "getindexedvar");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_setindexedvar, "setindexedvar");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_getscriptvar, "getscriptvar");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_setscriptvar, "setscriptvar");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_getentityvar, "getentityvar");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_setentityvar, "setentityvar");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_shutdown, "shutdown");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_jumptobranch, "jumptobranch");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_changelight, "changelight");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_changeshadowcolor, "changeshadowcolor");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_bindentity, "bindentity");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_array, "array");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_size, "size");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_get, "get");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_set, "set");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_delete, "delete");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_add, "add");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_reset, "reset");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_next, "next");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_previous, "previous");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_key, "key");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_value, "value");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_islast, "islast");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_isfirst, "isfirst");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_allocscreen, "allocscreen");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_clearscreen, "clearscreen");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_setdrawmethod, "setdrawmethod");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_changedrawmethod, "changedrawmethod");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_getdrawmethod, "getdrawmethod");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_updateframe, "updateframe");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_performattack, "performattack");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_executeanimation, "executeanimation");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_setidle, "setidle");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_getentity, "getentity");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_loadmodel, "loadmodel");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_loadsprite, "loadsprite");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_hallfame, "hallfame");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_menu_options, "options");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_playwebm, "playwebm");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_playgif, "playgif");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_openanigif, "openanigif");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_decodeanigif, "decodeanigif");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_getanigifinfo, "getanigifinfo");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_strinfirst, "strinfirst");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_strinlast, "strinlast");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_strleft, "strleft");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_strlength, "strlength");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_strwidth, "strwidth");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_strright, "strright");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_getmodelproperty, "getmodelproperty");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_changemodelproperty, "changemodelproperty");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_rgbcolor, "rgbcolor");

    List_InsertAfter(&theFunctionList,
                     (void *)openbor_adjustwalkanimation, "adjustwalkanimation");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_finditem, "finditem");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_pickup, "pickup");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_waypoints, "waypoints");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_drawspriteq, "drawspriteq");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_clearspriteq, "clearspriteq");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_getgfxproperty, "getgfxproperty");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_allocscript, "allocscript");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_loadscript, "loadscript");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_compilescript, "compilescript");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_executescript, "executescript");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_loadgamefile, "loadgamefile");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_finishlevel, "finishlevel");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_gotomainmenu, "gotomainmenu");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_playgame, "playgame");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_getrecordingstatus, "getrecordingstatus");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_recordinputs, "recordinputs");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_getsaveinfo, "getsaveinfo");

    //printf("Done!\n");

}

#endif // SCRIPT_COMMON
