#!/bin/bash
astyle --style=allman --indent=spaces=2 --indent-switches --break-blocks --pad-comma --pad-oper --pad-header --lineend=linux --align-pointer=name --align-reference=name --max-code-length=80 --recursive "*.c" "*.h"
