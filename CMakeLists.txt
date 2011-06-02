# The name of our project is "test". CMakeLists files in this project can
# refer to the root source directory of the project as ${TEST_SOURCE_DIR} and
# to the root binary directory of the project as ${TEST_BINARY_DIR}.
cmake_minimum_required (VERSION 2.6)
project (test)

include_directories ("${test_SOURCE_DIR}/engine/" "${test_SOURCE_DIR}/engine/linux")

add_executable (test main.cpp render.cpp)
target_link_libraries (test glfw GLU)

