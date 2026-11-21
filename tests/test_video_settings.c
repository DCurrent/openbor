/*
 * Damon V. Caskey
 * 2026-11-20
 * Regression coverage for persisted display controls and production menu
 * actions. No scene renderer, driver or SDL window is needed for these tests.
 */
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "video_settings.h"
#include "types.h"
#include "gfx.h"

#ifdef ANDROID
#define TEST_ANDROID 1
#define TEST_PLATFORM "Android"
#else
#define TEST_ANDROID 0
#define TEST_PLATFORM "desktop"
#endif

/* Dependencies unrelated to display settings remain intentionally small. The
 * extracted clearsettings() still supplies the actual production defaults. */
#define MAX_BUFFER_LEN 512
#define CHEAT_OPTIONS_ALL_MENU 1
static struct { int cheats; } global_config;
s_savedata savedata;
static s_videomodes videomodes;
static char settings_directory[MAX_BUFFER_LEN];
static int mode_calls;
static int fullscreen_calls;
static int correction_calls;
static int wait_calls;
static int stretch_calls;
static int last_gamma;
static int last_brightness;
static int last_stretch;

static void clearbuttons(int player)
{
    assert(player >= 0 && player < MAX_PLAYERS);
    memset(savedata.keys[player], 0, sizeof(savedata.keys[player]));
}

static void getBasePath(char *path, const char *folder, int create)
{
    assert(strcmp(folder, "Saves") == 0);
    (void)create;
    assert(strlen(settings_directory) + 2 < MAX_BUFFER_LEN);
    strcpy(path, settings_directory);
    strcat(path, "/");
}

static void getPakName(char *name, int extension)
{
    assert(extension == 4);
    strcpy(name, "module.cfg");
}

static int fileExists(const char *path)
{
    return access(path, F_OK) == 0;
}

static int video_set_mode(s_videomodes mode)
{
    assert(mode.hRes == videomodes.hRes && mode.vRes == videomodes.vRes);
    ++mode_calls;
    return 1;
}

static void video_fullscreen_flip(void)
{
    ++fullscreen_calls;
    savedata.fullscreen = !savedata.fullscreen;
}

static void vga_vwait(void)
{
    ++wait_calls;
}

static void set_color_correction(int gamma, int brightness)
{
    ++correction_calls;
    last_gamma = gamma;
    last_brightness = brightness;
}

static void video_stretch(int stretch)
{
    ++stretch_calls;
    last_stretch = stretch;
}

void loadfromdefault(void);
#include "video_settings_engine.inc"

static void reset_observations(void)
{
    mode_calls = fullscreen_calls = correction_calls = wait_calls = stretch_calls = 0;
    last_gamma = last_brightness = last_stretch = INT_MIN;
}

static s_savedata example_settings(void)
{
    int player, button;
    memset(&savedata, 0, sizeof(savedata));
    clearsettings();
    savedata.gamma = -40;
    savedata.brightness = 24;
    savedata.windowpos = 5;
    savedata.soundvol = 63;
    savedata.musicvol = 71;
    savedata.effectvol = 82;
    savedata.fullscreen = 0;
    savedata.stretch = 1;
    savedata.fpslimit = 3;
    savedata.swfilter = BLITTER_SCANLINES;
    savedata.hwfilter = VIDEO_SCALING_BILINEAR;
    savedata.hwscale = 3.25f;
    savedata.usegl = VIDEO_SETTINGS_NATIVE;
    savedata.screen[0][0] = 937;
    savedata.screen[0][1] = 619;
    for(player = 0; player < MAX_PLAYERS; ++player)
    {
        savedata.joyrumble[player] = player & 1;
        for(button = 0; button < MAX_BTN_NUM; ++button)
            savedata.keys[player][button] = 1000 + player * 100 + button;
    }
#ifdef ANDROID
    savedata.is_touchpad_vibration_enabled = 1;
#endif
    memset(&videomodes, 0, sizeof(videomodes));
    videomodes.hRes = 320;
    videomodes.vRes = 240;
    videomodes.pixel = PIXEL_32;
    videomodes.filter = (char)savedata.swfilter;
    reset_observations();
    return savedata;
}

static void assert_settings_equal(const s_savedata *expected)
{
    assert(memcmp(&savedata, expected, sizeof(savedata)) == 0);
}

static void test_saved_layout(void)
{
    /* These numbers describe the old raw .cfg format, not the new header. */
    assert(sizeof(int) == 4 && sizeof(float) == 4);
    assert(offsetof(s_savedata, gamma) == 4);
    assert(offsetof(s_savedata, brightness) == 8);
    assert(offsetof(s_savedata, windowpos) == 36);
    assert(offsetof(s_savedata, keys) == 40);
    assert(offsetof(s_savedata, swfilter) == 272);
    assert(offsetof(s_savedata, fullscreen) == 288);
    assert(offsetof(s_savedata, stretch) == 292);
    assert(offsetof(s_savedata, fpslimit) == 304);
    assert(offsetof(s_savedata, usegl) == 308);
    assert(offsetof(s_savedata, hwscale) == 312);
    assert(offsetof(s_savedata, hwfilter) == 316);
#ifdef ANDROID
    assert(offsetof(s_savedata, is_touchpad_vibration_enabled) == 320);
    assert(sizeof(savedata) == 324);
#else
    assert(sizeof(savedata) == 320);
#endif
}

static void test_defaults(void)
{
    memset(&savedata, 0, sizeof(savedata));
    clearsettings();
    assert(savedata.usegl == VIDEO_SETTINGS_NATIVE);
    assert(savedata.swfilter == BLITTER_NONE);
    assert(savedata.hwfilter == (TEST_ANDROID ? VIDEO_SCALING_NEAREST : VIDEO_SCALING_HIGH_QUALITY));
    assert(savedata.hwscale == (TEST_ANDROID ? 0.0f : 1.0f));
    assert(savedata.gamma == 0 && savedata.brightness == 0);
    assert(savedata.stretch == 0 && savedata.fpslimit == 1);
}

static void test_normalization(void)
{
    static const struct { int backend, old_filter, desktop, android; } migrations[] = {
        {0, 0, VIDEO_SCALING_BILINEAR, VIDEO_SCALING_BILINEAR},
        {0, 1, VIDEO_SCALING_NEAREST, VIDEO_SCALING_NEAREST},
        {1, 0, VIDEO_SCALING_HIGH_QUALITY, VIDEO_SCALING_BILINEAR},
        {1, 1, VIDEO_SCALING_HIGH_QUALITY, VIDEO_SCALING_NEAREST},
        {0, -8, VIDEO_SCALING_NEAREST, VIDEO_SCALING_NEAREST},
        {1, INT_MAX, VIDEO_SCALING_HIGH_QUALITY, VIDEO_SCALING_NEAREST},
        {VIDEO_SETTINGS_NATIVE, 0, VIDEO_SCALING_BILINEAR, VIDEO_SCALING_BILINEAR},
        {VIDEO_SETTINGS_NATIVE, 1, VIDEO_SCALING_NEAREST, VIDEO_SCALING_NEAREST},
        {VIDEO_SETTINGS_NATIVE, 2, VIDEO_SCALING_HIGH_QUALITY, VIDEO_SCALING_HIGH_QUALITY},
        {VIDEO_SETTINGS_NATIVE, -1, VIDEO_SCALING_HIGH_QUALITY, VIDEO_SCALING_NEAREST},
        {VIDEO_SETTINGS_NATIVE, INT_MAX, VIDEO_SCALING_HIGH_QUALITY, VIDEO_SCALING_NEAREST}
    };
    static const struct { float input, desktop, android; } scales[] = {
        {-100.0f, .25f, 0.0f}, {0.0f, .25f, 0.0f}, {.125f, .25f, .25f},
        {.25f, .25f, .25f}, {.75f, .75f, .75f}, {1.0f, 1.0f, 1.0f},
        {1.625f, 1.625f, 1.625f}, {4.0f, 4.0f, 4.0f},
        {7.75f, 7.75f, 7.75f}, {8.0f, 8.0f, 8.0f},
        {8.25f, 8.0f, 8.0f}, {1.0e30f, 8.0f, 8.0f},
        {INFINITY, 1.0f, 0.0f}, {-INFINITY, 1.0f, 0.0f}, {NAN, 1.0f, 0.0f}
    };
    size_t i;
    int android;
    video_settings_normalize(NULL, TEST_ANDROID);
    for(android = 0; android <= 1; ++android)
    {
        for(i = 0; i < sizeof(migrations) / sizeof(migrations[0]); ++i)
        {
            s_savedata expected = example_settings();
            savedata.usegl = migrations[i].backend;
            savedata.hwfilter = migrations[i].old_filter;
            expected.hwfilter = android ? migrations[i].android : migrations[i].desktop;
            video_settings_normalize(&savedata, android);
            assert_settings_equal(&expected);
            video_settings_normalize(&savedata, android);
            assert_settings_equal(&expected);
        }
        for(i = 0; i < sizeof(scales) / sizeof(scales[0]); ++i)
        {
            s_savedata expected = example_settings();
            savedata.hwscale = scales[i].input;
            expected.hwscale = android ? scales[i].android : scales[i].desktop;
            video_settings_normalize(&savedata, android);
            assert_settings_equal(&expected);
        }
    }
    assert(VIDEO_SCALING_BILINEAR == 0 && VIDEO_SCALING_NEAREST == 1);
    assert(strcmp(video_scaling_filter_name(VIDEO_SCALING_BILINEAR), "Bilinear") == 0);
    assert(strcmp(video_scaling_filter_name(VIDEO_SCALING_NEAREST), "Nearest") == 0);
    assert(strcmp(video_scaling_filter_name(VIDEO_SCALING_HIGH_QUALITY), "High Quality") == 0);
    assert(strcmp(video_scaling_filter_name(-1), "Unknown") == 0);
    assert(strcmp(video_scaling_filter_name(INT_MAX), "Unknown") == 0);
}

static void settings_path(char *path, const char *name)
{
    getBasePath(path, "Saves", 0);
    assert(strlen(path) + strlen(name) < MAX_BUFFER_LEN);
    strcat(path, name);
}

static void assert_saved_file(const char *name, const s_savedata *expected)
{
    char path[MAX_BUFFER_LEN];
    s_savedata stored;
    FILE *file;
    settings_path(path, name);
    file = fopen(path, "rb");
    assert(file);
    assert(fread(&stored, 1, sizeof(stored), file) == sizeof(stored));
    assert(fgetc(file) == EOF);
    assert(fclose(file) == 0);
    assert(memcmp(&stored, expected, sizeof(stored)) == 0);
}

static void write_legacy_file(const char *name, int backend, int filter)
{
    char path[MAX_BUFFER_LEN];
    unsigned char legacy[TEST_ANDROID ? 324 : 320] = {0};
    const unsigned compatible = COMPATIBLEVERSION;
    const int brightness = 48, gamma = -32, effect = BLITTER_DOTMATRIX;
    const float scale = 3.25f;
    FILE *file;
    /* Known old offsets prevent a changed production struct from creating a
     * matching but incompatible fixture. Original settings use native endian. */
    memcpy(legacy, &compatible, 4);
    memcpy(legacy + 4, &gamma, 4);
    memcpy(legacy + 8, &brightness, 4);
    memcpy(legacy + 272, &effect, 4);
    memcpy(legacy + 308, &backend, 4);
    memcpy(legacy + 312, &scale, 4);
    memcpy(legacy + 316, &filter, 4);
    settings_path(path, name);
    file = fopen(path, "wb");
    assert(file);
    assert(fwrite(legacy, 1, sizeof(legacy), file) == sizeof(legacy));
    assert(fclose(file) == 0);
}

static void test_saved_round_trips(void)
{
    static const float desktop_scales[] = {.25f, 1.0f, 1.25f, 2.0f, 4.0f, 7.75f, 8.0f};
    char path[MAX_BUFFER_LEN];
    int filter, effect, backend, defaults;
    size_t scale;
    for(filter = 0; filter < VIDEO_SCALING_COUNT; ++filter)
    {
        for(effect = BLITTER_NONE; effect < BLITTER_MAX; ++effect)
        {
            for(scale = 0; scale < sizeof(desktop_scales) / sizeof(desktop_scales[0]); ++scale)
            {
                s_savedata expected = example_settings();
                expected.hwfilter = filter;
                expected.swfilter = effect;
                expected.hwscale = desktop_scales[scale];
                savedata = expected;
                savesettings();
                assert_saved_file("module.cfg", &expected);
                memset(&savedata, 0x5a, sizeof(savedata));
                loadsettings();
                assert_settings_equal(&expected);
            }
        }
    }

    for(defaults = 0; defaults <= 1; ++defaults)
    {
        {
            s_savedata expected = example_settings();
            expected.hwfilter = TEST_ANDROID ? VIDEO_SCALING_BILINEAR : VIDEO_SCALING_HIGH_QUALITY;
            expected.hwscale = TEST_ANDROID ? 0.0f : 1.0f;
            savedata.usegl = 1;
            savedata.hwscale = NAN;
            if(defaults) saveasdefault();
            else savesettings();
            assert_settings_equal(&expected);
            assert_saved_file(defaults ? "default.cfg" : "module.cfg", &expected);
        }
        for(backend = 0; backend <= 1; ++backend)
        {
            for(filter = 0; filter <= 1; ++filter)
            {
                write_legacy_file(defaults ? "default.cfg" : "module.cfg", backend, filter);
                memset(&savedata, 0x5a, sizeof(savedata));
                if(defaults) loadfromdefault();
                else loadsettings();
                assert(savedata.compatibleversion == COMPATIBLEVERSION);
                assert(savedata.usegl == VIDEO_SETTINGS_NATIVE);
                assert(savedata.hwfilter == (!TEST_ANDROID && backend ? VIDEO_SCALING_HIGH_QUALITY : filter));
                assert(savedata.swfilter == BLITTER_DOTMATRIX);
                assert(savedata.hwscale == 3.25f);
                assert(savedata.gamma == -32 && savedata.brightness == 48);
            }
        }
    }

    {
        s_savedata expected = example_settings();
        expected.hwscale = TEST_ANDROID ? 0.0f : 8.0f;
        expected.hwfilter = VIDEO_SCALING_HIGH_QUALITY;
        savedata = expected;
        saveasdefault();
        assert_saved_file("default.cfg", &expected);
        settings_path(path, "module.cfg");
        assert(unlink(path) == 0);
        memset(&savedata, 0x5a, sizeof(savedata));
        loadsettings();
        assert_settings_equal(&expected);
        memset(&savedata, 0x5a, sizeof(savedata));
        loadfromdefault();
        assert_settings_equal(&expected);
    }
    settings_path(path, "default.cfg");
    assert(unlink(path) == 0);
}

static void test_menu_scale(void)
{
    s_savedata expected = example_settings();
    int quarter;
    savedata.hwscale = expected.hwscale = TEST_ANDROID ? 0.0f : .25f;
    for(quarter = TEST_ANDROID ? 1 : 2; quarter <= 32; ++quarter)
    {
        expected.hwscale = quarter * .25f;
        menu_video_adjust(VIDEO_OPTION_WINDOW_SCALE, 1);
        assert_settings_equal(&expected);
        assert(videomodes.filter == expected.swfilter);
    }
    menu_video_adjust(VIDEO_OPTION_WINDOW_SCALE, 1);
    assert_settings_equal(&expected);
    for(quarter = 31; quarter >= (TEST_ANDROID ? 0 : 1); --quarter)
    {
        expected.hwscale = quarter * .25f;
        menu_video_adjust(VIDEO_OPTION_WINDOW_SCALE, -1);
        assert_settings_equal(&expected);
    }
    menu_video_adjust(VIDEO_OPTION_WINDOW_SCALE, -1);
    assert_settings_equal(&expected);
    assert(correction_calls == 0 && fullscreen_calls == 0 && stretch_calls == 0);
#ifndef ANDROID
    savedata.fullscreen = expected.fullscreen = 1;
    savedata.hwscale = expected.hwscale = 7.75f;
    reset_observations();
    menu_video_adjust(VIDEO_OPTION_WINDOW_SCALE, 1);
    assert_settings_equal(&expected);
    assert(mode_calls == 0);
#endif
}

static void test_menu_filter_independence(void)
{
    s_savedata expected = example_settings();
    int filter;
    savedata.hwscale = expected.hwscale = 1.0f;
    savedata.hwfilter = expected.hwfilter = VIDEO_SCALING_BILINEAR;
    for(filter = 1; filter <= VIDEO_SCALING_COUNT; ++filter)
    {
        expected.hwfilter = filter % VIDEO_SCALING_COUNT;
        menu_video_adjust(VIDEO_OPTION_SCALING_FILTER, 1);
        assert_settings_equal(&expected);
    }
    menu_video_adjust(VIDEO_OPTION_SCALING_FILTER, -1);
    expected.hwfilter = VIDEO_SCALING_HIGH_QUALITY;
    assert_settings_equal(&expected);

    /* Selection persists below the 2x activation threshold. Presentation
     * tests separately check whether the stored final effect is active. */
    savedata.swfilter = expected.swfilter = BLITTER_NONE;
    videomodes.filter = BLITTER_NONE;
    for(filter = 1; filter <= BLITTER_MAX; ++filter)
    {
        expected.swfilter = filter % BLITTER_MAX;
        menu_video_adjust(VIDEO_OPTION_FINAL_FILTER, 1);
        assert_settings_equal(&expected);
        assert(videomodes.filter == expected.swfilter);
    }
    menu_video_adjust(VIDEO_OPTION_FINAL_FILTER, -1);
    expected.swfilter = BLITTER_DOTMATRIX;
    assert_settings_equal(&expected);
    assert(videomodes.filter == BLITTER_DOTMATRIX);
    assert(correction_calls == 0 && fullscreen_calls == 0 && stretch_calls == 0);
}

static void test_menu_remaining_actions(void)
{
    s_savedata expected = example_settings();
    int i;
    menu_video_adjust(VIDEO_OPTION_BRIGHTNESS, 1);
    expected.brightness += 8;
    assert_settings_equal(&expected);
    assert(correction_calls == 1 && last_gamma == expected.gamma && last_brightness == expected.brightness);
    menu_video_adjust(VIDEO_OPTION_GAMMA, -1);
    expected.gamma -= 8;
    assert_settings_equal(&expected);
    assert(correction_calls == 2 && last_gamma == expected.gamma && last_brightness == expected.brightness);
    assert(wait_calls == 2);

    savedata.brightness = expected.brightness = 256;
    menu_video_adjust(VIDEO_OPTION_BRIGHTNESS, 1);
    assert_settings_equal(&expected);
    savedata.brightness = expected.brightness = -256;
    menu_video_adjust(VIDEO_OPTION_BRIGHTNESS, -1);
    assert_settings_equal(&expected);
    savedata.gamma = expected.gamma = 256;
    menu_video_adjust(VIDEO_OPTION_GAMMA, 1);
    assert_settings_equal(&expected);
    savedata.gamma = expected.gamma = -256;
    menu_video_adjust(VIDEO_OPTION_GAMMA, -1);
    assert_settings_equal(&expected);

    menu_video_adjust(VIDEO_OPTION_WINDOW_OFFSET, 1);
    ++expected.windowpos;
    assert_settings_equal(&expected);
    savedata.windowpos = expected.windowpos = 20;
    menu_video_adjust(VIDEO_OPTION_WINDOW_OFFSET, 1);
    assert_settings_equal(&expected);
    savedata.windowpos = expected.windowpos = -2;
    menu_video_adjust(VIDEO_OPTION_WINDOW_OFFSET, -1);
    assert_settings_equal(&expected);

    for(i = 0; i < 4; ++i)
    {
        expected.fpslimit = i;
        menu_video_adjust(VIDEO_OPTION_FPS_LIMIT, 1);
        assert_settings_equal(&expected);
    }
    menu_video_adjust(VIDEO_OPTION_FPS_LIMIT, -1);
    expected.fpslimit = 2;
    assert_settings_equal(&expected);

    savedata.fullscreen = expected.fullscreen = 1;
#ifdef ANDROID
    savedata.hwscale = expected.hwscale = VIDEO_SCALE_AUTOMATIC;
#endif
    menu_video_adjust(VIDEO_OPTION_FULLSCREEN_TYPE, 1);
    expected.stretch = 0;
    assert_settings_equal(&expected);
    assert(stretch_calls == 1 && last_stretch == 0);
}

static void test_menu_zero_direction(void)
{
    static const e_video_option options[] = {
        VIDEO_OPTION_BRIGHTNESS, VIDEO_OPTION_GAMMA, VIDEO_OPTION_WINDOW_OFFSET,
        VIDEO_OPTION_DISPLAY_MODE, VIDEO_OPTION_WINDOW_SCALE,
        VIDEO_OPTION_SCALING_FILTER, VIDEO_OPTION_FINAL_FILTER,
        VIDEO_OPTION_FPS_LIMIT, VIDEO_OPTION_FULLSCREEN_TYPE, VIDEO_OPTION_BACK
    };
    size_t i;
    s_savedata expected = example_settings();
    for(i = 0; i < sizeof(options) / sizeof(options[0]); ++i)
    {
        menu_video_adjust(options[i], 0);
        assert_settings_equal(&expected);
        assert(videomodes.filter == expected.swfilter);
    }
    assert(mode_calls == 0 && fullscreen_calls == 0 && correction_calls == 0 && wait_calls == 0 && stretch_calls == 0);
}

static void test_menu_availability(void)
{
    s_savedata expected = example_settings();
    assert(menu_video_option_enabled(VIDEO_OPTION_SCALING_FILTER));
    assert(menu_video_option_enabled(VIDEO_OPTION_FINAL_FILTER));
    assert(menu_video_option_enabled(VIDEO_OPTION_WINDOW_SCALE));
    assert(!menu_video_option_enabled(VIDEO_OPTION_FULLSCREEN_TYPE));
#ifdef ANDROID
    assert(!menu_video_option_enabled(VIDEO_OPTION_DISPLAY_MODE));
    menu_video_adjust(VIDEO_OPTION_DISPLAY_MODE, 1);
    assert_settings_equal(&expected);
    assert(fullscreen_calls == 0);
    savedata.fullscreen = expected.fullscreen = 1;
    assert(!menu_video_option_enabled(VIDEO_OPTION_FULLSCREEN_TYPE));
    menu_video_adjust(VIDEO_OPTION_FULLSCREEN_TYPE, 1);
    assert_settings_equal(&expected);
    assert(stretch_calls == 0);
    savedata.hwscale = expected.hwscale = VIDEO_SCALE_AUTOMATIC;
    assert(menu_video_option_enabled(VIDEO_OPTION_FULLSCREEN_TYPE));
#else
    assert(menu_video_option_enabled(VIDEO_OPTION_DISPLAY_MODE));
    menu_video_adjust(VIDEO_OPTION_DISPLAY_MODE, 1);
    expected.fullscreen = 1;
    assert_settings_equal(&expected);
    assert(fullscreen_calls == 1);
    assert(!menu_video_option_enabled(VIDEO_OPTION_WINDOW_SCALE));
    assert(menu_video_option_enabled(VIDEO_OPTION_FULLSCREEN_TYPE));
    menu_video_adjust(VIDEO_OPTION_DISPLAY_MODE, -1);
    expected.fullscreen = 0;
    assert_settings_equal(&expected);
    assert(fullscreen_calls == 2);
    assert(menu_video_option_enabled(VIDEO_OPTION_WINDOW_SCALE));
#endif
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    assert(strlen(argv[1]) < sizeof(settings_directory));
    strcpy(settings_directory, argv[1]);
    test_saved_layout();
    test_defaults();
    test_normalization();
    test_saved_round_trips();
    test_menu_scale();
    test_menu_filter_independence();
    test_menu_remaining_actions();
    test_menu_zero_direction();
    test_menu_availability();
    printf("Video controls %s: packed compatibility, migration, persistence and independent menu actions passed.\n", TEST_PLATFORM);
    return 0;
}
