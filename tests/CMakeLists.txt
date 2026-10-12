# DWG read tests for libdxfrw.
#
# Tests invoke the dwg2dxf binary against sample .dwg files and verify
# the produced .dxf is non-empty and structurally well-formed.
#
# Sample files live in tests/samples/<AC_VERSION>/. A test is registered
# for every .dwg under each version directory at configure time. Drop a
# new .dwg in to register a new test on next CMake reconfigure.
#
# Each registered test cross-converts to multiple output DXF versions
# (R12 / v2000 / v2010) so the writer side gets exercised too. The harness
# verifies:
#   1. The sample's first 6 bytes literally match its declared AC version.
#   2. dwg2dxf returns 0 and produces a non-empty DXF.
#   3. The DXF contains required structural markers (SECTION / ENDSEC / EOF).
#   4. The DXF contains at least one entity OR table block.
#
# Test labels: "dwg_read", the AC version (AC1021 / AC1024 / ...), and
# the output version (out_R12 / out_v2000 / out_v2010).
#
# Run:
#   cd build && ctest -L dwg_read --output-on-failure
#   cd build && ctest -L AC1024 --output-on-failure
#   cd build && ctest -L out_v2010 --output-on-failure

if(NOT TARGET dwg2dxf)
    message(STATUS "tests/: dwg2dxf target not built; skipping DWG read tests")
    return()
endif()

# Optional external qualification for AC1015 SAT-v1 modeler carriers. Neither
# the DWG nor independent reference DXF is vendored; callers provide paths to
# their local evidence. The generated input/output copies remain in build/.
option(LIBDXFRW_ENABLE_EXTERNAL_SAT_V1_CLI_CONTROL
    "Enable external AC1015 SAT-v1 dwg2dxf comparison" OFF)
set(LIBDXFRW_SAT_V1_DWG "" CACHE FILEPATH
    "External AC1015 DWG with a qualified SAT-v1 3DSOLID carrier")
set(LIBDXFRW_SAT_V1_REFERENCE_DXF "" CACHE FILEPATH
    "Independent DXF export of the same AC1015 SAT-v1 DWG")
if(LIBDXFRW_ENABLE_EXTERNAL_SAT_V1_CLI_CONTROL)
    foreach(_sat_v1_input IN ITEMS
            LIBDXFRW_SAT_V1_DWG LIBDXFRW_SAT_V1_REFERENCE_DXF)
        if(NOT EXISTS "${${_sat_v1_input}}")
            message(FATAL_ERROR
                "Set ${_sat_v1_input} to an existing external evidence file")
        endif()
    endforeach()
    add_executable(libdxfrw_modeler_sat_dxf_compare
        ${CMAKE_CURRENT_SOURCE_DIR}/modeler_sat_dxf_compare.cpp)
    target_compile_features(libdxfrw_modeler_sat_dxf_compare PRIVATE cxx_std_17)
    target_compile_options(libdxfrw_modeler_sat_dxf_compare
        PRIVATE ${LIBDXFRW_WARNING_FLAGS})
    add_test(
        NAME dwg2dxf_freecad_external_sat_v1_cli
        COMMAND ${CMAKE_COMMAND}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            -DSAMPLE=${LIBDXFRW_SAT_V1_DWG}
            -DREFERENCE_DXF=${LIBDXFRW_SAT_V1_REFERENCE_DXF}
            -DCOMPARATOR=$<TARGET_FILE:libdxfrw_modeler_sat_dxf_compare>
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_sat_v1_external_cli_test.cmake.in
    )
    set_tests_properties(dwg2dxf_freecad_external_sat_v1_cli PROPERTIES
        LABELS "freecad;cli;external;integration"
        TIMEOUT 120)
endif()

# Keep the no-version FreeCAD CLI revision policy independently tested from
# full DWG input availability. The source-version mapping is pure policy.
add_executable(libdxfrw_dwg2dxf_version_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf_version_tests.cpp
)
target_compile_features(libdxfrw_dwg2dxf_version_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg2dxf_version_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg2dxf_version_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf
    ${CMAKE_SOURCE_DIR}/src
)
add_test(NAME dwg2dxf_version_policy
    COMMAND libdxfrw_dwg2dxf_version_tests)
set_tests_properties(dwg2dxf_version_policy PROPERTIES
    LABELS "cli;fast")

# Keep the concrete dwg2dxf adapter's cross-callback image link alive. The
# source DXF is generated at test time, passed through the real CLI twice, and
# read back through the same adapter; no image/DXF fixture is committed.
add_executable(libdxfrw_dwg2dxf_adapter_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf_adapter_tests.cpp
    ${CMAKE_SOURCE_DIR}/dwg2dxf/dx_iface.cpp
)
target_compile_features(libdxfrw_dwg2dxf_adapter_tests PRIVATE cxx_std_17)
target_compile_options(libdxfrw_dwg2dxf_adapter_tests
    PRIVATE ${LIBDXFRW_WARNING_FLAGS})
target_include_directories(libdxfrw_dwg2dxf_adapter_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dwg2dxf
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(libdxfrw_dwg2dxf_adapter_tests PRIVATE dxfrw)
add_test(
    NAME dwg2dxf_image_path_cli
    COMMAND ${CMAKE_COMMAND}
        "-DADAPTER_TEST=$<TARGET_FILE:libdxfrw_dwg2dxf_adapter_tests>"
        "-DDWG2DXF=$<TARGET_FILE:dwg2dxf>"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg2dxf_image_path_test.cmake
)
set_tests_properties(dwg2dxf_image_path_cli PROPERTIES
    LABELS "cli;fast;dxf;adapter"
    TIMEOUT 30
)

# Dynamic-block objects are retained by the converter adapter and must block
# publication until a semantics-qualified DXF writer exists.
add_test(
    NAME dwg2dxf_dynamic_block_fail_closed
    COMMAND libdxfrw_dwg2dxf_adapter_tests --reject-dynamic-block
)
set_tests_properties(dwg2dxf_dynamic_block_fail_closed PROPERTIES
    LABELS "fast;dxf;adapter;dynamic-block"
    TIMEOUT 10
)

# Keep the concrete adapter's modern custom-class 3DLINE subtype and payload
# through generated DXF, two exact FreeCAD-argv CLI passes, and public readback.
add_test(
    NAME dwg2dxf_3dline_adapter_cli
    COMMAND ${CMAKE_COMMAND}
        "-DADAPTER_TEST=$<TARGET_FILE:libdxfrw_dwg2dxf_adapter_tests>"
        "-DDWG2DXF=$<TARGET_FILE:dwg2dxf>"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
        "-DCASE_NAME=3dline-adapter"
        "-DGENERATE_MODE=--generate-3dline"
        "-DVERIFY_MODE=--verify-3dline"
        "-DNEGATIVE_MODE=--reject-3dline-ac1014"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg2dxf_image_path_test.cmake
)
set_tests_properties(dwg2dxf_3dline_adapter_cli PROPERTIES
    LABELS "cli;fast;dxf;adapter;3d"
    TIMEOUT 30
)

# Exercise the same generated adapter subtype through binary-DXF input and
# output, with public readback after two exact CLI conversion passes.
add_test(
    NAME dwg2dxf_3dline_binary_adapter_cli
    COMMAND ${CMAKE_COMMAND}
        "-DADAPTER_TEST=$<TARGET_FILE:libdxfrw_dwg2dxf_adapter_tests>"
        "-DDWG2DXF=$<TARGET_FILE:dwg2dxf>"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
        "-DCASE_NAME=3dline-binary-adapter"
        "-DGENERATE_MODE=--generate-3dline-binary"
        "-DVERIFY_MODE=--verify-3dline"
        "-DCONVERSION_OPTIONS=-b"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg2dxf_image_path_test.cmake
)
set_tests_properties(dwg2dxf_3dline_binary_adapter_cli PROPERTIES
    LABELS "cli;fast;dxf;adapter;3d;binary"
    TIMEOUT 30
)

# Match FreeCAD's Draft/importDWG.py subprocess contract without requiring a
# FreeCAD installation. The DWG fixture is already tracked in this repository.
set(_freecad_cli_sample
    ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/ordinary_enc_AC1027.dwg
)
set(_freecad_cli_rtext_sample
    ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/rtext_arctext.dwg
)
set(_freecad_cli_mpolygon_sample
    ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/mpolygon_solid.dwg
)
if(EXISTS "${_freecad_cli_sample}"
        AND EXISTS "${_freecad_cli_rtext_sample}"
        AND EXISTS "${_freecad_cli_mpolygon_sample}")
    add_test(
        NAME dwg2dxf_freecad_cli_compat
        COMMAND ${CMAKE_COMMAND}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            -DSAMPLE=${_freecad_cli_sample}
            -DRTEXT_SAMPLE=${_freecad_cli_rtext_sample}
            -DMPOLYGON_SAMPLE=${_freecad_cli_mpolygon_sample}
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_dwg2dxf_compat_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_cli_compat PROPERTIES
        LABELS "dwg_read;freecad;cli;fast"
    )

    # Exercise the artifact FreeCAD actually launches, not just the build-tree
    # executable. Keep this test inside the chosen install prefix and skip
    # non-relocatable GNUInstallDirs layouts rather than risking a host install.
    set(_freecad_install_dirs_safe TRUE)
    foreach(_install_dir IN ITEMS
            "${CMAKE_INSTALL_BINDIR}" "${CMAKE_INSTALL_LIBDIR}")
        if(IS_ABSOLUTE "${_install_dir}"
                OR "${_install_dir}" MATCHES "(^|/)\\.\\.(/|$)")
            set(_freecad_install_dirs_safe FALSE)
        endif()
    endforeach()
    if(_freecad_install_dirs_safe)
        add_test(
            NAME dwg2dxf_freecad_installed_cli_compat
            COMMAND ${CMAKE_COMMAND}
                "-DBUILD_DIR=${CMAKE_BINARY_DIR}"
                "-DBUILD_DWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DINSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}"
                "-DINSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}"
                "-DEXECUTABLE_SUFFIX=${CMAKE_EXECUTABLE_SUFFIX}"
                "-DSAMPLE=${_freecad_cli_sample}"
                "-DRTEXT_SAMPLE=${_freecad_cli_rtext_sample}"
                "-DMPOLYGON_SAMPLE=${_freecad_cli_mpolygon_sample}"
                "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
                "-DCMAKE_CONFIG=$<CONFIG>"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_installed_dwg2dxf_test.cmake"
        )
        set_tests_properties(dwg2dxf_freecad_installed_cli_compat PROPERTIES
            LABELS "freecad;cli;fast;installed"
            TIMEOUT 180
        )
    else()
        message(STATUS
            "Skipping installed FreeCAD CLI test: GNUInstallDirs destinations are absolute or escape the install prefix")
    endif()
endif()

# The core codec can replay raw sections, but dwg2dxf must also retain them
# through its concrete DXF import/export adapter. Generate the tiny opaque
# ACDSDATA-shaped control in the build tree; it is not a schema-validity oracle.
add_test(
    NAME dwg2dxf_raw_dxf_section_passthrough
    COMMAND ${CMAKE_COMMAND}
        -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
        -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg2dxf_raw_dxf_section_test.cmake
)
set_tests_properties(dwg2dxf_raw_dxf_section_passthrough PROPERTIES
    LABELS "cli;fast;dxf"
    TIMEOUT 20
)

# Optional test of the installed converter from FreeCAD's actual desktop
# process. It uses only a tracked ordinary DWG and is deliberately disabled
# by default; the normal inner loop remains the fast, FreeCAD-independent CLI
# contract test above.
option(LIBDXFRW_ENABLE_FREECAD_DESKTOP_CONTROL
    "Enable the opt-in FreeCAD GUI dwg2dxf open/insert smoke" OFF)
set(LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE "" CACHE FILEPATH
    "Installed dwg2dxf executable to qualify inside FreeCAD")
set(LIBDXFRW_FREECAD_GUI_EXECUTABLE "" CACHE FILEPATH
    "FreeCAD GUI executable (required outside macOS)")
set(LIBDXFRW_FREECAD_APP_BUNDLE "" CACHE PATH
    "FreeCAD .app bundle path (required on macOS)")
if(LIBDXFRW_ENABLE_FREECAD_DESKTOP_CONTROL)
    if(NOT EXISTS "${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}")
        message(FATAL_ERROR
            "LIBDXFRW_ENABLE_FREECAD_DESKTOP_CONTROL requires the installed "
            "LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE path")
    endif()
    if(APPLE)
        if(NOT IS_DIRECTORY "${LIBDXFRW_FREECAD_APP_BUNDLE}")
            message(FATAL_ERROR
                "On macOS, set LIBDXFRW_FREECAD_APP_BUNDLE to FreeCAD.app")
        endif()
    elseif(NOT EXISTS "${LIBDXFRW_FREECAD_GUI_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECAD_GUI_EXECUTABLE to FreeCAD's desktop executable")
    endif()
    add_test(
        NAME dwg2dxf_freecad_desktop_open_insert
        COMMAND ${CMAKE_COMMAND}
            -DFREECAD_DWG2DXF_EXECUTABLE=${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}
            -DBUILD_DWG2DXF_EXECUTABLE=$<TARGET_FILE:dwg2dxf>
            -DSAMPLE=${_freecad_cli_sample}
            -DIMPORT_CHECK_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_dwg2dxf_import_check.FCMacro
            -DDESKTOP_CHECK_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_dwg2dxf_desktop_check.FCMacro
            -DFREECAD_GUI_EXECUTABLE=${LIBDXFRW_FREECAD_GUI_EXECUTABLE}
            -DFREECAD_APP_BUNDLE=${LIBDXFRW_FREECAD_APP_BUNDLE}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_desktop_dwg_import_check.cmake
    )
    set_tests_properties(dwg2dxf_freecad_desktop_open_insert PROPERTIES
        LABELS "freecad;desktop;external;integration"
        TIMEOUT 240
    )
endif()

# Optional pinned FreeCAD runtime for the external AC1015 SAT-v1 witness.
# Reuses the external sample/reference configured for the CLI comparison and
# the existing import macro; never downloads or vendors a DWG/DXF fixture.
option(LIBDXFRW_ENABLE_FREECAD_SAT_V1_RUNTIME_CONTROL
    "Enable the opt-in FreeCAD SAT-v1 open/insert handoff check" OFF)
set(LIBDXFRW_FREECADCMD_EXECUTABLE "" CACHE FILEPATH
    "FreeCAD freecadcmd executable for the SAT-v1 runtime control")
if(LIBDXFRW_ENABLE_FREECAD_SAT_V1_RUNTIME_CONTROL)
    if(NOT LIBDXFRW_ENABLE_EXTERNAL_SAT_V1_CLI_CONTROL)
        message(FATAL_ERROR
            "The FreeCAD SAT-v1 runtime control also requires "
            "LIBDXFRW_ENABLE_EXTERNAL_SAT_V1_CLI_CONTROL=ON and its external "
            "DWG/reference paths")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECADCMD_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECADCMD_EXECUTABLE to FreeCAD's freecadcmd")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE to the installed "
            "libdxfrw dwg2dxf binary, not the build-tree target")
    endif()
    foreach(_operation IN ITEMS open insert)
        add_test(
            NAME dwg2dxf_freecad_sat_v1_${_operation}
            COMMAND ${CMAKE_COMMAND}
                "-DFREECADCMD=${LIBDXFRW_FREECADCMD_EXECUTABLE}"
                "-DDWG2DXF=${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}"
                "-DBUILD_DWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DSAMPLE=${LIBDXFRW_SAT_V1_DWG}"
                "-DREFERENCE_DXF=${LIBDXFRW_SAT_V1_REFERENCE_DXF}"
                "-DCOMPARATOR=$<TARGET_FILE:libdxfrw_modeler_sat_dxf_compare>"
                "-DIMPORT_CHECK_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_dwg2dxf_import_check.FCMacro"
                "-DOPERATION=${_operation}"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_sat_v1_runtime_test.cmake.in
        )
        set_tests_properties(dwg2dxf_freecad_sat_v1_${_operation}
            PROPERTIES
                LABELS "freecad;modeler;external;integration"
                TIMEOUT 240)
    endforeach()
endif()

# External AC1027 ACDSDATA handoff qualification. The caller supplies the DWG,
# installed converter, its expected digest, and a pinned FreeCAD 1.1.3 runtime.
# No drawing or generated output is added to the source tree. This deliberately
# checks converter handoff plus FreeCAD's current unsupported-3DSOLID outcome,
# not FreeCAD B-rep support. An optional libdxfrw DXF baseline enables the
# stricter, separately maintained ACDSDATA/proxy/SAB round-trip checker.
option(LIBDXFRW_ENABLE_FREECAD_ACDS_RUNTIME_CONTROL
    "Enable the opt-in external AC1027 ACDSDATA FreeCAD open/insert handoff check" OFF)
set(LIBDXFRW_ACDS_DWG "" CACHE FILEPATH
    "Caller-supplied AC1027 DWG with one 3DSOLID and ACDSDATA")
set(LIBDXFRW_ACDS_INSTALLED_DWG2DXF "" CACHE FILEPATH
    "Installed libdxfrw dwg2dxf executable to qualify in FreeCAD")
set(LIBDXFRW_ACDS_INSTALLED_SHA256 "" CACHE STRING
    "Expected SHA-256 of the installed ACDSDATA dwg2dxf executable")
set(LIBDXFRW_ACDS_FREECAD_REVISION "" CACHE STRING
    "Exact FreeCAD 1.1.3 App.Version()[7] revision to qualify")
set(LIBDXFRW_ACDS_BASELINE_DXF "" CACHE FILEPATH
    "Optional caller-supplied libdxfrw AC1027 DXF for strict ACDSDATA comparison")
if(LIBDXFRW_ENABLE_FREECAD_ACDS_RUNTIME_CONTROL)
    foreach(_acds_path IN ITEMS LIBDXFRW_ACDS_DWG
            LIBDXFRW_ACDS_INSTALLED_DWG2DXF LIBDXFRW_FREECADCMD_EXECUTABLE)
        if(NOT EXISTS "${${_acds_path}}")
            message(FATAL_ERROR
                "FreeCAD ACDSDATA control requires existing ${_acds_path}")
        endif()
    endforeach()
    string(LENGTH "${LIBDXFRW_ACDS_INSTALLED_SHA256}" _acds_sha_length)
    if(NOT _acds_sha_length EQUAL 64
            OR NOT LIBDXFRW_ACDS_INSTALLED_SHA256 MATCHES "^[0-9a-fA-F]+$")
        message(FATAL_ERROR
            "Set LIBDXFRW_ACDS_INSTALLED_SHA256 to the installed binary's exact SHA-256")
    endif()
    string(LENGTH "${LIBDXFRW_ACDS_FREECAD_REVISION}" _acds_revision_length)
    if(NOT _acds_revision_length EQUAL 40
            OR NOT LIBDXFRW_ACDS_FREECAD_REVISION MATCHES "^[0-9a-fA-F]+$")
        message(FATAL_ERROR
            "Set LIBDXFRW_ACDS_FREECAD_REVISION to the pinned FreeCAD 1.1.3 Git revision")
    endif()
    if(LIBDXFRW_ACDS_BASELINE_DXF)
        if(NOT EXISTS "${LIBDXFRW_ACDS_BASELINE_DXF}")
            message(FATAL_ERROR
                "LIBDXFRW_ACDS_BASELINE_DXF must name an existing external DXF")
        endif()
        find_package(Python3 COMPONENTS Interpreter REQUIRED)
    endif()
    foreach(_operation IN ITEMS open insert)
        add_test(
            NAME dwg2dxf_freecad_acds_${_operation}_handoff
            COMMAND ${CMAKE_COMMAND}
                "-DFREECADCMD=${LIBDXFRW_FREECADCMD_EXECUTABLE}"
                "-DDWG2DXF=${LIBDXFRW_ACDS_INSTALLED_DWG2DXF}"
                "-DBUILD_DWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DSAMPLE=${LIBDXFRW_ACDS_DWG}"
                "-DEXPECTED_SHA256=${LIBDXFRW_ACDS_INSTALLED_SHA256}"
                "-DFREECAD_REVISION=${LIBDXFRW_ACDS_FREECAD_REVISION}"
                "-DBASELINE_DXF=${LIBDXFRW_ACDS_BASELINE_DXF}"
                "-DPYTHON=${Python3_EXECUTABLE}"
                "-DACDS_CHECKER=${CMAKE_CURRENT_SOURCE_DIR}/check_acds_roundtrip.py"
                "-DIMPORT_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_dwg2dxf_acds_check.FCMacro"
                "-DOPERATION=${_operation}"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_acds_runtime_test.cmake
        )
        set_tests_properties(dwg2dxf_freecad_acds_${_operation}_handoff
            PROPERTIES
                LABELS "freecad;acds;AC1027;external;integration"
                TIMEOUT 240)
    endforeach()
endif()

# Optional failure-path qualification through FreeCAD's real Draft wrapper.
# FreeCAD currently treats output-file existence as conversion success, so this
# confirms that a failing installed converter cannot hand off a partial DXF.
# The malformed DWG is created locally beneath system temp; FreeCAD stays out
# of default CI.
option(LIBDXFRW_ENABLE_FREECAD_FAILURE_CONTROL
    "Enable the opt-in FreeCAD DWG conversion failure-path check" OFF)
if(LIBDXFRW_ENABLE_FREECAD_FAILURE_CONTROL)
    if(NOT EXISTS "${LIBDXFRW_FREECADCMD_EXECUTABLE}")
        message(FATAL_ERROR
            "LIBDXFRW_ENABLE_FREECAD_FAILURE_CONTROL requires "
            "LIBDXFRW_FREECADCMD_EXECUTABLE")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}")
        message(FATAL_ERROR
            "LIBDXFRW_ENABLE_FREECAD_FAILURE_CONTROL requires the installed "
            "LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE")
    endif()
    add_test(
        NAME dwg2dxf_freecad_failed_conversion
        COMMAND ${CMAKE_COMMAND}
            "-DFREECADCMD=${LIBDXFRW_FREECADCMD_EXECUTABLE}"
            "-DDWG2DXF=${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}"
            "-DBUILD_DWG2DXF=$<TARGET_FILE:dwg2dxf>"
            "-DIMPORT_CHECK_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_dwg2dxf_import_check.FCMacro"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_dwg2dxf_failure_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_failed_conversion PROPERTIES
        LABELS "freecad;external;integration"
        TIMEOUT 120
    )
endif()

# Bridge the positive nonzero-Z FreeCAD geometry control through the installed
# artifact. The ordinary fast CLI checks above remain dependency-free.
option(LIBDXFRW_ENABLE_FREECAD_INSTALLED_3D_LINE_CONTROL
    "Enable installed dwg2dxf FreeCAD 3D LINE open/insert checks" OFF)
if(LIBDXFRW_ENABLE_FREECAD_INSTALLED_3D_LINE_CONTROL)
    find_program(LIBDXFRW_DWGADD_EXECUTABLE NAMES dwgadd)
    if(NOT LIBDXFRW_DWGADD_EXECUTABLE)
        message(FATAL_ERROR
            "LIBDXFRW_ENABLE_FREECAD_INSTALLED_3D_LINE_CONTROL requires LibreDWG dwgadd")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECADCMD_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECADCMD_EXECUTABLE to FreeCAD's freecadcmd")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE to the installed dwg2dxf")
    endif()

    foreach(_operation IN ITEMS open insert)
        add_test(
            NAME dwg2dxf_freecad_installed_3d_line_${_operation}
            COMMAND ${CMAKE_COMMAND}
                "-DFREECADCMD=${LIBDXFRW_FREECADCMD_EXECUTABLE}"
                "-DDWG2DXF=${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}"
                "-DBUILD_DWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DDWGADD=${LIBDXFRW_DWGADD_EXECUTABLE}"
                "-DRECIPE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/ac1015_3d_line_control.dwgadd"
                "-DIMPORT_CHECK_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_dwg2dxf_import_check.FCMacro"
                "-DOPERATION=${_operation}"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_installed_3d_line_test.cmake
        )
        set_tests_properties(dwg2dxf_freecad_installed_3d_line_${_operation}
            PROPERTIES
                LABELS "freecad;installed;generated;integration"
                TIMEOUT 180)
    endforeach()
endif()

# Native Linux/Windows qualification can reuse the exact LibreCAD-tracked
# AC1027 three-LINE fixture. This avoids requiring an external dwgadd build or
# adding another DWG to the repository. Keep it opt-in: it launches the pinned
# FreeCAD consumer twice (open and insert) and is intended for manual platform
# qualification, not every fast CI run.
option(LIBDXFRW_ENABLE_FREECAD_TRACKED_AC1027_LINE_CONTROL
    "Enable pinned FreeCAD checks using the tracked AC1027 three-LINE DWG" OFF)
if(LIBDXFRW_ENABLE_FREECAD_TRACKED_AC1027_LINE_CONTROL)
    if(NOT EXISTS "${LIBDXFRW_FREECADCMD_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECADCMD_EXECUTABLE to FreeCAD's FreeCADCmd/freecadcmd")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE to the installed dwg2dxf")
    endif()
    if(NOT EXISTS "${_freecad_cli_sample}")
        message(FATAL_ERROR
            "The tracked ordinary_enc_AC1027.dwg fixture is required")
    endif()

    foreach(_operation IN ITEMS open insert)
        add_test(
            NAME dwg2dxf_freecad_tracked_ac1027_line_${_operation}
            COMMAND ${CMAKE_COMMAND}
                "-DFREECADCMD=${LIBDXFRW_FREECADCMD_EXECUTABLE}"
                "-DDWG2DXF=${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}"
                "-DBUILD_DWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DSAMPLE=${_freecad_cli_sample}"
                -DEXPECTED_INPUT_SHA256=a0ebf245e570bf0dc337c696330b7ea883feaebb7c4e781de8d77ac723815f83
                -DEXPECTED_MAGIC_HEX=414331303237
                "-DIMPORT_CHECK_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_dwg2dxf_import_check.FCMacro"
                "-DOPERATION=${_operation}"
                -DINPUT_PATH_UNICODE=ON
                -DEXPECTED_FREECAD_REVISION=145529fe741292ff0b3977a01195bf0247425794
                -DEXPECTED_LINE_BOUNDS=[[1,2,0,3,4,0],[5,6,0,7,8,0],[9,10,0,11,12,0]]
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_installed_3d_line_test.cmake
        )
        set_tests_properties(dwg2dxf_freecad_tracked_ac1027_line_${_operation}
            PROPERTIES
                LABELS "freecad;installed;generated;integration;AC1027;platform"
                TIMEOUT 180)
    endforeach()
endif()

# Exercise the current shared build after CMake installs it into a relocatable
# prefix, then reuse the installed-artifact FreeCAD geometry harness. CMake's
# install step rewrites the executable RPATH, so this must be a distinct
# attribution path from the byte-identical static/install test above.
option(LIBDXFRW_ENABLE_FREECAD_SHARED_INSTALLED_3D_LINE_CONTROL
    "Enable FreeCAD open/insert checks against this shared build's isolated install" OFF)
if(LIBDXFRW_ENABLE_FREECAD_SHARED_INSTALLED_3D_LINE_CONTROL)
    if(NOT BUILD_SHARED_LIBS OR NOT UNIX)
        message(FATAL_ERROR
            "LIBDXFRW_ENABLE_FREECAD_SHARED_INSTALLED_3D_LINE_CONTROL requires a shared Unix build")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECADCMD_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECADCMD_EXECUTABLE to FreeCAD's freecadcmd")
    endif()
    find_program(LIBDXFRW_DWGADD_EXECUTABLE NAMES dwgadd)
    if(NOT LIBDXFRW_DWGADD_EXECUTABLE)
        message(FATAL_ERROR
            "LIBDXFRW_ENABLE_FREECAD_SHARED_INSTALLED_3D_LINE_CONTROL requires LibreDWG dwgadd")
    endif()

    foreach(_operation IN ITEMS open insert)
        add_test(
            NAME dwg2dxf_freecad_shared_installed_3d_line_${_operation}
            COMMAND ${CMAKE_COMMAND}
                "-DBUILD_DIR=${CMAKE_BINARY_DIR}"
                "-DBUILD_DWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DFREECADCMD=${LIBDXFRW_FREECADCMD_EXECUTABLE}"
                "-DDWGADD=${LIBDXFRW_DWGADD_EXECUTABLE}"
                "-DRECIPE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/ac1015_3d_line_control.dwgadd"
                "-DIMPORT_CHECK_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_dwg2dxf_import_check.FCMacro"
                "-DOPERATION=${_operation}"
                "-DINSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}"
                "-DINSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}"
                "-DEXECUTABLE_SUFFIX=${CMAKE_EXECUTABLE_SUFFIX}"
                "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
                "-DCMAKE_CONFIG=$<CONFIG>"
                -DBUILD_SHARED_LIBS=ON
                -P "${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_shared_installed_3d_line_test.cmake"
        )
        set_tests_properties(dwg2dxf_freecad_shared_installed_3d_line_${_operation}
            PROPERTIES
                LABELS "freecad;shared;installed;generated;integration"
                TIMEOUT 300)
    endforeach()
endif()

# Qualify the ordinary FreeCAD Automatic preference separately. Unlike the
# LibreDWG-only controls above, keep real ODA/QCAD resolvers enabled and assert
# that the installed dwg2dxf succeeds before either fallback is launched.
option(LIBDXFRW_ENABLE_FREECAD_AUTOMATIC_3D_LINE_CONTROL
    "Enable FreeCAD Automatic-mode dwg2dxf 3D LINE open/insert checks" OFF)
if(LIBDXFRW_ENABLE_FREECAD_AUTOMATIC_3D_LINE_CONTROL)
    find_program(LIBDXFRW_DWGADD_EXECUTABLE NAMES dwgadd)
    if(NOT LIBDXFRW_DWGADD_EXECUTABLE)
        message(FATAL_ERROR
            "LIBDXFRW_ENABLE_FREECAD_AUTOMATIC_3D_LINE_CONTROL requires LibreDWG dwgadd")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECADCMD_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECADCMD_EXECUTABLE to FreeCAD's freecadcmd")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE to the installed dwg2dxf")
    endif()

    foreach(_operation IN ITEMS open insert)
        add_test(
            NAME dwg2dxf_freecad_automatic_3d_line_${_operation}
            COMMAND ${CMAKE_COMMAND}
                "-DFREECADCMD=${LIBDXFRW_FREECADCMD_EXECUTABLE}"
                "-DDWG2DXF=${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}"
                "-DBUILD_DWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DDWGADD=${LIBDXFRW_DWGADD_EXECUTABLE}"
                "-DRECIPE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/ac1015_3d_line_control.dwgadd"
                "-DIMPORT_CHECK_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_dwg2dxf_import_check.FCMacro"
                "-DOPERATION=${_operation}"
                "-DDWG_CONVERSION_MODE=0"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_installed_3d_line_test.cmake
        )
        set_tests_properties(dwg2dxf_freecad_automatic_3d_line_${_operation}
            PROPERTIES
                LABELS "freecad;automatic;installed;generated;integration"
                TIMEOUT 180)
    endforeach()
endif()

# Optional AC1027 POINT qualification using a caller-supplied AutoCAD 2016
# sample. The DWG stays outside this repository; only test code is committed.
option(LIBDXFRW_ENABLE_FREECAD_AUTOCAD_POINT3D_CONTROL
    "Enable the external AutoCAD AC1027 3D POINT FreeCAD control" OFF)
set(LIBDXFRW_AUTOCAD_POINT3D_DWG "" CACHE FILEPATH
    "External AutoCAD 2016 AC1027 DWG containing POINT (50,50,50)")
if(LIBDXFRW_ENABLE_FREECAD_AUTOCAD_POINT3D_CONTROL)
    if(NOT EXISTS "${LIBDXFRW_AUTOCAD_POINT3D_DWG}")
        message(FATAL_ERROR
            "Set LIBDXFRW_AUTOCAD_POINT3D_DWG to the external point3d_2013.dwg")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECADCMD_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECADCMD_EXECUTABLE to FreeCAD's freecadcmd")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE to the installed dwg2dxf")
    endif()
    add_test(
        NAME dwg2dxf_freecad_autocad_ac1027_point3d
        COMMAND ${CMAKE_COMMAND}
            "-DFREECADCMD=${LIBDXFRW_FREECADCMD_EXECUTABLE}"
            "-DDWG2DXF=${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}"
            "-DBUILD_DWG2DXF=$<TARGET_FILE:dwg2dxf>"
            "-DSAMPLE=${LIBDXFRW_AUTOCAD_POINT3D_DWG}"
            "-DEXPECTED_SHA256=839297daa100c318d5afbfa128145d2c4782300650c86b2ab58b34cf6fe1aba4"
            "-DIMPORT_CHECK_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_dwg2dxf_3d_point_check.FCMacro"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_autocad_point3d_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_autocad_ac1027_point3d PROPERTIES
        LABELS "freecad;external;autocad;AC1027;integration"
        TIMEOUT 180)
endif()

# Optional locally generated DWG control for the FreeCAD nonzero-Z path.
# LibreDWG dwgadd and FreeCAD remain external integration tools, not required
# dependencies of the normal build/test configuration.
option(LIBDXFRW_ENABLE_DWGADD_FREECAD_CONTROL
    "Enable the LibreDWG-generated FreeCAD 3D LINE converter control" OFF)
if(LIBDXFRW_ENABLE_DWGADD_FREECAD_CONTROL)
    find_program(LIBDXFRW_DWGADD_EXECUTABLE NAMES dwgadd)
    if(NOT LIBDXFRW_DWGADD_EXECUTABLE)
        message(FATAL_ERROR
            "LIBDXFRW_ENABLE_DWGADD_FREECAD_CONTROL requires LibreDWG dwgadd")
    endif()
    add_test(
        NAME dwg2dxf_freecad_3d_line_cli
        COMMAND ${CMAKE_COMMAND}
            -DDWGADD=${LIBDXFRW_DWGADD_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            -DRECIPE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/ac1015_3d_line_control.dwgadd
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3d_line_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_line_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated")
    add_test(
        NAME dwg2dxf_freecad_3d_point_cli
        COMMAND ${CMAKE_COMMAND}
            -DDWGADD=${LIBDXFRW_DWGADD_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            -DRECIPE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/ac1015_3d_point_freecad_control.dwgadd
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3d_point_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_point_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated")
    add_test(
        NAME dwg2dxf_freecad_3d_arc_circle_cli
        COMMAND ${CMAKE_COMMAND}
            -DDWGADD=${LIBDXFRW_DWGADD_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            -DRECIPE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/ac1015_arc_circle_freecad_control.dwgadd
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3d_arc_circle_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_arc_circle_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated")
    add_test(
        NAME dwg2dxf_freecad_3d_ellipse_cli
        COMMAND ${CMAKE_COMMAND}
            -DDWGADD=${LIBDXFRW_DWGADD_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            -DRECIPE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/ac1015_ellipse_freecad_control.dwgadd
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3d_ellipse_dwggenerated_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_ellipse_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated")
    add_test(
        NAME dwg2dxf_freecad_3d_polyline_cli
        COMMAND ${CMAKE_COMMAND}
            -DDWGADD=${LIBDXFRW_DWGADD_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            -DRECIPE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/ac1015_3d_polyline_freecad_control.dwgadd
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3d_polyline_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_polyline_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated")
    foreach(_classic_family mesh pface pface_multiface)
        add_test(
            NAME dwg2dxf_freecad_3d_${_classic_family}_cli
            COMMAND ${CMAKE_COMMAND}
                -DDWGADD=${LIBDXFRW_DWGADD_EXECUTABLE}
                -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
                -DRECIPE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dwg/ac1015_${_classic_family}_freecad_control.dwgadd
                -DFAMILY=${_classic_family}
                -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_classic_polyline_cli_test.cmake
        )
        set_tests_properties(dwg2dxf_freecad_3d_${_classic_family}_cli PROPERTIES
            LABELS "dwg_read;freecad;cli;fast;generated")
    endforeach()
endif()

# Optional independent-writer DWG controls for FreeCAD entity routes.
# DXF sources are locally authored; ODA and FreeCAD remain external tools and
# are not dependencies of the normal build/test configuration.
option(LIBDXFRW_ENABLE_ODAFILECONVERTER_FREECAD_CONTROL
    "Enable ODA-generated FreeCAD 3D converter controls" OFF)
if(LIBDXFRW_ENABLE_ODAFILECONVERTER_FREECAD_CONTROL)
    find_program(LIBDXFRW_ODAFILECONVERTER_EXECUTABLE NAMES ODAFileConverter)
    find_program(LIBDXFRW_LIBREDWG_DWGREAD_EXECUTABLE NAMES dwgread)
    if(NOT LIBDXFRW_ODAFILECONVERTER_EXECUTABLE)
        message(FATAL_ERROR
            "LIBDXFRW_ENABLE_ODAFILECONVERTER_FREECAD_CONTROL requires ODAFileConverter; "
            "set LIBDXFRW_ODAFILECONVERTER_EXECUTABLE to its path")
    endif()
    add_test(
        NAME dwg2dxf_freecad_3d_insert_cli
        COMMAND ${CMAKE_COMMAND}
            -DODAFILECONVERTER=${LIBDXFRW_ODAFILECONVERTER_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            -DSOURCE_DXF=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dxf/ac1015_insert_freecad_control.dxf
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3d_insert_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_insert_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated;oda")
    add_test(
        NAME dwg2dxf_freecad_3d_minsert_cli
        COMMAND ${CMAKE_COMMAND}
            -DODAFILECONVERTER=${LIBDXFRW_ODAFILECONVERTER_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            "-DSOURCE_DXF=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dxf/ac1015_minsert_freecad_control.dxf"
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3d_minsert_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_minsert_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated;oda")
    add_test(
        NAME dwg2dxf_freecad_3d_spline_cli
        COMMAND ${CMAKE_COMMAND}
            -DODAFILECONVERTER=${LIBDXFRW_ODAFILECONVERTER_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            "-DSOURCE_DXF=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dxf/ac1015_nonplanar_spline_freecad_control.dxf"
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3d_spline_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_spline_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated;oda")
    add_test(
        NAME dwg2dxf_freecad_3d_helix_cli
        COMMAND ${CMAKE_COMMAND}
            -DODAFILECONVERTER=${LIBDXFRW_ODAFILECONVERTER_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            "-DSOURCE_DXF=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dxf/ac1015_helix_freecad_control.dxf"
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3d_helix_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_helix_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated;oda")
    add_test(
        NAME dwg2dxf_freecad_3d_ellipse_oda_cli
        COMMAND ${CMAKE_COMMAND}
            -DODAFILECONVERTER=${LIBDXFRW_ODAFILECONVERTER_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            "-DSOURCE_DXF=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dxf/ac1015_ellipse_freecad_control.dxf"
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3d_ellipse_oda_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_ellipse_oda_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated;oda")
    add_test(
        NAME dwg2dxf_freecad_3d_solid_oda_cli
        COMMAND ${CMAKE_COMMAND}
            -DODAFILECONVERTER=${LIBDXFRW_ODAFILECONVERTER_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            "-DSOURCE_DXF=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dxf/ac1015_solid_freecad_control.dxf"
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3d_solid_oda_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_solid_oda_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated;oda")
    add_test(
        NAME dwg2dxf_freecad_3dface_oda_cli
        COMMAND ${CMAKE_COMMAND}
            -DODAFILECONVERTER=${LIBDXFRW_ODAFILECONVERTER_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            "-DSOURCE_DXF=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dxf/ac1015_3dface_freecad_control.dxf"
            "-DLIBREDWG_DWGREAD=${LIBDXFRW_LIBREDWG_DWGREAD_EXECUTABLE}"
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3dface_oda_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3dface_oda_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated;oda")
    set(_freecad_3dface_oda_versions
        "ACAD2004|AC1018"
        "ACAD2007|AC1021"
        "ACAD2010|AC1024"
        "ACAD2013|AC1027"
        "ACAD2018|AC1032")
    foreach(_version_pair IN LISTS _freecad_3dface_oda_versions)
        string(REPLACE "|" ";" _version_fields "${_version_pair}")
        list(GET _version_fields 0 _oda_target_version)
        list(GET _version_fields 1 _expected_dwg_version)
        add_test(
            NAME dwg2dxf_freecad_3dface_${_expected_dwg_version}_oda_cli
            COMMAND ${CMAKE_COMMAND}
                -DODAFILECONVERTER=${LIBDXFRW_ODAFILECONVERTER_EXECUTABLE}
                -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
                "-DSOURCE_DXF=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dxf/ac1015_3dface_freecad_control.dxf"
                -DODA_TARGET_VERSION=${_oda_target_version}
                -DEXPECTED_DWG_VERSION=${_expected_dwg_version}
                "-DLIBREDWG_DWGREAD=${LIBDXFRW_LIBREDWG_DWGREAD_EXECUTABLE}"
                -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3dface_oda_cli_test.cmake
        )
        set_tests_properties(
            dwg2dxf_freecad_3dface_${_expected_dwg_version}_oda_cli PROPERTIES
            LABELS "dwg_read;freecad;cli;fast;generated;oda;${_expected_dwg_version}")
    endforeach()
    add_test(
        NAME dwg2dxf_freecad_2d_lwpolyline_oda_cli
        COMMAND ${CMAKE_COMMAND}
            -DODAFILECONVERTER=${LIBDXFRW_ODAFILECONVERTER_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            "-DSOURCE_DXF=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dxf/ac1015_lwpolyline_freecad_control.dxf"
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_2d_lwpolyline_oda_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_2d_lwpolyline_oda_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated;oda")
    add_test(
        NAME dwg2dxf_freecad_3d_2d_polyline_ocs_oda_cli
        COMMAND ${CMAKE_COMMAND}
            -DODAFILECONVERTER=${LIBDXFRW_ODAFILECONVERTER_EXECUTABLE}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            "-DSOURCE_DXF=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dxf/ac1015_2d_polyline_ocs_freecad_control.dxf"
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_2d_polyline_ocs_oda_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_2d_polyline_ocs_oda_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated;oda")
endif()

# Optional pinned FreeCAD importer-outcome check for one tilted WCS 3DFACE.
# It uses the independent ODA writer control and an installed build-identical
# converter, keeping FreeCAD out of default CI.
option(LIBDXFRW_ENABLE_FREECAD_3DFACE_RUNTIME_CONTROL
    "Enable the opt-in FreeCAD 3DFACE converter/importer outcome check" OFF)
if(LIBDXFRW_ENABLE_FREECAD_3DFACE_RUNTIME_CONTROL)
    find_program(LIBDXFRW_ODAFILECONVERTER_EXECUTABLE NAMES ODAFileConverter)
    if(NOT LIBDXFRW_ODAFILECONVERTER_EXECUTABLE)
        message(FATAL_ERROR
            "LIBDXFRW_ENABLE_FREECAD_3DFACE_RUNTIME_CONTROL requires ODAFileConverter")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECADCMD_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECADCMD_EXECUTABLE to FreeCAD's freecadcmd")
    endif()
    if(NOT EXISTS "${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}")
        message(FATAL_ERROR
            "Set LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE to the installed dwg2dxf")
    endif()
    foreach(_operation IN ITEMS open insert)
        add_test(
            NAME dwg2dxf_freecad_3dface_${_operation}_handoff
            COMMAND ${CMAKE_COMMAND}
                "-DFREECADCMD=${LIBDXFRW_FREECADCMD_EXECUTABLE}"
                "-DDWG2DXF=${LIBDXFRW_FREECAD_DWG2DXF_EXECUTABLE}"
                "-DBUILD_DWG2DXF=$<TARGET_FILE:dwg2dxf>"
                "-DODAFILECONVERTER=${LIBDXFRW_ODAFILECONVERTER_EXECUTABLE}"
                "-DSOURCE_DXF=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dxf/ac1015_3dface_freecad_control.dxf"
                "-DIMPORT_CHECK_MACRO=${CMAKE_CURRENT_SOURCE_DIR}/freecad_dwg2dxf_import_check.FCMacro"
                "-DOPERATION=${_operation}"
                -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3dface_runtime_test.cmake
        )
        set_tests_properties(dwg2dxf_freecad_3dface_${_operation}_handoff
            PROPERTIES
                LABELS "freecad;installed;generated;integration"
                TIMEOUT 180)
    endforeach()
endif()

# Optional locally-generated oblique OCS ARC/CIRCLE control. The public
# LibreDWG API is required because its dwgadd recipe cannot set point vectors.
option(LIBDXFRW_ENABLE_LIBREDWG_API_FREECAD_CONTROL
    "Enable the LibreDWG API-generated FreeCAD oblique OCS control" OFF)
if(LIBDXFRW_ENABLE_LIBREDWG_API_FREECAD_CONTROL)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBREDWG_API REQUIRED IMPORTED_TARGET libredwg)
    add_executable(libdxfrw_freecad_oblique_arc_circle_generator
        fixtures/dwg/create_oblique_arc_circle_freecad_control.c)
    target_link_libraries(libdxfrw_freecad_oblique_arc_circle_generator
        PRIVATE PkgConfig::LIBREDWG_API)
    add_test(
        NAME dwg2dxf_freecad_3d_oblique_arc_circle_cli
        COMMAND ${CMAKE_COMMAND}
            -DGENERATOR=$<TARGET_FILE:libdxfrw_freecad_oblique_arc_circle_generator>
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_freecad_3d_oblique_arc_circle_cli_test.cmake
    )
    set_tests_properties(dwg2dxf_freecad_3d_oblique_arc_circle_cli PROPERTIES
        LABELS "dwg_read;freecad;cli;fast;generated;libredwg-api")
endif()

# LibreCAD_3 ABI compatibility check.
# Mirrors LibreCAD_3's DXFimpl override surface; fails to compile if upstream
# introduces a new pure virtual on DRW_Interface or removes/renames a public
# DRW_* field LibreCAD_3 reads. Built as an OBJECT library — no installed
# binary, no link-time impact, just a compile-only ABI gate. See the file
# header in lc3_compat_check.cpp for the sync procedure.
add_library(lc3_compat_check OBJECT
    ${CMAKE_CURRENT_SOURCE_DIR}/lc3_compat_check.cpp
)
target_include_directories(lc3_compat_check PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/intern
)
target_compile_features(lc3_compat_check PRIVATE cxx_std_17)

# Exercise a public DRW_Interface sink against locally generated 3D DXF
# records. The probe writes ASCII and binary DXFs at runtime, reads them back
# through dxfRW, and checks that native Z/topology/OCS values reach the sink.
# It deliberately compiles only against the standalone library API; the
# separate lc3_compat_check above remains the older 2D-consumer guard.
add_executable(libdxfrw_3d_consumer_probe
    ${CMAKE_CURRENT_SOURCE_DIR}/semantic_differential_adapter.cpp
)
target_compile_features(libdxfrw_3d_consumer_probe PRIVATE cxx_std_17)
target_compile_options(libdxfrw_3d_consumer_probe
    PRIVATE ${LIBDXFRW_ADAPTER_WARNING_FLAGS})
target_compile_definitions(libdxfrw_3d_consumer_probe PRIVATE
    LIBDXFRW_SEMANTIC_3D_CONSUMER_PROBE=1)
target_include_directories(libdxfrw_3d_consumer_probe PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/intern)
target_link_libraries(libdxfrw_3d_consumer_probe PRIVATE dxfrw)
add_test(NAME libdxfrw_3d_consumer_probe
    COMMAND libdxfrw_3d_consumer_probe --self-test)
set_tests_properties(libdxfrw_3d_consumer_probe PROPERTIES
    LABELS "dxf;consumer;fast")

# DXF splinepoints round-trip regression test.
# Verifies Band BC: code 44 (tolfit) + fit points (11/21/31) survive a
# DWG -> DXF -> DXF round-trip. Picks a known-spline-bearing sample.
set(_dxf_roundtrip_spline_sample
    ${CMAKE_CURRENT_SOURCE_DIR}/samples/AC1024/visualization_-_conference_room.dwg
)
if(EXISTS "${_dxf_roundtrip_spline_sample}")
    add_test(
        NAME dxf_roundtrip_splinepoints
        COMMAND ${CMAKE_COMMAND}
            -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
            -DSPLINE_DWG=${_dxf_roundtrip_spline_sample}
            -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dxf_roundtrip_splinepoints_test.cmake
    )
    set_tests_properties(dxf_roundtrip_splinepoints PROPERTIES
        LABELS "dxf_roundtrip;BC"
    )
endif()

set(_dwg_test_versions AC1021 AC1024 AC1027 AC1032)
set(_dwg_test_outputs R12 v2000 v2010)

foreach(_ver IN LISTS _dwg_test_versions)
    file(GLOB _samples
        LIST_DIRECTORIES false
        "${CMAKE_CURRENT_SOURCE_DIR}/samples/${_ver}/*.dwg"
    )
    list(LENGTH _samples _n)
    if(_n EQUAL 0)
        message(STATUS "tests/: no ${_ver} samples in tests/samples/${_ver}/ — drop *.dwg files there to enable")
        continue()
    endif()
    list(LENGTH _dwg_test_outputs _outs)
    math(EXPR _total "${_n} * ${_outs}")
    message(STATUS "tests/: registering ${_n} ${_ver} sample(s) × ${_outs} output version(s) = ${_total} test(s)")
    foreach(_sample IN LISTS _samples)
        get_filename_component(_name "${_sample}" NAME_WE)
        foreach(_out IN LISTS _dwg_test_outputs)
            add_test(
                NAME dwg_read_${_ver}_${_name}_${_out}
                COMMAND ${CMAKE_COMMAND}
                    -DDWG2DXF=$<TARGET_FILE:dwg2dxf>
                    -DSAMPLE=${_sample}
                    -DEXPECTED_VERSION=${_ver}
                    -DOUTPUT_VERSION=${_out}
                    -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
                    -P ${CMAKE_CURRENT_SOURCE_DIR}/run_dwg_read_test.cmake
            )
            set_tests_properties(dwg_read_${_ver}_${_name}_${_out} PROPERTIES
                LABELS "dwg_read;${_ver};out_${_out}"
            )
        endforeach()
    endforeach()
endforeach()
