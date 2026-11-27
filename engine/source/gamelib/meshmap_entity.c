/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 *
 * Native terrain participates at the same points as legacy floors, walls and
 * platform entities: base selection, vertical movement, and lateral movement.
 * This adapter keeps engine animation/event policy outside the geometry core.
 */
#include "openbor.h"
#include <math.h>
#include <string.h>

extern s_level *level;
extern s_global_config global_config;

#define TERRAIN_SKIN 0.01
#define TERRAIN_REST_DISTANCE 0.1

static int terrain_enabled(const entity *e)
{
    return e && e->meshmap.enabled && level && level->meshmaps &&
           level->meshmaps->map_count;
}

static s_meshmap_vec3 terrain_position(const entity *e)
{
    s_meshmap_vec3 p = {e->position.x, e->position.y, e->position.z};
    return p;
}

static double terrain_height(const entity *e)
{
    double height = e->animation && e->animation->size.y
        ? e->animation->size.y : e->modeldata.size.y;
    return height > 0.0 ? height : 0.0;
}

/*
 * BODY admission is opt-in. Resolve the active frame's explicit body boxes
 * once, using the same normalized coordinates and facing as hit detection.
 * A model's size supplies the fallback for frames without an active body box.
 * No velocity or prospective movement participates in this calculation.
 */
static s_meshmap_bounds terrain_body(const entity *e)
{
    s_meshmap_bounds result;
    const double half_x = fabs((double)e->modeldata.size.x) * 0.5;
    /* Models have a planar fallback size. Depth comes from an explicit body
     * box when one exists; a missing depth remains a zero-width Z interval. */
    const double half_z = 0.0;
    const s_meshmap_vec3 p = terrain_position(e);
    s_collision_collection *collection = NULL;
    unsigned i;
    int found = 0;

    result.min = (s_meshmap_vec3){p.x - half_x, p.y, p.z - half_z};
    result.max = (s_meshmap_vec3){p.x + half_x, p.y + terrain_height(e), p.z + half_z};
    if(e->animation && e->animation->collision_body &&
       e->animpos < e->animation->numframes)
        collection = e->animation->collision_body[e->animpos];
    if(!collection) return result;

    for(i = 0; i < MAX_COLLISION_BOXES_PER_FRAME; ++i)
    {
        const s_collision_instance *body;
        const s_hitbox *c;
        s_meshmap_bounds b;
        if(!(collection->active_status & (UINT64_C(1) << i))) continue;
        body = collection->slots[i];
        if(!body) continue;
        c = &body->coords;
        if(e->direction == DIRECTION_LEFT)
        {
            b.min.x = p.x - c->width;
            b.max.x = p.x - c->x;
        }
        else
        {
            b.min.x = p.x + c->x;
            b.max.x = p.x + c->width;
        }
        b.min.y = p.y - c->height;
        b.max.y = p.y - c->y;
        b.min.z = p.z - c->z_background;
        b.max.z = p.z + c->z_foreground;
        if(b.min.x > b.max.x || b.min.y > b.max.y || b.min.z > b.max.z) continue;
        if(!found) result = b;
        else
        {
            result.min.x = fmin(result.min.x, b.min.x);
            result.min.y = fmin(result.min.y, b.min.y);
            result.min.z = fmin(result.min.z, b.min.z);
            result.max.x = fmax(result.max.x, b.max.x);
            result.max.y = fmax(result.max.y, b.max.y);
            result.max.z = fmax(result.max.z, b.max.z);
        }
        found = 1;
    }
    return result;
}

static s_meshmap_query terrain_query(const entity *e)
{
    s_meshmap_query query;
    meshmap_query_default(&query, terrain_position(e));
    query.required_flags = MESHMAP_COLLIDE;
    query.slope_limit = e->meshmap.max_slope_degrees;
    query.movement_plane = e->meshmap.movement_plane;
    if(meshmap_store_uses_body_gates(level->meshmaps))
        query.body = terrain_body(e);
    if(e->meshmap.drop_id >= 0)
    {
        query.flags |= MESHMAP_QUERY_IGNORE_DROP_GROUP;
        query.ignore_group = e->meshmap.drop_group;
        query.ignore_id = (uint32_t)e->meshmap.drop_id;
        query.ignore_generation = e->meshmap.drop_generation;
    }
    return query;
}

static void terrain_clear_support(entity *e)
{
    e->meshmap.support_id = -1;
    e->meshmap.support_group = MESHMAP_GROUP_NONE;
    e->meshmap.support_generation = 0;
}

static void terrain_set_support(entity *e, const s_meshmap_hit *hit)
{
    e->meshmap.support_id = (int)hit->id;
    e->meshmap.support_group = hit->support_group;
    e->meshmap.support_generation = hit->generation;
    e->meshmap.support_y = (float)hit->position.y;
}

void meshmap_entity_initialize(entity *e)
{
    if(!e) return;
    memset(&e->meshmap, 0, sizeof(e->meshmap));
    e->meshmap.enabled = 1;
    e->meshmap.max_slope_degrees = 45.0f;
    e->meshmap.max_step_height = 4.0f;
    e->meshmap.movement_plane = MESHMAP_PLANE_NONE;
    e->meshmap.support_id = e->meshmap.drop_id = -1;
    e->meshmap.support_group = e->meshmap.drop_group = MESHMAP_GROUP_NONE;
}

/*
 * Drop-through belongs to one entity and its present support. An explicit
 * group joins authored platform pieces; an ungrouped map uses its ID AND
 * allocation generation, so deleting/recreating a slot cannot inherit an
 * old exception. Other platforms remain eligible during the same fall.
 */
int meshmap_entity_drop(entity *e)
{
    const s_meshmap *map;
    if(!terrain_enabled(e) || e->meshmap.support_id < 0) return 0;
    map = meshmap_get_const(level->meshmaps, (uint32_t)e->meshmap.support_id);
    if(!map || map->generation != e->meshmap.support_generation ||
       !meshmap_is_active(level->meshmaps, map->id) ||
       (map->config.flags & (MESHMAP_ONE_WAY | MESHMAP_DROP_THROUGH)) !=
           (MESHMAP_ONE_WAY | MESHMAP_DROP_THROUGH)) return 0;
    e->meshmap.drop_id = (int)map->id;
    e->meshmap.drop_group = map->config.support_group;
    e->meshmap.drop_generation = map->generation;
    e->meshmap.drop_y = e->meshmap.support_y;
    terrain_clear_support(e);
    e->base = MESHMAP_ENTITY_VOID_BASE;
    if(e->velocity.y >= 0.0f) e->velocity.y = -0.5f;
    return 1;
}

typedef struct terrain_drop_context
{
    const entity *e;
    s_meshmap_query query;
    int overlaps;
} terrain_drop_context;

static int terrain_drop_overlap(const s_meshmap *map, void *opaque)
{
    terrain_drop_context *context = opaque;
    const entity *e = context->e;
    s_meshmap_vec3 feet = terrain_position(e), head = feet;
    s_meshmap_hit hit;
    if(e->meshmap.drop_group != MESHMAP_GROUP_NONE)
    {
        if(map->config.support_group != e->meshmap.drop_group) return 1;
    }
    else if(map->id != (uint32_t)e->meshmap.drop_id ||
            map->generation != e->meshmap.drop_generation) return 1;
    head.y += terrain_height(e);
    if(map->config.axis == MESHMAP_AXIS_Y)
    {
        s_meshmap_sample sample;
        if(meshmap_sample_world(map, feet, &sample) &&
           head.y >= sample.position.y - TERRAIN_SKIN)
            context->overlaps = 1;
    }
    else if(meshmap_contains_point(map, feet) || meshmap_contains_point(map, head) ||
            meshmap_trace_map(map, &context->query, head, feet, &hit) != 0)
        context->overlaps = 1;
    return !context->overlaps;
}

static void terrain_refresh_drop(entity *e)
{
    terrain_drop_context context;
    if(e->meshmap.drop_id < 0) return;
    context.e = e;
    context.query = terrain_query(e);
    context.query.flags = MESHMAP_QUERY_INCLUDE_ONE_WAY | MESHMAP_QUERY_ENTER_ONLY;
    context.query.ignore_id = MESHMAP_ID_NONE;
    context.query.ignore_group = MESHMAP_GROUP_NONE;
    context.overlaps = 0;
    meshmap_foreach(level->meshmaps, &context.query, terrain_drop_overlap, &context);
    if(!context.overlaps)
    {
        e->meshmap.drop_id = -1;
        e->meshmap.drop_group = MESHMAP_GROUP_NONE;
        e->meshmap.drop_generation = 0;
    }
}

/*
 * A trace selects the exposed contact and all of its rules, then reconstructs
 * its position along the ray. At a flat Y=0 face that final subtraction can
 * leave a tiny signed residue. Movement and base refresh use different ray
 * spans, so their residues need not match even at identical stored X/Z.
 * Native gravity deliberately compares position.y and base exactly; feeding
 * it those two residues invents a new landing and clears horizontal velocity.
 *
 * For a Y-map height face, obtain the stored height from that selected map's
 * geometric sample instead. This is independent of the ray's start and end.
 * Keep the trace's identity, normal, admission and one-way decisions intact.
 * Domain/hole edges and X/Z-map faces have different geometry, so they retain
 * their original contact. The same conversion serves support refresh, walking
 * and vertical landing, rather than making gravity tolerate unrelated gaps.
 */
static void terrain_canonical_contact_height(s_meshmap_hit *hit)
{
    s_meshmap_sample sample;
    if(!hit->boundary && hit->map->config.axis == MESHMAP_AXIS_Y &&
       meshmap_sample_world(hit->map, hit->position, &sample))
        hit->position.y = sample.position.y;
}

static int terrain_support(const s_meshmap_query *snapshot,
                           double x, double z, double top, double bottom,
                           s_meshmap_hit *hit)
{
    s_meshmap_query query = *snapshot;
    const s_meshmap_vec3 start = {x, top, z};
    int result;
    if(top <= bottom) return 0;
    query.flags |= MESHMAP_QUERY_INCLUDE_ONE_WAY | MESHMAP_QUERY_SUPPORT_ONLY |
                   MESHMAP_QUERY_ENTER_ONLY;
    result = meshmap_trace_axis(level->meshmaps, &query, start,
                               MESHMAP_AXIS_Y, bottom, hit);
    if(result > 0) terrain_canonical_contact_height(hit);
    return result;
}

/*
 * Native stages with mesh terrain obtain their support from authored terrain
 * and explicit legacy walls/platforms. The implicit Y=0 floor must not fill
 * a painted hole or prevent a valid negative-height map from being reached.
 * Only inspect the current fall distance plus a small resting interval: an
 * actor in midair need not search an entire tall side map every update.
 */
void meshmap_entity_adjust_base(entity *e, float legacy_base)
{
    s_meshmap_query query;
    s_meshmap_hit hit;
    double distance, velocity_scale;
    if(!terrain_enabled(e)) return;
    terrain_refresh_drop(e);
    if(e->meshmap.movement_plane == MESHMAP_AXIS_Y)
    {
        /* A top-down movement slice has no vertical travel. Keeping its base
         * at the slice height also prevents legacy gravity from accumulating
         * a fall against an unreachable mesh support. */
        terrain_clear_support(e);
        e->base = e->position.y;
        e->velocity.y = 0.0f;
        return;
    }
    query = terrain_query(e);
    velocity_scale = global_config.game_speed > 0 ? 100.0 / global_config.game_speed : 1.0;
    distance = fmax(8.0, -e->velocity.y * velocity_scale + TERRAIN_REST_DISTANCE);
    terrain_clear_support(e);
    e->base = legacy_base;
    if(e->velocity.y > 0.0f) return;
    if(terrain_support(&query, e->position.x, e->position.z,
                       e->position.y + TERRAIN_SKIN,
                       e->position.y - distance, &hit) > 0 &&
       hit.walkable && hit.position.y >= legacy_base)
    {
        e->base = (float)hit.position.y;
        if(fabs(e->position.y - hit.position.y) <= TERRAIN_REST_DISTANCE)
            terrain_set_support(e, &hit);
    }
}

/*
 * Native gravity also lands within its resting tolerance before the feet
 * actually cross a surface. Base selection can know that receiver while the
 * pre-move position is still airborne, so a vertical trace need not have set
 * support yet. Resolve identity at the final clamped position instead of
 * claiming contact early. Exact stored-height agreement preserves the chosen
 * legacy floor when a mesh lies nearby beneath a wall or platform.
 */
void meshmap_entity_land(entity *e)
{
    s_meshmap_query query;
    s_meshmap_hit hit;
    if(!e) return;
    terrain_clear_support(e);
    if(!terrain_enabled(e) || e->meshmap.movement_plane == MESHMAP_AXIS_Y) return;
    terrain_refresh_drop(e);
    query = terrain_query(e);
    if(terrain_support(&query, e->position.x, e->position.z,
                       e->position.y + TERRAIN_SKIN,
                       e->position.y - TERRAIN_REST_DISTANCE, &hit) > 0 &&
       hit.walkable && (float)hit.position.y == e->base)
        terrain_set_support(e, &hit);
}

float meshmap_entity_check_base(entity *e, float x, float z,
                               float ceiling, float legacy_base)
{
    s_meshmap_query query;
    s_meshmap_hit hit;
    if(!terrain_enabled(e)) return legacy_base;
    if(e->meshmap.movement_plane == MESHMAP_AXIS_Y) return e->position.y;
    query = terrain_query(e);
    if(terrain_support(&query, x, z, ceiling + TERRAIN_SKIN,
                       ceiling - fmax(8.0, e->meshmap.max_step_height), &hit) > 0 &&
       hit.walkable && hit.position.y > legacy_base)
        return (float)hit.position.y;
    return legacy_base;
}

/*
 * Trace feet while descending and the head while rising. The original point
 * or explicit body snapshot still decides bank admission; the movement
 * segment is used only after entering the detailed terrain query.
 */
float meshmap_entity_vertical(entity *e, float destination_y)
{
    s_meshmap_query query;
    s_meshmap_vec3 start;
    s_meshmap_hit hit;
    const double height = terrain_height(e);
    const int descending = destination_y < e->position.y;
    int result;
    if(!terrain_enabled(e) || destination_y == e->position.y) return destination_y;
    if(e->meshmap.movement_plane == MESHMAP_AXIS_Y)
    {
        e->velocity.y = 0.0f;
        return e->position.y;
    }
    query = terrain_query(e);
    query.flags |= MESHMAP_QUERY_ENTER_ONLY;
    start = terrain_position(e);
    if(descending)
    {
        start.y += TERRAIN_SKIN;
        query.flags |= MESHMAP_QUERY_INCLUDE_ONE_WAY;
    }
    else start.y += height - TERRAIN_SKIN;
    result = meshmap_trace_axis(level->meshmaps, &query, start, MESHMAP_AXIS_Y,
                                  destination_y + (descending ? 0.0 : height), &hit);
    if(result < 0) return e->position.y;
    if(!result || !hit.entering) return destination_y;
    terrain_canonical_contact_height(&hit);
    if(descending)
    {
        if(hit.walkable)
        {
            e->base = (float)hit.position.y;
            terrain_set_support(e, &hit);
        }
        else e->velocity.y = 0.0f;
        return (float)hit.position.y;
    }
    e->velocity.y = 0.0f;
    return (float)(hit.position.y - height);
}

/*
 * Bank admission has already happened in the immutable query snapshot. Inside
 * admitted maps, sweep the entity's central vertical body interval. A pair of
 * feet/head rays would miss a beam strictly between those two points.
 * The small inset keeps resting contact from becoming a lateral obstruction.
 */
static int terrain_body_sweep(const s_meshmap_query *snapshot,
                              s_meshmap_vec3 start, s_meshmap_vec3 end,
                              double height, s_meshmap_hit *hit)
{
    s_meshmap_query query = *snapshot;
    /* The core admits only inward walkable contacts for one-way maps. Keep
     * those tops here so a body follows a rounded crest; their sides and
     * undersides remain passable, including during an upward jump. */
    query.flags &= ~MESHMAP_QUERY_SUPPORT_ONLY;
    query.flags |= MESHMAP_QUERY_INCLUDE_ONE_WAY | MESHMAP_QUERY_ENTER_ONLY;
    start.y += TERRAIN_SKIN;
    end.y += TERRAIN_SKIN;
    return meshmap_trace_body(level->meshmaps, &query, start, end,
                               fmax(0.0, height - 2.0 * TERRAIN_SKIN), hit);
}

static int terrain_same_support(const entity *e, const s_meshmap_hit *hit)
{
    if(e->meshmap.support_group != MESHMAP_GROUP_NONE)
        return hit->support_group == e->meshmap.support_group;
    return e->meshmap.support_id == (int)hit->id &&
           e->meshmap.support_generation == hit->generation;
}

static double terrain_accept_support(entity *e, e_meshmap_axis axis,
                                      s_meshmap_vec3 end, double delta,
                                      const s_meshmap_hit *support)
{
    e->position.y = e->base = (float)support->position.y;
    terrain_set_support(e, support);
    if(axis == MESHMAP_AXIS_X) e->position.x = (float)end.x;
    else e->position.z = (float)end.z;
    return delta;
}

/*
 * Resolve one lateral axis. A ramp follows the line to the destination support
 * only if the complete body can travel there. A step instead tests an upward,
 * across, downward route bounded by max_step_height. This is why a clear floor
 * beyond a narrow wall cannot make that intervening wall disappear.
 *
 * Changes in slope can obstruct the straight chord between two valid supports.
 * Subdivide only these terrain-following cases, to a bounded depth, so a crest
 * or several small stairs can be traversed without jumping through their peak.
 * The original point/body bank snapshot is retained through every subdivision.
 */
static double terrain_move_axis_part(entity *e, const s_meshmap_query *snapshot,
                                     e_meshmap_axis axis, double delta, unsigned depth)
{
    s_meshmap_vec3 start = terrain_position(e), end = start;
    s_meshmap_hit obstruction, support, route_hit;
    const double height = terrain_height(e);
    const double step = fmax(0.0, e->meshmap.max_step_height);
    const int had_support = e->meshmap.support_id >= 0;
    const int grounded = e->velocity.y <= 0.0f &&
        (had_support || fabs(e->position.y - e->base) <= TERRAIN_REST_DISTANCE);
    double fraction = 1.0, lift = 0.0;
    int blocked, support_result = 0, subdivide = 0;
    if(delta == 0.0 || e->meshmap.movement_plane == (int)axis) return 0.0;
    /* Entity coordinates are stored as floats. Query the endpoint that can
     * actually be stored, then return its matching displacement. Sampling an
     * unrounded double endpoint gives a slightly different support Y on the
     * next base refresh, which legacy gravity interprets as a fresh landing
     * and uses to stop horizontal motion. Large worlds expose this even on
     * extremely shallow slopes. Keep all later route tests on this endpoint. */
    if(axis == MESHMAP_AXIS_X)
    {
        end.x = (float)(end.x + delta);
        delta = end.x - start.x;
    }
    else
    {
        end.z = (float)(end.z + delta);
        delta = end.z - start.z;
    }
    if(delta == 0.0) return 0.0;
    blocked = terrain_body_sweep(snapshot, start, end, height, &obstruction);
    if(blocked < 0) return 0.0;

    if(grounded)
    {
        const double angle = fmin(89.0, fmax(0.0, e->meshmap.max_slope_degrees));
        const double rise = step + fabs(delta) * tan(angle * 0.017453292519943295);
        support_result = terrain_support(snapshot, end.x, end.z,
                              start.y + rise + TERRAIN_SKIN,
                              start.y - rise - TERRAIN_SKIN, &support);
        if(support_result < 0) return 0.0;
        if(support_result > 0 && support.walkable)
        {
            const int continuing = terrain_same_support(e, &support);
            int route_blocked;
            s_meshmap_vec3 candidate = {end.x, support.position.y, end.z};
            lift = support.position.y - start.y;
            /* A falling edge does not pull an actor onto a remote platform.
             * Continuous terrain can follow its downhill slope; a different
             * support only snaps down through the normal step allowance. */
            const int can_descend = lift >= -step - TERRAIN_SKIN || continuing;
            const int can_climb = lift <= step + TERRAIN_SKIN ||
                (blocked && obstruction.walkable) ||
                (continuing && (support.flags & MESHMAP_ONE_WAY));
            const int one_way_ok = !(support.flags & MESHMAP_ONE_WAY) ||
                lift <= TERRAIN_SKIN || continuing;
            /* One map/group may contain disconnected islands. Inspect a
             * longer descent in smaller moves before accepting its chord,
             * so crossing a hole releases support instead of pulling the
             * actor straight onto a lower island of the same map. */
            const int inspect_descent = continuing && lift < -step - TERRAIN_SKIN &&
                depth < 8 && fabs(delta) > 0.25;
            if(can_descend && can_climb && one_way_ok && !inspect_descent)
            {
                route_blocked = terrain_body_sweep(snapshot, start, candidate, height, &route_hit);
                if(route_blocked < 0) return 0.0;
                if(!route_blocked)
                    return terrain_accept_support(e, axis, candidate, delta, &support);
                subdivide = route_hit.walkable;

                if(step > 0.0 && lift <= step + TERRAIN_SKIN)
                {
                    s_meshmap_vec3 raised_start = start, raised_end = end;
                    raised_start.y = raised_end.y = start.y + step;
                    /* Check the head during the lift, the entire body while
                     * crossing, and the feet/body while settling afterward. */
                    if(terrain_body_sweep(snapshot, start, raised_start, height, &route_hit) == 0 &&
                       terrain_body_sweep(snapshot, raised_start, raised_end, height, &route_hit) == 0 &&
                       terrain_body_sweep(snapshot, raised_end, candidate, height, &route_hit) == 0)
                        return terrain_accept_support(e, axis, candidate, delta, &support);
                }
            }
            if(lift > step + TERRAIN_SKIN || inspect_descent) subdivide = 1;
        }
        if(blocked && obstruction.walkable) subdivide = 1;
    }

    if(subdivide && depth < 8 && fabs(delta) > 0.25)
    {
        const double first = terrain_move_axis_part(e, snapshot, axis, delta * 0.5, depth + 1);
        if(fabs(first - delta * 0.5) > TERRAIN_SKIN) return first;
        return first + terrain_move_axis_part(e, snapshot, axis, delta - first, depth + 1);
    }

    if(blocked) fraction = fmax(0.0, obstruction.fraction - TERRAIN_SKIN / fabs(delta));
    delta *= fraction;
    if(axis == MESHMAP_AXIS_X) e->position.x += (float)delta;
    else e->position.z += (float)delta;
    if(grounded && had_support)
    {
        /* Walking beyond a mesh edge must release that support immediately.
         * An explicit legacy wall/platform retains its own native base policy. */
        s_meshmap_hit resting;
        const int result = terrain_support(snapshot, e->position.x, e->position.z,
                             e->position.y + TERRAIN_SKIN,
                             e->position.y - TERRAIN_REST_DISTANCE, &resting);
        if(result > 0 && resting.walkable)
        {
            e->base = (float)resting.position.y;
            terrain_set_support(e, &resting);
        }
        else
        {
            terrain_clear_support(e);
            e->base = MESHMAP_ENTITY_VOID_BASE;
        }
    }
    return delta;
}

static double terrain_move_axis(entity *e, const s_meshmap_query *snapshot,
                                e_meshmap_axis axis, double delta)
{
    return terrain_move_axis_part(e, snapshot, axis, delta, 0);
}

void meshmap_entity_move(entity *e, float *move_x, float *move_z)
{
    s_meshmap_query snapshot;
    float original_x, original_z, requested_x, requested_z;
    if(!move_x || !move_z || !terrain_enabled(e)) return;
    terrain_refresh_drop(e);
    snapshot = terrain_query(e);
    original_x = e->position.x;
    original_z = e->position.z;
    requested_x = *move_x;
    requested_z = *move_z;
    *move_x = (float)terrain_move_axis(e, &snapshot, MESHMAP_AXIS_X, *move_x);
    *move_z = (float)terrain_move_axis(e, &snapshot, MESHMAP_AXIS_Z, *move_z);
    /* Legacy trymove compares the accepted delta to its exact request when
     * reporting blocked movement. Float coordinate rounding is not a wall:
     * retain that request when it reaches the same stored endpoint. Partial
     * moves still report the delta to their actual stored position. */
    *move_x = e->position.x == (float)(original_x + requested_x)
        ? requested_x : e->position.x - original_x;
    *move_z = e->position.z == (float)(original_z + requested_z)
        ? requested_z : e->position.z - original_z;
    /* Existing engine movement applies the accepted X/Z deltas and emits its
     * normal movement events. Any permitted step/ramp Y adjustment stays. */
    e->position.x = original_x;
    e->position.z = original_z;
}

int meshmap_entity_test_move(const entity *e, float start_x, float start_z,
                            float x, float z)
{
    entity preview;
    float dx, dz, requested_x, requested_z;
    if(!terrain_enabled(e)) return 1;
    preview = *e;
    /* Pathfinding supplies hypothetical node origins. Resolve the copied
     * entity at that origin so both cheap bank gates and the detailed move
     * inspect this edge, rather than a ray from the real actor's position.
     * Support identity is refreshed locally and never changes the actor. */
    preview.position.x = start_x;
    preview.position.z = start_z;
    if(start_x != e->position.x || start_z != e->position.z)
        meshmap_entity_adjust_base(&preview,
            e->meshmap.support_id < 0 ? e->base : MESHMAP_ENTITY_VOID_BASE);
    dx = requested_x = x - start_x;
    dz = requested_z = z - start_z;
    meshmap_entity_move(&preview, &dx, &dz);
    return fabs(dx - requested_x) <= TERRAIN_SKIN &&
           fabs(dz - requested_z) <= TERRAIN_SKIN;
}

/* Pit death follows the authored world's vertical extent. The core caches
 * this metadata and invalidates it when terrain changes; ordinary entity
 * updates do not scan every map to obtain the cutoff. */
float meshmap_entity_pit_depth(const entity *e)
{
    if(!terrain_enabled(e)) return PIT_DEPTH;
    return (float)(fmin(0.0, meshmap_store_minimum_y(level->meshmaps, 0.0)) + PIT_DEPTH);
}
