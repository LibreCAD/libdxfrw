if(NOT DEFINED ODAFILECONVERTER OR NOT DEFINED DWG2DXF
        OR NOT DEFINED SOURCE_DXF OR NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR
        "ODAFILECONVERTER, DWG2DXF, SOURCE_DXF, and OUTPUT_DIR are required")
endif()

set(_test_dir "${OUTPUT_DIR}/freecad 3d insert local control")
set(_oda_output_dir "${_test_dir}/oda dwg")
file(MAKE_DIRECTORY "${_test_dir}" "${_oda_output_dir}")
get_filename_component(_source_name "${SOURCE_DXF}" NAME)
get_filename_component(_source_stem "${SOURCE_DXF}" NAME_WE)
set(_input "${_oda_output_dir}/${_source_stem}.dwg")
set(_output "${_test_dir}/${_source_stem} dwg2dxf.dxf")
set(_roundtrip "${_test_dir}/${_source_stem} readback.dxf")
file(REMOVE "${_input}" "${_output}" "${_roundtrip}")

# Generate the DWG from the locally authored DXF using ODA as an independent
# DWG writer. All generated DWG/DXF outputs stay under the build directory.
get_filename_component(_source_dir "${SOURCE_DXF}" DIRECTORY)
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
        "ODA could not create the local INSERT DWG control (${_write_result})\n"
        "${_write_stdout}\n${_write_stderr}")
endif()
file(READ "${_input}" _magic_hex LIMIT 6 HEX)
if(NOT "${_magic_hex}" STREQUAL "414331303135")
    message(FATAL_ERROR "ODA control has unexpected DWG signature '${_magic_hex}'")
endif()

# This is exactly the command shape FreeCAD's Draft DWG importer invokes.
execute_process(
    COMMAND "${DWG2DXF}" "${_input}" -o "${_output}"
    RESULT_VARIABLE _convert_result
    OUTPUT_VARIABLE _convert_stdout
    ERROR_VARIABLE _convert_stderr
    TIMEOUT 60
)
if(NOT "${_convert_result}" STREQUAL "0" OR NOT EXISTS "${_output}")
    message(FATAL_ERROR
        "FreeCAD-form INSERT conversion failed (${_convert_result})\n"
        "${_convert_stdout}\n${_convert_stderr}")
endif()

function(assert_insert_fields path)
    file(READ "${path}" _dxf)
    string(REPLACE "\r" "" _dxf "${_dxf}")
    string(FIND "${_dxf}" "\n  9\n$ACADVER\n  1\nAC1015\n" _version_pos)
    if(_version_pos EQUAL -1)
        message(FATAL_ERROR "${path} is not the expected AC1015 DXF")
    endif()

    foreach(_record "BLOCK" "LINE" "INSERT" "ENDBLK")
        string(REGEX MATCHALL "\n${_record}\n" _records "${_dxf}")
        list(LENGTH _records _count)
        if(_record STREQUAL "BLOCK" OR _record STREQUAL "ENDBLK")
            if(NOT _count EQUAL 3)
                message(FATAL_ERROR "Expected three ${_record} records in ${path}; got ${_count}")
            endif()
        elseif(NOT _count EQUAL 1)
            message(FATAL_ERROR "Expected one ${_record} record in ${path}; got ${_count}")
        endif()
    endforeach()

    foreach(_expected
            "\nAcDbLine\n 10\n1\n 20\n2\n 30\n3\n 11\n4\n 21\n6\n 31\n9\n"
            "\nAcDbBlockReference\n  2\nINSERT_CONTROL\n 10\n10\n 20\n20\n 30\n30\n 41\n2\n 42\n3\n 43\n4\n 50\n90\n")
        string(FIND "${_dxf}" "${_expected}" _field_pos)
        if(_field_pos EQUAL -1)
            message(FATAL_ERROR "${path} lost the INSERT block, XYZ, scale, or rotation fields")
        endif()
    endforeach()
endfunction()

assert_insert_fields("${_output}")

# Verify that the generated DXF remains readable and stable on the public
# converter path; the FreeCAD runtime macro separately checks the transformed
# App::Link shape against the source-derived endpoints.
execute_process(
    COMMAND "${DWG2DXF}" "${_output}" -o "${_roundtrip}"
    RESULT_VARIABLE _readback_result
    OUTPUT_VARIABLE _readback_stdout
    ERROR_VARIABLE _readback_stderr
    TIMEOUT 60
)
if(NOT "${_readback_result}" STREQUAL "0" OR NOT EXISTS "${_roundtrip}")
    message(FATAL_ERROR
        "INSERT DXF readback failed (${_readback_result})\n"
        "${_readback_stdout}\n${_readback_stderr}")
endif()
assert_insert_fields("${_roundtrip}")

message(STATUS
    "ODA DWG -> dwg2dxf preserved INSERT placement and referenced 3D LINE fields")
