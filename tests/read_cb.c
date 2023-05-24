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

#define NUM_BYTES 64 * 1024

/* Tests basic GET with a custom read callback */

static size_t read_cb(void *buf, size_t size, size_t nitems, void *userdata)
{
    char** dat = (char**)userdata;
    char* ptr = *dat;
    memcpy(ptr, buf, size * nitems);
    ptr += size * nitems;
    *dat = ptr;
    return nitems * size;
}

int main(int argc, char *argv[])
{
  int res;
  uint8_t *data;
  size_t length;
  ms3_st *ms3;
  char *test_string = malloc(NUM_BYTES);
  char *dest = malloc(NUM_BYTES);
  char* userdata = dest;
  char *s3key = getenv("S3KEY");
  char *s3secret = getenv("S3SECRET");
  char *s3region = getenv("S3REGION");
  char *s3bucket = getenv("S3BUCKET");
  char *s3host = getenv("S3HOST");
  char *s3noverify = getenv("S3NOVERIFY");
  char *s3usehttp = getenv("S3USEHTTP");
  char *s3port = getenv("S3PORT");

  SKIP_IF_(!s3key, "Environment variable S3KEY missing");
  SKIP_IF_(!s3secret, "Environment variable S3SECRET missing");
  SKIP_IF_(!s3region, "Environment variable S3REGION missing");
  SKIP_IF_(!s3bucket, "Environment variable S3BUCKET missing");

  (void) argc;
  (void) argv;

  memset(test_string, 'a', NUM_BYTES);
  memset(dest, 'b', NUM_BYTES);

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

  ASSERT_NOT_NULL(ms3);

  res = ms3_put(ms3, s3bucket, "test/read_cb.dat",
                (const uint8_t *)test_string,
                NUM_BYTES);
  ASSERT_EQ_(res, 0, "Result: %u", res);
  res = ms3_set_option(ms3, MS3_OPT_READ_CB, read_cb);
  ASSERT_EQ_(res, 0, "Result: %u", res);
  res = ms3_set_option(ms3, MS3_OPT_USER_DATA, &userdata);
  ASSERT_EQ_(res, 0, "Result: %u", res);
  length = 0;
  data = 0;
  res = ms3_get(ms3, s3bucket, "test/read_cb.dat", &data, &length);
  ASSERT_EQ_(res, 0, "Result: %u", res);
  ASSERT_EQ(data, 0);
  ASSERT_EQ(length, 0);
  ASSERT_EQ(memcmp(test_string, dest, NUM_BYTES), 0);
  res = ms3_delete(ms3, s3bucket, "test/read_cb.dat");
  ASSERT_EQ_(res, 0, "Result: %u", res);
  free(test_string);
  free(dest);
  ms3_free(data);
  ms3_deinit(ms3);
  ms3_library_deinit();
  return 0;
}
