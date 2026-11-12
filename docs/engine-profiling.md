# Engine rendering profiler

Author: Damon V. Caskey

Date: 2026-11-12

## Purpose and scope

`experiment/gpu-profiling` measures engine boundaries around the existing
renderer. `experiment/gpu-palette-cache` retains those measurements while
adding a bounded resident palette cache and cache-specific accounting.
Profiling is enabled by default on both diagnostic branches. Games, including
DC3D, require no script, asset, lighting, or rendering changes.

The profiler separates update phases, main-queue playback, offscreen captures,
CPU-side native-renderer work, presentation, transfers, and software fallbacks.
Optional GPU timestamps provide a second timeline. Existing input order, frame
limiting, game ticks, drawing order, and renderer selection remain unchanged.

These measurements locate engine costs. They do not prove that any proposed
optimization will improve a particular game, and instrumentation has its own
cost. Keep measurement settings identical when comparing workloads.

## Build and run

Build this branch using the normal OpenBOR toolchain. When reusing an existing
CMake build directory, rerun its normal configure command once before building:
the source-file glob must discover the new `engine_profile.c` file.

Launch the resulting executable and enter gameplay. The default collection
skips 60 eligible update entries, then reports each window of 300 complete
frame intervals. Reports go to the normal `Logs/OpenBorLog.txt` file with the
prefix `[ENGINE PROFILE]`; a console window is unnecessary.

Windows PowerShell example:

```powershell
$env:OPENBOR_PROFILE = "1"
$env:OPENBOR_PROFILE_FRAMES = "300"
$env:OPENBOR_PROFILE_WARMUP = "60"
.\OpenBOR.exe
```

Linux shell example, using the executable name produced by the local build:

```sh
OPENBOR_PROFILE=1 OPENBOR_PROFILE_FRAMES=300 OPENBOR_PROFILE_WARMUP=60 ./OpenBOR
```

These are environment settings inherited at launch, not new engine command-line
flags or script APIs. Save each run's log before starting the next run because
normal engine startup recreates its log.

| Environment variable | Default | Meaning |
| --- | --- | --- |
| `OPENBOR_PROFILE` | Enabled on this branch | Exact value `0` disables collection and its timer reads. |
| `OPENBOR_PROFILE_FRAMES` | `300` | Accepted intervals per full report, from `1` through `1000`. Invalid values use the default. |
| `OPENBOR_PROFILE_WARMUP` | `60` | Eligible update entries skipped after startup or a reset, from `0` through `100000`. Invalid values use the default. |
| `OPENBOR_PROFILE_GPU` | Enabled when supported | Exact value `0` disables GPU timestamps while leaving CPU profiling and native rendering enabled. |
| `OPENBOR_GPU` | Native renderer enabled when supported | Existing exact value `0` selects software scene drawing while retaining the selected OpenGL presenter. This is independent of timestamp sampling. |

Renderer configuration is logged when the video mode is selected. Check
`kind=video_config`, `kind=video_backend`, `kind=renderer`, and `kind=gpu_timer`
before interpreting timing. They identify frame-limit selection, output-filter
state, dimensions, active native drawing, requested/actual swap interval, driver
information, and optional timer availability. The palette-cache branch also
logs `kind=palette_cache` when its resident atlas is enabled, including capacity,
palette bytes, GPU logical bytes, CPU cache bytes, and the `content_lru` policy.
OpenGL presentation alone does not establish that scene drawing used the GPU.

## Frame sampling and window boundaries

Each sample has a unique engine frame ID. Its update-body duration is complete
at the end of `update()`, but its entry-to-entry interval becomes known only
when the following eligible update begins. The collector keeps that body
pending until the matching next entry arrives.

This distinction prevents an interval from being paired with the following
frame's work. It also means the final pending body at shutdown cannot be used
as a complete frame sample.

Full reports deliberately skip the entry that publishes them. Log formatting,
percentile calculation, and log I/O therefore do not inflate the next sampled
interval. Normal reporting skips one entry without repeating warmup. Scene,
pause/menu, context, and explicit reset boundaries restart warmup.

Interrupted updates are discarded. Previously accepted samples can be published
as a partial report at the next reset or shutdown. Nested updates also create
boundaries; old frame tokens cannot add time to the new frame. Intervals over
one second, backward clocks, and intervals shorter than their completed update
body are rejected as discontinuities rather than reported as slow rendering.
Each rejection is logged as `kind=gap`. The one-second cutoff is a sampling
policy, not proof that the engine was suspended: genuine stalls that long also
appear in those records and must be considered alongside the window statistics.

`partial=1` reports remain useful, but their sample count differs from the
configured full-window size. Do not average window FPS values to combine runs.
Pool accepted intervals and frame counts, then calculate:

```text
combined frame mean = total accepted interval time / total accepted frames
combined effective FPS = total accepted frames / total accepted interval seconds
```

The reported p95 is the exact nearest-rank percentile of that window's measured
intervals: sorted sample number `ceil(0.95 * frames)`. Window p95 values cannot
be averaged to reconstruct a run-wide p95.

## Reading a report

Every record carries a `kind`; window records also carry the same `window` ID.
CPU durations and displayed GPU durations are milliseconds. Internal CPU
accumulation uses integer microseconds; GPU timestamps retain integer
nanoseconds until reporting.

| Record kind | What it establishes |
| --- | --- |
| `frame` | Accepted count, full/partial status, frame-ID range, interval and update means, outside-update residual, effective FPS, minimum, p95, and maximum interval. |
| `phases_exclusive` | Nonoverlapping CPU wall-time checkpoints partitioning the measured update body. Their means sum to `update_mean_ms`. |
| `spans_inclusive` | Nested operation durations, such as queue playback, uploads, or swap. These overlap the phases and may overlap one another. |
| `counts_mean` | Per-accepted-frame mean operation counts and logical byte volumes. |
| `counts_max` | Highest single-frame value for each count/byte field in the window. |
| `fallback_totals` | Number of declined native operations by reason, including an absent native backend. |
| `gpu_elapsed` | Completed timestamp samples for one scope, mean per available sample, and sum divided by accepted frame count. |
| `diagnostics` | Cancelled frames, rejected gaps, late GPU results, clock-read count, and the skipped-reporting-frame marker. |
| `palette_cache` | Native-cache configuration, including capacity and memory sizes; this is a lifecycle record, not a per-window measurement. |

### Exclusive CPU phases

| Field | Engine boundary |
| --- | --- |
| `profile_entry_ms` | Ready-query polling and sample-pair bookkeeping at update entry. |
| `pre_wait_ms` | Existing pre-input refresh pacing. |
| `input_ms` | Input interval/polling, recording/playback, and movie input processing. |
| `update_script_ms` | `execute_updatescripts()`. |
| `simulation_ms` | Key scripts and the fixed-step logic/entity catch-up loop. |
| `draw_setup_ms` | Main-screen clear, native background/status setup, and entity draw queueing. |
| `draw_script_ms` | `execute_updatedscripts()` and remaining pre-queue work. Native captures performed inside scripts are included here. |
| `main_queue_ms` | Final `spriteq_draw()` into the frame's main screen, including sort and playback. |
| `overlay_ms` | Screenshot/debug-overlay work after the main queue. |
| `present_ms` | Existing wait/presentation call, including any nested swap or limiter delay. |
| `finish_ms` | Remaining queue cleanup and music servicing through the end of `update()`. |

`outside_update_ms` is the entry-to-entry interval minus the measured update
body. It represents work or delay outside that body, not an unidentified GPU
rendering measurement. Likewise, CPU time inside a GL call can include driver
work or synchronization; it is not automatically shader execution time.

The current VSync path waits approximately one refresh period minus 4 ms before
polling input. The constant is not a fixed 4 ms sleep. `pre_wait_ms` measures
the actual retained delay. The 200/500 FPS limiter and buffer swap have their
own nested spans; the profiler changes none of these pacing rules.

### Nested spans and work counts

`main_sort_ms` and `capture_sort_ms` distinguish queue sorting by destination.
`main_playback_ms` and `capture_playback_ms` time accepted-command dispatch after
sorting. Main classification uses the frame's main-screen pointer; other
targets are captures. These are inclusive costs within whichever update phase
called the queue, not extra costs to add to the phase totals.

Queue `entries` counts the complete queue seen by each playback call.
`accepted` counts commands remaining after lock/new-only and Z-range filtering.
Rasterizer clipping can still produce no pixels. Replaying one queue into
several captures counts it several times; these fields are not unique sprite
counts.

Native counters distinguish successful, nonempty render invocations
(`native_draws`) from their quads, submitted batches, and clears. Repeated draws
can create multiple native invocations for one accepted queue command; empty
or fully clipped commands can create none. These counters are therefore not
one-for-one replacements for accepted queue-command counts. Quad/batch counts
describe the engine's native submission groups, not physical GPU utilization.
CPU native spans separate geometry preparation, state/material work, and
command submission.

Image, palette, and blend-table uploads have separate counts and byte totals.
Readbacks count actual GPU-to-CPU image synchronization. Destination copies
count GPU snapshots used by blending or self-drawing; self-copies are also
identified separately. Byte values are logical payload sizes, not measured
bus traffic or GPU memory-bandwidth consumption.

`software_ms` and the software draw/clear/copy counters cover the instrumented
high-level rendering fallbacks. They do not claim to count every low-level CPU
pixel routine in the engine. When a fallback first synchronizes a GPU-authored
screen, its inclusive software span includes that readback; `readback_ms`
also records the transfer itself. Adding both would count that time twice.

### Resident palette-cache accounting

The palette-cache branch keeps up to 1024 complete 256-color palettes in a
256-by-1024 RGBA8 atlas. Its logical GPU payload is 1,048,576 bytes (1 MiB).
CPU content copies also occupy 1 MiB, with fixed additional lookup/LRU metadata.
The configuration record's `cpu_bytes` reports the complete cache structure's
actual size for the build (1,073,168 bytes with 32-bit `int` and the current
layout); driver allocation overhead is not measured.

| Field | Meaning |
| --- | --- |
| `palette_lookup_ms` | CPU selected-row comparison, content hashing and collision verification, victim selection, and hit-side LRU maintenance, ending before any texture upload. |
| `palette_cache_hits` | Palette lookups whose complete contents already have a resident atlas row. |
| `palette_cache_misses` | Palette lookups whose contents are absent and require an attempted row upload. |
| `palette_cache_evictions` | Occupied rows replaced after a successful upload when the bounded cache is full. |
| `palette_uploads` / `palette_upload_bytes` | Actual successful palette updates and their logical payload; their existing meaning is unchanged. |

Cache counts use the usual per-frame means in `counts_mean` and single-frame
maxima in `counts_max`. Hits plus misses count eligible palette lookups, not
unique palettes. Reusing an evicted palette produces another miss, so misses
also cannot be interpreted as a distinct-palette count. When useful, compute
the hit fraction as `hits / (hits + misses)` with a nonzero denominator.

Content identity, rather than pointer identity, lets equal palettes at different
addresses share one row. Consecutive reuse first compares the selected row's
complete contents; other lookups hash the contents and verify matching
candidates in full. These checks guard against collisions and detect edits
made in place. No caller palette pointers are retained. Least-recently-used
victim selection and list updates are deterministic and constant-time; hash
bucket traversal and content verification remain visible in `palette_lookup_ms`.

Lookup and upload are separate spans within the calling frame phase. They are
not extra time to add to `main_queue_ms`, `draw_script_ms`, or frame time.
`upload_ms` remains the combined CPU duration of instrumented image, palette,
and table uploads, so inspect their individual counters to identify the source
of changes. Source-image uploads are independent of palette residency.

Successful-miss content copies, hash-chain publication, and eviction/insertion
bookkeeping happen after the upload. They remain within the calling frame
phase but are not included in `palette_lookup_ms` or `upload_ms`. Therefore
lookup time does not claim to measure every CPU cache-management operation.
Misses count attempted residency changes; a failed upload increments neither
successful-upload nor eviction counts, so those totals need not match misses
on an error path.

Cache residency lasts for the native renderer's context lifetime. Frame and
report boundaries, profiler warmup, and menus do not themselves flush it;
native draws during unmeasured periods can populate or update it. Native
shutdown or context recreation discards the old residency, and successful
initialization begins with an empty atlas. An upload error also invalidates
residency rather than reusing uncertain GPU contents. `OPENBOR_PROFILE=0`
disables measurement while leaving the cache active; `OPENBOR_GPU=0` disables
the entire native renderer. No cache-only runtime switch is provided. The first
measured gameplay window is therefore not necessarily a cold-cache window.

### Optional GPU timestamps

Desktop GPU timestamps require OpenGL 3.3 or `GL_ARB_timer_query`, the complete
timer-query entry points, and a 64-bit timestamp counter. Unsupported drivers
keep CPU profiling and native drawing. Zero GPU samples mean no usable result
was attached to that window, not that GPU work took zero time.

The `scene` scope brackets scene drawing from the main clear through post-queue
overlay work. `main_queue` and `capture` bracket their respective queue playback
calls. `present` brackets the GL presentation commands and ends before the
buffer-swap wait. Scene and queue/capture samples overlap and must not be added
together.

These are elapsed GPU command spans. Their boundaries can include GPU idle time
while the CPU prepares and submits later commands. They are not exclusive
GPU-busy time, shader-only time, or CPU frame duration.

Results are read only after both endpoints are already available. There is no
query-induced `glFinish`, `glFlush`, spin wait, or blocking timestamp read.
The bounded pool holds 256 outstanding pairs; saturation drops measurement
samples instead of stalling drawing. Teardown records unavailable/open samples
as abandoned rather than waiting for them.

Each result retains its originating frame ID. Results can join a current,
pending, or already accepted frame while that window remains open. Once the
window has been reported or the frame discarded, later results increment
`late_gpu_results` instead of being reassigned to the current frame. Published
reports are not retroactively rewritten.

`sample_mean_ms` divides by available GPU samples. `per_frame_ms` divides that
same accumulated duration by accepted CPU frames. Multiple capture samples per
frame and incomplete availability make those denominators different. Check
sample coverage, dropped/skipped counts, and late-result diagnostics before
comparing GPU means. Incomplete coverage can bias an apparently fast result.

## Suggested comparison procedure

1. Use the same build, machine, resolution, display mode, output filter, and
   frame-limit setting for each comparison. Confirm those settings in the log.
2. Reproduce the same camera position or route and visible scene. Compare the
   point-light workload against the matched no-point-light workload using the
   game's existing controls or test content. Do not add script optimizations
   between runs.
3. Collect several full windows after warmup. Avoid menus, screenshots, scene
   loading, and configuration changes in the measured section unless that
   operation is the subject of the test.
4. Compare phase means first, then inspect the nested spans and counters within
   the phase that changed. Use p95/max to distinguish a general slowdown from
   intermittent stalls. Save both complete logs.
5. Repeat with `OPENBOR_GPU=0` only when testing the native/software boundary,
   retaining the same presenter and pacing. Repeat with
   `OPENBOR_PROFILE_GPU=0` to estimate timestamp instrumentation overhead without
   switching renderers. Finally compare `OPENBOR_PROFILE=0` when assessing the
   total diagnostic overhead.

No expected speedup is encoded in this procedure. Heavy captures may remain
GPU-resident while CPU setup, command count, destination snapshots, pacing, or
software fallbacks dominate another part of the frame. The combined timing
and work counters are intended to distinguish those cases.

### Comparing the resident-palette change

Use profiling commit `722df2d` as the baseline and
`experiment/gpu-palette-cache` as the changed renderer. This step changes palette
residency only; cross-entry state caching, draw batching, destination-copy
optimization, frame pacing, and game scripts remain outside its scope.

1. Build both revisions with matching compiler/build settings, then keep the
   same game content, machine, resolution, output scaling/filter, and frame
   limit. Confirm `native_active=1` and actual swap interval in both logs, and
   `kind=palette_cache state=enabled` in the changed build.
2. Reproduce the same stationary view or repeatable route, including the same
   point-light workload. Collect several complete windows after warmup and
   preserve each complete log. Restart the executable before each independent
   trial; menu/loading draws can still populate residency before gameplay.
3. Compare `main_queue_ms`, `upload_ms`, `palette_uploads`, and
   `palette_upload_bytes` first, then frame mean/p95 and effective FPS. The new
   branch's `palette_lookup_ms` and hit/miss/eviction fields show the CPU work
   exchanged for fewer uploads. Those fields do not exist in the baseline log;
   their absence is not a measured zero.
4. Keep first-observed and steady-state windows separate. Frequent misses with
   evictions can indicate a working set exceeding capacity or continuously
   changing contents; frequent hits with lower uploads demonstrate reuse. Do
   not expect every workload to fit the cache or every miss to be a new unique
   palette. Compare source-image and table upload counts independently.
5. Verify output and ordering at the same views. Repeat matching runs with GPU
   timestamps disabled, then with all profiling disabled, when evaluating
   observer overhead. The lookup instrumentation itself adds clock reads, and
   `profile_entry_ms` does not capture all per-operation measurement overhead.

Fewer uploads establish reduced palette-transfer work, not a guaranteed total
frame-rate improvement. Per-draw state and submission costs, destination copies,
and script execution may remain dominant after palette reuse improves.

## Regression tests

Run `bash tests/run_gpu_tests.sh`. The context-independent suite checks disabled
operation, environment defaults/bounds, warmup, pending interval pairing,
partial/full windows, report barriers, nested scopes, counters/bytes, pauses,
nested updates, context resets, long gaps, nearest-rank p95, and delayed GPU
results across cancellation and collector reconfiguration. Palette accounting
also checks lookup/upload separation, hit/miss/eviction totals and maxima,
disabled collection, and the public log names and per-frame units.

With SDL2 development files and an available desktop OpenGL context, the runner
also verifies real native counters and byte totals, independent timestamp
opt-out, nested timestamps, pool saturation, preservation of existing GL errors,
and stale tokens after actual context recreation. Pixel reads in these tests
validate image contents and ordinary coherence; production timestamp polling
itself never adds a wait.

`OPENBOR_REQUIRE_GPU_TESTS=1` makes missing native-test context support a failure.
Timer-query capability remains separately optional; a driver without it skips
only timestamp-specific assertions while native CPU/counter checks still run.
The existing queue, transform, color, capture, coherence, shutdown, and
presentation regression suites remain in the same runner.

The native suite also passes with an OpenGL 2.1 context and
`GL_ARB_timer_query` disabled, retaining CPU/counter coverage while skipping
only optional timestamps. The supported timestamp path has been exercised
under address, undefined-behavior, and float-cast-overflow sanitizers.

Headless Mesa/llvmpipe execution validates behavior and accounting, not hardware
performance. Real-device logs and matched gameplay runs remain necessary for
performance conclusions.
