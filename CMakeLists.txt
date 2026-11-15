CMAKE_MINIMUM_REQUIRED(VERSION 3.28)

cmake_policy(SET CMP0048 NEW)
project(DXFRW VERSION 2.0.1)

# set preferred cache variables
option(LIBDXFRW_BUILD_LIBRARY "Build the libdxfrw library" ON)
option(LIBDXFRW_BUILD_DOC "Build the documentation" ON)
option(LIBDXFRW_BUILD_DWG2DXF "Build the dwg2dxf application" ON)
option(LIBDXFRW_BUILD_TESTS "Build dependency-free Wave 1 tests" OFF)
option(LIBDXFRW_DEDUP_TEST_ADAPTER
    "Share explicitly audited, identical uninstalled test adapter objects" ON)
# CMake 3.28 sets CMP0077 to NEW, so a parent project's normal variable is
# preserved when this project is added as a subdirectory.
option(LIBDXFRW_INSTALL_DEV
    "Install headers, the pkg-config file and the CMake package" ON)
option(LIBDXFRW_BUILD_QUALIFICATION
    "Build the pinned target/standalone semantic qualification adapters" OFF)
option(LIBDXFRW_BUILD_LONG_FUZZ
    "Build the opt-in extended deterministic parser fuzz target" OFF)
option(LIBDXFRW_ENABLE_INDEPENDENT_DXF_ORACLE
    "Enable advisory generated DXF comparisons with an explicitly pinned ezdxf runtime" OFF)
set(LIBDXFRW_EZDXF_PYTHON_EXECUTABLE "" CACHE FILEPATH
    "Existing Python interpreter with ezdxf exactly 1.4.4 (never auto-installed)")
set(LIBDXFRW_TARGET_GIT_DIR "" CACHE PATH
    "Git directory for the pinned LibreCAD target qualification build")
set(LIBDXFRW_LIBREDWG_DWGREAD "" CACHE FILEPATH
    "Pinned LibreDWG dwgread executable for semantic field qualification")

if(LIBDXFRW_BUILD_DWG2DXF AND NOT LIBDXFRW_BUILD_LIBRARY)
    message(FATAL_ERROR "LIBDXFRW_BUILD_DWG2DXF requires LIBDXFRW_BUILD_LIBRARY")
endif()
if(LIBDXFRW_BUILD_TESTS AND NOT LIBDXFRW_BUILD_LIBRARY)
    message(FATAL_ERROR "LIBDXFRW_BUILD_TESTS requires LIBDXFRW_BUILD_LIBRARY")
endif()
if(LIBDXFRW_BUILD_LONG_FUZZ AND NOT LIBDXFRW_BUILD_TESTS)
    message(FATAL_ERROR "LIBDXFRW_BUILD_LONG_FUZZ requires LIBDXFRW_BUILD_TESTS")
endif()
if(LIBDXFRW_BUILD_QUALIFICATION AND NOT LIBDXFRW_BUILD_TESTS)
    message(FATAL_ERROR "LIBDXFRW_BUILD_QUALIFICATION requires LIBDXFRW_BUILD_TESTS")
endif()
if(LIBDXFRW_ENABLE_INDEPENDENT_DXF_ORACLE AND
        (NOT LIBDXFRW_BUILD_TESTS OR NOT LIBDXFRW_BUILD_DWG2DXF))
    message(FATAL_ERROR "Independent DXF oracle requires tests and dwg2dxf")
endif()

# Keep warnings target-local so consumers are not forced to inherit the
# library's maintainer diagnostics policy.
if (MSVC)
    set(LIBDXFRW_WARNING_FLAGS /W3)
    set(LIBDXFRW_ADAPTER_WARNING_FLAGS /W3 /WX /bigobj)
    # The adapter is compiled into several test targets and also as a shared
    # object-library source. Keep the large-object option attached to the
    # source so every MSVC compilation path gets it.
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf/dx_iface.cpp
        PROPERTIES COMPILE_OPTIONS /bigobj)
else()
    set(LIBDXFRW_WARNING_FLAGS -Wall -Wextra -pedantic -Werror)
    set(LIBDXFRW_ADAPTER_WARNING_FLAGS ${LIBDXFRW_WARNING_FLAGS})
endif()


include(GNUInstallDirs)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The pinned source list is the completeness oracle once the target snapshot
# is present.  It is activated atomically with the imported source tree.
include(${CMAKE_CURRENT_SOURCE_DIR}/libdxfrw_sources.cmake)
set(libdxfrw_srcs ${LIBDXFRW_SOURCES})
set(libdxfrw_hdrs ${LIBDXFRW_PRIVATE_HEADERS} ${LIBDXFRW_PUBLIC_HEADERS})
set(installdxfrw_hdrs ${LIBDXFRW_PUBLIC_HEADERS})
set(libdxfrw_transitive_hdrs
    src/intern/dwg_fixed_handles.h
    src/intern/dwgutil.h
    src/intern/dwgbufferw.h
    src/intern/dwgsafety.h
)

set(LIBDXFRW_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR}/libdxfrw)

if(LIBDXFRW_BUILD_LIBRARY)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)
    add_library(dxfrw ${libdxfrw_srcs} ${libdxfrw_hdrs})
    set_target_properties(dxfrw PROPERTIES
        VERSION "${PROJECT_VERSION}"
        SOVERSION "${PROJECT_VERSION_MAJOR}"
        EXPORT_NAME libdxfrw
    )
    target_compile_features(dxfrw PUBLIC cxx_std_17)
    target_compile_options(dxfrw PRIVATE ${LIBDXFRW_WARNING_FLAGS})
    target_link_libraries(dxfrw PUBLIC Threads::Threads)
    if(MSVC)
        # The broad Wave 1 seam reaches five private dxfRW methods directly
        # through its test-only private->public header view.  Keep their
        # symbols externally emitted in MSVC static-library builds without
        # exporting them for ordinary consumers.  Gated on the tests: with it
        # defined unconditionally, an ordinary consumer's DLL exported those
        # five private methods and the class definition differed between the
        # library's translation units and everyone else's.
        if(LIBDXFRW_BUILD_TESTS)
            target_compile_definitions(dxfrw PRIVATE LIBDXFRW_FORCE_TEST_SEAMS=1)
        endif()
        # libdxfrw.cpp exceeds MSVC's default section limit on its own, so
        # /bigobj is needed whatever else is being built.
        target_compile_options(dxfrw PRIVATE /bigobj)

        # The two flags below exist for the Wave 1 test link, and are gated with
        # the seams they protect.
        #
        # Visual Studio Release enables whole-program optimization for static
        # libraries, which discards the explicitly exported seam declarations
        # before the broad Wave 1 executable can reference them; /Zc:inline may
        # discard otherwise unreferenced COMDAT methods from libdxfrw.cpp for
        # the same reason.  With LIBDXFRW_FORCE_TEST_SEAMS undefined there are
        # no such references, and leaving whole-program optimization off would
        # be a pessimisation charged to every consumer who never builds the
        # tests.
        if(LIBDXFRW_BUILD_TESTS)
            target_compile_options(dxfrw PRIVATE /GL-)
            set_source_files_properties(
                ${CMAKE_CURRENT_SOURCE_DIR}/src/libdxfrw.cpp
                PROPERTIES COMPILE_OPTIONS /Zc:inline-)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # Keep the byte-frozen entity parser source identical to the
        # qualification oracle.  GCC diagnoses two legacy enum/integer
        # conditional expressions in that frozen TU under -Wextra; contain
        # only that warning family for this source while retaining -Werror
        # for every other library translation unit.
        set_source_files_properties(
            ${CMAKE_CURRENT_SOURCE_DIR}/src/drw_entities.cpp
            PROPERTIES COMPILE_OPTIONS -Wno-extra)
    endif()
    target_include_directories(dxfrw
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
            $<INSTALL_INTERFACE:${LIBDXFRW_INSTALL_INCLUDEDIR}>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/intern
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
    # Keep the standalone compatibility and sample-driven harness in the
    # dedicated tests/ directory. Add it after the optional converter so CLI
    # integration registrations can check actual target availability. Adapter,
    # parser and LibreCAD_3 compile-only gates also work with the CLI disabled,
    # while all tests remain absent from normal library builds.
    # Resolve and validate Python before tests/ captures it in command vectors.
    find_program(LIBDXFRW_PYTHON_EXECUTABLE NAMES python3)
    if(NOT LIBDXFRW_PYTHON_EXECUTABLE)
        message(FATAL_ERROR "LIBDXFRW_BUILD_TESTS requires a Python 3 interpreter")
    endif()
    set(_libdxfrw_python_probe_timeout_seconds 5)
    execute_process(
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -c
            "import sys; raise SystemExit(0 if sys.version_info >= (3, 10) else 1)"
        RESULT_VARIABLE LIBDXFRW_PYTHON_VERSION_OK
        TIMEOUT ${_libdxfrw_python_probe_timeout_seconds}
    )
    if("${LIBDXFRW_PYTHON_VERSION_OK}" STREQUAL "1")
        message(FATAL_ERROR "LIBDXFRW_BUILD_TESTS requires Python 3.10 or newer")
    elseif(NOT "${LIBDXFRW_PYTHON_VERSION_OK}" STREQUAL "0")
        message(FATAL_ERROR
            "LIBDXFRW_BUILD_TESTS Python version probe failed (${LIBDXFRW_PYTHON_VERSION_OK})")
    endif()
    add_test(NAME libdxfrw_build_inventory
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_build_inventory.py --check)
    add_test(NAME libdxfrw_build_inventory_selftest
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_build_inventory.py --self-test)
    set_tests_properties(libdxfrw_build_inventory libdxfrw_build_inventory_selftest
        PROPERTIES LABELS "fast;build;package" TIMEOUT 15)
    add_subdirectory(tests)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/libdxfrwRootTestAdapters.cmake)
    # This compiler-only contract needs no library build or X11 dependency.
    # An explicit include root adds an actual X11 header control when available.
    set(LIBDXFRW_HEADER_X11_INCLUDE_DIR "" CACHE PATH
        "Optional X11 include root for public-header macro coexistence tests")
    set(_libdxfrw_header_x11_args "")
    if(LIBDXFRW_HEADER_X11_INCLUDE_DIR)
        list(APPEND _libdxfrw_header_x11_args --x11-include
            "${LIBDXFRW_HEADER_X11_INCLUDE_DIR}")
    endif()
    add_test(NAME libdxfrw_public_header_hygiene
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/public_header_hygiene_tests.py
            --compiler "${CMAKE_CXX_COMPILER}"
            --compiler-id "${CMAKE_CXX_COMPILER_ID}"
            --frontend-variant "${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}"
            --compiler-target "${CMAKE_CXX_COMPILER_TARGET}"
            --sysroot "${CMAKE_SYSROOT}"
            --cmake "${CMAKE_COMMAND}"
            --generator "${CMAKE_GENERATOR}"
            --generator-platform "${CMAKE_GENERATOR_PLATFORM}"
            --generator-toolset "${CMAKE_GENERATOR_TOOLSET}"
            --generator-instance "${CMAKE_GENERATOR_INSTANCE}"
            --toolchain "${CMAKE_TOOLCHAIN_FILE}"
            --make-program "${CMAKE_MAKE_PROGRAM}"
            --system-name "${CMAKE_SYSTEM_NAME}"
            --system-version "${CMAKE_SYSTEM_VERSION}"
            --system-processor "${CMAKE_SYSTEM_PROCESSOR}"
            --configuration "$<CONFIG>"
            --include-root "${CMAKE_CURRENT_SOURCE_DIR}/src"
            ${_libdxfrw_header_x11_args})
    set_tests_properties(libdxfrw_public_header_hygiene PROPERTIES
        LABELS "fast;headers;consumer" TIMEOUT 90)
    add_test(NAME libdxfrw_bounded_process
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/bounded_process_tests.py)
    set_tests_properties(libdxfrw_bounded_process PROPERTIES
        LABELS "fast;tools;process" TIMEOUT 15)
    if(LIBDXFRW_BUILD_DWG2DXF)
        add_test(NAME dwg2dxf_cli_contract
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg2dxf_cli_contract_tests.py
                --converter $<TARGET_FILE:dwg2dxf>
                --writer $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
                --fast-only)
        set_tests_properties(dwg2dxf_cli_contract PROPERTIES
            LABELS "fast;cli;process" TIMEOUT 25)
        add_test(NAME dwg2dxf_cli_partial_read_disposition
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg2dxf_cli_contract_tests.py
                --converter $<TARGET_FILE:dwg2dxf>
                --writer $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
                --partial-read-only)
        set_tests_properties(dwg2dxf_cli_partial_read_disposition PROPERTIES
            LABELS "fast;cli;dwg;disposition" TIMEOUT 30)
        add_test(NAME dwg2dxf_cli_section_settings_recovery
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg2dxf_cli_contract_tests.py
                --converter $<TARGET_FILE:dwg2dxf>
                --writer $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
                --section-settings-only)
        set_tests_properties(dwg2dxf_cli_section_settings_recovery PROPERTIES
            LABELS "fast;cli;dwg;disposition" TIMEOUT 20)
        add_test(NAME dwg2dxf_cli_pointcloud_colormap_recovery
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg2dxf_cli_contract_tests.py
                --converter $<TARGET_FILE:dwg2dxf>
                --writer $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
                --pointcloud-colormap-only)
        set_tests_properties(dwg2dxf_cli_pointcloud_colormap_recovery PROPERTIES
            LABELS "fast;cli;dwg;disposition" TIMEOUT 20)
        add_test(NAME dwg2dxf_cli_pointcloud_definition_recovery
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg2dxf_cli_contract_tests.py
                --converter $<TARGET_FILE:dwg2dxf>
                --writer $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
                --pointcloud-definition-only)
        set_tests_properties(dwg2dxf_cli_pointcloud_definition_recovery PROPERTIES
            LABELS "fast;cli;dwg;disposition" TIMEOUT 20)
        add_test(NAME dwg2dxf_cli_pointcloud_reactor_recovery
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg2dxf_cli_contract_tests.py
                --converter $<TARGET_FILE:dwg2dxf>
                --writer $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
                --pointcloud-reactor-only)
        set_tests_properties(dwg2dxf_cli_pointcloud_reactor_recovery PROPERTIES
            LABELS "fast;cli;dwg;disposition" TIMEOUT 20)
        add_test(NAME dwg2dxf_cli_sun_recovery
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg2dxf_cli_contract_tests.py
                --converter $<TARGET_FILE:dwg2dxf>
                --writer $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
                --sun-only)
        set_tests_properties(dwg2dxf_cli_sun_recovery PROPERTIES
            LABELS "fast;cli;dwg;disposition" TIMEOUT 20)
        add_test(NAME dwg2dxf_cli_sunstudy_recovery
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg2dxf_cli_contract_tests.py
                --converter $<TARGET_FILE:dwg2dxf>
                --writer $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
                --sunstudy-recovery-only)
        set_tests_properties(dwg2dxf_cli_sunstudy_recovery PROPERTIES
            LABELS "fast;cli;dwg;disposition" TIMEOUT 20)
        add_test(NAME dwg2dxf_cli_wipeoutvariables_recovery
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg2dxf_cli_contract_tests.py
                --converter $<TARGET_FILE:dwg2dxf>
                --writer $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
                --wipeoutvariables-recovery-only)
        set_tests_properties(dwg2dxf_cli_wipeoutvariables_recovery PROPERTIES
            LABELS "fast;cli;dwg;disposition" TIMEOUT 20)
        add_test(NAME dwg2dxf_cli_rastervariables_recovery
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg2dxf_cli_contract_tests.py
                --converter $<TARGET_FILE:dwg2dxf>
                --writer $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
                --rastervariables-recovery-only)
        set_tests_properties(dwg2dxf_cli_rastervariables_recovery PROPERTIES
            LABELS "fast;cli;dwg;disposition" TIMEOUT 20)
        add_test(NAME dwg2dxf_cli_mleaderstyle_recovery
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg2dxf_cli_contract_tests.py
                --converter $<TARGET_FILE:dwg2dxf>
                --writer $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
                --mleaderstyle-only)
        set_tests_properties(dwg2dxf_cli_mleaderstyle_recovery PROPERTIES
            LABELS "fast;cli;dwg;disposition" TIMEOUT 20)
        add_test(NAME dwg2dxf_cli_spatial_filter_recovery
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg2dxf_cli_contract_tests.py
                --converter $<TARGET_FILE:dwg2dxf>
                --writer $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
                --spatial-filter-only)
        set_tests_properties(dwg2dxf_cli_spatial_filter_recovery PROPERTIES
            LABELS "fast;cli;dwg;disposition" TIMEOUT 20)
        add_test(NAME dwg2dxf_cli_idbuffer_recovery
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg2dxf_cli_contract_tests.py
                --converter $<TARGET_FILE:dwg2dxf>
                --writer $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
                --idbuffer-only)
        set_tests_properties(dwg2dxf_cli_idbuffer_recovery PROPERTIES
            LABELS "fast;cli;dwg;disposition" TIMEOUT 20)
        add_test(NAME dwg2dxf_cli_layer_index_recovery
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg2dxf_cli_contract_tests.py
                --converter $<TARGET_FILE:dwg2dxf>
                --writer $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
                --layer-index-only)
        set_tests_properties(dwg2dxf_cli_layer_index_recovery PROPERTIES
            LABELS "fast;cli;dwg;disposition" TIMEOUT 20)
        add_test(NAME dwg2dxf_cli_spatial_index_recovery
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg2dxf_cli_contract_tests.py
                --converter $<TARGET_FILE:dwg2dxf>
                --writer $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
                --spatial-index-only)
        set_tests_properties(dwg2dxf_cli_spatial_index_recovery PROPERTIES
            LABELS "fast;cli;dwg;disposition" TIMEOUT 20)
        add_test(NAME dwg2dxf_cli_lightlist_recovery
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg2dxf_cli_contract_tests.py
                --converter $<TARGET_FILE:dwg2dxf>
                --writer $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
                --lightlist-only)
        set_tests_properties(dwg2dxf_cli_lightlist_recovery PROPERTIES
            LABELS "fast;cli;dwg;disposition" TIMEOUT 20)
        add_test(NAME dwg2dxf_cli_geodata_recovery
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg2dxf_cli_contract_tests.py
                --converter $<TARGET_FILE:dwg2dxf>
                --writer $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
                --geodata-only)
        set_tests_properties(dwg2dxf_cli_geodata_recovery PROPERTIES
            LABELS "fast;cli;dwg;disposition" TIMEOUT 20)
        add_test(NAME dwg2dxf_cli_tablestyle_recovery
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg2dxf_cli_contract_tests.py
                --converter $<TARGET_FILE:dwg2dxf>
                --writer $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
                --tablestyle-only)
        set_tests_properties(dwg2dxf_cli_tablestyle_recovery PROPERTIES
            LABELS "fast;cli;dwg;disposition" TIMEOUT 20)
        add_test(NAME dwg2dxf_cli_extended_contract
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg2dxf_cli_contract_tests.py
                --converter $<TARGET_FILE:dwg2dxf>
                --writer $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>)
        set_tests_properties(dwg2dxf_cli_extended_contract PROPERTIES
            LABELS "extended;cli;process" TIMEOUT 180)
        add_test(NAME dwg2dxf_modern_cli_versions
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg2dxf_cli_contract_tests.py
                --converter $<TARGET_FILE:dwg2dxf> --modern-versions-only)
        set_tests_properties(dwg2dxf_modern_cli_versions PROPERTIES
            LABELS "fast;cli;process;dxf;geometry;consumer" TIMEOUT 30)
        add_test(NAME dwg2dxf_unsupported_source_codepage
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg2dxf_cli_contract_tests.py
                --converter $<TARGET_FILE:dwg2dxf>
                --unsupported-dwg-codepage-only)
        set_tests_properties(dwg2dxf_unsupported_source_codepage PROPERTIES
            LABELS "fast;cli;dwg;codepage" TIMEOUT 25)
        add_test(NAME dwg2dxf_skipped_record_disposition
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg2dxf_cli_contract_tests.py
                --converter $<TARGET_FILE:dwg2dxf>
                --writer $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
                --skipped-record-only)
        set_tests_properties(dwg2dxf_skipped_record_disposition PROPERTIES
            LABELS "fast;cli;dwg;disposition" TIMEOUT 30)
        add_test(NAME dwg2dxf_historical_ac1021_role_refusal
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg2dxf_cli_contract_tests.py
                --converter $<TARGET_FILE:dwg2dxf>
                --historical-ac1021-only)
        set_tests_properties(dwg2dxf_historical_ac1021_role_refusal PROPERTIES
            LABELS "fast;cli;dwg;compatibility;disposition" TIMEOUT 15)
    endif()
    add_test(NAME libdxfrw_independent_dxf_selftest
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -I -B
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_independent_dxf.py --self-test)
    set_tests_properties(libdxfrw_independent_dxf_selftest PROPERTIES
        LABELS "fast;tools;dxf" TIMEOUT 15)
    if(LIBDXFRW_ENABLE_INDEPENDENT_DXF_ORACLE)
        if(NOT EXISTS "${LIBDXFRW_EZDXF_PYTHON_EXECUTABLE}")
            message(FATAL_ERROR "Set an existing LIBDXFRW_EZDXF_PYTHON_EXECUTABLE explicitly")
        endif()
        execute_process(COMMAND "${LIBDXFRW_EZDXF_PYTHON_EXECUTABLE}" -I -B
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_independent_dxf.py --check-runtime
            RESULT_VARIABLE _dxfrw_ezdxf_runtime_ok TIMEOUT 15)
        if(NOT "${_dxfrw_ezdxf_runtime_ok}" STREQUAL "0")
            message(FATAL_ERROR "Independent DXF oracle requires an isolated Python>=3.10 with ezdxf exactly1.4.4")
        endif()
        add_test(NAME libdxfrw_independent_dxf_runtime
            COMMAND "${LIBDXFRW_PYTHON_EXECUTABLE}" -I -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/independent_dxf_runtime_tests.py
                --oracle-python "${LIBDXFRW_EZDXF_PYTHON_EXECUTABLE}"
                --runner ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_independent_dxf.py)
        set_tests_properties(libdxfrw_independent_dxf_runtime PROPERTIES
            LABELS "advisory;independent;dxf;tools" TIMEOUT 60)
        foreach(_dxfrw_family IN ITEMS core spline-unit mtext-height header90)
            add_test(NAME libdxfrw_independent_dxf_${_dxfrw_family}
                COMMAND "${LIBDXFRW_EZDXF_PYTHON_EXECUTABLE}" -I -B
                    ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_independent_dxf.py
                    --converter $<TARGET_FILE:dwg2dxf> --family ${_dxfrw_family}
                    --timeout 5 --output
                    "${CMAKE_CURRENT_BINARY_DIR}/independent-dxf-${_dxfrw_family}.json")
            set_tests_properties(libdxfrw_independent_dxf_${_dxfrw_family} PROPERTIES
                LABELS "advisory;independent;dxf" TIMEOUT 60)
        endforeach()
    endif()
    set(LIBDXFRW_QUALIFIED_DIFFERENTIAL_MANIFEST
        ${CMAKE_CURRENT_SOURCE_DIR}/metadata/qualified-differential-v2.json)
    set(LIBDXFRW_QUALIFIED_SEMANTIC_FIELDS_CONTRACT
        ${CMAKE_CURRENT_SOURCE_DIR}/metadata/admitted-candidate-fields-v1.json)
    set(LIBDXFRW_AC1024_CANDIDATE_FIELDS_CONTRACT
        ${CMAKE_CURRENT_SOURCE_DIR}/metadata/ac1024-candidate-fields-v1.json)
    set(LIBDXFRW_AC1024_CANDIDATE_FIELDS_V2_CONTRACT
        ${CMAKE_CURRENT_SOURCE_DIR}/metadata/ac1024-candidate-fields-v2.json)

    if(LIBDXFRW_BUILD_QUALIFICATION)
        if(NOT LIBDXFRW_BUILD_LIBRARY)
            message(FATAL_ERROR
                "LIBDXFRW_BUILD_QUALIFICATION requires LIBDXFRW_BUILD_LIBRARY")
        endif()
        if(BUILD_SHARED_LIBS)
            message(FATAL_ERROR
                "LIBDXFRW_BUILD_QUALIFICATION requires a static libdxfrw build")
        endif()
        if(NOT LIBDXFRW_TARGET_GIT_DIR)
            message(FATAL_ERROR
                "LIBDXFRW_BUILD_QUALIFICATION requires LIBDXFRW_TARGET_GIT_DIR")
        endif()
        if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
            message(FATAL_ERROR
                "LIBDXFRW_BUILD_QUALIFICATION requires an explicit "
                "CMAKE_BUILD_TYPE for a single-config generator")
        endif()
        if(NOT EXISTS "${LIBDXFRW_TARGET_GIT_DIR}")
            message(FATAL_ERROR
                "LIBDXFRW_TARGET_GIT_DIR does not exist: ${LIBDXFRW_TARGET_GIT_DIR}")
        endif()
        if(NOT LIBDXFRW_LIBREDWG_DWGREAD)
            message(FATAL_ERROR
                "LIBDXFRW_BUILD_QUALIFICATION requires "
                "LIBDXFRW_LIBREDWG_DWGREAD")
        endif()
        if(NOT EXISTS "${LIBDXFRW_LIBREDWG_DWGREAD}")
            message(FATAL_ERROR
                "LIBDXFRW_LIBREDWG_DWGREAD does not exist: "
                "${LIBDXFRW_LIBREDWG_DWGREAD}")
        endif()

        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic_differential_adapter.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/generated_raw_dwg_object.h
            ${LIBDXFRW_QUALIFIED_DIFFERENTIAL_MANIFEST}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_admitted_candidate_fields.py
            ${LIBDXFRW_QUALIFIED_SEMANTIC_FIELDS_CONTRACT}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_ac1024_candidate_fields.py
            ${LIBDXFRW_AC1024_CANDIDATE_FIELDS_CONTRACT}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/ac1024_candidate_fields_v2_live.py
            ${LIBDXFRW_AC1024_CANDIDATE_FIELDS_V2_CONTRACT})

        add_test(NAME libdxfrw_qualified_differential_v2_schema
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_qualified_differential_v2.py
                --self-test
                --manifest ${LIBDXFRW_QUALIFIED_DIFFERENTIAL_MANIFEST})
        add_test(NAME libdxfrw_semantic_adapter_build_helper
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_semantic_adapter.py
                --self-test)
        set_tests_properties(libdxfrw_semantic_adapter_build_helper PROPERTIES
            LABELS qualification)
        set_tests_properties(libdxfrw_qualified_differential_v2_schema PROPERTIES
            LABELS qualification)

        set(LIBDXFRW_SEMANTIC_QUALIFICATION_DIR
            ${CMAKE_CURRENT_BINARY_DIR}/semantic-qualification/$<CONFIG>)
        set(LIBDXFRW_SEMANTIC_TARGET_BUILD_DIR
            ${LIBDXFRW_SEMANTIC_QUALIFICATION_DIR}/target)
        set(LIBDXFRW_SEMANTIC_TARGET_RECEIPT
            ${LIBDXFRW_SEMANTIC_TARGET_BUILD_DIR}/semantic-adapter-target-receipt.json)
        set(LIBDXFRW_SEMANTIC_STANDALONE_RECEIPT
            ${LIBDXFRW_SEMANTIC_QUALIFICATION_DIR}/semantic-adapter-standalone-receipt.json)

        add_custom_target(libdxfrw_semantic_adapter_target
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_semantic_adapter.py
                build-target
                --target-git-dir ${LIBDXFRW_TARGET_GIT_DIR}
                --build-dir ${LIBDXFRW_SEMANTIC_TARGET_BUILD_DIR}
                --parent-cmake-build-dir ${CMAKE_CURRENT_BINARY_DIR}
                --lock ${CMAKE_CURRENT_SOURCE_DIR}/metadata/libdxfrw-target-lock.json
                --manifest ${CMAKE_CURRENT_SOURCE_DIR}/metadata/libdxfrw-target-source-manifest.txt
                --semantic-manifest ${LIBDXFRW_QUALIFIED_DIFFERENTIAL_MANIFEST}
                --adapter-source ${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic_differential_adapter.cpp
                --adapter-support-input ${CMAKE_CURRENT_SOURCE_DIR}/tests/generated_raw_dwg_object.h
                --wrapper-source ${CMAKE_CURRENT_SOURCE_DIR}/cmake/semantic_adapter_target
                --configuration $<CONFIG>
                --parallel 2
                --receipt ${LIBDXFRW_SEMANTIC_TARGET_RECEIPT}
                --cmake ${CMAKE_COMMAND}
            DEPENDS
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_semantic_adapter.py
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic_differential_adapter.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/generated_raw_dwg_object.h
                ${CMAKE_CURRENT_SOURCE_DIR}/cmake/semantic_adapter_target/CMakeLists.txt
                ${CMAKE_CURRENT_SOURCE_DIR}/metadata/libdxfrw-target-lock.json
                ${CMAKE_CURRENT_SOURCE_DIR}/metadata/libdxfrw-target-source-manifest.txt
                ${LIBDXFRW_QUALIFIED_DIFFERENTIAL_MANIFEST}
            COMMENT "Building receipt-bound pinned-target semantic adapter"
            VERBATIM)

        add_custom_target(libdxfrw_semantic_adapter_standalone_receipt
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_semantic_adapter.py
                build-standalone
                --source-root ${CMAKE_CURRENT_SOURCE_DIR}
                --source-list ${CMAKE_CURRENT_SOURCE_DIR}/libdxfrw_sources.cmake
                --build-dir ${LIBDXFRW_SEMANTIC_QUALIFICATION_DIR}/standalone-build
                --parent-cmake-build-dir ${CMAKE_CURRENT_BINARY_DIR}
                --adapter-source ${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic_differential_adapter.cpp
                --adapter-support-input ${CMAKE_CURRENT_SOURCE_DIR}/tests/generated_raw_dwg_object.h
                --semantic-manifest ${LIBDXFRW_QUALIFIED_DIFFERENTIAL_MANIFEST}
                --configuration $<CONFIG>
                --parallel 2
                --receipt ${LIBDXFRW_SEMANTIC_STANDALONE_RECEIPT}
                --cmake ${CMAKE_COMMAND}
            DEPENDS
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_semantic_adapter.py
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic_differential_adapter.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/generated_raw_dwg_object.h
                ${CMAKE_CURRENT_SOURCE_DIR}/libdxfrw_sources.cmake
                ${LIBDXFRW_QUALIFIED_DIFFERENTIAL_MANIFEST}
            COMMENT "Building a fresh controlled standalone semantic adapter"
            VERBATIM)
        add_custom_target(libdxfrw_semantic_adapters ALL
            DEPENDS
                libdxfrw_semantic_adapter_target
                libdxfrw_semantic_adapter_standalone_receipt)

        file(SHA256
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic_differential_adapter.cpp
            LIBDXFRW_SEMANTIC_RECIPE_GENERATOR_DIGEST)
        set(LIBDXFRW_QUALIFIED_DIFFERENTIAL_COMMON
            --manifest ${LIBDXFRW_QUALIFIED_DIFFERENTIAL_MANIFEST}
            --target-receipt ${LIBDXFRW_SEMANTIC_TARGET_RECEIPT}
            --standalone-receipt ${LIBDXFRW_SEMANTIC_STANDALONE_RECEIPT}
            --input-git-dir ${LIBDXFRW_TARGET_GIT_DIR})

        add_test(NAME libdxfrw_semantic_adapters_setup
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
                --target libdxfrw_semantic_adapters
                --config $<CONFIG>
                --parallel 1)
        set_tests_properties(libdxfrw_semantic_adapters_setup PROPERTIES
            DEPENDS "libdxfrw_qualified_differential_v2_schema;libdxfrw_semantic_adapter_build_helper"
            FIXTURES_SETUP libdxfrw_semantic_adapters_fixture
            LABELS qualification)

        add_test(NAME libdxfrw_qualified_differential_v2
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_qualified_differential_v2.py
                ${LIBDXFRW_QUALIFIED_DIFFERENTIAL_COMMON}
                --input ${CMAKE_CURRENT_SOURCE_DIR}/metadata/semantic-minimal-line-v1.json
                --input-id recipe:minimal-line-v1
                --input-path-hint metadata/semantic-minimal-line-v1.json
                --origin-kind localFromScratch
                --generator-digest ${LIBDXFRW_SEMANTIC_RECIPE_GENERATOR_DIGEST}
                --facade dwgRW
                --direction write
                --output ${LIBDXFRW_SEMANTIC_QUALIFICATION_DIR}/qualified-differential-dwg-write.json)
        add_test(NAME libdxfrw_qualified_differential_v2_dxf_write
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_qualified_differential_v2.py
                ${LIBDXFRW_QUALIFIED_DIFFERENTIAL_COMMON}
                --input ${CMAKE_CURRENT_SOURCE_DIR}/metadata/semantic-minimal-line-v1.json
                --input-id recipe:minimal-line-v1
                --input-path-hint metadata/semantic-minimal-line-v1.json
                --origin-kind localFromScratch
                --generator-digest ${LIBDXFRW_SEMANTIC_RECIPE_GENERATOR_DIGEST}
                --facade dxfRW
                --direction write
                --output ${LIBDXFRW_SEMANTIC_QUALIFICATION_DIR}/qualified-differential-dxf-write.json)
        add_test(NAME libdxfrw_qualified_differential_v2_dxf_write_failure
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_qualified_differential_v2.py
                ${LIBDXFRW_QUALIFIED_DIFFERENTIAL_COMMON}
                --input ${CMAKE_CURRENT_SOURCE_DIR}/metadata/semantic-minimal-line-v1.json
                --input-id recipe:minimal-line-v1
                --input-path-hint metadata/semantic-minimal-line-v1.json
                --origin-kind localFromScratch
                --generator-digest ${LIBDXFRW_SEMANTIC_RECIPE_GENERATOR_DIGEST}
                --facade dxfRW
                --direction write
                --expect-operation-failure
                --expected-failure-id failure:dxf-write-existing-directory
                --output ${LIBDXFRW_SEMANTIC_QUALIFICATION_DIR}/qualified-differential-dxf-write-failure.json)
        add_test(NAME libdxfrw_qualified_differential_v2_dxf_read
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_qualified_differential_v2.py
                ${LIBDXFRW_QUALIFIED_DIFFERENTIAL_COMMON}
                --input ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/dxf/classes_raw_entity_r2007.dxf
                --input-id fixture:dxf:classes-raw-entity-r2007
                --input-path-hint tests/fixtures/dxf/classes_raw_entity_r2007.dxf
                --origin-kind lockedRepositoryBlob
                --input-repository LibreCAD/LibreCAD
                --input-commit 3c7785ebbcbfc8f3c8f79dbba093aff09cdec753
                --facade dxfRW
                --direction read
                --output ${LIBDXFRW_SEMANTIC_QUALIFICATION_DIR}/qualified-differential-dxf-read.json)
        add_test(NAME libdxfrw_qualified_differential_v2_dwg_read
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_qualified_differential_v2.py
                ${LIBDXFRW_QUALIFIED_DIFFERENTIAL_COMMON}
                --input ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/dwg/rtext_arctext.dwg
                --input-id fixture:rtext_arctext
                --input-path-hint tests/fixtures/dwg/rtext_arctext.dwg
                --origin-kind lockedRepositoryBlob
                --input-repository LibreCAD/LibreCAD
                --input-commit 3c7785ebbcbfc8f3c8f79dbba093aff09cdec753
                --facade dwgRW
                --direction read
                --output ${LIBDXFRW_SEMANTIC_QUALIFICATION_DIR}/qualified-differential-dwg-read.json)
        add_test(NAME libdxfrw_qualified_semantic_fields
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_admitted_candidate_fields.py
                --root ${CMAKE_CURRENT_SOURCE_DIR}
                --contract ${LIBDXFRW_QUALIFIED_SEMANTIC_FIELDS_CONTRACT}
                --manifest ${LIBDXFRW_QUALIFIED_DIFFERENTIAL_MANIFEST}
                --target-receipt ${LIBDXFRW_SEMANTIC_TARGET_RECEIPT}
                --standalone-receipt ${LIBDXFRW_SEMANTIC_STANDALONE_RECEIPT}
                --input-git-dir ${LIBDXFRW_TARGET_GIT_DIR}
                --dwgread ${LIBDXFRW_LIBREDWG_DWGREAD}
                --output ${LIBDXFRW_SEMANTIC_QUALIFICATION_DIR}/qualified-semantic-fields.json)
        add_test(NAME libdxfrw_ac1024_candidate_fields
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_ac1024_candidate_fields.py
                --manifest ${LIBDXFRW_AC1024_CANDIDATE_FIELDS_CONTRACT}
                --writer $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
                --oracle ${LIBDXFRW_LIBREDWG_DWGREAD}
                --target-receipt ${LIBDXFRW_SEMANTIC_TARGET_RECEIPT}
                --standalone-receipt ${LIBDXFRW_SEMANTIC_STANDALONE_RECEIPT}
                --expect-excluded-claims
                    rtext-insertion-x
                    rtext-insertion-y
                    rtext-insertion-z
                    rtext-rotation)
        add_test(NAME libdxfrw_ac1024_candidate_fields_v2
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/ac1024_candidate_fields_v2_live.py
                --manifest ${LIBDXFRW_AC1024_CANDIDATE_FIELDS_V2_CONTRACT}
                --writer $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
                --oracle ${LIBDXFRW_LIBREDWG_DWGREAD}
                --target-receipt ${LIBDXFRW_SEMANTIC_TARGET_RECEIPT}
                --standalone-receipt ${LIBDXFRW_SEMANTIC_STANDALONE_RECEIPT})
        set_tests_properties(
            libdxfrw_qualified_differential_v2
            libdxfrw_qualified_differential_v2_dxf_write
            libdxfrw_qualified_differential_v2_dxf_write_failure
            libdxfrw_qualified_differential_v2_dxf_read
            libdxfrw_qualified_differential_v2_dwg_read
            PROPERTIES
                DEPENDS libdxfrw_qualified_differential_v2_schema
                FIXTURES_REQUIRED libdxfrw_semantic_adapters_fixture
                LABELS qualification)
        set_tests_properties(libdxfrw_qualified_semantic_fields PROPERTIES
            DEPENDS libdxfrw_qualified_differential_v2_schema
            FIXTURES_REQUIRED libdxfrw_semantic_adapters_fixture
            LABELS qualification)
        set_tests_properties(libdxfrw_ac1024_candidate_fields PROPERTIES
            DEPENDS libdxfrw_qualified_differential_v2_schema
            FIXTURES_REQUIRED libdxfrw_semantic_adapters_fixture
            LABELS qualification)
        set_tests_properties(libdxfrw_ac1024_candidate_fields_v2 PROPERTIES
            DEPENDS libdxfrw_qualified_differential_v2_schema
            FIXTURES_REQUIRED libdxfrw_semantic_adapters_fixture
            LABELS qualification)
    else()
        add_test(NAME libdxfrw_qualified_differential_v2
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_qualified_differential_v2.py
                --self-test
                --manifest ${LIBDXFRW_QUALIFIED_DIFFERENTIAL_MANIFEST})
        add_test(NAME libdxfrw_qualified_semantic_fields
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_admitted_candidate_fields.py
                --self-test)
        if(WIN32)
            # The frozen v1 checker self-test probes /usr/bin/env.  Keep that
            # checker byte-identical and use a tiny host adapter only for the
            # Windows self-test path; live candidate qualification is unchanged.
            add_test(NAME libdxfrw_ac1024_candidate_fields
                COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                    ${CMAKE_CURRENT_SOURCE_DIR}/tests/ac1024_candidate_self_test.py
                    --checker v1
                    --manifest ${LIBDXFRW_AC1024_CANDIDATE_FIELDS_CONTRACT})
            add_test(NAME libdxfrw_ac1024_candidate_fields_v2
                COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                    ${CMAKE_CURRENT_SOURCE_DIR}/tests/ac1024_candidate_self_test.py
                    --checker v2
                    --manifest ${LIBDXFRW_AC1024_CANDIDATE_FIELDS_V2_CONTRACT})
        else()
            add_test(NAME libdxfrw_ac1024_candidate_fields
                COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                    ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_ac1024_candidate_fields.py
                    --self-test
                    --manifest ${LIBDXFRW_AC1024_CANDIDATE_FIELDS_CONTRACT})
            add_test(NAME libdxfrw_ac1024_candidate_fields_v2
                COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                    ${CMAKE_CURRENT_SOURCE_DIR}/tests/ac1024_candidate_fields_v2_live.py
                    --self-test
                    --manifest ${LIBDXFRW_AC1024_CANDIDATE_FIELDS_V2_CONTRACT})
        endif()
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
            ${CMAKE_CURRENT_SOURCE_DIR}/metadata/parity-differential-runners-v2.json)
    add_test(NAME libdxfrw_json_target_package_differential
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_json_target_package_differential.py
            --self-test)
    add_test(NAME libdxfrw_differential_debt
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_differential_debt.py
            --self-test)
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
            --manifest ${CMAKE_CURRENT_SOURCE_DIR}/metadata/parity-differential-runners-v1.json
            --support-matrix ${CMAKE_CURRENT_SOURCE_DIR}/metadata/support-matrix-v1.json)
    add_test(NAME libdxfrw_support_matrix
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_support_matrix.py
            --mapping ${CMAKE_CURRENT_SOURCE_DIR}/metadata/parity-source-routes-v1.json
            --registry ${CMAKE_CURRENT_SOURCE_DIR}/metadata/parity-test-oracles-v1.json
            --matrix ${CMAKE_CURRENT_SOURCE_DIR}/metadata/support-matrix-v1.json)
    add_test(NAME libdxfrw_runtime_evidence_queue
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_runtime_evidence_queue.py
            --registry ${CMAKE_CURRENT_SOURCE_DIR}/metadata/parity-test-oracles-v1.json
            --advisory ${CMAKE_CURRENT_SOURCE_DIR}/metadata/external-advisory-report.json)
    add_test(NAME libdxfrw_target_differential_advisory
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_target_differential_advisory.py
            --self-test)
    add_test(NAME libdxfrw_target_advisory_differential
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_target_advisory_differential.py
            --self-test)
    add_test(NAME libdxfrw_libredwg_advisory_differential
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_libredwg_advisory_differential.py
            --self-test)
    add_test(NAME libdxfrw_libredwg_ac1024_advisory
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_libredwg_ac1024_advisory.py
            --self-test)
    add_test(NAME libdxfrw_external_evidence_resolution
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_external_evidence_resolution.py
            --self-test)
    add_test(NAME libdxfrw_external_advisory_runner
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_external_advisory.py
            --self-test)
    add_test(NAME libdxfrw_local_dwg_oracle_advisory
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_local_dwg_oracle_advisory.py
            --self-test)
    add_test(NAME libdxfrw_local_dwg_oracle_matrix
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_local_dwg_oracle_matrix.py
            --self-test)
    add_test(NAME libdxfrw_local_dwg_object_oracle
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_local_dwg_object_oracle.py
            --self-test)
    add_test(NAME libdxfrw_qualified_support_metadata
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_qualified_format_support.py
            --self-test)
    add_test(NAME libdxfrw_fast_focus
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_fast_focus.py
            --build-dir ${CMAKE_BINARY_DIR})

    # Opt-in source-span research tool. It accepts a user-supplied AC1021
    # drawing and is intentionally not registered as a fixture-based test.
    add_executable(libdxfrw_ac1021_field_source_raw_probe EXCLUDE_FROM_ALL
        tests/ac1021_field_source_raw_probe.cpp
        ${_libdxfrw_reader_test_adapter_source})
    target_compile_features(libdxfrw_ac1021_field_source_raw_probe PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_ac1021_field_source_raw_probe PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intern
        ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf)
    target_link_libraries(libdxfrw_ac1021_field_source_raw_probe PRIVATE dxfrw)
    add_test(NAME libdxfrw_ac1021_field_source_span_self_test
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/ac1021_field_source_span_probe.py --self-test)
    set_tests_properties(libdxfrw_ac1021_field_source_span_self_test PROPERTIES
        LABELS "fast;dwg;field;generated")

    # Optional physical modeler/AcDs probe; no drawing is bundled as a test.
    add_executable(libdxfrw_ac1027_acds_source_probe EXCLUDE_FROM_ALL
        tests/ac1027_acds_source_probe.cpp
        ${_libdxfrw_reader_test_adapter_source})
    target_compile_features(libdxfrw_ac1027_acds_source_probe PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_ac1027_acds_source_probe PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intern
        ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf)
    target_link_libraries(libdxfrw_ac1027_acds_source_probe PRIVATE dxfrw)
    add_test(NAME libdxfrw_ac1027_acds_source_compare_self_test
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/ac1027_acds_source_compare.py --self-test)
    set_tests_properties(libdxfrw_ac1027_acds_source_compare_self_test PROPERTIES
        LABELS "fast;dwg;acds;generated")
    add_test(NAME libdxfrw_ac1027_modeler_uid_span_self_test
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/ac1027_modeler_uid_span.py --self-test)
    set_tests_properties(libdxfrw_ac1027_modeler_uid_span_self_test PROPERTIES
        LABELS "fast;dwg;acds;generated")

    add_executable(libdxfrw_wave1_tests
        tests/wave1_tests.cpp
        ${_libdxfrw_wave1_adapter_source})
    target_compile_features(libdxfrw_wave1_tests PRIVATE cxx_std_17)
    if(MSVC)
        target_compile_definitions(libdxfrw_wave1_tests PRIVATE
            LIBDXFRW_FORCE_TEST_SEAMS=1)
    endif()
    target_include_directories(libdxfrw_wave1_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intern
        ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf)
    target_link_libraries(libdxfrw_wave1_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_wave1 COMMAND libdxfrw_wave1_tests)
    add_test(NAME libdxfrw_pointcloud_definition_source_spans
        COMMAND libdxfrw_wave1_tests --pointcloud-definition-source-spans)
    set_tests_properties(libdxfrw_pointcloud_definition_source_spans PROPERTIES
        LABELS "fast;dxf;objects;pointcloud;generated;source-spans"
        TIMEOUT 30)
    add_test(NAME libdxfrw_wipeoutvariables_display_mode_domain
        COMMAND libdxfrw_wave1_tests --wipeoutvariables-display-mode-domain)
    set_tests_properties(libdxfrw_wipeoutvariables_display_mode_domain PROPERTIES
        LABELS "fast;dxf;writer;objects;wipeout;generated;transaction" TIMEOUT 15)
    add_test(NAME libdxfrw_dxf_writer_emitted_handle_inventory
        COMMAND libdxfrw_wave1_tests --dxf-writer-emitted-handle-inventory)
    set_tests_properties(libdxfrw_dxf_writer_emitted_handle_inventory PROPERTIES
        LABELS "fast;dxf;writer;inventory;generated;transaction" TIMEOUT 30)
    add_test(NAME libdxfrw_pointcloud_reference_remapping
        COMMAND libdxfrw_wave1_tests --pointcloud-reference-remapping)
    set_tests_properties(libdxfrw_pointcloud_reference_remapping PROPERTIES
        LABELS "fast;dxf;writer;pointcloud;references;generated"
        TIMEOUT 15)
    add_test(NAME libdxfrw_pointcloud_definition_identity_remapping
        COMMAND libdxfrw_wave1_tests
            --pointcloud-definition-identity-remapping)
    set_tests_properties(libdxfrw_pointcloud_definition_identity_remapping
        PROPERTIES LABELS "fast;dxf;writer;pointcloud;identity;generated;transaction"
        TIMEOUT 15)
    add_test(NAME libdxfrw_external_definition_identity_remapping
        COMMAND libdxfrw_wave1_tests
            --external-definition-identity-remapping)
    set_tests_properties(libdxfrw_external_definition_identity_remapping
        PROPERTIES LABELS "fast;dxf;writer;objects;identity;generated;transaction"
        TIMEOUT 15)
    add_test(NAME libdxfrw_raster_wipeout_variables_identity_remapping
        COMMAND libdxfrw_wave1_tests
            --raster-wipeout-variables-identity-remapping)
    set_tests_properties(libdxfrw_raster_wipeout_variables_identity_remapping
        PROPERTIES LABELS "fast;dxf;writer;objects;identity;generated;transaction"
        TIMEOUT 15)
    add_test(NAME libdxfrw_geodata_identity_remapping
        COMMAND libdxfrw_wave1_tests --geodata-identity-remapping)
    set_tests_properties(libdxfrw_geodata_identity_remapping
        PROPERTIES LABELS "fast;dxf;writer;geodata;identity;generated;transaction"
        TIMEOUT 15)
    add_test(NAME libdxfrw_geodata_versioned_writer
        COMMAND libdxfrw_wave1_tests --geodata-versioned-writer)
    set_tests_properties(libdxfrw_geodata_versioned_writer
        PROPERTIES LABELS "fast;dxf;writer;geodata;generated;transaction"
        TIMEOUT 15)
    add_test(NAME libdxfrw_dbcolor_identity_remapping
        COMMAND libdxfrw_wave1_tests --dbcolor-identity-remapping)
    set_tests_properties(libdxfrw_dbcolor_identity_remapping
        PROPERTIES LABELS "fast;dxf;writer;objects;identity;generated;transaction"
        TIMEOUT 15)
    add_test(NAME libdxfrw_render_settings_identity_remapping
        COMMAND libdxfrw_wave1_tests --render-settings-identity-remapping)
    set_tests_properties(libdxfrw_render_settings_identity_remapping
        PROPERTIES LABELS "fast;dxf;writer;render-settings;identity;generated;transaction"
        TIMEOUT 15)
    add_test(NAME libdxfrw_blockrecord_insert_reference_remapping
        COMMAND libdxfrw_wave1_tests
            --blockrecord-insert-reference-remapping)
    set_tests_properties(libdxfrw_blockrecord_insert_reference_remapping PROPERTIES
        LABELS "fast;dxf;writer;block-record;references;generated;transaction"
        TIMEOUT 15)
    add_test(NAME libdxfrw_section_manager_reference_remapping
        COMMAND libdxfrw_wave1_tests --section-manager-reference-remapping)
    set_tests_properties(libdxfrw_section_manager_reference_remapping PROPERTIES
        LABELS "fast;dxf;writer;objects;references;generated;transaction"
        TIMEOUT 15)
    add_test(NAME libdxfrw_section_settings_reference_remapping
        COMMAND libdxfrw_wave1_tests --section-settings-reference-remapping)
    set_tests_properties(libdxfrw_section_settings_reference_remapping PROPERTIES
        LABELS "fast;dxf;writer;objects;references;generated;transaction"
        TIMEOUT 15)
    add_test(NAME libdxfrw_lofted_surface_path_reference_dxf_refusal
        COMMAND libdxfrw_wave1_tests
            --lofted-surface-path-reference-dxf-refusal)
    set_tests_properties(libdxfrw_lofted_surface_path_reference_dxf_refusal PROPERTIES
        LABELS "fast;dxf;writer;3d;references;generated;transaction"
        TIMEOUT 15)
    add_test(NAME libdxfrw_direct_surface_subtype_admission
        COMMAND libdxfrw_wave1_tests --direct-surface-subtype-admission)
    set_tests_properties(libdxfrw_direct_surface_subtype_admission PROPERTIES
        LABELS "fast;dxf;writer;3d;runtime-type;generated;transaction"
        TIMEOUT 15)
    add_test(NAME libdxfrw_layer_material_reference_remapping
        COMMAND libdxfrw_wave1_tests --layer-material-reference-remapping)
    set_tests_properties(libdxfrw_layer_material_reference_remapping PROPERTIES
        LABELS "fast;dxf;writer;layer;references;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_solid_history_reference_remapping
        COMMAND libdxfrw_wave1_tests --solid-history-reference-remapping)
    set_tests_properties(libdxfrw_solid_history_reference_remapping PROPERTIES
        LABELS "fast;dxf;writer;3d;references;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_table_reference_remapping
        COMMAND libdxfrw_wave1_tests --table-reference-remapping)
    set_tests_properties(libdxfrw_table_reference_remapping PROPERTIES
        LABELS "fast;dxf;writer;table;references;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_object_owner_reference_remapping
        COMMAND libdxfrw_wave1_tests --object-owner-reference-remapping)
    set_tests_properties(libdxfrw_object_owner_reference_remapping PROPERTIES
        LABELS "fast;dxf;writer;objects;references;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_block_reference_remapping
        COMMAND libdxfrw_wave1_tests --block-reference-remapping)
    set_tests_properties(libdxfrw_block_reference_remapping PROPERTIES
        LABELS "fast;dxf;writer;blocks;references;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_leader_annotation_reference_remapping
        COMMAND libdxfrw_wave1_tests
            --leader-annotation-reference-remapping)
    set_tests_properties(libdxfrw_leader_annotation_reference_remapping PROPERTIES
        LABELS "fast;dxf;writer;entities;references;generated;transaction"
        TIMEOUT 15)
    add_test(NAME libdxfrw_entity_reference_remapping
        COMMAND libdxfrw_wave1_tests --entity-reference-remapping)
    set_tests_properties(libdxfrw_entity_reference_remapping PROPERTIES
        LABELS "fast;dxf;writer;entities;references;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_view_reference_remapping
        COMMAND libdxfrw_wave1_tests --view-reference-remapping)
    set_tests_properties(libdxfrw_view_reference_remapping PROPERTIES
        LABELS "fast;dxf;writer;view;references;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_viewport_reference_remapping
        COMMAND libdxfrw_wave1_tests --viewport-reference-remapping)
    set_tests_properties(libdxfrw_viewport_reference_remapping PROPERTIES
        LABELS "fast;dxf;writer;viewport;references;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_vport_reference_remapping
        COMMAND libdxfrw_wave1_tests --vport-reference-remapping)
    set_tests_properties(libdxfrw_vport_reference_remapping PROPERTIES
        LABELS "fast;dxf;writer;vport;references;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_scene_object_reference_remapping
        COMMAND libdxfrw_wave1_tests --scene-object-reference-remapping)
    set_tests_properties(libdxfrw_scene_object_reference_remapping PROPERTIES
        LABELS "fast;dxf;writer;scene;references;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_sunstudy_dxf_hour_count
        COMMAND libdxfrw_wave1_tests --sunstudy-dxf-hour-count)
    set_tests_properties(libdxfrw_sunstudy_dxf_hour_count PROPERTIES
        LABELS "fast;dxf;writer;objects;sunstudy;generated;readback"
        TIMEOUT 20)
    add_test(NAME libdxfrw_lightlist_reference_remapping
        COMMAND libdxfrw_wave1_tests --lightlist-reference-remapping)
    set_tests_properties(libdxfrw_lightlist_reference_remapping PROPERTIES
        LABELS "fast;dxf;writer;objects;references;generated;transaction"
        TIMEOUT 15)
    add_test(NAME libdxfrw_partialviewingindex_reference_closure
        COMMAND libdxfrw_wave1_tests
            --partialviewingindex-reference-closure)
    set_tests_properties(libdxfrw_partialviewingindex_reference_closure
        PROPERTIES LABELS "fast;dxf;writer;objects;references;generated;transaction"
        TIMEOUT 15)
    add_test(NAME libdxfrw_layout_plotsettings_reference_remapping
        COMMAND libdxfrw_wave1_tests
            --layout-plotsettings-reference-remapping)
    set_tests_properties(libdxfrw_layout_plotsettings_reference_remapping
        PROPERTIES LABELS "fast;dxf;writer;layout;references;generated"
        TIMEOUT 15)
    add_test(NAME libdxfrw_navisworks_definition_reference_remapping
        COMMAND libdxfrw_wave1_tests
            --navisworks-definition-reference-remapping)
    set_tests_properties(libdxfrw_navisworks_definition_reference_remapping
        PROPERTIES LABELS "fast;dxf;writer;navisworks;references;generated"
        TIMEOUT 15)
    add_test(NAME libdxfrw_underlay_definition_reference_remapping
        COMMAND libdxfrw_wave1_tests
            --underlay-definition-reference-remapping)
    set_tests_properties(libdxfrw_underlay_definition_reference_remapping
        PROPERTIES LABELS "fast;dxf;writer;underlay;references;generated"
        TIMEOUT 15)
    add_test(NAME libdxfrw_wipeout_reference_remapping
        COMMAND libdxfrw_wave1_tests --wipeout-reference-remapping)
    set_tests_properties(libdxfrw_wipeout_reference_remapping
        PROPERTIES LABELS "fast;dxf;writer;wipeout;references;generated"
        TIMEOUT 15)
    add_test(NAME libdxfrw_dxf_standard_object_identities
        COMMAND libdxfrw_wave1_tests --dxf-standard-object-identities)
    set_tests_properties(libdxfrw_dxf_standard_object_identities PROPERTIES
        LABELS "fast;dxf;writer;objects;identity;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_generated_dictionary_source_metadata
        COMMAND libdxfrw_wave1_tests --generated-dictionary-source-metadata)
    set_tests_properties(libdxfrw_generated_dictionary_source_metadata PROPERTIES
        LABELS "fast;dxf;writer;objects;dictionary;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_dxf_objects_write_inspection
        COMMAND libdxfrw_wave1_tests --dxf-objects-write-inspection)
    set_tests_properties(libdxfrw_dxf_objects_write_inspection PROPERTIES
        LABELS "fast;dxf;writer;objects;transaction;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_associative_dependency_writer
        COMMAND libdxfrw_wave1_tests --associative-dependency-writer)
    set_tests_properties(libdxfrw_associative_dependency_writer PROPERTIES
        LABELS "fast;dxf;writer;associative;transaction;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_associative_network_writer
        COMMAND libdxfrw_wave1_tests --associative-network-writer)
    set_tests_properties(libdxfrw_associative_network_writer PROPERTIES
        LABELS "fast;dxf;writer;associative;transaction;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_associative_variable_writer
        COMMAND libdxfrw_wave1_tests --associative-variable-writer)
    set_tests_properties(libdxfrw_associative_variable_writer PROPERTIES
        LABELS "fast;dxf;writer;associative;transaction;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_associative_dependency_body_writer
        COMMAND libdxfrw_wave1_tests --associative-dependency-body-writer)
    set_tests_properties(libdxfrw_associative_dependency_body_writer PROPERTIES
        LABELS "fast;dxf;writer;associative;transaction;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_associative_constraint_group_writer
        COMMAND libdxfrw_wave1_tests --associative-constraint-group-writer)
    set_tests_properties(libdxfrw_associative_constraint_group_writer PROPERTIES
        LABELS "fast;dxf;writer;associative;transaction;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_common_reference_output_id_guard
        COMMAND libdxfrw_wave1_tests --common-reference-output-id-guard)
    set_tests_properties(libdxfrw_common_reference_output_id_guard PROPERTIES
        LABELS "fast;dxf;writer;objects;references;generated;transaction"
        TIMEOUT 15)
    add_test(NAME libdxfrw_common_reactor_count_bound
        COMMAND libdxfrw_wave1_tests --common-reactor-count-bound)
    set_tests_properties(libdxfrw_common_reactor_count_bound PROPERTIES
        LABELS "fast;dxf;writer;objects;references;resources;generated;transaction"
        TIMEOUT 15)
    add_test(NAME libdxfrw_image_definition_reactor_count_bound
        COMMAND libdxfrw_wave1_tests --image-definition-reactor-count-bound)
    set_tests_properties(libdxfrw_image_definition_reactor_count_bound PROPERTIES
        LABELS "fast;dxf;writer;objects;references;resources;generated;transaction"
        TIMEOUT 15)
    add_test(NAME libdxfrw_image_definition_count_bound
        COMMAND libdxfrw_wave1_tests --image-definition-count-bound)
    set_tests_properties(libdxfrw_image_definition_count_bound PROPERTIES
        LABELS "fast;dxf;writer;objects;references;resources;generated;transaction"
        TIMEOUT 15)
    add_test(NAME libdxfrw_dxf_tables_write_inspection
        COMMAND libdxfrw_symbol_output_tests --dxf-tables-write-inspection)
    set_tests_properties(libdxfrw_dxf_tables_write_inspection PROPERTIES
        LABELS "fast;dxf;writer;tables;transaction;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_dxf_objects_callback_bad_alloc
        COMMAND libdxfrw_symbol_output_tests --dxf-objects-callback-bad-alloc)
    set_tests_properties(libdxfrw_dxf_objects_callback_bad_alloc PROPERTIES
        LABELS "fast;dxf;writer;objects;transaction;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_symbol_name_wire_inspection
        COMMAND libdxfrw_symbol_output_tests --symbol-name-wire-inspection)
    set_tests_properties(libdxfrw_symbol_name_wire_inspection PROPERTIES
        LABELS "fast;dxf;tables;identity;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_dxf_source_table_identities
        COMMAND libdxfrw_symbol_output_tests --source-table-identities)
    set_tests_properties(libdxfrw_dxf_source_table_identities PROPERTIES
        LABELS "fast;dxf;writer;tables;identity;transaction;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_symbol_unicode_keys
        COMMAND libdxfrw_symbol_output_tests --symbol-unicode-keys)
    set_tests_properties(libdxfrw_symbol_unicode_keys PROPERTIES
        LABELS "fast;dxf;tables;unicode;generated" TIMEOUT 15)
    add_executable(libdxfrw_symbol_output_tests
        tests/dxf_symbol_output_tests.cpp ${_libdxfrw_reader_test_adapter_source})
    target_compile_features(libdxfrw_symbol_output_tests PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_symbol_output_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/intern
        ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf)
    target_link_libraries(libdxfrw_symbol_output_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_standard_object_wire_comparison
        COMMAND libdxfrw_wave1_tests --standard-object-wire-comparison)
    set_tests_properties(libdxfrw_standard_object_wire_comparison PROPERTIES
        LABELS "fast;dxf;objects;preservation;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_xrecord_xdata_channels
        COMMAND libdxfrw_dxf_fixture_tests --xrecord-xdata-channels)
    set_tests_properties(libdxfrw_xrecord_xdata_channels PROPERTIES
        LABELS "fast;dxf;objects;xrecord;xdata;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_xrecord_normal_string_codes
        COMMAND libdxfrw_dxf_fixture_tests --xrecord-normal-string-codes)
    set_tests_properties(libdxfrw_xrecord_normal_string_codes PROPERTIES
        LABELS "fast;dxf;objects;xrecord;compatibility;generated" TIMEOUT 30)
    add_test(NAME libdxfrw_xrecord_legacy_double_codes
        COMMAND libdxfrw_dxf_fixture_tests --xrecord-legacy-double-codes)
    add_test(NAME libdxfrw_xrecord_legacy_boolean_codes
        COMMAND libdxfrw_dxf_fixture_tests --xrecord-legacy-boolean-codes)
    set_tests_properties(libdxfrw_xrecord_legacy_double_codes libdxfrw_xrecord_legacy_boolean_codes PROPERTIES
        LABELS "fast;dxf;objects;xrecord;compatibility;generated" TIMEOUT 30)
    add_test(NAME libdxfrw_xrecord_writer_codepage
        COMMAND libdxfrw_gbk_codepage_tests --xrecord-writer-codepage)
    set_tests_properties(libdxfrw_xrecord_writer_codepage PROPERTIES
        LABELS "fast;dwg;writer;objects;codepage;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_text_decode_issue_accumulation
        COMMAND libdxfrw_gbk_codepage_tests --text-decode-issue-accumulation)
    set_tests_properties(libdxfrw_text_decode_issue_accumulation PROPERTIES
        LABELS "fast;codepage;generated" TIMEOUT 10)
    add_test(NAME libdxfrw_dwg_eed_text_provenance
        COMMAND libdxfrw_gbk_codepage_tests --dwg-eed-text-provenance)
    set_tests_properties(libdxfrw_dwg_eed_text_provenance PROPERTIES
        LABELS "fast;dwg;codepage;xdata;generated" TIMEOUT 10)
    add_test(NAME libdxfrw_xrecord_per_value_codepage
        COMMAND libdxfrw_gbk_codepage_tests --xrecord-per-value-codepage)
    set_tests_properties(libdxfrw_xrecord_per_value_codepage PROPERTIES
        LABELS "fast;dwg;codepage;xrecord;generated" TIMEOUT 10)
    add_test(NAME libdxfrw_appid_name_source
        COMMAND libdxfrw_gbk_codepage_tests --appid-name-source)
    set_tests_properties(libdxfrw_appid_name_source PROPERTIES
        LABELS "fast;dwg;codepage;appid;generated" TIMEOUT 10)
    add_test(NAME libdxfrw_layer_name_source
        COMMAND libdxfrw_gbk_codepage_tests --layer-name-source)
    set_tests_properties(libdxfrw_layer_name_source PROPERTIES
        LABELS "fast;dwg;codepage;layer;generated" TIMEOUT 10)
    add_test(NAME libdxfrw_standard_dwg_output_allocation
        COMMAND libdxfrw_gbk_codepage_tests --standard-dwg-output-allocation)
    set_tests_properties(libdxfrw_standard_dwg_output_allocation PROPERTIES
        LABELS "fast;dwg;dxf;objects;allocation;generated" TIMEOUT 30)
    add_test(NAME libdxfrw_sectionobject_indicator_color_wire
        COMMAND libdxfrw_wave1_tests --sectionobject-indicator-color-wire)
    set_tests_properties(libdxfrw_sectionobject_indicator_color_wire PROPERTIES
        LABELS "fast;dxf;writer;sectionobject" TIMEOUT 10)
    add_test(NAME libdxfrw_mleader_writer_version
        COMMAND libdxfrw_wave1_tests --mleader-writer-validation)
    set_tests_properties(libdxfrw_mleader_writer_version PROPERTIES
        LABELS "fast;dxf;writer;mleader"
        TIMEOUT 10)
    add_test(NAME libdxfrw_mline_style_dxf_defaults
        COMMAND libdxfrw_wave1_tests --mline-style-dxf-defaults)
    set_tests_properties(libdxfrw_mline_style_dxf_defaults PROPERTIES
        LABELS "fast;dxf;reader;mline;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_mline_style_validation
        COMMAND libdxfrw_wave1_tests --mline-style-validation)
    set_tests_properties(libdxfrw_mline_style_validation PROPERTIES
        LABELS "fast;dxf;writer;mlinestyle;cmc"
        TIMEOUT 10)
    add_test(NAME libdxfrw_sun_cmc_dxf_projection
        COMMAND libdxfrw_wave1_tests --sun-cmc-dxf-projection)
    set_tests_properties(libdxfrw_sun_cmc_dxf_projection PROPERTIES
        LABELS "fast;dxf;writer;sun;cmc"
        TIMEOUT 10)
    add_test(NAME libdxfrw_section_settings_cmc_dxf_projection
        COMMAND libdxfrw_wave1_tests --section-settings-cmc-dxf-projection)
    set_tests_properties(libdxfrw_section_settings_cmc_dxf_projection PROPERTIES
        LABELS "fast;dxf;writer;section-settings;cmc"
        TIMEOUT 10)
    add_test(NAME libdxfrw_acad_field_writer_candidate
        COMMAND libdxfrw_wave1_tests --acad-field-writer-candidate)
    set_tests_properties(libdxfrw_acad_field_writer_candidate PROPERTIES
        LABELS "fast;dxf;writer;field"
        TIMEOUT 10)
    add_test(NAME libdxfrw_field_dxf_versioned_wire_profiles
        COMMAND libdxfrw_wave1_tests --field-dxf-versioned-wire-profiles)
    set_tests_properties(libdxfrw_field_dxf_versioned_wire_profiles PROPERTIES
        LABELS "fast;dxf;writer;field;generated"
        TIMEOUT 10)
    add_test(NAME libdxfrw_field_reference_output_id_guard
        COMMAND libdxfrw_wave1_tests --field-reference-output-id-guard)
    set_tests_properties(libdxfrw_field_reference_output_id_guard PROPERTIES
        LABELS "fast;dxf;writer;field;references;generated;transaction"
        TIMEOUT 10)
    add_test(NAME libdxfrw_solid_nondefault_ocs
        COMMAND libdxfrw_wave1_tests --solid-nondefault-ocs)
    set_tests_properties(libdxfrw_solid_nondefault_ocs PROPERTIES
        LABELS "fast;dxf;reader;3d"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dwg_decompressor_size_bounds
        COMMAND libdxfrw_wave1_tests --dwg-decompressor-size-bounds)
    set_tests_properties(libdxfrw_dwg_decompressor_size_bounds PROPERTIES
        LABELS "fast;dwg;hardening"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_code_type_boundaries
        COMMAND libdxfrw_wave1_tests --dxf-code-type-boundaries)
    set_tests_properties(libdxfrw_dxf_code_type_boundaries PROPERTIES
        LABELS "fast;dxf;hardening"
        TIMEOUT 10)
    add_test(NAME libdxfrw_hatch_inactive_gradient_guard
        COMMAND libdxfrw_wave1_tests --hatch-gradient-dxf-grammar)
    set_tests_properties(libdxfrw_hatch_inactive_gradient_guard PROPERTIES
        LABELS "fast;dxf;hatch;gradient;generated"
        TIMEOUT 15)

    add_executable(libdxfrw_gbk_codepage_tests
        tests/gbk_codepage_tests.cpp
        dwg2dxf/dx_iface.cpp)
    target_compile_features(libdxfrw_gbk_codepage_tests PRIVATE cxx_std_17)
    target_compile_definitions(libdxfrw_gbk_codepage_tests PRIVATE
        LIBDXFRW_DWG_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/dwg")
    target_include_directories(libdxfrw_gbk_codepage_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intern
        ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(libdxfrw_gbk_codepage_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_gbk_codepage
        COMMAND libdxfrw_gbk_codepage_tests)
    add_test(NAME libdxfrw_dwg_codepage_snapshot
        COMMAND libdxfrw_gbk_codepage_tests --dwg-codepage-snapshot)
    set_tests_properties(libdxfrw_dwg_codepage_snapshot PROPERTIES
        LABELS "fast;dwg;codepage")
    add_test(NAME libdxfrw_standard_dwg_payload_profile_and_allocation
        COMMAND libdxfrw_gbk_codepage_tests --standard-dwg-payload-profile-and-allocation)
    add_test(NAME libdxfrw_generated_dictionary_metadata_allocation
        COMMAND libdxfrw_gbk_codepage_tests --generated-dictionary-metadata-allocation)
    set_tests_properties(libdxfrw_generated_dictionary_metadata_allocation PROPERTIES
        LABELS "fast;dxf;dictionary;allocation;generated" TIMEOUT 15)
    set_tests_properties(libdxfrw_standard_dwg_payload_profile_and_allocation PROPERTIES
        LABELS "fast;dwg;codepage;adapter;allocation;generated" TIMEOUT 10)
    add_test(NAME libdxfrw_mac_roman_codepage
        COMMAND libdxfrw_gbk_codepage_tests --mac-roman-codepage)
    set_tests_properties(libdxfrw_mac_roman_codepage PROPERTIES
        LABELS "fast;dxf")
    add_test(NAME libdxfrw_iso8859_2_codepage
        COMMAND libdxfrw_gbk_codepage_tests --iso8859-2-codepage)
    set_tests_properties(libdxfrw_iso8859_2_codepage PROPERTIES
        LABELS "fast;dxf")
    add_test(NAME libdxfrw_iso8859_3_codepage
        COMMAND libdxfrw_gbk_codepage_tests --iso8859-3-codepage)
    set_tests_properties(libdxfrw_iso8859_3_codepage PROPERTIES
        LABELS "fast;dxf")
    add_test(NAME libdxfrw_iso8859_5_codepage
        COMMAND libdxfrw_gbk_codepage_tests --iso8859-5-codepage)
    set_tests_properties(libdxfrw_iso8859_5_codepage PROPERTIES
        LABELS "fast;dxf")
    add_test(NAME libdxfrw_iso8859_6_codepage
        COMMAND libdxfrw_gbk_codepage_tests --iso8859-6-codepage)
    set_tests_properties(libdxfrw_iso8859_6_codepage PROPERTIES
        LABELS "fast;dxf")
    add_test(NAME libdxfrw_iso8859_7_codepage
        COMMAND libdxfrw_gbk_codepage_tests --iso8859-7-codepage)
    set_tests_properties(libdxfrw_iso8859_7_codepage PROPERTIES
        LABELS "fast;dxf")
    add_test(NAME libdxfrw_iso8859_8_codepage
        COMMAND libdxfrw_gbk_codepage_tests --iso8859-8-codepage)
    set_tests_properties(libdxfrw_iso8859_8_codepage PROPERTIES
        LABELS "fast;dxf")
    add_test(NAME libdxfrw_iso8859_9_codepage
        COMMAND libdxfrw_gbk_codepage_tests --iso8859-9-codepage)
    set_tests_properties(libdxfrw_iso8859_9_codepage PROPERTIES
        LABELS "fast;dxf")
    add_test(NAME libdxfrw_iso8859_10_codepage
        COMMAND libdxfrw_gbk_codepage_tests --iso8859-10-codepage)
    set_tests_properties(libdxfrw_iso8859_10_codepage PROPERTIES
        LABELS "fast;dxf")
    add_test(NAME libdxfrw_iso8859_11_codepage
        COMMAND libdxfrw_gbk_codepage_tests --iso8859-11-codepage)
    set_tests_properties(libdxfrw_iso8859_11_codepage PROPERTIES
        LABELS "fast;dxf")
    add_test(NAME libdxfrw_iso8859_13_codepage
        COMMAND libdxfrw_gbk_codepage_tests --iso8859-13-codepage)
    set_tests_properties(libdxfrw_iso8859_13_codepage PROPERTIES
        LABELS "fast;dxf")
    add_test(NAME libdxfrw_iso8859_14_codepage
        COMMAND libdxfrw_gbk_codepage_tests --iso8859-14-codepage)
    set_tests_properties(libdxfrw_iso8859_14_codepage PROPERTIES
        LABELS "fast;dxf")
    add_test(NAME libdxfrw_iso8859_15_codepage
        COMMAND libdxfrw_gbk_codepage_tests --iso8859-15-codepage)
    set_tests_properties(libdxfrw_iso8859_15_codepage PROPERTIES
        LABELS "fast;dxf")

    add_executable(libdxfrw_dwg_local_roundtrip
        tests/dwg_local_roundtrip_tests.cpp
        tests/modeler_dxf_writer_matrix_tests.cpp
        tests/modeler_block_census_tests.cpp
        tests/hatch_reference_tests.cpp
        ${_libdxfrw_root_test_adapter_source})
    target_compile_features(libdxfrw_dwg_local_roundtrip PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_dwg_local_roundtrip PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(libdxfrw_dwg_local_roundtrip PRIVATE dxfrw)
    add_test(NAME libdxfrw_dwg_local_roundtrip
        COMMAND libdxfrw_dwg_local_roundtrip)
    add_test(NAME libdxfrw_dwg_ltype_text_area_contract
        COMMAND libdxfrw_dwg_local_roundtrip --dwg-ltype-text-area-contract)
    set_tests_properties(libdxfrw_dwg_ltype_text_area_contract PROPERTIES
        LABELS "fast;dwg;ltype;generated" TIMEOUT 30)
    add_test(NAME libdxfrw_surface_viewport_roundtrip
        COMMAND libdxfrw_dwg_local_roundtrip --surface-viewport-roundtrip)
    set_tests_properties(libdxfrw_surface_viewport_roundtrip PROPERTIES
        LABELS "fast;dxf;geometry;generated"
        TIMEOUT 30)
    add_test(NAME libdxfrw_dxf_spline_helix_source_identities
        COMMAND libdxfrw_dwg_local_roundtrip --dxf-spline-helix-source-identities)
    set_tests_properties(libdxfrw_dxf_spline_helix_source_identities PROPERTIES
        LABELS "fast;dxf;geometry;identity;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_spline_fit_only_dxf
        COMMAND libdxfrw_dwg_local_roundtrip --spline-fit-only-dxf)
    set_tests_properties(libdxfrw_spline_fit_only_dxf PROPERTIES
        LABELS "fast;dxf;geometry;generated" TIMEOUT 30)
    add_test(NAME libdxfrw_table_content_dwg_object_roundtrip
        COMMAND libdxfrw_dwg_local_roundtrip --table-content-dwg-object-roundtrip)
    set_tests_properties(libdxfrw_table_content_dwg_object_roundtrip PROPERTIES
        LABELS "fast;dwg;generated;tables;graph;writer" TIMEOUT 30)
    add_test(NAME libdxfrw_tablestyle_nod_whole_file_roundtrip
        COMMAND libdxfrw_dwg_local_roundtrip --tablestyle-nod-whole-file-roundtrip)
    set_tests_properties(libdxfrw_tablestyle_nod_whole_file_roundtrip PROPERTIES
        LABELS "fast;dwg;generated;tables;graph;writer;readback" TIMEOUT 30)
    add_test(NAME libdxfrw_table_ac1018_whole_file_roundtrip
        COMMAND libdxfrw_dwg_local_roundtrip --table-ac1018-whole-file-roundtrip)
    set_tests_properties(libdxfrw_table_ac1018_whole_file_roundtrip PROPERTIES
        LABELS "fast;dwg;generated;tables;graph;geometry;writer;readback"
        TIMEOUT 30)
    add_test(NAME libdxfrw_table_ac1018_empty_extension_roundtrip
        COMMAND libdxfrw_dwg_local_roundtrip
            --table-ac1018-empty-extension-roundtrip)
    set_tests_properties(libdxfrw_table_ac1018_empty_extension_roundtrip
        PROPERTIES LABELS "fast;dwg;generated;tables;graph;transaction;readback"
        TIMEOUT 30)
    add_test(NAME libdxfrw_table_ac1018_zero_mask_extension_roundtrip
        COMMAND libdxfrw_dwg_local_roundtrip
            --table-ac1018-zero-mask-extension-roundtrip)
    set_tests_properties(libdxfrw_table_ac1018_zero_mask_extension_roundtrip
        PROPERTIES LABELS "fast;dwg;generated;tables;graph;transaction;readback"
        TIMEOUT 30)
    add_test(NAME libdxfrw_table_ac1018_zero_mask_scalar_extension_roundtrip
        COMMAND libdxfrw_dwg_local_roundtrip
            --table-ac1018-zero-mask-scalars-extension-roundtrip)
    set_tests_properties(libdxfrw_table_ac1018_zero_mask_scalar_extension_roundtrip
        PROPERTIES LABELS "fast;dwg;generated;tables;graph;transaction;readback"
        TIMEOUT 30)
    add_executable(libdxfrw_table_extension_facade_allocation_tests
        tests/table_extension_facade_allocation_tests.cpp
        tests/modeler_dxf_writer_matrix_tests.cpp
        tests/modeler_block_census_tests.cpp
        tests/hatch_reference_tests.cpp
        ${_libdxfrw_root_test_adapter_source})
    target_compile_features(libdxfrw_table_extension_facade_allocation_tests
        PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_table_extension_facade_allocation_tests
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(libdxfrw_table_extension_facade_allocation_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_table_extension_facade_allocation_retry
        COMMAND libdxfrw_table_extension_facade_allocation_tests)
    set_tests_properties(libdxfrw_table_extension_facade_allocation_retry
        PROPERTIES LABELS "fast;dwg;generated;tables;allocation;retry" TIMEOUT 120)
    add_test(NAME libdxfrw_table_ac1018_dbcolor_whole_file_roundtrip
        COMMAND libdxfrw_dwg_local_roundtrip
            --table-ac1018-dbcolor-whole-file-roundtrip)
    set_tests_properties(libdxfrw_table_ac1018_dbcolor_whole_file_roundtrip
        PROPERTIES LABELS "fast;dwg;generated;tables;graph;dbcolor;writer;readback"
        TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_builtin_block_owner_modes
        COMMAND libdxfrw_dwg_local_roundtrip --built-in-block-owner-modes)
    set_tests_properties(libdxfrw_dwg_builtin_block_owner_modes
        PROPERTIES LABELS "fast;dwg;blocks;ownership;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_dwg_block_control_handle_boundary
        COMMAND libdxfrw_dwg_local_roundtrip --block-control-handle-boundary)
    add_test(NAME libdxfrw_dwg_r2007_generic_control_boundaries
        COMMAND libdxfrw_dwg_local_roundtrip --r2007-generic-control-boundaries)
    add_test(NAME libdxfrw_dwg_r2007_layer_string_boundaries
        COMMAND libdxfrw_dwg_local_roundtrip --r2007-layer-string-boundaries)
    add_test(NAME libdxfrw_dwg_table_entry_appdata_guards
        COMMAND libdxfrw_dwg_local_roundtrip --table-entry-appdata-guards)
    set_tests_properties(libdxfrw_dwg_table_entry_appdata_guards PROPERTIES
        LABELS "fast;dwg;generated;transaction;metadata")
    add_test(NAME libdxfrw_dwg_r2007_primitive_footers
        COMMAND libdxfrw_dwg_local_roundtrip --r2007-primitive-footers)
    set_tests_properties(libdxfrw_dwg_r2007_primitive_footers PROPERTIES
        LABELS "fast;dwg;AC1021;generated;transaction")
    add_test(NAME libdxfrw_dwg_r2007_lwpolyline_footer
        COMMAND libdxfrw_dwg_local_roundtrip --r2007-lwpolyline-footer)
    set_tests_properties(libdxfrw_dwg_r2007_lwpolyline_footer PROPERTIES
        LABELS "fast;dwg;AC1021;generated;transaction")
    add_test(NAME libdxfrw_dwg_r2007_polyline3d_footer
        COMMAND libdxfrw_dwg_local_roundtrip --r2007-polyline3d-footer)
    set_tests_properties(libdxfrw_dwg_r2007_polyline3d_footer PROPERTIES
        LABELS "fast;dwg;AC1021;generated;transaction")
    set_tests_properties(libdxfrw_dwg_r2007_layer_string_boundaries PROPERTIES
        LABELS "fast;dwg;AC1021;generated;transaction")
    set_tests_properties(libdxfrw_dwg_r2007_generic_control_boundaries PROPERTIES
        LABELS "fast;dwg;AC1021;generated;transaction")
    set_tests_properties(libdxfrw_dwg_block_control_handle_boundary
        PROPERTIES LABELS "fast;dwg;blocks;framing;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_dwg_group_dictionary_controls
        COMMAND libdxfrw_dwg_local_roundtrip --dwg-group-dictionary-controls)
    set_tests_properties(libdxfrw_dwg_group_dictionary_controls PROPERTIES
        LABELS "fast;dwg;generated;group;graph"
        TIMEOUT 60)
    add_test(NAME libdxfrw_standard_dwg_source_baseline
        COMMAND libdxfrw_dwg_local_roundtrip --standard-dwg-source-baseline)
    set_tests_properties(libdxfrw_standard_dwg_source_baseline PROPERTIES
        LABELS "fast;dwg;generated;objects;retention;transaction" TIMEOUT 15)
    add_test(NAME libdxfrw_rooted_standard_dwg_output_proof
        COMMAND libdxfrw_dwg_local_roundtrip --rooted-standard-dwg-output-proof)
    add_test(NAME libdxfrw_dwg_fixed_metadata_pointer
        COMMAND libdxfrw_dwg_local_roundtrip --dwg-fixed-metadata-pointer)
    set_tests_properties(libdxfrw_dwg_fixed_metadata_pointer PROPERTIES LABELS "fast;dwg;metadata;generated")
    add_test(NAME libdxfrw_standard_dwg_group_metadata
        COMMAND libdxfrw_dwg_local_roundtrip --standard-dwg-group-metadata)
    set_tests_properties(libdxfrw_standard_dwg_group_metadata PROPERTIES
        LABELS "fast;dwg;dxf;objects;group;graph;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_rooted_standard_dwg_xrecord_eed
        COMMAND libdxfrw_dwg_local_roundtrip --rooted-standard-dwg-xrecord-eed)
    set_tests_properties(libdxfrw_rooted_standard_dwg_xrecord_eed PROPERTIES
        LABELS "fast;dwg;dxf;objects;xrecord;xdata;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_rooted_standard_dwg_layer_reference
        COMMAND libdxfrw_dwg_local_roundtrip --rooted-standard-dwg-layer-reference)
    set_tests_properties(libdxfrw_rooted_standard_dwg_layer_reference PROPERTIES
        LABELS "fast;dwg;dxf;objects;layer;xdata;generated" TIMEOUT 30)
    set_tests_properties(libdxfrw_rooted_standard_dwg_output_proof PROPERTIES
        LABELS "fast;dwg;dxf;generated;objects;preservation;graph" TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_header_cmc_metadata
        COMMAND libdxfrw_dwg_local_roundtrip --header-cmc-metadata)
    set_tests_properties(libdxfrw_dwg_header_cmc_metadata PROPERTIES
        LABELS "fast;dwg;generated;header;cmc"
        TIMEOUT 60)
    add_test(NAME libdxfrw_section_settings_cmc
        COMMAND libdxfrw_dwg_local_roundtrip --section-settings-cmc)
    set_tests_properties(libdxfrw_section_settings_cmc PROPERTIES
        LABELS "fast;dwg;generated;section-settings;cmc"
        TIMEOUT 60)
    add_test(NAME libdxfrw_mpolygon_fill_cmc
        COMMAND libdxfrw_dwg_local_roundtrip --mpolygon-fill-cmc)
    set_tests_properties(libdxfrw_mpolygon_fill_cmc PROPERTIES
        LABELS "fast;dwg;generated;mpolygon;cmc"
        TIMEOUT 60)
    add_test(NAME dxf_ellipse_extrusion_policy
        COMMAND libdxfrw_dwg_local_roundtrip --dxf-ellipse-extrusion)
    set_tests_properties(dxf_ellipse_extrusion_policy PROPERTIES
        LABELS "fast;dxf;generated;geometry;ocs"
        TIMEOUT 60)
    add_test(NAME dwg2dxf_modeler_dxf_writer_matrix
        COMMAND libdxfrw_dwg_local_roundtrip --modeler-dxf-matrix)
    set_tests_properties(dwg2dxf_modeler_dxf_writer_matrix PROPERTIES
        LABELS "fast;dxf;writer;modeler"
        TIMEOUT 30)
    add_executable(libdxfrw_modeler_dxf_allocation_tests
        tests/modeler_dxf_allocation_tests.cpp
        ${_libdxfrw_root_test_adapter_source})
    target_compile_features(libdxfrw_modeler_dxf_allocation_tests PRIVATE cxx_std_17)
    target_compile_options(libdxfrw_modeler_dxf_allocation_tests
        PRIVATE ${LIBDXFRW_WARNING_FLAGS})
    target_include_directories(libdxfrw_modeler_dxf_allocation_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(libdxfrw_modeler_dxf_allocation_tests PRIVATE dxfrw)
    add_test(NAME dwg2dxf_modeler_dxf_allocation_transaction
        COMMAND libdxfrw_modeler_dxf_allocation_tests
            ${CMAKE_CURRENT_BINARY_DIR}/modeler-dxf-allocation)
    set_tests_properties(dwg2dxf_modeler_dxf_allocation_transaction PROPERTIES
        LABELS "fast;dxf;3d;modeler;transaction;allocation"
        TIMEOUT 60)
    add_test(NAME dwg2dxf_modeler_acds_blob_projection
        COMMAND libdxfrw_dwg_local_roundtrip --modeler-acds-blob-projection)
    set_tests_properties(dwg2dxf_modeler_acds_blob_projection PROPERTIES
        LABELS "fast;dwg;dxf;modeler;acds;blob;generated" TIMEOUT 60)
    add_test(NAME dwg2dxf_native_paged_datastorage
        COMMAND libdxfrw_dwg_local_roundtrip --native-paged-datastorage)
    set_tests_properties(dwg2dxf_native_paged_datastorage PROPERTIES
        LABELS "fast;dwg;dxf;modeler;acds;blob;generated" TIMEOUT 60)
    add_test(NAME dwg2dxf_native_paged_datastorage_faults
        COMMAND libdxfrw_dwg_local_roundtrip --native-paged-datastorage-faults)
    set_tests_properties(dwg2dxf_native_paged_datastorage_faults PROPERTIES
        LABELS "fast;dwg;dxf;modeler;acds;blob;transaction;generated" TIMEOUT 60)
    add_test(NAME libdxfrw_acds_schema_shell_guards
        COMMAND libdxfrw_dwg_local_roundtrip --acds-schema-shell-guards)
    set_tests_properties(libdxfrw_acds_schema_shell_guards PROPERTIES
        LABELS "fast;dwg;dxf;modeler;acds;generated;transaction" TIMEOUT 30)
    add_test(NAME libdxfrw_acds_exact_record_wire_controls
        COMMAND libdxfrw_dwg_local_roundtrip --acds-record-wire-controls)
    set_tests_properties(libdxfrw_acds_exact_record_wire_controls PROPERTIES
        LABELS "fast;dxf;modeler;acds;wire;generated" TIMEOUT 10)
    foreach(_solid_mode IN ITEMS paged inline)
        if(_solid_mode STREQUAL "inline")
            set(_solid_selector "--native-historyless-solid-inline")
        else()
            set(_solid_selector "--native-historyless-solid")
        endif()
        add_test(NAME dwg2dxf_native_historyless_solid_${_solid_mode}
            COMMAND libdxfrw_dwg_local_roundtrip ${_solid_selector})
        add_test(NAME dwg2dxf_native_historyless_solid_${_solid_mode}_faults
            COMMAND libdxfrw_dwg_local_roundtrip ${_solid_selector}-faults)
        set_tests_properties(dwg2dxf_native_historyless_solid_${_solid_mode}
            dwg2dxf_native_historyless_solid_${_solid_mode}_faults PROPERTIES
            LABELS "fast;dwg;dxf;modeler;acds;history;generated" TIMEOUT 60)
    endforeach()
    unset(_solid_selector)
    unset(_solid_mode)
    add_test(NAME dwg2dxf_acds_typed_historyless_companion
        COMMAND libdxfrw_dwg_local_roundtrip --acds-typed-historyless-companion)
    set_tests_properties(dwg2dxf_acds_typed_historyless_companion PROPERTIES
        LABELS "fast;dwg;dxf;modeler;acds;history;transaction;generated" TIMEOUT 30)
    add_test(NAME dwg2dxf_acds_history_reexport_faults
        COMMAND libdxfrw_dwg_local_roundtrip --acds-history-reexport-faults)
    set_tests_properties(dwg2dxf_acds_history_reexport_faults PROPERTIES
        LABELS "fast;dxf;modeler;acds;history;transaction;generated" TIMEOUT 60)
    add_test(NAME libdxfrw_acds_raw_scalar_wire_controls
        COMMAND libdxfrw_dwg_local_roundtrip --acds-raw-scalar-wire-controls)
    add_test(NAME dwg2dxf_acds_strict_material_closure
        COMMAND libdxfrw_dwg_local_roundtrip --acds-strict-material-closure)
    set_tests_properties(dwg2dxf_acds_strict_material_closure PROPERTIES
        LABELS "fast;dxf;adapter;material;history;transaction;generated" TIMEOUT 30)
    set_tests_properties(libdxfrw_acds_raw_scalar_wire_controls PROPERTIES
        LABELS "fast;dxf;modeler;acds;wire;generated" TIMEOUT 10)
    add_test(NAME libdxfrw_material_raw_scalar_wire_controls
        COMMAND libdxfrw_dwg_local_roundtrip --material-raw-scalar-wire-controls)
    set_tests_properties(libdxfrw_material_raw_scalar_wire_controls PROPERTIES
        LABELS "fast;dxf;material;wire;generated" TIMEOUT 10)
    add_test(NAME dwg2dxf_modeler_acds_blob_faults
        COMMAND libdxfrw_dwg_local_roundtrip --modeler-acds-blob-faults)
    set_tests_properties(dwg2dxf_modeler_acds_blob_faults PROPERTIES
        LABELS "fast;dwg;dxf;modeler;acds;blob;transaction;generated" TIMEOUT 60)
    add_test(NAME libdxfrw_datastorage_page_boundaries
        COMMAND libdxfrw_dwg_local_roundtrip --datastorage-page-boundaries)
    add_test(NAME libdxfrw_raw_modeler_acds_replay
        COMMAND libdxfrw_dwg_local_roundtrip --raw-modeler-acds-replay)
    set_tests_properties(libdxfrw_datastorage_page_boundaries libdxfrw_raw_modeler_acds_replay PROPERTIES
        LABELS "fast;dwg;modeler;acds;blob;generated" TIMEOUT 30)
    add_test(NAME dwg2dxf_modeler_acds_r2013plus_projection
        COMMAND libdxfrw_dwg_local_roundtrip --modeler-acds-r2013plus-projection)
    set_tests_properties(dwg2dxf_modeler_acds_r2013plus_projection PROPERTIES
        LABELS "fast;dxf;writer;modeler;acds"
        TIMEOUT 60)
    add_test(NAME dwg2dxf_acds_multi_record_projection
        COMMAND libdxfrw_dwg_local_roundtrip --dxf-acds-multiple-data-storage)
    set_tests_properties(dwg2dxf_acds_multi_record_projection PROPERTIES
        LABELS "fast;dxf;writer;modeler;acds;generated;multi-record"
        TIMEOUT 60)
    add_test(NAME dwg2dxf_modeler_acds_block_projection
        COMMAND libdxfrw_dwg_local_roundtrip --modeler-acds-block-projection)
    set_tests_properties(dwg2dxf_modeler_acds_block_projection PROPERTIES
        LABELS "fast;dxf;dwg;modeler;acds;graph"
        TIMEOUT 60)
    add_test(NAME dwg_dxf_issue_1410_cross_codec
        COMMAND libdxfrw_dwg_local_roundtrip --issue-1410-cross-codec)
    set_tests_properties(dwg_dxf_issue_1410_cross_codec PROPERTIES
        LABELS "fast;dxf;dwg;roundtrip"
        TIMEOUT 60)
    add_test(NAME dwg_hatch_rational_spline_weight_handoff
        COMMAND libdxfrw_dwg_local_roundtrip --hatch-dwg-spline-weights)
    set_tests_properties(dwg_hatch_rational_spline_weight_handoff PROPERTIES
        LABELS "fast;dxf;dwg;hatch;semantic"
        TIMEOUT 60)
    add_test(NAME dwg_hatch_dwg_boundary_references
        COMMAND libdxfrw_dwg_local_roundtrip --hatch-dwg-boundary-references)
    set_tests_properties(dwg_hatch_dwg_boundary_references PROPERTIES
        LABELS "fast;dxf;dwg;hatch;semantic"
        TIMEOUT 60)
    add_test(NAME dwg_hatch_reference_graph
        COMMAND libdxfrw_dwg_local_roundtrip --hatch-references)
    set_tests_properties(dwg_hatch_reference_graph PROPERTIES
        LABELS "fast;dxf;hatch;graph"
        TIMEOUT 60)
    add_test(NAME dxf_hatch_derived_pixel_size_presence
        COMMAND libdxfrw_dwg_local_roundtrip --hatch-pixel-presence)
    set_tests_properties(dxf_hatch_derived_pixel_size_presence PROPERTIES
        LABELS "fast;dxf;hatch;generated;group47"
        TIMEOUT 60)

    add_executable(libdxfrw_dwg_reader_matrix_tests
        tests/dwg_reader_matrix_tests.cpp
        ${_libdxfrw_reader_test_adapter_source})
    target_compile_features(libdxfrw_dwg_reader_matrix_tests PRIVATE cxx_std_17)
    # Fixture paths belong to test drivers, never the reusable adapter TU.
    set_property(SOURCE tests/dwg_reader_matrix_tests.cpp APPEND PROPERTY COMPILE_DEFINITIONS
        LIBDXFRW_READER_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/dwg")
    target_include_directories(libdxfrw_dwg_reader_matrix_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intern
        ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf)
    target_link_libraries(libdxfrw_dwg_reader_matrix_tests PRIVATE dxfrw)
    # Opt-in source-bound DIMASSOC transport probe. This executable accepts a
    # caller-supplied DWG path and deliberately has no committed sample or
    # default CTest registration.
    add_executable(libdxfrw_dimassoc_physical_transport_tests
        tests/dimassoc_physical_transport_tests.cpp
        ${_libdxfrw_reader_test_adapter_source})
    target_compile_features(libdxfrw_dimassoc_physical_transport_tests
        PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_dimassoc_physical_transport_tests
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
                ${CMAKE_CURRENT_SOURCE_DIR}/src/intern
                ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf)
    target_link_libraries(libdxfrw_dimassoc_physical_transport_tests
        PRIVATE dxfrw)
    add_test(NAME libdxfrw_dwg_reader_matrix
        COMMAND libdxfrw_dwg_reader_matrix_tests)
    add_test(NAME libdxfrw_dwg_erased_block_record_table_recovery
        COMMAND libdxfrw_dwg_reader_matrix_tests
            --erased-block-record-table-recovery)
    set_tests_properties(libdxfrw_dwg_erased_block_record_table_recovery
        PROPERTIES LABELS "fast;dwg;tables;generated;AC1015"
        TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_orphan_entity_sweep_recovery
        COMMAND libdxfrw_dwg_reader_matrix_tests
            --orphan-entity-sweep-recovery)
    set_tests_properties(libdxfrw_dwg_orphan_entity_sweep_recovery
        PROPERTIES LABELS "fast;dwg;entities;generated;AC1027"
        TIMEOUT 30)
    # These DWG test executables share generated fixed temp paths. Serialize
    # the CTest entries within a build until each harness owns unique paths.
    set_property(TEST libdxfrw_dwg_local_roundtrip
        libdxfrw_dwg_reader_matrix
        libdxfrw_raw_modeler_acds_replay
        APPEND PROPERTY RESOURCE_LOCK libdxfrw_generated_dwg_tmpfiles)
    add_test(NAME libdxfrw_dwg_r2018_auxheader_trailer
        COMMAND libdxfrw_dwg_reader_matrix_tests
            --r2018-auxheader-trailer)
    set_tests_properties(libdxfrw_dwg_r2018_auxheader_trailer PROPERTIES
        LABELS "fast;dwg;header;generated;AC1027;AC1032"
        TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_header_version_envelopes
        COMMAND libdxfrw_dwg_reader_matrix_tests
            --header-version-envelopes)
    set_tests_properties(libdxfrw_dwg_header_version_envelopes PROPERTIES
        LABELS "fast;dwg;header;generated;AC1027"
        TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_template_fields
        COMMAND libdxfrw_dwg_reader_matrix_tests --template-fields)
    set_tests_properties(libdxfrw_dwg_template_fields PROPERTIES
        LABELS "fast;dwg;header;generated"
        TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_tablestyle_malformed_count_dispatch
        COMMAND libdxfrw_dwg_reader_matrix_tests
            --tablestyle-malformed-count-dispatch)
    set_tests_properties(libdxfrw_dwg_tablestyle_malformed_count_dispatch
        PROPERTIES LABELS "fast;dwg;tablestyle;generated;AC1032"
        TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_acds_dataindex_order
        COMMAND libdxfrw_dwg_reader_matrix_tests --acds-dataindex-order)
    set_tests_properties(libdxfrw_dwg_acds_dataindex_order PROPERTIES
        LABELS "fast;dwg;acds;generated"
        TIMEOUT 30)
    add_test(NAME libdxfrw_proxy_subentity_marker_journal
        COMMAND libdxfrw_dwg_reader_matrix_tests
            --proxy-subentity-marker-journal)
    set_tests_properties(libdxfrw_proxy_subentity_marker_journal PROPERTIES
        LABELS "fast;dwg;generated;proxy-graphics"
        TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_r2007_class_string_footer
        COMMAND libdxfrw_dwg_reader_matrix_tests --r2007-class-string-footer)
    set_tests_properties(libdxfrw_dwg_r2007_class_string_footer PROPERTIES
        LABELS "fast;dwg;framing;generated"
        TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_acdb_placeholder_class_dispatch
        COMMAND libdxfrw_dwg_reader_matrix_tests
            --acdb-placeholder-class-dispatch)
    set_tests_properties(libdxfrw_dwg_acdb_placeholder_class_dispatch PROPERTIES
        LABELS "fast;dwg;objects;generated"
        TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_r2007_crc_spec_vectors
        COMMAND libdxfrw_dwg_reader_matrix_tests --r2007-crc-literal-vectors)
    set_tests_properties(libdxfrw_dwg_r2007_crc_spec_vectors PROPERTIES
        LABELS "fast;dwg;crc;generated;AC1021"
        TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_r2007_data_page_crc_profiles
        COMMAND libdxfrw_dwg_reader_matrix_tests --r2007-data-page-profiles)
    set_tests_properties(libdxfrw_dwg_r2007_data_page_crc_profiles PROPERTIES
        LABELS "fast;dwg;crc;generated;AC1021"
        TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_r2007_section_map_page_extent
        COMMAND libdxfrw_dwg_reader_matrix_tests
            --r2007-section-map-page-extent)
    add_test(NAME libdxfrw_dwg_header_reference_roles
        COMMAND libdxfrw_dwg_reader_matrix_tests --header-reference-roles)
    set_tests_properties(libdxfrw_dwg_header_reference_roles PROPERTIES
        LABELS "fast;dwg;header;generated;handles" TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_header_packed_flags
        COMMAND libdxfrw_dwg_reader_matrix_tests --header-packed-flags)
    set_tests_properties(libdxfrw_dwg_header_packed_flags PROPERTIES
        LABELS "fast;dwg;header;generated;roundtrip" TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_header_current_references
        COMMAND libdxfrw_dwg_reader_matrix_tests --header-current-references)
    set_tests_properties(libdxfrw_dwg_header_current_references PROPERTIES
        LABELS "fast;dwg;header;generated;handles;transaction" TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_header_writer_sealing
        COMMAND libdxfrw_dwg_reader_matrix_tests --header-writer-sealing)
    set_tests_properties(libdxfrw_dwg_header_writer_sealing PROPERTIES
        LABELS "fast;dwg;header;generated;transaction" TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_r2004_system_terminators
        COMMAND libdxfrw_dwg_reader_matrix_tests --r2004-system-terminators)
    set_tests_properties(libdxfrw_dwg_r2004_system_terminators PROPERTIES
        LABELS "fast;dwg;system-pages;generated;framing" TIMEOUT 30)
    set_tests_properties(libdxfrw_dwg_r2007_section_map_page_extent PROPERTIES
        LABELS "fast;dwg;section-map;generated;AC1021"
        TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_insert_staging_index
        COMMAND libdxfrw_dwg_reader_matrix_tests --insert-staging-index)
    set_tests_properties(libdxfrw_dwg_insert_staging_index PROPERTIES
        LABELS "fast;dwg;insert;generated"
        TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_block_record_xref_index
        COMMAND libdxfrw_dwg_reader_matrix_tests --block-record-xref-index)
    set_tests_properties(libdxfrw_dwg_block_record_xref_index PROPERTIES
        LABELS "fast;dwg;xref;generated"
        TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_block_record_xref_roundtrip
        COMMAND libdxfrw_dwg_local_roundtrip --block-record-xref-roundtrip)
    set_tests_properties(libdxfrw_dwg_block_record_xref_roundtrip PROPERTIES
        LABELS "fast;dwg;xref;generated;roundtrip"
        TIMEOUT 60)
    add_test(NAME libdxfrw_dwg_plotsettings_nod_graph
        COMMAND libdxfrw_dwg_local_roundtrip --plotsettings-nod-graph)
    set_tests_properties(libdxfrw_dwg_plotsettings_nod_graph PROPERTIES
        LABELS "fast;dwg;plotsettings;generated;roundtrip"
        TIMEOUT 60)
    add_test(NAME libdxfrw_dwg_block_owner_membership_index
        COMMAND libdxfrw_dwg_local_roundtrip --block-owner-membership-index)
    set_tests_properties(libdxfrw_dwg_block_owner_membership_index PROPERTIES
        LABELS "fast;dwg;ownership;generated"
        TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_image_compound_transaction
        COMMAND libdxfrw_dwg_local_roundtrip --image-compound-transaction)
    set_tests_properties(libdxfrw_dwg_image_compound_transaction PROPERTIES
        LABELS "fast;dwg;image;transaction;generated"
        TIMEOUT 30)
    add_test(NAME libdxfrw_wipeoutvariables_dwg_scalar_domain
        COMMAND libdxfrw_dwg_local_roundtrip --wipeoutvariables-dwg-scalar-domain)
    set_tests_properties(libdxfrw_wipeoutvariables_dwg_scalar_domain PROPERTIES
        LABELS "fast;dwg;wipeout;generated"
        TIMEOUT 30)
    add_test(NAME libdxfrw_rastervariables_dwg_scalar_domain
        COMMAND libdxfrw_dwg_local_roundtrip --rastervariables-dwg-scalar-domain)
    set_tests_properties(libdxfrw_rastervariables_dwg_scalar_domain PROPERTIES
        LABELS "fast;dwg;raster;generated"
        TIMEOUT 30)
    add_executable(libdxfrw_raster_dwg_wire_tests tests/raster_dwg_wire_tests.cpp)
    target_include_directories(libdxfrw_raster_dwg_wire_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf)
    target_compile_features(libdxfrw_raster_dwg_wire_tests PRIVATE cxx_std_14)
    add_test(NAME libdxfrw_raster_dwg_independent_wire COMMAND libdxfrw_raster_dwg_wire_tests)
    set_tests_properties(libdxfrw_raster_dwg_independent_wire PROPERTIES
        LABELS "fast;dwg;image;wire;generated;independent" TIMEOUT 30)
    add_executable(libdxfrw_class_dwg_wire_tests tests/dwg_class_wire_tests.cpp)
    target_include_directories(libdxfrw_class_dwg_wire_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf)
    target_compile_options(libdxfrw_class_dwg_wire_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
    set_target_properties(libdxfrw_class_dwg_wire_tests PROPERTIES
        CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    add_test(NAME libdxfrw_class_dwg_independent_wire COMMAND libdxfrw_class_dwg_wire_tests)
    set_tests_properties(libdxfrw_class_dwg_independent_wire PROPERTIES
        LABELS "fast;dwg;classes;wire;generated;independent" TIMEOUT 15)
    add_executable(libdxfrw_class_section_wire_tests tests/dwg_class_section_wire_tests.cpp)
    target_include_directories(libdxfrw_class_section_wire_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf)
    target_compile_options(libdxfrw_class_section_wire_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
    set_target_properties(libdxfrw_class_section_wire_tests PROPERTIES
        CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    add_test(NAME libdxfrw_class_section_independent_wire COMMAND libdxfrw_class_section_wire_tests)
    set_tests_properties(libdxfrw_class_section_independent_wire PROPERTIES
        LABELS "fast;dwg;classes;framing;generated;independent" TIMEOUT 30)
    add_executable(libdxfrw_class_evidence_tests tests/dwg_class_evidence_tests.cpp)
    target_include_directories(libdxfrw_class_evidence_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_features(libdxfrw_class_evidence_tests PRIVATE cxx_std_17)
    target_compile_options(libdxfrw_class_evidence_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
    add_test(NAME libdxfrw_class_source_join COMMAND libdxfrw_class_evidence_tests)
    set_tests_properties(libdxfrw_class_source_join PROPERTIES
        LABELS "fast;dwg;classes;source;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_dwg_residual_entity_sweep
        COMMAND libdxfrw_dwg_reader_matrix_tests --residual-entity-sweep)
    set_tests_properties(libdxfrw_dwg_residual_entity_sweep PROPERTIES
        LABELS "fast;dwg;ordering;generated"
        TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_mixed_missing_block_ownership
        COMMAND libdxfrw_dwg_reader_matrix_tests --mixed-missing-block-ownership)
    set_tests_properties(libdxfrw_dwg_mixed_missing_block_ownership PROPERTIES
        LABELS "fast;dwg;ownership;generated"
        TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_control_handle_slots
        COMMAND libdxfrw_dwg_reader_matrix_tests --control-handle-slots)
    set_tests_properties(libdxfrw_dwg_control_handle_slots PROPERTIES
        LABELS "fast;dwg;tables;proxy;generated"
        TIMEOUT 15)
    add_test(NAME libdxfrw_dwg_ac1015_ltype_count
        COMMAND libdxfrw_dwg_reader_matrix_tests --ac1015-ltype-count)
    set_tests_properties(libdxfrw_dwg_ac1015_ltype_count PROPERTIES
        LABELS "fast;dwg;tables;generated"
        TIMEOUT 15)
    add_test(NAME libdxfrw_dwg_space_block_delimiter_owner
        COMMAND libdxfrw_dwg_reader_matrix_tests --space-block-delimiter-owner)
    set_tests_properties(libdxfrw_dwg_space_block_delimiter_owner PROPERTIES
        LABELS "fast;dwg;ownership;generated"
        TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_r11_paper_space_projection
        COMMAND libdxfrw_dwg_reader_matrix_tests --r11-paper-space-projection)
    set_tests_properties(libdxfrw_dwg_r11_paper_space_projection PROPERTIES
        LABELS "fast;dwg;space;generated"
        TIMEOUT 30)
    if(LIBDXFRW_BUILD_DWG2DXF)
        add_test(NAME libdxfrw_dwg_r11_paper_space_cli_refusal
            COMMAND ${CMAKE_COMMAND}
                "-DREADER_TEST=$<TARGET_FILE:libdxfrw_dwg_reader_matrix_tests>"
                "-DDWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r11_paper_space_cli_test.cmake.in)
        set_tests_properties(libdxfrw_dwg_r11_paper_space_cli_refusal PROPERTIES
            LABELS "fast;dwg;space;generated;cli"
            TIMEOUT 30)
    endif()

    add_executable(libdxfrw_graph_preservation_tests
        tests/graph_preservation_tests.cpp)
    target_compile_features(libdxfrw_graph_preservation_tests PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_graph_preservation_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intern)
    target_link_libraries(libdxfrw_graph_preservation_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_graph_preservation
        COMMAND libdxfrw_graph_preservation_tests)
    add_test(NAME libdxfrw_datastorage_bounds
        COMMAND libdxfrw_graph_preservation_tests --datastorage-bounds)
    set_tests_properties(libdxfrw_datastorage_bounds PROPERTIES
        LABELS "fast;dwg;generated;datastorage"
        TIMEOUT 30)
    add_test(NAME libdxfrw_proxy_subentity_markers
        COMMAND libdxfrw_graph_preservation_tests --proxy-subentity-markers)
    set_tests_properties(libdxfrw_proxy_subentity_markers PROPERTIES
        LABELS "fast;dwg;generated;proxy-graphics"
        TIMEOUT 30)

    add_executable(libdxfrw_writer_primitives_tests
        tests/writer_primitives_tests.cpp)
    target_compile_features(libdxfrw_writer_primitives_tests PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_writer_primitives_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intern)
    target_link_libraries(libdxfrw_writer_primitives_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_writer_primitives
        COMMAND libdxfrw_writer_primitives_tests)

    add_executable(libdxfrw_dwg_object_vectors_tests
        tests/dwg_object_vectors_tests.cpp)
    target_compile_features(libdxfrw_dwg_object_vectors_tests PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_dwg_object_vectors_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intern)
    target_link_libraries(libdxfrw_dwg_object_vectors_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_dwg_object_vectors
        COMMAND libdxfrw_dwg_object_vectors_tests)
    add_test(NAME libdxfrw_dwg_dxf_transparency_boundaries
        COMMAND libdxfrw_dwg_object_vectors_tests
            --transparency-format-boundaries)
    set_tests_properties(libdxfrw_dwg_dxf_transparency_boundaries PROPERTIES
        LABELS "fast;dwg;dxf;transparency;generated"
        TIMEOUT 30)
    add_executable(libdxfrw_dwg_object_encoder_allocation_tests
        tests/dimassoc_dwg_encoder_allocation_tests.cpp)
    target_compile_features(libdxfrw_dwg_object_encoder_allocation_tests
        PRIVATE cxx_std_17)
    target_compile_options(libdxfrw_dwg_object_encoder_allocation_tests
        PRIVATE ${LIBDXFRW_WARNING_FLAGS})
    target_include_directories(libdxfrw_dwg_object_encoder_allocation_tests
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
                ${CMAKE_CURRENT_SOURCE_DIR}/src/intern)
    target_link_libraries(libdxfrw_dwg_object_encoder_allocation_tests
        PRIVATE dxfrw)
    add_test(NAME libdxfrw_dwg_object_encoder_allocation_retry
        COMMAND libdxfrw_dwg_object_encoder_allocation_tests)
    set_tests_properties(libdxfrw_dwg_object_encoder_allocation_retry
        PROPERTIES LABELS "fast;dwg;object;allocation;transaction;generated"
                   TIMEOUT 60)
    add_test(NAME libdxfrw_ac1027_object_common_handle_roles
        COMMAND libdxfrw_dwg_object_vectors_tests
            --ac1027-object-common-handle-roles)
    set_tests_properties(libdxfrw_ac1027_object_common_handle_roles PROPERTIES
        LABELS "fast;dwg;object-vectors;handles;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_ac1018plus_layer_body_handle_roles
        COMMAND libdxfrw_dwg_object_vectors_tests
            --ac1018plus-layer-body-handle-roles)
    set_tests_properties(libdxfrw_ac1018plus_layer_body_handle_roles PROPERTIES
        LABELS "fast;dwg;object-vectors;handles;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_ac1015plus_dimstyle_body_handle_roles
        COMMAND libdxfrw_dwg_object_vectors_tests
            --ac1015plus-dimstyle-body-handle-roles)
    set_tests_properties(libdxfrw_ac1015plus_dimstyle_body_handle_roles PROPERTIES
        LABELS "fast;dwg;object-vectors;handles;generated" TIMEOUT 30)
    add_test(NAME libdxfrw_ac1012plus_vport_handle_roles
        COMMAND libdxfrw_dwg_object_vectors_tests
            --ac1012plus-vport-handle-roles)
    set_tests_properties(libdxfrw_ac1012plus_vport_handle_roles PROPERTIES
        LABELS "fast;dwg;object-vectors;handles;generated" TIMEOUT 30)
    add_test(NAME libdxfrw_ac1012plus_ucs_handle_roles
        COMMAND libdxfrw_dwg_object_vectors_tests
            --ac1012plus-ucs-handle-roles)
    set_tests_properties(libdxfrw_ac1012plus_ucs_handle_roles PROPERTIES
        LABELS "fast;dwg;object-vectors;handles;generated" TIMEOUT 30)
    add_test(NAME libdxfrw_ac1012plus_view_handle_roles
        COMMAND libdxfrw_dwg_object_vectors_tests
            --ac1012plus-view-handle-roles)
    set_tests_properties(libdxfrw_ac1012plus_view_handle_roles PROPERTIES
        LABELS "fast;dwg;object-vectors;handles;generated" TIMEOUT 30)
    add_test(NAME libdxfrw_ac1032_mlinestyle_handle_roles
        COMMAND libdxfrw_dwg_object_vectors_tests
            --ac1032-mlinestyle-handle-roles)
    set_tests_properties(libdxfrw_ac1032_mlinestyle_handle_roles PROPERTIES
        LABELS "fast;dwg;object-vectors;handles;generated" TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_table_cell_overrides
        COMMAND libdxfrw_dwg_object_vectors_tests --table-cell-overrides)
    set_tests_properties(libdxfrw_dwg_table_cell_overrides PROPERTIES
        LABELS "fast;dwg;object-vectors;table;generated;transaction"
        TIMEOUT 15)
    add_executable(libdxfrw_table_dwg_codec_allocation_tests
        tests/table_dwg_codec_allocation_tests.cpp)
    target_compile_features(libdxfrw_table_dwg_codec_allocation_tests
        PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_table_dwg_codec_allocation_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intern)
    target_link_libraries(libdxfrw_table_dwg_codec_allocation_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_table_dwg_codec_allocation_retry
        COMMAND libdxfrw_table_dwg_codec_allocation_tests)
    set_tests_properties(libdxfrw_table_dwg_codec_allocation_retry PROPERTIES
        LABELS "fast;dwg;table;generated;allocation;retry"
        TIMEOUT 15)
    add_test(NAME libdxfrw_mesh_dwg_crease_count_guard
        COMMAND libdxfrw_dwg_object_vectors_tests
            --mesh-dwg-crease-count-guard)
    set_tests_properties(libdxfrw_mesh_dwg_crease_count_guard PROPERTIES
        LABELS "fast;dwg;object-vectors;mesh;generated"
        TIMEOUT 15)
    add_test(NAME libdxfrw_legacy_dictionary_group_links
        COMMAND libdxfrw_dwg_object_vectors_tests
            --legacy-dictionary-group-links)
    set_tests_properties(libdxfrw_legacy_dictionary_group_links PROPERTIES
        LABELS "fast;dwg;object-vectors;dictionary;group;generated;AC1012;AC1014"
        TIMEOUT 15)
    add_test(NAME libdxfrw_dictionary_item_wire_role
        COMMAND libdxfrw_dwg_object_vectors_tests
            --dictionary-item-wire-role)
    set_tests_properties(libdxfrw_dictionary_item_wire_role PROPERTIES
        LABELS "fast;dwg;object-vectors;dictionary;generated"
        TIMEOUT 15)
    add_test(NAME libdxfrw_plotsettings_dwg_decode_roundtrip
        COMMAND libdxfrw_dwg_object_vectors_tests
            --plotsettings-dwg-decode-roundtrip)
    set_tests_properties(libdxfrw_plotsettings_dwg_decode_roundtrip
        PROPERTIES LABELS "fast;dwg;object-vectors;plotsettings;generated"
                   TIMEOUT 15)
    add_test(NAME libdxfrw_scale_dwg_unit_domains
        COMMAND libdxfrw_dwg_object_vectors_tests --scale-dwg-unit-domains)
    set_tests_properties(libdxfrw_scale_dwg_unit_domains PROPERTIES
        LABELS "fast;dwg;object-vectors;scale;generated"
        TIMEOUT 15)
    add_test(NAME libdxfrw_dictionary_default_handle_role
        COMMAND libdxfrw_dwg_object_vectors_tests
            --dictionary-default-handle-role)
    set_tests_properties(libdxfrw_dictionary_default_handle_role PROPERTIES
        LABELS "fast;dwg;object-vectors;graph;generated"
        TIMEOUT 15)
    add_test(NAME libdxfrw_dictionary_default_dwg_encode_allocation_retry
        COMMAND libdxfrw_dwg_object_vectors_tests
            --dictionary-default-dwg-encode-allocation-rollback)
    set_tests_properties(
        libdxfrw_dictionary_default_dwg_encode_allocation_retry PROPERTIES
        LABELS "fast;dwg;object-vectors;allocation;transaction;generated"
        TIMEOUT 30)
    add_test(NAME libdxfrw_dimassoc_dwg_detail_readback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --dimassoc-dwg-detail-readback)
    set_tests_properties(libdxfrw_dimassoc_dwg_detail_readback
        PROPERTIES LABELS "fast;dwg;object-vectors;dimassoc;generated"
                   TIMEOUT 15)
    add_test(NAME libdxfrw_persistent_manager_exact_footer
        COMMAND libdxfrw_dwg_object_vectors_tests --persistent-manager-exact-footer)
    set_tests_properties(libdxfrw_persistent_manager_exact_footer PROPERTIES
        LABELS "fast;dwg;object-vectors;associative;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_geodata_civil_tail_target_matrix
        COMMAND libdxfrw_dwg_object_vectors_tests
            --geodata-civil-tail-target-matrix)
    set_tests_properties(libdxfrw_geodata_civil_tail_target_matrix
        PROPERTIES LABELS "fast;dwg;object-vectors;geodata" TIMEOUT 10)
    add_test(NAME libdxfrw_geodata_dwg_scale_method_domain
        COMMAND libdxfrw_dwg_object_vectors_tests
            --geodata-dwg-scale-method-domain)
    set_tests_properties(libdxfrw_geodata_dwg_scale_method_domain PROPERTIES
        LABELS "fast;dwg;object-vectors;geodata;writer;domain;generated" TIMEOUT 10)
    add_test(NAME libdxfrw_geodata_dwg_unit_domain
        COMMAND libdxfrw_dwg_object_vectors_tests
            --geodata-dwg-unit-domain)
    set_tests_properties(libdxfrw_geodata_dwg_unit_domain PROPERTIES
        LABELS "fast;dwg;object-vectors;geodata;writer;domain;generated" TIMEOUT 10)
    add_test(NAME libdxfrw_geodata_dwg_coordinate_type_domain
        COMMAND libdxfrw_dwg_object_vectors_tests
            --geodata-dwg-coordinate-type-domain)
    set_tests_properties(libdxfrw_geodata_dwg_coordinate_type_domain PROPERTIES
        LABELS "fast;dwg;object-vectors;geodata;writer;domain;generated" TIMEOUT 10)
    add_test(NAME libdxfrw_associative_action_value_kinds
        COMMAND libdxfrw_dwg_object_vectors_tests
            --associative-action-value-kinds)
    set_tests_properties(libdxfrw_associative_action_value_kinds PROPERTIES
        LABELS "fast;dwg;object-vectors" TIMEOUT 10)
    add_test(NAME libdxfrw_associative_dimension_dependency_body
        COMMAND libdxfrw_dwg_object_vectors_tests
            --associative-dimension-dependency-body)
    set_tests_properties(libdxfrw_associative_dimension_dependency_body PROPERTIES
        LABELS "fast;dwg;object-vectors;associative;generated" TIMEOUT 10)
    add_test(NAME libdxfrw_associative_dependency_fields
        COMMAND libdxfrw_dwg_object_vectors_tests
            --associative-dependency-fields)
    set_tests_properties(libdxfrw_associative_dependency_fields PROPERTIES
        LABELS "fast;dwg;object-vectors;associative;generated" TIMEOUT 10)
    add_test(NAME libdxfrw_table_break_data_flag_domain
        COMMAND libdxfrw_dwg_object_vectors_tests
            --table-break-data-flag-domain)
    set_tests_properties(libdxfrw_table_break_data_flag_domain PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_table_break_option_direction_domains
        COMMAND libdxfrw_dwg_object_vectors_tests
            --table-break-option-direction-domains)
    set_tests_properties(libdxfrw_table_break_option_direction_domains PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_table_row_range_count_domain
        COMMAND libdxfrw_dwg_object_vectors_tests
            --table-row-range-count-domain)
    set_tests_properties(libdxfrw_table_row_range_count_domain PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_table_manual_position_count_domain
        COMMAND libdxfrw_dwg_object_vectors_tests
            --table-manual-position-count-domain)
    set_tests_properties(libdxfrw_table_manual_position_count_domain PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_table_cell_state_flag_domain
        COMMAND libdxfrw_dwg_object_vectors_tests
            --table-cell-state-flag-domain)
    set_tests_properties(libdxfrw_table_cell_state_flag_domain PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_table_cell_content_type_domain
        COMMAND libdxfrw_dwg_object_vectors_tests
            --table-cell-content-type-domain)
    set_tests_properties(libdxfrw_table_cell_content_type_domain PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_table_content_style_id_domains
        COMMAND libdxfrw_dwg_object_vectors_tests
            --table-content-style-id-domains)
    set_tests_properties(libdxfrw_table_content_style_id_domains PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_table_content_dwg_payload_encoder
        COMMAND libdxfrw_dwg_object_vectors_tests
            --table-content-dwg-payload-encoder)
    set_tests_properties(libdxfrw_table_content_dwg_payload_encoder PROPERTIES
        LABELS "fast;dwg;object-vectors;tables;generated" TIMEOUT 10)
    add_test(NAME libdxfrw_table_content_cell_style_border_alignment
        COMMAND libdxfrw_dwg_object_vectors_tests
            --table-content-cell-style-border-alignment)
    set_tests_properties(libdxfrw_table_content_cell_style_border_alignment PROPERTIES
        LABELS "fast;dwg;object-vectors;tables;generated" TIMEOUT 10)
    add_test(NAME libdxfrw_tablegeometry_dwg_bd_tail
        COMMAND libdxfrw_dwg_object_vectors_tests
            --tablegeometry-dwg-bd-tail)
    set_tests_properties(libdxfrw_tablegeometry_dwg_bd_tail PROPERTIES
        LABELS "fast;dwg;object-vectors;tables;generated" TIMEOUT 10)
    add_test(NAME libdxfrw_table_content_override_style_types
        COMMAND libdxfrw_dwg_object_vectors_tests
            --table-content-override-style-types)
    set_tests_properties(libdxfrw_table_content_override_style_types PROPERTIES
        LABELS "fast;dwg;object-vectors;tables;generated" TIMEOUT 10)
    add_test(NAME libdxfrw_table_cell_unit_type_domain
        COMMAND libdxfrw_dwg_object_vectors_tests
            --table-cell-unit-type-domain)
    set_tests_properties(libdxfrw_table_cell_unit_type_domain PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_tablestyle_legacy_unit_type_domains
        COMMAND libdxfrw_dwg_object_vectors_tests
            --tablestyle-legacy-unit-type-domains)
    set_tests_properties(libdxfrw_tablestyle_legacy_unit_type_domains PROPERTIES
        LABELS "fast;dwg;object-vectors;tablestyle;generated"
        TIMEOUT 10)
    add_test(NAME libdxfrw_tablestyle_legacy_row_handle_role
        COMMAND libdxfrw_dwg_object_vectors_tests
            --tablestyle-legacy-row-handle-role)
    set_tests_properties(libdxfrw_tablestyle_legacy_row_handle_role PROPERTIES
        LABELS "fast;dwg;object-vectors;tablestyle;generated" TIMEOUT 10)
    add_test(NAME libdxfrw_table_cell_general_value_format_flag
        COMMAND libdxfrw_dwg_object_vectors_tests
            --table-cell-general-value-format-flag)
    set_tests_properties(libdxfrw_table_cell_general_value_format_flag PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_cadvalue_payload_presence
        COMMAND libdxfrw_dwg_object_vectors_tests --cadvalue-payload-presence)
    set_tests_properties(libdxfrw_cadvalue_payload_presence PROPERTIES
        LABELS "fast;dwg;object-vectors;tables;field;generated" TIMEOUT 10)
    add_test(NAME libdxfrw_table_cadvalue_long
        COMMAND libdxfrw_dwg_object_vectors_tests --table-cadvalue-long)
    set_tests_properties(libdxfrw_table_cadvalue_long PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_table_cadvalue_string
        COMMAND libdxfrw_dwg_object_vectors_tests --table-cadvalue-string)
    set_tests_properties(libdxfrw_table_cadvalue_string PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_table_cadvalue_date_payload
        COMMAND libdxfrw_dwg_object_vectors_tests
            --table-cadvalue-date-payload)
    set_tests_properties(libdxfrw_table_cadvalue_date_payload PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_table_cadvalue_points
        COMMAND libdxfrw_dwg_object_vectors_tests --table-cadvalue-points)
    set_tests_properties(libdxfrw_table_cadvalue_points PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_table_cadvalue_object_id
        COMMAND libdxfrw_dwg_object_vectors_tests
            --table-cadvalue-object-id)
    set_tests_properties(libdxfrw_table_cadvalue_object_id PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_table_cadvalue_unknown_type
        COMMAND libdxfrw_dwg_object_vectors_tests
            --table-cadvalue-unknown-type)
    set_tests_properties(libdxfrw_table_cadvalue_unknown_type PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_table_cadvalue_unsupported_buffers
        COMMAND libdxfrw_dwg_object_vectors_tests
            --table-cadvalue-unsupported-buffers)
    set_tests_properties(libdxfrw_table_cadvalue_unsupported_buffers PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_table_r2010_unknown_prefix_defaults
        COMMAND libdxfrw_dwg_object_vectors_tests
            --table-r2010-unknown-prefix)
    set_tests_properties(libdxfrw_table_r2010_unknown_prefix_defaults
        PROPERTIES LABELS "fast;dwg;object-vectors;tables;generated"
            TIMEOUT 10)
    add_test(NAME libdxfrw_tablestyle_data_flag_domain
        COMMAND libdxfrw_dwg_object_vectors_tests --tablestyle-data-flag)
    set_tests_properties(libdxfrw_tablestyle_data_flag_domain PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_tablestyle_type_domain
        COMMAND libdxfrw_dwg_object_vectors_tests --tablestyle-type-domain)
    set_tests_properties(libdxfrw_tablestyle_type_domain PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_cellstylemap_dwg_encode_roundtrip
        COMMAND libdxfrw_dwg_object_vectors_tests
            --cellstylemap-dwg-encode-roundtrip)
    set_tests_properties(libdxfrw_cellstylemap_dwg_encode_roundtrip
        PROPERTIES LABELS "fast;dwg;object-vectors;tablestyle;generated"
            TIMEOUT 10)
    add_test(NAME libdxfrw_tablestyle_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --tablestyle-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_tablestyle_dwg_encode_allocation_rollback
        PROPERTIES LABELS "fast;dwg;object-vectors;tablestyle;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_tablestyle_merge_flag_domain
        COMMAND libdxfrw_dwg_object_vectors_tests
            --tablestyle-merge-flag-domain)
    set_tests_properties(libdxfrw_tablestyle_merge_flag_domain PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_tablestyle_margin_override_flag_domain
        COMMAND libdxfrw_dwg_object_vectors_tests
            --tablestyle-margin-override-flag-domain)
    set_tests_properties(libdxfrw_tablestyle_margin_override_flag_domain PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_tablestyle_content_property_flag_domain
        COMMAND libdxfrw_dwg_object_vectors_tests
            --tablestyle-content-property-flag-domain)
    set_tests_properties(libdxfrw_tablestyle_content_property_flag_domain PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_tablestyle_property_override_flag_domain
        COMMAND libdxfrw_dwg_object_vectors_tests
            --tablestyle-property-override-flag-domain)
    set_tests_properties(libdxfrw_tablestyle_property_override_flag_domain PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_tablestyle_content_layout_flag_domain
        COMMAND libdxfrw_dwg_object_vectors_tests
            --tablestyle-content-layout-flag-domain)
    set_tests_properties(libdxfrw_tablestyle_content_layout_flag_domain PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_tablestyle_border_flag_domains
        COMMAND libdxfrw_dwg_object_vectors_tests
            --tablestyle-border-flag-domains)
    set_tests_properties(libdxfrw_tablestyle_border_flag_domains PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_tablestyle_identity_domains
        COMMAND libdxfrw_dwg_object_vectors_tests
            --tablestyle-identity-domains)
    set_tests_properties(libdxfrw_tablestyle_identity_domains PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_cellstylemap_identity_domains
        COMMAND libdxfrw_dwg_object_vectors_tests
            --cellstylemap-identity-domains)
    set_tests_properties(libdxfrw_cellstylemap_identity_domains PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_tablestyle_content_format_enum_domains
        COMMAND libdxfrw_dwg_object_vectors_tests
            --tablestyle-content-format-enum-domains)
    set_tests_properties(libdxfrw_tablestyle_content_format_enum_domains PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_tablestyle_handle_roles
        COMMAND libdxfrw_dwg_object_vectors_tests --tablestyle-handle-roles)
    set_tests_properties(libdxfrw_tablestyle_handle_roles PROPERTIES
        LABELS "fast;dwg;object-vectors;tables" TIMEOUT 10)
    add_test(NAME libdxfrw_dbcolor_unsupported_method_refusal
        COMMAND libdxfrw_dwg_object_vectors_tests
            --dbcolor-unsupported-method-refusal)
    set_tests_properties(libdxfrw_dbcolor_unsupported_method_refusal PROPERTIES
        LABELS "fast;dwg;object-vectors;cmc" TIMEOUT 10)
    add_test(NAME libdxfrw_cmcolor_foreground_projection
        COMMAND libdxfrw_dwg_object_vectors_tests
            --cmcolor-foreground-projection)
    set_tests_properties(libdxfrw_cmcolor_foreground_projection PROPERTIES
        LABELS "fast;dwg;object-vectors;cmc" TIMEOUT 10)
    add_test(NAME libdxfrw_enc_reserved_flags_refusal
        COMMAND libdxfrw_dwg_object_vectors_tests
            --enc-reserved-flags-refusal)
    set_tests_properties(libdxfrw_enc_reserved_flags_refusal PROPERTIES
        LABELS "fast;dwg;object-vectors;color" TIMEOUT 10)
    add_test(NAME libdxfrw_visualstyle_cmc_source_replay
        COMMAND libdxfrw_dwg_object_vectors_tests
            --visualstyle-cmc-source-replay)
    set_tests_properties(libdxfrw_visualstyle_cmc_source_replay PROPERTIES
        LABELS "fast;dwg;object-vectors;cmc;visualstyle" TIMEOUT 10)
    add_test(NAME libdxfrw_material_ac1024_typed_profile
        COMMAND libdxfrw_dwg_object_vectors_tests
            --material-ac1024-typed-profile)
    set_tests_properties(libdxfrw_material_ac1024_typed_profile PROPERTIES
        LABELS "fast;dwg;object-vectors;material;generated" TIMEOUT 10)
    add_test(NAME libdxfrw_viewport_ambient_color_method
        COMMAND libdxfrw_dwg_object_vectors_tests
            --viewport-ambient-color-method)
    set_tests_properties(libdxfrw_viewport_ambient_color_method PROPERTIES
        LABELS "fast;dwg;object-vectors;cmc;viewport" TIMEOUT 10)
    add_test(NAME libdxfrw_sun_cmc_retention
        COMMAND libdxfrw_dwg_object_vectors_tests --sun-cmc-retention)
    set_tests_properties(libdxfrw_sun_cmc_retention PROPERTIES
        LABELS "fast;dwg;object-vectors;cmc;sun" TIMEOUT 10)
    add_test(NAME libdxfrw_modeldoc_view_style_cmc
        COMMAND libdxfrw_dwg_object_vectors_tests
            --modeldoc-view-style-cmc)
    set_tests_properties(libdxfrw_modeldoc_view_style_cmc PROPERTIES
        LABELS "fast;dwg;object-vectors;cmc;view-style" TIMEOUT 10)
    add_test(NAME libdxfrw_mlinestyle_cmc_retention
        COMMAND libdxfrw_dwg_object_vectors_tests --mlinestyle-cmc-retention)
    set_tests_properties(libdxfrw_mlinestyle_cmc_retention PROPERTIES
        LABELS "fast;dwg;object-vectors;cmc;mlinestyle" TIMEOUT 10)
    add_test(NAME libdxfrw_light_cmc_rgb
        COMMAND libdxfrw_dwg_object_vectors_tests --light-cmc-rgb)
    set_tests_properties(libdxfrw_light_cmc_rgb PROPERTIES
        LABELS "fast;dwg;object-vectors;cmc;light" TIMEOUT 10)
    add_test(NAME libdxfrw_sectionobject_cmc_rgb
        COMMAND libdxfrw_dwg_object_vectors_tests --sectionobject-cmc-rgb)
    set_tests_properties(libdxfrw_sectionobject_cmc_rgb PROPERTIES
        LABELS "fast;dwg;object-vectors;cmc;sectionobject" TIMEOUT 10)
    add_test(NAME libdxfrw_mleaderstyle_cmc_rgb
        COMMAND libdxfrw_dwg_object_vectors_tests --mleaderstyle-cmc-rgb)
    set_tests_properties(libdxfrw_mleaderstyle_cmc_rgb PROPERTIES
        LABELS "fast;dwg;object-vectors;cmc;mleaderstyle" TIMEOUT 10)
    add_test(NAME libdxfrw_mleader_line_cmc_rgb
        COMMAND libdxfrw_dwg_object_vectors_tests --mleader-line-cmc-rgb)
    set_tests_properties(libdxfrw_mleader_line_cmc_rgb PROPERTIES
        LABELS "fast;dwg;object-vectors;cmc;mleader" TIMEOUT 10)
    add_test(NAME libdxfrw_dimstyle_dimclr_cmc
        COMMAND libdxfrw_dwg_object_vectors_tests --dimstyle-dimclr-cmc)
    set_tests_properties(libdxfrw_dimstyle_dimclr_cmc PROPERTIES
        LABELS "fast;dwg;object-vectors;cmc;dimstyle" TIMEOUT 10)
    add_test(NAME libdxfrw_dimstyle_dimtfill_cmc
        COMMAND libdxfrw_dwg_object_vectors_tests --dimstyle-dimtfill-cmc)
    set_tests_properties(libdxfrw_dimstyle_dimtfill_cmc PROPERTIES
        LABELS "fast;dwg;object-vectors;cmc;dimstyle" TIMEOUT 10)
    add_test(NAME libdxfrw_mtext_background_cmc
        COMMAND libdxfrw_dwg_object_vectors_tests --mtext-background-cmc)
    set_tests_properties(libdxfrw_mtext_background_cmc PROPERTIES
        LABELS "fast;dwg;object-vectors;cmc;mtext" TIMEOUT 10)
    add_test(NAME libdxfrw_background_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --background-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_background_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;background;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_mtext_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --mtext-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_mtext_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;mtext;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_text_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --text-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_text_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;text;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_line_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --line-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_line_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;line;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_circle_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --circle-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_circle_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;circle;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_arc_ellipse_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --arc-ellipse-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_arc_ellipse_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;arc;ellipse;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_point_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --point-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_point_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;point;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_ray_xline_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --ray-xline-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_ray_xline_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;ray;xline;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_trace_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --trace-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_trace_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;trace;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_solid_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --solid-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_solid_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;solid;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_3dface_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --3dface-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_3dface_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;3dface;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_3dline_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --3dline-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_3dline_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;3dline;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_lwpolyline_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --lwpolyline-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_lwpolyline_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;lwpolyline;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_spline_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --spline-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_spline_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;spline;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_helix_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --helix-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_helix_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;helix;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_block_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --block-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_block_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;block;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_insert_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --insert-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_insert_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;insert;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_attrib_attdef_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --attrib-attdef-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_attrib_attdef_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;attrib;attdef;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_vertex_polyline_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --vertex-polyline-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_vertex_polyline_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;vertex;polyline;seqend;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_leader_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --leader-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_leader_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;leader;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_mline_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --mline-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_mline_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;mline;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_shape_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --shape-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_shape_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;shape;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_ole2frame_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --ole2frame-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_ole2frame_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;ole2frame;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_oleframe_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --oleframe-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_oleframe_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;oleframe;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_mesh_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --mesh-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_mesh_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;mesh;3d;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_tolerance_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --tolerance-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_tolerance_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;tolerance;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_rtext_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --rtext-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_rtext_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;rtext;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_aligned_linear_dim_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --aligned-linear-dim-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_aligned_linear_dim_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;dimension;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_radial_dim_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --radial-dim-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_radial_dim_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;dimension;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_diametric_angular_dim_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --diametric-angular-dim-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_diametric_angular_dim_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;dimension;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_arc_ordinate_dim_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --arc-ordinate-dim-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_arc_ordinate_dim_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;dimension;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_image_wipeout_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --image-wipeout-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_image_wipeout_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;image;wipeout;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_navisworks_model_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --navisworks-model-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_navisworks_model_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;navisworks;3d;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_pointcloudex_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --pointcloudex-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_pointcloudex_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;pointcloud;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_pointcloud_dwg_representability
        COMMAND libdxfrw_dwg_object_vectors_tests
            --pointcloud-dwg-representability)
    set_tests_properties(libdxfrw_pointcloud_dwg_representability
        PROPERTIES
            LABELS "fast;dwg;object-vectors;pointcloud;generated;representability"
            TIMEOUT 20)
    add_test(NAME libdxfrw_pointcloud_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --pointcloud-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_pointcloud_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;pointcloud;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_underlay_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --underlay-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_underlay_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;underlay;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_viewport_dwg_encode_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --viewport-dwg-encode-allocation-rollback)
    set_tests_properties(libdxfrw_viewport_dwg_encode_allocation_rollback
        PROPERTIES
            LABELS "fast;dwg;object-vectors;viewport;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_r2018_attribute_outer_annotative
        COMMAND libdxfrw_dwg_object_vectors_tests --r2018-attribute-outer)
    set_tests_properties(libdxfrw_r2018_attribute_outer_annotative PROPERTIES
        LABELS "fast;dwg;object-vectors;attrib" TIMEOUT 30)
    add_test(NAME libdxfrw_field_oversized_child_value_count
        COMMAND libdxfrw_dwg_object_vectors_tests
            --field-oversized-child-value-count)
    set_tests_properties(libdxfrw_field_oversized_child_value_count PROPERTIES
        LABELS "fast;dwg;object-vectors;field" TIMEOUT 10)
    add_test(NAME libdxfrw_field_unit12_dwg_strings
        COMMAND libdxfrw_dwg_object_vectors_tests --field-unit12-dwg-strings)
    set_tests_properties(libdxfrw_field_unit12_dwg_strings PROPERTIES
        LABELS "fast;dwg;field;generated")
    add_test(NAME libdxfrw_field_object_encoder_allocation_rollback
        COMMAND libdxfrw_dwg_object_vectors_tests
            --field-object-encoder-allocation-rollback)
    set_tests_properties(libdxfrw_field_object_encoder_allocation_rollback
        PROPERTIES LABELS "fast;dwg;object-vectors;field;generated;transaction"
            TIMEOUT 20)
    add_test(NAME libdxfrw_field_cadvalue_general_format_flag
        COMMAND libdxfrw_dwg_object_vectors_tests
            --field-cadvalue-general-format-flag)
    set_tests_properties(libdxfrw_field_cadvalue_general_format_flag PROPERTIES
        LABELS "fast;dwg;object-vectors;field" TIMEOUT 10)
    add_test(NAME libdxfrw_field_unqualified_format_flags
        COMMAND libdxfrw_dwg_object_vectors_tests
            --field-cadvalue-unqualified-format-flags)
    set_tests_properties(libdxfrw_field_unqualified_format_flags PROPERTIES
        LABELS "fast;dwg;object-vectors;field" TIMEOUT 10)
    add_test(NAME libdxfrw_field_cadvalue_unknown_type
        COMMAND libdxfrw_dwg_object_vectors_tests
            --field-cadvalue-unknown-type)
    set_tests_properties(libdxfrw_field_cadvalue_unknown_type PROPERTIES
        LABELS "fast;dwg;object-vectors;field" TIMEOUT 10)
    add_test(NAME libdxfrw_field_cadvalue_long
        COMMAND libdxfrw_dwg_object_vectors_tests
            --field-cadvalue-long)
    set_tests_properties(libdxfrw_field_cadvalue_long PROPERTIES
        LABELS "fast;dwg;object-vectors;field" TIMEOUT 10)
    add_test(NAME libdxfrw_field_cadvalue_double
        COMMAND libdxfrw_dwg_object_vectors_tests
            --field-cadvalue-double)
    set_tests_properties(libdxfrw_field_cadvalue_double PROPERTIES
        LABELS "fast;dwg;object-vectors;field" TIMEOUT 10)
    add_test(NAME libdxfrw_field_cadvalue_string
        COMMAND libdxfrw_dwg_object_vectors_tests
            --field-cadvalue-string)
    set_tests_properties(libdxfrw_field_cadvalue_string PROPERTIES
        LABELS "fast;dwg;object-vectors;field" TIMEOUT 10)
    add_test(NAME libdxfrw_field_cadvalue_date_payload
        COMMAND libdxfrw_dwg_object_vectors_tests
            --field-cadvalue-date-payload)
    set_tests_properties(libdxfrw_field_cadvalue_date_payload PROPERTIES
        LABELS "fast;dwg;object-vectors;field" TIMEOUT 10)
    add_test(NAME libdxfrw_field_cadvalue_points
        COMMAND libdxfrw_dwg_object_vectors_tests
            --field-cadvalue-points)
    set_tests_properties(libdxfrw_field_cadvalue_points PROPERTIES
        LABELS "fast;dwg;object-vectors;field" TIMEOUT 10)
    add_test(NAME libdxfrw_field_cadvalue_object_id
        COMMAND libdxfrw_dwg_object_vectors_tests
            --field-cadvalue-object-id)
    set_tests_properties(libdxfrw_field_cadvalue_object_id PROPERTIES
        LABELS "fast;dwg;object-vectors;field" TIMEOUT 10)
    add_test(NAME libdxfrw_field_child_cadvalue_types
        COMMAND libdxfrw_dwg_object_vectors_tests
            --field-child-cadvalue-types)
    set_tests_properties(libdxfrw_field_child_cadvalue_types PROPERTIES
        LABELS "fast;dwg;object-vectors;field" TIMEOUT 10)
    add_test(NAME libdxfrw_field_unsupported_child_cadvalue_buffers
        COMMAND libdxfrw_dwg_object_vectors_tests
            --field-unsupported-child-cadvalue-buffers)
    set_tests_properties(libdxfrw_field_unsupported_child_cadvalue_buffers
        PROPERTIES LABELS "fast;dwg;object-vectors;field" TIMEOUT 10)
    add_test(NAME libdxfrw_field_unknown_child_cadvalue_versions
        COMMAND libdxfrw_dwg_object_vectors_tests
            --field-unknown-child-cadvalue-versions)
    set_tests_properties(libdxfrw_field_unknown_child_cadvalue_versions
        PROPERTIES LABELS "fast;dwg;object-vectors;field" TIMEOUT 10)
    add_test(NAME libdxfrw_field_child_object_id_invalid_handle_role
        COMMAND libdxfrw_dwg_object_vectors_tests
            --field-child-object-id-invalid-handle-role)
    set_tests_properties(libdxfrw_field_child_object_id_invalid_handle_role
        PROPERTIES LABELS "fast;dwg;object-vectors;field" TIMEOUT 10)
    add_test(NAME libdxfrw_visualstyle_slot_manifest
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_visualstyle_slot_manifest.py
            --manifest
            ${CMAKE_CURRENT_SOURCE_DIR}/docs/research/visualstyle-slot-map-readiness-2026-10-15.json
            --grades
            ${CMAKE_CURRENT_SOURCE_DIR}/docs/research/visualstyle-slot-evidence-grades-2026-10-16.json
            --vectors
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg_object_vectors_tests.cpp)

    add_executable(libdxfrw_hardening_tests
        tests/hardening_tests.cpp)
    target_compile_features(libdxfrw_hardening_tests PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_hardening_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intern)
    target_link_libraries(libdxfrw_hardening_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_hardening
        COMMAND libdxfrw_hardening_tests)
    add_test(NAME libdxfrw_acsh_action_body_exact_span
        COMMAND libdxfrw_hardening_tests --acsh-action-body-exact-span)
    set_tests_properties(libdxfrw_acsh_action_body_exact_span PROPERTIES
        LABELS "fast;dwg;acds;parser;generated" TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_operation_admission
        COMMAND libdxfrw_hardening_tests --dxf-operation-admission)
    set_tests_properties(libdxfrw_dxf_operation_admission PROPERTIES
        LABELS "fast;dxf;reader;writer;transaction;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_section_settings_color_code
        COMMAND libdxfrw_hardening_tests --section-settings-color-code)
    set_tests_properties(libdxfrw_section_settings_color_code PROPERTIES
        LABELS "fast;dxf;reader;section-settings"
        TIMEOUT 10)
    add_test(NAME libdxfrw_spatial_filter_simple_boundary
        COMMAND libdxfrw_hardening_tests --spatial-filter-dxf-validation)
    set_tests_properties(libdxfrw_spatial_filter_simple_boundary PROPERTIES
        LABELS "fast;dxf;reader;writer;spatial-filter;generated"
        TIMEOUT 10)
    add_test(NAME libdxfrw_hatch_boolean_dxf_domain
        COMMAND libdxfrw_hardening_tests --hatch-boolean-dxf-domain)
    set_tests_properties(libdxfrw_hatch_boolean_dxf_domain PROPERTIES
        LABELS "fast;dxf;reader;hatch;generated"
        TIMEOUT 10)
    add_test(NAME libdxfrw_hatch_pattern_dxf_domains
        COMMAND libdxfrw_hardening_tests --hatch-pattern-dxf-domains)
    set_tests_properties(libdxfrw_hatch_pattern_dxf_domains PROPERTIES
        LABELS "fast;dxf;reader;writer;hatch;generated"
        TIMEOUT 10)
    add_test(NAME libdxfrw_ltype_alignment_dxf_domain
        COMMAND libdxfrw_hardening_tests --ltype-alignment-dxf-domain)
    set_tests_properties(libdxfrw_ltype_alignment_dxf_domain PROPERTIES
        LABELS "fast;dxf;reader;writer;ltype;generated"
        TIMEOUT 10)
    add_test(NAME libdxfrw_ltype_shape_number_dxf_width
        COMMAND libdxfrw_hardening_tests --ltype-shape-number-dxf-width)
    set_tests_properties(libdxfrw_ltype_shape_number_dxf_width PROPERTIES
        LABELS "fast;dxf;reader;writer;ltype;generated"
        TIMEOUT 10)
    add_test(NAME libdxfrw_ltype_standard_flag_dxf_domain
        COMMAND libdxfrw_hardening_tests --ltype-standard-flag-dxf-domain)
    set_tests_properties(libdxfrw_ltype_standard_flag_dxf_domain PROPERTIES
        LABELS "fast;dxf;reader;writer;ltype;generated"
        TIMEOUT 10)
    add_test(NAME libdxfrw_ltype_element_flag_read_tolerance
        COMMAND libdxfrw_hardening_tests --ltype-element-flag-read-tolerance)
    set_tests_properties(libdxfrw_ltype_element_flag_read_tolerance PROPERTIES
        LABELS "fast;dxf;reader;writer;ltype;generated"
        TIMEOUT 10)
    add_test(NAME libdxfrw_ltype_group9_encoded_length
        COMMAND libdxfrw_hardening_tests --ltype-group9-encoded-length)
    set_tests_properties(libdxfrw_ltype_group9_encoded_length PROPERTIES
        LABELS "fast;dxf;reader;writer;ltype;transaction;generated"
        TIMEOUT 15)
    add_test(NAME libdxfrw_ltype_scale_header_domain
        COMMAND libdxfrw_hardening_tests --ltype-scale-header-domain)
    set_tests_properties(libdxfrw_ltype_scale_header_domain PROPERTIES
        LABELS "fast;dxf;reader;writer;ltype;header;transaction;generated"
        TIMEOUT 15)
    add_test(NAME libdxfrw_linetype_display_scale_domains
        COMMAND libdxfrw_hardening_tests --linetype-display-scale-domains)
    set_tests_properties(libdxfrw_linetype_display_scale_domains PROPERTIES
        LABELS "fast;dxf;reader;writer;ltype;header;transaction;generated"
        TIMEOUT 15)
    add_test(NAME libdxfrw_measurement_header_domain
        COMMAND libdxfrw_hardening_tests --dxf-measurement-header-domain)
    set_tests_properties(libdxfrw_measurement_header_domain PROPERTIES
        LABELS "fast;dxf;reader;writer;header;transaction;generated"
        TIMEOUT 15)
    add_test(NAME libdxfrw_ucs_base_header_fields
        COMMAND libdxfrw_hardening_tests --dxf-ucs-base-header-fields)
    set_tests_properties(libdxfrw_ucs_base_header_fields PROPERTIES
        LABELS "fast;dxf;reader;writer;header;ucs;transaction;generated"
        TIMEOUT 15)
    add_test(NAME libdxfrw_insunits_header_domain
        COMMAND libdxfrw_hardening_tests --dxf-insunits-header-domain)
    set_tests_properties(libdxfrw_insunits_header_domain PROPERTIES
        LABELS "fast;dxf;dwg;reader;writer;header;transaction;generated"
        TIMEOUT 20)
    add_test(NAME libdxfrw_lineweight_header_domains
        COMMAND libdxfrw_hardening_tests --dxf-lineweight-header-domains)
    set_tests_properties(libdxfrw_lineweight_header_domains PROPERTIES
        LABELS "fast;dxf;reader;writer;header;transaction;generated"
        TIMEOUT 20)
    add_test(NAME libdxfrw_orientation_attribute_header_domains
        COMMAND libdxfrw_hardening_tests
            --dxf-orientation-attribute-header-domains)
    set_tests_properties(libdxfrw_orientation_attribute_header_domains PROPERTIES
        LABELS "fast;dxf;reader;writer;header;transaction;generated"
        TIMEOUT 20)
    add_test(NAME libdxfrw_attribute_prompt_header_domains
        COMMAND libdxfrw_hardening_tests --dxf-attribute-prompt-header-domains)
    set_tests_properties(libdxfrw_attribute_prompt_header_domains PROPERTIES
        LABELS "fast;dxf;reader;writer;header;transaction;generated"
        TIMEOUT 20)
    add_test(NAME libdxfrw_mesh_crease_value_domains
        COMMAND libdxfrw_hardening_tests --dxf-mesh-crease-value-domains)
    set_tests_properties(libdxfrw_mesh_crease_value_domains PROPERTIES
        LABELS "fast;dxf;dwg;reader;writer;mesh;transaction;generated"
        TIMEOUT 20)
    add_test(NAME libdxfrw_scale_units_dxf_domain
        COMMAND libdxfrw_hardening_tests --dxf-scale-units-domain)
    set_tests_properties(libdxfrw_scale_units_dxf_domain PROPERTIES
        LABELS "fast;dxf;reader;writer;scale;generated"
        TIMEOUT 10)
    add_test(NAME libdxfrw_scale_flag_output_guard
        COMMAND libdxfrw_hardening_tests --dxf-scale-flag-output-guard)
    set_tests_properties(libdxfrw_scale_flag_output_guard PROPERTIES
        LABELS "fast;dxf;reader;writer;scale;generated"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dbcolor_book_name_owner_budget
        COMMAND libdxfrw_hardening_tests
            --dxf-dbcolor-book-name-owner-budget)
    set_tests_properties(libdxfrw_dbcolor_book_name_owner_budget PROPERTIES
        LABELS "fast;dxf;reader;dbcolor;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_color_book_name_token_transfer
        COMMAND libdxfrw_hardening_tests
            --dxf-color-book-name-token-transfer)
    set_tests_properties(libdxfrw_dxf_color_book_name_token_transfer PROPERTIES
        LABELS "fast;dxf;reader;color-book;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_dimassoc_xref_token_transfer
        COMMAND libdxfrw_hardening_tests
            --dxf-dimassoc-xref-text-owner-budget)
    set_tests_properties(libdxfrw_dxf_dimassoc_xref_token_transfer PROPERTIES
        LABELS "fast;dxf;reader;dimassoc;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_entity_boundary_token_transfer
        COMMAND libdxfrw_hardening_tests
            --dxf-entity-boundary-token-transfer)
    set_tests_properties(libdxfrw_dxf_entity_boundary_token_transfer PROPERTIES
        LABELS "fast;dxf;reader;record-boundary"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_dimension_block_name_owner_budget
        COMMAND libdxfrw_hardening_tests
            --dxf-dimension-block-name-owner-budget)
    set_tests_properties(libdxfrw_dxf_dimension_block_name_owner_budget PROPERTIES
        LABELS "fast;dxf;reader;dimension;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_layer_text_owner_budget
        COMMAND libdxfrw_hardening_tests
            --dxf-layer-text-owner-budget)
    set_tests_properties(libdxfrw_dxf_layer_text_owner_budget PROPERTIES
        LABELS "fast;dxf;reader;layer;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_mline_style_name_owner_budget
        COMMAND libdxfrw_hardening_tests
            --dxf-mline-style-name-owner-budget)
    set_tests_properties(libdxfrw_dxf_mline_style_name_owner_budget PROPERTIES
        LABELS "fast;dxf;reader;mline;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_appdata_retained_owner_budget
        COMMAND libdxfrw_hardening_tests
            --dxf-appdata-retained-owner-budget)
    set_tests_properties(libdxfrw_dxf_appdata_retained_owner_budget PROPERTIES
        LABELS "fast;dxf;reader;appdata;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_appdata_handle_token_transfer
        COMMAND libdxfrw_hardening_tests
            --dxf-appdata-handle-token-transfer)
    set_tests_properties(libdxfrw_dxf_appdata_handle_token_transfer PROPERTIES
        LABELS "fast;dxf;reader;appdata;handle"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_xdata_string_owner_budget
        COMMAND libdxfrw_hardening_tests
            --dxf-xdata-string-owner-budget)
    set_tests_properties(libdxfrw_dxf_xdata_string_owner_budget PROPERTIES
        LABELS "fast;dxf;reader;xdata;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_class_parser_retained_text_budget
        COMMAND libdxfrw_hardening_tests
            --dxf-class-parser-retained-text-budget)
    set_tests_properties(libdxfrw_dxf_class_parser_retained_text_budget PROPERTIES
        LABELS "fast;dxf;reader;classes;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_ltype_retained_owner_budget
        COMMAND libdxfrw_hardening_tests
            --dxf-ltype-retained-owner-budget)
    set_tests_properties(libdxfrw_dxf_ltype_retained_owner_budget PROPERTIES
        LABELS "fast;dxf;reader;ltype;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_block_record_parser_retained_budget
        COMMAND libdxfrw_hardening_tests
            --dxf-block-record-parser-retained-budget)
    set_tests_properties(libdxfrw_dxf_block_record_parser_retained_budget PROPERTIES
        LABELS "fast;dxf;reader;block-record;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_modeler_text_direct_append
        COMMAND libdxfrw_hardening_tests
            --dxf-modeler-text-direct-append)
    set_tests_properties(libdxfrw_dxf_modeler_text_direct_append PROPERTIES
        LABELS "fast;dxf;reader;modeler;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_modeler_uid_owner_budget
        COMMAND libdxfrw_hardening_tests
            --dxf-modeler-uid-owner-budget)
    set_tests_properties(libdxfrw_dxf_modeler_uid_owner_budget PROPERTIES
        LABELS "fast;dxf;reader;modeler;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_group_310_direct_decode
        COMMAND libdxfrw_hardening_tests
            --dxf-group-310-direct-decode)
    set_tests_properties(libdxfrw_dxf_group_310_direct_decode PROPERTIES
        LABELS "fast;dxf;reader;binary-payload;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_mtext_borrowed_chunk_view
        COMMAND libdxfrw_hardening_tests
            --dxf-mtext-borrowed-chunk-view)
    set_tests_properties(libdxfrw_dxf_mtext_borrowed_chunk_view PROPERTIES
        LABELS "fast;dxf;reader;mtext;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_borrowed_subclass_markers
        COMMAND libdxfrw_hardening_tests
            --dxf-borrowed-subclass-markers)
    set_tests_properties(libdxfrw_dxf_borrowed_subclass_markers PROPERTIES
        LABELS "fast;dxf;reader;subclass-marker;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_shape_style_name_owner_budget
        COMMAND libdxfrw_hardening_tests
            --dxf-shape-style-name-owner-budget)
    set_tests_properties(libdxfrw_dxf_shape_style_name_owner_budget PROPERTIES
        LABELS "fast;dxf;reader;shape;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_raw_carrier_retention_budget
        COMMAND libdxfrw_hardening_tests
            --dxf-raw-carrier-retention-budget)
    set_tests_properties(libdxfrw_dxf_raw_carrier_retention_budget PROPERTIES
        LABELS "fast;dxf;reader;raw-carrier;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_unknown_table_name_view
        COMMAND libdxfrw_hardening_tests
            --dxf-unknown-table-name-view)
    set_tests_properties(libdxfrw_dxf_unknown_table_name_view PROPERTIES
        LABELS "fast;dxf;reader;tables;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_viewstyle_subclass_marker_budget
        COMMAND libdxfrw_hardening_tests
            --dxf-viewstyle-subclass-marker-budget)
    set_tests_properties(libdxfrw_viewstyle_subclass_marker_budget PROPERTIES
        LABELS "fast;dxf;reader;viewstyle;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_geodata_civil_marker_views
        COMMAND libdxfrw_hardening_tests --dxf-geodata-civil-marker-views)
    set_tests_properties(libdxfrw_geodata_civil_marker_views PROPERTIES
        LABELS "fast;dxf;reader;geodata;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_section_text_owner_budget
        COMMAND libdxfrw_hardening_tests --dxf-section-text-owner-budget)
    set_tests_properties(libdxfrw_section_text_owner_budget PROPERTIES
        LABELS "fast;dxf;reader;section;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_material_text_owner_budget
        COMMAND libdxfrw_hardening_tests --dxf-material-text-owner-budget)
    set_tests_properties(libdxfrw_dxf_material_text_owner_budget PROPERTIES
        LABELS "fast;dxf;reader;material;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_table_cell_text_owner_budget
        COMMAND libdxfrw_hardening_tests
            --dxf-table-cell-text-owner-budget)
    set_tests_properties(libdxfrw_dxf_table_cell_text_owner_budget PROPERTIES
        LABELS "fast;dxf;reader;table;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_xrecord_string_owner_budget
        COMMAND libdxfrw_hardening_tests --dxf-xrecord-string-owner-budget)
    set_tests_properties(libdxfrw_dxf_xrecord_string_owner_budget PROPERTIES
        LABELS "fast;dxf;reader;xrecord;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_datalink_text_owner_budget
        COMMAND libdxfrw_hardening_tests --dxf-data-link-text-owner-budget)
    set_tests_properties(libdxfrw_datalink_text_owner_budget PROPERTIES
        LABELS "fast;dxf;reader;datalink;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_render_settings_text_owner_budget
        COMMAND libdxfrw_hardening_tests
            --dxf-render-settings-text-owner-budget)
    set_tests_properties(libdxfrw_render_settings_text_owner_budget PROPERTIES
        LABELS "fast;dxf;reader;render-settings;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_viewport_text_owner_budget
        COMMAND libdxfrw_hardening_tests --dxf-viewport-text-owner-budget)
    set_tests_properties(libdxfrw_dxf_viewport_text_owner_budget PROPERTIES
        LABELS "fast;dxf;reader;viewport;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_associative_subentity_handle_limit
        COMMAND libdxfrw_hardening_tests
            --dxf-associative-subentity-handle-limit)
    set_tests_properties(libdxfrw_associative_subentity_handle_limit PROPERTIES
        LABELS "fast;dxf;reader;associative;resource-budget"
        TIMEOUT 10)
    add_test(NAME libdxfrw_sectionobject_indicator_color_groups
        COMMAND libdxfrw_hardening_tests
            --sectionobject-indicator-color-groups)
    set_tests_properties(libdxfrw_sectionobject_indicator_color_groups PROPERTIES
        LABELS "fast;dxf;reader;sectionobject" TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_proxy_subclass_marker_view
        COMMAND libdxfrw_hardening_tests
            --dxf-proxy-subclass-marker-view)
    set_tests_properties(libdxfrw_dxf_proxy_subclass_marker_view PROPERTIES
        LABELS "fast;dxf;reader;proxy;allocation" TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_proxy_retained_budget
        COMMAND libdxfrw_hardening_tests --dxf-proxy-retained-budget)
    set_tests_properties(libdxfrw_dxf_proxy_retained_budget PROPERTIES
        LABELS "fast;dxf;reader;proxy;resource-budget" TIMEOUT 10)
    add_test(NAME libdxfrw_dxf_proxy_object_id_terminator
        COMMAND libdxfrw_hardening_tests
            --dxf-proxy-object-id-terminator)
    set_tests_properties(libdxfrw_dxf_proxy_object_id_terminator PROPERTIES
        LABELS "fast;dxf;reader;proxy;handles" TIMEOUT 10)

    add_executable(libdxfrw_debug_state_tests
        tests/debug_state_tests.cpp dwg2dxf/dx_iface.cpp)
    target_compile_features(libdxfrw_debug_state_tests PRIVATE cxx_std_17)
    target_compile_options(libdxfrw_debug_state_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
    target_compile_definitions(libdxfrw_debug_state_tests PRIVATE
        LIBDXFRW_DWG_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/dwg")
    target_include_directories(libdxfrw_debug_state_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(libdxfrw_debug_state_tests PRIVATE dxfrw Threads::Threads)
    add_test(NAME libdxfrw_debug_state COMMAND libdxfrw_debug_state_tests)
    set_tests_properties(libdxfrw_debug_state PROPERTIES
        LABELS "fast;hardening;debug" TIMEOUT 30)
    add_test(NAME libdxfrw_scoped_codec_concurrency
        COMMAND libdxfrw_debug_state_tests --scoped-codecs-only)
    set_tests_properties(libdxfrw_scoped_codec_concurrency PROPERTIES
        LABELS "fast;dxf;reader;writer;concurrency" TIMEOUT 30)

    if(LIBDXFRW_BUILD_LONG_FUZZ)
        add_executable(libdxfrw_long_fuzz_tests
            tests/hardening_tests.cpp)
        target_compile_features(libdxfrw_long_fuzz_tests PRIVATE cxx_std_17)
        target_compile_definitions(libdxfrw_long_fuzz_tests PRIVATE
            LIBDXFRW_LONG_FUZZ=1)
        target_include_directories(libdxfrw_long_fuzz_tests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/src/intern)
        target_link_libraries(libdxfrw_long_fuzz_tests PRIVATE dxfrw)
        add_test(NAME libdxfrw_long_fuzz
            COMMAND libdxfrw_long_fuzz_tests)
    endif()

    add_executable(libdxfrw_writer_version_matrix_tests
        tests/writer_version_matrix_tests.cpp)
    target_compile_features(libdxfrw_writer_version_matrix_tests PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_writer_version_matrix_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intern
        ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf)
    target_compile_options(libdxfrw_writer_version_matrix_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
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

    add_executable(libdxfrw_dxf_fixture_tests
        tests/dxf_fixture_tests.cpp
        ${_libdxfrw_root_test_adapter_source})
    target_compile_features(libdxfrw_dxf_fixture_tests PRIVATE cxx_std_17)
    set_property(SOURCE tests/dxf_fixture_tests.cpp APPEND PROPERTY COMPILE_DEFINITIONS
        LIBDXFRW_DXF_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/dxf")
    target_include_directories(libdxfrw_dxf_fixture_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(libdxfrw_dxf_fixture_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_dxf_fixtures
        COMMAND libdxfrw_dxf_fixture_tests)
    add_test(NAME libdxfrw_dxf_transparency_xref_roundtrip
        COMMAND libdxfrw_dxf_fixture_tests --block-xref-path-roundtrip)
    set_tests_properties(libdxfrw_dxf_transparency_xref_roundtrip PROPERTIES
        LABELS "fast;dxf;transparency;xref;roundtrip"
        TIMEOUT 30)
    add_test(NAME libdxfrw_dxf_consumer_write_facade
        COMMAND libdxfrw_dxf_fixture_tests --consumer-write-facade)
    set_tests_properties(libdxfrw_dxf_consumer_write_facade PROPERTIES
        LABELS "fast;dxf;consumer;generated;transaction" TIMEOUT 30)
    add_test(NAME libdxfrw_dxf_text_ocs_extrusion
        COMMAND libdxfrw_dxf_fixture_tests --text-ocs-extrusion)
    set_tests_properties(libdxfrw_dxf_text_ocs_extrusion PROPERTIES
        LABELS "fast;dxf;reader;generated;geometry" TIMEOUT 30)
    add_test(NAME libdxfrw_dxf_write_attempt_failure
        COMMAND libdxfrw_dxf_fixture_tests --dxf-write-attempt-failure)
    set_tests_properties(libdxfrw_dxf_write_attempt_failure PROPERTIES
        LABELS "fast;dxf;writer;rollback;retry" TIMEOUT 30)
    add_test(NAME libdxfrw_dxf_hatch_ocs_wcs_coordinate
        COMMAND libdxfrw_dxf_fixture_tests --hatch-ocs-wcs-coordinate)
    set_tests_properties(libdxfrw_dxf_hatch_ocs_wcs_coordinate PROPERTIES
        LABELS "fast;dxf;geometry" TIMEOUT 30)
    add_test(NAME libdxfrw_dxf_polyline_ocs_extrusion
        COMMAND libdxfrw_dxf_fixture_tests --polyline-ocs-extrusion)
    set_tests_properties(libdxfrw_dxf_polyline_ocs_extrusion PROPERTIES
        LABELS "fast;dxf;reader;generated;geometry" TIMEOUT 30)
    add_test(NAME libdxfrw_dxf_underlay_ocs_extrusion
        COMMAND libdxfrw_dxf_fixture_tests --underlay-ocs-extrusion)
    set_tests_properties(libdxfrw_dxf_underlay_ocs_extrusion PROPERTIES
        LABELS "fast;dxf;reader;generated;geometry" TIMEOUT 30)
    add_test(NAME libdxfrw_underlay_dxf_allocation_failure
        COMMAND libdxfrw_dxf_fixture_tests
            --underlay-dxf-allocation-failure)
    set_tests_properties(libdxfrw_underlay_dxf_allocation_failure PROPERTIES
        LABELS "fast;dxf;reader;underlay;allocation;retry" TIMEOUT 30)
    add_test(NAME libdxfrw_dxf_mesh_parser_allocation_retry
        COMMAND libdxfrw_dxf_fixture_tests --mesh-dxf-allocation-failure)
    set_tests_properties(libdxfrw_dxf_mesh_parser_allocation_retry PROPERTIES
        LABELS "fast;dxf;reader;mesh;allocation;retry;generated" TIMEOUT 30)
    add_test(NAME libdxfrw_dxf_hatch_parser_allocation_retry
        COMMAND libdxfrw_dxf_fixture_tests --hatch-dxf-allocation-failure)
    set_tests_properties(libdxfrw_dxf_hatch_parser_allocation_retry PROPERTIES
        LABELS "fast;dxf;reader;hatch;allocation;retry;generated" TIMEOUT 30)
    add_test(NAME libdxfrw_dxf_mline_parser_allocation_retry
        COMMAND libdxfrw_dxf_fixture_tests --mline-dxf-allocation-failure)
    set_tests_properties(libdxfrw_dxf_mline_parser_allocation_retry PROPERTIES
        LABELS "fast;dxf;reader;mline;allocation;retry;generated" TIMEOUT 30)
    add_test(NAME libdxfrw_dxf_dimension_ocs_extrusion
        COMMAND libdxfrw_dxf_fixture_tests --dimension-ocs-extrusion)
    set_tests_properties(libdxfrw_dxf_dimension_ocs_extrusion PROPERTIES
        LABELS "fast;dxf;reader;generated;geometry" TIMEOUT 30)
    add_test(NAME libdxfrw_dynamic_constraint_dimension_dxf_profile
        COMMAND libdxfrw_wave1_tests
            --dynamic-constraint-dimension-dxf-profile)
    set_tests_properties(libdxfrw_dynamic_constraint_dimension_dxf_profile
        PROPERTIES LABELS "fast;dxf;writer;dimension;generated" TIMEOUT 30)
    add_test(NAME libdxfrw_mtext_legacy_truecolor_compatibility
        COMMAND libdxfrw_dxf_fixture_tests
            --mtext-legacy-truecolor-compatibility)
    set_tests_properties(libdxfrw_mtext_legacy_truecolor_compatibility
        PROPERTIES LABELS "fast;dxf;generated;compatibility"
        TIMEOUT 30)
    add_test(NAME libdxfrw_dxf_proxy_graphic_loss_policy
        COMMAND libdxfrw_dxf_fixture_tests --proxy-graphic-loss-policy)
    set_tests_properties(libdxfrw_dxf_proxy_graphic_loss_policy PROPERTIES
        LABELS "fast;dxf;reader;writer;proxy;generated" TIMEOUT 60)
    add_test(NAME libdxfrw_material_body_transactions
        COMMAND libdxfrw_dxf_fixture_tests --material-body-transactions)
    set_tests_properties(libdxfrw_material_body_transactions PROPERTIES
        LABELS "fast;dxf;adapter;material;transaction;generated" TIMEOUT 120)
    add_test(NAME libdxfrw_material_normal_boundaries
        COMMAND libdxfrw_dxf_fixture_tests --material-normal-boundaries)
    set_tests_properties(libdxfrw_material_normal_boundaries PROPERTIES
        LABELS "fast;dxf;adapter;material;transaction;generated" TIMEOUT 60)
    add_test(NAME libdxfrw_material_source_order
        COMMAND libdxfrw_dxf_fixture_tests --material-source-order)
    set_tests_properties(libdxfrw_material_source_order PROPERTIES
        LABELS "fast;dxf;reader;adapter;material;generated" TIMEOUT 60)
    add_test(NAME libdxfrw_mline_style_body_transactions
        COMMAND libdxfrw_dxf_fixture_tests --mline-style-body-transactions)
    set_tests_properties(libdxfrw_mline_style_body_transactions PROPERTIES
        LABELS "fast;dxf;adapter;mline;transaction;generated" TIMEOUT 60)
    add_test(NAME libdxfrw_mline_ltype_referenced_flags
        COMMAND libdxfrw_dxf_fixture_tests --mline-ltype-referenced-flags)
    set_tests_properties(libdxfrw_mline_ltype_referenced_flags PROPERTIES
        LABELS "fast;dxf;adapter;mline;linetype;transaction;generated" TIMEOUT 60)
    add_test(NAME libdxfrw_mline_ltype_provider_profiles
        COMMAND libdxfrw_dxf_fixture_tests --mline-ltype-provider-profiles)
    set_tests_properties(libdxfrw_mline_ltype_provider_profiles PROPERTIES
        LABELS "fast;dxf;adapter;mline;linetype;transaction;generated" TIMEOUT 60)
    add_test(NAME libdxfrw_mline_ltype_appid_xdata
        COMMAND libdxfrw_dxf_fixture_tests --mline-ltype-appid-xdata)
    set_tests_properties(libdxfrw_mline_ltype_appid_xdata PROPERTIES
        LABELS "fast;dxf;adapter;mline;linetype;xdata;transaction;generated" TIMEOUT 60)
    add_test(NAME libdxfrw_mline_ltype_appid_flag64
        COMMAND libdxfrw_dxf_fixture_tests --mline-ltype-appid-flag64)
    set_tests_properties(libdxfrw_mline_ltype_appid_flag64 PROPERTIES
        LABELS "fast;dxf;adapter;mline;linetype;appid;transaction;generated" TIMEOUT 60)
    add_test(NAME libdxfrw_mline_ltype_appid_budget
        COMMAND libdxfrw_dxf_fixture_tests --mline-ltype-appid-budget)
    set_tests_properties(libdxfrw_mline_ltype_appid_budget PROPERTIES
        LABELS "dxf;adapter;mline;linetype;xdata;transaction;generated;budget" TIMEOUT 60)
    add_test(NAME libdxfrw_mline_style_xdata
        COMMAND libdxfrw_dxf_fixture_tests --mline-style-xdata)
    set_tests_properties(libdxfrw_mline_style_xdata PROPERTIES
        LABELS "fast;dxf;adapter;mline;xdata;transaction;generated" TIMEOUT 60)
    add_test(NAME libdxfrw_mline_ltype_text_style_provider
        COMMAND libdxfrw_dxf_fixture_tests --mline-ltype-text-style-provider)
    set_tests_properties(libdxfrw_mline_ltype_text_style_provider PROPERTIES
        LABELS "fast;dxf;adapter;mline;linetype;style;transaction;generated" TIMEOUT 60)
    add_test(NAME libdxfrw_mline_ltype_shape_style_provider
        COMMAND libdxfrw_dxf_fixture_tests --mline-ltype-shape-style-provider)
    set_tests_properties(libdxfrw_mline_ltype_shape_style_provider PROPERTIES
        LABELS "fast;dxf;adapter;mline;linetype;shape;style;transaction;generated" TIMEOUT 60)
    add_test(NAME libdxfrw_mline_ltype_mixed_segments
        COMMAND libdxfrw_dxf_fixture_tests --mline-ltype-mixed-segments)
    set_tests_properties(libdxfrw_mline_ltype_mixed_segments PROPERTIES
        LABELS "fast;dxf;adapter;mline;linetype;style;transaction;generated" TIMEOUT 60)
    add_test(NAME libdxfrw_mline_ltype_reverse_shape_text
        COMMAND libdxfrw_dxf_fixture_tests --mline-ltype-reverse-shape-text)
    set_tests_properties(libdxfrw_mline_ltype_reverse_shape_text PROPERTIES
        LABELS "fast;dxf;adapter;mline;linetype;style;transaction;generated" TIMEOUT 60)
    add_test(NAME libdxfrw_mline_style_authority
        COMMAND libdxfrw_dxf_fixture_tests --mline-style-authority)
    set_tests_properties(libdxfrw_mline_style_authority PROPERTIES
        LABELS "fast;dxf;adapter;mline;transaction;generated" TIMEOUT 60)
    add_test(NAME libdxfrw_scale_body_transactions
        COMMAND libdxfrw_dxf_fixture_tests --scale-body-transactions)
    set_tests_properties(libdxfrw_scale_body_transactions PROPERTIES
        LABELS "fast;dxf;adapter;scale;transaction;generated" TIMEOUT 60)
    add_test(NAME libdxfrw_dictionaryvar_legacy
        COMMAND libdxfrw_dxf_fixture_tests --dictionaryvar-legacy)
    set_tests_properties(libdxfrw_dictionaryvar_legacy PROPERTIES
        LABELS "fast;dxf;adapter;dictionary;text;transaction;generated" TIMEOUT 60)
    add_test(NAME libdxfrw_dictionaryvar_transactions
        COMMAND libdxfrw_dxf_fixture_tests --dictionaryvar-transactions)
    set_tests_properties(libdxfrw_dictionaryvar_transactions PROPERTIES
        LABELS "fast;dxf;adapter;dictionary;transaction;generated" TIMEOUT 60)
    add_test(NAME libdxfrw_dictionaryvar_custom_owners
        COMMAND libdxfrw_dxf_fixture_tests --dictionaryvar-custom-owners)
    set_tests_properties(libdxfrw_dictionaryvar_custom_owners PROPERTIES
        LABELS "fast;dxf;adapter;dictionary;transaction;generated" TIMEOUT 60)
    add_test(NAME libdxfrw_header_material_bindings
        COMMAND libdxfrw_dxf_fixture_tests --header-material-bindings)
    set_tests_properties(libdxfrw_header_material_bindings PROPERTIES
        LABELS "fast;dxf;adapter;material;header;writer;graph" TIMEOUT 120)
    add_test(NAME libdxfrw_layer_material_bindings
        COMMAND libdxfrw_dxf_fixture_tests --layer-material-bindings)
    set_tests_properties(libdxfrw_layer_material_bindings PROPERTIES
        LABELS "fast;dxf;adapter;material;layer;writer;graph" TIMEOUT 60)
    add_test(NAME libdxfrw_material_consumer_bindings
        COMMAND libdxfrw_dxf_fixture_tests --material-consumer-bindings)
    set_tests_properties(libdxfrw_material_consumer_bindings PROPERTIES
        LABELS "fast;dxf;adapter;material;writer;graph" TIMEOUT 120)
    add_test(NAME libdxfrw_material_root_output_compatibility
        COMMAND libdxfrw_dxf_fixture_tests --material-object-order)
    set_tests_properties(libdxfrw_material_root_output_compatibility PROPERTIES
        LABELS "fast;dxf;adapter;material;generated" TIMEOUT 30)
    add_test(NAME libdxfrw_material_color_lexemes
        COMMAND libdxfrw_dxf_fixture_tests --material-color-lexemes)
    set_tests_properties(libdxfrw_material_color_lexemes PROPERTIES
        LABELS "fast;dxf;reader;writer;material;generated" TIMEOUT 30)
    add_test(NAME libdxfrw_material_strict_color_closure
        COMMAND libdxfrw_dxf_fixture_tests --material-strict-color-closure)
    set_tests_properties(libdxfrw_material_strict_color_closure PROPERTIES
        LABELS "fast;dxf;adapter;material;transaction;generated" TIMEOUT 30)
    add_test(NAME libdxfrw_dxf_consumer_finalization
        COMMAND libdxfrw_dxf_fixture_tests --dxf-consumer-finalization)
    set_tests_properties(libdxfrw_dxf_consumer_finalization PROPERTIES
        LABELS "fast;dxf;writer;transaction;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_classic_vertex_xdata
        COMMAND libdxfrw_dxf_fixture_tests --classic-vertex-xdata)
    set_tests_properties(libdxfrw_classic_vertex_xdata PROPERTIES
        LABELS "fast;dxf;identity;polyline;xdata" TIMEOUT 15)
    add_test(NAME libdxfrw_insert_child_handle_rollback
        COMMAND libdxfrw_dxf_fixture_tests --insert-child-handle-rollback)
    set_tests_properties(libdxfrw_insert_child_handle_rollback PROPERTIES
        LABELS "fast;dxf;insert;transaction;retry;readback" TIMEOUT 30)
    add_test(NAME libdxfrw_insert_attribute_count_bound
        COMMAND libdxfrw_dxf_fixture_tests --insert-attribute-count-bound)
    set_tests_properties(libdxfrw_insert_attribute_count_bound PROPERTIES
        LABELS "fast;dxf;insert;resources;generated;transaction" TIMEOUT 15)
    add_test(NAME libdxfrw_dictionary_key_admission
        COMMAND libdxfrw_dxf_fixture_tests --dictionary-key-admission)
    set_tests_properties(libdxfrw_dictionary_key_admission PROPERTIES
        LABELS "fast;dxf;writer;dictionary;graph" TIMEOUT 15)
    add_test(NAME libdxfrw_mtext_column_object_scope
        COMMAND libdxfrw_dxf_fixture_tests --mtext-column-object-scope)
    set_tests_properties(libdxfrw_mtext_column_object_scope PROPERTIES
        LABELS "fast;dxf;mtext;columns" TIMEOUT 30)
    add_test(NAME libdxfrw_legacy_mtext_xdata_identity
        COMMAND libdxfrw_dxf_fixture_tests --legacy-mtext-xdata-identity)
    set_tests_properties(libdxfrw_legacy_mtext_xdata_identity PROPERTIES
        LABELS "fast;dxf;mtext;columns;xdata;identity;transaction" TIMEOUT 15)
    add_test(NAME libdxfrw_header_custom_property_roundtrip
        COMMAND libdxfrw_dxf_fixture_tests --header-custom-property-roundtrip)
    set_tests_properties(libdxfrw_header_custom_property_roundtrip PROPERTIES
        LABELS "fast;dxf;header" TIMEOUT 60)
    add_test(NAME libdxfrw_header_required_versions
        COMMAND libdxfrw_dxf_fixture_tests --header-required-versions)
    set_tests_properties(libdxfrw_header_required_versions PROPERTIES
        LABELS "fast;dxf;header" TIMEOUT 60)
    add_test(NAME libdxfrw_header_cepsnid
        COMMAND libdxfrw_dxf_fixture_tests --header-cepsnid)
    set_tests_properties(libdxfrw_header_cepsnid PROPERTIES
        LABELS "fast;dxf;header;handles" TIMEOUT 60)
    add_test(NAME libdxfrw_header_repeated_groups
        COMMAND libdxfrw_dxf_fixture_tests --header-repeated-groups)
    set_tests_properties(libdxfrw_header_repeated_groups PROPERTIES
        LABELS "fast;dxf;header" TIMEOUT 60)
    add_test(NAME libdxfrw_header_unknown_variable_roundtrip
        COMMAND libdxfrw_dxf_fixture_tests
            --header-unknown-variable-roundtrip)
    set_tests_properties(libdxfrw_header_unknown_variable_roundtrip PROPERTIES
        LABELS "fast;dxf;header" TIMEOUT 60)
    add_test(NAME libdxfrw_ltype_mirror_authority
        COMMAND libdxfrw_dxf_fixture_tests --ltype-mirror-authority)
    set_tests_properties(libdxfrw_ltype_mirror_authority PROPERTIES
        LABELS "fast;dxf;linetype" TIMEOUT 30)
    add_test(NAME libdxfrw_complex_ltype_r12_refusal_retry
        COMMAND libdxfrw_dxf_fixture_tests --complex-ltype-r12-refusal-retry)
    set_tests_properties(libdxfrw_complex_ltype_r12_refusal_retry PROPERTIES
        LABELS "fast;dxf;linetype;versions;retry" TIMEOUT 30)
    add_test(NAME libdxfrw_linetype_extension_graph
        COMMAND libdxfrw_dxf_fixture_tests --linetype-extension-graph)
    set_tests_properties(libdxfrw_linetype_extension_graph PROPERTIES
        LABELS "fast;dxf;graph;linetype" TIMEOUT 30)
    add_test(NAME libdxfrw_layer_dwg_cmc_roundtrip
        COMMAND libdxfrw_dxf_fixture_tests --layer-dxf-color-policy)
    set_tests_properties(libdxfrw_layer_dwg_cmc_roundtrip PROPERTIES
        LABELS "fast;dxf;dwg;color" TIMEOUT 30)
    add_test(NAME libdxfrw_layer_truecolor_version_loss_retry
        COMMAND libdxfrw_dxf_fixture_tests --layer-truecolor-version-loss-retry)
    set_tests_properties(libdxfrw_layer_truecolor_version_loss_retry PROPERTIES
        LABELS "fast;dxf;layer;color;versions;retry" TIMEOUT 30)
    add_test(NAME libdxfrw_view_vport_ambient_cmc
        COMMAND libdxfrw_dxf_fixture_tests --view-vport-ambient-cmc)
    set_tests_properties(libdxfrw_view_vport_ambient_cmc PROPERTIES
        LABELS "fast;dxf;dwg;color" TIMEOUT 30)
    add_test(NAME libdxfrw_vport_version_loss_refusal_retry
        COMMAND libdxfrw_dxf_fixture_tests --vport-version-loss-refusal-retry)
    set_tests_properties(libdxfrw_vport_version_loss_refusal_retry PROPERTIES
        LABELS "fast;dxf;vport;versions;retry" TIMEOUT 30)
    add_test(NAME libdxfrw_dictionary_with_default_writer
        COMMAND libdxfrw_dxf_fixture_tests --dictionary-with-default-writer)
    set_tests_properties(libdxfrw_dictionary_with_default_writer PROPERTIES
        LABELS "fast;dxf;objects" TIMEOUT 30)
    add_test(NAME libdxfrw_dxf_binary_group_width_guard
        COMMAND libdxfrw_dxf_fixture_tests --dxf-binary-group-width-guard)
    set_tests_properties(libdxfrw_dxf_binary_group_width_guard PROPERTIES
        LABELS "fast;dxf;binary" TIMEOUT 30)
    add_test(NAME libdxfrw_block_xref_path_roundtrip
        COMMAND libdxfrw_dxf_fixture_tests --block-xref-path-roundtrip)
    set_tests_properties(libdxfrw_block_xref_path_roundtrip PROPERTIES
        LABELS "fast;dxf;xref" TIMEOUT 30)
    add_test(NAME libdxfrw_mline_vector_component_order
        COMMAND libdxfrw_dxf_fixture_tests --mline-vector-component-order)
    set_tests_properties(libdxfrw_mline_vector_component_order PROPERTIES
        LABELS "fast;dxf;geometry" TIMEOUT 30)
    add_test(NAME libdxfrw_lwpolyline_point_component_order
        COMMAND libdxfrw_dxf_fixture_tests --lwpolyline-point-component-order)
    set_tests_properties(libdxfrw_lwpolyline_point_component_order PROPERTIES
        LABELS "fast;dxf;reader;lwpolyline;geometry;allocation" TIMEOUT 30)
    add_test(NAME libdxfrw_leader_point_component_order
        COMMAND libdxfrw_dxf_fixture_tests --leader-point-component-order)
    set_tests_properties(libdxfrw_leader_point_component_order PROPERTIES
        LABELS "fast;dxf;reader;leader;geometry" TIMEOUT 30)
    add_test(NAME libdxfrw_leader_dxf_allocation_failure
        COMMAND libdxfrw_dxf_fixture_tests --leader-dxf-allocation-failure)
    set_tests_properties(libdxfrw_leader_dxf_allocation_failure PROPERTIES
        LABELS "fast;dxf;reader;leader;allocation;retry" TIMEOUT 30)
    add_test(NAME libdxfrw_spline_dxf_allocation_failure
        COMMAND libdxfrw_dxf_fixture_tests
            --spline-dxf-allocation-failure)
    set_tests_properties(libdxfrw_spline_dxf_allocation_failure PROPERTIES
        LABELS "fast;dxf;reader;spline;allocation;retry" TIMEOUT 30)
    add_test(NAME libdxfrw_mleader_column_size_allocation_failure
        COMMAND libdxfrw_dxf_fixture_tests
            --mleader-column-size-allocation-failure)
    set_tests_properties(libdxfrw_mleader_column_size_allocation_failure PROPERTIES
        LABELS "fast;dxf;reader;mleader;allocation;retry" TIMEOUT 30)
    add_test(NAME libdxfrw_mleader_arrowhead_allocation_failure
        COMMAND libdxfrw_dxf_fixture_tests
            --mleader-arrowhead-allocation-failure)
    set_tests_properties(libdxfrw_mleader_arrowhead_allocation_failure PROPERTIES
        LABELS "fast;dxf;reader;mleader;allocation;retry" TIMEOUT 30)
    add_test(NAME libdxfrw_mleader_block_label_allocation_failure
        COMMAND libdxfrw_dxf_fixture_tests
            --mleader-block-label-allocation-failure)
    set_tests_properties(libdxfrw_mleader_block_label_allocation_failure PROPERTIES
        LABELS "fast;dxf;reader;mleader;allocation;retry" TIMEOUT 30)
    add_test(NAME libdxfrw_mleader_nested_allocation_failure
        COMMAND libdxfrw_dxf_fixture_tests
            --mleader-nested-allocation-failure)
    set_tests_properties(libdxfrw_mleader_nested_allocation_failure PROPERTIES
        LABELS "fast;dxf;reader;mleader;allocation;retry" TIMEOUT 30)
    add_test(NAME libdxfrw_hatch_spline_dxf_allocation_failure
        COMMAND libdxfrw_dxf_fixture_tests
            --hatch-spline-dxf-allocation-failure)
    set_tests_properties(libdxfrw_hatch_spline_dxf_allocation_failure PROPERTIES
        LABELS "fast;dxf;reader;hatch;spline;allocation;retry" TIMEOUT 30)
    add_test(NAME libdxfrw_hatch_dxf_allocation_failure
        COMMAND libdxfrw_dxf_fixture_tests
            --hatch-dxf-allocation-failure)
    set_tests_properties(libdxfrw_hatch_dxf_allocation_failure PROPERTIES
        LABELS "fast;dxf;reader;hatch;allocation;retry" TIMEOUT 30)
    add_test(NAME libdxfrw_mesh_dxf_allocation_failure
        COMMAND libdxfrw_dxf_fixture_tests --mesh-dxf-allocation-failure)
    set_tests_properties(libdxfrw_mesh_dxf_allocation_failure PROPERTIES
        LABELS "fast;dxf;reader;mesh;allocation;retry" TIMEOUT 30)
    add_test(NAME libdxfrw_mpolygon_degenerate_count_domain
        COMMAND libdxfrw_dxf_fixture_tests
            --mpolygon-degenerate-count-domain)
    set_tests_properties(libdxfrw_mpolygon_degenerate_count_domain PROPERTIES
        LABELS "fast;dxf;reader;mpolygon;validation;retry" TIMEOUT 30)
    add_test(NAME libdxfrw_mpolygon_aci_domain
        COMMAND libdxfrw_dxf_fixture_tests --mpolygon-aci-domain)
    set_tests_properties(libdxfrw_mpolygon_aci_domain PROPERTIES
        LABELS "fast;dxf;reader;writer;mpolygon;validation;retry" TIMEOUT 30)
    add_test(NAME libdxfrw_hatch_gradient_stop_color_domain
        COMMAND libdxfrw_dxf_fixture_tests
            --hatch-gradient-stop-color-domain)
    set_tests_properties(libdxfrw_hatch_gradient_stop_color_domain PROPERTIES
        LABELS "fast;dxf;reader;hatch;gradient;validation;retry" TIMEOUT 30)
    add_test(NAME libdxfrw_hatch_spline_point_component_order
        COMMAND libdxfrw_dxf_fixture_tests
            --hatch-spline-point-component-order)
    set_tests_properties(libdxfrw_hatch_spline_point_component_order PROPERTIES
        LABELS "fast;dxf;reader;hatch;spline;geometry" TIMEOUT 30)
    add_test(NAME libdxfrw_mline_dxf_allocation_failure
        COMMAND libdxfrw_dxf_fixture_tests --mline-dxf-allocation-failure)
    set_tests_properties(libdxfrw_mline_dxf_allocation_failure PROPERTIES
        LABELS "fast;dxf;reader;mline;allocation;retry" TIMEOUT 30)
    add_test(NAME libdxfrw_classes_wire_census
        COMMAND libdxfrw_dxf_fixture_tests --classes-wire-census)
    set_tests_properties(libdxfrw_classes_wire_census PROPERTIES
        LABELS "fast;dxf;classes;wire" TIMEOUT 30)
    add_test(NAME libdxfrw_dimstyle_subclass_wire_census
        COMMAND libdxfrw_dxf_fixture_tests --dimstyle-subclass-wire-census)
    set_tests_properties(libdxfrw_dimstyle_subclass_wire_census PROPERTIES
        LABELS "fast;dxf;dimstyle;wire" TIMEOUT 30)

    add_executable(libdxfrw_dwg_fixture_tests
        tests/dwg_fixture_tests.cpp
        ${_libdxfrw_root_test_adapter_source})
    target_compile_features(libdxfrw_dwg_fixture_tests PRIVATE cxx_std_17)
    set_property(SOURCE tests/dwg_fixture_tests.cpp APPEND PROPERTY COMPILE_DEFINITIONS
        LIBDXFRW_DWG_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/dwg")
    target_include_directories(libdxfrw_dwg_fixture_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(libdxfrw_dwg_fixture_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_dwg_fixtures
        COMMAND libdxfrw_dwg_fixture_tests)
    add_test(NAME libdxfrw_ac1021_historical_enc_role_profile
        COMMAND libdxfrw_dwg_fixture_tests --ordinary-enc-ac1021-only)
    set_tests_properties(libdxfrw_ac1021_historical_enc_role_profile PROPERTIES
        LABELS "fast;dwg;AC1021;compatibility;handles" TIMEOUT 15)

    add_test(NAME libdxfrw_hatch_mpolygon_inactive_gradient_guard
        COMMAND libdxfrw_dwg_fixture_tests --hatch-mpolygon-inactive-gradient-guard)
    set_tests_properties(libdxfrw_hatch_mpolygon_inactive_gradient_guard PROPERTIES
        LABELS "fast;dxf;hatch;mpolygon;loss-guard;transaction"
        TIMEOUT 30)

    add_test(NAME libdxfrw_implementation_speed
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_implementation_speed.py
            --metadata
            ${CMAKE_CURRENT_SOURCE_DIR}/metadata/implementation-speed-baseline-v1.json)

    set_property(TEST
        libdxfrw_dwg_local_roundtrip
        libdxfrw_dwg_reader_matrix
        libdxfrw_diagnostic
        libdxfrw_dwg_fixtures
        libdxfrw_qualified_differential_v2
        libdxfrw_qualified_semantic_fields
        libdxfrw_qualified_support_metadata
        APPEND PROPERTY LABELS native-qualification-focused)
    libdxfrw_check_root_test_adapter_context()
endif()

# INSTALLATION
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/libdxfrw)

# Package license material is required for runtime-only installs too. Conan's
# package() calls cmake.install(), so it follows this same delivery policy.
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/COPYING"
    "${CMAKE_CURRENT_SOURCE_DIR}/LICENSES/Unicode-3.0.txt"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/libdxfrw")

# A .pc file must resolve the install it lives in, including --install --prefix
# and later relocation. GNUInstallDirs may use lib64 or a multiarch libdir, so
# derive the path back to the prefix instead of assuming ../.. . Absolute
# destinations deliberately remain absolute and are not relocation promises.
if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
    set(LIBDXFRW_PC_PREFIX "${CMAKE_INSTALL_PREFIX}")
    set(LIBDXFRW_PC_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
else()
    file(RELATIVE_PATH LIBDXFRW_PC_PREFIX_RELATIVE
        "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/pkgconfig"
        "${CMAKE_INSTALL_PREFIX}")
    set(LIBDXFRW_PC_PREFIX "\${pcfiledir}/${LIBDXFRW_PC_PREFIX_RELATIVE}")
    # --define-prefix implementations may guess the wrong prefix for a nested
    # libdir. Keep flags independent of that optional override's heuristic.
    set(LIBDXFRW_PC_LIBDIR "\${pcfiledir}/..")
endif()
if(IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
    set(LIBDXFRW_PC_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}")
else()
    if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
        set(_dxfrw_pc_directory "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    else()
        set(_dxfrw_pc_directory "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    endif()
    file(RELATIVE_PATH LIBDXFRW_PC_INCLUDE_RELATIVE
        "${_dxfrw_pc_directory}"
        "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")
    set(LIBDXFRW_PC_INCLUDEDIR "\${pcfiledir}/${LIBDXFRW_PC_INCLUDE_RELATIVE}")
endif()
foreach(_dxfrw_pc_path IN ITEMS LIBDXFRW_PC_PREFIX LIBDXFRW_PC_LIBDIR LIBDXFRW_PC_INCLUDEDIR)
    string(REPLACE " " "\\ " ${_dxfrw_pc_path} "${${_dxfrw_pc_path}}")
endforeach()
# FindThreads' flag branch requires -pthread during compilation too. Do not
# copy a link-only library (such as -lpthread), or invent flags for libc threads.
set(LIBDXFRW_PC_THREAD_CFLAGS "")
if(LIBDXFRW_BUILD_LIBRARY AND THREADS_HAVE_PTHREAD_ARG)
    set(LIBDXFRW_PC_THREAD_CFLAGS "-pthread")
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libdxfrw.pc.in libdxfrw.pc @ONLY)

if(LIBDXFRW_BUILD_LIBRARY)
    install(TARGETS dxfrw
        EXPORT libdxfrw-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    if(LIBDXFRW_INSTALL_DEV)
        install(EXPORT libdxfrw-targets
            FILE libdxfrwTargets.cmake
            NAMESPACE libdxfrw::
            DESTINATION ${INSTALL_CONFIGDIR}
        )
    endif()

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

    if(LIBDXFRW_INSTALL_DEV)
        install(FILES
                ${installdxfrw_hdrs}
                DESTINATION ${LIBDXFRW_INSTALL_INCLUDEDIR}
        )
        # The imported public façades still expose these declarations.  Install
        # only the narrow transitive closure; HandleAllocator is now internal and the
        # DataStorage writer enum is declared by the installed foundational header.
        install(FILES ${libdxfrw_transitive_hdrs}
                DESTINATION ${LIBDXFRW_INSTALL_INCLUDEDIR}/intern)
        install(FILES
                ${CMAKE_CURRENT_BINARY_DIR}/libdxfrw.pc
                DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
        )
        install(FILES
                ${CMAKE_CURRENT_BINARY_DIR}/libdxfrwConfig.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/libdxfrwConfigVersion.cmake
                ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindLibdxfrw.cmake
                ${CMAKE_CURRENT_SOURCE_DIR}/cmake/libdxfrwConanThreads.cmake
                DESTINATION ${INSTALL_CONFIGDIR}
        )
    endif()
endif()
