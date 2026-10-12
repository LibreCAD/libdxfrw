foreach(_required IN ITEMS FREECADCMD DWG2DXF BUILD_DWG2DXF IMPORT_CHECK_MACRO)
    if(NOT DEFINED ${_required})
        message(FATAL_ERROR "${_required} is required")
    endif()
    if(NOT EXISTS "${${_required}}")
        message(FATAL_ERROR "${_required} does not exist: ${${_required}}")
    endif()
endforeach()

get_filename_component(_installed_converter "${DWG2DXF}" REALPATH)
get_filename_component(_build_converter "${BUILD_DWG2DXF}" REALPATH)
if(_installed_converter STREQUAL _build_converter)
    message(FATAL_ERROR
        "FreeCAD failure-path qualification must invoke the installed converter")
endif()

if(DEFINED ENV{TMPDIR} AND IS_DIRECTORY "$ENV{TMPDIR}")
    set(_system_temp_base "$ENV{TMPDIR}")
elseif(DEFINED ENV{TEMP} AND IS_DIRECTORY "$ENV{TEMP}")
    set(_system_temp_base "$ENV{TEMP}")
else()
    message(FATAL_ERROR "TMPDIR or TEMP must identify the system temp directory")
endif()
get_filename_component(_system_temp_base_real "${_system_temp_base}" REALPATH)
if(NOT IS_DIRECTORY "${_system_temp_base_real}"
        OR _system_temp_base_real STREQUAL "/")
    message(FATAL_ERROR
        "Refusing to create FreeCAD test data in unsafe temp root: ${_system_temp_base_real}")
endif()
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef _run_id)
set(_temp_parent
    "${_system_temp_base_real}/libdxfrw-freecad-failure-${_run_id} with spaces")
set(_test_root "${_temp_parent}/runtime")
if(EXISTS "${_temp_parent}")
    message(FATAL_ERROR "Unique FreeCAD failure-test root already exists: ${_temp_parent}")
endif()
file(MAKE_DIRECTORY "${_test_root}")
get_filename_component(_temp_parent_real "${_temp_parent}" REALPATH)
string(FIND "${_temp_parent_real}/" "${_system_temp_base_real}/" _temp_prefix)
if(NOT _temp_prefix EQUAL 0 OR _temp_parent_real STREQUAL _system_temp_base_real)
    message(FATAL_ERROR
        "Refusing to use a non-temporary FreeCAD test root: ${_temp_parent_real}")
endif()

set(_input_dir "${_test_root}/malformed input with spaces")
set(_input "${_input_dir}/invalid drawing.dwg")
set(_profile "${_test_root}/profile")
set(_user_cfg "${_profile}/user.cfg")
set(_user_home "${_profile}/home")
set(_user_data "${_profile}/data")
set(_user_temp "${_profile}/temp")
file(MAKE_DIRECTORY "${_input_dir}" "${_user_home}" "${_user_data}"
    "${_user_temp}")
# This deliberately malformed input is generated from scratch and is never
# added to the source tree. The current CLI rejects it before opening a writer.
file(WRITE "${_input}" "AC1027")
file(WRITE "${_user_cfg}"
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<FCParameters>\n"
    "  <FCParamGroup Name=\"Root\">\n"
    "    <FCParamGroup Name=\"BaseApp\"/>\n"
    "  </FCParamGroup>\n"
    "</FCParameters>\n")

file(SHA256 "${_installed_converter}" _converter_sha256)
get_filename_component(_converter_bin_dir "${_installed_converter}" DIRECTORY)
if(WIN32)
    set(_path_separator ";")
else()
    set(_path_separator ":")
endif()
set(_test_path "${_converter_bin_dir}${_path_separator}$ENV{PATH}")
set(_environment
    "LIBDXFRW_FREECAD_DWG=${_input}"
    "LIBDXFRW_FREECAD_OPERATION=open"
    "LIBDXFRW_FREECAD_DISCOVERY=path"
    "LIBDXFRW_FREECAD_EXPECT_CONVERSION_FAILURE=1"
    "LIBDXFRW_FREECAD_SEED_ISOLATED_PREFERENCES=1"
    "LIBDXFRW_FREECAD_USER_CFG=${_user_cfg}"
    "LIBDXFRW_FREECAD_TEMP_ROOT=${_test_root}"
    "LIBDXFRW_FREECAD_EXPECT_DWG2DXF=${_installed_converter}"
    "LIBDXFRW_FREECAD_EXPECT_DWG2DXF_SHA256=${_converter_sha256}"
    "PATH=${_test_path}"
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
    TIMEOUT 90
)
if(NOT "${_freecad_result}" STREQUAL "0")
    message(FATAL_ERROR
        "FreeCAD conversion-failure check failed (${_freecad_result}).\n"
        "stdout:\n${_freecad_stdout}\nstderr:\n${_freecad_stderr}\n"
        "Isolated evidence retained at ${_test_root}")
endif()
string(FIND "${_freecad_stdout}" "FREECAD_DWG_IMPORT_FAILURE_ASSERTIONS_PASS="
    _failure_marker)
string(FIND "${_freecad_stdout}" "\"expected_failure\": true" _expected_failure)
string(FIND "${_freecad_stdout}" "\"failure_stage\": \"dwg2dxf\"" _failure_stage)
string(FIND "${_freecad_stdout}" "\"output_exists\": false" _output_absent)
string(FIND "${_freecad_stdout}" "\"import_handoff\": null" _no_handoff)
string(FIND "${_freecad_stdout}" "\"returncode\":" _return_code_recorded)
string(FIND "${_freecad_stdout}" "FREECAD_DWG_IMPORT_ASSERTIONS_PASS="
    _positive_marker)
string(FIND "${_freecad_stdout}" "Conversion successful" _false_success)
if(_failure_marker EQUAL -1 OR _expected_failure EQUAL -1
        OR _failure_stage EQUAL -1 OR _output_absent EQUAL -1
        OR _no_handoff EQUAL -1 OR _return_code_recorded EQUAL -1
        OR NOT _positive_marker EQUAL -1 OR NOT _false_success EQUAL -1)
    message(FATAL_ERROR
        "FreeCAD failure-path output is missing a required negative assertion.\n"
        "stdout:\n${_freecad_stdout}\nstderr:\n${_freecad_stderr}\n"
        "Isolated evidence retained at ${_test_root}")
endif()

message(STATUS
    "FreeCAD rejected malformed DWG without a DXF-importer handoff.\n"
    "Installed converter: ${_installed_converter} (SHA-256 ${_converter_sha256})\n"
    "Observed result:\n${_freecad_stdout}\n${_freecad_stderr}")
file(REMOVE_RECURSE "${_temp_parent_real}")
