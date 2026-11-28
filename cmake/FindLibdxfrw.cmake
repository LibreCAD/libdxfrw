# FindLibdxfrw.cmake
#
# Module-mode fallback uses the selected library's co-located package metadata.
# A raw header/library installation remains usable without a version request.

find_path(Libdxfrw_INCLUDE_DIR NAMES drw_base.h PATH_SUFFIXES libdxfrw)
find_library(Libdxfrw_LIBRARY NAMES dxfrw libdxfrw)

# Read a selected configuration's locations. NEW alone permits falling back
# from an absent configuration-specific property to its agnostic counterpart.
# Presence, including a present-empty value, is distinct from absence.
function(_libdxfrw_locations _target _suffix _fallback)
    foreach(_kind IN ITEMS LOCATION IMPLIB)
        get_property(_set TARGET "${_target}" PROPERTY "IMPORTED_${_kind}${_suffix}" SET)
        get_target_property(_value "${_target}" "IMPORTED_${_kind}${_suffix}")
        if(NOT _set AND _fallback AND NOT _suffix STREQUAL "")
            get_property(_set TARGET "${_target}" PROPERTY "IMPORTED_${_kind}" SET)
            get_target_property(_value "${_target}" "IMPORTED_${_kind}")
        endif()
        set(_${_kind} "${_value}" PARENT_SCOPE)
        set(_${_kind}_SET "${_set}" PARENT_SCOPE)
    endforeach()
endfunction()

# Narrow CMP0200 OLD/NEW selection for the installed library kinds. Policy is
# captured at target creation, not by cmake_policy(GET) at discovery time.
function(_libdxfrw_target_artifact _target _config _policy _result)
    set(${_result} "" PARENT_SCOPE)
    get_target_property(_type "${_target}" TYPE)
    get_property(_configs_set TARGET "${_target}" PROPERTY IMPORTED_CONFIGURATIONS SET)
    get_target_property(_configs "${_target}" IMPORTED_CONFIGURATIONS)
    if(NOT _configs_set)
        set(_configs "")
    endif()
    set(_configs_value "${_configs}")
    string(TOUPPER "${_configs}" _configs)
    # CMake's advertised configuration list discards empty elements.
    list(FILTER _configs EXCLUDE REGEX "^$")
    if(_policy STREQUAL NEW AND _configs_set AND _configs STREQUAL "")
        # Conservatively reject pre-4.4.4 NEW present-empty lists. Verified
        # upstream 4.2.0/4.2.4/4.3.5/4.4.3 can index an empty advertised list;
        # 23f1cb47 (in 4.4.4) fixes empty==absent. Vendor backports may differ.
        # This version bound selects implementation behavior, not creation
        # policy. Separator-only lists still have no selectable entry.
        if(CMAKE_VERSION VERSION_LESS 4.4.4 OR NOT _configs_value STREQUAL "")
            return()
        endif()
    endif()
    get_property(_mapping_set TARGET "${_target}"
        PROPERTY "MAP_IMPORTED_CONFIG_${_config}" SET)
    get_target_property(_mapped "${_target}" "MAP_IMPORTED_CONFIG_${_config}")
    set(_candidates "")
    if(_mapping_set)
        # Encode defaults separately from configuration names; no sentinel
        # configuration name can collide with the empty mapping element.
        if(_mapped STREQUAL "")
            list(APPEND _candidates d)
        else()
            foreach(_mapped_config IN LISTS _mapped)
                if(_mapped_config STREQUAL "")
                    list(APPEND _candidates d)
                else()
                    string(TOUPPER "${_mapped_config}" _mapped_config)
                    list(APPEND _candidates "c${_mapped_config}")
                endif()
            endforeach()
        endif()
    elseif(_policy STREQUAL OLD)
        list(APPEND _candidates "c${_config}" d)
        foreach(_available IN LISTS _configs)
            list(APPEND _candidates "c${_available}")
        endforeach()
    elseif(NOT _configs STREQUAL "")
        if(_config IN_LIST _configs)
            list(APPEND _candidates "c${_config}")
        else()
            list(GET _configs 0 _available)
            list(APPEND _candidates "c${_available}")
        endif()
    else()
        list(APPEND _candidates "c${_config}")
    endif()

    foreach(_candidate IN LISTS _candidates)
        if(_candidate STREQUAL d)
            set(_suffix "")
            set(_candidate_config "")
        else()
            string(SUBSTRING "${_candidate}" 1 -1 _candidate_config)
            set(_suffix "_${_candidate_config}")
        endif()
        if(_policy STREQUAL NEW AND NOT _configs STREQUAL "")
            if(NOT _candidate_config IN_LIST _configs)
                continue()
            endif()
        endif()
        if(_policy STREQUAL NEW)
            _libdxfrw_locations("${_target}" "${_suffix}" TRUE)
        else()
            _libdxfrw_locations("${_target}" "${_suffix}" FALSE)
        endif()
        if(NOT _LOCATION_SET AND NOT (_type STREQUAL SHARED_LIBRARY AND _IMPLIB_SET))
            if(_policy STREQUAL NEW AND NOT _configs STREQUAL "")
                return()
            endif()
            continue()
        endif()

        # Configuration selection and link-artifact selection are separate.
        # SHARED can link an import library (DLL or non-DLL stub), while
        # STATIC/UNKNOWN ignore IMPORTED_IMPLIB even if it is present.
        if(_type STREQUAL SHARED_LIBRARY)
            if(NOT _IMPLIB_SET)
                get_target_property(_IMPLIB "${_target}" IMPORTED_IMPLIB)
            endif()
            if(NOT "${_IMPLIB}" STREQUAL "" AND NOT _IMPLIB STREQUAL "_IMPLIB-NOTFOUND")
                set(_artifact "${_IMPLIB}")
            elseif(WIN32)
                return() # A DLL is not the archive selected by find_library.
            else()
                set(_artifact "${_LOCATION}")
            endif()
        else()
            set(_artifact "${_LOCATION}")
        endif()
        if(NOT IS_ABSOLUTE "${_artifact}" OR NOT EXISTS "${_artifact}"
            OR IS_DIRECTORY "${_artifact}")
            return()
        endif()
        file(REAL_PATH "${_artifact}" _artifact)
        set(${_result} "${_artifact}" PARENT_SCOPE)
        return()
    endforeach()
endfunction()

function(_libdxfrw_selected_target_matches _target _result)
    set(${_result} FALSE PARENT_SCOPE)
    set(_Libdxfrw_target_reason "unsupported target or selected header binding" PARENT_SCOPE)
    if(NOT TARGET "${_target}")
        return()
    endif()
    get_target_property(_alias "${_target}" ALIASED_TARGET)
    get_property(_alias_set TARGET "${_target}" PROPERTY ALIASED_TARGET SET)
    if(_alias_set)
        set(_target "${_alias}")
    endif()
    get_target_property(_imported "${_target}" IMPORTED)
    get_target_property(_type "${_target}" TYPE)
    if(NOT _imported OR NOT _type MATCHES "^(STATIC|SHARED|UNKNOWN)_LIBRARY$")
        return()
    endif()
    get_target_property(_includes "${_target}" INTERFACE_INCLUDE_DIRECTORIES)
    list(LENGTH _includes _include_count)
    if(NOT _include_count EQUAL 1 OR NOT IS_ABSOLUTE "${_includes}")
        return()
    endif()
    file(REAL_PATH "${_includes}" _includes)
    file(REAL_PATH "${Libdxfrw_INCLUDE_DIR}" _selected_include)
    file(REAL_PATH "${Libdxfrw_LIBRARY}" _selected_library)
    if(NOT _includes STREQUAL _selected_include)
        return()
    endif()
    get_property(_multi GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(_multi)
        set(_consumer_configs "${CMAKE_CONFIGURATION_TYPES}")
    else()
        set(_consumer_configs "${CMAKE_BUILD_TYPE}")
    endif()
    if(_consumer_configs STREQUAL "")
        set(_consumer_configs NOCONFIG)
    endif()
    foreach(_config IN LISTS _consumer_configs)
        string(TOUPPER "${_config}" _config)
        if(_config STREQUAL "")
            set(_config NOCONFIG)
        endif()
        _libdxfrw_target_artifact("${_target}" "${_config}" OLD _old)
        if(NOT _old STREQUAL _selected_library)
            set(_Libdxfrw_target_reason
                "OLD selection unavailable or differs for ${_config}" PARENT_SCOPE)
            return()
        endif()
        if(POLICY CMP0200)
            _libdxfrw_target_artifact("${_target}" "${_config}" NEW _new)
            if(NOT _new STREQUAL _selected_library)
                # Unknown creation policy: both possible selections must bind
                # the selected artifact. Do not guess from the current caller.
                set(_Libdxfrw_target_reason
                    "CMP0200 creation-policy ambiguity for ${_config}" PARENT_SCOPE)
                return()
            endif()
        endif()
    endforeach()
    set(${_result} TRUE PARENT_SCOPE)
endfunction()

function(_libdxfrw_probe_config)
    get_filename_component(_directory "${Libdxfrw_LIBRARY}" DIRECTORY)
    set(_directory "${_directory}/cmake/libdxfrw")
    if(NOT EXISTS "${_directory}/libdxfrwConfig.cmake"
        OR NOT EXISTS "${_directory}/libdxfrwConfigVersion.cmake")
        if(NOT "${Libdxfrw_FIND_VERSION}" STREQUAL "")
            set(Libdxfrw_METADATA_MATCH FALSE PARENT_SCOPE)
            set(Libdxfrw_METADATA_REASON
                "selected library has no co-located version metadata" PARENT_SCOPE)
        endif()
        return()
    endif()

    # Evaluate this exact version file using CMake's documented version-file
    # inputs. No nested find_package search can consult a host DIR cache,
    # package redirect, registry or another prefix, or alter that cache.
    set(PACKAGE_FIND_NAME libdxfrw)
    foreach(_suffix IN ITEMS VERSION VERSION_COMPLETE VERSION_MAJOR VERSION_MINOR VERSION_PATCH
            VERSION_TWEAK VERSION_COUNT VERSION_RANGE VERSION_RANGE_MIN
            VERSION_RANGE_MAX VERSION_MIN VERSION_MIN_MAJOR VERSION_MIN_MINOR
            VERSION_MIN_PATCH VERSION_MIN_TWEAK VERSION_MIN_COUNT VERSION_MAX
            VERSION_MAX_MAJOR VERSION_MAX_MINOR VERSION_MAX_PATCH
            VERSION_MAX_TWEAK VERSION_MAX_COUNT)
        set(PACKAGE_FIND_${_suffix} "${Libdxfrw_FIND_${_suffix}}")
    endforeach()
    # Normal values shadow inherited cache entries without changing them.
    set(PACKAGE_VERSION "")
    set(PACKAGE_VERSION_COMPATIBLE FALSE)
    set(PACKAGE_VERSION_EXACT FALSE)
    set(PACKAGE_VERSION_UNSUITABLE FALSE)
    include("${_directory}/libdxfrwConfigVersion.cmake")
    if(PACKAGE_VERSION_UNSUITABLE
        OR (NOT "${Libdxfrw_FIND_VERSION}" STREQUAL ""
            AND (NOT PACKAGE_VERSION_COMPATIBLE
                OR (Libdxfrw_FIND_VERSION_EXACT AND NOT PACKAGE_VERSION_EXACT))))
        set(Libdxfrw_METADATA_MATCH FALSE PARENT_SCOPE)
        set(Libdxfrw_METADATA_REASON
            "selected package metadata does not satisfy the version request" PARENT_SCOPE)
        return()
    endif()
    file(STRINGS "${Libdxfrw_INCLUDE_DIR}/drw_base.h" _version_line
        LIMIT_COUNT 1 REGEX "^#define[ \t]+DRW_VERSION[ \t]+\"[0-9]+\\.[0-9]+\\.[0-9]+\"[ \t]*$")
    string(REGEX REPLACE ".*\"([^\"]+)\".*" "\\1" _header_version "${_version_line}")
    if(NOT _header_version STREQUAL PACKAGE_VERSION)
        set(Libdxfrw_METADATA_MATCH FALSE PARENT_SCOPE)
        set(Libdxfrw_METADATA_REASON
            "selected header differs from co-located package version" PARENT_SCOPE)
        return()
    endif()
    set(_selected_version "${PACKAGE_VERSION}")

    set(libdxfrw_FIND_QUIETLY TRUE)
    set(libdxfrw_FIND_REQUIRED FALSE)
    set(libdxfrw_FIND_COMPONENTS "")
    # find_dependency uses this name for QUIET/REQUIRED and failure results.
    # The caller is currently in FindLibdxfrw, not a native CONFIG search.
    set(CMAKE_FIND_PACKAGE_NAME libdxfrw)
    include("${_directory}/libdxfrwConfig.cmake")
    if(NOT libdxfrw_FOUND)
        set(Libdxfrw_METADATA_MATCH FALSE PARENT_SCOPE)
        set(Libdxfrw_METADATA_REASON
            "selected package config or its dependencies are unavailable" PARENT_SCOPE)
        return()
    endif()
    set(Libdxfrw_VERSION "${_selected_version}" PARENT_SCOPE)
endfunction()

set(Libdxfrw_METADATA_MATCH TRUE)
set(Libdxfrw_METADATA_REASON "")
set(Libdxfrw_VERSION "")
if(Libdxfrw_LIBRARY AND Libdxfrw_INCLUDE_DIR)
    _libdxfrw_probe_config()
    # Validate existing targets even for the unversioned/manual fallback.
    foreach(_target IN ITEMS libdxfrw::libdxfrw Libdxfrw::Libdxfrw)
        if(TARGET "${_target}")
            _libdxfrw_selected_target_matches("${_target}" _matches)
            if(NOT _matches)
                set(Libdxfrw_METADATA_MATCH FALSE)
                set(Libdxfrw_METADATA_REASON
                    "existing target ${_target}: ${_Libdxfrw_target_reason}")
            endif()
        endif()
    endforeach()
endif()
foreach(_Libdxfrw_component IN LISTS Libdxfrw_FIND_COMPONENTS)
    set(Libdxfrw_${_Libdxfrw_component}_FOUND FALSE)
endforeach()

include(FindPackageHandleStandardArgs)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads QUIET)
# Raw/manual targets also inherit CMAKE_MAP_IMPORTED_CONFIG_<CONFIG> at
# creation. Validate that actual target before advertising FOUND; validating
# only targets which existed on entry would miss this first-discovery path.
if(Libdxfrw_LIBRARY AND Libdxfrw_INCLUDE_DIR AND Threads_FOUND
    AND Libdxfrw_METADATA_MATCH AND NOT TARGET libdxfrw::libdxfrw)
    add_library(libdxfrw::libdxfrw UNKNOWN IMPORTED)
    set_target_properties(libdxfrw::libdxfrw PROPERTIES
        IMPORTED_LOCATION "${Libdxfrw_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Libdxfrw_INCLUDE_DIR}"
        INTERFACE_COMPILE_FEATURES cxx_std_17
        INTERFACE_LINK_LIBRARIES Threads::Threads
    )
    _libdxfrw_selected_target_matches(libdxfrw::libdxfrw _matches)
    if(NOT _matches)
        set(Libdxfrw_METADATA_MATCH FALSE)
        set(Libdxfrw_METADATA_REASON "new manual target: ${_Libdxfrw_target_reason}")
    endif()
endif()
find_package_handle_standard_args(Libdxfrw
    REQUIRED_VARS Libdxfrw_LIBRARY Libdxfrw_INCLUDE_DIR Threads_FOUND Libdxfrw_METADATA_MATCH
    VERSION_VAR Libdxfrw_VERSION HANDLE_VERSION_RANGE HANDLE_COMPONENTS
    REASON_FAILURE_MESSAGE "${Libdxfrw_METADATA_REASON}"
)

if(Libdxfrw_FOUND)
    set(Libdxfrw_INCLUDE_DIRS "${Libdxfrw_INCLUDE_DIR}")
    set(Libdxfrw_LIBRARIES "${Libdxfrw_LIBRARY}" Threads::Threads)
    if(NOT TARGET Libdxfrw::Libdxfrw)
        get_target_property(_Libdxfrw_alias libdxfrw::libdxfrw ALIASED_TARGET)
        get_property(_Libdxfrw_alias_set TARGET libdxfrw::libdxfrw PROPERTY ALIASED_TARGET SET)
        if(_Libdxfrw_alias_set)
            add_library(Libdxfrw::Libdxfrw ALIAS "${_Libdxfrw_alias}")
        else()
            add_library(Libdxfrw::Libdxfrw ALIAS libdxfrw::libdxfrw)
        endif()
    endif()
endif()

mark_as_advanced(Libdxfrw_INCLUDE_DIR Libdxfrw_LIBRARY)
