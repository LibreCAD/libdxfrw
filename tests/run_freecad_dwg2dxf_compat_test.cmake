if(NOT DEFINED DWG2DXF OR NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR "DWG2DXF and OUTPUT_DIR are required")
endif()
if(NOT DEFINED SAMPLE AND NOT DEFINED DWG_SOURCE_GENERATOR)
    message(FATAL_ERROR "SAMPLE or DWG_SOURCE_GENERATOR is required")
endif()
if(DEFINED DWG_SOURCE_GENERATOR AND NOT EXISTS "${DWG_SOURCE_GENERATOR}")
    message(FATAL_ERROR "DWG source generator does not exist: ${DWG_SOURCE_GENERATOR}")
endif()
if(DEFINED SAMPLE AND NOT EXISTS "${SAMPLE}")
    message(FATAL_ERROR "DWG sample does not exist: ${SAMPLE}")
endif()
if(NOT DEFINED RTEXT_SAMPLE OR NOT DEFINED MPOLYGON_SAMPLE)
    message(FATAL_ERROR "RTEXT_SAMPLE and MPOLYGON_SAMPLE are required")
endif()

# The registered fast test supplies Python for the macro's actual pure
# predicates. The installed clean-loader wrapper intentionally remains a
# converter-only gate and does not add Python to that environment.
if(DEFINED PYTHON)
    if(NOT IS_ABSOLUTE "${PYTHON}" OR NOT EXISTS "${PYTHON}")
        message(FATAL_ERROR "PYTHON must be an existing absolute path for LINE predicate controls")
    endif()
    execute_process(
        COMMAND "${PYTHON}" -I -B
            "${CMAKE_CURRENT_LIST_DIR}/freecad_dwg2dxf_import_check.FCMacro" --self-test
        RESULT_VARIABLE _predicate_result
        OUTPUT_VARIABLE _predicate_stdout
        ERROR_VARIABLE _predicate_stderr
        TIMEOUT 15
    )
    if(NOT "${_predicate_result}" STREQUAL "0"
            OR NOT _predicate_stdout MATCHES "FreeCAD LINE predicate controls: PASS")
        message(FATAL_ERROR
            "Dependency-free LINE predicate controls failed (${_predicate_result}).\n"
            "${_predicate_stdout}\n${_predicate_stderr}")
    endif()
endif()

string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef _nonce)
set(_test_dir "${OUTPUT_DIR}/freecad dwg2dxf cli test-${_nonce}")
if(EXISTS "${_test_dir}")
    message(FATAL_ERROR "Refusing to reuse existing test directory '${_test_dir}'")
endif()
file(MAKE_DIRECTORY "${_test_dir}")
set(_input "${_test_dir}/source drawing AC1027.dwg")
if(DEFINED SAMPLE)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy "${SAMPLE}" "${_input}"
        RESULT_VARIABLE _copy_result
    )
    if(NOT "${_copy_result}" STREQUAL "0")
        message(FATAL_ERROR "Could not prepare the test-local DWG input (${_copy_result})")
    endif()
else()
    execute_process(
        COMMAND "${DWG_SOURCE_GENERATOR}" --generate-cli-batch-source "${_input}" AC1027
        RESULT_VARIABLE _generate_result
        OUTPUT_VARIABLE _generate_stdout
        ERROR_VARIABLE _generate_stderr
        TIMEOUT 30
    )
    if(NOT "${_generate_result}" STREQUAL "0" OR NOT EXISTS "${_input}")
        message(FATAL_ERROR
            "Could not generate the test-local AC1027 DWG (${_generate_result})\n"
            "${_generate_stdout}\n${_generate_stderr}")
    endif()
endif()
set(_output "${_test_dir}/ordinary encoding AC1027.dxf")
file(REMOVE "${_output}")

# This is the exact argv shape used by FreeCAD: no shell, version, or -y flag.
execute_process(
    COMMAND "${DWG2DXF}" "${_input}" -o "${_output}"
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
    TIMEOUT 60
)
if(NOT "${_result}" STREQUAL "0")
    message(FATAL_ERROR
        "FreeCAD-form dwg2dxf invocation failed (${_result})\n${_stdout}\n${_stderr}")
endif()
if(NOT EXISTS "${_output}")
    message(FATAL_ERROR "FreeCAD-form invocation did not create its DXF output")
endif()

file(READ "${_output}" _header LIMIT 2048)
string(FIND "${_header}" "SECTION" _section_pos)
string(FIND "${_header}" "AC1027" _version_pos)
if(_section_pos EQUAL -1 OR _version_pos EQUAL -1)
    message(FATAL_ERROR
        "Expected ASCII DXF structure and source revision AC1027 in output header")
endif()

# DXF's effective text-codec version is AC1021 for all modern releases, but
# the CLI's default output policy must use the exact declared $ACADVER. Verify
# that a second FreeCAD-form conversion does not silently downgrade AC1027.
set(_readback_output "${_test_dir}/AC1027 DXF readback.dxf")
file(REMOVE "${_readback_output}")
execute_process(
    COMMAND "${DWG2DXF}" "${_output}" -o "${_readback_output}"
    RESULT_VARIABLE _readback_result
    OUTPUT_VARIABLE _readback_stdout
    ERROR_VARIABLE _readback_stderr
    TIMEOUT 60
)
if(NOT "${_readback_result}" STREQUAL "0" OR NOT EXISTS "${_readback_output}")
    message(FATAL_ERROR
        "FreeCAD-form DXF readback failed (${_readback_result})\n"
        "${_readback_stdout}\n${_readback_stderr}")
endif()
file(READ "${_readback_output}" _readback_header LIMIT 2048)
string(FIND "${_readback_header}" "AC1027" _readback_version_pos)
if(_readback_version_pos EQUAL -1)
    message(FATAL_ERROR
        "FreeCAD-form DXF readback downgraded the source revision from AC1027")
endif()

function(assert_freecad_entity_refusal sample stem diagnostic)
    set(_special_input "${_test_dir}/${stem} source.dwg")
    set(_special_output "${_test_dir}/${stem} result.dxf")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy "${sample}" "${_special_input}"
        RESULT_VARIABLE _copy_result
    )
    if(NOT "${_copy_result}" STREQUAL "0")
        message(FATAL_ERROR "Could not prepare ${stem} DWG input (${_copy_result})")
    endif()
    file(REMOVE "${_special_output}")
    execute_process(
        COMMAND "${DWG2DXF}" "${_special_input}" -o "${_special_output}"
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _stdout
        ERROR_VARIABLE _stderr
        TIMEOUT 60
    )
    string(FIND "${_stderr}" "${diagnostic}" _diagnostic_pos)
    if("${_result}" STREQUAL "0" OR EXISTS "${_special_output}"
            OR _diagnostic_pos EQUAL -1)
        message(FATAL_ERROR
            "FreeCAD-form ${stem} refusal was not explicit and atomic (${_result})\n"
            "${_stdout}\n${_stderr}")
    endif()
    file(GLOB _temporaries "${_special_output}.libdxfrw-*")
    if(_temporaries)
        message(FATAL_ERROR
            "FreeCAD-form ${stem} refusal left output temporaries: ${_temporaries}")
    endif()
endfunction()

# These tracked special-record drawings currently report a source-header or
# class CRC mismatch. The strict FreeCAD route must surface that first read
# completeness gate and leave no DXF behind; the adapter's typed-proxy gate is
# checked separately by the CLI contract tests.
set(_strict_read_refusal
    "strict-complete conversion requires successful, complete DWG read disposition")
assert_freecad_entity_refusal("${RTEXT_SAMPLE}" "rtext arctext"
    "${_strict_read_refusal}")
assert_freecad_entity_refusal("${MPOLYGON_SAMPLE}" "mpolygon"
    "${_strict_read_refusal}")

# Unix command-line arguments are byte strings (UTF-8 on the supported host
# environments exercised here). Keep Windows Unicode argv qualification
# separate because the narrow main(argc, argv) entry point has a distinct
# encoding contract there.
if(NOT CMAKE_HOST_WIN32)
    set(_unicode_dir "${_test_dir}/entrée café")
    file(MAKE_DIRECTORY "${_unicode_dir}")
    set(_unicode_input "${_unicode_dir}/drawing été.dwg")
    set(_unicode_output "${_unicode_dir}/résultat été.dxf")
    file(REMOVE "${_unicode_output}")
    if(DEFINED SAMPLE)
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E copy "${SAMPLE}" "${_unicode_input}"
            RESULT_VARIABLE _copy_result
        )
        if(NOT "${_copy_result}" STREQUAL "0")
            message(FATAL_ERROR
                "Could not prepare Unicode-path DWG input (${_copy_result})")
        endif()
    else()
        execute_process(
            COMMAND "${DWG_SOURCE_GENERATOR}" --generate-cli-batch-source "${_unicode_input}" AC1027
            RESULT_VARIABLE _generate_result
            OUTPUT_VARIABLE _generate_stdout
            ERROR_VARIABLE _generate_stderr
            TIMEOUT 30
        )
        if(NOT "${_generate_result}" STREQUAL "0" OR NOT EXISTS "${_unicode_input}")
            message(FATAL_ERROR
                "Could not generate the Unicode-path AC1027 DWG (${_generate_result})\n"
                "${_generate_stdout}\n${_generate_stderr}")
        endif()
    endif()
    execute_process(
        COMMAND "${DWG2DXF}" "${_unicode_input}" -o "${_unicode_output}"
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _stdout
        ERROR_VARIABLE _stderr
        TIMEOUT 60
    )
    if(NOT "${_result}" STREQUAL "0" OR NOT EXISTS "${_unicode_output}")
        message(FATAL_ERROR
            "FreeCAD-form conversion failed for UTF-8 paths (${_result})\n${_stdout}\n${_stderr}")
    endif()
    file(READ "${_unicode_output}" _unicode_header LIMIT 2048)
    string(FIND "${_unicode_header}" "AC1027" _unicode_version_pos)
    if(_unicode_version_pos EQUAL -1)
        message(FATAL_ERROR "UTF-8-path output is not a valid AC1027 DXF")
    endif()
else()
    message(STATUS
        "Skipping non-ASCII argv test on Windows; narrow argv encoding needs native qualification")
endif()

# Preserve the established positional form and its explicit-version behavior.
set(_legacy_output "${_test_dir}/legacy positional AC1024.dxf")
file(REMOVE "${_legacy_output}")
execute_process(
    COMMAND "${DWG2DXF}" "${_input}" -v2010 "${_legacy_output}"
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
    TIMEOUT 60
)
if(NOT "${_result}" STREQUAL "0")
    message(FATAL_ERROR
        "Legacy positional invocation failed (${_result})\n${_stdout}\n${_stderr}")
endif()
file(READ "${_legacy_output}" _legacy_header LIMIT 2048)
string(FIND "${_legacy_header}" "AC1024" _legacy_version_pos)
if(_legacy_version_pos EQUAL -1)
    message(FATAL_ERROR "Legacy positional invocation ignored -v2010")
endif()

# The normal FreeCAD argv has no -y. It must fail promptly without prompting,
# and must preserve an existing output instead of truncating it.
file(WRITE "${_output}" "freecad-output-sentinel\n")
execute_process(
    COMMAND "${DWG2DXF}" "${_input}" -o "${_output}"
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
    TIMEOUT 5
)
if("${_result}" STREQUAL "0")
    message(FATAL_ERROR "No-overwrite FreeCAD-form invocation unexpectedly succeeded")
endif()
file(READ "${_output}" _sentinel)
if(NOT "${_sentinel}" STREQUAL "freecad-output-sentinel\n")
    message(FATAL_ERROR "No-overwrite failure modified the existing output")
endif()

# An explicit -y authorizes replacement. The output remains test-local.
execute_process(
    COMMAND "${DWG2DXF}" "${_input}" -o "${_output}" -y
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
    TIMEOUT 60
)
if(NOT "${_result}" STREQUAL "0")
    message(FATAL_ERROR
        "Explicit overwrite invocation failed (${_result})\n${_stdout}\n${_stderr}")
endif()
file(READ "${_output}" _header LIMIT 2048)
string(FIND "${_header}" "AC1027" _version_pos)
if(_version_pos EQUAL -1)
    message(FATAL_ERROR "Explicit overwrite did not replace the sentinel with a DXF")
endif()

# FreeCAD ignores the converter exit status and accepts a path that exists.
# A malformed input must therefore leave no apparently successful output.
set(_bad_input "${_test_dir}/malformed source.dwg")
set(_bad_output "${_test_dir}/malformed result.dxf")
file(WRITE "${_bad_input}" "AC1027")
file(REMOVE "${_bad_output}")
execute_process(
    COMMAND "${DWG2DXF}" "${_bad_input}" -o "${_bad_output}"
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
    TIMEOUT 5
)
if("${_result}" STREQUAL "0")
    message(FATAL_ERROR
        "Malformed FreeCAD-form conversion unexpectedly succeeded\n${_stdout}\n${_stderr}")
endif()
if(EXISTS "${_bad_output}")
    message(FATAL_ERROR
        "Malformed FreeCAD-form conversion left an output path FreeCAD could import")
endif()

# Even with -y, failed input must not replace a previous complete result.
file(WRITE "${_bad_output}" "preserve-after-conversion-failure\n")
execute_process(
    COMMAND "${DWG2DXF}" "${_bad_input}" -o "${_bad_output}" -y
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
    TIMEOUT 5
)
if("${_result}" STREQUAL "0")
    message(FATAL_ERROR
        "Malformed explicit-overwrite conversion unexpectedly succeeded\n${_stdout}\n${_stderr}")
endif()
file(READ "${_bad_output}" _preserved)
if(NOT "${_preserved}" STREQUAL "preserve-after-conversion-failure\n")
    message(FATAL_ERROR
        "Failed explicit-overwrite conversion changed the previous output")
endif()

file(GLOB _failed_temporary_files "${_bad_output}.libdxfrw-*")
if(_failed_temporary_files)
    message(FATAL_ERROR
        "Failed conversion left output temporaries: ${_failed_temporary_files}")
endif()
file(REMOVE_RECURSE "${_test_dir}")
