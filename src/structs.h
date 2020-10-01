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

struct ms3_pool_alloc_list_st
{
  struct ms3_list_st *pool;
  struct ms3_pool_alloc_list_st *prev;
};

struct ms3_list_container_st
{
  struct ms3_list_st *pool;
  struct ms3_list_st *start;
  struct ms3_pool_alloc_list_st *pool_list;
  struct ms3_list_st *next;
  size_t pool_free;
};

struct ms3_st
{
  char *s3key;
  char *s3secret;
  char *region;
  char *base_domain;
  int port; // 0 means "Use default"

  char *sts_endpoint;
  char *sts_region;
  char *iam_endpoint;
  char *iam_role;
  char *role_key;
  char *role_secret;
  char *role_session_token;
  char *iam_role_arn;
  size_t role_session_duration;

  size_t buffer_chunk_size;
  CURL *curl;
  char *last_error;
  bool use_http;
  bool disable_verification;
  uint8_t list_version;
  uint8_t protocol_version;
  bool first_run;
  char *path_buffer;
  char *query_buffer;
  struct ms3_list_container_st list_container;
};

struct memory_buffer_st
{
  uint8_t *data;
  size_t length;
  size_t alloced;
  size_t buffer_chunk_size;
};

struct put_buffer_st
{
  const uint8_t *data;
  size_t length;
  size_t offset;
};
