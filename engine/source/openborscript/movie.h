/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) OpenBOR Team
 */

#ifndef OPENBORSCRIPT_MOVIE_H
#define OPENBORSCRIPT_MOVIE_H

typedef enum e_movie_properties {
    MOVIE_PROPERTY_ACTIVE,
    MOVIE_PROPERTY_BLACK_FILTER,
    MOVIE_PROPERTY_CHANNEL,
    MOVIE_PROPERTY_DURATION,       /* Milliseconds, read only. */
    MOVIE_PROPERTY_HEIGHT,
    MOVIE_PROPERTY_INTERRUPT,
    MOVIE_PROPERTY_OFFSET_X,
    MOVIE_PROPERTY_OFFSET_Y,
    MOVIE_PROPERTY_PAUSED,
    MOVIE_PROPERTY_POSITION,       /* Milliseconds; writing seeks. */
    MOVIE_PROPERTY_REPEAT,
    MOVIE_PROPERTY_SOUND_CHANNEL,
    MOVIE_PROPERTY_SOURCE,
    MOVIE_PROPERTY_SPEED,
    MOVIE_PROPERTY_WIDTH,
    MOVIE_PROPERTY_END
} e_movie_properties;

HRESULT openbor_movie_load(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_movie_unload(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_movie_play(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_movie_draw_to_screen(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_movie_set_sound_channel(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_movie_stop(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_movie_get_channel_object(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_movie_get_channel_index(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_movie_get_channel_mask(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_movie_get_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_movie_set_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);

#endif
