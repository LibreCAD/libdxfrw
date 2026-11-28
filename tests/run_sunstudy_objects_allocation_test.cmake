foreach(version IN ITEMS AC1024 AC1027 AC1032)
    set(source "${OUTPUT_DIR}/sunstudy-objects-allocation-${version}-source.dxf")
    set(destination "${OUTPUT_DIR}/sunstudy-objects-allocation-${version}-output.dxf")
    file(REMOVE "${source}" "${destination}")

    execute_process(
        COMMAND "${PYTHON}" "${AUTHOR}" author "${source}" --version "${version}"
        RESULT_VARIABLE authored)
    if(NOT authored EQUAL 0)
        message(FATAL_ERROR "SUNSTUDY ${version} allocation source author failed: ${authored}")
    endif()

    execute_process(
        COMMAND "${ALLOCATION_TEST}" "${source}" "${destination}" "${version}"
        RESULT_VARIABLE result)
    file(REMOVE "${source}" "${destination}")
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "SUNSTUDY ${version} OBJECTS allocation retry failed: ${result}")
    endif()
endforeach()
