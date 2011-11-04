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
#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Release)

find_package(GLFW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLU REQUIRED)
if (WIN32)
	# Can't seem to use find_package for OpenAL on my Windows 7 box...
	set(OPENAL_INCLUDE_DIR "/local/include/AL")
	set(OPENAL_LIBRARY "/local/lib/OpenAL32.lib")
endif (WIN32)
if (UNIX)
find_package(OpenAL REQUIRED)
endif (UNIX)

include_directories(${GLFW_INCLUDE_DIR} ${OPENAL_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${GLU_INCLUDE_DIR})
set(LIBS ${LIBS} ${GLFW_LIBRARY} ${OPENAL_LIBRARY} ${OPENGL_LIBRARY} ${GLU_LIBRARY})

include_directories ("${test_SOURCE_DIR}/engine/independent")
if (WIN32)
	include_directories ("${test_SOURCE_DIR}/engine/windows")
endif (WIN32)
if (UNIX)
	include_directories ("${test_SOURCE_DIR}/engine/linux")
	# set(LIBS ${LIBS} GL GLU)
endif (UNIX)

add_executable (test main.cpp display.cpp keyboard.cpp sound.cpp zxspectrum.cpp z80.cpp)
#DBG_MSG("LIBS = [${LIBS}]")
target_link_libraries (test ${LIBS})

