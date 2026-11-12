/*
 * Deterministic engine-profiler accounting tests.
 *
 * Run with tests/run_gpu_tests.sh. These fixtures inject time and completed
 * timestamp-query results, so no graphics context or wall-clock delay affects
 * the expected frame, phase, counter, or percentile values.
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine_profile.h"

typedef struct test_query
{
    uint64_t handle;
    uint64_t frame_id;
    uint64_t elapsed_ns;
    ep_gpu_kind kind;
    int ended;
    int ready;
    int delivered;
} test_query;

static uint64_t test_time_us;
static uint64_t test_clock_reads;
static unsigned test_log_lines;
static unsigned test_query_count;
static unsigned test_query_ends;
static unsigned test_query_polls;
static uint64_t test_query_poll_delay_us;
static char test_span_log[4096];
static char test_count_mean_log[4096];
static char test_count_max_log[4096];
static test_query test_queries[32];
static int test_scene;
static int test_main_target;
static int test_capture_target;

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Expose explicitly advanced microseconds and count timer-hook calls. Disabled
 * profiling must avoid those calls rather than merely discard their results.
 */
static uint64_t test_clock(void)
{
    ++test_clock_reads;
    return test_time_us;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Validate the engine-log prefix and retain the named series used to check
 * field labels and per-frame formatting. Exact accounting assertions use the
 * report snapshot instead of depending only on rounded log text.
 */
static void test_log(const char *line)
{
    assert(line);
    assert(strncmp(line, "[ENGINE PROFILE]", 16) == 0);
    if(strstr(line, "kind=spans_inclusive "))
    {
        snprintf(test_span_log, sizeof(test_span_log), "%s", line);
    }
    else if(strstr(line, "kind=counts_mean "))
    {
        snprintf(test_count_mean_log, sizeof(test_count_mean_log), "%s", line);
    }
    else if(strstr(line, "kind=counts_max "))
    {
        snprintf(test_count_max_log, sizeof(test_count_max_log), "%s", line);
    }
    ++test_log_lines;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Record query identity without completing it. Fixtures decide when each
 * result becomes ready, independently of the order in which scopes end.
 */
static uint64_t test_query_begin(uint64_t frame_id, ep_gpu_kind kind)
{
    test_query *query;

    assert(test_query_count < sizeof(test_queries) / sizeof(test_queries[0]));
    query = &test_queries[test_query_count++];
    query->handle = test_query_count;
    query->frame_id = frame_id;
    query->kind = kind;
    return query->handle;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Close a fake scope without making its timestamps available. This separates
 * submission from completion in the same way an asynchronous GPU does.
 */
static void test_query_end(uint64_t handle)
{
    assert(handle > 0 && handle <= test_query_count);
    test_queries[handle - 1].ended = 1;
    ++test_query_ends;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Deliver only already-ready results, preserving their original frame IDs.
 * Open scopes and unfinished queries remain pending across as many polls as
 * the fixture requires; polling never advances the injected clock.
 */
static void test_query_poll(void)
{
    unsigned index;

    ++test_query_polls;
    test_time_us += test_query_poll_delay_us;
    for(index = 0; index < test_query_count; ++index)
    {
        test_query *query = &test_queries[index];

        if(query->ended && query->ready && !query->delivered)
        {
            engine_profile_gpu_result(query->frame_id, query->kind, query->elapsed_ns);
            query->delivered = 1;
        }
    }
}

static const ep_gpu_backend test_gpu_backend = {
    test_query_begin,
    test_query_end,
    test_query_poll
};

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Start each fixture with independent counters and report storage. Detaching
 * the fake backend first prevents cleanup of a prior fixture from submitting
 * results into the next one's frame namespace.
 */
static void test_configure(int enabled, unsigned frames, unsigned warmup)
{
    engine_profile_set_gpu_backend(NULL);
    engine_profile_configure(0, 1, 0);
    test_time_us = 0;
    test_clock_reads = 0;
    test_log_lines = 0;
    test_query_count = 0;
    test_query_ends = 0;
    test_query_polls = 0;
    test_query_poll_delay_us = 0;
    test_span_log[0] = '\0';
    test_count_mean_log[0] = '\0';
    test_count_max_log[0] = '\0';
    memset(test_queries, 0, sizeof(test_queries));
    engine_profile_set_hooks(test_clock, test_log);
    engine_profile_configure(enabled, frames, warmup);
    test_clock_reads = 0;
    test_log_lines = 0;
    assert(engine_profile_last_report()->frames == 0);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Enter one eligible update at an explicit time. Both identity pointers stay
 * stable unless a particular test deliberately changes scene or target.
 */
static uint64_t test_begin(uint64_t time_us)
{
    test_time_us = time_us;
    return engine_profile_frame_begin(1, &test_scene, &test_main_target);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Finish the update body without supplying its interval. That interval can
 * only be established by the next eligible update entry.
 */
static void test_end(uint64_t frame_id, uint64_t time_us)
{
    test_time_us = time_us;
    engine_profile_frame_end(frame_id);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Make a previously submitted fake query observable on the next poll.
 */
static void test_query_ready(uint64_t handle, uint64_t elapsed_ns)
{
    assert(handle > 0 && handle <= test_query_count);
    test_queries[handle - 1].elapsed_ns = elapsed_ns;
    test_queries[handle - 1].ready = 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Verify the complete disabled fast path, including nested scopes, counters,
 * queue identity, timestamp providers and invalidation calls.
 */
static void test_disabled(void)
{
    ep_scope scope;

    test_configure(0, 3, 0);
    engine_profile_set_gpu_backend(&test_gpu_backend);
    assert(!engine_profile_enabled());
    assert(test_begin(1000) == 0);
    assert(!engine_profile_recording());
    assert(engine_profile_frame_id() == 0);
    scope = engine_profile_span_begin();
    assert(scope.frame_id == 0);
    engine_profile_span_end(scope, EP_SPAN_CAPTURE_PLAYBACK);
    engine_profile_span_end(scope, EP_SPAN_PALETTE_LOOKUP);
    engine_profile_frame_mark(0, EP_PHASE_INPUT);
    engine_profile_count(EP_COUNT_NATIVE_DRAWS, 900);
    engine_profile_count(EP_COUNT_PALETTE_CACHE_HITS, 10);
    engine_profile_count(EP_COUNT_PALETTE_CACHE_MISSES, 5);
    engine_profile_count(EP_COUNT_PALETTE_CACHE_EVICTIONS, 2);
    engine_profile_fallback(EP_FALLBACK_SOURCE_FORMAT);
    assert(engine_profile_gpu_begin(EP_GPU_SCENE) == 0);
    engine_profile_gpu_end(0);
    engine_profile_gpu_poll();
    engine_profile_frame_end(0);
    engine_profile_frame_cancel(0, "disabled fixture");
    engine_profile_reset("disabled fixture");
    engine_profile_shutdown();
    assert(test_clock_reads == 0);
    assert(test_query_count == 0);
    assert(test_query_ends == 0);
    assert(test_query_polls == 0);
    assert(test_log_lines == 0);
    assert(engine_profile_last_report()->frames == 0);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Pair three completed bodies with their following entries and verify exact
 * totals. Counter deltas belong to each body, never the entry that accepts it.
 */
static void test_pending_window_and_counters(void)
{
    const ep_report *report;
    uint64_t first, second, third;

    test_configure(1, 3, 0);
    engine_profile_count(EP_COUNT_NATIVE_DRAWS, 9999);
    first = test_begin(1000);
    assert(first);
    assert(engine_profile_is_main_target(&test_main_target));
    assert(!engine_profile_is_main_target(&test_capture_target));
    engine_profile_count(EP_COUNT_NATIVE_DRAWS, 2);
    engine_profile_count(EP_COUNT_IMAGE_UPLOAD_BYTES, 100);
    engine_profile_fallback(EP_FALLBACK_TARGET_FORMAT);
    test_end(first, 2000);
    assert(engine_profile_last_report()->frames == 0);

    /* Accepting a previous body starts a separate current frame. No report
     * exists until the configured number of complete interval pairs exists. */
    second = test_begin(11000);
    assert(second > first);
    engine_profile_count(EP_COUNT_NATIVE_DRAWS, 3);
    engine_profile_count(EP_COUNT_NATIVE_DRAWS, 2);
    engine_profile_count(EP_COUNT_IMAGE_UPLOAD_BYTES, 200);
    test_end(second, 13000);
    third = test_begin(31000);
    assert(third > second);
    engine_profile_count(EP_COUNT_NATIVE_DRAWS, 1);
    engine_profile_count(EP_COUNT_IMAGE_UPLOAD_BYTES, 300);
    test_end(third, 34000);
    assert(engine_profile_last_report()->frames == 0);

    /* This entry reports the third pair and becomes an unmeasured barrier.
     * Report formatting cannot leak into the next sampled frame interval. */
    assert(test_begin(61000) == 0);
    assert(!engine_profile_recording());
    report = engine_profile_last_report();
    assert(report->frames == 3 && !report->partial);
    assert(report->first_frame_id == first && report->last_frame_id == third);
    assert(report->interval_us == 60000);
    assert(report->update_us == 6000);
    assert(report->interval_min_us == 10000);
    assert(report->interval_max_us == 30000);
    assert(report->interval_p95_us == 30000);
    assert(report->phases_us[EP_PHASE_FINISH] == 6000);
    assert(report->counts[EP_COUNT_NATIVE_DRAWS] == 8);
    assert(report->count_max[EP_COUNT_NATIVE_DRAWS] == 5);
    assert(report->counts[EP_COUNT_IMAGE_UPLOAD_BYTES] == 600);
    assert(report->count_max[EP_COUNT_IMAGE_UPLOAD_BYTES] == 300);
    assert(report->fallbacks[EP_FALLBACK_TARGET_FORMAT] == 1);
    assert(test_log_lines > 0);

    /* A long delay after logging is deliberately outside any sample. There
     * is no pending frame whose interval could include that delay. */
    first = test_begin(UINT64_C(5000000));
    assert(first);
    test_end(first, UINT64_C(5000100));
    second = test_begin(UINT64_C(5010000));
    assert(second);
    engine_profile_frame_cancel(second, "barrier fixture complete");
    engine_profile_reset("publish accepted barrier fixture");
    report = engine_profile_last_report();
    assert(report->frames == 1 && report->partial);
    assert(report->interval_us == 10000);
    assert(report->update_us == 100);
    assert(report->counts[EP_COUNT_NATIVE_DRAWS] == 0);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Verify initial warmup and the report barrier have distinct meanings.
 * Warmup restarts after invalidation, while ordinary reports skip one entry.
 */
static void test_warmup_and_restart(void)
{
    uint64_t first, second;

    test_configure(1, 2, 2);
    assert(test_begin(10000) == 0);
    assert(test_begin(20000) == 0);
    first = test_begin(30000);
    assert(first);
    test_end(first, 31000);
    second = test_begin(40000);
    assert(second);
    test_end(second, 41000);
    assert(test_begin(50000) == 0);
    assert(engine_profile_last_report()->frames == 2);
    assert(engine_profile_last_report()->interval_us == 20000);
    first = test_begin(60000);
    assert(first);
    engine_profile_frame_cancel(first, "discard incomplete frame");
    assert(test_begin(70000));
    engine_profile_reset("restart warmup");
    assert(test_begin(80000) == 0);
    assert(test_begin(90000) == 0);
    assert(test_begin(100000));
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Check the exclusive phase partition separately from inclusive nested work.
 * Even nested scopes of the same kind retain their full durations, so adding
 * spans to the phase sum would double-count time by construction.
 */
static void test_exclusive_phases_and_inclusive_spans(void)
{
    const ep_report *report;
    ep_scope outer, inner, geometry;
    uint64_t frame, phase_sum = 0;
    unsigned index;

    test_configure(1, 1, 0);
    frame = test_begin(1000);
    test_time_us = 1100;
    outer = engine_profile_span_begin();
    test_time_us = 1200;
    inner = engine_profile_span_begin();
    test_time_us = 1700;
    engine_profile_span_end(inner, EP_SPAN_CAPTURE_PLAYBACK);
    test_time_us = 1900;
    engine_profile_span_end(outer, EP_SPAN_CAPTURE_PLAYBACK);
    test_time_us = 2000;
    engine_profile_frame_mark(frame, EP_PHASE_PRE_WAIT);
    test_time_us = 2100;
    geometry = engine_profile_span_begin();
    test_time_us = 2600;
    engine_profile_span_end(geometry, EP_SPAN_GPU_GEOMETRY);
    test_time_us = 3000;
    engine_profile_frame_mark(frame, EP_PHASE_INPUT);
    test_end(frame, 4000);
    assert(test_begin(5000) == 0);
    report = engine_profile_last_report();
    for(index = 0; index < EP_PHASE_COUNT; ++index)
    {
        phase_sum += report->phases_us[index];
    }
    assert(phase_sum == report->update_us && phase_sum == 3000);
    assert(report->interval_us == 4000);
    assert(report->phases_us[EP_PHASE_PRE_WAIT] == 1000);
    assert(report->phases_us[EP_PHASE_INPUT] == 1000);
    assert(report->phases_us[EP_PHASE_FINISH] == 1000);
    assert(report->spans_us[EP_SPAN_CAPTURE_PLAYBACK] == 1300);
    assert(report->spans_us[EP_SPAN_GPU_GEOMETRY] == 500);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Keep resident-palette lookup work separate from actual upload work. Distinct
 * frame values check accumulated counts, maxima and formatted means, including
 * the unchanged successful-upload counters used for baseline comparisons.
 */
static void test_palette_cache_accounting(void)
{
    const ep_report *report;
    ep_scope lookup, upload;
    uint64_t first, second;

    test_configure(1, 2, 0);
    engine_profile_count(EP_COUNT_PALETTE_CACHE_HITS, 999);
    first = test_begin(1000);
    assert(first);

    /* The renderer finishes content lookup before starting any upload.
     * These sibling spans must not charge the same time to both fields;
     * post-upload cache publication remains only in the enclosing phase. */
    test_time_us = 1100;
    lookup = engine_profile_span_begin();
    test_time_us = 1150;
    engine_profile_span_end(lookup, EP_SPAN_PALETTE_LOOKUP);
    upload = engine_profile_span_begin();
    test_time_us = 1350;
    engine_profile_span_end(upload, EP_SPAN_UPLOAD);
    engine_profile_count(EP_COUNT_PALETTE_CACHE_HITS, 4);
    engine_profile_count(EP_COUNT_PALETTE_CACHE_MISSES, 2);
    engine_profile_count(EP_COUNT_PALETTE_CACHE_EVICTIONS, 1);
    engine_profile_count(EP_COUNT_PALETTE_UPLOADS, 2);
    engine_profile_count(EP_COUNT_PALETTE_UPLOAD_BYTES, 2048);
    test_time_us = 1400;
    engine_profile_frame_mark(first, EP_PHASE_MAIN_QUEUE);
    test_end(first, 1500);

    /* Vary the second frame so an accidental field-order shift or a sum
     * incorrectly substituted for a maximum cannot pass the assertions. */
    second = test_begin(11000);
    assert(second > first);
    test_time_us = 11100;
    lookup = engine_profile_span_begin();
    test_time_us = 11175;
    engine_profile_span_end(lookup, EP_SPAN_PALETTE_LOOKUP);
    upload = engine_profile_span_begin();
    test_time_us = 11575;
    engine_profile_span_end(upload, EP_SPAN_UPLOAD);
    engine_profile_count(EP_COUNT_PALETTE_CACHE_HITS, 8);
    engine_profile_count(EP_COUNT_PALETTE_CACHE_MISSES, 1);
    engine_profile_count(EP_COUNT_PALETTE_UPLOADS, 1);
    engine_profile_count(EP_COUNT_PALETTE_UPLOAD_BYTES, 1024);
    test_time_us = 11600;
    engine_profile_frame_mark(second, EP_PHASE_MAIN_QUEUE);
    test_end(second, 11700);
    assert(test_begin(21000) == 0);

    report = engine_profile_last_report();
    assert(report->frames == 2);
    assert(report->interval_us == 20000 && report->update_us == 1200);
    assert(report->phases_us[EP_PHASE_MAIN_QUEUE] == 1000);
    assert(report->phases_us[EP_PHASE_FINISH] == 200);
    assert(report->spans_us[EP_SPAN_PALETTE_LOOKUP] == 125);
    assert(report->spans_us[EP_SPAN_UPLOAD] == 600);
    assert(report->counts[EP_COUNT_PALETTE_CACHE_HITS] == 12);
    assert(report->counts[EP_COUNT_PALETTE_CACHE_MISSES] == 3);
    assert(report->counts[EP_COUNT_PALETTE_CACHE_EVICTIONS] == 1);
    assert(report->count_max[EP_COUNT_PALETTE_CACHE_HITS] == 8);
    assert(report->count_max[EP_COUNT_PALETTE_CACHE_MISSES] == 2);
    assert(report->count_max[EP_COUNT_PALETTE_CACHE_EVICTIONS] == 1);
    assert(report->counts[EP_COUNT_PALETTE_UPLOADS] == 3);
    assert(report->counts[EP_COUNT_PALETTE_UPLOAD_BYTES] == 3072);
    assert(report->count_max[EP_COUNT_PALETTE_UPLOADS] == 2);
    assert(report->count_max[EP_COUNT_PALETTE_UPLOAD_BYTES] == 2048);

    /* Verify the public log labels as well as raw array indices. Existing
     * upload fields retain their meaning when the new fields are inserted. */
    assert(strstr(test_span_log, " palette_lookup_ms=0.062500"));
    assert(strstr(test_span_log, " upload_ms=0.300000"));
    assert(strstr(test_count_mean_log, " palette_cache_hits=6.000000"));
    assert(strstr(test_count_mean_log, " palette_cache_misses=1.500000"));
    assert(strstr(test_count_mean_log, " palette_cache_evictions=0.500000"));
    assert(strstr(test_count_mean_log, " palette_uploads=1.500000"));
    assert(strstr(test_count_mean_log, " palette_upload_bytes=1536.000000"));
    assert(strstr(test_count_max_log, " palette_cache_hits=8.000000"));
    assert(strstr(test_count_max_log, " palette_cache_misses=2.000000"));
    assert(strstr(test_count_max_log, " palette_cache_evictions=1.000000"));
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Attribute timestamp polling and entry bookkeeping to the profiler prologue,
 * keeping their CPU cost separate from the retained pre-input pacing phase.
 */
static void test_profile_entry_phase(void)
{
    const ep_report *report;
    uint64_t frame;

    test_configure(1, 1, 0);
    test_query_poll_delay_us = 250;
    engine_profile_set_gpu_backend(&test_gpu_backend);
    frame = test_begin(1000);
    assert(frame && test_time_us == 1250);
    test_time_us = 1750;
    engine_profile_frame_mark(frame, EP_PHASE_PRE_WAIT);
    test_end(frame, 2000);
    assert(test_begin(11000) == 0);
    report = engine_profile_last_report();
    assert(report->interval_us == 10000 && report->update_us == 1000);
    assert(report->phases_us[EP_PHASE_PROFILE_ENTRY] == 250);
    assert(report->phases_us[EP_PHASE_PRE_WAIT] == 500);
    assert(report->phases_us[EP_PHASE_FINISH] == 250);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Exercise cancellation, pause and context invalidation. Retained partial
 * windows contain only previously paired frames; cancelled scopes and pending
 * bodies cannot be relabeled as work belonging to a later update.
 */
static void test_cancel_pause_and_reset(void)
{
    const ep_report *report;
    ep_scope stale;
    uint64_t first, cancelled, current;

    test_configure(1, 3, 0);
    first = test_begin(1000);
    engine_profile_count(EP_COUNT_CAPTURE_ENTRIES, 4);
    test_end(first, 2000);
    cancelled = test_begin(11000);
    test_time_us = 12000;
    stale = engine_profile_span_begin();
    engine_profile_count(EP_COUNT_CAPTURE_ENTRIES, 900);
    engine_profile_frame_cancel(cancelled, "fixture pause");
    assert(!engine_profile_recording());
    assert(engine_profile_last_report()->frames == 0);
    engine_profile_reset("publish cancelled partial window");
    report = engine_profile_last_report();
    assert(report->frames == 1 && report->partial);
    assert(report->first_frame_id == first && report->last_frame_id == first);
    assert(report->counts[EP_COUNT_CAPTURE_ENTRIES] == 4);
    assert(report->spans_us[EP_SPAN_CAPTURE_PLAYBACK] == 0);
    assert(report->cancelled_frames == 1);

    current = test_begin(21000);
    assert(current != cancelled);
    test_time_us = 22000;
    engine_profile_span_end(stale, EP_SPAN_CAPTURE_PLAYBACK);
    engine_profile_frame_mark(cancelled, EP_PHASE_DRAW_SCRIPT);
    test_end(cancelled, 23000);
    assert(engine_profile_frame_id() == current);
    test_end(current, 24000);

    /* A menu/pause entry invalidates the pending interval before accepting
     * it. The first live update after the pause starts a fresh measurement. */
    test_time_us = 31000;
    assert(engine_profile_frame_begin(0, &test_scene, &test_main_target) == 0);
    first = test_begin(41000);
    assert(first);
    engine_profile_count(EP_COUNT_CAPTURE_ENTRIES, 7);
    test_end(first, 42000);
    current = test_begin(51000);
    assert(current);
    engine_profile_reset("fixture context replacement");
    report = engine_profile_last_report();
    assert(report->frames == 1 && report->partial);
    assert(report->first_frame_id == first && report->last_frame_id == first);
    assert(report->interval_us == 10000 && report->update_us == 1000);
    assert(report->counts[EP_COUNT_CAPTURE_ENTRIES] == 7);
    assert(report->spans_us[EP_SPAN_CAPTURE_PLAYBACK] == 0);

    /* Shutdown may publish accepted samples, but a final body without its
     * next entry is intentionally omitted because its interval is unknown. */
    first = test_begin(61000);
    test_end(first, 62000);
    current = test_begin(71000);
    test_end(current, 72000);
    engine_profile_shutdown();
    report = engine_profile_last_report();
    assert(report->frames == 1 && report->partial);
    assert(report->first_frame_id == first && report->last_frame_id == first);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Reject an unfinished outer update when a nested update enters the engine.
 * The interruption counts once, publishes already paired frames, and gives
 * the nested body a fresh identity that older scope ends cannot modify.
 */
static void test_nested_update(void)
{
    const ep_report *report;
    ep_scope stale;
    uint64_t first, outer, nested, following;

    test_configure(1, 3, 0);
    first = test_begin(1000);
    engine_profile_count(EP_COUNT_MAIN_ENTRIES, 4);
    test_end(first, 2000);
    outer = test_begin(11000);
    stale = engine_profile_span_begin();
    engine_profile_count(EP_COUNT_MAIN_ENTRIES, 900);
    nested = test_begin(12000);
    assert(nested && nested != outer);
    report = engine_profile_last_report();
    assert(report->frames == 1 && report->partial);
    assert(report->first_frame_id == first && report->last_frame_id == first);
    assert(report->cancelled_frames == 1);
    assert(report->counts[EP_COUNT_MAIN_ENTRIES] == 4);
    test_time_us = 12500;
    engine_profile_span_end(stale, EP_SPAN_MAIN_PLAYBACK);
    engine_profile_count(EP_COUNT_MAIN_ENTRIES, 7);
    test_end(nested, 13000);
    following = test_begin(22000);
    assert(following);
    engine_profile_reset("nested fixture complete");
    report = engine_profile_last_report();
    assert(report->frames == 1 && report->partial);
    assert(report->first_frame_id == nested && report->last_frame_id == nested);
    assert(report->interval_us == 10000 && report->update_us == 1000);
    assert(report->counts[EP_COUNT_MAIN_ENTRIES] == 7);
    assert(report->spans_us[EP_SPAN_MAIN_PLAYBACK] == 0);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Exclude debugger stalls and suspension-sized gaps without accepting the
 * prior body's counters or duration. The first subsequent valid pair remains
 * measurable, preventing one discontinuity from poisoning later windows.
 */
static void test_gap_rejection(void)
{
    const ep_report *report;
    uint64_t old, current;
    uint64_t next_time = EP_MAX_GAP_US + 1001;

    test_configure(1, 1, 0);
    old = test_begin(1000);
    engine_profile_count(EP_COUNT_READBACK_BYTES, 999999);
    test_end(old, 1100);
    current = test_begin(next_time);
    assert(engine_profile_last_report()->frames == 0);
    if(!current)
    {
        next_time += 10000;
        current = test_begin(next_time);
    }
    assert(current && current != old);
    engine_profile_count(EP_COUNT_READBACK_BYTES, 64);
    test_end(current, next_time + 100);
    assert(test_begin(next_time + 10000) == 0);
    report = engine_profile_last_report();
    assert(report->frames == 1 && report->first_frame_id == current);
    assert(report->interval_us == 10000 && report->update_us == 100);
    assert(report->counts[EP_COUNT_READBACK_BYTES] == 64);
    assert(report->gap_frames == 1);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Verify exact nearest-rank percentiles for different sample counts. Shuffled
 * intervals prevent a mistakenly unsorted percentile calculation from passing.
 */
static void test_percentiles(void)
{
    const unsigned windows[] = {1, 20, 21, 300, 1000};
    unsigned window_index;

    for(window_index = 0; window_index < sizeof(windows) / sizeof(windows[0]); ++window_index)
    {
        const ep_report *report;
        unsigned frames = windows[window_index];
        unsigned sample;
        uint64_t time_us = 1000;
        uint64_t frame;

        test_configure(1, frames, 0);
        frame = test_begin(time_us);
        for(sample = 0; sample < frames; ++sample)
        {
            uint64_t interval = ((sample * 37) % frames + 1) * 100;

            assert(frame);
            test_end(frame, time_us + 10);
            time_us += interval;
            frame = test_begin(time_us);
        }
        assert(frame == 0);
        report = engine_profile_last_report();
        assert(report->frames == frames);
        assert(report->interval_us == (uint64_t)frames * (frames + 1) * 50);
        assert(report->update_us == (uint64_t)frames * 10);
        assert(report->interval_min_us == 100);
        assert(report->interval_max_us == (uint64_t)frames * 100);
        assert(report->interval_p95_us == (uint64_t)((frames * 95 + 99) / 100) * 100);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Deliver timestamps while their frame is current, pending, accepted, and
 * already reported. Only the first three states may receive GPU duration;
 * late results are counted separately rather than charged to a newer frame.
 */
static void test_gpu_result_correlation(void)
{
    const ep_report *report;
    uint64_t first, second, scene, capture_current, capture_pending, late, next_scene;

    test_configure(1, 2, 0);
    engine_profile_set_gpu_backend(&test_gpu_backend);
    first = test_begin(1000);
    scene = engine_profile_gpu_begin(EP_GPU_SCENE);
    capture_current = engine_profile_gpu_begin(EP_GPU_CAPTURE);
    capture_pending = engine_profile_gpu_begin(EP_GPU_CAPTURE);
    late = engine_profile_gpu_begin(EP_GPU_MAIN_QUEUE);
    assert(scene && capture_current && capture_pending && late);
    engine_profile_gpu_end(capture_current);
    engine_profile_gpu_end(capture_pending);
    engine_profile_gpu_end(late);
    engine_profile_gpu_end(scene);
    test_query_ready(capture_current, 110);
    engine_profile_gpu_poll();
    test_end(first, 2000);
    test_query_ready(capture_pending, 220);
    engine_profile_gpu_poll();

    second = test_begin(11000);
    assert(second > first);
    test_query_ready(scene, 330);
    engine_profile_gpu_poll();
    next_scene = engine_profile_gpu_begin(EP_GPU_SCENE);
    engine_profile_gpu_end(next_scene);
    test_query_ready(next_scene, 400);
    engine_profile_gpu_poll();
    test_end(second, 12000);
    assert(test_begin(21000) == 0);
    report = engine_profile_last_report();
    assert(report->gpu_ns[EP_GPU_SCENE] == 730);
    assert(report->gpu_samples[EP_GPU_SCENE] == 2);
    assert(report->gpu_ns[EP_GPU_CAPTURE] == 330);
    assert(report->gpu_samples[EP_GPU_CAPTURE] == 2);
    assert(report->gpu_samples[EP_GPU_MAIN_QUEUE] == 0);
    assert(report->late_gpu_results == 0);

    /* The delayed queue result belongs to the closed window. It must not
     * alter the published snapshot or a live frame in the following window. */
    test_query_ready(late, 999999);
    engine_profile_gpu_poll();
    assert(engine_profile_last_report()->gpu_samples[EP_GPU_MAIN_QUEUE] == 0);
    first = test_begin(31000);
    test_end(first, 32000);
    second = test_begin(41000);
    test_end(second, 42000);
    assert(test_begin(51000) == 0);
    report = engine_profile_last_report();
    assert(report->gpu_samples[EP_GPU_MAIN_QUEUE] == 0);
    assert(report->gpu_ns[EP_GPU_MAIN_QUEUE] == 0);
    assert(report->late_gpu_results == 1);
    assert(test_query_ends == 5);
    assert(test_query_polls > 0);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Keep delayed timestamps from a cancelled frame separate from the next live
 * frame. Rendering can continue while old query objects finish asynchronously.
 */
static void test_cancelled_gpu_result(void)
{
    const ep_report *report;
    uint64_t cancelled, current, old_query, new_query;

    test_configure(1, 1, 0);
    engine_profile_set_gpu_backend(&test_gpu_backend);
    cancelled = test_begin(1000);
    old_query = engine_profile_gpu_begin(EP_GPU_CAPTURE);
    engine_profile_gpu_end(old_query);
    engine_profile_frame_cancel(cancelled, "cancel before timestamp completion");
    current = test_begin(11000);
    assert(current && current != cancelled);
    test_query_ready(old_query, 999999);
    engine_profile_gpu_poll();
    new_query = engine_profile_gpu_begin(EP_GPU_CAPTURE);
    engine_profile_gpu_end(new_query);
    test_query_ready(new_query, 100);
    engine_profile_gpu_poll();
    test_end(current, 12000);
    assert(test_begin(21000) == 0);
    report = engine_profile_last_report();
    assert(report->first_frame_id == current && report->last_frame_id == current);
    assert(report->gpu_samples[EP_GPU_CAPTURE] == 1);
    assert(report->gpu_ns[EP_GPU_CAPTURE] == 100);
    assert(report->cancelled_frames == 1);
    assert(report->late_gpu_results == 1);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Retain frame-identity uniqueness across explicit collector reconfiguration.
 * Installed clock/query providers can outlive a collection window, so their
 * outstanding scopes must not match a newly reset frame by accident.
 */
static void test_reconfigured_frame_identity(void)
{
    const ep_report *report;
    ep_scope stale;
    uint64_t old_frame, current, old_query, new_query;

    test_configure(1, 3, 0);
    engine_profile_set_gpu_backend(&test_gpu_backend);
    old_frame = test_begin(1000);
    stale = engine_profile_span_begin();
    old_query = engine_profile_gpu_begin(EP_GPU_CAPTURE);
    engine_profile_gpu_end(old_query);
    engine_profile_count(EP_COUNT_MAIN_ENTRIES, 999);
    engine_profile_configure(1, 1, 0);
    assert(engine_profile_last_report()->frames == 0);
    current = test_begin(11000);
    assert(current > old_frame);
    test_time_us = 11500;
    engine_profile_span_end(stale, EP_SPAN_CAPTURE_PLAYBACK);
    test_query_ready(old_query, 999999);
    engine_profile_gpu_poll();
    new_query = engine_profile_gpu_begin(EP_GPU_CAPTURE);
    engine_profile_gpu_end(new_query);
    test_query_ready(new_query, 100);
    engine_profile_gpu_poll();
    engine_profile_count(EP_COUNT_MAIN_ENTRIES, 5);
    test_end(current, 12000);
    assert(test_begin(21000) == 0);
    report = engine_profile_last_report();
    assert(report->first_frame_id == current && report->last_frame_id == current);
    assert(report->counts[EP_COUNT_MAIN_ENTRIES] == 5);
    assert(report->spans_us[EP_SPAN_CAPTURE_PLAYBACK] == 0);
    assert(report->gpu_samples[EP_GPU_CAPTURE] == 1);
    assert(report->gpu_ns[EP_GPU_CAPTURE] == 100);
    assert(report->late_gpu_results == 1);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Check a freshly started process's environment-derived collection settings.
 * The runner invokes this before any explicit configure call in that process.
 */
static void test_environment(unsigned frames, unsigned warmup, int enabled)
{
    uint64_t frame;
    uint64_t time_us = 1000;
    unsigned sample;

    engine_profile_set_hooks(test_clock, test_log);
    assert(engine_profile_enabled() == enabled);
    if(!enabled)
    {
        assert(test_begin(time_us) == 0);
        assert(test_clock_reads == 0);
        return;
    }
    for(sample = 0; sample < warmup; ++sample)
    {
        assert(test_begin(time_us) == 0);
        time_us += 10000;
    }
    frame = test_begin(time_us);
    for(sample = 0; sample < frames; ++sample)
    {
        assert(frame);
        test_end(frame, time_us + 100);
        time_us += 10000;
        frame = test_begin(time_us);
    }
    assert(frame == 0);
    assert(engine_profile_last_report()->frames == frames);
    assert(engine_profile_last_report()->interval_us == (uint64_t)frames * 10000);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Run deterministic accounting independently of SDL. Dedicated process modes
 * also exercise environment parsing without relying on private reset hooks.
 */
int main(int argc, char **argv)
{
    if(argc > 1)
    {
        if(strcmp(argv[1], "--environment-defaults") == 0)
        {
            test_environment(EP_DEFAULT_WINDOW, EP_DEFAULT_WARMUP, 1);
        }
        else if(strcmp(argv[1], "--environment-disabled") == 0)
        {
            test_environment(0, 0, 0);
        }
        else if(strcmp(argv[1], "--environment-custom") == 0)
        {
            test_environment(7, 2, 1);
        }
        else
        {
            fprintf(stderr, "Unknown profiler fixture: %s\n", argv[1]);
            return 1;
        }
        puts("Engine profiler environment test passed.");
        return 0;
    }
    test_disabled();
    test_pending_window_and_counters();
    test_warmup_and_restart();
    test_exclusive_phases_and_inclusive_spans();
    test_palette_cache_accounting();
    test_profile_entry_phase();
    test_cancel_pause_and_reset();
    test_nested_update();
    test_gap_rejection();
    test_percentiles();
    test_gpu_result_correlation();
    test_cancelled_gpu_result();
    test_reconfigured_frame_identity();
    engine_profile_configure(0, 1, 0);
    puts("Engine profiler tests passed: pairing, windows, phases, spans, counters, palette-cache accounting, invalidation, p95, delayed GPU results.");
    return 0;
}
