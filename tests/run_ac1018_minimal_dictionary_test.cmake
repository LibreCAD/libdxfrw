set(source "${OUTPUT_DIR}/ac1018-minimal-dictionary-source.dwg")
file(REMOVE "${source}")
execute_process(COMMAND "${WRITER}" --generate-cli-batch-source "${source}" AC1018
    RESULT_VARIABLE generated)
if(NOT generated EQUAL 0)
    message(FATAL_ERROR "Could not author scratch AC1018 source: ${generated}")
endif()
execute_process(COMMAND "${ADAPTER_TEST}" --ac1018-minimal-root-group "${source}"
    RESULT_VARIABLE result)
file(REMOVE "${source}")
if(NOT result EQUAL 0)
    message(FATAL_ERROR "AC1018 minimal dictionary controls failed: ${result}")
endif()
