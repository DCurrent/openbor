#include <string.h>
#include "commands.h"
#include "globals.h"

void freeCommandList(List *list)
{
    List_Clear(list);
    free(list);
}

// attention: modifies usercommand to lowercase
void *getCommandlistCommand(List *list, char *usercommand)
{
    if (!usercommand || !usercommand[0])
    {
        goto fail;
    }
    lc(usercommand, strlen(usercommand));
    Node *n = List_GetNodeByName(list, usercommand);
    if(n)
    {
        return n->value;
    }
fail:
    return NULL;
}

modelCommands getModelCommand(List *list, char *usercommand)
{
    return (modelCommands) getCommandlistCommand(list, usercommand);
}

modelstxtCommands getModelstxtCommand(List *list, char *usercommand)
{
    return (modelstxtCommands) getCommandlistCommand(list, usercommand);
}

levelCommands getLevelCommand(List *list, char *usercommand)
{
    return (levelCommands) getCommandlistCommand(list, usercommand);
}

levelOrderCommands getLevelOrderCommand(List *list, char *usercommand)
{
    return (levelOrderCommands) getCommandlistCommand(list, usercommand);
}

List *prepareList(void)
{
    List *result = malloc(sizeof(List));
    assert(result);
    List_Init(result);
    return result;
}

List *createModelstxtCommandList(void)
{
    List *result = prepareList();
#define LIST_ADD(y,z) List_InsertAfter(result, (void*) y, z)
    LIST_ADD(CMD_MODELSTXT_AJSPECIAL, "ajspecial");
    LIST_ADD(CMD_MODELSTXT_AUTOLAND, "autoland");
    LIST_ADD(CMD_MODELSTXT_BLOCKRATIO, "blockratio");
    LIST_ADD(CMD_MODELSTXT_COLOURSELECT, "colourselect");
    LIST_ADD(CMD_MODELSTXT_COMBODELAY, "combodelay");
    LIST_ADD(CMD_MODELSTXT_CREDSCORE, "credscore");
    LIST_ADD(CMD_MODELSTXT_DEBUG_ANAF, "agg_noatk_factor");
    LIST_ADD(CMD_MODELSTXT_DEBUG_GNAF, "group_noatk_factor");
    LIST_ADD(CMD_MODELSTXT_DEBUG_MAXNA, "max_noatk_chance");
    LIST_ADD(CMD_MODELSTXT_DEBUG_MINNA, "min_noatk_chance");
    LIST_ADD(CMD_MODELSTXT_DEBUG_MNAF, "move_noatk_factor");
    LIST_ADD(CMD_MODELSTXT_DEBUG_NAD, "noatk_duration");
    LIST_ADD(CMD_MODELSTXT_DEBUG_OSNAF, "offscreen_noatk_factor");
    LIST_ADD(CMD_MODELSTXT_DROPV, "dropv");
    LIST_ADD(CMD_MODELSTXT_GRABDISTANCE, "grabdistance");
    LIST_ADD(CMD_MODELSTXT_JUMPHEIGHT, "jumpheight");
    LIST_ADD(CMD_MODELSTXT_JUMPSPEED, "jumpspeed");
    LIST_ADD(CMD_MODELSTXT_KNOW, "know");
    LIST_ADD(CMD_MODELSTXT_LIFESCORE, "lifescore");
    LIST_ADD(CMD_MODELSTXT_LOAD, "load");
    LIST_ADD(CMD_MODELSTXT_MAX_COLLISIONS, "collisions.max");
    LIST_ADD(CMD_MODELSTXT_MAXATTACKS, "maxattacks");
    LIST_ADD(CMD_MODELSTXT_MAXATTACKTYPES, "maxattacktypes");
    LIST_ADD(CMD_MODELSTXT_MAXBACKWALKS, "maxbackwalks");
    LIST_ADD(CMD_MODELSTXT_MAXDOWNS, "maxdowns");
    LIST_ADD(CMD_MODELSTXT_MAXFOLLOWS, "maxfollows");
    LIST_ADD(CMD_MODELSTXT_MAXFREESPECIALS, "maxfreespecials");
    LIST_ADD(CMD_MODELSTXT_MAXIDLES, "maxidles");
    LIST_ADD(CMD_MODELSTXT_MAXUPS, "maxups");
    LIST_ADD(CMD_MODELSTXT_MAXWALKS, "maxwalks");
    LIST_ADD(CMD_MODELSTXT_MPBLOCK, "mpblock");
    LIST_ADD(CMD_MODELSTXT_MUSIC, "music");
    LIST_ADD(CMD_MODELSTXT_NOAIRCANCEL, "noaircancel");
    LIST_ADD(CMD_MODELSTXT_NOCHEATS, "nocheats");
    LIST_ADD(CMD_MODELSTXT_NOCHIPDEATH, "nochipdeath");
    LIST_ADD(CMD_MODELSTXT_NOCOST, "nocost");
    LIST_ADD(CMD_MODELSTXT_NODEBUG, "nodebug");
    LIST_ADD(CMD_MODELSTXT_NODROPEN, "nodropen");
    LIST_ADD(CMD_MODELSTXT_NODROPSPAWN, "nodropspawn");
    LIST_ADD(CMD_MODELSTXT_NOLOST, "nolost");
    LIST_ADD(CMD_MODELSTXT_NOMAXRUSHRESET, "nomaxrushreset");
    LIST_ADD(CMD_MODELSTXT_SPDIRECTION, "spdirection");
    LIST_ADD(CMD_MODELSTXT_VERSUSDAMAGE, "versusdamage");

#undef LIST_ADD
    return result;
}

List *createModelCommandList(void)
{
    int i, amax = 99;
    char buf[64] ;
    List *result = prepareList();
#define LIST_ADD(y,z) List_InsertAfter(result, (void*) y, z)
        LIST_ADD(CMD_MODEL_AGGRESSION, "aggression");
    LIST_ADD(CMD_MODEL_AIATTACK, "aiattack");
    LIST_ADD(CMD_MODEL_AIMOVE, "aimove");
    LIST_ADD(CMD_MODEL_AIRONLY, "aironly");
    LIST_ADD(CMD_MODEL_ALPHA, "alpha");
    LIST_ADD(CMD_MODEL_ALPHAMASK, "alphamask");
    LIST_ADD(CMD_MODEL_ALTERNATEPAL, "alternatepal");
    LIST_ADD(CMD_MODEL_ANIM, "anim");
    LIST_ADD(CMD_MODEL_ANIMAL, "animal");
    LIST_ADD(CMD_MODEL_ANIMATIONSCRIPT, "animationscript");
    LIST_ADD(CMD_MODEL_ANIMHEIGHT, "animheight");
    LIST_ADD(CMD_MODEL_ANTIGRAB, "antigrab");
    LIST_ADD(CMD_MODEL_ANTIGRAVITY, "antigravity");
    LIST_ADD(CMD_MODEL_AT_CMD, "@cmd");
    LIST_ADD(CMD_MODEL_AT_SCRIPT, "@script");
    LIST_ADD(CMD_MODEL_ATCHAIN, "atchain");
    LIST_ADD(CMD_MODEL_COLLISION, "attack");
    LIST_ADD(CMD_MODEL_COLLISION_BLOCK_COST,                       "attack.block.cost");                       // guardcost
    LIST_ADD(CMD_MODEL_COLLISION_BLOCK_PENETRATE,                  "attack.block.penetrate");                  // no_block
    LIST_ADD(CMD_MODEL_COLLISION_COUNTER,                          "attack.counter");
    LIST_ADD(CMD_MODEL_COLLISION_DAMAGE_FORCE,                     "attack.damage.force");
    LIST_ADD(CMD_MODEL_COLLISION_DAMAGE_LAND_FORCE,                "attack.damage.land.force");
    LIST_ADD(CMD_MODEL_COLLISION_DAMAGE_LAND_MODE,                 "attack.damage.land.mode");
    LIST_ADD(CMD_MODEL_COLLISION_DAMAGE_LETHAL_DISABLE,            "attack.damage.lethal.disable");            // No_kill / Only fall to 1 HP.
    LIST_ADD(CMD_MODEL_COLLISION_DAMAGE_RECURSIVE_FORCE,           "attack.damage.recursive.force");
    LIST_ADD(CMD_MODEL_COLLISION_DAMAGE_RECURSIVE_INDEX,           "attack.damage.recursive.index");
    LIST_ADD(CMD_MODEL_COLLISION_DAMAGE_RECURSIVE_MODE,            "attack.damage.recursive.mode");
    LIST_ADD(CMD_MODEL_COLLISION_DAMAGE_RECURSIVE_TIME_EXPIRE,     "attack.damage.recursive.time");
    LIST_ADD(CMD_MODEL_COLLISION_DAMAGE_RECURSIVE_TIME_RATE,       "attack.damage.recursive.rate");
    LIST_ADD(CMD_MODEL_COLLISION_DAMAGE_STEAL,                     "attack.damage.steal");
    LIST_ADD(CMD_MODEL_COLLISION_DAMAGE_TYPE,                      "attack.damage.type");
    LIST_ADD(CMD_MODEL_COLLISION_EFFECT_BLOCK_FLASH,               "attack.effect.block.flash.model");
    LIST_ADD(CMD_MODEL_COLLISION_EFFECT_BLOCK_SOUND,               "attack.effect.block.sound.path");
    LIST_ADD(CMD_MODEL_COLLISION_EFFECT_HIT_FLASH,                 "attack.effect.hit.flash.model");
    LIST_ADD(CMD_MODEL_COLLISION_EFFECT_HIT_FLASH_DISABLE,         "attack.effect.hit.flash.disable");
    LIST_ADD(CMD_MODEL_COLLISION_EFFECT_HIT_SOUND,                 "attack.effect.hit.sound.path");
    LIST_ADD(CMD_MODEL_COLLISION_GROUND,                           "attack.ground.mode");                       // otg
    LIST_ADD(CMD_MODEL_COLLISION_MAP_INDEX,                        "attack.map.index");                        // Forcemap.
    LIST_ADD(CMD_MODEL_COLLISION_MAP_TIME,                         "attack.map.time");                         // Forcemap time.
    LIST_ADD(CMD_MODEL_COLLISION_POSITION_X,                       "attack.position.x");
    LIST_ADD(CMD_MODEL_COLLISION_POSITION_Y,                       "attack.position.y");
    LIST_ADD(CMD_MODEL_COLLISION_REACTION_FALL_FORCE,              "attack.reaction.fall.force");              // Knockdown power
    LIST_ADD(CMD_MODEL_COLLISION_REACTION_FALL_VELOCITY_X,         "attack.reaction.fall.velocity.x");
    LIST_ADD(CMD_MODEL_COLLISION_REACTION_FALL_VELOCITY_Y,         "attack.reaction.fall.velocity.y");
    LIST_ADD(CMD_MODEL_COLLISION_REACTION_FALL_VELOCITY_Z,         "attack.reaction.fall.velocity.z");
    LIST_ADD(CMD_MODEL_COLLISION_REACTION_FREEZE_MODE,             "attack.reaction.freeze.mode");             // Freeze
    LIST_ADD(CMD_MODEL_COLLISION_REACTION_FREEZE_TIME,             "attack.reaction.freeze.time");             // Freeze time
    LIST_ADD(CMD_MODEL_COLLISION_REACTION_INVINCIBLE_TIME,         "attack.reaction.invincible.time");
    LIST_ADD(CMD_MODEL_COLLISION_REACTION_PAIN_SKIP,               "attack.reaction.pain.disable");            // No reflect / No pain
    LIST_ADD(CMD_MODEL_COLLISION_REACTION_PAUSE_TIME,              "attack.reaction.pause.time");              // Pause add
    LIST_ADD(CMD_MODEL_COLLISION_REACTION_REPOSITION_DIRECTION,    "attack.reaction.reposition.direction");
    LIST_ADD(CMD_MODEL_COLLISION_REACTION_REPOSITION_DISTANCE,     "attack.reaction.reposition.distance");     // Grabin distance
    LIST_ADD(CMD_MODEL_COLLISION_REACTION_REPOSITION_MODE,         "attack.reaction.reposition.mode");         // Grabin
    LIST_ADD(CMD_MODEL_COLLISION_SEAL_COST,                        "attack.seal.cost");                        // Seal max cost.
    LIST_ADD(CMD_MODEL_COLLISION_SEAL_TIME,                        "attack.seal.time");
    LIST_ADD(CMD_MODEL_COLLISION_SIZE_X,                           "attack.size.x");
    LIST_ADD(CMD_MODEL_COLLISION_SIZE_Y,                           "attack.size.y");
    LIST_ADD(CMD_MODEL_COLLISION_SIZE_Z_1,                         "attack.size.z.1");
    LIST_ADD(CMD_MODEL_COLLISION_SIZE_Z_2,                         "attack.size.z.2");
    LIST_ADD(CMD_MODEL_COLLISION_STAYDOWN_RISE,                    "attack.staydown.rise.time");
    LIST_ADD(CMD_MODEL_COLLISION_STAYDOWN_RISEATTACK,              "attack.staydown.attack.time");
    LIST_ADD(CMD_MODEL_COLLISION_TAG,                              "attack.tag");
    LIST_ADD(CMD_MODEL_COLLISION1, "attack1");
    LIST_ADD(CMD_MODEL_COLLISION2, "attack2");
    LIST_ADD(CMD_MODEL_COLLISION3, "attack3");
    LIST_ADD(CMD_MODEL_COLLISION4, "attack4");
    LIST_ADD(CMD_MODEL_COLLISION5, "attack5");
    LIST_ADD(CMD_MODEL_COLLISION6, "attack6");
    LIST_ADD(CMD_MODEL_COLLISION7, "attack7");
    LIST_ADD(CMD_MODEL_COLLISION8, "attack8");
    LIST_ADD(CMD_MODEL_COLLISION9, "attack9");
    LIST_ADD(CMD_MODEL_COLLISION10, "attack10");

    // Attacks 10+
    for(i = 11; i <= amax; i++)
    {
        sprintf(buf, "attack%d", i);
        LIST_ADD(CMD_MODEL_COLLISION_ETC, buf);
    }

    LIST_ADD(CMD_MODEL_COLLISIONONE, "attackone");
    LIST_ADD(CMD_MODEL_ATTACKTHROTTLE, "attackthrottle");
    LIST_ADD(CMD_MODEL_COLLISIONZ, "attackz");
    LIST_ADD(CMD_MODEL_BACKPAIN, "backpain");
    LIST_ADD(CMD_MODEL_BACKPAIN, "nohithead");
    LIST_ADD(CMD_MODEL_BBOX, "bbox");
    LIST_ADD(CMD_MODEL_BBOX_INDEX, "bbox.index");
    LIST_ADD(CMD_MODEL_BBOX_POSITION_X, "bbox.position.x");
    LIST_ADD(CMD_MODEL_BBOX_POSITION_Y, "bbox.position.y");
    LIST_ADD(CMD_MODEL_BBOX_SIZE_X, "bbox.size.x");
    LIST_ADD(CMD_MODEL_BBOX_SIZE_Y, "bbox.size.y");
    LIST_ADD(CMD_MODEL_BBOX_SIZE_Z_1, "bbox.size.z.1");
    LIST_ADD(CMD_MODEL_BBOX_SIZE_Z_2, "bbox.size.z.2");
    LIST_ADD(CMD_MODEL_BBOXZ, "bboxz");
    LIST_ADD(CMD_MODEL_BFLASH, "bflash");
    LIST_ADD(CMD_MODEL_BLAST, "blast");
    LIST_ADD(CMD_MODEL_BLOCKBACK, "blockback");
    LIST_ADD(CMD_MODEL_BLOCKFLASH, "blockflash");
    LIST_ADD(CMD_MODEL_BLOCKFX, "blockfx");
    LIST_ADD(CMD_MODEL_BLOCKODDS, "blockodds");
    LIST_ADD(CMD_MODEL_BLOCKPAIN, "blockpain");
    LIST_ADD(CMD_MODEL_BOMB, "bomb");
    LIST_ADD(CMD_MODEL_BOUNCE, "bounce");
    LIST_ADD(CMD_MODEL_BOUNCEFACTOR, "bouncefactor");
    LIST_ADD(CMD_MODEL_BRANCH, "branch");
    LIST_ADD(CMD_MODEL_BURN, "burn");
    LIST_ADD(CMD_MODEL_CANCEL, "cancel");
    LIST_ADD(CMD_MODEL_CANDAMAGE, "candamage");
    LIST_ADD(CMD_MODEL_CANTGRAB, "cantgrab");
    LIST_ADD(CMD_MODEL_CHARGERATE, "chargerate");
    LIST_ADD(CMD_MODEL_CHARGETIME, "chargetime");
    LIST_ADD(CMD_MODEL_COM, "com");
    LIST_ADD(CMD_MODEL_COMBOSTYLE, "combostyle");
    LIST_ADD(CMD_MODEL_COUNTER, "counter");
    LIST_ADD(CMD_MODEL_COUNTERATTACK, "counterattack");
    LIST_ADD(CMD_MODEL_COUNTERRANGE, "counterrange");
    LIST_ADD(CMD_MODEL_CREDIT, "credit");
    LIST_ADD(CMD_MODEL_CUSTBOMB, "custbomb");
    LIST_ADD(CMD_MODEL_CUSTENTITY, "custentity");
    LIST_ADD(CMD_MODEL_CUSTKNIFE, "custknife");
    LIST_ADD(CMD_MODEL_CUSTPBOMB, "custpbomb");
    LIST_ADD(CMD_MODEL_CUSTPSHOT, "custpshot");
    LIST_ADD(CMD_MODEL_CUSTPSHOTNO, "custpshotno");
    LIST_ADD(CMD_MODEL_CUSTPSHOTW, "custpshotw");
    LIST_ADD(CMD_MODEL_CUSTSTAR, "custstar");
    LIST_ADD(CMD_MODEL_DAMAGEONLANDING, "damageonlanding");
    LIST_ADD(CMD_MODEL_DEATH, "death");
    LIST_ADD(CMD_MODEL_DEFENSE, "defense");
    LIST_ADD(CMD_MODEL_DELAY, "delay");
    LIST_ADD(CMD_MODEL_DIDBLOCKSCRIPT, "didblockscript");
    LIST_ADD(CMD_MODEL_DIDHITSCRIPT, "didhitscript");
    LIST_ADD(CMD_MODEL_DIESOUND, "diesound");
    LIST_ADD(CMD_MODEL_DIVE, "dive");
    LIST_ADD(CMD_MODEL_DIVE1, "dive1");
    LIST_ADD(CMD_MODEL_DIVE2, "dive2");
    LIST_ADD(CMD_MODEL_DOT, "dot");
    LIST_ADD(CMD_MODEL_DRAWMETHOD, "drawmethod");
    LIST_ADD(CMD_MODEL_DROPFRAME, "dropframe");
    LIST_ADD(CMD_MODEL_DROPV, "dropv");
    LIST_ADD(CMD_MODEL_DUST, "dust");
    LIST_ADD(CMD_MODEL_EBOX, "ebox");
    LIST_ADD(CMD_MODEL_EBOX_INDEX, "ebox.index");
    LIST_ADD(CMD_MODEL_EBOX_POSITION_X, "ebox.position.x");
    LIST_ADD(CMD_MODEL_EBOX_POSITION_Y, "ebox.position.y");
    LIST_ADD(CMD_MODEL_EBOX_SIZE_X, "ebox.size.x");
    LIST_ADD(CMD_MODEL_EBOX_SIZE_Y, "ebox.size.y");
    LIST_ADD(CMD_MODEL_EBOX_SIZE_Z_1, "ebox.size.z.1");
    LIST_ADD(CMD_MODEL_EBOX_SIZE_Z_2, "ebox.size.z.2");
    LIST_ADD(CMD_MODEL_EBOXZ, "eboxz");
    LIST_ADD(CMD_MODEL_EDELAY, "edelay");
    LIST_ADD(CMD_MODEL_EDGERANGE, "edgerange");
    LIST_ADD(CMD_MODEL_ENERGYCOST, "energycost");
    LIST_ADD(CMD_MODEL_ENTITYPUSHING, "entitypushing");
    LIST_ADD(CMD_MODEL_ESCAPEHITS, "escapehits");
    LIST_ADD(CMD_MODEL_FACING, "facing");
    LIST_ADD(CMD_MODEL_FALLDIE, "falldie");
    LIST_ADD(CMD_MODEL_FASTATTACK, "fastattack");
    LIST_ADD(CMD_MODEL_FIREB, "fireb");
    LIST_ADD(CMD_MODEL_FLASH, "flash");
    LIST_ADD(CMD_MODEL_FLIPFRAME, "flipframe");
    LIST_ADD(CMD_MODEL_FMAP, "fmap");
    LIST_ADD(CMD_MODEL_FOLLOWANIM, "followanim");
    LIST_ADD(CMD_MODEL_FOLLOWCOND, "followcond");
    LIST_ADD(CMD_MODEL_FORCEDIRECTION, "forcedirection");
    LIST_ADD(CMD_MODEL_FORCEMAP, "forcemap");
    LIST_ADD(CMD_MODEL_FRAME, "frame");
    LIST_ADD(CMD_MODEL_FREEZE, "freeze");
    LIST_ADD(CMD_MODEL_FSHADOW, "fshadow");
    LIST_ADD(CMD_MODEL_GFXSHADOW, "gfxshadow");
    LIST_ADD(CMD_MODEL_GLOBALMAP, "globalmap");
    LIST_ADD(CMD_MODEL_GRABBACK, "grabback");
    LIST_ADD(CMD_MODEL_GRABDISTANCE, "grabdistance");
    LIST_ADD(CMD_MODEL_GRABFINISH, "grabfinish");
    LIST_ADD(CMD_MODEL_GRABFLIP, "grabflip");
    LIST_ADD(CMD_MODEL_GRABFORCE, "grabforce");
    LIST_ADD(CMD_MODEL_GRABIN, "grabin");
    LIST_ADD(CMD_MODEL_GRABTURN, "grabturn");
    LIST_ADD(CMD_MODEL_GRABWALK, "grabwalk");
    LIST_ADD(CMD_MODEL_GUARDCOST, "guardcost");
    LIST_ADD(CMD_MODEL_GUARDPOINTS, "guardpoints");
    LIST_ADD(CMD_MODEL_GUARDRATE, "guardrate");
    LIST_ADD(CMD_MODEL_HEALTH, "health");
    LIST_ADD(CMD_MODEL_HEIGHT, "height");
    LIST_ADD(CMD_MODEL_HITENEMY, "hitenemy");
    LIST_ADD(CMD_MODEL_HITFLASH, "hitflash");
    LIST_ADD(CMD_MODEL_HITFX, "hitfx");
    LIST_ADD(CMD_MODEL_HITWALLTYPE, "hitwalltype");
    LIST_ADD(CMD_MODEL_HITZ, "hitz");
    LIST_ADD(CMD_MODEL_HMAP, "hmap");
    LIST_ADD(CMD_MODEL_HOLDBLOCK, "holdblock");
    LIST_ADD(CMD_MODEL_HOSTILE, "hostile");
    LIST_ADD(CMD_MODEL_ICON, "icon");
    LIST_ADD(CMD_MODEL_ICONDIE, "icondie");
    LIST_ADD(CMD_MODEL_ICONGET, "iconget");
    LIST_ADD(CMD_MODEL_ICONMPHALF, "iconmphalf");
    LIST_ADD(CMD_MODEL_ICONMPHIGH, "iconmphigh");
    LIST_ADD(CMD_MODEL_ICONMPLOW, "iconmplow");
    LIST_ADD(CMD_MODEL_ICONPAIN, "iconpain");
    LIST_ADD(CMD_MODEL_ICONPOSITION, "iconposition");
    LIST_ADD(CMD_MODEL_ICONW, "iconw");
    LIST_ADD(CMD_MODEL_IDLE, "idle");
    LIST_ADD(CMD_MODEL_IGNOREATTACKID, "ignoreattackid");
    LIST_ADD(CMD_MODEL_INHOLESCRIPT, "inholescript");
    LIST_ADD(CMD_MODEL_INSTANTITEMDEATH, "instantitemdeath");
    LIST_ADD(CMD_MODEL_ITEMBOX, "itembox");
    LIST_ADD(CMD_MODEL_JUGGLECOST, "jugglecost");
    LIST_ADD(CMD_MODEL_JUGGLEPOINTS, "jugglepoints");
    LIST_ADD(CMD_MODEL_JUMPFRAME, "jumpframe");
    LIST_ADD(CMD_MODEL_JUMPHEIGHT, "jumpheight");
    LIST_ADD(CMD_MODEL_JUMPMOVE, "jumpmove");
    LIST_ADD(CMD_MODEL_JUMPSPEED, "jumpspeed");
    LIST_ADD(CMD_MODEL_JUMPSPEEDF, "jumpspeedf");
    LIST_ADD(CMD_MODEL_KEYSCRIPT, "keyscript");
    LIST_ADD(CMD_MODEL_KNIFE, "knife");
    LIST_ADD(CMD_MODEL_KNOCKDOWNCOUNT, "knockdowncount");
    LIST_ADD(CMD_MODEL_KOMAP, "komap");
    LIST_ADD(CMD_MODEL_LANDFRAME, "landframe");
    LIST_ADD(CMD_MODEL_LIFEBARSTATUS, "lifebarstatus");
    LIST_ADD(CMD_MODEL_LIFEPOSITION, "lifeposition");
    LIST_ADD(CMD_MODEL_LIFESPAN, "lifespan");
    LIST_ADD(CMD_MODEL_LOAD, "load");
    LIST_ADD(CMD_MODEL_LOOP, "loop");
    LIST_ADD(CMD_MODEL_LOSE, "lose");
    LIST_ADD(CMD_MODEL_MAKEINV, "makeinv");
    LIST_ADD(CMD_MODEL_MODELFLAG, "modelflag");
    LIST_ADD(CMD_MODEL_MOVE, "move");
    LIST_ADD(CMD_MODEL_MOVEA, "movea");
    LIST_ADD(CMD_MODEL_MOVEZ, "movez");
    LIST_ADD(CMD_MODEL_MP, "mp");
    LIST_ADD(CMD_MODEL_MPCOST, "mpcost");
    LIST_ADD(CMD_MODEL_MPONLY, "mponly");
    LIST_ADD(CMD_MODEL_MPRATE, "mprate");
    LIST_ADD(CMD_MODEL_MPSET, "mpset");
    LIST_ADD(CMD_MODEL_NAME, "name");
    LIST_ADD(CMD_MODEL_NAMEPOSITION, "nameposition");
    LIST_ADD(CMD_MODEL_NO_ADJUST_BASE, "no_adjust_base");
    LIST_ADD(CMD_MODEL_NOATFLASH, "noatflash");
    LIST_ADD(CMD_MODEL_NODIEBLINK, "nodieblink");
    LIST_ADD(CMD_MODEL_NODRAWMETHOD, "nodrawmethod");
    LIST_ADD(CMD_MODEL_NODROP, "nodrop");
    LIST_ADD(CMD_MODEL_NOGRAB, "nograb");
    LIST_ADD(CMD_MODEL_NOKILL, "nokill");
    LIST_ADD(CMD_MODEL_NOLIFE, "nolife");
    LIST_ADD(CMD_MODEL_NOMOVE, "nomove");
    LIST_ADD(CMD_MODEL_NOPAIN, "nopain");
    LIST_ADD(CMD_MODEL_NOPASSIVEBLOCK, "nopassiveblock");
    LIST_ADD(CMD_MODEL_NOQUAKE, "noquake");
    LIST_ADD(CMD_MODEL_NOREFLECT, "noreflect");
    LIST_ADD(CMD_MODEL_NOTGRAB, "notgrab");
    LIST_ADD(CMD_MODEL_OFFENSE, "offense");
    LIST_ADD(CMD_MODEL_OFFSCREENKILL, "offscreenkill");
    LIST_ADD(CMD_MODEL_ONAF, "offscreen_noatk_factor");
    LIST_ADD(CMD_MODEL_OFFSET, "offset");
    LIST_ADD(CMD_MODEL_ONBLOCKASCRIPT, "onblockascript");
    LIST_ADD(CMD_MODEL_ONBLOCKOSCRIPT, "onblockoscript");
    LIST_ADD(CMD_MODEL_ONBLOCKPSCRIPT, "onblockpscript");
    LIST_ADD(CMD_MODEL_ONBLOCKSSCRIPT, "onblocksscript");
    LIST_ADD(CMD_MODEL_ONBLOCKWSCRIPT, "onblockwscript");
    LIST_ADD(CMD_MODEL_ONBLOCKZSCRIPT, "onblockzscript");
    LIST_ADD(CMD_MODEL_ONDEATHSCRIPT, "ondeathscript");
    LIST_ADD(CMD_MODEL_ONDOATTACKSCRIPT, "ondoattackscript");
    LIST_ADD(CMD_MODEL_ONDRAWSCRIPT, "ondrawscript");
    LIST_ADD(CMD_MODEL_ONENTITYCOLLISIONSCRIPT, "onentitycollisionscript");
    LIST_ADD(CMD_MODEL_ONFALLSCRIPT, "onfallscript");
    LIST_ADD(CMD_MODEL_ONKILLSCRIPT, "onkillscript");
    LIST_ADD(CMD_MODEL_ONMODELCOPYSCRIPT, "onmodelcopyscript");
    LIST_ADD(CMD_MODEL_ONMOVEASCRIPT, "onmoveascript");
    LIST_ADD(CMD_MODEL_ONMOVEXSCRIPT, "onmovexscript");
    LIST_ADD(CMD_MODEL_ONMOVEZSCRIPT, "onmovezscript");
    LIST_ADD(CMD_MODEL_ONPAINSCRIPT, "onpainscript");
    LIST_ADD(CMD_MODEL_ONSPAWNSCRIPT, "onspawnscript");
    LIST_ADD(CMD_MODEL_OTG, "otg");
    LIST_ADD(CMD_MODEL_PAINGRAB, "paingrab");
    LIST_ADD(CMD_MODEL_PALETTE, "palette");
    LIST_ADD(CMD_MODEL_PARROW, "parrow");
    LIST_ADD(CMD_MODEL_PARROW2, "parrow2");
    LIST_ADD(CMD_MODEL_PARROW3, "parrow3");
    LIST_ADD(CMD_MODEL_PARROW4, "parrow4");
    LIST_ADD(CMD_MODEL_PATHFINDSTEP, "pfstep");
    LIST_ADD(CMD_MODEL_PBOMBFRAME, "pbombframe");
    LIST_ADD(CMD_MODEL_PLATFORM, "platform");
    LIST_ADD(CMD_MODEL_PLAYBOMB, "playbomb");
    LIST_ADD(CMD_MODEL_PLAYSHOT, "playshot");
    LIST_ADD(CMD_MODEL_PLAYSHOTNO, "playshotno");
    LIST_ADD(CMD_MODEL_PLAYSHOTW, "playshotw");
    LIST_ADD(CMD_MODEL_PRIORITY, "priority");
    LIST_ADD(CMD_MODEL_PROJECT, "project");
    LIST_ADD(CMD_MODEL_PROJECTILEHIT, "projectilehit");
    LIST_ADD(CMD_MODEL_PSHOTFRAME, "pshotframe");
    LIST_ADD(CMD_MODEL_PSHOTFRAMENO, "pshotframeno");
    LIST_ADD(CMD_MODEL_PSHOTFRAMEW, "pshotframew");
    LIST_ADD(CMD_MODEL_PUSHINGFACTOR, "pushingfactor");
    LIST_ADD(CMD_MODEL_QUAKEFRAME, "quakeframe");
    LIST_ADD(CMD_MODEL_RANGE, "range");
    LIST_ADD(CMD_MODEL_RANGEA, "rangea");
    LIST_ADD(CMD_MODEL_RANGEB, "rangeb");
    LIST_ADD(CMD_MODEL_RANGEZ, "rangez");
    LIST_ADD(CMD_MODEL_RELOAD, "reload");
    LIST_ADD(CMD_MODEL_REMAP, "remap");
    LIST_ADD(CMD_MODEL_REMOVE, "remove");
    LIST_ADD(CMD_MODEL_RIDER, "rider");
    LIST_ADD(CMD_MODEL_RISEATTACKTYPE, "riseattacktype");
    LIST_ADD(CMD_MODEL_RISEINV, "riseinv");
    LIST_ADD(CMD_MODEL_RISETIME, "risetime");
    LIST_ADD(CMD_MODEL_RUNNING, "running");
    LIST_ADD(CMD_MODEL_RUNNING_CONTINUE, "run.jump.continue");
    LIST_ADD(CMD_MODEL_RUNNING_JUMP_VELOCITY_X, "run.jump.x");
    LIST_ADD(CMD_MODEL_RUNNING_JUMP_VELOCITY_Y, "run.jump.y");
    LIST_ADD(CMD_MODEL_RUNNING_SPEED, "run.speed.x");
    LIST_ADD(CMD_MODEL_RUNNING_Z_MOVE, "run.move.z");
    LIST_ADD(CMD_MODEL_SCORE, "score");
    LIST_ADD(CMD_MODEL_SCRIPT, "script");
    LIST_ADD(CMD_MODEL_SCROLL, "scroll");
    LIST_ADD(CMD_MODEL_SEAL, "seal");
    LIST_ADD(CMD_MODEL_SECRET, "secret");
    LIST_ADD(CMD_MODEL_SETA, "seta");
    LIST_ADD(CMD_MODEL_SETLAYER, "setlayer");
    LIST_ADD(CMD_MODEL_SHADOW, "shadow");
    LIST_ADD(CMD_MODEL_SHADOWCOORDS, "shadowcoords");
    LIST_ADD(CMD_MODEL_SHOCK, "shock");
    LIST_ADD(CMD_MODEL_SHOOTFRAME, "shootframe");
    LIST_ADD(CMD_MODEL_SHOOTNUM, "shootnum");
    LIST_ADD(CMD_MODEL_SLEEPWAIT, "sleepwait");
    LIST_ADD(CMD_MODEL_SMARTBOMB, "smartbomb");
    LIST_ADD(CMD_MODEL_SOUND, "sound");
    LIST_ADD(CMD_MODEL_SPAWNFRAME, "spawnframe");
    LIST_ADD(CMD_MODEL_SPEED, "speed");
    LIST_ADD(CMD_MODEL_SPEEDF, "speedf");
    LIST_ADD(CMD_MODEL_STAR, "star");
    LIST_ADD(CMD_MODEL_STAR_VELOCITY, "starvelocity");
    LIST_ADD(CMD_MODEL_STAYDOWN, "staydown");
    LIST_ADD(CMD_MODEL_STEAL, "steal");
    LIST_ADD(CMD_MODEL_STEALTH, "stealth");
    LIST_ADD(CMD_MODEL_STUN, "stun");
    LIST_ADD(CMD_MODEL_SUBCLASS, "subclass");
    LIST_ADD(CMD_MODEL_SUBENTITY, "subentity");
    LIST_ADD(CMD_MODEL_SUBJECT_TO_BASEMAP, "subject_to_basemap");
    LIST_ADD(CMD_MODEL_SUBJECT_TO_GRAVITY, "subject_to_gravity");
    LIST_ADD(CMD_MODEL_SUBJECT_TO_HOLE, "subject_to_hole");
    LIST_ADD(CMD_MODEL_SUBJECT_TO_MAXZ, "subject_to_maxz");
    LIST_ADD(CMD_MODEL_SUBJECT_TO_MINZ, "subject_to_minz");
    LIST_ADD(CMD_MODEL_SUBJECT_TO_OBSTACLE, "subject_to_obstacle");
    LIST_ADD(CMD_MODEL_SUBJECT_TO_PLATFORM, "subject_to_platform");
    LIST_ADD(CMD_MODEL_SUBJECT_TO_SCREEN, "subject_to_screen");
    LIST_ADD(CMD_MODEL_SUBJECT_TO_WALL, "subject_to_wall");
    LIST_ADD(CMD_MODEL_SUBTYPE, "subtype");
    LIST_ADD(CMD_MODEL_SUMMONFRAME, "summonframe");
    LIST_ADD(CMD_MODEL_SUMMONKILL, "summonkill");
    LIST_ADD(CMD_MODEL_SYNC, "sync");
    LIST_ADD(CMD_MODEL_TAKEDAMAGESCRIPT, "takedamagescript");
    LIST_ADD(CMD_MODEL_THINKSCRIPT, "thinkscript");
    LIST_ADD(CMD_MODEL_THOLD, "thold");
    LIST_ADD(CMD_MODEL_THROW, "throw");
    LIST_ADD(CMD_MODEL_THROWDAMAGE, "throwdamage");
    LIST_ADD(CMD_MODEL_THROWFRAME, "throwframe");
    LIST_ADD(CMD_MODEL_THROWFRAMEWAIT, "throwframewait");
    LIST_ADD(CMD_MODEL_TOFLIP, "toflip");
    LIST_ADD(CMD_MODEL_TOSSFRAME, "tossframe");
    LIST_ADD(CMD_MODEL_TURNDELAY, "turndelay");
    LIST_ADD(CMD_MODEL_TYPE, "type");
    LIST_ADD(CMD_MODEL_TYPESHOT, "typeshot");
    LIST_ADD(CMD_MODEL_UNSUMMONFRAME, "unsummonframe");
    LIST_ADD(CMD_MODEL_WALKOFFMOVE, "walkoffmove");
    LIST_ADD(CMD_MODEL_WEAPLOSS, "weaploss");
    LIST_ADD(CMD_MODEL_WEAPNUM, "weapnum");
    LIST_ADD(CMD_MODEL_WEAPONFRAME, "weaponframe");
    LIST_ADD(CMD_MODEL_WEAPONS, "weapons");


#undef LIST_ADD
    return result;
}

List *createLevelCommandList(void)
{
    List *result = prepareList();
#define LIST_ADD(y,z) List_InsertAfter(result, (void*) y, z)

    LIST_ADD(CMD_LEVEL_2PHEALTH, "2phealth");
    LIST_ADD(CMD_LEVEL_2PITEM, "2pitem");
    LIST_ADD(CMD_LEVEL_2PSPAWN, "2pspawn");
    LIST_ADD(CMD_LEVEL_3PHEALTH, "3phealth");
    LIST_ADD(CMD_LEVEL_3PITEM, "3pitem");
    LIST_ADD(CMD_LEVEL_3PSPAWN, "3pspawn");
    LIST_ADD(CMD_LEVEL_4PHEALTH, "4phealth");
    LIST_ADD(CMD_LEVEL_4PITEM, "4pitem");
    LIST_ADD(CMD_LEVEL_4PSPAWN, "4pspawn");
    LIST_ADD(CMD_LEVEL_AGGRESSION, "aggression");
    LIST_ADD(CMD_LEVEL_ALIAS, "alias");
    LIST_ADD(CMD_LEVEL_ALLOWSELECT, "allowselect");
    LIST_ADD(CMD_LEVEL_ALPHA, "alpha");
    LIST_ADD(CMD_LEVEL_ALPHAMASK, "alphamask");
    LIST_ADD(CMD_LEVEL_AT, "at");
    LIST_ADD(CMD_LEVEL_AT_SCRIPT, "@script");
    LIST_ADD(CMD_LEVEL_BACKGROUND, "background");
    LIST_ADD(CMD_LEVEL_BASEMAP, "basemap");
    LIST_ADD(CMD_LEVEL_BGLAYER, "bglayer");
    LIST_ADD(CMD_LEVEL_BGSPEED, "bgspeed");
    LIST_ADD(CMD_LEVEL_BLOCKADE, "blockade");
    LIST_ADD(CMD_LEVEL_BLOCKED, "blocked");
    LIST_ADD(CMD_LEVEL_BOSS, "boss");
    LIST_ADD(CMD_LEVEL_BOSSMUSIC, "bossmusic");
    LIST_ADD(CMD_LEVEL_CAMERAOFFSET, "cameraoffset");
    LIST_ADD(CMD_LEVEL_CAMERATYPE, "cameratype");
    LIST_ADD(CMD_LEVEL_CANJOIN, "canjoin");
    LIST_ADD(CMD_LEVEL_COORDS, "coords");
    LIST_ADD(CMD_LEVEL_CREDIT, "credit");
    LIST_ADD(CMD_LEVEL_DIRECTION, "direction");
    LIST_ADD(CMD_LEVEL_DYING, "dying");
    LIST_ADD(CMD_LEVEL_ENDHOLE, "endhole");
    LIST_ADD(CMD_LEVEL_ENDLEVELSCRIPT, "endlevelscript");
    LIST_ADD(CMD_LEVEL_FACING, "facing");
    LIST_ADD(CMD_LEVEL_FGLAYER, "fglayer");
    LIST_ADD(CMD_LEVEL_FLIP, "flip");
    LIST_ADD(CMD_LEVEL_FRONTPANEL, "frontpanel");
    LIST_ADD(CMD_LEVEL_GRAVITY, "gravity");
    LIST_ADD(CMD_LEVEL_GROUP, "group");
    LIST_ADD(CMD_LEVEL_HEALTH, "health");
    LIST_ADD(CMD_LEVEL_HOLE, "hole");
    LIST_ADD(CMD_LEVEL_ITEM, "item");
    LIST_ADD(CMD_LEVEL_ITEMALIAS, "itemalias");
    LIST_ADD(CMD_LEVEL_ITEMALPHA, "itemalpha");
    LIST_ADD(CMD_LEVEL_ITEMHEALTH, "itemhealth");
    LIST_ADD(CMD_LEVEL_ITEMMAP, "itemmap");
    LIST_ADD(CMD_LEVEL_ITEMTRANS, "itemtrans");
    LIST_ADD(CMD_LEVEL_KEYSCRIPT, "keyscript");
    LIST_ADD(CMD_LEVEL_LAYER, "layer");
    LIST_ADD(CMD_LEVEL_LEVELSCRIPT, "levelscript");
    LIST_ADD(CMD_LEVEL_LIGHT, "light");
    LIST_ADD(CMD_LEVEL_LOAD, "load");
    LIST_ADD(CMD_LEVEL_LOADINGBG, "loadingbg");
    LIST_ADD(CMD_LEVEL_MAP, "map");
    LIST_ADD(CMD_LEVEL_MAXFALLSPEED, "maxfallspeed");
    LIST_ADD(CMD_LEVEL_MAXTOSSSPEED, "maxtossspeed");
    LIST_ADD(CMD_LEVEL_MIRROR, "mirror");
    LIST_ADD(CMD_LEVEL_MP, "mp");
    LIST_ADD(CMD_LEVEL_MUSIC, "music");
    LIST_ADD(CMD_LEVEL_MUSICFADE, "musicfade");
    LIST_ADD(CMD_LEVEL_NOFADEOUT, "nofadeout");
    LIST_ADD(CMD_LEVEL_NOHIT, "nohit");
    LIST_ADD(CMD_LEVEL_NOJOIN, "nojoin");
    LIST_ADD(CMD_LEVEL_NOLIFE, "nolife");
    LIST_ADD(CMD_LEVEL_NOPAUSE, "nopause");
    LIST_ADD(CMD_LEVEL_NORESET, "noreset");
    LIST_ADD(CMD_LEVEL_NOSAVE, "nosave");
    LIST_ADD(CMD_LEVEL_NOSCREENSHOT, "noscreenshot");
    LIST_ADD(CMD_LEVEL_NOSLOW, "noslow");
    LIST_ADD(CMD_LEVEL_NOTIME, "notime");
    LIST_ADD(CMD_LEVEL_ORDER, "order");
    LIST_ADD(CMD_LEVEL_PALETTE, "palette");
    LIST_ADD(CMD_LEVEL_PANEL, "panel");
    LIST_ADD(CMD_LEVEL_ROCK, "rock");
    LIST_ADD(CMD_LEVEL_SCORE, "score");
    LIST_ADD(CMD_LEVEL_SCROLLSPEED, "scrollspeed");
    LIST_ADD(CMD_LEVEL_SCROLLX, "scrollx");
    LIST_ADD(CMD_LEVEL_SCROLLZ, "scrollz");
    LIST_ADD(CMD_LEVEL_SETPALETTE, "setpalette");
    LIST_ADD(CMD_LEVEL_SETTIME, "settime");
    LIST_ADD(CMD_LEVEL_SETWEAP, "setweap");
    LIST_ADD(CMD_LEVEL_SHADOWALPHA, "shadowalpha");
    LIST_ADD(CMD_LEVEL_SHADOWCOLOR, "shadowcolor");
    LIST_ADD(CMD_LEVEL_SHADOWOPACITY, "shadowopacity");
    LIST_ADD(CMD_LEVEL_SPAWN, "spawn");
    LIST_ADD(CMD_LEVEL_SPAWN1, "spawn1");
    LIST_ADD(CMD_LEVEL_SPAWN2, "spawn2");
    LIST_ADD(CMD_LEVEL_SPAWN3, "spawn3");
    LIST_ADD(CMD_LEVEL_SPAWN4, "spawn4");
    LIST_ADD(CMD_LEVEL_SPAWNSCRIPT, "spawnscript");
    LIST_ADD(CMD_LEVEL_STAGENUMBER, "stagenumber");
    LIST_ADD(CMD_LEVEL_TYPE, "type");
    LIST_ADD(CMD_LEVEL_UPDATEDSCRIPT, "updatedscript");
    LIST_ADD(CMD_LEVEL_UPDATESCRIPT, "updatescript");
    LIST_ADD(CMD_LEVEL_VBGSPEED, "vbgspeed");
    LIST_ADD(CMD_LEVEL_WAIT, "wait");
    LIST_ADD(CMD_LEVEL_WALL, "wall");
    LIST_ADD(CMD_LEVEL_WATER, "water");
    LIST_ADD(CMD_LEVEL_WEAPON, "weapon");

#undef LIST_ADD
    return result;
}

List *createLevelOrderCommandList(void)
{
    List *result = prepareList();
#define LIST_ADD(y,z) List_InsertAfter(result, (void*) y, z)
    LIST_ADD(CMD_LEVELORDER_BGICON, "bgicon");
    LIST_ADD(CMD_LEVELORDER_BLENDFX, "blendfx");
    LIST_ADD(CMD_LEVELORDER_BRANCH, "branch");
    LIST_ADD(CMD_LEVELORDER_CANSAVE, "cansave");
    LIST_ADD(CMD_LEVELORDER_CLEARBONUS, "clearbonus");
    LIST_ADD(CMD_LEVELORDER_COMPLETEBG, "completebg");
    LIST_ADD(CMD_LEVELORDER_CONTINUESCORE, "continuescore");
    LIST_ADD(CMD_LEVELORDER_CREDITS, "credits");
    LIST_ADD(CMD_LEVELORDER_CUSTFADE, "custfade");
    LIST_ADD(CMD_LEVELORDER_DISABLEGAMEOVER, "disablegameover");
    LIST_ADD(CMD_LEVELORDER_DISABLEHOF, "disablehof");
    LIST_ADD(CMD_LEVELORDER_E1ICON, "e1icon");
    LIST_ADD(CMD_LEVELORDER_E1LIFE, "e1life");
    LIST_ADD(CMD_LEVELORDER_E1NAME, "e1name");
    LIST_ADD(CMD_LEVELORDER_E2ICON, "e2icon");
    LIST_ADD(CMD_LEVELORDER_E2LIFE, "e2life");
    LIST_ADD(CMD_LEVELORDER_E2NAME, "e2name");
    LIST_ADD(CMD_LEVELORDER_E3ICON, "e3icon");
    LIST_ADD(CMD_LEVELORDER_E3LIFE, "e3life");
    LIST_ADD(CMD_LEVELORDER_E3NAME, "e3name");
    LIST_ADD(CMD_LEVELORDER_E4ICON, "e4icon");
    LIST_ADD(CMD_LEVELORDER_E4LIFE, "e4life");
    LIST_ADD(CMD_LEVELORDER_E4NAME, "e4name");
    LIST_ADD(CMD_LEVELORDER_END, "end");
    LIST_ADD(CMD_LEVELORDER_EQUALAIRPAUSE, "equalairpause");
    LIST_ADD(CMD_LEVELORDER_FILE, "file");
    LIST_ADD(CMD_LEVELORDER_GRAVITY, "gravity");
    LIST_ADD(CMD_LEVELORDER_HISCOREBG, "hiscorebg");
    LIST_ADD(CMD_LEVELORDER_IFCOMPLETE, "ifcomplete");
    LIST_ADD(CMD_LEVELORDER_LBARSIZE, "lbarsize");
    LIST_ADD(CMD_LEVELORDER_LBARTEXT, "lbartext");
    LIST_ADD(CMD_LEVELORDER_LIFEBONUS, "lifebonus");
    LIST_ADD(CMD_LEVELORDER_LIVES, "lives");
    LIST_ADD(CMD_LEVELORDER_LOADINGBG, "loadingbg");
    LIST_ADD(CMD_LEVELORDER_LOADINGBG2, "loadingbg2");
    LIST_ADD(CMD_LEVELORDER_LOADINGMUSIC, "loadingmusic");
    LIST_ADD(CMD_LEVELORDER_MAXENTITIES, "maxentities");
    LIST_ADD(CMD_LEVELORDER_MAXPLAYERS, "maxplayers");
    LIST_ADD(CMD_LEVELORDER_MAXWALLHEIGHT, "maxwallheight");
    LIST_ADD(CMD_LEVELORDER_MP1ICON, "mp1icon");
    LIST_ADD(CMD_LEVELORDER_MP2ICON, "mp2icon");
    LIST_ADD(CMD_LEVELORDER_MP3ICON, "mp3icon");
    LIST_ADD(CMD_LEVELORDER_MP4ICON, "mp4icon");
    LIST_ADD(CMD_LEVELORDER_MPBARSIZE, "mpbarsize");
    LIST_ADD(CMD_LEVELORDER_MPBARTEXT, "mpbartext");
    LIST_ADD(CMD_LEVELORDER_MUSICOVERLAP, "musicoverlap");
    LIST_ADD(CMD_LEVELORDER_NEXT, "next");
    LIST_ADD(CMD_LEVELORDER_NOSAME, "nosame");
    LIST_ADD(CMD_LEVELORDER_NOSHARE, "noshare");
    LIST_ADD(CMD_LEVELORDER_NOSHOWCOMPLETE, "noshowcomplete");
    LIST_ADD(CMD_LEVELORDER_NOSLOWFX, "noslowfx");
    LIST_ADD(CMD_LEVELORDER_OLBARSIZE, "olbarsize");
    LIST_ADD(CMD_LEVELORDER_OLICON, "olicon");
    LIST_ADD(CMD_LEVELORDER_P1ICON, "p1icon");
    LIST_ADD(CMD_LEVELORDER_P1ICONW, "p1iconw");
    LIST_ADD(CMD_LEVELORDER_P1LIFE, "p1life");
    LIST_ADD(CMD_LEVELORDER_P1LIFEN, "p1lifen");
    LIST_ADD(CMD_LEVELORDER_P1LIFEX, "p1lifex");
    LIST_ADD(CMD_LEVELORDER_P1MP, "p1mp");
    LIST_ADD(CMD_LEVELORDER_P1NAMEJ, "p1namej");
    LIST_ADD(CMD_LEVELORDER_P1RUSH, "p1rush");
    LIST_ADD(CMD_LEVELORDER_P1SCORE, "p1score");
    LIST_ADD(CMD_LEVELORDER_P1SHOOT, "p1shoot");
    LIST_ADD(CMD_LEVELORDER_P1SMENU, "p1smenu");
    LIST_ADD(CMD_LEVELORDER_P2ICON, "p2icon");
    LIST_ADD(CMD_LEVELORDER_P2ICONW, "p2iconw");
    LIST_ADD(CMD_LEVELORDER_P2LIFE, "p2life");
    LIST_ADD(CMD_LEVELORDER_P2LIFEN, "p2lifen");
    LIST_ADD(CMD_LEVELORDER_P2LIFEX, "p2lifex");
    LIST_ADD(CMD_LEVELORDER_P2MP, "p2mp");
    LIST_ADD(CMD_LEVELORDER_P2NAMEJ, "p2namej");
    LIST_ADD(CMD_LEVELORDER_P2RUSH, "p2rush");
    LIST_ADD(CMD_LEVELORDER_P2SCORE, "p2score");
    LIST_ADD(CMD_LEVELORDER_P2SHOOT, "p2shoot");
    LIST_ADD(CMD_LEVELORDER_P2SMENU, "p2smenu");
    LIST_ADD(CMD_LEVELORDER_P3ICON, "p3icon");
    LIST_ADD(CMD_LEVELORDER_P3ICONW, "p3iconw");
    LIST_ADD(CMD_LEVELORDER_P3LIFE, "p3life");
    LIST_ADD(CMD_LEVELORDER_P3LIFEN, "p3lifen");
    LIST_ADD(CMD_LEVELORDER_P3LIFEX, "p3lifex");
    LIST_ADD(CMD_LEVELORDER_P3MP, "p3mp");
    LIST_ADD(CMD_LEVELORDER_P3NAMEJ, "p3namej");
    LIST_ADD(CMD_LEVELORDER_P3RUSH, "p3rush");
    LIST_ADD(CMD_LEVELORDER_P3SCORE, "p3score");
    LIST_ADD(CMD_LEVELORDER_P3SHOOT, "p3shoot");
    LIST_ADD(CMD_LEVELORDER_P3SMENU, "p3smenu");
    LIST_ADD(CMD_LEVELORDER_P4ICON, "p4icon");
    LIST_ADD(CMD_LEVELORDER_P4ICONW, "p4iconw");
    LIST_ADD(CMD_LEVELORDER_P4LIFE, "p4life");
    LIST_ADD(CMD_LEVELORDER_P4LIFEN, "p4lifen");
    LIST_ADD(CMD_LEVELORDER_P4LIFEX, "p4lifex");
    LIST_ADD(CMD_LEVELORDER_P4MP, "p4mp");
    LIST_ADD(CMD_LEVELORDER_P4NAMEJ, "p4namej");
    LIST_ADD(CMD_LEVELORDER_P4RUSH, "p4rush");
    LIST_ADD(CMD_LEVELORDER_P4SCORE, "p4score");
    LIST_ADD(CMD_LEVELORDER_P4SHOOT, "p4shoot");
    LIST_ADD(CMD_LEVELORDER_P4SMENU, "p4smenu");
    LIST_ADD(CMD_LEVELORDER_PAUSEOFFSET, "pauseoffset");
    LIST_ADD(CMD_LEVELORDER_RUSH, "rush");
    LIST_ADD(CMD_LEVELORDER_RUSHBONUS, "rushbonus");
    LIST_ADD(CMD_LEVELORDER_SCBONUSES, "scbonuses");
    LIST_ADD(CMD_LEVELORDER_SCENE, "scene");
    LIST_ADD(CMD_LEVELORDER_SCOREFORMAT, "scoreformat");
    LIST_ADD(CMD_LEVELORDER_SELECT, "select");
    LIST_ADD(CMD_LEVELORDER_SET, "set");
    LIST_ADD(CMD_LEVELORDER_SHOWCOMPLETE, "showcomplete");
    LIST_ADD(CMD_LEVELORDER_SHOWRUSHBONUS, "showrushbonus");
    LIST_ADD(CMD_LEVELORDER_SINGLE, "single");
    LIST_ADD(CMD_LEVELORDER_SKIPSELECT, "skipselect");
    LIST_ADD(CMD_LEVELORDER_SKIPTOSET, "skiptoset");
    LIST_ADD(CMD_LEVELORDER_SPAWNOVERRIDE, "spawnoverride");
    LIST_ADD(CMD_LEVELORDER_TIMEICON, "timeicon");
    LIST_ADD(CMD_LEVELORDER_TIMELOC, "timeloc");
    LIST_ADD(CMD_LEVELORDER_TOTALSCORE, "totalscore");
    LIST_ADD(CMD_LEVELORDER_TYPEMP, "typemp");
    LIST_ADD(CMD_LEVELORDER_UNLOCKBG, "unlockbg");
    LIST_ADD(CMD_LEVELORDER_Z, "z");

#undef LIST_ADD
    return result;
}
