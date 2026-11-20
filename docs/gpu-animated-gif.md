# GPU animated GIF playback

Author: Damon V. Caskey

Date: 2026-11-20

## Scope

Animated GIF playback uses the shared native scene renderer on Windows, Linux,
macOS and Android. Its output canvases are RGB32 screens. Indexed GIF pixels
and color tables describe source artwork; they do not introduce another scene
color mode or application architecture.

The CPU still reads the GIF stream and performs LZW decompression. Palette
lookup, transparent frame accumulation and OpenBOR's three-stream RGB
composition execute on the GPU. There is no CPU palette-expansion or channel-
merging rendering fallback.

## Frame pipeline

1. The decoder reads one image rectangle and its applicable color table.
2. LZW decompression writes tightly packed palette indices into reusable CPU
   staging storage. Interlaced input is placed in ordinary row order.
3. The completed rectangle is submitted to the native renderer with its
   position, palette and transparent index.
4. The renderer uploads indices, looks up their RGB colors in the shader, and
   writes visible pixels into the retained RGB32 canvas. Transparent pixels and
   pixels outside the rectangle leave existing canvas pixels intact.
5. Ordinary screen draws sample the resident canvas, including the selected
   drawmethod resampling and other supported transformations.

The raw upload descriptor accepts arbitrary rectangle widths, including one-
and three-pixel patches. GIF canvases also allocate their exact logical width
and height, avoiding the general screen allocator's four-pixel width rounding.
This is local to GIF storage and does not change general screen allocation.

GIF palettes are small CPU-prepared lookup tables. Per-pixel RGB expansion no
longer runs in the decoder. Normal playback uploads one source index per patch
pixel instead of uploading a CPU-expanded RGB canvas.

## Three-stream RGB GIFs

OpenBOR's existing source convention remains `namer.gif`, `nameg.gif`, and
`nameb.gif` for a requested `name.gif`. The existing indexed-asset configuration
selects this form when all three files are present. The source-asset format is
separate from the RGB32 scene format.

Each stream retains its own RGB32 canvas. After decoding, one native pass
combines their corresponding RGB bytes with the existing bitwise OR operation.
It does not assume that a stream contains only its named channel. This preserves
the behavior of source palettes and explicit edits to the component screens.
The unused packed high byte remains outside the engine's RGB color contract.

The combined screen remains resident and can be drawn repeatedly without
downloading and merging the three canvases on the CPU.

## Script and playback behavior

Existing entry points remain:

- `playgif(path, x, y, noskip)` for blocking playback.
- `openanigif(path)` to create a script-managed decoder.
- `decodeanigif(handle)` to advance it.
- `getanigifinfo(handle, property)` to inspect the buffer, frame, timing,
  dimensions and completion state.
- `free(handle)` to close a script-managed decoder.

`getanigifinfo(handle, "buffer")` returns the same RGB32 screen interface. Asking
for that pointer or other metadata does not read the canvas back. Explicit
pixel reads and screenshot encoders still synchronize through the common
screen ownership system. Explicit CPU image edits are uploaded when the GPU
next consumes them; GPU drawing into a retained canvas stays native.

Frame counters, delay accumulation, blocking playback scheduling and the
three-stream advancement order are retained. This migration also retains the
existing single-stream index-zero black convention, sticky transparency
selection and ignored disposal controls. It does not add a new GIF disposal or
animation-loop policy. The [GIF89a specification](https://www.w3.org/Graphics/GIF/spec-gif89a.txt)
describes the format independently of these existing OpenBOR playback choices.

## Failure and resource behavior

Malformed or truncated image data is rejected before publishing an incomplete
rectangle. Allocation and native-composition failures end the affected decode
without running a CPU pixel substitute. Frame-staging storage is reused across
frames and released with the decoder.

Runtime decoder state uses natural 64-bit pointer alignment. Byte packing is
retained only for the on-disk GIF header structures, whose layout is defined by
the file format.

The native backend owns its upload textures with the corresponding canvas.
Freeing screens or replacing the context follows the shared resource lifecycle.
Startup diagnostics describe the native GIF path; first use validates its
shader. Rejected native composition is logged with bounded warnings.

## Verification

Run the focused desktop suite:

```sh
OPENBOR_REQUIRE_GPU_TESTS=1 bash tests/run_gpu_anigif_tests.sh
```

Run the same image and ownership checks against an actual GLES3 context:

```sh
OPENBOR_REQUIRE_GPU_TESTS=1 OPENBOR_GLES_TEST_CASES=anigif \
    bash tests/run_gles_scene_tests.sh
```

Both are included in `bash tests/run_gpu_tests.sh`. The fixtures exercise the
production decoder and renderer with independent expected pixel values,
including palettes, partial and interlaced rectangles, exact RGB merging,
retained edits, failure handling and the absence of normal-playback readbacks.
Host GLES execution checks the shared Android shader path; Android device
drivers, suspension and performance remain device-validation work.
