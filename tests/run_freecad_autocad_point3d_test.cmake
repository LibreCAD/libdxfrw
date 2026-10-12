foreach(_required IN ITEMS FREECADCMD DWG2DXF BUILD_DWG2DXF SAMPLE
        EXPECTED_SHA256 IMPORT_CHECK_MACRO)
    if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
        message(FATAL_ERROR "${_required} is required")
    endif()
endforeach()
foreach(_path IN ITEMS FREECADCMD DWG2DXF BUILD_DWG2DXF SAMPLE IMPORT_CHECK_MACRO)
    if(NOT EXISTS "${${_path}}")
        message(FATAL_ERROR "${_path} does not exist: ${${_path}}")
    endif()
endforeach()

file(SHA256 "${SAMPLE}" _sample_sha256)
if(NOT _sample_sha256 STREQUAL EXPECTED_SHA256)
    message(FATAL_ERROR
        "Unexpected AutoCAD POINT sample SHA-256: ${_sample_sha256}")
endif()
file(READ "${SAMPLE}" _sample_magic LIMIT 6 HEX)
if(NOT _sample_magic STREQUAL "414331303237")
    message(FATAL_ERROR
        "The AutoCAD POINT sample must be AC1027; got ${_sample_magic}")
endif()

get_filename_component(_installed_converter "${DWG2DXF}" REALPATH)
get_filename_component(_build_converter "${BUILD_DWG2DXF}" REALPATH)
if(_installed_converter STREQUAL _build_converter)
    message(FATAL_ERROR "FreeCAD must invoke the installed dwg2dxf artifact")
endif()
file(SHA256 "${_installed_converter}" _installed_sha256)
file(SHA256 "${_build_converter}" _build_sha256)
if(NOT _installed_sha256 STREQUAL _build_sha256)
    message(FATAL_ERROR
        "Installed dwg2dxf differs from this build: "
        "${_installed_sha256} != ${_build_sha256}")
endif()

if(DEFINED ENV{TMPDIR} AND IS_DIRECTORY "$ENV{TMPDIR}")
    set(_system_temp_base "$ENV{TMPDIR}")
elseif(DEFINED ENV{TEMP} AND IS_DIRECTORY "$ENV{TEMP}")
    set(_system_temp_base "$ENV{TEMP}")
else()
    message(FATAL_ERROR "TMPDIR or TEMP must identify the system temp directory")
endif()
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef _run_id)
set(_temp_parent "${_system_temp_base}/libdxfrw-autocad-point3d-${_run_id} with spaces")
set(_test_root "${_temp_parent}/runtime")
set(_input_dir "${_test_root}/input with spaces")
set(_input "${_input_dir}/point3d_2013.dwg")
set(_output "${_test_root}/dwg2dxf output.dxf")
set(_roundtrip "${_test_root}/dwg2dxf readback.dxf")
set(_profile "${_test_root}/freecad profile")
set(_user_cfg "${_profile}/user.cfg")
set(_user_home "${_profile}/home")
set(_user_data "${_profile}/data")
set(_user_temp "${_profile}/temp")
file(MAKE_DIRECTORY "${_input_dir}" "${_profile}" "${_user_home}"
    "${_user_data}" "${_user_temp}")
get_filename_component(_temp_parent_real "${_temp_parent}" REALPATH)
get_filename_component(_system_temp_base_real "${_system_temp_base}" REALPATH)
string(FIND "${_temp_parent_real}/" "${_system_temp_base_real}/" _temp_prefix)
if(NOT _temp_prefix EQUAL 0)
    message(FATAL_ERROR "Refusing to use a non-temporary test root: ${_temp_parent_real}")
endif()
configure_file("${SAMPLE}" "${_input}" COPYONLY)

function(assert_ac1027_point path)
    file(READ "${path}" _dxf)
    string(REPLACE "\r" "" _dxf "${_dxf}")
    string(FIND "${_dxf}" "\n$ACADVER\n" _acadver_pos)
    string(FIND "${_dxf}" "\nAC1027\n" _version_pos)
    if(_acadver_pos EQUAL -1 OR _version_pos LESS _acadver_pos)
        message(FATAL_ERROR "${path} does not preserve AC1027 source version")
    endif()

    string(FIND "${_dxf}" "\nENTITIES\n" _entities_pos)
    if(_entities_pos EQUAL -1)
        message(FATAL_ERROR "${path} has no ENTITIES section")
    endif()
    string(SUBSTRING "${_dxf}" ${_entities_pos} -1 _entities)
    string(FIND "${_entities}" "\nENDSEC\n" _entities_end)
    if(_entities_end EQUAL -1)
        message(FATAL_ERROR "${path} has no terminated ENTITIES section")
    endif()
    string(SUBSTRING "${_entities}" 0 ${_entities_end} _entities)
    string(REGEX MATCHALL "\nPOINT\n" _points "${_entities}")
    list(LENGTH _points _point_count)
    if(NOT _point_count EQUAL 1)
        message(FATAL_ERROR "Expected one POINT in ${path}; got ${_point_count}")
    endif()
    string(REGEX MATCH
        "\n[ ]*10\n50([.]0+)?\n[ ]*20\n50([.]0+)?\n[ ]*30\n50([.]0+)?\n"
        _point_xyz "${_entities}")
    if(NOT _point_xyz)
        message(FATAL_ERROR "${path} does not preserve POINT WCS (50,50,50)")
    endif()
endfunction()

# Match FreeCAD Draft's exact converter argument form.
execute_process(
    COMMAND "${_build_converter}" "${_input}" -o "${_output}"
    RESULT_VARIABLE _convert_result
    OUTPUT_VARIABLE _convert_stdout
    ERROR_VARIABLE _convert_stderr
    TIMEOUT 60
)
if(NOT "${_convert_result}" STREQUAL "0" OR NOT EXISTS "${_output}")
    message(FATAL_ERROR
        "AC1027 AutoCAD DWG conversion failed (${_convert_result}); evidence retained at ${_temp_parent}\n"
        "${_convert_stdout}\n${_convert_stderr}")
endif()
assert_ac1027_point("${_output}")

execute_process(
    COMMAND "${_build_converter}" "${_output}" -o "${_roundtrip}"
    RESULT_VARIABLE _roundtrip_result
    OUTPUT_VARIABLE _roundtrip_stdout
    ERROR_VARIABLE _roundtrip_stderr
    TIMEOUT 60
)
if(NOT "${_roundtrip_result}" STREQUAL "0" OR NOT EXISTS "${_roundtrip}")
    message(FATAL_ERROR
        "AC1027 POINT DXF readback failed (${_roundtrip_result}); evidence retained at ${_temp_parent}\n"
        "${_roundtrip_stdout}\n${_roundtrip_stderr}")
endif()
assert_ac1027_point("${_roundtrip}")

file(WRITE "${_user_cfg}"
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<FCParameters>\n"
    "  <FCParamGroup Name=\"Root\">\n"
    "    <FCParamGroup Name=\"BaseApp\"/>\n"
    "  </FCParamGroup>\n"
    "</FCParameters>\n")
get_filename_component(_converter_bin_dir "${_installed_converter}" DIRECTORY)
if(WIN32)
    set(_path_separator ";")
else()
    set(_path_separator ":")
endif()
set(_test_path "${_converter_bin_dir}${_path_separator}$ENV{PATH}")
if(WIN32)
    string(REPLACE ";" "\\;" _test_path_arg "${_test_path}")
else()
    set(_test_path_arg "${_test_path}")
endif()
set(_environment
    "LIBDXFRW_FREECAD_DWG=${_input}"
    "LIBDXFRW_FREECAD_EXPECT_POINT=50,50,50"
    "LIBDXFRW_FREECAD_EXPECT_DWG2DXF=${_installed_converter}"
    "LIBDXFRW_FREECAD_EXPECT_DWG2DXF_SHA256=${_installed_sha256}"
    "LIBDXFRW_FREECAD_USER_CFG=${_user_cfg}"
    "PATH=${_test_path_arg}"
    "FREECAD_USER_HOME=${_user_home}"
    "FREECAD_USER_DATA=${_user_data}"
    "FREECAD_USER_TEMP=${_user_temp}"
    "TMPDIR=${_temp_parent}"
    "TEMP=${_temp_parent}"
    "TMP=${_temp_parent}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env ${_environment}
        "${FREECADCMD}" --user-cfg "${_user_cfg}" "${IMPORT_CHECK_MACRO}"
    RESULT_VARIABLE _freecad_result
    OUTPUT_VARIABLE _freecad_stdout
    ERROR_VARIABLE _freecad_stderr
    TIMEOUT 120
)
if(NOT "${_freecad_result}" STREQUAL "0")
    message(FATAL_ERROR
        "FreeCAD failed (${_freecad_result}); evidence retained at ${_temp_parent}\n"
        "stdout:\n${_freecad_stdout}\nstderr:\n${_freecad_stderr}")
endif()
string(FIND "${_freecad_stdout}" "FREECAD_DWG2DXF_3D_POINT_ASSERTIONS_PASS=" _pass_marker)
string(FIND "${_freecad_stdout}" "${_installed_sha256}" _converter_hash_marker)
string(FIND "${_freecad_stdout}"
    "\"expected_point\": [50.0, 50.0, 50.0]" _expected_point_marker)
if(_pass_marker EQUAL -1 OR _converter_hash_marker EQUAL -1
        OR _expected_point_marker EQUAL -1)
    message(FATAL_ERROR
        "FreeCAD output lacks the attributed AC1027 POINT pass; evidence retained at ${_temp_parent}\n"
        "stdout:\n${_freecad_stdout}\nstderr:\n${_freecad_stderr}")
endif()

message(STATUS
    "AutoCAD-authored AC1027 POINT (50,50,50) passed the installed FreeCAD converter route.\n"
    "Source SHA-256: ${_sample_sha256}\n"
    "Installed dwg2dxf SHA-256: ${_installed_sha256}\n"
    "${_freecad_stdout}")
file(REMOVE_RECURSE "${_temp_parent}")
