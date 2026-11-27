/*
 * OpenBOR - https://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 *
 * Damon V. Caskey
 * 2026-11-27
 * Lossless terrain samples and ordinary indexed/RGB terrain artwork.
 */
#ifndef OPENBOR_MESHMAP_IMAGE_H
#define OPENBOR_MESHMAP_IMAGE_H

#include <stddef.h>
#include <stdint.h>
#include "types.h"
#include "meshmap.h"

/* Outputs change only after the complete PNG, including its ending chunks,
 * has validated. The caller owns successful samples (free) or artwork
 * (freescreen). Geometry requires grayscale16; artwork requires indexed8 or
 * RGB24. PNG display gamma never modifies numerical geometry samples. */
int meshmap_image_load_samples(const char *filename, const char *packfilename,
                              uint16_t **samples, unsigned *width, unsigned *height,
                              char *error, size_t error_capacity);
int meshmap_image_load_artwork(const char *filename, const char *packfilename,
                              s_screen **artwork, char *error, size_t error_capacity);
int meshmap_image_replace_geometry(s_meshmap *map, const char *filename,
                                   const char *packfilename, char *error, size_t error_capacity);
/* An empty artwork path removes only the optional picture. */
int meshmap_image_replace_artwork(s_meshmap *map, const char *filename,
                                  const char *packfilename, char *error, size_t error_capacity);

/* Independent resource transactions for selected level declarations.
 * Geometry includes its proposed configuration and preserves the texture.
 * Texture changes preserve geometry; an empty path removes the texture.
 * Failed input leaves an existing map intact and creates no new map. */
int meshmap_image_apply_geometry(s_meshmap_store *store, uint32_t id,
                                 const s_meshmap_config *config,
                                 const char *geometry_filename,
                                 const char *packfilename,
                                 char *error, size_t error_capacity);
int meshmap_image_apply_texture(s_meshmap_store *store, uint32_t id,
                                const char *texture_filename,
                                const char *packfilename,
                                char *error, size_t error_capacity);

#endif
