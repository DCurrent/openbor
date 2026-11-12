/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 */

#include "gpu_shader.h"

const char *gpu_vertex_shader_source =
    "#version 120\n"
    "uniform vec2 targetSize;\n"
    "varying vec2 sourcePosition;\n"
    "/*\n"
    " * Damon V. Caskey\n"
    " * 2026-11-12\n"
    " *\n"
    " * Map logical pixel edges to the target, preserving raw source texels.\n"
    " */\n"
    "void main()\n"
    "{\n"
    "    /*\n"
    "     * Geometry arrives in logical destination pixels. Normalize each\n"
    "     * axis to [-1, 1] while retaining the shared screen/texture row origin.\n"
    "     */\n"
    "    gl_Position = vec4(gl_Vertex.xy * 2.0 / targetSize - 1.0, 0.0, 1.0);\n"
    "\n"
    "    /*\n"
    "     * Preserve source coordinates in texels through interpolation. The\n"
    "     * fragment stage must select a texel before applying palette lookup,\n"
    "     * color-key coverage, or the transform's wrapping rules.\n"
    "     */\n"
    "    sourcePosition = gl_MultiTexCoord0.xy;\n"
    "}\n";

const char *gpu_fragment_shader_source =
    "#version 120\n"
    "uniform vec2 targetSize;\n"
    "uniform vec2 sourceSize;\n"
    "uniform sampler2D sourceTexture;\n"
    "uniform sampler2D paletteTexture;\n"
    "uniform sampler2D maskTexture;\n"
    "uniform sampler2D destinationTexture;\n"
    "uniform sampler2D blendTableTexture;\n"
    "uniform int indexedSource;\n"
    "uniform int maskEnabled;\n"
    "uniform int colorKey;\n"
    "uniform int fillEnabled;\n"
    "uniform int hasDestination;\n"
    "uniform int wrapSource;\n"
    "uniform int blendMode;\n"
    "uniform int tintMode;\n"
    "uniform int blendTableMask;\n"
    "uniform vec3 fillColor;\n"
    "uniform vec3 tintColor;\n"
    "uniform vec3 channelOpacity;\n"
    "varying vec2 sourcePosition;\n"
    "/*\n"
    " * Damon V. Caskey\n"
    " * 2026-11-12\n"
    " *\n"
    " * Recover exact byte values from normalized nearest-neighbor samples.\n"
    " */\n"
    "vec3 bytes(vec3 value)\n"
    "{\n"
    "    /*\n"
    "     * Recover the nearest stored byte before using it as a palette/LUT\n"
    "     * address or an integer-domain blend operand. This also bounds the\n"
    "     * values supplied through normalized material uniforms.\n"
    "     */\n"
    "    return floor(clamp(value, 0.0, 1.0) * 255.0 + 0.5);\n"
    "}\n"
    "/*\n"
    " * Damon V. Caskey\n"
    " * 2026-11-12\n"
    " *\n"
    " * Evaluate one existing RGB blend equation in its original byte domain.\n"
    " * Uploaded tables retain custom channel mappings when present.\n"
    " */\n"
    "vec3 blend(int mode, vec3 source, vec3 destination)\n"
    "{\n"
    "    /* Mode zero leaves the prepared source unchanged for an opaque copy. */\n"
    "    if(mode < 1 || mode > 6)\n"
    "    {\n"
    "        return source;\n"
    "    }\n"
    "\n"
    "    /*\n"
    "     * Each enabled mode owns a 256-row atlas slice. Destination bytes\n"
    "     * select columns and source bytes select rows, matching the CPU LUT\n"
    "     * address (source << 8) | destination independently for RGB.\n"
    "     */\n"
    "    if(blendTableMask != 0)\n"
    "    {\n"
    "        float flag = mod(floor(float(blendTableMask) / pow(2.0, float(mode - 1))), 2.0);\n"
    "        if(flag > 0.5)\n"
    "        {\n"
    "            vec3 tx = (destination + 0.5) / 256.0;\n"
    "            vec3 ty = (source + float(mode - 1) * 256.0 + 0.5) / 1536.0;\n"
    "            return bytes(vec3(texture2D(blendTableTexture, vec2(tx.r, ty.r)).r,\n"
    "                              texture2D(blendTableTexture, vec2(tx.g, ty.g)).r,\n"
    "                              texture2D(blendTableTexture, vec2(tx.b, ty.b)).r));\n"
    "        }\n"
    "    }\n"
    "\n"
    "    /*\n"
    "     * Built-in equations retain their byte-domain divisors. The small\n"
    "     * division guard keeps an integral quotient from rounding just below\n"
    "     * its intended value before floor converts it back to a byte.\n"
    "     */\n"
    "    if(mode == 1)\n"
    "    {\n"
    "        return 255.0 - floor((255.0-source)*(255.0-destination)/255.0 + 0.0001);\n"
    "    }\n"
    "    if(mode == 2)\n"
    "    {\n"
    "        return floor(source * destination / 256.0);\n"
    "    }\n"
    "    if(mode == 3 || mode == 4)\n"
    "    {\n"
    "        /*\n"
    "         * Overlay chooses its branch from the destination; hardlight\n"
    "         * chooses from the source. The channel-wise selector allows\n"
    "         * bright and dark channels to follow their own branch.\n"
    "         */\n"
    "        vec3 selector = (mode == 3) ? destination : source;\n"
    "        vec3 other = (mode == 3) ? source : destination;\n"
    "        vec3 low = floor(2.0 * selector * other / 256.0);\n"
    "        vec3 high = 255.0 - floor((511.0-2.0*selector)*(255.0-other)/255.0 + 0.0001);\n"
    "        return mix(low, high, step(vec3(128.0), selector));\n"
    "    }\n"
    "    if(mode == 5)\n"
    "    {\n"
    "        /* Source bytes stop at 255, so the dodge divisor remains positive. */\n"
    "        return min(vec3(255.0), floor(destination*256.0/(256.0-source) + 0.0001));\n"
    "    }\n"
    "\n"
    "    /* The remaining valid mode is the half-intensity average. */\n"
    "    return floor((source + destination) / 2.0);\n"
    "}\n"
    "/*\n"
    " * Damon V. Caskey\n"
    " * 2026-11-12\n"
    " *\n"
    " * Match the engine's separate alpha-mask/per-channel opacity equation.\n"
    " */\n"
    "vec3 opacityBlend(vec3 source, vec3 destination, vec3 opacity)\n"
    "{\n"
    "    /*\n"
    "     * Existing mask/channel opacity uses a 255 complement and a 256\n"
    "     * divisor. Keep both terms so GPU compositing preserves that rule\n"
    "     * at intermediate values and at the opacity endpoints.\n"
    "     */\n"
    "    return floor((source*opacity + destination*(255.0-opacity)) / 256.0);\n"
    "}\n"
    "/*\n"
    " * Damon V. Caskey\n"
    " * 2026-11-12\n"
    " *\n"
    " * Keep mathematically integral texture coordinates stable after floating\n"
    " * interpolation; do not retain accumulated software coefficient drift.\n"
    " */\n"
    "vec2 stableBoundary(vec2 position)\n"
    "{\n"
    "    /*\n"
    "     * A coordinate just below an integer can choose a neighboring texel\n"
    "     * or wrap to the opposite texture edge. Restrict stabilization to\n"
    "     * the documented 0.0001-texel band and retain other fractions.\n"
    "     */\n"
    "    vec2 nearest = floor(position + 0.5);\n"
    "    return mix(position, nearest, vec2(lessThan(abs(position-nearest), vec2(0.0001))));\n"
    "}\n"
    "/*\n"
    " * Damon V. Caskey\n"
    " * 2026-11-12\n"
    " *\n"
    " * Sample indexed or truecolor content, honor zero-key coverage before\n"
    " * fill, and composite against a separate completed target snapshot.\n"
    " */\n"
    "void main()\n"
    "{\n"
    "    vec2 pixel;\n"
    "    if(wrapSource == 1)\n"
    "    {\n"
    "        /*\n"
    "         * Plane coordinates may extend beyond either source edge.\n"
    "         * Stabilize their boundaries, truncate toward zero like the\n"
    "         * plane sampler, then wrap into the positive texture range.\n"
    "         */\n"
    "        vec2 position = stableBoundary(sourcePosition);\n"
    "        pixel = sign(position) * floor(abs(position));\n"
    "        pixel = mod(pixel, sourceSize);\n"
    "    }\n"
    "    else\n"
    "    {\n"
    "        /*\n"
    "         * Scale coordinates belong to an 8.8 fixed-point grid. Recover\n"
    "         * that grid after interpolation; rotation instead retains its\n"
    "         * continuous fractions and stabilizes only integer boundaries.\n"
    "         */\n"
    "        vec2 position;\n"
    "        if(wrapSource == 2)\n"
    "        {\n"
    "            position = floor(sourcePosition*256.0 + 0.5)/256.0;\n"
    "        }\n"
    "        else if(wrapSource == 3)\n"
    "        {\n"
    "            position = stableBoundary(sourcePosition);\n"
    "        }\n"
    "        else\n"
    "        {\n"
    "            position = sourcePosition;\n"
    "        }\n"
    "        pixel = floor(position);\n"
    "        if(any(lessThan(pixel, vec2(0.0))) || any(greaterThanEqual(pixel, sourceSize)))\n"
    "        {\n"
    "            discard;\n"
    "        }\n"
    "    }\n"
    "\n"
    "    /*\n"
    "     * Sample the selected texel at its center. Indexed coverage depends\n"
    "     * on the raw index before palette lookup: a visible palette entry\n"
    "     * may legitimately contain black. Truecolor coverage uses RGB zero.\n"
    "     */\n"
    "    vec2 uv = (pixel + 0.5) / sourceSize;\n"
    "    vec3 source = bytes(texture2D(sourceTexture, uv).rgb);\n"
    "    if(indexedSource != 0)\n"
    "    {\n"
    "        if(colorKey != 0 && source.r == 0.0)\n"
    "        {\n"
    "            discard;\n"
    "        }\n"
    "        source = bytes(texture2D(paletteTexture, vec2((source.r + 0.5)/256.0, 0.5)).rgb);\n"
    "    }\n"
    "    else if(colorKey != 0 && all(equal(source, vec3(0.0))))\n"
    "    {\n"
    "        discard;\n"
    "    }\n"
    "\n"
    "    /* Fill replaces visible color after source coverage has been decided. */\n"
    "    if(fillEnabled != 0)\n"
    "    {\n"
    "        source = bytes(fillColor);\n"
    "    }\n"
    "\n"
    "    /*\n"
    "     * Destination-dependent effects read a completed target snapshot.\n"
    "     * Opaque copies and source-only tints can skip this texture read.\n"
    "     */\n"
    "    vec3 destination = vec3(0.0);\n"
    "    if(hasDestination != 0)\n"
    "    {\n"
    "        destination = bytes(texture2D(destinationTexture, gl_FragCoord.xy/targetSize).rgb);\n"
    "    }\n"
    "\n"
    "    /*\n"
    "     * Match material precedence: a mask supplies opacity directly; tint\n"
    "     * prepares source color before the selected destination blend; RGB\n"
    "     * channel opacity replaces half blending only when tint is absent.\n"
    "     */\n"
    "    vec3 result;\n"
    "    if(maskEnabled != 0)\n"
    "    {\n"
    "        float opacity = floor(texture2D(maskTexture, uv).r * 255.0 + 0.5);\n"
    "        result = opacityBlend(source, destination, vec3(opacity));\n"
    "    }\n"
    "    else if(tintMode != 0)\n"
    "    {\n"
    "        result = blend(blendMode, blend(tintMode, bytes(tintColor), source), destination);\n"
    "    }\n"
    "    else if(blendMode == 6 && any(lessThan(bytes(channelOpacity), vec3(255.0))))\n"
    "    {\n"
    "        result = opacityBlend(source, destination, bytes(channelOpacity));\n"
    "    }\n"
    "    else\n"
    "    {\n"
    "        result = blend(blendMode, source, destination);\n"
    "    }\n"
    "\n"
    "    /*\n"
    "     * Store RGB byte results as normalized color. Screen alpha stays\n"
    "     * opaque because future draws choose their own color-key coverage.\n"
    "     */\n"
    "    gl_FragColor = vec4(clamp(result, 0.0, 255.0)/255.0, 1.0);\n"
    "}\n";

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Expose the canonical byte equations without SDL/OpenGL for regression tests.
 * Mode numbers are drawmethod alpha/tint values, not zero-based BLEND_* IDs.
 */
unsigned gpu_shader_blend_reference(int mode, unsigned source, unsigned destination)
{
    unsigned result;

    /* Match texture sampling's byte domain even when callers pass packed words. */
    source &= 255u;
    destination &= 255u;

    /* Keep each mode separate so tests can compare it with its CPU counterpart. */
    switch(mode)
    {
    case 1:
    {
        return 255u - ((255u - source) * (255u - destination) / 255u);
    }
    case 2:
    {
        return source * destination / 256u;
    }
    case 3:
    {
        return destination < 128u ? 2u * destination * source / 256u
             : 255u - ((511u - 2u * destination) * (255u - source) / 255u);
    }
    case 4:
    {
        return source < 128u ? 2u * source * destination / 256u
             : 255u - ((511u - 2u * source) * (255u - destination) / 255u);
    }
    case 5:
    {
        result = destination * 256u / (256u - source);
        return result > 255u ? 255u : result;
    }
    case 6:
    {
        return (source + destination) / 2u;
    }
    default:
    {
        return source;
    }
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Keep the existing 255-complement/256-divisor opacity rule visible and tested.
 * Changing it is a separate visual-behavior decision, not an implicit GPU change.
 */
unsigned gpu_shader_channel_reference(unsigned source, unsigned destination, unsigned opacity)
{
    return ((source & 255u) * (opacity & 255u)
         + (destination & 255u) * (255u - (opacity & 255u))) / 256u;
}
