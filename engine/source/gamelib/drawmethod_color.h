/*
 * OpenBOR - https://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 */
#ifndef OPENBOR_DRAWMETHOD_COLOR_H
#define OPENBOR_DRAWMETHOD_COLOR_H

#include "types.h"

/*
 * Damon V. Caskey
 * 2026-11-14
 * Reject duplicate active matches and replacements that remain intercepted.
 */
static inline int drawmethod_substitutes_valid(const s_drawmethod_colors *colors)
{
    unsigned first, second;
    if(colors->substitute_enabled & ~DRAWMETHOD_COLOR_MASK)
    {
        return 0;
    }

    /* Disabled slots retain their data but cannot conflict with live rules. */
    for(first = 0; first < DRAWMETHOD_COLOR_SLOTS; ++first)
    {
        if(!(colors->substitute_enabled & (1u << first)))
        {
            continue;
        }
        for(second = 0; second < DRAWMETHOD_COLOR_SLOTS; ++second)
        {
            if(!(colors->substitute_enabled & (1u << second)))
            {
                continue;
            }
            if((colors->substitute[first].replacement & 0xFFFFFFu) ==
               (colors->substitute[second].match & 0xFFFFFFu) ||
               (first != second &&
                (colors->substitute[first].match & 0xFFFFFFu) ==
                (colors->substitute[second].match & 0xFFFFFFu)))
            {
                return 0;
            }
        }
    }
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Publish a complete match/replacement pair only after whole-list validation.
 */
static inline int drawmethod_set_substitute(s_drawmethod *method, unsigned slot,
                                          unsigned match, unsigned replacement)
{
    s_drawmethod_colors next;
    if(!method || slot >= DRAWMETHOD_COLOR_SLOTS ||
       match > 0xFFFFFFu || replacement > 0xFFFFFFu)
    {
        return 0;
    }
    next = method->colors;
    next.substitute[slot].match = match;
    next.substitute[slot].replacement = replacement;
    next.substitute_enabled |= 1u << slot;
    if(!drawmethod_substitutes_valid(&next))
    {
        return 0;
    }
    method->colors = next;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Enable or disable stored substitutions while validating the resulting list.
 */
static inline int drawmethod_set_substitute_enabled(s_drawmethod *method, unsigned mask)
{
    s_drawmethod_colors next;
    if(!method || (mask & ~DRAWMETHOD_COLOR_MASK))
    {
        return 0;
    }
    next = method->colors;
    next.substitute_enabled = mask;
    if(!drawmethod_substitutes_valid(&next))
    {
        return 0;
    }
    method->colors = next;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Set and enable one palette-resolved RGB chroma key, including numeric black.
 */
static inline int drawmethod_set_chroma(s_drawmethod *method, unsigned slot, unsigned color)
{
    if(!method || slot >= DRAWMETHOD_COLOR_SLOTS || color > 0xFFFFFFu)
    {
        return 0;
    }
    method->colors.chroma[slot] = color;
    method->colors.chroma_enabled |= 1u << slot;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Change chroma coverage independently of stored colors and substitute rules.
 */
static inline int drawmethod_set_chroma_enabled(s_drawmethod *method, unsigned mask)
{
    if(!method || (mask & ~DRAWMETHOD_COLOR_MASK))
    {
        return 0;
    }
    method->colors.chroma_enabled = mask;
    return 1;
}

#endif
