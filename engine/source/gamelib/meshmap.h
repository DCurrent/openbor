/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 *
 * Damon V. Caskey - 2026-11-26.
 * Native, world-aligned image meshes and their sparse level collection.
 * This core owns numerical geometry and collision queries only. It has no
 * SDL, image-decoder, renderer, script-VM or entity dependencies.
 */
#ifndef OPENBOR_MESHMAP_H
#define OPENBOR_MESHMAP_H

#include <stddef.h>
#include <stdint.h>

#define MESHMAP_BANK_SLOTS 64u
#define MESHMAP_ID_MAX UINT32_C(0x00ffffff)
#define MESHMAP_ID_NONE UINT32_MAX
#define MESHMAP_GROUP_NONE UINT32_MAX
#define MESHMAP_EPSILON 0.000001

typedef enum e_meshmap_axis
{
    MESHMAP_AXIS_X = 0,
    MESHMAP_AXIS_Y = 1,
    MESHMAP_AXIS_Z = 2
} e_meshmap_axis;

typedef enum e_meshmap_side
{
    MESHMAP_SIDE_NEGATIVE = 0, /* Local mapped coordinate <= the surface. */
    MESHMAP_SIDE_POSITIVE = 1  /* Local mapped coordinate >= the surface. */
} e_meshmap_side;

typedef enum e_meshmap_gate_mode
{
    MESHMAP_GATE_POSITION = 0,
    MESHMAP_GATE_BODY = 1
} e_meshmap_gate_mode;

typedef enum e_meshmap_flags
{
    MESHMAP_COLLIDE = 1u << 0,
    MESHMAP_DRAW = 1u << 1, /* Automatic native queue; explicit draws may override this bit. */
    MESHMAP_WALKABLE = 1u << 2,
    MESHMAP_ONE_WAY = 1u << 3,
    MESHMAP_DROP_THROUGH = 1u << 4
} e_meshmap_flags;

#define MESHMAP_FLAGS_NONE 0u
#define MESHMAP_FLAGS_DEFAULT (MESHMAP_COLLIDE | MESHMAP_DRAW | MESHMAP_WALKABLE)
#define MESHMAP_FLAGS_ALL (MESHMAP_COLLIDE | MESHMAP_DRAW | MESHMAP_WALKABLE | MESHMAP_ONE_WAY | MESHMAP_DROP_THROUGH)

/* Scripts may clear or restore an opaque bank selection. Individual children
 * use ordinary numeric slot indices through the indexed bank properties.
 * There is no fixed ALL mask: "all" means that bank's present children. */
#define MESHMAP_BANK_MASK_NONE UINT64_C(0)

typedef struct s_meshmap_vec3 { double x, y, z; } s_meshmap_vec3;
typedef struct s_meshmap_bounds { s_meshmap_vec3 min, max; } s_meshmap_bounds;

typedef struct s_meshmap_gate
{
    int enabled; /* Zero means explicitly unbounded, rather than an empty box. */
    e_meshmap_gate_mode mode;
    s_meshmap_bounds bounds; /* Creator-authored world coordinates. */
} s_meshmap_gate;

typedef struct s_meshmap_config
{
    e_meshmap_axis axis;
    e_meshmap_side side;
    s_meshmap_vec3 origin;
    double spacing_u, spacing_v;
    double min_height, max_height;
    double slope_limit; /* Degrees from world up, inclusive, in [0,90]. */
    uint32_t flags;
    uint32_t support_group; /* NONE means independent map identity, not a numeric group. */
    int layer;
} s_meshmap_config;

typedef void (*meshmap_art_release_fn)(void *artwork);
struct s_meshmap_store;

typedef struct s_meshmap
{
    struct s_meshmap_store *owner;
    uint32_t id;
    uint64_t generation, revision;
    s_meshmap_config config;
    s_meshmap_gate gate;
    uint32_t width, height;
    uint16_t *samples; /* Row-major, owned; zero is absent. */
    uint8_t *cells; /* One presence byte per cell; all four corners must exist. */
    size_t cell_count, present_cells;
    char *geometry_path;
    void *artwork;
    char *artwork_path;
    meshmap_art_release_fn artwork_release;
} s_meshmap;

typedef struct s_meshmap_bank
{
    struct s_meshmap_store *owner;
    unsigned depth; /* Root=0, area=1, sector=2, room=3; room children are maps. */
    uint64_t present_mask, active_mask;
    s_meshmap_gate gate;
    void *children[MESHMAP_BANK_SLOTS];
} s_meshmap_bank;

typedef struct s_meshmap_store
{
    s_meshmap_bank root;
    uint64_t next_generation;
    size_t map_count, bank_count; /* bank_count includes the root. */
    size_t body_gate_count; /* Enabled BODY gates, including inactive banks. */
    double minimum_y;
    int minimum_y_valid, minimum_y_dirty;
} s_meshmap_store;

typedef struct s_meshmap_vertex
{
    s_meshmap_vec3 position;
    double u, v; /* Normalized artwork coordinates, independent of art size. */
} s_meshmap_vertex;

typedef struct s_meshmap_triangle
{
    s_meshmap_vertex vertex[3];
    s_meshmap_vec3 normal; /* Outward from the selected solid side. */
    uint32_t column, row;
    unsigned triangle;
} s_meshmap_triangle;

typedef struct s_meshmap_sample
{
    s_meshmap_vec3 position, normal;
    double local_u, local_v, height;
    uint32_t column, row;
    unsigned triangle;
} s_meshmap_sample;

/* A gate snapshot is independent of movement. POSITION uses exactly position;
 * BODY uses exactly body. Neither is enlarged to include a sweep. */
typedef struct s_meshmap_query
{
    s_meshmap_vec3 position;
    s_meshmap_bounds body;
    uint32_t required_flags;
    uint32_t ignore_group;
    uint32_t ignore_id;
    uint64_t ignore_generation;
    double slope_limit; /* Query capability, [0,90]; map limit also applies. */
    int movement_plane; /* -1: XYZ; axis X/Y/Z: that coordinate stays fixed. */
    unsigned flags;
} s_meshmap_query;

typedef enum e_meshmap_query_flags
{
    MESHMAP_QUERY_INCLUDE_ONE_WAY = 1u << 0,
    MESHMAP_QUERY_SUPPORT_ONLY = 1u << 1,
    MESHMAP_QUERY_IGNORE_DROP_GROUP = 1u << 2,
    MESHMAP_QUERY_ENTER_ONLY = 1u << 3,
    MESHMAP_QUERY_EXIT_ONLY = 1u << 4
} e_meshmap_query_flags;

typedef struct s_meshmap_hit
{
    const s_meshmap *map;
    uint32_t id, support_group;
    uint64_t generation;
    uint32_t flags;
    s_meshmap_vec3 position, normal;
    double fraction;
    double body_offset; /* Contact's Y offset above the swept feet; point traces use zero. */
    int entering; /* One means free-to-solid; zero means solid-to-free. */
    int boundary; /* One means a domain/hole side, zero means the height face. */
    int walkable;
} s_meshmap_hit;

typedef int (*meshmap_visit_fn)(const s_meshmap *map, void *context);

void meshmap_config_default(s_meshmap_config *config);
void meshmap_query_default(s_meshmap_query *query, s_meshmap_vec3 position);
int meshmap_config_valid(const s_meshmap_config *config);
int meshmap_gate_valid(const s_meshmap_gate *gate);
int meshmap_pack_id(unsigned area, unsigned sector, unsigned room, unsigned map, uint32_t *id);
unsigned meshmap_id_slot(uint32_t id, unsigned depth);

s_meshmap_store *meshmap_store_create(void);
void meshmap_store_free(s_meshmap_store *store);
int meshmap_store_uses_body_gates(const s_meshmap_store *store);
/* Cached conservative lower surface bound for active, nonempty collision maps.
 * Authoring gates do not affect the global pit/death floor. */
double meshmap_store_minimum_y(s_meshmap_store *store, double fallback);
/* prefix is a packed map address. Slots below the requested depth are ignored. */
s_meshmap_bank *meshmap_bank_get(s_meshmap_store *store, unsigned depth, uint32_t prefix, int create);
int meshmap_bank_remove(s_meshmap_store *store, unsigned depth, uint32_t prefix);
int meshmap_bank_set_gate(s_meshmap_bank *bank, const s_meshmap_gate *gate);
int meshmap_bank_set_mask(s_meshmap_bank *bank, uint64_t active_mask);
int meshmap_bank_set_slot(s_meshmap_bank *bank, unsigned slot, int active);
s_meshmap *meshmap_get(s_meshmap_store *store, uint32_t id);
const s_meshmap *meshmap_get_const(const s_meshmap_store *store, uint32_t id);
s_meshmap *meshmap_create(s_meshmap_store *store, uint32_t id);
int meshmap_remove(s_meshmap_store *store, uint32_t id);
int meshmap_is_active(const s_meshmap_store *store, uint32_t id);
int meshmap_set_active(s_meshmap_store *store, uint32_t id, int active);
int meshmap_configure(s_meshmap *map, const s_meshmap_config *config);
int meshmap_set_gate(s_meshmap *map, const s_meshmap_gate *gate);

/* Replacements copy their input and commit only after all validation/allocation
 * succeeds. On failure the old resource, paths and revision remain untouched.
 * Artwork ownership transfers only on success. */
int meshmap_set_geometry(s_meshmap *map, uint32_t width, uint32_t height,
                         const uint16_t *samples, const char *path);
int meshmap_set_sample(s_meshmap *map, uint32_t column, uint32_t row, uint16_t sample);
int meshmap_set_artwork(s_meshmap *map, void *artwork, meshmap_art_release_fn release,
                        const char *path);
int meshmap_set_geometry_path(s_meshmap *map, const char *path);
int meshmap_set_artwork_path(s_meshmap *map, const char *path);

int meshmap_gate_accepts(const s_meshmap_gate *gate, const s_meshmap_query *query);
int meshmap_foreach(const s_meshmap_store *store, const s_meshmap_query *query,
                     meshmap_visit_fn visit, void *context);
int meshmap_cell_present(const s_meshmap *map, uint32_t column, uint32_t row);
double meshmap_decode_height(const s_meshmap *map, uint16_t sample);
int meshmap_vertex(const s_meshmap *map, uint32_t column, uint32_t row, s_meshmap_vertex *vertex);
int meshmap_triangle(const s_meshmap *map, uint32_t column, uint32_t row,
                     unsigned triangle, s_meshmap_triangle *result);
/* u/v are physical local grid coordinates, not texel indices. */
int meshmap_sample_local(const s_meshmap *map, double u, double v, s_meshmap_sample *sample);
int meshmap_sample_world(const s_meshmap *map, s_meshmap_vec3 point, s_meshmap_sample *sample);
int meshmap_contains_point(const s_meshmap *map, s_meshmap_vec3 point);

/* Trace the union of eligible solid maps. Boundary entries and exits include
 * the finite grid and holes, so a side map can supply actual floors/ceilings.
 * Active bank gates use the supplied current-state snapshot throughout.
 * Return negative on allocation failure, zero for no crossing, one for a hit. */
int meshmap_trace(const s_meshmap_store *store, const s_meshmap_query *query,
                   s_meshmap_vec3 start, s_meshmap_vec3 end, s_meshmap_hit *hit);
int meshmap_trace_map(const s_meshmap *map, const s_meshmap_query *query,
                       s_meshmap_vec3 start, s_meshmap_vec3 end, s_meshmap_hit *hit);
/* Exact vertical-line body sweep, including obstacles between feet and head.
 * The body height affects narrow-phase collision only, never bank admission.
 * hit.position is the actual contact; fraction locates the feet trajectory. */
int meshmap_trace_body(const s_meshmap_store *store, const s_meshmap_query *query,
                        s_meshmap_vec3 start_feet, s_meshmap_vec3 end_feet, double height,
                        s_meshmap_hit *hit);
int meshmap_trace_axis(const s_meshmap_store *store, const s_meshmap_query *query,
                        s_meshmap_vec3 start, e_meshmap_axis axis, double end_coordinate,
                        s_meshmap_hit *hit);

#endif
