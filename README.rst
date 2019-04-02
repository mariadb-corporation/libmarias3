libMariaS3
==========

This is a lightweight C library to read/write to AWS S3 buckets using objects in memory.

You will need an access key which for AWS can be created at `the AWS security crenditials page <https://console.aws.amazon.com/iam/home?#/security_credentials>`_.

Compiling
---------

.. code-block:: bash

   autoreconf -fi
   ./configure
   make

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

The test suite is automatically built along with the library and can be executed with ``make check`` or ``make distcheck``.
