typedef enum
{
	DRAWMETHOD_PROPERTY_ALPHA,
	DRAWMETHOD_PROPERTY_CENTER_X,
	DRAWMETHOD_PROPERTY_CENTER_Y,
	DRAWMETHOD_PROPERTY_CHANNEL_BLUE,
	DRAWMETHOD_PROPERTY_CHANNEL_GREEN,
	DRAWMETHOD_PROPERTY_CHANNEL_RED,
	DRAWMETHOD_PROPERTY_CLIP_POSITION_X,
	DRAWMETHOD_PROPERTY_CLIP_POSITION_Y,
	DRAWMETHOD_PROPERTY_CLIP_SIZE_X,
	DRAWMETHOD_PROPERTY_CLIP_SIZE_Y,
	DRAWMETHOD_PROPERTY_COLORSET_INDEX,
	DRAWMETHOD_PROPERTY_COLORSET_TABLE,
	DRAWMETHOD_PROPERTY_CONFIG,
	DRAWMETHOD_PROPERTY_FILL_COLOR,
	DRAWMETHOD_PROPERTY_REPEAT_X,
	DRAWMETHOD_PROPERTY_REPEAT_Y,
	DRAWMETHOD_PROPERTY_ROTATE,
	DRAWMETHOD_PROPERTY_SCALE_X,
	DRAWMETHOD_PROPERTY_SCALE_Y,
	DRAWMETHOD_PROPERTY_SHIFT_X,
	DRAWMETHOD_PROPERTY_SPAN_X,
	DRAWMETHOD_PROPERTY_SPAN_Y,
	DRAWMETHOD_PROPERTY_TAG,
	DRAWMETHOD_PROPERTY_TINT_COLOR,
	DRAWMETHOD_PROPERTY_TINT_MODE,	
	DRAWMETHOD_PROPERTY_WATER_MODE,
	DRAWMETHOD_PROPERTY_WATER_PERSPECTIVE,
	DRAWMETHOD_PROPERTY_WATER_SIZE_BEGIN,
	DRAWMETHOD_PROPERTY_WATER_SIZE_END,
	DRAWMETHOD_PROPERTY_WATER_WAVE_AMPLITUDE,
	DRAWMETHOD_PROPERTY_WATER_WAVE_LENGTH,
	DRAWMETHOD_PROPERTY_WATER_WAVE_SPEED,
	DRAWMETHOD_PROPERTY_WATER_WAVE_TIME,

    /* Append color controls so existing numeric property IDs remain stable. */
    DRAWMETHOD_PROPERTY_CHROMA_KEY_A,
    DRAWMETHOD_PROPERTY_CHROMA_KEY_B,
    DRAWMETHOD_PROPERTY_CHROMA_KEY_C,
    DRAWMETHOD_PROPERTY_CHROMA_KEY_D,
    DRAWMETHOD_PROPERTY_CHROMA_KEY_E,
    DRAWMETHOD_PROPERTY_CHROMA_KEY_ENABLED,
    DRAWMETHOD_PROPERTY_SUBSTITUTE_A,
    DRAWMETHOD_PROPERTY_SUBSTITUTE_B,
    DRAWMETHOD_PROPERTY_SUBSTITUTE_C,
    DRAWMETHOD_PROPERTY_SUBSTITUTE_D,
    DRAWMETHOD_PROPERTY_SUBSTITUTE_E,
    DRAWMETHOD_PROPERTY_SUBSTITUTE_ENABLED,
	DRAWMETHOD_PROPERTY_END,
} e_drawmethod_properties;

/* Public shorthand used by the constant-based drawmethod convenience API. */
#define DM_CHROMA_KEY_A DRAWMETHOD_PROPERTY_CHROMA_KEY_A
#define DM_CHROMA_KEY_B DRAWMETHOD_PROPERTY_CHROMA_KEY_B
#define DM_CHROMA_KEY_C DRAWMETHOD_PROPERTY_CHROMA_KEY_C
#define DM_CHROMA_KEY_D DRAWMETHOD_PROPERTY_CHROMA_KEY_D
#define DM_CHROMA_KEY_E DRAWMETHOD_PROPERTY_CHROMA_KEY_E
#define DM_CHROMA_KEY_ENABLED DRAWMETHOD_PROPERTY_CHROMA_KEY_ENABLED
#define DM_SUBSTITUTE_A DRAWMETHOD_PROPERTY_SUBSTITUTE_A
#define DM_SUBSTITUTE_B DRAWMETHOD_PROPERTY_SUBSTITUTE_B
#define DM_SUBSTITUTE_C DRAWMETHOD_PROPERTY_SUBSTITUTE_C
#define DM_SUBSTITUTE_D DRAWMETHOD_PROPERTY_SUBSTITUTE_D
#define DM_SUBSTITUTE_E DRAWMETHOD_PROPERTY_SUBSTITUTE_E
#define DM_SUBSTITUTE_ENABLED DRAWMETHOD_PROPERTY_SUBSTITUTE_ENABLED
#define DM_COLOR_MATCH 0
#define DM_COLOR_SUBSTITUTE 1
#define DM_COLOR_ENABLED 2

// Drawmethod properties.
HRESULT openbor_allocate_drawmethod(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_copy_drawmethod(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_get_drawmethod_property(const ScriptVariant* const* varlist, ScriptVariant** const pretvar, const int paramCount);
HRESULT openbor_set_drawmethod_property(ScriptVariant** varlist, ScriptVariant** const pretvar, const int paramCount);
HRESULT openbor_set_draw_method(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT openbor_get_draw_method(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
int drawmethod_color_property_set(s_drawmethod *method, int property,
                                 const ScriptVariant *const *values, int count);
int drawmethod_color_property_get(const s_drawmethod *method, int property,
                                 int component, LONG *value);

HRESULT openbor_allocate_palette(ScriptVariant** varlist, ScriptVariant** pretvar, const int paramCount);
HRESULT openbor_copy_palette(ScriptVariant** varlist, ScriptVariant** pretvar, const int paramCount);
HRESULT openbor_load_palette(ScriptVariant** varlist, ScriptVariant** pretvar, const int paramCount);
HRESULT openbor_get_palette_property(const ScriptVariant* const* varlist, ScriptVariant** const pretvar, const int paramCount);
HRESULT openbor_set_palette_property(ScriptVariant** varlist, ScriptVariant** const pretvar, const int paramCount);
