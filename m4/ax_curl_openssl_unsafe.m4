# SYNOPSIS
#
#   AX_CURL_OPENSSL_UNSAFE
#
# DESCRIPTION
#
#   Checks to see if Curl is using a thread safe version of OpenSSL
#
# LICENSE
#
#   Copyright (c) 2019 Andrew Hutchings <andrew.hutchings@mariadb.com>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 1

AC_DEFUN([AX_CURL_OPENSSL_UNSAFE],
    [AC_PREREQ([2.63])dnl
    AC_CACHE_CHECK([whether curl uses an OpenSSL which is not thread safe],
      [ax_cv_curl_openssl_unsafe],
      [AX_SAVE_FLAGS
      LIBS="-lcurl $LIBS"
      AC_LANG_PUSH([C])
      AC_RUN_IFELSE([AC_LANG_PROGRAM([[#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>]],
[[curl_version_info_data *data = curl_version_info(CURLVERSION_NOW);

  if (data->ssl_version)
  {
    if (strncmp(data->ssl_version, "OpenSSL", 7) != 0)
    {
      return EXIT_FAILURE;
    }
    if (data->ssl_version[8] == '0')
    {
      return EXIT_SUCCESS;
    }
    if ((data->ssl_version[8] == '1') && (data->ssl_version[10] == '0'))
    {
      return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
  }
  else
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;]])],
      [ax_cv_curl_openssl_unsafe=yes],
      [ax_cv_curl_openssl_unsafe=no],
      [ax_cv_curl_openssl_unsafe=no])
    AC_LANG_POP
    AX_RESTORE_FLAGS])
    AC_MSG_CHECKING([checking for curl_openssl_unsafe])
  AC_MSG_RESULT(["$ax_cv_curl_openssl_unsafe"])
  AS_IF([test "x$ax_cv_curl_openssl_unsafe" = xyes],
      [AC_DEFINE([HAVE_CURL_OPENSSL_UNSAFE],[1],[define if curl's OpenSSL is not thread safe])])
  ])

