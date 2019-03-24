#!/bin/sh
autoreconf -fi
./configure --enable-debug
make distcheck 2>/dev/null
