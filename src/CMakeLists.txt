CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET( jsoncpp_lib_srcs jsoncpp.cpp )
include_directories( include/ ${INSTALL_INCS})
ADD_LIBRARY( jsoncpp SHARED ${jsoncpp_lib_srcs})

SET(tunnelier_src tunnelier.cpp requestHandler.cpp SharedMemory.cpp) 
ADD_EXECUTABLE( tunnelier ${tunnelier_src})
TARGET_LINK_LIBRARIES(tunnelier ${LIBS} event jsoncpp)

