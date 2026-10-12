foreach(_required IN ITEMS ADAPTER_TEST DWG2DXF OUTPUT_DIR)
    if(NOT DEFINED ${_required})
        message(FATAL_ERROR "${_required} is required")
    endif()
endforeach()
foreach(_path IN ITEMS ADAPTER_TEST DWG2DXF)
    if(NOT EXISTS "${${_path}}")
        message(FATAL_ERROR "${_path} does not exist: ${${_path}}")
    endif()
endforeach()

if(NOT DEFINED CASE_NAME)
    set(CASE_NAME "image-path")
endif()
if(NOT DEFINED GENERATE_MODE)
    set(GENERATE_MODE "--generate")
endif()
if(NOT DEFINED VERIFY_MODE)
    set(VERIFY_MODE "--verify")
endif()

string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef _run_id)
set(_test_root "${OUTPUT_DIR}/dwg2dxf-${CASE_NAME}-${_run_id}")
file(MAKE_DIRECTORY "${_test_root}")
set(_input "${_test_root}/${CASE_NAME} source.dxf")
set(_first "${_test_root}/${CASE_NAME} first pass.dxf")
set(_second "${_test_root}/${CASE_NAME} second pass.dxf")

execute_process(
    COMMAND "${ADAPTER_TEST}" "${GENERATE_MODE}" "${_input}"
    RESULT_VARIABLE _generate_result
    OUTPUT_VARIABLE _generate_stdout
    ERROR_VARIABLE _generate_stderr
    TIMEOUT 10
)
if(NOT "${_generate_result}" STREQUAL "0")
    message(FATAL_ERROR
        "Could not create adapter control (${_generate_result}).\n"
        "${_generate_stdout}\n${_generate_stderr}\nEvidence: ${_test_root}")
endif()

execute_process(
    COMMAND "${DWG2DXF}" "${_input}" -o "${_first}" ${CONVERSION_OPTIONS}
    RESULT_VARIABLE _first_result
    OUTPUT_VARIABLE _first_stdout
    ERROR_VARIABLE _first_stderr
    TIMEOUT 10
)
if(NOT "${_first_result}" STREQUAL "0")
    message(FATAL_ERROR
        "First exact-argv adapter conversion failed (${_first_result}).\n"
        "${_first_stdout}\n${_first_stderr}\nEvidence: ${_test_root}")
endif()

execute_process(
    COMMAND "${DWG2DXF}" "${_first}" -o "${_second}" ${CONVERSION_OPTIONS}
    RESULT_VARIABLE _second_result
    OUTPUT_VARIABLE _second_stdout
    ERROR_VARIABLE _second_stderr
    TIMEOUT 10
)
if(NOT "${_second_result}" STREQUAL "0")
    message(FATAL_ERROR
        "Second exact-argv adapter conversion failed (${_second_result}).\n"
        "${_second_stdout}\n${_second_stderr}\nEvidence: ${_test_root}")
endif()

foreach(_output IN ITEMS "${_first}" "${_second}")
    execute_process(
        COMMAND "${ADAPTER_TEST}" "${VERIFY_MODE}" "${_output}"
        RESULT_VARIABLE _verify_result
        OUTPUT_VARIABLE _verify_stdout
        ERROR_VARIABLE _verify_stderr
        TIMEOUT 10
    )
    if(NOT "${_verify_result}" STREQUAL "0")
        message(FATAL_ERROR
            "Adapter read-back failed for ${_output} (${_verify_result}).\n"
            "${_verify_stdout}\n${_verify_stderr}\nEvidence: ${_test_root}")
    endif()
endforeach()

if(DEFINED NEGATIVE_MODE AND NOT "${NEGATIVE_MODE}" STREQUAL "")
    set(_negative_output "${_test_root}/${CASE_NAME} unsupported AC1014.dxf")
    execute_process(
        COMMAND "${ADAPTER_TEST}" "${NEGATIVE_MODE}" "${_negative_output}"
        RESULT_VARIABLE _negative_result
        OUTPUT_VARIABLE _negative_stdout
        ERROR_VARIABLE _negative_stderr
        TIMEOUT 10
    )
    if(NOT "${_negative_result}" STREQUAL "0")
        message(FATAL_ERROR
            "Unsupported-version adapter gate failed (${_negative_result}).\n"
            "${_negative_stdout}\n${_negative_stderr}\nEvidence: ${_test_root}")
    endif()
    if(EXISTS "${_negative_output}")
        message(FATAL_ERROR
            "Unsupported-version adapter gate published ${_negative_output}")
    endif()
endif()

file(REMOVE_RECURSE "${_test_root}")
message(STATUS
    "Adapter control survived generated export, two exact-argv CLI passes, and public read-back.")
