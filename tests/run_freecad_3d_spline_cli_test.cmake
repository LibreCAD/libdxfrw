if(NOT DEFINED ODAFILECONVERTER OR NOT DEFINED DWG2DXF
        OR NOT DEFINED SOURCE_DXF OR NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR
        "ODAFILECONVERTER, DWG2DXF, SOURCE_DXF, and OUTPUT_DIR are required")
endif()

set(_test_dir "${OUTPUT_DIR}/freecad 3d spline local control")
set(_oda_output_dir "${_test_dir}/oda dwg")
file(MAKE_DIRECTORY "${_test_dir}" "${_oda_output_dir}")
get_filename_component(_source_name "${SOURCE_DXF}" NAME)
get_filename_component(_source_stem "${SOURCE_DXF}" NAME_WE)
get_filename_component(_source_dir "${SOURCE_DXF}" DIRECTORY)
set(_input "${_oda_output_dir}/${_source_stem}.dwg")
set(_output "${_test_dir}/${_source_stem} dwg2dxf.dxf")
set(_roundtrip "${_test_dir}/${_source_stem} readback.dxf")
file(REMOVE "${_input}" "${_output}" "${_roundtrip}")

# The tracked source is authored locally; ODA writes only a build-tree DWG.
execute_process(
    COMMAND "${ODAFILECONVERTER}" "${_source_dir}" "${_oda_output_dir}"
        ACAD2000 DWG 0 1 "${_source_name}"
    RESULT_VARIABLE _write_result
    OUTPUT_VARIABLE _write_stdout
    ERROR_VARIABLE _write_stderr
    TIMEOUT 60
)
if(NOT "${_write_result}" STREQUAL "0" OR NOT EXISTS "${_input}")
    message(FATAL_ERROR
        "ODA could not create the local SPLINE DWG control (${_write_result})\n"
        "${_write_stdout}\n${_write_stderr}")
endif()
file(READ "${_input}" _magic_hex LIMIT 6 HEX)
if(NOT "${_magic_hex}" STREQUAL "414331303135")
    message(FATAL_ERROR "ODA control has unexpected DWG signature '${_magic_hex}'")
endif()

function(assert_spline_fields path)
    file(STRINGS "${path}" _lines)
    list(LENGTH _lines _line_count)
    set(_index 0)
    set(_in_spline FALSE)
    set(_record "")
    set(_records)
    while(_index LESS _line_count)
        math(EXPR _value_index "${_index} + 1")
        if(_value_index GREATER_EQUAL _line_count)
            message(FATAL_ERROR "Malformed DXF code/value pair in ${path}")
        endif()
        list(GET _lines ${_index} _code)
        list(GET _lines ${_value_index} _value)
        string(STRIP "${_code}" _code)
        string(STRIP "${_value}" _value)

        if(_code STREQUAL "0")
            if(_in_spline)
                list(APPEND _records "${_record}")
                set(_record "")
                set(_in_spline FALSE)
            endif()
            if(_value STREQUAL "SPLINE")
                set(_in_spline TRUE)
                set(_record "\n0|SPLINE\n")
            endif()
        elseif(_in_spline)
            string(APPEND _record "${_code}|${_value}\n")
        endif()
        math(EXPR _index "${_index} + 2")
    endwhile()
    if(_in_spline)
        list(APPEND _records "${_record}")
    endif()

    list(LENGTH _records _spline_count)
    if(NOT _spline_count EQUAL 4)
        message(FATAL_ERROR "Expected four SPLINE records in ${path}; got ${_spline_count}")
    endif()

    list(GET _records 0 _control_nonplanar)
    list(GET _records 1 _control_planar)
    list(GET _records 2 _fit_nonplanar)
    list(GET _records 3 _control_near_planar)

    foreach(_record_and_fields IN ITEMS
            "${_control_nonplanar}|||\n70|0\n71|3\n72|8\n73|4\n74|0\n"
            "${_control_planar}|||\n210|0.4082482904638631\n220|0.8164965809277261\n230|-0.408248290463863\n70|8\n71|3\n72|8\n73|4\n74|0\n"
            "${_fit_nonplanar}|||\n70|0\n71|3\n72|0\n73|0\n74|4\n")
        string(FIND "${_record_and_fields}" "|||" _separator)
        string(SUBSTRING "${_record_and_fields}" 0 ${_separator} _record)
        math(EXPR _expected_start "${_separator} + 3")
        string(SUBSTRING "${_record_and_fields}" ${_expected_start} -1 _expected)
        string(FIND "${_record}" "${_expected}" _fields_pos)
        if(_fields_pos EQUAL -1)
            message(FATAL_ERROR "SPLINE scenario flags/counts/normal mismatch in ${path}")
        endif()
    endforeach()

    string(FIND "${_control_near_planar}"
        "\n70|0\n71|3\n72|8\n73|4\n74|0\n42|1e-07\n"
        _near_planar_fields_pos)
    if(_near_planar_fields_pos EQUAL -1)
        message(FATAL_ERROR
            "Near-planar SPLINE within control tolerance was incorrectly classified in ${path}")
    endif()

    foreach(_expected
            "\n10|1\n20|2\n30|3\n"
            "\n10|4\n20|6\n30|9\n"
            "\n10|7\n20|2\n30|15\n"
            "\n10|10\n20|8\n30|22\n")
        string(FIND "${_control_nonplanar}" "${_expected}" _point_pos)
        if(_point_pos EQUAL -1)
            message(FATAL_ERROR "Nonplanar control SPLINE lost a WCS control point in ${path}")
        endif()
    endforeach()
    foreach(_expected
            "\n10|1\n20|0\n30|1\n"
            "\n10|0\n20|1\n30|2\n"
            "\n10|2\n20|1\n30|4\n"
            "\n10|3\n20|0\n30|3\n")
        string(FIND "${_control_planar}" "${_expected}" _point_pos)
        if(_point_pos EQUAL -1)
            message(FATAL_ERROR "Planar control SPLINE lost a WCS control point in ${path}")
        endif()
    endforeach()
    foreach(_expected
            "\n11|0\n21|1\n31|2\n"
            "\n11|1\n21|2\n31|5\n"
            "\n11|2\n21|4\n31|6\n"
            "\n11|3\n21|6\n31|10\n"
            "\n12|1\n22|1\n32|3\n"
            "\n13|1\n23|2\n33|4\n")
        string(FIND "${_fit_nonplanar}" "${_expected}" _point_pos)
        if(_point_pos EQUAL -1)
            message(FATAL_ERROR "Nonplanar fit SPLINE lost a point or tangent in ${path}")
        endif()
    endforeach()

    foreach(_expected
            "\n10|0\n20|0\n30|0\n"
            "\n10|1\n20|0\n30|0\n"
            "\n10|1\n20|1\n30|2\n"
            "\n10|0\n20|1\n30|2.00000001\n")
        string(FIND "${_control_near_planar}" "${_expected}" _point_pos)
        if(_point_pos EQUAL -1)
            message(FATAL_ERROR
                "Near-planar control SPLINE lost its small nonplanar deviation in ${path}")
        endif()
    endforeach()

    foreach(_planar_only_record IN ITEMS "${_control_nonplanar}" "${_fit_nonplanar}" "${_control_near_planar}")
        foreach(_normal_code 210 220 230)
            string(FIND "${_planar_only_record}\n" "\n${_normal_code}|" _normal_pos)
            if(NOT _normal_pos EQUAL -1)
                message(FATAL_ERROR "Nonplanar SPLINE incorrectly publishes a normal in ${path}")
            endif()
        endforeach()
    endforeach()
endfunction()

# Match the exact argv FreeCAD's Draft DWG importer uses. Outputs stay in build/.
execute_process(
    COMMAND "${DWG2DXF}" "${_input}" -o "${_output}"
    RESULT_VARIABLE _convert_result
    OUTPUT_VARIABLE _convert_stdout
    ERROR_VARIABLE _convert_stderr
    TIMEOUT 60
)
if(NOT "${_convert_result}" STREQUAL "0" OR NOT EXISTS "${_output}")
    message(FATAL_ERROR
        "FreeCAD-form SPLINE conversion failed (${_convert_result})\n"
        "${_convert_stdout}\n${_convert_stderr}")
endif()
assert_spline_fields("${_output}")

execute_process(
    COMMAND "${DWG2DXF}" "${_output}" -o "${_roundtrip}"
    RESULT_VARIABLE _readback_result
    OUTPUT_VARIABLE _readback_stdout
    ERROR_VARIABLE _readback_stderr
    TIMEOUT 60
)
if(NOT "${_readback_result}" STREQUAL "0" OR NOT EXISTS "${_roundtrip}")
    message(FATAL_ERROR
        "SPLINE DXF readback failed (${_readback_result})\n"
        "${_readback_stdout}\n${_readback_stderr}")
endif()
assert_spline_fields("${_roundtrip}")

message(STATUS
    "FreeCAD-form local SPLINE control retained planar flags/normals and all 3D geometry")
