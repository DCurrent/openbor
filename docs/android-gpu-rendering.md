# Android native GPU rendering

Author: Damon V. Caskey

Date: 2026-11-20

## Supported rendering path

Android uses the shared native renderer for RGB32 scenes, scripted subscreens,
sprites, primitives, launcher text, output filters and movies. Applications
remain 64-bit ARM64. The graphics baseline is OpenGL ES 3.0, matching the
existing Android manifest requirement. GLES2-only devices are outside this
path; failed initialization logs the missing capability and rejects the video
mode without selecting software rendering.

`engine/sdl/gpu_renderer.c` owns scene resources and executes the same material
and transform logic as desktop. `engine/android/app/jni/openbor/video.c` owns
the SDL window, GLES context, final presentation and touch assets. SDL supplies
windowing, events and context management; its separate 2D renderer is no longer
used for Android scene, launcher, filter or movie presentation.

The port retains RGB32 screen identities and their existing script interfaces.
Indexed images remain asset storage, with separate resident index and palette
textures. They do not introduce another scene color mode.

## Shared functionality

| Area | Android behavior |
| --- | --- |
| Screen drawing, clear and copy | GPU-resident RGB32 targets |
| Sprites and indexed sources | Shared source cache, palette atlas and opacity masks |
| Scale, flip, rotation and affine | Shared geometry and shader sampling rules |
| Water, shear and plane effects | Shared shader paths, bounded approximations and cached plane recurrence |
| Blend, tint and channel opacity | Shared byte arithmetic and custom lookup tables |
| Chroma and Substitute | Existing native color rules and capability reporting |
| Drawmethod resampling | Nearest, bilinear/movie, named cubics and Lanczos 2/3 |
| Capture, feedback and self-drawing | Ordered GPU copies and destination snapshots |
| Launcher | Shared native glyph atlas and general screen storage |
| Output filters | Shared RGB8 kernels and reusable GPU-only 2x target |
| Movies | Shared YUV conversion/resampling for screens and fullscreen output |
| Pixel reads and screenshots | Explicit synchronization of the requested screen |

The same input limits and optional-capability diagnostics apply as on desktop.
Unavailable operations are logged and skipped, with no CPU rendering retry.
Existing CPU preparation for exceptional transform geometry does not rasterize
scene pixels. File decoding, GIF frame preparation and asset processing remain
separate CPU work.

## GLES resource and shader adaptation

The GLES build uses GLSL ES 3.00 with explicit high precision. Shader bodies
share their sampling, color and blend equations with desktop GLSL. GLES uses
core R8/RED storage for single-channel source indices, masks and movie planes,
RGBA8 scene targets, and sampled RGBA32F plane-row maps. Texture limits and
actual shader/resource creation are checked before reporting native support.

The renderer uses streaming vertex buffers, cached triangle indices and
explicit vertex attributes. Desktop compatibility state such as alpha-test
enables is not issued to the GLES context. CPU-to-GPU asset transfers and
explicit GPU readbacks use GLES-supported byte formats.

CPU profiling and renderer counters remain available on Android. Optional GPU
timestamp measurements remain desktop-only; the GLES path reports that timing
capability as unavailable without affecting native drawing.

## Presentation and movies

Gameplay presentation consumes a resident native texture, optionally runs the
selected output filter, and draws the result at the configured display size.
It does not read back or upload a complete CPU-composited frame. Source uploads
still occur when an asset is first used or explicitly edited on the CPU.

Display correction uses the common gamma-then-brightness polynomial and the
existing control range. Android's gamma control now participates in that
correction. Brightness uses the common value divided by 256, replacing the
former SDL overlay's `(abs(value) - 1) / 255` opacity and its slightly different
rounding. Touch controls are composed afterward so scene correction does not
alter their colors. Existing touch positions, authored skins, source clipping,
opacity and hide timing remain available. The native presenter requests VSync
when the existing FPS-limit setting selects it; engine tick scheduling and
the existing frame delays remain unchanged.

Movies retain the shared playback controller, channel behavior, CPU decoding
and audio. Native screen composition and fullscreen output use the same YUV
shader and all supported movie resampling formulas. Fullscreen movie output
remains independent of the final-scene output filter. Explicit movie captures
render into native screens and synchronize only when their CPU consumer needs
the pixels. Normal movie/game switching does not require a graphics-context
replacement.

## Context lifecycle

GPU-produced screens retain CPU shadow allocations, but those shadows are not
updated after each draw. Rendering, capture/reuse and ordinary presentation
therefore do not incur a frame-by-frame readback.

Controlled mode replacement preserves live screens before releasing their
context. Android application lifecycle events are processed on the rendering
thread. Before background suspension, newer native screen contents are copied
into their shadows once; source sprites and other recreatable assets retain
their existing CPU originals.

Reset notifications only record state on the notifying thread. Native
operations check context usability before touching GL objects. Foreground
recovery can reconstruct resident resources from current preserved shadows.
Objects from a lost context are abandoned without deleting their numeric GL
names in a replacement context. The context generation also distinguishes
replacement contexts when their native handles are reused. Private fullscreen
movie contents can be discarded and reconstructed from decoded planes; this
exception does not apply to script-owned render targets.

Unexpected loss before newer GPU contents can be preserved is unrecoverable:
the renderer logs and fails explicitly instead of publishing stale shadow
pixels, silently clearing scripted screens or selecting software rendering.
This is distinct from a normal suspend/resume sequence that preserves screen
contents before SDL releases its current surface.

Explicit CPU readers retain the existing coherence contract. Script pixel
queries and screenshot encoders receive current pixels or an explicit failure;
metadata queries do not force a readback. Freeing a screen or sprite retires its
owner identity before that address can be reused.

## Verification

The shared GLES suite runs existing independent image and ownership oracles
against a genuine ES3 context:

```sh
OPENBOR_REQUIRE_GPU_TESTS=1 bash tests/run_gles_scene_tests.sh
```

Production Android video integration has its own focused suite:

```sh
OPENBOR_REQUIRE_GPU_TESTS=1 bash tests/run_android_scene_tests.sh
```

The full `bash tests/run_gpu_tests.sh` includes both along with desktop
regressions. The Android ARM64 build compiles the actual NDK GLES3 interfaces.
Host Mesa execution verifies shader output, ownership, failure behavior and
lifecycle contracts; physical Android suspend/resume, device drivers, touch
input and performance still require device validation. No device FPS gain is
claimed from these tests.

## Remaining migration work

Animated GIF palette expansion and retained-frame/channel composition now use
the shared native backend; see [GPU animated GIF playback](gpu-animated-gif.md).
Launcher previews resize the original decoded source directly on the GPU.
Production software scene rasterizers have been removed. Sloped-line geometry
and batching across compatible draw calls remain separate follow-ups. Removing the
Android pack launcher is also a separate product decision.
