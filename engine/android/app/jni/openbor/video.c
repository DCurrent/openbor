/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) OpenBOR Team
 *
 * Video.c - adjunct to the main build's video.c.
 * Made by UTunnels (utunnels@hotmail.com).
 * Modifications by CRxTRDude, White Dragon and msmalik681.
 */

#include <math.h>
#include <limits.h>
#include "types.h"
#include "video.h"
#include "vga.h"
#include "screen.h"
#include "sdlport.h"
#include "openbor.h"
#include "gfx.h"
#include "videocommon.h"
#include "gpu.h"
#include "gpu_renderer.h"
#include "gpu_gl_api.h"

#include "pngdec.h"

extern int videoMode;

SDL_Window *window = NULL;
static SDL_GLContext video_context;
static GLuint video_program, video_vertices;
static GLint video_target_uniform, video_source_uniform, video_color_uniform;
static GLint video_textured_uniform, video_correction_uniform;
static GLint video_gamma_uniform, video_brightness_uniform;
static SDL_atomic_t video_reset_pending;
static int video_watch_installed, video_mode_ready, video_suspended;
static int video_transition, video_recovering, video_failed, video_warning;
static SDL_GLContext video_failed_context;
static const s_screen *launcher_owner;
static int launcher_frame_valid;
static int launcher_warning;
static s_screen *movie_screen;
static int movie_frame_valid;

/* Touch artwork is immutable GPU source storage, rebuilt after context loss. */
static GLuint buttons;
static int buttons_width, buttons_height;
static int video_upload_buttons(SDL_Surface *surface);

s_videomodes stored_videomodes;
yuv_video_mode stored_yuv_mode;

char windowTitle[MAX_LABEL_LEN] = {"OpenBOR"};

int stretch = 1;
int opengl = 0;

int nativeWidth, nativeHeight;           // resolution of device screen
static int textureWidth, textureHeight;  // dimensions of game screen and texture

int brightness = 0;
static int video_gamma;

#include "button_png_800x480.h" // default button skin


void initSDL()
{
    SDL_DisplayMode mode;
    int init_flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC;
    const char *var = SDL_getenv("SDL_VIDEO_FULLSCREEN_DISPLAY");
    int vm;


#ifdef CUSTOM_SIGNAL_HANDLER
    init_flags |= SDL_INIT_NOPARACHUTE;
#endif

    if(SDL_Init(init_flags) < 0)
    {
        printf("SDL Failed to Init!!!! (%s)\n", SDL_GetError());
        borExit(0);
    }
    SDL_ShowCursor(SDL_DISABLE);
    SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
    //atexit(SDL_Quit); //White Dragon: use SDL_Quit() into sdlport.c it's best practice!

    if ( !var )
    {
        var = SDL_getenv("SDL_VIDEO_FULLSCREEN_HEAD");
    }
    if ( var )
    {
        vm = SDL_atoi(var);
    }
    else
    {
        vm = 0;
    }

    // Store the monitor's current resolution before setting the video mode for the first time
    if(SDL_GetDesktopDisplayMode(vm, &mode) == 0)
    {
        nativeWidth = mode.w;
        nativeHeight = mode.h;
    }
    else
    {
        nativeWidth = 640;
        nativeHeight = 480;
    }

		//Hardcode full screen mode
    savedata.fullscreen = 1;

}

//Start of touch control UI code

#define DOCKLEFT 8
#define DOCKRIGHT 2
#define DOCKTOP 1
#define DOCKBOTTOM 4
static int screendocking = DOCKTOP; //0 center if possible, 1 top 2 right 4 bottom 8 left

float bx[MAXTOUCHB];
float by[MAXTOUCHB];
float br[MAXTOUCHB];
unsigned touchstates[MAXTOUCHB];
#define _RE(x,y) {144*(x),144*(y),96,96}

SDL_Rect btnsrc[MAXTOUCHB] =
{
    [SDID_MOVEUP] = _RE(0, 2), [SDID_MOVERIGHT] = _RE(1, 2), [SDID_MOVEDOWN] = _RE(2, 2), [SDID_MOVELEFT] = _RE(3, 2), //dpad
    [SDID_SPECIAL] = _RE(4, 0), [SDID_ATTACK2] = _RE(1, 0), [SDID_JUMP] = _RE(1, 1), [SDID_ATTACK] = _RE(0, 0), //special a2 jump a1
    [SDID_START] = _RE(2, 1), [SDID_ESC] = _RE(3, 1), [SDID_ATTACK3] = _RE(2, 0), [SDID_ATTACK4] = _RE(3, 0), //start esc a3 a4
    [SDID_SCREENSHOT] = _RE(4, 1)
};
SDL_Rect btndes[MAXTOUCHB];

/*
Default touch UI
*/

static void setup_touch_default()
{
    float w = nativeWidth;
    float h = nativeHeight;
    float hh = w * 480 / 800;
    float ra = 0.15f, rb = ra / 1.75f;
    float cx1 = (ra + rb + rb / 5.0f) * hh, cy1 = h - cx1;
    float cy2 = cy1;
    //dpad
    bx[SDID_MOVEUP] = cx1;
    by[SDID_MOVEUP] = cy1 - ra * hh;
    br[SDID_MOVEUP] = rb * hh;
    bx[SDID_MOVERIGHT] = bx[SDID_MOVEUP] + ra * hh;
    by[SDID_MOVERIGHT] = cy1;
    br[SDID_MOVERIGHT] = br[SDID_MOVEUP];
    bx[SDID_MOVEDOWN] = bx[SDID_MOVEUP];
    by[SDID_MOVEDOWN] = cy1 + ra * hh;
    br[SDID_MOVEDOWN] = br[SDID_MOVEUP];
    bx[SDID_MOVELEFT] = bx[SDID_MOVEUP] - ra * hh;
    by[SDID_MOVELEFT] = cy1;
    br[SDID_MOVELEFT] = br[SDID_MOVEUP];
    //special a2 jump a1
    bx[SDID_SPECIAL] = w - bx[SDID_MOVEUP];
    by[SDID_SPECIAL] = cy2 - ra * hh;
    br[SDID_SPECIAL] = rb * hh;
    bx[SDID_ATTACK2] = bx[SDID_SPECIAL] + ra * hh;
    by[SDID_ATTACK2] = cy2;
    br[SDID_ATTACK2] = br[SDID_MOVEUP];
    bx[SDID_JUMP] = bx[SDID_SPECIAL];
    by[SDID_JUMP] = cy2 + ra * hh;
    br[SDID_JUMP] = br[SDID_MOVEUP];
    bx[SDID_ATTACK] = bx[SDID_SPECIAL] - ra * hh;
    by[SDID_ATTACK] = cy2;
    br[SDID_ATTACK] = br[SDID_MOVEUP];
    //start, esc
    bx[SDID_START] = bx[SDID_MOVELEFT];
    by[SDID_START] = h - by[SDID_MOVEDOWN];
    br[SDID_START] = br[SDID_MOVELEFT];
    bx[SDID_ESC] = bx[SDID_ATTACK2];
    by[SDID_ESC] = h - by[SDID_JUMP];
    br[SDID_ESC] = br[SDID_ATTACK2];
    //a3 a4
    bx[SDID_ATTACK3] = w / 2.0f - ra * hh;
    by[SDID_ATTACK3] = by[SDID_MOVEDOWN];
    br[SDID_ATTACK3] = br[SDID_MOVEDOWN];
    bx[SDID_ATTACK4] = w / 2.0f + ra * hh;
    by[SDID_ATTACK4] = by[SDID_JUMP];
    br[SDID_ATTACK4] = br[SDID_JUMP];
    //screenshot
    bx[SDID_SCREENSHOT] = w / 2.0f;
    by[SDID_SCREENSHOT] = h / 2.0f;
    br[SDID_SCREENSHOT] = br[SDID_MOVEDOWN];


    screendocking = DOCKTOP;
}

/*
Code to use touch.txt to displace buttons and set up skin.
*/

static int setup_touch_txt()
{
    int pos, i, sdid, t;
    static char filename[MAX_FILENAME_LEN];
    static char pakname[MAX_FILENAME_LEN];
    static char touchfilename[MAX_FILENAME_LEN];
    char *buf, *command, *value;
    size_t size;
    ArgList arglist;
    char argbuf[MAX_ARG_LEN + 1] = "";
    float w = nativeWidth;
    float h = nativeHeight;
    float hh = w * 480 / 800;
    SDL_Surface *ts;
    char *pngb;
    size_t pngs;

    getPakName(pakname, -1);
    sprintf(filename, "%s/%s", savesDir, pakname);
    dirExists(filename, 1);
    sprintf(filename, "%s/%s/touch.txt", savesDir, pakname);
    sprintf(touchfilename, "%s/touch.txt", savesDir);
    
    // Kratus (02-2023) Rearranged the touch path order, external first and then the internal pak/data folder
    // Read file
    if( buffer_pakfile(filename, &buf, &size) != 1 &&
        buffer_pakfile(touchfilename, &buf, &size) != 1 &&
        buffer_pakfile("data/touch.txt", &buf, &size) != 1 )
    {
        return 0;
    }

    // Now interpret the contents of buf line by line
    pos = 0;
    while(pos < size)
    {
        if(ParseArgs(&arglist, buf + pos, argbuf))
        {
            command = GET_ARG(0);
            if(command && command[0])
            {
                if(stricmp(command, "button") == 0)
                {
                    sdid = translate_SDID(GET_ARG(1));
                    if(sdid >= 0)
                    {
                        bx[sdid] = GET_FLOAT_ARG(2) * hh;
                        by[sdid] = GET_FLOAT_ARG(3) * hh;
                        br[sdid] = GET_FLOAT_ARG(4) * hh;
                        t = GET_INT_ARG(5);
                        /*
                        corners:
                        0     1

                        3     2
                        */
                        if(t == 1 || t == 2)
                        {
                            bx[sdid] = w - bx[sdid];
                        }
                        if(t == 2 || t == 3)
                        {
                            by[sdid] = h - by[sdid];
                        }
                    }
                }
                // CRxTRDude 11/15/14 - Added 'skin' as a replacement to
								//					'texture' command. Texture is still retained
								//					for backwards compatibility.
                else if((stricmp(command, "texture") == 0) || (stricmp(command, "skin") == 0))
                {
                    if(buffer_pakfile(GET_ARG(1), &pngb, &pngs))
                    {
                        ts = pngToSurface(pngb);
                        if(!ts || !video_upload_buttons(ts))
                        {
                            printf("error: %s\n", SDL_GetError());
                        }
                        if(ts)
                        {
                            SDL_FreeSurface(ts);
                        }
                        if(pngb)
                        {
                            free(pngb);
                        }
                    }
                }
                else if(stricmp(command, "screendocking") == 0)
                {
                    screendocking = 0;
                    for(i = 1; ; i++)
                    {
                        value = GET_ARG(i);
                        if(!value || !value[0])
                        {
                            break;
                        }
                        if(stricmp(value, "left") == 0)
                        {
                            screendocking |= DOCKLEFT;
                        }
                        else if(stricmp(value, "right") == 0)
                        {
                            screendocking |= DOCKRIGHT;
                        }
                        else if(stricmp(value, "top") == 0)
                        {
                            screendocking |= DOCKTOP;
                        }
                        else if(stricmp(value, "bottom") == 0)
                        {
                            screendocking |= DOCKBOTTOM;
                        }
                        else
                        {
                            screendocking = GET_INT_ARG(i);
                        }
                    }
                }
            }
        }

        // Go to next line
        pos += getNewLineStart(buf + pos);
    }

    if(buf != NULL)
    {
        free(buf);
        buf = NULL;
    }

    return 1;
}

static void setup_touch()
{
    int i;
    setup_touch_default();
    setup_touch_txt();

    for(i = 0; i < MAXTOUCHB; i++)
    {
        btndes[i].x = bx[i] - br[i];
        btndes[i].y = by[i] - br[i];
        btndes[i].h = btndes[i].w = 2 * br[i];
    }
}
//End of touch control UI code

/* Android native GPU presentation boundary. */

enum
{
    VIDEO_PRESENT_TOUCH = 0,
    VIDEO_PRESENT_SCENE = 1,
    VIDEO_PRESENT_MOVIE = 2
};

/*
 * Damon V. Caskey
 * 2026-11-20
 * Android owns one GLES3 context. Scene targets, palettes, glyphs, transforms,
 * movies and output filters use the shared renderer. This final pass owns only
 * the window quad and immutable touch artwork; it never uploads scene pixels.
 */
static const char *video_vertex_source =
    "#version 300 es\n"
    "precision highp float;\n"
    "in vec2 position; in vec2 texcoord;\n"
    "uniform vec2 targetSize; out vec2 uv;\n"
    "void main(){ uv=texcoord; gl_Position=vec4(position.x*2.0/targetSize.x-1.0,"
    "1.0-position.y*2.0/targetSize.y,0.0,1.0); }\n";
static const char *video_fragment_source =
    "#version 300 es\n"
    "precision highp float;\n"
    "precision highp sampler2D;\n"
    "uniform sampler2D sourceTexture; uniform vec4 modulation;\n"
    "uniform int textured; uniform int correction;\n"
    "uniform float gammaValue; uniform float brightnessValue;\n"
    "in vec2 uv; out vec4 outputColor;\n"
    "void main(){ vec4 c=textured!=0?texture(sourceTexture,uv)*modulation:modulation;\n"
    "if(correction==1){\n"
    "if(gammaValue>0.0) c.rgb=1.0-((1.0-c.rgb)*(1.0-c.rgb*gammaValue));\n"
    "else c.rgb=c.rgb*(1.0-((1.0-c.rgb)*-gammaValue));\n"
    "if(brightnessValue>0.0) c.rgb=mix(c.rgb,vec3(1.0),brightnessValue);\n"
    "else c.rgb=mix(c.rgb,vec3(0.0),-brightnessValue); }\n"
    "if(correction!=0) c.a=1.0;\n"
    "outputColor=c; }\n";

/* Damon V. Caskey - 2026-11-20: bound repeated presentation diagnostics. */
static int video_reject(const char *reason)
{
    if(!video_warning)
    {
        gpu_log("WARNING: Android native video failed at %s; output skipped without CPU retry.", reason);
        video_warning = 1;
    }
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * An unexpected context loss cannot reconstruct arbitrary script-authored
 * pixels from source assets. Stop explicitly rather than expose stale shadows.
 */
static void video_lost_contents(void)
{
    gpu_log("ERROR: Android graphics context was lost with unsynchronized GPU screen contents. "
            "Stopping rather than restoring stale pixels or selecting CPU rendering.");
    fprintf(stderr, "Android GPU screen contents were lost; rendering cannot safely continue.\n");
    abort();
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * SDL may report device reset from another thread. Only publish an atomic flag;
 * all preservation, reconstruction and GL work runs on the engine thread.
 */
static int SDLCALL video_native_watch(void *unused, SDL_Event *event)
{
    (void)unused;
    if(event && (event->type == SDL_RENDER_TARGETS_RESET ||
                 event->type == SDL_RENDER_DEVICE_RESET))
        SDL_AtomicSet(&video_reset_pending, 1);
    return 1;
}

/* Damon V. Caskey - 2026-11-20: compile the small GLES3 window shader. */
static GLuint video_compile_shader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    GLint compiled = 0;
    char message[2048] = {0};
    if(!shader) return 0;
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if(!compiled)
    {
        GLsizei length = 0;
        glGetShaderInfoLog(shader, sizeof(message), &length, message);
        message[sizeof(message) - 1] = 0;
        gpu_log("ERROR: Android presentation shader failed: %s.", message);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

/* Damon V. Caskey - 2026-11-20: release only names belonging to this context. */
static void video_destroy_presenter(int context_lost)
{
    if(!context_lost)
    {
        if(buttons) glDeleteTextures(1, &buttons);
        if(video_vertices) glDeleteBuffers(1, &video_vertices);
        if(video_program) glDeleteProgram(video_program);
    }
    buttons = video_vertices = video_program = 0;
    buttons_width = buttons_height = 0;
}

/* Damon V. Caskey - 2026-11-20: initialize buffered final composition. */
static int video_create_presenter(void)
{
    GLuint vertex = video_compile_shader(GL_VERTEX_SHADER, video_vertex_source);
    GLuint fragment = video_compile_shader(GL_FRAGMENT_SHADER, video_fragment_source);
    GLint linked = 0;
    if(!vertex || !fragment)
    {
        if(vertex) glDeleteShader(vertex);
        if(fragment) glDeleteShader(fragment);
        return 0;
    }
    video_program = glCreateProgram();
    if(video_program)
    {
        glAttachShader(video_program, vertex);
        glAttachShader(video_program, fragment);
        glBindAttribLocation(video_program, 0, "position");
        glBindAttribLocation(video_program, 1, "texcoord");
        glLinkProgram(video_program);
        glGetProgramiv(video_program, GL_LINK_STATUS, &linked);
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    if(!linked)
    {
        char message[2048] = {0};
        if(video_program) glGetProgramInfoLog(video_program, sizeof(message), NULL, message);
        message[sizeof(message) - 1] = 0;
        gpu_log("ERROR: Android presentation program failed: %s.", message);
        return 0;
    }
    video_target_uniform = glGetUniformLocation(video_program, "targetSize");
    video_source_uniform = glGetUniformLocation(video_program, "sourceTexture");
    video_color_uniform = glGetUniformLocation(video_program, "modulation");
    video_textured_uniform = glGetUniformLocation(video_program, "textured");
    video_correction_uniform = glGetUniformLocation(video_program, "correction");
    video_gamma_uniform = glGetUniformLocation(video_program, "gammaValue");
    video_brightness_uniform = glGetUniformLocation(video_program, "brightnessValue");
    glGenBuffers(1, &video_vertices);
    return video_vertices && video_target_uniform >= 0 && video_source_uniform >= 0 &&
        video_color_uniform >= 0 && video_textured_uniform >= 0 &&
        video_correction_uniform >= 0 && video_gamma_uniform >= 0 &&
        video_brightness_uniform >= 0 && glGetError() == GL_NO_ERROR;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Decode/normalize touch artwork once, then keep its RGBA pixels resident.
 * This is asset upload, independent of all screen composition and readback.
 */
static int video_upload_buttons(SDL_Surface *surface)
{
    SDL_Surface *rgba;
    GLuint replacement = 0;
    int row;
    if(!surface || surface->w <= 0 || surface->w > INT_MAX / 4 || surface->h <= 0) return 0;
    rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ABGR8888, 0);
    if(!rgba) return 0;
    if(SDL_MUSTLOCK(rgba) && SDL_LockSurface(rgba) < 0)
    {
        SDL_FreeSurface(rgba);
        return 0;
    }
    gpu_gl_invalidate_state();
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &replacement);
    glBindTexture(GL_TEXTURE_2D, replacement);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if(rgba->pitch == rgba->w * 4)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rgba->pixels);
    else
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        for(row = 0; row < rgba->h; ++row)
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, row, rgba->w, 1, GL_RGBA,
                            GL_UNSIGNED_BYTE, (const unsigned char *)rgba->pixels + (size_t)row * rgba->pitch);
    }
    if(SDL_MUSTLOCK(rgba)) SDL_UnlockSurface(rgba);
    if(!replacement || glGetError() != GL_NO_ERROR)
    {
        if(replacement) glDeleteTextures(1, &replacement);
        SDL_FreeSurface(rgba);
        return 0;
    }
    if(buttons) glDeleteTextures(1, &buttons);
    buttons = replacement;
    buttons_width = rgba->w;
    buttons_height = rgba->h;
    SDL_FreeSurface(rgba);
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Preserve native screen owners before controlled destruction. Lost GL names
 * are abandoned without deletion, and only preserved CPU shadows can recover.
 */
static void video_release_resources(int context_lost)
{
    gpu_gl_set_context_guard(NULL);
    /* Fullscreen movie storage is derived from retained decoder planes, never
     * a script-visible owner. Discard it before preserving genuine screens;
     * lost contexts must not delete numeric names that may already be reused. */
    if(movie_screen)
    {
        if(context_lost || video_recovering) gpu_gl_discard_screen(movie_screen);
        /* A backend guard can arrive with this screen already passed as its
         * destination. Keep the owner allocation alive until that call returns;
         * its discarded pixels cannot be presented until preparation completes
         * entirely within the new context generation. */
        if(!video_recovering) freescreen(&movie_screen);
    }
    if(context_lost)
    {
        if(!gpu_gl_abandon()) video_lost_contents();
    }
    else gpu_gl_shutdown();
    video_destroy_presenter(context_lost);
    movie_frame_valid = 0;
    launcher_frame_valid = 0;
    video_mode_ready = 0;
    opengl = 0;
}

/* Damon V. Caskey - 2026-11-20: create all resources before enabling drawing. */
static int video_create_resources(void)
{
    s_screen *probe = NULL;
    unsigned filtered_texture;
    int filtered_width, filtered_height, major = 0, profile = 0;
    if(SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major) < 0 ||
       SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profile) < 0 ||
       major < 3 || !(profile & SDL_GL_CONTEXT_PROFILE_ES))
    {
        gpu_log("ERROR: Android requires an OpenGL ES 3.0 or newer context.");
        return 0;
    }
    SDL_GL_GetDrawableSize(window, &nativeWidth, &nativeHeight);
    if(nativeWidth <= 0 || nativeHeight <= 0 || !gpu_gl_init() || !video_create_presenter())
        return 0;
    /* Validate the selected filter before accepting a mode, not on its first
     * visible frame. The probe has no retained owner and is never presented. */
    if(savedata.swfilter)
    {
        probe = allocscreen(textureWidth, textureHeight, PIXEL_32);
        if(!probe || !gpu_clear_screen(probe) ||
           !gpu_gl_get_filtered_screen_texture(probe, savedata.swfilter,
                &filtered_texture, &filtered_width, &filtered_height))
        {
            if(probe) freescreen(&probe);
            return 0;
        }
        freescreen(&probe);
    }
    setup_touch();
    if(!buttons)
    {
        SDL_Surface *surface = pngToSurface(buttonpng);
        int uploaded = surface && video_upload_buttons(surface);
        if(surface) SDL_FreeSurface(surface);
        if(!uploaded) return 0;
    }
    if(SDL_GL_SetSwapInterval(savedata.fpslimit == 1 ? 1 : 0) < 0)
        gpu_log("WARNING: Android could not apply the requested swap interval: %s.", SDL_GetError());
    video_mode_ready = 1;
    video_failed = 0;
    video_warning = 0;
    video_failed_context = NULL;
    opengl = 1;
    savedata.usegl = 1;
    gpu_gl_set_context_guard(video_ensure_context);
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * The backend checks this boundary before any scene access. SDL can create a
 * replacement context during resume; adopt it only after old names have been
 * abandoned. Failed initialization stays latched until a new context/reset or
 * explicit mode change, preventing repeated compile/allocation/log work.
 */
int video_ensure_context(void)
{
    SDL_GLContext current = SDL_GL_GetCurrentContext();
    SDL_GLContext previous;
    int reset = SDL_AtomicGet(&video_reset_pending);
    if(video_transition) return current && current == video_context;
    if(video_suspended) return 0;
    if(!window || stored_videomodes.hRes <= 0 || stored_videomodes.vRes <= 0) return 0;
    if(!current && video_context && !reset &&
       SDL_GL_MakeCurrent(window, video_context) == 0)
        current = SDL_GL_GetCurrentContext();
    if(video_mode_ready && current && current == video_context && !reset && gpu_is_active())
        return 1;
    if(video_failed && current == video_failed_context && !reset) return 0;

    video_transition = 1;
    video_recovering = 1;
    previous = video_context;
    SDL_AtomicSet(&video_reset_pending, 0);
    video_release_resources(reset || current != previous);
    /* SDL owns an automatically created replacement until the app adopts it.
     * Do not delete stale GL object names or destroy that replacement context. */
    if(current && current != previous)
    {
        video_context = current;
        /* SDL's Android restore creates a replacement without destroying the
         * failed EGL context. Retire that handle after abandoning its GL names;
         * SDL leaves the distinct current replacement bound. */
        if(previous) SDL_GL_DeleteContext(previous);
    }
    else
    {
        if(previous) SDL_GL_DeleteContext(previous);
        video_context = SDL_GL_CreateContext(window);
    }
    if(!video_context || SDL_GL_MakeCurrent(window, video_context) < 0 ||
       !video_create_resources())
    {
        video_release_resources(SDL_GL_GetCurrentContext() != video_context);
        video_failed = 1;
        video_failed_context = SDL_GL_GetCurrentContext();
        video_recovering = 0;
        video_transition = 0;
        return video_reject("context restoration");
    }
    video_recovering = 0;
    video_transition = 0;
    gpu_log("Android native GPU context restored from preserved screen contents.");
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Lifecycle events are processed on the render thread before SDL's Android
 * pump detaches and pauses the context. Snapshot live screens once here, never
 * per frame. Foreground restoration then reuses or safely recreates residency.
 */
void video_handle_event(const SDL_Event *event)
{
    if(!event) return;
    if(event->type == SDL_APP_WILLENTERBACKGROUND)
    {
        if(video_mode_ready && !video_suspended)
        {
            if(!video_ensure_context()) video_lost_contents();
            /* Decoder planes remain available, so fullscreen output need not
             * add a drawable-sized readback to the genuine screen checkpoint. */
            if(movie_screen) freescreen(&movie_screen);
            movie_frame_valid = 0;
            if(!gpu_gl_suspend()) video_lost_contents();
        }
        video_suspended = 1;
    }
    else if(event->type == SDL_APP_DIDENTERFOREGROUND)
    {
        video_suspended = 0;
        if(stored_videomodes.hRes > 0) video_ensure_context();
    }
    else if((event->type == SDL_RENDER_TARGETS_RESET ||
             event->type == SDL_RENDER_DEVICE_RESET) &&
            SDL_AtomicGet(&video_reset_pending) && !video_suspended)
    {
        /* The event watch already recorded this notification. A reset consumed
         * during foreground restoration must not rebuild the new context twice. */
        video_ensure_context();
    }
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Required GLES scene initialization is atomic from the caller's perspective.
 * Every supported scene remains RGB32; initialization failure never enables
 * CPU rasterization. Ordinary mode changes preserve all surviving screen data.
 */
int video_set_mode(s_videomodes mode)
{
    int shutdown = mode.hRes == 0 && mode.vRes == 0;
    SDL_GLContext current;
    if(!shutdown && (mode.pixel != 4 || mode.hRes <= 0 || mode.vRes <= 0 ||
       savedata.swfilter < BLITTER_NONE || savedata.swfilter >= BLITTER_MAX))
    {
        gpu_log("ERROR: Android video mode requires valid RGB32 dimensions and a supported GPU output filter.");
        return 0;
    }
    gpu_set_native_required(1);
    video_transition = 1;
    current = SDL_GL_GetCurrentContext();
    if(video_context || gpu_is_active())
    {
        int lost = SDL_AtomicGet(&video_reset_pending) || current != video_context;
        if(!current && video_context && !SDL_AtomicGet(&video_reset_pending) &&
           SDL_GL_MakeCurrent(window, video_context) == 0)
        {
            lost = 0;
            current = SDL_GL_GetCurrentContext();
        }
        video_release_resources(lost);
        /* A queued Android reset may have installed a distinct SDL context
         * before explicit mode teardown. Both handles belong to this video
         * lifetime; no replacement may be left alive and unowned. */
        if(current && current != video_context) SDL_GL_DeleteContext(current);
        if(video_context) SDL_GL_DeleteContext(video_context);
        video_context = NULL;
    }
    launcher_owner = NULL;
    launcher_frame_valid = launcher_warning = 0;
    movie_frame_valid = 0;
    stored_videomodes = mode;
    SDL_AtomicSet(&video_reset_pending, 0);
    video_failed = video_warning = video_suspended = 0;
    video_failed_context = NULL;
    if(shutdown)
    {
        video_transition = 0;
        return 0;
    }
    textureWidth = mode.hRes;
    textureHeight = mode.vRes;
    savedata.fullscreen = 1;
    if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES) < 0 ||
       SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) < 0 ||
       SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0) < 0 ||
       SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8) < 0 ||
       SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8) < 0 ||
       SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8) < 0 ||
       SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8) < 0 ||
       SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) < 0)
        goto failed;
    if(!window)
        window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            nativeWidth, nativeHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL);
    if(!window) goto failed;
    if(!video_watch_installed)
    {
        SDL_AddEventWatch(video_native_watch, NULL);
        video_watch_installed = 1;
    }
    video_context = SDL_GL_CreateContext(window);
    if(!video_context || SDL_GL_MakeCurrent(window, video_context) < 0 ||
       !video_create_resources()) goto failed;
    video_transition = 0;
    gpu_log("Android scene rendering: NATIVE - OpenGL ES 3, RGB32 targets; no CPU scene upload or rendering retry.");
    return 1;
failed:
    video_release_resources(SDL_GL_GetCurrentContext() != video_context);
    video_failed = 1;
    video_failed_context = SDL_GL_GetCurrentContext();
    video_transition = 0;
    return video_reject("required GLES3 mode initialization");
}

/* Damon V. Caskey - 2026-11-20: retire native resources before SDL teardown. */
void video_shutdown(void)
{
    s_videomodes shutdown = {0};
    video_set_mode(shutdown);
    if(video_watch_installed)
    {
        SDL_DelEventWatch(video_native_watch, NULL);
        video_watch_installed = 0;
    }
    if(window) SDL_DestroyWindow(window);
    window = NULL;
}

/* Damon V. Caskey - 2026-11-20: a failed launcher command rejects its frame. */
static int video_launcher_reject(const char *reason)
{
    launcher_frame_valid = 0;
    if(!launcher_warning)
    {
        gpu_log("WARNING: Android GPU launcher failed at %s; frame skipped without CPU retry.", reason);
        launcher_warning = 1;
    }
    return 0;
}

/* Damon V. Caskey - 2026-11-20: ordinary native screen ownership serves the launcher. */
int video_launcher_begin(s_screen *target)
{
    launcher_owner = target;
    launcher_frame_valid = 0;
    if(!target || target->pixelformat != PIXEL_32 ||
       target->width != textureWidth || target->height != textureHeight ||
       !video_ensure_context() || !gpu_clear_screen(target))
        return video_launcher_reject("composition start");
    launcher_frame_valid = 1;
    return 1;
}

/* Damon V. Caskey - 2026-11-20: maintain complete-frame launcher ordering. */
static int video_launcher_current(const s_screen *target)
{
    if(!target || target != launcher_owner || !launcher_frame_valid || !video_ensure_context())
        return 0;
    /* Recovery can invalidate the frame while preserving the screen owner. */
    return launcher_frame_valid;
}

/* Damon V. Caskey - 2026-11-20: consume transient previews through native image residency. */
int video_launcher_screen(s_screen *target, s_screen *source, int x, int y)
{
    if(!video_launcher_current(target) || !gpu_draw_screen(target, source, x, y, NULL))
        return video_launcher_reject("image composition");
    return 1;
}

/* Damon V. Caskey - 2026-11-20: preserve opaque launcher panel order. */
int video_launcher_box(s_screen *target, int x, int y, int width, int height, unsigned color)
{
    if(!video_launcher_current(target) || !gpu_draw_box(target, x, y, width, height, color, NULL))
        return video_launcher_reject("box composition");
    return 1;
}

/* Damon V. Caskey - 2026-11-20: share the native resident font atlas on Android. */
int video_launcher_text(s_screen *target, int x, int y, unsigned foreground,
                         unsigned background, int fill, const unsigned char *glyphs,
                         size_t count, const unsigned char *font_rows, size_t font_glyph_count)
{
    if(!video_launcher_current(target) || !gpu_gl_draw_launcher_text(target, x, y,
        foreground, background, fill, glyphs, count, font_rows, font_glyph_count))
        return video_launcher_reject("text composition");
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Submit one textured rectangle in top-left window coordinates. Native image
 * row zero is texture row zero; explicit coordinates avoid SDL texture caches
 * and preserve the shared renderer's logical screen orientation. Scene/movie
 * output is opaque RGB32 regardless of its unused source byte; only touch
 * artwork carries real alpha. Movie colors were corrected before YUV clamp.
 */
static void video_draw_rectangle(GLuint image, float x, float y, float width, float height,
    float u0, float v0, float u1, float v1, float opacity, int correction, int linear)
{
    const GLfloat vertices[] = {
        x, y, u0, v0, x + width, y, u1, v0, x, y + height, u0, v1,
        x + width, y, u1, v0, x + width, y + height, u1, v1, x, y + height, u0, v1
    };
    glBindTexture(GL_TEXTURE_2D, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear ? GL_LINEAR : GL_NEAREST);
    glUniform4f(video_color_uniform, 1.0f, 1.0f, 1.0f, opacity);
    glUniform1i(video_textured_uniform, 1);
    glUniform1i(video_correction_uniform, correction);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    /* Borrowed native images always return with their established sampler. */
    if(linear)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
}

/* Damon V. Caskey - 2026-11-20: retain SDL's clipped source-rectangle behavior. */
static void video_draw_button(int index)
{
    SDL_Rect source = btnsrc[index], dest = btndes[index];
    int left = MAX(0, source.x), top = MAX(0, source.y);
    int64_t right = MIN((int64_t)buttons_width, (int64_t)source.x + source.w);
    int64_t bottom = MIN((int64_t)buttons_height, (int64_t)source.y + source.h);
    if(source.w <= 0 || source.h <= 0 || dest.w <= 0 || dest.h <= 0 ||
       left >= right || top >= bottom) return;
    /* SDL_RenderCopy clips the source, then stretches that intersection over
     * the complete requested destination. Do not shrink the destination. */
    video_draw_rectangle(buttons, dest.x, dest.y, dest.w, dest.h,
        (float)left / buttons_width, (float)top / buttons_height,
        (float)right / buttons_width, (float)bottom / buttons_height,
        (touchstates[index] ? 128.0f : 64.0f) / 255.0f,
        VIDEO_PRESENT_TOUCH, !savedata.hwfilter);
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Present a resident native texture, apply display correction, then draw touch
 * controls. Movie targets are already corrected at final resolution so their
 * second pass is a neutral one-to-one copy. No scene readback/upload occurs.
 */
static int video_present_texture(GLuint image, int width, int height, int movie)
{
    int i, held = 0, drawable_width, drawable_height;
    int hide_touch;
    float output_width, output_height, x, y;
    float corrected_black = brightness > 0 ? brightness / 256.0f : 0.0f;
    extern int hide_t;
    /* The texture was borrowed after the caller's context guard. Rebuilding
     * here would invalidate that numeric name, so reject a changed context and
     * let the next owner-level operation reconstruct/reacquire its resource. */
    if(!image || width <= 0 || height <= 0 || !video_mode_ready ||
       SDL_GL_GetCurrentContext() != video_context || SDL_AtomicGet(&video_reset_pending))
        return video_reject("context changed before presentation");
    SDL_GL_GetDrawableSize(window, &drawable_width, &drawable_height);
    if(drawable_width <= 0 || drawable_height <= 0) return video_reject("window dimensions");
    if(drawable_width != nativeWidth || drawable_height != nativeHeight)
    {
        nativeWidth = drawable_width;
        nativeHeight = drawable_height;
        setup_touch();
    }
    if(movie && (width != nativeWidth || height != nativeHeight))
        return video_reject("movie viewport changed after preparation");
    gpu_gl_prepare_present();
    glViewport(0, 0, nativeWidth, nativeHeight);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DITHER);
    glDisable(GL_BLEND);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(corrected_black, corrected_black, corrected_black, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(video_program);
    glBindBuffer(GL_ARRAY_BUFFER, video_vertices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (const void *)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (const void *)(2 * sizeof(GLfloat)));
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(video_source_uniform, 0);
    glUniform2f(video_target_uniform, nativeWidth, nativeHeight);
    glUniform1f(video_gamma_uniform, video_gamma / 256.0f);
    glUniform1f(video_brightness_uniform, brightness / 256.0f);
    output_width = movie || stretch ? nativeWidth : nativeHeight * ((float)width / height);
    output_height = nativeHeight;
    x = movie || stretch ? 0.0f : (int)(nativeWidth / 2.0f - output_width / 2.0f);
    y = 0.0f;
    /* Existing Android aspect behavior is height-fitted, with window clipping
     * for a source wider than the device. Keep its integer destination width. */
    video_draw_rectangle(image, x, y, (int)output_width, output_height,
                         0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
                         movie ? VIDEO_PRESENT_MOVIE : VIDEO_PRESENT_SCENE,
                         !movie && !savedata.hwfilter);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    hide_touch = timer_gettick() > hide_t;
    for(i = 0; i < MAXTOUCHB; ++i)
    {
        held += touchstates[i];
        if(!hide_touch) video_draw_button(i);
    }
    if(held) hide_t = timer_gettick() + 5000;
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
    gpu_gl_invalidate_state();
    if(glGetError() != GL_NO_ERROR || SDL_AtomicGet(&video_reset_pending))
        return video_reject("window or touch draw");
    SDL_GL_SwapWindow(window);
    video_warning = 0;
    return 1;
}

/* Damon V. Caskey - 2026-11-20: present ordinary GPU screens and the launcher identically. */
int video_copy_screen(s_screen *source)
{
    unsigned image;
    int width, height;
    int is_launcher = source && source == launcher_owner;
    if(is_launcher && !video_launcher_current(source))
        return video_launcher_reject("incomplete frame");
    if(!source || source->pixelformat != PIXEL_32 || !video_ensure_context() ||
       !gpu_gl_get_filtered_screen_texture(source, savedata.swfilter, &image, &width, &height) ||
       (is_launcher && !launcher_frame_valid) ||
       !video_present_texture(image, width, height, 0))
    {
        if(is_launcher) return video_launcher_reject("filtered presentation");
        return video_reject("scene presentation");
    }
    if(is_launcher) launcher_warning = 0;
    SDL_Delay(1);
    return 1;
}

/* Damon V. Caskey - 2026-11-20: clear the real window without touching CPU pixels. */
void video_clearscreen(void)
{
    if(!video_ensure_context()) return;
    gpu_gl_prepare_present();
    glViewport(0, 0, nativeWidth, nativeHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void video_stretch(int enable)
{
    stretch = enable;
}

void video_fullscreen_flip(void)
{
    /* Android remains fullscreen. */
}

void vga_vwait(void)
{
    static int prevtick;
    int now = SDL_GetTicks();
    int wait = 1000 / 60 - (now - prevtick);
    SDL_Delay(wait > 0 ? wait : 1);
    prevtick = now;
}

/* Damon V. Caskey - 2026-11-20: share desktop display-control range and equations. */
void video_set_color_correction(int gamma, int value)
{
    video_gamma = MAX(-256, MIN(256, gamma));
    brightness = MAX(-256, MIN(256, value));
}

/* Damon V. Caskey - 2026-11-20: finish movies without retiring gameplay residency. */
int video_restore_mode(void)
{
    if(movie_screen) freescreen(&movie_screen);
    movie_frame_valid = 0;
    return video_ensure_context();
}

/* Damon V. Caskey - 2026-11-20: movies use the same scene backend and all fixed kernels. */
int video_setup_yuv_overlay(const yuv_video_mode *mode)
{
    if(!mode || mode->width < 2 || mode->height < 2 ||
       (mode->width & 1) || (mode->height & 1) ||
       mode->display_width <= 0 || mode->display_height <= 0 || !video_ensure_context())
        return 0;
    stored_yuv_mode = *mode;
    movie_frame_valid = 0;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Sample YUV directly at the final drawable resolution. Keeping this target
 * separate from gameplay preserves its scene and avoids double reconstruction
 * when cubic or Lanczos kernels are selected for fullscreen movies.
 */
int video_prepare_yuv_frame(const gpu_yuv_source *source, e_drawmethod_resample resample)
{
    gpu_movie_draw draw = {0};
    int width, height;
    uint64_t generation;
    double output_width;
    movie_frame_valid = 0;
    if(!source || source->width != stored_yuv_mode.width ||
       source->height != stored_yuv_mode.height || !video_ensure_context()) return 0;
    generation = gpu_gl_get_context_generation();
    SDL_GL_GetDrawableSize(window, &width, &height);
    if(width <= 0 || height <= 0) return 0;
    if(movie_screen && (movie_screen->width != width || movie_screen->height != height))
        freescreen(&movie_screen);
    if(!movie_screen) movie_screen = allocscreen(width, height, PIXEL_32);
    if(!movie_screen) return video_reject("movie target allocation");
    output_width = stretch ? width : (double)height * stored_yuv_mode.display_width / stored_yuv_mode.display_height;
    if(output_width < 1.0 || output_width > INT_MAX) return 0;
    draw.width = (int)output_width;
    draw.height = height;
    draw.x = stretch ? 0 : (int)(width / 2.0 - output_width / 2.0);
    draw.resample = resample;
    draw.gamma = video_gamma / 256.0f;
    draw.brightness = brightness / 256.0f;
    /* Android applies brightness across the complete display, including bars. */
    if(brightness > 0)
    {
        unsigned c = (unsigned)(brightness * 255.0f / 256.0f + 0.5f);
        if(!gpu_draw_box(movie_screen, 0, 0, width, height, c | (c << 8) | (c << 16), NULL)) return 0;
    }
    else if(!gpu_clear_screen(movie_screen)) return 0;
    if(!gpu_draw_yuv(movie_screen, source, &draw) ||
       generation != gpu_gl_get_context_generation()) return 0;
    movie_frame_valid = 1;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Movie capture uses native YUV conversion and the same display correction.
 * The screen remains GPU-owned until the screenshot encoder explicitly reads.
 */
int video_capture_yuv_frame(s_screen *screen, const gpu_yuv_source *source,
                            e_drawmethod_resample resample)
{
    gpu_movie_draw draw = {0};
    if(!screen || screen->pixelformat != PIXEL_32 || !video_ensure_context()) return 0;
    draw.width = screen->width;
    draw.height = screen->height;
    draw.resample = resample;
    draw.gamma = video_gamma / 256.0f;
    draw.brightness = brightness / 256.0f;
    return gpu_draw_yuv(screen, source, &draw);
}

/* Damon V. Caskey - 2026-11-20: display a prepared movie while retaining all scene resources. */
int video_display_yuv_frame(void)
{
    unsigned image;
    int width, height;
    if(!video_ensure_context() || !movie_frame_valid || !movie_screen ||
       !gpu_gl_get_screen_texture(movie_screen, &image, &width, &height) ||
       !movie_frame_valid) return 0;
    return video_present_texture(image, width, height, 1);
}

int video_current_refresh_rate(void)
{
    SDL_DisplayMode mode;
    if(!window || SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window), &mode) != 0)
        return 60;
    return mode.refresh_rate;
}
