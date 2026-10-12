if(NOT DEFINED FREECAD_DWG2DXF_EXECUTABLE
        OR NOT EXISTS "${FREECAD_DWG2DXF_EXECUTABLE}")
    message(FATAL_ERROR
        "FREECAD_DWG2DXF_EXECUTABLE must name the installed dwg2dxf artifact")
endif()
if(NOT DEFINED BUILD_DWG2DXF_EXECUTABLE
        OR NOT EXISTS "${BUILD_DWG2DXF_EXECUTABLE}")
    message(FATAL_ERROR "BUILD_DWG2DXF_EXECUTABLE is required for install isolation")
endif()
if(NOT DEFINED SAMPLE OR NOT EXISTS "${SAMPLE}")
    message(FATAL_ERROR "SAMPLE must name an existing tracked DWG")
endif()
if(NOT DEFINED DESKTOP_CHECK_MACRO OR NOT EXISTS "${DESKTOP_CHECK_MACRO}"
        OR NOT DEFINED IMPORT_CHECK_MACRO OR NOT EXISTS "${IMPORT_CHECK_MACRO}")
    message(FATAL_ERROR "Both FreeCAD desktop check macros are required")
endif()
get_filename_component(_installed_converter
    "${FREECAD_DWG2DXF_EXECUTABLE}" REALPATH)
get_filename_component(_build_converter
    "${BUILD_DWG2DXF_EXECUTABLE}" REALPATH)
if(_installed_converter STREQUAL _build_converter)
    message(FATAL_ERROR
        "FreeCAD desktop qualification must use the installed artifact, not the build target")
endif()
if(NOT DEFINED FREECAD_GUI_EXECUTABLE OR NOT EXISTS "${FREECAD_GUI_EXECUTABLE}")
    if(NOT APPLE OR NOT DEFINED FREECAD_APP_BUNDLE
            OR NOT IS_DIRECTORY "${FREECAD_APP_BUNDLE}")
        message(FATAL_ERROR
            "Set FREECAD_GUI_EXECUTABLE, or on macOS set FREECAD_APP_BUNDLE")
    endif()
endif()

if(APPLE)
    if(NOT DEFINED FREECAD_APP_BUNDLE OR NOT IS_DIRECTORY "${FREECAD_APP_BUNDLE}")
        message(FATAL_ERROR "macOS desktop qualification needs FREECAD_APP_BUNDLE")
    endif()
    set(_freecad_launcher "${FREECAD_APP_BUNDLE}/Contents/MacOS/FreeCAD")
    if(NOT EXISTS "${_freecad_launcher}")
        message(FATAL_ERROR
            "FreeCAD.app does not contain Contents/MacOS/FreeCAD")
    endif()
elseif(NOT DEFINED FREECAD_GUI_EXECUTABLE
        OR NOT EXISTS "${FREECAD_GUI_EXECUTABLE}")
    message(FATAL_ERROR "FREECAD_GUI_EXECUTABLE was not found")
else()
    set(_freecad_launcher "${FREECAD_GUI_EXECUTABLE}")
endif()

file(SHA256 "${_installed_converter}" _converter_sha256)
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef _run_id)
if(DEFINED ENV{TMPDIR} AND IS_DIRECTORY "$ENV{TMPDIR}")
    set(_system_temp "$ENV{TMPDIR}")
elseif(DEFINED ENV{TEMP} AND IS_DIRECTORY "$ENV{TEMP}")
    set(_system_temp "$ENV{TEMP}")
else()
    message(FATAL_ERROR
        "Set TMPDIR (or TEMP on Windows) to a system temporary directory")
endif()
set(_test_root "${_system_temp}/libdxfrw-freecad-desktop-${_run_id}")
file(MAKE_DIRECTORY "${_test_root}")
set(_input_dir "${_test_root}/input with spaces")
set(_source_copy "${_input_dir}/ordinary encoding.dwg")
file(MAKE_DIRECTORY "${_input_dir}")
configure_file("${SAMPLE}" "${_source_copy}" COPYONLY)

set(_runtime_evidence "")
foreach(_operation IN ITEMS open insert)
    set(_profile "${_test_root}/profile-${_operation}")
    set(_user_home "${_profile}/home")
    set(_user_data "${_profile}/data")
    set(_user_temp "${_profile}/temp")
    set(_user_cfg "${_profile}/user.cfg")
    set(_result_file "${_test_root}/${_operation}.json")
    file(MAKE_DIRECTORY "${_user_home}" "${_user_data}" "${_user_temp}")
    file(WRITE "${_user_cfg}"
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<FCParameters>\n"
        "  <FCParamGroup Name=\"Root\">\n"
        "    <FCParamGroup Name=\"BaseApp\"/>\n"
        "  </FCParamGroup>\n"
        "</FCParameters>\n")

    set(_environment
        "LIBDXFRW_FREECAD_DWG=${_source_copy}"
        "LIBDXFRW_FREECAD_OPERATION=${_operation}"
        "LIBDXFRW_FREECAD_DISCOVERY=configured"
        "LIBDXFRW_FREECAD_SEED_ISOLATED_PREFERENCES=1"
        "LIBDXFRW_FREECAD_USER_CFG=${_user_cfg}"
        "LIBDXFRW_FREECAD_TEIGHA_CONVERTER=${_installed_converter}"
        "LIBDXFRW_FREECAD_EXPECT_DWG2DXF=${_installed_converter}"
        "LIBDXFRW_FREECAD_EXPECT_DWG2DXF_SHA256=${_converter_sha256}"
        "LIBDXFRW_FREECAD_TEMP_ROOT=${_test_root}"
        "LIBDXFRW_FREECAD_RESULT_FILE=${_result_file}"
        "LIBDXFRW_FREECAD_CHECK_MACRO=${IMPORT_CHECK_MACRO}"
        "FREECAD_USER_HOME=${_user_home}"
        "FREECAD_USER_DATA=${_user_data}"
        "FREECAD_USER_TEMP=${_user_temp}"
        "TMPDIR=${_system_temp}")

    set(_command
        "${CMAKE_COMMAND}" -E env ${_environment}
        "${_freecad_launcher}" --user-cfg "${_user_cfg}"
        "${DESKTOP_CHECK_MACRO}")

    execute_process(
        COMMAND ${_command}
        RESULT_VARIABLE _exit_status
        OUTPUT_VARIABLE _stdout
        ERROR_VARIABLE _stderr
        TIMEOUT 90
    )
    if(NOT _exit_status STREQUAL "0")
        message(FATAL_ERROR
            "FreeCAD desktop ${_operation} run failed (status ${_exit_status}).\n"
            "stdout:\n${_stdout}\nstderr:\n${_stderr}\n"
            "Isolated evidence retained at ${_test_root}")
    endif()
    if(NOT EXISTS "${_result_file}")
        message(FATAL_ERROR
            "FreeCAD desktop ${_operation} emitted no result file.\n"
            "stdout:\n${_stdout}\nstderr:\n${_stderr}\n"
            "Isolated evidence retained at ${_test_root}")
    endif()
    file(READ "${_result_file}" _result_json)
    string(FIND "${_result_json}" "\"status\": \"pass\"" _pass_status)
    string(FIND "${_result_json}" "\"desktop_dispatch\": true" _desktop_dispatch)
    string(FIND "${_result_json}" "\"selected_import_module\": \"importDWG\""
        _registered_module)
    string(FIND "${_result_json}" "\"dxf_import_dialog_enabled\": false"
        _import_dialog_disabled)
    if(_pass_status EQUAL -1 OR _desktop_dispatch EQUAL -1
            OR _registered_module EQUAL -1 OR _import_dialog_disabled EQUAL -1)
        message(FATAL_ERROR
            "FreeCAD desktop ${_operation} assertions did not pass.\n"
            "result:\n${_result_json}\nstdout:\n${_stdout}\nstderr:\n${_stderr}\n"
            "Isolated evidence retained at ${_test_root}")
    endif()
    set(_runtime_evidence "${_runtime_evidence}\n${_operation}: ${_result_json}")
endforeach()

file(REMOVE_RECURSE "${_test_root}")
message(STATUS
    "FreeCAD desktop open/insert passed with installed dwg2dxf ${_converter_sha256}${_runtime_evidence}")
