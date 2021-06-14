Introduction to Contributing
============================

There are many ways to contribute to libMariaS3.  Simply using it and creating an issue report when you found a bug or have a suggestion is a great contribution.  Documentation and code contribituions are also greatly appreciated.

Layout
------

The code for libMariaS3 in several parts:

+--------------------+-------------------------------+
| Directory          | Contents                      |
+====================+===============================+
| ``/src``           | The API source code           |
+--------------------+-------------------------------+
| ``/libmarias3``    | The public API headers        |
+--------------------+-------------------------------+
| ``/tests``         | Unit tests for the public API |
+--------------------+-------------------------------+

In each case if any files are added or removed the ``include.am`` file in that directory will require updating to reflect the change.

Submitting to Github
--------------------

The main hub for the code is `GitHub <https://github.com/>`_.  The main tree is the `libMariaS3 GitHub tree <https://github.com/mariadb-corporation/libmarias3>`_.  Anyone is welcome to submit pull requests or issues.  All requests will be considered and appropriate feedback given.

Modifying the Build System
--------------------------

The build system is an m4 template system called `DDM4 <https://github.com/BrianAker/DDM4>`_.  If any changes are made to the scripts in ``m4`` directory the *serial* line will need incrementing in that file.  You should look for a line near the top that looks like:

.. code-block:: makefile

   #serial 7

Shared Library Version
^^^^^^^^^^^^^^^^^^^^^^

If any of the source code has changed please see ``LIBMARIAS3_LIBRARY_VERSION`` in ``configure.ac``.  This gives rules on bumping the shared library versioning, not to be confused with the API public version which follows similar rules as described in the next section.

API Version
-----------

API versioning is stored in the ``VERSION.txt`` file which is used by the build system to version the API and docs.  The versioning scheme follows the `Semantic Versioning Rules <http://semver.org/>`_.

Function Visibility
-------------------

The code and build system only exposes public API functions as usable symbols in the finished binary.  This cuts down on binary size quite significantly and also discourages use of undocumented functionality that was not designed for public use.

When adding a new API function to ``/libmarias3`` always add ``MS3_API`` on its own on the line above the function definition in the header.  This tells the build system this is an API function to be included.

License Headers
---------------

Please make sure before committing that all new files have appropriate license headers in.  Only add to the copyright of older headers if you have made a significant contribution to that file (25 - 50 lines is typically classed as significant for Open Souce projects).

