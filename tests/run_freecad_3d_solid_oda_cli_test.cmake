if(NOT DEFINED ODAFILECONVERTER OR NOT DEFINED DWG2DXF
        OR NOT DEFINED SOURCE_DXF OR NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR
        "ODAFILECONVERTER, DWG2DXF, SOURCE_DXF, and OUTPUT_DIR are required")
endif()
foreach(_required IN ITEMS ODAFILECONVERTER DWG2DXF SOURCE_DXF)
    if(NOT EXISTS "${${_required}}")
        message(FATAL_ERROR "${_required} does not exist: ${${_required}}")
    endif()
endforeach()

set(_test_dir "${OUTPUT_DIR}/freecad 3d solid independent control")
set(_oda_output_dir "${_test_dir}/oda dwg")
file(MAKE_DIRECTORY "${_test_dir}" "${_oda_output_dir}")
get_filename_component(_source_name "${SOURCE_DXF}" NAME)
get_filename_component(_source_dir "${SOURCE_DXF}" DIRECTORY)
set(_input "${_oda_output_dir}/ac1015_solid_freecad_control.dwg")
set(_output "${_test_dir}/ac1015 solid dwg2dxf.dxf")
set(_roundtrip "${_test_dir}/ac1015 solid readback.dxf")
file(REMOVE "${_input}" "${_output}" "${_roundtrip}")

# The source DXF is authored in this repository. ODA creates only an
# ephemeral independent-writer DWG under the build tree.
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
        "ODA could not create the local AC1015 SOLID control (${_write_result})\n"
        "${_write_stdout}\n${_write_stderr}")
endif()
file(READ "${_input}" _magic_hex LIMIT 6 HEX)
if(NOT "${_magic_hex}" STREQUAL "414331303135")
    message(FATAL_ERROR "ODA control has unexpected DWG signature '${_magic_hex}'")
endif()

function(assert_solid_fields path)
    file(STRINGS "${path}" _lines)
    list(LENGTH _lines _line_count)
    set(_index 0)
    set(_in_solid FALSE)
    set(_record "")
    set(_records)
    set(_acadver_ok FALSE)
    set(_previous_code "")
    set(_previous_value "")

    while(_index LESS _line_count)
        math(EXPR _value_index "${_index} + 1")
        if(_value_index GREATER_EQUAL _line_count)
            message(FATAL_ERROR "Malformed DXF code/value pair in ${path}")
        endif()
        list(GET _lines ${_index} _code)
        list(GET _lines ${_value_index} _value)
        string(STRIP "${_code}" _code)
        string(STRIP "${_value}" _value)

        if(_previous_code STREQUAL "9" AND _previous_value STREQUAL "$ACADVER"
                AND _code STREQUAL "1" AND _value STREQUAL "AC1015")
            set(_acadver_ok TRUE)
        endif()
        set(_previous_code "${_code}")
        set(_previous_value "${_value}")

        if(_code STREQUAL "0")
            if(_in_solid)
                list(APPEND _records "${_record}")
                set(_record "")
                set(_in_solid FALSE)
            endif()
            if(_value STREQUAL "SOLID")
                set(_in_solid TRUE)
            endif()
        elseif(_in_solid)
            string(APPEND _record "\n${_code}|${_value}")
        endif()
        math(EXPR _index "${_index} + 2")
    endwhile()
    if(_in_solid)
        list(APPEND _records "${_record}")
    endif()

    if(NOT _acadver_ok)
        message(FATAL_ERROR "${path} does not identify AC1015 in $ACADVER")
    endif()
    list(LENGTH _records _solid_count)
    if(NOT _solid_count EQUAL 1)
        message(FATAL_ERROR "Expected one SOLID record in ${path}; got ${_solid_count}")
    endif()
    list(GET _records 0 _solid)

    # Accept harmless decimal formatting differences while requiring all four
    # OCS corners, including their elevated Z coordinate, to remain intact.
    set(_expected_pairs
        "10|0([.]0*)?"
        "20|0([.]0*)?"
        "30|5([.]0*)?"
        "11|4([.]0*)?"
        "21|0([.]0*)?"
        "31|5([.]0*)?"
        "12|0([.]0*)?"
        "22|3([.]0*)?"
        "32|5([.]0*)?"
        "13|4([.]0*)?"
        "23|3([.]0*)?"
        "33|5([.]0*)?")
    foreach(_expected IN LISTS _expected_pairs)
        string(REPLACE "|" "\\|" _pattern "${_expected}")
        string(REGEX MATCH "\n${_pattern}(\n|$)" _match "${_solid}")
        if(_match STREQUAL "")
            message(FATAL_ERROR "SOLID field '${_expected}' is missing from ${path}")
        endif()
    endforeach()
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
        "FreeCAD-form SOLID conversion failed (${_convert_result})\n"
        "${_convert_stdout}\n${_convert_stderr}")
endif()
assert_solid_fields("${_output}")

execute_process(
    COMMAND "${DWG2DXF}" "${_output}" -o "${_roundtrip}"
    RESULT_VARIABLE _readback_result
    OUTPUT_VARIABLE _readback_stdout
    ERROR_VARIABLE _readback_stderr
    TIMEOUT 60
)
if(NOT "${_readback_result}" STREQUAL "0" OR NOT EXISTS "${_roundtrip}")
    message(FATAL_ERROR
        "SOLID DXF readback failed (${_readback_result})\n"
        "${_readback_stdout}\n${_readback_stderr}")
endif()
assert_solid_fields("${_roundtrip}")

message(STATUS
    "Independent AC1015 SOLID control retained four elevated corners through conversion/readback")
