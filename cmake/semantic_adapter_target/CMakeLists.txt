cmake_minimum_required(VERSION 3.10)

project(libdxfrw_semantic_target_adapter LANGUAGES CXX)

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
