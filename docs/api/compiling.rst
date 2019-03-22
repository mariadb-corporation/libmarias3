Compiling Your Application
==========================

Include Files
-------------

Make sure that your application includes the main libMariaS3 include as follows:

.. code-block:: c

   #include <libmarias3/marias3.h>

This will pull in all the libMariaS3 functions and constants you may require for your application.

Package Config
--------------

libMaria3e includes a utility called ``libmarias3-config``.  This can give you all the options used to compile the library as well as the compiler options to link the library.  For a full list of what it providesrun:

.. code-block:: bash

   libmarias3-config --help

Compiling
---------

If the library is installed correctly in your Linux distribution compiling your application with libMariaS3 should be a simple matter of adding the library to link to as follows:

.. code-block:: bash

   gcc -o basic basic.c -lmarias3

And likewise for CLang:

.. code-block:: bash

   clang -o basic basic.c -lmarias3
