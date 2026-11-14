# Native GPU renderer migration

Author: Damon V. Caskey

Date: 2026-11-14

## Scope

This branch introduces native scene rendering into the existing desktop
OpenGL backend. The software renderer remains available for unsupported
operations. The branch also adds opt-in, general-purpose affine transforms
for sprites and screens, plus drawmethod chroma keys and final-color Substitute.

Existing OpenGL selection automatically enables native drawing when the
required shaders and render targets initialize successfully. Existing drawing
calls select it automatically. `OPENBOR_GPU=0` disables native scene drawing for comparison with
the existing software renderer and OpenGL presenter.

The first native target format is `PIXEL_32`. Indexed sprite artwork remains
indexed in GPU textures and uses a separate palette texture. This is distinct
from rendering into an indexed framebuffer, which remains on the software path.

| Operation | Migration behavior |
| --- | --- |
| RGB32 screens and scene composition | GPU textures and framebuffer objects |
| Indexed sprites, source palettes and remaps | Cached index textures, bounded resident palette atlas, and draw-time palette lookup |
| Alpha-mask sprites | Separate indexed opacity texture, existing precedence and opacity equation |
| Existing scale, flip, rotation, shear, water and plane transforms | GPU pixel sampling with existing transform parameters |
| Opt-in 2x3 affine transforms | One bounded quad, shader pixel-center sampling, existing material operations and software fallback |
| Six blend modes, tint, independent channel opacity | Shader composition; existing channel lookup tables are uploaded when used |
| Five chroma keys and five Substitute pairs per drawmethod | Native shader rules for RGB32 destinations; unavailable rules are ignored |
| Boxes, dots and lines | Native rendering; lines retain integer point coverage |
| Queue sorting, layers, offsets, lock/newonly and replay | Existing queue implementation, unchanged |
| Capture into screens and reuse of captured screens | GPU-to-GPU rendering without CPU readback |
| Screen clear/copy | Native operations for supported targets |
| Script pixel reads and screenshots | Lazy synchronization of the relevant CPU shadow |
| Direct CPU image edits and unsupported raster operations | Synchronized software operation followed by upload when next used by GPU |
| 8-bit and 16-bit destination rendering | Existing software implementation |
| Active CPU output filters | Existing software renderer/presentation path |
| Android | Existing renderer retained; no native GLES migration claimed |
| Video playback | Existing movie path retained, with synchronized transitions |

The affine contract from the separate transform experiment is available through
`changedrawmethod()`; see [the affine guide](affine-transforms.md). New projective
and curve mappings, public programmable shaders and 24-bit asset loading remain
separate capabilities. See [the drawmethod color guide](drawmethod-colors.md)
for opt-in chroma coverage and final-output substitutions. Color rules execute
only on supported native RGB32 draws. Software draws and unsupported native
color capabilities ignore those settings while retaining ordinary drawing.
No frame limiter, 4 ms wait, game tick, or input scheduling change is included.

`experiment/gpu-palette-cache` builds on profiling commit `722df2d` with resident
palette reuse and subsequent context-local state caching. Palette-only commit
`f2c96c3` remains the comparison point before redundant-state elimination.
Both changes retain draw order and existing render operations; neither adds
cross-entry draw batching.

## Startup capability report

Video initialization writes a prominent `[GPU]` section to the ordinary
`Logs/OpenBorLog.txt`, before gameplay and before any scene draw is required.
The report also runs when a video mode recreates the graphics context. These
records are independent of profiling and remain visible with
`OPENBOR_PROFILE=0`; no per-frame logging is added.

The graphics section uses headings, tab-indented rows and explicit units. The
first video initialization labels its menu settings **Starting Values**. Later
initializations use **Values at Video Reinitialization**, including when a menu
change recreates the video mode. These are snapshots, and menu options can
change them during play. Shutdown does not print a zero-resolution settings
snapshot.

For example, the starting settings from a windowed RGB32 game might read:

```text
[GPU] Starting Values
    Menu options can change these values during play.
    Video backend requested:    OpenGL
    FPS limit:                  200 FPS
    Display mode:               Windowed
    Display scale:              3.00x
    Software filter selected:   Disabled
    CPU output filter:          Inactive
    Scene resolution:           480 x 272
    Scene color format:         RGB32 (4 bytes per pixel)
```

FPS settings use their menu meanings: Disabled, VSync, 200 FPS or 500 FPS.
Fullscreen scaling is identified as automatic, with the stored window scale
shown separately in the same row. Filter selection and filter activity have
separate rows because a selected filter can be inactive at the current scale.
The **Active Presentation** section reports the backend and VSync state that
were actually selected, plus display refresh rate and output color depth.
It follows the requested settings and capability checks, keeping them distinct.
Unavailable display refresh information is labeled `Unavailable`, rather than
presented as a zero-Hz display.

New native rendering features should extend this checklist with their required
capabilities, initialization result, enabled execution path and any CPU
fallback consequence. Successful activation should be explicit alongside
warnings for missing capabilities.

The native OpenGL capability check reports:

- Driver vendor, renderer name, OpenGL version and GLSL version.
- Every missing required shader or framebuffer entry point, including the
  framebuffer extension alternatives.
- Maximum texture size, fragment texture units and fragment-uniform capacity,
  with the native requirements beside the reported limits.
- Actual shader compilation and linking, including driver diagnostics on
  failure and retries of supported affine/color-rule combinations.
- Shared texture allocation and a real 1x1 RGBA8 framebuffer completeness
  probe, using the existing white texture without drawing into it.

Each native capability report ends with one of these results:

| Result | Meaning |
| --- | --- |
| `NATIVE` | Native scene rendering and affine transforms are available within the backend's supported formats and draw methods. |
| `LIMITED NATIVE` | The material renderer is available, while affine transforms use CPU software rendering. |
| `SOFTWARE` | Native scene rendering is unavailable or explicitly disabled; scene draws use CPU software rendering. |

The separate **Color Rules** checklist reports chroma and Substitute execution,
five slots for each, RGB32 destination scope, and matching order. The general
result above continues to describe scene/affine availability; consult the
color rows to establish native color-rule support. If an optional combination
fails, initialization prefers preserving native affine support before trying
color-only and legacy variants. Missing color support is reported as
`UNAVAILABLE - ignored`, with an explicit notice that no CPU implementation
exists. Color settings never force an otherwise native draw onto the CPU.
Software drawing ignores both controls without a per-draw warning.

Missing capabilities that require software fallback produce `WARNING` records
with the cause, the work that moves to the CPU, and the statement
`Significant slowdown is possible.` For example, insufficient affine capacity
produces these records during initialization:

```text
[GPU] WARNING: Insufficient fragment uniform components for native affine transforms.
    Available:                         64
    Required:                          256
    Affine draws will use CPU software rendering.
[GPU] RESULT: LIMITED NATIVE - native scene rendering available; affine transforms use CPU software rendering.
[GPU] WARNING: Affine transforms will use CPU software rendering. Significant slowdown is possible.
```

Intentional settings are identified as configuration choices. Selecting SDL
presentation, enabling a CPU output filter, or setting `OPENBOR_GPU=0` is not
reported as a GPU capability defect. If a setting prevents native
initialization, the log explicitly says the native capabilities were not
tested. Accelerated SDL presentation only displays the completed image and
does not establish native scene rendering. Recognized software OpenGL driver
names also trigger a warning: a usable shader backend on such a driver still
executes on the CPU.

Successful initialization cannot predict every future image or operation.
The startup report lists potential CPU fallback for 8/16-bit destinations,
unsupported draw methods, numerically extreme transforms, images exceeding
the reported texture-size limit, and resource allocation failures. Existing
coherent fallback remains available; its potential performance cost is visible
at startup rather than waiting for an affected draw.

## Where the boundary moved

`gpu.c`/`gpu.h` provide window-system-independent dispatch and synchronization.
The public gamelib drawing entry points attempt native drawing first. A native
backend returns unsupported before changing the destination, allowing the
existing implementation to run without applying the effect twice.

`engine/sdl/gpu_renderer.c` owns the desktop resources, shaders, native draws,
and GPU/CPU transfer boundaries. `gpu_transform.c` prepares transform geometry;
it does not run the source-pixel/color blending loop. Water and plane mapping
use row geometry so their existing settings keep their established meanings.

`video_copy_screen()` tries native texture presentation before requesting the
CPU frame through `getVideoSurface()`. Merely uploading a completed CPU frame
would not constitute native scene rendering; the new path avoids that step.

## Sprite residency

Sprites retain their compact RLE-encoded CPU source. On the first supported
native draw, the renderer decodes that source into a temporary index buffer,
uploads an indexed GPU texture, and frees the temporary buffer. Later draws
reuse the texture across frames; they do not decode or upload the image again.
Separate resident palettes preserve cheap color swaps, and opacity masks use
their own cached index textures.

The GPU copy lasts until sprite release, re-encoding, or native context
shutdown. The CPU source supports existing software fallback and rebuilding
resources after context changes. Eager upload would move first-use work into
loading, not eliminate a transfer currently repeated on every draw. Discarding
the CPU source would require a separate lifetime/restoration design and is not
part of state caching.

## Screen ownership and synchronization

Screens retain their CPU allocation as a migration shadow. The copy in system
memory is not refreshed after every GPU draw.

- CPU-authored image: upload before its next GPU use.
- GPU-authored image: keep it resident while other draws sample it.
- CPU reader: read back once if the GPU holds a newer image version.
- Partial CPU writer: first synchronize, then mark the CPU image dirty.
- Complete replacement: it may mark the CPU image dirty without preserving
  pixels that will all be overwritten.
- Free/re-encode: evict GPU ownership before the object's address can be reused.
- Controlled context recreation: synchronize live screens before destroying
  the context, then rebuild native resources lazily.

New native code that accesses `screen->data` must use `gpu_sync_cpu()` before
reads or `gpu_prepare_cpu_write()` before read-modify-write operations. The
existing native-code paths have been audited and instrumented, including
scripts, menu pixels, GIFs, movies, screenshots and low-level rasterizers.

Readback failure is explicit and fatal rather than silently returning stale
pixels. Allocation/capability failure before drawing can select software.
Unexpected failure after partially submitting an effect is not retried on top
of its partial result.

This retains synchronous script pixel access, but does not promise that
reading GPU-produced pixels is as inexpensive as reading an original CPU asset.
Screenshots remain CPU-encoded after synchronization at their existing capture
point. Normal capture/reuse/presentation does not synchronize the CPU image.

## Ordering, capture and feedback

The queue remains a retained, sorted list. Captures do not consume it or
automatically clear their targets. `newonly` retains its existing locked-prefix
meaning, and minimum/maximum Z limits remain inclusive.

Image contents are used when the queue is replayed, not frozen when an entry
is added. A screen modified between two replays affects the later replay.
GPU commands preserve this order without requiring a CPU wait between targets.

Self-drawing takes a GPU snapshot of the source immediately before that draw.
It never samples the texture currently attached for writing. This gives defined
overlapping-copy behavior rather than preserving traversal-dependent smearing.

Destination-dependent blending also uses a GPU snapshot. Only the affected
rectangle is copied for ordinary blending; self-drawing snapshots the full
source because transformed sampling can access pixels outside that rectangle.
Snapshots are reused, not allocated for each draw. Queue entries are not
reordered by texture or material.

## Resident source palettes

The renderer stores up to 1024 complete 256-color palettes in one
256-by-1024 RGBA8 atlas, selecting the resident row for each indexed draw. The
atlas has a fixed logical payload of 1,048,576 bytes (1 MiB). CPU copies of its
palette contents require another 1 MiB plus bounded lookup and recency metadata;
the profiling configuration record reports that structure's complete
`cpu_bytes` size for the current build (1,073,168 bytes with 32-bit `int` and the
current layout). These figures exclude driver overhead.

Consecutive reuse compares the complete contents of the previously selected
row. Other lookups hash the complete palette and verify matching candidates
with a full comparison. Identical values at different source addresses reuse
one resident row, hash collisions cannot silently select another palette, and
in-place edits are recognized on the next lookup. The cache owns copies of
contents, not caller pointers, so a palette's address may be freed or reused
without leaving a retained reference.

Unused rows fill first. Once all rows are occupied, deterministic
least-recently-used replacement selects a victim with constant-time list
maintenance. Uploading a replacement happens in the same ordered GL command
stream as drawing. Previously issued draws therefore retain their earlier
palette use; later draws select the replacement without reordering the queue.
Eviction trades a future upload for bounded memory rather than retaining an
unlimited set of transient palettes.

Residency survives frame boundaries, warmup, report windows, and menus while
the native context remains alive. Renderer shutdown or context recreation
discards residency and reconstructs an empty atlas on successful initialization.
Palette uploads are independent of source-image uploads, and disabling the
profiler with `OPENBOR_PROFILE=0` does not disable this renderer cache.
`OPENBOR_GPU=0` disables the entire native renderer; no cache-only runtime
switch is added. An upload error invalidates residency before fallback so a
later lookup cannot trust uncertain GPU row contents.

`palette_lookup_ms`, `palette_cache_hits`, `palette_cache_misses`, and
`palette_cache_evictions` expose lookup work and reuse. The lookup span ends
before upload; successful-miss content copies and insertion/eviction bookkeeping
remain in the calling frame phase, outside that span. Existing palette upload
counters continue to record actual successful updates. See
[the engine profiling guide](engine-profiling.md#comparing-the-resident-palette-change)
for the baseline comparison procedure. Performance depends on reuse and the
remaining engine workload; no speedup is assumed from the allocation alone.

## Context-local graphics-state reuse

The native renderer caches framebuffer binding, viewport, scissor, fixed raster
state, shader program, five texture-unit bindings, and material uniforms.
Cached setters compare the required value with the known current value and
emit GL calls only for changes or unknown state. Texture filtering and clamp
parameters are initialized with their texture objects instead of being
reapplied on every bind. Presentation's temporary filtering retains its own
restoration contract.

The cache describes one current context, not a set of assumptions that may
survive arbitrary external GL calls. Presentation and external helper ownership
boundaries explicitly invalidate tracked state. The next native operation
restores its required values; no hot-path `glGet` checks are needed to discover
what external code changed. Internal clear, readback, upload, and snapshot
paths participate in cache maintenance. Texture/framebuffer deletion, failed
resource recovery, initialization, and context replacement update or invalidate
the relevant remembered state before numeric GL names can be reused.

This changes setup frequency, not the sequence or meaning of rendering:

- Queue entries retain their original order and are not batched together.
- Destination snapshots and self-draw handling remain unchanged.
- Palette contents and residency keep their existing validation rules.
- No new CPU/GPU synchronization wait or flush is introduced.
- Unsupported operations retain the coherent software fallback path.

`gpu_state_changes` and `gpu_state_reuses` report emitted and avoided cached
setter calls as aggregate deltas for successful native draw/clear operations.
Quad-submission deltas include palette/table/snapshot preparation inside
`gpu_render_quads()` and wrap handling outside the existing setup-only
`gpu_state_ms` span. Caller-side source/target image creation and uploads before
that boundary are excluded; clear deltas include target preparation. Independent
resource/presentation helpers are also excluded from frame counters, while
internal 64-bit lifetime statistics still count all cached setters. These are
call counts, not hardware draw counts,
GPU utilization, or an assumed amount of time saved. See
[state accounting and comparison guidance](engine-profiling.md#cached-graphics-state-accounting)
before interpreting them.

## Color and intended-behavior corrections

RGB32 screen alpha bytes are unused in the engine's color contract. Native
targets therefore do not treat zeroed storage as automatically transparent.
Black is opaque unless the draw explicitly requests a color key. CPU readback
normalizes the unused high byte to zero.

Existing blend equations, including their `/256` opacity arithmetic, are
retained in this migration. Custom channel lookup tables are supported; table
replacement invalidates the GPU copy. Palette edits are detected by value,
not just pointer address.

The native renderer intentionally does not reproduce these incidental defects:

- A nonzero unused high byte allowing RGB black to escape a requested zero key.
- Primitive routines ignoring an explicitly configured clipping rectangle.
- Untransformed screen paths ignoring flip/fill settings that transformed paths honor.
- Rotation bounding-box clipping inconsistent with its nonuniform scaling.
- Rotation sampling drift caused by truncated inverse coefficients at exact
  texel boundaries.
- Per-pixel floating accumulation drift at exact plane texel boundaries.

Scale/flip/shear retain the existing fixed-point sample phase. Plane and rotation
sampling use stable mappings; values within 0.0001 texel of an integer boundary
snap to that boundary. Differences from repeated CPU floating addition or old
rotation coefficient truncation are checked against independent canonical
samplers, not accepted with a general neighboring-pixel tolerance.

Affine has its own sampling policy. It inverse-maps destination pixel centers
and selects source texels without the legacy rotation/plane snapping band.
The CPU supplies homogeneous numerators and a positive determinant, encoded as
whole parts and seven signed fractional bytes. The shader evaluates an initial
quotient from fragment coordinates and corrects the selected texel with exact
integer-valued byte-carry remainder comparisons. This
preserves exact rational boundaries, including 3x scale, and real fractional
translations immediately beside a half pixel. It avoids both interpolated UV
drift and pre-rounded reciprocal steps. These comparisons also avoid relying
on compensated float expressions that shader compilers may reassociate.
GLSL 1.20 remains the required language baseline.

Preflight bounds each scaled numerator contribution, including its full target
extent, to 2^18. The largest absolute pixel offset from draw position on each
target axis, plus the larger source extent including its absolute pivot, plus
two, must total at most 65535. Draw position and pivot remain separate from the
source numerator origin so exact pivot samples do not suffer cancellation.
Values requiring more than 56 fractional
bits after homogeneous scaling also select coherent software fallback instead
of being quantized. These bounds keep all byte products and carries exact in
ordinary 32-bit shader floats. The CPU's per-draw setup still uses double
precision; this does not claim arbitrary-precision matrix arithmetic.

Native affine requires at least 256 fragment-uniform components, allowing
conservative array padding. Contexts with less capacity retain the earlier
native renderer and use software for affine draws. If an older compiler rejects
the optional affine shader, initialization retries the earlier material shader.
The ordinary startup capability report warns about the reason and the CPU
performance consequence before any affine draw. Its labeled limit and result
rows report the fragment-uniform capacity and native affine availability.
Repeated affine draws
validate every instance first and retain their original blending order.

## Validation

Run `bash tests/run_gpu_tests.sh` with SDL2 development files installed.
The headless native tests use `SDL_VIDEODRIVER=offscreen` by default.

The suites cover:

- Existing stable queue sorting.
- All byte pairs for the six blend equations.
- Transform parameter validation and emitted geometry.
- Startup capability reports with profiling disabled, limited native and full
  software fallback warnings, real shader compile/link failures, framebuffer
  rejection, clean GL error handling, and no repeated draw-time log output.
- Production GPU logging routed to the ordinary engine log independently of
  profiling, including bounded and literal driver diagnostics.
- Production rasterizers with a separate-image mock backend to check ownership,
  fallback, indexed operations, replay semantics and resource eviction.
- Actual OpenGL capture/reuse, no-readback assertions, dirty uploads, palettes,
  masks, blend tables, overlapping self-draws and context recreation.
- Resident palette reuse, in-place edits, unaligned input, equal contents at
  different addresses, full-hash collisions, LRU overflow, upload-error recovery,
  capture ordering, and fresh residency after actual context recreation.
- Cached-state reuse with pixel verification across material, texture, target,
  clipping, presentation, resource replacement, and context transitions.
- Chroma/Substitute native complete-image checks, all five slots, palette/fill
  ordering, blend-created matches, zero-opacity exclusions, primitive source
  colors, queue snapshots, repeats, affine draws, and overlapping self-draws;
  ignored-rule checks preserve ordinary software output and native availability.
- Native transform output compared with production CPU rendering.
- Affine geometry and pixels checked against independent mappings, including
  adjacent fractional translations, clipped pivots/flips, material effects,
  repeated blend order, queue copies, capture/replay and invalid-input atomicity.
- Real presentation, orientation, output-filter/video transitions and mode changes.
- Deliberately colliding texture-cache identities during shutdown.

The development environment uses Mesa llvmpipe for real OpenGL execution.
This validates shader execution and resource behavior, not hardware performance.
Actual GPU timings, DC3D gameplay, representative conventional games, and
platform build results remain release gates before replacing the software
backend outright.

## Follow-on migration work

This branch retains software as an explicit fallback/reference path. It does
not claim full retirement of that code. Remaining work includes native indexed
and 16-bit destinations, Android/GLES integration, additional batching, and
GPU equivalents for selected CPU output filters. These are separate from new
rendering features: retaining the existing drawing capabilities is the goal.

Future programmable materials should apply after source color decoding so
indexed palettes and direct RGB assets can share geometry and material effects.
True-color recoloring also needs a defined authoring contract: color transforms,
lookup textures and optional region masks can express different kinds of color
changes. RGB values alone do not retain the palette-index identities that
indexed artwork currently supplies.
