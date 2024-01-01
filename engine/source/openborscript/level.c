/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c)  OpenBOR Team
 */

// Level Properties
// 2017-04-26
// Caskey, Damon V.
//
// Access to level and level set properties.

#include "scriptcommon.h"

#define invalid_pointer_input arg_value->vt != VT_PTR && arg_value->vt != VT_EMPTY

// Set Handle
// Caskey, Damon V.
// 2017-04-26
//
// Get the handle (pointer) of a level set.
//
// get_set_handle(int index)
HRESULT openbor_get_set_handle(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "get_set_handle(int index)"
    #define ARG_MINIMUM     1   // Minimum required arguments.
    #define ARG_INDEX       0   // Array index.

    int                 result;
    LONG                index;
    extern s_set_entry *levelsets;
    extern int          num_difficulties;

    if(paramCount != ARG_MINIMUM)
    {
        goto error_local;
    }

    if(FAILED(ScriptVariant_IntegerValue(varlist[ARG_INDEX], &index)))
    {
        goto error_local;
    }

    ScriptVariant_Clear(*pretvar);

    // Verify the index is within bounds, and
    // if so, dereference the array element pointer.
    if(index < num_difficulties && index >= 0)
    {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = &levelsets[index];
    }

    result = S_OK;

    return result;

    // Error trapping.
    error_local:

    printf("You must provide a valid index: " SELF_NAME "\n");

    *pretvar    = NULL;
    result      = E_FAIL;
    return result;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_INDEX
}

// Set specific properties.
// Caskey, Damon V.
// 2017-04-25
//
// Access set property by handle.
//
// get_set_property(void handle, int frame, int property)
HRESULT openbor_get_set_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "get_set_property(void handle, int property)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_OBJECT      0   // Handle (pointer to property structure).
    #define ARG_PROPERTY    1   // Property to access.


    int                     result      = S_OK; // Success or error?
    s_set_entry             *handle     = NULL; // Property handle.
    e_set_properties        property    = 0;    // Property argument.

    // Clear pass by reference argument used to send
    // property data back to calling script.     .
    ScriptVariant_Clear(*pretvar);

    // Verify incoming arguments. There should at least
    // be a pointer for the property handle and an integer
    // to determine which property is accessed.
    if(paramCount < ARG_MINIMUM
       || varlist[ARG_OBJECT]->vt != VT_PTR
       || varlist[ARG_PROPERTY]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }
    else
    {
        handle      = (s_set_entry *)varlist[ARG_OBJECT]->ptrVal;
        property    = (LONG)varlist[ARG_PROPERTY]->lVal;
    }

    // Which property to get?
    switch(property)
    {
        case SET_PROPERTY_COMPLETE_FLAG:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->ifcomplete;
            break;

        case SET_PROPERTY_COMPLETE_SKIP:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->noshowcomplete;

        case SET_PROPERTY_CONTINUE_SCORE_TYPE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->continuescore;

        case SET_PROPERTY_CREDITS:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->credits;
            break;

        case SET_PROPERTY_GAME_OVER_SKIP:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->noshowgameover;
            break;

        case SET_PROPERTY_HOF_DISABLE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->noshowhof;
            break;

        case SET_PROPERTY_LEVELSET_COLLECTION:

            // Verify the handle is populated.
            if(handle->levelorder)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->levelorder;
            }

            break;

        case SET_PROPERTY_LEVELSET_COUNT:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->numlevels;
            break;

        case SET_PROPERTY_LIVES:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->lives;
            break;

        case SET_PROPERTY_MP_RECOVER_TYPE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->typemp;
            break;

        case SET_PROPERTY_MUSIC_FADE_TIME:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->custfade;
            break;

        case SET_PROPERTY_MUSIC_OVERLAP:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->musicoverlap;
            break;

        case SET_PROPERTY_NAME:

            ScriptVariant_ChangeType(*pretvar, VT_STR);
            (*pretvar)->strVal = StrCache_CreateNewFrom(handle->name);
            break;

        case SET_PROPERTY_PLAYER_MAX:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->maxplayers;
            break;

        case SET_PROPERTY_SAVE_TYPE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->saveflag;
            break;

        case SET_PROPERTY_SELECT_DISABLE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->noselect;
            break;

        case SET_PROPERTY_SELECT_NO_SAME:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->nosame;
            break;

        default:

            printf("Unsupported property.\n");
            goto error_local;
            break;
    }

    return result;

    // Error trapping.
    error_local:

    printf("You must provide a valid handle and property: " SELF_NAME "\n");

    result = E_FAIL;
    return result;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_OBJECT
    #undef ARG_PROPERTY
}

// Set specific properties.
// Caskey, Damon V.
// 2016-10-20
//
// Access set property by handle (pointer).
//
// set_set_property(void handle, int property, value)
HRESULT openbor_set_set_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME           "set_set_property(void handle, int property, value)"
    #define ARG_MINIMUM         3   // Minimum required arguments.
    #define ARG_OBJECT          0   // Handle (pointer to property structure).
    #define ARG_PROPERTY        1   // Property to access.
    #define ARG_VALUE           2   // New value to apply.

    int                     result      = S_OK; // Success or error?
    s_set_entry             *handle     = NULL; // Property handle.
    e_set_properties        property    = 0;    // Property to access.

    // Value carriers to apply on properties after
    // taken from argument.
    //LONG     temp_int;
    //DOUBLE  temp_float;

    // Verify incoming arguments. There must be a
    // pointer for the animation handle, an integer
    // property, and a new value to apply.
    if(paramCount < ARG_MINIMUM
       || varlist[ARG_OBJECT]->vt != VT_PTR
       || varlist[ARG_PROPERTY]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }
    else
    {
        handle      = (s_set_entry *)varlist[ARG_OBJECT]->ptrVal;
        property    = (LONG)varlist[ARG_PROPERTY]->lVal;
    }

    // Which property to modify?
    switch(property)
    {
        case SET_PROPERTY_LEVELSET_COLLECTION:

            handle->levelorder = (s_level_entry *)varlist[ARG_VALUE]->ptrVal;

            break;

        default:

            printf("Unsupported or read only property.\n");
            goto error_local;

            break;
    }

    return result;

    // Error trapping.
    error_local:

    printf("You must provide a valid handle, property and value: " SELF_NAME "\n");

    result = E_FAIL;
    return result;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_OBJECT
    #undef ARG_PROPERTY
    #undef ARG_VALUE
}

// Level specific properties.
// Caskey, Damon V.
// 2017-04-25
//
// Access level property by handle.
//
// get_level_property(void handle, int property)
HRESULT openbor_get_level_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME       "get_level_property(void handle, int property)"
    #define ARG_MINIMUM     2   // Minimum required arguments.
    #define ARG_OBJECT      0   // Handle (pointer to property structure).
    #define ARG_PROPERTY    1   // Property to access.


    int                     result      = S_OK; // Success or error?
    s_level                 *handle     = NULL; // Property handle.
    e_level_properties      property    = 0;    // Property argument.

    // Clear pass by reference argument used to send
    // property data back to calling script.     .
    ScriptVariant_Clear(*pretvar);

    // Verify incoming arguments. There should at least
    // be a pointer for the property handle and an integer
    // to determine which property is accessed.
    if(paramCount < ARG_MINIMUM
       || varlist[ARG_PROPERTY]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }

    // If the handle argument is null, and it almost
    // certainly will be, use current level.
    // At the time this function was written (2017-04-27),
    // there was no such thing as a level collection.
    // OpenBOR loads one level at a time and unloads when
    // completed to conserve memory. The handle argument
    // is in place for future compatibility in case this
    // ever changes.
    if(varlist[ARG_OBJECT]->vt != VT_PTR)
    {
        handle = level;
    }
    else
    {
        handle = (s_level *)varlist[ARG_OBJECT]->ptrVal;
    }

    // Get the property argument.
    property    = (LONG)varlist[ARG_PROPERTY]->lVal;

    // Which property to get?
    switch(property)
    {

        case LEVEL_PROPERTY_AUTO_SCROLL_DIRECTION:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->bgdir;
            break;

        case LEVEL_PROPERTY_AUTO_SCROLL_X:

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)handle->bgspeed;
            break;

        case LEVEL_PROPERTY_AUTO_SCROLL_Y:

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)handle->vbgspeed;
            break;

        case LEVEL_PROPERTY_BASEMAP_COLLECTION:

            // Verify the handle is populated.
            if(handle->basemaps)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->basemaps;
            }

            break;

        case LEVEL_PROPERTY_BASEMAP_COUNT:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->numbasemaps;
            break;

        case LEVEL_PROPERTY_BOSS_COUNT:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->bossescount;
            break;

        case LEVEL_PROPERTY_BOSS_MUSIC_NAME:

            ScriptVariant_ChangeType(*pretvar, VT_STR);
            (*pretvar)->strVal = StrCache_CreateNewFrom(handle->bossmusic);
            break;

        case LEVEL_PROPERTY_BOSS_MUSIC_OFFSET:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->bossmusic_offset;
            break;

        case LEVEL_PROPERTY_BOSS_SLOW:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->boss_slow;
            break;

        case LEVEL_PROPERTY_CAMERA_OFFSET_X:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->cameraxoffset;
            break;

        case LEVEL_PROPERTY_CAMERA_OFFSET_Z:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->camerazoffset;
            break;

        case LEVEL_PROPERTY_COMPLETE_FORCE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->force_finishlevel;
            break;

        case LEVEL_PROPERTY_GAMEOVER:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->force_gameover;
            break;

        case LEVEL_PROPERTY_DAMAGE_FROM_ENEMY:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->nohurt;
            break;

        case LEVEL_PROPERTY_DAMAGE_FROM_PLAYER:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->nohit;
            break;

        case LEVEL_PROPERTY_FACING:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->facing;
            break;

        case LEVEL_PROPERTY_GRAVITY:

           ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)handle->gravity;
            break;


        case LEVEL_PROPERTY_HOLE_COLLECTION:

            // Verify the handle is populated.
            if(handle->holes)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->holes;
            }

            break;

        case LEVEL_PROPERTY_HOLE_COUNT:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->numholes;
            break;

        case LEVEL_PROPERTY_LAYER_BACKGROUND_DEFAULT_HANDLE:

            // Verify the handle is populated.
            if(handle->background)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->background;
            }

            break;

        case LEVEL_PROPERTY_LAYER_BACKGROUND_COLLECTION:

            // Verify the handle is populated.
            if(handle->bglayers)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->bglayers;
            }

            break;

        case LEVEL_PROPERTY_LAYER_BACKGROUND_COUNT:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->numbglayers;
            break;

        case LEVEL_PROPERTY_LAYER_COLLECTION:

            // Verify animation has item.
            if(handle->layers)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->layers;
            }

            break;

        case LEVEL_PROPERTY_LAYER_COUNT:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->numlayers;
            break;

        case LEVEL_PROPERTY_LAYER_FOREGROUND_COLLECTION:

            // Verify the handle is populated.
            if(handle->fglayers)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->fglayers;
            }

            break;

        case LEVEL_PROPERTY_LAYER_FOREGROUND_COUNT:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->numfglayers;
            break;

        case LEVEL_PROPERTY_LAYER_FRONTPANEL_COLLECTION:

            // Verify the handle is populated.
            if(handle->frontpanels)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->frontpanels;
            }

            break;

        case LEVEL_PROPERTY_LAYER_FRONTPANEL_COUNT:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->numfrontpanels;
            break;

        case LEVEL_PROPERTY_LAYER_GENERIC_COLLECTION:

            // Verify the handle is populated.
            if(handle->genericlayers)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->genericlayers;
            }

            break;

        case LEVEL_PROPERTY_LAYER_GENERIC_COUNT:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->numgenericlayers;
            break;

        case LEVEL_PROPERTY_LAYER_PANEL_COLLECTION:          // s_layer *(*panels)[3]; //normal neon screen

            // Verify the handle is populated.
            if(handle->panels)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->panels;
            }

            break;

        case LEVEL_PROPERTY_LAYER_PANEL_COUNT:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->numpanels;
            break;

        case LEVEL_PROPERTY_LAYER_REF_COLLECTION:

            // Verify the handle is populated.
            if(handle->layersref)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->layersref;
            }

            break;

        case LEVEL_PROPERTY_LAYER_REF_COUNT:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->numlayersref;
            break;

        case LEVEL_PROPERTY_LAYER_WATER_COLLECTION:

            // Verify the handle is populated.
            if(handle->waters)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->waters;
            }

            break;

        case LEVEL_PROPERTY_LAYER_WATER_COUNT:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->numwaters;
            break;

        case LEVEL_PROPERTY_MAX_FALL_VELOCITY:

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)handle->maxfallspeed;
            break;

        case LEVEL_PROPERTY_MAX_TOSS_VELOCITY:

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)handle->maxtossspeed;
            break;

        case LEVEL_PROPERTY_MIRROR:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->mirror;
            break;

        case LEVEL_PROPERTY_NAME:

            ScriptVariant_ChangeType(*pretvar, VT_STR);
            (*pretvar)->strVal = StrCache_CreateNewFrom(handle->name);
            break;

        case LEVEL_PROPERTY_NUM_BOSSES:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->numbosses;
            break;

        case LEVEL_PROPERTY_PALETTE_BLENDING_COLLECTION:

            printf("LEVEL_PROPERTY_PALETTE_BLENDING_COLLECTION no longer supported");
            ScriptVariant_Clear(*pretvar);

            break;

        case LEVEL_PROPERTY_PALETTE_COLLECTION:

            if(handle->palettes)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->palettes;
            }

            break;

        case LEVEL_PROPERTY_PALETTE_COUNT:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->numpalettes;
            break;

        case LEVEL_PROPERTY_POSITION_X:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->pos;
            break;

        case LEVEL_PROPERTY_QUAKE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->quake;
            break;

        case LEVEL_PROPERTY_QUAKE_TIME:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->quaketime;
            break;

        case LEVEL_PROPERTY_ROCKING:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->rocking;
            break;

        case LEVEL_PROPERTY_SCRIPT_LEVEL_END:

            //if(&handle->endlevel_script)
            //{
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = &handle->endlevel_script;
            //}

        case LEVEL_PROPERTY_SCRIPT_LEVEL_START:              // Script level_script;

            //if(&handle->level_script)
            //{
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = &handle->level_script;
            //}

        case LEVEL_PROPERTY_SCRIPT_KEY:

            //if(&handle->key_script)
            //{
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = &handle->key_script;
            //}

        case LEVEL_PROPERTY_SCRIPT_UPDATE:

            //if(&handle->update_script)
            //{
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = &handle->update_script;
            //}

        case LEVEL_PROPERTY_SCRIPT_UPDATED:

            //if(&handle->updated_script)
            //{
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = &handle->updated_script;
            //}

        case LEVEL_PROPERTY_SCROLL_DIRECTION:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->scrolldir;
            break;

        case LEVEL_PROPERTY_SCROLL_VELOCITY:

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->lVal = (DOUBLE)handle->scrollspeed;
            break;

        case LEVEL_PROPERTY_SIZE_X:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->width;
            break;

        case LEVEL_PROPERTY_SPAWN_COLLECTION:

            if(handle->spawnpoints)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->spawnpoints;
            }

        case LEVEL_PROPERTY_SPAWN_COUNT:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->numspawns;
            break;

        case LEVEL_PROPERTY_SPAWN_PLAYER_COLLECTION:

            //if(&handle->spawn)
            //{
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->spawn;
            //}

        case LEVEL_PROPERTY_SPECIAL_DISABLE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->nospecial;
            break;

        case LEVEL_PROPERTY_TEXT_OBJECT_COLLECTION:          // s_textobj *textobjs;

            if(handle->textobjs)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->textobjs;
            }

        case LEVEL_PROPERTY_TEXT_OBJECT_COUNT:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->numtextobjs;
            break;

        case LEVEL_PROPERTY_TIME_ADVANCE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->advancetime;
            break;

        case LEVEL_PROPERTY_TIME_DISPLAY:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->notime;
            break;

        case LEVEL_PROPERTY_TIME_RESET:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->noreset;
            break;

        case LEVEL_PROPERTY_TIME_SET:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->settime;
            break;

        case LEVEL_PROPERTY_TYPE:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->type;
            break;


        case LEVEL_PROPERTY_WAITING:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->waiting;
            break;

        case LEVEL_PROPERTY_WALL_COLLECTION:

            // Verify animation has item.
            if(handle->walls)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->walls;
            }

            break;

        case LEVEL_PROPERTY_WALL_COUNT:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->numwalls;
            break;

        case LEVEL_PROPERTY_WEAPON:

            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)handle->setweap;
            break;

        default:

            printf("Unsupported property.\n");
            goto error_local;
            break;
    }

    return result;

    // Error trapping.
    error_local:

    printf("You must provide a valid handle and property: " SELF_NAME "\n");

    result = E_FAIL;
    return result;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_OBJECT
    #undef ARG_PROPERTY
}

// Level specific properties.
// Caskey, Damon V.
// 2017-04-25
//
// Access level property by handle (pointer).
//
// set_level_property(void handle, int property, value)
HRESULT openbor_set_level_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
    #define SELF_NAME           "set_level_property(void handle, int property, value)"
    #define ARG_MINIMUM         3   // Minimum required arguments.
    #define ARG_OBJECT          0   // Handle (pointer to property structure).
    #define ARG_PROPERTY        1   // Property to access.
    #define ARG_VALUE           2   // New value to apply.

    int                     result;     // Success or error?
    s_level                 *handle;    // Property handle.
    e_level_properties      property;   // Property to access.

    // Value carriers to apply on properties after
    // taken from value argument.
    LONG             temp_int;   // Integer type argument.
    DOUBLE          temp_float; // Float type argument.
    ScriptVariant   *arg_value; // Argument input carrier.

    // Verify incoming arguments. There should at least
    // be a pointer for the property handle, an integer
    // to determine which property is accessed and a
    // new value to apply.
    if(paramCount < ARG_MINIMUM
       || varlist[ARG_OBJECT]->vt != VT_PTR
       || varlist[ARG_PROPERTY]->vt != VT_INTEGER)
    {
        *pretvar = NULL;
        goto error_local;
    }
    else
    {
        handle      = (s_level *)varlist[ARG_OBJECT]->ptrVal;
        property    = (LONG)varlist[ARG_PROPERTY]->lVal;
        arg_value   = varlist[ARG_VALUE];
    }

    // Which property to modify?
    switch(property)
    {
        case LEVEL_PROPERTY_AUTO_SCROLL_DIRECTION:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->bgdir = temp_int;
            break;

        case LEVEL_PROPERTY_AUTO_SCROLL_X:

            if(FAILED(ScriptVariant_DecimalValue(arg_value, &temp_float)))
            {
                goto error_local;
            }

            handle->bgspeed = temp_float;
            break;

        case LEVEL_PROPERTY_AUTO_SCROLL_Y:

            if(FAILED(ScriptVariant_DecimalValue(arg_value, &temp_float)))
            {
                goto error_local;
            }

            handle->vbgspeed = temp_float;
            break;

        case LEVEL_PROPERTY_BASEMAP_COLLECTION:

            if(invalid_pointer_input)
            {
                goto error_local;
            }

            handle->basemaps = (s_basemap *)arg_value->ptrVal;
            break;

        case LEVEL_PROPERTY_BASEMAP_COUNT:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->numbasemaps = temp_int;
            break;

        case LEVEL_PROPERTY_BOSS_COUNT:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->bossescount = temp_int;
            break;

        case LEVEL_PROPERTY_BOSS_MUSIC_NAME:

            if(arg_value->vt != VT_STR)
            {
                goto error_local;
            }
            strcpy(handle->bossmusic, (char *)StrCache_Get(arg_value->strVal));
            break;

        case LEVEL_PROPERTY_BOSS_MUSIC_OFFSET:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->bossmusic_offset = temp_int;
            break;

        case LEVEL_PROPERTY_BOSS_SLOW:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->boss_slow = temp_int;
            break;

        case LEVEL_PROPERTY_CAMERA_OFFSET_X:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->cameraxoffset = temp_int;
            break;

        case LEVEL_PROPERTY_CAMERA_OFFSET_Z:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->camerazoffset = temp_int;
            break;

        case LEVEL_PROPERTY_COMPLETE_FORCE:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->force_finishlevel = temp_int;
            break;

        case LEVEL_PROPERTY_GAMEOVER:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->force_gameover = temp_int;
            break;

        case LEVEL_PROPERTY_DAMAGE_FROM_ENEMY:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->nohurt = temp_int;
            break;

        case LEVEL_PROPERTY_DAMAGE_FROM_PLAYER:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->nohit = temp_int;
            break;

        case LEVEL_PROPERTY_FACING:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->facing = temp_int;
            break;

        case LEVEL_PROPERTY_GRAVITY:

            if(FAILED(ScriptVariant_DecimalValue(arg_value, &temp_float)))
            {
                goto error_local;
            }

            handle->gravity = temp_float;
            break;

        case LEVEL_PROPERTY_HOLE_COLLECTION:

            if(invalid_pointer_input)
            {
                goto error_local;
            }

            handle->holes = (s_terrain *)arg_value->ptrVal;
            break;

        case LEVEL_PROPERTY_HOLE_COUNT:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->numholes = temp_int;
            break;

        case LEVEL_PROPERTY_LAYER_BACKGROUND_DEFAULT_HANDLE:

            if(invalid_pointer_input)
            {
                goto error_local;
            }

            handle->background = (s_layer *)arg_value->ptrVal;
            break;

        case LEVEL_PROPERTY_LAYER_BACKGROUND_COLLECTION:

            if(invalid_pointer_input)
            {
                goto error_local;
            }

            handle->bglayers = (s_layer **)varlist[ARG_VALUE]->ptrVal;

            break;

        case LEVEL_PROPERTY_LAYER_BACKGROUND_COUNT:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->numbglayers = temp_int;
            break;

        case LEVEL_PROPERTY_LAYER_COLLECTION:

            if(invalid_pointer_input)
            {
                goto error_local;
            }

            handle->layers = (s_layer *)arg_value->ptrVal;
            break;

        case LEVEL_PROPERTY_LAYER_COUNT:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->numlayers = temp_int;
            break;

        case LEVEL_PROPERTY_LAYER_FOREGROUND_COLLECTION:

            if(invalid_pointer_input)
            {
                goto error_local;
            }

            handle->fglayers = (s_layer **)arg_value->ptrVal;
            break;

        case LEVEL_PROPERTY_LAYER_FOREGROUND_COUNT:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->numfglayers = temp_int;
            break;

        case LEVEL_PROPERTY_LAYER_FRONTPANEL_COLLECTION:

           if(invalid_pointer_input)
            {
                goto error_local;
            }

            handle->frontpanels = (s_layer **)arg_value->ptrVal;
            break;

        case LEVEL_PROPERTY_LAYER_FRONTPANEL_COUNT:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->numfrontpanels = temp_int;
            break;

        case LEVEL_PROPERTY_LAYER_GENERIC_COLLECTION:

            if(invalid_pointer_input)
            {
                goto error_local;
            }

            handle->genericlayers = (s_layer **)arg_value->ptrVal;
            break;

        case LEVEL_PROPERTY_LAYER_GENERIC_COUNT:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->numgenericlayers = temp_int;
            break;

        case LEVEL_PROPERTY_LAYER_PANEL_COLLECTION:

            if(invalid_pointer_input)
            {
                goto error_local;
            }

            handle->panels = (VOID *)arg_value->ptrVal;
            break;

        case LEVEL_PROPERTY_LAYER_PANEL_COUNT:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->numpanels = temp_int;
            break;

        case LEVEL_PROPERTY_LAYER_REF_COLLECTION:

            if(invalid_pointer_input)
            {
                goto error_local;
            }

            handle->layersref = (s_layer *)arg_value->ptrVal;
            break;

        case LEVEL_PROPERTY_LAYER_REF_COUNT:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->numlayersref = temp_int;
            break;

        case LEVEL_PROPERTY_LAYER_WATER_COLLECTION:

            if(invalid_pointer_input)
            {
                goto error_local;
            }

            handle->waters = (s_layer **)arg_value->ptrVal;
            break;

        case LEVEL_PROPERTY_LAYER_WATER_COUNT:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->numwaters = temp_int;
            break;

        case LEVEL_PROPERTY_MAX_FALL_VELOCITY:

            if(FAILED(ScriptVariant_DecimalValue(arg_value, &temp_float)))
            {
                goto error_local;
            }

            handle->maxfallspeed = temp_float;
            break;

        case LEVEL_PROPERTY_MAX_TOSS_VELOCITY:

            if(FAILED(ScriptVariant_DecimalValue(arg_value, &temp_float)))
            {
                goto error_local;
            }

            handle->maxtossspeed = temp_float;
            break;

        case LEVEL_PROPERTY_MIRROR:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->mirror = temp_int;
            break;

        case LEVEL_PROPERTY_NAME:

            if(arg_value->vt != VT_STR)
            {
                goto error_local;
            }
            strcpy(handle->name, (char *)StrCache_Get(arg_value->strVal));
            break;

        case LEVEL_PROPERTY_NUM_BOSSES:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->numbosses = temp_int;
            break;

        case LEVEL_PROPERTY_PALETTE_BLENDING_COLLECTION:

            // This property is read only.
            goto error_local;
            break;

        case LEVEL_PROPERTY_PALETTE_COLLECTION:

            if(invalid_pointer_input)
            {
                goto error_local;
            }

            handle->palettes = (VOID *)arg_value->ptrVal;
            break;

        case LEVEL_PROPERTY_PALETTE_COUNT:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->numpalettes = temp_int;
            break;

        case LEVEL_PROPERTY_POSITION_X:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->pos = temp_int;
            break;

        case LEVEL_PROPERTY_QUAKE:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->quake = temp_int;
            break;

        case LEVEL_PROPERTY_QUAKE_TIME:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->quaketime = temp_int;
            break;

        case LEVEL_PROPERTY_ROCKING:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->rocking = temp_int;
            break;

        case LEVEL_PROPERTY_SCRIPT_LEVEL_END:

            if(invalid_pointer_input)
            {
                goto error_local;
            }

            handle->endlevel_script = *(Script *)arg_value->ptrVal;
            break;

        case LEVEL_PROPERTY_SCRIPT_LEVEL_START:              // Script level_script;

            if(invalid_pointer_input)
            {
                goto error_local;
            }
            handle->level_script = *(Script *)arg_value->ptrVal;
            break;

        case LEVEL_PROPERTY_SCRIPT_KEY:

            if(invalid_pointer_input)
            {
                goto error_local;
            }

            handle->key_script = *(Script *)arg_value->ptrVal;
            break;

        case LEVEL_PROPERTY_SCRIPT_UPDATE:

            if(invalid_pointer_input)
            {
                goto error_local;
            }

            handle->update_script = *(Script *)arg_value->ptrVal;
            break;

        case LEVEL_PROPERTY_SCRIPT_UPDATED:

            if(invalid_pointer_input)
            {
                goto error_local;
            }

            handle->updated_script = *(Script *)arg_value->ptrVal;
            break;

        case LEVEL_PROPERTY_SCROLL_DIRECTION:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->scrolldir = temp_int;
            break;

        case LEVEL_PROPERTY_SCROLL_VELOCITY:

            if(FAILED(ScriptVariant_DecimalValue(arg_value, &temp_float)))
            {
                goto error_local;
            }

            handle->scrollspeed = temp_float;
            break;

        case LEVEL_PROPERTY_SIZE_X:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->width = temp_int;
            break;

        case LEVEL_PROPERTY_SPAWN_COLLECTION:

            if(invalid_pointer_input)
            {
                goto error_local;
            }

            handle->spawnpoints = (s_spawn_entry *)arg_value->ptrVal;
            break;

        case LEVEL_PROPERTY_SPAWN_COUNT:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->numspawns = temp_int;
            break;

        case LEVEL_PROPERTY_SPAWN_PLAYER_COLLECTION:

            if(invalid_pointer_input)
            {
                goto error_local;
            }

            handle->spawn = (s_axis_principal_float *)arg_value->ptrVal;
            break;

        case LEVEL_PROPERTY_SPECIAL_DISABLE:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->nospecial = temp_int;
            break;

        case LEVEL_PROPERTY_TEXT_OBJECT_COLLECTION:

            if(invalid_pointer_input)
            {
                goto error_local;
            }

            handle->textobjs = (s_textobj *)arg_value->ptrVal;
            break;

        case LEVEL_PROPERTY_TEXT_OBJECT_COUNT:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->numtextobjs = temp_int;
            break;

        case LEVEL_PROPERTY_TIME_ADVANCE:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->advancetime = temp_int;
            break;

        case LEVEL_PROPERTY_TIME_DISPLAY:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->notime = temp_int;
            break;

        case LEVEL_PROPERTY_TIME_RESET:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->noreset = temp_int;
            break;

        case LEVEL_PROPERTY_TIME_SET:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->settime = temp_int;
            break;

        case LEVEL_PROPERTY_TYPE:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->type = temp_int;
            break;


        case LEVEL_PROPERTY_WAITING:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->waiting = temp_int;
            break;

        case LEVEL_PROPERTY_WALL_COLLECTION:

            // Verify animation has item.
            if(handle->walls)
            {
                ScriptVariant_ChangeType(*pretvar, VT_PTR);
                (*pretvar)->ptrVal = (VOID *)handle->walls;
            }

            break;

        case LEVEL_PROPERTY_WALL_COUNT:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->numwalls = temp_int;
            break;

        case LEVEL_PROPERTY_WEAPON:

            if(FAILED(ScriptVariant_IntegerValue(arg_value, &temp_int)))
            {
                goto error_local;
            }

            handle->setweap = temp_int;
            break;


    }


    result = S_OK;

    return result;

    // Error trapping.
    error_local:

    printf("You must provide a valid handle and property: " SELF_NAME "\n");

    result = E_FAIL;
    return result;

    #undef SELF_NAME
    #undef ARG_MINIMUM
    #undef ARG_OBJECT
    #undef ARG_PROPERTY
    #undef ARG_VALUE
}

// ===== Depreciated =====
//
// The following items are depreciated as
// of 2017-04-26 and are included only for
// backward compatibility. Do not add to or
// update.
//
// DC

// ===== level properties ======
enum levelproperty_enum
{
    _lp_basemap,
    _lp_bgspeed,
    _lp_cameraxoffset,
    _lp_camerazoffset,
    _lp_gravity,
    _lp_hole,
    _lp_maxfallspeed,
    _lp_maxtossspeed,
    _lp_quake,
    _lp_rocking,
    _lp_scrollspeed,
    _lp_type,
    _lp_vbgspeed,
    _lp_wall,
    _lp_the_end,
};

enum basemap_enum
{
    _lp_bm_map,
    _lp_bm_x,
    _lp_bm_xsize,
    _lp_bm_z,
    _lp_bm_zsize,
    _lp_bm_the_end,
};

enum terrain_enum
{
    _lp_terrain_depth,
    _lp_terrain_height,
    _lp_terrain_lowerleft,
    _lp_terrain_lowerright,
    _lp_terrain_type,
    _lp_terrain_upperleft,
    _lp_terrain_upperright,
    _lp_terrain_x,
    _lp_terrain_z,
    _lp_terrain_the_end,
};

int mapstrings_levelproperty(ScriptVariant **varlist, int paramCount)
{
    char *propname = NULL;
    int prop;

    static const char *proplist[] =
    {
        "basemap",
        "bgspeed",
        "cameraxoffset",
        "camerazoffset",
        "gravity",
        "hole",
        "maxfallspeed",
        "maxtossspeed",
        "quake",
        "rocking",
        "scrollspeed",
        "type",
        "vbgspeed",
        "wall",
    };

    static const char *basemaplist[] =
    {
        "map",
        "x",
        "xsize",
        "z",
        "zsize",
    };

    static const char *terrainlist[] =
    {
        /*
        Walls and holes.
        */

        "depth",
        "height",
        "lowerleft",
        "lowerright",
        "type",
        "upperleft",
        "upperright",
        "x",
        "z",
    };

    if(paramCount < 1)
    {
        return 1;
    }
    MAPSTRINGS(varlist[0], proplist, _lp_the_end,
               "Level property '%s' is not supported.\n");

    if(paramCount >= 3 && varlist[0]->vt == VT_INTEGER && varlist[0]->lVal == _lp_basemap)
    {
        MAPSTRINGS(varlist[2], basemaplist, _lp_bm_the_end,
                   _is_not_supported_by_, "basemap");
    }

    if(paramCount >= 3 && varlist[0]->vt == VT_INTEGER && (varlist[0]->lVal == _lp_hole || varlist[0]->lVal == _lp_wall))
    {
        MAPSTRINGS(varlist[2], terrainlist, _lp_terrain_the_end,
                   _is_not_supported_by_, "wall/hole");
    }

    return 1;
}

HRESULT openbor_getlevelproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ltemp;
    DOUBLE dbltemp2, dbltemp3;
    mapstrings_levelproperty(varlist, paramCount);

    switch(varlist[0]->lVal)
    {
    case _lp_bgspeed:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)level->bgspeed;
        break;
    }
    case _lp_vbgspeed:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)level->vbgspeed;
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
    case _lp_gravity:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)level->gravity;
        break;
    }
    case _lp_maxfallspeed:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)level->maxfallspeed;
        break;
    }
    case _lp_maxtossspeed:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)level->maxtossspeed;
        break;
    }
    case _lp_quake:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->quake;
        break;
    }
    case _lp_scrollspeed:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)level->scrollspeed;
        break;
    }
    case _lp_type:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->type;
        break;
    }
    case _lp_hole:
    {

        if(paramCount > 2 && SUCCEEDED(ScriptVariant_IntegerValue(varlist[1], &ltemp))
                && ltemp >= 0 && ltemp < level->numholes)
        {

            if(varlist[2]->vt != VT_INTEGER)
            {
                printf("You must provide a string value for hole subproperty.\n");
                goto getlevelproperty_error;
            }

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);

            switch(varlist[2]->lVal)
            {
                case _lp_terrain_height:
                {
                    (*pretvar)->dblVal = level->holes[ltemp].height;
                    break;
                }
                case _lp_terrain_depth:
                {
                    (*pretvar)->dblVal = level->holes[ltemp].depth;
                    break;
                }
                case _lp_terrain_lowerleft:
                {
                    (*pretvar)->dblVal = level->holes[ltemp].lowerleft;
                    break;
                }
                case _lp_terrain_lowerright:
                {
                    (*pretvar)->dblVal = level->holes[ltemp].lowerright;
                    break;
                }
                case _lp_terrain_upperleft:
                {
                    (*pretvar)->dblVal = level->holes[ltemp].upperleft;
                    break;
                }
                case _lp_terrain_upperright:
                {
                    (*pretvar)->dblVal = level->holes[ltemp].upperright;
                    break;
                }
                case _lp_terrain_x:
                {
                    (*pretvar)->dblVal = level->holes[ltemp].x;
                    break;
                }
                case _lp_terrain_z:
                {
                    (*pretvar)->dblVal = level->holes[ltemp].z;
                    break;
                }
                case _lp_terrain_type:
                {
                    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
                    (*pretvar)->lVal = level->holes[ltemp].type;
                    break;
                }
                default:
                {
                    printf("Invalid subproperty for hole.\n");
                    goto getlevelproperty_error;
                }
            }
        }
        else
        {
            printf("Error in wall property.\n");
            goto getlevelproperty_error;
        }
        break;

    }
    case _lp_wall:
    {
        if(paramCount > 2 && SUCCEEDED(ScriptVariant_IntegerValue(varlist[1], &ltemp))
                && ltemp >= 0 && ltemp < level->numwalls)
        {

            if(varlist[2]->vt != VT_INTEGER)
            {
                printf("You must provide a string value for wall subproperty.\n");
                goto getlevelproperty_error;
            }

            ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);

            switch(varlist[2]->lVal)
            {
                case _lp_terrain_depth:
                {
                    (*pretvar)->dblVal = level->walls[ltemp].depth;
                    break;
                }
                case _lp_terrain_height:
                {
                    (*pretvar)->dblVal = level->walls[ltemp].height;
                    break;
                }
                case _lp_terrain_lowerleft:
                {
                    (*pretvar)->dblVal = level->walls[ltemp].lowerleft;
                    break;
                }
                case _lp_terrain_lowerright:
                {
                    (*pretvar)->dblVal = level->walls[ltemp].lowerright;
                    break;
                }
                case _lp_terrain_upperleft:
                {
                    (*pretvar)->dblVal = level->walls[ltemp].upperleft;
                    break;
                }
                case _lp_terrain_upperright:
                {
                    (*pretvar)->dblVal = level->walls[ltemp].upperright;
                    break;
                }
                case _lp_terrain_x:
                {
                    (*pretvar)->dblVal = level->walls[ltemp].x;
                    break;
                }
                case _lp_terrain_z:
                {
                    (*pretvar)->dblVal = level->walls[ltemp].z;
                    break;
                }
                case _lp_terrain_type:
                {
                    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
                    (*pretvar)->lVal = level->walls[ltemp].type;
                    break;
                }
                default:
                {
                    printf("Invalid subproperty for wall.\n");
                    goto getlevelproperty_error;
                }
            }
        }
        else
        {
            printf("Error in wall property.\n");
            goto getlevelproperty_error;
        }
        break;
    }
    case _lp_basemap:
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[1], &ltemp)) && ltemp >= 0 && ltemp < level->numbasemaps)
        {
            if(paramCount >= 3)
            {

                if(varlist[2]->vt != VT_INTEGER)
                {
                    printf("You must provide a string value for basemap subproperty.\n");
                    goto getlevelproperty_error;
                }

                ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);

                switch(varlist[2]->lVal)
                {
                case _lp_bm_x:
                    (*pretvar)->dblVal = level->basemaps[ltemp].position.x;
                    break;
                case _lp_bm_xsize:
                    (*pretvar)->dblVal = level->basemaps[ltemp].size.x;
                    break;
                case _lp_bm_z:
                    (*pretvar)->dblVal = level->basemaps[ltemp].position.z;
                    break;
                case _lp_bm_zsize:
                    (*pretvar)->dblVal = level->basemaps[ltemp].size.z;
                    break;
                case _lp_bm_map:
                    if(paramCount >= 5 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp2)) &&
                            SUCCEEDED(ScriptVariant_DecimalValue(varlist[4], &dbltemp3)) &&
                            dbltemp2 >= 0 && dbltemp2 < level->basemaps[ltemp].size.x && dbltemp3 >= 0 && dbltemp3 < level->basemaps[ltemp].size.z
                      )
                    {
                        if(!level->basemaps[ltemp].map)
                        {
                            (*pretvar)->dblVal = (DOUBLE)0;
                        }
                        else (*pretvar)->dblVal = (DOUBLE)level->basemaps[ltemp].map[(LONG)(dbltemp2 + dbltemp3 * level->basemaps[ltemp].size.x)];
                    }
                    else
                    {
                        goto getlevelproperty_error;
                    }
                    break;
                default:
                    goto getlevelproperty_error;
                }
            }
            else
            {
                goto getlevelproperty_error;
            }
        }
        else
        {
            goto getlevelproperty_error;
        }
        break;
    default:
        printf("Property is not supported by function getlevelproperty yet. %d\n", varlist[0]->lVal);
        goto getlevelproperty_error;
        break;
    }

    return S_OK;

getlevelproperty_error:
    *pretvar = NULL;
    return E_FAIL;
}

//changelevelproperty(name, value)
HRESULT openbor_changelevelproperty(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount)
{
    LONG ltemp, ltemp1;
    DOUBLE dbltemp, dbltemp2, dbltemp3;
    static char buf[64];
    int i;
    ScriptVariant *arg = NULL;

    *pretvar = NULL;

    if(paramCount < 2)
    {
        printf("Function changelevelproperty(prop, value) need at least 2 parameters.\n");
        return E_FAIL;
    }

    mapstrings_levelproperty(varlist, paramCount);

    arg = varlist[1];

    switch(varlist[0]->lVal)
    {
    case _lp_rocking:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            level->rocking = (int)ltemp;
        }
        else
        {
            goto clperror;
        }
        break;
    case _lp_bgspeed:
        if(SUCCEEDED(ScriptVariant_DecimalValue(arg, &dbltemp)))
        {
            level->bgspeed = (float)dbltemp;
        }
        else
        {
            goto clperror;
        }
        break;
    case _lp_vbgspeed:
        if(SUCCEEDED(ScriptVariant_DecimalValue(arg, &dbltemp)))
        {
            level->vbgspeed = (float)dbltemp;
        }
        else
        {
            goto clperror;
        }
        break;
    case _lp_scrollspeed:
        if(SUCCEEDED(ScriptVariant_DecimalValue(arg, &dbltemp)))
        {
            level->scrollspeed = (float)dbltemp;
        }
        else
        {
            goto clperror;
        }
        break;
    case _lp_type:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            level->type = (int)ltemp;
        }
        else
        {
            goto clperror;
        }
        break;
    case _lp_cameraxoffset:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            level->cameraxoffset = (int)ltemp;
        }
        else
        {
            goto clperror;
        }
        break;
    case _lp_camerazoffset:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            level->camerazoffset = (int)ltemp;
        }
        else
        {
            goto clperror;
        }
        break;
    case _lp_gravity:
        if(SUCCEEDED(ScriptVariant_DecimalValue(arg, &dbltemp)))
        {
            level->gravity = (float)dbltemp;
        }
        else
        {
            goto clperror;
        }
        break;
    case _lp_maxfallspeed:
        if(SUCCEEDED(ScriptVariant_DecimalValue(arg, &dbltemp)))
        {
            level->maxfallspeed = (float)dbltemp;
        }
        else
        {
            goto clperror;
        }
        break;
    case _lp_maxtossspeed:
        if(SUCCEEDED(ScriptVariant_DecimalValue(arg, &dbltemp)))
        {
            level->maxtossspeed = (float)dbltemp;
        }
        else
        {
            goto clperror;
        }
        break;
    case _lp_quake:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
        {
            level->quake = (int)ltemp;
        }
        else
        {
            goto clperror;
        }
        break;
    case _lp_basemap:
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[1], &ltemp)) && ltemp >= 0)
        {
            if(ltemp >= level->numbasemaps)
            {
                __reallocto(level->basemaps, level->numbasemaps, ltemp + 1);
                level->numbasemaps = ltemp + 1;
            }
            if(paramCount >= 4)
            {
                switch(varlist[2]->lVal)
                {
                case _lp_bm_x:
                    if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp2)) )
                    {
                        level->basemaps[ltemp].position.x = dbltemp2;
                    }
                    else
                    {
                        goto clperror;
                    }
                    break;
                case _lp_bm_xsize:
                    if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp2)) )
                    {
                        level->basemaps[ltemp].size.x = dbltemp2;
                    }
                    else
                    {
                        goto clperror;
                    }
                    break;
                case _lp_bm_z:
                    if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp2)) )
                    {
                        level->basemaps[ltemp].position.z = dbltemp2;
                    }
                    else
                    {
                        goto clperror;
                    }
                    break;
                case _lp_bm_zsize:
                    if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp2)) )
                    {
                        level->basemaps[ltemp].size.z = dbltemp2;
                    }
                    else
                    {
                        goto clperror;
                    }
                    break;
                case _lp_bm_map:
                    if(paramCount >= 6 && SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp2)) &&
                            SUCCEEDED(ScriptVariant_DecimalValue(varlist[4], &dbltemp3)) &&
                            SUCCEEDED(ScriptVariant_DecimalValue(varlist[5], &dbltemp)) &&
                            dbltemp2 >= 0 && dbltemp2 < level->basemaps[ltemp].size.x && dbltemp3 >= 0 && dbltemp3 < level->basemaps[ltemp].size.z
                      )
                    {
                        if(!level->basemaps[ltemp].map)
                        {
                            level->basemaps[ltemp].map = calloc( 1, (LONG)(sizeof(*(level->basemaps[ltemp].map)) * (level->basemaps[ltemp].size.x+1)*(level->basemaps[ltemp].size.z+1)) );
                        }
                        level->basemaps[ltemp].map[(LONG)(dbltemp2 + dbltemp3 * level->basemaps[ltemp].size.x)] = (float)dbltemp;
                    }
                    else
                    {
                        goto clperror;
                    }
                    break;
                default:
                    goto clperror;
                }
            }
            else
            {
                goto clperror;
            }
        }
        else
        {
            goto clperror;
        }
        break;
    case _lp_hole:
    {
        if( SUCCEEDED(ScriptVariant_IntegerValue(varlist[1], &ltemp)) && ltemp >= 0
           && ( SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)) || SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp1)) ) )
        {
            if(varlist[2]->vt != VT_INTEGER)
            {
                printf("You must provide a string value for hole subproperty.\n");
                goto clperror;
            }

            if(ltemp >= level->numholes)
            {
                __reallocto(level->holes, level->numholes, ltemp + 1);
                level->numholes = ltemp + 1;
            }

            switch(varlist[2]->lVal)
            {
                case _lp_terrain_height:
                {
                    level->holes[ltemp].height = dbltemp;
                    break;
                }
                case _lp_terrain_depth:
                {
                    level->holes[ltemp].depth = dbltemp;
                    break;
                }
                case _lp_terrain_lowerleft:
                {
                    level->holes[ltemp].lowerleft = dbltemp;
                    break;
                }
                case _lp_terrain_lowerright:
                {
                    level->holes[ltemp].lowerright = dbltemp;
                    break;
                }
                case _lp_terrain_upperleft:
                {
                    level->holes[ltemp].upperleft = dbltemp;
                    break;
                }
                case _lp_terrain_upperright:
                {
                    level->holes[ltemp].upperright = dbltemp;
                    break;
                }
                case _lp_terrain_x:
                {
                    level->holes[ltemp].x = dbltemp;
                    break;
                }
                case _lp_terrain_z:
                {
                    level->holes[ltemp].z = dbltemp;
                    break;
                }
                case _lp_terrain_type:
                {
                    level->holes[ltemp].type = ltemp1;
                    break;
                }
                default:
                {
                    printf("Invalid subproperty for hole.\n");
                    goto clperror;
                }
            }
        }
        else
        {
            goto clperror;
        }
        break;

    }
    case _lp_wall:
    {
        if( SUCCEEDED(ScriptVariant_IntegerValue(varlist[1], &ltemp)) && ltemp >= 0
           && ( SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)) || SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp1)) ) )
        {
            if(varlist[2]->vt != VT_INTEGER)
            {
                printf("You must provide a string value for wall subproperty.\n");
                goto clperror;
            }

            if(ltemp >= level->numwalls)
            {
                __reallocto(level->walls, level->numwalls, ltemp + 1);
                level->numwalls = ltemp + 1;
            }

            switch(varlist[2]->lVal)
            {
                case _lp_terrain_depth:
                {
                    level->walls[ltemp].depth = dbltemp;
                    break;
                }
                case _lp_terrain_height:
                {
                    level->walls[ltemp].height = dbltemp;
                    break;
                }
                case _lp_terrain_lowerleft:
                {
                    level->walls[ltemp].lowerleft = dbltemp;
                    break;
                }
                case _lp_terrain_lowerright:
                {
                    level->walls[ltemp].lowerright = dbltemp;
                    break;
                }
                case _lp_terrain_upperleft:
                {
                    level->walls[ltemp].upperleft = dbltemp;
                    break;
                }
                case _lp_terrain_upperright:
                {
                    level->walls[ltemp].upperright = dbltemp;
                    break;
                }
                case _lp_terrain_x:
                {
                    level->walls[ltemp].x = dbltemp;
                    break;
                }
                case _lp_terrain_z:
                {
                    level->walls[ltemp].z = dbltemp;
                    break;
                }
                case _lp_terrain_type:
                {
                    level->walls[ltemp].type = ltemp1;
                    break;
                }
                default:
                {
                    printf("Invalid subproperty for wall.\n");
                    goto clperror;
                }
            }
        }
        else
        {
            goto clperror;
        }
        break;
    }
    default:
        printf("Invalid or read-only level property.\n");
        return E_FAIL;
        break;
    }

    return S_OK;
clperror:
    printf("Function changelevelproperty(prop, value) received invalid value(s). \n");
    printf("Dumping values: ");
    for(i = 1; i < paramCount; i++)
    {
        ScriptVariant_ToString(varlist[i], buf);
        printf("%s, ", buf);
    }
    printf("\n");
    return E_FAIL;
}


