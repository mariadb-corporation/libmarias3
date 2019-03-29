Version History
===============

Version 2.1
-----------

Version 2.1.0 GA
^^^^^^^^^^^^^^^^

* Add :c:func:`ms3_set_option` to set various connection options
* Deprecated :c:func:`ms3_buffer_chunk_size`, use :c:func:`ms3_set_option` instead
* Added options to use ``http`` instead of ``https`` and to disable SSL verification
* Added debugging output for server/curl error messages
* Added compatibility for V1 bucket list API. Will turn on automatically for non-Amazon S3 compatible servers. Additionally an option has been created to force V1 or V2

Version 2.0
-----------

Version 2.0.0 GA (2019-03-28)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Fix double-free when using :c:func:`ms3_thread_init` and an error occurs
* Fix error when a PUT >= 65535 is attempted
* Improve performance of GET for large files
* Make :c:func:`ms3_thread_init` treat empty string base_domain as ``NULL``
* Add :c:func:`ms3_free`
* Add :c:func:`ms3_buffer_chunk_size`
* Cleanup linking
* Removed ``ms3_init``
* Added :c:func:`ms3_server_error` to get the last server or Curl error

Version 1.1
-----------

Version 1.1.0 GA (2019-03-27)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Fix memory leak in libxml2 function usage
* Fix memory leaks in libcurl usage
* Fix test collisions causing failures
* Added :c:func:`ms3_library_init` and :c:func:`ms3_thread_init` for higher-performance acceses

Version 1.0
-----------

Version 1.0.1 RC (2019-03-26)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Fixed issues found with valgrind, cppcheck and scanbuild
* Added RPM & DEB build systems
* Fixed pagination calls for :c:func:`ms3_list` so it support > 1000 objects
* Made ``ms3_init()`` thread safe

Version 1.0.0 Beta (2019-03-25)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Initial Beta version
