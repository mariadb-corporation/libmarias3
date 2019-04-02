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
/* Test adds two files and checks that list prefix filters one out */

int main(int argc, char *argv[])
{
  (void) argc;
  (void) argv;
  int res;
  ms3_list_st *list = NULL, *list_it = NULL;
  const char *test_string = "Another one bites the dust";
  char *s3key = getenv("S3KEY");
  char *s3secret = getenv("S3SECRET");
  char *s3region = getenv("S3REGION");
  char *s3bucket = getenv("S3BUCKET");
  char *s3host = getenv("S3HOST");
  char *s3noverify = getenv("S3NOVERIFY");

  SKIP_IF_(!s3key, "Environemnt variable S3KEY missing");
  SKIP_IF_(!s3secret, "Environemnt variable S3SECRET missing");
  SKIP_IF_(!s3region, "Environemnt variable S3REGION missing");
  SKIP_IF_(!s3bucket, "Environemnt variable S3BUCKET missing");

  ms3_st *ms3 = ms3_thread_init(s3key, s3secret, s3region, s3host);

  if (s3noverify && !strcmp(s3noverify, "1"))
  {
    bool option = true;
    ms3_set_option(ms3, MS3_OPT_DISABLE_SSL_VERIFY, &option);
  }

//  ms3_debug(true);
  ASSERT_NOT_NULL(ms3);

  res = ms3_put(ms3, s3bucket, "test/prefix.txt", (const uint8_t *)test_string,
                strlen(test_string));
  ASSERT_EQ(res, 0);
  res = ms3_put(ms3, s3bucket, "other/prefix.txt", (const uint8_t *)test_string,
                strlen(test_string));
  ASSERT_EQ(res, 0);

  bool found_good = false;
  bool found_bad = false;

  for (int i = 0; i <= 3; i++)
  {
    res = ms3_list(ms3, s3bucket, "test", &list);
    ASSERT_EQ(res, 0);
    list_it = list;
    uint8_t file_count = 0;

    while (list_it)
    {
      if (!strncmp(list_it->key, "test/prefix.txt", 12))
      {
        found_good = true;
      }

      if (!strncmp(list_it->key, "other/prefix.txt", 12))
      {
        found_bad = true;
      }

      list_it = list_it->next;
      file_count++;
    }

    if ((file_count == 0) || !found_good)
    {
      sleep(1);
      printf("Bad file count, retrying");
      found_good = false;
      found_bad = false;
      ms3_list_free(list);
      continue;
    }
    else
    {
      break;
    }
  }

  ASSERT_EQ_(found_good, 1, "Created file not found");
  ASSERT_EQ_(found_bad, 0, "Filter found file it shouldn't");
  ms3_list_free(list);
  res = ms3_delete(ms3, s3bucket, "test/prefix.txt");
  ASSERT_EQ(res, 0);
  res = ms3_delete(ms3, s3bucket, "other/prefix.txt");
  ASSERT_EQ(res, 0);
  ms3_deinit(ms3);
}
