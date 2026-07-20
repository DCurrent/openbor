/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) OpenBOR Team
 */

#ifndef OPENBOR_SCRIPT_COMMAND_INPUT_H
#define OPENBOR_SCRIPT_COMMAND_INPUT_H

/*
* Caskey, Damon V.
* 2026-07-19
*
* Script-visible properties for one entry in an
* s_command_input_event collection.
*/
typedef enum e_command_input_event_properties
{
    COMMAND_INPUT_EVENT_PROPERTY_HELD,
    COMMAND_INPUT_EVENT_PROPERTY_HOLD,
    COMMAND_INPUT_EVENT_PROPERTY_PRESS,
    COMMAND_INPUT_EVENT_PROPERTY_PRESS_CHORD,
    COMMAND_INPUT_EVENT_PROPERTY_RELEASE,
    COMMAND_INPUT_EVENT_PROPERTY_TICKS,
    COMMAND_INPUT_EVENT_PROPERTY_TIME,
    COMMAND_INPUT_EVENT_PROPERTY_END
} e_command_input_event_properties;

/*
* Caskey, Damon V.
* 2026-07-19
*
* Script-visible properties for one entry in an
* s_command_input_step collection.
*/
typedef enum e_command_input_step_properties
{
    COMMAND_INPUT_STEP_PROPERTY_CHORD_TIME,
    COMMAND_INPUT_STEP_PROPERTY_HOLD,
    COMMAND_INPUT_STEP_PROPERTY_HOLD_TIME,
    COMMAND_INPUT_STEP_PROPERTY_HOLD_TIME_MAXIMUM,
    COMMAND_INPUT_STEP_PROPERTY_HOLD_TRIGGER,
    COMMAND_INPUT_STEP_PROPERTY_PRESS,
    COMMAND_INPUT_STEP_PROPERTY_RELEASE,
    COMMAND_INPUT_STEP_PROPERTY_END
} e_command_input_step_properties;

HRESULT openbor_get_command_input_event_object(ScriptVariant** varlist, ScriptVariant** pretvar, const int paramCount);
HRESULT openbor_get_command_input_event_property(ScriptVariant** varlist, ScriptVariant** pretvar, const int paramCount);
HRESULT openbor_set_command_input_event_property(ScriptVariant** varlist, ScriptVariant** pretvar, const int paramCount);

HRESULT openbor_get_command_input_step_object(ScriptVariant** varlist, ScriptVariant** pretvar, const int paramCount);
HRESULT openbor_get_command_input_step_property(ScriptVariant** varlist, ScriptVariant** pretvar, const int paramCount);
HRESULT openbor_set_command_input_step_property(ScriptVariant** varlist, ScriptVariant** pretvar, const int paramCount);

#endif
