cmake_minimum_required(VERSION 3.15)
project(hive VERSION 0.1 DESCRIPTION "hive")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_FLAGS "-std=gnu99 -O3 -march=native -fopenmp -static")

# Add main.cpp file of project root directory as source file
set(LIB_FILES engine/board.c engine/list.c engine/moves.c engine/node.c engine/tt.c engine/utils.c)
set(SOURCE_FILES main.c engine/moves.c engine/moves.h engine/board.c engine/board.h pns/pn_tree.c pns/pn_tree.h engine/list.c engine/list.h pns/pns.c pns/pns.h mm/mm.c mm/mm.h engine/node.c engine/node.h timing/timing.h timing/timing.c mm/evaluation.c mm/evaluation.h engine/tt.c engine/tt.h mcts/mcts.c mcts/mcts.h)
# Add executable target with source files listed in SOURCE_FILES variable
add_executable(hive_run ${SOURCE_FILES} engine/utils.c engine/utils.h puzzles.c puzzles.h)
# Add math library to compilation for MCTS
target_link_libraries(hive_run m)
target_compile_definitions(hive_run PRIVATE CENTERED=1 MAX_TURNS=180)


add_executable(perft ${LIB_FILES} perft.c engine/utils.h)
target_compile_definitions(hive_run PRIVATE CENTERED=1 MAX_TURNS=180)


# Testing executable with TESTING compile flag
#add_executable(hive_test ${SOURCE_FILES})
#target_compile_definitions(hive_test PRIVATE TESTING=1 CENTERED=1)

add_compile_definitions(hive CENTERED=1 MAX_TURNS=180)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/engine)
add_library(hive SHARED ${LIB_FILES} mm/mm.c mm/evaluation.c mcts/mcts.c)