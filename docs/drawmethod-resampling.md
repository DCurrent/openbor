# Drawmethod resampling

Author: Damon V. Caskey

Date: 2026-11-15

## One filter per draw

`DM_RESAMPLE` selects one reconstruction formula for an image draw. Scaling,
rotation, explicit affine transforms, shear, water and perspective planes use
the selected formula after determining the source coordinates. There are no
separate scale, rotation or affine filter overrides.

The default is `DC_RESAMPLE_NEAREST`. Resetting a drawmethod restores nearest;
copying it copies the selection. Queued draws capture the selection by value,
so changing the drawmethod afterward does not change an earlier queued draw.
Disabled drawmethods retain their stored selection but render with the normal
default method. Dots, lines and filled boxes have no image to reconstruct and
ignore this selector.

## Available formulas

Resolve the following names through `openborconstant()`. Numeric values are
listed for reference; scripts should use the named constants.

| Constant | Value | Formula and intended character | Maximum source positions per output pixel |
| --- | ---: | --- | ---: |
| `DC_RESAMPLE_NEAREST` | 0 | Existing exact nearest sampling; crisp pixel edges | 1 |
| `DC_RESAMPLE_BILINEAR` | 1 | Linear interpolation along both source axes | 4 |
| `DC_RESAMPLE_BICUBIC_CATMULL_ROM` | 2 | Cubic with B = 0, C = 1/2; sharper interpolation | 16 |
| `DC_RESAMPLE_BICUBIC_MITCHELL` | 3 | Cubic with B = 1/3, C = 1/3; balanced smoothing | 16 |
| `DC_RESAMPLE_BICUBIC_BSPLINE` | 4 | Cubic with B = 1, C = 0; softer reconstruction | 16 |
| `DC_RESAMPLE_LANCZOS_2` | 5 | `sinc(x) * sinc(x / 2)`, zero outside radius 2 | 16 |
| `DC_RESAMPLE_LANCZOS_3` | 6 | `sinc(x) * sinc(x / 3)`, zero outside radius 3 | 36 |
| `DC_RESAMPLE_MOVIE` | 1 | Alias of bilinear, matching the hardware movie interpolation formula | 4 |

Here `sinc(x) = sin(pi * x) / (pi * x)`, with `sinc(0) = 1`. Each filtered
formula is separable: its two-dimensional weight is the product of its X and
Y weights. Wider kernels are normalized at the actual fractional source
position. Negative lobes remain part of Catmull-Rom, Mitchell and Lanczos;
their reconstructed color and coverage are clamped to the output range.

The counts describe source positions, not total texture reads. Indexed
sources also resolve palette entries, masks require opacity reads, and some
material operations read a destination snapshot or blend table. Built-in
formulas run in the GPU shader; no script executes per pixel.

## Script API

The convenience API accepts a drawmethod handle or `NULL()` for common script
drawmethod state:

```c
// Select the spatial filtering used by hardware movies.
set_draw_method(NULL(), openborconstant("DM_RESAMPLE"),
                openborconstant("DC_RESAMPLE_MOVIE"));

// Select a different formula for subsequent image draws.
set_draw_method(NULL(), openborconstant("DM_RESAMPLE"),
                openborconstant("DC_RESAMPLE_BICUBIC_MITCHELL"));

int selected = get_draw_method(NULL(), openborconstant("DM_RESAMPLE"));

// Restore the ordinary default explicitly.
set_draw_method(NULL(), openborconstant("DM_RESAMPLE"),
                openborconstant("DC_RESAMPLE_NEAREST"));
```

The long-form property name is `DRAWMETHOD_PROPERTY_RESAMPLE`, used with
`set_drawmethod_property()` and `get_drawmethod_property()`. `DM_RESAMPLE` is
its shorthand. These are scalar controls: setters take exactly one integer
selector and getters take no component argument. Undefined selectors,
decimal values, strings, pointers and `NULL()` values are rejected without
changing the previous selection. `NULL()` remains valid as the convenience
API's target, as shown above.

The existing entity/global API also supports the property name `"resample"`:

```c
changedrawmethod(entity, "resample",
                 openborconstant("DC_RESAMPLE_BICUBIC_CATMULL_ROM"));
changedrawmethod(NULL(), "resample",
                 openborconstant("DC_RESAMPLE_BILINEAR"));
int selected = getdrawmethod(NULL(), "resample");
```

The selected setting does not enable a disabled drawmethod. Existing
drawmethod configuration, lifetime and draw submission rules still apply.

## Coordinates and borders

Nearest preserves the existing transform-specific integer selection,
including fixed-point scale/shear rules and exact affine boundary decisions.
Opt-in filters instead place their kernels at continuous source positions
aligned to destination pixel centers. Source texel centers are half a pixel
inside their source pixel edges, matching the usual texture filtering
convention.

Scale and shear retain their established covered destination spans, while
their source coordinates use the true reciprocal scale. Clipping does not
restart the filter's fractional phase. Rotation retains its existing
angle/flip convention and maps destination pixel centers for filtering.
Explicit affine uses its source-to-destination matrix and exact source
boundary classification.

Water retains its quantized wave phase and rounded displacement. Filtering
changes reconstruction of the source image, not the authored water motion.
Planes retain the existing row recurrence, with continuous source Y and a
half-step center offset preserved in the cached row data. Nearest and filtered
draws share those row maps without making the filter part of the cache key.

Ordinary images clamp each contributing source position to the logical image
edge. Plane textures wrap each position independently across both seams.
Filtering does not expand the image's geometric coverage into an antialiased
silhouette; source transparency inside the covered image can still produce
fractional coverage.

## Palettes, transparency and material order

Each contributing texel is resolved before colors are combined:

1. Fetch the exact source texel or palette index.
2. Apply requested zero-key transparency. Sprite index zero remains a hole.
3. Resolve indexed color through its current palette/remap.
4. Test Chroma against that exact resolved color.
5. Include mask opacity, when present, in the contributing coverage.
6. Accumulate coverage-weighted color and coverage with the selected kernel.
7. Reconstruct RGB, clamp and round it to byte operands, then apply the
   existing fill, tint, blend and channel-opacity rules.
8. Composite partial coverage against the completed GPU destination and
   apply Substitute to the final quantized RGB result.

Zero-coverage samples cannot contaminate neighboring colors. Palette indices
and blend-table addresses are never interpolated. Fill still respects source
coverage. Mask precedence is unchanged: a mask controls opacity directly and
supersedes tint/blend/channel-opacity choices. Its existing 255-complement,
256-divisor equation remains in effect. Fully transparent filtered samples
leave the destination untouched, including fully zero masks; nearest retains
its existing arithmetic exactly.

RGB32 screen storage still has no persistent alpha channel. Filtered partial
coverage is composed into the current destination. Reusing that finished
screen uses its stored RGB and its next draw's transparency settings.

## Movie relationship

The existing hardware movie presenter applies `GL_LINEAR` independently to
the Y, U and V textures before converting to RGB and applying display color
correction. `DC_RESAMPLE_MOVIE` names that bilinear interpolation formula;
it is the same selector value as `DC_RESAMPLE_BILINEAR`.

Selecting this alias on an RGB/indexed scene draw does not invoke YUV
conversion or make quantized RGB interpolation identical to filtering YUV
planes first. Movie playback now uses a shared GPU YUV renderer and exposes
`MOVIE_PROPERTY_RESAMPLE` with the same constants, defaulting to
`DC_RESAMPLE_MOVIE`. That renderer reconstructs each plane before conversion,
retaining the existing hardware coefficients and direct fullscreen correction
order. See [GPU movie playback](gpu-movie-playback.md) for script examples,
screen/fullscreen contracts, capture, and platform availability.

## Availability and limits

The desktop native OpenGL scene renderer implements these formulas for RGB32
destinations, indexed sprites, indexed source screens and RGB32 source
screens. GPU-produced screens are sampled directly without CPU readback.
No new software resampler is provided.

Explicit filtered draws that cannot execute natively are logged and skipped.
They do not silently become nearest or fall back to CPU pixel processing.
This includes the unmigrated Android scene renderer and configurations still
using CPU output filters. Their existing nearest behavior remains available
until those rendering paths are migrated separately. Startup capability
logging reports native resampling availability.

These are fixed-support reconstruction filters. Strong reduction can still
alias; footprint-aware area filtering, mipmaps and anisotropic sampling are
separate work. Wider filters can ring near sharp transitions and cost more
source reads. Pixel-art enlargement algorithms and custom formulas are not
part of this selection.

## Verification

`tests/run_gpu_resample_tests.sh` compares production native output against
independent long-double kernels and inverse mappings. It covers every formula,
source formats, transparency/material combinations, transform families,
clipping, queue snapshots and GPU-resident source reuse. The full
`tests/run_gpu_tests.sh` includes this suite alongside the exact nearest,
capability, lifecycle, state-cache and presentation regressions. Script API
validation is included in `tests/run_drawmethod_color_script_tests.sh`.

Formula references: [PBRT image reconstruction](https://pbr-book.org/4ed/Sampling_and_Reconstruction/Image_Reconstruction)
and [GPU Gems 2, high-quality filtering](https://developer.nvidia.com/gpugems/gpugems2/part-iii-high-quality-rendering/chapter-27-advanced-high-quality-filtering).
