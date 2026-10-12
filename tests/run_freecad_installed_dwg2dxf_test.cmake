if(NOT DEFINED BUILD_DIR OR NOT DEFINED BUILD_DWG2DXF
        OR NOT DEFINED INSTALL_BINDIR OR NOT DEFINED INSTALL_LIBDIR
        OR NOT DEFINED EXECUTABLE_SUFFIX OR NOT DEFINED SAMPLE
        OR NOT DEFINED RTEXT_SAMPLE OR NOT DEFINED MPOLYGON_SAMPLE
        OR NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR "Installed FreeCAD CLI test arguments are incomplete")
endif()

foreach(_install_dir IN ITEMS "${INSTALL_BINDIR}" "${INSTALL_LIBDIR}")
    if(IS_ABSOLUTE "${_install_dir}"
            OR "${_install_dir}" MATCHES "(^|/)\\.\\.(/|$)")
        message(FATAL_ERROR
            "Refusing test install with non-relocatable destination '${_install_dir}'")
    endif()
endforeach()
if(NOT EXISTS "${BUILD_DWG2DXF}")
    message(FATAL_ERROR "Build-tree converter does not exist: ${BUILD_DWG2DXF}")
endif()

string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef _nonce)
set(_test_root "${OUTPUT_DIR}/FreeCAD installed CLI test-${_nonce}")
if(EXISTS "${_test_root}")
    message(FATAL_ERROR "Refusing to reuse existing test directory '${_test_root}'")
endif()
set(_install_prefix "${_test_root}/installed prefix")
set(_test_output "${_test_root}/conversion outputs")
file(MAKE_DIRECTORY "${_test_root}" "${_test_output}")

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
        "Isolated dwg2dxf install failed (${_install_result}); diagnostics retained under '${_test_root}'\n"
        "${_install_stdout}\n${_install_stderr}")
endif()

set(_installed_dwg2dxf
    "${_install_prefix}/${INSTALL_BINDIR}/dwg2dxf${EXECUTABLE_SUFFIX}")
if(NOT EXISTS "${_installed_dwg2dxf}")
    message(FATAL_ERROR
        "Install did not produce the expected FreeCAD converter '${_installed_dwg2dxf}'")
endif()
file(REAL_PATH "${_installed_dwg2dxf}" _installed_real_path)
file(REAL_PATH "${BUILD_DWG2DXF}" _build_real_path)
if("${_installed_real_path}" STREQUAL "${_build_real_path}")
    message(FATAL_ERROR
        "Installed CLI resolves to the build-tree executable '${_build_real_path}'")
endif()

# FreeCAD inherits its process environment. Remove common developer loader
# overrides so a shared build must resolve libdxfrw from the install itself.
if(CMAKE_HOST_WIN32)
    if(NOT DEFINED ENV{SystemRoot} OR "$ENV{SystemRoot}" STREQUAL "")
        message(FATAL_ERROR "SystemRoot is required to construct a clean Windows PATH")
    endif()
    set(_clean_path "$ENV{SystemRoot}/System32")
else()
    set(_clean_path "/usr/bin:/bin")
endif()
set(_clean_environment_command
    "${CMAKE_COMMAND}" -E env
    --unset=DYLD_LIBRARY_PATH
    --unset=DYLD_FALLBACK_LIBRARY_PATH
    --unset=DYLD_FRAMEWORK_PATH
    --unset=DYLD_FALLBACK_FRAMEWORK_PATH
    --unset=DYLD_INSERT_LIBRARIES
    --unset=LD_LIBRARY_PATH
    --unset=LD_PRELOAD
    --unset=LD_AUDIT
    "PATH=${_clean_path}"
    "${CMAKE_COMMAND}"
    "-DDWG2DXF=${_installed_dwg2dxf}"
    "-DSAMPLE=${SAMPLE}"
    "-DRTEXT_SAMPLE=${RTEXT_SAMPLE}"
    "-DMPOLYGON_SAMPLE=${MPOLYGON_SAMPLE}"
    "-DOUTPUT_DIR=${_test_output}"
    -P "${CMAKE_CURRENT_LIST_DIR}/run_freecad_dwg2dxf_compat_test.cmake")
execute_process(
    COMMAND ${_clean_environment_command}
    RESULT_VARIABLE _compat_result
    OUTPUT_VARIABLE _compat_stdout
    ERROR_VARIABLE _compat_stderr
    TIMEOUT 120
)
if(NOT "${_compat_result}" STREQUAL "0")
    message(FATAL_ERROR
        "Installed FreeCAD CLI compatibility check failed (${_compat_result}); diagnostics retained under '${_test_root}'\n"
        "${_compat_stdout}\n${_compat_stderr}")
endif()

file(REMOVE_RECURSE "${_test_root}")
message(STATUS
    "Installed FreeCAD CLI passed from an isolated prefix with loader overrides unset")
