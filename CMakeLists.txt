CMAKE_MINIMUM_REQUIRED(VERSION 2.8.10)
PROJECT (DXFRW)
ADD_DEFINITIONS(-std=c++11)

SET(LIBDXFRW_VERSION 1.0.0)
SET(LIBDXFRW_SOVERSION 1)

# SOURCE FILES
set(libdxfrw_srcs
  src/intern/drw_dbg.cpp
  src/intern/drw_textcodec.cpp
  src/intern/dwgbuffer.cpp
  src/intern/dwgreader.cpp
  src/intern/dwgreader15.cpp
  src/intern/dwgreader18.cpp
  src/intern/dwgreader21.cpp
  src/intern/dwgreader24.cpp
  src/intern/dwgreader27.cpp
  src/intern/dwgutil.cpp
  src/intern/dxfreader.cpp
  src/intern/dxfwriter.cpp
  src/intern/rscodec.cpp
  src/drw_classes.cpp
  src/drw_entities.cpp
  src/drw_header.cpp
  src/drw_objects.cpp
  src/libdwgr.cpp
  src/libdxfrw.cpp
)

# HEADER FILES
set(libdxfrw_hdrs
  src/intern/drw_cptable932.h
  src/intern/drw_cptable936.h
  src/intern/drw_cptable949.h
  src/intern/drw_cptable950.h
  src/intern/drw_cptables.h
  src/intern/drw_dbg.h
  src/intern/drw_textcodec.h
  src/intern/dwgbuffer.h
  src/intern/dwgreader.h
  src/intern/dwgreader15.h
  src/intern/dwgreader18.h
  src/intern/dwgreader21.h
  src/intern/dwgreader24.h
  src/intern/dwgreader27.h
  src/intern/dwgutil.h
  src/intern/dxfreader.h
  src/intern/dxfwriter.h
  src/intern/rscodec.h
  src/main_doc.h
)

set(installdxfrw_hdrs
  src/drw_base.h
  src/drw_classes.h
  src/drw_entities.h
  src/drw_header.h
  src/drw_interface.h
  src/drw_objects.h
  src/libdwgr.h
  src/libdxfrw.h
)

  
# DEBUGGING PATHS 
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()

add_library(dxfrw SHARED ${libdxfrw_srcs} ${libdxfrw_hdrs})

set_target_properties(dxfrw PROPERTIES
  VERSION ${LIBDXFRW_VERSION}
  SOVERSION ${LIBDXFRW_SOVERSION}
)

# INSTALLATION
install(TARGETS dxfrw DESTINATION lib)
set(HDRSUBDIR "/libdxfrw0/")
install(FILES ${installdxfrw_hdrs} DESTINATION include${HDRSUBDIR})
