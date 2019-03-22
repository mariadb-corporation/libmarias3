# SYNOPSIS
#
#   AX_LIBMASH([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]]) 
#
# DESCRIPTION
#
#   libmhash library
#
# LICENSE
#
#   Copyright (c) 2012 Brian Aker <brian@tangent.org>
#   Copyright (c) 2019 Andrew Hutchings <linuxjedi@mariadb.com>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 1
 
AC_DEFUN([AX_LIBMHASH],
         [AC_PREREQ([2.63])dnl
         AC_CACHE_CHECK([test for a working libmhash],[ax_cv_libmhash],
           [AX_SAVE_FLAGS
           LIBS="-lmhash $LIBS"
           AC_LANG_PUSH([C])
           AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <mhash.h>
               ]],[[
               MHASH td;
               td = mhash_init(MHASH_MD5);
               ]])],
             [ax_cv_libmhash=yes],
             [ax_cv_libmhash=no],
             [AC_MSG_WARN([test program execution failed])])
           AC_LANG_POP
           AX_RESTORE_FLAGS
           ])

         AS_IF([test "x$ax_cv_libmhash" = "xyes"],
             [AC_SUBST([LIBMHASH_LIB],[-lmhash])
             AC_DEFINE([HAVE_LIBMHASH],[1],[Define if mhash_init is present in mhash.h.])],
             [AC_DEFINE([HAVE_LIBMHASH],[0],[Define if mhash_init is present in mhash.h.])])

         AM_CONDITIONAL(HAVE_LIBMHASH, test "x$ax_cv_libmhash" = "xyes")
# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
         AS_IF([test "x$ax_cv_libmhash" = xyes],
             [$1],
             [$2])
         ])
