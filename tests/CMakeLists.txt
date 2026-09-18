# DWG read tests for libdxfrw.
#
# Tests invoke the dwg2dxf binary against sample .dwg files and verify
# the produced .dxf is non-empty and structurally well-formed.
#
# Sample files live in tests/samples/<AC_VERSION>/. A test is registered
# for every .dwg under each version directory at configure time. Drop a
# new .dwg in to register a new test on next CMake reconfigure.
#
# Each registered test cross-converts to multiple output DXF versions
# (R12 / v2000 / v2010) so the writer side gets exercised too. The harness
# verifies:
#   1. The sample's first 6 bytes literally match its declared AC version.
#   2. dwg2dxf returns 0 and produces a non-empty DXF.
#   3. The DXF contains required structural markers (SECTION / ENDSEC / EOF).
#   4. The DXF contains at least one entity OR table block.
#
# Test labels: "dwg_read", the AC version (AC1021 / AC1024 / ...), and
# the output version (out_R12 / out_v2000 / out_v2010).
#
# Run:
#   cd build && ctest -L dwg_read --output-on-failure
#   cd build && ctest -L AC1024 --output-on-failure
#   cd build && ctest -L out_v2010 --output-on-failure

if(NOT TARGET dwg2dxf)
    message(STATUS "tests/: dwg2dxf target not built; skipping DWG read tests")
    return()
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

# DXF splinepoints round-trip regression test.
# Verifies Band BC: code 44 (tolfit) + fit points (11/21/31) survive a
# DWG -> DXF -> DXF round-trip. Picks a known-spline-bearing sample.
set(_dxf_roundtrip_spline_sample
    ${CMAKE_CURRENT_SOURCE_DIR}/samples/AC1024/visualization_-_conference_room.dwg
)
if(EXISTS "${_dxf_roundtrip_spline_sample}")
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

set(_dwg_test_versions AC1021 AC1024 AC1027 AC1032)
set(_dwg_test_outputs R12 v2000 v2010)

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
    list(LENGTH _dwg_test_outputs _outs)
    math(EXPR _total "${_n} * ${_outs}")
    message(STATUS "tests/: registering ${_n} ${_ver} sample(s) × ${_outs} output version(s) = ${_total} test(s)")
    foreach(_sample IN LISTS _samples)
        get_filename_component(_name "${_sample}" NAME_WE)
        foreach(_out IN LISTS _dwg_test_outputs)
            add_test(
                NAME dwg_read_${_ver}_${_name}_${_out}
                COMMAND ${CMAKE_COMMAND}
                    -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
                    -DSAMPLE=${_sample}
                    -DEXPECTED_VERSION=${_ver}
                    -DOUTPUT_VERSION=${_out}
                    -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
                    -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg_read_test.cmake
            )
            set_tests_properties(dwg_read_${_ver}_${_name}_${_out} PROPERTIES
                LABELS "dwg_read;${_ver};out_${_out}"
            )
        endforeach()
    endforeach()
endforeach()
