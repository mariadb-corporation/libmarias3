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
  char *s3bucket;
  char *s3key;
  char *s3secret;
  char *s3region;
  char *s3host;
  char *s3port;
  bool usehttp;
  bool noverify;
};

const char *test_string = "Another one bites the dust";

static void *put_thread(void *arg)
{
  int i;
  struct thread_info *tinfo = arg;
  ms3_st *ms3 = ms3_init(tinfo->s3key, tinfo->s3secret, tinfo->s3region,
                         tinfo->s3host);

  if (tinfo->noverify)
  {
    ms3_set_option(ms3, MS3_OPT_DISABLE_SSL_VERIFY, NULL);
  }

  if (tinfo->usehttp)
  {
    ms3_set_option(ms3, MS3_OPT_USE_HTTP, NULL);
  }

  if (tinfo->s3port)
  {
    int port = atol(tinfo->s3port);
    ms3_set_option(ms3, MS3_OPT_PORT_NUMBER, &port);
  }

  for (i = tinfo->start_count; i < tinfo->start_count + 150; i++)
  {
    uint8_t res;
    char fname[64];
    snprintf(fname, 64, "listtest/list-%d.dat", i);
    res = ms3_put(ms3, tinfo->s3bucket, fname, (const uint8_t *)test_string,
                  strlen(test_string));
    ASSERT_EQ(res, 0);
  }

  ms3_deinit(ms3);

  return NULL;
}

static void *delete_thread(void *arg)
{
  int i;
  struct thread_info *tinfo = arg;
  ms3_st *ms3 = ms3_init(tinfo->s3key, tinfo->s3secret, tinfo->s3region,
                         tinfo->s3host);

  if (tinfo->noverify)
  {
    ms3_set_option(ms3, MS3_OPT_DISABLE_SSL_VERIFY, NULL);
  }

  if (tinfo->usehttp)
  {
    ms3_set_option(ms3, MS3_OPT_USE_HTTP, NULL);
  }

  if (tinfo->s3port)
  {
    int port = atol(tinfo->s3port);
    ms3_set_option(ms3, MS3_OPT_PORT_NUMBER, &port);
  }

  for (i = tinfo->start_count; i < tinfo->start_count + 150; i++)
  {
    uint8_t res;
    char fname[64];
    snprintf(fname, 64, "listtest/list-%d.dat", i);
    res = ms3_delete(ms3, tinfo->s3bucket, fname);
    ASSERT_EQ(res, 0);
  }

  ms3_deinit(ms3);
  return NULL;
}


int main(int argc, char *argv[])
{


  int tnum;
  char *s3key = getenv("S3KEY");
  char *s3secret = getenv("S3SECRET");
  char *s3region = getenv("S3REGION");
  char *s3bucket = getenv("S3BUCKET");
  char *s3host = getenv("S3HOST");
  char *s3noverify = getenv("S3NOVERIFY");
  char *s3usehttp = getenv("S3USEHTTP");
  char *s3port = getenv("S3PORT");

  bool noverify = false;
  bool usehttp = false;
  struct thread_info *tinfo;
  int start_count;
  uint8_t res;
  uint8_t list_version;
  pthread_attr_t attr;
  ms3_st *ms3;
  int res_count;
  ms3_list_st *list = NULL, *list_it = NULL;

  if (s3noverify && !strcmp(s3noverify, "1"))
  {
    noverify = true;
  }

  if (s3usehttp && !strcmp(s3usehttp, "1"))
  {
    usehttp = true;
  }

  SKIP_IF_(!s3key, "Environemnt variable S3KEY missing");
  SKIP_IF_(!s3secret, "Environemnt variable S3SECRET missing");
  SKIP_IF_(!s3region, "Environemnt variable S3REGION missing");
  SKIP_IF_(!s3bucket, "Environemnt variable S3BUCKET missing");
  (void) argc;
  (void) argv;
  ms3_library_init();

//  ms3_debug(true);

  tinfo = calloc(10, sizeof(struct thread_info));

  start_count = 1000;

  pthread_attr_init(&attr);

  // Write 1500 files using 10 threads
  printf("Writing 1500 items\n");

  for (tnum = 0; tnum < 10; tnum++)
  {
    tinfo[tnum].thread_num = tnum + 1;
    tinfo[tnum].start_count = start_count;
    start_count += 150;
    tinfo[tnum].s3key = s3key;
    tinfo[tnum].s3secret = s3secret;
    tinfo[tnum].s3region = s3region;
    tinfo[tnum].s3host = s3host;
    tinfo[tnum].s3bucket = s3bucket;
    tinfo[tnum].s3port = s3port;
    tinfo[tnum].noverify = noverify;
    tinfo[tnum].usehttp = usehttp;
    pthread_create(&tinfo[tnum].thread_id, &attr,
                   &put_thread, &tinfo[tnum]);
  }

  for (tnum = 0; tnum < 10; tnum++)
  {
    pthread_join(tinfo[tnum].thread_id, NULL);
  }

  free(tinfo);

  ms3 = ms3_init(s3key, s3secret, s3region, s3host);

  if (noverify)
  {
    ms3_set_option(ms3, MS3_OPT_DISABLE_SSL_VERIFY, NULL);
  }

  if (usehttp)
  {
    ms3_set_option(ms3, MS3_OPT_USE_HTTP, NULL);
  }

  if (s3port)
  {
    int port = atol(s3port);
    ms3_set_option(ms3, MS3_OPT_PORT_NUMBER, &port);
  }

  res = ms3_list(ms3, s3bucket, "listtest/", &list);
  ASSERT_EQ(res, 0);
  list_it = list;
  res_count = 0;

  while (list_it)
  {
    res_count++;
    list_it = list_it->next;
  }

  printf("Found %d items\n", res_count);
  ASSERT_EQ(res_count, 1500);

  // Reattempt with list version 1
  list_version = 1;
  list = NULL;
  ms3_set_option(ms3, MS3_OPT_FORCE_LIST_VERSION, &list_version);
  res = ms3_list(ms3, s3bucket, "listtest/", &list);
  ASSERT_EQ(res, 0);
  list_it = list;
  res_count = 0;

  while (list_it)
  {
    res_count++;
    list_it = list_it->next;
  }

  printf("V1 Found %d items\n", res_count);
  ASSERT_EQ(res_count, 1500);

  ms3_deinit(ms3);

  tinfo = calloc(10, sizeof(struct thread_info));
  start_count = 1000;

  // Destroy 1500 files using 10 threads
  printf("Deleting 1500 items");

  for (tnum = 0; tnum < 10; tnum++)
  {
    tinfo[tnum].thread_num = tnum + 1;
    tinfo[tnum].start_count = start_count;
    start_count += 150;
    tinfo[tnum].s3key = s3key;
    tinfo[tnum].s3secret = s3secret;
    tinfo[tnum].s3region = s3region;
    tinfo[tnum].s3host = s3host;
    tinfo[tnum].s3bucket = s3bucket;
    tinfo[tnum].noverify = noverify;
    tinfo[tnum].usehttp = usehttp;
    tinfo[tnum].s3port = s3port;
    pthread_create(&tinfo[tnum].thread_id, &attr,
                   &delete_thread, &tinfo[tnum]);
  }

  for (tnum = 0; tnum < 10; tnum++)
  {
    pthread_join(tinfo[tnum].thread_id, NULL);
  }

  free(tinfo);
  ms3_library_deinit();
  return 0;
}
