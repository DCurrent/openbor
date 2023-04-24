#include "scriptcommon.h"

/*
* Caskey, Damon V.
* 2023-04-17
*
* Accepts property map, the number of
* items in property list, and an acting 
* object or NULL. Dumps all available 
* property meta info to log. If acting 
* object supplied, dump includes property 
* values.
*/
void property_access_dump_members(property_access_map get_property_map, const int member_count, const void* acting_object)
{
    int i;
    size_t id_string_length;
    size_t id_string_length_max = 10;
    char fixed_string[MAX_NAME_LEN] = "";
    char access_type_text[12] = "";

    s_property_access_map property_map;

    /*
    * We want to output a neat table. The
    * other members of property map are
    * fairly uniform, but property ids
    * can have wildly varying lengths.
    * 
    * Let's find the longest here to use
    * for the property ID column width.
    */
    for (i = 0; i < member_count; i++)
    {
        property_map = get_property_map(acting_object, i);

        if (property_map.config_flags)
        {
            id_string_length = strlen(property_map.id_string);

            id_string_length_max = id_string_length > id_string_length_max ? id_string_length : id_string_length_max;
        }
    }

    /*
    * Title, and if there's an object
    * supplied, include its pointer 
    * as well.
    */

    printf("\n\n Property List:");

    if (acting_object)
    {
        printf(" %p", acting_object);
    }

    /* Header row. */
    printf("\n\t | %-5s | %-*s | %-10s | %-12s | %s ", "Index", (int)id_string_length_max, "ID", "Type", "Access", "Value");

    /* 
    * Print out property map info as a the
    * row. If we have a supplied object then
    * include the property value as an exta
    * column
    */

    for (i = 0; i < member_count; i++)
    {
        property_map = get_property_map(acting_object, i);

        access_type_text[0] = '\0';
        
        if (property_map.config_flags)
        {
            if (property_map.config_flags & PROPERTY_ACCESS_CONFIG_READ)
            {
                strcat(access_type_text, "Read");
            }

            if (property_map.config_flags & PROPERTY_ACCESS_CONFIG_WRITE)
            {
                strcat(access_type_text, ", Write");
            }

            printf("\n\t | %-5d | %-*s | %10s | %-12s", i, (int)id_string_length_max, property_map.id_string, script_variant_meta_list[property_map.type].id_string, access_type_text);

            if (acting_object)
            { 
                switch (property_map.type)
                {
                case VT_EMPTY:

                    printf(" | %s", "NULL (Undefined)");
                    break;

                case VT_INTEGER:

                    printf(" | %d", *(LONG*)property_map.field);
                    break;

                case VT_PTR:
                    
                    if (property_map.config_flags & PROPERTY_ACCESS_CONFIG_STATIC_POINTER)
                    {                        
                        printf(" | %p", property_map.field);
                    }
                    else
                    {
                        printf(" | %p", *(void**)property_map.field);
                    }
                    
                    break;

                case VT_DECIMAL:
                    
                    printf(" | %f", *(DOUBLE*)property_map.field);
                    break;
                case VT_STR:

                    if (property_map.config_flags & PROPERTY_ACCESS_CONFIG_STATIC_LENGTH) {
                        memcpy(fixed_string, *(char(*)[MAX_NAME_LEN])property_map.field, MAX_NAME_LEN);
                        printf(" | %s", fixed_string);
                    }
                    else {
                        printf(" | %s", *(char**)property_map.field);
                    }

                    break;
                }
            }
            else
            {
                printf(" | %s", "NA");
            }
        }
    }

    printf("\n\n");
}

/*
* Caskey, Damon V.
* 2023-04-17
* 
* Accept a completed property map pointer
* and return varible parameter.
* 
* Populates return var from the property 
* map field's value and property type.
*/
HRESULT property_access_get_member(const s_property_access_map* property_map, ScriptVariant* pretvar)
{
    //printf("\n property_access_get_member() \n");

    char fixed_string[MAX_NAME_LEN] = { 0 };

    if (property_map == NULL || property_map->field == NULL) {
        printf("\n\n Error: Null pointer passed to a property access get function. \n");
        return E_FAIL;
    }

    //printf("\n property_map->type: %d", property_map->type);

    ScriptVariant_ChangeType(pretvar, property_map->type);


    switch (property_map->type)
    {
    case VT_INTEGER:

        //printf("\n\t VT_INTEGER");
        pretvar->lVal = *(LONG*)property_map->field;        
        break;
    case VT_PTR:

        //printf("\n\t VT_PTR");

        if (property_map->config_flags & PROPERTY_ACCESS_CONFIG_STATIC_POINTER)
        {
            pretvar->ptrVal = (void*)property_map->field;
        }
        else
        {
            pretvar->ptrVal = *(void**)property_map->field;
        }
        break;
    case VT_DECIMAL:

        //printf("\n\t VT_DECIMAL");
        pretvar->dblVal = *(DOUBLE*)property_map->field;
        break;
    case VT_STR:        

        //printf("\n\t VT_STR");

        if (property_map->config_flags & PROPERTY_ACCESS_CONFIG_STATIC_LENGTH) {            
            memcpy(fixed_string, *(char(*)[MAX_NAME_LEN])property_map->field, MAX_NAME_LEN);
            pretvar->strVal = StrCache_Pop(MAX_NAME_LEN);
            memcpy(StrCache_Get(pretvar->strVal), fixed_string, MAX_NAME_LEN);
        }
        else {
            pretvar->strVal = StrCache_CreateNewFrom(*(char**)property_map->field);
        }

        break;
    case VT_EMPTY:

        //printf("\n\t VT_EMPTY");
        break;
    }

    //printf("\n property_access_get_member OK.");

    return S_OK;
}

/*
* Caskey, Damon V.
* 2023-04-19
* 
* Accepts an object, a completed property
* map pointer, and a value argument. 
*
* Populates the property map's field value
* map from value argument and its property 
* type.
*/
HRESULT property_access_set_member(const void* const acting_object, const s_property_access_map* property_map, ScriptVariant* acting_value)
{
    LONG    temp_int;
    DOUBLE  temp_float;
    char temp_buffer[MAX_NAME_LEN];

    /*
    * If not read only, then populate the
    * property based on its variable type.
    */
    if (property_map->config_flags & PROPERTY_ACCESS_CONFIG_WRITE)
    {
        switch (property_map->type)
        {
        case VT_INTEGER:

            if (SUCCEEDED(ScriptVariant_IntegerValue(acting_value, &temp_int)))
            {
                *(LONG*)property_map->field = temp_int;
            }
            break;

        case VT_PTR:

            *(void**)property_map->field = acting_value->ptrVal;
            break;

        case VT_DECIMAL:

            if (SUCCEEDED(ScriptVariant_DecimalValue(acting_value, &temp_float)))
            {
                *(DOUBLE*)property_map->field = temp_float;
            }
            break;

        case VT_STR:
            
            if (property_map->config_flags & PROPERTY_ACCESS_CONFIG_STATIC_LENGTH) {
                
                strcpy(temp_buffer, (char*)StrCache_Get(acting_value->strVal));
                memcpy((void*)property_map->field, temp_buffer, MAX_NAME_LEN);
            }
            else{
                strcpy(*(char**)property_map->field, (char*)StrCache_Get(acting_value->strVal));
            }
            break;

        case VT_EMPTY:
            
            break;
        }
    }
    else
    {
        printf("\n\n Warning: %s is a read only pointer. Use the appropriate sub property function to modify values. \n", property_map->id_string);
    }

    return S_OK;
}