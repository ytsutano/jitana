cmake_minimum_required(VERSION 2.6)

project(Jitana)
set(JITANA_VERSION_MAJOR 1)
set(JITANA_VERSION_MINOR 0)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
CHECK_CXX_COMPILER_FLAG("-std=c++1y" COMPILER_SUPPORTS_CXX1Y)
if(COMPILER_SUPPORTS_CXX14)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
elseif(COMPILER_SUPPORTS_CXX1Y)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
else()
    message(FATAL_ERROR
            "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support."
            "Please use a different C++ compiler.")
endif()
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -pedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

include_directories("include")

find_package(Boost COMPONENTS system REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

#-------------------------------------------------------------------------------
# clang-format
#-------------------------------------------------------------------------------

add_custom_target(format
    find include lib tools \\\( -name "*.hpp" -or -name "*.cpp" \\\)
            -type f -exec clang-format -i {} \\+
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

#-------------------------------------------------------------------------------
# Line Count
#-------------------------------------------------------------------------------

add_custom_target(count
    find include lib tools \\\( -name "*.hpp" -or -name "*.cpp" \\\)
            -print0 | xargs -0 wc -l
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

#-------------------------------------------------------------------------------
# jitana library
#-------------------------------------------------------------------------------

add_library(jitana
    lib/jitana/core/virtual_machine.cpp
    lib/jitana/core/class_loader.cpp
    lib/jitana/core/dex_file.cpp
    lib/jitana/core/insn_info.cpp
)

#-------------------------------------------------------------------------------
# jitana-graph
#-------------------------------------------------------------------------------

add_executable(jitana-graph
    tools/jitana-graph/main.cpp
)
target_link_libraries(jitana-graph
    jitana
)

#-------------------------------------------------------------------------------
# jitana-travis
#-------------------------------------------------------------------------------

find_package(OpenGL)
find_package(GLUT)
find_package(Threads)

add_executable(jitana-travis
    tools/jitana-travis/main.cpp
    lib/jitana/util/jdwp.cpp
)
target_link_libraries(jitana-travis
    jitana
    ${Boost_LIBRARIES}
    ${OPENGL_LIBRARY}
    ${GLUT_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
)
add_custom_target(jitana-travis-copy-files
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/jitana-travis/scripts
        ${CMAKE_CURRENT_BINARY_DIR}
)
add_dependencies(jitana-travis jitana-travis-copy-files)

#-------------------------------------------------------------------------------
# Doxygen
#-------------------------------------------------------------------------------

find_package(Doxygen)
if(DOXYGEN_FOUND)
    if(COMPILER_SUPPORTS_CXX14)
        set(CMAKE_DOXYGEN_CLANG_OPTIONS "${CMAKE_DOXYGEN_CLANG_OPTIONS} -std=c++14")
    else()
        set(CMAKE_DOXYGEN_CLANG_OPTIONS "${CMAKE_DOXYGEN_CLANG_OPTIONS} -std=c++1y")
    endif()
    set(CMAKE_DOXYGEN_CLANG_OPTIONS "${CMAKE_DOXYGEN_CLANG_OPTIONS} -stdlib=libc++")
    set(CMAKE_DOXYGEN_CLANG_OPTIONS "${CMAKE_DOXYGEN_CLANG_OPTIONS} --verbose")

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/doxygen/Doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY
    )
    add_custom_target(doxygen
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
endif(DOXYGEN_FOUND)
