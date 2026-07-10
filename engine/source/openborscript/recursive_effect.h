// Recursive effect properties
// 2019-01-19
// Caskey, Damon V.

typedef enum
{
	_RECURSIVE_EFFECT_FORCE,
	_RECURSIVE_EFFECT_INDEX,
	_RECURSIVE_EFFECT_MODE,
	_RECURSIVE_EFFECT_OWNER,
	_RECURSIVE_EFFECT_RATE,
	_RECURSIVE_EFFECT_TAG,
	_RECURSIVE_EFFECT_TICK,
	_RECURSIVE_EFFECT_TIME,
	_RECURSIVE_EFFECT_TYPE,
	_RECURSIVE_EFFECT_END,
} e_recursive_effect_properties;

HRESULT openbor_get_recursive_effect_object(ScriptVariant** varlist, ScriptVariant** pretvar, const int paramCount);
HRESULT openbor_get_recursive_effect_property(ScriptVariant **varlist, ScriptVariant **pretvar, const int paramCount);
HRESULT openbor_set_recursive_effect_property(ScriptVariant **varlist, ScriptVariant **pretvar, const int paramCount);

int mapstrings_recursive_effect_property(ScriptVariant **varlist, int paramCount);