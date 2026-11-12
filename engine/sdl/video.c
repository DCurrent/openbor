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

void initSDL()
{
	SDL_DisplayMode video_info;
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
		printf("Warning: couldn't load OpenGL library (%s)\n", SDL_GetError());
	}
#endif

	SDL_GetCurrentDisplayMode(0, &video_info);
	nativeWidth = video_info.w;
	nativeHeight = video_info.h;
	printf("debug:nativeWidth, nativeHeight, bpp, Hz  %d, %d, %d, %d\n", nativeWidth, nativeHeight, SDL_BITSPERPIXEL(video_info.format), video_info.refresh_rate);
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
			printf("Error: failed to create window: %s\n", SDL_GetError());
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
			printf("Error: failed to create renderer: %s\n", SDL_GetError());
			return 0;
		}
	}

	return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Select presentation for a normal game screen. Preserve the original mode
 * for restoration, prepare any CPU conversion/filter surfaces, and choose
 * OpenGL or the SDL streaming-texture path using the existing user settings.
 */
int video_set_mode(s_videomodes videomodes)
{
	/* Configuration changes can replace the renderer or its synchronization
	 * policy. Finish the old measurement window before recording the new mode. */
	engine_profile_reset("video_mode");
	engine_profile_log("kind=video_config requested_backend=%s fpslimit=%d "
	                   "fullscreen=%d scale=%.2f software_filter=%d filter_active=%d "
	                   "source_width=%d source_height=%d source_bytes=%d",
	                   savedata.usegl ? "OpenGL" : "SDL", savedata.fpslimit,
	                   savedata.fullscreen, (double)savedata.hwscale, savedata.swfilter,
	                   !!(savedata.swfilter && (savedata.hwscale >= 2.0 || savedata.fullscreen)),
	                   videomodes.hRes, videomodes.vRes, videomodes.pixel);

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

	SDL_ShowCursor(SDL_DISABLE);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	video_stretch(savedata.stretch);

	/* SDL can select a driver independently of the requested engine backend.
	 * Record its actual capabilities rather than inferring them from defaults. */
	{
		SDL_RendererInfo information;
		int information_valid = SDL_GetRendererInfo(renderer, &information) == 0;
		engine_profile_log("kind=video_backend backend=SDL native_active=%d driver=%s "
		                   "requested_vsync=%d actual_vsync=%d refresh_hz=%d output_bytes=%d",
		                   gpu_is_active(), information_valid && information.name ? information.name : "unknown",
		                   savedata.fpslimit == 1,
		                   information_valid ? !!(information.flags & SDL_RENDERER_PRESENTVSYNC) : -1,
		                   video_current_refresh_rate(), videomodes.pixel);
	}

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
