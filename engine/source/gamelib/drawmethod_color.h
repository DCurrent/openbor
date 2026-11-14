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
 * Identify opt-in color processing without changing disabled drawmethods.
 */
static inline int drawmethod_colors_active(const s_drawmethod *method)
{
    /* The normal enabled flag remains the owner of all drawmethod effects. */
    return method && (method->config & DRAWMETHOD_CONFIG_ENABLED) &&
           ((method->colors.chroma_enabled | method->colors.substitute_enabled) &
            DRAWMETHOD_COLOR_MASK);
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Test resolved source RGB before fill, lighting, masks or destination blends.
 */
static inline int drawmethod_chroma_matches(const s_drawmethod *method, unsigned color)
{
    unsigned slot;
    /* Packed screen colors reserve the high byte; key matching uses RGB only. */
    color &= 0xFFFFFFu;
    for(slot = 0; slot < DRAWMETHOD_COLOR_SLOTS; ++slot)
    {
        if((method->colors.chroma_enabled & (1u << slot)) &&
           (method->colors.chroma[slot] & 0xFFFFFFu) == color)
        {
            return 1;
        }
    }
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-14
 * Replace one completed RGB result. Replacements are never tested again.
 */
static inline unsigned drawmethod_substitute_color(const s_drawmethod *method, unsigned color)
{
    unsigned slot;
    /* Keep an unmatched packed value intact; comparisons ignore its high byte. */
    const unsigned rgb = color & 0xFFFFFFu;
    for(slot = 0; slot < DRAWMETHOD_COLOR_SLOTS; ++slot)
    {
        if((method->colors.substitute_enabled & (1u << slot)) &&
           (method->colors.substitute[slot].match & 0xFFFFFFu) == rgb)
        {
            return method->colors.substitute[slot].replacement & 0xFFFFFFu;
        }
    }
    return color;
}

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
