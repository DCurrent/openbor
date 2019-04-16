/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 - 2017 OpenBOR Team
 */

 // Drawmethod Properties
 // 2019-03-28
 // Caskey, Damon V.

#include "scriptcommon.h"

// Use string property argument to find an
// integer property constant and populate
// varlist->lval.
int mapstrings_drawmethod(ScriptVariant **varlist, int paramCount)
{
#define ARG_MINIMUM     2   // Minimum number of arguments allowed in varlist.
#define ARG_PROPERTY    1   // Varlist element carrying which property is requested.

	char *propname = NULL;  // Placeholder for string property name from varlist.
	int prop;               // Placeholder for integer constant located by string.

	static const char *proplist[] =
	{
		"alpha",
		"background_transparency",
		"center_x",
		"center_y",
		"channel_blue",
		"channel_green",
		"channel_red",
		"clip_position_x",
		"clip_position_y",
		"clip_size_x",
		"clip_size_y",
		"colorset_index",
		"colorset_table",
		"enable",
		"fill_color",
		"flip_x",
		"flip_y",
		"repeat_x",
		"repeat_y",
		"rotate",
		"rotate_flip",
		"scale_x",
		"scale_y",
		"shift_x",
		"span_x",
		"span_y",
		"tag",
		"tint_color",
		"tint_mode",
		"water_mode",
		"water_perspective",
		"water_size_begin",
		"water_size_end",
		"water_wave_amplitude",
		"water_wave_length",
		"water_wave_speed",
		"water_wave_time"
	};

	// If the minimum argument count
	// was not passed, then there is
	// nothing to map. Return true - we'll
	// catch the mistake in property access
	// functions.
	if (paramCount < ARG_MINIMUM)
	{
		return 1;
	}

	// See macro - will return 0 on fail.
	MAPSTRINGS(varlist[ARG_PROPERTY], proplist, _DRAWMETHOD_END,
		"Property name '%s' is not supported by drawmethod.\n");


	// If we made it this far everything should be OK.
	return 1;

#undef ARG_MINIMUM
#undef ARG_PROPERTY
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
#define SELF_NAME       "openbor_copy_drawmethod(void source, void target)"
#define ARG_MINIMUM     2   // Minimum required arguments.
#define ARG_SOURCE		0
#define ARG_TARGET		1

	s_drawmethod *source;
	s_drawmethod *target;

	// Verify arguments. 
	if (paramCount < ARG_MINIMUM
		|| varlist[ARG_SOURCE]->vt != VT_PTR
		|| varlist[ARG_TARGET]->vt != VT_PTR)
	{
		*pretvar = NULL;
		goto error_local;
	}
	else
	{
		// Populate local vars with arguments.
		source = (s_drawmethod *)varlist[ARG_SOURCE]->ptrVal;
		target = (s_drawmethod *)varlist[ARG_TARGET]->ptrVal;
	}

	// Default drawmethod (plainmethod) is a const and therefore cannot
	// be mutated. If the author tries it's sure to cause a crash or
	// even worse, untracable bugs. We'll send a warning to the log and 
	// exit function. Note it's perfectly fine to use the default 
	// drawmethod as a source, and that's probably what will be done 
	// most of the time anyway.
	if (target == &plainmethod)
	{
		printf("\n Warning: The default drawmethod and its properties are read only: " SELF_NAME "\n");

		return S_OK;
	}

	// Copy values into target drawmethod.
	memcpy(target, source, sizeof(*target));

	return S_OK;

error_local:

	printf("\nYou must provide valid source and target drawmethod pointers: " SELF_NAME "\n");
	*pretvar = NULL;

	return E_FAIL;

#undef SELF_NAME
#undef ARG_MINIMUM
#undef ARG_SOURCE
#undef ARG_TARGET
}

// Caskey, Damon  V.
// 2019-04-16
//
// Allocate a new drawmethod and return the pointer.
HRESULT openbor_free_drawmethod(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
#define SELF_NAME       "openbor_free_drawmethod(void drawmethod)"
#define ARG_MINIMUM     1   // Minimum required arguments.
#define ARG_TARGET		0

	s_drawmethod *target;

	// Verify arguments.
	if (paramCount < ARG_MINIMUM
		|| varlist[ARG_TARGET]->vt != VT_PTR)
	{
		*pretvar = NULL;
		goto error_local;
	}
	else
	{
		// Populate local vars with arguments.
		target = (s_drawmethod *)varlist[ARG_TARGET]->ptrVal;
	}

	// Default drawmethod (plainmethod) is a const and therefore cannot
	// be mutated. If the author tries it's sure to cause a crash or
	// even worse, untracable bugs. We'll send a warning to the log and 
	// exit function.
	if (target == &plainmethod)
	{
		printf("\n Warning: The default drawmethod and its properties are read only: " SELF_NAME "\n");

		return S_OK;
	}

	// Free the drawmethod.
	free(target);

	return S_OK;

error_local:

	printf("\n You must provide a valid drawmethod pointer: " SELF_NAME "\n");
	*pretvar = NULL;

	return E_FAIL;

#undef SELF_NAME
#undef ARG_MINIMUM
#undef ARG_TARGET
}

// Caskey, Damon  V.
// 2019-03-28
//
// Return a drawmethod property. Requires
// the pointer from drawmethod property
// and a property to access.
HRESULT openbor_get_drawmethod_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
#define SELF_NAME       "openbor_get_drawmethod_property(void drawmethod, char property)"
#define ARG_MINIMUM     2   // Minimum required arguments.
#define ARG_HANDLE      0   // Handle (pointer to property structure).
#define ARG_PROPERTY    1   // Property to access.

	s_drawmethod				*handle = NULL;		// Property handle.
	e_drawmethod_properties		property = 0;		// Property argument.

	// Clear pass by reference argument used to send
	// property data back to calling script.     .
	ScriptVariant_Clear(*pretvar);

	// Verify arguments. There should at least
	// be a pointer for the property handle and an integer
	// to determine which property constant is accessed.
	if (paramCount < ARG_MINIMUM
		|| varlist[ARG_HANDLE]->vt != VT_PTR
		|| varlist[ARG_PROPERTY]->vt != VT_INTEGER)
	{
		*pretvar = NULL;
		goto error_local;
	}
	else
	{
		// Populate local vars for readability.
		handle = (s_drawmethod *)varlist[ARG_HANDLE]->ptrVal;
		property = (LONG)varlist[ARG_PROPERTY]->lVal;
	}

	switch (property)
	{		
	case _DRAWMETHOD_ALPHA:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->alpha;

		break;

	case _DRAWMETHOD_BACKGROUND_TRANSPARENCY:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->transbg;

		break;

	case _DRAWMETHOD_CENTER_X:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->centerx;

		break;

	case _DRAWMETHOD_CENTER_Y:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->centery;

		break;

	case _DRAWMETHOD_CHANNEL_BLUE:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->channelb;

		break;

	case _DRAWMETHOD_CHANNEL_GREEN:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->channelg;

		break;

	case _DRAWMETHOD_CHANNEL_RED:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->channelr;

		break;

	case _DRAWMETHOD_CLIP_POSITION_X:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->clipx;

		break;

	case _DRAWMETHOD_CLIP_POSITION_Y:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->clipy;

		break;

	case _DRAWMETHOD_CLIP_SIZE_X:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->clipw;

		break;

	case _DRAWMETHOD_CLIP_SIZE_Y:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->cliph;

		break;

	case _DRAWMETHOD_COLORSET_INDEX:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->remap;

		break;

	case _DRAWMETHOD_COLORSET_TABLE:

		ScriptVariant_ChangeType(*pretvar, VT_PTR);
		(*pretvar)->ptrVal = (VOID *)(handle->table);

		break;

	case _DRAWMETHOD_ENABLE:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->flag;

		break;
	
	case _DRAWMETHOD_FILL_COLOR:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->fillcolor;

		break;

	case _DRAWMETHOD_FLIP_X:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->flipx;

		break;

	case _DRAWMETHOD_FLIP_Y:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->flipy;

		break;

	case _DRAWMETHOD_REPEAT_X:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->xrepeat;

		break;

	case _DRAWMETHOD_REPEAT_Y:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->yrepeat;

		break;

	case _DRAWMETHOD_ROTATE:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->rotate;

		break;

	case _DRAWMETHOD_ROTATE_FLIP:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->fliprotate;

		break;

	case _DRAWMETHOD_SCALE_X:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->scalex;

		break;

	case _DRAWMETHOD_SCALE_Y:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->scaley;

		break;

	case _DRAWMETHOD_SHIFT_X:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->shiftx;

		break;

	case _DRAWMETHOD_SPAN_X:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->xspan;

		break;

	case _DRAWMETHOD_SPAN_Y:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->yspan;

		break;

	case _DRAWMETHOD_TAG:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->tag;

		break;

	case _DRAWMETHOD_TINT_COLOR:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->tintcolor;

		break;

	case _DRAWMETHOD_TINT_MODE:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->tintmode;

		break;

	case _DRAWMETHOD_WATER_MODE:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->water.watermode;

		break;

	case _DRAWMETHOD_WATER_PERSPECTIVE:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->water.perspective;

		break;

	case _DRAWMETHOD_WATER_SIZE_BEGIN:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->water.beginsize;

		break;

	case _DRAWMETHOD_WATER_SIZE_END:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->water.endsize;

		break;

	case _DRAWMETHOD_WATER_WAVE_AMPLITUDE:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->water.amplitude;

		break;

	case _DRAWMETHOD_WATER_WAVE_LENGTH:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->water.wavelength;

		break;

	case _DRAWMETHOD_WATER_WAVE_SPEED:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->water.wavespeed;

		break;

	case _DRAWMETHOD_WATER_WAVE_TIME:

		ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)handle->water.wavetime;

		break;

	default:

		printf("Unsupported property.\n");
		goto error_local;

		break;
	}

	return S_OK;

error_local:

	printf("\nYou must provide a valid pointer and property name: " SELF_NAME "\n");
	*pretvar = NULL;

	return E_FAIL;

#undef SELF_NAME
#undef ARG_MINIMUM
#undef ARG_HANDLE
#undef ARG_INDEX
}

// Caskey, Damon  V.
// 2019-03-28
//
// Mutate a drawmethod property. Requires
// the pointer from drawmethod property
// and a property to access.
HRESULT openbor_set_drawmethod_property(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount)
{
#define SELF_NAME			"set_drawmethod_property(void drawmethod, char property, mixed value)"
#define ARG_MINIMUM         3   // Minimum required arguments.
#define ARG_HANDLE          0   // Handle (pointer to property structure).
#define ARG_PROPERTY        1   // Property to access.
#define ARG_VALUE           2   // New value to apply.

	s_drawmethod			*handle = NULL;	// Property handle.
	e_drawmethod_properties	property = 0;	// Property to access.

	// Value carriers to apply on properties after
	// taken from argument.
	LONG         temp_int;

	// Verify incoming arguments. There should at least
	// be a pointer for the property handle and an integer
	// to determine which property is accessed.
	if (paramCount < ARG_MINIMUM
		|| varlist[ARG_HANDLE]->vt != VT_PTR
		|| varlist[ARG_PROPERTY]->vt != VT_INTEGER)
	{
		*pretvar = NULL;
		goto error_local;
	}

	// Populate local handle and property vars.
	handle = (s_drawmethod *)varlist[ARG_HANDLE]->ptrVal;
	property = (LONG)varlist[ARG_PROPERTY]->lVal;

	// Default drawmethod (plainmethod) is a const and therefore cannot
	// be mutated. If the author tries it's sure to cause a crash or
	// even worse, untracable bugs. We'll send a warning to the log and 
	// exit function.
	if (handle == &plainmethod)
	{
		printf("\n Warning: The default drawmethod and its properties are read only: " SELF_NAME "\n");
		
		return S_OK;
	}		

	// Which property to modify?
	switch (property)
	{
	
	case _DRAWMETHOD_ALPHA:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->alpha = temp_int;
		}

		break;

	case _DRAWMETHOD_BACKGROUND_TRANSPARENCY:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->transbg = temp_int;
		}

		break;

	case _DRAWMETHOD_CENTER_X:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->centerx = temp_int;
		}

		break;

	case _DRAWMETHOD_CENTER_Y:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->centery = temp_int;
		}

		break;

	case _DRAWMETHOD_CHANNEL_BLUE:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->channelb = temp_int;
		}

		break;

	case _DRAWMETHOD_CHANNEL_GREEN:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->channelg = temp_int;
		}

		break;

	case _DRAWMETHOD_CHANNEL_RED:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->channelr = temp_int;
		}

		break;

	case _DRAWMETHOD_CLIP_POSITION_X:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->clipx = temp_int;
		}

		break;

	case _DRAWMETHOD_CLIP_POSITION_Y:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->clipy = temp_int;
		}

		break;

	case _DRAWMETHOD_CLIP_SIZE_X:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->clipw = temp_int;
		}

		break;

	case _DRAWMETHOD_CLIP_SIZE_Y:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->cliph = temp_int;
		}

		break;

	case _DRAWMETHOD_COLORSET_INDEX:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->remap = temp_int;
		}

		break;

	case _DRAWMETHOD_COLORSET_TABLE:

		handle->table = (VOID *)varlist[ARG_VALUE]->ptrVal;

		break;

	case _DRAWMETHOD_ENABLE:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->flag = temp_int;
		}

		break;

	case _DRAWMETHOD_FILL_COLOR:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->fillcolor = temp_int;
		}

		break;

	case _DRAWMETHOD_FLIP_X:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->flipx = temp_int;
		}

		break;

	case _DRAWMETHOD_FLIP_Y:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->flipy = temp_int;
		}

		break;

	case _DRAWMETHOD_REPEAT_X:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->xrepeat = temp_int;
		}

		break;

	case _DRAWMETHOD_REPEAT_Y:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->yrepeat = temp_int;
		}

		break;

	case _DRAWMETHOD_ROTATE:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->rotate = temp_int;
		}

		break;

	case _DRAWMETHOD_ROTATE_FLIP:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->fliprotate = temp_int;
		}

		break;

	case _DRAWMETHOD_SCALE_X:
		
		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->scalex = temp_int;
		}

		break;

	case _DRAWMETHOD_SCALE_Y:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->scaley = temp_int;
		}

		break;

	case _DRAWMETHOD_SHIFT_X:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->shiftx = temp_int;
		}

		break;

	case _DRAWMETHOD_SPAN_X:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->xspan = temp_int;
		}

		break;

	case _DRAWMETHOD_SPAN_Y:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->yspan = temp_int;
		}

		break;

	case _DRAWMETHOD_TAG:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->tag = temp_int;
		}

		break;

	case _DRAWMETHOD_TINT_COLOR:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->tintcolor = temp_int;
		}

		break;

	case _DRAWMETHOD_TINT_MODE:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->tintmode = temp_int;
		}

		break;

	case _DRAWMETHOD_WATER_MODE:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->water.watermode = temp_int;
		}

		break;

	case _DRAWMETHOD_WATER_PERSPECTIVE:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->water.perspective = temp_int;
		}

	case _DRAWMETHOD_WATER_SIZE_BEGIN:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->water.beginsize = temp_int;
		}

	case _DRAWMETHOD_WATER_SIZE_END:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->water.endsize = temp_int;
		}

		break;

	case _DRAWMETHOD_WATER_WAVE_AMPLITUDE:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->water.amplitude = temp_int;
		}

		break;

	case _DRAWMETHOD_WATER_WAVE_LENGTH:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->water.wavelength = temp_int;
		}

		break;

	case _DRAWMETHOD_WATER_WAVE_SPEED:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->water.wavespeed = temp_int;
		}

		break;

	case _DRAWMETHOD_WATER_WAVE_TIME:

		if (SUCCEEDED(ScriptVariant_IntegerValue(varlist[ARG_VALUE], &temp_int)))
		{
			handle->water.wavetime = temp_int;
		}

		break;

	default:

		printf("Unsupported property.\n");
		goto error_local;

		break;
	}

	return S_OK;

	// Error trapping.
error_local:

	printf("\nYou must provide a valid pointer, property name, and new value: " SELF_NAME "\n");
	
	return E_FAIL;

#undef SELF_NAME
#undef ARG_MINIMUM
#undef ARG_HANDLE
#undef ARG_PROPERTY
#undef ARG_VALUE
}