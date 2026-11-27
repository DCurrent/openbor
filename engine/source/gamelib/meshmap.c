/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 *
 * Damon V. Caskey - 2026-11-26.
 *
 * Mesh maps deliberately keep geometry, ownership and collision independent of
 * the renderer. The image loader supplies native 16-bit samples. This file owns
 * their decoded meaning, the same fixed triangles used by drawing, and the
 * solid volume behind those triangles. Entity movement remains an engine
 * concern; queries here return contacts without moving an entity or choosing
 * its animation, gravity, jump state or current platform.
 */
#include "meshmap.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MESHMAP_PI 3.14159265358979323846264338327950288

typedef struct s_meshmap_local
{
    double u, v, w;
} s_meshmap_local;

static int meshmap_finite_vec(s_meshmap_vec3 value)
{
    return isfinite(value.x) && isfinite(value.y) && isfinite(value.z);
}

static int meshmap_bounds_valid(const s_meshmap_bounds *bounds)
{
    return bounds && meshmap_finite_vec(bounds->min) && meshmap_finite_vec(bounds->max)
        && bounds->min.x <= bounds->max.x && bounds->min.y <= bounds->max.y
        && bounds->min.z <= bounds->max.z;
}

static char *meshmap_copy_path(const char *path)
{
    char *copy;
    size_t size;
    if(!path || !path[0])
    {
        return NULL;
    }
    size = strlen(path) + 1;
    copy = malloc(size);
    if(copy)
    {
        memcpy(copy, path, size);
    }
    return copy;
}

static int meshmap_body_gate(const s_meshmap_gate *gate)
{
    return gate->enabled && gate->mode == MESHMAP_GATE_BODY;
}

static int meshmap_extent_valid(const s_meshmap_config *config, uint32_t width, uint32_t height)
{
    double u = width ? (width - 1.0) * config->spacing_u : 0.0;
    double v = height ? (height - 1.0) * config->spacing_v : 0.0;
    double mapped = config->axis == MESHMAP_AXIS_Y ? config->origin.y
        : config->axis == MESHMAP_AXIS_Z ? config->origin.z : config->origin.x;
    if(!isfinite(u) || !isfinite(v) || !isfinite(mapped + config->min_height)
        || !isfinite(mapped + config->max_height))
    {
        return 0;
    }
    if(config->axis == MESHMAP_AXIS_Y)
    {
        return isfinite(config->origin.x + u) && isfinite(config->origin.z + v);
    }
    return isfinite(config->origin.y - v)
        && isfinite((config->axis == MESHMAP_AXIS_Z ? config->origin.x : config->origin.z) + u);
}

static void meshmap_assign_gate(s_meshmap_store *store, s_meshmap_gate *destination,
                                 const s_meshmap_gate *source)
{
    if(store)
    {
        store->body_gate_count -= meshmap_body_gate(destination);
        store->body_gate_count += meshmap_body_gate(source);
    }
    *destination = *source;
    destination->enabled = !!destination->enabled;
}

void meshmap_config_default(s_meshmap_config *config)
{
    if(!config)
    {
        return;
    }
    memset(config, 0, sizeof(*config));
    config->axis = MESHMAP_AXIS_Y;
    config->side = MESHMAP_SIDE_NEGATIVE;
    config->spacing_u = config->spacing_v = 1.0;
    config->max_height = 1.0;
    config->slope_limit = 45.0;
    config->flags = MESHMAP_FLAGS_DEFAULT;
    config->support_group = MESHMAP_GROUP_NONE;
}

void meshmap_query_default(s_meshmap_query *query, s_meshmap_vec3 position)
{
    if(!query)
    {
        return;
    }
    memset(query, 0, sizeof(*query));
    query->position = position;
    query->body.min = query->body.max = position;
    query->required_flags = MESHMAP_COLLIDE;
    query->ignore_group = MESHMAP_GROUP_NONE;
    query->ignore_id = MESHMAP_ID_NONE;
    query->slope_limit = 90.0;
    query->movement_plane = -1;
    query->flags = MESHMAP_QUERY_INCLUDE_ONE_WAY;
}

int meshmap_config_valid(const s_meshmap_config *config)
{
    return config && config->axis >= MESHMAP_AXIS_X && config->axis <= MESHMAP_AXIS_Z
        && (config->side == MESHMAP_SIDE_NEGATIVE || config->side == MESHMAP_SIDE_POSITIVE)
        && meshmap_finite_vec(config->origin) && isfinite(config->spacing_u)
        && isfinite(config->spacing_v) && config->spacing_u > 0.0 && config->spacing_v > 0.0
        && isfinite(1.0 / config->spacing_u) && isfinite(1.0 / config->spacing_v)
        && isfinite(config->min_height) && isfinite(config->max_height)
        && config->min_height <= config->max_height
        && isfinite(config->max_height - config->min_height)
        && isfinite((config->max_height - config->min_height) / fmin(config->spacing_u, config->spacing_v))
        && isfinite(config->slope_limit) && config->slope_limit >= 0.0 && config->slope_limit <= 90.0
        && !(config->flags & ~MESHMAP_FLAGS_ALL) && meshmap_extent_valid(config, 0, 0);
}

int meshmap_gate_valid(const s_meshmap_gate *gate)
{
    return gate && (gate->mode == MESHMAP_GATE_POSITION || gate->mode == MESHMAP_GATE_BODY)
        && meshmap_finite_vec(gate->bounds.min) && meshmap_finite_vec(gate->bounds.max)
        && (!gate->enabled || meshmap_bounds_valid(&gate->bounds));
}

int meshmap_pack_id(unsigned area, unsigned sector, unsigned room, unsigned map, uint32_t *id)
{
    if(!id || area >= 64 || sector >= 64 || room >= 64 || map >= 64)
    {
        return 0;
    }
    *id = (area << 18) | (sector << 12) | (room << 6) | map;
    return 1;
}

unsigned meshmap_id_slot(uint32_t id, unsigned depth)
{
    if(id > MESHMAP_ID_MAX || depth > 3)
    {
        return MESHMAP_BANK_SLOTS;
    }
    return (id >> (18 - depth * 6)) & 63u;
}

/*
 * Every allocated bank contains only 64 child pointers. Its two masks have
 * different purposes: present says which children exist; active says which
 * existing children participate. An inactive child retains its resources and
 * descendants. No array of 16 million slots is allocated up front.
 *
 * Explicitly created empty banks are useful authoring objects, so deleting the
 * last map does not silently delete its banks or their configured ranges.
 */
static void meshmap_free_map(s_meshmap *map)
{
    if(!map)
    {
        return;
    }
    if(map->owner)
    {
        map->owner->map_count--;
        map->owner->body_gate_count -= meshmap_body_gate(&map->gate);
        map->owner->minimum_y_dirty = 1;
    }
    if(map->artwork && map->artwork_release)
    {
        map->artwork_release(map->artwork);
    }
    free(map->samples);
    free(map->cells);
    free(map->geometry_path);
    free(map->artwork_path);
    free(map);
}

static void meshmap_clear_bank(s_meshmap_bank *bank)
{
    unsigned slot;
    for(slot = 0; slot < MESHMAP_BANK_SLOTS; slot++)
    {
        if(!(bank->present_mask & (UINT64_C(1) << slot)))
        {
            continue;
        }
        if(bank->depth == 3)
        {
            meshmap_free_map(bank->children[slot]);
        }
        else
        {
            s_meshmap_bank *child = bank->children[slot];
            meshmap_clear_bank(child);
            bank->owner->bank_count--;
            free(child);
        }
        bank->children[slot] = NULL;
    }
    bank->owner->body_gate_count -= meshmap_body_gate(&bank->gate);
    memset(&bank->gate, 0, sizeof(bank->gate));
    bank->present_mask = bank->active_mask = 0;
    bank->owner->minimum_y_dirty = 1;
}

s_meshmap_store *meshmap_store_create(void)
{
    s_meshmap_store *store = calloc(1, sizeof(*store));
    if(store)
    {
        store->root.owner = store;
        store->next_generation = 1;
        store->bank_count = 1;
        store->minimum_y_dirty = 1;
    }
    return store;
}

void meshmap_store_free(s_meshmap_store *store)
{
    if(store)
    {
        meshmap_clear_bank(&store->root);
        free(store);
    }
}

int meshmap_store_uses_body_gates(const s_meshmap_store *store)
{
    return store && store->body_gate_count != 0;
}

s_meshmap_bank *meshmap_bank_get(s_meshmap_store *store, unsigned depth, uint32_t prefix, int create)
{
    s_meshmap_bank *bank, *first_parent = NULL;
    unsigned current, first_slot = 0;
    if(!store || depth > 3 || prefix > MESHMAP_ID_MAX)
    {
        return NULL;
    }
    bank = &store->root;
    for(current = 0; current < depth; current++)
    {
        unsigned slot = meshmap_id_slot(prefix, current);
        uint64_t bit = UINT64_C(1) << slot;
        if(!(bank->present_mask & bit))
        {
            s_meshmap_bank *child;
            if(!create)
            {
                return NULL;
            }
            child = calloc(1, sizeof(*child));
            if(!child)
            {
                /* Roll back the entire new path, retaining pre-existing banks. */
                if(first_parent)
                {
                    s_meshmap_bank *first = first_parent->children[first_slot];
                    meshmap_clear_bank(first);
                    free(first);
                    store->bank_count--;
                    first_parent->children[first_slot] = NULL;
                    first_parent->present_mask &= ~(UINT64_C(1) << first_slot);
                    first_parent->active_mask &= ~(UINT64_C(1) << first_slot);
                }
                return NULL;
            }
            if(!first_parent)
            {
                first_parent = bank;
                first_slot = slot;
            }
            child->owner = store;
            child->depth = current + 1;
            bank->children[slot] = child;
            bank->present_mask |= bit;
            bank->active_mask |= bit;
            store->bank_count++;
            store->minimum_y_dirty = 1;
        }
        bank = bank->children[slot];
    }
    return bank;
}

int meshmap_bank_remove(s_meshmap_store *store, unsigned depth, uint32_t prefix)
{
    s_meshmap_bank *parent, *bank;
    unsigned slot;
    uint64_t bit;
    if(!store || depth > 3 || prefix > MESHMAP_ID_MAX)
    {
        return 0;
    }
    if(depth == 0)
    {
        /* Keep both the level's store handle and its monotonic generations. */
        meshmap_clear_bank(&store->root);
        return 1;
    }
    parent = meshmap_bank_get(store, depth - 1, prefix, 0);
    slot = meshmap_id_slot(prefix, depth - 1);
    bit = UINT64_C(1) << slot;
    if(!parent || !(parent->present_mask & bit))
    {
        return 0;
    }
    bank = parent->children[slot];
    meshmap_clear_bank(bank);
    free(bank);
    store->bank_count--;
    parent->children[slot] = NULL;
    parent->present_mask &= ~bit;
    parent->active_mask &= ~bit;
    return 1;
}

int meshmap_bank_set_gate(s_meshmap_bank *bank, const s_meshmap_gate *gate)
{
    if(!bank || !meshmap_gate_valid(gate))
    {
        return 0;
    }
    meshmap_assign_gate(bank->owner, &bank->gate, gate);
    return 1;
}

int meshmap_bank_set_mask(s_meshmap_bank *bank, uint64_t active_mask)
{
    if(!bank || (active_mask & ~bank->present_mask))
    {
        return 0;
    }
    bank->active_mask = active_mask;
    bank->owner->minimum_y_dirty = 1;
    return 1;
}

int meshmap_bank_set_slot(s_meshmap_bank *bank, unsigned slot, int active)
{
    uint64_t bit;
    if(!bank || slot >= MESHMAP_BANK_SLOTS)
    {
        return 0;
    }
    bit = UINT64_C(1) << slot;
    if(!(bank->present_mask & bit))
    {
        return 0;
    }
    if(active)
    {
        bank->active_mask |= bit;
    }
    else
    {
        bank->active_mask &= ~bit;
    }
    bank->owner->minimum_y_dirty = 1;
    return 1;
}

s_meshmap *meshmap_get(s_meshmap_store *store, uint32_t id)
{
    s_meshmap_bank *room = meshmap_bank_get(store, 3, id, 0);
    return room ? room->children[id & 63u] : NULL;
}

const s_meshmap *meshmap_get_const(const s_meshmap_store *store, uint32_t id)
{
    const s_meshmap_bank *bank;
    unsigned depth;
    if(!store || id > MESHMAP_ID_MAX)
    {
        return NULL;
    }
    bank = &store->root;
    for(depth = 0; depth < 3; depth++)
    {
        unsigned slot = meshmap_id_slot(id, depth);
        if(!(bank->present_mask & (UINT64_C(1) << slot)))
        {
            return NULL;
        }
        bank = bank->children[slot];
    }
    return bank->children[id & 63u];
}

s_meshmap *meshmap_create(s_meshmap_store *store, uint32_t id)
{
    s_meshmap *map;
    s_meshmap_bank *room;
    unsigned slot;
    if(!store || id > MESHMAP_ID_MAX)
    {
        return NULL;
    }
    map = meshmap_get(store, id);
    if(map)
    {
        return map;
    }
    map = calloc(1, sizeof(*map));
    if(!map)
    {
        return NULL;
    }
    room = meshmap_bank_get(store, 3, id, 1);
    if(!room)
    {
        free(map);
        return NULL;
    }
    map->owner = store;
    map->id = id;
    map->generation = store->next_generation++;
    if(!store->next_generation)
    {
        store->next_generation = 1;
    }
    map->revision = 1;
    meshmap_config_default(&map->config);
    slot = id & 63u;
    room->children[slot] = map;
    room->present_mask |= UINT64_C(1) << slot;
    room->active_mask |= UINT64_C(1) << slot;
    store->map_count++;
    store->minimum_y_dirty = 1;
    return map;
}

int meshmap_remove(s_meshmap_store *store, uint32_t id)
{
    s_meshmap_bank *room = meshmap_bank_get(store, 3, id, 0);
    unsigned slot = id & 63u;
    uint64_t bit = UINT64_C(1) << slot;
    if(!room || !(room->present_mask & bit))
    {
        return 0;
    }
    meshmap_free_map(room->children[slot]);
    room->children[slot] = NULL;
    room->present_mask &= ~bit;
    room->active_mask &= ~bit;
    return 1;
}

int meshmap_is_active(const s_meshmap_store *store, uint32_t id)
{
    const s_meshmap_bank *bank;
    unsigned depth;
    if(!store || id > MESHMAP_ID_MAX)
    {
        return 0;
    }
    bank = &store->root;
    for(depth = 0; depth < 4; depth++)
    {
        unsigned slot = meshmap_id_slot(id, depth);
        uint64_t bit = UINT64_C(1) << slot;
        if(!(bank->present_mask & bank->active_mask & bit))
        {
            return 0;
        }
        if(depth < 3)
        {
            bank = bank->children[slot];
        }
    }
    return 1;
}

int meshmap_set_active(s_meshmap_store *store, uint32_t id, int active)
{
    s_meshmap_bank *room = meshmap_bank_get(store, 3, id, 0);
    return room && meshmap_bank_set_slot(room, id & 63u, active);
}

int meshmap_configure(s_meshmap *map, const s_meshmap_config *config)
{
    if(!map || !meshmap_config_valid(config) || !meshmap_extent_valid(config, map->width, map->height))
    {
        return 0;
    }
    map->config = *config;
    map->revision++;
    if(map->owner)
    {
        map->owner->minimum_y_dirty = 1;
    }
    return 1;
}

int meshmap_set_gate(s_meshmap *map, const s_meshmap_gate *gate)
{
    if(!map || !meshmap_gate_valid(gate))
    {
        return 0;
    }
    meshmap_assign_gate(map->owner, &map->gate, gate);
    map->revision++;
    return 1;
}

/*
 * Resource replacement is a transaction. Temporary samples, the cell-presence
 * cache and the path are fully allocated before any old pointer is released.
 * Zero is never decoded as a height: a zero corner removes its entire cell.
 * This avoids a sloping "curtain" down to a fictitious zero-height hole floor.
 */
static int meshmap_raw_cell(const uint16_t *samples, uint32_t width, uint32_t column, uint32_t row)
{
    size_t index = (size_t)row * width + column;
    return samples[index] && samples[index + 1] && samples[index + width]
        && samples[index + width + 1];
}

int meshmap_set_geometry(s_meshmap *map, uint32_t width, uint32_t height,
                         const uint16_t *samples, const char *path)
{
    uint16_t *replacement = NULL;
    uint8_t *cells = NULL;
    char *copy = NULL;
    size_t count = 0, cell_count = 0, present = 0;
    uint32_t column, row;
    if(!map)
    {
        return 0;
    }
    if(width || height || samples)
    {
        if(width < 2 || height < 2 || !samples || (size_t)width > SIZE_MAX / height
            || (size_t)width * height > SIZE_MAX / sizeof(*replacement)
            || !meshmap_extent_valid(&map->config, width, height))
        {
            return 0;
        }
        count = (size_t)width * height;
        cell_count = (size_t)(width - 1) * (height - 1);
        replacement = malloc(count * sizeof(*replacement));
        cells = malloc(cell_count);
        if(!replacement || !cells)
        {
            free(replacement);
            free(cells);
            return 0;
        }
        memcpy(replacement, samples, count * sizeof(*replacement));
        for(row = 0; row < height - 1; row++)
        {
            for(column = 0; column < width - 1; column++)
            {
                size_t index = (size_t)row * (width - 1) + column;
                cells[index] = meshmap_raw_cell(replacement, width, column, row);
                present += cells[index];
            }
        }
    }
    copy = meshmap_copy_path(path);
    if(path && path[0] && !copy)
    {
        free(replacement);
        free(cells);
        return 0;
    }
    free(map->samples);
    free(map->cells);
    free(map->geometry_path);
    map->samples = replacement;
    map->cells = cells;
    map->width = width;
    map->height = height;
    map->cell_count = cell_count;
    map->present_cells = present;
    map->geometry_path = copy;
    map->revision++;
    if(map->owner)
    {
        map->owner->minimum_y_dirty = 1;
    }
    return 1;
}

int meshmap_set_sample(s_meshmap *map, uint32_t column, uint32_t row, uint16_t sample)
{
    uint32_t u, v, first_u, first_v, last_u, last_v;
    if(!map || !map->samples || column >= map->width || row >= map->height)
    {
        return 0;
    }
    map->samples[(size_t)row * map->width + column] = sample;
    first_u = column ? column - 1 : 0;
    first_v = row ? row - 1 : 0;
    last_u = column < map->width - 1 ? column : map->width - 2;
    last_v = row < map->height - 1 ? row : map->height - 2;
    for(v = first_v; v <= last_v; v++)
    {
        for(u = first_u; u <= last_u; u++)
        {
            size_t index = (size_t)v * (map->width - 1) + u;
            map->present_cells -= map->cells[index];
            map->cells[index] = meshmap_raw_cell(map->samples, map->width, u, v);
            map->present_cells += map->cells[index];
        }
    }
    map->revision++;
    if(map->owner)
    {
        map->owner->minimum_y_dirty = 1;
    }
    return 1;
}

int meshmap_set_artwork(s_meshmap *map, void *artwork, meshmap_art_release_fn release, const char *path)
{
    char *copy;
    if(!map)
    {
        return 0;
    }
    copy = meshmap_copy_path(path);
    if(path && path[0] && !copy)
    {
        return 0;
    }
    if(map->artwork && map->artwork != artwork && map->artwork_release)
    {
        map->artwork_release(map->artwork);
    }
    free(map->artwork_path);
    map->artwork = artwork;
    map->artwork_release = release;
    map->artwork_path = copy;
    map->revision++;
    return 1;
}

static int meshmap_set_path(s_meshmap *map, char **destination, const char *path)
{
    char *copy = meshmap_copy_path(path);
    if(path && path[0] && !copy)
    {
        return 0;
    }
    free(*destination);
    *destination = copy;
    map->revision++;
    return 1;
}

int meshmap_set_geometry_path(s_meshmap *map, const char *path)
{
    return map && meshmap_set_path(map, &map->geometry_path, path);
}

int meshmap_set_artwork_path(s_meshmap *map, const char *path)
{
    return map && meshmap_set_path(map, &map->artwork_path, path);
}

int meshmap_gate_accepts(const s_meshmap_gate *gate, const s_meshmap_query *query)
{
    if(!gate || !gate->enabled || !query)
    {
        return 1;
    }
    if(gate->mode == MESHMAP_GATE_BODY)
    {
        return meshmap_bounds_valid(&query->body)
            && query->body.max.x >= gate->bounds.min.x && query->body.min.x <= gate->bounds.max.x
            && query->body.max.y >= gate->bounds.min.y && query->body.min.y <= gate->bounds.max.y
            && query->body.max.z >= gate->bounds.min.z && query->body.min.z <= gate->bounds.max.z;
    }
    return meshmap_finite_vec(query->position)
        && query->position.x >= gate->bounds.min.x && query->position.x <= gate->bounds.max.x
        && query->position.y >= gate->bounds.min.y && query->position.y <= gate->bounds.max.y
        && query->position.z >= gate->bounds.min.z && query->position.z <= gate->bounds.max.z;
}

static int meshmap_visit_bank(const s_meshmap_bank *bank, const s_meshmap_query *query,
                               meshmap_visit_fn visit, void *context)
{
    uint64_t mask = bank->present_mask & bank->active_mask;
    unsigned slot;
    if(!meshmap_gate_accepts(&bank->gate, query))
    {
        return 1;
    }
    for(slot = 0; slot < MESHMAP_BANK_SLOTS; slot++)
    {
        if(!(mask & (UINT64_C(1) << slot)))
        {
            continue;
        }
        if(bank->depth == 3)
        {
            const s_meshmap *map = bank->children[slot];
            if(query && ((map->config.flags & query->required_flags) != query->required_flags))
            {
                continue;
            }
            if(meshmap_gate_accepts(&map->gate, query) && !visit(map, context))
            {
                return 0;
            }
        }
        else if(!meshmap_visit_bank(bank->children[slot], query, visit, context))
        {
            return 0;
        }
    }
    return 1;
}

int meshmap_foreach(const s_meshmap_store *store, const s_meshmap_query *query,
                     meshmap_visit_fn visit, void *context)
{
    if(!store || !visit)
    {
        return 0;
    }
    return meshmap_visit_bank(&store->root, query, visit, context);
}

static int meshmap_minimum_y_visit(const s_meshmap *map, void *context)
{
    s_meshmap_store *store = context;
    double minimum;
    if(!map->present_cells || !(map->config.flags & MESHMAP_COLLIDE))
    {
        return 1;
    }
    minimum = map->config.axis == MESHMAP_AXIS_Y
        ? map->config.origin.y + map->config.min_height
        : map->config.origin.y - (map->height - 1.0) * map->config.spacing_v;
    if(!store->minimum_y_valid || minimum < store->minimum_y)
    {
        store->minimum_y = minimum;
    }
    store->minimum_y_valid = 1;
    return 1;
}

double meshmap_store_minimum_y(s_meshmap_store *store, double fallback)
{
    if(!store)
    {
        return fallback;
    }
    if(store->minimum_y_dirty)
    {
        store->minimum_y_valid = 0;
        /* NULL deliberately ignores authored gates while respecting active
         * ancestry. Recompute after mutations, never for every entity ray. */
        meshmap_foreach(store, NULL, meshmap_minimum_y_visit, store);
        store->minimum_y_dirty = 0;
    }
    return store->minimum_y_valid ? store->minimum_y : fallback;
}

/*
 * The three map types share one local cell sampler. Only this small axis
 * conversion differs. Side maps use image rows downward (-world Y), matching
 * ordinary image artwork. Ground-map rows advance in +world Z. None of these
 * mappings rotates world gravity or the engine's entity coordinates.
 */
static s_meshmap_local meshmap_to_local(const s_meshmap *map, s_meshmap_vec3 point)
{
    s_meshmap_local local;
    point.x -= map->config.origin.x;
    point.y -= map->config.origin.y;
    point.z -= map->config.origin.z;
    if(map->config.axis == MESHMAP_AXIS_Y)
    {
        local.u = point.x;
        local.v = point.z;
        local.w = point.y;
    }
    else if(map->config.axis == MESHMAP_AXIS_Z)
    {
        local.u = point.x;
        local.v = -point.y;
        local.w = point.z;
    }
    else
    {
        local.u = point.z;
        local.v = -point.y;
        local.w = point.x;
    }
    return local;
}

static s_meshmap_vec3 meshmap_from_local(const s_meshmap *map, double u, double v, double w, int vector)
{
    s_meshmap_vec3 result;
    if(map->config.axis == MESHMAP_AXIS_Y)
    {
        result.x = u;
        result.y = w;
        result.z = v;
    }
    else if(map->config.axis == MESHMAP_AXIS_Z)
    {
        result.x = u;
        result.y = -v;
        result.z = w;
    }
    else
    {
        result.x = w;
        result.y = -v;
        result.z = u;
    }
    if(!vector)
    {
        result.x += map->config.origin.x;
        result.y += map->config.origin.y;
        result.z += map->config.origin.z;
    }
    return result;
}

int meshmap_cell_present(const s_meshmap *map, uint32_t column, uint32_t row)
{
    return map && map->cells && column < map->width - 1 && row < map->height - 1
        && map->cells[(size_t)row * (map->width - 1) + column];
}

double meshmap_decode_height(const s_meshmap *map, uint16_t sample)
{
    if(!map || !sample)
    {
        return NAN;
    }
    return map->config.min_height + ((sample - 1.0) / 65534.0)
        * (map->config.max_height - map->config.min_height);
}

int meshmap_vertex(const s_meshmap *map, uint32_t column, uint32_t row, s_meshmap_vertex *vertex)
{
    uint16_t value;
    if(!map || !vertex || !map->samples || column >= map->width || row >= map->height)
    {
        return 0;
    }
    value = map->samples[(size_t)row * map->width + column];
    if(!value)
    {
        return 0;
    }
    vertex->position = meshmap_from_local(map, column * map->config.spacing_u,
        row * map->config.spacing_v, meshmap_decode_height(map, value), 0);
    vertex->u = (double)column / (map->width - 1);
    vertex->v = (double)row / (map->height - 1);
    return 1;
}

static void meshmap_plane(const s_meshmap *map, uint32_t column, uint32_t row, unsigned triangle,
                           double *height, double *slope_u, double *slope_v)
{
    size_t index = (size_t)row * map->width + column;
    double h00 = meshmap_decode_height(map, map->samples[index]);
    double h10 = meshmap_decode_height(map, map->samples[index + 1]);
    double h01 = meshmap_decode_height(map, map->samples[index + map->width]);
    double h11 = meshmap_decode_height(map, map->samples[index + map->width + 1]);
    *height = h00;
    if(triangle == 0)
    {
        *slope_u = (h10 - h00) / map->config.spacing_u;
        *slope_v = (h11 - h10) / map->config.spacing_v;
    }
    else
    {
        *slope_u = (h11 - h01) / map->config.spacing_u;
        *slope_v = (h01 - h00) / map->config.spacing_v;
    }
}

static s_meshmap_vec3 meshmap_plane_normal(const s_meshmap *map, double slope_u, double slope_v)
{
    double largest = fmax(1.0, fmax(fabs(slope_u), fabs(slope_v)));
    double nu = -slope_u / largest, nv = -slope_v / largest, nw = 1.0 / largest;
    double scale = 1.0 / hypot(hypot(nu, nv), nw);
    if(map->config.side == MESHMAP_SIDE_POSITIVE)
    {
        scale = -scale;
    }
    return meshmap_from_local(map, nu * scale, nv * scale, nw * scale, 1);
}

int meshmap_triangle(const s_meshmap *map, uint32_t column, uint32_t row,
                     unsigned triangle, s_meshmap_triangle *result)
{
    double height, slope_u, slope_v, orientation;
    s_meshmap_vec3 a, b, cross;
    if(!result || triangle > 1 || !meshmap_cell_present(map, column, row))
    {
        return 0;
    }
    meshmap_vertex(map, column, row, &result->vertex[0]);
    meshmap_vertex(map, column + 1, row + 1, &result->vertex[triangle ? 1 : 2]);
    meshmap_vertex(map, column + !triangle, row + !!triangle, &result->vertex[triangle ? 2 : 1]);
    meshmap_plane(map, column, row, triangle, &height, &slope_u, &slope_v);
    result->normal = meshmap_plane_normal(map, slope_u, slope_v);
    a.x = result->vertex[1].position.x - result->vertex[0].position.x;
    a.y = result->vertex[1].position.y - result->vertex[0].position.y;
    a.z = result->vertex[1].position.z - result->vertex[0].position.z;
    b.x = result->vertex[2].position.x - result->vertex[0].position.x;
    b.y = result->vertex[2].position.y - result->vertex[0].position.y;
    b.z = result->vertex[2].position.z - result->vertex[0].position.z;
    cross.x = a.y * b.z - a.z * b.y;
    cross.y = a.z * b.x - a.x * b.z;
    cross.z = a.x * b.y - a.y * b.x;
    orientation = cross.x * result->normal.x + cross.y * result->normal.y + cross.z * result->normal.z;
    if(orientation < 0.0)
    {
        s_meshmap_vertex swap = result->vertex[1];
        result->vertex[1] = result->vertex[2];
        result->vertex[2] = swap;
    }
    result->column = column;
    result->row = row;
    result->triangle = triangle;
    return 1;
}

/* At an exact grid line, either neighboring cell may own its closed boundary.
 * Inspect both instead of turning an existing edge into a hole merely because
 * floor() chose the absent neighbor. Away from a grid line this is one cell. */
static unsigned meshmap_grid_candidates(double coordinate, uint32_t samples, uint32_t result[2])
{
    double nearest;
    uint32_t cell;
    unsigned count = 0;
    if(!isfinite(coordinate) || coordinate < -1.0e-9 || coordinate > samples - 1.0 + 1.0e-9)
    {
        return 0;
    }
    coordinate = fmax(0.0, fmin(samples - 1.0, coordinate));
    cell = coordinate >= samples - 1.0 ? samples - 2 : (uint32_t)floor(coordinate);
    result[count++] = cell;
    nearest = floor(coordinate + 0.5);
    if(fabs(coordinate - nearest) <= 1.0e-9 && nearest > 0.0 && nearest < samples - 1.0)
    {
        uint32_t neighbor = cell == (uint32_t)nearest ? (uint32_t)nearest - 1 : (uint32_t)nearest;
        if(neighbor != cell)
        {
            result[count++] = neighbor;
        }
    }
    return count;
}

int meshmap_sample_local(const s_meshmap *map, double u, double v, s_meshmap_sample *sample)
{
    uint32_t columns[2], rows[2];
    unsigned column_count, row_count, cu, cv;
    if(!map || !sample || !map->present_cells || !isfinite(u) || !isfinite(v))
    {
        return 0;
    }
    column_count = meshmap_grid_candidates(u / map->config.spacing_u, map->width, columns);
    row_count = meshmap_grid_candidates(v / map->config.spacing_v, map->height, rows);
    for(cv = 0; cv < row_count; cv++)
    {
        for(cu = 0; cu < column_count; cu++)
        {
            uint32_t column = columns[cu], row = rows[cv];
            double local_u, local_v, slope_u, slope_v, height;
            unsigned triangle;
            if(!meshmap_cell_present(map, column, row))
            {
                continue;
            }
            local_u = u - column * map->config.spacing_u;
            local_v = v - row * map->config.spacing_v;
            triangle = local_v / map->config.spacing_v > local_u / map->config.spacing_u;
            meshmap_plane(map, column, row, triangle, &height, &slope_u, &slope_v);
            height += local_u * slope_u + local_v * slope_v;
            sample->position = meshmap_from_local(map, u, v, height, 0);
            sample->normal = meshmap_plane_normal(map, slope_u, slope_v);
            sample->local_u = u;
            sample->local_v = v;
            sample->height = height;
            sample->column = column;
            sample->row = row;
            sample->triangle = triangle;
            return 1;
        }
    }
    return 0;
}

int meshmap_sample_world(const s_meshmap *map, s_meshmap_vec3 point, s_meshmap_sample *sample)
{
    s_meshmap_local local;
    if(!map || !meshmap_finite_vec(point))
    {
        return 0;
    }
    local = meshmap_to_local(map, point);
    return meshmap_sample_local(map, local.u, local.v, sample);
}

int meshmap_contains_point(const s_meshmap *map, s_meshmap_vec3 point)
{
    s_meshmap_local local;
    s_meshmap_sample sample;
    if(!map || !meshmap_finite_vec(point))
    {
        return 0;
    }
    local = meshmap_to_local(map, point);
    if(!meshmap_sample_local(map, local.u, local.v, &sample))
    {
        return 0;
    }
    return map->config.side == MESHMAP_SIDE_NEGATIVE ? local.w <= sample.height + MESHMAP_EPSILON
        : local.w >= sample.height - MESHMAP_EPSILON;
}

/*
 * Collision walkthrough
 * ---------------------
 * A triangle is more than a visible sheet. Its selected solid side occupies a
 * half-infinite prism: the three edges bound its local image footprint, and
 * the height plane bounds its mapped coordinate. Clipping a segment against
 * these four planes produces its entry and exit from that solid volume.
 *
 * This distinction matters for side maps. A Z map can describe a platform in
 * a fixed-Z game. Its top and underside may be boundaries of its X/Y coverage,
 * not intersections with a depth-facing height triangle. They still provide
 * real +Y and -Y collision normals through the prism's footprint planes.
 *
 * Neighboring cells and overlapping ordinary maps contribute a UNION of solid
 * intervals. We merge those intervals before reporting a boundary. Internal
 * triangle diagonals and cell edges consequently cannot become invisible
 * walls. One-way maps contribute only walkable inward support contacts; their
 * undersides and exits remain passable.
 */
typedef struct s_meshmap_interval
{
    double begin, end;
    s_meshmap_hit entry, exit;
} s_meshmap_interval;

#define MESHMAP_LOCAL_INTERVALS 16
typedef struct s_meshmap_intervals
{
    s_meshmap_interval *items;
    size_t count, capacity;
    s_meshmap_interval local[MESHMAP_LOCAL_INTERVALS];
} s_meshmap_intervals;

typedef struct s_meshmap_trace_context
{
    const s_meshmap_query *query;
    s_meshmap_vec3 start, end;
    double time_epsilon;
    double body_height;
    s_meshmap_intervals raw, solids, one_way;
    int failed;
} s_meshmap_trace_context;

static void meshmap_intervals_init(s_meshmap_intervals *list)
{
    list->items = list->local;
    list->count = 0;
    list->capacity = MESHMAP_LOCAL_INTERVALS;
}

static void meshmap_intervals_free(s_meshmap_intervals *list)
{
    if(list->items != list->local)
    {
        free(list->items);
    }
}

static int meshmap_intervals_append(s_meshmap_intervals *list, const s_meshmap_interval *interval)
{
    if(list->count == list->capacity)
    {
        s_meshmap_interval *larger;
        size_t capacity;
        if(list->capacity > SIZE_MAX / 2 / sizeof(*larger))
        {
            return 0;
        }
        capacity = list->capacity * 2;
        larger = malloc(capacity * sizeof(*larger));
        if(!larger)
        {
            return 0;
        }
        memcpy(larger, list->items, list->count * sizeof(*larger));
        if(list->items != list->local)
        {
            free(list->items);
        }
        list->items = larger;
        list->capacity = capacity;
    }
    list->items[list->count++] = *interval;
    return 1;
}

static int meshmap_interval_compare(const void *left, const void *right)
{
    const s_meshmap_interval *a = left, *b = right;
    if(a->begin != b->begin)
    {
        return a->begin < b->begin ? -1 : 1;
    }
    if(a->end != b->end)
    {
        return a->end > b->end ? -1 : 1;
    }
    return a->entry.id < b->entry.id ? -1 : a->entry.id > b->entry.id;
}

static void meshmap_intervals_merge(s_meshmap_intervals *list, double epsilon)
{
    size_t read, write;
    if(list->count < 2)
    {
        return;
    }
    qsort(list->items, list->count, sizeof(*list->items), meshmap_interval_compare);
    write = 0;
    for(read = 1; read < list->count; read++)
    {
        s_meshmap_interval *current = &list->items[write];
        const s_meshmap_interval *next = &list->items[read];
        if(next->begin <= current->end + epsilon)
        {
            /* Coincident roots often differ by a final floating-point bit.
             * Prefer the actual height or exposed footprint face over an
             * internal cell/diagonal plane at that same union boundary. */
            if(next->begin <= current->begin + epsilon
                && next->entry.boundary < current->entry.boundary)
            {
                current->entry = next->entry;
            }
            if(next->end > current->end + epsilon)
            {
                current->end = next->end;
                current->exit = next->exit;
            }
            else if(fabs(next->end - current->end) <= epsilon)
            {
                if(next->exit.boundary < current->exit.boundary)
                {
                    current->exit = next->exit;
                }
                current->end = fmax(current->end, next->end);
            }
        }
        else
        {
            list->items[++write] = *next;
        }
    }
    list->count = write + 1;
}

static int meshmap_walkable(const s_meshmap *map, const s_meshmap_query *query, s_meshmap_vec3 normal)
{
    double length, limit;
    if(!(map->config.flags & MESHMAP_WALKABLE))
    {
        return 0;
    }
    /* Slope belongs to the entity's permitted movement space. A fixed-Z
     * platformer sees the X/Y slice normal, rather than an unrelated depth
     * component of the same surface. The returned hit normal itself remains
     * the exact world normal for callers that need full 3D contacts. */
    if(query->movement_plane == MESHMAP_AXIS_X)
    {
        normal.x = 0.0;
    }
    else if(query->movement_plane == MESHMAP_AXIS_Y)
    {
        normal.y = 0.0;
    }
    else if(query->movement_plane == MESHMAP_AXIS_Z)
    {
        normal.z = 0.0;
    }
    length = hypot(hypot(normal.x, normal.y), normal.z);
    if(length <= DBL_MIN || normal.y <= 0.0)
    {
        return 0;
    }
    limit = fmin(map->config.slope_limit, query->slope_limit);
    return normal.y / length + 1.0e-12 >= cos(limit * MESHMAP_PI / 180.0);
}

static void meshmap_contact(s_meshmap_hit *hit, const s_meshmap *map,
                             const s_meshmap_query *query, s_meshmap_vec3 normal,
                             int boundary, int entering, double fraction)
{
    memset(hit, 0, sizeof(*hit));
    hit->map = map;
    hit->id = map->id;
    hit->support_group = map->config.support_group;
    hit->generation = map->generation;
    hit->flags = map->config.flags;
    hit->normal = normal;
    hit->boundary = boundary;
    hit->entering = entering;
    hit->fraction = fraction;
    hit->walkable = meshmap_walkable(map, query, normal);
}

static int meshmap_clip_time(s_meshmap_interval *interval, const s_meshmap *map,
                              const s_meshmap_query *query, double value, double velocity,
                              s_meshmap_vec3 normal, int boundary, double epsilon)
{
    double time;
    if(fabs(velocity) <= DBL_MIN)
    {
        return value <= MESHMAP_EPSILON;
    }
    time = -value / velocity;
    if(velocity < 0.0)
    {
        if(time > interval->begin + epsilon
            || (fabs(time - interval->begin) <= epsilon && boundary < interval->entry.boundary))
        {
            meshmap_contact(&interval->entry, map, query, normal, boundary, 1, time);
        }
        interval->begin = fmax(interval->begin, time);
    }
    else
    {
        if(time < interval->end - epsilon
            || (fabs(time - interval->end) <= epsilon && boundary < interval->exit.boundary))
        {
            meshmap_contact(&interval->exit, map, query, normal, boundary, 0, time);
        }
        interval->end = fmin(interval->end, time);
    }
    return interval->begin <= interval->end;
}

static int meshmap_clip_plane(s_meshmap_interval *interval, const s_meshmap *map,
                               const s_meshmap_query *query, s_meshmap_local start,
                               s_meshmap_local delta, s_meshmap_local anchor,
                               double nu, double nv, double nw, int boundary, double epsilon)
{
    double largest = fmax(fabs(nu), fmax(fabs(nv), fabs(nw)));
    double length, value, velocity;
    s_meshmap_vec3 normal;
    nu /= largest;
    nv /= largest;
    nw /= largest;
    length = hypot(hypot(nu, nv), nw);
    nu /= length;
    nv /= length;
    nw /= length;
    value = nu * (start.u - anchor.u) + nv * (start.v - anchor.v) + nw * (start.w - anchor.w);
    velocity = nu * delta.u + nv * delta.v + nw * delta.w;
    normal = meshmap_from_local(map, nu, nv, nw, 1);
    return meshmap_clip_time(interval, map, query, value, velocity, normal, boundary, epsilon);
}

typedef struct s_meshmap_body_plane
{
    double value, velocity, body_coefficient;
    s_meshmap_vec3 normal;
    int boundary, geometry;
} s_meshmap_body_plane;

static void meshmap_make_body_plane(s_meshmap_body_plane *plane, const s_meshmap *map,
                                     s_meshmap_local start, s_meshmap_local delta, s_meshmap_local anchor,
                                     double nu, double nv, double nw, int boundary)
{
    double largest = fmax(fabs(nu), fmax(fabs(nv), fabs(nw)));
    double length;
    nu /= largest;
    nv /= largest;
    nw /= largest;
    length = hypot(hypot(nu, nv), nw);
    nu /= length;
    nv /= length;
    nw /= length;
    plane->normal = meshmap_from_local(map, nu, nv, nw, 1);
    plane->value = nu * (start.u - anchor.u) + nv * (start.v - anchor.v) + nw * (start.w - anchor.w);
    plane->velocity = nu * delta.u + nv * delta.v + nw * delta.w;
    plane->body_coefficient = plane->normal.y;
    plane->boundary = boundary;
    plane->geometry = 1;
}

static void meshmap_body_contact_offset(s_meshmap_hit *hit, const s_meshmap_body_plane planes[6], double height)
{
    double low = 0.0, high = height;
    unsigned index;
    if(!hit->map || !isfinite(hit->fraction))
    {
        return;
    }
    for(index = 0; index < 4; index++)
    {
        const s_meshmap_body_plane *plane = &planes[index];
        double value = plane->value + hit->fraction * plane->velocity;
        if(plane->body_coefficient > 0.0)
        {
            high = fmin(high, -value / plane->body_coefficient);
        }
        else if(plane->body_coefficient < 0.0)
        {
            low = fmax(low, -value / plane->body_coefficient);
        }
    }
    hit->body_offset = fmax(0.0, fmin(height, low + (high - low) * 0.5));
}

/*
 * The swept body is a vertical segment, not merely two independent rays.
 * Write each prism inequality as a*t + b*s + c <= 0, where t is motion time
 * and 0 <= s <= body_height is the point's height above the feet. Constraints
 * with b<0 give lower bounds for s; b>0 give upper bounds. Every lower bound
 * must fit below every upper bound. Eliminating s therefore gives a small set
 * of exact linear constraints in t, which the ordinary interval merger uses.
 *
 * There are only four geometry planes and two body end planes. This catches a
 * thin beam anywhere between feet and head without sampling body-height steps.
 * When two geometric faces meet at the contact, their weighted normal is the
 * outward normal of the swept body's contact boundary. Its Y components cancel
 * for a side contact at a beam edge; actual face contacts retain that face's
 * normal. The recorded body_offset places hit.position on the touched solid.
 */
static int meshmap_clip_body_triangle(s_meshmap_trace_context *context, const s_meshmap *map,
                                       uint32_t column, uint32_t row, unsigned triangle,
                                       s_meshmap_local start, s_meshmap_local delta)
{
    s_meshmap_interval interval;
    s_meshmap_body_plane planes[6];
    s_meshmap_local anchor;
    double height, slope_u, slope_v;
    double su = map->config.spacing_u, sv = map->config.spacing_v;
    double side = map->config.side == MESHMAP_SIDE_NEGATIVE ? 1.0 : -1.0;
    unsigned lower, upper;
    memset(&interval, 0, sizeof(interval));
    memset(planes, 0, sizeof(planes));
    interval.begin = -INFINITY;
    interval.end = INFINITY;
    interval.entry.id = interval.exit.id = map->id;
    anchor.u = column * su;
    anchor.v = row * sv;
    anchor.w = 0.0;
    if(triangle == 0)
    {
        int top = row && meshmap_cell_present(map, column, row - 1) ? 2 : 1;
        int right = meshmap_cell_present(map, column + 1, row) ? 2 : 1;
        meshmap_make_body_plane(&planes[0], map, start, delta, anchor, 0, -1, 0, top);
        meshmap_make_body_plane(&planes[1], map, start, delta, anchor, -1 / su, 1 / sv, 0, 2);
        anchor.u += su;
        meshmap_make_body_plane(&planes[2], map, start, delta, anchor, 1, 0, 0, right);
        anchor.u -= su;
    }
    else
    {
        int left = column && meshmap_cell_present(map, column - 1, row) ? 2 : 1;
        int bottom = meshmap_cell_present(map, column, row + 1) ? 2 : 1;
        meshmap_make_body_plane(&planes[0], map, start, delta, anchor, -1, 0, 0, left);
        meshmap_make_body_plane(&planes[1], map, start, delta, anchor, 1 / su, -1 / sv, 0, 2);
        anchor.v += sv;
        meshmap_make_body_plane(&planes[2], map, start, delta, anchor, 0, 1, 0, bottom);
        anchor.v -= sv;
    }
    meshmap_plane(map, column, row, triangle, &height, &slope_u, &slope_v);
    anchor.w = height;
    meshmap_make_body_plane(&planes[3], map, start, delta, anchor, -slope_u * side, -slope_v * side, side, 0);
    planes[4].body_coefficient = -1.0; /* s >= 0 */
    planes[5].body_coefficient = 1.0;  /* s <= body_height */
    planes[5].value = -context->body_height;
    for(lower = 0; lower < 4; lower++)
    {
        const s_meshmap_body_plane *plane = &planes[lower];
        if(plane->body_coefficient == 0.0
            && !meshmap_clip_time(&interval, map, context->query, plane->value, plane->velocity,
                                   plane->normal, plane->boundary, context->time_epsilon))
        {
            return 1;
        }
    }
    for(lower = 0; lower < 6; lower++)
    {
        const s_meshmap_body_plane *low = &planes[lower];
        if(low->body_coefficient >= 0.0)
        {
            continue;
        }
        for(upper = 0; upper < 6; upper++)
        {
            const s_meshmap_body_plane *high = &planes[upper];
            s_meshmap_vec3 normal;
            double value, velocity, length;
            int boundary;
            if(high->body_coefficient <= 0.0)
            {
                continue;
            }
            /* Multiplication avoids dividing by a nearly horizontal face's
             * small Y coefficient. Both weights are nonnegative. */
            value = high->body_coefficient * low->value - low->body_coefficient * high->value;
            velocity = high->body_coefficient * low->velocity - low->body_coefficient * high->velocity;
            normal.x = high->body_coefficient * low->normal.x - low->body_coefficient * high->normal.x;
            normal.y = high->body_coefficient * low->normal.y - low->body_coefficient * high->normal.y;
            normal.z = high->body_coefficient * low->normal.z - low->body_coefficient * high->normal.z;
            length = hypot(hypot(normal.x, normal.y), normal.z);
            if(length <= DBL_MIN)
            {
                if(value > MESHMAP_EPSILON)
                {
                    return 1;
                }
                continue;
            }
            normal.x /= length;
            normal.y /= length;
            normal.z /= length;
            boundary = low->geometry ? low->boundary : high->boundary;
            if(low->geometry && high->geometry)
            {
                boundary = low->boundary < high->boundary ? low->boundary : high->boundary;
            }
            if(!meshmap_clip_time(&interval, map, context->query, value / length, velocity / length,
                                   normal, boundary, context->time_epsilon))
            {
                return 1;
            }
        }
    }
    if(interval.begin > 1.0 + context->time_epsilon || interval.end < -context->time_epsilon
        || interval.end - interval.begin <= context->time_epsilon)
    {
        return 1;
    }
    meshmap_body_contact_offset(&interval.entry, planes, context->body_height);
    meshmap_body_contact_offset(&interval.exit, planes, context->body_height);
    return meshmap_intervals_append(&context->raw, &interval);
}

static int meshmap_clip_triangle(s_meshmap_trace_context *context, const s_meshmap *map,
                                  uint32_t column, uint32_t row, unsigned triangle,
                                  s_meshmap_local start, s_meshmap_local delta)
{
    s_meshmap_interval interval;
    s_meshmap_local anchor;
    double height, slope_u, slope_v;
    double su = map->config.spacing_u, sv = map->config.spacing_v;
    double side = map->config.side == MESHMAP_SIDE_NEGATIVE ? 1.0 : -1.0;
    memset(&interval, 0, sizeof(interval));
    interval.begin = -INFINITY;
    interval.end = INFINITY;
    interval.entry.id = interval.exit.id = map->id;
    anchor.u = column * su;
    anchor.v = row * sv;
    anchor.w = 0.0;
    if(triangle == 0)
    {
        int top = row && meshmap_cell_present(map, column, row - 1) ? 2 : 1;
        int right = meshmap_cell_present(map, column + 1, row) ? 2 : 1;
        /* TL/TR/BR: v >= row, u <= column+1, and normalized v <= u. */
        if(!meshmap_clip_plane(&interval, map, context->query, start, delta, anchor, 0, -1, 0, top, context->time_epsilon)
            || !meshmap_clip_plane(&interval, map, context->query, start, delta, anchor, -1 / su, 1 / sv, 0, 2, context->time_epsilon))
        {
            return 1;
        }
        anchor.u += su;
        if(!meshmap_clip_plane(&interval, map, context->query, start, delta, anchor, 1, 0, 0, right, context->time_epsilon))
        {
            return 1;
        }
        anchor.u -= su;
    }
    else
    {
        int left = column && meshmap_cell_present(map, column - 1, row) ? 2 : 1;
        int bottom = meshmap_cell_present(map, column, row + 1) ? 2 : 1;
        /* TL/BR/BL: u >= column, v <= row+1, and normalized u <= v. */
        if(!meshmap_clip_plane(&interval, map, context->query, start, delta, anchor, -1, 0, 0, left, context->time_epsilon)
            || !meshmap_clip_plane(&interval, map, context->query, start, delta, anchor, 1 / su, -1 / sv, 0, 2, context->time_epsilon))
        {
            return 1;
        }
        anchor.v += sv;
        if(!meshmap_clip_plane(&interval, map, context->query, start, delta, anchor, 0, 1, 0, bottom, context->time_epsilon))
        {
            return 1;
        }
        anchor.v -= sv;
    }
    meshmap_plane(map, column, row, triangle, &height, &slope_u, &slope_v);
    anchor.w = height;
    if(!meshmap_clip_plane(&interval, map, context->query, start, delta, anchor,
                           -slope_u * side, -slope_v * side, side, 0, context->time_epsilon))
    {
        return 1;
    }
    /* Keep real plane times, even outside [0,1]. Clamping them here would
     * invent a contact at the query start when it is already inside a solid.
     * A tangent point has no free-to-solid transition and is not a blocker. */
    if(interval.begin > 1.0 + context->time_epsilon || interval.end < -context->time_epsilon
        || interval.end - interval.begin <= context->time_epsilon)
    {
        return 1;
    }
    return meshmap_intervals_append(&context->raw, &interval);
}

static int meshmap_trace_cell(s_meshmap_trace_context *context, const s_meshmap *map,
                               uint32_t column, uint32_t row, s_meshmap_local start, s_meshmap_local delta)
{
    unsigned triangle;
    if(!meshmap_cell_present(map, column, row))
    {
        return 1;
    }
    for(triangle = 0; triangle < 2; triangle++)
    {
        int result = context->body_height > 0.0
            ? meshmap_clip_body_triangle(context, map, column, row, triangle, start, delta)
            : meshmap_clip_triangle(context, map, column, row, triangle, start, delta);
        if(!result)
        {
            return 0;
        }
    }
    return 1;
}

static int meshmap_trace_cells_at(s_meshmap_trace_context *context, const s_meshmap *map,
                                   s_meshmap_local start, s_meshmap_local delta, double time)
{
    uint32_t columns[2], rows[2];
    unsigned column_count, row_count, u, v;
    double gu = (start.u + time * delta.u) / map->config.spacing_u;
    double gv = (start.v + time * delta.v) / map->config.spacing_v;
    column_count = meshmap_grid_candidates(gu, map->width, columns);
    row_count = meshmap_grid_candidates(gv, map->height, rows);
    for(v = 0; v < row_count; v++)
    {
        for(u = 0; u < column_count; u++)
        {
            if(!meshmap_trace_cell(context, map, columns[u], rows[v], start, delta))
            {
                return 0;
            }
        }
    }
    return 1;
}

static int meshmap_clip_grid_axis(double start, double delta, double maximum, double *begin, double *end)
{
    double first, last;
    if(fabs(delta) <= DBL_MIN)
    {
        return start >= 0.0 && start <= maximum;
    }
    first = -start / delta;
    last = (maximum - start) / delta;
    if(first > last)
    {
        double swap = first;
        first = last;
        last = swap;
    }
    *begin = fmax(*begin, first);
    *end = fmin(*end, last);
    return *begin <= *end;
}

static int meshmap_trace_body_column(s_meshmap_trace_context *context, const s_meshmap *map,
                                      s_meshmap_local start, s_meshmap_local delta, double begin, double end)
{
    double first_v = start.v + begin * delta.v, last_v = start.v + end * delta.v;
    double low = (fmin(first_v, last_v) - context->body_height) / map->config.spacing_v;
    double high = fmax(first_v, last_v) / map->config.spacing_v;
    double u = (start.u + (begin + end) * 0.5 * delta.u) / map->config.spacing_u;
    uint32_t columns[2], first_row, last_row, row;
    unsigned column_count, column;
    if(high < 0.0 || low > map->height - 1.0)
    {
        return 1;
    }
    low = fmax(0.0, fmin(map->height - 1.0, low));
    high = fmax(0.0, fmin(map->height - 1.0, high));
    first_row = (uint32_t)floor(low);
    if(first_row && fabs(low - floor(low + 0.5)) <= 1.0e-9)
    {
        first_row--;
    }
    if(first_row >= map->height - 1)
    {
        first_row = map->height - 2;
    }
    last_row = high >= map->height - 1.0 ? map->height - 2 : (uint32_t)floor(high);
    column_count = meshmap_grid_candidates(u, map->width, columns);
    for(column = 0; column < column_count; column++)
    {
        for(row = first_row; row <= last_row; row++)
        {
            if(!meshmap_trace_cell(context, map, columns[column], row, start, delta))
            {
                return 0;
            }
        }
    }
    return 1;
}

/* Side-map body coverage is a strip in image space. Walk its U grid crossings
 * and visit just the V rows covered during each crossing interval. This avoids
 * scanning the much larger bounding rectangle of a long diagonal motion. */
static int meshmap_collect_body_grid(s_meshmap_trace_context *context, const s_meshmap *map,
                                      s_meshmap_local start, s_meshmap_local delta)
{
    double begin = 0.0, end = 1.0, time, grid_u, velocity_u, edge_u = 0.0, step_u, next_u;
    double maximum_v = (map->height - 1.0) * map->config.spacing_v + context->body_height;
    uint64_t budget;
    if(!meshmap_clip_grid_axis(start.u, delta.u, (map->width - 1.0) * map->config.spacing_u, &begin, &end)
        || !meshmap_clip_grid_axis(start.v, delta.v, maximum_v, &begin, &end))
    {
        return 1;
    }
    if(!meshmap_trace_body_column(context, map, start, delta, begin, begin))
    {
        return 0;
    }
    grid_u = start.u / map->config.spacing_u;
    velocity_u = delta.u / map->config.spacing_u;
    step_u = velocity_u > 0.0 ? 1.0 : -1.0;
    next_u = INFINITY;
    if(velocity_u != 0.0)
    {
        double coordinate = grid_u + begin * velocity_u;
        edge_u = velocity_u > 0.0 ? floor(coordinate) + 1.0 : ceil(coordinate) - 1.0;
        next_u = (edge_u - grid_u) / velocity_u;
    }
    time = begin;
    budget = (uint64_t)map->width + 4;
    while(time < end && budget--)
    {
        double next = fmin(end, next_u);
        if(next > time && !meshmap_trace_body_column(context, map, start, delta, time, next))
        {
            return 0;
        }
        if(next >= end)
        {
            break;
        }
        edge_u += step_u;
        next_u = (edge_u - grid_u) / velocity_u;
        time = fmax(time, next);
    }
    return meshmap_trace_body_column(context, map, start, delta, end, end);
}

/* Visit only cells crossed in local image space. A mapped-axis ray touches at
 * most four cells; an in-plane ray visits a row/column; a diagonal walks grid
 * crossing events. Work follows the path length, not the image's full area. */
static int meshmap_collect_map(s_meshmap_trace_context *context, const s_meshmap *map)
{
    s_meshmap_local start = meshmap_to_local(map, context->start);
    s_meshmap_local finish = meshmap_to_local(map, context->end), delta;
    double begin = 0.0, end = 1.0, time, next_u, next_v, step_u, step_v;
    double grid_u, grid_v, velocity_u, velocity_v, edge_u = 0.0, edge_v = 0.0;
    uint64_t budget;
    delta.u = finish.u - start.u;
    delta.v = finish.v - start.v;
    delta.w = finish.w - start.w;
    if(!isfinite(start.u) || !isfinite(start.v) || !isfinite(start.w)
        || !isfinite(delta.u) || !isfinite(delta.v) || !isfinite(delta.w))
    {
        return 1;
    }
    if(context->body_height > 0.0 && map->config.axis != MESHMAP_AXIS_Y)
    {
        return meshmap_collect_body_grid(context, map, start, delta);
    }
    if(!meshmap_clip_grid_axis(start.u, delta.u, (map->width - 1) * map->config.spacing_u, &begin, &end)
        || !meshmap_clip_grid_axis(start.v, delta.v, (map->height - 1) * map->config.spacing_v, &begin, &end))
    {
        return 1;
    }
    if(!meshmap_trace_cells_at(context, map, start, delta, begin))
    {
        return 0;
    }
    grid_u = start.u / map->config.spacing_u;
    grid_v = start.v / map->config.spacing_v;
    velocity_u = delta.u / map->config.spacing_u;
    velocity_v = delta.v / map->config.spacing_v;
    step_u = velocity_u > 0.0 ? 1.0 : -1.0;
    step_v = velocity_v > 0.0 ? 1.0 : -1.0;
    next_u = next_v = INFINITY;
    if(velocity_u != 0.0)
    {
        double coordinate = grid_u + begin * velocity_u;
        edge_u = velocity_u > 0.0 ? floor(coordinate) + 1.0 : ceil(coordinate) - 1.0;
        next_u = (edge_u - grid_u) / velocity_u;
    }
    if(velocity_v != 0.0)
    {
        double coordinate = grid_v + begin * velocity_v;
        edge_v = velocity_v > 0.0 ? floor(coordinate) + 1.0 : ceil(coordinate) - 1.0;
        next_v = (edge_v - grid_v) / velocity_v;
    }
    time = begin;
    budget = (uint64_t)map->width + map->height + 4;
    while(time < end && budget--)
    {
        double next = fmin(end, fmin(next_u, next_v));
        if(next > time && !meshmap_trace_cells_at(context, map, start, delta, (time + next) * 0.5))
        {
            return 0;
        }
        if(next >= end)
        {
            break;
        }
        if(next_u <= next)
        {
            edge_u += step_u;
            next_u = (edge_u - grid_u) / velocity_u;
        }
        if(next_v <= next)
        {
            edge_v += step_v;
            next_v = (edge_v - grid_v) / velocity_v;
        }
        time = fmax(time, next);
    }
    return meshmap_trace_cells_at(context, map, start, delta, end);
}

static int meshmap_query_valid(const s_meshmap_query *query)
{
    return query && meshmap_finite_vec(query->position) && meshmap_bounds_valid(&query->body)
        && isfinite(query->slope_limit) && query->slope_limit >= 0.0 && query->slope_limit <= 90.0
        && query->movement_plane >= -1 && query->movement_plane <= MESHMAP_AXIS_Z;
}

static int meshmap_drop_ignored(const s_meshmap *map, const s_meshmap_query *query)
{
    if(!(query->flags & MESHMAP_QUERY_IGNORE_DROP_GROUP) || !(map->config.flags & MESHMAP_DROP_THROUGH))
    {
        return 0;
    }
    if(query->ignore_group != MESHMAP_GROUP_NONE)
    {
        return map->config.support_group == query->ignore_group;
    }
    return map->config.support_group == MESHMAP_GROUP_NONE && map->id == query->ignore_id
        && map->generation == query->ignore_generation;
}

static int meshmap_trace_visit(const s_meshmap *map, void *data)
{
    s_meshmap_trace_context *context = data;
    s_meshmap_intervals *destination;
    size_t index;
    if(!map->present_cells || meshmap_drop_ignored(map, context->query)
        || ((map->config.flags & MESHMAP_ONE_WAY)
            && !(context->query->flags & MESHMAP_QUERY_INCLUDE_ONE_WAY)))
    {
        return 1;
    }
    context->raw.count = 0;
    if(!meshmap_collect_map(context, map))
    {
        context->failed = 1;
        return 0;
    }
    meshmap_intervals_merge(&context->raw, context->time_epsilon);
    destination = (map->config.flags & MESHMAP_ONE_WAY) ? &context->one_way : &context->solids;
    for(index = 0; index < context->raw.count; index++)
    {
        if(!meshmap_intervals_append(destination, &context->raw.items[index]))
        {
            context->failed = 1;
            return 0;
        }
    }
    return 1;
}

static int meshmap_contact_eligible(const s_meshmap_query *query, const s_meshmap_hit *hit)
{
    if((query->flags & MESHMAP_QUERY_ENTER_ONLY) && !hit->entering)
    {
        return 0;
    }
    if((query->flags & MESHMAP_QUERY_EXIT_ONLY) && hit->entering)
    {
        return 0;
    }
    return !(query->flags & MESHMAP_QUERY_SUPPORT_ONLY) || (hit->entering && hit->walkable);
}

static void meshmap_choose_contact(s_meshmap_trace_context *context, const s_meshmap_hit *candidate,
                                    s_meshmap_hit *hit, int *found)
{
    double time = candidate->fraction;
    if(!candidate->map || time < -context->time_epsilon || time > 1.0 + context->time_epsilon
        || !meshmap_contact_eligible(context->query, candidate))
    {
        return;
    }
    if(!*found || time < hit->fraction - context->time_epsilon)
    {
        *hit = *candidate;
        hit->fraction = fmax(0.0, fmin(1.0, time));
        *found = 1;
    }
}

static int meshmap_finish_trace(s_meshmap_trace_context *context, s_meshmap_hit *hit)
{
    size_t index;
    int found = 0;
    if(context->failed)
    {
        return -1;
    }
    meshmap_intervals_merge(&context->solids, context->time_epsilon);
    for(index = 0; index < context->solids.count; index++)
    {
        const s_meshmap_interval *interval = &context->solids.items[index];
        meshmap_choose_contact(context, &interval->entry, hit, &found);
        meshmap_choose_contact(context, &interval->exit, hit, &found);
    }
    for(index = 0; index < context->one_way.count; index++)
    {
        const s_meshmap_hit *candidate = &context->one_way.items[index].entry;
        size_t solid;
        int covered = 0;
        if(!candidate->map || !candidate->walkable)
        {
            continue;
        }
        /* A one-way support hidden inside an ordinary solid is not an exposed
         * boundary. Keeping that distinction also prevents a lower platform
         * from replacing a wall contact merely because SUPPORT_ONLY is used. */
        for(solid = 0; solid < context->solids.count; solid++)
        {
            const s_meshmap_interval *interval = &context->solids.items[solid];
            if(candidate->fraction > interval->begin + context->time_epsilon
                && candidate->fraction < interval->end - context->time_epsilon)
            {
                covered = 1;
                break;
            }
        }
        if(!covered)
        {
            meshmap_choose_contact(context, candidate, hit, &found);
        }
    }
    if(found)
    {
        /* Value 2 is an internal tie-breaking priority. At a corner a ray can
         * leave through the endpoint of such an edge; the union transition is
         * still a genuine footprint boundary, exposed publicly as value 1. */
        hit->boundary = hit->boundary != 0;
        hit->position.x = context->start.x + hit->fraction * (context->end.x - context->start.x);
        hit->position.y = context->start.y + hit->fraction * (context->end.y - context->start.y) + hit->body_offset;
        hit->position.z = context->start.z + hit->fraction * (context->end.z - context->start.z);
    }
    return found;
}

static int meshmap_start_trace(s_meshmap_trace_context *context, const s_meshmap_query *query,
                                s_meshmap_vec3 start, s_meshmap_vec3 end)
{
    double dx, dy, dz, distance;
    if(!meshmap_query_valid(query) || !meshmap_finite_vec(start) || !meshmap_finite_vec(end))
    {
        return 0;
    }
    dx = end.x - start.x;
    dy = end.y - start.y;
    dz = end.z - start.z;
    distance = hypot(hypot(dx, dy), dz);
    if(!isfinite(distance) || distance <= DBL_MIN)
    {
        return 0;
    }
    memset(context, 0, sizeof(*context));
    context->query = query;
    context->start = start;
    context->end = end;
    context->time_epsilon = MESHMAP_EPSILON / fmax(1.0, distance);
    meshmap_intervals_init(&context->raw);
    meshmap_intervals_init(&context->solids);
    meshmap_intervals_init(&context->one_way);
    return 1;
}

static void meshmap_end_trace(s_meshmap_trace_context *context)
{
    meshmap_intervals_free(&context->raw);
    meshmap_intervals_free(&context->solids);
    meshmap_intervals_free(&context->one_way);
}

int meshmap_trace(const s_meshmap_store *store, const s_meshmap_query *query,
                   s_meshmap_vec3 start, s_meshmap_vec3 end, s_meshmap_hit *hit)
{
    s_meshmap_query fallback;
    s_meshmap_trace_context context;
    int result;
    if(!query)
    {
        meshmap_query_default(&fallback, start);
        query = &fallback;
    }
    if(!store || !hit || !meshmap_start_trace(&context, query, start, end))
    {
        return 0;
    }
    meshmap_foreach(store, query, meshmap_trace_visit, &context);
    result = meshmap_finish_trace(&context, hit);
    meshmap_end_trace(&context);
    return result;
}

int meshmap_trace_body(const s_meshmap_store *store, const s_meshmap_query *query,
                        s_meshmap_vec3 start_feet, s_meshmap_vec3 end_feet, double height,
                        s_meshmap_hit *hit)
{
    s_meshmap_query fallback;
    s_meshmap_trace_context context;
    int result;
    if(!isfinite(height) || height < 0.0 || !isfinite(start_feet.y + height) || !isfinite(end_feet.y + height))
    {
        return 0;
    }
    if(height == 0.0)
    {
        return meshmap_trace(store, query, start_feet, end_feet, hit);
    }
    if(!query)
    {
        meshmap_query_default(&fallback, start_feet);
        query = &fallback;
    }
    if(!store || !hit || !meshmap_start_trace(&context, query, start_feet, end_feet))
    {
        return 0;
    }
    context.body_height = height;
    meshmap_foreach(store, query, meshmap_trace_visit, &context);
    result = meshmap_finish_trace(&context, hit);
    meshmap_end_trace(&context);
    return result;
}

static int meshmap_ancestry_accepts(const s_meshmap *map, const s_meshmap_query *query)
{
    const s_meshmap_bank *bank;
    unsigned depth;
    if(!meshmap_gate_accepts(&map->gate, query)
        || (map->config.flags & query->required_flags) != query->required_flags)
    {
        return 0;
    }
    if(!map->owner)
    {
        return 1;
    }
    bank = &map->owner->root;
    for(depth = 0; depth < 4; depth++)
    {
        unsigned slot = meshmap_id_slot(map->id, depth);
        uint64_t bit = UINT64_C(1) << slot;
        if(!(bank->present_mask & bank->active_mask & bit) || !meshmap_gate_accepts(&bank->gate, query))
        {
            return 0;
        }
        if(depth < 3)
        {
            bank = bank->children[slot];
        }
    }
    return 1;
}

int meshmap_trace_map(const s_meshmap *map, const s_meshmap_query *query,
                       s_meshmap_vec3 start, s_meshmap_vec3 end, s_meshmap_hit *hit)
{
    s_meshmap_query fallback;
    s_meshmap_trace_context context;
    int result;
    if(!query)
    {
        meshmap_query_default(&fallback, start);
        query = &fallback;
    }
    if(!map || !hit || !meshmap_start_trace(&context, query, start, end))
    {
        return 0;
    }
    if(meshmap_ancestry_accepts(map, query))
    {
        meshmap_trace_visit(map, &context);
    }
    result = meshmap_finish_trace(&context, hit);
    meshmap_end_trace(&context);
    return result;
}

int meshmap_trace_axis(const s_meshmap_store *store, const s_meshmap_query *query,
                        s_meshmap_vec3 start, e_meshmap_axis axis, double end_coordinate,
                        s_meshmap_hit *hit)
{
    s_meshmap_vec3 end = start;
    if(axis == MESHMAP_AXIS_X)
    {
        end.x = end_coordinate;
    }
    else if(axis == MESHMAP_AXIS_Y)
    {
        end.y = end_coordinate;
    }
    else if(axis == MESHMAP_AXIS_Z)
    {
        end.z = end_coordinate;
    }
    else
    {
        return 0;
    }
    return meshmap_trace(store, query, start, end, hit);
}
