// Caskey, Damon V.
// 2018-05-13
//
// Axis property prototypes and constants.

typedef enum
{
    _AXIS_PLANE_LATERAL_X,
    _AXIS_PLANE_LATERAL_Z,
    _AXIS_PLANE_LATERAL_END,
} e_axis_plane_lateral_properties;

typedef enum
{
    _AXIS_PLANE_VERTICAL_X,
    _AXIS_PLANE_VERTICAL_Y,
    _AXIS_PLANE_VERTICAL_END,
} e_axis_plane_vertical_properties;

typedef enum
{
    _AXIS_PRINCIPAL_X,
    _AXIS_PRINCIPAL_Y,
    _AXIS_PRINCIPAL_Z,
    _AXIS_PRINCIPAL_END,
} e_axis_principal_properties;

// Access and mutator prototypes.

// Lateral plane (x, z).
HRESULT openbor_get_axis_plane_lateral_float_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set_axis_plane_lateral_float_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

HRESULT openbor_get_axis_plane_lateral_int_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set_axis_plane_lateral_int_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

// Vertical plane (x, y).
HRESULT openbor_get_axis_plane_vertical_int_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set_axis_plane_vertical_int_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

// Principal (x, y, z).
HRESULT openbor_get_axis_principal_float_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set_axis_principal_float_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

HRESULT openbor_get_axis_principal_int_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set_axis_principal_int_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

// Mapstring prototypes.
int mapstrings_axis_plane_lateral_property(ScriptVariant **varlist, int paramCount);
int mapstrings_axis_plane_vertical_property(ScriptVariant **varlist, int paramCount);
int mapstrings_axis_principal_property(ScriptVariant **varlist, int paramCount);

