Version History
===============

Version 1.1
-----------

Version 1.1.1
^^^^^^^^^^^^^

* Fix double-free when using :c:func:`ms3_thread_init` and an error occurs

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
* Fixed pagination calls for ms3_list() so it support > 1000 objects
* Made ms3_init() thread safe

Version 1.0.0 Beta (2019-03-25)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Initial Beta version
