include_guard(GLOBAL)

# Only compare explicitly audited targets created in the same directory.
# Directory/source properties and global/toolchain flags are then inherited
# together; target-local context and dependency lists must also match exactly.
# This is not a general equivalence proof for arbitrary target-dependent usage
# requirements. Separate build trees/configurations never reuse object files.
function(libdxfrw_check_adapter_context _adapter)
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
        foreach(_directory IN ITEMS SOURCE_DIR BINARY_DIR)
            get_property(_shared TARGET ${_adapter} PROPERTY ${_directory})
            get_property(_member TARGET ${_target} PROPERTY ${_directory})
            if(NOT "${_shared}" STREQUAL "${_member}")
                message(FATAL_ERROR
                    "${_target}: ${_directory} differs from the shared adapter. "
                    "Adapter pools require the same creation directory.")
            endif()
        endforeach()
        foreach(_property IN LISTS _properties)
            get_property(_shared TARGET ${_adapter} PROPERTY ${_property})
            get_property(_member TARGET ${_target} PROPERTY ${_property})
            # OFF initializes executable properties but not OBJECT properties.
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
