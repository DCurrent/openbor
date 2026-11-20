/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef GAMELIB_TYPES_H
#define GAMELIB_TYPES_H

#ifndef TRANSPARENT_IDX
#define		TRANSPARENT_IDX		0x00
#endif

#define	ANYNUMBER		 2

/*
 * Damon V. Caskey
 * 2026-11-15
 * Every supported engine target uses 64-bit pointers. Keep this assertion
 * compatible with C99, including builds without C11 static assertions.
 */
typedef char openbor_requires_64_bit_pointers[(sizeof(void *) == 8) ? 1 : -1];

#if SDL
#pragma pack(push,4)
#endif

/* Indexed values describe artwork and masks; RGB32 is the scene format.
 * Preserve these identities without reserving any alternate color mode. */
#define PIXEL_8          0
#define PIXEL_x8         1
#define PIXEL_32         4

/*
 * Damon V. Caskey
 * 2026-11-15
 * Identify byte RLE artwork and opacity masks by their indexed formats.
 */
static inline int pixel_format_indexed(int format)
{
    return format == PIXEL_8 || format == PIXEL_x8;
}

/*
 * Damon V. Caskey
 * 2026-11-15
 * Validate artwork and RGB32 screen identities before dispatch or stride
 * lookup, excluding obsolete color formats and out-of-range values.
 */
static inline int pixel_format_supported(int format)
{
    return pixel_format_indexed(format) || format == PIXEL_32;
}


#define BLEND_SCREEN     0
#define BLEND_MULTIPLY   1
#define BLEND_OVERLAY    2
#define BLEND_HARDLIGHT  3
#define BLEND_DODGE      4
#define BLEND_HALF       5

#define MAX_BLENDINGS    6

/* Default asset and palette format, independent of RGB32 scene surfaces. */
extern int pixelformat;
// in pixelformat.c
extern int pixelbytes[(int)5];

#define PAL_BYTES ((pixelbytes[(int)PIXEL_32]*256))

// Define the fixed-size integer types used by engine image storage.
// TODO: use these for throughout the engine where fixed-length types are needed
#if ((__STDC_VERSION__ >= 199901L) || (defined(__GNUC__) && (__GNUC__ >= 3))) && (!defined(DC))
#include <stdint.h>
typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef int64_t s64;
typedef uint64_t u64;
#else // MSVC - no C99 support :(
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long s64; // FIXME: not sure if this will have the correct length in MSVC 64-bit mode
typedef unsigned long long u64;
#endif

// Define the "bool" type and standard values for "true" and "false"
#if (__STDC_VERSION__ >= 199901L) || (defined(__GNUC__) && (__GNUC__ >= 3))
#include <stdbool.h>
#else // MSVC again... :(
#if !defined(bool)
#define bool s8
#endif
#if !defined(true) && !defined(false)
#define true 1
#define false 0
#endif
#endif

//scr
#define screen_magic ((int)0x726373)

typedef enum e_color_components {
    COLOR_COMPONENT_RED    = 0,
    COLOR_COMPONENT_GREEN  = 1,
    COLOR_COMPONENT_BLUE   = 2,
    COLOR_COMPONENT_ALPHA  = 3,
    COLOR_COMPONENT_RGB    = 3,
    COLOR_COMPONENT_END    = 4
} e_color_components;

/*
* Caskey, Damon V.
* 2023-04-25
*
* Identify object type so we can
* verify a pointer to an object
* is valid.
*
* Each object has an object type
* member with identical member name
* (object_type). We populate the
* member with an appropriate value
* from this list on allocation.
*/
typedef enum e_object_type {
    OBJECT_TYPE_NONE,
    OBJECT_TYPE_BIND,
    OBJECT_TYPE_DRAWMETHOD,
    OBJECT_TYPE_ENTITY,
    OBJECT_TYPE_FACTION,
    OBJECT_TYPE_FLASH,
    OBJECT_TYPE_GLOBAL_CONFIG,
    OBJECT_TYPE_MODEL,
    OBJECT_TYPE_SOUND,
    OBJECT_TYPE_MOVIE_PLAYBACK
} e_object_type;

typedef struct
{
    int magic;
    int	width;
    int	height;
    int pixelformat;
    unsigned char *palette;
    unsigned char data[ANYNUMBER];
} s_screen;


typedef struct
{
    int	width;
    int	height;
    int	planar;
    int	banked;		// Still unused
    unsigned char 	*data;
} s_vram;


//bmp
#define bitmap_magic ((int)0x706d62)

typedef struct
{
    int magic;
    int	width;
    int	height;
    int pixelformat;
    int clipped_x_offset;
    int clipped_y_offset;
    int clipped_width;
    int clipped_height;
    unsigned char *palette;
    unsigned char data[ANYNUMBER];
} s_bitmap;

//spr
#define sprite_magic ((int)0x727073)

typedef struct spritestruct
{
    int magic;
    int	centerx;
    int	centery;
    int offsetx;
    int offsety;
    int srcwidth;
    int srcheight;
    int	width;
    int	height;
    int pixelformat;
    struct spritestruct *mask;
    unsigned char *palette;
    int data[ANYNUMBER];
} s_sprite;

struct sprite_list
{
    char *filename;
    s_sprite *sprite;
    int ref;
    struct sprite_list *next;
};
typedef struct sprite_list s_sprite_list;

typedef struct
{
    s_sprite_list *node;
    int  centerx;
    int  centery;
} s_sprite_map;

/* Channel lookup tables are GPU material sources, not scene pixel writers. */
void set_blendtables(unsigned char *tables[]);
extern unsigned char *blendtables[MAX_BLENDINGS];
unsigned colour32(unsigned char r, unsigned char g, unsigned char b);

typedef enum
{
	WATER_MODE_NONE,
	WATER_MODE_SINE = 1,
	WATER_MODE_SHEAR = 3
} e_water_mode;


typedef enum
{
	WATER_PERSPECTIVE_NONE,
	WATER_PERSPECTIVE_TILE,
	WATER_PERSPECTIVE_STRETCH
} e_water_perspective;

typedef enum e_drawmethod_config
{
    DRAWMETHOD_CONFIG_NONE                      = 0,
    DRAWMETHOD_CONFIG_BACKGROUND_TRANSPARENCY   = (1 << 1),
    DRAWMETHOD_CONFIG_ENABLED                   = (1 << 2),
    DRAWMETHOD_CONFIG_FLIP_ROTATE               = (1 << 3),
    DRAWMETHOD_CONFIG_FLIP_X                    = (1 << 4),
    DRAWMETHOD_CONFIG_FLIP_Y                    = (1 << 5),
    DRAWMETHOD_CONFIG_AFFINE                    = (1 << 6)
    
} e_drawmethod_config;

/*
 * Damon V. Caskey
 * 2026-11-15
 * Select one built-in reconstruction filter for the completed draw transform.
 * Zero preserves nearest sampling for defaults and zero-initialized methods.
 * MOVIE names the existing hardware movie's spatial filter only; movie color
 * conversion remains independent of this selector.
 */
typedef enum e_drawmethod_resample
{
    DC_RESAMPLE_NEAREST = 0,
    DC_RESAMPLE_BILINEAR = 1,
    DC_RESAMPLE_BICUBIC_CATMULL_ROM = 2,
    DC_RESAMPLE_BICUBIC_MITCHELL = 3,
    DC_RESAMPLE_BICUBIC_BSPLINE = 4,
    DC_RESAMPLE_LANCZOS_2 = 5,
    DC_RESAMPLE_LANCZOS_3 = 6,
    DC_RESAMPLE_END = 7,
    DC_RESAMPLE_MOVIE = DC_RESAMPLE_BILINEAR
} e_drawmethod_resample;

typedef struct
{
    union
    {
        int amplitude;
        float beginsize;
    };
    union
    {
        float wavelength;
        float endsize;
    };
    int wavetime;
    union
    {
        float wavespeed;
		e_water_perspective	perspective;
    };
    e_water_mode watermode;
} water_transform;

/*
 * Damon V. Caskey
 * 2026-11-13
 * Source-to-destination affine transform (2x3 matrix).
 *
 * x' = m00 * x + m01 * y + tx
 * y' = m10 * x + m11 * y + ty
 *
 * Source coordinates are relative to the draw pivot. The matrix has its
 * own opt-in flag and does not share the legacy scale/shear/water state.
 */
typedef struct
{
    float m00;
    float m01;
    float m10;
    float m11;
    float tx;
    float ty;
} affine_transform;

/*
 * Damon V. Caskey
 * 2026-11-14
 * Capture five source chroma keys and five final-output substitutions inside
 * each drawmethod. Explicit masks keep packed RGB zero available as a color.
 */
#define DRAWMETHOD_COLOR_SLOTS 5
#define DRAWMETHOD_COLOR_MASK ((1u << DRAWMETHOD_COLOR_SLOTS) - 1u)

typedef struct
{
    unsigned int match;
    unsigned int replacement;
} s_color_substitute;

typedef struct
{
    unsigned int chroma[DRAWMETHOD_COLOR_SLOTS];
    s_color_substitute substitute[DRAWMETHOD_COLOR_SLOTS];
    unsigned int chroma_enabled;
    unsigned int substitute_enabled;
} s_drawmethod_colors;

typedef struct
{
    unsigned char *table;	// ~~
    //void *fp;
    unsigned int fillcolor;		// ~~
    e_drawmethod_config config;
        //int flag;				// When 0, the global plainmethod is used. ~~
    int alpha;				// ~~				
    int remap;				// ~~
        //int flipx;				// ~~
        //int flipy;				// ~~
        //int transbg;			// ~~
        //int fliprotate;			// entity only, whether the flip is affected by the entity's facing(not the sprite's flip ) ~~
    float rotate;			// 360 degrees ~~
    int scalex;				// ~~
    int scaley;				// ~~
    int shiftx;				// ~~
    int centerx;			// shift centerx ~~
    int centery;			// shift centery ~~
    int xrepeat;			// ~~
    int yrepeat;			// ~~
    int xspan;				// ~~
    int yspan;				// ~~
    unsigned char channelr;	// ~~
    unsigned char channelg; // ~~
    unsigned char channelb;	// ~~
    unsigned tintmode;		// tint the sprite with color ~~
    unsigned tintcolor;		// ~~
    int clipx;				// ~~
    int clipy;				// ~~
    int clipw;				// ~~
    int cliph;				// ~~
    water_transform water;	
    affine_transform affine;

    /* Inline storage gives queued draws immutable copies of color settings. */
    s_drawmethod_colors colors;
    /* Damon V. Caskey, 2026-11-15: queued copies retain their sampler choice. */
    e_drawmethod_resample resample;
	int tag;				// ~~
    e_object_type object_type;
} s_drawmethod;
extern const s_drawmethod plainmethod;

typedef struct
{
    union
    {
        s_screen *screen;
        s_sprite *sprite;
        s_bitmap *bitmap;
        void      *handle;
    };
} gfx_entry;


typedef struct
{
    short hRes;        // Horizontal Resolution
    short vRes;		 // Vertical Resolution
    short hShift;	     // Offset for X-Axis Text
    short vShift;	     // Offset for Y-Axis Text
    short dOffset;	 // Offset for Debug Text
    short shiftpos[4];
    char filter;
    char mode;
    char pixel;
    float hScale;    // Multiplier for X-Axis
    float vScale;    // Multiplier for Y-Axis

} s_videomodes;

#if SDL
#pragma pack(pop)
#endif

#endif
