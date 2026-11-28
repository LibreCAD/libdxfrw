# -*- Makefile -*-
AM_CPPFLAGS = ${my_CPPFLAGS}
AM_CXXFLAGS = -Wall -Wextra -pedantic -Werror $(LIBDXFRW_OVERLOADED_VIRTUAL_FLAG) $(PTHREAD_CFLAGS)
ACLOCAL_AMFLAGS = -I m4

library_includedir = $(includedir)/libdxfrw
include $(srcdir)/source-files.am

lib_LTLIBRARIES = libdxfrw.la
noinst_LTLIBRARIES = libdxfrw_entities.la
# Match CMake's narrow GCC policy for the byte-frozen parser translation unit.
libdxfrw_entities_la_CXXFLAGS = $(AM_CXXFLAGS) $(LIBDXFRW_ENTITY_WARNING_FLAGS)
libdxfrw_la_LDFLAGS = -no-undefined -version-number $(LIBDXFRW_ABI_MAJOR):0:0
libdxfrw_la_LIBADD = libdxfrw_entities.la $(PTHREAD_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(top_builddir)/libdxfrw.pc
