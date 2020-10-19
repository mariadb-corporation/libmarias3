/* vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * Copyright 2020 MariaDB Corporation Ab. All rights reserved.
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

const char *default_iam_domain = "iam.amazonaws.com";
const char *default_sts_domain = "sts.amazonaws.com";
const char *iam_request_region = "us-east-1";

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

static uint8_t build_assume_role_request_uri(CURL *curl, const char *base_domain, const char *query, bool use_http)
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
    domain = default_sts_domain;
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
    if (path_parts + strlen(domain) + strlen(query) >= MAX_URI_LENGTH - 1)
    {
      return MS3_ERR_URI_TOO_LONG;
    }

    snprintf(uri_buffer, MAX_URI_LENGTH - 1, "%s://%s/?%s", protocol,
             domain, query);
  }
  else
  {
      return MS3_ERR_PARAMETER;
  }

  ms3debug("URI: %s", uri_buffer);
  curl_easy_setopt(curl, CURLOPT_URL, uri_buffer);
  return 0;
}

static char *generate_assume_role_query(CURL *curl, const char *action, size_t role_duration,
                            const char *version, const char *role_session_name, const char *role_arn,
                            const char *continuation, char *query_buffer)
{
  size_t query_buffer_length = 0;
  char *encoded;
  query_buffer[0] = '\0';

  if (action)
  {
    encoded = curl_easy_escape(curl, action, (int)strlen(action));
    query_buffer_length = strlen(query_buffer);
    if (query_buffer_length)
    {
      snprintf(query_buffer + query_buffer_length, 3072 - query_buffer_length,
               "&Action=%s", encoded);
    }
    else
    {
      snprintf(query_buffer, 3072, "Action=%s", encoded);
    }
    curl_free(encoded);
  }
  if (role_duration >= 900 && role_duration <= 43200)
  {
    query_buffer_length = strlen(query_buffer);
    if (query_buffer_length)
    {
      snprintf(query_buffer + query_buffer_length, 3072 - query_buffer_length,
               "&DurationSeconds=%zu", role_duration);
    }
    else
    {
      snprintf(query_buffer, 3072, "DurationSeconds=%zu", role_duration);
    }
  }
  if (continuation)
  {
      encoded = curl_easy_escape(curl, continuation, (int)strlen(continuation));
      query_buffer_length = strlen(query_buffer);
      if (query_buffer_length)
      {
        snprintf(query_buffer + query_buffer_length, 3072 - query_buffer_length,
                 "&Marker=%s", encoded);
      }
      else
      {
        snprintf(query_buffer, 3072, "Marker=%s", encoded);
      }
      curl_free(encoded);
  }
  if (role_arn)
  {
    encoded = curl_easy_escape(curl, role_arn, (int)strlen(role_arn));
    query_buffer_length = strlen(query_buffer);
    if (query_buffer_length)
    {
      snprintf(query_buffer + query_buffer_length, 3072 - query_buffer_length,
               "&RoleArn=%s", encoded);
    }
    else
    {
      snprintf(query_buffer, 3072, "RoleArn=%s", encoded);
    }
    curl_free(encoded);
  }
  if (role_session_name)
  {
    encoded = curl_easy_escape(curl, role_session_name, (int)strlen(role_session_name));
    query_buffer_length = strlen(query_buffer);
    if (query_buffer_length)
    {
      snprintf(query_buffer + query_buffer_length, 3072 - query_buffer_length,
               "&RoleSessionName=%s", encoded);
    }
    else
    {
      snprintf(query_buffer, 3072, "RoleSessionName=%s", encoded);
    }
    curl_free(encoded);
  }
  if (version)
  {
    encoded = curl_easy_escape(curl, version, (int)strlen(version));
    query_buffer_length = strlen(query_buffer);
    if (query_buffer_length)
    {
      snprintf(query_buffer + query_buffer_length, 3072 - query_buffer_length,
               "&Version=%s", encoded);
    }
    else
    {
      snprintf(query_buffer, 3072, "Version=%s", encoded);
    }
    curl_free(encoded);
  }

  return query_buffer;
}


static uint8_t generate_assume_role_request_hash(uri_method_t method, const char *query, char *post_hash,
                                        struct curl_slist *headers, char *return_hash)
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

  // URL query (if exists)
  if (query)
  {
    snprintf(signing_data + pos, sizeof(signing_data) - pos, "/\n%s\n", query);
    pos += strlen(query) + 3;
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
  snprintf(signing_data + pos, sizeof(signing_data) - pos,
             "\nhost;x-amz-content-sha256;x-amz-date\n");
  pos += 38;

  // Hash of post data (can be hash of empty)
  snprintf(signing_data + pos, sizeof(signing_data) - pos, "%.*s", 64, post_hash);
  //pos+= 64;

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

static uint8_t
build_assume_role_request_headers(CURL *curl, struct curl_slist **head,
                                  const char *base_domain,
                                  const char* endpoint_type,
                                  const char *region, const char *key,
                                  const char *secret, const char *query,
                                  uri_method_t method,
                                  struct put_buffer_st *post_data)
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
  const char *type;
  struct curl_slist *headers = NULL;
  uint8_t offset;
  uint8_t i;
  struct curl_slist *current_header;

  // Host header
  if (base_domain)
  {
    domain = base_domain;
  }
  else
  {
    domain = default_sts_domain;
  }

  if (endpoint_type)
  {
      type = endpoint_type;
  }
  else
  {
      type = "sts";
  }

  snprintf(headerbuf, sizeof(headerbuf), "host:%s", domain);

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

  // Date/time header
  time(&now);
  snprintf(headerbuf, sizeof(headerbuf), "x-amz-date:");
  offset = strlen(headerbuf);
  gmtime_r(&now, &tmp_tm);
  strftime(headerbuf + offset, sizeof(headerbuf) - offset, "%Y%m%dT%H%M%SZ",
           &tmp_tm);
  headers = curl_slist_append(headers, headerbuf);

  // Builds the request hash
  ret = generate_assume_role_request_hash(method, query, post_hash, headers, sha256hash);

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

  // Service signed by above key
  hmac_sha256(hmac_hash2, 32, (uint8_t *)type, strlen(type),
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
           "%.*s/%s/%s/aws4_request\n%.*s", 8, date, region, type, 64, sha256hash);
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
    snprintf(headerbuf, sizeof(headerbuf),
             "Authorization: AWS4-HMAC-SHA256 Credential=%s/%s/%s/%s/aws4_request, SignedHeaders=host;x-amz-content-sha256;x-amz-date, Signature=%s",
             key, date, region, type, sha256hash);

  headers = curl_slist_append(headers, headerbuf);

  // Disable this header or PUT will barf with a 501
  sprintf(headerbuf, "Transfer-Encoding:");
  headers = curl_slist_append(headers, headerbuf);

  current_header = headers;

  do
  {
    ms3debug("Header: %s", current_header->data);
  }
  while ((current_header = current_header->next));

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  return 0;
}

uint8_t execute_assume_role_request(ms3_st *ms3, command_t cmd,
                                    const uint8_t *data, size_t data_size,
                                    char *continuation)
{
  CURL *curl = NULL;
  struct curl_slist *headers = NULL;
  uint8_t res = 0;
  struct memory_buffer_st mem;
  uri_method_t method;
  char *query = NULL;
  struct put_buffer_st post_data;
  CURLcode curl_res;
  long response_code = 0;
  char* endpoint = NULL;
  const char* region = iam_request_region;
  char endpoint_type[8];

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

  if (cmd == MS3_CMD_ASSUME_ROLE)
  {
      query = generate_assume_role_query(curl, "AssumeRole", ms3->role_session_duration, "2011-06-15", "libmariaS3",
                                         ms3->iam_role_arn, continuation, ms3->query_buffer);
      endpoint = ms3->sts_endpoint;
      region = ms3->sts_region;
      sprintf(endpoint_type, "sts");
      method = MS3_GET;
  }
  else if (cmd == MS3_CMD_LIST_ROLE)
  {
      query = generate_assume_role_query(curl, "ListRoles", 0, "2010-05-08", NULL, NULL, continuation, ms3->query_buffer);
      endpoint = ms3->iam_endpoint;
      sprintf(endpoint_type, "iam");
      method = MS3_GET;
  }

  res = build_assume_role_request_uri(curl, endpoint, query, ms3->use_http);

  if (res)
  {
    return res;
  }

  res = build_assume_role_request_headers(curl, &headers, endpoint,
                                          endpoint_type, region,
                                          ms3->s3key, ms3->s3secret, query,
                                          method, &post_data);

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
  }

  switch (cmd)
   {
     case MS3_CMD_LIST_ROLE:
     {
       char *cont = NULL;
       res = parse_role_list_response((const char *)mem.data, mem.length, ms3->iam_role ,ms3->iam_role_arn, &cont);

       if (cont && res)
       {
         res = execute_assume_role_request(ms3, cmd, data, data_size, cont);
         if (res)
         {
           ms3_cfree(cont);
           ms3_cfree(mem.data);
           curl_slist_free_all(headers);
           return res;
         }
         ms3_cfree(cont);
       }

       ms3_cfree(mem.data);
       break;
     }

     case MS3_CMD_ASSUME_ROLE:
     {
       if (res)
       {
         ms3_cfree(mem.data);
         curl_slist_free_all(headers);
         return res;
       }
       res = parse_assume_role_response((const char *)mem.data, mem.length, ms3->role_key, ms3->role_secret, ms3->role_session_token);
       ms3_cfree(mem.data);
       break;
     }

     case MS3_CMD_LIST:
     case MS3_CMD_LIST_RECURSIVE:
     case MS3_CMD_PUT:
     case MS3_CMD_GET:
     case MS3_CMD_DELETE:
     case MS3_CMD_HEAD:
     case MS3_CMD_COPY:
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
