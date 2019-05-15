#!/bin/sh
export CC=clang
export CFLAGS="-fsanitize=thread"
autoreconf -fi
./configure --enable-debug=yes
make
make check 2>/dev/null
