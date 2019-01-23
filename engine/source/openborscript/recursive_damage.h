// Recursive damage properties
// 2019-01-19
// Caskey, Damon V.

typedef enum
{
	_RECURSIVE_DAMAGE_FORCE,
	_RECURSIVE_DAMAGE_INDEX,
	_RECURSIVE_DAMAGE_MODE,
	_RECURSIVE_DAMAGE_NEXT,
	_RECURSIVE_DAMAGE_OWNER,
	_RECURSIVE_DAMAGE_RATE,
	_RECURSIVE_DAMAGE_TAG,
	_RECURSIVE_DAMAGE_TICK,
	_RECURSIVE_DAMAGE_TIME,
	_RECURSIVE_DAMAGE_TYPE,
	_RECURSIVE_DAMAGE_END,
} e_recursive_damage_properties;

HRESULT openbor_get_recursive_damage_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set_recursive_damage_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);

int mapstrings_recursive_damage_property(ScriptVariant **varlist, int paramCount);