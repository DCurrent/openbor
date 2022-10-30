typedef enum
{
	_GLOBAL_CONFIG_AJSPECIAL,
	_GLOBAL_CONFIG_CHEATS,
	_GLOBAL_CONFIG_END,
} e_global_config_properties;

// Binding properties.
HRESULT openbor_get_global_config_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount);
HRESULT openbor_set_global_config_property(ScriptVariant** varlist, ScriptVariant** pretvar, int paramCount);

int mapstrings_global_config_property(ScriptVariant** varlist, int paramCount);
