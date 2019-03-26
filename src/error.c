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

#include "config.h"
#include "common.h"

// NOTE: for every new error, add an entry to errmsgs here

const char *errmsgs[] =
{
  "No error",
  "Parameter error",
  "No data",
  "Could not parse response XML",
  "Generated URI too long",
  "Error making REST request",
  "Out of memory",
  "Impossible condition detected",
  "Authentication error",
  "File not found",
  "S3 server error",
  "Data too big. Maximum data size is 4GB"
};
