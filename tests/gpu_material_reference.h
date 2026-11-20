/*
 * Damon V. Caskey
 * 2026-11-20
 * Scalar byte equations belong only to independent regression expectations.
 * Production GPU shaders do not expose or call these CPU reference helpers.
 */
#ifndef OPENBOR_TEST_GPU_MATERIAL_REFERENCE_H
#define OPENBOR_TEST_GPU_MATERIAL_REFERENCE_H

enum
{
    REFERENCE_CHANNEL_MAX = 255,
    REFERENCE_CHANNEL_SCALE = REFERENCE_CHANNEL_MAX + 1,
    REFERENCE_CHANNEL_MIDPOINT = REFERENCE_CHANNEL_SCALE / 2,
    REFERENCE_CHANNEL_DOUBLE_RANGE = 2 * REFERENCE_CHANNEL_SCALE - 1
};

/*
 * Damon V. Caskey
 * 2026-11-20
 * Retain canonical alpha/tint byte equations as a test-only oracle. Mode IDs
 * are drawmethod values, one greater than the blend-table enumeration. The
 * exhaustive fixture independently compares all byte pairs with table math.
 */
static unsigned reference_blend_byte(int mode, unsigned source, unsigned destination)
{
    unsigned result;
    source &= REFERENCE_CHANNEL_MAX;
    destination &= REFERENCE_CHANNEL_MAX;
    switch(mode)
    {
    case BLEND_SCREEN + 1:
        return REFERENCE_CHANNEL_MAX - ((REFERENCE_CHANNEL_MAX - source) *
               (REFERENCE_CHANNEL_MAX - destination) / REFERENCE_CHANNEL_MAX);
    case BLEND_MULTIPLY + 1:
        return source * destination / REFERENCE_CHANNEL_SCALE;
    case BLEND_OVERLAY + 1:
        return destination < REFERENCE_CHANNEL_MIDPOINT ? 2u * destination * source / REFERENCE_CHANNEL_SCALE
             : REFERENCE_CHANNEL_MAX - ((REFERENCE_CHANNEL_DOUBLE_RANGE - 2u * destination) *
               (REFERENCE_CHANNEL_MAX - source) / REFERENCE_CHANNEL_MAX);
    case BLEND_HARDLIGHT + 1:
        return source < REFERENCE_CHANNEL_MIDPOINT ? 2u * source * destination / REFERENCE_CHANNEL_SCALE
             : REFERENCE_CHANNEL_MAX - ((REFERENCE_CHANNEL_DOUBLE_RANGE - 2u * source) *
               (REFERENCE_CHANNEL_MAX - destination) / REFERENCE_CHANNEL_MAX);
    case BLEND_DODGE + 1:
        result = destination * REFERENCE_CHANNEL_SCALE / (REFERENCE_CHANNEL_SCALE - source);
        return result > REFERENCE_CHANNEL_MAX ? REFERENCE_CHANNEL_MAX : result;
    case BLEND_HALF + 1:
        return (source + destination) / 2u;
    default:
        return source;
    }
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Preserve the established 255-complement/256-divisor opacity convention.
 * Mask each argument to its channel byte independently of packed RGB words.
 */
static unsigned reference_channel_byte(unsigned source, unsigned destination, unsigned opacity)
{
    return ((source & REFERENCE_CHANNEL_MAX) * (opacity & REFERENCE_CHANNEL_MAX)
         + (destination & REFERENCE_CHANNEL_MAX) * (REFERENCE_CHANNEL_MAX -
             (opacity & REFERENCE_CHANNEL_MAX))) / REFERENCE_CHANNEL_SCALE;
}

#endif
