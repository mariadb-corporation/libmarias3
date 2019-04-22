Functions
=========

ms3_library_init()
------------------

.. c:function:: void ms3_library_init(void)

   Initializes the library for use.
   Should be called before any threads are spawned.

ms3_library_deinit()
--------------------

.. c:function:: void ms3_library_deinit(void)

   Cleans up the library, typically for the end of the application's execution.

ms3_library_init_malloc()
-------------------------

.. c:function:: uint8_t ms3_library_init_malloc(ms3_malloc_callback m, ms3_free_callback f, ms3_realloc_callback r, ms3_strdup_callback s, ms3_calloc_callback c)

   Initialize the library for use with custom allocator replacement functions. These functions are also fed into libcurl and libxml2. The function prototypes should be as follows:

   .. c:function:: void *ms3_malloc_callback(size_t size)

      To replace ``malloc()``.

   .. c:function:: void ms3_free_callback(void *ptr)

      To replace ``free()``.

   .. c:function:: void *ms3_realloc_callback(void *ptr, size_t size)

      To replace ``realloc()``.

   .. c:function:: char *ms3_strdup_callback(const char *str)

      To replace ``strdup()``.

   .. c:function:: void *ms3_calloc_callback(size_t nmemb, size_t size)

      To replace ``calloc()``.

   Should be called before any threads are spawned. All parameters are required or the function *will* fail.

   Remember: With great power comes great responsibility.

   :param m: The malloc callback
   :param f: The free callback
   :param r: The realloc callback
   :param s: The strdup callback
   :param c: The calloc callback
   :returns: ``0`` on success, ``MS3_ERR_PARAMETER`` if a parameter is ``NULL``

ms3_init()
----------

.. c:function:: ms3_st *ms3_thread_init(const char *s3key, const char *s3secret, const char *region, const char *base_domain)

   .. deprecated:: 2.2.0
      Use :c:func:`ms3_init` instead. Will be removed in 3.0.0.

.. c:function:: ms3_st *ms3_init(const char *s3key, const char *s3secret, const char *region, const char *base_domain)

   Initializes a :c:type:`ms3_st` object. This object should only be used in
   the thread that created it because it reuses connections. But it is safe to
   have other :c:type:`ms3_st` objects running at the same time in other threads.

   .. note::
       You *MUST* call :c:func:`ms3_library_init` before
       spawning threads when using this access method.

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

ms3_server_error()
------------------

.. c:function:: const char *ms3_server_error(ms3_st *ms3)

   Returns the last error message from the S3 server or underlying Curl library.

   :param ms3: The marias3 object
   :returns: The error message string or ``NULL`` if there is no message.

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

   ms3_library_init();
   ms3_st *ms3= ms3_thread_init(s3key, s3secret, s3region, NULL);

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

   ms3_library_init();
   ms3_st *ms3= ms3_thread_init(s3key, s3secret, s3region, NULL);

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

   ms3_library_init();
   ms3_st *ms3= ms3_thread_init(s3key, s3secret, s3region, NULL);

   res= ms3_get(ms3, s3bucket, "test/ms3.txt", &data, &length);
   if (res)
   {
       printf("Error occured: %d\n", res);
       return;
   }
   printf("File contents: %s\n", data);
   printf("File length: %ld\n", length);
   ms3_free(data);
   ms3_deinit(ms3);

ms3_free()
----------

.. c:function:: void ms3_free(uint8_t *data)

   Used to free the data allocated by :c:func:`ms3_get`.

   :param data: The data to free

ms3_buffer_chunk_size()
-----------------------

.. c:function:: uint8_t ms3_buffer_chunk_size(size_t new_size)

   Set the chunk size for the receive buffer. Default is 1MB.
   If you are receiving a large file a realloc will have to happen every time the buffer is full. For performance reasons you may want to increase the size of this buffer to reduce the reallocs and associated memory copies.

   .. note::
      Attempts to set this lower than 1MB will be ignored and will result in an error

   .. deprecated:: 2.1.0
      Use :c:func:`ms3_set_option` with MS3_OPT_BUFFER_CHUNK_SIZE instead. Will be removed in 3.0.0.

   :param new_size: The new buffer chunk size to set
   :returns: ``0`` on success, a positive integer on failure

ms3_set_option()
----------------

.. c:function:: uint8_t ms3_set_option(ms3_st *ms3, ms3_set_option_t option, void *value)

   Sets a given connection option. See :c:type:`ms3_set_option_t` for a list of options.

   :param ms3: The marias3 object
   :param option: The option to set
   :param value: A pointer to the value for the option (if required, ``NULL`` if not)
   :returns: ``0`` on success, a positive integer on failure

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

   ms3_library_init();
   ms3_st *ms3= ms3_thread_init(s3key, s3secret, s3region, NULL);

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

   ms3_library_init();
   ms3_st *ms3= ms3_thread_init(s3key, s3secret, s3region, NULL);

   res= ms3_status(ms3, s3bucket, "test/ms3.txt", &status);
   if (res)
   {
       printf("Error occured: %d\n", res);
       return;
   }
   printf("File length: %ld\n", status.length);
   printf("File timestamp: %ld\n", status.created);
   ms3_deinit(ms3);

