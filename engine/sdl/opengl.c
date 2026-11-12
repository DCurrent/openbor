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
#include "engine_profile.h"
#include <math.h>

#define nextpowerof2(x) pow(2,ceil(log(x)/log(2)))
#define abs(x)			((x<0)?-(x):(x))

static SDL_GLContext context = NULL;

static GLuint gltexture[3];
static GLint textureColorFormat[3];
static GLint texturePixelFormat[3];

static int viewportWidth, viewportHeight;      // dimensions of display area
static int textureWidth, textureHeight;        // dimensions of game screen and GL texture 0
static int displayWidth, displayHeight;

static GLfloat tcx, tcy; // maximum x and y texture coords in floating-point form
static GLuint shaderProgram; // fragment shader program

// use some variables declared in video.c that are common to both backends
extern int stretch;
extern int nativeWidth, nativeHeight;
extern SDL_Window* window;

#define FRAGMENT_SHADER_COMMON                                             \
    "uniform sampler2D tex;\n"                                             \
	"uniform float brightness;\n"                                          \
	"uniform float gamma;\n"                                               \
	"uniform vec2 texDims;\n"                                              \
	"uniform float scaleFactor;\n"                                         \
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

#define FRAGMENT_SHADER_RGB_BASIC                                          \
	"void main()\n"                                                        \
	"{\n"                                                                  \
	"    vec3 color = texture2D(tex, gl_TexCoord[0].xy).xyz;\n"            \
	"    gl_FragColor.xyz = applyCorrection(color);\n"                     \
	"    gl_FragColor.w = 1.0;\n"                                          \
	"}\n"

#define FRAGMENT_SHADER_RGB_HIGH_QUALITY                                   \
	"vec3 getPixel(vec2 coord)\n"                                          \
    "{\n"                                                                  \
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

#define FRAGMENT_SHADER_YUV                                                \
	"uniform sampler2D utex;\n"                                            \
	"uniform sampler2D vtex;\n"                                            \
	"\n"                                                                   \
	"vec3 yuv2rgb(vec3 yuv)\n"                                             \
	"{\n"                                                                  \
	"	yuv += vec3(-.0625, -.5, -.5);\n"                                  \
	"	vec3 rgb;\n"                                                       \
	"	rgb.x = dot(yuv, vec3(1.164, 0, 1.596));\n"                        \
	"	rgb.y = dot(yuv, vec3(1.164, -.391, -.813));\n"                    \
	"	rgb.z = dot(yuv, vec3(1.164, 2.018, 0));\n"                        \
	"	return rgb;\n"                                                     \
	"}\n"                                                                  \
	"\n"                                                                   \
	"void main()\n"                                                        \
	"{\n"                                                                  \
	"	vec3 yuv;\n"                                                       \
	"	yuv.x = texture2D(tex,  gl_TexCoord[0].xy).x; // Y\n"              \
	"	yuv.y = texture2D(utex, gl_TexCoord[0].xy).x; // U\n"              \
	"	yuv.z = texture2D(vtex, gl_TexCoord[0].xy).x; // V\n"              \
	"	gl_FragColor.xyz = applyCorrection(yuv2rgb(yuv));\n"               \
	"	gl_FragColor.w = 1.0;\n"                                           \
	"}\n"


static const char *fragmentShaderSourceHighQualityRGB =
        FRAGMENT_SHADER_COMMON FRAGMENT_SHADER_RGB_HIGH_QUALITY;
static const char *fragmentShaderSourceYUV =
        FRAGMENT_SHADER_COMMON FRAGMENT_SHADER_YUV;
/*static const char *vertexShaderSource =
        "varying vec2 texCoord;\n"
        "void main()\n"
        "{\n"
        "    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
        "    gl_Position = ftransform();\n"
        "}\n";*/

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
	memset(gltexture, 0, sizeof(gltexture));
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
 * Allocate the presenter's RGB or video-plane texture, invalidating native
 * bindings before replacing objects or selecting a different texture unit.
 */
void video_gl_init_texture(int index, int width, int height, int bytesPerPixel)
{
	int allocTextureWidth, allocTextureHeight;
	SDL_Surface *surface;

	/* Deletion and allocation can reuse GL names as well as change bindings. */
	gpu_gl_invalidate_state();

	// set texture format
	switch(bytesPerPixel)
	{
	case 1: // Y/U/V (not indexed!)
		textureColorFormat[index] = GL_RED;
		texturePixelFormat[index] = GL_UNSIGNED_BYTE;
		break;
	case 2: // BGR565
		textureColorFormat[index] = GL_RGB;
		texturePixelFormat[index] = GL_UNSIGNED_SHORT_5_6_5_REV;
		break;
	case 4: // XBGR8888
		textureColorFormat[index] = GL_RGBA;
		texturePixelFormat[index] = GL_UNSIGNED_BYTE;
		break;
	default:
		assert(!"unknown bytes per pixel; should be 1, 2, or 4");
	}

	// enable 2D textures
	glActiveTexture(GL_TEXTURE0 + index);
	glEnable(GL_TEXTURE_2D);

	// use non-power-of-two texture dimensions if they are available
	if(SDL_GL_ExtensionSupported("GL_ARB_texture_non_power_of_two"))
	{
		allocTextureWidth = width;
		allocTextureHeight = height;
	}
	else
	{
		allocTextureWidth = nextpowerof2(width);
		allocTextureHeight = nextpowerof2(height);
	}

	// calculate maximum texture coordinates
	tcx = (width == 0) ? 0 : (float)width / (float)allocTextureWidth;
	tcy = (height == 0) ? 0 : (float)height / (float)allocTextureHeight;

	// allocate a temporary surface to initialize the texture with
	surface = SDL_CreateRGBSurface(0, allocTextureWidth, allocTextureHeight, bytesPerPixel * 8, 0,0,0,0);

	// create texture object
	glDeleteTextures(1, &gltexture[index]);
	glGenTextures(1, &gltexture[index]);
	glBindTexture(GL_TEXTURE_2D, gltexture[index]);
	glTexImage2D(GL_TEXTURE_2D, 0, textureColorFormat[index], allocTextureWidth, allocTextureHeight,
	        0, textureColorFormat[index], texturePixelFormat[index], surface->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// free up the temporary surface
	SDL_FreeSurface(surface);
	surface = NULL;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Create the desktop GL presentation context and its completed-image shader.
 * Preserve live screens across mode changes, validate the presentation path,
 * then enable native scene drawing when the selected output mode supports it.
 */
int video_gl_set_mode(s_videomodes videomodes)
{
	GLint maxTextureSize;

	/*
	 * Script-created screens can survive display changes. Drain their latest
	 * pixels while the previous window/context pair is still intact.
	 */
	video_gl_shutdown();

	/* These dimensions describe the completed image after CPU preprocessing. */
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
		printf("Can't set up OpenGL double buffering (%s)...", SDL_GetError());
		goto error;
	}

	// create an OpenGL compatibility context, not a core or ES context
#ifndef WIN // except on Windows, where some Nvidia drivers really don't like us doing this
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
#endif

	/* The window must advertise GL support before SDL can create its context. */
	SetVideoMode(viewportWidth, viewportHeight, 0, true);
	if(!window)
	{
		printf("Failed to create OpenGL-compatible window (%s)...", SDL_GetError());
		goto error;
	}
	context = SDL_GL_CreateContext(window);

	// make sure the context was created successfully
	if(!context)
	{
		printf("OpenGL initialization failed (%s)...", SDL_GetError());
		goto error;
	}

	// update viewport size based on actual dimensions
	if(SDL_GL_MakeCurrent(window, context) < 0)
	{
		printf("MakeCurrent on OpenGL context failed (%s)...", SDL_GetError());
		goto error;
	}

	/* Keep the existing frame-pacing selection when recreating the context. */
	if(SDL_GL_SetSwapInterval(savedata.fpslimit == 1) < 0)
	{
		printf("Warning: can't disable vertical retrace sync (%s)...\n", SDL_GetError());
	}

#ifdef LOADGL
	/* Resolve required GL entry points against the newly created context. */
	if(LoadGLFunctions() == 0)
	{
		goto error;
	}
#endif

	/*
	 * Check the completed-image presenter's requirements before creating its
	 * texture and shader. Native drawing performs its own capability checks.
	 */
	if(!SDL_GL_ExtensionSupported("GL_ARB_fragment_shader"))
	{
		printf("OpenGL fragment shaders not supported...");
		goto error;
	}

	// reject the Mesa software renderer (swrast) because it's slow
	if(stricmp((const char*)glGetString(GL_RENDERER), "Software Rasterizer") == 0)
	{
		printf("Not going to use the Mesa software renderer...");
		goto error;
	}

	// don't try to create a texture larger than the maximum allowed dimensions
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	if((textureWidth > maxTextureSize) || (textureHeight > maxTextureSize))
	{
		printf("Unable to create a %ix%i OpenGL texture (max texture size %i)...", textureWidth, textureHeight, maxTextureSize);
		goto error;
	}

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

	// initialize texture object
	video_gl_init_texture(0, textureWidth, textureHeight, videomodes.pixel);

	// set up offsets, scale factors, and viewport
	video_gl_setup_screen();

	/*
	 * This shader owns final display scaling and color correction. Scene draws
	 * bind their own program; each presentation pass restores this one.
	 */
	GLuint fragmentShader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glShaderSourceARB(fragmentShader, 1, &fragmentShaderSourceHighQualityRGB, NULL);
	glCompileShaderARB(fragmentShader);
	shaderProgram = glCreateProgramObjectARB();
	glAttachObjectARB(shaderProgram, fragmentShader);
	glLinkProgramARB(shaderProgram);
	glUseProgramObjectARB(shaderProgram);
	glUniform1iARB(glGetUniformLocationARB(shaderProgram, "tex"), 0);
	glUniform2fARB(glGetUniformLocationARB(shaderProgram, "texDims"), textureWidth, textureHeight);
	float scaleFactor = MIN((float)viewportWidth / textureWidth, (float)viewportHeight / textureHeight);
	glUniform1fARB(glGetUniformLocationARB(shaderProgram, "scaleFactor"), ceilf(scaleFactor));
	video_gl_set_color_correction(savedata.gamma, savedata.brightness);

	opengl = 1;
	/*
	 * CPU output filters operate on the whole composed image. Keep that mode
	 * fully on the legacy path instead of reading a GPU frame back each time.
	 * Initialization failure leaves the existing GL presenter usable.
	 */
	if(!(savedata.swfilter && (savedata.hwscale >= 2.0 || savedata.fullscreen)))
	{
		gpu_gl_init();
	}
	else
	{
		printf("Native GPU drawing disabled while a CPU output filter is active.\n");
	}

	/* Confirm the active scene backend and driver in the regular engine log.
	 * GL presentation alone does not indicate whether scene drawing is native. */
	{
		const char *vendor = (const char *)glGetString(GL_VENDOR);
		const char *renderer_name = (const char *)glGetString(GL_RENDERER);
		const char *version = (const char *)glGetString(GL_VERSION);
		engine_profile_log("kind=video_backend backend=OpenGL native_active=%d "
		                   "requested_swap_interval=%d actual_swap_interval=%d refresh_hz=%d output_bytes=%d "
		                   "vendor=\"%s\" renderer=\"%s\" version=\"%s\"",
		                   gpu_is_active(), savedata.fpslimit == 1, SDL_GL_GetSwapInterval(),
		                   video_current_refresh_rate(), videomodes.pixel, vendor ? vendor : "unknown",
		                   renderer_name ? renderer_name : "unknown", version ? version : "unknown");
	}
	return 1;
error:
	/* Release any partial context before the caller constructs an SDL presenter. */
	printf("falling back to SDL video backend\n");
	video_gl_shutdown();
	savedata.usegl = 0;
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
		glMultiTexCoord2f(GL_TEXTURE0, 0.0, tcy);
		glVertex2i(x, y);

		// Top right
		glMultiTexCoord2f(GL_TEXTURE0, tcx, tcy);
		glVertex2i(x+width-1, y);

		// Bottom right
		glMultiTexCoord2f(GL_TEXTURE0, tcx, 0.0);
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
 * Present a completed CPU image through the GL output texture. Restore the
 * window's render state after native operations, upload the synchronized
 * pixels, then apply the existing display scaling and frame-pacing policy.
 */
int video_gl_copy_screen(s_videosurface* surface)
{
	uint64_t gpu_scope = engine_profile_gpu_begin(EP_GPU_PRESENT);
	ep_scope setup_scope = engine_profile_span_begin();
	ep_scope swap_scope;

	/*
	 * Native draws and explicit CPU readbacks can leave an offscreen target
	 * bound. Reestablish the window framebuffer, projection and RGB shader.
	 */
	gpu_gl_prepare_present();
	video_gl_setup_screen();
	glUseProgramObjectARB(shaderProgram);
	glDisable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_DEPTH_TEST);
	glUniform2fARB(glGetUniformLocationARB(shaderProgram, "texDims"), textureWidth, textureHeight);

	/* Upload the caller's already converted/filtered CPU image as one texture. */
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gltexture[0]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureWidth, textureHeight, textureColorFormat[0],
			texturePixelFormat[0], surface->data);

	// set linear filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// render the frame
	render();
	engine_profile_span_end(setup_scope, EP_SPAN_PRESENT_SETUP);
	engine_profile_gpu_end(gpu_scope);

	/* End the GPU presentation interval before a possibly blocking swap.
	 * Swap and the explicit FPS limiter receive separate CPU measurements. */
	swap_scope = engine_profile_span_begin();
	SDL_GL_SwapWindow(window);
	engine_profile_span_end(swap_scope, EP_SPAN_SWAP);

	if(savedata.fpslimit >= 2)
	{
		FramerateDelay();
	}

	return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Present a GPU-resident screen with the normal display scaling and color
 * correction, without converting or uploading its CPU pixel buffer. Return
 * zero when the native renderer cannot provide this screen so the caller can
 * use the existing software presentation path.
 */
int video_gl_copy_gpu_screen(s_screen *screen)
{
	unsigned nativeTexture;
	int nativeWidth, nativeHeight;
	GLfloat savedTcx, savedTcy;
	float scaleFactor;
	uint64_t gpu_scope = engine_profile_gpu_begin(EP_GPU_PRESENT);
	ep_scope setup_scope = engine_profile_span_begin();
	ep_scope swap_scope;

	if(!gpu_gl_get_screen_texture(screen, &nativeTexture, &nativeWidth, &nativeHeight))
	{
		/* Let the caller select CPU conversion/upload for unsupported screen types. */
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
	glUseProgramObjectARB(shaderProgram);
	glDisable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_DEPTH_TEST);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, nativeTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glUniform2fARB(glGetUniformLocationARB(shaderProgram, "texDims"), nativeWidth, nativeHeight);
	scaleFactor = MIN((float)viewportWidth / nativeWidth, (float)viewportHeight / nativeHeight);
	glUniform1fARB(glGetUniformLocationARB(shaderProgram, "scaleFactor"), ceilf(scaleFactor));

	/*
	 * Native targets occupy their full texture. Preserve the legacy presenter's
	 * texture-coordinate limits so a later CPU upload can reuse its allocation.
	 */
	savedTcx = tcx;
	savedTcy = tcy;
	tcx = tcy = 1.0f;
	render();
	tcx = savedTcx;
	tcy = savedTcy;

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

	glUseProgramObjectARB(shaderProgram);
	brightnessLocation = glGetUniformLocationARB(shaderProgram, "brightness");
	glUniform1fARB(brightnessLocation, brightness / 256.0);
	gammaLocation = glGetUniformLocationARB(shaderProgram, "gamma");
	glUniform1fARB(gammaLocation, gamma / 256.0);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Switch the presenter from game RGB output to decoded YUV video planes.
 * Preserve native screen contents before retiring scene drawing, then prepare
 * three plane textures and the conversion shader used during movie playback.
 */
int video_gl_setup_yuv_overlay(const yuv_video_mode *mode)
{
	/* Scripted screens remain valid while movie frames use the upload path. */
	gpu_gl_shutdown();

	/* Source plane dimensions and display aspect can differ for decoded video. */
	textureWidth = mode->width;
	textureHeight = mode->height;
	displayWidth = mode->display_width;
	displayHeight = mode->display_height;
	video_gl_init_texture(0, mode->width, mode->height, 1);
	video_gl_init_texture(1, mode->width/2, mode->height/2, 1);
	video_gl_init_texture(2, mode->width/2, mode->height/2, 1);

	/* The movie shader combines the three planes and retains display correction. */
	GLuint fragmentShader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glShaderSourceARB(fragmentShader, 1, &fragmentShaderSourceYUV, NULL);
	glCompileShaderARB(fragmentShader);
	shaderProgram = glCreateProgramObjectARB();
	glAttachObjectARB(shaderProgram, fragmentShader);
	glLinkProgramARB(shaderProgram);
	glUseProgramObjectARB(shaderProgram);
	glUniform1iARB(glGetUniformLocationARB(shaderProgram, "tex"), 0);
	glUniform1iARB(glGetUniformLocationARB(shaderProgram, "utex"), 1);
	glUniform1iARB(glGetUniformLocationARB(shaderProgram, "vtex"), 2);
	video_gl_set_color_correction(lastGamma, lastBrightness);
	return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Upload a decoded movie frame and compose it into the window back buffer.
 * Restore presentation state, update full-resolution luma and half-resolution
 * chroma planes, then leave the final swap to the video timing path.
 */
int video_gl_prepare_yuv_frame(yuv_frame *frame)
{
	/* Reassert window state so previous offscreen work cannot redirect the movie. */
	gpu_gl_prepare_present();
	video_gl_setup_screen();
	glUseProgramObjectARB(shaderProgram);
	glDisable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_DEPTH_TEST);

	/* Each texture unit remains paired with the sampler configured for its plane. */
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gltexture[0]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureWidth, textureHeight,
			textureColorFormat[0], texturePixelFormat[0], frame->lum);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gltexture[1]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureWidth/2, textureHeight/2,
			textureColorFormat[1], texturePixelFormat[1], frame->cr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gltexture[2]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureWidth/2, textureHeight/2,
			textureColorFormat[2], texturePixelFormat[2], frame->cb);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	/* Compose all three updated planes with the current display aspect setting. */
	render();
	return 1;
}

// display the rendered frame on the screen
int video_gl_display_yuv_frame(void)
{
	SDL_GL_SwapWindow(window);
	return 1;
}

#endif
