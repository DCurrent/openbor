# Native GPU renderer migration

Author: Damon V. Caskey

Date: 2026-11-12

## Scope

This branch introduces native scene rendering into the existing desktop
OpenGL backend. It is a migration implementation, not the removal of every
software renderer and not a new set of game or script features.

Existing OpenGL selection automatically enables native drawing when the
required shaders and render targets initialize successfully. There is no new
script API. `OPENBOR_GPU=0` disables native scene drawing for comparison with
the existing software renderer and OpenGL presenter.

The first native target format is `PIXEL_32`. Indexed sprite artwork remains
indexed in GPU textures and uses a separate palette texture. This is distinct
from rendering into an indexed framebuffer, which remains on the software path.

| Operation | Migration behavior |
| --- | --- |
| RGB32 screens and scene composition | GPU textures and framebuffer objects |
| Indexed sprites, source palettes and remaps | Cached index textures and draw-time palette lookup |
| Alpha-mask sprites | Separate indexed opacity texture, existing precedence and opacity equation |
| Existing scale, flip, rotation, shear, water and plane transforms | GPU pixel sampling with existing transform parameters |
| Six blend modes, tint, independent channel opacity | Shader composition; existing channel lookup tables are uploaded when used |
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

No affine/curve API from the separate transform experiment is added here.
No frame limiter, 4 ms wait, game tick, or input scheduling change is included.

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

## Validation

Run `bash tests/run_gpu_tests.sh` with SDL2 development files installed.
The headless native tests use `SDL_VIDEODRIVER=offscreen` by default.

The suites cover:

- Existing stable queue sorting.
- All byte pairs for the six blend equations.
- Transform parameter validation and emitted geometry.
- Production rasterizers with a separate-image mock backend to check ownership,
  fallback, indexed operations, replay semantics and resource eviction.
- Actual OpenGL capture/reuse, no-readback assertions, dirty uploads, palettes,
  masks, blend tables, overlapping self-draws and context recreation.
- Native transform output compared with production CPU rendering.
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
