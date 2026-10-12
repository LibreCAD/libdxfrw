include(GNUInstallDirs)

add_executable(dwg2dxf dx_iface.cpp main.cpp dx_iface.h dx_data.h)
target_compile_options(dwg2dxf PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(dwg2dxf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(dwg2dxf dxfrw)

# A shared libdxfrw build installs its library under libdir while the
# standalone converter is installed under bindir. Keep the installed CLI
# runnable when FreeCAD launches it without developer loader paths.
if(BUILD_SHARED_LIBS AND (APPLE OR UNIX))
    if(IS_ABSOLUTE "${CMAKE_INSTALL_BINDIR}"
            OR IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
        set(_dxfrw_install_libdir "${CMAKE_INSTALL_FULL_LIBDIR}")
    else()
        cmake_path(RELATIVE_PATH CMAKE_INSTALL_LIBDIR
            BASE_DIRECTORY CMAKE_INSTALL_BINDIR
            OUTPUT_VARIABLE _dxfrw_install_libdir)
        if(APPLE)
            set(_dxfrw_install_origin "@loader_path")
        else()
            set(_dxfrw_install_origin "$ORIGIN")
        endif()
        set(_dxfrw_install_libdir
            "${_dxfrw_install_origin}/${_dxfrw_install_libdir}")
    endif()
    set_property(TARGET dwg2dxf APPEND PROPERTY INSTALL_RPATH
        "${_dxfrw_install_libdir}")
endif()

install(TARGETS dwg2dxf DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES dwg2dxf.1 DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
