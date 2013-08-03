#include <math.h>
#include "types.h"
#include "video.h"
#include "vga.h"
#include "screen.h"
#include "sdlport.h"
#include "opengl.h"
#include "openbor.h"
#include "gfxtypes.h"
#include "gfx.h"
#include "pngdec.h"
#include "SDL_opengles.h"

extern int videoMode;

#define nextpowerof2(x) pow(2,ceil(log(x)/log(2)))
#define abs(x)			((x<0)?-(x):(x))

#define VIDEO_USE_OPENGL (savedata.usegl[savedata.fullscreen])
#define MUST_USE_BSCREEN 1

s_videomodes stored_videomodes;
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;
SDL_Texture *buttons = NULL;
SDL_Surface *bscreen = NULL;
static int bytes_per_pixel = 1;
int stretch = 1;
int nativeWidth, nativeHeight; // monitor resolution used in fullscreen mode
static unsigned glpalette[256]; // for 8bit
static int viewportWidth, viewportHeight;      // dimensions of display area
static int scaledWidth, scaledHeight;          // dimensions of game screen after scaling
static int textureWidth, textureHeight;        // dimensions of game screen and GL texture
static int xOffset, yOffset;                   // offset of game screen on display area
static int bytesPerPixel;
int opengl = 1; //though using SDL, we need its video setting

#include "button_png_800x480.h"

void initSDL()
{
    SDL_DisplayMode mode;
    int init_flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK;
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
    atexit(SDL_Quit);

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

    savedata.fullscreen = 1;
}


static int textureDepths[4] = {32, 16, 24, 32};
static unsigned masks[4][4] = {{0xFF, 0xFF00, 0xFF0000, 0}, {0x1F, 0x07E0, 0xF800, 0}, {0xFF, 0xFF00, 0xFF0000, 0}, {0xFF, 0xFF00, 0xFF0000, 0}};

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
setup android touch positions.
Use native width (w) as basic measurement, stretch in a ratio of 480/800,
put direction buttons and main control buttons in the lower corners,
a3 and a4 in center bottom, esc and start in upper corners and screenshot
center of the screen.

    start                       esc



            (screenshot)



                               special
 directions    a3   a4       a1       a2
                                jump
*/
static void setup_touch_default()
{
    float w = nativeWidth;
    float h = nativeHeight;
    float hh = w * 480 / 800;
    float ra = 0.15f, rb = ra / 1.75f;
    float cx1 = (ra + rb + rb / 5.0f) * hh, cy1 = h - cx1;
    float cx2 = w - cx1, cy2 = cy1;
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

static int setup_touch_txt()
{
    int pos, i, sdid, t;
    static char filename[256];
    static char pakname[256];
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
    sprintf(filename, "/mnt/sdcard/OpenBOR/Saves/%s", pakname);
    dirExists(filename, 1);
    sprintf(filename, "/mnt/sdcard/OpenBOR/Saves/%s/touch.txt", pakname);
    // Read file
    if( buffer_pakfile(filename, &buf, &size) != 1 &&
            buffer_pakfile("data/touch.txt", &buf, &size) != 1 &&
            buffer_pakfile("/mnt/sdcard/OpenBOR/Saves/touch.txt", &buf, &size) != 1)
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
                else if(stricmp(command, "texture") == 0)
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
                else if(stricmp(command, "screendocking") == 0) // change screen position to avoid buttons
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

int video_set_mode(s_videomodes videomodes)
{
    //if(memcmp(&stored_videomodes, &videomodes, sizeof(videomodes))==0) return 1;
    stored_videomodes = videomodes;

    int b;
    int allocTextureWidth, allocTextureHeight;

    savedata.screen[videoMode][0] = 0;
    savedata.fullscreen = 1;
    bytes_per_pixel = videomodes.pixel;
    b = bytes_per_pixel - 1;

    //destroy all
    if(bscreen)
    {
        SDL_FreeSurface(bscreen);
    }
    bscreen = NULL;
    if(texture)
    {
        SDL_DestroyTexture(texture);
    }
    texture = NULL;
    if(buttons)
    {
        SDL_DestroyTexture(buttons);
    }
    buttons = NULL;
    //mysterious crash in sdl 2.0 if these are recreated, so leave them alone for now
    //if(renderer) SDL_DestroyRenderer(renderer);
    //renderer = NULL;
    //if(window) SDL_DestroyWindow(window);
    //window = NULL;

    if(videomodes.hRes == 0 && videomodes.vRes == 0)
    {
        return 0;
    }

    viewportWidth = nativeWidth;
    viewportHeight = nativeHeight;

    if(!window && !(window = SDL_CreateWindow("OpenBOR", 0, 0, nativeWidth, nativeHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN)))
    {
        printf("error: %s\n", SDL_GetError());
        return 0;
    }

    if(!renderer && !(renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED)))
    {
        printf("error: %s\n", SDL_GetError());
        return 0;
    }

    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer, &info);

    printf("SDL video Renderer: %s \n", info.name);

    SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, savedata.glfilter[savedata.fullscreen] ? "nearest" : "linear", SDL_HINT_DEFAULT);

    // now create a texture
    textureWidth = videomodes.hRes;
    textureHeight = videomodes.vRes;

    allocTextureWidth = nextpowerof2(textureWidth);
    allocTextureHeight = nextpowerof2(textureHeight);

    int format = SDL_MasksToPixelFormatEnum (textureDepths[b], masks[b][0], masks[b][1], masks[b][2], masks[b][3]);
    if(!(texture = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_STREAMING, allocTextureWidth, allocTextureHeight)))
    {
        printf("error: %s\n", SDL_GetError());
        return 0;
    }

    setup_touch();

    if(!buttons)
    {
        bscreen = pngToSurface(buttonpng);
        if(!bscreen || !(buttons = SDL_CreateTextureFromSurface(renderer, bscreen)))
        {
            printf("error: %s\n", SDL_GetError());
            return 0;
        }
        SDL_FreeSurface(bscreen);
        bscreen = NULL;
    }

    //create a buffer for 8bit mode, masks don't really matter but anyway set them
    if(bytes_per_pixel == 1)
    {
        bscreen = SDL_CreateRGBSurface(SDL_SWSURFACE, textureWidth, textureHeight, textureDepths[b], masks[b][0], masks[b][1], masks[b][2], masks[b][3]);
    }

    video_clearscreen();

    return 1;
}

void video_fullscreen_flip()
{
    //dummy for now
}

int video_copy_screen(s_screen *src)
{
    void *data;
    int pitch, linew, i, h;
    unsigned char *sp;
    unsigned char *dp;
    SDL_Rect rectdes, rectsrc;
    rectsrc.x = rectsrc.y = 0;
    rectsrc.w = textureWidth;
    rectsrc.h = textureHeight;
    int hide_touch;
    extern int hide_t;

    linew = src->width * bytes_per_pixel;
    if(bscreen)
    {
        if(src->width != bscreen->w || src->height != bscreen->h)
        {
            return 0;
        }

        if(SDL_MUSTLOCK(bscreen))
        {
            SDL_LockSurface(bscreen);
        }
        sp = (unsigned char *)src->data;
        dp = bscreen->pixels;
        h = src->height;
        do
        {
            //u16pcpy((unsigned short*)dp, sp, glpalette, linew);
            i = linew - 1;
            do
            {
                ((unsigned *)dp)[i] = glpalette[sp[i]];
            }
            while(i--);
            sp += linew;
            dp += bscreen->pitch;
        }
        while(--h);
        data = bscreen->pixels;
        pitch = bscreen->pitch;
    }
    else
    {
        data = src->data;
        pitch = linew;
    }
    SDL_UpdateTexture(texture, &rectsrc, data, pitch);
    if(bscreen && SDL_MUSTLOCK(bscreen))
    {
        SDL_UnlockSurface(bscreen);
    }

    if(stretch)
    {
        //rectdes.w = textureWidth;
        //rectdes.h = textureHeight;
        //rectdes.x = (viewportWidth-rectdes.w)/2;
        //rectdes.y = (viewportHeight-rectdes.h)/2;
        rectdes.w = viewportWidth;
        rectdes.h = viewportHeight;
        rectdes.x = 0;
        rectdes.y = 0;
    }
    else if(savedata.glscale > 0)
    {
        rectdes.w = textureWidth * savedata.glscale;
        rectdes.h = textureHeight * savedata.glscale;
        rectdes.x = (viewportWidth - rectdes.w) / 2;
        rectdes.y = (viewportHeight - rectdes.h) / 2;
        if(rectdes.h < viewportHeight - 2)
        {
            if(screendocking & DOCKTOP)
            {
                rectdes.y = 1;
            }
            else if(screendocking & DOCKBOTTOM)
            {
                rectdes.y = viewportHeight - 1 - rectdes.h;
            }
        }
        if(rectdes.w < viewportWidth - 2)
        {
            if(screendocking & DOCKLEFT)
            {
                rectdes.x = 1;
            }
            else if(screendocking & DOCKRIGHT)
            {
                rectdes.x = viewportWidth - 1 - rectdes.w;
            }
        }
    }
    else if((float)viewportWidth / (float)viewportHeight > (float)textureWidth / (float)textureHeight)
    {
        rectdes.h = viewportHeight;
        rectdes.w = rectdes.h * textureWidth / textureHeight;
        rectdes.x = (viewportWidth - rectdes.w) / 2;
        rectdes.y = 0;
    }
    else
    {
        rectdes.w = viewportWidth;
        rectdes.h = rectdes.w * textureHeight / textureWidth;
        rectdes.y = (viewportHeight - rectdes.h) / 2;
        rectdes.x = 0;
    }

    //SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, savedata.glfilter[savedata.fullscreen]?"linear":"nearest", SDL_HINT_DEFAULT);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, &rectsrc, &rectdes);

    hide_touch = (timer_gettick() > hide_t);
    for(i = 0, h = 0; i < MAXTOUCHB; i++)
    {
        h += touchstates[i];
        if(!hide_touch && (i != SDID_SCREENSHOT || touchstates[i]))
        {
            SDL_SetTextureAlphaMod(buttons, touchstates[i] ? 128 : 64);
            SDL_RenderCopy(renderer, buttons, &btnsrc[i], &btndes[i]);
        }
    }
    if(h)
    {
        hide_t = timer_gettick() + 5000;
    }
    SDL_RenderPresent(renderer);

    SDL_Delay(1);
    return 1;
}

void video_clearscreen()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    //SDL_RenderPresent(renderer);
}

void video_stretch(int enable)
{
    //video_clearscreen();
    stretch = enable;
}

void vga_vwait(void)
{
    static int prevtick = 0;
    int now = SDL_GetTicks();
    int wait = 1000 / 60 - (now - prevtick);
    if (wait > 0)
    {
        SDL_Delay(wait);
    }
    else
    {
        SDL_Delay(1);
    }
    prevtick = now;
}

void vga_setpalette(unsigned char *palette)
{
    int i;
    for(i = 0; i < 256; i++)
    {
        glpalette[i] = colour32(palette[0], palette[1], palette[2]);
        palette += 3;
    }
}

//TODO finish this
void vga_set_color_correction(int gm, int br)
{
    /*
    Uint16 ramp[256];
    SDL_CalculateGammaRamp((float)gm/256, ramp);
    SDL_SetWindowGammaRamp(window, ramp, ramp, ramp);*/
}



/*
	glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_DITHER);

	glViewport(0, 0, viewportWidth, viewportHeight);

	// set up orthographic projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(0, viewportWidth, 0, viewportHeight, -1, 1);

	// reset the model view
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnableClientState(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	GLfloat texture_coordinates[] = {0.0f, tcy,
                                     0.0f, 0.0f,
                                     tcx, tcy,
                                     tcx, 0.0f};

	GLfloat vertices[] = {rectdes.x, rectdes.y+rectdes.h,
                          rectdes.x, rectdes.y,
                          rectdes.x+rectdes.w, rectdes.y+rectdes.h,
                          rectdes.x+rectdes.w, rectdes.y};

	SDL_GL_BindTexture(texture, NULL, NULL);

	glTexCoordPointer(2, GL_FLOAT, 0, texture_coordinates);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	SDL_GL_UnbindTexture(texture);
	SDL_GL_SwapWindow(window);
*/