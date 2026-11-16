# GPU output filters

Author: Damon V. Caskey

Date: 2026-11-16

## Selection and scope

Video output filters now execute on the GPU on Windows, Linux, macOS and
Android. OpenBOR scene screens remain RGB32, and applications remain 64-bit.
Selecting a filter on desktop keeps scene composition in the native renderer.
The former CPU filter callbacks, color-conversion surfaces and reduced-color
intermediate are removed.

Existing saved selections retain their numeric values:

| ID | Internal selector | Menu name | Effect |
| --- | --- | --- | --- |
| 0 | `BLITTER_NONE` | Disabled | Present the original scene without a filter pass |
| 1 | `BLITTER_BILINEAR` | Bilinear | Existing forward-neighbor interpolation with explicit integer rounding |
| 2 | `BLITTER_ADMAME2X` | Advance Mame2x | Scale2x edge selection using four orthogonal neighbors |
| 3 | `BLITTER_SCANLINES` | Scanlines | Duplicate source pixels horizontally; make alternating output rows black |
| 4 | `BLITTER_SCANLINESTV` | Scanlines TV | Interpolate horizontally; darken alternating rows |
| 5 | `BLITTER_TV2X` | TV 2x | Duplicate source pixels; reduce alternating rows to seven eighths |
| 6 | `BLITTER_DOTMATRIX` | Dot Matrix | Duplicate source pixels and apply the repeating color-channel dot pattern |

Every enabled filter produces a texture exactly twice the source width and
height. Disabled selection is a true bypass; it does not allocate a 2x target
or submit a filter draw. There is no separate Simple2x menu effect at ID 0.

The existing activation rule remains: the selected filter runs when its ID is
nonzero and either the window scale is at least 2x or fullscreen mode is
selected. Below 2x in a window, the selection remains stored but inactive.
Unknown IDs are rejected. Logs show selection and activity separately.

Filters process the completed game scene. Window scaling and display color
correction follow the filter pass, so scanline and dot phases belong to the
2x scene image even when the final display scale is fractional. Original
scene dimensions, drawing coordinates and script-created screen dimensions
remain unchanged. Android touch controls remain separate display layers.

Output-filter selection is independent of `DM_RESAMPLE`. Drawmethod resampling
reconstructs each image while drawing it into a scene; the output filter
processes that completed scene. In particular, this output Bilinear filter has
the established forward-neighbor phase, explicit border policy and integer
half sums described below. It is distinct from the center-aligned bilinear
formula selected by `DC_RESAMPLE_BILINEAR` or `DC_RESAMPLE_MOVIE`.

## Color and arithmetic contract

Source RGB channels are sampled at exact texel centers and recovered as bytes
from 0 through 255. All equations below operate independently on each of those
three channels. GPU storage is RGBA8 and filtered output is opaque; source
alpha does not affect the filter.

There is no packing into 16-bit color, reduction to five/six-bit channels, CPU
color conversion, or display-dependent channel mask. Low RGB bits therefore
remain significant, including equality tests used by Scale2x. This intentionally
removes the color loss of the old reduced-color filter path.

Separate truncation steps in the established formulas remain explicit. Define
the half-sum operation as:

```text
half(a, b) = floor(a / 2) + floor(b / 2)
```

For example, `half(3, 5)` is `3`. Combining the operands before division would
produce `4` and would change the filter. Nested applications below preserve
their own intermediate truncation as well.

Let `E` be the current source pixel, `R` its right neighbor, `D` the next row's
pixel in the same column, and `RD` the next row's right neighbor. Each source
pixel produces four output positions, named TL, TR, BL and BR.

### Bilinear

| Output position | RGB byte equation |
| --- | --- |
| TL | `E` |
| TR | `half(E, R)` |
| BL | `half(E, D)` |
| BR | `half(half(E, R), half(D, RD))` |

Samples beyond the right or bottom edge are black. This includes the diagonal
tap at the lower-right corner. There is no clamping of those samples to the
last source pixel.

### Advance Mame2x / Scale2x

Use `U` for the source pixel immediately above `E`, with `L`, `R` and `D` for
the left, right and lower neighbors. If `U == D` or `L == R`, all four output
pixels equal `E`. Otherwise:

| Output position | RGB selection |
| --- | --- |
| TL | `L` when `L == U`, otherwise `E` |
| TR | `R` when `R == U`, otherwise `E` |
| BL | `L` when `L == D`, otherwise `E` |
| BR | `R` when `R == D`, otherwise `E` |

Equality compares all three full RGB8 channels. Above the first row, `U`
repeats the first row. Below the last row, `D` repeats the last row. Missing
left/right neighbors are black. These rules also define one-pixel-wide and
one-pixel-high sources.

The old implementation computed its interior lower-row pointer from the
previous row and consequently reused the current row. The GPU implementation
corrects that pointer error: `D` is the actual next source row. This can change
edge decisions that depended on the erroneous neighbor. The regression suite
distinguishes those decisions, including colors differing only in their low
RGB bits.

### Scanlines

TL and TR equal `E`. BL and BR are black. The first output row is the bright
row; final window scaling does not restart this phase.

### Scanlines TV

TL equals `E`, and TR equals `half(E, R)`. Missing right neighbors are black.
Each lower pixel is derived from its corresponding upper pixel:

```text
BL = 3 * floor(TL / 4)
BR = 3 * floor(TR / 4)
```

Multiplication occurs after truncation. This preserves the established
quarter-step darkening rather than substituting `floor(3 * color / 4)`.

### TV 2x

TL and TR equal `E`. Both lower pixels equal:

```text
floor(7 * E / 8)
```

### Dot Matrix

Start with `E` in all four output pixels. The following pattern repeats in
output coordinates modulo four, beginning at output row zero and column zero.
Each table entry identifies the channels to darken:

| Output Y modulo 4 | X = 0 | X = 1 | X = 2 | X = 3 |
| --- | --- | --- | --- | --- |
| 0 | G | R | B | none |
| 1 | R, G, B | none | R, G, B | none |
| 2 | B | none | G | R |
| 3 | R, G, B | none | R, G, B | none |

For each selected channel `c`, store `c - floor(c / 4)`. Unselected channels
keep their original byte. Pattern selection uses scalar conditions, without
uniform-array indexing or a CPU-generated pattern image.

## GPU ownership and platform boundaries

Desktop `gpu_gl_get_filtered_screen_texture()` borrows the completed native
scene texture. Active filters render one quad into one private 2x RGBA8 texture
and framebuffer. Success returns the borrowed filtered texture and its true
dimensions for normal window presentation. Source screenshots or explicit
reads still access the original scene; the filter does not mutate that screen.

The target is reused across frames and filter changes with the same source
dimensions. Every presentation recomputes its pixels, so GPU draws and explicit
CPU edits cannot leave a stale cached filter result. Changed dimensions build
a replacement target before retiring the previous allocation. Stored output
requires `16 * width * height` bytes, excluding driver bookkeeping, and has no
CPU shadow. No filter-history or delta buffer is needed.

Context shutdown discards this intermediate directly. Ordinary live scene
screens retain their existing preservation rules, including an explicit
readback when needed before losing their graphics context. Filter output adds
no readback to that process. Context recreation compiles the filter shader and
allocates its target again on first use.

Android gameplay still uses its existing CPU scene composition. Its presentation path
uploads that complete RGB32 frame once, runs the same equations through a
dedicated GLES2 pass, and gives the filtered texture back to SDL for final
display. Native SDL 32-bit RGBX textures match the engine's existing RGB byte
order, so this upload needs no CPU color-conversion surface. Their GL storage
retains eight bits per channel, and the filter writes opaque RGBA output.
This step removes CPU output filtering and reduced-color conversion;
porting Android scene drawing to a native GLES backend remains separate work.

The Android pack launcher supplies its resident GPU composition target directly
to the same output-filter pass, so launcher presentation has no CPU scene
upload. Filtered launcher frames retain the normal brightness, aspect and touch
overlay order. See [launcher composition](gpu-renderer-migration.md#launcher-text-and-composition)
for its scoped target, failure handling and reset recovery.

Desktop GLSL 1.20 and GLES GLSL ES 1.00 sources come from one body in
[`gpu_shader.c`](../engine/source/gamelib/gpu_shader.c). GLES uses high precision
for floating-point arithmetic, integers and texture samples. The shared shader
uses no derivatives or uniform arrays. Both backends retain logical row zero
at texture coordinate `v = 0`, and source positions are expressed in original
pixel-edge coordinates. Backend state restoration protects subsequent scene,
movie and display-layer rendering.

## Movies and presentation order

Movies composed into an RGB32 screen are part of that finished scene and
receive its selected output filter during normal presentation. Movie plane
resampling and YUV conversion happen first; the final scene filter then
processes the stored RGB result along with the rest of the scene.

Direct fullscreen movie presentation remains independent. It samples retained
YUV planes and presents directly to the window, preserving its hardware color
conversion and correction order. It does not pass through the game-resolution
scene or the scene's output filter. Switching between those movie presentations
and filtered scene presentations does not disable the desktop native backend.
See [GPU movie playback](gpu-movie-playback.md) for movie controls and platform
availability.

## Rejection and diagnostics

Desktop compiles the dedicated filter program on first active use. Missing
shader support, invalid input, excessive doubled dimensions or failed resource
creation reject filtered presentation and log the problem. Output dimensions
are checked before multiplication and allocation. Failure leaves the caller's
returned texture/dimensions unchanged and leaves scene pixels coherent.
There is no CPU filter implementation or rendering retry.

Android requires SDL's GLES2 renderer with render-target textures, a native
`SDL_PIXELFORMAT_BGR888` RGBX format and high-precision shader support. It
checks doubled dimensions against the texture limit before target allocation,
then creates the filter program and target during video-mode setup. Setup
rejects unavailable resources. Failed runtime filtering logs the rejection
and skips the frame. The adapter restores SDL's renderer state on success and
failure, and rejects source textures that do not match its dimensions, native
RGB32 storage contract or normalized unit texture-coordinate scale.

The shader has a separate program lifetime from scene drawing. Filter shader
failure cannot retire a working desktop scene backend. Replacing a target is
transactional, so an allocation failure cannot publish an incomplete target.
Rejected filter submissions affect only the private intermediate and skip
that presentation instead of displaying partial output.

Startup reports identify GPU output filtering, available filters, full RGB8
precision, private target storage and the absence of CPU processing/readback.
The desktop shader's actual successful first-use compilation is logged as
well. Diagnostic statistics expose completed filter draws, successful target
allocations and the number of live targets, which is zero or one.

## Validation

Run the focused desktop suite with SDL2 development files installed:

```sh
OPENBOR_REQUIRE_GPU_TESTS=1 bash tests/run_gpu_output_filter_tests.sh
```

The suite uses a real offscreen OpenGL context and an independent scalar RGB8
oracle. It verifies all six filters and bypass, every channel byte value,
one-pixel dimensions, odd dimensions, every output phase, explicit borders and
81 Scale2x neighbor combinations. Additional checks cover source mutations,
explicit CPU edits, state recovery, target reuse, rejected size/overflow inputs,
injected framebuffer failure and actual graphics-context replacement.

The initial implementation passes 293 complete-image comparisons totaling
65,079 pixels with exact RGB agreement. Readback belongs only to explicit test
verification, with separate assertions that filtering itself does not read the
scene back.

The full `bash tests/run_gpu_tests.sh` suite also exercises production window
presentation, orientation, filter activity transitions, native drawing after
filter changes and movie coexistence.

The Android adapter has a separate host GLES2 suite:

```sh
OPENBOR_REQUIRE_GPU_TESTS=1 bash tests/run_android_output_filter_tests.sh
```

Its real OpenGL ES 2.0 Mesa run passes 73 complete-image comparisons totaling
75,288 pixels with exact RGB8 agreement. Coverage includes all six formulas,
ordinary SDL target composition, row orientation, preserved renderer state,
and injected missing-function, shader and draw failures with recovery. It
asserts that filtering adds neither readbacks nor pixel uploads beyond the
existing source upload.

Offscreen Mesa execution checks shader results and resource handling;
performance and supported-device driver behavior still require physical GPU
validation. These host results do not substitute for platform builds or Android
device testing.
