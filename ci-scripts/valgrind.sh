#!/bin/sh
autoreconf -fi
./configure --enable-debug
TESTS_ENVIRONMENT="./libtool --mode=execute valgrind --error-exitcode=1 --leak-check=yes --track-fds=no --malloc-fill=A5 --free-fill=DE --suppressions=ci-scripts/valgrind.supp" make check
