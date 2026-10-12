if(NOT DEFINED ODAFILECONVERTER OR NOT DEFINED DWG2DXF
        OR NOT DEFINED SOURCE_DXF OR NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR
        "ODAFILECONVERTER, DWG2DXF, SOURCE_DXF, and OUTPUT_DIR are required")
endif()

set(_test_dir "${OUTPUT_DIR}/freecad 3d minsert local control")
set(_oda_output_dir "${_test_dir}/oda dwg")
file(MAKE_DIRECTORY "${_test_dir}" "${_oda_output_dir}")
get_filename_component(_source_name "${SOURCE_DXF}" NAME)
get_filename_component(_source_stem "${SOURCE_DXF}" NAME_WE)
get_filename_component(_source_dir "${SOURCE_DXF}" DIRECTORY)
set(_input "${_oda_output_dir}/${_source_stem}.dwg")
set(_output "${_test_dir}/${_source_stem} dwg2dxf.dxf")
set(_roundtrip "${_test_dir}/${_source_stem} readback.dxf")
file(REMOVE "${_input}" "${_output}" "${_roundtrip}")

# Generate the DWG from a locally authored DXF with ODA as an independent
# writer. Every generated DWG/DXF stays in the build tree.
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
        "ODA could not create the local MINSERT DWG control (${_write_result})\n"
        "${_write_stdout}\n${_write_stderr}")
endif()
file(READ "${_input}" _magic_hex LIMIT 6 HEX)
if(NOT "${_magic_hex}" STREQUAL "414331303135")
    message(FATAL_ERROR "ODA control has unexpected DWG signature '${_magic_hex}'")
endif()

# This is the exact noninteractive command shape used by FreeCAD's DWG importer.
execute_process(
    COMMAND "${DWG2DXF}" "${_input}" -o "${_output}"
    RESULT_VARIABLE _convert_result
    OUTPUT_VARIABLE _convert_stdout
    ERROR_VARIABLE _convert_stderr
    TIMEOUT 60
)
if(NOT "${_convert_result}" STREQUAL "0" OR NOT EXISTS "${_output}")
    message(FATAL_ERROR
        "FreeCAD-form MINSERT conversion failed (${_convert_result})\n"
        "${_convert_stdout}\n${_convert_stderr}")
endif()

function(assert_minsert_fields path)
    file(READ "${path}" _dxf)
    string(REPLACE "\r" "" _dxf "${_dxf}")
    string(FIND "${_dxf}" "\n  9\n$ACADVER\n  1\nAC1015\n" _version_pos)
    if(_version_pos EQUAL -1)
        message(FATAL_ERROR "${path} is not the expected AC1015 DXF")
    endif()

    foreach(_record "INSERT" "LINE")
        string(REGEX MATCHALL "\n${_record}\n" _records "${_dxf}")
        list(LENGTH _records _count)
        if(NOT _count EQUAL 1)
            message(FATAL_ERROR "Expected one ${_record} record in ${path}; got ${_count}")
        endif()
    endforeach()

    string(REGEX MATCHALL "\nAcDbMInsertBlock\n" _subclasses "${_dxf}")
    list(LENGTH _subclasses _subclass_count)
    if(NOT _subclass_count EQUAL 1)
        message(FATAL_ERROR "Expected one MINSERT subclass in ${path}; got ${_subclass_count}")
    endif()

    set(_expected
        "\nAcDbMInsertBlock\n  2\nARRAY_CELL\n 10\n10\n 20\n20\n 30\n30\n 41\n1\n 42\n1\n 43\n1\n 50\n0\n 70\n    2\n 71\n    2\n 44\n5\n 45\n4\n")
    string(FIND "${_dxf}" "${_expected}" _field_pos)
    if(_field_pos EQUAL -1)
        message(FATAL_ERROR
            "${path} lost MINSERT block, insertion point, scale/rotation, or 2x2 spacing fields")
    endif()
endfunction()

assert_minsert_fields("${_output}")

# Re-read and rewrite through the public DXF path. This guards the complete
# converter contract without requiring FreeCAD in normal CI.
execute_process(
    COMMAND "${DWG2DXF}" "${_output}" -o "${_roundtrip}"
    RESULT_VARIABLE _readback_result
    OUTPUT_VARIABLE _readback_stdout
    ERROR_VARIABLE _readback_stderr
    TIMEOUT 60
)
if(NOT "${_readback_result}" STREQUAL "0" OR NOT EXISTS "${_roundtrip}")
    message(FATAL_ERROR
        "MINSERT DXF readback failed (${_readback_result})\n"
        "${_readback_stdout}\n${_readback_stderr}")
endif()
assert_minsert_fields("${_roundtrip}")

message(STATUS
    "ODA DWG -> dwg2dxf -> DXF readback preserved the 2x2 MINSERT array fields")
