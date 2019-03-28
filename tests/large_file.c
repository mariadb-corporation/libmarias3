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

/* Tests basic PUT/GET a 64MB file */

int main(int argc, char *argv[])
{
  (void) argc;
  (void) argv;
  int res;
  uint8_t *data;
  size_t length;
  char *test_string = malloc(64 * 1024 * 1024);
  memset(test_string, 'a', 64 * 1024 * 1024);
  char *s3key = getenv("S3KEY");
  char *s3secret = getenv("S3SECRET");
  char *s3region = getenv("S3REGION");
  char *s3bucket = getenv("S3BUCKET");
  char *s3host = getenv("S3HOST");

  SKIP_IF_(!s3key, "Environemnt variable S3KEY missing");
  SKIP_IF_(!s3secret, "Environemnt variable S3SECRET missing");
  SKIP_IF_(!s3region, "Environemnt variable S3REGION missing");
  SKIP_IF_(!s3bucket, "Environemnt variable S3BUCKET missing");

  ms3_library_init();
  ms3_st *ms3 = ms3_thread_init(s3key, s3secret, s3region, s3host);

//  ms3_debug(true);
  ASSERT_NOT_NULL(ms3);

  res = ms3_put(ms3, s3bucket, "test/large_file.dat",
                (const uint8_t *)test_string,
                64 * 1024 * 1024);
  ASSERT_EQ_(res, 0, "Result: %u", res);
  res = ms3_get(ms3, s3bucket, "test/large_file.dat", &data, &length);
  ASSERT_EQ_(res, 0, "Result: %u", res);
  ASSERT_EQ(length, 64 * 1024 * 1024);
  res = ms3_delete(ms3, s3bucket, "test/large_file.dat");
  ASSERT_EQ_(res, 0, "Result: %u", res);
  free(test_string);
  ms3_free(data);
  ms3_deinit(ms3);
}
