#!/bin/sh
export CC="clang"
autoreconf -fi
./configure --enable-debug
scan-build --use-cc=clang --use-c++=clang++ --status-bugs make
