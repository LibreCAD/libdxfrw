if(NOT DEFINED DWGADD OR NOT DEFINED DWG2DXF OR NOT DEFINED RECIPE
        OR NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR "DWGADD, DWG2DXF, RECIPE, and OUTPUT_DIR are required")
endif()

set(_test_dir "${OUTPUT_DIR}/freecad 3d arc circle local control")
file(MAKE_DIRECTORY "${_test_dir}")
set(_input "${_test_dir}/ac1015 arc circle control.dwg")
set(_output "${_test_dir}/ac1015 arc circle control.dxf")
set(_roundtrip "${_test_dir}/ac1015 arc circle readback.dxf")
file(REMOVE "${_input}" "${_output}" "${_roundtrip}")

execute_process(
    COMMAND "${DWGADD}" --as r2000 -o "${_input}" "${RECIPE}"
    RESULT_VARIABLE _generate_result
    OUTPUT_VARIABLE _generate_stdout
    ERROR_VARIABLE _generate_stderr
    TIMEOUT 60
)
if(NOT "${_generate_result}" STREQUAL "0" OR NOT EXISTS "${_input}")
    message(FATAL_ERROR
        "Could not generate the local AC1015 ARC/CIRCLE control (${_generate_result})\n"
        "${_generate_stdout}\n${_generate_stderr}")
endif()

execute_process(
    COMMAND "${DWG2DXF}" "${_input}" -o "${_output}"
    RESULT_VARIABLE _convert_result
    OUTPUT_VARIABLE _convert_stdout
    ERROR_VARIABLE _convert_stderr
    TIMEOUT 60
)
if(NOT "${_convert_result}" STREQUAL "0" OR NOT EXISTS "${_output}")
    message(FATAL_ERROR
        "FreeCAD-form ARC/CIRCLE conversion failed (${_convert_result})\n"
        "${_convert_stdout}\n${_convert_stderr}")
endif()

function(assert_arc_circle path)
    file(READ "${path}" _dxf)
    string(REPLACE "\r" "" _dxf "${_dxf}")
    string(FIND "${_dxf}" "\n  9\n$ACADVER\n  1\nAC1015\n" _version_pos)
    string(FIND "${_dxf}" "\nENTITIES\n" _entities_pos)
    if(_version_pos EQUAL -1 OR _entities_pos EQUAL -1)
        message(FATAL_ERROR "${path} is not the expected AC1015 DXF")
    endif()
    string(SUBSTRING "${_dxf}" ${_entities_pos} -1 _entities)
    string(FIND "${_entities}" "\nENDSEC\n" _entities_end)
    if(_entities_end EQUAL -1)
        message(FATAL_ERROR "${path} has no terminated ENTITIES section")
    endif()
    string(SUBSTRING "${_entities}" 0 ${_entities_end} _entities)

    foreach(_record "ARC" "CIRCLE")
        string(REGEX MATCHALL "\n${_record}\n" _records "${_entities}")
        list(LENGTH _records _record_count)
        if(NOT _record_count EQUAL 1)
            message(FATAL_ERROR "Expected one ${_record} in ${path}; got ${_record_count}")
        endif()
    endforeach()

    foreach(_expected
            "\nARC\n"
            "\n 10\n20\n 20\n30\n 30\n40\n 40\n3\n"
            "\n 50\n0\n 51\n90\n"
            "\nCIRCLE\n"
            "\n 10\n10\n 20\n20\n 30\n30\n 40\n5\n")
        string(FIND "${_entities}" "${_expected}" _expected_pos)
        if(_expected_pos EQUAL -1)
            message(FATAL_ERROR "${path} lost ARC/CIRCLE center, radius, or angles")
        endif()
    endforeach()
endfunction()

assert_arc_circle("${_output}")

execute_process(
    COMMAND "${DWG2DXF}" "${_output}" -o "${_roundtrip}"
    RESULT_VARIABLE _roundtrip_result
    OUTPUT_VARIABLE _roundtrip_stdout
    ERROR_VARIABLE _roundtrip_stderr
    TIMEOUT 60
)
if(NOT "${_roundtrip_result}" STREQUAL "0" OR NOT EXISTS "${_roundtrip}")
    message(FATAL_ERROR
        "DXF readback of local ARC/CIRCLE control failed (${_roundtrip_result})\n"
        "${_roundtrip_stdout}\n${_roundtrip_stderr}")
endif()
assert_arc_circle("${_roundtrip}")

message(STATUS
    "FreeCAD-form local elevated ARC/CIRCLE controls retained fields through readback")
