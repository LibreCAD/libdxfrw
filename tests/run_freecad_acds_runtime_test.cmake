# Opt-in, caller-supplied AC1027 ACDSDATA FreeCAD handoff check. This script
# keeps the input, isolated preferences, and generated DXF in system temp.
foreach(_required IN ITEMS FREECADCMD DWG2DXF BUILD_DWG2DXF SAMPLE
        EXPECTED_SHA256 FREECAD_REVISION IMPORT_MACRO OPERATION)
    if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
        message(FATAL_ERROR "${_required} is required")
    endif()
endforeach()
foreach(_path IN ITEMS FREECADCMD DWG2DXF BUILD_DWG2DXF SAMPLE IMPORT_MACRO)
    if(NOT EXISTS "${${_path}}")
        message(FATAL_ERROR "${_path} does not exist: ${${_path}}")
    endif()
endforeach()
if(NOT OPERATION STREQUAL "open" AND NOT OPERATION STREQUAL "insert")
    message(FATAL_ERROR "OPERATION must be open or insert")
endif()
get_filename_component(_installed_converter "${DWG2DXF}" REALPATH)
get_filename_component(_build_converter "${BUILD_DWG2DXF}" REALPATH)
if(_installed_converter STREQUAL _build_converter)
    message(FATAL_ERROR "FreeCAD ACDSDATA check requires an installed converter")
endif()
file(SHA256 "${_installed_converter}" _actual_sha256)
if(NOT _actual_sha256 STREQUAL EXPECTED_SHA256)
    message(FATAL_ERROR
        "Installed converter SHA-256 differs from caller expectation: "
        "${_actual_sha256} != ${EXPECTED_SHA256}")
endif()
file(READ "${SAMPLE}" _magic_hex LIMIT 6 HEX)
if(NOT _magic_hex STREQUAL "414331303237")
    message(FATAL_ERROR "The external ACDSDATA witness must be AC1027")
endif()
if(DEFINED BASELINE_DXF AND NOT "${BASELINE_DXF}" STREQUAL "")
    if(NOT EXISTS "${BASELINE_DXF}" OR NOT EXISTS "${ACDS_CHECKER}")
        message(FATAL_ERROR "The baseline DXF and ACDSDATA checker must exist")
    endif()
    if(NOT DEFINED PYTHON OR NOT EXISTS "${PYTHON}")
        message(FATAL_ERROR "PYTHON is required with BASELINE_DXF")
    endif()
endif()

string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef _run_id)
if(DEFINED ENV{TMPDIR} AND IS_DIRECTORY "$ENV{TMPDIR}")
    set(_system_temp_base "$ENV{TMPDIR}")
elseif(DEFINED ENV{TEMP} AND IS_DIRECTORY "$ENV{TEMP}")
    set(_system_temp_base "$ENV{TEMP}")
else()
    message(FATAL_ERROR "TMPDIR or TEMP must identify a system temp directory")
endif()
set(_temp_parent "${_system_temp_base}/libdxfrw-freecad-acds-${_run_id} with spaces")
if(EXISTS "${_temp_parent}")
    message(FATAL_ERROR "Refusing an existing FreeCAD test root: ${_temp_parent}")
endif()
set(_input_dir "${_temp_parent}/input with spaces")
set(_test_root "${_temp_parent}/runtime")
set(_profile "${_test_root}/profile-${OPERATION}")
set(_user_cfg "${_profile}/user.cfg")
set(_user_home "${_profile}/home")
set(_user_data "${_profile}/data")
set(_user_temp "${_profile}/temp")
set(_input "${_input_dir}/AC1027 modeler input.dwg")
file(MAKE_DIRECTORY "${_input_dir}" "${_user_home}" "${_user_data}"
    "${_user_temp}")
get_filename_component(_temp_parent_real "${_temp_parent}" REALPATH)
get_filename_component(_system_temp_base_real "${_system_temp_base}" REALPATH)
string(FIND "${_temp_parent_real}/" "${_system_temp_base_real}/" _temp_prefix)
if(NOT _temp_prefix EQUAL 0)
    message(FATAL_ERROR "Refusing non-temporary FreeCAD root: ${_temp_parent_real}")
endif()
configure_file("${SAMPLE}" "${_input}" COPYONLY)
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
set(_environment
    "LIBDXFRW_ACDS_SAMPLE=${_input}"
    "LIBDXFRW_ACDS_OPERATION=${OPERATION}"
    "LIBDXFRW_ACDS_BINARY=${_installed_converter}"
    "LIBDXFRW_ACDS_BINARY_SHA=${EXPECTED_SHA256}"
    "LIBDXFRW_ACDS_FREECAD_REVISION=${FREECAD_REVISION}"
    "LIBDXFRW_ACDS_USER_CFG=${_user_cfg}"
    "LIBDXFRW_ACDS_TEMP_ROOT=${_temp_parent}"
    "PATH=${_test_path}"
    "FREECAD_USER_HOME=${_user_home}"
    "FREECAD_USER_DATA=${_user_data}"
    "FREECAD_USER_TEMP=${_user_temp}"
    "TMPDIR=${_user_temp}"
    "TEMP=${_user_temp}"
    "TMP=${_user_temp}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env ${_environment}
        "${FREECADCMD}" --user-cfg "${_user_cfg}" "${IMPORT_MACRO}"
    RESULT_VARIABLE _freecad_result
    OUTPUT_VARIABLE _freecad_stdout
    ERROR_VARIABLE _freecad_stderr
    TIMEOUT 150)
if(NOT "${_freecad_result}" STREQUAL "0")
    message(FATAL_ERROR
        "FreeCAD ACDSDATA ${OPERATION} failed (${_freecad_result}).\n"
        "stdout:\n${_freecad_stdout}\nstderr:\n${_freecad_stderr}\n"
        "Evidence retained at ${_temp_parent}")
endif()
string(REGEX MATCH "FREECAD_ACDS_HANDOFF_PASS=(\\{[^\n]*\\})"
    _pass_line "${_freecad_stdout}")
if(NOT _pass_line)
    message(FATAL_ERROR
        "FreeCAD ACDSDATA ${OPERATION} returned no assertion marker.\n"
        "stdout:\n${_freecad_stdout}\nstderr:\n${_freecad_stderr}\n"
        "Evidence retained at ${_temp_parent}")
endif()
set(_result_json "${CMAKE_MATCH_1}")
string(JSON _observed_operation GET "${_result_json}" operation)
string(JSON _observed_status GET "${_result_json}" semantic_status)
string(JSON _observed_support GET "${_result_json}" support_level)
string(JSON _output_dxf GET "${_result_json}" output)
if(NOT _observed_operation STREQUAL OPERATION
        OR NOT _observed_status STREQUAL "unsupported_by_pinned_freecad_importer"
        OR NOT _observed_support STREQUAL "converter-integration-only"
        OR NOT EXISTS "${_output_dxf}")
    message(FATAL_ERROR
        "FreeCAD ACDSDATA ${OPERATION} result lacks pinned handoff outcome: "
        "${_result_json}; evidence retained at ${_temp_parent}")
endif()
if(DEFINED BASELINE_DXF AND NOT "${BASELINE_DXF}" STREQUAL "")
    execute_process(
        COMMAND "${PYTHON}" "${ACDS_CHECKER}" "${BASELINE_DXF}"
            "${_output_dxf}"
        RESULT_VARIABLE _checker_result
        OUTPUT_VARIABLE _checker_stdout
        ERROR_VARIABLE _checker_stderr
        TIMEOUT 30)
    if(NOT "${_checker_result}" STREQUAL "0")
        message(FATAL_ERROR
            "ACDSDATA structural/SAB comparison failed (${_checker_result}).\n"
            "stdout:\n${_checker_stdout}\nstderr:\n${_checker_stderr}\n"
            "Evidence retained at ${_temp_parent}")
    endif()
    message(STATUS "ACDSDATA comparison: ${_checker_stdout}")
endif()
message(STATUS
    "FreeCAD ACDSDATA ${OPERATION} converter handoff passed for installed "
    "${_installed_converter} (SHA-256 ${_actual_sha256}).\n"
    "Observation: ${_result_json}")
file(REMOVE_RECURSE "${_temp_parent}")
