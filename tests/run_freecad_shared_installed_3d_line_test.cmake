foreach(_required IN ITEMS BUILD_DIR BUILD_DWG2DXF FREECADCMD DWGADD RECIPE
        IMPORT_CHECK_MACRO OPERATION INSTALL_BINDIR INSTALL_LIBDIR
        EXECUTABLE_SUFFIX OUTPUT_DIR)
    if(NOT DEFINED ${_required})
        message(FATAL_ERROR "${_required} is required")
    endif()
endforeach()
if(NOT BUILD_SHARED_LIBS)
    message(FATAL_ERROR "This FreeCAD install test requires BUILD_SHARED_LIBS=ON")
endif()
if(NOT EXISTS "${BUILD_DWG2DXF}" OR NOT EXISTS "${FREECADCMD}"
        OR NOT EXISTS "${DWGADD}" OR NOT EXISTS "${RECIPE}"
        OR NOT EXISTS "${IMPORT_CHECK_MACRO}")
    message(FATAL_ERROR "A shared FreeCAD install test input does not exist")
endif()
if(NOT OPERATION STREQUAL "open" AND NOT OPERATION STREQUAL "insert")
    message(FATAL_ERROR "OPERATION must be open or insert")
endif()
foreach(_install_dir IN ITEMS "${INSTALL_BINDIR}" "${INSTALL_LIBDIR}")
    if(IS_ABSOLUTE "${_install_dir}"
            OR "${_install_dir}" MATCHES "(^|/)\\.\\.(/|$)")
        message(FATAL_ERROR
            "Refusing test install with non-relocatable destination '${_install_dir}'")
    endif()
endforeach()

string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef _nonce)
set(_test_root "${OUTPUT_DIR}/FreeCAD shared installed test-${_nonce}")
if(EXISTS "${_test_root}")
    message(FATAL_ERROR "Refusing to reuse existing test directory '${_test_root}'")
endif()
set(_install_prefix "${_test_root}/isolated install prefix")
file(MAKE_DIRECTORY "${_test_root}")

set(_install_command "${CMAKE_COMMAND}" --install "${BUILD_DIR}"
    --prefix "${_install_prefix}")
if(DEFINED CMAKE_CONFIG AND NOT "${CMAKE_CONFIG}" STREQUAL "")
    list(APPEND _install_command --config "${CMAKE_CONFIG}")
endif()
execute_process(
    COMMAND ${_install_command}
    RESULT_VARIABLE _install_result
    OUTPUT_VARIABLE _install_stdout
    ERROR_VARIABLE _install_stderr
    TIMEOUT 120
)
if(NOT "${_install_result}" STREQUAL "0")
    message(FATAL_ERROR
        "Isolated shared install failed (${_install_result}); diagnostics retained under '${_test_root}'\n"
        "${_install_stdout}\n${_install_stderr}")
endif()

set(_installed_dwg2dxf
    "${_install_prefix}/${INSTALL_BINDIR}/dwg2dxf${EXECUTABLE_SUFFIX}")
if(NOT EXISTS "${_installed_dwg2dxf}")
    message(FATAL_ERROR
        "Install did not produce the expected FreeCAD converter '${_installed_dwg2dxf}'")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        "-DFREECADCMD=${FREECADCMD}"
        "-DDWG2DXF=${_installed_dwg2dxf}"
        "-DBUILD_DWG2DXF=${BUILD_DWG2DXF}"
        "-DDWGADD=${DWGADD}"
        "-DRECIPE=${RECIPE}"
        "-DIMPORT_CHECK_MACRO=${IMPORT_CHECK_MACRO}"
        "-DOPERATION=${OPERATION}"
        -DDWG_CONVERSION_MODE=1
        -DALLOW_INSTALLED_RPATH_REWRITE=ON
        -DBUILD_SHARED_LIBS=ON
        "-DINSTALL_PREFIX=${_install_prefix}"
        -P "${CMAKE_CURRENT_LIST_DIR}/run_freecad_installed_3d_line_test.cmake"
    RESULT_VARIABLE _freecad_result
    OUTPUT_VARIABLE _freecad_stdout
    ERROR_VARIABLE _freecad_stderr
    TIMEOUT 180
)
if(NOT "${_freecad_result}" STREQUAL "0")
    message(FATAL_ERROR
        "FreeCAD ${OPERATION} failed with the isolated shared install (${_freecad_result}); "
        "diagnostics retained under '${_test_root}'\n"
        "${_freecad_stdout}\n${_freecad_stderr}")
endif()

file(REMOVE_RECURSE "${_test_root}")
message(STATUS
    "FreeCAD ${OPERATION} passed using the shared dwg2dxf installed at '${_installed_dwg2dxf}'\n"
    "${_freecad_stdout}\n${_freecad_stderr}")
