cmake_minimum_required(VERSION 2.6)

project(Jitana C CXX)
set(Jitana_VERSION_MAJOR 1)
set(Jitana_VERSION_MINOR 0)

if(${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR})
    message(FATAL_ERROR "Please build in a separate directory." )
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E copy
    ${PROJECT_SOURCE_DIR}/scripts/git/pre-commit
    ${PROJECT_SOURCE_DIR}/.git/hooks
)

include(CheckCXXCompilerFlag)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    # Avoid the variadic template bugs in MSVC.
    add_definitions(-DBOOST_NO_CXX11_VARIADIC_TEMPLATES)
else()
    check_cxx_compiler_flag("-std=c++14" COMPILER_SUPPORTS_CXX14)
    check_cxx_compiler_flag("-std=c++1y" COMPILER_SUPPORTS_CXX1Y)
    if(COMPILER_SUPPORTS_CXX14)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    elseif(COMPILER_SUPPORTS_CXX1Y)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
    else()
        message(FATAL_ERROR
                "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support."
                "Please use a different C++ compiler.")
    endif()
    if(${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -pedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
endif()

include_directories("include")

set(BOOST_MIN_VERSION "1.54.0")
find_package(Boost ${BOOST_MIN_VERSION}
    COMPONENTS system iostreams REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

#-------------------------------------------------------------------------------
# clang-format
#-------------------------------------------------------------------------------

add_custom_target(format
    find include lib tools test \\\( -name "*.hpp" -or -name "*.cpp" \\\)
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
    include/jitana/algorithm/monotonic_dataflow.hpp
    include/jitana/algorithm/unique_sort.hpp
    include/jitana/analysis/call_graph.hpp
    include/jitana/analysis/data_flow.hpp
    include/jitana/analysis/resource_sharing.hpp
    include/jitana/analysis_graph/contextual_call_graph.hpp
    include/jitana/analysis_graph/pointer_assignment_graph.hpp
    include/jitana/analysis_graph/resource_sharing_graph.hpp
    include/jitana/vm_core/access_flags.hpp
    include/jitana/vm_core/class_loader.hpp
    include/jitana/vm_core/dex_file.hpp
    include/jitana/vm_core/dex_raw_types.hpp
    include/jitana/vm_core/hdl.hpp
    include/jitana/vm_core/idx.hpp
    include/jitana/vm_core/insn.hpp
    include/jitana/vm_core/insn_info.hpp
    include/jitana/vm_core/opcode.hpp
    include/jitana/vm_core/virtual_machine.hpp
    include/jitana/vm_graph/class_graph.hpp
    include/jitana/vm_graph/edge_filtered_graph.hpp
    include/jitana/vm_graph/field_graph.hpp
    include/jitana/vm_graph/graphviz.hpp
    include/jitana/vm_graph/graph_common.hpp
    include/jitana/vm_graph/insn_graph.hpp
    lib/jitana/analysis/points_to.cpp
    lib/jitana/util/jdwp.cpp
    lib/jitana/vm_core/class_loader.cpp
    lib/jitana/vm_core/dex_file.cpp
    lib/jitana/vm_core/insn_info.cpp
    lib/jitana/vm_core/virtual_machine.cpp
)
target_link_libraries(jitana
    ${Boost_LIBRARIES}
)

#-------------------------------------------------------------------------------
# Tools
#-------------------------------------------------------------------------------

file(GLOB TOOL_DIRS tools/*)
foreach(TOOL_DIR ${TOOL_DIRS})
    if(IS_DIRECTORY ${TOOL_DIR})
        add_subdirectory(${TOOL_DIR})
    endif()
endforeach()

#-------------------------------------------------------------------------------
# Testing
#-------------------------------------------------------------------------------

enable_testing()

add_library(unit-test
    test/unit_test.cpp
)

file(GLOB TEST_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/test_*.cpp)

# Run through each source
foreach(SRC ${TEST_SRCS})
    # Extract the filename without an extension.
    get_filename_component(TEST_EXEC_NAME ${SRC} NAME_WE)
    string(REGEX REPLACE "_" "-" TEST_EXEC_NAME ${TEST_EXEC_NAME})

    list(APPEND TEST_EXEC_NAME_LIST ${TEST_EXEC_NAME})

    add_executable(${TEST_EXEC_NAME}
        ${SRC}
    )
    target_link_libraries(${TEST_EXEC_NAME}
        jitana
        unit-test
    )

    file(READ "${SRC}" SOURCE_FILE_CONTENTS)
    string(REGEX MATCHALL "BOOST_AUTO_TEST_CASE\\( *([A-Za-z_0-9]+) *\\)"
        FOUND_TESTS ${SOURCE_FILE_CONTENTS})

    foreach(HIT ${FOUND_TESTS})
        string(REGEX REPLACE ".*\\( *([A-Za-z_0-9]+) *\\).*" "\\1"
            TEST_NAME ${HIT})

        set_target_properties(${TEST_EXEC_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test)

        add_test(NAME "${TEST_EXEC_NAME}.${TEST_NAME}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test
            COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test/${TEST_EXEC_NAME}
                --run_test=${TEST_NAME} --catch_system_error=yes)
    endforeach()
endforeach(SRC)

add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND}
    DEPENDS ${TEST_EXEC_NAME_LIST}
)

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
