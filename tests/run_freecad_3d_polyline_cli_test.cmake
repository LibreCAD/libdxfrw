if(NOT DEFINED DWGADD OR NOT DEFINED DWG2DXF OR NOT DEFINED RECIPE
        OR NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR "DWGADD, DWG2DXF, RECIPE, and OUTPUT_DIR are required")
endif()

set(_test_dir "${OUTPUT_DIR}/freecad 3d polyline local control")
file(MAKE_DIRECTORY "${_test_dir}")
set(_input "${_test_dir}/ac1015 3d polyline control.dwg")
set(_output "${_test_dir}/ac1015 3d polyline control.dxf")
set(_roundtrip "${_test_dir}/ac1015 3d polyline readback.dxf")
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
        "Could not generate the local AC1015 3D POLYLINE control (${_generate_result})\n"
        "${_generate_stdout}\n${_generate_stderr}")
endif()

# Match Draft.importDWG's argv exactly, then read the ASCII DXF back through
# dwg2dxf to check the public parser/writer path independently of FreeCAD.
execute_process(
    COMMAND "${DWG2DXF}" "${_input}" -o "${_output}"
    RESULT_VARIABLE _convert_result
    OUTPUT_VARIABLE _convert_stdout
    ERROR_VARIABLE _convert_stderr
    TIMEOUT 60
)
if(NOT "${_convert_result}" STREQUAL "0" OR NOT EXISTS "${_output}")
    message(FATAL_ERROR
        "FreeCAD-form 3D POLYLINE conversion failed (${_convert_result})\n"
        "${_convert_stdout}\n${_convert_stderr}")
endif()

function(assert_ac1015_polyline path)
    file(READ "${path}" _dxf)
    string(REPLACE "\r" "" _dxf "${_dxf}")
    string(FIND "${_dxf}" "AC1015" _version_pos)
    if(_version_pos EQUAL -1)
        message(FATAL_ERROR "Expected source revision AC1015 in ${path}")
    endif()

    string(REGEX MATCHALL "\nPOLYLINE\n" _polylines "${_dxf}")
    string(REGEX MATCHALL "\nVERTEX\n" _vertices "${_dxf}")
    string(REGEX MATCHALL "\nSEQEND\n" _sequence_ends "${_dxf}")
    list(LENGTH _polylines _polyline_count)
    list(LENGTH _vertices _vertex_count)
    list(LENGTH _sequence_ends _seqend_count)
    if(NOT _polyline_count EQUAL 1 OR NOT _vertex_count EQUAL 3
            OR NOT _seqend_count EQUAL 1)
        message(FATAL_ERROR
            "Expected 1 POLYLINE, 3 VERTEX, 1 SEQEND in ${path}; got "
            "${_polyline_count}/${_vertex_count}/${_seqend_count}")
    endif()

    foreach(_coordinate_block
            "\n 10\n0\n 20\n0\n 30\n1\n"
            "\n 10\n2\n 20\n3\n 30\n4\n"
            "\n 10\n5\n 20\n1\n 30\n-2\n")
        string(FIND "${_dxf}" "${_coordinate_block}" _coordinate_pos)
        if(_coordinate_pos EQUAL -1)
            message(FATAL_ERROR
                "${path} does not preserve all nonzero-Z 3D POLYLINE vertices")
        endif()
    endforeach()

    string(FIND "${_dxf}" "\n 70\n    8\n" _subtype_pos)
    if(_subtype_pos EQUAL -1)
        message(FATAL_ERROR "${path} does not retain the 3D POLYLINE flag")
    endif()

    string(FIND "${_dxf}" "\n100\nAcDb3dPolyline\n" _parent_pos)
    string(FIND "${_dxf}" "\n  0\nVERTEX\n" _first_vertex_pos)
    if(_parent_pos EQUAL -1 OR _first_vertex_pos EQUAL -1
            OR _first_vertex_pos LESS _parent_pos)
        message(FATAL_ERROR "Could not isolate the WCS POLYLINE parent in ${path}")
    endif()
    math(EXPR _parent_length "${_first_vertex_pos} - ${_parent_pos}")
    string(SUBSTRING "${_dxf}" ${_parent_pos} ${_parent_length} _parent)
    foreach(_extrusion_code 210 220 230)
        string(FIND "${_parent}" "\n${_extrusion_code}\n" _extrusion_pos)
        if(NOT _extrusion_pos EQUAL -1)
            message(FATAL_ERROR
                "WCS 3D POLYLINE in ${path} must not carry extrusion code ${_extrusion_code}")
        endif()
    endforeach()
endfunction()

assert_ac1015_polyline("${_output}")

execute_process(
    COMMAND "${DWG2DXF}" "${_output}" -o "${_roundtrip}"
    RESULT_VARIABLE _roundtrip_result
    OUTPUT_VARIABLE _roundtrip_stdout
    ERROR_VARIABLE _roundtrip_stderr
    TIMEOUT 60
)
if(NOT "${_roundtrip_result}" STREQUAL "0" OR NOT EXISTS "${_roundtrip}")
    message(FATAL_ERROR
        "DXF readback of the local 3D POLYLINE failed (${_roundtrip_result})\n"
        "${_roundtrip_stdout}\n${_roundtrip_stderr}")
endif()
assert_ac1015_polyline("${_roundtrip}")

message(STATUS
    "FreeCAD-form local 3D POLYLINE control preserved three WCS vertices")
