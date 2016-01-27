cmake_minimum_required(VERSION 2.6)

add_executable(jitana-bench
    main.cpp
)
target_link_libraries(jitana-bench
    jitana
)

add_custom_target(jitana-bench-copy-files
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts
        ${CMAKE_CURRENT_BINARY_DIR}
)
add_dependencies(jitana-bench jitana-bench-copy-files)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/output/insn)
