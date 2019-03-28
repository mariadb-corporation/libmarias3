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

#include <curl/curl.h>
#include <stdint.h>
#include <stdbool.h>

#include <libmarias3/visibility.h>

struct ms3_st;
typedef struct ms3_st ms3_st;

struct ms3_list_st
{
  char *key;
  size_t length;
  time_t created;
  struct ms3_list_st *next;
};

typedef struct ms3_list_st ms3_list_st;

struct ms3_status_st
{
  size_t length;
  time_t created;
};

typedef struct ms3_status_st ms3_status_st;

enum MS3_ERROR_CODES
{
  MS3_ERR_NONE,
  MS3_ERR_PARAMETER,
  MS3_ERR_NO_DATA,
  MS3_ERR_URI_TOO_LONG,
  MS3_ERR_RESPONSE_PARSE,
  MS3_ERR_REQUEST_ERROR,
  MS3_ERR_OOM,
  MS3_ERR_IMPOSSIBLE,
  MS3_ERR_AUTH,
  MS3_ERR_NOT_FOUND,
  MS3_ERR_SERVER,
  MS3_ERR_TOO_BIG,
  MS3_ERR_MAX // Always the last error
};

MS3_API
void ms3_library_init(void);

MS3_API
ms3_st *ms3_thread_init(const char *s3key, const char *s3secret,
                        const char *region,
                        const char *base_domain);

MS3_API
uint8_t ms3_buffer_chunk_size(ms3_st *ms3, size_t new_size);

MS3_API
void ms3_deinit(ms3_st *ms3);

MS3_API
const char *ms3_server_error(ms3_st *ms3);

MS3_API
const char *ms3_error(uint8_t errcode);

MS3_API
void ms3_debug(bool state);

MS3_API
uint8_t ms3_list(ms3_st *ms3, const char *bucket, const char *prefix,
                 ms3_list_st **list);

MS3_API
void ms3_list_free(ms3_list_st *list);

MS3_API
uint8_t ms3_put(ms3_st *ms3, const char *bucket, const char *key,
                const uint8_t *data, size_t length);

MS3_API
uint8_t ms3_get(ms3_st *ms3, const char *bucket, const char *key,
                uint8_t **data, size_t *length);

MS3_API
void ms3_free(uint8_t *data);

MS3_API
uint8_t ms3_delete(ms3_st *ms3, const char *bucket, const char *key);

MS3_API
uint8_t ms3_status(ms3_st *ms3, const char *bucket, const char *key,
                   ms3_status_st *status);
