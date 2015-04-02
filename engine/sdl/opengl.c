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

static GLuint gltexture;
static GLint textureTarget;
static GLint textureColorFormat;
static GLint texturePixelFormat;

static int viewportWidth, viewportHeight;      // dimensions of display area
static int textureWidth, textureHeight;        // dimensions of game screen and GL texture
static int bytesPerPixel;

static GLfloat tcx, tcy; // maximum x and y texture coords in floating-point form

// use some variables declared in video.c that are common to both backends
extern FPSmanager framerate_manager;
extern int stretch;
extern int nativeWidth, nativeHeight;
extern SDL_Window* window;

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
void video_gl_init_textures()
{
	int allocTextureWidth, allocTextureHeight;
	SDL_Surface *surface;

	// set texture target, format, etc.
	textureTarget = GL_TEXTURE_2D;
	textureColorFormat = (bytesPerPixel == 4) ? GL_RGBA : GL_RGB;
	texturePixelFormat = (bytesPerPixel <= 2) ? GL_UNSIGNED_SHORT_5_6_5_REV : GL_UNSIGNED_BYTE;

	// enable 2D textures
	glEnable(textureTarget);

	// use non-power-of-two texture dimensions if they are available
	if(SDL_GL_ExtensionSupported("GL_ARB_texture_non_power_of_two"))
	{
		allocTextureWidth = textureWidth;
		allocTextureHeight = textureHeight;
	}
	else
	{
		allocTextureWidth = nextpowerof2(textureWidth);
		allocTextureHeight = nextpowerof2(textureHeight);
	}

	// calculate maximum texture coordinates
	tcx = (textureWidth == 0) ? 0 : (float)textureWidth / (float)allocTextureWidth;
	tcy = (textureHeight == 0) ? 0 : (float)textureHeight / (float)allocTextureHeight;

	// allocate a temporary surface to initialize the texture with
	surface = SDL_CreateRGBSurface(0, allocTextureWidth, allocTextureHeight, bytesPerPixel * 8, 0,0,0,0);

	// create texture object
	glDeleteTextures(1, &gltexture);
	glGenTextures(1, &gltexture);
	glBindTexture(textureTarget, gltexture);
	glTexImage2D(textureTarget, 0, textureColorFormat, allocTextureWidth,
			allocTextureHeight, 0, textureColorFormat, texturePixelFormat, surface->pixels);
	glTexParameteri(textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);

	// free up the temporary surface
	SDL_FreeSurface(surface);
	surface = NULL;
}

int video_gl_set_mode(s_videomodes videomodes)
{
	GLint maxTextureSize;

	bytesPerPixel = videomodes.pixel;
	textureWidth = videomodes.hRes;
	textureHeight = videomodes.vRes;

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
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

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
	if(SDL_GL_SetSwapInterval(0) < 0)
	{
		printf("Warning: can't disable vertical retrace sync (%s)...\n", SDL_GetError());
	}

#ifdef LOADGL
	// load OpenGL functions dynamically in Linux/Windows/OSX
	if(LoadGLFunctions() == 0) goto error;
#endif

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
	video_gl_init_textures();

	// set up offsets, scale factors, and viewport
	video_gl_setup_screen();

	video_gl_set_color_correction(savedata.gamma, savedata.brightness);

	opengl = 1;
	return 1;
error:
	printf("falling back on SDL video backend\n");
	opengl = 0;
	savedata.usegl[savedata.fullscreen] = 0;
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
	glBegin(GL_QUADS);
		// Top left
		glMultiTexCoord2f(GL_TEXTURE0, 0.0, tcy);
		glMultiTexCoord2f(GL_TEXTURE1, 0.0, tcy);
		glVertex2i(x, y);

		// Top right
		glMultiTexCoord2f(GL_TEXTURE0, tcx, tcy);
		glMultiTexCoord2f(GL_TEXTURE1, tcx, tcy);
		glVertex2i(x+width-1, y);

		// Bottom right
		glMultiTexCoord2f(GL_TEXTURE0, tcx, 0.0);
		glMultiTexCoord2f(GL_TEXTURE1, tcx, 0.0);
		glVertex2i(x+width-1, y+height-1);

		// Bottom left
		glMultiTexCoord2f(GL_TEXTURE0, 0.0, 0.0);
		glMultiTexCoord2f(GL_TEXTURE1, 0.0, 0.0);
		glVertex2i(x, y+height-1);
	glEnd();
}

int video_gl_copy_screen(s_videosurface* surface)
{
	int xOffset, yOffset; // offset of game screen on display area
	int scaledWidth, scaledHeight; // dimensions of game screen after scaling

	glClear(GL_COLOR_BUFFER_BIT);

	// update texture contents with new surface contents
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(textureTarget, gltexture);
	glTexSubImage2D(textureTarget, 0, 0, 0, textureWidth, textureHeight, textureColorFormat,
			texturePixelFormat, surface->data);

	// determine x and y scale factors
	float texScale = MIN((float)viewportWidth/(float)textureWidth, (float)viewportHeight/(float)textureHeight);

	// determine on-screen dimensions
	scaledWidth = (int)(textureWidth * texScale);
	scaledHeight = (int)(textureHeight * texScale);

	// determine offsets
	xOffset = (viewportWidth - scaledWidth) / 2;
	yOffset = (viewportHeight - scaledHeight) / 2;

	// set texture scaling type
	if((savedata.glfilter[savedata.fullscreen]) || (textureWidth == (stretch ? viewportWidth : scaledWidth)))
	{
		glTexParameteri(textureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	else
	{
		glTexParameteri(textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	// render the quad
	if(stretch)
		video_gl_draw_quad(0, 0, viewportWidth, viewportHeight);
	else
		video_gl_draw_quad(xOffset, yOffset, scaledWidth, scaledHeight);

	// blit the image to the screen
	SDL_GL_SwapWindow(window);

#if WIN || LINUX
	// limit framerate to 200 fps
	SDL_framerateDelay(&framerate_manager);
#endif

	return 1;
}

// Set the brightness and gamma values
void video_gl_set_color_correction(int gamma, int brightness)
{
	int numTexEnvStages = 0;

	if(gamma < -256) gamma = -256;
	if(gamma > 256) gamma = 256;
	if(brightness < -256) brightness = -256;
	if(brightness > 256) brightness = 256;
	glColor4f(abs(gamma / 256.0), abs(gamma / 256.0), abs(gamma / 256.0), abs(brightness / 256.0));

	if(SDL_GL_ExtensionSupported("GL_ARB_texture_env_combine"))
	{
		if(gamma > 0)
		{
			numTexEnvStages = 3;

			// first stage: c*g
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(textureTarget, gltexture);
			glEnable(textureTarget);
		    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,  GL_COMBINE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PRIMARY_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

			// second stage: (1.0-c)*(1.0-prev) = (1.0-c)*(1.0-(c*g))
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(textureTarget, gltexture);
			glEnable(textureTarget);
		    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,  GL_COMBINE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_ONE_MINUS_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PREVIOUS);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_ONE_MINUS_SRC_COLOR);

			// third stage: 1.0-prev = 1.0-((1.0-c)*(1.0-(c*g)))
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(textureTarget, gltexture);
			glEnable(textureTarget);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_ONE_MINUS_SRC_COLOR);
		}
		else if(gamma < 0)
		{
			numTexEnvStages = 2;

			// first stage: (1.0-c)*g
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(textureTarget, gltexture);
			glEnable(textureTarget);
		    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,  GL_COMBINE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_ONE_MINUS_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PRIMARY_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

			// second stage: c*(1.0-prev) = c*(1.0-((1.0-c)*g))
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(textureTarget, gltexture);
			glEnable(textureTarget);
		    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,  GL_COMBINE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PREVIOUS);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_ONE_MINUS_SRC_COLOR);

			// third stage: disabled
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(textureTarget, 0);
			glDisable(textureTarget);
		}
		else // gamma == 0 -> no gamma correction
		{
			numTexEnvStages = 0;

			// first stage: sampled color from texture
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(textureTarget, gltexture);
			glEnable(textureTarget);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,  GL_REPLACE);

			// second stage: disabled
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(textureTarget, 0);
			glDisable(textureTarget);

			// third stage: disabled
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(textureTarget, 0);
			glDisable(textureTarget);
		}
	}

	if(brightness)
	{
		// brightness correction
		GLfloat blendColor = brightness > 0 ? 1.0 : 0.0; // white (positive brightess) or black (0 or negative)
		GLfloat rgba[4] = {blendColor, blendColor, blendColor, 1.0};

		glActiveTexture(GL_TEXTURE0 + numTexEnvStages);
		glBindTexture(textureTarget, gltexture);
		glEnable(textureTarget);
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, rgba);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, numTexEnvStages > 0 ? GL_PREVIOUS : GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_CONSTANT);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SRC2_RGB, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_ONE_MINUS_SRC_ALPHA);
	}
}

#endif
