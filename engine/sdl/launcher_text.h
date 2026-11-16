/*
 * OpenBOR - http://www.chronocrash.com
 * All rights reserved, see LICENSE in OpenBOR root for details.
 * Copyright (c) OpenBOR Team
 */
#ifndef OPENBOR_LAUNCHER_TEXT_H
#define OPENBOR_LAUNCHER_TEXT_H

#include <stdarg.h>
#include <stddef.h>
#include "types.h"

#define LAUNCHER_FONT_WIDTH 5
#define LAUNCHER_FONT_HEIGHT 10
#define LAUNCHER_FONT_GLYPHS 160

/* Map bytes in place, stopping at NUL or capacity. Unsupported characters
 * occupy a blank cell; control bytes do not introduce new lines or tabs. */
size_t launcher_text_map(unsigned char *text, size_t capacity);

/* Desktop launcher labels use native RGB32 drawing exclusively. Formatting
 * and glyph mapping remain CPU work; no destination pixels are read or written.
 * Returns zero on failure after logging, without a CPU rendering retry. */
int launcher_text_vprintf(s_screen *screen, int x, int y,
                         unsigned foreground, unsigned background, int fill,
                         const char *format, va_list args);

#endif
