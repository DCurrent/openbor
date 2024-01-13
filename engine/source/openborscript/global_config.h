typedef enum
{
	GLOBAL_CONFIG_PROPERTY_AJSPECIAL,
	GLOBAL_CONFIG_PROPERTY_BLOCK_RATIO,
	GLOBAL_CONFIG_PROPERTY_BLOCK_TYPE,
	GLOBAL_CONFIG_PROPERTY_CHEATS,
	GLOBAL_CONFIG_PROPERTY_FLASH,
	GLOBAL_CONFIG_PROPERTY_SHOW_GO,
	GLOBAL_CONFIG_PROPERTY_END,
} e_global_config_properties;

// Binding properties.
HRESULT openbor_get_global_config_property(const ScriptVariant* const* varlist, ScriptVariant** const pretvar, const int paramCount);
HRESULT openbor_set_global_config_property(ScriptVariant** varlist, ScriptVariant** const pretvar, const int paramCount);
