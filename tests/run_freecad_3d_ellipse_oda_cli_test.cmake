if(NOT DEFINED ODAFILECONVERTER OR NOT DEFINED DWG2DXF
        OR NOT DEFINED SOURCE_DXF OR NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR
        "ODAFILECONVERTER, DWG2DXF, SOURCE_DXF, and OUTPUT_DIR are required")
endif()

set(_test_dir "${OUTPUT_DIR}/freecad 3d ellipse independent control")
set(_oda_output_dir "${_test_dir}/oda dwg")
file(MAKE_DIRECTORY "${_test_dir}" "${_oda_output_dir}")
get_filename_component(_source_name "${SOURCE_DXF}" NAME)
get_filename_component(_source_dir "${SOURCE_DXF}" DIRECTORY)
set(_input "${_oda_output_dir}/ac1015_ellipse_freecad_control.dwg")
set(_output "${_test_dir}/ac1015 ellipse dwg2dxf.dxf")
set(_roundtrip "${_test_dir}/ac1015 ellipse readback.dxf")
file(REMOVE "${_input}" "${_output}" "${_roundtrip}")

# The locally-authored DXF is the independent semantic source; ODA writes only
# an ephemeral DWG under the selected build/temp output directory.
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
        "ODA could not create the local ELLIPSE DWG control (${_write_result})\n"
        "${_write_stdout}\n${_write_stderr}")
endif()
file(READ "${_input}" _magic_hex LIMIT 6 HEX)
if(NOT "${_magic_hex}" STREQUAL "414331303135")
    message(FATAL_ERROR "ODA control has unexpected DWG signature '${_magic_hex}'")
endif()

function(collect_ellipse_records path output_var)
    file(STRINGS "${path}" _lines)
    list(LENGTH _lines _line_count)
    set(_index 0)
    set(_in_ellipse FALSE)
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
            if(_in_ellipse)
                list(APPEND _records "${_record}")
                set(_record "")
                set(_in_ellipse FALSE)
            endif()
            if(_value STREQUAL "ELLIPSE")
                set(_in_ellipse TRUE)
            endif()
        elseif(_in_ellipse)
            string(APPEND _record "\n${_code}|${_value}")
        endif()
        math(EXPR _index "${_index} + 2")
    endwhile()
    if(_in_ellipse)
        list(APPEND _records "${_record}")
    endif()

    list(LENGTH _records _ellipse_count)
    if(NOT _ellipse_count EQUAL 3)
        message(FATAL_ERROR "Expected three ELLIPSE records in ${path}; got ${_ellipse_count}")
    endif()
    set(${output_var} "${_records}" PARENT_SCOPE)
endfunction()

function(select_ellipse_record records center_x center_y center_z output_var path)
    set(_anchor "\n10|${center_x}\n20|${center_y}\n30|${center_z}")
    set(_selected "")
    foreach(_record IN LISTS records)
        string(FIND "${_record}" "${_anchor}" _position)
        if(NOT _position EQUAL -1)
            if(NOT "${_selected}" STREQUAL "")
                message(FATAL_ERROR "Duplicate ELLIPSE center in ${path}: ${_anchor}")
            endif()
            set(_selected "${_record}")
        endif()
    endforeach()
    if("${_selected}" STREQUAL "")
        message(FATAL_ERROR "Missing ELLIPSE center in ${path}: ${_anchor}")
    endif()
    set(${output_var} "${_selected}" PARENT_SCOPE)
endfunction()

function(assert_fields record fields path label)
    foreach(_field IN LISTS fields)
        string(FIND "${record}" "${_field}" _position)
        if(_position EQUAL -1)
            message(FATAL_ERROR "${label} ELLIPSE field '${_field}' missing from ${path}")
        endif()
    endforeach()
endfunction()

function(assert_default_normal record path label)
    foreach(_code 210 220 230)
        string(FIND "${record}\n" "\n${_code}|" _position)
        if(NOT _position EQUAL -1)
            message(FATAL_ERROR "${label} default-normal ELLIPSE has explicit ${_code} in ${path}")
        endif()
    endforeach()
endfunction()

function(assert_acad_version path)
    file(STRINGS "${path}" _lines)
    list(LENGTH _lines _line_count)
    set(_index 0)
    set(_version_ok FALSE)
    while(_index LESS _line_count)
        math(EXPR _value_index "${_index} + 1")
        if(_value_index GREATER_EQUAL _line_count)
            break()
        endif()
        list(GET _lines ${_index} _code)
        list(GET _lines ${_value_index} _value)
        string(STRIP "${_code}" _code)
        string(STRIP "${_value}" _value)
        if(_code STREQUAL "9" AND _value STREQUAL "$ACADVER")
            math(EXPR _version_code_index "${_index} + 2")
            math(EXPR _version_value_index "${_index} + 3")
            if(_version_value_index LESS _line_count)
                list(GET _lines ${_version_code_index} _version_code)
                list(GET _lines ${_version_value_index} _version_value)
                string(STRIP "${_version_code}" _version_code)
                string(STRIP "${_version_value}" _version_value)
                if(_version_code STREQUAL "1" AND _version_value STREQUAL "AC1015")
                    set(_version_ok TRUE)
                    break()
                endif()
            endif()
        endif()
        math(EXPR _index "${_index} + 2")
    endwhile()
    if(NOT _version_ok)
        message(FATAL_ERROR "${path} does not identify AC1015 in $ACADVER")
    endif()
endfunction()

function(assert_ellipse_fields path)
    assert_acad_version("${path}")
    collect_ellipse_records("${path}" _records)
    select_ellipse_record("${_records}" 10 20 0 _full "${path}")
    select_ellipse_record("${_records}" 1 2 30 _elevated "${path}")
    select_ellipse_record("${_records}" -10 5 2 _tilted "${path}")

    set(_full_fields
        "\n10|10\n20|20\n30|0"
        "\n11|5\n21|0\n31|0"
        "\n40|0.5\n41|0\n42|6.283185307179586")
    set(_elevated_fields
        "\n10|1\n20|2\n30|30"
        "\n11|3\n21|0\n31|0"
        "\n40|0.5\n41|0.25\n42|1.25")
    set(_tilted_fields
        "\n10|-10\n20|5\n30|2"
        "\n11|2\n21|0\n31|-1.5"
        "\n40|0.5\n41|0.25\n42|1.75"
        "\n210|0.6\n220|0\n230|0.8")
    assert_fields("${_full}" "${_full_fields}" "${path}" "full")
    assert_fields("${_elevated}" "${_elevated_fields}" "${path}" "elevated-partial")
    assert_fields("${_tilted}" "${_tilted_fields}" "${path}" "tilted-partial")
    assert_default_normal("${_full}" "${path}" "full")
    assert_default_normal("${_elevated}" "${path}" "elevated-partial")
endfunction()

# Match FreeCAD's exact `dwg2dxf input -o output` invocation.
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
    "Independent AC1015 ELLIPSE control retained 3D vectors, parameters, and normal through conversion/readback")
