/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
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
#include "SDL2_framerate.h"
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
extern FPSmanager framerate_manager;
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

// calculates scale factors and offsets based on viewport and texture sizes
void video_gl_setup_screen()
{
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

// initializes the OpenGL texture for the game screen
void video_gl_init_texture(int index, int width, int height, int bytesPerPixel)
{
	int allocTextureWidth, allocTextureHeight;
	SDL_Surface *surface;

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

int video_gl_set_mode(s_videomodes videomodes)
{
	GLint maxTextureSize;

	displayWidth = textureWidth = videomodes.hRes;
	displayHeight = textureHeight = videomodes.vRes;

	// use the current monitor resolution in fullscreen mode to prevent aspect ratio distortion
	viewportWidth = savedata.fullscreen ? nativeWidth : (int)(videomodes.hRes * MAX(0.25,videomodes.hScale));
	viewportHeight = savedata.fullscreen ? nativeHeight : (int)(videomodes.vRes * MAX(0.25,videomodes.vScale));

	// zero width/height means close the window, not make it enormous!
	if((viewportWidth == 0) || (viewportHeight == 0)) return 0;

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

	// get window and initialize OpenGL context
	SetVideoMode(viewportWidth, viewportHeight, 0, true);
	if(!window)
	{
		printf("Failed to create OpenGL-compatible window (%s)...", SDL_GetError());
		goto error;
	}
	if((context = SDL_GL_GetCurrentContext()))
		SDL_GL_DeleteContext(context);
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

	// try to disable vertical retrace syncing (VSync)
	if(SDL_GL_SetSwapInterval(!!savedata.vsync) < 0)
	{
		printf("Warning: can't disable vertical retrace sync (%s)...\n", SDL_GetError());
	}

#ifdef LOADGL
	// load OpenGL functions dynamically in Linux/Windows/OSX
	if(LoadGLFunctions() == 0) goto error;
#endif

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

	// set background to black
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

	// set up a GLSL fragment shader if supported
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
	return 1;
error:
	printf("falling back to SDL video backend\n");
	opengl = 0;
	savedata.usegl = 0;
	return 0;
}

void video_gl_clearscreen()
{
	// clear both buffers in a double-buffered setup
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapWindow(window);
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapWindow(window);
}

void video_gl_draw_quad(int x, int y, int width, int height)
{
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

int video_gl_copy_screen(s_videosurface* surface)
{
	// update texture contents with new surface contents
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gltexture[0]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureWidth, textureHeight, textureColorFormat[0],
			texturePixelFormat[0], surface->data);

	// set linear filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// render the frame
	render();

	// display the rendered frame on the screen
	SDL_GL_SwapWindow(window);

#if WIN || LINUX
	// limit framerate to 200 fps
	SDL_framerateDelay(&framerate_manager);
#endif

	return 1;
}

// Set the brightness and gamma values
int lastGamma = 0, lastBrightness = 0;
void video_gl_set_color_correction(int gamma, int brightness)
{
	lastGamma = gamma;
	lastBrightness = brightness;
	GLint brightnessLocation, gammaLocation;
	if(gamma < -256) gamma = -256;
	if(gamma > 256) gamma = 256;
	if(brightness < -256) brightness = -256;
	if(brightness > 256) brightness = 256;

	glUseProgramObjectARB(shaderProgram);
	brightnessLocation = glGetUniformLocationARB(shaderProgram, "brightness");
	glUniform1fARB(brightnessLocation, brightness / 256.0);
	gammaLocation = glGetUniformLocationARB(shaderProgram, "gamma");
	glUniform1fARB(gammaLocation, gamma / 256.0);
}

// set up YUV mode
int video_gl_setup_yuv_overlay(const yuv_video_mode *mode)
{
	textureWidth = mode->width;
	textureHeight = mode->height;
	displayWidth = mode->display_width;
	displayHeight = mode->display_height;
	video_gl_init_texture(0, mode->width, mode->height, 1);
	video_gl_init_texture(1, mode->width/2, mode->height/2, 1);
	video_gl_init_texture(2, mode->width/2, mode->height/2, 1);

	// set up shader to do YUV->RGB conversion
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

// render the frame
int video_gl_prepare_yuv_frame(yuv_frame *frame)
{
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
