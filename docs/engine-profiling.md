# Engine rendering profiler

Author: Damon V. Caskey

Date: 2026-11-14

## Purpose and scope

`experiment/gpu-profiling` measures engine boundaries around the existing
renderer. `experiment/gpu-palette-cache` retains those measurements while
adding bounded resident palettes, context-local graphics-state caching, and
cache-specific accounting. It also provides sampled interpreter/native-call
profiling within the existing frame phases, compiled native-call descriptors,
and an opcode body embedded in the interpreter execution loops. These changes
reduce repeated native-dispatch setup and per-opcode call boundaries.
Successive changes remain on that same branch; use the individual commits below
for controlled comparisons.
Profiling is enabled by default on both diagnostic branches. Using the profiler
requires no game script, asset, lighting, or rendering changes. New drawmethod
color rules are separately opt-in; they are not enabled in DC3D automatically.

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
| `OPENBOR_PROFILE_SCRIPT` | Enabled on this branch | Exact value `0` disables detailed interpreter/native-call timing without disabling the existing frame, renderer, or GPU reports. |
| `OPENBOR_PROFILE_SCRIPT_INTERVAL` | `16` | Measure all interpreter/native transitions within one eligible recorded frame out of this many, from `1` through `1000`. Invalid values use `16`. Value `1` measures every recorded frame and has greater observer cost. |
| `OPENBOR_PROFILE_GPU` | Enabled when supported | Exact value `0` disables GPU timestamps while leaving CPU profiling and native rendering enabled. |
| `OPENBOR_GPU` | Native renderer enabled when supported | Existing exact value `0` selects software scene drawing while retaining the selected OpenGL presenter. This is independent of timestamp sampling. |

Renderer configuration is logged as readable sections when the video mode is
selected. Check **Starting Values**, **GPU CAPABILITY REPORT**, **Active
Presentation**, and **GPU Timing** before interpreting timing. They identify
frame-limit selection, output-filter state, dimensions, active native drawing,
requested/selected VSync, driver information, and optional timer availability.
The first settings snapshot is labeled **Starting Values** because the user can
change these options through the menu. Subsequent snapshots are labeled
**Values at Video Reinitialization**; neither label implies that the settings
remain fixed for the full session.

Startup diagnostics use headings, tab-indented labeled rows, readable option
names and explicit units. For example, the stored frame-limit value `2` appears
as `200 FPS`. These sections replace the older dense `kind=video_config`,
`kind=video_backend`, and renderer/cache configuration records. Palette and
graphics-state cache sections retain capacity, policy and memory sizes.
OpenGL presentation alone does not establish that scene drawing used the GPU.

**Measurement Settings** and **Script Profiling** describe the profiler's own
configuration. Periodic frame, timing, counter and script-detail records retain
their structured fields and window identities for comparison and analysis.

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

Periodic measurement records carry a `kind`; window records also carry the
same `window` ID. Startup and lifecycle sections use descriptive headings.
CPU durations and displayed GPU durations are milliseconds. Internal CPU
accumulation uses integer microseconds; GPU timestamps retain integer
nanoseconds until reporting.

| Record or section | What it establishes |
| --- | --- |
| `frame` | Accepted count, full/partial status, frame-ID range, interval and update means, outside-update residual, effective FPS, minimum, p95, and maximum interval. |
| `phases_exclusive` | Nonoverlapping CPU wall-time checkpoints partitioning the measured update body. Their means sum to `update_mean_ms`. |
| `spans_inclusive` | Nested operation durations, such as queue playback, uploads, or swap. These overlap the phases and may overlap one another. |
| `counts_mean` | Per-accepted-frame mean operation counts and logical byte volumes. |
| `counts_max` | Highest single-frame value for each count/byte field in the window. |
| `fallback_totals` | Number of declined native operations by reason, including an absent native backend. |
| `gpu_elapsed` | Completed timestamp samples for one scope, mean per available sample, and sum divided by accepted frame count. |
| `diagnostics` | Cancelled frames, rejected gaps, late GPU results, clock-read count, and the skipped-reporting-frame marker. |
| `Renderer Caches` section | Palette and graphics-state cache configuration, including capacities and memory sizes; this is a lifecycle report, not a per-window measurement. |
| `Script Profiling` section | Detailed script-profiling enablement, whole-frame sampling interval, fixed limits, and the explicit inclusion of instrumentation overhead in measured time. |
| `script_summary` | Valid detailed, invalid detailed, and nondetailed frame counts; separate detailed/nondetailed interval and update means; detailed clock reads, errors, and registry coverage. |
| `script_phase` | Per-phase interpreter/native/outside split, using only valid detailed frames as its denominator. |
| `script_native` | Up to eight native function names per phase, ranked by accumulated exclusive time, with measured calls and per-sample/per-call means. |
| `script_slow` | One of up to three slowest valid detailed frames, identified by its frame ID, complete interval, update duration, outside-update residual, and detail clock reads. |
| `script_slow_phases`, `script_slow_spans`, `script_slow_counts` | Matching ordinary phase/span/work data for that individual slow sampled frame. |
| `script_slow_split`, `script_slow_native` | The same frame's per-phase interpreter/native split and up to eight highest-cost native phase/name entries. |

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

### Sampled interpreter and native-call profiling

The `draw_script_ms` phase includes both interpreted execution and native engine
functions called by scripts. Its name alone cannot establish which part is
expensive. Detailed script profiling measures those boundaries in the engine;
DC3D and other modules require no instrumentation or gameplay changes.

By default, one complete eligible recorded frame out of every 16 receives
detailed timing. Every interpreter/native transition within that selected frame
is measured. Individual native calls are not sparsely sampled or extrapolated.
The existing all-frame reports still contain every accepted frame, including
both detailed and nondetailed frames.

| Detailed phase component | Meaning |
| --- | --- |
| `interpreter_self_ms` | Exclusive wall time inside the instrumented interpreter execution scopes while no native engine function owns the current execution context. This includes script instruction execution and interpreter dispatch work, not just script arithmetic. |
| `native_self_ms` | Exclusive CPU-side wall time in script-callable native functions. Nested interpreter callbacks, and native calls made by those callbacks, are charged to their own execution contexts instead of being counted twice. |
| `outside_interpreter_ms` | The matching phase's remaining wall time outside those measured interpreter/native scopes, including host-side script setup and surrounding engine work. It is not an estimate of GPU execution time. |

Those components partition each phase for the same valid detailed samples.
They are a breakdown of existing phase time, not additional time to add to the
frame. Inclusive renderer spans can lie inside `native_self_ms`; adding those
spans again would double-count their cost. Native CPU wall time can include
driver work or waits and remains distinct from optional GPU elapsed spans.

Each native function is identified internally by a bounded registry ID and
reported by its script-call name, resolved while CALL instructions are compiled.
Runtime dispatch uses the cached ID instead of scanning names for each
invocation. IDs survive collector resets within that process; compare function
names, not numeric IDs, across independent runs. The log ranks up to eight
native functions within each frame phase, so the same function's draw-script
costs can be distinguished from its simulation costs. Ranking is by exclusive
time, then call count, with stable phase/ID tie ordering. The phase total still
includes native work below that reporting cutoff.

`calls` and `native_calls` are measured invocation totals, while
`calls_per_sample` and `self_ms_per_sample` use valid detailed frames as the
denominator. `self_us_per_call` is accumulated exclusive time divided by the
recorded calls, not an independently measured latency percentile. Counts refer
to native entries in that phase; execution time crossing a phase boundary is
split at the boundary. These fields do not describe all calls made by the game
across the complete window.
If execution continues into a phase with no native entry, that phase can have
nonzero self time and zero calls. Its per-call mean is unavailable (displayed
as zero), not evidence of free execution.

Detailed means use their own valid sampled-frame count. They must not be
compared as if they used the ordinary window's denominator. The detailed report
also identifies invalid and nondetailed frames. An independent nondetailed-frame
mean helps distinguish the workload under ordinary profiling from frames with
extra call-boundary instrumentation; it is still not an uninstrumented baseline
or a causal overhead estimate. Workload can differ between selected frames,
and detail acceptance bookkeeping can run at the following frame's entry.
`sampled_frames + invalid_frames + unsampled_frames` equals the ordinary
accepted frame count. Zero samples indicate unavailable detail, not zero cost.

#### Matching individual slow frames

Each reporting window retains up to three slowest valid detailed samples,
ranked by their complete entry-to-entry intervals. Their frame IDs, phase
durations, inclusive renderer spans, work counters, interpreter/native split,
and up to eight most expensive native `(phase, function ID)` entries refer to
the same frame. These records make it possible to associate a particular slow
frame with the operations performed in that frame instead of attributing a
window-wide average to its maximum or p95.

These are the slowest **sampled** frames, not necessarily the slowest frames in
the whole window. Nondetailed frames have no native-function breakdown, and
instrumented frames can be slower because of measurement itself. Set
`OPENBOR_PROFILE_SCRIPT_INTERVAL=1` for an every-frame diagnostic run when
coverage is more important than observer cost. Even then, retain the existing
rules for warmup, cancelled frames, discontinuities, and reporting barriers.

Details travel with the originating frame through pending interval pairing.
Cancelled, rejected, or final unpaired frames cannot contribute native-function
totals to an accepted report. Logging and selection of slow samples occur at
separate boundaries: snapshots are selected once a completed interval is
accepted, and logging is deferred until the report. Neither occurs inside
individual native calls.

#### Bounds, diagnostics, and observer overhead

Storage is bounded: 512 function-ID slots include ID zero, `other_native`;
registered names use 96-byte buffers including the terminator; the
execution-context stack has 128 entries. Compiled instructions also retain one
unsigned native-ID field, including when detailed timing is disabled.
Unclassified or unregistrable native functions remain visible through
the fallback ID rather than silently disappearing from native totals. Detailed
stack overflow, mismatched scopes, invalid clocks, or unclosed scopes invalidate
that frame's detailed accounting. Its otherwise valid ordinary frame sample is
retained, and detailed diagnostics identify the discarded breakdown.

`error_flags` combines bit values `1` for stack depth, `2` for scope order or
identity, `4` for a detail-clock discontinuity, and `8` for unclosed scopes.
The summary's `clock_reads` includes detail reads from invalid accepted frames
as well as valid ones. `registry_names` counts the process-lifetime registered
names excluding `other_native`; `registry_fallbacks` counts registration
attempts rejected for invalid/oversized names or exhausted capacity. It is
cumulative across windows, not a per-window unknown-call count.

Timing occurs at execution-category boundaries rather than on every opcode.
Nondetailed frames bypass the detail timing and registry lookup path. Nevertheless,
selected frames incur additional timer reads and accounting work. That observer
cost remains in measured wall time; the profiler does not subtract a guessed
overhead or claim that sampled-frame FPS is unperturbed. Detailed clock-read
counts expose instrumentation volume, not an exact overhead duration.

The clock has microsecond resolution. An individually tiny native call can
therefore record zero microseconds while still incrementing its call count.
Inspect accumulated costs across sufficient calls and repeated runs rather
than interpreting a zero-duration call as free work. High call counts can also
make profiling overhead significant even when individual functions are cheap.

For a comparison with earlier renderer-profile logs, disable only this new
detail layer before launching the executable:

```powershell
$env:OPENBOR_PROFILE_SCRIPT = "0"
.\OpenBOR.exe
```

For a detailed run, restart with `OPENBOR_PROFILE_SCRIPT=1`; optionally set
`OPENBOR_PROFILE_SCRIPT_INTERVAL=1` for complete eligible-frame coverage or
leave it at `16` for the default cadence. Keep the same scene, route, pacing,
and existing CPU/GPU profiler settings when comparing runs. The detail opt-out
does not disable palette residency, graphics-state caching, or native drawing.

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
command submission. Buffered submission retains logical quad counts;
`native_batches` counts indexed-triangle draw calls, one per contiguous
sampling-mode run. `gpu_submit_ms` includes vertex-buffer preparation/upload,
indexed drawing and their error checks. Index storage is cached and rebuilt
only when its capacity must grow or recover from a resource failure. These
geometry transfers do not increment image-upload counts or byte totals.

### Drawmethod color-rule accounting

The [drawmethod color API](drawmethod-colors.md) adds these fields to the
existing `counts_mean` and `counts_max` records:

| Field | Meaning |
| --- | --- |
| `native_chroma_draws` | Successful, nonempty native submissions executing at least one chroma key. |
| `native_substitute_draws` | Successful, nonempty native submissions executing at least one Substitute pair. |

One native submission may increment both fields. They count rules actually
executing in the shader, not merely enabled settings. Ignored rules increment
neither field. The counters do not count matching pixels, substitutions
performed, unique sprites, or unique queue entries. Native repeats can create
multiple submissions; do not add both fields as a total draw count.

These controls have no CPU implementation. Missing shader support leaves
ordinary native drawing available and ignores the rules; existing software
draws also ignore them. There is no software-color counter or color-rule
fallback reason. Other causes, including an absent backend, unsupported
geometry, or missing resources, retain their existing fallback reasons.

Startup's separate **Color Rules** checklist reports capability with profiling
disabled as well. The counters establish actual native-path use during sampled
frames, while the startup report establishes availability. Software OpenGL
drivers such as llvmpipe can execute that native path on the CPU, so neither
signal alone establishes physical GPU utilization or a frame-rate gain.

Color rules alone do not require destination copies or readbacks. Existing
blending, self-drawing, or software fallback can still require them. Compare
matching draw workloads and transfer counts before attributing a change to
chroma or Substitute.

### Cached graphics-state accounting

The native renderer remembers context-local framebuffer, viewport, scissor,
fixed raster, program, texture-binding, material-uniform, and vertex/index
binding and attribute-array state. Cached setters issue a GL call only when
its required value changes or the previous
state is unknown. This avoids redundant driver calls without reordering draws
or combining queue entries into batches.

| Field | Meaning |
| --- | --- |
| `gpu_state_changes` | Cached GL setter calls actually emitted during successful native draw/clear operations. |
| `gpu_state_reuses` | Cached GL setter calls avoided because the required state was already known to match. |
| `gpu_state_ms` | Existing CPU state/material setup span; its timing boundary is unchanged. |

Each successful native quad submission publishes an aggregate delta from
`gpu_render_quads()`: palette/table/snapshot preparation within that function,
material/setup, geometry binding and geometry-wrap handling. Source/target
image creation or uploads performed by its caller before that boundary are excluded. Clear
operations include their target preparation and clear setup. These deltas are
therefore not limited to setters inside `gpu_state_ms`, nor do they count every
resource operation. Independent presentation/readback helpers are also excluded
from the frame counters. The renderer's internal 64-bit lifetime statistics
include every cached setter, including those excluded preparations/helpers.

The two counts use per-accepted-frame means in `counts_mean` and single-frame
maxima in `counts_max`. They count setter calls, not draws, unique state values,
hardware commands, GPU occupancy, or time saved. The first draw after
invalidation must establish state again, so required state restoration is a
change, not a cache failure. Logical reuse is evidence of avoided calls, not a
guaranteed frame-rate improvement.

`gpu_state_ms` does not expand to cover every counted setter. Resource
preparation remains in its calling phase, image transfers remain in `upload_ms`
or `readback_ms`, geometry uploads belong to `gpu_submit_ms`, and snapshots
remain in `dest_copy_ms`. Those nested spans already
belong to their enclosing update phases; do not add them to frame time or infer
per-setter latency by dividing the setup-only span by the wider operation count.

Presentation/external helper boundaries invalidate state ownership; the next
native operation restores its required state without querying GL to validate
the cache. Resource uploads, clear/readback operations, deletion, recovery, and
context teardown update or invalidate tracked state as appropriate. No new
GPU wait, flush, draw reordering, or hot-path `glGet` validation is introduced.
`OPENBOR_PROFILE=0` leaves state reuse active while disabling its frame reports.
The startup graphics-state cache section reports its tracked texture-unit count
and the CPU cache structure's byte size for the current build. This state cache
does not allocate additional GPU image storage.

### Transfers and fallback work

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
   switching renderers, and with `OPENBOR_PROFILE_SCRIPT=0` to check detailed
   script-profiling overhead separately. Finally compare `OPENBOR_PROFILE=0`
   when assessing the total diagnostic overhead.

No expected speedup is encoded in this procedure. Heavy captures may remain
GPU-resident while CPU setup, command count, destination snapshots, pacing, or
software fallbacks dominate another part of the frame. The combined timing
and work counters are intended to distinguish those cases.

### Comparing the resident-palette change

Use profiling commit `722df2d` as the baseline and
palette-cache commit `f2c96c3` as the changed renderer. That commit changes palette
residency only; cross-entry state caching, draw batching, destination-copy
optimization, frame pacing, and game scripts remain outside its scope.
The same branch now contains subsequent state-caching work, so its moving tip
is not a palette-only comparison point.

1. Build both revisions with matching compiler/build settings, then keep the
   same game content, machine, resolution, output scaling/filter, and frame
   limit. Confirm that native drawing and the selected VSync state match in both
   logs, and that the palette cache is enabled in the changed build. Older logs
   use `native_active=1` and `kind=palette_cache state=enabled`; current startup
   sections express those settings with labeled rows.
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

### Comparing redundant-state elimination

Use palette-cache commit `f2c96c3` as the baseline and state-cache commit
`3e2dd97` as the changed renderer. The branch tip now also includes additional
script profiling, so it is not a state-cache-only comparison point. Keep the same scene,
route or stationary view, build options, driver, presentation settings, pacing,
and profiler configuration. Record the exact commit from each executable's log
and collect repeated full windows after warmup.

1. Compare `gpu_state_ms` and the enclosing `main_queue_ms`/`draw_script_ms`,
   then mean/p95/max frame intervals. The state cache may also affect resource
   preparation outside the setup span, so the whole-frame result matters.
2. Inspect `gpu_state_changes` and `gpu_state_reuses` in the changed build.
   Their absence in the baseline is not a measured zero. Match native
   draw/quad/batch counts and scene conditions before attributing differences
   to the optimization; the cache does not intentionally reduce draw count.
3. Compare image/palette uploads, destination copies, readbacks, and fallback
   totals independently. State reuse must not hide an unexpected transfer or
   software fallback. Native sprites already reuse resident index textures;
   repeated draws should not require repeated source-image uploads.
4. Verify exact output around target changes, captures, palette/mask changes,
   clipping, self-drawing, presentation, movie transitions, and context changes.
   Restored state is essential at these ownership boundaries even when it
   reduces the observed reuse fraction.
5. Repeat with GPU timestamps disabled and then all profiling disabled to
   check observer overhead. Neither opt-out disables the state cache.

These counters establish avoided cached setter calls. Only matched real-device
timings establish whether that reduction improves the workload's frame rate.

### Comparing compiled native-call dispatch

Use script-profiling commit `67555aa` as the baseline for the dedicated
`CALL_NATIVE` instruction. This change affects interpreter dispatch only. Native
function bodies, array access and bounds checks, variant arithmetic, argument
evaluation order, rendering, and game scripts are unchanged. It is not a
specialized `get()` implementation or a general bytecode-fusion pass.

After normal target resolution and compilation, eligible native calls retain a
descriptor containing the resolved function, contiguous argument references,
argument count, result slot, argument-list cursor, and profiling ID. Execution
uses those already-validated references instead of repeating the generic
script/import/native selection and parameter-list metadata checks. Argument
values are still evaluated normally on every execution; the descriptor caches
the call's structure, not the results of earlier calls.

Script-local and imported targets retain the ordinary call path and their
existing resolution priority. Native calls retain current-call context,
argument-cursor reset, return/completion handling, native-triggered callbacks,
and error reporting. Detailed native timing still surrounds the same native
function invocation. If optional descriptor allocation fails, the call remains
on its ordinary validated path rather than making an otherwise valid script
fail to compile.

The dedicated native helper and diagnostic formatter use deliberate no-inline
boundaries. Their local storage must not be folded into the shared opcode
dispatcher and make unrelated arithmetic instructions pay a larger stack-frame
cost. This preserves the existing execution loop rather than replacing it.

| Storage | Supported 64-bit build |
| --- | ---: |
| Descriptor payload per compiled native-call site | 40 bytes |
| Existing instruction structure | 80 bytes, unchanged |

These sizes assume the current layout and 32-bit `int`/`unsigned` fields within
the supported 64-bit process. The descriptor reuses a pointer field unused by
ordinary CALL instructions, so it does not enlarge every instruction.
Allocator bookkeeping and padding beyond the descriptor payload are additional
and platform-dependent. Allocation occurs once per compiled call site, not per
invocation; normal instruction teardown releases it. Profiler opt-out does not
disable the optimized call instruction.

1. Build the baseline and changed revision with matching release options. Keep
   the game, camera route or stationary view, driver, display, and pacing
   settings identical, and record the exact commit from each executable.
2. Set `OPENBOR_PROFILE_SCRIPT=0` in both runs, retaining the existing frame and
   renderer profiling. Per-call detailed timing can materially perturb this
   workload, so sampled slow-frame time is not a clean dispatch benchmark.
3. Compare `draw_script_ms` and complete frame intervals across repeated matched
   runs. Check rendering work counts and visual/gameplay behavior as before.
   Then repeat with `OPENBOR_PROFILE=0` to check total diagnostic overhead.
4. Use a separate detail-enabled run only to verify native-call counts and
   attribution. The same functions should still execute; this optimization
   reduces dispatch bookkeeping rather than removing native invocations.

The headless differential benchmark described below is a controlled synthetic
check, not a substitute for this matched game-level comparison or an estimate
of DC3D's expected FPS gain.

### Comparing in-loop opcode execution

Use dedicated-native-call commit `66e79ce` as the baseline for this separate
interpreter change. Both revisions already use `CALL_NATIVE`; this comparison
therefore isolates removal of the C function boundary around each executed
opcode rather than combining it with native-call descriptor changes.

The opcode switch has one shared source implementation. GCC/Clang use the
`always_inline` annotation; the MSVC spelling is `__forceinline`. Run-to-completion execution
and immediate-code scanning embed that implementation directly. The public
`Interpreter_EvalInstruction()` entry point still executes exactly one opcode
for existing callers and regression checks. Other compilers retain a normal
inline hint rather than requiring a compiler-specific extension.

Instruction ordering, current-instruction updates, return/completion behavior,
native and imported callbacks, errors, and profiling boundaries are unchanged.
Immediate scanning retains its separate traversal rules rather than being
converted into the run-to-completion loop. There is no cached program counter,
opcode fusion, variant specialization, new script API, or module change.
The native-call helper and error formatter remain out of line so their larger
local storage does not inflate the ordinary opcode path.

This trades additional compiled machine code at the execution entry points for
fewer call/return boundaries. It adds no heap allocations, per-script storage,
or instruction fields. Exact text size and code layout depend on the compiler
and optimization settings; reduced call count alone is not a guarantee of a
particular whole-game speedup.

Build both revisions with the same release options and repeat the matched
scene procedure above with detailed profiling disabled, then with all profiling
disabled. Compare `draw_script_ms`, complete frame intervals, rendering work
counts, and visual/gameplay behavior. Keep arithmetic-heavy and mixed-script
controls alongside native-heavy tests: the shared opcode execution path affects
all three, not only engine-function calls. Synthetic host benchmarks must use
the same checked workload in separate baseline and changed executables, with
native-call specialization already active in both. Preserve assertions in the
benchmark harness while using normal optimized production objects.

## Regression tests

Run `bash tests/run_gpu_tests.sh`. The context-independent suite checks disabled
operation, environment defaults/bounds, warmup, pending interval pairing,
partial/full windows, report barriers, nested scopes, counters/bytes, pauses,
nested updates, context resets, long gaps, nearest-rank p95, and delayed GPU
results across cancellation and collector reconfiguration. Palette accounting
also checks lookup/upload separation, hit/miss/eviction totals and maxima,
disabled collection, and the public log names and per-frame units.
State accounting checks independent change/reuse totals, per-frame maxima,
public labels, exclusion outside sampled bodies, and unchanged setup/transfer
span accounting.

Script-detail regressions check independent enablement and interval bounds,
whole-frame cadence across reporting barriers, exact exclusive reentry,
same-owner recursion without additional clocks, phase-crossing charges,
zero-duration native calls, valid/invalid/nondetailed denominators, bounded
top-function and slow-frame rankings, copied native names, registry fallback,
scope/clock/depth failures, and cancellation/reset/gap/shutdown exclusion.
The runtime integration fixture executes production parsing, compilation,
imports, immediate code, main/init/clear functions, helpers, native aliases,
callbacks, and native error paths. Its native game-service stubs provide
deterministic work; script execution and argument/return handling remain real.

Dedicated-call regressions compare equivalent compiled scripts through generic
CALL and CALL_NATIVE dispatch. They exercise native argument values and side
effects, result types, script/import resolution, nested callbacks, error
recovery, and profiling ownership. The same fixtures also compare the normal
execution loops against repeated public single-step evaluation for both native
dispatch forms. These routes must retain equivalent observable behavior;
timing is not a correctness assertion.

Instruction-boundary regressions also cover nonunit steps, taken/untaken
branches, return targets, errors, null instruction slots, and already-completed
runs. The public single-step API must still execute one instruction even when
the run-to-completion flag is set. Separate object-code checks verify both
`-O0` and `-O3`: the two execution modes must not call the shared opcode body or
public one-step wrapper, while the public entry point and separate native-call
helper remain present. These code-generation checks catch a compiler silently
declining to inline even when functional tests still pass.

`test_native_call_descriptor` separately checks lowering eligibility, zero and
80 arguments, unchanged instruction layout, and allocator-wrapped failure of
the optional allocation. Ownership checks cover one-time descriptor release
and borrowed string arguments without transferring their lifetime to the
descriptor.

The generated `test_script_profile_integration` test executable also accepts
`--benchmark` for an optional native-heavy interpreter-loop comparison of
detail disabled, interval 16, and interval 1. It rotates three trials per mode,
uses a monotonic host clock, discards log I/O, and reports observations without
timing-based pass/fail assertions. This measures that synthetic host workload,
not the production SDL timer's exact overhead or DC3D/GPU performance.

Its separate `--native-benchmark` option compares generic and dedicated native
dispatch on the same compiled native-heavy workload with profiling disabled.
It rotates execution order across repeated trials and verifies call totals
while reporting elapsed time per native invocation. That metric includes the
surrounding interpreted loop and fixture function body; it is not isolated
dispatch latency. For release-like measurements, compile production objects
with `-O3 -DNDEBUG` but compile the test harness with assertions enabled before
linking them together. The harness rejects `NDEBUG` because it would remove
assert-wrapped execution and validation, invalidating the benchmark. No
synthetic timing threshold is used as a test pass/fail criterion.

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
