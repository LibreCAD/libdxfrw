if(NOT EXISTS "${DWG2DXF}")
    message(FATAL_ERROR "dwg2dxf executable not found: ${DWG2DXF}")
endif()

if(NOT IS_DIRECTORY "${OUTPUT_DIR}")
    file(MAKE_DIRECTORY "${OUTPUT_DIR}")
endif()

string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef _token)
set(_test_dir "${OUTPUT_DIR}/dwg2dxf-raw-section-${_token}")
file(MAKE_DIRECTORY "${_test_dir}")
set(_input "${_test_dir}/opaque-acdsdata-input.dxf")
set(_output "${_test_dir}/opaque-acdsdata-output.dxf")
set(_output2 "${_test_dir}/opaque-acdsdata-output2.dxf")

# Locally author the controls in the build tree; this is not a committed sample
# or a schema-validity oracle. It verifies the CLI's standalone DXF-to-DXF
# handle identity repair, VIEWPORT typed pass-through, and keeps an unrelated
# code-320 field opaque.
file(WRITE "${_input}"
    "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1032\n0\nENDSEC\n"
    "0\nSECTION\n2\nENTITIES\n0\nLINE\n5\n30\n330\n1F\n"
    "100\nAcDbEntity\n8\n0\n100\nAcDbLine\n"
    "10\n1.0\n20\n2.0\n30\n3.0\n11\n4.0\n21\n5.0\n31\n6.0\n"
    "0\nRAY\n5\n32\n330\n1F\n100\nAcDbEntity\n8\n0\n"
    "100\nAcDbRay\n10\n1\n20\n2\n30\n3\n11\n0\n21\n0\n31\n1\n"
    "0\nXLINE\n5\n33\n330\n1F\n100\nAcDbEntity\n8\n0\n"
    "100\nAcDbXline\n10\n4\n20\n5\n30\n6\n11\n0\n21\n0.6\n31\n0.8\n"
    "0\nVIEWPORT\n5\n31\n330\n1F\n100\nAcDbEntity\n8\n0\n"
    "100\nAcDbViewport\n10\n10\n20\n20\n30\n30\n69\n7\n"
    "292\n0\n282\n2\n141\n0.25\n142\n0.75\n"
    "0\n3DSOLID\n5\nD65\n330\n1F\n100\nAcDbEntity\n8\n0\n"
    "100\nAcDbModelerGeometry\n290\n1\n"
    "2\n{1A113328-EB6D-D44D-824D-78B33668F9E7}\n"
    "100\nAcDb3dSolid\n"
    "0\nENDSEC\n0\nSECTION\n2\nACDSDATA\n"
    "0\nACDSSCHEMA\n90\n7\n1\nOpaqueSchema\n"
    "0\nACDSRECORD\n90\n8\n2\nAcDbDs::ID\n280\n10\n"
    "320\nD65\n2\nASM_Data\n280\n15\n94\n4\n310\n41434453\n"
    "0\nACDSRECORD\n90\n9\n2\nOpaqueRecord\n"
    "320\n30\n94\n4\n310\n41434453\n"
    "0\nENDSEC\n0\nEOF\n"
)

function(assert_modeler_acds_link _path _pass_name)
    file(READ "${_path}" _contents)
    string(REGEX MATCH
        "[ \t]*0\n3DSOLID\n[ \t]*5\n([0-9A-F]+)"
        _entity_match "${_contents}")
    set(_entity_handle "${CMAKE_MATCH_1}")
    string(REGEX MATCH
        "AcDbDs::ID\n[ \t]*280\n10\n[ \t]*320\n([0-9A-F]+)\n[ \t]*2\nASM_Data"
        _acds_match "${_contents}")
    set(_acds_handle "${CMAKE_MATCH_1}")
    if(_entity_match STREQUAL "" OR _acds_match STREQUAL ""
            OR NOT "${_entity_handle}" STREQUAL "${_acds_handle}")
        message(FATAL_ERROR
            "${_pass_name} did not link ACDSDATA ASM_Data key to emitted 3DSOLID handle; entity='${_entity_handle}', key='${_acds_handle}'")
    endif()
    string(REGEX MATCH
        "OpaqueRecord\n[ \t]*320\n30\n" _opaque_match "${_contents}")
    if(_opaque_match STREQUAL "")
        message(FATAL_ERROR
            "${_pass_name} globally remapped or dropped unrelated OpaqueRecord code-320 value")
    endif()
endfunction()

execute_process(
    COMMAND "${DWG2DXF}" "${_input}" -o "${_output}"
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
    TIMEOUT 10
)
if(NOT _result EQUAL 0 OR NOT EXISTS "${_output}")
    message(FATAL_ERROR
        "first conversion failed (${_result}):\n${_stdout}\n${_stderr}")
endif()

file(READ "${_output}" _contents)
foreach(_required IN ITEMS
        "AC1032" "LINE" "RAY" "XLINE" "VIEWPORT"
        "ACDSDATA" "ACDSSCHEMA" "7" "OpaqueSchema"
        "ACDSRECORD" "9" "OpaqueRecord" "320" "30"
        "94" "4" "310" "41434453")
    string(FIND "${_contents}" "${_required}" _found)
    if(_found EQUAL -1)
        message(FATAL_ERROR
            "first conversion dropped required opaque section value: ${_required}")
    endif()
endforeach()
foreach(_entity_check IN ITEMS
        "AcDbRay\n[ \t]*10\n1\n[ \t]*20\n2\n[ \t]*30\n3\n[ \t]*11\n0\n[ \t]*21\n0\n[ \t]*31\n1\n"
        "AcDbXline\n[ \t]*10\n4\n[ \t]*20\n5\n[ \t]*30\n6\n[ \t]*11\n0\n[ \t]*21\n0\\.6\n[ \t]*31\n0\\.8\n")
    string(REGEX MATCH "${_entity_check}" _entity_match "${_contents}")
    if(_entity_match STREQUAL "")
        message(FATAL_ERROR
            "first conversion lost the RAY/XLINE WCS base point or direction")
    endif()
endforeach()
string(REGEX MATCH "292\n0\n282\n[ \t]*2\n" _viewport_lighting
    "${_contents}")
if(_viewport_lighting STREQUAL "")
    message(FATAL_ERROR
        "first conversion lost the VIEWPORT boolean lighting field or following field alignment")
endif()
assert_modeler_acds_link("${_output}" "first exact-argv conversion")
string(FIND "${_contents}" " 10\n1\n" _line_start)
string(FIND "${_contents}" " 31\n6\n" _line_end)
if(_line_start EQUAL -1 OR _line_end EQUAL -1)
    message(FATAL_ERROR "first conversion changed the control LINE endpoints")
endif()

execute_process(
    COMMAND "${DWG2DXF}" "${_output}" -o "${_output2}"
    RESULT_VARIABLE _result2
    OUTPUT_VARIABLE _stdout2
    ERROR_VARIABLE _stderr2
    TIMEOUT 10
)
if(NOT _result2 EQUAL 0 OR NOT EXISTS "${_output2}")
    message(FATAL_ERROR
        "second conversion failed (${_result2}):\n${_stdout2}\n${_stderr2}")
endif()

file(READ "${_output2}" _contents2)
foreach(_required IN ITEMS
        "AC1032" "LINE" "RAY" "XLINE" "VIEWPORT"
        "ACDSDATA" "ACDSSCHEMA" "7" "OpaqueSchema"
        "ACDSRECORD" "9" "OpaqueRecord" "320" "30"
        "94" "4" "310" "41434453")
    string(FIND "${_contents2}" "${_required}" _found)
    if(_found EQUAL -1)
        message(FATAL_ERROR
            "second conversion dropped required opaque section value: ${_required}")
    endif()
endforeach()
foreach(_entity_check IN ITEMS
        "AcDbRay\n[ \t]*10\n1\n[ \t]*20\n2\n[ \t]*30\n3\n[ \t]*11\n0\n[ \t]*21\n0\n[ \t]*31\n1\n"
        "AcDbXline\n[ \t]*10\n4\n[ \t]*20\n5\n[ \t]*30\n6\n[ \t]*11\n0\n[ \t]*21\n0\\.6\n[ \t]*31\n0\\.8\n")
    string(REGEX MATCH "${_entity_check}" _entity_match "${_contents2}")
    if(_entity_match STREQUAL "")
        message(FATAL_ERROR
            "second conversion lost the RAY/XLINE WCS base point or direction")
    endif()
endforeach()
string(REGEX MATCH "292\n0\n282\n[ \t]*2\n" _viewport_lighting2
    "${_contents2}")
if(_viewport_lighting2 STREQUAL "")
    message(FATAL_ERROR
        "second conversion lost the VIEWPORT boolean lighting field or following field alignment")
endif()
assert_modeler_acds_link("${_output2}" "second exact-argv conversion")
string(FIND "${_contents2}" " 10\n1\n" _line_start2)
string(FIND "${_contents2}" " 31\n6\n" _line_end2)
if(_line_start2 EQUAL -1 OR _line_end2 EQUAL -1)
    message(FATAL_ERROR "second conversion changed the control LINE endpoints")
endif()

file(REMOVE_RECURSE "${_test_dir}")
