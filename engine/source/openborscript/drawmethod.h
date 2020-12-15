typedef enum
{
	_DRAWMETHOD_ALPHA,
	_DRAWMETHOD_BACKGROUND_TRANSPARENCY,
	_DRAWMETHOD_CENTER_X,
	_DRAWMETHOD_CENTER_Y,
	_DRAWMETHOD_CHANNEL_BLUE,
	_DRAWMETHOD_CHANNEL_GREEN,
	_DRAWMETHOD_CHANNEL_RED,
	_DRAWMETHOD_CLIP_POSITION_X,
	_DRAWMETHOD_CLIP_POSITION_Y,
	_DRAWMETHOD_CLIP_SIZE_X,
	_DRAWMETHOD_CLIP_SIZE_Y,
	_DRAWMETHOD_COLORSET_INDEX,
	_DRAWMETHOD_COLORSET_TABLE,
	_DRAWMETHOD_ENABLE,
	_DRAWMETHOD_FILL_COLOR,
	_DRAWMETHOD_FLIP_X,
	_DRAWMETHOD_FLIP_Y,
	_DRAWMETHOD_REPEAT_X,
	_DRAWMETHOD_REPEAT_Y,
	_DRAWMETHOD_ROTATE,
	_DRAWMETHOD_ROTATE_FLIP,
	_DRAWMETHOD_SCALE_X,
	_DRAWMETHOD_SCALE_Y,
	_DRAWMETHOD_SHIFT_X,
	_DRAWMETHOD_SPAN_X,
	_DRAWMETHOD_SPAN_Y,
	_DRAWMETHOD_TAG,
	_DRAWMETHOD_TINT_COLOR,
	_DRAWMETHOD_TINT_MODE,	
	_DRAWMETHOD_WATER_MODE,
	_DRAWMETHOD_WATER_PERSPECTIVE,
	_DRAWMETHOD_WATER_SIZE_BEGIN,
	_DRAWMETHOD_WATER_SIZE_END,
	_DRAWMETHOD_WATER_WAVE_AMPLITUDE,
	_DRAWMETHOD_WATER_WAVE_LENGTH,
	_DRAWMETHOD_WATER_WAVE_SPEED,
	_DRAWMETHOD_WATER_WAVE_TIME,
	_DRAWMETHOD_END,
} e_drawmethod_properties;

// Drawmethod properties.
HRESULT openbor_allocate_drawmethod(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_copy_drawmethod(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_free_drawmethod(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_get_drawmethod_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set_drawmethod_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);

int mapstrings_drawmethod(ScriptVariant **varlist, int paramCount);