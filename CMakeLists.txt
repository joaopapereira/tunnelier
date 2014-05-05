CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
#####################################
## Option part
#####################################
option(compile_with_debug "Build library with debug." OFF)
option(compile_with_tests "Build tests for application." OFF)

#####################################
## Definition of environment
#####################################
IF( DEFINED ENV{INSTALL_LIB_DIR} )
   SET(INSTALL_LIBS $ENV{INSTALL_LIB_DIR}/ )
ELSE()
   SET(INSTALL_LIBS ${CMAKE_HOME_DIRECTORY}/lib )
ENDIF()
IF( DEFINED ENV{INSTALL_INC_DIR} )
   SET(INSTALL_INCS $ENV{INSTALL_INC_DIR}/ )
ELSE()
   SET(INSTALL_INCS ${CMAKE_HOME_DIRECTORY}/include )
ENDIF()


SET(LIBRARY_OUTPUT_PATH .)
SET(EXECUTABLE_OUTPUT_PATH .)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_FIND_LIBRARY_PREFIXES Find)
set(CMAKE_FIND_LIBRARY_SUFFIXES cmake)

message("Before searching for LoggerLib")
#FIND_LIBRARY(JPLoggerStatic REQUIRED)
#FIND_LIBRARY( LOGGER_LIBRARY NAMES JPLoggerStatic
#                PATHS ${LOCAL_LIBS} ${INSTALL_LIBS} )
#message("Library at: ${LOGGER_LIBRARY}")
#message("Before searching for LoggerLib")
#include_directories(${logger_INCLUDE_DIRS})

# build dependencies.
#set(logger_DIR
#  ${CMAKE_CURRENT_SOURCE_DIR}/modules/logger
#)
#find_package(JPLogger)
#####################################
## Project name
#####################################
PROJECT ( Tunnelier CXX )

#####################################
## Include directories 
INCLUDE_DIRECTORIES( include modules/logger/include lib ${INSTALL_LIBS} ${INSTALL_INCS})
ADD_DEFINITIONS( -std=c++11 -DUSE_DEBUG )

#####################################
# Search for needed lib
#####################################
FIND_PACKAGE(Event REQUIRED)
SET( LIBS ${LIBS} ${Event_LIBRARIES})
FIND_PACKAGE(SSH REQUIRED)
SET( LIBS ${LIBS} ${SSH_LIBRARIES})
message("Writting the libs")
message("${LIBS}")
message("Writting the install libs")
message(${INSTALL_LIBS})



#####################################
## Folders to be build
#####################################
ADD_SUBDIRECTORY( src/jsoncpp lib )
ADD_SUBDIRECTORY( src/tunnels lib )
ADD_SUBDIRECTORY( modules/logger lib )
ADD_SUBDIRECTORY( src bin )
#ADD_SUBDIRECTORY( exampleProgram bin )
if( compile_with_tests )
  ADD_SUBDIRECTORY( tests bin/tests)
endif()


#####################################
# Instalation section
#####################################

#####################################
# Others
#####################################
IF( UNIX )

    

   #ADD_CUSTOM_TARGET( distclean @@echo cleaning for source distribution)
  SET(DISTCLEANED
   cmake.depends
   cmake.check_depends
   CMakeCache.txt
   cmake.check_cache
   cmake*.cmake
   Makefile
   core core.*
   gmon.out
   *~
   lib
   CMakeFiles
   bin
   install_*
   cmake_install.cmake 
   doc/docs/

  )

  ADD_CUSTOM_COMMAND(
    DEPENDS clean
    COMMENT "distribution clean"
    COMMAND rm
    ARGS    -Rf CMakeTmp ${DISTCLEANED}
    TARGET  distclean
  )

ENDIF( UNIX )
if( compile_with_debug )
	set(CMAKE_CXX_FLAGS "-DDEBUG=1")
	set(CMAKE_BUILD_TYPE Debug)
endif()

#####################################
# add a target to generate API documentation with Doxygen
#####################################
find_package(Doxygen)
if(DOXYGEN_FOUND)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doc/tunnelier.dox.in ${CMAKE_CURRENT_BINARY_DIR}/doc/tunnelier.dox @ONLY)
	add_custom_target(docs_tunnelier
			${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doc/thrpool.dox
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			COMMENT "Generating Tunnelier documentation with Doxygen" VERBATIM
)
endif(DOXYGEN_FOUND)
#
