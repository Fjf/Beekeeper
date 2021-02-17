cmake_minimum_required(VERSION 3.13)
project(hive VERSION 0.1 DESCRIPTION "hive")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_FLAGS "-o3")

# Add main.cpp file of project root directory as source file
set(SOURCE_FILES main.c engine/moves.c engine/moves.h engine/board.c engine/board.h pns/pn_tree.c pns/pn_tree.h engine/list.c engine/list.h pns/pns.c pns/pns.h mm/mm.c mm/mm.h engine/node.c engine/node.h timing/timing.h timing/timing.c)

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(hive_run ${SOURCE_FILES})

set(LIB_FILES engine/board.c engine/list.c engine/moves.c engine/node.c mm/mm.c timing/timing.c)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/engine)
add_library(hive SHARED ${LIB_FILES})