/* vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * Copyright 2019 MariaDB Corporation Ab. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
 */

#pragma once

#include "config.h"

// NOTE: for every new error, add an entry to errmsgs in error.c
enum MS3_ERROR_CODES
{
  MS3_ERR_NONE,
  MS3_ERR_PARAMETER,
  MS3_ERR_NO_DATA,
  MS3_ERR_URI_TOO_LONG,
  MS3_ERR_RESPONSE_PARSE,
  MS3_ERR_REQUEST_ERROR,
  MS3_ERR_OOM,
  MS3_ERR_IMPOSSIBLE,
  MS3_ERR_AUTH,
  MS3_ERR_NOT_FOUND,
  MS3_ERR_SERVER,
  MS3_ERR_TOO_BIG,
  MS3_ERR_MAX // Always the last error
};

#define baderror "No such error code"

// extern and define in C file so we don't get redefinition at link time
extern const char *errmsgs[];
