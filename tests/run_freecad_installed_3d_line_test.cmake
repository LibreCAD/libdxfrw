foreach(_required IN ITEMS FREECADCMD DWG2DXF BUILD_DWG2DXF
        IMPORT_CHECK_MACRO OPERATION)
    if(NOT DEFINED ${_required})
        message(FATAL_ERROR "${_required} is required")
    endif()
endforeach()
foreach(_path IN ITEMS FREECADCMD DWG2DXF BUILD_DWG2DXF IMPORT_CHECK_MACRO)
    if(NOT EXISTS "${${_path}}")
        message(FATAL_ERROR "${_path} does not exist: ${${_path}}")
    endif()
endforeach()
if(DEFINED SAMPLE)
    foreach(_required IN ITEMS EXPECTED_INPUT_SHA256 EXPECTED_MAGIC_HEX)
        if(NOT DEFINED ${_required})
            message(FATAL_ERROR "${_required} is required with SAMPLE")
        endif()
    endforeach()
    if(NOT EXISTS "${SAMPLE}")
        message(FATAL_ERROR "SAMPLE does not exist: ${SAMPLE}")
    endif()
else()
    foreach(_required IN ITEMS DWGADD RECIPE)
        if(NOT DEFINED ${_required})
            message(FATAL_ERROR "${_required} is required without SAMPLE")
        endif()
    endforeach()
    foreach(_path IN ITEMS DWGADD RECIPE)
        if(NOT EXISTS "${${_path}}")
            message(FATAL_ERROR "${_path} does not exist: ${${_path}}")
        endif()
    endforeach()
endif()
if(NOT OPERATION STREQUAL "open" AND NOT OPERATION STREQUAL "insert")
    message(FATAL_ERROR "OPERATION must be open or insert")
endif()
if(DEFINED INPUT_PATH_UNICODE AND NOT INPUT_PATH_UNICODE STREQUAL "ON"
        AND NOT INPUT_PATH_UNICODE STREQUAL "OFF")
    message(FATAL_ERROR "INPUT_PATH_UNICODE must be ON or OFF")
endif()
if(NOT DEFINED DWG_CONVERSION_MODE)
    set(DWG_CONVERSION_MODE 1)
endif()
if(NOT DWG_CONVERSION_MODE STREQUAL "0"
        AND NOT DWG_CONVERSION_MODE STREQUAL "1")
    message(FATAL_ERROR "DWG_CONVERSION_MODE must be 0 or 1")
endif()

get_filename_component(_installed_converter "${DWG2DXF}" REALPATH)
get_filename_component(_build_converter "${BUILD_DWG2DXF}" REALPATH)
if(_installed_converter STREQUAL _build_converter)
    message(FATAL_ERROR "FreeCAD integration must invoke the installed converter")
endif()

set(_allow_installed_rpath_rewrite FALSE)
if(DEFINED ALLOW_INSTALLED_RPATH_REWRITE
        AND ALLOW_INSTALLED_RPATH_REWRITE)
    if(NOT DEFINED BUILD_SHARED_LIBS OR NOT BUILD_SHARED_LIBS
            OR NOT DEFINED INSTALL_PREFIX OR NOT IS_DIRECTORY "${INSTALL_PREFIX}")
        message(FATAL_ERROR
            "RPATH-rewritten install qualification requires a shared build and its isolated install prefix")
    endif()
    file(REAL_PATH "${INSTALL_PREFIX}" _install_prefix_real)
    string(FIND "${_installed_converter}/" "${_install_prefix_real}/"
        _install_prefix_position)
    if(NOT _install_prefix_position EQUAL 0)
        message(FATAL_ERROR
            "Installed converter is outside the isolated install prefix: ${_installed_converter}")
    endif()
    set(_allow_installed_rpath_rewrite TRUE)
endif()

file(SHA256 "${_installed_converter}" _installed_sha256)
file(SHA256 "${_build_converter}" _build_sha256)
if(NOT _installed_sha256 STREQUAL _build_sha256
        AND NOT _allow_installed_rpath_rewrite)
    message(FATAL_ERROR
        "Installed dwg2dxf is not byte-identical to this build's target: "
        "${_installed_sha256} != ${_build_sha256}")
endif()
if(_allow_installed_rpath_rewrite
        AND NOT _installed_sha256 STREQUAL _build_sha256)
    message(STATUS
        "Installed shared executable hash differs from the build artifact after CMake install RPATH rewriting; "
        "the isolated wrapper installed this target into ${_install_prefix_real}")
endif()
set(_converter_sha256 "${_installed_sha256}")
get_filename_component(_converter_bin_dir "${_installed_converter}" DIRECTORY)

if(DEFINED ENV{TMPDIR} AND IS_DIRECTORY "$ENV{TMPDIR}")
    set(_system_temp_base "$ENV{TMPDIR}")
elseif(DEFINED ENV{TEMP} AND IS_DIRECTORY "$ENV{TEMP}")
    set(_system_temp_base "$ENV{TEMP}")
else()
    message(FATAL_ERROR "TMPDIR or TEMP must identify the system temp directory")
endif()
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef _run_id)
set(_temp_parent "${_system_temp_base}/libdxfrw-freecad-installed-line-${_run_id} with spaces")
set(_test_root "${_temp_parent}/runtime")
if(INPUT_PATH_UNICODE)
    set(_input_dir "${_test_root}/entrée café input with spaces")
else()
    set(_input_dir "${_test_root}/input with spaces")
endif()
if(DEFINED SAMPLE)
    set(_input "${_input_dir}/AC1027 tracked 3D LINE.dwg")
else()
    set(_input "${_input_dir}/AC1015 generated 3D LINE.dwg")
endif()
set(_profile "${_test_root}/profile-${OPERATION}")
set(_user_cfg "${_profile}/user.cfg")
set(_user_home "${_profile}/home")
set(_user_data "${_profile}/data")
set(_user_temp "${_profile}/temp")
file(MAKE_DIRECTORY "${_input_dir}" "${_user_home}" "${_user_data}"
    "${_user_temp}")
get_filename_component(_temp_parent_real "${_temp_parent}" REALPATH)
get_filename_component(_system_temp_base_real "${_system_temp_base}" REALPATH)
string(FIND "${_temp_parent_real}/" "${_system_temp_base_real}/" _temp_prefix)
if(NOT _temp_prefix EQUAL 0)
    message(FATAL_ERROR
        "Refusing to use a non-temporary test root: ${_temp_parent_real}")
endif()

if(DEFINED SAMPLE)
    file(SHA256 "${SAMPLE}" _sample_sha256)
    if(NOT _sample_sha256 STREQUAL EXPECTED_INPUT_SHA256)
        message(FATAL_ERROR
            "Tracked FreeCAD DWG sample hash mismatch: ${_sample_sha256} != ${EXPECTED_INPUT_SHA256}")
    endif()
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy "${SAMPLE}" "${_input}"
        RESULT_VARIABLE _copy_result
        OUTPUT_VARIABLE _copy_stdout
        ERROR_VARIABLE _copy_stderr
        TIMEOUT 60
    )
    if(NOT "${_copy_result}" STREQUAL "0" OR NOT EXISTS "${_input}")
        message(FATAL_ERROR
            "Could not copy the tracked AC1027 LINE control (${_copy_result}).\n"
            "Evidence retained at ${_temp_parent}\n${_copy_stdout}\n${_copy_stderr}")
    endif()
else()
    execute_process(
        COMMAND "${DWGADD}" --as r2000 -o "${_input}" "${RECIPE}"
        RESULT_VARIABLE _generate_result
        OUTPUT_VARIABLE _generate_stdout
        ERROR_VARIABLE _generate_stderr
        TIMEOUT 60
    )
    if(NOT "${_generate_result}" STREQUAL "0" OR NOT EXISTS "${_input}")
        message(FATAL_ERROR
            "Could not generate the local AC1015 LINE control (${_generate_result}).\n"
            "Evidence retained at ${_temp_parent}\n"
            "${_generate_stdout}\n${_generate_stderr}")
    endif()
endif()
file(READ "${_input}" _magic_hex LIMIT 6 HEX)
if(DEFINED EXPECTED_MAGIC_HEX)
    set(_expected_magic_hex "${EXPECTED_MAGIC_HEX}")
else()
    set(_expected_magic_hex "414331303135")
endif()
if(NOT "${_magic_hex}" STREQUAL "${_expected_magic_hex}")
    message(FATAL_ERROR
        "The FreeCAD DWG control has unexpected version magic (${_magic_hex} != ${_expected_magic_hex}); "
        "evidence retained at ${_temp_parent}")
endif()

file(WRITE "${_user_cfg}"
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<FCParameters>\n"
    "  <FCParamGroup Name=\"Root\">\n"
    "    <FCParamGroup Name=\"BaseApp\"/>\n"
    "  </FCParamGroup>\n"
    "</FCParameters>\n")

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
    "LIBDXFRW_FREECAD_DWG_CONVERSION=${DWG_CONVERSION_MODE}"
    "LIBDXFRW_FREECAD_SEED_ISOLATED_PREFERENCES=1"
    "LIBDXFRW_FREECAD_USER_CFG=${_user_cfg}"
    "LIBDXFRW_FREECAD_TEMP_ROOT=${_test_root}"
    "LIBDXFRW_FREECAD_EXPECT_DWG2DXF=${_installed_converter}"
    "LIBDXFRW_FREECAD_EXPECT_DWG2DXF_SHA256=${_converter_sha256}"
    "LIBDXFRW_FREECAD_EXPECT_LINE_BOUNDS=[[1,2,3,4,6,9]]"
    "PATH=${_test_path_arg}"
    "FREECAD_USER_HOME=${_user_home}"
    "FREECAD_USER_DATA=${_user_data}"
    "FREECAD_USER_TEMP=${_user_temp}"
    "TMPDIR=${_temp_parent}"
    "TEMP=${_temp_parent}"
    "TMP=${_temp_parent}")
if(DEFINED EXPECTED_LINE_BOUNDS)
    list(REMOVE_ITEM _environment
        "LIBDXFRW_FREECAD_EXPECT_LINE_BOUNDS=[[1,2,3,4,6,9]]")
    list(APPEND _environment
        "LIBDXFRW_FREECAD_EXPECT_LINE_BOUNDS=${EXPECTED_LINE_BOUNDS}")
endif()
if(DEFINED EXPECTED_FREECAD_REVISION)
    list(APPEND _environment
        "LIBDXFRW_FREECAD_EXPECT_REVISION=${EXPECTED_FREECAD_REVISION}")
endif()

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
        "FreeCAD ${OPERATION} integration failed (${_freecad_result}).\n"
        "Evidence retained at ${_temp_parent}\n"
        "stdout:\n${_freecad_stdout}\nstderr:\n${_freecad_stderr}")
endif()

foreach(_expected IN ITEMS
        "FREECAD_DWG_IMPORT_ASSERTIONS_PASS="
        "${_installed_converter}"
        "${_converter_sha256}"
        "\"operation\": \"${OPERATION}\""
        "\"discovery\": \"path\""
        "\"dwg_conversion\": ${DWG_CONVERSION_MODE}"
        "\"expected_line_bounds\"")
    string(FIND "${_freecad_stdout}" "${_expected}" _found)
    if(_found EQUAL -1)
        message(FATAL_ERROR
            "FreeCAD ${OPERATION} output lacks '${_expected}'.\n"
            "Evidence retained at ${_temp_parent}\n"
            "stdout:\n${_freecad_stdout}\nstderr:\n${_freecad_stderr}")
    endif()
endforeach()

message(STATUS
    "Installed dwg2dxf FreeCAD ${OPERATION} geometry check passed.\n"
    "Converter: ${_installed_converter} (SHA-256 ${_converter_sha256})\n"
    "FreeCAD output:\n${_freecad_stdout}\n${_freecad_stderr}")
file(REMOVE_RECURSE "${_temp_parent}")
