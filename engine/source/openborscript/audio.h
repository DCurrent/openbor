/*
* Caskey, Damon V.
* 2026-08-02
*
* Script-visible properties for a sound object
* retained by the banked channel pool.
*/
typedef enum e_sound_properties {
    SOUND_PROPERTY_ACTIVE,
    SOUND_PROPERTY_CHANNEL,
    SOUND_PROPERTY_CHANNELS,
    SOUND_PROPERTY_LOOP_OFFSET,
    SOUND_PROPERTY_PAUSED,
    SOUND_PROPERTY_PERIOD,
    SOUND_PROPERTY_PLAY_ID,
    SOUND_PROPERTY_PRIORITY,
    SOUND_PROPERTY_SAMPLE,
    SOUND_PROPERTY_SAMPLE_POSITION,
    SOUND_PROPERTY_VOLUME_DIVISOR,
    SOUND_PROPERTY_VOLUME_LEFT,
    SOUND_PROPERTY_VOLUME_RIGHT,
    SOUND_PROPERTY_END
} e_sound_properties;

HRESULT openbor_get_sound_channel_bank_mask(ScriptVariant** varlist, ScriptVariant** pretvar, const int paramCount);
HRESULT openbor_get_sound_channel_mask(ScriptVariant** varlist, ScriptVariant** pretvar, const int paramCount);
HRESULT openbor_get_sound_channel_object(ScriptVariant** varlist, ScriptVariant** pretvar, const int paramCount);
HRESULT openbor_get_sound_channel_index(ScriptVariant** varlist, ScriptVariant** pretvar, const int paramCount);
HRESULT openbor_get_sound_property(const ScriptVariant* const* varlist, ScriptVariant** const pretvar, const int paramCount);
HRESULT openbor_set_sound_property(ScriptVariant** varlist, ScriptVariant** const pretvar, const int paramCount);
