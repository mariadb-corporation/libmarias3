#!/bin/sh
autoreconf -fi
./configure --enable-debug=yes
make distcheck 2>/dev/null
