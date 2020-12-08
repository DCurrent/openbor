
typedef enum
{
	_BIND_ANIMATION_FRAME,
	_BIND_ANIMATION_ID,
	_BIND_ANIMATION_MATCH,
	_BIND_DIRECTION,
	_BIND_MODE_X,
	_BIND_MODE_Y,
	_BIND_MODE_Z,
	_BIND_OFFSET_X,
	_BIND_OFFSET_Y,
	_BIND_OFFSET_Z,
	_BIND_OVERRIDE,
	_BIND_SORT_ID,
	_BIND_TAG,
	_BIND_TARGET,
	_BIND_END,
} e_bind_properties;

// Binding properties.
HRESULT openbor_get_bind_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set_bind_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_update_bind(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);

int mapstrings_bind_property(ScriptVariant **varlist, int paramCount);

