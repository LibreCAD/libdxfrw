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

add_executable(libdxfrw_batch_output_names_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dx_batch_output_names_tests.cpp
    ${CMAKE_SOURCE_DIR}/dwg2dxf/dx_batch_output_names.cpp)
target_compile_features(libdxfrw_batch_output_names_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_batch_output_names_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_batch_output_names_tests
    PRIVATE ${CMAKE_SOURCE_DIR}/dwg2dxf)
add_test(NAME libdxfrw_batch_output_names
    COMMAND libdxfrw_batch_output_names_tests)
set_tests_properties(libdxfrw_batch_output_names PROPERTIES
    LABELS "fast;cli;filesystem" TIMEOUT 30)

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

# Explicit legacy-applied CIRCLE/ARC recovery is opt-in and leaves legacy
# reader/writer and public entity layouts unchanged.
add_executable(libdxfrw_dxf_legacy_extrusion_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_legacy_extrusion_conversion_tests.cpp)
target_compile_features(libdxfrw_dxf_legacy_extrusion_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_legacy_extrusion_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_legacy_extrusion_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_legacy_extrusion_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_legacy_extrusion
    COMMAND libdxfrw_dxf_legacy_extrusion_tests)
set_tests_properties(libdxfrw_dxf_legacy_extrusion PROPERTIES LABELS "dxf;fast;extrusion" TIMEOUT 30)

add_executable(libdxfrw_dwg_ac1024_layer_source_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_ac1024_layer_source_tests.cpp)
target_compile_features(libdxfrw_dwg_ac1024_layer_source_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_ac1024_layer_source_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_ac1024_layer_source_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libdxfrw_dwg_ac1024_layer_source_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_ac1024_layer_source_receipt
    COMMAND libdxfrw_dwg_ac1024_layer_source_tests
        ${CMAKE_CURRENT_BINARY_DIR}/ac1024-layer-source-scratch)
set_tests_properties(libdxfrw_dwg_ac1024_layer_source_receipt PROPERTIES
    LABELS "fast;dwg;layer;generated;source" TIMEOUT 30)

# Plotstyle root collision checks preserve the caller's spelling and avoid
# changing admission rules for unrelated dictionary keys.
add_executable(libdxfrw_dwg_plotstyle_nod_admission_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_plotstyle_nod_admission_tests.cpp)
target_compile_features(libdxfrw_dwg_plotstyle_nod_admission_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_plotstyle_nod_admission_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_plotstyle_nod_admission_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_plotstyle_nod_admission_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_plotstyle_nod_admission COMMAND libdxfrw_dwg_plotstyle_nod_admission_tests)
set_tests_properties(libdxfrw_dwg_plotstyle_nod_admission PROPERTIES LABELS "dwg;fast;plotstyle" TIMEOUT 30)

add_executable(libdxfrw_dwg_default_plotstyle_optin_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_default_plotstyle_optin_tests.cpp)
target_compile_features(libdxfrw_dwg_default_plotstyle_optin_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_default_plotstyle_optin_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_default_plotstyle_optin_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_default_plotstyle_optin_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_default_plotstyle_optin COMMAND libdxfrw_dwg_default_plotstyle_optin_tests)
set_tests_properties(libdxfrw_dwg_default_plotstyle_optin PROPERTIES LABELS "dwg;fast;plotstyle" TIMEOUT 30)

# AC1024 default MATERIAL source-profile constructor and following handles.
add_executable(libdxfrw_dwg_ac1024_default_material_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_ac1024_default_material_tests.cpp)
target_compile_features(libdxfrw_dwg_ac1024_default_material_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_ac1024_default_material_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_ac1024_default_material_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_ac1024_default_material_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_ac1024_default_material COMMAND libdxfrw_dwg_ac1024_default_material_tests)
set_tests_properties(libdxfrw_dwg_ac1024_default_material PROPERTIES LABELS "dwg;fast;material" TIMEOUT 30)

add_executable(libdxfrw_dwg_default_material_optin_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_default_material_optin_tests.cpp)
target_compile_features(libdxfrw_dwg_default_material_optin_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_default_material_optin_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_default_material_optin_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_default_material_optin_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_default_material_optin COMMAND libdxfrw_dwg_default_material_optin_tests)
set_tests_properties(libdxfrw_dwg_default_material_optin PROPERTIES LABELS "dwg;fast;material" TIMEOUT 30)

add_executable(libdxfrw_dwg_ac1027_material_dictionary_wire_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_ac1027_material_dictionary_wire_tests.cpp)
target_compile_features(libdxfrw_dwg_ac1027_material_dictionary_wire_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_ac1027_material_dictionary_wire_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_ac1027_material_dictionary_wire_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_ac1027_material_dictionary_wire_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_ac1027_material_dictionary_wire
    COMMAND libdxfrw_dwg_ac1027_material_dictionary_wire_tests)
set_tests_properties(libdxfrw_dwg_ac1027_material_dictionary_wire PROPERTIES
    LABELS "dwg;fast;material;AC1027;generated" TIMEOUT 10)

add_test(NAME libdxfrw_ac1021_material_peer_audit
    COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/ac1021_material_peer_audit_tests.py)
set_tests_properties(libdxfrw_ac1021_material_peer_audit PROPERTIES
    LABELS "dwg;fast;material;tools" TIMEOUT 15)

add_test(NAME libdxfrw_ac1021_geometry_peer_audit
    COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/ac1021_geometry_peer_audit_tests.py)
set_tests_properties(libdxfrw_ac1021_geometry_peer_audit PROPERTIES
    LABELS "dwg;fast;geometry;tools" TIMEOUT 15)

add_test(NAME libdxfrw_ac1021_varied_geometry_peer_audit
    COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/ac1021_varied_geometry_peer_audit_tests.py)
set_tests_properties(libdxfrw_ac1021_varied_geometry_peer_audit PROPERTIES
    LABELS "dwg;fast;geometry;tools" TIMEOUT 15)

add_test(NAME libdxfrw_ac1021_curve_geometry_peer_audit
    COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/ac1021_curve_geometry_peer_audit_tests.py)
set_tests_properties(libdxfrw_ac1021_curve_geometry_peer_audit PROPERTIES
    LABELS "dwg;fast;geometry;tools" TIMEOUT 15)

add_test(NAME libdxfrw_ac1021_lwpolyline_footer_peer_audit
    COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/ac1021_lwpolyline_footer_peer_audit_tests.py)
set_tests_properties(libdxfrw_ac1021_lwpolyline_footer_peer_audit PROPERTIES
    LABELS "dwg;fast;geometry;tools;AC1021" TIMEOUT 15)

add_test(NAME libdxfrw_ac1021_polyline_owners_peer_audit
    COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/ac1021_polyline_owners_peer_audit_tests.py)
set_tests_properties(libdxfrw_ac1021_polyline_owners_peer_audit PROPERTIES
    LABELS "dwg;fast;polyline;tools;AC1021" TIMEOUT 15)

add_test(NAME libdxfrw_polyline_cross_version_membership_audit
    COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/polyline_cross_version_membership_audit_tests.py)
set_tests_properties(libdxfrw_polyline_cross_version_membership_audit PROPERTIES
    LABELS "dwg;fast;polyline;tools;AC1018;AC1024" TIMEOUT 15)

add_executable(libdxfrw_dwg_modern_header_defaults_tests ${CMAKE_CURRENT_SOURCE_DIR}/dwg_modern_header_default_tests.cpp)
target_compile_features(libdxfrw_dwg_modern_header_defaults_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_modern_header_defaults_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_modern_header_defaults_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_modern_header_defaults_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_modern_header_defaults COMMAND libdxfrw_dwg_modern_header_defaults_tests)
set_tests_properties(libdxfrw_dwg_modern_header_defaults PROPERTIES LABELS "dwg;fast;header;AC1027;AC1032" TIMEOUT 30)
add_test(NAME libdxfrw_modern_header_default_peer
    COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/modern_header_default_peer_tests.py)
set_tests_properties(libdxfrw_modern_header_default_peer PROPERTIES LABELS "dwg;fast;header;tools;AC1027;AC1032" TIMEOUT 15)

add_test(NAME libdxfrw_modern_writer_rejection_audit
    COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/modern_writer_rejection_audit_tests.py)
set_tests_properties(libdxfrw_modern_writer_rejection_audit PROPERTIES
    LABELS "dwg;fast;tools;AC1027;AC1032" TIMEOUT 15)

add_test(NAME libdxfrw_polyline_cross_scope_peer_audit
    COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/polyline_cross_scope_peer_audit_tests.py)
set_tests_properties(libdxfrw_polyline_cross_scope_peer_audit PROPERTIES
    LABELS "dwg;fast;polyline;tools;AC1018;AC1024" TIMEOUT 15)

add_executable(libdxfrw_dwg_polyline_1018_1024_membership_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_polyline_1018_1024_membership_tests.cpp)
target_compile_features(libdxfrw_dwg_polyline_1018_1024_membership_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_polyline_1018_1024_membership_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_polyline_1018_1024_membership_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_polyline_1018_1024_membership_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_polyline_1018_1024_membership
    COMMAND libdxfrw_dwg_polyline_1018_1024_membership_tests)
set_tests_properties(libdxfrw_dwg_polyline_1018_1024_membership PROPERTIES
    LABELS "dwg;fast;polyline;transaction;AC1018;AC1024" TIMEOUT 30)

add_executable(libdxfrw_dwg_polyline_cross_scope_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_polyline_cross_scope_tests.cpp)
target_compile_features(libdxfrw_dwg_polyline_cross_scope_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_polyline_cross_scope_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_polyline_cross_scope_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_polyline_cross_scope_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_polyline_cross_scope COMMAND libdxfrw_dwg_polyline_cross_scope_tests)
set_tests_properties(libdxfrw_dwg_polyline_cross_scope PROPERTIES
    LABELS "dwg;fast;polyline;transaction;AC1018;AC1024" TIMEOUT 30)

add_executable(libdxfrw_dwg_polyline_cross_scope_group_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_polyline_cross_scope_group_tests.cpp)
target_compile_features(libdxfrw_dwg_polyline_cross_scope_group_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_polyline_cross_scope_group_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_polyline_cross_scope_group_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_polyline_cross_scope_group_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_polyline_cross_scope_group COMMAND libdxfrw_dwg_polyline_cross_scope_group_tests)
set_tests_properties(libdxfrw_dwg_polyline_cross_scope_group PROPERTIES
    LABELS "dwg;fast;polyline;transaction;AC1018;AC1024" TIMEOUT 30)

add_executable(libdxfrw_dwg_ac1021_solid_trace_footer_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_ac1021_solid_trace_footer_tests.cpp)
target_compile_features(libdxfrw_dwg_ac1021_solid_trace_footer_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_ac1021_solid_trace_footer_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_ac1021_solid_trace_footer_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_ac1021_solid_trace_footer_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_ac1021_solid_trace_footer COMMAND libdxfrw_dwg_ac1021_solid_trace_footer_tests)
set_tests_properties(libdxfrw_dwg_ac1021_solid_trace_footer PROPERTIES
    LABELS "dwg;fast;AC1021;transaction" TIMEOUT 30)

add_executable(libdxfrw_dwg_circle_arc_positive_radius_reader_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_circle_arc_positive_radius_reader_tests.cpp)
target_compile_features(libdxfrw_dwg_circle_arc_positive_radius_reader_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_circle_arc_positive_radius_reader_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_circle_arc_positive_radius_reader_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_circle_arc_positive_radius_reader_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_circle_arc_positive_radius_reader
    COMMAND libdxfrw_dwg_circle_arc_positive_radius_reader_tests)
set_tests_properties(libdxfrw_dwg_circle_arc_positive_radius_reader PROPERTIES
    LABELS "dwg;fast;reader;circle;arc;AC1021;retry" TIMEOUT 30)

add_executable(libdxfrw_dwg_ellipse_geometry_reader_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_ellipse_geometry_reader_tests.cpp)
target_compile_features(libdxfrw_dwg_ellipse_geometry_reader_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_ellipse_geometry_reader_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_ellipse_geometry_reader_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_ellipse_geometry_reader_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_ellipse_geometry_reader
    COMMAND libdxfrw_dwg_ellipse_geometry_reader_tests)
set_tests_properties(libdxfrw_dwg_ellipse_geometry_reader PROPERTIES
    LABELS "dwg;fast;reader;ellipse;AC1021;retry" TIMEOUT 30)

add_executable(libdxfrw_dwg_solid_trace_elevation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_solid_trace_elevation_tests.cpp)
target_compile_features(libdxfrw_dwg_solid_trace_elevation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_solid_trace_elevation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_solid_trace_elevation_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_solid_trace_elevation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_solid_trace_elevation COMMAND libdxfrw_dwg_solid_trace_elevation_tests)
set_tests_properties(libdxfrw_dwg_solid_trace_elevation PROPERTIES
    LABELS "dwg;fast;AC1021;transaction" TIMEOUT 30)

add_executable(libdxfrw_dxf_lwpoly_wcs_vertices_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_lwpoly_wcs_vertices_tests.cpp)
target_compile_features(libdxfrw_dxf_lwpoly_wcs_vertices_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_lwpoly_wcs_vertices_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_lwpoly_wcs_vertices_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_lwpoly_wcs_vertices_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_lwpoly_wcs_vertices COMMAND libdxfrw_dxf_lwpoly_wcs_vertices_tests)
set_tests_properties(libdxfrw_dxf_lwpoly_wcs_vertices PROPERTIES LABELS "dxf;fast;geometry" TIMEOUT 30)

add_executable(libdxfrw_dxf_r12_ellipse_wrap_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_r12_ellipse_wrap_tests.cpp)
target_compile_features(libdxfrw_dxf_r12_ellipse_wrap_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_r12_ellipse_wrap_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_r12_ellipse_wrap_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_r12_ellipse_wrap_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_r12_ellipse_wrap COMMAND libdxfrw_dxf_r12_ellipse_wrap_tests)
set_tests_properties(libdxfrw_dxf_r12_ellipse_wrap PROPERTIES LABELS "dxf;fast;geometry;transaction" TIMEOUT 30)

add_executable(libdxfrw_dxf_r12_ellipse_boundary_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_r12_ellipse_boundary_tests.cpp)
target_compile_features(libdxfrw_dxf_r12_ellipse_boundary_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_r12_ellipse_boundary_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_r12_ellipse_boundary_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_r12_ellipse_boundary_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_r12_ellipse_boundary COMMAND libdxfrw_dxf_r12_ellipse_boundary_tests)
set_tests_properties(libdxfrw_dxf_r12_ellipse_boundary PROPERTIES LABELS "dxf;fast;geometry;transaction" TIMEOUT 30)

add_executable(libdxfrw_dxf_mtext_legacy_columns_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_mtext_legacy_columns_tests.cpp
    ${CMAKE_SOURCE_DIR}/dwg2dxf/dx_iface.cpp)
target_compile_features(libdxfrw_dxf_mtext_legacy_columns_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_mtext_legacy_columns_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_mtext_legacy_columns_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/dwg2dxf)
target_link_libraries(libdxfrw_dxf_mtext_legacy_columns_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_mtext_legacy_columns COMMAND libdxfrw_dxf_mtext_legacy_columns_tests)
set_tests_properties(libdxfrw_dxf_mtext_legacy_columns PROPERTIES LABELS "dxf;fast;mtext;compatibility;transaction" TIMEOUT 30)

add_executable(libdxfrw_dwg_mtext_rotation_direction_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_mtext_rotation_direction_tests.cpp)
target_compile_features(libdxfrw_dwg_mtext_rotation_direction_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_mtext_rotation_direction_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_mtext_rotation_direction_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_mtext_rotation_direction_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_mtext_rotation_direction COMMAND libdxfrw_dwg_mtext_rotation_direction_tests)
set_tests_properties(libdxfrw_dwg_mtext_rotation_direction PROPERTIES
    LABELS "dwg;fast;mtext;geometry;transaction" TIMEOUT 30)

add_executable(libdxfrw_dwg_text_style_resolution_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_text_style_resolution_tests.cpp)
target_compile_features(libdxfrw_dwg_text_style_resolution_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_text_style_resolution_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_text_style_resolution_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_text_style_resolution_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_text_style_resolution COMMAND libdxfrw_dwg_text_style_resolution_tests)
set_tests_properties(libdxfrw_dwg_text_style_resolution PROPERTIES
    LABELS "dwg;fast;text;transaction" TIMEOUT 30)

add_executable(libdxfrw_dwg_hatch_edge_subtype_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_hatch_edge_subtype_tests.cpp)
target_compile_features(libdxfrw_dwg_hatch_edge_subtype_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_hatch_edge_subtype_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_hatch_edge_subtype_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_hatch_edge_subtype_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_hatch_edge_subtype COMMAND libdxfrw_dwg_hatch_edge_subtype_tests)
set_tests_properties(libdxfrw_dwg_hatch_edge_subtype PROPERTIES
    LABELS "dwg;fast;hatch;transaction" TIMEOUT 30)

add_executable(libdxfrw_dwg_entity_encoder_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_hatch_encoder_allocation_tests.cpp)
target_compile_features(libdxfrw_dwg_entity_encoder_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_entity_encoder_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_entity_encoder_allocation_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_entity_encoder_allocation_tests PRIVATE dxfrw)
add_test(NAME dwg_entity_encoder_allocation_rollback_retry
    COMMAND libdxfrw_dwg_entity_encoder_allocation_tests)
set_tests_properties(dwg_entity_encoder_allocation_rollback_retry PROPERTIES
    LABELS "dwg;fast;entity;transaction;allocation" TIMEOUT 30)

add_executable(libdxfrw_dwg_hatch_ellipse_edge_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_hatch_ellipse_edge_dwg_tests.cpp)
target_compile_features(libdxfrw_dwg_hatch_ellipse_edge_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_hatch_ellipse_edge_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_hatch_ellipse_edge_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_hatch_ellipse_edge_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_hatch_ellipse_edge COMMAND libdxfrw_dwg_hatch_ellipse_edge_tests)
set_tests_properties(libdxfrw_dwg_hatch_ellipse_edge PROPERTIES
    LABELS "dwg;fast;hatch;transaction" TIMEOUT 30)

add_executable(libdxfrw_dwg_polyline_owned_handle_roles_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_polyline_owned_handle_roles_tests.cpp)
target_compile_features(libdxfrw_dwg_polyline_owned_handle_roles_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_polyline_owned_handle_roles_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_polyline_owned_handle_roles_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_polyline_owned_handle_roles_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_polyline_owned_handle_roles
    COMMAND libdxfrw_dwg_polyline_owned_handle_roles_tests)
set_tests_properties(libdxfrw_dwg_polyline_owned_handle_roles PROPERTIES
    LABELS "dwg;fast;polyline" TIMEOUT 30)

add_executable(libdxfrw_dwg_polyline_owner_facade_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_polyline_owner_facade_tests.cpp)
target_compile_features(libdxfrw_dwg_polyline_owner_facade_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_polyline_owner_facade_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_polyline_owner_facade_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_polyline_owner_facade_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_polyline_owner_facade COMMAND libdxfrw_dwg_polyline_owner_facade_tests)
set_tests_properties(libdxfrw_dwg_polyline_owner_facade PROPERTIES LABELS "dwg;fast;polyline" TIMEOUT 30)

add_executable(libdxfrw_dwg_modern_repeated_header_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_modern_repeated_header_tests.cpp)
target_compile_features(libdxfrw_dwg_modern_repeated_header_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_modern_repeated_header_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_modern_repeated_header_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_modern_repeated_header_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_modern_repeated_header
    COMMAND libdxfrw_dwg_modern_repeated_header_tests)
set_tests_properties(libdxfrw_dwg_modern_repeated_header PROPERTIES
    LABELS "dwg;fast;transaction;AC1027;AC1032" TIMEOUT 30)

add_executable(libdxfrw_dwg_modern_repeated_header_fault_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_modern_repeated_header_tests.cpp
    ${CMAKE_SOURCE_DIR}/src/intern/dwgwriter18.cpp
    ${CMAKE_SOURCE_DIR}/src/intern/dwgwriterlayoutvalidation.cpp)
target_compile_features(libdxfrw_dwg_modern_repeated_header_fault_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_modern_repeated_header_fault_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_compile_definitions(libdxfrw_dwg_modern_repeated_header_fault_tests PRIVATE DWG_LAYOUT_VALIDATION_TESTS)
target_include_directories(libdxfrw_dwg_modern_repeated_header_fault_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/intern)
target_link_libraries(libdxfrw_dwg_modern_repeated_header_fault_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_modern_repeated_header_fault
    COMMAND libdxfrw_dwg_modern_repeated_header_fault_tests)
set_tests_properties(libdxfrw_dwg_modern_repeated_header_fault PROPERTIES
    LABELS "dwg;fast;transaction;AC1027;AC1032" TIMEOUT 30)

add_executable(libdxfrw_dwg_modern_header_section_size_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_modern_header_section_size_tests.cpp)
target_compile_features(libdxfrw_dwg_modern_header_section_size_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_modern_header_section_size_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_modern_header_section_size_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_modern_header_section_size_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_modern_header_section_size
    COMMAND libdxfrw_dwg_modern_header_section_size_tests)
set_tests_properties(libdxfrw_dwg_modern_header_section_size PROPERTIES
    LABELS "dwg;fast;transaction;AC1027;AC1032" TIMEOUT 30)

add_executable(libdxfrw_dwg_ac1021_polyline_block_membership_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_ac1021_polyline_block_membership_tests.cpp)
target_compile_features(libdxfrw_dwg_ac1021_polyline_block_membership_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_ac1021_polyline_block_membership_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_ac1021_polyline_block_membership_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_ac1021_polyline_block_membership_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_ac1021_polyline_block_membership
    COMMAND libdxfrw_dwg_ac1021_polyline_block_membership_tests)
set_tests_properties(libdxfrw_dwg_ac1021_polyline_block_membership PROPERTIES
    LABELS "dwg;fast;polyline;transaction;AC1021" TIMEOUT 30)

add_executable(libdxfrw_dwg_ac1021_polyline_group_targets_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_ac1021_polyline_group_targets_tests.cpp)
target_compile_features(libdxfrw_dwg_ac1021_polyline_group_targets_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_ac1021_polyline_group_targets_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_ac1021_polyline_group_targets_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_ac1021_polyline_group_targets_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_ac1021_polyline_group_targets
    COMMAND libdxfrw_dwg_ac1021_polyline_group_targets_tests)
set_tests_properties(libdxfrw_dwg_ac1021_polyline_group_targets PROPERTIES
    LABELS "dwg;fast;polyline;group;transaction;AC1021" TIMEOUT 30)

add_test(NAME libdxfrw_ac1021_polyline_group_peer_audit
    COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/ac1021_polyline_group_peer_audit_tests.py)
set_tests_properties(libdxfrw_ac1021_polyline_group_peer_audit PROPERTIES
    LABELS "dwg;fast;polyline;group;tools;AC1021" TIMEOUT 15)

add_executable(libdxfrw_dwg_legacy_model_chain_optin_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_legacy_model_chain_optin_tests.cpp)
target_compile_features(libdxfrw_dwg_legacy_model_chain_optin_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_legacy_model_chain_optin_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_legacy_model_chain_optin_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_legacy_model_chain_optin_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_legacy_model_chain_optin COMMAND libdxfrw_dwg_legacy_model_chain_optin_tests)
set_tests_properties(libdxfrw_dwg_legacy_model_chain_optin PROPERTIES LABELS "dwg;fast;legacy-chain" TIMEOUT 30)

add_executable(libdxfrw_dwg_legacy_polyline_chain_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_legacy_polyline_chain_tests.cpp)
target_compile_features(libdxfrw_dwg_legacy_polyline_chain_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_legacy_polyline_chain_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_legacy_polyline_chain_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_legacy_polyline_chain_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_legacy_polyline_chain COMMAND libdxfrw_dwg_legacy_polyline_chain_tests)
set_tests_properties(libdxfrw_dwg_legacy_polyline_chain PROPERTIES LABELS "dwg;fast;legacy-chain;polyline" TIMEOUT 30)

add_executable(libdxfrw_dwg_legacy_insert_chain_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_legacy_insert_chain_tests.cpp)
target_compile_features(libdxfrw_dwg_legacy_insert_chain_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_legacy_insert_chain_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_legacy_insert_chain_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_legacy_insert_chain_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_legacy_insert_chain COMMAND libdxfrw_dwg_legacy_insert_chain_tests)
set_tests_properties(libdxfrw_dwg_legacy_insert_chain PROPERTIES LABELS "dwg;fast;legacy-chain;insert" TIMEOUT 30)

add_executable(libdxfrw_dwg_legacy_block_content_chain_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_legacy_block_content_chain_tests.cpp)
target_compile_features(libdxfrw_dwg_legacy_block_content_chain_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_legacy_block_content_chain_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_legacy_block_content_chain_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_legacy_block_content_chain_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_legacy_block_content_chain COMMAND libdxfrw_dwg_legacy_block_content_chain_tests)
set_tests_properties(libdxfrw_dwg_legacy_block_content_chain PROPERTIES LABELS "dwg;fast;legacy-chain;block" TIMEOUT 30)

add_executable(libdxfrw_dxf_fourth_corner_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_fourth_corner_tests.cpp)
target_compile_features(libdxfrw_dxf_fourth_corner_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_fourth_corner_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_fourth_corner_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_fourth_corner_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_fourth_corner COMMAND libdxfrw_dxf_fourth_corner_tests)
set_tests_properties(libdxfrw_dxf_fourth_corner PROPERTIES LABELS "dxf;fast;geometry" TIMEOUT 30)

add_executable(libdxfrw_dxf_ellipse_extreme_axis_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_ellipse_extreme_axis_tests.cpp)
target_compile_features(libdxfrw_dxf_ellipse_extreme_axis_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_ellipse_extreme_axis_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_ellipse_extreme_axis_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_ellipse_extreme_axis_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_ellipse_extreme_axis COMMAND libdxfrw_dxf_ellipse_extreme_axis_tests)
set_tests_properties(libdxfrw_dxf_ellipse_extreme_axis PROPERTIES LABELS "dxf;fast;geometry" TIMEOUT 30)

add_executable(libdxfrw_dwg_ltype_multiversion_count_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_ltype_multiversion_count_tests.cpp)
target_compile_features(libdxfrw_dwg_ltype_multiversion_count_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_ltype_multiversion_count_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_ltype_multiversion_count_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_ltype_multiversion_count_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_ltype_multiversion_count COMMAND libdxfrw_dwg_ltype_multiversion_count_tests)
set_tests_properties(libdxfrw_dwg_ltype_multiversion_count PROPERTIES LABELS "dwg;fast;ltype" TIMEOUT 30)

add_executable(libdxfrw_dwg_ltype_ac1021_count_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_ltype_ac1021_count_tests.cpp)
target_compile_features(libdxfrw_dwg_ltype_ac1021_count_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_ltype_ac1021_count_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_ltype_ac1021_count_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_ltype_ac1021_count_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_ltype_ac1021_count COMMAND libdxfrw_dwg_ltype_ac1021_count_tests)
set_tests_properties(libdxfrw_dwg_ltype_ac1021_count PROPERTIES LABELS "dwg;fast;ltype" TIMEOUT 30)

add_executable(libdxfrw_dxf_hatch_boundary_payload_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_hatch_boundary_payload_tests.cpp)
target_compile_features(libdxfrw_dxf_hatch_boundary_payload_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_hatch_boundary_payload_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_hatch_boundary_payload_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_hatch_boundary_payload_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_hatch_boundary_payload COMMAND libdxfrw_dxf_hatch_boundary_payload_tests)
set_tests_properties(libdxfrw_dxf_hatch_boundary_payload PROPERTIES LABELS "dxf;fast;geometry" TIMEOUT 30)

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
add_executable(libdxfrw_ac1021_group_polyline_dxf_readback_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/ac1021_group_polyline_dxf_readback_tests.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_ac1021_group_polyline_dxf_readback_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_ac1021_group_polyline_dxf_readback_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_ac1021_group_polyline_dxf_readback_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_ac1021_group_polyline_dxf_readback_tests PRIVATE dxfrw)

add_executable(libdxfrw_xdata_int16_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_xdata_int16_tests.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_xdata_int16_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_xdata_int16_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_xdata_int16_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_xdata_int16_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_xdata_int16_exact_numeric
    COMMAND libdxfrw_xdata_int16_tests)
set_tests_properties(libdxfrw_xdata_int16_exact_numeric PROPERTIES
    LABELS "fast;dxf;xdata;transaction" TIMEOUT 20)
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

add_test(NAME libdxfrw_ltype_forward_style_identity
    COMMAND libdxfrw_dxf_fixture_tests --ltype-style-handle-remap)
set_tests_properties(libdxfrw_ltype_forward_style_identity PROPERTIES
    LABELS "dxf;fast;transaction" TIMEOUT 30)

# A WCS POLYLINE cannot silently shed a non-default extrusion vector. The
# runtime-authored ASCII/Binary control checks atomic refusal, corrected retry
# on the same adapter, and typed vertex readback; no drawing fixture is added.
add_test(NAME libdxfrw_polyline_wcs_extrusion_loss_guard
    COMMAND libdxfrw_dxf_fixture_tests --polyline-wcs-extrusion-loss-guard)
set_tests_properties(libdxfrw_polyline_wcs_extrusion_loss_guard PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 30)
add_test(NAME libdxfrw_polyline_vertex_role_closure
    COMMAND libdxfrw_dxf_fixture_tests --polyline-vertex-role-closure)
set_tests_properties(libdxfrw_polyline_vertex_role_closure PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 30)

# The source and output are authored at runtime. This selected AC1027 profile
# proves consumed MLINE group-41 reprojection in ASCII/binary, plus atomic
# refusal and same-adapter retry controls; it adds no drawing fixture.
add_test(NAME libdxfrw_mline_consumed_offset_projection
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-offset-projection)
set_tests_properties(libdxfrw_mline_consumed_offset_projection PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 30)

# Generated at runtime from the selected AC1027 MLINE profile. This proves
# consumed-style offset reprojection with documented cap-suppression flags,
# exact physical group-71/group-41 output and semantic readback, refusal and
# rollback for closed/unknown flags, and same-adapter retry behavior.
add_test(NAME libdxfrw_mline_consumed_offset_cap_flags
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-offset-cap-flags)
set_tests_properties(libdxfrw_mline_consumed_offset_cap_flags PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 30)

# Generated AC1027 sources isolate the interaction between group 71 and
# group-72 vertex counts 2 and 4; the existing consumed-offset selector covers
# the three-vertex profile. These runtime sources add no drawing fixtures.
add_test(NAME libdxfrw_mline_vertex_flag_profiles
    COMMAND libdxfrw_dxf_fixture_tests --mline-vertex-flag-profiles)
set_tests_properties(libdxfrw_mline_vertex_flag_profiles PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 30)

# Runtime-authored simple fill boundaries are projected with the documented
# group-41 parameter rule. Cut intervals and mismatched profiles remain refused.
add_test(NAME libdxfrw_mline_fill_parameter_profiles
    COMMAND libdxfrw_dxf_fixture_tests --mline-fill-parameter-profiles)
set_tests_properties(libdxfrw_mline_fill_parameter_profiles PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 30)

# Generated at runtime from the selected AC1027 MLINE profiles. This proves
# offset reprojection when retargeting group 340, including documented
# start/end-cap suppression flags, exact redundant group-2/reference/geometry
# output, fresh readback, closed/unknown-flag refusal, rollback and same-adapter
# retry.
add_test(NAME libdxfrw_mline_retarget_offset_projection
    COMMAND libdxfrw_dxf_fixture_tests --mline-retarget-offset-projection)
set_tests_properties(libdxfrw_mline_retarget_offset_projection PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 30)

# Runtime-generated AC1027 retarget profile changes the target's start/end
# angles and verifies projections with documented MLINE cap-suppression flags.
# Check endpoint miters, group-41 values, exact output, and failure/retry.
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

# Runtime-generated AC1027 retarget from three- through thirty-two-element styles
# after the selected middle or all single-deletion removals. Verify projected
# parameters, exact references/readback and failure/retry controls.
add_test(NAME libdxfrw_mline_retarget_removal_projection
    COMMAND libdxfrw_dxf_fixture_tests --mline-retarget-removal-projection)
set_tests_properties(libdxfrw_mline_retarget_removal_projection PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)

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

# Runtime-generated AC1027 2-to-3 consumed-style append profiles preserve
# documented MLINE start/end-cap suppression flags. Prove ASCII/binary
# output/readback, closed/unknown refusal, rollback, and same-adapter retry.
add_test(NAME libdxfrw_mline_consumed_style_append_cap_flags
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-append-cap-flags)
set_tests_properties(libdxfrw_mline_consumed_style_append_cap_flags PROPERTIES
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

# Runtime-generated AC1027 2-to-1 consumed-style removal profiles preserve
# start/end-cap suppression flags and prove both survivor positions, exact
# parameter output/readback, closed/unknown refusal, rollback and retry.
add_test(NAME libdxfrw_mline_consumed_style_removal_cap_flags
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-removal-cap-flags)
set_tests_properties(libdxfrw_mline_consumed_style_removal_cap_flags PROPERTIES
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
add_test(NAME libdxfrw_mline_consumed_style_eight_from_eleven_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-eight-from-eleven-removal)
set_tests_properties(libdxfrw_mline_consumed_style_eight_from_eleven_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_nine_from_eleven_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-nine-from-eleven-removal)
set_tests_properties(libdxfrw_mline_consumed_style_nine_from_eleven_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_ten_from_eleven_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-ten-from-eleven-removal)
set_tests_properties(libdxfrw_mline_consumed_style_ten_from_eleven_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_one_from_twelve_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-one-from-twelve-removal)
set_tests_properties(libdxfrw_mline_consumed_style_one_from_twelve_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_two_from_twelve_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-two-from-twelve-removal)
set_tests_properties(libdxfrw_mline_consumed_style_two_from_twelve_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_three_from_twelve_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-three-from-twelve-removal)
set_tests_properties(libdxfrw_mline_consumed_style_three_from_twelve_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_four_from_twelve_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-four-from-twelve-removal)
set_tests_properties(libdxfrw_mline_consumed_style_four_from_twelve_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_five_from_twelve_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-five-from-twelve-removal)
set_tests_properties(libdxfrw_mline_consumed_style_five_from_twelve_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_six_from_twelve_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-six-from-twelve-removal)
set_tests_properties(libdxfrw_mline_consumed_style_six_from_twelve_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_seven_from_twelve_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-seven-from-twelve-removal)
set_tests_properties(libdxfrw_mline_consumed_style_seven_from_twelve_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_eight_from_twelve_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-eight-from-twelve-removal)
set_tests_properties(libdxfrw_mline_consumed_style_eight_from_twelve_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_nine_from_twelve_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-nine-from-twelve-removal)
set_tests_properties(libdxfrw_mline_consumed_style_nine_from_twelve_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_ten_from_twelve_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-ten-from-twelve-removal)
set_tests_properties(libdxfrw_mline_consumed_style_ten_from_twelve_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_eleven_from_twelve_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-eleven-from-twelve-removal)
set_tests_properties(libdxfrw_mline_consumed_style_eleven_from_twelve_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_one_from_thirteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-one-from-thirteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_one_from_thirteen_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_two_from_thirteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-two-from-thirteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_two_from_thirteen_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_three_from_thirteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-three-from-thirteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_three_from_thirteen_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_four_from_thirteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-four-from-thirteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_four_from_thirteen_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_five_from_thirteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-five-from-thirteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_five_from_thirteen_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)
add_test(NAME libdxfrw_mline_consumed_style_six_from_thirteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-six-from-thirteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_six_from_thirteen_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)
add_test(NAME libdxfrw_mline_consumed_style_seven_from_thirteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-seven-from-thirteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_seven_from_thirteen_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)
add_test(NAME libdxfrw_mline_consumed_style_eight_from_thirteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-eight-from-thirteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_eight_from_thirteen_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)
add_test(NAME libdxfrw_mline_consumed_style_nine_from_thirteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-nine-from-thirteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_nine_from_thirteen_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)
add_test(NAME libdxfrw_mline_consumed_style_ten_from_thirteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-ten-from-thirteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_ten_from_thirteen_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)
add_test(NAME libdxfrw_mline_consumed_style_eleven_from_thirteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-eleven-from-thirteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_eleven_from_thirteen_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)
add_test(NAME libdxfrw_mline_consumed_style_twelve_from_thirteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-twelve-from-thirteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_twelve_from_thirteen_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)
add_test(NAME libdxfrw_mline_consumed_style_one_from_fourteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-one-from-fourteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_one_from_fourteen_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)
add_test(NAME libdxfrw_mline_consumed_style_two_from_fourteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-two-from-fourteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_two_from_fourteen_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)
add_test(NAME libdxfrw_mline_consumed_style_three_from_fourteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-three-from-fourteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_three_from_fourteen_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)
add_test(NAME libdxfrw_mline_consumed_style_four_from_fourteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-four-from-fourteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_four_from_fourteen_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)
add_test(NAME libdxfrw_mline_consumed_style_five_from_fourteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-five-from-fourteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_five_from_fourteen_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 180)
add_test(NAME libdxfrw_mline_consumed_style_six_from_fourteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-six-from-fourteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_six_from_fourteen_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 240)
add_test(NAME libdxfrw_mline_consumed_style_seven_from_fourteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-seven-from-fourteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_seven_from_fourteen_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 240)
add_test(NAME libdxfrw_mline_consumed_style_eight_from_fourteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-eight-from-fourteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_eight_from_fourteen_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 240)
add_test(NAME libdxfrw_mline_consumed_style_nine_from_fourteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-nine-from-fourteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_nine_from_fourteen_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 240)
add_test(NAME libdxfrw_mline_consumed_style_ten_from_fourteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-ten-from-fourteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_ten_from_fourteen_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 180)
add_test(NAME libdxfrw_mline_consumed_style_eleven_from_fourteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-eleven-from-fourteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_eleven_from_fourteen_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 120)
add_test(NAME libdxfrw_mline_consumed_style_twelve_from_fourteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-twelve-from-fourteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_twelve_from_fourteen_removal PROPERTIES
    LABELS "dxf;fast;transaction"
    TIMEOUT 90)
add_test(NAME libdxfrw_mline_consumed_style_thirteen_from_fourteen_removal
    COMMAND libdxfrw_dxf_fixture_tests --mline-consumed-style-thirteen-from-fourteen-removal)
set_tests_properties(libdxfrw_mline_consumed_style_thirteen_from_fourteen_removal PROPERTIES
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
add_executable(libdxfrw_dwg_xrecord_verified_source_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_xrecord_verified_source_tests.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_dwg_xrecord_verified_source_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_xrecord_verified_source_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_xrecord_verified_source_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_xrecord_verified_source_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_xrecord_verified_source
    COMMAND libdxfrw_dwg_xrecord_verified_source_tests
        ${CMAKE_CURRENT_BINARY_DIR}/generated-xrecord-verified-source.dwg)
set_tests_properties(libdxfrw_dwg_xrecord_verified_source PROPERTIES
    LABELS "dwg;fast;xrecord" TIMEOUT 30)
add_executable(libdxfrw_dwg_datatable_ac1018_candidate_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_datatable_ac1018_candidate_tests.cpp)
target_compile_features(libdxfrw_dwg_datatable_ac1018_candidate_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_datatable_ac1018_candidate_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_datatable_ac1018_candidate_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_datatable_ac1018_candidate_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_datatable_ac1018_candidate
    COMMAND libdxfrw_dwg_datatable_ac1018_candidate_tests)
set_tests_properties(libdxfrw_dwg_datatable_ac1018_candidate PROPERTIES
    LABELS "dwg;fast;datatable" TIMEOUT 30)
add_executable(libdxfrw_dwg_tablegeometry_ac1018_candidate_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_tablegeometry_ac1018_candidate_tests.cpp)
target_compile_features(libdxfrw_dwg_tablegeometry_ac1018_candidate_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_tablegeometry_ac1018_candidate_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_tablegeometry_ac1018_candidate_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_tablegeometry_ac1018_candidate_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_tablegeometry_ac1018_candidate
    COMMAND libdxfrw_dwg_tablegeometry_ac1018_candidate_tests)
set_tests_properties(libdxfrw_dwg_tablegeometry_ac1018_candidate PROPERTIES
    LABELS "dwg;fast;tablegeometry" TIMEOUT 30)
add_executable(libdxfrw_dwg_table_roundtrip_xrecord_ac1018_candidate_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_table_roundtrip_xrecord_ac1018_candidate_tests.cpp)
target_compile_features(libdxfrw_dwg_table_roundtrip_xrecord_ac1018_candidate_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_table_roundtrip_xrecord_ac1018_candidate_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_table_roundtrip_xrecord_ac1018_candidate_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_table_roundtrip_xrecord_ac1018_candidate_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_table_roundtrip_xrecord_ac1018_candidate
    COMMAND libdxfrw_dwg_table_roundtrip_xrecord_ac1018_candidate_tests)
set_tests_properties(libdxfrw_dwg_table_roundtrip_xrecord_ac1018_candidate PROPERTIES
    LABELS "dwg;fast;xrecord" TIMEOUT 30)
add_executable(libdxfrw_dwg_tablecontent_ac1018_candidate_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_tablecontent_ac1018_candidate_tests.cpp)
target_compile_features(libdxfrw_dwg_tablecontent_ac1018_candidate_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_tablecontent_ac1018_candidate_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_tablecontent_ac1018_candidate_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_tablecontent_ac1018_candidate_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_tablecontent_ac1018_candidate
    COMMAND libdxfrw_dwg_tablecontent_ac1018_candidate_tests)
set_tests_properties(libdxfrw_dwg_tablecontent_ac1018_candidate PROPERTIES
    LABELS "dwg;fast;tablecontent" TIMEOUT 30)
add_executable(libdxfrw_dwg_table_candidate_binding_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_table_candidate_binding_tests.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_dwg_table_candidate_binding_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_table_candidate_binding_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_table_candidate_binding_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_table_candidate_binding_tests PRIVATE dxfrw)
add_dependencies(libdxfrw_dwg_table_candidate_binding_tests
    libdxfrw_dwg_tablecontent_ac1018_candidate_tests
    libdxfrw_dwg_datatable_ac1018_candidate_tests
    libdxfrw_dwg_tablegeometry_ac1018_candidate_tests
    libdxfrw_dwg_table_roundtrip_xrecord_ac1018_candidate_tests)
add_test(NAME libdxfrw_dwg_table_candidate_binding
    COMMAND ${CMAKE_COMMAND}
        -DCONTENT_TEST=$<TARGET_FILE:libdxfrw_dwg_tablecontent_ac1018_candidate_tests>
        -DDATA_TEST=$<TARGET_FILE:libdxfrw_dwg_datatable_ac1018_candidate_tests>
        -DGEOMETRY_TEST=$<TARGET_FILE:libdxfrw_dwg_tablegeometry_ac1018_candidate_tests>
        -DROOT_TEST=$<TARGET_FILE:libdxfrw_dwg_table_roundtrip_xrecord_ac1018_candidate_tests>
        -DBIND_TEST=$<TARGET_FILE:libdxfrw_dwg_table_candidate_binding_tests>
        -DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/generated-table-binding
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg_table_candidate_binding.cmake)
set_tests_properties(libdxfrw_dwg_table_candidate_binding PROPERTIES
    LABELS "dwg;fast;tablecontent;datatable" TIMEOUT 30)
add_executable(libdxfrw_underlay_objects_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/underlay_objects_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source}
)
add_executable(libdxfrw_material_objects_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/material_objects_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source}
)
target_compile_features(libdxfrw_material_objects_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_material_objects_allocation_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_material_objects_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(libdxfrw_material_objects_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_material_objects_allocation_retry
    COMMAND libdxfrw_material_objects_allocation_tests
        "${CMAKE_CURRENT_BINARY_DIR}/material-objects-allocation-scratch")
set_tests_properties(libdxfrw_material_objects_allocation_retry PROPERTIES
    LABELS "fast;dxf;material;allocation;transaction;generated"
    TIMEOUT 60)
add_executable(libdxfrw_plotsettings_objects_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/plotsettings_objects_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source}
)
target_compile_features(libdxfrw_plotsettings_objects_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_plotsettings_objects_allocation_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_plotsettings_objects_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(libdxfrw_plotsettings_objects_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_plotsettings_objects_allocation_retry
    COMMAND libdxfrw_plotsettings_objects_allocation_tests
        "${CMAKE_CURRENT_BINARY_DIR}/plotsettings-objects-allocation-scratch")
set_tests_properties(libdxfrw_plotsettings_objects_allocation_retry PROPERTIES
    LABELS "fast;dxf;plotsettings;allocation;transaction;generated"
    TIMEOUT 60)
add_executable(libdxfrw_geodata_objects_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/geodata_objects_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source}
)
target_compile_features(libdxfrw_geodata_objects_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_geodata_objects_allocation_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_geodata_objects_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(libdxfrw_geodata_objects_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_geodata_objects_allocation_retry
    COMMAND libdxfrw_geodata_objects_allocation_tests
        "${CMAKE_CURRENT_BINARY_DIR}/geodata-objects-allocation-scratch")
set_tests_properties(libdxfrw_geodata_objects_allocation_retry PROPERTIES
    LABELS "fast;dxf;geodata;allocation;transaction;generated"
    TIMEOUT 60)
add_executable(libdxfrw_dbcolor_objects_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dbcolor_objects_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source}
)
target_compile_features(libdxfrw_dbcolor_objects_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dbcolor_objects_allocation_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dbcolor_objects_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(libdxfrw_dbcolor_objects_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dbcolor_objects_allocation_retry
    COMMAND libdxfrw_dbcolor_objects_allocation_tests
        "${CMAKE_CURRENT_BINARY_DIR}/dbcolor-objects-allocation-scratch")
set_tests_properties(libdxfrw_dbcolor_objects_allocation_retry PROPERTIES
    LABELS "fast;dxf;dbcolor;allocation;transaction;generated"
    TIMEOUT 60)
add_executable(libdxfrw_background_objects_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/background_objects_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source}
)
target_compile_features(libdxfrw_background_objects_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_background_objects_allocation_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_background_objects_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(libdxfrw_background_objects_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_background_objects_allocation_retry
    COMMAND libdxfrw_background_objects_allocation_tests
        "${CMAKE_CURRENT_BINARY_DIR}/background-objects-allocation-scratch")
set_tests_properties(libdxfrw_background_objects_allocation_retry PROPERTIES
    LABELS "fast;dxf;background;allocation;transaction;generated"
    TIMEOUT 60)
add_executable(libdxfrw_spatial_filter_objects_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/spatial_filter_objects_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source}
)
target_compile_features(libdxfrw_spatial_filter_objects_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_spatial_filter_objects_allocation_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_spatial_filter_objects_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(libdxfrw_spatial_filter_objects_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_spatial_filter_objects_allocation_retry
    COMMAND libdxfrw_spatial_filter_objects_allocation_tests
        "${CMAKE_CURRENT_BINARY_DIR}/spatial-filter-objects-allocation-scratch")
set_tests_properties(libdxfrw_spatial_filter_objects_allocation_retry PROPERTIES
    LABELS "fast;dxf;spatial-filter;allocation;transaction;generated"
    TIMEOUT 60)
add_executable(libdxfrw_partial_viewing_index_objects_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/partial_viewing_index_objects_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source}
)
target_compile_features(libdxfrw_partial_viewing_index_objects_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_partial_viewing_index_objects_allocation_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_partial_viewing_index_objects_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(libdxfrw_partial_viewing_index_objects_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_partial_viewing_index_objects_allocation_retry
    COMMAND libdxfrw_partial_viewing_index_objects_allocation_tests
        "${CMAKE_CURRENT_BINARY_DIR}/partial-viewing-index-objects-allocation-scratch")
set_tests_properties(libdxfrw_partial_viewing_index_objects_allocation_retry PROPERTIES
    LABELS "fast;dxf;field;fieldlist;motion-path;curve-path;point-path;objectptr;partial-viewing-index;allocation;transaction;generated"
    TIMEOUT 60)
add_executable(libdxfrw_scale_objects_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/scale_objects_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source}
)
target_compile_features(libdxfrw_scale_objects_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_scale_objects_allocation_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_scale_objects_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(libdxfrw_scale_objects_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_scale_objects_allocation_retry
    COMMAND libdxfrw_scale_objects_allocation_tests
        "${CMAKE_CURRENT_BINARY_DIR}/scale-objects-allocation-scratch")
set_tests_properties(libdxfrw_scale_objects_allocation_retry PROPERTIES
    LABELS "fast;dxf;scale;allocation;transaction;generated"
    TIMEOUT 60)
add_executable(libdxfrw_dictionaryvar_objects_version_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dictionaryvar_objects_version_allocation_tests.cpp)
target_compile_features(libdxfrw_dictionaryvar_objects_version_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dictionaryvar_objects_version_allocation_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dictionaryvar_objects_version_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dictionaryvar_objects_version_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dictionaryvar_objects_version_allocation_retry
    COMMAND libdxfrw_dictionaryvar_objects_version_allocation_tests
        "${CMAKE_CURRENT_BINARY_DIR}/dictionaryvar-objects-version-allocation-scratch")
set_tests_properties(libdxfrw_dictionaryvar_objects_version_allocation_retry PROPERTIES
    LABELS "fast;dxf;dictionaryvar;allocation;transaction;generated"
    TIMEOUT 60)
add_executable(libdxfrw_datalink_objects_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/datalink_objects_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source}
)
target_compile_features(libdxfrw_datalink_objects_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_datalink_objects_allocation_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_datalink_objects_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(libdxfrw_datalink_objects_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_datalink_objects_allocation_retry
    COMMAND libdxfrw_datalink_objects_allocation_tests
        "${CMAKE_CURRENT_BINARY_DIR}/datalink-objects-allocation-scratch")
set_tests_properties(libdxfrw_datalink_objects_allocation_retry PROPERTIES
    LABELS "fast;dxf;datalink;allocation;transaction;generated"
    TIMEOUT 60)
add_executable(libdxfrw_tablecontent_objects_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tablecontent_objects_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source}
)
target_compile_features(libdxfrw_tablecontent_objects_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_tablecontent_objects_allocation_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_tablecontent_objects_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/intern
)
target_link_libraries(libdxfrw_tablecontent_objects_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_tablecontent_objects_allocation_retry
    COMMAND libdxfrw_tablecontent_objects_allocation_tests
        "${CMAKE_CURRENT_BINARY_DIR}/tablecontent-objects-allocation-scratch")
set_tests_properties(libdxfrw_tablecontent_objects_allocation_retry PROPERTIES
    LABELS "fast;dxf;tablecontent;allocation;transaction;generated"
    TIMEOUT 60)
add_executable(libdxfrw_associative_network_objects_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/associative_network_objects_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source}
)
target_compile_features(libdxfrw_associative_network_objects_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_associative_network_objects_allocation_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_associative_network_objects_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(libdxfrw_associative_network_objects_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_associative_network_objects_allocation_retry
    COMMAND libdxfrw_associative_network_objects_allocation_tests
        "${CMAKE_CURRENT_BINARY_DIR}/associative-network-objects-allocation-scratch")
set_tests_properties(libdxfrw_associative_network_objects_allocation_retry PROPERTIES
    LABELS "fast;dxf;associative;allocation;transaction;generated"
    TIMEOUT 60)
add_executable(libdxfrw_associative_dependency_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/associative_dependency_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source}
)
target_compile_features(libdxfrw_associative_dependency_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_associative_dependency_allocation_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_associative_dependency_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(libdxfrw_associative_dependency_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_associative_dependency_allocation_retry
    COMMAND libdxfrw_associative_dependency_allocation_tests
        "${CMAKE_CURRENT_BINARY_DIR}/associative-dependency-objects-scratch")
set_tests_properties(libdxfrw_associative_dependency_allocation_retry PROPERTIES
    LABELS "fast;dxf;associative;allocation;transaction;generated"
    TIMEOUT 60)
add_executable(libdxfrw_associative_geometry_graph_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/associative_geometry_graph_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source}
)
target_compile_features(libdxfrw_associative_geometry_graph_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_associative_geometry_graph_allocation_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_associative_geometry_graph_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(libdxfrw_associative_geometry_graph_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_associative_geometry_graph_allocation_retry
    COMMAND libdxfrw_associative_geometry_graph_allocation_tests
        "${CMAKE_CURRENT_BINARY_DIR}/associative-geometry-graph-allocation-scratch")
set_tests_properties(libdxfrw_associative_geometry_graph_allocation_retry PROPERTIES
    LABELS "fast;dxf;associative;allocation;transaction;generated"
    TIMEOUT 60)
add_executable(libdxfrw_dictionary_default_objects_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dictionary_default_objects_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source}
)
target_compile_features(libdxfrw_dictionary_default_objects_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dictionary_default_objects_allocation_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dictionary_default_objects_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(libdxfrw_dictionary_default_objects_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dictionary_default_objects_allocation_retry
    COMMAND libdxfrw_dictionary_default_objects_allocation_tests
        "${CMAKE_CURRENT_BINARY_DIR}/dictionary-default-objects-allocation-scratch")
set_tests_properties(libdxfrw_dictionary_default_objects_allocation_retry PROPERTIES
    LABELS "fast;dxf;dictionary-default;allocation;transaction;generated"
    TIMEOUT 60)
add_executable(libdxfrw_image_writer_version_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/image_writer_version_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source}
)
target_compile_features(libdxfrw_image_writer_version_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_image_writer_version_allocation_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_image_writer_version_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(libdxfrw_image_writer_version_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_image_writer_version_allocation
    COMMAND libdxfrw_image_writer_version_allocation_tests
        "${CMAKE_CURRENT_BINARY_DIR}/image-writer-version-allocation-scratch")
set_tests_properties(libdxfrw_image_writer_version_allocation PROPERTIES
    LABELS "fast;dxf;image;allocation;transaction;generated"
    TIMEOUT 60)
target_compile_features(libdxfrw_underlay_objects_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_underlay_objects_allocation_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_underlay_objects_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(libdxfrw_underlay_objects_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_underlay_objects_allocation_retry
    COMMAND libdxfrw_underlay_objects_allocation_tests
        "${CMAKE_CURRENT_BINARY_DIR}/underlay-objects-allocation-scratch")
set_tests_properties(libdxfrw_underlay_objects_allocation_retry PROPERTIES
    LABELS "fast;dxf;underlay;allocation;transaction;generated"
    TIMEOUT 60)
add_executable(libdxfrw_wipeout_dwg_proxy_replay_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/wipeout_dwg_proxy_replay_tests.cpp
    ${_libdxfrw_test_adapter_source}
)
target_compile_features(libdxfrw_wipeout_dwg_proxy_replay_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_wipeout_dwg_proxy_replay_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_wipeout_dwg_proxy_replay_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_wipeout_dwg_proxy_replay_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_wipeout_dwg_proxy_replay COMMAND libdxfrw_wipeout_dwg_proxy_replay_tests)
set_tests_properties(libdxfrw_wipeout_dwg_proxy_replay PROPERTIES LABELS "fast;dwg;dxf;wipeout;transaction" TIMEOUT 30)
target_compile_features(libdxfrw_dwg2dxf_adapter_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg2dxf_adapter_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg2dxf_adapter_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(libdxfrw_dwg2dxf_adapter_tests PRIVATE dxfrw)
set(_modeler_wireframe_sample
    "${CMAKE_CURRENT_SOURCE_DIR}/samples/AC1024/visualization_-_aerial.dwg")
if(EXISTS "${_modeler_wireframe_sample}")
    add_test(NAME libdxfrw_dwg_ac1024_modeler_wireframe_presence
        COMMAND libdxfrw_dwg2dxf_adapter_tests
            --modeler-ac1024-wireframe-presence "${_modeler_wireframe_sample}")
    set_tests_properties(libdxfrw_dwg_ac1024_modeler_wireframe_presence
        PROPERTIES LABELS "fast;dwg;AC1024;modeler;wireframe;physical" TIMEOUT 30)
endif()
if(TARGET dwg2dxf)
add_test(NAME dwg2dxf_generated_ac1021_group_parent_reactor_closure
    COMMAND ${CMAKE_COMMAND}
        "-DAUTHOR=$<TARGET_FILE:libdxfrw_dwg_ac1021_polyline_group_targets_tests>"
        "-DADAPTER=$<TARGET_FILE:libdxfrw_dwg2dxf_adapter_tests>"
        "-DCONVERTER=$<TARGET_FILE:dwg2dxf>"
        "-DREADBACK=$<TARGET_FILE:libdxfrw_ac1021_group_polyline_dxf_readback_tests>"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_ac1021_group_reactor_closure_test.cmake)
set_tests_properties(dwg2dxf_generated_ac1021_group_parent_reactor_closure
    PROPERTIES LABELS "fast;dwg;dxf;group;adapter;transaction;generated;AC1021"
    TIMEOUT 45)
add_test(NAME dwg2dxf_dimassoc_adapter_closure_refusal
    COMMAND ${CMAKE_COMMAND}
        "-DAUTHOR=$<TARGET_FILE:libdxfrw_dimassoc_physical_transport_tests>"
        "-DCONVERTER=$<TARGET_FILE:dwg2dxf>"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dimassoc_adapter_closure_test.cmake)
set_tests_properties(dwg2dxf_dimassoc_adapter_closure_refusal PROPERTIES
    LABELS "fast;dwg;dxf;dimassoc;adapter;graph;generated;transaction"
    TIMEOUT 30)
endif()
add_test(NAME dwg2dxf_generated_dimassoc_graph_node
    COMMAND libdxfrw_dwg2dxf_adapter_tests --generated-dimassoc-graph)
set_tests_properties(dwg2dxf_generated_dimassoc_graph_node PROPERTIES
    LABELS "fast;dwg;dxf;dimassoc;adapter;graph;generated;retry"
    TIMEOUT 15)
add_test(NAME dwg2dxf_dimassoc_stale_generation_ignored
    COMMAND libdxfrw_dimassoc_physical_transport_tests
        --stale-generation-output
        "${CMAKE_CURRENT_BINARY_DIR}/dimassoc-stale-generation-scratch.dxf")
set_tests_properties(dwg2dxf_dimassoc_stale_generation_ignored PROPERTIES
    LABELS "fast;dwg;dxf;dimassoc;adapter;generation;generated"
    TIMEOUT 30)
add_test(NAME dwg2dxf_dimassoc_adapter_closure_retry_ascii
    COMMAND libdxfrw_dimassoc_physical_transport_tests
        --adapter-closure-retry-ascii
        "${CMAKE_CURRENT_BINARY_DIR}/dimassoc-adapter-retry-ascii-scratch.dxf")
set_tests_properties(dwg2dxf_dimassoc_adapter_closure_retry_ascii PROPERTIES
    LABELS "fast;dwg;dxf;dimassoc;adapter;graph;retry;generated"
    TIMEOUT 30)
add_test(NAME dwg2dxf_dimassoc_adapter_closure_retry_binary
    COMMAND libdxfrw_dimassoc_physical_transport_tests
        --adapter-closure-retry-binary
        "${CMAKE_CURRENT_BINARY_DIR}/dimassoc-adapter-retry-binary-scratch.dxf")
set_tests_properties(dwg2dxf_dimassoc_adapter_closure_retry_binary PROPERTIES
    LABELS "fast;dwg;dxf;dimassoc;adapter;graph;retry;generated"
    TIMEOUT 30)
add_test(NAME libdxfrw_dimassoc_dxf_allocation_rollback_retry
    COMMAND libdxfrw_dimassoc_physical_transport_tests
        --allocation-rollback-retry)
set_tests_properties(libdxfrw_dimassoc_dxf_allocation_rollback_retry PROPERTIES
    LABELS "fast;dxf;dimassoc;allocation;transaction;generated"
    TIMEOUT 120)
add_test(NAME dwg2dxf_sunstudy_dxf_graph
    COMMAND ${CMAKE_COMMAND}
        "-DADAPTER_TEST=$<TARGET_FILE:libdxfrw_dwg2dxf_adapter_tests>"
        "-DPYTHON=${LIBDXFRW_PYTHON_EXECUTABLE}"
        "-DAUTHOR=${CMAKE_SOURCE_DIR}/tools/audit_sunstudy_dxf_graph.py"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_sunstudy_dxf_graph_test.cmake)
set_tests_properties(dwg2dxf_sunstudy_dxf_graph PROPERTIES LABELS "fast;dxf;graph;adapter;transaction" TIMEOUT 30)
add_executable(libdxfrw_sunstudy_objects_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/sunstudy_objects_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source}
)
target_compile_features(libdxfrw_sunstudy_objects_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_sunstudy_objects_allocation_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_sunstudy_objects_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(libdxfrw_sunstudy_objects_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_sunstudy_objects_allocation_retry
    COMMAND ${CMAKE_COMMAND}
        "-DALLOCATION_TEST=$<TARGET_FILE:libdxfrw_sunstudy_objects_allocation_tests>"
        "-DPYTHON=${LIBDXFRW_PYTHON_EXECUTABLE}"
        "-DAUTHOR=${CMAKE_SOURCE_DIR}/tools/audit_sunstudy_dxf_graph.py"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_sunstudy_objects_allocation_test.cmake)
set_tests_properties(libdxfrw_sunstudy_objects_allocation_retry PROPERTIES
    LABELS "fast;dxf;sunstudy;allocation;transaction;generated"
    TIMEOUT 60)
foreach(_sunstudy_version IN ITEMS AC1024 AC1032)
    add_test(NAME dwg2dxf_sunstudy_dxf_graph_${_sunstudy_version}
        COMMAND ${CMAKE_COMMAND}
            "-DADAPTER_TEST=$<TARGET_FILE:libdxfrw_dwg2dxf_adapter_tests>"
            "-DPYTHON=${LIBDXFRW_PYTHON_EXECUTABLE}"
            "-DAUTHOR=${CMAKE_SOURCE_DIR}/tools/audit_sunstudy_dxf_graph.py"
            "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
            "-DVERSION=${_sunstudy_version}"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_sunstudy_dxf_graph_test.cmake)
    set_tests_properties(dwg2dxf_sunstudy_dxf_graph_${_sunstudy_version} PROPERTIES
        LABELS "fast;dxf;graph;adapter;transaction" TIMEOUT 30)
endforeach()
add_test(NAME libdxfrw_ac1018_minimal_dictionary_source
    COMMAND ${CMAKE_COMMAND}
        "-DADAPTER_TEST=$<TARGET_FILE:libdxfrw_dwg2dxf_adapter_tests>"
        "-DWRITER=$<TARGET_FILE:libdxfrw_dwg_local_roundtrip>"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_ac1018_minimal_dictionary_test.cmake)
set_tests_properties(libdxfrw_ac1018_minimal_dictionary_source PROPERTIES
    LABELS "fast;dwg;dxf;graph;adapter;transaction" TIMEOUT 45)
add_test(NAME libdxfrw_ucs_base_name_target_closure
    COMMAND libdxfrw_dwg2dxf_adapter_tests --ucs-base-name-target-closure)
set_tests_properties(libdxfrw_ucs_base_name_target_closure PROPERTIES
    LABELS "fast;dxf;header;graph;adapter;transaction" TIMEOUT 60)
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
add_test(NAME libdxfrw_fieldlist_membership_output
    COMMAND libdxfrw_dwg2dxf_adapter_tests --fieldlist-membership-output)
set_tests_properties(libdxfrw_fieldlist_membership_output PROPERTIES
    LABELS "fast;dxf;graph;adapter;field;transaction" TIMEOUT 30)
add_test(NAME libdxfrw_extension_dictionary_owner_graphs
    COMMAND libdxfrw_dwg2dxf_adapter_tests --extension-dictionary-owner-graphs)
set_tests_properties(libdxfrw_extension_dictionary_owner_graphs PROPERTIES
    LABELS "fast;dxf;graph;adapter;objects;transaction" TIMEOUT 30)
add_test(NAME libdxfrw_ase_hierarchy_ac1021_insert_target_roundtrip
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --ase-hierarchy-generated-ac1021-insert-targets)
set_tests_properties(libdxfrw_ase_hierarchy_ac1021_insert_target_roundtrip
    PROPERTIES LABELS "fast;dxf;graph;adapter;objects;transaction" TIMEOUT 60)
add_test(NAME libdxfrw_ase_hierarchy_ac1021_empty_cache_roundtrip
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --ase-hierarchy-generated-ac1021-empty-cache)
set_tests_properties(libdxfrw_ase_hierarchy_ac1021_empty_cache_roundtrip
    PROPERTIES LABELS "fast;dxf;graph;adapter;objects;transaction" TIMEOUT 30)
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
add_test(NAME libdxfrw_table_dwg_proxy_graphics_roundtrip
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --table-dwg-proxy-graphics-roundtrip)
set_tests_properties(libdxfrw_table_dwg_proxy_graphics_roundtrip PROPERTIES
    LABELS "fast;dxf;dwg;adapter;tables;proxy-graphics;generated;readback"
    TIMEOUT 30)
add_test(NAME libdxfrw_table_fallback_runtime_dispatch
    COMMAND libdxfrw_dwg2dxf_adapter_tests --table-fallback-runtime-type)
set_tests_properties(libdxfrw_table_fallback_runtime_dispatch PROPERTIES
    LABELS "fast;dxf;adapter;table;transaction;readback" TIMEOUT 30)
add_test(NAME libdxfrw_tablestyle_nested_raw_net
    COMMAND libdxfrw_dwg2dxf_adapter_tests --tablestyle-nested-raw-net)
set_tests_properties(libdxfrw_tablestyle_nested_raw_net PROPERTIES
    LABELS "fast;dxf;tablestyle;generated;raw-net" TIMEOUT 30)
add_test(NAME libdxfrw_tablestyle_r24_typed_writer
    COMMAND libdxfrw_dwg2dxf_adapter_tests --tablestyle-r24-typed-writer)
set_tests_properties(libdxfrw_tablestyle_r24_typed_writer PROPERTIES
    LABELS "fast;dxf;adapter;tablestyle;generated;transaction" TIMEOUT 30)
add_test(NAME libdxfrw_tablestyle_legacy_ordered_writer
    COMMAND libdxfrw_dwg2dxf_adapter_tests --tablestyle-legacy-ordered-writer)
set_tests_properties(libdxfrw_tablestyle_legacy_ordered_writer PROPERTIES
    LABELS "fast;dxf;adapter;tablestyle;generated;order;readback" TIMEOUT 30)
add_test(NAME libdxfrw_table_scalar_override_output
    COMMAND libdxfrw_dwg2dxf_adapter_tests --table-scalar-override-output)
set_tests_properties(libdxfrw_table_scalar_override_output PROPERTIES
    LABELS "fast;dxf;adapter;tables;generated;readback;refusal;retry"
    TIMEOUT 30)
add_test(NAME libdxfrw_visual_style_import_evidence
    COMMAND libdxfrw_dwg2dxf_adapter_tests --visual-style-import-evidence
        ${CMAKE_CURRENT_BINARY_DIR})
set_tests_properties(libdxfrw_visual_style_import_evidence PROPERTIES
    LABELS "fast;dxf;graph;adapter;visualstyle" TIMEOUT 30)
add_test(NAME libdxfrw_visual_style_ac1024_typed_writer
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --visual-style-ac1024-typed-writer
        ${CMAKE_CURRENT_BINARY_DIR})
set_tests_properties(libdxfrw_visual_style_ac1024_typed_writer PROPERTIES
    LABELS "fast;dxf;graph;adapter;visualstyle;generated;transaction"
    TIMEOUT 30)
add_test(NAME libdxfrw_visual_style_ac1024_extension_profile
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --visual-style-ac1024-extension-profile)
set_tests_properties(libdxfrw_visual_style_ac1024_extension_profile PROPERTIES
    LABELS "fast;dxf;graph;adapter;visualstyle;generated" TIMEOUT 30)
add_test(NAME libdxfrw_visual_style_ac1021_typed_writer
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --visual-style-ac1021-typed-writer
        ${CMAKE_CURRENT_BINARY_DIR})
set_tests_properties(libdxfrw_visual_style_ac1021_typed_writer PROPERTIES
    LABELS "fast;dxf;graph;adapter;visualstyle;generated;transaction"
    TIMEOUT 30)
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
add_test(NAME dwg2dxf_action_path_object_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests --action-path-object-capture-limits)
set_tests_properties(dwg2dxf_action_path_object_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;objects" TIMEOUT 10)
add_test(NAME dwg2dxf_sunstudy_callback_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests --sunstudy-callback-capture-limits)
set_tests_properties(dwg2dxf_sunstudy_callback_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;objects;rendering" TIMEOUT 10)
add_test(NAME dwg2dxf_lightlist_callback_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests --lightlist-callback-capture-limits)
set_tests_properties(dwg2dxf_lightlist_callback_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;objects;rendering" TIMEOUT 10)
add_test(NAME dwg2dxf_geomapimage_callback_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests --geomapimage-callback-capture-limits)
set_tests_properties(dwg2dxf_geomapimage_callback_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;objects;rendering" TIMEOUT 10)
add_test(NAME dwg2dxf_hatch_mpolygon_runtime_dispatch
    COMMAND libdxfrw_dwg2dxf_adapter_tests --hatch-mpolygon-runtime-dispatch)
set_tests_properties(dwg2dxf_hatch_mpolygon_runtime_dispatch PROPERTIES
    LABELS "fast;dxf;adapter;entities;hatch;mpolygon;transaction"
    TIMEOUT 15)
add_test(NAME dwg2dxf_viewport_runtime_dispatch
    COMMAND libdxfrw_dwg2dxf_adapter_tests --viewport-runtime-dispatch)
set_tests_properties(dwg2dxf_viewport_runtime_dispatch PROPERTIES
    LABELS "fast;dxf;adapter;entities;transaction;viewport" TIMEOUT 15)
add_test(NAME dwg2dxf_insert_runtime_dispatch
    COMMAND libdxfrw_dwg2dxf_adapter_tests --insert-runtime-dispatch)
set_tests_properties(dwg2dxf_insert_runtime_dispatch PROPERTIES
    LABELS "fast;dxf;adapter;entities;transaction;insert" TIMEOUT 15)
add_test(NAME dwg2dxf_entity_runtime_dispatch
    COMMAND libdxfrw_dwg2dxf_adapter_tests --entity-runtime-dispatch)
set_tests_properties(dwg2dxf_entity_runtime_dispatch PROPERTIES
    LABELS "fast;dxf;adapter;entities;transaction;runtime-dispatch" TIMEOUT 15)
add_test(NAME dwg2dxf_point_cloud_adapter_transport
    COMMAND libdxfrw_dwg2dxf_adapter_tests --point-cloud-adapter-transport)
set_tests_properties(dwg2dxf_point_cloud_adapter_transport PROPERTIES
    LABELS "fast;dxf;adapter;entities;pointcloud;navisworks;transaction"
    TIMEOUT 20)
add_test(NAME libdxfrw_tolerance_dimstyle_reference_binding
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --tolerance-dimstyle-reference-binding)
set_tests_properties(libdxfrw_tolerance_dimstyle_reference_binding PROPERTIES
    LABELS "fast;dwg;dxf;adapter;entities;references;annotation;transaction"
    TIMEOUT 30)
add_test(NAME libdxfrw_dimension_dimstyle_reference_binding
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --dimension-dimstyle-reference-binding)
set_tests_properties(libdxfrw_dimension_dimstyle_reference_binding PROPERTIES
    LABELS "fast;dwg;dxf;adapter;entities;references;annotation;transaction"
    TIMEOUT 30)
add_test(NAME libdxfrw_leader_dimstyle_reference_binding
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --leader-dimstyle-reference-binding)
set_tests_properties(libdxfrw_leader_dimstyle_reference_binding PROPERTIES
    LABELS "fast;dwg;dxf;adapter;entities;references;annotation;transaction"
    TIMEOUT 30)
add_test(NAME dwg2dxf_proxy_entity_output_gate
    COMMAND libdxfrw_dwg2dxf_adapter_tests --dwg-proxy-entity-output-gate)
set_tests_properties(dwg2dxf_proxy_entity_output_gate PROPERTIES
    LABELS "fast;dxf;adapter;entities;proxy;transaction" TIMEOUT 15)
add_test(NAME dwg2dxf_light_adapter_transport
    COMMAND libdxfrw_dwg2dxf_adapter_tests --light-adapter-transport)
set_tests_properties(dwg2dxf_light_adapter_transport PROPERTIES
    LABELS "fast;dxf;adapter;entities;light;transaction" TIMEOUT 15)
add_test(NAME dwg2dxf_camera_view_reference_closure
    COMMAND libdxfrw_dwg2dxf_adapter_tests --camera-view-reference-closure)
set_tests_properties(dwg2dxf_camera_view_reference_closure PROPERTIES
    LABELS "fast;dxf;adapter;entities;camera;references;transaction" TIMEOUT 15)
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
add_test(NAME dwg2dxf_dwg_typed_sun_output
    COMMAND libdxfrw_dwg2dxf_adapter_tests --dwg-typed-sun-output)
set_tests_properties(dwg2dxf_dwg_typed_sun_output PROPERTIES
    LABELS "fast;dxf;adapter;objects" TIMEOUT 30)
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
add_test(NAME libdxfrw_mleaderstyle_boolean_source_domain
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --mleaderstyle-boolean-source-domain)
set_tests_properties(libdxfrw_mleaderstyle_boolean_source_domain PROPERTIES
    LABELS "fast;dxf;adapter;objects;mleader;generated" TIMEOUT 30)
add_test(NAME libdxfrw_mleaderstyle_integer_source_domains
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --mleaderstyle-integer-source-domains)
set_tests_properties(libdxfrw_mleaderstyle_integer_source_domains PROPERTIES
    LABELS "fast;dxf;adapter;objects;mleader;generated" TIMEOUT 30)
add_test(NAME libdxfrw_mleaderstyle_duplicate_singleton_fields
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --mleaderstyle-duplicate-singleton-fields)
set_tests_properties(libdxfrw_mleaderstyle_duplicate_singleton_fields PROPERTIES
    LABELS "fast;dxf;adapter;objects;mleader;generated" TIMEOUT 30)
add_test(NAME libdxfrw_mlinestyle_duplicate_singleton_fields
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --mlinestyle-duplicate-singleton-fields)
set_tests_properties(libdxfrw_mlinestyle_duplicate_singleton_fields PROPERTIES
    LABELS "fast;dxf;adapter;objects;mlinestyle;generated" TIMEOUT 30)
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

# PurgePreventer and GripLocation have admitted owner-graph routes. Visibility
# Grip has a separate record-body profile; incomplete owner graphs stay refused.
add_test(
    NAME dwg2dxf_dynamic_block_purge_preventer
    COMMAND libdxfrw_dwg2dxf_adapter_tests --dynamic-block-purge-preventer
)
set_tests_properties(dwg2dxf_dynamic_block_purge_preventer PROPERTIES
    LABELS "fast;dxf;adapter;dynamic-block"
    TIMEOUT 30
)

add_test(
    NAME dwg2dxf_dynamic_block_grip_location
    COMMAND libdxfrw_dwg2dxf_adapter_tests --dynamic-block-grip-location
)
set_tests_properties(dwg2dxf_dynamic_block_grip_location PROPERTIES
    LABELS "fast;dxf;adapter;dynamic-block"
    TIMEOUT 30
)

add_test(
    NAME dwg2dxf_dynamic_block_visibility_grip
    COMMAND libdxfrw_dwg2dxf_adapter_tests --dynamic-block-visibility-grip
)
set_tests_properties(dwg2dxf_dynamic_block_visibility_grip PROPERTIES
    LABELS "fast;dxf;adapter;dynamic-block"
    TIMEOUT 30
)

add_test(
    NAME dwg2dxf_dynamic_block_oriented_grips
    COMMAND libdxfrw_dwg2dxf_adapter_tests --dynamic-block-oriented-grips
)
set_tests_properties(dwg2dxf_dynamic_block_oriented_grips PROPERTIES
    LABELS "fast;dxf;adapter;dynamic-block"
    TIMEOUT 30
)

add_test(
    NAME dwg2dxf_dynamic_block_visibility_parameter
    COMMAND libdxfrw_dwg2dxf_adapter_tests --dynamic-block-visibility-parameter
)
set_tests_properties(dwg2dxf_dynamic_block_visibility_parameter PROPERTIES
    LABELS "fast;dxf;adapter;dynamic-block"
    TIMEOUT 30
)

add_test(
    NAME dwg2dxf_dynamic_block_visibility_parameter_door_profile
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --dynamic-block-visibility-parameter-door-profile
)
set_tests_properties(dwg2dxf_dynamic_block_visibility_parameter_door_profile PROPERTIES
    LABELS "fast;dxf;adapter;dynamic-block"
    TIMEOUT 30
)

add_test(
    NAME dwg2dxf_block_representation_data_output
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --block-representation-data-output
)
set_tests_properties(dwg2dxf_block_representation_data_output PROPERTIES
    LABELS "fast;dxf;adapter;dynamic-block"
    TIMEOUT 30
)

add_test(
    NAME dwg2dxf_insert_block_record_source_handle_binding
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --insert-block-record-source-handle-binding
)
set_tests_properties(dwg2dxf_insert_block_record_source_handle_binding PROPERTIES
    LABELS "fast;dxf;adapter;ownership"
    TIMEOUT 30
)

add_test(
    NAME dwg2dxf_ownerless_fixed_space_insert_output
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --ownerless-fixed-space-insert-output
)
set_tests_properties(dwg2dxf_ownerless_fixed_space_insert_output PROPERTIES
    LABELS "fast;dwg;adapter;ownership;generated"
    TIMEOUT 30
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
    NAME dwg2dxf_mleader_adapter_writer
    COMMAND libdxfrw_dwg2dxf_adapter_tests --mleader-adapter-writer
)
set_tests_properties(dwg2dxf_mleader_adapter_writer PROPERTIES
    LABELS "fast;dxf;adapter;mleader"
    TIMEOUT 10
)

add_test(
    NAME dwg2dxf_arcalignedtext_d2t_output_guard
    COMMAND libdxfrw_dwg2dxf_adapter_tests --arcalignedtext-d2t-output-guard
)
set_tests_properties(dwg2dxf_arcalignedtext_d2t_output_guard PROPERTIES
    LABELS "fast;dxf;adapter;annotation;generated"
    TIMEOUT 30
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
add_test(NAME libdxfrw_attdef_attribute_xrecord
    COMMAND libdxfrw_attdef_source_tests --attribute-xrecord)
set_tests_properties(libdxfrw_attdef_attribute_xrecord PROPERTIES
    LABELS "fast;dxf;attdef;xrecord;source;generated" TIMEOUT 60)
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
set_tests_properties(libdxfrw_table_legacy_override_177 PROPERTIES
    LABELS "fast;dxf;adapter;table;generated;transaction" TIMEOUT 30)
add_test(NAME libdxfrw_table_row_alignment_override_parse
    COMMAND libdxfrw_table_content_dxf_tests --table-row-alignment-overrides)
set_tests_properties(libdxfrw_table_row_alignment_override_parse PROPERTIES
    LABELS "fast;dxf;table;generated;parser" TIMEOUT 30)
add_test(NAME libdxfrw_table_legacy_text_height
    COMMAND libdxfrw_table_content_dxf_tests --legacy-table-height)
set_tests_properties(libdxfrw_table_legacy_text_height PROPERTIES LABELS "dxf;table")
add_test(NAME libdxfrw_table_legacy_presence_allocation
    COMMAND libdxfrw_table_content_dxf_tests --legacy-table-presence-allocation)
set_tests_properties(libdxfrw_table_legacy_presence_allocation PROPERTIES
    LABELS "fast;dxf;table;transaction;allocation;generated" TIMEOUT 60)
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
add_test(NAME libdxfrw_table_content_grid_write
    COMMAND libdxfrw_table_content_dxf_tests --table-grid-write)
set_tests_properties(libdxfrw_table_content_grid_write PROPERTIES
    LABELS "fast;dxf;adapter;table;generated;transaction" TIMEOUT 30)
add_test(NAME libdxfrw_table_cell_appearance_override_write
    COMMAND libdxfrw_table_content_dxf_tests --table-cell-appearance-write)
set_tests_properties(libdxfrw_table_cell_appearance_override_write PROPERTIES
    LABELS "fast;dxf;adapter;table;generated;transaction" TIMEOUT 30)
add_test(NAME libdxfrw_table_content_cell_border_visibility_edges
    COMMAND libdxfrw_table_content_dxf_tests
        --table-cell-border-visibility-edges)
set_tests_properties(libdxfrw_table_content_cell_border_visibility_edges
    PROPERTIES LABELS "fast;dxf;adapter;table;generated;transaction"
    TIMEOUT 30)
add_test(NAME libdxfrw_table_content_linked_grid_write
    COMMAND libdxfrw_table_content_dxf_tests --table-linked-grid-write)
set_tests_properties(libdxfrw_table_content_linked_grid_write PROPERTIES
    LABELS "fast;dxf;adapter;table;datalink;generated;transaction" TIMEOUT 30)
add_test(NAME libdxfrw_table_content_row_margin_write
    COMMAND libdxfrw_table_content_dxf_tests --table-row-margin-write)
set_tests_properties(libdxfrw_table_content_row_margin_write PROPERTIES
    LABELS "fast;dxf;adapter;table;generated;transaction" TIMEOUT 30)
add_test(NAME libdxfrw_table_content_column_table_margin_write
    COMMAND libdxfrw_table_content_dxf_tests --table-column-table-margin-write)
set_tests_properties(libdxfrw_table_content_column_table_margin_write PROPERTIES
    LABELS "fast;dxf;adapter;table;generated;transaction" TIMEOUT 30)
add_test(NAME libdxfrw_table_content_single_cell_style_margin_write
    COMMAND libdxfrw_table_content_dxf_tests --table-single-cell-style-margin-write)
set_tests_properties(libdxfrw_table_content_single_cell_style_margin_write PROPERTIES
    LABELS "fast;dxf;adapter;table;generated;transaction" TIMEOUT 30)
add_test(NAME libdxfrw_table_content_table_style_format_write
    COMMAND libdxfrw_table_content_dxf_tests --table-style-format-write)
set_tests_properties(libdxfrw_table_content_table_style_format_write PROPERTIES
    LABELS "fast;dxf;adapter;table;generated;transaction;format" TIMEOUT 30)
add_test(NAME libdxfrw_table_content_table_style_reference_write
    COMMAND libdxfrw_table_content_dxf_tests --table-style-reference-write)
set_tests_properties(libdxfrw_table_content_table_style_reference_write PROPERTIES
    LABELS "fast;dxf;adapter;table;tablestyle;transaction" TIMEOUT 30)
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
add_test(NAME libdxfrw_layout_objects_allocation_retry
    COMMAND libdxfrw_dxf_fixture_tests --layout-objects-allocation-retry)
set_tests_properties(libdxfrw_layout_objects_allocation_retry PROPERTIES
    LABELS "fast;dxf;adapter;layout;render-settings;allocation;transaction;generated"
    TIMEOUT 60)
add_test(NAME libdxfrw_layout_source_identity
    COMMAND libdxfrw_wave1_tests --layout-source-identity)
set_tests_properties(libdxfrw_layout_source_identity PROPERTIES
    LABELS "fast;dxf;writer;layout;identity;transaction;generated"
    TIMEOUT 30)
add_test(NAME libdxfrw_cellstylemap_source_identity
    COMMAND libdxfrw_wave1_tests --cellstylemap-source-identity)
set_tests_properties(libdxfrw_cellstylemap_source_identity PROPERTIES
    LABELS "fast;dxf;writer;cellstylemap;identity;transaction;generated"
    TIMEOUT 30)
add_test(NAME libdxfrw_layer_viewport_override_xrecord_transport
    COMMAND libdxfrw_dxf_fixture_tests --layer-viewport-overrides)
set_tests_properties(libdxfrw_layer_viewport_override_xrecord_transport PROPERTIES
    LABELS "fast;dxf;adapter;layer;viewport;xrecord;generated" TIMEOUT 60)
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
    NAME dwg2dxf_opaque_source_capture_and_strict_output_gate
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --opaque-dwg-source-capture-and-output-gate
)
set_tests_properties(dwg2dxf_opaque_source_capture_and_strict_output_gate
    PROPERTIES LABELS "fast;dwg;adapter;completeness" TIMEOUT 10)

add_test(
    NAME dwg2dxf_geodata_dwg_adapter_transport
    COMMAND libdxfrw_dwg2dxf_adapter_tests --geodata-dwg-adapter-transport
)
set_tests_properties(dwg2dxf_geodata_dwg_adapter_transport PROPERTIES
    LABELS "fast;dwg;dxf;geodata;adapter;generated;transaction"
    TIMEOUT 30
)

add_test(
    NAME dwg2dxf_dbcolor_late_callback_binding
    COMMAND libdxfrw_dwg2dxf_adapter_tests --dbcolor-late-callback-binding
)
set_tests_properties(dwg2dxf_dbcolor_late_callback_binding PROPERTIES
    LABELS "fast;dwg;dxf;color;adapter;generated;callback-order"
    TIMEOUT 10
)

# Locally authored AC1027 DBCOLOR proves only self-consistency. It cannot
# discharge the independent producer/profile/readback gate for output support.
add_test(
    NAME dwg2dxf_dbcolor_external_evidence_gate
    COMMAND libdxfrw_dwg2dxf_adapter_tests --dbcolor-external-evidence-gate
)
add_test(NAME dwg2dxf_dbcolor_eed_source_capture
    COMMAND libdxfrw_dwg2dxf_adapter_tests --dbcolor-eed-source-capture)
set_tests_properties(dwg2dxf_dbcolor_eed_source_capture PROPERTIES
    LABELS "fast;dwg;dbcolor;eed;transaction" TIMEOUT 15)
set_tests_properties(dwg2dxf_dbcolor_external_evidence_gate PROPERTIES
    LABELS "fast;dwg;dxf;color;adapter;generated;transaction"
    TIMEOUT 60
)
add_test(
    NAME dwg2dxf_default_plotstyle_layer_graph
    COMMAND libdxfrw_dwg2dxf_adapter_tests --dxf-default-plotstyle-layers
)
set_tests_properties(dwg2dxf_default_plotstyle_layer_graph PROPERTIES
    LABELS "fast;dxf;plotstyle;adapter;generated;graph"
    TIMEOUT 30
)

add_test(
    NAME dwg2dxf_rastervariables_dwg_adapter_transport
    COMMAND libdxfrw_dwg2dxf_adapter_tests --rastervariables-dwg-adapter-transport
)
set_tests_properties(dwg2dxf_rastervariables_dwg_adapter_transport PROPERTIES
    LABELS "fast;dwg;dxf;raster;adapter;generated;transaction"
    TIMEOUT 30
)
add_executable(libdxfrw_rastervariables_objects_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/rastervariables_objects_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source}
)
target_compile_features(libdxfrw_rastervariables_objects_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_rastervariables_objects_allocation_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_rastervariables_objects_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(libdxfrw_rastervariables_objects_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_rastervariables_objects_allocation_retry
    COMMAND libdxfrw_rastervariables_objects_allocation_tests
        "${CMAKE_CURRENT_BINARY_DIR}/rastervariables-objects-allocation-scratch")
set_tests_properties(libdxfrw_rastervariables_objects_allocation_retry PROPERTIES
    LABELS "fast;dwg;dxf;raster;allocation;transaction;generated"
    TIMEOUT 60)

add_executable(libdxfrw_wipeoutvariables_objects_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/wipeoutvariables_objects_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source}
)
target_compile_features(libdxfrw_wipeoutvariables_objects_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_wipeoutvariables_objects_allocation_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_wipeoutvariables_objects_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(libdxfrw_wipeoutvariables_objects_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_wipeoutvariables_objects_allocation_retry
    COMMAND libdxfrw_wipeoutvariables_objects_allocation_tests
        "${CMAKE_CURRENT_BINARY_DIR}/wipeoutvariables-objects-allocation-scratch")
set_tests_properties(libdxfrw_wipeoutvariables_objects_allocation_retry PROPERTIES
    LABELS "fast;dwg;dxf;wipeout;allocation;transaction;generated"
    TIMEOUT 60)
if(LIBDXFRW_BUILD_DWG2DXF)
    add_test(NAME dwg2dxf_wipeoutvariables_incomplete_graph_refusal
        COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B
            ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf_wipeoutvariables_cli_contract.py
            --converter $<TARGET_FILE:dwg2dxf>
            --generator $<TARGET_FILE:libdxfrw_wipeoutvariables_objects_allocation_tests>)
    set_tests_properties(dwg2dxf_wipeoutvariables_incomplete_graph_refusal PROPERTIES
        LABELS "fast;cli;dwg;dxf;wipeout;transaction;generated"
        TIMEOUT 45)
endif()

# The direct NOD -> ACAD_IMAGE_VARS -> RASTERVARIABLES profile is authored
# locally from the observed physical AC1027 ownership shape. It proves only
# adapter transport and typed DXF readback, not producer interoperability.
add_test(
    NAME dwg2dxf_rastervariables_direct_nod_adapter_transport
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --rastervariables-dwg-adapter-transport-direct-nod
)
set_tests_properties(dwg2dxf_rastervariables_direct_nod_adapter_transport
    PROPERTIES
    LABELS "fast;dwg;dxf;raster;adapter;generated;transaction"
    TIMEOUT 30
)

# AC1018 exact direct-NOD 0/0/1/3 profile; physical whole-file output remains
# separately unqualified.
add_test(NAME dwg2dxf_rastervariables_ac1018_direct_nod_adapter_transport
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --rastervariables-dwg-adapter-transport-ac1018-direct-nod)
set_tests_properties(dwg2dxf_rastervariables_ac1018_direct_nod_adapter_transport
    PROPERTIES LABELS "fast;dwg;dxf;raster;adapter;generated;transaction;AC1018" TIMEOUT 30)

# Exercise the same bounded direct-owner path against a locally authored
# AC1021 source and same-revision output; the physical AC1021 witness is a
# separate manual selector and is not committed as a fixture.
add_test(
    NAME dwg2dxf_rastervariables_ac1021_direct_nod_adapter_transport
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --rastervariables-dwg-adapter-transport-ac1021-direct-nod
)
set_tests_properties(dwg2dxf_rastervariables_ac1021_direct_nod_adapter_transport
    PROPERTIES
    LABELS "fast;dwg;dxf;raster;adapter;generated;transaction;AC1021"
    TIMEOUT 30
)

# AC1024 keeps the exact independently observed direct-NOD scalar/common-link
# profile; this generated control is not a whole-source qualification.
add_test(
    NAME dwg2dxf_rastervariables_ac1024_direct_nod_adapter_transport
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --rastervariables-dwg-adapter-transport-ac1024-direct-nod
)
set_tests_properties(dwg2dxf_rastervariables_ac1024_direct_nod_adapter_transport
    PROPERTIES
    LABELS "fast;dwg;dxf;raster;adapter;generated;transaction;AC1024"
    TIMEOUT 30
)

add_test(NAME dwg2dxf_rastervariables_ac1018_frame1_adapter_transport
    COMMAND libdxfrw_dwg2dxf_adapter_tests --rastervariables-dwg-adapter-transport-ac1018-frame1)
set_tests_properties(dwg2dxf_rastervariables_ac1018_frame1_adapter_transport PROPERTIES
    LABELS "dwg;dxf;adapter;fast;AC1018;transaction" TIMEOUT 30)

add_test(NAME dwg2dxf_rastervariables_ac1032_frame1_adapter_transport
    COMMAND libdxfrw_dwg2dxf_adapter_tests --rastervariables-dwg-adapter-transport-ac1032-frame1)
set_tests_properties(dwg2dxf_rastervariables_ac1032_frame1_adapter_transport PROPERTIES
    LABELS "dwg;dxf;adapter;fast;AC1032;transaction" TIMEOUT 30)

add_test(NAME dwg2dxf_rastervariables_ac1024_frame1_adapter_transport
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --rastervariables-dwg-adapter-transport-ac1024-frame1)
set_tests_properties(dwg2dxf_rastervariables_ac1024_frame1_adapter_transport
    PROPERTIES LABELS "fast;dwg;dxf;raster;adapter;generated;transaction;AC1024"
    TIMEOUT 30)

add_test(
    NAME dwg2dxf_rastervariables_ac1032_direct_nod_adapter_transport
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --rastervariables-dwg-adapter-transport-ac1032-direct-nod
)
set_tests_properties(dwg2dxf_rastervariables_ac1032_direct_nod_adapter_transport
    PROPERTIES
    LABELS "fast;dwg;dxf;raster;adapter;generated;transaction;AC1032"
    TIMEOUT 30
)

add_test(
    NAME dwg2dxf_wipeoutvariables_dwg_adapter_transport
    COMMAND libdxfrw_dwg2dxf_adapter_tests --wipeoutvariables-dwg-adapter-transport
)
set_tests_properties(dwg2dxf_wipeoutvariables_dwg_adapter_transport PROPERTIES
    LABELS "fast;dwg;dxf;wipeout;adapter;generated;transaction"
    TIMEOUT 30
)

add_test(
    NAME dwg2dxf_wipeoutvariables_empty_class_gate
    COMMAND libdxfrw_dwg2dxf_adapter_tests --wipeoutvariables-empty-class-gate
)
set_tests_properties(dwg2dxf_wipeoutvariables_empty_class_gate PROPERTIES
    LABELS "fast;dwg;adapter;wipeout;generated"
    TIMEOUT 30
)

add_test(
    NAME dwg2dxf_wipeoutvariables_ac1027_direct_nod
    COMMAND libdxfrw_dwg2dxf_adapter_tests --wipeoutvariables-dwg-direct-nod
)
set_tests_properties(dwg2dxf_wipeoutvariables_ac1027_direct_nod PROPERTIES
    LABELS "fast;dwg;dxf;wipeout;adapter;generated;transaction;AC1027"
    TIMEOUT 30
)

add_test(NAME dwg2dxf_wipeoutvariables_ac1024_direct_nod
    COMMAND libdxfrw_dwg2dxf_adapter_tests --wipeoutvariables-dwg-ac1024-direct-nod)
add_test(NAME dwg2dxf_wipeoutvariables_ac1021_direct_nod
    COMMAND libdxfrw_dwg2dxf_adapter_tests --wipeoutvariables-dwg-ac1021-direct-nod)
set_tests_properties(dwg2dxf_wipeoutvariables_ac1021_direct_nod PROPERTIES
    LABELS "fast;dwg;dxf;wipeout;adapter;generated;transaction;AC1021"
    TIMEOUT 30)
add_test(NAME dwg2dxf_wipeoutvariables_ac1024_frame1
    COMMAND libdxfrw_dwg2dxf_adapter_tests --wipeoutvariables-dwg-ac1024-frame1)
add_test(NAME dwg2dxf_wipeoutvariables_ac1027_frame1
    COMMAND libdxfrw_dwg2dxf_adapter_tests --wipeoutvariables-dwg-ac1027-frame1)
add_test(NAME dwg2dxf_wipeoutvariables_ac1032_frame1
    COMMAND libdxfrw_dwg2dxf_adapter_tests --wipeoutvariables-dwg-ac1032-frame1)
set_tests_properties(dwg2dxf_wipeoutvariables_ac1027_frame1
    dwg2dxf_wipeoutvariables_ac1032_frame1 PROPERTIES
    LABELS "fast;dwg;dxf;wipeout;adapter;generated;transaction;AC1027;AC1032" TIMEOUT 30)
set_tests_properties(dwg2dxf_wipeoutvariables_ac1024_frame1 PROPERTIES
    LABELS "fast;dwg;dxf;wipeout;adapter;generated;transaction;AC1024" TIMEOUT 30)
add_test(NAME dwg2dxf_wipeoutvariables_ac1018_direct_nod
    COMMAND libdxfrw_dwg2dxf_adapter_tests --wipeoutvariables-dwg-ac1018-direct-nod)
add_test(NAME dwg2dxf_wipeoutvariables_ac1018_frame1
    COMMAND libdxfrw_dwg2dxf_adapter_tests --wipeoutvariables-dwg-ac1018-frame1)
set_tests_properties(dwg2dxf_wipeoutvariables_ac1018_frame1 PROPERTIES
    LABELS "fast;dwg;dxf;wipeout;adapter;generated;transaction;AC1018" TIMEOUT 30)
set_tests_properties(dwg2dxf_wipeoutvariables_ac1018_direct_nod PROPERTIES
    LABELS "fast;dwg;dxf;graph;adapter;wipeout;transaction" TIMEOUT 30)
set_tests_properties(dwg2dxf_wipeoutvariables_ac1024_direct_nod PROPERTIES
    LABELS "fast;dwg;dxf;wipeout;adapter;generated;transaction;AC1024" TIMEOUT 30)
add_test(NAME dwg2dxf_wipeoutvariables_ac1024_nested_refusal
    COMMAND libdxfrw_dwg2dxf_adapter_tests --wipeoutvariables-dwg-ac1024-nested-refusal)
set_tests_properties(dwg2dxf_wipeoutvariables_ac1024_nested_refusal PROPERTIES
    LABELS "fast;dwg;dxf;wipeout;adapter;generated;transaction;AC1024" TIMEOUT 30)

add_test(NAME dwg2dxf_wipeoutvariables_ac1032_direct_nod
    COMMAND libdxfrw_dwg2dxf_adapter_tests --wipeoutvariables-dwg-ac1032-direct-nod)
set_tests_properties(dwg2dxf_wipeoutvariables_ac1032_direct_nod PROPERTIES
    LABELS "fast;dwg;dxf;wipeout;adapter;generated;transaction;AC1032" TIMEOUT 30)
add_test(NAME dwg2dxf_wipeoutvariables_ac1032_nested_refusal
    COMMAND libdxfrw_dwg2dxf_adapter_tests --wipeoutvariables-dwg-ac1032-nested-refusal)
set_tests_properties(dwg2dxf_wipeoutvariables_ac1032_nested_refusal PROPERTIES
    LABELS "fast;dwg;dxf;wipeout;adapter;generated;transaction;AC1032" TIMEOUT 30)

add_test(
    NAME dwg2dxf_standard_dwg_object_source_capture
    COMMAND libdxfrw_dwg2dxf_adapter_tests --standard-dwg-object-source-capture
)
set_tests_properties(dwg2dxf_standard_dwg_object_source_capture PROPERTIES
    LABELS "fast;dwg;adapter;objects"
    TIMEOUT 10
)
add_test(
    NAME dwg2dxf_dwg_block_record_control_membership
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --dwg-block-record-control-membership
)
set_tests_properties(dwg2dxf_dwg_block_record_control_membership PROPERTIES
    LABELS "fast;dwg;adapter;tables;ownership;generated"
    TIMEOUT 15
)
add_test(
    NAME dwg2dxf_dwg_ltype_table_control_membership
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --dwg-ltype-table-control-membership
)
set_tests_properties(dwg2dxf_dwg_ltype_table_control_membership PROPERTIES
    LABELS "fast;dwg;adapter;tables;ownership;generated"
    TIMEOUT 15
)
add_test(
    NAME dwg2dxf_dwg_layer_table_control_dictionary
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --dwg-layer-table-control-dictionary
)
set_tests_properties(dwg2dxf_dwg_layer_table_control_dictionary PROPERTIES
    LABELS "fast;dwg;adapter;tables;ownership;generated"
    TIMEOUT 15
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
add_test(NAME dwg2dxf_material_ac1024_typed_graph_output
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --material-ac1024-typed-graph-output)
set_tests_properties(dwg2dxf_material_ac1024_typed_graph_output PROPERTIES
    LABELS "fast;dwg;dxf;adapter;objects;graph;material;generated;transaction"
    TIMEOUT 30)
add_test(NAME dwg2dxf_generated_image_dictionary_source_identity
    COMMAND libdxfrw_dwg2dxf_adapter_tests --generated-image-dictionary-source-identity)
set_tests_properties(dwg2dxf_generated_image_dictionary_source_identity PROPERTIES
    # Includes108 wire-fault refusal/retries plus native readback; allow ASan
    # headroom under concurrent CTest load without removing any controls.
    LABELS "fast;dwg;dxf;image;adapter;objects;generated" TIMEOUT 30)
add_test(NAME dwg2dxf_imagedef_common_hard_owner_import
    COMMAND libdxfrw_dwg2dxf_adapter_tests --imagedef-common-hard-owner-import)
set_tests_properties(dwg2dxf_imagedef_common_hard_owner_import PROPERTIES
    LABELS "fast;dwg;image;adapter;objects;generated" TIMEOUT 30)
add_test(NAME dwg2dxf_image_common_handle_role_controls
    COMMAND libdxfrw_dwg2dxf_adapter_tests --image-common-handle-role-controls)
set_tests_properties(dwg2dxf_image_common_handle_role_controls PROPERTIES
    LABELS "fast;dwg;image;objects;generated" TIMEOUT 10)
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
add_test(NAME dwg2dxf_xrecord_dwg_compatibility_codes
    COMMAND libdxfrw_dwg2dxf_adapter_tests --xrecord-dwg-compatibility-codes)
set_tests_properties(dwg2dxf_xrecord_dwg_compatibility_codes PROPERTIES
    LABELS "fast;dxf;objects;xrecord;compatibility;generated" TIMEOUT 30)
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
add_test(NAME dwg2dxf_underlay_definition_graph
    COMMAND libdxfrw_dwg2dxf_adapter_tests --underlay-definition-graph)
set_tests_properties(dwg2dxf_underlay_definition_graph PROPERTIES
    LABELS "fast;dxf;underlay;dictionary;graph;adapter;generated;transaction"
    TIMEOUT 30)
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
    NAME dwg2dxf_raw_dxf_entity_table_name_references
    COMMAND libdxfrw_dwg2dxf_adapter_tests --raw-dxf-entity-table-name-references
)
set_tests_properties(dwg2dxf_raw_dxf_entity_table_name_references PROPERTIES
    LABELS "fast;dxf;adapter;raw-entity;table-reference;transaction;generated"
    TIMEOUT 30
)

add_test(
    NAME dxf_raw_entity_material_visualstyle_bindings
    COMMAND libdxfrw_dxf_fixture_tests --material-consumer-bindings
)
set_tests_properties(dxf_raw_entity_material_visualstyle_bindings PROPERTIES
    LABELS "fast;dxf;adapter;raw-entity;material;visual-style;reference;transaction;generated"
    TIMEOUT 30
)

add_test(
    NAME dxf_header_material_bindings
    COMMAND libdxfrw_dxf_fixture_tests --header-material-bindings
)
set_tests_properties(dxf_header_material_bindings PROPERTIES
    LABELS "fast;dxf;adapter;header;material;reference;transaction;generated"
    TIMEOUT 30
)

add_test(
    NAME dxf_layer_material_bindings
    COMMAND libdxfrw_dxf_fixture_tests --layer-material-bindings
)
set_tests_properties(dxf_layer_material_bindings PROPERTIES
    LABELS "fast;dxf;adapter;layer;material;reference;transaction;generated"
    TIMEOUT 30
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
add_test(NAME libdxfrw_view_live_section_reference_closure
    COMMAND libdxfrw_dxf_fixture_tests --view-live-section-reference-closure)
set_tests_properties(libdxfrw_view_live_section_reference_closure PROPERTIES
    LABELS "fast;dxf;view;section;graph;transaction" TIMEOUT 30)
add_test(NAME libdxfrw_section_objects_allocation_retry
    COMMAND libdxfrw_dxf_fixture_tests --section-objects-allocation-retry)
set_tests_properties(libdxfrw_section_objects_allocation_retry PROPERTIES
    LABELS "fast;dxf;section;graph;allocation;transaction" TIMEOUT 120)
add_test(NAME libdxfrw_xrecord_objects_allocation_retry
    COMMAND libdxfrw_dxf_fixture_tests --xrecord-objects-allocation-retry)
set_tests_properties(libdxfrw_xrecord_objects_allocation_retry PROPERTIES
    LABELS "fast;dxf;xrecord;graph;allocation;transaction" TIMEOUT 120)
add_test(NAME libdxfrw_dictionaryvar_objects_allocation_retry
    COMMAND libdxfrw_dxf_fixture_tests --dictionaryvar-objects-allocation-retry)
set_tests_properties(libdxfrw_dictionaryvar_objects_allocation_retry PROPERTIES
    LABELS "fast;dxf;dictionaryvar;graph;allocation;transaction" TIMEOUT 120)
add_test(NAME libdxfrw_dictionaryvar_ac1024_mixed_graph
    COMMAND libdxfrw_dwg2dxf_adapter_tests --dictionaryvar-ac1024-mixed-generated
        ${CMAKE_CURRENT_BINARY_DIR}/dictionaryvar-mixed-generated)
set_tests_properties(libdxfrw_dictionaryvar_ac1024_mixed_graph PROPERTIES
    LABELS "fast;dxf;dictionaryvar;graph;generated;transaction" TIMEOUT 30)
add_test(NAME libdxfrw_pointcloud_objects_allocation_retry
    COMMAND libdxfrw_dxf_fixture_tests --pointcloud-objects-allocation-retry)
set_tests_properties(libdxfrw_pointcloud_objects_allocation_retry PROPERTIES
    LABELS "fast;dxf;pointcloud;graph;allocation;transaction" TIMEOUT 120)
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
    NAME dwg2dxf_dynamic_block_visibility_composed_graph
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --dynamic-block-visibility-composed-graph
)
set_tests_properties(dwg2dxf_dynamic_block_visibility_composed_graph PROPERTIES
    LABELS "fast;dxf;adapter;dynamic-block"
    TIMEOUT 30
)

# Exercise the exact AC1024 parameter-dependency body DXF fields, typed/raw
# readback, refusal, and transactional retry using runtime-authored records.
add_test(
    NAME dwg2dxf_dynamic_block_parameter_dependency_body_writer
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --dynamic-block-parameter-dependency-body-dxf-writer
)
set_tests_properties(dwg2dxf_dynamic_block_parameter_dependency_body_writer
    PROPERTIES
    LABELS "fast;dxf;adapter;dynamic-block"
    TIMEOUT 30
)

# Exercise the exact AC1024 base-point parameter fields in both output
# encodings, including invalid-coordinate rollback and a same-writer retry.
add_test(
    NAME dwg2dxf_dynamic_block_basepoint_parameter_writer
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --dynamic-block-basepoint-parameter-dxf-writer
)
set_tests_properties(dwg2dxf_dynamic_block_basepoint_parameter_writer
    PROPERTIES
    LABELS "fast;dxf;adapter;dynamic-block"
    TIMEOUT 30
)

# Exercise the AC1024 two-point alignment profile, including all four
# connection code/name lists, raw-only unknown tails, rollback, and retry.
add_test(
    NAME dwg2dxf_dynamic_block_alignment_parameter_writer
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --dynamic-block-alignment-parameter-dxf-writer
)
set_tests_properties(dwg2dxf_dynamic_block_alignment_parameter_writer
    PROPERTIES
    LABELS "fast;dxf;adapter;dynamic-block"
    TIMEOUT 30
)

# Exercise AC1024 BLOCKFLIPACTION action indexes, dependency handles, and
# connection-point fields in both DXF output encodings.
add_test(
    NAME dwg2dxf_dynamic_block_flip_action_writer
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --dynamic-block-flip-action-dxf-writer
)
set_tests_properties(dwg2dxf_dynamic_block_flip_action_writer
    PROPERTIES
    LABELS "fast;dxf;adapter;dynamic-block"
    TIMEOUT 30
)

# Exercise AC1024 BLOCKMOVEACTION, BLOCKSCALEACTION and BLOCKSTRETCHACTION
# fields in both output encodings, including nested stretch references,
# unknown-tail raw-only fallback, transactional rollback and retry.
add_test(
    NAME dwg2dxf_dynamic_block_actions_writer
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --dynamic-block-actions-dxf-writer
)
set_tests_properties(dwg2dxf_dynamic_block_actions_writer PROPERTIES
    LABELS "fast;dxf;adapter;dynamic-block"
    TIMEOUT 30
)

# Exercise the source-qualified AC1024 BLOCKLOOKUPACTION grid and per-column
# metadata in both encodings, including output rollback and retry.
add_test(
    NAME dwg2dxf_dynamic_block_lookup_action_writer
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --dynamic-block-lookup-action-dxf-writer
)
set_tests_properties(dwg2dxf_dynamic_block_lookup_action_writer PROPERTIES
    LABELS "fast;dxf;adapter;dynamic-block"
    TIMEOUT 30
)

# The physical sample is supplied locally and intentionally is not vendored.
set(_lookup_action_ac1024_sample
    "${CMAKE_CURRENT_SOURCE_DIR}/samples/AC1024/visualization_-_sun_and_sky_demo.dwg")
if(EXISTS "${_lookup_action_ac1024_sample}")
    add_test(
        NAME dwg2dxf_dynamic_block_lookup_action_physical_writer
        COMMAND libdxfrw_dwg2dxf_adapter_tests
            --dynamic-block-lookup-action-dxf-writer
            "${_lookup_action_ac1024_sample}"
    )
    set_tests_properties(dwg2dxf_dynamic_block_lookup_action_physical_writer
        PROPERTIES
        LABELS "fast;dxf;adapter;dynamic-block;sample;AC1024"
        TIMEOUT 60
    )
endif()

# Exercise the four AC1024 AcDbBlock2PtParameter subclasses with exact
# ASCII/Binary field order, typed readback, and atomic rollback/retry.
add_test(
    NAME dwg2dxf_dynamic_block_two_point_parameter_writer
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --dynamic-block-two-point-parameter-dxf-writer
)
set_tests_properties(dwg2dxf_dynamic_block_two_point_parameter_writer
    PROPERTIES
    LABELS "fast;dxf;adapter;dynamic-block"
    TIMEOUT 30
)

add_test(
    NAME dwg2dxf_proxy_object_capture_limits
    COMMAND libdxfrw_dwg2dxf_adapter_tests --proxy-object-capture-limits
)
set_tests_properties(dwg2dxf_proxy_object_capture_limits PROPERTIES
    LABELS "fast;dxf;adapter;proxy"
    TIMEOUT 10
)

# Exercise the integrated CLASS table and opaque ACAD_PROXY_OBJECT writer,
# then verify typed and raw readback in both DXF encodings.
add_test(
    NAME dwg2dxf_acds_opaque_proxy_codec_readback
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --acds-opaque-proxy-codec-readback
)
set_tests_properties(dwg2dxf_acds_opaque_proxy_codec_readback PROPERTIES
    LABELS "fast;dxf;adapter;proxy;acds"
    TIMEOUT 30
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

# Associative source handles share the document identity namespace with all
# other captured object families; duplicate or cross-family aliases fail graph
# preflight before any writer state is published.
add_test(
    NAME dwg2dxf_associative_source_handle_inventory
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --associative-source-handle-inventory
)
set_tests_properties(dwg2dxf_associative_source_handle_inventory PROPERTIES
    LABELS "fast;dxf;adapter;objects;associative;identity"
    TIMEOUT 15
)

add_test(
    NAME dwg2dxf_associative_source_handle_reservation
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --associative-source-handle-reservation
)
set_tests_properties(dwg2dxf_associative_source_handle_reservation PROPERTIES
    LABELS "fast;dxf;adapter;objects;associative;identity;writer"
    TIMEOUT 15
)

# Exercise the admitted AC1024 DWG-root network + fully consumed variable and
# value-dependency graph through strict document output, remapped identities,
# exact wire inspection, readback, atomic refusal, and corrected retry.
add_test(
    NAME dwg2dxf_associative_dwg_root_network_value_dependency_export
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --associative-dwg-root-network-value-dependency-export
)
set_tests_properties(dwg2dxf_associative_dwg_root_network_value_dependency_export PROPERTIES
    LABELS "fast;dxf;adapter;objects;associative;strict;writer"
    TIMEOUT 30
)

# Exercise the AC1024 network -> inline constraint group -> SingleEdge geometry
# dependency route through strict output, readback, atomic refusal, and retry.
add_test(
    NAME dwg2dxf_associative_geometry_dependency_graph_export
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --associative-geometry-dependency-graph-export
)
set_tests_properties(dwg2dxf_associative_geometry_dependency_graph_export PROPERTIES
    LABELS "fast;dxf;adapter;objects;associative;strict;writer"
    TIMEOUT 30
)

# Exercise a source-bound non-owned root-network edge to a nested network
# stored under a SORTENTSTABLE extension dictionary, including output readback,
# atomic refusal, and corrected retry for both DXF encodings.
add_test(
    NAME dwg2dxf_associative_dwg_nested_network_sortents_output
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --associative-dwg-nested-network-sortents-output
)
set_tests_properties(dwg2dxf_associative_dwg_nested_network_sortents_output PROPERTIES
    LABELS "fast;dxf;adapter;objects;associative;strict;writer;sortents"
    TIMEOUT 30
)

# Exercise the selected-network projection with a locally authored AC1024
# source graph, including exclusion through a SORTENTS-owning dictionary.
add_test(
    NAME dwg2dxf_generated_selected_associative_networks_output
    COMMAND libdxfrw_dwg2dxf_adapter_tests
        --generated-selected-associative-networks-output
)
set_tests_properties(dwg2dxf_generated_selected_associative_networks_output PROPERTIES
    LABELS "fast;dxf;adapter;objects;associative;strict;writer;sortents"
    TIMEOUT 30
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

add_test(NAME dwg_ellipse_wcs_extrusion
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip> --dwg-ellipse-wcs-extrusion)
set_tests_properties(dwg_ellipse_wcs_extrusion PROPERTIES
    LABELS "fast;dwg;ellipse;extrusion" TIMEOUT 30)

add_test(
    NAME dwg_r2007_polyline3d_closed_flag_roundtrip
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
            --r2007-polyline3d-footer
)
set_tests_properties(dwg_r2007_polyline3d_closed_flag_roundtrip PROPERTIES
    LABELS "fast;dwg;polyline;AC1021;wire-readback"
    TIMEOUT 30
)

add_test(
    NAME dwg_r2007_class_string_footer
    COMMAND $<TARGET_FILE:libdxfrw_dwg_reader_matrix_tests>
            --r2007-class-string-footer
)
set_tests_properties(dwg_r2007_class_string_footer PROPERTIES
    LABELS "fast;dwg;classes;AC1024;generated"
    TIMEOUT 30
)

add_test(
    NAME dxf_mpolygon_metadata_boundary_roundtrip
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip> --mpolygon-metadata
)
set_tests_properties(dxf_mpolygon_metadata_boundary_roundtrip PROPERTIES
    LABELS "fast;dxf;adapter;mpolygon"
    TIMEOUT 30
)

add_test(
    NAME dxf_viewport_undocumented_group46_isolation
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
            --viewport-group46-isolation
)
set_tests_properties(dxf_viewport_undocumented_group46_isolation PROPERTIES
    LABELS "fast;dxf;viewport;field-mapping"
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

# The selected primitive writer supports the exact one-node ACAD_EVALUATION_GRAPH
# closure for a material-free TORUS, in addition to the existing BOX profile.
add_test(
    NAME dxf_acsh_torus_typed_roundtrip
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
            --acds-typed-torus
)
set_tests_properties(dxf_acsh_torus_typed_roundtrip PROPERTIES
    LABELS "fast;dxf;dwg;adapter;3d;acds;generated"
    TIMEOUT 30
)

# Decode the empirically observed ACIS 21200 SAB token forms and serialize a
# bounded SAT-v1 text stream, including the zero-count/explicit-terminator
# profile. The control is locally authored in memory and adds no fixture.
add_test(
    NAME acis_sab21200_sat_v1_token_controls
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
            --sab21200-sat-v1-controls
)
set_tests_properties(acis_sab21200_sat_v1_token_controls PROPERTIES
    LABELS "fast;dwg;dxf;3d;acis;generated"
    TIMEOUT 30
)

# Keep the bounded DWG SAT-v1 extraction controls independently runnable for
# the AC1015 and AC1018 modeler-version-1 profiles.
add_test(
    NAME dwg_sat_v1_bounded_extraction
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
            --modeler-sat-v1-extraction
)
set_tests_properties(dwg_sat_v1_bounded_extraction PROPERTIES
    LABELS "fast;dwg;3d;acis;generated"
    TIMEOUT 30
)

# The SAT-v1 body is an optional decoded view of an otherwise-valid DWG
# modeler frame. Verify that failure to allocate that view does not fail the
# enclosing record parse, and that a retry repopulates the exact carrier.
add_executable(libdxfrw_modeler_sat_v1_allocation_tests
    modeler_sat_v1_allocation_tests.cpp
)
target_link_libraries(libdxfrw_modeler_sat_v1_allocation_tests PRIVATE dxfrw)
add_test(
    NAME dwg_sat_v1_optional_extraction_allocation_retry
    COMMAND $<TARGET_FILE:libdxfrw_modeler_sat_v1_allocation_tests>
)
set_tests_properties(dwg_sat_v1_optional_extraction_allocation_retry PROPERTIES
    LABELS "fast;dwg;3d;acis;generated;allocation"
    TIMEOUT 30
)

# Exercise the source-bound AC1024 ACIS 700 to SAT 700 route without adding an
# external DWG/DXF/SAT fixture to the repository.
add_test(
    NAME dwg_ac1024_acis700_sat_dxf_roundtrip
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
            --modeler-ac1024-acis700-sat
)
set_tests_properties(dwg_ac1024_acis700_sat_dxf_roundtrip PROPERTIES
    LABELS "fast;dwg;dxf;3d;acis;generated"
    TIMEOUT 30
)

add_executable(libdxfrw_acis_sat_allocation_tests
    acis_sat_allocation_tests.cpp)
target_compile_features(libdxfrw_acis_sat_allocation_tests PRIVATE cxx_std_17)
target_include_directories(libdxfrw_acis_sat_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_acis_sat_allocation_tests PRIVATE dxfrw)
add_test(NAME acis_sat_allocation_rollback_retry
    COMMAND libdxfrw_acis_sat_allocation_tests)
set_tests_properties(acis_sat_allocation_rollback_retry PROPERTIES
    LABELS "fast;dwg;dxf;3d;acis;allocation;transaction;generated"
    TIMEOUT 30)

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

# Verify BLOCK_HEADER INSERT reverse membership, refusal on mismatched source
# handles, and same-facade retry across generated writer version families.
add_test(
    NAME dwg_block_record_xref_insert_membership
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
            --block-record-xref-roundtrip
)
set_tests_properties(dwg_block_record_xref_insert_membership PROPERTIES
    LABELS "fast;dwg;writer;xref;transaction;generated"
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
    NAME dwg_hatch_circle_boundary_output
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
            --hatch-circle-boundary-reference
)
set_tests_properties(dwg_hatch_circle_boundary_output PROPERTIES
    LABELS "fast;dwg;dxf;adapter;hatch;circle;generated;transaction"
    TIMEOUT 30
)

add_test(
    NAME dwg_spline_bitlong_boundary
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
            --spline-bitlong-boundary
)
set_tests_properties(dwg_spline_bitlong_boundary PROPERTIES
    LABELS "fast;dwg;reader;spline"
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
    NAME libdxfrw_dwg_3dface_flag_domain
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
            --dwg-3dface-flag-domain
)
set_tests_properties(libdxfrw_dwg_3dface_flag_domain PROPERTIES
    LABELS "fast;dwg;writer;reader;3dface;rollback"
    TIMEOUT 30
)

# Invalid CIRCLE/ARC radii must not replace an existing DWG; correcting the
# same writer must succeed and read back in every supported output version.
add_test(
    NAME libdxfrw_dwg_circle_arc_radius_domain
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
            --dwg-circle-arc-radius-domain
)
set_tests_properties(libdxfrw_dwg_circle_arc_radius_domain PROPERTIES
    LABELS "fast;dwg;writer;reader;circle;arc;rollback;generated"
    TIMEOUT 30
)

# Keep DWG ELLIPSE output within the AutoCAD ratio and axis-geometry domain;
# each refused profile must preserve output and allow corrected same-writer retry.
add_test(
    NAME libdxfrw_dwg_ellipse_geometry_admission
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
            --dwg-ellipse-geometry-admission
)
set_tests_properties(libdxfrw_dwg_ellipse_geometry_admission PROPERTIES
    LABELS "fast;dwg;writer;reader;ellipse;rollback;generated"
    TIMEOUT 30
)

# ARC angles are encoded as BD values; reject non-finite values before DWG
# output publication and verify correction/retry for each writer version.
add_test(
    NAME libdxfrw_dwg_arc_angle_domain
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
            --dwg-arc-angle-domain
)
set_tests_properties(libdxfrw_dwg_arc_angle_domain PROPERTIES
    LABELS "fast;dwg;writer;reader;arc;rollback;generated"
    TIMEOUT 30
)

# CIRCLE/ARC centers, thickness and extrusion directions share a finite,
# nondegenerate geometry contract across DWG writer versions.
add_test(
    NAME libdxfrw_dwg_circle_arc_geometry_domain
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
            --dwg-circle-arc-geometry-domain
)
set_tests_properties(libdxfrw_dwg_circle_arc_geometry_domain PROPERTIES
    LABELS "fast;dwg;writer;reader;circle;arc;rollback;generated"
    TIMEOUT 30
)

# POINT and LINE writers must reject non-finite modeled fields that their DWG
# readers already refuse, preserving the destination for same-writer retry.
add_test(
    NAME libdxfrw_dwg_point_line_finite_admission
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
            --dwg-point-line-finite-admission
)
set_tests_properties(libdxfrw_dwg_point_line_finite_admission PROPERTIES
    LABELS "fast;dwg;writer;reader;point;line;rollback;generated"
    TIMEOUT 30
)

# RAY and XLINE share bounded point/vector fields and must not emit non-finite
# coordinates that their readers cannot publish.
add_test(
    NAME libdxfrw_dwg_ray_xline_finite_admission
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
            --dwg-ray-xline-finite-admission
)
set_tests_properties(libdxfrw_dwg_ray_xline_finite_admission PROPERTIES
    LABELS "fast;dwg;writer;reader;ray;xline;rollback;generated"
    TIMEOUT 30
)

# 3DLINE uses a separate custom-class body; ensure all raw-double fields are
# finite before output and that corrected same-writer output can be read back.
add_test(
    NAME libdxfrw_dwg_3dline_finite_admission
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
            --dwg-3dline-finite-admission
)
set_tests_properties(libdxfrw_dwg_3dline_finite_admission PROPERTIES
    LABELS "fast;dwg;writer;reader;3dline;rollback;generated"
    TIMEOUT 30
)

# TRACE/SOLID require finite 2.5D fields. 3DFACE must preserve only its four
# WCS corners and edge flags, refusing inherited common fields absent on wire.
add_test(
    NAME libdxfrw_dwg_trace_solid_3dface_geometry
    COMMAND $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>
            --dwg-trace-solid-3dface-geometry
)
set_tests_properties(libdxfrw_dwg_trace_solid_3dface_geometry PROPERTIES
    LABELS "fast;dwg;writer;reader;trace;solid;3dface;rollback;generated"
    TIMEOUT 30
)

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
    # FreeCAD installation. Generate the positive drawing locally so a broken
    # or incomplete third-party fixture cannot invalidate the CLI contract.
    set(_freecad_cli_source_generator $<TARGET_FILE:libdxfrw_dwg_local_roundtrip>)
    set(_freecad_cli_rtext_sample
        ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/rtext_arctext.dwg
    )
    set(_freecad_cli_mpolygon_sample
        ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/mpolygon_solid.dwg
    )
    if(EXISTS "${_freecad_cli_rtext_sample}"
            AND EXISTS "${_freecad_cli_mpolygon_sample}")
        add_test(
            NAME dwg2dxf_freecad_cli_compat
            COMMAND ${CMAKE_COMMAND}
                -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
                "-DDWG_SOURCE_GENERATOR=${_freecad_cli_source_generator}"
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
                    "-DDWG_SOURCE_GENERATOR=${_freecad_cli_source_generator}"
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

add_test(
    NAME libdxfrw_image_definition_plan_resource_bounds
    COMMAND libdxfrw_dxf_fixture_tests --image-definition-plan-resource-bounds
)
set_tests_properties(libdxfrw_image_definition_plan_resource_bounds PROPERTIES
    LABELS "fast;dxf;image;resource"
    TIMEOUT 30
)

add_test(NAME libdxfrw_table_attribute_count_bound
    COMMAND libdxfrw_dxf_fixture_tests --table-attribute-count-bound)
set_tests_properties(libdxfrw_table_attribute_count_bound PROPERTIES
    LABELS "fast;dxf;table;writer;transaction"
    TIMEOUT 30)

add_test(NAME libdxfrw_group_setter_bounds
    COMMAND libdxfrw_dxf_fixture_tests --group-setter-bounds)
set_tests_properties(libdxfrw_group_setter_bounds PROPERTIES
    LABELS "fast;dxf;group;adapter;resource"
    TIMEOUT 30)

add_test(
    NAME libdxfrw_generated_group_dictionary_objects_allocation_retry
    COMMAND libdxfrw_dxf_fixture_tests
        --generated-group-dictionary-objects-allocation-retry)
set_tests_properties(
    libdxfrw_generated_group_dictionary_objects_allocation_retry PROPERTIES
    LABELS "fast;dxf;writer;objects;group;dictionary;allocation;transaction"
    TIMEOUT 30)

add_test(
    NAME libdxfrw_generated_image_dictionary_objects_allocation_retry
    COMMAND libdxfrw_dxf_fixture_tests
        --generated-image-dictionary-objects-allocation-retry)
set_tests_properties(
    libdxfrw_generated_image_dictionary_objects_allocation_retry PROPERTIES
    LABELS "fast;dxf;writer;objects;image;dictionary;allocation;transaction"
    TIMEOUT 30)

add_test(NAME libdxfrw_generated_dictionary_source_allocation_failure
    COMMAND libdxfrw_dxf_fixture_tests
        --generated-dictionary-source-allocation-failure)
set_tests_properties(libdxfrw_generated_dictionary_source_allocation_failure
    PROPERTIES LABELS "fast;dxf;writer;objects;dictionary;allocation;transaction"
    TIMEOUT 30)

add_test(NAME libdxfrw_root_dict_entry_count_bound
    COMMAND libdxfrw_dxf_fixture_tests --root-dict-entry-count-bound)
set_tests_properties(libdxfrw_root_dict_entry_count_bound PROPERTIES
    LABELS "fast;dxf;dictionary;resource"
    TIMEOUT 30)

add_test(NAME libdxfrw_named_dict_object_count_bound
    COMMAND libdxfrw_dxf_fixture_tests --named-dict-object-count-bound)
set_tests_properties(libdxfrw_named_dict_object_count_bound PROPERTIES
    LABELS "fast;dxf;dictionary;resource"
    TIMEOUT 30)

add_test(NAME libdxfrw_dxf_class_count_bound
    COMMAND libdxfrw_dxf_fixture_tests --dxf-class-count-bound)
set_tests_properties(libdxfrw_dxf_class_count_bound PROPERTIES
    LABELS "fast;dxf;classes;resource"
    TIMEOUT 30)

add_test(NAME libdxfrw_raw_dxf_section_setter_bound
    COMMAND libdxfrw_dxf_fixture_tests --raw-dxf-section-setter-bound)
set_tests_properties(libdxfrw_raw_dxf_section_setter_bound PROPERTIES
    LABELS "fast;dxf;raw-section;resource;transaction"
    TIMEOUT 30)

add_test(NAME libdxfrw_resolved_write_reference_setter_bounds
    COMMAND libdxfrw_dxf_fixture_tests --resolved-write-reference-setter-bounds)
set_tests_properties(libdxfrw_resolved_write_reference_setter_bounds PROPERTIES
    LABELS "fast;dxf;reference;resource;transaction"
    TIMEOUT 30)

add_test(NAME libdxfrw_dxf_handle_remap_setter_bound
    COMMAND libdxfrw_dxf_fixture_tests --dxf-handle-remap-setter-bound)
set_tests_properties(libdxfrw_dxf_handle_remap_setter_bound PROPERTIES
    LABELS "fast;dxf;handle-remap;resource;transaction"
    TIMEOUT 30)

add_test(NAME libdxfrw_dxf_source_table_metadata_setter_bounds
    COMMAND libdxfrw_dxf_fixture_tests --dxf-source-table-metadata-setter-bounds)
set_tests_properties(libdxfrw_dxf_source_table_metadata_setter_bounds PROPERTIES
    LABELS "fast;dxf;table;resource;transaction"
    TIMEOUT 30)

add_test(NAME libdxfrw_tablecontent_multicell_field_reference
    COMMAND libdxfrw_dxf_fixture_tests --tablecontent-multicell-field)
set_tests_properties(libdxfrw_tablecontent_multicell_field_reference PROPERTIES
    LABELS "fast;dxf;adapter;table;field;generated;transaction"
    TIMEOUT 30)

add_test(NAME libdxfrw_tablecontent_multicell_block_reference
    COMMAND libdxfrw_dxf_fixture_tests --tablecontent-multicell-block)
set_tests_properties(libdxfrw_tablecontent_multicell_block_reference PROPERTIES
    LABELS "fast;dxf;adapter;table;block;generated;transaction"
    TIMEOUT 30)

# Optional test of the installed converter from FreeCAD's actual desktop
# process. Generate its AC1021 LINE source locally with the existing writer
# control, then require strict installed-converter preflight before launching
# FreeCAD. Keep generated DWG/DXF files under the isolated test root.
option(LIBDXFRW_ENABLE_FREECAD_DESKTOP_CONTROL
    "Enable the opt-in FreeCAD GUI dwg2dxf generated-source open/insert control" OFF)
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
            "-DDWG_SOURCE_GENERATOR=$<TARGET_FILE:libdxfrw_dwg_local_roundtrip>"
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

# Native Linux/Windows qualification uses the same locally generated AC1021
# 3D LINE source and strict converter preflight as the desktop control above.
# Keep it opt-in: it launches the pinned FreeCAD consumer twice (open/insert)
# and is intended for manual platform qualification, not every fast CI run.
option(LIBDXFRW_ENABLE_FREECAD_GENERATED_AC1021_LINE_CONTROL
    "Enable pinned FreeCAD checks using the generated AC1021 3D LINE control" OFF)
set(LIBDXFRW_FREECAD_EXPECTED_REVISION
    "4fd3bf320d9566a27e60069fc8387448aaa3a094" CACHE STRING
    "Expected FreeCAD source revision for native integration checks")
if(LIBDXFRW_ENABLE_FREECAD_GENERATED_AC1021_LINE_CONTROL)
    libdxfrw_require_test_cli(LIBDXFRW_ENABLE_FREECAD_GENERATED_AC1021_LINE_CONTROL)
    if(NOT EXISTS "${LIBDXFRW_FREECADCMD_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECADCMD_EXECUTABLE to FreeCAD's FreeCADCmd/freecadcmd")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE to the installed dwg2dxf")
    endif()
    foreach(_operation IN ITEMS open insert)
        add_test(
            NAME dwg2dxf_freecad_generated_ac1021_line_${_operation}
            COMMAND ${CMAKE_COMMAND}
                "-DFREECADCMD=${LIBDXFRW_FREECADCMD_EXECUTABLE}"
                "-DDWG2DXF=${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}"
                "-DBUILD_DWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DDWG_SOURCE_GENERATOR=$<TARGET_FILE:libdxfrw_dwg_local_roundtrip>"
                -DEXPECTED_MAGIC_HEX=414331303231
                "-DIMPORT_CHECK_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_dwg2dxf_import_check.FCMacro"
                "-DOPERATION=${_operation}"
                -DINPUT_PATH_UNICODE=ON
                "-DEXPECTED_FREECAD_REVISION=${LIBDXFRW_FREECAD_EXPECTED_REVISION}"
                -DEXPECTED_LINE_BOUNDS=[[1,2,3,4,5,6]]
                -DEXPECTED_LINE_ENDPOINTS=[[[1,2,3],[4,5,6]]]
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_installed_3d_line_test.cmake
        )
        set_tests_properties(dwg2dxf_freecad_generated_ac1021_line_${_operation}
            PROPERTIES
                LABELS "freecad;installed;generated;integration;AC1021;platform"
                TIMEOUT 180)
    endforeach()
endif()

# A locally generated mixed 2D/3D primitive source broadens the native
# consumer check beyond a single LINE while keeping exact geometry assertions.
option(LIBDXFRW_ENABLE_FREECAD_GENERATED_AC1021_PRIMITIVE_CONTROL
    "Enable FreeCAD open/insert checks for generated AC1021, AC1024, and AC1032 2D/3D primitives" OFF)
if(LIBDXFRW_ENABLE_FREECAD_GENERATED_AC1021_PRIMITIVE_CONTROL)
    libdxfrw_require_test_cli(LIBDXFRW_ENABLE_FREECAD_GENERATED_AC1021_PRIMITIVE_CONTROL)
    if(NOT EXISTS "${LIBDXFRW_FREECADCMD_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECADCMD_EXECUTABLE to FreeCAD's FreeCADCmd/freecadcmd")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE to the installed dwg2dxf")
    endif()
    foreach(_version IN ITEMS AC1021 AC1024 AC1032)
        if(_version STREQUAL "AC1021")
            set(_magic_hex 414331303231)
        elseif(_version STREQUAL "AC1024")
            set(_magic_hex 414331303234)
        else()
            set(_magic_hex 414331303332)
        endif()
        string(TOLOWER "${_version}" _version_lc)
        foreach(_operation IN ITEMS open insert)
            add_test(
                NAME dwg2dxf_freecad_generated_${_version_lc}_primitives_${_operation}
                COMMAND ${CMAKE_COMMAND}
                    "-DFREECADCMD=${LIBDXFRW_FREECADCMD_EXECUTABLE}"
                    "-DDWG2DXF=${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}"
                    "-DBUILD_DWG2DXF=$<TARGET_FILE:dwg2dxf>"
                    "-DDWG_SOURCE_GENERATOR=$<TARGET_FILE:libdxfrw_dwg_local_roundtrip>"
                    "-DDWG_SOURCE_VERSION=${_version}"
                    "-DEXPECTED_MAGIC_HEX=${_magic_hex}"
                    "-DEXPECTED_FREECAD_REVISION=${LIBDXFRW_FREECAD_EXPECTED_REVISION}"
                    "-DEXPECT_3DFACE=unsupported"
                    "-DEXPECT_SOLID=unsupported"
                    "-DEXPECTED_PRIMITIVE_COUNTS=POINT:1,LINE:1,CIRCLE:1,ARC:1,ELLIPSE:1,LWPOLYLINE:1,POLYLINE:3,3DFACE:1,SOLID:1,SPLINE:1"
                    "-DEXPECTED_PRIMITIVE_SHAPES=1,2,3,4,5,6,1,2,5.196152422706632,0|7.5,8.5,12,12.5,13.5,12,1,1,15.707963267948966,0|14.10362826838344,14.86591385739083,15,16.391193475987258,17.32144616774455,15,1,2,3.5,0|22,25,28,30,29,28,1,1,19.37689644109535,0|16,17,0,18,20,0,2,3,5,0|21,22,23,21,22,23,0,1,0,0|61,62,1,70,68,8,3,4,14.649695377207546,0|91,92,0,95,96,0,4,4,16,0|0,0,0,0,0,0,1,2,7.880049135263712,1"
                    "-DEXPECTED_FREECAD_POLYFACE={\"triangles\":2,\"bounds\":[121,122,1,124,126,8],\"points\":[[121,122,1],[124,122,4],[124,126,8],[121,126,5]]}"
                    "-DEXPECTED_SPLINE_SAMPLES=[[0,31,32,33],[0.5,33,34.5,35],[1,35,32,37]]"
                    "-DEXPECTED_POLYLINE_POINTS=[[61,62,1],[64,62,4],[65,66,5],[70,68,8]]"
                    "-DEXPECTED_CLOSED_2D_POLYLINE_POINTS=[[91,92,0],[95,92,0],[95,96,0],[91,96,0]]"
                    "-DIMPORT_CHECK_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_dwg2dxf_import_check.FCMacro"
                    "-DOPERATION=${_operation}"
                    -DINPUT_PATH_UNICODE=ON
                    -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_installed_3d_line_test.cmake
            )
            set_tests_properties(dwg2dxf_freecad_generated_${_version_lc}_primitives_${_operation}
                PROPERTIES
                    LABELS "freecad;installed;generated;integration;${_version};platform;primitives"
                    TIMEOUT 180)
        endforeach()
    endforeach()
    add_test(
        NAME freecad_dxf_minsert_direct_import_diagnostic
        COMMAND "${LIBDXFRW_FREECADCMD_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/freecad_dxf_minsert_import_check.FCMacro"
    )
    set_tests_properties(freecad_dxf_minsert_direct_import_diagnostic PROPERTIES
        ENVIRONMENT
            "LIBDXFRW_FREECAD_DXF=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dxf/ac1015_minsert_freecad_control.dxf"
        LABELS "freecad;native;diagnostic;AC1015"
        TIMEOUT 120
    )
endif()

option(LIBDXFRW_ENABLE_FREECAD_LEGACY_POLYFACE_CONTROL
    "Enable FreeCAD legacy Python importer POLYFACE open/insert checks" OFF)
set(LIBDXFRW_FREECAD_LEGACY_DXF_ADDON_DIR "" CACHE PATH
    "Explicit local Draft-dxf-importer module directory (no automatic download)")
if(LIBDXFRW_ENABLE_FREECAD_LEGACY_POLYFACE_CONTROL)
    libdxfrw_require_test_cli(LIBDXFRW_ENABLE_FREECAD_LEGACY_POLYFACE_CONTROL)
    foreach(_required IN ITEMS
            LIBDXFRW_FREECADCMD_EXECUTABLE
            LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE
            LIBDXFRW_FREECAD_LEGACY_DXF_ADDON_DIR)
        if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
            message(FATAL_ERROR "${_required} must be set for the legacy POLYFACE control")
        endif()
    endforeach()
    foreach(_module IN ITEMS
            dxfReader.py dxfLibrary.py dxfColorMap.py dxfImportObjects.py)
        if(NOT EXISTS "${LIBDXFRW_FREECAD_LEGACY_DXF_ADDON_DIR}/${_module}")
            message(FATAL_ERROR
                "Missing ${_module} under LIBDXFRW_FREECAD_LEGACY_DXF_ADDON_DIR")
        endif()
    endforeach()
    foreach(_required IN ITEMS
            LIBDXFRW_FREECADCMD_EXECUTABLE
            LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE)
        if(NOT EXISTS "${${_required}}")
            message(FATAL_ERROR "${_required} does not exist: ${${_required}}")
        endif()
    endforeach()
    foreach(_version IN ITEMS AC1021 AC1024 AC1032)
        string(HEX "${_version}" _magic)
        string(TOLOWER "${_version}" _version_lower)
        foreach(_operation IN ITEMS open insert)
            if(_version STREQUAL "AC1021")
                # Preserve the original names for existing callers.
                set(_test_name dwg2dxf_freecad_legacy_polyface_${_operation})
            else()
                set(_test_name dwg2dxf_freecad_legacy_polyface_${_version_lower}_${_operation})
            endif()
            add_test(
                NAME ${_test_name}
                COMMAND ${CMAKE_COMMAND}
                    "-DFREECADCMD=${LIBDXFRW_FREECADCMD_EXECUTABLE}"
                    "-DDWG2DXF=${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}"
                    "-DBUILD_DWG2DXF=$<TARGET_FILE:dwg2dxf>"
                    "-DDWG_SOURCE_GENERATOR=$<TARGET_FILE:libdxfrw_dwg_local_roundtrip>"
                    "-DDWG_SOURCE_VERSION=${_version}"
                    "-DEXPECTED_MAGIC_HEX=${_magic}"
                    "-DIMPORT_CHECK_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_legacy_polyface_import_check.FCMacro"
                    -DIMPORT_CHECK_MODE=legacy_polyface
                    "-DFREECAD_LEGACY_DXF_ADDON_DIR=${LIBDXFRW_FREECAD_LEGACY_DXF_ADDON_DIR}"
                    "-DOPERATION=${_operation}"
                    -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_installed_3d_line_test.cmake
            )
            set_tests_properties(${_test_name}
                PROPERTIES
                    LABELS "freecad;installed;generated;integration;${_version};platform;legacy;polyface"
                    TIMEOUT 180)
        endforeach()
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

# The public target must carry the language level required by the installed
# headers. Model an older downstream's own requested standard and let dxfrw's
# PUBLIC cxx_std_17 usage requirement raise the effective compile mode. This
# catches regressions that the direct C++17 API facade check above cannot.
add_library(libdxfrw_consumer_cxx17_contract OBJECT
    ${CMAKE_CURRENT_SOURCE_DIR}/consumer_cxx17_contract.cpp
)
set_target_properties(libdxfrw_consumer_cxx17_contract PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
target_link_libraries(libdxfrw_consumer_cxx17_contract PRIVATE dxfrw)

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
add_test(NAME libdxfrw_dwg_class_source_tail
    COMMAND libdxfrw_dwg_class_source_tests --classes-tail-only)
set_tests_properties(libdxfrw_dwg_class_source_tail PROPERTIES
    LABELS "dwg;classes;source;fast" TIMEOUT 10)

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

add_test(NAME libdxfrw_block_record_body_handle_roles
    COMMAND libdxfrw_dwg_reader_matrix_tests
        --block-record-body-handle-roles)
set_tests_properties(libdxfrw_block_record_body_handle_roles PROPERTIES
    LABELS "fast;dwg;tables;handles;generated" TIMEOUT 15)

add_test(NAME libdxfrw_ac1027_common_entity_handle_roles
    COMMAND libdxfrw_dwg_reader_matrix_tests
        --ac1027-common-entity-handle-roles)
set_tests_properties(libdxfrw_ac1027_common_entity_handle_roles PROPERTIES
    LABELS "fast;dwg;entities;handles;generated" TIMEOUT 15)

add_test(NAME libdxfrw_entity_space_from_block_record
    COMMAND libdxfrw_dwg_reader_matrix_tests
        --entity-space-from-block-record)
set_tests_properties(libdxfrw_entity_space_from_block_record PROPERTIES
    LABELS "fast;dwg;entities;ownership;space;generated" TIMEOUT 15)

add_test(NAME libdxfrw_block_control_null_member_diagnostic
    COMMAND libdxfrw_dwg_reader_matrix_tests --control-handle-slots)
set_tests_properties(libdxfrw_block_control_null_member_diagnostic PROPERTIES
    LABELS "fast;dwg;tables;integrity;generated" TIMEOUT 15)

add_test(NAME libdxfrw_associative_constraint_group_generated
    COMMAND libdxfrw_dwg_reader_matrix_tests
        --associative-constraint-group-generated)
set_tests_properties(libdxfrw_associative_constraint_group_generated PROPERTIES
    LABELS "fast;dwg;objects;associative;generated" TIMEOUT 15)

add_test(NAME libdxfrw_associative_network_action_index_generated
    COMMAND libdxfrw_dwg_reader_matrix_tests
        --associative-network-action-index-generated)
set_tests_properties(libdxfrw_associative_network_action_index_generated
    PROPERTIES LABELS "fast;dwg;objects;associative;generated" TIMEOUT 15)

add_executable(libdxfrw_assoc_group_registry_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/assoc_group_registry_allocation_tests.cpp)
target_compile_features(libdxfrw_assoc_group_registry_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_assoc_group_registry_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_assoc_group_registry_allocation_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_assoc_group_registry_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_assoc_group_registry_allocation
    COMMAND libdxfrw_assoc_group_registry_allocation_tests)
set_tests_properties(libdxfrw_assoc_group_registry_allocation PROPERTIES
    LABELS "fast;dwg;associative;constraint-group;transaction" TIMEOUT 30)
add_executable(libdxfrw_associative_constraint_group_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/associative_constraint_group_allocation_tests.cpp)
target_compile_features(libdxfrw_associative_constraint_group_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_associative_constraint_group_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_associative_constraint_group_allocation_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_associative_constraint_group_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_associative_constraint_group_allocation_retry
    COMMAND libdxfrw_associative_constraint_group_allocation_tests)
set_tests_properties(libdxfrw_associative_constraint_group_allocation_retry PROPERTIES
    LABELS "fast;dxf;associative;constraint-group;allocation;transaction;generated" TIMEOUT 30)
add_executable(libdxfrw_constraint_group_candidate_api_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/constraint_group_candidate_api_tests.cpp)
target_compile_features(libdxfrw_constraint_group_candidate_api_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_constraint_group_candidate_api_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_constraint_group_candidate_api_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_constraint_group_candidate_api_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_constraint_group_candidate_api
    COMMAND libdxfrw_constraint_group_candidate_api_tests)
set_tests_properties(libdxfrw_constraint_group_candidate_api PROPERTIES
    LABELS "fast;dwg;associative;constraint-group;generated" TIMEOUT 30)
add_test(NAME libdxfrw_constraint_group_ac1032_candidate_generated
    COMMAND libdxfrw_dwg_reader_matrix_tests
        --constraint-group-ac1032-candidate-generated)
set_tests_properties(libdxfrw_constraint_group_ac1032_candidate_generated
    PROPERTIES LABELS "fast;dwg;associative;constraint-group" TIMEOUT 30)
add_test(NAME libdxfrw_assocnetwork_ac1032_candidate_generated
    COMMAND libdxfrw_dwg_reader_matrix_tests
        --assocnetwork-ac1032-candidate-generated)
set_tests_properties(libdxfrw_assocnetwork_ac1032_candidate_generated
    PROPERTIES LABELS "fast;dwg;objects;associative;generated;AC1032"
               TIMEOUT 15)

add_test(NAME libdxfrw_assocnetwork_ac1032_multi_action_generated
    COMMAND libdxfrw_dwg_reader_matrix_tests
        --assocnetwork-ac1032-multi-action-generated)
set_tests_properties(libdxfrw_assocnetwork_ac1032_multi_action_generated
    PROPERTIES LABELS "fast;dwg;objects;associative;generated;AC1032"
               TIMEOUT 15)

add_test(NAME libdxfrw_associative_variable_generated
    COMMAND libdxfrw_dwg_reader_matrix_tests
        --associative-variable-generated)
set_tests_properties(libdxfrw_associative_variable_generated PROPERTIES
    LABELS "fast;dwg;objects;associative;generated" TIMEOUT 15)

set(_assoc_custom_sample
    "${CMAKE_CURRENT_SOURCE_DIR}/samples/AC1024/visualization_-_sun_and_sky_demo.dwg")
if(EXISTS "${_assoc_custom_sample}")
    add_test(
        NAME dwg_associative_custom_object_raw_retention
        COMMAND $<TARGET_FILE:libdxfrw_dwg_reader_matrix_tests>
            --associative-custom-object-raw-retention "${_assoc_custom_sample}")
    set_tests_properties(dwg_associative_custom_object_raw_retention PROPERTIES
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
        set(_expected_object_parse_failures 0)
        set(_expected_untyped_acsh_loft_frames 0)
        set(_expected_untyped_acsh_revolve_frames 0)
        if(_ver STREQUAL "AC1024"
                AND _name STREQUAL "visualization_-_condominium_with_skylight")
            # The three exact AC1024 AcSh failures have dedicated raw-frame
            # integrity controls below. The sample probe still requires each
            # failure to match one validated AC1024 AcSh frame and requires
            # complete source-frame disposition.
            set(_expected_object_parse_failures 3)
            set(_expected_untyped_acsh_loft_frames 2)
            set(_expected_untyped_acsh_revolve_frames 1)
        elseif(_ver STREQUAL "AC1024"
                AND _name STREQUAL "visualization_-_conference_room")
            set(_expected_object_parse_failures 3)
            set(_expected_untyped_acsh_revolve_frames 3)
        endif()
        add_test(
            NAME dwg_read_${_ver}_${_name}
            COMMAND ${CMAKE_COMMAND}
                -DREADER_TEST=$<TARGET_FILE:libdxfrw_dwg_reader_matrix_tests>
                -DSAMPLE=${_sample}
                -DEXPECTED_VERSION=${_ver}
                -DEXPECTED_OBJECT_PARSE_FAILURES=${_expected_object_parse_failures}
                -DEXPECTED_UNTYPED_ACSH_LOFT_FRAMES=${_expected_untyped_acsh_loft_frames}
                -DEXPECTED_UNTYPED_ACSH_REVOLVE_FRAMES=${_expected_untyped_acsh_revolve_frames}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg_read_test.cmake
        )
        set_tests_properties(dwg_read_${_ver}_${_name} PROPERTIES
            LABELS "dwg_read;${_ver};sample"
            TIMEOUT 60
        )
    endforeach()
endforeach()

# These bounded AC1024 custom-object profiles use a locally supplied sample;
# no external DWG is vendored. They validate the selected BLOCKLOOKUPACTION
# grid/column layout and the handle-363 PERSUBENTMGR partial prefix while
# retaining the latter as an unvalidated raw carrier.
set(_custom_objects_ac1024_sample
    "${CMAKE_CURRENT_SOURCE_DIR}/samples/AC1024/visualization_-_sun_and_sky_demo.dwg")
if(EXISTS "${_custom_objects_ac1024_sample}")
    add_test(NAME dwg_read_AC1024_selected_custom_object_profiles
        COMMAND $<TARGET_FILE:libdxfrw_dwg_reader_matrix_tests>
            --custom-objects-ac1024-sample
            "${_custom_objects_ac1024_sample}")
    set_tests_properties(dwg_read_AC1024_selected_custom_object_profiles PROPERTIES
        LABELS "dwg_read;AC1024;sample;fast" TIMEOUT 60)
endif()

# AC1021 BLOCK_CONTROL common-handle reactor ordering and reset/retry.
add_executable(libdxfrw_block_control_reactor_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/block_control_reactor_tests.cpp)
target_compile_features(libdxfrw_block_control_reactor_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_block_control_reactor_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_block_control_reactor_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_block_control_reactor_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_block_control_reactors COMMAND libdxfrw_block_control_reactor_tests)
set_tests_properties(libdxfrw_dwg_block_control_reactors PROPERTIES
    LABELS "fast;dwg;AC1021;transaction" TIMEOUT 15)

add_executable(libdxfrw_dwg_material_replay_refusal_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_material_replay_refusal_tests.cpp)
target_compile_features(libdxfrw_dwg_material_replay_refusal_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_material_replay_refusal_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_material_replay_refusal_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_material_replay_refusal_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_material_replay_refusal COMMAND libdxfrw_dwg_material_replay_refusal_tests)
set_tests_properties(libdxfrw_dwg_material_replay_refusal PROPERTIES
    LABELS "fast;dwg;material;transaction" TIMEOUT 30)

add_executable(libdxfrw_dwg_ac1024_header_suffix_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_ac1024_header_suffix_tests.cpp)
target_compile_features(libdxfrw_dwg_ac1024_header_suffix_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_ac1024_header_suffix_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_ac1024_header_suffix_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_ac1024_header_suffix_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_ac1024_header_suffix COMMAND libdxfrw_dwg_ac1024_header_suffix_tests)
set_tests_properties(libdxfrw_dwg_ac1024_header_suffix PROPERTIES LABELS "dwg;fast;header" TIMEOUT 30)
add_executable(libdxfrw_dxf_lightlist_reference_tests ${CMAKE_CURRENT_SOURCE_DIR}/dxf_lightlist_reference_tests.cpp)
target_compile_features(libdxfrw_dxf_lightlist_reference_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_lightlist_reference_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_lightlist_reference_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_lightlist_reference_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_lightlist_reference COMMAND libdxfrw_dxf_lightlist_reference_tests)
set_tests_properties(libdxfrw_dxf_lightlist_reference PROPERTIES LABELS "dxf;fast;lightlist;reference" TIMEOUT 30)

add_executable(libdxfrw_dxf_datatable_read_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_datatable_read_tests.cpp)
target_compile_features(libdxfrw_dxf_datatable_read_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_datatable_read_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_datatable_read_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_datatable_read_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_datatable_read COMMAND libdxfrw_dxf_datatable_read_tests)
set_tests_properties(libdxfrw_dxf_datatable_read PROPERTIES LABELS "dxf;fast;table" TIMEOUT 30)

add_executable(libdxfrw_field_objects_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/field_objects_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_field_objects_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_field_objects_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_field_objects_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_field_objects_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_field_objects_allocation
    COMMAND libdxfrw_field_objects_allocation_tests
        ${CMAKE_CURRENT_BINARY_DIR}/field-objects-allocation)
set_tests_properties(libdxfrw_field_objects_allocation PROPERTIES
    LABELS "dxf;fast;field;transaction" TIMEOUT 30)

add_executable(libdxfrw_evaluation_graph_objects_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/evaluation_graph_objects_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_evaluation_graph_objects_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_evaluation_graph_objects_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_evaluation_graph_objects_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_evaluation_graph_objects_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_evaluation_graph_objects_allocation
    COMMAND libdxfrw_evaluation_graph_objects_allocation_tests
        ${CMAKE_CURRENT_BINARY_DIR}/evaluation-graph-objects-allocation)
set_tests_properties(libdxfrw_evaluation_graph_objects_allocation PROPERTIES
    LABELS "dxf;fast;objects;transaction" TIMEOUT 30)

add_executable(libdxfrw_proxy_entity_object_id_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/proxy_entity_object_id_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_proxy_entity_object_id_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_proxy_entity_object_id_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_proxy_entity_object_id_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_proxy_entity_object_id_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_proxy_entity_object_id_allocation
    COMMAND libdxfrw_proxy_entity_object_id_allocation_tests
        ${CMAKE_CURRENT_BINARY_DIR}/proxy-entity-object-id-allocation)
set_tests_properties(libdxfrw_proxy_entity_object_id_allocation PROPERTIES
    LABELS "dxf;fast;entity;transaction;allocation;versions" TIMEOUT 120)

add_executable(libdxfrw_3d_entity_version_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_3d_entity_version_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_3d_entity_version_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_3d_entity_version_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_3d_entity_version_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_3d_entity_version_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_3d_entity_version_allocation
    COMMAND libdxfrw_3d_entity_version_allocation_tests
        ${CMAKE_CURRENT_BINARY_DIR}/3d-entity-version-allocation)
set_tests_properties(libdxfrw_3d_entity_version_allocation PROPERTIES
    LABELS "dxf;fast;entity;3d;transaction;allocation;versions" TIMEOUT 120)
add_executable(libdxfrw_attrib_version_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_attrib_version_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_attrib_version_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_attrib_version_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_attrib_version_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_attrib_version_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_attrib_version_allocation
    COMMAND libdxfrw_attrib_version_allocation_tests
        ${CMAKE_CURRENT_BINARY_DIR}/attrib-version-allocation)
set_tests_properties(libdxfrw_attrib_version_allocation PROPERTIES
    LABELS "dxf;fast;entity;attrib;transaction;allocation;versions" TIMEOUT 120)
add_test(NAME libdxfrw_sectionobject_version_allocation
    COMMAND libdxfrw_3d_entity_version_allocation_tests
        ${CMAKE_CURRENT_BINARY_DIR}/sectionobject-version-allocation
        --sectionobject-versions)
set_tests_properties(libdxfrw_sectionobject_version_allocation PROPERTIES
    LABELS "dxf;fast;entity;sectionobject;transaction;allocation;versions"
    TIMEOUT 30)
add_test(NAME libdxfrw_pointcloud_version_allocation
    COMMAND libdxfrw_3d_entity_version_allocation_tests
        ${CMAKE_CURRENT_BINARY_DIR}/pointcloud-version-allocation
        --pointcloud-versions)
set_tests_properties(libdxfrw_pointcloud_version_allocation PROPERTIES
    LABELS "dxf;fast;entity;pointcloud;transaction;allocation;versions"
    TIMEOUT 30)
add_test(NAME libdxfrw_pointcloudex_version_allocation
    COMMAND libdxfrw_3d_entity_version_allocation_tests
        ${CMAKE_CURRENT_BINARY_DIR}/pointcloudex-version-allocation
        --pointcloudex-versions)
set_tests_properties(libdxfrw_pointcloudex_version_allocation PROPERTIES
    LABELS "dxf;fast;entity;pointcloudex;transaction;allocation;versions"
    TIMEOUT 30)
add_test(NAME libdxfrw_navisworks_version_allocation
    COMMAND libdxfrw_3d_entity_version_allocation_tests
        ${CMAKE_CURRENT_BINARY_DIR}/navisworks-version-allocation
        --navisworks-versions)
set_tests_properties(libdxfrw_navisworks_version_allocation PROPERTIES
    LABELS "dxf;fast;entity;navisworks;transaction;allocation;versions"
    TIMEOUT 30)
add_executable(libdxfrw_navisworks_model_def_version_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_navisworks_model_def_version_allocation_tests.cpp)
target_compile_features(libdxfrw_navisworks_model_def_version_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_navisworks_model_def_version_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_navisworks_model_def_version_allocation_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_navisworks_model_def_version_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_navisworks_model_def_version_allocation
    COMMAND libdxfrw_navisworks_model_def_version_allocation_tests
        ${CMAKE_CURRENT_BINARY_DIR}/navisworks-model-def-version-allocation)
set_tests_properties(libdxfrw_navisworks_model_def_version_allocation PROPERTIES
    LABELS "dxf;fast;objects;navisworks;transaction;allocation;versions"
    TIMEOUT 30)
add_executable(libdxfrw_pointcloud_def_version_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_pointcloud_def_version_allocation_tests.cpp)
target_compile_features(libdxfrw_pointcloud_def_version_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_pointcloud_def_version_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_pointcloud_def_version_allocation_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_pointcloud_def_version_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_pointcloud_def_version_allocation
    COMMAND libdxfrw_pointcloud_def_version_allocation_tests
        ${CMAKE_CURRENT_BINARY_DIR}/pointcloud-def-version-allocation)
set_tests_properties(libdxfrw_pointcloud_def_version_allocation PROPERTIES
    LABELS "dxf;fast;objects;pointcloud;transaction;allocation;versions"
    TIMEOUT 30)
add_executable(libdxfrw_pointcloud_colormap_version_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_pointcloud_colormap_version_allocation_tests.cpp)
target_compile_features(libdxfrw_pointcloud_colormap_version_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_pointcloud_colormap_version_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_pointcloud_colormap_version_allocation_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_pointcloud_colormap_version_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_pointcloud_colormap_version_allocation
    COMMAND libdxfrw_pointcloud_colormap_version_allocation_tests
        ${CMAKE_CURRENT_BINARY_DIR}/pointcloud-colormap-version-allocation)
set_tests_properties(libdxfrw_pointcloud_colormap_version_allocation PROPERTIES
    LABELS "dxf;fast;objects;pointcloud;transaction;allocation;versions"
    TIMEOUT 60)
add_executable(libdxfrw_wipeout_version_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_wipeout_version_allocation_tests.cpp)
target_compile_features(libdxfrw_wipeout_version_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_wipeout_version_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_wipeout_version_allocation_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_wipeout_version_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_wipeout_version_allocation
    COMMAND libdxfrw_wipeout_version_allocation_tests
        ${CMAKE_CURRENT_BINARY_DIR}/wipeout-version-allocation)
set_tests_properties(libdxfrw_wipeout_version_allocation PROPERTIES
    LABELS "dxf;fast;entity;wipeout;transaction;allocation;versions"
    TIMEOUT 60)

add_executable(libdxfrw_sun_owner_override_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/sun_owner_override_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_sun_owner_override_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_sun_owner_override_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_sun_owner_override_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_sun_owner_override_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_sun_owner_override_allocation
    COMMAND libdxfrw_sun_owner_override_allocation_tests
        ${CMAKE_CURRENT_BINARY_DIR}/sun-owner-override-allocation)
set_tests_properties(libdxfrw_sun_owner_override_allocation PROPERTIES
    LABELS "dxf;fast;objects;transaction;allocation" TIMEOUT 30)

add_executable(libdxfrw_raw_object_string_override_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/raw_object_string_override_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_raw_object_string_override_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_raw_object_string_override_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_raw_object_string_override_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_raw_object_string_override_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_raw_object_string_override_allocation
    COMMAND libdxfrw_raw_object_string_override_allocation_tests
        ${CMAKE_CURRENT_BINARY_DIR}/raw-object-string-override-allocation)
set_tests_properties(libdxfrw_raw_object_string_override_allocation PROPERTIES
    LABELS "dxf;fast;objects;transaction;allocation" TIMEOUT 30)

add_executable(libdxfrw_dxf_tablestyle_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_tablestyle_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_dxf_tablestyle_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_tablestyle_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_tablestyle_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_tablestyle_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_tablestyle_allocation
    COMMAND libdxfrw_dxf_tablestyle_allocation_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-tablestyle-allocation)
set_tests_properties(libdxfrw_dxf_tablestyle_allocation PROPERTIES
    LABELS "dxf;fast;objects;tablestyle;transaction;allocation" TIMEOUT 30)

add_executable(libdxfrw_dxf_cellstylemap_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_cellstylemap_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_dxf_cellstylemap_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_cellstylemap_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_cellstylemap_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_cellstylemap_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_cellstylemap_allocation
    COMMAND libdxfrw_dxf_cellstylemap_allocation_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-cellstylemap-allocation)
set_tests_properties(libdxfrw_dxf_cellstylemap_allocation PROPERTIES
    LABELS "dxf;fast;objects;cellstylemap;transaction;allocation" TIMEOUT 30)

add_executable(libdxfrw_dxf_write_allocation_transaction_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_write_allocation_transaction_tests.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_dxf_write_allocation_transaction_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_write_allocation_transaction_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_write_allocation_transaction_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_write_allocation_transaction_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_write_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-write-allocation-transaction)
set_tests_properties(libdxfrw_dxf_write_allocation_transaction PROPERTIES
    LABELS "dxf;fast;transaction;allocation" TIMEOUT 60)
add_test(NAME libdxfrw_dxf_objectptr_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-objectptr-allocation objectptr)
set_tests_properties(libdxfrw_dxf_objectptr_allocation_transaction PROPERTIES
    LABELS "dxf;fast;objects;transaction;allocation;objectptr;versions"
    TIMEOUT 30)
add_test(NAME libdxfrw_dxf_r2018_embedded_attribute_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-r2018-embedded-attribute-allocation
        r2018-embedded-attribute)
set_tests_properties(libdxfrw_dxf_r2018_embedded_attribute_allocation_transaction PROPERTIES
    LABELS "dxf;fast;transaction;allocation" TIMEOUT 60)
add_test(NAME libdxfrw_dxf_legacy_compound_entities_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-legacy-compound-entities-allocation
        legacy-compound-entities)
set_tests_properties(libdxfrw_dxf_legacy_compound_entities_allocation_transaction PROPERTIES
    LABELS "dxf;fast;transaction;allocation" TIMEOUT 60)
add_test(NAME libdxfrw_dxf_polyline_topology_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-polyline-topology-allocation
        polyline-topologies)
set_tests_properties(libdxfrw_dxf_polyline_topology_allocation_transaction PROPERTIES
    LABELS "dxf;fast;transaction;allocation;polyline" TIMEOUT 120)
add_test(NAME libdxfrw_dxf_dimension_version_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-dimension-version-allocation
        dimension-versions)
set_tests_properties(libdxfrw_dxf_dimension_version_allocation_transaction PROPERTIES
    LABELS "dxf;fast;transaction;allocation;dimension;versions" TIMEOUT 120)
add_test(NAME libdxfrw_dxf_hatch_mpolygon_version_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-hatch-mpolygon-version-allocation
        hatch-mpolygon-versions)
set_tests_properties(libdxfrw_dxf_hatch_mpolygon_version_allocation_transaction PROPERTIES
    LABELS "dxf;fast;transaction;allocation;hatch;mpolygon" TIMEOUT 120)
add_test(NAME libdxfrw_dxf_long_mtext_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-long-mtext-allocation
        long-mtext)
set_tests_properties(libdxfrw_dxf_long_mtext_allocation_transaction PROPERTIES
    LABELS "dxf;fast;transaction;allocation" TIMEOUT 60)
add_test(NAME libdxfrw_dxf_lightlist_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-lightlist-allocation
        light-list)
set_tests_properties(libdxfrw_dxf_lightlist_allocation_transaction PROPERTIES
    LABELS "dxf;fast;objects;transaction;allocation" TIMEOUT 60)
add_test(NAME libdxfrw_dxf_light_entity_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-light-entity-allocation
        light-entity)
set_tests_properties(libdxfrw_dxf_light_entity_allocation_transaction PROPERTIES
    LABELS "dxf;fast;transaction;allocation;light" TIMEOUT 60)
add_test(NAME libdxfrw_dxf_ole_frames_version_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-ole-frames-version-allocation
        ole-frames)
set_tests_properties(libdxfrw_dxf_ole_frames_version_allocation_transaction PROPERTIES
    LABELS "dxf;fast;transaction;allocation;ole;versions" TIMEOUT 120)
add_test(NAME libdxfrw_dxf_ray_xline_version_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-ray-xline-version-allocation
        ray-xline)
set_tests_properties(libdxfrw_dxf_ray_xline_version_allocation_transaction PROPERTIES
    LABELS "dxf;fast;transaction;allocation;linear-entities;versions" TIMEOUT 120)
add_test(NAME libdxfrw_dxf_tolerance_version_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-tolerance-version-allocation
        tolerance)
set_tests_properties(libdxfrw_dxf_tolerance_version_allocation_transaction PROPERTIES
    LABELS "dxf;fast;transaction;allocation;dimensions;versions" TIMEOUT 120)
add_test(NAME libdxfrw_dxf_helix_version_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-helix-version-allocation
        helix)
set_tests_properties(libdxfrw_dxf_helix_version_allocation_transaction PROPERTIES
    LABELS "dxf;fast;transaction;allocation;spline;versions" TIMEOUT 120)
add_test(NAME libdxfrw_dxf_spline_version_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-spline-version-allocation
        spline)
set_tests_properties(libdxfrw_dxf_spline_version_allocation_transaction PROPERTIES
    LABELS "dxf;fast;transaction;allocation;spline;versions" TIMEOUT 120)
add_test(NAME libdxfrw_dxf_camera_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-camera-allocation
        camera)
set_tests_properties(libdxfrw_dxf_camera_allocation_transaction PROPERTIES
    LABELS "dxf;fast;transaction;allocation;camera" TIMEOUT 60)
add_test(NAME libdxfrw_dxf_mesh_version_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-mesh-version-allocation
        mesh-versions)
set_tests_properties(libdxfrw_dxf_mesh_version_allocation_transaction PROPERTIES
    LABELS "dxf;fast;transaction;allocation;mesh;versions" TIMEOUT 180)
add_test(NAME libdxfrw_dxf_viewport_version_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-viewport-version-allocation
        viewport-versions)
set_tests_properties(libdxfrw_dxf_viewport_version_allocation_transaction PROPERTIES
    LABELS "dxf;fast;transaction;allocation;viewport;versions" TIMEOUT 180)
add_test(NAME libdxfrw_dxf_geo_marker_version_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-geo-marker-version-allocation
        geo-marker-versions)
set_tests_properties(libdxfrw_dxf_geo_marker_version_allocation_transaction PROPERTIES
    LABELS "dxf;fast;transaction;allocation;geopositionmarker;versions" TIMEOUT 120)
add_test(NAME libdxfrw_dxf_mleader_version_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-mleader-version-allocation
        mleader-versions)
set_tests_properties(libdxfrw_dxf_mleader_version_allocation_transaction PROPERTIES
    LABELS "dxf;fast;transaction;allocation;mleader;versions" TIMEOUT 120)
add_test(NAME libdxfrw_dxf_leader_version_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-leader-version-allocation
        leader-versions)
set_tests_properties(libdxfrw_dxf_leader_version_allocation_transaction PROPERTIES
    LABELS "dxf;fast;transaction;allocation;leader;versions" TIMEOUT 120)
add_test(NAME libdxfrw_dxf_sunstudy_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-sunstudy-allocation
        sun-study)
set_tests_properties(libdxfrw_dxf_sunstudy_allocation_transaction PROPERTIES
    LABELS "dxf;fast;objects;transaction;allocation" TIMEOUT 60)
add_test(NAME libdxfrw_dxf_spatial_filter_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-spatial-filter-allocation
        spatial-filter)
set_tests_properties(libdxfrw_dxf_spatial_filter_allocation_transaction PROPERTIES
    LABELS "dxf;fast;objects;transaction;allocation" TIMEOUT 60)
add_test(NAME libdxfrw_dxf_section_manager_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-section-manager-allocation
        section-manager)
set_tests_properties(libdxfrw_dxf_section_manager_allocation_transaction PROPERTIES
    LABELS "dxf;fast;objects;transaction;allocation" TIMEOUT 60)
add_test(NAME libdxfrw_dxf_section_settings_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-section-settings-allocation
        section-settings)
set_tests_properties(libdxfrw_dxf_section_settings_allocation_transaction PROPERTIES
    LABELS "dxf;fast;objects;transaction;allocation" TIMEOUT 60)
add_test(NAME libdxfrw_dxf_block_record_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-block-record-allocation
        block-record)
set_tests_properties(libdxfrw_dxf_block_record_allocation_transaction PROPERTIES
    LABELS "dxf;fast;tables;transaction;allocation" TIMEOUT 60)
add_test(NAME libdxfrw_dxf_background_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-background-allocation
        background)
set_tests_properties(libdxfrw_dxf_background_allocation_transaction PROPERTIES
    LABELS "dxf;fast;objects;transaction;allocation" TIMEOUT 60)
add_test(NAME libdxfrw_dxf_render_settings_allocation_transaction
    COMMAND libdxfrw_dxf_write_allocation_transaction_tests
        ${CMAKE_CURRENT_BINARY_DIR}/dxf-render-settings-allocation
        render-settings)
set_tests_properties(libdxfrw_dxf_render_settings_allocation_transaction PROPERTIES
    LABELS "dxf;fast;objects;transaction;allocation" TIMEOUT 60)

add_executable(libdxfrw_tablegeometry_objects_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tablegeometry_objects_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_tablegeometry_objects_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_tablegeometry_objects_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_tablegeometry_objects_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_tablegeometry_objects_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_tablegeometry_objects_allocation
    COMMAND libdxfrw_tablegeometry_objects_allocation_tests
        ${CMAKE_CURRENT_BINARY_DIR}/tablegeometry-objects-allocation)
set_tests_properties(libdxfrw_tablegeometry_objects_allocation PROPERTIES
    LABELS "dxf;fast;table;transaction" TIMEOUT 30)

add_executable(libdxfrw_mleaderstyle_objects_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/mleaderstyle_objects_allocation_tests.cpp)
target_compile_features(libdxfrw_mleaderstyle_objects_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_mleaderstyle_objects_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_mleaderstyle_objects_allocation_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_mleaderstyle_objects_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_mleaderstyle_objects_allocation COMMAND libdxfrw_mleaderstyle_objects_allocation_tests)
set_tests_properties(libdxfrw_mleaderstyle_objects_allocation PROPERTIES LABELS "dxf;fast;mleaderstyle;transaction" TIMEOUT 30)

add_executable(libdxfrw_sortents_objects_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/sortents_objects_allocation_tests.cpp)
target_compile_features(libdxfrw_sortents_objects_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_sortents_objects_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_sortents_objects_allocation_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_sortents_objects_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_sortents_objects_allocation COMMAND libdxfrw_sortents_objects_allocation_tests)
set_tests_properties(libdxfrw_sortents_objects_allocation PROPERTIES LABELS "dxf;fast;sortents;transaction" TIMEOUT 30)

add_executable(libdxfrw_dynamic_block_objects_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dynamic_block_objects_allocation_tests.cpp)
target_compile_features(libdxfrw_dynamic_block_objects_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dynamic_block_objects_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dynamic_block_objects_allocation_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dynamic_block_objects_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dynamic_block_objects_allocation
    COMMAND libdxfrw_dynamic_block_objects_allocation_tests)
set_tests_properties(libdxfrw_dynamic_block_objects_allocation PROPERTIES
    LABELS "dxf;fast;dynamic-block;allocation;transaction;generated" TIMEOUT 60)

add_executable(libdxfrw_mline_style_objects_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/mline_style_objects_allocation_tests.cpp)
target_compile_features(libdxfrw_mline_style_objects_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_mline_style_objects_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_mline_style_objects_allocation_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_mline_style_objects_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_mline_style_objects_allocation COMMAND libdxfrw_mline_style_objects_allocation_tests)
set_tests_properties(libdxfrw_mline_style_objects_allocation PROPERTIES LABELS "dxf;fast;mline-style;transaction" TIMEOUT 30)

add_executable(libdxfrw_ucs_table_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/ucs_table_allocation_tests.cpp)
target_compile_features(libdxfrw_ucs_table_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_ucs_table_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_ucs_table_allocation_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_ucs_table_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_ucs_table_allocation COMMAND libdxfrw_ucs_table_allocation_tests)
set_tests_properties(libdxfrw_ucs_table_allocation PROPERTIES LABELS "dxf;fast;ucs;transaction;generated" TIMEOUT 30)

add_executable(libdxfrw_view_table_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/view_table_allocation_tests.cpp)
target_compile_features(libdxfrw_view_table_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_view_table_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_view_table_allocation_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_view_table_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_view_table_allocation COMMAND libdxfrw_view_table_allocation_tests)
set_tests_properties(libdxfrw_view_table_allocation PROPERTIES LABELS "dxf;fast;view;transaction;generated" TIMEOUT 30)

add_executable(libdxfrw_layer_table_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/layer_table_allocation_tests.cpp)
target_compile_features(libdxfrw_layer_table_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_layer_table_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_layer_table_allocation_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_layer_table_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_layer_table_allocation COMMAND libdxfrw_layer_table_allocation_tests)
set_tests_properties(libdxfrw_layer_table_allocation PROPERTIES LABELS "dxf;fast;layer;transaction;generated" TIMEOUT 30)

add_executable(libdxfrw_ltype_table_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/ltype_table_allocation_tests.cpp)
target_compile_features(libdxfrw_ltype_table_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_ltype_table_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_ltype_table_allocation_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_ltype_table_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_ltype_table_allocation COMMAND libdxfrw_ltype_table_allocation_tests)
set_tests_properties(libdxfrw_ltype_table_allocation PROPERTIES LABELS "dxf;fast;ltype;transaction;generated" TIMEOUT 30)

add_executable(libdxfrw_textstyle_table_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/textstyle_table_allocation_tests.cpp)
target_compile_features(libdxfrw_textstyle_table_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_textstyle_table_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_textstyle_table_allocation_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_textstyle_table_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_textstyle_table_allocation COMMAND libdxfrw_textstyle_table_allocation_tests)
set_tests_properties(libdxfrw_textstyle_table_allocation PROPERTIES LABELS "dxf;fast;textstyle;transaction;generated" TIMEOUT 30)

add_executable(libdxfrw_appid_table_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/appid_table_allocation_tests.cpp)
target_compile_features(libdxfrw_appid_table_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_appid_table_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_appid_table_allocation_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_appid_table_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_appid_table_allocation COMMAND libdxfrw_appid_table_allocation_tests)
set_tests_properties(libdxfrw_appid_table_allocation PROPERTIES LABELS "dxf;fast;appid;transaction;generated" TIMEOUT 30)

add_executable(libdxfrw_dimstyle_table_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dimstyle_table_allocation_tests.cpp)
target_compile_features(libdxfrw_dimstyle_table_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dimstyle_table_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dimstyle_table_allocation_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dimstyle_table_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dimstyle_table_allocation COMMAND libdxfrw_dimstyle_table_allocation_tests)
set_tests_properties(libdxfrw_dimstyle_table_allocation PROPERTIES LABELS "dxf;fast;dimstyle;transaction;generated" TIMEOUT 60)

add_executable(libdxfrw_vport_table_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/vport_table_allocation_tests.cpp)
target_compile_features(libdxfrw_vport_table_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_vport_table_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_vport_table_allocation_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_vport_table_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_vport_table_allocation COMMAND libdxfrw_vport_table_allocation_tests)
set_tests_properties(libdxfrw_vport_table_allocation PROPERTIES LABELS "dxf;fast;vport;transaction;generated" TIMEOUT 30)

add_executable(libdxfrw_blockrepresentation_objects_allocation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/blockrepresentation_objects_allocation_tests.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_blockrepresentation_objects_allocation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_blockrepresentation_objects_allocation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_blockrepresentation_objects_allocation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_blockrepresentation_objects_allocation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_blockrepresentation_objects_allocation
    COMMAND libdxfrw_blockrepresentation_objects_allocation_tests
        ${CMAKE_CURRENT_BINARY_DIR}/blockrepresentation-objects-allocation)
set_tests_properties(libdxfrw_blockrepresentation_objects_allocation PROPERTIES
    LABELS "dxf;fast;blockrepresentation;allocation;transaction;generated" TIMEOUT 60)

add_executable(libdxfrw_dxf_light_rgb_421_tests ${CMAKE_CURRENT_SOURCE_DIR}/dxf_light_rgb_421_tests.cpp)
target_compile_features(libdxfrw_dxf_light_rgb_421_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_light_rgb_421_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_light_rgb_421_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_light_rgb_421_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_light_rgb_421 COMMAND libdxfrw_dxf_light_rgb_421_tests)
set_tests_properties(libdxfrw_dxf_light_rgb_421 PROPERTIES LABELS "dxf;dwg;fast;light;transaction" TIMEOUT 30)
add_executable(libdxfrw_dxf_sparse_material_raw_tests ${CMAKE_CURRENT_SOURCE_DIR}/dxf_sparse_material_raw_tests.cpp)
target_compile_features(libdxfrw_dxf_sparse_material_raw_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_sparse_material_raw_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_sparse_material_raw_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_sparse_material_raw_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_sparse_material_raw COMMAND libdxfrw_dxf_sparse_material_raw_tests)
set_tests_properties(libdxfrw_dxf_sparse_material_raw PROPERTIES LABELS "dxf;dwg;fast;material;transaction" TIMEOUT 30)

add_executable(libdxfrw_light_dxf_proxy_replay_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/light_dxf_proxy_replay_tests.cpp)
target_compile_features(libdxfrw_light_dxf_proxy_replay_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_light_dxf_proxy_replay_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_light_dxf_proxy_replay_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_light_dxf_proxy_replay_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_light_dxf_proxy_replay COMMAND libdxfrw_light_dxf_proxy_replay_tests)
set_tests_properties(libdxfrw_light_dxf_proxy_replay PROPERTIES LABELS "fast;dxf;light;transaction" TIMEOUT 30)

add_executable(libdxfrw_dwg_ac1027_block_control_boundary_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_ac1027_block_control_boundary_tests.cpp)
target_compile_features(libdxfrw_dwg_ac1027_block_control_boundary_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_ac1027_block_control_boundary_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_ac1027_block_control_boundary_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_ac1027_block_control_boundary_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_ac1027_block_control_boundary COMMAND libdxfrw_dwg_ac1027_block_control_boundary_tests)
set_tests_properties(libdxfrw_dwg_ac1027_block_control_boundary PROPERTIES LABELS "dwg;fast;transaction;AC1027" TIMEOUT 30)
add_test(NAME libdxfrw_ac1027_block_control_peer_audit COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/ac1027_block_control_peer_audit_tests.py)
set_tests_properties(libdxfrw_ac1027_block_control_peer_audit PROPERTIES LABELS "dwg;fast;AC1027;peer" TIMEOUT 30)

add_executable(libdxfrw_dxf_lightlist_resolver_snapshot_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_lightlist_resolver_snapshot_tests.cpp)
target_compile_features(libdxfrw_dxf_lightlist_resolver_snapshot_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_lightlist_resolver_snapshot_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_lightlist_resolver_snapshot_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_lightlist_resolver_snapshot_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_lightlist_resolver_snapshot COMMAND libdxfrw_dxf_lightlist_resolver_snapshot_tests)
set_tests_properties(libdxfrw_dxf_lightlist_resolver_snapshot PROPERTIES LABELS "fast;dxf;lightlist;transaction" TIMEOUT 30)

add_executable(libdxfrw_dxf_layer_color_order_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_layer_color_order_tests.cpp)
target_compile_features(libdxfrw_dxf_layer_color_order_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_layer_color_order_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_layer_color_order_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_layer_color_order_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_layer_color_order COMMAND libdxfrw_dxf_layer_color_order_tests)
set_tests_properties(libdxfrw_dxf_layer_color_order PROPERTIES LABELS "fast;dxf;layer;color" TIMEOUT 30)

add_executable(libdxfrw_dwg_vertex_tangent_units_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_vertex_tangent_units_tests.cpp)
target_compile_features(libdxfrw_dwg_vertex_tangent_units_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_vertex_tangent_units_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_vertex_tangent_units_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_vertex_tangent_units_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_vertex_tangent_units COMMAND libdxfrw_dwg_vertex_tangent_units_tests)
set_tests_properties(libdxfrw_dwg_vertex_tangent_units PROPERTIES LABELS "fast;dwg;geometry;transaction" TIMEOUT 30)

add_executable(libdxfrw_dwg_ac1027_empty_view_ucs_controls_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_ac1027_empty_view_ucs_controls_tests.cpp)
target_compile_features(libdxfrw_dwg_ac1027_empty_view_ucs_controls_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_ac1027_empty_view_ucs_controls_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_ac1027_empty_view_ucs_controls_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_ac1027_empty_view_ucs_controls_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_ac1027_empty_view_ucs_controls COMMAND libdxfrw_dwg_ac1027_empty_view_ucs_controls_tests)
set_tests_properties(libdxfrw_dwg_ac1027_empty_view_ucs_controls PROPERTIES LABELS "dwg;fast;transaction;AC1027" TIMEOUT 30)
add_test(NAME libdxfrw_ac1027_empty_view_ucs_peer COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/ac1027_empty_view_ucs_peer_tests.py)
set_tests_properties(libdxfrw_ac1027_empty_view_ucs_peer PROPERTIES LABELS "dwg;fast;AC1027;peer" TIMEOUT 30)

add_executable(libdxfrw_dxf_viewport_ambient_color_order_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_viewport_ambient_color_order_tests.cpp)
target_compile_features(libdxfrw_dxf_viewport_ambient_color_order_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_viewport_ambient_color_order_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_viewport_ambient_color_order_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_viewport_ambient_color_order_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_viewport_ambient_color_order COMMAND libdxfrw_dxf_viewport_ambient_color_order_tests)
set_tests_properties(libdxfrw_dxf_viewport_ambient_color_order PROPERTIES LABELS "fast;dxf;viewport;color" TIMEOUT 30)

add_executable(libdxfrw_dxf_viewport_status_domain_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_viewport_status_domain_tests.cpp)
target_compile_features(libdxfrw_dxf_viewport_status_domain_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_viewport_status_domain_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_viewport_status_domain_tests
    PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_viewport_status_domain_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_viewport_status_domain
    COMMAND libdxfrw_dxf_viewport_status_domain_tests)
set_tests_properties(libdxfrw_dxf_viewport_status_domain PROPERTIES
    LABELS "fast;dxf;viewport;domain" TIMEOUT 30)

add_executable(libdxfrw_dwg_hatch_pattern_angle_units_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_hatch_pattern_angle_units_tests.cpp)
target_compile_features(libdxfrw_dwg_hatch_pattern_angle_units_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_hatch_pattern_angle_units_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_hatch_pattern_angle_units_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_hatch_pattern_angle_units_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_hatch_pattern_angle_units COMMAND libdxfrw_dwg_hatch_pattern_angle_units_tests)
set_tests_properties(libdxfrw_dwg_hatch_pattern_angle_units PROPERTIES LABELS "fast;dwg;hatch;transaction" TIMEOUT 30)

add_executable(libdxfrw_dxf_hatch_pattern_angle_units_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dxf_hatch_pattern_angle_units_tests.cpp)
target_compile_features(libdxfrw_dxf_hatch_pattern_angle_units_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_hatch_pattern_angle_units_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_hatch_pattern_angle_units_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_hatch_pattern_angle_units_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_hatch_pattern_angle_units COMMAND libdxfrw_dxf_hatch_pattern_angle_units_tests)
set_tests_properties(libdxfrw_dxf_hatch_pattern_angle_units PROPERTIES LABELS "fast;dxf;hatch;units" TIMEOUT 30)

add_executable(libdxfrw_dxf_sun_color_order_tests ${CMAKE_CURRENT_SOURCE_DIR}/dxf_sun_color_order_tests.cpp)
target_compile_features(libdxfrw_dxf_sun_color_order_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_sun_color_order_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_sun_color_order_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_sun_color_order_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_sun_color_order COMMAND libdxfrw_dxf_sun_color_order_tests)
set_tests_properties(libdxfrw_dxf_sun_color_order PROPERTIES LABELS "fast;dxf;sun;color" TIMEOUT 30)

add_test(NAME libdxfrw_ac1027_material_peer_audit COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/ac1027_material_peer_audit_tests.py)
set_tests_properties(libdxfrw_ac1027_material_peer_audit PROPERTIES LABELS "dwg;fast;AC1027;peer" TIMEOUT 30)

add_executable(libdxfrw_dwg_vport_angle_units_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_vport_angle_units_tests.cpp)
target_compile_features(libdxfrw_dwg_vport_angle_units_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_vport_angle_units_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_vport_angle_units_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_vport_angle_units_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_vport_angle_units COMMAND libdxfrw_dwg_vport_angle_units_tests)
set_tests_properties(libdxfrw_dwg_vport_angle_units PROPERTIES LABELS "fast;dwg;vport;transaction" TIMEOUT 30)

add_executable(libdxfrw_dwg_view_angle_units_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_view_angle_units_tests.cpp)
target_compile_features(libdxfrw_dwg_view_angle_units_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_view_angle_units_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_view_angle_units_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_view_angle_units_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_view_angle_units COMMAND libdxfrw_dwg_view_angle_units_tests)
set_tests_properties(libdxfrw_dwg_view_angle_units PROPERTIES LABELS "fast;dwg;view;transaction" TIMEOUT 30)

add_executable(libdxfrw_dxf_sectionobject_411_tests ${CMAKE_CURRENT_SOURCE_DIR}/dxf_sectionobject_411_tests.cpp)
target_compile_features(libdxfrw_dxf_sectionobject_411_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_sectionobject_411_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_sectionobject_411_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_sectionobject_411_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_sectionobject_411 COMMAND libdxfrw_dxf_sectionobject_411_tests)
set_tests_properties(libdxfrw_dxf_sectionobject_411 PROPERTIES LABELS "fast;dxf;section;raw" TIMEOUT 30)

add_test(NAME libdxfrw_ac1027_active_vport_peer COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/ac1027_active_vport_peer_tests.py)
set_tests_properties(libdxfrw_ac1027_active_vport_peer PROPERTIES LABELS "dwg;fast;AC1027;peer" TIMEOUT 30)

add_executable(libdxfrw_dwg_viewport_angle_units_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_viewport_angle_units_tests.cpp)
target_compile_features(libdxfrw_dwg_viewport_angle_units_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_viewport_angle_units_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_viewport_angle_units_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_viewport_angle_units_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_viewport_angle_units COMMAND libdxfrw_dwg_viewport_angle_units_tests)
set_tests_properties(libdxfrw_dwg_viewport_angle_units PROPERTIES LABELS "fast;dwg;viewport;transaction" TIMEOUT 30)

add_executable(libdxfrw_dxf_sectionobject_vertices_tests ${CMAKE_CURRENT_SOURCE_DIR}/dxf_sectionobject_vertices_tests.cpp)
target_compile_features(libdxfrw_dxf_sectionobject_vertices_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_sectionobject_vertices_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_sectionobject_vertices_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_sectionobject_vertices_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_sectionobject_vertices COMMAND libdxfrw_dxf_sectionobject_vertices_tests)
set_tests_properties(libdxfrw_dxf_sectionobject_vertices PROPERTIES LABELS "fast;dxf;section;geometry" TIMEOUT 30)

add_executable(libdxfrw_dxf_dictionary_late_hardowner_tests ${CMAKE_CURRENT_SOURCE_DIR}/dxf_dictionary_late_hardowner_tests.cpp)
target_compile_features(libdxfrw_dxf_dictionary_late_hardowner_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_dictionary_late_hardowner_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_dictionary_late_hardowner_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_dictionary_late_hardowner_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_dictionary_late_hardowner COMMAND libdxfrw_dxf_dictionary_late_hardowner_tests)
set_tests_properties(libdxfrw_dxf_dictionary_late_hardowner PROPERTIES LABELS "fast;dxf;dictionary" TIMEOUT 30)

add_executable(libdxfrw_dwg_ac1027_maintenance_header_tests ${CMAKE_CURRENT_SOURCE_DIR}/dwg_ac1027_maintenance_header_tests.cpp)
target_compile_features(libdxfrw_dwg_ac1027_maintenance_header_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_ac1027_maintenance_header_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_ac1027_maintenance_header_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_ac1027_maintenance_header_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_ac1027_maintenance_header COMMAND libdxfrw_dwg_ac1027_maintenance_header_tests)
set_tests_properties(libdxfrw_dwg_ac1027_maintenance_header PROPERTIES LABELS "dwg;fast;AC1027;transaction" TIMEOUT 30)

add_test(NAME libdxfrw_ac1027_maintenance_header_audit COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/ac1027_maintenance_header_audit_tests.py)
set_tests_properties(libdxfrw_ac1027_maintenance_header_audit PROPERTIES LABELS "dwg;fast;AC1027;peer" TIMEOUT 30)
add_test(NAME libdxfrw_ac1027_header_geometry_peer COMMAND ${LIBDXFRW_PYTHON_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/ac1027_header_geometry_peer_tests.py)
set_tests_properties(libdxfrw_ac1027_header_geometry_peer PROPERTIES LABELS "dwg;fast;AC1027;peer" TIMEOUT 30)

add_executable(libdxfrw_dxf_compact_material_graph_tests ${CMAKE_CURRENT_SOURCE_DIR}/dxf_compact_material_graph_tests.cpp $<TARGET_OBJECTS:libdxfrw_test_adapter>)
target_compile_features(libdxfrw_dxf_compact_material_graph_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_compact_material_graph_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_compact_material_graph_tests PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/dwg2dxf)
target_link_libraries(libdxfrw_dxf_compact_material_graph_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_compact_material_graph COMMAND libdxfrw_dxf_compact_material_graph_tests)
set_tests_properties(libdxfrw_dxf_compact_material_graph PROPERTIES LABELS "fast;dxf;material;graph;transaction" TIMEOUT 60)

add_executable(libdxfrw_dxf_dimension_oblique_tests ${CMAKE_CURRENT_SOURCE_DIR}/dxf_dimension_oblique_tests.cpp)
target_compile_features(libdxfrw_dxf_dimension_oblique_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_dimension_oblique_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_dimension_oblique_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_dimension_oblique_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_dimension_oblique COMMAND libdxfrw_dxf_dimension_oblique_tests)
set_tests_properties(libdxfrw_dxf_dimension_oblique PROPERTIES LABELS "fast;dxf;dimension;transaction" TIMEOUT 30)

add_executable(libdxfrw_dwg2dxf_dimension_angle_tests ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf_dimension_angle_tests.cpp ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_dwg2dxf_dimension_angle_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg2dxf_dimension_angle_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg2dxf_dimension_angle_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/dwg2dxf)
target_link_libraries(libdxfrw_dwg2dxf_dimension_angle_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg2dxf_dimension_angle COMMAND libdxfrw_dwg2dxf_dimension_angle_tests)
set_tests_properties(libdxfrw_dwg2dxf_dimension_angle PROPERTIES LABELS "fast;dwg;dxf;dimension;adapter;transaction" TIMEOUT 30)

add_executable(libdxfrw_dwg2dxf_light_angle_tests ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf_light_angle_tests.cpp ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_dwg2dxf_light_angle_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg2dxf_light_angle_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg2dxf_light_angle_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/dwg2dxf)
target_link_libraries(libdxfrw_dwg2dxf_light_angle_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg2dxf_light_angle COMMAND libdxfrw_dwg2dxf_light_angle_tests)
set_tests_properties(libdxfrw_dwg2dxf_light_angle PROPERTIES LABELS "fast;dwg;dxf;light;adapter;transaction" TIMEOUT 30)

add_executable(libdxfrw_dwg_ac1027_default_graph_strict_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_ac1027_default_graph_strict_tests.cpp
    $<TARGET_OBJECTS:libdxfrw_test_adapter>)
target_compile_features(libdxfrw_dwg_ac1027_default_graph_strict_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_ac1027_default_graph_strict_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_ac1027_default_graph_strict_tests PRIVATE
    ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libdxfrw_dwg_ac1027_default_graph_strict_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_ac1027_default_graph_strict COMMAND libdxfrw_dwg_ac1027_default_graph_strict_tests)
set_tests_properties(libdxfrw_dwg_ac1027_default_graph_strict PROPERTIES LABELS "dwg;fast;AC1027;graph;transaction" TIMEOUT 120)
add_executable(libdxfrw_dwg_text_alignment_elevation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_text_alignment_elevation_tests.cpp)
target_compile_features(libdxfrw_dwg_text_alignment_elevation_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_text_alignment_elevation_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_text_alignment_elevation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libdxfrw_dwg_text_alignment_elevation_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_text_alignment_elevation COMMAND libdxfrw_dwg_text_alignment_elevation_tests)
set_tests_properties(libdxfrw_dwg_text_alignment_elevation PROPERTIES LABELS "dwg;fast;text;transaction" TIMEOUT 60)
add_executable(libdxfrw_dwg_entity_table_ref_diagnostic_tests ${CMAKE_CURRENT_SOURCE_DIR}/dwg_entity_table_ref_diagnostic_tests.cpp)
target_compile_features(libdxfrw_dwg_entity_table_ref_diagnostic_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_entity_table_ref_diagnostic_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_entity_table_ref_diagnostic_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_entity_table_ref_diagnostic_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_entity_table_ref_diagnostic COMMAND libdxfrw_dwg_entity_table_ref_diagnostic_tests)
set_tests_properties(libdxfrw_dwg_entity_table_ref_diagnostic PROPERTIES LABELS "dwg;fast;diagnostics" TIMEOUT 30)

add_executable(libdxfrw_dwg_pointcloudex_cropped_fields_tests ${CMAKE_CURRENT_SOURCE_DIR}/dwg_pointcloudex_cropped_fields_tests.cpp)
target_compile_features(libdxfrw_dwg_pointcloudex_cropped_fields_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_pointcloudex_cropped_fields_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_pointcloudex_cropped_fields_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_pointcloudex_cropped_fields_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_pointcloudex_cropped_fields COMMAND libdxfrw_dwg_pointcloudex_cropped_fields_tests)
set_tests_properties(libdxfrw_dwg_pointcloudex_cropped_fields PROPERTIES LABELS "dwg;fast;pointcloud;transaction" TIMEOUT 30)

add_executable(libdxfrw_dxf_pointcloud_omitted_fields_tests ${CMAKE_CURRENT_SOURCE_DIR}/dxf_pointcloud_omitted_fields_tests.cpp)
target_compile_features(libdxfrw_dxf_pointcloud_omitted_fields_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_pointcloud_omitted_fields_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_pointcloud_omitted_fields_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_pointcloud_omitted_fields_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_pointcloud_omitted_fields COMMAND libdxfrw_dxf_pointcloud_omitted_fields_tests)
set_tests_properties(libdxfrw_dxf_pointcloud_omitted_fields PROPERTIES LABELS "dxf;fast;pointcloud;transaction" TIMEOUT 30)

add_executable(libdxfrw_dxf_spline_helix_dispatch_tests ${CMAKE_CURRENT_SOURCE_DIR}/dxf_spline_helix_dispatch_tests.cpp)
target_compile_features(libdxfrw_dxf_spline_helix_dispatch_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_spline_helix_dispatch_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_spline_helix_dispatch_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_spline_helix_dispatch_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_spline_helix_dispatch COMMAND libdxfrw_dxf_spline_helix_dispatch_tests)
set_tests_properties(libdxfrw_dxf_spline_helix_dispatch PROPERTIES LABELS "dxf;fast;spline;helix;compatibility" TIMEOUT 30)

add_executable(libdxfrw_spline_rational_weight_precision_tests ${CMAKE_CURRENT_SOURCE_DIR}/spline_rational_weight_precision_tests.cpp)
target_compile_features(libdxfrw_spline_rational_weight_precision_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_spline_rational_weight_precision_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_spline_rational_weight_precision_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_spline_rational_weight_precision_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_spline_rational_weight_precision COMMAND libdxfrw_spline_rational_weight_precision_tests)
set_tests_properties(libdxfrw_spline_rational_weight_precision PROPERTIES LABELS "fast;dxf;dwg;spline;helix;roundtrip" TIMEOUT 30)

add_executable(libdxfrw_dxf_primitive_subtype_dispatch_tests ${CMAKE_CURRENT_SOURCE_DIR}/dxf_primitive_subtype_dispatch_tests.cpp)
target_compile_features(libdxfrw_dxf_primitive_subtype_dispatch_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_primitive_subtype_dispatch_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_primitive_subtype_dispatch_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_primitive_subtype_dispatch_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_primitive_subtype_dispatch COMMAND libdxfrw_dxf_primitive_subtype_dispatch_tests)
set_tests_properties(libdxfrw_dxf_primitive_subtype_dispatch PROPERTIES LABELS "dxf;fast;entities;compatibility" TIMEOUT 30)

add_executable(libdxfrw_dwg_table_child_xrecord_ac1018_candidate_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_table_child_xrecord_ac1018_candidate_tests.cpp)
target_compile_features(libdxfrw_dwg_table_child_xrecord_ac1018_candidate_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_table_child_xrecord_ac1018_candidate_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_table_child_xrecord_ac1018_candidate_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_table_child_xrecord_ac1018_candidate_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_table_child_xrecord_ac1018_candidate COMMAND libdxfrw_dwg_table_child_xrecord_ac1018_candidate_tests)
set_tests_properties(libdxfrw_dwg_table_child_xrecord_ac1018_candidate PROPERTIES LABELS "dwg;fast;table" TIMEOUT 30)

add_executable(libdxfrw_dwg_table_entity_xdictionary_ac1018_candidate_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_table_entity_xdictionary_ac1018_candidate_tests.cpp)
target_compile_features(libdxfrw_dwg_table_entity_xdictionary_ac1018_candidate_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_table_entity_xdictionary_ac1018_candidate_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_table_entity_xdictionary_ac1018_candidate_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_table_entity_xdictionary_ac1018_candidate_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_table_entity_xdictionary_ac1018_candidate COMMAND libdxfrw_dwg_table_entity_xdictionary_ac1018_candidate_tests)
set_tests_properties(libdxfrw_dwg_table_entity_xdictionary_ac1018_candidate PROPERTIES LABELS "dwg;fast;table" TIMEOUT 30)

add_executable(libdxfrw_dwg_table_entity_reader_lease_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_table_entity_reader_lease_tests.cpp ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_dwg_table_entity_reader_lease_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_table_entity_reader_lease_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_table_entity_reader_lease_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/dwg2dxf)
target_link_libraries(libdxfrw_dwg_table_entity_reader_lease_tests PRIVATE dxfrw)
add_dependencies(libdxfrw_dwg_table_entity_reader_lease_tests libdxfrw_dwg_table_entity_xdictionary_ac1018_candidate_tests)
add_test(NAME libdxfrw_dwg_table_entity_reader_lease COMMAND ${CMAKE_COMMAND}
    -DBODY_TEST=$<TARGET_FILE:libdxfrw_dwg_table_entity_xdictionary_ac1018_candidate_tests>
    -DLEASE_TEST=$<TARGET_FILE:libdxfrw_dwg_table_entity_reader_lease_tests>
    -DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/generated-table-entity-lease
    -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg_table_entity_reader_lease.cmake)
set_tests_properties(libdxfrw_dwg_table_entity_reader_lease PROPERTIES LABELS "dwg;fast;table" TIMEOUT 30)

add_executable(libdxfrw_dwg_tablestyle_ac1018_candidate_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_tablestyle_ac1018_candidate_tests.cpp)
target_compile_features(libdxfrw_dwg_tablestyle_ac1018_candidate_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_tablestyle_ac1018_candidate_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_tablestyle_ac1018_candidate_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_tablestyle_ac1018_candidate_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_tablestyle_ac1018_candidate COMMAND libdxfrw_dwg_tablestyle_ac1018_candidate_tests)
set_tests_properties(libdxfrw_dwg_tablestyle_ac1018_candidate PROPERTIES LABELS "dwg;fast;table" TIMEOUT 30)

add_executable(libdxfrw_dwg_table_model_ac1018_candidate_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_table_model_ac1018_candidate_tests.cpp ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_dwg_table_model_ac1018_candidate_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_table_model_ac1018_candidate_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_table_model_ac1018_candidate_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/dwg2dxf)
target_link_libraries(libdxfrw_dwg_table_model_ac1018_candidate_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_table_model_ac1018_candidate COMMAND libdxfrw_dwg_table_model_ac1018_candidate_tests)
set_tests_properties(libdxfrw_dwg_table_model_ac1018_candidate PROPERTIES LABELS "dwg;fast;table" TIMEOUT 30)

add_executable(libdxfrw_dwg_tablestyle_reader_source_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_tablestyle_reader_source_tests.cpp $<TARGET_OBJECTS:libdxfrw_test_adapter>)
target_compile_features(libdxfrw_dwg_tablestyle_reader_source_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_tablestyle_reader_source_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_tablestyle_reader_source_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/dwg2dxf)
target_link_libraries(libdxfrw_dwg_tablestyle_reader_source_tests PRIVATE dxfrw)
add_dependencies(libdxfrw_dwg_tablestyle_reader_source_tests libdxfrw_dwg_tablestyle_ac1018_candidate_tests)
add_test(NAME libdxfrw_dwg_tablestyle_reader_source COMMAND ${CMAKE_COMMAND}
    -DBODY_TEST=$<TARGET_FILE:libdxfrw_dwg_tablestyle_ac1018_candidate_tests>
    -DREADER_TEST=$<TARGET_FILE:libdxfrw_dwg_tablestyle_reader_source_tests>
    -DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/tablestyle_reader_source
    -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg_tablestyle_reader_source.cmake)
set_tests_properties(libdxfrw_dwg_tablestyle_reader_source PROPERTIES LABELS "dwg;fast;table" TIMEOUT 30)

add_executable(libdxfrw_dwg_table_projection_ac1018_candidate_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_table_projection_ac1018_candidate_tests.cpp ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_dwg_table_projection_ac1018_candidate_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_table_projection_ac1018_candidate_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_table_projection_ac1018_candidate_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/dwg2dxf)
target_link_libraries(libdxfrw_dwg_table_projection_ac1018_candidate_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_table_projection_ac1018_candidate COMMAND libdxfrw_dwg_table_projection_ac1018_candidate_tests)
set_tests_properties(libdxfrw_dwg_table_projection_ac1018_candidate PROPERTIES LABELS "dwg;fast;table" TIMEOUT 30)

add_executable(libdxfrw_dwg_tablestyle_extension_dictionary_ac1018_candidate_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_tablestyle_extension_dictionary_ac1018_candidate_tests.cpp)
target_compile_features(libdxfrw_dwg_tablestyle_extension_dictionary_ac1018_candidate_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_tablestyle_extension_dictionary_ac1018_candidate_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_tablestyle_extension_dictionary_ac1018_candidate_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_tablestyle_extension_dictionary_ac1018_candidate_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_tablestyle_extension_dictionary_ac1018_candidate COMMAND libdxfrw_dwg_tablestyle_extension_dictionary_ac1018_candidate_tests)
set_tests_properties(libdxfrw_dwg_tablestyle_extension_dictionary_ac1018_candidate PROPERTIES LABELS "dwg;fast;table" TIMEOUT 30)

add_executable(libdxfrw_dwg_tablestyle_roundtrip_xrecord_ac1018_candidate_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_tablestyle_roundtrip_xrecord_ac1018_candidate_tests.cpp)
target_compile_features(libdxfrw_dwg_tablestyle_roundtrip_xrecord_ac1018_candidate_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_tablestyle_roundtrip_xrecord_ac1018_candidate_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_tablestyle_roundtrip_xrecord_ac1018_candidate_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_tablestyle_roundtrip_xrecord_ac1018_candidate_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_tablestyle_roundtrip_xrecord_ac1018_candidate COMMAND libdxfrw_dwg_tablestyle_roundtrip_xrecord_ac1018_candidate_tests)
set_tests_properties(libdxfrw_dwg_tablestyle_roundtrip_xrecord_ac1018_candidate PROPERTIES LABELS "dwg;fast;table" TIMEOUT 30)

add_executable(libdxfrw_dwg_tablestyle_xrecord_reader_binding_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_tablestyle_xrecord_reader_binding_tests.cpp ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_dwg_tablestyle_xrecord_reader_binding_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_tablestyle_xrecord_reader_binding_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_tablestyle_xrecord_reader_binding_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/dwg2dxf)
target_link_libraries(libdxfrw_dwg_tablestyle_xrecord_reader_binding_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_tablestyle_xrecord_reader_binding
    COMMAND libdxfrw_dwg_tablestyle_xrecord_reader_binding_tests ${CMAKE_CURRENT_BINARY_DIR}/generated-tablestyle-xrecord-reader-binding.dwg)
set_tests_properties(libdxfrw_dwg_tablestyle_xrecord_reader_binding PROPERTIES LABELS "dwg;fast;table;xrecord" TIMEOUT 30)

add_executable(libdxfrw_dwg_tablestyle_xrecord_full_bundle_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_tablestyle_xrecord_full_bundle_tests.cpp ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_dwg_tablestyle_xrecord_full_bundle_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_tablestyle_xrecord_full_bundle_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_tablestyle_xrecord_full_bundle_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/dwg2dxf)
target_link_libraries(libdxfrw_dwg_tablestyle_xrecord_full_bundle_tests PRIVATE dxfrw)
add_dependencies(libdxfrw_dwg_tablestyle_xrecord_full_bundle_tests
    libdxfrw_dwg_tablecontent_ac1018_candidate_tests libdxfrw_dwg_datatable_ac1018_candidate_tests
    libdxfrw_dwg_tablegeometry_ac1018_candidate_tests libdxfrw_dwg_table_roundtrip_xrecord_ac1018_candidate_tests
    libdxfrw_dwg_tablestyle_ac1018_candidate_tests libdxfrw_dwg_tablestyle_extension_dictionary_ac1018_candidate_tests
    libdxfrw_dwg_cellstylemap_ac1018_candidate_tests)
add_test(NAME libdxfrw_dwg_tablestyle_xrecord_full_bundle COMMAND ${CMAKE_COMMAND}
    -DCONTENT_TEST=$<TARGET_FILE:libdxfrw_dwg_tablecontent_ac1018_candidate_tests>
    -DDATA_TEST=$<TARGET_FILE:libdxfrw_dwg_datatable_ac1018_candidate_tests>
    -DGEOMETRY_TEST=$<TARGET_FILE:libdxfrw_dwg_tablegeometry_ac1018_candidate_tests>
    -DROOT_TEST=$<TARGET_FILE:libdxfrw_dwg_table_roundtrip_xrecord_ac1018_candidate_tests>
    -DSTYLE_TEST=$<TARGET_FILE:libdxfrw_dwg_tablestyle_ac1018_candidate_tests>
    -DDICTIONARY_TEST=$<TARGET_FILE:libdxfrw_dwg_tablestyle_extension_dictionary_ac1018_candidate_tests>
    -DMAP_TEST=$<TARGET_FILE:libdxfrw_dwg_cellstylemap_ac1018_candidate_tests>
    -DBIND_TEST=$<TARGET_FILE:libdxfrw_dwg_tablestyle_xrecord_full_bundle_tests>
    -DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/generated-tablestyle-xrecord-full-bundle
    -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg_tablestyle_xrecord_full_bundle.cmake)
set_tests_properties(libdxfrw_dwg_tablestyle_xrecord_full_bundle PROPERTIES LABELS "dwg;fast;table;xrecord" TIMEOUT 30)

add_executable(libdxfrw_dwg_tablestyle_extension_dictionary_reader_lease_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_tablestyle_extension_dictionary_reader_lease_tests.cpp ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_dwg_tablestyle_extension_dictionary_reader_lease_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_tablestyle_extension_dictionary_reader_lease_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_tablestyle_extension_dictionary_reader_lease_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/dwg2dxf)
target_link_libraries(libdxfrw_dwg_tablestyle_extension_dictionary_reader_lease_tests PRIVATE dxfrw)
add_dependencies(libdxfrw_dwg_tablestyle_extension_dictionary_reader_lease_tests libdxfrw_dwg_tablestyle_extension_dictionary_ac1018_candidate_tests)
add_test(NAME libdxfrw_dwg_tablestyle_extension_dictionary_reader_lease COMMAND ${CMAKE_COMMAND}
    -DBODY_TEST=$<TARGET_FILE:libdxfrw_dwg_tablestyle_extension_dictionary_ac1018_candidate_tests>
    -DLEASE_TEST=$<TARGET_FILE:libdxfrw_dwg_tablestyle_extension_dictionary_reader_lease_tests>
    -DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/generated-tablestyle-dictionary-lease
    -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg_tablestyle_extension_dictionary_reader_lease.cmake)
set_tests_properties(libdxfrw_dwg_tablestyle_extension_dictionary_reader_lease PROPERTIES LABELS "dwg;fast;table" TIMEOUT 30)

add_executable(libdxfrw_dwg_cellstylemap_ac1018_candidate_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_cellstylemap_ac1018_candidate_tests.cpp)
target_compile_features(libdxfrw_dwg_cellstylemap_ac1018_candidate_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_cellstylemap_ac1018_candidate_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_cellstylemap_ac1018_candidate_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_cellstylemap_ac1018_candidate_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_cellstylemap_ac1018_candidate COMMAND libdxfrw_dwg_cellstylemap_ac1018_candidate_tests)
set_tests_properties(libdxfrw_dwg_cellstylemap_ac1018_candidate PROPERTIES LABELS "dwg;fast;table" TIMEOUT 30)

add_executable(libdxfrw_dwg_cellstylemap_reader_source_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_cellstylemap_reader_source_tests.cpp $<TARGET_OBJECTS:libdxfrw_test_adapter>)
target_compile_features(libdxfrw_dwg_cellstylemap_reader_source_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_cellstylemap_reader_source_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_cellstylemap_reader_source_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/dwg2dxf)
target_link_libraries(libdxfrw_dwg_cellstylemap_reader_source_tests PRIVATE dxfrw)
add_dependencies(libdxfrw_dwg_cellstylemap_reader_source_tests libdxfrw_dwg_cellstylemap_ac1018_candidate_tests)
add_test(NAME libdxfrw_dwg_cellstylemap_reader_source COMMAND ${CMAKE_COMMAND}
    -DBODY_TEST=$<TARGET_FILE:libdxfrw_dwg_cellstylemap_ac1018_candidate_tests>
    -DREADER_TEST=$<TARGET_FILE:libdxfrw_dwg_cellstylemap_reader_source_tests>
    -DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/cellstylemap_reader_source
    -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg_cellstylemap_reader_source.cmake)
set_tests_properties(libdxfrw_dwg_cellstylemap_reader_source PROPERTIES LABELS "dwg;fast;table" TIMEOUT 30)

add_executable(libdxfrw_dwg_ltype_control_byblock_ac1018_candidate_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_ltype_control_byblock_ac1018_candidate_tests.cpp)
target_compile_features(libdxfrw_dwg_ltype_control_byblock_ac1018_candidate_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_ltype_control_byblock_ac1018_candidate_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_ltype_control_byblock_ac1018_candidate_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dwg_ltype_control_byblock_ac1018_candidate_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_ltype_control_byblock_ac1018_candidate COMMAND libdxfrw_dwg_ltype_control_byblock_ac1018_candidate_tests)
set_tests_properties(libdxfrw_dwg_ltype_control_byblock_ac1018_candidate PROPERTIES LABELS "dwg;fast;table" TIMEOUT 30)

add_executable(libdxfrw_dwg_ltype_table_ticket_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg_ltype_table_ticket_tests.cpp ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_dwg_ltype_table_ticket_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg_ltype_table_ticket_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg_ltype_table_ticket_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/dwg2dxf)
target_link_libraries(libdxfrw_dwg_ltype_table_ticket_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dwg_ltype_table_ticket COMMAND libdxfrw_dwg_ltype_table_ticket_tests)
set_tests_properties(libdxfrw_dwg_ltype_table_ticket PROPERTIES LABELS "dwg;fast;table" TIMEOUT 30)

add_executable(p5_ac1018_ltype_pair_candidate_live_probe
    ${CMAKE_SOURCE_DIR}/tools/research/p5_ac1018_ltype_pair_candidate_live_probe.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(p5_ac1018_ltype_pair_candidate_live_probe PRIVATE cxx_std_17)
target_compile_options(p5_ac1018_ltype_pair_candidate_live_probe PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(p5_ac1018_ltype_pair_candidate_live_probe PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/dwg2dxf)
target_link_libraries(p5_ac1018_ltype_pair_candidate_live_probe PRIVATE dxfrw)
add_test(NAME p5_ltype_generated_reader_ticket
    COMMAND p5_ac1018_ltype_pair_candidate_live_probe --self-test
        ${CMAKE_CURRENT_BINARY_DIR}/generated-p5-ltype-ticket.dwg)
set_tests_properties(p5_ltype_generated_reader_ticket PROPERTIES LABELS "dwg;fast;table" TIMEOUT 30)

add_executable(p5_ac1018_style_resource_source_probe
    ${CMAKE_SOURCE_DIR}/tools/research/p5_ac1018_style_resource_source_probe.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(p5_ac1018_style_resource_source_probe PRIVATE cxx_std_17)
target_compile_options(p5_ac1018_style_resource_source_probe PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(p5_ac1018_style_resource_source_probe PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/dwg2dxf)
target_link_libraries(p5_ac1018_style_resource_source_probe PRIVATE dxfrw)
add_test(NAME p5_style_resource_generated_source_binding
    COMMAND p5_ac1018_style_resource_source_probe
        ${CMAKE_CURRENT_BINARY_DIR}/generated-p5-style-resource-source.dwg)
set_tests_properties(p5_style_resource_generated_source_binding PROPERTIES LABELS "dwg;fast;table" TIMEOUT 30)

add_executable(libdxfrw_mpolygon_fill_default_tests mpolygon_fill_default_tests.cpp)
target_compile_features(libdxfrw_mpolygon_fill_default_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_mpolygon_fill_default_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_mpolygon_fill_default_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_mpolygon_fill_default_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_mpolygon_fill_default COMMAND libdxfrw_mpolygon_fill_default_tests)
set_tests_properties(libdxfrw_mpolygon_fill_default PROPERTIES LABELS "dxf;fast;mpolygon" TIMEOUT 30)

add_executable(libdxfrw_mpolygon_edge_projection_tests mpolygon_edge_projection_tests.cpp)
target_compile_features(libdxfrw_mpolygon_edge_projection_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_mpolygon_edge_projection_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_mpolygon_edge_projection_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_mpolygon_edge_projection_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_mpolygon_edge_projection COMMAND libdxfrw_mpolygon_edge_projection_tests)
set_tests_properties(libdxfrw_mpolygon_edge_projection PROPERTIES LABELS "dxf;fast;mpolygon;geometry" TIMEOUT 30)

add_executable(libdxfrw_dimassoc_duplicate_dxf_tests dimassoc_duplicate_dxf_tests.cpp)
target_compile_features(libdxfrw_dimassoc_duplicate_dxf_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dimassoc_duplicate_dxf_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dimassoc_duplicate_dxf_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dimassoc_duplicate_dxf_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dimassoc_duplicate_dxf COMMAND libdxfrw_dimassoc_duplicate_dxf_tests)
set_tests_properties(libdxfrw_dimassoc_duplicate_dxf PROPERTIES LABELS "dxf;fast;dimassoc;generated" TIMEOUT 30)

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(NAME libdxfrw_mtext_field_source_audit
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/audit_mtext_field_source_tests.py)
    set_tests_properties(libdxfrw_mtext_field_source_audit PROPERTIES
        LABELS "dxf;fast;field;research" TIMEOUT 10)
    add_test(NAME libdxfrw_ac1021_field_graph_audit
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/audit_ac1021_field_graph_tests.py)
    set_tests_properties(libdxfrw_ac1021_field_graph_audit PROPERTIES
        LABELS "dwg;fast;field;research" TIMEOUT 10)
endif()

# Opt-in research probe: the source file is obtained/generated separately and
# is deliberately not part of the repository or the default build/test set.
add_executable(libdxfrw_ac1021_mtext_field_census EXCLUDE_FROM_ALL
    ${CMAKE_SOURCE_DIR}/tools/research/ac1021_mtext_field_census.cpp
    ${_libdxfrw_test_adapter_source})
target_compile_features(libdxfrw_ac1021_mtext_field_census PRIVATE cxx_std_17)
target_compile_options(libdxfrw_ac1021_mtext_field_census PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_ac1021_mtext_field_census PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_ac1021_mtext_field_census PRIVATE dxfrw)
libdxfrw_check_test_adapter_context(libdxfrw_ac1021_mtext_field_census)

add_executable(libdxfrw_dxf_mtext_caret_tests dxf_mtext_caret_tests.cpp)
target_compile_features(libdxfrw_dxf_mtext_caret_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_mtext_caret_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_mtext_caret_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_mtext_caret_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_mtext_caret COMMAND libdxfrw_dxf_mtext_caret_tests)
set_tests_properties(libdxfrw_dxf_mtext_caret PROPERTIES LABELS "dxf;fast;mtext" TIMEOUT 30)

add_executable(libdxfrw_dxf_field_mtext_tests dxf_field_mtext_tests.cpp)
target_compile_features(libdxfrw_dxf_field_mtext_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dxf_field_mtext_tests PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dxf_field_mtext_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(libdxfrw_dxf_field_mtext_tests PRIVATE dxfrw)
add_test(NAME libdxfrw_dxf_field_mtext COMMAND libdxfrw_dxf_field_mtext_tests)
set_tests_properties(libdxfrw_dxf_field_mtext PROPERTIES LABELS "dxf;fast;field;mtext" TIMEOUT 30)
