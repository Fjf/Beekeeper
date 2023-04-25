cmake_minimum_required(VERSION 3.15)
project(Beekeeper VERSION 0.1 DESCRIPTION "Beekeeper" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O3 -march=native -fopenmp")

# Set include directories for builds
include_directories(.)


# Add main.cpp file of project root directory as source file
set(LIB_FILES engine/board.cpp engine/position.h engine/board.h engine/tt.cpp engine/game.cpp engine/game.h engine/tree.cpp engine/tree.cpp engine/tree.h engine/utils.cpp engine/utils.h)


include_directories(${CMAKE_SOURCE_DIR}/engine)
add_library(cxx_hive SHARED ${LIB_FILES})


# Add executable target with source files listed in SOURCE_FILES variable
set(SOURCE_FILES main.cpp)
add_executable(cxx_hive_run ${SOURCE_FILES})
target_link_libraries(cxx_hive_run cxx_hive)


add_executable(cxx_perft perft.cpp)
target_link_libraries(cxx_perft cxx_hive)