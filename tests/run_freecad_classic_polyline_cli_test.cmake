if(NOT DEFINED DWGADD OR NOT DEFINED DWG2DXF OR NOT DEFINED RECIPE
        OR NOT DEFINED OUTPUT_DIR OR NOT DEFINED FAMILY)
    message(FATAL_ERROR
        "DWGADD, DWG2DXF, RECIPE, OUTPUT_DIR, and FAMILY are required")
endif()

set(_test_dir "${OUTPUT_DIR}/freecad 3d ${FAMILY} local control")
file(MAKE_DIRECTORY "${_test_dir}")
set(_input "${_test_dir}/ac1015 ${FAMILY} control.dwg")
set(_output "${_test_dir}/ac1015 ${FAMILY} control.dxf")
set(_roundtrip "${_test_dir}/ac1015 ${FAMILY} readback.dxf")
file(REMOVE "${_input}" "${_output}" "${_roundtrip}")

execute_process(
    COMMAND "${DWGADD}" --as r2000 -o "${_input}" "${RECIPE}"
    RESULT_VARIABLE _generate_result
    OUTPUT_VARIABLE _generate_stdout
    ERROR_VARIABLE _generate_stderr
    TIMEOUT 60
)
if(NOT "${_generate_result}" STREQUAL "0" OR NOT EXISTS "${_input}")
    message(FATAL_ERROR
        "Could not generate the local AC1015 ${FAMILY} control (${_generate_result})\n"
        "${_generate_stdout}\n${_generate_stderr}")
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
        "FreeCAD-form ${FAMILY} conversion failed (${_convert_result})\n"
        "${_convert_stdout}\n${_convert_stderr}")
endif()

function(assert_polyline_family path)
    file(READ "${path}" _dxf)
    string(REPLACE "\r" "" _dxf "${_dxf}")
    string(FIND "${_dxf}" "AC1015" _version_pos)
    string(FIND "${_dxf}" "\nENTITIES\n" _entities_pos)
    if(_version_pos EQUAL -1 OR _entities_pos EQUAL -1)
        message(FATAL_ERROR "${path} is not the expected AC1015 DXF")
    endif()
    string(SUBSTRING "${_dxf}" ${_entities_pos} -1 _entities)
    string(FIND "${_entities}" "\nENDSEC\n" _entities_end)
    if(_entities_end EQUAL -1)
        message(FATAL_ERROR "${path} has no terminated ENTITIES section")
    endif()
    string(SUBSTRING "${_entities}" 0 ${_entities_end} _entities)

    foreach(_record "POLYLINE" "VERTEX" "SEQEND")
        string(REGEX MATCHALL "\n${_record}\n" _records "${_entities}")
        list(LENGTH _records _record_count)
        if(_record STREQUAL "VERTEX")
            if(FAMILY STREQUAL "pface")
                set(_expected_count 5)
            elseif(FAMILY STREQUAL "pface_multiface")
                set(_expected_count 7)
            else()
                set(_expected_count 4)
            endif()
        else()
            set(_expected_count 1)
        endif()
        if(NOT _record_count EQUAL _expected_count)
            message(FATAL_ERROR
                "Expected ${_expected_count} ${_record} records in ${path}; got ${_record_count}")
        endif()
    endforeach()

    if(FAMILY STREQUAL "mesh")
        set(_coordinates
            "\n 10\n0\n 20\n0\n 30\n0\n"
            "\n 10\n2\n 20\n0\n 30\n1\n"
            "\n 10\n0\n 20\n3\n 30\n-1\n"
            "\n 10\n2\n 20\n3\n 30\n2\n")
        foreach(_expected "\n 71\n    2\n 72\n    2\n")
            string(FIND "${_entities}" "${_expected}" _expected_pos)
            if(_expected_pos EQUAL -1)
                message(FATAL_ERROR
                    "${path} lost polygon-mesh dimensions")
            endif()
        endforeach()
        string(REGEX MATCHALL "AcDbPolygonMeshVertex" _mesh_vertices "${_entities}")
        list(LENGTH _mesh_vertices _mesh_vertex_count)
        if(NOT _mesh_vertex_count EQUAL 4)
            message(FATAL_ERROR
                "Expected four polygon-mesh vertices in ${path}; got ${_mesh_vertex_count}")
        endif()
        string(FIND "${_entities}" "\n 70\n   16\n" _mesh_flag_pos)
        if(_mesh_flag_pos EQUAL -1)
            message(FATAL_ERROR "${path} lost the polygon-mesh flag")
        endif()
    elseif(FAMILY STREQUAL "pface" OR FAMILY STREQUAL "pface_multiface")
        if(FAMILY STREQUAL "pface")
            set(_face_counts "\n 71\n    4\n 72\n    1\n")
            set(_face_indices "\n 71\n    1\n 72\n    2\n 73\n    3\n 74\n    4\n")
            set(_coordinates
                "\n 10\n0\n 20\n0\n 30\n1\n"
                "\n 10\n2\n 20\n0\n 30\n2\n"
                "\n 10\n2\n 20\n3\n 30\n-1\n"
                "\n 10\n0\n 20\n3\n 30\n4\n")
        else()
            set(_face_counts "\n 71\n    5\n 72\n    2\n")
            set(_face_indices
                "\n 71\n    1\n 72\n    2\n 73\n    3\n 74\n    4\n"
                "\n 71\n    2\n 72\n    3\n 73\n    4\n 74\n    5\n")
            set(_coordinates
                "\n 10\n-2\n 20\n0\n 30\n1\n"
                "\n 10\n0\n 20\n0\n 30\n1.5\n"
                "\n 10\n1\n 20\n2\n 30\n0\n"
                "\n 10\n0\n 20\n3\n 30\n-0.5\n"
                "\n 10\n-1\n 20\n1\n 30\n2\n")
        endif()
        foreach(_expected "${_face_counts}" "AcDbPolyFaceMeshVertex")
            string(FIND "${_entities}" "${_expected}" _expected_pos)
            if(_expected_pos EQUAL -1)
                message(FATAL_ERROR
                    "${path} lost polyface subtype, declared face indices, or WCS coordinates")
            endif()
        endforeach()
        string(REGEX MATCHALL "AcDbPolyFaceMeshVertex" _pface_vertices "${_entities}")
        list(LENGTH _pface_vertices _pface_vertex_count)
        string(REGEX MATCHALL "AcDbFaceRecord" _face_records "${_entities}")
        list(LENGTH _face_records _face_record_count)
        if(FAMILY STREQUAL "pface")
            set(_expected_face_count 1)
        else()
            set(_expected_face_count 2)
        endif()
        if(NOT _pface_vertex_count EQUAL 4 AND FAMILY STREQUAL "pface")
            message(FATAL_ERROR "Expected four polyface coordinate vertices in ${path}")
        elseif(NOT _pface_vertex_count EQUAL 5 AND FAMILY STREQUAL "pface_multiface")
            message(FATAL_ERROR "Expected five polyface coordinate vertices in ${path}")
        endif()
        if(NOT _face_record_count EQUAL _expected_face_count)
            message(FATAL_ERROR
                "Expected ${_expected_face_count} face records in ${path}; got ${_face_record_count}")
        endif()
        string(FIND "${_entities}" "\n 70\n   64\n" _pface_flag_pos)
        if(_pface_flag_pos EQUAL -1)
            message(FATAL_ERROR "${path} lost the polyface flag")
        endif()
        set(_previous_face_position -1)
        foreach(_expected_face IN LISTS _face_indices)
            string(FIND "${_entities}" "${_expected_face}" _face_position)
            if(_face_position LESS_EQUAL _previous_face_position)
                message(FATAL_ERROR
                    "${path} lost or reordered polyface index records")
            endif()
            set(_previous_face_position ${_face_position})
        endforeach()
        set(_previous_coordinate_position -1)
        foreach(_expected_coordinate IN LISTS _coordinates)
            string(FIND "${_entities}" "${_expected_coordinate}" _coordinate_pos)
            if(_coordinate_pos LESS_EQUAL _previous_coordinate_position)
                message(FATAL_ERROR
                    "${path} lost or reordered polyline WCS coordinates")
            endif()
            set(_previous_coordinate_position ${_coordinate_pos})
        endforeach()
    else()
        message(FATAL_ERROR "Unsupported FAMILY '${FAMILY}'")
    endif()
endfunction()

assert_polyline_family("${_output}")

execute_process(
    COMMAND "${DWG2DXF}" "${_output}" -o "${_roundtrip}"
    RESULT_VARIABLE _roundtrip_result
    OUTPUT_VARIABLE _roundtrip_stdout
    ERROR_VARIABLE _roundtrip_stderr
    TIMEOUT 60
)
if(NOT "${_roundtrip_result}" STREQUAL "0" OR NOT EXISTS "${_roundtrip}")
    message(FATAL_ERROR
        "DXF readback of the local ${FAMILY} control failed (${_roundtrip_result})\n"
        "${_roundtrip_stdout}\n${_roundtrip_stderr}")
endif()
assert_polyline_family("${_roundtrip}")

message(STATUS
    "FreeCAD-form local ${FAMILY} control retained its DXF record semantics through readback")
