/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */
#if ANDROID

// CRxTRDude - changed the directory for neatness.
#include "android/app/jni/openbor/video.c"

#else

#include "sdlport.h"
#include "types.h"
#include "video.h"
#include "vga.h"
#include "opengl.h"
#include "savedata.h"
#include "gfx.h"
#include "videocommon.h"
#include "timer.h"
#include "gpu.h"
#include "engine_profile.h"

SDL_Window *window = NULL;
s_videomodes stored_videomodes;
yuv_video_mode stored_yuv_mode;
int yuv_mode = 0;
char windowTitle[MAX_LABEL_LEN] = {"OpenBOR"};
int stretch = 0;
int opengl = 0; // OpenGL backend currently in use?
int nativeWidth, nativeHeight; // monitor resolution used in fullscreen mode

/*
 * Damon V. Caskey
 * 2026-11-13
 * Initialize SDL and expose an unavailable GL library in the startup log.
 * Later video initialization reports the actual selected rendering path.
 */
void initSDL()
{
	SDL_DisplayMode video_info;
	char refresh_rate[32];
	int init_flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC;

	SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");

	if(SDL_Init(init_flags) < 0)
	{
		printf("SDL Failed to Init!!!! (%s)\n", SDL_GetError());
		borExit(0);
	}
	SDL_ShowCursor(SDL_DISABLE);

#ifdef LOADGL
	if(SDL_GL_LoadLibrary(NULL) < 0)
	{
		gpu_log("WARNING: OpenGL library loading failed: %s. Required native RGB32 modes cannot initialize without GPU rendering.", SDL_GetError());
	}
#endif

	SDL_GetCurrentDisplayMode(0, &video_info);
	nativeWidth = video_info.w;
	nativeHeight = video_info.h;
	if(video_info.refresh_rate > 0)
	{
		snprintf(refresh_rate, sizeof(refresh_rate), "%d Hz", video_info.refresh_rate);
	}
	else
	{
		snprintf(refresh_rate, sizeof(refresh_rate), "Unavailable");
	}
	gpu_log("Display\n"
	        "\t%-26s\t%d x %d\n"
	        "\t%-26s\t%d-bit\n"
	        "\t%-26s\t%s\n",
	        "Desktop resolution:", nativeWidth, nativeHeight,
	        "Desktop color depth:", SDL_BITSPERPIXEL(video_info.format),
	        "Refresh rate:", refresh_rate);
}

void video_set_window_title(const char* title)
{
	if(window) SDL_SetWindowTitle(window, title);
	strncpy(windowTitle, title, sizeof(windowTitle)-1);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Prepare the desktop OpenGL window and retain its windowed position across
 * fullscreen changes. The caller owns context preservation and recreation.
 */
int SetVideoMode(int w, int h)
{
    int flags = SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS |
                SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
    static int last_x = SDL_WINDOWPOS_UNDEFINED;
    static int last_y = SDL_WINDOWPOS_UNDEFINED;

    /* Retain the location before entering fullscreen or resizing the window. */
    if(savedata.fullscreen)
    {
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }
    if(window && !(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP))
    {
        SDL_GetWindowPosition(window, &last_x, &last_y);
    }

    /* Every supported desktop mode uses the same OpenGL window family. */
    if(window)
    {
        if(savedata.fullscreen)
        {
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        }
        else
        {
#ifndef WIN
            if(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP)
            {
                SDL_HideWindow(window);
            }
#endif
            SDL_SetWindowFullscreen(window, 0);
            SDL_SetWindowSize(window, w, h);
            SDL_SetWindowPosition(window, last_x, last_y);
            SDL_ShowWindow(window);
        }
    }
    else
    {
        window = SDL_CreateWindow(windowTitle, last_x, last_y, w, h, flags);
        if(!window)
        {
            gpu_log("ERROR: Failed to create the OpenGL presentation window: %s.", SDL_GetError());
            return 0;
        }
        if(!savedata.fullscreen)
        {
            SDL_GetWindowPosition(window, &last_x, &last_y);
        }
    }
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * 2026-11-15 - Report the sole supported RGB32 scene format.
 * 2026-11-16 - Identify final-scene output filters as GPU work.
 * Describe the menu settings selected for this initialization. The first
 * snapshot records Starting Values; later snapshots describe reinitialization
 * without implying that settings remained fixed throughout the session.
 */
static void video_log_values(const s_videomodes *mode)
{
	static int starting_values_reported;
	char fps_limit[48], scale[80];
	const char *filter = "Disabled";
	int filter_active = savedata.swfilter && (savedata.hwscale >= 2.0 || savedata.fullscreen);

	/* Decode stored options using the same meanings shown in the video menu.
	 * Fullscreen sizing is automatic even when a window scale is retained. */
	switch(savedata.fpslimit)
	{
	case 0: snprintf(fps_limit, sizeof(fps_limit), "Disabled"); break;
	case 1: snprintf(fps_limit, sizeof(fps_limit), "VSync (display refresh)"); break;
	case 2: snprintf(fps_limit, sizeof(fps_limit), "200 FPS"); break;
	case 3: snprintf(fps_limit, sizeof(fps_limit), "500 FPS"); break;
	default: snprintf(fps_limit, sizeof(fps_limit), "Unknown (setting %d)", savedata.fpslimit); break;
	}
	if(savedata.fullscreen)
	{
		snprintf(scale, sizeof(scale), "Automatic (stored window scale: %.2fx)", (double)savedata.hwscale);
	}
	else
	{
		snprintf(scale, sizeof(scale), "%.2fx", (double)savedata.hwscale);
	}
	if(savedata.swfilter)
	{
		filter = savedata.swfilter > 0 && savedata.swfilter < BLITTER_MAX &&
		         GfxBlitterNames[savedata.swfilter] ? GfxBlitterNames[savedata.swfilter] : "Unknown";
	}
	/* Keep authored scene dimensions separate from the GPU filter target. */
	gpu_log("%s\n"
	        "\tMenu options can change these values during play.\n"
	        "\t%-26s\t%s\n"
	        "\t%-26s\t%s\n"
	        "\t%-26s\t%s\n"
	        "\t%-26s\t%s\n"
	        "\t%-26s\t%s\n"
	        "\t%-26s\t%s\n"
	        "\t%-26s\t%d x %d\n"
	        "\t%-26s\t%s (%d bytes per pixel)\n",
	        starting_values_reported ? "Values at Video Reinitialization" : "Starting Values",
	        "Video backend requested:", savedata.usegl ? "OpenGL" : "SDL",
	        "FPS limit:", fps_limit,
	        "Display mode:", savedata.fullscreen ? "Fullscreen" : "Windowed",
	        "Display scale:", scale,
	        "Output filter selected:", filter,
	        "GPU output filter:", filter_active ? "Active" : "Inactive",
	        "Scene resolution:", mode->hRes, mode->vRes,
	        "Scene color format:", "RGB32", mode->pixel);
	starting_values_reported = 1;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Require native RGB32 rendering for every desktop presentation mode. GPU
 * output filters run after scene composition and never select CPU drawing.
 * Invalid format requests leave the existing window and context intact.
 */
int video_set_mode(s_videomodes videomodes)
{
    int report_mode = videomodes.hRes > 0 && videomodes.vRes > 0;

    /* Shutdown has no image format; all nonempty scenes must be RGB32. */
    if((videomodes.hRes != 0 || videomodes.vRes != 0) && videomodes.pixel != 4)
    {
        gpu_log("ERROR: Video mode rejected: OpenBOR scenes require RGB32 (4 bytes per pixel), received %d.", videomodes.pixel);
        return 0;
    }
    if(videomodes.hRes < 0 || videomodes.vRes < 0 ||
       ((videomodes.hRes == 0) != (videomodes.vRes == 0)))
    {
        gpu_log("ERROR: Video mode rejected: invalid scene dimensions %d x %d.", videomodes.hRes, videomodes.vRes);
        return 0;
    }
    if(report_mode && (savedata.swfilter < BLITTER_NONE || savedata.swfilter >= BLITTER_MAX))
    {
        gpu_log("ERROR: Video mode rejected: unknown GPU output filter %d.", savedata.swfilter);
        return 0;
    }

    /* Publish the previous timing window before starting new diagnostics. */
    engine_profile_reset("video_mode");
    if(report_mode)
    {
        gpu_log("========== GRAPHICS INITIALIZATION ==========");
        video_log_values(&videomodes);
        if(!savedata.usegl)
        {
            gpu_log("RGB32 scene drawing requires the native OpenGL renderer.\n"
                    "\tThe stored SDL preference does not select CPU scene rendering.");
        }
    }
    stored_videomodes = videomodes;
    yuv_mode = 0;

    /* Context teardown preserves genuine scene owners, never filter scratch. */
    if(videomodes.hRes == 0 && videomodes.vRes == 0)
    {
        video_gl_shutdown();
        return 0;
    }
    gpu_set_native_required(1);
    savedata.usegl = 1;
    videomodes = setupPreBlitProcessing(videomodes);

    /* Both native composition and its presenter are required. Failure rejects
     * this mode and retains the strict drawing policy without a CPU retry. */
    if(videomodes.pixel && video_gl_set_mode(videomodes) && gpu_is_active())
    {
        video_stretch(savedata.stretch);
        if(report_mode) gpu_log("========== END GRAPHICS INITIALIZATION ==========");
        return 1;
    }
    gpu_log("ERROR: Required native RGB32 presentation could not initialize.\n"
            "\tVideo mode rejected; CPU scene rendering will not be selected.");
    video_gl_shutdown();
    if(report_mode) gpu_log("========== END GRAPHICS INITIALIZATION ==========");
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Finish movie output without recreating a healthy native context. The RGB
 * presenter and scene targets remain available throughout movie playback.
 */
int video_restore_mode(void)
{
    yuv_mode = 0;
    if(opengl && gpu_is_active())
    {
        return 1;
    }
    return video_set_mode(stored_videomodes);
}

void video_fullscreen_flip()
{
	int restore_yuv = yuv_mode;
	savedata.fullscreen ^= 1;
	if(window) video_set_mode(stored_videomodes);
	if(restore_yuv) video_setup_yuv_overlay(&stored_yuv_mode);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Retain the selected 200/500 FPS limiter and measure its actual elapsed wait.
 * The deadline and sleep loop are unchanged so profiling does not alter pacing.
 */
void FramerateDelay()
{
    static u64 last_time = 0;
    ep_scope limiter_scope = engine_profile_span_begin();

    if (savedata.fpslimit < 2 || savedata.fpslimit > 3)
    {
        engine_profile_span_end(limiter_scope, EP_SPAN_LIMITER);
        return;
    }
    int fps_limit = savedata.fpslimit == 3 ? 500 : 200;

    /* This minimum frame interval only sleeps when the preceding work has
     * finished before its deadline. Slow frames receive no additional delay. */
    u64 target_time = last_time + 1000000/fps_limit;
    u64 current_time = timer_uticks();
    while (current_time < target_time)
    {
        usleep(target_time - current_time);
        current_time = timer_uticks();
    }
    last_time = current_time;
    engine_profile_span_end(limiter_scope, EP_SPAN_LIMITER);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Present the resident RGB32 scene through optional GPU output filtering.
 * Rejected frames stop before any CPU readback, conversion, or retry.
 */
int video_copy_screen(s_screen *src)
{
    if(!src || src->pixelformat != PIXEL_32)
    {
        return 0;
    }
    if(opengl && video_gl_copy_gpu_screen(src))
    {
        yuv_mode = 0;
        return 1;
    }

    /* The existing once-per-operation diagnostic reports a rejected frame. */
    gpu_allow_cpu_operation(src, GPU_OPERATION_PRESENT);
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Start desktop launcher composition in its ordinary native RGB32 screen.
 * The shared menu uses this same boundary for Android's private GPU target.
 */
int video_launcher_begin(s_screen *target)
{
    static int warned;
    int okay = target && target->pixelformat == PIXEL_32 && opengl &&
               gpu_is_active() && gpu_clear_screen(target);
    if(!okay && !warned)
    {
        gpu_log("WARNING: Native launcher frame unavailable; frame preparation skipped without CPU retry.");
    }
    warned = !okay;
    return okay;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Keep launcher backgrounds and previews in their original opaque native
 * draw order. Rejection reaches the existing diagnostic without CPU drawing.
 */
int video_launcher_screen(s_screen *target, s_screen *source, int x, int y)
{
    if(gpu_draw_screen(target, source, x, y, NULL)) return 1;
    gpu_allow_cpu_operation(target, GPU_OPERATION_SCREEN);
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Compose the launcher's opaque music-information box through native geometry.
 */
int video_launcher_box(s_screen *target, int x, int y, int width, int height,
                       unsigned color)
{
    if(gpu_draw_box(target, x, y, width, height, color, NULL)) return 1;
    gpu_allow_cpu_operation(target, GPU_OPERATION_BOX);
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Clear the active GPU presenter without creating a separate SDL renderer.
 */
void video_clearscreen(void)
{
    if(opengl) video_gl_clearscreen();
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Store aspect handling for the next GPU presentation quad.
 */
void video_stretch(int enable)
{
    stretch = enable;
}

void video_set_color_correction(int gm, int br)
{
	if(opengl) video_gl_set_color_correction(gm, br);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Configure native movie layout while retaining normal scene drawing. Failed
 * setup leaves the current presenter unchanged and never selects CPU work.
 */
int video_setup_yuv_overlay(const yuv_video_mode *mode)
{
    if(!opengl || !video_gl_setup_yuv_overlay(mode))
    {
        gpu_log("WARNING: Native movie presentation is unavailable; playback output was not changed.");
        return 0;
    }
    stored_yuv_mode = *mode;
    yuv_mode = 1;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Submit the same retained source used by movie-to-screen composition. Plane
 * upload identity is independent of which output last presented the frame.
 */
int video_prepare_yuv_frame(const gpu_yuv_source *source,
                            e_drawmethod_resample resample)
{
    if(!opengl || !video_gl_prepare_yuv_frame(source, resample))
    {
        return 0;
    }
    yuv_mode = 1;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Prepare an explicitly requested screenshot with hardware movie colors.
 * This neither swaps a window nor acknowledges a playback presentation.
 */
int video_capture_yuv_frame(s_screen *screen, const gpu_yuv_source *source,
                            e_drawmethod_resample resample)
{
    return opengl && video_gl_capture_yuv_frame(screen, source, resample);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Complete native movie presentation without switching rendering modes.
 */
int video_display_yuv_frame(void)
{
    return opengl && video_gl_display_yuv_frame();
}

int video_current_refresh_rate()
{
    SDL_DisplayMode display_mode;
    if (SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window), &display_mode) != 0)
        return 60;
    return display_mode.refresh_rate;
}

void vga_vwait(void)
{
	static int prevtick = 0;
	int now = SDL_GetTicks();
	int wait = 1000/60 - (now - prevtick);
	if (wait>0)
	{
		SDL_Delay(wait);
	}
	else SDL_Delay(1);
	prevtick = now;
}

#endif
