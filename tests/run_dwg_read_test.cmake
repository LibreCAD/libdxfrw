# CTest harness invoked per (sample × output_version) by tests/CMakeLists.txt.
# Required vars (passed via -D):
#   DWG2DXF           absolute path to the dwg2dxf executable
#   SAMPLE            absolute path to the .dwg input
#   EXPECTED_VERSION  version label e.g. AC1024 (used to validate header bytes)
#   OUTPUT_VERSION    -R12 / -v2000 / -v2004 / -v2007 / -v2010 (without dash)
#   OUTPUT_DIR        directory in which to write the produced .dxf

if(NOT DWG2DXF OR NOT SAMPLE OR NOT EXPECTED_VERSION OR NOT OUTPUT_VERSION OR NOT OUTPUT_DIR)
    message(FATAL_ERROR "missing one of: DWG2DXF SAMPLE EXPECTED_VERSION OUTPUT_VERSION OUTPUT_DIR")
endif()

# Step 1: verify the sample's first 6 bytes match EXPECTED_VERSION.
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

# Step 2: run dwg2dxf -y -<OUTPUT_VERSION> SAMPLE OUT
get_filename_component(_name "${SAMPLE}" NAME_WE)
set(_out "${OUTPUT_DIR}/${EXPECTED_VERSION}_${_name}_${OUTPUT_VERSION}.dxf")
file(REMOVE "${_out}")

execute_process(
    COMMAND "${DWG2DXF}" "${SAMPLE}" -y -${OUTPUT_VERSION} "${_out}"
    RESULT_VARIABLE _rc
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
)

if(NOT _rc EQUAL 0)
    message(FATAL_ERROR
        "dwg2dxf returned ${_rc} for ${SAMPLE} (output=${OUTPUT_VERSION})\n"
        "stdout:\n${_stdout}\n"
        "stderr:\n${_stderr}"
    )
endif()

# Step 3: produced DXF must exist and be non-empty.
if(NOT EXISTS "${_out}")
    message(FATAL_ERROR "dwg2dxf reported success but no output at ${_out}")
endif()
file(SIZE "${_out}" _sz)
if(_sz LESS 100)
    message(FATAL_ERROR "${_out} is only ${_sz} bytes — likely truncated")
endif()

# Step 4: minimal DXF structural sanity — required markers.
file(READ "${_out}" _dxf)
foreach(_marker SECTION ENDSEC EOF HEADER TABLES)
    string(FIND "${_dxf}" "${_marker}" _pos)
    if(_pos EQUAL -1)
        message(FATAL_ERROR "${_out}: missing required DXF marker '${_marker}'")
    endif()
endforeach()

# Step 5: must have at least one entity or table record. Look for any of the
# common record types — at least one should appear in any non-empty drawing.
set(_found_record 0)
foreach(_rec LAYER LINE LWPOLYLINE POLYLINE INSERT BLOCK CIRCLE TEXT MTEXT VPORT BLOCK_RECORD)
    string(FIND "${_dxf}" "${_rec}" _pos)
    if(NOT _pos EQUAL -1)
        set(_found_record 1)
        break()
    endif()
endforeach()
if(NOT _found_record)
    message(FATAL_ERROR "${_out}: no recognizable DXF record types — output likely empty or malformed")
endif()

message(STATUS "OK ${EXPECTED_VERSION}/${_name}/${OUTPUT_VERSION}: ${_sz} bytes")
