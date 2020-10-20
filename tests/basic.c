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

/* Tests basic put, list, get, status, delete using the thread calls */

int main(int argc, char *argv[])
{
  int res;
  ms3_list_st *list = NULL, *list_it = NULL;
  uint8_t *data;
  size_t length;
  int i;
  bool found;
  uint8_t list_version;
  const char *test_string = "Another one bites the dust";
  ms3_status_st status;
  ms3_st *ms3;
  char *s3key = getenv("S3KEY");
  char *s3secret = getenv("S3SECRET");
  char *s3region = getenv("S3REGION");
  char *s3bucket = getenv("S3BUCKET");
  char *s3host = getenv("S3HOST");
  char *s3noverify = getenv("S3NOVERIFY");
  char *s3usehttp = getenv("S3USEHTTP");
  char *s3port = getenv("S3PORT");

  SKIP_IF_(!s3key, "Environemnt variable S3KEY missing");
  SKIP_IF_(!s3secret, "Environemnt variable S3SECRET missing");
  SKIP_IF_(!s3region, "Environemnt variable S3REGION missing");
  SKIP_IF_(!s3bucket, "Environemnt variable S3BUCKET missing");

  (void) argc;
  (void) argv;

  ms3_library_init();
  ms3 = ms3_init(s3key, s3secret, s3region, s3host);

  if (s3noverify && !strcmp(s3noverify, "1"))
  {
    ms3_set_option(ms3, MS3_OPT_DISABLE_SSL_VERIFY, NULL);
  }

  if (s3usehttp && !strcmp(s3usehttp, "1"))
  {
    ms3_set_option(ms3, MS3_OPT_USE_HTTP, NULL);
  }

  if (s3port)
  {
    int port = atol(s3port);
    ms3_set_option(ms3, MS3_OPT_PORT_NUMBER, &port);
  }

//  ms3_debug();
  ASSERT_NOT_NULL(ms3);

  res = ms3_put(ms3, s3bucket, "test/basic_thread.txt",
                (const uint8_t *)test_string,
                strlen(test_string));
  ASSERT_EQ_(res, 0, "Result: %u", res);

  // A prefix that will give no results;
  res = ms3_list(ms3, s3bucket, "asdfghjkl", &list);
  ASSERT_EQ_(res, 0, "Result: %u", res);
  ASSERT_NULL_(list, "List not empty");

  res = ms3_list(ms3, s3bucket, NULL, &list);
  ASSERT_EQ_(res, 0, "Result: %u", res);
  found = false;
  list_it = list;

  while (list_it)
  {
    if (!strncmp(list_it->key, "test/basic_thread.txt", 21))
    {
      found = true;
      break;
    }

    list_it = list_it->next;
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

  // Retry list with V1 API
  list_version = 1;
  list = NULL;
  ms3_set_option(ms3, MS3_OPT_FORCE_LIST_VERSION, &list_version);
  res = ms3_list(ms3, s3bucket, NULL, &list);
  ASSERT_EQ_(res, 0, "Result: %u", res);
  found = false;
  list_it = list;

  while (list_it)
  {
    if (!strncmp(list_it->key, "test/basic_thread.txt", 21))
    {
      found = true;
      break;
    }

    list_it = list_it->next;
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

  res = ms3_get(ms3, s3bucket, "test/basic_thread.txt", &data, &length);
  ASSERT_EQ_(res, 0, "Result: %u", res);
  ASSERT_EQ(length, 26);
  ASSERT_STREQ((char *)data, test_string);

  for (i = 0; i <= 3; i++)
  {
    res = ms3_status(ms3, s3bucket, "test/basic_thread.txt", &status);

    if (res == MS3_ERR_NOT_FOUND)
    {
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
  res = ms3_delete(ms3, s3bucket, "test/basic_thread.txt");
  ASSERT_EQ_(res, 0, "Result: %u", res);
  ms3_free(data);
  res = ms3_get(ms3, s3bucket, "test/basic_thread.txt", &data, &length);
  ASSERT_NEQ_(res, 0, "Object should error");
  ASSERT_NULL_(data, "Data should be NULL");
  ASSERT_EQ_(length, 0, "There should be no data");
  ms3_deinit(ms3);
  ms3_library_deinit();
  return 0;
}
