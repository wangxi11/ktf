## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -I$(top_srcdir) $(LIBNL3_CFLAGS) -Wall -Werror -Wno-packed-bitfield-compat -D_GNU_SOURCE
AM_CXXFLAGS = -I$(top_srcdir) $(LIBNL3_CFLAGS) \
		-Wall \
		-Wno-packed-bitfield-compat \
		-Wno-pointer-arith -Werror \
		-D__FILENAME__=\"`basename $<`\"

bin_PROGRAMS = ktfrun ktfcov

## Simple kernel test runner sample program:
ktfrun_SOURCES = ktfrun.cpp
ktfrun_LDADD = -L$(top_builddir)/lib -lktf $(NETLINK_LIBS) $(GTEST_LIBS)
ktfcov_SOURCES = ktfcov.cpp
ktfcov_LDADD = -L$(top_builddir)/lib -lktf $(NETLINK_LIBS) $(GTEST_LIBS)
