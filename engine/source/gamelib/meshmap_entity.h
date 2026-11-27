/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 *
 * The entity adapter connects native terrain queries to the existing movement
 * and gravity lifecycle. Configuration belongs to each entity; map IDs and
 * generations identify contacts without retaining pointers into removable maps.
 */
#ifndef OPENBOR_MESHMAP_ENTITY_H
#define OPENBOR_MESHMAP_ENTITY_H

#include <stdint.h>
#include <float.h>

struct entity;

#define MESHMAP_PLANE_NONE (-1)
#define MESHMAP_ENTITY_VOID_BASE (-FLT_MAX)

typedef struct s_meshmap_entity
{
    int enabled;
    float max_slope_degrees;
    float max_step_height;
    int movement_plane; /* -1 is 3D; otherwise the fixed world axis. */
    int support_id;
    uint32_t support_group;
    uint64_t support_generation;
    float support_y;
    int drop_id;
    uint32_t drop_group;
    uint64_t drop_generation;
    float drop_y;
} s_meshmap_entity;

void meshmap_entity_initialize(struct entity *entity);
int meshmap_entity_drop(struct entity *entity);
void meshmap_entity_adjust_base(struct entity *entity, float legacy_base);
void meshmap_entity_land(struct entity *entity);
float meshmap_entity_vertical(struct entity *entity, float destination_y);
void meshmap_entity_move(struct entity *entity, float *move_x, float *move_z);
int meshmap_entity_test_move(const struct entity *entity,
                            float start_x, float start_z, float x, float z);
float meshmap_entity_check_base(struct entity *entity, float x, float z,
                               float ceiling, float legacy_base);
float meshmap_entity_pit_depth(const struct entity *entity);

#endif
