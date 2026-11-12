/*
 * Deterministic sampled script/native profiler regressions.
 *
 * Run through tests/run_gpu_tests.sh. Explicit clock values distinguish exact
 * exclusive accounting from whole-frame sampling and delayed frame acceptance.
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine_profile.h"

static uint64_t test_time_us;
static uint64_t test_clock_reads;
static unsigned test_log_lines;
static char test_log_text[131072];
static size_t test_log_length;
static int test_scene;
static int test_target;
static unsigned test_alpha;
static unsigned test_beta;
static unsigned test_gamma;

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Return fixture-controlled time and expose every clock access. Rejected,
 * disabled and unsampled hooks must not hide timing work behind zero totals.
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
 * Retain bounded complete report text while keeping exact assertions on the
 * public integer snapshots rather than their rounded millisecond formatting.
 */
static void test_log(const char *line)
{
    size_t length;

    assert(line && strncmp(line, "[ENGINE PROFILE]", 16) == 0);
    length = strlen(line);
    if(length < sizeof(test_log_text) - test_log_length)
    {
        memcpy(test_log_text + test_log_length, line, length + 1);
        test_log_length += length;
    }
    ++test_log_lines;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Reset collection, preserving the process-lifetime native ID registry just
 * as compiled instructions do. Every fixture explicitly chooses its cadence.
 */
static void test_configure(int enabled, int script_enabled,
                           unsigned frames, unsigned warmup, unsigned interval)
{
    engine_profile_set_gpu_backend(NULL);
    engine_profile_set_hooks(test_clock, test_log);
    engine_profile_configure(enabled, frames, warmup);
    engine_profile_script_configure(script_enabled, interval);
    test_time_us = 0;
    test_clock_reads = 0;
    test_log_lines = 0;
    test_log_length = 0;
    test_log_text[0] = '\0';
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Begin a live update with stable scene and target identity. The following
 * entry, not its own end, supplies this frame's accepted interval.
 */
static uint64_t test_begin(uint64_t time_us)
{
    test_time_us = time_us;
    return engine_profile_frame_begin(1, &test_scene, &test_target);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Advance the engine's completed-phase boundary. Script time crossing the
 * mark must be charged to the old phase before the new phase becomes active.
 */
static void test_mark(uint64_t frame, ep_phase phase, uint64_t time_us)
{
    test_time_us = time_us;
    engine_profile_frame_mark(frame, phase);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Finish one body without accepting its interval or manufacturing an extra
 * script/native call at the frame boundary.
 */
static void test_end(uint64_t frame, uint64_t time_us)
{
    test_time_us = time_us;
    engine_profile_frame_end(frame);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Enter an interpreter scope at a known timestamp. This also exercises the
 * public hook's own no-work path, independently of its production fast gate.
 */
static ep_script_scope test_script_begin(uint64_t time_us)
{
    test_time_us = time_us;
    return engine_profile_script_begin();
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Enter a registered native function at an exact boundary between owners.
 */
static ep_script_scope test_native_begin(unsigned id, uint64_t time_us)
{
    test_time_us = time_us;
    return engine_profile_native_begin(id);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Return from the requested scope. Token identity is deliberately independent
 * of the current stack owner so tests can exercise stale and invalid returns.
 */
static void test_scope_end(ep_script_scope scope, uint64_t time_us)
{
    test_time_us = time_us;
    engine_profile_script_end(scope);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Verify stable copied identifiers, including names whose temporary backing
 * buffer is later changed. Invalid names must never become format/log input.
 */
static void test_registry_identity(void)
{
    char copied[] = "profile_fixture_copied";
    char oversized[EP_SCRIPT_NAME_LENGTH + 1];
    unsigned id;

    test_alpha = engine_profile_native_register("profile_fixture_alpha");
    test_beta = engine_profile_native_register("profile_fixture_beta");
    test_gamma = engine_profile_native_register("profile_fixture_gamma");
    assert(test_alpha && test_beta && test_gamma);
    assert(test_alpha != test_beta && test_beta != test_gamma);
    assert(engine_profile_native_register("profile_fixture_alpha") == test_alpha);
    assert(strcmp(engine_profile_native_name(test_alpha), "profile_fixture_alpha") == 0);
    id = engine_profile_native_register(copied);
    assert(id);
    copied[0] = 'X';
    assert(strcmp(engine_profile_native_name(id), "profile_fixture_copied") == 0);
    assert(engine_profile_native_register(NULL) == 0);
    assert(engine_profile_native_register("") == 0);
    assert(engine_profile_native_register("9bad_name") == 0);
    assert(engine_profile_native_register("bad name") == 0);
    assert(engine_profile_native_register("bad%name") == 0);
    assert(engine_profile_native_register("bad\nname") == 0);
    memset(oversized, 'x', sizeof(oversized) - 1);
    oversized[sizeof(oversized) - 1] = '\0';
    assert(engine_profile_native_register(oversized) == 0);
    assert(strcmp(engine_profile_native_name(0), "other_native") == 0);

    /* Reconfiguration must not recycle IDs stored by existing CALL records. */
    test_configure(1, 1, 1, 0, 1);
    assert(engine_profile_native_register("profile_fixture_alpha") == test_alpha);
    engine_profile_reset("registry fixture");
    assert(strcmp(engine_profile_native_name(test_beta), "profile_fixture_beta") == 0);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Prove that neither the master-disabled path nor the detailed opt-out does
 * timer work. Warmup and report barriers similarly have no live script gate.
 */
static void test_disabled_and_inactive(void)
{
    ep_script_scope script, native;
    const ep_script_report *report;
    uint64_t frame, reads;

    test_configure(0, 1, 1, 0, 1);
    assert(!engine_profile_script_active && test_begin(1000) == 0);
    script = test_script_begin(1100);
    native = test_native_begin(test_alpha, 1200);
    test_scope_end(native, 1300);
    test_scope_end(script, 1400);
    assert(script.frame_id == 0 && native.frame_id == 0);
    assert(test_clock_reads == 0 && test_log_lines == 0);

    test_configure(1, 0, 1, 0, 1);
    frame = test_begin(1000);
    assert(frame && !engine_profile_script_active);
    reads = test_clock_reads;
    script = test_script_begin(1100);
    native = test_native_begin(test_alpha, 1200);
    test_scope_end(native, 1300);
    test_scope_end(script, 1400);
    assert(test_clock_reads == reads);
    test_end(frame, 2000);
    assert(test_begin(11000) == 0);
    report = engine_profile_last_script_report();
    assert(report->sampled_frames == 0 && report->invalid_frames == 0);
    assert(report->unsampled_frames == 1 && report->clock_reads == 0);
    assert(!engine_profile_script_active);
    reads = test_clock_reads;
    assert(test_script_begin(12000).frame_id == 0);
    assert(test_clock_reads == reads);

    test_configure(1, 1, 1, 2, 1);
    assert(test_begin(1000) == 0 && !engine_profile_script_active);
    reads = test_clock_reads;
    assert(test_script_begin(2000).frame_id == 0);
    assert(test_clock_reads == reads);
    assert(test_begin(11000) == 0 && !engine_profile_script_active);
    frame = test_begin(21000);
    assert(frame && engine_profile_script_active);
    test_end(frame, 22000);
    assert(test_begin(31000) == 0);
    report = engine_profile_last_script_report();
    assert(report->sampled_frames == 1 && report->unsampled_frames == 0);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Partition VM -> native -> native and native -> VM -> native reentry exactly.
 * Same-owner VM nesting requires stack identity but no extra clock reads.
 */
static void test_exclusive_reentry(void)
{
    ep_script_scope outer, same_vm, alpha, beta, callback, gamma;
    const ep_script_report *report;
    const ep_script_phase *phase;
    uint64_t frame, reads;

    test_configure(1, 1, 1, 0, 1);
    frame = test_begin(1000);
    test_mark(frame, EP_PHASE_DRAW_SETUP, 1000);
    outer = test_script_begin(1100);
    reads = test_clock_reads;
    same_vm = test_script_begin(1200);
    test_scope_end(same_vm, 1300);
    assert(test_clock_reads == reads);
    alpha = test_native_begin(test_alpha, 1400);
    beta = test_native_begin(test_beta, 1500);
    test_scope_end(beta, 1600);
    callback = test_script_begin(1700);
    gamma = test_native_begin(test_gamma, 1800);
    test_scope_end(gamma, 1900);
    test_scope_end(callback, 2000);
    test_scope_end(alpha, 2100);
    test_scope_end(outer, 2300);
    engine_profile_count(EP_COUNT_CAPTURE_ENTRIES, 41);
    test_mark(frame, EP_PHASE_DRAW_SCRIPT, 2400);
    test_end(frame, 2500);
    assert(test_begin(11000) == 0);

    report = engine_profile_last_script_report();
    phase = &report->phases[EP_PHASE_DRAW_SCRIPT];
    assert(report->sampled_frames == 1 && report->invalid_frames == 0);
    assert(report->clock_reads == 10);
    assert(phase->interpreter_us == 700 && phase->native_us == 500);
    assert(phase->native_calls == 3);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][test_alpha].self_us == 300);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][test_beta].self_us == 100);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][test_gamma].self_us == 100);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][test_alpha].calls == 1);
    assert(report->phases_us[EP_PHASE_DRAW_SCRIPT] == 1400);
    assert(report->interval_us == 10000 && report->update_us == 1500);
    assert(report->slow_count == 1 && report->slow[0].frame_id == frame);
    assert(report->slow[0].counts[EP_COUNT_CAPTURE_ENTRIES] == 41);
    assert(report->slow[0].script.phases[EP_PHASE_DRAW_SCRIPT].native_us == 500);
    assert(report->slow[0].top_count == 3);
    assert(report->slow[0].top[0].function_id == test_alpha);
    assert(report->slow[0].top[0].phase == EP_PHASE_DRAW_SCRIPT);
    assert(report->slow[0].top[0].self_us == 300);
    assert(strstr(test_log_text, "profile_fixture_alpha"));
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Native time crossing a phase checkpoint follows execution time, while its
 * invocation remains counted once in the entry phase. Zero-length calls and
 * same-ID native nesting retain their counts without adding timer reads.
 */
static void test_phase_crossing_and_short_calls(void)
{
    ep_script_scope script, alpha, same_native, zero;
    const ep_script_report *report;
    uint64_t frame, reads;

    test_configure(1, 1, 1, 0, 1);
    frame = test_begin(1000);
    script = test_script_begin(1050);
    alpha = test_native_begin(test_alpha, 1100);
    test_mark(frame, EP_PHASE_PRE_WAIT, 1200);
    test_scope_end(alpha, 1300);
    test_scope_end(script, 1400);
    test_mark(frame, EP_PHASE_INPUT, 1450);
    test_mark(frame, EP_PHASE_DRAW_SETUP, 1450);
    script = test_script_begin(1500);
    alpha = test_native_begin(test_alpha, 1550);
    reads = test_clock_reads;
    same_native = test_native_begin(test_alpha, 1560);
    test_scope_end(same_native, 1570);
    assert(test_clock_reads == reads);
    test_scope_end(alpha, 1600);
    zero = test_native_begin(test_beta, 1600);
    test_scope_end(zero, 1600);
    test_scope_end(script, 1650);
    test_mark(frame, EP_PHASE_DRAW_SCRIPT, 1700);
    test_end(frame, 1750);
    assert(test_begin(11000) == 0);
    report = engine_profile_last_script_report();
    assert(report->invalid_frames == 0);
    assert(report->phases[EP_PHASE_PRE_WAIT].interpreter_us == 50);
    assert(report->phases[EP_PHASE_PRE_WAIT].native_us == 100);
    assert(report->phases[EP_PHASE_PRE_WAIT].native_calls == 1);
    assert(report->phases[EP_PHASE_INPUT].interpreter_us == 100);
    assert(report->phases[EP_PHASE_INPUT].native_us == 100);
    assert(report->phases[EP_PHASE_INPUT].native_calls == 0);
    assert(report->functions[EP_PHASE_INPUT][test_alpha].calls == 0);
    assert(report->functions[EP_PHASE_INPUT][test_alpha].self_us == 100);
    assert(report->phases[EP_PHASE_DRAW_SCRIPT].interpreter_us == 100);
    assert(report->phases[EP_PHASE_DRAW_SCRIPT].native_us == 50);
    assert(report->phases[EP_PHASE_DRAW_SCRIPT].native_calls == 3);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][test_alpha].calls == 2);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][test_alpha].self_us == 50);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][test_beta].calls == 1);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][test_beta].self_us == 0);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Retain slow detailed samples with their original phase, span and counter
 * values. Faster sampled frames must not borrow the larger intervals of
 * unsampled frames, and sampled averages use only the sampled denominator.
 */
static void test_sampling_and_slow_frame_identity(void)
{
    const uint64_t intervals[] = {10000, 50000, 30000, 60000, 20000, 70000};
    uint64_t frames[6];
    uint64_t time_us = 1000;
    uint64_t frame;
    unsigned index;
    const ep_script_report *report;

    test_configure(1, 1, 6, 0, 2);
    frame = test_begin(time_us);
    for(index = 0; index < 6; ++index)
    {
        ep_script_scope script, native;
        ep_scope span;
        uint64_t duration = (index + 1) * 100;
        uint64_t reads;

        assert(frame);
        frames[index] = frame;
        assert(engine_profile_script_active == (index % 2 == 0));
        test_mark(frame, EP_PHASE_DRAW_SETUP, time_us);
        reads = test_clock_reads;
        script = test_script_begin(time_us + 10);
        native = test_native_begin(index == 2 ? test_beta : test_alpha, time_us + 20);
        test_scope_end(native, time_us + 20 + duration);
        test_scope_end(script, time_us + 30 + duration);
        if(index % 2)
        {
            assert(test_clock_reads == reads);
        }
        test_mark(frame, EP_PHASE_DRAW_SCRIPT, time_us + 50 + duration);
        engine_profile_count(EP_COUNT_NATIVE_DRAWS, 100 + index);
        test_time_us = time_us + 700;
        span = engine_profile_span_begin();
        test_time_us = time_us + 710 + index;
        engine_profile_span_end(span, EP_SPAN_GPU_STATE);
        test_end(frame, time_us + 1000);
        time_us += intervals[index];
        frame = test_begin(time_us);
    }
    assert(frame == 0);
    report = engine_profile_last_script_report();
    assert(report->interval == 2);
    assert(report->sampled_frames == 3 && report->unsampled_frames == 3);
    assert(report->invalid_frames == 0);
    assert(report->interval_us == 60000 && report->update_us == 3000);
    assert(report->unsampled_interval_us == 180000);
    assert(report->unsampled_update_us == 3000);
    assert(report->phases_us[EP_PHASE_DRAW_SCRIPT] == 1050);
    assert(report->phases[EP_PHASE_DRAW_SCRIPT].interpreter_us == 60);
    assert(report->phases[EP_PHASE_DRAW_SCRIPT].native_us == 900);
    assert(report->phases[EP_PHASE_DRAW_SCRIPT].native_calls == 3);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][test_alpha].calls == 2);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][test_alpha].self_us == 600);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][test_beta].self_us == 300);
    assert(report->clock_reads == 12);
    assert(report->slow_count == 3);
    assert(report->slow[0].frame_id == frames[2] && report->slow[0].interval_us == 30000);
    assert(report->slow[1].frame_id == frames[4] && report->slow[1].interval_us == 20000);
    assert(report->slow[2].frame_id == frames[0] && report->slow[2].interval_us == 10000);
    assert(report->slow[0].phases_us[EP_PHASE_DRAW_SCRIPT] == 350);
    assert(report->slow[0].spans_us[EP_SPAN_GPU_STATE] == 12);
    assert(report->slow[0].counts[EP_COUNT_NATIVE_DRAWS] == 102);
    assert(report->slow[0].top_count == 1);
    assert(report->slow[0].top[0].function_id == test_beta);
    assert(report->slow[0].top[0].self_us == 300);
    assert(engine_profile_last_report()->interval_max_us == 70000);
    assert(strstr(test_log_text,
        "interval=2 sampled_frames=3 invalid_frames=0 unsampled_frames=3 "
        "sampled_frame_mean_ms=20.000000 sampled_update_mean_ms=1.000000 "
        "unsampled_frame_mean_ms=60.000000 unsampled_update_mean_ms=1.000000"));
    assert(strstr(test_log_text,
        "phase=draw_script phase_ms=0.350000 interpreter_self_ms=0.020000 "
        "native_self_ms=0.300000 outside_interpreter_ms=0.030000 "
        "native_calls=3 calls_per_sample=1.000000"));
    assert(strstr(test_log_text,
        "function=profile_fixture_alpha calls=2 calls_per_sample=0.666667 "
        "self_ms_per_sample=0.200000 self_us_per_call=300.000000"));
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Keep sample cadence independent of ordinary report barriers. A three-frame
 * window sampled every two frames alternates between two and one samples.
 */
static void test_cadence_across_report_barrier(void)
{
    unsigned index;
    uint64_t time_us = 1000;
    uint64_t frame;

    test_configure(1, 1, 3, 0, 2);
    frame = test_begin(time_us);
    for(index = 0; index < 6; ++index)
    {
        assert(frame);
        assert(engine_profile_script_active == (index % 2 == 0));
        test_end(frame, time_us + 100);
        time_us += 10000;
        frame = test_begin(time_us);
        if(index == 2 || index == 5)
        {
            const ep_script_report *report = engine_profile_last_script_report();

            assert(frame == 0);
            assert(report->sampled_frames == (index == 2 ? 2U : 1U));
            assert(report->unsampled_frames == (index == 2 ? 1U : 2U));
            if(index == 2)
            {
                time_us += 10000;
                frame = test_begin(time_us);
            }
        }
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Bound slow-frame and native-function rankings without truncating aggregate
 * accounting. Equal native time prefers call count then stable ID, and equal
 * frame intervals preserve the earlier frame's correlated snapshot first.
 */
static void test_bounded_rankings(void)
{
    const uint64_t intervals[] = {90000, 30000, 60000, 60000, 20000};
    unsigned ids[EP_SCRIPT_TOP_FUNCTIONS + 2];
    uint64_t frames[5];
    uint64_t frame, time_us = 1000;
    unsigned index, function;
    char name[64];
    const ep_script_report *report;

    for(function = 0; function < sizeof(ids) / sizeof(ids[0]); ++function)
    {
        snprintf(name, sizeof(name), "profile_rank_%u", function);
        ids[function] = engine_profile_native_register(name);
        assert(ids[function]);
    }
    test_configure(1, 1, 5, 0, 1);
    frame = test_begin(time_us);
    for(index = 0; index < 5; ++index)
    {
        ep_script_scope script, native;
        uint64_t call_time = time_us + 20;

        frames[index] = frame;
        test_mark(frame, EP_PHASE_DRAW_SETUP, time_us);
        script = test_script_begin(time_us + 10);
        for(function = 0; function < sizeof(ids) / sizeof(ids[0]); ++function)
        {
            native = test_native_begin(ids[function], call_time);
            if(function == EP_SCRIPT_TOP_FUNCTIONS + 1)
            {
                test_scope_end(native, call_time + 50);
                native = test_native_begin(ids[function], call_time + 50);
            }
            call_time += 100;
            test_scope_end(native, call_time);
        }
        test_scope_end(script, call_time + 10);
        test_mark(frame, EP_PHASE_DRAW_SCRIPT, call_time + 20);
        engine_profile_count(EP_COUNT_NATIVE_DRAWS, 100 + index);
        test_end(frame, time_us + 1200);
        time_us += intervals[index];
        frame = test_begin(time_us);
    }
    assert(frame == 0);
    report = engine_profile_last_script_report();
    assert(report->sampled_frames == 5);
    assert(report->slow_count == EP_SCRIPT_SLOW_FRAMES);
    assert(report->slow[0].frame_id == frames[0]);
    assert(report->slow[1].frame_id == frames[2]);
    assert(report->slow[2].frame_id == frames[3]);
    assert(report->slow[2].counts[EP_COUNT_NATIVE_DRAWS] == 103);
    for(index = 0; index < EP_SCRIPT_SLOW_FRAMES; ++index)
    {
        const ep_script_slow_frame *slow = &report->slow[index];

        assert(slow->top_count == EP_SCRIPT_TOP_FUNCTIONS);
        assert(slow->top[0].function_id == ids[EP_SCRIPT_TOP_FUNCTIONS + 1]);
        assert(slow->top[0].calls == 2 && slow->top[0].self_us == 100);
        for(function = 1; function < EP_SCRIPT_TOP_FUNCTIONS; ++function)
        {
            assert(slow->top[function].function_id == ids[function - 1]);
            assert(slow->top[function].calls == 1 && slow->top[function].self_us == 100);
        }
    }
    for(function = 0; function < sizeof(ids) / sizeof(ids[0]); ++function)
    {
        assert(report->functions[EP_PHASE_DRAW_SCRIPT][ids[function]].self_us == 500);
        assert(report->functions[EP_PHASE_DRAW_SCRIPT][ids[function]].calls ==
               (function == EP_SCRIPT_TOP_FUNCTIONS + 1 ? 10U : 5U));
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Old tokens from cancelled, reconfigured or interrupted updates must never
 * pop a newer frame's same-depth scope or contribute old native call counts.
 */
static void test_stale_scope_identity(void)
{
    ep_script_scope old_script, old_native, script, native;
    const ep_script_report *report;
    uint64_t old, current, reads;

    test_configure(1, 1, 1, 0, 1);
    old = test_begin(1000);
    old_script = test_script_begin(1100);
    old_native = test_native_begin(test_alpha, 1200);
    engine_profile_frame_cancel(old, "stale script fixture");
    assert(!engine_profile_script_active);
    current = test_begin(11000);
    assert(current > old);
    test_mark(current, EP_PHASE_DRAW_SETUP, 11000);
    script = test_script_begin(11100);
    native = test_native_begin(test_beta, 11200);
    reads = test_clock_reads;
    test_scope_end(old_native, 11300);
    test_scope_end(old_script, 11400);
    assert(test_clock_reads == reads && engine_profile_script_active);
    test_scope_end(native, 11500);
    test_scope_end(script, 11600);
    test_mark(current, EP_PHASE_DRAW_SCRIPT, 11700);
    test_end(current, 12000);
    assert(test_begin(21000) == 0);
    report = engine_profile_last_script_report();
    assert(report->sampled_frames == 1 && report->invalid_frames == 0);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][test_alpha].calls == 0);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][test_beta].calls == 1);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][test_beta].self_us == 300);
    assert(report->slow[0].frame_id == current);

    /* Explicit collector configuration likewise preserves unique frame IDs. */
    old = test_begin(31000);
    old_script = test_script_begin(31100);
    test_configure(1, 1, 1, 0, 1);
    current = test_begin(41000);
    assert(current > old);
    script = test_script_begin(41100);
    test_scope_end(old_script, 41200);
    test_scope_end(script, 41300);
    test_mark(current, EP_PHASE_PRE_WAIT, 41400);
    test_end(current, 42000);
    assert(test_begin(51000) == 0);
    report = engine_profile_last_script_report();
    assert(report->sampled_frames == 1 && report->invalid_frames == 0);
    assert(report->phases[EP_PHASE_PRE_WAIT].interpreter_us == 200);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Corrupted scope sequencing invalidates detailed attribution only. Ordinary
 * frame duration/work counters remain useful and invalid timing cannot enter
 * function totals or the slow detailed-frame ranking.
 */
static void test_invalid_detail(unsigned scenario, unsigned expected_flag)
{
    ep_script_scope outer, inner, fresh;
    ep_script_scope stack[EP_SCRIPT_STACK_LIMIT + 1];
    const ep_script_report *report;
    uint64_t frame;
    unsigned index;

    test_configure(1, 1, 1, 0, 1);
    frame = test_begin(1000);
    test_mark(frame, EP_PHASE_DRAW_SETUP, 1000);
    outer = test_script_begin(1100);
    if(scenario == 0)
    {
        inner = test_native_begin(test_alpha, 1200);
        test_scope_end(outer, 1300);
        test_scope_end(inner, 1400);
    }
    else if(scenario == 1)
    {
        inner = test_native_begin(test_alpha, 1200);
        test_scope_end(inner, 1300);
        fresh = test_native_begin(test_beta, 1400);
        test_scope_end(inner, 1500);
        test_scope_end(fresh, 1600);
    }
    else if(scenario == 2)
    {
        stack[0] = outer;
        for(index = 1; index <= EP_SCRIPT_STACK_LIMIT; ++index)
        {
            stack[index] = test_script_begin(1200);
        }
        for(index = EP_SCRIPT_STACK_LIMIT + 1; index > 0; --index)
        {
            test_scope_end(stack[index - 1], 1300);
        }
    }
    else if(scenario == 3)
    {
        inner = test_native_begin(test_alpha, 1050);
        test_scope_end(inner, 1200);
    }
    else if(scenario == 5)
    {
        test_scope_end(outer, 1400);
        inner = test_script_begin(1300);
        test_scope_end(inner, 1500);
    }
    else
    {
        assert(scenario == 4);
        (void)test_native_begin(test_alpha, 1200);
    }
    if(scenario != 4)
    {
        test_scope_end(outer, 1700);
    }
    engine_profile_count(EP_COUNT_NATIVE_DRAWS, 17);
    test_mark(frame, EP_PHASE_DRAW_SCRIPT, 2000);
    test_end(frame, 2100);
    assert(test_begin(11000) == 0);
    report = engine_profile_last_script_report();
    assert(report->sampled_frames == 0 && report->invalid_frames == 1);
    assert(report->unsampled_frames == 0 && report->slow_count == 0);
    assert(report->error_flags & expected_flag);
    assert(report->phases[EP_PHASE_DRAW_SCRIPT].native_us == 0);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][test_alpha].calls == 0);
    assert(engine_profile_last_report()->frames == 1);
    assert(engine_profile_last_report()->counts[EP_COUNT_NATIVE_DRAWS] == 17);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Never publish a final body without its next entry, or reuse a rejected long
 * interval's native cost. Partial reports retain accepted sampled frames only.
 */
static void test_partial_and_gap_exclusion(void)
{
    ep_script_scope script, native;
    const ep_script_report *report;
    uint64_t frame, accepted, time_us;

    test_configure(1, 1, 3, 0, 1);
    accepted = test_begin(1000);
    script = test_script_begin(1100);
    native = test_native_begin(test_alpha, 1200);
    test_scope_end(native, 1300);
    test_scope_end(script, 1400);
    test_mark(accepted, EP_PHASE_PRE_WAIT, 1500);
    test_end(accepted, 2000);
    frame = test_begin(11000);
    script = test_script_begin(11100);
    native = test_native_begin(test_beta, 11200);
    test_scope_end(native, 11900);
    test_scope_end(script, 12000);
    test_mark(frame, EP_PHASE_PRE_WAIT, 12100);
    test_end(frame, 12500);
    engine_profile_shutdown();
    report = engine_profile_last_script_report();
    assert(report->sampled_frames == 1 && report->slow[0].frame_id == accepted);
    assert(report->functions[EP_PHASE_PRE_WAIT][test_alpha].self_us == 100);
    assert(report->functions[EP_PHASE_PRE_WAIT][test_beta].calls == 0);

    test_configure(1, 1, 1, 0, 1);
    frame = test_begin(1000);
    script = test_script_begin(1100);
    native = test_native_begin(test_alpha, 1200);
    test_scope_end(native, 1900);
    test_scope_end(script, 2000);
    test_mark(frame, EP_PHASE_PRE_WAIT, 2050);
    test_end(frame, 2100);
    time_us = EP_MAX_GAP_US + 2000;
    frame = test_begin(time_us);
    if(!frame)
    {
        time_us += 10000;
        frame = test_begin(time_us);
    }
    assert(frame);
    script = test_script_begin(time_us + 100);
    native = test_native_begin(test_beta, time_us + 200);
    test_scope_end(native, time_us + 300);
    test_scope_end(script, time_us + 400);
    test_mark(frame, EP_PHASE_PRE_WAIT, time_us + 500);
    test_end(frame, time_us + 1000);
    assert(test_begin(time_us + 10000) == 0);
    report = engine_profile_last_script_report();
    assert(report->sampled_frames == 1 && report->invalid_frames == 0);
    assert(report->functions[EP_PHASE_PRE_WAIT][test_alpha].calls == 0);
    assert(report->functions[EP_PHASE_PRE_WAIT][test_beta].self_us == 100);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Fill fixed registry storage only after other fixtures need their names.
 * Saturation keeps existing IDs stable and accounts unknown work in ID zero.
 */
static void test_registry_capacity(void)
{
    char name[64];
    unsigned index, id = 1;
    ep_script_scope script, native;
    const ep_script_report *report;
    uint64_t frame;

    for(index = 0; index <= EP_SCRIPT_MAX_FUNCTIONS; ++index)
    {
        snprintf(name, sizeof(name), "profile_capacity_%u", index);
        id = engine_profile_native_register(name);
        if(!id)
        {
            break;
        }
    }
    assert(id == 0 && index < EP_SCRIPT_MAX_FUNCTIONS);
    assert(engine_profile_native_register("profile_fixture_alpha") == test_alpha);
    assert(engine_profile_native_register("profile_capacity_after_full") == 0);
    test_configure(1, 1, 1, 0, 1);
    frame = test_begin(1000);
    script = test_script_begin(1100);
    native = test_native_begin(0, 1200);
    test_scope_end(native, 1400);
    test_scope_end(script, 1500);
    test_mark(frame, EP_PHASE_PRE_WAIT, 1600);
    test_end(frame, 2000);
    assert(test_begin(11000) == 0);
    report = engine_profile_last_script_report();
    assert(report->functions[EP_PHASE_PRE_WAIT][0].calls == 1);
    assert(report->functions[EP_PHASE_PRE_WAIT][0].self_us == 200);
    assert(report->slow[0].top[0].function_id == 0);
    assert(strstr(test_log_text, "other_native"));
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Exercise startup environment parsing in independent processes. No explicit
 * collector configure may run before this test's first profiler entry.
 */
static void test_environment(unsigned expected_interval, int detailed_enabled)
{
    const ep_script_report *report;
    unsigned index;
    uint64_t time_us = 1000;
    uint64_t frame;

    engine_profile_set_hooks(test_clock, test_log);
    frame = test_begin(time_us);
    for(index = 0; index < 5; ++index)
    {
        assert(frame);
        assert(engine_profile_script_active ==
               (detailed_enabled && index % expected_interval == 0));
        test_end(frame, time_us + 100);
        time_us += 10000;
        frame = test_begin(time_us);
    }
    assert(frame == 0);
    report = engine_profile_last_script_report();
    assert(report->interval == expected_interval);
    assert(report->sampled_frames == (detailed_enabled ? (4 / expected_interval + 1) : 0));
    assert(report->unsampled_frames == 5 - report->sampled_frames);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Run standalone sampled-script accounting fixtures without SDL or engine
 * interpreter dependencies. The separate integration suite uses real scripts.
 */
int main(int argc, char **argv)
{
    if(argc == 2)
    {
        if(strcmp(argv[1], "--environment-defaults") == 0)
        {
            test_environment(EP_DEFAULT_SCRIPT_INTERVAL, 1);
        }
        else if(strcmp(argv[1], "--environment-disabled") == 0)
        {
            test_environment(EP_DEFAULT_SCRIPT_INTERVAL, 0);
        }
        else if(strcmp(argv[1], "--environment-custom") == 0)
        {
            test_environment(2, 1);
        }
        else
        {
            fprintf(stderr, "Unknown script-profiler fixture: %s\n", argv[1]);
            return 1;
        }
        puts("Script profiler environment test passed.");
        return 0;
    }
    test_registry_identity();
    test_disabled_and_inactive();
    test_exclusive_reentry();
    test_phase_crossing_and_short_calls();
    test_sampling_and_slow_frame_identity();
    test_cadence_across_report_barrier();
    test_bounded_rankings();
    test_stale_scope_identity();
    test_invalid_detail(0, EP_SCRIPT_ERROR_SCOPE);
    test_invalid_detail(1, EP_SCRIPT_ERROR_SCOPE);
    test_invalid_detail(2, EP_SCRIPT_ERROR_DEPTH);
    test_invalid_detail(3, EP_SCRIPT_ERROR_CLOCK);
    test_invalid_detail(4, EP_SCRIPT_ERROR_UNCLOSED);
    test_invalid_detail(5, EP_SCRIPT_ERROR_CLOCK);
    test_partial_and_gap_exclusion();
    test_registry_capacity();
    engine_profile_configure(0, 1, 0);
    puts("Script profiler tests passed: exclusive reentry, phases, exact sampled calls, cadence, slow frames, registry, invalidation and disabled timing.");
    return 0;
}
