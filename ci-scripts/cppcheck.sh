#!/bin/sh
cppcheck --quiet --enable=all --error-exitcode=1 . src tests libmarias3
