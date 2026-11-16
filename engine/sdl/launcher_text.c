/*
 * OpenBOR - http://www.chronocrash.com
 * All rights reserved, see LICENSE in OpenBOR root for details.
 * Copyright (c) OpenBOR Team
 */
#include <stdio.h>
#include <stdlib.h>
#include "launcher_text.h"
#include "hankaku.h"
#include "gpu.h"
#if defined(ANDROID) || defined(OPENBOR_ANDROID_LAUNCHER_TEST)
#include "video.h"
#elif defined(OPENGL)
#include "gpu_renderer.h"
#endif

/*
 * Damon V. Caskey
 * 2026-11-16
 * Translate the launcher's original byte-oriented character set into the
 * resident font's 160 glyphs without reading beyond that immutable font.
 */
size_t launcher_text_map(unsigned char *text, size_t capacity)
{
    size_t count;

    if(!text)
    {
        return 0;
    }
    for(count = 0; count < capacity && text[count]; ++count)
    {
        unsigned char character = text[count];
        /* Preserve the two original printable ranges. Other nonzero bytes
         * still advance one cell, using the font's blank first glyph. */
        if(character >= 0x20 && character < 0x80)
        {
            text[count] = character - 0x20;
        }
        else if(character >= 0xa0 && character < 0xe0)
        {
            text[count] = character - 0x40;
        }
        else
        {
            text[count] = 0;
        }
    }
    return count;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Format short labels without allocation and measure longer log lines before
 * allocating their complete text. Submit one mapped run to the native atlas
 * renderer; the launcher's destination stays GPU-owned throughout.
 */
int launcher_text_vprintf(s_screen *screen, int x, int y,
                         unsigned foreground, unsigned background, int fill,
                         const char *format, va_list args)
{
    unsigned char local[128];
    unsigned char *text = local;
    va_list copy;
    int length, result = 0;
    size_t count;
#if !defined(OPENGL) && !defined(ANDROID) && !defined(OPENBOR_ANDROID_LAUNCHER_TEST)
    static int unavailable_logged;
#endif

    if(!format)
    {
        gpu_log("WARNING: Launcher text has no format string; draw skipped.");
        return 0;
    }

    /* Each formatting pass gets its own argument cursor. The reported size
     * excludes the terminator, which is reserved in both storage paths. */
    va_copy(copy, args);
    length = vsnprintf((char *)local, sizeof(local), format, copy);
    va_end(copy);
    if(length < 0)
    {
        gpu_log("WARNING: Launcher text formatting failed; draw skipped.");
        return 0;
    }
    if((size_t)length >= sizeof(local))
    {
        text = malloc((size_t)length + 1);
        if(!text)
        {
            gpu_log("WARNING: Launcher text allocation failed; draw skipped.");
            return 0;
        }
        va_copy(copy, args);
        result = vsnprintf((char *)text, (size_t)length + 1, format, copy);
        va_end(copy);
        if(result != length)
        {
            free(text);
            gpu_log("WARNING: Launcher text formatting changed length; draw skipped.");
            return 0;
        }
    }

    /* Mapping stops at the first NUL, including one produced by %c. Blank
     * glyphs inside the mapped run are ordinary cells, not terminators. */
    count = launcher_text_map(text, (size_t)length);
#if defined(ANDROID) || defined(OPENBOR_ANDROID_LAUNCHER_TEST)
    /* Android composes the complete launcher in its scoped GLES target. Its
     * video adapter retains frame ordering and owns presentation eligibility. */
    result = video_launcher_text(screen, x, y, foreground, background, fill,
                                 text, count, hankaku_font10, LAUNCHER_FONT_GLYPHS);
#elif defined(OPENGL)
    result = gpu_gl_draw_launcher_text(screen, x, y, foreground, background,
                                       fill, text, count, hankaku_font10,
                                       LAUNCHER_FONT_GLYPHS);
#else
    /* Desktop RGB32 mode already requires native rendering. Keep manual
     * non-GL builds linkable without introducing a software glyph path. */
    (void)screen;
    (void)x;
    (void)y;
    (void)foreground;
    (void)background;
    (void)fill;
    (void)count;
    result = 0;
    if(!unavailable_logged)
    {
        gpu_log("WARNING: Native launcher text unavailable; draw skipped without CPU retry.");
        unavailable_logged = 1;
    }
#endif
    if(text != local)
    {
        free(text);
    }
    /* The native renderer owns its bounded rejection diagnostic. Do not emit
     * another warning for every failed label in the same launcher frame. */
    return result;
}
