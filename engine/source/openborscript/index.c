/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 - 2017 OpenBOR Team
 */

#include "scriptcommon.h"

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

    // Axis
    else if (functionRef == ((void *)openbor_get_axis_plane_lateral_float_property))
    {
        return "get_axis_plane_lateral_float_property";
    }
    else if (functionRef == ((void *)openbor_get_axis_plane_lateral_int_property))
    {
        return "get_axis_plane_lateral_int_property";
    }
    else if (functionRef == ((void *)openbor_get_axis_plane_vertical_int_property))
    {
        return "get_axis_plane_vertical_int_property";
    }
    else if (functionRef == ((void *)openbor_get_axis_principal_float_property))
    {
        return "get_axis_principal_float_property";
    }
    else if (functionRef == ((void *)openbor_get_axis_principal_int_property))
    {
        return "get_axis_principal_int_property";
    }
    else if (functionRef == ((void *)openbor_set_axis_plane_lateral_float_property))
    {
        return "set_axis_plane_lateral_float_property";
    }
    else if (functionRef == ((void *)openbor_set_axis_plane_lateral_int_property))
    {
        return "set_axis_plane_lateral_int_property";
    }
    else if (functionRef == ((void *)openbor_set_axis_plane_vertical_int_property))
    {
        return "set_axis_plane_vertical_int_property";
    }
    else if (functionRef == ((void *)openbor_set_axis_principal_float_property))
    {
        return "set_axis_principal_float_property";
    }
    else if (functionRef == ((void *)openbor_set_axis_principal_int_property))
    {
        return "set_axis_principal_int_property";
    }

    // Binding
    else if (functionRef == ((void *)openbor_get_binding_property))
    {
        return "get_binding_property";
    }
    else if (functionRef == ((void *)openbor_set_binding_property))
    {
        return "set_binding_property";
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

    else if (functionRef == ((void *)openbor_get_energy_status_property))
    {
        return "get_energy_status_property";
    }
    else if (functionRef == ((void *)openbor_set_energy_status_property))
    {
        return "set_energy_status_property";
    }

    else if (functionRef == ((void *)openbor_get_entity_property))
    {
        return "get_entity_property";
    }
    else if (functionRef == ((void *)openbor_set_entity_property))
    {
        return "set_entity_property";
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

    // Entity collision (ebox)
    else if (functionRef == ((void *)openbor_get_entity_collision_collection))
    {
        return "get_entity_collision_collection";
    }
    else if (functionRef == ((void *)openbor_get_entity_collision_instance))
    {
        return "get_entity_collision_instance";
    }
    else if (functionRef == ((void *)openbor_get_entity_collision_property))
    {
        return "get_entity_collision_property";
    }
    else if (functionRef == ((void *)openbor_set_entity_collision_property))
    {
        return "set_entity_collision_property";
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
    else if (functionRef == ((void *)openbor_getcomputeddamage))
    {
        return "getcomputeddamage";
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
    else if (functionRef == ((void *)openbor_checkbase))
    {
        return "checkbase";
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
    else if (functionRef == ((void *)openbor_gameover))
    {
        return "gameover";
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
    else if (functionRef == ((void *)openbor_get_energy_status_property))
    {
        return (void *)mapstrings_energy_status_property;
    }
    else if (functionRef == ((void *)openbor_set_energy_status_property))
    {
        return (void *)mapstrings_energy_status_property;
    }
    else if (functionRef == ((void *)openbor_get_entity_property))
    {
        return (void *)mapstrings_entity_property;
    }
    else if (functionRef == ((void *)openbor_set_entity_property))
    {
        return (void *)mapstrings_entity_property;
    }
    else if (functionRef == ((void *)openbor_getplayerproperty))
    {
        return (void *)mapstrings_playerproperty;
    }
    else if (functionRef == ((void *)openbor_changeplayerproperty))
    {
        return (void *)mapstrings_playerproperty;
    }

    // Axis
    else if (functionRef == ((void *)openbor_get_axis_plane_lateral_float_property))
    {
        return (void *)mapstrings_axis_plane_lateral_property;
    }
    else if (functionRef == ((void *)openbor_get_axis_plane_lateral_int_property))
    {
        return (void *)mapstrings_axis_plane_lateral_property;
    }
    else if (functionRef == ((void *)openbor_get_axis_plane_vertical_int_property))
    {
        return (void *)mapstrings_axis_plane_vertical_property;
    }
    else if (functionRef == ((void *)openbor_get_axis_principal_float_property))
    {
        return (void *)mapstrings_axis_principal_property;
    }
    else if (functionRef == ((void *)openbor_get_axis_principal_int_property))
    {
        return (void *)mapstrings_axis_principal_property;
    }
    else if (functionRef == ((void *)openbor_set_axis_plane_lateral_float_property))
    {
        return (void *)mapstrings_axis_plane_lateral_property;
    }
    else if (functionRef == ((void *)openbor_set_axis_plane_lateral_int_property))
    {
        return (void *)mapstrings_axis_plane_lateral_property;
    }
    else if (functionRef == ((void *)openbor_set_axis_plane_vertical_int_property))
    {
        return (void *)mapstrings_axis_plane_vertical_property;
    }
    else if (functionRef == ((void *)openbor_set_axis_principal_float_property))
    {
        return (void *)mapstrings_axis_principal_property;
    }
    else if (functionRef == ((void *)openbor_set_axis_principal_int_property))
    {
        return (void *)mapstrings_axis_principal_property;
    }

    // Binding
    else if (functionRef == ((void *)openbor_get_binding_property))
    {
        return (void *)mapstrings_binding;
    }
    else if (functionRef == ((void *)openbor_set_binding_property))
    {
        return (void *)mapstrings_binding;
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


    // Axis
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_get_axis_plane_lateral_float_property, "get_axis_plane_lateral_float_property");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_get_axis_plane_lateral_int_property, "get_axis_plane_lateral_int_property");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_get_axis_plane_vertical_int_property, "get_axis_plane_vertical_int_property");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_get_axis_principal_float_property, "get_axis_principal_float_property");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_get_axis_principal_int_property, "get_axis_principal_int_property");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_set_axis_plane_lateral_float_property, "set_axis_plane_lateral_float_property");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_set_axis_plane_lateral_int_property, "set_axis_plane_lateral_int_property");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_set_axis_plane_vertical_int_property, "set_axis_plane_vertical_int_property");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_set_axis_principal_float_property, "set_axis_principal_float_property");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_set_axis_principal_int_property, "set_axis_principal_int_property");

    // Binding
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_get_binding_property, "get_binding_property");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_set_binding_property, "set_binding_property");

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

    // Entity collision (ebox) properties.
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_get_entity_collision_collection, "get_entity_collision_collection");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_get_entity_collision_instance, "get_entity_collision_instance");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_get_entity_collision_property, "get_entity_collision_property");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_set_entity_collision_property, "set_entity_collision_property");

    // Energy status properties.
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_get_energy_status_property, "get_energy_status_property");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_get_energy_status_property, "set_energy_status_property");

    // Entity properties.
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_get_entity_property, "get_entity_property");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_set_entity_property, "set_entity_property");

    List_InsertAfter(&theFunctionList,
                     (void *)openbor_changeentityproperty, "changeentityproperty");
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
                     (void *)openbor_getcomputeddamage, "getcomputeddamage");
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
                     (void *)openbor_checkbasemapindex, "checkbasemapindex");
    List_InsertAfter(&theFunctionList,
                     (void *)openbor_checkbase, "checkbase");
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
                     (void *)openbor_gameover, "gameover");
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

