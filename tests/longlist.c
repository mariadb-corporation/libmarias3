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
#include <pthread.h>

/* Tests listing 1500 items
 * This test creates 1500 items using 10 threads, lists them to check
 * we can get them all due to the 1000 item pagination limit and then
 * deletes them using 10 threads.
 */

struct thread_info
{
  pthread_t thread_id;
  int thread_num;
  int start_count;
  ms3_st *ms3;
  char *s3bucket;
};

const char *test_string = "Another one bites the dust";

static void *put_thread(void *arg)
{
  uint8_t res;
  struct thread_info *tinfo = arg;

  for (int i = tinfo->start_count; i < tinfo->start_count + 150; i++)
  {
    char fname[64];
    snprintf(fname, 64, "test/list-%d.dat", i);
    res = ms3_put(tinfo->ms3, tinfo->s3bucket, fname, (const uint8_t *)test_string,
                  strlen(test_string));
    ASSERT_EQ(res, 0);
  }

  return NULL;
}

static void *delete_thread(void *arg)
{
  uint8_t res;
  struct thread_info *tinfo = arg;

  for (int i = tinfo->start_count; i < tinfo->start_count + 150; i++)
  {
    char fname[64];
    snprintf(fname, 64, "test/list-%d.dat", i);
    res = ms3_delete(tinfo->ms3, tinfo->s3bucket, fname);
    ASSERT_EQ(res, 0);
  }

  return NULL;
}


int main(int argc, char *argv[])
{
  (void) argc;
  (void) argv;

  char *s3key = getenv("S3KEY");
  char *s3secret = getenv("S3SECRET");
  char *s3region = getenv("S3REGION");
  char *s3bucket = getenv("S3BUCKET");
  char *s3host = getenv("S3HOST");

  struct thread_info *tinfo;

  SKIP_IF_(!s3key, "Environemnt variable S3KEY missing");
  SKIP_IF_(!s3secret, "Environemnt variable S3SECRET missing");
  SKIP_IF_(!s3region, "Environemnt variable S3REGION missing");
  SKIP_IF_(!s3bucket, "Environemnt variable S3BUCKET missing");

  ms3_st *ms3 = ms3_init(s3key, s3secret, s3region, s3host);

//  ms3_debug(true);
  ASSERT_NOT_NULL(ms3);

  tinfo = calloc(10, sizeof(struct thread_info));

  int start_count = 1000;
  pthread_attr_t attr;
  pthread_attr_init(&attr);

  // Write 1500 files using 10 threads
  printf("Writing 1500 items\n");

  for (int tnum = 0; tnum < 10; tnum++)
  {
    tinfo[tnum].thread_num = tnum + 1;
    tinfo[tnum].start_count = start_count;
    start_count += 150;
    tinfo[tnum].ms3 = ms3;
    tinfo[tnum].s3bucket = s3bucket;
    pthread_create(&tinfo[tnum].thread_id, &attr,
                   &put_thread, &tinfo[tnum]);
  }

  for (int tnum = 0; tnum < 10; tnum++)
  {
    pthread_join(tinfo[tnum].thread_id, NULL);
  }

  free(tinfo);

  uint8_t res;
  ms3_list_st *list = NULL, *list_it = NULL;
  res = ms3_list(ms3, s3bucket, "test/", &list);
  ASSERT_EQ(res, 0);
  list_it = list;
  int res_count = 0;

  while (list_it)
  {
    res_count++;
    list_it = list_it->next;
  }

  printf("Found %d items\n", res_count);
  ASSERT_EQ(res_count, 1500);
  ms3_list_free(list);

  tinfo = calloc(10, sizeof(struct thread_info));
  start_count = 1000;

  // Destroy 1500 files using 10 threads
  printf("Deleteing 1500 items");

  for (int tnum = 0; tnum < 10; tnum++)
  {
    tinfo[tnum].thread_num = tnum + 1;
    tinfo[tnum].start_count = start_count;
    start_count += 150;
    tinfo[tnum].ms3 = ms3;
    tinfo[tnum].s3bucket = s3bucket;
    pthread_create(&tinfo[tnum].thread_id, &attr,
                   &delete_thread, &tinfo[tnum]);
  }

  for (int tnum = 0; tnum < 10; tnum++)
  {
    pthread_join(tinfo[tnum].thread_id, NULL);
  }

  free(tinfo);
  ms3_deinit(ms3);
}
