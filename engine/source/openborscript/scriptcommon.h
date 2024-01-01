/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef SCRIPT_COMMON_H
#define SCRIPT_COMMON_H

#include "openbor.h"

#define _is_not_a_known_subproperty_of_  "'%s' is not a known subproperty of '%s'.\n"
#define _is_not_supported_by_ "'%s' is not supported by '%s'.\n"

// Define macro for string mapping.
//
// 1. Is property argument a string?
//      a. True
//          1. Populate 'propname' with string.
//          2. Use searchlist function to populate
//          'prop' with integer constant matching
//          position in propstring list. So if a
//          propname matches the second item in
//          propstring list, then searchlist will
//          return 1.
//
//          c. Is prop a 0+ integer?
//              1. True
//                  a. Use prop to populate varlist->lval.
//                  We now have a property integer we can
//                  compare to enumerated property constant
//                  list and take action as needed by property
//                  access functions.
//
//              2. False
//                  a. Send a failed message to the log.
//                  User most likely made a typo error
//                  and tried to access a property that
//                  does not exist.
//                  b. Return 0.
//
//      b. False.
//          1. Do nothing and allow function to continue.
#define MAPSTRINGS(VAR, LIST, MAXINDEX, FAILMSG, args...) \
{\
    int proplist_cursor; \
    if(VAR->vt == VT_STR) { \
        propname = (char*)StrCache_Get(VAR->strVal); \
        prop = searchList(LIST, propname, MAXINDEX); \
        if(prop >= 0) { \
            ScriptVariant_ChangeType(VAR, VT_INTEGER); \
            VAR->lVal = prop; \
        } else { \
            \
            printf(FAILMSG, propname, ##args);  \
            printf("\n Available properties:\n"); \
            \
            for(proplist_cursor = 0; LIST[proplist_cursor] != NULL; proplist_cursor++){ \
               printf("\n\t%s", LIST[proplist_cursor]); \
            } \
            \
            printf("\n\n"); \
            return 0; \
        }\
    }\
}

extern int			  PLAYER_MIN_Z;
extern int			  PLAYER_MAX_Z;
extern int			  BGHEIGHT;
extern int            MAX_WALL_HEIGHT;

extern s_global_sample global_sample_list;
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
extern int            *animbackrises;
extern int            *animbackriseattacks;
extern int            *animblkpains;
extern int            *animbackblkpains;

extern s_global_config global_config;

extern int            noshare;
extern int            credits;
extern char           musicname[128];
extern float          musicfade[2];
extern int            musicloop;
extern u32            musicoffset;
extern int            models_cached;
extern int            noaircancel; // Kratus (10-2021) Now the "noaircancel" function is accessible by script using "openborvariant"
extern int endgame;
extern int useSave;
extern int useSet;


extern s_sprite_list *sprite_list;
extern s_sprite_map *sprite_map;

extern unsigned char *blendings[MAX_BLENDINGS];
extern int            current_palette;
extern s_attack emptyattack;



/*
* Caskey, Damon V.
* 2023-04-14
*
* Flags to set any special behaviors
* or configuration for property maps.
*/
typedef enum e_property_access_config_flags
{
    PROPERTY_ACCESS_CONFIG_NONE = 0,
    PROPERTY_ACCESS_CONFIG_READ = (1 << 0),
    PROPERTY_ACCESS_CONFIG_WRITE = (1 << 1),
    PROPERTY_ACCESS_CONFIG_STATIC_LENGTH = (1 << 2),
    PROPERTY_ACCESS_CONFIG_STATIC_POINTER = (1 << 3),

    PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT = (PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_WRITE)
}e_property_access_config_flags;

/*
* Caskey, Damon V.
* 2023-04-14
*
* Property access structure. Used to
* build lookup tables of property
* access fields and behaiors.
*/
typedef struct s_property_access_map {
    e_property_access_config_flags config_flags;    // Any special behaviors, ex.: Read only.
    const char* id_string;                          // Text name of property we can output as debug info. Should always be same as the property index constant (ex. MODEL_PROPERTY_ACTION_FREEZE = "MODEL_PROPERTY_ACTION_FREEZE").
    const void* field;                              // Structure member property routes to. Ex: &((s_model*)0)->dofreeze
    VARTYPE type;                                   // Property data type (VT_INTEGER, VT_PTR, etc.).
} s_property_access_map;

/*
* Caskey, Damon V.
* 2023-04-20
*
* So we can reuse property access
* map functions (ex. property dump).
*/
typedef const s_property_access_map(*property_access_map)(const void* acting_object_param, const unsigned int property_index_param);

void property_access_dump_members(property_access_map get_property_map, const int table_count, const void* acting_object);
HRESULT property_access_get_member(const s_property_access_map* property_map, ScriptVariant* pretvar);
HRESULT property_access_set_member(const void* const acting_object, const s_property_access_map* property_map, ScriptVariant* acting_value);

#endif // SCRIPT_COMMON_H

