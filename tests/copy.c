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

/* Tests copy and move calls */

int main(int argc, char *argv[])
{
  int res;
  ms3_list_st *list = NULL, *list_it = NULL;
  uint8_t *data;
  size_t length;
  bool found, found_orig, found_new;
  ms3_st *ms3;
  const char *test_string = "Another one bites the dust";
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

  res = ms3_put(ms3, s3bucket, "test/copy_test.txt",
                (const uint8_t *)test_string,
                strlen(test_string));
  ASSERT_EQ_(res, 0, "Result: %u", res);

  res = ms3_copy(ms3, s3bucket, "test/copy_test.txt", s3bucket,
                 "test/copied.txt");
  ASSERT_EQ_(res, 0, "Result: %u", res);

  res = ms3_list(ms3, s3bucket, NULL, &list);
  ASSERT_EQ_(res, 0, "Result: %u", res);
  found = false;
  list_it = list;

  while (list_it)
  {
    if (!strncmp(list_it->key, "test/copied.txt", 12))
    {
      found = true;
      break;
    }

    list_it = list_it->next;
  }

  ASSERT_EQ_(found, 1, "Copied file not found");

  // Test with hash chars in filename

  res = ms3_put(ms3, s3bucket, "test/copy_###_test.txt",
                (const uint8_t *)test_string,
                strlen(test_string));
  ASSERT_EQ_(res, 0, "Result: %u", res);

  res = ms3_copy(ms3, s3bucket, "test/copy_###_test.txt", s3bucket,
                 "test/copied###.txt");
  ASSERT_EQ_(res, 0, "Result: %u", res);

  res = ms3_list(ms3, s3bucket, NULL, &list);
  ASSERT_EQ_(res, 0, "Result: %u", res);
  found = false;
  list_it = list;

  while (list_it)
  {
    if (!strncmp(list_it->key, "test/copied###.txt", 12))
    {
      found = true;
      break;
    }

    list_it = list_it->next;
  }

  ASSERT_EQ_(found, 1, "Copied file not found");

  if (list_it)
  {
    ASSERT_EQ_(list_it->length, 26, "Copied file is unexpected length");
    ASSERT_NEQ_(list_it->created, 0, "Copied file timestamp is bad");
  }
  else
  {
    ASSERT_TRUE_(false, "No resuts from list");
  }

  res = ms3_move(ms3, s3bucket, "test/copy_test.txt", s3bucket, "test/moved.txt");
  ASSERT_EQ_(res, 0, "Result: %u", res);

  res = ms3_list(ms3, s3bucket, NULL, &list);
  ASSERT_EQ_(res, 0, "Result: %u", res);
  found_orig = false;
  found_new = false;
  list_it = list;

  while (list_it)
  {
    if (!strncmp(list_it->key, "test/moved.txt", 12))
    {
      found_new = true;
    }
    else if (!strncmp(list_it->key, "test/copy_test.txt", 12))
    {
      found_orig = true;
    }


    list_it = list_it->next;
  }

  ASSERT_EQ_(found_new, 1, "Copied file not found");
  ASSERT_EQ_(found_orig, 0, "Original file still exists after move");

  res = ms3_get(ms3, s3bucket, "test/moved.txt", &data, &length);
  ASSERT_EQ_(res, 0, "Result: %u", res);
  ASSERT_EQ(length, 26);
  ASSERT_STREQ((char *)data, test_string);

  res = ms3_delete(ms3, s3bucket, "test/moved.txt");
  ASSERT_EQ_(res, 0, "Result: %u", res);
  res = ms3_delete(ms3, s3bucket, "test/copied.txt");
  res = ms3_delete(ms3, s3bucket, "test/copy_###_test.txt");
  res = ms3_delete(ms3, s3bucket, "test/copied###.txt");

  ms3_free(data);
  ms3_deinit(ms3);
  ms3_library_deinit();
  return 0;
}
