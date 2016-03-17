cmake_minimum_required(VERSION 2.6)

add_executable(jitana-iac
    main.cpp
)
target_link_libraries(jitana-iac
    jitana
)
add_custom_target(jitana-iac-copy-files
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts
        ${CMAKE_CURRENT_BINARY_DIR}
)
add_dependencies(jitana-iac jitana-iac-copy-files)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/output/insn)
