/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 */

#ifndef OPENBOR_GPU_SHADER_H
#define OPENBOR_GPU_SHADER_H

/*
 * GLSL 1.20 compatibility-profile programs. Logical row zero is GL row zero;
 * presentation applies the final screen orientation. The vertexPosition and
 * vertexTexcoord attributes carry destination pixel edges and raw source
 * texel coordinates. Each transform selects its sampling policy before
 * the fragment stage evaluates source coverage and material compositing.
 *
 * Uniforms:
 *   vec2 targetSize, sourceSize
 *   sampler2D sourceTexture, paletteTexture, maskTexture, destinationTexture
 *   sampler2D blendTableTexture
 *   int indexedSource, maskEnabled, colorKey, fillEnabled, hasDestination
 *   int blendMode, tintMode             0=copy, 1..6=existing alpha modes
 *   int wrapSource                     0=floor+clip, 1=truncate+tile,
 *                                      2=8.8 fixed-point floor+clip,
 *                                      3=continuous boundary-stable floor+clip,
 *                                      4=affine, 5=legacy shear,
 *                                      6=water displacement, 7=prepared plane
 *   int blendTableMask                 bit (mode-1) selects an uploaded LUT
 *   vec3 fillColor, tintColor, channelOpacity  normalized RGB components
 *   GPU_COLOR_RULES optional variant:
 *     int chromaEnabled, substituteEnabled   five independent enabled bits
 *     int primitiveSource                   use fillColor for source keying
 *     vec3 chromaKeys[5]                     RGB bytes, after palette lookup
 *     vec3 substituteMatch[5], substituteValue[5]  final RGB byte mappings
 *   GPU_WATER optional variant:
 *     vec3 waterOrigin                      image left/top, first visible row
 *     vec3 waterWave                        first visible phase, step, amplitude
 *     float waterAnchor                     existing origin displacement
 *   GPU_PLANE optional variant:
 *     sampler2D planeRowsTexture             unit 5, one RGBA32F texel per row
 *     vec3 planeOrigin                      clipped left, image top, source U
 *     vec2 planeInfo                        horizontal center, row-map height
 *
 * All samplers use nearest filtering. Indexed source and mask are LUMINANCE8;
 * palette is a bounded RGBA8 atlas with 256 colors per row. Truecolor screen
 * pixels carry RGB; drawmethod's color
 * key and separate mask choose coverage. The uploaded RGBA alpha channel is
 * ignored, allowing black screen pixels to remain opaque when keying is off.
 * blendTableTexture is a 256x1536 LUMINANCE8 atlas. Each 256-row slice is one
 * blend mode, row=source byte, column=destination byte. A zero blendTableMask
 * selects the built-in equations and needs no atlas upload.
 * Plane/rotation sampling snaps values within 0.0001 texel of an integer
 * boundary to that integer. This stabilizes texel selection and wrapping
 * after GPU interpolation while keeping pixel work inside the shader.
 * Water retains quantized phase and integer displacement; the renderer only
 * selects direct phase evaluation within its documented approximation bound.
 * Prepared plane records contain reciprocal horizontal scale and an exact
 * wrapped source row, retaining sequential vertical stepping without a loop
 * per fragment. Placement and scrolling do not change the intrinsic row map.
 *
 * destinationTexture must be a distinct, completed snapshot of the target.
 * It must never be the texture currently attached for rendering. Hardware
 * blending must be disabled because the fragment shader performs compositing.
 * The backend only needs a destination snapshot for mask or blend operations;
 * opaque source copies and source-only tints leave that sampler unused.
 * Color rules alone do not require destination access. Chroma precedes fill
 * and effects; one Substitute mapping follows final byte quantization and
 * cannot repair discarded coverage or fully zero-opacity mask samples.
 */
extern const char *gpu_vertex_shader_source;
extern const char *gpu_fragment_shader_source;
/* Shared direct YUV reconstruction for screen targets and window output. */
extern const char *gpu_movie_fragment_shader_source;
/* Output filters share exact RGB8 equations across desktop GL and GLES2.
 * The body omits version/precision; complete variants supply GLSL 1.20 or
 * GLSL ES 1.00 with high precision. Uniforms: sourceTexture, sourceSize and
 * filterMode (1..6). sourcePosition carries original source pixel edges;
 * source row zero and output row zero both use texture coordinate v=0. */
extern const char *gpu_output_filter_fragment_shader_body;
extern const char *gpu_output_filter_fragment_shader_source;
extern const char *gpu_output_filter_fragment_shader_es_source;

/* Standalone reference helpers used by regression tests and shader diagnostics. */
unsigned gpu_shader_blend_reference(int mode, unsigned source, unsigned destination);
unsigned gpu_shader_channel_reference(unsigned source, unsigned destination, unsigned opacity);

#endif
