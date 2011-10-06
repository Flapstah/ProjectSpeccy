# Locate the glfw library
# This module defines the following variables:
# GLFW_LIBRARY, the name of the library;
# GLFW_INCLUDE_DIR, where to find glfw include files.
# GLFW_FOUND, true if both the GLFW_LIBRARY and GLFW_INCLUDE_DIR have been found.
#
# To help locate the library and include file, you could define an environment variable called
# GLFW_ROOT which points to the root of the glfw library installation. This is pretty useful
# on a Windows platform.
#
#
# Usage example to compile an "executable" target to the glfw library:
#
# FIND_PACKAGE (glfw REQUIRED)
# INCLUDE_DIRECTORIES (${GLFW_INCLUDE_DIR})
# ADD_EXECUTABLE (executable ${EXECUTABLE_SRCS})
# TARGET_LINK_LIBRARIES (executable ${GLFW_LIBRARY})
#
# TODO:
# Allow the user to select to link to a shared library or to a static library.


#Search for the include file...
if (WIN32)
	#Don't seem to be able to find this on Windows (installation/config error?)
	#set it directly
	set(GLFW_INCLUDE_DIR "/usr/local/include/")
endif (WIN32)
if (UNIX)
	FIND_PATH(GLFW_INCLUDE_DIR GL/glfw.h DOC "Path to GLFW include directory."
		HINTS
		$ENV{GLFW_ROOT}
		PATH_SUFFIX include #For finding the include file under the root of the glfw expanded archive, typically on Windows.
		PATHS
		/usr/include/
		/usr/local/include/
		${PROJECT_ROOT_DIR}/3pp/include/ # added by ptr
		${PROJECT_ROOT_DIR}/3pp/include/GL/ # added by ptr
		)
endif (UNIX)
#DBG_MSG("GLFW_INCLUDE_DIR = ${GLFW_INCLUDE_DIR}")

if (WIN32)
	#Don't seem to be able to find this on Windows (installation/config error?)
	#set it directly
	set(GLFW_LIBRARY "/usr/local/lib/libglfw.a")
endif (WIN32)
if (UNIX)
	FIND_LIBRARY(GLFW_LIBRARY DOC "Absolute path to GLFW library."
		NAMES glfw
		HINTS
		$ENV{GLFW_ROOT}
		PATH_SUFFIXES lib/win32 #For finding the library file under the root of the glfw expanded archive, typically on Windows.
		PATHS
		/usr/lib
		/usr/local/lib
		${PROJECT_ROOT_DIR}/3pp/lib/ # added by ptr
		)
endif (UNIX)
#DBG_MSG("GLFW_LIBRARY = ${GLFW_LIBRARY}")

SET(GLFW_FOUND 0)
IF(GLFW_LIBRARY AND GLFW_INCLUDE_DIR)
  SET(GLFW_FOUND 1)
ENDIF(GLFW_LIBRARY AND GLFW_INCLUDE_DIR)


