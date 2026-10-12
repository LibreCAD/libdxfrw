foreach(_required IN ITEMS FREECADCMD DWG2DXF BUILD_DWG2DXF ODAFILECONVERTER
        SOURCE_DXF IMPORT_CHECK_MACRO OUTPUT_DIR)
    if(NOT DEFINED ${_required} OR NOT EXISTS "${${_required}}")
        message(FATAL_ERROR "${_required} is missing or does not exist: '${${_required}}'")
    endif()
endforeach()
if(NOT DEFINED OPERATION)
    message(FATAL_ERROR "OPERATION is required")
endif()
if(NOT OPERATION STREQUAL "open" AND NOT OPERATION STREQUAL "insert")
    message(FATAL_ERROR "OPERATION must be open or insert")
endif()

get_filename_component(_installed_converter "${DWG2DXF}" REALPATH)
get_filename_component(_build_converter "${BUILD_DWG2DXF}" REALPATH)
if(_installed_converter STREQUAL _build_converter)
    message(FATAL_ERROR "FreeCAD geometry check must invoke the installed converter")
endif()
file(SHA256 "${_installed_converter}" _installed_sha256)
file(SHA256 "${_build_converter}" _build_sha256)
if(NOT _installed_sha256 STREQUAL _build_sha256)
    message(FATAL_ERROR
        "Installed converter differs from this build: ${_installed_sha256} != ${_build_sha256}")
endif()

if(DEFINED ENV{TMPDIR} AND IS_DIRECTORY "$ENV{TMPDIR}")
    set(_system_temp_base "$ENV{TMPDIR}")
elseif(DEFINED ENV{TEMP} AND IS_DIRECTORY "$ENV{TEMP}")
    set(_system_temp_base "$ENV{TEMP}")
else()
    message(FATAL_ERROR "TMPDIR or TEMP must identify the system temp directory")
endif()
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef _run_id)
set(_temp_parent "${_system_temp_base}/libdxfrw-freecad-3dface-${_run_id} with spaces")
set(_test_root "${_temp_parent}/runtime")
set(_oda_output_dir "${_test_root}/oda dwg")
set(_profile "${_test_root}/profile-${OPERATION}")
set(_user_cfg "${_profile}/user.cfg")
set(_user_home "${_profile}/home")
set(_user_data "${_profile}/data")
set(_user_temp "${_profile}/temp")
file(MAKE_DIRECTORY "${_oda_output_dir}" "${_user_home}" "${_user_data}"
    "${_user_temp}")
get_filename_component(_temp_parent_real "${_temp_parent}" REALPATH)
get_filename_component(_system_temp_real "${_system_temp_base}" REALPATH)
string(FIND "${_temp_parent_real}/" "${_system_temp_real}/" _temp_prefix)
if(NOT _temp_prefix EQUAL 0)
    message(FATAL_ERROR "Refusing a non-temporary FreeCAD test root: ${_temp_parent_real}")
endif()

get_filename_component(_source_name "${SOURCE_DXF}" NAME)
get_filename_component(_source_dir "${SOURCE_DXF}" DIRECTORY)
set(_input "${_oda_output_dir}/ac1015_3dface_freecad_control.dwg")
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
        "ODA could not create the AC1015 3DFACE control (${_write_result}).\n"
        "Evidence retained at ${_test_root}\n${_write_stdout}\n${_write_stderr}")
endif()
file(READ "${_input}" _magic_hex LIMIT 6 HEX)
if(NOT "${_magic_hex}" STREQUAL "414331303135")
    message(FATAL_ERROR "Expected ODA's AC1015 output, got DWG signature ${_magic_hex}")
endif()

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
if(WIN32)
    string(REPLACE ";" "\\;" _test_path_arg "${_test_path}")
else()
    set(_test_path_arg "${_test_path}")
endif()
set(_environment
    "LIBDXFRW_FREECAD_DWG=${_input}"
    "LIBDXFRW_FREECAD_OPERATION=${OPERATION}"
    "LIBDXFRW_FREECAD_DISCOVERY=path"
    "LIBDXFRW_FREECAD_SEED_ISOLATED_PREFERENCES=1"
    "LIBDXFRW_FREECAD_USER_CFG=${_user_cfg}"
    "LIBDXFRW_FREECAD_TEMP_ROOT=${_test_root}"
    "LIBDXFRW_FREECAD_EXPECT_DWG2DXF=${_installed_converter}"
    "LIBDXFRW_FREECAD_EXPECT_DWG2DXF_SHA256=${_converter_sha256}"
    "LIBDXFRW_FREECAD_EXPECT_3DFACE=unsupported"
    "LIBDXFRW_FREECAD_EXPECT_REVISION=145529fe741292ff0b3977a01195bf0247425794"
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
        "FreeCAD 3DFACE integration failed (${_freecad_result}).\n"
        "Evidence retained at ${_test_root}\n"
        "stdout:\n${_freecad_stdout}\nstderr:\n${_freecad_stderr}")
endif()
foreach(_expected IN ITEMS
        "FREECAD_DWG_IMPORT_ASSERTIONS_PASS="
        "${_installed_converter}"
        "${_converter_sha256}"
        "\"operation\": \"${OPERATION}\""
        "\"support_level\": \"converter-integration-only\""
        "\"importer_outcome\": \"unsupported_by_pinned_freecad_importer\""
        "\"entity_count\": 1"
        "\"unsupported_count\": 1"
        "\"created_shape_count\": 0")
    string(FIND "${_freecad_stdout}" "${_expected}" _found)
    if(_found EQUAL -1)
        message(FATAL_ERROR
            "FreeCAD output lacks '${_expected}'.\n"
            "Evidence retained at ${_test_root}\n"
            "stdout:\n${_freecad_stdout}\nstderr:\n${_freecad_stderr}")
    endif()
endforeach()

message(STATUS
    "FreeCAD AC1015 3DFACE converter integration and pinned importer limitation "
    "check passed for installed ${_installed_converter} "
    "(SHA-256 ${_converter_sha256}).\n${_freecad_stdout}\n${_freecad_stderr}")
file(REMOVE_RECURSE "${_temp_parent}")
