Functions
=========

ms3_init()
----------

.. c:function:: ms3_st *ms3_init(const char *s3key, const char *s3secret, const char *region, const char *base_domain)

   Initializes a :c:type:`ms3_st` object.

   Note::
       This function is not thread safe

   :param s3key: The AWS access key
   :param s3secret: The AWS secret key
   :param region: The AWS region to use (such as ``us-east-1``)
   :param base_domain: A domain name to use if AWS S3 is not the desired server (set to ``NULL`` for S3)
   :returns: A newly allocated marias3 object

ms3_deinit()
------------

.. c:function:: void ms3_deinit(ms3_st *ms3)

   Cleans up and frees a :c:type:`ms3_st` object.

   :param ms3: The marias3 object

ms3_error()
-----------

.. c:function:: const char *ms3_error(uint8_t errcode)

   Returns an error message for a given error code

   :param errcode: The error code to translate
   :returns: The error message

ms3_debug()
-----------

.. c:function:: void ms3_debug(bool state)

   Enables and disables debugging output on stderr

   Note::
       This enables/disables globally for the library

   :param state: Set to ``true`` to enable and ``false`` to disable

ms3_list()
----------

.. c:function:: uint8_t ms3_list(ms3_st *ms3, const char *bucket, const char *prefix, ms3_list_st **list)

   Retrieves a list of files from a given S3 bucket and fills it into a :c:type:`ms3_list_st`.

   The resulting list should be freed using :c:func:`ms3_list_free`

   :param ms3: The marias3 object
   :param bucket: The bucket name to use
   :param prefix: An optional path/file prefix to use (``NULL`` for all files)
   :param list: A pointer to a pointer that will contain the returned list
   :returns: ``0`` on success, a positive integer on failure

Example
^^^^^^^

.. code-block:: c

   char *s3key= getenv("S3KEY");
   char *s3secret= getenv("S3SECRET");
   char *s3region= getenv("S3REGION");
   char *s3bucket= getenv("S3BUCKET");
   ms3_list_st *list= NULL, *list_it= NULL;
   uint8_t res;

   ms3_st *ms3= ms3_init(s3key, s3secret, s3region, NULL);

   res= ms3_list(ms3, s3bucket, NULL, &list);
   if (res)
   {
       printf("Error occured: %d\n", res);
       return;
   }
   list_it= list;
   while(list_it)
   {
     printf("File: %s, size: %ld, tstamp: %ld\n", list_it->key, list_it->length, list_it->created);
     list_it= list_it->next;
   }
   ms3_list_free(list);
   ms3_deinit(ms3);

ms3_list_free()
---------------

.. c:function:: void ms3_list_free(ms3_list_st *list)

   Frees a list generated using :c:func:`ms3_list`

   :param list: The list to free

ms3_put()
---------

.. c:function:: uint8_t ms3_put(ms3_st *ms3, const char *bucket, const char *key, const uint8_t *data, size_t length)

   Puts a binary data from a given pointer into S3 at a given key/filename. If an existing key/file exists with the same name this will be overwritten.

   :param ms3: The marias3 object
   :param bucket: The bucket name to use
   :param key: The key/filename to create/overwrite
   :param data: A pointer to the data to write
   :param length: The length of the data to write
   :returns: ``0`` on success, a positive integer on failure

Example
^^^^^^^

.. code-block:: c

   char *s3key= getenv("S3KEY");
   char *s3secret= getenv("S3SECRET");
   char *s3region= getenv("S3REGION");
   char *s3bucket= getenv("S3BUCKET");
   uint8_t res;
   const char *test_string= "Another one bites the dust";

   ms3_st *ms3= ms3_init(s3key, s3secret, s3region, NULL);

   res= ms3_put(ms3, s3bucket, "test/ms3.txt", (const uint8_t*)test_string, strlen(test_string));
   if (res)
   {
       printf("Error occured: %d\n", res);
       return;
   }
   ms3_deinit(ms3);


ms3_get()
---------

.. c:function:: uint8_t ms3_get(ms3_st *ms3, const char *bucket, const char *key, uint8_t **data, size_t *length)

   Retrieves a given object from S3.

   .. Note::
       The application is expected to free the resulting data pointer after use

   :param ms3: The marias3 object
   :param bucket: The bucket name to use
   :param key: The key/filename to retrieve
   :param data: A pointer to a pointer the data to be retrieved into
   :param length: A pointer to the data length
   :returns: ``0`` on success, a positive integer on failure

Example
^^^^^^^

.. code-block:: c

   char *s3key= getenv("S3KEY");
   char *s3secret= getenv("S3SECRET");
   char *s3region= getenv("S3REGION");
   char *s3bucket= getenv("S3BUCKET");
   uint8_t res;
   uint8_t *data= NULL;
   size_t length;

   ms3_st *ms3= ms3_init(s3key, s3secret, s3region, NULL);

   res= ms3_get(ms3, s3bucket, "test/ms3.txt", &data, &length);
   if (res)
   {
       printf("Error occured: %d\n", res);
       return;
   }
   printf("File contents: %s\n", data);
   printf("File length: %ld\n", length);
   free(data);
   ms3_deinit(ms3);

ms3_delete()
------------

.. c:function:: uint8_t ms3_delete(ms3_st *ms3, const char *bucket, const char *key)

   Deletes an object from an S3 bucket

   :param ms3: The marias3 object
   :param bucket: The bucket name to use
   :param key: The key/filename to delete
   :returns: ``0`` on success, a positive integer on failure

Example
^^^^^^^

.. code-block:: c

   char *s3key= getenv("S3KEY");
   char *s3secret= getenv("S3SECRET");
   char *s3region= getenv("S3REGION");
   char *s3bucket= getenv("S3BUCKET");
   uint8_t res;

   ms3_st *ms3= ms3_init(s3key, s3secret, s3region, NULL);

   res = ms3_delete(ms3, s3bucket, "test/ms3.txt");
   if (res)
   {
       printf("Error occured: %d\n", res);
       return;
   }
   ms3_deinit(ms3);

ms3_status()
------------

.. c:function:: uint8_t ms3_status(ms3_st *ms3, const char *bucket, const char *key, ms3_status_st *status)

   Retreives the status of a given filename/key into a :c:type:`ms3_status_st` object. Will return an error if not found.

   :param ms3: The marias3 object
   :param bucket: The bucket name to use
   :param key: The key/filename to status check
   :param status: A status object to fill
   :returns: ``0`` on success, a positive integer on failure

Example
^^^^^^^

.. code-block:: c

   char *s3key= getenv("S3KEY");
   char *s3secret= getenv("S3SECRET");
   char *s3region= getenv("S3REGION");
   char *s3bucket= getenv("S3BUCKET");
   uint8_t res;
   ms3_status_st status;

   ms3_st *ms3= ms3_init(s3key, s3secret, s3region, NULL);

   res= ms3_status(ms3, s3bucket, "test/ms3.txt", &status);
   if (res)
   {
       printf("Error occured: %d\n", res);
       return;
   }
   printf("File length: %ld\n", status.length);
   printf("File timestamp: %ld\n", status.created);
   ms3_deinit(ms3);

