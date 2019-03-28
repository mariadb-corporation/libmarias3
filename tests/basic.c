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

#include <yatl/lite.h>
#include <libmarias3/marias3.h>

#include <unistd.h>
/* Tests basic put, list, get, status, delete */

int main(int argc, char *argv[])
{
  (void) argc;
  (void) argv;
  int res;
  ms3_list_st *list = NULL, *list_it = NULL;
  uint8_t *data;
  size_t length;
  const char *test_string = "Another one bites the dust";
  ms3_status_st status;
  char *s3key = getenv("S3KEY");
  char *s3secret = getenv("S3SECRET");
  char *s3region = getenv("S3REGION");
  char *s3bucket = getenv("S3BUCKET");
  char *s3host = getenv("S3HOST");

  SKIP_IF_(!s3key, "Environemnt variable S3KEY missing");
  SKIP_IF_(!s3secret, "Environemnt variable S3SECRET missing");
  SKIP_IF_(!s3region, "Environemnt variable S3REGION missing");
  SKIP_IF_(!s3bucket, "Environemnt variable S3BUCKET missing");

  ms3_st *ms3 = ms3_init(s3key, s3secret, s3region, s3host);

//  ms3_debug(true);
  ASSERT_NOT_NULL(ms3);

  res = ms3_put(ms3, s3bucket, "test/basic.txt", (const uint8_t *)test_string,
                strlen(test_string));
  ASSERT_EQ_(res, 0, "Result: %u", res);

  // A prefix that will give no results;
  res = ms3_list(ms3, s3bucket, "asdfghjkl", &list);
  ASSERT_EQ_(res, 0, "Result: %u", res);
  ASSERT_NULL_(list, "List not empty");

  bool found = false;

  for (int i = 0; i <= 1; i++)
  {
    res = ms3_list(ms3, s3bucket, NULL, &list);
    ASSERT_EQ_(res, 0, "Result: %u", res);
    list_it = list;

    while (list_it)
    {
      if (!strncmp(list_it->key, "test/basic.txt", 12))
      {
        found = true;
        break;
      }

      list_it = list_it->next;
    }

    if (!found)
    {
      sleep(1);
      printf("List can't see file, retrying\n");
      ms3_list_free(list);
      list = NULL;
    }
    else
    {
      break;
    }
  }

  ASSERT_EQ_(found, 1, "Created file not found");

  if (list_it)
  {
    ASSERT_EQ_(list_it->length, 26, "Created file is unexpected length");
    ASSERT_NEQ_(list_it->created, 0, "Created file timestamp is bad");
  }
  else
  {
    ASSERT_TRUE_(false, "No resuts from list");
  }

  ms3_list_free(list);

  for (int i = 0; i <= 3; i++)
  {
    res = ms3_get(ms3, s3bucket, "test/basic.txt", &data, &length);

    if (res == MS3_ERR_NOT_FOUND)
    {
      sleep(1);
      printf("Not found in get, looping\n");
      continue;
    }

    ASSERT_EQ_(res, 0, "Result: %u", res);

    if (res == 0)
    {
      break;
    }
  }

  ASSERT_EQ(length, 26);
  ASSERT_STREQ((char *)data, test_string);

  for (int i = 0; i <= 3; i++)
  {
    res = ms3_status(ms3, s3bucket, "test/basic.txt", &status);

    if (res == MS3_ERR_NOT_FOUND)
    {
      sleep(1);
      printf("Not found in status, looping\n");
      continue;
    }

    ASSERT_EQ_(res, 0, "Result: %u", res);

    if (res == 0)
    {
      break;
    }
  }

  ASSERT_EQ(status.length, 26);
  ASSERT_NEQ(status.created, 0);
  res = ms3_delete(ms3, s3bucket, "test/basic.txt");
  ASSERT_EQ_(res, 0, "Result: %u", res);
  ms3_free(data);
  ms3_deinit(ms3);
}
