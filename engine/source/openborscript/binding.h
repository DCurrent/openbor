
typedef enum
{
	BIND_PROPERTY_ANIMATION_FRAME,
	BIND_PROPERTY_ANIMATION_ID,
	BIND_PROPERTY_CONFIG,
	BIND_PROPERTY_DIRECTION_ADJUST,
	BIND_PROPERTY_META_DATA,
	BIND_PROPERTY_META_TAG,
	BIND_PROPERTY_OFFSET_X,
	BIND_PROPERTY_OFFSET_Y,
	BIND_PROPERTY_OFFSET_Z,
	BIND_PROPERTY_SORT_ID,
	BIND_PROPERTY_TARGET,
	BIND_PROPERTY_END,
} e_bind_properties;

// Binding properties.
HRESULT openbor_get_bind_property(const ScriptVariant* const* varlist, ScriptVariant** const pretvar, const int paramCount);
HRESULT openbor_set_bind_property(ScriptVariant** varlist, ScriptVariant** const pretvar, const int paramCount);
HRESULT openbor_update_bind(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);

