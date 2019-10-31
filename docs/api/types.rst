Structs
=======

.. c:type:: ms3_st

   An internal struct which contains authentication information

.. c:type:: ms3_list_st

   A linked-list struct which contains a list of files/keys and information about them

   .. c:member:: char *key

      The key/filename for the object

   .. c:member:: size_t length

      The data size for the object

   .. c:member:: time_t created

      The created / updated timestamp for the object

   .. c:member:: struct ms3_list_st *next

      A pointer to the next struct in the list

.. c:type:: ms3_status_st

   An struct which contains the status of an object

   .. c:member:: size_t length

      The data size for the object

   .. c:member:: time_t created

      The created / updated timestamp for the object

Constants
=========

.. c:type:: ms3_set_option_t

   Options to use for :c:func:`ms3_set_option`. Possible values:

   * ``MS3_OPT_USE_HTTP`` - Use ``http://`` instead of ``https://``. The ``value`` parameter of :c:func:`ms3_set_option` is unused and each call to this toggles the flag (HTTPS is used by default)
   * ``MS3_OPT_DISABLE_SSL_VERIFY`` - Disable SSL verification. The ``value`` parameter of :c:func:`ms3_set_option` is unused and each call to this toggles the flag (SSL verification is on by default)
   * ``MS3_OPT_BUFFER_CHUNK_SIZE`` - Set the chunk size in bytes for the receive buffer. Default is 1MB. If you are receiving a large file a realloc will have to happen every time the buffer is full. For performance reasons you may want to increase the size of this buffer to reduce the reallocs and associated memory copies. The ``value`` parameter of :c:func:`ms3_set_option` should be a pointer to a :c:type:`size_t` greater than 1.
   * ``MS3_OPT_FORCE_LIST_VERSION`` - An internal option for the regression suite only. The ``value`` parameter of :c:func:`ms3_set_option` should be a pointer to a :c:type:`uint8_t` of value ``1`` or ``2``
   * ``MS3_OPT_FORCE_PROTOCOL_VERSION`` - Set to 1 to force talking to the S3 server using version 1 of the List Bucket API, this is for S3 compatible servers. Set to 2 to force talking to the S3 server version 2 of the List Bucket API. This is for use when the autodetect bsaed on providing a base_domain does the wrong thing. The ``value`` parameter of :c:func:`ms3_set_option` should be a pointer to a :c:type:`uint8_t` of value ``1`` or ``2``

Built-In Types
==============

.. c:type:: NULL

   A null pointer as defined in the standard header ``string.h``.

.. c:type:: uint8_t

   An unsigned single byte character as defined in the standard header ``stdint.h``

.. c:type:: bool

   A boolean type as defined in the standard header ``stdbool.h``

