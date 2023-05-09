cmake_minimum_required(VERSION 3.15)
project(Beekeeper VERSION 0.1 DESCRIPTION "Beekeeper" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)


set(default_build_type "Release")
if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
    set(default_build_type "Debug")
endif ()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
            STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif ()

# Default flags
set(CMAKE_CXX_FLAGS "-O3 -fno-lto -march=native")

# Include packages for ML MCTS bindings
find_package(Torch REQUIRED)
find_package(CUDAToolkit REQUIRED)
find_package(PythonLibs REQUIRED)

include_directories(${TORCH_INCLUDE_DIRS})
include_directories(${CUDAToolkit_INCLUDE_DIRS})
include_directories(${PYTHON_INCLUDE_DIRS})


# Set tcmalloc flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}    -fno-builtin-malloc -fno-builtin-calloc    -fno-builtin-realloc -fno-builtin-free    -rdynamic -L/usr/lib/x86_64-linux-gnu/libtcmalloc.so    -fPIC ")

# Create disassembly
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -S -g")

# Set include directories for builds
include_directories(.)

# Add main.cpp file of project root directory as source file
set(HIVE_SOURCES engine/board.cpp engine/position.h engine/board.h engine/tt.cpp engine/game.h engine/tree.cpp engine/tree.cpp engine/tree.h engine/utils.cpp engine/utils.h engine/tree_impl.cpp engine/move.cpp engine/move.h)
set(MCTS_SOURCES ml/ai_mcts.cpp ml/ai_mcts.h engine/constants.h)


include_directories(${CMAKE_SOURCE_DIR}/engine)

# ML MCTS library generating nodes with assistance of neural network.
add_library(cxx_hive_torch SHARED ${HIVE_SOURCES} ${MCTS_SOURCES} )
target_link_libraries(cxx_hive_torch ${TORCH_LIBRARIES})

# Main runner executable, currently not doing much.
add_executable(cxx_hive_run main.cpp ${HIVE_SOURCES} )
target_link_libraries(cxx_hive_run -ltcmalloc)

# Performance tracking executable
add_executable(cxx_perft perft.cpp ${HIVE_SOURCES} )
