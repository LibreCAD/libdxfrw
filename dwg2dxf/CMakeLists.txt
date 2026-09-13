include(GNUInstallDirs)

add_executable(dwg2dxf dx_iface.cpp main.cpp dx_iface.h dx_data.h)
target_compile_options(dwg2dxf PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(dwg2dxf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(dwg2dxf dxfrw)

install(TARGETS dwg2dxf DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES dwg2dxf.1 DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
