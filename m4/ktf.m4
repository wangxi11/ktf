#
# Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
#    Author: Knut Omang <knut.omang@oracle.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2
# as published by the Free Software Foundation.
#
#
# Dependent modules should make this file available in their m4 path
# and call this from their configure.ac:
#
# Require ktf and its dependencies (setting up KTF):
#
# AM_LIB_KTF
#
# For each subdirectory where kernel test modules are located within the project, call:
# AM_KTF_DIR([subdirectory])
#
#
AC_DEFUN([AM_LIB_KTF],
[

ktf_build="`pwd`/../ktf"
ktf_src="$ac_confdir/../ktf"

AC_ARG_WITH([ktf],
        [AS_HELP_STRING([--with-ktf],
        [where to find the ktf utility binaries])],
        [ktf_build=$with_ktf],
	[])

dnl deduce source directory from build directory - note the ][ to avoid
dnl that M4 expands $2. Also if src is relative to build, convert to abs path:
AS_IF([test -f $ktf_build/config.log],
   [ktf_configure=`awk '/^  \\$ .*configure/ { print $[]2; }' $ktf_build/config.log`

    AS_IF([test "x$ktf_configure" = "x./configure" ],dnl
          [ktf_src="$ktf_build"],dnl
          [
	    ktf_src=`dirname $ktf_configure`
	    ktf_src=`cd $ktf_build && cd $ktf_src && pwd`
	  ])
	  ktf_dir=$ktf_src/kernel
	  ktf_bdir=$ktf_build/kernel
   ],
   [ktf_src=$ktf_build
    ktf_dir=$ktf_build/include/ktf
    ktf_bdir=$ktf_dir
    ktf_scripts=$ktf_dir
   ]
)

AC_SUBST([KTF_DIR],[$ktf_dir])
AC_SUBST([KTF_BDIR],[$ktf_bdir])

PKG_CHECK_MODULES(LIBNL3, libnl-3.0 >= 3.1, [have_libnl3=yes],[ dnl
  have_libnl3=no
  PKG_CHECK_MODULES([NETLINK], [libnl-1 >= 1.1])
])

if (test "${have_libnl3}" = "yes"); then
        NETLINK_CFLAGS+=" $LIBNL3_CFLAGS"
        NETLINK_LIBS+=" $LIBNL3_LIBS -lnl-genl-3"
	AC_DEFINE([HAVE_LIBNL3], 1, [Using netlink v.3])
fi

KTF_CFLAGS="-I$ktf_src/lib"
KTF_LIBS="-L$ktf_build/lib -lktf $NETLINK_LIBS"

AC_ARG_VAR([KTF_CFLAGS],[Include files options needed for C/C++ user space program clients])
AC_ARG_VAR([KTF_LIBS],[Library options for tests accessing KTF functionality])

AS_IF([ test x$KVER != x ],dnl
	[AC_MSG_NOTICE(building against kernel version $KVER)],dnl
	[AC_MSG_ERROR("Kernel version (KVER) not set")])

AC_ARG_VAR([KVER],[Kernel version to use])

])

AC_DEFUN([AM_CONFIG_KTF],
[

gtest_fail_msg="No gtest library - install gtest-devel"

GTEST_LIB_CHECK([1.5.0],[echo -n ""],[AC_MSG_ERROR([$gtest_fail_msg])])

KTF_DIR="$srcdir/kernel"
KTF_BDIR="`pwd`/kernel"

ktf_scripts="$srcdir/scripts"

AC_SUBST([KTF_DIR],[$KTF_DIR])
AC_SUBST([KTF_BDIR],[$KTF_BDIR])

])

AC_DEFUN([AM_KTF_DIR],dnl Usage: AM_KTF_DIR([subdir]) where subdir contains kernel test defs
[

TEST_DIR="$srcdir/$1"
TEST_SRC=`cd $TEST_DIR && ls *.h *.c *.S 2> /dev/null | tr '\n' ' '| sed 's/ \w*\.mod\.c|\w*version.c|\wversioninfo.h//'`

dnl Provide automatic generation of internal symbol resolving from ktf_syms.txt
dnl if it exists:
dnl
ktf_symfile=`cd $TEST_DIR && ls ktf_syms.txt 2> /dev/null || true`

rulepath="$1"
rulefile="$rulepath/ktf_gen.mk"
top_builddir="`pwd`"

mkdir -p $rulepath
cat - > $rulefile <<EOF

top_builddir = $top_builddir
srcdir = $TEST_DIR
src_links = $TEST_SRC
ktf_symfile = $ktf_symfile

ktf_syms = \$(ktf_symfile:%.txt=%.h)
ktf_scripts = $ktf_scripts

obj-installed = \$(obj-m:%.o=$prefix/kernel/%.ko)

all: \$(ktf_syms) \$(src_links) module

Makefile: \$(srcdir)/Makefile.in \$(top_builddir)/config.status
	@case '\$?' in \\
	  *config.status*) \\
	    cd \$(top_builddir) && \$(MAKE) \$(AM_MAKEFLAGS) am--refresh;; \\
	  *) \\
	    echo ' cd \$(top_builddir) && \$(SHELL) ./config.status \$(subdir)/\$[]@; \
	    cd \$(top_builddir) && \$(SHELL) ./config.status \$(subdir)/\$[]@ ;; \\
	esac;

ktf_syms.h: \$(srcdir)/ktf_syms.txt
	\$(ktf_scripts)/resolve \$(ccflags-y) \$< \$[]@

install: \$(obj-installed)

\$(obj-installed): $prefix/kernel/%: %
	@(test -d $prefix/kernel || mkdir -p $prefix/kernel)
	cp \$< \$[]@

uninstall:
	@rm -f \$(src_links)

\$(src_links): %: \$(srcdir)/% Makefile
	@(test -e \$[]@ || ln -s \$< \$[]@)

EOF

])
