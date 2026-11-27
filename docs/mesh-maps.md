# Mesh maps

Author: Damon V. Caskey

Date: 2026-11-27

## Scope of this draft

Mesh maps turn 16-bit grayscale PNG data into native terrain. The same grid
supplies collision and, when artwork is attached, a textured surface. Creators
can use Y maps for ground relief, Z maps for platforming terrain in an X/Y
slice, and X maps for surfaces described over Y/Z. Multiple maps can overlap
and carry different collision, traversal and drawing rules.

The level owns a sparse Area -> Sector -> Room -> Map collection. Native
level-sheet declarations and scripts operate on this same collection. Terrain
geometry, PNG loading, entity movement and GPU rendering have separate native
modules so the numerical core does not depend on a particular game, script VM
or rendering backend.

This implementation is a draft for the `experiment/gpu-palette-cache` branch.
It supplies the actual terrain and rendering paths, along with regression
tests. It does not introduce a general rigid-body physics engine, automatic
world streaming, a level editor or a scene depth buffer. Existing solid
entities remain useful for moving platforms, destructible doors and authored
exceptions.

The implemented contract is defined by the [native core](../engine/source/gamelib/meshmap.h),
[level/script interface](../engine/source/openborscript/meshmap.c),
[entity adapter](../engine/source/gamelib/meshmap_entity.c) and
[rendering interface](../engine/source/gamelib/meshmap_render.h).

## Coordinates and image values

OpenBOR world coordinates use X for horizontal position, Y for altitude and Z
for depth. Image columns increase to the right; image rows increase downward.
Each geometry pixel supplies one vertex. For column `column`, row `row`,
positive sample spacing `spacing_u` and `spacing_v`, and decoded displacement
`w`, define:

```text
u = column * spacing_u
v = row    * spacing_v
```

The following table gives the vertex position before adding the map's XYZ
origin:

| Map axis | Image plane | Local XYZ position | Column direction | Row direction |
| --- | --- | --- | --- | --- |
| `MESHMAP_AXIS_Y` / `y` / 1 | X/Z | `(u, w, v)` | +X | +Z |
| `MESHMAP_AXIS_Z` / `z` / 2 | X/Y | `(u, -v, w)` | +X | -Y |
| `MESHMAP_AXIS_X` / `x` / 0 | Y/Z | `(w, -v, u)` | +Z | -Y |

The X map therefore uses image width for Z extent and image height for Y
extent. For X and Z maps, the origin is at the image's upper edge because
successive rows descend in world Y.

There is no map tilt or rotation setting. View projection can rotate the
image of the world, but it does not rotate the map's collision axes.

### The 16-bit contract

Geometry must be a PNG with **one 16-bit grayscale channel**, at least two
pixels wide and two pixels high. RGB images that merely look gray, 8-bit
grayscale, grayscale-plus-alpha and indexed PNGs are not geometry inputs.

| Sample | Meaning |
| ---: | --- |
| 0 | This vertex does not exist |
| 1 | The configured minimum displacement |
| 65535 | The configured maximum displacement |
| 2 through 65534 | Linear displacement between the endpoints |

For a nonzero sample:

```text
w = minimum + (sample - 1) * (maximum - minimum) / 65534
world_position = origin + local_position
```

Both displacement endpoints and every origin coordinate may be negative.
The endpoints are **local displacement**, not absolute world height and not
the lower and upper thickness of a solid volume. For example, a Y map with
`origin.y = 100`, `minimum = -20` and `maximum = 40` has surface heights
from 80 through 140.

The minimum must not exceed the maximum. Equal endpoints produce a flat
surface while preserving zero-sample holes. Spacing defaults to one world
unit on each image axis and must remain positive and finite. The W by H image
covers `(W - 1) * spacing_u` by `(H - 1) * spacing_v` world units.

The loader preserves raw 16-bit values. It does not apply gamma correction,
strip the low byte or reinterpret zero as a height. Adam7 interlacing is
supported. Saving a visually similar 8-bit image loses the required data
format even when an editor displays it identically.

### Cells, holes and interpolation

Each four-vertex cell uses the diagonal from its top-left corner to its
bottom-right corner. Its two triangles are top-left/top-right/bottom-right
and top-left/bottom-right/bottom-left. Rendering and collision use this same
split and piecewise linear surface.

**Every corner must be nonzero for the cell to exist.** One zero vertex
removes all adjacent cells that depend on it. The engine does not form a
partial triangle from the remaining three vertices, and it does not slope a
surface down toward an invented zero-height floor.

This rule makes hole edges predictable. It also means a single zero pixel
can remove up to four cells. Paint holes with the intended cell footprint in
mind, particularly at a narrow platform edge.

### Artwork

Artwork is optional and uses a separate PNG:

- **8-bit indexed PNG** retains its own palette.
- **24-bit RGB PNG** becomes an RGB32 source texture internally.
- Artwork dimensions may differ from geometry dimensions.
- Odd image widths are preserved.
- The complete artwork image covers the complete grid through normalized UVs.

Artwork color does not create or remove terrain. Black paint remains paint;
only geometry samples control holes. PNG alpha/tRNS is not a separate
meshmap transparency feature. Drawing uses the engine's existing material
and color-key rules. Use an actual geometry hole when movement must pass
through the surface.

Only the sampled displacement surface receives this artwork. Collision can
also use the finite grid's domain edges, but this draft does not generate
textured sidewalls or cap artwork around those edges.

## Build a first terrain patch

The following generator uses only Python's standard library. Run it from a
game's directory to create small, valid test resources under
`data/terrain`. It makes a gentle Y-map trough, a flat two-by-two grid, a
side-map example and RGB artwork.

```bash
python3 - <<'PY'
from pathlib import Path
import math
import struct
import zlib

out = Path("data/terrain")
out.mkdir(parents=True, exist_ok=True)

def chunk(kind, payload):
    return (
        struct.pack(">I", len(payload))
        + kind + payload
        + struct.pack(">I", zlib.crc32(kind + payload))
    )

def write_png(name, width, height, depth, color_type, rows):
    data = b"\x89PNG\r\n\x1a\n"
    data += chunk(b"IHDR", struct.pack(
        ">IIBBBBB", width, height, depth, color_type, 0, 0, 0))
    data += chunk(b"IDAT", zlib.compress(b"".join(b"\0" + row for row in rows)))
    data += chunk(b"IEND", b"")
    (out / name).write_bytes(data)

width, height = 65, 17
rows = []
for row in range(height):
    displacement = -24.0 * math.sin(math.pi * row / (height - 1)) ** 2
    sample = 1 + round((displacement + 24.0) / 24.0 * 65534)
    rows.append(struct.pack(">" + "H" * width, *([sample] * width)))
write_png("trough16.png", width, height, 16, 0, rows)

write_png("flat16.png", 2, 2, 16, 0,
          [struct.pack(">2H", 1, 1), struct.pack(">2H", 1, 1)])

# At world Z = 0, a Z map spanning displacement -1..+1 uses these
# values for air and solid. Every vertex still exists.
rows = []
for row in range(17):
    y = 128 - row * 8
    samples = []
    for column in range(17):
        x = column * 8
        floor = 24 + (x // 32) * 8
        beam = 32 <= x <= 96 and 80 <= y <= 96
        samples.append(65535 if y <= floor or beam else 1)
    rows.append(struct.pack(">17H", *samples))
write_png("side16.png", 17, 17, 16, 0, rows)

rows = []
for row in range(65):
    pixels = bytearray()
    for column in range(257):
        if 48 <= column <= 208:
            color = (60, 66, 72)
            if column in (49, 50, 206, 207):
                color = (235, 230, 205)
        else:
            color = (42, 108, 57)
        pixels.extend(color)
    rows.append(bytes(pixels))
write_png("road24.png", 257, 65, 8, 2, rows)
PY
```

Place these declarations in an otherwise valid level sheet. The selected
address starts at 0/0/0/0, so this single-map example needs no selector:

```text
# Current-position gate, deliberately larger than the grid.
meshmap_area -128 -256 -128 1152 512 384

# Address 0/0/0/0, Y displacement, origin XYZ, minimum/maximum, images.
meshmap y 0 0 0 -24 0 data/terrain/trough16.png data/terrain/road24.png

# 65x17 vertices now span 1024x256 world units.
meshmap_spacing 16 16

# Collision, automatic drawing and walkable support.
meshmap_flags collide draw walkable

# Solid below; independent support; 45-degree slope limit; layer -1000.
meshmap_rules 0 4294967295 45 -1000
```

Place an entity over this patch, with X between 0 and 1024 and Z between 0
and 256. The trough starts and ends at Y = 0 and reaches Y = -24 in the
middle. The native entity defaults permit a 45-degree slope and a four-unit
step. Those are capabilities, not instructions to move the entity or start
a race.

The image files are ordinary game assets and can be packaged with the game.
The example specifies a low painter layer so the ground is normally drawn
behind actors. The layer is a draw-order number; it is not the map's world Z
position.

## Native level-sheet commands

Commands use the fields in the order below. Brackets denote optional
trailing arguments and are not typed in the level sheet. Address components
are integers from 0 through 63. Axis accepts `x`, `y`, `z`, their uppercase
forms or the numeric values in the coordinate table. Flags use
case-insensitive names separated by spaces or tabs. Addresses, child-slot
indices, side selectors and support-group IDs remain ordinary integers.

```text
meshmap_index area sector room index
meshmap axis x y z minimum maximum geometry_png [artwork_png]
meshmap_spacing spacing_u spacing_v
meshmap_flags flag [flag ...]
meshmap_rules side [support_group] [slope_limit] [layer]
meshmap_active enabled
meshmap_bounds min_x min_y min_z max_x max_y max_z [gate_mode]

meshmap_area min_x min_y min_z max_x max_y max_z [gate_mode]
meshmap_sector min_x min_y min_z max_x max_y max_z [gate_mode]
meshmap_room min_x min_y min_z max_x max_y max_z [gate_mode]

meshmap_bank depth min_x min_y min_z max_x max_y max_z [gate_mode]
meshmap_mask depth none
meshmap_mask depth all
meshmap_mask depth slots slot [slot ...]
```

### Select a map and configure it

`meshmap_index` selects the address used by subsequent meshmap commands.
Every level load starts at 0/0/0/0. Selection persists until another
`meshmap_index` command changes it; creating or editing a map does not
advance the index. Selection itself allocates nothing and does not reset an
existing map. Script APIs continue to take explicit map IDs.

The spacing, flags, rules, active and bounds commands configure the
selected map. If it does not yet exist, a valid setting creates an empty
map with the remaining defaults. Its geometry can be supplied later.
Reselecting an existing map lets these commands change individual settings
while preserving its other properties.

```text
# Configure map 1 before loading its geometry.
meshmap_index 0 0 0 1
meshmap_spacing 160 32
meshmap_flags collide walkable one_way drop_through
meshmap y 160 80 96 0 0 data/terrain/flat16.png

# Return to the original map and change only its slope limit.
meshmap_index 0 0 0 0
meshmap_rules 0 4294967295 30
```

`meshmap` loads geometry at the selected address and sets the declared
axis, XYZ origin and displacement range. It creates the map if needed.
When the map exists, its spacing, flags, traversal settings, bounds and
active state remain intact. Supplying an artwork path replaces artwork;
omitting the path preserves existing artwork, and `none` clears it. New
maps without an artwork path remain invisible. Invalid definitions retain
the existing map and its resources.

Bank declarations can precede maps. `meshmap_area`, `meshmap_sector` and
`meshmap_room` configure the area, sector and room along the selected path.
`meshmap_bank` selects an ancestor by depth along that same path. These
commands create banks when needed and leave the selected map address
unchanged. Ancestors created implicitly by a map start with unrestricted
gates. The default `gate_mode` is 0, current position; 1 selects the optional
current body box. `meshmap_bounds` adds the same kind of gate to the map.

`meshmap_flags` replaces the map's complete flag selection with the named
flags on that line. The names are combined internally. For example,
`collide walkable` enables collision and supporting contacts without
automatic drawing. Numeric masks, pipe-delimited expressions and unknown
names are rejected. `none` contributes no flags; used alone, it disables all
flags. It does not remove names already included on the same line.

`meshmap_rules` requires side 0 for the negative side or 1 for the positive
side. Its optional arguments retain their current values when omitted.
Support-group IDs are ordinary unsigned integers; 4294967295 means
independent support identity. Optional arguments are positional: supply the
group before the slope limit or layer. IDs and other integer data fields
accept decimal or hexadecimal text.

### Default map settings and named flags

| Setting | Default |
| --- | --- |
| Axis | Y for an empty map; required explicitly by `meshmap` |
| Origin | `(0, 0, 0)` |
| Displacement range | `0..1` for an empty map; explicit in `meshmap` |
| Spacing U/V | `1 / 1` |
| Solid side | Negative |
| Flags | `default`: collide, automatic draw and walkable |
| Support group | 4294967295 / `MESHMAP_GROUP_NONE` |
| Maximum walkable slope | 45 degrees |
| Painter layer | 0 |
| Gates | Disabled, meaning unrestricted |
| Active slot | Enabled when created |

| Native name | Script constant | Meaning |
| --- | --- | --- |
| `collide` | `MESHMAP_COLLIDE` | Participate in native terrain collision |
| `draw` | `MESHMAP_DRAW` | Automatically queue artwork in the native scene |
| `walkable` | `MESHMAP_WALKABLE` | Permit qualifying upward contacts to support an entity |
| `one_way` | `MESHMAP_ONE_WAY` | Use the one-way platform behavior in the entity adapter |
| `drop_through` | `MESHMAP_DROP_THROUGH` | Permit an explicit drop request on a one-way support |
| `none` | `MESHMAP_FLAGS_NONE` | No flags |
| `default` | `MESHMAP_FLAGS_DEFAULT` | Collision, automatic drawing and walkable support |
| `all` | `MESHMAP_FLAGS_ALL` | Every defined flag |

Scripts use these constants with `openborconstant()`. Combine constants
with bitwise OR when assigning `MESHMAP_PROPERTY_FLAGS`:

```c
set_meshmap_property(maps, id,
    openborconstant("MESHMAP_PROPERTY_FLAGS"),
    openborconstant("MESHMAP_COLLIDE")
        | openborconstant("MESHMAP_WALKABLE"));
```

The individual `COLLIDE`, `DRAW`, `WALKABLE`, `ONE_WAY` and `DROP_THROUGH`
properties also accept ordinary Boolean 0/1 values. Use them when changing
one option while preserving the others.

`MESHMAP_DRAW` controls **automatic native scene drawing**. Explicit
`draw_meshmap()` and `draw_meshmap_projected()` calls can draw an active map
with artwork even when this flag is off. This is useful when a game renders
its terrain only into a projected subscreen.

### Solid side and overlapping maps

Side 0 / `MESHMAP_SIDE_NEGATIVE` makes mapped coordinates at or below the
sampled surface solid within an existing cell. Side 1 /
`MESHMAP_SIDE_POSITIVE` makes coordinates at or above that surface solid.

For a Y map, negative means the ground below the surface. Positive can
describe an overhead solid region. For an X map, negative is the lower-X
side; for a Z map, negative is the lower-Z side. The finite image domain and
zero-cell holes limit where that rule applies on the other two axes.

These are solids extending to one side of a surface. The displacement
minimum does not create a second face. Two overlapping solids combine as a
union for collision: entering any applicable solid is blocked. Interior
boundaries inside that union are not treated as exposed walls. Empty space
in one map does not subtract a different map's solid terrain.

## Addresses, banks and cheap rejection

Each map has a 24-bit level-local ID:

```text
id = (area << 18) | (sector << 12) | (room << 6) | index
```

There are **16,777,216 possible addresses**, numbered **0 through 16,777,215**
(`0xFFFFFF`). The upper count itself is not a valid ID. For example,
area 1, sector 2, room 3, map 4 has ID 270532 (`0x0420C4`).

The collection stores nested 64-slot banks and allocates children only when
needed. Declaring a map does not allocate all possible addresses. Each bank
has a present mask and an active mask. Only existing, active children are
visited; disabling a parent removes its descendants from collision and
drawing without freeing their resources.

| Bank depth | Bank addressed | Child slots refer to |
| ---: | --- | --- |
| 0 | Collection root | Areas |
| 1 | Selected area | Sectors |
| 2 | Selected area/sector | Rooms |
| 3 | Selected area/sector/room | Maps |

Native bank commands derive their path from the current `meshmap_index`.
Depth 0 selects the collection root, regardless of that address. Depths 1,
2 and 3 select its area, sector and room. The shortcut bounds commands
`meshmap_area`, `meshmap_sector` and `meshmap_room` use those three depths.
Neither bounds nor mask commands change the selected map.

Script bank APIs still take an explicit packed prefix in full map-ID
format. Components below the requested depth are ignored. Prefix 270532
therefore addresses area 1 at depth 1, sector 1/2 at depth 2, or room 1/2/3
at depth 3. The script root convention is depth 0, prefix 0.

`meshmap_mask` replaces a bank's active selection. Use `slots` followed by
ordinary child indices from 0 through 63, `none` to disable every child, or
`all` to enable every child present when the command runs. `none` and `all`
appear alone after the depth. Keywords are case-insensitive. The `slots`
keyword identifies a list of indices; the engine builds its internal mask.
Each selected slot must already exist. Packed numeric masks are rejected.

For example, when areas 0 and 1 already exist:

```text
# Keep these two areas active and disable the other areas.
meshmap_mask 0 slots 0 1

# Temporarily disable every area while retaining their resources.
meshmap_mask 0 none

# Enable every area that currently exists.
meshmap_mask 0 all
```

The active selection cannot include a nonexistent child. Creating children
activates their new slots; it does not reactivate an already disabled
ancestor. Enabling one map's local slot likewise does not override a
disabled area, sector or room.

### Position gates and optional body gates

Every bank and map can have a creator-authored XYZ minimum/maximum range.
Each enabled range is checked before descending further. The point gate uses
the entity's **current position**. The body gate uses overlap with its
**current body box**. Bounds comparisons include the boundary.

Neither test expands its range to include velocity, a future position or
swept body bounds. Creators must overlap or enlarge applicability ranges
enough for the movement speeds and transitions in their game. The fast entity
whose admission snapshot lies outside a bank can skip that bank even when
its requested move would reach terrain inside it.

The optional body snapshot comes from active frame body boxes, including
facing, with a model-size fallback. It is only constructed when an enabled
body gate exists in the collection. Selecting body admission does not
change detailed terrain collision into a full moving-AABB solver.

After admission, the mesh collision code can trace the requested movement
through the relevant cells. That detailed work does not change the simple
bank test. Bounds are applicability gates, not extra physical walls and not
a clipping volume for the artwork.

Automatic and explicit rendering honor active masks, but do not use
entity-position collision gates. Terrain artwork should not disappear just
because a particular actor is outside its collision range.

## Two other axis examples

### Platforming in an X/Y slice

Use the generated `side16.png` in a separate test level:

```text
meshmap_area -128 -256 -8 256 256 8
meshmap z 0 128 0 -1 1 data/terrain/side16.png
meshmap_spacing 8 8
meshmap_flags collide walkable
meshmap_rules 0 4294967295 45
```

This map spans X = 0..128 and Y = 128..0. At the play plane Z = 0, values
mapping to +1 are solid because Z <= +1; values mapping to -1 leave that
plane free because Z > -1. The image contains a staircase-shaped floor and
an overhead beam with space below it. Triangle interpolation places the
actual boundary between neighboring air and solid samples.

This distinction is useful: sample 1 remains a real vertex at the minimum
displacement, while sample 0 removes neighboring cells altogether. Ordinary
air/solid contours can use nonzero depth values on opposite sides of the
play plane. Zero provides an explicit absence rule where needed.

Set the entity's movement plane to Z using the script example below.
Walkability then uses the X/Y slice normal, so an appropriate platform ramp
is measured in the movement plane. Existing stage art can supply the
visuals; attaching artwork is optional.

### X-facing barrier

The following creates a solid region beginning at X = 300, covering
Y = 0..128 and Z = 0..128:

```text
meshmap x 300 128 0 0 0 data/terrain/flat16.png
meshmap_spacing 128 128
meshmap_flags collide
meshmap_rules 1
```

The positive solid side blocks entry toward increasing X. Using the
negative side would place the solid region on the other side of the
surface. This example provides collision only and does not add a floor;
use it alongside the intended ground terrain.

## Script access through the level

Resolve property and rule names with `openborconstant()`. These APIs use
constant IDs, not string property names.

```c
void maps = get_level_property(
    NULL(), openborconstant("LEVEL_PROPERTY_MESHMAP_COLLECTION"));

int count = get_level_property(
    NULL(), openborconstant("LEVEL_PROPERTY_MESHMAP_COUNT"));
```

The collection getter lazily creates an empty root if needed. The count
includes existing maps, including inactive maps. Both level properties are
read-only: ownership remains with the level, and mutations go through the
meshmap APIs.

Collection handles are borrowed from the current level. Reacquire them
after a level transition and do not pass them to `free()`. Individual maps
are accessed by ID rather than exposing a writable allocation pointer.

### Create, edit and remove a map

This level-initialization script creates map 0 when that slot is free. Use
it as an alternative to the earlier native declaration, not in addition
to that declaration:

```c
void main()
{
    void maps = get_level_property(
        NULL(), openborconstant("LEVEL_PROPERTY_MESHMAP_COLLECTION"));
    int id = 0;

    if(!get_meshmap_property(
        maps, id, openborconstant("MESHMAP_PROPERTY_EXISTS")))
    {
        set_meshmap_property(maps, id,
            openborconstant("MESHMAP_PROPERTY_EXISTS"), 1);
        set_meshmap_property(maps, id,
            openborconstant("MESHMAP_PROPERTY_AXIS"),
            openborconstant("MESHMAP_AXIS_Y"));
        set_meshmap_property(maps, id,
            openborconstant("MESHMAP_PROPERTY_MINIMUM"), -24);
        set_meshmap_property(maps, id,
            openborconstant("MESHMAP_PROPERTY_MAXIMUM"), 0);
        set_meshmap_property(maps, id,
            openborconstant("MESHMAP_PROPERTY_SPACING_U"), 16);
        set_meshmap_property(maps, id,
            openborconstant("MESHMAP_PROPERTY_SPACING_V"), 16);
        set_meshmap_property(maps, id,
            openborconstant("MESHMAP_PROPERTY_GEOMETRY"),
            "data/terrain/trough16.png");
        set_meshmap_property(maps, id,
            openborconstant("MESHMAP_PROPERTY_ARTWORK"),
            "data/terrain/road24.png");
    }
}
```

Individual assignments validate the resulting map before publishing the
change. Maintain a valid range while editing endpoints: lower the minimum
before lowering the maximum past it, or raise the maximum before raising
the minimum past it. Several successful assignments are not one combined
transaction.

Reading and changing a sample uses **value before column and row** in the
setter:

```c
int sample = get_meshmap_property(
    maps, 0, openborconstant("MESHMAP_PROPERTY_SAMPLE"), 10, 4);

set_meshmap_property(
    maps, 0, openborconstant("MESHMAP_PROPERTY_SAMPLE"), 0, 10, 4);
```

That example opens the cells sharing vertex column 10, row 4. Edits affect
the live geometry and its cell-presence cache. They do not write the source
PNG back to disk. The stored geometry path still identifies the loaded
source asset.

Resource setters load and validate a replacement before releasing the old
resource. Malformed PNGs and rejected values report a script error while
retaining the previous resource. Successful meshmap setters return 1.

```c
// Remove artwork while retaining collision geometry.
set_meshmap_property(
    maps, 0, openborconstant("MESHMAP_PROPERTY_ARTWORK"), "");

// Disable a local slot without releasing it.
set_meshmap_property(
    maps, 0, openborconstant("MESHMAP_PROPERTY_ENABLED"), 0);

// Remove the map and its owned resources.
set_meshmap_property(
    maps, 0, openborconstant("MESHMAP_PROPERTY_EXISTS"), 0);
```

### Map property reference

Function signatures:

```text
get_meshmap_property(collection, id, property [, column, row])
set_meshmap_property(collection, id, property, value [, column, row])
```

Only `SAMPLE` uses column and row. Prefix every suffix below with
`MESHMAP_PROPERTY_`.

| Suffix | Access | Meaning |
| --- | --- | --- |
| `EXISTS` | Read/write | Query presence; 1 creates, 0 removes |
| `ID`, `AREA`, `SECTOR`, `ROOM`, `INDEX` | Read | Packed ID and its components |
| `ENABLED` | Read/write | Getter checks active ancestry; setter changes the local slot |
| `AXIS`, `SIDE` | Read/write | Axis and solid-side constants |
| `ORIGIN_X`, `ORIGIN_Y`, `ORIGIN_Z` | Read/write | World origin |
| `MINIMUM`, `MAXIMUM` | Read/write | Signed local displacement endpoints |
| `SPACING_U`, `SPACING_V` | Read/write | Positive world units per vertex interval |
| `SLOPE_LIMIT` | Read/write | Map's maximum supporting slope, 0..90 degrees |
| `FLAGS` | Read/write | Complete flag selection, supplied using named rule constants |
| `COLLIDE`, `DRAW`, `WALKABLE`, `ONE_WAY`, `DROP_THROUGH` | Read/write | Individual rule switches as integer 0/1 |
| `SUPPORT_GROUP` | Read/write | Unsigned group ID or `MESHMAP_GROUP_NONE` |
| `LAYER` | Read/write | Automatic native painter layer |
| `WIDTH`, `HEIGHT` | Read | Geometry dimensions in vertices |
| `PRESENT_CELLS` | Read | Count of cells with all four corners present |
| `SAMPLE` | Read/write | Raw uint16 sample at column/row |
| `GEOMETRY`, `ARTWORK` | Read/write | Resource paths; setters load resources |
| `REVISION` | Read | Resource/configuration revision |
| `GENERATION` | Read | Allocation identity, changes when an ID is recreated |
| `GATE_ENABLED` | Read/write | 0 makes this gate unrestricted |
| `GATE_MODE` | Read/write | `MESHMAP_GATE_POSITION` or `MESHMAP_GATE_BODY` |
| `MIN_X`, `MIN_Y`, `MIN_Z`, `MAX_X`, `MAX_Y`, `MAX_Z` | Read/write | Creator-authored world gate bounds |

IDs, selectors, sample coordinates and booleans require integer arguments.
Use named constants for flags. Geometric quantities accept finite numeric
values. Requests for an absent map are errors except for `EXISTS`.

### World-space surface probes

Camera, scenery and rider scripts can inspect the exact native triangle at a
world position through the read-only `SURFACE_*` properties:

```text
get_meshmap_property(collection, id, surface_property, world_x, world_y, world_z)
```

`MESHMAP_PROPERTY_SURFACE_PRESENT` returns 1 when the probe lies in an existing
cell and 0 outside the map or over a hole. `SURFACE_X`, `SURFACE_Y` and
`SURFACE_Z` return the sampled world surface position. `SURFACE_NORMAL_X`,
`SURFACE_NORMAL_Y` and `SURFACE_NORMAL_Z` return its outward unit normal.
`SURFACE_COLUMN`, `SURFACE_ROW` and `SURFACE_TRIANGLE` identify the cell and
triangle. Prefix each suffix with `MESHMAP_PROPERTY_`.

The mapped coordinate of the probe is replaced by the sampled displacement;
for example, Y-map sampling uses the supplied X/Z and returns the ground Y.
Supply three finite coordinates even when one coordinate does not affect the
lookup. Fields other than `SURFACE_PRESENT` return NULL over missing terrain.
These are numerical resource queries, so active masks and entity-admission
bounds do not restrict them. Native movement continues to enforce those rules.

```c
if(get_meshmap_property(maps, id,
    openborconstant("MESHMAP_PROPERTY_SURFACE_PRESENT"), x, 0, z))
{
    float ground_y = get_meshmap_property(maps, id,
        openborconstant("MESHMAP_PROPERTY_SURFACE_Y"), x, 0, z);
}
```

This keeps presentation and placement tied to the same PNG samples and
triangle interpolation as collision, without a separately generated script
height table. Sample values are read immediately; no result handle or extra
allocation needs to be released.

### Bank properties

```text
get_meshmap_bank_property(collection, depth, prefix, property)
set_meshmap_bank_property(collection, depth, prefix, property, value)

get_meshmap_bank_property(collection, depth, prefix, child_property, slot)
set_meshmap_bank_property(collection, depth, prefix, child_property, value, slot)
```

Prefix the following names with `MESHMAP_BANK_PROPERTY_`:

| Suffix | Access | Meaning |
| --- | --- | --- |
| `EXISTS` | Read/write | Create or remove the selected bank |
| `DEPTH` | Read | Depth 0..3 |
| `PRESENT_MASK` | Read | Opaque selection of existing immediate children; assign to `ACTIVE_MASK` to enable them |
| `ACTIVE_MASK` | Read/write | Enabled immediate children; use an opaque returned mask or `MESHMAP_BANK_MASK_NONE` |
| `CHILD_PRESENT` | Read | Whether the selected immediate child exists; extra `slot` argument |
| `CHILD_ACTIVE` | Read/write | Selected immediate child's local active state as 0/1; extra `slot` argument |
| `CHILD_COUNT` | Read | Number of present immediate children |
| `GATE_ENABLED`, `GATE_MODE` | Read/write | Gate activation and position/body mode |
| `MIN_X`, `MIN_Y`, `MIN_Z`, `MAX_X`, `MAX_Y`, `MAX_Z` | Read/write | World gate bounds |

Removing a bank also removes its descendants. Removing depth 0 clears the
collection while retaining the level-owned collection handle. Removing one
map does not automatically prune its empty ancestor banks.

Use `CHILD_PRESENT` and `CHILD_ACTIVE` for individual slots. The slot is an
ordinary index from 0 through 63. It follows the value in the setter:

```c
// Disable area 1 while preserving every other area's active state.
if(get_meshmap_bank_property(maps, 0, 0,
    openborconstant("MESHMAP_BANK_PROPERTY_CHILD_PRESENT"), 1))
{
    set_meshmap_bank_property(maps, 0, 0,
        openborconstant("MESHMAP_BANK_PROPERTY_CHILD_ACTIVE"), 0, 1);
}
```

To enable every currently present child, pass the opaque `PRESENT_MASK`
value to `ACTIVE_MASK`:

```c
set_meshmap_bank_property(maps, 0, 0,
    openborconstant("MESHMAP_BANK_PROPERTY_ACTIVE_MASK"),
    get_meshmap_bank_property(maps, 0, 0,
        openborconstant("MESHMAP_BANK_PROPERTY_PRESENT_MASK")));
```

Whole masks can also be saved and restored as opaque values. Keep the
returned variant without converting it to a floating-point value or a
narrower integer type, so every slot is preserved:

```c
void saved_mask = get_meshmap_bank_property(
    maps, 0, 0, openborconstant("MESHMAP_BANK_PROPERTY_ACTIVE_MASK"));

set_meshmap_bank_property(
    maps, 0, 0, openborconstant("MESHMAP_BANK_PROPERTY_ACTIVE_MASK"),
    openborconstant("MESHMAP_BANK_MASK_NONE"));

// Later, while these children still exist:
set_meshmap_bank_property(
    maps, 0, 0, openborconstant("MESHMAP_BANK_PROPERTY_ACTIVE_MASK"), saved_mask);
```

Configure individual gate bounds while `GATE_ENABLED` is 0, then enable
the gate after all six values form a valid box. This avoids invalid
intermediate ranges during a multi-property edit.

## Entity traversal, support and drop-through

Each entity starts with mesh collision enabled, a 45-degree slope
capability, four units of step capability and no fixed movement plane.
These settings use the current `get_entity_property()` and
`set_entity_property()` APIs.

For a platforming actor constrained to an X/Y slice, the following can run
from an entity initialization script:

```c
void main()
{
    void self = getlocalvar("self");

    set_entity_property(self,
        openborconstant("ENTITY_PROPERTY_POSITION_Z"), 0);
    set_entity_property(self,
        openborconstant("ENTITY_PROPERTY_MESHMAP_MOVEMENT_PLANE"),
        openborconstant("MESHMAP_AXIS_Z"));
    set_entity_property(self,
        openborconstant("ENTITY_PROPERTY_MESHMAP_MAX_SLOPE"), 45);
    set_entity_property(self,
        openborconstant("ENTITY_PROPERTY_MESHMAP_STEP_HEIGHT"), 4);
}
```

Walkability requires an upward-facing contact, the map's `WALKABLE` flag and
a slope permitted by **both** the map and entity limits. The slope is
measured from world up. Selecting a fixed movement plane projects the contact normal
into the permitted plane for that decision. The stored world contact normal
remains the original three-dimensional normal.

Native terrain movement holds the selected X, Y or Z coordinate fixed.
Selecting Y also holds the current altitude as the entity's base and clears
vertical velocity. The plane property selects the corresponding normal
projection for slope checks. Direct script assignments to positions remain
intentional teleports, and explicit legacy platform bindings retain their
own behavior outside this terrain constraint.

The native movement adapter uses the entity's reference X/Z position and a
vertical body interval for detailed movement checks. It can reject an
obstruction between feet and head along the movement route. This is
separate from the optional body box used by bank admission. It does not
sweep the complete frame hitbox volume as a general three-dimensional
character controller.

### One-way platforms

For a simple one-way platform above the first terrain patch:

```text
meshmap_index 0 0 0 1
meshmap y 160 80 96 0 0 data/terrain/flat16.png
meshmap_spacing 160 32

# One-way, drop-through, walkable collision; no artwork attached.
meshmap_flags collide walkable one_way drop_through
meshmap_rules 0 12 45 80
```

This platform spans X = 160..320, Z = 96..128, at Y = 80. The negative solid
side supplies an upward support face. The one-way rule permits upward
passage and ignores ordinary lateral solid blocking for that map; falling
onto its eligible top can establish support.

`DROP_THROUGH` permits a request; it does not choose an input binding.
Invoke the request when the game's control script has recognized the
desired command:

```c
int started_drop = set_entity_property(
    self, openborconstant("ENTITY_PROPERTY_MESHMAP_DROP_REQUEST"), 1);
```

The action returns 1 only when the entity's current support permits the
drop. Both `ONE_WAY` and `DROP_THROUGH` must be set on that support. The
adapter records an exception for that entity and support identity, starts
downward movement when necessary, and clears the exception after the entity
has cleared the relevant support.

Numeric support groups join pieces that should be ignored together during
that drop. Group 12 in the example can therefore span several platform
maps. The default `MESHMAP_GROUP_NONE` leaves each map independent. For
independent maps, ID and allocation generation are checked together so a
deleted/recreated ID cannot inherit an old exception. Unrelated lower
platforms remain eligible during the fall.

Setting `MESHMAP_DROP_REQUEST` to 0 cancels the active exception. Its getter
returns whether an exception is active, not the support map ID.

### Entity property reference

Prefix each suffix with `ENTITY_PROPERTY_MESHMAP_`.

| Suffix | Access | Meaning |
| --- | --- | --- |
| `ENABLED` | Read/write | Integer 0/1; default 1 |
| `MAX_SLOPE` | Read/write | Degrees 0..90; default 45 |
| `STEP_HEIGHT` | Read/write | Nonnegative world units; default 4 |
| `MOVEMENT_PLANE` | Read/write | `MESHMAP_PLANE_NONE` (-1), or an axis constant |
| `SUPPORT_ID` | Read | Current mesh support ID, or -1 |
| `SUPPORT_GROUP` | Read | Current support group, or `MESHMAP_GROUP_NONE` |
| `SUPPORT_GENERATION` | Read | Current support allocation generation |
| `SUPPORT_Y` | Read | Recorded support altitude |
| `DROP_REQUEST` | Read/write action | Request/cancel a permitted drop; getter indicates active exception |
| `DROP_ID`, `DROP_GROUP`, `DROP_GENERATION`, `DROP_Y` | Read | Recorded drop exception state |

Contact state is read-only. Read `SUPPORT_ID` or `DROP_ID` before treating
the corresponding recorded altitude as a current contact.

### Interaction with ordinary floors and platforms

For a mesh-enabled entity in a level containing at least one map, support
comes from authored terrain and explicit legacy basemaps, walls/platforms
and platform entities. The implicit Y = 0 floor no longer fills mesh holes
or prevents negative-height terrain from being reached. The pit cutoff
also takes the authored terrain's lower extent into account.

Creating an empty collection alone does not change legacy floor behavior.
The draft selects mesh terrain from the existence of map objects, so
disabling all active masks, turning off `COLLIDE` or retaining only a
drawing map does not restore the implicit floor. Author the needed ground
or disable mesh participation for the particular entity. Setting
`ENTITY_PROPERTY_MESHMAP_ENABLED` to 0 restores that entity's ordinary
legacy terrain behavior.

Existing moving platforms and solid entities retain their own roles.
Mesh maps are not a substitute for every entity-based interaction.

### Movement callbacks

Step and slope adjustments that change Y during native lateral movement
emit the existing `onmoveyscript` event. Rejected upward movement against a
mesh ceiling emits `onblockyscript` with `obstacle` set to `NULL()`, because
the obstruction is not a platform entity. Lateral mesh contacts do not
fabricate a legacy wall payload, and this draft has no separate
`onblockmesh` script API.

## Rendering in the native scene or a subscreen

### Automatic native drawing

Maps with active ancestry, artwork, present cells and `MESHMAP_DRAW` are
queued at their configured `LAYER`. The native projection is:

```text
screen_x = world_x - screenx + gfx_x_offset
screen_y = world_z - world_y - screeny + gfx_y_offset
```

Queue entries retain the level collection and numeric map ID. They resolve
the current resources when drawn. Removing a queued map, replacing its
artwork or clearing its `DRAW` flag before playback is therefore safe.

### Explicit draw functions

```text
draw_meshmap(collection, id, target_screen, drawmethod)
draw_meshmap(collection, id, target_screen, drawmethod, offset_x, offset_y)

draw_meshmap_projected(collection, id, target_screen, drawmethod,
    m00, m01, m02, m03,
    m10, m11, m12, m13,
    m20, m21, m22, m23 [, near_w])
```

The destination must be an RGB32 screen, such as one returned by
`allocscreen(width, height)`. These functions draw immediately into that
screen. They return 1 for a successful draw or an eligible no-op, and 0
when the native drawing operation is rejected. Invalid handles, argument
types and missing map IDs report script errors.

Pass a drawmethod object or `NULL()`. Here `NULL()` selects the plain
material; it does not inherit the common script drawmethod. Material and
sampling controls use the established renderer. Terrain placement comes
from the map and supplied view, not a drawmethod affine or curve transform.

Explicit draws honor active ancestry and available artwork, but do not
require `MESHMAP_DRAW`. Set `MESHMAP_PROPERTY_DRAW` to 0 for a map rendered exclusively
through a scripted view. Its `LAYER` controls the automatic scene entry
only; the caller chooses where a completed subscreen is queued.

Without a custom matrix, the projection is
`screen_x = X + offset_x`, `screen_y = Z - Y + offset_y`.
Both offsets must be supplied together.

### The 3x4 view

The twelve coefficients are row-major and transform world XYZ as follows:

```text
sx = m00*X + m01*Y + m02*Z + m03
sy = m10*X + m11*Y + m12*Z + m13
w  = m20*X + m21*Y + m22*Z + m23

screen_x = sx / w
screen_y = sy / w
```

All coefficients must be finite. `near_w` must be positive and defaults to
0.0001. Geometry is clipped against `w >= near_w` and the target's screen
edges before division. Texture coordinates retain the same W for
perspective-correct sampling.

For an X/Y platforming view that ignores Z visually, use rows
`(1,0,0,offset_x)`, `(0,-1,0,offset_y)`, `(0,0,0,1)`.
For a camera looking along +Z, a useful construction is:

```text
sx = focal*(X - camera_x) + center_x*(Z - camera_z)
sy = focal*(camera_y - Y) + center_y*(Z - camera_z)
w  = Z - camera_z
```

This changes the view of the authored world. Collision stays in the original
world axes, and sprites/entities need their corresponding view treatment
from the game.

### Reusable projected road screen

For the first Y-map example, set automatic drawing off during setup:

```c
set_meshmap_property(
    maps, 0, openborconstant("MESHMAP_PROPERTY_DRAW"), 0);
```

The following drawing script reuses one 320x240 screen. Its camera is at
`(512,180,-256)`, looking along +Z, with focal length 240 and image center
`(160,48)`:

```c
void main()
{
    void maps = get_level_property(
        NULL(), openborconstant("LEVEL_PROPERTY_MESHMAP_COLLECTION"));
    void surface = getglobalvar("meshmap_road_screen");

    if(surface == NULL())
    {
        surface = allocscreen(320, 240);
        setglobalvar("meshmap_road_screen", surface);
    }

    clearscreen(surface);
    draw_meshmap_projected(maps, 0, surface, NULL(),
         240,    0, 160, -81920,
           0, -240,  48,  55488,
           0,    0,   1,    256,
         1);

    // This queues the completed screen using the existing common drawmethod.
    drawscreen(surface, 0, 0, 100);
}
```

Call this from the game's normal drawing phase while map 0 exists. The
screen remains script-owned and must outlive any queued `drawscreen()`
entry. Release it through the game's usual script-resource teardown after
the relevant queue is no longer using it; do not free it immediately after
queueing the frame. Reset the stored global when disposing of it.

There is **no scene Z buffer**. Native layers, explicit draw order and
subscreen composition determine visibility. Perspective W controls
projection and texture interpolation, not automatic occlusion between
terrain, actors or overlapping maps. Self-overlapping projected surfaces
also follow submission order.

## Coding walkthrough

The implementation separates storage and geometry from engine policy in
the following order.

1. **Allocate addresses and apply admission rules.**
   `engine/source/gamelib/meshmap.[ch]` owns the lazy banks, masks, paths,
   samples, revisions and generations. Traversal checks active children and
   the supplied current-position/current-body gates before visiting a map.

2. **Decode authored resources without changing their meaning.**
   `engine/source/gamelib/meshmap_image.[ch]` reads PNGs through the existing
   packfile API and libpng dependency. Geometry stays grayscale16. Artwork
   becomes an ordinary indexed or RGB32 screen with exact image dimensions.
   Replacement data is validated before ownership changes.

3. **Construct the same surface for every consumer.**
   The core turns grid samples into fixed-axis vertices, caches whole-cell
   presence and uses one shared triangle diagonal. Its collision queries
   operate on eligible solid regions, including finite-domain and hole
   boundaries. Overlapping solid intervals are merged before exposed
   contacts are selected.

4. **Apply entity movement policy.**
   `engine/source/gamelib/meshmap_entity.[ch]` connects terrain contacts to
   support selection, vertical motion, X/Z movement, step/slope capability
   and per-entity drop exceptions. `engine/openbor.c` invokes this adapter
   at the existing movement/base lifecycle points. Map identities use
   generations so resource removal does not leave borrowed contact pointers.
   Lateral support is sampled at the float endpoint the entity will actually
   store, preventing tiny double/float height disagreements from generating
   repeated landings at large world coordinates. Unobstructed displacement
   retains its original request for the legacy movement-result comparison.
   Gravity's final floor clamp also revalidates mesh support identity, so a
   landing inside the engine's resting tolerance has a support ID immediately.
   That refresh requires the selected base to match the mesh's stored height;
   nearby terrain cannot claim a higher legacy wall or platform.

5. **Expose one native model to sheets and scripts.**
   `engine/source/openborscript/meshmap.[ch]` parses complete native
   declarations and implements typed map/bank properties and draw wrappers.
   `commands.[ch]`, `constants.c`, `config.h` and `index.c` register the
   commands, constants and script functions. `level.c` exposes the
   collection; `entity.[ch]` exposes entity capabilities and contact state.

6. **Project and submit artwork.**
   `engine/source/gamelib/meshmap_render.[ch]` visits present cells, projects
   and clips their triangles, and retains a reusable CPU geometry buffer.
   `gpu.[ch]` dispatches the prepared triangle list to
   `engine/sdl/gpu_renderer.[ch]`, which uses the shared native material,
   palette and vertex/index streams. `spriteq.[ch]` supplies the ID-based
   native queue bridge. Pixel sampling and rasterization stay in the GPU
   renderer; there is no per-triangle script or sprite loop.

7. **Release resources with their owner.**
   The level owns the collection and each map owns its geometry/artwork.
   The native queue is cleared before that collection is destroyed.
   Artwork release also retires its renderer resource, and reusable
   projection scratch storage is freed during the terrain shutdown path.

## Validation and practical limits

Run the targeted suites from the repository root. SDL2 and libpng
development headers are needed for the engine-facing fixtures; the pure
core suite has no SDL/PNG dependency.

```bash
bash tests/run_meshmap_tests.sh
bash tests/run_meshmap_image_tests.sh
bash tests/run_meshmap_script_tests.sh
bash tests/run_meshmap_entity_tests.sh
bash tests/run_meshmap_native_hook_tests.sh
OPENBOR_REQUIRE_GPU_TESTS=1 bash tests/run_meshmap_render_tests.sh
```

The render runner defaults to SDL's offscreen video driver. Select an
appropriate driver for the host if that driver is unavailable. Requiring
GPU tests turns an unavailable rendering context into a failing test
instead of an optional skip.

The same rendering fixture can exercise the shared GLES3 path on a host
with GLES development/runtime libraries:

```bash
OPENBOR_TEST_CFLAGS="-DOPENBOR_GLES=1" \
OPENBOR_TEST_SDL_CFLAGS="$(pkg-config --cflags sdl2 glesv2)" \
OPENBOR_TEST_SDL_LIBS="$(pkg-config --libs sdl2 glesv2)" \
OPENBOR_REQUIRE_GPU_TESTS=1 \
bash tests/run_meshmap_render_tests.sh
```

| Suite | What it checks |
| --- | --- |
| `test_meshmap.c` | Sparse IDs and banks, exact sample decoding/geometry, holes, gates, solids and boundaries, overlapping maps, slope projection and drop identity |
| `test_meshmap_image.c` | Real grayscale16/interlaced PNGs, preserved values, odd-width indexed/RGB art, corrupt/truncated inputs and atomic resource replacement |
| `test_meshmap_script.c` | Production level/property entry points, constants, integer validation, image loading and property mutation without starting a graphical game |
| `test_meshmap_entity.c` | Native movement-adapter outcomes for support, holes, slopes, side platforms, bounded steps, body-height obstructions, gates, fixed planes, one-way crests, disconnected platform pieces and drop-through |
| `test_meshmap_native_hooks.c` | Exact production base/movement functions with unrelated services isolated; explicit zero basemaps, negative mesh floors, direct movement, Y movement events and unchanged legacy floors |
| `test_meshmap_render.c` | Production renderer output against independent pixel expectations, all axes, holes, palette/RGB sources, perspective, near clipping, explicit/automatic visibility and queued resource changes |

The native-hook runner extracts the current `check_basemap`,
`check_basemap_index`, `checkbase`, `adjust_base` and `check_move` function
bodies directly from `engine/openbor.c` into a temporary test translation
unit. It does not maintain a second implementation of those functions.
Their unrelated platform/event services are isolated by the fixture; the
current suite checks 51 native outcomes without starting the full game.

The rendering suite has passed on desktop OpenGL and a verified OpenGL ES
3.2 host context: nine independently checked images and 36,814 pixel
comparisons per run. Expected pixels come from analytic rectangles and
ray/plane intersections, rather than calling the meshmap projection code
to predict its own results. Existing curve/material regression tests also
pass.

The core and PNG suites have passed AddressSanitizer and UndefinedBehaviorSanitizer
with leak detection disabled. LeakSanitizer could not initialize under the
test environment's ptrace restrictions. Host GLES execution uses the same
renderer path as Android, but it does not establish results on physical
Android hardware.

This draft intentionally retains several boundaries:

- Maps stay aligned to their selected axes and contain one displacement
  value per image vertex. Use multiple maps or entities for geometry that
  needs a different representation.
- Collision gates remain creator-authored current-state checks. They do
  not guarantee admission for every possible fast move.
- Detailed entity collision uses the reference X/Z and vertical body
  interval described above, not a general swept hitbox volume.
- Drawing walks grid cells in each active submitted map, projects present
  cells in native CPU code and streams clipped geometry through the existing
  renderer. There is no automatic mesh LOD,
  world streaming, spatial camera hierarchy or retained per-map world VBO.
- Collision side boundaries do not acquire automatic textured side faces.
- View projection does not add depth-buffer occlusion or automatically
  project every entity in a game.
- Tests establish specific numerical, resource, adapter and rendering
  behavior. No broad performance benchmark or complete game playthrough
  establishes that every scene will be faster than a wall-based design.

Choose grid spacing for the smallest terrain feature that needs to affect
movement, then use separate artwork resolution for visual detail. Split
large worlds into useful authored banks and maps. This keeps the intended
cheap admission checks useful while limiting the amount of detailed
geometry visited for a particular query or draw.
