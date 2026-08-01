/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) OpenBOR Team
 */

#ifndef PACKFILE_HANDLE_H
#define PACKFILE_HANDLE_H

#include <stddef.h>
#include <stdint.h>

typedef enum e_packfile_handle_type
{
    PACKFILE_HANDLE_FREE = 0,   
    PACKFILE_HANDLE_DIRECT,
    PACKFILE_HANDLE_CACHED
} e_packfile_handle_type;

typedef struct s_packfile_handle
{
    e_packfile_handle_type type;
    int next_free;
    int file_descriptor;
    uint64_t data_start;
    uint64_t size;
    uint64_t position;
    int readahead_size;
} s_packfile_handle;

typedef struct s_packfile_handle_table
{
    s_packfile_handle *handle;
    size_t capacity;
    int first_free;
} s_packfile_handle_table;

void packfile_handle_table_init(s_packfile_handle_table *table);
void packfile_handle_table_destroy(s_packfile_handle_table *table);
int packfile_handle_acquire(s_packfile_handle_table *table, e_packfile_handle_type type);
void packfile_handle_release(s_packfile_handle_table *table, int handle);
s_packfile_handle *packfile_handle_get(s_packfile_handle_table *table, int handle);
s_packfile_handle *packfile_handle_get_type(s_packfile_handle_table *table, int handle, e_packfile_handle_type type);

#endif
