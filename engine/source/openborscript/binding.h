
typedef enum
{
    _binding_animation,
    _binding_bind_x,
    _binding_bind_y,
    _binding_bind_z,
    _binding_direction,
    _binding_entity,
    _binding_offset,
    _binding_sort_id,
    _binding_the_end,
} e_binding_properties;

// Binding properties.
HRESULT openbor_get_binding_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);
HRESULT openbor_set_binding_property(ScriptVariant **varlist , ScriptVariant **pretvar, int paramCount);

int mapstrings_binding(ScriptVariant **varlist, int paramCount);

