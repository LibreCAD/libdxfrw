get_filename_component(DXFRW_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

if(NOT TARGET DXFRW::dxfrw)
    include("${DXFRW_CMAKE_DIR}/DXFRWTargets.cmake")
endif()