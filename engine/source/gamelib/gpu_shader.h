/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 */

#ifndef OPENBOR_GPU_SHADER_H
#define OPENBOR_GPU_SHADER_H

/*
 * GLSL 1.20 compatibility-profile programs. Logical row zero is GL row zero;
 * presentation, not drawing, is responsible for the final screen orientation.
 * Vertex inputs are gl_Vertex.xy (destination pixel edges) and
 * gl_MultiTexCoord0.xy (un-normalized source coordinates).
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
 *   vec3 fillColor, tintColor, channelOpacity  normalized RGB, not packed words
 *
 * All samplers use nearest filtering. Indexed source and mask are LUMINANCE8;
 * palette is 256x1 RGBA8. Source RGBA alpha is deliberately ignored: OpenBOR
 * truecolor screens are RGB with optional zero color key, not RGBA images.
 * blendTableTexture is a 256x1536 LUMINANCE8 atlas. Each 256-row slice is one
 * blend mode, row=source byte, column=destination byte. A zero blendTableMask
 * selects the built-in equations and needs no atlas upload.
 * Plane/rotation sampling snaps values within 0.0001 texel of an integer boundary to
 * that integer. This avoids unstable wrapping from accumulated CPU float
 * drift or GPU interpolation error, without per-pixel work on the CPU.
 *
 * destinationTexture must be a distinct, completed snapshot of the target.
 * It must never be the texture currently attached for rendering. Hardware
 * blending must be disabled because the fragment shader performs compositing.
 */
extern const char *gpu_vertex_shader_source;
extern const char *gpu_fragment_shader_source;

/* Standalone reference helpers used by regression tests and shader diagnostics. */
unsigned gpu_shader_blend_reference(int mode, unsigned source, unsigned destination);
unsigned gpu_shader_channel_reference(unsigned source, unsigned destination, unsigned opacity);

#endif
