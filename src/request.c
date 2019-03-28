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

const char *default_domain = "s3.amazonaws.com";

static void set_error(ms3_st *ms3, const char *error)
{
  free(ms3->last_error);
  if (not error)
  {
    return;
  }
  ms3->last_error = strdup(error);
}

static void set_error_nocopy(ms3_st *ms3, char *error)
{
  free(ms3->last_error);
  if (not error)
  {
    return;
  }
  ms3->last_error = error;
}

static uint8_t build_request_uri(CURL *curl, const char *base_domain,
                                 const char *bucket, const char *object, const char *query)
{
  char uri_buffer[MAX_URI_LENGTH];
  const char *domain;
  const uint8_t path_parts = 10; // "https://" + "." + "/"

  if (base_domain)
  {
    domain = base_domain;
  }
  else
  {
    domain = default_domain;
  }

  if (query)
  {
    if (path_parts + strlen(domain) + strlen(bucket) + strlen(object) + strlen(
          query) >= MAX_URI_LENGTH - 1)
    {
      return MS3_ERR_URI_TOO_LONG;
    }

    snprintf(uri_buffer, MAX_URI_LENGTH - 1, "https://%s.%s%s?%s", bucket, domain,
             object, query);
  }
  else
  {
    if (path_parts + strlen(domain) + strlen(bucket) + strlen(
          object) >= MAX_URI_LENGTH - 1)
    {
      return MS3_ERR_URI_TOO_LONG;
    }

    snprintf(uri_buffer, MAX_URI_LENGTH - 1, "https://%s.%s%s", bucket, domain,
             object);
  }

  ms3debug("URI: %s", uri_buffer);
  curl_easy_setopt(curl, CURLOPT_URL, uri_buffer);
  return 0;
}

/* Handles object name to path conversion.
 * Must always start with a '/' even if object is empty.
 * Object should be urlencoded. Unfortunately curl also urlencodes slashes.
 * So this breaks up on slashes and reassembles the encoded parts.
 * Not very efficient but works until we write a custom encoder.
 */

static char *generate_path(CURL *curl, const char *object)
{
  char *ret_buf = malloc(sizeof(char) * 1024);
  char *tok_ptr = NULL;
  char *save_ptr = NULL;
  char *out_ptr = ret_buf;

  // Keep scanbuild happy
  ret_buf[0] = '\0';

  if (not object)
  {
    sprintf(ret_buf, "/");
    return ret_buf;
  }

  char *path = strdup(object); // Because strtok_r is destructive

  tok_ptr = strtok_r((char *)path, "/", &save_ptr);

  while (tok_ptr != NULL)
  {
    char *encoded = curl_easy_escape(curl, tok_ptr, (int)strlen(tok_ptr));
    snprintf(out_ptr, 1024 - (ret_buf - out_ptr), "/%s", encoded);
    out_ptr += strlen(encoded) + 1;
    curl_free(encoded);
    tok_ptr = strtok_r(NULL, "/", &save_ptr);
  }

  if (ret_buf[0] != '/')
  {
    sprintf(ret_buf, "/");
  }

  free(path);
  return ret_buf;
}


/* At a later date we need to make this accept multi-param/values to support
 * pagination
 */

static char *generate_query(CURL *curl, const char *value,
                            const char *continuation)
{
  char *ret_buf = malloc(sizeof(char) * 1024);
  char *encoded;

  if (continuation)
  {
    encoded = curl_easy_escape(curl, continuation, (int)strlen(continuation));
    snprintf(ret_buf, 1024, "continuation-token=%s&list-type=2", encoded);
    free(encoded);
  }
  else
  {
    sprintf(ret_buf, "list-type=2");
  }

  if (value)
  {
    encoded = curl_easy_escape(curl, value, (int)strlen(value));
    snprintf(ret_buf + strlen(ret_buf), 1024 - strlen(ret_buf), "&prefix=%s",
             encoded);
    free(encoded);
  }

  return ret_buf;
}


/*
<HTTPMethod>\n
<CanonicalURI>\n
<CanonicalQueryString>\n
<CanonicalHeaders>\n
<SignedHeaders>\n - host;x-amz-content-sha256;x-amz-date
<HashedPayload> - empty if no POST data
*/
static uint8_t generate_request_hash(uri_method_t method, const char *path,
                                     const char *query, char *post_hash, struct curl_slist *headers,
                                     char *return_hash)
{
  char signing_data[1024];
  size_t pos = 0;
  MHASH td;
  uint8_t sha256hash[32]; // SHA_256 binary length
  uint8_t hash_pos = 0;

  // Method first
  switch (method)
  {
    case MS3_GET:
    {
      sprintf(signing_data, "GET\n");
      pos += 4;
      break;
    }

    case MS3_HEAD:
    {
      sprintf(signing_data, "HEAD\n");
      pos += 5;
      break;
    }

    case MS3_PUT:
    {
      sprintf(signing_data, "PUT\n");
      pos += 4;
      break;
    }

    case MS3_DELETE:
    {
      sprintf(signing_data, "DELETE\n");
      pos += 7;
      break;
    }

    default:
    {
      ms3debug("Bad method detected");
      return MS3_ERR_IMPOSSIBLE;
    }
  }

  // URL path
  snprintf(signing_data + pos, sizeof(signing_data) - pos, "%s\n", path);
  pos += strlen(path) + 1;

  // URL query (if exists)
  if (query)
  {
    snprintf(signing_data + pos, sizeof(signing_data) - pos, "%s\n", query);
    pos += strlen(query) + 1;
  }
  else
  {
    sprintf(signing_data + pos, "\n");
    pos++;
  }

  // All the headers
  struct curl_slist *current_header = headers;

  do
  {
    snprintf(signing_data + pos, sizeof(signing_data) - pos, "%s\n",
             current_header->data);
    pos += strlen(current_header->data) + 1;
  }
  while ((current_header = current_header->next));

  // List if header names
  // The newline between headers and this is important
  snprintf(signing_data + pos, sizeof(signing_data) - pos,
           "\nhost;x-amz-content-sha256;x-amz-date\n");
  pos += 38;

  // Hash of post data (can be hash of empty)
  snprintf(signing_data + pos, sizeof(signing_data) - pos, "%.*s", 64, post_hash);
  //pos+= 64;

  // Hash all of the above
  td = mhash_init(MHASH_SHA256);
  mhash(td, signing_data, (uint32_t)strlen(signing_data));
  mhash_deinit(td, sha256hash);

  for (uint8_t i = 0; i < mhash_get_block_size(MHASH_SHA256); i++)
  {
    sprintf(return_hash + hash_pos, "%.2x", sha256hash[i]);
    hash_pos += 2;
  }

  ms3debug("Signature data: %s", signing_data);
  ms3debug("Signature: %.*s", 64, return_hash);

  return 0;
}

static size_t put_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
  put_buffer_st *buf = (put_buffer_st *)stream;
  size_t buffer_size = size * nmemb;

  ms3debug("PUT callback %lu bytes remaining, %lu buffer", buf->length - buf->offset, buffer_size);
  // All data copied
  if (buf->length == buf->offset)
  {
    return 0;
  }

  if (buffer_size >= buf->length - buf->offset)
  {
    size_t transfer = buf->length - buf->offset;
    memcpy(ptr, buf->data + buf->offset, transfer);
    buf->offset = buf->length;
    return transfer;
  }
  else
  {
    memcpy(ptr, buf->data + buf->offset, buffer_size);
    buf->offset += buffer_size;
    return buffer_size;
  }

}

static uint8_t build_request_headers(CURL *curl, struct curl_slist **head,
                                     const char *base_domain, const char *region, const char *key,
                                     const char *secret, const char *object, const char *query,
                                     uri_method_t method, const char *bucket, put_buffer_st *post_data)
{
  uint8_t ret = 0;
  time_t now;
  char headerbuf[1024];
  char date[9];
  MHASH td;
  char sha256hash[65];
  char post_hash[65];
  uint8_t tmp_hash[32];
  uint8_t hmac_hash[32];
  uint8_t hash_pos = 0;
  const char *domain;
  struct curl_slist *headers = NULL;

  // Host header
  if (base_domain)
  {
    domain = base_domain;
  }
  else
  {
    domain = default_domain;
  }

  snprintf(headerbuf, sizeof(headerbuf), "host:%s.%s", bucket, domain);
  headers = curl_slist_append(headers, headerbuf);
  *head = headers;

  // Hash post data
  td = mhash_init(MHASH_SHA256);
  mhash(td, post_data->data, (uint32_t)post_data->length);
  mhash_deinit(td, tmp_hash);

  for (uint8_t i = 0; i < mhash_get_block_size(MHASH_SHA256); i++)
  {
    sprintf(post_hash + hash_pos, "%.2x", tmp_hash[i]);
    hash_pos += 2;
  }

  snprintf(headerbuf, sizeof(headerbuf), "x-amz-content-sha256:%.*s", 64,
           post_hash);
  headers = curl_slist_append(headers, headerbuf);

  // Date/time header
  time(&now);
  snprintf(headerbuf, sizeof(headerbuf), "x-amz-date:");
  uint8_t offset = strlen(headerbuf);
  strftime(headerbuf + offset, sizeof(headerbuf) - offset, "%Y%m%dT%H%M%SZ",
           gmtime(&now));
  headers = curl_slist_append(headers, headerbuf);

  // Builds the request hash
  ret = generate_request_hash(method, object, query, post_hash, headers,
                              sha256hash);

  if (ret)
  {
    return ret;
  }

  // User signing key hash
  // Date hashed using AWS4:secret_key
  snprintf(headerbuf, sizeof(headerbuf), "AWS4%.*s", 40, secret);
  td = mhash_hmac_init(MHASH_SHA256, headerbuf, (uint32_t)strlen(headerbuf),
                       mhash_get_hash_pblock(MHASH_SHA1));
  strftime(headerbuf, sizeof(headerbuf), "%Y%m%d", gmtime(&now));
  mhash(td, headerbuf, (uint32_t)strlen(headerbuf));
  mhash_hmac_deinit(td, hmac_hash);

  // Region signed by above key
  td = mhash_hmac_init(MHASH_SHA256, hmac_hash, 32,
                       mhash_get_hash_pblock(MHASH_SHA1));
  mhash(td, region, (uint32_t)strlen(region));
  mhash_hmac_deinit(td, hmac_hash);

  // Service signed by above key (s3 always)
  td = mhash_hmac_init(MHASH_SHA256, hmac_hash, 32,
                       mhash_get_hash_pblock(MHASH_SHA1));
  sprintf(headerbuf, "s3");
  mhash(td, headerbuf, (uint32_t)strlen(headerbuf));
  mhash_hmac_deinit(td, hmac_hash);

  // Request version signed by above key (always "aws4_request")
  td = mhash_hmac_init(MHASH_SHA256, hmac_hash, 32,
                       mhash_get_hash_pblock(MHASH_SHA1));
  sprintf(headerbuf, "aws4_request");
  mhash(td, headerbuf, (uint32_t)strlen(headerbuf));
  mhash_hmac_deinit(td, hmac_hash);

  // Sign everything with the key
  snprintf(headerbuf, sizeof(headerbuf), "AWS4-HMAC-SHA256\n");
  offset = strlen(headerbuf);
  strftime(headerbuf + offset, sizeof(headerbuf) - offset, "%Y%m%dT%H%M%SZ\n",
           gmtime(&now));
  offset = strlen(headerbuf);
  strftime(date, 9, "%Y%m%d", gmtime(&now));
  snprintf(headerbuf + offset, sizeof(headerbuf) - offset,
           "%.*s/%s/s3/aws4_request\n%.*s", 8, date, region, 64, sha256hash);
  ms3debug("Data to sign: %s", headerbuf);
  td = mhash_hmac_init(MHASH_SHA256, hmac_hash, 32,
                       mhash_get_hash_pblock(MHASH_SHA1));
  mhash(td, headerbuf, (uint32_t)strlen(headerbuf));
  mhash_hmac_deinit(td, hmac_hash);

  hash_pos = 0;

  for (uint8_t i = 0; i < mhash_get_block_size(MHASH_SHA256); i++)
  {
    sprintf(sha256hash + hash_pos, "%.2x", hmac_hash[i]);
    hash_pos += 2;
  }

  // Make auth header
  snprintf(headerbuf, sizeof(headerbuf),
           "Authorization: AWS4-HMAC-SHA256 Credential=%.*s/%s/%s/s3/aws4_request, SignedHeaders=host;x-amz-content-sha256;x-amz-date, Signature=%s",
           20, key, date, region, sha256hash);
  headers = curl_slist_append(headers, headerbuf);

  // Disable this header or PUT will barf with a 501
  sprintf(headerbuf, "Transfer-Encoding:");
  headers = curl_slist_append(headers, headerbuf);

  if (method == MS3_PUT)
  {
    snprintf(headerbuf, sizeof(headerbuf), "Content-Length:%zu", post_data->length);
    headers = curl_slist_append(headers, headerbuf);
  }

  struct curl_slist *current_header = headers;

  do
  {
    ms3debug("Header: %s", current_header->data);
  }
  while ((current_header = current_header->next));

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  switch (method)
  {
    case MS3_GET:
    {
      // Nothing extra to do here
      break;
    }

    case MS3_HEAD:
    {
      curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
      break;
    }

    case MS3_PUT:
    {
      curl_easy_setopt(curl, CURLOPT_UPLOAD, 1);
      curl_easy_setopt(curl, CURLOPT_PUT, 1L);
      curl_easy_setopt(curl, CURLOPT_READDATA, post_data);
      curl_easy_setopt(curl, CURLOPT_READFUNCTION, put_callback);
      break;
    }

    case MS3_DELETE:
    {
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
      break;
    }

    default:
      ms3debug("Bad method detected");
      return MS3_ERR_IMPOSSIBLE;
  }

  return 0;
}
static size_t header_callback(char *buffer, size_t size,
                              size_t nitems, void *userdata)
{
  ms3debug("%.*s\n", (int)(nitems * size), buffer);

  if (userdata)
  {
    // HEAD request
    if (not strncmp(buffer, "Last-Modified", 13))
    {
      ms3_status_st *status = (ms3_status_st *) userdata;
      // Date/time, format: Fri, 15 Mar 2019 16:58:54 GMT
      struct tm ttmp = {0};
      strptime(buffer + 15, "%a, %d %b %Y %H:%M:%S %Z", &ttmp);
      status->created = mktime(&ttmp);
    }
    else if (not strncmp(buffer, "Content-Length", 14))
    {
      ms3_status_st *status = (ms3_status_st *) userdata;
      // Length
      status->length = strtoull(buffer + 16, NULL, 10);
    }
  }

  return nitems * size;
}

static size_t body_callback(void *buffer, size_t size,
                            size_t nitems, void *userdata)
{
  size_t realsize = nitems * size;

  struct memory_buffer_st *mem = (struct memory_buffer_st *)userdata;

  if (realsize + mem->length > mem->alloced)
  {
    uint8_t *ptr = realloc(mem->data, mem->alloced + mem->buffer_chunk_size);

    if (not ptr)
    {
      ms3debug("Curl response OOM");
      return 0;
    }
    mem->alloced += mem->buffer_chunk_size;
    mem->data = ptr;
  }

  memcpy(&(mem->data[mem->length]), buffer, realsize);
  mem->length += realsize;
  mem->data[mem->length] = 0;

  ms3debug("Read %lu bytes, buffer %lu bytes", realsize, mem->length);
  return nitems * size;
}

uint8_t execute_request(ms3_st *ms3, command_t cmd, const char *bucket,
                        const char *object, const char *filter, const uint8_t *data, size_t data_size,
                        char *continuation,
                        void *ret_ptr)
{
  CURL *curl = NULL;
  struct curl_slist *headers = NULL;
  uint8_t res = 0;
  struct memory_buffer_st mem;
  mem.data = malloc(1);
  mem.length = 0;
  mem.alloced = 1;
  mem.buffer_chunk_size = ms3->buffer_chunk_size;
  uri_method_t method;
  char *path = NULL;
  char *query = NULL;
  struct put_buffer_st post_data;
  post_data.data = (uint8_t *) data;
  post_data.length = data_size;
  post_data.offset = 0;

  curl = ms3->curl;
  curl_easy_reset(curl);

  path = generate_path(curl, object);

  if (cmd == MS3_CMD_LIST)
  {
    query = generate_query(curl, filter, continuation);
  }

  res = build_request_uri(curl, ms3->base_domain, bucket, path, query);

  if (res)
  {
    free(mem.data);
    free(path);
    free(query);

    return res;
  }

  switch (cmd)
  {
    case MS3_CMD_PUT:
      method = MS3_PUT;
      break;

    case MS3_CMD_DELETE:
      method = MS3_DELETE;
      break;

    case MS3_CMD_HEAD:
      method = MS3_HEAD;
      curl_easy_setopt(curl, CURLOPT_HEADERDATA, ret_ptr);
      break;

    case MS3_CMD_LIST:
    case MS3_CMD_GET:
      method = MS3_GET;
      break;

    default:
      ms3debug("Bad cmd detected");
      free(mem.data);
      free(path);
      free(query);

      return MS3_ERR_IMPOSSIBLE;
  }

  res = build_request_headers(curl, &headers, ms3->base_domain, ms3->region,
                              ms3->s3key, ms3->s3secret, path, query, method, bucket, &post_data);

  if (res)
  {
    free(mem.data);
    free(path);
    free(query);
    curl_slist_free_all(headers);

    return res;
  }

  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, body_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&mem);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
  CURLcode curl_res = curl_easy_perform(curl);

  if (curl_res != CURLE_OK)
  {
    ms3debug("Curl error: %d", curl_res);
    set_error(ms3, curl_easy_strerror(curl_res));
    free(mem.data);
    free(path);
    free(query);
    curl_slist_free_all(headers);

    return MS3_ERR_REQUEST_ERROR;
  }

  long response_code;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
  ms3debug("Response code: %ld", response_code);

  if (response_code == 404)
  {
    char *message = parse_error_message((char*)mem.data, mem.length);
    set_error_nocopy(ms3, message);
    res = MS3_ERR_NOT_FOUND;
  }
  else if (response_code == 403)
  {
    char *message = parse_error_message((char*)mem.data, mem.length);
    set_error_nocopy(ms3, message);
    res = MS3_ERR_AUTH;
  }
  else if (response_code >= 400)
  {
    char *message = parse_error_message((char*)mem.data, mem.length);
    set_error_nocopy(ms3, message);
    res = MS3_ERR_SERVER;
  }

  switch (cmd)
  {
    case MS3_CMD_LIST:
    {
      ms3_list_st **list = (ms3_list_st **) ret_ptr;
      char *cont = NULL;
      parse_list_response((const char *)mem.data, mem.length, list, &cont);

      if (cont)
      {
        ms3_list_st *append_list = NULL;
        res = execute_request(ms3, cmd, bucket, object, filter, data, data_size, cont,
                              &append_list);
        ms3_list_st *list_it = *list;

        while (list_it->next != NULL)
        {
          list_it = list_it->next;
        }

        list_it->next = append_list;

        if (res)
        {
          return res;
        }

        free(cont);
      }

      free(mem.data);
      break;
    }

    case MS3_CMD_PUT:
    {
      free(mem.data);
      break;
    }

    case MS3_CMD_GET:
    {
      memory_buffer_st *buf = (memory_buffer_st *) ret_ptr;
      buf->data = mem.data;
      buf->length = mem.length;
      break;
    }

    case MS3_CMD_DELETE:
    {
      free(mem.data);
      break;
    }

    case MS3_CMD_HEAD:
    {
      free(mem.data);
      break;
    }

    default:
    {
      free(mem.data);
      ms3debug("Bad cmd detected");
      res = MS3_ERR_IMPOSSIBLE;
    }
  }

  free(path);
  free(query);
  curl_slist_free_all(headers);

  return res;
}
