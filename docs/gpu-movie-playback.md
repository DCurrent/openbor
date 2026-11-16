# GPU movie playback

Author: Damon V. Caskey

Date: 2026-11-15

## Shared playback and rendering

Movie-to-screen composition and fullscreen movie output now share the native
GPU YUV renderer on Windows, Linux, and macOS. The two script functions select
the output target; they no longer select CPU versus GPU color conversion.
OpenBOR continues to use RGB32 scene screens and 64-bit applications.

The existing playback controller still owns 64 movie channels, reusable
streamed/cached sources, asynchronous decoder lifecycle, pause, seeking,
looping, playback speed, interruption, audio channel assignment and audio/video
synchronization. CPU VP8 decoding produces retained YUV420 planes. This change
moves conversion, reconstruction, resizing, exact-black protection and screen
composition to the GPU; it does not add hardware codec decoding.

`movie_draw_to_screen(screen, channel)` writes an opaque, clipped rectangle into
an RGB32 screen. Script call order determines layering. That screen remains a
normal GPU-resident screen and can subsequently use drawmethods, transformations,
blending, chroma keys, and the ordinary screen drawing API.

`movie_draw_to_yuv(channel)` presents directly to the window using the same
uploaded planes and shader. It does not downsample through the game-resolution
screen. It remains a final-output operation: each call presents that channel,
rather than layering multiple channels in the window. Compose channels into a
screen when layering is needed.

Fullscreen output no longer shuts down scene rendering or replaces the normal
RGB presenter. Screen drawing can continue between movie presentations, and
returning to a game screen requires no graphics-context recreation.

## Resampling selection

Each playback object has one `MOVIE_PROPERTY_RESAMPLE` selection. It uses the
same constants and formulas as the master drawmethod resampling switch:

| Constant | Formula |
| --- | --- |
| `DC_RESAMPLE_NEAREST` | Nearest texel |
| `DC_RESAMPLE_BILINEAR` | Bilinear |
| `DC_RESAMPLE_BICUBIC_CATMULL_ROM` | Bicubic, B = 0, C = 1/2 |
| `DC_RESAMPLE_BICUBIC_MITCHELL` | Bicubic, B = 1/3, C = 1/3 |
| `DC_RESAMPLE_BICUBIC_BSPLINE` | Bicubic, B = 1, C = 0 |
| `DC_RESAMPLE_LANCZOS_2` | Two-lobe Lanczos |
| `DC_RESAMPLE_LANCZOS_3` | Three-lobe Lanczos |
| `DC_RESAMPLE_MOVIE` | Alias of bilinear; default for movie playback |

One selection applies to all three planes. Each plane is reconstructed at its
own resolution before YUV-to-RGB conversion. There are no separate settings for
horizontal/vertical scaling and no custom formulas. The selector must be an
integer constant in the defined range; invalid values fail without modifying
the playback object.

Changing the filter, dimensions, offsets or black protection does not require
a new decoded frame. Paused frames can be redrawn with those changes. Ordinary
scene drawmethods still default to nearest. Movie drawing remains independent
of the ambient global drawmethod.

```c
// Start one source and configure its output once.
int source = movie_load("data/movies/intro.webm", MOVIE_LOADING_STREAM);
void playback = movie_play(source, MOVIE_CHANNEL_AUTO);
int channel = movie_get_channel_index(playback);

movie_set_property(playback, MOVIE_PROPERTY_RESAMPLE,
                   DC_RESAMPLE_BICUBIC_MITCHELL);
movie_set_property(playback, MOVIE_PROPERTY_WIDTH, 640);
movie_set_property(playback, MOVIE_PROPERTY_HEIGHT, 360);
movie_set_property(playback, MOVIE_PROPERTY_OFFSET_X, 24);
movie_set_property(playback, MOVIE_PROPERTY_OFFSET_Y, 16);
movie_set_property(playback, MOVIE_PROPERTY_BLACK_FILTER, 1);

// During the drawing callback, draw the retained frame into an existing screen.
// Store the channel/playback handles using the module's normal script state.
movie_draw_to_screen(screen, channel);
```

For direct fullscreen output:

```c
movie_set_property(playback, MOVIE_PROPERTY_RESAMPLE, DC_RESAMPLE_MOVIE);
movie_set_property(playback, MOVIE_PROPERTY_WIDTH, MOVIE_SIZE_NATIVE);
movie_set_property(playback, MOVIE_PROPERTY_HEIGHT, MOVIE_SIZE_NATIVE);
movie_draw_to_yuv(channel);
```

Public position and duration remain milliseconds. Neither draw function advances
the playback clock. The engine update continues to manage transport and frame
promotion.

## Placement, size and black protection

Each dimension resolves independently:

| Value | Screen composition | Fullscreen output |
| --- | --- | --- |
| `0` | Destination screen dimension | WebM display dimension |
| `MOVIE_SIZE_NATIVE` | Encoded frame dimension | WebM display dimension |
| Positive integer | Requested rectangle dimension | Requested logical display dimension |

Screen composition retains signed X/Y offsets, source-phase-preserving clipping,
and opaque replacement including zero-black pixels. Fully clipped draws
succeed without changing screen pixels or requiring plane upload. Fullscreen
output preserves its existing aspect/stretch sizing rule and ignores screen
placement offsets and black protection.

`MOVIE_PROPERTY_BLACK_FILTER` changes only a final quantized RGB `(0,0,0)` into
`(1,1,1)`. It protects black when the resulting screen is subsequently drawn with
the usual zero-color key. It does not make movie-to-screen composition itself
transparent, and it does not change other near-black colors.

## Hardware color reference

The established hardware movie conversion is preserved exactly as a policy.
Normalized samples use offsets `(-0.0625, -0.5, -0.5)` and the existing matrix:

```text
R = 1.164 Y + 1.596 V
G = 1.164 Y - 0.391 U - 0.813 V
B = 1.164 Y + 2.018 U
```

Those offsets and coefficients are intentionally retained rather than replaced
with a different range convention or a more precise standard matrix. With the
default movie filter, the GPU linearly samples Y, U and V independently, converts,
applies fullscreen gamma followed by brightness, then clamps to the output.
Conversion therefore retains out-of-range values until display correction.

Screen composition uses the same plane sampling and conversion but stores
uncorrected RGB32. Normal scene presentation then applies display correction
once. The RGB32 screen boundary necessarily clamps/quantizes its stored colors;
under nonzero display correction, this can differ from direct fullscreen output
for out-of-gamut YUV values. Direct fullscreen playback preserves its existing
correction order without an intermediate RGB32 image.

Screen playback intentionally adopts the hardware conversion and movie-default
bilinear filter, replacing the former CPU conversion and nearest resize. Existing
scripts need no signature changes to receive GPU rendering, but authors can
select nearest when they want sharp, uninterpolated samples.

RGB/indexed drawmethods with `DC_RESAMPLE_MOVIE` still filter their existing RGB
colors. They do not invoke movie conversion or recreate the per-plane sampling
of a YUV source.

## Ownership, lifetime and capture

Each playback object supplies an immutable decoded-frame generation. The GPU
caches three plane textures by owner and generation, uploading each newly
promoted frame once per graphics context. Repeated output to different screens,
fullscreen output, paused redraws and property edits reuse those planes.

Channel retirement releases its textures before recycling the owner. Context
shutdown releases every movie cache; the retained decoded frame can be uploaded
again after context recreation. Upload generations are independent of the
presentation-dirty flag, avoiding stale frames when screen and fullscreen output
alternate.

Movie composition leaves the destination GPU-authoritative. Normal decoding,
conversion, scaling and presentation do not read pixels back. Explicit screenshot
capture renders the retained planes with hardware movie color conversion and
display correction, then reads the capture screen when encoding the file. Legacy
`playwebm()` screenshots retain encoded-frame dimensions, so they exclude window
letterboxing and do not claim to be a pixel-for-pixel window capture.

The CPU YUV converter, lookup tables, scaled-frame buffers, black-pixel scan and
screen-copy loops have been removed. Their unused 16-bit conversion branches
are removed with them. YUV frame allocation and CPU codec decoding remain.

## Availability and failure behavior

Desktop native OpenGL movie rendering supports every listed filter. Missing
backends, invalid input and rejected GPU operations report failure. There is no
CPU conversion or resampling retry. Decoder and audio errors retain their
existing controller handling.

Android retains its explicitly selected accelerated SDL fullscreen presenter,
with dedicated movie storage separate from the game texture. It supports nearest
and bilinear movie sampling; wider kernels are rejected and logged. Its explicit
screenshot path uses SDL GPU conversion followed by capture readback. Android
movie-to-screen composition requires the pending native scene renderer port and
is rejected instead of executing the removed CPU converter. No Android native
scene-rendering migration is claimed by this change.

Configured CPU output filters likewise do not acquire a software movie converter.
The native backend must be available for movie-to-screen composition.

The existing decoder contract remains even-sized, tightly packed, 8-bit YUV420
VP8 frames. Dynamic resolution and additional codec/color-metadata policies are
separate decoder work. Texture size and allocation limits remain device limits.
Higher reconstruction filters use fixed support; strong downscaling can alias,
and wider kernels can ring around sharp transitions. They are not area filters
or a mipmapping implementation.

## Verification

`tests/run_gpu_movie_tests.sh` exercises production GPU movie rendering against
an independent scalar YUV and resampling oracle. The complete
`tests/run_gpu_tests.sh` includes this suite plus playback-controller, script,
production presenter/capture, ownership and context-lifecycle coverage.

The native suite checks 190 complete images and 115,553 RGB pixels. Nearest,
cubic and Lanczos reconstruction agree with the independent long-double oracle
to within one RGB byte. Preserved hardware bilinear filtering can differ from
ideal arithmetic by three RGB levels on sharp YUV patterns; 30 additional
comparisons against an independent copy of the old hardware shader remain
within one RGB level on the same driver. These comparisons include direct
fullscreen output and adversarial gamma/brightness inputs.

Failure tests inject actual plane-upload errors and verify unchanged targets,
complete retry, generation rollback, released owner reuse and subsequent scene
drawing. Controller tests exercise defaults, strict script selection, frame
promotion, cross-output ordering, sizing, failed-draw acknowledgement, capture
isolation and channel retirement. The Android adapter also has an accelerated
host-SDL fixture for plane caching, sampling, context recreation, capture state
and return-to-game behavior; it is not a device GLES scene-rendering test.

The development host uses Mesa llvmpipe for actual OpenGL execution. This
validates behavior and resource ownership but does not measure physical-GPU
throughput. Platform compilation and real-game playback remain distinct from
these deterministic tests.
