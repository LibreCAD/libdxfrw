# DXF splinepoints round-trip regression test.
#
# Verifies that a DXF written by libdxfrw retains the conic-spline data
# LibreCAD relies on:
#   - Group code 44 (tolfit)             — fitting tolerance
#   - Group codes 11/21/31 (fit points)  — splinepoints fit-list
#
# These two are LibreCAD-fork customizations of writeSpline that the
# upstream's writer omitted before Band BC. Without them, a DWG → DXF
# round-trip silently drops the splinepoints fit-list and tolerance,
# breaking LibreCAD's LC_SplinePoints reload.
#
# Required vars (passed via -D):
#   DWG2DXF      absolute path to the dwg2dxf executable
#   SPLINE_DWG   absolute path to a known DWG sample containing splines
#                with non-zero nfit (e.g. tablet.dwg)
#   OUTPUT_DIR   directory in which to write intermediate DXFs

if(NOT DWG2DXF OR NOT SPLINE_DWG OR NOT OUTPUT_DIR)
    message(FATAL_ERROR "missing one of: DWG2DXF SPLINE_DWG OUTPUT_DIR")
endif()
if(NOT EXISTS "${SPLINE_DWG}")
    message(FATAL_ERROR "spline source not found: ${SPLINE_DWG}")
endif()

get_filename_component(_name "${SPLINE_DWG}" NAME_WE)
set(_pass1 "${OUTPUT_DIR}/${_name}.pass1.dxf")
set(_pass2 "${OUTPUT_DIR}/${_name}.pass2.dxf")
file(REMOVE "${_pass1}" "${_pass2}")

# Pass 1: DWG -> DXF
execute_process(
    COMMAND "${DWG2DXF}" "${SPLINE_DWG}" -y -v2010 "${_pass1}"
    RESULT_VARIABLE _rc1
    OUTPUT_QUIET ERROR_QUIET
)
if(NOT _rc1 EQUAL 0 OR NOT EXISTS "${_pass1}")
    message(FATAL_ERROR "pass1 (DWG -> DXF) failed for ${SPLINE_DWG}")
endif()

# Step A — pass1 must have at least one SPLINE entity.
file(READ "${_pass1}" _dxf1)
string(ASCII 10 _LF)
string(REGEX MATCHALL "${_LF}  0${_LF}SPLINE${_LF}" _spline_hits "${_dxf1}")
list(LENGTH _spline_hits _spline_count)
if(_spline_count EQUAL 0)
    message(FATAL_ERROR "${_pass1}: no SPLINE entities found — wrong sample for this test")
endif()

# Step B — pass1 must contain writeSpline's BC.1+BC.2 emission pattern:
# ` 43\n<tolcontrol>\n 44\n<tolfit>\n 11\n` (or 41/10/40, depending on
# whether the spline has nfit / weights / controls / nknots).
# writeVport also emits 43→44 (frontClip→backClip) but always follows
# with code 50 (snapAngle). For the chosen sample (conference_room),
# splines have nfit > 0, so 44 is followed by 11 (first fit-point X).
# This is a clean post-BC signature.
#
# CMake regex requires literal newlines in patterns; bracket-form `\n`
# matches `n` not LF. Build the pattern with `string(ASCII 10 LF)`.
set(_tolfit_re " 43${_LF}[^${_LF}]+${_LF} 44${_LF}[^${_LF}]+${_LF} 11${_LF}")
string(REGEX MATCHALL "${_tolfit_re}" _tolfit_hits1 "${_dxf1}")
list(LENGTH _tolfit_hits1 _tolfit_count1)
if(_tolfit_count1 EQUAL 0)
    message(FATAL_ERROR
        "${_pass1}: no SPLINE block emits code 44 (tolfit) — writeSpline regressed BC.1")
endif()

# Pass 2: DXF -> DXF (round-trip). Tests that read+write of code 44 + fit
# points round-trips losslessly.
execute_process(
    COMMAND "${DWG2DXF}" "${_pass1}" -y -v2010 "${_pass2}"
    RESULT_VARIABLE _rc2
    OUTPUT_QUIET ERROR_QUIET
)
if(NOT _rc2 EQUAL 0 OR NOT EXISTS "${_pass2}")
    message(FATAL_ERROR "pass2 (DXF -> DXF) failed for ${_pass1}")
endif()

# Step C — pass2 must still have SPLINE entities + code 44.
file(READ "${_pass2}" _dxf2)
string(REGEX MATCHALL "${_LF}  0${_LF}SPLINE${_LF}" _spline_hits2 "${_dxf2}")
list(LENGTH _spline_hits2 _spline_count2)
if(NOT _spline_count2 EQUAL _spline_count)
    message(FATAL_ERROR
        "${_pass2}: SPLINE count ${_spline_count2} != pass1 count ${_spline_count} — round-trip lost splines")
endif()

string(REGEX MATCHALL "${_tolfit_re}" _tolfit_hits2 "${_dxf2}")
list(LENGTH _tolfit_hits2 _tolfit_count2)
if(_tolfit_count2 EQUAL 0)
    message(FATAL_ERROR "${_pass2}: SPLINE tolfit (code 44) lost on round-trip")
endif()

message(STATUS "OK splinepoints round-trip ${_name}: ${_spline_count} splines, ${_tolfit_count1}->${_tolfit_count2} SPLINE-tolfit markers")
