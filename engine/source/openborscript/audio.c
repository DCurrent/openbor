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

const s_property_access_map music_channel_get_property_map(const void* acting_object_param, const unsigned int property_index_param)
{
	s_property_access_map property_map;
	const musicchannelstruct* acting_object = acting_object_param;
	const e_music_channel_properties property_index = property_index_param;

	switch (property_index)
	{
	case MUSIC_CHANNEL_PROPERTY_ACTIVE:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->active;
		property_map.id_string = "MUSIC_CHANNEL_PROPERTY_ACTIVE";
		property_map.type = VT_INTEGER;
		break;

	case MUSIC_CHANNEL_PROPERTY_BUFFER_LIST:
		property_map.config_flags = (PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER);
		property_map.field = &acting_object->buf;
		property_map.id_string = "MUSIC_CHANNEL_PROPERTY_BUFFER_LIST";
		property_map.type = VT_PTR;
		break;

	case MUSIC_CHANNEL_PROPERTY_CHANNELS:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->channels;
		property_map.id_string = "MUSIC_CHANNEL_PROPERTY_CHANNELS";
		property_map.type = VT_INTEGER;
		break;

	case MUSIC_CHANNEL_PROPERTY_PAUSED:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->paused;
		property_map.id_string = "MUSIC_CHANNEL_PROPERTY_PAUSED";
		property_map.type = VT_INTEGER;
		break;

	case MUSIC_CHANNEL_PROPERTY_PERIOUD:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->fp_period;
		property_map.id_string = "MUSIC_CHANNEL_PROPERTY_PERIOUD";
		property_map.type = VT_INTEGER;
		break;

	case MUSIC_CHANNEL_PROPERTY_PLAY_BUFFER:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->playing_buffer;
		property_map.id_string = "MUSIC_CHANNEL_PROPERTY_PLAY_BUFFER";
		property_map.type = VT_INTEGER;
		break;

	case MUSIC_CHANNEL_PROPERTY_PLAY_TO:
		property_map.config_flags = (PROPERTY_ACCESS_CONFIG_READ | PROPERTY_ACCESS_CONFIG_STATIC_POINTER);
		property_map.field = &acting_object->fp_playto;
		property_map.id_string = "MUSIC_CHANNEL_PROPERTY_PLAY_TO";
		property_map.type = VT_PTR;
		break;

	case MUSIC_CHANNEL_PROPERTY_SAMPLE_POSITION:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->fp_samplepos;
		property_map.id_string = "MUSIC_CHANNEL_PROPERTY_SAMPLE_POSITION";
		property_map.type = VT_INTEGER;
		break;

	case MUSIC_CHANNEL_PROPERTY_VOLUME_LEFT:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->volume[SOUND_SPATIAL_CHANNEL_LEFT];
		property_map.id_string = "MUSIC_CHANNEL_PROPERTY_VOLUME_LEFT";
		property_map.type = VT_INTEGER;
		break;

	case MUSIC_CHANNEL_PROPERTY_VOLUME_RIGHT:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_MACRO_DEFAULT;
		property_map.field = &acting_object->volume[SOUND_SPATIAL_CHANNEL_RIGHT];
		property_map.id_string = "MUSIC_CHANNEL_PROPERTY_VOLUME_RIGHT";
		property_map.type = VT_INTEGER;
		
		break;

	case MUSIC_CHANNEL_PROPERTY_END:
	default:
		property_map.config_flags = PROPERTY_ACCESS_CONFIG_NONE;
		property_map.field = NULL;
		property_map.id_string = "music_channel";
		property_map.type = VT_EMPTY;
		break;

	}
	return property_map;
}
/*
* Caskey, Damon  V.
* 2024-01-31
*
* Return a property. Requires
* a object pointer and property
* constant to access.
*/
HRESULT openbor_get_music_channel_property(const ScriptVariant* const* varlist, ScriptVariant** const pretvar, const int paramCount)
{
	const char* SELF_NAME = "get_music_channel_property(void object, int property)";
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

	const musicchannelstruct* const acting_object = (const musicchannelstruct* const)varlist[ARG_OBJECT]->ptrVal;

	if (acting_object->object_type != OBJECT_TYPE_MUSIC_CHANNEL) {
		printf("\n\nScript error: %s. Object pointer is not correct type (%d).\n\n", SELF_NAME, acting_object->object_type);
		*pretvar = NULL;
		return E_FAIL;
	}

	const int property_id_param = (const int)varlist[ARG_PROPERTY]->lVal;

	const e_music_channel_properties property_id = (e_music_channel_properties)(property_id_param);
	const s_property_access_map property_map = music_channel_get_property_map(acting_object, property_id);

	/*
	* If property id is in range, we send
	* the property map and return parameter
	* for population, then ext.
	*/

	if (property_id_param >= 0 && property_id_param < MUSIC_CHANNEL_PROPERTY_END) {
		property_access_get_member(&property_map, *pretvar);
		return S_OK;
	}

	/*
	* Is this a dump request? If not, then
	* the property id is invalid.
	*/

	if (property_id_param == PROPERTY_ACCESS_DUMP) {
		property_access_dump_members(music_channel_get_property_map, MUSIC_CHANNEL_PROPERTY_END, acting_object);
	}
	else {
		printf("\n\nScript error: %s. Unknown property id (%d). \n\n", SELF_NAME, property_id_param);
		return E_FAIL;
	}

	return S_OK;
}


/*
* Caskey, Damon  V.
* 2024-01-31
*
* Mutate a property. Requires
* the object pointer, a property
* id, and new value.
*/
HRESULT openbor_set_music_channel_property(ScriptVariant** varlist, ScriptVariant** const pretvar, const int paramCount)
{
	const char* SELF_NAME = "set_music_channel_property(void object, int property, <mixed> value)";
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

	const musicchannelstruct* const acting_object = (const musicchannelstruct* const)varlist[ARG_OBJECT]->ptrVal;

	if (acting_object->object_type != OBJECT_TYPE_MUSIC_CHANNEL) {
		printf("\n\nScript error: %s. Object pointer is not correct type (%d).\n\n", SELF_NAME, acting_object->object_type);
		*pretvar = NULL;
		return E_FAIL;
	}

	const int property_id_param = (const int)varlist[ARG_PROPERTY]->lVal;
	const e_music_channel_properties property_id = (e_music_channel_properties)(property_id_param);

	if (property_id_param < 0 && property_id_param >= MUSIC_CHANNEL_PROPERTY_END) {
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

	const s_property_access_map property_map = music_channel_get_property_map(acting_object, property_id);

	/*
	* Populate the property value on
	* acting object and return OK/FAIL.
	*/

	return property_access_set_member(acting_object, &property_map, varlist[ARG_VALUE]);

	return S_OK;
}
