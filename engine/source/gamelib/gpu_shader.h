/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 */

#ifndef OPENBOR_GPU_SHADER_H
#define OPENBOR_GPU_SHADER_H

/*
 * GLSL 1.20 compatibility-profile programs. Logical row zero is GL row zero;
 * presentation applies the final screen orientation. Vertex inputs carry
 * gl_Vertex.xy as destination pixel edges and gl_MultiTexCoord0.xy as raw
 * source texel coordinates. Each transform selects its sampling policy before
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
 *                                      3=continuous boundary-stable floor+clip
 *   int blendTableMask                 bit (mode-1) selects an uploaded LUT
 *   vec3 fillColor, tintColor, channelOpacity  normalized RGB components
 *
 * All samplers use nearest filtering. Indexed source and mask are LUMINANCE8;
 * palette is 256x1 RGBA8. Truecolor screen pixels carry RGB; drawmethod's color
 * key and separate mask choose coverage. The uploaded RGBA alpha channel is
 * ignored, allowing black screen pixels to remain opaque when keying is off.
 * blendTableTexture is a 256x1536 LUMINANCE8 atlas. Each 256-row slice is one
 * blend mode, row=source byte, column=destination byte. A zero blendTableMask
 * selects the built-in equations and needs no atlas upload.
 * Plane/rotation sampling snaps values within 0.0001 texel of an integer
 * boundary to that integer. This stabilizes texel selection and wrapping
 * after GPU interpolation while keeping pixel work inside the shader.
 *
 * destinationTexture must be a distinct, completed snapshot of the target.
 * It must never be the texture currently attached for rendering. Hardware
 * blending must be disabled because the fragment shader performs compositing.
 * The backend only needs a destination snapshot for mask or blend operations;
 * opaque source copies and source-only tints leave that sampler unused.
 */
extern const char *gpu_vertex_shader_source;
extern const char *gpu_fragment_shader_source;

/* Standalone reference helpers used by regression tests and shader diagnostics. */
unsigned gpu_shader_blend_reference(int mode, unsigned source, unsigned destination);
unsigned gpu_shader_channel_reference(unsigned source, unsigned destination, unsigned opacity);

#endif
