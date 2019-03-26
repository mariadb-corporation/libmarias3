#!/bin/sh
export CC=clang
export CFLAGS="-fsanitize=address"
autoreconf -fi
./configure --enable-debug=yes
make
make check 2>/dev/null
