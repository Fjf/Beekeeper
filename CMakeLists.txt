cmake_minimum_required(VERSION 3.13)  # CMake version check
project(hive)               # Create project "simple_example"
set(CMAKE_CXX_STANDARD 14)            # Enable c++14 standard
set(CMAKE_C_FLAGS "-o3")

# Add main.cpp file of project root directory as source file
set(SOURCE_FILES main.c moves.c moves.h board.c board.h pn_tree.c pn_tree.h list.c list.h pns.c pns.h)

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(hive ${SOURCE_FILES})