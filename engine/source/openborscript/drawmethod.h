typedef enum
{
	DRAWMETHOD_PROPERTY_ALPHA,
	DRAWMETHOD_PROPERTY_CENTER_X,
	DRAWMETHOD_PROPERTY_CENTER_Y,
	DRAWMETHOD_PROPERTY_CHANNEL_BLUE,
	DRAWMETHOD_PROPERTY_CHANNEL_GREEN,
	DRAWMETHOD_PROPERTY_CHANNEL_RED,
	DRAWMETHOD_PROPERTY_CLIP_POSITION_X,
	DRAWMETHOD_PROPERTY_CLIP_POSITION_Y,
	DRAWMETHOD_PROPERTY_CLIP_SIZE_X,
	DRAWMETHOD_PROPERTY_CLIP_SIZE_Y,
	DRAWMETHOD_PROPERTY_COLORSET_INDEX,
	DRAWMETHOD_PROPERTY_COLORSET_TABLE,
	DRAWMETHOD_PROPERTY_CONFIG,
	DRAWMETHOD_PROPERTY_FILL_COLOR,
	DRAWMETHOD_PROPERTY_REPEAT_X,
	DRAWMETHOD_PROPERTY_REPEAT_Y,
	DRAWMETHOD_PROPERTY_ROTATE,
	DRAWMETHOD_PROPERTY_SCALE_X,
	DRAWMETHOD_PROPERTY_SCALE_Y,
	DRAWMETHOD_PROPERTY_SHIFT_X,
	DRAWMETHOD_PROPERTY_SPAN_X,
	DRAWMETHOD_PROPERTY_SPAN_Y,
	DRAWMETHOD_PROPERTY_TAG,
	DRAWMETHOD_PROPERTY_TINT_COLOR,
	DRAWMETHOD_PROPERTY_TINT_MODE,	
	DRAWMETHOD_PROPERTY_WATER_MODE,
	DRAWMETHOD_PROPERTY_WATER_PERSPECTIVE,
	DRAWMETHOD_PROPERTY_WATER_SIZE_BEGIN,
	DRAWMETHOD_PROPERTY_WATER_SIZE_END,
	DRAWMETHOD_PROPERTY_WATER_WAVE_AMPLITUDE,
	DRAWMETHOD_PROPERTY_WATER_WAVE_LENGTH,
	DRAWMETHOD_PROPERTY_WATER_WAVE_SPEED,
	DRAWMETHOD_PROPERTY_WATER_WAVE_TIME,
	DRAWMETHOD_PROPERTY_END,
} e_drawmethod_properties;

// Drawmethod properties.
HRESULT openbor_allocate_drawmethod(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_copy_drawmethod(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_get_drawmethod_property(const ScriptVariant* const* varlist, ScriptVariant** const pretvar, const int paramCount);
HRESULT openbor_set_drawmethod_property(ScriptVariant** varlist, ScriptVariant** const pretvar, const int paramCount);

HRESULT openbor_allocate_palette(ScriptVariant** varlist, ScriptVariant** pretvar, const int paramCount);
HRESULT openbor_copy_palette(ScriptVariant** varlist, ScriptVariant** pretvar, const int paramCount);
HRESULT openbor_load_palette(ScriptVariant** varlist, ScriptVariant** pretvar, const int paramCount);
HRESULT openbor_get_palette_property(const ScriptVariant* const* varlist, ScriptVariant** const pretvar, const int paramCount);
HRESULT openbor_set_palette_property(ScriptVariant** varlist, ScriptVariant** const pretvar, const int paramCount);