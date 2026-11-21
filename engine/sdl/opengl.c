/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

/************************ OpenGL video backend *************************/
/*
 * Video backend that uses OpenGL as a cross-platform API for optimized
 * hardware blitting and scaling.
 */

#ifdef OPENGL

#include "sdlport.h"
#include "savedata.h"
#include "opengl.h"
#include "video.h"
#include "loadgl.h"
#include "gpu_renderer.h"
#include "gpu.h"
#include "gfx.h"
#include "engine_profile.h"
#include "video_settings.h"
#include <math.h>
#include <limits.h>

static SDL_GLContext context = NULL;

static int viewportWidth, viewportHeight;      // dimensions of display area
static int textureWidth, textureHeight;        // authored game-screen dimensions
static int displayWidth, displayHeight;
/* Movies retain their own layout; selecting one never replaces RGB state. */
static yuv_video_mode movieMode;

static GLuint shaderProgram; // fragment shader program

// Window state is shared with video.c.
extern int stretch;
extern int nativeWidth, nativeHeight;
extern SDL_Window* window;

#define FRAGMENT_SHADER_COMMON                                             \
    "uniform sampler2D tex;\n"                                             \
	"uniform float brightness;\n"                                          \
	"uniform float gamma;\n"                                               \
	"uniform vec2 texDims;\n"                                              \
	"uniform float scaleFactor;\n"                                         \
	"uniform bool highQuality;\n"                                          \
	"vec3 applyCorrection(vec3 color)\n"                                   \
	"{\n"                                                                  \
	"    if (gamma > 0.0)\n"                                               \
	"        color = 1.0 - ((1.0 - color) * (1.0 - (color * gamma)));\n"   \
	"    else\n"                                                           \
	"        color = color * (1.0 - ((1.0 - color) * -gamma));\n"          \
	"    if (brightness > 0.0)\n"                                          \
	"        color = mix(color, vec3(1.0), brightness);\n"                 \
	"    else\n"                                                           \
	"        color = mix(color, vec3(0.0), -brightness);\n"                \
	"    return color;\n"                                                  \
	"}\n"

#define FRAGMENT_SHADER_RGB                                                \
	"vec3 getPixel(vec2 coord)\n"                                          \
    "{\n"                                                                  \
    "   if (!highQuality) return texture2D(tex, coord).xyz;\n"             \
    "   vec2 texel = coord * texDims;\n"                                   \
    "   float region_range = 0.5 - 0.5 / scaleFactor;\n"                   \
    "   vec2 distFromCenter = fract(texel) - 0.5;\n"                       \
    "   vec2 f = (distFromCenter - clamp(distFromCenter, -region_range, region_range)) * scaleFactor + 0.5;\n" \
    "   return texture2D(tex, (floor(texel) + f) / texDims).xyz;\n"        \
    "}\n"                                                                  \
	"void main()\n"                                                        \
	"{\n"                                                                  \
	"    vec3 color = getPixel(gl_TexCoord[0].xy).xyz;\n"                  \
	"    gl_FragColor.xyz = applyCorrection(color);\n"                     \
	"    gl_FragColor.w = 1.0;\n"                                          \
	"}\n"

static const char *fragmentShaderSourceRGB =
        FRAGMENT_SHADER_COMMON FRAGMENT_SHADER_RGB;


/*
 * Damon V. Caskey
 * 2026-11-12
 * Synchronize live native render targets before discarding their GL context.
 * Reset context-owned names so a new context never deletes unrelated objects
 * that happen to receive the same numeric names.
 */
void video_gl_shutdown(void)
{
	/*
	 * Render targets belong to this context. Make it current before draining
	 * newer GPU pixels into their CPU owners so those owners can outlive it.
	 */
	if(context)
	{
		if(window && SDL_GL_GetCurrentContext() != context &&
		   SDL_GL_MakeCurrent(window, context) < 0)
		{
			/* Never read another context's framebuffer into a live screen shadow. */
			fprintf(stderr, "Unable to restore GL context for screen preservation: %s\n", SDL_GetError());
			abort();
		}
		/* Finish screen preservation and release scene resources before the context. */
		gpu_gl_shutdown();
		SDL_GL_DeleteContext(context);
		context = NULL;
	}

	/* Numeric GL object names become reusable after context destruction. */
	shaderProgram = 0;
	opengl = 0;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Restore the window viewport and fixed-function projection independently
 * of any previously cached native offscreen rendering state.
 */
void video_gl_setup_screen()
{
	/* This helper can also run outside a complete presentation pass. */
	gpu_gl_invalidate_state();
	// set up the viewport
	glViewport(0, 0, viewportWidth, viewportHeight);

	// set up orthographic projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, viewportWidth-1, 0, viewportHeight-1, -1, 1);

	// reset the model view
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-13 - Make presentation failures and CPU drawing modes visible at startup.
 * 2026-11-13 - Format presentation checks and initialized settings for readers.
 * 2026-11-15 - Leave failed native RGB32 mode selection to the strict caller policy.
 * 2026-11-16 - Require native scenes and keep filter intermediates on the GPU.
 * Create the desktop GL presentation context and its completed-image shader.
 * Preserve live screens across mode changes, validate the presentation path,
 * then enable native scene drawing for every output-filter selection.
 */
int video_gl_set_mode(s_videomodes videomodes)
{
	GLint maxTextureSize;
	const char *failure_reason = "OpenGL initialization failed";
	int filter_active = savedata.swfilter && (savedata.hwscale >= 2.0 || savedata.fullscreen);
	GLint shader_status;
	char shader_log[1024] = { 0 };
	GLuint fragmentShader;

	/*
	 * Script-created screens can survive display changes. Drain their latest
	 * pixels while the previous window/context pair is still intact.
	 */
	video_gl_shutdown();

	/* Authored dimensions control layout independently of GPU filter targets. */
	displayWidth = textureWidth = videomodes.hRes;
	displayHeight = textureHeight = videomodes.vRes;

	// use the current monitor resolution in fullscreen mode to prevent aspect ratio distortion
	viewportWidth = savedata.fullscreen ? nativeWidth : (int)(videomodes.hRes * MAX(0.25,videomodes.hScale));
	viewportHeight = savedata.fullscreen ? nativeHeight : (int)(videomodes.vRes * MAX(0.25,videomodes.vScale));

	/* Zero-sized modes close video use instead of creating a presentation target. */
	if((viewportWidth == 0) || (viewportHeight == 0))
	{
		return 0;
	}

	// set up OpenGL double buffering
	if(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) != 0)
	{
		failure_reason = "OpenGL double buffering is unavailable";
		gpu_log("WARNING: %s: %s.", failure_reason, SDL_GetError());
		goto error;
	}

	// create an OpenGL compatibility context, not a core or ES context
#ifndef WIN // except on Windows, where some Nvidia drivers really don't like us doing this
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
#endif

	/* The window must advertise GL support before SDL can create its context. */
	if(!SetVideoMode(viewportWidth, viewportHeight))
	{
		failure_reason = "the OpenGL-compatible window could not be created";
		gpu_log("WARNING: %s: %s.", failure_reason, SDL_GetError());
		goto error;
	}
	context = SDL_GL_CreateContext(window);

	// make sure the context was created successfully
	if(!context)
	{
		failure_reason = "the OpenGL context could not be created";
		gpu_log("WARNING: %s: %s.", failure_reason, SDL_GetError());
		goto error;
	}

	// update viewport size based on actual dimensions
	if(SDL_GL_MakeCurrent(window, context) < 0)
	{
		failure_reason = "the OpenGL context could not be activated";
		gpu_log("WARNING: %s: %s.", failure_reason, SDL_GetError());
		goto error;
	}

	/* Keep the existing frame-pacing selection when recreating the context. */
	if(SDL_GL_SetSwapInterval(savedata.fpslimit == 1) < 0)
	{
		gpu_log("WARNING: Could not %s vertical sync.\n\tSDL diagnostic: %s",
		        savedata.fpslimit == 1 ? "enable" : "disable", SDL_GetError());
	}

#ifdef LOADGL
	/* Resolve required GL entry points against the newly created context. */
	if(LoadGLFunctions() == 0)
	{
		failure_reason = "required OpenGL entry points are unavailable";
		gpu_log("WARNING: %s. Native scene capabilities could not be checked.", failure_reason);
		goto error;
	}
#endif

	/*
	 * Check the completed-image presenter's requirements before creating its
	 * texture and shader. Native drawing performs its own capability checks.
	 */
	if(!SDL_GL_ExtensionSupported("GL_ARB_fragment_shader"))
	{
		failure_reason = "the required GL_ARB_fragment_shader capability is missing";
		gpu_log("WARNING: %s.", failure_reason);
		goto error;
	}

	// reject the Mesa software renderer (swrast) because it's slow
	if(stricmp((const char*)glGetString(GL_RENDERER), "Software Rasterizer") == 0)
	{
		failure_reason = "the OpenGL driver reports the unsupported Software Rasterizer";
		gpu_log("WARNING: %s.", failure_reason);
		goto error;
	}

	// don't try to create a texture larger than the maximum allowed dimensions
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	if((textureWidth > maxTextureSize / (filter_active ? 2 : 1)) ||
	   (textureHeight > maxTextureSize / (filter_active ? 2 : 1)))
	{
		failure_reason = filter_active ? "the 2x output-filter target exceeds the GPU texture-size limit" :
		                  "the presentation texture exceeds the GPU texture-size limit";
		gpu_log("WARNING: %s: requested=%dx%d, maximum=%d.", failure_reason,
		        textureWidth, textureHeight, maxTextureSize);
		goto error;
	}
	/* Native scene resources receive their own capability checks below. */
	gpu_log("OpenGL Presentation Checks\n"
	        "\tFragment shader extension:         AVAILABLE\n"
	        "\tMaximum texture size:              %d pixels per axis\n"
	        "\tRequested texture size:            %d x %d pixels\n",
	        maxTextureSize, textureWidth, textureHeight);

	// set proper viewport width/height for high-DPI support
	SDL_GL_GetDrawableSize(window, &viewportWidth, &viewportHeight);

	/* Establish clean window-frame state before creating presentation resources. */
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	// disable unneeded features
	glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_DITHER);

	// set up offsets, scale factors, and viewport
	video_gl_setup_screen();

	/*
	 * This shader owns final display scaling and color correction. Scene draws
	 * bind their own program; each presentation pass restores this one.
	 */
	fragmentShader = ptr_glCreateShader(GL_FRAGMENT_SHADER);
	if(!fragmentShader)
	{
		failure_reason = "the presentation shader could not be allocated";
		goto error;
	}
	ptr_glShaderSource(fragmentShader, 1, &fragmentShaderSourceRGB, NULL);
	ptr_glCompileShader(fragmentShader);
	ptr_glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &shader_status);
	if(!shader_status)
	{
		ptr_glGetShaderInfoLog(fragmentShader, sizeof(shader_log), NULL, shader_log);
		gpu_log("WARNING: Presentation shader compilation failed: %s", shader_log);
		ptr_glDeleteShader(fragmentShader);
		failure_reason = "the presentation shader could not compile";
		goto error;
	}
	shaderProgram = ptr_glCreateProgram();
	if(!shaderProgram)
	{
		ptr_glDeleteShader(fragmentShader);
		failure_reason = "the presentation program could not be allocated";
		goto error;
	}
	ptr_glAttachShader(shaderProgram, fragmentShader);
	ptr_glLinkProgram(shaderProgram);
	ptr_glDeleteShader(fragmentShader);
	ptr_glGetProgramiv(shaderProgram, GL_LINK_STATUS, &shader_status);
	if(!shader_status)
	{
		ptr_glGetProgramInfoLog(shaderProgram, sizeof(shader_log), NULL, shader_log);
		gpu_log("WARNING: Presentation shader linking failed: %s", shader_log);
		failure_reason = "the presentation shader could not link";
		goto error;
	}
	ptr_glUseProgram(shaderProgram);
	ptr_glUniform1i(ptr_glGetUniformLocation(shaderProgram, "tex"), 0);
	ptr_glUniform1i(ptr_glGetUniformLocation(shaderProgram, "highQuality"),
	                savedata.hwfilter == VIDEO_SCALING_HIGH_QUALITY);
	ptr_glUniform2f(ptr_glGetUniformLocation(shaderProgram, "texDims"), textureWidth, textureHeight);
	float scaleFactor = MIN((float)viewportWidth / textureWidth, (float)viewportHeight / textureHeight);
	ptr_glUniform1f(ptr_glGetUniformLocation(shaderProgram, "scaleFactor"), ceilf(scaleFactor));
	video_gl_set_color_correction(savedata.gamma, savedata.brightness);

	/* Scene composition stays native with every output-filter selection. */
	if(!gpu_gl_init())
	{
		failure_reason = "required native scene rendering is unavailable";
		goto error;
	}
	opengl = 1;

	/* These values describe this initialization. Menu changes can select
	 * different presentation settings later in the same session. */
	{
		int swap_interval = SDL_GL_GetSwapInterval();
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
		        "\tValues confirmed at this video initialization.\n"
		        "\tBackend:                           OpenGL\n"
		        "\tNative drawing backend:            %s\n"
		        "\tRequested vertical sync:           %s\n"
		        "\tActual vertical sync:              %s\n"
		        "\tSwap interval:                     %d\n"
		        "\tDisplay refresh rate:              %s\n"
		        "\tOutput color depth:                %d bits\n",
		        "Active (RGB32 scenes and GPU output filters)",
		        savedata.fpslimit == 1 ? "Enabled" : "Disabled",
		        swap_interval == 0 ? "Disabled" : swap_interval < 0 ? "Adaptive" : "Enabled",
		        swap_interval, refresh_label, videomodes.pixel * 8);
	}
	return 1;
error:
	/* Release partial resources while retaining required native scene policy. */
	gpu_log("ERROR: GPU presentation initialization failed.\n"
	        "\tReason: %s.", failure_reason);
	video_gl_shutdown();
	return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Clear both window buffers after restoring the presentation framebuffer.
 * Native offscreen targets retain their contents for subsequent scene draws.
 */
void video_gl_clearscreen()
{
	/* Route clearing back to the window after any offscreen scene operation. */
	gpu_gl_prepare_present();

	/* Clearing both sides prevents an old image from returning on the next swap. */
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapWindow(window);
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapWindow(window);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Draw a presentation quad using window-owned state. Keep its direct texture
 * unit change visible to the native cache even when invoked independently.
 */
void video_gl_draw_quad(int x, int y, int width, int height)
{
	gpu_gl_invalidate_state();
	glClear(GL_COLOR_BUFFER_BIT);
	glActiveTexture(GL_TEXTURE0);
	glBegin(GL_QUADS);
		// Top left
		glMultiTexCoord2f(GL_TEXTURE0, 0.0, 1.0);
		glVertex2i(x, y);

		// Top right
		glMultiTexCoord2f(GL_TEXTURE0, 1.0, 1.0);
		glVertex2i(x+width-1, y);

		// Bottom right
		glMultiTexCoord2f(GL_TEXTURE0, 1.0, 0.0);
		glVertex2i(x+width-1, y+height-1);

		// Bottom left
		glMultiTexCoord2f(GL_TEXTURE0, 0.0, 0.0);
		glVertex2i(x, y+height-1);
	glEnd();
}

void render()
{
	// determine x and y scale factors
	float texScale = MIN((float)viewportWidth/(float)textureWidth, (float)viewportHeight/(float)textureHeight);

	// determine on-screen dimensions
	int scaledWidth = (int)(displayWidth * texScale);
	int scaledHeight = (int)(displayHeight * texScale);

	// determine offsets
	int xOffset = (viewportWidth - scaledWidth) / 2;
	int yOffset = (viewportHeight - scaledHeight) / 2;

	// render the quad
	if(stretch)
		video_gl_draw_quad(0, 0, viewportWidth, viewportHeight);
	else
		video_gl_draw_quad(xOffset, yOffset, scaledWidth, scaledHeight);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * 2026-11-15 - Return rejected presentation to the caller's no-retry policy.
 * 2026-11-16 - Filter resident RGB32 scenes before final display scaling.
 * Present a GPU-resident screen with the normal display scaling and color
 * correction, without converting or uploading its CPU pixel buffer. Return
 * zero on rejection before touching the window; the caller logs and stops.
 */
int video_gl_copy_gpu_screen(s_screen *screen)
{
	unsigned nativeTexture;
	int nativeWidth, nativeHeight;
	int filter = savedata.swfilter && (savedata.hwscale >= 2.0 || savedata.fullscreen) ?
	             savedata.swfilter : BLITTER_NONE;
	GLint sampling = savedata.hwfilter == VIDEO_SCALING_NEAREST ? GL_NEAREST : GL_LINEAR;
	float scaleFactor;
	uint64_t gpu_scope = engine_profile_gpu_begin(EP_GPU_PRESENT);
	ep_scope setup_scope = engine_profile_span_begin();
	ep_scope swap_scope;

	if(!gpu_gl_get_filtered_screen_texture(screen, filter, &nativeTexture, &nativeWidth, &nativeHeight))
	{
		/* Invalid or unavailable filters never trigger a CPU presentation retry. */
		engine_profile_span_end(setup_scope, EP_SPAN_PRESENT_SETUP);
		engine_profile_gpu_end(gpu_scope);
		return 0;
	}

	/*
	 * The native renderer owns its draw state. Reestablish everything used by
	 * the presenter, including framebuffer zero, shader and orthographic view.
	 */
	gpu_gl_prepare_present();
	video_gl_setup_screen();
	ptr_glUseProgram(shaderProgram);
	glDisable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_DEPTH_TEST);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, nativeTexture);
	/* Display sampling is independent of scene drawmethod resampling and the
	 * optional final-image effect. Only HQ remaps texture coordinates. */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampling);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampling);
	ptr_glUniform1i(ptr_glGetUniformLocation(shaderProgram, "highQuality"),
	                savedata.hwfilter == VIDEO_SCALING_HIGH_QUALITY);
	ptr_glUniform2f(ptr_glGetUniformLocation(shaderProgram, "texDims"), nativeWidth, nativeHeight);
	scaleFactor = MIN((float)viewportWidth / nativeWidth, (float)viewportHeight / nativeHeight);
	ptr_glUniform1f(ptr_glGetUniformLocation(shaderProgram, "scaleFactor"), ceilf(scaleFactor));

	/* The scene and optional 2x filter target share the same image extent. */
	render();

	/* Native sampling parameters are initialized once, not reissued per draw.
	 * Restore this borrowed texture's NEAREST policy before returning ownership. */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	engine_profile_span_end(setup_scope, EP_SPAN_PRESENT_SETUP);
	engine_profile_gpu_end(gpu_scope);

	/* Keep swap waiting separate from submitting the final presentation quad. */
	swap_scope = engine_profile_span_begin();
	SDL_GL_SwapWindow(window);
	engine_profile_span_end(swap_scope, EP_SPAN_SWAP);
	if(savedata.fpslimit >= 2)
	{
		FramerateDelay();
	}
	return 1;
}

// Set the brightness and gamma values
int lastGamma = 0, lastBrightness = 0;
/*
 * Damon V. Caskey
 * 2026-11-12
 * Change the presentation shader's color correction without leaving the
 * native renderer believing its own program is still bound.
 */
void video_gl_set_color_correction(int gamma, int brightness)
{
	/* Runtime menu updates can reach this helper between two native draws. */
	gpu_gl_invalidate_state();
	lastGamma = gamma;
	lastBrightness = brightness;
	GLint brightnessLocation, gammaLocation;
	if(gamma < -256)
	{
		gamma = -256;
	}
	if(gamma > 256)
	{
		gamma = 256;
	}
	if(brightness < -256)
	{
		brightness = -256;
	}
	if(brightness > 256)
	{
		brightness = 256;
	}

	ptr_glUseProgram(shaderProgram);
	brightnessLocation = ptr_glGetUniformLocation(shaderProgram, "brightness");
	ptr_glUniform1f(brightnessLocation, brightness / 256.0);
	gammaLocation = ptr_glGetUniformLocation(shaderProgram, "gamma");
	ptr_glUniform1f(gammaLocation, gamma / 256.0);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Select movie display dimensions without retiring scene resources or changing
 * the normal RGB presenter. The native renderer owns all movie plane textures
 * and the shared screen/window conversion shader.
 */
int video_gl_setup_yuv_overlay(const yuv_video_mode *mode)
{
    if(!gpu_is_active() || !mode || mode->width < 2 || mode->height < 2 ||
       (mode->width & 1) || (mode->height & 1) ||
       mode->display_width <= 0 || mode->display_height <= 0)
    {
        gpu_log("WARNING: GPU movie presentation requires an active native renderer and valid YUV420 dimensions.");
        return 0;
    }
    movieMode = *mode;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Present retained YUV planes at window resolution using the same native movie
 * shader as screen composition. Preserve the existing encoded/display sizing
 * rule and apply display correction before clamping the converted RGB result.
 */
int video_gl_prepare_yuv_frame(const gpu_yuv_source *source,
                               e_drawmethod_resample resample)
{
    gpu_movie_draw draw = {0};
    double scaled_width, scaled_height;
    float scale;
    if(!source || source->width != movieMode.width ||
       source->height != movieMode.height ||
       viewportWidth <= 0 || viewportHeight <= 0)
    {
        return 0;
    }

    /* Fullscreen movie output retains decoded resolution, independently of the
     * authored game screen. Stretch continues to select the entire window. */
    scale = MIN((float)viewportWidth / source->width,
                (float)viewportHeight / source->height);
    scaled_width = stretch ? viewportWidth : (float)movieMode.display_width * scale;
    scaled_height = stretch ? viewportHeight : (float)movieMode.display_height * scale;
    if(scaled_width < 1.0 || scaled_height < 1.0 ||
       scaled_width > INT_MAX || scaled_height > INT_MAX)
    {
        gpu_log("WARNING: GPU movie display dimensions are outside the supported viewport range.");
        return 0;
    }
    draw.width = (int)scaled_width;
    draw.height = (int)scaled_height;
    draw.x = (viewportWidth - draw.width) / 2;
    draw.y = (viewportHeight - draw.height) / 2;
    draw.resample = resample;
    draw.gamma = MAX(-256, MIN(256, lastGamma)) / 256.0f;
    draw.brightness = MAX(-256, MIN(256, lastBrightness)) / 256.0f;
    /* Fullscreen movies remain opaque; black protection is for subscreens. */
    return gpu_gl_present_yuv(source, &draw, viewportWidth, viewportHeight);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Render a screenshot through the same YUV conversion and display correction
 * as fullscreen output. The caller chooses capture dimensions and performs
 * the explicit readback only when it writes the screenshot file.
 */
int video_gl_capture_yuv_frame(s_screen *screen, const gpu_yuv_source *source,
                               e_drawmethod_resample resample)
{
    gpu_movie_draw draw = {0};
    if(!screen)
    {
        return 0;
    }
    draw.width = screen->width;
    draw.height = screen->height;
    draw.resample = resample;
    draw.gamma = MAX(-256, MIN(256, lastGamma)) / 256.0f;
    draw.brightness = MAX(-256, MIN(256, lastBrightness)) / 256.0f;
    return gpu_draw_yuv(screen, source, &draw);
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Swap a prepared native movie without changing scene ownership. Subsequent
 * screen drawing or RGB presentation can use the same context immediately.
 */
int video_gl_display_yuv_frame(void)
{
    if(!gpu_is_active())
    {
        return 0;
    }
    SDL_GL_SwapWindow(window);
    return 1;
}

#endif
