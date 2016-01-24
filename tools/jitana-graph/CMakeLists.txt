cmake_minimum_required(VERSION 2.6)

add_executable(jitana-graph
    main.cpp
)
target_link_libraries(jitana-graph
    jitana
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/output/insn)
