# Affine transforms

Author: Damon V. Caskey

Date: 2026-11-20

The drawmethod supports an opt-in, six-coefficient affine transform for sprites
and screens. The native renderer draws supported affine operations directly
on the GPU. OpenBOR scene targets use RGB32 only. Desktop and Android scenes
require the native renderer; unsupported affine draws are logged and skipped
without a CPU rendering retry. GPU output filters do not change that policy.
No intermediate screen is required just to apply an affine transform.

Hardware capability warnings appear in the ordinary engine log during video
initialization, even when profiling is disabled. If native affine support is
unavailable, the startup report identifies the cause and states that affected
draws are rejected without a software retry. See the
[startup capability report](gpu-renderer-migration.md#startup-capability-report)
for the general renderer checks and drawing limits.

This is a reusable engine capability. Perspective projections, nonlinear curves,
public programmable materials and 24-bit asset loading remain separate work.

## Script interface

Call `changedrawmethod(entity, "affine", m00, m01, m10, m11, tx, ty)` to
set all six coefficients in one validated, atomic operation. This also enables
the new affine path. Pass `NULL()` instead of an entity to use the current
script draw method (for `drawsprite` or `drawscreen`).

For example, an identity transform:

```c
changedrawmethod(NULL(), "affine", 1, 0, 0, 1, 0, 0);
```

The coefficients map source coordinates relative to the draw pivot into the
destination screen:

```text
dest_x = draw_x + tx + m00 * local_x + m01 * local_y
dest_y = draw_y + ty + m10 * local_x + m11 * local_y
```

Here `local_x = source_x - (sprite_centerx + drawmethod_centerx)` and likewise
for Y. Source screens use `(0, 0)` as their built-in pivot. The `flipx` and
`flipy` flags, when enabled, negate the corresponding **local** source axes
before applying the matrix. The matrix operates on continuous coordinates;
the renderer samples the source at destination pixel centers. Nearest neighbor
is the default; the selected [drawmethod resampling](drawmethod-resampling.md)
formula also applies to affine draws.

Some examples:

```c
// Stretch horizontally by 150% and vertically by 75%.
changedrawmethod(NULL(), "affine", 1.5, 0, 0, 0.75, 0, 0);

// Shear horizontally by 0.4 units for every unit down the image.
changedrawmethod(NULL(), "affine", 1, 0.4, 0, 1, 0, 0);

// Rotate 90 degrees clockwise in screen coordinates (Y grows downward).
changedrawmethod(NULL(), "affine", 0, -1, 1, 0, 0, 0);

// Move the transformed image 12 pixels right and 8 up.
changedrawmethod(NULL(), "affine", 1, 0, 0, 1, 12, -8);

// Return to normal rendering without discarding stored coefficients.
changedrawmethod(NULL(), "affineenabled", 0);
```

For animation, individual coefficients are accessible with
`changedrawmethod(entity, property, value)` and
`getdrawmethod(entity, property)`. Supported property names are `affine00`,
`affine01`, `affine10`, `affine11`, `affinex`, and `affiney`. Setting any
coefficient enables affine rendering. `affineenabled` controls that flag;
`getdrawmethod(entity, "affine")` is another way to read its value.

## Interaction with the existing renderer

- Affine rendering is off by default. The ordinary screen and sprite fast
  paths are unchanged unless it is explicitly enabled.
- When enabled, affine takes precedence over legacy `rotate`, `scalex`,
  `scaley`, `shiftx`, and water/perspective transforms. Their stored values
  remain intact and take effect again when affine is disabled. This also
  applies when a saved legacy scale is zero.
- Draw position, source pivot, extra center offsets, flip flags, repeats,
  clipping, transparency, palette remapping, fill, tint and blending still
  participate in drawing. Affine only replaces **geometry**.
- Singular matrices (determinant zero) draw nothing. Invalid non-finite
  numbers are rejected by the script setter. Direct native calls also reject
  invalid coefficients before writing destination pixels.
- All existing numeric drawmethod property IDs retain their values. New
  property names are resolved through an alphabetized lookup table without
  renumbering the older properties.
- The drawmethod's general `enabled` flag still controls whether any of its
  settings apply. Toggling `affineenabled` preserves all six coefficients.

## GPU execution and CPU image access

Hardware drawing keeps source pixels resident through the existing renderer.
The CPU prepares one clipped bounding quad and a small amount of sampling state;
the GPU evaluates the affine mapping, selects source texels and performs the
supported material operations. There is no CPU loop transforming the source
image into another image for every native draw.

Affine sampling has no blanket near-integer snapping tolerance. Integer byte
comparisons of source numerators and the determinant preserve exact scale
boundaries and authored fractional positions. Numerically extreme mappings are
rejected without a CPU retry; see
[the renderer guide](gpu-renderer-migration.md#color-and-intended-behavior-corrections)
for the native arithmetic and uniform-capacity bounds.

Repeated affine draws validate every instance before any destination write.
Reusable storage retains up to 8,192 prepared quads and their exact sampling
parameters, removing the second preparation pass for those grids. Submission
still follows the original repeat order, including individual destination
snapshots for overlapping blends and self-drawing. Larger grids or unavailable
cache storage retain preflight and recomputation while rendering pixels on the
GPU. The cache changes preparation work, not the affine API or sampling rules.

Explicit CPU image reads and edits still use the existing synchronization
contract. Reading GPU-owned pixels resolves the current image first; editing
its CPU copy invalidates the uploaded version. Asset decoding and these
requested image operations remain available. Production code contains no CPU
affine rasterizer, and rejected native draws do not request a rendering
readback or execute a software retry.

Hardware transforms still consume draw submission, rasterization and fill
resources. Their cost depends on drawn area, overlap, effects, batching and
preparation; affine support does not imply that all drawing becomes
free or that CPU world simulation moves onto the GPU.

## Separate future capabilities

Affine uses a constant 2x3 mapping, which preserves straight lines and
parallelism. Perspective requires a separate projective mapping and sampling
contract. Nonlinear curves require their own warp parameters. Possible curve
operations include:

1. **Road/path curve** - progressively shift or rotate horizontal strips
   around a supplied path, suitable for a racing-game floor or roadway.
2. **Cylindrical bend** - curve a vertical image about an axis for columns,
   rounded walls and similar surfaces.

These capabilities should compose with the renderer's material operations
without overloading the affine coefficients or changing legacy behavior.

Future 24-bit assets require a true-color asset and texture path plus explicit
recoloring semantics. Palette indices carry color-group identity that an RGB
image does not automatically preserve. Programmable materials can support
color transforms, lookup textures or author-supplied masks when that work is
introduced. The affine interface only supplies geometry and does not assume
that every source uses an indexed palette.

## Validation

Run the renderer suite from the repository root:

```sh
OPENBOR_REQUIRE_GPU_TESTS=1 bash tests/run_gpu_tests.sh
```

The suite compares desktop OpenGL and Android GLES3 output with independent
scalar expectations in `tests/test_gpu_transform_gl.c`. Those checks evaluate
the inverse mapping at each destination pixel center without calling a
production CPU rasterizer. Test-only formulas for the other transform families
live in `tests/gpu_transform_reference.h`; neither file is linked into the
engine. The former standalone CPU affine test and its production rasterizer
have been removed.

For a focused GLES3 transform run:

```sh
OPENBOR_REQUIRE_GPU_TESTS=1 OPENBOR_GLES_TEST_CASES=transform_gl \
    bash tests/run_gles_scene_tests.sh
```

The tests cover identity, source pivot and translation, flips, right-angle
rotation, non-uniform scale, shear with clipping, fractional sampling, exact
texel boundaries, adjacent half-pixel translations, threefold scale with
pivots, collapsed matrices, offscreen transforms and invalid floating-point
input. The full renderer suite also checks native material/coverage behavior,
repeat preparation reuse, and unchanged RGB32 destinations after rejected
draws or backend loss. Indexed source-asset operations and explicit CPU access
are tested separately from scene rendering. Requiring GPU tests prevents a
missing graphics context from silently skipping native coverage. Measure
performance using representative sprites, screens and effects on the target
hardware.
