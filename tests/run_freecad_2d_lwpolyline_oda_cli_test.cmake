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

set(_test_dir "${OUTPUT_DIR}/freecad 2d lwpolyline local control")
set(_oda_output_dir "${_test_dir}/oda dwg")
file(MAKE_DIRECTORY "${_test_dir}" "${_oda_output_dir}")
get_filename_component(_source_name "${SOURCE_DXF}" NAME)
get_filename_component(_source_dir "${SOURCE_DXF}" DIRECTORY)
set(_input "${_oda_output_dir}/ac1015_lwpolyline_freecad_control.dwg")
set(_output "${_test_dir}/ac1015 lwpolyline dwg2dxf.dxf")
set(_roundtrip "${_test_dir}/ac1015 lwpolyline readback.dxf")
file(REMOVE "${_input}" "${_output}" "${_roundtrip}")

# The source DXF is locally authored. Keep ODA-generated DWG and conversion
# outputs in the build tree; do not turn these controls into binary fixtures.
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
        "ODA could not create the local AC1015 LWPOLYLINE control (${_write_result})\n"
        "${_write_stdout}\n${_write_stderr}")
endif()
file(READ "${_input}" _magic_hex LIMIT 6 HEX)
if(NOT "${_magic_hex}" STREQUAL "414331303135")
    message(FATAL_ERROR "ODA control has unexpected DWG signature '${_magic_hex}'")
endif()

function(assert_lwpolyline path)
    file(STRINGS "${path}" _lines)
    list(LENGTH _lines _line_count)
    set(_index 0)
    set(_acadver_pending FALSE)
    set(_acadver_ok FALSE)
    set(_active FALSE)
    set(_polyline_count 0)
    set(_subclass_ok FALSE)
    set(_vertex_count -1)
    set(_flags -1)
    set(_elevation "0")
    set(_pending_x "")
    set(_vertices)

    while(_index LESS _line_count)
        math(EXPR _value_index "${_index} + 1")
        if(_value_index GREATER_EQUAL _line_count)
            message(FATAL_ERROR "Malformed DXF code/value pair in ${path}")
        endif()
        list(GET _lines ${_index} _code)
        list(GET _lines ${_value_index} _value)
        string(STRIP "${_code}" _code)
        string(STRIP "${_value}" _value)

        if(_acadver_pending)
            if(_code STREQUAL "1" AND _value STREQUAL "AC1015")
                set(_acadver_ok TRUE)
            endif()
            set(_acadver_pending FALSE)
        endif()
        if(_code STREQUAL "9" AND _value STREQUAL "$ACADVER")
            set(_acadver_pending TRUE)
        endif()

        if(_code STREQUAL "0")
            set(_active FALSE)
            if(_value STREQUAL "LWPOLYLINE")
                math(EXPR _polyline_count "${_polyline_count} + 1")
                set(_active TRUE)
                set(_subclass_ok FALSE)
                set(_vertex_count -1)
                set(_flags -1)
                set(_elevation "0")
                set(_pending_x "")
                set(_vertices)
            endif()
        elseif(_active)
            if(_code STREQUAL "100" AND _value STREQUAL "AcDbPolyline")
                set(_subclass_ok TRUE)
            elseif(_code STREQUAL "90")
                set(_vertex_count "${_value}")
            elseif(_code STREQUAL "70")
                set(_flags "${_value}")
            elseif(_code STREQUAL "38")
                set(_elevation "${_value}")
            elseif(_code STREQUAL "10")
                set(_pending_x "${_value}")
            elseif(_code STREQUAL "20")
                if(_pending_x STREQUAL "")
                    message(FATAL_ERROR "LWPOLYLINE Y value has no preceding X in ${path}")
                endif()
                list(APPEND _vertices "${_pending_x}|${_value}")
                set(_pending_x "")
            endif()
        endif()

        math(EXPR _index "${_index} + 2")
    endwhile()

    if(NOT _acadver_ok)
        message(FATAL_ERROR "${path} does not declare AC1015 in $ACADVER")
    endif()
    if(NOT _polyline_count EQUAL 1 OR NOT _subclass_ok)
        message(FATAL_ERROR
            "Expected one AcDbPolyline entity in ${path}; got ${_polyline_count}")
    endif()
    if(NOT _vertex_count EQUAL 4 OR NOT _flags EQUAL 1)
        message(FATAL_ERROR
            "Expected four vertices and only the closed flag in ${path}; got count=${_vertex_count}, flags=${_flags}")
    endif()
    if(NOT "${_elevation}" MATCHES "^[+-]?0([.]0*)?$")
        message(FATAL_ERROR "Expected default-zero polyline elevation in ${path}; got ${_elevation}")
    endif()
    if(NOT _pending_x STREQUAL "")
        message(FATAL_ERROR "LWPOLYLINE has an X coordinate without a Y in ${path}")
    endif()
    list(LENGTH _vertices _actual_vertex_count)
    if(NOT _actual_vertex_count EQUAL 4)
        message(FATAL_ERROR
            "Expected four ordered LWPOLYLINE XY vertices in ${path}; got ${_vertices}")
    endif()

    set(_expected_vertices "0|0" "4|0" "4|3" "0|3")
    foreach(_vertex_index RANGE 0 3)
        list(GET _vertices ${_vertex_index} _actual_vertex)
        list(GET _expected_vertices ${_vertex_index} _expected_vertex)
        string(REPLACE "|" ";" _actual_parts "${_actual_vertex}")
        string(REPLACE "|" ";" _expected_parts "${_expected_vertex}")
        list(GET _actual_parts 0 _actual_x)
        list(GET _actual_parts 1 _actual_y)
        list(GET _expected_parts 0 _expected_x)
        list(GET _expected_parts 1 _expected_y)
        foreach(_axis IN ITEMS x y)
            if(_axis STREQUAL "x")
                set(_actual_number "${_actual_x}")
                set(_expected_number "${_expected_x}")
            else()
                set(_actual_number "${_actual_y}")
                set(_expected_number "${_expected_y}")
            endif()
            if(_expected_number STREQUAL "0")
                set(_number_pattern "^[+-]?0([.]0*)?$")
            elseif(_expected_number STREQUAL "3")
                set(_number_pattern "^3([.]0*)?$")
            else()
                set(_number_pattern "^4([.]0*)?$")
            endif()
            if("${_actual_number}" MATCHES "${_number_pattern}")
                set(_number_ok TRUE)
            else()
                set(_number_ok FALSE)
            endif()
            if(NOT _number_ok)
                message(FATAL_ERROR
                    "LWPOLYLINE vertex ${_vertex_index} ${_axis} changed in ${path}: expected ${_expected_number}, got ${_actual_number}")
            endif()
        endforeach()
    endforeach()
endfunction()

# Match the exact command shape used by FreeCAD Draft.importDWG.
execute_process(
    COMMAND "${DWG2DXF}" "${_input}" -o "${_output}"
    RESULT_VARIABLE _convert_result
    OUTPUT_VARIABLE _convert_stdout
    ERROR_VARIABLE _convert_stderr
    TIMEOUT 60
)
if(NOT "${_convert_result}" STREQUAL "0" OR NOT EXISTS "${_output}")
    message(FATAL_ERROR
        "FreeCAD-form LWPOLYLINE conversion failed (${_convert_result})\n"
        "${_convert_stdout}\n${_convert_stderr}")
endif()
assert_lwpolyline("${_output}")

execute_process(
    COMMAND "${DWG2DXF}" "${_output}" -o "${_roundtrip}"
    RESULT_VARIABLE _readback_result
    OUTPUT_VARIABLE _readback_stdout
    ERROR_VARIABLE _readback_stderr
    TIMEOUT 60
)
if(NOT "${_readback_result}" STREQUAL "0" OR NOT EXISTS "${_roundtrip}")
    message(FATAL_ERROR
        "LWPOLYLINE public DXF readback failed (${_readback_result})\n"
        "${_readback_stdout}\n${_readback_stderr}")
endif()
assert_lwpolyline("${_roundtrip}")

message(STATUS
    "AC1015 closed LWPOLYLINE preserved four ordered vertices through ODA, dwg2dxf, and DXF readback")
