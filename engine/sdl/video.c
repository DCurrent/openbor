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
#include <math.h>
#include "types.h"
#include "video.h"
#include "vga.h"
#include "screen.h"
#include "opengl.h"
#include "savedata.h"
#include "gfxtypes.h"
#include "gfx.h"
#include "pngdec.h"
#include "videocommon.h"
#include "timer.h"
#include "gpu.h"
#include "engine_profile.h"
#include "../resources/OpenBOR_Icon_32x32_png.h"

SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;
s_videomodes stored_videomodes;
yuv_video_mode stored_yuv_mode;
int yuv_mode = 0;
char windowTitle[MAX_LABEL_LEN] = {"OpenBOR"};
int stretch = 0;
int opengl = 0; // OpenGL backend currently in use?
int nativeWidth, nativeHeight; // monitor resolution used in fullscreen mode
int brightness = 0;

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
		gpu_log("WARNING: OpenGL library loading failed: %s. GPU rendering may be unavailable; video initialization will report the selected fallback.", SDL_GetError());
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

static unsigned pixelformats[4] = {SDL_PIXELFORMAT_INDEX8, SDL_PIXELFORMAT_BGR565, SDL_PIXELFORMAT_BGR888, SDL_PIXELFORMAT_ABGR8888};

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-13 - Keep window/presenter creation failures in startup diagnostics.
 * Prepare the SDL window for the selected presentation backend. Keep resource
 * destruction in ownership order, preserving GPU-backed screen contents before
 * replacing a window whose graphics context owns those screens.
 */
int SetVideoMode(int w, int h, int bpp, bool gl)
{
	int flags = SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS;
	static bool last_gl = false;
	static int last_x = SDL_WINDOWPOS_UNDEFINED;
	static int last_y = SDL_WINDOWPOS_UNDEFINED;

	/* Retain the windowed position so leaving fullscreen restores its location. */
	if(savedata.fullscreen)
	{
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}

	if(window && !(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP))
	{
		SDL_GetWindowPosition(window, &last_x, &last_y);
	}

	if (gl)
	{
		// The SDL video backend doesn't support high-quality (sharp bilinear) scaling,
		// so it will produce bad results if it tries to scale by a fractional factor.
		// The results are worse than what we get from upscaling with nearest-neighbor
		// and letting the windowing system upscale the result. So only use the high-DPI
		// flag with the OpenGL backend. Unfortunately, we're stuck with it on Windows,
		// where high-DPI support is controlled by a global hint and the ALLOW_HIGHDPI
		// flag is ignored.
		flags |= SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
	}

	/*
	 * Release texture storage before its renderer, then release the renderer
	 * while its window still exists. Both resources are recreated for the new
	 * mode, even when the window itself can be reused.
	 */
	if(texture)
	{
		SDL_DestroyTexture(texture);
	}
	if(renderer)
	{
		SDL_DestroyRenderer(renderer);
	}
	texture = NULL;
	renderer = NULL;

	if(window && gl != last_gl)
	{
		/*
		 * Changing presentation families requires a new window. Synchronize
		 * live native targets before destroying their old context and window.
		 */
		if(last_gl)
		{
			video_gl_shutdown();
		}
		SDL_DestroyWindow(window);
		window = NULL;
	}
	last_gl = gl;

	/* Reuse compatible windows; otherwise create one with the required flags. */
	if(window)
	{
		if(savedata.fullscreen)
		{
			SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		}
		else
		{
#ifndef WIN // hiding and showing the window is problematic on Windows
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
			gpu_log("ERROR: Failed to create the %s presentation window: %s.", gl ? "OpenGL" : "SDL", SDL_GetError());
			return 0;
		}
		
		// Kratus (11-2022) Disabled the native OpenBOR icon
		// SDL_Surface* icon = (SDL_Surface*)pngToSurface((void*)openbor_icon_32x32_png.data);
		// SDL_SetWindowIcon(window, icon);
		// SDL_FreeSurface(icon);
		if(!savedata.fullscreen)
		{
			SDL_GetWindowPosition(window, &last_x, &last_y);
		}
	}

	/* The SDL presenter owns an SDL renderer; the GL path creates its context later. */
	if(!gl)
	{
		renderer = SDL_CreateRenderer(window, -1, savedata.fpslimit == 1 ? SDL_RENDERER_PRESENTVSYNC : 0);
		if(!renderer)
		{
			gpu_log("ERROR: Failed to create the SDL presenter: %s.", SDL_GetError());
			return 0;
		}
	}

	return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Describe the menu settings selected for this initialization. The first
 * snapshot records Starting Values; later snapshots describe reinitialization
 * without implying that settings remained fixed throughout the session.
 */
static void video_log_values(const s_videomodes *mode)
{
	static int starting_values_reported;
	char fps_limit[48], scale[80];
	const char *filter = "Disabled";
	const char *format = "Unknown";
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
		filter = savedata.swfilter > 0 && savedata.swfilter < BLITTER_MAX * 2 &&
		         GfxBlitterNames[savedata.swfilter] ? GfxBlitterNames[savedata.swfilter] : "Unknown";
	}
	switch(mode->pixel)
	{
	case 1: format = "Indexed 8-bit"; break;
	case 2: format = "RGB16"; break;
	case 3: format = "RGB24"; break;
	case 4: format = "RGB32"; break;
	}

	/* Keep authored scene dimensions separate from the converted/filtered
	 * presentation image reported after the selected backend is ready. */
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
	        "Software filter selected:", filter,
	        "CPU output filter:", filter_active ? "Active" : "Inactive",
	        "Scene resolution:", mode->hRes, mode->vRes,
	        "Scene color format:", format, mode->pixel);
	starting_values_reported = 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-13 - Report readable starting/reinitialized settings without profiling.
 * Select presentation for a normal game screen. Preserve the original mode
 * for restoration, prepare any CPU conversion/filter surfaces, and choose
 * OpenGL or the SDL streaming-texture path using the existing user settings.
 */
int video_set_mode(s_videomodes videomodes)
{
	int requested_gl = savedata.usegl;
	int report_mode = videomodes.hRes > 0 && videomodes.vRes > 0;

	/* Publish the previous measurement window before starting a new graphics
	 * section. Zero-sized shutdown modes have no new starting settings. */
	engine_profile_reset("video_mode");

	/* Report the authored scene format before conversion or filtering changes
	 * the presentation image. These diagnostics run only at mode initialization. */
	if(report_mode)
	{
		gpu_log("========== GRAPHICS INITIALIZATION ==========");
		video_log_values(&videomodes);
		if(videomodes.pixel != 4)
		{
			gpu_log("WARNING: The %d-bit main scene uses CPU drawing.\n"
			        "\tNative scene targets require RGB32.\n"
			        "\tConverting the finished image for presentation does not accelerate scene drawing.\n"
			        "\tSignificant slowdown is possible.", videomodes.pixel * 8);
		}
		if(!requested_gl)
		{
			gpu_log("WARNING: SDL presentation is selected in settings.\n"
			        "\tScene drawing will use the CPU. Significant slowdown is possible.\n"
			        "\tNative OpenGL capabilities are not tested in this mode.");
		}
#ifndef OPENGL
		else
		{
			gpu_log("WARNING: This build has no native OpenGL backend.\n"
			        "\tRequested OpenGL presentation will use the SDL path with CPU scene drawing.\n"
			        "\tSignificant slowdown is possible.");
		}
#endif
	}

	/* Store game dimensions before preprocessing can expand the output image. */
	stored_videomodes = videomodes;
	yuv_mode = 0;

	/* Zero-sized modes end video use and must drain live native screen storage. */
	if(videomodes.hRes==0 && videomodes.vRes==0)
	{
		video_gl_shutdown();
		return 0;
	}

	videomodes = setupPreBlitProcessing(videomodes);

	// 8-bit color should be transparently converted to 32-bit
	assert(videomodes.pixel == 2 || videomodes.pixel == 4);

	/*
	 * OpenGL initializes both presentation and any supported native drawing.
	 * If it cannot initialize, continue through the SDL streaming-image path.
	 */
	if(savedata.usegl && video_gl_set_mode(videomodes))
	{
		if(report_mode) gpu_log("========== END GRAPHICS INITIALIZATION ==========");
		return 1;
	}
	else
	{
		opengl = 0;
	}

	if(!SetVideoMode(videomodes.hRes * videomodes.hScale,
	                 videomodes.vRes * videomodes.vScale,
	                 videomodes.pixel * 8, false))
	{
		if(report_mode) gpu_log("========== END GRAPHICS INITIALIZATION ==========");
		return 0;
	}

	if(savedata.hwfilter ||
	   (videomodes.hScale == 1 && videomodes.vScale == 1 && !savedata.fullscreen))
	{
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	}
	else
	{
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	}

	/* Uploads use the preprocessed format and dimensions selected above. */
	texture = SDL_CreateTexture(renderer,
	                            pixelformats[videomodes.pixel-1],
	                            SDL_TEXTUREACCESS_STREAMING,
	                            videomodes.hRes, videomodes.vRes);
	if(!texture)
	{
		gpu_log("ERROR: SDL presentation texture creation failed: %s.", SDL_GetError());
	}

	SDL_ShowCursor(SDL_DISABLE);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	video_stretch(savedata.stretch);

	/* SDL can select a driver independently of the requested engine backend.
	 * Record its actual capabilities rather than inferring them from defaults. */
	{
		SDL_RendererInfo information;
		int information_valid = SDL_GetRendererInfo(renderer, &information) == 0;
		int refresh_rate = video_current_refresh_rate();
		char refresh_label[32];
		if(refresh_rate > 0)
		{
			snprintf(refresh_label, sizeof(refresh_label), "%d Hz", refresh_rate);
		}
		else
		{
			snprintf(refresh_label, sizeof(refresh_label), "Unavailable");
		}
		gpu_log("Active Presentation\n"
		        "\t%-26s\tSDL\n"
		        "\t%-26s\t%s\n"
		        "\t%-26s\t%s\n"
		        "\t%-26s\t%s\n"
		        "\t%-26s\tInactive (CPU scene drawing)\n"
		        "\t%-26s\t%s\n"
		        "\t%-26s\t%s\n"
		        "\t%-26s\t%s\n"
		        "\t%-26s\t%d-bit (%d bytes per pixel)\n"
		        "\tSDL acceleration displays completed images; scene drawing stays on the CPU.\n",
		        "Backend:", "Status:", texture ? "Ready" : "Unavailable",
		        "Driver:", information_valid && information.name ? information.name : "Unknown",
		        "Presentation acceleration:", information_valid ?
		            ((information.flags & SDL_RENDERER_ACCELERATED) ? "Yes" : "No") : "Unknown",
		        "Native drawing backend:",
		        "VSync requested:", savedata.fpslimit == 1 ? "Enabled" : "Disabled",
		        "VSync selected:", information_valid ?
		            ((information.flags & SDL_RENDERER_PRESENTVSYNC) ? "Enabled" : "Disabled") : "Unknown",
		        "Display refresh rate:", refresh_label,
		        "Output color depth:", videomodes.pixel * 8, videomodes.pixel);
	}

	if(report_mode) gpu_log("========== END GRAPHICS INITIALIZATION ==========");
	return 1;
}

int video_restore_mode(void)
{
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
 * Submit the SDL presentation quad and brightness overlay, measuring window
 * preparation separately from SDL's potentially blocking presentation call.
 */
void blit()
{
	ep_scope setup_scope = engine_profile_span_begin();
	ep_scope swap_scope;

	/* Preserve the existing composition order before handing the frame to SDL. */
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);

	if (brightness > 0)
	{
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, brightness-1);
	}
	else if (brightness < 0)
	{
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, (-brightness)-1);
	}
	SDL_RenderFillRect(renderer, NULL);
	engine_profile_span_end(setup_scope, EP_SPAN_PRESENT_SETUP);

	/* Present may wait for vertical synchronization or outstanding GPU work.
	 * Keep that wait visible instead of charging it to the preceding draw calls. */
	swap_scope = engine_profile_span_begin();
	SDL_RenderPresent(renderer);
	engine_profile_span_end(swap_scope, EP_SPAN_SWAP);
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
 * 2026-11-12
 * Present the completed game image. Try a resident GPU texture first, then
 * obtain synchronized CPU pixels only when a legacy upload or output filter
 * is required. Each successful path retains its normal frame pacing.
 */
int video_copy_screen(s_screen* src)
{
	ep_scope setup_scope;
	s_videosurface *surface;

	/*
	 * Native GPU screens already contain the composed frame. Present their
	 * texture before requesting CPU pixels or applying legacy conversion.
	 */
	if(opengl && !yuv_mode && video_gl_copy_gpu_screen(src))
	{
		return 1;
	}

	/*
	 * The native path declined this image or presentation mode. Synchronize
	 * its CPU shadow and apply the configured palette conversion/output filter.
	 */
	setup_scope = engine_profile_span_begin();
	surface = getVideoSurface(src);
	engine_profile_span_end(setup_scope, EP_SPAN_PRESENT_SETUP);

	if(opengl)
	{
		return video_gl_copy_screen(surface);
	}

	/* SDL presentation uploads the completed CPU image to its streaming texture. */
	setup_scope = engine_profile_span_begin();
	SDL_UpdateTexture(texture, NULL, surface->data, surface->pitch);
	engine_profile_span_end(setup_scope, EP_SPAN_PRESENT_SETUP);
	blit();

	if(savedata.fpslimit >= 2)
	{
		FramerateDelay();
	}

	return 1;
}

void video_clearscreen()
{
	if(opengl) { video_gl_clearscreen(); return; }

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
}

void video_stretch(int enable)
{
	stretch = enable;
	if(window && !opengl)
	{
		if(stretch)
			SDL_RenderSetLogicalSize(renderer, 0, 0);
		else
			SDL_RenderSetLogicalSize(renderer, stored_videomodes.hRes, stored_videomodes.vRes);
	}
}

void video_set_color_correction(int gm, int br)
{
	brightness = br;
	if(opengl) video_gl_set_color_correction(gm, br);
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Report the actual movie presenter after selecting the existing GL or SDL
 * upload path. Playback does not enable native scene drawing.
 */
int video_setup_yuv_overlay(const yuv_video_mode *mode)
{
	stored_yuv_mode = *mode;
	yuv_mode = 1;
	if(opengl) return video_gl_setup_yuv_overlay(mode);

	SDL_DestroyTexture(texture);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	texture = SDL_CreateTexture(renderer,
	                            SDL_PIXELFORMAT_YV12,
	                            SDL_TEXTUREACCESS_STREAMING,
	                            mode->width, mode->height);
	if(!stretch)
		SDL_RenderSetLogicalSize(renderer, mode->display_width, mode->display_height);
	if(texture)
	{
		gpu_log("Video playback uses SDL YUV presentation. Native scene drawing remains inactive in this mode.");
	}
	else
	{
		gpu_log("ERROR: SDL YUV presentation texture creation failed: %s.", SDL_GetError());
	}
	return texture ? 1 : 0;
}

int video_prepare_yuv_frame(yuv_frame *src)
{
	if(opengl) return video_gl_prepare_yuv_frame(src);

	SDL_UpdateYUVTexture(texture, NULL, src->lum, stored_yuv_mode.width,
	        src->cr, stored_yuv_mode.width/2, src->cb, stored_yuv_mode.width/2);
	return 1;
}

int video_display_yuv_frame(void)
{
	if(opengl) return video_gl_display_yuv_frame();

	blit();
	return 1;
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
