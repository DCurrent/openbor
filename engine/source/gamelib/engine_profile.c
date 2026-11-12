/*
 * OpenBOR - http://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 */
#include "engine_profile.h"
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PP_TEST
#include "timer.h"
#include "utils.h"
#endif

typedef struct ep_frame
{
    uint64_t id;
    uint64_t start_us;
    uint64_t last_us;
    uint64_t interval_us;
    uint64_t update_us;
    uint64_t phases_us[EP_PHASE_COUNT];
    uint64_t spans_us[EP_SPAN_COUNT];
    uint64_t counts[EP_COUNT_COUNT];
    uint64_t fallbacks[EP_FALLBACK_COUNT];
    uint64_t gpu_ns[EP_GPU_COUNT];
    uint64_t gpu_samples[EP_GPU_COUNT];
} ep_frame;

static struct
{
    int configured;
    int enabled;
    int have_scene;
    unsigned window;
    unsigned warmup;
    unsigned warm_left;
    unsigned accepted;
    const void *scene;
    const void *main_target;
    uint64_t next_id;
    uint64_t window_id;
    uint64_t cancelled;
    uint64_t gaps;
    uint64_t late_gpu;
    uint64_t clock_reads;
    ep_frame current;
    ep_frame pending;
    ep_frame frames[EP_MAX_WINDOW];
    ep_report last_report;
} profile;

static uint64_t (*profile_clock_hook)(void);
static void (*profile_log_hook)(const char *);
static const ep_gpu_backend *profile_gpu_backend;

static const char *const phase_names[EP_PHASE_COUNT] =
{
    "profile_entry_ms", "pre_wait_ms", "input_ms", "update_script_ms", "simulation_ms",
    "draw_setup_ms", "draw_script_ms", "main_queue_ms", "overlay_ms",
    "present_ms", "finish_ms"
};

static const char *const span_names[EP_SPAN_COUNT] =
{
    "main_sort_ms", "main_playback_ms", "capture_sort_ms", "capture_playback_ms",
    "gpu_geometry_ms", "gpu_state_ms", "gpu_submit_ms", "palette_lookup_ms", "upload_ms",
    "readback_ms", "dest_copy_ms", "software_ms", "present_setup_ms",
    "swap_ms", "limiter_ms"
};

static const char *const counter_names[EP_COUNT_COUNT] =
{
    "main_calls", "main_entries", "main_accepted", "capture_calls",
    "capture_entries", "capture_accepted", "native_draws", "native_quads",
    "native_batches", "native_clears", "gpu_state_changes", "gpu_state_reuses",
    "image_uploads", "image_upload_bytes",
    "palette_uploads", "palette_upload_bytes", "palette_cache_hits",
    "palette_cache_misses", "palette_cache_evictions", "table_uploads", "table_upload_bytes",
    "readbacks", "readback_bytes", "dest_copies", "dest_copy_bytes", "self_copies",
    "software_draws", "software_clears", "software_copies", "gpu_query_pairs",
    "gpu_query_dropped", "gpu_query_skipped"
};

static const char *const fallback_names[EP_FALLBACK_COUNT] =
{
    "no_backend", "target_format", "source_format", "image_size", "resource",
    "palette", "blend", "mask", "transform", "other"
};

static const char *const gpu_names[EP_GPU_COUNT] =
{
    "scene", "main_queue", "capture", "present"
};

/*
 * Damon V. Caskey
 * 2026-11-12
 * Read the monotonic engine clock through an injectable test boundary.
 * Counting reads exposes the amount of CPU timing instrumentation performed.
 */
static uint64_t ep_now(void)
{
    profile.clock_reads++;
    if(profile_clock_hook)
    {
        return profile_clock_hook();
    }
#ifndef PP_TEST
    return (uint64_t)timer_uticks();
#else
    /* Standalone renderer tests can link this collector without a timer. */
    return 0;
#endif
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Deliver complete diagnostic lines to the normal engine log or a test sink.
 */
static void ep_write(const char *text)
{
    if(profile_log_hook)
    {
        profile_log_hook(text);
        return;
    }
#ifndef PP_TEST
    /* Treat driver names and other inserted text as data, never a format. */
    writeToLogFile("%s", text);
#else
    (void)text;
#endif
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Read a bounded numeric environment option while keeping invalid values
 * on the documented default. Parsing happens once, outside draw hot paths.
 */
static unsigned ep_environment_unsigned(const char *name, unsigned fallback,
                                         unsigned minimum, unsigned maximum)
{
    const char *setting = getenv(name);
    char *end;
    unsigned long value;
    if(!setting || !*setting || *setting == '-')
    {
        return fallback;
    }
    errno = 0;
    value = strtoul(setting, &end, 10);
    if(errno || *end || value < minimum || value > maximum)
    {
        return fallback;
    }
    return (unsigned)value;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Initialize the diagnostic branch's defaults on first use. Explicit test
 * configuration bypasses environment parsing through the same state object.
 */
static void ep_initialize(void)
{
    const char *setting;
    if(profile.configured)
    {
        return;
    }
    setting = getenv("OPENBOR_PROFILE");
    engine_profile_configure(!setting || strcmp(setting, "0") != 0,
        ep_environment_unsigned("OPENBOR_PROFILE_FRAMES", EP_DEFAULT_WINDOW, 1, EP_MAX_WINDOW),
        ep_environment_unsigned("OPENBOR_PROFILE_WARMUP", EP_DEFAULT_WARMUP, 0, 100000));
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Replace external clock/log boundaries without changing collected samples.
 * Deterministic fixtures use these hooks to verify exact timing attribution.
 */
void engine_profile_set_hooks(uint64_t (*clock_us)(void), void (*log_line)(const char *))
{
    profile_clock_hook = clock_us;
    profile_log_hook = log_line;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Start a fresh collector with bounded storage and no allocation in frames.
 * Production normally reaches this once through environment initialization.
 */
void engine_profile_configure(int enabled, unsigned frames, unsigned warmup)
{
    uint64_t next_id = profile.next_id;
    /* Keep the clock, log sink and optional GPU provider independently owned
     * so tests and renderer context setup can install them in either order.
     * Frame identities also survive reconfiguration: outstanding GPU samples
     * and CPU scopes must never attach to a new frame with a recycled ID. */
    memset(&profile, 0, sizeof(profile));
    profile.next_id = next_id;
    profile.configured = 1;
    profile.enabled = enabled != 0;
    profile.window = frames ? frames : EP_DEFAULT_WINDOW;
    if(profile.window > EP_MAX_WINDOW)
    {
        profile.window = EP_MAX_WINDOW;
    }
    profile.warmup = warmup;
    profile.warm_left = warmup;
    engine_profile_log("kind=config enabled=%d frames=%u warmup=%u gap_ms=%u "
        "clock=monotonic_us gpu_time=elapsed_command_span",
        profile.enabled, profile.window, profile.warmup,
        (unsigned)(EP_MAX_GAP_US / 1000));
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Expose the inexpensive master gate to frame, queue and renderer hooks.
 */
int engine_profile_enabled(void)
{
    ep_initialize();
    return profile.enabled;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Identify accepted measurement work, excluding warmup, menus and reporting
 * barriers. Disabled scopes avoid all clock reads and GPU query creation.
 */
int engine_profile_recording(void)
{
    return engine_profile_enabled() && profile.current.id != 0;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Return the active measurement identity for asynchronous sample correlation.
 */
uint64_t engine_profile_frame_id(void)
{
    return engine_profile_recording() ? profile.current.id : 0;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Format bounded diagnostic text using the normal log destination. Logging
 * is performed at configuration changes and report barriers, outside draws.
 */
void engine_profile_log(const char *format, ...)
{
    char line[4096];
    size_t used;
    va_list args;
    if(!engine_profile_enabled() || !format)
    {
        return;
    }
    strcpy(line, "[ENGINE PROFILE] ");
    used = strlen(line);
    va_start(args, format);
    vsnprintf(line + used, sizeof(line) - used - 2, format, args);
    va_end(args);
    used = strlen(line);
    if(!used || line[used - 1] != '\n')
    {
        line[used++] = '\n';
        line[used] = '\0';
    }
    ep_write(line);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Order integer sample durations for an exact nearest-rank percentile.
 * Subtraction is avoided so a large duration cannot overflow the comparator.
 */
static int ep_compare_duration(const void *left, const void *right)
{
    uint64_t a = *(const uint64_t *)left;
    uint64_t b = *(const uint64_t *)right;
    return (a > b) - (a < b);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Emit one named series with explicit units and denominator. CPU span totals
 * are inclusive; count means and maxima describe the same accepted frames.
 */
static void ep_log_series(const char *kind, const char *const *names,
                           const uint64_t *values, unsigned count, double divisor)
{
    char fields[3072];
    size_t used = 0;
    unsigned i;
    fields[0] = '\0';
    for(i = 0; i < count; i++)
    {
        int written = snprintf(fields + used, sizeof(fields) - used,
                               " %s=%.6f", names[i], values[i] / divisor);
        if(written < 0 || (size_t)written >= sizeof(fields) - used)
        {
            break;
        }
        used += (size_t)written;
    }
    engine_profile_log("kind=%s window=%" PRIu64 "%s", kind,
                       profile.last_report.window_id, fields);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Build a report from paired frames and retain its raw snapshot for tests.
 * GPU results are aggregated only from their originating accepted frames.
 */
static void ep_publish(unsigned partial, const char *reason)
{
    ep_report *report = &profile.last_report;
    uint64_t durations[EP_MAX_WINDOW];
    unsigned i, j, rank;
    double frames;
    if(!profile.accepted)
    {
        return;
    }

    memset(report, 0, sizeof(*report));
    report->frames = profile.accepted;
    report->partial = partial;
    report->window_id = ++profile.window_id;
    report->first_frame_id = profile.frames[0].id;
    report->last_frame_id = profile.frames[profile.accepted - 1].id;
    report->interval_min_us = UINT64_MAX;
    for(i = 0; i < profile.accepted; i++)
    {
        const ep_frame *frame = &profile.frames[i];
        durations[i] = frame->interval_us;
        report->interval_us += frame->interval_us;
        report->update_us += frame->update_us;
        if(frame->interval_us < report->interval_min_us)
        {
            report->interval_min_us = frame->interval_us;
        }
        if(frame->interval_us > report->interval_max_us)
        {
            report->interval_max_us = frame->interval_us;
        }
        for(j = 0; j < EP_PHASE_COUNT; j++)
        {
            report->phases_us[j] += frame->phases_us[j];
        }
        for(j = 0; j < EP_SPAN_COUNT; j++)
        {
            report->spans_us[j] += frame->spans_us[j];
        }
        for(j = 0; j < EP_COUNT_COUNT; j++)
        {
            report->counts[j] += frame->counts[j];
            if(frame->counts[j] > report->count_max[j])
            {
                report->count_max[j] = frame->counts[j];
            }
        }
        for(j = 0; j < EP_FALLBACK_COUNT; j++)
        {
            report->fallbacks[j] += frame->fallbacks[j];
        }
        for(j = 0; j < EP_GPU_COUNT; j++)
        {
            report->gpu_ns[j] += frame->gpu_ns[j];
            report->gpu_samples[j] += frame->gpu_samples[j];
        }
    }
    qsort(durations, profile.accepted, sizeof(durations[0]), ep_compare_duration);
    rank = (profile.accepted * 95 + 99) / 100;
    report->interval_p95_us = durations[rank - 1];
    report->cancelled_frames = profile.cancelled;
    report->gap_frames = profile.gaps;
    report->late_gpu_results = profile.late_gpu;
    report->clock_reads = profile.clock_reads;
    frames = report->frames;

    /* Report on a skipped frame so file flushing and percentile calculation
     * cannot inflate the following measured entry-to-entry interval. */
    engine_profile_log("kind=frame window=%" PRIu64 " frames=%u partial=%u reason=%s "
        "first=%" PRIu64 " last=%" PRIu64 " frame_mean_ms=%.6f update_mean_ms=%.6f "
        "outside_update_ms=%.6f effective_fps=%.6f frame_min_ms=%.6f "
        "frame_p95_ms=%.6f frame_max_ms=%.6f",
        report->window_id, report->frames, partial, reason,
        report->first_frame_id, report->last_frame_id,
        report->interval_us / (frames * 1000.0), report->update_us / (frames * 1000.0),
        (report->interval_us - report->update_us) / (frames * 1000.0),
        report->interval_us ? frames * 1000000.0 / report->interval_us : 0.0,
        report->interval_min_us / 1000.0, report->interval_p95_us / 1000.0,
        report->interval_max_us / 1000.0);
    ep_log_series("phases_exclusive", phase_names, report->phases_us, EP_PHASE_COUNT, frames * 1000.0);
    ep_log_series("spans_inclusive", span_names, report->spans_us, EP_SPAN_COUNT, frames * 1000.0);
    ep_log_series("counts_mean", counter_names, report->counts, EP_COUNT_COUNT, frames);
    ep_log_series("counts_max", counter_names, report->count_max, EP_COUNT_COUNT, 1.0);
    ep_log_series("fallback_totals", fallback_names, report->fallbacks, EP_FALLBACK_COUNT, 1.0);
    for(i = 0; i < EP_GPU_COUNT; i++)
    {
        /* Nested GPU spans overlap and timestamp elapsed time can include
         * command starvation. Expose sample coverage and both denominators. */
        engine_profile_log("kind=gpu_elapsed window=%" PRIu64 " scope=%s samples=%" PRIu64
            " sample_mean_ms=%.6f per_frame_ms=%.6f",
            report->window_id, gpu_names[i], report->gpu_samples[i],
            report->gpu_samples[i] ? report->gpu_ns[i] / (report->gpu_samples[i] * 1000000.0) : 0.0,
            report->gpu_ns[i] / (frames * 1000000.0));
    }
    engine_profile_log("kind=diagnostics window=%" PRIu64 " cancelled=%" PRIu64
        " gaps=%" PRIu64 " late_gpu_results=%" PRIu64 " clock_reads=%" PRIu64
        " reporting_frame_skipped=1",
        report->window_id, report->cancelled_frames, report->gap_frames,
        report->late_gpu_results, report->clock_reads);

    profile.accepted = 0;
    profile.cancelled = 0;
    profile.gaps = 0;
    profile.late_gpu = 0;
    profile.clock_reads = 0;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Pair the previous completed update with its next entry timestamp. Menus,
 * scene changes, warmup and report callbacks form explicit sample barriers.
 */
uint64_t engine_profile_frame_begin(int eligible, const void *scene, const void *main_target)
{
    uint64_t now;
    if(!engine_profile_enabled())
    {
        return 0;
    }
    if(!eligible)
    {
        if(profile.have_scene || profile.current.id || profile.pending.id || profile.accepted)
        {
            engine_profile_reset("inactive");
        }
        return 0;
    }
    if(profile.current.id)
    {
        engine_profile_reset("nested_update");
    }
    if(!profile.have_scene || profile.scene != scene)
    {
        if(profile.have_scene)
        {
            engine_profile_reset("scene_changed");
        }
        profile.have_scene = 1;
        profile.scene = scene;
        profile.warm_left = profile.warmup;
    }

    now = ep_now();
    /* Consume ready timestamps before closing the previous CPU window. The
     * provider checks availability and returns immediately for pending work. */
    engine_profile_gpu_poll();
    if(profile.pending.id)
    {
        uint64_t interval = now >= profile.pending.start_us ? now - profile.pending.start_us : 0;
        if(now < profile.pending.start_us || interval > EP_MAX_GAP_US
           || interval < profile.pending.update_us)
        {
            profile.gaps++;
            engine_profile_log("kind=gap frame=%" PRIu64 " reason=%s interval_us=%" PRIu64,
                               profile.pending.id,
                               now < profile.pending.start_us ? "clock_backwards" :
                               (interval > EP_MAX_GAP_US ? "over_one_second" : "short_interval"),
                               interval);
            engine_profile_reset("clock_or_gap");
            return 0;
        }
        profile.pending.interval_us = interval;
        profile.frames[profile.accepted++] = profile.pending;
        memset(&profile.pending, 0, sizeof(profile.pending));
        if(profile.accepted == profile.window)
        {
            ep_publish(0, "window");
            return 0;
        }
    }
    if(profile.warm_left)
    {
        profile.warm_left--;
        return 0;
    }

    memset(&profile.current, 0, sizeof(profile.current));
    profile.current.id = ++profile.next_id;
    profile.current.start_us = now;
    profile.current.last_us = ep_now();
    if(profile.current.last_us < now)
    {
        engine_profile_frame_cancel(profile.current.id, "clock_backwards");
        return 0;
    }
    /* Ready-query polling and per-frame bookkeeping are profiler work. Keep
     * their entry cost explicit so pre_wait_ms describes the engine's pacing
     * phase even when vertical synchronization is disabled. Other timing-hook
     * overhead remains within the measured phases and can be checked by opt-out. */
    profile.current.phases_us[EP_PHASE_PROFILE_ENTRY] = profile.current.last_us - now;
    profile.main_target = main_target;
    return profile.current.id;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Attribute time since the previous checkpoint to one exclusive update stage.
 * Identity guards keep nested menu calls from modifying a suspended frame.
 */
void engine_profile_frame_mark(uint64_t frame_id, ep_phase completed_phase)
{
    uint64_t now;
    if(!engine_profile_recording() || frame_id != profile.current.id
       || (unsigned)completed_phase >= EP_PHASE_COUNT)
    {
        return;
    }
    now = ep_now();
    if(now < profile.current.last_us)
    {
        engine_profile_frame_cancel(frame_id, "clock_backwards");
        return;
    }
    profile.current.phases_us[completed_phase] += now - profile.current.last_us;
    profile.current.last_us = now;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Complete CPU accounting but defer acceptance until the next update supplies
 * the matching full frame interval. Late GPU samples can still join this frame.
 */
void engine_profile_frame_end(uint64_t frame_id)
{
    if(!engine_profile_recording() || frame_id != profile.current.id)
    {
        return;
    }
    engine_profile_frame_mark(frame_id, EP_PHASE_FINISH);
    if(profile.current.id != frame_id)
    {
        return;
    }
    profile.current.update_us = profile.current.last_us - profile.current.start_us;
    profile.pending = profile.current;
    memset(&profile.current, 0, sizeof(profile.current));
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Drop an interrupted update before pause menus or level transitions enter
 * nested event loops. Already accepted samples remain available for reporting.
 */
void engine_profile_frame_cancel(uint64_t frame_id, const char *reason)
{
    if(!engine_profile_recording() || frame_id != profile.current.id)
    {
        return;
    }
    profile.cancelled++;
    memset(&profile.current, 0, sizeof(profile.current));
    engine_profile_log("kind=cancel frame=%" PRIu64 " reason=%s", frame_id,
                       reason ? reason : "unspecified");
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Finish a partial window at a configuration or scene boundary. Unpaired
 * updates are discarded because their complete frame duration is unknown.
 */
void engine_profile_reset(const char *reason)
{
    if(!engine_profile_enabled())
    {
        return;
    }
    engine_profile_gpu_poll();
    if(profile.current.id)
    {
        profile.cancelled++;
    }
    ep_publish(1, reason ? reason : "reset");
    memset(&profile.current, 0, sizeof(profile.current));
    memset(&profile.pending, 0, sizeof(profile.pending));
    profile.have_scene = 0;
    profile.scene = NULL;
    profile.main_target = NULL;
    profile.warm_left = profile.warmup;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Preserve any accepted partial window during normal engine shutdown.
 */
void engine_profile_shutdown(void)
{
    engine_profile_reset("shutdown");
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Classify queue playback by destination identity without coupling gamelib
 * tests and renderers to the engine's global vscreen variable.
 */
int engine_profile_is_main_target(const void *target)
{
    return engine_profile_recording() && target == profile.main_target;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Start an inclusive CPU span only while a frame is being measured.
 */
ep_scope engine_profile_span_begin(void)
{
    ep_scope scope = {0, 0};
    if(engine_profile_recording())
    {
        scope.frame_id = profile.current.id;
        scope.start_us = ep_now();
    }
    return scope;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Accumulate nested stage costs without altering the exclusive phase clock.
 * A cancelled scope cannot leak its duration into a later update.
 */
void engine_profile_span_end(ep_scope scope, ep_span_kind kind)
{
    uint64_t now;
    if(!engine_profile_recording() || scope.frame_id != profile.current.id
       || (unsigned)kind >= EP_SPAN_COUNT)
    {
        return;
    }
    now = ep_now();
    if(now >= scope.start_us)
    {
        profile.current.spans_us[kind] += now - scope.start_us;
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Count rendering work and bytes in the originating measured frame. Warmup
 * and uninstrumented menu activity do not contaminate gameplay averages.
 */
void engine_profile_count(ep_counter counter, uint64_t amount)
{
    if(engine_profile_recording() && (unsigned)counter < EP_COUNT_COUNT)
    {
        profile.current.counts[counter] += amount;
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Distinguish an absent native backend from operation-specific declines.
 */
void engine_profile_fallback(ep_fallback reason)
{
    if(engine_profile_recording() && (unsigned)reason < EP_FALLBACK_COUNT)
    {
        profile.current.fallbacks[reason]++;
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Expose the most recently emitted raw report for deterministic regression
 * checks. Reading this snapshot performs no clock or graphics operations.
 */
const ep_report *engine_profile_last_report(void)
{
    return &profile.last_report;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Register context-owned timestamp callbacks independently of drawing support.
 * Providers remove themselves before their graphics context is destroyed.
 */
void engine_profile_set_gpu_backend(const ep_gpu_backend *backend)
{
    profile_gpu_backend = backend;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Begin an optional GPU timeline scope carrying the active CPU frame identity.
 * Unsupported timers remain visible as skipped samples while CPU profiling runs.
 */
uint64_t engine_profile_gpu_begin(ep_gpu_kind kind)
{
    if(!engine_profile_recording() || (unsigned)kind >= EP_GPU_COUNT)
    {
        return 0;
    }
    if(!profile_gpu_backend || !profile_gpu_backend->begin)
    {
        engine_profile_count(EP_COUNT_GPU_QUERY_SKIPPED, 1);
        return 0;
    }
    return profile_gpu_backend->begin(profile.current.id, kind);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Close an issued timestamp pair even if its CPU frame was subsequently
 * cancelled. The provider rejects stale tokens from retired contexts.
 */
void engine_profile_gpu_end(uint64_t handle)
{
    if(handle && profile_gpu_backend && profile_gpu_backend->end)
    {
        profile_gpu_backend->end(handle);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Collect only available GPU results. Blocking, flushing or forcing GPU
 * completion would change the workload that this diagnostic measures.
 */
void engine_profile_gpu_poll(void)
{
    if(engine_profile_enabled() && profile_gpu_backend && profile_gpu_backend->poll)
    {
        profile_gpu_backend->poll();
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Attach asynchronous GPU elapsed time to its original frame. Already reported
 * or discarded frame identities are counted as late instead of being reassigned.
 */
void engine_profile_gpu_result(uint64_t frame_id, ep_gpu_kind kind, uint64_t elapsed_ns)
{
    ep_frame *frame = NULL;
    unsigned i;
    if(!engine_profile_enabled() || !frame_id || (unsigned)kind >= EP_GPU_COUNT)
    {
        return;
    }
    if(profile.current.id == frame_id)
    {
        frame = &profile.current;
    }
    else if(profile.pending.id == frame_id)
    {
        frame = &profile.pending;
    }
    else
    {
        for(i = 0; i < profile.accepted; i++)
        {
            if(profile.frames[i].id == frame_id)
            {
                frame = &profile.frames[i];
                break;
            }
        }
    }
    if(frame)
    {
        frame->gpu_ns[kind] += elapsed_ns;
        frame->gpu_samples[kind]++;
    }
    else
    {
        profile.late_gpu++;
    }
}
