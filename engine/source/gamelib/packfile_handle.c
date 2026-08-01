/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) OpenBOR Team
 */

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "packfile_handle.h"

#define PACKFILE_HANDLE_INITIAL_CAPACITY 8U

/*
* Caskey, Damon V.
* 2026-07-31
*
* Grow the packfile handle table. Newly 
* allocated handles are pushed onto
* the free list in reverse order so that
* the lowest-numbered handle is returned first.
*
* Returns true on success, false on failure.
*/
static bool packfile_handle_grow(s_packfile_handle_table *table) {
    size_t handle_index;
    size_t old_capacity;
    size_t new_capacity;
    s_packfile_handle *new_handle;

    old_capacity = table->capacity;
    if(old_capacity >= (size_t)INT_MAX) {
        return false;
    }

    if(old_capacity == 0) {
        new_capacity = PACKFILE_HANDLE_INITIAL_CAPACITY;
    
    } else if(old_capacity > (size_t)INT_MAX / 2U) {
        new_capacity = (size_t)INT_MAX;
    
    } else {
        new_capacity = old_capacity * 2U;
    }

    if(new_capacity > SIZE_MAX / sizeof(*new_handle)) {
        return false;
    }

    new_handle = realloc(table->handle, new_capacity * sizeof(*new_handle));
    if(!new_handle) {
        return false;
    }

    table->handle = new_handle;
    table->capacity = new_capacity;

    /* 
    * Push in reverse order so newly allocated 
    * handles are returned low first. 
    */
    
    for(handle_index = new_capacity; handle_index > old_capacity;) {
        handle_index--;
        memset(&table->handle[handle_index], 0, sizeof(table->handle[handle_index]));
        table->handle[handle_index].type = PACKFILE_HANDLE_FREE;
        table->handle[handle_index].file_descriptor = -1;
        table->handle[handle_index].next_free = table->first_free;
        table->first_free = (int)handle_index;
    }

    return true;
}

/*
* Caskey, Damon V.
* 2026-07-31
*
* Initialize the packfile handle table.
*/
void packfile_handle_table_init(s_packfile_handle_table *table) {

    if(!table) {
        return;
    }

    table->handle = NULL;
    table->capacity = 0;
    table->first_free = -1;
}

/*
* Caskey, Damon V.
* 2026-07-31
*
* Destroy the packfile handle table.
*/
void packfile_handle_table_destroy(s_packfile_handle_table *table){

    if(!table) {
        return;
    }

    free(table->handle);
    packfile_handle_table_init(table);
}

/*
* Caskey, Damon V.
* 2026-07-31
*
* Acquire a packfile handle.
*/
int packfile_handle_acquire(s_packfile_handle_table *table, e_packfile_handle_type type) {
    
    int handle;
    s_packfile_handle *record;

    if(!table || type == PACKFILE_HANDLE_FREE) {
        return -1;
    }

    if(table->first_free < 0 && !packfile_handle_grow(table)) {
        return -1;
    }

    handle = table->first_free;
    record = &table->handle[handle];
    table->first_free = record->next_free;

    memset(record, 0, sizeof(*record));
    record->type = type;
    record->file_descriptor = -1;
    record->next_free = -1;
    return handle;
}

/*
* Caskey, Damon V.
* 2026-07-31
*
* Release a packfile handle.
*/
void packfile_handle_release(s_packfile_handle_table *table, int handle) {

    s_packfile_handle *record;

    record = packfile_handle_get(table, handle);
    
    if(!record) {
        return;
    }

    memset(record, 0, sizeof(*record));
    record->type = PACKFILE_HANDLE_FREE;
    record->file_descriptor = -1;
    record->next_free = table->first_free;
    table->first_free = handle;
}

/*
* Caskey, Damon V.
* 2026-07-31
*
* Get a packfile handle.
*/
s_packfile_handle *packfile_handle_get(s_packfile_handle_table *table, int handle) {

    if(!table || handle < 0 || (size_t)handle >= table->capacity) {
        return NULL;
    }

    if(table->handle[handle].type == PACKFILE_HANDLE_FREE) {
        return NULL;
    }

    return &table->handle[handle];
}

/*
* Caskey, Damon V.
* 2026-07-31
*
* Get a packfile handle of a specific type.
*/
s_packfile_handle *packfile_handle_get_type(s_packfile_handle_table *table, int handle, e_packfile_handle_type type) {
    s_packfile_handle *record;

    record = packfile_handle_get(table, handle);

    if(!record || record->type != type) {
        return NULL;
    }

    return record;
}
