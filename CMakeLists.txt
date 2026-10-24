CMAKE_MINIMUM_REQUIRED(VERSION 3.28)

cmake_policy(SET CMP0048 NEW)
project(DXFRW VERSION 2.0.0)

# set preferred cache variables
option(LIBDXFRW_BUILD_LIBRARY "Build the libdxfrw library" ON)
option(LIBDXFRW_BUILD_DOC "Build the documentation" ON)
option(LIBDXFRW_BUILD_DWG2DXF "Build the dwg2dxf application" ON)
option(LIBDXFRW_BUILD_TESTS "Build dependency-free Wave 1 tests" OFF)
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
    # dedicated tests/ directory.  It is added here (after dwg2dxf) so the
    # compile-only LibreCAD_3 source-compatibility gate can resolve the CLI
    # target when sample tests are enabled, while remaining absent from normal
    # library builds.
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
    add_subdirectory(tests)
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
                --converter $<TARGET_FILE:dwg2dxf>)
        set_tests_properties(dwg2dxf_cli_contract PROPERTIES
            LABELS "fast;cli;process" TIMEOUT 30)
        add_test(NAME dwg2dxf_unsupported_source_codepage
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/dwg2dxf_cli_contract_tests.py
                --converter $<TARGET_FILE:dwg2dxf>
                --unsupported-dwg-codepage-only)
        set_tests_properties(dwg2dxf_unsupported_source_codepage PROPERTIES
            LABELS "fast;cli;dwg;codepage" TIMEOUT 25)
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

    add_executable(libdxfrw_wave1_tests
        tests/wave1_tests.cpp
        dwg2dxf/dx_iface.cpp)
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
    add_test(NAME libdxfrw_dxf_tables_write_inspection
        COMMAND libdxfrw_symbol_output_tests --dxf-tables-write-inspection)
    set_tests_properties(libdxfrw_dxf_tables_write_inspection PROPERTIES
        LABELS "fast;dxf;writer;tables;transaction;generated" TIMEOUT 15)
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
        tests/dxf_symbol_output_tests.cpp dwg2dxf/dx_iface.cpp)
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
        dwg2dxf/dx_iface.cpp)
    target_compile_features(libdxfrw_dwg_local_roundtrip PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_dwg_local_roundtrip PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(libdxfrw_dwg_local_roundtrip PRIVATE dxfrw)
    add_test(NAME libdxfrw_dwg_local_roundtrip
        COMMAND libdxfrw_dwg_local_roundtrip)
    add_test(NAME libdxfrw_dxf_spline_helix_source_identities
        COMMAND libdxfrw_dwg_local_roundtrip --dxf-spline-helix-source-identities)
    set_tests_properties(libdxfrw_dxf_spline_helix_source_identities PROPERTIES
        LABELS "fast;dxf;geometry;identity;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_table_content_dwg_object_roundtrip
        COMMAND libdxfrw_dwg_local_roundtrip --table-content-dwg-object-roundtrip)
    set_tests_properties(libdxfrw_table_content_dwg_object_roundtrip PROPERTIES
        LABELS "fast;dwg;generated;tables;graph;writer" TIMEOUT 30)
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
        dwg2dxf/dx_iface.cpp)
    target_compile_features(libdxfrw_dwg_reader_matrix_tests PRIVATE cxx_std_17)
    target_compile_definitions(libdxfrw_dwg_reader_matrix_tests PRIVATE
        LIBDXFRW_READER_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/dwg")
    target_include_directories(libdxfrw_dwg_reader_matrix_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intern
        ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf)
    target_link_libraries(libdxfrw_dwg_reader_matrix_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_dwg_reader_matrix
        COMMAND libdxfrw_dwg_reader_matrix_tests)
    add_test(NAME libdxfrw_dwg_r2018_auxheader_trailer
        COMMAND libdxfrw_dwg_reader_matrix_tests
            --r2018-auxheader-trailer)
    set_tests_properties(libdxfrw_dwg_r2018_auxheader_trailer PROPERTIES
        LABELS "fast;dwg;header;generated;AC1027;AC1032"
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
    add_test(NAME libdxfrw_dwg_r2007_class_string_footer
        COMMAND libdxfrw_dwg_reader_matrix_tests --r2007-class-string-footer)
    set_tests_properties(libdxfrw_dwg_r2007_class_string_footer PROPERTIES
        LABELS "fast;dwg;framing;generated"
        TIMEOUT 30)
    add_test(NAME libdxfrw_dwg_r2007_section_map_page_extent
        COMMAND libdxfrw_dwg_reader_matrix_tests
            --r2007-section-map-page-extent)
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
    add_executable(libdxfrw_raster_dwg_wire_tests tests/raster_dwg_wire_tests.cpp)
    target_include_directories(libdxfrw_raster_dwg_wire_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf)
    target_compile_features(libdxfrw_raster_dwg_wire_tests PRIVATE cxx_std_14)
    add_test(NAME libdxfrw_raster_dwg_independent_wire COMMAND libdxfrw_raster_dwg_wire_tests)
    set_tests_properties(libdxfrw_raster_dwg_independent_wire PROPERTIES
        LABELS "fast;dwg;image;wire;generated;independent" TIMEOUT 30)
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
    add_test(NAME libdxfrw_persistent_manager_exact_footer
        COMMAND libdxfrw_dwg_object_vectors_tests --persistent-manager-exact-footer)
    set_tests_properties(libdxfrw_persistent_manager_exact_footer PROPERTIES
        LABELS "fast;dwg;object-vectors;associative;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_geodata_civil_tail_target_matrix
        COMMAND libdxfrw_dwg_object_vectors_tests
            --geodata-civil-tail-target-matrix)
    set_tests_properties(libdxfrw_geodata_civil_tail_target_matrix
        PROPERTIES LABELS "fast;dwg;object-vectors;geodata" TIMEOUT 10)
    add_test(NAME libdxfrw_associative_action_value_kinds
        COMMAND libdxfrw_dwg_object_vectors_tests
            --associative-action-value-kinds)
    set_tests_properties(libdxfrw_associative_action_value_kinds PROPERTIES
        LABELS "fast;dwg;object-vectors" TIMEOUT 10)
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
    add_test(NAME libdxfrw_field_oversized_child_value_count
        COMMAND libdxfrw_dwg_object_vectors_tests
            --field-oversized-child-value-count)
    set_tests_properties(libdxfrw_field_oversized_child_value_count PROPERTIES
        LABELS "fast;dwg;object-vectors;field" TIMEOUT 10)
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
    add_test(NAME libdxfrw_dxf_operation_admission
        COMMAND libdxfrw_hardening_tests --dxf-operation-admission)
    set_tests_properties(libdxfrw_dxf_operation_admission PROPERTIES
        LABELS "fast;dxf;reader;writer;transaction;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_section_settings_color_code
        COMMAND libdxfrw_hardening_tests --section-settings-color-code)
    set_tests_properties(libdxfrw_section_settings_color_code PROPERTIES
        LABELS "fast;dxf;reader;section-settings"
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

    add_executable(libdxfrw_dxf_fixture_tests
        tests/dxf_fixture_tests.cpp
        dwg2dxf/dx_iface.cpp)
    target_compile_features(libdxfrw_dxf_fixture_tests PRIVATE cxx_std_17)
    target_compile_definitions(libdxfrw_dxf_fixture_tests PRIVATE
        LIBDXFRW_DXF_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/dxf")
    target_include_directories(libdxfrw_dxf_fixture_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(libdxfrw_dxf_fixture_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_dxf_fixtures
        COMMAND libdxfrw_dxf_fixture_tests)
    add_test(NAME libdxfrw_dxf_consumer_finalization
        COMMAND libdxfrw_dxf_fixture_tests --dxf-consumer-finalization)
    set_tests_properties(libdxfrw_dxf_consumer_finalization PROPERTIES
        LABELS "fast;dxf;writer;transaction;generated" TIMEOUT 15)
    add_test(NAME libdxfrw_classic_vertex_xdata
        COMMAND libdxfrw_dxf_fixture_tests --classic-vertex-xdata)
    set_tests_properties(libdxfrw_classic_vertex_xdata PROPERTIES
        LABELS "fast;dxf;identity;polyline;xdata" TIMEOUT 15)
    add_test(NAME libdxfrw_dictionary_key_admission
        COMMAND libdxfrw_dxf_fixture_tests --dictionary-key-admission)
    set_tests_properties(libdxfrw_dictionary_key_admission PROPERTIES
        LABELS "fast;dxf;writer;dictionary;graph" TIMEOUT 15)
    add_test(NAME libdxfrw_mtext_column_object_scope
        COMMAND libdxfrw_dxf_fixture_tests --mtext-column-object-scope)
    set_tests_properties(libdxfrw_mtext_column_object_scope PROPERTIES
        LABELS "fast;dxf;mtext;columns" TIMEOUT 30)
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
    add_test(NAME libdxfrw_linetype_extension_graph
        COMMAND libdxfrw_dxf_fixture_tests --linetype-extension-graph)
    set_tests_properties(libdxfrw_linetype_extension_graph PROPERTIES
        LABELS "fast;dxf;graph;linetype" TIMEOUT 30)
    add_test(NAME libdxfrw_layer_dwg_cmc_roundtrip
        COMMAND libdxfrw_dxf_fixture_tests --layer-dxf-color-policy)
    set_tests_properties(libdxfrw_layer_dwg_cmc_roundtrip PROPERTIES
        LABELS "fast;dxf;dwg;color" TIMEOUT 30)
    add_test(NAME libdxfrw_view_vport_ambient_cmc
        COMMAND libdxfrw_dxf_fixture_tests --view-vport-ambient-cmc)
    set_tests_properties(libdxfrw_view_vport_ambient_cmc PROPERTIES
        LABELS "fast;dxf;dwg;color" TIMEOUT 30)
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
        dwg2dxf/dx_iface.cpp)
    target_compile_features(libdxfrw_dwg_fixture_tests PRIVATE cxx_std_17)
    target_compile_definitions(libdxfrw_dwg_fixture_tests PRIVATE
        LIBDXFRW_DWG_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/dwg")
    target_include_directories(libdxfrw_dwg_fixture_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(libdxfrw_dwg_fixture_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_dwg_fixtures
        COMMAND libdxfrw_dwg_fixture_tests)

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
                ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindLibdxfrw.cmake
                ${CMAKE_CURRENT_SOURCE_DIR}/cmake/libdxfrwConanThreads.cmake
                DESTINATION ${INSTALL_CONFIGDIR}
        )
    endif()
endif()
