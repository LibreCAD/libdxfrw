if(NOT DEFINED DWGADD OR NOT DEFINED DWG2DXF OR NOT DEFINED RECIPE
        OR NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR "DWGADD, DWG2DXF, RECIPE, and OUTPUT_DIR are required")
endif()

set(_test_dir "${OUTPUT_DIR}/freecad 3d ellipse local control")
file(MAKE_DIRECTORY "${_test_dir}")
set(_input "${_test_dir}/ac1015 ellipse control.dwg")
set(_output "${_test_dir}/ac1015 ellipse dwg2dxf.dxf")
set(_roundtrip "${_test_dir}/ac1015 ellipse readback.dxf")
file(REMOVE "${_input}" "${_output}" "${_roundtrip}")

# LibreDWG writes only this locally authored AC1015 DWG in the build tree.
execute_process(
    COMMAND "${DWGADD}" --as r2000 -o "${_input}" "${RECIPE}"
    RESULT_VARIABLE _generate_result
    OUTPUT_VARIABLE _generate_stdout
    ERROR_VARIABLE _generate_stderr
    TIMEOUT 60
)
if(NOT "${_generate_result}" STREQUAL "0" OR NOT EXISTS "${_input}")
    message(FATAL_ERROR
        "Could not generate the local AC1015 ELLIPSE control (${_generate_result})\n"
        "${_generate_stdout}\n${_generate_stderr}")
endif()
file(READ "${_input}" _magic_hex LIMIT 6 HEX)
if(NOT "${_magic_hex}" STREQUAL "414331303135")
    message(FATAL_ERROR "DWGADD control has unexpected DWG signature '${_magic_hex}'")
endif()

function(assert_ellipse_fields path)
    file(READ "${path}" _dxf)
    string(REPLACE "\r" "" _dxf "${_dxf}")
    string(FIND "${_dxf}" "  9\n$ACADVER\n  1\nAC1015\n" _version_position)
    string(FIND "${_dxf}" "\nENTITIES\n" _entities_position)
    if(_version_position EQUAL -1 OR _entities_position EQUAL -1)
        message(FATAL_ERROR "${path} is not the expected AC1015 DXF")
    endif()
    string(SUBSTRING "${_dxf}" ${_entities_position} -1 _entities)
    string(FIND "${_entities}" "\nENDSEC\n" _entities_end)
    if(_entities_end EQUAL -1)
        message(FATAL_ERROR "${path} has no terminated ENTITIES section")
    endif()
    string(SUBSTRING "${_entities}" 0 ${_entities_end} _entities)

    string(REGEX MATCHALL "\nELLIPSE\n" _ellipse_records "${_entities}")
    list(LENGTH _ellipse_records _ellipse_count)
    if(NOT _ellipse_count EQUAL 2)
        message(FATAL_ERROR
            "Expected two ELLIPSE records in ${path}; got ${_ellipse_count}")
    endif()

    set(_full_ellipse
        "\n100\nAcDbEllipse\n 10\n10\n 20\n20\n 30\n0\n 11\n5\n 21\n5\n 31\n0\n 40\n0.5\n 41\n0\n 42\n6.283185307179586\n")
    set(_partial_ellipse
        "\n100\nAcDbEllipse\n 10\n1\n 20\n2\n 30\n0\n 11\n3\n 21\n3\n 31\n0\n 40\n0.5\n 41\n0.25\n 42\n1.25\n")
    string(FIND "${_entities}" "${_full_ellipse}" _full_position)
    string(FIND "${_entities}" "${_partial_ellipse}" _partial_position)
    if(_full_position EQUAL -1 OR _partial_position EQUAL -1
            OR NOT _full_position LESS _partial_position)
        message(FATAL_ERROR
            "${path} changed full/partial ELLIPSE fields, WCS axis order, or record order")
    endif()
    string(FIND "${_entities}" "\n210\n" _extrusion_position)
    if(NOT _extrusion_position EQUAL -1)
        message(FATAL_ERROR
            "${path} added a non-default extrusion group to planar ELLIPSE records")
    endif()
endfunction()

# Match the exact noninteractive converter argv used by FreeCAD Draft.
execute_process(
    COMMAND "${DWG2DXF}" "${_input}" -o "${_output}"
    RESULT_VARIABLE _convert_result
    OUTPUT_VARIABLE _convert_stdout
    ERROR_VARIABLE _convert_stderr
    TIMEOUT 60
)
if(NOT "${_convert_result}" STREQUAL "0" OR NOT EXISTS "${_output}")
    message(FATAL_ERROR
        "FreeCAD-form ELLIPSE conversion failed (${_convert_result})\n"
        "${_convert_stdout}\n${_convert_stderr}")
endif()
assert_ellipse_fields("${_output}")

# Exercise the public DXF parser/writer route with the same CLI form.
execute_process(
    COMMAND "${DWG2DXF}" "${_output}" -o "${_roundtrip}"
    RESULT_VARIABLE _readback_result
    OUTPUT_VARIABLE _readback_stdout
    ERROR_VARIABLE _readback_stderr
    TIMEOUT 60
)
if(NOT "${_readback_result}" STREQUAL "0" OR NOT EXISTS "${_roundtrip}")
    message(FATAL_ERROR
        "ELLIPSE DXF readback failed (${_readback_result})\n"
        "${_readback_stdout}\n${_readback_stderr}")
endif()
assert_ellipse_fields("${_roundtrip}")

message(STATUS
    "Local AC1015 ELLIPSE controls survived exact FreeCAD-form dwg2dxf conversion and DXF readback")
