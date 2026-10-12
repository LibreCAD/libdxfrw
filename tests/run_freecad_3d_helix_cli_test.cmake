if(NOT DEFINED ODAFILECONVERTER OR NOT DEFINED DWG2DXF
        OR NOT DEFINED SOURCE_DXF OR NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR
        "ODAFILECONVERTER, DWG2DXF, SOURCE_DXF, and OUTPUT_DIR are required")
endif()

set(_test_dir "${OUTPUT_DIR}/freecad 3d helix local control")
set(_oda_output_dir "${_test_dir}/oda dwg")
file(MAKE_DIRECTORY "${_test_dir}" "${_oda_output_dir}")
get_filename_component(_source_name "${SOURCE_DXF}" NAME)
get_filename_component(_source_stem "${SOURCE_DXF}" NAME_WE)
get_filename_component(_source_dir "${SOURCE_DXF}" DIRECTORY)
set(_input "${_oda_output_dir}/${_source_stem}.dwg")
set(_output "${_test_dir}/${_source_stem} dwg2dxf.dxf")
set(_roundtrip "${_test_dir}/${_source_stem} readback.dxf")
file(REMOVE "${_input}" "${_output}" "${_roundtrip}")

# The control is authored locally; ODA writes only a build-tree DWG.
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
        "ODA could not create the local HELIX DWG control (${_write_result})\n"
        "${_write_stdout}\n${_write_stderr}")
endif()
file(READ "${_input}" _magic_hex LIMIT 6 HEX)
if(NOT "${_magic_hex}" STREQUAL "414331303135")
    message(FATAL_ERROR "ODA control has unexpected DWG signature '${_magic_hex}'")
endif()

function(assert_weighted_spline_fields path)
    file(STRINGS "${path}" _lines)
    list(LENGTH _lines _line_count)
    set(_index 0)
    set(_spline_count 0)
    set(_in_spline FALSE)
    set(_record "")
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
                break()
            endif()
            if(_value STREQUAL "SPLINE")
                math(EXPR _spline_count "${_spline_count} + 1")
                set(_in_spline TRUE)
            endif()
        elseif(_in_spline)
            string(APPEND _record "\n${_code}|${_value}")
        endif()
        math(EXPR _index "${_index} + 2")
    endwhile()
    if(NOT _spline_count EQUAL 1 OR NOT _in_spline)
        message(FATAL_ERROR "Expected exactly one standalone SPLINE record in ${path}")
    endif()

    string(FIND "${_record}" "\n100|AcDbSpline" _subclass_pos)
    if(_subclass_pos EQUAL -1)
        message(FATAL_ERROR "${path} lost the AcDbSpline subclass")
    endif()
    string(SUBSTRING "${_record}" ${_subclass_pos} -1 _spline_body)
    string(FIND "${_spline_body}" "\n70|4\n71|3\n72|8\n73|4\n74|0\n" _fields_pos)
    if(_fields_pos EQUAL -1)
        message(FATAL_ERROR "${path} changed weighted 3D SPLINE flags or counts")
    endif()

    string(REGEX MATCHALL "\n10\\|" _control_points "${_spline_body}")
    list(LENGTH _control_points _control_count)
    string(REGEX MATCHALL "\n41\\|" _weights "${_spline_body}")
    list(LENGTH _weights _weight_count)
    if(NOT _control_count EQUAL 4 OR NOT _weight_count EQUAL 4)
        message(FATAL_ERROR
            "${path} has ${_control_count} weighted SPLINE controls and ${_weight_count} weights; expected four each")
    endif()
    foreach(_expected
            "\n10|0\n20|0\n30|0\n41|1"
            "\n10|1\n20|0\n30|1\n41|0.5"
            "\n10|1\n20|1\n30|2\n41|0.5"
            "\n10|0\n20|1\n30|3\n41|1")
        string(FIND "${_spline_body}" "${_expected}" _point_pos)
        if(_point_pos EQUAL -1)
            message(FATAL_ERROR "${path} changed weighted SPLINE WCS control order/coordinates")
        endif()
    endforeach()
    foreach(_planar_code 210 220 230)
        string(FIND "${_spline_body}" "\n${_planar_code}|" _normal_pos)
        if(NOT _normal_pos EQUAL -1)
            message(FATAL_ERROR
                "${path} emitted planar normal group ${_planar_code} for a nonplanar SPLINE")
        endif()
    endforeach()
endfunction()

function(assert_helix_fields path)
    file(READ "${path}" _dxf)
    string(REPLACE "\r" "" _dxf "${_dxf}")
    string(FIND "${_dxf}" "\n  9\n$ACADVER\n  1\nAC1015\n" _version_pos)
    if(_version_pos EQUAL -1)
        message(FATAL_ERROR "${path} is not the expected AC1015 DXF")
    endif()

    file(STRINGS "${path}" _lines)
    list(LENGTH _lines _line_count)
    set(_index 0)
    set(_helix_count 0)
    set(_in_helix FALSE)
    set(_record "")
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
            if(_in_helix)
                break()
            endif()
            if(_value STREQUAL "HELIX")
                math(EXPR _helix_count "${_helix_count} + 1")
                set(_in_helix TRUE)
            endif()
        elseif(_in_helix)
            string(APPEND _record "\n${_code}|${_value}")
        endif()
        math(EXPR _index "${_index} + 2")
    endwhile()
    if(NOT _helix_count EQUAL 1 OR NOT _in_helix)
        message(FATAL_ERROR "Expected exactly one HELIX record in ${path}")
    endif()

    string(FIND "${_record}"
        "\n70|4\n71|3\n72|17\n73|13\n74|0\n" _spline_fields_pos)
    if(_spline_fields_pos EQUAL -1)
        message(FATAL_ERROR
            "${path} lost rational cubic HELIX spline fields or emitted a false linear/planar flag")
    endif()
    string(FIND "${_record}" "\n100|AcDbHelix" _helix_subclass_pos)
    if(_helix_subclass_pos EQUAL -1)
        message(FATAL_ERROR "${path} lost the AcDbHelix subclass")
    endif()
    string(SUBSTRING "${_record}" 0 ${_helix_subclass_pos} _spline_body)
    string(REGEX MATCHALL "\n10\\|" _control_points "${_spline_body}")
    list(LENGTH _control_points _control_count)
    string(REGEX MATCHALL "\n41\\|" _weights "${_spline_body}")
    list(LENGTH _weights _weight_count)
    if(NOT _control_count EQUAL 13 OR NOT _weight_count EQUAL 13)
        message(FATAL_ERROR
            "${path} has ${_control_count} control points and ${_weight_count} weights; expected 13 each")
    endif()
    foreach(_expected
            "\n10|1\n20|0\n30|0\n41|1"
            "\n10|1\n20|0\n30|4\n41|1")
        string(FIND "${_spline_body}" "${_expected}" _point_pos)
        if(_point_pos EQUAL -1)
            message(FATAL_ERROR "${path} lost the ordered HELIX start/end control points")
        endif()
    endforeach()
    foreach(_planar_code 210 220 230)
        string(FIND "${_spline_body}" "\n${_planar_code}|" _normal_pos)
        if(NOT _normal_pos EQUAL -1)
            message(FATAL_ERROR
                "${path} emitted planar normal group ${_planar_code} for a nonplanar HELIX")
        endif()
    endforeach()

    string(FIND "${_record}"
        "\n100|AcDbHelix\n90|1\n91|2\n10|0\n20|0\n30|0\n11|1\n21|0\n31|0\n12|0\n22|0\n32|1\n40|1\n41|1\n42|4\n290|1\n280|2"
        _metadata_pos)
    if(_metadata_pos EQUAL -1)
        message(FATAL_ERROR "${path} lost HELIX axis, radius, turn, handedness, or constraint metadata")
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
        "FreeCAD-form HELIX conversion failed (${_convert_result})\n"
        "${_convert_stdout}\n${_convert_stderr}")
endif()
assert_helix_fields("${_output}")
assert_weighted_spline_fields("${_output}")

# Readback checks exercise the public DXF parser/writer path too.
execute_process(
    COMMAND "${DWG2DXF}" "${_output}" -o "${_roundtrip}"
    RESULT_VARIABLE _readback_result
    OUTPUT_VARIABLE _readback_stdout
    ERROR_VARIABLE _readback_stderr
    TIMEOUT 60
)
if(NOT "${_readback_result}" STREQUAL "0" OR NOT EXISTS "${_roundtrip}")
    message(FATAL_ERROR
        "HELIX DXF readback failed (${_readback_result})\n"
        "${_readback_stdout}\n${_readback_stderr}")
endif()
assert_helix_fields("${_roundtrip}")
assert_weighted_spline_fields("${_roundtrip}")

message(STATUS
    "ODA DWG -> exact FreeCAD-form dwg2dxf -> DXF readback preserved weighted 3D SPLINE and HELIX fields")
