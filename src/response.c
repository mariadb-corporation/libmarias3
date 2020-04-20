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

#include "xml.h"

char *parse_error_message(const char *data, size_t length)
{
  struct xml_document *doc = NULL;
  struct xml_node *node = NULL;
  struct xml_node *root = NULL;

  uint64_t node_it = 0;

  if (!data || !length)
  {
    return NULL;
  }

  doc = xml_parse_document((uint8_t*)data, length);

  if (!doc)
  {
    return NULL;
  }

  root = xml_document_root(doc);

  if (!node)
  {
    xml_document_free(doc, false);
    return NULL;
  }

  // First node is Error
  node = xml_node_child(root, node_it);
  while(node)
  {
    if (!xml_node_name_cmp(node, "Message"))
    {
      struct xml_string *content = xml_node_content(node);
      uint8_t *message = ms3_cmalloc(xml_string_length(content) + 1);
      xml_string_copy(content, message, xml_string_length(content));
      xml_document_free(doc, false);
      return (char *)message;
    }

    node_it++;
    node = xml_node_child(root, node_it);
  }

  xml_document_free(doc, false);
  return NULL;
}

static ms3_list_st *get_next_list_ptr(struct ms3_list_container_st *container)
{
  ms3_list_st *new_alloc = NULL;
  struct ms3_pool_alloc_list_st *new_pool_next = NULL;
  struct ms3_pool_alloc_list_st *new_pool_prev = NULL;
  ms3_list_st *ret = NULL;
  if (container->pool_free == 0)
  {
    new_alloc = (ms3_list_st*)ms3_cmalloc(sizeof(ms3_list_st) * 1024);
    new_pool_next = (struct ms3_pool_alloc_list_st*)ms3_cmalloc(sizeof(struct ms3_pool_alloc_list_st));

    if (!new_alloc || !new_pool_next)
    {
        ms3debug("List realloc OOM");
        return NULL;
    }

    new_pool_prev = container->pool_list;
    container->pool_list = new_pool_next;
    if (new_pool_prev)
    {
      container->pool_list->prev = new_pool_prev;
    }
    else
    {
      container->pool_list->prev = NULL;
    }
    container->pool_list->pool = new_alloc;

    container->pool_free = 1024;
    if (!container->start)
    {
      container->start = new_alloc;
    }
    container->pool = container->next = new_alloc;
  }
  else
  {
    container->next++;
  }
  ret = container->next;
  container->pool_free--;
  return ret;
}

uint8_t parse_list_response(const char *data, size_t length, struct ms3_list_container_st *list_container,
                            uint8_t list_version,
                            char **continuation)
{
  struct xml_document *doc;
  struct xml_node *root;
  struct xml_node *node;
  struct xml_node *child;
  char *filename = NULL;
  char *filesize = NULL;
  char *filedate = NULL;
  size_t size = 0;
  struct tm ttmp = {0};
  time_t tout = 0;
  bool truncated = false;
  const char *last_key = NULL;
  ms3_list_st *nextptr = NULL, *lastptr = list_container->next;
  uint64_t node_it = 0;

  // Empty list
  if (!data || !length)
  {
    return 0;
  }

  doc = xml_parse_document((uint8_t*)data, length);

  if (!doc)
  {
    return MS3_ERR_RESPONSE_PARSE;
  }

  /* For version 1:
   * If IsTruncated is set, get the last key in the list, this will be used as
   * "marker" in the next request.
   * For version 2:
   * If NextContinuationToken is set, use this for the next request
   *
   * We use the "continuation" return value for both
   */

  root = xml_document_root(doc);
  // First node is ListBucketResponse
  node = xml_node_child(root, 0);

  do
  {
    if (!xml_node_name_cmp(node, "NextContinuationToken"))
    {
      struct xml_string *content = xml_node_content(node);
      *continuation = ms3_cmalloc(xml_string_length(content) + 1);
      xml_string_copy(content, (uint8_t*)*continuation, xml_string_length(content));
      continue;
    }

    if (list_version == 1)
    {
      if (!xml_node_name_cmp(node, "IsTruncated"))
      {
        struct xml_string *content = xml_node_content(node);
        char *trunc_value = ms3_cmalloc(xml_string_length(content) + 1);
        xml_string_copy(content, (uint8_t*)trunc_value, xml_string_length(content));

        if (!strcmp(trunc_value, "true"))
        {
          truncated = true;
        }

        ms3_cfree(trunc_value);
        continue;
      }
    }

    if (!xml_node_name_cmp(node, "Contents"))
    {
      bool skip = false;
      uint64_t child_it = 0;
      // Found contents
      child = xml_node_child(node, 0);

      do
      {
        if (!xml_node_name_cmp(child, "Key"))
        {
          struct xml_string *content = xml_node_content(child);
          filename = ms3_cmalloc(xml_string_length(content) + 1);
          xml_string_copy(content, (uint8_t*)filename, xml_string_length(content));

          ms3debug("Filename: %s", filename);

          if (filename[strlen((const char *)filename) - 1] == '/')
          {
            skip = true;
            ms3_cfree(filename);
            break;
          }

          continue;
        }

        if (!xml_node_name_cmp(child, "Size"))
        {
          struct xml_string *content = xml_node_content(child);
          filesize = ms3_cmalloc(xml_string_length(content) + 1);
          xml_string_copy(content, (uint8_t*)filesize, xml_string_length(content));

          ms3debug("Size: %s", filesize);
          size = strtoull((const char *)filesize, NULL, 10);
          ms3_cfree(filesize);
          continue;
        }

        if (!xml_node_name_cmp(child, "LastModified"))
        {
          struct xml_string *content = xml_node_content(child);
          filedate = ms3_cmalloc(xml_string_length(content) + 1);
          xml_string_copy(content, (uint8_t*)filedate, xml_string_length(content));

          ms3debug("Date: %s", filedate);
          strptime((const char *)filedate, "%Y-%m-%dT%H:%M:%SZ", &ttmp);
          tout = mktime(&ttmp);
          ms3_cfree(filedate);
          continue;
        }
      }
      while ((child = xml_node_child(node, ++child_it)));

      if (!skip)
      {
        nextptr = get_next_list_ptr(list_container);
        nextptr->next = NULL;

        if (lastptr)
        {
          lastptr->next = nextptr;
        }
        lastptr = nextptr;

        if (filename)
        {
          nextptr->key = (char *)filename;

          if (list_version == 1)
          {
            last_key = nextptr->key;
          }
        }
        else
        {
          nextptr->key = NULL;
        }

        nextptr->length = size;
        nextptr->created = tout;
      }

      continue;
    }

    if (!xml_node_name_cmp(node, "CommonPrefixes"))
    {
      child = xml_node_child(node, 0);

      if (!xml_node_name_cmp(child, "Prefix"))
      {
        struct xml_string *content = xml_node_content(child);
        filename = ms3_cmalloc(xml_string_length(content) + 1);
        xml_string_copy(content, (uint8_t*)filename, xml_string_length(content));

        ms3debug("Filename: %s", filename);
        nextptr = get_next_list_ptr(list_container);
        nextptr->next = NULL;

        if (lastptr)
        {
          lastptr->next = nextptr;
        }
        lastptr = nextptr;

        nextptr->key = (char *)filename;
        nextptr->length = 0;
        nextptr->created = 0;
      }
    }

  }
  while ((node = xml_node_child(root, ++node_it)));

  if (list_version == 1 && truncated && last_key)
  {
    *continuation = ms3_cstrdup(last_key);
  }

  xml_document_free(doc, false);
  return 0;
}
