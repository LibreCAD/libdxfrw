@PACKAGE_INIT@

# This package has no optional feature components. Do not accept an unknown
# required component, or inherit a caller's stale component-found variable.
set(libdxfrw_FOUND TRUE)
unset(libdxfrw_NOT_FOUND_MESSAGE)
foreach(_libdxfrw_component IN LISTS libdxfrw_FIND_COMPONENTS)
    set(libdxfrw_${_libdxfrw_component}_FOUND FALSE)
    if(libdxfrw_FIND_REQUIRED_${_libdxfrw_component})
        set(libdxfrw_NOT_FOUND_MESSAGE
            "libdxfrw has no component: ${_libdxfrw_component}")
    endif()
endforeach()
check_required_components(libdxfrw)
if(NOT libdxfrw_FOUND)
    return()
endif()

get_filename_component(DXFRW_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
file(REAL_PATH "${CMAKE_CURRENT_LIST_FILE}" _libdxfrw_config_origin)
foreach(_libdxfrw_target IN ITEMS libdxfrw::libdxfrw Libdxfrw::Libdxfrw)
    if(TARGET "${_libdxfrw_target}")
        get_target_property(_libdxfrw_existing_origin "${_libdxfrw_target}"
            _LIBDXFRW_PACKAGE_CONFIG)
        if(NOT _libdxfrw_existing_origin STREQUAL _libdxfrw_config_origin)
            set(libdxfrw_FOUND FALSE)
            set(libdxfrw_NOT_FOUND_MESSAGE
                "existing ${_libdxfrw_target} target belongs to another package")
            return()
        endif()
    endif()
endforeach()

include(CMakeFindDependencyMacro)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_dependency(Threads)

if(NOT TARGET libdxfrw::libdxfrw)
    include("${DXFRW_CMAKE_DIR}/libdxfrwTargets.cmake")
    set_property(TARGET libdxfrw::libdxfrw PROPERTY
        _LIBDXFRW_PACKAGE_CONFIG "${_libdxfrw_config_origin}")
endif()
