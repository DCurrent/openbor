/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c)  OpenBOR Team
 */

 // Drawmethod Properties
 // 2019-03-28
 // Caskey, Damon V.

#include "scriptcommon.h"

const s_property_access_map drawmethod_get_property_map(const void* acting_object_param, const unsigned int property_index_param)
{
	s_property_access_map property_map;
	const s_drawmethod* acting_object = acting_object_param;
	const e_drawmethod_properties property_index = property_index_param;

	switch (property_index)
	{
	case DRAWMETHOD_PROPERTY_ALPHA:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->alpha;
		property_map.id_string = "DRAWMETHOD_PROPERTY_ALPHA";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_CENTER_X:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->centerx;
		property_map.id_string = "DRAWMETHOD_PROPERTY_CENTER_X";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_CENTER_Y:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->centery;
		property_map.id_string = "DRAWMETHOD_PROPERTY_CENTER_Y";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_CHANNEL_BLUE:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->channelb;
		property_map.id_string = "DRAWMETHOD_PROPERTY_CHANNEL_BLUE";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_CHANNEL_GREEN:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->channelg;
		property_map.id_string = "DRAWMETHOD_PROPERTY_CHANNEL_GREEN";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_CHANNEL_RED:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->channelr;
		property_map.id_string = "DRAWMETHOD_PROPERTY_CHANNEL_RED";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_CLIP_POSITION_X:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->clipx;
		property_map.id_string = "DRAWMETHOD_PROPERTY_CLIP_POSITION_X";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_CLIP_POSITION_Y:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->clipy;
		property_map.id_string = "DRAWMETHOD_PROPERTY_CLIP_POSITION_Y";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_CLIP_SIZE_X:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->clipw;
		property_map.id_string = "DRAWMETHOD_PROPERTY_CLIP_SIZE_X";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_CLIP_SIZE_Y:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->cliph;
		property_map.id_string = "DRAWMETHOD_PROPERTY_CLIP_SIZE_Y";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_COLORSET_INDEX:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->remap;
		property_map.id_string = "DRAWMETHOD_PROPERTY_COLORSET_INDEX";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_COLORSET_TABLE:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->table;
		property_map.id_string = "DRAWMETHOD_PROPERTY_COLORSET_TABLE";
		property_map.type = VT_PTR;
		break;

	case DRAWMETHOD_PROPERTY_CONFIG:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->config;
		property_map.id_string = "DRAWMETHOD_PROPERTY_CONFIG";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_FILL_COLOR:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->fillcolor;
		property_map.id_string = "DRAWMETHOD_PROPERTY_FILL_COLOR";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_REPEAT_X:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->xrepeat;
		property_map.id_string = "DRAWMETHOD_PROPERTY_REPEAT_X";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_REPEAT_Y:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->yrepeat;
		property_map.id_string = "DRAWMETHOD_PROPERTY_REPEAT_Y";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_ROTATE:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->rotate;
		property_map.id_string = "DRAWMETHOD_PROPERTY_ROTATE";
		property_map.type = VT_DECIMAL;
		break;

	case DRAWMETHOD_PROPERTY_SCALE_X:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->scalex;
		property_map.id_string = "DRAWMETHOD_PROPERTY_SCALE_X";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_SCALE_Y:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->scaley;
		property_map.id_string = "DRAWMETHOD_PROPERTY_SCALE_Y";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_SPAN_X:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->xspan;
		property_map.id_string = "DRAWMETHOD_PROPERTY_SPAN_X";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_SPAN_Y:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->yspan;
		property_map.id_string = "DRAWMETHOD_PROPERTY_SPAN_Y";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_TINT_COLOR:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->tintcolor;
		property_map.id_string = "DRAWMETHOD_PROPERTY_TINT_COLOR";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_TINT_MODE:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->tintmode;
		property_map.id_string = "DRAWMETHOD_PROPERTY_TINT_MODE";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_WATER_MODE:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->water.watermode;
		property_map.id_string = "DRAWMETHOD_PROPERTY_WATER_MODE";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_WATER_PERSPECTIVE:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->water.perspective;
		property_map.id_string = "DRAWMETHOD_PROPERTY_WATER_PERSPECTIVE";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_WATER_SIZE_BEGIN:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->water.beginsize;
		property_map.id_string = "DRAWMETHOD_PROPERTY_WATER_SIZE_BEGIN";
		property_map.type = VT_DECIMAL;
		break;

	case DRAWMETHOD_PROPERTY_WATER_SIZE_END:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->water.endsize;
		property_map.id_string = "DRAWMETHOD_PROPERTY_WATER_SIZE_END";
		property_map.type = VT_DECIMAL;
		break;

	case DRAWMETHOD_PROPERTY_WATER_WAVE_AMPLITUDE:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->water.amplitude;
		property_map.id_string = "DRAWMETHOD_PROPERTY_WATER_WAVE_AMPLITUDE";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_WATER_WAVE_LENGTH:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->water.wavelength;
		property_map.id_string = "DRAWMETHOD_PROPERTY_WATER_WAVE_AMPLITUDE";
		property_map.type = VT_DECIMAL;
		break;

	case DRAWMETHOD_PROPERTY_WATER_WAVE_SPEED:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->water.wavespeed;
		property_map.id_string = "DRAWMETHOD_PROPERTY_WATER_WAVE_SPEED";
		property_map.type = VT_DECIMAL;
		break;

	case DRAWMETHOD_PROPERTY_WATER_WAVE_TIME:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->water.wavetime;
		property_map.id_string = "DRAWMETHOD_PROPERTY_WATER_WAVE_TIME";
		property_map.type = VT_INTEGER;
		break;

	case DRAWMETHOD_PROPERTY_END:
	default:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_NONE;
		property_map.field = NULL;
		property_map.id_string = "Drawmethod";
		property_map.type = VT_EMPTY;
		break;

	}
	return property_map;
}

// Caskey, Damon  V.
// 2019-04-15
//
// Allocate a new drawmethod and return the pointer.
HRESULT openbor_allocate_drawmethod(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
	extern s_drawmethod *allocate_drawmethod();
	s_drawmethod *drawmethod;

	ScriptVariant_ChangeType(*pretvar, VT_PTR);
	
	if ((drawmethod = allocate_drawmethod()))
	{
		(*pretvar)->ptrVal = (s_drawmethod *)drawmethod;
	}

	return S_OK;
}

// Caskey, Damon  V.
// 2019-04-15
//
// Copy properties of source drawmethod to target drawmethod.
HRESULT openbor_copy_drawmethod(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
	const char* SELF_NAME = "copy_drawmethod(void target, void source)";
	const int ARG_MINIMUM = 2;   // Minimum required arguments.
	const int ARG_TARGET = 0;
	const int ARG_SOURCE = 1;

	// Verify arguments. 
	if (paramCount < ARG_MINIMUM
		|| varlist[ARG_SOURCE]->vt != VT_PTR
		|| varlist[ARG_TARGET]->vt != VT_PTR)
	{
		printf("\n\n Script error: %s. You must provide a valid target and source object.\n\n", SELF_NAME);
		*pretvar = NULL;
		return E_FAIL;
	}
	
	// Populate local vars with arguments.
	const s_drawmethod* const source = (const s_drawmethod *)varlist[ARG_SOURCE]->ptrVal;
	s_drawmethod* target = (s_drawmethod *)varlist[ARG_TARGET]->ptrVal;

	/*
	* Default drawmethod (plainmethod) is a const and 
	* therefore cannot be mutated. We'll send a warning 
	* to the log and exit function. Note it's perfectly 
	* fine to use the default drawmethod as a source, 
	* and that's probably what will be done most of the 
	* time anyway.
	*/
	if (target == &plainmethod)
	{
		printf("\n Warning: The default drawmethod and its properties are read only: %s\n\n", SELF_NAME);

		return S_OK;
	}

	// Copy values into target drawmethod.
	memcpy(target, source, sizeof(*target));

	return S_OK;
}

/*
* Caskey, Damon  V.
* 2023-03-03
*
* Return a property. Requires
* a object pointer and property
* constant to access.
*/
HRESULT openbor_get_drawmethod_property(const ScriptVariant* const* varlist, ScriptVariant** const pretvar, const int paramCount)
{
	const char* SELF_NAME = "openbor_get_drawmethod_property(void drawmethod, int property)";
	const int ARG_OBJECT = 0;
	const int ARG_PROPERTY = 1;

	/*
	* Clear pass by reference argument used to send
	* property data back to calling script.
	*/
	ScriptVariant_Clear(*pretvar);

	/*
	* Should at least be a pointer to the
	* acting object and a property id.
	*/
	if (varlist[ARG_OBJECT]->vt != VT_PTR
		|| varlist[ARG_PROPERTY]->vt != VT_INTEGER) {
		printf("\n\n Script error: %s. You must provide a valid object pointer and property id.\n\n", SELF_NAME);
		return E_FAIL;
	}

	/*
	* Now let's make sure the object type is
	* correct (ex. drawmethod vs. model) so we
	* can shut down gracefully if there's
	* a mismatch.
	*/

	const s_drawmethod* const acting_object = (const s_drawmethod* const)varlist[ARG_OBJECT]->ptrVal;

	if (acting_object->object_type != OBJECT_TYPE_DRAWMETHOD) {
		printf("\n\nScript error: %s. Object pointer is not correct type.\n\n", SELF_NAME);
		*pretvar = NULL;
		return E_FAIL;
	}

	const int property_id_param = (const int)varlist[ARG_PROPERTY]->lVal;

	const e_drawmethod_properties property_id = (e_drawmethod_properties)(property_id_param);
	const s_property_access_map property_map = drawmethod_get_property_map(acting_object, property_id);

	/*
	* If property id is in range, we send
	* the property map and return parameter
	* for population, then ext.
	*/

	if (property_id_param >= 0 && property_id_param < DRAWMETHOD_PROPERTY_END) {
		property_access_get_member(&property_map, *pretvar);
		return S_OK;
	}

	/*
	* Is this a dump request? If not, then
	* the property id is invalid.
	*/

	if (property_id_param == PROPERTY_ACCESS_DUMP) {
		property_access_dump_members(drawmethod_get_property_map, DRAWMETHOD_PROPERTY_END, acting_object);
	}
	else {
		printf("\n\nScript error: %s. Unknown property id (%d). \n\n", SELF_NAME, property_id_param);
		return E_FAIL;
	}

	return S_OK;
}


/*
* Caskey, Damon  V.
* 2018-04-03
*
* Mutate a property. Requires
* the object pointer, a property
* id, and new value.
*/
HRESULT openbor_set_drawmethod_property(ScriptVariant** varlist, ScriptVariant** const pretvar, const int paramCount)
{
	const char* SELF_NAME = "openbor_set_drawmethod_property(void drawmethod, int property, <mixed> value)";
	const int ARG_OBJECT = 0;
	const int ARG_PROPERTY = 1;
	const int ARG_VALUE = 2;
	const int ARG_MINIMUM = 3;

	/*
	* Should at least be a pointer to the
	* acting object, a property id, and
	* a new value.
	*/

	if (varlist[ARG_OBJECT]->vt != VT_PTR
		|| varlist[ARG_PROPERTY]->vt != VT_INTEGER
		|| paramCount < ARG_MINIMUM) {
		printf("\n\n Script error: %s. You must provide a valid object pointer, property id, and new value.\n\n", SELF_NAME);
		*pretvar = NULL;
		return E_FAIL;
	}

	/*
	* Now let's make sure the object type is
	* correct (ex. drawmethod vs. model) so we
	* can shut down gracefully if there's
	* a mismatch.
	*/

	const s_drawmethod* const acting_object = (const s_drawmethod* const)varlist[ARG_OBJECT]->ptrVal;

	if (acting_object->object_type != OBJECT_TYPE_DRAWMETHOD) {
		printf("\n\nScript error: %s. Object pointer is not correct type.\n\n", SELF_NAME);
		*pretvar = NULL;
		return E_FAIL;
	}

	const int property_id_param = (const int)varlist[ARG_PROPERTY]->lVal;
	const e_drawmethod_properties property_id = (e_drawmethod_properties)(property_id_param);

	if (property_id_param < 0 && property_id_param >= DRAWMETHOD_PROPERTY_END) {
		printf("\n\nScript error: %s. Unknown property id (%d). \n\n", SELF_NAME, property_id_param);
		return E_FAIL;
	}

	/*
	* Get map of property. This is a struct
	* that contains the property variable
	* type, reference to the acting object's
	* appropriate data member, text name,
	* read only, etc.
	*/

	const s_property_access_map property_map = drawmethod_get_property_map(acting_object, property_id);

	/*
	* Populate the property value on
	* acting object and return OK/FAIL.
	*/

	return property_access_set_member(acting_object, &property_map, varlist[ARG_VALUE]);

	return S_OK;
}

/*
* Caskey, Damon C.
* 2024-01-23
* 
* Allocate a new blank palette and return pointer.
*/
HRESULT openbor_allocate_palette(ScriptVariant **varlist, ScriptVariant **pretvar, const int paramCount)
{
	unsigned char* acting_object = NULL; 

	ScriptVariant_ChangeType(*pretvar, VT_PTR);
	
	if ((acting_object = malloc(PAL_BYTES)))
	{
		memset(acting_object, 0, MAX_PAL_SIZE);

		(*pretvar)->ptrVal = (unsigned char *)acting_object;
	}

	return S_OK;
}

/*
* Caskey, Damon V.
* 2024-01-23
* 
* Copy color entries from source to target.
*/
HRESULT openbor_copy_palette(ScriptVariant** varlist, ScriptVariant** pretvar, const int paramCount)
{
	const char* SELF_NAME = "openbor_copy_palette(void target, void source)";
	const int ARG_MINIMUM = 2;   // Minimum required arguments.
	const int ARG_SOURCE = 1;
	const int ARG_TARGET = 0;

	// Verify arguments. 
	if (paramCount < ARG_MINIMUM
		|| varlist[ARG_SOURCE]->vt != VT_PTR
		|| varlist[ARG_TARGET]->vt != VT_PTR){

		printf("\n\n Script error: %s. You must provide a valid target object and source object.\n\n", SELF_NAME);
		*pretvar = NULL;
		return E_FAIL;
	}

	// Populate local vars with arguments.
	const unsigned char* const source = (const unsigned char* const)varlist[ARG_SOURCE]->ptrVal;
	unsigned char* target = (unsigned char*)varlist[ARG_TARGET]->ptrVal;

	// Copy values into target object.
	memcpy(target, source, sizeof(*target));

	return S_OK;
}

HRESULT openbor_load_palette(ScriptVariant** varlist, ScriptVariant** pretvar, const int paramCount)
{
	const char* SELF_NAME = "openbor_load_palette(void target, char path)";
	const int ARG_MINIMUM = 2;   // Minimum required arguments.
	const int ARG_SOURCE = 1;
	const int ARG_TARGET = 0;

	// Verify arguments. 
	if (paramCount < ARG_MINIMUM
		|| varlist[ARG_SOURCE]->vt != VT_STR
		|| varlist[ARG_TARGET]->vt != VT_PTR) {

		printf("\n\n Script error: %s. You must provide a valid target object and source path.\n\n", SELF_NAME);
		*pretvar = NULL;
		return E_FAIL;
	}

	// Populate local vars with arguments.
	char* source = (char*)StrCache_Get(varlist[ARG_SOURCE]->strVal);
	unsigned char* target = (unsigned char*)varlist[ARG_TARGET]->ptrVal;

	if (!target) {
		printf("\n\nScript error: %s. Missing or invalid target pointer.\n\n", SELF_NAME);
		*pretvar = NULL;
		return E_FAIL;
	}

	/*
	* Clear pass by reference argument used to send
	* property data back to calling script.
	*/
	ScriptVariant_Clear(*pretvar);
	ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

	/* Read file and populate palette with values. */
	(*pretvar)->lVal = load_palette(target, source);

	return S_OK;
}

/*
* Caskey, Damon V.
* 2024-01-22
*
* Get palette entry component value.
*/
HRESULT openbor_get_palette_property(const ScriptVariant* const* varlist, ScriptVariant** const pretvar, const int paramCount)
{
	const char* SELF_NAME = "openbor_set_palette_property(void drawmethod, int color_index, int color_component)";
	const int ARG_OBJECT = 0;
	const int ARG_COLOR_INDEX = 1;
	const int ARG_COLOR_COMPONENT = 2;
	const int ARG_MINIMUM = 3;

	/*
	* Verify parameters.
	*/
	if (varlist[ARG_OBJECT]->vt != VT_PTR
		|| varlist[ARG_COLOR_INDEX]->vt != VT_INTEGER
		|| varlist[ARG_COLOR_COMPONENT]->vt != VT_INTEGER
		|| paramCount < ARG_MINIMUM) {
		printf("\n\n Script error: %s. You must provide a valid object pointer, color index, and color component.\n\n", SELF_NAME);
		*pretvar = NULL;
		return E_FAIL;
	}

	/*
	* Verify we don't have a NULL pointer.
	* Our target object is an array, not
	* a structure, so there's no really
	* good way to verify type.
	*/

	unsigned char* acting_object = (unsigned char*)varlist[ARG_OBJECT]->ptrVal;

	if (!acting_object) {
		printf("\n\nScript error: %s. Object pointer is not correct type.\n\n", SELF_NAME);
		*pretvar = NULL;
		return E_FAIL;
	}

	/*
	* Clear pass by reference argument used to send
	* property data back to calling script.
	*/
	ScriptVariant_Clear(*pretvar);

	const unsigned char color_index_param = (unsigned char)varlist[ARG_COLOR_INDEX]->lVal;
	const e_color_components color_component_param = (e_color_components)varlist[ARG_COLOR_COMPONENT]->lVal;

	const unsigned int component_id = COLOR_COMPONENT_END * color_index_param + color_component_param;

	/*
	* If property id is in range, we send
	* the property map and return parameter
	* for population, then ext.
	*/

	if (component_id >= 0 && component_id < MAX_PAL_SIZE) {		

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
			
		(*pretvar)->lVal = acting_object[component_id];

		return S_OK;
	}

	/*
	* Is this a dump request? If not, then
	* the property id is invalid.
	*/

	//if (color_index_param == PROPERTY_ACCESS_DUMP) {
		
	//}
	//else {
	//	printf("\n\nScript error: %s. Unknown color index (%d) or component (%d). \n\n", SELF_NAME, color_index_param, color_component_param);
	//	return E_FAIL;
	//}

	return S_OK;
}

/*
* Caskey, Damon V.
* 2024-01-22
* 
* Edit palette entry component value.
*/
HRESULT openbor_set_palette_property(ScriptVariant** varlist, ScriptVariant** const pretvar, const int paramCount)
{
	const char* SELF_NAME = "openbor_set_palette_property(void drawmethod, int color_index, int color_component, int value)";
	const int ARG_OBJECT = 0;
	const int ARG_COLOR_INDEX = 1;
	const int ARG_COLOR_COMPONENT = 2;
	const int ARG_VALUE = 3;
	const int ARG_MINIMUM = 4;

	/*
	* Verify parameters.
	*/
	if (varlist[ARG_OBJECT]->vt != VT_PTR
		|| varlist[ARG_COLOR_INDEX]->vt != VT_INTEGER
		|| varlist[ARG_COLOR_COMPONENT]->vt != VT_INTEGER
		|| paramCount < ARG_MINIMUM) {
		printf("\n\n Script error: %s. You must provide a valid object pointer, color index, color component, and new value.\n\n", SELF_NAME);
		*pretvar = NULL;
		return E_FAIL;
	}

	/*
	* Verify we don't have a NULL pointer. 
	* Our target object is an array, not 
	* a structure, so there's no really
	* good way to verify type.
	*/

	unsigned char* acting_object = (unsigned char*)varlist[ARG_OBJECT]->ptrVal;

	if (!acting_object) {
		printf("\n\nScript error: %s. Object pointer is not correct type.\n\n", SELF_NAME);
		*pretvar = NULL;
		return E_FAIL;
	}

	const unsigned char color_index_param = (unsigned char)varlist[ARG_COLOR_INDEX]->lVal;
	const e_color_components color_component_param = (e_color_components)varlist[ARG_COLOR_COMPONENT]->lVal;
	
	
	const unsigned int component_id = COLOR_COMPONENT_END * color_index_param + color_component_param;
	
	acting_object[component_id] = (unsigned int)varlist[ARG_VALUE]->lVal;

	return S_OK;
}