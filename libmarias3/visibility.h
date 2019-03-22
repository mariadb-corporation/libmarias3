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

#if defined(BUILDING_MS3)
# if defined(HAVE_VISIBILITY) && HAVE_VISIBILITY
#  define MS3_API __attribute__ ((visibility("default")))
# elif defined (__SUNPRO_C) && (__SUNPRO_C >= 0x550)
#  define MS3_API __global
# elif defined(_MSC_VER)
#  define MS3_API extern __declspec(dllexport)
# else
#  define MS3_API
# endif /* defined(HAVE_VISIBILITY) */
#else  /* defined(BUILDING_MS3) */
# if defined(_MSC_VER)
#  define MS3_API extern __declspec(dllimport)
# else
#  define MS3_API
# endif /* defined(_MSC_VER) */
#endif /* defined(BUILDING_MS3) */
