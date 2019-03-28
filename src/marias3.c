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

#include <pthread.h>

void ms3_library_init(void)
{
  curl_global_init(CURL_GLOBAL_DEFAULT);
}

ms3_st *ms3_thread_init(const char *s3key, const char *s3secret,
                        const char *region,
                        const char *base_domain)
{
  if ((s3key == NULL) or (s3secret == NULL))
  {
    return NULL;
  }

  if ((strlen(s3key) < 20) or (strlen(s3secret) < 40))
  {
    return NULL;
  }

  ms3_st *ms3 = malloc(sizeof(ms3_st));

  memcpy(ms3->s3key, s3key, 20);
  memcpy(ms3->s3secret, s3secret, 40);
  ms3->region = strdup(region);

  if (base_domain and strlen(base_domain))
  {
    ms3->base_domain = strdup(base_domain);
  }
  else
  {
    ms3->base_domain = NULL;
  }

  ms3->buffer_chunk_size = READ_BUFFER_GROW_SIZE;

  ms3->curl = curl_easy_init();
  ms3->last_error = NULL;

  return ms3;
}

void ms3_deinit(ms3_st *ms3)
{
  if (not ms3)
  {
    return;
  }
  ms3debug("deinit: 0x%" PRIXPTR, (uintptr_t)ms3);
  free(ms3->region);
  curl_easy_cleanup(ms3->curl);
  free(ms3->last_error);
  free(ms3);
}

const char *ms3_server_error(ms3_st *ms3)
{
  if (not ms3)
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

  if (not ms3 or not bucket or not list)
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

  if (not ms3 or not bucket or not key or not data)
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

  if (not ms3 or not bucket or not key or not data or not length)
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

  if (not ms3 or not bucket or not key)
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

  if (not ms3 or not bucket or not key or not status)
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
    free(list->key);
    ms3_list_st *tmp = list;
    list = list->next;
    free(tmp);
  }
}

void ms3_free(uint8_t *data)
{
  free(data);
}

uint8_t ms3_buffer_chunk_size(ms3_st *ms3, size_t new_size)
{
  if (not ms3)
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
