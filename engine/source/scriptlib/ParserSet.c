/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include "ParserSet.h"
#include "FirstFollow.h"
#include <string.h>

void ParserSet_Buildup(ParserSet *pset)
{
    MY_TOKEN_TYPE *ptr = NULL;
    int i = 0;

    ptr = first;
    pset->FirstSet[i] = ptr;
    do
    {
        ptr++;
        if((*ptr) == END_OF_TOKENS)
        {
            i++;
            pset->FirstSet[i] = ++ptr;
        }
    }
    while(i < NUMPRODUCTIONS);
    i = 0;
    ptr = follow;
    pset->FollowSet[i] = ptr;
    do
    {
        ptr++;
        if((*ptr) == END_OF_TOKENS)
        {
            i++;
            pset->FollowSet[i] = ++ptr;
        }
    }
    while(i < NUMPRODUCTIONS);
}



void ParserSet_Clear(ParserSet *pset)
{
    memset(pset, 0, sizeof(ParserSet));
}


BOOL ParserSet_First(ParserSet *pset, PRODUCTION theProduction, MY_TOKEN_TYPE theToken)
{
    MY_TOKEN_TYPE *ptr = NULL;
    ptr = pset->FirstSet[theProduction];
    while((*ptr) != END_OF_TOKENS)
    {
        if(*ptr == theToken)
        {
            return TRUE;
        }
        ptr++;
    }
    return FALSE;
}

BOOL ParserSet_Follow(ParserSet *pset, PRODUCTION theProduction, MY_TOKEN_TYPE theToken)
{
    MY_TOKEN_TYPE *ptr = NULL;
    ptr = pset->FollowSet[theProduction];
    while((*ptr) != END_OF_TOKENS)
    {
        if(*ptr == theToken)
        {
            return TRUE;
        }
        ptr++;
    }
    return FALSE;
}

