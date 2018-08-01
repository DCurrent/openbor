/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2017 OpenBOR Team
 */

/* This file include all script methods used by openbor engine

 Notice: Make sure to null *pretvar when you about to return E_FAIL,
		 Or the engine might crash.

 Notice: Every new ScriptVariant must be initialized when you first alloc it by
		 ScriptVariant_Init immediately, memset it all to zero should also work by now,
		 unless VT_EMPTY is changed.

		 If you want to reset a ScriptVariant to empty, you must use ScriptVariant_Clear instead.
		 ScriptVariant_Init or memset must be called only ONCE, later you should use ScriptVariant_Clear.

		 Be sure to call ScriptVariant_Clear if you want to use free to delete those variants.

		 If you want to copy a ScriptVariant from another, use ScriptVariant_Copy instead of assignment,
		 not because it is faster, but this method is neccessary for string types.

		 If you want to change types of an ScriptVariant, use ScriptVariant_ChangeType, don't change vt directly.

*/

#include "config.h"
#include "openbor.h"
#include "soundmix.h"
#include "globals.h"
#include "ImportCache.h"
#include "models.h"
#include "scriptcommon.h"

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

extern int  ent_count;
extern int  ent_max;
extern int  gameOver;
extern int  selectScreen;
extern int	titleScreen;
extern int  hallOfFame;
extern int	showComplete;
extern char	*currentScene;
extern int  enginecreditsScreen;
extern int	menuScreen;
extern int  optionsMenu;
extern int	controloptionsMenu;
extern int	videooptionsMenu;
extern int  soundoptionsMenu;
extern int	systemoptionsMenu;
extern int	startgameMenu;
extern int  newgameMenu;
extern int  loadgameMenu;
extern int  num_difficulties;
extern u32  _time;
extern int goto_mainmenu_flag;
extern int _pause;
extern int timeleft;
extern int gfx_x_offset;
extern int gfx_y_offset;
extern int gfx_y_offset_adj;
extern s_lasthit lasthit;
extern int current_set;
extern int current_level;
extern int current_stage;
extern int timevar;
extern float   bgtravelled;
extern float   vbgtravelled;
extern int traveltime;
extern int texttime;
extern int timetoshow;
extern int is_total_timeover;
extern int showgo;
extern float   advancex;
extern float   advancey;
extern float   scrolldx;
extern float   scrolldy;
extern float   scrollminz;
extern float   scrollmaxz;
extern float   blockade;
extern float   scrollminx;
extern float   scrollmaxx;
extern s_videomodes videomodes;
extern int  panel_height;
extern char branch_name[MAX_NAME_LEN + 1];
extern s_set_entry *levelsets;
extern unsigned int models_loaded;
//extern unsigned int   models_cached;
extern int viewportx;
extern int viewporty;
extern int viewportw;
extern int viewporth;
extern int nosave;
extern int nopause;
extern int nofadeout;
extern int noscreenshot;
extern int nojoin;
extern s_screen *vscreen;
extern entity *smartbomber;
extern entity *textbox;
extern s_screen *background;
extern int skiptoset;
extern s_slow_motion    slowmotion;
extern int shadowcolor;
extern int shadowalpha;
extern int shadowopacity;
extern s_axis_plane_vertical_int light;
extern int max_attack_types;
extern int max_animations;

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
            borShutdown(1, "Fatal: failed to execute 'init' in script %s %s", pdest->pinterpreter->theSymbolTable.name, pdest->comment ? pdest->comment : "");
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
            borShutdown(1, "Fatal: failed to execute 'clear' in script %s %s", pscript->pinterpreter->theSymbolTable.name, pscript->comment ? pscript->comment : "");
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
        borShutdown(1, "Can't compile script '%s' %s\n", pscript->pinterpreter->theSymbolTable.name, pscript->comment ? pscript->comment : "");
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
        borShutdown(1, "There's an exception while executing script '%s' %s", pscript->pinterpreter->theSymbolTable.name, pscript->comment ? pscript->comment : "");
    }
    return result;
}

static s_collision_attack attack;

//////////////////////////////////////////////////////////
////////////   system functions
//////////////////////////////////////////////////////////
//isempty(var);
HRESULT system_isempty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    if(paramCount != 1)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    (*pretvar)->lVal = (LONG)( (varlist[0])->vt == VT_EMPTY );

    return S_OK;
}
//NULL();
HRESULT system_NULL(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    ScriptVariant_Clear(*pretvar);

    return S_OK;
}
HRESULT system_exit(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    *pretvar = NULL;
    pcurrentscript->pinterpreter->bReset = FALSE;
    return S_OK;
}
HRESULT system_rand(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)rand32();
    return S_OK;
}
HRESULT system_srand(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ltemp;

    *pretvar = NULL;
    if(paramCount != 1)
    {
        return E_FAIL;
    }
    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ltemp)))
    {
        return E_FAIL;
    }

    srand32(ltemp);
    return S_OK;
}
//getglobalvar(varname);
HRESULT system_getglobalvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ltemp;
    ScriptVariant *ptmpvar;

    if(paramCount != 1)
    {
        goto ggv_error;
    }

    if(varlist[0]->vt == VT_STR)
    {
        ptmpvar = Varlist_GetByName(&global_var_list, StrCache_Get(varlist[0]->strVal));
    }
    else if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[0], &ltemp)))
    {
        ptmpvar = Varlist_GetByIndex(&global_var_list, (LONG)ltemp);
    }
    else
    {
        goto ggv_error;
    }

    if(ptmpvar)
    {
        ScriptVariant_Copy(*pretvar,  ptmpvar);
    }
    else
    {
        ScriptVariant_Clear(*pretvar);
    }
    return S_OK;

ggv_error:
    *pretvar = NULL;
    return E_FAIL;
}
//setglobalvar(varname, value);
HRESULT system_setglobalvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ltemp;
    if(paramCount < 2)
    {
        goto sgv_error;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    if(varlist[0]->vt == VT_STR)
    {
        (*pretvar)->lVal = (LONG)Varlist_SetByName(&global_var_list, StrCache_Get(varlist[0]->strVal), varlist[1]);
    }
    else if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[0], &ltemp)))
    {
        (*pretvar)->lVal = (LONG)Varlist_SetByIndex(&global_var_list, (LONG)ltemp, varlist[1]);
    }
    else
    {
        goto sgv_error;
    }

    return S_OK;
sgv_error:
    *pretvar = NULL;
    return E_FAIL;
}
//getlocalvar(varname);
HRESULT system_getlocalvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ltemp;
    ScriptVariant *ptmpvar;

    if(paramCount != 1)
    {
        goto glv_error;
    }

    if(varlist[0]->vt == VT_STR)
    {
        ptmpvar = Varlist_GetByName(pcurrentscript->varlist, StrCache_Get(varlist[0]->strVal));
    }
    else if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[0], &ltemp)))
    {
        ptmpvar = Varlist_GetByIndex(pcurrentscript->varlist, (LONG)ltemp);
    }
    else
    {
        goto glv_error;
    }

    if(ptmpvar)
    {
        ScriptVariant_Copy(*pretvar,  ptmpvar);
    }
    else
    {
        ScriptVariant_Clear(*pretvar);
    }
    return S_OK;

glv_error:
    *pretvar = NULL;
    return E_FAIL;
}
//setlocalvar(varname, value);
HRESULT system_setlocalvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ltemp;
    if(paramCount < 2)
    {
        goto slv_error;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    if(varlist[0]->vt == VT_STR)
    {
        (*pretvar)->lVal = (LONG)Varlist_SetByName(pcurrentscript->varlist, StrCache_Get(varlist[0]->strVal), varlist[1]);
    }
    else if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[0], &ltemp)))
    {
        (*pretvar)->lVal = (LONG)Varlist_SetByIndex(pcurrentscript->varlist, (LONG)ltemp, varlist[1]);
    }
    else
    {
        goto slv_error;
    }

    return S_OK;
slv_error:
    *pretvar = NULL;
    return E_FAIL;
}
//clearlocalvar();
HRESULT system_clearlocalvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    *pretvar = NULL;
    Varlist_Cleanup(pcurrentscript->varlist);
    return S_OK;
}
//clearglobalvar();
HRESULT system_clearglobalvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    *pretvar = NULL;
    Varlist_Cleanup(&global_var_list);
    return S_OK;
}

//free();
HRESULT system_free(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    *pretvar = NULL;
    if(paramCount < 1)
    {
        return E_FAIL;
    }
    if(List_Includes(&scriptheap, varlist[0]->ptrVal))
    {
        _freeheapnode(List_Retrieve(&scriptheap));
        // a script's ondestroy() may free something else and change the list
        // position, so set the position to this variant again
        List_Includes(&scriptheap, varlist[0]->ptrVal);
        List_Remove(&scriptheap);
        return S_OK;
    }
    return E_FAIL;
}

//typeof(v);
HRESULT system_typeof(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    if(paramCount < 1)
    {
        *pretvar = NULL;
        return E_FAIL;
    }
    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)varlist[0]->vt;
    return S_OK;
}

//////////////////////////////////////////////////////////
////////////   openbor functions
//////////////////////////////////////////////////////////

//check openborscript.h for systemvariant_enum

// arranged list, for searching
static const char *svlist[] =
{
    "background",
    "blockade",
    "bossescount",
    "branchname",
    "cheats",
    "count_enemies",
    "count_entities",
    "count_npcs",
    "count_players",
    "current_branch",
    "current_level",
    "current_palette",
    "current_scene",
    "current_set",
    "current_stage",
    "effectvol",
    "elapsed_time",
    "ent_max",
    "fps",
    "freeram",
    "game_paused",
    "game_speed",
    "game_time",
    "gfx_x_offset",
    "gfx_y_offset",
    "gfx_y_offset_adj",
    "hresolution",
    "in_cheat_options",
    "in_control_options",
    "in_enginecreditsscreen",
    "in_gameoverscreen",
    "in_halloffamescreen",
    "in_level",
    "in_load_game",
    "in_menuscreen",
    "in_new_game",
    "in_options",
    "in_selectscreen",
    "in_showcomplete",
    "in_sound_options",
    "in_start_game",
    "in_system_options",
    "in_titlescreen",
    "in_video_options",
    "lasthita",
    "lasthitc",
    "lasthitt",
    "lasthitx",
    "lasthity",
    "lasthitz",
    "levelheight",
    "levelpos",
    "levelwidth",
    "lightx",
    "lightz",
    "maxanimations",
    "maxattacktypes",
    "maxentityvars",
    "maxglobalvars",
    "maxindexedvars",
    "maxplayers",
    "maxscriptvars",
    "maxsoundchannels",
    "models_cached",
    "models_loaded",
    "musicvol",
    "nofadeout",
    "nogameover",
    "nohof",
    "nojoin",
    "nopause",
    "nosave",
    "noscreenshot",
    "noshowcomplete",
    "numbasemaps",
    "numbosses",
    "numholes",
    "numlayers",
    "numpalettes",
    "numwalls",
    "pakname",
    "pause",
    "pixelformat",
    "player",
    "player1",
    "player2",
    "player3",
    "player4",
    "player_max_z",
    "player_min_z",
    "porting",
    "sample_play_id",
    "scrollmaxx",
    "scrollmaxz",
    "scrollminx",
    "scrollminz",
    "self",
    "sets_count",
    "shadowalpha",
    "shadowcolor",
    "shadowopacity",
    "skiptoset",
    "slowmotion",
    "slowmotion_duration",
    "smartbomber",
    "soundvol",
    "textbox",
    "ticks",
    "totalram",
    "usedram",
    "viewporth",
    "viewportw",
    "viewportx",
    "viewporty",
    "vresolution",
    "vscreen",
    "waiting",
    "xpos",
    "ypos",
};


// ===== openborvariant =====
int mapstrings_systemvariant(ScriptVariant **varlist, int paramCount)
{
    char *propname;
    int prop;


    MAPSTRINGS(varlist[0], svlist, _sv_the_end,
               "openborvariant: System variable name not found: '%s'\n");

    return 1;
}

//sample function, used for getting a system variant
//openborvariant(varname);
HRESULT openbor_systemvariant(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    //used for getting the index from the enum of properties
    int variantindex = -1;
    //the paramCount used for checking.
    //check it first so the engine wont crash if the list is empty
    if(paramCount != 1)
    {
        goto systemvariant_error;
    }
    //call this function's mapstrings function to map string constants to enum values
    mapstrings_systemvariant(varlist, paramCount);
    //the variant name should be here
    //you can check the argument type if you like
    if(varlist[0]->vt == VT_INTEGER)
    {
        variantindex = varlist[0]->lVal;
    }
    else
    {
        goto systemvariant_error;
    }
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


//used for changing a system variant
//setopenborvariant(varname, value);
HRESULT openbor_setsystemvariant(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    //used for getting the enum constant corresponding to the desired variable
    int variantindex = 0;
    //reference to the arguments
    ScriptVariant *arg = NULL;
    //the paramCount used for checking.
    //check it first so the engine wont crash if the list is empty
    if(paramCount != 2)
    {
        goto changesystemvariant_error;
    }
    // map string constants to enum constants for speed
    mapstrings_systemvariant(varlist, paramCount);
    //get the 1st argument
    arg = varlist[0];
    //the variant name should be here
    //you can check the argument type if you like
    if(arg->vt == VT_INTEGER)
    {
        variantindex = arg->lVal;
    }
    else
    {
        goto changesystemvariant_error;
    }

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
HRESULT openbor_drawstring(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    int i;
    char buf[MAX_BUFFER_LEN];
    LONG value[4];
    *pretvar = NULL;

    if(paramCount < 4)
    {
        goto drawstring_error;
    }

    for(i = 0; i < 3; i++)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value + i)))
        {
            goto drawstring_error;
        }
    }
    if(paramCount > 4)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[4], value + 3)))
        {
            goto drawstring_error;
        }
    }
    else
    {
        value[3] = 0;
    }
    ScriptVariant_ToString(varlist[3], buf);
    font_printf((int)value[0], (int)value[1], (int)value[2], (int)value[3], "%s", buf);
    return S_OK;

drawstring_error:
    printf("First 3 values must be integer values and 4th value a string: drawstring(int x, int y, int font, value)\n");
    return E_FAIL;
}

//use screen_printf
//drawstringtoscreen(screen, x, y, font, string);
HRESULT openbor_drawstringtoscreen(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    int i;
    s_screen *scr;
    char buf[MAX_BUFFER_LEN];
    LONG value[3];
    *pretvar = NULL;

    if(paramCount != 5)
    {
        goto drawstring_error;
    }

    if(varlist[0]->vt != VT_PTR)
    {
        goto drawstring_error;
    }
    scr = (s_screen *)varlist[0]->ptrVal;
    if(!scr)
    {
        goto drawstring_error;
    }

    for(i = 0; i < 3; i++)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[i + 1], value + i)))
        {
            goto drawstring_error;
        }
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
HRESULT openbor_log(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    char buf[MAX_BUFFER_LEN];
    *pretvar = NULL;

    if(paramCount != 1)
    {
        goto drawstring_error;
    }

    ScriptVariant_ToString(varlist[0], buf);
    printf("%s", buf);
    return S_OK;

drawstring_error:
    printf("Function needs 1 parameter: log(value)\n");
    return E_FAIL;
}

//drawbox(x, y, width, height, z, color, lut);
HRESULT openbor_drawbox(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    int i;
    LONG value[6], l;
    *pretvar = NULL;
    s_drawmethod dm;

    if(paramCount < 6)
    {
        goto drawbox_error;
    }

    for(i = 0; i < 6; i++)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value + i)))
        {
            goto drawbox_error;
        }
    }

    if(paramCount > 6)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[6], &l)))
        {
            goto drawbox_error;
        }
    }
    else
    {
        l = -1;
    }

    if(l >= 0)
    {
        l %= MAX_BLENDINGS + 1;
    }
    if(drawmethod.flag)
    {
        dm = drawmethod;
    }
    else
    {
        dm = plainmethod;
    }
    dm.alpha = l;
    spriteq_add_box((int)value[0], (int)value[1], (int)value[2], (int)value[3], (int)value[4], (int)value[5], &dm);

    return S_OK;

drawbox_error:
    printf("Function requires 6 integer values: drawbox(int x, int y, int width, int height, int z, int color, int lut)\n");
    return E_FAIL;
}

//drawboxtoscreen(screen, x, y, width, height, color, lut);
HRESULT openbor_drawboxtoscreen(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    int i;
    s_screen *s;
    LONG value[5], l;
    *pretvar = NULL;
    s_drawmethod dm;

    if(paramCount < 6)
    {
        goto drawbox_error;
    }

    s = (s_screen *)varlist[0]->ptrVal;

    if(!s)
    {
        goto drawbox_error;
    }

    for(i = 1; i < 6; i++)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value + i - 1)))
        {
            goto drawbox_error;
        }
    }

    if(paramCount > 6)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[6], &l)))
        {
            goto drawbox_error;
        }
    }
    else
    {
        l = -1;
    }

    if(l >= 0)
    {
        l %= MAX_BLENDINGS + 1;
    }
    if(drawmethod.flag)
    {
        dm = drawmethod;
    }
    else
    {
        dm = plainmethod;
    }
    dm.alpha = l;

    putbox((int)value[0], (int)value[1], (int)value[2], (int)value[3], (int)value[4], s, &dm);

    return S_OK;

drawbox_error:
    printf("Function requires a screen handle and 5 integer values, 7th integer value is optional: drawboxtoscreen(screen, int x, int y, int width, int height, int color, int lut)\n");
    return E_FAIL;
}

//drawline(x1, y1, x2, y2, z, color, lut);
HRESULT openbor_drawline(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    int i;
    LONG value[6], l;
    *pretvar = NULL;
    s_drawmethod dm;

    if(paramCount < 6)
    {
        goto drawline_error;
    }

    for(i = 0; i < 6; i++)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value + i)))
        {
            goto drawline_error;
        }
    }

    if(paramCount > 6)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[6], &l)))
        {
            goto drawline_error;
        }
    }
    else
    {
        l = -1;
    }

    if(l >= 0 )
    {
        l %= MAX_BLENDINGS + 1;
    }
    if(drawmethod.flag)
    {
        dm = drawmethod;
    }
    else
    {
        dm = plainmethod;
    }
    dm.alpha = l;
    spriteq_add_line((int)value[0], (int)value[1], (int)value[2], (int)value[3], (int)value[4], (int)value[5], &dm);

    return S_OK;

drawline_error:
    printf("Function requires 6 integer values, 7th integer value is optional: drawline(int x1, int y1, int x2, int y2, int z, int color, int lut)\n");
    return E_FAIL;
}

//drawlinetoscreen(screen, x1, y1, x2, y2, color, lut);
HRESULT openbor_drawlinetoscreen(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    int i;
    LONG value[5], l;
    s_screen *s;
    *pretvar = NULL;
    s_drawmethod dm;

    if(paramCount < 6)
    {
        goto drawline_error;
    }

    s = (s_screen *)varlist[0]->ptrVal;

    if(!s)
    {
        goto drawline_error;
    }

    for(i = 1; i < 6; i++)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value + i - 1)))
        {
            goto drawline_error;
        }
    }

    if(paramCount > 6)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[6], &l)))
        {
            goto drawline_error;
        }
    }
    else
    {
        l = -1;
    }

    if(l >= 0 )
    {
        l %= MAX_BLENDINGS + 1;
    }
    if(drawmethod.flag)
    {
        dm = drawmethod;
    }
    else
    {
        dm = plainmethod;
    }
    dm.alpha = l;
    putline((int)value[0], (int)value[1], (int)value[2], (int)value[3], (int)value[4], s, &dm);

    return S_OK;
drawline_error:
    printf("Function requires a screen handle and 5 integer values, 7th integer value is optional: drawlinetoscreen(screen, int x1, int y1, int x2, int y2, int color, int lut)\n");
    return E_FAIL;
}

//drawsprite(sprite, x, y, z, sortid);
HRESULT openbor_drawsprite(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    int i;
    LONG value[4];
    s_sprite *spr;
    *pretvar = NULL;

    if(paramCount < 4)
    {
        goto drawsprite_error;
    }
    if(varlist[0]->vt != VT_PTR)
    {
        goto drawsprite_error;
    }

    spr = varlist[0]->ptrVal;
    if(!spr)
    {
        goto drawsprite_error;
    }

    value[3] = (LONG)0;
    for(i = 1; i < paramCount && i < 5; i++)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value + i - 1)))
        {
            goto drawsprite_error;
        }
    }

    spriteq_add_frame((int)value[0], (int)value[1], (int)value[2], spr, &drawmethod, (int)value[3]);

    return S_OK;

drawsprite_error:
    printf("Function requires a valid sprite handle 3 integer values, 5th integer value is optional: drawsprite(sprite, int x, int y, int z, int sortid)\n");
    return E_FAIL;
}

//drawspritetoscreen(sprite, screen, x, y);
HRESULT openbor_drawspritetoscreen(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    int i;
    LONG value[2];
    s_sprite *spr;
    s_screen *scr;
    *pretvar = NULL;

    if(paramCount < 4)
    {
        goto drawsprite_error;
    }
    if(varlist[0]->vt != VT_PTR)
    {
        goto drawsprite_error;
    }
    spr = varlist[0]->ptrVal;
    if(!spr)
    {
        goto drawsprite_error;
    }

    if(varlist[1]->vt != VT_PTR)
    {
        goto drawsprite_error;
    }
    scr = varlist[1]->ptrVal;
    if(!scr)
    {
        goto drawsprite_error;
    }

    for(i = 2; i < paramCount && i < 4; i++)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value + i - 2)))
        {
            goto drawsprite_error;
        }
    }

    putsprite((int)value[0], (int)value[1], spr, scr, &drawmethod);

    return S_OK;

drawsprite_error:
    printf("Function requires a valid sprite handle, a valid screen handle and 2 integer values: drawspritetoscreen(sprite, screen, int x, int y)\n");
    return E_FAIL;
}

//drawdot(x, y, z, color, lut);
HRESULT openbor_drawdot(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    int i;
    LONG value[4], l;
    *pretvar = NULL;
    s_drawmethod dm;

    if(paramCount < 4)
    {
        goto drawdot_error;
    }

    for(i = 0; i < 4; i++)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value + i)))
        {
            goto drawdot_error;
        }
    }

    if(paramCount > 4)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[4], &l)))
        {
            goto drawdot_error;
        }
    }
    else
    {
        l = -1;
    }

    if(l >= 0 )
    {
        l %= MAX_BLENDINGS + 1;
    }
    if(drawmethod.flag)
    {
        dm = drawmethod;
    }
    else
    {
        dm = plainmethod;
    }
    dm.alpha = l;
    spriteq_add_dot((int)value[0], (int)value[1], (int)value[2], (int)value[3], &dm);

    return S_OK;

drawdot_error:
    printf("Function requires 4 integer values, 5th integer value is optional: drawdot(int x, int y, int z, int color, int lut)\n");
    return E_FAIL;
}

//drawdottoscreen(screen, x, y, color, lut);
HRESULT openbor_drawdottoscreen(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    int i;
    LONG value[3], l;
    s_screen *s;
    *pretvar = NULL;
    s_drawmethod dm;

    if(paramCount < 4)
    {
        goto drawdot_error;
    }

    s = (s_screen *)varlist[0]->ptrVal;

    if(!s)
    {
        goto drawdot_error;
    }

    for(i = 1; i < 4; i++)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value + i - 1)))
        {
            goto drawdot_error;
        }
    }

    if(paramCount > 4)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[4], &l)))
        {
            goto drawdot_error;
        }
    }
    else
    {
        l = -1;
    }

    if(l >= 0 )
    {
        l %= MAX_BLENDINGS + 1;
    }
    if(drawmethod.flag)
    {
        dm = drawmethod;
    }
    else
    {
        dm = plainmethod;
    }
    dm.alpha = l;

    putpixel((int)value[0], (int)value[1], (int)value[2], s, &dm);

    return S_OK;

drawdot_error:
    printf("Function requires a screen handle and 3 integer values, 5th integer value is optional: dottoscreen(screen, int x, int y, int color, int lut)\n");
    return E_FAIL;
}


//drawscreen(screen, x, y, z, lut);
HRESULT openbor_drawscreen(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    int i;
    LONG value[3], l;
    s_screen *s;
    s_drawmethod screenmethod;
    *pretvar = NULL;

    if(paramCount < 4)
    {
        goto drawscreen_error;
    }

    s = (s_screen *)varlist[0]->ptrVal;

    if(!s)
    {
        goto drawscreen_error;
    }

    for(i = 1; i < 4; i++)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value + i - 1)))
        {
            goto drawscreen_error;
        }
    }

    if(paramCount > 4)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[4], &l)))
        {
            goto drawscreen_error;
        }
    }
    else
    {
        l = -1;
    }

    if(l >= 0 )
    {
        l %= MAX_BLENDINGS + 1;
    }
    if(paramCount <= 4)
    {
        screenmethod = drawmethod;
    }
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
HRESULT openbor_getindexedvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    return system_getglobalvar(varlist, pretvar, paramCount);
}

//setindexedvar(int index, var);
HRESULT openbor_setindexedvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    return system_setglobalvar(varlist, pretvar, paramCount);
}

//getscriptvar(int index);
HRESULT openbor_getscriptvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    return system_getlocalvar(varlist, pretvar, paramCount);
}

//setscriptvar(int index, var);
HRESULT openbor_setscriptvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    return system_setlocalvar(varlist, pretvar, paramCount);
}

//getentityvar(entity, int index);
HRESULT openbor_getentityvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ltemp;
    ScriptVariant *ptmpvar;
    entity *ent;

    if(paramCount < 2 || varlist[0]->vt != VT_PTR || !varlist[0]->ptrVal)
    {
        goto gev_error;
    }

    ent = (entity *)varlist[0]->ptrVal;

    if(varlist[1]->vt == VT_STR)
    {
        ptmpvar = Varlist_GetByName(ent->varlist, StrCache_Get(varlist[1]->strVal));
    }
    else if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[1], &ltemp)))
    {
        ptmpvar = Varlist_GetByIndex(ent->varlist, (LONG)ltemp);
    }
    else
    {
        goto gev_error;
    }

    if(ptmpvar)
    {
        ScriptVariant_Copy(*pretvar,  ptmpvar);
    }
    else
    {
        ScriptVariant_Clear(*pretvar);
    }
    return S_OK;

gev_error:
    *pretvar = NULL;
    return E_FAIL;
}

//setentityvar(int index, var);
HRESULT openbor_setentityvar(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ltemp;
    entity *ent;

    if(paramCount < 3 || varlist[0]->vt != VT_PTR || !varlist[0]->ptrVal)
    {
        goto sev_error;
    }

    ent = (entity *)varlist[0]->ptrVal;

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    if(varlist[1]->vt == VT_STR)
    {
        (*pretvar)->lVal = (LONG)Varlist_SetByName(ent->varlist, StrCache_Get(varlist[1]->strVal), varlist[2]);
    }
    else if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[1], &ltemp)))
    {
        (*pretvar)->lVal = (LONG)Varlist_SetByIndex(ent->varlist, (LONG)ltemp, varlist[2]);
    }
    else
    {
        goto sev_error;
    }

    return S_OK;
sev_error:
    *pretvar = NULL;
    return E_FAIL;
}

HRESULT openbor_getmodelproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    int iArg;

    if(paramCount < 2)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    if((varlist[0]->vt != VT_INTEGER && varlist[0]->vt != VT_STR) || varlist[1]->vt != VT_INTEGER)
    {
        printf("\n Error, getmodelproperty({model}, {property}): Invalid or missing parameter. Getmodelproperty must be passed valid {model} and {property} indexes.\n");
    }

    iArg = varlist[0]->vt == VT_INTEGER ? varlist[0]->lVal : get_cached_model_index(StrCache_Get(varlist[0]->strVal));

    if(iArg < 0 || iArg >= models_cached)
    {
        return E_FAIL;
    }

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
        (*pretvar)->ptrVal = (VOID *)model_cache[iArg].model;
    }
    case 2:
    {
        ScriptVariant_ChangeType(*pretvar, VT_STR);
        (*pretvar)->strVal = StrCache_CreateNewFrom(model_cache[iArg].name);
        break;
    }
    case 3:
    {
        ScriptVariant_ChangeType(*pretvar, VT_STR);
        (*pretvar)->strVal = StrCache_CreateNewFrom(model_cache[iArg].path);
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

HRESULT openbor_changemodelproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    int iArg;
    LONG ltemp;

    if(paramCount < 2)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    if((varlist[0]->vt != VT_INTEGER && varlist[0]->vt != VT_STR) || varlist[1]->vt != VT_INTEGER)
    {
        printf("\n Error, changemodelproperty({model}, {property}, {value}): Invalid or missing parameter. Changemodelproperty must be passed valid {model}, {property} and {value}.\n");
    }

    iArg = varlist[0]->vt == VT_INTEGER ? varlist[0]->lVal : get_cached_model_index(StrCache_Get(varlist[0]->strVal));

    if(iArg < 0 || iArg >= models_cached)
    {
        return E_FAIL;
    }

    switch (varlist[1]->lVal)
    {
    case 0:                                                    //Loaded?
    {
        /*
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        	model_cache[iArg].loadflag = (LONG)ltemp;
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
        {
            model_cache[iArg].selectable = (LONG)ltemp;
        }
        else
        {
            (*pretvar)->lVal = (LONG)0;
        }
        break;
    }
    }

    return S_OK;
}

// ===== getentityproperty =====
enum entityproperty_enum
{
    _ep_a,
    _ep_aggression,
    _ep_aiattack,
    _ep_aiflag,
    _ep_aimove,
    _ep_alpha,
    _ep_animal,
    _ep_animating,
    _ep_animation,
    _ep_animation_handle,
    _ep_animationid,
    _ep_animheight,
    _ep_animhits,
    _ep_animnum,
    _ep_animpos,
    _ep_animvalid,
    _ep_antigrab,
    _ep_antigravity,
    _ep_attackid,
    _ep_attacking,
    _ep_attackthrottle,
    _ep_attackthrottletime,
    _ep_autokill,
    _ep_base,
    _ep_bbox,
    _ep_binding,
    _ep_blink,
    _ep_blockback,
    _ep_blockodds,
    _ep_blockpain,
    _ep_boss,
    _ep_bounce,
    _ep_bound,
    _ep_candamage,
    _ep_chargerate,
    _ep_collidedentity,
    _ep_colourmap,
    _ep_colourtable,
    _ep_combostep,
    _ep_combotime,
    _ep_custom_target,
    _ep_damage_on_landing,
    _ep_dead,
    _ep_defaultmodel,
    _ep_defaultname,
    _ep_defense,
    _ep_destx,
    _ep_destz,
    _ep_detect,
    _ep_die_on_landing,
    _ep_direction,
    _ep_dot,
    _ep_dropframe,
    _ep_edelay,
    _ep_edge,
    _ep_edgerange,
    _ep_energycost,
    _ep_entitypushing,
    _ep_escapecount,
    _ep_escapehits,
    _ep_exists,
    _ep_facing,
    _ep_falldie,
    _ep_flash,
    _ep_freezetime,
    _ep_frozen,
    _ep_gfxshadow,
    _ep_grabbing,
    _ep_grabforce,
    _ep_guardpoints,
    _ep_hasplatforms,
    _ep_health,
    _ep_height,
    _ep_hitbyid,
    _ep_hitheadplatform,
    _ep_hitwall,
    _ep_hmapl,
    _ep_hmapu,
    _ep_hostile,
    _ep_icon,
    _ep_iconposition,
    _ep_invincible,
    _ep_invinctime,
    _ep_jugglepoints,
    _ep_jumpheight,
    _ep_jumpmovex,
    _ep_jumpmovez,
    _ep_jumpspeed,
    _ep_knockdowncount,
    _ep_komap,
    _ep_landedplatform,
    _ep_landframe,
    _ep_lifeposition,
    _ep_lifespancountdown,
    _ep_link,
    _ep_map,
    _ep_mapcount,
    _ep_mapdefault,
    _ep_maps,
    _ep_maptime,
    _ep_maxguardpoints,
    _ep_maxhealth,
    _ep_maxjugglepoints,
    _ep_maxmp,
    _ep_model,
    _ep_movex,
    _ep_movez,
    _ep_mp,
    _ep_mpdroprate,
    _ep_mprate,
    _ep_mpset,
    _ep_mpstable,
    _ep_mpstableval,
    _ep_name,
    _ep_nameposition,
    _ep_nextanim,
    _ep_nextmove,
    _ep_nextthink,
    _ep_no_adjust_base,
    _ep_noaicontrol,
    _ep_nodieblink,
    _ep_nodrop,
    _ep_nograb,
    _ep_nohithead,
    _ep_nolife,
    _ep_nopain,
    _ep_numweapons,
    _ep_offense,
    _ep_offscreen_noatk_factor,
    _ep_offscreenkill,
    _ep_opponent,
    _ep_owner,
    _ep_pain_time,
    _ep_parent,
    _ep_path,
    _ep_pathfindstep,
    _ep_playerindex,
    _ep_position,
    _ep_prevanimationid,
    _ep_projectile,
    _ep_projectilehit,
    _ep_pushingfactor,
    _ep_range,
    _ep_releasetime,
    _ep_running,
    _ep_rush_count,
    _ep_rush_tally,
    _ep_rush_time,
    _ep_score,
    _ep_scroll,
    _ep_seal,
    _ep_sealtime,
    _ep_setlayer,
    _ep_shadowbase,
    _ep_sortid,
    _ep_spawntype,
    _ep_speed,
    _ep_sprite,
    _ep_spritea,
    _ep_stalltime,
    _ep_staydown,
    _ep_staydownatk,
    _ep_stealth,
    _ep_subentity,
    _ep_subject_to_basemap,
    _ep_subject_to_gravity,
    _ep_subject_to_hole,
    _ep_subject_to_maxz,
    _ep_subject_to_minz,
    _ep_subject_to_obstacle,
    _ep_subject_to_platform,
    _ep_subject_to_screen,
    _ep_subject_to_wall,
    _ep_subtype,
    _ep_takeaction,
    _ep_think,
    _ep_thold,
    _ep_throwdamage,
    _ep_throwdist,
    _ep_throwframewait,
    _ep_throwheight,
    _ep_tosstime,
    _ep_tossv,
    _ep_trymove,
    _ep_type,
    _ep_velocity,
    _ep_vulnerable,
    _ep_walkoffmovex,
    _ep_walkoffmovez,
    _ep_weapent,
    _ep_weaploss,
    _ep_weapnum,
    _ep_weapon,
    _ep_x,
    _ep_xdir,
    _ep_y,
    _ep_z,
    _ep_zdir,
    _ep_the_end,
};

// arranged list, for searching
static const char *eplist[] =
{
    "a",
    "aggression",
    "aiattack",
    "aiflag",
    "aimove",
    "alpha",
    "animal",
    "animating",
    "animation",
    "animation.handle",
    "animationid",
    "animheight",
    "animhits",
    "animnum",
    "animpos",
    "animvalid",
    "antigrab",
    "antigravity",
    "attackid",
    "attacking",
    "attackthrottle",
    "attackthrottletime",
    "autokill",
    "base",
    "bbox",
    "binding",
    "blink",
    "blockback",
    "blockodds",
    "blockpain",
    "boss",
    "bounce",
    "bound",
    "candamage",
    "chargerate",
    "collidedentity",
    "colourmap",
    "colourtable",
    "combostep",
    "combotime",
    "custom_target",
    "damage_on_landing",
    "dead",
    "defaultmodel",
    "defaultname",
    "defense",
    "destx",
    "destz",
    "detect",
    "die_on_landing",
    "direction",
    "dot",
    "dropframe",
    "edelay",
    "edge",
    "edgerange",
    "energycost",
    "entitypushing",
    "escapecount",
    "escapehits",
    "exists",
    "facing",
    "falldie",
    "flash",
    "freezetime",
    "frozen",
    "gfxshadow",
    "grabbing",
    "grabforce",
    "guardpoints",
    "hasplatforms",
    "health",
    "height",
    "hitbyid",
    "hitheadplatform",
    "hitwall",
    "hmapl",
    "hmapu",
    "hostile",
    "icon",
    "iconposition",
    "invincible",
    "invinctime",
    "jugglepoints",
    "jumpheight",
    "jumpmovex",
    "jumpmovez",
    "jumpspeed",
    "knockdowncount",
    "komap",
    "landedplatform",
    "landframe",
    "lifeposition",
    "lifespancountdown",
    "link",
    "map",
    "mapcount",
    "mapdefault",
    "maps",
    "maptime",
    "maxguardpoints",
    "maxhealth",
    "maxjugglepoints",
    "maxmp",
    "model",
    "movex",
    "movez",
    "mp",
    "mpdroprate",
    "mprate",
    "mpset",
    "mpstable",
    "mpstableval",
    "name",
    "nameposition",
    "nextanim",
    "nextmove",
    "nextthink",
    "no_adjust_base",
    "noaicontrol",
    "nodieblink",
    "nodrop",
    "nograb",
    "nohithead",
    "nolife",
    "nopain",
    "numweapons",
    "offense",
    "offscreennoatkfactor",
    "offscreenkill",
    "opponent",
    "owner",
    "pain_time",
    "parent",
    "path",
    "pathfindstep",
    "playerindex",
    "position",
    "prevanimationid",
    "projectile",
    "projectilehit",
    "pushingfactor",
    "range",
    "releasetime",
    "running",
    "rush_count",
    "rush_tally",
    "rush_time",
    "score",
    "scroll",
    "seal",
    "sealtime",
    "setlayer",
    "shadowbase",
    "sortid",
    "spawntype",
    "speed",
    "sprite",
    "spritea",
    "stalltime",
    "staydown",
    "staydownatk",
    "stealth",
    "subentity",
    "subject_to_basemap",
    "subject_to_gravity",
    "subject_to_hole",
    "subject_to_maxz",
    "subject_to_minz",
    "subject_to_obstacle",
    "subject_to_platform",
    "subject_to_screen",
    "subject_to_wall",
    "subtype",
    "takeaction",
    "think",
    "thold",
    "throwdamage",
    "throwdist",
    "throwframewait",
    "throwheight",
    "tosstime",
    "tossv",
    "trymove",
    "type",
    "velocity",
    "vulnerable",
    "walkoffmovex",
    "walkoffmovez",
    "weapent",
    "weaploss",
    "weapnum",
    "weapon",
    "x",
    "xdir",
    "y",
    "z",
    "zdir",
};

enum aiflag_enum
{
    _ep_aiflag_animating,
    _ep_aiflag_attacking,
    _ep_aiflag_autokill,
    _ep_aiflag_blink,
    _ep_aiflag_blocking,
    _ep_aiflag_charging,
    _ep_aiflag_dead,
    _ep_aiflag_drop,
    _ep_aiflag_ducking,
    _ep_aiflag_falling,
    _ep_aiflag_frozen,
    _ep_aiflag_getting,
    _ep_aiflag_idlemode,
    _ep_aiflag_idling,
    _ep_aiflag_inbackpain,
    _ep_aiflag_inpain,
    _ep_aiflag_invincible,
    _ep_aiflag_jumpid,
    _ep_aiflag_jumping,
    _ep_aiflag_projectile,
    _ep_aiflag_riseattacking,
    _ep_aiflag_rising,
    _ep_aiflag_running,
    _ep_aiflag_toexplode,
    _ep_aiflag_turning,
    _ep_aiflag_walking,
    _ep_aiflag_walkmode,
    _ep_aiflag_the_end,
};


static const char *eplist_aiflag[] =
{
    "animating",
    "attacking",
    "autokill",
    "blink",
    "blocking",
    "charging",
    "dead",
    "drop",
    "ducking",
    "falling",
    "frozen",
    "getting",
    "idlemode",
    "idling",
    "inbackpain",
    "inpain",
    "invincible",
    "jumpid",
    "jumping",
    "projectile",
    "riseattacking",
    "rising",
    "running",
    "toexplode",
    "turning",
    "walking",
    "walkmode",
};

enum edgerange_enum
{
    _ep_edgerange_x,
    _ep_edgerange_z,
    _ep_edgerange_the_end,
};

static const char *eplist_edgerange[] =
{
    "x",
    "z",
};

// ===== changedrawmethod ======
enum drawmethod_enum
{
    _dm_alpha,
    _dm_amplitude,
    _dm_beginsize,
    _dm_centerx,
    _dm_centery,
    _dm_channelb,
    _dm_channelg,
    _dm_channelr,
    _dm_clip,
    _dm_cliph,
    _dm_clipw,
    _dm_clipx,
    _dm_clipy,
    _dm_enabled,
    _dm_endsize,
    _dm_fillcolor,
    _dm_flag,
    _dm_fliprotate,
    _dm_flipx,
    _dm_flipy,
    _dm_perspective,
    _dm_remap,
    _dm_reset,
    _dm_rotate,
    _dm_scalex,
    _dm_scaley,
    _dm_shiftx,
    _dm_table,
    _dm_tintcolor,
    _dm_tintmode,
    _dm_transbg,
    _dm_watermode,
    _dm_wavelength,
    _dm_wavespeed,
    _dm_wavetime,
    _dm_xrepeat,
    _dm_xspan,
    _dm_yrepeat,
    _dm_yspan,
    _dm_the_end,
};

enum _prop_dropframe_enum
{
    _PROP_DROPFRAME_FRAME,
    _PROP_DROPFRAME_VELOCITY_X,
    _PROP_DROPFRAME_VELOCITY_Y,
    _PROP_DROPFRAME_VELOCITY_Z,
    _PROP_DROPFRAME_THE_END
};

enum _prop_followup_enum
{
    _PROP_FOLLOWUP_ANIMATION,
    _PROP_FOLLOWUP_CONDITION,
    _PROP_FOLLOWUP_THE_END
};

enum _prop_jumpframe_enum
{
    _PROP_JUMPFRAME_FRAME,
    _PROP_JUMPFRAME_VELOCITY_X,
    _PROP_JUMPFRAME_VELOCITY_Y,
    _PROP_JUMPFRAME_VELOCITY_Z,
    _PROP_JUMPFRAME_THE_END
};

enum _prop_landframe_enum
{
    _PROP_LANDFRAME_FRAME,
    _PROP_LANDFRAME_VELOCITY_X,
    _PROP_LANDFRAME_VELOCITY_Y,
    _PROP_LANDFRAME_VELOCITY_Z,
    _PROP_LANDFRAME_THE_END
};

enum _prop_loop_enum
{
    _PROP_LOOP_FRAME_MAX,
    _PROP_LOOP_FRAME_MIN,
    _PROP_LOOP_MODE,
    _PROP_LOOP_THE_END
};

 enum _prop_move_enum
{
    _PROP_MOVE_BASE,
    _PROP_MOVE_X,
    _PROP_MOVE_Y,
    _PROP_MOVE_Z,
    _PROP_MOVE_THE_END
};

 enum _prop_offset_enum
{
    _PROP_OFFSET_X,
    _PROP_OFFSET_Y,
    _PROP_OFFSET_THE_END
};

 enum _prop_platform_enum
{
    _PROP_PLATFORM_ALT,
    _PROP_PLATFORM_DEPTH,
    _PROP_PLATFORM_LOWERLEFT,
    _PROP_PLATFORM_LOWERRIGHT,
    _PROP_PLATFORM_UPPERLEFT,
    _PROP_PLATFORM_UPPERRIGHT,
    _PROP_PLATFORM_X,
    _PROP_PLATFORM_Z,
    _PROP_PLATFORM_THE_END
};

enum _prop_projectile_enum
{
    _PROP_PROJECTILE_BOMB,
    _PROP_PROJECTILE_FLASH,
    _PROP_PROJECTILE_KNIFE,
    _PROP_PROJECTILE_SHOOTFRAME,
    _PROP_PROJECTILE_STAR,
    _PROP_PROJECTILE_THROWFRAME,
    _PROP_PROJECTILE_THROWPOSITION_BASE,
    _PROP_PROJECTILE_THROWPOSITION_X,
    _PROP_PROJECTILE_THROWPOSITION_Y,
    _PROP_PROJECTILE_THROWPOSITION_Z,
    _PROP_PROJECTILE_TOSSFRAME,
    _PROP_PROJECTILE_THE_END
};

enum _prop_quakeframe_enum
{
    _PROP_QUAKEFRAME_FRAMESTART,
    _PROP_QUAKEFRAME_INTENSITY,
    _PROP_QUAKEFRAME_REPEAT,
    _PROP_QUAKEFRAME_THE_END
};

enum _prop_range_enum
{
    _PROP_RANGEA_MAX,
    _PROP_RANGEA_MIN,
    _PROP_RANGEB_MAX,
    _PROP_RANGEB_MIN,
    _PROP_RANGEX_MAX,
    _PROP_RANGEX_MIN,
    _PROP_RANGEZ_MAX,
    _PROP_RANGEZ_MIN,
    _PROP_RANGE_THE_END
};

enum _prop_size_enum
{
    _PROP_SIZE_BASE,
    _PROP_SIZE_X,
    _PROP_SIZE_Y,
    _PROP_SIZE_Z,
    _PROP_SIZE_THE_END
};

 enum _prop_shadow_enum
{
    _PROP_FSHADOW,
    _PROP_SHADOW_COORDS_X,
    _PROP_SHADOW_COORDS_Y,
    _PROP_SHADOW_THE_END
};

enum _prop_spawnframe_enum
{
    _PROP_SPAWNFRAME_FRAME,
    _PROP_SPAWNFRAME_RELATIVE,
    _PROP_SPAWNFRAME_X,
    _PROP_SPAWNFRAME_Y,
    _PROP_SPAWNFRAME_Z,
    _PROP_SPAWNFRAME_THE_END
};

enum _prop_spritea_enum
{
    _PROP_SPRITEA_CENTERX,
    _PROP_SPRITEA_CENTERY,
    _PROP_SPRITEA_FILE,
    _PROP_SPRITEA_OFFSETX,
    _PROP_SPRITEA_OFFSETY,
    _PROP_SPRITEA_SPRITE,
    _PROP_SPRITEA_THE_END
};

enum _prop_summonframe_enum
{
    _PROP_SUMMONFRAME_FRAME,
    _PROP_SUMMONFRAME_RELATIVE,
    _PROP_SUMMONFRAME_X,
    _PROP_SUMMONFRAME_Y,
    _PROP_SUMMONFRAME_Z,
    _PROP_SUMMONFRAME_THE_END
};

enum _prop_starvelocity_enum
{
    _PROP_STARVELOCITY_1,
    _PROP_STARVELOCITY_2,
    _PROP_STARVELOCITY_3,
    _PROP_STARVELOCITY_THE_END
};

enum _prop_weaponframe_enum
{
    _PROP_WEAPONFRAME_FRAME,
    _PROP_WEAPONFRAME_WEAPON,
    _PROP_WEAPONFRAME_THE_END
};

enum _ep_defense_enum
{
    _ep_defense_blockpower,
    _ep_defense_blockratio,
    _ep_defense_blockthreshold,
    _ep_defense_blocktype,
    _ep_defense_factor,
    _ep_defense_knockdown,
    _ep_defense_pain,
    _ep_defense_the_end,
};

enum gep_dot_enum
{
    _ep_dot_force,
    _ep_dot_mode,
    _ep_dot_owner,
    _ep_dot_rate,
    _ep_dot_time,
    _ep_dot_type,
    _ep_dot_the_end,
};

enum gep_edelay_enum
{
    _ep_edelay_cap_max,
    _ep_edelay_cap_min,
    _ep_edelay_factor,
    _ep_edelay_mode,
    _ep_edelay_range_max,
    _ep_edelay_range_min,
    _ep_edelay_the_end,
};

enum gep_energycost_enum
{
    _ep_energycost_cost,
    _ep_energycost_disable,
    _ep_energycost_mponly,
    _ep_energycost_the_end,
};

enum gep_flash_enum
{
    _ep_flash_block,
    _ep_flash_def,
    _ep_flash_noattack,
    _ep_flash_the_end,
};

enum gep_icon_enum
{
    _ep_icon_def,
    _ep_icon_die,
    _ep_icon_get,
    _ep_icon_mphigh,
    _ep_icon_mplow,
    _ep_icon_mpmed,
    _ep_icon_pain,
    _ep_icon_weapon,
    _ep_icon_x,
    _ep_icon_y,
    _ep_icon_the_end,
};

enum _ep_knockdowncount_enum
{
    _ep_knockdowncount_current,
    _ep_knockdowncount_max,
    _ep_knockdowncount_time,
    _ep_knockdowncount_the_end,
};

enum gep_landframe_enum
{
    _ep_landframe_ent,
    _ep_landframe_frame,
    _ep_landframe_the_end,
};

enum gep_maps_enum
{
    _ep_maps_count,
    _ep_maps_current,
    _ep_maps_default,
    _ep_maps_dying,
    _ep_maps_dying_critical,
    _ep_maps_dying_low,
    _ep_maps_frozen,
    _ep_maps_hide_end,
    _ep_maps_hide_start,
    _ep_maps_ko,
    _ep_maps_kotype,
    _ep_maps_table,
    _ep_maps_time,
    _ep_maps_the_end,
};

enum gep_range_enum
{
    _ep_range_amax,
    _ep_range_amin,
    _ep_range_bmax,
    _ep_range_bmin,
    _ep_range_xmax,
    _ep_range_xmin,
    _ep_range_zmax,
    _ep_range_zmin,
    _ep_range_the_end,
};

enum gep_running_enum
{
    _ep_running_jumpx,
    _ep_running_jumpy,
    _ep_running_land,
    _ep_running_movez,
    _ep_running_speed,
    _ep_running_the_end,
};

enum gep_spritea_enum
{
    _ep_spritea_centerx,
    _ep_spritea_centery,
    _ep_spritea_file,
    _ep_spritea_offsetx,
    _ep_spritea_offsety,
    _ep_spritea_sprite,
    _ep_spritea_the_end,
};

enum gep_staydown_enum
{
    _ep_staydown_rise,
    _ep_staydown_riseattack,
    _ep_staydown_riseattack_stall,
    _ep_staydown_the_end,
};

enum cep_hostile_candamage_enum
{
    _ep_hcd_ground,
    _ep_hcd_type_enemy,
    _ep_hcd_type_npc,
    _ep_hcd_type_obstacle,
    _ep_hcd_type_player,
    _ep_hcd_type_shot,
    _ep_hcd_the_end,
};

enum cep_takeaction_enum
{
    _ep_ta_bomb_explode,
    _ep_ta_common_animation_normal,
    _ep_ta_common_attack_proc,
    _ep_ta_common_block,
    _ep_ta_common_drop,
    _ep_ta_common_fall,
    _ep_ta_common_get,
    _ep_ta_common_grab,
    _ep_ta_common_grabattack,
    _ep_ta_common_grabbed,
    _ep_ta_common_jump,
    _ep_ta_common_land,
    _ep_ta_common_lie,
    _ep_ta_common_pain,
    _ep_ta_common_prejump,
    _ep_ta_common_rise,
    _ep_ta_common_spawn,
    _ep_ta_common_turn,
    _ep_ta_normal_prepare,
    _ep_ta_npc_warp,
    _ep_ta_player_blink,
    _ep_ta_suicide,
    _ep_ta_the_end,
};

enum cep_think_enum   // 2011_03_03, DC: Think types.
{
    _ep_th_common_think,
    _ep_th_player_think,
    _ep_th_steam_think,
    _ep_th_steamer_think,
    _ep_th_text_think,
    _ep_th_trap_think,
    _ep_th_the_end,
};

int mapstrings_animationproperty(ScriptVariant **varlist, int paramCount)
{
    return 0;
//    char *propname;
//    const char *aps;
//    int prop, ap; //int prop, i, ep, t;
//    int result = 1;
//
//    MAPSTRINGS(varlist[1], list_animation_prop, ANI_PROP_THE_END,
//               "Property name '%s' is not a supported animation property.\n");
//
//    if(paramCount < 3 || varlist[1]->vt != VT_INTEGER)
//    {
//        return result;
//    }
//    else
//    {
//        ap = varlist[1]->lVal;
//        aps = (ap < ANI_PROP_THE_END && ap >= 0) ? list_animation_prop[ap] : "";
//    }
//
//    return result;
}

int mapstrings_entityproperty(ScriptVariant **varlist, int paramCount)
{
    char *propname;
    const char *eps;
    int prop, i, ep, t;

    static const char *proplist_defense[] =
    {
        "blockpower",
        "blockratio",
        "blockthreshold",
        "blocktype",
        "factor",
        "knockdown",
        "pain",
    };

    static const char *proplist_dot[] =
    {
        "force",
        "mode",
        "owner",
        "rate",
        "time",
        "type",
    };

    static const char *proplist_edelay[] =
    {
        "cap_max",
        "cap_min",
        "factor",
        "mode",
        "range_max",
        "range_min",
    };

    static const char *proplist_energycost[] =
    {
        "cost",
        "disable",
        "mponly",
    };

    static const char *proplist_flash[] =
    {
        "block",
        "default",
        "noattack",
    };

    static const char *proplist_icon[] =
    {
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

    static const char *proplist_knockdowncount[] =
    {
        "current",
        "max",
        "time",
    };

    static const char *proplist_landframe[] =
    {
        "ent",
        "frame",
    };

    static const char *proplist_maps[] =
    {
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

    static const char *proplist_range[] =
    {
        "amax",
        "amin",
        "bmax",
        "bmin",
        "xmax",
        "xmin",
        "zmax",
        "zmin",
    };

    static const char *proplist_running[] =
    {
        "jumpx",
        "jumpy",
        "land",
        "movez",
        "speed",
    };

    static const char *proplist_spritea[] =
    {
        "centerx",
        "centery",
        "file",
        "offsetx",
        "offsety",
        "sprite",
    };

    static const char *proplist_staydown[] =
    {
        "rise",
        "riseattack",
        "riseattack_stall",
    };

    static const char *proplist_hostile_candamage[] =
    {
        "ground",
        "type_enemy",
        "type_npc",
        "type_obstacle",
        "type_player",
        "type_shot",
    };

    static const char *proplist_takeaction[] =
    {
        "bomb_explode",
        "common_animation_normal",
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
        "normal_prepare",
        "npc_warp",
        "player_blink",
        "suicide",
    };

    static const char *proplist_think[] =   // 2011_03_03, DC: Think types.
    {
        "common_think",
        "player_think",
        "steam_think",
        "steamer_think",
        "text_think",
        "trap_think",
    };

    if(paramCount < 2)
    {
        return 1;
    }

    // map entity properties
    MAPSTRINGS(varlist[1], eplist, _ep_the_end,
               "Property name '%s' is not supported by function getentityproperty.\n");

    if(paramCount < 3 || varlist[1]->vt != VT_INTEGER)
    {
        return 1;
    }

    ep = varlist[1]->lVal;
    eps = (ep < _ep_the_end && ep >= 0) ? eplist[ep] : "";

    switch (ep)
    {
    // deprecation warning for "a" property
    case _ep_a:
    {
        printf("\nNote: Property 'a' has been deprecated. Use 'y' to access the Y (vertical) axis property.\n");
        break;
    }
    // map subproperties of aiflag property
    case _ep_aiflag:
    {
        MAPSTRINGS(varlist[2], eplist_aiflag, _ep_aiflag_the_end,
                   _is_not_a_known_subproperty_of_, eps);
        break;
    }
    // map subproperties of edgerange property
    case _ep_edgerange:
    {
        MAPSTRINGS(varlist[2], eplist_edgerange, _ep_edgerange_the_end,
                   _is_not_a_known_subproperty_of_, eps);
        break;
    }
    // map subproperties of defense property
    case _ep_defense:
    {
        if(paramCount >= 4)
        {
            MAPSTRINGS(varlist[3], proplist_defense, _ep_defense_the_end,
                       _is_not_a_known_subproperty_of_, eps);
        }
        break;
    }
    // map subproperties of DOT
    case _ep_dot:
    {
        MAPSTRINGS(varlist[2], proplist_dot, _ep_dot_the_end,
                   _is_not_a_known_subproperty_of_, eps);
        break;
    }
    // map subproperties of Edelay property
    case _ep_edelay:
    {
        MAPSTRINGS(varlist[2], proplist_edelay, _ep_edelay_the_end,
                   _is_not_a_known_subproperty_of_, eps);
        break;
    }
    // map subproperties of Energycost
    case _ep_energycost:
    {
        MAPSTRINGS(varlist[2], proplist_energycost, _ep_energycost_the_end,
                   _is_not_a_known_subproperty_of_, eps);
        break;
    }
    // map subproperties of Flash
    case _ep_flash:
    {
        MAPSTRINGS(varlist[2], proplist_flash, _ep_flash_the_end,
                   _is_not_a_known_subproperty_of_, eps);
        break;
    }
    // map subproperties of Icon
    case _ep_icon:
    {
        MAPSTRINGS(varlist[2], proplist_icon, _ep_icon_the_end,
                   _is_not_a_known_subproperty_of_, eps);
        break;
    }

    // map subproperties of Knockdowncount
    case _ep_knockdowncount:
    {
        MAPSTRINGS(varlist[2], proplist_knockdowncount, _ep_knockdowncount_the_end,
                   _is_not_a_known_subproperty_of_, eps);
        break;
    }
    // map subproperties of Landframe
    case _ep_landframe:
    {
        MAPSTRINGS(varlist[2], proplist_landframe, _ep_landframe_the_end,
                   _is_not_a_known_subproperty_of_, eps);
        break;
    }
    // map subproperties of Maps
    case  _ep_maps:
    {
        MAPSTRINGS(varlist[2], proplist_maps, _ep_maps_the_end,
                   _is_not_a_known_subproperty_of_, eps);
        break;
    }
    // map subproperties of Range
    case _ep_range:
    {
        MAPSTRINGS(varlist[2], proplist_range, _ep_range_the_end,
                   _is_not_a_known_subproperty_of_, eps);
        break;
    }
    // map subproperties of Running
    case _ep_running:
    {
        MAPSTRINGS(varlist[2], proplist_running, _ep_running_the_end,
                   _is_not_a_known_subproperty_of_, eps);
        break;
    }

    // map subproperties of Spritea
    case _ep_spritea:
    {
        MAPSTRINGS(varlist[2], proplist_spritea, _ep_spritea_the_end,
                   _is_not_a_known_subproperty_of_, eps);
        break;
    }
    // map subproperties of Staydown
    case _ep_staydown:
    {
        MAPSTRINGS(varlist[2], proplist_staydown, _ep_running_the_end,
                   _is_not_a_known_subproperty_of_, eps);
        break;
    }
    //hostile, candamage, projectilehit
    case _ep_hostile:
    case _ep_candamage:
    case _ep_projectilehit:
    {
        for(i = 2; i < paramCount; i++)
        {
            t = varlist[i]->vt;
            MAPSTRINGS(varlist[i], proplist_hostile_candamage, _ep_hcd_the_end,
                       _is_not_supported_by_, eps);

            if(varlist[i]->vt == VT_INTEGER && t == VT_STR)
            {
                varlist[i]->lVal |= 0x80000000;    //flag it
            }
        }
        break;
    }
    // action for takeaction
    case _ep_takeaction:
    {
        MAPSTRINGS(varlist[2], proplist_takeaction, _ep_ta_the_end,
                   _is_not_supported_by_, eps);
        break;
    }
    // 2011_03_13, DC: Think sets for think.
    case _ep_think:
    {
        MAPSTRINGS(varlist[2], proplist_think, _ep_th_the_end,
                   _is_not_supported_by_, eps);
        break;
    }
    }

    return 1;
}

//getentityproperty(pentity, propname);
HRESULT openbor_getentityproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    entity *ent			= NULL;
    ScriptVariant *arg	= NULL;
    ScriptVariant *arg1	= NULL;
    s_sprite *spr;
    LONG ltemp, ltemp2;
    int i				= 0;
    int propind ;
    int tempint			= 0;

    if(paramCount < 2)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_Clear(*pretvar);
    mapstrings_entityproperty(varlist, paramCount);

    arg = varlist[0];
    if(arg->vt != VT_PTR && arg->vt != VT_EMPTY)
    {
        printf("Function getentityproperty must have a valid entity handle.\n");
        *pretvar = NULL;
        return E_FAIL;
    }
    ent = (entity *)arg->ptrVal; //retrieve the entity
    if(!ent)
    {
        return S_OK;
    }

    arg = varlist[1];
    if(arg->vt != VT_INTEGER)
    {
        printf("Function getentityproperty must have a string property name.\n");
    }

    propind = arg->lVal;

    switch(propind)
    {
    case _ep_a:
    case _ep_y:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->position.y;
        break;
    }
    case _ep_aggression:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.aggression;
        break;
    }
    case _ep_aiattack:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.aiattack;
        break;
    }
    case _ep_aiflag:
    {
        if(paramCount < 3)
        {
            break;
        }
        arg = varlist[2];
        if(arg->vt != VT_INTEGER)
        {
            printf("You must give a string name for aiflag.\n");
            return E_FAIL;
        }
        ltemp = arg->lVal;

        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        switch(ltemp)
        {
        case _ep_aiflag_dead:
            (*pretvar)->lVal = (LONG)ent->dead;
            break;
        case _ep_aiflag_jumpid:
            (*pretvar)->lVal = (LONG)ent->jump.animation_id;
            break;
        case _ep_aiflag_jumping:
            (*pretvar)->lVal = (LONG)ent->jumping;
            break;
        case _ep_aiflag_idling:
            (*pretvar)->lVal = (LONG)ent->idling;
            break;
        case _ep_aiflag_drop:
            (*pretvar)->lVal = (LONG)ent->drop;
            break;
        case _ep_aiflag_attacking:
            (*pretvar)->lVal = (LONG)ent->attacking;
            break;
        case _ep_aiflag_getting:
            (*pretvar)->lVal = (LONG)ent->getting;
            break;
        case _ep_aiflag_turning:
            (*pretvar)->lVal = (LONG)ent->turning;
            break;
        case _ep_aiflag_charging:
            (*pretvar)->lVal = (LONG)ent->charging;
            break;
        case _ep_aiflag_blocking:
            (*pretvar)->lVal = (LONG)ent->blocking;
            break;
        case _ep_aiflag_ducking:
            (*pretvar)->lVal = (LONG)ent->ducking;
            break;
        case _ep_aiflag_falling:
            (*pretvar)->lVal = (LONG)ent->falling;
            break;
        case _ep_aiflag_running:
            (*pretvar)->lVal = (LONG)ent->running;
            break;
        case _ep_aiflag_inpain:
            (*pretvar)->lVal = (LONG)ent->inpain;
            break;
        case _ep_aiflag_rising:
            (*pretvar)->lVal = (LONG)ent->rising;
            break;
        case _ep_aiflag_riseattacking:
            (*pretvar)->lVal = (LONG)ent->riseattacking;
            break;
        case _ep_aiflag_inbackpain:
            (*pretvar)->lVal = (LONG)ent->inbackpain;
            break;
        case _ep_aiflag_projectile:
            (*pretvar)->lVal = (LONG)ent->projectile;
            break;
        case _ep_aiflag_frozen:
            (*pretvar)->lVal = (LONG)ent->frozen;
            break;
        case _ep_aiflag_toexplode:
            (*pretvar)->lVal = (LONG)ent->toexplode;
            break;
        case _ep_aiflag_animating:
            (*pretvar)->lVal = (LONG)ent->animating;
            break;
        case _ep_aiflag_blink:
            (*pretvar)->lVal = (LONG)ent->blink;
            break;
        case _ep_aiflag_invincible:
            (*pretvar)->lVal = (LONG)ent->invincible;
            break;
        case _ep_aiflag_autokill:
            (*pretvar)->lVal = (LONG)ent->autokill;
            break;
        case _ep_aiflag_idlemode:
            (*pretvar)->lVal = (LONG)ent->idlemode;
            break;
        case _ep_aiflag_walkmode:
            (*pretvar)->lVal = (LONG)ent->walkmode;
            break;
        case _ep_aiflag_walking:
            (*pretvar)->lVal = (LONG)ent->walking;
            break;
        default:
            ScriptVariant_Clear(*pretvar);
            return E_FAIL;
        }
        break;
    }
    case _ep_aimove:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.aimove;
        break;
    }
    case _ep_alpha:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.alpha;
        break;
    }
    case _ep_animal:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.animal;
        break;
    }
    case _ep_animating:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->animating;
        break;
    }
    case _ep_animation_handle:
    {
        #define ARG_ANIMATION_ID 2

        // Did the user provide an animation id?
        if(paramCount > 2)
        {
            arg = varlist[ARG_ANIMATION_ID];

            // If the argument is invalid, use current animation ID instead.
            if(FAILED(ScriptVariant_IntegerValue(arg, &ltemp)))
            {
                ltemp = (LONG)ent->animnum;
            }
        }
        else
        {
            ltemp = (LONG)ent->animnum;
        }

        // If the animation exists, get the handle.
        if(validanim(ent, ltemp))
        {
            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)ent->modeldata.animation[ltemp];
        }

        break;

        #undef ARG_ANIMATION_ID
    }
    /*
    case _ep_animationid: See animnum.
    */
    case _ep_animheight:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->animation->size.x;
        break;
    }
    case _ep_animhits:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->animation->animhits;
        break;
    }
    case _ep_animnum:
    case _ep_animationid:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->animnum;
        break;
    }
    case _ep_prevanimationid:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->prevanimnum;
        break;
    }
    case _ep_animpos:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->animpos;
        break;
    }
    case _ep_animvalid:
    {
        ltemp = 0;
        if(paramCount == 3)
        {
            arg = varlist[2];
            if(FAILED(ScriptVariant_IntegerValue(arg, &ltemp)))
            {
                ltemp = (LONG)0;
            }
        }
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)validanim(ent, ltemp);
        break;
    }
    case _ep_antigrab:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->modeldata.antigrab;
        break;
    }
    case _ep_antigravity:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->modeldata.antigravity;
        break;
    }
    case _ep_attacking:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->attacking;
        break;
    }
    case _ep_attackid:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->attack_id_outgoing;
        break;
    }
    case _ep_autokill:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->autokill;
        break;
    }
    case _ep_base:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->base;
        break;
    }
    case _ep_vulnerable:
    {
        if(paramCount == 2)
        {
            i		= ent->animnum;
            tempint	= ent->animpos;
        }
        else if(paramCount < 4
                || varlist[2]->vt != VT_INTEGER
                || varlist[3]->vt != VT_INTEGER)
        {
            printf("\n Error, getentityproperty({ent}, \"vulnerable\", {animation}, {frame}): parameters missing or invalid. \n");
            *pretvar = NULL;
            return E_FAIL;
        }
        else
        {
            i		= varlist[2]->lVal;												//Animation parameter.
            tempint	= varlist[3]->lVal;												//Frame parameter.
        }
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.animation[i]->vulnerable[tempint];
        break;
    }
    case _ep_blink:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->blink;
        break;
    }
    case _ep_blockback:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.blockback;
        break;
    }
    case _ep_blockodds:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.blockodds;
        break;
    }
    case _ep_blockpain:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.blockpain;
        break;
    }
    case _ep_boss:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->boss;
        break;
    }
    case _ep_bounce:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.bounce;
        break;
    }
    case _ep_bound:
    {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = (VOID *)ent->binding.ent;
        break;
    }
    case _ep_binding:
    {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = (VOID *)&ent->binding;
        break;
    }
    case _ep_candamage:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.candamage;
        break;
    }
    case _ep_combostep:
    {
        if(paramCount >= 3)
        {
            if(FAILED(ScriptVariant_IntegerValue(varlist[3], &ltemp2)))
            {
                *pretvar = NULL;
                return E_FAIL;
            }
        }
        else
        {
            ltemp2 = 0;
        }
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->combostep[(LONG)ltemp2];
        break;
    }
    case _ep_combotime:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->combotime;
        break;
    }
    case _ep_collidedentity:
    {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = (VOID *)&ent->collided_entity;
        break;
    }
    case _ep_hostile:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.hostile;
        break;
    }
    case _ep_projectilehit:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.projectilehit;
        break;
    }
    case _ep_chargerate:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.chargerate;
        break;
    }
    case _ep_colourmap:
    {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = (VOID *)(ent->colourmap);
        break;
    }
    case _ep_colourtable:
    {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = (VOID *)model_get_colourmap(&(ent->modeldata), varlist[2]->lVal + 1);
        break;
    }
    case _ep_damage_on_landing:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->damage_on_landing.attack_force;


        break;
    }
    case _ep_dead:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->dead;
        break;
    }
    case _ep_defaultmodel:
    case _ep_defaultname:
    {
        ScriptVariant_ChangeType(*pretvar, VT_STR);
        (*pretvar)->strVal = StrCache_CreateNewFrom(ent->defaultmodel->name);
        break;
    }
    case _ep_defense:
    {
        ltemp = 0;
        if(paramCount >= 3)
        {
            if(FAILED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            {
                printf("You must specify an attack type for your defense property.\n");
                *pretvar = NULL;
                return E_FAIL;
            }
            ltemp2 = _ep_defense_factor;
        }

        if(paramCount >= 4)
        {
            if(FAILED(ScriptVariant_IntegerValue(varlist[3], &ltemp2)))
            {
                *pretvar = NULL;
                return E_FAIL;
            }
        }
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);

        switch(ltemp2)
        {
        case _ep_defense_factor:
        {
            (*pretvar)->dblVal = (DOUBLE)ent->defense[(LONG)ltemp].factor;
            break;
        }
        case _ep_defense_blockpower:
        {
            (*pretvar)->dblVal = (DOUBLE)ent->defense[(LONG)ltemp].blockpower;
            break;
        }
        case _ep_defense_blockratio:
        {
            (*pretvar)->dblVal = (DOUBLE)ent->defense[(LONG)ltemp].blockratio;
            break;
        }
        case _ep_defense_blockthreshold:
        {
            (*pretvar)->dblVal = (DOUBLE)ent->defense[(LONG)ltemp].blockthreshold;
            break;
        }
        case _ep_defense_blocktype:
        {
            (*pretvar)->dblVal = (DOUBLE)ent->defense[(LONG)ltemp].blocktype;
            break;
        }
        case _ep_defense_knockdown:
        {
            (*pretvar)->dblVal = (DOUBLE)ent->defense[(LONG)ltemp].knockdown;
            break;
        }
        case _ep_defense_pain:
        {
            (*pretvar)->dblVal = (DOUBLE)ent->defense[(LONG)ltemp].pain;
            break;
        }
        default:
            *pretvar = NULL;
            return E_FAIL;
        }
        break;
    }
    case _ep_destx:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->destx;
        break;
    }
    case _ep_destz:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->destz;
        break;
    }
    case _ep_detect:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.stealth.detect;
        break;
    }
    case _ep_die_on_landing:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->die_on_landing;
        break;
    }
    case _ep_direction:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->direction;
        break;
    }
    case _ep_dot:
    {
        if(paramCount < 4)
        {
            break;
        }

        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            i = (LONG)ltemp;
        }

        arg = varlist[3];
        if(arg->vt != VT_INTEGER)
        {
            printf("You must provide a string name for dot subproperty.\n\
	~'time'\n\
	~'mode'\n\
	~'force'\n\
	~'rate'\n\
	~'type'\n\
	~'owner'\n");
            *pretvar = NULL;
            return E_FAIL;
        }
        switch(arg->lVal)
        {
        case _ep_dot_time:
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)ent->dot_time[i];
            break;
        }
        case _ep_dot_mode:
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)ent->dot[i];
            break;
        }
        case _ep_dot_force:

        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)ent->dot_force[i];
            break;
        }
        case _ep_dot_rate:
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)ent->dot_rate[i];
            break;
        }
        case _ep_dot_type:
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)ent->dot_atk[i];
            break;
        }
        case _ep_dot_owner:
        {
            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)ent->dot_owner[i];
            break;
        }
        break;
        }
    }
    case _ep_dropframe:
    {
        ltemp = 0;
        if(paramCount == 3)
        {
            arg = varlist[2];
            if(FAILED(ScriptVariant_IntegerValue(arg, &ltemp)))
            {
                ltemp = 0;
            }
        }

        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

        if(ent->modeldata.animation[ltemp]->dropframe)
        {
            (*pretvar)->lVal = ent->modeldata.animation[ltemp]->dropframe->frame;
        }

        break;
    }
    case _ep_edelay:
    {
        arg = varlist[2];
        if(arg->vt != VT_INTEGER)
        {
            printf("You must provide a string name for edelay subproperty.\n\
	~'cap_max'\n\
	~'cap_min'\n\
	~'factor'\n\
	~'mode'\n\
	~'range_max'\n\
	~'range_min'\n");
            *pretvar = NULL;
            return E_FAIL;
        }
        ltemp = arg->lVal;
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

        switch(ltemp)
        {
        case _ep_edelay_mode:
        {
            (*pretvar)->lVal = (LONG)ent->modeldata.edelay.mode;
            break;
        }
        case _ep_edelay_factor:
        {
            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)ent->modeldata.edelay.factor;
            break;
        }
        case _ep_edelay_cap_min:
        {
            (*pretvar)->lVal = (LONG)ent->modeldata.edelay.cap.min;
            break;
        }
        case _ep_edelay_cap_max:
        {
            (*pretvar)->lVal = (LONG)ent->modeldata.edelay.cap.max;
            break;
        }
        case _ep_edelay_range_min:
        {
            (*pretvar)->lVal = (LONG)ent->modeldata.edelay.range.min;
            break;
        }
        case _ep_edelay_range_max:
        {
            (*pretvar)->lVal = (LONG)ent->modeldata.edelay.range.max;
            break;
        }
        default:
            *pretvar = NULL;
            return E_FAIL;
        }
        break;
    }
    case _ep_edge:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->edge;
        break;
    }
    case _ep_edgerange:
    {
        if(paramCount < 3)
        {
            break;
        }
        arg = varlist[2];
        if(arg->vt != VT_INTEGER)
        {
            printf("You must give a string name for edgerange.\n");
            return E_FAIL;
        }
        ltemp = arg->lVal;

        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        switch(ltemp)
        {
        case _ep_edgerange_x:
            (*pretvar)->dblVal = (DOUBLE)ent->modeldata.edgerange.x;
            break;
        case _ep_edgerange_z:
            (*pretvar)->dblVal = (DOUBLE)ent->modeldata.edgerange.z;
            break;
        default:
            ScriptVariant_Clear(*pretvar);
            return E_FAIL;
        }
        break;
    }
    case _ep_energycost:
    {
        if(paramCount < 4)
        {
            break;
        }

        if(varlist[2]->vt != VT_INTEGER)
        {
            printf("You must provide a string name for energycost.\n\
	~'cost'\n\
	~'disable'\n\
	~'mponly'\n");
            *pretvar = NULL;
            return E_FAIL;
        }
        ltemp	= varlist[2]->lVal;												//Subproperty.
        i		= varlist[3]->lVal;												//Animation.

        if(!validanim(ent, i))													//Verify animation.
        {
            break;
        }

        switch(ltemp)
        {
        case _ep_energycost_cost:
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            if(ent->modeldata.animation[i]->energycost)
            {
                (*pretvar)->lVal = ent->modeldata.animation[i]->energycost->cost;
            }

            break;
        case _ep_energycost_disable:
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            if(ent->modeldata.animation[i]->energycost)
            {
                (*pretvar)->lVal = ent->modeldata.animation[i]->energycost->disable;
            }
            break;
        case _ep_energycost_mponly:
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            if(ent->modeldata.animation[i]->energycost)
            {
                (*pretvar)->lVal = ent->modeldata.animation[i]->energycost->mponly;
            }
            break;
        default:
            *pretvar = NULL;
            return E_FAIL;
        }
        break;
    }
    case _ep_escapecount:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->escapecount;
        break;
    }
    case _ep_escapehits:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.escapehits;
        break;
    }
    case _ep_exists:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->exists;
        break;
    }
    case _ep_facing:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.facing;
        break;
    }
    case _ep_falldie:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.falldie;
        break;
    }
    case _ep_flash:
    {
        arg = varlist[2];
        if(arg->vt != VT_INTEGER)
        {
            printf("You must give a string name for flash property.\n");
            *pretvar = NULL;
            return E_FAIL;
        }
        ltemp = arg->lVal;
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

        switch(ltemp)
        {
        case _ep_flash_block:
        {
            i = ent->modeldata.bflash;
            break;
        }
        case _ep_flash_def:
        {
            i = ent->modeldata.flash;
            break;
        }
        case _ep_flash_noattack:
        {
            i = ent->modeldata.noatflash;
            break;
        }
        default:
        {
            *pretvar = NULL;
            return E_FAIL;
        }
        }
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)i;
        break;
    }
    case _ep_pain_time:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->pain_time;
        break;
    }
    case _ep_freezetime:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->freezetime;
        break;
    }
    case _ep_frozen:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->frozen;
        break;
    }
    case _ep_gfxshadow:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.gfxshadow;
        break;
    }
    case _ep_shadowbase:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.shadowbase;
        break;
    }
    case _ep_grabbing:
    {
        if(ent->grabbing) // always return an empty var if it is NULL
        {
            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)ent->grabbing;
        }
        break;
    }
    case _ep_grabforce:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.grabforce;
        break;
    }
    case _ep_guardpoints:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.guardpoints.current;
        break;
    }
    case _ep_hasplatforms:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.hasPlatforms;
        break;
    }
    case _ep_health:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->energy_status.health_current;
        break;
    }
    case _ep_height:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.size.y;
        break;
    }
    case _ep_hitbyid:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->attack_id_incoming;
        break;
    }
    case _ep_hitheadplatform:
    {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = (VOID *)ent->hithead;
        break;
    }
    case _ep_landedplatform:
    {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = (VOID *)ent->landed_on_platform;
        break;
    }
    case _ep_hitwall:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->hitwall;
        break;
    }
    case _ep_hmapl:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.maps.hide_start;
        break;
    }
    case _ep_hmapu:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.maps.hide_end;
        break;
    }
    case _ep_icon:
    {
        arg = varlist[2];
        if(arg->vt != VT_INTEGER)
        {
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
            *pretvar = NULL;
            return E_FAIL;
        }
        ltemp = arg->lVal;
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

        switch(ltemp)
        {
        case _ep_icon_def:
        {
            i = ent->modeldata.icon.def;
            break;
        }
        case _ep_icon_die:
        {
            i = ent->modeldata.icon.die;
            break;
        }
        case _ep_icon_get:
        {
            i = ent->modeldata.icon.get;
            break;
        }
        case _ep_icon_mphigh:
        {
            i = ent->modeldata.icon.mphigh;
            break;
        }
        case _ep_icon_mplow:
        {
            i = ent->modeldata.icon.mplow;
            break;
        }
        case _ep_icon_mpmed:
        {
            i = ent->modeldata.icon.mpmed;
            break;
        }
        case _ep_icon_pain:
        {
            i = ent->modeldata.icon.pain;
            break;
        }
        case _ep_icon_weapon:
        {
            i = ent->modeldata.icon.weapon;
            break;
        }
        case _ep_icon_x:
        {
            i = ent->modeldata.icon.position.x;
            break;
        }
        case _ep_icon_y:
        {
            i = ent->modeldata.icon.position.y;
            break;
        }
        default:
        {
            *pretvar = NULL;
            return E_FAIL;
        }
        }

        if (i >= 0)
        {
            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            spr = sprite_map[i].node->sprite;
            spr->centerx = sprite_map[i].centerx;
            spr->centery = sprite_map[i].centery;
            (*pretvar)->ptrVal = (VOID *)(spr);
        }
        else
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = -1;
        }
        break;
    }
    case _ep_invincible:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->invincible;
        break;
    }
    case _ep_invinctime:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->invinctime;
        break;
    }
    case _ep_jugglepoints:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.jugglepoints.current;
        break;
    }
    case _ep_jumpheight:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->modeldata.jumpheight;
        break;
    }
    case _ep_jumpmovex:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.jumpmovex;
        break;
    }
    case _ep_jumpmovez:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.jumpmovez;
        break;
    }
    case _ep_jumpspeed:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->modeldata.jumpspeed;
        break;
    }
    case _ep_knockdowncount:
    {
        /*
        2011_04_14, DC: Backward compatability; default to current if subproperty not provided.
        */
        if(paramCount < 3)
        {
            ltemp = _ep_knockdowncount_current;
        }
        else
        {
            arg = varlist[2];

            if(arg->vt != VT_INTEGER)
            {
                printf("You must provide a string name for knockdowncount subproperty:\n\
		getentityproperty({ent}, 'knockdowncount', {subproperty})\n\
		~'current'\n\
		~'max'\n\
		~'time'\n");
                *pretvar = NULL;
                return E_FAIL;
            }

            ltemp = arg->lVal;
        }

        switch(ltemp)
        {
        case _ep_knockdowncount_current:
            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)ent->knockdowncount;
            break;
        case _ep_knockdowncount_max:
            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)ent->modeldata.knockdowncount;
            break;
        case _ep_knockdowncount_time:
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)ent->knockdowncount;
            break;
        default:
            *pretvar = NULL;
            return E_FAIL;
        }
        break;
    }
    case _ep_komap:
    {
        if(paramCount < 2)
        {
            break;
        }

        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.maps.ko;
        break;
    }
    case _ep_landframe:
    {
        if(paramCount < 4)
        {
            break;
        }

        if(varlist[2]->vt != VT_INTEGER
                || varlist[3]->vt != VT_INTEGER)
        {
            printf("\n Error, getentityproperty({ent}, 'landframe', {sub property}, {animation}): {Sub property} or {Animation} parameter is missing or invalid. \n");
            *pretvar = NULL;
            return E_FAIL;
        }
        ltemp	= varlist[2]->lVal;												//Subproperty.
        i		= varlist[3]->lVal;												//Animation.

        if(!validanim(ent, i))													//Verify animation.
        {
            break;
        }

        // entity must have a land frame set.
        if(!ent->modeldata.animation[i]->landframe)
        {
            break;
        }

        switch(ltemp)
        {
        case _ep_landframe_ent:
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)ent->modeldata.animation[i]->landframe->ent;
            break;
        case _ep_landframe_frame:
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)ent->modeldata.animation[i]->landframe->frame;
            break;
        default:
            *pretvar = NULL;
            return E_FAIL;
        }
        break;
    }
    case _ep_lifespancountdown:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->lifespancountdown;
        break;
    }
    case _ep_attackthrottle:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->modeldata.attackthrottle;
        break;
    }
    case _ep_attackthrottletime:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->modeldata.attackthrottletime;
        break;
    }
    case _ep_link:
    {
        if(ent->link) // always return an empty var if it is NULL
        {
            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)ent->link;
        }
        break;
    }
    case _ep_map:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)0;
        for(i = 0; i < ent->modeldata.maps_loaded; i++)
        {
            if(ent->colourmap == ent->modeldata.colourmap[i])
            {
                (*pretvar)->lVal = (LONG)(i + 1);
                break;
            }
        }
        break;
    }
    case _ep_mapcount:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)(ent->modeldata.maps_loaded + 1);
        break;
    }
    case _ep_mapdefault:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)(ent->map);
        break;
    }
    case _ep_maps:
    {
        arg = varlist[2];
        if(arg->vt != VT_INTEGER)
        {
            printf("You must give a string name for maps property.\n");
            *pretvar = NULL;
            return E_FAIL;
        }
        ltemp = arg->lVal;
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

        switch(ltemp)
        {
        case _ep_maps_count:
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)(ent->modeldata.maps_loaded + 1);
            break;
        }

        case _ep_maps_current:
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)0;
            for(i = 0; i < ent->modeldata.maps_loaded; i++)
            {
                if(ent->colourmap == ent->modeldata.colourmap[i])
                {
                    (*pretvar)->lVal = (LONG)(i + 1);
                    break;
                }
            }
            break;
        }
        case _ep_maps_dying:
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            if(paramCount >= 3)
            {
                if(FAILED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
                {
                    printf("You must specify the integer value for remap.\n");
                    *pretvar = NULL;
                    return E_FAIL;
                }
                else
                {
                    if (ltemp == 0) (*pretvar)->lVal = (LONG)(ent->dying);
                    else (*pretvar)->lVal = (LONG)(ent->dying2);
                }
            }
            else
            {
                (*pretvar)->lVal = (LONG)(ent->dying);
            }
            break;
        }
        case _ep_maps_dying_critical:
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)(ent->per2);
            break;
        }
        case _ep_maps_dying_low:
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)(ent->per1);
            break;
        }
        case _ep_maps_frozen:
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)(ent->modeldata.maps.frozen);
            break;
        }
        case _ep_maps_hide_end:
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)(ent->modeldata.maps.hide_end);
            break;
        }
        case _ep_maps_hide_start:
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)(ent->modeldata.maps.hide_start);
            break;
        }
        case _ep_maps_ko:
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)(ent->modeldata.maps.ko);
            break;
        }
        case _ep_maps_kotype:
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)(ent->modeldata.maps.kotype);
            break;
        }
        case _ep_maps_table:
        {
            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)(ent->colourmap);
            break;
        }
        case _ep_maps_time:
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)(ent->maptime);
            break;
        }
        default:
        {
            *pretvar = NULL;
            return E_FAIL;
        }
        }
        break;
    }
    case _ep_maxguardpoints:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.guardpoints.max;
        break;
    }
    case _ep_maxhealth:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.health;
        break;
    }
    case _ep_maxjugglepoints:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.jugglepoints.max;
        break;
    }
    case _ep_maxmp:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.mp;
        break;
    }
    case _ep_model:
    {
        ScriptVariant_ChangeType(*pretvar, VT_STR);
        (*pretvar)->strVal = StrCache_CreateNewFrom(ent->model->name);
        break;
    }
    case _ep_movex:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->movex;
        break;
    }
    case _ep_movez:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->movex;
        break;
    }
    case _ep_entitypushing:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.entitypushing;
        break;
    }
    case _ep_pushingfactor:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->modeldata.pushingfactor;
        break;
    }
    case _ep_mp:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->energy_status.mp_current;
        break;
    }
    case _ep_mpdroprate:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.mpdroprate;
        break;
    }
    case _ep_mprate:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.mprate;
        break;
    }
    case _ep_mpstable:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.mpstable;
        break;
    }
    case _ep_mpstableval:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.mpstableval;
        break;
    }
    case _ep_name:
    {
        ScriptVariant_ChangeType(*pretvar, VT_STR);
        (*pretvar)->strVal = StrCache_CreateNewFrom(ent->name);
        break;
    }
    case _ep_nextanim:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->nextanim;
        break;
    }
    case _ep_nextmove:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->nextmove;
        break;
    }
    case _ep_nextthink:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->nextthink;
        break;
    }
    case _ep_no_adjust_base:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.no_adjust_base;
        break;
    }
    case _ep_noaicontrol:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->noaicontrol;
        break;
    }
    case _ep_nodieblink:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.nodieblink;
        break;
    }
    case _ep_nodrop:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.nodrop;
        break;
    }
    case _ep_nograb:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->nograb;
        break;
    }
    case _ep_nohithead:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.nohithead;
        break;
    }
    case _ep_nolife:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.nolife;
        break;
    }
    case _ep_nopain:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.nopain;
        break;
    }
    case _ep_offense:
    {
        ltemp = 0;
        if(paramCount >= 3)
        {
            arg = varlist[2];
            if(FAILED(ScriptVariant_IntegerValue(arg, &ltemp)))
            {
                ltemp = (LONG)0;
            }
        }
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->offense_factors[(LONG)ltemp];
        break;
    }
    case _ep_offscreen_noatk_factor:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->modeldata.offscreen_noatk_factor;
        break;
    }
    case _ep_offscreenkill:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.offscreenkill;
        break;
    }
    case _ep_opponent:
    {
        if(ent->opponent) // always return an empty var if it is NULL
        {
            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)ent->opponent;
        }
        break;
    }
    case _ep_custom_target:
    {
        if(ent->custom_target) // always return an empty var if it is NULL
        {
            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)ent->custom_target;
        }
        break;
    }
    case _ep_owner:
    {
        if(ent->owner) // always return an empty var if it is NULL
        {
            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)ent->owner;
        }
        break;
    }
    case _ep_parent:
    {
        if(ent->parent) // always return an empty var if it is NULL
        {
            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)ent->parent;
        }
        break;
    }
    case _ep_path:
    {
        ScriptVariant_ChangeType(*pretvar, VT_STR);
        (*pretvar)->strVal = StrCache_CreateNewFrom(ent->modeldata.path);
        break;
    }
    case _ep_pathfindstep:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->modeldata.pathfindstep;
        //printf("%d %s %d\n", ent->sortid, ent->name, ent->playerindex);
        break;
    }
    case _ep_playerindex:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->playerindex;
        //printf("%d %s %d\n", ent->sortid, ent->name, ent->playerindex);
        break;
    }
    case _ep_projectile:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->projectile;
        break;
    }
    case _ep_range:
    {
        if(paramCount < 4)
        {
            break;
        }

        if(varlist[2]->vt != VT_INTEGER
                || varlist[3]->vt != VT_INTEGER)
        {
            printf("\n Error, getentityproperty({ent}, 'range', {sub property}, {animation}): {Sub property} or {Animation} parameter is missing or invalid. \n");
            *pretvar = NULL;
            return E_FAIL;
        }
        ltemp	= varlist[2]->lVal;												//Subproperty.
        i		= varlist[3]->lVal;												//Animation.

        if(!validanim(ent, i))													//Verify animation.
        {
            break;
        }

        switch(ltemp)
        {
        case _ep_range_amax:
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)ent->modeldata.animation[i]->range.y.max;
            break;
        case _ep_range_amin:
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)ent->modeldata.animation[i]->range.y.min;
            break;
        case _ep_range_bmax:
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)ent->modeldata.animation[i]->range.base.max;
            break;
        case _ep_range_bmin:
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)ent->modeldata.animation[i]->range.base.min;
            break;
        case _ep_range_xmax:
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)ent->modeldata.animation[i]->range.x.max;
            break;
        case _ep_range_xmin:
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)ent->modeldata.animation[i]->range.x.min;
            break;
        case _ep_range_zmax:
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)ent->modeldata.animation[i]->range.z.max;
            break;
        case _ep_range_zmin:
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)ent->modeldata.animation[i]->range.z.min;
            break;
        default:
            *pretvar = NULL;
            return E_FAIL;
        }
        break;
    }
    case _ep_running:
    {
        if(paramCount < 3)
        {
            break;
        }
        arg = varlist[2];
        if(arg->vt != VT_INTEGER)
        {
            printf("You must give a string name for running property.\n");
            *pretvar = NULL;
            return E_FAIL;
        }
        ltemp = arg->lVal;
        switch(ltemp)
        {
        case _ep_running_speed:
        {
            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)ent->modeldata.runspeed;
            break;
        }
        case _ep_running_jumpy:
        {
            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)ent->modeldata.runjumpheight;
            break;
        }
        case _ep_running_jumpx:
        {
            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)ent->modeldata.runjumpdist;
            break;
        }
        case _ep_running_land:
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)ent->modeldata.runhold;
            break;
        }
        case _ep_running_movez:
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)ent->modeldata.runupdown;
            break;
        }
        }
        break;
    }
    case _ep_rush_count:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->rush.count.current;
        break;
    }
    case _ep_rush_tally:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->rush.count.max;
        break;
    }
    case _ep_rush_time:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->rush.time;
        break;
    }
    case _ep_score:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.score;
        break;
    }
    case _ep_scroll:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->modeldata.scroll;
        break;
    }
    case _ep_seal:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->seal;
        break;
    }
    case _ep_sealtime:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->sealtime;
        break;
    }
    case _ep_setlayer:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.setlayer;
        break;
    }
    case _ep_sortid:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->sortid;
        break;
    }
    case _ep_spawntype:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->spawntype;
        break;
    }
    case _ep_speed:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->modeldata.speed;
        break;
    }
    case _ep_sprite:
    {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        i = ent->animation->sprite[ent->animpos];
        spr = sprite_map[i].node->sprite;
        spr->centerx = sprite_map[i].centerx;
        spr->centery = sprite_map[i].centery;
        (*pretvar)->ptrVal = (VOID *)(spr);
        break;
    }
    case _ep_spritea:
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
        case _ep_spritea_centerx:
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)sprite_map[i].centerx;
            break;
        }
        case _ep_spritea_centery:
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)sprite_map[i].centery;
            break;
        }
        case _ep_spritea_file:
        {
            ScriptVariant_ChangeType(*pretvar, VT_STR);
            (*pretvar)->strVal = StrCache_CreateNewFrom(sprite_map[i].node->filename);
            break;
        }
        case _ep_spritea_offsetx:
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)sprite_map[i].node->sprite->offsetx;
            break;
        }
        case _ep_spritea_offsety:
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)sprite_map[i].node->sprite->offsety;
            break;
        }
        case _ep_spritea_sprite:
        {
            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            spr = sprite_map[i].node->sprite;
            spr->centerx = sprite_map[i].centery;
            spr->centery = sprite_map[i].centery;
            (*pretvar)->ptrVal = (VOID *)(spr);
            break;
        }
        default:
        {
            *pretvar = NULL;
            return E_FAIL;
        }
        }

        break;

    }
    case _ep_stalltime:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->stalltime;
        break;
    }
    case _ep_releasetime:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->releasetime;
        break;
    }
    case _ep_staydown:
    {
        arg = varlist[2];
        if(arg->vt != VT_INTEGER)
        {

            printf("You must provide a string name for staydown property:\n\
	getentityproperty({ent}, 'staydown', {subproperty})\n\
	~'rise'\n\
	~'riseattack'\n\
	~'riseattack_stall' \n");
            *pretvar = NULL;
            return E_FAIL;
        }
        ltemp = arg->lVal;
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

        switch(ltemp)
        {
        case _ep_staydown_rise:
        {
            i = ent->staydown.rise;
            break;
        }
        case _ep_staydown_riseattack:
        {
            i = ent->staydown.riseattack;
            break;
        }
        case _ep_staydown_riseattack_stall:
        {
            i = ent->staydown.riseattack_stall;
            break;
        }
        default:
        {
            *pretvar = NULL;
            return E_FAIL;
        }
        }
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)i;
        break;
    }
    case _ep_stealth:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.stealth.hide;
        break;
    }
    case _ep_subentity:
    {
        if(ent->subentity) // always return an empty var if it is NULL
        {
            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)ent->subentity;
        }
        break;
    }
    case _ep_subject_to_basemap:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.subject_to_basemap;
        break;
    }
    case _ep_subject_to_gravity:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.subject_to_gravity;
        break;
    }
    case _ep_subject_to_hole:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.subject_to_hole;
        break;
    }
    case _ep_subject_to_maxz:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.subject_to_maxz;
        break;
    }
    case _ep_subject_to_minz:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.subject_to_minz;
        break;
    }
    case _ep_subject_to_obstacle:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.subject_to_obstacle;
        break;
    }
    case _ep_subject_to_platform:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.subject_to_platform;
        break;
    }
    case _ep_subject_to_screen:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.subject_to_screen;
        break;
    }
    case _ep_subject_to_wall:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.subject_to_wall;
        break;
    }
    case _ep_subtype:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.subtype;
        break;
    }
    case _ep_thold:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.thold;
        break;
    }
    case _ep_throwdamage:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.throwdamage;
        break;
    }
    case _ep_throwdist:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->modeldata.throwdist;
        break;
    }
    case _ep_throwframewait:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.throwframewait;
        break;
    }
    case _ep_throwheight:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->modeldata.throwheight;
        break;
    }
    case _ep_tosstime:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->toss_time;
        break;
    }
    case _ep_tossv:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->velocity.y;
        break;
    }
    case _ep_type:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.type;
        break;
    }
    case _ep_walkoffmovex:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.walkoffmovex;
        break;
    }
    case _ep_walkoffmovez:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.walkoffmovez;
        break;
    }
    case _ep_weapent:
    {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = (VOID *)ent->weapent;
        break;
    }
    case _ep_numweapons:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.numweapons;
        break;
    }
    case _ep_weapnum:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.weapnum;
        break;
    }
    case _ep_weaploss:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        if(paramCount >= 3)
        {
            if(FAILED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            {
                printf("You must specify the flag value.\n");
                *pretvar = NULL;
                return E_FAIL;
            }

            if (ltemp == 0) (*pretvar)->lVal = (LONG)ent->modeldata.weaploss[0];
            else (*pretvar)->lVal = (LONG)ent->modeldata.weaploss[1];
        }
        else (*pretvar)->lVal = (LONG)ent->modeldata.weaploss[0];

        break;
    }
    case _ep_x:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->position.x;
        break;
    }
    case _ep_xdir:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->velocity.x;
        break;
    }
    case _ep_z:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->position.z;
        break;
    }
    case _ep_zdir:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->velocity.z;
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


//changeentityproperty(pentity, propname, value1[ ,value2, value3, ...]);
HRESULT openbor_changeentityproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    entity *ent = NULL;
    s_model *tempmodel ;
    char *tempstr = NULL;
    LONG ltemp, ltemp2;
    DOUBLE dbltemp;
    int propind;
    int i = 0;

    static const void *actions[] =   // for takeaction
    {
        bomb_explode,
        common_animation_normal,
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
        normal_prepare,
        npc_warp,
        player_blink,
        suicide,
    };

    static const int entitytypes[] =
    {
        0, // "ground"; not a real entity type
        TYPE_ENEMY,
        TYPE_NPC,
        TYPE_OBSTACLE,
        TYPE_PLAYER,
        TYPE_SHOT,

    };

    static const void *think[] =   // 2011_03_03, DC: Think types.
    {
        common_think,
        player_think,
        steam_think,
        steamer_think,
        text_think,
        trap_think,
    };

    *pretvar = NULL;

    if(paramCount < 3)
    {
        printf("Function changeentityproperty must have have at least 3 parameters.");
        goto changeentityproperty_error;
    }

    mapstrings_entityproperty(varlist, paramCount);

    if(varlist[0]->vt != VT_PTR && varlist[0]->vt != VT_EMPTY)
    {
        printf("Function changeentityproperty must have a valid entity handle.");
        goto changeentityproperty_error;
    }
    ent = (entity *)varlist[0]->ptrVal; //retrieve the entity
    if(!ent)
    {
        return S_OK;
    }

    if(varlist[1]->vt != VT_INTEGER)
    {
        if(varlist[1]->vt != VT_STR)
        {
            printf("Function changeentityproperty must have a string property name.\n");
        }
        goto changeentityproperty_error;
    }

    propind = varlist[1]->lVal;

    switch(propind)
    {
    case _ep_aggression:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.aggression = (LONG)ltemp;
        }
        break;
    }
    case _ep_aiattack:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.aiattack = (LONG)ltemp;
        }
        break;
    }
    case _ep_aiflag:
    {
        if(varlist[2]->vt != VT_INTEGER)
        {
            if(varlist[2]->vt != VT_STR)
            {
                printf("You must give a string value for AI flag name.\n");
            }
            goto changeentityproperty_error;
        }
        if(paramCount < 4)
        {
            break;
        }

        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
        {
            switch(varlist[2]->lVal)
            {
            case _ep_aiflag_dead:
                ent->dead = (LONG)ltemp;
                break;
            case _ep_aiflag_jumpid:
                ent->jump.animation_id = (LONG)ltemp;
                break;
            case _ep_aiflag_jumping:
                ent->jumping = (LONG)ltemp;
                break;
            case _ep_aiflag_idling:
                ent->idling = (LONG)ltemp;
                break;
            case _ep_aiflag_drop:
                ent->drop = (LONG)ltemp;
                break;
            case _ep_aiflag_attacking:
                ent->attacking = (LONG)ltemp;
                break;
            case _ep_aiflag_getting:
                ent->getting = (LONG)ltemp;
                break;
            case _ep_aiflag_turning:
                ent->turning = (LONG)ltemp;
                break;
            case _ep_aiflag_charging:
                ent->charging = (LONG)ltemp;
                break;
            case _ep_aiflag_blocking:
                ent->blocking = (LONG)ltemp;
                break;
            case _ep_aiflag_ducking:
                ent->ducking = (LONG)ltemp;
                break;
            case _ep_aiflag_falling:
                ent->falling = (LONG)ltemp;
                break;
            case _ep_aiflag_running:
                ent->running = (LONG)ltemp;
                break;
            case _ep_aiflag_inpain:
                ent->inpain = (LONG)ltemp;
                break;
            case _ep_aiflag_rising:
                ent->rising = (LONG)ltemp;
                break;
            case _ep_aiflag_riseattacking:
                ent->riseattacking = (LONG)ltemp;
                break;
            case _ep_aiflag_inbackpain:
                ent->inbackpain = (LONG)ltemp;
                break;
            case _ep_aiflag_projectile:
                ent->projectile = (LONG)ltemp;
                break;
            case _ep_aiflag_frozen:
                ent->frozen = (LONG)ltemp;
                break;
            case _ep_aiflag_toexplode:
                ent->toexplode = (LONG)ltemp;
                break;
            case _ep_aiflag_animating:
                ent->animating = (LONG)ltemp;
                break;
            case _ep_aiflag_blink:
                ent->blink = (LONG)ltemp;
                break;
            case _ep_aiflag_invincible:
                ent->invincible = (LONG)ltemp;
                break;
            case _ep_aiflag_autokill:
                ent->autokill = (LONG)ltemp;
                break;
            case _ep_aiflag_idlemode:
                ent->idlemode = (LONG)ltemp;
                break;
            case _ep_aiflag_walkmode:
                ent->walkmode = (LONG)ltemp;
                break;
            case _ep_aiflag_walking:
                ent->walking = (LONG)ltemp;
                break;
            default:
                printf("Unknown AI flag.\n");
                goto changeentityproperty_error;
            }
        }
        break;
    }
    case _ep_aimove:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.aimove = (LONG)ltemp;
        }
        break;
    }
    case _ep_alpha:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.alpha = (LONG)ltemp;
        }
        break;
    }
    case _ep_animation:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ltemp2 = (LONG)1;
            if(paramCount < 4 || SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp2)))
            {
                ent_set_anim(ent, (LONG)ltemp, (LONG)ltemp2);
            }
        }
        break;
    }
    case _ep_animhits:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->animation->animhits = (LONG)ltemp;
        }
        break;
    }
    case _ep_animpos:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->animpos = (LONG)ltemp;
        }
        break;
    }
    case _ep_antigrab:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.antigrab = (LONG)ltemp;
        }
        break;
    }
    case _ep_antigravity:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->modeldata.antigravity = (DOUBLE)dbltemp;
        }
        break;
    }

    case _ep_attacking:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->attacking = (LONG)ltemp;
        }
        break;
    }
    case _ep_attackid:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->attack_id_outgoing = (LONG)ltemp;
        }
        break;
    }
    case _ep_autokill:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->autokill = (LONG)ltemp;
        }
        break;
    }
    case _ep_base:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->base = (DOUBLE)dbltemp;
        }
        break;
    }
    case _ep_blink:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->blink = (LONG)ltemp;
        }
        break;
    }
    case _ep_blockback:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.blockback = (LONG)ltemp;
        }
        break;
    }
    case _ep_blockodds:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.blockodds = (LONG)ltemp;
        }
        break;
    }
    case _ep_blockpain:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.blockpain = (LONG)ltemp;
        }
        break;
    }
    case _ep_boss:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->boss = (LONG)ltemp;
        }
        break;
    }
    case _ep_bounce:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.bounce = (LONG)ltemp;
        }
        break;
    }
    case _ep_candamage:
    {
        ent->modeldata.candamage = 0;

        for(i = 2; i < paramCount; i++)
        {
            if(varlist[i]->vt == VT_INTEGER) // known entity type
            {
                ltemp = varlist[i]->lVal;
                if(ltemp == (_ep_hcd_ground | 0x80000000)) // "ground" - not needed?
                {
                    ent->modeldata.ground = 1;
                }
                else if(ltemp & 0x80000000)
                {
                    ent->modeldata.candamage |= entitytypes[ltemp & 0x7fffffff];
                }
                else
                {
                    ent->modeldata.candamage |= ltemp;
                }
            }
            else
            {
                printf("You must pass one or more string constants for candamage entity type.\n");
                goto changeentityproperty_error;
            }
        }
        break;
    }
    case _ep_combostep:
    {
        if(paramCount >= 4 &&
                SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)) &&
                SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp2)))
        {
            ent->combostep[(LONG)ltemp] = (LONG)ltemp2;
        }
        break;
    }
    case _ep_combotime:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->combotime = (LONG)ltemp;
        }
        break;
    }
    case _ep_collidedentity:
    {
        ent->collided_entity = (entity *)varlist[2]->ptrVal;
        break;
    }
    case _ep_colourmap:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->colourmap = (VOID *)model_get_colourmap(&(ent->modeldata), ltemp);
        }
        break;
    }
    case _ep_damage_on_landing:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->damage_on_landing.attack_force = (LONG)ltemp;
        }
        break;
    }
    case _ep_dead:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->dead = (LONG)ltemp;
        }
        break;
    }
    case _ep_defaultname:
    {
        if(varlist[2]->vt != VT_STR)
        {
            printf("You must give a string value for entity name.\n");
            goto changeentityproperty_error;
        }
        tempmodel = findmodel((char *)StrCache_Get(varlist[2]->strVal));
        if(!tempmodel)
        {
            printf("Use must give an existing model's name for entity's default model name.\n");
            goto changeentityproperty_error;
        }
        ent->defaultmodel = tempmodel;
        break;
    }
    case _ep_defense:
    {
        if((ltemp2 =
                    (paramCount >= 4 &&
                     SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)) &&
                     ltemp < (LONG)MAX_ATKS && ltemp >= (LONG)0 &&
                     SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
           ))
        {
            ent->defense[(LONG)ltemp].factor = (DOUBLE)dbltemp;
        }

        if(paramCount >= 5 && ltemp2 && (ltemp2 = SUCCEEDED(ScriptVariant_DecimalValue(varlist[4], &dbltemp))))
        {
            ent->defense[(LONG)ltemp].pain = (DOUBLE)dbltemp;
        }
        if(paramCount >= 6 && ltemp2 && (ltemp2 = SUCCEEDED(ScriptVariant_DecimalValue(varlist[5], &dbltemp))))
        {
            ent->defense[(LONG)ltemp].knockdown = (DOUBLE)dbltemp;
        }
        if(paramCount >= 7 && ltemp2 && (ltemp2 = SUCCEEDED(ScriptVariant_DecimalValue(varlist[6], &dbltemp))))
        {
            ent->defense[(LONG)ltemp].blockpower = (DOUBLE)dbltemp;
        }
        if(paramCount >= 8 && ltemp2 && (ltemp2 = SUCCEEDED(ScriptVariant_DecimalValue(varlist[7], &dbltemp))))
        {
            ent->defense[(LONG)ltemp].blockthreshold = (DOUBLE)dbltemp;
        }
        if(paramCount >= 9 && ltemp2 && (ltemp2 = SUCCEEDED(ScriptVariant_DecimalValue(varlist[8], &dbltemp))))
        {
            ent->defense[(LONG)ltemp].blockratio = (DOUBLE)dbltemp;
        }
        if(paramCount >= 10 && ltemp2 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[9], &dbltemp)))
        {
            ent->defense[(LONG)ltemp].blocktype = dbltemp;
        }

        break;
    }
    case _ep_destx:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->destx = (DOUBLE)dbltemp;
        }
        break;
    }
    case _ep_destz:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->destz = (DOUBLE)dbltemp;
        }
        break;
    }
    case _ep_detect:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.stealth.detect = (LONG)ltemp;
        }
        break;
    }
    case _ep_direction:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->direction = (LONG)ltemp;
        }
        break;
    }
    case _ep_dot:
    {
        if((SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp))))
        {
            i = (LONG)ltemp;
            if(paramCount >= 4 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
            {
                ent->dot_time[i] = (int)dbltemp;
            }
            if(paramCount >= 5 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[4], &dbltemp)))
            {
                ent->dot[i] = (int)dbltemp;
            }
            if(paramCount >= 6 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[5], &dbltemp)))
            {
                ent->dot_force[i] = (int)dbltemp;
            }
            if(paramCount >= 7 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[6], &dbltemp)))
            {
                ent->dot_rate[i] = (int)dbltemp;
            }
            if(paramCount >= 8 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[7], &dbltemp)))
            {
                ent->dot_atk[i] = (int)dbltemp;
            }
            if(paramCount >= 9)
            {
                ent->dot_owner[i] = (entity *)varlist[8]->ptrVal;
            }
        }
        break;
    }
    case _ep_edelay:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.edelay.mode = (LONG)ltemp;
        }
        if(paramCount >= 3 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
        {
            ent->modeldata.edelay.factor = (DOUBLE)dbltemp;
        }
        if(paramCount >= 4 && SUCCEEDED(ScriptVariant_IntegerValue(varlist[4], &ltemp)))
        {
            ent->modeldata.edelay.cap.min = (LONG)ltemp;
        }
        if(paramCount >= 5 && SUCCEEDED(ScriptVariant_IntegerValue(varlist[5], &ltemp)))
        {
            ent->modeldata.edelay.cap.max = (LONG)ltemp;
        }
        if(paramCount >= 6 && SUCCEEDED(ScriptVariant_IntegerValue(varlist[6], &ltemp)))
        {
            ent->modeldata.edelay.range.min = (LONG)ltemp;
        }
        if(paramCount >= 7 && SUCCEEDED(ScriptVariant_IntegerValue(varlist[7], &ltemp)))
        {
            ent->modeldata.edelay.range.max = (LONG)ltemp;
        }

        break;
    }
    case _ep_edge:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->edge = (LONG)ltemp;
        }
        break;
    }
    case _ep_edgerange:
    {
        if(varlist[2]->vt != VT_INTEGER)
        {
            if(varlist[2]->vt != VT_STR)
            {
                printf("You must give a string value for edgerange name.\n");
            }
            goto changeentityproperty_error;
        }
        if(paramCount < 4)
        {
            break;
        }

        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
        {
            switch(varlist[2]->lVal)
            {
            case _ep_edgerange_x:
                ent->modeldata.edgerange.x = (DOUBLE)dbltemp;
                break;
            case _ep_edgerange_z:
                ent->modeldata.edgerange.z = (DOUBLE)dbltemp;
                break;
            default:
                printf("Unknown edgerange.\n");
                goto changeentityproperty_error;
            }
        }
        break;
    }
    case _ep_energycost:
    {
        if(paramCount != 5)
        {
            printf("\n Error, changeentityproperty({ent}, 'energycost', {subproperty}, {animation}, {value}): Invalid or missing parameter. \n");
            goto changeentityproperty_error;
        }

        if(FAILED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
        {
            printf("\n Error, changeentityproperty has invalid animation id.\n");
            goto changeentityproperty_error;
        }

        i = (LONG)ltemp;

        if(!validanim(ent, i))
        {
            printf("\n Error, changeentityproperty({ent}, 'energycost', {subproperty}, {animation}, {value}): {animation} parameter invalid. Make sure the animation exists. \n");
            goto changeentityproperty_error;
        }

        if(varlist[2]->vt != VT_INTEGER)
        {
            printf("You must give a string value for energycost flag name.\n");
            goto changeentityproperty_error;
        }

        switch(varlist[2]->lVal)
        {
        case _ep_energycost_cost:
        {
            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[4], &ltemp)))
            {
                if(ent->modeldata.animation[i]->energycost)
                {
                    ent->modeldata.animation[i]->energycost->cost = ltemp;
                }

            }
            break;
        }
        case _ep_energycost_disable:
        {
            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[4], &ltemp)))
            {
                if(ent->modeldata.animation[i]->energycost)
                {
                    ent->modeldata.animation[i]->energycost->disable = ltemp;
                }
            }
            break;
        }
        case _ep_energycost_mponly:
        {
            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[4], &ltemp)))
            {
                if(ent->modeldata.animation[i]->energycost)
                {
                    ent->modeldata.animation[i]->energycost->mponly = ltemp;
                }
            }
            break;
        }
        default:
            printf("Unknown Energycost flag.\n");
            goto changeentityproperty_error;
        }
        break;
    }
    case _ep_escapecount:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->escapecount = (LONG)ltemp;
        }
        break;
    }
    case _ep_escapehits:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.escapehits = (LONG)ltemp;
        }
        break;
    }
    case _ep_facing:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.facing = (LONG)ltemp;
        }
        break;
    }
    case _ep_falldie:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.falldie = (LONG)ltemp;
        }
        break;
    }
    case _ep_playerindex:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->playerindex = (LONG)ltemp;
        }
        break;
    }
    case _ep_pain_time:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->pain_time = (LONG)ltemp;
        }
        break;
    }
    case _ep_freezetime:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->freezetime = (LONG)ltemp;
        }
        break;
    }
    case _ep_frozen:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->frozen = (LONG)ltemp;
        }
        break;
    }
    case _ep_gfxshadow:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.gfxshadow = (LONG)ltemp;
        }
        break;
    }
    case _ep_shadowbase:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.shadowbase = (LONG)ltemp;
        }
        break;
    }
    case _ep_grabforce:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.grabforce = (LONG)ltemp;
        }
        break;
    }
    case _ep_guardpoints:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.guardpoints.current = (LONG)ltemp;
        }
        break;
    }
    case _ep_hasplatforms:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.hasPlatforms = (LONG)ltemp;
        }
        break;
    }
    case _ep_health:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->energy_status.health_current = (LONG)ltemp;
            if(ent->energy_status.health_current > ent->modeldata.health)
            {
                ent->energy_status.health_current = ent->modeldata.health;
            }
            else if(ent->energy_status.health_current < 0)
            {
                ent->energy_status.health_current = 0;
            }
        }
        break;
    }
    case _ep_hitbyid:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->attack_id_incoming = (LONG)ltemp;
        }
        break;
    }
    case _ep_hitheadplatform:
    {
        ent->hithead = (entity *)varlist[2]->ptrVal;
        break;
    }
    case _ep_landedplatform:
    {
        ent->landed_on_platform = (entity *)varlist[2]->ptrVal;
        break;
    }
    case _ep_hitwall:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->hitwall = (LONG)ltemp;
        }
        break;
    }
    case _ep_hmapl:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.maps.hide_start = ltemp;
        }
        break;
    }
    case _ep_hmapu:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.maps.hide_end = ltemp;
        }
        break;
    }
    case _ep_hostile:
    {
        ent->modeldata.hostile = 0;
        for(i = 2; i < paramCount; i++)
        {
            if(varlist[i]->vt == VT_INTEGER) // known entity type
            {
                ltemp = varlist[i]->lVal;
                if(ltemp & 0x80000000)
                {
                    ent->modeldata.hostile |= entitytypes[ltemp & 0x7fffffff];
                }
                else
                {
                    ent->modeldata.hostile |= ltemp;
                }
            }
            else
            {
                printf("You must pass one or more string constants for hostile entity type.\n");
                goto changeentityproperty_error;
            }
        }

        break;
    }
    case _ep_iconposition:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.icon.position.x = (LONG)ltemp;
        }
        if(paramCount > 3 && SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
        {
            ent->modeldata.icon.position.y = (LONG)ltemp;
        }
        break;
    }
    case _ep_invincible:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->invincible = (LONG)ltemp;
        }
        break;
    }
    case _ep_invinctime:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->invinctime = (LONG)ltemp;
        }
        break;
    }
    case _ep_jugglepoints:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.jugglepoints.current = (LONG)ltemp;
        }
        break;
    }
    case _ep_jumpheight:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->modeldata.jumpheight = (DOUBLE)dbltemp;
        }
        break;
    }
    case _ep_jumpmovex:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.jumpmovex = (LONG)ltemp;
        }
        break;
    }
    case _ep_jumpmovez:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.jumpmovex = (LONG)ltemp;
        }
        break;
    }
    case _ep_jumpspeed:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->modeldata.jumpspeed = (DOUBLE)dbltemp;
        }
        break;
    }
    case _ep_knockdowncount:
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
        case _ep_knockdowncount_current:
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
            {
                ent->knockdowncount = (DOUBLE)dbltemp;
            }
            break;
        }
        case _ep_knockdowncount_max:
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
            {
                ent->modeldata.knockdowncount = (DOUBLE)dbltemp;
            }
            break;
            case _ep_knockdowncount_time:
                if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
                {
                    ent->knockdowntime = (LONG)ltemp;
                }
                break;
            }
        default:
            printf("Unknown knockdowncount subproperty.\n");
            goto changeentityproperty_error;
        }
        break;
    }
    case _ep_komap:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.maps.ko = (LONG)ltemp;
        }
        if(paramCount >= 4 && SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
        {
            ent->modeldata.maps.kotype = (LONG)ltemp;
        }
        break;
    }
    case _ep_lifeposition:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.hpx = (LONG)ltemp;
        }
        if(paramCount > 3 && SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
        {
            ent->modeldata.hpy = (LONG)ltemp;
        }
        break;
    }
    case _ep_lifespancountdown:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->lifespancountdown = (LONG)ltemp;
        }
        break;
    }
    case _ep_attackthrottle:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->modeldata.attackthrottle = (DOUBLE)dbltemp;
        }
        break;
    }
    case _ep_attackthrottletime:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->modeldata.attackthrottletime = (DOUBLE)dbltemp;
        }
        break;
    }
    case _ep_map:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent_set_colourmap(ent, (LONG)ltemp);
        }
        break;
    }
    case _ep_maptime:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->maptime = (LONG)ltemp;
        }
        break;
    }
    case _ep_maxguardpoints:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.guardpoints.max = (LONG)ltemp;
        }
        break;
    }
    case _ep_maxhealth:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.health = (LONG)ltemp;
            if(ent->modeldata.health < 0)
            {
                ent->modeldata.health = 0;    //OK, no need to have ot below 0
            }
        }
        break;
    }
    case _ep_maxjugglepoints:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.jugglepoints.max = (LONG)ltemp;
        }
        break;
    }
    case _ep_maxmp:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.mp = (LONG)ltemp;
            if(ent->modeldata.mp < 0)
            {
                ent->modeldata.mp = 0;    //OK, no need to have ot below 0
            }
        }
        break;
    }
    case _ep_model:
    {
        if(varlist[2]->vt != VT_STR)
        {
            printf("You must give a string value for model name.\n");
            goto changeentityproperty_error;
        }
        tempstr = (char *)StrCache_Get(varlist[2]->strVal);
        if(paramCount > 3 && SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
        {
            set_model_ex(ent, tempstr, -1, NULL, (LONG)ltemp);
            if(!ent->weapent)
            {
                ent->weapent = ent;
            }
        }
        break;
    }
    case _ep_movex:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->movex = (DOUBLE)dbltemp;
        }
        break;
    }
    case _ep_movez:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->movez = (DOUBLE)dbltemp;
        }
        break;
    }
    case _ep_entitypushing:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.entitypushing = (LONG)ltemp;
        }
        break;
    }
    case _ep_pushingfactor:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->modeldata.pushingfactor = (DOUBLE)dbltemp;
        }
        break;
    }
    case _ep_mp:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->energy_status.mp_current = (LONG)ltemp;
            if(ent->energy_status.mp_current > ent->modeldata.mp)
            {
                ent->energy_status.mp_current = ent->modeldata.mp;
            }
            else if(ent->energy_status.mp_current < 0)
            {
                ent->energy_status.mp_current = 0;
            }
        }
        break;
    }
    case _ep_mpset:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->modeldata.mp = (int)dbltemp;
        }
        if(paramCount >= 4 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
        {
            ent->modeldata.mpstable = (int)dbltemp;
        }
        if(paramCount >= 5 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[4], &dbltemp)))
        {
            ent->modeldata.mpstableval = (int)dbltemp;
        }
        if(paramCount >= 6 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[5], &dbltemp)))
        {
            ent->modeldata.mprate = (int)dbltemp;
        }
        if(paramCount >= 7 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[6], &dbltemp)))
        {
            ent->modeldata.mpdroprate = (int)dbltemp;
        }
        if(paramCount >= 8 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[7], &dbltemp)))
        {
            ent->modeldata.chargerate = (int)dbltemp;
        }
        break;
    }
    case _ep_name:
    {
        if(varlist[2]->vt != VT_STR)
        {
            printf("You must give a string value for entity name.\n");
            goto changeentityproperty_error;
        }
        strcpy(ent->name, (char *)StrCache_Get(varlist[2]->strVal));
        break;
    }
    case _ep_nameposition:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.namex = (LONG)ltemp;
        }
        if(paramCount > 3 && SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
        {
            ent->modeldata.namey = (LONG)ltemp;
        }
        break;
    }
    case _ep_nextanim:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->nextanim = (LONG)ltemp;
        }
        break;
    }
    case _ep_nextmove:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->nextmove = (LONG)ltemp;
        }
        break;
    }
    case _ep_nextthink:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->nextthink = (LONG)ltemp;
        }
        break;
    }
    case _ep_no_adjust_base:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.no_adjust_base = (LONG)ltemp;
        }
        break;
    }
    case _ep_noaicontrol:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->noaicontrol = (LONG)ltemp;
        }
        break;
    }
    case _ep_nodieblink:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.nodieblink = (LONG)ltemp;
        }
        break;
    }
    case _ep_nodrop:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.nodrop = (LONG)ltemp;
        }
        break;
    }
    case _ep_nograb:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->nograb = (LONG)ltemp;
        }
        break;
    }
    case _ep_nohithead:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.nohithead = (LONG)ltemp;
        }
        break;
    }
    case _ep_nolife:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.nolife = (LONG)ltemp;
        }
        break;
    }
    case _ep_nopain:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.nopain = (LONG)ltemp;
        }
        break;
    }
    case _ep_offense:
    {
        if(paramCount >= 4 &&
                SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)) &&
                ltemp < (LONG)MAX_ATKS && ltemp >= (LONG)0 &&
                SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
        {
            ent->offense_factors[(LONG)ltemp] = (DOUBLE)dbltemp;
        }
        break;
    }
    case _ep_offscreen_noatk_factor:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->modeldata.offscreen_noatk_factor = (DOUBLE)dbltemp;
        }
        break;
    }
    case _ep_offscreenkill:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.offscreenkill = (LONG)ltemp;
        }
        break;
    }
    case _ep_opponent:
    {
        ent->opponent = (entity *)varlist[2]->ptrVal;
        break;
    }
    case _ep_custom_target:
    {
        ent->custom_target = (entity *)varlist[2]->ptrVal;
        break;
    }
    case _ep_owner:
    {
        ent->owner = (entity *)varlist[2]->ptrVal;
        break;
    }
    case _ep_parent:
    {
        ent->parent = (entity *)varlist[2]->ptrVal;
        break;
    }
    case _ep_pathfindstep:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->modeldata.pathfindstep = (DOUBLE)dbltemp;
        }
        break;
    }
    case _ep_position:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->position.x = (DOUBLE)dbltemp;
        }
        if(paramCount >= 4 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
        {
            ent->position.z = (DOUBLE)dbltemp;
        }
        if(paramCount >= 5 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[4], &dbltemp)))
        {
            ent->position.y = (DOUBLE)dbltemp;
        }
        break;
    }
    case _ep_x:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->position.x = (DOUBLE)dbltemp;
        }
        break;
    }
    case _ep_z:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->position.z = (DOUBLE)dbltemp;
        }
        break;
    }
    case _ep_a:
    case _ep_y:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->position.y = (DOUBLE)dbltemp;
        }
        break;
    }
    case _ep_projectile:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->projectile = (LONG)ltemp;
        }
        break;
    }
    case _ep_projectilehit:
    {
        ent->modeldata.projectilehit = 0;

        for(i = 2; i < paramCount; i++)
        {
            if(varlist[i]->vt == VT_INTEGER) // known entity type
            {
                ltemp = varlist[i]->lVal;
                if(ltemp & 0x80000000)
                {
                    ent->modeldata.projectilehit |= entitytypes[ltemp & 0x7fffffff];
                }
                else
                {
                    ent->modeldata.projectilehit |= ltemp;
                }
            }
            else
            {
                printf("You must pass one or more string constants for projectilehit entity type.\n");
                goto changeentityproperty_error;
            }
        }

        break;
    }
    case _ep_running:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->modeldata.runspeed = (DOUBLE)dbltemp;
        }
        if(paramCount >= 4 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
        {
            ent->modeldata.runjumpheight = (DOUBLE)dbltemp;
        }
        if(paramCount >= 5 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[4], &dbltemp)))
        {
            ent->modeldata.runjumpdist = (DOUBLE)dbltemp;
        }
        if(paramCount >= 6 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[5], &dbltemp)))
        {
            ent->modeldata.runhold = (int)dbltemp;
        }
        if(paramCount >= 7 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[6], &dbltemp)))
        {
            ent->modeldata.runupdown = (int)dbltemp;
        }

        break;
    }
    case _ep_rush_count:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->rush.count.current = (LONG)ltemp;
        }
        break;
    }
    case _ep_rush_tally:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->rush.count.max = (LONG)ltemp;
        }
        break;
    }
    case _ep_rush_time:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->rush.time = (LONG)ltemp;
        }
        break;
    }
    case _ep_score:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.score = (LONG)ltemp;
        }
        break;
    }
    case _ep_scroll:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->modeldata.scroll = (DOUBLE)dbltemp;
        }
        break;
    }
    case _ep_seal:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->seal = (LONG)ltemp;
        }
        break;
    }
    case _ep_sealtime:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->sealtime = (LONG)ltemp;
        }
        break;
    }
    case _ep_setlayer:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.setlayer = (LONG)ltemp;
        }
        break;
    }
    case _ep_sortid:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->sortid = (LONG)ltemp;
        }
        break;
    }
    case _ep_speed:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->modeldata.speed = (DOUBLE)dbltemp;
        }
        break;
    }
    case _ep_spritea:
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
        if(!validanim(ent, varlist[3]->lVal) || !(ent->modeldata.animation[varlist[3]->lVal]->numframes >= varlist[4]->lVal) || paramCount < 5)
        {
            break;
        }

        i = ent->modeldata.animation[varlist[3]->lVal]->sprite[varlist[4]->lVal];   //Get sprite index.

        switch(ltemp)
        {
        case _ep_spritea_centerx:
        {
            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[5], &ltemp)))
            {
                sprite_map[i].centerx = (LONG)ltemp;
            }

            break;
        }
        case _ep_spritea_centery:
        {
            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[5], &ltemp)))
            {
                sprite_map[i].centery = (LONG)ltemp;
            }
            break;
        }
        case _ep_spritea_file:
        {
            if(varlist[5]->vt != VT_STR)
            {
                printf("You must provide a string value for file name.\n");
                goto changeentityproperty_error;
            }
            strcpy(sprite_map[i].node->filename, (char *)StrCache_Get(varlist[5]->strVal));
            break;
        }
        /*
        case _ep_spritea_offsetx:
        {
            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[5], &ltemp)))
            {
                sprite_map[i].ofsx = (LONG)ltemp;
                (*pretvar)->lVal = (LONG)1;
            }
            break;
        }
        case _ep_spritea_offsety:
        {
            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            {
                sprite_map[i].ofsy = (LONG)ltemp;
                (*pretvar)->lVal = (LONG)1;
            }
            break;
        }*/
        case _ep_spritea_sprite:
        {
            sprite_map[i].node->sprite = (VOID *)varlist[5]->ptrVal;

            break;
        }
        default:
            printf("Unknown Sprite Array subproperty.\n");
            goto changeentityproperty_error;
        }
        break;
    }
    case _ep_stalltime:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->stalltime = (LONG)ltemp;
        }
        break;
    }
    case _ep_releasetime:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->releasetime = (LONG)ltemp;
        }
        break;
    }
    case _ep_staydown:
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
        if(paramCount < 4)
        {
            break;
        }

        switch(varlist[2]->lVal)
        {
        case _ep_staydown_rise:
        {
            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
            {
                ent->staydown.rise = (LONG)ltemp;
            }
            break;
        }
        case _ep_staydown_riseattack:
        {
            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
            {
                ent->staydown.riseattack = (LONG)ltemp;
            }
            break;
        }
        case _ep_staydown_riseattack_stall:
        {
            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
            {
                ent->staydown.riseattack_stall = (LONG)ltemp;
            }
            break;
        }
        default:
            printf("Unknown Staydown subproperty.\n");
            goto changeentityproperty_error;
        }
        break;
    }
    case _ep_stealth:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.stealth.hide = (LONG)ltemp;
        }
        break;
    }
    case _ep_subentity:
    {
        if(ent->subentity)
        {
            ent->subentity->parent = NULL;
        }
        ent->subentity = (entity *)varlist[2]->ptrVal;
        if(ent->subentity)
        {
            ent->subentity->parent = ent;
        }
        break;
    }
    case _ep_subject_to_basemap:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.subject_to_basemap = (LONG)ltemp;
        }
        break;
    }
    case _ep_subject_to_gravity:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.subject_to_gravity = (LONG)ltemp;
        }
        break;
    }
    case _ep_subject_to_hole:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.subject_to_hole = (LONG)ltemp;
        }
        break;
    }
    case _ep_subject_to_maxz:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.subject_to_maxz = (LONG)ltemp;
        }
        break;
    }
    case _ep_subject_to_minz:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.subject_to_minz = (LONG)ltemp;
        }
        break;
    }
    case _ep_subject_to_obstacle:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.subject_to_obstacle = (LONG)ltemp;
        }
        break;
    }
    case _ep_subject_to_platform:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.subject_to_platform = (LONG)ltemp;
        }
        break;
    }
    case _ep_subject_to_screen:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.subject_to_screen = (LONG)ltemp;
        }
        break;
    }
    case _ep_subject_to_wall:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.subject_to_wall = (LONG)ltemp;
        }
        break;
    }
    case _ep_subtype:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.subtype = (LONG)ltemp;
        }
        break;
    }
    case _ep_takeaction:
    {
        //if(varlist[2]->vt == VT_STRING)
        if(varlist[2]->vt == VT_EMPTY)
        {
            // UT: changed this to only accept NULL(), otherwise the log file is filled with warnings
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
        if((ltemp >= 0) && (ltemp < _ep_ta_the_end))
        {
            ent->takeaction = actions[(LONG)ltemp];
        }

        break;
    }
    case _ep_think:
    {
        //if(varlist[2]->vt == VT_STRING)
        if(varlist[2]->vt == VT_EMPTY)
        {
            // UT: changed this to only accept NULL(), otherwise the log file is filled with warnings
            break;
        }
        else if(varlist[2]->vt != VT_INTEGER)
        {
            printf("You must give a string value for think type.\n");
            goto changeentityproperty_error;
        }

        // otherwise, the parameter is a known action
        ltemp = varlist[2]->lVal;
        if((ltemp >= 0) && (ltemp < _ep_th_the_end))
        {
            ent->think = think[(LONG)ltemp];
        }

        break;
    }
    case _ep_thold:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.thold = (LONG)ltemp;
        }
        break;
    }
    case _ep_throwdamage:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.throwdamage = (LONG)ltemp;
        }
        break;
    }
    case _ep_throwdist:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->modeldata.throwdist = (DOUBLE)dbltemp;
        }
        break;
    }
    case _ep_throwframewait:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.throwframewait = (LONG)ltemp;
        }
        break;
    }
    case _ep_throwheight:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->modeldata.throwheight = (DOUBLE)dbltemp;
        }
        break;
    }
    case _ep_tosstime:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->toss_time = (LONG)ltemp;
        }
        break;
    }
    case _ep_trymove:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            if(ltemp == 1)
            {
                ent->trymove = common_trymove;
            }
            else if(ltemp == 2)
            {
                ent->trymove = player_trymove;
            }
            else
            {
                ent->trymove = NULL;
            }
        }
        break;
    }
    case _ep_type:
    {
        if(varlist[2]->vt != VT_INTEGER)
        {
            printf("You must provide a type constant for type.\n");
            goto changeentityproperty_error;
        }

        ltemp = varlist[2]->lVal;
        ent->modeldata.type = (LONG)ltemp;

        break;
    }
    case _ep_velocity:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->velocity.x = (DOUBLE)dbltemp;
        }
        if(paramCount >= 4 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
        {
            ent->velocity.z = (DOUBLE)dbltemp;
        }
        if(paramCount >= 5 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[4], &dbltemp)))
        {
            ent->velocity.y = (DOUBLE)dbltemp;
        }
        break;
    }
    case _ep_walkoffmovex:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.walkoffmovex = (LONG)ltemp;
        }
        break;
    }
    case _ep_walkoffmovez:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.walkoffmovez = (LONG)ltemp;
        }
        break;
    }
    case _ep_weapnum:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.weapnum = (LONG)ltemp;
        }
        break;
    }
    case _ep_weaploss:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.weaploss[0] = (LONG)ltemp;
        }

        if(paramCount >= 4)
        {
            if(FAILED(ScriptVariant_IntegerValue(varlist[3], &ltemp2)))
            {
                printf("You must specify the flag value.\n");
                *pretvar = NULL;
                return E_FAIL;
            }
            ent->modeldata.weaploss[1] = (LONG)ltemp2;
        }
        break;
    }
    case _ep_weapon:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ltemp2 = (LONG)0;
            if(paramCount < 4 ||  SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp2)))
            {
                set_weapon(ent, (LONG)ltemp, (LONG)ltemp2);
            }
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
    return E_FAIL;
}

//tossentity(entity, height, speedx, speedz)
HRESULT openbor_tossentity(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    entity *ent = NULL;
    DOUBLE height = 0, speedx = 0, speedz = 0;

    if(paramCount < 1)
    {
        goto toss_error;
    }

    ent = (entity *)varlist[0]->ptrVal; //retrieve the entity
    if(!ent)
    {
        goto toss_error;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)1;

    if(paramCount >= 2)
    {
        ScriptVariant_DecimalValue(varlist[1], &height);
    }
    if(paramCount >= 3)
    {
        ScriptVariant_DecimalValue(varlist[2], &speedx);
    }
    if(paramCount >= 4)
    {
        ScriptVariant_DecimalValue(varlist[3], &speedz);
    }

    ent->velocity.x = (float)speedx;
    ent->velocity.z = (float)speedz;
    toss(ent, (float)height);
    return S_OK;

toss_error:
    printf("Function tossentity(entity,height, speedx, speedz) requires at least a valid entity handle.\n");
    *pretvar = NULL;
    return E_FAIL;
}

// ===== getplayerproperty =====
enum playerproperty_enum
{
    _pp_colourmap,
    _pp_combokey,
    _pp_combostep,
    _pp_credits,
    _pp_disablekeys,
    _pp_ent,
    _pp_entity,
    _pp_hasplayed,
    _pp_hmapl,
    _pp_hmapu,
    _pp_inputtime,
    _pp_joining,
    _pp_keys,
    _pp_lives,
    _pp_mapcount,
    _pp_name,
    _pp_newkeys,
    _pp_numweapons,
    _pp_playkeys,
    _pp_releasekeys,
    _pp_score,
    _pp_spawnhealth,
    _pp_spawnmp,
    _pp_weapnum,
    _pp_weapon,
    _pp_the_end
};

int mapstrings_playerproperty(ScriptVariant **varlist, int paramCount)
{
    char *propname;
    int prop;

    static const char *proplist[] =
    {
        "colourmap",
        "combokey",
        "combostep",
        "credits",
        "disablekeys",
        "ent",
        "entity",
        "hasplayed",
        "hmapl",
        "hmapu",
        "inputtime",
        "joining",
        "keys",
        "lives",
        "mapcount",
        "name",
        "newkeys",
        "numweapons",
        "playkeys",
        "releasekeys",
        "score",
        "spawnhealth",
        "spawnmp",
        "weapnum",
        "weapon",
    };

    if(paramCount < 2)
    {
        return 1;
    }

    // property name
    MAPSTRINGS(varlist[1], proplist, _pp_the_end,
               "Player property name '%s' is not supported yet.\n");

    return 1;
}

//getplayerproperty(index, propname);
HRESULT openbor_getplayerproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ltemp;
    int index;
    entity *ent = NULL;
    int prop = -1;
    ScriptVariant *arg = NULL;

    if(paramCount < 2)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    mapstrings_playerproperty(varlist, paramCount);
    ScriptVariant_Clear(*pretvar);

    arg = varlist[0];
    if(FAILED(ScriptVariant_IntegerValue(arg, &ltemp)))
    {
        index = 0;
    }
    else
    {
        index = (LONG)ltemp;
    }


    ent = player[index].ent;

    arg = varlist[1];
    if(arg->vt != VT_INTEGER)
    {
        if(arg->vt != VT_STR)
        {
            printf("Function call getplayerproperty has invalid propery name parameter, it must be a string value.\n");
        }
        *pretvar = NULL;
        return E_FAIL;
    }
    prop = arg->lVal;

    switch(prop)
    {
    case _pp_ent:
    case _pp_entity:
    {
        if(!ent)
        {
            ScriptVariant_Clear(*pretvar);    // player not spawned
        }
        else
        {
            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID *)ent;
        }
        break;
    }
    case _pp_name:
    {
        ScriptVariant_ChangeType(*pretvar, VT_STR);
        (*pretvar)->strVal = StrCache_CreateNewFrom(player[index].name);
        break;
    }
    case _pp_colourmap:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)player[index].colourmap;
        break;
    }
    case _pp_score:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)player[index].score;
        break;
    }
    case _pp_hasplayed:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)player[index].hasplayed;
        break;
    }
    case _pp_spawnhealth:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)player[index].spawnhealth;
        break;
    }
    case _pp_spawnmp:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)player[index].spawnmp;
        break;
    }
    case _pp_lives:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)player[index].lives;
        break;
    }
    case _pp_disablekeys:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)player[index].disablekeys;
        break;
    }
    case _pp_playkeys:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)player[index].playkeys;
        break;
    }
    case _pp_keys:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)player[index].keys;
        break;
    }
    case _pp_newkeys:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)player[index].newkeys;
        break;
    }
    case _pp_releasekeys:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)player[index].releasekeys;
        break;
    }
    case _pp_credits:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        if(noshare)
        {
            (*pretvar)->lVal = (LONG)player[index].credits;
        }
        else
        {
            (*pretvar)->lVal = credits;
        }
        break;
    }
    case _pp_weapnum:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)player[index].weapnum;
        break;
    }
    case _pp_numweapons:
    {
        int cacheindex;

        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

        if ( stricmp(player[index].name,"") != 0 ) {
            cacheindex = get_cached_model_index(player[index].name);
            if ( cacheindex == -1 )
            {
               (*pretvar)->lVal = (LONG)0;
               break;
            }
        } else
        {
           (*pretvar)->lVal = (LONG)0;
           break;
        }

        (*pretvar)->lVal = (LONG)model_cache[cacheindex].model->numweapons;
        break;
    }
    case _pp_joining:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)player[index].joining;
        break;
    }
    case _pp_combokey:
    {
        ScriptVariant *frm = NULL;
        frm = varlist[2];
        if(frm->vt != VT_INTEGER)
        {
            printf("Need a combostep value number for this property.\n");
            *pretvar = NULL;
            return E_FAIL;
        }
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)player[index].combokey[frm->lVal];
        break;
    }
    case _pp_combostep:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)player[index].combostep;
        break;
    }
    case _pp_inputtime:
    {
        ScriptVariant *frm = NULL;
        frm = varlist[2];
        if(frm->vt != VT_INTEGER)
        {
            printf("Need a combostep value number for this property.\n");
            *pretvar = NULL;
            return E_FAIL;
        }
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)player[index].inputtime[frm->lVal];
        break;
    }
    case _pp_hmapl:
    {
        int cacheindex;

        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

        if ( stricmp(player[index].name,"") != 0 ) {
            cacheindex = get_cached_model_index(player[index].name);
            if ( cacheindex == -1 )
            {
               (*pretvar)->lVal = (LONG)0;
               break;
            }
        } else
        {
           (*pretvar)->lVal = (LONG)0;
           break;
        }

        (*pretvar)->lVal = (LONG)model_cache[cacheindex].model->maps.hide_start;
        break;
    }
    case _pp_hmapu:
    {
        int cacheindex;

        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

        if ( stricmp(player[index].name,"") != 0 ) {
            cacheindex = get_cached_model_index(player[index].name);
            if ( cacheindex == -1 )
            {
               (*pretvar)->lVal = (LONG)0;
               break;
            }
        } else
        {
           (*pretvar)->lVal = (LONG)0;
           break;
        }

        (*pretvar)->lVal = (LONG)model_cache[cacheindex].model->maps.hide_end;
        break;
    }
    case _pp_mapcount:
    {
        int cacheindex;

        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

        if ( stricmp(player[index].name,"") != 0 ) {
            cacheindex = get_cached_model_index(player[index].name);
            if ( cacheindex == -1 )
            {
               (*pretvar)->lVal = (LONG)0;
               break;
            }
        } else
        {
           (*pretvar)->lVal = (LONG)0;
           break;
        }

        (*pretvar)->lVal = (LONG)(model_cache[cacheindex].model->maps_loaded + 1);
        break;
    }
    default:
    {
        //printf("Property name '%s' is not supported by function getplayerproperty.\n", propname);
        *pretvar = NULL;
        return E_FAIL;
        break;
    }
    //this property is not known
    //default:{
    //  .....
    //}
    }
    return S_OK;
}


//changeplayerproperty(index, propname, value[, value2, value3,...]);
HRESULT openbor_changeplayerproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ltemp, ltemp2;
    int index;
    entity *ent = NULL;
    int prop = -1;
    char *tempstr = NULL;
    static char buffer[64];
    ScriptVariant *arg = NULL;

    *pretvar = NULL;
    if(paramCount < 3)
    {
        printf("Function changeplayerproperty must have at least 3 arguments.\n");
        return E_FAIL;
    }

    mapstrings_playerproperty(varlist, paramCount);
    arg = varlist[0];

    if(FAILED(ScriptVariant_IntegerValue(arg, &ltemp)))
    {
        index = 0;
    }
    else
    {
        index = (LONG)ltemp;
    }

    ent = player[index].ent;

    if(varlist[1]->vt != VT_INTEGER)
    {
        if(varlist[1]->vt != VT_STR)
        {
            printf("You must give a string value for player property name.\n");
        }
        return E_FAIL;
    }
    prop = varlist[1]->lVal;

    arg = varlist[2];

    //change the model
    switch(prop)
    {
    case _pp_ent:
    case _pp_entity:
    {
        if(arg->vt == VT_PTR || arg->vt == VT_EMPTY)
        {
            player[index].ent = (entity *)arg->ptrVal;
        }
        else
        {
            goto cpperror;
        }
        break;
    }
    case _pp_weapon:
    {
        if(ent)
        {
            if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
            {
                if(paramCount > 3)
                {
                    if(FAILED(ScriptVariant_IntegerValue(varlist[3], &ltemp2)))
                    {
                        goto cpperror;
                    }
                }
                else
                {
                    ltemp2 = (LONG)0;
                }
                set_weapon(player[index].ent, (LONG)ltemp, (LONG)ltemp2);
            }
            else
            {
                goto cpperror;
            }
        }
        break;
    }
    case _pp_weapnum:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            player[index].weapnum = (LONG)ltemp;
        }
        else
        {
            goto cpperror;
        }
        break;
    }
    case _pp_name:
    {
        if(arg->vt != VT_STR)
        {
            goto cpperror;
        }
        tempstr = (char *)StrCache_Get(arg->strVal);
        strcpy(player[index].name, tempstr);
        break;
    }
    case _pp_colourmap:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            player[index].colourmap = (LONG)ltemp;
        }
        else
        {
            goto cpperror;
        }
        break;
    }
    case _pp_score:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            if(ltemp < 0)
            {
                ltemp = 0;
            }
            else if(ltemp > 999999999)
            {
                ltemp = 999999999;
            }
            player[index].score = (unsigned int)ltemp;
        }
        else
        {
            goto cpperror;
        }
        break;
    }
    case _pp_hasplayed:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            player[index].hasplayed = (LONG)ltemp;
        }
        else
        {
            goto cpperror;
        }
        break;
    }
    case _pp_spawnhealth:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            player[index].spawnhealth = (LONG)ltemp;
        }
        else
        {
            goto cpperror;
        }
        break;
    }
    case _pp_spawnmp:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            player[index].spawnmp = (LONG)ltemp;
        }
        else
        {
            goto cpperror;
        }
        break;
    }
    case _pp_lives:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            player[index].lives = (LONG)ltemp;
        }
        else
        {
            goto cpperror;
        }
        break;
    }
    case _pp_disablekeys:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            player[index].disablekeys = (LONG)ltemp;
        }
        else
        {
            goto cpperror;
        }
        break;
    }
    case _pp_playkeys:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            player[index].playkeys = (LONG)ltemp;
        }
        else
        {
            goto cpperror;
        }
        break;
    }
    case _pp_keys:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            player[index].keys = (LONG)ltemp;
        }
        else
        {
            goto cpperror;
        }
        break;
    }
    case _pp_newkeys:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg,&ltemp)))
        {
            player[index].newkeys = (LONG)ltemp;
        }
        else
        {
            goto cpperror;
        }
        break;
    }
    case _pp_credits:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            if(noshare)
            {
                player[index].credits = (LONG)ltemp;
            }
            else
            {
                credits = (LONG)ltemp;
            }
        }
        else
        {
            goto cpperror;
        }
        break;
    }
    case _pp_joining:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg,&ltemp)))
        {
            player[index].joining = (LONG)ltemp;
        }
        else
        {
            goto cpperror;
        }
        break;
    }
    case _pp_releasekeys:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg,&ltemp)))
        {
            player[index].releasekeys = (LONG)ltemp;
        }
        else
        {
            goto cpperror;
        }
        break;
    }
    case _pp_combokey:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg,&ltemp)))
        {
            ScriptVariant *value = NULL;
            value = varlist[3];
            if(value->vt != VT_INTEGER)
            {
                printf("Need a value and combostep value for this property.\n");
                *pretvar = NULL;
                return E_FAIL;
            }
            player[index].combokey[ltemp] = (int)value->lVal;
        }
        else
        {
            goto cpperror;
        }
        break;
    }
    case _pp_combostep:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg,&ltemp)))
        {
            player[index].combostep = (LONG)ltemp;
        }
        else
        {
            goto cpperror;
        }
        break;
    }
    case _pp_inputtime:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg,&ltemp)))
        {
            ScriptVariant *value = NULL;
            value = varlist[3];
            if(value->vt != VT_INTEGER)
            {
                printf("Need a value and combostep value number for this property.\n");
                *pretvar = NULL;
                return E_FAIL;
            }
            player[index].inputtime[ltemp] = (int)value->lVal;
        }
        else
        {
            goto cpperror;
        }
        break;
    }
    default:
        printf("Invalid property name for function changeplayerproperty.\n");
        return E_FAIL;
    }

    return S_OK;
cpperror:
    ScriptVariant_ToString(arg, buffer);
    printf("Function changeplayerproperty receives an invalid value: %s.\n", buffer);
    return E_FAIL;
}

//this method is used by script engine, we move it here
// it will get a system property, put it in the ScriptVariant
// if failed return 0, otherwise return 1
int getsyspropertybyindex(ScriptVariant *var, int index)
{
    if(!var)
    {
        return 0;
    }

    switch(index)
    {
    case _sv_count_enemies:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = count_ents(TYPE_ENEMY);
        break;
    case _sv_count_players:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = count_ents(TYPE_PLAYER);
        break;
    case _sv_count_npcs:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = count_ents(TYPE_NPC);
        break;
    case _sv_count_entities:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = ent_count;
        break;
    case _sv_ent_max:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = ent_max;
        break;
    case _sv_in_level:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (level != NULL);
        break;
    case _sv_in_gameoverscreen:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = gameOver;
        break;
    case _sv_in_menuscreen:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        if(selectScreen || titleScreen || hallOfFame || gameOver || showComplete || currentScene || level || enginecreditsScreen)
        {
            var->lVal = 0;
        }
        else
        {
            var->lVal = menuScreen;
        }
        break;
    case _sv_in_enginecreditsscreen:
    		ScriptVariant_ChangeType(var, VT_INTEGER);
    		var->lVal = enginecreditsScreen;
    		break;
    case _sv_in_options:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = optionsMenu;
        break;
    case _sv_in_system_options:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = systemoptionsMenu;
        break;
    case _sv_in_cheat_options:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (optionsMenu && is_cheat_actived()) ? 1:0;
        break;
    case _sv_cheats:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = is_cheat_actived();
        break;
    case _sv_in_control_options:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = controloptionsMenu;
        break;
    case _sv_in_sound_options:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = soundoptionsMenu;
        break;
    case _sv_in_video_options:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = videooptionsMenu;
        break;
    case _sv_in_start_game:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = startgameMenu;
        break;
    case _sv_in_new_game:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = newgameMenu;
        break;
    case _sv_in_load_game:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = loadgameMenu;
        break;
    case _sv_sets_count:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = num_difficulties;
        break;
    case _sv_in_showcomplete:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = showComplete;
        break;
    case _sv_in_titlescreen:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = titleScreen;
        break;
    case _sv_in_halloffamescreen:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = (hallOfFame);
        break;
    case _sv_sample_play_id:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = sample_play_id;
    case _sv_effectvol:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = savedata.effectvol;
        break;
    case _sv_elapsed_time:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = _time;
        break;
    case _sv_game_speed:
        if(!level)
        {
            return 0;
        }
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = GAME_SPEED;
        break;
    case _sv_game_paused:
    case _sv_pause:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        if( !(goto_mainmenu_flag&1) ) var->lVal = (_pause);
        else var->lVal = 0;
        break;
    case _sv_game_time:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = timeleft;
        break;
    case _sv_porting:
    {
        e_porting porting;

        #if ANDROID
            porting = PORTING_ANDROID;
        #elif DARWIN
            porting = PORTING_DARWIN;
        #elif DC
            porting = PORTING_DREAMCAST;
        #elif GPX2
            porting = PORTING_GPX2;
        #elif LINUX
            porting = PORTING_LINUX;
        #elif OPENDINGUX
            porting = PORTING_OPENDINGUX;
        #elif PSP
            porting = PORTING_PSP;
        #elif WII
            porting = PORTING_WII;
        #elif WIN
            porting = PORTING_WINDOWS;
        #elif WIZ
            porting = PORTING_WIZ;
        #elif XBOX
            porting = PORTING_XBOX;
        #elif VITA
            porting = PORTING_VITA;
        #else
            porting = PORTING_UNKNOWN;
        #endif

        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = porting;
        break;
    }
    case _sv_gfx_x_offset:
        if(!level)
        {
            return 0;
        }
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = gfx_x_offset;
        break;
    case _sv_gfx_y_offset:
        if(!level)
        {
            return 0;
        }
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = gfx_y_offset;
        break;
    case _sv_gfx_y_offset_adj:
        if(!level)
        {
            return 0;
        }
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = gfx_y_offset_adj;
        break;
    case _sv_in_selectscreen:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = selectScreen;
        break;
    case _sv_lasthita:
    case _sv_lasthity:
        ScriptVariant_ChangeType(var, VT_DECIMAL);
        var->dblVal = lasthit.position.y;
        break;
    case _sv_lasthitc:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = lasthit.confirm;
        break;
    case _sv_lasthitt:
        ScriptVariant_ChangeType(var, VT_INTEGER);

        if(lasthit.attack)
        {
            var->lVal = lasthit.attack->attack_type;
        }

        break;
    case _sv_lasthitx:
        ScriptVariant_ChangeType(var, VT_DECIMAL);
        var->dblVal = lasthit.position.x;
        break;
    case _sv_lasthitz:
        ScriptVariant_ChangeType(var, VT_DECIMAL);
        var->dblVal = lasthit.position.z;
        break;
    case _sv_xpos:
        if(!level)
        {
            return 0;
        }
        ScriptVariant_ChangeType(var, VT_DECIMAL);
        var->dblVal = advancex;
        break;
    case _sv_ypos:
        if(!level)
        {
            return 0;
        }
        ScriptVariant_ChangeType(var, VT_DECIMAL);
        var->dblVal = advancey;
        break;
    case _sv_hresolution:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = videomodes.hRes;
        break;
    case _sv_vresolution:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = videomodes.vRes;
        break;
    case _sv_current_scene:
        if(currentScene)
        {
            ScriptVariant_ChangeType(var, VT_STR);
            var->strVal = StrCache_CreateNewFrom(currentScene);
        }
        else
        {
            ScriptVariant_Clear(var);
        }
        break;
    case _sv_current_set:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = current_set;
        break;
    case _sv_current_level:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = current_level;
        break;
    case _sv_current_palette:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = current_palette;
        break;
    case _sv_current_stage:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = current_stage;
        break;
    case _sv_levelwidth:
        if(!level)
        {
            return 0;
        }
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = level->width;
        break;
    case _sv_levelheight:
        if(!level)
        {
            return 0;
        }
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = panel_height;
        break;
    case _sv_branchname:
        ScriptVariant_ChangeType(var, VT_STR);
        var->strVal = StrCache_CreateNewFrom(branch_name);
        break;
    case _sv_current_branch:
        if(level != NULL && levelsets && levelsets[current_set].levelorder && levelsets[current_set].levelorder[current_level].branchname)
        {
            ScriptVariant_ChangeType(var, VT_STR);
            var->strVal = StrCache_CreateNewFrom(levelsets[current_set].levelorder[current_level].branchname);
        }
        else
        {
            ScriptVariant_Clear(var);
        }
        break;
    case _sv_pakname:
    {
        char tempstr[MAX_BUFFER_LEN];
        getPakName(tempstr, -1);
        ScriptVariant_ChangeType(var, VT_STR);
        var->strVal = StrCache_CreateNewFrom(tempstr);
        break;
    }
    case _sv_bossescount:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = level->bossescount;
        break;
    case _sv_maxsoundchannels:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = maxchannels();
        break;
    case _sv_maxentityvars:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = max_entity_vars;
        break;
    case _sv_maxindexedvars:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = max_indexed_vars;
        break;
    case _sv_maxplayers:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = levelsets[current_set].maxplayers;
        break;
    case _sv_maxscriptvars:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = max_script_vars;
        break;
    case _sv_models_cached:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = models_cached;
        break;
    case _sv_models_loaded:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = models_loaded;
        break;
    case _sv_musicvol:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = savedata.musicvol;
        break;
    case _sv_nofadeout:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = nofadeout;
        break;
    case _sv_nojoin:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = nojoin;
        break;
    case _sv_nopause:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = nopause;
        break;
    case _sv_nosave:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = nosave;
        break;
    case _sv_noscreenshot:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = noscreenshot;
        break;
    case _sv_noshowcomplete:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = levelsets[current_set].noshowcomplete;
        break;
    case _sv_numbasemaps:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = level->numbasemaps;
        break;
    case _sv_numbosses:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = level->numbosses;
        break;
    case _sv_numholes:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = level->numholes;
        break;
    case _sv_numlayers:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = level->numlayers;
        break;
    case _sv_numpalettes:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = level->numpalettes;
        break;
    case _sv_numwalls:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = level->numwalls;
        break;
    case _sv_pixelformat:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = pixelformat;
        break;
    case _sv_player:
    case _sv_player1:
        ScriptVariant_ChangeType(var, VT_PTR);
        var->ptrVal = player;
        break;
    case _sv_player2:
        ScriptVariant_ChangeType(var, VT_PTR);
        var->ptrVal = player + 1;
        break;
    case _sv_player3:
        ScriptVariant_ChangeType(var, VT_PTR);
        var->ptrVal = player + 2;
        break;
    case _sv_player4:
        ScriptVariant_ChangeType(var, VT_PTR);
        var->ptrVal = player + 3;
        break;
    case _sv_player_max_z:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = PLAYER_MAX_Z;
        break;
    case _sv_player_min_z:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = PLAYER_MIN_Z;
        break;
    case _sv_lightx:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = light.x;
        break;
    case _sv_lightz:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = light.y;
        break;
    case _sv_self:
        ScriptVariant_ChangeType(var, VT_PTR);
        var->ptrVal = self;
        break;
    case _sv_shadowalpha:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = shadowalpha;
        break;
    case _sv_shadowcolor:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = shadowcolor;
        break;
    case _sv_shadowopacity:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = shadowopacity;
        break;
    case _sv_skiptoset:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = skiptoset;
        break;
    case _sv_slowmotion:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = slowmotion.toggle;
        break;
    case _sv_slowmotion_duration:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = slowmotion.duration;
        break;
    case _sv_soundvol:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = savedata.soundvol;
        break;
    case _sv_totalram:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = getSystemRam(KBYTES);
        break;
    case _sv_freeram:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = getFreeRam(KBYTES);
        break;
    case _sv_usedram:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = getUsedRam(KBYTES);
        break;
    case _sv_textbox:
        ScriptVariant_ChangeType(var, VT_PTR);
        var->ptrVal = textbox;
        break;
    case _sv_background:
        ScriptVariant_ChangeType(var, VT_PTR);
        var->ptrVal = background;
        break;
    case _sv_vscreen:
        ScriptVariant_ChangeType(var, VT_PTR);
        var->ptrVal = vscreen;
        break;
    case _sv_viewportx:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = viewportx;
        break;
    case _sv_viewporty:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = viewporty;
        break;
    case _sv_viewportw:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = viewportw;
        break;
    case _sv_viewporth:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = viewporth;
        break;
    case _sv_scrollminx:
        ScriptVariant_ChangeType(var, VT_DECIMAL);
        var->dblVal = scrollminx;
        break;
    case _sv_scrollmaxx:
        ScriptVariant_ChangeType(var, VT_DECIMAL);
        var->dblVal = scrollmaxx;
        break;
    case _sv_scrollminz:
        ScriptVariant_ChangeType(var, VT_DECIMAL);
        var->dblVal = scrollminz;
        break;
    case _sv_scrollmaxz:
        ScriptVariant_ChangeType(var, VT_DECIMAL);
        var->dblVal = scrollmaxz;
        break;
    case _sv_blockade:
        ScriptVariant_ChangeType(var, VT_DECIMAL);
        var->dblVal = blockade;
        break;
    case _sv_waiting:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = level ? level->waiting : 0;
    case _sv_maxattacktypes:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = max_attack_types;
        break;
    case _sv_maxanimations:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = max_animations;
        break;
    case _sv_ticks:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = timer_gettick();
        break;
    case _sv_nogameover:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = levelsets[current_set].noshowgameover; // or s_set_entry *set = levelsets + current_set;
        break;
    case _sv_nohof:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = levelsets[current_set].noshowhof;
        break;
    case _sv_fps:
        ScriptVariant_ChangeType(var, VT_INTEGER);
        var->lVal = getFPS();
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
int changesyspropertybyindex(int index, ScriptVariant *value)
{
    //char* tempstr = NULL;
    LONG ltemp;
    DOUBLE dbltemp;

    switch(index)
    {
    case _sv_elapsed_time:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            _time = (LONG)ltemp;
        }
        break;
    case _sv_current_stage:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            current_stage = (LONG)ltemp;
        }
        break;
    case _sv_current_set:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            current_set = (LONG)ltemp;
        }
        break;
    case _sv_current_level:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            current_level = (LONG)ltemp;
        }
        break;
    case _sv_game_time:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            timeleft = (LONG)ltemp;
        }
        break;
    case _sv_gfx_x_offset:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            gfx_x_offset = (LONG)ltemp;
        }
        break;
    case _sv_gfx_y_offset:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            gfx_y_offset = (LONG)ltemp;
        }
        break;
    case _sv_gfx_y_offset_adj:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            gfx_y_offset_adj = (LONG)ltemp;
        }
        break;
    case _sv_levelpos:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            level->pos = (LONG)ltemp;
        }
        break;
    case _sv_xpos:
        if(SUCCEEDED(ScriptVariant_DecimalValue(value, &dbltemp)))
        {
            advancex = (float)dbltemp;
        }
        break;
    case _sv_ypos:
        if(SUCCEEDED(ScriptVariant_DecimalValue(value, &dbltemp)))
        {
            advancey = (DOUBLE)dbltemp;
        }
        break;
    case _sv_scrollminz:
        if(SUCCEEDED(ScriptVariant_DecimalValue(value, &dbltemp)))
        {
            scrollminz = (DOUBLE)dbltemp;
        }
        break;
    case _sv_scrollmaxz:
        if(SUCCEEDED(ScriptVariant_DecimalValue(value, &dbltemp)))
        {
            scrollmaxz = (DOUBLE)dbltemp;
        }
        break;
    case _sv_scrollminx:
        if(SUCCEEDED(ScriptVariant_DecimalValue(value, &dbltemp)))
        {
            scrollminx = (DOUBLE)dbltemp;
        }
        break;
    case _sv_scrollmaxx:
        if(SUCCEEDED(ScriptVariant_DecimalValue(value, &dbltemp)))
        {
            scrollmaxx = (DOUBLE)dbltemp;
        }
        break;
    case _sv_blockade:
        if(SUCCEEDED(ScriptVariant_DecimalValue(value, &dbltemp)))
        {
            blockade = (DOUBLE)dbltemp;
        }
        break;
    case _sv_shadowcolor:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            shadowcolor = (LONG)ltemp;
        }
        break;
    case _sv_shadowalpha:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            shadowalpha = (LONG)ltemp;
        }
        break;
    case _sv_shadowopacity:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            shadowopacity = (LONG)ltemp;
        }
        break;
    case _sv_skiptoset:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            skiptoset = (LONG)ltemp;
        }
        break;
    case _sv_slowmotion:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            slowmotion.toggle = (unsigned)ltemp;
        }
        break;
    case _sv_slowmotion_duration:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            slowmotion.duration = (unsigned)ltemp;
        }
        break;
    case _sv_lasthita:
    case _sv_lasthity:
        if(SUCCEEDED(ScriptVariant_DecimalValue(value, &dbltemp)))
        {
            lasthit.position.y = (DOUBLE)dbltemp;
        }
        break;
    case _sv_lasthitx:
        if(SUCCEEDED(ScriptVariant_DecimalValue(value, &dbltemp)))
        {
            lasthit.position.x = (DOUBLE)dbltemp;
        }
        break;
    case _sv_lasthitz:
        if(SUCCEEDED(ScriptVariant_DecimalValue(value, &dbltemp)))
        {
            lasthit.position.z = (DOUBLE)dbltemp;
        }
        break;
    case _sv_lasthitc:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            lasthit.confirm = (LONG)ltemp;
        }
        break;
    case _sv_lasthitt:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            lasthit.attack->attack_type = (LONG)ltemp;
        }
        break;
    case _sv_smartbomber:
        smartbomber = (entity *)value->ptrVal;
        break;
    case _sv_textbox:
        textbox = (entity *)value->ptrVal;
        break;
    case _sv_background:
        background = (s_screen *)value->ptrVal;
        break;
    case _sv_bossescount:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            level->bossescount = (LONG)ltemp;
        }
        break;
    case _sv_vscreen:
        vscreen = (s_screen *)value->ptrVal;
        break;
    case _sv_nofadeout:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            nofadeout = (LONG)ltemp;
        }
        break;
    case _sv_nojoin:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            nojoin = (LONG)ltemp;
        }
        break;
    case _sv_nopause:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            nopause = (LONG)ltemp;
        }
        break;
    case _sv_nosave:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            nosave = (LONG)ltemp;
        }
        break;
    case _sv_noscreenshot:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            noscreenshot = (LONG)ltemp;
        }
        break;
    case _sv_noshowcomplete:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            levelsets[current_set].noshowcomplete = (LONG)ltemp;
        }
        break;
    case _sv_numbosses:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            level->numbosses = (LONG)ltemp;
        }
        break;
    case _sv_viewportx:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            viewportx = (LONG)ltemp;
        }
        break;
    case _sv_viewporty:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            viewporty = (LONG)ltemp;
        }
        break;
    case _sv_viewportw:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            viewportw = (LONG)ltemp;
        }
        break;
    case _sv_viewporth:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            viewporth = (LONG)ltemp;
        }
        break;
    case _sv_waiting:
        if(!level)
        {
            break;
        }
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            level->waiting = (LONG)ltemp;
        }
        break;
    case _sv_nogameover:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            levelsets[current_set].noshowgameover = (LONG)ltemp;
        }
        break;
    case _sv_nohof:
        if(SUCCEEDED(ScriptVariant_IntegerValue(value, &ltemp)))
        {
            levelsets[current_set].noshowhof = (LONG)ltemp;
        }
        break;
    default:
        return 0;
    }

    return 1;
}

//checkhole(x,z,a), return 1 if there's hole here
HRESULT openbor_checkhole(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    ScriptVariant *arg = NULL;
    DOUBLE x, z, a;

    if(paramCount < 2)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)0;

    arg = varlist[0];
    if(FAILED(ScriptVariant_DecimalValue(arg, &x)))
    {
        return S_OK;
    }

    arg = varlist[1];
    if(FAILED(ScriptVariant_DecimalValue(arg, &z)))
    {
        return S_OK;
    }

    if ( paramCount >= 3 )
    {
        arg = varlist[2];
        if(FAILED(ScriptVariant_DecimalValue(arg, &a)))
        {
            return S_OK;
        }

        (*pretvar)->lVal = (LONG)(checkhole_in((float)x, (float)z, (float)a) && checkwall_index((float)x, (float)z) < 0);
    }
    else (*pretvar)->lVal = (LONG)(checkhole((float)x, (float)z) && checkwall_index((float)x, (float)z) < 0);

    return S_OK;
}

//checkholeindex(x,z,a), return hole index if there's hole here, else it returns -1
HRESULT openbor_checkholeindex(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    ScriptVariant *arg = NULL;
    DOUBLE x, z, a;

    if(paramCount < 2)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)-1;

    arg = varlist[0];
    if(FAILED(ScriptVariant_DecimalValue(arg, &x)))
    {
        return S_OK;
    }

    arg = varlist[1];
    if(FAILED(ScriptVariant_DecimalValue(arg, &z)))
    {
        return S_OK;
    }

    if ( checkwall_index((float)x, (float)z) < 0 )
    {
        if ( paramCount >= 3 )
        {
            arg = varlist[2];
            if(FAILED(ScriptVariant_DecimalValue(arg, &a)))
            {
                return S_OK;
            }

            (*pretvar)->lVal = (LONG)checkholeindex_in((float)x, (float)z, (float)a);
        }
        else (*pretvar)->lVal = (LONG)checkhole_index((float)x, (float)z);
    }

    return S_OK;
}

//checkbase(x,z,y,entity), return base of terrain. -1 if there is a wall and no platform. entity param is optional.
HRESULT openbor_checkbase(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    ScriptVariant *arg = NULL;
    DOUBLE x, z, y;
    entity *ent = NULL;
    float base = -1.0f;

    if(paramCount < 3)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
    (*pretvar)->dblVal = (DOUBLE)base;

    arg = varlist[0];
    if(FAILED(ScriptVariant_DecimalValue(arg, &x)))
    {
        return S_OK;
    }

    arg = varlist[1];
    if(FAILED(ScriptVariant_DecimalValue(arg, &z)))
    {
        return S_OK;
    }

    arg = varlist[2];
    if(FAILED(ScriptVariant_DecimalValue(arg, &y)))
    {
        return S_OK;
    }

    if (paramCount > 3)
    {
        arg = varlist[3];
        /*if(arg->vt != VT_PTR && arg->vt != VT_EMPTY)
        {
            printf("Function checkbase must have a valid entity handle or NULL().\n");
            *pretvar = NULL;
            return E_FAIL;
        }*/
        ent = (entity *)arg->ptrVal; //retrieve the entity
    }

    if((base = checkbase((float)x, (float)z, (float)y, ent)) >= 0)
    {
        (*pretvar)->dblVal = (DOUBLE)base;
    }
    return S_OK;
}

//checkwall(x,z), return wall height, or 0 | accept checkwall_index(x,z,y) too
HRESULT openbor_checkwall(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    ScriptVariant *arg = NULL;
    DOUBLE x, z, y;
    int wall;
    float h = T_MAX_CHECK_ALTITUDE;

    if(paramCount < 2)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
    (*pretvar)->dblVal = (DOUBLE)0;

    arg = varlist[0];
    if(FAILED(ScriptVariant_DecimalValue(arg, &x)))
    {
        return S_OK;
    }

    arg = varlist[1];
    if(FAILED(ScriptVariant_DecimalValue(arg, &z)))
    {
        return S_OK;
    }

    if (paramCount > 2)
    {
        arg = varlist[2];
        if(FAILED(ScriptVariant_DecimalValue(arg, &y)))
        {
            return S_OK;
        }
        h = (float)y;
    }

    if((wall = checkwall_below((float)x, (float)z, (float)h)) >= 0)
    {
        (*pretvar)->dblVal = (DOUBLE)level->walls[wall].height;
    }
    return S_OK;
}

//checkwallindex(x,z), return wall index, or -1 | accept checkwallindex(x,z,y) too
HRESULT openbor_checkwallindex(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    ScriptVariant *arg = NULL;
    DOUBLE x, z, y;
    int wall;
    float h = 100000;

    if(paramCount < 2)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)-1;

    arg = varlist[0];
    if(FAILED(ScriptVariant_DecimalValue(arg, &x)))
    {
        return S_OK;
    }

    arg = varlist[1];
    if(FAILED(ScriptVariant_DecimalValue(arg, &z)))
    {
        return S_OK;
    }

    if (paramCount > 2)
    {
        arg = varlist[2];
        if(FAILED(ScriptVariant_DecimalValue(arg, &y)))
        {
            return S_OK;
        }
        h = (float)arg->dblVal;
    }

    if((wall = checkwall_below((float)x, (float)z, (float)h)) >= 0)
    {
        (*pretvar)->lVal = (LONG)wall;
    }
    return S_OK;
}

//checkplatformbelow(x,z,a), return the highest platfrom entity below
HRESULT openbor_checkplatformbelow(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    ScriptVariant *arg = NULL;
    DOUBLE x, z, a;

    if(paramCount < 3)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    //ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
    //(*pretvar)->dblVal = (DOUBLE)0;
    ScriptVariant_ChangeType(*pretvar, VT_PTR);
    (*pretvar)->ptrVal = (VOID *)NULL;

    arg = varlist[0];
    if(FAILED(ScriptVariant_DecimalValue(arg, &x)))
    {
        return S_OK;
    }

    arg = varlist[1];
    if(FAILED(ScriptVariant_DecimalValue(arg, &z)))
    {
        return S_OK;
    }

    arg = varlist[2];
    if(FAILED(ScriptVariant_DecimalValue(arg, &a)))
    {
        return S_OK;
    }

    ScriptVariant_ChangeType(*pretvar, VT_PTR);
    (*pretvar)->ptrVal = (VOID *)check_platform_below((float)x, (float)z, (float)a, NULL);
    return S_OK;
}

//checkplatformabove(x,z,a), find a lowest platform above this altitude
HRESULT openbor_checkplatformabove(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    ScriptVariant *arg = NULL;
    DOUBLE x, z, a;

    if(paramCount < 3)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    //ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
    //(*pretvar)->dblVal = (DOUBLE)0;
    ScriptVariant_ChangeType(*pretvar, VT_PTR);
    (*pretvar)->ptrVal = (VOID *)NULL;

    arg = varlist[0];
    if(FAILED(ScriptVariant_DecimalValue(arg, &x)))
    {
        return S_OK;
    }

    arg = varlist[1];
    if(FAILED(ScriptVariant_DecimalValue(arg, &z)))
    {
        return S_OK;
    }

    arg = varlist[2];
    if(FAILED(ScriptVariant_DecimalValue(arg, &a)))
    {
        return S_OK;
    }

    ScriptVariant_ChangeType(*pretvar, VT_PTR);
    (*pretvar)->ptrVal = (VOID *)check_platform_above((float)x, (float)z, (float)a, NULL);
    return S_OK;
}

//checkplatformbetween(x,z,a_min,a_max), find the first platform between these 2 altitudes
HRESULT openbor_checkplatformbetween(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    ScriptVariant *arg = NULL;
    DOUBLE x, z, amin, amax;

    if(paramCount < 3)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    //ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
    //(*pretvar)->dblVal = (DOUBLE)0;
    ScriptVariant_ChangeType(*pretvar, VT_PTR);
    (*pretvar)->ptrVal = (VOID *)NULL;

    arg = varlist[0];
    if(FAILED(ScriptVariant_DecimalValue(arg, &x)))
    {
        return S_OK;
    }

    arg = varlist[1];
    if(FAILED(ScriptVariant_DecimalValue(arg, &z)))
    {
        return S_OK;
    }

    arg = varlist[2];
    if(FAILED(ScriptVariant_DecimalValue(arg, &amin)))
    {
        return S_OK;
    }

    arg = varlist[3];
    if(FAILED(ScriptVariant_DecimalValue(arg, &amax)))
    {
        return S_OK;
    }

    ScriptVariant_ChangeType(*pretvar, VT_PTR);
    (*pretvar)->ptrVal = (VOID *)check_platform_between((float)x, (float)z, (float)amin, (float)amax, NULL);
    return S_OK;
}

//checkbasemap(x,z), return basemap height (float)
HRESULT openbor_checkbasemap(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    ScriptVariant *arg = NULL;
    DOUBLE x, z;

    if(paramCount < 2)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
    (*pretvar)->dblVal = (DOUBLE)0;

    arg = varlist[0];
    if(FAILED(ScriptVariant_DecimalValue(arg, &x)))
    {
        return S_OK;
    }

    arg = varlist[1];
    if(FAILED(ScriptVariant_DecimalValue(arg, &z)))
    {
        return S_OK;
    }

    (*pretvar)->dblVal = (DOUBLE)check_basemap((float)x, (float)z);

    return S_OK;
}

//checkbasemapindex(x,z), return basemap index in (x,z)
HRESULT openbor_checkbasemapindex(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    ScriptVariant *arg = NULL;
    DOUBLE x, z;

    if(paramCount < 2)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
    (*pretvar)->dblVal = (DOUBLE)0;

    arg = varlist[0];
    if(FAILED(ScriptVariant_DecimalValue(arg, &x)))
    {
        return S_OK;
    }

    arg = varlist[1];
    if(FAILED(ScriptVariant_DecimalValue(arg, &z)))
    {
        return S_OK;
    }

    (*pretvar)->dblVal = (DOUBLE)check_basemap_index((float)x, (float)z);

    return S_OK;
}

//generatebasemap(int map_index, float rx, float rz, float x_size, float z_size, float min_y, float max_y, int x_cont) to generate an inclined terrain
HRESULT openbor_generatebasemap(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    DOUBLE rx, rz, x_size, z_size, min_y, max_y;
    LONG index;
    LONG x_cont = (LONG)0;

    if(paramCount < 7)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    if ( FAILED(ScriptVariant_IntegerValue(varlist[0], &index)) )
    {
        *pretvar = NULL;
        return E_FAIL;
    }
    if(paramCount > 7)
    {
        if ( FAILED(ScriptVariant_IntegerValue(varlist[7], &x_cont)) )
        {
            *pretvar = NULL;
            return E_FAIL;
        }
    }
    if ( FAILED(ScriptVariant_DecimalValue(varlist[1], &rx)) || FAILED(ScriptVariant_DecimalValue(varlist[2], &rz)) ||
         FAILED(ScriptVariant_DecimalValue(varlist[3], &x_size)) || FAILED(ScriptVariant_DecimalValue(varlist[4], &z_size)) ||
         FAILED(ScriptVariant_DecimalValue(varlist[5], &min_y)) || FAILED(ScriptVariant_DecimalValue(varlist[6], &max_y))
    )
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    generate_basemap(index, rx, rz, x_size, z_size, min_y, max_y, x_cont);

    return S_OK;
}

HRESULT openbor_openfilestream(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    char *filename = NULL;
    ScriptVariant *arg = NULL;
    LONG location = 0;
    int fsindex;

    FILE *handle = NULL;
    char path[MAX_BUFFER_LEN] = {""};
    char tmpname[MAX_BUFFER_LEN] = {""};
    long size;

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    if(paramCount < 1)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    arg = varlist[0];
    if(arg->vt != VT_STR)
    {
        printf("Filename for openfilestream must be a string.\n");
        *pretvar = NULL;
        return E_FAIL;
    }

    filename = (char *)StrCache_Get(arg->strVal);

    if(paramCount > 1)
    {
        arg = varlist[1];
        if(FAILED(ScriptVariant_IntegerValue(arg, &location)))
        {
            *pretvar = NULL;
            return E_FAIL;
        }
    }

    for(fsindex = 0; fsindex < numfilestreams; fsindex++)
    {
        if(filestreams[fsindex].buf == NULL)
        {
            break;
        }
    }

    if(fsindex == numfilestreams)
    {
        __realloc(filestreams, numfilestreams); //warning, don't ++ here, its a macro
        numfilestreams++;
    }

    // Load file from saves directory if specified
    if(location)
    {
        getBasePath(path, "Saves", 0);
        getPakName(tmpname, -1);
        strcat(path, tmpname);
        strcat(path, "/");
        strcat(path, filename);
        //printf("open path: %s", path);
#ifndef DC
        if(!(fileExists(path)))
        {
            /*
            2011_03_27, DC: Let's be a little more friendly about missing files; this will let a function evaluate if file exists and decide what to do.

            printf("Openfilestream - file specified does not exist.\n"); //Keep this for possible debug mode in the future.
            */
            (*pretvar)->lVal = -1;

            return S_OK;
        }
#endif
        handle = fopen(path, "rb");
        if(handle == NULL)
        {
            (*pretvar)->lVal = -1;
            return S_OK;
        }
        //printf("\nfile opened\n");
        fseek(handle, 0, SEEK_END);
        size = ftell(handle);
        //printf("\n file size %d fsindex %d\n", size, fsindex);
        rewind(handle);
        filestreams[fsindex].buf = malloc(sizeof(*filestreams[fsindex].buf) * (size + 1));
        if(filestreams[fsindex].buf == NULL)
        {
            (*pretvar)->lVal = -1;
            return S_OK;
        }
        fread(filestreams[fsindex].buf, 1, size, handle);
        filestreams[fsindex].buf[size] = 0;
        filestreams[fsindex].size = size;
    }
    else if(buffer_pakfile(filename, &filestreams[fsindex].buf, &filestreams[fsindex].size) != 1)
    {
        printf("Invalid filename used in openfilestream.\n");
        (*pretvar)->lVal = -1;
        return S_OK;
    }

    (*pretvar)->lVal = (LONG)fsindex;

    filestreams[fsindex].pos = 0;
    return S_OK;
}

HRESULT openbor_getfilestreamline(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    int length;
    char *buf;
    char *dst;
    ScriptVariant *arg = NULL;
    LONG filestreamindex;

    if(paramCount < 1)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    arg = varlist[0];
    if(FAILED(ScriptVariant_IntegerValue(arg, &filestreamindex)))
    {
        return S_OK;
    }

    ScriptVariant_ChangeType(*pretvar, VT_STR);

    length = 0;
    buf = filestreams[filestreamindex].buf + filestreams[filestreamindex].pos;
    while(buf[length] && buf[length] != '\n' && buf[length] != '\r')
    {
        ++length;
    }

    (*pretvar)->strVal = StrCache_Pop(length);
    dst = StrCache_Get((*pretvar)->strVal);
    memcpy(dst, buf, length);
    dst[length] = '\0';

    return S_OK;
}

HRESULT openbor_getfilestreamargument(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    ScriptVariant *arg = NULL;
    LONG filestreamindex, argument;
    char *argtype = NULL;

    if(paramCount < 3)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    arg = varlist[0];
    if(FAILED(ScriptVariant_IntegerValue(arg, &filestreamindex)))
    {
        return S_OK;
    }

    arg = varlist[1];
    if(FAILED(ScriptVariant_IntegerValue(arg, &argument)))
    {
        return S_OK;
    }
    ScriptVariant_Clear(*pretvar);

    if(varlist[2]->vt != VT_STR)
    {
        printf("You must give a string value specifying what kind of value you want the argument converted to.\n");
        return E_FAIL;
    }
    argtype = (char *)StrCache_Get(varlist[2]->strVal);

    if(stricmp(argtype, "string") == 0)
    {
        ScriptVariant_ChangeType(*pretvar, VT_STR);
        (*pretvar)->strVal = StrCache_CreateNewFrom(findarg(filestreams[filestreamindex].buf + filestreams[filestreamindex].pos, argument));
    }
    else if(stricmp(argtype, "int") == 0)
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)atoi(findarg(filestreams[filestreamindex].buf + filestreams[filestreamindex].pos, argument));
    }
    else if(stricmp(argtype, "float") == 0)
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)atof(findarg(filestreams[filestreamindex].buf + filestreams[filestreamindex].pos, argument));
    }
    else if(stricmp(argtype, "byte") == 0) // By White Dragon
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)(readByte(filestreams[filestreamindex].buf + filestreams[filestreamindex].pos));
    }
    else
    {
        printf("Invalid type for argument converted to (getfilestreamargument).\n");
        return E_FAIL;
    }

    return S_OK;
}

HRESULT openbor_filestreamnextline(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    ScriptVariant *arg = NULL;
    char *buf;
    size_t pos;
    LONG filestreamindex;

    if(paramCount < 1)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    arg = varlist[0];
    if(FAILED(ScriptVariant_IntegerValue(arg, &filestreamindex)))
    {
        return S_OK;
    }
    pos = filestreams[filestreamindex].pos;
    buf = filestreams[filestreamindex].buf;
    while(buf[pos] && buf[pos] != '\n' && buf[pos] != '\r')
    {
        ++pos;
    }
    while(buf[pos] == '\n' || buf[pos] == '\r')
    {
        ++pos;
    }
    filestreams[filestreamindex].pos = pos;

    return S_OK;
}

HRESULT openbor_getfilestreamposition(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    ScriptVariant *arg = NULL;
    LONG filestreamindex;

    if(paramCount < 1)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    arg = varlist[0];
    if(FAILED(ScriptVariant_IntegerValue(arg, &filestreamindex)))
    {
        return S_OK;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)filestreams[filestreamindex].pos;
    return S_OK;
}

HRESULT openbor_setfilestreamposition(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    ScriptVariant *arg = NULL;
    LONG filestreamindex, position;


    if(paramCount < 2)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    arg = varlist[0];
    if(FAILED(ScriptVariant_IntegerValue(arg, &filestreamindex)))
    {
        return S_OK;
    }

    arg = varlist[1];
    if(FAILED(ScriptVariant_IntegerValue(arg, &position)))
    {
        return S_OK;
    }

    filestreams[filestreamindex].pos = position;
    return S_OK;
}

HRESULT openbor_filestreamappend(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG filestreamindex;
    ScriptVariant *arg = NULL;
    LONG appendtype = -1;
    size_t len1, len2;
    char *temp;
    static char append[2048];

    *pretvar = NULL;
    if(paramCount < 2)
    {
        goto append_error;
    }

    arg = varlist[0];
    if(FAILED(ScriptVariant_IntegerValue(arg, &filestreamindex)))
    {
        goto append_error;
    }

    if(paramCount >= 3)
    {
        arg = varlist[2];
        if(FAILED(ScriptVariant_IntegerValue(arg, &appendtype)))
        {
            goto append_error;
        }
    }

    arg = varlist[1];

    /*
     * By White Dragon to write a byte
     */
    if ( paramCount >= 4 )
    {
        char* argtype = NULL;
        unsigned char byte = (unsigned char)0x00;
        if ( varlist[3]->vt != VT_STR ) goto append_error;

        argtype = (char *)StrCache_Get(varlist[3]->strVal);

        if( stricmp(argtype, "byte") != 0 ) goto append_error;
        else
        {
            int inc = -1; // if buf > 0 (prev bytes) you need to begin from size-1 (index)

            len1 = 1+1; // +1 is the NULL to close the buffer
            len2 = filestreams[filestreamindex].size;

            filestreams[filestreamindex].buf = realloc( filestreams[filestreamindex].buf, sizeof(*temp)*(len1+len2+0) );

            byte = (unsigned char)varlist[1]->lVal;
            //printf("a:%s->%d->%d\n",filestreams[filestreamindex].buf,byte,filestreams[filestreamindex].size);

            if ( len2 <= 0 ) inc = 0;

            filestreams[filestreamindex].buf[filestreams[filestreamindex].size+inc] = byte; // overwrite 0x00 byte
            if (appendtype <= 1) filestreams[filestreamindex].buf[filestreams[filestreamindex].size+1+inc] = 0x00;
            //printf("b:%s\n",filestreams[filestreamindex].buf);

            filestreams[filestreamindex].size = len1 + len2;
        }
    } else
    {
        ScriptVariant_ToString(arg, append);

        len1 = strlen(append);
        len2 = filestreams[filestreamindex].size;

        filestreams[filestreamindex].buf = realloc(filestreams[filestreamindex].buf, sizeof(*temp) * (len1 + len2 + 4));

        if(appendtype == 0)
        {
            append[len1] = ' ';
            append[++len1] = '\0';
            strcpy(filestreams[filestreamindex].buf + len2, "\r\n");
            len2 += 2;
            strcpy(filestreams[filestreamindex].buf + len2, append);
        }
        else if(appendtype == 1)
        {
            append[len1] = ' ';
            append[++len1] = '\0';
            strcpy(filestreams[filestreamindex].buf + len2, append);
        }
        else
        {
            strcpy(filestreams[filestreamindex].buf + len2, append);
        }
        filestreams[filestreamindex].size = len1 + len2;
    }

    return S_OK;

append_error:
    return E_FAIL;

}

HRESULT openbor_createfilestream(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    int fsindex;
    ScriptVariant_Clear(*pretvar);

    for(fsindex = 0; fsindex < numfilestreams; fsindex++)
    {
        if(filestreams[fsindex].buf == NULL)
        {
            break;
        }
    }

    if(fsindex == numfilestreams)
    {
        __realloc(filestreams, numfilestreams); //warning, don't ++ here, its a macro
        numfilestreams++;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)fsindex;

    // Initialize the new filestream
    filestreams[fsindex].pos = 0;
    filestreams[fsindex].size = 0;
    filestreams[fsindex].buf = malloc(sizeof(*filestreams[fsindex].buf) * 128);
    filestreams[fsindex].buf[0] = '\0';
    return S_OK;
}

HRESULT openbor_savefilestream(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    int i;
    LONG filestreamindex;
    ScriptVariant *arg = NULL;
    char *bytearg = NULL, *patharg = NULL;
    FILE *handle = NULL;
    char path[MAX_BUFFER_LEN] = {""};
    char tmpname[MAX_BUFFER_LEN] = {""};

    *pretvar = NULL;

    if(paramCount < 1)
    {
        return E_FAIL;
    }

    arg = varlist[0];
    if(FAILED(ScriptVariant_IntegerValue(arg, &filestreamindex)))
    {
        printf("You must give a valid filestrema handle for savefilestream!\n");
        return E_FAIL;
    }

    arg = varlist[1];
    if(arg->vt != VT_STR)
    {
        printf("Filename for savefilestream must be a string.\n");
        return E_FAIL;
    }

    if (paramCount > 2)
    {
        patharg = (char *)StrCache_Get(varlist[2]->strVal);
        if( varlist[2]->vt != VT_STR )
        {
            printf("The pathname parameter must be a string.\n");
            return E_FAIL;
        }
    }

    if (paramCount > 3) // By White Dragon
    {
        bytearg = (char *)StrCache_Get(varlist[3]->strVal);
        if( stricmp(bytearg, "byte") != 0 )
        {
            printf("%s parameter does not exist.\n",bytearg);
            return E_FAIL;
        }
    }

    // Get the saves directory
    if ( paramCount <= 2 || patharg == NULL )
    {
        getBasePath(path, "Saves", 0);
        getPakName(tmpname, -1);
        strcat(path, tmpname);
        // Add user's filename to path and write the filestream to it
        strcat(path, "/");
    } else // By White Dragon
    {
        strcat(path, "./");
        strcat(path, patharg);
    }
    //printf("path:%s\n",path);

    strcat(path, (char *)StrCache_Get(arg->strVal));

    for(i = strlen(path) - 1; i >= 0; i--)
    {

        if(path[i] == '/' || path[i] == '\\')
        {
            path[i] = 0;
            // Make folder if it doesn't exist
#ifndef DC
            dirExists(path, 1);
#endif
            path[i] = '/';
            break;
        }
    }

    //printf("save path: %s", path);
    handle = fopen(path, "wb");
    if(handle == NULL)
    {
        return E_FAIL;
    }
    fwrite(filestreams[filestreamindex].buf, 1, strlen(filestreams[filestreamindex].buf), handle);

    // add blank line so it can be read successfully
    if ( paramCount <= 3 || (paramCount > 3 && stricmp(bytearg, "byte") != 0 ) ) fwrite("\r\n", 1, 2, handle);
    fclose(handle);

    return S_OK;
}

HRESULT openbor_closefilestream(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG filestreamindex;
    ScriptVariant *arg = NULL;

    *pretvar = NULL;

    if(paramCount < 1)
    {
        return E_FAIL;
    }

    arg = varlist[0];
    if(FAILED(ScriptVariant_IntegerValue(arg, &filestreamindex)))
    {
        return E_FAIL;
    }


    if(filestreams[filestreamindex].buf)
    {
        free(filestreams[filestreamindex].buf);
        filestreams[filestreamindex].buf = NULL;
    }
    return S_OK;
}
//damageentity(entity, other, force, drop, type)
HRESULT openbor_damageentity(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    entity *ent = NULL;
    entity *other = NULL;
    entity *temp = NULL;
    LONG force, drop, type;
    s_collision_attack atk;

    if(paramCount < 1)
    {
        printf("Function requires at least 1 parameter.\n");
        goto de_error;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)0;

    ent = (entity *)(varlist[0])->ptrVal; //retrieve the entity
    if(!ent)
    {
        printf("Invalid entity parameter.\n");
        goto de_error;
    }

    other = ent;

    if(paramCount >= 2 && varlist[1]->ptrVal)
    {
        other = (entity *)(varlist[1])->ptrVal;
    }

    if(paramCount >= 3 )
    {
        force = (LONG)1;
        drop = (LONG)0;
        type = (LONG)ATK_NORMAL;

        if(FAILED(ScriptVariant_IntegerValue((varlist[2]), &force)))
        {
            printf("Wrong force value.\n");
            goto de_error;
        }

        if(paramCount >= 4)
        {
            if(FAILED(ScriptVariant_IntegerValue((varlist[3]), &drop)))
            {
                printf("Wrong drop value.\n");
                goto de_error;
            }
        }
        if(paramCount >= 5)
        {
            if(FAILED(ScriptVariant_IntegerValue((varlist[4]), &type)))
            {
                printf("Wrong type value.\n");
                goto de_error;
            }
        }

        atk = emptyattack;
        atk.attack_force = force;
        atk.attack_drop = drop;
        if(drop)
        {
            atk.dropv.y = (float)DEFAULT_ATK_DROPV_Y;
            atk.dropv.x = (float)DEFAULT_ATK_DROPV_X;
            atk.dropv.z = (float)DEFAULT_ATK_DROPV_Z;
        }
        atk.attack_type = type;
    }
    else
    {
        atk = attack;
    }

    if(!ent->takedamage)
    {
        ent->energy_status.health_current -= atk.attack_force;
        if(ent->energy_status.health_current <= 0)
        {
            kill_entity(ent);
        }
        (*pretvar)->lVal = (LONG)1;
    }
    else
    {
        temp = self;
        self = ent;
        (*pretvar)->lVal = (LONG)self->takedamage(other, &atk, 0);
        self = temp;
    }
    return S_OK;

de_error:
    *pretvar = NULL;
    return E_FAIL;
}

//getcomputeddamage(defender, attacker, force, drop, type), it returns the real damage. Minimum 3 params (defender, attacker, force)
HRESULT openbor_getcomputeddamage(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    entity *defender = NULL;
    entity *attacker = NULL;
    entity *temp = NULL;
    LONG force, drop, type;
    s_collision_attack atk;

    if(paramCount < 3)
    {
        printf("Function requires at least 3 parameters.\n");
        goto gcd_error;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)0;

    force = (LONG)0;
    drop = (LONG)0;
    type = (LONG)ATK_NORMAL;

    defender = (entity *)(varlist[0])->ptrVal; //retrieve the entity
    if(!defender)
    {
        printf("Invalid entity parameter.\n");
        goto gcd_error;
    }

    if(varlist[1]->ptrVal)
    {
        attacker = (entity *)(varlist[1])->ptrVal;
    }

    if(FAILED(ScriptVariant_IntegerValue((varlist[2]), &force)))
    {
        printf("Wrong force value.\n");
        goto gcd_error;
    }

    if(paramCount >= 4)
    {
        if(FAILED(ScriptVariant_IntegerValue((varlist[3]), &drop)))
        {
            printf("Wrong drop value.\n");
            goto gcd_error;
        }
    }
    if(paramCount >= 5)
    {
        if(FAILED(ScriptVariant_IntegerValue((varlist[4]), &type)))
        {
            printf("Wrong type value.\n");
            goto gcd_error;
        }
    }

    atk = emptyattack;
    atk.attack_force = force;
    atk.attack_drop = drop;
    if(drop)
    {
        atk.dropv.y = (float)DEFAULT_ATK_DROPV_Y;
        atk.dropv.x = (float)DEFAULT_ATK_DROPV_X;
        atk.dropv.z = (float)DEFAULT_ATK_DROPV_Z;
    }
    atk.attack_type = type;

    temp = self;
    self = defender;
    (*pretvar)->lVal = (LONG)calculate_force_damage(attacker, &atk);
    self = temp;

    return S_OK;

gcd_error:
    *pretvar = NULL;
    return E_FAIL;
}

//killentity(entity)
HRESULT openbor_killentity(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    entity *ent = NULL;

    if(paramCount < 1)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    ent = (entity *)(varlist[0])->ptrVal; //retrieve the entity
    if(ent == NULL)
    {
        (*pretvar)->lVal = (LONG)0;
        return S_OK;
    }
    kill_entity(ent);
    (*pretvar)->lVal = (LONG)1;
    return S_OK;
}

// dograb
// Damon V. Caskey
// 2013-12-30
//
// Enables initiation of the engine's default grab state between attacker and
// target entities.
//
// dograb(ptr attacker, ptr target, int adjust);
//
// attacker: Entity attempting grab.
// target: Entity to be grabbed.
// adjustcheck: Engine's dograb adjust check flag.
HRESULT openbor_dograb(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME           "dograb(void attacker, void target, int adjust)"
    #define ARG_MINIMUM         2
    #define ARG_ATTACKER        0
    #define ARG_TARGET          1
    #define ARG_ADJUST          2
    #define ARG_ADJUST_DEFAULT  1

    LONG adjust          = ARG_ADJUST_DEFAULT;   // dograb adjust check.
    int result          = S_OK;                 // Function pass/fail result.
    entity *attacker    = NULL;                 // Attacker entity (attempting grab)
    entity *target      = NULL;                 // Target entity (to be grabbed)

    ScriptVariant_Clear(*pretvar);

    // Validate there are at least two parameters (attacker and target entities).
    if(paramCount < ARG_MINIMUM)
    {
        goto error_local;
    }

    // Get adjust check.
    if(paramCount > ARG_ADJUST)
    {
        ScriptVariant_IntegerValue(varlist[ARG_ADJUST], &adjust);
    }

    // Get attacking and target entity.
    attacker = (entity *)(varlist[ARG_ATTACKER])->ptrVal;
    target = (entity *)(varlist[ARG_TARGET])->ptrVal;

    // Validate entities.
    if(!attacker && !target)
    {
        goto error_local;
    }

    // Execute engine's grab function.
    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = dograb(attacker, target, adjust);

    return result;

    // Error trap
    error_local:

    result = E_FAIL;
    printf("\nYou must provide valid entity handles and an optional adjustment: " SELF_NAME);

    // Return result.
    return result;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_ATTACKER
    #undef ARG_TARGET
    #undef ARG_ADJUST
    #undef ARG_ADJUST_DEFAULT
}

//findtarget(entity, int animation);
HRESULT openbor_findtarget(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG i = 0;
    entity *ent = NULL;
    entity *tempself, *target;
    LONG anim = -1;

    if(paramCount > 2)
    {
        ScriptVariant_IntegerValue(varlist[2], &i);
    }

    if(paramCount < 1)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_ChangeType(*pretvar, VT_PTR);

    ent = (entity *)(varlist[0])->ptrVal; //retrieve the entity
    if(!ent)
    {
        ScriptVariant_Clear(*pretvar);
        return S_OK;
    }
    if(paramCount > 1 && FAILED(ScriptVariant_IntegerValue(varlist[1], &anim)))
    {
        return E_FAIL;
    }
    tempself = self;
    self = ent;
    target = normal_find_target((int)anim, i);
    if(!target)
    {
        ScriptVariant_Clear(*pretvar);
    }
    else
    {
        (*pretvar)->ptrVal = (VOID *)target;
    }
    self = tempself;
    return S_OK;
}

//checkrange(entity, target, int ani);
HRESULT openbor_checkrange(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    entity *ent = NULL, *target = NULL;
    LONG ani = 0;
    extern int max_animations;

    if(paramCount < 2)
    {
        goto checkrange_error;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    if(varlist[0]->vt != VT_PTR || varlist[1]->vt != VT_PTR)
    {
        goto checkrange_error;
    }

    ent = (entity *)(varlist[0])->ptrVal; //retrieve the entity
    target = (entity *)(varlist[1])->ptrVal; //retrieve the target

    if(!ent || !target)
    {
        goto checkrange_error;
    }

    if(paramCount > 2 && FAILED(ScriptVariant_IntegerValue(varlist[2], &ani)))
    {
        goto checkrange_error;
    }
    else if(paramCount <= 2)
    {
        ani = ent->animnum;
    }

    if(ani < 0 || ani >= max_animations)
    {
        printf("Animation id out of range: %d / %d.\n", (int)ani, max_animations);
        goto checkrange_error;
    }

    (*pretvar)->lVal = check_range_target_all(ent, target, ani);

    return S_OK;

checkrange_error:
    printf("Function needs at least 2 valid entity handles, the third parameter is optional: checkrange(entity, target, int animnum)\n");
    *pretvar = NULL;
    return E_FAIL;
}

//clearspawnentry();
HRESULT openbor_clearspawnentry(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    *pretvar = NULL;
    memset(&spawnentry, 0, sizeof(spawnentry));
    spawnentry.index = spawnentry.item_properties.index = spawnentry.weaponindex = -1;
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
    _sse_parent,
    _sse_type,
    _sse_weapon,
    _sse_the_end,
};

int mapstrings_setspawnentry(ScriptVariant **varlist, int paramCount)
{
    char *propname;
    int prop;
    static const char *proplist[] =
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
        "parent",
        "type",
        "weapon",
    };

    MAPSTRINGS(varlist[0], proplist, _sse_the_end,
               "Property name '%s' is not supported by setspawnentry.\n");

    return 1;
}

//setspawnentry(propname, value1[, value2, value3, ...]);
HRESULT openbor_setspawnentry(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    LONG ltemp;
    s_model *tempmodel;
    DOUBLE dbltemp;
    int temp, prop;
    ScriptVariant *arg = NULL;

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
        {
            printf("You must give a string value for spawn entry property name.\n");
        }
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
        spawnentry.model = findmodel((char *)StrCache_Get(arg->strVal));
        break;
    case _sse_alias:
        if(arg->vt != VT_STR)
        {
            goto setspawnentry_error;
        }
        strcpy(spawnentry.alias, (char *)StrCache_Get(arg->strVal));
        break;
    case _sse_item:
        if(arg->vt != VT_STR)
        {
            goto setspawnentry_error;
        }
        spawnentry.itemmodel = findmodel((char *)StrCache_Get(arg->strVal));
        spawnentry.item = spawnentry.itemmodel->name;
        spawnentry.item_properties.index = get_cached_model_index(spawnentry.item);
        spawnentry.item_properties.player_count = 0;
        break;
    case _sse_2pitem:
        if(arg->vt != VT_STR)
        {
            goto setspawnentry_error;
        }
        tempmodel = findmodel((char *)StrCache_Get(arg->strVal));
        if(!tempmodel)
        {
            spawnentry.item = NULL;
        }
        else
        {
            spawnentry.item = tempmodel->name;
        }
        spawnentry.item_properties.player_count = 1;
        break;
    case _sse_3pitem:
        if(arg->vt != VT_STR)
        {
            goto setspawnentry_error;
        }
        spawnentry.itemmodel = findmodel((char *)StrCache_Get(arg->strVal));
        spawnentry.item_properties.player_count = 2;
        break;
    case _sse_4pitem:
        if(arg->vt != VT_STR)
        {
            goto setspawnentry_error;
        }
        spawnentry.itemmodel = findmodel((char *)StrCache_Get(arg->strVal));
        spawnentry.item_properties.player_count = 3;
        break;
    case _sse_health:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            spawnentry.health[0] = (LONG)ltemp;
        }
        else
        {
            (*pretvar)->lVal = (LONG)0;
        }
        break;
    case _sse_itemhealth:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            spawnentry.item_properties.health = (LONG)ltemp;
        }
        else
        {
            (*pretvar)->lVal = (LONG)0;
        }
        break;
    case _sse_itemalias:
        if(arg->vt != VT_STR)
        {
            return E_FAIL;
        }
        strcpy(spawnentry.item_properties.alias, (char *)StrCache_Get(arg->strVal));
        break;
    case _sse_2phealth:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            spawnentry.health[1] = (LONG)ltemp;
        }
        else
        {
            (*pretvar)->lVal = (LONG)0;
        }
        break;
    case _sse_3phealth:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            spawnentry.health[2] = (LONG)ltemp;
        }
        else
        {
            (*pretvar)->lVal = (LONG)0;
        }
        break;
    case _sse_4phealth:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            spawnentry.health[3] = (LONG)ltemp;
        }
        else
        {
            (*pretvar)->lVal = (LONG)0;
        }
        break;
    case _sse_coords:
        temp = 1;
        if(SUCCEEDED(ScriptVariant_DecimalValue(arg, &dbltemp)))
        {
            spawnentry.position.x = (DOUBLE)dbltemp;
        }
        else
        {
            temp = 0;
        }
        if(paramCount >= 3 && temp)
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
            {
                spawnentry.position.z = (DOUBLE)dbltemp;
            }
            else
            {
                temp = 0;
            }
        }
        if(paramCount >= 4 && temp)
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
            {
                spawnentry.position.y = (DOUBLE)dbltemp;
            }
            else
            {
                temp = 0;
            }
        }
        (*pretvar)->lVal = (LONG)temp;
        break;
    case _sse_mp:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            spawnentry.mp = (LONG)ltemp;
        }
        else
        {
            (*pretvar)->lVal = (LONG)0;
        }
        break;
    case _sse_map:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            spawnentry.colourmap = (LONG)ltemp;
        }
        else
        {
            (*pretvar)->lVal = (LONG)0;
        }
        break;
    case _sse_itemmap:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            spawnentry.item_properties.colorset = (LONG)ltemp;
        }
        else
        {
            (*pretvar)->lVal = (LONG)0;
        }
        break;
    case _sse_alpha:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            spawnentry.alpha = (LONG)ltemp;
        }
        else
        {
            (*pretvar)->lVal = (LONG)0;
        }
        break;
    case _sse_multiple:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            spawnentry.multiple = (LONG)ltemp;
        }
        else
        {
            (*pretvar)->lVal = (LONG)0;
        }
        break;
    case _sse_dying:
        temp = 1;
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            spawnentry.dying = (LONG)ltemp;
        }
        else
        {
            temp = 0;
        }
        if(paramCount >= 3 && temp)
        {
            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            {
                spawnentry.per1 = (LONG)ltemp;
            }
            else
            {
                temp = 0;
            }
        }
        if(paramCount >= 4 && temp)
        {
            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
            {
                spawnentry.per2 = (LONG)ltemp;
            }
            else
            {
                temp = 0;
            }
        }
        (*pretvar)->lVal = (LONG)temp;
        break;
    case _sse_nolife:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            spawnentry.nolife = (LONG)ltemp;
        }
        else
        {
            (*pretvar)->lVal = (LONG)0;
        }
        break;
    case _sse_boss:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            spawnentry.boss = (LONG)ltemp;
        }
        else
        {
            (*pretvar)->lVal = (LONG)0;
        }
        break;
    case _sse_flip:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            spawnentry.flip = (LONG)ltemp;
        }
        else
        {
            (*pretvar)->lVal = (LONG)0;
        }
        break;
    case _sse_credit:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            spawnentry.credit = (LONG)ltemp;
        }
        else
        {
            (*pretvar)->lVal = (LONG)0;
        }
        break;
    case _sse_aggression:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            spawnentry.aggression = (LONG)ltemp;
        }
        else
        {
            (*pretvar)->lVal = (LONG)0;
        }
        break;
    case _sse_parent:
        if( arg->vt == VT_PTR ) //&& arg->vt != VT_EMPTY
        {
            spawnentry.parent = (entity *)arg->ptrVal;
        }
        else
        {
            (*pretvar)->ptrVal = (VOID *)NULL;
        }
        break;
    case _sse_type:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            spawnentry.entitytype = (LONG)ltemp;
        }
        else
        {
            (*pretvar)->lVal = (LONG)0;
        }
        break;
    case _sse_weapon:
        if(arg->vt != VT_STR)
        {
            goto setspawnentry_error;
        }
        spawnentry.weaponmodel = findmodel((char *)StrCache_Get(arg->strVal));
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
HRESULT openbor_spawn(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    entity *ent;

    if(spawnentry.boss && level)
    {
        level->bossescount++;
        level->numbosses++;
    }

    ent = smartspawn(&spawnentry);

    if(ent)
    {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = (VOID *) ent;
    }
    else
    {
        ScriptVariant_Clear(*pretvar);
    }

    return S_OK;
}

//entity * projectile([0/1], char *name, float x, float z, float a, int direction, int pytype, int type, int map);
HRESULT openbor_projectile(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    DOUBLE temp = 0;
    LONG ltemp = 0;
    entity *ent;
    char *name = NULL;
    float x = 0, z = 0, a = 0;
    int direction = DIRECTION_LEFT;
    int type = 0;
    int projectile_prime = 0;
    int map = 0;

    int relative;

    if(paramCount >= 1 && varlist[0]->vt == VT_INTEGER && varlist[0]->lVal)
    {
        relative = 1;
        paramCount--;
        varlist++;
    }
    else
    {
        relative = 0;
    }

    if(paramCount >= 1 && varlist[0]->vt == VT_STR)
    {
        name = StrCache_Get(varlist[0]->strVal);
    }

    if(paramCount >= 2 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[1], &temp)))
    {
        x = (float)temp;
    }
    else if(relative)
    {
        x = 0;
    }
    else
    {
        x = self->position.x;
    }
    if(paramCount >= 3 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &temp)))
    {
        z = (float)temp;
    }
    else if(relative)
    {
        z = 0;
    }
    else
    {
        z = self->position.z;
    }
    if(paramCount >= 4 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &temp)))
    {
        a = (float)temp;
    }
    else if(relative)
    {
        a  = self->animation->projectile.position.y;
    }
    else
    {
        a = self->position.y + self->animation->projectile.position.y;
    }
    if(paramCount >= 5 && SUCCEEDED(ScriptVariant_IntegerValue(varlist[4], &ltemp)))
    {
        direction = (LONG)ltemp;
    }
    else if(relative)
    {
        direction  = DIRECTION_RIGHT;
    }
    else
    {
        direction = self->direction;
    }
    if(paramCount >= 6 && SUCCEEDED(ScriptVariant_IntegerValue(varlist[5], &ltemp)))
    {

        // Backwards compatibility for modules made before bitwise update
        // of projectile_prime and expect base vs. floor and moving
        // behavior to both be tied to a single 0 or 1 value.
        if((LONG)ltemp)
        {
            projectile_prime = PROJECTILE_PRIME_BASE_FLOOR;
            projectile_prime += PROJECTILE_PRIME_LAUNCH_STATIONARY;
        }
        else
        {
            projectile_prime = PROJECTILE_PRIME_BASE_Y;
            projectile_prime += PROJECTILE_PRIME_LAUNCH_MOVING;
        }
    }
    if(paramCount >= 7 && SUCCEEDED(ScriptVariant_IntegerValue(varlist[6], &ltemp)))
    {
        type = (LONG)ltemp;
    }
    if(paramCount >= 8 && SUCCEEDED(ScriptVariant_IntegerValue(varlist[7], &ltemp)))
    {
        map = (LONG)ltemp;
    }

    if(relative)
    {
        if(self->direction == DIRECTION_RIGHT)
        {
            x += self->position.x;
        }
        else
        {
            x = self->position.x - x;
            direction = !direction;
        }
        z += self->position.z;
        a += self->position.y;
    }

    switch(type)
    {
    default:
    case 0:
        ent = knife_spawn(name, -1, x, z, a, direction, projectile_prime, map);
        break;
    case 1:
        ent = bomb_spawn(name, -1, x, z, a, direction, map);
        break;
    }

    ScriptVariant_ChangeType(*pretvar, VT_PTR);
    (*pretvar)->ptrVal = (VOID *) ent;

    return S_OK;
}




//int rgbcolor(int r, int g, int b);
HRESULT openbor_rgbcolor(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG r, g, b;

    if(paramCount != 3)
    {
        goto rgbcolor_error;
    }
    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &r)))
    {
        goto rgbcolor_error;    // decimal/integer value for red?
    }
    if(FAILED(ScriptVariant_IntegerValue(varlist[1], &g)))
    {
        goto rgbcolor_error;    // decimal/integer value for green?
    }
    if(FAILED(ScriptVariant_IntegerValue(varlist[2], &b)))
    {
        goto rgbcolor_error;    // decimal/integer value for blue?
    }

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

int mapstrings_playerkeys(ScriptVariant **varlist, int paramCount)
{
    char *propname = NULL;
    int i, prop;

    static const char *proplist[] = // for args 2+
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

    for(i = 2; i < paramCount; i++)
    {
        MAPSTRINGS(varlist[i], proplist, _pk_the_end,
                   "Button name '%s' is not supported by playerkeys.");
    }

    return 1;
}

//playerkeys(playerindex, newkey?, key1, key2, ...);
HRESULT openbor_playerkeys(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ltemp;
    int index, newkey;
    int i;
    u64 keys;
    ScriptVariant *arg = NULL;

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
    }
    else
    {
        index = (LONG)ltemp;
    }

    if(SUCCEEDED(ScriptVariant_IntegerValue((varlist[1]), &ltemp)))
    {
        newkey = (LONG)ltemp;
    }
    else
    {
        newkey = 0;
    }

    if(newkey == 1)
    {
        keys = player[index].newkeys;
    }
    else if(newkey == 2)
    {
        keys = player[index].releasekeys;
    }
    else
    {
        keys = player[index].keys;
    }

    for(i = 2; i < paramCount; i++)
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
        else
        {
            (*pretvar)->lVal = (LONG)0;
        }
        if(!((*pretvar)->lVal))
        {
            break;
        }
    }

    return S_OK;
}

//playmusic(name, loop)
HRESULT openbor_playmusic(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    int loop = 0;
    LONG offset = 0;
    char *thename = NULL;

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
        {
            return E_FAIL;
        }
    }


    music(thename, loop, offset);
    return S_OK;
}

//fademusic(fade, name, loop, offset)
HRESULT openbor_fademusic(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    DOUBLE value = 0;
    LONG values[2] = {0, 0};
    *pretvar = NULL;
    if(paramCount < 1)
    {
        goto fademusic_error;
    }
    if(FAILED(ScriptVariant_DecimalValue(varlist[0], &value)))
    {
        goto fademusic_error;
    }
    musicfade[0] = value;
    musicfade[1] = (float)savedata.musicvol;

    if(paramCount == 4)
    {
        strncpy(musicname, StrCache_Get(varlist[1]->strVal), MAX_STR_LEN - 1);
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &values[0])))
        {
            goto fademusic_error;
        }
        if(FAILED(ScriptVariant_IntegerValue(varlist[3], &values[1])))
        {
            goto fademusic_error;
        }
        musicloop = values[0];
        musicoffset = values[1];
    }
    return S_OK;

fademusic_error:
    printf("Function requires 1 value, with an optional 3 for music triggering: fademusic_error(float fade, char name, int loop, unsigned long offset)\n");
    return E_FAIL;
}

//setmusicvolume(left, right)
HRESULT openbor_setmusicvolume(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG channels[2];

    if(paramCount < 1)
    {
        return S_OK;
    }

    if(FAILED(ScriptVariant_IntegerValue(varlist[0], channels)))
    {
        goto setmusicvolume_error;
    }

    if(paramCount > 1)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[1], channels + 1)))
        {
            goto setmusicvolume_error;
        }
    }
    else
    {
        channels[1] = channels[0];
    }

    sound_volume_music((int)channels[0], (int)channels[1]);
    return S_OK;

setmusicvolume_error:
    printf("values must be integers: setmusicvolume(int left, (optional)int right)\n");
    return E_FAIL;
}

//setmusicvolume(left, right)
HRESULT openbor_setmusictempo(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG new_tempo;

    if(paramCount < 1)
    {
        return S_OK;
    }

    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &new_tempo)))
    {
        return E_FAIL;
    }

    sound_music_tempo(new_tempo);
    return S_OK;
}

//pausemusic(togglr)
HRESULT openbor_pausemusic(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
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

//pausesamples(toggle)
HRESULT openbor_pausesamples(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    int pause = 0;
    if(paramCount < 1)
    {
        return S_OK;
    }

    pause = (int)ScriptVariant_IsTrue(varlist[0]);

    sound_pause_sample(pause);
    return S_OK;
}

//pausesample(toggle,channel)
HRESULT openbor_pausesample(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    int pause = 0;
    LONG channel = 0;
    if(paramCount < 2)
    {
        return S_OK;
    }

    pause = (int)ScriptVariant_IsTrue(varlist[0]);
    if(FAILED(ScriptVariant_IntegerValue(varlist[1], &channel)))
    {
        return S_OK;
    }

    sound_pause_single_sample(pause,channel);
    return S_OK;
}

HRESULT openbor_querychannel(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ltemp;
    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ltemp)))
    {
        goto query_error;
    }
    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = sound_query_channel((LONG)ltemp);

    query_error:
    *pretvar = NULL;
    return E_FAIL;
}

HRESULT openbor_stopchannel(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ltemp;
    *pretvar = NULL;
    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ltemp)))
    {
        goto sc_error;
    }
    sound_stop_sample((LONG)ltemp);

    sc_error:
    return E_FAIL;
}

//isactivesample(channel): returns 1 is sample is active, returns 0 otherwise
HRESULT openbor_isactivesample(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ltemp;
    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ltemp)))
    {
        goto error;
    }
    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = sound_is_active((LONG)ltemp);

    error:
    *pretvar = NULL;
    return E_FAIL;
}

//sampleid(channel): returns sample id in channel if sample is active, it returns -1 otherwise
HRESULT openbor_sampleid(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ltemp;
    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ltemp)))
    {
        goto error;
    }
    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = sound_id((LONG)ltemp);

    error:
    *pretvar = NULL;
    return E_FAIL;
}

//playsample(id, priority, lvolume, rvolume, speed, loop)
HRESULT openbor_playsample(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    int i, result;
    LONG value[6] = { -1, 0, savedata.effectvol, savedata.effectvol, 100, 0};

    for(i = 0; i < 6 && i < paramCount; i++)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value + i)))
        {
            goto playsample_error;
        }
    }
    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    if((int)value[5])
    {
        result = sound_loop_sample((int)value[0], (unsigned int)value[1], (int)value[2], (int)value[3], (unsigned int)value[4]);
    }
    else
    {
        result = sound_play_sample((int)value[0], (unsigned int)value[1], (int)value[2], (int)value[3], (unsigned int)value[4]);
    }
    (*pretvar)->lVal = (LONG)result;
    return S_OK;

playsample_error:
    *pretvar = NULL;
    printf("Function requires 6 integer values: playsample(int id, unsigned int priority, int lvolume, int rvolume, unsigned int speed, int loop)\n");
    return E_FAIL;
}

// int loadsample(filename, log)
HRESULT openbor_loadsample(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    int arg = 0;

    if(paramCount < 1)
    {
        goto loadsample_error;
    }
    if(varlist[0]->vt != VT_STR)
    {
        goto loadsample_error;
    }

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
HRESULT openbor_unloadsample(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG id;
    *pretvar = NULL;
    if(paramCount != 1 )
    {
        goto unloadsample_error;
    }

    if(FAILED(ScriptVariant_IntegerValue((varlist[0]), &id)))
    {
        goto unloadsample_error;
    }

    sound_unload_sample((int)id);
    return S_OK;

unloadsample_error:
    printf("Function requires 1 integer value: unloadsample(int id)\n");
    return E_FAIL;
}

//fadeout(type, speed);
HRESULT openbor_fadeout(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG type;
    LONG speed;
    *pretvar = NULL;
    if(paramCount < 1 )
    {
        goto fade_out_error;
    }

    if(FAILED(ScriptVariant_IntegerValue((varlist[0]), &type)))
    {
        goto fade_out_error;
    }
    if(FAILED(ScriptVariant_IntegerValue((varlist[1]), &speed)))

    {
        fade_out((int)type, (int)speed);
    }
    return S_OK;

fade_out_error:
    printf("Function requires 2 integer values: fade_out(int type, int speed)\n");
    return E_FAIL;
}

//changepalette(index);
HRESULT openbor_changepalette(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG index;

    *pretvar = NULL;

    if(paramCount < 1)
    {
        goto changepalette_error;
    }

    if(FAILED(ScriptVariant_IntegerValue((varlist[0]), &index)))
    {
        goto changepalette_error;
    }

    change_system_palette((int)index);

    return S_OK;

changepalette_error:
    printf("Function requires 1 integer value: changepalette(int index)\n");
    return E_FAIL;
}

//changelight(x, z);
HRESULT openbor_changelight(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG x, z;
    extern s_axis_plane_vertical_int light;
    ScriptVariant *arg = NULL;

    *pretvar = NULL;
    if(paramCount < 2)
    {
        goto changelight_error;
    }

    arg = varlist[0];
    if(arg->vt != VT_EMPTY)
    {
        if(FAILED(ScriptVariant_IntegerValue(arg, &x)))
        {
            goto changelight_error;
        }
        light.x = (int)x;
    }

    arg = varlist[1];
    if(arg->vt != VT_EMPTY)
    {
        if(FAILED(ScriptVariant_IntegerValue(arg, &z)))
        {
            goto changelight_error;
        }
        light.y = (int)z;
    }

    return S_OK;
changelight_error:
    printf("Function requires 2 integer values: changepalette(int x, int z)\n");
    return E_FAIL;
}

//changeshadowcolor(color, alpha);
// color = 0 means no gfxshadow, -1 means don't fill the shadow with colour
// alpha default to 2, <=0 means no alpha effect
HRESULT openbor_changeshadowcolor(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG c, a;
    extern int            shadowcolor;
    extern int            shadowalpha;

    *pretvar = NULL;
    if(paramCount < 1)
    {
        goto changeshadowcolor_error;
    }

    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &c)))
    {
        goto changeshadowcolor_error;
    }

    shadowcolor = (int)c;

    if(paramCount > 1)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[1], &a)))
        {
            goto changeshadowcolor_error;
        }
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
    _top_a,
    _top_font,
    _top_text,
    _top_time,
    _top_x,
    _top_y,
    _top_z,
    _top_the_end,
};

int mapstrings_textobjproperty(ScriptVariant **varlist, int paramCount)
{
    char *propname = NULL;
    int prop;

    static const char *proplist[] =
    {
        "a",
        "font",
        "text",
        "time",
        "x",
        "y",
        "z",
    };

    if(paramCount < 2)
    {
        return 1;
    }

    MAPSTRINGS(varlist[1], proplist, _top_the_end,
               "'%s' is not a valid textobj property.\n");

    return 1;
}

HRESULT openbor_gettextobjproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ind;
    int propind;

    if(paramCount < 2)
    {
        goto gettextobjproperty_error;
    }


    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
    {
        printf("Function's 1st argument must be a numeric value: gettextproperty(int index, \"property\")\n");
        goto gettextobjproperty_error;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    mapstrings_textobjproperty(varlist, paramCount);

    if(ind < 0 || ind >= level->numtextobjs)
    {
        (*pretvar)->lVal = 0;
        return S_OK;
    }

    if(varlist[1]->vt != VT_INTEGER)
    {
        if(varlist[1]->vt != VT_STR)
        {
            printf("Function gettextobjproperty must have a string property name.\n");
        }
        goto gettextobjproperty_error;
    }

    propind = varlist[1]->lVal;

    switch(propind)
    {
    case _top_font:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->textobjs[ind].font;
        break;
    }
    case _top_text:
    {
        ScriptVariant_ChangeType(*pretvar, VT_STR);
        (*pretvar)->strVal = StrCache_CreateNewFrom(level->textobjs[ind].text);
        break;
    }
    case _top_time:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->textobjs[ind].time;
        break;
    }
    case _top_x:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->textobjs[ind].position.x;
        break;
    }
    case _top_y:
    case _top_a:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->textobjs[ind].position.y;
        break;
    }
    case _top_z:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->textobjs[ind].position.z;
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

HRESULT openbor_changetextobjproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ind;
    int propind;
    static char buf[MAX_STR_VAR_LEN];
    LONG ltemp;
    const char *ctotext = "changetextobjproperty(int index, \"property\", value)";

    *pretvar = NULL;

    if(paramCount < 3)
    {
        printf("Function needs at last 3 parameters: %s\n", ctotext);
        return E_FAIL;
    }

    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
    {
        printf("Function's 1st argument must be a numeric value: %s\n", ctotext);
        return E_FAIL;
    }

    mapstrings_textobjproperty(varlist, paramCount);

    if(ind < 0)
    {
        printf("Invalid textobj index, must be >= 0\n");
        return E_FAIL;
    }
    else if (ind >= level->numtextobjs)
    {
        __reallocto(level->textobjs, level->numtextobjs, ind + 1);
        level->numtextobjs = ind + 1;
    }

    if(varlist[1]->vt != VT_INTEGER)
    {
        printf("Invalid property type for changetextobjproperty.\n");
        return E_FAIL;
    }

    propind = varlist[1]->lVal;

    switch(propind)
    {
    case _top_font:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            level->textobjs[ind].font = (LONG)ltemp;
        }
        else
        {
            goto changetextobjproperty_error;
        }
        break;
    }
    case _top_text:
    {
        ScriptVariant_ToString(varlist[2], buf);
        level->textobjs[ind].text = malloc(MAX_STR_VAR_LEN);
        strncpy(level->textobjs[ind].text, buf, MAX_STR_VAR_LEN);
        break;
    }
    case _top_time:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            level->textobjs[ind].time = (LONG)ltemp;
        }
        else
        {
            goto changetextobjproperty_error;
        }
        break;
    }
    case _top_x:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            level->textobjs[ind].position.x = (LONG)ltemp;
        }
        else
        {
            goto changetextobjproperty_error;
        }
        break;
    }
    case _top_a:
    case _top_y:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            level->textobjs[ind].position.y = (LONG)ltemp;
        }
        else
        {
            goto changetextobjproperty_error;
        }
        break;
    }
    case _top_z:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            level->textobjs[ind].position.z = (LONG)ltemp;
        }
        else
        {
            goto changetextobjproperty_error;
        }
        break;
    }
    default:
        //printf("Property name '%s' is not supported by function changetextobjproperty.\n", propname);
        return E_FAIL;
        break;
    }

    return S_OK;

changetextobjproperty_error:
    ScriptVariant_ToString(varlist[2], buf);
    printf("Invalid textobj value: %s\n", buf);
    return E_FAIL;
}

// settextobj(int index, int x, int y, int font, int z, char text, int time {optional})
HRESULT openbor_settextobj(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ind;
    LONG X, Y, Z, F, T = 0;
    static char buf[MAX_STR_VAR_LEN];
    const char *stotext = "settextobj(int index, int x, int y, int font, int z, char text, int time {optional})";

    *pretvar = NULL;


    if(paramCount < 6)
    {
        printf("Function needs at least 6 parameters: %s\n", stotext);
        return E_FAIL;
    }

    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
    {
        printf("Function's 1st argument must be a numeric value: %s\n", stotext);
        return E_FAIL;
    }

    if(ind < 0)
    {
        printf("Invalid textobj index, must be >= 0\n");
        return E_FAIL;
    }
    else if(ind >= level->numtextobjs)
    {
        __reallocto(level->textobjs, level->numtextobjs, ind + 1);
        level->numtextobjs = ind + 1;
    }

    if(FAILED(ScriptVariant_IntegerValue(varlist[1], &X)))
    {
        goto settextobj_error;
    }
    if(FAILED(ScriptVariant_IntegerValue(varlist[2], &Y)))
    {
        goto settextobj_error;
    }
    if(FAILED(ScriptVariant_IntegerValue(varlist[3], &F)))
    {
        goto settextobj_error;
    }
    if(FAILED(ScriptVariant_IntegerValue(varlist[4], &Z)))
    {
        goto settextobj_error;
    }
    ScriptVariant_ToString(varlist[5], buf);
    if(paramCount >= 7 && FAILED(ScriptVariant_IntegerValue(varlist[6], &T)))
    {
        goto settextobj_error;
    }

    level->textobjs[ind].time = (int)T;
    level->textobjs[ind].position.x = (int)X;
    level->textobjs[ind].position.y = (int)Y;
    level->textobjs[ind].position.z = (int)Z;
    level->textobjs[ind].font = (int)F;

    if(!level->textobjs[ind].text)
    {
        level->textobjs[ind].text = (char *)malloc(MAX_STR_VAR_LEN);
    }
    strncpy(level->textobjs[ind].text, buf, MAX_STR_VAR_LEN);

    return S_OK;

settextobj_error:
    printf("Invalid value(s) for settextobj: %s\n", stotext);
    return E_FAIL;
}

HRESULT openbor_cleartextobj(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ind;
    const char *cltotext = "cleartextobj(int index)";

    *pretvar = NULL;

    if(paramCount < 1)
    {
        printf("Function needs at least 1 parameter: %s\n", cltotext);
        return E_FAIL;
    }

    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
    {
        printf("Function's 1st argument must be a numeric value: %s\n", cltotext);
        return E_FAIL;
    }

    if(ind < 0 || ind >= level->numtextobjs)
    {
        return S_OK;
    }

    level->textobjs[ind].time = 0;
    level->textobjs[ind].position.x = 0;
    level->textobjs[ind].position.y = 0;
    level->textobjs[ind].font = 0;
    level->textobjs[ind].position.z = 0;
    if(level->textobjs[ind].text)
    {
        free(level->textobjs[ind].text);
    }
    level->textobjs[ind].text = NULL;
    return S_OK;
}

// ===== get layer type ======
enum getlt_enum
{
    _glt_background,
    _glt_bglayer,
    _glt_fglayer,
    _glt_frontpanel,
    _glt_generic,
    _glt_neon,
    _glt_panel,
    _glt_screen,
    _glt_water,
    _glt_the_end,
};


// ===== getbglayerproperty ======
enum getbglp_enum
{
    _glp_alpha,
    _glp_amplitude,
    _glp_bgspeedratio,
    _glp_enabled,
    _glp_neon,
    _glp_quake,
    _glp_transparency,
    _glp_watermode,
    _glp_wavelength,
    _glp_wavespeed,
    _glp_xoffset,
    _glp_xratio,
    _glp_xrepeat,
    _glp_xspacing,
    _glp_z,
    _glp_zoffset,
    _glp_zratio,
    _glp_zrepeat,
    _glp_zspacing,
    _glp_the_end,
};

int mapstrings_layerproperty(ScriptVariant **varlist, int paramCount)
{
    char *propname = NULL;
    int prop;

    static const char *proplist[] =
    {
        "alpha",
        "amplitude",
        "bgspeedratio",
        "enabled",
        "neon",
        "quake",
        "transparency",
        "watermode",
        "wavelength",
        "wavespeed",
        "xoffset",
        "xratio",
        "xrepeat",
        "xspacing",
        "z",
        "zoffset",
        "zratio",
        "zrepeat",
        "zspacing",
    };

    static const char *typelist[] =
    {
        "background",
        "bglayer",
        "fglayer",
        "frontpanel",
        "generic",
        "neon",
        "panel",
        "water",
    };

    if(paramCount < 3)
    {
        return 1;
    }
    MAPSTRINGS(varlist[0], typelist, _glt_the_end,
               "Type name '%s' is not supported by function getlayerproperty.\n");
    MAPSTRINGS(varlist[2], proplist, _glp_the_end,
               "Property name '%s' is not supported by function getlayerproperty.\n");

    return 1;
}

HRESULT _getlayerproperty(s_layer *layer, int propind, ScriptVariant **pretvar)
{

    switch(propind)
    {
    case _glp_alpha:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)layer->drawmethod.alpha;
        break;
    }
    case _glp_amplitude:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)layer->drawmethod.water.amplitude;
        break;
    }
    case _glp_bgspeedratio:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)layer->bgspeedratio;
        break;
    }
    case _glp_enabled:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)layer->enabled;
        break;
    }
    case _glp_neon:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)layer->neon;
        break;
    }
    case _glp_quake:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)layer->quake;
        break;
    }
    case _glp_transparency:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)layer->drawmethod.transbg;
        break;
    }
    case _glp_watermode:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)layer->drawmethod.water.watermode;
        break;
    }

    case _glp_wavelength:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)layer->drawmethod.water.wavelength;
        break;
    }
    case _glp_wavespeed:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)layer->drawmethod.water.wavespeed;
        break;
    }
    case _glp_xoffset:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)layer->offset.x;
        break;
    }
    case _glp_xratio:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)layer->ratio.x;
        break;
    }
    case _glp_xrepeat:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)layer->drawmethod.xrepeat;
        break;
    }
    case _glp_xspacing:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)layer->spacing.x;
        break;
    }
    case _glp_z:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)layer->z;
        break;
    }
    case _glp_zoffset:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)layer->offset.z;
        break;
    }
    case _glp_zratio:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)layer->ratio.z;
        break;
    }
    case _glp_zrepeat:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)layer->drawmethod.yrepeat;
        break;
    }
    case _glp_zspacing:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)layer->spacing.z;
        break;
    }
    default:
        *pretvar = NULL;
        return E_FAIL;
    }
    return S_OK;
}

HRESULT _changelayerproperty(s_layer *layer, int propind, ScriptVariant *var)
{
    LONG temp;
    DOUBLE temp2;
    switch(propind)
    {
    case _glp_alpha:
    {
        if(FAILED(ScriptVariant_IntegerValue(var, &temp)))
        {
            return E_FAIL;
        }
        layer->drawmethod.alpha = temp;
        break;
    }
    case _glp_amplitude:
    {
        if(FAILED(ScriptVariant_IntegerValue(var, &temp)))
        {
            return E_FAIL;
        }
        layer->drawmethod.water.amplitude = temp;
        break;
    }
    case _glp_bgspeedratio:
    {
        if(FAILED(ScriptVariant_DecimalValue(var, &temp2)))
        {
            return E_FAIL;
        }
        layer->bgspeedratio = temp2;
        break;
    }
    case _glp_enabled:
    {
        if(FAILED(ScriptVariant_IntegerValue(var, &temp)))
        {
            return E_FAIL;
        }
        layer->enabled = temp;
        break;
    }
    case _glp_neon:
    {
        if(FAILED(ScriptVariant_IntegerValue(var, &temp)))
        {
            return E_FAIL;
        }
        layer->neon = temp;
        break;
    }
    case _glp_quake:
    {
        if(FAILED(ScriptVariant_IntegerValue(var, &temp)))
        {
            return E_FAIL;
        }
        layer->quake = temp;
        break;
    }
    case _glp_transparency:
    {
        if(FAILED(ScriptVariant_IntegerValue(var, &temp)))
        {
            return E_FAIL;
        }
        layer->drawmethod.transbg = temp;
        break;
    }
    case _glp_watermode:
    {
        if(FAILED(ScriptVariant_IntegerValue(var, &temp)))
        {
            return E_FAIL;
        }
        layer->drawmethod.water.watermode = temp;
        break;
    }

    case _glp_wavelength:
    {
        if(FAILED(ScriptVariant_DecimalValue(var, &temp2)))
        {
            return E_FAIL;
        }
        layer->drawmethod.water.wavelength = temp2;
        break;
    }
    case _glp_wavespeed:
    {
        if(FAILED(ScriptVariant_DecimalValue(var, &temp2)))
        {
            return E_FAIL;
        }
        layer->drawmethod.water.wavespeed = temp2;
        break;
    }
    case _glp_xoffset:
    {
        if(FAILED(ScriptVariant_IntegerValue(var, &temp)))
        {
            return E_FAIL;
        }
        layer->offset.x = temp;
        break;
    }
    case _glp_xratio:
    {
        if(FAILED(ScriptVariant_DecimalValue(var, &temp2)))
        {
            return E_FAIL;
        }
        layer->ratio.x = temp2;
        break;
    }
    case _glp_xrepeat:
    {
        if(FAILED(ScriptVariant_IntegerValue(var, &temp)))
        {
            return E_FAIL;
        }
        layer->drawmethod.xrepeat = temp;
        break;
    }
    case _glp_xspacing:
    {
        if(FAILED(ScriptVariant_IntegerValue(var, &temp)))
        {
            return E_FAIL;
        }
        layer->spacing.x = temp;
        break;
    }
    case _glp_z:
    {
        if(FAILED(ScriptVariant_IntegerValue(var, &temp)))
        {
            return E_FAIL;
        }
        layer->z = temp;
        break;
    }
    case _glp_zoffset:
    {
        if(FAILED(ScriptVariant_IntegerValue(var, &temp)))
        {
            return E_FAIL;
        }
        layer->offset.z = temp;
        break;
    }
    case _glp_zratio:
    {
        if(FAILED(ScriptVariant_DecimalValue(var, &temp2)))
        {
            return E_FAIL;
        }
        layer->ratio.z = temp2;
        break;
    }
    case _glp_zrepeat:
    {
        if(FAILED(ScriptVariant_IntegerValue(var, &temp)))
        {
            return E_FAIL;
        }
        layer->drawmethod.yrepeat = temp;
        break;
    }
    case _glp_zspacing:
    {
        if(FAILED(ScriptVariant_IntegerValue(var, &temp)))
        {
            return E_FAIL;
        }
        layer->spacing.z = temp;
        break;
    }
    default:
        return E_FAIL;
    }
    return S_OK;
}

s_layer *_getlayer(int type, int ind)
{
    switch(type)
    {
    case _glt_background:
        return level->background;
    case _glt_bglayer:
        if(ind < 0 || ind >= level->numbglayers)
        {
            return NULL;
        }
        return level->bglayers[ind];
    case _glt_fglayer:
        if(ind < 0 || ind >= level->numfglayers)
        {
            return NULL;
        }
        return level->fglayers[ind];
    case _glt_frontpanel:
        if(ind < 0 || ind >= level->numfrontpanels)
        {
            return NULL;
        }
        return level->frontpanels[ind];
    case _glt_generic:
        if(ind < 0 || ind >= level->numgenericlayers)
        {
            return NULL;
        }
        return level->genericlayers[ind];
    case _glt_neon:
        if(ind < 0 || ind >= level->numpanels)
        {
            return NULL;
        }
        return level->panels[ind][1];
    case _glt_panel:
        if(ind < 0 || ind >= level->numpanels)
        {
            return NULL;
        }
        return level->panels[ind][0];
    case _glt_screen:
        if(ind < 0 || ind >= level->numpanels)
        {
            return NULL;
        }
        return level->panels[ind][2];
    case _glt_water:
        if(ind < 0 || ind >= level->numwaters)
        {
            return NULL;
        }
        return level->waters[ind];
    default:
        return NULL;
    }
}

// getlayerproperty(type, index, propertyname);
HRESULT openbor_getlayerproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ind;
    int propind, type;
    s_layer *layer = NULL;

    if(paramCount < 3)
    {
        goto getlayerproperty_error;
    }

    mapstrings_layerproperty(varlist, paramCount);

    type = varlist[0]->lVal;
    propind = varlist[2]->lVal;

    if(FAILED(ScriptVariant_IntegerValue(varlist[1], &ind)))
    {
        goto getlayerproperty_error2;
    }

    layer = _getlayer(type, (int)ind);

    if(layer == NULL)
    {
        goto getlayerproperty_error2;
    }

    if(FAILED(_getlayerproperty(layer, propind, pretvar)))
    {
        goto getlayerproperty_error3;
    }

    return S_OK;

getlayerproperty_error:
    *pretvar = NULL;
    printf("Function getlayerproperty must have 3 parameters: layertype, index and propertyname\n");
    return E_FAIL;
getlayerproperty_error2:
    *pretvar = NULL;
    printf("Layer not found!\n");
    return E_FAIL;
getlayerproperty_error3:
    *pretvar = NULL;
    printf("Bad property name or value.\n");
    return E_FAIL;
}

// changelayerproperty(type, index, propertyname);
HRESULT openbor_changelayerproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ind;
    int propind, type;
    s_layer *layer = NULL;
    *pretvar = NULL;

    if(paramCount < 4)
    {
        goto chglayerproperty_error;
    }

    mapstrings_layerproperty(varlist, paramCount);

    type = varlist[0]->lVal;
    propind = varlist[2]->lVal;

    if(FAILED(ScriptVariant_IntegerValue(varlist[1], &ind)))
    {
        goto chglayerproperty_error2;
    }

    layer = _getlayer(type, (int)ind);

    if(layer == NULL)
    {
        goto chglayerproperty_error2;
    }

    if(FAILED(_changelayerproperty(layer, propind, varlist[3])))
    {
        goto chglayerproperty_error3;
    }

    return S_OK;

chglayerproperty_error:
    printf("Function changelayerproperty must have 4 parameters: layertype, index, propertyname and value\n");
    return E_FAIL;
chglayerproperty_error2:
    printf("Layer not found!\n");
    return E_FAIL;
chglayerproperty_error3:
    printf("Layer property not understood or bad value.\n");
    return E_FAIL;
}

//shutdown(status, message)
HRESULT openbor_shutdown(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ltemp = 0;

    *pretvar = NULL;

    if(paramCount > 0 && FAILED(ScriptVariant_IntegerValue(varlist[0], &ltemp)))
    {
        goto shutdown_error;
    }
    if(paramCount > 1 && varlist[1]->vt != VT_STR)
    {
        goto shutdown_error;
    }

    borShutdown((LONG)ltemp,  paramCount > 1 ? StrCache_Get(varlist[1]->strVal) : (DEFAULT_SHUTDOWN_MESSAGE));

    return S_OK;
shutdown_error:
    printf("shutdown(status, message): both parameters are optional but must be valid.\n");
    return E_FAIL;
}

//jumptobranch(name, immediate)
HRESULT openbor_jumptobranch(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ltemp;
    extern char branch_name[MAX_NAME_LEN + 1];
    *pretvar = NULL;
    if(paramCount < 1)
    {
        goto jumptobranch_error;
    }
    if(varlist[0]->vt != VT_STR)
    {
        goto jumptobranch_error;
    }

    strncpy(branch_name, StrCache_Get(varlist[0]->strVal), MIN(MAX_NAME_LEN, MAX_STR_VAR_LEN)); // copy the string value to branch name

    if(paramCount >= 2)
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[1], &ltemp)))
        {
            endgame = (LONG)ltemp;
            // 1 means goto that level immediately, or, wait until the level is complete
        }
        else
        {
            goto jumptobranch_error;
        }
    }

    return S_OK;
jumptobranch_error:
    printf("Function requires 1 string value, the second argument is optional(int): jumptobranch(name, immediate)\n");
    return E_FAIL;
}

//bindentity(entity, target, x, z, a, direction, binding.ani_bind);
//bindentity(entity, NULL()); // unbind
HRESULT openbor_bindentity(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    entity *ent = NULL;
    entity *other = NULL;
    ScriptVariant *arg = NULL;
    void adjust_bind(entity * e);
    LONG x = 0, z = 0, a = 0, dir = 0, anim = 0, sortid = -1;

    *pretvar = NULL;
    if(paramCount < 2)
    {
        return E_FAIL;
    }

    ent = (entity *)(varlist[0])->ptrVal; //retrieve the entity
    if(!ent)
    {
        return S_OK;
    }

    other = (entity *)(varlist[1])->ptrVal;
    if(!other)
    {
        ent->binding.ent = NULL;
        ent->binding.bind_toggle.x = 0;
        ent->binding.bind_toggle.z = 0;
        ent->binding.bind_toggle.y = 0;
        return S_OK;
    }

    ent->binding.ent = other;
    ent->binding.sortid = sortid;

    if(paramCount < 3)
    {
        goto BIND;
    }
    // x
    arg = varlist[2];
    if(arg->vt != VT_EMPTY)
    {
        if(FAILED(ScriptVariant_IntegerValue(arg, &x)))
        {
            return E_FAIL;
        }

        ent->binding.offset.x = (int)x;
        ent->binding.bind_toggle.x = 1;
    } else ent->binding.bind_toggle.x = 0;
    if(paramCount < 4)
    {
        goto BIND;
    }
    // z
    arg = varlist[3];
    if(arg->vt != VT_EMPTY)
    {
        if(FAILED(ScriptVariant_IntegerValue(arg, &z)))
        {
            return E_FAIL;
        }
        ent->binding.offset.z = (int)z;
        ent->binding.bind_toggle.z = 1;
    } else ent->binding.bind_toggle.z = 0;
    if(paramCount < 5)
    {
        goto BIND;
    }
    // a
    arg = varlist[4];
    if(arg->vt != VT_EMPTY)
    {
        if(FAILED(ScriptVariant_IntegerValue(arg, &a)))
        {
            return E_FAIL;
        }
        ent->binding.offset.y = (int)a;
        ent->binding.bind_toggle.y = 1;
    } else ent->binding.bind_toggle.y = 0;
    if(paramCount < 6)
    {
        goto BIND;
    }
    // direction
    arg = varlist[5];
    if(arg->vt != VT_EMPTY)
    {
        if(FAILED(ScriptVariant_IntegerValue(arg, &dir)))
        {
            return E_FAIL;
        }
        ent->binding.direction = (int)dir;
    }
    if(paramCount < 7)
    {
        goto BIND;
    }
    // animation
    arg = varlist[6];
    if(arg->vt != VT_EMPTY)
    {
        if(FAILED(ScriptVariant_IntegerValue(arg, &anim)))
        {
            return E_FAIL;
        }
        ent->binding.ani_bind = (int)anim;
    }
    if(paramCount < 8)
    {
        goto BIND;
    }
    // sortid
    arg = varlist[7];
    if(arg->vt != VT_EMPTY)
    {
        if(FAILED(ScriptVariant_IntegerValue(arg, &sortid)))
        {
            return E_FAIL;
        }
        ent->binding.sortid = (int)sortid;
    }

BIND:
    adjust_bind(ent);

    return S_OK;
}

//array(size);
HRESULT openbor_array(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG size;
    Varlist *array;

    if(paramCount < 1 || FAILED(ScriptVariant_IntegerValue(varlist[0], &size)) || size < 0)
    {
        printf("Function requires 1 positive int value: array(int size)\n");
        goto array_error;
    }

    ScriptVariant_ChangeType(*pretvar, VT_PTR);
    array = malloc(sizeof(*array));
    (*pretvar)->ptrVal = (VOID *)array;

    if((*pretvar)->ptrVal == NULL)
    {
        printf("Not enough memory: array(%d)\n", (int)size);
        goto array_error;
    }

    Varlist_Init(array, size);

    List_InsertAfter(&scriptheap, (void *)(array), "openbor_array");
    return S_OK;

array_error:
    (*pretvar) = NULL;
    return E_FAIL;
}

//size(array)
HRESULT openbor_size(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    Varlist *array;
    if(paramCount < 1 || varlist[0]->vt != VT_PTR || !(array = (Varlist *)varlist[0]->ptrVal) || array->magic != varlist_magic)
    {
        goto size_error;
    }

    // By White Dragon
    if( array->list->size != 0 ) //or array->list->first
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)array->list->size;
    }
    else
    {
        ScriptVariant_Copy(*pretvar, array->vars);
    }

    return S_OK;
size_error:
    printf("Function requires 1 array handle: %s(array)\n", "size");
    (*pretvar) = NULL;
    return E_FAIL;
}

//get(array, index);
HRESULT openbor_get(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    ScriptVariant *ptmpvar;
    Varlist *array;
    LONG ltemp;

    if(paramCount < 2 || varlist[0]->vt != VT_PTR || !(array = (Varlist *)varlist[0]->ptrVal) || array->magic != varlist_magic)
    {
        goto get_error;
    }

    if(varlist[1]->vt == VT_STR)
    {
        ptmpvar = Varlist_GetByName(array, StrCache_Get(varlist[1]->strVal));
    }
    else if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[1], &ltemp)))
    {
        ptmpvar = Varlist_GetByIndex(array, (LONG)ltemp);
    }
    else
    {
        goto get_error;
    }

    if(ptmpvar)
    {
        ScriptVariant_Copy(*pretvar,  ptmpvar);
    }
    else
    {
        ScriptVariant_Clear(*pretvar);
    }
    return S_OK;

get_error:
    printf("Function requires 1 array handle and 1 int value: get(array, int index)\n");
    (*pretvar) = NULL;
    return E_FAIL;
}

//set(array, index, value);
HRESULT openbor_set(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    Varlist *array;
    LONG ltemp;

    *pretvar = NULL;
    if(paramCount < 3 || varlist[0]->vt != VT_PTR || !(array = (Varlist *)varlist[0]->ptrVal) || array->magic != varlist_magic)
    {
        goto set_error;
    }

    if(varlist[1]->vt == VT_STR)
    {
        Varlist_SetByName(array, StrCache_Get(varlist[1]->strVal), varlist[2]);
    }
    else if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[1], &ltemp)))
    {
        Varlist_SetByIndex(array, (LONG)ltemp, varlist[2]);
    }
    else
    {
        goto set_error;
    }

    return S_OK;

set_error:
    printf("Function requires 1 array handle, 1 int value and 1 value: set(array, int index, value)\n");
    return E_FAIL;
}

//delete(array, index); // By White Dragon
HRESULT openbor_delete(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    Varlist *array;
    LONG index;

    *pretvar = NULL;
    if(paramCount < 2 || varlist[0]->vt != VT_PTR || !(array = (Varlist *)varlist[0]->ptrVal) || array->magic != varlist_magic)
    {
        goto set_error;
    }

    if(varlist[1]->vt == VT_STR)
    {
        if ( !Varlist_DeleteByName(array, StrCache_Get(varlist[1]->strVal)) ) goto set_error;
    }
    else if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[1], &index)))
    {
        if ( !Varlist_DeleteByIndex(array, (int)index) ) goto set_error;
    }
    else
    {
        goto set_error;
    }

    return S_OK;

set_error:
    printf("Function requires 1 array handle and 1 int value (index): delete(array, index)\n");
    return E_FAIL;
}

//add(array, index); // By White Dragon
HRESULT openbor_add(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    Varlist *array;
    LONG index;

    *pretvar = NULL;
    if(paramCount < 2 || varlist[0]->vt != VT_PTR || !(array = (Varlist *)varlist[0]->ptrVal) || array->magic != varlist_magic)
    {
        goto add_error;
    }

    if(varlist[1]->vt == VT_STR)
    {
        Varlist_SetByName(array, StrCache_Get(varlist[1]->strVal), varlist[2]);
    }
    else if( SUCCEEDED(ScriptVariant_IntegerValue(varlist[1], &index)) )
    {
        if ( !Varlist_AddByIndex(array, (int)index, varlist[2]) ) goto add_error;
    }
    else
    {
        goto add_error;
    }

    return S_OK;

add_error:
    printf("Function requires 1 array handle and 1 int value (index): add(array, index)\n");
    return E_FAIL;
}

//reset(array)
HRESULT openbor_reset(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    Varlist *array;
    if(paramCount < 1 || varlist[0]->vt != VT_PTR || !(array = (Varlist *)varlist[0]->ptrVal) || array->magic != varlist_magic)
    {
        goto reset_error;
    }
    List_Reset(array->list);

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)(array->list->current != NULL);

    return S_OK;
reset_error:
    printf("Function requires 1 array handle: %s(array)\n", "reset");
    (*pretvar) = NULL;
    return E_FAIL;
}

//next(array)
HRESULT openbor_next(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    Varlist *array;
    if(paramCount < 1 || varlist[0]->vt != VT_PTR || !(array = (Varlist *)varlist[0]->ptrVal) || array->magic != varlist_magic)
    {
        goto next_error;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)(List_GotoNext(array->list));

    return S_OK;
next_error:
    printf("Function requires 1 array handle: %s(array)\n", "next");
    (*pretvar) = NULL;
    return E_FAIL;
}

//previous(array)
HRESULT openbor_previous(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    Varlist *array;
    if(paramCount < 1 || varlist[0]->vt != VT_PTR || !(array = (Varlist *)varlist[0]->ptrVal) || array->magic != varlist_magic)
    {
        goto previous_error;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)(List_GotoPrevious(array->list));

    return S_OK;
previous_error:
    printf("Function requires 1 array handle: %s(array)\n", "previous");
    (*pretvar) = NULL;
    return E_FAIL;
}

//islast(array)
HRESULT openbor_islast(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    Varlist *array;
    if(paramCount < 1 || varlist[0]->vt != VT_PTR || !(array = (Varlist *)varlist[0]->ptrVal) || array->magic != varlist_magic)
    {
        goto islast_error;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)(  List_Retrieve(array->list) == List_GetLast(array->list) );

    return S_OK;
islast_error:
    printf("Function requires 1 array handle: %s(array)\n", "islast");
    (*pretvar) = NULL;
    return E_FAIL;
}

//isfirst(array)
HRESULT openbor_isfirst(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    Varlist *array;
    if(paramCount < 1 || varlist[0]->vt != VT_PTR || !(array = (Varlist *)varlist[0]->ptrVal) || array->magic != varlist_magic)
    {
        goto isfirst_error;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)(  List_Retrieve(array->list) == List_GetFirst(array->list) );

    return S_OK;
isfirst_error:
    printf("Function requires 1 array handle: %s(array)\n", "isfirst");
    (*pretvar) = NULL;
    return E_FAIL;
}

//key(array)
HRESULT openbor_key(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    char *name;
    Varlist *array;
    if(paramCount < 1 || varlist[0]->vt != VT_PTR || !(array = (Varlist *)varlist[0]->ptrVal) || array->magic != varlist_magic)
    {
        goto key_error;
    }

    name = List_GetName(array->list);
    if(name)
    {
        ScriptVariant_ChangeType(*pretvar, VT_STR);
        (*pretvar)->strVal = StrCache_CreateNewFrom(name);
    }
    else
    {
        ScriptVariant_Clear(*pretvar);
    }

    return S_OK;
key_error:
    printf("Function requires 1 array handle: %s(array)\n", "key");
    (*pretvar) = NULL;
    return E_FAIL;
}

//value(array)
HRESULT openbor_value(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    ScriptVariant *var;
    Varlist *array;
    if(paramCount < 1 || varlist[0]->vt != VT_PTR || !(array = (Varlist *)varlist[0]->ptrVal) || array->magic != varlist_magic)
    {
        goto value_error;
    }

    var = List_Retrieve(array->list);
    if(var)
    {
        ScriptVariant_Copy(*pretvar, var);
    }
    else
    {
        ScriptVariant_Clear(*pretvar);
    }

    return S_OK;
value_error:
    printf("Function requires 1 array handle: %s(array)\n", "value");
    (*pretvar) = NULL;
    return E_FAIL;
}

//allocscreen(int w, int h);
HRESULT openbor_allocscreen(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG w, h;
    s_screen *screen;

    if(paramCount < 2)
    {
        goto allocscreen_error;
    }

    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &w)))
    {
        goto allocscreen_error;
    }
    if(FAILED(ScriptVariant_IntegerValue(varlist[1], &h)))
    {
        goto allocscreen_error;
    }


    ScriptVariant_ChangeType(*pretvar, VT_PTR);
    screen = allocscreen((int)w, (int)h, PIXEL_32);
    if(screen)
    {
        clearscreen(screen);
    }
    (*pretvar)->ptrVal = (VOID *)screen;

    if((*pretvar)->ptrVal == NULL)
    {
        printf("Not enough memory: allocscreen(%d, %d)\n", (int)w, (int)h);
        (*pretvar) = NULL;
        return E_FAIL;
    }
    List_InsertAfter(&scriptheap, (void *)((*pretvar)->ptrVal), "openbor_allocscreen");
    return S_OK;

allocscreen_error:
    printf("Function requires 2 int values: allocscreen(int width, int height)\n");
    (*pretvar) = NULL;
    return E_FAIL;
}

//clearscreen(s_screen* screen)
HRESULT openbor_clearscreen(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    s_screen *screen;

    *pretvar = NULL;
    if(paramCount != 1)
    {
        goto clearscreen_error;
    }
    if(varlist[0]->vt != VT_PTR)
    {
        goto clearscreen_error;
    }

    screen = (s_screen *)varlist[0]->ptrVal;

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

int mapstrings_drawmethodproperty(ScriptVariant **varlist, int paramCount)
{
    char *propname = NULL;
    int prop;

    static const char *proplist[] =
    {
        "alpha",
        "amplitude",
        "beginsize",
        "centerx",
        "centery",
        "channelb",
        "channelg",
        "channelr",
        "clip",
        "cliph",
        "clipw",
        "clipx",
        "clipy",
        "enabled",
        "endsize",
        "fillcolor",
        "flag",
        "fliprotate",
        "flipx",
        "flipy",
        "perspective",
        "remap",
        "reset",
        "rotate",
        "scalex",
        "scaley",
        "shiftx",
        "table",
        "tintcolor",
        "tintmode",
        "transbg",
        "watermode",
        "wavelength",
        "wavespeed",
        "wavetime",
        "xrepeat",
        "xspan",
        "yrepeat",
        "yspan",
    };


    if(paramCount < 2)
    {
        return 1;
    }
    MAPSTRINGS(varlist[1], proplist, _dm_the_end,
               "Property name '%s' is not supported by drawmethod.\n");

    return 1;
}

// changedrawmethod(entity, propertyname, value);
HRESULT openbor_changedrawmethod(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    entity *e;
    LONG temp = 0;
    DOUBLE ftemp = 0;
    s_drawmethod *pmethod;
    *pretvar = NULL;

    if(paramCount < 3)
    {
        goto changedm_error;
    }

    mapstrings_drawmethodproperty(varlist, paramCount);

    if(varlist[0]->vt == VT_EMPTY)
    {
        e = NULL;
    }
    else if(varlist[0]->vt == VT_PTR)
    {
        e = (entity *)varlist[0]->ptrVal;
    }
    else
    {
        goto changedm_error;
    }

    if(e)
    {
        pmethod = &(e->drawmethod);
    }
    else
    {
        pmethod = &(drawmethod);
    }

    switch(varlist[1]->lVal)
    {

    case _dm_alpha:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->alpha = (int)temp;
        break;
    case _dm_amplitude:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->water.amplitude = (int)temp;
        break;
    case _dm_beginsize:
        if(FAILED(ScriptVariant_DecimalValue(varlist[2], &ftemp)))
        {
            return E_FAIL;
        }
        pmethod->water.beginsize = (float)ftemp;
        break;
    case _dm_centerx:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->centerx = (int)temp;
        break;
    case _dm_centery:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->centery = (int)temp;
        break;
    case _dm_channelb:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->channelb = (int)temp;
        break;
    case _dm_channelg:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->channelg = (int)temp;
        break;
    case _dm_channelr:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->channelr = (int)temp;
        break;
    case _dm_clip:
        if(paramCount < 6)
        {
            return E_FAIL;
        }
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->clipx = (int)temp;
        if(FAILED(ScriptVariant_IntegerValue(varlist[3], &temp)))
        {
            return E_FAIL;
        }
        pmethod->clipy = (int)temp;
        if(FAILED(ScriptVariant_IntegerValue(varlist[4], &temp)))
        {
            return E_FAIL;
        }
        pmethod->clipw = (int)temp;
        if(FAILED(ScriptVariant_IntegerValue(varlist[5], &temp)))
        {
            return E_FAIL;
        }
        pmethod->cliph = (int)temp;
        break;
    case _dm_clipx:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->clipx = (int)temp;
        break;
    case _dm_clipy:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->clipy = (int)temp;
        break;
    case _dm_clipw:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->clipw = (int)temp;
        break;
    case _dm_cliph:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->cliph = (int)temp;
        break;
    case _dm_enabled:
    case _dm_flag:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->flag = (int)temp;
        break;
    case _dm_endsize:
        if(FAILED(ScriptVariant_DecimalValue(varlist[2], &ftemp)))
        {
            return E_FAIL;
        }
        pmethod->water.endsize = (float)ftemp;
        break;
    case _dm_fillcolor:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->fillcolor = (int)temp;
        break;
    case _dm_fliprotate:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->fliprotate = (int)temp;
        break;
    case _dm_flipx:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->flipx = (int)temp;
        break;
    case _dm_flipy:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->flipy = (int)temp;
        break;
    case _dm_perspective:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->water.perspective = (int)temp;
        break;
    case _dm_remap:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->remap = (int)temp;
        break;
    case _dm_reset:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        if(temp)
        {
            *pmethod = plainmethod;
        }
        break;
    case _dm_rotate:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->rotate = (float)temp;
        break;
    case _dm_scalex:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->scalex = (int)temp;
        break;
    case _dm_scaley:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->scaley = (int)temp;
        break;
    case _dm_shiftx:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->shiftx = (int)temp;
        break;
    case _dm_table:
        if(varlist[2]->vt != VT_PTR && varlist[2]->vt != VT_EMPTY )
        {
            return E_FAIL;
        }
        pmethod->table = (void *)varlist[2]->ptrVal;
        break;
    case _dm_tintmode:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->tintmode = (int)temp;
        break;
    case _dm_tintcolor:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->tintcolor = (int)temp;
        break;
    case _dm_transbg:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->transbg = (int)temp;
        break;
    case _dm_watermode:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->water.watermode = (int)temp;
        break;
    case _dm_wavelength:
        if(FAILED(ScriptVariant_DecimalValue(varlist[2], &ftemp)))
        {
            return E_FAIL;
        }
        pmethod->water.wavelength = (float)ftemp;
        break;
    case _dm_wavespeed:
        if(FAILED(ScriptVariant_DecimalValue(varlist[2], &ftemp)))
        {
            return E_FAIL;
        }
        pmethod->water.wavespeed = (float)ftemp;
        break;
    case _dm_wavetime:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->water.wavetime = (int)(temp * pmethod->water.wavespeed);
        break;
    case _dm_xrepeat:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->xrepeat = (int)temp;
        break;
    case _dm_yrepeat:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->yrepeat = (int)temp;
        break;
    case _dm_xspan:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->xspan = (int)temp;
        break;
    case _dm_yspan:
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &temp)))
        {
            return E_FAIL;
        }
        pmethod->yspan = (int)temp;
        break;
    default:
        break;

    }

    return S_OK;

changedm_error:
    printf("Function changedrawmethod must have at least 3 parameters: entity, propertyname, value\n");
    return E_FAIL;
}

//getdrawmethod(<entity>, <property>)
HRESULT openbor_getdrawmethod(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    /*
    getdrawmethod
    Damon V. Caskey
    2013-11-09

    Allow module author to read current drawmethod settings.
    */
    entity *e;
    s_drawmethod *pmethod;

    if(paramCount < 2)
    {
        goto getdm_error;
    }

    ScriptVariant_Clear(*pretvar);
    mapstrings_drawmethodproperty(varlist, paramCount);

    if(varlist[0]->vt == VT_EMPTY)
    {
        e = NULL;
    }
    else if(varlist[0]->vt == VT_PTR)
    {
        e = (entity *)varlist[0]->ptrVal;
    }
    else
    {
        goto getdm_error;
    }

    if(e)
    {
        pmethod = &(e->drawmethod);
    }
    else
    {
        pmethod = &(drawmethod);
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    switch(varlist[1]->lVal)
    {
        case _dm_alpha:
            (*pretvar)->lVal = (int)pmethod->alpha;
            break;
        case _dm_amplitude:
            (*pretvar)->lVal = (int)pmethod->water.amplitude;
            break;
        case _dm_beginsize:
            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (float)pmethod->water.beginsize;
            break;
        case _dm_centerx:
            (*pretvar)->lVal = (int)pmethod->centerx;
            break;
        case _dm_centery:
            (*pretvar)->lVal = (int)pmethod->centery;
            break;
        case _dm_channelb:
            (*pretvar)->lVal = (int)pmethod->channelb;
            break;
        case _dm_channelg:
            (*pretvar)->lVal = (int)pmethod->channelg;
            break;
        case _dm_channelr:
            (*pretvar)->lVal = (int)pmethod->channelr;
            break;
        case _dm_clipx:
            (*pretvar)->lVal = (int)pmethod->clipx;
            break;
        case _dm_clipy:
            (*pretvar)->lVal = (int)pmethod->clipy;
            break;
        case _dm_clipw:
            (*pretvar)->lVal = (int)pmethod->clipw;
            break;
        case _dm_cliph:
            (*pretvar)->lVal = (int)pmethod->cliph;
            break;
        case _dm_endsize:
            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (float)pmethod->water.endsize;
            break;
        case _dm_fillcolor:
            (*pretvar)->lVal = (int)pmethod->fillcolor;
            break;
        case _dm_fliprotate:
            (*pretvar)->lVal = (int)pmethod->fliprotate;
            break;
        case _dm_flipx:
            (*pretvar)->lVal = (int)pmethod->flipx;
            break;
        case _dm_flipy:
            (*pretvar)->lVal = (int)pmethod->flipy;
            break;
        case _dm_perspective:
            (*pretvar)->lVal = (int)pmethod->water.perspective;
            break;
        case _dm_remap:
            (*pretvar)->lVal = (int)pmethod->remap;
            break;
        case _dm_rotate:
            (*pretvar)->lVal = (int)pmethod->rotate;
            break;
        case _dm_scalex:
            (*pretvar)->lVal = (int)pmethod->scalex;
            break;
        case _dm_scaley:
            (*pretvar)->lVal = (int)pmethod->scaley;
            break;
        case _dm_shiftx:
            (*pretvar)->lVal = (int)pmethod->shiftx;
            break;
        case _dm_table:
            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (void *)pmethod->table;
            break;
        case _dm_tintmode:
            (*pretvar)->lVal = (int)pmethod->tintmode;
            break;
        case _dm_tintcolor:
            (*pretvar)->lVal = (int)pmethod->tintcolor;
            break;
        case _dm_transbg:
            (*pretvar)->lVal = (int)pmethod->transbg;
            break;
        case _dm_watermode:
            (*pretvar)->lVal = (int)pmethod->water.watermode;
            break;
        case _dm_wavelength:
            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (float)pmethod->water.wavelength;
            break;
        case _dm_wavespeed:
            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (float)pmethod->water.wavespeed;
            break;
        case _dm_wavetime:
            (*pretvar)->lVal = (int)pmethod->water.wavetime;
            break;
        case _dm_xrepeat:
            (*pretvar)->lVal = (int)pmethod->xrepeat;
            break;
        case _dm_yrepeat:
            (*pretvar)->lVal = (int)pmethod->yrepeat;
            break;
        case _dm_xspan:
            (*pretvar)->lVal = (int)pmethod->xspan;
            break;
        case _dm_yspan:
            (*pretvar)->lVal = pmethod->yspan;
            break;
        default:
        case _dm_enabled:
        case _dm_flag:
            (*pretvar)->lVal = (int)pmethod->flag;
            break;
    }

    return S_OK;

getdm_error:
    printf("Function getdrawmethod must have at least 2 parameters: entity, propertyname\n");
    *pretvar = NULL;
    return E_FAIL;
}

//deprecated
//setdrawmethod(entity, int flag, int scalex, int scaley, int flipx, int flipy, int shiftx, int alpha, int remap, int fillcolor, int rotate, int fliprotate, int transparencybg, void* colourmap, int centerx, int centery);
HRESULT openbor_setdrawmethod(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG value[14];
    entity *e;
    s_drawmethod *pmethod;
    int i;

    *pretvar = NULL;
    if(paramCount < 2)
    {
        goto setdrawmethod_error;
    }

    if(varlist[0]->vt == VT_EMPTY)
    {
        e = NULL;
    }
    else if(varlist[0]->vt == VT_PTR)
    {
        e = (entity *)varlist[0]->ptrVal;
    }
    else
    {
        goto setdrawmethod_error;
    }

    if(e)
    {
        pmethod = &(e->drawmethod);
    }
    else
    {
        pmethod = &(drawmethod);
    }

    memset(value, 0, sizeof(value));
    for(i = 1; i < paramCount && i < 13; i++)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value + i - 1)))
        {
            goto setdrawmethod_error;
        }
    }

    if(paramCount >= 14 && varlist[13]->vt != VT_PTR && varlist[13]->vt != VT_EMPTY)
    {
        goto setdrawmethod_error;
    }

    for(i = 14; i < paramCount && i < 16; i++)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value + i - 2)))
        {
            goto setdrawmethod_error;
        }
    }

    pmethod->flag = (int)value[0];
    pmethod->scalex = (int)value[1];
    pmethod->scaley = (int)value[2];
    pmethod->flipx = (int)value[3];
    pmethod->flipy = (int)value[4];
    pmethod->shiftx = (int)value[5];
    pmethod->alpha = (int)value[6];
    pmethod->remap = (int)value[7];
    pmethod->fillcolor = (int)value[8];
    pmethod->rotate = ((int)value[9]) % 360;
    pmethod->fliprotate = (int)value[10];
    pmethod->transbg = (int)value[11];
    if(paramCount >= 14)
    {
        pmethod->table = (unsigned char *)varlist[13]->ptrVal;
    }
    pmethod->centerx = (int)value[12];
    pmethod->centery = (int)value[13];

    if(pmethod->rotate < 0)
    {
        pmethod->rotate += 360;
    }
    return S_OK;

setdrawmethod_error:
    printf("Function need a valid entity handle and at least 1 interger parameter, setdrawmethod(entity, int flag, int scalex, int scaley, int flipx, int flipy, int shiftx, int alpha, int remap, int fillcolor, int rotate, int fliprotate, int transparencybg, void* colourmap, centerx, centery)\n");
    return E_FAIL;
}

//updateframe(entity, int frame);
HRESULT openbor_updateframe(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG f;
    entity *e;
    void update_frame(entity * ent, int f);

    *pretvar = NULL;
    if(paramCount < 2)
    {
        goto updateframe_error;
    }

    if(varlist[0]->vt == VT_EMPTY)
    {
        e = NULL;
    }
    else if(varlist[0]->vt == VT_PTR)
    {
        e = (entity *)varlist[0]->ptrVal;
    }
    else
    {
        goto updateframe_error;
    }

    if(!e)
    {
        goto updateframe_error;
    }

    if(FAILED(ScriptVariant_IntegerValue(varlist[1], &f)))
    {
        goto updateframe_error;
    }

    update_frame(e, (int)f);

    return S_OK;

updateframe_error:
    printf("Function need a valid entity handle and at an interger parameter: updateframe(entity, int frame)\n");
    return E_FAIL;
}

//executeanimation(entity, int anim, int resetable);
HRESULT openbor_executeanimation(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG anim, resetable = 0;
    entity *e;

    *pretvar = NULL;
    if(paramCount < 1)
    {
        goto executeanimation_error;
    }

    if(varlist[0]->vt == VT_EMPTY)
    {
        e = NULL;
    }
    else if(varlist[0]->vt == VT_PTR)
    {
        e = (entity *)varlist[0]->ptrVal;
    }
    else
    {
        goto executeanimation_error;
    }

    if(!e)
    {
        goto executeanimation_error;
    }

    e->takeaction = common_animation_normal;
    e->attacking = ATTACKING_INACTIVE;
    e->idling = IDLING_INACTIVE;
    e->drop = 0;
    e->falling = 0;
    e->inpain = 0;
    e->rising = 0;
    e->riseattacking = 0;
    e->edge = EDGE_NO;
    e->ducking = DUCK_INACTIVE;
    e->inbackpain = 0;
    e->blocking = 0;

    if(paramCount == 1)
    {
        return S_OK;
    }

    if(paramCount > 1 && FAILED(ScriptVariant_IntegerValue(varlist[1], &anim)))
    {
        goto executeanimation_error;
    }
    if(paramCount > 2 && FAILED(ScriptVariant_IntegerValue(varlist[2], &resetable)))
    {
        goto executeanimation_error;
    }
    ent_set_anim(e, (int)anim, (int)resetable);

    return S_OK;

executeanimation_error:
    printf("Function need a valid entity handle, the other 2 integer parameters are optional: performattack(entity, int anim, int resetable)\n");
    return E_FAIL;
}

//performattack(entity, int anim, int resetable);
HRESULT openbor_performattack(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG anim, resetable = 0;
    entity *e;

    *pretvar = NULL;
    if(paramCount < 1)
    {
        goto performattack_error;
    }

    if(varlist[0]->vt == VT_EMPTY)
    {
        e = NULL;
    }
    else if(varlist[0]->vt == VT_PTR)
    {
        e = (entity *)varlist[0]->ptrVal;
    }
    else
    {
        goto performattack_error;
    }

    if(!e)
    {
        goto performattack_error;
    }

    e->takeaction = common_attack_proc;
    e->attacking = ATTACKING_ACTIVE;
    e->idling = IDLING_INACTIVE;
    e->drop = 0;
    e->falling = 0;
    e->inpain = 0;
    e->rising = 0;
    e->riseattacking = 0;
    e->edge = EDGE_NO;
    e->inbackpain = 0;
    e->blocking = 0;

    if(paramCount == 1)
    {
        return S_OK;
    }

    if(paramCount > 1 && FAILED(ScriptVariant_IntegerValue(varlist[1], &anim)))
    {
        goto performattack_error;
    }
    if(paramCount > 2 && FAILED(ScriptVariant_IntegerValue(varlist[2], &resetable)))
    {
        goto performattack_error;
    }
    ent_set_anim(e, (int)anim, (int)resetable);

    return S_OK;

performattack_error:
    printf("Function need a valid entity handle, the other 2 integer parameters are optional: performattack(entity, int anim, int resetable)\n");
    return E_FAIL;
}

//setidle(entity, int anim, int resetable, int stalladd);
HRESULT openbor_setidle(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG anim = 0, resetable = 0, stalladd = 0;
    entity *e;
    extern unsigned int _time;

    *pretvar = NULL;
    if(paramCount < 1)
    {
        goto setidle_error;
    }

    if(varlist[0]->vt == VT_EMPTY)
    {
        e = NULL;
    }
    else if(varlist[0]->vt == VT_PTR)
    {
        e = (entity *)varlist[0]->ptrVal;
    }
    else
    {
        goto setidle_error;
    }

    if(!e)
    {
        goto setidle_error;
    }

    e->takeaction = NULL;
    e->attacking = ATTACKING_INACTIVE;
    e->idling = 1;
    e->drop = 0;
    e->falling = 0;
    e->inpain = 0;
    e->rising = 0;
    e->riseattacking = 0;
    e->edge = EDGE_NO;
    e->ducking = DUCK_INACTIVE;
    e->inbackpain = 0;
    e->blocking = 0;
    e->nograb = e->nograb_default; //e->nograb = 0;
    e->destx = e->position.x;
    e->destz = e->position.z;

    if(paramCount == 1)
    {
        return S_OK;
    }

    if(paramCount > 1 && FAILED(ScriptVariant_IntegerValue(varlist[1], &anim)))
    {
        goto setidle_error;
    }
    if(paramCount > 2 && FAILED(ScriptVariant_IntegerValue(varlist[2], &resetable)))
    {
        goto setidle_error;
    }
    if(paramCount > 3 && FAILED(ScriptVariant_IntegerValue(varlist[3], &stalladd)))
    {
        goto setidle_error;
    }
    ent_set_anim(e, (int)anim, (int)resetable);

    if(stalladd > 0)
    {
        e->stalltime = _time + stalladd;
    }

    return S_OK;

setidle_error:
    printf("Function need a valid entity handle, the other 3 integer parameters are optional: setidle(entity, int anim, int resetable, int stalladd)\n");
    return E_FAIL;
}

//getentity(int index_from_list)
HRESULT openbor_getentity(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ind;
    extern entity **ent_list;
    extern int ent_list_size;

    if(paramCount != 1)
    {
        goto getentity_error;
    }

    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
    {
        goto getentity_error;
    }

    ScriptVariant_Clear(*pretvar);

    if((int)ind < ent_list_size && (int)ind >= 0)
    {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = (VOID *)ent_list[(int)ind];
    }
    //else, it should return an empty value
    return S_OK;

getentity_error:
    printf("Function need an integer parameter: getentity(int index_in_list)\n");
    *pretvar = NULL;
    return E_FAIL;
}


//loadmodel(name)
HRESULT openbor_loadmodel(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG unload = 0;
    s_model *model;
    if(paramCount < 1)
    {
        goto loadmodel_error;
    }
    if(varlist[0]->vt != VT_STR)
    {
        goto loadmodel_error;
    }

    ScriptVariant_ChangeType(*pretvar, VT_PTR);
    if(paramCount >= 2)
        if(FAILED(ScriptVariant_IntegerValue(varlist[1], &unload)))
        {
            goto loadmodel_error;
        }

    model = load_cached_model(StrCache_Get(varlist[0]->strVal), "openbor_loadmodel", (char)unload);

    if(paramCount >= 3 && model)
    {
        model_cache[model->index].selectable = (char)ScriptVariant_IsTrue(varlist[2]);
    }

    (*pretvar)->ptrVal = (VOID *)model;

    //else, it should return an empty value
    return S_OK;

loadmodel_error:
    printf("Function needs a string and integer parameters: loadmodel(name, unload, selectable)\n");
    ScriptVariant_Clear(*pretvar);
    *pretvar = NULL;
    return E_FAIL;
}

// load a sprite which doesn't belong to the sprite_cache
// loadsprite(path, maskpath)
HRESULT openbor_loadsprite(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    extern s_sprite *loadsprite2(char * filename, int * width, int * height);
    s_sprite *spr, *mask;
    if(paramCount < 1)
    {
        goto loadsprite_error;
    }

    if(varlist[0]->vt != VT_STR)
    {
        goto loadsprite_error;
    }

    ScriptVariant_ChangeType(*pretvar, VT_PTR);
    if((spr = loadsprite2(StrCache_Get(varlist[0]->strVal), NULL, NULL)))
    {
        (*pretvar)->ptrVal = (VOID *)spr;
        if(paramCount > 1 && (mask = loadsprite2(StrCache_Get(varlist[1]->strVal), NULL, NULL)))
        {
            spr->mask = mask;
        }
        List_InsertAfter(&scriptheap, (void *)spr, "openbor_loadsprite");
    }
    //else, it should return an empty value
    return S_OK;

loadsprite_error:
    printf("Function need a string parameter: loadsprite(path)\n");
    ScriptVariant_Clear(*pretvar);
    *pretvar = NULL;
    return E_FAIL;
}

// Call options menu, blocked
HRESULT openbor_menu_options(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    //void menu_options();

    menu_options();

    *pretvar = NULL;
    return S_OK;
}

HRESULT openbor_hallfame(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    hallfame(0);

    *pretvar = NULL;
    return S_OK;
}

//playwebm(path, int noskip)
HRESULT openbor_playwebm(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG temp = 0; //noskip
    extern int playwebm(char * filename, int noskip); // avoid implicit declaration

    if(paramCount < 1)
    {
        goto playwebm_error;
    }

    if(varlist[0]->vt != VT_STR)
    {
        goto playwebm_error;
    }

    if(paramCount > 1)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[1], &temp)))
        {
            goto playwebm_error;
        }
    }

    #ifdef WEBM
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)playwebm(StrCache_Get(varlist[0]->strVal), (int)temp);
    #else
        printf("Skipping video %s; WebM playback not supported on this platform\n");
        *pretvar = NULL;
    #endif

    return S_OK;

playwebm_error:
    printf("Function need a string parameter, other parameters are optional: playwebm(path, int noskip)\n");
    *pretvar = NULL;
    return E_FAIL;
}

//playgif(path, int x, int y, int noskip)
HRESULT openbor_playgif(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG temp[3] = {0, 0, 0}; //x,y,noskip
    int i;
    extern unsigned char pal[1024];
    extern int playgif(char * filename, int x, int y, int noskip);

    if(paramCount < 1)
    {
        goto playgif_error;
    }

    if(varlist[0]->vt != VT_STR)
    {
        goto playgif_error;
    }

    for(i = 0; i < 3 && i < paramCount - 1; i++)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[i + 1], temp + i)))
        {
            goto playgif_error;
        }
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)playgif(StrCache_Get(varlist[0]->strVal), (int)(temp[0]), (int)(temp[1]), (int)(temp[2]));
    palette_set_corrected(pal, savedata.gamma, savedata.gamma, savedata.gamma, savedata.brightness, savedata.brightness, savedata.brightness);
    return S_OK;

playgif_error:
    printf("Function need a string parameter, other parameters are optional: playgif(path, int x, int y, int noskip)\n");
    *pretvar = NULL;
    return E_FAIL;
}

//open and return a handle
//TODO: error messages
HRESULT openbor_openanigif(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    anigif_info *info = NULL;

    if(varlist[0]->vt != VT_STR)
    {
        goto openanigif_error;
    }

    info = calloc(1, sizeof(*info));
    if(anigif_open(StrCache_Get(varlist[0]->strVal), packfile, info))
    {
        info->magic = anigif_magic;
        List_InsertAfter(&scriptheap, (void *)info, "openbor_openanigif");
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = (VOID *)info;
        return S_OK;
    }

openanigif_error:
    if(info)
    {
        free(info);
    }
    *pretvar = NULL;
    return E_FAIL;
}

//decode a frame if any
HRESULT openbor_decodeanigif(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    anigif_info *info = NULL;

    if(varlist[0]->vt != VT_PTR || !varlist[0]->ptrVal)
    {
        goto decodeanigif_error;
    }
    info = (anigif_info *) varlist[0]->ptrVal;

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)anigif_decode_frame(info);
    return S_OK;

decodeanigif_error:
    *pretvar = NULL;
    return E_FAIL;
}

//TODO mapstrings
HRESULT openbor_getanigifinfo(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    anigif_info *info = NULL;
    char *name;

    if(varlist[0]->vt != VT_PTR || !varlist[0]->ptrVal)
    {
        goto getanigifinfo_error;
    }
    info = (anigif_info *) varlist[0]->ptrVal;

    if(varlist[1]->vt != VT_STR)
    {
        goto getanigifinfo_error;
    }
    name = StrCache_Get(varlist[1]->strVal);
    if(0 == stricmp(name, "buffer"))
    {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = (VOID *)anigif_getbuffer(info);
    }
    else if(0 == stricmp(name, "done"))
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)info->done;
    }
    else if(0 == stricmp(name, "frame"))
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)info->frame;
    }
    else if(0 == stricmp(name, "isRGB"))
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)info->isRGB;
    }
    else if(0 == stricmp(name, "width"))
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)(info->gifbuffer[0] ? info->gifbuffer[0]->width : 0);
    }
    else if(0 == stricmp(name, "height"))
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)(info->gifbuffer[0] ? info->gifbuffer[0]->height : 0);
    }
    else if(0 == stricmp(name, "nextframe"))
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)info->info[0].nextframe;
    }
    else if(0 == stricmp(name, "lastdelay"))
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)info->info[0].lastdelay;
    }
    else
    {
        goto getanigifinfo_error;
    }

    return S_OK;

getanigifinfo_error:
    *pretvar = NULL;
    return E_FAIL;
}

// complex, so make a function for ai
// adjustwalkanimation(ent, target);
HRESULT openbor_adjustwalkanimation(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{

    entity *e, *t, *temp;

    *pretvar = NULL;

    if(paramCount < 1)
    {
        e = self;
    }
    else if(varlist[0]->vt == VT_PTR)
    {
        e = (entity *)varlist[0]->ptrVal;
    }
    else
    {
        goto adjustwalkanimation_error;
    }

    if(paramCount < 2)
    {
        t = NULL;
    }
    else if(varlist[1]->vt == VT_PTR)
    {
        t = (entity *)varlist[1]->ptrVal;
    }
    else if(varlist[1]->vt == VT_EMPTY)
    {
        t = NULL;
    }
    else
    {
        goto adjustwalkanimation_error;
    }

    temp = self;

    self = e;
    adjust_walk_animation(t);
    self = temp;

    return S_OK;
adjustwalkanimation_error:
    printf("Function adjustwalkanimation(entity, target), both parameters are optional, but must be valid.");
    return E_FAIL;
}

//finditem(entity)
HRESULT openbor_finditem(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{

    entity *e, *t, *temp;

    if(paramCount < 1)
    {
        e = self;
    }
    else if(varlist[0]->vt == VT_PTR)
    {
        e = (entity *)varlist[0]->ptrVal;
    }
    else
    {
        goto finditem_error;
    }

    temp = self;

    self = e;
    t = normal_find_item();
    self = temp;

    ScriptVariant_ChangeType(*pretvar, VT_PTR);
    (*pretvar)->ptrVal = (VOID *)t;

    return S_OK;
finditem_error:

    *pretvar = NULL;
    printf("Function finditem(entity), entity is optional, but must be valid.");
    return E_FAIL;
}

//pickup(entity, item)
HRESULT openbor_pickup(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{

    entity *e, *t, *temp;

    *pretvar = NULL;

    if(paramCount < 2)
    {
        goto pickup_error;
    }

    if(varlist[0]->vt == VT_PTR)
    {
        e = (entity *)varlist[0]->ptrVal;
    }
    else
    {
        goto pickup_error;
    }

    if(varlist[1]->vt == VT_PTR)
    {
        t = (entity *)varlist[1]->ptrVal;
    }
    else
    {
        goto pickup_error;
    }

    if(!e || !t)
    {
        goto pickup_error;
    }

    temp = self;

    self = e;
    common_pickupitem(t);
    self = temp;

    return S_OK;
pickup_error:
    printf("Function pickup(entity, item), handles must be valid.");
    return E_FAIL;
}

//waypoints(ent, x1, z1, x2, z2, x3, z3, ...)
//zero length list means clear waypoints
HRESULT openbor_waypoints(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    int num, i;
    s_axis_principal_float *wp = NULL;
    DOUBLE x, z;

    entity *e;
    *pretvar = NULL;

    if(paramCount < 1)
    {
        goto wp_error;
    }

    if(varlist[0]->vt == VT_PTR)
    {
        e = (entity *)varlist[0]->ptrVal;
    }
    else
    {
        goto wp_error;
    }

    num = (paramCount - 1) / 2;
    if(num > 0)
    {
        //append
        wp = malloc(sizeof(*wp) * (num + e->numwaypoints));

        for(i = 0; i < num ; i++)
        {
            if(FAILED(ScriptVariant_DecimalValue(varlist[1], &x)))
            {
                goto wp_error;
            }

            if(FAILED(ScriptVariant_DecimalValue(varlist[2], &z)))
            {
                goto wp_error;
            }

            wp[num - i - 1].x = (float)x;
            wp[num - i - 1].z = (float)z;
        }
        if(e->numwaypoints)
        {
            for(i = 0; i < e->numwaypoints; i++)
            {
                wp[i + num] = e->waypoints[i];
            }
        }

        if(e->waypoints)
        {
            free(e->waypoints);
        }
        e->waypoints = wp;
        e->numwaypoints = num;
    }
    else
    {
        e->numwaypoints = 0;
        if(e->waypoints)
        {
            free(e->waypoints);
        }
        e->waypoints = NULL;
    }
    return S_OK;

wp_error:
    if(wp)
    {
        free(wp);
    }
    wp = NULL;
    printf("Function waypoints requires a valid entity handle and a list of x, z value pairs.");
    return E_FAIL;
}

//testmove(entity, x, z)
HRESULT openbor_testmove(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{

    entity *e;
    DOUBLE x, z;

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    if(paramCount < 3)
    {
        goto testmove_error;
    }

    if(varlist[0]->vt == VT_PTR)
    {
        e = (entity *)varlist[0]->ptrVal;
    }
    else
    {
        goto testmove_error;
    }

    if(FAILED(ScriptVariant_DecimalValue(varlist[1], &x)))
    {
        goto testmove_error;
    }

    if(FAILED(ScriptVariant_DecimalValue(varlist[2], &z)))
    {
        goto testmove_error;
    }

    if(!e)
    {
        goto testmove_error;
    }

    (*pretvar)->lVal = (LONG) testmove(e, e->position.x, e->position.z, x, z);

    return S_OK;
testmove_error:
    *pretvar = NULL;
    printf("Function testmove(entity, x, z)");
    return E_FAIL;
}

//spriteq_draw(vscreen, 0, MIN_INT, MAX_INT, dx, dy)
HRESULT openbor_drawspriteq(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{

    LONG value[5] = {0, MIN_INT, MAX_INT, 0, 0};
    int i;
    s_screen *screen;
    extern s_screen *vscreen;

    *pretvar = NULL;

    if(paramCount < 1)
    {
        goto drawsq_error;
    }

    if(varlist[0]->vt != VT_PTR && varlist[0]->vt != VT_EMPTY)
    {
        goto drawsq_error;
    }

    if(varlist[0]->ptrVal)
    {
        screen = (s_screen *)varlist[0]->ptrVal;
    }
    else
    {
        screen = vscreen;
    }

    for(i = 1; i < paramCount && i <= 5; i++)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value + i - 1)))
        {
            goto drawsq_error;
        }
    }

    spriteq_draw(screen, (int)value[0], (int)value[1], (int)value[2], (int)value[3], (int)value[4]);

    return S_OK;

drawsq_error:
    printf("Function drawspriteq needs a valid screen handle and all other paramaters must be integers.");
    return E_FAIL;

}

HRESULT openbor_clearspriteq(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    *pretvar = NULL;
    spriteq_clear();
    return S_OK;

}

// ===== gfxproperty ======
enum gfxproperty_enum
{
    _gfx_centerx,
    _gfx_centery,
    _gfx_height,
    _gfx_palette,
    _gfx_pixel,
    _gfx_pixelformat,
    _gfx_srcheight,
    _gfx_srcwidth,
    _gfx_width,
    _gfx_the_end,
};

int mapstrings_gfxproperty(ScriptVariant **varlist, int paramCount)
{
    char *propname = NULL;
    int prop;

    static const char *proplist[] =
    {
        "centerx",
        "centery",
        "height",
        "palette",
        "pixel",
        "pixelformat",
        "srcheight",
        "srcwidth",
        "width",
    };


    if(paramCount < 2)
    {
        return 1;
    }
    MAPSTRINGS(varlist[1], proplist, _gfx_the_end,
               "Property name '%s' is not supported by gfxproperty.\n");

    return 1;
}

// getgfxproperty(handle, propertyname, ...);
HRESULT openbor_getgfxproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    s_screen *screen;
    s_sprite *sprite;
    s_bitmap *bitmap;
    LONG value[2] = {0, 0}, v;
    void *handle;
    int i, x, y;

    if(paramCount < 2)
    {
        goto ggp_error;
    }

    mapstrings_gfxproperty(varlist, paramCount);

    if(varlist[0]->vt != VT_PTR)
    {
        goto ggp_error;
    }

    handle = varlist[0]->ptrVal;

    if(!handle)
    {
        goto ggp_error;
    }

    screen = (s_screen *)handle;
    sprite = (s_sprite *)handle;
    bitmap = (s_bitmap *)handle;

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);


    switch(varlist[1]->lVal)
    {
    case _gfx_width:
        switch(screen->magic)
        {
        case screen_magic:
            (*pretvar)->lVal = screen->width;
            break;
        case sprite_magic:
            (*pretvar)->lVal = sprite->width;
            break;
        case bitmap_magic:
            (*pretvar)->lVal = bitmap->width;
            break;
        default:
            goto ggp_error2;
        }
        break;
    case _gfx_height:
        switch(screen->magic)
        {
        case screen_magic:
            (*pretvar)->lVal = screen->height;
            break;
        case sprite_magic:
            (*pretvar)->lVal = sprite->height;
            break;
        case bitmap_magic:
            (*pretvar)->lVal = bitmap->height;
            break;
        default:
            goto ggp_error2;
        }
        break;
    case _gfx_srcwidth:
        switch(screen->magic)
        {
        case sprite_magic:
            (*pretvar)->lVal = sprite->srcwidth;
            break;
        case screen_magic:
            (*pretvar)->lVal = screen->width;
            break;
        case bitmap_magic:
            (*pretvar)->lVal = bitmap->width;
            break;
        default:
            goto ggp_error2;
        }
        break;
    case _gfx_srcheight:
        switch(screen->magic)
        {
        case sprite_magic:
            (*pretvar)->lVal = sprite->srcheight;
            break;
        case screen_magic:
            (*pretvar)->lVal = screen->height;
            break;
        case bitmap_magic:
            (*pretvar)->lVal = bitmap->height;
            break;
        default:
            goto ggp_error2;
        }
        break;
    case _gfx_centerx:
        switch(screen->magic)
        {
        case screen_magic:
        case bitmap_magic:
            (*pretvar)->lVal = 0;
            break;
        case sprite_magic:
            (*pretvar)->lVal = sprite->centerx;
            break;
        default:
            goto ggp_error2;
        }
        break;
    case _gfx_centery:
        switch(screen->magic)
        {
        case screen_magic:
        case bitmap_magic:
            (*pretvar)->lVal = 0;
            break;
        case sprite_magic:
            (*pretvar)->lVal = sprite->centery;
            break;
        default:
            goto ggp_error2;
        }
        break;
    case _gfx_palette:
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        switch(screen->magic)
        {
        case screen_magic:
            (*pretvar)->ptrVal = (VOID *)screen->palette;
            break;
        case sprite_magic:
            (*pretvar)->ptrVal = (VOID *)sprite->palette;
            break;
        case bitmap_magic:
            (*pretvar)->ptrVal = (VOID *)bitmap->palette;
            break;
        default:
            goto ggp_error2;
        }
        break;
    case _gfx_pixelformat:
        switch(screen->magic)
        {
        case screen_magic:
            (*pretvar)->lVal = screen->pixelformat;
            break;
        case sprite_magic:
            (*pretvar)->lVal = sprite->pixelformat;
            break;
        case bitmap_magic:
            (*pretvar)->lVal = bitmap->pixelformat;
            break;
        default:
            goto ggp_error2;
        }
        break;
    case _gfx_pixel:
        if(paramCount < 4)
        {
            goto ggp_error3;
        }
        for(i = 2; i < 4; i++)
        {
            if(FAILED(ScriptVariant_IntegerValue(varlist[i], value + i - 2)))
            {
                goto ggp_error4;
            }
        }
        x = value[0];
        y = value[1];
        switch(screen->magic)
        {
        case bitmap_magic: //As long as the two structures are identical...
        case screen_magic:
            if(x < 0 || x >= screen->width || y < 0 || y >= screen->height)
            {
                v = 0;
            }
            else
            {
                switch(screen->pixelformat)
                {
                case PIXEL_8:
                case PIXEL_x8:
                    v = (LONG)(((unsigned char *)screen->data)[y * screen->width + x]);
                    break;
                case PIXEL_16:
                    v = (LONG)(((unsigned short *)screen->data)[y * screen->width + x]);
                    break;
                case PIXEL_32:
                    v = (LONG)(((unsigned *)screen->data)[y * screen->width + x]);
                    break;
                default:
                    v = 0;
                }
            }
            break;
        case sprite_magic:
            if(x < 0 || x >= sprite->width || y < 0 || y >= sprite->height)
            {
                v = 0;
            }
            else
            {
                v = (LONG)sprite_get_pixel(sprite, x, y);
            }
            break;
        default:
            goto ggp_error2;
        }
        (*pretvar)->lVal = v;
        break;
    default:
        break;

    }

    return S_OK;

ggp_error:
    printf("Function getgfxproperty must have a valid handle and a property name.\n");
    *pretvar = NULL;
    return E_FAIL;
ggp_error2:
    printf("Function getgfxproperty encountered an invalid handle.\n");
    *pretvar = NULL;
    return E_FAIL;
ggp_error3:
    printf("You need to specify x, y value for getgfxproperty.\n");
    *pretvar = NULL;
    return E_FAIL;
ggp_error4:
    printf("Invalid x or y value for getgfxproperty.\n");
    *pretvar = NULL;
    return E_FAIL;
}

//allocscript(name, comment);
HRESULT openbor_allocscript(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    Script *ns;
    char *name = NULL, *comment = NULL;

    ns = malloc(sizeof(Script));

    if(ns == NULL)
    {
        goto as_error;
    }

    if(paramCount >= 1 && varlist[0]->vt == VT_STR)
    {
        name = (char *)StrCache_Get(varlist[0]->strVal);
    }
    if(paramCount >= 2 && varlist[1]->vt == VT_STR)
    {
        comment = (char *)StrCache_Get(varlist[1]->strVal);
    }

    Script_Init(ns, name, comment, 1);

    List_InsertAfter(&scriptheap, (void *)ns, "openbor_allocscript");

    ScriptVariant_ChangeType(*pretvar, VT_PTR);
    (*pretvar)->ptrVal = (VOID *)ns;
    return S_OK;

as_error:
    printf("Function allocscript failed to alloc enough memory.\n");
    (*pretvar) = NULL;
    return E_FAIL;
}

//loadscript(handle, path);
HRESULT openbor_loadscript(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    Script *ns = NULL;
    char *path = NULL;
    int load_script(Script * script, char * file);

    (*pretvar) = NULL;

    if(paramCount >= 1 && varlist[0]->vt == VT_PTR)
    {
        ns = (Script *)varlist[0]->ptrVal;
    }
    if(ns == NULL || ns->magic != script_magic)
    {
        goto ls_error;
    }
    if(paramCount >= 2 && varlist[1]->vt == VT_STR)
    {
        path = (char *)StrCache_Get(varlist[1]->strVal);
    }
    if(path == NULL)
    {
        goto ls_error;
    }

    load_script(ns, path);
    //Script_Init(ns, name, comment, 1);
    //if(!load_script(ns, path)) goto ls_error2;

    return S_OK;

ls_error:
    printf("Function loadscript requires a valid script handle and a path.\n");
    return E_FAIL;
}

//compilescript(handle);
HRESULT openbor_compilescript(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    Script *ns = NULL;

    (*pretvar) = NULL;

    if(paramCount >= 1 && varlist[0]->vt == VT_PTR)
    {
        ns = (Script *)varlist[0]->ptrVal;
    }
    if(ns == NULL || ns->magic != script_magic)
    {
        goto cs_error;
    }

    Script_Compile(ns);

    return S_OK;

cs_error:
    printf("Function compilescript requires a valid script handle.\n");
    return E_FAIL;
}

//executescript(handle);
HRESULT openbor_executescript(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    Script *ns = NULL;

    (*pretvar) = NULL;

    if(paramCount >= 1 && varlist[0]->vt == VT_PTR)
    {
        ns = (Script *)varlist[0]->ptrVal;
    }
    if(ns == NULL || ns->magic != script_magic)
    {
        goto cs_error;
    }

    Script_Execute(ns);

    return S_OK;

cs_error:
    printf("Function executescript requires a valid script handle.\n");
    return E_FAIL;
}


//loadgamefile() //only reload saved level file from saves
HRESULT openbor_loadgamefile(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    loadGameFile();
    *pretvar = NULL;
    return S_OK;
}

//finishlevel()
HRESULT openbor_finishlevel(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    *pretvar = NULL;
    level->force_finishlevel = 1;
    return S_OK;
}

//gameover()
HRESULT openbor_gameover(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    *pretvar = NULL;
    level->force_gameover = 1;
    return S_OK;
}

//gotomainmenu(flag); flag: 1 = no gameover, 2 = no halloffame, 4 = no intro.txt; you can use a combination like 1+2 = 3 or all 1+2+4 = 7
HRESULT openbor_gotomainmenu(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ltemp = 0;

    *pretvar = NULL;

    if(paramCount >= 1 && FAILED(ScriptVariant_IntegerValue(varlist[0], &ltemp)) )
    {
        return E_FAIL;
    }

    goto_mainmenu((LONG)ltemp);

    return S_OK;
}

//playgame(set, usesave?)
HRESULT openbor_playgame(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG lset = 0, lsave = -1;
    *pretvar = NULL;

    if(paramCount >= 1 && FAILED(ScriptVariant_IntegerValue(varlist[0], &lset)) )
    {
        goto pg_error;
    }
    if(paramCount >= 2 && FAILED(ScriptVariant_IntegerValue(varlist[1], &lsave)) )
    {
        goto pg_error;
    }


    useSave = lsave;
    useSet = lset;
    endgame = 1;

    return S_OK;

pg_error:
    *pretvar = NULL;
    return E_FAIL;
}

//getrecordingstatus() it returns 0 = stop, 1 = rec, 2 = play, 4 = free buffer
HRESULT openbor_getrecordingstatus(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount) {
    ScriptVariant_Clear(*pretvar);
    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)playrecstatus->status;
    return S_OK;
}

//recordinputs(value) -> 0 = stop, 1 = rec, 2 = play, 4 = free buffer
HRESULT openbor_recordinputs(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    int ltemp;

    *pretvar = NULL;
    if( paramCount < 1 && varlist[0]->vt != VT_INTEGER )
    {
        return E_FAIL;
    }

    ltemp = (int)varlist[0]->lVal;
    switch(ltemp)
    {
        case A_REC_STOP:
            stopRecordInputs();
            break;
        case A_REC_REC:
            if( paramCount < 3 || varlist[1]->vt != VT_STR || varlist[2]->vt != VT_STR )
            {
                printf("Function recordinputs requires a pathname and filename parameters.\n");
                return E_FAIL;
            }
            strcpy(playrecstatus->path,(char*)StrCache_Get(varlist[1]->strVal));
            strcpy(playrecstatus->filename,(char*)StrCache_Get(varlist[2]->strVal));
            //debug_printf("%s/%s",(char*)StrCache_Get(varlist[1]->strVal),(char*)StrCache_Get(varlist[2]->strVal));
            stopRecordInputs();
            playrecstatus->status = A_REC_REC;
            recordInputs();
            break;
        case A_REC_PLAY:
            if( paramCount < 3 || varlist[1]->vt != VT_STR || varlist[2]->vt != VT_STR )
            {
                printf("Function recordinputs requires a pathname and filename parameters.\n");
                return E_FAIL;
            }
            strcpy(playrecstatus->path,(char*)StrCache_Get(varlist[1]->strVal));
            strcpy(playrecstatus->filename,(char*)StrCache_Get(varlist[2]->strVal));
            stopRecordInputs();
            playrecstatus->status = A_REC_PLAY;
            playRecordedInputs();
            break;
        case A_REC_FREE:
            freeRecordedInputs();
            break;
    }

    return S_OK;
}

// getsaveinfo(set, prop);
HRESULT openbor_getsaveinfo(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ltemp;
    s_savelevel *slot;
    char *prop;
    if(paramCount < 2)
    {
        goto gsi_error;
    }

    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ltemp)) || varlist[1]->vt != VT_STR)
    {
        goto gsi_error;
    }

    if(!savelevel)
    {
        ScriptVariant_Clear(*pretvar);
        return S_OK;
    }

    slot = savelevel + ltemp;
    prop = (char *)StrCache_Get(varlist[1]->strVal);

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    if(0 == stricmp(prop, "flag"))
    {
        (*pretvar)->lVal = (LONG)slot->flag;
    }
    else if(0 == stricmp(prop, "level"))
    {
        (*pretvar)->lVal = (LONG)slot->level;
    }
    else if(0 == stricmp(prop, "stage"))
    {
        (*pretvar)->lVal = (LONG)slot->stage;
    }
    else if(0 == stricmp(prop, "set"))
    {
        (*pretvar)->lVal = (LONG)slot->which_set;
    }
    else if(0 == stricmp(prop, "times_completed"))
    {
        (*pretvar)->lVal = (LONG)slot->times_completed;
    }
    else if(0 == stricmp(prop, "score"))
    {
        if(paramCount < 3 || FAILED(ScriptVariant_IntegerValue(varlist[2], &ltemp)) )
        {
            goto gsi_error;
        }
        (*pretvar)->lVal = (LONG)slot->pScores[ltemp];
    }
    else if(0 == stricmp(prop, "lives"))
    {
        if(paramCount < 3 || FAILED(ScriptVariant_IntegerValue(varlist[2], &ltemp)) )
        {
            goto gsi_error;
        }
        (*pretvar)->lVal = (LONG)slot->pLives[ltemp];
    }
    else if(0 == stricmp(prop, "credits"))
    {
        if(paramCount < 3 || FAILED(ScriptVariant_IntegerValue(varlist[2], &ltemp)) )
        {
            goto gsi_error;
        }
        (*pretvar)->lVal = (LONG)(noshare ? slot->credits : slot->pCredits[ltemp]);
    }
    else if(0 == stricmp(prop, "name"))
    {
        ScriptVariant_ChangeType(*pretvar, VT_STR);
        (*pretvar)->strVal = StrCache_CreateNewFrom(slot->dName);
    }
    else if(0 == stricmp(prop, "playername"))
    {
        if(paramCount < 3 || FAILED(ScriptVariant_IntegerValue(varlist[2], &ltemp)) )
        {
            goto gsi_error;
        }
        ScriptVariant_ChangeType(*pretvar, VT_STR);
        (*pretvar)->strVal = StrCache_CreateNewFrom(slot->pName[ltemp]);
    }
    else if(0 == stricmp(prop, "health"))
    {
        if(paramCount < 3 || FAILED(ScriptVariant_IntegerValue(varlist[2], &ltemp)) )
        {
            goto gsi_error;
        }
        (*pretvar)->lVal = (LONG)slot->pSpawnhealth[ltemp];
    }
    else if(0 == stricmp(prop, "mp"))
    {
        if(paramCount < 3 || FAILED(ScriptVariant_IntegerValue(varlist[2], &ltemp)) )
        {
            goto gsi_error;
        }
        (*pretvar)->lVal = (LONG)slot->pSpawnmp[ltemp];
    }
    else
    {
        goto gsi_error;
    }
    return S_OK;

gsi_error:
    *pretvar = NULL;
    return E_FAIL;
}

