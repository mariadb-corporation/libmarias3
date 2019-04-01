Coding Standard
===============

General
-------

We are aiming for a minimum of C99 support. A script in ``extra`` can be found called ``astyle.sh``. This uses the Linux tool `Artistic Style <http://astyle.sourceforge.net/>`_ to enforce coding standards.

Coding Style
------------

Everyone has a preferred coding style, there is no real correct style.  What is important is that we stick to one style throughout the code.

We should use a variant of the `Allman coding style <http://en.wikipedia.org/wiki/Indent_style#Allman_style>`_.  The variation is to use 2 spaces instead of tabs.  The exception to the rule is Makefiles where space indentation can break them.

Allman style specifies that braces associated with a statement should be on the following line with the same indentation and the statements inside the braces are next level indented.  The closing braces are also on a new line at the same indentation as the original statement.

For example:

.. code-block:: cpp

   while (x == y)
   {
     something();
     somethingelse();
   }
   finalthing();


Types
-----

Use C99 types (where possible), this will very much help us to find conversion bugs.  So:

* Use bool, not my_bool.
* Use true and false, not TRUE and FALSE (those macros need to die).
* ulong → uint32_t
* ulonglong uint64_t
* long int → int32_t

The keyword :c:type:`NULL` should always be used when referring to the pointer NULL

Allocation
----------

For performance reasons we should try to limit the number of times we allocate and deallocate memory.  Do not do thousands of allocates and deallocates to save 32k of RAM.

Naming style
------------

Variable names
^^^^^^^^^^^^^^

Variables should be verbosely names, no caps, underscores with spaces.  Do not just use ``i`` in for loops, again we have developers with bad eyes.

Types
^^^^^

New types should use the ``_t`` postfix.  Private structs should be typedef'ed and also use this.

Public Structs
^^^^^^^^^^^^^^

Public structs should be typedef'ed and use the ``_st`` postfix

Conventions
^^^^^^^^^^^

* use *column* instead of *field*
* use *schema* instead of *database*

Include Files
-------------

Includes that will be installed need to be written like:

.. code-block:: cpp

   #include <drizzled/field/blob.h>


The following should only be used in cases where we are to never install these libraries in the filesystem:

.. code-block:: cpp

   #include "item.h"

Comments
--------

Where it is not obvious what is going on.  Hopefully most of the code will be self-commenting.

All code should have license headers.

Comment blocks should use the format:

.. code-block:: cpp

   /* Comment Block
    * This is a multi-line comment block
    */

C99 style in-line and single line comments are allowed for small comments

.. code-block:: cpp

   // small comment

Line lengths
------------

Whilst there is no hard limit on line lengths it is recommended that lines stay under 80 characters unless going above this increases readability of the code.
