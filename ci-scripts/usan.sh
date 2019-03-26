#!/bin/sh
export CC=clang
export CFLAGS="-fsanitize=undefined -fsanitize=nullability"
autoreconf -fi
./configure --enable-debug=yes
make
make check 2>/dev/null
