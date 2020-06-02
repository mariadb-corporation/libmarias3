# configure.ac -*- autoconf -*- vim: filetype=config
#
# DDM4
# Copyright (C) 2012 Data Differential, http://datadifferential.com/
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in this directory for full text.

AC_COPYRIGHT([2019 MariaDB Corporation Ab])
#AC_REVISION([m4_esyscmd_s([git describe --always])])
AC_PREREQ([2.63])

# These two needed for RHEL/CentOS 6
m4_ifndef([m4_esyscmd_s], [m4_define([m4_chomp_all],[m4_format([[%.*s]], m4_bregexp(m4_translit([[$1]],[/],[/]),[/*$]), [$1])])] [m4_define([m4_esyscmd_s], [m4_chomp_all(m4_esyscmd([$1]))])])

m4_ifndef([AS_VAR_COPY], [m4_define([AS_VAR_COPY], [AS_LITERAL_IF([$1[]$2], [$1=$$2],[eval $1=\$$2])])])

# NOTE: Major version should be < 100 and minor/patch < 256 or bad things will
# happen in src/asql/utility.cc
AC_INIT([libmarias3],m4_esyscmd_s([tr -d '\n' < VERSION]),[linuxjedi@mariadb.com],[libmarias3],[https://mariadb.com])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Must come before AM_INIT_AUTOMAKE.
AC_CONFIG_AUX_DIR([build-aux])
AC_CANONICAL_HOST
AC_CONFIG_MACRO_DIR([m4])

# Setup the compilers early on
AC_PROG_CC([cc gcc clang])

# Automake version before 1.13 (when the serial-tests option was dnl still the default) still defined the badly obsolete macro.  'AC_PROG_INSTALL'.
AM_INIT_AUTOMAKE(1.11 no-define color-tests -Wno-portability subdir-objects foreign tar-ustar m4_ifndef([AC_PROG_INSTALL], [serial-tests]))

AC_ARG_PROGRAM
AC_USE_SYSTEM_EXTENSIONS

AC_CONFIG_HEADERS([config.h:config.in])dnl Keep filename to 8.3 for MS-DOS.

# shared library versioning
LIBMARIAS3_LIBRARY_VERSION=4:2:1
#                          | | |
#                   +------+ | +---+
#                   |        |     |
#                current:revision:age
#                   |        |     |
#                   |        |     +- increment if interfaces have been added
#                   |        |        set to zero if interfaces have been removed or changed
#                   |        +- increment if source code has changed
#                   |           set to zero if current is incremented
#                   +- increment if interfaces have been added, removed or changed
AC_SUBST([LIBMARIAS3_LIBRARY_VERSION])

LT_PREREQ([2.2])
LT_INIT
LT_LANG([C])

AX_PLATFORM

CONFIG_EXTRA

# Checks for programs.
AC_CHECK_PROGS([LSB_RELEASE],[lsb_release])
AC_CHECK_PROGS([DPKG_GENSYMBOLS],[dpkg-gensymbols])
AX_PROG_SPHINX_BUILD(,[AC_MSG_WARN([sphinx-build version 1.0 or greater is required to build man pages])])
#AX_PROG_VALGRIND([memcheck],[--error-exitcode=1 --leak-check=yes --track-fds=yes --malloc-fill=A5 --free-fill=DE --suppressions=valgrind.supp])
AC_CHECK_PROGS([RPMBUILD],[rpmbuild --nocheck])
AC_CHECK_PROGS([RPMDEV_SETUPTREE],[rpmdev-setuptree])
AC_CHECK_PROGS([RPM],[rpm])

# Checks for typedefs, structures, and compiler characteristics.

PKG_CHECK_MODULES([LIBCURL], [libcurl >= 7.0], [ax_cv_libcurl=yes], [AC_MSG_ERROR(could not find a suitable version of libcurl)])
LT_LIB_M

AX_ENDIAN
AX_HEX_VERSION([LIBMARIAS3],[$VERSION])
AC_SUBST([RPM_RELEASE],[1])

AX_HARDEN_COMPILER_FLAGS

AC_DEFINE_UNQUOTED([C_COMPILER_VENDOR],["$ax_cv_c_compiler_vendor"],[Compiler vendor])
AC_DEFINE_UNQUOTED([CC],["$CC"],[Compiler information for CC compiler])
AC_DEFINE_UNQUOTED([CC_VERSION],["$CC_VERSION"],[Version information for CC compiler])
AC_DEFINE_UNQUOTED([CFLAGS],["$CFLAGS"],[CFLAGS used for compiling binary])

AS_IF([test -n "${LSB_RELEASE}"],[
       LSB_DESCRIPTION="`lsb_release -d -s`"
       ],[LSB_DESCRIPTION="unknown"])

AX_AM_JOBSERVER([yes])

AC_CONFIG_FILES([Makefile
                 rpm/libmarias3.spec
                 version.h
                 libmarias3.pc
                 ])

AX_CREATE_GENERIC_CONFIG

AC_OUTPUT

echo "---"
echo "Configuration summary for $PACKAGE_NAME version $VERSION"
echo ""
echo "   * Installation prefix:       $prefix"
echo "   * LSB Release:               $LSB_RELEASE"
echo "   * System type:               $host_vendor-$host_os"
echo "   * Host CPU:                  $host_cpu"
echo "   * Compiler Vendor:           $ax_cv_c_compiler_vendor"
echo "   * C Compiler:                $CC"
echo "   * C Compiler Version:        $ax_c_compiler_version"
echo "   * C Flags:                   $CFLAGS"
echo "   * LIBS Flags:                $LIBS"
echo "   * LDFLAGS Flags:             $LDFLAGS"
echo "   * Assertions enabled:        $ax_enable_assert"
echo "   * Debug enabled:             $ax_enable_debug"
echo "   * Warnings as failure:       $ac_cv_warnings_as_errors"
echo "   * make -j:                   $enable_jobserver"
echo "   * VCS checkout:              $ac_cv_vcs_system"
echo ""
echo "---"
