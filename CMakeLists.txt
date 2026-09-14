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
    find_program(LIBDXFRW_PYTHON_EXECUTABLE NAMES python3)
    if(NOT LIBDXFRW_PYTHON_EXECUTABLE)
        message(FATAL_ERROR "LIBDXFRW_BUILD_TESTS requires a Python 3 interpreter")
    endif()
    execute_process(
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -c
            "import sys; raise SystemExit(0 if sys.version_info >= (3, 10) else 1)"
        RESULT_VARIABLE LIBDXFRW_PYTHON_VERSION_OK
    )
    if(NOT "${LIBDXFRW_PYTHON_VERSION_OK}" STREQUAL "0")
        message(FATAL_ERROR "LIBDXFRW_BUILD_TESTS requires Python 3.10 or newer")
    endif()
    add_test(NAME libdxfrw_parity_inventory_inputs
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_parity_inventory_inputs.py
            --self-test)
    add_test(NAME libdxfrw_parity_source_routes
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/extract_parity_source_routes.py
            --self-test)
    add_test(NAME libdxfrw_parity_differential
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_parity_differential.py
            --self-test)
    add_test(NAME libdxfrw_parity_differential_manifest
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_parity_differential.py
            --manifest
            ${CMAKE_CURRENT_SOURCE_DIR}/metadata/parity-differential-runners-v1.json)
    add_test(NAME libdxfrw_dxf_parity_lane
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_dxf_lane.py
            --mapping ${CMAKE_CURRENT_SOURCE_DIR}/metadata/parity-source-routes-v1.json
            --target ${CMAKE_CURRENT_SOURCE_DIR}/metadata/parity-source-routes-v1/target-dxfRW.json
            --standalone ${CMAKE_CURRENT_SOURCE_DIR}/metadata/parity-source-routes-v1/standalone-dxfRW.json)
    add_test(NAME libdxfrw_dwg_reader_parity_lane
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_dwg_lane.py
            --mode reader
            --mapping ${CMAKE_CURRENT_SOURCE_DIR}/metadata/parity-source-routes-v1.json
            --target ${CMAKE_CURRENT_SOURCE_DIR}/metadata/parity-source-routes-v1/target-dwgRW.json
            --standalone ${CMAKE_CURRENT_SOURCE_DIR}/metadata/parity-source-routes-v1/standalone-dwgRW.json)
    add_test(NAME libdxfrw_dwg_writer_parity_lane
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_dwg_lane.py
            --mode writer
            --mapping ${CMAKE_CURRENT_SOURCE_DIR}/metadata/parity-source-routes-v1.json
            --target ${CMAKE_CURRENT_SOURCE_DIR}/metadata/parity-source-routes-v1/target-dwgRW.json
            --standalone ${CMAKE_CURRENT_SOURCE_DIR}/metadata/parity-source-routes-v1/standalone-dwgRW.json)
    add_test(NAME libdxfrw_release_readiness
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_release_readiness.py
            --plan ${CMAKE_CURRENT_SOURCE_DIR}/LIBRECAD_DXFRW_UPGRADE_PLAN.md
            --mapping ${CMAKE_CURRENT_SOURCE_DIR}/metadata/parity-source-routes-v1.json
            --registry ${CMAKE_CURRENT_SOURCE_DIR}/metadata/parity-test-oracles-v1.json
            --manifest ${CMAKE_CURRENT_SOURCE_DIR}/metadata/parity-differential-runners-v1.json)

    add_executable(libdxfrw_wave1_tests tests/wave1_tests.cpp)
    target_compile_features(libdxfrw_wave1_tests PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_wave1_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intern
        ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf)
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

    add_executable(libdxfrw_writer_primitives_tests
        tests/writer_primitives_tests.cpp)
    target_compile_features(libdxfrw_writer_primitives_tests PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_writer_primitives_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intern)
    target_link_libraries(libdxfrw_writer_primitives_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_writer_primitives
        COMMAND libdxfrw_writer_primitives_tests)

    add_executable(libdxfrw_hardening_tests
        tests/hardening_tests.cpp)
    target_compile_features(libdxfrw_hardening_tests PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_hardening_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intern)
    target_link_libraries(libdxfrw_hardening_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_hardening
        COMMAND libdxfrw_hardening_tests)

    add_executable(libdxfrw_writer_version_matrix_tests
        tests/writer_version_matrix_tests.cpp)
    target_compile_features(libdxfrw_writer_version_matrix_tests PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_writer_version_matrix_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intern)
    target_link_libraries(libdxfrw_writer_version_matrix_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_writer_version_matrix
        COMMAND libdxfrw_writer_version_matrix_tests)

    add_executable(libdxfrw_diagnostic_tests
        tests/diagnostic_tests.cpp)
    target_compile_features(libdxfrw_diagnostic_tests PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_diagnostic_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intern)
    target_link_libraries(libdxfrw_diagnostic_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_diagnostic
        COMMAND libdxfrw_diagnostic_tests)
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
                  src/intern/dwgbufferw.h src/intern/dwgsafety.h
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
