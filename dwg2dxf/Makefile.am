# -*- Makefile.am -*-
ACLOCAL_AMFLAGS = -I m4

dwg2dxfdir = $(srcdir)

bin_PROGRAMS = dwg2dxf

AM_LDFLAGS = -L../src -ldxfrw

dist_noinst_HEADERS = dx_iface.h dx_data.h dx_dependency_audit.h dx_plotstyle_source.h dx_standard_output.h \
	dx_symbol_output.h dx_symbol_name_key.h dx_attdef_source_profile.h dx_block_source_profile.h dx_block_membership.h dx_compound_source_profile.h dx_layout_source_profile.h \
	dx_raster_dwg_wire.h dx_raster_output.h

dwg2dxf_SOURCES = dx_iface.cpp main.cpp

AM_CPPFLAGS = ${my_CPPFLAGS} -Wall -I$(top_srcdir)/src

man_MANS = dwg2dxf.1

EXTRA_DIST = $(dwg2dxf_HEADERS) $(dwg2dxf_SOURCES) $(man_MANS) ./vs2013/*
