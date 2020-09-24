/* vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * Copyright 2019 MariaDB Corporation Ab. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
 */

#pragma once

#include "config.h"
#include <stdint.h>
#include <stddef.h>

// Maxmum S3 file size is 1024 bytes so for protection we make the maximum
// URI length this
#define MAX_URI_LENGTH 1024

#define READ_BUFFER_DEFAULT_SIZE 1024*1024

enum uri_method_t
{
  MS3_GET,
  MS3_HEAD,
  MS3_PUT,
  MS3_DELETE
};

typedef enum uri_method_t uri_method_t;

enum command_t
{
  MS3_CMD_LIST,
  MS3_CMD_LIST_RECURSIVE,
  MS3_CMD_PUT,
  MS3_CMD_GET,
  MS3_CMD_DELETE,
  MS3_CMD_HEAD,
  MS3_CMD_COPY,
  MS3_CMD_LIST_ROLE,
  MS3_CMD_ASSUME_ROLE
};

typedef enum command_t command_t;

struct ms3_st;

uint8_t execute_request(ms3_st *ms3, command_t command, const char *bucket,
                        const char *object, const char *source_bucket, const char *source_object,
                        const char *filter, const uint8_t *data, size_t data_size,
                        char *continuation,
                        void *ret_ptr);
