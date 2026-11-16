/*
 * OpenBOR - http://www.chronocrash.com
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Damon V. Caskey
 * 2026-11-15
 * Exercise production movie control and script APIs with deterministic decoder
 * and GPU boundaries. Include the controller to drive real frame promotion and
 * metadata transitions without exposing test hooks in the engine interface.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scriptcommon.h"
#include "../engine/source/webmlib/movie_playback.c"

#undef malloc
#undef calloc
#undef realloc

s_savedata savedata;
int BGHEIGHT;
animation_id_t *animspecials, *animattacks, *animfollows, *animpains;
animation_id_t *animfalls, *animrises, *animriseattacks, *animdies;
animation_id_t *animidles, *animwalks, *animbackwalks, *animups, *animdowns;
animation_id_t *animbackpains, *animbackfalls, *animbackdies, *animbackrises;
animation_id_t *animbackriseattacks, *animblkpains, *animbackblkpains;

static gpu_yuv_source captured_source;
static gpu_movie_draw captured_draw;
static yuv_video_mode captured_mode;
static yuv_video_mode decoder_mode = { 8, 6, 16, 9 };
static e_drawmethod_resample captured_resample;
static const void *released_owner;
static unsigned draw_calls, capture_calls, setup_calls, release_calls;
static unsigned rejected_selectors;
static int gpu_result = 1;
static uint64_t fake_time = 1000;

/* Damon V. Caskey - 2026-11-15: fail clearly on fixture allocation failure. */
void *__wrap_checkAlloc(void *pointer, size_t size, const char *function,
                        const char *file, int line)
{
    (void)size; (void)function; (void)file; (void)line;
    assert(pointer);
    return pointer;
}

/* Damon V. Caskey - 2026-11-15: observe accepted and rejected GPU submissions. */
int gpu_draw_yuv(s_screen *screen, const gpu_yuv_source *source,
                 const gpu_movie_draw *draw)
{
    assert(screen && source && draw);
    captured_source = *source;
    captured_draw = *draw;
    ++draw_calls;
    return gpu_result;
}

/* Damon V. Caskey - 2026-11-15: observe resource release before owner reuse. */
void gpu_release_yuv(const void *owner)
{
    released_owner = owner;
    ++release_calls;
}

/* Damon V. Caskey - 2026-11-15: capture fullscreen logical sizing decisions. */
int video_setup_yuv_overlay(const yuv_video_mode *mode)
{
    captured_mode = *mode;
    ++setup_calls;
    return gpu_result;
}

/* Damon V. Caskey - 2026-11-15: retain fullscreen plane identity and filter. */
int video_prepare_yuv_frame(const gpu_yuv_source *source,
                            e_drawmethod_resample resample)
{
    captured_source = *source;
    captured_resample = resample;
    return gpu_result;
}

/* Damon V. Caskey - 2026-11-15: isolate explicit capture from presentation. */
int video_capture_yuv_frame(s_screen *screen, const gpu_yuv_source *source,
                            e_drawmethod_resample resample)
{
    assert(screen);
    captured_source = *source;
    captured_resample = resample;
    ++capture_calls;
    return gpu_result;
}

/* Damon V. Caskey - 2026-11-15: supply deterministic final-output success. */
int video_display_yuv_frame(void) { return gpu_result; }

/* Damon V. Caskey - 2026-11-15: omit window creation from controller tests. */
int video_restore_mode(void) { return 1; }

/* Damon V. Caskey - 2026-11-15: provide a monotonic fixture clock. */
u64 timer_uticks(void) { return fake_time; }

/* Damon V. Caskey - 2026-11-15: keep decoder lifecycle initialization local. */
int webm_lifecycle_init(void) { return 1; }

/* Damon V. Caskey - 2026-11-15: no decoder workers exist in this fixture. */
void webm_lifecycle_shutdown(void) {}

/* Damon V. Caskey - 2026-11-15: fail unexpected attempts to open real media. */
webm_context *webm_start_playback_ex(const char *path, int volume,
    int sound_channel, const unsigned char *cache_buffer, size_t cache_size,
    uint64_t seek_timestamp, int play_audio, int replace_all_audio)
{
    (void)path; (void)volume; (void)sound_channel; (void)cache_buffer;
    (void)cache_size; (void)seek_timestamp; (void)play_audio;
    (void)replace_all_audio;
    assert(!"controller fixture attempted to open media");
    return NULL;
}

/* Damon V. Caskey - 2026-11-15: expose a controllable ready metadata boundary. */
e_webm_decoder_state webm_get_decoder_state(webm_context *context)
{
    (void)context;
    return WEBM_DECODER_STATE_READY;
}

/* Damon V. Caskey - 2026-11-15: supply decoder metadata without video decoding. */
void webm_get_video_info(webm_context *context, yuv_video_mode *mode)
{
    (void)context;
    *mode = decoder_mode;
}

/* Damon V. Caskey - 2026-11-15: use a fixed ten-second media duration. */
uint64_t webm_get_duration(webm_context *context)
{
    (void)context;
    return UINT64_C(10000000000);
}

/* Damon V. Caskey - 2026-11-15: select the silent-media monotonic clock. */
int webm_get_audio_playback_position(webm_context *context, uint64_t *position)
{
    (void)context; (void)position;
    return 0;
}

/* Damon V. Caskey - 2026-11-15: keep audio mutation outside renderer assertions. */
void webm_set_audio_paused(webm_context *context, int paused)
{
    (void)context; (void)paused;
}

/* Damon V. Caskey - 2026-11-15: no PCM stream exists in this fixture. */
void webm_set_audio_speed(webm_context *context, double speed)
{
    (void)context; (void)speed;
}

/* Damon V. Caskey - 2026-11-15: emulate nonblocking decoder retirement. */
void webm_request_close(webm_context *context) { (void)context; }

/* Damon V. Caskey - 2026-11-15: allow normal production retirement cleanup. */
int webm_poll_closed(webm_context *context) { (void)context; return 1; }

/* Damon V. Caskey - 2026-11-15: synchronous teardown has no worker to join. */
void webm_close(webm_context *context) { (void)context; }

/* Damon V. Caskey - 2026-11-15: frame scheduling uses the retained look-ahead. */
int webm_get_pending_frame_count(webm_context *context)
{
    (void)context;
    return 0;
}

/* Damon V. Caskey - 2026-11-15: an empty decoder queue is not end of stream. */
int webm_try_get_next_frame(webm_context *context, yuv_frame **frame)
{
    (void)context; (void)frame;
    return 0;
}

/* Damon V. Caskey - 2026-11-15: allocate a real RGB32 destination sentinel. */
static s_screen *fixture_screen(int width, int height)
{
    size_t bytes = (size_t)width * height * sizeof(uint32_t);
    s_screen *screen = calloc(1, sizeof(*screen) + bytes);
    assert(screen);
    screen->magic = screen_magic;
    screen->width = width;
    screen->height = height;
    screen->pixelformat = PIXEL_32;
    memset(screen->data, 0xA5, bytes);
    return screen;
}

/* Damon V. Caskey - 2026-11-15: give one stable channel a retained test frame. */
static s_movie_playback *fixture_playback(int channel)
{
    s_movie_playback *playback = movie_playback_get(channel);
    assert(playback);
    playback->active = 1;
    playback->video_mode = decoder_mode;
    movie_playback_pool.active_mask |= UINT64_C(1) << channel;
    playback->next_frame = yuv_frame_create(8, 6);
    assert(playback->next_frame);
    movie_playback_promote_next_frame(playback);
    return playback;
}

/* Damon V. Caskey - 2026-11-15: verify target sizing, GPU identity and failure. */
static void test_composition(s_movie_playback *playback, s_screen *screen)
{
    unsigned before;
    int index;
    assert(playback->resample == DC_RESAMPLE_MOVIE);
    assert(playback->frame_generation == 1);
    assert(movie_playback_draw_to_screen(screen, playback->index));
    assert(captured_source.owner == playback);
    assert(captured_source.generation == 1);
    assert(captured_source.width == 8 && captured_source.height == 6);
    assert(captured_source.y == playback->current_frame->lum);
    assert(captured_source.u == playback->current_frame->cr);
    assert(captured_source.v == playback->current_frame->cb);
    assert(captured_draw.width == screen->width);
    assert(captured_draw.height == screen->height);
    assert(captured_draw.gamma == 0 && captured_draw.brightness == 0);
    assert(captured_draw.resample == DC_RESAMPLE_MOVIE);
    assert(playback->current_frame_presented && !playback->frame_dirty);

    assert(movie_playback_set_width(playback, MOVIE_SIZE_NATIVE));
    assert(movie_playback_set_height(playback, 5));
    assert(movie_playback_set_offset_x(playback, INT_MIN));
    assert(movie_playback_set_offset_y(playback, INT_MAX));
    assert(movie_playback_set_black_filter(playback, 1));
    assert(movie_playback_set_resample(playback, DC_RESAMPLE_LANCZOS_3));
    assert(movie_playback_draw_to_screen(screen, playback->index));
    assert(captured_draw.width == 8 && captured_draw.height == 5);
    assert(captured_draw.x == INT_MIN && captured_draw.y == INT_MAX);
    assert(captured_draw.black_filter == 1);
    assert(captured_draw.resample == DC_RESAMPLE_LANCZOS_3);
    assert(captured_source.generation == 1);

    /* Large clipped rectangles need no intermediate RGB allocation. */
    assert(movie_playback_set_width(playback, INT_MAX));
    assert(movie_playback_set_height(playback, INT_MAX));
    assert(movie_playback_draw_to_screen(screen, playback->index));
    assert(captured_draw.width == INT_MAX && captured_draw.height == INT_MAX);
    assert(!movie_playback_set_width(playback, (uint64_t)INT_MAX + 1));
    assert(!movie_playback_set_height(playback, (uint64_t)INT_MAX + 1));

    gpu_result = 0;
    playback->frame_dirty = 1;
    playback->current_frame_presented = 0;
    assert(!movie_playback_draw_to_screen(screen, playback->index));
    assert(playback->frame_dirty && !playback->current_frame_presented);
    for(index = 0; index < screen->width * screen->height * 4; ++index)
        assert(screen->data[index] == 0xA5);
    gpu_result = 1;
    before = draw_calls;
    screen->pixelformat = PIXEL_8;
    assert(!movie_playback_draw_to_screen(screen, playback->index));
    assert(draw_calls == before);
    screen->pixelformat = PIXEL_32;
    assert(!movie_playback_draw_to_screen(screen, -1));
    assert(!movie_playback_draw_to_screen(screen, MOVIE_CHANNEL_COUNT));
}

/* Damon V. Caskey - 2026-11-15: retain fullscreen dimensions and capture rules. */
static void test_outputs(s_movie_playback *playback, s_screen *screen)
{
    unsigned before;
    assert(movie_playback_set_width(playback, 0));
    assert(movie_playback_set_height(playback, MOVIE_SIZE_NATIVE));
    playback->frame_dirty = 1;
    playback->current_frame_presented = 0;
    assert(movie_playback_capture_frame(screen, playback->index));
    assert(capture_calls == 1);
    assert(captured_resample == DC_RESAMPLE_LANCZOS_3);
    assert(playback->frame_dirty && !playback->current_frame_presented);
    gpu_result = 0;
    assert(!movie_playback_capture_frame(screen, playback->index));
    assert(playback->frame_dirty && !playback->current_frame_presented);
    gpu_result = 1;
    assert(movie_playback_draw_to_yuv(playback->index));
    assert(captured_mode.display_width == 16);
    assert(captured_mode.display_height == 9);
    assert(!playback->frame_dirty && playback->current_frame_presented);
    assert(movie_playback_draw_to_screen(screen, playback->index));
    assert(captured_draw.width == screen->width && captured_draw.height == 6);
    assert(captured_source.generation == 1);

    before = setup_calls;
    assert(movie_playback_set_resample(playback, DC_RESAMPLE_NEAREST));
    assert(movie_playback_draw_to_yuv(playback->index));
    assert(setup_calls == before && captured_resample == DC_RESAMPLE_NEAREST);
    assert(movie_playback_set_width(playback, 21));
    assert(movie_playback_draw_to_yuv(playback->index));
    assert(setup_calls == before + 1 && captured_mode.display_width == 21);

    playback->next_frame = yuv_frame_create(8, 6);
    assert(playback->next_frame);
    movie_playback_promote_next_frame(playback);
    assert(movie_playback_draw_to_yuv(playback->index));
    assert(captured_source.generation == 2);
    assert(movie_playback_draw_to_screen(screen, playback->index));
    assert(captured_source.generation == 2);
}

/* Damon V. Caskey - 2026-11-15: capture must not acknowledge a pending frame. */
static void test_presentation_ack(s_movie_playback *playback, s_screen *screen)
{
    int terminal = 0;
    uint64_t generation = playback->frame_generation;
    playback->current_frame->timestamp = 0;
    playback->current_frame_presented = 0;
    playback->frame_dirty = 1;
    playback->next_frame = yuv_frame_create(8, 6);
    assert(playback->next_frame);
    playback->next_frame->timestamp = UINT64_C(10000000);
    assert(movie_playback_capture_frame(screen, playback->index));
    movie_playback_poll_frames(playback, UINT64_C(20000000), &terminal);
    assert(playback->frame_generation == generation);
    assert(!terminal && playback->next_frame);
    gpu_result = 0;
    assert(!movie_playback_draw_to_screen(screen, playback->index));
    movie_playback_poll_frames(playback, UINT64_C(20000000), &terminal);
    assert(playback->frame_generation == generation);
    gpu_result = 1;
    assert(movie_playback_draw_to_screen(screen, playback->index));
    movie_playback_poll_frames(playback, UINT64_C(20000000), &terminal);
    assert(playback->frame_generation == generation + 1);
    assert(!playback->current_frame_presented && playback->frame_dirty);
}

/* Damon V. Caskey - 2026-11-15: validate real script selectors and property maps. */
static void test_script_resample(s_movie_playback *playback)
{
    ScriptVariant args[3] = { 0 }, result = { 0 };
    ScriptVariant *arguments[] = { &args[0], &args[1], &args[2] };
    ScriptVariant *returned = &result;
    const VARTYPE rejected_types[] = { VT_DECIMAL, VT_UINTEGER64, VT_PTR, VT_EMPTY };
    s_movie_playback snapshot;
    unsigned index;
    int selector;
    args[0].vt = VT_PTR;
    args[0].ptrVal = playback;
    args[1].vt = VT_INTEGER;
    args[1].lVal = MOVIE_PROPERTY_RESAMPLE;
    args[2].vt = VT_INTEGER;
    assert(movie_playback_set_paused(playback, 1));
    for(selector = DC_RESAMPLE_NEAREST; selector < DC_RESAMPLE_END; ++selector)
    {
        args[2].lVal = selector;
        returned = &result;
        assert(SUCCEEDED(openbor_movie_set_property(arguments, &returned, 3)));
        returned = &result;
        assert(SUCCEEDED(openbor_movie_get_property(arguments, &returned, 2)));
        assert(result.vt == VT_INTEGER && result.lVal == selector);
    }
    assert(playback->paused);
    snapshot = *playback;
    for(index = 0; index < sizeof(rejected_types) / sizeof(rejected_types[0]); ++index)
    {
        args[2].vt = rejected_types[index];
        args[2].ullVal = 1;
        returned = &result;
        assert(FAILED(openbor_movie_set_property(arguments, &returned, 3)));
        assert(!memcmp(playback, &snapshot, sizeof(snapshot)));
        ++rejected_selectors;
    }
    args[2].vt = VT_INTEGER;
    args[2].lVal = -1;
    returned = &result;
    assert(FAILED(openbor_movie_set_property(arguments, &returned, 3)));
    ++rejected_selectors;
    args[2].lVal = DC_RESAMPLE_END;
    returned = &result;
    assert(FAILED(openbor_movie_set_property(arguments, &returned, 3)));
    ++rejected_selectors;
    args[2].vt = VT_STR;
    args[2].strVal = StrCache_CreateNewFrom("1");
    returned = &result;
    assert(FAILED(openbor_movie_set_property(arguments, &returned, 3)));
    ++rejected_selectors;
    ScriptVariant_Clear(&args[2]);
    assert(playback->resample == DC_RESAMPLE_LANCZOS_3);
    assert(!memcmp(playback, &snapshot, sizeof(snapshot)));
    assert(!movie_playback_set_resample(playback, (e_drawmethod_resample)-1));
    assert(!movie_playback_set_resample(playback, DC_RESAMPLE_END));
    assert(MOVIE_PROPERTY_RESAMPLE == MOVIE_PROPERTY_WIDTH + 1);

    args[0].vt = VT_STR;
    args[0].strVal = StrCache_CreateNewFrom("MOVIE_PROPERTY_RESAMPLE");
    returned = &result;
    assert(SUCCEEDED(openbor_transconst(arguments, &returned, 1)));
    assert(result.vt == VT_INTEGER && result.lVal == MOVIE_PROPERTY_RESAMPLE);
    ScriptVariant_Clear(&args[0]);
    ScriptVariant_Clear(&result);
}

/* Damon V. Caskey - 2026-11-15: metadata changes invalidate retained frame data. */
static void test_metadata_and_recycle(s_movie_playback *playback, s_screen *screen)
{
    unsigned before = release_calls;
    unsigned setup_before;
    s_movie_decoder *decoder = calloc(1, sizeof(*decoder));
    assert(decoder);
    movie_playback_pool.decoder[playback->index] = decoder;
    decoder->source_id = -1;
    decoder->channel = playback->index;
    decoder_mode.width = 40000;
    decoder_mode.height = 2;
    decoder_mode.display_width = 40000;
    decoder_mode.display_height = 2;
    assert(movie_playback_poll_decoder(playback) == 1);
    assert(playback->video_mode.width == 40000);
    assert(!playback->current_frame);
    assert(movie_playback_draw_to_screen(screen, playback->index));
    assert(!movie_playback_capture_frame(screen, playback->index));
    movie_playback_stop(playback);
    assert(release_calls == before + 1 && released_owner == playback);
    assert(playback->frame_generation == 0);
    assert(playback->resample == DC_RESAMPLE_MOVIE);

    decoder_mode = (yuv_video_mode){ 8, 6, 16, 9 };
    setup_before = setup_calls;
    assert(fixture_playback(playback->index) == playback);
    assert(movie_playback_draw_to_yuv(playback->index));
    assert(captured_source.generation == 1 && captured_source.owner == playback);
    assert(setup_calls == setup_before + 1);
    /* A restored presenter receives the retained source even without new pixels. */
    assert(movie_playback_draw_to_yuv(playback->index));
    assert(captured_source.generation == 1);
}

/* Damon V. Caskey - 2026-11-15: execute production controller integration cases. */
int main(void)
{
    s_screen *screen = fixture_screen(19, 11);
    s_movie_playback *playback;
    s_movie_playback *second;
    assert(movie_playback_init());
    playback = fixture_playback(0);
    test_composition(playback, screen);
    test_outputs(playback, screen);
    test_presentation_ack(playback, screen);
    test_script_resample(playback);
    test_metadata_and_recycle(playback, screen);
    second = fixture_playback(1);
    assert(movie_playback_draw_to_screen(screen, second->index));
    assert(captured_source.owner == second && second != playback);
    assert(captured_source.generation == 1);
    movie_playback_shutdown();
    free(screen);
    StrCache_Clear();
    printf("Movie GPU controller: %u compositions, %u captures, %u rejected script selectors; all contracts passed.\n",
           draw_calls, capture_calls, rejected_selectors);
    return 0;
}
