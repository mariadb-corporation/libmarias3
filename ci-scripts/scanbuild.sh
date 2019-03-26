#!/bin/sh
export CC="clang"
autoreconf -fi
./configure --enable-debug=yes
scan-build --use-cc=clang --use-c++=clang++ --status-bugs make
