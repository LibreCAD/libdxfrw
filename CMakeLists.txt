CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

cmake_policy(SET CMP0048 NEW)
project(DXFRW VERSION 2.0.0)

# set preferred cache variables
option(LIBDXFRW_BUILD_LIBRARY "Build the libdxfrw library" ON)
option(LIBDXFRW_BUILD_DOC "Build the documentation" ON)
option(LIBDXFRW_BUILD_DWG2DXF "Build the dwg2dxf application" ON)
option(LIBDXFRW_BUILD_TESTS "Build dependency-free Wave 1 tests" OFF)

if(LIBDXFRW_BUILD_DWG2DXF AND NOT LIBDXFRW_BUILD_LIBRARY)
    message(FATAL_ERROR "LIBDXFRW_BUILD_DWG2DXF requires LIBDXFRW_BUILD_LIBRARY")
endif()

# Keep warnings target-local so consumers are not forced to inherit the
# library's maintainer diagnostics policy.
if (MSVC)
    set(LIBDXFRW_WARNING_FLAGS /W3)
else()
    set(LIBDXFRW_WARNING_FLAGS -Wall -Wextra -pedantic -Werror)
endif()


include(GNUInstallDirs)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The pinned source list is the completeness oracle once the target snapshot
# is present.  It is activated atomically with the imported source tree.
include(${CMAKE_CURRENT_SOURCE_DIR}/libdxfrw_sources.cmake)
set(libdxfrw_srcs ${LIBDXFRW_SOURCES})
set(libdxfrw_hdrs ${LIBDXFRW_PRIVATE_HEADERS} ${LIBDXFRW_PUBLIC_HEADERS})
set(installdxfrw_hdrs ${LIBDXFRW_PUBLIC_HEADERS})
set(libdxfrw_transitive_hdrs
    src/handle_allocator.h
    src/intern/dwg_fixed_handles.h
    src/intern/dwgutil.h
)

set(LIBDXFRW_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR}/libdxfrw)

if(LIBDXFRW_BUILD_LIBRARY)
    add_library(dxfrw ${libdxfrw_srcs} ${libdxfrw_hdrs})
    set_target_properties(dxfrw PROPERTIES
        VERSION "${PROJECT_VERSION}"
        SOVERSION "${PROJECT_VERSION_MAJOR}"
        EXPORT_NAME libdxfrw
    )
    target_compile_features(dxfrw PUBLIC cxx_std_17)
    target_compile_options(dxfrw PRIVATE ${LIBDXFRW_WARNING_FLAGS})
    if(MSVC)
        target_compile_options(dxfrw PRIVATE /bigobj)
    endif()
    target_include_directories(dxfrw
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
            $<INSTALL_INTERFACE:${LIBDXFRW_INSTALL_INCLUDEDIR}>
    )
endif()

if(LIBDXFRW_BUILD_DOC)
    add_custom_command(OUTPUT doc/html/index.html
	    COMMAND doxygen ARGS ${CMAKE_CURRENT_SOURCE_DIR}/libdxfrw.dox
	    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/libdxfrw.dox
	    VERBATIM)

    add_custom_target(doc
	    DEPENDS doc/html/index.html)
endif()

if(LIBDXFRW_BUILD_DWG2DXF)
    add_subdirectory(dwg2dxf)
endif()

if(LIBDXFRW_BUILD_TESTS)
    enable_testing()
    add_executable(libdxfrw_wave1_tests tests/wave1_tests.cpp)
    target_compile_features(libdxfrw_wave1_tests PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_wave1_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intern)
    target_link_libraries(libdxfrw_wave1_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_wave1 COMMAND libdxfrw_wave1_tests)

    add_executable(libdxfrw_dwg_reader_matrix_tests
        tests/dwg_reader_matrix_tests.cpp)
    target_compile_features(libdxfrw_dwg_reader_matrix_tests PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_dwg_reader_matrix_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intern)
    target_link_libraries(libdxfrw_dwg_reader_matrix_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_dwg_reader_matrix
        COMMAND libdxfrw_dwg_reader_matrix_tests)

    add_executable(libdxfrw_graph_preservation_tests
        tests/graph_preservation_tests.cpp)
    target_compile_features(libdxfrw_graph_preservation_tests PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_graph_preservation_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intern)
    target_link_libraries(libdxfrw_graph_preservation_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_graph_preservation
        COMMAND libdxfrw_graph_preservation_tests)
endif()

# INSTALLATION
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/libdxfrw)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libdxfrw.pc.in libdxfrw.pc @ONLY)

if(LIBDXFRW_BUILD_LIBRARY)
    install(TARGETS dxfrw
        EXPORT libdxfrw-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(EXPORT libdxfrw-targets
        FILE libdxfrwTargets.cmake
        NAMESPACE libdxfrw::
        DESTINATION ${INSTALL_CONFIGDIR}
    )

    include(CMakePackageConfigHelpers)
    configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/libdxfrwConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/libdxfrwConfig.cmake
            INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
    )
    write_basic_package_version_file(
            ${CMAKE_CURRENT_BINARY_DIR}/libdxfrwConfigVersion.cmake
            VERSION ${PROJECT_VERSION}
            COMPATIBILITY SameMajorVersion
    )

    install(FILES
            ${installdxfrw_hdrs}
            DESTINATION ${LIBDXFRW_INSTALL_INCLUDEDIR}
    )
    # The imported public façades still expose these declarations.  Install
    # only the narrow transitive closure; hiding HandleAllocator and the
    # DataStorage writer enum is tracked as a compatibility follow-up.
    install(FILES src/handle_allocator.h
            DESTINATION ${LIBDXFRW_INSTALL_INCLUDEDIR})
    install(FILES src/intern/dwgutil.h src/intern/dwg_fixed_handles.h
            DESTINATION ${LIBDXFRW_INSTALL_INCLUDEDIR}/intern)
    install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/libdxfrw.pc
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    )
    install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/libdxfrwConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/libdxfrwConfigVersion.cmake
            DESTINATION ${INSTALL_CONFIGDIR}
    )
endif()
