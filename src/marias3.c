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

#include "config.h"
#include "common.h"

#include <libxml/xmlmemory.h>
#include <pthread.h>

ms3_malloc_callback ms3_cmalloc = (ms3_malloc_callback)malloc;
ms3_free_callback ms3_cfree = (ms3_free_callback)free;
ms3_realloc_callback ms3_crealloc = (ms3_realloc_callback)realloc;
ms3_strdup_callback ms3_cstrdup = (ms3_strdup_callback)strdup;
ms3_calloc_callback ms3_ccalloc = (ms3_calloc_callback)calloc;

bool ms3_library_init_malloc(ms3_malloc_callback m,
                             ms3_free_callback f, ms3_realloc_callback r,
                             ms3_strdup_callback s, ms3_calloc_callback c)
{
  if (!m || !f || !r || !s || !c)
  {
    return false;
  }

  ms3_cmalloc = m;
  ms3_cfree = f;
  ms3_crealloc = r;
  ms3_cstrdup = s;
  ms3_ccalloc = c;

  if (curl_global_init_mem(CURL_GLOBAL_DEFAULT, m, f, r, s, c))
  {
    return false;
  }

  if (xmlMemSetup(f, m, r, s))
  {
    return false;
  }

  return true;
}

void ms3_library_init(void)
{
  curl_global_init(CURL_GLOBAL_DEFAULT);
}

ms3_st *ms3_thread_init(const char *s3key, const char *s3secret,
                        const char *region,
                        const char *base_domain)
{
  return ms3_init(s3key, s3secret, region, base_domain);
}

ms3_st *ms3_init(const char *s3key, const char *s3secret,
                        const char *region,
                        const char *base_domain)
{
  if ((s3key == NULL) || (s3secret == NULL))
  {
    return NULL;
  }

  if ((strlen(s3key) < 20) || (strlen(s3secret) < 40))
  {
    return NULL;
  }

  ms3_st *ms3 = ms3_cmalloc(sizeof(ms3_st));

  memcpy(ms3->s3key, s3key, 20);
  memcpy(ms3->s3secret, s3secret, 40);
  ms3->region = ms3_cstrdup(region);

  if (base_domain && strlen(base_domain))
  {
    ms3->base_domain = ms3_cstrdup(base_domain);
    // Assume that S3-compatible APIs can't support v2 list
    ms3->list_version = 1;
  }
  else
  {
    ms3->base_domain = NULL;
    ms3->list_version = 2;
  }

  ms3->buffer_chunk_size = READ_BUFFER_GROW_SIZE;

  ms3->curl = curl_easy_init();
  ms3->last_error = NULL;
  ms3->use_http = false;
  ms3->disable_verification = false;

  return ms3;
}

void ms3_deinit(ms3_st *ms3)
{
  if (!ms3)
  {
    return;
  }

  ms3debug("deinit: 0x%" PRIXPTR, (uintptr_t)ms3);
  ms3_cfree(ms3->region);
  ms3_cfree(ms3->base_domain);
  curl_easy_cleanup(ms3->curl);
  ms3_cfree(ms3->last_error);
  ms3_cfree(ms3);
}

const char *ms3_server_error(ms3_st *ms3)
{
  if (!ms3)
  {
    return NULL;
  }

  return ms3->last_error;
}

void ms3_debug(bool state)
{
  ms3debug_set(state);

  if (state)
  {
    ms3debug("enabling debug");
  }
}

const char *ms3_error(uint8_t errcode)
{
  if (errcode >= MS3_ERR_MAX)
  {
    return baderror;
  }

  return errmsgs[errcode];
}

uint8_t ms3_list(ms3_st *ms3, const char *bucket, const char *prefix,
                 ms3_list_st **list)
{
  (void) prefix;
  uint8_t res = 0;

  if (!ms3 || !bucket || !list)
  {
    return MS3_ERR_PARAMETER;
  }

  res = execute_request(ms3, MS3_CMD_LIST, bucket, NULL, prefix, NULL, 0, NULL,
                        list);
  return res;
}

uint8_t ms3_put(ms3_st *ms3, const char *bucket, const char *key,
                const uint8_t *data, size_t length)
{
  uint8_t res;

  if (!ms3 || !bucket || !key || !data)
  {
    return MS3_ERR_PARAMETER;
  }

  if (length == 0)
  {
    return MS3_ERR_NO_DATA;
  }

  // mhash can't hash more than 4GB it seems
  if (length > UINT32_MAX)
  {
    return MS3_ERR_TOO_BIG;
  }

  res = execute_request(ms3, MS3_CMD_PUT, bucket, key, NULL, data, length, NULL,
                        NULL);

  return res;
}

uint8_t ms3_get(ms3_st *ms3, const char *bucket, const char *key,
                uint8_t **data, size_t *length)
{
  uint8_t res = 0;
  memory_buffer_st buf;

  if (!ms3 || !bucket || !key || !data || !length)
  {
    return MS3_ERR_PARAMETER;
  }

  res = execute_request(ms3, MS3_CMD_GET, bucket, key, NULL, NULL, 0, NULL, &buf);
  *data = buf.data;
  *length = buf.length;
  return res;
}

uint8_t ms3_delete(ms3_st *ms3, const char *bucket, const char *key)
{
  uint8_t res;

  if (!ms3 || !bucket || !key)
  {
    return MS3_ERR_PARAMETER;
  }

  res = execute_request(ms3, MS3_CMD_DELETE, bucket, key, NULL, NULL, 0, NULL,
                        NULL);
  return res;
}

uint8_t ms3_status(ms3_st *ms3, const char *bucket, const char *key,
                   ms3_status_st *status)
{
  uint8_t res;

  if (!ms3 || !bucket || !key || !status)
  {
    return MS3_ERR_PARAMETER;
  }

  res = execute_request(ms3, MS3_CMD_HEAD, bucket, key, NULL, NULL, 0, NULL,
                        status);
  return res;
}

void ms3_list_free(ms3_list_st *list)
{
  while (list)
  {
    ms3_cfree(list->key);
    ms3_list_st *tmp = list;
    list = list->next;
    ms3_cfree(tmp);
  }
}

void ms3_free(uint8_t *data)
{
  ms3_cfree(data);
}

uint8_t ms3_buffer_chunk_size(ms3_st *ms3, size_t new_size)
{
  if (!ms3)
  {
    return MS3_ERR_PARAMETER;
  }

  if (new_size < READ_BUFFER_GROW_SIZE)
  {
    return MS3_ERR_PARAMETER;
  }

  ms3->buffer_chunk_size = new_size;
  return 0;
}

uint8_t ms3_set_option(ms3_st *ms3, ms3_set_option_t option, void *value)
{
  if (!ms3)
  {
    return MS3_ERR_PARAMETER;
  }

  switch (option)
  {
    case MS3_OPT_USE_HTTP:
    {
      bool param;

      if (value)
      {
        param = *(bool *)value;
      }
      else
      {
        param = true;
      }

      ms3->use_http = param;
      break;
    }

    case MS3_OPT_DISABLE_SSL_VERIFY:
    {
      bool param;

      if (value)
      {
        param = *(bool *)value;
      }
      else
      {
        param = true;
      }

      ms3->disable_verification = param;
      break;
    }

    case MS3_OPT_BUFFER_CHUNK_SIZE:
    {
      size_t new_size;

      if (!value)
      {
        return MS3_ERR_PARAMETER;
      }

      new_size = *(size_t *)value;

      if (new_size < READ_BUFFER_GROW_SIZE)
      {
        return MS3_ERR_PARAMETER;
      }

      ms3->buffer_chunk_size = new_size;
      break;
    }

    case MS3_OPT_FORCE_LIST_VERSION:
    {
      uint8_t list_version;

      if (!value)
      {
        return MS3_ERR_PARAMETER;
      }

      list_version = *(uint8_t *)value;

      if (list_version < 1 || list_version > 2)
      {
        return MS3_ERR_PARAMETER;
      }

      ms3->list_version = list_version;
      break;
    }

    default:
      return MS3_ERR_PARAMETER;
  }

  return 0;
}
