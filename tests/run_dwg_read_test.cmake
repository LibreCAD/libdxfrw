# CTest harness invoked once per sample by tests/CMakeLists.txt.
# Required vars (passed via -D):
#   READER_TEST       absolute path to the DWG reader matrix test executable
#   SAMPLE            absolute path to the .dwg input
#   EXPECTED_VERSION  version label e.g. AC1024 (used to validate header bytes)

if(NOT READER_TEST OR NOT SAMPLE OR NOT EXPECTED_VERSION)
    message(FATAL_ERROR "missing one of: READER_TEST SAMPLE EXPECTED_VERSION")
endif()

# Check the sample's first 6 bytes against its declared AC version.
file(READ "${SAMPLE}" _hdr LIMIT 6 HEX)
string(LENGTH "${_hdr}" _hex_len)
if(NOT _hex_len EQUAL 12)
    message(FATAL_ERROR "${SAMPLE}: cannot read first 6 bytes (got ${_hex_len} hex chars)")
endif()
set(_ascii "")
foreach(_i RANGE 0 10 2)
    string(SUBSTRING "${_hdr}" ${_i} 2 _byte)
    math(EXPR _val "0x${_byte}")
    string(ASCII ${_val} _c)
    string(APPEND _ascii "${_c}")
endforeach()
if(NOT _ascii STREQUAL EXPECTED_VERSION)
    message(FATAL_ERROR "${SAMPLE}: header is '${_ascii}', expected '${EXPECTED_VERSION}'")
endif()

# Keep parser success separate from the CLI's strict-complete export
# admission. Output structure and semantic/export completeness have their own
# tests and must not be inferred from this reader-only sample check.
execute_process(
    COMMAND "${READER_TEST}" --read-dwg-sample "${SAMPLE}"
    RESULT_VARIABLE _rc
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
)

if(NOT _rc EQUAL 0)
    message(FATAL_ERROR
        "DWG reader probe returned ${_rc} for ${SAMPLE}\n"
        "stdout:\n${_stdout}\n"
        "stderr:\n${_stderr}"
    )
endif()

message(STATUS "OK ${EXPECTED_VERSION}: DWG parser completed without record parse failures")
