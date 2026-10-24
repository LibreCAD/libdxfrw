# Only the four audited tests/ executables use this compile context. In
# particular, do not pool the CLI (framework search paths), root tests
# (different includes/fixture definitions/warnings), or MSVC Wave 1 seams.
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
    # All members are created in this directory, and inherit the same directory
    # flags and source properties. Compare target-local and dependency context
    # too, rather than silently dropping a future member's sanitizer/seam flags.
    # Separate build trees/configurations always produce separate object files.
    set(_properties COMPILE_DEFINITIONS COMPILE_OPTIONS COMPILE_FLAGS
        INCLUDE_DIRECTORIES SYSTEM_INCLUDE_DIRECTORIES COMPILE_FEATURES
        CXX_STANDARD CXX_STANDARD_REQUIRED CXX_EXTENSIONS
        MSVC_RUNTIME_LIBRARY INTERPROCEDURAL_OPTIMIZATION
        POSITION_INDEPENDENT_CODE CXX_VISIBILITY_PRESET VISIBILITY_INLINES_HIDDEN
        ENABLE_EXPORTS LINK_LIBRARIES PRECOMPILE_HEADERS
        DISABLE_PRECOMPILE_HEADERS PCH_INSTANTIATE_TEMPLATES PCH_WARN_INVALID
        COMPILE_WARNING_AS_ERROR CXX_COMPILER_LAUNCHER CXX_SCAN_FOR_MODULES
        CXX_CLANG_TIDY CXX_CPPCHECK CXX_CPPLINT CXX_INCLUDE_WHAT_YOU_USE
        NO_SYSTEM_FROM_IMPORTED)
    foreach(_config IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL
            ${CMAKE_BUILD_TYPE} ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER "${_config}" _config)
        list(APPEND _properties COMPILE_DEFINITIONS_${_config}
            INTERPROCEDURAL_OPTIMIZATION_${_config})
    endforeach()
    foreach(_target IN LISTS ARGN)
        foreach(_property IN LISTS _properties)
            get_property(_shared TARGET libdxfrw_test_adapter PROPERTY ${_property})
            get_property(_member TARGET ${_target} PROPERTY ${_property})
            # CMAKE_ENABLE_EXPORTS=OFF initializes executable properties, but
            # not OBJECT properties. Both are the same non-exporting context.
            if(_property STREQUAL "ENABLE_EXPORTS" AND NOT _shared AND NOT _member)
                continue()
            endif()
            if(NOT "${_shared}" STREQUAL "${_member}")
                message(FATAL_ERROR
                    "${_target}: ${_property} differs from the shared adapter. "
                    "Use a separate adapter variant, or configure with "
                    "LIBDXFRW_DEDUP_TEST_ADAPTER=OFF.")
            endif()
        endforeach()
    endforeach()
endfunction()
