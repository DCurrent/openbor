/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

/************************ OpenGL video backend *************************/
/*
 * New video backend that uses OpenGL as a cross-platform API for optimized
 * hardware blitting and scaling.  Supports both OpenGL 1.2+ and OpenGL ES 1.x.

 * It also offers better fullscreen than software SDL because the picture is
 * guaranteed to display at the correct aspect ratio and not have black bars on
 * all 4 sides.
 */

#ifdef OPENGL

#include "SDL.h"
#include <math.h>
#include "openbor.h"
#include "opengl.h"
#include "video.h"
#include "sdlport.h"
#include "loadgl.h"

#ifdef SDL2
#include "SDL2_framerate.h"
#else
#include "SDL_framerate.h"
#endif

#define nextpowerof2(x) pow(2,ceil(log(x)/log(2)))
#define abs(x)			((x<0)?-(x):(x))

#if SDL2
static SDL_GLContext context = NULL;
#else
static SDL_Surface* context = NULL;
#endif

static SDL_Surface* bscreen = NULL; // FIXME: unnecessary SDL dependency; this should be an s_screen

static int textureDepths[4] = {16,16,24,32};
static const char* glExtensions;

static float ycompress = 1.0;

static GLushort glpalette[256];
static GLuint gltexture;

static GLint textureTarget;
static GLint textureInternalColorFormat;
static GLint textureColorFormat;
static GLint texturePixelFormat;

static int viewportWidth, viewportHeight;      // dimensions of display area
static int scaledWidth, scaledHeight;          // dimensions of game screen after scaling
static int textureWidth, textureHeight;        // dimensions of game screen and GL texture
static int xOffset, yOffset;                   // offset of game screen on display area
static int bytesPerPixel;

#ifdef GLES
static GLfixed tcx, tcy; // maximum x and y texture coords in fixed-point form
#else
static GLfloat tcx, tcy; // maximum x and y texture coords in floating-point form
#endif

// use some variables declared in video.c that are common to both backends
extern s_videomodes stored_videomodes;
extern FPSmanager framerate_manager;
extern int stretch;
extern int nativeWidth, nativeHeight;
#if SDL2
extern SDL_Window* window;
#endif

// Macros for compatibility between OpenGL ES and normal OpenGL
#ifdef GLES
#define glOrtho(l,r,b,t,n,f) glOrthox((l)<<16,(r)<<16,(b)<<16,(t)<<16,(n)<<16,(f)<<16)
#define glTexParameter glTexParameterx
#define BOR_UNSIGNED_SHORT_5_6_5 GL_UNSIGNED_SHORT_5_6_5
#define BOR_CLAMP GL_CLAMP_TO_EDGE
#else
#define glTexParameter glTexParameteri
#define BOR_UNSIGNED_SHORT_5_6_5 GL_UNSIGNED_SHORT_5_6_5_REV
#define BOR_CLAMP GL_CLAMP
#endif

// SDL 1.2 and SDL 2.0 use different calls for SwapBuffers
#if SDL2
#define SwapBuffers() SDL_GL_SwapWindow(window)
#else
#define SwapBuffers() SDL_GL_SwapBuffers()
#endif

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
	bool npotTexturesSupported;

	// set texture target, format, etc.
	textureTarget = GL_TEXTURE_2D;
	textureColorFormat = (bytesPerPixel == 4) ? GL_RGBA : GL_RGB;
	textureInternalColorFormat = textureColorFormat;
	texturePixelFormat = (bytesPerPixel <= 2) ? BOR_UNSIGNED_SHORT_5_6_5 : GL_UNSIGNED_BYTE;

	// enable 2D textures
	glEnable(textureTarget);

	// detect support for non-power-of-two texture dimensions
#ifdef GLES
	npotTexturesSupported = strstr(glExtensions, "GL_IMG_texture_npot") || strstr(glExtensions, "GL_OES_texture_npot");
#else
	npotTexturesSupported = strstr(glExtensions, "GL_ARB_texture_non_power_of_two") ? true : false;
#endif

	/*
	 * FIXME: NPOT textures are disabled for now because they cause a software fallback with
	 * a few drivers for older Nvidia cards. But POT textures eat up more memory than
	 * is necessary on computers with NPOT texture support. There has to be a
	 * better solution to this problem than just disabling them for everyone...the
	 * "GL_*_texture_rectangle" extensions might solve the problem in some cases.
	 */
	npotTexturesSupported = false;

	if(npotTexturesSupported)
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
#ifdef GLES
	tcx = (textureWidth == 0) ? 0 : (textureWidth<<16) / allocTextureWidth;
	tcy = (textureHeight == 0) ? 0 : (textureHeight<<16) / allocTextureHeight;
#else
	tcx = (textureWidth == 0) ? 0 : (float)textureWidth / (float)allocTextureWidth;
	tcy = (textureHeight == 0) ? 0 : (float)textureHeight / (float)allocTextureHeight;
#endif

	// allocate a surface to initialize the texture with
	bscreen = SDL_CreateRGBSurface(SDL_SWSURFACE, allocTextureWidth, allocTextureHeight, textureDepths[bytesPerPixel-1], 0,0,0,0);

	// create texture object
	glDeleteTextures(1, &gltexture);
	glGenTextures(1, &gltexture);
	glBindTexture(textureTarget, gltexture);
	glTexImage2D(textureTarget, 0, textureInternalColorFormat, allocTextureWidth,
			allocTextureHeight, 0, textureColorFormat, texturePixelFormat, bscreen->pixels);
	glTexParameter(textureTarget, GL_TEXTURE_WRAP_S, BOR_CLAMP);
	glTexParameter(textureTarget, GL_TEXTURE_WRAP_T, BOR_CLAMP);

	// we don't need bscreen anymore, except in 8-bit mode
	SDL_FreeSurface(bscreen);
	bscreen = NULL;

	if(bytesPerPixel == 1) bscreen = SDL_CreateRGBSurface(SDL_SWSURFACE, textureWidth, textureHeight, 16, 0,0,0,0);
}

int video_gl_set_mode(s_videomodes videomodes)
{
	GLint maxTextureSize;

	bytesPerPixel = videomodes.pixel;
	textureWidth = videomodes.hRes;
	textureHeight = videomodes.vRes;

	// use the current monitor resolution in fullscreen mode to prevent aspect ratio distortion
	viewportWidth = savedata.fullscreen ? nativeWidth : (int)(videomodes.hRes * MAX(0.25,savedata.glscale));
	viewportHeight = savedata.fullscreen ? nativeHeight : (int)(videomodes.vRes * MAX(0.25,savedata.glscale));

	// zero width/height means close the window, not make it enormous!
	if((viewportWidth == 0) || (viewportHeight == 0)) return 0;

	// set up OpenGL double buffering
	if(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) != 0)
	{
		printf("Can't set up OpenGL double buffering (%s)...", SDL_GetError());
		goto error;
	}

	// try to disable vertical retrace syncing (VSync)
#ifdef SDL2
	if(SDL_GL_SetSwapInterval(0) < 0)
#else
	if(SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 0) != 0)
#endif
	{
		printf("Warning: can't disable vertical retrace sync (%s)\n", SDL_GetError());
	}

#if SDL2 && defined(GLES)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_EGL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif SDL2
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_EGL, 0);
#endif

	// free existing surfaces
	if(bscreen) { SDL_FreeSurface(bscreen); bscreen=NULL; }

	// create main video surface and initialize OpenGL context
#if SDL2
	SetVideoMode(viewportWidth, viewportHeight, 0, true);
	if(!window)
	{
		printf("Failed to create OpenGL-compatible window (%s)...", SDL_GetError());
		goto error;
	}
	context = SDL_GL_CreateContext(window);
#else
	context = SDL_SetVideoMode(viewportWidth, viewportHeight, 0, savedata.fullscreen ? (SDL_OPENGL|SDL_FULLSCREEN) : SDL_OPENGL);
#endif

	// make sure the surface was created successfully
	if(!context)
	{
		printf("OpenGL initialization failed (%s)...", SDL_GetError());
		goto error;
	}

	// update viewport size based on actual dimensions
#if SDL2
	SDL_GL_MakeCurrent(window, context);
	SDL_GetWindowSize(window, &viewportWidth, &viewportHeight);
#else
	viewportWidth = context->w;
	viewportHeight = context->h;
#endif

#ifdef LOADGL
	// load OpenGL functions dynamically in Linux/Windows/OSX
	if(LoadGLFunctions() == 0) goto error;
#endif

	// reject the MesaGL software renderer (swrast) because it's much slower than SDL software blitting
	if(stricmp((const char*)glGetString(GL_RENDERER), "Software Rasterizer") == 0)
	{
		printf("Not going to use the Mesa software renderer...");
		goto error;
	}

	// get the list of available extensions
	glExtensions = (const char*)glGetString(GL_EXTENSIONS);

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

#ifdef GLES
	// enable vertex/texcoord arrays in OpenGL ES since we can't use glBegin()/glEnd()
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#endif

	// initialize texture object
	video_gl_init_textures();

	// set up offsets, scale factors, and viewport
	video_gl_setup_screen();

	// FIXME: don't use savedata for this; use the previous brightness and gamma (will need to coordinate with software SDL)
	video_gl_set_color_correction(savedata.gamma, savedata.brightness);

	opengl = 1;
	return 1;
error:
	printf("falling back on SDL video backend\n");
	if(bscreen) { SDL_FreeSurface(bscreen); bscreen=NULL; }
	opengl = 0;
	savedata.usegl[savedata.fullscreen] = 0;
	return 0;
}

void video_gl_clearscreen()
{
	// clear both buffers in a double-buffered setup
	glClear(GL_COLOR_BUFFER_BIT);
	SwapBuffers();
	glClear(GL_COLOR_BUFFER_BIT);
	SwapBuffers();
}

void video_gl_fullscreen_flip()
{
	savedata.fullscreen ^= 1;
	video_set_mode(stored_videomodes);
}

void video_gl_draw_quad(int x, int y, int width, int height)
{
#ifdef GLES
	// OpenGL ES supports neither quads nor glBegin()/glEnd()  :(
	GLshort box[] = {x,y,    x+width-1,y,  x+width-1,y+height-1,  x,y+height-1};
	GLfixed tex[] = {0,tcy,  tcx,tcy,      tcx,0,                 0,0,        };
	GLubyte indices[] = {0,1,3,2};

	glActiveTexture(GL_TEXTURE0);
	glTexCoordPointer(2, GL_FIXED, 0, tex);
	glActiveTexture(GL_TEXTURE1);
	glTexCoordPointer(2, GL_FIXED, 0, tex);
	glVertexPointer(2, GL_SHORT, 0, box);
	glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, indices);
#else
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
#endif
}

int video_gl_copy_screen(s_screen* src)
{
	unsigned char *sp;
	unsigned char *dp;
	int width, height, linew, slinew;
	int h, i;
	float texScale;
	SDL_Surface* ds = NULL;

	// Convert 8-bit s_screen to a 16-bit SDL_Surface
	if(bscreen)
	{
		if(SDL_MUSTLOCK(bscreen)) SDL_LockSurface(bscreen);

		width = bscreen->w;
		if(width > src->width) width = src->width;
		height = bscreen->h;
		if(height > src->height) height = src->height;
		if(!width || !height) return 0;
		h = height;

		sp = (unsigned char*)src->data;
		ds = bscreen;
		dp = ds->pixels;

		linew = width*bytesPerPixel;
		slinew = src->width*bytesPerPixel;

		do{
			//u16pcpy((unsigned short*)dp, sp, glpalette, linew);
			i=linew-1;
			do
			{
			   ((unsigned short*)dp)[i] = glpalette[sp[i]];
			}while(i--);
			sp += slinew;
			dp += ds->pitch;
		}while(--h);

		if(SDL_MUSTLOCK(bscreen)) SDL_UnlockSurface(bscreen);
	}

	glClear(GL_COLOR_BUFFER_BIT);

	// update texture contents with new surface contents
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(textureTarget, gltexture);
	glTexSubImage2D(textureTarget, 0, 0, 0, textureWidth, textureHeight, textureColorFormat,
			texturePixelFormat, bscreen ? bscreen->pixels : src->data);

	// determine x and y scale factors
	texScale = MIN((float)viewportWidth/(float)textureWidth, (float)viewportHeight/(float)textureHeight);

	// determine on-screen dimensions
	scaledWidth = (int)(textureWidth * texScale);
	scaledHeight = (int)(textureHeight * texScale) * ycompress;

	// determine offsets
	xOffset = (viewportWidth - scaledWidth) / 2;
	yOffset = (viewportHeight - scaledHeight) / 2;

	// set texture scaling type
	if((savedata.glfilter[savedata.fullscreen]) || (textureWidth == (stretch ? viewportWidth : scaledWidth)))
	{
		glTexParameter(textureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameter(textureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	else
	{
		glTexParameter(textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameter(textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	// render the quad
	if(stretch)
		video_gl_draw_quad(0, 0, viewportWidth, viewportHeight);
	else
		video_gl_draw_quad(xOffset, yOffset, scaledWidth, scaledHeight);

	// blit the image to the screen
	SwapBuffers();

#if WIN || LINUX
	// limit framerate to 200 fps
	SDL_framerateDelay(&framerate_manager);
#endif

	return 1;
}

void video_gl_setpalette(unsigned char* pal)
{
	int i;
	for(i=0; i<256; i++)
	{
		glpalette[i] = colour16(pal[0], pal[1], pal[2]);
		pal += 3;
	}
}

// Set the brightness and gamma values
void video_gl_set_color_correction(int gamma, int brightness)
{
	int numTexEnvStages = 0;

#ifdef GLES
	// FIXME: color correction is broken with OpenGL ES
	return;
#endif

	if(gamma < -256) gamma = -256;
	if(gamma > 256) gamma = 256;
	if(brightness < -256) brightness = -256;
	if(brightness > 256) brightness = 256;
	glColor4f(abs(gamma / 256.0), abs(gamma / 256.0), abs(gamma / 256.0), abs(brightness / 256.0));

#if GLES
	if(gamma)
#else
	if(strstr(glExtensions, "GL_ARB_texture_env_combine"))
#endif
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

	//fprintf(stderr, "set brightness=%d and gamma=%d in %d texenv stages\n", brightness, gamma, numTexEnvStages + 1);
}

#endif
