# This optional control is tied to the independently inspected ACadSharp sample.
file(SHA256 "${SAMPLE}" sample_hash)
if(NOT sample_hash STREQUAL "d1ec4519be14ac49ecab5ec024ad951271e00261a1851f623a784a97ba6ede73")
    message(FATAL_ERROR "AC1014 TABLE sample hash does not match the qualified control")
endif()
execute_process(COMMAND "${TEST_EXECUTABLE}" "${SAMPLE}"
    RESULT_VARIABLE status)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "AC1014 TABLE prefix control failed: ${status}")
endif()
