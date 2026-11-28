cmake_minimum_required(VERSION 3.28)

project(libdxfrw_semantic_target_adapter LANGUAGES CXX)

function(_libdxfrw_reject_launcher_channels)
    # A toolchain, project include, or source-list include can seed these as
    # normal variables instead of cache entries. Check at each configure
    # boundary where user-controlled CMake code has just run.
    foreach(_channel IN ITEMS
            CMAKE_CXX_COMPILER_ARG1
            CMAKE_CXX_COMPILER_LAUNCHER
            CMAKE_CXX_LINKER_LAUNCHER
            CMAKE_RULE_LAUNCH_COMPILE
            CMAKE_RULE_LAUNCH_LINK)
        if(DEFINED ${_channel} AND NOT "${${_channel}}" STREQUAL "")
            message(FATAL_ERROR
                "unsupported compiler argument/launcher channel: ${_channel}")
        endif()
    endforeach()

    foreach(_property IN ITEMS RULE_LAUNCH_COMPILE RULE_LAUNCH_LINK)
        get_property(_property_set GLOBAL PROPERTY "${_property}" SET)
        if(_property_set)
            get_property(_property_value GLOBAL PROPERTY "${_property}")
            if(NOT "${_property_value}" STREQUAL "")
                message(FATAL_ERROR
                    "unsupported global launcher property: ${_property}")
            endif()
        endif()
    endforeach()
endfunction()

_libdxfrw_reject_launcher_channels()

if(NOT DEFINED LIBDXFRW_SEMANTIC_TARGET_ROOT)
    message(FATAL_ERROR
        "LIBDXFRW_SEMANTIC_TARGET_ROOT must name the extracted "
        "libraries/libdxfrw directory")
endif()
if(NOT DEFINED LIBDXFRW_SEMANTIC_ADAPTER_SOURCE)
    message(FATAL_ERROR
        "LIBDXFRW_SEMANTIC_ADAPTER_SOURCE must name the shared adapter source")
endif()

get_filename_component(LIBDXFRW_SEMANTIC_TARGET_ROOT
    "${LIBDXFRW_SEMANTIC_TARGET_ROOT}" REALPATH)
get_filename_component(LIBDXFRW_SEMANTIC_ADAPTER_SOURCE
    "${LIBDXFRW_SEMANTIC_ADAPTER_SOURCE}" REALPATH)

set(LIBDXFRW_SEMANTIC_SOURCE_LIST
    "${LIBDXFRW_SEMANTIC_TARGET_ROOT}/libdxfrw_sources.cmake")
if(NOT EXISTS "${LIBDXFRW_SEMANTIC_SOURCE_LIST}")
    message(FATAL_ERROR
        "Pinned target source list is missing: ${LIBDXFRW_SEMANTIC_SOURCE_LIST}")
endif()
if(NOT EXISTS "${LIBDXFRW_SEMANTIC_ADAPTER_SOURCE}")
    message(FATAL_ERROR
        "Shared semantic adapter source is missing: "
        "${LIBDXFRW_SEMANTIC_ADAPTER_SOURCE}")
endif()

include("${LIBDXFRW_SEMANTIC_SOURCE_LIST}")
_libdxfrw_reject_launcher_channels()
if(NOT LIBDXFRW_SOURCES)
    message(FATAL_ERROR "Pinned target source list defines no sources")
endif()

# Use fixed output directories for both single- and multi-configuration
# generators.  The receipt generator can therefore identify exactly one
# executable and one static library without guessing generator-specific paths.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config} "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${config} "${CMAKE_BINARY_DIR}/lib")
endforeach()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    set(LIBDXFRW_SEMANTIC_TARGET_WARNING_FLAGS /W3 /bigobj)
    set(LIBDXFRW_SEMANTIC_ADAPTER_WARNING_FLAGS /W3 /WX /bigobj)
else()
    # The locked LibreCAD target's own CMake policy makes its warnings
    # non-fatal.  Keep that immutable package policy for the target archive;
    # the new adapter itself is repository code and remains warning-clean.
    set(LIBDXFRW_SEMANTIC_TARGET_WARNING_FLAGS -Wall -Wextra -pedantic)
    set(LIBDXFRW_SEMANTIC_ADAPTER_WARNING_FLAGS
        -Wall -Wextra -pedantic -Werror)
endif()

add_library(libdxfrw_semantic_target STATIC ${LIBDXFRW_SOURCES})
set_target_properties(libdxfrw_semantic_target PROPERTIES
    OUTPUT_NAME dxfrw_semantic_target)
target_compile_features(libdxfrw_semantic_target PUBLIC cxx_std_17)
target_compile_options(libdxfrw_semantic_target
    PRIVATE ${LIBDXFRW_SEMANTIC_TARGET_WARNING_FLAGS})
target_include_directories(libdxfrw_semantic_target
    PUBLIC "${LIBDXFRW_SEMANTIC_TARGET_ROOT}/src"
    PRIVATE "${LIBDXFRW_SEMANTIC_TARGET_ROOT}/src/intern")

add_executable(libdxfrw_semantic_adapter_target
    "${LIBDXFRW_SEMANTIC_ADAPTER_SOURCE}")
target_compile_features(libdxfrw_semantic_adapter_target PRIVATE cxx_std_17)
target_compile_options(libdxfrw_semantic_adapter_target
    PRIVATE ${LIBDXFRW_SEMANTIC_ADAPTER_WARNING_FLAGS})
target_include_directories(libdxfrw_semantic_adapter_target PRIVATE
    "${LIBDXFRW_SEMANTIC_TARGET_ROOT}/src"
    "${LIBDXFRW_SEMANTIC_TARGET_ROOT}/src/intern")
target_compile_definitions(libdxfrw_semantic_adapter_target PRIVATE
    LIBDXFRW_SEMANTIC_SIDE_TARGET=1
    "LIBDXFRW_SEMANTIC_ADAPTER_NAME=\"libdxfrw_semantic_adapter_target\"")

# CMAKE_* variables normally initialize these target properties, but a
# toolchain/project include can also set them directly after target creation.
# Such a build would not be represented by the direct compile-option receipt.
foreach(_target IN ITEMS
        libdxfrw_semantic_target
        libdxfrw_semantic_adapter_target)
    foreach(_property IN ITEMS CXX_COMPILER_LAUNCHER CXX_LINKER_LAUNCHER)
        get_target_property(_property_value "${_target}" "${_property}")
        if(NOT "${_property_value}" STREQUAL ""
                AND NOT "${_property_value}" MATCHES "-NOTFOUND$")
            message(FATAL_ERROR
                "unsupported target launcher property: ${_target}.${_property}")
        endif()
    endforeach()
endforeach()

if(DEFINED LIBDXFRW_SEMANTIC_ADAPTER_COMMIT)
    target_compile_definitions(libdxfrw_semantic_adapter_target PRIVATE
        "LIBDXFRW_SEMANTIC_ADAPTER_COMMIT=\"${LIBDXFRW_SEMANTIC_ADAPTER_COMMIT}\"")
endif()
if(DEFINED LIBDXFRW_SEMANTIC_CONFIG_DIGEST)
    target_compile_definitions(libdxfrw_semantic_adapter_target PRIVATE
        "LIBDXFRW_SEMANTIC_ADAPTER_CONFIG_DIGEST=\"${LIBDXFRW_SEMANTIC_CONFIG_DIGEST}\"")
endif()

target_link_libraries(libdxfrw_semantic_adapter_target
    PRIVATE libdxfrw_semantic_target)

# Apple ld's map records the exact archive members and SDK stubs selected by
# this executable link. Keep it beside (not in) the adapter artifact; the
# receipt builder parses and hashes the observed inputs after the build.
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin"
        AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_link_options(libdxfrw_semantic_adapter_target PRIVATE
        "-Wl,-map,${CMAKE_BINARY_DIR}/semantic-adapter-link.map")
endif()

# Clang may read default config files that inject options after CMake's
# codemodel has recorded the target fragments.  Disable that implicit channel
# for both the target library and adapter; the receipt validator requires the
# adapter option on Clang-family compilers.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(libdxfrw_semantic_target PRIVATE --no-default-config)
    target_compile_options(libdxfrw_semantic_adapter_target
        PRIVATE --no-default-config)
endif()
