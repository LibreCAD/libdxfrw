# Conan CMakeDeps generates its own target instead of loading our installed
# CONFIG target. Add the same portable Threads dependency after that target
# exists; do not guess platform-specific pthread flags in the recipe.
include(CMakeFindDependencyMacro)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_dependency(Threads)

if(NOT TARGET libdxfrw::libdxfrw)
    message(FATAL_ERROR "libdxfrw Conan Threads bridge requires its generated target")
endif()
get_target_property(_libdxfrw_threads_target libdxfrw::libdxfrw ALIASED_TARGET)
if(NOT _libdxfrw_threads_target)
    set(_libdxfrw_threads_target libdxfrw::libdxfrw)
endif()
set_property(TARGET ${_libdxfrw_threads_target} APPEND PROPERTY
    INTERFACE_LINK_LIBRARIES Threads::Threads)
unset(_libdxfrw_threads_target)
