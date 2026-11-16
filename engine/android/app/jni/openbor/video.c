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
#include "gpu_output_filter_android.h"
#include "gpu_launcher_android.h"

#include "pngdec.h"

extern int videoMode;

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;
SDL_Texture *texture_base = NULL;
/* Android retains its accelerated SDL fullscreen presenter until the shared
 * native scene renderer is ported. Movie storage is separate from game output. */
static SDL_Texture *movie_texture;
static e_drawmethod_resample movie_resample = DC_RESAMPLE_MOVIE;
static const void *movie_owner;
static uint64_t movie_generation;
static gpu_output_filter_android *output_filter;
static gpu_launcher_android *launcher;
static const s_screen *launcher_owner;
static SDL_GLContext launcher_context;
static SDL_atomic_t launcher_reset_pending;
static int launcher_watch_installed;
static int launcher_frame_valid;
static int launcher_init_failed;
static int launcher_warning;
static int launcher_rebuild_pending;
static int video_mode_ready;

//For Android - Textures and a surface for the buttons
SDL_Texture *buttons = NULL;

s_videomodes stored_videomodes;
yuv_video_mode stored_yuv_mode;

char windowTitle[MAX_LABEL_LEN] = {"OpenBOR"};

int stretch = 1;
int opengl = 0;

int nativeWidth, nativeHeight;           // resolution of device screen
static int textureWidth, textureHeight;  // dimensions of game screen and texture

int brightness = 0;

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
                        if(!ts || !(buttons = SDL_CreateTextureFromSurface(renderer, ts)))
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

/* Android launcher GPU presentation boundary. */

/*
 * Damon V. Caskey
 * 2026-11-16
 * SDL can announce renderer loss from another thread. Record only an atomic
 * notification here; the render thread retires resources and starts a new
 * composition before any launcher pixels can be presented again.
 */
static int SDLCALL video_launcher_watch(void *unused, SDL_Event *event)
{
    (void)unused;
    if(event && (event->type == SDL_RENDER_TARGETS_RESET ||
                 event->type == SDL_RENDER_DEVICE_RESET))
    {
        SDL_AtomicSet(&launcher_reset_pending, 1);
    }
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Failed launcher work poisons this frame, including idle re-presentation.
 * Keep its owner registered so stale CPU screen pixels cannot become a retry.
 * Rearm diagnostics only after a complete frame is successfully presented.
 */
static int video_launcher_reject(const char *reason)
{
    launcher_frame_valid = 0;
    gpu_launcher_android_invalidate(launcher);
    if(!launcher_warning)
    {
        gpu_log("WARNING: Android GPU launcher failed at %s; frame skipped without CPU retry.", reason);
        launcher_warning = 1;
    }
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Retire the private composition before its SDL renderer or screen owner.
 * Lost contexts require abandoning raw GL names, which may already refer to
 * unrelated objects in a replacement context. SDL still owns its textures.
 */
static void video_launcher_retire(int context_lost)
{
    if(launcher_watch_installed)
    {
        SDL_DelEventWatch(video_launcher_watch, NULL);
        launcher_watch_installed = 0;
    }
    launcher_frame_valid = 0;
    launcher_owner = NULL;
    if(context_lost) gpu_launcher_android_abandon(launcher);
    else gpu_launcher_android_destroy(launcher);
    launcher = NULL;
    launcher_context = NULL;
    launcher_init_failed = 0;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Reject a stale frame before accessing its target, atlas or source storage.
 * Ordinary game screens remain outside this launcher-only ownership boundary.
 */
static int video_launcher_current(const s_screen *target)
{
    if(launcher_context && SDL_GL_GetCurrentContext() != launcher_context)
    {
        SDL_AtomicSet(&launcher_reset_pending, 1);
    }
    if(!target || target != launcher_owner || !launcher || !renderer ||
       !video_mode_ready || !launcher_frame_valid ||
       SDL_AtomicGet(&launcher_reset_pending))
    {
        return video_launcher_reject("unavailable or invalidated composition");
    }
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Begin each launcher composition on its private RGB32 GPU target. Register
 * the requested owner even when initialization fails, so presentation cannot
 * fall through to the unrelated CPU scene upload. Reset recovery recreates
 * SDL presentation resources before replaying the caller's normal draw order.
 * Failed construction or mode recovery stays unavailable until a new mode,
 * context or reset event, bounding initialization attempts and diagnostics
 * while leaving command retry intact.
 */
int video_launcher_begin(s_screen *target)
{
    launcher_owner = target;
    launcher_frame_valid = 0;
    if(SDL_AtomicGet(&launcher_reset_pending) ||
       ((launcher || launcher_init_failed) &&
        SDL_GL_GetCurrentContext() != launcher_context))
    {
        launcher_rebuild_pending = 1;
    }
    if(launcher_rebuild_pending)
    {
        s_videomodes mode = stored_videomodes;
        if(!video_set_mode(mode))
        {
            launcher_owner = target;
            launcher_context = SDL_GL_GetCurrentContext();
            launcher_init_failed = 1;
            launcher_rebuild_pending = 0;
            /* Mode teardown removed the old watch. A fresh reset can permit
             * another recovery attempt; ordinary redraws must not repeatedly
             * recreate the renderer or compile an unavailable filter. */
            if(!launcher_watch_installed)
            {
                SDL_AddEventWatch(video_launcher_watch, NULL);
                launcher_watch_installed = 1;
            }
            return video_launcher_reject("renderer reset recovery");
        }
        launcher_owner = target;
        launcher_rebuild_pending = 0;
    }
    if(!video_mode_ready || !renderer || !texture || !target ||
       target->pixelformat != PIXEL_32 || target->width != textureWidth ||
       target->height != textureHeight || target->width <= 0 || target->height <= 0)
    {
        return video_launcher_reject("invalid RGB32 owner or video mode");
    }
    if(!launcher_watch_installed)
    {
        SDL_AddEventWatch(video_launcher_watch, NULL);
        launcher_watch_installed = 1;
    }
    if(!launcher)
    {
        if(launcher_init_failed)
            return video_launcher_reject("unavailable compositor initialization");
        launcher_context = SDL_GL_GetCurrentContext();
        launcher = gpu_launcher_android_create(renderer, target->width, target->height);
        if(!launcher)
        {
            launcher_init_failed = 1;
            return video_launcher_reject("composition allocation");
        }
        launcher_context = SDL_GL_GetCurrentContext();
    }
    if(SDL_AtomicGet(&launcher_reset_pending) ||
       !gpu_launcher_android_begin(launcher))
    {
        return video_launcher_reject("composition start");
    }
    launcher_frame_valid = 1;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Consume launcher backgrounds and previews synchronously, retaining no
 * borrowed screen pointer after return. Previews may be freed immediately.
 */
int video_launcher_screen(s_screen *target, s_screen *source, int x, int y)
{
    if(!video_launcher_current(target)) return 0;
    if(source == launcher_owner)
        return video_launcher_reject("GPU owner used as a CPU source asset");
    if(!gpu_launcher_android_screen(launcher, source, x, y))
        return video_launcher_reject("image composition");
    return 1;
}

/* Damon V. Caskey - 2026-11-16: preserve opaque launcher panel order on GPU. */
int video_launcher_box(s_screen *target, int x, int y, int width, int height, unsigned color)
{
    if(!video_launcher_current(target)) return 0;
    if(!gpu_launcher_android_box(launcher, x, y, width, height, color))
        return video_launcher_reject("box composition");
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * The common formatter supplies safe byte-mapped glyphs. Rasterize them at
 * their original call site without writing or synchronizing the CPU owner.
 */
int video_launcher_text(s_screen *target, int x, int y, unsigned foreground,
                         unsigned background, int fill, const unsigned char *glyphs,
                         size_t count, const unsigned char *font_rows, size_t font_glyph_count)
{
    if(!video_launcher_current(target)) return 0;
    if(!gpu_launcher_android_text(launcher, x, y, foreground, background, fill,
                                  glyphs, count, font_rows, font_glyph_count))
        return video_launcher_reject("text composition");
    return 1;
}

/* End Android launcher GPU presentation boundary. */

/*
Start of video code. Unlike the original code for video, everything is
incorporated in video_set_mode, since this is isolated from the main
code.
*/

/*
 * Damon V. Caskey
 * 2026-11-16
 * Initialize Android presentation for RGB32 scenes only. Optional output
 * filters use a GLES2 target before the normal SDL brightness/touch pass.
 * Failed filter initialization stops the mode without a CPU filter retry.
 */
int video_set_mode(s_videomodes videomodes)
{
    //hardcode flags
    int flags = SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL;
    int recreate_renderer;

    /* Zero dimensions request shutdown and carry no scene format. */
    if(videomodes.hRes != 0 || videomodes.vRes != 0)
    {
        if(videomodes.pixel != 4)
        {
            gpu_log("ERROR: Video mode rejected: OpenBOR scenes require RGB32 (4 bytes per pixel), received %d.", videomodes.pixel);
            return 0;
        }
        if(videomodes.hRes <= 0 || videomodes.vRes <= 0 ||
           savedata.swfilter < BLITTER_NONE || savedata.swfilter >= BLITTER_MAX)
        {
            gpu_log("ERROR: Android video mode rejected invalid dimensions or output-filter selection.");
            return 0;
        }
    }
    stored_videomodes = videomodes;
    savedata.fullscreen = 1;

    //destroy all
    video_mode_ready = 0;
    recreate_renderer = launcher_rebuild_pending || SDL_AtomicGet(&launcher_reset_pending) ||
        (launcher_context && SDL_GL_GetCurrentContext() != launcher_context);
    video_launcher_retire(recreate_renderer);
    SDL_AtomicSet(&launcher_reset_pending, 0);
    if(recreate_renderer) gpu_output_filter_android_abandon(output_filter);
    else gpu_output_filter_android_destroy(output_filter);
    output_filter = NULL;
    SDL_DestroyTexture(movie_texture);
    movie_texture = NULL;
    movie_owner = NULL;
    if(texture_base)
    {
        SDL_DestroyTexture(texture_base);
        texture_base = NULL;
    }
    if(texture)
    {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }
    if(buttons)
    {
        SDL_DestroyTexture(buttons);
        buttons = NULL;
    }
    if(recreate_renderer && renderer)
    {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }

    if(videomodes.hRes == 0 && videomodes.vRes == 0)
    {
        //Term_Gfx();
        launcher_rebuild_pending = 0;
        launcher_warning = 0;
        return 0;
    }

    videomodes = setupPreBlitProcessing(videomodes);
    if(!videomodes.pixel)
    {
        return 0;
    }
    assert(videomodes.pixel == 4);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    if(!window && !(window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, nativeWidth, nativeHeight, flags)))
    {
        printf("error: %s\n", SDL_GetError());
        return 0;
    }

    if(!renderer)
    {
        int index, selected = -1;
        SDL_RendererInfo candidate;
        for(index = 0; index < SDL_GetNumRenderDrivers(); ++index)
        {
            if(SDL_GetRenderDriverInfo(index, &candidate) == 0 &&
               candidate.name && !strcmp(candidate.name, "opengles2"))
            {
                selected = index;
                break;
            }
        }
        if(selected < 0 || !(renderer = SDL_CreateRenderer(window, selected,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE)))
        {
            gpu_log("ERROR: Android requires SDL's opengles2 presenter: %s.", SDL_GetError());
            return 0;
        }
    }

    //For status
    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer, &info);

    printf("SDL video Renderer: %s \n", info.name);

    SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, savedata.hwfilter ? "nearest" : "linear", SDL_HINT_DEFAULT);

    // now create a texture

  	textureWidth = videomodes.hRes;
  	textureHeight = videomodes.vRes;

    if(!texture_base)
    {
        SDL_Surface *tmp_surface = SDL_CreateRGBSurface(0, 800, 480, 32, 0, 0, 0, 0);
        if(!tmp_surface)
        {
            printf("error: %s\n", SDL_GetError());
            return 0;
        }
        SDL_FillRect(tmp_surface, NULL, SDL_MapRGB(tmp_surface->format, 0, 0, 0));
        SDL_SetSurfaceBlendMode(tmp_surface, SDL_BLENDMODE_NONE);
        if(!(texture_base = SDL_CreateTextureFromSurface(renderer, tmp_surface)))
        {
            printf("error: %s\n", SDL_GetError());
            return 0;
        }
        SDL_FreeSurface(tmp_surface);
        tmp_surface = NULL;
    }

    if(!(texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888,
        SDL_TEXTUREACCESS_STREAMING, textureWidth, textureHeight)))
    {
        printf("error: %s\n", SDL_GetError());
        return 0;
    }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
    if(savedata.swfilter)
    {
        output_filter = gpu_output_filter_android_create(renderer,
            textureWidth, textureHeight, savedata.swfilter);
        if(!output_filter) return 0;
        gpu_log("Android RGB32 GPU output filter %d enabled.", savedata.swfilter);
    }

    setup_touch();

    if(!buttons)
    {
        SDL_Surface *btn_screen = pngToSurface(buttonpng);
        if(!btn_screen || !(buttons = SDL_CreateTextureFromSurface(renderer, btn_screen)))
        {
            printf("error: %s\n", SDL_GetError());
            return 0;
        }
        SDL_FreeSurface(btn_screen);
        btn_screen = NULL;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    video_clearscreen();

    video_mode_ready = 1;
    launcher_rebuild_pending = 0;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Release fullscreen movie storage while retaining Android game textures and
 * touch controls. Movie selection no longer replaces their presentation state.
 */
int video_restore_mode(void)
{
    SDL_DestroyTexture(movie_texture);
    movie_texture = NULL;
    movie_owner = NULL;
    return renderer && texture;
}

void video_fullscreen_flip()
{
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Retain Android's final brightness, aspect and touch composition, reporting
 * SDL failures so a GPU-only launcher frame can be rebuilt without uploading
 * an obsolete CPU screen. Flush before present to observe queued draw errors.
 */
int blit(void)
{
    int i, h;
    int hide_touch;
    static int warned;
    extern int hide_t;

    if(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0) < 0 ||
       SDL_RenderClear(renderer) < 0) goto failed;

    //SDL_SetRenderTarget(renderer, NULL);
    //SDL_RenderSetLogicalSize(renderer, 0, 0);
    if(SDL_SetTextureBlendMode(texture_base, SDL_BLENDMODE_NONE) < 0 ||
       SDL_RenderCopy(renderer, texture_base, NULL, NULL) < 0) goto failed;

    if(stretch)
    {
        //SDL_RenderSetLogicalSize(renderer, 0, 0);
        if(SDL_RenderCopy(renderer, texture, NULL, NULL) < 0) goto failed;
    }
    else
    {
        //SDL_RenderSetLogicalSize(renderer, textureWidth, textureHeight);
        float aspectRatio = (float)textureWidth / (float)textureHeight;
        float newWidth = nativeHeight * aspectRatio;

        //SDL_Log("aspect: from %d/%d con %f, orig: %d/%d -> %d",textureWidth,textureHeight,aspectRatio,nativeWidth,nativeHeight,(int)newWidth);
        SDL_Rect d_rect = {(int)(nativeWidth/2.0f - newWidth/2.0f), 0, (int)newWidth, nativeHeight};
        if(SDL_RenderCopy(renderer, texture, NULL, &d_rect) < 0) goto failed;
    }

    if(brightness > 0)
    {
        if(SDL_SetRenderDrawColor(renderer, 255, 255, 255, brightness-1) < 0) goto failed;
    }
    else if(brightness < 0)
    {
        if(SDL_SetRenderDrawColor(renderer, 0, 0, 0, (-brightness)-1) < 0) goto failed;
    }
    if(SDL_RenderFillRect(renderer, NULL) < 0) goto failed;

    //SDL_RenderSetLogicalSize(renderer, 0, 0);
    hide_touch = (timer_gettick() > hide_t);
    for(i = 0, h = 0; i < MAXTOUCHB; i++)
    {
        h += touchstates[i];

        /*
            Kratus (02-2023) Now the screenshot key will always be shown
            The modder can simply move or disable it by using the touch.txt file
            The original code is commented in case any modder wants to use it
        */

        // if(!hide_touch && (i != SDID_SCREENSHOT || touchstates[i]))
        if(!hide_touch)
        {
            if(SDL_SetTextureAlphaMod(buttons, touchstates[i] ? 128 : 64) < 0 ||
               SDL_RenderCopy(renderer, buttons, &btnsrc[i], &btndes[i]) < 0) goto failed;
        }
    }
    if(h)
    {
        hide_t = timer_gettick() + 5000;
    }
    if(SDL_RenderFlush(renderer) < 0) goto failed;
    SDL_RenderPresent(renderer);
    warned = 0;
    return 1;
failed:
    if(!warned)
    {
        gpu_log("WARNING: Android presentation failed: %s; frame skipped.", SDL_GetError());
        warned = 1;
    }
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Present a completed launcher target directly, or upload the ordinary CPU
 * game scene. Both use the same GLES2 output filter and existing brightness,
 * aspect and touch presentation. Launcher failure never enters the CPU path.
 * Movie textures remain independent of both scene sources.
 */
int video_copy_screen(s_screen *src)
{
    SDL_Texture *game_texture = texture;
    SDL_Texture *source_texture, *filtered;
    int game_width = textureWidth, game_height = textureHeight;
    int is_launcher = src && src == launcher_owner;
    int success;
    if(is_launcher)
    {
        if(!video_launcher_current(src)) return 0;
        source_texture = gpu_launcher_android_texture(launcher);
        if(!source_texture) return video_launcher_reject("completed target access");
    }
    else
    {
        s_videosurface *surface = getVideoSurface(src);
        if(!surface) return 0;
        if(surface->width != textureWidth || surface->height != textureHeight ||
           SDL_UpdateTexture(texture, NULL, surface->data, surface->pitch) < 0)
        {
            gpu_log("ERROR: Android RGB32 scene upload failed: %s.", SDL_GetError());
            return 0;
        }
        source_texture = texture;
    }
    if(savedata.swfilter)
    {
        filtered = gpu_output_filter_android_apply(output_filter, source_texture);
        if(!filtered)
        {
            if(is_launcher) video_launcher_reject("output filtering");
            return 0;
        }
        source_texture = filtered;
        textureWidth *= 2;
        textureHeight *= 2;
    }
    texture = source_texture;
    success = blit();
    texture = game_texture;
    textureWidth = game_width;
    textureHeight = game_height;
    if(is_launcher)
    {
        if(!success || SDL_AtomicGet(&launcher_reset_pending))
            return video_launcher_reject("final presentation");
        launcher_warning = 0;
    }
    if(!success) return 0;

    SDL_Delay(1);
    return 1;
}

void video_clearscreen()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    //SDL_RenderPresent(renderer);
}

void video_stretch(int enable)
{
    stretch = enable;
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

void video_set_color_correction(int gm, int br)
{
	brightness = br;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Allocate an Android hardware movie texture with an explicitly supported
 * sampling choice. Restore the global SDL hint so movie selection cannot
 * change subsequent game textures. Higher kernels need the native port.
 */
static SDL_Texture *video_android_movie_texture(int width, int height,
                                               e_drawmethod_resample resample)
{
    SDL_Texture *result;
    const char *hint = SDL_GetHint(SDL_HINT_RENDER_SCALE_QUALITY);
    char *saved_hint = hint ? SDL_strdup(hint) : NULL;
    if(resample != DC_RESAMPLE_NEAREST && resample != DC_RESAMPLE_BILINEAR)
    {
        static int warned;
        if(!warned)
        {
            warned = 1;
            gpu_log("WARNING: Android SDL movies support nearest and bilinear sampling. Higher resampling kernels require the native scene renderer port; output skipped.");
        }
        SDL_free(saved_hint);
        return NULL;
    }
    if(hint && !saved_hint)
    {
        gpu_log("WARNING: Android hardware movie sampling setup could not allocate hint storage.");
        return NULL;
    }
    /* Match the game's DEFAULT hint priority. Higher-priority configuration
     * may reject this request; never silently use a different movie filter. */
    if(!SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY,
                resample == DC_RESAMPLE_NEAREST ? "nearest" : "linear", SDL_HINT_DEFAULT))
    {
        const char *active = SDL_GetHint(SDL_HINT_RENDER_SCALE_QUALITY);
        int matches = active && (resample == DC_RESAMPLE_NEAREST
            ? (!SDL_strcasecmp(active, "nearest") || !strcmp(active, "0"))
            : (!SDL_strcasecmp(active, "linear") || !strcmp(active, "1")));
        if(!matches)
        {
            gpu_log("WARNING: Android movie sampling is blocked by an overriding SDL scale-quality setting; output skipped.");
            SDL_free(saved_hint);
            return NULL;
        }
    }
    result = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12,
                                SDL_TEXTUREACCESS_STREAMING, width, height);
    SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY,
                            saved_hint ? saved_hint : "nearest", SDL_HINT_DEFAULT);
    SDL_free(saved_hint);
    if(!result)
    {
        gpu_log("WARNING: Android hardware movie texture allocation failed: %s.", SDL_GetError());
    }
    return result;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Retain Android fullscreen playback without replacing its RGB game texture.
 * This platform's SDL presenter is explicit and is never a retry after a
 * declined native movie-to-screen operation.
 */
int video_setup_yuv_overlay(const yuv_video_mode *mode)
{
    SDL_Texture *replacement;
    if(!renderer || !mode || mode->width < 2 || mode->height < 2 ||
       (mode->width & 1) || (mode->height & 1) ||
       mode->display_width <= 0 || mode->display_height <= 0)
    {
        return 0;
    }
    replacement = video_android_movie_texture(mode->width, mode->height, DC_RESAMPLE_MOVIE);
    if(!replacement)
    {
        return 0;
    }
    SDL_DestroyTexture(movie_texture);
    movie_texture = replacement;
    movie_resample = DC_RESAMPLE_MOVIE;
    movie_owner = NULL;
    stored_yuv_mode = *mode;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Upload retained planes to Android's hardware presenter only when the
 * decoded generation changes. Plane names here explicitly follow Y, U, V.
 */
int video_prepare_yuv_frame(const gpu_yuv_source *source,
                            e_drawmethod_resample resample)
{
    if(!source || !source->owner || !source->y || !source->u || !source->v ||
       !renderer || source->width != stored_yuv_mode.width ||
       source->height != stored_yuv_mode.height)
    {
        return 0;
    }
    /* Controller layout can remain selected across a video-mode recreation.
     * Rebuild discarded storage from the retained frame on its next draw. */
    if(!movie_texture || movie_resample != resample)
    {
        SDL_Texture *replacement = video_android_movie_texture(source->width, source->height, resample);
        if(!replacement)
        {
            return 0;
        }
        SDL_DestroyTexture(movie_texture);
        movie_texture = replacement;
        movie_resample = resample;
        movie_owner = NULL;
    }
    if(movie_owner != source->owner || movie_generation != source->generation)
    {
        if(SDL_UpdateYUVTexture(movie_texture, NULL, source->y, source->width,
                                source->u, source->width / 2,
                                source->v, source->width / 2) < 0)
        {
            gpu_log("WARNING: Android hardware movie upload failed: %s.", SDL_GetError());
            return 0;
        }
        movie_owner = source->owner;
        movie_generation = source->generation;
    }
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Capture Android movie pixels through SDL's accelerated YUV conversion and
 * an explicit render-target readback. No CPU color converter is retained.
 * Restore presenter state before returning, including after failed capture.
 */
int video_capture_yuv_frame(s_screen *screen, const gpu_yuv_source *source,
                            e_drawmethod_resample resample)
{
    SDL_Texture *planes = NULL, *target = NULL, *saved_target;
    SDL_Rect viewport, clip;
    SDL_BlendMode blend;
    Uint8 red, green, blue, alpha;
    float scale_x, scale_y;
    int logical_width, logical_height, clipped, success = 0;
    if(!renderer || !screen || screen->pixelformat != PIXEL_32 ||
       screen->width <= 0 || screen->height <= 0 || !source ||
       source->width < 2 || source->height < 2 ||
       (source->width & 1) || (source->height & 1) ||
       !source->y || !source->u || !source->v)
    {
        return 0;
    }
    planes = video_android_movie_texture(source->width, source->height, resample);
    if(!planes)
    {
        return 0;
    }
    target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888,
                                SDL_TEXTUREACCESS_TARGET, screen->width, screen->height);
    if(!target ||
       SDL_UpdateYUVTexture(planes, NULL, source->y, source->width,
                            source->u, source->width / 2,
                            source->v, source->width / 2) < 0)
    {
        goto done;
    }
    saved_target = SDL_GetRenderTarget(renderer);
    SDL_RenderGetViewport(renderer, &viewport);
    SDL_RenderGetClipRect(renderer, &clip);
    clipped = SDL_RenderIsClipEnabled(renderer);
    SDL_RenderGetScale(renderer, &scale_x, &scale_y);
    SDL_RenderGetLogicalSize(renderer, &logical_width, &logical_height);
    SDL_GetRenderDrawBlendMode(renderer, &blend);
    SDL_GetRenderDrawColor(renderer, &red, &green, &blue, &alpha);
    if(SDL_SetRenderTarget(renderer, target) == 0)
    {
        SDL_RenderSetLogicalSize(renderer, 0, 0);
        SDL_RenderSetScale(renderer, 1.0f, 1.0f);
        SDL_RenderSetViewport(renderer, NULL);
        SDL_RenderSetClipRect(renderer, NULL);
        SDL_SetTextureBlendMode(planes, SDL_BLENDMODE_NONE);
        if(SDL_RenderCopy(renderer, planes, NULL, NULL) == 0)
        {
            if(brightness)
            {
                int opacity = brightness > 0 ? MIN(256, brightness) : -MAX(-256, brightness);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, brightness > 0 ? 255 : 0,
                    brightness > 0 ? 255 : 0, brightness > 0 ? 255 : 0,
                    (Uint8)(opacity - 1));
                SDL_RenderFillRect(renderer, NULL);
            }
            /* Screenshot output consumes RGB only. SDL owns the YUV conversion;
             * this explicit capture is the sole requested CPU pixel transfer. */
            gpu_prepare_cpu_write(screen);
            success = SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ABGR8888,
                                            screen->data, screen->width * 4) == 0;
        }
    }
    SDL_SetRenderTarget(renderer, saved_target);
    SDL_RenderSetLogicalSize(renderer, logical_width, logical_height);
    SDL_RenderSetScale(renderer, scale_x, scale_y);
    SDL_RenderSetViewport(renderer, &viewport);
    SDL_RenderSetClipRect(renderer, clipped ? &clip : NULL);
    SDL_SetRenderDrawBlendMode(renderer, blend);
    SDL_SetRenderDrawColor(renderer, red, green, blue, alpha);
done:
    if(!success)
    {
        gpu_log("WARNING: Android hardware movie screenshot capture failed: %s.", SDL_GetError());
    }
    if(planes) SDL_DestroyTexture(planes);
    if(target) SDL_DestroyTexture(target);
    return success;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Present the dedicated Android movie texture with existing touch controls
 * and display correction, then restore the game texture and its dimensions.
 */
int video_display_yuv_frame(void)
{
    SDL_Texture *game_texture = texture;
    int game_width = textureWidth, game_height = textureHeight;
    int success;
    if(!movie_texture)
    {
        return 0;
    }
    texture = movie_texture;
    textureWidth = stored_yuv_mode.display_width;
    textureHeight = stored_yuv_mode.display_height;
    success = blit();
    texture = game_texture;
    textureWidth = game_width;
    textureHeight = game_height;
    return success;
}

int video_current_refresh_rate()
{
    SDL_DisplayMode display_mode;
    if (SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window), &display_mode) != 0)
        return 60;
    return display_mode.refresh_rate;
}
