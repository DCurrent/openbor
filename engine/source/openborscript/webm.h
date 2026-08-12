/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) OpenBOR Team
 */

#ifndef OPENBORSCRIPT_WEBM_H
#define OPENBORSCRIPT_WEBM_H

typedef enum e_webm_properties {
    WEBM_PROPERTY_ACTIVE,
    WEBM_PROPERTY_CHANNEL,
    WEBM_PROPERTY_DISPLAY_MODE,
    WEBM_PROPERTY_DURATION,       /* Milliseconds, read only. */
    WEBM_PROPERTY_HEIGHT,
    WEBM_PROPERTY_INTERRUPT,
    WEBM_PROPERTY_LOADING,
    WEBM_PROPERTY_OFFSET_X,
    WEBM_PROPERTY_OFFSET_Y,
    WEBM_PROPERTY_PAUSED,
    WEBM_PROPERTY_POSITION,       /* Milliseconds; writing seeks. */
    WEBM_PROPERTY_REPEAT,
    WEBM_PROPERTY_SCREEN,
    WEBM_PROPERTY_SOUND_CHANNEL,
    WEBM_PROPERTY_SPEED,
    WEBM_PROPERTY_WIDTH,
    WEBM_PROPERTY_END
} e_webm_properties;

HRESULT openbor_open_webm(
    ScriptVariant **varlist,
    ScriptVariant **pretvar,
    int paramCount
);
HRESULT openbor_get_webm_channel_object(
    ScriptVariant **varlist,
    ScriptVariant **pretvar,
    int paramCount
);
HRESULT openbor_get_webm_channel_index(
    ScriptVariant **varlist,
    ScriptVariant **pretvar,
    int paramCount
);
HRESULT openbor_get_webm_channel_mask(
    ScriptVariant **varlist,
    ScriptVariant **pretvar,
    int paramCount
);
HRESULT openbor_get_webm_property(
    ScriptVariant **varlist,
    ScriptVariant **pretvar,
    int paramCount
);
HRESULT openbor_set_webm_property(
    ScriptVariant **varlist,
    ScriptVariant **pretvar,
    int paramCount
);
HRESULT openbor_stop_webm(
    ScriptVariant **varlist,
    ScriptVariant **pretvar,
    int paramCount
);

#endif
