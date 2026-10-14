get_filename_component(DXFRW_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

include(CMakeFindDependencyMacro)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_dependency(Threads)

if(NOT TARGET libdxfrw::libdxfrw)
    include("${DXFRW_CMAKE_DIR}/libdxfrwTargets.cmake")
endif()
