cmake_minimum_required(VERSION 2.6)

find_package(OpenGL)
find_package(GLUT)
find_package(Threads)

add_executable(jitana-travis
    main.cpp
)
target_link_libraries(jitana-travis
    jitana
    ${OPENGL_LIBRARY}
    ${GLUT_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
)
add_custom_target(jitana-travis-copy-files
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts
        ${CMAKE_CURRENT_BINARY_DIR}
)
add_dependencies(jitana-travis jitana-travis-copy-files)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/output/insn)
