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

/* Tests basic error handling */

int main(int argc, char *argv[])
{
  uint8_t *data;
  size_t length;
  const char *errmsg;
  uint8_t res;
  ms3_st *ms3 = ms3_init("12345678901234567890",
                         "1234567890123456789012345678901234567890", "us-east-1", NULL);

  (void) argc;
  (void) argv;

  // Enable here so cppcheck shows coverage
  ms3_debug();
  ASSERT_NOT_NULL(ms3);
  errmsg = ms3_error(255);
  ASSERT_STREQ(errmsg, "No such error code");
  errmsg = ms3_error(0);
  ASSERT_STREQ(errmsg, "No error");
  res = ms3_get(ms3, "bad", "bad/file.txt", &data, &length);
  printf("%d\n", res);
  printf("%s\n", ms3_server_error(ms3));
  ASSERT_EQ(res, MS3_ERR_AUTH); // Bad auth
  free(data);
  ms3_deinit(ms3);
  ms3 = ms3_init("12345678901234567890",
                 "1234567890123456789012345678901234567890", "us-east-1", "bad-domain");
  res = ms3_get(ms3, "bad", "bad/file.txt", &data, &length);
  ASSERT_EQ(res, MS3_ERR_REQUEST_ERROR);
  free(data);
  ms3_deinit(ms3);
  ms3_library_deinit();
  return 0;
}
