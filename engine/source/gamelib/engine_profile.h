/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 */
#ifndef OPENBOR_ENGINE_PROFILE_H
#define OPENBOR_ENGINE_PROFILE_H

#include <stdint.h>

#define EP_DEFAULT_WINDOW 300
#define EP_DEFAULT_WARMUP 60
#define EP_MAX_WINDOW 1000
#define EP_MAX_GAP_US UINT64_C(1000000)

/* These checkpoints partition update() in execution order. Nested spans
 * below describe inclusive costs within those checkpoints and must be read
 * separately instead of being added to the frame-stage totals. */
typedef enum ep_phase
{
    EP_PHASE_PROFILE_ENTRY,
    EP_PHASE_PRE_WAIT,
    EP_PHASE_INPUT,
    EP_PHASE_UPDATE_SCRIPT,
    EP_PHASE_SIMULATION,
    EP_PHASE_DRAW_SETUP,
    EP_PHASE_DRAW_SCRIPT,
    EP_PHASE_MAIN_QUEUE,
    EP_PHASE_OVERLAY,
    EP_PHASE_PRESENT,
    EP_PHASE_FINISH,
    EP_PHASE_COUNT
} ep_phase;

typedef enum ep_span_kind
{
    EP_SPAN_MAIN_SORT,
    EP_SPAN_MAIN_PLAYBACK,
    EP_SPAN_CAPTURE_SORT,
    EP_SPAN_CAPTURE_PLAYBACK,
    EP_SPAN_GPU_GEOMETRY,
    EP_SPAN_GPU_STATE,
    EP_SPAN_GPU_SUBMIT,
    /* Content lookup and hit-side LRU work end before any upload. */
    EP_SPAN_PALETTE_LOOKUP,
    EP_SPAN_UPLOAD,
    EP_SPAN_READBACK,
    EP_SPAN_DEST_COPY,
    EP_SPAN_SOFTWARE,
    EP_SPAN_PRESENT_SETUP,
    EP_SPAN_SWAP,
    EP_SPAN_LIMITER,
    EP_SPAN_COUNT
} ep_span_kind;

typedef enum ep_counter
{
    EP_COUNT_MAIN_QUEUE_CALLS,
    EP_COUNT_MAIN_ENTRIES,
    EP_COUNT_MAIN_ACCEPTED,
    EP_COUNT_CAPTURE_QUEUE_CALLS,
    EP_COUNT_CAPTURE_ENTRIES,
    EP_COUNT_CAPTURE_ACCEPTED,
    EP_COUNT_NATIVE_DRAWS,
    EP_COUNT_NATIVE_QUADS,
    EP_COUNT_NATIVE_BATCHES,
    EP_COUNT_NATIVE_CLEARS,
    EP_COUNT_IMAGE_UPLOADS,
    EP_COUNT_IMAGE_UPLOAD_BYTES,
    EP_COUNT_PALETTE_UPLOADS,
    EP_COUNT_PALETTE_UPLOAD_BYTES,
    /* Residency events remain distinct from successful GPU updates. */
    EP_COUNT_PALETTE_CACHE_HITS,
    EP_COUNT_PALETTE_CACHE_MISSES,
    EP_COUNT_PALETTE_CACHE_EVICTIONS,
    EP_COUNT_TABLE_UPLOADS,
    EP_COUNT_TABLE_UPLOAD_BYTES,
    EP_COUNT_READBACKS,
    EP_COUNT_READBACK_BYTES,
    EP_COUNT_DEST_COPIES,
    EP_COUNT_DEST_COPY_BYTES,
    EP_COUNT_SELF_COPIES,
    EP_COUNT_SOFTWARE_DRAWS,
    EP_COUNT_SOFTWARE_CLEARS,
    EP_COUNT_SOFTWARE_COPIES,
    EP_COUNT_GPU_QUERY_PAIRS,
    EP_COUNT_GPU_QUERY_DROPPED,
    EP_COUNT_GPU_QUERY_SKIPPED,
    EP_COUNT_COUNT
} ep_counter;

/* A declined native operation records one reason at its return boundary.
 * Software-only backend selection is counted independently from a native
 * renderer declining a particular source, target or drawing method. */
typedef enum ep_fallback
{
    EP_FALLBACK_NO_BACKEND,
    EP_FALLBACK_TARGET_FORMAT,
    EP_FALLBACK_SOURCE_FORMAT,
    EP_FALLBACK_IMAGE_SIZE,
    EP_FALLBACK_RESOURCE,
    EP_FALLBACK_PALETTE,
    EP_FALLBACK_BLEND,
    EP_FALLBACK_MASK,
    EP_FALLBACK_TRANSFORM,
    EP_FALLBACK_OTHER,
    EP_FALLBACK_COUNT
} ep_fallback;

typedef enum ep_gpu_kind
{
    EP_GPU_SCENE,
    EP_GPU_MAIN_QUEUE,
    EP_GPU_CAPTURE,
    EP_GPU_PRESENT,
    EP_GPU_COUNT
} ep_gpu_kind;

/* Scope identity prevents a cancelled or nested frame from charging an
 * older operation to a later frame. Disabled scopes contain frame_id zero. */
typedef struct ep_scope
{
    uint64_t frame_id;
    uint64_t start_us;
} ep_scope;

typedef struct ep_gpu_backend
{
    uint64_t (*begin)(uint64_t frame_id, ep_gpu_kind kind);
    void (*end)(uint64_t handle);
    void (*poll)(void);
} ep_gpu_backend;

/* Report values retain integer microseconds/bytes and raw counts. The log
 * converts these totals to per-frame means; GPU elapsed samples retain their
 * own denominators because availability and capture counts can differ. */
typedef struct ep_report
{
    unsigned frames;
    unsigned partial;
    uint64_t window_id;
    uint64_t first_frame_id;
    uint64_t last_frame_id;
    uint64_t interval_us;
    uint64_t update_us;
    uint64_t interval_min_us;
    uint64_t interval_max_us;
    uint64_t interval_p95_us;
    uint64_t phases_us[EP_PHASE_COUNT];
    uint64_t spans_us[EP_SPAN_COUNT];
    uint64_t counts[EP_COUNT_COUNT];
    uint64_t count_max[EP_COUNT_COUNT];
    uint64_t fallbacks[EP_FALLBACK_COUNT];
    uint64_t gpu_ns[EP_GPU_COUNT];
    uint64_t gpu_samples[EP_GPU_COUNT];
    uint64_t cancelled_frames;
    uint64_t gap_frames;
    uint64_t late_gpu_results;
    uint64_t clock_reads;
} ep_report;

/* Defaults use timer_uticks() and writeToLogFile() in production. Tests can
 * inject a deterministic clock and capture complete log lines. Configuration
 * is sampled once from OPENBOR_PROFILE, OPENBOR_PROFILE_FRAMES and
 * OPENBOR_PROFILE_WARMUP; profiling defaults on in this diagnostic branch. */
void engine_profile_set_hooks(uint64_t (*clock_us)(void), void (*log_line)(const char *));
void engine_profile_configure(int enabled, unsigned frames, unsigned warmup);
int engine_profile_enabled(void);
int engine_profile_recording(void);
uint64_t engine_profile_frame_id(void);
uint64_t engine_profile_frame_begin(int eligible, const void *scene, const void *main_target);
void engine_profile_frame_mark(uint64_t frame_id, ep_phase completed_phase);
void engine_profile_frame_end(uint64_t frame_id);
void engine_profile_frame_cancel(uint64_t frame_id, const char *reason);
void engine_profile_reset(const char *reason);
void engine_profile_shutdown(void);
int engine_profile_is_main_target(const void *target);

ep_scope engine_profile_span_begin(void);
void engine_profile_span_end(ep_scope scope, ep_span_kind kind);
void engine_profile_count(ep_counter counter, uint64_t amount);
void engine_profile_fallback(ep_fallback reason);
void engine_profile_log(const char *format, ...);
const ep_report *engine_profile_last_report(void);

/* Timestamp scopes are backend optional. Polling consumes only already-ready
 * results. Results retain their originating frame ID and are discarded with
 * a late-result count once that frame's window has already been reported. */
void engine_profile_set_gpu_backend(const ep_gpu_backend *backend);
uint64_t engine_profile_gpu_begin(ep_gpu_kind kind);
void engine_profile_gpu_end(uint64_t handle);
void engine_profile_gpu_poll(void);
void engine_profile_gpu_result(uint64_t frame_id, ep_gpu_kind kind, uint64_t elapsed_ns);

#endif
