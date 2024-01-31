typedef enum
{
	FLASH_PROPERTY_LAYER_ADJUST,
	FLASH_PROPERTY_LAYER_SOURCE,
	FLASH_PROPERTY_MODEL_BLOCK,
	FLASH_PROPERTY_MODEL_HIT,
	FLASH_PROPERTY_Z_SOURCE,
	FLASH_PROPERTY_END,
} e_flash_properties;

// Binding properties.
HRESULT openbor_get_flash_property(const ScriptVariant* const* varlist, ScriptVariant** const pretvar, const int paramCount);
HRESULT openbor_set_flash_property(ScriptVariant** varlist, ScriptVariant** const pretvar, const int paramCount);
