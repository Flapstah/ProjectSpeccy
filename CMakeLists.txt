# Macro to print some message to stdout, useful for debugging purpose.
MACRO(DBG_MSG _MSG)
FOREACH(_MSG_STRING ${_MSG})
  MESSAGE(STATUS "${CMAKE_CURRENT_LIST_FILE}(${CMAKE_CURRENT_LIST_LINE}): ${_MSG_STRING}")
ENDFOREACH()
ENDMACRO(DBG_MSG)

#
# Build type
#
# If the user specifies -DCMAKE_BUILD_TYPE on the command line, take their definition
# and dump it in the cache along with proper documentation, otherwise set CMAKE_BUILD_TYPE
# to Debug prior to calling PROJECT()
#
set(CMAKE_BUILD_TYPE_DOCUMENTATION "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release")
IF(DEFINED CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING ${CMAKE_BUILD_TYPE_DOCUMENTATION})
ELSE()
   SET(CMAKE_BUILD_TYPE Debug CACHE STRING ${CMAKE_BUILD_TYPE_DOCUMENTATION})
ENDIF()

# The name of our project is "test". CMakeLists files in this project can
# refer to the root source directory of the project as ${test_SOURCE_DIR} and
# to the root binary directory of the project as ${test_BINARY_DIR}.
cmake_minimum_required (VERSION 2.6)
project (test)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# Compiler flags
#set(LINK_FLAGS "-g")
#set(CMAKE_CXX_FLAGS "-g")
#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Release)

# Platform includes
set(PLATFORM_INCLUDES "${test_SOURCE_DIR}/engine/independent")
if (WIN32)
	set(PLATFORM_INCLUDES ${PLATFORM_INCLUDES} "${test_SOURCE_DIR}/engine/windows")
endif (WIN32)
if (UNIX)
	set(PLATFORM_INCLUDES ${PLATFORM_INCLUDES} "${test_SOURCE_DIR}/engine/linux")
endif (UNIX)

find_package(OpenGL REQUIRED)
if (WIN32)
	# Can't seem to use find_package for GLFW or OpenAL on my Windows 7 box...
	set(OPENAL_INCLUDE_DIR "D:/msysgit/msysgit/local/include")
	set(OPENAL_LIBRARY "D:/msysgit/local/lib/OpenAL32.lib")
	set(GLFW_INCLUDE_DIR "D:/msysgit/msysgit/local/include")
	set(GLFW_LIBRARY "D:/msysgit/local/lib/libglfw.a")
endif (WIN32)
if (UNIX)
	find_package(GLFW REQUIRED)
	find_package(OpenAL REQUIRED)
endif (UNIX)

include_directories(${PLATFORM_INCLUDES} ${GLFW_INCLUDE_DIR} ${OPENAL_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
set(LIBS ${LIBS} ${GLFW_LIBRARY} ${OPENAL_LIBRARY} ${OPENGL_LIBRARY})


#get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
#DBG_MSG("INCLUDE_DIRECTORIES = [${dirs}]")
#DBG_MSG("LIBS = [${LIBS}]")
#DBG_MSG("PLATFORM_INCLUDES = [${PLATFORM_INCLUDES}]")

add_executable (test main.cpp display.cpp keyboard.cpp sound.cpp zxspectrum.cpp z80.cpp)
target_link_libraries (test ${LIBS})

