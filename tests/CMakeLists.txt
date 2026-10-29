# Sample-backed DWG reader smoke tests for libdxfrw.
#
# These tests call the reader directly so successful parsing is not conflated
# with the CLI's intentionally stricter complete-export admission. DXF writer
# behavior and strict conversion refusal are tested in their own lanes.
#
# Sample files live in tests/samples/<AC_VERSION>/. A test is registered
# for every .dwg under each version directory at configure time. Drop a
# new .dwg in to register a new test on next CMake reconfigure.
#
# The harness verifies:
#   1. The sample's first 6 bytes literally match its declared AC version.
#   2. DWG import succeeds with no entity/object parse failures.
#
# Test labels: "dwg_read", the AC version (AC1021 / AC1024 / ...), and
# "sample". This lane makes no DXF export or semantic-completeness claim.
#
# Run:
#   cd build && ctest -L dwg_read --output-on-failure
#   cd build && ctest -L AC1024 --output-on-failure
#   cd build && ctest -R '^dwg_read_' --output-on-failure

# Optional external qualification for AC1015 SAT-v1 modeler carriers. Neither
# the DWG nor independent reference DXF is vendored; callers provide paths to
# their local evidence. The generated input/output copies remain in build/.
function(libdxfrw_require_test_cli control)
    if(NOT TARGET dwg2dxf)
        message(FATAL_ERROR
            "${control} requires LIBDXFRW_BUILD_DWG2DXF=ON")
    endif()
endfunction()

option(LIBDXFRW_ENABLE_EXTERNAL_SAT_V1_CLI_CONTROL
    "Enable external AC1015 SAT-v1 dwg2dxf comparison" OFF)
set(LIBDXFRW_SAT_V1_DWG "" CACHE FILEPATH
    "External AC1015 DWG with a qualified SAT-v1 3DSOLID carrier")
set(LIBDXFRW_SAT_V1_REFERENCE_DXF "" CACHE FILEPATH
    "Independent DXF export of the same AC1015 SAT-v1 DWG")
if(LIBDXFRW_ENABLE_EXTERNAL_SAT_V1_CLI_CONTROL)
    libdxfrw_require_test_cli(LIBDXFRW_ENABLE_EXTERNAL_SAT_V1_CLI_CONTROL)
    foreach(_sat_v1_input IN ITEMS
            LIBDXFRW_SAT_V1_DWG LIBDXFRW_SAT_V1_REFERENCE_DXF)
        if(NOT EXISTS "${${_sat_v1_input}}")
            message(FATAL_ERROR
                "Set ${_sat_v1_input} to an existing external evidence file")
        endif()
    endforeach()
    add_executable(libdxfrw_modeler_sat_dxf_compare
        ${CMAKE_CURRENT_SOURCE_DIR}/modeler_sat_dxf_compare.cpp)
    target_compile_features(libdxfrw_modeler_sat_dxf_compare PRIVATE cxx_std_17)
    target_compile_options(libdxfrw_modeler_sat_dxf_compare
        PRIVATE ${LIBDXFRW_WARNING_FLAGS})
    add_test(
        NAME dwg2dxf_freecad_external_sat_v1_cli
        COMMAND ${CMAKE_COMMAND}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            -DSAMPLE=${LIBDXFRW_SAT_V1_DWG}
            -DREFERENCE_DXF=${LIBDXFRW_SAT_V1_REFERENCE_DXF}
            -DCOMPARATOR=$<TARGET_FILE:libdxfrw_modeler_sat_dxf_compare>
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_sat_v1_external_cli_test.cmake.in
    )
    set_tests_properties(dwg2dxf_freecad_external_sat_v1_cli PROPERTIES
        LABELS "freecad;cli;external;integration"
        TIMEOUT 120)
endif()

# Keep the no-version FreeCAD CLI revision policy independently tested from
# full DWG input availability. The source-version mapping is pure policy.
add_executable(libdxfrw_dwg2dxf_version_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf_version_tests.cpp
)
target_compile_features(libdxfrw_dwg2dxf_version_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg2dxf_version_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg2dxf_version_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf
    ${CMAKE_SOURCE_DIR}/src
)
add_test(NAME dwg2dxf_version_policy
    COMMAND libdxfrw_dwg2dxf_version_tests)
set_tests_properties(dwg2dxf_version_policy PROPERTIES
    LABELS "cli;fast")

# Keep the concrete dwg2dxf adapter's cross-callback image link alive. The
# source DXF is generated at test time, passed through the real CLI twice, and
# read back through the same adapter; no image/DXF fixture is committed.
# Default-off codec guard; generated drawings exercise the actual replay boundary.
add_executable(libdxfrw_no_raw_sections_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_no_raw_sections_tests.cpp)
target_compile_features(libdxfrw_no_raw_sections_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_no_raw_sections_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_no_raw_sections_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_no_raw_sections_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_no_raw_sections COMMAND libdxfrw_no_raw_sections_tests)
set_tests_properties(libdxfrw_no_raw_sections PROPERTIES LABELS "dxf;fast;transaction" TIMEOUT 30)

include(${CMAKE_SOURCE_DIR}/cmake/libdxfrwTestAdapter.cmake)
add_executable(libdxfrw_mline_source_dependency_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_mline_source_dependency_tests.cpp
    ${_libdxfrw_test_adapter_source}
)
target_compile_features(libdxfrw_mline_source_dependency_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_mline_source_dependency_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_mline_source_dependency_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_mline_source_dependency_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_mline_source_dependency COMMAND libdxfrw_mline_source_dependency_tests)
set_tests_properties(libdxfrw_mline_source_dependency PROPERTIES LABELS "dxf;fast")
add_test(NAME libdxfrw_mline_layer_source_live
    COMMAND libdxfrw_mline_source_dependency_tests --layer-source-live)
set_tests_properties(libdxfrw_mline_layer_source_live PROPERTIES LABELS "dxf;fast" TIMEOUT 30)
add_test(NAME libdxfrw_mline_layer_carrier_geometry_guard
    COMMAND libdxfrw_mline_source_dependency_tests --mline-layer-carrier-geometry-guard)
set_tests_properties(libdxfrw_mline_layer_carrier_geometry_guard PROPERTIES
    LABELS "dxf;fast;transaction" TIMEOUT 30)
add_test(NAME libdxfrw_mline_dictionary_carrier_geometry_guard
    COMMAND libdxfrw_mline_source_dependency_tests --mline-dictionary-carrier-geometry-guard)
set_tests_properties(libdxfrw_mline_dictionary_carrier_geometry_guard PROPERTIES
    LABELS "dxf;fast;transaction" TIMEOUT 30)
add_test(NAME libdxfrw_mline_ltype_provider_geometry_guard
    COMMAND libdxfrw_mline_source_dependency_tests --mline-ltype-provider-geometry-guard)
set_tests_properties(libdxfrw_mline_ltype_provider_geometry_guard PROPERTIES
    LABELS "dxf;fast;transaction" TIMEOUT 30)
add_test(NAME libdxfrw_mline_style_body_geometry_guard
    COMMAND libdxfrw_mline_source_dependency_tests --mline-style-body-geometry-guard)
set_tests_properties(libdxfrw_mline_style_body_geometry_guard PROPERTIES
    LABELS "dxf;fast;transaction" TIMEOUT 30)
add_test(NAME libdxfrw_mline_entity_carrier_geometry_guard
    COMMAND libdxfrw_mline_source_dependency_tests --mline-entity-carrier-geometry-guard)
set_tests_properties(libdxfrw_mline_entity_carrier_geometry_guard PROPERTIES
    LABELS "dxf;fast;transaction" TIMEOUT 30)
add_test(NAME libdxfrw_mline_consumer_census_budget_guard
    COMMAND libdxfrw_mline_source_dependency_tests --mline-consumer-census-budget-guard)
set_tests_properties(libdxfrw_mline_consumer_census_budget_guard PROPERTIES
    LABELS "dxf;fast;transaction;resource-budget" TIMEOUT 30)
add_test(NAME libdxfrw_mline_ltype_source_live
    COMMAND libdxfrw_mline_source_dependency_tests --ltype-source-live)
set_tests_properties(libdxfrw_mline_ltype_source_live PROPERTIES LABELS "dxf;fast" TIMEOUT 30)
add_test(NAME libdxfrw_mline_root_dictionary_source_live
    COMMAND libdxfrw_mline_source_dependency_tests --mline-root-dictionary-source-live)
set_tests_properties(libdxfrw_mline_root_dictionary_source_live PROPERTIES LABELS "dxf;fast" TIMEOUT 30)
add_test(NAME libdxfrw_mline_style_dictionary_source_live
    COMMAND libdxfrw_mline_source_dependency_tests --mline-style-dictionary-source-live)
set_tests_properties(libdxfrw_mline_style_dictionary_source_live PROPERTIES LABELS "dxf;fast" TIMEOUT 30)
add_test(NAME libdxfrw_mline_style_body_source_live
    COMMAND libdxfrw_mline_source_dependency_tests --mline-style-body-source-live)
set_tests_properties(libdxfrw_mline_style_body_source_live PROPERTIES LABELS "dxf;fast" TIMEOUT 30)
add_test(NAME libdxfrw_mline_deleted_consumer_geometry_guard
    COMMAND libdxfrw_mline_source_dependency_tests --mline-deleted-consumer-geometry-guard)
set_tests_properties(libdxfrw_mline_deleted_consumer_geometry_guard PROPERTIES LABELS "dxf;fast;transaction" TIMEOUT 30)
add_test(NAME libdxfrw_mline_raw_section_geometry_guard
    COMMAND libdxfrw_mline_source_dependency_tests --mline-raw-section-geometry-guard)
set_tests_properties(libdxfrw_mline_raw_section_geometry_guard PROPERTIES LABELS "dxf;fast;transaction" TIMEOUT 30)

# The source and output are authored at runtime. This selected AC1027 profile
# proves consumed MLINE group-41 reprojection in ASCII/binary, plus atomic
# refusal and same-adapter retry controls; it adds no drawing fixture.
add_test(NAME libdxfrw_mline_consumed_offset_projection
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-offset-projection)
set_tests_properties(libdxfrw_mline_consumed_offset_projection PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 30)

# Generated at runtime from the selected AC1027 MLINE profile. This proves
# offset reprojection when retargeting group 340, exact redundant group-2 and
# reference output, fresh readback, refusal, rollback and same-adapter retry.
add_test(NAME libdxfrw_mline_retarget_offset_projection
    COMMAND libdxfrw_dxf_fixture_tests --mline-retarget-offset-projection)
set_tests_properties(libdxfrw_mline_retarget_offset_projection PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 30)

# Runtime-generated AC1027 retarget profile changes only the selected target's
# start/end angles. Verify endpoint miter and group-41 reprojection, exact output,
# refusal/rollback and same-adapter retry across source/output encodings.
add_test(NAME libdxfrw_mline_retarget_angle_projection
    COMMAND libdxfrw_dxf_fixture_tests --mline-retarget-angle-projection)
set_tests_properties(libdxfrw_mline_retarget_angle_projection PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 60)

# Runtime-generated AC1027 MLINESTYLE retarget with one appended element.
# Verify style ownership, group 73/74/75/41 output, preserved redundant names,
# fresh readback, rollback and retry across source/output encodings.
add_test(NAME libdxfrw_mline_retarget_append_projection
    COMMAND libdxfrw_dxf_fixture_tests --mline-retarget-append-projection)
set_tests_properties(libdxfrw_mline_retarget_append_projection PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 60)

# Runtime-generated AC1027 MLINE consumers qualify the documented closed and
# cap-suppression group-71 edits in ASCII/binary, with literal output, fresh
# readback, destination rollback and same-adapter retry. No fixture is stored.
add_test(NAME libdxfrw_mline_flag_edits
    COMMAND libdxfrw_dxf_fixture_tests --mline-flag-edits)
set_tests_properties(libdxfrw_mline_flag_edits PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 30)

# Runtime-generated AC1027 profile for appending one style element consumed by
# open, uncut MLINEs. It checks 73/74/41/75/42 output, bottom-offset rebasing,
# readback, refusal and same-adapter retry without adding fixture drawings.
add_test(NAME libdxfrw_mline_consumed_style_append
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-append)
set_tests_properties(libdxfrw_mline_consumed_style_append PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 30)

# Runtime-generated AC1027 profile for appending two tail elements to a style
# consumed by open, uncut MLINEs. It checks offset rebasing, full parameter
# arrays, output/readback and failure/retry behavior without committed fixtures.
add_test(NAME libdxfrw_mline_consumed_style_multi_append
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-multi-append)
set_tests_properties(libdxfrw_mline_consumed_style_multi_append PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 30)

# Runtime-generated AC1027 profile for appending three tail elements to a
# consumed two-element style. It checks projected parameters, output/readback,
# failure/retry, and rollback without fixture files.
add_test(NAME libdxfrw_mline_consumed_style_triple_append
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-triple-append)
set_tests_properties(libdxfrw_mline_consumed_style_triple_append PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 30)

# Runtime-generated AC1027 profile for appending four tail elements to a
# consumed two-element style. It verifies six-element output/readback, exact
# MLINE parameters, failure rollback and retry behavior.
add_test(NAME libdxfrw_mline_consumed_style_quad_append
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-quad-append)
set_tests_properties(libdxfrw_mline_consumed_style_quad_append PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 30)

# Runtime-generated AC1027 profile for appending five tail elements to a
# consumed two-element style. It verifies seven-element output/readback, exact
# provider and MLINE parameter fields, failure rollback and retry without
# committing fixture files.
add_test(NAME libdxfrw_mline_consumed_style_quint_append
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-quint-append)
set_tests_properties(libdxfrw_mline_consumed_style_quint_append PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 30)

# Runtime-generated AC1027 profile for appending six tail elements to a
# consumed two-element style. It verifies eight-element output/readback, exact
# provider and MLINE parameter fields, failure rollback and retry without
# committing fixture files.
add_test(NAME libdxfrw_mline_consumed_style_sext_append
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-sext-append)
set_tests_properties(libdxfrw_mline_consumed_style_sext_append PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 30)

# Runtime-generated AC1027 profile for appending seven tail elements to a
# consumed two-element style. It verifies nine-element output/readback, exact
# provider and MLINE parameter fields, failure rollback and retry without
# committing fixture files.
add_test(NAME libdxfrw_mline_consumed_style_sept_append
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-sept-append)
set_tests_properties(libdxfrw_mline_consumed_style_sept_append PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 30)

# Runtime-generated AC1027 profile for appending eight tail elements to a
# consumed two-element style. It verifies ten-element output/readback, exact
# provider and MLINE parameter fields, failure rollback and retry without
# committing fixture files.
add_test(NAME libdxfrw_mline_consumed_style_oct_append
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-oct-append)
set_tests_properties(libdxfrw_mline_consumed_style_oct_append PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 30)

# Generated AC1027 profiles exercise the generic consumed-style append range:
# representative interior counts, the 255-element cap across all source/output
# encodings, 256 refusal, provider/readback proof and failure/retry behavior.
add_test(NAME libdxfrw_mline_consumed_style_append_range
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-append-range)
set_tests_properties(libdxfrw_mline_consumed_style_append_range PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 180)

# Runtime-generated AC1027 source-count profile: append one tuple to a
# source-bound four-element style and verify all dependent MLINE arrays,
# output/readback, rollback and same-adapter retries.
add_test(NAME libdxfrw_mline_consumed_four_source_append
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-four-source-append)
set_tests_properties(libdxfrw_mline_consumed_four_source_append PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 60)

# Runtime-generated AC1027 three-element source profile; use the same exact
# output, provider, rollback and retry proof to check a second non-two source.
add_test(NAME libdxfrw_mline_consumed_three_source_append
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-three-source-append)
set_tests_properties(libdxfrw_mline_consumed_three_source_append PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 60)

# Runtime-generated AC1027 five-element source profile; append one tuple and
# verify the complete consumer parameter arrays, output/readback and retries.
add_test(NAME libdxfrw_mline_consumed_five_source_append
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-five-source-append)
set_tests_properties(libdxfrw_mline_consumed_five_source_append PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 60)

# Runtime-generated AC1027 six-element source profile; append one tuple and
# verify exact consumer reprojection, output/readback, rollback and retries.
add_test(NAME libdxfrw_mline_consumed_six_source_append
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-six-source-append)
set_tests_properties(libdxfrw_mline_consumed_six_source_append PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 60)

# Runtime-generated AC1027 seven-element source profile; append one tuple and
# verify exact consumer reprojection, output/readback, rollback and retries.
add_test(NAME libdxfrw_mline_consumed_seven_source_append
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-seven-source-append)
set_tests_properties(libdxfrw_mline_consumed_seven_source_append PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 60)

# Runtime-generated AC1027 eight-element source profile; append one tuple and
# verify exact consumer reprojection, output/readback, rollback and retries.
add_test(NAME libdxfrw_mline_consumed_eight_source_append
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-eight-source-append)
set_tests_properties(libdxfrw_mline_consumed_eight_source_append PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 60)

# Runtime-generated AC1027 nine-element source profile; append one tuple and
# verify exact consumer reprojection, output/readback, rollback and retries.
add_test(NAME libdxfrw_mline_consumed_nine_source_append
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-nine-source-append)
set_tests_properties(libdxfrw_mline_consumed_nine_source_append PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 60)

# Runtime-generated AC1027 ten-element source profile; append one tuple and
# verify exact consumer reprojection, output/readback, rollback and retries.
add_test(NAME libdxfrw_mline_consumed_ten_source_append
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-ten-source-append)
set_tests_properties(libdxfrw_mline_consumed_ten_source_append PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 60)

# Runtime-generated AC1027 eleven-element source profile; append one tuple and
# verify exact consumer reprojection, output/readback, rollback and retries.
add_test(NAME libdxfrw_mline_consumed_eleven_source_append
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-eleven-source-append)
set_tests_properties(libdxfrw_mline_consumed_eleven_source_append PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 60)

# Runtime-generated AC1027 twelve-element source profile; append one tuple and
# verify exact consumer reprojection, output/readback, rollback and retries.
add_test(NAME libdxfrw_mline_consumed_twelve_source_append
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-twelve-source-append)
set_tests_properties(libdxfrw_mline_consumed_twelve_source_append PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 60)

# Runtime-generated AC1027 selected source-count profiles exercise the shared
# append recipe across intermediate counts; the maximum-count boundary remains
# outside this positive qualification.
add_test(NAME libdxfrw_mline_consumed_selected_source_counts
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-selected-source-counts)
set_tests_properties(libdxfrw_mline_consumed_selected_source_counts PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 60)

# Run the next source-count batch independently so extending the range does not
# repeat the already-qualified 13..128 profiles on every implementation slice.
add_test(NAME libdxfrw_mline_consumed_source_count_append_129_173
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 129 173)
set_tests_properties(libdxfrw_mline_consumed_source_count_append_129_173 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 60)

# Runtime-generated AC1027 source counts 174 through 253 after bounded direct
# physical-group decoding; verify output/readback, rollback and same-adapter retry.
add_test(NAME libdxfrw_mline_consumed_source_count_append_174_253
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 174 253)
set_tests_properties(libdxfrw_mline_consumed_source_count_append_174_253 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)

# Highest legal one-element append: source count 254 to target count 255.
add_test(NAME libdxfrw_mline_consumed_source_count_append_254_255
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 254 254)
set_tests_properties(libdxfrw_mline_consumed_source_count_append_254_255 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 60)

# Runtime-generated AC1027 non-two-element sources exercise a two-tuple
# append with exact consumer reprojection and rollback/retry controls.
add_test(NAME libdxfrw_mline_consumed_source_count_multi_append_non_two
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 3 4 2)
set_tests_properties(libdxfrw_mline_consumed_source_count_multi_append_non_two PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 60)

# Extend the same two-tuple batch across generated non-two source counts 5..253.
# Split the loop to keep each focused transaction test below its own timeout.
add_test(NAME libdxfrw_mline_consumed_source_count_multi_append_5_127
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 5 127 2)
set_tests_properties(libdxfrw_mline_consumed_source_count_multi_append_5_127 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_source_count_multi_append_128_253
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 128 253 2)
set_tests_properties(libdxfrw_mline_consumed_source_count_multi_append_128_253 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)

# Qualify the selected three-tuple recipe across all non-two source counts
# whose target count remains representable in the one-byte MLINESTYLE field.
add_test(NAME libdxfrw_mline_consumed_source_count_triple_append_3_127
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 3 127 3)
set_tests_properties(libdxfrw_mline_consumed_source_count_triple_append_3_127 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_source_count_triple_append_128_190
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 128 190 3)
set_tests_properties(libdxfrw_mline_consumed_source_count_triple_append_128_190 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_source_count_triple_append_191_252
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 191 252 3)
set_tests_properties(libdxfrw_mline_consumed_source_count_triple_append_191_252 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)

# Qualify a selected four-tuple batch across all non-two source counts whose
# resulting MLINESTYLE count fits its one-byte field; split for bounded runtime.
add_test(NAME libdxfrw_mline_consumed_source_count_quad_append_3_127
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 3 127 4)
set_tests_properties(libdxfrw_mline_consumed_source_count_quad_append_3_127 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_source_count_quad_append_128_251
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 128 251 4)
set_tests_properties(libdxfrw_mline_consumed_source_count_quad_append_128_251 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)

# Qualify the selected five-tuple recipe over all non-two source counts whose
# resulting MLINESTYLE count fits its one-byte field; split the focused matrix.
add_test(NAME libdxfrw_mline_consumed_source_count_quint_append_3_127
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 3 127 5)
set_tests_properties(libdxfrw_mline_consumed_source_count_quint_append_3_127 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)
add_test(NAME libdxfrw_mline_consumed_source_count_quint_append_128_250
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 128 250 5)
set_tests_properties(libdxfrw_mline_consumed_source_count_quint_append_128_250 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)

# Qualify the selected six-tuple recipe across the count-valid non-two sources.
add_test(NAME libdxfrw_mline_consumed_source_count_six_append_3_127
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 3 127 6)
set_tests_properties(libdxfrw_mline_consumed_source_count_six_append_3_127 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)
add_test(NAME libdxfrw_mline_consumed_source_count_six_append_128_249
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 128 249 6)
set_tests_properties(libdxfrw_mline_consumed_source_count_six_append_128_249 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)

# Qualify the selected seven-tuple recipe over all count-valid non-two sources.
add_test(NAME libdxfrw_mline_consumed_source_count_seven_append_3_127
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 3 127 7)
set_tests_properties(libdxfrw_mline_consumed_source_count_seven_append_3_127 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)
add_test(NAME libdxfrw_mline_consumed_source_count_seven_append_128_248
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 128 248 7)
set_tests_properties(libdxfrw_mline_consumed_source_count_seven_append_128_248 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)

# Qualify the selected eight-tuple recipe over all count-valid non-two sources.
add_test(NAME libdxfrw_mline_consumed_source_count_eight_append_3_127
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 3 127 8)
set_tests_properties(libdxfrw_mline_consumed_source_count_eight_append_3_127 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)
add_test(NAME libdxfrw_mline_consumed_source_count_eight_append_128_247
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 128 247 8)
set_tests_properties(libdxfrw_mline_consumed_source_count_eight_append_128_247 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)

# Qualify the selected nine-tuple recipe over all count-valid non-two sources.
add_test(NAME libdxfrw_mline_consumed_source_count_nine_append_3_127
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 3 127 9)
set_tests_properties(libdxfrw_mline_consumed_source_count_nine_append_3_127 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)
add_test(NAME libdxfrw_mline_consumed_source_count_nine_append_128_246
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 128 246 9)
set_tests_properties(libdxfrw_mline_consumed_source_count_nine_append_128_246 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)

# Qualify the selected ten-tuple recipe over all count-valid non-two sources.
add_test(NAME libdxfrw_mline_consumed_source_count_ten_append_3_127
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 3 127 10)
set_tests_properties(libdxfrw_mline_consumed_source_count_ten_append_3_127 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)
add_test(NAME libdxfrw_mline_consumed_source_count_ten_append_128_245
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 128 245 10)
set_tests_properties(libdxfrw_mline_consumed_source_count_ten_append_128_245 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)

# Qualify the selected eleven-tuple recipe over all count-valid non-two sources.
add_test(NAME libdxfrw_mline_consumed_source_count_eleven_append_3_127
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 3 127 11)
set_tests_properties(libdxfrw_mline_consumed_source_count_eleven_append_3_127 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)
add_test(NAME libdxfrw_mline_consumed_source_count_eleven_append_128_244
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 128 244 11)
set_tests_properties(libdxfrw_mline_consumed_source_count_eleven_append_128_244 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)

# Qualify the selected twelve-tuple recipe over all count-valid non-two sources.
add_test(NAME libdxfrw_mline_consumed_source_count_twelve_append_3_127
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 3 127 12)
set_tests_properties(libdxfrw_mline_consumed_source_count_twelve_append_3_127 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)
add_test(NAME libdxfrw_mline_consumed_source_count_twelve_append_128_243
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 128 243 12)
set_tests_properties(libdxfrw_mline_consumed_source_count_twelve_append_128_243 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)

# Qualify the selected thirteen-tuple recipe over all count-valid non-two sources.
add_test(NAME libdxfrw_mline_consumed_source_count_thirteen_append_3_127
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 3 127 13)
set_tests_properties(libdxfrw_mline_consumed_source_count_thirteen_append_3_127 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)
add_test(NAME libdxfrw_mline_consumed_source_count_thirteen_append_128_242
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-source-count-append-range 128 242 13)
set_tests_properties(libdxfrw_mline_consumed_source_count_thirteen_append_128_242 PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)

# Runtime-generated AC1027 profile for removing either element from a two-item
# style used by open, uncut MLINEs. It checks parameter remapping, exact output,
# zero-count refusal, rollback and same-adapter retry.
add_test(NAME libdxfrw_mline_consumed_style_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-removal)
set_tests_properties(libdxfrw_mline_consumed_style_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 30)

# Runtime-generated AC1027 profile for removing two middle elements from a
# four-element style used by open, uncut MLINEs. It checks ordered survivor
# mapping, exact fields/readback, failure rollback, a narrow count boundary and
# same-adapter retry without committing fixture drawings.
add_test(NAME libdxfrw_mline_consumed_style_multi_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-multi-removal)
set_tests_properties(libdxfrw_mline_consumed_style_multi_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 30)

# Runtime-generated AC1027 profiles remove each possible element in turn from
# a four-element style consumed by open, uncut MLINEs. The test checks unique
# survivor maps, emitted fields/readback, rollback and retry in both DXF forms.
add_test(NAME libdxfrw_mline_consumed_style_three_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-three-removal)
set_tests_properties(libdxfrw_mline_consumed_style_three_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 30)

# Runtime-generated AC1027 profiles retain each possible element in turn from
# a four-element style consumed by open, uncut MLINEs. It checks exact survivor
# mapping/readback, caller/destination rollback and same-adapter retry.
add_test(NAME libdxfrw_mline_consumed_style_single_from_four_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-single-from-four-removal)
set_tests_properties(libdxfrw_mline_consumed_style_single_from_four_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 30)

# Runtime-generated AC1027 profile retains each possible tuple from a
# five-element consumed style. Verify all survivor positions, consumer
# reprojection, exact readback, refusal rollback and retry.
add_test(NAME libdxfrw_mline_consumed_style_single_from_five_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-single-from-five-removal)
set_tests_properties(libdxfrw_mline_consumed_style_single_from_five_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 60)

# Runtime-generated AC1027 profiles retain every ordered two-tuple subset
# from a five-element consumed style; verify projection and transaction guards.
add_test(NAME libdxfrw_mline_consumed_style_two_from_five_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-two-from-five-removal)
set_tests_properties(libdxfrw_mline_consumed_style_two_from_five_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 60)

# Continue the selected five-element source through the remaining nonzero
# target counts; every ordered survivor subset is checked by the shared harness.
add_test(NAME libdxfrw_mline_consumed_style_three_from_five_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-three-from-five-removal)
set_tests_properties(libdxfrw_mline_consumed_style_three_from_five_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 60)
add_test(NAME libdxfrw_mline_consumed_style_four_from_five_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-four-from-five-removal)
set_tests_properties(libdxfrw_mline_consumed_style_four_from_five_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 60)

# Runtime-generated full ordered-survivor matrices for six-, seven-, and
# eight-element AC1027 source styles, with a separate bounded test per source.
add_test(NAME libdxfrw_mline_consumed_style_removal_matrix_six
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-removal-matrix-six)
set_tests_properties(libdxfrw_mline_consumed_style_removal_matrix_six PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_removal_matrix_seven
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-removal-matrix-seven)
set_tests_properties(libdxfrw_mline_consumed_style_removal_matrix_seven PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_removal_matrix_eight
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-removal-matrix-eight)
set_tests_properties(libdxfrw_mline_consumed_style_removal_matrix_eight PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)

# Extend single-survivor removal to a nine-element style, covering all nine
# survivor positions through the established exact-output and retry inspector.
add_test(NAME libdxfrw_mline_consumed_style_single_from_nine_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-single-from-nine-removal)
set_tests_properties(libdxfrw_mline_consumed_style_single_from_nine_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_two_from_nine_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-two-from-nine-removal)
set_tests_properties(libdxfrw_mline_consumed_style_two_from_nine_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_three_from_nine_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-three-from-nine-removal)
set_tests_properties(libdxfrw_mline_consumed_style_three_from_nine_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_four_from_nine_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-four-from-nine-removal)
set_tests_properties(libdxfrw_mline_consumed_style_four_from_nine_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_five_from_nine_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-five-from-nine-removal)
set_tests_properties(libdxfrw_mline_consumed_style_five_from_nine_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_six_from_nine_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-six-from-nine-removal)
set_tests_properties(libdxfrw_mline_consumed_style_six_from_nine_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_seven_from_nine_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-seven-from-nine-removal)
set_tests_properties(libdxfrw_mline_consumed_style_seven_from_nine_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_eight_from_nine_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-eight-from-nine-removal)
set_tests_properties(libdxfrw_mline_consumed_style_eight_from_nine_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_single_from_ten_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-single-from-ten-removal)
set_tests_properties(libdxfrw_mline_consumed_style_single_from_ten_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_two_from_ten_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-two-from-ten-removal)
set_tests_properties(libdxfrw_mline_consumed_style_two_from_ten_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_three_from_ten_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-three-from-ten-removal)
set_tests_properties(libdxfrw_mline_consumed_style_three_from_ten_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_four_from_ten_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-four-from-ten-removal)
set_tests_properties(libdxfrw_mline_consumed_style_four_from_ten_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_five_from_ten_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-five-from-ten-removal)
set_tests_properties(libdxfrw_mline_consumed_style_five_from_ten_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_six_from_ten_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-six-from-ten-removal)
set_tests_properties(libdxfrw_mline_consumed_style_six_from_ten_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_seven_from_ten_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-seven-from-ten-removal)
set_tests_properties(libdxfrw_mline_consumed_style_seven_from_ten_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_eight_from_ten_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-eight-from-ten-removal)
set_tests_properties(libdxfrw_mline_consumed_style_eight_from_ten_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_nine_from_ten_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-nine-from-ten-removal)
set_tests_properties(libdxfrw_mline_consumed_style_nine_from_ten_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_one_from_eleven_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-one-from-eleven-removal)
set_tests_properties(libdxfrw_mline_consumed_style_one_from_eleven_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_two_from_eleven_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-two-from-eleven-removal)
set_tests_properties(libdxfrw_mline_consumed_style_two_from_eleven_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_three_from_eleven_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-three-from-eleven-removal)
set_tests_properties(libdxfrw_mline_consumed_style_three_from_eleven_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_four_from_eleven_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-four-from-eleven-removal)
set_tests_properties(libdxfrw_mline_consumed_style_four_from_eleven_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_five_from_eleven_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-five-from-eleven-removal)
set_tests_properties(libdxfrw_mline_consumed_style_five_from_eleven_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_six_from_eleven_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-six-from-eleven-removal)
set_tests_properties(libdxfrw_mline_consumed_style_six_from_eleven_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_seven_from_eleven_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-seven-from-eleven-removal)
set_tests_properties(libdxfrw_mline_consumed_style_seven_from_eleven_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)

# Runtime-generated AC1027 profiles remove each element in turn from a
# three-element style consumed by open, uncut MLINEs. Check parameter
# reprojection, exact output/readback, rollback and retries across DXF encodings.
add_test(NAME libdxfrw_mline_consumed_three_to_two_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-three-to-two-removal)
set_tests_properties(libdxfrw_mline_consumed_three_to_two_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 60)

# Runtime-generated AC1027 profile retains each possible element from a
# three-element consumed style. Verify exact single-survivor parameter mapping,
# output/readback, rollback and same-adapter retries.
add_test(NAME libdxfrw_mline_consumed_three_to_one_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-three-to-one-removal)
set_tests_properties(libdxfrw_mline_consumed_three_to_one_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 60)
add_executable(libdxfrw_whole_source_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_whole_source_tests.cpp
    ${_libdxfrw_test_adapter_source}
)
target_compile_features(libdxfrw_whole_source_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_whole_source_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_whole_source_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_whole_source_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_whole_source_capture COMMAND libdxfrw_whole_source_tests)
set_tests_properties(libdxfrw_whole_source_capture PROPERTIES LABELS "dxf;fast")
add_executable(libdxfrw_dwg2dxf_adapter_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf_adapter_tests.cpp
    ${_libdxfrw_test_adapter_source}
)
target_compile_features(libdxfrw_dwg2dxf_adapter_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg2dxf_adapter_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg2dxf_adapter_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(libdxfrw_dwg2dxf_adapter_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_sortents_adapter
    COMMAND libdxfrw_dwg2dxf_adapter_tests --sortents-adapter
        "${CMAKE_CURRENT_BINARY_DIR}/sortents-adapter")
set_tests_properties(libdxfrw_sortents_adapter PROPERTIES
    LABELS "fast;dxf;graph;adapter" TIMEOUT 30)
add_test(NAME libdxfrw_group_adapter_roundtrip
    COMMAND libdxfrw_dwg2dxf_adapter_tests --group-adapter-roundtrip)
set_tests_properties(libdxfrw_group_adapter_roundtrip PROPERTIES
    LABELS "fast;dxf;graph;adapter" TIMEOUT 30)
add_test(NAME libdxfrw_field_graph_export_refusal
    COMMAND libdxfrw_dwg2dxf_adapter_tests --field-graph-export-refusal)
set_tests_properties(libdxfrw_field_graph_export_refusal PROPERTIES
    LABELS "fast;dxf;graph;adapter;field" TIMEOUT 30)
if(TARGET dwg2dxf)
    add_test(NAME libdxfrw_extension_graph_limits
        COMMAND ${CMAKE_COMMAND}
            "-DADAPTER_TEST=$<TARGET_FILE:libdxfrw_dwg2dxf_adapter_tests>"
            "-DDWG2DXF=$<TARGET_FILE:dwg2dxf>"
            "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_extension_graph_limits_test.cmake)
    set_tests_properties(libdxfrw_extension_graph_limits PROPERTIES
        LABELS "fast;dxf;graph;adapter;cli" TIMEOUT 30)
endif()
add_test(NAME libdxfrw_table_callback_owner_order
    COMMAND libdxfrw_dwg2dxf_adapter_tests --retain-table-callback)
set_tests_properties(libdxfrw_table_callback_owner_order PROPERTIES
    LABELS "fast;dxf;graph;adapter;tables" TIMEOUT 30)
add_test(NAME libdxfrw_tablestyle_nested_raw_net
    COMMAND libdxfrw_dwg2dxf_adapter_tests --tablestyle-nested-raw-net)
set_tests_properties(libdxfrw_tablestyle_nested_raw_net PROPERTIES
    LABELS "fast;dxf;tablestyle;generated;raw-net" TIMEOUT 30)
add_test(NAME libdxfrw_visual_style_import_evidence
    COMMAND libdxfrw_dwg2dxf_adapter_tests --visual-style-import-evidence
        ${CMAKE_CURRENT_BINARY_DIR})
set_tests_properties(libdxfrw_visual_style_import_evidence PROPERTIES
    LABELS "fast;dxf;graph;adapter;visualstyle" TIMEOUT 30)
add_test(NAME dwg2dxf_visual_style_raw_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests --visual-style-raw-capture-limits)
set_tests_properties(dwg2dxf_visual_style_raw_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;visualstyle" TIMEOUT 10)
add_test(NAME dwg2dxf_dictionary_default_raw_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --dictionary-default-raw-capture-limits)
set_tests_properties(dwg2dxf_dictionary_default_raw_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;objects" TIMEOUT 10)
add_test(NAME dwg2dxf_dictionary_typed_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --dictionary-typed-capture-limits)
set_tests_properties(dwg2dxf_dictionary_typed_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;objects" TIMEOUT 10)
add_test(NAME dwg2dxf_dxf_class_typed_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --dxf-class-typed-capture-limits)
set_tests_properties(dwg2dxf_dxf_class_typed_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;objects" TIMEOUT 10)
add_test(NAME dwg2dxf_basic_table_typed_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --basic-table-typed-capture-limits)
set_tests_properties(dwg2dxf_basic_table_typed_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;tables" TIMEOUT 10)
add_test(NAME dwg2dxf_typed_entity_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests --typed-entity-capture-limits)
set_tests_properties(dwg2dxf_typed_entity_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;entities" TIMEOUT 10)
add_test(NAME dwg2dxf_dictionary_default_typed_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --dictionary-default-typed-capture-limits)
set_tests_properties(dwg2dxf_dictionary_default_typed_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;objects" TIMEOUT 10)
add_test(NAME dwg2dxf_render_background_sun_raw_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --render-background-sun-raw-capture-limits)
set_tests_properties(dwg2dxf_render_background_sun_raw_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;objects" TIMEOUT 10)
add_test(NAME dwg2dxf_field_carrier_raw_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --field-carrier-raw-capture-limits)
set_tests_properties(dwg2dxf_field_carrier_raw_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;objects" TIMEOUT 10)
add_test(NAME dwg2dxf_sort_table_style_raw_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --sort-table-style-raw-capture-limits)
set_tests_properties(dwg2dxf_sort_table_style_raw_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;objects" TIMEOUT 10)
add_test(NAME dwg2dxf_render_settings_typed_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --render-settings-typed-capture-limits)
set_tests_properties(dwg2dxf_render_settings_typed_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;objects;render" TIMEOUT 10)
add_test(NAME dwg2dxf_sortents_typed_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --sortents-typed-capture-limits)
set_tests_properties(dwg2dxf_sortents_typed_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;graph;objects" TIMEOUT 10)
add_test(NAME dwg2dxf_table_style_typed_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --table-style-typed-capture-limits)
set_tests_properties(dwg2dxf_table_style_typed_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;objects" TIMEOUT 10)
add_test(NAME dwg2dxf_cell_style_map_typed_capture_fail_closed
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --cell-style-map-typed-capture-fail-closed)
set_tests_properties(dwg2dxf_cell_style_map_typed_capture_fail_closed PROPERTIES
    LABELS "fast;dxf;adapter;objects" TIMEOUT 10)
add_test(NAME dwg2dxf_table_content_style_reference_census
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --table-content-style-reference-census)
set_tests_properties(dwg2dxf_table_content_style_reference_census PROPERTIES
    LABELS "fast;dxf;adapter;graph;table;cellstylemap;generated" TIMEOUT 10)
add_test(NAME dwg2dxf_table_base_style_id_dxf_refusal
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --table-base-style-id-dxf-refusal)
set_tests_properties(dwg2dxf_table_base_style_id_dxf_refusal PROPERTIES
    LABELS "fast;dxf;adapter;table;generated;refusal" TIMEOUT 10)
add_test(NAME libdxfrw_cellstylemap_dxf_writer
    COMMAND libdxfrw_dwg2dxf_adapter_tests --cell-style-map-dxf-writer)
set_tests_properties(libdxfrw_cellstylemap_dxf_writer PROPERTIES
    LABELS "fast;dxf;adapter;objects;cellstylemap;generated" TIMEOUT 20)
add_test(NAME dwg2dxf_modeldoc_view_style_typed_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --modeldoc-view-style-typed-capture-limits)
set_tests_properties(dwg2dxf_modeldoc_view_style_typed_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;objects" TIMEOUT 10)
add_test(NAME dwg2dxf_field_carrier_typed_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --field-carrier-typed-capture-limits)
set_tests_properties(dwg2dxf_field_carrier_typed_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;objects" TIMEOUT 10)
add_test(NAME dwg2dxf_datalink_typed_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --datalink-typed-capture-limits)
set_tests_properties(dwg2dxf_datalink_typed_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;objects" TIMEOUT 10)
add_test(NAME dwg2dxf_xrecord_typed_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --xrecord-typed-capture-limits)
set_tests_properties(dwg2dxf_xrecord_typed_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;objects" TIMEOUT 10)
add_test(NAME dwg2dxf_material_typed_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --material-typed-capture-limits)
set_tests_properties(dwg2dxf_material_typed_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;objects" TIMEOUT 10)
add_test(NAME dwg2dxf_background_sun_typed_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --background-sun-typed-capture-limits)
set_tests_properties(dwg2dxf_background_sun_typed_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;objects" TIMEOUT 10)
add_test(NAME dwg2dxf_visual_style_typed_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --visual-style-typed-capture-limits)
set_tests_properties(dwg2dxf_visual_style_typed_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;objects" TIMEOUT 10)
add_test(NAME dwg2dxf_mline_style_typed_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --mline-style-typed-capture-limits)
set_tests_properties(dwg2dxf_mline_style_typed_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;objects" TIMEOUT 10)
add_test(NAME dwg2dxf_named_resource_typed_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --named-resource-typed-capture-limits)
set_tests_properties(dwg2dxf_named_resource_typed_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;objects" TIMEOUT 10)
add_test(NAME dwg2dxf_named_resource_raw_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --named-resource-raw-capture-limits)
set_tests_properties(dwg2dxf_named_resource_raw_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;objects" TIMEOUT 10)
add_test(NAME libdxfrw_dimension_picture_block_full_conversion
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --dimension-picture-block-full-conversion)
set_tests_properties(libdxfrw_dimension_picture_block_full_conversion
    PROPERTIES LABELS "fast;dxf;graph;adapter" TIMEOUT 30)
add_test(NAME libdxfrw_named_resource_import_evidence
    COMMAND libdxfrw_dwg2dxf_adapter_tests --named-resource-import-evidence
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/tests/fixtures/dwg/rtext_arctext.dwg)
set_tests_properties(libdxfrw_named_resource_import_evidence PROPERTIES
    LABELS "fast;dxf;graph;adapter;objects" TIMEOUT 30)
add_test(NAME libdxfrw_mleaderstyle_writer_closure
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --mleaderstyle-writer-closure ${CMAKE_CURRENT_BINARY_DIR})
set_tests_properties(libdxfrw_mleaderstyle_writer_closure PROPERTIES
    LABELS "fast;dxf;graph;adapter;objects;mleader" TIMEOUT 30)
add_test(NAME libdxfrw_dictionary_default_adapter_roundtrip
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --dictionary-default-adapter-roundtrip)
set_tests_properties(libdxfrw_dictionary_default_adapter_roundtrip PROPERTIES
    LABELS "fast;dxf;graph;adapter;objects" TIMEOUT 30)
add_executable(libdxfrw_block_owner_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/block_owner_tests.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_block_owner_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_block_owner_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_block_owner_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_block_owner_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_block_owners COMMAND libdxfrw_block_owner_tests)
set_tests_properties(libdxfrw_block_owners PROPERTIES LABELS "fast;dxf;graph;adapter" TIMEOUT 30)
if(TARGET dwg2dxf)
    add_test(NAME dwg2dxf_block_owner_rejection_cli
        COMMAND ${CMAKE_COMMAND}
            "-DADAPTER_TEST=$<TARGET_FILE:libdxfrw_block_owner_tests>"
            "-DNOD_TEST=$<TARGET_FILE:libdxfrw_dxf_fixture_tests>"
            "-DDWG2DXF=$<TARGET_FILE:dwg2dxf>"
            "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_block_owner_rejection_test.cmake)
    set_tests_properties(dwg2dxf_block_owner_rejection_cli PROPERTIES
        LABELS "fast;dxf;graph;cli" TIMEOUT 30)
    foreach(_owner_mode IN ITEMS ascii binary)
        set(_owner_generate --generate)
        set(_owner_options "")
        if(_owner_mode STREQUAL "binary")
            set(_owner_generate --generate-binary)
            set(_owner_options -b)
        endif()
        add_test(NAME dwg2dxf_block_owners_${_owner_mode}_cli
            COMMAND ${CMAKE_COMMAND}
                "-DADAPTER_TEST=$<TARGET_FILE:libdxfrw_block_owner_tests>"
                "-DDWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
                "-DCASE_NAME=block-owners-${_owner_mode}"
                "-DGENERATE_MODE=${_owner_generate}"
                "-DVERIFY_MODE=--verify"
                "-DCONVERSION_OPTIONS=${_owner_options}"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg2dxf_image_path_test.cmake)
        set_tests_properties(dwg2dxf_block_owners_${_owner_mode}_cli PROPERTIES
            LABELS "fast;dxf;graph;cli" TIMEOUT 30)
    endforeach()
    add_test(
        NAME dwg2dxf_image_path_cli
        COMMAND ${CMAKE_COMMAND}
            "-DADAPTER_TEST=$<TARGET_FILE:libdxfrw_dwg2dxf_adapter_tests>"
            "-DDWG2DXF=$<TARGET_FILE:dwg2dxf>"
            "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
            "-DBINARY_GENERATE_MODE=--generate-binary"
            "-DBINARY_VERIFY_MODE=--verify-binary"
            "-DBINARY_CONVERSION_OPTIONS=-b"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg2dxf_image_path_test.cmake
    )
    set_tests_properties(dwg2dxf_image_path_cli PROPERTIES
        LABELS "cli;fast;dxf;adapter"
        TIMEOUT 30
    )

    # HEADER group 90 is 32-bit in AC1032. Generate the literal source without
    # DRW_Header, then exercise two real converter invocations and public readback.
    foreach(_header_mode IN ITEMS ascii binary)
        set(_header_generate_mode --generate-maintenance-header)
        set(_header_conversion_options "")
        if(_header_mode STREQUAL "binary")
            set(_header_generate_mode --generate-maintenance-header-binary)
            set(_header_conversion_options -b)
        endif()
        add_test(
            NAME dwg2dxf_header_maintenance_${_header_mode}_cli
            COMMAND ${CMAKE_COMMAND}
                "-DADAPTER_TEST=$<TARGET_FILE:libdxfrw_dwg2dxf_adapter_tests>"
                "-DDWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
                "-DCASE_NAME=header-maintenance-${_header_mode}"
                "-DGENERATE_MODE=${_header_generate_mode}"
                "-DVERIFY_MODE=--verify-maintenance-header"
                "-DNEGATIVE_MODE=--reject-maintenance-downgrade"
                "-DCONVERSION_OPTIONS=${_header_conversion_options}"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg2dxf_image_path_test.cmake
        )
        set_tests_properties(dwg2dxf_header_maintenance_${_header_mode}_cli PROPERTIES
            LABELS "cli;fast;dxf;header;adapter"
            TIMEOUT 30
        )
    endforeach()

    # Source-generated model/paper ellipses downgrade to R12 through two actual
    # CLI passes; ordinary POLYLINE controls distinguish derivation from policy.
    foreach(_ellipse_mode IN ITEMS ascii binary)
        set(_ellipse_generate --generate-ellipse-carriers)
        set(_ellipse_options -R12)
        if(_ellipse_mode STREQUAL "binary")
            set(_ellipse_generate --generate-ellipse-carriers-binary)
            list(APPEND _ellipse_options -b)
        endif()
        add_test(NAME dwg2dxf_r12_ellipse_carriers_${_ellipse_mode}_cli
            COMMAND ${CMAKE_COMMAND}
                "-DADAPTER_TEST=$<TARGET_FILE:libdxfrw_dxf_fixture_tests>"
                "-DDWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
                "-DCASE_NAME=r12-ellipse-carriers-${_ellipse_mode}"
                "-DGENERATE_MODE=${_ellipse_generate}"
                "-DVERIFY_MODE=--verify-ellipse-carriers"
                "-DCONVERSION_OPTIONS=${_ellipse_options}"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg2dxf_image_path_test.cmake)
        set_tests_properties(dwg2dxf_r12_ellipse_carriers_${_ellipse_mode}_cli PROPERTIES
            LABELS "fast;dxf;cli;ellipse" TIMEOUT 30)
    endforeach()
endif()

# The converter transfers its completed AcDs section into its owned document
# model. Existing consumers retain the default const-reference callback.
add_test(
    NAME dwg2dxf_datastorage_move_callback
    COMMAND libdxfrw_dwg2dxf_adapter_tests --data-storage-move-callback
)
set_tests_properties(dwg2dxf_datastorage_move_callback PROPERTIES
    LABELS "fast;dwg;adapter;datastorage"
    TIMEOUT 10
)

add_test(
    NAME dwg2dxf_datastorage_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests --data-storage-capture-limits
)
set_tests_properties(dwg2dxf_datastorage_capture_limits PROPERTIES
    LABELS "fast;dwg;adapter;datastorage"
    TIMEOUT 10
)

add_test(
    NAME dwg2dxf_modeler_geometry_move_callback
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --modeler-geometry-move-callback-fallback
)
set_tests_properties(dwg2dxf_modeler_geometry_move_callback PROPERTIES
    LABELS "fast;dxf;adapter;modeler"
    TIMEOUT 10
)

add_test(
    NAME dwg2dxf_modeler_dxf_move_dispatch
    COMMAND libdxfrw_dwg2dxf_adapter_tests --dxf-modeler-move-dispatch
)
set_tests_properties(dwg2dxf_modeler_dxf_move_dispatch PROPERTIES
    LABELS "fast;dxf;adapter;modeler"
    TIMEOUT 10
)

# Dynamic-block objects are retained by the converter adapter and must block
# publication until a semantics-qualified DXF writer exists.
add_test(
    NAME dwg2dxf_dynamic_block_fail_closed
    COMMAND libdxfrw_dwg2dxf_adapter_tests --reject-dynamic-block
)
set_tests_properties(dwg2dxf_dynamic_block_fail_closed PROPERTIES
    LABELS "fast;dxf;adapter;dynamic-block"
    TIMEOUT 10
)

# File-format dispatch folds only ASCII extension letters; non-ASCII suffix
# bytes must be preserved and rejected without locale-dependent ctype calls.
add_test(
    NAME dwg2dxf_file_extension_case
    COMMAND libdxfrw_dwg2dxf_adapter_tests --file-extension-case
)
set_tests_properties(dwg2dxf_file_extension_case PROPERTIES
    LABELS "fast;dxf;adapter;paths"
    TIMEOUT 10
)

add_test(
    NAME dwg2dxf_mleader_fail_closed
    COMMAND libdxfrw_dwg2dxf_adapter_tests --reject-mleader-without-serializer
)
set_tests_properties(dwg2dxf_mleader_fail_closed PROPERTIES
    LABELS "fast;dxf;adapter;mleader"
    TIMEOUT 10
)

# The concrete converter adapter must own every decoded named-object callback;
# the in-memory WDFLT control also guards copied parse-validity evidence.
add_test(
    NAME dwg2dxf_named_object_callback_storage
    COMMAND libdxfrw_dwg2dxf_adapter_tests --retain-named-object-callbacks
)
set_tests_properties(dwg2dxf_named_object_callback_storage PROPERTIES
    LABELS "fast;dxf;adapter;objects"
    TIMEOUT 10
)

add_executable(libdxfrw_table_geometry_dxf_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/table_geometry_dxf_tests.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_table_geometry_dxf_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_table_geometry_dxf_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_table_geometry_dxf_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_table_geometry_dxf_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_table_geometry_dxf_projection COMMAND libdxfrw_table_geometry_dxf_tests)
set_tests_properties(libdxfrw_table_geometry_dxf_projection PROPERTIES
    LABELS "fast;dxf;adapter;table;generated" TIMEOUT 30)
add_test(NAME libdxfrw_table_geometry_observer_selftest
    COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -I -B
        ${CMAKE_CURRENT_SOURCE_DIR}/table_geometry_source_observer.py --self-test)
set_tests_properties(libdxfrw_table_geometry_observer_selftest PROPERTIES
    LABELS "fast;dxf;table;independent;selftest" TIMEOUT 10)

add_executable(libdxfrw_attdef_source_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/attdef_source_tests.cpp ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_attdef_source_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_attdef_source_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_attdef_source_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_attdef_source_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_attdef_source_transport COMMAND libdxfrw_attdef_source_tests)
set_tests_properties(libdxfrw_attdef_source_transport PROPERTIES
    LABELS "fast;dxf;attdef;source;generated" TIMEOUT 30)
add_test(NAME libdxfrw_nested_mtext_common_owner_budget
    COMMAND libdxfrw_attdef_source_tests --nested-common-budget)
set_tests_properties(libdxfrw_nested_mtext_common_owner_budget PROPERTIES
    LABELS "fast;dxf;attdef;mtext;resource-budget;generated" TIMEOUT 30)
add_test(NAME libdxfrw_attdef_immutable_capture
    COMMAND libdxfrw_attdef_source_tests --immutable-capture)
set_tests_properties(libdxfrw_attdef_immutable_capture PROPERTIES
    LABELS "fast;dxf;attdef;source;resource-budget;generated" TIMEOUT 60)
add_test(NAME libdxfrw_attdef_source_profile
    COMMAND libdxfrw_attdef_source_tests --source-profile)
add_test(NAME libdxfrw_attdef_legacy_observation
    COMMAND libdxfrw_attdef_source_tests --legacy-observation)
set_tests_properties(libdxfrw_attdef_legacy_observation PROPERTIES LABELS "dxf;adapter;fast")
add_test(NAME libdxfrw_attdef_legacy_adapter_capture
    COMMAND libdxfrw_attdef_source_tests --legacy-adapter-capture)
set_tests_properties(libdxfrw_attdef_legacy_adapter_capture PROPERTIES LABELS "dxf;adapter;fast")
add_test(NAME libdxfrw_block_legacy_capture
    COMMAND libdxfrw_attdef_source_tests --legacy-block-capture)
set_tests_properties(libdxfrw_block_legacy_capture PROPERTIES LABELS "dxf;adapter;fast")
add_test(NAME libdxfrw_legacy_attdef_scope_join COMMAND libdxfrw_attdef_source_tests --legacy-attdef-scope)
set_tests_properties(libdxfrw_legacy_attdef_scope_join PROPERTIES LABELS "dxf;adapter;source;identity;fast;generated")
add_test(NAME libdxfrw_block_record_legacy_observation
    COMMAND libdxfrw_attdef_source_tests --legacy-block-record-observation)
set_tests_properties(libdxfrw_block_record_legacy_observation PROPERTIES LABELS "dxf;adapter;fast")
add_test(NAME libdxfrw_block_record_legacy_adapter
    COMMAND libdxfrw_attdef_source_tests --legacy-block-record-adapter)
set_tests_properties(libdxfrw_block_record_legacy_adapter PROPERTIES LABELS "dxf;adapter;fast")
set_tests_properties(libdxfrw_attdef_source_profile PROPERTIES
    LABELS "fast;dxf;attdef;source;independent;generated" TIMEOUT 60)
add_test(NAME libdxfrw_attdef_embedded_equal_alignment
    COMMAND libdxfrw_attdef_source_tests --equal-alignment)
set_tests_properties(libdxfrw_attdef_embedded_equal_alignment PROPERTIES
    LABELS "fast;dxf;attdef;attrib;source;independent;generated" TIMEOUT 60)
add_test(NAME libdxfrw_attdef_embedded_dual280
    COMMAND libdxfrw_attdef_source_tests --embedded-dual280)
set_tests_properties(libdxfrw_attdef_embedded_dual280 PROPERTIES
    LABELS "fast;dxf;attdef;attrib;source;independent;generated" TIMEOUT 60)
add_test(NAME libdxfrw_block_source_capture
    COMMAND libdxfrw_attdef_source_tests --block-source)
set_tests_properties(libdxfrw_block_source_capture PROPERTIES
    LABELS "fast;dxf;block;source;resource-budget;generated" TIMEOUT 60)
add_test(NAME libdxfrw_block_source_proof COMMAND libdxfrw_attdef_source_tests --block-proof)
set_tests_properties(libdxfrw_block_source_proof PROPERTIES
    LABELS "fast;dxf;block;source;independent;resource-budget;generated" TIMEOUT 60)
add_test(NAME libdxfrw_block_children_source COMMAND libdxfrw_attdef_source_tests --block-children)
set_tests_properties(libdxfrw_block_children_source PROPERTIES
    LABELS "fast;dxf;block;source;independent;resource-budget;generated" TIMEOUT 60)
add_test(NAME libdxfrw_raw_application_identity COMMAND libdxfrw_attdef_source_tests --raw-identity)
add_test(NAME libdxfrw_raw_proxy_application_scope COMMAND libdxfrw_attdef_source_tests --proxy-scope)
add_test(NAME libdxfrw_raw_application_writer COMMAND libdxfrw_attdef_source_tests --raw-writer)
set_tests_properties(libdxfrw_raw_application_identity libdxfrw_raw_proxy_application_scope libdxfrw_raw_application_writer PROPERTIES
    LABELS "fast;dxf;raw;proxy;application;source;generated" TIMEOUT 60)
add_test(NAME libdxfrw_raw_shell_application_scope COMMAND libdxfrw_attdef_source_tests --raw-shell)
add_test(NAME libdxfrw_acsh_projection_application_scope COMMAND libdxfrw_attdef_source_tests --acsh-scope)
set_tests_properties(libdxfrw_raw_shell_application_scope libdxfrw_acsh_projection_application_scope PROPERTIES
    LABELS "fast;dxf;raw;acsh;application;source;generated" TIMEOUT 60)
add_test(NAME libdxfrw_block_child_delivery COMMAND libdxfrw_attdef_source_tests --block-delivery)
set_tests_properties(libdxfrw_block_child_delivery PROPERTIES
    LABELS "fast;dxf;block;source;delivery;independent;generated" TIMEOUT 60)
add_test(NAME libdxfrw_attdef_original_origin COMMAND libdxfrw_attdef_source_tests --attdef-origin)
set_tests_properties(libdxfrw_attdef_original_origin PROPERTIES
    LABELS "fast;dxf;block;attdef;source;origin;independent;generated" TIMEOUT 60)
add_test(NAME libdxfrw_block_original_membership COMMAND libdxfrw_attdef_source_tests --block-membership)
set_tests_properties(libdxfrw_block_original_membership PROPERTIES
    LABELS "fast;dxf;block;compound;source;independent;generated" TIMEOUT 60)
add_test(NAME libdxfrw_compound_body_replay COMMAND libdxfrw_attdef_source_tests --compound-body)
set_tests_properties(libdxfrw_compound_body_replay PROPERTIES
    LABELS "fast;dxf;block;compound;source;independent;generated" TIMEOUT 120)
add_test(NAME libdxfrw_compound_source_profile COMMAND libdxfrw_attdef_source_tests --compound-profile)
set_tests_properties(libdxfrw_compound_source_profile PROPERTIES
    LABELS "fast;dxf;block;compound;source;profile;independent;generated" TIMEOUT 60)
add_test(NAME libdxfrw_application_handle_width COMMAND libdxfrw_attdef_source_tests --application-width)
set_tests_properties(libdxfrw_application_handle_width PROPERTIES
    LABELS "fast;dxf;application;handles;source;generated" TIMEOUT 60)

add_executable(libdxfrw_table_content_dxf_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/table_content_dxf_tests.cpp ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_table_content_dxf_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_table_content_dxf_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_table_content_dxf_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_table_content_dxf_tests PRIVATE dxfrw)
add_executable(libdxfrw_legacy_entities_compound_join_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/legacy_entities_compound_join_tests.cpp ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_legacy_entities_compound_join_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_legacy_entities_compound_join_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_legacy_entities_compound_join_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../src ${CMAKE_CURRENT_SOURCE_DIR}/../dwg2dxf)
target_link_libraries(libdxfrw_legacy_entities_compound_join_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_legacy_entities_compound_join COMMAND libdxfrw_legacy_entities_compound_join_tests)
set_tests_properties(libdxfrw_legacy_entities_compound_join PROPERTIES LABELS "fast;dxf;adapter;entities;compound;generated" TIMEOUT 60)

add_executable(libdxfrw_legacy_blockref_relation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/legacy_blockref_relation_tests.cpp ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_legacy_blockref_relation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_legacy_blockref_relation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_legacy_blockref_relation_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../src ${CMAKE_CURRENT_SOURCE_DIR}/../dwg2dxf)
target_link_libraries(libdxfrw_legacy_blockref_relation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_legacy_blockref_relation COMMAND libdxfrw_legacy_blockref_relation_tests)
set_tests_properties(libdxfrw_legacy_blockref_relation PROPERTIES LABELS "fast;dxf;adapter;entities;source;generated" TIMEOUT 60)

# Authentication controls use authored in-memory / temporary DXF records.
foreach(facet codec adapter)
    add_executable(libdxfrw_entities_ledger_${facet}_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/entities_ledger_${facet}_tests.cpp ${_libdxfrw_test_adapter_source})
    target_compile_features(libdxfrw_entities_ledger_${facet}_tests PRIVATE cxx_std_17)
    target_compile_options(libdxfrw_entities_ledger_${facet}_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
    target_include_directories(libdxfrw_entities_ledger_${facet}_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../src ${CMAKE_CURRENT_SOURCE_DIR}/../dwg2dxf)
    target_link_libraries(libdxfrw_entities_ledger_${facet}_tests PRIVATE dxfrw)
    add_test(NAME libdxfrw_entities_ledger_${facet} COMMAND libdxfrw_entities_ledger_${facet}_tests)
    set_tests_properties(libdxfrw_entities_ledger_${facet} PROPERTIES LABELS "fast;dxf;adapter;entities;authentication;generated" TIMEOUT 60)
endforeach()
add_executable(libdxfrw_acds_delivery_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/acds_delivery_tests.cpp ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_acds_delivery_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_acds_delivery_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_acds_delivery_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src ${CMAKE_CURRENT_SOURCE_DIR}/../dwg2dxf)
target_link_libraries(libdxfrw_acds_delivery_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_acds_source_delivery COMMAND libdxfrw_acds_delivery_tests)
set_tests_properties(libdxfrw_acds_source_delivery PROPERTIES LABELS "fast;dxf;adapter;acds;generated" TIMEOUT 60)
add_test(NAME libdxfrw_table_content_dxf_projection COMMAND libdxfrw_table_content_dxf_tests)
add_test(NAME libdxfrw_table_legacy_override_177
    COMMAND libdxfrw_table_content_dxf_tests --legacy-table-177)
set_tests_properties(libdxfrw_table_legacy_override_177 PROPERTIES LABELS "dxf;table")
add_test(NAME libdxfrw_table_legacy_text_height
    COMMAND libdxfrw_table_content_dxf_tests --legacy-table-height)
set_tests_properties(libdxfrw_table_legacy_text_height PROPERTIES LABELS "dxf;table")
add_test(NAME libdxfrw_table_legacy_merge
    COMMAND libdxfrw_table_content_dxf_tests --legacy-table-merge)
set_tests_properties(libdxfrw_table_legacy_merge PROPERTIES LABELS "fast;dxf;table;generated" TIMEOUT 60)
set_tests_properties(libdxfrw_table_content_dxf_projection PROPERTIES
    LABELS "fast;dxf;adapter;table;generated" TIMEOUT 30)
add_test(NAME libdxfrw_table_compound_source
    COMMAND libdxfrw_table_content_dxf_tests --compound-source)
set_tests_properties(libdxfrw_table_compound_source PROPERTIES
    LABELS "fast;dxf;adapter;table;generated" TIMEOUT 30)
add_test(NAME libdxfrw_table_content_source_proof
    COMMAND libdxfrw_table_content_dxf_tests --compound-content-proof)
set_tests_properties(libdxfrw_table_content_source_proof PROPERTIES
    LABELS "fast;dxf;adapter;table;generated" TIMEOUT 30)
add_test(NAME libdxfrw_table_content_reference_observer_selftest
    COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -I -B
        ${CMAKE_CURRENT_SOURCE_DIR}/table_content_reference_observer.py --self-test)
set_tests_properties(libdxfrw_table_content_reference_observer_selftest PROPERTIES
    LABELS "fast;dxf;table;independent;selftest" TIMEOUT 10)
add_test(NAME libdxfrw_table_content_bindings
    COMMAND libdxfrw_table_content_dxf_tests --compound-bindings)
set_tests_properties(libdxfrw_table_content_bindings PROPERTIES
    LABELS "fast;dxf;adapter;table;generated" TIMEOUT 30)
add_test(NAME libdxfrw_table_block_source_bindings
    COMMAND libdxfrw_table_content_dxf_tests --block-source-bindings)
set_tests_properties(libdxfrw_table_block_source_bindings PROPERTIES
    LABELS "fast;dxf;adapter;table;block;source;independent;generated" TIMEOUT 60)
add_test(NAME libdxfrw_table_block_name_bindings
    COMMAND libdxfrw_table_content_dxf_tests --block-name-bindings)
set_tests_properties(libdxfrw_table_block_name_bindings PROPERTIES
    LABELS "fast;dxf;adapter;table;block;source;graph;generated" TIMEOUT 60)
add_test(NAME libdxfrw_entities_source_transport
    COMMAND libdxfrw_table_content_dxf_tests --entities-source)
set_tests_properties(libdxfrw_entities_source_transport PROPERTIES
    LABELS "fast;dxf;adapter;source;entities;independent;generated" TIMEOUT 60)
add_test(NAME libdxfrw_entities_delivery_transport
    COMMAND libdxfrw_table_content_dxf_tests --entities-delivery)
set_tests_properties(libdxfrw_entities_delivery_transport PROPERTIES
    LABELS "fast;dxf;adapter;source;entities;dispatch;generated" TIMEOUT 60)
add_test(NAME libdxfrw_entities_immutable_origins
    COMMAND libdxfrw_table_content_dxf_tests --entities-origins)
set_tests_properties(libdxfrw_entities_immutable_origins PROPERTIES
    LABELS "fast;dxf;adapter;source;entities;dispatch;independent;generated" TIMEOUT 60)
add_test(NAME libdxfrw_entities_compound_source
    COMMAND libdxfrw_table_content_dxf_tests --entities-compounds)
set_tests_properties(libdxfrw_entities_compound_source PROPERTIES
    LABELS "fast;dxf;adapter;source;entities;compound;independent;generated" TIMEOUT 60)
add_test(NAME libdxfrw_entities_primary_source
    COMMAND libdxfrw_table_content_dxf_tests --entities-primaries)
set_tests_properties(libdxfrw_entities_primary_source PROPERTIES
    LABELS "fast;dxf;adapter;source;entities;primary;independent;generated" TIMEOUT 60)
add_test(NAME libdxfrw_layout_source_proof
    COMMAND libdxfrw_table_content_dxf_tests --layout-source)
set_tests_properties(libdxfrw_layout_source_proof PROPERTIES
    LABELS "fast;dxf;adapter;source;layout;independent;generated" TIMEOUT 60)
add_test(NAME libdxfrw_identity_source_namespace
    COMMAND libdxfrw_table_content_dxf_tests --identity-source)
set_tests_properties(libdxfrw_identity_source_namespace PROPERTIES
    LABELS "fast;dxf;adapter;source;identity;independent;generated" TIMEOUT 60)
add_test(NAME libdxfrw_structural_source_origins
    COMMAND libdxfrw_table_content_dxf_tests --structural-source)
set_tests_properties(libdxfrw_structural_source_origins PROPERTIES
    LABELS "fast;dxf;adapter;source;identity;block;dictionary;generated" TIMEOUT 60)
add_test(NAME libdxfrw_dictionary_source_origins
    COMMAND libdxfrw_table_content_dxf_tests --dictionary-source)
set_tests_properties(libdxfrw_dictionary_source_origins PROPERTIES
    LABELS "fast;dxf;adapter;source;identity;dictionary;generated" TIMEOUT 60)
add_test(NAME libdxfrw_original_layout_roles
    COMMAND libdxfrw_table_content_dxf_tests --layout-roles)
set_tests_properties(libdxfrw_original_layout_roles PROPERTIES
    LABELS "fast;dxf;adapter;source;identity;dictionary;layout;graph;generated" TIMEOUT 60)
add_test(NAME libdxfrw_original_entities_roles
    COMMAND libdxfrw_table_content_dxf_tests --entities-roles)
set_tests_properties(libdxfrw_original_entities_roles PROPERTIES
    LABELS "fast;dxf;adapter;source;identity;entities;layout;graph;generated" TIMEOUT 60)
add_test(NAME libdxfrw_block_primary_source
    COMMAND libdxfrw_attdef_source_tests --block-primary)
set_tests_properties(libdxfrw_block_primary_source PROPERTIES
    LABELS "fast;dxf;adapter;source;identity;block;generated" TIMEOUT 60)
add_test(NAME libdxfrw_block_producer_origins
    COMMAND libdxfrw_attdef_source_tests --block-origins)
set_tests_properties(libdxfrw_block_producer_origins PROPERTIES
    LABELS "fast;dxf;adapter;source;identity;block;generated" TIMEOUT 60)
add_test(NAME libdxfrw_block_empty_polyline_source
    COMMAND libdxfrw_attdef_source_tests --empty-block-polyline)
set_tests_properties(libdxfrw_block_empty_polyline_source PROPERTIES
    LABELS "fast;dxf;adapter;source;identity;block;generated" TIMEOUT 60)
add_test(NAME libdxfrw_original_block_roles
    COMMAND libdxfrw_table_content_dxf_tests --block-roles)
set_tests_properties(libdxfrw_original_block_roles PROPERTIES
    LABELS "fast;dxf;adapter;source;identity;block;generated" TIMEOUT 60)
add_test(NAME libdxfrw_original_attdef_roles
    COMMAND libdxfrw_table_content_dxf_tests --attdef-roles)
set_tests_properties(libdxfrw_original_attdef_roles PROPERTIES
    LABELS "fast;dxf;adapter;source;identity;attdef;block;entities;independent;generated" TIMEOUT 60)
add_test(NAME libdxfrw_table_block_source_transport
    COMMAND libdxfrw_table_content_dxf_tests --table-block-source)
set_tests_properties(libdxfrw_table_block_source_transport PROPERTIES
    LABELS "fast;dxf;adapter;source;table;block;generated" TIMEOUT 60)
add_test(NAME libdxfrw_table_original_inputs
    COMMAND libdxfrw_table_content_dxf_tests --table-inputs)
set_tests_properties(libdxfrw_table_original_inputs PROPERTIES
    LABELS "fast;dxf;adapter;source;identity;table;block;entities;generated" TIMEOUT 60)
add_test(NAME libdxfrw_table_original_bodies
    COMMAND libdxfrw_table_content_dxf_tests --table-bodies)
set_tests_properties(libdxfrw_table_original_bodies PROPERTIES
    LABELS "fast;dxf;adapter;source;identity;table;block;entities;generated" TIMEOUT 60)
add_test(NAME libdxfrw_table_original_profiles
    COMMAND libdxfrw_table_content_dxf_tests --table-profiles)
set_tests_properties(libdxfrw_table_original_profiles PROPERTIES
    LABELS "fast;dxf;adapter;source;identity;table;block;entities;independent;generated" TIMEOUT 60)
add_test(NAME libdxfrw_table_value_metadata_scopes
    COMMAND libdxfrw_table_content_dxf_tests --table-value-scopes)
set_tests_properties(libdxfrw_table_value_metadata_scopes PROPERTIES
    LABELS "fast;dxf;adapter;source;table;generated" TIMEOUT 30)
add_test(NAME libdxfrw_table_export_reuse
    COMMAND libdxfrw_table_content_dxf_tests --table-export-reuse)
set_tests_properties(libdxfrw_table_export_reuse PROPERTIES
    LABELS "fast;dxf;adapter;table;generated;transaction" TIMEOUT 30)
add_test(NAME libdxfrw_table_value_payload
    COMMAND libdxfrw_table_content_dxf_tests --table-value-payload)
set_tests_properties(libdxfrw_table_value_payload PROPERTIES
    LABELS "fast;dxf;adapter;table;generated;value" TIMEOUT 30)
add_test(NAME libdxfrw_table_value_inventory_selftest
    COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -I -B
        ${CMAKE_SOURCE_DIR}/tools/audit_dxf_table_values.py --self-test)
add_test(NAME libdxfrw_table_value_original_profiles
    COMMAND libdxfrw_table_content_dxf_tests --table-value-profiles)
set_tests_properties(libdxfrw_table_value_original_profiles PROPERTIES
    LABELS "fast;dxf;adapter;table;value;source;independent;generated" TIMEOUT 30)
add_test(NAME libdxfrw_table_original_reference_targets
    COMMAND libdxfrw_table_content_dxf_tests --table-references)
set_tests_properties(libdxfrw_table_original_reference_targets PROPERTIES
    LABELS "fast;dxf;adapter;table;value;source;identity;generated" TIMEOUT 30)
add_test(NAME libdxfrw_resource_original_publications
    COMMAND libdxfrw_table_content_dxf_tests --resource-origins)
set_tests_properties(libdxfrw_resource_original_publications PROPERTIES
    LABELS "fast;dxf;adapter;source;identity;generated;resource" TIMEOUT 30)
add_test(NAME libdxfrw_symbol_original_publications
    COMMAND libdxfrw_table_content_dxf_tests --symbol-origins)
set_tests_properties(libdxfrw_symbol_original_publications PROPERTIES
    LABELS "fast;dxf;adapter;source;identity;generated;symbol" TIMEOUT 30)
add_test(NAME libdxfrw_original_dependency_targets
    COMMAND libdxfrw_table_content_dxf_tests --dependencies)
set_tests_properties(libdxfrw_original_dependency_targets PROPERTIES
    LABELS "fast;dxf;adapter;source;identity;generated;dependency" TIMEOUT 30)
add_test(NAME libdxfrw_style_font_xdata_scopes
    COMMAND libdxfrw_table_content_dxf_tests --style-font-xdata)
set_tests_properties(libdxfrw_style_font_xdata_scopes PROPERTIES
    LABELS "fast;dxf;adapter;source;style;xdata;generated" TIMEOUT 30)
add_test(NAME libdxfrw_plotstyle_original_sources
    COMMAND libdxfrw_table_content_dxf_tests --plotstyle-sources)
set_tests_properties(libdxfrw_plotstyle_original_sources PROPERTIES
    LABELS "fast;dxf;adapter;source;plotstyle;generated" TIMEOUT 30)
add_test(NAME libdxfrw_dictionary_effective_ownership
    COMMAND libdxfrw_table_content_dxf_tests --effective-ownership)
set_tests_properties(libdxfrw_dictionary_effective_ownership PROPERTIES
    LABELS "fast;dxf;adapter;source;ownership;generated" TIMEOUT 30)
add_test(NAME libdxfrw_table_geometry_dependencies
    COMMAND libdxfrw_table_content_dxf_tests --table-geometry-dependencies)
set_tests_properties(libdxfrw_table_geometry_dependencies PROPERTIES
    LABELS "fast;dxf;adapter;source;dependency;table;generated" TIMEOUT 30)
add_test(NAME libdxfrw_original_entity_dependencies
    COMMAND libdxfrw_table_content_dxf_tests --entity-dependencies)
set_tests_properties(libdxfrw_original_entity_dependencies PROPERTIES
    LABELS "fast;dxf;adapter;source;dependency;entity;generated" TIMEOUT 30)
add_test(NAME libdxfrw_original_layout_dependencies
    COMMAND libdxfrw_table_content_dxf_tests --layout-dependencies)
set_tests_properties(libdxfrw_original_layout_dependencies PROPERTIES
    LABELS "fast;dxf;adapter;source;dependency;layout;generated" TIMEOUT 30)
add_test(NAME libdxfrw_view_ucs_original_sources
    COMMAND libdxfrw_table_content_dxf_tests --view-ucs-sources)
set_tests_properties(libdxfrw_view_ucs_original_sources PROPERTIES
    LABELS "fast;dxf;adapter;source;dependency;view;ucs;generated" TIMEOUT 30)
add_test(NAME libdxfrw_viewport_original_sources
    COMMAND libdxfrw_table_content_dxf_tests --viewport-sources)
set_tests_properties(libdxfrw_viewport_original_sources PROPERTIES
    LABELS "fast;dxf;adapter;source;dependency;viewport;generated" TIMEOUT 30)
set_tests_properties(libdxfrw_table_value_inventory_selftest PROPERTIES
    LABELS "fast;dxf;table;independent;selftest" TIMEOUT 10)
add_test(NAME libdxfrw_layout_graph_read_contract
    COMMAND libdxfrw_dxf_fixture_tests --layout-graph-read-contract)
set_tests_properties(libdxfrw_layout_graph_read_contract PROPERTIES
    LABELS "fast;dxf;adapter;layout;graph;compatibility" TIMEOUT 60)
add_test(NAME libdxfrw_cellstylemap_source_profiles
    COMMAND libdxfrw_table_content_dxf_tests --cell-style-map-source)
set_tests_properties(libdxfrw_cellstylemap_source_profiles PROPERTIES
    LABELS "fast;dxf;adapter;table;generated" TIMEOUT 30)
add_test(NAME libdxfrw_cellstylemap_source_proof
    COMMAND libdxfrw_table_content_dxf_tests --cell-style-map-proof)
set_tests_properties(libdxfrw_cellstylemap_source_proof PROPERTIES
    LABELS "fast;dxf;adapter;table;generated" TIMEOUT 60)
add_test(NAME libdxfrw_table_resource_reference_observer_selftest
    COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -I -B
        ${CMAKE_CURRENT_SOURCE_DIR}/table_resource_reference_observer.py --self-test)
set_tests_properties(libdxfrw_table_resource_reference_observer_selftest PROPERTIES
    LABELS "fast;dxf;table;independent;selftest" TIMEOUT 10)
add_test(NAME libdxfrw_tablestyle_source_proof
    COMMAND libdxfrw_table_content_dxf_tests --table-style-proof)
set_tests_properties(libdxfrw_tablestyle_source_proof PROPERTIES
    LABELS "fast;dxf;adapter;table;generated" TIMEOUT 90)
add_test(NAME libdxfrw_symbol_source_proof
    COMMAND libdxfrw_table_content_dxf_tests --symbol-source-proof)
set_tests_properties(libdxfrw_symbol_source_proof PROPERTIES
    LABELS "fast;dxf;adapter;table;generated" TIMEOUT 60)
add_test(NAME libdxfrw_complex_symbol_bindings
    COMMAND libdxfrw_table_content_dxf_tests --complex-symbol-bindings)
set_tests_properties(libdxfrw_complex_symbol_bindings PROPERTIES
    LABELS "fast;dxf;adapter;table;linetype;generated" TIMEOUT 30)
add_test(NAME libdxfrw_auxiliary_source_proof
    COMMAND libdxfrw_table_content_dxf_tests --auxiliary-source-proof)
set_tests_properties(libdxfrw_auxiliary_source_proof PROPERTIES
    LABELS "fast;dxf;adapter;table;field;datalink;generated" TIMEOUT 120)
add_test(NAME libdxfrw_spine_source_proof
    COMMAND libdxfrw_table_content_dxf_tests --spine-source-proof)
set_tests_properties(libdxfrw_spine_source_proof PROPERTIES
    LABELS "fast;dxf;adapter;table;dictionary;xrecord;generated" TIMEOUT 90)
add_test(NAME libdxfrw_block_record_source_proof
    COMMAND libdxfrw_table_content_dxf_tests --block-record-source-proof)
set_tests_properties(libdxfrw_block_record_source_proof PROPERTIES
    LABELS "fast;dxf;adapter;table;block-record;generated" TIMEOUT 90)
add_executable(libdxfrw_field_retained_budget_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/field_retained_budget_tests.cpp)
target_link_libraries(libdxfrw_field_retained_budget_tests PRIVATE dxfrw)
target_include_directories(libdxfrw_field_retained_budget_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src ${CMAKE_CURRENT_SOURCE_DIR}/../src/intern)
target_compile_options(libdxfrw_field_retained_budget_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
add_test(NAME libdxfrw_field_retained_owner_budget
    COMMAND libdxfrw_field_retained_budget_tests)
set_tests_properties(libdxfrw_field_retained_owner_budget PROPERTIES
    LABELS "fast;dxf;reader;field;resource-budget;generated" TIMEOUT 30)
if(LIBDXFRW_ENABLE_INDEPENDENT_DXF_ORACLE)
    add_test(NAME libdxfrw_independent_symbol_source
        COMMAND "${LIBDXFRW_EZDXF_PYTHON_EXECUTABLE}" -I -B
            ${CMAKE_CURRENT_SOURCE_DIR}/table_resource_reference_observer.py
            --generated-executable $<TARGET_FILE:libdxfrw_table_content_dxf_tests>)
    set_tests_properties(libdxfrw_independent_symbol_source PROPERTIES
        LABELS "advisory;independent;dxf;table;linetype;generated" TIMEOUT 30)
    add_test(NAME libdxfrw_independent_auxiliary_source
        COMMAND "${LIBDXFRW_EZDXF_PYTHON_EXECUTABLE}" -I -B
            ${CMAKE_CURRENT_SOURCE_DIR}/table_resource_reference_observer.py
            --generated-auxiliary-executable $<TARGET_FILE:libdxfrw_table_content_dxf_tests>)
    set_tests_properties(libdxfrw_independent_auxiliary_source PROPERTIES
        LABELS "advisory;independent;dxf;table;field;datalink;generated" TIMEOUT 30)
    add_test(NAME libdxfrw_independent_spine_source
        COMMAND "${LIBDXFRW_EZDXF_PYTHON_EXECUTABLE}" -I -B
            ${CMAKE_CURRENT_SOURCE_DIR}/table_resource_reference_observer.py
            --generated-spine-executable $<TARGET_FILE:libdxfrw_table_content_dxf_tests>)
    set_tests_properties(libdxfrw_independent_spine_source PROPERTIES
        LABELS "advisory;independent;dxf;table;dictionary;xrecord;generated" TIMEOUT 30)
    add_test(NAME libdxfrw_independent_block_record_source
        COMMAND "${LIBDXFRW_EZDXF_PYTHON_EXECUTABLE}" -I -B
            ${CMAKE_CURRENT_SOURCE_DIR}/table_resource_reference_observer.py
            --generated-block-record-executable $<TARGET_FILE:libdxfrw_table_content_dxf_tests>)
    set_tests_properties(libdxfrw_independent_block_record_source PROPERTIES
        LABELS "advisory;independent;dxf;table;block-record;generated" TIMEOUT 30)
endif()

add_test(
    NAME dwg2dxf_field_dxf_multichunk_value
    COMMAND libdxfrw_dwg2dxf_adapter_tests --field-dxf-multichunk-value
)
set_tests_properties(dwg2dxf_field_dxf_multichunk_value PROPERTIES
    LABELS "fast;dxf;adapter;field"
    TIMEOUT 10
)

# Keep the typed/raw view-style overlay roundtrip independently runnable from
# the broader named-object callback collection.
add_test(
    NAME dwg2dxf_modeldoc_view_style_callbacks
    COMMAND libdxfrw_dwg2dxf_adapter_tests --modeldoc-view-style-callbacks
)
set_tests_properties(dwg2dxf_modeldoc_view_style_callbacks PROPERTIES
    LABELS "fast;dxf;adapter;objects;view-style"
    TIMEOUT 10
)

# Typed AcSh history callback copies are bounded before retention and any
# rejected callback makes conversion fail closed; all data is generated here.
add_test(
    NAME dwg2dxf_acsh_history_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests --acsh-history-capture-limits
)
set_tests_properties(dwg2dxf_acsh_history_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;acds"
    TIMEOUT 10
)

add_test(
    NAME dwg2dxf_acds_history_dwg_frame_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests --acds-history-dwg-frame-capture-limits
)
set_tests_properties(dwg2dxf_acds_history_dwg_frame_capture_limits PROPERTIES
    LABELS "fast;dwg;adapter;acds"
    TIMEOUT 10
)

add_test(
    NAME dwg2dxf_standard_dwg_object_source_capture
    COMMAND libdxfrw_dwg2dxf_adapter_tests --standard-dwg-object-source-capture
)
set_tests_properties(dwg2dxf_standard_dwg_object_source_capture PROPERTIES
    LABELS "fast;dwg;adapter;objects"
    TIMEOUT 10
)
add_test(NAME dwg2dxf_standard_dwg_object_payload_correlation
    COMMAND libdxfrw_dwg2dxf_adapter_tests --standard-dwg-object-payload-correlation)
set_tests_properties(dwg2dxf_standard_dwg_object_payload_correlation PROPERTIES
    LABELS "fast;dwg;adapter;objects;generated" TIMEOUT 10)
add_test(NAME dwg2dxf_standard_dwg_source_text_observations
    COMMAND libdxfrw_dwg2dxf_adapter_tests --standard-dwg-source-text-observations)
set_tests_properties(dwg2dxf_standard_dwg_source_text_observations PROPERTIES
    LABELS "fast;dwg;adapter;objects;codepage;generated" TIMEOUT 10)
add_test(NAME dwg2dxf_standard_dwg_source_eed_text_observations
    COMMAND libdxfrw_dwg2dxf_adapter_tests --standard-dwg-source-eed-text-observations)
set_tests_properties(dwg2dxf_standard_dwg_source_eed_text_observations PROPERTIES
    LABELS "fast;dwg;adapter;objects;codepage;xdata;generated" TIMEOUT 10)
add_test(NAME dwg2dxf_standard_dwg_appid_dependencies
    COMMAND libdxfrw_dwg2dxf_adapter_tests --standard-dwg-appid-dependencies)
set_tests_properties(dwg2dxf_standard_dwg_appid_dependencies PROPERTIES
    LABELS "fast;dwg;adapter;objects;codepage;appid;generated" TIMEOUT 10)
add_test(NAME dwg2dxf_standard_dwg_layer_dependencies
    COMMAND libdxfrw_dwg2dxf_adapter_tests --standard-dwg-layer-dependencies)
set_tests_properties(dwg2dxf_standard_dwg_layer_dependencies PROPERTIES
    LABELS "fast;dwg;adapter;objects;codepage;layer;generated" TIMEOUT 10)
add_test(NAME dwg2dxf_xrecord_group_reference_admission
    COMMAND libdxfrw_dwg2dxf_adapter_tests --xrecord-group-reference-admission)
set_tests_properties(dwg2dxf_xrecord_group_reference_admission PROPERTIES
    LABELS "fast;adapter;objects;graph;generated" TIMEOUT 10)
add_test(NAME dwg2dxf_generated_image_dictionary_source_identity
    COMMAND libdxfrw_dwg2dxf_adapter_tests --generated-image-dictionary-source-identity)
set_tests_properties(dwg2dxf_generated_image_dictionary_source_identity PROPERTIES
    # Includes108 wire-fault refusal/retries plus native readback; allow ASan
    # headroom under concurrent CTest load without removing any controls.
    LABELS "fast;dwg;dxf;image;adapter;objects;generated" TIMEOUT 30)
add_test(NAME dwg2dxf_raster_dwg_source_profile_eed
    COMMAND libdxfrw_dwg2dxf_adapter_tests --raster-dwg-source-profile-eed)
set_tests_properties(dwg2dxf_raster_dwg_source_profile_eed PROPERTIES
    LABELS "fast;dwg;dxf;image;eed;adapter;generated" TIMEOUT 30)
add_test(NAME dwg2dxf_raster_class_history
    COMMAND libdxfrw_dwg2dxf_adapter_tests --raster-class-history)
set_tests_properties(dwg2dxf_raster_class_history PROPERTIES
    LABELS "fast;dwg;dxf;image;classes;history;adapter;generated" TIMEOUT 30)
add_test(NAME dwg2dxf_rooted_custom_imagedef
    COMMAND libdxfrw_dwg2dxf_adapter_tests --rooted-custom-imagedef)
set_tests_properties(dwg2dxf_rooted_custom_imagedef PROPERTIES
    LABELS "fast;dwg;dxf;image;classes;graph;adapter;generated" TIMEOUT 30)
add_test(NAME dwg2dxf_rooted_image_dictionary_closure
    COMMAND libdxfrw_dwg2dxf_adapter_tests --rooted-imagedef-entry-closure)
set_tests_properties(dwg2dxf_rooted_image_dictionary_closure PROPERTIES
    LABELS "fast;dwg;dxf;image;graph;adapter;generated" TIMEOUT 45)
add_test(NAME dwg2dxf_generated_root_common_closure
    COMMAND libdxfrw_dwg2dxf_adapter_tests --generated-root-common-closure)
set_tests_properties(dwg2dxf_generated_root_common_closure PROPERTIES
    LABELS "fast;dxf;dictionary;graph;adapter;generated" TIMEOUT 30)
add_test(NAME dwg2dxf_generated_dictionary_binary_chunks
    COMMAND libdxfrw_dwg2dxf_adapter_tests --generated-dictionary-binary-chunks)
set_tests_properties(dwg2dxf_generated_dictionary_binary_chunks PROPERTIES
    LABELS "fast;dxf;dictionary;xdata;adapter;generated" TIMEOUT 60)
add_test(NAME dwg2dxf_generated_dictionary_chunk_faults
    COMMAND libdxfrw_dwg2dxf_adapter_tests --generated-dictionary-chunk-faults)
set_tests_properties(dwg2dxf_generated_dictionary_chunk_faults PROPERTIES
    LABELS "fast;dxf;dictionary;xdata;transaction;adapter;generated" TIMEOUT 60)
add_test(NAME dwg2dxf_standard_dwg_binary_transport
    COMMAND libdxfrw_dwg2dxf_adapter_tests --standard-dwg-binary-transport)
set_tests_properties(dwg2dxf_standard_dwg_binary_transport PROPERTIES
    LABELS "fast;dwg;dxf;objects;xdata;adapter;generated" TIMEOUT 60)
add_test(NAME dwg2dxf_native_xrecord_binary_transport
    COMMAND libdxfrw_dwg2dxf_adapter_tests --native-xrecord-binary-transport)
set_tests_properties(dwg2dxf_native_xrecord_binary_transport PROPERTIES
    LABELS "fast;dwg;dxf;objects;xrecord;adapter;generated" TIMEOUT 60)
add_test(NAME dwg2dxf_native_xrecord_binary_faults
    COMMAND libdxfrw_dwg2dxf_adapter_tests --native-xrecord-binary-faults)
set_tests_properties(dwg2dxf_native_xrecord_binary_faults PROPERTIES
    LABELS "fast;dwg;dxf;objects;xrecord;transaction;adapter;generated" TIMEOUT 60)
add_test(NAME dwg2dxf_xrecord_application_chunk_controls
    COMMAND libdxfrw_dwg2dxf_adapter_tests --xrecord-application-chunk-controls)
set_tests_properties(dwg2dxf_xrecord_application_chunk_controls PROPERTIES
    LABELS "fast;dxf;objects;xrecord;adapter;generated" TIMEOUT 30)
add_test(NAME dwg2dxf_native_xrecord_coordinates
    COMMAND libdxfrw_dwg2dxf_adapter_tests --native-xrecord-coordinates)
set_tests_properties(dwg2dxf_native_xrecord_coordinates PROPERTIES
    LABELS "fast;dwg;dxf;objects;xrecord;adapter;generated" TIMEOUT 60)
add_test(NAME dwg2dxf_multiple_selected_xrecords
    COMMAND libdxfrw_dwg2dxf_adapter_tests --multiple-selected-xrecords)
set_tests_properties(dwg2dxf_multiple_selected_xrecords PROPERTIES
    LABELS "fast;dwg;dxf;objects;xrecord;adapter;generated" TIMEOUT 60)
add_test(NAME dwg2dxf_branched_xrecord_common
    COMMAND libdxfrw_dwg2dxf_adapter_tests --branched-xrecord-common)
set_tests_properties(dwg2dxf_branched_xrecord_common PROPERTIES
    LABELS "fast;dwg;dxf;objects;xrecord;adapter;generated" TIMEOUT 60)
add_test(NAME dwg2dxf_xrecord_owned_extension
    COMMAND libdxfrw_dwg2dxf_adapter_tests --xrecord-owned-extension)
set_tests_properties(dwg2dxf_xrecord_owned_extension PROPERTIES
    LABELS "fast;dwg;dxf;objects;xrecord;adapter;generated" TIMEOUT 60)
add_test(NAME dwg2dxf_xrecord_owned_extension_faults
    COMMAND libdxfrw_dwg2dxf_adapter_tests --xrecord-owned-extension-faults)
set_tests_properties(dwg2dxf_xrecord_owned_extension_faults PROPERTIES
    LABELS "fast;dwg;dxf;objects;xrecord;transaction;adapter;generated" TIMEOUT 60)
add_test(NAME dwg2dxf_branched_xrecord_common_faults
    COMMAND libdxfrw_dwg2dxf_adapter_tests --branched-xrecord-common-faults)
set_tests_properties(dwg2dxf_branched_xrecord_common_faults PROPERTIES
    LABELS "fast;dwg;dxf;objects;xrecord;transaction;adapter;generated" TIMEOUT 60)
add_test(NAME dwg2dxf_multiple_selected_xrecord_faults
    COMMAND libdxfrw_dwg2dxf_adapter_tests --multiple-selected-xrecord-faults)
set_tests_properties(dwg2dxf_multiple_selected_xrecord_faults PROPERTIES
    LABELS "fast;dwg;dxf;objects;xrecord;transaction;adapter;generated" TIMEOUT 60)
add_test(NAME dwg2dxf_native_xrecord_coordinate_faults
    COMMAND libdxfrw_dwg2dxf_adapter_tests --native-xrecord-coordinate-faults)
set_tests_properties(dwg2dxf_native_xrecord_coordinate_faults PROPERTIES
    LABELS "fast;dwg;dxf;objects;xrecord;transaction;adapter;generated" TIMEOUT 30)
add_test(NAME libdxfrw_xrecord_point18
    COMMAND libdxfrw_dxf_fixture_tests --xrecord-point18)
set_tests_properties(libdxfrw_xrecord_point18 PROPERTIES
    LABELS "fast;dxf;objects;xrecord;generated" TIMEOUT 60)
add_test(NAME libdxfrw_table_entry_datastorage_refusal
    COMMAND libdxfrw_dxf_fixture_tests --table-entry-datastorage-refusal)
set_tests_properties(libdxfrw_table_entry_datastorage_refusal PROPERTIES
    LABELS "fast;dxf;objects;tables;acds;transaction;generated" TIMEOUT 30)
add_test(NAME libdxfrw_xrecord_coordinate_writer_domain
    COMMAND libdxfrw_dxf_fixture_tests --xrecord-coordinate-writer-domain)
set_tests_properties(libdxfrw_xrecord_coordinate_writer_domain PROPERTIES
    LABELS "fast;dxf;objects;xrecord;generated" TIMEOUT 30)
add_test(NAME dwg2dxf_raster_finite_real_controls
    COMMAND libdxfrw_dwg2dxf_adapter_tests --raster-finite-real-controls)
set_tests_properties(dwg2dxf_raster_finite_real_controls PROPERTIES
    LABELS "fast;dxf;objects;reader;adapter;generated" TIMEOUT 30)
add_test(NAME dwg2dxf_standard_dwg_profile_isolation
    COMMAND libdxfrw_dwg2dxf_adapter_tests --standard-dwg-profile-isolation)
set_tests_properties(dwg2dxf_standard_dwg_profile_isolation PROPERTIES
    LABELS "fast;dwg;dxf;objects;material;adapter;generated" TIMEOUT 60)
add_test(NAME dwg2dxf_native_xrecord_application_scope
    COMMAND libdxfrw_dwg2dxf_adapter_tests --native-xrecord-application-scope)
set_tests_properties(dwg2dxf_native_xrecord_application_scope PROPERTIES
    LABELS "fast;dwg;dxf;objects;xrecord;adapter;generated" TIMEOUT 60)
add_test(NAME dwg2dxf_native_xrecord_scope_faults
    COMMAND libdxfrw_dwg2dxf_adapter_tests --native-xrecord-scope-faults)
set_tests_properties(dwg2dxf_native_xrecord_scope_faults PROPERTIES
    LABELS "fast;dwg;dxf;objects;xrecord;transaction;adapter;generated" TIMEOUT 60)
add_test(NAME dwg2dxf_raster_xrecord_scope_framing
    COMMAND libdxfrw_dwg2dxf_adapter_tests --raster-xrecord-scope-framing)
set_tests_properties(dwg2dxf_raster_xrecord_scope_framing PROPERTIES
    LABELS "fast;dxf;objects;xrecord;adapter;generated" TIMEOUT 30)
add_test(NAME dwg2dxf_rooted_xrecord_string_widths
    COMMAND libdxfrw_dwg2dxf_adapter_tests --rooted-xrecord-string-widths)
set_tests_properties(dwg2dxf_rooted_xrecord_string_widths PROPERTIES
    LABELS "fast;dxf;objects;xrecord;adapter;generated" TIMEOUT 60)
add_test(NAME dwg2dxf_rooted_xrecord_string_width_faults
    COMMAND libdxfrw_dwg2dxf_adapter_tests --rooted-xrecord-string-width-faults)
set_tests_properties(dwg2dxf_rooted_xrecord_string_width_faults PROPERTIES
    LABELS "fast;dxf;objects;xrecord;transaction;adapter;generated" TIMEOUT 30)
add_test(NAME dwg2dxf_raster_token_width_controls
    COMMAND libdxfrw_dwg2dxf_adapter_tests --raster-token-width-controls)
set_tests_properties(dwg2dxf_raster_token_width_controls PROPERTIES
    LABELS "fast;dxf;objects;reader;adapter;generated" TIMEOUT 30)
add_test(NAME dwg2dxf_raster_observer_profiles
    COMMAND libdxfrw_dwg2dxf_adapter_tests --raster-observer-profiles)
set_tests_properties(dwg2dxf_raster_observer_profiles PROPERTIES
    LABELS "fast;dxf;objects;image;compatibility;adapter;generated" TIMEOUT 60)
add_test(NAME dwg2dxf_raster_observer_profile_faults
    COMMAND libdxfrw_dwg2dxf_adapter_tests --raster-observer-profile-faults)
set_tests_properties(dwg2dxf_raster_observer_profile_faults PROPERTIES
    LABELS "fast;dxf;objects;image;compatibility;transaction;adapter;generated" TIMEOUT 60)
add_test(NAME dwg2dxf_raster_profile_token_controls
    COMMAND libdxfrw_dwg2dxf_adapter_tests --raster-profile-token-controls)
set_tests_properties(dwg2dxf_raster_profile_token_controls PROPERTIES
    LABELS "fast;dxf;objects;reader;compatibility;adapter;generated" TIMEOUT 30)
add_test(NAME dwg2dxf_standard_output_framing_controls
    COMMAND libdxfrw_dwg2dxf_adapter_tests --standard-output-framing-controls)
set_tests_properties(dwg2dxf_standard_output_framing_controls PROPERTIES
    LABELS "fast;dxf;objects;compatibility;adapter;generated" TIMEOUT 60)
add_test(NAME dwg2dxf_image_dictionary_imported_common
    COMMAND libdxfrw_dwg2dxf_adapter_tests --image-dictionary-imported-common)
set_tests_properties(dwg2dxf_image_dictionary_imported_common PROPERTIES
    LABELS "fast;dwg;dxf;image;dictionary;adapter;generated" TIMEOUT 45)
add_test(NAME dwg2dxf_generated_dictionary_scalar_transport
    COMMAND libdxfrw_dwg2dxf_adapter_tests --generated-dictionary-scalar-transport)
set_tests_properties(dwg2dxf_generated_dictionary_scalar_transport PROPERTIES
    LABELS "fast;dxf;dictionary;xdata;adapter;generated" TIMEOUT 60)
add_test(NAME dwg2dxf_generated_dictionary_scalar_faults
    COMMAND libdxfrw_dwg2dxf_adapter_tests --generated-dictionary-scalar-faults)
set_tests_properties(dwg2dxf_generated_dictionary_scalar_faults PROPERTIES
    LABELS "fast;dxf;dictionary;xdata;transaction;adapter;generated" TIMEOUT 60)
add_test(NAME dwg2dxf_selected_raster_reactor_metadata
    COMMAND libdxfrw_dwg2dxf_adapter_tests --selected-raster-reactor-metadata)
set_tests_properties(dwg2dxf_selected_raster_reactor_metadata PROPERTIES
    LABELS "fast;dwg;dxf;image;eed;adapter;generated" TIMEOUT 30)
add_test(NAME dwg2dxf_acds_raw_dxf_source_baseline
    COMMAND libdxfrw_dwg2dxf_adapter_tests --acds-raw-dxf-source-baseline)
set_tests_properties(dwg2dxf_acds_raw_dxf_source_baseline PROPERTIES
    LABELS "fast;dxf;adapter;acds;retention;generated" TIMEOUT 10)

add_test(
    NAME dwg2dxf_raw_dxf_entity_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests --raw-dxf-entity-capture-limits
)
set_tests_properties(dwg2dxf_raw_dxf_entity_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;raw-entity"
    TIMEOUT 10
)

add_test(
    NAME dwg2dxf_raw_dxf_section_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests --raw-dxf-section-capture-limits
)
set_tests_properties(dwg2dxf_raw_dxf_section_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;raw-section"
    TIMEOUT 10
)

add_test(
    NAME dwg2dxf_plotsettings_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests --plotsettings-capture-limits
)
set_tests_properties(dwg2dxf_plotsettings_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;plotsettings"
    TIMEOUT 10
)

add_test(
    NAME libdxfrw_plotsettings_visualstyle_shade_reference
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --plotsettings-visualstyle-shade-reference
)
set_tests_properties(libdxfrw_plotsettings_visualstyle_shade_reference PROPERTIES
    LABELS "fast;dxf;graph;adapter;plotsettings"
    TIMEOUT 30
)
add_test(
    NAME libdxfrw_render_settings_owner_order
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --retain-render-settings-callback-and-fail-closed
)
set_tests_properties(libdxfrw_render_settings_owner_order PROPERTIES
    LABELS "fast;dxf;graph;adapter;render;plotsettings"
    TIMEOUT 30
)
add_test(NAME libdxfrw_rendering_dxf_projections
    COMMAND libdxfrw_dxf_fixture_tests --rendering-projections)
set_tests_properties(libdxfrw_rendering_dxf_projections PROPERTIES
    LABELS "fast;dxf;render;parser" TIMEOUT 30)
add_test(NAME libdxfrw_rendering_original_publications
    COMMAND libdxfrw_dxf_fixture_tests --rendering-publications)
set_tests_properties(libdxfrw_rendering_original_publications PROPERTIES
    LABELS "fast;dxf;render;parser;source" TIMEOUT 30)
add_test(NAME libdxfrw_rendering_input_failures
    COMMAND libdxfrw_table_content_dxf_tests --rendering-input-failures)
set_tests_properties(libdxfrw_rendering_input_failures PROPERTIES
    LABELS "fast;dxf;render;source;failure" TIMEOUT 30)
add_test(NAME libdxfrw_rendering_model_replay
    COMMAND libdxfrw_dxf_fixture_tests --rendering-model-replay)
set_tests_properties(libdxfrw_rendering_model_replay PROPERTIES
    LABELS "fast;dxf;render;model;source" TIMEOUT 30)
add_test(NAME libdxfrw_rendering_model_failures
    COMMAND libdxfrw_table_content_dxf_tests --rendering-model-failures)
set_tests_properties(libdxfrw_rendering_model_failures PROPERTIES
    LABELS "fast;dxf;render;model;failure" TIMEOUT 30)
add_test(NAME libdxfrw_visual_style_primary_properties
    COMMAND libdxfrw_dxf_fixture_tests --visual-style-properties)
set_tests_properties(libdxfrw_visual_style_primary_properties PROPERTIES
    LABELS "fast;dxf;render;model;parser;source" TIMEOUT 30)
add_test(NAME libdxfrw_rendering_original_sources
    COMMAND libdxfrw_table_content_dxf_tests --rendering-sources)
set_tests_properties(libdxfrw_rendering_original_sources PROPERTIES
    LABELS "fast;dxf;render;source;graph" TIMEOUT 30)
add_test(NAME libdxfrw_objects_physical_sources
    COMMAND libdxfrw_table_content_dxf_tests --object-physical)
set_tests_properties(libdxfrw_objects_physical_sources PROPERTIES
    LABELS "fast;dxf;render;source;graph;physical" TIMEOUT 30)
add_test(NAME libdxfrw_render_preset_sources
    COMMAND libdxfrw_table_content_dxf_tests --render-presets)
set_tests_properties(libdxfrw_render_preset_sources PROPERTIES
    LABELS "fast;dxf;render;source;graph;writer" TIMEOUT 30)
add_test(NAME libdxfrw_render_preset_output
    COMMAND libdxfrw_table_content_dxf_tests --render-preset-output)
set_tests_properties(libdxfrw_render_preset_output PROPERTIES
    LABELS "fast;dxf;render;writer;physical" TIMEOUT 30)
add_test(NAME libdxfrw_render_preset_graph_output
    COMMAND libdxfrw_table_content_dxf_tests --render-preset-graph-output)
set_tests_properties(libdxfrw_render_preset_graph_output PROPERTIES
    LABELS "fast;dxf;render;writer;graph;physical" TIMEOUT 30)
add_test(NAME libdxfrw_render_preset_transactions
    COMMAND libdxfrw_table_content_dxf_tests --render-preset-transactions)
set_tests_properties(libdxfrw_render_preset_transactions PROPERTIES
    LABELS "fast;dxf;render;writer;graph;physical" TIMEOUT 60)
add_test(NAME libdxfrw_render_preset_edits
    COMMAND libdxfrw_table_content_dxf_tests --render-preset-edits)
set_tests_properties(libdxfrw_render_preset_edits PROPERTIES
    LABELS "fast;dxf;render;writer;graph;physical" TIMEOUT 60)
add_test(NAME libdxfrw_full_style_carriers
    COMMAND libdxfrw_table_content_dxf_tests --full-style-carriers)
set_tests_properties(libdxfrw_full_style_carriers PROPERTIES
    LABELS "fast;dxf;render;writer;graph;physical" TIMEOUT 120)
add_test(NAME libdxfrw_shade_consumer_output
    COMMAND libdxfrw_table_content_dxf_tests --shade-consumer-output)
set_tests_properties(libdxfrw_shade_consumer_output PROPERTIES
    LABELS "fast;dxf;render;writer;graph;physical" TIMEOUT 30)

add_test(
    NAME dwg2dxf_evaluation_graph_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests --evaluation-graph-capture-limits
)
set_tests_properties(dwg2dxf_evaluation_graph_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;acds"
    TIMEOUT 10
)

add_test(
    NAME dwg2dxf_dynamic_block_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests --dynamic-block-capture-limits
)
set_tests_properties(dwg2dxf_dynamic_block_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;dynamic-block"
    TIMEOUT 10
)

add_test(
    NAME dwg2dxf_proxy_object_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests --proxy-object-capture-limits
)
set_tests_properties(dwg2dxf_proxy_object_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;proxy"
    TIMEOUT 10
)

add_test(
    NAME dwg2dxf_raw_dxf_object_list_node_accounting
    COMMAND libdxfrw_dwg2dxf_adapter_tests --raw-dxf-object-list-node-accounting
)
set_tests_properties(dwg2dxf_raw_dxf_object_list_node_accounting PROPERTIES
    LABELS "fast;dxf;adapter;retention"
    TIMEOUT 10
)

# DXF associative records publish one typed shell and one complete raw
# companion; malformed or truncated records must not publish later callbacks.
add_test(
    NAME dwg2dxf_associative_callback_contract
    COMMAND libdxfrw_dwg2dxf_adapter_tests --associative-dxf-callback-contract
)
set_tests_properties(dwg2dxf_associative_callback_contract PROPERTIES
    LABELS "fast;dxf;adapter;objects;associative"
    TIMEOUT 15
)

# Strict complete-output conversion must fail closed for associative DXF
# callbacks that the concrete adapter does not retain, preserving the existing
# destination and allowing a corrected same-adapter retry.
add_test(
    NAME dwg2dxf_associative_strict_loss_gate
    COMMAND libdxfrw_dwg2dxf_adapter_tests --associative-dxf-strict-loss-gate
)
set_tests_properties(dwg2dxf_associative_strict_loss_gate PROPERTIES
    LABELS "fast;dxf;adapter;objects;associative;strict"
    TIMEOUT 15
)

if(TARGET dwg2dxf)
    # Keep the concrete adapter's modern custom-class 3DLINE subtype and payload
    # through generated DXF, two exact FreeCAD-argv CLI passes, and public readback.
    add_test(
        NAME dwg2dxf_3dline_adapter_cli
        COMMAND ${CMAKE_COMMAND}
            "-DADAPTER_TEST=$<TARGET_FILE:libdxfrw_dwg2dxf_adapter_tests>"
            "-DDWG2DXF=$<TARGET_FILE:dwg2dxf>"
            "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
            "-DCASE_NAME=3dline-adapter"
            "-DGENERATE_MODE=--generate-3dline"
            "-DVERIFY_MODE=--verify-3dline"
            "-DNEGATIVE_MODE=--reject-3dline-ac1014"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg2dxf_image_path_test.cmake
    )
    set_tests_properties(dwg2dxf_3dline_adapter_cli PROPERTIES
        LABELS "cli;fast;dxf;adapter;3d"
        TIMEOUT 30
    )

    # Exercise the same generated adapter subtype through binary-DXF input and
    # output, with public readback after two exact CLI conversion passes.
    add_test(
        NAME dwg2dxf_3dline_binary_adapter_cli
        COMMAND ${CMAKE_COMMAND}
            "-DADAPTER_TEST=$<TARGET_FILE:libdxfrw_dwg2dxf_adapter_tests>"
            "-DDWG2DXF=$<TARGET_FILE:dwg2dxf>"
            "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
            "-DCASE_NAME=3dline-binary-adapter"
            "-DGENERATE_MODE=--generate-3dline-binary"
            "-DVERIFY_MODE=--verify-3dline"
            "-DCONVERSION_OPTIONS=-b"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg2dxf_image_path_test.cmake
    )
    set_tests_properties(dwg2dxf_3dline_binary_adapter_cli PROPERTIES
        LABELS "cli;fast;dxf;adapter;3d;binary"
        TIMEOUT 30
    )

    # Retain zero-fit HATCH tangent metadata through two exact-argv CLI passes.
    # The local-roundtrip executable authors both encodings at runtime.
    foreach(_hatch_encoding IN ITEMS ascii binary)
        set(_hatch_generate_mode "--generate-hatch-tangents")
        set(_hatch_conversion_options "")
        if(_hatch_encoding STREQUAL "binary")
            set(_hatch_generate_mode "--generate-hatch-tangents-binary")
            set(_hatch_conversion_options "-b")
        endif()
        add_test(
            NAME dwg2dxf_hatch_tangents_${_hatch_encoding}_cli
            COMMAND ${CMAKE_COMMAND}
                "-DADAPTER_TEST=$<TARGET_FILE:libdxfrw_dwg_local_roundtrip>"
                "-DDWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
                "-DCASE_NAME=hatch-tangents-${_hatch_encoding}"
                "-DGENERATE_MODE=${_hatch_generate_mode}"
                "-DVERIFY_MODE=--verify-hatch-tangents"
                "-DCONVERSION_OPTIONS=${_hatch_conversion_options}"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg2dxf_image_path_test.cmake
        )
        set_tests_properties(dwg2dxf_hatch_tangents_${_hatch_encoding}_cli PROPERTIES
            LABELS "cli;fast;dxf;adapter;hatch"
            TIMEOUT 30
        )
    endforeach()

    # LEADER group76 is advisory on OPEN; literal source counts differ from the
    # actual arrays and must canonicalize through two real CLI passes.
    foreach(_leader_encoding IN ITEMS ascii binary)
        set(_leader_generate_mode --generate-leader-advisory-counts)
        set(_leader_options "")
        if(_leader_encoding STREQUAL "binary")
            set(_leader_generate_mode --generate-leader-advisory-counts-binary)
            set(_leader_options -b)
        endif()
        add_test(NAME dwg2dxf_leader_advisory_count_${_leader_encoding}_cli
            COMMAND ${CMAKE_COMMAND}
                -DADAPTER_TEST=$<TARGET_FILE:libdxfrw_dxf_fixture_tests>
                -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
                -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
                -DCASE_NAME=leader-advisory-count-${_leader_encoding}
                -DGENERATE_MODE=${_leader_generate_mode}
                -DVERIFY_MODE=--verify-leader-advisory-counts
                -DCONVERSION_OPTIONS=${_leader_options}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg2dxf_image_path_test.cmake)
        set_tests_properties(dwg2dxf_leader_advisory_count_${_leader_encoding}_cli
            PROPERTIES LABELS "cli;fast;dxf;adapter;leader" TIMEOUT 30)
    endforeach()

    # Independently authored version/fill pairs with polyline/bulge boundaries.
    # Read back after both exact-argv CLI passes; no drawing payload is vendored.
    foreach(_mpolygon_encoding IN ITEMS ascii binary)
        set(_mpolygon_generate_mode "--generate-mpolygon-header")
        set(_mpolygon_conversion_options "")
        if(_mpolygon_encoding STREQUAL "binary")
            set(_mpolygon_generate_mode "--generate-mpolygon-header-binary")
            set(_mpolygon_conversion_options "-b")
        endif()
        add_test(
            NAME dwg2dxf_mpolygon_header_${_mpolygon_encoding}_cli
            COMMAND ${CMAKE_COMMAND}
                "-DADAPTER_TEST=$<TARGET_FILE:libdxfrw_dwg_local_roundtrip>"
                "-DDWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
                "-DCASE_NAME=mpolygon-header-${_mpolygon_encoding}"
                "-DGENERATE_MODE=${_mpolygon_generate_mode}"
                "-DVERIFY_MODE=--verify-mpolygon-header"
                "-DCONVERSION_OPTIONS=${_mpolygon_conversion_options}"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg2dxf_image_path_test.cmake
        )
        set_tests_properties(dwg2dxf_mpolygon_header_${_mpolygon_encoding}_cli PROPERTIES
            LABELS "cli;fast;dxf;adapter;mpolygon"
            TIMEOUT 30
        )
        string(REPLACE "header" "metadata" _mpolygon_metadata_mode "${_mpolygon_generate_mode}")
        add_test(
            NAME dwg2dxf_mpolygon_metadata_${_mpolygon_encoding}_cli
            COMMAND ${CMAKE_COMMAND}
                "-DADAPTER_TEST=$<TARGET_FILE:libdxfrw_dwg_local_roundtrip>"
                "-DDWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
                "-DCASE_NAME=mpolygon-metadata-${_mpolygon_encoding}"
                "-DGENERATE_MODE=${_mpolygon_metadata_mode}"
                "-DVERIFY_MODE=--verify-mpolygon-metadata"
                "-DCONVERSION_OPTIONS=${_mpolygon_conversion_options}"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg2dxf_image_path_test.cmake
        )
        set_tests_properties(dwg2dxf_mpolygon_metadata_${_mpolygon_encoding}_cli PROPERTIES
            LABELS "cli;fast;dxf;adapter;mpolygon"
            TIMEOUT 30
        )
    endforeach()
endif()

add_test(
    NAME dxf_mpolygon_metadata_boundary_roundtrip
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip> --mpolygon-metadata
)
set_tests_properties(dxf_mpolygon_metadata_boundary_roundtrip PROPERTIES
    LABELS "fast;dxf;adapter;mpolygon"
    TIMEOUT 30
)

# Typed ACSH BOX export may only project the indexed CMC forms established by
# its DXF mapping; unsupported method/flag/reserved-index metadata must refuse
# atomically before replacing an existing destination.
add_test(
    NAME dxf_acsh_cmc_loss_guard
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
            --acsh-cmc-loss-guard
)
set_tests_properties(dxf_acsh_cmc_loss_guard PROPERTIES
    LABELS "fast;dxf;dwg;adapter;3d;loss-guard"
    TIMEOUT 30
)

# Preserve the PFACE record boundary and following entity through direct
# ASCII/Binary read and canonical write/readback without running the full suite.
add_test(
    NAME dxf_polyface_completed_pool
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip> --polyface-completed-pool
)
set_tests_properties(dxf_polyface_completed_pool PROPERTIES
    LABELS "fast;dxf;adapter;polyface"
    TIMEOUT 30
)

# Verify the BLOCK_RECORD-to-LAYOUT hard-pointer slot across the supported
# writer version families without running the full round-trip executable.
add_test(
    NAME dwg_block_record_layout_association
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
            --block-record-layout-association
)
set_tests_properties(dwg_block_record_layout_association PROPERTIES
    LABELS "fast;dwg;writer;layout"
    TIMEOUT 30
)

# Unsupported DWG TABLE entities must not be serialized through the inherited
# INSERT encoder; ensure refusal is atomic across supported writer families.
add_test(
    NAME dwg_table_writer_refusal
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
            --dwg-table-writer-refusal
)
set_tests_properties(dwg_table_writer_refusal PROPERTIES
    LABELS "fast;dwg;writer;tables"
    TIMEOUT 30
)

# Generated HATCH/MPOLYGON gradient CMC controls exercise direct reader
# alignment, DWG writer/readback, and opaque same-version preservation without
# running the broad local round-trip executable.
add_test(
    NAME dwg_hatch_gradient_cmc
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
            --hatch-gradient-cmc
)
set_tests_properties(dwg_hatch_gradient_cmc PROPERTIES
    LABELS "fast;dwg;reader;writer;hatch"
    TIMEOUT 30
)

add_test(
    NAME dwg_layout_compound_insert_owner_roundtrip
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
            --layout-plot-dpi-read-controls
)
set_tests_properties(dwg_layout_compound_insert_owner_roundtrip PROPERTIES
    LABELS "fast;dwg;writer;reader;layout;attrib"
    TIMEOUT 30
)

# Exercise record-level read recovery without running the broad local
# round-trip executable. The generated non-finite LINE is rejected while the
# following valid LINE remains readable and produces finalized partial coverage.
add_test(
    NAME libdxfrw_dwg_read_outcome_policy
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip> --dwg-read-outcome
)
set_tests_properties(libdxfrw_dwg_read_outcome_policy PROPERTIES
    LABELS "fast;dwg;reader;read-outcome"
    TIMEOUT 30
)

if(TARGET dwg2dxf)
    foreach(_dimension_encoding IN ITEMS ascii binary)
        set(_dimension_generate_mode --generate-dimension-frames)
        set(_dimension_conversion_options "")
        if(_dimension_encoding STREQUAL "binary")
            set(_dimension_generate_mode --generate-dimension-frames-binary)
            set(_dimension_conversion_options -b)
        endif()
        add_test(
            NAME dwg2dxf_dimension_frames_${_dimension_encoding}_cli
            COMMAND ${CMAKE_COMMAND}
                "-DADAPTER_TEST=$<TARGET_FILE:libdxfrw_dwg_local_roundtrip>"
                "-DDWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
                "-DCASE_NAME=dimension-frames-${_dimension_encoding}"
                "-DGENERATE_MODE=${_dimension_generate_mode}"
                "-DVERIFY_MODE=--verify-dimension-frames"
                "-DCONVERSION_OPTIONS=${_dimension_conversion_options}"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg2dxf_image_path_test.cmake
        )
        set_tests_properties(dwg2dxf_dimension_frames_${_dimension_encoding}_cli PROPERTIES
            LABELS "cli;fast;dxf;adapter;dimension"
            TIMEOUT 30
        )
    endforeach()

    foreach(_mtext_encoding IN ITEMS ascii binary)
        set(_mtext_generate_mode --generate-mtext-orientation)
        set(_mtext_conversion_options "")
        if(_mtext_encoding STREQUAL "binary")
            set(_mtext_generate_mode --generate-mtext-orientation-binary)
            set(_mtext_conversion_options -b)
        endif()
        add_test(
            NAME dwg2dxf_mtext_orientation_${_mtext_encoding}_cli
            COMMAND ${CMAKE_COMMAND}
                "-DADAPTER_TEST=$<TARGET_FILE:libdxfrw_dxf_fixture_tests>"
                "-DDWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
                "-DCASE_NAME=mtext-orientation-${_mtext_encoding}"
                "-DGENERATE_MODE=${_mtext_generate_mode}"
                "-DVERIFY_MODE=--verify-mtext-orientation"
                "-DCONVERSION_OPTIONS=${_mtext_conversion_options}"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg2dxf_image_path_test.cmake
        )
        set_tests_properties(dwg2dxf_mtext_orientation_${_mtext_encoding}_cli PROPERTIES
            LABELS "cli;fast;dxf;adapter;mtext"
            TIMEOUT 30
        )
    endforeach()

    foreach(_arc_encoding IN ITEMS ascii binary)
        set(_arc_generate_mode --generate-arc-dimension-flags)
        set(_arc_conversion_options "")
        if(_arc_encoding STREQUAL "binary")
            set(_arc_generate_mode --generate-arc-dimension-flags-binary)
            set(_arc_conversion_options -b)
        endif()
        add_test(
            NAME dwg2dxf_arc_dimension_flags_${_arc_encoding}_cli
            COMMAND ${CMAKE_COMMAND}
                "-DADAPTER_TEST=$<TARGET_FILE:libdxfrw_dwg_local_roundtrip>"
                "-DDWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
                "-DCASE_NAME=arc-dimension-flags-${_arc_encoding}"
                "-DGENERATE_MODE=${_arc_generate_mode}"
                "-DVERIFY_MODE=--verify-arc-dimension-flags"
                "-DCONVERSION_OPTIONS=${_arc_conversion_options}"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg2dxf_image_path_test.cmake
        )
        set_tests_properties(dwg2dxf_arc_dimension_flags_${_arc_encoding}_cli PROPERTIES
            LABELS "cli;fast;dxf;adapter;dimension"
            TIMEOUT 30
        )
    endforeach()

    # Match FreeCAD's Draft/importDWG.py subprocess contract without requiring a
    # FreeCAD installation. The DWG fixture is already tracked in this repository.
    set(_freecad_cli_sample
        ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/ordinary_enc_AC1027.dwg
    )
    set(_freecad_cli_rtext_sample
        ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/rtext_arctext.dwg
    )
    set(_freecad_cli_mpolygon_sample
        ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/mpolygon_solid.dwg
    )
    if(EXISTS "${_freecad_cli_sample}"
            AND EXISTS "${_freecad_cli_rtext_sample}"
            AND EXISTS "${_freecad_cli_mpolygon_sample}")
        add_test(
            NAME dwg2dxf_freecad_cli_compat
            COMMAND ${CMAKE_COMMAND}
                -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
                -DSAMPLE=${_freecad_cli_sample}
                -DRTEXT_SAMPLE=${_freecad_cli_rtext_sample}
                -DMPOLYGON_SAMPLE=${_freecad_cli_mpolygon_sample}
                "-DPYTHON=${LIBDXFRW_PYTHON_EXECUTABLE}"
                -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_dwg2dxf_compat_test.cmake
        )
        set_tests_properties(dwg2dxf_freecad_cli_compat PROPERTIES
            LABELS "dwg_read;freecad;cli;fast"
        )

        # Exercise the artifact FreeCAD actually launches, not just the build-tree
        # executable. Keep this test inside the chosen install prefix and skip
        # non-relocatable GNUInstallDirs layouts rather than risking a host install.
        set(_freecad_install_dirs_safe TRUE)
        foreach(_install_dir IN ITEMS
                "${CMAKE_INSTALL_BINDIR}" "${CMAKE_INSTALL_LIBDIR}")
            if(IS_ABSOLUTE "${_install_dir}"
                    OR "${_install_dir}" MATCHES "(^|/)\\.\\.(/|$)")
                set(_freecad_install_dirs_safe FALSE)
            endif()
        endforeach()
        if(_freecad_install_dirs_safe)
            add_test(
                NAME dwg2dxf_freecad_installed_cli_compat
                COMMAND ${CMAKE_COMMAND}
                    "-DBUILD_DIR=${CMAKE_BINARY_DIR}"
                    "-DBUILD_DWG2DXF=$<TARGET_FILE:dwg2dxf>"
                    "-DINSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}"
                    "-DINSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}"
                    "-DEXECUTABLE_SUFFIX=${CMAKE_EXECUTABLE_SUFFIX}"
                    "-DSAMPLE=${_freecad_cli_sample}"
                    "-DRTEXT_SAMPLE=${_freecad_cli_rtext_sample}"
                    "-DMPOLYGON_SAMPLE=${_freecad_cli_mpolygon_sample}"
                    "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
                    "-DCMAKE_CONFIG=$<CONFIG>"
                    -P "${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_installed_dwg2dxf_test.cmake"
            )
            set_tests_properties(dwg2dxf_freecad_installed_cli_compat PROPERTIES
                LABELS "freecad;cli;fast;installed"
                TIMEOUT 180
            )
        else()
            message(STATUS
                "Skipping installed FreeCAD CLI test: GNUInstallDirs destinations are absolute or escape the install prefix")
        endif()
    endif()

    # The core codec can replay raw sections, but dwg2dxf must also retain them
    # through its concrete DXF import/export adapter. Generate the tiny opaque
    # ACDSDATA-shaped control in the build tree; it is not a schema-validity oracle.
    add_test(
        NAME dwg2dxf_raw_dxf_section_passthrough
        COMMAND ${CMAKE_COMMAND}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg2dxf_raw_dxf_section_test.cmake
    )
    set_tests_properties(dwg2dxf_raw_dxf_section_passthrough PROPERTIES
        LABELS "cli;fast;dxf"
        TIMEOUT 20
    )
endif()

add_test(
    NAME libdxfrw_image_definition_plan_failure_retry
    COMMAND libdxfrw_dxf_fixture_tests --image-definition-plan-failure-retry
)
set_tests_properties(libdxfrw_image_definition_plan_failure_retry PROPERTIES
    LABELS "fast;dxf;image;writer"
    TIMEOUT 20
)

# Optional test of the installed converter from FreeCAD's actual desktop
# process. It uses only a tracked ordinary DWG and is deliberately disabled
# by default; the normal inner loop remains the fast, FreeCAD-independent CLI
# contract test above.
option(LIBDXFRW_ENABLE_FREECAD_DESKTOP_CONTROL
    "Enable the opt-in FreeCAD GUI dwg2dxf open/insert smoke" OFF)
set(LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE "" CACHE FILEPATH
    "Installed dwg2dxf executable to qualify inside FreeCAD")
set(LIBDXFRW_FREECAD_GUI_EXECUTABLE "" CACHE FILEPATH
    "FreeCAD GUI executable (required outside macOS)")
set(LIBDXFRW_FREECAD_APP_BUNDLE "" CACHE PATH
    "FreeCAD .app bundle path (required on macOS)")
if(LIBDXFRW_ENABLE_FREECAD_DESKTOP_CONTROL)
    libdxfrw_require_test_cli(LIBDXFRW_ENABLE_FREECAD_DESKTOP_CONTROL)
    if(NOT EXISTS "${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}")
        message(FATAL_ERROR
            "LIBDXFRW_ENABLE_FREECAD_DESKTOP_CONTROL requires the installed "
            "LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE path")
    endif()
    if(APPLE)
        if(NOT IS_DIRECTORY "${LIBDXFRW_FREECAD_APP_BUNDLE}")
            message(FATAL_ERROR
                "On macOS, set LIBDXFRW_FREECAD_APP_BUNDLE to FreeCAD.app")
        endif()
    elseif(NOT EXISTS "${LIBDXFRW_FREECAD_GUI_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECAD_GUI_EXECUTABLE to FreeCAD's desktop executable")
    endif()
    add_test(
        NAME dwg2dxf_freecad_desktop_open_insert
        COMMAND ${CMAKE_COMMAND}
            -DFREECAD_DWG2DXF_EXECUTABLE=${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}
            -DBUILD_DWG2DXF_EXECUTABLE=$<TARGET_FILE:dwg2dxf>
            -DSAMPLE=${_freecad_cli_sample}
            -DIMPORT_CHECK_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_dwg2dxf_import_check.FCMacro
            -DDESKTOP_CHECK_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_dwg2dxf_desktop_check.FCMacro
            -DFREECAD_GUI_EXECUTABLE=${LIBDXFRW_FREECAD_GUI_EXECUTABLE}
            -DFREECAD_APP_BUNDLE=${LIBDXFRW_FREECAD_APP_BUNDLE}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_desktop_dwg_import_check.cmake
    )
    set_tests_properties(dwg2dxf_freecad_desktop_open_insert PROPERTIES
        LABELS "freecad;desktop;external;integration"
        TIMEOUT 240
    )
endif()

# Optional pinned FreeCAD runtime for the external AC1015 SAT-v1 witness.
# Reuses the external sample/reference configured for the CLI comparison and
# the existing import macro; never downloads or vendors a DWG/DXF fixture.
option(LIBDXFRW_ENABLE_FREECAD_SAT_V1_RUNTIME_CONTROL
    "Enable the opt-in FreeCAD SAT-v1 open/insert handoff check" OFF)
set(LIBDXFRW_FREECADCMD_EXECUTABLE "" CACHE FILEPATH
    "FreeCAD freecadcmd executable for the SAT-v1 runtime control")
if(LIBDXFRW_ENABLE_FREECAD_SAT_V1_RUNTIME_CONTROL)
    libdxfrw_require_test_cli(LIBDXFRW_ENABLE_FREECAD_SAT_V1_RUNTIME_CONTROL)
    if(NOT LIBDXFRW_ENABLE_EXTERNAL_SAT_V1_CLI_CONTROL)
        message(FATAL_ERROR
            "The FreeCAD SAT-v1 runtime control also requires "
            "LIBDXFRW_ENABLE_EXTERNAL_SAT_V1_CLI_CONTROL=ON and its external "
            "DWG/reference paths")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECADCMD_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECADCMD_EXECUTABLE to FreeCAD's freecadcmd")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE to the installed "
            "libdxfrw dwg2dxf binary, not the build-tree target")
    endif()
    foreach(_operation IN ITEMS open insert)
        add_test(
            NAME dwg2dxf_freecad_sat_v1_${_operation}
            COMMAND ${CMAKE_COMMAND}
                "-DFREECADCMD=${LIBDXFRW_FREECADCMD_EXECUTABLE}"
                "-DDWG2DXF=${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}"
                "-DBUILD_DWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DSAMPLE=${LIBDXFRW_SAT_V1_DWG}"
                "-DREFERENCE_DXF=${LIBDXFRW_SAT_V1_REFERENCE_DXF}"
                "-DCOMPARATOR=$<TARGET_FILE:libdxfrw_modeler_sat_dxf_compare>"
                "-DIMPORT_CHECK_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_dwg2dxf_import_check.FCMacro"
                "-DOPERATION=${_operation}"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_sat_v1_runtime_test.cmake.in
        )
        set_tests_properties(dwg2dxf_freecad_sat_v1_${_operation}
            PROPERTIES
                LABELS "freecad;modeler;external;integration"
                TIMEOUT 240)
    endforeach()
endif()

# External AC1027 ACDSDATA handoff qualification. The caller supplies the DWG,
# installed converter, its expected digest, and a pinned FreeCAD 1.1.3 runtime.
# No drawing or generated output is added to the source tree. This deliberately
# checks converter handoff plus FreeCAD's current unsupported-3DSOLID outcome,
# not FreeCAD B-rep support. An optional libdxfrw DXF baseline enables the
# stricter, separately maintained ACDSDATA/proxy/SAB round-trip checker.
option(LIBDXFRW_ENABLE_FREECAD_ACDS_RUNTIME_CONTROL
    "Enable the opt-in external AC1027 ACDSDATA FreeCAD open/insert handoff check" OFF)
set(LIBDXFRW_ACDS_DWG "" CACHE FILEPATH
    "Caller-supplied AC1027 DWG with one 3DSOLID and ACDSDATA")
set(LIBDXFRW_ACDS_INSTALLED_DWG2DXF "" CACHE FILEPATH
    "Installed libdxfrw dwg2dxf executable to qualify in FreeCAD")
set(LIBDXFRW_ACDS_INSTALLED_SHA256 "" CACHE STRING
    "Expected SHA-256 of the installed ACDSDATA dwg2dxf executable")
set(LIBDXFRW_ACDS_FREECAD_REVISION "" CACHE STRING
    "Exact FreeCAD 1.1.3 App.Version()[7] revision to qualify")
set(LIBDXFRW_ACDS_BASELINE_DXF "" CACHE FILEPATH
    "Optional caller-supplied libdxfrw AC1027 DXF for strict ACDSDATA comparison")
if(LIBDXFRW_ENABLE_FREECAD_ACDS_RUNTIME_CONTROL)
    libdxfrw_require_test_cli(LIBDXFRW_ENABLE_FREECAD_ACDS_RUNTIME_CONTROL)
    foreach(_acds_path IN ITEMS LIBDXFRW_ACDS_DWG
            LIBDXFRW_ACDS_INSTALLED_DWG2DXF LIBDXFRW_FREECADCMD_EXECUTABLE)
        if(NOT EXISTS "${${_acds_path}}")
            message(FATAL_ERROR
                "FreeCAD ACDSDATA control requires existing ${_acds_path}")
        endif()
    endforeach()
    string(LENGTH "${LIBDXFRW_ACDS_INSTALLED_SHA256}" _acds_sha_length)
    if(NOT _acds_sha_length EQUAL 64
            OR NOT LIBDXFRW_ACDS_INSTALLED_SHA256 MATCHES "^[0-9a-fA-F]+$")
        message(FATAL_ERROR
            "Set LIBDXFRW_ACDS_INSTALLED_SHA256 to the installed binary's exact SHA-256")
    endif()
    string(LENGTH "${LIBDXFRW_ACDS_FREECAD_REVISION}" _acds_revision_length)
    if(NOT _acds_revision_length EQUAL 40
            OR NOT LIBDXFRW_ACDS_FREECAD_REVISION MATCHES "^[0-9a-fA-F]+$")
        message(FATAL_ERROR
            "Set LIBDXFRW_ACDS_FREECAD_REVISION to the pinned FreeCAD 1.1.3 Git revision")
    endif()
    if(LIBDXFRW_ACDS_BASELINE_DXF)
        if(NOT EXISTS "${LIBDXFRW_ACDS_BASELINE_DXF}")
            message(FATAL_ERROR
                "LIBDXFRW_ACDS_BASELINE_DXF must name an existing external DXF")
        endif()
        find_package(Python3 COMPONENTS Interpreter REQUIRED)
    endif()
    foreach(_operation IN ITEMS open insert)
        add_test(
            NAME dwg2dxf_freecad_acds_${_operation}_handoff
            COMMAND ${CMAKE_COMMAND}
                "-DFREECADCMD=${LIBDXFRW_FREECADCMD_EXECUTABLE}"
                "-DDWG2DXF=${LIBDXFRW_ACDS_INSTALLED_DWG2DXF}"
                "-DBUILD_DWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DSAMPLE=${LIBDXFRW_ACDS_DWG}"
                "-DEXPECTED_SHA256=${LIBDXFRW_ACDS_INSTALLED_SHA256}"
                "-DFREECAD_REVISION=${LIBDXFRW_ACDS_FREECAD_REVISION}"
                "-DBASELINE_DXF=${LIBDXFRW_ACDS_BASELINE_DXF}"
                "-DPYTHON=${Python3_EXECUTABLE}"
                "-DACDS_CHECKER=${CMAKE_CURRENT_SOURCE_DIR}/check_acds_roundtrip.py"
                "-DIMPORT_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_dwg2dxf_acds_check.FCMacro"
                "-DOPERATION=${_operation}"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_acds_runtime_test.cmake
        )
        set_tests_properties(dwg2dxf_freecad_acds_${_operation}_handoff
            PROPERTIES
                LABELS "freecad;acds;AC1027;external;integration"
                TIMEOUT 240)
    endforeach()
endif()

# Optional failure-path qualification through FreeCAD's real Draft wrapper.
# FreeCAD currently treats output-file existence as conversion success, so this
# confirms that a failing installed converter cannot hand off a partial DXF.
# The malformed DWG is created locally beneath system temp; FreeCAD stays out
# of default CI.
option(LIBDXFRW_ENABLE_FREECAD_FAILURE_CONTROL
    "Enable the opt-in FreeCAD DWG conversion failure-path check" OFF)
if(LIBDXFRW_ENABLE_FREECAD_FAILURE_CONTROL)
    libdxfrw_require_test_cli(LIBDXFRW_ENABLE_FREECAD_FAILURE_CONTROL)
    if(NOT EXISTS "${LIBDXFRW_FREECADCMD_EXECUTABLE}")
        message(FATAL_ERROR
            "LIBDXFRW_ENABLE_FREECAD_FAILURE_CONTROL requires "
            "LIBDXFRW_FREECADCMD_EXECUTABLE")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}")
        message(FATAL_ERROR
            "LIBDXFRW_ENABLE_FREECAD_FAILURE_CONTROL requires the installed "
            "LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE")
    endif()
    add_test(
        NAME dwg2dxf_freecad_failed_conversion
        COMMAND ${CMAKE_COMMAND}
            "-DFREECADCMD=${LIBDXFRW_FREECADCMD_EXECUTABLE}"
            "-DDWG2DXF=${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}"
            "-DBUILD_DWG2DXF=$<TARGET_FILE:dwg2dxf>"
            "-DIMPORT_CHECK_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_dwg2dxf_import_check.FCMacro"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_dwg2dxf_failure_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_failed_conversion PROPERTIES
        LABELS "freecad;external;integration"
        TIMEOUT 120
    )
endif()

# Bridge the positive nonzero-Z FreeCAD geometry control through the installed
# artifact. The ordinary fast CLI checks above remain dependency-free.
option(LIBDXFRW_ENABLE_FREECAD_INSTALLED_3D_LINE_CONTROL
    "Enable installed dwg2dxf FreeCAD 3D LINE open/insert checks" OFF)
if(LIBDXFRW_ENABLE_FREECAD_INSTALLED_3D_LINE_CONTROL)
    libdxfrw_require_test_cli(LIBDXFRW_ENABLE_FREECAD_INSTALLED_3D_LINE_CONTROL)
    find_program(LIBDXFRW_DWGADD_EXECUTABLE NAMES dwgadd)
    if(NOT LIBDXFRW_DWGADD_EXECUTABLE)
        message(FATAL_ERROR
            "LIBDXFRW_ENABLE_FREECAD_INSTALLED_3D_LINE_CONTROL requires LibreDWG dwgadd")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECADCMD_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECADCMD_EXECUTABLE to FreeCAD's freecadcmd")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE to the installed dwg2dxf")
    endif()

    foreach(_operation IN ITEMS open insert)
        add_test(
            NAME dwg2dxf_freecad_installed_3d_line_${_operation}
            COMMAND ${CMAKE_COMMAND}
                "-DFREECADCMD=${LIBDXFRW_FREECADCMD_EXECUTABLE}"
                "-DDWG2DXF=${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}"
                "-DBUILD_DWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DDWGADD=${LIBDXFRW_DWGADD_EXECUTABLE}"
                "-DRECIPE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/ac1015_3d_line_control.dwgadd"
                "-DIMPORT_CHECK_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_dwg2dxf_import_check.FCMacro"
                "-DOPERATION=${_operation}"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_installed_3d_line_test.cmake
        )
        set_tests_properties(dwg2dxf_freecad_installed_3d_line_${_operation}
            PROPERTIES
                LABELS "freecad;installed;generated;integration"
                TIMEOUT 180)
    endforeach()
endif()

# Native Linux/Windows qualification can reuse the exact LibreCAD-tracked
# AC1027 three-LINE fixture. This avoids requiring an external dwgadd build or
# adding another DWG to the repository. Keep it opt-in: it launches the pinned
# FreeCAD consumer twice (open and insert) and is intended for manual platform
# qualification, not every fast CI run.
option(LIBDXFRW_ENABLE_FREECAD_TRACKED_AC1027_LINE_CONTROL
    "Enable pinned FreeCAD checks using the tracked AC1027 three-LINE DWG" OFF)
if(LIBDXFRW_ENABLE_FREECAD_TRACKED_AC1027_LINE_CONTROL)
    libdxfrw_require_test_cli(LIBDXFRW_ENABLE_FREECAD_TRACKED_AC1027_LINE_CONTROL)
    if(NOT EXISTS "${LIBDXFRW_FREECADCMD_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECADCMD_EXECUTABLE to FreeCAD's FreeCADCmd/freecadcmd")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE to the installed dwg2dxf")
    endif()
    if(NOT EXISTS "${_freecad_cli_sample}")
        message(FATAL_ERROR
            "The tracked ordinary_enc_AC1027.dwg fixture is required")
    endif()

    foreach(_operation IN ITEMS open insert)
        add_test(
            NAME dwg2dxf_freecad_tracked_ac1027_line_${_operation}
            COMMAND ${CMAKE_COMMAND}
                "-DFREECADCMD=${LIBDXFRW_FREECADCMD_EXECUTABLE}"
                "-DDWG2DXF=${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}"
                "-DBUILD_DWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DSAMPLE=${_freecad_cli_sample}"
                -DEXPECTED_INPUT_SHA256=a0ebf245e570bf0dc337c696330b7ea883feaebb7c4e781de8d77ac723815f83
                -DEXPECTED_MAGIC_HEX=414331303237
                "-DIMPORT_CHECK_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_dwg2dxf_import_check.FCMacro"
                "-DOPERATION=${_operation}"
                -DINPUT_PATH_UNICODE=ON
                -DEXPECTED_FREECAD_REVISION=145529fe741292ff0b3977a01195bf0247425794
                -DEXPECTED_LINE_BOUNDS=[[1,2,0,3,4,0],[5,6,0,7,8,0],[9,10,0,11,12,0]]
                -DEXPECTED_LINE_ENDPOINTS=[[[1,2,0],[3,4,0]],[[5,6,0],[7,8,0]],[[9,10,0],[11,12,0]]]
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_installed_3d_line_test.cmake
        )
        set_tests_properties(dwg2dxf_freecad_tracked_ac1027_line_${_operation}
            PROPERTIES
                LABELS "freecad;installed;generated;integration;AC1027;platform"
                TIMEOUT 180)
    endforeach()
endif()

# Exercise the current shared build after CMake installs it into a relocatable
# prefix, then reuse the installed-artifact FreeCAD geometry harness. CMake's
# install step rewrites the executable RPATH, so this must be a distinct
# attribution path from the byte-identical static/install test above.
option(LIBDXFRW_ENABLE_FREECAD_SHARED_INSTALLED_3D_LINE_CONTROL
    "Enable FreeCAD open/insert checks against this shared build's isolated install" OFF)
if(LIBDXFRW_ENABLE_FREECAD_SHARED_INSTALLED_3D_LINE_CONTROL)
    libdxfrw_require_test_cli(LIBDXFRW_ENABLE_FREECAD_SHARED_INSTALLED_3D_LINE_CONTROL)
    if(NOT BUILD_SHARED_LIBS OR NOT UNIX)
        message(FATAL_ERROR
            "LIBDXFRW_ENABLE_FREECAD_SHARED_INSTALLED_3D_LINE_CONTROL requires a shared Unix build")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECADCMD_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECADCMD_EXECUTABLE to FreeCAD's freecadcmd")
    endif()
    find_program(LIBDXFRW_DWGADD_EXECUTABLE NAMES dwgadd)
    if(NOT LIBDXFRW_DWGADD_EXECUTABLE)
        message(FATAL_ERROR
            "LIBDXFRW_ENABLE_FREECAD_SHARED_INSTALLED_3D_LINE_CONTROL requires LibreDWG dwgadd")
    endif()

    foreach(_operation IN ITEMS open insert)
        add_test(
            NAME dwg2dxf_freecad_shared_installed_3d_line_${_operation}
            COMMAND ${CMAKE_COMMAND}
                "-DBUILD_DIR=${CMAKE_BINARY_DIR}"
                "-DBUILD_DWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DFREECADCMD=${LIBDXFRW_FREECADCMD_EXECUTABLE}"
                "-DDWGADD=${LIBDXFRW_DWGADD_EXECUTABLE}"
                "-DRECIPE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/ac1015_3d_line_control.dwgadd"
                "-DIMPORT_CHECK_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_dwg2dxf_import_check.FCMacro"
                "-DOPERATION=${_operation}"
                "-DINSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}"
                "-DINSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}"
                "-DEXECUTABLE_SUFFIX=${CMAKE_EXECUTABLE_SUFFIX}"
                "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
                "-DCMAKE_CONFIG=$<CONFIG>"
                -DBUILD_SHARED_LIBS=ON
                -P "${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_shared_installed_3d_line_test.cmake"
        )
        set_tests_properties(dwg2dxf_freecad_shared_installed_3d_line_${_operation}
            PROPERTIES
                LABELS "freecad;shared;installed;generated;integration"
                TIMEOUT 300)
    endforeach()
endif()

# Qualify the ordinary FreeCAD Automatic preference separately. Unlike the
# LibreDWG-only controls above, keep real ODA/QCAD resolvers enabled and assert
# that the installed dwg2dxf succeeds before either fallback is launched.
option(LIBDXFRW_ENABLE_FREECAD_AUTOMATIC_3D_LINE_CONTROL
    "Enable FreeCAD Automatic-mode dwg2dxf 3D LINE open/insert checks" OFF)
if(LIBDXFRW_ENABLE_FREECAD_AUTOMATIC_3D_LINE_CONTROL)
    libdxfrw_require_test_cli(LIBDXFRW_ENABLE_FREECAD_AUTOMATIC_3D_LINE_CONTROL)
    find_program(LIBDXFRW_DWGADD_EXECUTABLE NAMES dwgadd)
    if(NOT LIBDXFRW_DWGADD_EXECUTABLE)
        message(FATAL_ERROR
            "LIBDXFRW_ENABLE_FREECAD_AUTOMATIC_3D_LINE_CONTROL requires LibreDWG dwgadd")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECADCMD_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECADCMD_EXECUTABLE to FreeCAD's freecadcmd")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE to the installed dwg2dxf")
    endif()

    foreach(_operation IN ITEMS open insert)
        add_test(
            NAME dwg2dxf_freecad_automatic_3d_line_${_operation}
            COMMAND ${CMAKE_COMMAND}
                "-DFREECADCMD=${LIBDXFRW_FREECADCMD_EXECUTABLE}"
                "-DDWG2DXF=${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}"
                "-DBUILD_DWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DDWGADD=${LIBDXFRW_DWGADD_EXECUTABLE}"
                "-DRECIPE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/ac1015_3d_line_control.dwgadd"
                "-DIMPORT_CHECK_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_dwg2dxf_import_check.FCMacro"
                "-DOPERATION=${_operation}"
                "-DDWG_CONVERSION_MODE=0"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_installed_3d_line_test.cmake
        )
        set_tests_properties(dwg2dxf_freecad_automatic_3d_line_${_operation}
            PROPERTIES
                LABELS "freecad;automatic;installed;generated;integration"
                TIMEOUT 180)
    endforeach()
endif()

# Optional AC1027 POINT qualification using a caller-supplied AutoCAD 2016
# sample. The DWG stays outside this repository; only test code is committed.
option(LIBDXFRW_ENABLE_FREECAD_AUTOCAD_POINT3D_CONTROL
    "Enable the external AutoCAD AC1027 3D POINT FreeCAD control" OFF)
set(LIBDXFRW_AUTOCAD_POINT3D_DWG "" CACHE FILEPATH
    "External AutoCAD 2016 AC1027 DWG containing POINT (50,50,50)")
if(LIBDXFRW_ENABLE_FREECAD_AUTOCAD_POINT3D_CONTROL)
    libdxfrw_require_test_cli(LIBDXFRW_ENABLE_FREECAD_AUTOCAD_POINT3D_CONTROL)
    if(NOT EXISTS "${LIBDXFRW_AUTOCAD_POINT3D_DWG}")
        message(FATAL_ERROR
            "Set LIBDXFRW_AUTOCAD_POINT3D_DWG to the external point3d_2013.dwg")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECADCMD_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECADCMD_EXECUTABLE to FreeCAD's freecadcmd")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE to the installed dwg2dxf")
    endif()
    add_test(
        NAME dwg2dxf_freecad_autocad_ac1027_point3d
        COMMAND ${CMAKE_COMMAND}
            "-DFREECADCMD=${LIBDXFRW_FREECADCMD_EXECUTABLE}"
            "-DDWG2DXF=${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}"
            "-DBUILD_DWG2DXF=$<TARGET_FILE:dwg2dxf>"
            "-DSAMPLE=${LIBDXFRW_AUTOCAD_POINT3D_DWG}"
            "-DEXPECTED_SHA256=839297daa100c318d5afbfa128145d2c4782300650c86b2ab58b34cf6fe1aba4"
            "-DIMPORT_CHECK_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_dwg2dxf_3d_point_check.FCMacro"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_autocad_point3d_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_autocad_ac1027_point3d PROPERTIES
        LABELS "freecad;external;autocad;AC1027;integration"
        TIMEOUT 180)
endif()

# Optional locally generated DWG control for the FreeCAD nonzero-Z path.
# LibreDWG dwgadd and FreeCAD remain external integration tools, not required
# dependencies of the normal build/test configuration.
option(LIBDXFRW_ENABLE_DWGADD_FREECAD_CONTROL
    "Enable the LibreDWG-generated FreeCAD 3D LINE converter control" OFF)
if(LIBDXFRW_ENABLE_DWGADD_FREECAD_CONTROL)
    libdxfrw_require_test_cli(LIBDXFRW_ENABLE_DWGADD_FREECAD_CONTROL)
    find_program(LIBDXFRW_DWGADD_EXECUTABLE NAMES dwgadd)
    if(NOT LIBDXFRW_DWGADD_EXECUTABLE)
        message(FATAL_ERROR
            "LIBDXFRW_ENABLE_DWGADD_FREECAD_CONTROL requires LibreDWG dwgadd")
    endif()
    add_test(
        NAME dwg2dxf_freecad_3d_line_cli
        COMMAND ${CMAKE_COMMAND}
            -DDWGADD=${LIBDXFRW_DWGADD_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            -DRECIPE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/ac1015_3d_line_control.dwgadd
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3d_line_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_line_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated")
    add_test(
        NAME dwg2dxf_freecad_3d_point_cli
        COMMAND ${CMAKE_COMMAND}
            -DDWGADD=${LIBDXFRW_DWGADD_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            -DRECIPE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/ac1015_3d_point_freecad_control.dwgadd
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3d_point_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_point_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated")
    add_test(
        NAME dwg2dxf_freecad_3d_arc_circle_cli
        COMMAND ${CMAKE_COMMAND}
            -DDWGADD=${LIBDXFRW_DWGADD_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            -DRECIPE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/ac1015_arc_circle_freecad_control.dwgadd
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3d_arc_circle_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_arc_circle_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated")
    add_test(
        NAME dwg2dxf_freecad_3d_ellipse_cli
        COMMAND ${CMAKE_COMMAND}
            -DDWGADD=${LIBDXFRW_DWGADD_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            -DRECIPE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/ac1015_ellipse_freecad_control.dwgadd
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3d_ellipse_dwggenerated_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_ellipse_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated")
    add_test(
        NAME dwg2dxf_freecad_3d_polyline_cli
        COMMAND ${CMAKE_COMMAND}
            -DDWGADD=${LIBDXFRW_DWGADD_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            -DRECIPE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/ac1015_3d_polyline_freecad_control.dwgadd
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3d_polyline_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_polyline_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated")
    foreach(_classic_family mesh pface pface_multiface)
        add_test(
            NAME dwg2dxf_freecad_3d_${_classic_family}_cli
            COMMAND ${CMAKE_COMMAND}
                -DDWGADD=${LIBDXFRW_DWGADD_EXECUTABLE}
                -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
                -DRECIPE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/ac1015_${_classic_family}_freecad_control.dwgadd
                -DFAMILY=${_classic_family}
                -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_classic_polyline_cli_test.cmake
        )
        set_tests_properties(dwg2dxf_freecad_3d_${_classic_family}_cli PROPERTIES
            LABELS "dwg_read;freecad;cli;fast;generated")
    endforeach()
endif()

# Optional independent-writer DWG controls for FreeCAD entity routes.
# DXF sources are locally authored; ODA and FreeCAD remain external tools and
# are not dependencies of the normal build/test configuration.
option(LIBDXFRW_ENABLE_ODAFILECONVERTER_FREECAD_CONTROL
    "Enable ODA-generated FreeCAD 3D converter controls" OFF)
if(LIBDXFRW_ENABLE_ODAFILECONVERTER_FREECAD_CONTROL)
    libdxfrw_require_test_cli(LIBDXFRW_ENABLE_ODAFILECONVERTER_FREECAD_CONTROL)
    find_program(LIBDXFRW_ODAFILECONVERTER_EXECUTABLE NAMES ODAFileConverter)
    find_program(LIBDXFRW_LIBREDWG_DWGREAD_EXECUTABLE NAMES dwgread)
    if(NOT LIBDXFRW_ODAFILECONVERTER_EXECUTABLE)
        message(FATAL_ERROR
            "LIBDXFRW_ENABLE_ODAFILECONVERTER_FREECAD_CONTROL requires ODAFileConverter; "
            "set LIBDXFRW_ODAFILECONVERTER_EXECUTABLE to its path")
    endif()
    add_test(
        NAME dwg2dxf_freecad_3d_insert_cli
        COMMAND ${CMAKE_COMMAND}
            -DODAFILECONVERTER=${LIBDXFRW_ODAFILECONVERTER_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            -DSOURCE_DXF=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dxf/ac1015_insert_freecad_control.dxf
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3d_insert_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_insert_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated;oda")
    add_test(
        NAME dwg2dxf_freecad_3d_minsert_cli
        COMMAND ${CMAKE_COMMAND}
            -DODAFILECONVERTER=${LIBDXFRW_ODAFILECONVERTER_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            "-DSOURCE_DXF=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dxf/ac1015_minsert_freecad_control.dxf"
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3d_minsert_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_minsert_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated;oda")
    add_test(
        NAME dwg2dxf_freecad_3d_spline_cli
        COMMAND ${CMAKE_COMMAND}
            -DODAFILECONVERTER=${LIBDXFRW_ODAFILECONVERTER_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            "-DSOURCE_DXF=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dxf/ac1015_nonplanar_spline_freecad_control.dxf"
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3d_spline_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_spline_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated;oda")
    add_test(
        NAME dwg2dxf_freecad_3d_helix_cli
        COMMAND ${CMAKE_COMMAND}
            -DODAFILECONVERTER=${LIBDXFRW_ODAFILECONVERTER_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            "-DSOURCE_DXF=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dxf/ac1015_helix_freecad_control.dxf"
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3d_helix_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_helix_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated;oda")
    add_test(
        NAME dwg2dxf_freecad_3d_ellipse_oda_cli
        COMMAND ${CMAKE_COMMAND}
            -DODAFILECONVERTER=${LIBDXFRW_ODAFILECONVERTER_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            "-DSOURCE_DXF=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dxf/ac1015_ellipse_freecad_control.dxf"
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3d_ellipse_oda_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_ellipse_oda_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated;oda")
    add_test(
        NAME dwg2dxf_freecad_3d_solid_oda_cli
        COMMAND ${CMAKE_COMMAND}
            -DODAFILECONVERTER=${LIBDXFRW_ODAFILECONVERTER_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            "-DSOURCE_DXF=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dxf/ac1015_solid_freecad_control.dxf"
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3d_solid_oda_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_solid_oda_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated;oda")
    add_test(
        NAME dwg2dxf_freecad_3dface_oda_cli
        COMMAND ${CMAKE_COMMAND}
            -DODAFILECONVERTER=${LIBDXFRW_ODAFILECONVERTER_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            "-DSOURCE_DXF=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dxf/ac1015_3dface_freecad_control.dxf"
            "-DLIBREDWG_DWGREAD=${LIBDXFRW_LIBREDWG_DWGREAD_EXECUTABLE}"
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3dface_oda_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3dface_oda_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated;oda")
    set(_freecad_3dface_oda_versions
        "ACAD2004|AC1018"
        "ACAD2007|AC1021"
        "ACAD2010|AC1024"
        "ACAD2013|AC1027"
        "ACAD2018|AC1032")
    foreach(_version_pair IN LISTS _freecad_3dface_oda_versions)
        string(REPLACE "|" ";" _version_fields "${_version_pair}")
        list(GET _version_fields 0 _oda_target_version)
        list(GET _version_fields 1 _expected_dwg_version)
        add_test(
            NAME dwg2dxf_freecad_3dface_${_expected_dwg_version}_oda_cli
            COMMAND ${CMAKE_COMMAND}
                -DODAFILECONVERTER=${LIBDXFRW_ODAFILECONVERTER_EXECUTABLE}
                -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
                "-DSOURCE_DXF=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dxf/ac1015_3dface_freecad_control.dxf"
                -DODA_TARGET_VERSION=${_oda_target_version}
                -DEXPECTED_DWG_VERSION=${_expected_dwg_version}
                "-DLIBREDWG_DWGREAD=${LIBDXFRW_LIBREDWG_DWGREAD_EXECUTABLE}"
                -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3dface_oda_cli_test.cmake
        )
        set_tests_properties(
            dwg2dxf_freecad_3dface_${_expected_dwg_version}_oda_cli PROPERTIES
            LABELS "dwg_read;freecad;cli;fast;generated;oda;${_expected_dwg_version}")
    endforeach()
    add_test(
        NAME dwg2dxf_freecad_2d_lwpolyline_oda_cli
        COMMAND ${CMAKE_COMMAND}
            -DODAFILECONVERTER=${LIBDXFRW_ODAFILECONVERTER_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            "-DSOURCE_DXF=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dxf/ac1015_lwpolyline_freecad_control.dxf"
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_2d_lwpolyline_oda_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_2d_lwpolyline_oda_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated;oda")
    add_test(
        NAME dwg2dxf_freecad_3d_2d_polyline_ocs_oda_cli
        COMMAND ${CMAKE_COMMAND}
            -DODAFILECONVERTER=${LIBDXFRW_ODAFILECONVERTER_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            "-DSOURCE_DXF=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dxf/ac1015_2d_polyline_ocs_freecad_control.dxf"
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_2d_polyline_ocs_oda_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_2d_polyline_ocs_oda_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated;oda")
endif()

# Optional pinned FreeCAD importer-outcome check for one tilted WCS 3DFACE.
# It uses the independent ODA writer control and an installed build-identical
# converter, keeping FreeCAD out of default CI.
option(LIBDXFRW_ENABLE_FREECAD_3DFACE_RUNTIME_CONTROL
    "Enable the opt-in FreeCAD 3DFACE converter/importer outcome check" OFF)
if(LIBDXFRW_ENABLE_FREECAD_3DFACE_RUNTIME_CONTROL)
    libdxfrw_require_test_cli(LIBDXFRW_ENABLE_FREECAD_3DFACE_RUNTIME_CONTROL)
    find_program(LIBDXFRW_ODAFILECONVERTER_EXECUTABLE NAMES ODAFileConverter)
    if(NOT LIBDXFRW_ODAFILECONVERTER_EXECUTABLE)
        message(FATAL_ERROR
            "LIBDXFRW_ENABLE_FREECAD_3DFACE_RUNTIME_CONTROL requires ODAFileConverter")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECADCMD_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECADCMD_EXECUTABLE to FreeCAD's freecadcmd")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE to the installed dwg2dxf")
    endif()
    foreach(_operation IN ITEMS open insert)
        add_test(
            NAME dwg2dxf_freecad_3dface_${_operation}_handoff
            COMMAND ${CMAKE_COMMAND}
                "-DFREECADCMD=${LIBDXFRW_FREECADCMD_EXECUTABLE}"
                "-DDWG2DXF=${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}"
                "-DBUILD_DWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DODAFILECONVERTER=${LIBDXFRW_ODAFILECONVERTER_EXECUTABLE}"
                "-DSOURCE_DXF=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dxf/ac1015_3dface_freecad_control.dxf"
                "-DIMPORT_CHECK_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_dwg2dxf_import_check.FCMacro"
                "-DOPERATION=${_operation}"
                -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3dface_runtime_test.cmake
        )
        set_tests_properties(dwg2dxf_freecad_3dface_${_operation}_handoff
            PROPERTIES
                LABELS "freecad;installed;generated;integration"
                TIMEOUT 180)
    endforeach()
endif()

# Optional locally-generated oblique OCS ARC/CIRCLE control. The public
# LibreDWG API is required because its dwgadd recipe cannot set point vectors.
option(LIBDXFRW_ENABLE_LIBREDWG_API_FREECAD_CONTROL
    "Enable the LibreDWG API-generated FreeCAD oblique OCS control" OFF)
if(LIBDXFRW_ENABLE_LIBREDWG_API_FREECAD_CONTROL)
    libdxfrw_require_test_cli(LIBDXFRW_ENABLE_LIBREDWG_API_FREECAD_CONTROL)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBREDWG_API REQUIRED IMPORTED_TARGET libredwg)
    add_executable(libdxfrw_freecad_oblique_arc_circle_generator
        fixtures/dwg/create_oblique_arc_circle_freecad_control.c)
    target_link_libraries(libdxfrw_freecad_oblique_arc_circle_generator
        PRIVATE PkgConfig::LIBREDWG_API)
    add_test(
        NAME dwg2dxf_freecad_3d_oblique_arc_circle_cli
        COMMAND ${CMAKE_COMMAND}
            -DGENERATOR=$<TARGET_FILE:libdxfrw_freecad_oblique_arc_circle_generator>
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3d_oblique_arc_circle_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_oblique_arc_circle_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated;libredwg-api")
endif()

# LibreCAD_3 ABI compatibility check.
# Mirrors LibreCAD_3's DXFimpl override surface; fails to compile if upstream
# introduces a new pure virtual on DRW_Interface or removes/renames a public
# DRW_* field LibreCAD_3 reads. Built as an OBJECT library — no installed
# binary, no link-time impact, just a compile-only ABI gate. See the file
# header in lc3_compat_check.cpp for the sync procedure.
add_library(lc3_compat_check OBJECT
    ${CMAKE_CURRENT_SOURCE_DIR}/lc3_compat_check.cpp
)
target_include_directories(lc3_compat_check PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/intern
)
target_compile_features(lc3_compat_check PRIVATE cxx_std_17)

# Exercise a public DRW_Interface sink against locally generated 3D DXF
# records. The probe writes ASCII and binary DXFs at runtime, reads them back
# through dxfRW, and checks that native Z/topology/OCS values reach the sink.
# It deliberately compiles only against the standalone library API; the
# separate lc3_compat_check above remains the older 2D-consumer guard.
add_executable(libdxfrw_3d_consumer_probe
    ${CMAKE_CURRENT_SOURCE_DIR}/semantic_differential_adapter.cpp
)
target_compile_features(libdxfrw_3d_consumer_probe PRIVATE cxx_std_17)
target_compile_options(libdxfrw_3d_consumer_probe
    PRIVATE ${LIBDXFRW_ADAPTER_WARNING_FLAGS})
target_compile_definitions(libdxfrw_3d_consumer_probe PRIVATE
    LIBDXFRW_SEMANTIC_3D_CONSUMER_PROBE=1)
target_include_directories(libdxfrw_3d_consumer_probe PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/intern)
target_link_libraries(libdxfrw_3d_consumer_probe PRIVATE dxfrw)
add_test(NAME libdxfrw_3d_consumer_probe
    COMMAND libdxfrw_3d_consumer_probe --self-test)
set_tests_properties(libdxfrw_3d_consumer_probe PROPERTIES
    LABELS "dxf;consumer;fast")

# DXF splinepoints round-trip regression test.
# Verifies Band BC: code 44 (tolfit) + fit points (11/21/31) survive a
# DWG -> DXF -> DXF round-trip. Picks a known-spline-bearing sample.
set(_dxf_roundtrip_spline_sample
    ${CMAKE_CURRENT_SOURCE_DIR}/samples/AC1024/visualization_-_conference_room.dwg
)
if(TARGET dwg2dxf AND EXISTS "${_dxf_roundtrip_spline_sample}")
    add_test(
        NAME dxf_roundtrip_splinepoints
        COMMAND ${CMAKE_COMMAND}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            -DSPLINE_DWG=${_dxf_roundtrip_spline_sample}
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dxf_roundtrip_splinepoints_test.cmake
    )
    set_tests_properties(dxf_roundtrip_splinepoints PROPERTIES
        LABELS "dxf_roundtrip;BC"
    )
endif()

# Distinct diffuse and normal MATERIAL map fields without an explicit marker.
# The sources are authored in the test; no drawing fixture is committed.
add_executable(libdxfrw_material_normal_role_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/material_normal_role_tests.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_material_normal_role_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_material_normal_role_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_material_normal_role_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_material_normal_role_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_material_normal_roles COMMAND libdxfrw_material_normal_role_tests)
set_tests_properties(libdxfrw_material_normal_roles PROPERTIES
    LABELS "dxf;fast;material;adapter;transaction" TIMEOUT 30)

# Text justification in pre-R2018 ATTRIB/ATTDEF subclass records.
add_executable(libdxfrw_attribute_subclass_alignment_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/attribute_subclass_alignment_tests.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_attribute_subclass_alignment_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_attribute_subclass_alignment_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_attribute_subclass_alignment_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_attribute_subclass_alignment_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_attribute_subclass_alignment COMMAND libdxfrw_attribute_subclass_alignment_tests)
set_tests_properties(libdxfrw_attribute_subclass_alignment PROPERTIES
    LABELS "dxf;fast;attribute;adapter" TIMEOUT 30)

# Empty optional DIMSTYLE reference vars must not corrupt a repeated CLI pass.
# Explicit null/nonempty lexemes are checked directly, without claiming their
# target closure is qualified. All drawing controls are generated at runtime.
add_executable(libdxfrw_dimstyle_optional_handle_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dimstyle_optional_handle_tests.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_dimstyle_optional_handle_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dimstyle_optional_handle_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dimstyle_optional_handle_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dimstyle_optional_handle_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dimstyle_optional_handles
    COMMAND libdxfrw_dimstyle_optional_handle_tests)
set_tests_properties(libdxfrw_dimstyle_optional_handles PROPERTIES
    LABELS "dxf;fast;adapter")
add_test(NAME libdxfrw_dimstyle_compatible_resource_remap
    COMMAND libdxfrw_dimstyle_optional_handle_tests --resource-remap)
set_tests_properties(libdxfrw_dimstyle_compatible_resource_remap PROPERTIES
    LABELS "dxf;fast;adapter;resource;transaction" TIMEOUT 30)
if(TARGET dwg2dxf)
    foreach(_encoding IN ITEMS ascii binary)
        set(_dimstyle_generate --generate)
        set(_dimstyle_options "")
        if(_encoding STREQUAL "binary")
            set(_dimstyle_generate --generate-binary)
            set(_dimstyle_options -b)
        endif()
        add_test(NAME dwg2dxf_dimstyle_optional_handles_${_encoding}_cli
            COMMAND ${CMAKE_COMMAND}
                -DADAPTER_TEST=$<TARGET_FILE:libdxfrw_dimstyle_optional_handle_tests>
                -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
                -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
                -DCASE_NAME=dimstyle-optional-handles-${_encoding}
                -DGENERATE_MODE=${_dimstyle_generate}
                -DCONVERSION_OPTIONS=${_dimstyle_options}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg2dxf_image_path_test.cmake)
        set_tests_properties(dwg2dxf_dimstyle_optional_handles_${_encoding}_cli
            PROPERTIES LABELS "dxf;fast;adapter;cli")
    endforeach()
endif()

add_executable(libdxfrw_numeric_locale_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/numeric_locale_tests.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_numeric_locale_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_numeric_locale_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_numeric_locale_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_numeric_locale_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_numeric_locale COMMAND libdxfrw_numeric_locale_tests)
add_test(NAME libdxfrw_numeric_process_locale
    COMMAND libdxfrw_numeric_locale_tests --process-locale)
set_tests_properties(libdxfrw_numeric_locale libdxfrw_numeric_process_locale
    PROPERTIES LABELS "dxf;numeric;fast" TIMEOUT 30)
set_tests_properties(libdxfrw_numeric_process_locale PROPERTIES SKIP_RETURN_CODE 77)
if(TARGET dwg2dxf)
    foreach(_numeric_encoding IN ITEMS ascii binary)
        set(_numeric_generate --generate)
        set(_numeric_options "")
        if(_numeric_encoding STREQUAL "binary")
            set(_numeric_generate --generate-binary)
            set(_numeric_options -b)
        endif()
        add_test(NAME dwg2dxf_numeric_locale_${_numeric_encoding}_cli
            COMMAND ${CMAKE_COMMAND}
                -DADAPTER_TEST=$<TARGET_FILE:libdxfrw_numeric_locale_tests>
                -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
                -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
                -DCASE_NAME=numeric-locale-${_numeric_encoding}
                -DGENERATE_MODE=${_numeric_generate}
                -DCONVERSION_OPTIONS=${_numeric_options}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg2dxf_image_path_test.cmake)
        set_tests_properties(dwg2dxf_numeric_locale_${_numeric_encoding}_cli
            PROPERTIES LABELS "dxf;numeric;cli;fast" TIMEOUT 30)
    endforeach()
endif()

add_executable(libdxfrw_dwg_class_source_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_class_source_tests.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_dwg_class_source_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_class_source_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_class_source_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_class_source_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_class_source
    COMMAND libdxfrw_dwg_class_source_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg
        ${CMAKE_CURRENT_BINARY_DIR}/class-source)
set_tests_properties(libdxfrw_dwg_class_source PROPERTIES
    LABELS "dwg;classes;source;fast" TIMEOUT 30)

# Keep this after every local target customization. A differing compile/link
# context must get its own adapter, not silently reuse these object files.
libdxfrw_check_test_adapter_context(
    libdxfrw_dwg2dxf_adapter_tests libdxfrw_block_owner_tests
    libdxfrw_dimstyle_optional_handle_tests libdxfrw_numeric_locale_tests
    libdxfrw_dwg_class_source_tests)
add_test(NAME libdxfrw_adapter_build_context
    COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/adapter_build_context_tests.py
        --cmake "${CMAKE_COMMAND}"
        --module "${CMAKE_SOURCE_DIR}/cmake/libdxfrwTestAdapter.cmake"
        --compiler "${CMAKE_CXX_COMPILER}"
        --generator "${CMAKE_GENERATOR}"
        --generator-platform "${CMAKE_GENERATOR_PLATFORM}"
        --generator-toolset "${CMAKE_GENERATOR_TOOLSET}"
        --toolchain "${CMAKE_TOOLCHAIN_FILE}")
set_tests_properties(libdxfrw_adapter_build_context PROPERTIES
    LABELS "fast;build;adapter" TIMEOUT 120)
add_test(NAME libdxfrw_root_adapter_build_context
    COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/root_adapter_build_context_tests.py
        --cmake "${CMAKE_COMMAND}"
        --ctest "${CMAKE_CTEST_COMMAND}"
        --module "${CMAKE_SOURCE_DIR}/cmake/libdxfrwRootTestAdapters.cmake"
        --build-dir "${CMAKE_BINARY_DIR}"
        --compiler "${CMAKE_CXX_COMPILER}"
        --generator "${CMAKE_GENERATOR}"
        --generator-platform "${CMAKE_GENERATOR_PLATFORM}"
        --generator-toolset "${CMAKE_GENERATOR_TOOLSET}"
        --generator-instance "${CMAKE_GENERATOR_INSTANCE}"
        --toolchain "${CMAKE_TOOLCHAIN_FILE}"
        --make-program "${CMAKE_MAKE_PROGRAM}"
        --compiler-target "${CMAKE_CXX_COMPILER_TARGET}"
        --sysroot "${CMAKE_SYSROOT}"
        --system-name "${CMAKE_SYSTEM_NAME}"
        --system-version "${CMAKE_SYSTEM_VERSION}"
        --system-processor "${CMAKE_SYSTEM_PROCESSOR}"
        --configuration "$<CONFIG>")
set_tests_properties(libdxfrw_root_adapter_build_context PROPERTIES
    LABELS "fast;build;adapter" TIMEOUT 120)
add_test(NAME libdxfrw_cli_optional_build
    COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/cli_optional_build_tests.py
        --source "${CMAKE_SOURCE_DIR}"
        --cmake "${CMAKE_COMMAND}"
        --ctest "${CMAKE_CTEST_COMMAND}"
        --compiler "${CMAKE_CXX_COMPILER}"
        --generator "${CMAKE_GENERATOR}"
        --generator-platform "${CMAKE_GENERATOR_PLATFORM}"
        --generator-toolset "${CMAKE_GENERATOR_TOOLSET}"
        --generator-instance "${CMAKE_GENERATOR_INSTANCE}"
        --toolchain "${CMAKE_TOOLCHAIN_FILE}"
        --make-program "${CMAKE_MAKE_PROGRAM}"
        --compiler-target "${CMAKE_CXX_COMPILER_TARGET}"
        --sysroot "${CMAKE_SYSROOT}"
        --system-name "${CMAKE_SYSTEM_NAME}"
        --system-version "${CMAKE_SYSTEM_VERSION}"
        --system-processor "${CMAKE_SYSTEM_PROCESSOR}"
        --configuration "$<CONFIG>")
set_tests_properties(libdxfrw_cli_optional_build PROPERTIES
    LABELS "fast;build;consumer;cli-optional" TIMEOUT 120)

set(_dwg_test_versions AC1021 AC1024 AC1027 AC1032)

set(_acsh_revolve_sample
    "${CMAKE_CURRENT_SOURCE_DIR}/samples/AC1024/visualization_-_conference_room.dwg")
if(EXISTS "${_acsh_revolve_sample}")
    add_test(
        NAME dwg_acsh_revolve_opaque_frame_capture
        COMMAND $<TARGET_FILE:libdxfrw_dwg_reader_matrix_tests>
            --acsh-opaque-frame-capture "${_acsh_revolve_sample}")
    set_tests_properties(dwg_acsh_revolve_opaque_frame_capture PROPERTIES
        LABELS "dwg_read;AC1024;sample;opaque;3d" TIMEOUT 60)
endif()

set(_acsh_failed_sample
    "${CMAKE_CURRENT_SOURCE_DIR}/samples/AC1024/visualization_-_condominium_with_skylight.dwg")
if(EXISTS "${_acsh_failed_sample}")
    add_test(
        NAME dwg_acsh_failed_frame_retention
        COMMAND $<TARGET_FILE:libdxfrw_dwg_reader_matrix_tests>
            --acsh-failed-frame-retention "${_acsh_failed_sample}")
    set_tests_properties(dwg_acsh_failed_frame_retention PROPERTIES
        LABELS "dwg_read;AC1024;sample;opaque;3d" TIMEOUT 60)
endif()

add_test(NAME libdxfrw_persistent_manager_prefix_dispatch
    COMMAND libdxfrw_dwg_reader_matrix_tests --persistent-manager-prefix-dispatch)
set_tests_properties(libdxfrw_persistent_manager_prefix_dispatch PROPERTIES
    LABELS "fast;dwg;objects;associative;generated" TIMEOUT 15)

set(_assoc_failed_sample
    "${CMAKE_CURRENT_SOURCE_DIR}/samples/AC1024/visualization_-_sun_and_sky_demo.dwg")
if(EXISTS "${_assoc_failed_sample}")
    add_test(
        NAME dwg_associative_failed_frame_retention
        COMMAND $<TARGET_FILE:libdxfrw_dwg_reader_matrix_tests>
            --associative-failed-frame-retention "${_assoc_failed_sample}")
    set_tests_properties(dwg_associative_failed_frame_retention PROPERTIES
        LABELS "dwg_read;AC1024;sample;opaque;associative;fast" TIMEOUT 60)
endif()

foreach(_ver IN LISTS _dwg_test_versions)
    file(GLOB _samples
        LIST_DIRECTORIES false
        "${CMAKE_CURRENT_SOURCE_DIR}/samples/${_ver}/*.dwg"
    )
    list(LENGTH _samples _n)
    if(_n EQUAL 0)
        message(STATUS "tests/: no ${_ver} samples in tests/samples/${_ver}/ — drop *.dwg files there to enable")
        continue()
    endif()
    message(STATUS "tests/: registering ${_n} ${_ver} reader sample test(s)")
    foreach(_sample IN LISTS _samples)
        get_filename_component(_name "${_sample}" NAME_WE)
        add_test(
            NAME dwg_read_${_ver}_${_name}
            COMMAND ${CMAKE_COMMAND}
                -DREADER_TEST=$<TARGET_FILE:libdxfrw_dwg_reader_matrix_tests>
                -DSAMPLE=${_sample}
                -DEXPECTED_VERSION=${_ver}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg_read_test.cmake
        )
        set_tests_properties(dwg_read_${_ver}_${_name} PROPERTIES
            LABELS "dwg_read;${_ver};sample"
            TIMEOUT 60
        )
    endforeach()
endforeach()
