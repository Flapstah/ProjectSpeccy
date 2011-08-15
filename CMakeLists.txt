# Macro to print some message to stdout, useful for debugging purpose.
MACRO(DBG_MSG _MSG)
  MESSAGE(STATUS "${CMAKE_CURRENT_LIST_FILE}(${CMAKE_CURRENT_LIST_LINE}): ${_MSG}")
ENDMACRO(DBG_MSG)

# The name of our project is "test". CMakeLists files in this project can
# refer to the root source directory of the project as ${test_SOURCE_DIR} and
# to the root binary directory of the project as ${test_BINARY_DIR}.
cmake_minimum_required (VERSION 2.6)
project (test)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
#set(LINK_FLAGS "-g")
#set(CMAKE_CXX_FLAGS "-g")
set(CMAKE_BUILD_TYPE Debug)

find_package(GLFW REQUIRED)
include_directories(${GLFW_INCLUDE_DIR})
set(LIBS ${LIBS} ${GLFW_LIBRARY})

include_directories ("${test_SOURCE_DIR}/engine/independent")
if (WIN32)
	include_directories ("${test_SOURCE_DIR}/engine/windows")
	set(LIBS ${LIBS} /mingw/lib/libopengl32.a /mingw/lib/libglu32.a)
endif (WIN32)
if (UNIX)
	include_directories ("${test_SOURCE_DIR}/engine/linux")
	set(LIBS ${LIBS} GL GLU)
endif (UNIX)

add_executable (test main.cpp display.cpp keyboard.cpp zxspectrum.cpp z80.cpp)
#DBG_MSG("LIBS = [${LIBS}]")
target_link_libraries (test ${LIBS})

