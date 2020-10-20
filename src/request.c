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
#include "sha256.h"

#include <math.h>

const char *default_domain = "s3.amazonaws.com";

static void set_error(ms3_st *ms3, const char *error)
{
  ms3_cfree(ms3->last_error);

  if (!error)
  {
    ms3->last_error = NULL;
    return;
  }

  ms3->last_error = ms3_cstrdup(error);
}

static void set_error_nocopy(ms3_st *ms3, char *error)
{
  ms3_cfree(ms3->last_error);

  if (!error)
  {
    ms3->last_error = NULL;
    return;
  }

  ms3->last_error = error;
}

static uint8_t build_request_uri(CURL *curl, const char *base_domain,
                                 const char *bucket, const char *object, const char *query, bool use_http,
                                 uint8_t protocol_version)
{
  char uri_buffer[MAX_URI_LENGTH];
  const char *domain;
  const uint8_t path_parts = 10; // "https://" + "." + "/"
  const char *http_protocol = "http";
  const char *https_protocol = "https";
  const char *protocol;

  if (base_domain)
  {
    domain = base_domain;
  }
  else
  {
    domain = default_domain;
  }

  if (use_http)
  {
    protocol = http_protocol;
  }
  else
  {
    protocol = https_protocol;
  }

  if (query)
  {
    if (path_parts + strlen(domain) + strlen(bucket) + strlen(object) + strlen(
          query) >= MAX_URI_LENGTH - 1)
    {
      return MS3_ERR_URI_TOO_LONG;
    }

    if (protocol_version == 1)
    {
      snprintf(uri_buffer, MAX_URI_LENGTH - 1, "%s://%s/%s%s?%s", protocol,
               domain, bucket,
               object, query);
    }
    else
    {
      snprintf(uri_buffer, MAX_URI_LENGTH - 1, "%s://%s.%s%s?%s", protocol,
               bucket, domain,
               object, query);
    }
  }
  else
  {
    if (path_parts + strlen(domain) + strlen(bucket) + strlen(
          object) >= MAX_URI_LENGTH - 1)
    {
      return MS3_ERR_URI_TOO_LONG;
    }

    if (protocol_version == 1)
    {
      snprintf(uri_buffer, MAX_URI_LENGTH - 1, "%s://%s/%s%s", protocol,
               domain,
               bucket,
               object);
    }
    else
    {
      snprintf(uri_buffer, MAX_URI_LENGTH - 1, "%s://%s.%s%s", protocol,
               bucket, domain,
               object);
    }
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

static char *generate_path(CURL *curl, const char *object, char *path_buffer)
{
  char *tok_ptr = NULL;
  char *save_ptr = NULL;
  char *out_ptr = path_buffer;
  char *path;

  // Keep scanbuild happy
  path_buffer[0] = '\0';

  if (!object)
  {
    sprintf(path_buffer, "/");
    return path_buffer;
  }

  path = ms3_cstrdup(object); // Because strtok_r is destructive

  tok_ptr = strtok_r((char *)path, "/", &save_ptr);

  while (tok_ptr != NULL)
  {
    char *encoded = curl_easy_escape(curl, tok_ptr, (int)strlen(tok_ptr));
    snprintf(out_ptr, 1024 - (out_ptr - path_buffer), "/%s", encoded);
    out_ptr += strlen(encoded) + 1;
    curl_free(encoded);
    tok_ptr = strtok_r(NULL, "/", &save_ptr);
  }

  if (path_buffer[0] != '/')
  {
    sprintf(path_buffer, "/");
  }

  ms3_cfree(path);
  return path_buffer;
}


/* At a later date we need to make this accept multi-param/values to support
 * pagination
 */

static char *generate_query(CURL *curl, const char *value,
                            const char *continuation, uint8_t list_version, bool use_delimiter,
                            char *query_buffer)
{
  char *encoded;
  query_buffer[0] = '\0';

  if (use_delimiter)
  {
    snprintf(query_buffer, 3072, "delimiter=%%2F");
  }

  if (list_version == 2)
  {
    if (continuation)
    {
      encoded = curl_easy_escape(curl, continuation, (int)strlen(continuation));

      if (strlen(query_buffer))
      {
        snprintf(query_buffer + strlen(query_buffer), 3072 - strlen(query_buffer),
                 "&continuation-token=%s&list-type=2", encoded);
      }
      else
      {
        snprintf(query_buffer, 3072, "continuation-token=%s&list-type=2", encoded);
      }

      curl_free(encoded);
    }
    else
    {
      if (strlen(query_buffer))
      {
        snprintf(query_buffer + strlen(query_buffer), 3072 - strlen(query_buffer),
                 "&list-type=2");
      }
      else
      {
        sprintf(query_buffer, "list-type=2");
      }
    }
  }
  else if (continuation)
  {
    // Continuation is really marker here
    encoded = curl_easy_escape(curl, continuation, (int)strlen(continuation));

    if (strlen(query_buffer))
    {
      snprintf(query_buffer + strlen(query_buffer), 3072 - strlen(query_buffer),
               "&marker=%s",
               encoded);
    }
    else
    {
      snprintf(query_buffer, 3072, "marker=%s", encoded);
    }

    curl_free(encoded);
  }

  if (value)
  {
    encoded = curl_easy_escape(curl, value, (int)strlen(value));

    if (strlen(query_buffer))
    {
      snprintf(query_buffer + strlen(query_buffer), 3072 - strlen(query_buffer),
               "&prefix=%s",
               encoded);
    }
    else
    {
      snprintf(query_buffer, 3072, "prefix=%s",
               encoded);
    }

    curl_free(encoded);
  }

  return query_buffer;
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
                                     const char *bucket,
                                     const char *query, char *post_hash, struct curl_slist *headers, bool has_source, bool has_token,
                                     char *return_hash)
{
  char signing_data[3072];
  size_t pos = 0;
  uint8_t sha256hash[32]; // SHA_256 binary length
  uint8_t hash_pos = 0;
  uint8_t i;
  struct curl_slist *current_header = headers;

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
  if (bucket)
  {
    snprintf(signing_data + pos, sizeof(signing_data) - pos, "/%s%s\n", bucket,
             path);
    pos += strlen(path) + strlen(bucket) + 2;
  }
  else
  {
    snprintf(signing_data + pos, sizeof(signing_data) - pos, "%s\n", path);
    pos += strlen(path) + 1;
  }

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

  do
  {
    snprintf(signing_data + pos, sizeof(signing_data) - pos, "%s\n",
             current_header->data);
    pos += strlen(current_header->data) + 1;
  }
  while ((current_header = current_header->next));

  // List if header names
  // The newline between headers and this is important
  if (has_source && has_token)
  {
    snprintf(signing_data + pos, sizeof(signing_data) - pos,
             "\nhost;x-amz-content-sha256;x-amz-copy-source;x-amz-date;x-amz-security-token\n");
    pos += 77;
  }
  else if (has_source)
  {
    snprintf(signing_data + pos, sizeof(signing_data) - pos,
             "\nhost;x-amz-content-sha256;x-amz-copy-source;x-amz-date\n");
    pos += 56;
  }
  else if (has_token)
  {
    snprintf(signing_data + pos, sizeof(signing_data) - pos,
             "\nhost;x-amz-content-sha256;x-amz-date;x-amz-security-token\n");
    pos += 59;
  }
  else
  {
    snprintf(signing_data + pos, sizeof(signing_data) - pos,
             "\nhost;x-amz-content-sha256;x-amz-date\n");
    pos += 38;
  }

  // Hash of post data (can be hash of empty)
  snprintf(signing_data + pos, sizeof(signing_data) - pos, "%.*s", 64, post_hash);
  //pos+= 64;
  ms3debug("Signature data1: %s", signing_data);

  // Hash all of the above
  sha256((uint8_t *)signing_data, strlen(signing_data), (uint8_t *)sha256hash);

  for (i = 0; i < 32; i++)
  {
    sprintf(return_hash + hash_pos, "%.2x", sha256hash[i]);
    hash_pos += 2;
  }

  ms3debug("Signature data: %s", signing_data);
  ms3debug("Signature: %.*s", 64, return_hash);

  return 0;
}

static uint8_t build_request_headers(CURL *curl, struct curl_slist **head,
                                     const char *base_domain, const char *region, const char *key,
                                     const char *secret, const char *object, const char *query,
                                     uri_method_t method, const char *bucket, const char *source_bucket,
                                     const char *source_key, struct put_buffer_st *post_data,
                                     uint8_t protocol_version, const char *session_token)
{
  uint8_t ret = 0;
  time_t now;
  struct tm tmp_tm;
  char headerbuf[3072];
  char secrethead[45];
  char date[9];
  char sha256hash[65];
  char post_hash[65];
  uint8_t tmp_hash[32];
  // Alternate between these two so hmac doesn't overwrite itself
  uint8_t hmac_hash[32];
  uint8_t hmac_hash2[32];
  uint8_t hash_pos = 0;
  const char *domain;
  struct curl_slist *headers = NULL;
  uint8_t offset;
  uint8_t i;
  bool has_source = false;
  bool has_token = false;
  struct curl_slist *current_header;

  // Host header
  if (base_domain)
  {
    domain = base_domain;
  }
  else
  {
    domain = default_domain;
  }

  if (protocol_version == 2)
  {
    snprintf(headerbuf, sizeof(headerbuf), "host:%s.%s", bucket, domain);
  }
  else
  {
    snprintf(headerbuf, sizeof(headerbuf), "host:%s", domain);
  }
  headers = curl_slist_append(headers, headerbuf);
  *head = headers;

  // Hash post data
  sha256(post_data->data, post_data->length, tmp_hash);

  for (i = 0; i < 32; i++)
  {
    sprintf(post_hash + hash_pos, "%.2x", tmp_hash[i]);
    hash_pos += 2;
  }

  snprintf(headerbuf, sizeof(headerbuf), "x-amz-content-sha256:%.*s", 64,
           post_hash);
  headers = curl_slist_append(headers, headerbuf);

  if (source_bucket)
  {
    char *bucket_escape;
    char *key_escape;
    bucket_escape = curl_easy_escape(curl, source_bucket, (int)strlen(source_bucket));
    key_escape = curl_easy_escape(curl, source_key, (int)strlen(source_key));
    snprintf(headerbuf, sizeof(headerbuf), "x-amz-copy-source:/%s/%s",
             bucket_escape, key_escape);
    headers = curl_slist_append(headers, headerbuf);
    ms3_cfree(bucket_escape);
    ms3_cfree(key_escape);
  }

  // Date/time header
  time(&now);
  snprintf(headerbuf, sizeof(headerbuf), "x-amz-date:");
  offset = strlen(headerbuf);
  gmtime_r(&now, &tmp_tm);
  strftime(headerbuf + offset, sizeof(headerbuf) - offset, "%Y%m%dT%H%M%SZ",
           &tmp_tm);
  headers = curl_slist_append(headers, headerbuf);

  // Temp Credentials Security Token
  if (session_token)
  {
    snprintf(headerbuf, sizeof(headerbuf), "x-amz-security-token:%s",session_token);
    headers = curl_slist_append(headers, headerbuf);
    has_token = true;
  }

  if (source_bucket)
  {
    has_source = true;
  }

  // Builds the request hash
  if (protocol_version == 1)
  {
    ret = generate_request_hash(method, object, bucket, query, post_hash, headers,
                                has_source, has_token,
                                sha256hash);
  }
  else
  {
    ret = generate_request_hash(method, object, NULL, query, post_hash, headers,
                                has_source, has_token,
                                sha256hash);
  }

  if (ret)
  {
    return ret;
  }

  // User signing key hash
  // Date hashed using AWS4:secret_key
  snprintf(secrethead, sizeof(secrethead), "AWS4%.*s", 40, secret);
  strftime(headerbuf, sizeof(headerbuf), "%Y%m%d", &tmp_tm);
  hmac_sha256((uint8_t *)secrethead, strlen(secrethead), (uint8_t *)headerbuf,
              strlen(headerbuf), hmac_hash);

  // Region signed by above key
  hmac_sha256(hmac_hash, 32, (uint8_t *)region, strlen(region),
              hmac_hash2);

  // Service signed by above key (s3 always)
  sprintf(headerbuf, "s3");
  hmac_sha256(hmac_hash2, 32, (uint8_t *)headerbuf, strlen(headerbuf),
              hmac_hash);

  // Request version signed by above key (always "aws4_request")
  sprintf(headerbuf, "aws4_request");
  hmac_sha256(hmac_hash, 32, (uint8_t *)headerbuf, strlen(headerbuf),
              hmac_hash2);

  // Sign everything with the key
  snprintf(headerbuf, sizeof(headerbuf), "AWS4-HMAC-SHA256\n");
  offset = strlen(headerbuf);
  strftime(headerbuf + offset, sizeof(headerbuf) - offset, "%Y%m%dT%H%M%SZ\n",
           &tmp_tm);
  offset = strlen(headerbuf);
  strftime(date, 9, "%Y%m%d", &tmp_tm);
  snprintf(headerbuf + offset, sizeof(headerbuf) - offset,
           "%.*s/%s/s3/aws4_request\n%.*s", 8, date, region, 64, sha256hash);
  ms3debug("Data to sign: %s", headerbuf);
  hmac_sha256(hmac_hash2, 32, (uint8_t *)headerbuf, strlen(headerbuf),
              hmac_hash);

  hash_pos = 0;

  for (i = 0; i < 32; i++)
  {
    sprintf(sha256hash + hash_pos, "%.2x", hmac_hash[i]);
    hash_pos += 2;
  }

  // Make auth header
  if (source_bucket && session_token)
  {
    snprintf(headerbuf, sizeof(headerbuf),
             "Authorization: AWS4-HMAC-SHA256 Credential=%s/%s/%s/s3/aws4_request, SignedHeaders=host;x-amz-content-sha256;x-amz-copy-source;x-amz-date;x-amz-security-token;x-amz-copy-source, Signature=%s",
             key, date, region, sha256hash);
  }
  else if (source_bucket)
  {
    snprintf(headerbuf, sizeof(headerbuf),
             "Authorization: AWS4-HMAC-SHA256 Credential=%s/%s/%s/s3/aws4_request, SignedHeaders=host;x-amz-content-sha256;x-amz-copy-source;x-amz-date, Signature=%s",
             key, date, region, sha256hash);
  }
  else if (session_token)
  {
    snprintf(headerbuf, sizeof(headerbuf),
             "Authorization: AWS4-HMAC-SHA256 Credential=%s/%s/%s/s3/aws4_request, SignedHeaders=host;x-amz-content-sha256;x-amz-date;x-amz-security-token, Signature=%s",
             key, date, region, sha256hash);
  }
  else
  {
    snprintf(headerbuf, sizeof(headerbuf),
             "Authorization: AWS4-HMAC-SHA256 Credential=%s/%s/%s/s3/aws4_request, SignedHeaders=host;x-amz-content-sha256;x-amz-date, Signature=%s",
             key, date, region, sha256hash);
  }

  headers = curl_slist_append(headers, headerbuf);

  // Disable this header or PUT will barf with a 501
  sprintf(headerbuf, "Transfer-Encoding:");
  headers = curl_slist_append(headers, headerbuf);

  if ((method == MS3_PUT) && !source_bucket)
  {
    snprintf(headerbuf, sizeof(headerbuf), "Content-Length:%zu", post_data->length);
    headers = curl_slist_append(headers, headerbuf);
  }

  current_header = headers;

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
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
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
    if (!strncasecmp(buffer, "Last-Modified", 13))
    {
      ms3_status_st *status = (ms3_status_st *) userdata;
      // Date/time, format: Fri, 15 Mar 2019 16:58:54 GMT
      struct tm ttmp = {0};
      strptime(buffer + 15, "%a, %d %b %Y %H:%M:%S %Z", &ttmp);
      status->created = mktime(&ttmp);
    }
    else if (!strncasecmp(buffer, "Content-Length", 14))
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
  uint8_t *ptr;
  size_t realsize = nitems * size;

  struct memory_buffer_st *mem = (struct memory_buffer_st *)userdata;

  if (realsize + mem->length >= mem->alloced)
  {
    size_t additional_size = mem->buffer_chunk_size;

    if (realsize >= mem->buffer_chunk_size)
    {
      additional_size = (ceil((double)realsize / (double)mem->buffer_chunk_size) + 1)
                        * mem->buffer_chunk_size;
    }

    ptr = (uint8_t *)ms3_crealloc(mem->data, mem->alloced + additional_size);

    if (!ptr)
    {
      ms3debug("Curl response OOM");
      return 0;
    }

    mem->alloced += additional_size;
    mem->data = ptr;
  }

  memcpy(&(mem->data[mem->length]), buffer, realsize);
  mem->length += realsize;
  mem->data[mem->length] = '\0';

  ms3debug("Read %zu bytes, buffer %zu bytes", realsize, mem->length);
//  ms3debug("Data: %s", (char*)buffer);
  return nitems * size;
}

uint8_t execute_request(ms3_st *ms3, command_t cmd, const char *bucket,
                        const char *object, const char *source_bucket, const char *source_object,
                        const char *filter, const uint8_t *data, size_t data_size,
                        char *continuation,
                        void *ret_ptr)
{
  CURL *curl = NULL;
  struct curl_slist *headers = NULL;
  uint8_t res = 0;
  struct memory_buffer_st mem;
  uri_method_t method;
  char *path = NULL;
  char *query = NULL;
  struct put_buffer_st post_data;
  CURLcode curl_res;
  long response_code = 0;

  mem.data = NULL;
  mem.length = 0;
  mem.alloced = 1;
  mem.buffer_chunk_size = ms3->buffer_chunk_size;

  post_data.data = (uint8_t *) data;
  post_data.length = data_size;
  post_data.offset = 0;

  curl = ms3->curl;

  if (!ms3->first_run)
  {
    curl_easy_reset(curl);
  }
  else
  {
    ms3->first_run = false;
  }

  path = generate_path(curl, object, ms3->path_buffer);

  if (cmd == MS3_CMD_LIST_RECURSIVE)
  {
    query = generate_query(curl, filter, continuation, ms3->list_version, false,
                           ms3->query_buffer);
  }
  else if (cmd == MS3_CMD_LIST)
  {
    query = generate_query(curl, filter, continuation, ms3->list_version, true,
                           ms3->query_buffer);
  }

  res = build_request_uri(curl, ms3->base_domain, bucket, path, query,
                          ms3->use_http, ms3->protocol_version);

  if (res)
  {
    return res;
  }

  switch (cmd)
  {
    case MS3_CMD_COPY:
    case MS3_CMD_PUT:
      method = MS3_PUT;
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char *)data);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data_size);
      break;

    case MS3_CMD_DELETE:
      method = MS3_DELETE;
      break;

    case MS3_CMD_HEAD:
      method = MS3_HEAD;
      curl_easy_setopt(curl, CURLOPT_HEADERDATA, ret_ptr);
      break;

    case MS3_CMD_LIST:
    case MS3_CMD_LIST_RECURSIVE:
    case MS3_CMD_GET:
    case MS3_CMD_LIST_ROLE:
      method = MS3_GET;
      break;

    case MS3_CMD_ASSUME_ROLE:
    default:
      ms3debug("Bad cmd detected");
      ms3_cfree(mem.data);

      return MS3_ERR_IMPOSSIBLE;
  }

  if (ms3->iam_role)
  {
      ms3debug("Using assumed role: %s",ms3->iam_role);
      res = build_request_headers(curl, &headers, ms3->base_domain, ms3->region,
                                  ms3->role_key, ms3->role_secret, path, query, method, bucket, source_bucket,
                                  source_object, &post_data, ms3->protocol_version, ms3->role_session_token);
  }
  else
  {
      res = build_request_headers(curl, &headers, ms3->base_domain, ms3->region,
                                  ms3->s3key, ms3->s3secret, path, query, method, bucket, source_bucket,
                                  source_object, &post_data, ms3->protocol_version, NULL);
  }
  if (res)
  {
    ms3_cfree(mem.data);
    curl_slist_free_all(headers);

    return res;
  }

  if (ms3->disable_verification)
  {
    ms3debug("Disabling SSL verification");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
  }

  if (ms3->port)
    curl_easy_setopt(curl, CURLOPT_PORT, (long)ms3->port);

  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, body_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&mem);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
  curl_res = curl_easy_perform(curl);

  if (curl_res != CURLE_OK)
  {
    ms3debug("Curl error: %s", curl_easy_strerror(curl_res));
    set_error(ms3, curl_easy_strerror(curl_res));
    ms3_cfree(mem.data);
    curl_slist_free_all(headers);

    return MS3_ERR_REQUEST_ERROR;
  }

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
  ms3debug("Response code: %ld", response_code);

  if (response_code == 404)
  {
    char *message = parse_error_message((char *)mem.data, mem.length);

    if (message)
    {
      ms3debug("Response message: %s", message);
    }

    set_error_nocopy(ms3, message);
    res = MS3_ERR_NOT_FOUND;
  }
  else if (response_code == 403)
  {
    char *message = parse_error_message((char *)mem.data, mem.length);

    if (message)
    {
      ms3debug("Response message: %s", message);
    }

    set_error_nocopy(ms3, message);
    res = MS3_ERR_AUTH;
  }
  else if (response_code >= 400)
  {
    char *message = parse_error_message((char *)mem.data, mem.length);

    if (message)
    {
      ms3debug("Response message: %s", message);
    }

    set_error_nocopy(ms3, message);
    res = MS3_ERR_SERVER;
    if (ms3->iam_role)
    {
      res = MS3_ERR_AUTH_ROLE;
    }
  }

  switch (cmd)
  {
    case MS3_CMD_LIST_RECURSIVE:
    case MS3_CMD_LIST:
    {
      char *cont = NULL;
      parse_list_response((const char *)mem.data, mem.length, &ms3->list_container, ms3->list_version,
                          &cont);

      if (cont)
      {
        res = execute_request(ms3, cmd, bucket, object, source_bucket, source_object,
                              filter, data, data_size, cont,
                              NULL);
        if (res)
        {
          return res;
        }

        ms3_cfree(cont);
      }

      ms3_cfree(mem.data);
      break;
    }

    case MS3_CMD_COPY:
    case MS3_CMD_PUT:
    {
      ms3_cfree(mem.data);
      break;
    }

    case MS3_CMD_GET:
    {
      struct memory_buffer_st *buf = (struct memory_buffer_st *) ret_ptr;

      if (res)
      {
        ms3_cfree(mem.data);
      }
      else
      {
        buf->data = mem.data;
        buf->length = mem.length;
      }

      break;
    }

    case MS3_CMD_DELETE:
    {
      ms3_cfree(mem.data);
      break;
    }

    case MS3_CMD_HEAD:
    {
      ms3_cfree(mem.data);
      break;
    }

    case MS3_CMD_LIST_ROLE:
    case MS3_CMD_ASSUME_ROLE:
    default:
    {
      ms3_cfree(mem.data);
      ms3debug("Bad cmd detected");
      res = MS3_ERR_IMPOSSIBLE;
    }
  }

  curl_slist_free_all(headers);

  return res;
}
