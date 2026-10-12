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

if(NOT DEFINED ODA_TARGET_VERSION)
    set(ODA_TARGET_VERSION ACAD2000)
endif()
if(NOT DEFINED EXPECTED_DWG_VERSION)
    set(EXPECTED_DWG_VERSION AC1015)
endif()
set(_supported_version_pairs
    "ACAD2000|AC1015"
    "ACAD2004|AC1018"
    "ACAD2007|AC1021"
    "ACAD2010|AC1024"
    "ACAD2013|AC1027"
    "ACAD2018|AC1032")
set(_requested_version_pair "${ODA_TARGET_VERSION}|${EXPECTED_DWG_VERSION}")
list(FIND _supported_version_pairs "${_requested_version_pair}" _pair_index)
if(_pair_index EQUAL -1)
    message(FATAL_ERROR
        "Unsupported ODA/DWG version pair: ${ODA_TARGET_VERSION}/${EXPECTED_DWG_VERSION}")
endif()
if(EXPECTED_DWG_VERSION STREQUAL "AC1015")
    set(_expected_dwg_magic_hex 414331303135)
elseif(EXPECTED_DWG_VERSION STREQUAL "AC1018")
    set(_expected_dwg_magic_hex 414331303138)
elseif(EXPECTED_DWG_VERSION STREQUAL "AC1021")
    set(_expected_dwg_magic_hex 414331303231)
elseif(EXPECTED_DWG_VERSION STREQUAL "AC1024")
    set(_expected_dwg_magic_hex 414331303234)
elseif(EXPECTED_DWG_VERSION STREQUAL "AC1027")
    set(_expected_dwg_magic_hex 414331303237)
elseif(EXPECTED_DWG_VERSION STREQUAL "AC1032")
    set(_expected_dwg_magic_hex 414331303332)
else()
    message(FATAL_ERROR "No DWG magic mapping for ${EXPECTED_DWG_VERSION}")
endif()

set(_test_dir
    "${OUTPUT_DIR}/freecad 3dface ${EXPECTED_DWG_VERSION} independent control")
set(_oda_output_dir "${_test_dir}/oda dwg")
file(MAKE_DIRECTORY "${_test_dir}" "${_oda_output_dir}")
get_filename_component(_source_name "${SOURCE_DXF}" NAME)
get_filename_component(_source_dir "${SOURCE_DXF}" DIRECTORY)
set(_input "${_oda_output_dir}/ac1015_3dface_freecad_control.dwg")
set(_output "${_test_dir}/${EXPECTED_DWG_VERSION} 3dface dwg2dxf.dxf")
set(_readback "${_test_dir}/${EXPECTED_DWG_VERSION} 3dface readback.dxf")
file(REMOVE "${_input}" "${_output}" "${_readback}")

execute_process(
    COMMAND "${ODAFILECONVERTER}" "${_source_dir}" "${_oda_output_dir}"
        "${ODA_TARGET_VERSION}" DWG 0 1 "${_source_name}"
    RESULT_VARIABLE _write_result
    OUTPUT_VARIABLE _write_stdout
    ERROR_VARIABLE _write_stderr
    TIMEOUT 60
)
if(NOT "${_write_result}" STREQUAL "0" OR NOT EXISTS "${_input}")
    message(FATAL_ERROR
        "ODA could not create the local AC1015 3DFACE control (${_write_result})\n"
        "${_write_stdout}\n${_write_stderr}")
endif()
file(READ "${_input}" _dwg_magic_hex LIMIT 6 HEX)
if(NOT "${_dwg_magic_hex}" STREQUAL "${_expected_dwg_magic_hex}")
    message(FATAL_ERROR
        "ODA control has unexpected DWG signature '${_dwg_magic_hex}', expected '${_expected_dwg_magic_hex}'")
endif()

function(assert_3dface_fields path expected_dwg_version)
    file(STRINGS "${path}" _lines)
    list(LENGTH _lines _line_count)
    set(_index 0)
    set(_in_face FALSE)
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
                AND _code STREQUAL "1" AND _value STREQUAL expected_dwg_version)
            set(_acadver_ok TRUE)
        endif()
        set(_previous_code "${_code}")
        set(_previous_value "${_value}")

        if(_code STREQUAL "0")
            if(_in_face)
                list(APPEND _records "${_record}")
                set(_record "")
                set(_in_face FALSE)
            endif()
            if(_value STREQUAL "3DFACE")
                set(_in_face TRUE)
            endif()
        elseif(_in_face)
            string(APPEND _record "\n${_code}|${_value}")
        endif()
        math(EXPR _index "${_index} + 2")
    endwhile()
    if(_in_face)
        list(APPEND _records "${_record}")
    endif()

    if(NOT _acadver_ok)
        message(FATAL_ERROR
            "${path} does not identify ${expected_dwg_version} in $ACADVER")
    endif()
    list(LENGTH _records _face_count)
    if(NOT _face_count EQUAL 1)
        message(FATAL_ERROR "Expected one 3DFACE record in ${path}; got ${_face_count}")
    endif()
    list(GET _records 0 _face)
    set(_expected_pairs
        "70|4"
        "10|0([.]0*)?" "20|0([.]0*)?" "30|0([.]0*)?"
        "11|4([.]0*)?" "21|0([.]0*)?" "31|0([.]0*)?"
        "12|4([.]0*)?" "22|3([.]0*)?" "32|4([.]0*)?"
        "13|0([.]0*)?" "23|3([.]0*)?" "33|4([.]0*)?")
    foreach(_expected IN LISTS _expected_pairs)
        string(REPLACE "|" "\\|" _pattern "${_expected}")
        string(REGEX MATCH "\n${_pattern}(\n|$)" _match "${_face}")
        if(_match STREQUAL "")
            message(FATAL_ERROR "3DFACE field '${_expected}' is missing from ${path}")
        endif()
    endforeach()
endfunction()

if(DEFINED LIBREDWG_DWGREAD AND EXISTS "${LIBREDWG_DWGREAD}")
    set(_libredwg_readback "${_test_dir}/${EXPECTED_DWG_VERSION} LibreDWG.dxf")
    file(REMOVE "${_libredwg_readback}")
    execute_process(
        COMMAND "${LIBREDWG_DWGREAD}" -O DXF -o "${_libredwg_readback}" "${_input}"
        RESULT_VARIABLE _libredwg_result
        OUTPUT_VARIABLE _libredwg_stdout
        ERROR_VARIABLE _libredwg_stderr
        TIMEOUT 60
    )
    if(NOT "${_libredwg_result}" STREQUAL "0"
            OR NOT EXISTS "${_libredwg_readback}")
        message(FATAL_ERROR
            "LibreDWG independent read of ${EXPECTED_DWG_VERSION} control failed (${_libredwg_result})\n"
            "${_libredwg_stdout}\n${_libredwg_stderr}")
    endif()
    assert_3dface_fields("${_libredwg_readback}" "${EXPECTED_DWG_VERSION}")
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
        "FreeCAD-form 3DFACE conversion failed (${_convert_result})\n"
        "${_convert_stdout}\n${_convert_stderr}")
endif()
assert_3dface_fields("${_output}" "${EXPECTED_DWG_VERSION}")

execute_process(
    COMMAND "${DWG2DXF}" "${_output}" -o "${_readback}"
    RESULT_VARIABLE _readback_result
    OUTPUT_VARIABLE _readback_stdout
    ERROR_VARIABLE _readback_stderr
    TIMEOUT 60
)
if(NOT "${_readback_result}" STREQUAL "0" OR NOT EXISTS "${_readback}")
    message(FATAL_ERROR
        "3DFACE public DXF readback failed (${_readback_result})\n"
        "${_readback_stdout}\n${_readback_stderr}")
endif()
assert_3dface_fields("${_readback}" "${EXPECTED_DWG_VERSION}")
if(DEFINED LIBREDWG_DWGREAD AND EXISTS "${LIBREDWG_DWGREAD}")
    message(STATUS
        "Independent ${EXPECTED_DWG_VERSION} 3DFACE control retained its tilted WCS corners and invisible-edge flag through LibreDWG and dwg2dxf readback")
else()
    message(STATUS
        "Independent ${EXPECTED_DWG_VERSION} 3DFACE control retained its tilted WCS corners and invisible-edge flag through dwg2dxf conversion/readback")
endif()
