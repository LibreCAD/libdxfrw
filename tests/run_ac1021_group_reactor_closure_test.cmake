if(NOT DEFINED AUTHOR OR NOT EXISTS "${AUTHOR}")
    message(FATAL_ERROR "AC1021 GROUP author executable is unavailable")
endif()
if(NOT DEFINED ADAPTER OR NOT EXISTS "${ADAPTER}")
    message(FATAL_ERROR "DWG/DXF adapter test executable is unavailable")
endif()
if(NOT DEFINED CONVERTER OR NOT EXISTS "${CONVERTER}")
    message(FATAL_ERROR "dwg2dxf converter executable is unavailable")
endif()
if(NOT DEFINED READBACK OR NOT EXISTS "${READBACK}")
    message(FATAL_ERROR "generated DXF polyline readback executable is unavailable")
endif()
if(NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR "scratch output directory was not provided")
endif()

set(scratch_dir "${OUTPUT_DIR}/ac1021-group-adapter-scratch")
set(source "${scratch_dir}/vertex2d.dwg")
file(REMOVE_RECURSE "${scratch_dir}")

execute_process(
    COMMAND "${AUTHOR}" "${scratch_dir}"
    RESULT_VARIABLE author_result
    OUTPUT_VARIABLE author_stdout
    ERROR_VARIABLE author_stderr)
if(NOT author_result EQUAL 0 OR NOT EXISTS "${source}")
    message(FATAL_ERROR
        "Could not author scratch AC1021 GROUP source: ${author_stdout}${author_stderr}")
endif()

execute_process(
    COMMAND "${ADAPTER}" --generated-group-ac1021-reactor-closure "${source}"
    RESULT_VARIABLE adapter_result
    OUTPUT_VARIABLE adapter_stdout
    ERROR_VARIABLE adapter_stderr)
if(NOT adapter_result EQUAL 0)
    message(FATAL_ERROR
        "Generated AC1021 GROUP output/readback failed: ${adapter_stdout}${adapter_stderr}")
endif()

set(dxf_output_2d "${scratch_dir}/strict-output-2d.dxf")
execute_process(
    COMMAND "${CONVERTER}" "${source}" -y -v2007 "${dxf_output_2d}"
    RESULT_VARIABLE converter_result
    OUTPUT_VARIABLE converter_stdout
    ERROR_VARIABLE converter_stderr)
if(NOT converter_result EQUAL 0 OR NOT EXISTS "${dxf_output_2d}")
    message(FATAL_ERROR
        "Generated AC1021 2D strict conversion failed: ${converter_stdout}${converter_stderr}")
endif()
set(dxf_output_3d "${scratch_dir}/strict-output-3d.dxf")
set(source_3d "${scratch_dir}/vertex3d.dwg")
execute_process(
    COMMAND "${CONVERTER}" "${source_3d}" -y -v2007 "${dxf_output_3d}"
    RESULT_VARIABLE converter_3d_result
    OUTPUT_VARIABLE converter_3d_stdout
    ERROR_VARIABLE converter_3d_stderr)
if(NOT converter_3d_result EQUAL 0 OR NOT EXISTS "${dxf_output_3d}")
    message(FATAL_ERROR
        "Generated AC1021 3D strict conversion failed: ${converter_3d_stdout}${converter_3d_stderr}")
endif()
file(READ "${dxf_output_2d}" dxf_contents_2d)
file(READ "${dxf_output_3d}" dxf_contents_3d)
string(FIND "${dxf_contents_2d}" "ACAD_PLOTSTYLENAME" plot_style_root)
string(FIND "${dxf_contents_2d}" "Normal" plot_style_default)
if(plot_style_root EQUAL -1 OR plot_style_default EQUAL -1)
    message(FATAL_ERROR "Strict 2D DXF output omitted the default plot-style graph")
endif()
string(FIND "${dxf_contents_3d}" "AcDb3dPolyline" polyline_3d_subclass)
if(polyline_3d_subclass EQUAL -1)
    message(FATAL_ERROR "Strict 3D DXF output omitted the 3D polyline subclass")
endif()
execute_process(
    COMMAND "${READBACK}" "${dxf_output_2d}" "${dxf_output_3d}"
    RESULT_VARIABLE readback_result
    OUTPUT_VARIABLE readback_stdout
    ERROR_VARIABLE readback_stderr)
if(NOT readback_result EQUAL 0)
    message(FATAL_ERROR
        "Generated AC1021 strict DXF typed readback failed: ${readback_stdout}${readback_stderr}")
endif()

file(REMOVE_RECURSE "${scratch_dir}")
message(STATUS
    "Generated AC1021 GROUP/default plot-style reactor graph closure, refusal/retry, strict 2D/3D DXF output and typed readback passed")
