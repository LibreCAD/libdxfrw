include(${CMAKE_CURRENT_LIST_DIR}/libdxfrwAdapterContext.cmake)

# Keep the CLI, tests/ strict-warning pool, GBK and debug-state variants separate.
# Neither include search order nor warning policy is normalized to force sharing.
set(_libdxfrw_root_test_adapter_source "${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf/dx_iface.cpp")
set(_libdxfrw_reader_test_adapter_source "${_libdxfrw_root_test_adapter_source}")
set(_libdxfrw_wave1_adapter_source "${_libdxfrw_root_test_adapter_source}")
set(_libdxfrw_root_test_adapters_shared FALSE)
if(LIBDXFRW_DEDUP_TEST_ADAPTER AND NOT CMAKE_POSITION_INDEPENDENT_CODE
        AND NOT CMAKE_ENABLE_EXPORTS)
    # Like the tests/ pool, retain per-executable compilation for PIE/export
    # configurations, whose compiler flags can differ from OBJECT libraries.
    foreach(_variant IN ITEMS root reader)
        add_library(libdxfrw_${_variant}_test_adapter OBJECT
            "${_libdxfrw_root_test_adapter_source}")
        target_compile_features(libdxfrw_${_variant}_test_adapter PRIVATE cxx_std_17)
        target_include_directories(libdxfrw_${_variant}_test_adapter PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src)
        if(_variant STREQUAL "reader")
            target_include_directories(libdxfrw_${_variant}_test_adapter PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/src/intern
                ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf)
        else()
            target_include_directories(libdxfrw_${_variant}_test_adapter PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR})
        endif()
        target_link_libraries(libdxfrw_${_variant}_test_adapter PRIVATE dxfrw)
    endforeach()
    set(_libdxfrw_root_test_adapter_source "$<TARGET_OBJECTS:libdxfrw_root_test_adapter>")
    set(_libdxfrw_reader_test_adapter_source "$<TARGET_OBJECTS:libdxfrw_reader_test_adapter>")
    if(NOT MSVC)
        # MSVC Wave1's LIBDXFRW_FORCE_TEST_SEAMS changes adapter declarations.
        set(_libdxfrw_wave1_adapter_source "${_libdxfrw_reader_test_adapter_source}")
    endif()
    set(_libdxfrw_root_test_adapters_shared TRUE)
endif()

# Call only after all root-directory target customizations.
function(libdxfrw_check_root_test_adapter_context)
    if(NOT _libdxfrw_root_test_adapters_shared)
        return()
    endif()
    libdxfrw_check_adapter_context(libdxfrw_root_test_adapter
        libdxfrw_dwg_local_roundtrip libdxfrw_dxf_fixture_tests
        libdxfrw_dwg_fixture_tests)
    set(_reader_members libdxfrw_symbol_output_tests libdxfrw_dwg_reader_matrix_tests)
    if(NOT MSVC)
        list(APPEND _reader_members libdxfrw_wave1_tests)
    endif()
    libdxfrw_check_adapter_context(libdxfrw_reader_test_adapter ${_reader_members})
endfunction()
