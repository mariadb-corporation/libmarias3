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

#include <libxml/parser.h>
#include <libxml/tree.h>

char *parse_error_message(const char *data, size_t length)
{
  xmlDocPtr doc;
  xmlNodePtr node;
  xmlChar *message = NULL;

  if (!data || !length)
  {
    return NULL;
  }

  doc = xmlReadMemory(data, (int)length, "noname.xml", NULL, 0);

  if (!doc)
  {
    return NULL;
  }

  node = xmlDocGetRootElement(doc);
  // First node is Error
  node = node->xmlChildrenNode;

  while (node)
  {
    if (!xmlStrcmp(node->name, (const unsigned char *)"Message"))
    {
      message = xmlNodeGetContent(node);
      xmlFreeDoc(doc);
      return (char *)message;
    }

    node = node->next;
  }

  xmlFreeDoc(doc);
  return NULL;
}

uint8_t parse_list_response(const char *data, size_t length, ms3_list_st **list,
                            uint8_t list_version,
                            char **continuation)
{
  xmlDocPtr doc;
  xmlNodePtr node;
  xmlNodePtr child;
  xmlChar *filename = NULL;
  xmlChar *filesize = NULL;
  xmlChar *filedate = NULL;
  size_t size = 0;
  struct tm ttmp = {0};
  time_t tout = 0;
  bool truncated = false;
  const char *last_key = NULL;
  ms3_list_st *nextptr = NULL, *lastptr = NULL;

  // Empty list
  if (!data || !length)
  {
    return 0;
  }

  doc = xmlReadMemory(data, (int)length, "noname.xml", NULL, 0);

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

  node = xmlDocGetRootElement(doc);
  // First node is ListBucketResponse
  node = node->xmlChildrenNode;

  while (node)
  {
    if (!xmlStrcmp(node->name, (const unsigned char *)"NextContinuationToken"))
    {
      *continuation = (char *)xmlNodeGetContent(node);
    }

    if (list_version == 1)
    {
      if (!xmlStrcmp(node->name, (const unsigned char *)"IsTruncated"))
      {
        xmlChar *trunc_value = xmlNodeGetContent(node);

        if (!xmlStrcmp(trunc_value, (const unsigned char *)"true"))
        {
          truncated = true;
        }

        xmlFree(trunc_value);
      }
    }

    if (!xmlStrcmp(node->name, (const unsigned char *)"Contents"))
    {
      bool skip = false;
      // Found contents
      child = node->xmlChildrenNode;

      do
      {
        if (!xmlStrcmp(child->name, (const unsigned char *)"Key"))
        {
          filename = xmlNodeGetContent(child);
          ms3debug("Filename: %s", filename);

          if (filename[strlen((const char *)filename) - 1] == '/')
          {
            skip = true;
            xmlFree(filename);
            break;
          }
        }

        if (!xmlStrcmp(child->name, (const unsigned char *)"Size"))
        {
          filesize = xmlNodeGetContent(child);
          ms3debug("Size: %s", filesize);
          size = strtoull((const char *)filesize, NULL, 10);
          xmlFree(filesize);
        }

        if (!xmlStrcmp(child->name, (const unsigned char *)"LastModified"))
        {
          filedate = xmlNodeGetContent(child);
          ms3debug("Date: %s", filedate);
          strptime((const char *)filedate, "%Y-%m-%dT%H:%M:%SZ", &ttmp);
          tout = mktime(&ttmp);
          xmlFree(filedate);
        }
      }
      while ((child = child->next));

      if (!skip)
      {
        nextptr = ms3_cmalloc(sizeof(ms3_list_st));
        nextptr->next = NULL;

        if (!lastptr)
        {
          *list = nextptr;
          lastptr = nextptr;
        }
        else
        {
          lastptr->next = nextptr;
          lastptr = nextptr;
        }

        if (filename)
        {
          nextptr->key = ms3_cstrdup((const char *)filename);

          if (list_version == 1)
          {
            last_key = nextptr->key;
          }

          xmlFree(filename);
        }
        else
        {
          nextptr->key = NULL;
        }

        nextptr->length = size;
        nextptr->created = tout;
      }

    }

    node = node->next;
  }

  if (list_version == 1 && truncated && last_key)
  {
    *continuation = ms3_cstrdup(last_key);
  }

  xmlFreeDoc(doc);
  return 0;
}
