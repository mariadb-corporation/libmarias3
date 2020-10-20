Version History
===============

Version 3.1
-----------

Version 3.1.3 GA
^^^^^^^^^^^^^^^^

* Fix :c:func:`ms3_copy` not working correctly with non-alphanumeric characters (also affected :c:func:`ms3_move`)

Version 3.1.2 GA
^^^^^^^^^^^^^^^^

* Make library work with quirks in Google Cloud's S3 implementation
* Detect when libcurl was built with OpenSSL < 1.1.0 and add workaround to thread safety issues in the older OpenSSL versions (affects Ubuntu 16.04 in particular)
* Remove libxml and replace it with a modified version of `xml.c <https://github.com/ooxi/xml.c>`_ which handles <? ?> tags and other minor changes
* Fix issue where an empty key for :c:func:`ms3_get` turns it into a list call
* Partially fix issue with ``AC_MSG_ERROR``. Will still fail if you don't have ``libtool`` and ``pkg-config`` installed.

Version 3.1.1 GA (2019-06-28)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Fix bad host header when path based buckets are used
* Make autodetection of access type and list version *much* smarter:

  * Checks for S3 domain in provided domain and uses list version 2
  * Checks for IP provided domain and turns on list version 1 and path based buckets
  * Any other domain uses list version one and domain based buckets

* Reduced linked list mallocs for :c:func:`ms3_list` and :c:func:`ms3_list_dir`. This also deprecates :c:func:`ms3_list_free`.

Version 3.1.0 GA (2019-06-24)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Fix compiling issues when ``-Wdeclaration-after-statement`` is enabled
* Add ``MS3_OPT_FORCE_PROTOCOL_VERSION`` for use with :c:func:`ms3_set_option` which will force use of AWS S3 methods and paths (version 2) or compatible methods and paths (version 1)
* Fix double-free upon certain errors
* Add snowman UTF-8 test and minor cleanups
* Cleanup build system

Version 3.0
-----------

Version 3.0.2 GA (2019-05-24)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Fix libm linkage
* Remove mhash dependency and use a modified cut-down version of wpa_supplicant's BSD licensed crypto code (required for Windows compiling)
* Several minor performance optimizations

  * Removed 2x1kb mallocs on every request (now on :c:func:`ms3_init` instead)
  * Compiling with ``-O3`` by default
  * Stop executing string compares in list loop when something is found
  * Remove unneeded ``strdup()`` usage

Version 3.0.1 GA (2019-05-16)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Improve performance of PUT
* Fix a few potential pointer arithmatic issues
* Fix race condition on time generation
* Added TSAN to ci-scripts
* Fix minor issues found in cppcheck
* Stop buffer overrun if the buffer chunk size is set smaller than packet
* Fix :c:func:`ms3_get` returning random data if a CURL request completely fails
* Fix potential crash if the server error message is junk
* Fix double-free if a server error message is ``NULL``

Version 3.0.0 GA (2019-05-13)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Allow compiling to work with gnu89 compiler mode
* Fix building in CLang
* Removed previous deprecated ``ms3_thread_init`` and ``ms3_buffer_chunk_size``
* Remove ``bool`` from frontend API by:

  * Making :c:func:`ms3_debug` a toggle
  * Making the boolean options of :c:func:`ms3_set_option` toggles

Version 2.3
-----------

Version 2.3.0 GA (2019-05-07)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Allow compiling with a C++ compiler
* Fix logic error in :c:func:`ms3_move`
* Stop :c:func:`ms3_get` returning the error message as the object data on error
* Add :c:func:`ms3_list_dir` to get a non-recursive directory listing
* Setting the buffer chunk size using ``ms3_buffer_chunk_size`` or :c:func:`ms3_set_option` no longer has a lower limit of 1MB

Version 2.2
-----------

Version 2.2.0 GA (2019-04-23)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Add :c:func:`ms3_init` to replace ``ms3_thread_init`` and deprecate the latter.
* Add :c:func:`ms3_library_init_malloc` to add custom allocators
* Add :c:func:`ms3_library_deinit` to cleanup`
* Add :c:func:`ms3_copy` and :c:func:`ms3_move` to use S3's internal file copy

Version 2.1
-----------

Version 2.1.1 GA (2019-04-02)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Remove iso646.h support in codebase
* Autoswitch to bucket path instead of bucket domain access method (for IP urls)
* Fixed issue with SSL disabled verification
* Fixed minor leak when base_domain is set
* Add ``S3NOVERIFY`` env var to tests which will disable SSL verification when set to ``1``

Version 2.1.0 GA (2019-03-29)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Add :c:func:`ms3_set_option` to set various connection options
* Deprecated ``ms3_buffer_chunk_size``, use :c:func:`ms3_set_option` instead
* Added options to use ``http`` instead of ``https`` and to disable SSL verification
* Added debugging output for server/curl error messages
* Added compatibility for V1 bucket list API. Will turn on automatically for non-Amazon S3 compatible servers. Additionally an option has been created to force V1 or V2

Version 2.0
-----------

Version 2.0.0 GA (2019-03-28)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Fix double-free when using ``ms3_thread_init`` and an error occurs
* Fix error when a PUT >= 65535 is attempted
* Improve performance of GET for large files
* Make ``ms3_thread_init`` treat empty string base_domain as ``NULL``
* Add :c:func:`ms3_free`
* Add ``ms3_buffer_chunk_size``
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
* Added :c:func:`ms3_library_init` and ``ms3_thread_init`` for higher-performance acceses

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
