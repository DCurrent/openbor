/*
 * OpenBOR - http://www.chronocrash.com
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Damon V. Caskey
 * 2026-11-20
 * Use one bounded curve contract in script setters and native geometry.
 * Ordered comparisons also reject NaN and either floating-point infinity.
 */
#ifndef OPENBOR_DRAWMETHOD_CURVE_H
#define OPENBOR_DRAWMETHOD_CURVE_H

#include "types.h"

static inline int drawmethod_curve_valid(const s_drawmethod_curve *curve)
{
    return curve &&
        curve->angle >= -180.0f && curve->angle <= 180.0f &&
        curve->axis >= -360.0f && curve->axis <= 360.0f &&
        curve->plane >= -360.0f && curve->plane <= 360.0f &&
        curve->focus >= -4.0f && curve->focus <= 5.0f &&
        curve->extent >= 0.01f && curve->extent <= 16.0f &&
        curve->perspective >= 0.0f && curve->perspective <= 64.0f &&
        curve->tilt >= -89.0f && curve->tilt <= 89.0f &&
        curve->profile >= DC_CURVE_PROFILE_UNIFORM &&
        curve->profile <= DC_CURVE_PROFILE_SMOOTH &&
        curve->scrollx >= -10000000.0f && curve->scrollx <= 10000000.0f &&
        curve->scrolly >= -10000000.0f && curve->scrolly <= 10000000.0f &&
        curve->wrap >= DC_CURVE_WRAP_CLIP && curve->wrap <= DC_CURVE_WRAP_REPEAT;
}

#endif
