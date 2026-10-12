if(NOT DEFINED DWGADD OR NOT DEFINED DWG2DXF OR NOT DEFINED RECIPE
        OR NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR "DWGADD, DWG2DXF, RECIPE, and OUTPUT_DIR are required")
endif()

set(_test_dir "${OUTPUT_DIR}/freecad 3d line local control")
file(MAKE_DIRECTORY "${_test_dir}")
set(_input "${_test_dir}/ac1015 3d line control.dwg")
set(_output "${_test_dir}/ac1015 3d line control.dxf")
set(_roundtrip "${_test_dir}/ac1015 3d line readback.dxf")
file(REMOVE "${_input}" "${_output}" "${_roundtrip}")

# This recipe is locally authored and generates a fresh test input. It is not
# an AutoCAD/ODA interoperability witness; it gives the end-to-end consumer
# test an exact, reproducible nonzero-Z LINE expectation without committing a
# generated DWG or DXF.
execute_process(
    COMMAND "${DWGADD}" --as r2000 -o "${_input}" "${RECIPE}"
    RESULT_VARIABLE _generate_result
    OUTPUT_VARIABLE _generate_stdout
    ERROR_VARIABLE _generate_stderr
    TIMEOUT 60
)
if(NOT "${_generate_result}" STREQUAL "0" OR NOT EXISTS "${_input}")
    message(FATAL_ERROR
        "Could not generate the local AC1015 3D LINE control (${_generate_result})\n"
        "${_generate_stdout}\n${_generate_stderr}")
endif()

# This is exactly the argv form used by FreeCAD's Draft DWG importer.
execute_process(
    COMMAND "${DWG2DXF}" "${_input}" -o "${_output}"
    RESULT_VARIABLE _convert_result
    OUTPUT_VARIABLE _convert_stdout
    ERROR_VARIABLE _convert_stderr
    TIMEOUT 60
)
if(NOT "${_convert_result}" STREQUAL "0" OR NOT EXISTS "${_output}")
    message(FATAL_ERROR
        "FreeCAD-form conversion of the local 3D LINE failed (${_convert_result})\n"
        "${_convert_stdout}\n${_convert_stderr}")
endif()

function(assert_ac1015_line path)
    file(READ "${path}" _dxf)
    string(REPLACE "\r" "" _dxf "${_dxf}")
    string(FIND "${_dxf}" "AC1015" _version_pos)
    if(_version_pos EQUAL -1)
        message(FATAL_ERROR "Expected source revision AC1015 in ${path}")
    endif()

    string(REGEX MATCHALL "\nLINE\n" _line_records "${_dxf}")
    list(LENGTH _line_records _line_count)
    if(NOT _line_count EQUAL 1)
        message(FATAL_ERROR "Expected exactly one LINE in ${path}; found ${_line_count}")
    endif()

    set(_expected_vector "\n 10\n1\n 20\n2\n 30\n3\n 11\n4\n 21\n6\n 31\n9\n")
    string(FIND "${_dxf}" "${_expected_vector}" _vector_pos)
    if(_vector_pos EQUAL -1)
        message(FATAL_ERROR
            "${path} does not preserve LINE endpoints (1,2,3) and (4,6,9)")
    endif()
endfunction()

assert_ac1015_line("${_output}")

# Read the emitted DXF back through the same public converter path. This is a
# fast internal consistency check; the opt-in FreeCAD macro below separately
# verifies the downstream B-rep geometry.
execute_process(
    COMMAND "${DWG2DXF}" "${_output}" -o "${_roundtrip}"
    RESULT_VARIABLE _roundtrip_result
    OUTPUT_VARIABLE _roundtrip_stdout
    ERROR_VARIABLE _roundtrip_stderr
    TIMEOUT 60
)
if(NOT "${_roundtrip_result}" STREQUAL "0" OR NOT EXISTS "${_roundtrip}")
    message(FATAL_ERROR
        "DXF readback of the local 3D LINE failed (${_roundtrip_result})\n"
        "${_roundtrip_stdout}\n${_roundtrip_stderr}")
endif()
assert_ac1015_line("${_roundtrip}")

message(STATUS
    "FreeCAD-form local 3D LINE control preserved AC1015 endpoints (1,2,3)-(4,6,9)")
