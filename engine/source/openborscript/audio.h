
typedef enum e_music_channel_properties
{
	MUSIC_CHANNEL_PROPERTY_ACTIVE,
	MUSIC_CHANNEL_PROPERTY_BUFFER_LIST,
	MUSIC_CHANNEL_PROPERTY_CHANNELS,
	MUSIC_CHANNEL_PROPERTY_PAUSED,
	MUSIC_CHANNEL_PROPERTY_PERIOUD,
	MUSIC_CHANNEL_PROPERTY_PLAY_BUFFER,
	MUSIC_CHANNEL_PROPERTY_PLAY_TO,
	MUSIC_CHANNEL_PROPERTY_SAMPLE_POSITION,
	MUSIC_CHANNEL_PROPERTY_VOLUME_LEFT,
	MUSIC_CHANNEL_PROPERTY_VOLUME_RIGHT,
	MUSIC_CHANNEL_PROPERTY_END,
} e_music_channel_properties;

// Drawmethod properties.
HRESULT openbor_get_music_channel_property(const ScriptVariant* const* varlist, ScriptVariant** const pretvar, const int paramCount);
HRESULT openbor_set_music_channel_property(ScriptVariant** varlist, ScriptVariant** const pretvar, const int paramCount);