# FindLibdxfrw.cmake
#
# Module-mode fallback for consumers that cannot use the installed CONFIG
# package.  The canonical target name is kept identical to the CONFIG package
# so switching between the two discovery modes does not change consumer code.

find_path(Libdxfrw_INCLUDE_DIR
    NAMES drw_base.h
    PATH_SUFFIXES libdxfrw
)
find_library(Libdxfrw_LIBRARY
    NAMES dxfrw libdxfrw
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libdxfrw
    REQUIRED_VARS Libdxfrw_LIBRARY Libdxfrw_INCLUDE_DIR
)

if(Libdxfrw_FOUND)
    set(Libdxfrw_INCLUDE_DIRS "${Libdxfrw_INCLUDE_DIR}")
    set(Libdxfrw_LIBRARIES "${Libdxfrw_LIBRARY}")
    if(NOT TARGET libdxfrw::libdxfrw)
        add_library(libdxfrw::libdxfrw UNKNOWN IMPORTED)
        set_target_properties(libdxfrw::libdxfrw PROPERTIES
            IMPORTED_LOCATION "${Libdxfrw_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${Libdxfrw_INCLUDE_DIR}"
            INTERFACE_COMPILE_FEATURES cxx_std_17
        )
    endif()
    if(NOT TARGET Libdxfrw::Libdxfrw)
        add_library(Libdxfrw::Libdxfrw ALIAS libdxfrw::libdxfrw)
    endif()
endif()

mark_as_advanced(Libdxfrw_INCLUDE_DIR Libdxfrw_LIBRARY)
