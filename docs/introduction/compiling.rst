Compiling libMariaS3
====================

libMariaS3 is designed to be compiled with GCC or CLang on a modern Linux distrubition or Mac OSX.

Prerequisites
-------------

libMariaS3 requires *libcurl 7.x* and *libxml2* to be installed. For RPM based distributions this can be installed using:

.. code-block:: bash

   sudo dnf install libcurl-devel libxml2-devel

Building
--------

On most systems you can use the following commands, this is especially useful for customising your install::

   autoreconf -fi
   ./configure
   make
   make install

The build system will automatically detect how many processor cores you have (physicaly and virtual) and set the ``--jobs`` options of make accordingly.

Testing
-------

libMariaS3 comes with a basic test suite which we recommend executing, especially if you are building for a new platform.

You will need the following OS environment variables set to run the tests:

+------------+----------------------------------------------------------+
| Variable   | Desription                                               |
+============+==========================================================+
| S3KEY      | Your AWS access key                                      |
+------------+----------------------------------------------------------+
| S3SECRET   | Your AWS secret key                                      |
+------------+----------------------------------------------------------+
| S3REGION   | The AWS region (for example us-east-1)                   |
+------------+----------------------------------------------------------+
| S3BUCKET   | The S3 bucket name                                       |
+------------+----------------------------------------------------------+
| S3HOST     | OPTIONAL hostname for non-AWS S3 service                 |
+------------+----------------------------------------------------------+
| S3NOVERIFY | Set to ``1`` if the host should not use SSL verification |
+------------+----------------------------------------------------------+

The test suite is automatically built along with the library and can be executed with ``make check`` or ``make distcheck``.  If you wish to test with valgrind you can use::

      TESTS_ENVIRONMENT="./libtool --mode=execute valgrind --error-exitcode=1 --leak-check=yes --track-fds=yes --malloc-fill=A5 --free-fill=DE" make check

Building RPMs
-------------

The build system for libMariaS3 has the capability to build RPMs.  To build RPMs simply do the following:

.. code-block:: bash

   autoreconf -fi
   ./configure
   make dist-rpm

.. note::
   The package ``redhat-rpm-config`` is required for building the RPM because this generates the debuginfo RPM.

Building DEBs
-------------

Debian packages for libMariaS3 can be built using the standard ``dpkg-buildpackage`` tool as follows:

.. code-block:: bash

   autoreconf -fi
   dpkg-buildpackage

.. note::
   You may need to add ``--no-sign`` to dpkg-buildpackage to build unsigned packages.
