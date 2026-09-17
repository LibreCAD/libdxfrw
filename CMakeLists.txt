CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

cmake_policy(SET CMP0048 NEW)
project(DXFRW VERSION 2.0.0)

# set preferred cache variables
option(LIBDXFRW_BUILD_LIBRARY "Build the libdxfrw library" ON)
option(LIBDXFRW_BUILD_DOC "Build the documentation" ON)
option(LIBDXFRW_BUILD_DWG2DXF "Build the dwg2dxf application" ON)
option(LIBDXFRW_BUILD_TESTS "Build dependency-free Wave 1 tests" OFF)
option(LIBDXFRW_BUILD_QUALIFICATION
    "Build the pinned target/standalone semantic qualification adapters" OFF)
option(LIBDXFRW_BUILD_LONG_FUZZ
    "Build the opt-in extended deterministic parser fuzz target" OFF)
set(LIBDXFRW_TARGET_GIT_DIR "" CACHE PATH
    "Git directory for the pinned LibreCAD target qualification build")
set(LIBDXFRW_LIBREDWG_DWGREAD "" CACHE FILEPATH
    "Pinned LibreDWG dwgread executable for semantic field qualification")

if(LIBDXFRW_BUILD_DWG2DXF AND NOT LIBDXFRW_BUILD_LIBRARY)
    message(FATAL_ERROR "LIBDXFRW_BUILD_DWG2DXF requires LIBDXFRW_BUILD_LIBRARY")
endif()
if(LIBDXFRW_BUILD_LONG_FUZZ AND NOT LIBDXFRW_BUILD_TESTS)
    message(FATAL_ERROR "LIBDXFRW_BUILD_LONG_FUZZ requires LIBDXFRW_BUILD_TESTS")
endif()
if(LIBDXFRW_BUILD_QUALIFICATION AND NOT LIBDXFRW_BUILD_TESTS)
    message(FATAL_ERROR "LIBDXFRW_BUILD_QUALIFICATION requires LIBDXFRW_BUILD_TESTS")
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

        add_executable(libdxfrw_semantic_adapter_standalone
            tests/semantic_differential_adapter.cpp)
        target_compile_features(libdxfrw_semantic_adapter_standalone
            PRIVATE cxx_std_17)
        target_compile_options(libdxfrw_semantic_adapter_standalone
            PRIVATE ${LIBDXFRW_ADAPTER_WARNING_FLAGS})
        target_compile_definitions(libdxfrw_semantic_adapter_standalone PRIVATE
            LIBDXFRW_SEMANTIC_SIDE_STANDALONE=1
            LIBDXFRW_SEMANTIC_ADAPTER_NAME="libdxfrw_semantic_adapter_standalone")
        target_include_directories(libdxfrw_semantic_adapter_standalone PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/src/intern)
        target_link_libraries(libdxfrw_semantic_adapter_standalone PRIVATE dxfrw)
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic_differential_adapter.cpp
            ${LIBDXFRW_QUALIFIED_DIFFERENTIAL_MANIFEST}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_admitted_candidate_fields.py
            ${LIBDXFRW_QUALIFIED_SEMANTIC_FIELDS_CONTRACT}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_ac1024_candidate_fields.py
            ${LIBDXFRW_AC1024_CANDIDATE_FIELDS_CONTRACT}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_ac1024_candidate_fields_v2.py
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
                --wrapper-source ${CMAKE_CURRENT_SOURCE_DIR}/cmake/semantic_adapter_target
                --configuration $<CONFIG>
                --parallel 2
                --receipt ${LIBDXFRW_SEMANTIC_TARGET_RECEIPT}
                --cmake ${CMAKE_COMMAND}
            DEPENDS
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_semantic_adapter.py
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic_differential_adapter.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/cmake/semantic_adapter_target/CMakeLists.txt
                ${CMAKE_CURRENT_SOURCE_DIR}/metadata/libdxfrw-target-lock.json
                ${CMAKE_CURRENT_SOURCE_DIR}/metadata/libdxfrw-target-source-manifest.txt
                ${LIBDXFRW_QUALIFIED_DIFFERENTIAL_MANIFEST}
            COMMENT "Building receipt-bound pinned-target semantic adapter"
            VERBATIM)

        add_custom_target(libdxfrw_semantic_adapter_standalone_receipt
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_semantic_adapter.py
                receipt-standalone
                --executable $<TARGET_FILE:libdxfrw_semantic_adapter_standalone>
                --static-library $<TARGET_FILE:dxfrw>
                --cmake-build-dir ${CMAKE_CURRENT_BINARY_DIR}
                --adapter-source ${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic_differential_adapter.cpp
                --semantic-manifest ${LIBDXFRW_QUALIFIED_DIFFERENTIAL_MANIFEST}
                --config-source ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
                --config-source ${CMAKE_CURRENT_SOURCE_DIR}/libdxfrw_sources.cmake
                --configuration $<CONFIG>
                --receipt ${LIBDXFRW_SEMANTIC_STANDALONE_RECEIPT}
                --cmake ${CMAKE_COMMAND}
            DEPENDS
                libdxfrw_semantic_adapter_standalone
                dxfrw
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_semantic_adapter.py
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic_differential_adapter.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
                ${CMAKE_CURRENT_SOURCE_DIR}/libdxfrw_sources.cmake
                ${LIBDXFRW_QUALIFIED_DIFFERENTIAL_MANIFEST}
            COMMENT "Attesting standalone semantic adapter build"
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
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_ac1024_candidate_fields_v2.py
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
        add_test(NAME libdxfrw_ac1024_candidate_fields
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_ac1024_candidate_fields.py
                --self-test
                --manifest ${LIBDXFRW_AC1024_CANDIDATE_FIELDS_CONTRACT})
        add_test(NAME libdxfrw_ac1024_candidate_fields_v2
            COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_ac1024_candidate_fields_v2.py
                --self-test
                --manifest ${LIBDXFRW_AC1024_CANDIDATE_FIELDS_V2_CONTRACT})
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

    add_executable(libdxfrw_wave1_tests tests/wave1_tests.cpp)
    target_compile_features(libdxfrw_wave1_tests PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_wave1_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intern
        ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf)
    target_link_libraries(libdxfrw_wave1_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_wave1 COMMAND libdxfrw_wave1_tests)

    add_executable(libdxfrw_dwg_local_roundtrip
        tests/dwg_local_roundtrip_tests.cpp
        dwg2dxf/dx_iface.cpp)
    target_compile_features(libdxfrw_dwg_local_roundtrip PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_dwg_local_roundtrip PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(libdxfrw_dwg_local_roundtrip PRIVATE dxfrw)
    add_test(NAME libdxfrw_dwg_local_roundtrip
        COMMAND libdxfrw_dwg_local_roundtrip)

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

    add_executable(libdxfrw_dwg_object_vectors_tests
        tests/dwg_object_vectors_tests.cpp)
    target_compile_features(libdxfrw_dwg_object_vectors_tests PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_dwg_object_vectors_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intern)
    target_link_libraries(libdxfrw_dwg_object_vectors_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_dwg_object_vectors
        COMMAND libdxfrw_dwg_object_vectors_tests)

    add_executable(libdxfrw_hardening_tests
        tests/hardening_tests.cpp)
    target_compile_features(libdxfrw_hardening_tests PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_hardening_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intern)
    target_link_libraries(libdxfrw_hardening_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_hardening
        COMMAND libdxfrw_hardening_tests)

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
            DESTINATION ${INSTALL_CONFIGDIR}
    )
endif()
