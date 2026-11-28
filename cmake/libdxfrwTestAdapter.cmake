# Only explicitly audited tests/ executables use this compile context. In
# particular, do not mix in the CLI (framework search paths), the separately
# audited root test contexts, or MSVC Wave 1 seams.
include(${CMAKE_CURRENT_LIST_DIR}/libdxfrwAdapterContext.cmake)
set(_libdxfrw_test_adapter_source "${CMAKE_SOURCE_DIR}/dwg2dxf/dx_iface.cpp")
set(_libdxfrw_test_adapter_shared FALSE)
if(LIBDXFRW_DEDUP_TEST_ADAPTER AND NOT CMAKE_POSITION_INDEPENDENT_CODE
        AND NOT CMAKE_ENABLE_EXPORTS)
    # Executable PIE/export flags can differ from an OBJECT library's flags.
    # Keep the original per-executable compilation in those configurations.
    add_library(libdxfrw_test_adapter OBJECT "${_libdxfrw_test_adapter_source}")
    target_compile_features(libdxfrw_test_adapter PRIVATE cxx_std_17)
    target_compile_options(libdxfrw_test_adapter PRIVATE ${LIBDXFRW_WARNING_FLAGS})
    target_include_directories(libdxfrw_test_adapter PRIVATE
        ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(libdxfrw_test_adapter PRIVATE dxfrw)
    set(_libdxfrw_test_adapter_source "$<TARGET_OBJECTS:libdxfrw_test_adapter>")
    set(_libdxfrw_test_adapter_shared TRUE)
endif()

function(libdxfrw_check_test_adapter_context)
    if(NOT _libdxfrw_test_adapter_shared)
        return()
    endif()
    libdxfrw_check_adapter_context(libdxfrw_test_adapter ${ARGN})
endfunction()
