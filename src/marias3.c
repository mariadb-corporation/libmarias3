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
#include <arpa/inet.h>
#include <netinet/in.h>

ms3_malloc_callback ms3_cmalloc = (ms3_malloc_callback)malloc;
ms3_free_callback ms3_cfree = (ms3_free_callback)free;
ms3_realloc_callback ms3_crealloc = (ms3_realloc_callback)realloc;
ms3_strdup_callback ms3_cstrdup = (ms3_strdup_callback)strdup;
ms3_calloc_callback ms3_ccalloc = (ms3_calloc_callback)calloc;


/* Thread locking code for OpenSSL < 1.1.0 */
#include <dlfcn.h>
static pthread_mutex_t *mutex_buf = NULL;
#define CRYPTO_LOCK 1
static void (*openssl_set_id_callback)(unsigned long (*func)(void));
static void (*openssl_set_locking_callback)(void (*func)(int mode,int type, const char *file,int line));
static int (*openssl_num_locks)(void);

static void locking_function(int mode, int n, const char *file, int line)
{
  (void) file;
  (void) line;
  if(mode & CRYPTO_LOCK)
    pthread_mutex_lock(&(mutex_buf[n]));
  else
    pthread_mutex_unlock(&(mutex_buf[n]));
}

static int curl_needs_openssl_locking()
{
  curl_version_info_data *data = curl_version_info(CURLVERSION_NOW);

  if (data->ssl_version == NULL)
  {
    return 0;
  }

  if (strncmp(data->ssl_version, "OpenSSL", 7) != 0)
  {
    return 0;
  }
  if (data->ssl_version[8] == '0')
  {
    return 1;
  }
  if ((data->ssl_version[8] == '1') && (data->ssl_version[10] == '0'))
  {
    openssl_set_id_callback = dlsym(RTLD_DEFAULT, "CRYPTO_set_id_callback");
    openssl_set_locking_callback = dlsym(RTLD_DEFAULT, "CRYPTO_set_locking_callback");
    openssl_num_locks = dlsym(RTLD_DEFAULT, "CRYPTO_num_locks");
    return openssl_set_id_callback != NULL &&
           openssl_set_locking_callback != NULL &&
           openssl_num_locks != NULL;
  }
  return 0;
}

static unsigned long __attribute__((unused)) id_function(void)
{
  return ((unsigned long)pthread_self());
}

uint8_t ms3_library_init_malloc(ms3_malloc_callback m,
                                ms3_free_callback f, ms3_realloc_callback r,
                                ms3_strdup_callback s, ms3_calloc_callback c)
{
  if (!m || !f || !r || !s || !c)
  {
    return MS3_ERR_PARAMETER;
  }

  ms3_cmalloc = m;
  ms3_cfree = f;
  ms3_crealloc = r;
  ms3_cstrdup = s;
  ms3_ccalloc = c;

  if (curl_needs_openssl_locking())
  {
    int i;
    mutex_buf = ms3_cmalloc(openssl_num_locks() * sizeof(pthread_mutex_t));
    if(mutex_buf)
    {
      for(i = 0; i < openssl_num_locks(); i++)
        pthread_mutex_init(&(mutex_buf[i]), NULL);
      openssl_set_id_callback(id_function);
      openssl_set_locking_callback(locking_function);
    }
  }

  if (curl_global_init_mem(CURL_GLOBAL_DEFAULT, m, f, r, s, c))
  {
    return MS3_ERR_PARAMETER;
  }

  return 0;
}

void ms3_library_init(void)
{
  if (curl_needs_openssl_locking())
  {
    int i;
    mutex_buf = malloc(openssl_num_locks() * sizeof(pthread_mutex_t));
    if(mutex_buf)
    {
      for(i = 0; i < openssl_num_locks(); i++)
        pthread_mutex_init(&(mutex_buf[i]), NULL);
      openssl_set_id_callback(id_function);
      openssl_set_locking_callback(locking_function);
    }
  }
  curl_global_init(CURL_GLOBAL_DEFAULT);
}

void ms3_library_deinit(void)
{
  int i;
  if (mutex_buf)
  {
    openssl_set_id_callback(NULL);
    openssl_set_locking_callback(NULL);
    for(i = 0;  i < openssl_num_locks();  i++)
      pthread_mutex_destroy(&(mutex_buf[i]));
    ms3_cfree(mutex_buf);
    mutex_buf = NULL;
  }
  curl_global_cleanup();
}

ms3_st *ms3_init(const char *s3key, const char *s3secret,
                 const char *region,
                 const char *base_domain)
{
  ms3_st *ms3;

  if ((s3key == NULL) || (s3secret == NULL))
  {
    return NULL;
  }

  ms3 = ms3_cmalloc(sizeof(ms3_st));

  ms3->s3key = ms3_cstrdup(s3key);
  ms3->s3secret = ms3_cstrdup(s3secret);
  ms3->region = ms3_cstrdup(region);
  ms3->port = 0; /* The default value */

  if (base_domain && strlen(base_domain))
  {
    struct sockaddr_in sa;
    ms3->base_domain = ms3_cstrdup(base_domain);
    if (inet_pton(AF_INET, base_domain, &(sa.sin_addr)))
    {
      ms3->list_version = 1;
      ms3->protocol_version = 1;
    }
    else if (strcmp(base_domain, "s3.amazonaws.com") == 0)
    {
      ms3->list_version = 2;
      ms3->protocol_version = 2;
    }
    else
    {
      // Assume that S3-compatible APIs can't support v2 list
      ms3->list_version = 1;
      ms3->protocol_version = 2;
    }
  }
  else
  {
    ms3->base_domain = NULL;
    ms3->list_version = 2;
    ms3->protocol_version = 2;
  }

  ms3->buffer_chunk_size = READ_BUFFER_DEFAULT_SIZE;

  ms3->curl = curl_easy_init();
  ms3->last_error = NULL;
  ms3->use_http = false;
  ms3->disable_verification = false;
  ms3->first_run = true;
  ms3->path_buffer = ms3_cmalloc(sizeof(char) * 1024);
  ms3->query_buffer = ms3_cmalloc(sizeof(char) * 3072);
  ms3->list_container.pool = NULL;
  ms3->list_container.next = NULL;
  ms3->list_container.start = NULL;
  ms3->list_container.pool_list = NULL;
  ms3->list_container.pool_free = 0;

  ms3->iam_role = NULL;
  ms3->role_key = NULL;
  ms3->role_secret = NULL;
  ms3->role_session_token = NULL;
  ms3->iam_endpoint = NULL;
  ms3->sts_endpoint = NULL;
  ms3->sts_region = NULL;
  ms3->iam_role_arn = NULL;

  return ms3;
}

uint8_t ms3_init_assume_role(ms3_st *ms3, const char *iam_role, const char *sts_endpoint, const char *sts_region)
{
  uint8_t ret=0;

  if (iam_role == NULL)
  {
      return MS3_ERR_PARAMETER;
  }
  ms3->iam_role = ms3_cstrdup(iam_role);

  if (sts_endpoint && strlen(sts_endpoint))
  {
      ms3->sts_endpoint = ms3_cstrdup(sts_endpoint);
  }
  else
  {
      ms3->sts_endpoint = ms3_cstrdup("sts.amazonaws.com");
  }

  if (sts_region && strlen(sts_region))
  {
      ms3->sts_region = ms3_cstrdup(sts_region);
  }
  else
  {
      ms3->sts_region = ms3_cstrdup("us-east-1");
  }

  ms3->iam_endpoint = ms3_cstrdup("iam.amazonaws.com");

  ms3->iam_role_arn = ms3_cmalloc(sizeof(char) * 2048);
  ms3->iam_role_arn[0] = '\0';
  ms3->role_key = ms3_cmalloc(sizeof(char) * 128);
  ms3->role_key[0] = '\0';
  ms3->role_secret = ms3_cmalloc(sizeof(char) * 1024);
  ms3->role_secret[0] = '\0';
  // aws says theres no maximum length here.. 2048 might be overkill
  ms3->role_session_token = ms3_cmalloc(sizeof(char) * 2048);
  ms3->role_session_token[0] = '\0';
  // 0 will uses the default and not set a value in the request
  ms3->role_session_duration = 0;

  ret = ms3_assume_role(ms3);

  return ret;
}

uint8_t ms3_ec2_set_cred(ms3_st *ms3, const char *iam_role,
                     const char *s3key, const char *s3secret,
                     const char *token)
{
  uint8_t ret=0;

  if (iam_role == NULL || token == NULL || s3key == NULL || s3secret == NULL)
  {
      return MS3_ERR_PARAMETER;
  }
  ms3->iam_role = ms3_cstrdup(iam_role);
  ms3->role_key = ms3_cstrdup(s3key);
  ms3->role_secret = ms3_cstrdup(s3secret);
  ms3->role_session_token = ms3_cstrdup(token);

  return ret;
}

static void list_free(ms3_st *ms3)
{
  ms3_list_st *list = ms3->list_container.start;
  struct ms3_pool_alloc_list_st *plist = NULL, *next = NULL;
  while (list)
  {
    ms3_cfree(list->key);
    list = list->next;
  }
  plist = ms3->list_container.pool_list;
  while (plist)
  {
    next = plist->prev;
    ms3_cfree(plist->pool);
    ms3_cfree(plist);
    plist = next;
  }
  ms3->list_container.pool = NULL;
  ms3->list_container.next = NULL;
  ms3->list_container.start = NULL;
  ms3->list_container.pool_list = NULL;
  ms3->list_container.pool_free = 0;
}

void ms3_deinit(ms3_st *ms3)
{
  if (!ms3)
  {
    return;
  }

  ms3debug("deinit: 0x%" PRIXPTR, (uintptr_t)ms3);
  ms3_cfree(ms3->s3secret);
  ms3_cfree(ms3->s3key);
  ms3_cfree(ms3->region);
  ms3_cfree(ms3->base_domain);
  ms3_cfree(ms3->iam_role);
  ms3_cfree(ms3->role_key);
  ms3_cfree(ms3->role_secret);
  ms3_cfree(ms3->role_session_token);
  ms3_cfree(ms3->iam_endpoint);
  ms3_cfree(ms3->sts_endpoint);
  ms3_cfree(ms3->sts_region);
  ms3_cfree(ms3->iam_role_arn);
  curl_easy_cleanup(ms3->curl);
  ms3_cfree(ms3->last_error);
  ms3_cfree(ms3->path_buffer);
  ms3_cfree(ms3->query_buffer);
  list_free(ms3);
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

void ms3_debug(void)
{
  bool state = ms3debug_get();
  ms3debug_set(!state);

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

uint8_t ms3_list_dir(ms3_st *ms3, const char *bucket, const char *prefix,
                     ms3_list_st **list)
{
  uint8_t res = 0;

  if (!ms3 || !bucket || !list)
  {
    return MS3_ERR_PARAMETER;
  }

  list_free(ms3);
  res = execute_request(ms3, MS3_CMD_LIST, bucket, NULL, NULL, NULL, prefix, NULL,
                        0, NULL,
                        NULL);
  *list = ms3->list_container.start;
  return res;
}

uint8_t ms3_list(ms3_st *ms3, const char *bucket, const char *prefix,
                 ms3_list_st **list)
{
  uint8_t res = 0;

  if (!ms3 || !bucket || !list)
  {
    return MS3_ERR_PARAMETER;
  }

  list_free(ms3);
  res = execute_request(ms3, MS3_CMD_LIST_RECURSIVE, bucket, NULL, NULL, NULL,
                        prefix, NULL,
                        0, NULL,
                        NULL);
  *list = ms3->list_container.start;
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

  res = execute_request(ms3, MS3_CMD_PUT, bucket, key, NULL, NULL, NULL, data,
                        length, NULL,
                        NULL);

  return res;
}

uint8_t ms3_get(ms3_st *ms3, const char *bucket, const char *key,
                uint8_t **data, size_t *length)
{
  uint8_t res = 0;
  struct memory_buffer_st buf;

  buf.data = NULL;
  buf.length = 0;

  if (!ms3 || !bucket || !key || key[0] == '\0' || !data || !length)
  {
    return MS3_ERR_PARAMETER;
  }

  res = execute_request(ms3, MS3_CMD_GET, bucket, key, NULL, NULL, NULL, NULL, 0,
                        NULL, &buf);
  *data = buf.data;
  *length = buf.length;
  return res;
}

uint8_t ms3_copy(ms3_st *ms3, const char *source_bucket, const char *source_key,
                 const char *dest_bucket, const char *dest_key)
{
  uint8_t res = 0;

  if (!ms3 || !source_bucket || !source_key || !dest_bucket || !dest_key)
  {
    return MS3_ERR_PARAMETER;
  }

  res = execute_request(ms3, MS3_CMD_COPY, dest_bucket, dest_key, source_bucket,
                        source_key, NULL, NULL, 0, NULL, NULL);
  return res;
}

uint8_t ms3_move(ms3_st *ms3, const char *source_bucket, const char *source_key,
                 const char *dest_bucket, const char *dest_key)
{
  uint8_t res = 0;

  if (!ms3 || !source_bucket || !source_key || !dest_bucket || !dest_key)
  {
    return MS3_ERR_PARAMETER;
  }

  res = ms3_copy(ms3, source_bucket, source_key, dest_bucket, dest_key);

  if (res)
  {
    return res;
  }

  res = ms3_delete(ms3, source_bucket, source_key);

  return res;
}

uint8_t ms3_delete(ms3_st *ms3, const char *bucket, const char *key)
{
  uint8_t res;

  if (!ms3 || !bucket || !key)
  {
    return MS3_ERR_PARAMETER;
  }

  res = execute_request(ms3, MS3_CMD_DELETE, bucket, key, NULL, NULL, NULL, NULL,
                        0, NULL,
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

  res = execute_request(ms3, MS3_CMD_HEAD, bucket, key, NULL, NULL, NULL, NULL, 0,
                        NULL,
                        status);
  return res;
}

void ms3_list_free(ms3_list_st *list)
{
  // Deprecated
  (void) list;
}

void ms3_free(uint8_t *data)
{
  ms3_cfree(data);
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
      ms3->use_http = ms3->use_http ? 0 : 1;
      break;
    }

    case MS3_OPT_DISABLE_SSL_VERIFY:
    {
      ms3->disable_verification = ms3->disable_verification ? 0 : 1;
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

      if (new_size < 1)
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

    case MS3_OPT_FORCE_PROTOCOL_VERSION:
    {
      uint8_t protocol_version;

      if (!value)
      {
        return MS3_ERR_PARAMETER;
      }

      protocol_version = *(uint8_t *)value;

      if (protocol_version < 1 || protocol_version > 2)
      {
        return MS3_ERR_PARAMETER;
      }

      ms3->list_version = protocol_version;
      break;
    }

    case MS3_OPT_PORT_NUMBER:
    {
      int port_number;

      if (!value)
      {
        return MS3_ERR_PARAMETER;
      }
      memcpy(&port_number, (void*)value, sizeof(int));

      ms3->port = port_number;
      break;
    }
    default:
      return MS3_ERR_PARAMETER;
  }

  return 0;
}

uint8_t ms3_assume_role(ms3_st *ms3)
{
    uint8_t res = 0;

    if (!ms3 || !ms3->iam_role)
    {
      return MS3_ERR_PARAMETER;
    }

    if (!strstr(ms3->iam_role_arn, ms3->iam_role))
    {
        ms3debug("Lookup IAM role ARN");
        res = execute_assume_role_request(ms3, MS3_CMD_LIST_ROLE, NULL, 0, NULL);
        if(res)
        {
          return res;
        }

    }
    ms3debug("Assume IAM role");
    res = execute_assume_role_request(ms3, MS3_CMD_ASSUME_ROLE, NULL, 0, NULL);

    return res;
}

