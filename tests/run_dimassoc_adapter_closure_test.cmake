if(NOT DEFINED AUTHOR OR NOT EXISTS "${AUTHOR}")
    message(FATAL_ERROR "DIMASSOC author executable is unavailable")
endif()
if(NOT DEFINED CONVERTER OR NOT EXISTS "${CONVERTER}")
    message(FATAL_ERROR "dwg2dxf converter executable is unavailable")
endif()
if(NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR "scratch output directory was not provided")
endif()

set(source "${OUTPUT_DIR}/dimassoc-adapter-closure-scratch.dwg")
set(output "${OUTPUT_DIR}/dimassoc-adapter-closure-scratch.dxf")
file(REMOVE "${source}" "${output}")

execute_process(
    COMMAND "${AUTHOR}" --generate-dimassoc "${source}"
    RESULT_VARIABLE author_result
    OUTPUT_VARIABLE author_stdout
    ERROR_VARIABLE author_stderr)
if(NOT author_result EQUAL 0 OR NOT EXISTS "${source}")
    message(FATAL_ERROR
        "Could not author scratch DIMASSOC source: ${author_stdout}${author_stderr}")
endif()

execute_process(
    COMMAND "${CONVERTER}" "${source}" -y -v2010 "${output}"
    RESULT_VARIABLE converter_result
    OUTPUT_VARIABLE converter_stdout
    ERROR_VARIABLE converter_stderr)
set(converter_log "${converter_stdout}${converter_stderr}")
if(converter_result EQUAL 0)
    message(FATAL_ERROR "Unclosed DIMASSOC source unexpectedly converted")
endif()
if(NOT converter_log MATCHES "DWG DIMASSOC records")
    message(FATAL_ERROR
        "Conversion failed before the DIMASSOC closure gate: ${converter_log}")
endif()
if(EXISTS "${output}")
    message(FATAL_ERROR "Failed DIMASSOC conversion published a DXF output")
endif()

file(REMOVE "${source}" "${output}")
message(STATUS
    "Unclosed DIMASSOC target refused atomically; scratch files removed")
