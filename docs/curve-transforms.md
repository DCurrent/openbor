# Curve transforms

Author: Damon V. Caskey

Date: 2026-11-20

## Scope

The drawmethod can bend a sprite or source screen into a continuous surface.
The same controls support a cylindrical column, an inward half pipe, a local
horizon bend, or a road that turns within the screen plane. Creators select
the bend direction, bend plane, total turn, position and extent. Perspective
and tilt can then project that surface into the destination.

This is an image drawmethod. Ordinary `drawsprite()`, `drawspritetoscreen()`
and `drawscreen()` calls use it through their existing drawmethod state;
entities can keep it in their own drawmethod. It does not change collision,
world positions, sprite-queue ordering or gameplay simulation. Composing two
bends uses ordinary screens and draw calls, with one bend on each draw.

Scene pixels are rasterized and sampled by the shared desktop OpenGL and
Android GLES3 renderer. Native CPU code prepares a bounded mesh of vertices;
it does not redraw the source image, run a pixel warp, or require a script
loop over image rows. Unsupported native draws are logged and skipped without
a software scene-rendering retry.

## Script interface

The named API accepts one complete curve in the following order:

```c
changedrawmethod(entity, "curve",
    angle, axis, plane, focus, extent, perspective, tilt,
    profile, scrollx, scrolly, wrap);
```

Pass `NULL()` as the target to change common script drawing state. Exactly
eleven values are required. The operation validates the entire curve before
changing any stored field or enabling the curve flag. Examples below assume
the other drawmethod settings have their ordinary enabled defaults; setting
the curve preserves those other settings.

For example, this creates a cylindrical bend across a sprite's width, with a
120-degree total turn and a camera distance of twice the source's larger
dimension:

```c
changedrawmethod(NULL(), "curve",
    120, 0, 90, 0.5, 1, 2, 0,
    openborconstant("DC_CURVE_PROFILE_UNIFORM"),
    0, 0, openborconstant("DC_CURVE_WRAP_CLIP"));

drawsprite(sprite, 160, 120, 100);

// Later queued draws can use the ordinary path again.
changedrawmethod(NULL(), "curveenabled", 0);
```

The constant-based convenience API accepts either a drawmethod handle or
`NULL()` for common drawing state:

```c
set_draw_method(NULL(), openborconstant("DM_CURVE"),
    120, 0, 90, 0.5, 1, 2, 0,
    openborconstant("DC_CURVE_PROFILE_UNIFORM"),
    0, 0, openborconstant("DC_CURVE_WRAP_CLIP"));

set_draw_method(NULL(), openborconstant("DM_CURVE_FOCUS"), 0.65);
int enabled = get_draw_method(NULL(), openborconstant("DM_CURVE_ENABLED"));
```

The object API uses the same property constants and value order:

```c
set_drawmethod_property(method, openborconstant("DM_CURVE"),
    120, 0, 90, 0.5, 1, 2, 0,
    openborconstant("DC_CURVE_PROFILE_UNIFORM"),
    0, 0, openborconstant("DC_CURVE_WRAP_CLIP"));

float focus = get_drawmethod_property(method, openborconstant("DM_CURVE_FOCUS"));
```

`changedrawmethod()` takes an entity or `NULL()`; it does not take a
drawmethod handle. `set_drawmethod_property()` requires a valid drawmethod
object. The convenience `set_draw_method()` API provides the explicit
object-or-common-state form.

### Properties and defaults

Resolve constant names through `openborconstant()`. Existing numeric property
IDs retain their values. The listed IDs are the object/convenience API IDs;
named legacy properties have their own existing lookup table.

| Named property | Constant | ID | Values and units | Default |
| --- | --- | ---: | --- | ---: |
| `curve` | `DM_CURVE` | 46 | Setter takes all eleven fields; getter returns the enable flag | 0, disabled |
| `curveenabled` | `DM_CURVE_ENABLED` | 47 | Integer 0 or 1 | 0 |
| `curveangle` | `DM_CURVE_ANGLE` | 48 | -180 through 180 degrees of total tangent turn | 0 |
| `curveaxis` | `DM_CURVE_AXIS` | 49 | -360 through 360 degrees in source XY | 0 |
| `curveplane` | `DM_CURVE_PLANE` | 50 | -360 through 360 degrees around the bend direction | 90 |
| `curvefocus` | `DM_CURVE_FOCUS` | 51 | -4 through 5, in normalized source-axis extent | 0.5 |
| `curveextent` | `DM_CURVE_EXTENT` | 52 | 0.01 through 16, in normalized source-axis extent | 1 |
| `curveperspective` | `DM_CURVE_PERSPECTIVE` | 53 | 0 through 64; 0 is orthographic, positive values set camera distance | 0 |
| `curvetilt` | `DM_CURVE_TILT` | 54 | -89 through 89 degrees of bend-axis/depth rotation | 0 |
| `curveprofile` | `DM_CURVE_PROFILE` | 55 | Integer profile selector, 0 or 1 | 0 |
| `curvescrollx` | `DM_CURVE_SCROLL_X` | 56 | -10,000,000 through 10,000,000 source texels | 0 |
| `curvescrolly` | `DM_CURVE_SCROLL_Y` | 57 | -10,000,000 through 10,000,000 source texels | 0 |
| `curvewrap` | `DM_CURVE_WRAP` | 58 | Integer addressing selector, 0 or 1 | 0 |

Every `DM_CURVE...` name has the equivalent long-form name with
`DRAWMETHOD_PROPERTY_` replacing `DM_`. For example, `DM_CURVE_SCROLL_X`
aliases `DRAWMETHOD_PROPERTY_CURVE_SCROLL_X`, and `DM_CURVE` aliases
`DRAWMETHOD_PROPERTY_CURVE`. Both forms are exported script constants.

| Constant | Value | Meaning |
| --- | ---: | --- |
| `DC_CURVE_PROFILE_UNIFORM` | 0 | Constant curvature through the active bend; circular cross-section |
| `DC_CURVE_PROFILE_SMOOTH` | 1 | Curvature rises smoothly to its maximum at the focus, then falls to zero |
| `DC_CURVE_WRAP_CLIP` | 0 | Discard source positions outside the image |
| `DC_CURVE_WRAP_REPEAT` | 1 | Repeat source texels in both directions across the same geometry |
| `DRAWMETHOD_CONFIG_CURVE` | 128, bit 7 | Curve enable bit in drawmethod configuration |

Geometric fields accept integer or decimal script values and are stored as
floats. Enable, profile and wrap require exact integer selectors; decimal
`1.0` is rejected for those fields. NaN, infinity, out-of-range values,
strings, pointers and `NULL()` values are rejected. The extent minimum is
represented as `0.01f` internally, so its getter result remains valid when
written back.

Individual setters take exactly one value, validate the resulting complete
curve, and enable the curve flag after success. Every getter takes no extra
component argument. `getdrawmethod(entity, "curve")` returns the enable flag;
read the individual properties to obtain the stored parameters. Enable,
profile and wrap getters return integers; geometric getters return decimals.

Disabling `curveenabled` preserves all parameters. Resetting a drawmethod
restores the defaults above, and copying a drawmethod copies the complete
curve by value. Queued draws also capture their own parameter snapshot.
Curve setters do not enable the drawmethod's separate general `enabled`
flag. If that general flag is off, ordinary default drawing applies.

## Choosing a bend

### Source direction and bend plane

Source X increases to the right and source Y increases downward.
`curveaxis` selects the direction **along which the bend changes**. At 0
degrees the bend develops across the image width; at 90 degrees it develops
down its height. Intermediate values permit diagonal bends. This direction
is different from the long axis of a cylindrical column: bending across X
with a depth-facing plane creates a cylinder whose long axis follows Y.

`curveplane` chooses where the cross-section bends:

| Plane | Effect |
| --- | --- |
| 0 degrees | Bend within source XY toward the perpendicular source direction; suitable for a flat road turn |
| 90 degrees | Bend out of the source plane into depth; suitable for columns, pipes and horizon rolls |
| 180 degrees | Bend within source XY toward the opposite perpendicular direction |
| -90 or 270 degrees | Bend out of the source plane toward the opposite depth direction |
| Intermediate angles | Combine an in-plane turn and an out-of-plane bend |

The sign of `curveangle` reverses the bend. With the default 90-degree plane,
positive angle bends the edges away from the neutral tangent, while negative
angle bends them in the opposite depth direction. Camera projection is
relative to the selected draw pivot, as described below.

For an in-plane bend, cross-sections rotate with the centerline. This preserves
the road's local width instead of merely shifting each row sideways. For a
90-degree depth bend, the perpendicular source direction remains a straight
extrusion, producing the cylindrical surface.

### Focus, extent and profile

`curvefocus` locates the middle of the active bend. At 0 the focus is at the
minimum source-axis edge, at 0.5 it is at the source center, and at 1 it is at
the maximum source-axis edge. Values outside that interval move the bend
beyond the image. The active bend spans `curveextent / 2` on either side of
the focus, measured in the same normalized coordinates.

For source width `W`, height `H`, and axis angle `a`, the source-axis span is:

```text
span = abs(cos(a)) * W + abs(sin(a)) * H
focus_distance = (curvefocus - 0.5) * span
bend_length = curveextent * span
```

`curveangle` is the total change of tangent direction across this active
interval. Its tangent is neutral at the focus and reaches minus/plus half
the total angle at the two ends. Beyond those ends, the surface continues
along the ending tangents. It does not resume an unrotated flat image or
repeat the bend. If the active interval extends beyond the source, only the
portion intersecting the source is drawn.

The uniform profile preserves source distance along a circular cross-section.
For nonzero angle in radians, its signed radius is `bend_length / angle`.
This gives a regular circular shape without introducing a special compression
band at the image center. It still has the normal foreshortening produced by
projecting a curved surface.

The smooth profile distributes the same total turn through the cubic
smoothstep tangent-angle function. If `t` runs from 0 to 1 through the active
interval, its tangent angle is `angle * (t*t*(3 - 2*t) - 0.5)`. Curvature is
zero at the ends and greatest at the focus. This profile is useful when a
horizon should fall away in one movable region or a road turn should join
straight sections smoothly. It is deliberately not a circular cross-section.

Moving the focus moves the region of strongest bending in the smooth profile.
Reducing extent concentrates the turn; increasing extent spreads it across
more source distance. These are normal animatable properties, so a horizon
drop or changing turn can be controlled by parameter updates.

### Perspective and tilt

`curveperspective = 0` selects orthographic projection. Positive values set a
camera distance in units of `max(W, H)`. For example, a value of 2 means a
distance of `2 * max(W, H)` source texels. Larger values produce weaker
perspective; small positive values produce stronger perspective and can put
part of the surface behind the near plane. Zero is the explicit orthographic
mode, not a zero-distance camera.

After forming the curve and subtracting its mapped pivot, `curvetilt` rotates
the bend-axis direction against depth. It can tip a flat or bent surface
toward or away from the viewer. With axis 90 degrees, a negative tilt makes
the top of a centered surface recede, useful as a starting orientation for
a road or floor.

Positive depth recedes from the camera. The perspective divisor is
`w = 1 + depth / camera_distance`; projected XY is divided by `w`.
The GPU receives this divisor for perspective-correct texture interpolation,
so a plane does not acquire a separate affine texture mapping in each triangle.
These depths are internal to the draw. They do not add a scene depth buffer,
sort entities in 3D or change painter order.

## Pivot and transform order

The mapped source pivot remains attached to the draw position while curve
parameters change. The pivot is the sprite's authored center plus drawmethod
center offsets. Source screens have an intrinsic pivot of `(0, 0)`; set
`centerx` and `centery` to half the screen dimensions when the bend should
stay anchored at that screen's center.

The enabled curve path applies these operations in order:

1. Form the curved surface from source coordinates, axis, plane, focus,
   extent, angle and profile.
2. Subtract the mapped source pivot in XYZ, then apply tilt and perspective.
3. Apply flip flags and the enabled affine matrix, including its translation.
   When affine is off, use the legacy scale/rotation placement, or `shiftx`
   shear when rotation is zero.
4. Place the result at the draw position and apply target-space clipping.
5. Sample source texels and apply the existing material/composition rules.

Curve followed by affine is supported in one draw. Affine retains precedence
over legacy scale, rotation and shear. Stored water/sine/legacy-plane geometry
is dormant while curve is enabled. Its values are preserved and resume under
the usual transform precedence when the curve is disabled.

With curve enabled and affine/rotation off, `shiftx` is a continuous shear
about the mapped pivot: `x -= (shiftx / 256) * postscaled_y`. This opt-in
placement differs from the ordinary shear path's bottom-row offset convention.
Disabling the curve restores that ordinary convention.

Repeats and spans continue to place complete instances in their original
order. Palette/remap selection, zero-key transparency, masks, fill, tint,
blends, channel opacity, Chroma and Substitute remain material controls.
`DM_RESAMPLE` chooses the reconstruction formula for the curved draw; see
[drawmethod resampling](drawmethod-resampling.md). No per-curve filter selector
is added. Video Options Final Filter and display sizing remain independent
[presentation controls](video-controls.md).

Curves are off by default. With the curve bit off, the existing geometry and
sampling path remains unchanged. Enabling a zero-angle curve creates a flat
continuous surface that can still have tilt, perspective, scrolling or affine
placement. Disable the curve to return to the ordinary transform path.

## Texture motion and wrapping

`curvescrollx` and `curvescrolly` add source texel offsets after geometry is
formed. Their units are image pixels regardless of draw scale or perspective;
fractional values are allowed. Positive X offset samples farther right in the
source, so the pattern travels left over the stationary surface.

Use texture scrolling to move markings along a road or a seamless pattern
around a column without changing its silhouette. Scrolling does not move the
focus, rotate the surface, enlarge it, or add more geometry.

With `DC_CURVE_WRAP_CLIP`, source positions outside the logical image are
discarded. Filter taps for valid positions clamp to the source edge. With
`DC_CURVE_WRAP_REPEAT`, both coordinates repeat through the source image;
negative scroll wraps correctly and each filtered tap wraps independently
across the seam. Wrap changes texture addressing only. It does not build a
closed 360-degree cylinder or repeat the geometric bend.

## Applying curves to screens

The four-argument `drawscreen()` form inherits common drawmethod state:

```c
changedrawmethod(NULL(), "curve",
    -150, 90, 90, 0.5, 1, 3, 0,
    openborconstant("DC_CURVE_PROFILE_UNIFORM"),
    0, 0, openborconstant("DC_CURVE_WRAP_CLIP"));

drawscreen(surface, x, y, z);
changedrawmethod(NULL(), "curveenabled", 0);
```

The optional fifth legacy blend argument makes `drawscreen()` construct its
own plain drawmethod. Omit it when using curve state, and set blending on the
drawmethod itself when needed.

For a turning road with a separate horizon roll, first draw the road with
an in-plane curve into a normal RGB32 screen. Then draw that screen with a
depth-facing curve. Existing `drawspriteq()` capture, screen allocation and
screen reuse provide this composition. Each pass is an ordinary draw and
each screen remains GPU-resident until an explicit CPU image read requests
synchronization. Creators decide which objects belong in each pass; a HUD
drawn afterward need not follow the background's curve.

Screen storage remains RGB32 without persistent alpha. Existing transparency
and blending rules still govern reuse of a completed screen. Applying a curve
to a screen changes only that screen draw; it does not modify its source
pixels or the stored positions of objects used to build it.

## Visibility and geometric limits

The curve is a single visible textured surface. Back-facing and zero-area
triangles are culled; flips and reflected affine placement are accounted for
so an ordinary mirror does not wrongly hide the front. This is not a closed
solid with an automatically rendered rear wall. Total turn is limited to
180 degrees in either direction.

Tight in-plane bends can make a wide strip fold over its inner edge. The
renderer rejects that entire geometric draw. In the local bend frame, it
requires `1 - curvature * transverse_distance * cos(plane)` to stay greater
than `1e-8` across the surface. Reducing the angle, increasing the extent or
using a narrower source avoids this fold. Cylindrical bends at 90 degrees do
not have this in-plane width constraint.

Perspective polygons are clipped at `w = 0.0001` before division. The target
bounds and an active drawmethod clip rectangle also clip the geometry, with
source coordinates and perspective interpolation retained. Fully clipped or
singular placement produces an empty draw. Invalid parameters, fold failures,
unrepresentable coordinates, allocation failure or exceeded preparation bounds
reject the draw; they do not switch to software rendering.

The tessellation targets at most 0.25 destination pixel of sampled midpoint
deviation per band, including longitudinal edge/centerline and triangle
diagonal probes, and limits the tangent-angle change to 5 degrees per band.
This is an adaptive approximation, not an analytic per-pixel intersection or
a proof of a uniform error bound over every point. Strong magnification or a
near-plane approach requires more geometry. The preparation limit is 8,192
quad records per instance and a subdivision depth of 18; clipped triangles
also consume quad records. Exceeding either bound rejects the draw instead
of silently lowering quality.

Source reconstruction uses the selected fixed-support filter. Strong
foreshortening can still alias because footprint-aware mipmaps, area filters
and anisotropic sampling are separate capabilities. Filtering the source
does not antialias the outer geometric silhouette.

## Work and reuse

Native preparation evaluates a continuous cross-section and emits adaptive
bands sharing their edges. It prepares positions, source coordinates and
perspective divisors only. Rasterization, palette lookup, source filtering,
mask coverage and color composition execute on the GPU. There is no
source-image readback just to bend a resident sprite or screen.

The renderer retains at most eight geometry-cache entries, each within the
mesh bounds above. Source and target dimensions, pivot, placement, clipping
and geometric parameters determine reuse. Changing the source image contents,
palette, material, resampling, texture scroll or wrap does not require a new
shape when the geometric inputs match. Animating the bend itself or moving
the draw can require fresh preparation. Storage is reused and cleared with
the renderer's context lifecycle.

Repeated instances are geometrically preflighted before the first destination
write and submitted in ordinary painter order. More than eight distinct
instances can evict and rebuild cached geometry. The limit bounds retained
work; it is not an unlimited instance cache or a cross-draw batching promise.

CPU setup, vertex uploads, drawn area, overlap and filter support all contribute
to cost. Native curves remove per-row scripting and CPU pixel warping while
retaining those ordinary rendering costs. Measure representative animated
surfaces on the intended hardware.

## Validation entry points

Run the production script-API fixture from the repository root:

```sh
bash tests/run_drawmethod_color_script_tests.sh
```

It checks named, object and convenience APIs, exported constants, atomic
failure, ranges and types, enable preservation, copying, reset and minimum
extent round trips. Its existing color/resampling checks remain in place.

Native geometry has a separate fixture:

```sh
bash tests/run_gpu_curve_geometry_tests.sh
```

The geometry checks use closed-form circular expectations and an independent
high-resolution integration oracle for the smooth profile. They do not link
a software scene rasterizer. The focused real-renderer fixture is:

```sh
OPENBOR_REQUIRE_GPU_TESTS=1 bash tests/run_gpu_curve_tests.sh
```

It compares circular surfaces and planar roads with independent inverse
projections, checks every pixel of flat identity and texture-phase cases,
and exercises palette/material reuse, queued state, GPU-resident two-pass
screens, overlapping self-copies, atomic rejection and context recreation.
The GLES runner uses the same fixture and expectations.

Run the renderer suite for shared rendering and platform integration checks:

```sh
OPENBOR_REQUIRE_GPU_TESTS=1 bash tests/run_gpu_tests.sh
```

These entry points describe regression coverage, not a guarantee of hardware
performance. Platform builds and representative module testing remain
necessary before a release.
