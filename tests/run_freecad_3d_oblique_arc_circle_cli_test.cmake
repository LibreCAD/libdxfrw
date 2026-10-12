if(NOT DEFINED GENERATOR OR NOT DEFINED DWG2DXF OR NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR "GENERATOR, DWG2DXF, and OUTPUT_DIR are required")
endif()

set(_test_dir "${OUTPUT_DIR}/freecad 3d oblique arc circle control")
file(MAKE_DIRECTORY "${_test_dir}")
set(_input "${_test_dir}/ac1015 oblique arc circle control.dwg")
set(_output "${_test_dir}/ac1015 oblique arc circle dwg2dxf.dxf")
set(_roundtrip "${_test_dir}/ac1015 oblique arc circle readback.dxf")
file(REMOVE "${_input}" "${_output}" "${_roundtrip}")

# The helper uses LibreDWG's API to set extrusion vectors that its dwgadd text
# recipe cannot express. It writes only an ephemeral control in the build tree.
execute_process(
    COMMAND "${GENERATOR}" "${_input}"
    RESULT_VARIABLE _generate_result
    OUTPUT_VARIABLE _generate_stdout
    ERROR_VARIABLE _generate_stderr
    TIMEOUT 60
)
if(NOT "${_generate_result}" STREQUAL "0" OR NOT EXISTS "${_input}")
    message(FATAL_ERROR
        "Could not generate the local oblique ARC/CIRCLE control (${_generate_result})\n"
        "${_generate_stdout}\n${_generate_stderr}")
endif()
file(READ "${_input}" _magic_hex LIMIT 6 HEX)
if(NOT "${_magic_hex}" STREQUAL "414331303135")
    message(FATAL_ERROR "Control has unexpected DWG signature '${_magic_hex}'")
endif()

# FreeCAD invokes this exact executable contract: dwg2dxf INPUT -o OUTPUT.
execute_process(
    COMMAND "${DWG2DXF}" "${_input}" -o "${_output}"
    RESULT_VARIABLE _convert_result
    OUTPUT_VARIABLE _convert_stdout
    ERROR_VARIABLE _convert_stderr
    TIMEOUT 60
)
if(NOT "${_convert_result}" STREQUAL "0" OR NOT EXISTS "${_output}")
    message(FATAL_ERROR
        "FreeCAD-form oblique ARC/CIRCLE conversion failed (${_convert_result})\n"
        "${_convert_stdout}\n${_convert_stderr}")
endif()

function(assert_oblique_arc_circle_fields path)
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
        message(FATAL_ERROR "${path} has an unterminated ENTITIES section")
    endif()
    string(SUBSTRING "${_entities}" 0 ${_entities_end} _entities)

    foreach(_entity ARC CIRCLE)
        string(REGEX MATCHALL "\n${_entity}\n" _records "${_entities}")
        list(LENGTH _records _count)
        if(NOT _count EQUAL 1)
            message(FATAL_ERROR "Expected one ${_entity} in ${path}; got ${_count}")
        endif()
    endforeach()

    set(_arc_fields
        "\nAcDbCircle\n 10\n10\n 20\n20\n 30\n30\n 40\n5\n210\n0.6\n220\n0\n230\n0.8\n100\nAcDbArc\n 50\n0\n 51\n90\n")
    set(_circle_fields
        "\nAcDbCircle\n 10\n-5\n 20\n4\n 30\n10\n 40\n2.5\n210\n0.6\n220\n0\n230\n0.8\n")
    foreach(_expected IN ITEMS "${_arc_fields}" "${_circle_fields}")
        string(FIND "${_entities}" "${_expected}" _field_pos)
        if(_field_pos EQUAL -1)
            message(FATAL_ERROR
                "${path} lost oblique OCS center/normal/radius/angle fields:\n${_expected}")
        endif()
    endforeach()
endfunction()

assert_oblique_arc_circle_fields("${_output}")

execute_process(
    COMMAND "${DWG2DXF}" "${_output}" -o "${_roundtrip}"
    RESULT_VARIABLE _roundtrip_result
    OUTPUT_VARIABLE _roundtrip_stdout
    ERROR_VARIABLE _roundtrip_stderr
    TIMEOUT 60
)
if(NOT "${_roundtrip_result}" STREQUAL "0" OR NOT EXISTS "${_roundtrip}")
    message(FATAL_ERROR
        "Oblique ARC/CIRCLE DXF readback failed (${_roundtrip_result})\n"
        "${_roundtrip_stdout}\n${_roundtrip_stderr}")
endif()
assert_oblique_arc_circle_fields("${_roundtrip}")

message(STATUS
    "AC1015 oblique OCS ARC/CIRCLE fields retained by exact FreeCAD dwg2dxf argv and DXF readback")
