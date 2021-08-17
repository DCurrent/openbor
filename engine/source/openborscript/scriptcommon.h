/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 - 2017 OpenBOR Team
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
extern int            *animbackrises;
extern int            *animbackriseattacks;
extern int            *animblkpains;
extern int            *animbackblkpains;

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

#endif // SCRIPT_COMMON_H

