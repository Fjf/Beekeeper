cmake_minimum_required(VERSION 3.13)
project(hive VERSION 0.1 DESCRIPTION "hive")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_FLAGS "-o3")

# Add main.cpp file of project root directory as source file
set(SOURCE_FILES main.cpp engine/moves.cpp engine/board.cpp pns/pn_tree.cpp engine/list.cpp pns/pns.cpp mm/mm.cpp engine/node.cpp mm/evaluation.cpp)

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(hive_run ${SOURCE_FILES})
target_compile_definitions(hive_run PRIVATE CENTERED=1)

# Testing executable with TESTING compile flag
add_executable(hive_test ${SOURCE_FILES})
target_compile_definitions(hive_test PRIVATE TESTING=1 CENTERED=1)

set(LIB_FILES engine/board.cpp engine/list.cpp engine/moves.cpp engine/node.cpp mm/mm.cpp mm/evaluation.cpp mm/evaluation.hpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/engine)
add_library(hive SHARED ${LIB_FILES})