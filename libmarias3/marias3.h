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

#ifdef __cplusplus
extern "C" {
#endif

#include <curl/curl.h>
#include <stdint.h>

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

typedef void *(*ms3_malloc_callback)(size_t size);
typedef void (*ms3_free_callback)(void *ptr);
typedef void *(*ms3_realloc_callback)(void *ptr, size_t size);
typedef char *(*ms3_strdup_callback)(const char *str);
typedef void *(*ms3_calloc_callback)(size_t nmemb, size_t size);

enum ms3_error_code_t
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
  MS3_ERR_AUTH_ROLE,
  MS3_ERR_MAX // Always the last error
};

typedef enum ms3_error_code_t ms3_error_code_t;

enum ms3_set_option_t
{
  MS3_OPT_USE_HTTP,
  MS3_OPT_DISABLE_SSL_VERIFY,
  MS3_OPT_BUFFER_CHUNK_SIZE,
  MS3_OPT_FORCE_LIST_VERSION,
  MS3_OPT_FORCE_PROTOCOL_VERSION,
  MS3_OPT_PORT_NUMBER
};

typedef enum ms3_set_option_t ms3_set_option_t;

MS3_API
void ms3_library_init(void);

MS3_API
void ms3_library_deinit(void);

MS3_API
uint8_t ms3_library_init_malloc(ms3_malloc_callback m,
                                ms3_free_callback f, ms3_realloc_callback r,
                                ms3_strdup_callback s, ms3_calloc_callback c);

MS3_API
ms3_st *ms3_init(const char *s3key, const char *s3secret,
                 const char *region,
                 const char *base_domain);

MS3_API
uint8_t ms3_init_assume_role(ms3_st *ms3, const char *iam_role, const char *sts_endpoint, const char *sts_region);

MS3_API
uint8_t ms3_ec2_set_cred(ms3_st *ms3, const char *iam_role,
                     const char *s3key, const char *s3secret,
                     const char *token);

MS3_API
uint8_t ms3_set_option(ms3_st *ms3, ms3_set_option_t option, void *value);

MS3_API
void ms3_deinit(ms3_st *ms3);

MS3_API
const char *ms3_server_error(ms3_st *ms3);

MS3_API
const char *ms3_error(uint8_t errcode);

MS3_API
void ms3_debug(void);

MS3_API
uint8_t ms3_list(ms3_st *ms3, const char *bucket, const char *prefix,
                 ms3_list_st **list);

MS3_API
uint8_t ms3_list_dir(ms3_st *ms3, const char *bucket, const char *prefix,
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
uint8_t ms3_copy(ms3_st *ms3, const char *source_bucket, const char *source_key,
                 const char *dest_bucket, const char *dest_key);

MS3_API
uint8_t ms3_move(ms3_st *ms3, const char *source_bucket, const char *source_key,
                 const char *dest_bucket, const char *dest_key);

MS3_API
void ms3_free(uint8_t *data);

MS3_API
uint8_t ms3_delete(ms3_st *ms3, const char *bucket, const char *key);

MS3_API
uint8_t ms3_status(ms3_st *ms3, const char *bucket, const char *key,
                   ms3_status_st *status);

MS3_API
uint8_t ms3_assume_role(ms3_st *ms3);

#ifdef __cplusplus
}
#endif
