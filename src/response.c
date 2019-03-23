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

uint8_t parse_list_response(const char *data, size_t length, ms3_list_st **list)
{
  xmlDocPtr doc;
  xmlNodePtr node;
  xmlNodePtr child;
  xmlChar *filename = NULL;
  xmlChar *filesize = NULL;
  xmlChar *filedate = NULL;
  size_t size = 0;
  struct tm ttmp;
  time_t tout = 0;
  ms3_list_st *nextptr = NULL, *lastptr = NULL;

  doc = xmlReadMemory(data, (int)length, "noname.xml", NULL, 0);

  if (not doc)
  {
    return MS3_ERR_RESPONSE_PARSE;
  }

  node = xmlDocGetRootElement(doc);
  // First node is ListBucketResponse
  node = node->xmlChildrenNode;

  while (node)
  {
    if (not xmlStrcmp(node->name, (const unsigned char *)"Contents"))
    {
      bool skip = false;
      // Found contents
      child = node->xmlChildrenNode;

      do
      {
        if (not xmlStrcmp(child->name, (const unsigned char *)"Key"))
        {
          filename = xmlNodeGetContent(child);
          ms3debug("Filename: %s", filename);

          if (filename[strlen((const char *)filename) - 1] == '/')
          {
            skip = true;
            break;
          }
        }

        if (not xmlStrcmp(child->name, (const unsigned char *)"Size"))
        {
          filesize = xmlNodeGetContent(child);
          ms3debug("Size: %s", filesize);
          size = strtoull((const char *)filesize, NULL, 10);
        }

        if (not xmlStrcmp(child->name, (const unsigned char *)"LastModified"))
        {
          filedate = xmlNodeGetContent(child);
          ms3debug("Date: %s", filedate);
          strptime((const char *)filedate, "%Y-%m-%dT%H:%M:%SZ", &ttmp);
          tout = mktime(&ttmp);
        }
      }
      while ((child = child->next));

      if (not skip)
      {
        nextptr = malloc(sizeof(ms3_list_st));
        nextptr->next = NULL;

        if (not lastptr)
        {
          *list = nextptr;
          lastptr = nextptr;
        }
        else
        {
          lastptr->next = nextptr;
          lastptr = nextptr;
        }

        nextptr->key = strdup((const char *)filename);
        nextptr->length = size;
        nextptr->created = tout;
      }

    }

    node = node->next;
  }

  xmlFreeDoc(doc);
  return 0;
}
