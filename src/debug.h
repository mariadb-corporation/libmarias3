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
#include <stdbool.h>

void ms3debug_set(bool enabled);
bool ms3debug_get(void);

#define ms3debug(MSG, ...) do { \
  if (ms3debug_get()) \
  { \
    fprintf(stderr, "[libmarias3] %s:%d " MSG "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
  } \
} while(0)
#define ms3debug_hex(DATA, LEN) do { \
  size_t hex_it; \
  fprintf(stderr, "[libmarias3] %s:%d packet hex: ", __FILE__, __LINE__); \
  for (hex_it = 0; hex_it < LEN ; hex_it++) \
  { \
    fprintf(stderr, "%02X ", (unsigned char)DATA[hex_it]); \
  } \
  fprintf(stderr, "\n"); \
  fprintf(stderr, "[libmarias3] %s:%d printable packet data: ", __FILE__, __LINE__); \
  for (hex_it = 0; hex_it < LEN ; hex_it++) \
  { \
    if (((unsigned char)DATA[hex_it] < 0x32) or (((unsigned char)DATA[hex_it] > 0x7e))) \
    { \
      fprintf(stderr, "."); \
    } \
    else \
    { \
      fprintf(stderr, "%c", (unsigned char)DATA[hex_it]); \
    } \
  } \
  fprintf(stderr, "\n"); \
} while(0)

