# -*- Makefile.am -*-
ACLOCAL_AMFLAGS = -I m4
bin_PROGRAMS = dwg2dxf
include $(srcdir)/source-files.am

AM_CPPFLAGS = ${my_CPPFLAGS} -I$(top_srcdir)/src
AM_CXXFLAGS = -Wall -Wextra -pedantic -Werror $(PTHREAD_CFLAGS)
dwg2dxf_LDADD = $(top_builddir)/src/libdxfrw.la $(PTHREAD_LIBS) $(COREFOUNDATION_LIBS)

man_MANS = dwg2dxf.1
